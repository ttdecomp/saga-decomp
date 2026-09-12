#include "legoapi/legoapi_types.h"
#include "legoapi/audio/audio.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "globals.h"

i32 GameAnimSet_IsAnimationReset(GAMEANIMSET_s *set);
void GizSpinner_GetSpinnerPos(GIZSPINNER_s *spinner, nuvec_s *position);
void NewBuzzFrames(nupad_s *pad, i32 frames, i32 mode);

i16 GizSpinnerGDeb_Fail[3] = {0x57, 1, 0x58};

GIZSPINNER_s *GizSpinner_Find(WORLDINFO_s *world, nuvec_s *position, i32 alternate) {
    GIZSPINNER_s *nearest = NULL;
    if (world->current_level->max_spinners != 0) {
        f32 nearest_distance = 1000000000.0f;
        if (alternate == 0) {
            i32 index = 0;
            do {
                GIZSPINNER_s *spinner = &world->spinners[index];
                if ((spinner->flags & 0x27) == GIZSPINNER_FLAG_VALID && (spinner->state_flags & 8) == 0) {
                    NUVEC spinner_position;
                    GizSpinner_GetSpinnerPos(spinner, &spinner_position);
                    f32 distance = NuVecDistSqr(position, &spinner_position, NULL);
                    if (distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = spinner;
                    }
                }
                ++index;
            } while (index < world->current_level->max_spinners);
        } else {
            i32 index = 0;
            do {
                GIZSPINNER_s *spinner = &world->spinners[index];
                if ((spinner->flags & 0x27) == GIZSPINNER_FLAG_VALID && (spinner->state_flags & 8) != 0) {
                    NUVEC spinner_position;
                    GizSpinner_GetSpinnerPos(spinner, &spinner_position);
                    f32 distance = NuVecDistSqr(position, &spinner_position, NULL);
                    if (distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = spinner;
                    }
                }
                ++index;
            } while (index < world->current_level->max_spinners);
        }
    }
    return nearest;
}

i32 GizSpinner_Push(GIZSPINNER_s *spinner, i32 context) {
    f32 frame_time = FRAMETIME;
    spinner->state_flags &= ~0x300u;

    i32 result = 0;
    if ((spinner->state_flags & 0x20) != 0) {
        spinner->state_flags &= ~0x20u;
        goto finalize;
    }

    {
        i32 rotation_step = static_cast<i32>(frame_time * 5461.0f);
        i32 at_start = GameAnimSet_IsAnimationReset(spinner->anim_set);
        bool at_end = spinner->anim_set->state == GAMEANIMSET_STATE_AT_END;
        u32 flags = spinner->state_flags;
        i32 clear_context = 0;

        if (context != 0x1e) {
            if ((flags & 4) != 0) {
                if (!at_end || spinner->anim_set->object_count == 0) {
                    goto rotate;
                }
                goto stop_at_endpoint;
            }
            if (at_start != 0) {
                if (spinner->anim_set->object_count == 0) {
                    goto rotate;
                }
                goto stop_at_endpoint;
            }
            if ((flags & 0x400) == 0 || !at_end) {
                goto rotate;
            }
            clear_context = 1;
            goto stop_at_endpoint;
        } else {
            if ((flags & 4) != 0) {
                if (at_start != 0) {
                    if (spinner->anim_set->object_count == 0) {
                        goto rotate;
                    }
                    goto stop_at_endpoint;
                }
                if ((flags & 0x400) == 0 || !at_end) {
                    goto rotate;
                }
                clear_context = 1;
                goto stop_at_endpoint;
            }
            if (!at_end || spinner->anim_set->object_count == 0) {
                goto rotate;
            }
            goto stop_at_endpoint;
        }

    stop_at_endpoint:
        flags &= ~0x40u;
        if ((flags & 2) == 0) {
            flags |= 0x300;
        }
        spinner->state_flags = flags;
        if (clear_context != 0) {
            context = -1;
        }
        goto finalize;

    rotate:
        if (context == 0x1e) {
            if ((flags & 2) != 0) {
                spinner->rotation += rotation_step;
            } else {
                spinner->rotation -= rotation_step;
            }
        } else if ((flags & 2) != 0) {
            spinner->rotation -= rotation_step;
        } else {
            spinner->rotation += rotation_step;
        }
        spinner->state_flags = flags | 0x40;
        result = 1;
    }

finalize:
    if (RotDiff(spinner->rotation, spinner->previous_rotation) < 0) {
        spinner->state_flags |= 1;
    } else {
        spinner->state_flags &= ~1u;
    }
    spinner->room_index = static_cast<i8>(context);
    spinner->target_rotation = spinner->rotation;
    spinner->previous_rotation = spinner->rotation;
    spinner->field_0x090 = 0.0f;
    return result;
}

i32 GizSpinner_Spin(GIZSPINNER_s *spinner, i32 context) {
    if ((spinner->state_flags & 0x4000) != 0 && context != 0x1e) {
        return 2;
    }
    if ((spinner->state_flags & 0x8000) != 0 && context != 0x1f) {
        return 2;
    }

    if (spinner->rotation != spinner->target_rotation || spinner->field_0x090 > 0.0f || spinner->anim_set == NULL ||
        spinner->room_index != -1) {
        return 0;
    }
    if (GameAnimSet_GetVisibility(spinner->anim_set) != GAMEANIMSET_VISIBILITY_ALL) {
        return 1;
    }

    f32 speed;
    u32 flags = spinner->state_flags;
    if ((context == 0x1e && (flags & 4) == 0) || (context != 0x1e && (flags & 4) != 0)) {
        if (spinner->anim_set->state == GAMEANIMSET_STATE_AT_END) {
            return 2;
        }
        speed = spinner->animation_speed;
        if ((context == 0x1e && (flags & 2) != 0) || (context != 0x1e && (flags & 2) == 0)) {
            flags |= 1;
        } else {
            flags &= ~1u;
        }
    } else {
        if (GameAnimSet_IsAnimationReset(spinner->anim_set) != 0) {
            return 2;
        }
        speed = -spinner->animation_speed;
        if ((context == 0x1f && (flags & 2) == 0) || (context != 0x1f && (flags & 2) != 0)) {
            flags |= 1;
        } else {
            flags &= ~1u;
        }
    }
    spinner->state_flags = flags;

    u16 step = static_cast<u16>(0x10000 / spinner->type);
    if ((spinner->state_flags & 1) != 0) {
        spinner->target_rotation = spinner->rotation + step;
    } else {
        spinner->target_rotation = spinner->rotation - step;
    }
    GameAnimSet_Play(spinner->anim_set, speed, 1);
    spinner->previous_rotation = spinner->rotation;
    spinner->field_0x090 = 0.25f;
    return 3;
}

int GizSpinner_Update(GIZSPINNER_s *) {
    return 0;
}

i32 GizSpinner_GetState(GIZSPINNER_s *spinner) {
    i32 state = 0;
    if (GameAnimSet_IsAnimationReset(spinner->anim_set) == 0) {
        state = (spinner->anim_set->state == GAMEANIMSET_STATE_AT_END) + 1;
    }
    return state;
}

void GizSpinner_PushFail(GameObject_s *object, GIZSPINNER_s *) {
    NewBuzzFrames(object->pad_gamepad->pad, 1, 0);

    NUVEC position;
    position.x = static_cast<GIZSPINNER_s *>(object->field_0x788)->position.x;
    f32 y = static_cast<GIZSPINNER_s *>(object->field_0x788)->position.y;
    position.y = QRAND_FLOAT() * 0.25f + 0.15f + y;
    position.z = static_cast<GIZSPINNER_s *>(object->field_0x788)->position.z;
    AddGameDebris(WORLD->debris_sys, GizSpinnerGDeb_Fail[1], &position);
    AddGameDebris(WORLD->debris_sys, GizSpinnerGDeb_Fail[2], &position);
    GameAudio_PlaySfx(0x39, &position, 0, 0);
}

GIZSPINNER_s *GizSpinner_FindNearest(nuvec_s *position, WORLDINFO_s *world, f32 *distance_out) {
    GIZSPINNER_s *nearest = NULL;
    if (world->spinners != NULL) {
        f32 nearest_distance = 1000000000.0f;
        for (i32 index = 0; index < world->current_level->max_spinners; ++index) {
            GIZSPINNER_s *spinner = &world->spinners[index];
            if ((spinner->flags & GIZSPINNER_FLAG_VALID) == 0 || (spinner->flags & 0x26) != 0) {
                continue;
            }

            NUVEC spinner_position;
            GizSpinner_GetSpinnerPos(spinner, &spinner_position);
            f32 distance = NuVecDistSqr(position, &spinner_position, NULL);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = spinner;
            }
        }
        if (distance_out != NULL) {
            *distance_out = nearest_distance;
        }
    }
    return nearest;
}

void GizSpinners_InitTerrain(WORLDINFO_s *) {
}

void GizSpinner_GetSpinnerPos(GIZSPINNER_s *spinner, nuvec_s *position) {
    if (spinner != NULL && position != NULL) {
        *position = spinner->position;
    }
}

i32 GizSpinner_GetTargetPoints(GIZSPINNER_s *spinner, nuvec_s *positions, nuvec_s *directions) {
    if (spinner == NULL || spinner->type == 0)
        return 0;
    u16 step = static_cast<u16>(65536 / spinner->type);
    u16 base = spinner->rotation + spinner->initial_rotation + spinner->field_0x08c;
    u16 position_angle = base - 0x8000;
    u32 flags = spinner->state_flags & 6;
    u16 direction_angle = base + ((flags == 0 || flags == 6) ? -0x4000 : 0x4000);
    f32 y = spinner->position.y + spinner->field_0x098;
    i32 count = 0;
    for (; count < spinner->type; ++count) {
        if (directions != NULL) {
            directions[count].x = NuTrigTable[direction_angle >> 1];
            directions[count].y = 0.0f;
            directions[count].z = NuTrigTable[((direction_angle + 0x4000) >> 1) & 0x7fff];
        }
        if (positions != NULL) {
            positions[count].x = spinner->field_0x094 * NuTrigTable[position_angle >> 1] + spinner->position.x;
            positions[count].y = y;
            positions[count].z =
                spinner->field_0x094 * NuTrigTable[((position_angle + 0x4000) >> 1) & 0x7fff] + spinner->position.z;
        }
        position_angle += step;
        direction_angle += step;
    }
    return count;
}

f32 GizSpinner_GetNearestTargetPoint(GIZSPINNER_s *spinner, nuvec_s *origin, nuvec_s *position, nuvec_s *direction,
                                     i32 check_direction) {
    NUVEC positions[8], directions[8];
    i32 count = GizSpinner_GetTargetPoints(spinner, positions, directions);
    if (count == 0 || origin == NULL)
        return -1.0f;
    f32 nearest_distance = 1000000000.0f;
    NUVEC *nearest_position = NULL;
    NUVEC *nearest_direction = NULL;
    for (i32 i = 0; i < count; ++i) {
        f32 distance = NuVecDistSqr(origin, &positions[i], NULL);
        if (check_direction != 0) {
            u16 angle = spinner->field_0x08c + 0x4000 - spinner->rotation - spinner->initial_rotation +
                        (-65536 / spinner->type) * i;
            NUVEC offset;
            NuVecSub(&offset, origin, &spinner->position);
            NuVecRotateY(&offset, &offset, angle);
            if (!(distance < nearest_distance && offset.z >= 0.0f))
                continue;
        } else if (!(distance < nearest_distance)) {
            continue;
        }
        nearest_distance = distance;
        nearest_position = &positions[i];
        nearest_direction = &directions[i];
    }
    if (nearest_position == NULL)
        return -1.0f;
    *position = *nearest_position;
    *direction = *nearest_direction;
    return nearest_distance;
}
