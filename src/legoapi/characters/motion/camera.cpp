#include "legoapi/world/area.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/props/system/socksys.h"
#include "legoapi/render/fx/spline_position.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "legoapi/menus/screens/gamemenuall.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/gizmos/object/newblowup.h"
#include "globals.h"
#include <stdlib.h>
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/gizmos/object/technos.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"

void Minicam_InitSystem(void);
void GameCam_ResetLookRot(GAMECAMERA_s *camera);
extern i32 newgamecam;
i32 GetMenuID(void);
i32 OnOrInsidePlane(NUVEC *point, NUVEC *plane_point, NUVEC *plane_normal, NUVEC *corrected_point, f32 normal_offset,
                    f32 *distance_out);
void Surface_Deflect(NUVEC *normal, NUVEC *movement, NUVEC *result, i32 mode);
void SpecialMove_Cancel(GameObject_s *object);
void Hint_CancelCurrent(void);
extern AREADATA_s *PODSPRINT_ADATA;
extern AREADATA_s *BATTLEOVERCORUSCANT_ADATA;

// Replaces VIEWCAM_s's five unresolved words without changing its target ABI.
struct VIEWCAM_s {
    i32 mode;
    NUVEC target;
    i16 pitch;         // 0x10
    i16 yaw;           // 0x12
    f32 distance;      // 0x14
    f32 target_height; // 0x18
    f32 zoom_scale;    // 0x1c
    f32 field_0x20;
    GAMEPAD_s *gamepad; // 0x24
};
VIEWCAM_s ViewCam = {0, {1000000000.0f, 0.0f, 0.0f}, -8192, -29500, 2.66f, 0.0f, 120.0f, 200.0f, NULL};
// Original data symbol testmovef is float0.005 at 0x617cb0.
f32 testmovef = 0.005f;

DECOMP_ASSERT(sizeof(VIEWCAM_s) == 0x28, "ViewCam size");
DECOMP_ASSERT(offsetof(VIEWCAM_s, pitch) == 0x10, "ViewCam pitch offset");
DECOMP_ASSERT(offsetof(VIEWCAM_s, gamepad) == 0x24, "ViewCam gamepad offset");

void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode) {
    if (camera == NULL) {
        camera = GameCam;
    }
    if (duration <= 0.0f || camera->mode == -1) {
        return;
    }

    camera->blend_start_pitch = camera->desired_pitch;
    camera->blend_start_yaw = camera->desired_yaw;
    camera->blend_start_roll = camera->desired_roll;
    camera->blend_mode = mode < 1 ? 1 : 2;
    camera->previous_camera_mode = camera->previous_mode;

    camera->blend_start_position = camera->desired_position;
    camera->blend_end_position = camera->desired_position;
    camera->blend_start_target = camera->target;
    camera->blend_end_target = camera->target;

    camera->reset_blend = 1;
    camera->blend_time = 0.0f;
    camera->blend_duration = duration;
    camera->blend_curve = curve;
}

void GameCam_Reset(GAMECAMERA_s *camera) {
    if (camera == NULL) {
        camera = GameCam;
    }

    camera->sock_position.location.sock = -1;
    camera->sock_position.location.segment = -1;
    camera->previous_mode = -1;
    camera->judder_time = 0.0f;
    ObstacleCamSpl = NULL;
    camera->shake_amplitude = 0.0f;
    MiniCutCam = 0;
    camera->shake_target_amount = 0.0f;
    camera->mode = -1;
    camera->shake_time = 0.0f;
    camera->shake_speed = 1.0f;
    camera->blend_duration = 0.0f;
    camera->blend_time = 0.0f;
    camera->position_seek = static_cast<f32>(static_cast<u8>(WORLD->current_level->cam_pos_seek));
    camera->field_0x1b8 = 0.0f;
    camera->angle_seek = static_cast<f32>(static_cast<u8>(WORLD->current_level->cam_angle_seek));
    camera->field_0x1ec = 0.0f;
    camera->field_0x1e8 = 0.0f;
    camera->field_0x1f4 = 0.0f;
    camera->field_0x1f0 = 0.0f;
    GameCam_ResetLookRot(camera);
    Minicam_InitSystem();
}

void GameCam_Judder(GAMECAMERA_s *camera, float amount, i32 axis, nuvec_s *source) {
    if (camera == NULL) {
        camera = GameCam;
    }

    const f32 absolute_amount = NuFabs(amount);
    if (absolute_amount <= camera->judder_time) {
        return;
    }

    f32 attenuated_amount = absolute_amount;
    if (source != NULL) {
        const f32 distance = NuVecDist(&camera->pos, source, NULL);
        const f32 maximum_distance = static_cast<f32>(static_cast<u8>(WORLD->current_level->camera_judder_distance));
        if (distance >= maximum_distance) {
            return;
        }
        attenuated_amount *= (maximum_distance - distance) / maximum_distance;
        if (attenuated_amount <= camera->judder_time) {
            return;
        }
    }

    camera->judder_reverse = amount < 0.0f;
    camera->judder_axis = static_cast<u8>(axis);
    camera->judder_duration = attenuated_amount;
    camera->judder_time = attenuated_amount;
}

void GameCam_HitRoll() {
    const f32 amount = qrand() > 0x7fff ? 0.25f : -0.25f;
    GameCam_Judder(GameCam, amount, 2, NULL);
}

void GameCam_NewShake(GAMECAMERA_s *camera, float amount, float duration, float speed) {
    if (camera == NULL) {
        camera = GameCam;
    }
    camera->shake_target_amount = amount;
    camera->shake_time = duration;
    camera->shake_speed = speed;
}

void GameCam_HitJudder() {
    f32 amount = VehicleArea != 0 ? -0.3f : -0.2f;
    if (qrand() <= 0x7fff) {
        amount = -amount;
    }
    GameCam_Judder(GameCam, amount, qrand() / 0x5556, NULL);
}

void GameCam_UpdateShake(GAMECAMERA_s *camera, float ambient_amount) {
    if (camera == NULL) {
        camera = GameCam;
    }

    if (camera->shake_time > 0.0f) {
        camera->shake_time -= FRAMETIME;
    }
    const f32 target_amount = camera->shake_time > 0.0f ? camera->shake_target_amount : MAX(ambient_amount, 0.0f);
    camera->shake_amplitude = SeekLinearF(camera->shake_amplitude, target_amount, FRAMETIME * 2.0f);

    constexpr f32 kShakeRadius = 0.1f;
    constexpr f32 kShakeRetargetDistanceSquared = 0.000625f;
    if (NuVecDistSqr(&camera->shake_direction, &camera->shake_target, NULL) < kShakeRetargetDistanceSquared) {
        camera->shake_target = {0.0f, static_cast<f32>(qrand()) * (1.0f / 65535.0f) * kShakeRadius, 0.0f};
        NuVecRotateZ(&camera->shake_target, &camera->shake_target, static_cast<NUANG>(qrand()));
        camera->shake_target.x *= 4.0f / 3.0f;
    }

    const bool forced_shake = camera->shake_time > 0.0f;
    SeekVec(&camera->shake_direction, &camera->shake_direction, &camera->shake_target,
            forced_shake ? camera->shake_speed * 10.0f : 2.0f);
    SeekVec(&camera->shake_offset, &camera->shake_offset, &camera->shake_direction,
            forced_shake ? camera->shake_speed * 2.0f : 2.0f);

    NUVEC scaled_offset;
    NuVecScale(&scaled_offset, &camera->shake_offset, camera->shake_amplitude);
    constexpr f32 kShakeAngleScale = 75.0f;
    const i32 pitch = static_cast<i32>(kShakeAngleScale * camera->shake_amplitude * (scaled_offset.y / kShakeRadius));
    const i32 yaw = static_cast<i32>(kShakeAngleScale * camera->shake_amplitude * (scaled_offset.x / kShakeRadius));
    NuMtxPreRotateX(&camera->render_mtx, pitch);
    NuMtxPreRotateY(&camera->render_mtx, yaw);
}

void GameCam_ResetLookRot(GAMECAMERA_s *camera) {
    if (camera == NULL) {
        camera = GameCam;
    }

    camera->field_0x214 = 0.0f;
    camera->field_0x218 = 0.0f;
    camera->field_0x20c = 0.0f;
    camera->field_0x210 = 0.0f;
    camera->field_0x204 = 0.0f;
    camera->field_0x208 = 0.0f;
}

static bool GameCam_AddPlayerLookRot(GAMECAMERA_s *camera, GameObject_s *object) {
    if (object == NULL || static_cast<i8>(object->apiobj.flags_low) >= 0 || object->pad_gamepad == NULL ||
        object->pad_gamepad->pad == NULL) {
        return false;
    }

    i32 look_source = 1;
    if (GameCam_ObjLookingWithLeftStick != NULL) {
        look_source = GameCam_ObjLookingWithLeftStick(object);
    }
    if (look_source != 1 && look_source != 2) {
        return false;
    }

    GAMEPAD_s *gamepad = object->pad_gamepad;
    constexpr f32 kLookPitch = 1820.0f;
    constexpr f32 kLookYaw = 2730.0f;

    if (gamepad->input_mode == 1) {
        const u32 horizontal = gamepad->buttons_held & (GAMEPAD_DLEFT | GAMEPAD_DRIGHT);
        const u32 vertical = gamepad->buttons_held & (GAMEPAD_DUP | GAMEPAD_DDOWN);
        if (horizontal == GAMEPAD_DLEFT) {
            camera->field_0x208 -= kLookYaw;
        } else if (horizontal == GAMEPAD_DRIGHT) {
            camera->field_0x208 += kLookYaw;
        }
        if (vertical == GAMEPAD_DUP) {
            camera->field_0x204 -= kLookPitch;
        } else if (vertical == GAMEPAD_DDOWN) {
            camera->field_0x204 += kLookPitch;
        }
        return true;
    }

    nupad_s *pad = gamepad->pad;
    const f32 analog_x = static_cast<f32>(look_source == 2 ? pad->analog_left_x : pad->analog_right_x);
    const f32 analog_y = static_cast<f32>(look_source == 2 ? pad->analog_left_y : pad->analog_right_y);
    constexpr f32 kAnalogCentre = 127.5f;
    constexpr f32 kAnalogScale = 1.0f / kAnalogCentre;
    camera->field_0x204 += (analog_y - kAnalogCentre) * kAnalogScale * kLookPitch;
    camera->field_0x208 += (analog_x - kAnalogCentre) * kAnalogScale * kLookYaw;
    return true;
}

void GameCam_UpdateLookRot(GAMECAMERA_s *camera) {
    if (camera == NULL) {
        camera = GameCam;
    }

    camera->field_0x204 = 0.0f;
    camera->field_0x208 = 0.0f;

    i32 contributing_players = 0;
    if (MiniCutCam == 0) {
        for (i32 i = 0; i < 2; ++i) {
            if (GameCam_AddPlayerLookRot(camera, Player[i])) {
                ++contributing_players;
            }
        }
    }
    if (contributing_players > 1) {
        const f32 inverse_count = 1.0f / static_cast<f32>(contributing_players);
        camera->field_0x204 *= inverse_count;
        camera->field_0x208 *= inverse_count;
    }

    camera->field_0x20c = SeekLinearF(camera->field_0x20c, camera->field_0x204, FRAMETIME * 2.0f);
    camera->field_0x210 = SeekLinearF(camera->field_0x210, camera->field_0x208, FRAMETIME * 2.0f);
    camera->field_0x214 = SeekValF(camera->field_0x214, camera->field_0x20c, 3.0f);
    camera->field_0x218 = SeekValF(camera->field_0x218, camera->field_0x210, 3.0f);
}

void GameCameraMakeMiniCut(nugspline_s *spline, f32 start, f32 end, f32 blend_in, f32 blend_out, i32 borders,
                           i32 hold_until_players_move) {
    if (spline == NULL)
        return;
    ObstacleCamSpl = spline;
    ObstacleCamStart = start;
    ObstacleCamEnd = end;
    ObstacleCamTime = 0.0f;
    ObstacleCamRotZ = 0;
    ObstacleCamBlendInTime = blend_in;
    ObstacleCamBlendOutTime = blend_out;
    ObstacleCamCutTgtPtr = NULL;
    ObstacleCamCutCamPtr = NULL;
    ObstacleCamBorders = borders;
    if (VehicleArea != 0)
        hold_until_players_move = 0;
    ObstacleCamHoldUntilPlayersMove = hold_until_players_move;
    if (borders != 0) {
        if (start == 0.0f && blend_in <= 0.0f)
            CutBorderScale = 1.0f;
    }
    if (start <= 0.0f && blend_in <= 0.0f) {
        GameCam->blend_time = GameCam->blend_duration;
        GameCam->mode = -1;
    }
    Hint_CancelCurrent();
    ObstacleCamAlwaysSnapAngles = 0;
}

void GameCameraMakeMiniCut2(nuvec_s *, nuvec_s *, i32, float, float, float, float, i32, i32, i32) {
}

void GameCameraMakeMiniCut3(u32, float, i32, i32, i32, void *, i32, nuvec_s *, float, float, float, float, float, float,
                            float, i32, nugspline_s *, char, char) {
}

u16 GameCam_GetAdjustedYRot(GAMECAMERA_s *camera) {
    if (camera == NULL) {
        camera = GameCam;
    }
    return static_cast<u16>(camera->input_yaw + static_cast<i32>(camera->field_0x218));
}

extern "C" {

    extern NUVEC *CutoffCameraVec;

    f32 CameraEmitterDistance(NUVEC *position) {
        if (CutoffCameraVec != NULL)
            return NuVecDist(position, CutoffCameraVec, NULL);
        return 0.0f;
    }

} // extern "C"

extern "C" i16 id_GRABCONTROL, id_GRABR2CONTROL, id_LANDSPEEDER, id_ATST, id_DEWBACK;
extern f32 MainRenderTime, newgamecamtime, cutscenecam_focalLength, CamStopBlend, EMPERORFIGHTA_CAMDYHACK;
extern i32 CUTCAMONLY, CUTCAM, NewMode, GUNSHIPAHACK, netcamera, complexsockposition_forcesock,
    movegamecamera_forcesock;
extern i32 LevFlag[4];
extern NUMTX cutscenecammtx, CutCamMtx;
extern u8 set_cutscenecammtx;
extern NUGSPLINE *hub_minikitviewer_camspl;
extern AREADATA_s *PODRACE_ADATA;
f32 getPodRoll(i32);
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
f32 PodSprint_InStartCountdown(WORLDINFO_s *);
i32 Players_AveragePos(NUVEC *, SOCKPOSITION_s *);
GameObject_s *FindNearestGameObject(NUVEC *, GameObject_s *, u32, f32, f32, i32, i32, i32, f32 *, i32,
                                    i32 (*)(GameObject_s *), bool);
void Minicam_Update();
void Customiser_GetActiveWeirdoIndex(i32 *, i32 *);
i32 ObjInNarrowSock(GameObject_s *, SOCKSYS *, i32);
void MakePlayPlanes(GAMECAMERA_s *);
i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);

static NUVEC GunshipAOffset = {12.0f, 22.5f, -12.0f};
NUVEC CustomisePos[2];
f32 HUB_MINIKITVIEWER_CAMDY = 0.3f;
f32 PodCamDist = 0.0f;
NUVEC ObstacleCamCutPts[2];
f32 EMPERORFIGHTA_CAMDYHACK = 0.35f;
i32 movegamecamera_forcesock = -1;
NUMTX CutCamMtx;
// Camera-only views of the context-specific data read by PlayerCamPos.
struct CAMERA_CONTEXT_PATH_s {
    u8 reserved_00[0x40];
    nugspline_s *path;
    u8 reserved_44[8];
    u8 flags;
};
struct CAMERA_CONTEXT_FOCUS_s {
    u8 reserved_00[0x28];
    NUVEC focus;
};
DECOMP_ASSERT(offsetof(CAMERA_CONTEXT_PATH_s, path) == 0x40, "Camera context path offset");
DECOMP_ASSERT(offsetof(CAMERA_CONTEXT_PATH_s, flags) == 0x4c, "Camera context flags offset");
NUVEC *Technos_TgtPos(TECHNO_s *);
i32 Grapple_LookAtPos(GameObject_s *, NUVEC *);
void CentreTwoPlayerCamera(NUVEC *, NUVEC *, NUVEC *, NUVEC *);

static void PlayerCamPos(GameObject_s *object, NUVEC *position, NUVEC *reference) {
    CHARACTERDATA *character = object->apiobj.character_data;
    f32 height = (character->field15_0x34 + character->field16_0x38) * character->field17_0x3c * 0.5f;
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    NUVEC direction, focus;
    NUVEC *target;
    if ((object->apiobj.field_0x287 != 0 || object->character_context == 0x2b) &&
        (WORLD->current_level->flags & 0x40000) == 0) {
        if (object->apiobj.field_0x287 != 0)
            *position = object->apiobj.position;
        else
            *position = object->apiobj.field_0x1c0;
        position->y += height;
        if (VehicleArea != 0 && static_cast<i8>(object->apiobj.flags_low) < 0 && object->field_0x101c > 0.0f) {
            f32 blend = 1.0f - object->field_0x101c * 0.5f;
            if (!(blend < 0.5f)) {
                blend =
                    1.0f - (NU_SIN_LUT(static_cast<i32>((blend - 0.5f) * 2.0f * 32768.0f + 16384.0f)) + 1.0f) * 0.5f;
                blend = 1.0f - blend;
            } else
                blend = NU_SIN_LUT(static_cast<i32>(blend * 2.0f * 16384.0f));
            position->x += object->apiobj.velocity.x * blend;
            position->z += object->apiobj.velocity.z * blend;
        }
        return;
    }
    if (object->character_context == 0x0f && object->field_0x788 != NULL &&
        (static_cast<CAMERA_CONTEXT_PATH_s *>(object->field_0x788)->flags & 0x10) != 0) {
        NUVEC *points = static_cast<CAMERA_CONTEXT_PATH_s *>(object->field_0x788)->path->pts;
        NuVecSub(&direction, &points[0], &points[3]);
        NuVecSub(&focus, &points[0], &object->apiobj.position);
        f32 scale = 1.0f / NuVecMagVU0(&direction);
        direction.x = __builtin_fabsf(points[3].x - points[0].x) * scale;
        direction.y = __builtin_fabsf(points[3].y - points[0].y) * scale;
        direction.z = __builtin_fabsf(points[3].z - points[0].z) * scale;
        position->x = points[0].x - direction.x * focus.x;
        position->y = points[0].y - direction.y * focus.y;
        position->z = points[0].z - direction.z * focus.z;
        return;
    }
    position->x = object->apiobj.position.x;
    position->y = object->apiobj.position.y + height;
    position->z = object->apiobj.position.z;
    target = NULL;
    if (object->cable != NULL && object->cable->target != NULL)
        target = &object->cable->target->apiobj.collision_position;
    switch (object->character_context) {
        case 8:
            if (object->gizforce_target != NULL)
                target = &object->gizforce_target->position;
            break;
        case 0x1b:
            if (object->field_0x780 != NULL)
                target = &static_cast<GameObject_s *>(object->field_0x780)->apiobj.collision_position;
            break;
        case 0x1d:
            if (object->force_part != NULL && (object->force_part->active & 1) != 0)
                target = &object->force_part->position;
            break;
        case 0x2d:
            if (object->field_0x788 != NULL)
                target = &static_cast<GIZBUILDIT_s *>(object->field_0x788)->position;
            break;
        case 0x46:
            if (Grapple_LookAtPos(object, &focus) != 0)
                target = &focus;
            break;
        case 0x51:
            target = Technos_TgtPos(static_cast<TECHNO_s *>(object->field_0x788));
            if (target != NULL) {
                NuVecSub(&focus, target, &object->apiobj.position);
                NuVecScale(&focus, &focus, static_cast<TECHNO_s *>(object->field_0x788)->scale);
                NuVecAdd(&focus, &focus, &object->apiobj.position);
                *position = focus;
                target = NULL;
            }
            break;
        case 0x59:
            if (object->field_0x788 != NULL)
                target = &static_cast<CAMERA_CONTEXT_FOCUS_s *>(object->field_0x788)->focus;
            break;
    }
    if ((object->id == id_GRABCONTROL || object->id == id_GRABR2CONTROL) && world->grabber != NULL) {
        direction.x = (position->x + world->grabber->grab_position.x) * 0.5f;
        direction.y = position->y;
        direction.z = (position->z + world->grabber->grab_position.z) * 0.5f;
        target = &direction;
    }
    if (target != NULL)
        CentreTwoPlayerCamera(position, position, target, reference);
}

// Function prefix and mode selection. MainRenderTime guard precedes cutscenes.
void MoveGameCamera(GAMECAMERA_s *camera) {
    i32 shared_count = 0;
    NUVEC player_focus[2], player_positions[2];
    GameObject_s *camera_players[2];
    i32 player_roll[2], vehicle_player[2];
    u16 player_yaw[2];
    f32 walker_distance_sq = 0.0f;
    f32 blend_duration = 0.5f;
    f32 camera_shake = 0.0f;
    MiniCutCam = 0;
    nucamera_farclip_hack = WORLD->current_level->data_display.unknown_00;
    i32 menu_id = GetMenuID();
    if (MainRenderTime < 1.0f) {
        camera->render_mtx = numtx_identity;
        camera->mtx = camera->render_mtx;
        pNuCam->mtx = camera->render_mtx;
        NuCameraSet(pNuCam);
        GameCam_ResetLookRot(camera);
        return;
    }
    if (CutSceneCameraCTRL && (CUTSTOPGAME || CUTCAMONLY)) {
        camera->render_mtx = cutscenecammtx;
        camera->mtx = camera->render_mtx;
        set_cutscenecammtx = 0;
        pNuCam->mtx = camera->render_mtx;
        if (cutscenecam_focalLength > 0.0f) {
            pNuCam->fov = NuCameraFocalLenToFOV(cutscenecam_focalLength);
            pNuCam->fov *= (1.0f / NuIOS_GetAspectRatio()) / 0.75f;
        }
        NuCameraSet(pNuCam);
        CutCamMtx = camera->render_mtx;
        CUTCAM = 1;
        return;
    }
    if ((!CutSceneCameraCTRL && CUTSTOPGAME) || NewMode || NewLData != NULL) {
        GameCam_ResetLookRot(camera);
        return;
    }
    i32 previous_mode = camera->mode;
    camera->mode = -1;
    camera->previous_mode = previous_mode;
    i8 previous_candidate_count = camera->sock_position.candidate_count;
    i8 previous_sock = camera->sock_position.location.sock;
    if (ViewCam.gamepad != NULL)
        camera->mode = 5;
    if (newgamecam) {
        newgamecamtime += FRAMETIME;
        if (newgamecamtime >= 10.0f || WORLD->camera_splines[6] == NULL || WORLD->camera_splines[7] == NULL)
            newgamecam = 0;
    }
    bool choose_fallback = false;
    if (menu_id == 14 && hub_minikitviewer_camspl != NULL)
        camera->mode = 9;
    else if (menu_id == 8 && WORLD->camera_splines[24] != NULL)
        camera->mode = 6;
    else if (SHOPACTIVE && shopcampos != NULL && shopcamlookat != NULL)
        camera->mode = 8;
    else if (menu_id == 12 && WORLD->camera_splines[15] != NULL)
        camera->mode = 7;
    else if (ObstacleCamSpl != NULL && (ObstacleCamTime <= ObstacleCamEnd || ObstacleCamHoldUntilPlayersMove)) {
        f32 old_time = ObstacleCamTime;
        if (ObstacleCamEnd > ObstacleCamTime)
            ObstacleCamTime += FRAMETIME;
        if (ObstacleCamTime >= ObstacleCamEnd) {
            ObstacleCamTime = ObstacleCamEnd;
            if (ObstacleCamHoldUntilPlayersMove) {
                camera->mode = 2;
                u32 buttons = GAMEPAD_START | GAMEPAD_JUMP | GAMEPAD_SPECIAL | GAMEPAD_ACTION | GAMEPAD_TAG |
                              GAMEPAD_TOGGLELEFT | GAMEPAD_TOGGLERIGHT;
                for (i32 i = 0; i < 2; i++) {
                    GameObject_s *object = Player[i];
                    if (object != NULL && static_cast<i8>(object->apiobj.flags_low) < 0 &&
                        (object->apiobj.model_draw_result == 0 || object->pad_gamepad->input_magnitude > 0.0f ||
                         (object->pad_gamepad->buttons_held & buttons) != 0)) {
                        ObstacleCamHoldUntilPlayersMove = 0;
                        break;
                    }
                }
            } else {
                ObstacleCamSpl = NULL;
                if (!(ObstacleCamBlendOutTime <= 0.0f))
                    blend_duration = ObstacleCamBlendOutTime;
                else {
                    GameCam_Reset(camera);
                    previous_mode = camera->mode;
                    camera->previous_mode = previous_mode;
                    previous_sock = camera->sock_position.location.sock;
                    previous_candidate_count = camera->sock_position.candidate_count;
                    CutBorderScale = 0.0f;
                }
                choose_fallback = true;
            }
        } else if (ObstacleCamTime >= ObstacleCamStart && ObstacleCamEnd > ObstacleCamTime) {
            camera->mode = 2;
            if (ObstacleCamStart > 0.0f && ObstacleCamStart > old_time && ObstacleCamBlendInTime <= 0.0f) {
                camera->blend_duration = 0.0f;
                camera->blend_time = 0.0f;
                camera->previous_mode = -1;
                CutBorderScale = 1.0f;
            }
        } else
            choose_fallback = true;
    } else if (Door_UseCutCam && Door_CutLookAtPlayers && Door_CutCamWait > 0.0f) {
        Door_CutCamWait -= FRAMETIME;
        camera->mode = 4;
    } else
        choose_fallback = true;
    if (choose_fallback && camera->mode == -1) {
        camera->mode = 0;
        if (Door_UseCutCam && GameTimer.update_count == 0)
            camera->mode = 4;
        else if (WORLD->current_level == TITLES_LDATA && WORLD->camera_splines[2] != NULL)
            camera->mode = 3;
        else if (PLAYERCOUNT > 0 && WORLD->sock_sys != NULL) {
            camera->mode = 1;
            if (GUNSHIPAHACK && WORLD->current_level == BONUS_GUNSHIPA_LDATA && WORLD->sock_sys->sock[0].valid)
                camera->mode = 11;
        }
    }
    if (camera->mode != previous_mode) {
        if (previous_mode == 4) {
            GameCam_Blend(camera, Door_CutCamBlendTime, Door_CutLookAtPlayers ? 0.0f : Door_CutCamWait, 1);
            Door_UseCutCam = 0;
        } else if (previous_mode != -1) {
            f32 duration = blend_duration;
            if (camera->mode == 2)
                duration = ObstacleCamBlendInTime;
            else if (camera->mode == 9 || previous_mode == 9 || camera->mode == 6 || previous_mode == 6)
                duration = 1.5f;
            else if (camera->mode == 8 || previous_mode == 8)
                duration = 1.0f;
            else if (camera->mode == 7 || previous_mode == 7)
                duration = GAMEDEMO ? 3.0f : 1.5f;
            if (camera->previous_mode != -1 && duration != 0.0f)
                GameCam_Blend(camera, duration, 0.0f, 1);
        }
    }
    if (camera->mode == 4 || camera->mode == 2)
        MiniCutCam = camera->blend_duration > camera->blend_time ? 1 : (ObstacleCamHoldUntilPlayersMove ? 3 : 2);
    else if (camera->blend_duration > camera->blend_time &&
             (camera->previous_camera_mode == 4 || camera->previous_camera_mode == 2))
        MiniCutCam = 4;
    NUVEC position = *PlayerStart[0].pos;
    NUVEC target = v000;
    NUVEC offset, direction;
    // The per-mode camera blend duration starts afresh after transition selection.
    blend_duration = 0.5f;
    f32 position_seek = static_cast<u32>(static_cast<u8>(WORLD->current_level->cam_pos_seek));
    f32 angle_seek = static_cast<u32>(static_cast<u8>(WORLD->current_level->cam_angle_seek));
    GAMEPAD_s *selected_pad = camera->mode == 5 ? ViewCam.gamepad : &GamePad[0];
    f32 left_y = static_cast<f32>(selected_pad->pad->analog_left_y) - 127.5f;
    f32 left_x = static_cast<f32>(selected_pad->pad->analog_left_x) - 127.5f;
    f32 right_y = static_cast<f32>(selected_pad->pad->analog_right_y) - 127.5f;
    f32 right_x = static_cast<f32>(selected_pad->pad->analog_right_x) - 127.5f;
    left_y = NuFabs(left_y) < 34.0f ? 0.0f : left_y / 127.5f;
    left_x = NuFabs(left_x) < 34.0f ? 0.0f : left_x / 127.5f;
    right_y = NuFabs(right_y) < 34.0f ? 0.0f : right_y / 127.5f;
    right_x = NuFabs(right_x) < 34.0f ? 0.0f : right_x / 127.5f;
    i32 pitch_override = -1, yaw_override = -1, roll_override = 0;
    i32 roll_hint_valid = 0;
    u16 roll_hint = 0;
    f32 roll_seek_override = 0.0f, stop_blend_rate = 0.1f;
    switch (camera->mode) {

        case 1: {
            // Case 1 prefix. Original 0x111314..0x1129bf and its child branches.

            i32 player_count = 0, vehicle_count = 0;
            f32 landspeeder_speed = 0.0f, landspeeder_yaw = -1.0f, landspeeder_lookahead = 0.0f;
            for (i32 i = 0; i < 2; i++) {
                GameObject_s *object = Player[i];
                if (object == NULL || (static_cast<i8>(object->apiobj.flags_low) >= 0 && !LookAtBoth) ||
                    (netcamera && (object->apiobj.field_0x1f4 & 0x40000) != 0) ||
                    (BonusWinner != -1 && BonusWinner != i))
                    continue;
                vehicle_player[i] = object->apiobj.character_data->model_flags & 0x2000;
                if (!(SPEEDERCHASEA_LDATA != NULL && WORLD->current_level == SPEEDERCHASEA_LDATA &&
                      object->id == id_SPEEDERBIKE && disable_narrow_socks) &&
                    vehicle_player[i]) {
                    vehicle_count++;
                    if (object->id == id_LANDSPEEDER) {
                        if (landspeeder_lookahead == 0.0f) {
                            landspeeder_speed = object->apiobj.horizontal_velocity_magnitude;
                            landspeeder_lookahead = object->apiobj.horizontal_velocity_magnitude * 0.6f;
                            landspeeder_yaw = static_cast<u16>(object->apiobj.movement_facing_angle);
                        } else {
                            landspeeder_lookahead =
                                (landspeeder_lookahead + object->apiobj.horizontal_velocity_magnitude * 0.6f) * 0.5f;
                            landspeeder_yaw =
                                (landspeeder_yaw + static_cast<u16>(object->apiobj.movement_facing_angle)) * 0.5f;
                            landspeeder_speed =
                                (landspeeder_speed + object->apiobj.horizontal_velocity_magnitude) * 0.5f;
                        }
                    }
                }
                if (PODRACE_ADATA != NULL && WORLD->area == PODRACE_ADATA) {
                    player_focus[player_count] = Player[i]->apiobj.collision_position;
                    NUVEC offset = {0.0f, 0.0f, 2.0f};
                    NuVecRotateY(
                        &offset, &offset,
                        static_cast<i32>(static_cast<u16>(object->apiobj.facing_angle) + getPodRoll(i) * 5461.0f));
                    NuVecAdd(&player_focus[player_count], &player_focus[player_count], &offset);
                } else
                    PlayerCamPos(object, &player_focus[player_count], &camera->pos);
                NUVEC *source_position = &object->apiobj.position;
                if (object->character_context == 0x2b && !object->apiobj.field_0x287)
                    source_position = &object->apiobj.field_0x1c0;
                else if (object->character_context == 0x51 && object->field_0x788 != NULL) {
                    NUVEC *techno_position = Technos_TgtPos(static_cast<TECHNO_s *>(object->field_0x788));
                    if (techno_position != NULL)
                        source_position = techno_position;
                }
                player_positions[player_count] = *source_position;
                if ((object->apiobj.character_data->model_flags & 0x2000) != 0) {
                    if (PODRACE_ADATA != NULL && WORLD->area == PODRACE_ADATA)
                        player_roll[player_count] = static_cast<i32>(getPodRoll(i) * 8192.0f);
                    else
                        player_roll[player_count] = RotDiff(0, object->movement_lean_angle);
                    RotDiff(0, object->tertiary_lean_angle);
                } else
                    player_roll[player_count] = 0;
                camera_players[player_count] = object;
                player_yaw[player_count] = object->apiobj.movement_facing_angle;
                player_count++;
            }
            if (player_count != 0) {
                complexsockposition_forcesock = movegamecamera_forcesock;
                f32 separation_scale;
                i32 camera_result =
                    SockSysCamera(WORLD->sock_sys, &camera->pos, camera->mode != camera->previous_mode, player_focus,
                                  player_positions, player_count, &camera->sock_position, &position, &target,
                                  &blend_duration, &position_seek, &angle_seek, &camera_shake, &separation_scale);
                NUVEC rail_position = position;
                blend_duration *= 1.5f;
                if (WORLD->current_level->cam_pullback_dist > 0.0f) {
                    i32 angle = NuAtan2D(target.x - position.x, target.z - position.z);
                    NUVEC direction, offset;
                    NuVecSub(&direction, &position, &target);
                    direction.y = 0.0f;
                    NuVecNorm(&direction, &direction);
                    f32 pullback = 0.0f;
                    i32 delta;
                    if (vehicle_count) {
                        u16 heading;
                        if (player_count == 2 && vehicle_player[0] && vehicle_player[1])
                            heading = player_yaw[0] + RotDiff(player_yaw[0], player_yaw[1]) / player_count;
                        else if (vehicle_player[0])
                            heading = player_yaw[0];
                        else
                            heading = player_yaw[1];
                        delta = RotDiff(angle, heading);
                        if ((camera->sock_position.location.sock == -1 ||
                             (WORLD->sock_sys->sock[camera->sock_position.location.sock].flags & 0x1000) == 0) &&
                            abs(delta) > 0x4000)
                            pullback = (abs(delta) - 0x4000) * (1.0f / 16384.0f);
                    }
                    camera->field_0x1e8 = SeekLinearF(camera->field_0x1e8, pullback, FRAMETIME);
                    camera->field_0x1ec = SeekValF(camera->field_0x1ec, camera->field_0x1e8, 3.0f);
                    NuVecScale(&offset, &direction, camera->field_0x1ec * WORLD->current_level->cam_pullback_dist);
                    NuVecAdd(&position, &position, &offset);
                    NuVecAdd(&target, &target, &offset);
                    f32 lateral = 0.0f;
                    if (vehicle_count &&
                        (camera->sock_position.location.sock == -1 ||
                         (WORLD->sock_sys->sock[camera->sock_position.location.sock].flags & 0x1000) == 0)) {
                        lateral = 1.0f - abs(0x4000 - abs(delta)) * (1.0f / 16384.0f);
                        if (delta < 0)
                            lateral = -lateral;
                    }
                    camera->field_0x1f0 = SeekLinearF(camera->field_0x1f0, lateral, FRAMETIME);
                    camera->field_0x1f4 = SeekValF(camera->field_0x1f4, camera->field_0x1f0, 3.0f);
                    NuVecRotateY(&offset, &direction, 0x4000);
                    NuVecScale(&offset, &offset, (0.5f * WORLD->current_level->cam_lateral_dist) * camera->field_0x1f4);
                    NuVecSub(&position, &position, &offset);
                    NuVecSub(&target, &target, &offset);
                }
                // Original PODRACE_ADATA / PODSPRINT_ADATA branches after SockSysCamera.
                if (WORLD->area != NULL && WORLD->area == PODRACE_ADATA) {
                    f32 distance;
                    if (avg_currentspeed_mul <= 0.333f)
                        distance = 3.0f;
                    else if (avg_currentspeed_mul < 1.0f)
                        distance = 3.0f - (avg_currentspeed_mul - 0.333f) / 0.667f;
                    else
                        distance = 2.0f;
                    PodCamDist = SeekLinearF(PodCamDist, distance, FRAMETIME);
                    NUVEC average = {0.0f, 0.0f, 0.0f};
                    for (i32 index = 0; index < player_count; ++index) {
                        GameObject_s *player = camera_players[index];
                        average.x += player->apiobj.pos_x - NU_SIN_LUT(player->apiobj.facing_angle) * PodCamDist;
                        average.y += player->apiobj.pos_y + 1.0f;
                        average.z += player->apiobj.pos_z - NU_COS_LUT(player->apiobj.facing_angle) * PodCamDist;
                    }
                    if (player_count != 1)
                        NuVecScale(&average, &average, 1.0f / player_count);
                    position = average;
                    position_seek = 10.0f;
                    angle_seek = 10.0f;
                    f32 floor = GameShadow(NULL, &position, 5.0f, -1);
                    if (floor != 2000000.0f) {
                        if (EShadY != 2000000.0f)
                            floor = EShadY > floor ? EShadY : floor;
                        floor += 1.0f;
                        if (floor > position.y)
                            position.y = floor;
                    }
                    target.y = position.y - 0.5f;
                    goto rail_vehicle_focus;
                }
                if (WORLD->area != NULL && WORLD->area == PODSPRINT_ADATA) {
                    f32 countdown = PodSprint_InStartCountdown(WORLD);
                    PodCamDist = countdown > 0.0f ? countdown / 3.0f * 10.0f + 2.0f : 2.0f;
                    if (player_count == 2)
                        PodCamDist *= 1.5f;
                    NUVEC average = {0.0f, 0.0f, 0.0f};
                    for (i32 index = 0; index < player_count; ++index) {
                        GameObject_s *player = camera_players[index];
                        average.x += player->apiobj.position.x;
                        average.y += player->apiobj.collision_min.y + 0.65f;
                        average.z += player->apiobj.position.z;
                    }
                    if (player_count != 1)
                        NuVecScale(&average, &average, 1.0f / player_count);
                    target = average;
                    u16 yaw = camera_players[0]->apiobj.movement_facing_angle;
                    if (player_count == 2)
                        yaw += RotDiff(yaw, camera_players[1]->apiobj.movement_facing_angle) / player_count;
                    position.x = target.x - NU_SIN_LUT(yaw) * PodCamDist;
                    position.y = target.y + 0.65f;
                    position.z = target.z - NU_COS_LUT(yaw) * PodCamDist;
                    if (countdown <= 0.0f) {
                        i32 roll = player_roll[0];
                        if (player_count == 2)
                            roll += RotDiff((u16)roll, (u16)player_roll[1]) / player_count;
                        yaw += (i32)(roll * 0.6f);
                        target.x = position.x + NU_SIN_LUT(yaw) * PodCamDist;
                        target.z = position.z + NU_COS_LUT(yaw) * PodCamDist;
                    }
                    NUVEC floor_probe = {position.x, target.y - 0.65f, position.z};
                    f32 floor = GameShadow(NULL, &floor_probe, 5.0f, -1);
                    if (floor != 2000000.0f) {
                        if (EShadY != 2000000.0f)
                            floor = EShadY > floor ? EShadY : floor;
                        floor += 0.65f;
                        if (floor > position.y)
                            position.y = floor;
                    }
                    position_seek = 10.0f;
                    angle_seek = 7.0f;
                    goto rail_vehicle_focus;
                }

                if (WORLD->current_level == BONUS_GUNSHIPB_LDATA && camera->sock_position.location.sock != -1) {
                    position.x += target.x * 0.0625f;
                    position.z += target.z * 0.0625f;
                } else if (WORLD->current_level == EMPERORFIGHTA_LDATA && player != NULL &&
                           player->sock_position.location.sock == 11 && WORLD->current_level != NULL)
                    position.y += EMPERORFIGHTA_CAMDYHACK;
            rail_vehicle_focus:
                if (landspeeder_lookahead != 0.0f) {
                    f32 opposite = static_cast<f32>(camera->yaw - 0x8000);
                    if (opposite < 0.0f)
                        opposite += 65536.0f;
                    NUVEC offset;
                    NuVecSub(&offset, &position, &target);
                    NuVecNorm(&offset, &offset);
                    NuVecScale(&offset, &offset, landspeeder_lookahead);
                    position.y += landspeeder_lookahead;
                    if (landspeeder_yaw > opposite - 4096.0f && landspeeder_yaw < opposite + 4096.0f) {
                        position.x += NU_SIN_LUT(static_cast<i32>(opposite)) * (landspeeder_speed * 0.5f) + offset.x;
                        position.z +=
                            NU_SIN_LUT(static_cast<i32>(opposite + 16384.0f)) * (landspeeder_speed * 0.5f) + offset.z;
                    } else if (landspeeder_yaw > static_cast<f32>(camera->yaw - 0x1000) &&
                               landspeeder_yaw < static_cast<f32>(camera->yaw + 0x1000)) {
                        position.x += NU_SIN_LUT(camera->yaw) * (landspeeder_speed * 0.75f) + offset.x;
                        position.z += NU_SIN_LUT(camera->yaw + 0x4000) * (landspeeder_speed * 0.75f) + offset.z;
                    } else {
                        position.x += offset.x;
                        position.z += offset.z;
                    }
                    GameCam_Blend(GameCam, 0.75f, 0.0f, 0);
                } else {
                    bool near_walker = false;
                    if ((SPEEDERCHASEA_LDATA != NULL && WORLD->current_level == SPEEDERCHASEA_LDATA) ||
                        (ENDORBATTLEC_LDATA != NULL && WORLD->current_level == ENDORBATTLEC_LDATA)) {
                        NUVEC average_position;
                        Players_AveragePos(&average_position, NULL);
                    } else if (id_ATST != -1) {
                        GameObject_s *walker = FindNearestGameObject(&position, NULL, 0, 5.0f, 0.0f, -1, id_ATST, -1,
                                                                     &walker_distance_sq, 0, NULL, false);
                        if (walker != NULL) {
                            bool is_player = false;
                            if (Player[0] != NULL && Player[0] == walker)
                                is_player = true;
                            if (Player[1] != NULL && Player[1] == walker)
                                is_player = true;
                            if (Player[2] != NULL && Player[2] == walker)
                                is_player = true;
                            if (Player[3] != NULL && Player[3] == walker)
                                is_player = true;
                            if (Player[4] != NULL && Player[4] == walker)
                                is_player = true;
                            if (Player[5] != NULL && Player[5] == walker)
                                is_player = true;
                            if (Player[6] != NULL && Player[6] == walker)
                                is_player = true;
                            if (Player[7] != NULL && Player[7] == walker)
                                is_player = true;
                            landspeeder_lookahead = is_player ? 4.0f : 2.0f;
                            f32 distance = NuFsqrt(walker_distance_sq);
                            f32 factor = MAX(0.0f, 1.0f - distance / 5.0f);
                            factor = (1.0f + NU_SIN_LUT(static_cast<i32>(factor * 32768.0f + 16384.0f))) * 0.5f;
                            NUVEC offset;
                            NuVecSub(&offset, &position, &target);
                            NuVecNorm(&offset, &offset);
                            f32 amount = landspeeder_lookahead * (1.0f - factor);
                            NuVecScale(&offset, &offset, amount);
                            position.y += amount;
                            position.x += offset.x;
                            position.z += offset.z;
                            near_walker = landspeeder_lookahead != 0.0f;
                        }
                    }
                    if (!near_walker && id_DEWBACK != -1) {
                        if (FindNearestGameObject(&position, NULL, 0, 5.0f, 0.0f, -1, id_DEWBACK, -1,
                                                  &walker_distance_sq, 0, NULL, false) != NULL) {
                            f32 distance = NuFsqrt(walker_distance_sq);
                            NUVEC offset;
                            NuVecSub(&offset, &position, &target);
                            NuVecNorm(&offset, &offset);
                            f32 amount = 1.5f * (1.0f - distance / 5.0f);
                            NuVecScale(&offset, &offset, amount);
                            position.y += amount;
                            position.x += offset.x;
                            position.z += offset.z;
                        }
                    }
                }
                if (TATOOINEE_LDATA != NULL && WORLD->current_level == TATOOINEE_LDATA &&
                    landspeeder_lookahead == 0.0f) {
                    NUVEC average_position, offset;
                    Players_AveragePos(&average_position, NULL);
                    NuVecSub(&offset, &rail_position, &average_position);
                    f32 magnitude = NuVecMagSqr(&offset);
                    f32 blend = 0.0f;
                    if (player2 == NULL) {
                        blend = magnitude / 125.0f;
                        if (blend > 0.75f)
                            blend = 0.6f;
                    }
                    f32 scale = 1.0f + NU_SIN_LUT(static_cast<i32>(blend * 16384.0f + 32768.0f + 16384.0f));
                    NuVecScale(&offset, &offset, scale);
                    offset.y = 0.0f;
                    NuVecSub(&position, &position, &offset);
                }
                stop_blend_rate = 0.1f;
                if (WORLD->current_level == PLATFORM_LDATA) {
                    position.x +=
                        0.2f *
                        NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 13.9815f) / 13.9815f * 65536.0f));
                    position.y +=
                        stop_blend_rate *
                        NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 8.688f) / 8.688f * 65536.0f));
                    position.z +=
                        0.2f *
                        NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 11.7745f) / 11.7745f * 65536.0f));
                }
                if (WORLD->area != NULL && WORLD->area == HUB_ADATA && camera->sock_position.location.sock != -1)
                    position.y = WORLD->sock_sys->sock[camera->sock_position.location.sock].camera_height_above_ground;
                roll_hint_valid = 0;
                roll_hint = 0;
                roll_override = 0;
                roll_seek_override = 0.0f;
                if (WORLD->current_level->cam_tilt != 0.0f && camera->mode == camera->previous_mode) {
                    NUVEC forward;
                    NuVecRotateY(&forward, &v001, camera->yaw);
                    i32 total = 0;
                    for (i32 i = 0; i < player_count; i++) {
                        i32 roll = -RotDiff(0, player_roll[i]);
                        if (camera_players[i] != NULL)
                            total += static_cast<i32>(roll * (forward.x * camera_players[i]->facing_direction.x +
                                                              forward.z * camera_players[i]->facing_direction.z));
                    }
                    f32 scale = WORLD->current_level->cam_tilt;
                    if (player_count != 1)
                        scale *= 1.0f / player_count;
                    roll_hint = static_cast<i32>(total * scale);
                    roll_hint_valid = 1;
                    roll_seek_override = 1.0f;
                    roll_override = -1;
                }
                if (BonusWinner != -1) {
                    NUVEC offset;
                    f32 distance = NuVecDist(&position, &target, &offset);
                    NuVecNorm(&offset, &offset);
                    NuVecScale(&offset, &offset, distance * 0.666f);
                    NuVecAdd(&position, &target, &offset);
                }
            } else {
                roll_hint_valid = 0;
                roll_hint = 0;
                roll_override = 0;
                roll_seek_override = 0.0f;
                stop_blend_rate = 0.1f;
            }
            pitch_override = -1;
            yaw_override = -1;
            movegamecamera_forcesock = -1;

            break;
        }
        case 2: {
            Minicam_Update();
            if (ObstacleCamCutCamPtr != NULL) {
                ObstacleCamCutPts[0] = *ObstacleCamCutCamPtr;
            }
            if (ObstacleCamCutTgtPtr != NULL) {
                ObstacleCamCutPts[1] = *ObstacleCamCutTgtPtr;
            }
            position = ObstacleCamSpl->pts[0];
            target = ObstacleCamSpl->pts[1];
            if (ObstacleCamAlwaysSnapAngles != 0) {
                position_seek = 10.0f;
                angle_seek = 10.0f;
            }
            roll_override = ObstacleCamRotZ != 0 ? static_cast<u16>(ObstacleCamRotZ) : 0;
            Hint_CancelCurrent();
            roll_hint_valid = 0;
            roll_hint = 0;
            roll_seek_override = 0.0f;
            yaw_override = -1;
            pitch_override = -1;
            stop_blend_rate = 0.1f;
            break;
        }

        case 3:
            position = WORLD->camera_splines[2]->pts[0];
            target = WORLD->camera_splines[2]->pts[1];
            break;
        case 4: {
            position = Door_CutCamPos0;
            target = Door_CutCamPos1;
            if (Door_CutLookAtPlayers != 0) {
                target.x = 0.0f;
                target.y = 0.0f;
                target.z = 0.0f;
                i32 focus_count = 0;
                NUVEC player_focus;
                if (Player[0] != NULL && (static_cast<i8>(Player[0]->apiobj.flags_low) < 0 || LookAtBoth != 0)) {
                    PlayerCamPos(Player[0], &player_focus, &position);
                    NuVecAdd(&target, &target, &player_focus);
                    focus_count = 1;
                }
                if (Player[1] != NULL && (static_cast<i8>(Player[1]->apiobj.flags_low) < 0 || LookAtBoth != 0)) {
                    PlayerCamPos(Player[1], &player_focus, &position);
                    NuVecAdd(&target, &target, &player_focus);
                    ++focus_count;
                }
                if (focus_count != 0) {
                    NuVecScale(&target, &target, 1.0f / static_cast<f32>(focus_count));
                }
            }
            ComplexSockPosition(WORLD->sock_sys, &position, -1, -1, &camera->sock_position);
            roll_hint_valid = 0;
            roll_hint = 0;
            roll_seek_override = 0.0f;
            roll_override = 0;
            yaw_override = -1;
            pitch_override = -1;
            stop_blend_rate = 0.1f;
            break;
        }

        // Case 5 in MoveGameCamera. Inputs have already undergone the original
        // signed-stick conversion and deadzone handling in the common prologue.
        // `selected_pad` is the GAMEPAD_s at the original esp+0x108.
        // left_x/left_y correspond to xmm7/xmm6; right_y/right_x to esp+0xfc/+0x104.
        case 5: {
            static f32 addViewLDy = 0.0f;
            f32 speed = NuFabs(ViewCam.distance) * testmovef * FRAMETIME;
            if (ViewCam.gamepad != NULL) {
                offset.x = (left_x * 127.5f) * speed;
                offset.y = static_cast<f32>(static_cast<u32>(ViewCam.gamepad->pad->analog_l1)) * speed -
                           static_cast<f32>(static_cast<u32>(ViewCam.gamepad->pad->analog_l2)) * speed;
                offset.z = (-left_y * 127.5f) * speed;
                NuVecRotateY(&offset, &offset, camera->yaw);
                NuVecAdd(&ViewCam.target, &ViewCam.target, &offset);
                ViewCam.pitch = static_cast<i16>(ViewCam.pitch + static_cast<i32>(right_y * 16384.0f * FRAMETIME));
                ViewCam.pitch = MAX(-0x4000, MIN(0x4000, ViewCam.pitch));
                ViewCam.yaw -= static_cast<i16>(right_x * 16384.0f * FRAMETIME);
                if ((ViewCam.gamepad->pad->digital_buttons & GAMEPAD_SELECT) == 0) {
                    f32 zoom_step = 0.1f / ViewCam.zoom_scale;
                    ViewCam.distance -= static_cast<f32>(static_cast<u32>(ViewCam.gamepad->pad->analog_r1)) * zoom_step;
                    ViewCam.distance += static_cast<f32>(static_cast<u32>(ViewCam.gamepad->pad->analog_r2)) * zoom_step;
                    if ((selected_pad->pad->digital_buttons & 0x40) != 0) {
                        addViewLDy += FRAMETIME;
                    } else if ((selected_pad->pad->digital_buttons & 0x20) != 0) {
                        addViewLDy -= FRAMETIME;
                    } else {
                        addViewLDy = 0.0f;
                    }
                    ViewCam.distance += addViewLDy;
                    ViewCam.distance = NuFmax(0.1f, ViewCam.distance);
                }
            }
            stop_blend_rate = 0.1f;
            position = ViewCam.target;
            target = ViewCam.target;
            target.y += ViewCam.target_height;
            NuVecRotateX(&direction, &v001, ViewCam.pitch);
            NuVecRotateY(&direction, &direction, ViewCam.yaw);
            position.x += ViewCam.distance * direction.x;
            position.y += ViewCam.distance * direction.y;
            position.z += ViewCam.distance * direction.z;
            break;
        }

        // Mode 6, original0x111163..0x1111aa. WORLD is in esi at dispatch.
        // Shares the three sine offsets and common setup with customiser mode7;
        // it does NOT join the unmodified title-camera case3.
        case 6: {
            f32 *points = WORLD->portal_places[24]->positions;
            position.x = points[0];
            position.y = points[1];
            position.z = points[2];
            target.x = points[3];
            target.y = points[4];
            target.z = points[5];
            goto customiser_camera_sway; // original0x111012
        }

        case 7: {
            position = WORLD->camera_splines[15]->pts[0];
            target = WORLD->camera_splines[15]->pts[1];
            i32 active_index;
            Customiser_GetActiveWeirdoIndex(&active_index, &shared_count);
            if (shared_count == 1) {
                if (MenuPacket.reserved_0[active_index] != 0)
                    active_index = !active_index;
                target.x = CustomisePos[active_index].x;
                target.z = CustomisePos[active_index].z;
                NUVEC offset;
                NuVecSub(&offset, &target, &position);
                NuVecScale(&offset, &offset, 0.25f);
                NuVecAdd(&position, &position, &offset);
                target.y -= 0.075f;
            }
        customiser_camera_sway:
            position.x += NU_SIN_LUT((i32)(NuFmod(GameTimer.time_elapsed, 9.321f) / 9.321f * 65536.0f)) * 0.05f;
            position.y += NU_SIN_LUT((i32)(NuFmod(GameTimer.time_elapsed, 5.792f) / 5.792f * 65536.0f)) * 0.025f;
            position.z += NU_SIN_LUT((i32)(NuFmod(GameTimer.time_elapsed, 7.183f) / 7.183f * 65536.0f)) * 0.025f;
            roll_hint_valid = 0;
            roll_hint = 0;
            roll_seek_override = 0.0f;
            pitch_override = yaw_override = -1;
            roll_override = 0;
            stop_blend_rate = 0.1f;
            break;
        }
        case 8:
            position = *shopcampos;
            GetShopCamLookPos(&target);
            position.x +=
                0.1f * NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 9.321f) / 9.321f * 65536.0f));
            position.y +=
                0.05f * NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 5.792f) / 5.792f * 65536.0f));
            position.z +=
                0.05f * NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 7.183f) / 7.183f * 65536.0f));
            break;
        case 9:
            position = hub_minikitviewer_camspl->pts[0];
            target = hub_minikitviewer_camspl->pts[1];
            target.y += HUB_MINIKITVIEWER_CAMDY;
            position_seek = 10.0f;
            angle_seek = 10.0f;
            roll_hint_valid = 0;
            roll_hint = 0;
            roll_seek_override = 0.0f;
            pitch_override = yaw_override = -1;
            roll_override = 0;
            stop_blend_rate = 0.1f;
            break;
        case 11: {
            f32 spline_position =
                (GamePlayTimer.time_elapsed + (reinterpret_cast<u8 *>(LevFlag)[0] == 2 ? 51.0f : 6.0f)) / 100.0f;
            f32 first_position = spline_position;
            if (spline_position > 0.95f) {
                reinterpret_cast<u8 *>(LevFlag)[1] = 1;
                first_position = 0.95f;
            }
            u16 yaw;
            PointAlongSpline(WORLD->sock_sys->sock[0].mid, first_position, &target, &yaw, NULL, 0);
            NUVEC ahead;
            PointAlongSpline(WORLD->sock_sys->sock[0].mid, spline_position > 1.0f ? 1.0f : spline_position, &ahead,
                             NULL, NULL, 0);
            NUVEC lateral;
            NuVecRotateY(&lateral, &v001, yaw);
            GunshipANorm = lateral;
            NuVecRotateY(&lateral, &lateral, 0x4000);
            NUVEC offset = {0.0f, 0.0f, 0.0f};
            i32 player_count = 0;
            for (i32 index = 0; index < 2; ++index) {
                if (Player[index] != NULL) {
                    f32 projection = (Player[index]->apiobj.pos_x - target.x) * lateral.x +
                                     (Player[index]->apiobj.pos_z - target.z) * lateral.z;
                    offset.x += lateral.x * projection;
                    offset.z += lateral.z * projection;
                    ++player_count;
                }
            }
            if (player_count == 2) {
                offset.x *= 0.5f;
                offset.z *= 0.5f;
            }
            NuVecAdd(&position, &target, &GunshipAOffset);
            position.x += offset.x;
            position.z += offset.z;
            target.x = ahead.x + offset.x;
            target.z = ahead.z + offset.z;
            NuVecSub(&offset, &target, &position);
            pitch_override = -NuAtan2D(offset.y, NuFsqrt(offset.x * offset.x + offset.z * offset.z));
            yaw_override = NuAtan2D(offset.x, offset.z);
            position_seek = 8.0f;
            roll_hint_valid = 0;
            roll_hint = 0;
            roll_seek_override = 0.0f;
            roll_override = 0;
            stop_blend_rate = 0.1f;
            break;
        }

        default:
            break;
    }
    if (WORLD->current_level == SARLACCPITA_LDATA || WORLD->current_level == SARLACCPITC_LDATA) {
        position.x +=
            stop_blend_rate * NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 9.321f) / 9.321f * 65536.0f));
        position.y += 0.05f * NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 5.792f) / 5.792f * 65536.0f));
        position.z += 0.05f * NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 7.183f) / 7.183f * 65536.0f));
    }
    // Entry locals: position, target, previous_sock, previous_candidate_count,
    // blend_duration=.5f, position_seek, angle_seek, pitch_override/yaw_override=-1,
    // roll_override=0, roll_hint_valid=0, roll_hint=0, roll_seek_override=0,
    // stop_blend_rate=.1f. Caller has already copied old mode to previous_mode.
    if (camera->mode == previous_mode) {
        if (previous_sock == -1 || camera->sock_position.location.sock == -1) {
            if (previous_sock != camera->sock_position.location.sock && WORLD->current_level != DOGFIGHTA_LDATA)
                GameCam_Blend(camera, 1.0f, 0.0f, 1);
        } else if (camera->sock_position.candidate_count != previous_candidate_count ||
                   camera->sock_position.location.sock != previous_sock) {
            GameCam_Blend(camera, blend_duration, 0.0f, 1);
        }
    }
    if (camera->reset_blend != 0) {
        camera->reset_blend = 0;
        camera->blend_destination_position = position;
        camera->blend_destination_target = target;
    }
    f32 blend = 0.0f, angle_blend = 0.0f;
    if (camera->blend_duration > camera->blend_time) {
        if (camera->blend_curve > 0.0f)
            camera->blend_curve -= FRAMETIME;
        else {
            camera->blend_time += FRAMETIME;
            if (camera->blend_time > camera->blend_duration)
                camera->blend_time = camera->blend_duration;
        }
        if (camera->blend_duration > camera->blend_time) {
            blend = camera->blend_time / camera->blend_duration;
            angle_blend = MIN(2.0f * blend, 1.0f);
            camera->blend_start_position.x =
                camera->blend_end_position.x +
                (camera->blend_destination_position.x - camera->blend_end_position.x) * blend;
            camera->blend_start_position.y =
                camera->blend_end_position.y +
                (camera->blend_destination_position.y - camera->blend_end_position.y) * blend;
            camera->blend_start_position.z =
                camera->blend_end_position.z +
                (camera->blend_destination_position.z - camera->blend_end_position.z) * blend;
            camera->blend_start_target.x =
                camera->blend_end_target.x + (camera->blend_destination_target.x - camera->blend_end_target.x) * blend;
            camera->blend_start_target.y =
                camera->blend_end_target.y + (camera->blend_destination_target.y - camera->blend_end_target.y) * blend;
            camera->blend_start_target.z =
                camera->blend_end_target.z + (camera->blend_destination_target.z - camera->blend_end_target.z) * blend;
        }
    }
    camera->position_seek = position_seek;
    camera->angle_seek = angle_seek;
    if (camera->mode != previous_mode && camera->blend_time >= camera->blend_duration)
        camera->pos = position;
    else {
        if (camera->blend_duration > camera->blend_time) {
            position.x = camera->blend_start_position.x + (position.x - camera->blend_start_position.x) * blend;
            position.y = camera->blend_start_position.y + (position.y - camera->blend_start_position.y) * blend;
            position.z = camera->blend_start_position.z + (position.z - camera->blend_start_position.z) * blend;
        }
        f32 seek = MIN(position_seek * FRAMETIME, 1.0f) * CamStopBlend;
        camera->pos.x += (position.x - camera->pos.x) * seek;
        camera->pos.y += (position.y - camera->pos.y) * seek;
        camera->pos.z += (position.z - camera->pos.z) * seek;
    }
    camera->desired_position = position;
    camera->target = target;
    f32 dx = camera->target.x - camera->pos.x;
    f32 dz = camera->target.z - camera->pos.z;
    i32 pitch = pitch_override;
    if (pitch_override == -1)
        pitch = static_cast<u16>(-NuAtan2D(camera->target.y - camera->pos.y, NuFsqrt(dx * dx + dz * dz)));
    if (camera->blend_duration > camera->blend_time && camera->blend_mode == 2)
        pitch = camera->blend_start_pitch + static_cast<i32>(RotDiff(camera->blend_start_pitch, pitch) * angle_blend);
    camera->desired_pitch = pitch;
    i32 yaw = yaw_override;
    if (yaw_override == -1)
        yaw = NuAtan2D(dx, dz);
    if (camera->blend_duration > camera->blend_time && camera->blend_mode == 2)
        yaw = camera->blend_start_yaw + static_cast<i32>(RotDiff(camera->blend_start_yaw, yaw) * angle_blend);
    camera->desired_yaw = yaw;
    i32 roll = roll_override;
    if (roll_override == -1)
        roll = roll_hint_valid ? roll_hint : 0;
    if (camera->blend_duration > camera->blend_time && camera->blend_mode == 2)
        roll = camera->blend_start_roll + static_cast<i32>(RotDiff(camera->blend_start_roll, roll) * angle_blend);
    camera->desired_roll = roll;
    if (camera->mode != previous_mode && camera->blend_time >= camera->blend_duration) {
        camera->pitch = pitch;
        camera->yaw = yaw;
        camera->roll = roll;
    } else {
        if (pitch_override == -1) {
            if (((camera->pitch - pitch) & 0xffff) > 0x7fff) {
                CamStopBlend += FRAMETIME;
                if (CamStopBlend > 1.0f)
                    CamStopBlend = 1.0f;
            }
            camera->pitch = SeekRot(camera->pitch, pitch, NuFsqrt(CamStopBlend) * camera->angle_seek);
        } else
            camera->pitch = pitch_override;
        if (yaw_override == -1)
            camera->yaw = SeekRot(camera->yaw, yaw, camera->angle_seek * CamStopBlend);
        else
            camera->yaw = yaw_override;
        if (roll_override == -1)
            roll = SeekRot(camera->roll, roll,
                           (roll_seek_override > 0.0f ? roll_seek_override : camera->angle_seek) * CamStopBlend);
        else
            roll = roll_override;
        camera->roll = roll;
        pitch = camera->pitch;
        yaw = camera->yaw;
    }
    CamStopBlend += stop_blend_rate * FRAMETIME;
    if (CamStopBlend > 1.0f)
        CamStopBlend = 1.0f;
    GameCam_UpdateLookRot(camera);

    NUMTX orientation = {};
    shared_count = 0;
    for (i32 i = 0; i < 2; i++) {
        if (Player[i] != NULL && static_cast<i8>(Player[i]->apiobj.flags_low) < 0 && Player[i]->field_0x1086 == 4) {
            NuVecAdd(NUMTX_GET_ROW_VEC(&orientation, 0), NUMTX_GET_ROW_VEC(&orientation, 0),
                     NUMTX_GET_ROW_VEC(&Player[i]->vehicle_orientation, 0));
            NuVecAdd(NUMTX_GET_ROW_VEC(&orientation, 1), NUMTX_GET_ROW_VEC(&orientation, 1),
                     NUMTX_GET_ROW_VEC(&Player[i]->vehicle_orientation, 1));
            NuVecAdd(NUMTX_GET_ROW_VEC(&orientation, 2), NUMTX_GET_ROW_VEC(&orientation, 2),
                     NUMTX_GET_ROW_VEC(&Player[i]->vehicle_orientation, 2));
            shared_count++;
        }
    }
    if (shared_count > 0) {
        NuMtxSetIdentity(&camera->mtx);
        if (shared_count > 1) {
            f32 scale = 1.0f / shared_count;
            NuVecScale(NUMTX_GET_ROW_VEC(&orientation, 0), NUMTX_GET_ROW_VEC(&orientation, 0), scale);
            NuVecScale(NUMTX_GET_ROW_VEC(&orientation, 1), NUMTX_GET_ROW_VEC(&orientation, 1), scale);
            NuVecScale(NUMTX_GET_ROW_VEC(&orientation, 2), NUMTX_GET_ROW_VEC(&orientation, 2), scale);
            NuVecNorm(NUMTX_GET_ROW_VEC(&orientation, 0), NUMTX_GET_ROW_VEC(&orientation, 0));
            NuVecNorm(NUMTX_GET_ROW_VEC(&orientation, 1), NUMTX_GET_ROW_VEC(&orientation, 1));
            NuVecNorm(NUMTX_GET_ROW_VEC(&orientation, 2), NUMTX_GET_ROW_VEC(&orientation, 2));
        }
        *NUMTX_GET_ROW_VEC(&camera->mtx, 0) = *NUMTX_GET_ROW_VEC(&orientation, 0);
        *NUMTX_GET_ROW_VEC(&camera->mtx, 1) = *NUMTX_GET_ROW_VEC(&orientation, 1);
        *NUMTX_GET_ROW_VEC(&camera->mtx, 2) = *NUMTX_GET_ROW_VEC(&orientation, 2);
        NuMtxTranslate(&camera->mtx, &camera->pos);
        camera->render_mtx = camera->mtx;
        NuMtxPreRotateZ(&camera->render_mtx, roll);
        NuVecNorm(NUMTX_GET_ROW_VEC(&camera->render_mtx, 0), NUMTX_GET_ROW_VEC(&camera->render_mtx, 0));
        NuVecNorm(NUMTX_GET_ROW_VEC(&camera->render_mtx, 1), NUMTX_GET_ROW_VEC(&camera->render_mtx, 1));
        NuVecNorm(NUMTX_GET_ROW_VEC(&camera->render_mtx, 2), NUMTX_GET_ROW_VEC(&camera->render_mtx, 2));
        camera->target_mtx = camera->render_mtx;
    } else {
        NuMtxSetRotationZ(&camera->mtx, roll);
        NuMtxRotateX(&camera->mtx, pitch + static_cast<u16>(static_cast<i32>(camera->field_0x214)));
        NuMtxRotateY(&camera->mtx, yaw + static_cast<u16>(static_cast<i32>(camera->field_0x218)));
        NuMtxTranslate(&camera->mtx, &camera->pos);
        camera->render_mtx = camera->mtx;
        NuMtxSetRotationZ(&camera->target_mtx, roll);
        NuMtxRotateX(&camera->target_mtx, pitch);
        NuMtxRotateY(&camera->target_mtx, yaw);
        NuMtxTranslate(&camera->target_mtx, &camera->pos);
    }
    if (camera->judder_time > 0.0f) {
        camera->judder_time -= FRAMETIME;
        if (camera->judder_time > 0.0f) {
            i32 degrees = (static_cast<i32>(camera->judder_duration / 0.333f) + 1) * 360;
            i32 phase = static_cast<i32>(((degrees * 0x10000) / 360) *
                                         ((camera->judder_duration - camera->judder_time) / camera->judder_duration));
            f32 amount = (camera->judder_time / camera->judder_duration) *
                         ((546.0f * camera->judder_duration) * NU_SIN_LUT(phase));
            if (camera->judder_reverse)
                amount = -amount;
            if (camera->judder_axis == 0)
                NuMtxPreRotateX(&camera->render_mtx, static_cast<u16>(static_cast<i32>(amount)));
            else if (camera->judder_axis == 1)
                NuMtxPreRotateY(&camera->render_mtx, static_cast<u16>(static_cast<i32>(amount)));
            else
                NuMtxPreRotateZ(&camera->render_mtx, static_cast<u16>(static_cast<i32>(amount)));
        }
    }
    GIZMO *window = GizmoFindByName(WORLD->gizmo_sys, blowup_gizmotype_id, "window_frame1");
    if (Cheat_PowerUpActive(-1) ||
        (WORLD->current_level == SPEEDERCHASEA_LDATA && !disable_narrow_socks &&
         camera->sock_position.location.sock == 1) ||
        (WORLD->current_level == CLOUDCITYTRAPB_LDATA && window != NULL && window->object != NULL &&
         (static_cast<GIZMOBLOWUP_s *>(window->object)->output_flags & 1) != 0 &&
         static_cast<u8>(camera->sock_position.location.sock) <= 1))
        camera_shake = 0.5f;
    else if (WORLD->current_level == DEATHSTAR2BATTLED_LDATA && reinterpret_cast<u8 *>(LevFlag)[0] != 0)
        camera_shake = 1.5f;
    if (VehicleArea && player != NULL && (player->apiobj.character_data->model_flags & 0x2000) != 0 &&
        player->combo_input_timer > 0.0f && Cheat_IsOn(0x14))
        camera_shake = 0.6f;
    else {
        for (i32 i = 0; i < 2; i++) {
            GameObject_s *object = Player[i];
            if (object != NULL && static_cast<i8>(object->apiobj.flags_low) < 0) {
                if (WORLD->current_level == DEATHSTARBATTLED_LDATA &&
                    ObjInNarrowSock(object, WORLD->sock_sys, WORLD->level_idx)) {
                    camera_shake = 0.6f;
                    break;
                }
                if (object->camera_shake_strength > 0.0f && WORLD->area != NULL &&
                    (WORLD->area == PODRACE_ADATA || WORLD->area == PODSPRINT_ADATA)) {
                    camera_shake = object->camera_shake_strength * 2.0f;
                    break;
                }
            }
        }
    }
    GameCam_UpdateShake(camera, camera_shake);
    camera->shaken_right = *NUMTX_GET_ROW_VEC(&camera->render_mtx, 0);
    camera->shaken_up = *NUMTX_GET_ROW_VEC(&camera->render_mtx, 1);
    camera->dir = *NUMTX_GET_ROW_VEC(&camera->render_mtx, 2);
    pNuCam->mtx = camera->render_mtx;
    NuCameraSet(pNuCam);
    MakePlayPlanes(camera);
    if (!VehicleArea) {
        for (i32 i = 0; i < 2; i++) {
            GameObject_s *object = Player[i];
            if (object == NULL || static_cast<i8>(object->apiobj.flags_low) >= 0)
                continue;
            if (FadeSys.fade != 0.0f || object->apiobj.field_0x287 != 0 || object->character_context == 0x2b ||
                object->timer_d5c > 0.0f || GetMenuID() != -1 || object->character_context == 0x0f ||
                object->field_0xcc0 != NULL || (object->field_0xe24 & 8) == 0 ||
                ((object->apiobj.character_data->model_flags & 0x40) != 0 && object->communicate_blend > 0.0f)) {
                object->field_0xef0 = 0.0f;
                object->pause_context_state = 0.0f;
                object->field_0xefe &= ~0x18;
            } else {
                i32 was_blocked = (object->field_0xefe >> 3) & 1;
                bool changed = false;
                if ((GameTimer.update_count & 1) == i) {
                    NUVEC sample;
                    sample.x = 0.0f;
                    sample.y = 0.0f;
                    sample.z = (qrand() * (1.0f / 65535.0f)) * object->apiobj.field_0x1dc;
                    NuVecRotateX(&sample, &sample, static_cast<i32>((qrand() * (1.0f / 65535.0f)) * -16384.0f));
                    sample.y *= object->apiobj.field_0x1e0 / object->apiobj.field_0x1dc;
                    NuVecRotateY(&sample, &sample, qrand());
                    NuVecAdd(&sample, &sample, &object->apiobj.collision_position);
                    NUVEC ray;
                    NuVecSub(&ray, &sample, &GameCam->pos);
                    i32 blocked = GameRayCast(&GameCam->pos, &ray, 0.0f, 0x1f) & 1;
                    object->field_0xefe = (object->field_0xefe & ~8) | (blocked << 3);
                    if (was_blocked != blocked) {
                        object->field_0xef0 = 0.0f;
                        changed = true;
                    }
                }
                if (!changed && object->field_0xef0 < 0.3f) {
                    object->field_0xef0 += FRAMETIME;
                    if (object->field_0xef0 >= 0.3f) {
                        object->field_0xef0 = 0.3f;
                        object->field_0xefe = (object->field_0xefe & ~0x10) | (was_blocked << 4);
                    }
                }
            }
            object->pause_context_state =
                SeekLinearF(object->pause_context_state, (object->field_0xefe & 0x10) ? 1.0f : 0.0f, 3.0f * FRAMETIME);
        }
    }
}

void ViewCamDraw() {
}

void KeepOnScreen(GameObject_s *object) {
    const f32 previous_keep_time = object->field_0xda8;
    object->field_0xda8 = 0.0f;

    if (newgamecam != 0 || (object->field_0xf03 & 0x10) != 0)
        return;
    bool controlled = false;
    for (i32 index = 0; index < 8; ++index) {
        GameObject_s *player = Player[index];
        if (player != NULL && player->character_context == 0x51 && player->field_0x788 != NULL &&
            static_cast<TECHNO *>(player->field_0x788)->controlled_object == object)
            controlled = true;
    }
    if ((static_cast<i8>(object->apiobj.flags_low) >= 0 && !controlled) || object->apiobj.field_0x287 != 0 ||
        (object->field_0xefe & 4) == 0 || MiniCutCam != 0 || GetMenuID() != -1 || object->character_context == 0x2b) {
        return;
    }

    switch (object->character_id_0x7a5) {
        case 0x0f:
        case 0x1f:
        case 0x2b:
        case 0x46:
        case 0x47:
        case 0x51:
            return;
        default:
            break;
    }

    if (WORLD->current_level == BONUS_GUNSHIPA_LDATA && GameCam->mode == 0x0b) {
        const f32 push_speed = object->apiobj.character_data->game_character->run_speed * 5.0f;
        f32 count = 0.0f;
        NUVEC response;
        const i32 planes[4] = {3, 2, 1, 4};
        const i32 rotations[4] = {0x6000, 0xa000, 0x2000, 0xe000};
        for (i32 index = 0; index < 4; ++index) {
            PLAYPLANE_s *plane = &PlayPlane[planes[index]];
            if (OnOrInsidePlane(&object->apiobj.collision_position, &plane->point, &plane->normal, NULL, 0.0f, NULL) !=
                0) {
                NUVEC normal;
                NuVecRotateY(&normal, &GunshipANorm, rotations[index]);
                Surface_Deflect(&normal, &object->apiobj.velocity, &response, 0);
                response.x += normal.x * push_speed;
                response.z += normal.z * push_speed;
                count += 1.0f;
            }
        }
        if (count > 0.0f) {
            const f32 inverse_count = 1.0f / count;
            // The original retains the last response, then divides by the
            // number of intersected planes; it does not accumulate responses.
            object->apiobj.movement_direction.x = response.x * inverse_count;
            object->apiobj.movement_direction.z = response.z * inverse_count;
        }
        return;
    }

    if (VehicleArea == 0) {
        GameObject_s *other_player = object == Player[0] ? Player[1] : object == Player[1] ? Player[0] : NULL;
        if (other_player != NULL &&
            (static_cast<i8>(other_player->apiobj.flags_low) >= 0 || other_player->character_context == 0x2b))
            return;
    }
    const bool two_players = Player[0] != NULL && static_cast<i8>(Player[0]->apiobj.flags_low) < 0 &&
                             Player[1] != NULL && static_cast<i8>(Player[1]->apiobj.flags_low) < 0;
    GAMECHARACTERDATA *character = object->apiobj.character_data->game_character;
    const f32 push_distance = VehicleArea != 0 ? character->walk_speed : character->run_speed;
    NUVEC constrained_movement = object->apiobj.velocity;
    bool constrained = false;

    // Near plane has no object-size inset in the original.
    if (OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[0].point, &PlayPlane[0].normal, NULL, 0.0f,
                        NULL) != 0) {
        Surface_Deflect(&PlayPlane[0].normal, &constrained_movement, &constrained_movement, 0);
        constrained_movement.x += PlayPlane[0].normal.x * push_distance;
        constrained_movement.z += PlayPlane[0].normal.z * push_distance;
        constrained = true;
    }

    // Test the right plane first, then the left plane.  Each response is
    // calculated from the unmodified velocity, exactly as in the target.
    PLAYPLANE_s *side_plane = NULL;
    if (OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[2].point, &PlayPlane[2].normal, NULL,
                        -object->apiobj.field_0x1dc, NULL) != 0) {
        side_plane = &PlayPlane[2];
    } else if (OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[1].point, &PlayPlane[1].normal, NULL,
                               -object->apiobj.field_0x1dc, NULL) != 0) {
        side_plane = &PlayPlane[1];
    }
    if (side_plane != NULL) {
        Surface_Deflect(&side_plane->normal, &object->apiobj.velocity, &constrained_movement, 0);
        constrained_movement.x += side_plane->normal.x * push_distance;
        constrained_movement.z += side_plane->normal.z * push_distance;
        constrained = true;
    }

    if ((VehicleArea != 0 && (WORLD->area == NULL || WORLD->area != PODSPRINT_ADATA)) || KEEPONSCREEN_SIDESONLY == 0 ||
        (two_players && abs(RotDiff(0, GameCam->pitch)) > 0x2000)) {
        PLAYPLANE_s *depth_plane = NULL;
        if (OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[4].point, &PlayPlane[4].normal, NULL,
                            -object->apiobj.field_0x1e0, NULL) != 0) {
            // Crossing the lower screen plane pushes toward the depth plane
            // selected by the camera pitch.
            depth_plane = GameCam->dir.y > 0.0f ? &PlayPlane[5] : &PlayPlane[0];
        } else if (OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[3].point, &PlayPlane[3].normal, NULL,
                                   -object->apiobj.field_0x1e0, NULL) != 0) {
            // The upper screen plane uses the opposite depth response.  This
            // distinction is important: using the same plane for both sides
            // can cancel movement in the wrong world-space direction.
            depth_plane = GameCam->dir.y > 0.0f ? &PlayPlane[0] : &PlayPlane[5];
        }

        if (depth_plane != NULL) {
            Surface_Deflect(&depth_plane->normal, &object->apiobj.velocity, &constrained_movement, 0);
            constrained_movement.x += depth_plane->normal.x * push_distance;
            constrained_movement.z += depth_plane->normal.z * push_distance;
            constrained = true;
        }
    }

    if (constrained) {
        object->apiobj.movement_direction.x = constrained_movement.x;
        object->apiobj.movement_direction.z = constrained_movement.z;
        object->field_0xda8 = previous_keep_time + FRAMETIME;
        SpecialMove_Cancel(object);
    }
}

NUVEC *ViewCamGetTgt() {
    return &ViewCam.target;
}

i32 ViewCamGetMode() {
    return ViewCam.mode;
}

void SetDepthOfField() {
}

void SpeedBlur_Apply(WORLDINFO_s *) {
}

void SpeedBlur_Update() {
}

void ViewCamSetActive(i32 mode, GAMEPAD_s *gamepad) {
    if (player != NULL) {
        ViewCam.mode = mode;
        if (mode != 0) {
            ViewCam.gamepad = gamepad;
            ViewCam.target = player->apiobj.collision_position;
        } else {
            ViewCam.gamepad = NULL;
        }
    }
}

void KeepPointOnScreen(NUVEC *position, NUVEC *velocity) {
    if (position->x < -0.85f) {
        position->x = -0.85f;
        if (velocity != NULL)
            velocity->x = 0.0f;
    } else if (position->x > 0.85f) {
        position->x = 0.85f;
        if (velocity != NULL)
            velocity->x = 0.0f;
    }
    if (position->y < -0.85f) {
        position->y = -0.85f;
        if (velocity != NULL)
            velocity->y = 0.0f;
    } else if (position->y > 0.85f) {
        position->y = 0.85f;
        if (velocity != NULL)
            velocity->y = 0.0f;
    }
}

void SetCameraMatrices() {
    NuRndrLightingStateCurrent.field_0x60 = 1;
    NuRndrLightingStateCurrent.field_0x74 = 0;
    NuRndrSetSpecularLightPS(NULL, NULL);

    NUMTX effect_matrix;
    NUVEC scale = {1.0f, 1.0f, 1.0f};
    NuMtxInvR(&effect_matrix, &global_camera.mtx);
    NuMtxScale(&effect_matrix, &scale);
    effect_matrix.m30 = 1.0f;
    effect_matrix.m31 = 1.0f;
    effect_matrix.m32 = 0.0f;
    effect_matrix.m33 = 1.0f;
    effect_matrix.m23 = 0.0f;
    effect_matrix.m13 = 0.0f;
    effect_matrix.m03 = 0.0f;
    NuRndrSetFxMtx(&effect_matrix);
}

GAMEPAD_s *ViewCamGetGamePad() {
    return ViewCam.gamepad;
}

// Original: 784 bytes.
void KeepVehicleOnScreen(GameObject_s *object, i32 sides, i32 top, i32 bottom) {
    object->field_0xf03 |= 0x10;
    if (GamePlayTimer.time_elapsed < 1.0f)
        return;
    f32 margin = WORLD->area == BATTLEOVERCORUSCANT_ADATA ? 0.5f : 1.0f;
    f32 distance;
    if (sides != 0) {
        OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[1].point, &PlayPlane[1].normal, NULL, 0.0f,
                        &distance);
        if (distance < margin) {
            f32 correction = -((distance - margin) / margin);
            f32 speed = object->apiobj.character_data->game_character->run_speed;
            object->target_velocity.x += (speed + speed) * (correction + correction);
        } else {
            OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[2].point, &PlayPlane[2].normal, NULL, 0.0f,
                            &distance);
            if (distance < margin) {
                f32 correction = -((distance - margin) / margin);
                f32 speed = -object->apiobj.character_data->game_character->run_speed;
                object->target_velocity.x += (speed + speed) * (correction + correction);
            }
        }
    }
    if (top != 0) {
        OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[3].point, &PlayPlane[3].normal, NULL, 0.0f,
                        &distance);
        if (distance != 1000000000.0f && distance < margin) {
            f32 correction = -((distance - margin) / margin);
            f32 speed = -object->apiobj.character_data->game_character->run_speed;
            object->target_velocity.y += (speed + speed) * (correction + correction);
            return;
        }
    } else {
        distance = 1000000000.0f;
    }
    if (bottom != 0) {
        OnOrInsidePlane(&object->apiobj.collision_position, &PlayPlane[4].point, &PlayPlane[4].normal, NULL, 0.0f,
                        &distance);
        if (distance < margin) {
            f32 correction = -((distance - margin) / margin);
            f32 speed = object->apiobj.character_data->game_character->run_speed;
            object->target_velocity.y += (speed + speed) * (correction + correction);
        }
    }
}

void CentreTwoPlayerCamera(nuvec_s *center, nuvec_s *player_a, nuvec_s *player_b, nuvec_s *reference) {
    const f32 distance_a = NuVecDist(reference, player_a, NULL);
    const f32 distance_b = NuVecDist(reference, player_b, NULL);
    const f32 blend = distance_a / (distance_a + distance_b);
    center->x = player_a->x + (player_b->x - player_a->x) * blend;
    center->y = player_a->y + (player_b->y - player_a->y) * blend;
    center->z = player_a->z + (player_b->z - player_a->z) * blend;
}

void do_Pad_flymode_camera(edcam_s *camera, float delta_time, nupad_s *pad) {
    constexpr f32 kNominalFrameTime = 1.0f / 60.0f;
    constexpr f32 kPitchSpeedScale = 32.0f;
    constexpr f32 kYawSpeedScale = 64.0f;
    constexpr f32 kPositionSpeedScale = 0.01f;
    constexpr i32 kPadDeadZone = 32;
    constexpr i32 kPitchLimit = 0x4000;

    const f32 frame_scale = delta_time / kNominalFrameTime;
    const f32 move_speed = camera->auto_move_dist_scale == 0.0f
                               ? 1.0f
                               : camera->auto_move_base + NuFabs(camera->distance) * camera->auto_move_dist_scale;
    const f32 zoom_speed = camera->auto_zoom_dist_scale == 0.0f
                               ? 1.0f
                               : camera->auto_zoom_base + NuFabs(camera->distance) * camera->auto_zoom_dist_scale;

    NUMTX rotation = numtx_identity;
    NuMtxRotateX(&rotation, camera->pitch);
    NuMtxRotateY(&rotation, camera->yaw);

    NUVEC opposite_offset = {0.0f, 0.0f, -camera->distance};
    NuVecMtxRotate(&opposite_offset, &opposite_offset, &rotation);
    NUVEC old_opposite;
    NuVecAdd(&old_opposite, &camera->position, &opposite_offset);

    const i32 pad_yaw = NuPs2ApplyDeadZone(pad->analog_right_x, kPadDeadZone);
    const i32 pad_pitch = NuPs2ApplyDeadZone(pad->analog_right_y, kPadDeadZone);
    i32 pitch_delta =
        static_cast<i32>(static_cast<f32>(pad_pitch * camera->pad_pitch_speed) * delta_time * kPitchSpeedScale);
    if ((camera->freedoms & EDCAM_FREEDOM_INVERT_PAD_PITCH) != 0) {
        pitch_delta = -pitch_delta;
    }
    if ((camera->freedoms & EDCAM_FREEDOM_PITCH) != 0) {
        camera->pitch += pitch_delta;
        if (camera->pitch > kPitchLimit) {
            camera->pitch = kPitchLimit;
        }
        if (camera->pitch < -kPitchLimit) {
            camera->pitch = -kPitchLimit;
        }
    }
    if ((camera->freedoms & EDCAM_FREEDOM_YAW) != 0) {
        camera->yaw +=
            static_cast<i32>(static_cast<f32>(pad_yaw * camera->pad_yaw_speed) * delta_time * kYawSpeedScale);
    }

    rotation = numtx_identity;
    NuMtxRotateX(&rotation, camera->pitch);
    NuMtxRotateY(&rotation, camera->yaw);
    opposite_offset = {0.0f, 0.0f, -camera->distance};
    NuVecMtxRotate(&opposite_offset, &opposite_offset, &rotation);
    NUVEC new_opposite;
    NuVecAdd(&new_opposite, &camera->position, &opposite_offset);
    NuVecAdd(&camera->position, &camera->position, &new_opposite);
    NuVecSub(&camera->position, &camera->position, &old_opposite);

    NUVEC movement = {0.0f, 0.0f, 0.0f};
    const i32 pad_move_x = NuPs2ApplyDeadZone(pad->analog_left_x, kPadDeadZone);
    movement.x -= static_cast<f32>(pad_move_x) * delta_time * camera->distance * kPositionSpeedScale;
    const i32 pad_move_z = NuPs2ApplyDeadZone(pad->analog_left_y, kPadDeadZone);
    movement.z += static_cast<f32>(pad_move_z) * delta_time * camera->distance * kPositionSpeedScale;

    movement.y += static_cast<f32>(pad->analog_l1) * camera->position_speed.y * move_speed * frame_scale * 0.5f;
    movement.y -= static_cast<f32>(pad->analog_l2) * camera->position_speed.y * move_speed * frame_scale * 0.5f;

    if ((camera->freedoms & EDCAM_FREEDOM_DISTANCE) != 0) {
        camera->distance += static_cast<f32>(pad->analog_r1) * camera->distance_speed * zoom_speed * frame_scale;
        camera->distance -= static_cast<f32>(pad->analog_r2) * camera->distance_speed * zoom_speed * frame_scale;
        if (camera->distance > -camera->minimum_distance) {
            camera->distance = -camera->minimum_distance;
        }
    }

    NuVecMtxRotate(&movement, &movement, &rotation);
    if ((camera->freedoms & EDCAM_FREEDOM_POSITION_X) != 0) {
        camera->position.x += movement.x;
    }
    if ((camera->freedoms & EDCAM_FREEDOM_POSITION_Y) != 0) {
        camera->position.y += movement.y;
    }
    if ((camera->freedoms & EDCAM_FREEDOM_POSITION_Z) != 0) {
        camera->position.z += movement.z;
    }
}

void InitCameraTargetMaterial() {
}

i32 GoingForwardsAlongNarrowSock(GameObject_s *object) {
    if (!object->in_narrow_socket)
        return 0;
    i32 delta = RotDiff(object->yrot, object->apiobj.field_0x276);
    if (delta < 0)
        delta = -delta;
    i32 forwards = delta < 0x4000;
    if (object->character_context == 0x2a && object->airborne_action_duration * 0.8f > object->context_animation_timer)
        forwards = 1 - forwards;
    return forwards;
}

void GetTopBot(GameObject_s *object) {
    CHARACTERDATA *character = object->apiobj.character_data;
    const f32 bottom = character->field15_0x34;
    const f32 top = character->field16_0x38;
    object->field_0xffc = bottom;
    object->field_0x1000 = top;

    if (object->field_0x1008 == 0.0f) {
        object->collision_y_scale = 0.0f;
    } else {
        object->collision_y_scale = (top - bottom) / (object->field_0x1008 * 2.0f);
    }
}

extern "C" {

    i32 near_clip_at_cursor;

    void cbNearClipAtCursor(void) {
    }

    void do_Pad_Standard_camera(edcam_s *, f32, nupad_s *) {
    }

    void do_maya_mouse_camera(edcam_s *) {
    }

    void do_mouse_flymode_camera(edcam_s *, f32) {
    }

} // extern "C"
