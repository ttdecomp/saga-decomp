#include "legoapi/gizmos/transport/tubes.h"

#include "decomp.h"

#include "gameapi/edtools/edfile.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

extern void GameAudio_PlaySfx(i32 sfx_id, NUVEC *position, i32 flags, i32 volume);
extern i32 GameAudio_GetPlrSfxBits(void *object);


i32 ObjInTube(GameObject_s *object) {
    if (LEGOCONTEXT_TUBE != -1 && object->character_context == LEGOCONTEXT_TUBE) {
        return 1;
    }
    if (LEGOCONTEXT_GLIDE != -1 && object->character_context == LEGOCONTEXT_GLIDE && object->field_0x788 != NULL) {
        return 1;
    }
    return 0;
}

static const i32 TUBE_AUDIO_EVENT = 0x2e;
static const u32 CHARACTER_MODEL_FLAG_TUBE_USER = 0x10;
static const u32 GAME_CHARACTER_FLAG_EXCLUDE_DIRECTIONAL_TUBE = 0x100;
static const f32 DIRECTIONAL_TUBE_ENTRY_RADIUS_SQUARED = 0.1225f;
static const i32 DIRECTIONAL_TUBE_MAX_NORMAL_Y_ANGLE = 0x3c71;
static const i32 DIRECTIONAL_TUBE_MIN_ALIGNMENT_ANGLE = 0x238e;

struct TUBEPROGRESS {
    u32 visible_mask;
    u32 active_mask;
};

static i32 Tubes_GetMaxGizmos(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    return world != NULL ? world->current_level->max_tubes : 0;
}

static void Tubes_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    for (i32 index = 0; index < world->tube_count; ++index) {
        if (NuStrLen(world->tubes[index].name) != 0) {
            AddGizmo(gizmo_sys, type_id, NULL, &world->tubes[index]);
        }
    }
}

static void Tubes_Update(void *world_ptr, void *, float frame_time) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL || world->tubes == NULL || world->tube_count <= 0) {
        return;
    }

    TUBE *tube = world->tubes;
    for (i32 tube_index = 0; tube_index < world->tube_count; ++tube_index, ++tube) {
        if (tube->audio_cooldown > 0.0f) {
            tube->audio_cooldown -= frame_time;
        }
        if (tube->audio_cooldown < 0.0f) {
            tube->audio_cooldown = 0.0f;
        }

        if ((tube->flags & TUBE_FLAG_DIRECTIONAL) != 0) {
            if ((tube->flags & (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE)) == (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE)) {
                GameAudio_PlaySfx(TUBE_AUDIO_EVENT, &tube->position, 0, 0);

                GameObject_s *object = Obj;
                for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index, ++object) {
                    if ((object->apiobj.field_0x1f8 & (APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER)) !=
                            (APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER) ||
                        object->apiobj.field_0x287 != 0 ||
                        (object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_TUBE_USER) == 0 ||
                        (GCDataList[object->id].field_0x94 & GAME_CHARACTER_FLAG_EXCLUDE_DIRECTIONAL_TUBE) != 0) {
                        continue;
                    }

                    f32 horizontal_distance_squared;
                    if (Tube_InCylinder(object, tube, &horizontal_distance_squared, 0) != 0) {
                        const f32 delta_x = tube->position.x - object->apiobj.pos_x;
                        const f32 delta_z = tube->position.z - object->apiobj.pos_z;
                        const i32 target_angle = NuAtan2D(delta_x, delta_z);
                        object->apiobj.movement_direction.x = NuTrigTable[static_cast<u16>(target_angle) >> 1];
                        object->apiobj.movement_direction.z =
                            NuTrigTable[(static_cast<u16>(target_angle) + 0x4000) >> 1 & 0x7fff];
                        object->context_target_position = &tube->position;

                        if (Tube_IsObjBitSet(tube, object->apiobj.field_0x289) == 0) {
                            if (object->field_0x1084 != 0) {
                                if (DIRECTIONAL_TUBE_ENTRY_RADIUS_SQUARED > horizontal_distance_squared) {
                                    if (NuTrigTable[DIRECTIONAL_TUBE_MAX_NORMAL_Y_ANGLE] >
                                        NuFabs(object->contact_normal.y)) {
                                        const f32 inverse_distance = 1.0f / NuFsqrt(horizontal_distance_squared);
                                        const f32 entry_alignment =
                                            delta_x * inverse_distance * object->contact_normal.x +
                                            delta_z * inverse_distance * object->contact_normal.z;

                                        if (-NuTrigTable[DIRECTIONAL_TUBE_MIN_ALIGNMENT_ANGLE] > entry_alignment) {
                                            GameAudio_PlaySfx(TUBE_AUDIO_EVENT, &object->apiobj.collision_position,
                                                              GameAudio_GetPlrSfxBits(object), 0);
                                            Tube_SetObjBit(tube, object->apiobj.field_0x289);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        tube->occupied_object_masks[0] = 0;
        tube->occupied_object_masks[1] = 0;
    }
}

static void Tubes_Draw(void *, void *, float) {
}

static char *Tube_GetGizmoName(GIZMO *gizmo) {
    return gizmo != NULL ? static_cast<TUBE *>(gizmo->object)->name : NULL;
}

static i32 Tube_GetOutput(GIZMO *gizmo, i32, i32) {
    TUBE *tube = static_cast<TUBE *>(gizmo->object);
    return (tube->flags & (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE)) == (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE);
}

static char *Tube_GetOutputName(GIZMO *, i32) {
    return const_cast<char *>("Active");
}

static i32 Tube_GetNumOutputs(GIZMO *) {
    return 1;
}

static void Tube_Activate(GIZMO *gizmo, i32 active) {
    if (gizmo != NULL) {
        TUBE *tube = static_cast<TUBE *>(gizmo->object);
        tube->active = active != 0;
    }
}

static i32 Tube_ActivateRev(GIZMO *gizmo, i32 reverse, i32 check_only) {
    if (gizmo == NULL || gizmo->object == NULL) {
        return 0;
    }

    TUBE *tube = static_cast<TUBE *>(gizmo->object);
    if ((check_only & 1) != 0) {
        return ((tube->flags & TUBE_FLAG_REVERSED) != 0) != (reverse != 0);
    }

    if (reverse != 0) {
        tube->flags = (tube->flags & ~TUBE_FLAG_ACTIVE) | TUBE_FLAG_REVERSED;
    } else {
        tube->flags = (tube->flags | TUBE_FLAG_ACTIVE) & ~TUBE_FLAG_REVERSED;
    }
    return 1;
}

static void Tube_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo != NULL) {
        TUBE *tube = static_cast<TUBE *>(gizmo->object);
        tube->visible = visible != 0;
    }
}

static NUVEC *Tube_GetPos(GIZMO *gizmo) {
    return gizmo != NULL ? &static_cast<TUBE *>(gizmo->object)->position : NULL;
}

static void *Tubes_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(TUBEPROGRESS));
}

static void Tubes_ClearProgress(void *, void *progress_ptr) {
    TUBEPROGRESS *progress = static_cast<TUBEPROGRESS *>(progress_ptr);
    if (progress != NULL) {
        progress->visible_mask = 0xffffffff;
        progress->active_mask = 0xffffffff;
    }
}

static void Tubes_StoreProgress(void *world_ptr, void *, void *progress_ptr) {
    TUBEPROGRESS *progress = static_cast<TUBEPROGRESS *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    progress->visible_mask = 0xffffffff;
    progress->active_mask = 0xffffffff;

    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL || world->tubes == NULL || world->tube_count <= 0) {
        return;
    }

    for (i32 index = 0; index < world->tube_count && index < 32; ++index) {
        const u32 tube_bit = 1U << index;
        if ((world->tubes[index].flags & TUBE_FLAG_VISIBLE) == 0) {
            progress->visible_mask &= ~tube_bit;
        }
        if ((world->tubes[index].flags & TUBE_FLAG_ACTIVE) == 0) {
            progress->active_mask &= ~tube_bit;
        }
    }
}

static void Tubes_Reset(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL || world->tubes == NULL || world->tube_count <= 0) {
        return;
    }

    TUBEPROGRESS *progress = static_cast<TUBEPROGRESS *>(progress_ptr);
    TUBE *tube = world->tubes;
    for (i32 index = 0; index < world->tube_count; ++index, ++tube) {
        tube->top = tube->position.y + tube->height;
        tube->radius_squared = tube->radius * tube->radius;
        tube->flags |= TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE;

        if (index < 32 && progress != NULL) {
            const u32 tube_bit = 1U << index;
            const i32 visible_flag = (progress->visible_mask & tube_bit) != 0;
            tube->flags = static_cast<u8>((tube->flags & ~TUBE_FLAG_VISIBLE) | (visible_flag << 1));
            const i32 active_flag = (progress->active_mask & tube_bit) != 0;
            tube->flags = static_cast<u8>((tube->flags & ~TUBE_FLAG_ACTIVE) | active_flag);
        }
    }
}

static void *Tubes_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    void *reserved_space = NULL;
    world->tubes = NULL;
    world->tube_count = 0;

    if (world->current_level->max_tubes != 0) {
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 16);
        world->tubes = reinterpret_cast<TUBE *>(world->giz_buffer.addr);
        world->giz_buffer.addr += world->current_level->max_tubes * sizeof(TUBE);
        reserved_space = world->tubes;
    }
    return reserved_space;
}

static i32 Tubes_Load(void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world->tube_count != 0) {
        return 0;
    }

    const i32 version = EdFileReadInt();
    world->tube_count = EdFileReadInt();

    TUBE *tube = world->tubes;
    for (i32 index = 0; index < world->tube_count; ++index, ++tube) {
        EdFileRead(tube->name, sizeof(tube->name));
        EdFileReadNuVec(&tube->position);
        tube->height = EdFileReadFloat();
        tube->radius = EdFileReadFloat();

        if (version > 1) {
            const i32 directional = EdFileReadChar() != 0;
            tube->field_0x24 = 1.25f;
            tube->flags = static_cast<u8>((tube->flags & ~TUBE_FLAG_DIRECTIONAL) | (directional << 2));
        } else {
            tube->field_0x24 = 1.25f;
            tube->flags &= static_cast<u8>(~TUBE_FLAG_DIRECTIONAL);
        }
    }

    return 1;
}

ADDGIZMOTYPE *Tubes_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "Tube";
    addtype.prefix = "";
    addtype.fns.unknown1 = 8;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = Tubes_GetMaxGizmos;
    addtype.fns.get_pos_fn = Tube_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = Tubes_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = Tubes_Update;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = Tubes_Draw;
    addtype.fns.get_gizmo_name_fn = Tube_GetGizmoName;
    addtype.fns.get_output_fn = Tube_GetOutput;
    addtype.fns.get_output_name_fn = Tube_GetOutputName;
    addtype.fns.get_num_outputs_fn = Tube_GetNumOutputs;
    addtype.fns.activate_fn = Tube_Activate;
    addtype.fns.activate_rev_fn = Tube_ActivateRev;
    addtype.fns.set_visibility_fn = Tube_SetVisibility;
    addtype.fns.allocate_progress_data_fn = Tubes_AllocateProgressData;
    addtype.fns.clear_progress_fn = Tubes_ClearProgress;
    addtype.fns.store_progress_fn = Tubes_StoreProgress;
    addtype.fns.reset_fn = Tubes_Reset;
    addtype.fns.reserve_buffer_space_fn = Tubes_ReserveBufferSpace;
    addtype.fns.load_fn = Tubes_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;

    return &addtype;
}
