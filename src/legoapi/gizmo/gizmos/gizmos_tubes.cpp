#include "legoapi/world/world.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include <string.h>
#include "legoapi/core/input/qrand.h"
#include "globals.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"
#include "legoapi/render/fx.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/world/level.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/world/area.h"
#include "legoapi/menus/core/gamehint.h"
#include "legoapi/menus/core/gamemessage.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/audio/audio.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "nu2api/nucore/nustring.h"

#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/gizmos/transport/tubes.h"
#include "legoapi/gizmos/traps/giztorpmachine.h"

void HomeNearestTorpTarget(BOLT_s *, TORPEDOPACKET_s *);
void Torpedo_UpdateJobbies(GameObject_s *);
void *FindNearestTorpTarget(WORLDINFO_s *, NUVEC *, f32, u8 *);
void FindAnglesXY(NUVEC *, u16 *, u16 *);
void GetRotationAngles(NUVEC *, u16 *, u16 *);
extern AREADATA *BOUNTYHUNTERPURSUIT_ADATA;
extern AREADATA *DOGFIGHT_ADATA;
extern AREADATA *PODSPRINT_ADATA;
extern AREADATA *GUNSHIP_ADATA;
extern i32 LEGOCONTEXT_GLIDE;
extern "C" void AddVariableShotDebrisEffectTimed1(i32, NUVEC *, i32, f32, i16, i16, NUMTX *);
void TorpedoHitTarget(BOLT_s *);
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void NewRumbleAllPlayers(f32, f32, i32, i32);
i32 GizmoBlowupBlowup(GIZMOBLOWUP_s *, i32, i32, i32, GameObject_s *, i32);
void GizTurrets_Hit(void *, GIZTURRET_s *, NUVEC *, i32, i32);
i32 GizObstacles_Hit(void *, GIZOBSTACLE_s *, NUVEC *, i32, i32);
void Bolt_End(BOLT_s *, i32);
i32 Bolt_HitGameObjects(BOLT_s *, NUVEC *, NUVEC *, NUVEC *, f32, u8 *);
GIZTORPMACHINE *GizTorpMachine_FindNearest(WORLDINFO_s *, NUVEC *, f32 *);
extern "C" void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);
BOLT_s *Bolt_Find(i32, NUVEC *, GameObject_s *);
void NewRumble(nupad_s *, f32, i32);
void NewBuzz(nupad_s *, f32, i32);
void DrawGameMessage_Targets(GAMEMESSAGE_s *, NUVEC *, f32);
void DrawTorpedoTargetSprite(void *, u8, f32);

f32 steal_torpedo_range = 2.0f;
f32 steal_torpedo_timer = 2.0f;
f32 TORPEDOGRABRANGE2 = 36.0f;

void TorpedoCode(GameObject_s *object, i32 fire, f32 fire_cooldown) {
    TORPEDOPACKET *packet = object->torpedo;
    if (packet == NULL)
        return;

    if (WORLD != NULL &&
        (WORLD->area == DOGFIGHT_ADATA || WORLD->area == PODSPRINT_ADATA || WORLD->area == GUNSHIP_ADATA)) {
        memset(packet, 0, sizeof(*packet));
        return;
    }

    if (object->torpedo_fire_cooldown > 0.0f)
        object->torpedo_fire_cooldown -= FRAMETIME;

    const i32 bolt_type =
        WORLD != NULL && WORLD->area != NULL && (WORLD->area->flags & AREAFLAG_BONUS_AREA) != 0 ? 16 : 15;
    if (Bolt_Find(bolt_type, NULL, object) != NULL)
        packet->field_0x1 |= 2;
    else
        packet->field_0x1 &= ~2U;

    if (fire != 0 && object->character_context == -1 && object->torpedo_fire_cooldown <= 0.0f &&
        object->apiobj.field_0x287 == 0 && (packet->field_0x1 & 2) == 0 && packet->field_08 >= 0.8f &&
        packet->count != 0) {
        PlaySfx(const_cast<char *>("XWing_Torpedo"), &object->apiobj.collision_position);
        NewRumble(object->pad_gamepad->pad, 0.5f, 0);
        NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
        object->field_0xe23 |= 4;
        object->field_0xef9 |= 8;
        object->torpedo_fire_cooldown = fire_cooldown;
        packet->field_02 = 0;
        if (static_cast<i8>(object->apiobj.flags_low) < 0)
            Hint_SetComplete(0x289);
    }

    const i32 maximum = getMaxTorpedos(object);
    while (packet->count > maximum) {
        const i32 debris =
            WORLD != NULL && WORLD->area != NULL && (WORLD->area->flags & AREAFLAG_BONUS_AREA) != 0 ? 72 : 71;
        AddVariableShotDebrisEffect(WORLD->debris_sys->entries[debris].effect,
                                    &packet->pickup_positions[packet->count - 1], 30, 0, 0);
        --packet->count;
    }

    if (packet->field_08 < 0.8f) {
        packet->field_08 += FRAMETIME;
        if (packet->field_08 >= 0.8f)
            packet->field_03 = 0;
    } else {
        if (Cheat_IsOn(42) != 0 && static_cast<i8>(object->apiobj.flags_low) < 0 &&
            object->torpedo_fire_cooldown <= 0.0f && object->apiobj.field_0x287 == 0 && (packet->field_0x1 & 2) == 0 &&
            packet->count < maximum) {
            packet->pickup_positions[packet->count] = object->apiobj.position;
            ++packet->count;
            packet->field_08 = 0.0f;
            packet->field_03 = 0;
        }

        if (packet->count < maximum) {
            f32 distance;
            GIZTORPMACHINE *machine = GizTorpMachine_FindNearest(WORLD, &object->apiobj.collision_position, &distance);
            if (machine != NULL && (machine->flags & GIZTORPMACHINE_FLAG_ACTIVE) != 0 && distance < TORPEDOGRABRANGE2) {
                NewRumble(object->pad_gamepad->pad, qrand() * 1.5259022e-05f * 0.4f, 0);
                if (packet->count < maximum) {
                    NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                    packet->pickup_positions[packet->count] = machine->position;
                    ++packet->count;
                    machine->activation_time = 0.0f;
                    packet->field_08 = 0.0f;
                    packet->field_03 = 0;
                }
            }
        }
    }

    Torpedo_UpdateJobbies(object);
    if (packet->count == 0)
        return;

    BOLTTYPE_s *type = BoltType_FindByID(bolt_type, WORLD);
    u8 target_type = packet->target_type;
    if ((packet->field_0x1 & 2) == 0 || packet->target == NULL) {
        const f32 range = type->field_14 * type->field_10;
        void *target = FindNearestTorpTarget(WORLD, &object->apiobj.position, range * range, &target_type);
        if (target == packet->target) {
            object->torpedo_target_timer += FRAMETIME;
            if (object->torpedo_target_timer > 0.25f)
                object->torpedo_target_timer = 0.25f;
        } else if (target != NULL) {
            if (object->torpedo_target_timer <= 0.0f || packet->target == NULL) {
                packet->target = target;
                packet->target_type = target_type;
                object->torpedo_target_timer = 0.0f;
            } else {
                const f32 timer = object->torpedo_target_timer - 2.0f * FRAMETIME;
                object->torpedo_target_timer = timer < 0.0f ? 0.0f : timer;
            }
        } else if ((packet->field_0x1 & 6) == 0) {
            if (object->torpedo_target_timer > 0.0f && packet->target != NULL) {
                const f32 timer = object->torpedo_target_timer - 2.0f * FRAMETIME;
                object->torpedo_target_timer = timer < 0.0f ? 0.0f : timer;
            } else {
                packet->target = NULL;
            }
        }
    } else {
        object->torpedo_target_timer += FRAMETIME;
        if (object->torpedo_target_timer > 0.25f)
            object->torpedo_target_timer = 0.25f;
    }

    if (static_cast<i8>(object->apiobj.flags_low) < 0 && packet->target != NULL &&
        object->torpedo_target_timer > 0.0f) {
        NUVEC *target_position = NULL;
        switch (packet->target_type) {
            case 0:
                target_position = &static_cast<GIZMOBLOWUP_s *>(packet->target)->mid_position;
                break;
            case 1:
                target_position =
                    NuSpecialGetDrawPos(&static_cast<GIZTURRET_s *>(packet->target)->primary_anim_obj->special);
                break;
            case 2:
                target_position = &static_cast<GIZOBSTACLE_s *>(packet->target)->evaluated_position;
                break;
        }
        if (target_position != NULL) {
            GAMEMESSAGE_s *message = static_cast<GAMEMESSAGE_s *>(
                AddGameMessage(const_cast<char *>("X"), target_position, 5.0f * AreaPickupScale, NULL, 0.0f, 255, 0,
                               255, 0x10c7, 0.0f));
            if (message != NULL) {
                message->target_type = 4;
                message->draw_callback = DrawGameMessage_Targets;
                message->alpha = static_cast<u8>(object->torpedo_target_timer * 4.0f * 128.0f);
            }
        }
    }
}

f32 Torpedo_Scale(BOLT_s *bolt) {
    if (bolt != NULL && bolt->owner != NULL)
        return 2.0f * bolt->owner->apiobj.field_0x1dc;
    return 3.0f;
}

void Torpedo_Shoot(GameObject_s *object) {
    NUVEC position = object->torpedo->pickup_positions[object->torpedo->count - 1];
    NUVEC direction;
    const u16 angle = BoltSys->shoot_direction(object, &direction);
    NUMTX matrix;
    matrix.m00 = matrix.m22 = NU_COS_LUT(angle);
    matrix.m20 = NU_SIN_LUT(angle);
    matrix.m02 = -matrix.m20;
    matrix.m11 = matrix.m33 = 1.0f;
    matrix.m01 = matrix.m03 = matrix.m10 = matrix.m12 = matrix.m13 = 0.0f;
    matrix.m21 = matrix.m23 = matrix.m30 = matrix.m31 = matrix.m32 = 0.0f;

    i32 type = 15;
    if (WORLD != NULL && WORLD->area != NULL && (WORLD->area->flags & AREAFLAG_BONUS_AREA) != 0) {
        type = 16;
    }
    Bolt_Add(object, &position, &matrix, type, 0);
    --object->torpedo->count;
    object->torpedo->field_0x1 |= 2;
    if (static_cast<i8>(object->apiobj.flags_low) < 0) {
        Hint_SetComplete(0x289);
    }
}

void Tube_MoveCode(GameObject_s *object, WORLDINFO_s *world) {
    if (LEGOCONTEXT_GLIDE != -1 && object->character_context == LEGOCONTEXT_GLIDE && object->field_0x788 != NULL) {
        if (Tube_InCylinder(object, static_cast<TUBE *>(object->field_0x788), NULL, 0) == 0)
            object->field_0x788 = NULL;
        return;
    }

    if (LEGOCONTEXT_TUBE == -1)
        return;
    if (object->character_context == LEGOCONTEXT_TUBE) {
        if (Tube_InCylinder(object, static_cast<TUBE *>(object->field_0x788), NULL, 0) == 0)
            object->character_context = -1;
        return;
    }
    if ((CInfo[object->character_context].flags & 0x4000) != 0 || world->tubes == NULL)
        return;

    for (i32 index = 0; index < world->tube_count; ++index) {
        TUBE *tube = &world->tubes[index];
        if ((tube->flags & (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE | TUBE_FLAG_DIRECTIONAL)) !=
            (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE)) {
            continue;
        }
        if (Tube_InCylinder(object, tube, NULL, 0) == 0 &&
            (object->tube_entry_state != 5 || object->tube_entry_data == NULL)) {
            continue;
        }

        if (LEGOCONTEXT_GLIDE != -1 && object->character_context == LEGOCONTEXT_GLIDE) {
            object->field_0x788 = tube;
            return;
        }
        object->field_0x788 = tube;
        object->field_0xe31 = 0;
        object->character_context = LEGOCONTEXT_TUBE;
        if (static_cast<i8>(object->apiobj.flags_low) < 0 && tube->audio_cooldown == 0.0f) {
            tube->audio_cooldown = 4.0f;
            GameAudio_PlaySfx(6, &object->apiobj.collision_position, 0, 0);
        }
        return;
    }
}

void Tube_SetObjBit(TUBE *tube, i32 object_index) {
    tube->occupied_object_masks[object_index / 32] |= 1U << object_index;
}

void Torpedo_EndBolt(BOLT_s *bolt) {
    TorpedoHitTarget(bolt);
    GameCam_Judder(GameCam, qrand() < 0x8000 ? 0.4f : -0.4f, 2, NULL);
    NewRumbleAllPlayers(0.7f, 0.0f, 0, 0);
}

TUBE *Tube_FindByName(WORLDINFO_s *world, char *name) {
    if (world == NULL || world->tubes == NULL)
        return NULL;
    for (i32 index = 0; index < world->tube_count; ++index) {
        if (NuStrICmp(world->tubes[index].name, name) == 0)
            return &world->tubes[index];
    }
    return NULL;
}

i32 Tube_InCylinder(GameObject_s *object, TUBE *tube, f32 *horizontal_distance_squared, i32 ignore_height) {
    if (tube == NULL || object == NULL) {
        return 0;
    }

    if (ignore_height == 0) {
        if (tube->position.y > object->apiobj.collision_max.y || object->apiobj.collision_min.y > tube->top) {
            return 0;
        }
    }

    const f32 delta_x = object->apiobj.collision_position.x - tube->position.x;
    const f32 delta_z = object->apiobj.collision_position.z - tube->position.z;
    const f32 distance_squared = delta_x * delta_x + delta_z * delta_z;

    f32 radius_squared = tube->radius_squared;
    if ((tube->flags & TUBE_FLAG_TOUCH_RADIUS) != 0 && TouchHacks::TouchControlsActive) {
        radius_squared *= 0.8f;
    }

    if (distance_squared > radius_squared) {
        return 0;
    }
    if (horizontal_distance_squared != NULL) {
        *horizontal_distance_squared = distance_squared;
    }
    return 1;
}

void TorpedoHitTarget(BOLT_s *bolt) {
    if (bolt == NULL || bolt->owner == NULL || bolt->owner->torpedo == NULL)
        return;

    TORPEDOPACKET *packet = bolt->owner->torpedo;
    if (packet->target != NULL) {
        NUVEC *position = NULL;
        f32 radius;
        switch (packet->target_type) {
            case 2: {
                GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(packet->target);
                radius = obstacle->field_0x58;
                position = &obstacle->evaluated_position;
                break;
            }
            case 0: {
                GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(packet->target);
                radius = blowup->target_scale;
                position = &blowup->mid_position;
                break;
            }
            case 1: {
                GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(packet->target);
                radius = NuSpecialGetOriginRadius(&turret->primary_anim_obj->special);
                position = NuSpecialGetDrawPos(&turret->primary_anim_obj->special);
                break;
            }
        }
        if (position != NULL) {
            NUVEC delta;
            NuVecSub(&delta, position, &bolt->position);
            if (NuVecMagSqr(&delta) < radius * radius) {
                i32 player = bolt->owner != NULL ? static_cast<i8>(bolt->owner->apiobj.field_0x27c) : -1;
                TORPEDOPACKET *hit_packet = bolt->owner->torpedo;
                u8 target_type = hit_packet->target_type;
                void *target = hit_packet->target;
                if (target != NULL) {
                    switch (target_type) {
                        case 0:
                            GizmoBlowupBlowup(static_cast<GIZMOBLOWUP_s *>(target), 1, 11, 1, NULL, 1);
                            break;
                        case 1:
                            GizTurrets_Hit(WORLD, static_cast<GIZTURRET_s *>(target), &bolt->position, player, -1);
                            break;
                        case 2:
                            GizObstacles_Hit(WORLD, static_cast<GIZOBSTACLE_s *>(target), &bolt->position, player, -1);
                            break;
                    }
                }
            }
        }
    }
    packet->field_0x1 = 1;
}

void DrawTorpedos(GameObject_s *object) {
    TORPEDOPACKET *packet = object->torpedo;
    if (packet == NULL)
        return;
    if (packet->target != NULL && static_cast<i8>(object->apiobj.flags_low) < 0)
        DrawTorpedoTargetSprite(packet->target, packet->target_type, 4.0f * object->torpedo_target_timer);
    if (packet->count == 0)
        return;

    for (i32 index = 0; index < packet->count; ++index) {
        i32 x_rotation = -NUANG_90DEG - static_cast<u16>(packet->pickup_flags[index]);
        if (packet->field_08 < 0.8f && index == packet->count - 1) {
            x_rotation = static_cast<i32>(RotDiff(0, static_cast<u16>(x_rotation)) *
                                          NU_SIN_LUT(packet->field_08 / 0.8f * 16384.0f));
        }

        NUMTX matrix;
        NuMtxSetIdentity(&matrix);
        NuMtxPreRotateX(&matrix, static_cast<u16>(x_rotation));
        NuMtxPreRotateY(&matrix, static_cast<u16>(packet->pickup_data[index]));
        NuMtxTranslate(&matrix, &packet->pickup_positions[index]);

        f32 blend = 1.0f;
        if (packet->field_03 == 0 && index == packet->count - 1) {
            blend = 1.0f - (NU_SIN_LUT(packet->field_08 / 0.8f * 32768.0f + 16384.0f) + 1.0f) * 0.5f;
        }
        const f32 machine_scale = 4.0f * WORLD->giz_torp_machine_sys->scale;
        const f32 scale = (2.0f * object->apiobj.field_0x1dc - machine_scale) * blend + machine_scale;
        NuMtxPreScaleU(&matrix, scale);

        if (WORLD->lev_objs[0x79].active != 0)
            NuSpecialDrawAt(&WORLD->lev_objs[0x79].special, &matrix);
        else if (WORLD->lev_objs[0x7a].active != 0)
            NuSpecialDrawAt(&WORLD->lev_objs[0x7a].special, &matrix);
    }
}

void Torpedo_InitBolt(BOLT_s *bolt) {
    if (bolt->owner != NULL && bolt->owner->torpedo != NULL) {
        HomeNearestTorpTarget(bolt, bolt->owner->torpedo);
        bolt->flags &= ~4U;
    }
}

void *FindNearestTorpTarget(WORLDINFO_s *world, NUVEC *position, f32 distance_squared, u8 *target_type) {
    if (world->gizmo_blowup_count == 0)
        return NULL;
    void *nearest = NULL;
    u8 type = 0;
    NUVEC delta;
    for (i32 i = 0; i < world->gizmo_blowup_count; ++i) {
        GIZMOBLOWUP_s *blowup = &world->gizmo_blowups[i];
        if ((blowup->visibility_flags & 0x40) != 0 && (blowup->status_flags & 0x20008000) != 0 &&
            (blowup->status_flags & 0x800001) == 0x800000 && (blowup->draw_flags & 0x1000000) != 0) {
            f32 distance = NuVecDistSqr(&blowup->mid_position, position, &delta);
            if (distance < distance_squared) {
                distance_squared = distance;
                nearest = blowup;
            }
        }
    }
    if (WORLD->area != NULL && WORLD->area == BOUNTYHUNTERPURSUIT_ADATA) {
        if (world->giz_turret_sys != NULL) {
            GIZTURRET_s *turret = world->giz_turret_sys->turrets;
            for (i32 i = 0; i < world->giz_turret_sys->count; ++i, ++turret) {
                if ((turret->flags & 4) != 0 && (turret->flags & 2) != 0 && (turret->flags & 0x30) == 0 &&
                    turret->primary_anim_obj != NULL) {
                    NUVEC *draw_position = NuSpecialGetDrawPos(&turret->primary_anim_obj->special);
                    if (draw_position != NULL) {
                        f32 distance = NuVecDistSqr(draw_position, position, &delta);
                        if (distance < distance_squared) {
                            distance_squared = distance;
                            nearest = turret;
                            type = 1;
                        }
                    }
                }
            }
        }
    }
    if (WORLD->area != NULL && WORLD->area == BOUNTYHUNTERPURSUIT_ADATA) {
        if (world->giz_obstacle_sys != NULL) {
            for (i32 i = 0; i < world->giz_obstacle_sys->active_gizmo_count; ++i) {
                GIZOBSTACLE_s *obstacle =
                    static_cast<GIZOBSTACLE_s *>(world->giz_obstacle_sys->active_gizmos[i]->object);
                if ((obstacle->progress_flags & 2) != 0 && (obstacle->progress_flags & 1) != 0 &&
                    (obstacle->runtime_flags & 0x80) == 0) {
                    f32 distance = NuVecDistSqr(&obstacle->evaluated_position, position, &delta);
                    if (distance < distance_squared) {
                        distance_squared = distance;
                        nearest = obstacle;
                        type = 2;
                    }
                }
            }
        }
    }
    if (nearest != NULL && target_type != NULL)
        *target_type = type;
    return nearest;
}

void HomeNearestTorpTarget(BOLT_s *bolt, TORPEDOPACKET_s *packet) {
    BOLTTYPE_s *type = BoltType_FindByID(15, WORLD);
    if (packet == NULL || bolt == NULL || (packet->field_0x1 & 8) != 0)
        return;
    f32 range = (bolt->lifetime - bolt->time) * bolt->speed;
    void *target = packet->target;
    u8 target_type;
    if (target == NULL) {
        target = FindNearestTorpTarget(WORLD, &bolt->position, range * range, &target_type);
        if (target == NULL) {
            if (bolt->time == 0.0f)
                bolt->velocity.y += 6.0f;
            return;
        }
    } else {
        target_type = packet->target_type;
    }

    NUVEC delta;
    NUVEC *target_position = NULL;
    switch (target_type) {
        case 0:
            target_position = &static_cast<GIZMOBLOWUP_s *>(target)->mid_position;
            break;
        case 1: {
            target_position = NuSpecialGetDrawPos(&static_cast<GIZTURRET_s *>(target)->primary_anim_obj->special);
            break;
        }
        case 2:
            target_position = &static_cast<GIZOBSTACLE_s *>(target)->evaluated_position;
            break;
    }
    if (target_position != NULL)
        NuVecSub(&delta, target_position, &bolt->position);
    f32 height = delta.y;
    delta.y = 0.0f;
    f32 distance = NuVecMag(&delta);
    f32 speed = bolt->speed;
    if (bolt->time == 0.0f) {
        bolt->velocity.y += 6.0f;
    } else if ((packet->field_0x1 & 0x10) == 0) {
        u16 current_x, current_y, target_x, target_y;
        FindAnglesXY(&bolt->velocity, &current_x, &current_y);
        bolt->velocity.x = 0.0f;
        bolt->velocity.y = 0.0f;
        bolt->velocity.z = bolt->speed;
        delta.y = height + 0.1f;
        NuVecNorm(&delta, &delta);
        FindAnglesXY(&delta, &target_x, &target_y);
        f32 seek = (1.0f + NU_SIN_LUT(bolt->time / type->field_14 * 16384.0f + 32768.0f + 16384.0f)) * 15.0f;
        target_x = SeekRot(current_x, target_x, seek);
        target_y = SeekRot(current_y, target_y, seek);
        NUMTX matrix __attribute__((aligned(16)));
        NuMtxSetIdentity(&matrix);
        NUANGVEC angles;
        angles.x = target_x;
        angles.y = target_y;
        NuMtxSetRotationXYVU0(&matrix, &angles);
        NuVecMtxRotate(&bolt->velocity, &bolt->velocity, &matrix);
        NuVecNorm(&bolt->field_0xac, &bolt->velocity);
        f32 lifetime = distance / speed + 0.05f + bolt->time;
        if (lifetime > bolt->lifetime && (packet->field_0x1 & 8) == 0)
            bolt->lifetime = lifetime;
    }
    packet->field_0x1 |= 4;
    packet->target = target;
}

void Torpedo_Ricochet(BOLT_s *bolt, TORPEDOPACKET_s *packet) {
    if (packet == NULL || bolt == NULL)
        return;
    if ((packet->field_0x1 & 8) != 0) {
        if (packet->ricochet_time == 0.0f) {
            NUVEC axis;
            NuVecCross(&axis, &packet->ricochet_position, &bolt->velocity);
            NuVecNorm(&axis, &axis);
            NUMTX matrix __attribute__((aligned(16)));
            NuMtxSetIdentity(&matrix);
            f32 lengths = NuVecMag(&packet->ricochet_position) * NuVecMag(&bolt->velocity);
            f32 dot = NuVecDot(&packet->ricochet_position, &bolt->velocity);
            f32 cosine = lengths == 0.0f || dot == 0.0f ? 0.0f : dot / lengths;
            f32 absolute = NuFabs(cosine);
            f32 root = NuFsqrt(1.0f - cosine * cosine);
            f32 smaller = MIN(root, absolute);
            f32 quadrant = CLAMP((absolute - 0.70710677f) * 3.40282e38f, -1.0f, 1.0f);
            f32 sign = MIN(cosine * 3.40282e38f, 1.0f);
            sign = MAX(sign, -1.0f);
            f32 product = quadrant * sign;
            f32 x = smaller * product;
            f32 square = x * x;
            f32 cube = x * square;
            f32 fourth = square * square;
            f32 angle_radians = (product + sign) * 0.785398f - x;
            angle_radians += (x * -0.166667f) * square;
            angle_radians += (-0.075f * square) * cube;
            angle_radians += (-0.0446429f * cube) * fourth;
            angle_radians += (-0.0303819f * fourth) * (square * cube);
            i16 angle = 0x4000 - static_cast<i32>(angle_radians * 10430.4f);
            NuMtxSetRotationAxis(&matrix, 0x8000 - 2 * angle, &axis);
            NuVecMtxRotate(&bolt->velocity, &bolt->velocity, &matrix);
            NuVecScale(&bolt->velocity, &bolt->velocity, 0.8f);
            bolt->speed *= 0.8f;
            AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[71].effect, &bolt->position, 60, FRAMETIME, 0,
                                              0, NULL);
            packet->ricochet_time += FRAMETIME;
            packet->field_0x1 |= 0x10;
        } else if (packet->ricochet_time < 0.2f) {
            packet->ricochet_time += FRAMETIME;
        } else {
            packet->field_0x1 &= ~0x18;
            packet->ricochet_position = v000;
        }
    }
    bolt->lifetime = bolt->time + 0.0001f;
}

i32 Tube_IsObjBitSet(TUBE *tube, i32 object_index) {
    return tube->occupied_object_masks[object_index / 32] >> object_index & 1;
}

void Torpedo_UpdateBolt(BOLT_s *bolt) {
    if (bolt->owner != NULL && bolt->owner->torpedo != NULL) {
        TORPEDOPACKET *packet = bolt->owner->torpedo;
        HomeNearestTorpTarget(bolt, packet);

        if (packet->target != NULL) {
            NUVEC *target_position = NULL;
            f32 target_radius;
            switch (packet->target_type) {
                case 0: {
                    GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(packet->target);
                    target_position = &blowup->mid_position;
                    target_radius = blowup->target_scale;
                    break;
                }
                case 1: {
                    GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(packet->target);
                    target_radius = NuSpecialGetOriginRadius(&turret->primary_anim_obj->special);
                    target_position = NuSpecialGetDrawPos(&turret->primary_anim_obj->special);
                    break;
                }
                case 2: {
                    GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(packet->target);
                    target_position = &obstacle->evaluated_position;
                    target_radius = obstacle->field_0x58;
                    break;
                }
            }

            if (target_position != NULL) {
                NUVEC direction;
                NuVecSub(&direction, target_position, &bolt->position);
                if (NuVecMagSqr(&direction) < target_radius * target_radius) {
                    Bolt_End(bolt, 1);
                }
            }
        }
    }

    if (bolt->time >= bolt->lifetime && bolt->owner != NULL && bolt->owner->torpedo != NULL &&
        (bolt->owner->torpedo->field_0x1 & 8) != 0) {
        NUVEC offset = {0.0f, 2.0f * bolt->collision_radius, 0.0f};
        NuVecMtxRotate(&offset, &offset, &bolt->orientation);
        NUVEC points;
        NuVecSub(&points, &bolt->position, &offset);
        if (Bolt_HitGameObjects(bolt, &points, &bolt->bounds_min, &bolt->bounds_max, bolt->collision_radius, NULL) ==
            0) {
            Torpedo_Ricochet(bolt, bolt->owner->torpedo);
        }
    }
}

void Torpedo_InitRicochet(BOLT_s *bolt, nuvec_s *position) {
    if (bolt != NULL && bolt->owner != NULL && bolt->owner->torpedo != NULL) {
        TORPEDOPACKET *packet = bolt->owner->torpedo;
        if (packet->target != NULL) {
            if (packet->field_02 < 5) {
                packet->field_0x1 |= 8;
                packet->ricochet_position = *position;
                packet->ricochet_time = 0.0f;
                ++packet->field_02;
            } else {
                packet->field_0x1 |= 0x10;
            }
        }
    }
}

void Torpedo_UpdateJobbies(GameObject_s *object) {
    TORPEDOPACKET *packet = object->torpedo;

    if (packet->steal_timer > 0.0f) {
        const f32 timer = packet->steal_timer - FRAMETIME;
        packet->steal_timer = timer < 0.0f ? 0.0f : timer;
    }

    for (i32 index = 0; index < packet->count; ++index) {
        u16 target_z_rotation;
        u16 target_y_rotation;

        if (index != 0) {
            NUVEC direction;
            NuVecSub(&direction, &packet->pickup_positions[index], &packet->pickup_positions[index - 1]);
            GetRotationAngles(&direction, &target_z_rotation, &target_y_rotation);
            AddVariableShotDebrisEffectTimed1(
                WORLD->debris_sys->entries[129].effect, &packet->pickup_positions[index - 1], 90, FRAMETIME,
                static_cast<i16>(target_y_rotation), static_cast<i16>(target_z_rotation), NULL);
        }

        const f32 distance = object->apiobj.field_0x1dc * (1.5f + static_cast<f32>(index) * 1.25f);
        const u16 z_rotation = static_cast<u16>(packet->pickup_data[index]);
        const u16 y_rotation = static_cast<u16>(packet->pickup_flags[index]);
        NUVEC position;
        position.x = NU_SIN_LUT(z_rotation) * NU_COS_LUT(y_rotation) * distance + object->apiobj.collision_position.x;
        position.y = NU_SIN_LUT(y_rotation) * distance + object->apiobj.collision_position.y;
        position.z = NU_COS_LUT(z_rotation) * NU_COS_LUT(y_rotation) * distance + object->apiobj.collision_position.z;

        if (packet->field_08 < 0.8f && index + 1 == packet->count) {
            NUVEC direction;
            NuVecSub(&direction, &position, &packet->pickup_positions[index]);
            const f32 scale = 1.0f - (NU_SIN_LUT(packet->field_08 / 0.8f * 32768.0f + 16384.0f) + 1.0f) * 0.5f;
            NuVecScale(&direction, &direction, scale);
            NuVecAdd(&packet->pickup_positions[index], &packet->pickup_positions[index], &direction);
        } else {
            packet->pickup_positions[index] = position;
        }

        if (index == 0) {
            target_z_rotation = NuAngAdd(object->apiobj.field_0x276, NUANG_180DEG);
            target_y_rotation = -object->secondary_lean_angle;
        } else {
            target_z_rotation = static_cast<u16>(packet->pickup_data[index - 1]);
            target_y_rotation = static_cast<u16>(packet->pickup_flags[index - 1]);
        }

        packet->pickup_data[index] = SeekRot(static_cast<u16>(packet->pickup_data[index]), target_z_rotation, 5.0f);
        packet->pickup_flags[index] = SeekRot(static_cast<u16>(packet->pickup_flags[index]), target_y_rotation, 5.0f);

        if (index == 0) {
            AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[129].effect, &object->apiobj.position, 90,
                                              FRAMETIME, static_cast<i16>(packet->pickup_flags[0] - NUANG_90DEG),
                                              static_cast<i16>(packet->pickup_data[0] - NUANG_90DEG), NULL);
        }
    }

    if ((packet->field_0x1 & 0x20) == 0 || packet->count == 0 || object->apiobj.field_0x27c != -1) {
        return;
    }

    i32 player_index = -1;
    for (i32 index = 0; index < 2; ++index) {
        GameObject_s *player = Player[index];
        if (player == NULL || (player->apiobj.flags_high & 0x10) == 0 || player->apiobj.field_0x287 != 0 ||
            static_cast<i8>(player->apiobj.field_0x1f8) >= 0 || (player->field_0xe20 & 0x20) != 0 ||
            player->torpedo->count > 4 || player->torpedo->steal_timer != 0.0f) {
            continue;
        }
        NUVEC distance;
        if (NuVecXZDistSqr(&packet->pickup_positions[packet->count - 1], &player->apiobj.collision_position,
                           &distance) < steal_torpedo_range * steal_torpedo_range) {
            player_index = index;
            break;
        }
    }

    if (player_index != -1) {
        TORPEDOPACKET *player_packet = Player[player_index]->torpedo;
        const i32 destination = player_packet->count;
        const i32 source = packet->count - 1;
        player_packet->pickup_positions[destination] = packet->pickup_positions[source];
        player_packet->pickup_data[destination] = packet->pickup_data[source];
        player_packet->pickup_flags[destination] = packet->pickup_flags[source];
        ++player_packet->count;
        player_packet->field_08 = 0.0f;
        player_packet->field_03 = 1;
        --packet->count;
        player_packet->steal_timer = steal_torpedo_timer;
    }
}

GIZTORPMACHINE *GizTorpMachine_FindNearest(WORLDINFO_s *world, NUVEC *position, f32 *distance_squared) {
    if (world == NULL || world->giz_torp_machine_sys == NULL) {
        return NULL;
    }

    GIZTORPMACHINE *nearest = NULL;
    f32 nearest_distance = 1000000000.0f;
    for (i32 index = 0; index < world->giz_torp_machine_sys->count; ++index) {
        GIZTORPMACHINE *machine = &world->giz_torp_machine_sys->machines[index];
        if ((machine->flags & (GIZTORPMACHINE_FLAG_ACTIVE | GIZTORPMACHINE_FLAG_VISIBLE)) !=
            (GIZTORPMACHINE_FLAG_ACTIVE | GIZTORPMACHINE_FLAG_VISIBLE)) {
            continue;
        }
        const f32 distance = NuVecDistSqr(position, &machine->position, NULL);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest = machine;
        }
    }
    if (distance_squared != NULL) {
        *distance_squared = nearest_distance;
    }
    return nearest;
}

TUBE *Tube_InAnyCylinder(WORLDINFO_s *world, GameObject_s *object, i32 ignore_height) {
    TUBE *tube = world->tubes;
    if (tube != NULL) {
        for (i32 index = 0; index < world->tube_count; ++index, ++tube) {
            if ((tube->flags & (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE | TUBE_FLAG_DIRECTIONAL)) ==
                    (TUBE_FLAG_ACTIVE | TUBE_FLAG_VISIBLE) &&
                Tube_InCylinder(object, tube, NULL, ignore_height) != 0) {
                return tube;
            }
        }
    }
    return NULL;
}
