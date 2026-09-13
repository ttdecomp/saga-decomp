#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/world/world.h"
#include "legoapi/legoapi_types.h"

u32 GizObstacles_TotalScore(void *world) {
    GIZOBSTACLESYS_s *system = static_cast<WORLDINFO_s *>(world)->giz_obstacle_sys;
    u32 total = 0;
    if (system != NULL) {
        GIZOBSTACLE_s *item = system->obstacles;
        if (item != NULL) {
            for (i32 i = 0; i < system->count; ++i, ++item)
                total += item->completion_score;
        }
    }
    return total;
}

#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "globals.h"
#include "legoapi/actions/combat/shovesys.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/gizmo/base/GizObstacleObjectInterface.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/gamemessage.h"
#include "legoapi/render/core/terrain.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numusic/sfx.h"

namespace {

    enum : i32 {
        GIZOBSTACLE_PROGRESS_CAPACITY = 128,
        GIZOBSTACLE_PROGRESS_WORDS = GIZOBSTACLE_PROGRESS_CAPACITY / 32,
    };

    enum GIZOBSTACLE_ACTIVATE_REVERSE_FLAGS : u32 {
        GIZOBSTACLE_ACTIVATE_REVERSE_VALIDATE = 0x01,
        GIZOBSTACLE_ACTIVATE_REVERSE_ALLOW_MATCHING_STATE = 0x04,
    };

    struct GIZOBSTACLEPROGRESS_s {
        u32 progress_flag_0[GIZOBSTACLE_PROGRESS_WORDS];
        u32 progress_flag_1[GIZOBSTACLE_PROGRESS_WORDS];
        u32 runtime_flag_0[GIZOBSTACLE_PROGRESS_WORDS];
        u32 runtime_flag_2[GIZOBSTACLE_PROGRESS_WORDS];
        u32 runtime_flag_3[GIZOBSTACLE_PROGRESS_WORDS];
        u32 runtime_flag_6[GIZOBSTACLE_PROGRESS_WORDS];
        u32 runtime_flag_7[GIZOBSTACLE_PROGRESS_WORDS];
    };

    DECOMP_ASSERT(sizeof(GIZOBSTACLEPROGRESS_s) == 0x70, "GIZOBSTACLE progress ABI");

    static void ClearObstacleProgress(GIZOBSTACLEPROGRESS_s *progress) {
        if (progress == NULL) {
            return;
        }

        for (i32 word = 0; word < GIZOBSTACLE_PROGRESS_WORDS; ++word) {
            progress->progress_flag_0[word] = 0xffffffff;
            progress->progress_flag_1[word] = 0xffffffff;
            progress->runtime_flag_0[word] = 0;
            progress->runtime_flag_2[word] = 0;
            progress->runtime_flag_3[word] = 0;
            progress->runtime_flag_6[word] = 0;
            progress->runtime_flag_7[word] = 0;
        }
    }

} // namespace

static void Gizobstacle_ReadAnimSetData(GAMEANIMOBJ_s *object, unsigned char version);

static NUVEC *gizobstacletriggers[16];
static i32 ngizobstacletriggers;

i32 obstacle_gizmotype_id = -1;

void GIZOBSTACLE_s::ClearMechObjectInterface() {
    if (mech_object_interface != NULL)
        delete mech_object_interface;
}

MechObjectInterface *GIZOBSTACLE_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL)
        new GizObstacleObjectInterface(*this);
    return mech_object_interface;
}

static i32 GizObstacles_GetMaxGizmos(void *obstacle) {
    WORLDINFO *world = static_cast<WORLDINFO *>(obstacle);
    if (world == NULL) {
        return 0;
    }
    return world->current_level->max_obstacles;
}

static void GizObstacles_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *, void *data) {
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    if (obstacle_sys != NULL) {
        if (obstacle_sys->count != 0) {
            for (i32 i = 0; i < obstacle_sys->count; ++i) {
                if (NuStrLen(obstacle_sys->obstacles[i].name) != 0) {
                    AddGizmo(gizmo_sys, type_id, NULL, &obstacle_sys->obstacles[i]);
                }
            }
        }
    }
}

static void GizObstacles_Update(void *world_ptr, void *data, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    if (obstacle_sys == NULL || world == NULL || world->gizmo_sys == NULL || world->gizmo_sys->sets == NULL) {
        return;
    }

    GIZMOSET &gizmo_set = world->gizmo_sys->sets[obstacle_gizmotype_id];
    obstacle_sys->active_gizmo_count = 0;

    for (i32 index = 0; index < gizmo_set.count; ++index) {
        GIZMO *gizmo = &gizmo_set.gizmos[index];
        GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
        if (obstacle == NULL) {
            continue;
        }

        GameObject_s *previous_triggering_object = obstacle->triggering_object;
        obstacle->triggering_object = NULL;
        if ((obstacle->progress_flags & GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) == 0 ||
            (obstacle->progress_flags &
             (GIZOBSTACLE_PROGRESS_FLAG_ENABLED | GIZOBSTACLE_PROGRESS_FLAG_EXTERNAL_CONTROL)) == 0 ||
            obstacle->anim_set == NULL || (obstacle->control_flags & GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE) != 0 ||
            (obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_DESTROYED) != 0) {
            continue;
        }

        GAMEANIMSET_s *anim_set = obstacle->anim_set;
        if (obstacle->mode == 7) {
            if ((obstacle->progress_flags & GIZOBSTACLE_PROGRESS_FLAG_PUSH_CONTROL) == 0) {
                if (obstacle->state == 1) {
                    obstacle->animation_speed = -1.0f;
                    GizObstacle_PlayForwards(obstacle);
                    anim_set = obstacle->anim_set;
                } else {
                    obstacle->animation_speed = 0.0f;
                }
            } else {
                obstacle->triggering_object = previous_triggering_object;
                if (obstacle->animation_speed != 0.0f) {
                    GizObstacle_PlayForwards(obstacle);
                    anim_set = obstacle->anim_set;
                }
            }
            GizObstacle_Stop(obstacle);
            anim_set = obstacle->anim_set;
        } else if ((obstacle->progress_flags & GIZOBSTACLE_PROGRESS_FLAG_EXTERNAL_CONTROL) == 0) {
            GIZOBSTACLEUPDATEFN update_fn = gizobstacleupdatefns[obstacle->mode];
            if (update_fn != NULL) {
                update_fn(obstacle);
                anim_set = obstacle->anim_set;
            }
        } else if (obstacle->animation_speed == 0.0f) {
            GizObstacle_Stop(obstacle);
            anim_set = obstacle->anim_set;
        } else {
            GizObstacle_PlayForwards(obstacle);
            anim_set = obstacle->anim_set;
        }

        if ((anim_set->flags & 7) != 0) {
            GizObstacle_EvalAveragePosAndRadius(obstacle, 2);

            if ((obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_ANIM_OBJECT_BIT_1) != 0) {
                for (GAMEANIMOBJ_s *object = anim_set->objects; object != NULL; object = object->next) {
                    i16 *object_data = static_cast<i16 *>(object->object_data);
                    if (object_data != NULL && (object_data[0] & 2) != 0) {
                        AddShoveObject(&object->special, object_data[1]);
                    }
                }
                anim_set = obstacle->anim_set;
            }

            const bool playing_reverse = obstacle->animation_speed < 0.0f;
            const i16 sfx_id = playing_reverse ? obstacle->stop_sfx_id : obstacle->start_sfx_id;
            const GAMEANIMSET_STATE endpoint = playing_reverse ? GAMEANIMSET_STATE_AT_END : GAMEANIMSET_STATE_AT_START;
            if (sfx_id != -1 && (anim_set->state == endpoint || IsSfxLooping(sfx_id) != 0)) {
                GameAudio_PlaySfxById(sfx_id, &obstacle->evaluated_position, 0, 0);
                anim_set = obstacle->anim_set;
            }
        }

        if (anim_set->state == GAMEANIMSET_STATE_AT_END) {
            obstacle->runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED);
        }

        const bool completion_mode = obstacle->state == 0 || obstacle->state == 3 ||
                                     (obstacle->config_flags & GIZOBSTACLE_CONFIG_RESET_AFTER_COMPLETION) != 0;
        const bool animation_finished =
            ((anim_set->flags & GAMEANIMSET_FLAG_STOP_REQUESTED) != 0 || anim_set->animated_object_count == 0) &&
            anim_set->state == GAMEANIMSET_STATE_AT_END;
        if (completion_mode && animation_finished) {
            if (obstacle->blowup_type != -1) {
                if ((obstacle->config_flags & GIZOBSTACLE_CONFIG_BLOWUP_AT_ANIM_OBJECTS) == 0) {
                    GizmoBlowUpTypeBlowUp(world, obstacle->blowup_type, &obstacle->evaluated_position);
                } else {
                    for (GAMEANIMOBJ_s *object = anim_set->objects; object != NULL; object = object->next) {
                        NUVEC *draw_position = NuSpecialGetDrawPos(&object->special);
                        if (draw_position != NULL) {
                            GizmoBlowUpTypeBlowUp(world, obstacle->blowup_type, draw_position);
                        }
                    }
                }
                GameAnimSet_SetVisibility(obstacle->anim_set, 0);
                obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_DESTROYED;
            }

            if (obstacle->pickup_count != 0 &&
                ((obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED) == 0 ||
                 (obstacle->config_flags & GIZOBSTACLE_CONFIG_RESET_AFTER_COMPLETION) != 0)) {
                NUVEC pickup_position;
                NUVEC pickup_direction;
                NuVecAdd(&pickup_position, &obstacle->evaluated_position, &obstacle->pickup_offset);
                NuVecRotateX(&pickup_direction, &v010, obstacle->pickup_direction_x);
                NuVecRotateY(&pickup_direction, &pickup_direction, obstacle->pickup_direction_y);
                AddPickups(static_cast<u16>(obstacle->pickup_count), 0, 0, 0, &pickup_position, &pickup_direction, 2.0f,
                           -1, obstacle->pickup_scatter_height, 2000000.0f, NULL, 1, 0, true);
                obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED;
            }

            if (obstacle->state == 3) {
                GameAnimSet_JumpToStart(obstacle->anim_set);
                GizObstacle_EvalAveragePosAndRadius(obstacle, 2);
                GameAnimSet_SetVisibility(obstacle->anim_set, 1);
                obstacle->runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_DESTROYED);
            }
        }

        if ((obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_DESTROYED) == 0 &&
            obstacle->anim_set->state != GAMEANIMSET_STATE_AT_END &&
            (obstacle->config_flags &
             (GIZOBSTACLE_CONFIG_ALWAYS_RUN_PROXIMITY | GIZOBSTACLE_CONFIG_ADD_TO_ACTIVE_LIST)) != 0) {
            obstacle_sys->active_gizmos[obstacle_sys->active_gizmo_count++] = gizmo;
        }

        obstacle->animation_speed = 1.0f;
        obstacle->progress_flags &=
            static_cast<u8>(~(GIZOBSTACLE_PROGRESS_FLAG_EXTERNAL_CONTROL | GIZOBSTACLE_PROGRESS_FLAG_PUSH_CONTROL));
    }

    ngizobstacletriggers = 0;
}

static void GizObstacles_Draw(void *world_ptr, void *data, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);

    if (obstacle_sys != NULL) {
        GIZOBSTACLE_s *obstacle = obstacle_sys->obstacles;
        if (obstacle_sys->count != 0) {
            for (i32 index = 0; obstacle_sys->count > index; ++index, ++obstacle) {
                if ((obstacle->progress_flags & GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) != 0 &&
                    (obstacle->room_id < 0 || world->rooms_visible_ptr[obstacle->room_id] != 0) &&
                    static_cast<i8>(obstacle->runtime_flags) >= 0 &&
                    (obstacle->config_flags & GIZOBSTACLE_CONFIG_DRAW_REFLECTION) != 0) {
                    GameAnimSet_DrawReflection(obstacle->anim_set, 2, obstacle->reflection_alpha, NULL);
                }
            }
        }
    }
}

static char *GizmoObstacle_GetGizmoName(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
        if (obstacle != NULL) {
            return obstacle->name;
        }
    }
    return NULL;
}

static i32 GizmoObstacle_GetOutput(GIZMO *gizmo, i32 output_index, i32 ignore_activation_state) {
    GIZOBSTACLE_s *obstacle = gizmo != NULL ? static_cast<GIZOBSTACLE_s *>(gizmo->object) : NULL;
    if (obstacle == NULL) {
        return 0;
    }
    if ((obstacle->progress_flags & (GIZOBSTACLE_PROGRESS_FLAG_ENABLED | GIZOBSTACLE_PROGRESS_FLAG_VISIBLE)) !=
            (GIZOBSTACLE_PROGRESS_FLAG_ENABLED | GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) &&
        ignore_activation_state == 0) {
        return 0;
    }

    switch (output_index) {
        case GIZOBSTACLE_OUTPUT_AT_END:
            return obstacle->anim_set != NULL && obstacle->anim_set->state == GAMEANIMSET_STATE_AT_END;
        case GIZOBSTACLE_OUTPUT_NOT_AT_START:
            return obstacle->anim_set != NULL && obstacle->anim_set->state != GAMEANIMSET_STATE_AT_START;
        case GIZOBSTACLE_OUTPUT_PROXIMITY:
            return obstacle->anim_set != NULL && obstacle->proximity_output != 0;
        case GIZOBSTACLE_OUTPUT_AT_START:
            return obstacle->anim_set != NULL && obstacle->anim_set->state == GAMEANIMSET_STATE_AT_START;
        case GIZOBSTACLE_OUTPUT_PLAYING_FORWARD:
            return obstacle->anim_set != NULL && obstacle->anim_set->state == GAMEANIMSET_STATE_ACTIVE_FORWARD;
        default:
            return 0;
    }
}

static char *GizmoObstacle_GetOutputName(GIZMO *, i32 output_index) {
    switch (output_index) {
        case GIZOBSTACLE_OUTPUT_AT_END:
            return const_cast<char *>("AtEnd");
        case GIZOBSTACLE_OUTPUT_NOT_AT_START:
            return const_cast<char *>("NotAtStart");
        case GIZOBSTACLE_OUTPUT_PROXIMITY:
            return const_cast<char *>("Proximity");
        case GIZOBSTACLE_OUTPUT_AT_START:
            return const_cast<char *>("AtStart");
        case GIZOBSTACLE_OUTPUT_PLAYING_FORWARD:
            return const_cast<char *>("PlayingForward");
        default:
            return NULL;
    }
}

static i32 GizmoObstacle_GetNumOutputs(GIZMO *) {
    return 5;
}

static void GizmoObstacle_Activate(GIZMO *gizmo, i32 activate) {
    if (gizmo == NULL) {
        return;
    }
    GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
    if (obstacle == NULL) {
        return;
    }

    if (activate == 0) {
        obstacle->progress_flags &= static_cast<u8>(~GIZOBSTACLE_PROGRESS_FLAG_ENABLED);
        obstacle->proximity_output = 0;
        return;
    }

    if ((obstacle->progress_flags & GIZOBSTACLE_PROGRESS_FLAG_ENABLED) == 0) {
        GameAnimSet_JumpToStart(obstacle->anim_set);
    }
    obstacle->proximity_output = 0;
    obstacle->progress_flags |= GIZOBSTACLE_PROGRESS_FLAG_ENABLED;
    obstacle->runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_DESTROYED);
    obstacle->control_flags &= static_cast<u8>(~GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE);
    GameAnimSet_EvaluateState(obstacle->anim_set);
}

static i32 GizmoObstacle_ActivateRev(GIZMO *gizmo, i32 activate, i32 flags) {
    GIZOBSTACLE_s *obstacle = gizmo != NULL ? static_cast<GIZOBSTACLE_s *>(gizmo->object) : NULL;
    if (obstacle == NULL) {
        return 0;
    }

    const u8 initial_runtime_flags = obstacle->runtime_flags;
    if ((obstacle->control_flags & GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE) != 0 && obstacle->anim_set != NULL &&
        (obstacle->anim_set->state == GAMEANIMSET_STATE_ACTIVE_FORWARD ||
         obstacle->anim_set->state == GAMEANIMSET_STATE_AT_END)) {
        obstacle->progress_flags |= GIZOBSTACLE_PROGRESS_FLAG_ENABLED;
        obstacle->control_flags &= static_cast<u8>(~GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE);
    }
    if ((initial_runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE) != 0) {
        obstacle->progress_flags |= GIZOBSTACLE_PROGRESS_FLAG_ENABLED;
        obstacle->control_flags &= static_cast<u8>(~GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE);
    }

    if ((flags & GIZOBSTACLE_ACTIVATE_REVERSE_VALIDATE) == 0) {
        if (activate == 0) {
            obstacle->progress_flags |= GIZOBSTACLE_PROGRESS_FLAG_ENABLED;
            obstacle->control_flags &= static_cast<u8>(~GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE);
            return 1;
        }
        GizObstacle_PlayBackwards(obstacle);
        obstacle->control_flags |= GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE;
        obstacle->progress_flags &= static_cast<u8>(~GIZOBSTACLE_PROGRESS_FLAG_ENABLED);
        return 1;
    }

    const i32 reverse_active = (obstacle->control_flags & GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE) != 0 ? 1 : 0;
    if (activate == reverse_active) {
        if ((flags & GIZOBSTACLE_ACTIVATE_REVERSE_ALLOW_MATCHING_STATE) == 0) {
            return 0;
        }
        const i32 visible = (obstacle->progress_flags & GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) != 0 ? 1 : 0;
        if (visible != activate) {
            return 0;
        }
    }
    if (activate == 1 && (initial_runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE) != 0) {
        return 0;
    }
    return 1;
}

static void GizmoObstacle_SetVisibility(GIZMO *gizmo, i32 visibility) {
    GIZOBSTACLE_s *obstacle = gizmo != NULL ? static_cast<GIZOBSTACLE_s *>(gizmo->object) : NULL;
    if (obstacle == NULL) {
        return;
    }
    GameAnimSet_SetVisibility(obstacle->anim_set, visibility);
    obstacle->progress_flags = static_cast<u8>((obstacle->progress_flags & ~GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) |
                                               (visibility != 0 ? GIZOBSTACLE_PROGRESS_FLAG_VISIBLE : 0));
}

static NUVEC *GizmoObstacle_GetPos(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
        if (obstacle != NULL) {
            return &obstacle->position;
        }
    }
    return NULL;
}

static i32 GizObstacles_BoltHitPlat(void *, void *, BOLT *, unsigned char *) {
    UNIMPLEMENTED();
    return {};
}

static i32 *GizObstacles_GetBestBoltTarget(GIZMOSET *, float *, NUVEC *, NUVEC *, void *, NUVEC *, NUVEC *, float,
                                           float, i32, i32, i32) {
    UNIMPLEMENTED();
    return {};
}

static i32 GizObstacles_BoltHit(void *, void *, void *, NUVEC *, i32, float, NUVEC *, NUVEC *, BOLT *, u32,
                                unsigned char *) {
    UNIMPLEMENTED();
    return {};
}

static void *GizObstacles_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(GIZOBSTACLEPROGRESS_s));
}

static void GizObstacles_ClearProgress(void *, void *progress_ptr) {
    ClearObstacleProgress(static_cast<GIZOBSTACLEPROGRESS_s *>(progress_ptr));
}

static void GizObstacles_StoreProgress(void *, void *data, void *progress_ptr) {
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    GIZOBSTACLEPROGRESS_s *progress = static_cast<GIZOBSTACLEPROGRESS_s *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    ClearObstacleProgress(progress);
    if (obstacle_sys == NULL) {
        return;
    }

    i32 count = obstacle_sys->count;
    if (count > GIZOBSTACLE_PROGRESS_CAPACITY) {
        count = GIZOBSTACLE_PROGRESS_CAPACITY;
    }

    for (i32 index = 0; index < count; ++index) {
        const GIZOBSTACLE_s &obstacle = obstacle_sys->obstacles[index];
        const i32 word = index >> 5;
        const u32 bit = 1u << (index & 31);

        if ((obstacle.progress_flags & GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) == 0) {
            progress->progress_flag_1[word] &= ~bit;
        }
        if ((obstacle.progress_flags & GIZOBSTACLE_PROGRESS_FLAG_ENABLED) == 0) {
            progress->progress_flag_0[word] &= ~bit;
        }
        if ((obstacle.runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED) != 0) {
            progress->runtime_flag_0[word] |= bit;
        }
        if ((obstacle.runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE) != 0) {
            progress->runtime_flag_2[word] |= bit;
        }
        if ((obstacle.runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_BLOCKED) != 0) {
            progress->runtime_flag_3[word] |= bit;
        }
        if ((obstacle.runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED) != 0) {
            progress->runtime_flag_6[word] |= bit;
        }
        if ((obstacle.runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_DESTROYED) != 0) {
            progress->runtime_flag_7[word] |= bit;
        }
    }
}

static void GizObstacles_Reset(void *world_ptr, void *data, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    GIZOBSTACLEPROGRESS_s *progress = static_cast<GIZOBSTACLEPROGRESS_s *>(progress_ptr);
    GIZOBSTACLE_s *obstacle_entry = obstacle_sys->obstacles;
    for (i32 index = 0; index < obstacle_sys->count; ++index, ++obstacle_entry) {
        GIZOBSTACLE_s &obstacle = *obstacle_entry;
        obstacle.progress_flags = static_cast<u8>(
            (obstacle.progress_flags | GIZOBSTACLE_PROGRESS_FLAG_ENABLED | GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) &
            ~(GIZOBSTACLE_PROGRESS_FLAG_EXTERNAL_CONTROL | GIZOBSTACLE_PROGRESS_FLAG_PUSH_CONTROL));
        obstacle.runtime_flags &=
            static_cast<u8>(GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED | GIZOBSTACLE_RUNTIME_FLAG_PENDING_BLOWUP_TYPE);
        obstacle.animation_speed = 1.0f;

        GAMEANIMSET_s *anim_set = obstacle.anim_set;
        if (anim_set != NULL) {
            for (GAMEANIMOBJ_s *object = anim_set->objects; object != NULL; object = object->next) {
                i16 *object_data = static_cast<i16 *>(object->object_data);
                object_data[1] = -1;
                if (world->terrain != NULL && (object_data[0] & 1) == 0 && NuSpecialExistsFn(&object->special) != 0) {
                    object_data[1] = FindPlatInst(NuSpecialGetInstanceix(&object->special));
                }
                if (object_data[1] != -1) {
                    obstacle.runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_HAS_PLATFORM;
                }
                if ((object_data[0] & 2) != 0) {
                    obstacle.runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_ANIM_OBJECT_BIT_1;
                }
            }

            obstacle.room_id = world->current_gscn != NULL
                                   ? static_cast<i16>(NuPortalWhichRoom(world->current_gscn, &obstacle.position))
                                   : -1;
            if ((obstacle.config_flags & GIZOBSTACLE_CONFIG_USE_ANIM_AVERAGE_POSITION) != 0) {
                GameAnimSet_GetAveragePos(anim_set, &obstacle.secondary_position, 0, 1, 1);
            }
            GameAnimSet_EvaluateState(anim_set);
            GizObstacle_EvalAveragePosAndRadius(&obstacle, 2);
        }

        if (progress != NULL && index < GIZOBSTACLE_PROGRESS_CAPACITY) {
            const i32 word = index >> 5;
            const u32 bit = 1u << (index & 31);

            obstacle.progress_flags =
                static_cast<u8>((obstacle.progress_flags & ~GIZOBSTACLE_PROGRESS_FLAG_VISIBLE) |
                                ((progress->progress_flag_1[word] & bit) != 0 ? GIZOBSTACLE_PROGRESS_FLAG_VISIBLE : 0));
            obstacle.progress_flags =
                static_cast<u8>((obstacle.progress_flags & ~GIZOBSTACLE_PROGRESS_FLAG_ENABLED) |
                                ((progress->progress_flag_0[word] & bit) != 0 ? GIZOBSTACLE_PROGRESS_FLAG_ENABLED : 0));
            obstacle.runtime_flags = static_cast<u8>(
                (obstacle.runtime_flags & ~GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED) |
                ((progress->runtime_flag_0[word] & bit) != 0 ? GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED : 0));
            obstacle.runtime_flags =
                static_cast<u8>((obstacle.runtime_flags & ~GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE) |
                                ((progress->runtime_flag_2[word] & bit) != 0 ? GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE : 0));
            obstacle.runtime_flags =
                static_cast<u8>((obstacle.runtime_flags & ~GIZOBSTACLE_RUNTIME_FLAG_BLOCKED) |
                                ((progress->runtime_flag_3[word] & bit) != 0 ? GIZOBSTACLE_RUNTIME_FLAG_BLOCKED : 0));
            obstacle.runtime_flags = static_cast<u8>(
                (obstacle.runtime_flags & ~GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED) |
                ((progress->runtime_flag_6[word] & bit) != 0 ? GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED : 0));
            obstacle.runtime_flags =
                static_cast<u8>((obstacle.runtime_flags & ~GIZOBSTACLE_RUNTIME_FLAG_DESTROYED) |
                                ((progress->runtime_flag_7[word] & bit) != 0 ? GIZOBSTACLE_RUNTIME_FLAG_DESTROYED : 0));
        }
    }
    ngizobstacletriggers = 0;
}

static void *GizObstacles_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, sizeof(GIZOBSTACLESYS_s)));

    obstacle_sys->capacity = world->current_level->max_obstacles;
    obstacle_sys->obstacles = static_cast<GIZOBSTACLE_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, obstacle_sys->capacity * sizeof(GIZOBSTACLE_s)));
    obstacle_sys->active_gizmos = static_cast<GIZMO **>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, obstacle_sys->capacity * sizeof(GIZMO *)));
    obstacle_sys->anim_pool = GameAnimSet_CreateObjectPool(&world->giz_buffer, &world->unknown_0108, 4,
                                                           world->current_level->max_obstacle_objs);

    for (i32 i = 0; i < obstacle_sys->capacity; ++i) {
        obstacle_sys->obstacles[i].anim_set =
            GameAnimSet_Create(&world->giz_buffer, &world->unknown_0108, obstacle_sys->anim_pool, world->game_anim_sys);
    }
    world->giz_obstacle_sys = obstacle_sys;
    return obstacle_sys;
}

static i32 GizObstacles_Load(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    const u8 version = static_cast<u8>(EdFileReadChar());
    obstacle_sys->count = static_cast<u16>(EdFileReadShort());

    for (i32 index = 0; index < obstacle_sys->count; ++index) {
        GIZOBSTACLE_s &obstacle = obstacle_sys->obstacles[index];
        EdFileRead(obstacle.name, sizeof(obstacle.name));
        EdFileReadNuVec(&obstacle.position);

        if (version <= 1) {
            obstacle.secondary_position = obstacle.position;
        } else {
            EdFileReadNuVec(&obstacle.secondary_position);
        }

        obstacle.auto_return_delay = EdFileReadFloat();
        obstacle.trigger_radius = EdFileReadFloat();
        if (version != 2) {
            EdFileReadNuVec(&obstacle.trigger_box_half_extents);
            obstacle.trigger_box_yaw = EdFileReadShort();
            obstacle.config_flags = static_cast<u32>(EdFileReadInt());
            if (version > 11) {
                obstacle.field_0x6c = static_cast<u32>(EdFileReadInt());
            }
        }

        if (version == 6) {
            EdFileReadShort();
            EdFileReadChar();
        }
        obstacle.state = static_cast<u8>(EdFileReadChar());
        obstacle.mode = static_cast<u8>(EdFileReadChar());
        if (version <= 6) {
            obstacle.trigger_mode = 0xff;
        } else {
            obstacle.trigger_mode = static_cast<u8>(EdFileReadChar());
        }

        GizmoFileReadGameAnimSet(obstacle.anim_set, world, Gizobstacle_ReadAnimSetData, version, const_cast<char *>(""),
                                 obstacle.name);

        if (version <= 3) {
            obstacle.field_0x4c = 1.0f;
            obstacle.field_0x50 = 1.0f;
        } else {
            obstacle.field_0x4c = EdFileReadFloat();
            if (version <= 4) {
                obstacle.field_0x50 = obstacle.field_0x4c;
            } else {
                obstacle.field_0x50 = EdFileReadFloat();
            }
        }

        obstacle.blowup_type = -1;
        if (version > 7) {
            obstacle.reflection_alpha = EdFileReadFloat();

            if (version == 9) {
                obstacle.blowup_type = EdFileReadShort();
                obstacle.pickup_count = EdFileReadShort();
                obstacle.pickup_direction_x = EdFileReadShort();
                obstacle.pickup_direction_y = EdFileReadShort();
                EdFileReadNuVec(&obstacle.pickup_offset);
            } else if (version > 9) {
                char blowup_name[32];
                const i32 name_length = static_cast<i8>(EdFileReadChar());
                if (name_length != 0) {
                    EdFileRead(blowup_name, name_length);
                    obstacle.blowup_type = static_cast<i16>(GizmoBlowupGetNameTableId(blowup_name));
                    if (obstacle.blowup_type != -1) {
                        obstacle.runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_PENDING_BLOWUP_TYPE;
                    }
                }

                obstacle.pickup_count = EdFileReadShort();
                obstacle.pickup_direction_x = EdFileReadShort();
                obstacle.pickup_direction_y = EdFileReadShort();
                EdFileReadNuVec(&obstacle.pickup_offset);
            }
        }

        if (version <= 9 || version == 10) {
            obstacle.pickup_scatter_height = world->area != NULL && (world->area->flags & 1) != 0 ? -105.0f : -999.0f;
            obstacle.start_sfx_id = -1;
            obstacle.stop_sfx_id = -1;
        } else {
            obstacle.pickup_scatter_height = EdFileReadFloat();
            obstacle.start_sfx_id = -1;
            obstacle.stop_sfx_id = -1;
            if (version > 12) {
                char sfx_name[32];
                if (GizmoFileReadName(sfx_name) != 0) {
                    obstacle.start_sfx_id = static_cast<i16>(GetSfxId(sfx_name));
                }
                if (version > 13 && GizmoFileReadName(sfx_name) != 0) {
                    obstacle.stop_sfx_id = static_cast<i16>(GetSfxId(sfx_name));
                }
            }
        }

        if (GizObstacle_SetDefaultSFXFn != NULL) {
            GizObstacle_SetDefaultSFXFn(world, &obstacle);
        }
    }

    return 1;
}

static void Gizobstacle_ReadAnimSetData(GAMEANIMOBJ_s *object, unsigned char version) {
    if (object == NULL) {
        return;
    }

    u16 fallback_data = 0;
    u16 *object_data = static_cast<u16 *>(object->object_data);
    if (object_data == NULL) {
        object_data = &fallback_data;
    }

    if (version > 7) {
        object_data[0] = static_cast<u16>(EdFileReadShort());
    } else if ((object->flags & 2) != 0) {
        object_data[0] |= 1;
    }
}

static void GizObstacles_PostLoad(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    if (obstacle_sys == NULL) {
        return;
    }

    for (i32 index = 0; index < obstacle_sys->count; ++index) {
        GIZOBSTACLE_s &obstacle = obstacle_sys->obstacles[index];
        if ((obstacle.runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_PENDING_BLOWUP_TYPE) != 0) {
            obstacle.blowup_type = static_cast<i16>(GizmoBlowupGetTypeFromNameTableId(world, obstacle.blowup_type));
            obstacle.runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_PENDING_BLOWUP_TYPE);
        }
    }
}

static void GizObstacles_AddLevelSfx(void *, void *data, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx) {
    GIZOBSTACLESYS_s *obstacle_sys = static_cast<GIZOBSTACLESYS_s *>(data);
    if (obstacle_sys == NULL) {
        return;
    }
    GIZOBSTACLE_s *obstacle = obstacle_sys->obstacles;
    if (obstacle_sys->count == 0) {
        return;
    }
    i32 index = 0;
    do {
        if (obstacle->start_sfx_id != -1) {
            AddLevelSfxFromId(obstacle->start_sfx_id, sfx_ids, sfx_count, max_sfx);
        }
        if (obstacle->stop_sfx_id != -1) {
            AddLevelSfxFromId(obstacle->stop_sfx_id, sfx_ids, sfx_count, max_sfx);
        }
        ++index;
        ++obstacle;
    } while (obstacle_sys->count > index);
}

ADDGIZMOTYPE *GizObstacles_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizObstacle";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0x70;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizObstacles_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizmoObstacle_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizObstacles_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = GizObstacles_BoltHitPlat;
    addtype.fns.get_best_bolt_target_fn = GizObstacles_GetBestBoltTarget;
    addtype.fns.late_update_fn = GizObstacles_Update;
    addtype.fns.bolt_hit_fn = GizObstacles_BoltHit;
    addtype.fns.draw_fn = GizObstacles_Draw;
    addtype.fns.get_gizmo_name_fn = GizmoObstacle_GetGizmoName;
    addtype.fns.get_output_fn = GizmoObstacle_GetOutput;
    addtype.fns.get_output_name_fn = GizmoObstacle_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizmoObstacle_GetNumOutputs;
    addtype.fns.activate_fn = GizmoObstacle_Activate;
    addtype.fns.activate_rev_fn = GizmoObstacle_ActivateRev;
    addtype.fns.set_visibility_fn = GizmoObstacle_SetVisibility;
    addtype.fns.allocate_progress_data_fn = GizObstacles_AllocateProgressData;
    addtype.fns.clear_progress_fn = GizObstacles_ClearProgress;
    addtype.fns.store_progress_fn = GizObstacles_StoreProgress;
    addtype.fns.reset_fn = GizObstacles_Reset;
    addtype.fns.reserve_buffer_space_fn = GizObstacles_ReserveBufferSpace;
    addtype.fns.load_fn = GizObstacles_Load;
    addtype.fns.post_load_fn = GizObstacles_PostLoad;
    addtype.fns.add_level_sfx_fn = GizObstacles_AddLevelSfx;
    obstacle_gizmotype_id = type_id;

    return &addtype;
}

namespace {

    struct OBSTACLE_CHARACTER_DATA_FLAGS_s {
        u32 flags_0x00;
        u8 flags_0x04;
        u8 flags_0x05;
    };

    static bool ObstacleUsesBoxTrigger(const GIZOBSTACLE_s *obstacle) {
        return obstacle->mode == 5 || obstacle->mode == 6;
    }

    static bool ObstacleCharacterDataHasFlag(const GameObject_s *object, u8 offset, u8 flag) {
        const OBSTACLE_CHARACTER_DATA_FLAGS_s *data = static_cast<const OBSTACLE_CHARACTER_DATA_FLAGS_s *>(
            static_cast<const void *>(object->apiobj.character_data));
        if (data == NULL) {
            return false;
        }
        return offset == 4 ? (data->flags_0x04 & flag) != 0 : (data->flags_0x05 & flag) != 0;
    }

} // namespace

static void GizObstacleUpdate_Proximity(GIZOBSTACLE_s *obstacle);
static i32 GizObstacle_SatisfyingTerrainChecks(GIZOBSTACLE_s *obstacle, GameObject_s *object);

static i32 GizObstacle_PosWithinBox(GIZOBSTACLE_s *obstacle, NUVEC *position) {
    if (obstacle == NULL || position == NULL) {
        return 0;
    }

    NUVEC local_position;
    NuVecSub(&local_position, position, &obstacle->secondary_position);
    NuVecRotateY(&local_position, &local_position, -obstacle->trigger_box_yaw);
    return local_position.x >= -obstacle->trigger_box_half_extents.x &&
           local_position.x <= obstacle->trigger_box_half_extents.x && local_position.y >= 0.0f &&
           local_position.y <= obstacle->trigger_box_half_extents.y &&
           local_position.z >= -obstacle->trigger_box_half_extents.z &&
           local_position.z <= obstacle->trigger_box_half_extents.z;
}

void GizObstacle_Stop(GIZOBSTACLE_s *obstacle) {
    if (obstacle != NULL) {
        GameAnimSet_Stop(obstacle->anim_set);
    }
}

i32 GizObstacles_Hit(void *context, GIZOBSTACLE_s *obstacle, nuvec_s *, i32 player, i32) {
    if ((obstacle->progress_flags & 2) == 0 || (obstacle->progress_flags & 1) == 0 ||
        (obstacle->runtime_flags & 0x80) != 0)
        return 0;

    if ((obstacle->config_flags & 0x800) != 0) {
        obstacle->runtime_flags |= 0x40;
    } else {
        if ((obstacle->config_flags & 0x1000) == 0)
            return 0;
        if (obstacle->blowup_type != -1) {
            if ((obstacle->config_flags & 0x200) != 0) {
                if (obstacle->anim_set != NULL) {
                    for (GAMEANIMOBJ_s *object = obstacle->anim_set->objects; object != NULL; object = object->next) {
                        NUVEC *position = NuSpecialGetDrawPos(&object->special);
                        if (position != NULL) {
                            GizmoBlowUpTypeBlowUp(static_cast<WORLDINFO_s *>(context), obstacle->blowup_type, position);
                        }
                    }
                }
            } else {
                GizmoBlowUpTypeBlowUp(static_cast<WORLDINFO_s *>(context), obstacle->blowup_type,
                                      &obstacle->evaluated_position);
            }
        }
        GameAnimSet_JumpToEnd(obstacle->anim_set);
        GizObstacle_EvalAveragePosAndRadius(obstacle, 2);
        GameAnimSet_SetVisibility(obstacle->anim_set, 0);
        obstacle->runtime_flags |= 0x80;
    }
    if (player != -1 && static_cast<i8>(Player[player]->apiobj.field_0x1f8) < 0) {
        NewBuzz(Player[player]->pad_gamepad->pad, 0.1f, 0);
    }
    return 1;
}

void GizObstacle_JumpToEnd(GIZOBSTACLE_s *obstacle) {
    if (obstacle != NULL && obstacle->anim_set != NULL) {
        GameAnimSet_JumpToEnd(obstacle->anim_set);
    }
}

GIZOBSTACLE_s *GizObstacle_FindByName(GIZOBSTACLESYS_s *system, char *name) {
    if (system == NULL || name == NULL) {
        return NULL;
    }

    for (i32 index = 0; index < system->count; ++index) {
        GIZOBSTACLE_s *obstacle = &system->obstacles[index];
        if (NuStrICmp(obstacle->name, name) == 0) {
            return obstacle;
        }
    }

    return NULL;
}

void GizObstacle_FindNearest(GIZOBSTACLESYS_s *, nuvec_s *, GameObject_s *, float *, i32) {
}

void GizObstacle_JumpToStart(GIZOBSTACLE_s *obstacle) {
    if (obstacle != NULL && obstacle->anim_set != NULL) {
        GameAnimSet_JumpToStart(obstacle->anim_set);
    }
}

void GizObstacles_AddTrigger(nuvec_s *position) {
    if (position == NULL) {
        return;
    }
    if (ngizobstacletriggers >= 16) {
        AddGameMessage(const_cast<char *>("MAXGIZOBSTACLETRIGGERS needs increasing, ask a programmer!"), &v001, 0.4f,
                       NULL, 0.4f, 200, 200, 200, 0x2080, 0.0f);
        return;
    }
    gizobstacletriggers[ngizobstacletriggers++] = position;
}

void GizObstacle_PlayForwards(GIZOBSTACLE_s *obstacle) {
    if (obstacle != NULL) {
        GameAnimSet_SetRepeating(obstacle->anim_set, obstacle->state == 2);
        if (obstacle->animation_speed < 0.0f) {
            GameAnimSet_Play(obstacle->anim_set, obstacle->animation_speed * obstacle->field_0x50, 0);
        } else {
            GameAnimSet_Play(obstacle->anim_set, obstacle->animation_speed * obstacle->field_0x4c, 0);
        }
    }
}

void GizObstacle_PlayBackwards(GIZOBSTACLE_s *obstacle) {
    if (obstacle != NULL) {
        GameAnimSet_SetRepeating(obstacle->anim_set, obstacle->state == 2);
        if (obstacle->animation_speed < 0.0f) {
            GameAnimSet_Play(obstacle->anim_set, -obstacle->animation_speed * obstacle->field_0x4c, 0);
        } else {
            GameAnimSet_Play(obstacle->anim_set, -obstacle->animation_speed * obstacle->field_0x50, 0);
        }
    }
}

void GizObstacle_SetPushControlled(GIZOBSTACLE_s *, GameObject_s *, float) {
}

void GizObstacle_SetDefaultSFXFn_LSW(void *, GIZOBSTACLE_s *) {
}

void GizObstacle_SetTechnoControlled(GIZOBSTACLE_s *obstacle, float speed) {
    obstacle->progress_external_control = 1;
    obstacle->animation_speed = speed;
}

i32 GizObstacle_CheckExcludeFlagsFn_LSW(GIZOBSTACLE_s *obstacle, GameObject_s *object) {
    if ((obstacle->field_0x6c & 1) != 0 &&
        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field275_0x116 != 10) {
        return 1;
    }
    if ((obstacle->field_0x6c & 2) != 0 && object->apiobj.character_data->move_fn != Move_BEAST) {
        return 1;
    }
    return 0;
}

void GizObstacle_EvalAveragePosAndRadius(GIZOBSTACLE_s *obstacle, i32 state) {
    obstacle->field_0x58 = 1.0f;
    obstacle->evaluated_position = obstacle->position;
    GameAnimSet_GetCentreAndRadius(obstacle->anim_set, &obstacle->evaluated_position, &obstacle->field_0x58, state, 1,
                                   1);
}

// Obstacle modes dispatch through this exact eight-entry target table.

static void GizObstacleUpdate_PushOnly(GIZOBSTACLE_s *) {
}

static void GizObstacleUpdate_AutoStart(GIZOBSTACLE_s *obstacle) {
    if ((obstacle->config_flags & GIZOBSTACLE_CONFIG_ALWAYS_RUN_PROXIMITY) != 0) {
        obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_BLOCKED;
        GizObstacleUpdate_Proximity(obstacle);
        return;
    }
    if (obstacle->anim_set->state == GAMEANIMSET_STATE_AT_START ||
        obstacle->anim_set->state == GAMEANIMSET_STATE_ACTIVE_BACKWARD) {
        GizObstacle_PlayForwards(obstacle);
    }
}

static void GizObstacleUpdate_NoTrigger(GIZOBSTACLE_s *obstacle) {
    if ((obstacle->config_flags & GIZOBSTACLE_CONFIG_ALWAYS_RUN_PROXIMITY) == 0 &&
        (obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE) == 0 &&
        obstacle->anim_set->state == GAMEANIMSET_STATE_AT_START) {
        return;
    }
    obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_BLOCKED;
    GizObstacleUpdate_Proximity(obstacle);
}

static void GizObstacleUpdate_Proximity(GIZOBSTACLE_s *obstacle) {
    obstacle->proximity_output = 0;

    if ((obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_BLOCKED) == 0) {
        const f32 radius_squared = obstacle->trigger_radius * obstacle->trigger_radius;
        f32 closest_distance_squared = radius_squared;
        NUVEC *closest_position = NULL;
        GameObject_s *closest_object = NULL;
        u32 players_not_satisfying_terrain = 0;

        const bool camera_mode = obstacle->mode == 1 || obstacle->mode == 5;
        if (camera_mode && GameCam != NULL && GameCam->mode != -1 && obstacle->trigger_mode == 0xff &&
            (obstacle->config_flags &
             (GIZOBSTACLE_CONFIG_INVERT_PROXIMITY | GIZOBSTACLE_CONFIG_EXCLUDE_NON_PLAYER |
              GIZOBSTACLE_CONFIG_CHECK_SUPPORTING_PLATFORM | GIZOBSTACLE_CONFIG_REQUIRE_ACTIVE_PLAYER |
              GIZOBSTACLE_CONFIG_REQUIRE_LINKED_OBJECT | GIZOBSTACLE_CONFIG_REQUIRE_CHARACTER_DATA_FLAG_04)) == 0) {
            const f32 distance_squared = NuVecDistSqr(&GameCam->pos, &obstacle->secondary_position, NULL);
            if (distance_squared < closest_distance_squared &&
                (!ObstacleUsesBoxTrigger(obstacle) || GizObstacle_PosWithinBox(obstacle, &GameCam->pos) != 0)) {
                closest_distance_squared = distance_squared;
                closest_position = &GameCam->pos;
            }
        }

        const bool accepts_external_triggers =
            obstacle->mode == 1 || obstacle->mode == 2 || obstacle->mode == 5 || obstacle->mode == 6;
        if (accepts_external_triggers && (obstacle->config_flags & GIZOBSTACLE_CONFIG_INVERT_PROXIMITY) == 0) {
            for (i32 index = 0; index < ngizobstacletriggers; ++index) {
                NUVEC *trigger = gizobstacletriggers[index];
                const f32 distance_squared = NuVecDistSqr(&obstacle->secondary_position, trigger, NULL);
                if (distance_squared < closest_distance_squared &&
                    (!ObstacleUsesBoxTrigger(obstacle) || GizObstacle_PosWithinBox(obstacle, trigger) != 0)) {
                    closest_distance_squared = distance_squared;
                    closest_position = trigger;
                }
            }
        }

        for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
            GameObject_s *object = &Obj[index];
            if ((object->apiobj.field_0x1f8 & (APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER)) !=
                    (APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER) ||
                object->apiobj.field_0x287 != 0 ||
                (LEGOCONTEXT_DOOMED != -1 && LEGOCONTEXT_DOOMED == static_cast<i8>(object->field_0x7a5)) ||
                (object->field_0x1050 & 1) == 0 || (object->field_0xe20 & 0x20) != 0) {
                continue;
            }

            const i8 player_index = object->apiobj.field_0x27c;
            if (player_index == -1) {
                if ((obstacle->config_flags & GIZOBSTACLE_CONFIG_EXCLUDE_NON_PLAYER) != 0) {
                    continue;
                }
            } else {
                players_not_satisfying_terrain |= 1u << (static_cast<u8>(player_index) & 31);
            }

            if (((obstacle->config_flags & GIZOBSTACLE_CONFIG_REQUIRE_ACTIVE_PLAYER) != 0 &&
                 (object->apiobj.field_0x1f8 & APIOBJECT_FLAG_PLAYER_ACTIVE) == 0) ||
                (CInfo[object->character_context].flags & 0x8000) != 0 ||
                ((obstacle->config_flags & GIZOBSTACLE_CONFIG_REQUIRE_LINKED_OBJECT) != 0 &&
                 object->field_0xcc0 == NULL) ||
                ((obstacle->config_flags & GIZOBSTACLE_CONFIG_REQUIRE_CHARACTER_DATA_FLAG_04) != 0 &&
                 !ObstacleCharacterDataHasFlag(object, 4, 0x04)) ||
                ((obstacle->config_flags & GIZOBSTACLE_CONFIG_REQUIRE_CHARACTER_DATA_FLAG_20) != 0 &&
                 !ObstacleCharacterDataHasFlag(object, 5, 0x20)) ||
                (GizObstacle_CheckExcludeFlagsFn != NULL && GizObstacle_CheckExcludeFlagsFn(obstacle, object) != 0)) {
                continue;
            }

            const bool requires_obstacle_platform =
                (obstacle->mode & 0xfb) == 2 && object->apiobj.field_0x27d == 0 &&
                (object->field_0xf03 & GAMEOBJECT_F03_FLAG_OBSTACLE_TERRAIN_VALID) == 0 && object->field_0xe31 != 1;
            if (requires_obstacle_platform) {
                bool on_obstacle_platform = false;
                if (obstacle->anim_set != NULL) {
                    for (GAMEANIMOBJ_s *anim_object = obstacle->anim_set->objects; anim_object != NULL;
                         anim_object = anim_object->next) {
                        i16 *object_data = static_cast<i16 *>(anim_object->object_data);
                        if (object_data != NULL && object_data[1] != -1 &&
                            object_data[1] == object->apiobj.supporting_platform_id) {
                            on_obstacle_platform = true;
                            break;
                        }
                    }
                }
                if (!on_obstacle_platform) {
                    continue;
                }
            }

            const f32 distance_squared =
                NuVecDistSqr(&obstacle->secondary_position, &object->apiobj.lower_position, NULL);
            if (distance_squared < closest_distance_squared) {
                if ((!ObstacleUsesBoxTrigger(obstacle) ||
                     GizObstacle_PosWithinBox(obstacle, &object->apiobj.collision_position) != 0) &&
                    GizObstacle_SatisfyingTerrainChecks(obstacle, object) != 0) {
                    if (player_index != -1) {
                        players_not_satisfying_terrain &= ~(1u << (static_cast<u8>(player_index) & 31));
                    }
                    closest_distance_squared = distance_squared;
                    closest_position = &object->apiobj.lower_position;
                    closest_object = object;
                }
            } else if ((obstacle->config_flags & GIZOBSTACLE_CONFIG_REQUIRE_ALL_PLAYERS) != 0 && player_index != -1 &&
                       distance_squared < radius_squared &&
                       (!ObstacleUsesBoxTrigger(obstacle) ||
                        GizObstacle_PosWithinBox(obstacle, &object->apiobj.collision_position) != 0) &&
                       GizObstacle_SatisfyingTerrainChecks(obstacle, object) != 0) {
                players_not_satisfying_terrain &= ~(1u << (static_cast<u8>(player_index) & 31));
            }
        }

        const bool invert = (obstacle->config_flags & GIZOBSTACLE_CONFIG_INVERT_PROXIMITY) != 0;
        const bool all_players_satisfied = (obstacle->config_flags & GIZOBSTACLE_CONFIG_REQUIRE_ALL_PLAYERS) == 0 ||
                                           players_not_satisfying_terrain == 0;
        if ((invert && closest_position == NULL) || (!invert && closest_position != NULL && all_players_satisfied)) {
            obstacle->proximity_output = 1;
            obstacle->triggering_object = closest_object;
        }
    }

    const GAMEANIMSET_STATE state = obstacle->anim_set->state;
    if (state == GAMEANIMSET_STATE_AT_START) {
        if (obstacle->proximity_output != 0 ||
            (obstacle->runtime_flags &
             (GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE | GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED)) != 0) {
            GizObstacle_PlayForwards(obstacle);
            obstacle->auto_return_timer = obstacle->auto_return_delay;
        }
        return;
    }

    bool may_return = state == GAMEANIMSET_STATE_AT_END && obstacle->state != 0;
    if (!may_return) {
        may_return = WORLD != NULL && WORLD->current_level == BLOCKADERUNNERC_LDATA && LevGizObst[0] == obstacle &&
                     state == GAMEANIMSET_STATE_BETWEEN_ENDPOINTS;
    }
    if (!may_return || obstacle->proximity_output != 0 ||
        (obstacle->runtime_flags & (GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE | GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED)) !=
            0) {
        return;
    }

    if (obstacle->auto_return_timer <= 0.0f) {
        GizObstacle_PlayBackwards(obstacle);
    } else if (state == GAMEANIMSET_STATE_AT_END) {
        obstacle->auto_return_timer -= FRAMETIME;
    }
}

static void GizObstacleUpdate_TechnoOnly(GIZOBSTACLE_s *) {
}

static i32 GizObstacle_SatisfyingTerrainChecks(GIZOBSTACLE_s *obstacle, GameObject_s *object) {
    if (static_cast<i8>(obstacle->trigger_mode) < 0) {
        if ((obstacle->config_flags & GIZOBSTACLE_CONFIG_CHECK_SUPPORTING_PLATFORM) == 0) {
            return 1;
        }
        if (object->apiobj.field_0x27d == 0 &&
            (object->field_0xf03 & GAMEOBJECT_F03_FLAG_OBSTACLE_TERRAIN_VALID) == 0) {
            return 0;
        }
        if (object->apiobj.supporting_platform_id >= 0) {
            if (obstacle->anim_set == NULL || (obstacle->runtime_flags & GIZOBSTACLE_RUNTIME_FLAG_HAS_PLATFORM) == 0) {
                return 0;
            }
            for (GAMEANIMOBJ_s *anim_object = obstacle->anim_set->objects; anim_object != NULL;
                 anim_object = anim_object->next) {
                i16 *object_data = static_cast<i16 *>(anim_object->object_data);
                if (object_data != NULL && object_data[1] == object->apiobj.supporting_platform_id) {
                    return 1;
                }
            }
        }
        return 0;
    }

    if ((object->apiobj.field_0x27d == 0 && (object->field_0xf03 & GAMEOBJECT_F03_FLAG_OBSTACLE_TERRAIN_VALID) == 0) ||
        obstacle->trigger_mode != object->apiobj.field_0x281) {
        return 0;
    }
    return 1;
}

GIZOBSTACLEUPDATEFN gizobstacleupdatefns[8] = {
    GizObstacleUpdate_AutoStart,  GizObstacleUpdate_Proximity, GizObstacleUpdate_Proximity, GizObstacleUpdate_NoTrigger,
    GizObstacleUpdate_TechnoOnly, GizObstacleUpdate_Proximity, GizObstacleUpdate_Proximity, GizObstacleUpdate_PushOnly,
};
