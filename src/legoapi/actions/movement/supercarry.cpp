#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/world/world.h"
#include "legoapi/gizmos/door/plugs.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nutrig.h"
#include "legoapi/core/input/qrand.h"
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void GizBlowup_Respawn(GIZMOBLOWUP_s *);
void GizmoBlowUp_AddEffects(NUVEC *, GIZMOBLOWUP_s *, i32, i32, GameObject_s *);
void DrawObjectOnCharacter(WORLDINFO_s *, GameObject_s *, i32, nuhspecial_s *, i32, i32, NUMTX *, i32, u32, NUMTX *,
                           NUVEC *, f32, f32);
void QuatInterpolateRotationMatrix(NUMTX *, NUMTX *, NUMTX *, f32);
extern i32 dco_locatorposonly;
extern ADDPART_s Default_ADDPART;
extern "C" PART_s *AddPart(ADDPART_s *);
u16 ObjHitObj_Flags(GameObject_s *);
f32 SUPERCARRY_THROWSPEED_XZ = 3.0f;
f32 SUPERCARRY_RELEASESPEED_XZ = 2.0f;
f32 SUPERCARRY_DROPSPEED_XZ = 0.2f;
f32 SUPERCARRY_DROPSPEED_Y = 1.0f;
f32 SUPERCARRY_THROWSPEED_Y = 3.0f;
f32 SUPERCARRY_OBJGRAVITY = -6.0f;
// Original defaults: 0x668d30 and 0x668d40, each a four-byte integer.
i32 SuperCarry_AlignObject = 1;
i32 SuperCarry_KeepObjectLevel = 1;

#include "nu2api/numath/nuang.h"
#include "nu2api/nu3d/nuspecial.h"
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);
i32 GizmoBlowupBlowup(GIZMOBLOWUP_s *, i32, i32, i32, GameObject_s *, i32);
u32 (*CanSuperCarryFn)(GameObject_s *) = NULL;
i32 SuperCarry_Carrying(GameObject_s *);

#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/characters/motion/animlist.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/timer.h"
#include "nu2api/numath/nufloat.h"
i32 SuperCarry_UseActionButton = 0;
i32 SuperCarry_PutDownDrop = 1;
f32 SUPERCARRY_JUMPSPEED = 1.4f;
extern f32 PUNCHGAP;
extern i32 objopponent_ignoreaiopponent;
extern ADDGAMEMSG AddGameMsg_Default;
extern char *LEGOASCII_DOWN;
extern char *txt_UNKNOWN;
extern u8 PlayerRGB[2][3];
GAMEMESSAGE_s *AddGameMsg(ADDGAMEMSG *);
i32 ObjOpponentStillThere(GameObject_s *, GameObject_s *, f32);
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
void NewRumble(nupad_s *, f32, i32);
void NewBuzzFrames(nupad_s *, i32, i32);

// Original 0x4fab00, 184 bytes.
static __used__ void SuperCarry_PartImpact(PART_s *part) {
    f32 intensity = NuVecMag(&part->velocity);
    if (intensity >= 4.0f)
        intensity = 1.0f;
    else
        intensity *= 0.25f;
    if (intensity > 0.0f) {
        const i32 axis = qrand() > 0x7fff ? 2 : 0;
        GameCam_Judder(NULL, intensity * 0.3f, axis, &part->position);
    }
}

// Original 0x4fabc0, 347 bytes. The original returns an integer.
static __used__ i32 SuperCarry_TurnBlowupBackOn(GIZMOBLOWUP_s *blowup, NUVEC *position, u16 angle, i32 keep_matrix) {
    blowup->state_flags |= 0x80;
    blowup->visibility_flags |= 0x40;
    GizBlowup_Respawn(blowup);
    blowup->state_flags |= 1;
    f32 distance;
    PLUG *plug = Plug_FindNearest(WORLD->plug_sys, position, &distance, 1);
    if (plug != NULL && distance < 0.5625f) {
        Plug_MakeDrawMtx(plug, &blowup->transform);
        plug->flags |= PLUG_FLAG_PLUGGED;
        blowup->state_flags &= static_cast<u8>(~0x80);
        blowup->field_0x9f |= 0x10;
        return 1;
    }
    if (keep_matrix == 0) {
        if (SuperCarry_AlignObject != 0)
            NuMtxSetRotationY(&blowup->transform, angle);
        else
            blowup->transform = numtx_identity;
        NuMtxTranslate(&blowup->transform, position);
    }
    return 0;
}

// Original 0x4fad20, 112 bytes.
static __used__ void SuperCarry_PartKill(PART_s *part, i32) {
    GIZMOBLOWUP_s *blowup = part->carried_blowup;
    if ((blowup->secondary_flags & 4) != 0)
        SuperCarry_TurnBlowupBackOn(blowup, &part->position, part->rotation_y, 0);
    else
        GizmoBlowUp_AddEffects(&part->position, blowup, 0, 15, NULL);
}

// Original 0x4fad90, 673 bytes.
void SuperCarry_DrawObject(GameObject_s *object) {
    if (static_cast<i8>(object->field_0xe22) >= 0) {
        if (LEGOCONTEXT_SUPERCARRY == -1 || object->character_context != LEGOCONTEXT_SUPERCARRY)
            return;
        switch (object->field_0x7a3) {
            case 0:
                if ((object->context_flags & 0x40) == 0)
                    return;
                break;
            case 1:
            case 4:
            case 5:
                if ((object->context_flags & 0x40) != 0)
                    return;
                break;
            case 2:
            case 3:
            case 6:
            case 7:
                break;
            default:
                return;
        }
    }
    NUMTX matrix;
    NUVEC offset;
    NUVEC *translation = NULL;
    if (SuperCarry_AlignObject != 0) {
        NuVecRotateY(&offset, &object->carried_object_basis[0], object->apiobj.field_0x276);
        NuMtxSetRotationY(&matrix, object->carried_object_angle);
        translation = &offset;
    } else {
        matrix = numtx_identity;
        *NUMTX_GET_ROW_VEC(&matrix, 0) = object->carried_object_basis[0];
        *NUMTX_GET_ROW_VEC(&matrix, 1) = object->carried_object_basis[1];
        *NUMTX_GET_ROW_VEC(&matrix, 2) = object->carried_object_basis[2];
    }
    if (SuperCarry_KeepObjectLevel != 0) {
        NuMtxRotateY(&matrix, object->apiobj.field_0x276);
        dco_locatorposonly = 1;
    }
    PLAYERCHARACTERCONFIG_s *config = object->apiobj.character_data->player_config;
    GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
    DrawObjectOnCharacter(WORLD, object, -1, &blowup->type->special, config->hand_joints[0], config->hand_joints[1],
                          object->joint_matrices, object->field_0x1088, object->field_0x1054, &matrix, translation,
                          1.0f, 1.0f);
}

// Original 0x4fb040, 410 bytes. Aligned carry uses the vector storage as offsets.
void SuperCarry_GetObjectPos(GameObject_s *object, NUVEC *position, NUVEC *secondary_position) {
    *position = object->apiobj.collision_position;
    PLAYERCHARACTERCONFIG_s *config = object->apiobj.character_data->player_config;
    const i32 first = config->hand_joints[0];
    if (first != -1 && object->apiobj.character_model->points_of_interest[first] != NULL) {
        const i32 second = config->hand_joints[1];
        if (second != -1 && object->apiobj.character_model->points_of_interest[second] != NULL) {
            *position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[first], 3);
            NUVEC *other = NUMTX_GET_ROW_VEC(&object->joint_matrices[second], 3);
            position->x += (other->x - position->x) * 0.5f;
            position->y += (other->y - position->y) * 0.5f;
            position->z += (other->z - position->z) * 0.5f;
        }
    }
    if (SuperCarry_AlignObject != 0) {
        NUVEC offset;
        if (secondary_position != NULL) {
            NuVecRotateY(&offset, &object->carried_object_basis[1], object->apiobj.field_0x276);
            NuVecAdd(secondary_position, position, &offset);
        }
        NuVecRotateY(&offset, &object->carried_object_basis[0], object->apiobj.field_0x276);
        NuVecAdd(position, position, &offset);
    }
}

// Original 0x4fb1e0, 2147 bytes.
void SuperCarry_Throw(GameObject_s *object, i32 mode) {
    PLAYERCHARACTERCONFIG_s *config = object->apiobj.character_data->player_config;
    const i32 first = config->hand_joints[0];
    if (first == -1 || object->apiobj.character_model->points_of_interest[first] == NULL)
        return;
    NUMTX matrix;
    if (SuperCarry_KeepObjectLevel != 0) {
        if (SuperCarry_AlignObject != 0) {
            NuMtxSetRotationY(&matrix, object->carried_object_angle);
        } else {
            matrix = numtx_identity;
            *NUMTX_GET_ROW_VEC(&matrix, 0) = object->carried_object_basis[0];
            *NUMTX_GET_ROW_VEC(&matrix, 1) = object->carried_object_basis[1];
            *NUMTX_GET_ROW_VEC(&matrix, 2) = object->carried_object_basis[2];
        }
        NuMtxRotateY(&matrix, object->apiobj.field_0x276);
        NuMtxTranslate(&matrix, NUMTX_GET_ROW_VEC(&object->joint_matrices[first], 3));
    } else {
        matrix = object->joint_matrices[first];
    }
    const i32 second = object->apiobj.character_data->player_config->hand_joints[1];
    if (second != -1 && object->apiobj.character_model->points_of_interest[second] != NULL) {
        NUVEC position = *NUMTX_GET_ROW_VEC(&matrix, 3);
        if (SuperCarry_KeepObjectLevel == 0) {
            NUMTX left = matrix;
            NUMTX right = object->joint_matrices[second];
            left.m30 = left.m31 = left.m32 = 0.0f;
            right.m30 = right.m31 = right.m32 = 0.0f;
            QuatInterpolateRotationMatrix(&matrix, &left, &right, 0.5f);
        }
        matrix.m30 = (position.x + object->joint_matrices[second].m30) * 0.5f;
        matrix.m31 = (position.y + object->joint_matrices[second].m31) * 0.5f;
        matrix.m32 = (position.z + object->joint_matrices[second].m32) * 0.5f;
    }
    if (SuperCarry_KeepObjectLevel == 0) {
        NUVEC position = *NUMTX_GET_ROW_VEC(&matrix, 3);
        NUMTX basis = numtx_identity;
        *NUMTX_GET_ROW_VEC(&basis, 0) = object->carried_object_basis[0];
        *NUMTX_GET_ROW_VEC(&basis, 1) = object->carried_object_basis[1];
        *NUMTX_GET_ROW_VEC(&basis, 2) = object->carried_object_basis[2];
        NuMtxMulR(&matrix, &basis, &matrix);
        *NUMTX_GET_ROW_VEC(&matrix, 3) = position;
    } else if (SuperCarry_AlignObject != 0) {
        NUVEC offset;
        NuVecRotateY(&offset, &object->carried_object_basis[0], object->apiobj.field_0x276);
        NuMtxTranslate(&matrix, &offset);
    }
    ADDPART_s params = Default_ADDPART;
    params.matrix = &matrix;
    NUVEC velocity;
    f32 speed;
    if (mode == 0)
        speed = SUPERCARRY_THROWSPEED_XZ;
    else if (mode == 1)
        speed = SUPERCARRY_RELEASESPEED_XZ;
    else
        speed = SUPERCARRY_DROPSPEED_XZ;
    velocity.x = NU_SIN_LUT(object->apiobj.facing_angle) * speed;
    velocity.z = NU_COS_LUT(object->apiobj.facing_angle) * speed;
    velocity.y = mode == 2 ? SUPERCARRY_DROPSPEED_Y : SUPERCARRY_THROWSPEED_Y;
    params.velocity = &velocity;
    GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
    params.field_14 = params.field_18 = 0.5f * blowup->target_scale;
    params.special = &blowup->type->special;
    params.flags = (blowup->secondary_flags & 4) != 0 ? 0x8011 : 0x8111;
    params.owner = object;
    params.time_step = FRAMETIME;
    params.gravity = SUPERCARRY_OBJGRAVITY;
    params.field_3c = SuperCarry_PartImpact;
    params.field_a4 = 3.0f;
    params.field_44 = SuperCarry_PartKill;
    params.lighting = reinterpret_cast<PARTLIGHTSOURCE_s *>(&object->light_data);
    PART_s *part = AddPart(&params);
    if (part != NULL) {
        part->force_flags = static_cast<u16>(ObjHitObj_Flags(object));
        part->carried_blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
        part->rotation_y = object->carried_object_angle + object->apiobj.movement_facing_angle;
    }
}

// Original 0x4fba50, 130 bytes. The original returns an integer.
i32 SuperCarry_Possible(GameObject_s *object, i32 require_grounded) {
    if (CanSuperCarryFn == NULL || CanSuperCarryFn(object) == 0)
        return 0;
    if (static_cast<i8>(object->apiobj.flags_low) >= 0)
        return 0;
    if (require_grounded != 0 && (object->apiobj.field_0x27d == 0 || ObjLandReady(object) == 0))
        return 0;
    return 1;
}

// Original 0x4fd530, 75 bytes.
void SuperCarry_Release(GameObject_s *object) {
    if (SuperCarry_Carrying(object) != 0) {
        SuperCarry_Throw(object, 1);
        object->character_context = -1;
    }
}

// Original 0x4fbae0, 1204 bytes.
void SuperCarry_Start(GameObject_s *object, GIZMOBLOWUP_s *blowup, i32 immediate) {
    object->character_context = static_cast<i8>(LEGOCONTEXT_SUPERCARRY);
    object->context_flags &= static_cast<u8>(~0x40);
    object->action_movement_state = 0;
    object->field_0x788 = blowup;
    if (immediate == 0) {
        if (LEGOACT_SUPERCARRY_PICKUP != -1 &&
            object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_PICKUP] != NULL) {
            object->context_animation = LEGOACT_SUPERCARRY_PICKUP;
            object->field_0x7a3 = 0;
            f32 duration = AnimDuration(object->id, LEGOACT_SUPERCARRY_PICKUP, 0.0f, 0.0f, 1);
            if (duration <= 0.0f)
                duration = 0.5f;
            object->context_animation_timer = duration;
            object->apiobj.movement_facing_angle =
                NuAtan2D(blowup->mid_position.x - object->apiobj.collision_position.x,
                         blowup->mid_position.z - object->apiobj.collision_position.z);
        } else {
            object->context_animation_timer = 0.0f;
            object->context_animation = LEGOACT_SUPERCARRY_IDLE;
            object->field_0x7a3 = 2;
            GizmoBlowupBlowup(blowup, 0, 7, -1, NULL, 1);
        }
    } else {
        object->context_animation_timer = 0.0f;
        object->context_animation = LEGOACT_SUPERCARRY_IDLE;
        object->field_0x7a3 = 2;
    }
    blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
    if (SuperCarry_AlignObject != 0) {
        NUVEC minimum, maximum;
        NuSpecialGetBounds(&blowup->type->special, &minimum, &maximum);
        blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
        const u16 angle = NuAtan2D(blowup->transform.m20, blowup->transform.m22);
        const i32 difference = RotDiff(object->apiobj.movement_facing_angle, angle);
        const i32 magnitude = difference < 0 ? -difference : difference;
        if (magnitude < 0x2000) {
            object->carried_object_angle = 0;
            object->carried_object_basis[0].x = 0.0f;
            object->carried_object_basis[0].y = -minimum.y;
            object->carried_object_basis[0].z = -minimum.z;
        } else if (magnitude > 0x6000) {
            object->carried_object_angle = 0x8000;
            object->carried_object_basis[0].x = 0.0f;
            object->carried_object_basis[0].y = -minimum.y;
            object->carried_object_basis[0].z = maximum.z;
        } else if (difference < 0) {
            object->carried_object_angle = 0xc000;
            object->carried_object_basis[0].x = 0.0f;
            object->carried_object_basis[0].y = -minimum.y;
            object->carried_object_basis[0].z = maximum.x;
        } else {
            object->carried_object_angle = 0x4000;
            object->carried_object_basis[0].x = 0.0f;
            object->carried_object_basis[0].y = -minimum.y;
            object->carried_object_basis[0].z = -minimum.x;
        }
        object->carried_object_basis[1].x = 0.0f;
        object->carried_object_basis[1].y = object->carried_object_basis[0].y + (maximum.y - minimum.y) * 0.5f;
        object->carried_object_basis[1].z = object->carried_object_basis[0].z;
        object->apiobj.movement_facing_angle = angle - object->carried_object_angle;
    } else {
        NUMTX matrix = blowup->transform;
        NuMtxRotateY(&matrix, 0x8000 - object->apiobj.movement_facing_angle);
        object->carried_object_basis[0] = *NUMTX_GET_ROW_VEC(&matrix, 0);
        object->carried_object_basis[1] = *NUMTX_GET_ROW_VEC(&matrix, 1);
        object->carried_object_basis[2] = *NUMTX_GET_ROW_VEC(&matrix, 2);
    }
    object->field_0x768 = 0.5f;
}

// Original 0x4fbfa0, 4691 bytes. Original carry state machine.
void SuperCarry_MoveCode(WORLDINFO_s *world, GameObject_s *object) {
    if (LEGOCONTEXT_SUPERCARRY == -1)
        return;
    if (object->character_context != LEGOCONTEXT_SUPERCARRY) {
        if (!SuperCarry_Possible(object, 1))
            return;
        NUVEC forward;
        NuVecRotateY(&forward, &v001, object->apiobj.movement_facing_angle);
        if (world->gizmo_blowups == NULL)
            return;
        GIZMOBLOWUP_s *nearest = NULL;
        f32 best = 100000.0f;
        GIZMOBLOWUP_s *blowup = world->gizmo_blowups;
        for (i32 i = 0; i < world->gizmo_blowup_count; ++i, ++blowup) {
            if ((blowup->status_flags & 0x80c001) != 0x80c000 || (blowup->secondary_flags & 1) == 0)
                continue;
            if ((blowup->draw_flags & 0x20) != 0 && ShadowMode == 0)
                continue;
            if (blowup->platform_id != -1 && blowup->platform_id == object->field_0x1078)
                continue;
            NUVEC delta;
            f32 distance = NuVecDistSqr(&blowup->mid_position, &object->apiobj.collision_position, &delta);
            f32 radius = object->apiobj.field_0x1dc + blowup->target_scale + 0.2f;
            if (distance < best && distance < radius * radius && delta.x * forward.x + delta.z * forward.z > 0.0f) {
                nearest = blowup;
                best = distance;
            }
        }
        if (nearest == NULL)
            return;
        const i32 player = object->apiobj.field_0x27c;
        if (static_cast<u8>(player) <= 1) {
            ADDGAMEMSG message = AddGameMsg_Default;
            message.text = LEGOASCII_DOWN != NULL ? LEGOASCII_DOWN : txt_UNKNOWN;
            message.position = &nearest->mid_position;
            message.flags = 0x87;
            message.field_0x4f = 4;
            message.scale = 2.0f;
            if (Player[0] != NULL && (Player[0]->apiobj.flags_low & 0x80) != 0 && Player[1] != NULL &&
                (Player[1]->apiobj.flags_low & 0x80) != 0)
                message.field_0x4f = player == 0 ? 12 : 6;
            message.red = PlayerRGB[player][0];
            message.green = PlayerRGB[player][1];
            message.blue = PlayerRGB[player][2];
            const u16 angle =
                static_cast<u16>(static_cast<i32>(NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f) * 2.0f * 65536.0f));
            message.alpha = static_cast<u8>(static_cast<i32>(80.0f + 48.0f * NuTrigTable[angle >> 1]));
            AddGameMsg(&message);
        }
        if ((object->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0)
            SuperCarry_Start(object, nearest, 0);
        return;
    }
    f32 *frame = NULL;
    if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL) {
        frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
        if (frame == NULL)
            return;
    }
    switch (object->field_0x7a3) {
        case 0: {
            bool event = false;
            if ((object->context_flags & 0x40) == 0) {
                GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
                object->apiobj.movement_facing_angle =
                    NuAtan2D(blowup->mid_position.x - object->apiobj.collision_position.x,
                             blowup->mid_position.z - object->apiobj.collision_position.z);
                if (frame != NULL) {
                    f32 marker = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                    event = marker >= 1.0f && *frame >= marker;
                }
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                object->field_0x7a3 = 2;
                object->context_animation = LEGOACT_SUPERCARRY_IDLE;
                if ((object->context_flags & 0x40) == 0)
                    event = true;
            }
            if (event) {
                object->context_flags |= 0x40;
                GizmoBlowupBlowup(static_cast<GIZMOBLOWUP_s *>(object->field_0x788), 0, 7, -1, NULL, 1);
                NewBuzzFrames(object->pad_gamepad->pad, 1, 0);
            }
            return;
        }
        case 1: {
            bool event = false;
            if ((object->context_flags & 0x40) == 0) {
                if (frame != NULL) {
                    f32 marker = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                    event = marker >= 1.0f && *frame >= marker;
                } else {
                    event = SuperCarry_PutDownDrop != 0;
                }
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                object->character_context = -1;
                if ((object->context_flags & 0x40) == 0)
                    event = true;
            }
            if (event) {
                object->context_flags |= 0x40;
                NewBuzzFrames(object->pad_gamepad->pad, 1, 0);
                GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
                if (SuperCarry_PutDownDrop != 0) {
                    blowup->state_flags |= 0x80;
                    blowup->visibility_flags |= 0x40;
                    GizBlowup_Respawn(blowup);
                    blowup->state_flags |= 1;
                    f32 distance;
                    PLUG *plug = Plug_FindNearest(WORLD->plug_sys, &object->carried_object_drop_position, &distance, 1);
                    if (plug != NULL && distance < 0.5625f) {
                        Plug_MakeDrawMtx(plug, &blowup->transform);
                        plug->flags |= 4;
                        blowup->state_flags &= static_cast<u8>(~0x80);
                        blowup->field_0x9f |= 0x10;
                    } else {
                        SuperCarry_Throw(object, 2);
                    }
                } else {
                    SuperCarry_TurnBlowupBackOn(blowup, &object->carried_object_drop_position,
                                                object->carried_object_angle + object->apiobj.movement_facing_angle, 0);
                }
            }
            return;
        }
        case 2:
        case 3: {
            object->context_animation_timer += FRAMETIME;
            if (object->apiobj.field_0x27d != 0) {
                bool throw_object = false, put_down = false;
                if (SuperCarry_UseActionButton != 0) {
                    if ((object->pad_gamepad->buttons_pressed & GAMEPAD_ACTION) != 0)
                        throw_object = true;
                    else
                        put_down = (object->pad_gamepad->buttons_pressed & GAMEPAD_SPECIAL) != 0;
                } else if ((object->pad_gamepad->buttons_pressed & GAMEPAD_SPECIAL) != 0) {
                    throw_object = object->pad_gamepad->input_magnitude > 0.0f;
                    put_down = !throw_object;
                }
                if (throw_object || put_down) {
                    if ((throw_object || SuperCarry_UseActionButton == 0) && SuperCarry_Bash != 0 &&
                        LEGOACT_SUPERCARRY_BASH != -1 &&
                        object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_BASH] != NULL) {
                        objopponent_ignoreaiopponent = 1;
                        object->force_target = ObjOpponent(object, 1.0f, PUNCHGAP, 1, 0, 1);
                        if (object->force_target == NULL) {
                            objopponent_ignoreaiopponent = 1;
                            object->force_target = ObjOpponent(object, 1.0f, PUNCHGAP, 1, 0, 2);
                        }
                        if (object->force_target != NULL) {
                            object->field_0x7a3 = 5;
                            object->context_animation = LEGOACT_SUPERCARRY_BASH;
                            object->context_flags &= static_cast<u8>(~0x40);
                            f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
                            object->context_animation_timer = duration <= 0.0f ? 0.5f : duration;
                            return;
                        }
                    }
                    if (throw_object) {
                        if (LEGOACT_SUPERCARRY_THROW != -1 &&
                            object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_THROW] != NULL) {
                            object->field_0x7a3 = 4;
                            object->context_animation = LEGOACT_SUPERCARRY_THROW;
                            object->context_flags &= static_cast<u8>(~0x40);
                            f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
                            object->context_animation_timer = duration <= 0.0f ? 0.5f : duration;
                        } else {
                            object->character_context = -1;
                            object->field_0xe22 |= 0x80;
                            NewRumble(object->pad_gamepad->pad, 0.6f, 0);
                        }
                        return;
                    }
                    GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
                    if (put_down && (blowup->secondary_flags & 4) != 0 && LEGOACT_SUPERCARRY_PUTDOWN != -1 &&
                        object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_PUTDOWN] != NULL &&
                        object->apiobj.field_0x218 != 2000000.0f &&
                        object->apiobj.lower_position.y - object->apiobj.field_0x218 < 0.1f) {
                        object->context_animation = LEGOACT_SUPERCARRY_PUTDOWN;
                        object->field_0x7a3 = 1;
                        object->context_flags &= static_cast<u8>(~0x40);
                        f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
                        object->context_animation_timer = duration <= 0.0f ? 0.5f : duration;
                        object->carried_object_drop_position.x = object->apiobj.lower_position.x;
                        object->carried_object_drop_position.y = object->apiobj.field_0x218;
                        object->carried_object_drop_position.z = object->apiobj.lower_position.z;
                        NUVEC offset;
                        NuVecRotateY(&offset, &object->carried_object_basis[0], object->apiobj.field_0x276);
                        object->carried_object_drop_position.x += offset.x;
                        object->carried_object_drop_position.z += offset.z;
                        return;
                    }
                }
            }
            if (SuperCarry_Jump != 0 && LEGOACT_SUPERCARRY_JUMP != -1 &&
                (object->pad_gamepad->buttons_pressed & GAMEPAD_JUMP) != 0 && object->apiobj.field_0x27d != 0) {
                object->field_0x7a3 = 6;
                object->context_animation = LEGOACT_SUPERCARRY_JUMP;
                object->context_animation_timer = 0.0f;
                object->apiobj.velocity.y = SUPERCARRY_JUMPSPEED;
                f32 duration;
                if (object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_JUMP] != NULL)
                    duration = AnimDuration(object->id, LEGOACT_SUPERCARRY_JUMP, 0.0f, 0.0f, 0);
                else
                    duration = (-SUPERCARRY_JUMPSPEED / object->apiobj.character_data->game_character->gravity) * 2.0f;
                object->apiobj.field_0x27d = 0;
                object->field_0x105c = 0;
                object->context_variant_flags &= 0x7f;
                object->airborne_action_duration = duration + 0.1f;
                object->jump_start_height = object->apiobj.collision_min.y;
                ResetAnimPacket(&object->apiobj.anim_packet, -1);
            }
            goto idle_or_walk;
        }
        case 4: {
            if (object->context_animation_timer <= 0.0f && (object->context_flags & 0x40) != 0) {
                object->character_context = -1;
                return;
            }
            bool event = false;
            if (frame != NULL && (object->context_flags & 0x40) == 0) {
                f32 marker = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                event = marker >= 1.0f && *frame >= marker;
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                if ((object->context_flags & 0x40) == 0)
                    event = true;
                else
                    object->character_context = -1;
            }
            if (!event)
                return;
            object->context_flags |= 0x40;
            object->field_0xe22 |= 0x80;
            NewRumble(object->pad_gamepad->pad, 0.6f, 0);
            return;
        }
        case 5: {
            if (object->context_animation_timer <= 0.0f && (object->context_flags & 0x40) != 0) {
                object->character_context = -1;
                return;
            }
            bool event = false;
            if (frame != NULL && (object->context_flags & 0x40) == 0) {
                f32 marker = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                event = marker >= 1.0f && *frame >= marker;
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                if ((object->context_flags & 0x40) == 0)
                    event = true;
                else
                    object->character_context = -1;
            }
            if (!event)
                return;
            object->context_flags |= 0x40;
            if (ObjOpponentStillThere(object, object->force_target, PUNCHGAP)) {
                ObjHitObj(object, object->force_target, 1, 0x2000, 0, 1);
            } else if (object->character_context != -1 && object->context_animation_timer > 0.0f) {
                object->context_flags &= static_cast<u8>(~0x40);
            }
            if ((object->context_flags & 0x40) != 0) {
                NUVEC position;
                SuperCarry_GetObjectPos(object, &position, NULL);
                GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
                GizmoBlowUp_AddEffects(&position, blowup, 0, blowup->field_0xb8 > 0.0f ? 11 : 3, NULL);
                NewRumble(object->pad_gamepad->pad, 0.6f, 0);
                NewBuzzFrames(object->pad_gamepad->pad, 2, 0);
            }
            return;
        }
        case 6: {
            object->context_animation_timer += FRAMETIME;
            if (object->context_animation_timer >= object->airborne_action_duration)
                object->context_variant_flags = static_cast<i8>(object->context_variant_flags | 0x80);
            if (object->context_animation_timer < 0.1f || object->apiobj.field_0x27d == 0)
                return;
            if (object->context_variant_flags < 0) {
                object->field_0x7a3 = 7;
                if (LEGOACT_SUPERCARRY_FALLLAND != -1 &&
                    object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_FALLLAND] != NULL) {
                    object->context_animation = LEGOACT_SUPERCARRY_FALLLAND;
                    goto landing;
                }
            } else if (object->pad_gamepad->input_magnitude != 0.0f) {
                goto idle_or_walk;
            }
            object->field_0x7a3 = 7;
            object->context_animation = LEGOACT_SUPERCARRY_LAND;
            if (object->context_animation == -1)
                goto idle_or_walk;
        landing:
            f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
            object->context_animation_timer = duration <= 0.0f ? 0.5f : duration;
            ResetAnimPacket(&object->apiobj.anim_packet, -1);
            object->apiobj.velocity.x = object->apiobj.velocity.z = 0.0f;
            NewBuzzFrames(object->pad_gamepad->pad, object->context_animation == LEGOACT_SUPERCARRY_FALLLAND ? 2 : 1,
                          0);
            return;
        }
        case 7:
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f)
                goto idle_or_walk;
            if (object->context_variant_flags >= 0 && object->pad_gamepad->input_magnitude > 0.0f)
                goto idle_or_walk;
            return;
        default:
            return;
    }
idle_or_walk:
    if (object->pad_gamepad->input_magnitude > 0.0f) {
        object->field_0x7a3 = 3;
        object->context_animation = LEGOACT_SUPERCARRY_WALK;
    } else {
        object->field_0x7a3 = 2;
        object->context_animation = LEGOACT_SUPERCARRY_IDLE;
    }
}
