#include "decomp.h"
#include "batman.h"
#include "globals.h"
#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nurand.h"
#include "legoapi/core/input/qrand.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

static char gizmoblowupnametable[32][32];
static i32 gizmoblowupnametable_numids;
i32 GizmoBlowup_HitMultiplier = 1;
void (*CheckLostDataFn)(GIZMOBLOWUP_s *) = NULL;

void GizBlowup_DeleteTerrain();
void GizmoBlowupUpdateMatrix(GIZMOBLOWUP_s *blowup);
void GizmoBlowupCreateStuff(GIZMOBLOWUP_s *blowup);
void GizmoBlowUp_AddEffects(NUVEC *position, GIZMOBLOWUP_s *blowup, i32 count, i32 flags, GameObject_s *object);
i32 ObjHitObj(GameObject_s *attacker, GameObject_s *target, i32 type, u16 damage, i32 flags, i32 context);
void GameAntinode_UnregisterAntiNode(GAMEANTINODESYS_s *system, GAMEANTINODE_s *node);
extern "C" void PlatOnOff(i32 platform, i32 enabled);
extern "C" i32 NewPlatInst(void *object, i32 instance);
extern "C" i32 FindPlatInst(i32 instance);
extern "C" i32 DeletePlatinst(i32 platform);
extern "C" void PlatInstRotate(i32 platform, i32 rotate);
i32 GizBlowup_InitSingleTerrain(GIZMOBLOWUP_s *blowup);
void GizBlowup_DeleteSingleTerrain(GIZMOBLOWUP_s *blowup);

void GizmoBlowupGenDecalMatrix(GIZMOBLOWUP_s *, NUMTX *, i32);
void GizmoBlowupGenShadowMatrix(GIZMOBLOWUP_s *, NUMTX *);

void GizmoBlowupDraw(void *world_ptr, void *, float) {
    WORLDINFO_s *world = static_cast<WORLDINFO_s *>(world_ptr);
    GIZMOBLOWUP_s *blowup = world->gizmo_blowups;
    NUMTX_ALIGNED16 matrix;
    for (i32 index = 0; index < world->gizmo_blowup_count; ++index, ++blowup) {
        if ((blowup->visibility_flags & 0x40) != 0) {
            i32 drawn = NuSpecialDrawAt(&blowup->type->animated_special, &blowup->transform);
            blowup->visibility_flags = (blowup->visibility_flags & 0x7f) | (drawn << 7);
            if ((blowup->visibility_flags & 0x80) != 0) {
                NuCameraTransformScreenClip(&blowup->screen_position, &blowup->position, 1, NULL);
            }
            if ((blowup->status_flags & 0x808000) == 0x808000 && (blowup->draw_flags & 0x10) != 0 &&
                GizmoBlowup_TransformDrawFn != NULL) {
                GizmoBlowup_TransformDrawFn(blowup);
            }
        } else {
            blowup->visibility_flags &= 0x7f;
            if ((blowup->output_flags & 1) != 0) {
                if ((blowup->draw_flags & 0x400) == 0) {
                    GizmoBlowupGenDecalMatrix(blowup, &matrix, 0);
                    NuSpecialDrawAt(&blowup->type->decal_special, &matrix);
                }
                if (NuSpecialExistsFn(&blowup->type->burst_special)) {
                    NuSpecialDrawAt(&blowup->type->burst_special, &blowup->transform);
                }
            }
        }
        if ((blowup->state_flags & 0x80) != 0 && (blowup->draw_flags & 0x20000) == 0 &&
            NuSpecialExistsFn(&blowup->type->shadow_special)) {
            GizmoBlowupGenShadowMatrix(blowup, &matrix);
            NuSpecialDrawAt(&blowup->type->shadow_special, &blowup->transform);
        }
    }
}

void GizmoSortBlowups(WORLDINFO_s *) {
}

void GizmoSwapBlowups(GIZMOBLOWUP_s *, GIZMOBLOWUP_s *) {
}

i32 MAXBLOWUPRANDSPANG = 60;
void (*GizmoBlowUp_SfxFn)(GIZMOBLOWUP_s *, NUVEC *) = NULL;
extern NUVEC v010;
extern ADDPART_s Default_ADDPART;
extern "C" void AddFiniteShotDebrisEffect(i32 *, i32, NUVEC *, i32);
extern "C" i32 AddFiniteShotPART(i32, NUVEC *, i32);
extern "C" PART_s *AddPart(ADDPART_s *);
i32 PartDraw_Flickerer(PART_s *);
void PartStop_Flickerer(PART_s *);
void GameCam_NewShake(GAMECAMERA_s *, f32, f32, f32);
void NewRumbleAllPlayers(f32, f32, i32, i32);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
i32 ReleaseHearts();
void AddPickups(i32, i32, i32, i32, NUVEC *, NUVEC *, f32, i32, f32, f32, GameObject_s *, i32, i32, bool);
EXPLOSION *AddExplosion(NUVEC *, f32, f32, GameObject_s *, i32, i32);

void GizmoBlowUp_AddEffects(nuvec_s *position, GIZMOBLOWUP_s *blowup, i32 offset, i32 flags, GameObject_s *object) {
    GIZMOBLOWUPTYPE_s *type = blowup->type;
    NUVEC effect_position = *position;
    if (offset != 0) {
        effect_position.x += blowup->field_0x80;
        effect_position.y += blowup->field_0x84;
        effect_position.z += blowup->field_0x88;
    }
    if ((flags & 1) != 0) {
        for (i32 i = 4; i < 7; ++i) {
            if (type->particle_types[i] != -1) {
                i32 key = -1;
                AddFiniteShotDebrisEffect(&key, type->particle_types[i], &effect_position, 1);
            }
        }
    }
    if ((flags & 2) != 0) {
        for (i32 i = 0; i < 2; ++i) {
            if (type->particle_types[i] != -1) {
                AddFiniteShotPART(type->particle_types[i], &effect_position, 1);
            }
        }
    }
    if ((flags & 4) != 0) {
        i32 count = static_cast<i8>(blowup->field_0x115) * blowup->field_0xa8;
        NUVEC direction = v010;
        NuVecRotateX(&direction, &direction, static_cast<u16>(blowup->field_0xf6));
        NuVecRotateY(&direction, &direction, static_cast<u16>(blowup->field_0xf8));
        i32 pickup_flag = (blowup->draw_flags >> 8) & 1;
        i32 hearts = count > 0 ? ReleaseHearts() : 0;
        AddPickups(count, hearts, 0, pickup_flag, position, &direction, 0.0f, -1, 1.5f, 2000000.0f, NULL, 1, 1, true);
    }
    if ((flags & 0x10) != 0) {
        GameCam_NewShake(GameCam, 1.0f, 1.0f, 1.0f);
        NewRumbleAllPlayers(1.0f, 0.1f, 0, 0);
    }
    if (static_cast<i32>(blowup->draw_flags) < 0 || static_cast<i8>(blowup->saved_state_1) <= 0) {
        for (i32 i = 0; i < 4; ++i) {
            if (!NuSpecialExistsFn(&type->alternate_specials[i]))
                continue;
            for (i32 j = 0; j < type->field_0xfa; ++j) {
                NUMTX_ALIGNED16 matrix;
                NuMtxSetIdentity(&matrix);
                matrix.m30 = position->x;
                matrix.m31 = position->y;
                matrix.m32 = position->z;
                NUVEC velocity = v010;
                f32 range = (MAXBLOWUPRANDSPANG << 16) / 360;
                i32 random = static_cast<i16>(qrand() * (1.0f / 65535.0f) * range -
                                              ((static_cast<i32>(MAXBLOWUPRANDSPANG * 0.5f) << 16) / 360));
                NuVecRotateX(&velocity, &velocity, static_cast<u16>(blowup->field_0xf6) + random);
                range = (MAXBLOWUPRANDSPANG << 16) / 360;
                random = static_cast<i16>(qrand() * (1.0f / 65535.0f) * range -
                                          ((static_cast<i32>(MAXBLOWUPRANDSPANG * 0.5f) << 16) / 360));
                NuVecRotateY(&velocity, &velocity, static_cast<u16>(blowup->field_0xf8) + random);
                NuVecScale(&velocity, &velocity, type->field_0x98);
                if (object != NULL) {
                    velocity.x += 0.75f * object->apiobj.velocity.x;
                    velocity.y += 0.75f * object->apiobj.velocity.y;
                    velocity.z += 0.75f * object->apiobj.velocity.z;
                }
                ADDPART_s part = Default_ADDPART;
                part.matrix = &matrix;
                part.velocity = &velocity;
                part.special = &type->alternate_specials[i];
                part.gravity = type->field_0x94;
                part.flags |= 0x80;
                part.stop_fn = PartStop_Flickerer;
                part.draw_fn = PartDraw_Flickerer;
                part.time_step = FRAMETIME;
                part.field_c4 = 1;
                AddPart(&part);
            }
        }
    }
    if (GizmoBlowUp_SfxFn != NULL) {
        GizmoBlowUp_SfxFn(blowup, position);
    } else {
        GameAudio_PlaySfx(0x33, position, 0, 0);
    }
    if ((flags & 8) != 0) {
        i32 explosion_flags = (blowup->draw_flags & 0x40) != 0 ? 0x122 : 0x127;
        if ((blowup->draw_flags & 0x10000000) != 0)
            explosion_flags |= 8;
        AddExplosion(position, blowup->field_0xb8, 0.5f, NULL, -1, explosion_flags);
    }
}

i32 InitGizmoBlowups(WORLDINFO_s *world) {
    world->gizmo_blowups = NULL;
    world->gizmo_blowup_count = 0;
    if (world->current_level->max_gizmo_blowups == 0) {
        return 0;
    }

    world->giz_buffer.addr = (world->giz_buffer.addr + 0xf) & ~static_cast<usize>(0xf);
    world->gizmo_blowups = static_cast<GIZMOBLOWUP_s *>(GameBufferAlloc(
        &world->giz_buffer, &world->unknown_0108, world->current_level->max_gizmo_blowups * sizeof(GIZMOBLOWUP_s)));
    memset(world->gizmo_blowups, 0, world->current_level->max_gizmo_blowups * sizeof(GIZMOBLOWUP_s));
    return world->gizmo_blowups != NULL;
}

extern "C" void AddFiniteShotDebrisEffect(i32 *, i32, NUVEC *, i32);

void GizBlowup_Respawn(GIZMOBLOWUP_s *blowup) {
    if (blowup == NULL)
        return;
    blowup->state_flags |= 0x80;
    blowup->output_flags &= ~1;
    blowup->field_0x9f &= ~1;
    blowup->visibility_flags = (blowup->visibility_flags & 0x7f) | 0x40;
    blowup->saved_state_1 = blowup->initial_state_1;
    blowup->saved_state_0 = blowup->initial_state_0;
    nuinstanim_s *animation = NuSpecialGetInstAnim(&blowup->type->animated_special);
    if (animation != NULL && animation->playing != 0) {
        blowup->state_flags |= 0x10;
        if (animation->repeating != 0)
            blowup->state_flags |= 0x48;
    }
    blowup->state_flags |= 1;
    if (BonusArea != 0 && VehicleArea != 0 && (blowup->draw_flags & 0x1000000) != 0) {
        if (NuSpecialExistsFn(&blowup->type->animated_special)) {
            NuSpecialSetVisibility(&blowup->type->animated_special, 0);
        }
    } else if ((blowup->visibility_flags & 0x40) != 0) {
        NuSpecialSetVisibility(&blowup->type->animated_special, 1);
    }
    if (NuSpecialExistsFn(&blowup->type->decal_special)) {
        NuSpecialSetVisibility(&blowup->type->decal_special, 0);
    }
    blowup->animation_time = 0.0f;
    blowup->output_flags &= 1;
    blowup->visibility_flags &= 0xe4;
    blowup->state_flags &= 0xf1;
    blowup->saved_state_0 = blowup->initial_state_0;
    blowup->field_0x9f &= ~2;
    blowup->saved_state_1 = blowup->initial_state_1;
    animation = NuSpecialGetInstAnim(&blowup->type->animated_special);
    if (animation != NULL) {
        if (BonusArea != 0 && VehicleArea != 0 && (blowup->draw_flags & 0x1000000) != 0 &&
            (blowup->state_flags & 0x10) != 0) {
            animation->playing = 1;
        }
        if (animation->playing != 0 && animation->repeating != 0)
            blowup->state_flags |= 0x48;
    }
    if (blowup->platform_id != -1)
        PlatOnOff(blowup->platform_id, 1);
    if (blowup->type->particle_types[2] != -1) {
        i32 handle = -1;
        AddFiniteShotDebrisEffect(&handle, blowup->type->particle_types[2], &blowup->mid_position, 1);
    }
    if (blowup->type->particle_types[3] != -1) {
        i32 handle = -1;
        AddFiniteShotDebrisEffect(&handle, blowup->type->particle_types[3], &blowup->mid_position, 1);
    }
}

i32 GizmoBlowupBlowup(GIZMOBLOWUP_s *blowup, i32 effects, i32 hit_type, i32 damage, GameObject_s *object,
                      i32 hit_context) {
    if (blowup == NULL || (blowup->state_flags & 0x80) == 0 || ((blowup->draw_flags & 0x20) && ShadowMode == 0) ||
        !TouchHacks::CanBlowupBeBlownUp(*blowup, hit_type)) {
        return 0;
    }
    nuinstanim_s *animation = NuSpecialGetInstAnim(&blowup->type->animated_special);
    GIZMOBLOWUPTYPE_s *type = blowup->type;
    if (animation != NULL && type->animation_start_frame != type->animation_end_frame) {
        if ((type->animation_flags & 0x20) == 0 && (blowup->state_flags & 0x40) == 0) {
            if ((blowup->draw_flags & 0x208) == 0) {
                if (blowup->animation_time < type->animation_end_frame) {
                    blowup->state_flags |= 8;
                    return 0;
                }
                blowup->state_flags &= ~8;
            }
        } else {
            const f32 frame = (blowup->draw_flags & 0x800000) ? animation->ltime : blowup->animation_time;
            if (frame < type->animation_start_frame || type->animation_end_frame < frame) {
                return 0;
            }
        }
    }
    switch (hit_type) {
        case 1:
            blowup->output_flags |= 2;
            break;
        case 2:
            blowup->output_flags |= 4;
            break;
        case 3:
            blowup->output_flags |= 8;
            break;
        case 4:
            blowup->output_flags |= 0x20;
            break;
        case 5:
            blowup->output_flags |= 0x10;
            break;
        case 6:
            blowup->output_flags |= 0x40;
            break;
        case 7:
            blowup->output_flags |= 0x80;
            break;
        case 8:
            blowup->visibility_flags |= 1;
            break;
        case 9:
            blowup->visibility_flags |= 2;
            break;
        case 10:
            blowup->visibility_flags |= 4;
            break;
        case 11:
            blowup->visibility_flags |= 8;
            break;
        case 12:
            blowup->visibility_flags |= 0x10;
            break;
        case 13:
            blowup->visibility_flags |= 0x20;
            break;
    }
    const i32 has_burst = NuSpecialExistsFn(&type->burst_special);
    if (has_burst) {
        nuinstanim_s *burst = NuSpecialGetInstAnim(&type->burst_special);
        if (burst != NULL) {
            burst->playing = 1;
        }
    }
    bool destroyed = false;
    u8 effect_flags = 1;
    if (damage != -1) {
        blowup->saved_state_1 = static_cast<u8>(blowup->saved_state_1 - damage);
    }
    if (damage == -1 || static_cast<i8>(blowup->saved_state_1) < 1) {
        blowup->saved_state_1 = 0;
        if ((type->animation_flags & 0x20) && (blowup->draw_flags & 0x800000)) {
            NuSpecialSetVisibility(&type->animated_special, 0);
        }
        blowup->output_flags |= 1;
        blowup->visibility_flags &= 0x3f;
        blowup->field_0x9f &= ~1;
        blowup->state_flags &= 0x73;
        blowup->animation_time = 1.0f;
        if (object == NULL) {
            if ((blowup->draw_flags & 0x208) && blowup->saved_state_0 != 0 &&
                (blowup->field_0xb4 > 0.0f || blowup->field_0xb8 <= 0.0f)) {
                const f32 radius_squared = blowup->field_0xb4 * blowup->field_0xb4;
                for (i32 player = 0; player < 8; ++player) {
                    GameObject_s *target = Player[player];
                    if (target != NULL && (target->apiobj.field_0x1f8 & 0x1001) == 0x1001 &&
                        target->apiobj.field_0x287 == 0 && target->field_0x101c <= 0.0f) {
                        const f32 x = blowup->mid_position.x - target->apiobj.position.x;
                        const f32 y = blowup->mid_position.y - target->apiobj.position.y;
                        const f32 z = blowup->mid_position.z - target->apiobj.position.z;
                        if (x * x + y * y + z * z < radius_squared) {
                            ObjHitObj(NULL, target, blowup->saved_state_0, 1, 0, hit_context);
                        }
                    }
                }
            }
        } else if (blowup->saved_state_0 != 0 &&
                   (object->id != id_SPEEDERBIKE || (object->apiobj.flags_low & 0x80) == 0 ||
                    WORLD->current_level != SPEEDERCHASEA_LDATA || disable_narrow_socks != 0)) {
            ObjHitObj(NULL, object, blowup->saved_state_0, 1, 0, hit_context);
        }
        if (blowup->platform_id != -1) {
            PlatOnOff(blowup->platform_id, 0);
            if (has_burst && blowup->field_0x10c != -1) {
                PlatOnOff(blowup->field_0x10c, 1);
            }
            if (type->animation_flags & 0x20) {
                NuSpecialSetVisibility(&type->animated_special, 0);
            }
        }
        if ((blowup->draw_flags & 0x400) == 0 && NuSpecialExistsFn(&blowup->type->decal_special)) {
            blowup->animation_time = 0.1f;
            blowup->state_flags |= 2;
        }
        if (blowup->draw_flags & 0x800) {
            GizmoBlowupCreateStuff(blowup);
        }
        if (GameBlowUpBlownUpFn != NULL) {
            GameBlowUpBlownUpFn(blowup);
        }
        if ((blowup->output_flags & 1) && CheckLostDataFn != NULL) {
            CheckLostDataFn(blowup);
        }
        if (blowup->field_0xd8 > 0.0f) {
            blowup->respawn_timer = blowup->field_0xd8;
        }
        effect_flags = (blowup->draw_flags & 0x40000) ? 0x17 : 7;
        if (blowup->anti_node != NULL) {
            GameAntinode_UnregisterAntiNode(WORLD->game_antinode_sys, blowup->anti_node);
            blowup->anti_node = NULL;
        }
        destroyed = true;
    } else if (blowup->state_flags & 0x20) {
        effect_flags = 4;
    }
    if (blowup->field_0xb8 > 0.0f && destroyed) {
        effect_flags |= 8;
    }
    if (effects != 0) {
        GizmoBlowUp_AddEffects(&blowup->mid_position, blowup, 1, effect_flags, object);
    }
    return 1;
}

void BlowupObjEmit_Stop(PART_s *) {
}

void GizmoBlowupTypeAdd(WORLDINFO_s *, nuhspecial_s *, i32, i32 *) {
}

GIZMOBLOWUPTYPE_s *GizmoBlowup_FindType(char *name, WORLDINFO_s *world) {
    if (world == NULL || world->gizmo_blowup_types == NULL || world->gizmo_blowup_type_count == 0 || name == NULL) {
        return NULL;
    }
    for (i32 index = 0; index < world->gizmo_blowup_type_count; ++index) {
        if (NuStrCmp(world->gizmo_blowup_types[index].name, name) == 0) {
            return &world->gizmo_blowup_types[index];
        }
    }
    return NULL;
}

i32 InitGizmoBlowupTypes(WORLDINFO_s *world) {
    world->gizmo_blowup_types = NULL;
    world->gizmo_blowup_type_count = 0;
    if (world->current_level->max_gizmo_blowup_types == 0) {
        return 0;
    }
    world->gizmo_blowup_types = static_cast<GIZMOBLOWUPTYPE_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108,
                        world->current_level->max_gizmo_blowup_types * sizeof(GIZMOBLOWUPTYPE_s)));
    return world->gizmo_blowup_types != NULL;
}

i32 SetGizmoBlowUpTarget(GameObject_s *object, GIZMOBLOWUP_s *blowup) {
    object->attack_target_position = blowup->mid_position;
    object->attack_target_velocity = v000;
    object->field_0xe21 |= 8;
    object->attack_blowup_target = VehicleArea != 0 ? NULL : blowup;
    return 1;
}

void GizBlowup_InitTerrain() {
    if (WORLD->gizmo_blowups != NULL) {
        for (i32 i = 0; i < WORLD->gizmo_blowup_count; ++i) {
            GIZMOBLOWUP_s *blowup = &WORLD->gizmo_blowups[i];
            blowup->platform_id = -1;
            blowup->field_0x10c = -1;
            if ((blowup->draw_flags & 4) != 0)
                GizBlowup_InitSingleTerrain(blowup);
        }
    }
}

void GizmoBlowupTypeRemove(GIZMOBLOWUPTYPE_s *, WORLDINFO_s *) {
}

void NewRumble(nupad_s *, f32, i32);
void Bolt_AddDeflectedBolt(BOLT_s *, nuvec_s *, nuvec_s *, unsigned char *);
void Bolt_End(BOLT_s *, i32);
void Bolt_PlayHitSfx(BOLT_s *);

void GizmoBlowup_HitBlowup(GameObject_s *object, GIZMOBLOWUP_s *blowup, i32 hit_type, BOLT_s *bolt, nuvec_s *position,
                           unsigned char *hit_data, u32 mode, i32) {
    if (GizmoBlowupBlowup(blowup, 1, hit_type, 1, NULL, 1)) {
        if (object != NULL) {
            NewRumble(object->pad_gamepad->pad, 0.4f, 0);
            GameCam_HitJudder();
        }
    } else if (bolt != NULL) {
        NUVEC direction;
        NuVecSub(&direction, &blowup->position, &bolt->position);
        NuVecNorm(&direction, &direction);
        Bolt_AddDeflectedBolt(bolt, &bolt->field_0xac, &direction, hit_data);
    }
    if (BoltSys->stop_targeting != NULL) {
        BoltSys->stop_targeting(object, position);
    }
    if (mode == 1) {
        BoltSys->debris(bolt, position, -1, NULL, 0);
        if (bolt->owner != NULL) {
            NewRumble(bolt->owner->pad_gamepad->pad, 0.6f, 0);
        }
        Bolt_End(bolt, 1);
        Bolt_PlayHitSfx(bolt);
    }
}

GIZMOBLOWUP_s *FindNearestGizmoBlowUp(WORLDINFO_s *world, nuvec_s *position, float max_distance_squared) {
    if (world == NULL || world->gizmo_blowup_count <= 0 || position == NULL)
        return NULL;
    GIZMOBLOWUP_s *nearest = NULL;
    f32 nearest_distance = max_distance_squared;
    GIZMOBLOWUP_s *end = world->gizmo_blowups + world->gizmo_blowup_count;
    GIZMOBLOWUP_s *blowup = world->gizmo_blowups;
    do {
        if ((blowup->status_flags & 0x804001) != 0x804000)
            continue;
        f32 x = blowup->position.x - position->x;
        f32 y = blowup->position.y - position->y;
        f32 z = blowup->position.z - position->z;
        f32 distance = x * x + y * y + z * z;
        if (distance < max_distance_squared && distance < nearest_distance) {
            nearest = blowup;
            nearest_distance = distance;
        }
    } while (++blowup != end);
    return nearest;
}

i16 GetGenericGoon(i32);

void GizmoBlowupCreateStuff(GIZMOBLOWUP_s *blowup) {
    char script[32] = "GoonBox";
    i32 model = GetGenericGoon(0);
    f32 choice = qrand() * (1.0f / 65535.0f) * 100.0f;
    if (!(choice < 50.0f)) {
        choice -= 50.0f;
        if (choice < 50.0f) {
            i32 count = static_cast<i32>(NuFloatRand(reinterpret_cast<NURAND *>(&GAMERAND)) * 3.0f);
            for (i32 i = 0; i <= count; ++i) {
                i32 angle =
                    static_cast<u16>(static_cast<i32>(NuFloatRand(reinterpret_cast<NURAND *>(&GAMERAND)) * 65536.0f));
                GameObject_s *object =
                    AddDynamicCreature(model, &blowup->mid_position, angle, script, NULL, NULL, 1, NULL, NULL, 0, -1);
                if (object != NULL) {
                    object->field_0x1038 = 0.0f;
                    object->field_0x1004 = 0.0f;
                    object->apiobj.field_0xa8 = 0.0f * object->apiobj.character_data->model_scale;
                } else {
                    i32 coins =
                        (static_cast<i32>(NuFloatRand(reinterpret_cast<NURAND *>(&GAMERAND)) * 1000.0f) / 100) * 100;
                    if (coins < 100)
                        coins = 100;
                    NUVEC direction = v010;
                    NuVecRotateX(&direction, &direction, static_cast<u16>(blowup->field_0xf6));
                    NuVecRotateY(&direction, &direction, static_cast<u16>(blowup->field_0xf8));
                    AddPickups(coins, ReleaseHearts(), 0, 0, &blowup->mid_position, &direction, 0.0f, -1, 1.5f,
                               2000000.0f, NULL, 1, 1, true);
                }
            }
            return;
        }
        choice -= 50.0f;
        if (choice < 0.0f)
            return;
    }
    i32 coins = (static_cast<i32>(NuFloatRand(reinterpret_cast<NURAND *>(&GAMERAND)) * 1000.0f) / 100) * 100;
    if (coins < 100)
        coins = 100;
    NUVEC direction = v010;
    NuVecRotateX(&direction, &direction, static_cast<u16>(blowup->field_0xf6));
    NuVecRotateY(&direction, &direction, static_cast<u16>(blowup->field_0xf8));
    AddPickups(coins, ReleaseHearts(), 0, 0, &blowup->mid_position, &direction, 0.0f, -1, 1.5f, 2000000.0f, NULL, 1, 1,
               true);
}

void GizmoBlowupsFinalSetup(WORLDINFO_s *world) {
    for (i32 type_index = 0; type_index < world->gizmo_blowup_type_count; ++type_index) {
        GIZMOBLOWUPTYPE_s *type = &world->gizmo_blowup_types[type_index];
        type->animation_runtime_flags &= ~1;
        nuinstanim_s *animation = NuSpecialGetInstAnim(&type->animated_special);
        type->animation_base_frame = animation != NULL ? animation->ltime : 0.0f;
        if (NuSpecialExistsFn(&type->decal_special) != 0) {
            NuSpecialSetVisibility(&type->decal_special, 0);
        }
    }

    for (i32 instance_index = 0; instance_index < world->gizmo_blowup_count; ++instance_index) {
        GIZMOBLOWUP_s *blowup = &world->gizmo_blowups[instance_index];
        blowup->animation_time = 1.0f;
        blowup->field_0x9f &= ~0x08;
        blowup->visibility_flags &= ~0x03;
        blowup->output_flags = 0;
        blowup->state_flags = (blowup->state_flags & 0xb7) | 0x81;

        nuhspecial_s *special = blowup->override_special;
        if (special == NULL || NuSpecialExistsFn(special) == 0) {
            special = &blowup->type->animated_special;
        }

        if (NuSpecialExistsFn(special) == 0) {
            blowup->bounds_min = v000;
            blowup->bounds_max = v000;
        } else {
            if (NuSpecialExistsFn(&blowup->type->burst_special) != 0) {
                NuSpecialSetVisibility(&blowup->type->burst_special, 0);
            }
            if ((blowup->type->animation_flags & 0x20) != 0) {
                blowup->position = *NuSpecialGetDrawPos(special);
            }

            NUVEC centre;
            NuSpecialGetRadius(special, &centre, &blowup->target_scale);
            centre.x += blowup->position.x;
            centre.y += blowup->position.y;
            centre.z += blowup->position.z;
            blowup->bounds_min.x = centre.x - blowup->target_scale;
            blowup->bounds_min.y = centre.y - blowup->target_scale;
            blowup->bounds_min.z = centre.z - blowup->target_scale;
            blowup->bounds_max.x = centre.x + blowup->target_scale;
            blowup->bounds_max.y = centre.y + blowup->target_scale;
            blowup->bounds_max.z = centre.z + blowup->target_scale;

            nuinstanim_s *animation = NuSpecialGetInstAnim(special);
            if (animation != NULL && animation->playing != 0) {
                const u8 original_state = blowup->state_flags;
                blowup->state_flags = original_state | 0x10;
                if (animation->repeating != 0) {
                    blowup->state_flags = original_state | 0x58;
                }
            }
        }

        GizmoBlowupUpdateMatrix(blowup);
        blowup->initial_state_1 = static_cast<u8>(blowup->initial_state_1 * GizmoBlowup_HitMultiplier);
    }

    GizBlowup_DeleteTerrain();
    GizBlowup_InitTerrain();
}

void GizBlowup_DeleteTerrain() {
    if (WORLD->gizmo_blowups != NULL) {
        for (i32 i = 0; i < WORLD->gizmo_blowup_count; ++i) {
            GizBlowup_DeleteSingleTerrain(&WORLD->gizmo_blowups[i]);
        }
    }
}

i32 GizmoBlowupTypeGetCount(WORLDINFO_s *world) {
    return world != NULL ? world->gizmo_blowup_type_count : -1;
}

void GizmoBlowupUpdateMatrix(GIZMOBLOWUP_s *blowup) {
    if (blowup == NULL) {
        return;
    }

    NUVEC position = blowup->position;
    blowup->transform = *NuSpecialGetDrawMtx(&blowup->type->animated_special);
    *NUMTX_GET_ROW_VEC(&blowup->transform, 3) = v000;
    NuMtxRotateY(&blowup->transform, blowup->field_0xf2);
    NuMtxPreRotateX(&blowup->transform, blowup->field_0xf0);
    NuMtxPreRotateY(&blowup->transform, blowup->field_0xf4);
    NuMtxTranslate(&blowup->transform, &position);
}

u32 GizmoBlowups_TotalScore(void *context) {
    WORLDINFO_s *world = static_cast<WORLDINFO_s *>(context);
    GIZMOBLOWUP_s *blowup = world->gizmo_blowups;
    u32 total = 0;
    if (blowup != NULL) {
        for (i32 i = 0; i < world->gizmo_blowup_count; ++i, ++blowup)
            total += static_cast<i8>(blowup->field_0x115) * blowup->field_0xa8;
    }
    return total;
}

i32 GizmoBlowupTypeNameBlank(char *name) {
    for (u32 offset = 0; offset < 0x20; offset += sizeof(i32)) {
        if (*reinterpret_cast<i32 *>(name + offset) != 0) {
            return 0;
        }
    }
    return 1;
}

i32 GizmoBlowupCheckProximity(WORLDINFO_s *world, GameObject_s *object) {
    i32 count = 0;
    if (object == NULL || static_cast<i8>(object->apiobj.field_0x1f8) >= 0)
        return count;
    i32 special;
    i32 ordinary;
    if ((object->apiobj.character_data->model_flags & 0x2000) != 0 || object->field_0xcc0 != NULL) {
        special = 1;
        ordinary = 0;
    } else {
        special = 0;
        ordinary = 1;
    }
    i32 destroy = 0;
    GIZMOBLOWUP_s *blowup = world->gizmo_blowups;
    for (i32 index = 0; index < world->gizmo_blowup_count; ++index, ++blowup) {
        if ((blowup->status_flags & 0x804001) != 0x804000)
            continue;
        if (special && (blowup->draw_flags & 0x200) == 0)
            continue;
        if (ordinary && (blowup->draw_flags & 8) == 0)
            continue;
        if ((blowup->state_flags & 4) != 0 || (blowup->field_0x9f & 2) != 0)
            continue;
        f32 x = object->apiobj.collision_position.x - blowup->mid_position.x;
        f32 y = object->apiobj.collision_position.y - blowup->mid_position.y;
        f32 z = object->apiobj.collision_position.z - blowup->mid_position.z;
        if (blowup->field_0xb4 != 0.0f && !(x * x + y * y + z * z < blowup->field_0xb4 * blowup->field_0xb4))
            continue;
        i32 hit_type;
        if (special) {
            blowup->output_flags |= 0x10;
            hit_type = 5;
        } else {
            blowup->output_flags |= 0x20;
            hit_type = 4;
        }
        nuinstanim_s *animation = NuSpecialGetInstAnim(&blowup->type->animated_special);
        if (blowup->field_0xc0 > 0.0f) {
            blowup->activation_delay = blowup->field_0xc0;
            blowup->state_flags |= 4;
        } else if (animation == NULL || (blowup->state_flags & 8) != 0) {
            destroy = 1;
        } else {
            blowup->state_flags |= 8;
            blowup->field_0x9f |= 2;
            blowup->animation_time = 0.0f;
        }
        if (destroy) {
            GizmoBlowupBlowup(blowup, 1, hit_type, 1, object, 1);
            ++count;
        }
    }
    return count;
}

void GizmoBlowupGenDecalMatrix(GIZMOBLOWUP_s *blowup, numtx_s *matrix, i32 alternate) {
    if (blowup == NULL)
        return;
    NuMtxSetIdentity(matrix);
    if (alternate != 0) {
        NuMtxScaleU(matrix, blowup->field_0xc8);
    } else {
        NuMtxScaleU(matrix, (1.0f - 10.0f * blowup->animation_time) * blowup->field_0xc8);
    }
    matrix->m30 = 0.0f;
    matrix->m31 = 0.0f;
    matrix->m32 = 0.0f;
    NuMtxRotateY(matrix, static_cast<u16>(blowup->field_0xe6));
    NuMtxPreRotateX(matrix, static_cast<u16>(blowup->field_0xe4));
    NuMtxPreRotateY(matrix, static_cast<u16>(blowup->field_0xe8));
    NUVEC position;
    position.x = blowup->position.x + blowup->field_0x74;
    position.y = blowup->position.y + blowup->field_0x78;
    position.z = blowup->position.z + blowup->field_0x7c;
    NuMtxTranslate(matrix, &position);
}

i32 GizmoBlowupGetNameTableId(char *name) {
    for (i32 id = 0; id < gizmoblowupnametable_numids; ++id) {
        if (NuStrICmp(gizmoblowupnametable[id], name) == 0) {
            return id;
        }
    }

    if (gizmoblowupnametable_numids >= 32) {
        return -1;
    }

    NuStrNCpy(gizmoblowupnametable[gizmoblowupnametable_numids], name, sizeof(gizmoblowupnametable[0]));
    return gizmoblowupnametable_numids++;
}

i32 InitGizmoBlowupsMtxBuffer(WORLDINFO_s *world) {
    world->gizmo_blowup_mtx_buffer = NULL;
    world->giz_buffer.addr = (world->giz_buffer.addr + 0x7f) & ~static_cast<usize>(0x7f);
    world->gizmo_blowup_mtx_buffer =
        static_cast<NUMTX *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 0x8000));
    return world->gizmo_blowup_mtx_buffer != NULL;
}

// Original writable flagremaptab at 0x668520, 32 pairs of 32-bit masks.
extern "C" {
    u32 flagremaptab[32][2] = {
        {0x00000001u, 0x00000002u}, {0x00000002u, 0x00000040u}, {0x00000004u, 0x00000001u}, {0x00000008u, 0x00000004u},
        {0x00000010u, 0x00000008u}, {0x00000020u, 0x00000010u}, {0x00000040u, 0x00000020u}, {0x00000080u, 0x00000080u},
        {0x00000100u, 0x00000100u}, {0x00000200u, 0x00000200u}, {0x00000400u, 0x00000400u}, {0x00000800u, 0x00000800u},
        {0x00001000u, 0x00001000u}, {0x00002000u, 0x20000000u}, {0x00004000u, 0x00008000u}, {0x00008000u, 0x00010000u},
        {0x00010000u, 0x00020000u}, {0x00020000u, 0x00040000u}, {0x00040000u, 0x00080000u}, {0x00080000u, 0x00100000u},
        {0x00100000u, 0x00200000u}, {0x00200000u, 0x00400000u}, {0x00400000u, 0x00800000u}, {0x00800000u, 0x01000000u},
        {0x01000000u, 0x02000000u}, {0x02000000u, 0x04000000u}, {0x04000000u, 0x10000000u}, {0x08000000u, 0x40000000u},
        {0x10000000u, 0x00004000u}, {0x20000000u, 0x08000000u}, {0x40000000u, 0x00002000u}, {0x80000000u, 0x80000000u}};
}

u32 RemapTypeFlagToBlowupFlag(u32 flag) {
    for (i32 index = 0; index < 32; ++index) {
        if (flagremaptab[index][0] == flag) {
            return flagremaptab[index][1];
        }
    }
    return 0;
}

void GizmoBlowupGenShadowMatrix(GIZMOBLOWUP_s *blowup, numtx_s *matrix) {
    extern NUVEC v111;
    if (blowup == NULL)
        return;
    NuMtxSetIdentity(matrix);
    NuMtxSetScale(matrix, &v111);
    matrix->m30 = 0.0f;
    matrix->m31 = 0.0f;
    matrix->m32 = 0.0f;
    NuMtxRotateY(matrix, static_cast<u16>(blowup->field_0xec));
    NuMtxPreRotateX(matrix, static_cast<u16>(blowup->field_0xea));
    NuMtxPreRotateY(matrix, static_cast<u16>(blowup->field_0xee));
    NUVEC position;
    position.x = blowup->position.x + blowup->field_0x8c;
    position.y = blowup->position.y + blowup->field_0x90;
    position.z = blowup->position.z + blowup->field_0x94;
    NuMtxTranslate(matrix, &position);
}

i32 GizBlowup_InitSingleTerrain(GIZMOBLOWUP_s *blowup) {
    if (blowup == NULL || (blowup->output_flags & 1) != 0 || (blowup->draw_flags & 4) == 0)
        return 0;
    if (blowup->platform_id == -1) {
        i32 instance = NuSpecialGetInstanceix(&blowup->type->animated_special);
        if (blowup->override_special != NULL && NuSpecialExistsFn(blowup->override_special)) {
            instance = NuSpecialGetInstanceix(blowup->override_special);
            blowup->platform_id = FindPlatInst(instance);
        } else if ((blowup->draw_flags & 0x802000) == 0x2000 || (blowup->draw_flags & 0x20000000) == 0) {
            blowup->platform_id = NewPlatInst(blowup, instance);
        } else {
            blowup->platform_id = FindPlatInst(instance);
        }
        if (blowup->platform_id == -1)
            return 0;
        if ((blowup->visibility_flags & 0x40) == 0)
            PlatOnOff(blowup->platform_id, 0);
    }
    PlatInstRotate(blowup->platform_id, 1);
    if (blowup->field_0x10c == -1 && NuSpecialExistsFn(&blowup->type->burst_special)) {
        blowup->field_0x10c = NewPlatInst(blowup, NuSpecialGetInstanceix(&blowup->type->burst_special));
        if (blowup->field_0x10c == -1)
            return -1;
    }
    PlatInstRotate(blowup->field_0x10c, 1);
    return 1;
}

void GizBlowup_DeleteSingleTerrain(GIZMOBLOWUP_s *blowup) {
    if (blowup == NULL)
        return;
    if ((blowup->override_special == NULL || !NuSpecialExistsFn(blowup->override_special)) &&
        blowup->platform_id != -1) {
        DeletePlatinst(blowup->platform_id);
        blowup->platform_id = -1;
    }
    if (blowup->field_0x10c != -1) {
        DeletePlatinst(blowup->field_0x10c);
        blowup->field_0x10c = -1;
    }
}

void GizmoBlowupVisibilityOverrides(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *blowup = world->gizmo_blowups;
    for (i32 index = 0; index < world->gizmo_blowup_count; ++index, ++blowup) {
        if ((blowup->type->animation_flags & 0x20) != 0 && (blowup->draw_flags & 0x800000) == 0) {
            NuSpecialSetVisibility(&blowup->type->animated_special, 0);
        }
    }
    for (i32 index = 0; index < world->gizmo_blowup_type_count; ++index) {
        nuhspecial_s *special = &world->gizmo_blowup_types[index].decal_special;
        if (NuSpecialExistsFn(special)) {
            NuSpecialSetVisibility(special, 0);
        }
    }
}

f32 GizmoBlowup_SetAutoSetReflectY(GIZMOBLOWUP_s *blowup, nuvec_s *position) {
    if (blowup != NULL) {
        if (blowup->platform_id != 0) {
            PlatOnOff(blowup->platform_id, 0);
        }
        f32 ground = GameShadow(NULL, position, 5.0f, -1);
        if (blowup->platform_id != 0) {
            PlatOnOff(blowup->platform_id, 1);
        }
        if (ground != 2000000.0f) {
            return ground;
        }
    }
    return 0.0f;
}

extern void Transform_DrawTarget(NUVEC *position, f32 scale, f32 opacity);
extern i32 Transform_TargettedByObj(void *object);

void GizmoBlowup_TransformDraw_Game(GIZMOBLOWUP_s *blowup) {
    if (Transform_TargettedByObj(blowup) != 0) {
        return;
    }

    Transform_DrawTarget(&blowup->mid_position, 1.4f * blowup->target_scale, 0.4f);
}

u32 RemapAllTypeFlagsToBlowupFlags(u32 flags) {
    u32 result = 0;
    for (i32 bit = 0; bit < 32; ++bit) {
        u32 flag = flags & (1u << bit);
        if (flag != 0) {
            result |= RemapTypeFlagToBlowupFlag(flag);
        }
    }
    return result;
}

char *GizmoBlowupTypeGetNameFromIndex(WORLDINFO_s *world, i32 index) {
    if (index == -1 || world == NULL || index > world->gizmo_blowup_type_count)
        return NULL;
    char *name = world->gizmo_blowup_types[index].name;
    return NuStrLen(name) != 0 ? name : NULL;
}

void GizmoBlowUpTypeBlowUp(WORLDINFO_s *world, i32 index, nuvec_s *position) {
    if (world == NULL)
        return;
    char *name = GizmoBlowupTypeGetNameFromIndex(world, index);
    if (name == NULL)
        return;
    GIZMOBLOWUPTYPE_s *type = GizmoBlowup_FindType(name, world);
    if (type == NULL)
        return;
    GIZMOBLOWUP_s blowup;
    memset(&blowup, 0, sizeof(blowup));
    blowup.type = type;
    GizmoBlowUp_AddEffects(position, &blowup, 0, 7, NULL);
    GameAudio_PlaySfx(0x33, position, 0, 0);
}

i32 GizmoBlowupGetTypeFromNameTableId(WORLDINFO_s *world, i32 name_id) {
    if (name_id < 0 || name_id >= gizmoblowupnametable_numids) {
        return -1;
    }

    const char *name = gizmoblowupnametable[name_id];
    for (i32 type_index = 0; type_index < world->gizmo_blowup_type_count; ++type_index) {
        if (NuStrICmp(world->gizmo_blowup_types[type_index].name, name) == 0) {
            return type_index;
        }
    }

    return -1;
}
