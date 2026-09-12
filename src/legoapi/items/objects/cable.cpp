#include "decomp.h"
#include "nu2api/numath/numtx.h"
#include "legoapi/legoapi_types.h"
#include "globals.h"
#include "legoapi/world/world.h"
#include "legoapi/audio/sfx.h"
#include "nu2api/numusic/sfx.h"
#include <string.h>
#include "legoapi/characters/core/character.h"
#include "legoapi/items/objects/gameobjects.h"
#include "nu2api/nucore/nupad.h"
#include "gameapi/ai/aisys/aisys.h"
#include "nu2api/numath/numath.h"
#include "nu2api/numath/nutrig.h"

extern CABLE_s cables[8];
extern AREADATA_s *GUNSHIP_ADATA;
void GameCam_Blend(GAMECAMERA_s *, f32, f32, i32);
void GameAudio_PlaySfxById(i32, NUVEC *, i32, i32);
f32 cable_ground_damping = 20.0f;
f32 cable_damping = 2.0f;
f32 cable_gravity = 3.0f;
f32 cable_speed = 20.0f;
i32 atat_locators[4] = {0, 1, 2, 3};
extern AREADATA_s *HOTHBATTLE_ADATA;
extern LEVELDATA_s *HOTHBATTLED_LDATA;
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);
void IncrementMinikitCounter(GameObject_s *);
void FaceOpponent(GameObject_s *, NUVEC *);
extern i16 id_ATAT, id_ATST, id_ATST_LOWRES;
extern i32 TERRAINMASK_NONDROID;
i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);
i32 XZLinesIntersect(NUVEC *, NUVEC *, NUVEC *, NUVEC *, f32 *, f32 *);
f32 default_targetmomf = 4.0f;
f32 targetmomf = 4.0f;
f32 default_ownermomf = 5.0f;
f32 ownermomf = 5.0f;
f32 default_max_cable_length = 15.0f;
f32 max_cable_length = 15.0f;
f32 default_tow_length = 6.0f;
f32 tow_length = 6.0f;

void InitCables(WORLDINFO_s *world) {
    memset(cables, 0, sizeof(cables));
    targetmomf = default_targetmomf;
    ownermomf = default_ownermomf;
    max_cable_length = default_max_cable_length;
    tow_length = default_tow_length;
    if (GUNSHIP_ADATA != NULL && world != NULL && GUNSHIP_ADATA == world->area) {
        targetmomf = 4.0f;
        max_cable_length = 10.0f;
        tow_length = 0.0f;
    }
}

CABLE_s *CreateCable(GameObject_s *source, GameObject_s *target, i32 sound) {
    if (target == NULL || source == NULL) {
        return NULL;
    }
    for (i32 i = 0; i < 8; ++i) {
        CABLE_s *cable = &cables[i];
        if ((cable->flags_1e9 & 1) == 0) {
            cable->flags_1e9 |= 1;
            cable->source = source;
            cable->target = target;
            if (source == player || source == player2) {
                GameCam_Blend(GameCam, 1.0f, 0.0f, 1);
            }
            if (sound) {
                PlaySfx("TowCable_Fire", &source->apiobj.collision_position);
            }
            return cable;
        }
    }
    return NULL;
}

void DestroyCable(CABLE_s *cable) {
    if (cable != NULL) {
        if (cable->source != NULL && cable->source->cable == cable) {
            cable->source->cable = NULL;
        }
        memset(cable, 0, sizeof(*cable));
    }
}
void ReleaseCable(CABLE_s *cable, i32 snapped) {
    cable->flags_1e9 |= 4;
    if (cable->target != NULL) {
        cable->target->field_0xf00 &= static_cast<u8>(~0x20u);
        cable->target->field_0xf01 &= static_cast<u8>(~2u);
    }
    i32 sound = GetSfxId(snapped ? "TowCable_Snap" : "TowCable_Detach");
    GameAudio_PlaySfxById(sound, cable->points, 1, 0);
    if (cable->source != NULL) {
        if (cable->source == player || cable->source == player2) {
            GameCam_Blend(GameCam, 1.0f, 0.0f, 1);
        }
        if (cable->source != NULL && cable->source->cable == cable) {
            cable->source->cable = NULL;
            cable->source = NULL;
        }
    }
    cable->target = NULL;
    for (i32 i = 0; i < cable->point_count; ++i) {
        cable->velocities[i].y = 0.0f;
    }
}
void UpdateCables() {
    for (CABLE_s *cable = cables; cable != cables + 8; ++cable) {
        if ((cable->flags_1e9 & 1) == 0)
            continue;
        NUVEC delta;
        if (cable->flags_1e9 & 4) {
            cable->total_length = 0.0f;
            bool grounded = true;
            for (i32 i = 0; i < cable->point_count; ++i) {
                f32 ground = GameShadow(NULL, &cable->points[i], 5.0f, -1);
                if (ground == 2000000.0f)
                    ground = cable->points[i].y;
                NUVEC *velocity = &cable->velocities[i];
                velocity->x -= (velocity->x * cable_damping) * FRAMETIME;
                velocity->y -= FRAMETIME * cable_gravity;
                velocity->z -= (cable_damping * velocity->z) * FRAMETIME;
                cable->points[i].x += FRAMETIME * velocity->x;
                cable->points[i].y += velocity->y * FRAMETIME;
                cable->points[i].z += FRAMETIME * velocity->z;
                if (ground > cable->points[i].y) {
                    cable->points[i].y = ground;
                    velocity->x -= (velocity->x * cable_ground_damping) * FRAMETIME;
                    velocity->z -= (cable_ground_damping * velocity->z) * FRAMETIME;
                } else {
                    grounded = false;
                }
                if (i < cable->point_count - 1) {
                    f32 length = NuVecDist(&cable->points[i + 1], &cable->points[i], &delta);
                    cable->segment_lengths[i] = length;
                    cable->total_length = length + cable->total_length;
                }
            }
            if (grounded)
                DestroyCable(cable);
            continue;
        }

        GameObject_s *target = cable->target;
        bool lost_target = target != NULL && target->apiobj.field_0x218 < -100.0f;
        auto release = [&](i32 snapped, bool toppled) {
            ReleaseCable(cable, snapped);
            if (toppled) {
                target->character_context = 0x3d;
                target->context_animation = 0x85;
                f32 duration = AnimDuration(static_cast<i16>(target->id), 0x85, 0, 0, 1);
                target->context_animation_timer = 0.0f;
                target->field_0xf02 |= 0x80;
                target->airborne_action_duration = duration;
                IncrementMinikitCounter(target);
                for (i32 j = 0; j < HIGHGAMEOBJECT; ++j) {
                    GameObject_s *object = &Obj[j];
                    if ((object->apiobj.field_0x1f8 & 0x1000) && object->apiobj.field_0x287 == 0 &&
                        object->cable != NULL && object->cable->target == target) {
                        ReleaseCable(object->cable, 1);
                    }
                }
            }
        };
        GameObject_s *source = cable->source;
        if (source == NULL || (source->apiobj.field_0x1f8 & 0x1000) == 0 || source->apiobj.field_0x287 != 0 ||
            (source->field_0xe20 & 0x20) || (target != NULL && target->character_context == 0x17) ||
            ((cable->flags_1e9 & 2) && source->pad_gamepad->pad != NULL &&
             (source->pad_gamepad->pad->digital_buttons_pressed & GAMEPAD_SPECIAL))) {
            release(0, false);
            continue;
        }
        cable->flags_1e9 |= 2;
        NUVEC path[16];
        path[0] = source->apiobj.collision_position;
        if (target == NULL || (target->apiobj.field_0x1f8 & 0x1000) == 0 || target->apiobj.field_0x287 != 0) {
            release(0, false);
            continue;
        }
        if (target->id == id_ATAT) {
            auto locator = [&](i32 index) -> NUVEC * {
                return reinterpret_cast<NUVEC *>(&cable->target->joint_matrices[atat_locators[index]].m30);
            };
            if (cable->wrap_count == 0) {
                f32 nearest = NuVecDistSqr(&path[0], locator(0), &delta);
                if (!(nearest < 1000000000.0f))
                    nearest = 1000000000.0f;
                i32 selected = 0;
                for (i32 i = 1; i < 4; ++i) {
                    f32 distance = NuVecDistSqr(&path[0], locator(i), &delta);
                    if (nearest > distance) {
                        nearest = distance;
                        selected = i;
                    }
                }
                cable->wrap_indices[cable->wrap_count++] = static_cast<u8>(selected);
            }
            i32 last = cable->wrap_indices[cable->wrap_count - 1];
            NUVEC *last_position = locator(last);
            if (cable->wrap_count > 1) {
                i32 previous = cable->wrap_indices[cable->wrap_count - 2];
                i32 next = (last + 1) & 3;
                i32 direction = 1;
                if (next == previous) {
                    next = (last + 3) & 3;
                    direction = -1;
                }
                NuVecSub(&delta, &path[0], last_position);
                i32 angle = NuAtan2D(delta.x, delta.z);
                NuVecSub(&delta, locator(next), last_position);
                i32 next_angle = NuAtan2D(delta.x, delta.z);
                i32 turn = NuAngSub(next_angle, angle);
                if (cable->wrap_count < 15 && turn * direction > 0) {
                    cable->wrap_indices[cable->wrap_count++] = static_cast<u8>(next);
                    if (cable->wrap_count == 15) {
                        DeactivatePlayer(cable->target, WORLD->current_level == HOTHBATTLED_LDATA ? 60.0f : 30.0f,
                                         NULL);
                        PlaySfx("AT_AT_FALL", &cable->target->apiobj.collision_position);
                        lost_target = true;
                    }
                } else {
                    NuVecSub(&delta, last_position, locator(previous));
                    i32 previous_angle = NuAtan2D(delta.x, delta.z);
                    if (NuAngSub(previous_angle, angle) * direction < 0) {
                        // The original clears the slot at the old count, which overlaps point_count at 15.
                        if (cable->wrap_count == 15)
                            cable->point_count = 0xff;
                        else
                            cable->wrap_indices[cable->wrap_count] = 0xff;
                        --cable->wrap_count;
                    }
                }
            } else {
                f32 along0, along1;
                i32 next = (last + 1) & 3;
                i32 previous = (last + 3) & 3;
                if (XZLinesIntersect(&path[0], last_position, locator(next), locator((last + 2) & 3), &along0,
                                     &along1)) {
                    cable->wrap_indices[cable->wrap_count++] = static_cast<u8>(next);
                } else if (XZLinesIntersect(&path[0], last_position, locator(previous), locator((previous + 3) & 3),
                                            &along0, &along1)) {
                    cable->wrap_indices[cable->wrap_count++] = static_cast<u8>(previous);
                }
            }
        } else if (target->id == id_DRAGBOMB) {
            target->character_context = 0x34;
            cable->target->context_animation_timer = 0.0f;
        }
        i32 path_count;
        if (cable->wrap_count != 0) {
            path_count = 1;
            for (i32 i = cable->wrap_count - 1; i >= 0; --i) {
                i32 index = cable->wrap_indices[i];
                if (cable->target->apiobj.character_model->points_of_interest[index]) {
                    path[path_count++] =
                        *reinterpret_cast<NUVEC *>(&cable->target->joint_matrices[atat_locators[index]].m30);
                }
            }
        } else {
            path_count = 2;
            path[1] = cable->target->apiobj.collision_position;
        }
        if (lost_target) {
            target = cable->target;
            release(0, false);
            continue;
        }
        if ((cable->source->apiobj.character_data->game_character->flags_090 & 0x400) == 0) {
            target = cable->target;
            release(0, false);
            continue;
        }
        f32 distance_sq = NuVecDistSqr(&path[0], &path[1], &delta);
        if (GameRayCast(&path[1], &delta, 0.0f, TERRAINMASK_NONDROID | 0x1f)) {
            target = cable->target;
            release(1, false);
            continue;
        }
        bool obstruction = false;
        for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
            GameObject_s *object = &Obj[i];
            if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
                (object->field_0xe20 & 0x20) || (object->apiobj.field_0x1f8 & 0x80))
                continue;
            if (WORLD->area != NULL && WORLD->area == HOTHBATTLE_ADATA && object->id != id_ATAT &&
                object->id != id_DRAGBOMB && object->id != id_ATST && object->id != id_ATST_LOWRES)
                continue;
            if (object->cable != NULL &&
                (object->cable->target == cable->target || object->cable->target == cable->source))
                continue;
            if (object == cable->source || object == cable->target)
                continue;
            NUVEC closest;
            f32 line_distance =
                NuLineToPointDistSqrEx(&path[0], &path[1], &object->apiobj.collision_position, &closest);
            f32 radius = object->apiobj.field_0x1dc;
            f32 half_height = object->apiobj.field_0x1e0;
            f32 combined = radius + half_height;
            if (combined * combined > line_distance && object->apiobj.collision_position.y + half_height > closest.y &&
                closest.y > object->apiobj.collision_position.y - half_height) {
                f32 dx = object->apiobj.collision_position.x - closest.x;
                f32 dz = object->apiobj.collision_position.z - closest.z;
                if (radius * radius > dx * dx + dz * dz) {
                    obstruction = true;
                    break;
                }
            }
        }
        if (obstruction) {
            target = cable->target;
            release(1, false);
            continue;
        }
        target = cable->target;
        if (target != NULL && (target->apiobj.field_0x1f8 & 0x1000)) {
            if (target->id != id_ATAT) {
                target->field_0xf01 |= 2;
                AIScriptSetBaseScriptStateByName(&cable->target->ai.script_process, "BeingTowed");
                cable->target->ai.opponent = cable->source;
                FaceOpponent(cable->target, &cable->source->apiobj.collision_position);
                target = cable->target;
                if ((target->field_0xefd & 0x80) && !(CInfo[static_cast<i8>(target->character_context)].flags & 1))
                    target->apiobj.movement_facing_angle += 0x8000;
                target->ai.goal_speed_mode = 1;
                cable->target->field_0xefd |= 0x80;
            }
            if (distance_sq > tow_length * tow_length) {
                f32 maximum_sq = max_cable_length * max_cable_length;
                f32 strength = distance_sq > maximum_sq ? distance_sq / maximum_sq : 1.0f;
                NuVecScale(&delta, &delta, FRAMETIME);
                NUVEC target_impulse, source_impulse;
                NuVecScale(&target_impulse, &delta, strength * targetmomf);
                NuVecScale(&source_impulse, &delta, strength * ownermomf);
                if (cable->max_length == 1000000000.0f && cable->target->id != id_ATAT) {
                    target_impulse.y = 0.0f;
                    NuVecAdd(&cable->target->apiobj.velocity, &cable->target->apiobj.velocity, &target_impulse);
                    if (cable->total_length > tow_length) {
                        cable->pull_time += FRAMETIME;
                        cable->target->field_0xf00 |= 0x20;
                    } else {
                        cable->pull_time = 0.0f;
                    }
                    if ((cable->target->id == id_ATST || cable->target->id == id_ATST_LOWRES) &&
                        cable->pull_time > 1.0f) {
                        NuVecSub(&cable->source->apiobj.velocity, &cable->source->apiobj.velocity, &source_impulse);
                        target = cable->target;
                        release(0, true);
                        continue;
                    }
                }
                NuVecSub(&cable->source->apiobj.velocity, &cable->source->apiobj.velocity, &source_impulse);
            }
        }
        cable->total_length = 0.0f;
        if (cable->max_length == 1000000000.0f) {
            cable->point_count = static_cast<u8>(path_count);
            for (i32 i = 0; i < path_count; ++i) {
                cable->points[i] = path[i];
                if (i < path_count - 1) {
                    f32 length = NuVecDist(&path[i + 1], &path[i], &delta);
                    cable->segment_lengths[i] = length;
                    cable->total_length = length + cable->total_length;
                }
            }
        } else {
            f32 remaining = cable->max_length + FRAMETIME * cable_speed;
            cable->points[0] = path[0];
            cable->point_count = 1;
            cable->max_length = remaining;
            for (i32 i = 0; i < path_count - 1; ++i) {
                f32 length = NuVecDist(&path[i + 1], &path[i], &delta);
                if (!(remaining >= length)) {
                    cable->segment_lengths[i] = remaining;
                    cable->total_length += remaining;
                    f32 scale = length == 0.0f || remaining == 0.0f ? 0.0f : remaining / length;
                    NuVecScale(&delta, &delta, scale);
                    NuVecAdd(&cable->points[cable->point_count], &cable->points[cable->point_count - 1], &delta);
                    ++cable->point_count;
                    break;
                }
                remaining -= length;
                cable->segment_lengths[i] = length;
                cable->total_length += length;
                cable->points[cable->point_count++] = path[i + 1];
                if (i == path_count - 2) {
                    cable->max_length = 1000000000.0f;
                    PlaySfx("TowCable_Latch", &path[i + 1]);
                }
            }
        }
        if (cable->source != NULL && cable->target != NULL) {
            for (i32 i = 0; i < cable->point_count; ++i)
                cable->velocities[i] = i == 0 ? cable->source->apiobj.velocity : cable->target->apiobj.velocity;
        }
    }
}

void CableTargetGameObject(GameObject_s *, nuvec_s *, float) {
}
void CableCode(GameObject_s *, i32, float) {
}
