#include "legoapi/gizmos/trigger/gizspecial.h"

#include <string.h>

#include "decomp.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/nustring.h"

i32 gizspecial_gizmotype_id = -1;

static char gizSpec_prefix[] = "qaz_";

static const i32 GIZSPECIAL_PROGRESS_WORD_COUNT = 8;

struct GIZSPECIALPROGRESS {
    u32 active[GIZSPECIAL_PROGRESS_WORD_COUNT];
    u32 reversed[GIZSPECIAL_PROGRESS_WORD_COUNT];
};
DECOMP_ASSERT(sizeof(GIZSPECIALPROGRESS) == 0x40, "GIZSPECIALPROGRESS ABI");

static const i32 GIZSPECIAL_PROGRESS_BITS = GIZSPECIAL_PROGRESS_WORD_COUNT * 32;

static i32 GizSpecial_GetMaxGizmos(void *special) {
    WORLDINFO *world = static_cast<WORLDINFO *>(special);
    return world->current_level->max_giz_specials;
}

static void GizSpecial_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world != NULL) {
        for (i32 i = 0; i < world->giz_special_sys->count; ++i) {
            if (NuStrLen(world->giz_special_sys->specials[i].name) != 0) {
                AddGizmo(gizmo_sys, type_id, NULL, &world->giz_special_sys->specials[i]);
            }
        }
    }
}

static char *GizSpecial_GetGizmoName(GIZMO *gizmo) {
    if (gizmo != NULL) {
        return GizSpecial_GetName(static_cast<GIZSPECIAL *>(gizmo->object));
    }
    return NULL;
}

static i32 GizSpecial_GetOutput(GIZMO *gizmo, i32 output_index, i32 include_inactive) {
    if (gizmo == NULL) {
        return 0;
    }

    GIZSPECIAL *special = static_cast<GIZSPECIAL *>(gizmo->object);
    if (special == NULL || special->anim_set->object_count == 0 ||
        NuSpecialExistsFn(&special->anim_set->objects->special) == 0) {
        return 0;
    }
    if (special->is_active == 0 && include_inactive == 0) {
        return 0;
    }

    if ((special->anim_set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) == 0) {
        GameAnimSet_EvaluateState(special->anim_set);
    }
    if (output_index == 0) {
        return special->anim_set->state == GAMEANIMSET_STATE_AT_END;
    } else if (output_index == 1) {
        return special->anim_set->state != GAMEANIMSET_STATE_AT_START;
    }
    return 0;
}

static char *GizSpecial_GetOutputName(GIZMO *gizmo, i32 output_index) {
    if (output_index == 0) {
        return "AtEnd";
    }
    if (output_index == 1) {
        return "NotAtStart";
    }
    return NULL;
}

static i32 GizSpecial_GetNumOutputs(GIZMO *gizmo) {
    return 2;
}

static void GizSpecial_Activate(GIZMO *gizmo, i32 active) {
    if (gizmo == NULL) {
        return;
    }

    GIZSPECIAL *special = static_cast<GIZSPECIAL *>(gizmo->object);
    if (special == NULL) {
        return;
    }

    if (active != 0) {
        GameAnimSet_JumpToStart(special->anim_set);
        GameAnimSet_Play(special->anim_set, 1.0f, 1);
        special->flags = static_cast<GIZSPECIAL_FLAGS>(special->flags | GIZSPECIAL_FLAG_ACTIVE);
        return;
    }

    GameAnimSet_Stop(special->anim_set);
    special->flags = static_cast<GIZSPECIAL_FLAGS>(special->flags & ~GIZSPECIAL_FLAG_ACTIVE);
}

static i32 GizSpecial_ActivateRev(GIZMO *gizmo, i32 reversed, i32 test_only) {
    if (gizmo == NULL) {
        return 0;
    }

    GIZSPECIAL *special = static_cast<GIZSPECIAL *>(gizmo->object);
    if (special == NULL) {
        return 0;
    }

    const i32 was_reversed = special->flags & GIZSPECIAL_FLAG_REVERSED;
    if ((test_only & 1) != 0) {
        return was_reversed != reversed;
    }

    if (reversed != 0) {
        GameAnimSet_Play(special->anim_set, -1.0f, 1);
        special->flags = static_cast<GIZSPECIAL_FLAGS>(special->flags | GIZSPECIAL_FLAG_REVERSED);
        return 1;
    }

    GameAnimSet_Play(special->anim_set, 1.0f, 1);
    special->flags = static_cast<GIZSPECIAL_FLAGS>(special->flags & ~GIZSPECIAL_FLAG_REVERSED);
    return 1;
}

static void GizSpecial_SetVisibility(GIZMO *gizmo, i32 visibility) {
    if (gizmo != NULL) {
        GIZSPECIAL *special = static_cast<GIZSPECIAL *>(gizmo->object);
        if (special != NULL) {
            GameAnimSet_SetVisibility(special->anim_set, visibility);
        }
    }
}

static NUVEC *GizSpecial_GetPos(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZSPECIAL *special = static_cast<GIZSPECIAL *>(gizmo->object);
        if (special != NULL && special->anim_set->object_count != 0) {
            if (NuSpecialExistsFn(&special->anim_set->objects->special) != 0) {
                return NuSpecialGetPos(&special->anim_set->objects->special);
            }
        }
    }
    return NULL;
}

static i32 GizSpecial_UsingSpecial(GIZMO **result, void *world_ptr, i32 result_capacity, char *name) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    i32 result_count = 0;

    if (world != NULL && world->giz_special_sys->specials != NULL) {
        for (i32 i = 0; i < world->giz_special_sys->count; ++i) {
            GIZSPECIAL *special = &world->giz_special_sys->specials[i];
            if (special->anim_set != NULL) {
                char *special_name = GizSpecial_GetName(special);
                if (NuStrICmp(name, special_name) == 0) {
                    result[result_count++] = GizmoFindByName(world->gizmo_sys, -1, special_name);
                    if (result_count == result_capacity) {
                        result_count = -1;
                        break;
                    }
                }
            }
        }
    }

    return result_count;
}

static void *GizSpecial_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(GIZSPECIALPROGRESS));
}

static void GizSpecial_ClearProgress(void *, void *progress_ptr) {
    GIZSPECIALPROGRESS *progress = static_cast<GIZSPECIALPROGRESS *>(progress_ptr);
    if (progress != NULL) {
        memset(progress->active, 0xff, sizeof(progress->active));
        memset(progress->reversed, 0, sizeof(progress->reversed));
    }
}

static void GizSpecial_StoreProgress(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZSPECIALPROGRESS *progress = static_cast<GIZSPECIALPROGRESS *>(progress_ptr);

    if (progress == NULL) {
        return;
    }
    GizSpecial_ClearProgress(NULL, progress);

    if (world != NULL && world->giz_special_sys != NULL && world->giz_special_sys->count != 0) {
        const i32 count = world->giz_special_sys->count;
        GIZSPECIAL *special = world->giz_special_sys->specials;
        for (i32 i = 0; i < count; ++i, ++special) {
            const i32 word_index = i >> 5;
            const u32 bit = 1u << (i & 31);
            if ((special->flags & GIZSPECIAL_FLAG_ACTIVE) == 0) {
                progress->active[word_index] &= ~bit;
            }
            if ((special->flags & GIZSPECIAL_FLAG_REVERSED) != 0) {
                progress->reversed[word_index] |= bit;
            }
        }
    }
}

static void GizSpecial_Reset(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZSPECIALPROGRESS *progress = static_cast<GIZSPECIALPROGRESS *>(progress_ptr);

    if (world == NULL || world->giz_special_sys == NULL || world->giz_special_sys->count == 0) {
        return;
    }

    GIZSPECIAL *special = world->giz_special_sys->specials;
    for (i32 i = 0; i < world->giz_special_sys->count; ++i, ++special) {
        special->is_active = 1;
        special->is_reversed = 0;

        if (i < GIZSPECIAL_PROGRESS_BITS && progress != NULL) {
            const i32 word_index = i >> 5;
            const u32 bit = 1u << (i & 31);
            special->is_active = (progress->active[word_index] & bit) != 0;
            special->is_reversed = (progress->reversed[word_index] & bit) != 0;
        }
    }
}

void *GizSpecial_ReserveBuffer(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);

    if (world->current_level->max_giz_specials != 0) {
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        GIZSPECIALSYS_s *special_sys = reinterpret_cast<GIZSPECIALSYS_s *>(world->giz_buffer.addr);
        world->giz_special_sys = special_sys;
        world->giz_buffer.addr += sizeof(GIZSPECIALSYS_s);

        special_sys->anim_pool = GameAnimSet_CreateObjectPool(&world->giz_buffer, &world->unknown_0108, 0,
                                                              world->current_level->max_giz_specials);

        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        special_sys->specials = reinterpret_cast<GIZSPECIAL_s *>(world->giz_buffer.addr);
        world->giz_buffer.addr += world->current_level->max_giz_specials * sizeof(GIZSPECIAL_s);
        memset(special_sys->specials, 0, world->current_level->max_giz_specials * sizeof(GIZSPECIAL_s));

        for (i32 i = 0; i < world->current_level->max_giz_specials; ++i) {
            special_sys->specials[i].anim_set = GameAnimSet_Create(&world->giz_buffer, &world->unknown_0108,
                                                                   special_sys->anim_pool, world->game_anim_sys);
        }

        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    }

    return world->giz_special_sys;
}

GIZMO *createGizSpecial(void *, char *name) {
    WORLDINFO *world = WorldInfo_CurrentlyLoading();
    if (world == NULL || name == NULL)
        return NULL;
    nuhspecial_s scene_special;
    char gizmo_name[32];
    NuSpecialFind(world->current_gscn, &scene_special, name, 0);
    NuStrCpy(gizmo_name, gizSpec_prefix);
    NuStrNCat(gizmo_name, name, 32 - NuStrLen(gizSpec_prefix));
    if (!NuSpecialExistsFn(&scene_special))
        return NULL;
    GIZMO *gizmo = GizmoFindByName(world->gizmo_sys, gizspecial_gizmotype_id, gizmo_name);
    if (gizmo != NULL)
        return gizmo;
    GIZSPECIALSYS_s *system = world->giz_special_sys;
    if (system->count >= world->current_level->max_giz_specials)
        return NULL;
    GIZSPECIAL_s *special = &system->specials[system->count];
    GameAnimSet_AddObject(special->anim_set, &scene_special, 1.0f, 1000000000.0f, 0);
    ++world->giz_special_sys->count;
    NuStrCpy(special->name, gizSpec_prefix);
    NuStrNCat(special->name, name, 32 - NuStrLen(gizSpec_prefix));
    return AddGizmo(world->gizmo_sys, gizspecial_gizmotype_id, NULL, special);
}

char *GizSpecial_GetName(GIZSPECIAL_s *special) {
    return special->name;
}

void GizSpecial_FindByName(char *, WORLDINFO_s *) {
}

ADDGIZMOTYPE *GizSpecial_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "NuSpecial";
    addtype.prefix = gizSpec_prefix;
    addtype.fns.unknown1 = 0;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizSpecial_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizSpecial_GetPos;
    addtype.fns.using_special_fn = GizSpecial_UsingSpecial;
    addtype.fns.add_gizmos_fn = GizSpecial_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = NULL;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = NULL;
    addtype.fns.get_gizmo_name_fn = GizSpecial_GetGizmoName;
    addtype.fns.get_output_fn = GizSpecial_GetOutput;
    addtype.fns.get_output_name_fn = GizSpecial_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizSpecial_GetNumOutputs;
    addtype.fns.activate_fn = GizSpecial_Activate;
    addtype.fns.activate_rev_fn = GizSpecial_ActivateRev;
    addtype.fns.set_visibility_fn = GizSpecial_SetVisibility;
    addtype.fns.allocate_progress_data_fn = GizSpecial_AllocateProgressData;
    addtype.fns.clear_progress_fn = GizSpecial_ClearProgress;
    addtype.fns.store_progress_fn = GizSpecial_StoreProgress;
    addtype.fns.reset_fn = GizSpecial_Reset;
    addtype.fns.reserve_buffer_space_fn = GizSpecial_ReserveBuffer;
    addtype.fns.load_fn = NULL;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;
    gizspecial_gizmotype_id = type_id;

    return &addtype;
}
