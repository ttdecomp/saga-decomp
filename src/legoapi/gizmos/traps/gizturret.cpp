#include "legoapi/gizmos/traps/gizturrets.h"
#include "legoapi/world/world.h"
#include "legoapi/legoapi_types.h"

#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/gizmo/base/GizTurretObjectInterface.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include "legoapi/items/collect/bolts.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/misc/utilities.h"
#include "legoapi/render/core/terrain.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numusic/sfx.h"

#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/audio/sfx.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include <stdlib.h>
f32 gizturret_rapid_fire_rate = 0.2f;
i32 gizturret_test_ang = 3640;
#include "nu2api/numath/nurand.h"
#include "legoapi/core/input/qrand.h"

struct GizTurretAnimObjectData {
    u8 flags;
    u8 role;
    i16 platform_id;
};

u32 GizTurrets_TotalScore(void *context) {
    GIZTURRETSYS_s *system = static_cast<WORLDINFO_s *>(context)->giz_turret_sys;
    u32 total = 0;
    if (system != NULL && system->turrets != NULL) {
        GIZTURRET_s *turret = system->turrets;
        for (i32 i = 0; i < system->count; ++i, ++turret)
            total += static_cast<u16>(turret->field_0x10c);
    }
    return total;
}
static void GizTurret_ReadAnimSetData(GAMEANIMOBJ_s *object, unsigned char version) {
    if (version <= 2 || object == NULL) {
        return;
    }

    u8 fallback[2] = {};
    u8 *object_data = static_cast<u8 *>(object->object_data);
    if (object_data == NULL) {
        object_data = fallback;
    }
    object_data[0] = static_cast<u8>(EdFileReadChar());
    object_data[1] = static_cast<u8>(EdFileReadChar());
}

i32 turret_gizmotype_id = -1;

static i32 GizTurrets_GetMaxGizmos(void *turret) {
    WORLDINFO *world = static_cast<WORLDINFO *>(turret);
    if (world == NULL) {
        return 0;
    }
    return world->current_level->max_turrets;
}

static void GizTurrets_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *, void *data) {
    GIZTURRETSYS_s *turret_sys = static_cast<GIZTURRETSYS_s *>(data);
    if (turret_sys != NULL) {
        if (turret_sys->count != 0) {
            i32 i = 0;
            do {
                if (NuStrLen(turret_sys->turrets[i].name) != 0) {
                    AddGizmo(gizmo_sys, type_id, NULL, &turret_sys->turrets[i]);
                }
                ++i;
            } while (turret_sys->count > i);
        }
    }
}

static void GizTurrets_Update(void *context, void *system_ptr, float frame_time) {
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system == NULL)
        return;
    GIZTURRET_s *turret = system->turrets;
    for (i32 index = 0; index < static_cast<u16>(system->count); ++index, ++turret) {
        u8 old_flags = turret->flags;
        u8 rotation_sound_playing = (turret->runtime_flags >> 3) & 1;
        turret->flags &= 0x7f;
        turret->runtime_flags &= ~8;
        if ((old_flags & 6) != 6 || (old_flags & 0x20))
            continue;
        BOLTTYPE_s *bolt_type =
            BoltType_FindByID(static_cast<i8>(turret->bolt_type_id), static_cast<WORLDINFO_s *>(context));
        NUMTX fallback_draw, fallback_base;
        NUMTX *primary_draw, *primary_base, *secondary_draw = NULL, *secondary_base = NULL, *reference;
        i32 base_yaw = 0;
        if (turret->primary_anim_obj == NULL || !NuSpecialExistsFn(&turret->primary_anim_obj->special)) {
            NuMtxSetTranslation(&fallback_draw, &turret->position);
            NuMtxPreRotateX(&fallback_draw, turret->pitch);
            NuMtxPreRotateY(&fallback_draw, turret->yaw);
            NuMtxSetTranslation(&fallback_base, &turret->position);
            NuMtxPreRotateY(&fallback_base, turret->base_y_rotation);
            primary_draw = &fallback_draw;
            primary_base = NULL;
            reference = &fallback_base;
            base_yaw = turret->base_y_rotation;
        } else {
            primary_draw = NuSpecialGetDrawMtx(&turret->primary_anim_obj->special);
            primary_base = NuSpecialGetMtx(&turret->primary_anim_obj->special);
            if (turret->secondary_anim_obj != NULL && NuSpecialExistsFn(&turret->secondary_anim_obj->special)) {
                secondary_base = NuSpecialGetMtx(&turret->secondary_anim_obj->special);
                secondary_draw = NuSpecialGetDrawMtx(&turret->secondary_anim_obj->special);
                reference = secondary_base;
            } else
                reference = primary_base;
        }
        turret->fire_cooldown -= frame_time;
        if (turret->fire_cooldown < 0.0f)
            turret->fire_cooldown = 0.0f;
        i32 desired_pitch = 0, desired_yaw = base_yaw;
        i32 should_fire = 0;
        GameObject_s *autoaim_target = NULL;
        if (turret->controller != NULL) {
            GameObject_s *controller = turret->controller;
            if (controller->field_0xcc0 != NULL && static_cast<i8>(controller->apiobj.field_0x1f8) < 0) {
                GAMEPAD_s *pad = controller->pad_gamepad;
                f32 pitch_rate = -pad->input_direction_x * turret->pitch_turn_speed;
                if (pitch_rate < 0.0f) {
                    desired_pitch = turret->field_0x58;
                    pitch_rate = -pitch_rate;
                } else if (pitch_rate > 0.0f)
                    desired_pitch = turret->field_0x5c;
                else
                    desired_pitch = turret->pitch;
                f32 yaw_rate = -pad->input_direction_z * turret->yaw_turn_speed;
                if (yaw_rate < 0.0f) {
                    desired_yaw = turret->field_0x64;
                    yaw_rate = -yaw_rate;
                } else if (yaw_rate > 0.0f)
                    desired_yaw = turret->field_0x68;
                else
                    desired_yaw = turret->yaw;
                turret->pitch = SeekRot(turret->pitch, desired_pitch, pitch_rate);
                turret->yaw = SeekRot(turret->yaw, desired_yaw, yaw_rate);
                if (turret->fire_cooldown > 0.0f &&
                    (turret->controller->pad_gamepad->buttons_pressed & GAMEPAD_ACTION)) {
                    f32 rapid_interval = turret->fire_interval * gizturret_rapid_fire_rate;
                    if (turret->fire_cooldown > rapid_interval)
                        turret->fire_cooldown = rapid_interval;
                    turret->flags |= 0x40;
                }
                if (turret->fire_cooldown == 0.0f &&
                    ((turret->controller->pad_gamepad->buttons_held & GAMEPAD_ACTION) || (turret->flags & 0x40))) {
                    turret->fire_cooldown = turret->fire_interval;
                    if (turret->flags & 0x40)
                        turret->fire_cooldown *= gizturret_rapid_fire_rate;
                    turret->flags &= ~0x40;
                    if (turret->behavior_flags & 0x200)
                        autoaim_target = GizTurret_GetTgt(turret, primary_draw);
                    should_fire = bolt_type != NULL;
                }
            } else {
                turret->pitch = SeekRot(turret->pitch, 0, turret->pitch_turn_speed);
                turret->yaw = SeekRot(turret->yaw, base_yaw, turret->yaw_turn_speed);
                turret->flags &= ~0x40;
            }
        } else {
            if (turret->flags & 8)
                goto autonomous_apply;
            if (turret->flags & 0x10) {
                desired_pitch = turret->field_0x6c;
                desired_yaw = turret->field_0x70;
                goto autonomous_apply;
            }
            if ((turret->behavior_flags & 0xc0) == 0xc0) {
                if ((turret->behavior_flags & 0x100) && turret->primary_anim_obj != NULL &&
                    NuSpecialExistsFn(&turret->primary_anim_obj->special) &&
                    !NuSpecialGetOnScreenFn(&turret->primary_anim_obj->special))
                    goto autonomous_apply;
                if (turret->fire_cooldown == 0.0f) {
                    turret->fire_cooldown = (NuRandFloat() + 0.5f) * turret->fire_interval;
                    should_fire = bolt_type != NULL;
                }
                goto autonomous_apply;
            }
            // The original performs this initial home seek before the common aiming seek.
            if (turret->field_0xe4 == NULL) {
                turret->pitch = SeekRot(turret->pitch, 0, turret->pitch_turn_speed);
                turret->yaw = SeekRot(turret->yaw, base_yaw, turret->yaw_turn_speed);
                turret->flags &= ~0x40;
                goto autonomous_apply;
            }
            {
                NUVEC *target_position, *target_velocity = NULL;
                if (turret->field_0x12c == 0) {
                    GameObject_s *target = static_cast<GameObject_s *>(turret->field_0xe4);
                    target_position = &target->apiobj.collision_position;
                    target_velocity = &target->apiobj.velocity;
                } else if (turret->field_0x12c == 1) {
                    target_position = reinterpret_cast<NUVEC *>(static_cast<u8 *>(turret->field_0xe4) + 0x11c);
                } else if (turret->field_0x12c == 2)
                    target_position = static_cast<NUVEC *>(turret->field_0xe4);
                else
                    goto autonomous_apply;
                NUVEC local_direction, world_direction;
                f32 distance = NuVecDistSqr(target_position, &turret->field_0x3c, &local_direction);
                should_fire = bolt_type != NULL;
                if ((turret->behavior_flags & 0x200) && target_velocity != NULL && bolt_type != NULL) {
                    GizTurret_CalculateInterceptVector(reinterpret_cast<NUVEC *>(&reference->m30), primary_draw,
                                                       target_position, target_velocity, bolt_type->field_10,
                                                       &world_direction, NULL, turret->controller != NULL);
                    should_fire = true;
                } else
                    NuVecSub(&world_direction, target_position, reinterpret_cast<NUVEC *>(&reference->m30));
                if ((turret->behavior_flags & 0xc0) != 0xc0) {
                    NuVecInvMtxRotate(&local_direction, &world_direction, reference);
                    if (!(turret->behavior_flags & 0x80)) {
                        desired_yaw = NuAngAdd(
                            static_cast<i32>(NuAtan2(-local_direction.x, -local_direction.z) * 10430.3779296875f), 0);
                        if (turret->field_0x64 != 0 && desired_yaw > turret->field_0x64)
                            desired_yaw = turret->field_0x64;
                        else if (turret->field_0x68 != 0 && desired_yaw < turret->field_0x68)
                            desired_yaw = turret->field_0x68;
                    }
                    if (!(turret->behavior_flags & 0x40)) {
                        NuVecRotateY(&local_direction, &local_direction, -desired_yaw);
                        if (secondary_base != NULL) {
                            local_direction.y -= turret->field_0xa4.m31;
                            NuVecInvMtxRotate(&local_direction, &local_direction, &turret->field_0xa4);
                            local_direction.y -= turret->field_0x74[0].y;
                        }
                        desired_pitch =
                            NuAngAdd(static_cast<i32>(
                                         NuAtan2(local_direction.y, NuFsqrt(local_direction.x * local_direction.x +
                                                                            local_direction.z * local_direction.z)) *
                                         10430.3779296875f),
                                     0);
                        if (turret->field_0x58 != 0 && desired_pitch > turret->field_0x58)
                            desired_pitch = turret->field_0x58;
                        else if (turret->field_0x5c != 0 && desired_pitch < turret->field_0x5c)
                            desired_pitch = turret->field_0x5c;
                    }
                }
                if ((turret->behavior_flags & 0x100) && turret->primary_anim_obj != NULL &&
                    NuSpecialExistsFn(&turret->primary_anim_obj->special) &&
                    !NuSpecialGetOnScreenFn(&turret->primary_anim_obj->special))
                    should_fire = false;
                else if (distance < turret->field_0xf0 * turret->field_0xf0 && turret->fire_cooldown == 0.0f)
                    turret->fire_cooldown = (NuRandFloat() + 0.5f) * turret->fire_interval;
                else
                    should_fire = false;
            }
        autonomous_apply:
            if (!(turret->primary_anim_obj != NULL && NuSpecialExistsFn(&turret->primary_anim_obj->special)))
                desired_yaw = NuAngAdd(desired_yaw, turret->base_y_rotation);
            if (turret->behavior_flags & 0x400) {
                turret->pitch = desired_pitch;
                turret->yaw = desired_yaw;
            } else {
                turret->pitch = SeekRot(turret->pitch, desired_pitch, turret->pitch_turn_speed);
                turret->yaw = SeekRot(turret->yaw, desired_yaw, turret->yaw_turn_speed);
            }
        }
        if (turret->primary_anim_obj != NULL) {
            if (NuSpecialExistsFn(&turret->primary_anim_obj->special)) {
                if (secondary_base != NULL) {
                    *secondary_draw = *secondary_base;
                    if (turret->yaw != 0)
                        NuMtxPreRotateY(secondary_draw, turret->yaw);
                    NuMtxMul(primary_draw, &turret->field_0xa4, secondary_draw);
                } else {
                    *primary_draw = *primary_base;
                    if (turret->yaw != 0)
                        NuMtxPreRotateY(primary_draw, turret->yaw);
                }
                if (turret->pitch != 0 && !(turret->behavior_flags & 0x40))
                    NuMtxPreRotateX(primary_draw, turret->pitch);
            }
            if (turret->primary_anim_obj != NULL)
                NuSpecialUpdate(&turret->primary_anim_obj->special);
        }
        if (turret->secondary_anim_obj != NULL)
            NuSpecialUpdate(&turret->secondary_anim_obj->special);
        if (turret->field_0x126 != -1 && player != NULL) {
            bool nearby = WORLD->area != NULL && (WORLD->area->flags & 1);
            if (!nearby) {
                NUVEC delta;
                delta.x = reference->m30 - player->apiobj.collision_position.x;
                delta.y = reference->m31 - player->apiobj.collision_position.y;
                delta.z = reference->m32 - player->apiobj.collision_position.z;
                nearby = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 9.0f &&
                         player->apiobj.model_draw_result != 0;
                if (!nearby && player2 != NULL) {
                    delta.x = reference->m30 - player2->apiobj.collision_position.x;
                    delta.y = reference->m31 - player2->apiobj.collision_position.y;
                    delta.z = reference->m32 - player2->apiobj.collision_position.z;
                    nearby = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z < 9.0f &&
                             player2->apiobj.model_draw_result != 0;
                }
            }
            // These are absolute differences of unsigned angles, not wrapped rotation differences.
            if (nearby && (abs(static_cast<i32>(static_cast<u16>(desired_pitch)) -
                               static_cast<i32>(static_cast<u16>(turret->pitch))) > 0x400 ||
                           abs(static_cast<i32>(static_cast<u16>(desired_yaw)) -
                               static_cast<i32>(static_cast<u16>(turret->yaw))) > 0x400)) {
                if (!rotation_sound_playing || IsSfxLooping(turret->field_0x126))
                    GameAudio_PlaySfxById(turret->field_0x126, reinterpret_cast<NUVEC *>(&reference->m30), 0, 0);
                turret->runtime_flags |= 8;
            }
        }
        if (!should_fire || MiniCutCam != 0 || (turret->behavior_flags & 0x4000))
            continue;
        if (turret->field_0x12a != -1) {
            GameAudio_PlaySfxById(turret->field_0x12a, reinterpret_cast<NUVEC *>(&reference->m30), 0, 0);
            addbolt_nosfx = 1;
        }
        i32 scatter_pitch = 0, scatter_yaw = 0;
        if (bolt_type->field_40 != 0) {
            f32 spread = static_cast<f32>(static_cast<i32>(bolt_type->field_40));
            f32 random = NuRandFloat();
            scatter_pitch = static_cast<i32>(spread - ((random * spread) + (random * spread))) / 2;
            spread = static_cast<f32>(static_cast<i32>(bolt_type->field_40));
            random = NuRandFloat();
            scatter_yaw = static_cast<i32>(spread - ((random * spread) + (random * spread)));
        }
        i32 sound_muzzle = 0;
        if (turret->field_0x130 != 0)
            sound_muzzle = qrand() / (65535 / turret->field_0x130 + 1);
        turret->flags |= 0x80;
        for (i32 muzzle = 0; muzzle < turret->field_0x130; ++muzzle) {
            NUMTX muzzle_matrix = *primary_draw;
            NUVEC *offset = &turret->field_0x74[muzzle];
            if (offset->x != 0.0f || offset->y != 0.0f || offset->z != 0.0f) {
                NuMtxPreRotateY(&muzzle_matrix, 0x8000);
                NuMtxPreTranslate(&muzzle_matrix, offset);
            }
            NUMTX direction_matrix = *primary_draw;
            if (turret->behavior_flags & 0x40)
                NuMtxPreRotateX(&direction_matrix, turret->pitch);
            NuMtxPreRotateY(&direction_matrix, 0x8000);
            if (bolt_type->field_40 != 0) {
                NuMtxPreRotateX(&direction_matrix, scatter_pitch);
                NuMtxPreRotateY(&direction_matrix, scatter_yaw);
            }
            if (autoaim_target != NULL) {
                NUVEC direction;
                GizTurret_CalculateInterceptVector(reinterpret_cast<NUVEC *>(&muzzle_matrix.m30), primary_draw,
                                                   &autoaim_target->apiobj.collision_position,
                                                   &autoaim_target->apiobj.velocity, bolt_type->field_10, &direction,
                                                   NULL, turret->controller != NULL);
                FindAnglesXY(&direction, NULL, NULL);
                NUANGVEC angles;
                angles.x = temp_xrot;
                angles.y = temp_yrot;
                NuMtxSetRotationXYVU0(&direction_matrix, &angles);
            }
            i32 flags = turret->controller != NULL ? 4 : 2;
            if (muzzle != sound_muzzle)
                addbolt_nosfx = 1;
            BOLT_s *bolt = Bolt_Add(NULL, reinterpret_cast<NUVEC *>(&muzzle_matrix.m30), &direction_matrix,
                                    static_cast<i8>(turret->bolt_type_id), flags);
            if (bolt != NULL && (turret->behavior_flags & 0x8000))
                bolt->flags |= 0x10;
        }
    }
}

static void GizTurrets_Draw(void *world_ptr, void *system_ptr, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system == NULL || system->count == 0) {
        return;
    }

    for (i32 index = 0; index < system->count; ++index) {
        GIZTURRET_s &turret = system->turrets[index];
        if ((turret.flags & GIZTURRET_FLAG_VISIBLE) == 0) {
            continue;
        }
        if (turret.room_id >= 0 && world->rooms_visible_ptr[turret.room_id] == 0) {
            continue;
        }
        if ((turret.animation_flags & GIZTURRET_ANIMATION_FLAG_DRAW_REFLECTION) == 0) {
            continue;
        }
        GameAnimSet_DrawReflection(turret.anim_set, 2, turret.reflection_alpha, NULL);
    }
}

static char *GizmoTurret_GetGizmoName(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
        return turret->name;
    }
    return NULL;
}

static i32 GizmoTurret_GetOutput(GIZMO *gizmo, i32 output_index, i32) {
    GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
    switch (output_index) {
        case 0:
            return (turret->flags & 0x30) != 0;
        case 1:
            return static_cast<i8>(turret->flags) < 0;
        case 2:
            return turret->field_0x132[0] >= turret->field_0x131;
        default:
            return 0;
    }
}

static char *GizmoTurret_GetOutputName(GIZMO *gizmo, i32 output_index) {
    static char str[32];
    switch (output_index) {
        case 0:
            return const_cast<char *>("destroyed");
        case 1:
            return const_cast<char *>("fired");
        case 2: {
            GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
            if (turret == NULL) {
                return NULL;
            }
            sprintf(str, "Fired %d Shots", turret->field_0x131);
            return str;
        }
        default:
            return NULL;
    }
}

static i32 GizmoTurret_GetNumOutputs(GIZMO *) {
    return 3;
}

static void GizmoTurret_Activate(GIZMO *gizmo, i32 active) {
    if (gizmo == NULL) {
        return;
    }
    GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
    u8 active_flag = active != 0;
    active_flag += active_flag;
    turret->flags = static_cast<u8>((turret->flags & ~GIZTURRET_FLAG_ACTIVE) | active_flag);
    if ((turret->flags & GIZTURRET_FLAG_ACTIVE) != 0) {
        turret->flags &= ~0x30;
        turret->field_0x132[0] = 0;
    }
}

static i32 GizmoTurret_ActivateRev(GIZMO *gizmo, i32 active, i32 reverse) {
    if (gizmo == NULL) {
        return 0;
    }
    GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
    if (turret == NULL) {
        return 0;
    }
    if ((reverse & 1) != 0) {
        if ((turret->flags & GIZTURRET_FLAG_ACTIVE) != 0) {
            return 0;
        }
        turret->flags &= ~0x10;
        return active == 0;
    }
    u8 active_flag = active == 0;
    active_flag += active_flag;
    turret->flags = static_cast<u8>((turret->flags & ~GIZTURRET_FLAG_ACTIVE) | active_flag);
    return 1;
}

static void GizmoTurret_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo == NULL) {
        return;
    }
    GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
    if (turret == NULL) {
        return;
    }
    GameAnimSet_SetVisibility(turret->anim_set, visible);
    u8 visibility_flag = visible != 0;
    visibility_flag <<= 2;
    turret->flags = static_cast<u8>((turret->flags & ~GIZTURRET_FLAG_VISIBLE) | visibility_flag);
}

static NUVEC *GizmoTurret_GetPos(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZTURRET_s *turret = static_cast<GIZTURRET_s *>(gizmo->object);
        if (turret != NULL) {
            return &turret->position;
        }
    }
    return NULL;
}

static i32 GizTurrets_BoltHitPlat(void *world_ptr, void *system_ptr, BOLT *bolt, unsigned char *) {
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system == NULL || system->count == 0) {
        return 0;
    }

    GIZTURRET_s *turret = system->turrets;
    for (i32 index = 0; index < system->count; ++index, ++turret) {
        if ((turret->flags & GIZTURRET_FLAG_VISIBLE) == 0 || (turret->flags & GIZTURRET_FLAG_ACTIVE) == 0 ||
            (turret->flags & 0x30) != 0 || (turret->runtime_flags & 2) == 0) {
            continue;
        }

        GAMEANIMOBJ_s *object = turret->anim_set->objects;
        while (object != NULL) {
            GizTurretAnimObjectData *data = static_cast<GizTurretAnimObjectData *>(object->object_data);
            if (data->platform_id == bolt->hit_platform) {
                BOLTTYPE_s *bolt_type = BoltType_FindByID(bolt->type_id, static_cast<WORLDINFO_s *>(world_ptr));
                i32 player_index;
                if (bolt->owner == NULL) {
                    player_index = -1;
                } else if (((turret->behavior_flags & 0x10000) != 0 && bolt->owner->field_0xcc0 == NULL) ||
                           ((turret->behavior_flags & 0x20000) != 0 &&
                            static_cast<i8>(bolt->owner->apiobj.field_0x1f8) >= 0)) {
                    player_index = 0;
                } else {
                    player_index = bolt->owner->apiobj.field_0x27c;
                }
                GizTurrets_Hit(world_ptr, turret, &bolt->position, player_index, bolt_type->field_3c);
                return 1;
            }
            object = object->next;
        }
    }
    return 0;
}

static i32 *GizTurrets_GetBestBoltTarget(GIZMOSET *, float *, NUVEC *, NUVEC *, void *, NUVEC *, NUVEC *, float, float,
                                         i32, i32, i32) {
    UNIMPLEMENTED();
    return {};
}

static i32 GizTurrets_BoltHit(void *, void *, void *, NUVEC *, i32, float, NUVEC *, NUVEC *, BOLT *, u32,
                              unsigned char *) {
    UNIMPLEMENTED();
    return {};
}

static void *GizTurrets_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, 0x70);
}

static void GizTurrets_ClearProgress(void *, void *progress_ptr) {
    u32 *progress = static_cast<u32 *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    progress[0] = 0xffffffff;
    progress[1] = 0xffffffff;
    progress[2] = 0xffffffff;
    progress[3] = 0xffffffff;
    progress[4] = 0;
    progress[5] = 0;
    progress[6] = 0;
    progress[7] = 0;
    progress[8] = 0;
    progress[9] = 0;
    progress[10] = 0;
    progress[11] = 0;
    progress[12] = 0xffffffff;
    progress[13] = 0xffffffff;
    progress[14] = 0xffffffff;
    progress[15] = 0xffffffff;
    progress[16] = 0xffffffff;
    progress[17] = 0xffffffff;
    progress[18] = 0xffffffff;
    progress[19] = 0xffffffff;
    progress[20] = 0xffffffff;
    progress[21] = 0xffffffff;
    progress[22] = 0xffffffff;
    progress[23] = 0xffffffff;
    progress[24] = 0xffffffff;
    progress[25] = 0xffffffff;
    progress[26] = 0xffffffff;
    progress[27] = 0xffffffff;
}

static void GizTurrets_StoreProgress(void *, void *system_ptr, void *progress_ptr) {
    if (progress_ptr == NULL) {
        return;
    }

    GizTurrets_ClearProgress(NULL, progress_ptr);
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system->count == 0) {
        return;
    }

    u32 *progress = static_cast<u32 *>(progress_ptr);
    GIZTURRET_s *turret = system->turrets;
    for (i32 index = 0; index < system->count && index != 64; ++index, ++turret) {
        const i32 word = index >> 5;
        const u32 mask = 1u << index;
        if ((turret->flags & GIZTURRET_FLAG_VISIBLE) == 0) {
            progress[word + 2] &= ~mask;
        }
        if ((turret->flags & GIZTURRET_FLAG_ACTIVE) == 0) {
            progress[word] &= ~mask;
        }
        if ((turret->flags & 0x08) != 0) {
            progress[word + 4] |= mask;
        }
        if ((turret->flags & 0x10) != 0) {
            progress[word + 6] |= mask;
        }
        if ((turret->flags & GIZTURRET_FLAG_UPDATE_DISABLED) != 0) {
            progress[word + 8] |= mask;
        }
        if ((turret->runtime_flags & 0x04) != 0) {
            progress[word + 10] |= mask;
        }
        static_cast<u8 *>(progress_ptr)[0x30 + index] = turret->field_0x12e;
    }
}

static void GizTurrets_Reset(void *world_ptr, void *system_ptr, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system->count == 0) {
        return;
    }

    GIZTURRET_s *turret = system->turrets;
    const i32 has_progress = progress_ptr != NULL;
    for (i32 index = 0; index < system->count; ++index, ++turret) {
        turret->field_0x12e = turret->field_0x12f;
        turret->flags |= GIZTURRET_FLAG_ACTIVE | GIZTURRET_FLAG_VISIBLE;
        turret->field_0x132[0] = 0;
        turret->primary_anim_obj = NULL;
        turret->secondary_anim_obj = NULL;

        if (turret->anim_set != NULL) {
            GAMEANIMOBJ_s *object = turret->anim_set->objects;
            while (object != NULL) {
                u8 *object_data = static_cast<u8 *>(object->object_data);
                if (object_data[1] == 1) {
                    turret->primary_anim_obj = object;
                    object_data[2] = 0xff;
                    if (world->terrain != NULL && NuSpecialExistsFn(&object->special) != 0) {
                        object_data[2] = static_cast<u8>(FindPlatInst(NuSpecialGetInstanceix(&object->special)));
                        if (object_data[2] != 0xff) {
                            turret->runtime_flags |= 2;
                        }
                    }
                } else if (object_data[1] == 2) {
                    turret->secondary_anim_obj = object;
                    object_data[2] = 0xff;
                }
                object = object->next;
            }
        }

        turret->room_id = world->current_gscn != NULL
                              ? static_cast<i16>(NuPortalWhichRoom(world->current_gscn, &turret->position))
                              : -1;
        if ((turret->behavior_flags & 2) != 0) {
            turret->field_0x3c = turret->position;
            GameAnimSet_GetAveragePos(turret->anim_set, &turret->field_0x30, 0, 1, 1);
            turret->field_0x3c = turret->field_0x30;
        }
        if ((turret->behavior_flags & 0x800) != 0) {
            turret->field_0xf0 = turret->field_0xec;
            turret->field_0x3c = turret->field_0x30;
        }
        GameAnimSet_EvaluateState(turret->anim_set);

        if (turret->primary_anim_obj != NULL && turret->secondary_anim_obj != NULL &&
            NuSpecialExistsFn(&turret->primary_anim_obj->special) != 0 &&
            NuSpecialExistsFn(&turret->secondary_anim_obj->special) != 0) {
            NUMTX *primary_mtx = NuSpecialGetMtx(&turret->primary_anim_obj->special);
            NUMTX *secondary_mtx = NuSpecialGetMtx(&turret->secondary_anim_obj->special);
            NuMtxInv(&turret->field_0xa4, primary_mtx);
            NuMtxMul(&turret->field_0xa4, secondary_mtx, &turret->field_0xa4);
        }

        if (index <= 0x3f && has_progress != 0) {
            u32 *progress = static_cast<u32 *>(progress_ptr);
            const i32 word = index >> 5;
            const u32 bit = 1u << (index & 31);
            turret->flags = static_cast<u8>((turret->flags & ~GIZTURRET_FLAG_VISIBLE) |
                                            ((progress[word + 2] & bit) != 0 ? GIZTURRET_FLAG_VISIBLE : 0));
            turret->flags = static_cast<u8>((turret->flags & ~GIZTURRET_FLAG_ACTIVE) |
                                            ((progress[word] & bit) != 0 ? GIZTURRET_FLAG_ACTIVE : 0));
            turret->flags = static_cast<u8>((turret->flags & ~0x8) | ((progress[word + 4] & bit) != 0 ? 0x8 : 0));
            turret->flags = static_cast<u8>((turret->flags & ~0x10) | ((progress[word + 6] & bit) != 0 ? 0x10 : 0));
            turret->flags = static_cast<u8>((turret->flags & ~0x20) | ((progress[word + 8] & bit) != 0 ? 0x20 : 0));
            turret->runtime_flags =
                static_cast<u8>((turret->runtime_flags & ~4) | ((progress[word + 10] & bit) != 0 ? 4 : 0));
            if (static_cast<u8 *>(progress_ptr)[index + 0x30] != 0xff) {
                turret->field_0x12e = static_cast<u8 *>(progress_ptr)[index + 0x30];
            }
        }

        if (turret->anim_set != NULL) {
            for (GAMEANIMOBJ_s *object = turret->anim_set->objects; object != NULL; object = object->next) {
                u8 *object_data = static_cast<u8 *>(object->object_data);
                if (object_data[1] == 3) {
                    NuSpecialSetVisibility(&object->special, (turret->flags >> 5) & 1);
                }
            }
        }
    }
}

static void *GizTurrets_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZTURRETSYS_s *turret_sys = static_cast<GIZTURRETSYS_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, sizeof(GIZTURRETSYS_s)));

    turret_sys->capacity = world->current_level->max_turrets;
    turret_sys->turrets = static_cast<GIZTURRET_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, turret_sys->capacity * sizeof(GIZTURRET_s)));
    turret_sys->anim_pool =
        GameAnimSet_CreateObjectPool(&world->giz_buffer, &world->unknown_0108, 4, turret_sys->capacity * 2);

    for (i32 i = 0; i < turret_sys->capacity; ++i) {
        turret_sys->turrets[i].anim_set =
            GameAnimSet_Create(&world->giz_buffer, &world->unknown_0108, turret_sys->anim_pool, world->game_anim_sys);
    }
    world->giz_turret_sys = turret_sys;
    return turret_sys;
}

static i32 GizTurrets_Load(void *world_ptr, void *system_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    const unsigned char version = static_cast<unsigned char>(EdFileReadChar());
    system->count = static_cast<u16>(EdFileReadShort());
    if (system->count == 0) {
        return 1;
    }

    GIZTURRET_s *turret = system->turrets;
    for (i32 index = 0; index < system->count; ++index, ++turret) {
        GAMEANIMSET_s *anim_set = turret->anim_set;
        turret->ClearMechObjectInterface();
        memset(turret, 0, sizeof(*turret));
        turret->anim_set = anim_set;
        turret->flags &= static_cast<u8>(~1);
        turret->runtime_flags &= static_cast<u8>(~GIZTURRET_RUNTIME_FLAG_ROTATION_SFX_PLAYING);
        turret->fire_interval = 1.0f;
        turret->pitch_turn_speed = 8.0f;
        turret->yaw_turn_speed = 8.0f;
        turret->field_0x140 = 0.2f;
        turret->behavior_flags |= 0x802;
        turret->field_0x12f = 1;
        turret->field_0x131 = 1;
        turret->field_0x12a = -1;
        turret->field_0x126 = -1;
        turret->field_0x138 = -1;
        turret->field_0x130 = 1;
        turret->blowup_type = -1;
        turret->bolt_type_id = -1;
        turret->field_0x120 = (WORLD->area != NULL && (WORLD->area->flags & 1) != 0) ? 12.0f : 1.75f;

        EdFileRead(turret->name, sizeof(turret->name));
        GizmoFileReadGameAnimSet(turret->anim_set, world_ptr, GizTurret_ReadAnimSetData, version,
                                 const_cast<char *>("GizTurret"), turret->name);
        EdFileReadNuVec(&turret->position);
        EdFileReadNuVec(&turret->field_0x30);
        EdFileReadNuVec(&turret->field_0x3c);
        EdFileReadNuVec(&turret->field_0x48);
        turret->field_0x58 = EdFileReadInt();
        turret->field_0x5c = EdFileReadInt();
        turret->field_0x64 = EdFileReadInt();
        turret->field_0x68 = EdFileReadInt();
        turret->field_0x6c = EdFileReadInt();
        turret->field_0x70 = EdFileReadInt();
        if (version > 1) {
            turret->behavior_flags = static_cast<u32>(EdFileReadInt());
        }
        turret->field_0x130 = static_cast<u8>(EdFileReadChar());
        for (i32 vector_index = 0; vector_index < turret->field_0x130; ++vector_index) {
            EdFileReadNuVec(&turret->field_0x74[vector_index]);
        }
        turret->field_0xec = EdFileReadFloat();
        turret->field_0xf0 = EdFileReadFloat();
        turret->reflection_alpha = EdFileReadFloat();
        turret->fire_interval = EdFileReadFloat();
        turret->pitch_turn_speed = EdFileReadFloat();
        turret->yaw_turn_speed = EdFileReadFloat();
        turret->field_0x10c = static_cast<i16>(EdFileReadShort());
        turret->field_0x110 = static_cast<i16>(EdFileReadShort());
        turret->field_0x112 = static_cast<i16>(EdFileReadShort());
        EdFileReadNuVec(&turret->field_0x114);

        char name[256];
        if (version > 5) {
            turret->field_0x120 = EdFileReadFloat();
            turret->field_0x12f = static_cast<u8>(EdFileReadChar());
        } else {
            turret->field_0x12f = static_cast<u8>(EdFileReadChar());
        }
        if (version > 3) {
            turret->field_0x131 = static_cast<u8>(EdFileReadChar());
            turret->base_y_rotation = static_cast<i16>(EdFileReadShort());
            if (GizmoFileReadName(name) != 0) {
                turret->bolt_type_id = static_cast<i8>(BoltType_FindIDByName(name, world));
            }
        } else {
            if (GizmoFileReadName(name) != 0) {
                turret->bolt_type_id = static_cast<i8>(BoltType_FindIDByName(name, world));
            }
        }
        if (GizmoFileReadName(name) != 0) {
            turret->field_0x12a = static_cast<i16>(GetSfxId(name));
        }
        if (GizmoFileReadName(name) != 0) {
            turret->field_0x126 = static_cast<i16>(GetSfxId(name));
        }
        if (version > 6) {
            // One optional sound name, not a zero-terminated list. Reading
            // again consumes the blowup name and shifts the next turret.
            if (GizmoFileReadName(name) != 0) {
                turret->field_0x138 = static_cast<i16>(GetSfxId(name));
            }
        }
        if (GizmoFileReadName(name) != 0) {
            turret->blowup_type = static_cast<i16>(GizmoBlowupGetNameTableId(name));
            if (turret->blowup_type != -1) {
                turret->runtime_flags |= GIZTURRET_RUNTIME_FLAG_BLOWUP_NAME_ID;
            }
        }
        turret->field_0x134 = static_cast<i16>(EdFileReadShort());
        if (version <= 4 && turret->anim_set != NULL) {
            i32 object_index = 0;
            for (GAMEANIMOBJ_s *object = turret->anim_set->objects; object != NULL;
                 object = object->next, ++object_index) {
                u8 *object_data = static_cast<u8 *>(object->object_data);
                if (object_index == 0) {
                    object_data[1] = 1;
                } else if (object_index == 1) {
                    object_data[1] = 2;
                } else {
                    object_data[1] = (object_data[0] & 1) != 0 ? 3 : 1;
                }
            }
        }
    }
    return 1;
}

static void GizTurrets_PostLoad(void *world_ptr, void *system_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system != NULL) {
        if (system->count != 0) {
            GIZTURRET_s *turret = system->turrets;
            i32 index = 0;
            do {
                if ((turret->runtime_flags & GIZTURRET_RUNTIME_FLAG_BLOWUP_NAME_ID) != 0) {
                    turret->blowup_type =
                        static_cast<i16>(GizmoBlowupGetTypeFromNameTableId(world, turret->blowup_type));
                    turret->runtime_flags &= ~GIZTURRET_RUNTIME_FLAG_BLOWUP_NAME_ID;
                }
                ++index;
                ++turret;
            } while (system->count > index);
        }
    }
}

static void GizTurrets_AddLevelSfx(void *, void *system_ptr, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx) {
    GIZTURRETSYS_s *system = static_cast<GIZTURRETSYS_s *>(system_ptr);
    if (system == NULL || system->count == 0) {
        return;
    }
    GIZTURRET_s *turret = system->turrets;
    for (i32 index = 0; index < system->count; ++index, ++turret) {
        if (turret->field_0x12a != -1) {
            AddLevelSfxFromId(turret->field_0x12a, sfx_ids, sfx_count, max_sfx);
        }
        if (turret->field_0x126 != -1) {
            AddLevelSfxFromId(turret->field_0x126, sfx_ids, sfx_count, max_sfx);
        }
        if (turret->field_0x138 != -1) {
            AddLevelSfxFromId(turret->field_0x138, sfx_ids, sfx_count, max_sfx);
        }
    }
}

ADDGIZMOTYPE *GizTurrets_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizTurret";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0x70;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizTurrets_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizmoTurret_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizTurrets_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = GizTurrets_BoltHitPlat;
    addtype.fns.get_best_bolt_target_fn = GizTurrets_GetBestBoltTarget;
    addtype.fns.late_update_fn = GizTurrets_Update;
    addtype.fns.bolt_hit_fn = GizTurrets_BoltHit;
    addtype.fns.draw_fn = GizTurrets_Draw;
    addtype.fns.get_gizmo_name_fn = GizmoTurret_GetGizmoName;
    addtype.fns.get_output_fn = GizmoTurret_GetOutput;
    addtype.fns.get_output_name_fn = GizmoTurret_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizmoTurret_GetNumOutputs;
    addtype.fns.activate_fn = GizmoTurret_Activate;
    addtype.fns.activate_rev_fn = GizmoTurret_ActivateRev;
    addtype.fns.set_visibility_fn = GizmoTurret_SetVisibility;
    addtype.fns.allocate_progress_data_fn = GizTurrets_AllocateProgressData;
    addtype.fns.clear_progress_fn = GizTurrets_ClearProgress;
    addtype.fns.store_progress_fn = GizTurrets_StoreProgress;
    addtype.fns.reset_fn = GizTurrets_Reset;
    addtype.fns.reserve_buffer_space_fn = GizTurrets_ReserveBufferSpace;
    addtype.fns.load_fn = GizTurrets_Load;
    addtype.fns.post_load_fn = GizTurrets_PostLoad;
    addtype.fns.add_level_sfx_fn = GizTurrets_AddLevelSfx;
    turret_gizmotype_id = type_id;

    return &addtype;
}

void GizTurrets_Hit(void *, GIZTURRET_s *, nuvec_s *, i32, i32) {
}

GameObject_s *GizTurret_GetTgt(GIZTURRET_s *, numtx_s *matrix) {
    NUVEC forward = {0.0f, 0.0f, -1.0f};
    if (WORLD != NULL && player != NULL && WORLD->current_level == DEATHSTARRESCUEE_LDATA &&
        player->id == id_GRABCONTROL && TouchHacks::TouchControlsActive) {
        return NULL;
    }

    NuVecMtxRotate(&forward, &forward, matrix);
    GameObject_s *best = NULL;
    f32 best_distance = 1000000000.0f;
    GameObject_s *object = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
            (object->apiobj.field_0x1f4 & 1) == 0 || object->apiobj.model_draw_result == 0) {
            continue;
        }

        NUVEC direction;
        const f32 distance =
            NuVecDistSqr(&object->apiobj.collision_position, reinterpret_cast<NUVEC *>(&matrix->m30), &direction);
        if (best_distance > distance) {
            const f32 length = NuFsqrt(distance);
            const f32 scale = length > 0.0f ? 1.0f / length : 0.0f;
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
            if (NuVecDot(&direction, &forward) <= NU_COS_LUT(gizturret_test_ang)) {
                continue;
            }
            best = object;
            best_distance = distance;
        }
    }
    return best;
}

GIZTURRET_s *GizTurret_FindByName(GIZTURRETSYS_s *system, char *name) {
    if (name == NULL || system == NULL || system->count == 0) {
        return NULL;
    }

    GIZTURRET_s *turret = system->turrets;
    i32 i = 0;
    do {
        if (NuStrICmp(turret->name, name) == 0) {
            return turret;
        }
        ++i;
        ++turret;
    } while (system->count > i);
    return NULL;
}

GIZTURRET_s *GizTurret_FindNearest(GIZTURRETSYS_s *system, nuvec_s *position, GameObject_s *, f32 *distance, i32) {
    if (system == NULL) {
        return NULL;
    }

    GIZTURRET_s *nearest = NULL;
    f32 nearest_distance = 1000000000.0f;
    GIZTURRET_s *turret = system->turrets;
    for (i32 i = 0; i < system->count; ++i, ++turret) {
        if ((turret->flags & 4) != 0 && (turret->flags & 2) != 0) {
            const f32 current_distance = NuVecDistSqr(position, &turret->position, NULL);
            if (current_distance < nearest_distance) {
                nearest = turret;
                nearest_distance = current_distance;
            }
        }
    }
    if (distance != NULL) {
        *distance = nearest_distance;
    }
    return nearest;
}

i32 GizTurrets_UpdateHint(HINT_s *) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    GIZTURRETSYS_s *system = world->giz_turret_sys;
    if ((world->area != NULL && world->area == HUB_ADATA) || VehicleArea != 0 || system == NULL) {
        return 0;
    }

    if (system->count == 0) {
        return 0;
    }

    GIZTURRET_s *turret = system->turrets;
    i32 i = 0;
    i32 result = 0;
    do {
        if ((turret->flags & 6) == 6) {
            if ((turret->behavior_flags & 0x4010) == 0x4000) {
                if (turret->field_0xe4 != NULL) {
                    if (36.0f > NuVecDistSqr(&GameCam->pos, &turret->position, NULL)) {
                        result = 1;
                        break;
                    }
                }
            }
        }
        ++i;
        ++turret;
    } while (system->count > i);
    return result;
}

GIZTURRET_s *GizTurret_FindByController(GIZTURRETSYS_s *system, GameObject_s &controller) {
    if (system != NULL) {
        const i32 count = system->count;
        GIZTURRET_s *turret = system->turrets;
        if (count != 0) {
            for (i32 i = 0; i < count; ++i, ++turret) {
                if (turret->controller == &controller) {
                    return turret;
                }
            }
        }
    }
    return NULL;
}

void GizTurrets_OpponentSelection(GIZTURRETSYS_s *, i32, APIOBJECT_s **, i32, APIOBJECT_s **) {
}

void GizTurret_CalculateInterceptVector(nuvec_s *origin, numtx_s *matrix, nuvec_s *target, nuvec_s *velocity, f32 speed,
                                        nuvec_s *intercept_out, nuvec_s *velocity_out, u32 fallback) {
    NUVEC forward = {0.0f, 0.0f, -1.0f};
    NuVecMtxRotate(&forward, &forward, matrix);

    NUVEC intercept;
    NUVEC intercept_velocity;
    CalculateInterceptVector(origin, target, velocity, speed, &intercept, &intercept_velocity);

    NUVEC direction;
    const f32 distance = NuVecDistSqr(&intercept, origin, &direction);
    const f32 length = NuFsqrt(distance);
    const f32 scale = length > 0.0f ? 1.0f / length : 0.0f;
    direction.x *= scale;
    direction.y *= scale;
    direction.z *= scale;

    if (NuVecDot(&direction, &forward) > NU_COS_LUT(gizturret_test_ang) || fallback == 0) {
        if (intercept_out != NULL) {
            *intercept_out = intercept;
        }
        if (velocity_out != NULL) {
            *velocity_out = intercept_velocity;
        }
    } else {
        if (intercept_out != NULL) {
            *intercept_out = forward;
        }
        if (velocity_out != NULL) {
            *velocity_out = *origin;
        }
    }
}

void GIZTURRET_s::ClearMechObjectInterface() {
    if (mech_object_interface != NULL) {
        delete mech_object_interface;
    }
}

MechObjectInterface *GIZTURRET_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL) {
        new GizTurretObjectInterface(*this);
    }
    return mech_object_interface;
}
