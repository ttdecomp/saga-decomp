#pragma once

#include "nu2api/nucore/common.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "decomp.h"

// Forward declarations shared with sibling headers.
struct numtl_s;
typedef struct numtl_s NUMTL;
struct nugscn_s;
struct nuhspecial_s;
struct nudisplayscene_s;

typedef struct numtlanimset_s {
    struct nudisplayscene_s *scene;
    i32 material_count;
    i32 *material_indices;
    struct numtlanimset_s *next;
} NUMTLANIMSET;

DECOMP_ASSERT(sizeof(NUMTLANIMSET) == 0x10, "material animation set size");

// ---------------------------------------------------------------------------
// Display-list item (original type `nudisplaylistitem_s`, 16 bytes).
//
// id classes (see NuDisplayListExecute @0x2f1230):
//   0        CNT   - execute nothing, continue with the next item
//   1        NEXT  - jump to item->next
//   3        CALL  - handler[item->type](item->next)
//   >=4      RETURN - terminate execution
// ---------------------------------------------------------------------------
typedef struct nudisplaylistitem_s {
    u8 type;
    u8 id;
    i16 size;
    void *next;
    u32 p[2];
} NUDISPLAYLISTITEM;

// Geometry packet referenced by the 0x82/0x98 display-list items.  Smooth
// skins reuse the ordinary geometry packet, optionally replacing
// dynamic_vertex_data with a deformed copy before the packet is submitted.
typedef struct nudisplaylistgeom_s {
    i32 primitive_type;              // 0x00
    i32 index_count;                 // 0x04
    u16 vertex_stride;               // 0x08
    u8 joint_indices[8];             // 0x0a, 0xff-terminated skin palette
    u16 reserved_12;                 // 0x12
    i32 base_vertex;                 // 0x14
    i32 vertex_count;                // 0x18
    i32 first_index;                 // 0x1c
    u32 index_buffer;                // 0x20
    usize vertex_buffer;             // GL buffer handle or immediate-data pointer
    i32 immediate;                   // 0x28
    NUVEC **deformer_vertex_offsets; // 0x2c, one xyz delta array per shape
    usize vertex_format;             // vertex declaration pointer
    void *dynamic_vertex_data;       // 0x34
} NUDISPLAYLISTGEOM;

// The first word is retained by the renderer's allocation API; weights used
// by DisplayListCreateSkinTransformPS start at +0x4.
typedef struct deformerweightsarray_s {
    i32 count;
    f32 weights[1];
} DEFORMERWEIGHTSARRAY;

DECOMP_ASSERT(sizeof(NUDISPLAYLISTGEOM) == 0x38, "display-list geometry packet size");
DECOMP_ASSERT(offsetof(NUDISPLAYLISTGEOM, joint_indices) == 0x0a, "geometry skin palette offset");
DECOMP_ASSERT(offsetof(NUDISPLAYLISTGEOM, dynamic_vertex_data) == 0x34, "geometry dynamic vertex data offset");

// ---------------------------------------------------------------------------
// Per-material display list state (original type `nudisplaylist_s`, 68 bytes;
// offsets verified against NuDisplayListBeforeFrame @0x2a9ea0 stores).
//
// NOTE: member `items` at 0x24 keeps the name already used across this tree
// (it is the next-free-item cursor consumed by NuDisplayListAddItem).
// ---------------------------------------------------------------------------
typedef struct nudisplaylist_s {
    struct nudisplayscene_s *dlist;      // 0x00 owning display-list scene
    i32 mtl_id;                          // 0x04 index into owning scene->mtls[]
    struct nurndrstate_s *state;         // 0x08 per-material render-state cache
    nudisplaylistitem_s *mtl_item;       // 0x0c
    nudisplaylistitem_s *first;          // 0x10
    nudisplaylistitem_s *mtl_last;       // 0x14
    nudisplaylistitem_s *dyn_geom;       // 0x18
    u32 texture_slots;                   // 0x1c
    u32 bitfield;                        // 0x20
    struct nudisplaylistitem_s *items;   // 0x24 add-item cursor (see note above)
    nudisplaylistitem_s *scene_first[2]; // 0x28
    nudisplaylistitem_s *scene_next;     // 0x30
    u32 scene_buffer;                    // 0x34
    i32 nscene_items;                    // 0x38
    u32 expansion_3c;                    // 0x3c
    u32 expansion_40;                    // 0x40
} NUDISPLAYLIST;

// original 0x2ec550 (_Z20DisplayListPrintItemP19nudisplaylistitem_siiPii --
// C++ linkage in the original binary); body stubbed in supportall.cpp.
void DisplayListLinkDynamicMtls(void);

void DisplayListPrintItem(nudisplaylistitem_s *item, i32 index, i32 depth, i32 *, i32 file_handle);
void DisplayListCreateDynMtlList(VARIPTR *buffer, VARIPTR buffer_end);

#ifdef __cplusplus
extern "C" {
#endif

    // Sort-priority record driving render-scene capture (36 bytes; layout given by
    // nusortpri_s in the original DB, display_scene offset confirmed by the
    // NuDisplayListCreate store at 0x2e8a40).
    typedef struct nusortpri_s {
        i32 sort_pri;                           // 0x00 sort key
        nudisplaylistitem_s *items;             // 0x04 first item executed for this pri
        struct nusortpri_s *sys_next;           // 0x08 global list link (manager sort_list)
        u32 reserved_0c;                        // 0x0c
        struct nusortpri_s *dlist_next;         // 0x10
        u32 flags;                              // 0x14 bit1: already captured into current frame
        u32 field_18;                           // 0x18 initialised from manager field_4a8
        struct nudisplayscene_s *display_scene; // 0x1c owning display-list scene (NULL == fx)
        u16 nmtls;                              // 0x20 number of materials covered
        u16 mtl_first;                          // 0x22 first material index in scene->mtls[]
    } NUSORTPRI;

    // Render-scene record queued between AddRenderScene and DrawRenderScene
    // (24 bytes; see NuDisplayListAddRenderScene @0x2edf20).
    typedef struct nudisplaylistrenderscene_s {
        i32 nsort_pris;                      // 0x00
        nusortpri_s **sort_pris;             // 0x04
        nudisplaylistitem_s render_2d_first; // 0x08 embedded 2D chain head
    } NUDISPLAYLISTRENDERSCENE;

    // Per-display-list clip object entry (12-byte stride; indices pointer at +0x8,
    // read by NuDisplayListSwapBuffersEndFrame @0x2eaef0).
    typedef struct nuclipobject_s {
        i32 nmaterials;    // 0x00 number of material ids touched by this object
        i32 *material_ids; // 0x04 material ids whose used bits must be set
        i32 *indices;      // 0x08 display-list item indices affected by this object
    } NUCLIPOBJECT;

    // Per-object axis-aligned clipping bounds, stored as center + extent.  The
    // w components are retained because this is the exact 0x20-byte record
    // stored in the scene, although the camera clip test consumes only xyz.
    typedef struct nuclipbounds_s {
        NUVEC center; // 0x00
        f32 center_w; // 0x0c
        NUVEC extent; // 0x10
        f32 extent_w; // 0x1c
    } NUCLIPBOUNDS;

    // ---------------------------------------------------------------------------
    // Display-list scene record.
    //
    // The original type is `nudisplayscene_s` (Ghidra DB: 144 bytes).
    // Every field offset matches the original exactly (verified against
    // NuDisplayListCreate @0x2e87d0, NuDisplayListSwapBuffersEndFrame @0x2eaef0
    // and NuDisplayListBeforeFrame @0x2a9ea0/@0x2a9ff0).
    // ---------------------------------------------------------------------------
    typedef struct nudisplayscene_s {
        char *name;                 // 0x00 debug name (CaptureSortPriority)
        i32 nitems;                 // 0x04
        nudisplaylistitem_s *items; // 0x08
        f32 *fade_ranges;           // 0x0c per-instance near/far fade pairs
        i32 nclip_objects;          // 0x10
        NUCLIPOBJECT *clip_objects; // 0x14
        u16 *clip_counts;           // 0x18 per-clip-object item counts
        void *field_1c;             // 0x1c
        void *field_20;             // 0x20
        void *field_24;             // 0x24
        f32 *lod_ranges;            // 0x28 zero-terminated LOD ranges per instance
        f32 *far_clip_ranges;       // 0x2c per-instance camera far-clip override
        u8 *clip_used[2];           // 0x30 double-buffered clip word bitmaps
        u8 pad_38[0x0c];            // 0x38..0x43 unnamed in original DB
        NUCLIPBOUNDS *clip_bounds;  // 0x44 per-instance center/extent bounds
        char *visibility_flags;     // 0x48
        u32 nmtls;                  // 0x4c
        NUMTL **mtls;               // 0x50
        NUDISPLAYLIST **dlist_mtls; // 0x54
        u8 *mtl_used[2];            // 0x58 double-buffered per-material used bits
        i32 nsort_pris;             // 0x60
        nusortpri_s *sort_pris;     // 0x64
        f32 *alpha_values;          // 0x68 per-clip-object alpha (SetAlphaPS)
        i32 nspecials;              // 0x6c
        void *specials;             // 0x70
        union {
            struct {
                u8 flags;         // 0x74 update-request bits (see enum below)
                u8 render_buffer; // 0x75 bit7 selects the current clip/mtl buffer
            };
            u16 flags_word;
            struct {
                u16 update_flags : 5;
                u16 material_layer_mask : 8;
                u16 buffer_flags : 3;
            };
        };
        u8 instance_visibility_enabled; // 0x76 bit0: per-instance visibility buffer is active
        u8 pad_77;
        struct nugscn_s *gscene;                 // 0x78
        void *ps;                                // 0x7c platform scratch
        struct nuglobalrndrstate_s *local_state; // 0x80
        u8 pad_84[4];                            // 0x84..0x87 unnamed in original DB
        u8 *portal_visibility_overrides;         // 0x88 instances excluded from portal clipping
        NUMTLANIMSET *material_animations;       // 0x8c
    } NUDLDLISTSCENE;

    // Flag bits inside the two bytes at 0x74/0x75 of the original scene record
    // (the `flags`/`render_buffer` pair; 0x74 is also read as one u16 by
    // SwapBuffersEndFrame's visibility test).
    enum {
        NUDL_SCENE_FLAG_CLIP_MATERIALS = 0x02, // 0x74 bit1: material update requested
        NUDL_SCENE_FLAG_CLIPPING = 0x04,       // 0x74 bit2: clip objects present
        NUDL_SCENE_FLAG_END_SCENE = 0x08,      // 0x74 bit3: update marked at EndScene
        NUDL_SCENE_FLAG_NEEDS_BUILD = 0x10,    // 0x74 bit4: rebuild dynamic items (AddRenderScene)
        NUDL_SCENE_RENDER_FLAG_CENTER_EXTENT_BOUNDS = 0x40,
        NUDL_SCENE_INSTANCE_VISIBILITY_ENABLED = 0x01,
        NUDL_INSTANCE_FLAG_VISIBLE = 0x01,
        NUDL_INSTANCE_FLAG_NO_VISIBILITY_TEST = 0x08,
        NUDL_INSTANCE_FLAG_CASTS_SHADOW = 0x20,
        NUDL_INSTANCE_FLAG_DISTANCE_FADE = 0x40,
    };

#ifdef __cplusplus
#if UINTPTR_MAX == UINT32_MAX
    static_assert(sizeof(NUDLDLISTSCENE) == 0x90, "dlist scene size");
    static_assert(offsetof(NUDLDLISTSCENE, clip_used) == 0x30, "scene.clip_used");
    static_assert(offsetof(NUDLDLISTSCENE, clip_counts) == 0x18, "scene.clip_counts");
    static_assert(offsetof(NUDLDLISTSCENE, clip_bounds) == 0x44, "scene.clip_bounds");
    static_assert(offsetof(NUDLDLISTSCENE, visibility_flags) == 0x48, "scene.visibility_flags");
    static_assert(offsetof(NUDLDLISTSCENE, nmtls) == 0x4c, "scene.nmtls");
    static_assert(offsetof(NUDLDLISTSCENE, mtls) == 0x50, "scene.mtls");
    static_assert(offsetof(NUDLDLISTSCENE, dlist_mtls) == 0x54, "scene.dlist_mtls");
    static_assert(offsetof(NUDLDLISTSCENE, mtl_used) == 0x58, "scene.mtl_used");
    static_assert(offsetof(NUDLDLISTSCENE, nsort_pris) == 0x60, "scene.nsort_pris");
    static_assert(offsetof(NUDLDLISTSCENE, sort_pris) == 0x64, "scene.sort_pris");
    static_assert(offsetof(NUDLDLISTSCENE, alpha_values) == 0x68, "scene.alpha_values");
    static_assert(offsetof(NUDLDLISTSCENE, flags) == 0x74, "scene.flags");
    static_assert(offsetof(NUDLDLISTSCENE, flags_word) == 0x74, "scene.flags_word");
    static_assert(offsetof(NUDLDLISTSCENE, render_buffer) == 0x75, "scene.render_buffer");
    static_assert(offsetof(NUDLDLISTSCENE, gscene) == 0x78, "scene.gscene");
    static_assert(offsetof(NUDLDLISTSCENE, local_state) == 0x80, "scene.local_state");
    static_assert(offsetof(NUDLDLISTSCENE, portal_visibility_overrides) == 0x88, "scene.portal_visibility_overrides");
    static_assert(offsetof(NUDLDLISTSCENE, material_animations) == 0x8c, "scene.material_animations");
#endif
#endif
    // The byte pair at 0x74 is also read as one u16 (flags | buffer<<8) by
    // SwapBuffersEndFrame's visibility test (& 0x1fe0).

    // ---------------------------------------------------------------------------
    // Bulk display-list manager (original bss object @0x11a0080, 0x604 bytes).
    // Replaces the former opaque byte array; offsets are asserted below.
    //
    // front_render_scenes/safe_render_scenes: NuDisplayListAddRenderScene writes
    // front[nrender_scenes] (@0x51C base); NuDisplayListDrawRenderScene(id) reads
    // safe[id] (@0x57C base = @0x51C + 24 entries, confirmed by the
    // `render_scenes[param_1 + 0x18]` access in the original); SwapBuffersEndFrame
    // moves every front slot to the same-indexed safe slot and clears front.
    // ---------------------------------------------------------------------------
    typedef struct nudlist_manager_s {
        i32 ndisplay_lists;                                  // 0x000
        NUDLDLISTSCENE *dlists[256];                         // 0x004
        NUDLDLISTSCENE dyn_mtl_dlist;                        // 0x404 dynamic-material scene
        i32 nnew_materials;                                  // 0x494
        NUMTL **new_materials;                               // 0x498
        i32 ndel_materials;                                  // 0x49c
        NUMTL **del_materials;                               // 0x4a0
        u8 *material_used;                                   // 0x4a4 refcounts for dyn_mtl_dlist slots
        u32 field_4a8;                                       // 0x4a8 copied into nusortpri_s.field_18
        VARIPTR mtlbuff;                                     // 0x4ac
        VARIPTR mtlbuffend;                                  // 0x4b0
        u8 *mtl_buffers_used;                                // 0x4b4
        NUDISPLAYLIST dlist_2d;                              // 0x4b8 static 2D display list
        NUDISPLAYLISTITEM dlist_2d_first;                    // 0x4fc stream area head of the 2D list
        u8 reserved_50c[4];                                  // 0x50c
        i32 nused_sort_pris;                                 // 0x510 live sortpri count (reserves ptr space)
        nusortpri_s *sort_list;                              // 0x514 global sortpri chain
        i32 nrender_scenes;                                  // 0x518
        nudisplaylistrenderscene_s *front_render_scenes[24]; // 0x51c written by AddRenderScene
        nudisplaylistrenderscene_s *safe_render_scenes[24];  // 0x57c read by DrawRenderScene(id)
        i32 max_fx;                                          // 0x5dc FX slot capacity
        u8 *fx_used;                                         // 0x5e0 handle allocation flags
        NUSORTPRI *fx_sort_pris;                             // 0x5e4 per-FX sort records
        void *fx_items;                                      // 0x5e8
        i32 loading_critical_section;                        // 0x5ec
        NUMTLANIMSET *mtlanim_list;                          // 0x5f0
        u8 tail_pad[0x604 - 0x5f4];                          // slack up to the original size
    } NUDLIST_MANAGER;

#ifdef __cplusplus
#if UINTPTR_MAX == UINT32_MAX
    static_assert(sizeof(NUDLIST_MANAGER) == 0x604, "manager size");
    static_assert(offsetof(NUDLIST_MANAGER, dlists) == 0x004, "mgr.dlists");
    static_assert(offsetof(NUDLIST_MANAGER, dyn_mtl_dlist) == 0x404, "mgr.dyn_mtl_dlist");
    static_assert(offsetof(NUDLIST_MANAGER, nnew_materials) == 0x494, "mgr.nnew_materials");
    static_assert(offsetof(NUDLIST_MANAGER, ndel_materials) == 0x49c, "mgr.ndel_materials");
    static_assert(offsetof(NUDLIST_MANAGER, material_used) == 0x4a4, "mgr.material_used");
    static_assert(offsetof(NUDLIST_MANAGER, mtlbuff) == 0x4ac, "mgr.mtlbuff");
    static_assert(offsetof(NUDLIST_MANAGER, mtl_buffers_used) == 0x4b4, "mgr.mtl_buffers_used");
    static_assert(offsetof(NUDLIST_MANAGER, dlist_2d) == 0x4b8, "mgr.dlist_2d");
    static_assert(offsetof(NUDLIST_MANAGER, dlist_2d_first) == 0x4fc, "mgr.dlist_2d_first");
    static_assert(offsetof(NUDLIST_MANAGER, nused_sort_pris) == 0x510, "mgr.nused_sort_pris");
    static_assert(offsetof(NUDLIST_MANAGER, sort_list) == 0x514, "mgr.sort_list");
    static_assert(offsetof(NUDLIST_MANAGER, nrender_scenes) == 0x518, "mgr.nrender_scenes");
    static_assert(offsetof(NUDLIST_MANAGER, front_render_scenes) == 0x51c, "mgr.front_render_scenes");
    static_assert(offsetof(NUDLIST_MANAGER, safe_render_scenes) == 0x57c, "mgr.safe_render_scenes");
    static_assert(offsetof(NUDLIST_MANAGER, fx_items) == 0x5e8, "mgr.fx_items");
    static_assert(offsetof(NUDLIST_MANAGER, loading_critical_section) == 0x5ec, "mgr.loading_critical_section");
#endif
#if UINTPTR_MAX == UINT32_MAX
    static_assert(sizeof(NUDISPLAYLISTITEM) == 0x10, "item size");
    static_assert(sizeof(NUDISPLAYLIST) == 0x44, "displaylist size");
    static_assert(sizeof(NUSORTPRI) == 0x24, "sortpri size");
    static_assert(sizeof(NUDISPLAYLISTRENDERSCENE) == 0x18, "renderscene size");
#endif
#endif

    extern "C" NUDLIST_MANAGER global_dlist_manager;

    extern VARIPTR *display_list_buffer;
    void NuDisplayListInit(VARIPTR *buffer, VARIPTR buffer_end);

    // Item-handler function signature (the original tables are private to
    // nudlist_android.c and indexed by item type - 0x80).
    typedef void (*nudl_handler_fn)(void *data);

    // Transcribed functions (original addresses in nudlist.cpp comments).
    void NuDisplayListExecute(nudisplaylistitem_s *item, const nudl_handler_fn *item_table);
    void NuDisplayListDrawItems(nudisplaylistitem_s *items);
    void NuDisplayListDrawRenderScene(i32 render_scene_id);
    i32 NuDisplayListAddRenderScene(void);
    void NuDisplayListSwapBuffersBeginFrame(void);
    void NuDisplayListSwapBuffersEndFrame(void);
    void NuDisplayListUpdateSpecial(struct nuhspecial_s *special);
    void DisplayListUpdateSpecialTransformPS(struct nuhspecial_s *special, NUMTX *matrix);
    void NuDisplayListReset(nudisplaylist_s *dl);
    void NuDisplayListCaptureSortPriority(nusortpri_s *sort_pri);
    void NuDisplayListSetItemTable(i32 which);
    void NuDisplaySceneRndr(void *display_scene);
    void NuDisplaySceneAdd(NUDLDLISTSCENE *scene);
    void NuDisplaySceneAddPS(NUDLDLISTSCENE *scene);
    void NuDisplaySceneDestroy(NUDLDLISTSCENE *scene);
    void NuDisplaySceneDestroyPS(NUDLDLISTSCENE *scene);
    void DisplayListSwapBuffersPS(void);
    void DisplayListSetAlphaPS(nudisplaylistitem_s *prev_item, nudisplaylistitem_s *item, f32 alpha);
    void DisplayListSetShadowCasterFlagPS(nudisplaylistitem_s *first_item, nudisplaylistitem_s *last_item, i32 enabled);
    void *DisplayListCreateSkinTransformPS(VARIPTR *buffer, NUMTX *skin_matrices,
                                           DEFORMERWEIGHTSARRAY *deformer_weights, NUDISPLAYLISTGEOM *geometry,
                                           NUDISPLAYLISTGEOM **render_geometry);
    void NuDisplayListAddClut(nudisplaylistitem_s *item, i32 clut_id);
    void NuDisplayListAddTexture(nudisplaylistitem_s *item, i32 tex_id);
    void NuDisplayListAddMaterialState(nudisplaylistitem_s *item, void *mtl);
    void NuDisplayListAddMicrocode(nudisplaylistitem_s *item, void *mtl);
    void NuDisplayListAddLightState(nudisplaylistitem_s *item, void *mtl);

    // Provided elsewhere (integration handled outside this TU; stubs live in
    // render_stubs.cpp):
    void RndrStateResetGlobalState(struct nuglobalrndrstate_s *state);
    void RndrStateResetSharedGlobalState(void);
    void RndrStateUpdateFx(void *state, nudisplaylistitem_s *item);
    void RndrStateUpdate(void *state, NUMTL *mtl, nudisplaylistitem_s *item);
    void DisplayListUpdateRenderState(void *dl, void *local_state);
    void NuDisplayListLinkItem(nudisplaylist_s *dl, u8 type, void *call_addr);
    void NuDisplayListLinkMtl(nudisplaylist_s *dl, NUMTL *mtl);
    void *DisplayListCreateGeomTransformPS(VARIPTR *buffer, NUMTX *transform, NUMTL *mtl, void *next, void *tx);
    void NuDisplayListBurstRndrSpecial(nuhspecial_s *handle, u32 count, NUMTX *matrices, i32 clip);
    VARIPTR *NuDisplayListLinkItems(nudisplaylist_s *dl, i32 count);
    void NuDisplayListLinkList(NUDISPLAYLIST *list, NUDISPLAYLISTITEM *first, NUDISPLAYLISTITEM *last);
    void NuDisplayListSetFxParam(i32 handle, i32 parameter, f32 value, i32 mode);
    void NuDisplayListBeginCriticalSection(void);
    void NuDisplayListEndCriticalSection(void);
    void NuDisplayListEndScene(void);
    void NuDisplayListDestroyFx(i32 handle);
    i32 NuDisplayListCreateFx(i32 type, i32 priority, i32 layer);
    void NuDisplayListDebugToFile(NUDISPLAYLISTITEM *item, i32 file);
    void NuDisplayListCaptureBegin(void);
    void NuDisplayListCaptureEnd(void);
    void NuDisplayListDraw2D(void);
    void NuDisplayListDrawAll(void);
    NUDLDLISTSCENE *NuDisplaySceneClone(NUDLDLISTSCENE *source, VARIPTR *buffer);
    void NuDisplaySceneClonePS(NUDLDLISTSCENE *source, NUDLDLISTSCENE *destination, VARIPTR *buffer);
    void DisplayListCreateFxList(VARIPTR *buffer, VARIPTR end, i32 count);
    VARIPTR *NuDisplayListLinkItemVP(nudisplaylist_s *dl, u8 type, void *call_addr, VARIPTR *buf);
    void *NuDisplayListPrepareFaceonPS(VARIPTR *buffer, void *faceon, NUMTX *transform);

    // Debug helpers consumed by NuDisplayListCaptureSortPriority (defined as
    // stubs in supportall.cpp / nucore_plain.cpp).
    void NuHtmlHeading1(const char *fmt, ...);
    void NuHtmlWrite(const char *text, ...);

    extern VARIPTR rndrstream_end;
    extern VARIPTR rndrstream_free;
    extern VARIPTR *display_list_buffer_end;

    static VARIPTR *NuDisplayListGetBuffer(void) {
        display_list_buffer->addr = ALIGN(display_list_buffer->addr, 0x10);

        return display_list_buffer;
    }
#ifdef __cplusplus
}

// Original core C++ entry point (_Z20NuDisplaySceneUnclipP16nudisplayscene_s).
void NuDisplaySceneUnclip(NUDLDLISTSCENE *scene);
#endif
