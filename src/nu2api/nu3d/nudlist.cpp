#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuhspecial.h"
#include <string.h>
// nudlist.cpp — Display-list manager
//
// Transcribed from libTTapp.so (Android x86). Builds and executes the
// per-material / per-scene display-list chains the renderer consumes.
//
// Each chain is a sequence of 16-byte `nudisplaylistitem_s` records:
//   id = 0 (CNT)  — no-op, advance to next record
//   id = 1 (NEXT) — jump to `item->next`
//   id = 3 (CALL) — dispatch `handler[item->type](item->next)`
//   id >=4 (RET)  — terminate execution
// `NuDisplayListExecute` walks a chain until RET or a NEXT that leaves the
// linear run. Double-buffered `clip_used` / `mtl_used` bitsets and the
// `render_buffer` toggle drive the per-frame visibility updates.

#include "nu2api/nu3d/nudlist.h"

#include "decomp.h"
#include "nu2api/nu3d/nutex.h"

#include "nu2api/nucore/numem.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/numtl.h"

#include <cfloat>

extern i32 numtl_renderplane;

// ──────────────────────────────────────────────────────────────────────────────
// Constants
// ──────────────────────────────────────────────────────────────────────────────

static constexpr usize kItemSize = 0x10;

static constexpr u8 kItemId_Cnt = 0;
static constexpr u8 kItemId_Next = 1;
static constexpr u8 kItemId_Call = 3;
static constexpr u8 kItemId_Ret = 4;

static constexpr u8 kItemType_Mtl = 0x80;
static constexpr u8 kItemType_Terminator = 0x84;
static constexpr u8 kItemType_Nop = 0x87;
static constexpr u8 kItemType_Link = 0x8d;

// ──────────────────────────────────────────────────────────────────────────────
// Globals
// ──────────────────────────────────────────────────────────────────────────────

extern VARIPTR rndrstream_end;
extern VARIPTR rndrstream_free;

extern "C" {
    VARIPTR *display_list_buffer = nullptr;
    VARIPTR *display_list_buffer_end = nullptr;

    // Manager lives in original bss @0x11a0080 (0x604 bytes).
    NUDLIST_MANAGER global_dlist_manager = {0};
}

// Capture debug state (bss @0x11a0070 / @0x11a0068).
extern "C" {
    i32 do_capture;
    i32 capture_fh;
    i32 nudlist_debug_level = 2;
    void NuHtmlBegin(void *file);
    void NuHtmlBanner(void);
    void NuHtmlEnd(void);
}

// The original retains reads of this local BSS control despite having no
// program-side setter. Preserve its externally observable debug accesses.
static volatile i32 capture_dlist;

extern "C" void NuDisplayListCaptureBegin(void) {
    i32 request = capture_dlist;
    if (request) {
        nudlist_debug_level = request - 1;
        capture_dlist = 0;
        do_capture = 1;
        capture_fh = NuFileOpen("dlist.htm", NUFILE_WRITE);
        NuHtmlBegin((void *)(isize)capture_fh);
        NuHtmlWrite("<body bgcolor=#C0C0C0>");
    }
}

extern "C" void NuDisplayListCaptureEnd(void) {
    if (do_capture) {
        NuHtmlBanner();
        NuHtmlEnd();
        NuFileClose(capture_fh);
        do_capture = 0;
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Dispatch tables
// ──────────────────────────────────────────────────────────────────────────────


// ──────────────────────────────────────────────────────────────────────────────
// Item helpers
// ──────────────────────────────────────────────────────────────────────────────

static inline void SetItemId(nudisplaylistitem_s *item, u8 id) {
    item->id = id;
}

static inline void SetItemNext(nudisplaylistitem_s *item, void *next) {
    item->next = next;
}

static nudisplaylistitem_s *AddCallItem(nudisplaylist_s *list, u8 type, void *next) {
    nudisplaylistitem_s *item = list->items;
    item->type = type;
    item->id = kItemId_Call;
    item->next = next;
    list->items = reinterpret_cast<nudisplaylistitem_s *>(reinterpret_cast<u8 *>(list->items) + kItemSize);
    return reinterpret_cast<nudisplaylistitem_s *>(reinterpret_cast<u8 *>(list->items) - kItemSize);
}

// ──────────────────────────────────────────────────────────────────────────────
// Executor
// ──────────────────────────────────────────────────────────────────────────────

// Original 0x2f1180: reset every material-list head and make all clipping
// objects visible in the current frame's two-bit visibility map.
void NuDisplaySceneUnclip(NUDLDLISTSCENE *scene) {
    for (i32 i = 0; i < static_cast<i32>(scene->nmtls); ++i) {
        scene->dlist_mtls[i]->mtl_item->id = 0;
    }
    for (i32 i = 0; i < scene->nclip_objects; ++i) {
        scene->clip_used[scene->render_buffer >> 7][i >> 2] |= static_cast<u8>(1u << (2 * (i & 3)));
    }
}

// Original 0x2f11f0: reset the scene's per-object near/far clip ranges.
extern "C" void NuInvalidateClipRanges(NUDLDLISTSCENE *scene) {
    for (i32 index = 0; index < scene->nclip_objects; ++index) {
        if (scene->lod_ranges[index] != 0.0f) {
            scene->lod_ranges[index] = FLT_MAX;
        }
        scene->far_clip_ranges[index] = FLT_MAX;
    }
}

extern "C" void NuDisplayListExecute(nudisplaylistitem_s *item, const nudl_handler_fn *item_table) {
    // `item_table` points at the entry for type 0x80.
    for (;;) {
        switch (item->id) {
        case kItemId_Next:
            item = static_cast<nudisplaylistitem_s *>(item->next);
            break;
        case kItemId_Cnt:
            ++item;
            break;
        case kItemId_Call: {
            auto handler = item_table[static_cast<u32>(item->type) - kItemType_Mtl];
            if (handler)
                handler(item->next);
            ++item;
            break;
        }
        default:
            // RET (and any unexpected id >=2 other than CALL) terminates.
            return;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Debug capture
extern "C" void NuDisplayListDebugToFile(NUDISPLAYLISTITEM *item, i32 file) {
    NuHtmlWrite("<font face=courier new>\n");
    if (item) {
        i32 index = 0;
        do {
            DisplayListPrintItem(item, index, 0, nullptr, file);
            if (item->id == kItemId_Next)
                item = (NUDISPLAYLISTITEM *)item->next;
            else
                ++item;
            ++index;
        } while (item->type != kItemType_Terminator);
        DisplayListPrintItem(item, index, 0, nullptr, file);
    }
}
// ──────────────────────────────────────────────────────────────────────────────

extern "C" void NuDisplayListCaptureSortPriority(nusortpri_s *sort_pri) {
    if (!do_capture) {
        return;
    }

    const char *name = nullptr;
    if (sort_pri->display_scene) {
        name = sort_pri->display_scene->name;
    }
    if (name == nullptr) {
        NuHtmlHeading1("Display Scene : UNKNOWN (sort: %d)", sort_pri->sort_pri);
    } else {
        NuHtmlHeading1("Display Scene : %s (sort: %d)", name, sort_pri->sort_pri);
    }
    NuHtmlWrite("<font face=courier new>\n");

    nudisplaylistitem_s *item = sort_pri->items;
    if (item == nullptr) {
        return;
    }

    i32 idx = 0;
    i32 printed_idx = 0;
    bool hit_terminator = false;
    do {
        while (true) {
            printed_idx = idx;
            DisplayListPrintItem(item, printed_idx, 0, nullptr, capture_fh);
            if (item->id != kItemId_Next) {
                break;
            }
            item = static_cast<nudisplaylistitem_s *>(item->next);
            ++idx;
            if (item->type == kItemType_Terminator) {
                hit_terminator = true;
                break;
            }
        }
        if (hit_terminator) {
            break;
        }
        ++item;
        idx = printed_idx + 1;
    } while (item->type != kItemType_Terminator);
    DisplayListPrintItem(item, printed_idx + 1, 0, nullptr, capture_fh);
}

// ──────────────────────────────────────────────────────────────────────────────
// Per-material reset
// ──────────────────────────────────────────────────────────────────────────────

extern "C" void NuDisplayListReset(nudisplaylist_s *dl) {
    nurndrstate_s *st = dl->state;
    st->mtl = nullptr;
    st->tex_id = -1;
    st->global_id = -1;
    st->lights_id = -1;
    st->camera_id = -1;
    st->fog_id = -1;
    st->konst_id = -1;
    st->reflection_id = -1;
    dl->mtl_last = dl->first;
}

static void ResetRenderStateCache(nurndrstate_s *st) {
    st->mtl = nullptr;
    st->tex_id = -1;
    st->global_id = -1;
    st->lights_id = -1;
    st->camera_id = -1;
    st->fog_id = -1;
    st->konst_id = -1;
    st->reflection_id = -1;
}

// ──────────────────────────────────────────────────────────────────────────────
// Frame setup
// ──────────────────────────────────────────────────────────────────────────────

// Clip bitsets hold two bits per clip object, followed by a sentinel block.
static i32 ClipUsedBlockCount(i32 nclip) {
    return (((nclip + 7) / 8) * 2) / 16 + 1;
}

static void ResetSceneBeforeFrame(nudisplayscene_s *scene, bool gated) {
    if (gated && (scene->flags & 6) == 0) {
        return;
    }

    if (scene->nmtls != 0) {
        NuMemSet128(scene->mtl_used[scene->render_buffer >> 7], 0, ((scene->nmtls + 7) >> 7) + 1);
    }

    if (scene->nclip_objects != 0) {
        i32 blocks = ClipUsedBlockCount(scene->nclip_objects);
        NuMemSet128(scene->clip_used[scene->render_buffer >> 7], 0, blocks);
        if (!gated) {
            // Ungated variant re-initialises the back buffer to all-ones.
            NuMemSet128(scene->clip_used[(scene->render_buffer >> 7) ^ 1], 0xff, blocks);
        }
    }

    for (u32 i = 0; i < scene->nmtls; ++i) {
        NUDISPLAYLIST *dl = scene->dlist_mtls[i];
        if (!gated) {
            dl->scene_buffer = 0;
        } else {
            u32 new_buf = (dl->scene_buffer == 0) ? 1 : 0;
            dl->scene_buffer = new_buf;
        }
        dl->mtl_last = dl->first;
        dl->scene_next = dl->scene_first[dl->scene_buffer];
        ResetRenderStateCache(dl->state);
    }
    scene->flags &= 0xf1;
}

template <typename T> static T *CloneSceneAllocate(VARIPTR *buffer, usize count, usize alignment = 4) {
    buffer->addr = (buffer->addr + alignment - 1) & ~(alignment - 1);
    T *result = static_cast<T *>(buffer->void_ptr);
    buffer->addr += sizeof(T) * count;
    return result;
}

extern "C" NUDLDLISTSCENE *NuDisplaySceneClone(NUDLDLISTSCENE *source, VARIPTR *buffer) {
    NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);
    NUDLDLISTSCENE *scene = CloneSceneAllocate<NUDLDLISTSCENE>(buffer, 1);
    *scene = *source;
    scene->items = CloneSceneAllocate<NUDISPLAYLISTITEM>(buffer, source->nitems);
    memcpy(scene->items, source->items, sizeof(NUDISPLAYLISTITEM) * source->nitems);
    i32 clip_bytes = ClipUsedBlockCount(source->nclip_objects) * 16;
    scene->clip_used[0] = CloneSceneAllocate<u8>(buffer, clip_bytes, 16);
    scene->clip_used[1] = CloneSceneAllocate<u8>(buffer, clip_bytes, 1);
    memset(scene->clip_used[0], 0, clip_bytes);
    memset(scene->clip_used[1], 0, clip_bytes);
    u32 mtl_bytes = (((source->nmtls + 7) >> 7) + 1) * 16;
    scene->mtl_used[0] = CloneSceneAllocate<u8>(buffer, mtl_bytes, 16);
    scene->mtl_used[1] = CloneSceneAllocate<u8>(buffer, mtl_bytes, 1);
    memset(scene->mtl_used[0], 0, mtl_bytes);
    memset(scene->mtl_used[1], 0, mtl_bytes);
    NUDISPLAYLIST *lists = CloneSceneAllocate<NUDISPLAYLIST>(buffer, source->nmtls);
    NUDISPLAYLIST *first_list = lists;
    scene->dlist_mtls = CloneSceneAllocate<NUDISPLAYLIST *>(buffer, source->nmtls);
    NURNDRSTATE *states = CloneSceneAllocate<NURNDRSTATE>(buffer, source->nmtls);
    memset(states, 0, sizeof(NURNDRSTATE) * source->nmtls);
    NUDISPLAYLISTITEM *heads = CloneSceneAllocate<NUDISPLAYLISTITEM>(buffer, source->nmtls);
    memset(heads, 0, sizeof(NUDISPLAYLISTITEM) * source->nmtls);
    for (i32 i = 0; i < static_cast<i32>(source->nmtls); ++i, ++lists, ++states, ++heads) {
        scene->dlist_mtls[i] = lists;
        memcpy(scene->dlist_mtls[i], source->dlist_mtls[i], sizeof(NUDISPLAYLIST));
        scene->dlist_mtls[i]->dlist = scene;
        scene->dlist_mtls[i]->state = states;
        scene->dlist_mtls[i]->mtl_item = scene->items + (source->dlist_mtls[i]->mtl_item - source->items);
        scene->dlist_mtls[i]->dyn_geom = scene->items + (source->dlist_mtls[i]->dyn_geom - source->items);
        scene->dlist_mtls[i]->first = heads;
        scene->dlist_mtls[i]->mtl_last = scene->dlist_mtls[i]->first;
        scene->dlist_mtls[i]->scene_first[0] =
            CloneSceneAllocate<NUDISPLAYLISTITEM>(buffer, source->dlist_mtls[i]->nscene_items, 16);
        scene->dlist_mtls[i]->scene_first[1] =
            CloneSceneAllocate<NUDISPLAYLISTITEM>(buffer, source->dlist_mtls[i]->nscene_items, 1);
        scene->dlist_mtls[i]->first->type = 0x8d;
        scene->dlist_mtls[i]->first->next = NULL;
        scene->dlist_mtls[i]->first->id = 1;
    }
    NUMTL *materials = CloneSceneAllocate<NUMTL>(buffer, source->nmtls);
    scene->mtls = CloneSceneAllocate<NUMTL *>(buffer, source->nmtls);
    for (i32 i = 0; i < static_cast<i32>(source->nmtls); ++i, ++materials) {
        scene->mtls[i] = materials;
        memcpy(scene->mtls[i], source->mtls[i], sizeof(NUMTL));
        scene->mtls[i]->display_list = reinterpret_cast<NUDISPLAYLIST *>(
            reinterpret_cast<u8 *>(first_list) +
            ((reinterpret_cast<usize>(source->mtls[i]->display_list) - reinterpret_cast<usize>(source->dlist_mtls[0])) &
             ~usize(3)));
        NuMtlUpdate(scene->mtls[i]);
    }
    scene->sort_pris = CloneSceneAllocate<NUSORTPRI>(buffer, source->nsort_pris);
    memcpy(scene->sort_pris, source->sort_pris, sizeof(NUSORTPRI) * source->nsort_pris);
    for (i32 i = 0; i < source->nsort_pris; ++i) {
        scene->sort_pris[i].display_scene = scene;
        scene->sort_pris[i].items = scene->items + (source->sort_pris[i].items - source->items);
        scene->sort_pris[i].dlist_next = &scene->sort_pris[i];
    }
    for (i32 i = 0; i < source->nitems; ++i) {
        NUDISPLAYLISTITEM *item = &scene->items[i];
        if (item->type == 0x80) {
            for (i32 j = 0; j < static_cast<i32>(source->nmtls); ++j) {
                if (source->items[i].next == source->mtls[j]) {
                    item->next = scene->mtls[j];
                    break;
                }
            }
        } else if (item->type == 0x85) {
            item->next = scene->items + (static_cast<NUDISPLAYLISTITEM *>(source->items[i].next) - source->items);
        }
    }
    if (source->material_animations != NULL) {
        scene->material_animations = CloneSceneAllocate<NUMTLANIMSET>(buffer, 1);
        *scene->material_animations = *source->material_animations;
        scene->material_animations->scene = scene;
        scene->material_animations->next = global_dlist_manager.mtlanim_list;
        global_dlist_manager.mtlanim_list = scene->material_animations;
    } else {
        scene->material_animations = NULL;
    }
    scene->local_state = CloneSceneAllocate<NUGLOBALRNDRSTATE>(buffer, 1);
    memset(scene->local_state, 0, sizeof(NUGLOBALRNDRSTATE));
    NuDisplaySceneClonePS(source, scene, buffer);
    global_dlist_manager.dlists[global_dlist_manager.ndisplay_lists++] = scene;
    ResetSceneBeforeFrame(scene, false);
    NUSORTPRI *sort_list = global_dlist_manager.sort_list;
    for (i32 i = 0; i < scene->nsort_pris; ++i) {
        NUSORTPRI *sort = &scene->sort_pris[i];
        sort->sort_pri &= 0x1ffff;
        if (numtl_renderplane != 0)
            sort->sort_pri += numtl_renderplane * 0x20000;
        NUSORTPRI *previous = NULL;
        NUSORTPRI *current = sort_list;
        while (current != NULL && current->sort_pri < sort->sort_pri) {
            previous = current;
            current = current->sys_next;
        }
        sort->sys_next = current;
        if (previous == NULL)
            sort_list = sort;
        else
            previous->sys_next = sort;
        ++global_dlist_manager.nused_sort_pris;
    }
    global_dlist_manager.sort_list = sort_list;
    scene->flags &= 0xef;
    scene->render_buffer |= 0x20;
    NuDisplaySceneAddPS(scene);
    NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
    return scene;
}

// NuDisplaySceneAdd @ 0x2e9e30
extern "C" void NuDisplaySceneAdd(NUDLDLISTSCENE *scene) {
    NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);

    global_dlist_manager.dlists[global_dlist_manager.ndisplay_lists++] = scene;
    ResetSceneBeforeFrame(scene, /*gated=*/false);

    if (scene->nsort_pris > 0) {
        NUSORTPRI *sort_list = global_dlist_manager.sort_list;
        i32 used_count = global_dlist_manager.nused_sort_pris;
        for (i32 i = 0; i < scene->nsort_pris; ++i) {
            NUSORTPRI *sort_pri = &scene->sort_pris[i];
            if (numtl_renderplane != 0) {
                sort_pri->sort_pri += numtl_renderplane * 0x20000;
            }

            NUSORTPRI *previous = nullptr;
            NUSORTPRI *current = sort_list;
            while (current != nullptr && current->sort_pri < sort_pri->sort_pri) {
                previous = current;
                current = current->sys_next;
            }
            sort_pri->sys_next = current;
            if (previous == nullptr) {
                sort_list = sort_pri;
            } else {
                previous->sys_next = sort_pri;
            }
            ++used_count;
        }
        global_dlist_manager.sort_list = sort_list;
        global_dlist_manager.nused_sort_pris = used_count;
    }

    if (scene->material_animations != nullptr) {
        scene->material_animations->next = global_dlist_manager.mtlanim_list;
        global_dlist_manager.mtlanim_list = scene->material_animations;
    }
    scene->flags &= 0xef;
    scene->render_buffer &= 0xdf;
    scene->alpha_values = nullptr;
    NuDisplaySceneAddPS(scene);

    NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
}

// NuDisplaySceneDestroy @ 0x2e9fd0
extern "C" void NuDisplaySceneDestroy(NUDLDLISTSCENE *scene) {
    if (scene == nullptr) {
        return;
    }

    NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);
    NuDisplaySceneDestroyPS(scene);

    i32 scene_index = 0;
    while (global_dlist_manager.dlists[scene_index] != scene) {
        ++scene_index;
    }

    NUSORTPRI *sort_list = global_dlist_manager.sort_list;
    for (i32 i = 0; i < scene->nsort_pris; ++i) {
        NUSORTPRI *sort_pri = &scene->sort_pris[i];
        if (sort_list == sort_pri) {
            sort_list = sort_pri->sys_next;
            continue;
        }

        NUSORTPRI *previous = sort_list;
        while (previous->sys_next != sort_pri) {
            previous = previous->sys_next;
        }
        previous->sys_next = sort_pri->sys_next;
    }
    global_dlist_manager.sort_list = sort_list;
    global_dlist_manager.nused_sort_pris -= scene->nsort_pris;

    if (scene->material_animations != nullptr) {
        NUMTLANIMSET *node = global_dlist_manager.mtlanim_list;
        if (node == scene->material_animations) {
            global_dlist_manager.mtlanim_list = scene->material_animations->next;
        } else {
            while (node->next != scene->material_animations) {
                node = node->next;
            }
            node->next = scene->material_animations->next;
        }
    }

    i32 removed = 0;
    for (i32 i = 0; i < global_dlist_manager.ndisplay_lists; ++i) {
        global_dlist_manager.dlists[i - removed] = global_dlist_manager.dlists[i];
        if (global_dlist_manager.dlists[i] == scene) {
            ++removed;
        }
    }
    if (removed != 0) {
        --global_dlist_manager.ndisplay_lists;
    }

    NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
}

extern "C" void NuDisplayListSwapBuffersBeginFrame(void) {
    NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);

    // Flip the dynamic-material scene's render buffer.
    u32 flip = (~global_dlist_manager.dyn_mtl_dlist.render_buffer) & 0x80;
    global_dlist_manager.dyn_mtl_dlist.render_buffer &= 0x7f;
    global_dlist_manager.dyn_mtl_dlist.render_buffer |= flip;
    ResetSceneBeforeFrame(&global_dlist_manager.dyn_mtl_dlist, /*gated=*/true);

    for (i32 i = 0; i < global_dlist_manager.ndisplay_lists; ++i) {
        nudisplayscene_s *sc = global_dlist_manager.dlists[i];
        u32 nv = (~static_cast<u32>(static_cast<u8>(sc->render_buffer))) & 0xffffff80;
        sc->render_buffer &= 0x7f;
        sc->render_buffer |= nv;
        ResetSceneBeforeFrame(sc, /*gated=*/true);
        if (sc->gscene) {
            *reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(sc->gscene) + 0x44) = 0;
        }
    }

    global_dlist_manager.nrender_scenes = 0;
    for (nusortpri_s *sp = global_dlist_manager.sort_list; sp; sp = sp->sys_next) {
        sp->flags &= 0xfd;
    }

    RndrStateResetSharedGlobalState();
    NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
}

// ──────────────────────────────────────────────────────────────────────────────
// Material clipping
// ──────────────────────────────────────────────────────────────────────────────

// NUMTL layout helpers — these fields are at fixed offsets in the original
// binary (verified against NuDisplayListSwapBuffersEndFrame visibility test).
#define MTL_BLEND_FLAG(mtl) (*(const u32 *)((const u8 *)(mtl) + 0xb0))
#define MTL_BLEND_OP2(mtl) (*(const u8 *)((const u8 *)(mtl) + 0xf8))
#define MTL_ATTRIB_DWORD1(mtl) (*(const u32 *)((const u8 *)(mtl) + 0x44))

static void UpdateMaterialClipBits(nudisplayscene_s *scene) {
    if (!scene || !scene->mtls || !scene->mtls[0]) {
        return;
    }
    u8 buf = static_cast<u8>(scene->render_buffer >> 7);
    const u8 *cur = scene->mtl_used[scene->render_buffer >> 7];
    const u8 *other = scene->mtl_used[buf ^ 1];
    u32 nbytes = (scene->nmtls + 7) >> 3;
    if (nbytes == 0)
        return;

    for (u32 byte_i = 0; byte_i < nbytes; ++byte_i) {
        u8 cur_byte = cur[byte_i];
        if (other[byte_i] == cur_byte)
            continue;
        for (u32 bit = 0; bit < 8; ++bit) {
            i32 idx = static_cast<i32>(byte_i * 8 + bit);
            if (idx >= static_cast<i32>(scene->nmtls))
                return;
            NUDISPLAYLIST *dl = scene->dlist_mtls[idx];
            NUMTL *mtl = scene->mtls[dl->mtl_id];
            bool enabled = (cur_byte >> bit) & 1;
            // Materials with blend-op 0xff are never drawn via this path.
            if (enabled && mtl && MTL_BLEND_FLAG(mtl) != 0 && MTL_BLEND_OP2(mtl) == 0xff) {
                enabled = false;
            }
            dl->mtl_item->id = enabled ? kItemId_Cnt : kItemId_Next;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Render-scene queueing
// ──────────────────────────────────────────────────────────────────────────────

extern "C" i32 NuDisplayListAddRenderScene(void) {
    NUDLIST_MANAGER *mgr = &global_dlist_manager;
    VARIPTR *buf = display_list_buffer;

    nusortpri_s **pris = reinterpret_cast<nusortpri_s **>((buf->addr + 0xfU) & ~0xfU);
    buf->addr = reinterpret_cast<usize>(pris + mgr->nused_sort_pris + 1);

    i32 count = 0;
    for (nusortpri_s *sp = mgr->sort_list; sp; sp = sp->sys_next) {
        nudisplayscene_s *sc = sp->display_scene;
        if (sc == nullptr) {
            // FX sortpri — carry over unless already captured this frame.
            if ((sp->flags & 2) == 0) {
                pris[count++] = sp;
                sp->flags |= 2;
            }
        } else if ((sc->flags & NUDL_SCENE_FLAG_NEEDS_BUILD) == 0) {
            if ((sp->flags & 2) == 0 && (sc->flags & 6) != 0) {
                pris[count++] = sp;
                sp->flags |= 2;
            }
        } else if ((sc->flags & 2) != 0) {
            // Rebuild this sortpri's material chains into the stream buffer.
            nusortpri_s *new_sp = reinterpret_cast<nusortpri_s *>((buf->addr + 3U) & ~0x3U);
            nudisplaylistitem_s *area = reinterpret_cast<nudisplaylistitem_s *>(new_sp + 1);
            buf->addr = reinterpret_cast<usize>(new_sp + 1);

            new_sp->display_scene = sc;
            new_sp->nmtls = sp->nmtls;
            new_sp->mtl_first = sp->mtl_first;
            new_sp->sort_pri = sp->sort_pri;
            new_sp->items = area;

            i32 slot = 0;
            for (i32 m = 0; m < sp->nmtls; ++m) {
                NUDISPLAYLIST *dl = sp->display_scene->dlist_mtls[sp->mtl_first + m];
                if (dl->mtl_last == dl->first)
                    continue;

                if (dl->mtl_item[1].type == kItemType_Mtl) {
                    new_sp->items[slot] = dl->mtl_item[1];
                } else {
                    new_sp->items[slot] = dl->mtl_item[3];
                }
                new_sp->items[slot + 1] = *dl->first;
                slot += 2;
                dl->mtl_last->next = &new_sp->items[slot];
                dl->mtl_last = dl->first;
                ResetRenderStateCache(dl->state);
            }
            new_sp->items[slot].id = kItemId_Ret;
            new_sp->items[slot].next = nullptr;
            new_sp->items[slot].type = kItemType_Terminator;
            buf->addr += static_cast<usize>(slot + 1) * kItemSize;

            pris[count++] = new_sp;
            new_sp->flags |= 2;
        }
    }

    if (count == 0 && mgr->dlist_2d.mtl_last == mgr->dlist_2d.first) {
        return -1;
    }

    auto *rs = reinterpret_cast<nudisplaylistrenderscene_s *>((buf->addr + 3U) & ~0x3U);
    mgr->front_render_scenes[mgr->nrender_scenes] = rs;
    buf->addr = reinterpret_cast<usize>(rs + 1);
    rs->nsort_pris = count;
    rs->sort_pris = pris;

    if (mgr->dlist_2d.mtl_last == mgr->dlist_2d.first) {
        rs->render_2d_first.type = kItemType_Terminator;
        rs->render_2d_first.id = kItemId_Ret;
        rs->render_2d_first.next = nullptr;
    } else {
        mgr->dlist_2d.mtl_last->type = kItemType_Terminator;
        mgr->dlist_2d.mtl_last->id = kItemId_Ret;
        mgr->dlist_2d.mtl_last->next = nullptr;
        rs->render_2d_first = *mgr->dlist_2d.first;
        NuDisplayListReset(&mgr->dlist_2d);
    }

    return mgr->nrender_scenes++;
}

extern "C" SAGA_HOST_WEAK void NuDisplayListDrawRenderScene(i32 render_scene_id) {
    NUDLIST_MANAGER *mgr = &global_dlist_manager;

    NuThreadCriticalSectionBegin(mgr->loading_critical_section);
    nudisplaylistrenderscene_s *rs = mgr->safe_render_scenes[render_scene_id];
    if (rs) {
        const i32 nsort_pris = rs->nsort_pris;
        for (i32 i = 0; i < nsort_pris; ++i) {
            nusortpri_s *sp = rs->sort_pris[i];
            NuDisplayListCaptureSortPriority(sp);
            NuDisplayListDrawItems(sp->items);
            // Callbacks can update the slot; the original reloads it before
            // drawing the next priority or the 2D tail.
            rs = mgr->safe_render_scenes[render_scene_id];
        }
        NuDisplayListDrawItems(&rs->render_2d_first);
        mgr->safe_render_scenes[render_scene_id] = nullptr;
    }
    NuThreadCriticalSectionEnd(mgr->loading_critical_section);
}

// ──────────────────────────────────────────────────────────────────────────────
// Dynamic material display lists
// ──────────────────────────────────────────────────────────────────────────────

nudisplaylistitem_s *NuDisplayListCreateMtlDlist(nudisplaylistitem_s * /*item*/, NUMTL *mtl, VARIPTR *buff,
                                                 VARIPTR /*buff_end*/) {
    auto *out = reinterpret_cast<nudisplaylistitem_s *>(buff->addr);
    buff->addr += 8 * kItemSize;

    out->type = 0x85;
    out->id = kItemId_Next;
    out->next = out + 7;

    NuDisplayListAddClut(out + 1, mtl->tex_id);
    NuDisplayListAddTexture(out + 2, mtl->tex_id);
    NuDisplayListAddMaterialState(out + 3, mtl);
    NuDisplayListAddMicrocode(out + 4, mtl);
    NuDisplayListAddLightState(out + 5, mtl);

    out[6].type = 0x8b;
    out[6].id = kItemId_Next;
    out[6].next = out + 7;
    out[7].type = kItemType_Terminator;
    out[7].id = kItemId_Ret;
    out[7].next = nullptr;

    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// Dynamic material link / unlink
// ──────────────────────────────────────────────────────────────────────────────

static i32 MtlSortKey(const NUMTL *mtl) {
    // Original: ((char)(u16)mtl[0] >> 4) * 0x20000 + sort_pri
    return static_cast<i32>(static_cast<i8>(((*reinterpret_cast<const u16 *>(mtl) >> 4) & 0xff))) * 0x20000 +
           mtl->sort_pri;
}

void DisplayListLinkDynamicMtls(void) {
    NUDLIST_MANAGER *mgr = &global_dlist_manager;
    if (mgr->nnew_materials == 0 && mgr->ndel_materials == 0)
        return;

    // ── Removal pass ──
    for (i32 di = 0; di < mgr->ndel_materials; ++di) {
        NUMTL *mtl = mgr->del_materials[di];
        if (!mtl->display_list || mgr->dyn_mtl_dlist.nmtls == 0)
            continue;

        // Locate slot.
        i32 idx = -1;
        if (mgr->dyn_mtl_dlist.mtls[0] == mtl) {
            idx = 0;
        } else {
            for (u32 j = 1; j < mgr->dyn_mtl_dlist.nmtls; ++j) {
                if (mgr->dyn_mtl_dlist.mtls[j] == mtl) {
                    idx = static_cast<i32>(j);
                    break;
                }
            }
            if (idx < 0)
                continue;
        }

        if (--mgr->material_used[idx] != 0)
            continue;

        // Find owning sortpri (last key match wins).
        i32 key = MtlSortKey(mtl);
        i32 sp_idx = -1;
        if (mgr->dyn_mtl_dlist.nsort_pris >= 1) {
            for (i32 j = 0; j < mgr->dyn_mtl_dlist.nsort_pris; ++j) {
                if (key == mgr->dyn_mtl_dlist.sort_pris[j].sort_pri)
                    sp_idx = j;
            }
        }
        nusortpri_s *sp = (sp_idx >= 0) ? &mgr->dyn_mtl_dlist.sort_pris[sp_idx] : nullptr;
        nudisplaylistitem_s *mtl_item = mtl->display_list->mtl_item;

        if (sp) {
            if (sp->items == mtl_item) {
                sp->items = static_cast<nudisplaylistitem_s *>(mtl_item[7].next);
                mtl_item[7].type = kItemType_Terminator;
                mtl_item[7].id = kItemId_Ret;
                mtl_item[7].next = nullptr;
            } else if (sp->nmtls != 0) {
                nudisplaylistitem_s *prev = mgr->dyn_mtl_dlist.mtls[sp->mtl_first]->display_list->mtl_item;
                bool found = (mtl_item == prev[7].next);
                for (u32 j = 0; !found && j + 1 < sp->nmtls; ++j) {
                    prev = mgr->dyn_mtl_dlist.mtls[static_cast<u32>(sp->mtl_first) + j + 1]->display_list->mtl_item;
                    found = (mtl_item == prev[7].next);
                }
                if (found) {
                    prev[7].next = mtl_item[7].next;
                    mtl_item[7].type = kItemType_Terminator;
                    mtl_item[7].id = kItemId_Ret;
                    mtl_item[7].next = nullptr;
                }
            }
        }

        // Free 0x80-byte stream buffer.
        for (u32 j = 0; j < mgr->dyn_mtl_dlist.nmtls; ++j) {
            if (mgr->mtl_buffers_used[j] != 0 &&
                mtl_item ==
                    reinterpret_cast<nudisplaylistitem_s *>(reinterpret_cast<u8 *>(mgr->mtlbuff.addr) + j * 0x80)) {
                mgr->mtl_buffers_used[j] = 0;
                break;
            }
        }

        if (sp) {
            if (--sp->nmtls == 0) {
                // Unlink empty sortpri.
                if (mgr->sort_list == sp) {
                    mgr->sort_list = sp->sys_next;
                } else {
                    for (nusortpri_s *cur = mgr->sort_list; cur; cur = cur->sys_next) {
                        if (cur->sys_next == sp) {
                            cur->sys_next = sp->sys_next;
                            break;
                        }
                    }
                }
                --mgr->nused_sort_pris;
                for (i32 j = sp_idx; j < mgr->dyn_mtl_dlist.nsort_pris - 1; ++j) {
                    mgr->dyn_mtl_dlist.sort_pris[j] = mgr->dyn_mtl_dlist.sort_pris[j + 1];
                }
                --mgr->dyn_mtl_dlist.nsort_pris;
            }

            // Compact material slots and recycle the removed list into the free tail.
            NUDISPLAYLIST *removed_dl = mgr->dyn_mtl_dlist.dlist_mtls[idx];
            for (u32 j = static_cast<u32>(idx); j + 1 < mgr->dyn_mtl_dlist.nmtls; ++j) {
                mgr->dyn_mtl_dlist.mtls[j] = mgr->dyn_mtl_dlist.mtls[j + 1];
                mgr->dyn_mtl_dlist.dlist_mtls[j] = mgr->dyn_mtl_dlist.dlist_mtls[j + 1];
                mgr->material_used[j] = mgr->material_used[j + 1];
            }
            mgr->dyn_mtl_dlist.dlist_mtls[mgr->dyn_mtl_dlist.nmtls - 1] = removed_dl;
            mgr->material_used[mgr->dyn_mtl_dlist.nmtls - 1] = 0;

            {
                for (u32 i = 0; i < mgr->dyn_mtl_dlist.nmtls; ++i) {
                    if (mgr->material_used[i] != 0) {
                        mgr->dyn_mtl_dlist.mtls[i]->display_list->mtl_id = static_cast<i32>(i);
                    }
                }
            }
            {
                u32 total = 0;
                for (i32 i = 0; i < mgr->dyn_mtl_dlist.nsort_pris; ++i) {
                    total += mgr->dyn_mtl_dlist.sort_pris[i].nmtls;
                }
                if (total != 0) {
                    for (i32 i = 0; i < mgr->dyn_mtl_dlist.nsort_pris; ++i) {
                        nusortpri_s *sp = &mgr->dyn_mtl_dlist.sort_pris[i];
                        if (sp->nmtls == 0)
                            continue;
                        for (u32 k = 0; k < total; ++k) {
                            if (MtlSortKey(mgr->dyn_mtl_dlist.mtls[k]) == sp->sort_pri) {
                                sp->mtl_first = static_cast<u16>(k);
                                break;
                            }
                        }
                    }
                }
            }
        }

        mtl->display_list = nullptr;
    }
    mgr->ndel_materials = 0;

    // ── Addition pass ──
    for (i32 ni = 0; ni < mgr->nnew_materials; ++ni) {
        NUMTL *mtl = mgr->new_materials[ni];
        if (mgr->dyn_mtl_dlist.nmtls <= 0)
            continue;

        // Already present? bump refcount.
        bool present = false;
        i32 idx = -1;
        if (mgr->dyn_mtl_dlist.mtls[0] == mtl) {
            present = true;
            idx = 0;
        } else {
            for (u32 j = 1; j < mgr->dyn_mtl_dlist.nmtls; ++j) {
                if (mgr->dyn_mtl_dlist.mtls[j] == mtl) {
                    present = true;
                    idx = static_cast<i32>(j);
                    break;
                }
            }
        }
        if (present) {
            ++mgr->material_used[idx];
            continue;
        }

        // Find free slot.
        i32 pos = -1;
        if (mgr->material_used[0] == 0)
            pos = 0;
        else {
            for (u32 j = 1; j < mgr->dyn_mtl_dlist.nmtls; ++j) {
                if (mgr->material_used[j] == 0) {
                    pos = static_cast<i32>(j);
                    break;
                }
            }
        }
        if (pos < 0)
            continue;

        // Find free stream buffer.
        i32 buf_off = -0x80;
        for (u32 j = 0; j < mgr->dyn_mtl_dlist.nmtls; ++j) {
            if (mgr->mtl_buffers_used[j] == 0) {
                mgr->mtl_buffers_used[j] = 1;
                buf_off = static_cast<i32>(j * 0x80);
                break;
            }
        }

        i32 key = MtlSortKey(mtl);
        i32 sp_idx = -1;
        if (mgr->dyn_mtl_dlist.nsort_pris > 0) {
            for (i32 j = 0; j < mgr->dyn_mtl_dlist.nsort_pris; ++j) {
                if (key == mgr->dyn_mtl_dlist.sort_pris[j].sort_pri)
                    sp_idx = j;
            }
        }

        // Insertion position — before first used material with key >= ours.
        i32 insert = pos;
        for (u32 j = 0; j < mgr->dyn_mtl_dlist.nmtls; ++j) {
            if (mgr->material_used[j] != 0 && key <= MtlSortKey(mgr->dyn_mtl_dlist.mtls[j])) {
                insert = static_cast<i32>(j);
                break;
            }
        }

        // Make room: shift [insert .. nmtls-2] right, tail goes to insert.
        NUDISPLAYLIST *old_tail = mgr->dyn_mtl_dlist.dlist_mtls[mgr->dyn_mtl_dlist.nmtls - 1];
        for (i32 j = static_cast<i32>(mgr->dyn_mtl_dlist.nmtls) - 2; j >= insert; --j) {
            mgr->dyn_mtl_dlist.mtls[j + 1] = mgr->dyn_mtl_dlist.mtls[j];
            mgr->dyn_mtl_dlist.dlist_mtls[j + 1] = mgr->dyn_mtl_dlist.dlist_mtls[j];
            mgr->material_used[j + 1] = mgr->material_used[j];
        }
        mgr->dyn_mtl_dlist.dlist_mtls[insert] = old_tail;
        mgr->material_used[insert] = 0;

        {
            for (u32 i = 0; i < mgr->dyn_mtl_dlist.nmtls; ++i) {
                if (mgr->material_used[i] != 0) {
                    mgr->dyn_mtl_dlist.mtls[i]->display_list->mtl_id = static_cast<i32>(i);
                }
            }
        }
        mgr->material_used[insert] = 1;

        NUDISPLAYLIST *dl = mgr->dyn_mtl_dlist.dlist_mtls[insert];
        mtl->display_list = dl;

        VARIPTR buf_ptr{reinterpret_cast<void *>(reinterpret_cast<u8 *>(mgr->mtlbuff.addr) + buf_off)};
        nudisplaylistitem_s *items = NuDisplayListCreateMtlDlist(dl->mtl_item, mtl, &buf_ptr, mgr->mtlbuffend);

        mgr->dyn_mtl_dlist.mtls[insert] = mtl;
        dl->dyn_geom = items + 6;
        dl->dlist = &mgr->dyn_mtl_dlist;
        dl->mtl_item = items;
        dl->mtl_id = insert;

        nusortpri_s *sp;
        if (sp_idx == -1) {
            sp = &mgr->dyn_mtl_dlist.sort_pris[mgr->dyn_mtl_dlist.nsort_pris];
            sp->items = items;
            sp->field_18 = mgr->field_4a8;
            sp->sort_pri = key;
            sp->nmtls = 0;

            if (!mgr->sort_list || mgr->sort_list->sort_pri >= key) {
                sp->sys_next = mgr->sort_list;
                mgr->sort_list = sp;
            } else {
                nusortpri_s *cur = mgr->sort_list;
                while (cur->sys_next && cur->sys_next->sort_pri < key)
                    cur = cur->sys_next;
                sp->sys_next = cur->sys_next;
                cur->sys_next = sp;
            }
            ++mgr->nused_sort_pris;
            ++mgr->dyn_mtl_dlist.nsort_pris;
            sp->nmtls = 1;
        } else {
            sp = &mgr->dyn_mtl_dlist.sort_pris[sp_idx];
            if (insert != 0 && MtlSortKey(mgr->dyn_mtl_dlist.mtls[insert - 1]) == key) {
                // Splice after previous material's block.
                auto *anchor = mgr->dyn_mtl_dlist.mtls[insert - 1]->display_list->mtl_item;
                auto *follow = static_cast<nudisplaylistitem_s *>(anchor[7].next);
                items[7].type = kItemType_Link;
                items[7].id = kItemId_Next;
                items[7].next = follow;
                anchor[7].next = items;
            } else {
                // Head of chain.
                items[7].type = kItemType_Link;
                items[7].id = kItemId_Next;
                items[7].next = sp->items;
                sp->items = items;
            }
            ++sp->nmtls;
        }

        {
            for (u32 i = 0; i < mgr->dyn_mtl_dlist.nmtls; ++i) {
                if (mgr->material_used[i] != 0) {
                    mgr->dyn_mtl_dlist.mtls[i]->display_list->mtl_id = static_cast<i32>(i);
                }
            }
        }
        {
            u32 total = 0;
            for (i32 i = 0; i < mgr->dyn_mtl_dlist.nsort_pris; ++i) {
                total += mgr->dyn_mtl_dlist.sort_pris[i].nmtls;
            }
            if (total != 0) {
                for (i32 i = 0; i < mgr->dyn_mtl_dlist.nsort_pris; ++i) {
                    nusortpri_s *sp = &mgr->dyn_mtl_dlist.sort_pris[i];
                    if (sp->nmtls == 0)
                        continue;
                    for (u32 k = 0; k < total; ++k) {
                        if (MtlSortKey(mgr->dyn_mtl_dlist.mtls[k]) == sp->sort_pri) {
                            sp->mtl_first = static_cast<u16>(k);
                            break;
                        }
                    }
                }
            }
        }
    }
    mgr->nnew_materials = 0;
}

// ──────────────────────────────────────────────────────────────────────────────
// End-of-frame
// ──────────────────────────────────────────────────────────────────────────────

extern "C" void NuDisplayListSwapBuffersEndFrame(void) {
    NUDLIST_MANAGER *mgr = &global_dlist_manager;

    NuThreadCriticalSectionBegin(mgr->loading_critical_section);
    DisplayListLinkDynamicMtls();
    DisplayListSwapBuffersPS();
    UpdateMaterialClipBits(&mgr->dyn_mtl_dlist);

    for (i32 i = 0; i < mgr->ndisplay_lists; ++i) {
        nudisplayscene_s *sc = mgr->dlists[i];
        u8 flags = sc->flags;

        if ((flags & 4) != 0) {
            // Clip-word diff: 2 bits per object, 16 objects per u32 word.
            u32 buf = static_cast<u32>(static_cast<u8>(sc->render_buffer >> 7));
            const u8 *cur_words = sc->clip_used[buf];
            const u8 *other_words = sc->clip_used[buf ^ 1];
            i32 nclip = sc->nclip_objects;
            i32 words = (nclip + 15) >> 4; // ceil(nclip/16)

            for (i32 w = 0; w < words; ++w) {
                const u8 *curw = cur_words + w * 4;
                if (*reinterpret_cast<const u32 *>(curw) == *reinterpret_cast<const u32 *>(other_words + w * 4)) {
                    continue;
                }
                for (i32 b = 0; b < 4; ++b) {
                    u8 byte_val = curw[b];
                    for (i32 shift = 0; shift < 8; shift += 2) {
                        i32 obj = w * 16 + b * 4 + shift / 2;
                        if (obj >= nclip)
                            break;
                        u32 code = (byte_val >> shift) & 3;
                        NUCLIPOBJECT *co = &sc->clip_objects[obj];
                        u16 cnt = sc->clip_counts[obj];
                        u8 new_id = (code == 0) ? kItemId_Cnt : kItemId_Call;
                        for (u16 j = 0; j < cnt; ++j) {
                            i32 idx = co->indices[j];
                            sc->items[idx].id = new_id;
                            sc->items[idx - 1].id = new_id;
                        }
                    }
                }
            }

            // Alpha fade — patch alpha into every 0x82 geom item.
            if (sc->alpha_values && nclip > 0) {
                for (i32 o = 0; o < nclip; ++o) {
                    u16 cnt = sc->clip_counts[o];
                    i32 *indices = sc->clip_objects[o].indices;
                    for (u16 j = 0; j < cnt; ++j) {
                        auto *item = &sc->items[indices[j]];
                        if (item->id != kItemId_Cnt && item->type == 0x82) {
                            DisplayListSetAlphaPS(item - 1, item, sc->alpha_values[o]);
                        }
                    }
                }
            }
            flags = sc->flags;
        }
        if ((flags & 6) != 0) {
            UpdateMaterialClipBits(sc);
        }
    }

    // Publish front → safe render scenes.
    for (i32 s = 0; s < 24; ++s) {
        mgr->safe_render_scenes[s] = mgr->front_render_scenes[s];
        mgr->front_render_scenes[s] = nullptr;
    }

    // Per-sortpri render-state walk.
    nurndrstate_s tmp_state{};
    tmp_state.mtl = nullptr;
    tmp_state.tex_id = -1;
    tmp_state.konst_id = -1;
    tmp_state.global_id = -1;
    tmp_state.lights_id = -1;
    tmp_state.camera_id = -1;
    tmp_state.fog_id = -1;
    tmp_state.reflection_id = -1;
    nuglobalrndrstate_s *last_local = nullptr;

    auto linkSceneGeometry = [](NUDISPLAYLIST *dl) {
        if (dl->scene_first[dl->scene_buffer] == dl->scene_next) {
            dl->dyn_geom->next = dl->dyn_geom + 1;
        } else {
            dl->dyn_geom->next = dl->scene_first[dl->scene_buffer];
            dl->scene_next->type = kItemType_Link;
            dl->scene_next->id = kItemId_Next;
            dl->scene_next->next = dl->dyn_geom + 1;
        }
    };

    auto copyMaterialGeometry = [&](NUDISPLAYLIST *dl, nudisplaylistitem_s *first, nudisplayscene_s *sc) {
        *dl->dyn_geom = *first;
        if ((sc->flags & 4) == 0) {
            dl->mtl_last->next = dl->mtl_item->next;
        } else if (dl->scene_first[dl->scene_buffer] != dl->scene_next) {
            dl->mtl_last->next = dl->scene_first[dl->scene_buffer];
            dl->scene_next->type = kItemType_Link;
            dl->scene_next->id = kItemId_Next;
            dl->scene_next->next = dl->dyn_geom + 1;
        }
        dl->mtl_last = first;
    };

    for (nusortpri_s *sp = mgr->sort_list; sp; sp = sp->sys_next) {
        nudisplayscene_s *sc = sp->display_scene;
        if (!sc) {
            RndrStateUpdateFx(&tmp_state, sp->items);
            continue;
        }
        if ((sc->flags & 6) == 0 || sp->nmtls == 0)
            continue;

        for (u32 m = 0; m < sp->nmtls; ++m) {
            i32 mi = static_cast<i32>(static_cast<u32>(sp->mtl_first) + m);
            NUDISPLAYLIST *dl = sc->dlist_mtls[mi];
            if (dl->mtl_item->id != kItemId_Cnt)
                continue;

            NUMTL *mtl = sc->mtls[mi];
            nuglobalrndrstate_s *ls = sc->local_state;

            if (!ls || (sc->flags & 4) == 0) {
                // No local state or no clipping — copy geometry if needed.
                if (dl->mtl_last != dl->first) {
                    copyMaterialGeometry(dl, dl->first, sc);
                } else {
                    linkSceneGeometry(dl);
                }
                last_local = nullptr;
            } else {
                bool same_as_last = (dl->mtl_last == dl->first && ls == last_local);
                if (!same_as_last) {
                    DisplayListUpdateRenderState(dl, ls);
                    last_local = sc->local_state;
                    if (dl->mtl_last != dl->first) {
                        copyMaterialGeometry(dl, dl->first, sc);
                    } else {
                        linkSceneGeometry(dl);
                    }
                } else {
                    linkSceneGeometry(dl);
                }
            }

            RndrStateUpdate(&tmp_state, mtl, dl->mtl_item);

            // Visibility cull — hide materials that fail the scene's flag test.
            u16 scene_flags = *reinterpret_cast<const u16 *>(&sc->flags);
            if ((scene_flags & 0x1fe0) != 0 &&
                ((static_cast<u8>(scene_flags >> 5) & static_cast<u8>(MTL_ATTRIB_DWORD1(mtl) >> 0xe)) != 0)) {
                dl->mtl_item->id = kItemId_Next;
                i32 mid = dl->mtl_id;
                i32 word = (mid >= 0) ? mid >> 3 : (mid + 7) >> 3;
                i32 bit = mid & 7;
                u8 *mu = sc->mtl_used[static_cast<u32>(static_cast<u8>(sc->render_buffer >> 7))];
                mu[word] &= static_cast<u8>((static_cast<u32>(-2) << bit) | (0xfffffffeu >> (32 - bit)));
            }
        }
    }

    for (i32 i = 0; i < mgr->ndisplay_lists; ++i) {
        nudisplayscene_s *sc = mgr->dlists[i];
        if (sc->local_state) {
            RndrStateResetGlobalState(sc->local_state);
        }
    }

    NuThreadCriticalSectionEnd(mgr->loading_critical_section);
}

extern "C" void NuDisplayListDraw2D(void) {
}

void NuDisplayListEndScene(void) {
    NuDisplayListBeginCriticalSection();
    for (i32 i = 0; i < global_dlist_manager.ndisplay_lists; ++i) {
        NUDLDLISTSCENE *scene = global_dlist_manager.dlists[i];
        if (scene->flags & (NUDL_SCENE_FLAG_CLIP_MATERIALS | NUDL_SCENE_FLAG_CLIPPING))
            scene->flags |= NUDL_SCENE_FLAG_END_SCENE;
    }
    if (global_dlist_manager.dyn_mtl_dlist.flags & NUDL_SCENE_FLAG_CLIP_MATERIALS)
        global_dlist_manager.dyn_mtl_dlist.flags |= NUDL_SCENE_FLAG_END_SCENE;
    NuDisplayListEndCriticalSection();
}

extern "C" void *DisplayListCreateFaceonTransformPS(VARIPTR *, NUMTX *, NUMTL *, void *);
extern "C" void *DisplayListCreateGeomTransformPS(VARIPTR *, NUMTX *, NUMTL *, void *, void *);

extern "C" void NuDisplayListBurstRndrSpecial(nuhspecial_s *handle, u32 count, NUMTX *matrices, i32 clip) {
    u16 visible[1024];
    if (handle == NULL)
        return;
    NUDISPLAYSPECIAL *special = handle->display_special;
    u32 visible_count;
    if (clip != 0) {
        visible_count = 0;
        for (u32 i = 0; i != count; ++i) {
            i32 result = NuCameraClipTestExtents(reinterpret_cast<NUVEC *>(&special->bounds_min),
                                                 reinterpret_cast<NUVEC *>(&special->bounds_max), &matrices[i], 0, 0);
            if (static_cast<u8>(result) != 0)
                visible[visible_count++] = static_cast<u16>(i | (result << 8));
        }
    } else {
        visible_count = count;
    }
    if (visible_count == 0)
        return;
    NUCLIPOBJECT *object = special->clip_objects;
    f32 *range = special->clip_range;
    while (*range > 0 && object->nmaterials == 0) {
        ++range;
        ++object;
    }
    NUDLDLISTSCENE *scene = handle->scene->display_list;
    scene->flags |= 2;
    for (u32 material = 0; material < static_cast<u32>(object->nmaterials); ++material) {
        NUDISPLAYLIST *list = scene->mtls[object->material_ids[material]]->display_list;
        scene->mtl_used[scene->render_buffer >> 7][list->mtl_id / 8] |= 1 << (list->mtl_id % 8);
        DisplayListUpdateRenderState(list, &render_state);
        VARIPTR *buffer = NuDisplayListLinkItems(list, visible_count * 2);
        NUDISPLAYLISTITEM *geometry = &scene->items[object->indices[material]];
        if (geometry->type == 0x8f) {
            for (u32 i = 0; i != visible_count; ++i) {
                NUMTX *matrix = &matrices[clip != 0 ? static_cast<u8>(visible[i]) : i];
                void *transform = DisplayListCreateFaceonTransformPS(buffer, matrix, scene->mtls[list->mtl_id],
                                                                     scene->items[object->indices[material]].next);
                void *faceon =
                    NuDisplayListPrepareFaceonPS(buffer, scene->items[object->indices[material]].next, matrix);
                list->items->type = 0x90;
                list->items->id = 3;
                list->items->next = transform;
                NUDISPLAYLISTITEM *first = list->items++;
                list->items->type = 0x8f;
                list->items->id = 3;
                list->items->next = faceon;
                NUDISPLAYLISTITEM *last = list->items++;
                DisplayListSetAlphaPS(first, last, 1);
            }
        } else {
            void *geometry_data = geometry->next;
            if (clip != 0) {
                for (u32 i = 0; i != visible_count; ++i) {
                    NUMTX *matrix = &matrices[static_cast<u8>(visible[i])];
                    void *transform = DisplayListCreateGeomTransformPS(buffer, matrix, scene->mtls[list->mtl_id],
                                                                       geometry_data, NULL);
                    list->items->type = 0x8c;
                    list->items->id = 3;
                    list->items->next = transform;
                    NUDISPLAYLISTITEM *first = list->items++;
                    list->items->type = 0x82;
                    list->items->id = 3;
                    list->items->next = geometry_data;
                    NUDISPLAYLISTITEM *last = list->items++;
                    DisplayListSetAlphaPS(first, last, 1);
                }
            } else {
                for (u32 i = 0; i != visible_count; ++i) {
                    NUMTX *matrix = &matrices[i];
                    void *transform = DisplayListCreateGeomTransformPS(buffer, matrix, scene->mtls[list->mtl_id],
                                                                       geometry_data, NULL);
                    list->items->type = 0x8c;
                    list->items->id = 3;
                    list->items->next = transform;
                    NUDISPLAYLISTITEM *first = list->items++;
                    list->items->type = 0x82;
                    list->items->id = 3;
                    list->items->next = geometry_data;
                    NUDISPLAYLISTITEM *last = list->items++;
                    DisplayListSetAlphaPS(first, last, 1);
                }
            }
        }
    }
}
