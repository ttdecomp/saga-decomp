#include "decomp.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "legoapi/actions/character/snake.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "globals.h"
#include "nu2api/nu3d/nuspecial.h"

static GameObject_s *ForceBackObj = NULL;
static NUVEC *ForceBackPos = NULL;
static i32 ForceBackType = 0;
static f32 ForceBackRadius = 0.0f;
static f32 ForceBackRadius2 = 0.0f;

#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmos/traps/attractos.h"
#include "legoapi/gizmos/trigger/signals.h"
#include "legoapi/gizmos/door/securitydoors.h"
#include "legoapi/gizmos/object/technos.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "legoapi/gizmos/object/gizpanel.h"
#include "legoapi/gizmos/transport/tubes.h"
#include "legoapi/gizmos/door/zipups.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "legoapi/gizmos/transport/tightropes.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/props/system/socksys.h"
#include "legoapi/render/core/rtl.h"
#include "legoapi/render/fx.h"
#include "legoapi/render/fx/spline_position.h"
#include "legoapi/world/level.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "legoapi/world/mission.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"

extern AREADATA_s *PODSPRINT_ADATA;
extern AREADATA_s *PODRACE_ADATA;
extern AREADATA_s *GUNSHIP_ADATA;
extern AREADATA_s *BONUS_GUNSHIP_ADATA;
extern "C" i16 id_GRABCONTROL, id_GRABR2CONTROL;

float SLAMGRAVITY = -15.0f;
static float applygravity_extrahoveroffset;


void MovePlayer_DIRECTIONAL(GameObject_s *object);
i32 CanStepBack(GameObject_s *object);
i32 StepBackFromTarget(GameObject_s *object);
i32 ForcePushed_SetTargetMom(GameObject_s *object, f32 *seek_rate);
i32 ForcePushed_YRotation(GameObject_s *object);
static i32 IsAFallAnim(i32 animation);
void MovePlayer_VEHICLEDIRECTIONAL(GameObject_s *object);
i32 MovePlayer_TWIST(GameObject_s *object);
i32 MovePlayer_CIRCLE(GameObject_s *object);
i32 CircleLevel(LEVELDATA_s *level);
f32 ForceTowardsMid(GameObject_s *object);
void MoveInactiveVehicle(GameObject_s *object, i32 mode, GameObject_s **vehicle);
void KeepVehicleOnScreen(GameObject_s *object, i32 sides, i32 top, i32 bottom);
i32 MovePlayer_GUNSHIPIN(GameObject_s *object);
i32 MovePlayer_POD(GameObject_s *object);
void ApplyGravity(GameObject_s *object, float *gravity, float hover_height, float seek_rate, float *ground_height);
float VehicleTurnOrLoopOffset(GameObject_s *object);
void GameObjectOrigin(GameObject_s *object);
void ComboHitFrame(GameObject_s *object, i32 damage);
i32 Grapple_LookAtPos(GameObject_s *object, NUVEC *position);
NUVEC *Technos_TgtPos(TECHNO_s *techno);
void GameCam_UpdateLookRot(GAMECAMERA_s *camera);
void GameCam_ResetLookRot(GAMECAMERA_s *camera);
void GameCam_UpdateShake(GAMECAMERA_s *camera, f32 ambient_amount);
void MakePlayPlanes(GAMECAMERA_s *camera);
u16 SeekRot(u16 current, u16 target, f32 rate);
void SeekVec(NUVEC *result, NUVEC *current, NUVEC *target, f32 rate);
void ShoveSystemCheckGameObject(GameObject_s *object);
i32 GizmoBlowupCheckProximity(WORLDINFO_s *world, GameObject_s *object);
void KeepWeaponOut(GameObject_s *object);
void DropInOutCode(GameObject_s *object);
void Signal_MoveCode(WORLDINFO_s *world, GameObject_s *object);
void TakeHitCode(GameObject_s *object);
void FloatCode(GameObject_s *object);
void SlideCode(GameObject_s *object);
void FlattenCode(GameObject_s *object);
void Hang_MoveCode(GameObject_s *object);
void Ledge_MoveCode(WORLDINFO_s *world, GameObject_s *object);
void LedgeTerrain_MoveCode(GameObject_s *object);
void Climb_MoveCode(GameObject_s *object);
void ForcedBackCode(GameObject_s *object);
void Tube_MoveCode(GameObject_s *object, WORLDINFO_s *world);
void PushCode(GameObject_s *object, i32 allow_grab);
void BackFlipCode(GameObject_s *object);
void TakeOverCode(GameObject_s *object, i32 tag_pressed);
void Glide_MoveCode(GameObject_s *object);
void JumpCode(GameObject_s *object, i32 jump_pressed, i32 jump_held, u32 animation_set, i32 action_pressed,
              i32 action_held, i32 special_animation);
void GizPanel_MoveCode(WORLDINFO_s *world, GameObject_s *object, i32 special_pressed);
void HatMachine_MoveCode(WORLDINFO_s *world, GameObject_s *object, i32 special_pressed);
void ZipUp_MoveCode(GameObject_s *object, i32 special_pressed);
void Lever_MoveCode(WORLDINFO_s *world, GameObject_s *object);
i32 ThermalDetonator_MoveCode(GameObject_s *object);
void Detonator_MoveCode(GameObject_s *object);
extern "C" void AddVariableShotDebrisEffectTimed1(i32, NUVEC *, i32, f32, i16, i16, NUMTX *);
extern "C" void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);
void Teleport_MoveCode(GameObject_s *object, i32 special_pressed);
void ComboRotateCode(GameObject_s *object, i32 action_held);
void WeaponOutCode(GameObject_s *object);
void WeaponInCode(GameObject_s *object);
void WeaponScalingCode(GameObject_s *object);
void HoldCode(GameObject_s *object);
void SetWeaponOut(GameObject_s *object);
void SlowWeaponIn(GameObject_s *object);
void FastWeaponIn(GameObject_s *object, i32 sound);
void SlowWeaponOut(GameObject_s *object);
void FastWeaponOut(GameObject_s *object, i32 sound);
void StartHold(GameObject_s *object);
void BlockSfx(GameObject_s *object);
i32 NewBlockAction(GameObject_s *object);
void MakeJumpReachHeight(GameObject_s *, f32, i32);
void PlayJumpSfx(GameObject_s *, i32);
void NewRumble(nupad_s *, f32, i32);
void FindAnglesZX(NUVEC *, u16 *, u16 *);
i32 GrappleSwingMode = 1;
void Hint_SetComplete(i32);
i32 (*CanStartHoldFn)(GameObject_s *) = NULL;
void PlaySabreSfx(char *, GameObject_s *, NUVEC *, i32);
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);
extern "C" f32 *AnimListFrameArray(CHARACTERMODEL_s *, i32);
void HeadMovement(GameObject_s *object);
void CloakMovement(GameObject_s *object);
void HairMovement(GameObject_s *object);
i32 AnakinGreenSabre(GameObject_s *object);
i32 SuperWeirdo(GameObject_s *object);
extern i16 id_IMPERIALGUARD;
extern i16 id_GAMORREANGUARD;
BOLT_s *FindIncomingBolt(GameObject_s *, i32, i32);
PART_s *FindIncomingPart(void *, NUVEC *, f32, u32, f32);
i32 StartFallLand(GameObject_s *object, i32 action);
void UpdateLastSafePosition(GameObject_s *object);
extern "C" TERRAIN_SURFACE_s TerSurface[32];
i32 NoLayerKill(GameObject_s *object);
void Punch_Hit(GameObject_s *, GameObject_s *, f32, f32);
void StartQuickShoot(GameObject_s *, i32);
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
extern "C" i32 AddGameDebrisRot(APIDEBRISSYS_s *, i32, NUVEC *, i32, i16, i16);
i32 (*FindSlamOrigin_UseCPosFn)(GameObject_s *) = NULL;
void (*Jump_EndOfLandContextFn)(GameObject_s *) = NULL;
void ConstantRumble(GameObject_s *object, f32 strength, f32 duration);
extern "C" void PlaySfxAndSetVolume(char *name, NUVEC *position, f32 volume);
i32 GizForce_FindBestForceTarget(GIZFORCESYS_s *, GameObject_s *);
i32 GizForce_StoodOnForce(GIZFORCE_s *, GameObject_s *);
u16 GizForces_AngleToForce(NUVEC *, GIZFORCE_s *);
void SetHeadTarget(GameObject_s *, NUVEC *, i8, f32, f32, f32);
void AlertSurroundingCreatures(GameObject_s *, NUVEC *);
void GameCam_Blend(GAMECAMERA_s *, f32, f32, i32);
void GameCam_HitRoll(void);
void EndForce(GameObject_s *, i32);
void ResetForceGlow(PLAYERPACKET_s *);
f32 FORCEGLOWTIME = 0.1f;
f32 ForceThrowGravity = -2.5f;
f32 ForceThrowSpeed = 5.0f;
f32 MaulF_ForceThrowSpeed = 1.0f;
f32 MaulF_ForceThrowGravity = -0.5f;
f32 DookuC_ForceThrowSpeed = 2.5f;
f32 DookuC_ForceThrowGravity = -2.5f;
PART_s *Part_FindFromHSpecial(nuhspecial_s *);
PART_s *GizForce_Throw(GameObject_s *, GIZFORCE_s *, f32, f32, i32);
f32 DEACTIVATEDTIME = 8.0f;
i32 ForcePush_SuperMindTrick;
i32 ForcePush_SuperPush;
i32 ForcePush_Waft;
i32 ZapTarget(GameObject_s *);
i32 CannotKill(GameObject_s *);
i32 FaceOpponent(GameObject_s *object, NUVEC *position);
void SetProtocolDroidDeactivatedAction(GameObject_s *);
extern i16 id_JAWA;
extern i16 id_GONKDROID;
void NewBuzz(nupad_s *, f32, i32);
void Arcade_AIKilled(i32);
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
i16 *objhitobj_killparts_yrot;
i32 objhitobj_noimpactsfx;
void Player_ClearContext(GameObject_s *, i32);
void Player_ResetContexts(PLAYERPACKET_s *);
void PlayDieSfx(GameObject_s *);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
void GameAudio_PlaySfxById(i32, NUVEC *, i32, i32);
static void CommunicateCode(GameObject_s *, i32, i32);
static void PunchCode(GameObject_s *, i32, i32, i32, i32, f32);
static i32 ShootCode(GameObject_s *, i32, i32, i32, i32, i32);
static void ForcePushed_MoveCode(GameObject_s *);
static void DeactivatedCode(GameObject_s *);
static void ZapCode(GameObject_s *, i32, i32);
static void FireCode(GameObject_s *, i32, i32, f32, i32);
void KeepWeaponIn(GameObject_s *);
void BigJumpCode(GameObject_s *);
void InstantKillParts(GameObject_s *, i32, f32);
EXPLOSION *Detonate(NUVEC *, u16);
void AddFancyMessage(char *, f32, f32, f32, f32, i32, i32);
extern i16 tCHEAT_SELFDESTRUCT;
extern i16 id_BATTLEDROID, id_BUZZDROID, id_SUPERBATTLEDROID, id_PROBEDROID;
extern i16 id_NAFFDROID1, id_NAFFDROID2, id_NAFFDROID4, id_MOUSEDROID;
static void DodgeCode(GameObject_s *, i32, i32);
void Grapple_MoveCode(GameObject_s *);
void SuperCarry_MoveCode(WORLDINFO_s *, GameObject_s *);
void SpecialMove_VictimCode(GameObject_s *);
i32 ObjInNarrowSock(GameObject_s *, SOCKSYS *, i32);
i32 SuperCarry_Carrying(GameObject_s *);
void Torpedo_UpdateJobbies(GameObject_s *);
void TorpedoCode(GameObject_s *, i32, f32);
void PeriscodeCode(GameObject_s *);
i32 PodLevel(AREADATA_s *);
void KeepOnScreen(GameObject_s *);
void UpdateSnakeBody(GameObject_s *);
void Teleport_NetMoveCode(GameObject_s *);
void TractorBeamCode(GameObject_s *);
void AddSurfaceRipples(GameObject_s *);
extern i16 id_SNAKE;
extern "C" i16 id_ATAT;
extern i16 id_YODA, id_YODAGHOST;
extern i16 id_C3PO, id_TC14;
extern i16 id_DROIDEKA, id_RANCOR;
extern "C" i16 id_ATST, id_MINIATST, id_ATST_LOWRES, id_MINIATAT, id_MINIATTE;
void Attracto_MoveCode(WORLDINFO_s *, GameObject_s *);
void SecurityDoor_MoveCode(WORLDINFO_s *, GameObject_s *);
void Batarang_MoveCode(GameObject_s *);
void FireBountyHunterRocket(GameObject_s *);
f32 DIEAIRSPEED = 1.5f;
f32 DIEAIRJUMPSPEED = 2.0f;
void SetObjAsHeadTarget(GameObject_s *, GameObject_s *, i8, f32, f32, f32);
void KillRumble(GameObject_s *);
void PlayHurtSfx(GameObject_s *);
void SetFlicker(GameObject_s *, f32);
void GameCam_NewShake(GAMECAMERA_s *, f32, f32, f32);
extern i16 id_BATTLEDROIDSECURITY, id_PKDROID, id_PITDROID;
void FindForcePushTarget(GameObject_s *, i32, i32);
void ReleaseForce(GameObject_s *, i32);
void BobaRocket_Move(PART_s *, f32);
void Boulder_Move(PART_s *, f32);
void Boulder_Kill(PART_s *, i32);
extern "C" void NewPartRotation(PART_s *);
void NewBuzzFrames(nupad_s *, i32, i32);
void PlayGruntSfx(GameObject_s *);
extern i32 dagobah_training;
extern i16 id_LUKESKYWALKERDAGOBAH;
extern i32 newgamecam;
extern f32 newgamecamtime;
extern FadeSystem FadeSys;
i32 GetMenuID(void);

// Multiplies the ordinary camera's per-frame position blend.  The original
// keeps this as writable camera state (default 1.0), rather than folding it
// into the socket seek rate.
f32 CamStopBlend = 1.0f;
i32 netcamera;

extern "C" {
    extern i16 id_BODYGUARD;
    extern i16 id_GRIEVOUS;
}

enum JEDI_ACTION : i16 {
    JEDI_ACTION_COMBO_1_1 = 46,
    JEDI_ACTION_COMBO_2_1 = 53,
};

static GAMECHARACTERDATA *GetGameCharacterData(GameObject_s *object) {
    if (object == NULL || object->apiobj.character_data == NULL) {
        return NULL;
    }
    return static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
}

static i32 RotationDistance(u16 current, u16 target) {
    const i32 difference = RotDiff(current, target);
    return difference < 0 ? -difference : difference;
}

static void ShiftPadDirectionHistory(GameObject_s *object, GAMEPAD_s *pad) {
    i32 player_index = -1;
    if (Player[0] == object) {
        player_index = 0;
    } else if (Player[1] == object) {
        player_index = 1;
    }
    if (player_index == -1) {
        return;
    }

    PadOldSpeed2[player_index] = PadOldSpeed[player_index];
    PadOldAngle2[player_index] = PadOldAngle[player_index];
    PadOldSpeed[player_index] = pad->input_magnitude;
    PadOldAngle[player_index] = pad->input_angle;
}

GAMEPAD_s *ViewCamGetGamePad();
void MovePlayer_ROLLING(GameObject_s *object);
extern f32 Hub_PadSpeed[2];
extern u16 Hub_PadAngle[2];
extern f32 drop_back_in_timer;
extern i32 LIFTPLAYER;
extern f32 OFFSCREEN_CATCHUP_TIME;

pushblock_s *BlockInBlock(WORLDINFO_s *, pushblock_s *, i32, pushblock_s **);
void MoveBlocksOverBlock(WORLDINFO_s *, pushblock_s *, i32, nuvec_s *);
void MoveBlocks(WORLDINFO_s *world, pushblock_s *block, i32 index, nuvec_s *velocity) {
    NUVEC delta = {velocity->x, 0.0f, velocity->z};
    block->position->x += delta.x;
    block->position->z += delta.z;
    NuSpecialUpdate(&block->special);
    for (i32 i = 0; i < block->end_position_count; ++i) {
        NUMTX *matrix = NuSpecialGetInstanceMtx(&block->end_position_specials[i]);
        matrix->m30 = block->position->x;
        matrix->m32 = block->position->z;
        NuSpecialUpdate(&block->end_position_specials[i]);
    }
    if (BlockInBlock(world, block, index, &block->supporting_block))
        block->runtime_flags_0c9 |= 4;
    else {
        MoveBlocksOverBlock(world, block, index, &delta);
        if (block->pushing_object && (fabsf(delta.x) > 0.001f || fabsf(delta.z) > 0.001f) &&
            NuFmod(GameTimer.time_elapsed, 0.25f) < 0.1f)
            NewRumble(block->pushing_object->pad_gamepad->pad, ((f32)qrand() * 1.5259022e-5f) * 0.05f, 0);
    }
    if (block->runtime_flags_0c8 & 8) {
        delta.y = velocity->y;
        delta.x = delta.z = 0;
        block->position->y -= delta.y;
        block->ground_offset -= delta.y;
        pushblock_s *hit = BlockInBlock(world, block, index, &block->supporting_block);
        if (hit) {
            block->settled_height = hit->position->y + hit->bounds_max.y;
            block->runtime_flags_0c9 |= 8;
        } else
            MoveBlocksOverBlock(world, block, index, &delta);
        NuSpecialUpdate(&block->special);
    }
}

// Original: 3,219 bytes.
void MovePlayer(GameObject_s *object) {
    GAMEPAD_s *pad = object->pad_gamepad;
    APIOBJECT &api = object->apiobj;
    const i32 analog_x = pad->pad != NULL ? pad->pad->analog_left_x : 0;
    const i32 analog_y = pad->pad != NULL ? pad->pad->analog_left_y : 0;
    object->field_0xe22 &= ~GAMEOBJECT_E22_FLAG_INPUT_ANGLE_VALID;
    if (pad->input_magnitude != pad->previous_input_magnitude)
        pad->allocated_5a |= GAMEPAD_RUNTIME_MAGNITUDE_CHANGED;
    api.field_0x214 = api.field_0x218;
    api.start_position = api.position;
    api.initial_position = api.collision_position;
    api.field_0x27e = api.field_0x27d;
    pad->previous_input_angle = pad->input_angle;
    pad->previous_input_magnitude = pad->input_magnitude;
    object->field_0xefd &= ~GAMEOBJECT_MOVEMENT_FLAG_BACKWARDS;
    object->previous_movement_angle = api.field_0x276;
    object->field_0xf1c = api.model_draw_result == 0 ? object->field_0xf1c + FRAMETIME : 0.0f;
    if (object->character_context == 0x2b)
        object->turn_braking += FRAMETIME;
    if (object->field_0xf03 & 4) {
        APIObjectVelocities(object);
        GameObjectOrigin(object);
        return;
    }
    if (api.field_0x287 != 0)
        return;
    pad->allocated_5a &= ~GAMEPAD_RUNTIME_WAGGLED;
    pad->waggle_magnitude = 0.0f;
    object->field_0x107a = -1;
    object->field_0xefd &= ~GAMEOBJECT_MOVEMENT_FLAG_FACE_REVERSED;
    const i32 menu_id = GetMenuID();
    if ((api.field_0x1f8 & APIOBJECT_FLAG_AI_PLAYER_MASK) == APIOBJECT_FLAG_PLAYER_ACTIVE) {
        if (FadeSys.fade > 0.0f || newgamecam != 0 ||
            (static_cast<i8>(api.flags_low) < 0 && (MiniCutCam == 2 || (menu_id != 0x0e && menu_id != -1)))) {
            pad->input_magnitude = 0.0f;
            pad->input_direction_z = 0.0f;
            pad->input_direction_x = 0.0f;
            pad->input_mode = 1;
        } else if (object->pad_gamepad == ViewCamGetGamePad()) {
            pad->input_magnitude = 0.0f;
        } else {
            f32 x, z;
            u32 buttons = pad->buttons_held;
            if (buttons & (GAMEPAD_DLEFT | GAMEPAD_DRIGHT | GAMEPAD_DUP | GAMEPAD_DDOWN)) {
                x = buttons & GAMEPAD_DLEFT ? -127.5f : buttons & GAMEPAD_DRIGHT ? 127.5f : 0.0f;
                z = buttons & GAMEPAD_DDOWN ? -127.5f : buttons & GAMEPAD_DUP ? 127.5f : 0.0f;
                if (x != 0.0f || z != 0.0f)
                    pad->input_mode = 1;
            } else {
                x = static_cast<f32>(analog_x) - 127.5f;
                z = static_cast<f32>(analog_y) - 127.5f;
                if (WORLD->current_level != DOGFIGHTA_LDATA)
                    z = -z;
                if (x * x + z * z < 1806.25f) {
                    x = z = 0.0f;
                } else if (x != 0.0f || z != 0.0f) {
                    pad->input_mode = 2;
                }
            }
            x *= 0.007843137718737125f;
            z *= 0.007843137718737125f;
            NUANG angle = NuAtan2D(x, z);
            NUVEC input, axis = {0.0f, 0.0f, fabsf(x) > fabsf(z) ? fabsf(x) : fabsf(z)};
            NuVecRotateY(&input, &axis, angle);
            f32 magnitude = NuFsqrt(input.x * input.x + input.z * input.z);
            ShiftPadDirectionHistory(object, pad);
            if (0.2f > magnitude) {
                pad->input_magnitude = 0.0f;
            } else {
                GAMECHARACTERDATA *data = api.character_data->game_character;
                if ((api.character_data->model_flags & 0x20) != 0 &&
                    (api.field_0x27c == -1 ? object->current_hp <= 2 : object->field_0xe38 <= 2)) {
                    pad->input_magnitude = data->walk_speed;
                } else if (magnitude < 0.5f && (data->flags_090 & 8) == 0) {
                    pad->input_magnitude = data->tiptoe_speed;
                } else if (magnitude < 0.8f) {
                    pad->input_magnitude = data->walk_speed;
                } else {
                    pad->input_magnitude = data->run_speed;
                }
            }
            // The original player path writes X to +0x2c and Z to +0x30;
            // the AI path below uses the opposite component convention.
            pad->input_direction_z = input.x;
            pad->input_direction_x = input.z;
            if (pad->input_magnitude > 0.0f)
                pad->input_angle = NuAtan2D(input.x, input.z);
            i32 waggle = GamePad_Waggle(object->pad_gamepad);
            pad->allocated_5a = static_cast<u8>((pad->allocated_5a & ~GAMEPAD_RUNTIME_WAGGLED) | ((waggle & 1) << 3));
            pad->waggle_magnitude = (Player[0] == object || Player[1] == object) ? GamePad_Rotate(object) : 0.0f;
        }
    } else {
        if (object->field_0xefc & 0x10) {
            pad->input_magnitude = 0.0f;
            object->pad_gamepad->buttons_held = 0;
            object->pad_gamepad->buttons_pressed = 0;
        } else if (object->ai.movement_stopped == 1) {
            pad->input_magnitude = 0.0f;
            pad->input_mode = 1;
            pad->input_direction_z = pad->input_direction_x = 0.0f;
        } else if (object->character_context == 0x17 && (object->field_0xf04 & 1) == 0) {
            pad->input_magnitude = 0.0f;
        } else {
            f32 x = object->ai.movement_position.x - api.position.x;
            f32 z = object->ai.movement_position.z - api.position.z;
            if (x == 0.0f && z == 0.0f) {
                pad->input_magnitude = 0.0f;
                pad->input_angle = 0;
                pad->input_direction_z = pad->input_direction_x = 0.0f;
            } else {
                f32 distance = NuFsqrt(x * x + z * z);
                f32 inverse = 1.0f / distance;
                pad->input_direction_x = x * inverse;
                pad->input_direction_z = z * inverse;
                pad->input_magnitude = distance / ai_moveradius;
                if (pad->input_magnitude > 1.0f)
                    pad->input_magnitude = 1.0f;
                pad->input_angle = NuAtan2D(pad->input_direction_x, pad->input_direction_z);
                pad->input_angle = NuAngSub(pad->input_angle, GameCam->input_yaw);
            }
            pad->input_mode = 1;
        }
        if (object->context_target_position != NULL) {
            pad->input_magnitude = 0.0f;
        } else {
            GAMECHARACTERDATA *data = api.character_data->game_character;
            f32 run_speed = object->field_0xee0 == 1000000000.0f ? data->run_speed : object->field_0xee0;
            f32 walk_speed =
                object->walk_speed_override == 1000000000.0f ? data->walk_speed : object->walk_speed_override;
            pad->input_magnitude = (object->ai.runtime_flags & 4) ? 0.0f : pad->input_magnitude * run_speed;
            if ((object->ai.runtime_flags & 8) == 0 && pad->input_magnitude < data->tiptoe_speed * 0.5f)
                pad->input_magnitude = 0.0f;
            if ((object->ai.movement_flags & 8) == 0) {
                if (object->ai.goal_speed_mode == 1 && pad->input_magnitude > walk_speed)
                    pad->input_magnitude = walk_speed;
                else if (object->ai.goal_speed_mode == 2 && pad->input_magnitude > data->tiptoe_speed)
                    pad->input_magnitude = data->tiptoe_speed;
            }
            if ((api.field_0x1f4 & 0x400) == 0 && (object->field_0xefb & 2) == 0 &&
                (object->field_0xf1c >= OFFSCREEN_CATCHUP_TIME ||
                 (api.model_draw_result == 0 && (object->tag_flags & 2) == 0 && drop_back_in_timer > 0.0f)) &&
                (pad->input_magnitude + 0.001f >= walk_speed || object->id == id_SPEEDERBIKE)) {
                object->field_0xefd |= GAMEOBJECT_MOVEMENT_FLAG_FACE_REVERSED;
            }
        }
        object->pad_gamepad->operator_data = object->ai.movement_look_target;
    }
    if (static_cast<i8>(api.flags_low) < 0 && menu_id == 0x0e && (object == Player[0] || object == Player[1])) {
        i32 index = object == Player[0] ? 0 : 1;
        Hub_PadSpeed[index] = pad->input_magnitude;
        Hub_PadAngle[index] = pad->input_angle;
        pad->input_mode = 1;
        pad->input_magnitude = 0.0f;
        pad->input_direction_z = pad->input_direction_x = 0.0f;
    } else if (pad->input_magnitude > 0.0f) {
        pad->animation_input_magnitude = pad->input_magnitude;
    }
    if (object->character_context == 0x3b || object->character_context == 0x39)
        return;
    object->field_0x1086 = 2;
    ShoveSystemCheckGameObject(object);
    if (object->id == id_DRAGBOMB) {
        MovePlayer_ROLLING(object);
    } else if (MovePlayer_TWIST(object) == 0 && MovePlayer_CIRCLE(object) == 0 && MovePlayer_GUNSHIPIN(object) == 0 &&
               MovePlayer_POD(object) == 0) {
        if ((api.character_data->model_flags & 0x2000) == 0)
            MovePlayer_DIRECTIONAL(object);
        else
            MovePlayer_VEHICLEDIRECTIONAL(object);
    }
    if (LIFTPLAYER != 0 && menu_id == -1 && api.field_0x287 == 0 && object->character_context != 0x2b &&
        (api.character_data->game_character->flags_090 & 0x80) == 0 && static_cast<i8>(api.flags_low) < 0 &&
        (pad->buttons_held & GAMEPAD_LIFT) != 0 && pad != ViewCamGetGamePad()) {
        GameObject_s *lift = object;
        if (object->character_context == 0x51) {
            TECHNO *techno = static_cast<TECHNO *>(object->field_0x788);
            if (techno != NULL && techno->target_mode == 1 && techno->controlled_object != NULL)
                lift = static_cast<GameObject_s *>(techno->controlled_object);
        }
        lift->apiobj.position.y += 1.5f * FRAMETIME;
        lift->apiobj.velocity.y = 0.0f;
        lift->apiobj.field_0x27d = 0;
        lift->field_0x105c = 0;
        if (lift->character_context == 0x33)
            lift->character_context = -1;
    }
    api.field_0x1fa |= 1;
    api.previous_velocity = api.velocity;
    APIObjectVelocities(object);
    GameObjectOrigin(object);
}

void Move_BEAST(GameObject_s *object);

void Move_BARMAN(GameObject_s *object) {
    DropInOutCode(object);
    ApplyGravity(object, NULL, 0.0f, 0.0f, NULL);
}

void Move_CANNON(GameObject_s *) {
}

void Move_WALKER(GameObject_s *) {
}

void Move_CRITTER(GameObject_s *) {
}

void Move_DEFAULT(GameObject_s *object) {
    DropInOutCode(object);
    if ((object->field_0xe20 & 0x20) == 0)
        ApplyGravity(object, NULL, 0.0f, 0.0f, NULL);
}

void Move_DRAGBOMB(GameObject_s *) {
}

void Move_DROIDEKA(GameObject_s *) {
}

i32 PodLevel(AREADATA_s *area);
f32 ForceAlongSock(GameObject_s *object);
extern LEVELDATA_s *PODRACEB_LDATA;
extern "C" {
    extern f32 pod_roll[2], pod_roll_target[2];
    extern i32 podrace_section;
}

// Original: 1,376 bytes.
i32 MovePlayer_POD(GameObject_s *object) {
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    GAMEPAD_s *pad = object->pad_gamepad;
    f32 input = pad->input_direction_z;
    if (object->apiobj.field_0x27c == -1 || !PodLevel(world->area))
        return 0;
    if (object->current_speed_mul == 0.0f)
        return 1;
    i32 index = object->apiobj.field_0x27c;
    if (index != -1 && WORLD->current_level == PODRACEB_LDATA && podrace_section != 1 &&
        (FadeSys.fade != 0.0f || MiniCutCam != 0) && object->sock_position.location.sock != -1 &&
        object->sock_position.distance < 20.0f)
        input = -0.625f;
    if (index <= 1) {
        pod_roll_target[index] = SeekLinearF(pod_roll_target[index], input, (6.0f * FRAMETIME) * fabsf(input));
        pod_roll_target[index] = SeekLinearF(pod_roll_target[index], 0.0f, 3.0f * FRAMETIME);
        pod_roll[index] = SeekValF(pod_roll[index], pod_roll_target[index], 8.0f);
    }
    object->field_0x1086 = 2;
    u16 heading = object->sock_position.location.sock == -1 ? 0 : object->sock_angles.y;
    u16 pitch = 0, roll = 0;
    if (object->apiobj.field_0x218 != 2000000.0f) {
        f32 height = object->apiobj.collision_min.y - object->apiobj.field_0x218;
        f32 blend;
        if (height < 0.5f)
            blend = 1.0f;
        else if (height < 1.5f)
            blend = 1.0f - (height - 0.5f);
        else
            blend = 0.0f;
        FindAnglesZX(&object->surface_normal, NULL, NULL);
        i32 x = RotDiff(0, temp_xrot);
        i32 z = RotDiff(0, temp_zrot);
        pitch = static_cast<u16>(static_cast<i32>((f32)x * blend));
        roll = static_cast<u16>(static_cast<i32>((f32)z * blend));
    }
    object->apiobj.pitch_angle = SeekRot(object->apiobj.pitch_angle, pitch, 8.0f);
    object->apiobj.roll_angle = SeekRot(object->apiobj.roll_angle, roll, 8.0f);
    if (object->field_0xe20 & 0x20) {
        MoveInactiveVehicle(object, 1, NULL);
    } else {
        object->target_velocity = {0.0f, 0.0f, 0.0f};
        if (object->sock_position.location.sock != -1) {
            object->target_velocity.x = index <= 1
                                            ? pod_roll[index] * object->apiobj.character_data->game_character->run_speed
                                            : input * pad->input_magnitude;
        }
        KeepVehicleOnScreen(object, 1, 1, 1);
        if (object->target_velocity.x != 0.0f || object->target_velocity.y != 0.0f || object->target_velocity.z != 0.0f)
            NuVecRotateY(&object->target_velocity, &object->target_velocity, heading);
        if (object->sock_position.location.sock != -1)
            ForceAlongSock(object);
        f32 seek_rate = object->apiobj.character_data->game_character->velocity_seek_rate;
        object->apiobj.velocity.x = SeekValF(object->apiobj.velocity.x, object->target_velocity.x, seek_rate);
        object->apiobj.velocity.z = SeekValF(object->apiobj.velocity.z, object->target_velocity.z, seek_rate);
    }
    object->apiobj.movement_facing_angle = NuAtan2D(object->target_velocity.x, object->target_velocity.z);
    object->apiobj.facing_angle = SeekRot(object->apiobj.facing_angle, object->apiobj.movement_facing_angle, 2.0f);
    object->apiobj.field_0x276 = object->apiobj.facing_angle;
    return 1;
}

void Move_CHARACTER(GameObject_s *object);

void Move_GEONOSIAN(GameObject_s *) {
}

void Move_HOVERDROID(GameObject_s *) {
}

void MovePlayerSpline(GameObject_s *) {
}

i32 TwistLevel(LEVELDATA_s *level);
extern f32 CurrentSpeed;
extern f32 avg_currentspeed_mul;

f32 ForceAlongSock(GameObject_s *object) {
    if (object->sock_position.location.sock == -1 || CurrentSpeed == 0.0f)
        return 0.0f;
    f32 speed = (1.0f + object->field_0xc38) * (CurrentSpeed * avg_currentspeed_mul);
    NUVEC force = {0.0f, 0.0f, speed};
    NuVecRotateX(&force, &force, object->sock_position.midpoint_rotation.x);
    NuVecRotateY(&force, &force, object->sock_position.midpoint_rotation.y);
    NuVecAdd(&object->target_velocity, &object->target_velocity, &force);
    return speed;
}

// Original: 1,336 bytes.
i32 MovePlayer_TWIST(GameObject_s *object) {
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    GAMEPAD_s *pad = object->pad_gamepad;
    if (object->apiobj.field_0x27c == -1 || !TwistLevel(world->current_level) || world->sock_sys == NULL)
        return 0;
    object->field_0x1086 = 4;
    if (object->sock_position.location.sock == -1 ||
        (world->sock_sys->sock[object->sock_position.location.sock].flags & 1))
        return 0;
    i32 yaw_lean = static_cast<i32>(2730.0f * pad->input_direction_z);
    i32 pitch_lean = static_cast<i32>(-pad->input_direction_x * 2730.0f);
    NUMTX orientation;
    SockRotationMatrix(world->sock_sys, &object->sock_position, &orientation, 3, 5);
    object->vehicle_orientation = orientation;
    u16 previous_lean;
    if (object->field_0xe20 & 0x20) {
        GameObject_s *vehicle;
        MoveInactiveVehicle(object, 1, &vehicle);
        if (vehicle != NULL) {
            object->apiobj.field_0x276 = vehicle->apiobj.field_0x276;
            object->apiobj.movement_facing_angle = vehicle->apiobj.field_0x276;
            object->apiobj.facing_angle = vehicle->apiobj.field_0x276;
            object->apiobj.velocity = vehicle->apiobj.velocity;
            object->vehicle_orientation = vehicle->vehicle_orientation;
            object->field_0xdc8 = vehicle->field_0xdc8;
            object->movement_lean_angle = 0;
            object->tertiary_lean_angle = 0;
            object->secondary_lean_angle = 0;
            object->apiobj.roll_angle = 0;
            object->apiobj.pitch_angle = 0;
            object->sock_position = vehicle->sock_position;
            previous_lean = 0;
        } else {
            previous_lean = object->tertiary_lean_angle;
        }
    } else {
        object->target_velocity = {0.0f, 0.0f, 0.0f};
        f32 speed = (pad->input_magnitude / object->apiobj.character_data->game_character->run_speed) * 8.0f;
        if (speed > 0.0f) {
            object->target_velocity.x = pad->input_direction_z * speed;
            object->target_velocity.y = -pad->input_direction_x * speed;
        }
        KeepVehicleOnScreen(object, 1, 1, 1);
        object->target_velocity.z = 0.0f;
        NuVecMtxRotate(&object->target_velocity, &object->target_velocity, &object->vehicle_orientation);
        ForceAlongSock(object);
        SeekVec(&object->apiobj.velocity, &object->apiobj.velocity, &object->target_velocity,
                object->apiobj.character_data->game_character->velocity_seek_rate);
        previous_lean = object->tertiary_lean_angle;
    }
    object->tertiary_lean_angle = SeekRot(previous_lean, static_cast<u16>(yaw_lean), 5.0f);
    object->movement_lean_angle = SeekRot(object->movement_lean_angle, static_cast<u16>(yaw_lean), 5.0f);
    object->secondary_lean_angle = SeekRot(object->secondary_lean_angle, static_cast<u16>(pitch_lean), 5.0f);
    return 1;
}

void Move_SPEEDERBIKE(GameObject_s *) {
}

// Original: 1,000 bytes.
i32 MovePlayer_CIRCLE(GameObject_s *object) {
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    GAMEPAD_s *pad = object->pad_gamepad;
    if (object->apiobj.field_0x27c == -1 || !CircleLevel(world->current_level) || world->sock_sys == NULL ||
        object->sock_position.location.sock == -1)
        return 0;
    object->field_0x1086 = 3;
    u16 heading = world->sock_sys->sock[object->sock_position.location.sock].input_yaw +
                  object->sock_position.midpoint_rotation.y;
    object->current_input_angle = heading + pad->input_angle;
    object->field_0xe22 |= GAMEOBJECT_E22_FLAG_INPUT_ANGLE_VALID;
    object->apiobj.movement_facing_angle = heading;
    object->apiobj.facing_angle = SeekRot(object->apiobj.facing_angle, heading, 8.0f);
    object->apiobj.field_0x276 = object->apiobj.facing_angle;
    object->movement_lean_angle = SeekRot(object->movement_lean_angle,
                                          static_cast<u16>(static_cast<i32>(5461.0f * pad->input_direction_z)), 8.0f);
    GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
    f32 seek_rate = data->velocity_seek_rate;
    if (object->character_context == 0x23 || object->character_context == 0x24) {
        object->target_velocity = {0.0f, 0.0f, 0.0f};
    } else if (object->field_0xe20 & 0x20) {
        GameObject_s *vehicle;
        MoveInactiveVehicle(object, 0, &vehicle);
        if (vehicle != NULL) {
            object->apiobj.field_0x276 = vehicle->apiobj.field_0x276;
            object->apiobj.movement_facing_angle = vehicle->apiobj.field_0x276;
            object->apiobj.facing_angle = vehicle->apiobj.field_0x276;
            object->apiobj.velocity = vehicle->apiobj.velocity;
            object->field_0xdc8 = vehicle->field_0xdc8;
            object->movement_lean_angle = 0;
            object->tertiary_lean_angle = 0;
            object->secondary_lean_angle = 0;
            object->apiobj.roll_angle = 0;
            object->apiobj.pitch_angle = 0;
        }
        return 1;
    } else {
        object->target_velocity.y = 0.0f;
        if (object->apiobj.movement_direction.x != 0.0f || object->apiobj.movement_direction.z != 0.0f) {
            object->target_velocity.x = object->apiobj.movement_direction.x;
            object->target_velocity.z = object->apiobj.movement_direction.z;
            object->apiobj.movement_direction.x = 0.0f;
            object->apiobj.movement_direction.z = 0.0f;
        } else {
            object->target_velocity.x =
                pad->input_magnitude > 0.0f ? pad->input_magnitude * pad->input_direction_z : 0.0f;
            object->target_velocity.z = 0.0f;
            if (object->field_0x1024 > 0.0f && (object->flicker_flags & 7) != 0) {
                f32 maximum = 1.5f * data->run_speed;
                f32 impulse = (object->field_0x1024 / 0.4f) * maximum;
                if ((object->flicker_flags & 7) == 3) {
                    object->target_velocity.x = object->target_velocity.x * 0.5f - impulse;
                    if (-maximum > object->target_velocity.x)
                        object->target_velocity.x = -maximum;
                } else {
                    f32 speed = object->target_velocity.x * 0.5f + impulse;
                    object->target_velocity.x = maximum < speed ? maximum : speed;
                }
            }
            KeepVehicleOnScreen(object, 1, 0, 0);
            NuVecRotateY(&object->target_velocity, &object->target_velocity, heading);
        }
        ForceTowardsMid(object);
    }
    object->apiobj.velocity.x = SeekValF(object->apiobj.velocity.x, object->target_velocity.x, seek_rate);
    object->apiobj.velocity.z = SeekValF(object->apiobj.velocity.z, object->target_velocity.z, seek_rate);
    return 1;
}

static __used__ void ZapCode(GameObject_s *, i32, i32) {
}

static __used__ void FireCode(GameObject_s *, i32, i32, f32, i32) {
}

static void SelfDestructCode(GameObject_s *object, i32 pressed) {
    if (static_cast<i8>(object->apiobj.flags_low) >= 0 || object->apiobj.field_0x287 != 0 ||
        object->character_context == 0x0b || object->character_context == 0x16 || object->character_context == 0x2b ||
        pressed == 0)
        return;
    InstantKillParts(object, 1, 0.0f);
    EXPLOSION *explosion = Detonate(&object->apiobj.collision_position, 0);
    if (explosion != NULL && Arcade != 0 && static_cast<u8>(object->apiobj.field_0x27c) <= 1) {
        explosion->field_0x24 |= 0x10000;
        explosion->object = object;
    }
    KillPlayer(object, 2, 1, NULL);
    if (object->camera_screen_position.z > 0.0f)
        AddFancyMessage(TTab[tCHEAT_SELFDESTRUCT], object->camera_screen_position.x, object->camera_screen_position.y,
                        0.25f, 1.0f, 1, 0);
}

void Move_DROIDGENERIC(GameObject_s *object) {
    const i32 jump_pressed = GAMEPAD_JUMP & object->pad_gamepad->buttons_pressed;
    const i32 jump_held = GAMEPAD_JUMP & object->pad_gamepad->buttons_held;
    i32 zap_pressed = (GAMEPAD_ACTION | GAMEPAD_SPECIAL) & object->pad_gamepad->buttons_pressed;
    if ((object->apiobj.character_data->model_flags & 0x40) != 0) {
        if (Cheat[32].enabled != 0)
            zap_pressed = GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed;
        KeepWeaponIn(object);
    } else if (object->apiobj.character_data->game_character->uses_weapon_action == 4 || object->id == id_PROBEDROID) {
        KeepWeaponOut(object);
    }
    DropInOutCode(object);
    ApplyGravity(object, NULL, 0.0f, object->id == id_BATTLEDROID ? 0.0f : 8.0f, NULL);
    if ((object->apiobj.character_data->model_flags & 0x10) != 0 || VehicleArea == 0) {
        TakeHitCode(object);
        if ((object->apiobj.character_data->model_flags & 0x40) != 0)
            FloatCode(object);
        SlideCode(object);
        FlattenCode(object);
        ForcePushed_MoveCode(object);
        ForcedBackCode(object);
        Tube_MoveCode(object, WORLD);
    }
    if ((object->apiobj.character_data->model_flags & 0x40) != 0) {
        JumpCode(object, jump_pressed, jump_held, 4, 0, 0, -1);
        GizPanel_MoveCode(WORLD, object, (GAMEPAD_SPECIAL | GAMEPAD_ACTION) & object->pad_gamepad->buttons_pressed);
    } else {
        Glide_MoveCode(object);
        if (object->id == id_SUPERBATTLEDROID) {
            WeaponOutCode(object);
            WeaponInCode(object);
            WeaponScalingCode(object);
            ShootCode(object, GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed,
                      GAMEPAD_SPECIAL & object->pad_gamepad->buttons_pressed, 1, 0, 0);
        } else if (object->id == id_GONKDROID) {
            if (Cheat_IsOn(8)) {
                JumpCode(object, jump_pressed, jump_held, 0x40, 0, 0, -1);
                goto zap;
            }
        } else if ((object->apiobj.character_data->model_flags & 0x20) == 0 &&
                   (object->apiobj.character_data->game_character->flags_094[0] & 8) == 0) {
            ShootCode(object, GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed, 0, 0, 0, 0);
        }
        JumpCode(object, 0, 0, 0x80, 0, 0, -1);
    }
zap:
    if (object->id == id_BUZZDROID)
        PunchCode(object, GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed, 0, 0, 1, 0.0f);
    ZapCode(object, zap_pressed, 0);
    DeactivatedCode(object);
    if (object->character_context == -1 && object->field_0xe31 == 0 && object->apiobj.field_0x27d != 0 &&
        object->apiobj.field_0x27e == 0 &&
        ((object->movement_runtime_flags & 4) != 0 ||
         (object->fall_animation_timer >= 0.2f && (object->pad_gamepad->input_magnitude == 0.0f ||
                                                   (static_cast<i8>(object->apiobj.flags_low) >= 0 &&
                                                    object->apiobj.character_model->model_data_b[0x59] != NULL)))))
        StartFallLand(object, -1);
    if (object->apiobj.field_0x27d != 0)
        object->movement_runtime_flags &= ~4;
    if ((object->apiobj.character_data->model_flags & 0x10) != 0) {
        if ((object->apiobj.character_data->model_flags & 0x40) != 0) {
            PeriscodeCode(object);
        } else {
            if (VehicleArea == 0) {
                if (object->character_context == -1 && object->field_0xe31 == 0 && object->apiobj.field_0x27d != 0 &&
                    object->apiobj.field_0x27e == 0 &&
                    ((object->movement_runtime_flags & 4) != 0 ||
                     (object->fall_animation_timer >= 0.2f &&
                      (object->pad_gamepad->input_magnitude == 0.0f ||
                       (static_cast<i8>(object->apiobj.flags_low) >= 0 &&
                        object->apiobj.character_model->model_data_b[0x59] != NULL)))))
                    StartFallLand(object, -1);
                if (object->apiobj.field_0x27d != 0)
                    object->movement_runtime_flags &= ~4;
            }
            if (object->id == id_NAFFDROID1 || object->id == id_NAFFDROID2 || object->id == id_NAFFDROID4 ||
                object->id == id_MOUSEDROID) {
                if (object->apiobj.field_0x27d != 0 && object->character_context == -1 &&
                    object->pad_gamepad->input_magnitude > 0.0f)
                    PlaySfx(const_cast<char *>(object->id == id_MOUSEDROID ? "drd_mousebot_lp" : "R2Move"),
                            &object->apiobj.collision_position);
            } else if (object->id == id_PROBEDROID) {
                PlaySfx("Probot_EngLp", &object->apiobj.collision_position);
                FireCode(object, GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed,
                         GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed, 0.5f, 1);
            }
        }
    }
    if (static_cast<i8>(object->apiobj.character_data->game_character->flags_094[0]) < 0)
        CommunicateCode(object, GAMEPAD_SPECIAL & object->pad_gamepad->buttons_pressed, 0);
    if ((object->apiobj.character_data->model_flags & 0x20) != 0) {
        GizPanel_MoveCode(WORLD, object, (GAMEPAD_SPECIAL | GAMEPAD_ACTION) & object->pad_gamepad->buttons_pressed);
        if (WORLD->current_level == HOTHESCAPEB_LDATA)
            BigJumpCode(object);
    }
    const i32 special_pressed = GAMEPAD_SPECIAL & object->pad_gamepad->buttons_pressed;
    if ((object->apiobj.character_data->model_flags & 0x10) != 0 && (object->movement_runtime_flags & 2) == 0 &&
        Cheat_IsOn(0x20))
        SelfDestructCode(object, special_pressed);
    GizmoBlowupCheckProximity(WORLD, object);
}

void GunShip_DragBombSeekBlowUp(GameObject_s *object);

// Original: 2,011 bytes.
void MovePlayer_ROLLING(GameObject_s *object) {
    object->field_0xe23 &= ~0x10;
    object->target_velocity.y = ObjInTube(object) ? 1.25f : 0.0f;
    f32 seek_rate = object->id == id_DRAGBOMB && (object->field_0xf01 & 2)
                        ? 1.0f
                        : object->apiobj.character_data->game_character->velocity_seek_rate;
    object->target_velocity.x = 0.0f;
    object->target_velocity.z = 0.0f;
    if (object->character_context == 0x2b) {
        if (1.0f > object->turn_braking) {
            object->target_velocity.x = (1.0f - object->turn_braking) * 0.0f;
            object->target_velocity.z = object->target_velocity.x;
        }
        if (object->field_0xe36 == 2) {
            object->target_velocity.x *= 0.5f;
            object->target_velocity.z *= 0.5f;
            NewRumble(object->pad_gamepad->pad, (qrand() * 1.5259022e-05f) * 0.5f, 0);
            i32 effect = WORLD->debris_sys->entries[58].effect;
            if (effect != -1) {
                i32 count = ParticlesPerSecond(60.0f, FRAMETIME);
                if (count > 0) {
                    NUVEC position = {object->apiobj.collision_position.x, object->apiobj.water_height,
                                      object->apiobj.collision_position.z};
                    AddVariableShotDebrisEffect(effect, &position, count, 0, 0);
                }
            }
        }
    }
    object->apiobj.velocity.x = SeekValF(object->apiobj.velocity.x, object->target_velocity.x, seek_rate);
    object->apiobj.velocity.z = SeekValF(object->apiobj.velocity.z, object->target_velocity.z, seek_rate);
    if (WORLD->area != NULL && WORLD->area == GUNSHIP_ADATA)
        GunShip_DragBombSeekBlowUp(object);
    object->field_0x1086 = 0;
    NUMTX matrix = object->apiobj.field_0xb8;
    matrix.m30 = matrix.m31 = matrix.m32 = 0.0f;
    i32 heading = NuAtan2D(object->apiobj.velocity.x, object->apiobj.velocity.z);
    NUVEC local_velocity;
    NuVecRotateY(&local_velocity, &object->apiobj.velocity, -heading);
    NuMtxRotateY(&matrix, -heading);
    NuMtxRotateX(&matrix, static_cast<i32>(((local_velocity.z * FRAMETIME) / object->apiobj.collision_radius) *
                                           10430.3779296875f));
    NuMtxRotateY(&matrix, heading);
    i32 pitch, yaw, roll;
    NuMtxGetEulerXYZ(&matrix, &pitch, &yaw, &roll);
    object->apiobj.pitch_angle = pitch;
    object->apiobj.field_0x276 = yaw;
    object->apiobj.roll_angle = roll;
    GizmoBlowupCheckProximity(WORLD, object);
}

void MoveSplinePosition(SPLINEPOS_s *position, f32 distance) {
    if (position == NULL || position->spline == NULL || position->spline->length <= 1)
        return;
    NUGSPLINE *spline = position->spline;
    i32 segments = spline->length - 1 + (position->looping != 0);
    if (position->segment >= segments)
        return;
    NUVEC offset;
    if (distance > 0.0f) {
        while (distance > 0.0f) {
            position->segment_distance += distance;
            if (!(position->segment_distance >= position->segment_length))
                break;
            distance = position->segment_distance - position->segment_length;
            i16 previous = position->segment;
            position->segment++;
            if (position->segment >= segments) {
                if (!position->looping) {
                    position->reached_end = 1;
                    position->position = *(NUVEC *)((u8 *)spline->pts + position->segment * (i16)spline->pt_size);
                    position->segment = previous;
                    position->along = 1.0f;
                    position->segment_distance = position->segment_length;
                    return;
                }
                position->segment = 0;
            }
            NUVEC *current = (NUVEC *)((u8 *)spline->pts + position->segment * (i16)spline->pt_size);
            NUVEC *next =
                (NUVEC *)((u8 *)spline->pts + ((position->segment + 1) % spline->length) * (i16)spline->pt_size);
            position->segment_distance = 0.0f;
            position->segment_length = NuVecDist(next, current, &offset);
            if (distance == 0.0f)
                position->position = *current;
            spline = position->spline;
        }
        if (!(distance > 0.0f))
            goto update_along;
    } else if (distance < 0.0f && position->segment >= 0) {
        for (;;) {
            position->segment_distance += distance;
            if (!(position->segment_distance < 0.0f))
                break;
            distance = position->segment_distance;
            i16 previous = position->segment;
            position->segment--;
            if (position->segment < 0) {
                if (!position->looping) {
                    position->reached_end = 1;
                    position->position = *spline->pts;
                    position->segment = previous;
                    position->along = 0.0f;
                    position->segment_distance = 0.0f;
                    return;
                }
                position->segment = segments - 1;
            }
            NUVEC *current = (NUVEC *)((u8 *)spline->pts + position->segment * (i16)spline->pt_size);
            NUVEC *next =
                (NUVEC *)((u8 *)spline->pts + ((position->segment + 1) % spline->length) * (i16)spline->pt_size);
            position->segment_length = NuVecDist(next, current, &offset);
            position->segment_distance = position->segment_length;
            if (distance == 0.0f)
                position->position = *next;
            spline = position->spline;
        }
    } else
        goto update_along;
    {
        NUVEC *current = (NUVEC *)((u8 *)spline->pts + position->segment * (i16)spline->pt_size);
        NUVEC *next = (NUVEC *)((u8 *)spline->pts + ((position->segment + 1) % spline->length) * (i16)spline->pt_size);
        NuVecSub(&offset, next, current);
        f32 fraction = 0.0f;
        if (position->segment_length != 0.0f)
            fraction = position->segment_distance / position->segment_length;
        NuVecScale(&offset, &offset, fraction);
        NuVecAdd(&position->position, current, &offset);
    }
update_along:
    position->along = (position->segment_distance / position->segment_length + position->segment) / segments;
}

void MoveBlocksOverBlock(WORLDINFO_s *world, pushblock_s *block, i32 excluded, nuvec_s *velocity) {
    if (block->runtime_flags_0c9 & 4)
        return;
    NUVEC *base = block->position;
    f32 xmin = block->bounds_min.x + base->x, xmax = base->x + block->bounds_max.x;
    f32 zmin = block->bounds_min.z + base->z, zmax = base->z + block->bounds_max.z;
    for (i32 i = 0; i < world->push_block_count; ++i) {
        if (i == excluded)
            continue;
        pushblock_s *other = &world->push_blocks[i];
        if ((other->flags_0cb & 4) || !(other->flags_0ca & 4))
            continue;
        NUVEC *position = other->position;
        if (base->y >= position->y)
            continue;
        if (!(position->x >= (xmin - other->bounds_max.x) + 0.01f &&
              position->x <= (xmax - other->bounds_min.x) - 0.01f &&
              position->z >= (zmin - other->bounds_max.z) + 0.01f &&
              position->z <= (zmax - other->bounds_min.z) - 0.01f))
            continue;
        other->snap_origin = *position;
        position->x += velocity->x;
        position->y -= velocity->y;
        other->ground_offset -= velocity->y;
        position->z += velocity->z;
        other->runtime_flags_0c8 |= 4;
        if (BlockInBlock(world, other, i, &other->supporting_block))
            other->runtime_flags_0c9 |= 4;
        NuSpecialUpdate(&other->special);
    }
}

void MoveInactiveVehicle(GameObject_s *object, i32, GameObject_s **followed_object) {
    object->target_velocity.x = object->target_velocity.y = object->target_velocity.z = 0.0f;
    GameObject_s *other;
    if (object == Player[0] || object == Player[1]) {
        other = object == Player[0] ? Player[1] : Player[0];
        NUVEC position = other->apiobj.position;
        SeekVec(&object->apiobj.position, &object->apiobj.position, &position, 10.0f);
        object->apiobj.velocity = object->target_velocity;
        object->apiobj.facing_angle = object->apiobj.movement_facing_angle = object->apiobj.field_0x276 =
            other->apiobj.field_0x276;
    } else {
        NUVEC position = v000;
        float count = 0.0f;
        if (Player[0] != NULL) {
            NuVecAdd(&position, &position, &Player[0]->apiobj.position);
            count += 1.0f;
        }
        if (Player[1] != NULL) {
            NuVecAdd(&position, &position, &Player[1]->apiobj.position);
            count += 1.0f;
        }
        if (count > 0.0f) {
            NuVecScale(&position, &position, 1.0f / count);
        }
        SeekVec(&object->apiobj.position, &object->apiobj.position, &position, 10.0f);
        object->apiobj.velocity = object->target_velocity;
        other = NULL;
    }
    if (followed_object != NULL) {
        *followed_object = other;
    }
    object->delayed_turn_timer = 0.0f;
    object->field_0xddc = 0.0f;
}

i32 GunshipInLevel(LEVELDATA_s *level);
extern i32 LevFlag[4];
// Original exported level control, initialized to 2 at 0x623d00.
i32 GUNSHIPAHACK = 2;

// Original: 1,232 bytes.
i32 MovePlayer_GUNSHIPIN(GameObject_s *object) {
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    GAMEPAD_s *pad = object->pad_gamepad;
    f32 input_x = pad->input_direction_z;
    f32 input_z = pad->input_direction_x;
    f32 magnitude = pad->input_magnitude;
    if (object->apiobj.field_0x27c == -1 || !GunshipInLevel(world->current_level) || world->sock_sys == NULL ||
        object->sock_position.location.sock == -1)
        return 0;
    object->field_0x1086 = 3;
    u16 heading = object->sock_angles.y;
    if (object->character_context == 0x23 || object->character_context == 0x24) {
        object->target_velocity = {0.0f, 0.0f, 0.0f};
        ForceAlongSock(object);
        object->apiobj.velocity.x = SeekValF(object->apiobj.velocity.x, object->target_velocity.x, 0.0f);
        object->apiobj.velocity.z = SeekValF(object->apiobj.velocity.z, object->target_velocity.z, 0.0f);
    } else if (object->field_0xe20 & 0x20) {
        GameObject_s *vehicle;
        MoveInactiveVehicle(object, 1, &vehicle);
        if (vehicle != NULL) {
            object->apiobj.field_0x276 = vehicle->apiobj.field_0x276;
            object->apiobj.facing_angle = vehicle->apiobj.field_0x276;
            object->apiobj.velocity = vehicle->apiobj.velocity;
            object->field_0xdc8 = vehicle->field_0xdc8;
            object->movement_lean_angle = 0;
            object->tertiary_lean_angle = 0;
            object->secondary_lean_angle = 0;
            object->apiobj.roll_angle = 0;
            object->apiobj.pitch_angle = 0;
        }
    } else {
        GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
        f32 seek_rate = data->velocity_seek_rate;
        object->target_velocity.y = 0.0f;
        if (object->apiobj.movement_direction.x != 0.0f || object->apiobj.movement_direction.z != 0.0f) {
            object->target_velocity.x = object->apiobj.movement_direction.x;
            object->target_velocity.z = object->apiobj.movement_direction.z;
            object->apiobj.movement_direction.x = 0.0f;
            object->apiobj.movement_direction.z = 0.0f;
        } else {
            if (((u8 *)LevFlag)[1] != 0) {
                input_x = object->apiobj.position.x < -1.0f ? 1.0f : object->apiobj.position.x > 1.0f ? -1.0f : 0.0f;
                magnitude = data->run_speed;
                input_z = 0.0f;
            }
            object->target_velocity.z = 0.0f;
            if (magnitude > 0.0f) {
                object->target_velocity.x = input_x * magnitude;
                if (GUNSHIPAHACK != 0)
                    object->target_velocity.z = input_z * magnitude;
            } else {
                object->target_velocity.x = 0.0f;
            }
            if (object->field_0x1024 > 0.0f && (object->flicker_flags & 7) != 0) {
                f32 maximum = 1.5f * data->run_speed;
                f32 impulse = (object->field_0x1024 / 0.4f) * maximum;
                if ((object->flicker_flags & 7) == 3) {
                    f32 speed = object->target_velocity.z * 0.5f + impulse;
                    object->target_velocity.z = maximum < speed ? maximum : speed;
                } else {
                    object->target_velocity.z = object->target_velocity.z * 0.5f - impulse;
                    if (-maximum > object->target_velocity.z)
                        object->target_velocity.z = -maximum;
                }
            }
            NuVecRotateY(&object->target_velocity, &object->target_velocity, heading);
            ForceAlongSock(object);
        }
        object->apiobj.velocity.x = SeekValF(object->apiobj.velocity.x, object->target_velocity.x, seek_rate);
        object->apiobj.velocity.z = SeekValF(object->apiobj.velocity.z, object->target_velocity.z, seek_rate);
    }
    object->apiobj.movement_facing_angle = heading;
    object->apiobj.facing_angle = SeekRot(object->apiobj.facing_angle, heading, 8.0f);
    object->apiobj.field_0x276 = object->apiobj.facing_angle;
    object->apiobj.pitch_angle = SeekRot(object->apiobj.pitch_angle, 0, 8.0f);
    u16 lean = (object->character_context == 0x23 || object->character_context == 0x24)
                   ? 0
                   : static_cast<u16>(static_cast<i32>(input_x * 5461.0f));
    object->movement_lean_angle = SeekRot(object->movement_lean_angle, lean, 8.0f);
    return 1;
}

void Move_REPUBLICGUNSHIP(GameObject_s *) {
}

void Move_SUPERBATTLEDROID(GameObject_s *) {
}

void MovePlayer_DIRECTIONAL(GameObject_s *object) {
    GAMEPAD_s *pad = object->pad_gamepad;
    GameObject_s *operator_object = object;
    const i32 current_animation = CurrentAnim(&object->apiobj.anim_packet);
    Techno_FindOperator(object, &pad, &operator_object);
    GAMECHARACTERDATA *game_character = object->apiobj.character_data->game_character;
    const f32 run_speed = game_character->run_speed;
    const f32 input_speed = pad->input_magnitude;
    const f32 operator_run_speed = operator_object->apiobj.character_data->game_character->run_speed;
    APIOBJECT &api = object->apiobj;

    // Original entry path, 0x16716f..0x16722a: attached inactive vehicles own movement.
    if ((object->field_0xe20 & 0x20) != 0 && object->character_context != 0x23 && object->character_context != 0x24) {
        GameObject_s *vehicle;
        MoveInactiveVehicle(object, 0, &vehicle);
        if (vehicle != NULL) {
            object->field_0xdc8 = vehicle->field_0xdc8;
            api.field_0x276 = vehicle->apiobj.field_0x276;
            api.movement_facing_angle = vehicle->apiobj.field_0x276;
            api.facing_angle = vehicle->apiobj.field_0x276;
            api.velocity = vehicle->apiobj.velocity;
            object->movement_lean_angle = 0;
            object->tertiary_lean_angle = 0;
            object->secondary_lean_angle = 0;
            api.roll_angle = 0;
            api.pitch_angle = 0;
        }
        return;
    }

    object->field_0xe23 &= ~0x10;
    if (MovingBackwards(object) || static_cast<i8>(object->field_0xefd) < 0) {
        object->field_0xefd |= GAMEOBJECT_MOVEMENT_FLAG_BACKWARDS;
    }
    if (object->character_context == 0x35) {
        if (api.field_0x27d != 0)
            object->character_context = -1;
        else
            PlaySfx("GrapWindLp", &api.collision_position);
    }
    const u16 input_angle = GamePad_InputAngle(object, pad);
    object->field_0xe22 |= GAMEOBJECT_E22_FLAG_INPUT_ANGLE_VALID;
    object->current_input_angle = input_angle;

    if (object->delayed_turn_timer > 0.0f) {
        if (pad->input_magnitude > 0.0f) {
            object->delayed_turn_target_angle = input_angle;
        }
        object->delayed_turn_timer -= FRAMETIME;
        if (object->delayed_turn_timer <= 0.0f) {
            api.movement_facing_angle = object->delayed_turn_target_angle;
        }
    } else {
        const bool has_turn_animation = api.character_model != NULL && (api.character_model->model_data_b[12] != NULL ||
                                                                        api.character_model->model_data_b[119] != NULL);
        if (has_turn_animation && (object->character_context == 1 || object->character_context == -1) &&
            pad->input_magnitude > 0.0f) {
            i32 difference = RotDiff(api.movement_facing_angle, input_angle);
            if (difference < 0)
                difference = -difference;
            if (difference > 0x6aaa) {
                object->delayed_turn_timer = 0.2f;
                object->delayed_turn_target_angle = input_angle;
            }
        }
    }

    const f32 desired_speed = run_speed * (input_speed / operator_run_speed);
    if (object->character_context == 0x33 && (api.character_data->game_character->flags_090 & 0x200) != 0) {
        object->field_0x1086 = 2;
    } else if ((object->movement_context_state & 0xffff00) == 0x54300) {
        Climb_UpdateMagnetRotation(object);
    } else if ((api.character_data->model_flags & 0x100100) == 0x100000) {
        if (object->action_movement_state == 1) {
            api.pitch_angle = 0;
            api.roll_angle = 0;
        } else {
            object->field_0x1086 = 0;
            u16 pitch = 0;
            if (object->character_context == 0x47) {
                pitch = object->magnet_surface_angle;
                ZIPUP *zipup = static_cast<ZIPUP *>(object->field_0x788);
                if (zipup != NULL && (zipup->flags & 1) != 0) {
                    pitch -= static_cast<i32>(static_cast<f32>(zipup->pitch_adjustment) *
                                              (object->context_animation_timer / 1.5f));
                }
            }
            api.pitch_angle = SeekRot(api.pitch_angle, pitch, 8.0f);
            api.roll_angle = SeekRot(api.roll_angle, 0, 8.0f);
        }
    } else if (object->id == id_GRABCONTROL || object->id == id_GRABR2CONTROL) {
        object->field_0x1086 = 2;
        u16 pitch = 0;
        u16 roll = 0;
        if (pad->input_magnitude > 0.0f) {
            NUVEC direction;
            NuVecRotateX(&direction, &v010,
                         static_cast<i32>((desired_speed / api.character_data->game_character->run_speed) * 2730.0f));
            NuVecRotateY(&direction, &direction, input_angle);
            FindAnglesZX(&direction, &pitch, &roll);
        }
        api.pitch_angle = SeekRot(api.pitch_angle, pitch, 8.0f);
        api.roll_angle = SeekRot(api.roll_angle, roll, 8.0f);
    }
    f32 turn_override = 0.0f;
    i32 heading_handled = 0;
    const u32 context_flags = CInfo[object->character_context].flags;
    if ((context_flags & 0x40) != 0) {
        if (api.velocity.x != 0.0f || api.velocity.z != 0.0f)
            api.movement_facing_angle = NuAtan2D(api.velocity.x, api.velocity.z);
    } else if ((context_flags & 0x4000) != 0) {
        api.movement_facing_angle = static_cast<u16>(GameCam->yaw + 0x8000);
    } else if (object->id == id_GRABCONTROL || object->id == id_GRABR2CONTROL) {
        if (WORLD->grabber != NULL) {
            NUVEC *position = Grabber_GetGrabPos(WORLD->grabber, NULL);
            api.movement_facing_angle =
                NuAtan2D(position->x - api.collision_position.x, position->z - api.collision_position.z);
        }
    } else if ((api.character_data->game_character->flags_094[0] & 2) == 0 && object->character_context != 0x3d &&
               object->character_context != 0x15 && object->character_context != 0x5a &&
               object->character_context != 0x0f && object->character_context != 0x1f &&
               object->character_context != 0x3c &&
               (object->character_context != 0x17 || (object->field_0xf04 & 1) != 0) &&
               object->character_context != 0x41 && object->character_context != 0x0b &&
               (object->character_context != 0 ||
                ((object->action_movement_state != 6 && object->action_movement_state != 7 &&
                  object->action_movement_state != 9) ||
                 object->context_variant_flags < 0)) &&
               object->character_context != 0x47 &&
               (object->character_context != 0x29 || api.character_model->model_data_b[0x57] != NULL ||
                (api.character_data->game_character->flags_094[2] & 0x10) != 0) &&
               ((object->field_0xeff & 2) != 0 || CurrentAnim(&api.anim_packet) != 5) &&
               object->character_context != 0x35) {
        switch (object->character_context) {
            case 0:
                if ((context_flags & 0x2000) == 0 && object->action_movement_state == 3) {
                    FaceOpponent(object, NULL);
                    break;
                }
                goto directional_common_heading;
            case 0x46:
                if (object->field_0x7a3 == 1 || GrappleSwingMode == 1)
                    break;
                goto directional_common_heading;
            case 0x0a:
                if ((context_flags & 0x2000) != 0 || static_cast<u16>(object->context_animation - 0x5a) > 2)
                    goto directional_common_heading;
                api.movement_facing_angle = NuAtan2D(object->attack_target_position.x - api.position.x,
                                                     object->attack_target_position.z - api.position.z);
                if (object->context_animation == 0x5a)
                    api.movement_facing_angle += 0x4000;
                else if (object->context_animation == 0x5b)
                    api.movement_facing_angle -= 0x4000;
                else
                    api.movement_facing_angle += 0x8000;
                break;
            case 0x18: {
                if ((context_flags & 0x2000) != 0)
                    goto directional_common_heading;
                NUVEC *position = NULL;
                if (object->incoming_bolt != NULL) {
                    if (object->incoming_bolt->active != 0)
                        position = &object->incoming_bolt->position;
                } else if (object->incoming_melee != NULL) {
                    if ((object->incoming_melee->apiobj.field_0x1f8 & 0x1000) != 0 &&
                        object->incoming_melee->apiobj.field_0x287 == 0)
                        position = &object->incoming_melee->apiobj.position;
                } else if (object->incoming_part != NULL && (object->incoming_part->active & 1) != 0) {
                    position = &object->incoming_part->position;
                }
                if (position != NULL)
                    api.movement_facing_angle = NuAtan2D(position->x - api.position.x, position->z - api.position.z);
                break;
            }
            case 0x0c: {
                if ((context_flags & 0x2000) != 0)
                    goto directional_common_heading;
                if (object->block_latch != 0)
                    break;
                NUVEC *position = NULL;
                if (object->blocked_bolt != NULL) {
                    if (object->blocked_bolt->active != 0)
                        position = &object->blocked_bolt->position;
                } else if (object->block_attacker != NULL) {
                    if ((object->block_attacker->apiobj.field_0x1f8 & 0x1000) != 0 &&
                        object->block_attacker->apiobj.field_0x287 == 0)
                        position = &object->block_attacker->apiobj.position;
                } else if (object->blocked_part != NULL && (object->blocked_part->active & 1) != 0) {
                    position = &object->blocked_part->position;
                }
                if (position != NULL)
                    api.movement_facing_angle = NuAtan2D(position->x - api.position.x, position->z - api.position.z);
                break;
            }
            case 0x10:
                if ((object->context_flags & 0x40) != 0)
                    goto directional_common_heading;
                if (FaceOpponent(object, NULL) != 0)
                    api.movement_facing_angle += 0x8000;
                break;
            case 0x25:
                if ((context_flags & 0x2000) == 0 && object->context_animation != 0x58) {
                    FaceOpponent(object, NULL);
                    break;
                }
                // The original falls through to the shared push/input heading path.
                goto directional_common_heading;
            case 0x22:
                if (ForceBackPos != NULL)
                    FaceOpponent(object, ForceBackPos);
                break;
            case 0x1d:
                if (object->force_part != NULL && (object->force_part->active & 1) != 0)
                    api.movement_facing_angle = NuAtan2D(object->force_part->position.x - api.collision_position.x,
                                                         object->force_part->position.z - api.collision_position.z);
                break;
            case -1:
            case 1:
            case 0x13:
            case 0x17:
            case 0x29:
            directional_common_heading:
                if ((context_flags & 0x2000) != 0) {
                    SetPushAngle(object);
                } else if (object->context_target_position != NULL) {
                    FaceOpponent(object, object->context_target_position);
                    api.movement_facing_angle += 0x8000;
                } else if (pad->operator_data != NULL && (context_flags & 1) == 0) {
                    NUVEC *position = static_cast<NUVEC *>(pad->operator_data);
                    api.movement_facing_angle = NuAtan2D(position->x - api.position.x, position->z - api.position.z);
                } else if (api.field_0x27c != -1 && api.field_0x27c == BonusWinner) {
                    api.movement_facing_angle =
                        NuAtan2D(GameCam->pos.x - api.position.x, GameCam->pos.z - api.position.z);
                } else if (pad->input_magnitude > 0.0f &&
                           ((context_flags & 1) == 0 || SuperCarry_Carrying(object) ||
                            (object->character_context == 0x20 && (object->field_0xef9 & 1) != 0)) &&
                           object->delayed_turn_timer <= 0.0f && object->character_context != 0x13 &&
                           (object->field_0xf01 & 2) == 0 &&
                           (object->character_context != 0 || object->context_variant_flags < 0 ||
                            object->action_movement_state == 0 || object->action_movement_state == 5 ||
                            object->action_movement_state == 8)) {
                    if ((api.flags_low & 0x80) != 0 || object->character_context != -1 || api.field_0x27d != 0 ||
                        IsAFallAnim(CurrentAnim(&api.anim_packet)) == 0)
                        api.movement_facing_angle = input_angle;
                    if (object->character_context == 0x20 && (object->field_0xef9 & 1) != 0)
                        api.movement_facing_angle += 0x8000;
                }
                break;
            case 5:
                if ((object->context_flags & 0x0c) == 0)
                    FaceOpponent(object, NULL);
                break;
            case 0x26:
                if (object->field_0x7a7 == -1 && FaceOpponent(object, NULL) != 0 && (object->context_flags & 0x80) != 0)
                    api.movement_facing_angle += 0x8000;
                break;
            case 0x33:
                if (api.velocity.x != 0.0f || api.velocity.z != 0.0f)
                    api.movement_facing_angle = NuAtan2D(api.velocity.x, api.velocity.z);
                break;
            case 0x1c:
                heading_handled = ForcePushed_YRotation(object);
                break;
            case 0x58:
                heading_handled = SuperCarry_YRotation(object, input_angle);
                break;
            case 0x1b:
                if ((api.flags_low & 0x80) != 0 && (object->field_0xe21 & 2) == 0) {
                    if (object->field_0x7a3 == 1)
                        api.movement_facing_angle = input_angle;
                    else
                        FaceOpponent(object, NULL);
                    turn_override = 0.333f;
                    break;
                }
                // Other actors and the alternate state use ordinary opponent-facing speed.
            case 0x16:
                FaceOpponent(object, NULL);
                break;
            case 8:
            case 0x12:
                if ((api.field_0x1f4 & 0x40000) == 0)
                    api.movement_facing_angle = object->force_heading;
                break;
            case 0x2d:
                if ((api.field_0x1f4 & 0x40000) == 0) {
                    NUVEC centre;
                    GizGetBuildItPlayerPos(object, NULL, &centre);
                    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(object->field_0x788);
                    if ((buildit->state_flags & 0x20) != 0)
                        GizBuildItPushAwayFromStart(object, buildit);
                    api.movement_facing_angle = NuAtan2D(centre.x - api.position.x, centre.z - api.position.z);
                }
                break;
            default:
                goto directional_common_heading;
        }
    }
    const i32 direct_turn = heading_handled != 0 ? 0 : (object->character_context == 0x33 ? 1 : object->snap_facing);
    if (heading_handled == 0) {
        f32 turn_rate;
        if (SuperCarry_Carrying(object))
            turn_rate = 8.0f * object->field_0x768;
        else if (object->character_context == 0x33 || object->character_context == 0x1c)
            turn_rate = 8.0f;
        else if (turn_override != 0.0f)
            turn_rate = 8.0f * turn_override;
        else {
            turn_rate = 8.0f * api.character_data->game_character->turn_rate;
            if (object->id == id_ATAT && WORLD->current_level == SPEEDERCHASEA_LDATA)
                turn_rate += turn_rate;
        }
        if ((object->field_0xefd & 0x80) != 0 && (CInfo[object->character_context].flags & 1) == 0)
            api.movement_facing_angle += 0x8000;
        if ((object->field_0xefd & 4) != 0) {
            api.facing_angle = api.movement_facing_angle;
            api.field_0x276 = api.movement_facing_angle;
        } else if (direct_turn == 0 &&
                   (turn_override != 0.0f || (CInfo[object->character_context].parameter & 2) != 0 ||
                    (api.character_data->game_character->flags_090 & 0x100) != 0 || SuperCarry_Carrying(object))) {
            if (object->character_context == 0x4b && (api.character_data->game_character->flags_090 & 0x100) == 0)
                turn_rate *= 0.333f;
            api.facing_angle =
                TurnRot(api.facing_angle, api.movement_facing_angle, static_cast<i32>(turn_rate * 16384.0f), NULL);
            api.field_0x276 = SeekRot(api.field_0x276, api.facing_angle, 10.0f);
            api.movement_facing_angle = api.facing_angle;
        } else {
            api.facing_angle = SeekRot(api.facing_angle, api.movement_facing_angle, turn_rate);
            api.field_0x276 = api.facing_angle;
        }
    }
    if (ObjInTube(object)) {
        object->target_velocity.y = static_cast<TUBE *>(object->field_0x788)->field_0x24;
        if (WORLD->current_level == CRUISERD_LDATA)
            object->target_velocity.y += object->target_velocity.y;
    } else {
        object->target_velocity.y = 0.0f;
    }
    i32 move_vertical = 0;
    bool separate_seek_rates = false;
    NUVEC seek_rates;
    seek_rates.x = api.character_data->game_character->velocity_seek_rate;
    if (api.movement_direction.x != 0.0f || api.movement_direction.z != 0.0f) {
        object->target_velocity.x = api.movement_direction.x;
        object->target_velocity.z = api.movement_direction.z;
        api.movement_direction.z = 0.0f;
        api.movement_direction.x = 0.0f;
        object->field_0xe23 |= 0x10;
    } else {
        do {
            if (object->character_context == 0x57) {
                api.position = api.start_position;
                api.collision_position = api.initial_position;
                api.velocity = v000;
                break;
            } else if (object->character_context == 0x45) {
                move_vertical = WallShuffle_SetTargetMom(object, input_angle);
                break;
            } else if (object->character_context == 0x55) {
                move_vertical = LedgeTerrain_SetTargetMom(object);
                break;
            } else if (object->character_context == 0x5b) {
                object->target_velocity.x = (object->launch_origin.x - api.upper_position.x) * 20.0f;
                object->target_velocity.y = (object->launch_origin.y - api.upper_position.y) * 20.0f;
                object->target_velocity.z = (object->launch_origin.z - api.upper_position.z) * 20.0f;
                move_vertical = 1;
                break;
            } else if (object->character_context == 0x27 || object->character_context == 0x28 ||
                       object->character_context == 0x59) {
                {
                    const u16 angle = api.movement_facing_angle;
                    object->target_velocity.x = NuTrigTable[angle >> 1];
                    api.velocity.x = object->target_velocity.x;
                    object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff];
                    api.velocity.z = object->target_velocity.z;
                    break;
                }
            } else if (object->character_context == 0x4e) {
                move_vertical = Hang_SetTargetMom(object);
                break;
            } else if (object->character_context == 0x43) {
                move_vertical = Climb_SetTargetMom(object, input_angle);
                break;
            } else if (object->character_context == 0x46) {
                Grapple_SetRotOrder(object);
                move_vertical = Grapple_SetTargetMom(object);
                break;
            } else if (object->character_context == 0x4a) {
                if ((api.field_0x1f4 & 0x40000) == 0) {
                    const NUVEC position = static_cast<LEVER_s *>(object->field_0x788)->floor_position;
                    const f32 rate = 10.0f * object->field_0x768 + 5.0f;
                    object->target_velocity.x = (position.x - api.position.x) * rate;
                    object->target_velocity.z = (position.z - api.position.z) * rate;
                }
                break;
            } else if (object->character_context == 0x0b) {
                if ((api.field_0x1f4 & 0x40000) == 0) {
                    NUVEC position;
                    GizPanel_GetAbsTargetPos(static_cast<GIZPANEL_s *>(object->field_0x788), &position, 1);
                    const f32 rate = 10.0f * object->field_0x768 + 5.0f;
                    object->target_velocity.x = (position.x - api.position.x) * rate;
                    object->target_velocity.z = (position.z - api.position.z) * rate;
                }
                break;
            } else if (object->character_context == 0x51) {
                {
                    TECHNO_s *techno = static_cast<TECHNO_s *>(object->field_0x788);
                    object->target_velocity.x = (techno->ground_position.x - api.position.x) * 5.0f;
                    object->target_velocity.z = (techno->ground_position.z - api.position.z) * 5.0f;
                    break;
                }
            } else if (object->character_context == 0x53) {
                {
                    ATTRACTO_s *attracto = static_cast<ATTRACTO_s *>(object->field_0x788);
                    object->target_velocity.x = (attracto->active_position.x - api.position.x) * 5.0f;
                    object->target_velocity.z = (attracto->active_position.z - api.position.z) * 5.0f;
                    break;
                }
            } else if (object->character_context == 0x5c) {
                {
                    SECURITYDOOR *door = static_cast<SECURITYDOOR *>(object->field_0x788);
                    object->target_velocity.x = (door->player_position.x - api.position.x) * 5.0f;
                    object->target_velocity.z = (door->player_position.z - api.position.z) * 5.0f;
                    break;
                }
            } else if (object->character_context == 0x4c) {
                {
                    SIGNAL_s *signal = static_cast<SIGNAL_s *>(object->field_0x788);
                    object->target_velocity.x = (signal->target_position.x - api.collision_position.x) * 5.0f;
                    object->target_velocity.y = (signal->target_position.y - api.collision_position.y) * 5.0f;
                    object->target_velocity.z = (signal->target_position.z - api.collision_position.z) * 5.0f;
                    move_vertical = 1;
                    break;
                }
            } else if (object->character_context == 0x44) {
                move_vertical = TightRope_SetTargetMom(object);
                break;
            } else if (object->character_context == 0x4f) {
                move_vertical = Glide_SetTargetMom(object);
                seek_rates.y = seek_rates.x;
                seek_rates.x = seek_rates.z = 2.0f;
                separate_seek_rates = true;
                break;
            } else {
                {
                    if (object->character_context == 0x61 && (api.field_0x1f4 & 0x40000) == 0) {
                        const NUVEC position = static_cast<HATMACHINE_s *>(object->field_0x788)->player_position;
                        const f32 rate = 10.0f * object->field_0x768 + 5.0f;
                        object->target_velocity.x = (position.x - api.position.x) * rate;
                        object->target_velocity.z = (position.z - api.position.z) * rate;
                        break;
                    }
                    const bool animation_movement = object->character_context != 5;
                    if (animation_movement) {
                        const f32 animation_speed = AnimSpeed(api.character_model, current_animation);
                        if (animation_speed != 0.0f) {
                            if (CanStepBack(object) && StepBackFromTarget(object))
                                break;
                            const f32 speed_multiplier = object->animation_speed_multiplier;
                            const f32 stop_frame = AnimStopFrame(api.character_model, current_animation);
                            f32 *frame = AnimPlaying(&api.anim_packet, current_animation, 1, 0);
                            bool stopped = stop_frame > 1.0f && frame != NULL && *frame >= stop_frame;
                            if (!stopped && frame != NULL && api.field_0x27d != 0 &&
                                api.character_model->model_data_b[current_animation] != NULL &&
                                (static_cast<CHARACTERANIM_s *>(api.character_model->model_data_a[current_animation])
                                     ->flags &
                                 2) == 0 &&
                                (api.anim_packet.flags & ANIMPACKET_FLAG_FINISHED) != 0)
                                stopped = true;
                            if (stopped) {
                                object->target_velocity.x = 0.0f;
                                object->target_velocity.z = 0.0f;
                                const i32 surface = static_cast<i8>(api.field_0x281);
                                if (!(api.field_0x27d != 0 && surface >= 0 && surface < 32 &&
                                      TerSurface[surface].movement_scale < 1.0f)) {
                                    api.velocity.x = 0.0f;
                                    api.velocity.z = 0.0f;
                                }
                            } else {
                                const u16 angle =
                                    (api.character_data->game_character->flags_090 & 0x100) != 0 && direct_turn == 0
                                        ? api.facing_angle
                                        : api.movement_facing_angle;
                                const f32 speed = animation_speed * speed_multiplier;
                                object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                if (object->character_context == 0x25 && object->context_animation != 0x58)
                                    NuVecRotateY(&object->target_velocity, &object->target_velocity, 0x4000);
                            }
                            break;
                        }
                        // Original zero-animation-speed dispatch, 0x1690d8.
                        if (object->character_context == 0x26 && object->field_0x7a7 != -1 &&
                            object->force_target != NULL) {
                            SpecialMove_Attacker_SetTargetMom(object);
                            break;
                        }
                        if (object->character_context == 0x33) {
                            move_vertical = Slide_SetTargetMom(object, input_angle, desired_speed);
                            break;
                        }
                        if (object->character_context == 0x58) {
                            move_vertical = SuperCarry_SetTargetMom(object, desired_speed);
                            break;
                        }
                        if (object->character_context == 0x5a) {
                            if (object->field_0x7a3 == 0 && (api.field_0x1f8 & 2) == 0) {
                                const u16 angle = api.movement_facing_angle;
                                object->target_velocity.x = -NuTrigTable[angle >> 1] * object->external_force.z;
                                object->target_velocity.z =
                                    -NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * object->external_force.z;
                            } else {
                                object->target_velocity.x = 0.0f;
                                object->target_velocity.z = 0.0f;
                            }
                            break;
                        }
                    }
                    if (animation_movement || object->character_context == 5) {
                        if ((CInfo[object->character_context].flags & 2) != 0) {
                            object->target_velocity.x = 0.0f;
                            object->target_velocity.z = 0.0f;
                            break;
                        }
                        if (object->character_context == 0x5f) {
                            object->target_velocity.x = object->external_force.x;
                            object->target_velocity.z = object->external_force.z;
                            break;
                        }
                        switch (object->character_context) {
                            case 0x21:
                            case 0x22: {
                                const u16 angle = api.movement_facing_angle;
                                object->target_velocity.x = -NuTrigTable[angle >> 1] * 1.2f;
                                object->target_velocity.z = -NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * 1.2f;
                                break;
                            }
                            case 0x18:
                            case 0x0c:
                                if (object->incoming_bolt != NULL && desired_speed > 0.0f &&
                                    (pad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
                                    (object->character_context == 0x18 || object->block_latch == 0)) {
                                    object->target_velocity.x =
                                        NuTrigTable[input_angle >> 1] * api.character_data->game_character->run_speed;
                                    object->target_velocity.z = NuTrigTable[((input_angle + 0x4000) >> 1) & 0x7fff] *
                                                                api.character_data->game_character->run_speed;
                                } else {
                                    object->target_velocity.x = 0.0f;
                                    object->target_velocity.z = 0.0f;
                                }
                                break;
                            case 0x41:
                            case 0x17:
                            case 0x1f:
                            case 0x3c:
                            case 0x35:
                                object->target_velocity.x = 0.0f;
                                object->target_velocity.z = 0.0f;
                                break;
                            case 0x29:
                                // Original 0x169c21: walking is allowed only when this
                                // model supplies animation 0x57.
                                if (api.character_model->model_data_b[0x57] != NULL)
                                    goto directional_walking;
                                object->target_velocity.x = 0.0f;
                                object->target_velocity.z = 0.0f;
                                break;
                            case 0x2d: {
                                NUVEC position = api.position;
                                GizGetBuildItPlayerPos(object, &position, NULL);
                                object->target_velocity.x = (position.x - api.position.x) * 3.0f;
                                object->target_velocity.z = (position.z - api.position.z) * 3.0f;
                                break;
                            }
                            case 0x1c:
                                move_vertical = ForcePushed_SetTargetMom(object, &seek_rates.x);
                                break;
                            case 0x0f:
                                break;
                            case 0x49:
                                object->target_velocity.x = (object->launch_origin.x - api.position.x) * 8.0f;
                                object->target_velocity.z = (object->launch_origin.z - api.position.z) * 8.0f;
                                break;
                            case 0x47: {
                                if (object->action_movement_state == 1) {
                                    object->target_velocity = v000;
                                    api.velocity = object->target_velocity;
                                } else {
                                    ZIPUP *zipup = static_cast<ZIPUP *>(object->field_0x788);
                                    if ((zipup->flags & 1) != 0) {
                                        SeekVec(&api.position, &api.position, &zipup->rider_target_position, 8.0f);
                                    } else {
                                        NuVecSub(&object->target_velocity, &zipup->hook_origin, &api.position);
                                        NuVecNorm(&object->target_velocity, &object->target_velocity);
                                        NuVecScale(&object->target_velocity, &object->target_velocity, 2.0f);
                                    }
                                }
                                move_vertical = 1;
                                break;
                            }
                            case 5:
                                if ((object->context_flags & 0x1c) != 0) {
                                    object->target_velocity.x = 0.0f;
                                    api.velocity.x = 0.0f;
                                    object->target_velocity.z = 0.0f;
                                    api.velocity.z = 0.0f;
                                } else if (StepBackFromTarget(object) == 0) {
                                    const f32 speed = AnimSpeed(api.character_model, object->context_animation);
                                    const u16 angle = api.movement_facing_angle;
                                    object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                    object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                }
                                break;
                            case 0x15: {
                                const f32 speed = AnimSpeed(api.character_model, object->context_animation);
                                const u16 angle = api.movement_facing_angle;
                                object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                break;
                            }
                            case 0x10: {
                                const f32 speed = object->context_animation == 0x29
                                                      ? AnimSpeed(api.character_model, 0x29)
                                                      : api.character_data->game_character->run_speed;
                                const u16 angle = api.movement_facing_angle;
                                object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                break;
                            }
                            case 0x31: {
                                if ((api.field_0x1f8 & 0x80) == 0)
                                    goto directional_walking;
                                const u16 angle = api.movement_facing_angle;
                                object->target_velocity.x =
                                    NuTrigTable[angle >> 1] * api.character_data->game_character->run_speed;
                                object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] *
                                                            api.character_data->game_character->run_speed;
                                break;
                            }
                            case 1:
                            case 0x19:
                            case 2:
                            case 3:
                            case 4:
                            case 0x0d:
                            case 0x0e:
                            case 9:
                            case 0x0a:
                            case 0x16:
                            case 0x1d:
                            case 0x1b:
                            case 0x12:
                            case 8:
                            case 0x13:
                            case 0x14: {
                            directional_action_momentum:
                                // Original 0x169a49: these contexts only move through
                                // their animation when CInfo permits animation momentum.
                                if ((CInfo[object->character_context].flags & 0x10) != 0) {
                                    const f32 speed = AnimSpeed(api.character_model, object->context_animation);
                                    const u16 angle = api.movement_facing_angle;
                                    object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                    object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                } else {
                                    object->target_velocity.x = 0.0f;
                                    object->target_velocity.z = 0.0f;
                                }
                                break;
                            }
                            case 0:
                                if (object->action_movement_state == 3) {
                                    if (StepBackFromTarget(object) == 0) {
                                        const u16 angle = api.movement_facing_angle;
                                        object->target_velocity.x = NuTrigTable[angle >> 1] * object->field_0x768;
                                        object->target_velocity.z =
                                            NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * object->field_0x768;
                                    }
                                    break;
                                }
                                if (object->action_movement_state == 4)
                                    goto directional_action_momentum;
                                if (object->action_movement_state == 2) {
                                    f32 speed = api.character_data->game_character->run_speed;
                                    if ((object->context_variant_flags & 0x20) != 0)
                                        speed = -speed;
                                    const u16 angle = api.movement_facing_angle;
                                    object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                    object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                    break;
                                }
                                if (object->context_variant_flags >= 0 &&
                                    (object->action_movement_state == 6 || object->action_movement_state == 7 ||
                                     object->action_movement_state == 9)) {
                                    f32 speed = object->airborne_action_timer;
                                    if (object->action_movement_state == 6 &&
                                        object->context_animation_timer >=
                                            api.character_data->game_character->jump_duration) {
                                        api.velocity.x = 0.0f;
                                        api.velocity.z = 0.0f;
                                        speed = 0.0f;
                                    }
                                    const u16 angle = api.movement_facing_angle;
                                    object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
                                    object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
                                    break;
                                }
                                goto directional_walking;
                            default:
                                goto directional_walking;
                        }
                        break;
                    }
                directional_walking:
                    if ((pad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) != 0) {
                        object->target_velocity.x = 0.0f;
                        object->target_velocity.z = 0.0f;
                        break;
                    }
                    const bool walking_input = pad->input_magnitude > 0.0f;
                    bool walking_without_input = false;
                    if (!walking_input) {
                        if (object->id == id_ATST || object->id == id_MINIATST || object->id == id_ATST_LOWRES ||
                            object->id == id_ATAT || object->id == id_MINIATAT || object->id == id_MINIATTE)
                            walking_without_input = CurrentAnim(&api.anim_packet) == 0;
                        if (!walking_without_input && object->id == id_RANCOR)
                            walking_without_input =
                                CurrentAnim(&api.anim_packet) == 0 || CurrentAnim(&api.anim_packet) == 0x40;
                    }
                    const bool calculate_walking_speed = walking_input || walking_without_input;
                    if (!calculate_walking_speed && api.anim_packet.blending != 0) {
                        i32 animation = api.anim_packet.blend_animation_a;
                        const f32 speed = AnimSpeed(api.character_model, animation);
                        if (speed != 0.0f) {
                            animation = api.anim_packet.blend_animation_a;
                            CHARACTERANIM_s *info =
                                static_cast<CHARACTERANIM_s *>(api.character_model->model_data_a[animation]);
                            bool animation_active = (info->flags & 2) != 0;
                            if (!animation_active) {
                                const f32 source_time = api.anim_packet.blend_source_time;
                                animation_active =
                                    NuAnimEndFrame(api.character_model->model_data_b[animation]) > source_time;
                            }
                            if (animation_active) {
                                animation = api.anim_packet.blend_animation_a;
                                bool stopped = static_cast<u16>(animation - 0x2e) <= 0x0d;
                                if (!stopped) {
                                    const f32 stop_frame = AnimStopFrame(api.character_model, animation);
                                    stopped = stop_frame > 0.0f && api.anim_packet.blend_source_time >= stop_frame;
                                }
                                if (stopped) {
                                    object->target_velocity.x = 0.0f;
                                    object->target_velocity.z = 0.0f;
                                    const i32 surface = static_cast<i8>(api.field_0x281);
                                    if (!(api.field_0x27d != 0 && surface >= 0 && surface < 32 &&
                                          TerSurface[surface].movement_scale < 1.0f)) {
                                        api.velocity.x = 0.0f;
                                        api.velocity.z = 0.0f;
                                    }
                                } else {
                                    const u16 angle =
                                        direct_turn == 0 &&
                                                ((CInfo[object->character_context].parameter & 2) != 0 ||
                                                 (api.character_data->game_character->flags_090 & 0x100) != 0)
                                            ? api.facing_angle
                                            : api.movement_facing_angle;
                                    object->target_velocity.x = speed * NuTrigTable[angle >> 1];
                                    object->target_velocity.z = speed * NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff];
                                    if (api.anim_packet.blend_animation_a == 0x4f ||
                                        api.anim_packet.blend_animation_a == 0x26)
                                        NuVecRotateY(&object->target_velocity, &object->target_velocity, 0x4000);
                                }
                                break;
                            }
                        }
                    }
                    if (!calculate_walking_speed) {
                        if (object->touch_task == NULL ||
                            object->touch_task->GetHashId().value != MechTouchTaskJump::HashId.value) {
                            object->target_velocity.x = 0.0f;
                            object->target_velocity.z = 0.0f;
                        }
                        break;
                    }
                    f32 water_speed_multiplier = 1.0f;
                    if (calculate_walking_speed) {
                        if (object->character_context == 0x4b) {
                            water_speed_multiplier = 0.7f;
                        } else if (api.intersects_water != 0 && object->id != id_YODA && object->id != id_YODAGHOST) {
                            const f32 depth = (api.water_height - api.collision_min.y) / api.field_0x1e0;
                            if (depth < 0.0f)
                                water_speed_multiplier = 0.0f;
                            else if (depth > 0.5f)
                                water_speed_multiplier = 0.7f;
                            else
                                water_speed_multiplier = 1.0f - (depth + depth) * 0.3f;
                        }
                    }
                    f32 walking_speed = desired_speed;
                    if ((object->field_0xeff & 2) == 0 && IsAFallAnim(CurrentAnim(&api.anim_packet)) != 0) {
                        walking_speed = 0.0f;
                    } else if (object->id == id_YODA || object->id == id_YODAGHOST) {
                        if (object->character_context == 0 && object->action_movement_state == 0) {
                            walking_speed = 1.2f;
                        } else if (pad->input_magnitude > 0.0f) {
                            walking_speed = api.character_data->game_character->run_speed;
                        } else if (AnimPlaying(&api.anim_packet, 5, 1, 0) != NULL) {
                            GAMECHARACTERDATA *character = api.character_data->game_character;
                            walking_speed = (0.7f * character->run_speed) / 3.0f;
                            const f32 magnitude = object->pad_gamepad->input_magnitude;
                            if (!((character->tiptoe_speed + character->walk_speed) * 0.5f >= magnitude)) {
                                if (!((character->walk_speed + character->run_speed) * 0.5f >= magnitude))
                                    walking_speed *= 3.0f;
                                else
                                    walking_speed += walking_speed;
                            }
                        }
                    } else {
                        GAMECHARACTERDATA *walking_character = api.character_data->game_character;
                        if ((walking_character->flags_094[3] & 0x10) != 0) {
                            if (object->field_0xe31 == 1) {
                                walking_speed = walking_character->run_speed / 3.0f;
                                const f32 magnitude = object->pad_gamepad->input_magnitude;
                                if (!((walking_character->tiptoe_speed + walking_character->walk_speed) * 0.5f >=
                                      magnitude)) {
                                    if (!((walking_character->walk_speed + walking_character->run_speed) * 0.5f >=
                                          magnitude))
                                        walking_speed *= 3.0f;
                                    else
                                        walking_speed += walking_speed;
                                }
                            } else {
                                walking_speed = walking_character->walk_speed;
                            }
                        } else {
                            if (object->id == id_GONKDROID) {
                                if (Cheat_IsOn(8) == 0)
                                    walking_speed = CurrentAnim(&api.anim_packet) == 5
                                                        ? 0.5f * api.character_data->game_character->run_speed
                                                        : api.character_data->game_character->walk_speed;
                            } else if (object->id == id_C3PO || object->id == id_TC14) {
                                if (object->character_context == -1 && object->field_0xdb0 > 0.0f) {
                                    walking_speed = (0.6f * walking_character->run_speed) / 3.0f;
                                    const f32 magnitude = object->pad_gamepad->input_magnitude;
                                    if (!((walking_character->tiptoe_speed + walking_character->walk_speed) * 0.5f >=
                                          magnitude)) {
                                        if (!((walking_character->walk_speed + walking_character->run_speed) * 0.5f >=
                                              magnitude))
                                            walking_speed *= 3.0f;
                                        else
                                            walking_speed += walking_speed;
                                    }
                                }
                            } else if (object->id == id_DROIDEKA) {
                                if (api.anim_packet.field_0x3a == 5)
                                    walking_speed *= 0.5f;
                            } else if (object->id == id_ATST || object->id == id_MINIATST ||
                                       object->id == id_ATST_LOWRES || object->id == id_ATAT ||
                                       object->id == id_MINIATAT || object->id == id_MINIATTE ||
                                       object->id == id_RANCOR) {
                                walking_speed = walking_character->run_speed;
                            }
                        }
                    }
                    if (object->character_context == 0 && object->action_movement_state == 0)
                        walking_speed *= api.character_data->game_character->field_0x84;
                    if ((object->field_0xefd & GAMEOBJECT_MOVEMENT_FLAG_BACKWARDS) != 0)
                        walking_speed *= api.character_data->game_character->backwards_speed_multiplier;
                    u16 walking_angle = input_angle;
                    if (direct_turn == 0 && ((CInfo[object->character_context].parameter & 2) != 0 ||
                                             (api.character_data->game_character->flags_090 & 0x100) != 0))
                        walking_angle = api.facing_angle;
                    if (WORLD->current_level == VADERA_LDATA && (api.field_0x1f8 & 0x80) == 0 &&
                        GameCam->sock_position.location.sock == 0)
                        walking_speed *= 1.0416666269302368f;
                    object->target_velocity.x =
                        NuTrigTable[walking_angle >> 1] * walking_speed * water_speed_multiplier;
                    object->target_velocity.z =
                        NuTrigTable[((walking_angle + 0x4000) >> 1) & 0x7fff] * walking_speed * water_speed_multiplier;
                    break;
                }
            }
        } while (false);
    }
    if ((object->field_0xefd & GAMEOBJECT_MOVEMENT_FLAG_REVERSE_VELOCITY) != 0) {
        NuVecRotateY(&object->target_velocity, &object->target_velocity, 0x8000);
    } else if ((api.field_0x1f8 & 0x80) != 0 && (api.character_data->game_character->flags_090 & 0x100) != 0)
        CharPivot_Check(object, &object->target_velocity);

    if (object->character_context == 0x4b) {
        const f32 float_height = api.collision_min.y + (api.collision_max.y - api.collision_min.y) * 0.625f;
        object->target_velocity.y = (api.water_height - float_height) * 3.0f;
        if (object->suit != NULL && (static_cast<SUIT_s *>(object->suit)->store_flag & 0x10) != 0) {
            if ((object->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) == 0) {
                object->target_velocity.y = 0.0f;
            } else {
                if (api.water_height > api.upper_position.y)
                    object->target_velocity.y = 0.6f;
                move_vertical = 1;
            }
        } else {
            move_vertical = 1;
        }
        seek_rates.x = seek_rates.z = 1.0f;
        seek_rates.y = api.character_data->game_character->velocity_seek_rate;
    } else if (!separate_seek_rates) {
        seek_rates.y =
            move_vertical != 0 ? seek_rates.x : object->apiobj.character_data->game_character->velocity_seek_rate;
        seek_rates.z = seek_rates.x;
    }

    if (api.field_0x27d != 0 && static_cast<i8>(api.field_0x281) != -1) {
        f32 scale = TerSurface[static_cast<i8>(api.field_0x281)].movement_scale;
        if (scale != 1.0f) {
            seek_rates.x *= scale;
            seek_rates.z *= scale;
        }
    }
    if (object->character_context == 0x2b) {
        if (object->turn_braking < 1.0f) {
            const f32 scale = 1.0f - object->turn_braking;
            object->target_velocity.x *= scale;
            object->target_velocity.z *= scale;
            if (object->field_0xe36 == 2 || object->field_0xe36 == 4) {
                object->target_velocity.x *= 0.5f;
                object->target_velocity.z *= 0.5f;
            }
        } else {
            object->target_velocity.x = 0.0f;
            object->target_velocity.z = 0.0f;
        }
        if (object->field_0xe36 == 2 || object->field_0xe36 == 4) {
            NewRumble(pad->pad, qrand() * 1.5259022e-05f * 0.25f, 0);
            const i32 effect = WORLD->debris_sys->entries[58].effect;
            if (effect != -1) {
                const i32 count = ParticlesPerSecond(60.0f, FRAMETIME);
                if (count > 0) {
                    NUVEC position = {api.collision_position.x, api.water_height, api.collision_position.z};
                    AddVariableShotDebrisEffect(effect, &position, count, 0, 0);
                }
            }
        }
    }
    if ((object->field_0xe23 & 0x10) == 0 && api.field_0x27d != 0 && static_cast<i8>(api.field_0x281) != -1 &&
        (TerSurface[static_cast<i8>(api.field_0x281)].flags & 0x100) != 0)
        Conveyor_AdjustSpeed(&object->target_velocity);
    if (WORLD->current_level == CRUISERB_LDATA && Mission_Active(NULL) != NULL) {
        NUVEC position = {11.25f, 1.466f, -25.445f};
        if (NuVecDistSqr(&api.collision_position, &position, NULL) < 0.4f * 0.4f &&
            api.position.z > position.z - 0.1f) {
            object->target_velocity.z = -2.0f;
        } else {
            position.x = 13.765f;
            if (NuVecDistSqr(&api.collision_position, &position, NULL) < 0.4f * 0.4f &&
                api.position.z > position.z - 0.1f)
                object->target_velocity.z = -2.0f;
        }
    }
    const bool seek_vertical = move_vertical != 0 || ObjInTube(object);
    // Original 0x167ac2: the context payload is shared by several gizmo
    // types; the vertical path tests its byte at +0x68 without a type gate.
    if (seek_vertical && object->field_0x788 != NULL && (static_cast<const u8 *>(object->field_0x788)[0x68] & 1) != 0) {
        seek_rates.x *= 0.5f;
        seek_rates.y *= 0.5f;
        seek_rates.z *= 0.5f;
    }
    if (!seek_vertical) {
        if ((object->field_0xe20 & 8) != 0 && api.field_0x27d != 0) {
            const f32 current_speed = NuVecMag(&api.velocity);
            const f32 target_speed = NuVecMag(&object->target_velocity);
            f32 multiplier = 5.0f;
            if (target_speed > 0.0f) {
                f32 ratio = current_speed / target_speed;
                if (ratio > 1.0f)
                    ratio = 1.0f - (ratio - 1.0f);
                if (ratio < 0.7f)
                    multiplier = 5.0f;
                else if (ratio < 0.8f)
                    multiplier = 4.0f;
                else if (ratio < 0.9f)
                    multiplier = 3.0f;
                else if (ratio < 1.0f)
                    multiplier = 2.0f - (ratio - 0.9f) / 0.1f;
                else
                    multiplier = 1.0f;
            }
            object->field_0xd78 = SeekLinearF(object->field_0xd78, multiplier, 5.0f * FRAMETIME);
            if (object->field_0xd78 > 1.0f) {
                seek_rates.x *= object->field_0xd78;
                seek_rates.z *= object->field_0xd78;
            }
        } else {
            object->field_0xd78 = 1.0f;
        }
    }
    if (!seek_vertical && object->character_context == 0x26) {
        api.velocity.x = object->target_velocity.x;
        api.velocity.z = object->target_velocity.z;
    } else {
        api.velocity.x = SeekValF(api.velocity.x, object->target_velocity.x, seek_rates.x);
        if (seek_vertical)
            api.velocity.y = SeekValF(api.velocity.y, object->target_velocity.y, seek_rates.y);
        api.velocity.z = SeekValF(api.velocity.z, object->target_velocity.z, seek_rates.z);
    }
    game_character = api.character_data->game_character;
    if ((game_character->flags_090 & 1) != 0) {
        i32 rate =
            static_cast<i32>(static_cast<f32>(RotDiff(object->previous_movement_angle, api.field_0x276)) / FRAMETIME);
        i32 lean;
        if (rate < -0x10000)
            lean = -0x2000;
        else if (rate > 0x10000)
            lean = 0x2000;
        else {
            lean = rate / 4;
            if (lean < -0x2000)
                lean = -0x2000;
            if (lean > 0x2000)
                lean = 0x2000;
        }
        object->movement_lean_angle = SeekRot(object->movement_lean_angle, static_cast<u16>(lean), 8.0f);
    } else if (object->character_context == 0x4b) {
        i32 lean = 0;
        if (object->pad_gamepad->input_magnitude > 0.0f) {
            lean = static_cast<i32>(static_cast<f32>(RotDiff(object->previous_movement_angle, api.field_0x276)) /
                                    FRAMETIME) /
                   15;
            if (lean > 0x2000)
                lean = 0x2000;
            if (lean < -0x2000)
                lean = -0x2000;
        }
        object->movement_lean_angle = SeekRot(object->movement_lean_angle, static_cast<u16>(lean), 8.0f);
    } else {
        object->movement_lean_angle = 0;
    }
    u16 secondary_lean = 0;
    if (object->character_context == 0x3d && object->context_animation != 0x85)
        secondary_lean = object->field_0x7a3 == 0 ? 0xce39 : 0x3c71;
    else if (object->character_context == 0x2f && object->context_animation == 5)
        secondary_lean = 0xc000;
    object->secondary_lean_angle = SeekRot(object->secondary_lean_angle, secondary_lean, 5.0f);
    GizmoBlowupCheckProximity(WORLD, object);
}

#include <stdlib.h>
extern "C" i16 id_LANDSPEEDER, id_WOOKIEFLYER, id_STAP2;
extern AREADATA_s *SPEEDERCHASE_ADATA;
extern GameObject_s *GetOtherActivePlayer(GameObject_s *);
extern GameObject_s *CarWashHack;
extern i32 IDLESPEEDINNARROWSOCKSONLY;
extern f32 GetVehicleSpeedMul(GameObject_s *, f32);
extern i32 GoingForwardsAlongNarrowSock(GameObject_s *);
extern f32 PodSprint_InStartCountdown(WORLDINFO_s *);
extern f32 DeathStar2BattleFire_GetSlowDownMul(GameObject_s *);
extern i32 OutSideSplineArea(NUVEC *, nugspline_s *, NUVEC *, NUVEC *, i32);
extern void VehicleCollisionCode(GameObject_s *);

i32 NeedsPretendAnim(GameObject_s *object) {
    return object->apiobj.character_model->model_data_b[object->apiobj.anim_packet.requested_animation] == NULL ||
           object->id == id_JEDISTARFIGHTERREDEP3 || object->id == id_JEDISTARFIGHTERYELLOWEP3 ||
           object->id == id_TIEINTERCEPTOR;
}

void MovePlayer_VEHICLEDIRECTIONAL(GameObject_s *object) {
    APIOBJECT_s &api = object->apiobj;
    GAMEPAD_s *pad = object->pad_gamepad;
    GameObject_s *operator_object = object;
    GameObject_s *other;
    NUVEC carwash_delta, a, b, direction, separation;
    if (CarWashHack == NULL && object->id == id_LANDSPEEDER && WORLD->current_level == MOSEISLEYB_LDATA &&
        NuSpecialExistsFn(&LevHSpecial[0]) &&
        NuVecXZDistSqr(NuSpecialGetDrawPos(&LevHSpecial[0]), &api.collision_position, &carwash_delta) < 0.36f) {
        CarWashHack = object;
        if (pad->input_magnitude > (((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->walk_speed +
                                    ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed) *
                                       0.5f &&
            (LevGizObst[0] == NULL || LevGizObst[0]->anim_set == NULL || (LevGizObst[0]->anim_set->flags & 1) == 0))
            CarWashHack = NULL;
    }
    if (object->id == id_WOOKIEFLYER && object->character_context == 0x17)
        ApplyGravity(object, NULL, 0.0f, 10.0f, NULL);
    object->field_0xe23 &= ~0x10;
    object->in_narrow_socket = ObjInNarrowSock(object, WORLD->sock_sys, WORLD->level_idx);
    u16 narrow_yaw = object->in_narrow_socket ? object->yrot : 0;
    if (object->character_context != 0x2a) {
        object->field_0xe24 &= ~2;
        object->previous_block_animation = -1;
        if (object->delayed_turn_timer > 0.0f && object->character_context != 0x36 && object->character_context != 0x3a)
            object->delayed_turn_timer -= FRAMETIME;
    } else
        object->previous_block_animation = -1;
    i32 in_tube = 0;
    if (PODSPRINT_ADATA != NULL && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA) &&
        (api.flags_low & 0x80) != 0 && Tube_InAnyCylinder(WORLD, object, 1)) {
        NewRumble(object->pad_gamepad->pad, qrand() * (1.0f / 65535.0f), 0);
        in_tube = 1;
    }
    Techno_FindOperator(object, &pad, &operator_object);
    f32 requested_speed;
    if (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
        requested_speed = pad->input_magnitude /
                          ((GAMECHARACTERDATA_s *)operator_object->apiobj.character_data->field11_0x24)->run_speed *
                          35.0f;
    else
        requested_speed = pad->input_magnitude /
                          ((GAMECHARACTERDATA_s *)operator_object->apiobj.character_data->field11_0x24)->run_speed *
                          ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed;
    if ((object->field_0xe20 & 0x20) != 0 && object->character_context != 0x23 && object->character_context != 0x24) {
        MoveInactiveVehicle(object, 0, &other);
        if (other != NULL) {
            api.field_0x276 = api.facing_angle = api.movement_facing_angle = other->apiobj.field_0x276;
            api.velocity = other->apiobj.velocity;
            object->field_0xdc8 = other->field_0xdc8;
            object->movement_lean_angle = object->secondary_lean_angle = object->tertiary_lean_angle = 0;
        }
        if (WORLD->current_level == PLATFORM_LDATA)
            return;
        goto vehicle_collision;
    }
    {
        u16 input_yaw = GamePad_InputAngle(object, pad);
        object->field_0xe22 |= 0x20;
        object->current_input_angle = input_yaw;
        object->target_velocity.y = 0.0f;
        if (object->field_0xddc > 0.0f)
            object->field_0xddc -= FRAMETIME;
        f32 turn_multiplier = 0.0f;
        if ((object->field_0xefd & 4) != 0) {
            api.facing_angle = api.movement_facing_angle = api.field_0x276 = input_yaw;
        } else {
            f32 heading_seek_rate = 10.0f;
            if (CarWashHack == object) {
                api.movement_facing_angle = 0x4000;
                heading_seek_rate = 3.0f;
            } else if (requested_speed > 0.0f && object->character_context != 0x2a &&
                       object->character_context != 0x36 && object->character_context != 0x3a &&
                       object->character_context != 0x23 && object->character_context != 0x24 &&
                       (object->character_context != 0x17 || (object->jump_input_flags & 1) != 0) &&
                       !AnimPlaying(&api.anim_packet, 12, 1, 1) && !AnimPlaying(&api.anim_packet, 6, 1, 1)) {
                if (object->in_narrow_socket &&
                    (((api.flags_low & 0x80) != 0 && WORLD->current_level == DEATHSTARBATTLED_LDATA &&
                      ObjInNarrowSock(object, WORLD->sock_sys, WORLD->level_idx)) ||
                     WORLD->current_level == DEATHSTAR2BATTLEE_LDATA ||
                     WORLD->current_level == DEATHSTAR2BATTLEF_LDATA ||
                     WORLD->current_level == DEATHSTAR2BATTLEG_LDATA))
                    turn_multiplier = 0.5f;
                else if (WORLD->current_level == SPEEDERCHASEA_LDATA && !disable_narrow_socks)
                    turn_multiplier = 0.75f;
                else if ((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA))
                    turn_multiplier = 0.25f;
                f32 ratio = (NuFsqrt(api.velocity.x * api.velocity.x + api.velocity.z * api.velocity.z) -
                             ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->tiptoe_speed) /
                            (((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed -
                             ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->tiptoe_speed);
                if (ratio < 0.0f)
                    ratio = 0.0f;
                if (ratio > 1.0f)
                    ratio = 1.0f;
                f32 rate = (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                               ? 0.0f * ratio + 1.0f
                               : ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->turn_rate +
                                     (((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->field_0x78 -
                                      ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->turn_rate) *
                                         ratio;
                if (object->in_narrow_socket) {
                    rate *= 1.0f - 0.25f * object->field_0xdc8;
                    if (turn_multiplier != 0.0f &&
                        !((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)))
                        rate *= turn_multiplier;
                }
                api.movement_facing_angle = TurnRot(api.movement_facing_angle, input_yaw, (i32)(rate * 65536.0f), NULL);
                if (object->in_narrow_socket) {
                    i32 delta = RotDiff(narrow_yaw, api.movement_facing_angle);
                    i32 degrees = (i32)(60.0f - 30.0f * object->field_0xdc8);
                    if (degrees < 0)
                        degrees = 0;
                    if ((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA))
                        degrees = 0;
                    else if (turn_multiplier != 0.0f)
                        degrees = (i32)(degrees * turn_multiplier);
                    i32 limit = (degrees << 16) / 360;
                    if (abs(delta) <= 0x4000) {
                        if (delta > limit)
                            api.movement_facing_angle = narrow_yaw + limit;
                        else if (delta < -limit)
                            api.movement_facing_angle = narrow_yaw - limit;
                    } else {
                        limit = ((180 - degrees) << 16) / 360;
                        if (delta > 0 && delta < limit)
                            api.movement_facing_angle = narrow_yaw + limit;
                        else if (delta < 0 && delta > -limit)
                            api.movement_facing_angle = narrow_yaw - limit;
                    }
                }
            } else if (object->in_narrow_socket &&
                       (requested_speed == 0.0f || object->character_context == 0x2a ||
                        object->character_context == 0x36 || object->character_context == 0x3a)) {
                u16 desired = narrow_yaw;
                if (abs(RotDiff(narrow_yaw, api.movement_facing_angle)) > 0x4000)
                    desired += 0x8000;
                f32 rate = (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                               ? 1.0f
                               : ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->turn_rate;
                if (object->character_context != 0x2a && object->character_context != 0x36 &&
                    object->character_context != 0x3a)
                    rate *= 0.25f;
                api.movement_facing_angle = TurnRot(api.movement_facing_angle, desired, (i32)(rate * 65536.0f), NULL);
            }
            if (object->character_context == -1 && object->field_0x1084 &&
                fabsf(object->contact_normal.y) < NuTrigTable[0x3aaa]) {
                if ((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA == NULL || WORLD->area != PODSPRINT_ADATA) &&
                    (((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->flags_090 & 0x10000) == 0 &&
                    WORLD->current_level != SPEEDERCHASEA_LDATA) {
                    u16 normal_yaw = NuAtan2D(object->contact_normal.x, object->contact_normal.z);
                    if (requested_speed == 0.0f || abs(RotDiff(normal_yaw, object->current_input_angle)) > 0x4000) {
                        i32 delta = RotDiff(normal_yaw, api.field_0x276);
                        if (abs(delta) > 0x4000) {
                            u16 tangent = normal_yaw + (delta < 0 ? -0x4000 : 0x4000);
                            if (object->field_0xddc > 0.0f &&
                                RotDiff(object->previous_boundary_angle, normal_yaw) > 0x2aaa &&
                                abs(RotDiff(api.field_0x276, normal_yaw)) > 0x3fff) {
                                api.movement_facing_angle =
                                    (u16)(object->previous_boundary_angle +
                                          0.5f * RotDiff(object->previous_boundary_angle, normal_yaw) + 32768.0f);
                                object->field_0xe24 |= 2;
                                object->field_0xddc = 0.1f;
                            } else {
                                api.movement_facing_angle = tangent;
                                object->previous_boundary_angle = normal_yaw;
                                object->field_0xddc = 0.1f;
                            }
                        }
                    }
                }
            } else if ((api.flags_low & 0x80) != 0 &&
                       (((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->flags_090 & 0x10000) == 0 &&
                       (i8)object->field_0xf03 >= 0) {
                i32 outside = OutSideSplineArea(&api.collision_position, WORLD->camera_splines[16], &a, &b, 0);
                i32 inside = 0;
                if (!outside)
                    inside = OutSideSplineArea(&api.collision_position, WORLD->camera_splines[17], &a, &b, 1);
                if ((api.field_0x1f4 & 0x400) == 0 && (object->field_0xe24 & 2) == 0 && (outside || inside)) {
                    api.movement_facing_angle = NuAtan2D(-(b.z - a.z), b.x - a.x);
                    object->field_0xe24 |= 2;
                    object->field_0xddc = 0.1f;
                }
            }
            if ((CInfo[object->character_context].flags & 1) != 0 && (object->id == id_STAP || object->id == id_STAP2))
                api.field_0x276 = api.movement_facing_angle = api.facing_angle;
            else
                api.facing_angle = api.field_0x276 =
                    SeekRot(api.facing_angle, api.movement_facing_angle, heading_seek_rate);
        }
        f32 seek_rate = (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                            ? 8.0f
                            : ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->velocity_seek_rate;
        if ((((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->flags_090 & 0x40) != 0 &&
            object->field_0xcc0 == NULL)
            seek_rate = 5.0f;
        i32 special_lean = 0;
        if (api.movement_direction.x != 0.0f || api.movement_direction.z != 0.0f) {
            object->field_0xe23 |= 0x10;
            object->target_velocity.x = api.movement_direction.x;
            object->target_velocity.z = api.movement_direction.z;
            api.movement_direction.x = api.movement_direction.z = 0.0f;
        } else if (CarWashHack == object) {
            object->target_velocity.x = carwash_delta.x * 3.0f;
            object->target_velocity.z = carwash_delta.z * 3.0f;
            seek_rate = 5.0f;
        } else {
            f32 water_mul = 1.0f;
            if (object->character_context == 0x3a)
                object->field_0xdc8 = 1.0f;
            else if (object->character_context == 0x36)
                object->field_0xdc8 =
                    NuTrigTable[((i32)((1.0f - object->context_animation_timer / object->airborne_action_duration) *
                                           65536.0f +
                                       16384.0f) >>
                                 1) &
                                0x7fff];
            else if (object->character_context == 0x2a) {
                f32 half = 0.5f * object->airborne_action_duration;
                object->field_0xdc8 =
                    object->context_animation_timer < half
                        ? -(1.0f - object->context_animation_timer / half)
                        : 1.0f - (object->airborne_action_duration - object->context_animation_timer) / half;
            } else if (api.field_0x27c != -1 && FadeSys.fade > 0.0f && (MiniCutCam == 0 || (api.flags_high & 1) == 0))
                object->field_0xdc8 = VehicleAreaRememberSpeed;
            else {
                f32 decel = (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                                ? 0.25f
                                : ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->field_0x80;
                f32 desired;
                if (WORLD->area == PODRACE_ADATA && object->field_0xee0 != 1.0e9f)
                    desired = GetVehicleSpeedMul(object, object->field_0xee0);
                else if ((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA) &&
                         object->in_narrow_socket) {
                    i32 mode = 0;
                    if ((pad->buttons_held & GAMEPAD_JUMP) != 0 && (pad->buttons_held & GAMEPAD_SPECIAL) == 0)
                        mode = 1;
                    else if ((pad->buttons_held & GAMEPAD_SPECIAL) != 0 && (pad->buttons_held & GAMEPAD_JUMP) == 0)
                        mode = -1;
                    if (PodSprint_InStartCountdown(WORLD) > 0.0f) {
                        object->previous_block_animation = 1;
                        desired = 0.0f;
                    } else if (pad->input_magnitude == 0.0f && mode == 0) {
                        object->previous_block_animation = 0;
                        desired = 25.0f;
                    } else if (mode != 1 && pad->input_angle > 0x3554 && pad->input_angle <= 0xcaaa) {
                        if (mode == -1 || (u16)(pad->input_angle - 0x4aab) <= 0x6aa9) {
                            object->previous_block_animation = 4;
                            desired = 15.0f;
                        } else {
                            object->previous_block_animation = 0;
                            desired = 25.0f;
                        }
                    } else {
                        object->previous_block_animation = 3;
                        desired = 35.0f;
                    }
                    desired /= 35.0f;
                } else if (requested_speed > 0.0f && object->character_context != 0x23 &&
                           object->character_context != 0x24) {
                    if (api.intersects_water) {
                        f32 fraction = (api.water_height - api.collision_min.y) / api.field_0x1e0;
                        if (fraction < 0.0f)
                            water_mul = 0.0f;
                        else if (fraction > 0.5f)
                            water_mul = 0.7f;
                        else
                            water_mul = 1.0f - (fraction + fraction) * 0.3f;
                    }
                    if (object->field_0xee0 != 1.0e9f)
                        desired = GetVehicleSpeedMul(object, object->field_0xee0);
                    else {
                        if (WORLD->current_level == SPEEDERCHASEA_LDATA && (api.flags_low & 0x80) != 0 &&
                            !disable_narrow_socks)
                            desired = GetVehicleSpeedMul(
                                object, ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed * 0.75f +
                                            ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed *
                                                0.25f *
                                                (pad->input_magnitude /
                                                 ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed));
                        else
                            desired = GetVehicleSpeedMul(object, requested_speed);
                        if (desired > object->field_0xdc8)
                            decel = (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                                        ? 0.5f
                                        : ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->field_0x7c;
                    }
                } else if (WORLD->current_level == SPEEDERCHASEA_LDATA && (api.flags_low & 0x80) != 0 &&
                           !disable_narrow_socks)
                    desired = GetVehicleSpeedMul(
                        object, ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed * 0.75f);
                else if ((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA))
                    desired = 25.0f;
                else if ((!IDLESPEEDINNARROWSOCKSONLY || object->in_narrow_socket) && (api.flags_low & 0x80) != 0 &&
                         (object->field_0xf03 & 2) == 0)
                    desired = ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->field_0x10 /
                              ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed;
                else
                    desired = 0.0f;
                f32 step = (1.0f / decel) * FRAMETIME;
                if ((PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA))
                    desired *= object->current_speed_mul;
                object->field_0xdc8 = SeekLinearF(object->field_0xdc8, desired, step);
            }
            f32 speed =
                object->field_0xdc8 * ((PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                                           ? 35.0f
                                           : ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed);
            if ((api.flags_low & 0x80) != 0 && WORLD->current_level == DEATHSTARBATTLED_LDATA &&
                ObjInNarrowSock(object, WORLD->sock_sys, WORLD->level_idx))
                speed *= 2.0f;
            else if ((api.flags_low & 0x80) != 0 && (WORLD->current_level == DEATHSTAR2BATTLEE_LDATA ||
                                                     WORLD->current_level == DEATHSTAR2BATTLEF_LDATA ||
                                                     WORLD->current_level == DEATHSTAR2BATTLEG_LDATA))
                speed *= DeathStar2BattleFire_GetSlowDownMul(object) * 1.5f;
            else if (object->id == id_SPEEDERBIKE) {
                if (WORLD->current_level == SPEEDERCHASEA_LDATA && disable_narrow_socks)
                    speed *= 0.333f;
            } else if (in_tube)
                speed *= 0.25f;
            if (object->in_narrow_socket) {
                other = GetOtherActivePlayer(object);
                i32 forwards;
                if (other != NULL && other->in_narrow_socket &&
                    (forwards = GoingForwardsAlongNarrowSock(object)) == GoingForwardsAlongNarrowSock(other)) {
                    NuVecRotateY(&direction, &v001, object->yrot);
                    NuVecSub(&separation, &other->apiobj.position, &api.position);
                    f32 longitudinal = separation.x * direction.x + separation.z * direction.z;
                    f32 radius = api.field_0x1dc + other->apiobj.field_0x1dc;
                    if (fabsf(longitudinal) > radius * 2.0f) {
                        f32 amount =
                            fabsf(longitudinal) > radius * 5.0f + 0.1f
                                ? 1.0f
                                : (fabsf(longitudinal) - radius * 2.0f) / (radius * 5.0f + 0.1f - radius * 2.0f);
                        i32 ahead = forwards ? longitudinal > 0.0f : longitudinal < 0.0f;
                        f32 factor = other->apiobj.field_0x287 ? 1.0f : 0.75f;
                        if (!ahead)
                            speed *= 1.0f - factor * amount;
                    }
                }
            }
            speed *= water_mul;
            object->target_velocity.z = speed;
            if (object->character_context == 0x3a) {
                object->target_velocity.x =
                    ((PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)
                         ? (object->field_0x7a3 == 0 ? 10.5f : -10.5f)
                         : (object->field_0x7a3 == 0 ? 0.4f : -0.4f) *
                               ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed);
                NuVecRotateY(&object->target_velocity, &object->target_velocity, api.facing_angle);
            } else if ((api.flags_low & 0x80) != 0 && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA) &&
                       object->in_narrow_socket) {
                special_lean = (i32)(8192.0f * pad->input_direction_z);
                object->target_velocity.x =
                    PodSprint_InStartCountdown(WORLD) > 0.0f ? 0.0f : 35.0f * pad->input_direction_z * 0.25f;
                NuVecRotateY(&object->target_velocity, &object->target_velocity, object->yrot);
            } else {
                object->target_velocity.x = 0.0f;
                NuVecRotateY(&object->target_velocity, &object->target_velocity, api.facing_angle);
            }
        }
        if ((api.flags_low & 0x80) != 0)
            CharPivot_Check(object, &object->target_velocity);
        if (WORLD->current_level == SPEEDERCHASEA_LDATA && object->id == id_SPEEDERBIKE &&
            (MiniCutCam != 0 || (!disable_narrow_socks && (api.flags_low & 0x80) != 0 &&
                                 (other = GetOtherActivePlayer(object)) != NULL && other->apiobj.field_0x287 != 0)))
            object->target_velocity.x = object->target_velocity.z = 0.0f;
        api.velocity.x = SeekValF(api.velocity.x, object->target_velocity.x, seek_rate);
        api.velocity.z = SeekValF(api.velocity.z, object->target_velocity.z, seek_rate);
        if (in_tube) {
            object->target_velocity.y = 5.0f;
            api.velocity.y = SeekValF(api.velocity.y, object->target_velocity.y, 10.0f);
        } else if ((((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->field_0x94 & 0x20) != 0 &&
                   ((pad->buttons_held & GAMEPAD_JUMP) != 0 || object->character_context != 0x4b)) {
            object->target_velocity.y =
                (pad->buttons_held & GAMEPAD_JUMP) != 0
                    ? ((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed * 0.7f
                    : -((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->run_speed * 0.7f;
            api.velocity.y = SeekValF(api.velocity.y, object->target_velocity.y, 3.0f);
        } else if (object->character_context == 0x4b) {
            object->target_velocity.y =
                (api.water_height - (api.collision_min.y + (api.collision_max.y - api.collision_min.y) * 0.2f)) * 3.0f;
            api.velocity.y = SeekValF(api.velocity.y, object->target_velocity.y, 10.0f);
        }
        if (WORLD->current_level == PLATFORM_LDATA) {
            api.position.x = 11.805f;
            api.position.y = 3.0f;
            api.position.z = -26.105f;
            object->target_velocity = api.velocity = v000;
        }
        if (special_lean != 0)
            object->movement_lean_angle = SeekRot(object->movement_lean_angle, special_lean, 3.0f);
        else if ((((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->flags_090 & 1) != 0) {
            i32 angular_speed = 0;
            if (object->character_context != 0x2a && object->character_context != 0x36 &&
                object->character_context != 0x3a) {
                angular_speed = (i32)(RotDiff(object->previous_movement_angle, api.field_0x276) / FRAMETIME);
                if (WORLD->current_level == PLATFORM_LDATA && NeedsPretendAnim(object)) {
                    u16 phase = (i32)(NuFmod(GameTimer.time_elapsed, 2.1f) / 2.1f * 65536.0f);
                    angular_speed = (i32)(angular_speed + 4551.0f * NuTrigTable[((phase - 0x2000) >> 1) & 0x7fff]);
                }
            }
            f32 rate = 8.0f;
            if ((((GAMECHARACTERDATA_s *)api.character_data->field11_0x24)->flags_090 & 0x10000) != 0) {
                angular_speed /= 4;
                rate = 4.0f;
            } else if (turn_multiplier != 0.0f)
                angular_speed = (i32)(angular_speed * (1.0f / turn_multiplier));
            if (angular_speed > 65536)
                angular_speed = 8192;
            else if (angular_speed < -65536)
                angular_speed = -8192;
            else {
                angular_speed /= 4;
                if (angular_speed > 8192)
                    angular_speed = 8192;
                else if (angular_speed < -8192)
                    angular_speed = -8192;
            }
            object->movement_lean_angle = SeekRot(object->movement_lean_angle, angular_speed, rate);
        } else
            object->movement_lean_angle = SeekRot(object->movement_lean_angle, 0, 10.0f);
        if (object->id == id_WOOKIEFLYER) {
            u16 target = object->character_context == 0x17 ? 0 : (i32)(-2730.0f * object->field_0xdc8);
            object->secondary_lean_angle = SeekRot(object->secondary_lean_angle, target, 5.0f);
        }
        if (WORLD->current_level == PLATFORM_LDATA) {
            if (object->character_context != 0x2a && object->character_context != 0x36 &&
                object->character_context != 0x3a) {
                u16 target = 0;
                if (object->pad_gamepad->input_magnitude > 0.0f)
                    target = -(i32)((1.0f - abs(RotDiff(api.field_0x276, input_yaw)) * (1.0f / 32768.0f)) * 4551.0f);
                object->secondary_lean_angle = SeekRot(object->secondary_lean_angle, target, 3.0f);
            }
            return;
        }
    }
vehicle_collision:
    GizmoBlowupCheckProximity(WORLD, object);
    if ((VehicleArea || (WORLD->area == SPEEDERCHASE_ADATA && object->id == id_SPEEDERBIKE)) &&
        !(WORLD->area != NULL && (PODSPRINT_ADATA != NULL && WORLD->area == PODSPRINT_ADATA)))
        VehicleCollisionCode(object);
}

void Move_POD(GameObject_s *) {
}

void Move_ATAT(GameObject_s *) {
}

void Move_JAWA(GameObject_s *) {
}

static bool JediHasAction(const GameObject_s *object, JEDI_ACTION action) {
    return object->apiobj.character_model != NULL && object->apiobj.character_model->model_data_b != NULL &&
           object->apiobj.character_model->model_data_b[action] != NULL;
}

static u8 SetComboOpponent(GameObject_s *object, f32 range, i32 update_heading, i32 mode) {
    object->force_target = NULL;
    object->blowup_target = NULL;
    bool searched = false;
    u8 result = 0;
    if ((object->apiobj.flags_low & 0x80) != 0 && mode == 0 && object->pad_gamepad->input_magnitude > 0.0f) {
        object->force_target = ObjOpponent(object, range, 0.0f, 0, 0, 1);
        searched = true;
        if (object->force_target != NULL) {
            result = 1;
            goto finished;
        }
    }
    if (object->ai.primary_target_ref != NULL && object->ai.nearest_opponent_metric < range) {
        ComboOpponent_Range2 = object->ai.nearest_opponent_metric;
        object->force_target = *object->ai.primary_target_ref;
        result = 1;
        goto finished;
    }
    if ((object->apiobj.flags_low & 0x80) == 0) {
        if ((object->field_0xef8 & 0x40) != 0 || object->use_action == 5) {
            object->blowup_target = GizmoBlowUpOpponent(object, range, 0.0f, 0.0f, mode, 0, 0, 0);
            if (object->blowup_target != NULL) {
                ComboOpponent_Range2 = GizmoBlowUpOpponent_Range2;
                result = 4;
            }
        }
        goto finished;
    }
    if (!searched) {
        object->force_target = ObjOpponent(object, range, 0.0f, 0, 0, 1);
        if (object->force_target != NULL) {
            result = 1;
            goto finished;
        }
    }
    object->blowup_target = GizmoBlowUpOpponent(object, range, 0.0f, 0.0f, mode, 0, 0, 0);
    if (object->blowup_target != NULL) {
        ComboOpponent_Range2 = GizmoBlowUpOpponent_Range2;
        result = 4;
        goto finished;
    }
    {
        NUVEC forward;
        if (object->pad_gamepad->input_magnitude > 0.0f) {
            NuVecRotateY(&forward, &v001, GamePad_InputAngle(object, object->pad_gamepad));
        }
        f32 nearest_distance = 1.0e8f;
        GameObject_s *nearest = NULL;
        for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
            GameObject_s *target = &Obj[i];
            if (target == object || (target->apiobj.field_0x1f8 & 0x1001) != 0x1001 ||
                target->apiobj.field_0x287 != 0 || (target->apiobj.field_0x1f4 & 1) != 0 ||
                (target->character_context & 0xfd) == 0x39 || target->character_context == 0x3c ||
                (GetGameCharacterData(target)->flags_090 & 0x8000) != 0 ||
                (CInfo[target->character_context].flags & 0x8000) != 0 || target == object->field_0xcc0)
                continue;
            NUVEC delta;
            f32 distance = NuVecDistSqr(&target->apiobj.position, &object->apiobj.position, &delta);
            if (distance >= range * range ||
                (object->pad_gamepad->input_magnitude > 0.0f && forward.x * delta.x + forward.z * delta.z < 0.0f))
                continue;
            u16 angle = NuAtan2D(target->apiobj.position.x - object->apiobj.position.x,
                                 target->apiobj.position.z - object->apiobj.position.z);
            i32 difference = RotDiff(object->apiobj.movement_facing_angle, angle);
            if (difference >= -0x2000 && difference <= 0x2000 && distance < nearest_distance) {
                nearest = target;
                nearest_distance = distance;
            }
        }
        PlayerOpponent_Range2 = nearest_distance;
        object->force_target = nearest;
        if (nearest != NULL) {
            ComboOpponent_Range2 = nearest_distance;
            result = 2;
            goto finished;
        }
    }
    if (!searched) {
        object->force_target = ObjOpponent(object, range, 0.0f, 0, 0, 1);
        result = object->force_target != NULL;
    }
finished:
    if (update_heading != 0 && object->force_target == NULL && object->pad_gamepad->input_magnitude > 0.0f &&
        (object->field_0xe22 & 0x20) != 0) {
        object->apiobj.movement_facing_angle = object->current_input_angle;
    }
    return result;
}

static void BlockCode(GameObject_s *object, i32 pressed, i32 held, i32 jump_pressed, i32 allow_start) {
    if ((object->field_0xe22 & 1) != 0 && object->weapon_scale_state == 0) {
        void **animations = object->apiobj.character_model->model_data_b;
        bool has_block = animations[0x1a] != NULL || animations[0x1b] != NULL || animations[0x1c] != NULL;
        object->incoming_bolt = has_block ? FindIncomingBolt(object, 0, 0) : NULL;
        animations = object->apiobj.character_model->model_data_b;
        has_block = animations[0x1a] != NULL || animations[0x1b] != NULL || animations[0x1c] != NULL;
        GameObject_s *melee = NULL;
        if (has_block) {
            for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
                GameObject_s *target = &Obj[i];
                if (target != object && (target->apiobj.field_0x1f8 & 0x1000) != 0 && target->apiobj.field_0x287 == 0 &&
                    target->character_context == 5 && (target->context_flags & 0xc) == 0 &&
                    target->force_target == object) {
                    melee = target;
                    break;
                }
            }
        }
        object->incoming_melee = melee;
        GameObject_s *special = NULL;
        if (((object->apiobj.character_data->model_flags & 8) != 0 || object->id == id_BODYGUARD ||
             object->id == id_IMPERIALGUARD) &&
            has_block) {
            for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
                GameObject_s *target = &Obj[i];
                if (target != object && (target->apiobj.field_0x1f8 & 0x1000) != 0 && target->apiobj.field_0x287 == 0 &&
                    target->character_context == 0x26 && target->force_target == object &&
                    (target->id != id_GAMORREANGUARD || target->context_animation != 0x56 ||
                     (target->context_flags & 0x40) == 0)) {
                    special = target;
                    break;
                }
            }
        }
        object->incoming_special = special;
        if (object->incoming_bolt == NULL && melee == NULL) {
            if (object->apiobj.field_0x27c == -1 || !has_block)
                object->incoming_part = NULL;
            else {
                f32 radius = object->apiobj.field_0x1e0 > object->apiobj.field_0x1dc ? object->apiobj.field_0x1e0
                                                                                     : object->apiobj.field_0x1dc;
                object->incoming_part = FindIncomingPart(object, &object->apiobj.collision_position, radius, 10, 0.0f);
            }
        }
        if ((object->field_0xe22 & 8) == 0 &&
            (object->incoming_bolt != NULL || object->incoming_melee != NULL || object->incoming_part != NULL) &&
            object->character_context == -1)
            object->field_0xe22 |= 8;
    }
    if (object->character_context == 0xc) {
        GameObject_s *attacker = object->block_attacker;
        if (attacker != NULL) {
            if (jump_pressed != 0 && object->apiobj.character_model->model_data_b[0x12] != NULL) {
                SetComboOpponent(object, 1.0f, 0, 1);
                f32 speed = GetGameCharacterData(object)->second_jump_speed;
                object->jump_flags &= ~1;
                object->context_animation_timer = 0.0f;
                object->delayed_turn_timer = 0.0f;
                object->airborne_input_timer = 0.0f;
                object->field_0xe22 |= 0x10;
                object->character_context = 0;
                object->apiobj.velocity.y = speed;
                object->context_variant_flags &= 0x4f;
                object->action_movement_state = 2;
                object->context_animation = 0x12;
                object->apiobj.field_0x27d = 0;
                object->field_0x105c = 0;
                object->collision_target = NULL;
                PlayJumpSfx(object, 2);
                ResetAnimPacket(&object->apiobj.anim_packet, object->context_animation);
                if ((object->apiobj.flags_low & 0x80) != 0)
                    object->field_0xef9 |= 8;
                return;
            }
            if (object->block_latch == 0 && object->blocked_attack_stage != 0) {
                if ((attacker->apiobj.field_0x1f8 & 0x1000) != 0 && attacker->apiobj.field_0x287 == 0) {
                    if ((object->field_0xe23 & 0x20) == 0) {
                        if (attacker->character_context == 5 && attacker->combo_stage < object->blocked_attack_stage)
                            return;
                    } else if (attacker->character_context == 0x26 &&
                               attacker->context_animation == object->blocked_attack_stage &&
                               (attacker->id != id_GAMORREANGUARD || attacker->context_animation != 0x56 ||
                                (attacker->context_flags & 0x40) == 0))
                        return;
                }
                object->blocked_attack_stage = 0;
                return;
            }
        }
        if (object->block_latch == 0) {
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                object->block_latch = 1;
                object->context_animation_timer = 0.2f;
            }
            return;
        }
        if (object->id == id_GAMORREANGUARD && (object->apiobj.flags_low & 0x80) == 0) {
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                object->character_context = -1;
                object->block_cooldown = 1.5f;
            }
            return;
        }
        if (pressed == 0 || (object->incoming_bolt == NULL && object->incoming_melee == NULL &&
                             object->incoming_special == NULL && object->incoming_part == NULL)) {
            if (held != 0 && object->incoming_bolt != NULL) {
                object->context_animation_timer = 0.2f;
                return;
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer > 0.0f)
                return;
            if (held != 0 &&
                (attacker == NULL || attacker->id != id_GAMORREANGUARD || attacker->character_context != 0x26 ||
                 attacker->context_animation != 0x56 || (attacker->context_flags & 0x40) == 0)) {
                StartHold(object);
                return;
            }
            object->character_context = -1;
            return;
        }
        if (!TouchHacks::ShouldBlock(*object))
            return;
    } else {
        if (object->block_cooldown > 0.0f) {
            object->block_cooldown -= FRAMETIME;
            return;
        }
        if (!allow_start || !pressed || object->apiobj.field_0x27d == 0 ||
            (CInfo[object->character_context].flags & 0x20) != 0 || (object->field_0xef8 & 2) == 0 ||
            (object->incoming_bolt == NULL && object->incoming_melee == NULL && object->incoming_special == NULL &&
             object->incoming_part == NULL))
            return;
    }
    if (!NewBlockAction(object)) {
        object->character_context = -1;
        return;
    }
    object->character_context = 0xc;
    object->block_latch = 0;
    if (object->incoming_bolt != NULL) {
        object->blocked_bolt = object->incoming_bolt;
        object->block_attacker = NULL;
        object->blocked_part = NULL;
        object->context_animation_timer = 0.3f;
        object->blocked_attack_stage = 0;
        if ((object->apiobj.flags_low & 0x80) != 0)
            Hint_SetComplete(0x5dd);
    } else if (object->incoming_part != NULL) {
        object->blocked_bolt = NULL;
        object->block_attacker = NULL;
        object->blocked_part = object->incoming_part;
        object->context_animation_timer = 0.3f;
        object->blocked_attack_stage = 0;
    } else {
        object->blocked_bolt = NULL;
        object->blocked_part = NULL;
        object->context_animation_timer = 0.1f;
        if (object->incoming_special != NULL) {
            object->block_attacker = object->incoming_special;
            object->blocked_attack_stage = object->incoming_special->context_animation;
            object->field_0xe23 |= 0x20;
        } else {
            object->block_attacker = object->incoming_melee;
            object->blocked_attack_stage = object->incoming_melee->combo_stage + 1;
        }
    }
}

static void SwipeCode(GameObject_s *object, i32 pressed, i32 held) {
    i8 context = object->character_context;
    if (context == 0x10) {
        f32 *frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
        if (frame != NULL) {
            if ((object->context_flags & 0x40) == 0) {
                f32 hit_frame = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                if (hit_frame >= 1.0f && hit_frame <= *frame)
                    ComboHitFrame(object, 1);
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                object->field_0xe20 |= 4;
                object->character_context = -1;
                if (object->context_animation == 0x29) {
                    if ((object->context_flags & 0x40) == 0) {
                        ComboHitFrame(object, 1);
                        object->character_context = 0x10;
                        object->context_animation_timer = 0.001f;
                    } else if (held)
                        StartHold(object);
                }
            }
        }
    } else if ((object->field_0xe22 & 1) != 0 && object->weapon_scale_state == 0 && pressed &&
               object->apiobj.field_0x27d != 0 && object->apiobj.character_model != NULL &&
               (context == 1 || context == -1 || context == 2 || context == 4 || context == 3) &&
               SetComboOpponent(object, 0.6f, 0, 2) == 1) {
        NUVEC *position;
        if (object->force_target != NULL)
            position = &object->force_target->apiobj.position;
        else if (object->blowup_target != NULL)
            position = &object->blowup_target->mid_position;
        else
            return;
        NUVEC delta;
        NuVecSub(&delta, position, &object->apiobj.position);
        NuVecRotateY(&delta, &delta, -object->apiobj.movement_facing_angle);
        if ((object->pad_gamepad->input_magnitude <= 0.0f || fabsf(delta.x) <= fabsf(delta.z)) && delta.z < 0.0f &&
            ComboOpponent_Range2 < 0.48999998f) {
            object->context_animation = 0x29;
            if (object->apiobj.character_model->model_data_b[0x29] != NULL) {
                object->context_animation_timer = AnimDuration(object->id, 0x29, 0.0f, 0.0f, 1);
                if (object->context_animation_timer > 0.0f) {
                    object->character_context = 0x10;
                    object->context_flags &= ~0x40;
                    PlaySabreSfx(NULL, object, NULL, 1);
                    NewRumble(object->pad_gamepad->pad, 0.4f, 0);
                }
            }
        }
    }
}

static void LightSabreComboCode(GameObject_s *object, i32 action_pressed, i32 action_held, i32 jump_pressed,
                                i32 buttons_pressed) {
    ANIMPACKET_s &animation = object->apiobj.anim_packet;
    if ((object->apiobj.flags_low & 0x80) != 0)
        object->ai_combo_cooldown = 0.0f;
    else if (object->ai_combo_cooldown > 0.0f)
        object->ai_combo_cooldown -= FRAMETIME;
    u8 previous_flags = object->context_flags;
    object->context_flags &= ~2;
    if (object->character_context == CHARACTER_CONTEXT_COMBO) {
        object->field_0xef9 |= 8;
        if ((object->context_flags & 4) != 0) {
            if ((action_pressed == 0 && object->combo_input_latched == 0) || object->combo_stage > 1) {
                object->combo_input_timer -= FRAMETIME;
                if (object->combo_input_timer <= 0.0f) {
                    object->character_context = CHARACTER_CONTEXT_NONE;
                    object->context_flags |= 2;
                    if (action_held != 0)
                        StartHold(object);
                } else if (object->combo_stage < 2 && jump_pressed != 0 &&
                           JediHasAction(object, static_cast<JEDI_ACTION>(0x12))) {
                    SetComboOpponent(object, 1.0f, 0, 1);
                    object->jump_flags &= ~1;
                    object->context_animation_timer = 0.0f;
                    object->delayed_turn_timer = 0.0f;
                    object->airborne_input_timer = 0.0f;
                    object->field_0xe22 |= 0x10;
                    object->character_context = 0;
                    object->apiobj.velocity.y = GetGameCharacterData(object)->second_jump_speed;
                    object->context_variant_flags &= 0x4f;
                    object->action_movement_state = 2;
                    object->context_animation = 0x12;
                    object->apiobj.field_0x27d = 0;
                    object->field_0x105c = 0;
                    object->collision_target = NULL;
                    PlayJumpSfx(object, 2);
                    ResetAnimPacket(&animation, object->context_animation);
                    if ((object->apiobj.flags_low & 0x80) != 0)
                        object->field_0xef9 |= 8;
                }
            } else {
                SetComboOpponent(object, 1.0f, 1, 0);
                object->context_flags &= ~4;
                object->combo_stage = object->combo_stage == 1 ? 2 : 1;
                if (object->id != id_IMPERIALGUARD)
                    PlaySabreSfx(NULL, object, NULL, 1);
                if (object->combo_stage == 2)
                    NewRumble(object->pad_gamepad->pad, 0.35f, 0);
                if (object->combo_stage == 1) {
                    object->combo_branch =
                        (object->field_0xe20 & 0x40) != 0 &&
                                JediHasAction(object, static_cast<JEDI_ACTION>(object->queued_context_animation + 2))
                            ? 2
                            : 1;
                    if (object->combo_branch == 1)
                        object->field_0xe20 &= ~0x40;
                    NewRumble(object->pad_gamepad->pad, object->combo_branch == 2 ? 0.5f : 0.35f, 0);
                    if (object->combo_branch == 2 && object->id != id_IMPERIALGUARD) {
                        PlaySabreSfx("ComboSaber2", object, &object->apiobj.collision_position, 0);
                    }
                } else if (object->combo_branch == 1) {
                    object->combo_branch =
                        (object->field_0xe20 & 0x40) != 0 &&
                                JediHasAction(object, static_cast<JEDI_ACTION>(object->queued_context_animation + 4))
                            ? 4
                            : 3;
                    if (object->combo_branch == 3)
                        object->field_0xe20 &= ~0x40;
                } else {
                    object->combo_branch =
                        (object->field_0xe20 & 0x40) != 0 &&
                                JediHasAction(object, static_cast<JEDI_ACTION>(object->queued_context_animation + 6))
                            ? 6
                            : 5;
                    if (object->combo_branch == 5)
                        object->field_0xe20 &= ~0x40;
                    else {
                        NewRumble(object->pad_gamepad->pad, 0.6f, 0);
                        if (object->id != id_IMPERIALGUARD) {
                            PlaySabreSfx("ComboSaber1", object, &object->apiobj.collision_position, 0);
                        }
                    }
                }
                object->context_animation = object->queued_context_animation + object->combo_branch;
                object->field_0xe20 |= 0x40;
                object->combo_input_latched = 0;
                object->context_flags &= ~0x50;
            }
            goto combo_finished;
        }
        if (animation.animation_index == object->context_animation && animation.blending == 0 &&
            animation.requested_animation == object->context_animation) {
            if (object->combo_stage == 2) {
                f32 *frames = AnimListFrameArray(object->apiobj.character_model, object->context_animation);
                if ((object->context_flags & 0x40) == 0 && frames[0] > 0.0f && animation.current_time >= frames[0]) {
                    ComboHitFrame(object, object->combo_branch == 6 ? 3 : 1);
                }
                if (frames[1] > 0.0f && animation.current_time >= frames[1])
                    object->context_flags |= 8;
                CHARACTERANIM_s *info = static_cast<CHARACTERANIM_s *>(
                    object->apiobj.character_model->model_data_a[object->context_animation]);
                if ((animation.flags & ANIMPACKET_FLAG_FINISHED) != 0 ||
                    (info->blend_out_time > 0.0f &&
                     AnimDuration(object->id, object->context_animation, animation.current_time, 0.0f, 0) <
                         info->blend_out_time)) {
                    object->character_context = CHARACTER_CONTEXT_NONE;
                    object->context_flags |= 2;
                }
                object->sabre_flags |= 2;
                if ((object->context_flags & 0x40) == 0 &&
                    (((object->context_flags & 8) != 0 && frames[0] == 0.0f) || (object->context_flags & 2) != 0)) {
                    ComboHitFrame(object, object->combo_branch == 6 ? 3 : 1);
                }
                if (object->character_context == CHARACTER_CONTEXT_NONE && action_held != 0 &&
                    (object->id != id_IMPERIALGUARD || object->combo_stage != 2))
                    StartHold(object);
                if ((object->context_flags & 0x40) != 0 && (previous_flags & 0x40) == 0) {
                    NewRumble(object->pad_gamepad->pad, object->combo_branch == 6 ? 0.7f : 0.35f, 0);
                }
                if ((object->apiobj.flags_low & 0x80) == 0 &&
                    ((object->field_0xefb & 8) != 0 || WORLD->current_level == VADERC_LDATA)) {
                    object->ai_combo_cooldown = 3.0f;
                }
                goto combo_finished;
            }
            if ((animation.flags & ANIMPACKET_FLAG_FINISHED) != 0) {
                object->combo_input_timer = 0.2f;
                object->context_flags |= 4;
                BlockSfx(object);
                object->sabre_flags |= 2;
                if ((object->context_flags & 0x40) == 0)
                    ComboHitFrame(object, 1);
                goto combo_finished;
            }
        }
        f32 *time = AnimPlaying(&animation, object->context_animation, 1, 0);
        if (time != NULL && (object->context_flags & 0x40) == 0) {
            const f32 frame = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
            if (frame > 0.0f && *time >= frame) {
                ComboHitFrame(object, object->combo_branch == 2 ? 2 : 1);
            }
        }
        bool can_queue = false;
        if (time != NULL) {
            const f32 frame = AnimListFrame(object->apiobj.character_model, object->context_animation, 1);
            if (frame > 0.0f && *time >= frame)
                object->context_flags |= 0x10;
            can_queue = AnimDuration(object->id, object->context_animation, *time, 0.0f, 0) < 0.1f;
            if (can_queue)
                object->field_0xe22 |= 8;
        }
        if (action_pressed != 0 && object->combo_input_latched == 0) {
            if (can_queue)
                object->combo_input_latched = 1;
            else
                object->field_0xe20 &= ~0x40;
        }
        object->sabre_flags |= 3;
        goto combo_finished;
    }

    {
        const i8 context = object->character_context;
        const bool player_active = (object->apiobj.flags_low & 0x80) != 0;
        if (action_pressed == 0 || (!player_active && object->ai_combo_cooldown > 0.0f) || context == 0x15 ||
            context == 0x5a || context == 0x13 || context == 0x2e ||
            (!player_active && static_cast<u8>(context - 6) < 2) || context == 8 || context == 0x1b ||
            context == 0x2d || context == 0xf || context == 0x27 || context == 0x28 || context == 0x59 ||
            context == 0x30 || context == 0x3d || context == 0x43 || context == 0x4a || context == 0x61 ||
            context == 0x4b) {
            if (((object->pad_gamepad->allocated_5a & 4) != 0 || buttons_pressed != 0) &&
                (object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 &&
                object->weapon_scale_state == WEAPON_SCALE_IDLE && context != 6 && context != 7 && context != 18 &&
                context != 27 && context != 29 && context != 13 && context != 14 && context != 46 &&
                !(context == 0 && static_cast<u8>(object->action_movement_state - 3) < 2) &&
                (object->field_0xe23 & 1) == 0 && (CInfo[context].flags & 0x4000000) == 0 &&
                ((CInfo[context].flags & 0x8000000) == 0 || (object->jump_flags & 2) == 0) && context != 8) {
                GAMECHARACTERDATA *data = GetGameCharacterData(object);
                const i32 action =
                    data->field275_0x116 == 0 && (object->apiobj.character_data->model_flags & 0x80) != 0 ? 126 : 16;
                if (object->apiobj.field_0x27d != 0 && object->pad_gamepad->input_magnitude == 0.0f &&
                    object->apiobj.character_model->model_data_b[action] != NULL &&
                    (context == -1 || (CInfo[context].flags & 4) != 0)) {
                    SlowWeaponIn(object);
                } else {
                    FastWeaponIn(object, 1);
                }
            }
            goto combo_finished;
        }
        if ((object->field_0xe22 & 1) == 0) {
            GAMECHARACTERDATA *data = GetGameCharacterData(object);
            const i32 draw_action =
                data->field275_0x116 == 0 && (object->apiobj.character_data->model_flags & 0x80) != 0 ? 127 : 17;
            if (object->weapon_scale_state == 0 && object->apiobj.field_0x27d != 0 &&
                object->pad_gamepad->input_magnitude == 0.0f &&
                object->apiobj.character_model->model_data_b[draw_action] != NULL && context >= -1 && context <= 4) {
                SlowWeaponOut(object);
            } else {
                FastWeaponOut(object, 1);
            }
            goto combo_finished;
        }
        if ((object->field_0xe20 & 4) != 0 || (!player_active && object->weapon_scale_state != 0) ||
            object->apiobj.field_0x27d == 0 ||
            (context != 1 && context != -1 && context != 2 && context != 4 && context != 3 && context != 6 &&
             context != 7))
            goto combo_finished;
        SetWeaponOut(object);
        if (object->incoming_bolt != NULL || object->incoming_melee != NULL || object->incoming_special != NULL ||
            object->incoming_part != NULL) {
            if (!TouchHacks::ShouldBlock(*object))
                goto combo_finished;
            if (!NewBlockAction(object)) {
                object->character_context = -1;
                goto combo_finished;
            }
            object->character_context = 0xc;
            object->block_latch = 0;
            if (object->incoming_bolt != NULL) {
                object->blocked_bolt = object->incoming_bolt;
                object->block_attacker = NULL;
                object->blocked_part = NULL;
                object->context_animation_timer = 0.3f;
                object->blocked_attack_stage = 0;
                if ((object->apiobj.flags_low & 0x80) != 0)
                    Hint_SetComplete(0x5dd);
            } else if (object->incoming_part != NULL) {
                object->blocked_bolt = NULL;
                object->block_attacker = NULL;
                object->blocked_part = object->incoming_part;
                object->blocked_attack_stage = 0;
                object->context_animation_timer = 0.3f;
            } else {
                object->blocked_bolt = NULL;
                object->blocked_part = NULL;
                object->context_animation_timer = 0.1f;
                if (object->incoming_special != NULL) {
                    object->block_attacker = object->incoming_special;
                    object->blocked_attack_stage = object->incoming_special->context_animation;
                    object->field_0xe23 |= 0x20;
                } else {
                    object->block_attacker = object->incoming_melee;
                    object->blocked_attack_stage = object->incoming_melee->combo_stage + 1;
                }
            }
            goto combo_finished;
        }
        if (!player_active &&
            ((object->field_0xf04 & 2) != 0 || (object->apiobj.character_data->model_flags & 4) == 0) &&
            (object->field_0xef9 & 4) == 0)
            goto combo_finished;
        SetComboOpponent(object, 1.0f, 1, 0);
        if ((object->apiobj.flags_low & 0x80) != 0 && object->force_target != NULL &&
            object->force_target->apiobj.field_0x27c == -1)
            Hint_SetComplete(0x277);
        object->character_context = CHARACTER_CONTEXT_COMBO;
        object->combo_stage = 0;
        object->combo_input_timer = 0.0f;

        JEDI_ACTION action = JEDI_ACTION_COMBO_1_1;
        u8 alternate = 0;
        if (JediHasAction(object, JEDI_ACTION_COMBO_2_1) && object->combo_alternate == 0) {
            action = JEDI_ACTION_COMBO_2_1;
            alternate = 1;
        }
        object->queued_context_animation = action;
        object->context_animation = action;
        object->field_0xe22 |= GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION;
        object->field_0xe20 |= GAMEOBJECT_E20_FLAG_COMBO_MOVEMENT;
        object->context_flags &= GAMEOBJECT_CONTEXT_FLAGS_COMBO_START_RETAIN_MASK;
        object->combo_alternate = alternate;
        object->combo_branch = 0;
        object->combo_input_latched = 0;
        object->weapon_scale = 1.0f;
        object->weapon_scale_state = WEAPON_SCALE_IDLE;
        object->apiobj.velocity.y = 0.0f;
        if (object->id != id_IMPERIALGUARD)
            PlaySabreSfx(NULL, object, NULL, 1);

        if (!JediHasAction(object, action)) {
            object->character_context = CHARACTER_CONTEXT_NONE;
        }
    }
combo_finished:
    if (object->weapon_scale == 1.0f && (object->field_0xe22 & 8) == 0 && object->character_context == 5 &&
        (object->context_flags & 4) != 0)
        object->field_0xe22 |= 8;
}

static void JediLightCode(GameObject_s *object) {
    if (object->dynamic_light_id == -1)
        return;
    NUVEC colour;
    NUVEC position;
    if (object->character_context == 0x1b && (object->field_0xe21 & 5) == 1 && object->force_target != NULL) {
        rtlDynamicEnable(object->dynamic_light_id, 1);
        NUVEC delta;
        f32 distance =
            NuVecDist(&object->force_target->apiobj.collision_position, &object->apiobj.collision_position, &delta);
        rtlDynamicSetRadii(object->dynamic_light_id, distance * 0.5f, distance * 0.5f + 0.5f);
        colour.x = 0.0f;
        colour.y = qrand() < 0x8000 ? 2.0f : 0.25f;
        colour.z = colour.y;
        rtlDynamicSetColours(object->dynamic_light_id, &colour, NULL);
        position.x = delta.x * 0.5f + object->apiobj.pos_x;
        position.y = delta.y * 0.5f + object->apiobj.pos_y;
        position.z = delta.z * 0.5f + object->apiobj.pos_z;
        rtlDynamicSetPos(object->dynamic_light_id, &position);
        return;
    }
    if (object->weapon_scale <= 0.0f || object->sabre_flags == 0)
        return;
    position = v000;
    if (object->apiobj.field_0x288 == 0)
        return;
    i32 point_count = 0;
    for (i32 blade = 0; blade < 4; ++blade) {
        if (object->apiobj.field_0x288 == 0)
            break;
        if ((object->field_0xe23 & 8) == 0)
            continue;
        GAMECHARACTERDATA *data = GetGameCharacterData(object);
        i32 first = data->streak_joints[blade][0];
        i32 second = data->streak_joints[blade][1];
        if (first == -1 || object->apiobj.character_model->points_of_interest[first] == NULL || second == -1 ||
            object->apiobj.character_model->points_of_interest[second] == NULL)
            continue;
        NuVecAdd(&position, &position, reinterpret_cast<NUVEC *>(&object->joint_matrices[first].m30));
        NuVecAdd(&position, &position, reinterpret_cast<NUVEC *>(&object->joint_matrices[second].m30));
        point_count += 2;
    }
    if (point_count == 0)
        return;
    NuVecScale(&position, &position, 1.0f / point_count);
    rtlDynamicEnable(object->dynamic_light_id, 1);
    f32 radius = object->sabre_collision_radius * 3.0f * object->weapon_scale * object->apiobj.field_0xa8;
    rtlDynamicSetRadii(object->dynamic_light_id, radius + 0.25f, radius + 0.5f);
    f32 brightness = qrand() < 0x8000 ? 0.003921569f : 0.007843138f;
    i32 blade = -1;
    if (object->apiobj.field_0x27c != -1 && Cheat_IsOn(0x19))
        blade = 0;
    else if (object->apiobj.field_0x27c != -1 && Player_HasPurpleForce(object))
        blade = 3;
    else if (object->id == id_GRIEVOUS) {
        f32 phase =
            (NU_SIN_LUT(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 2.0f) * 0.5f * 65536.0f)) + 1.0f) * 0.5f;
        colour.x = ((BladeTab[2].colour[0] - static_cast<f32>(BladeTab[1].colour[0])) * phase + BladeTab[1].colour[0]) *
                   brightness;
        colour.y = ((BladeTab[2].colour[1] - static_cast<f32>(BladeTab[1].colour[1])) * phase + BladeTab[1].colour[1]) *
                   brightness;
        colour.z = ((BladeTab[2].colour[2] - static_cast<f32>(BladeTab[1].colour[2])) * phase + BladeTab[1].colour[2]) *
                   brightness;
    } else
        blade = static_cast<i8>(GetGameCharacterData(object)->field_0x117);
    if (blade != -1) {
        colour.x = BladeTab[blade].colour[0] * brightness;
        colour.y = BladeTab[blade].colour[1] * brightness;
        colour.z = BladeTab[blade].colour[2] * brightness;
    }
    rtlDynamicSetColours(object->dynamic_light_id, &colour, NULL);
    rtlDynamicSetColours(object->dynamic_light_id, &colour, NULL);
    rtlDynamicSetPos(object->dynamic_light_id, &position);
}

static void ForceGlowCode(GameObject_s *object, i32 model) {
    if (object->field_0xd80 == 0.0f &&
        (object->force_glow_previous == NULL || object->force_glow_previous == object->force_glow_candidate)) {
        object->force_glow_object = object->force_glow_candidate;
        object->force_glow_kind = object->force_glow_candidate_kind;
    }
    f32 target;
    if (object->force_glow_object == NULL || object->force_glow_candidate != object->force_glow_object) {
        target = FORCEGLOWTIME;
        if (object->field_0xd80 == target && object->force_hold_time > 0.0f) {
            object->force_hold_time -= FRAMETIME;
        } else
            target = 0.0f;
    } else if ((object->apiobj.flags_low & 0x80) == 0 && object->force_glow_kind == 0 &&
               object->character_context != 8 && AnimPlaying(&object->apiobj.anim_packet, 0xb, 1, 1) == NULL &&
               AnimPlaying(&object->apiobj.anim_packet, 0x27, 1, 1) == NULL)
        target = 0.0f;
    else
        target = FORCEGLOWTIME;
    f32 previous = object->field_0xd80;
    if (object->force_glow_target == previous)
        object->force_glow_target = target;
    else {
        if (previous > object->force_glow_target) {
            object->field_0xd80 = previous - FRAMETIME * 0.5f;
            if (object->field_0xd80 < 0.0f)
                object->field_0xd80 = 0.0f;
        } else {
            object->field_0xd80 = previous + FRAMETIME;
            if (object->field_0xd80 > object->force_glow_target)
                object->field_0xd80 = object->force_glow_target;
        }
        if (object->field_0xd80 == FORCEGLOWTIME && previous != FORCEGLOWTIME)
            object->force_hold_time = 0.333f;
    }
    if (object->field_0xd80 > 0.0f && object->force_glow_object == NULL)
        ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    if (!(object->field_0xd80 > 0.0f) || object->force_glow_object == NULL)
        return;
    switch (object->force_glow_kind) {
        case 0: {
            GIZFORCE_s *force = static_cast<GIZFORCE_s *>(object->force_glow_object);
            if ((force->progress_flags & 1) == 0) {
                ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
                return;
            }
            f32 radius = force->radius;
            object->field_0xd8c = TouchHacks::TouchControlsActive && force->strength_0x6c > 0.0f
                                      ? force->strength_0x6c
                                      : force->horizontal_range * radius;
            object->field_0xe1e = model;
            object->force_glow_position = force->position;
            force->field_0xaa |= 2;
            break;
        }
        case 1: {
            GAMEANIMOBJ_s *animation = static_cast<GAMEANIMOBJ_s *>(object->force_glow_object);
            f32 scale = object->field_0xd8c;
            if (!NuSpecialExistsFn(&animation->special)) {
                ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
                return;
            }
            NuSpecialGetRadius(&animation->special, &object->force_glow_position, &object->field_0xd8c);
            NuVecMtxTransformVU0(&object->force_glow_position, &object->force_glow_position,
                                 NuSpecialGetDrawMtx(&animation->special));
            object->field_0xe1e = model;
            object->field_0xd8c *= scale;
            GIZFORCEANIMDATA_s *data = static_cast<GIZFORCEANIMDATA_s *>(animation->object_data);
            if (data != NULL)
                data->force_glow_active = 1;
            break;
        }
        case 2: {
            GameObject_s *target_object = static_cast<GameObject_s *>(object->force_glow_object);
            if ((target_object->apiobj.field_0x1f8 & 0x1001) == 0 ||
                (TouchHacks::TouchControlsActive && target_object->apiobj.field_0x27c != -1)) {
                ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
                return;
            }
            f32 radius = NuFmax(target_object->apiobj.field_0x1dc, target_object->apiobj.field_0x1e0) * 1.75f;
            object->field_0xe1e = model;
            object->force_glow_position.x = target_object->apiobj.collision_position.x;
            f32 bottom = target_object->character_bottom;
            f32 scale = target_object->apiobj.field_0xa8;
            object->force_glow_position.y = bottom * scale + target_object->apiobj.position.y +
                                            (target_object->character_top - bottom) * 0.5f * scale;
            object->force_glow_position.z = target_object->apiobj.collision_position.z;
            object->field_0xd8c = radius;
            break;
        }
        case 3: {
            PART_s *part = static_cast<PART_s *>(object->force_glow_object);
            if ((part->active & 1) == 0) {
                ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
                return;
            }
            f32 radius;
            if (NuSpecialExistsFn(&part->special)) {
                NUVEC center;
                NuSpecialGetRadius(&part->special, &center, &object->field_0xd8c);
                radius = object->field_0xd8c + object->field_0xd8c;
            } else
                radius = part->radius + part->radius;
            object->field_0xe1e = model;
            object->force_glow_position = part->position;
            object->field_0xd8c = radius;
            break;
        }
    }
    object->field_0xd8c *= 1.125f;
}

static void ForceCode(GameObject_s *object, i32 pressed, i32 held, i32) {
    u8 previous_force = object->force_repeat_requested;
    object->force_repeat_requested = 0;
    if (object->character_context == 8) {
        if ((!held && (object->pad_gamepad->allocated_5a & 4) == 0) || object->gizforce_target == NULL) {
            object->character_context = -1;
        } else {
            if ((dagobah_training == 0 || object->field_0xcc0 != NULL || FreePlay != 0) &&
                !GizForce_Complete(object->gizforce_target) &&
                !GizForce_StoodOnForce(object->gizforce_target, object)) {
                AlertSurroundingCreatures(object, &object->apiobj.collision_position);
                if ((WORLD->current_level == VADERC_LDATA || (object->apiobj.flags_low & 0x80) == 0 ||
                     object->apiobj.model_draw_result == 0 || !(object->field_0xc54 > 0.0f) ||
                     !(object->head_target_facing < -0.85f || object->head_target_facing > 0.85f)) &&
                    !(WORLD->current_level == DAGOBAHC_LDATA && object->id == id_LUKESKYWALKERDAGOBAH &&
                      LevGizmo[0] != NULL && object->gizforce_target == LevGizmo[0]->object &&
                      GameAnimSet_GetCompletionRatio(object->gizforce_target->anim_set) > 0.5f)) {
                    GIZFORCE_s *force = object->gizforce_target;
                    force->using_object = object;
                    object->force_heading = GizForces_AngleToForce(&object->apiobj.position, force);
                    AlertSurroundingCreatures(object, &object->apiobj.collision_position);
                    object->field_0xe22 |= 2;
                    force = object->gizforce_target;
                    if (object->gizforce_target_object == NULL) {
                        object->force_glow_candidate = force;
                        object->force_glow_candidate_kind = 0;
                        if (force != NULL)
                            SetHeadTarget(object, &force->file_position, 1, 2.0f, 5.0f, 10.0f);
                    } else {
                        object->force_glow_candidate = object->gizforce_target_object;
                        object->force_glow_candidate_kind = 1;
                        object->field_0xd8c = force == NULL ? 1.0f : force->horizontal_range;
                        SetHeadTarget(object, NuSpecialGetDrawPos(&object->gizforce_target_object->special), 1, 2.0f,
                                      5.0f, 10.0f);
                    }
                    if (force != NULL)
                        SetHeadTarget(object, &force->file_position, 1, 2.0f, 5.0f, 10.0f);
                } else {
                    object->character_context = -1;
                    object->gizforce_target = NULL;
                }
            } else {
                AlertSurroundingCreatures(object, &object->apiobj.collision_position);
                object->character_context = -1;
                object->gizforce_target = NULL;
            }
        }
        if (object->character_context == -1)
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
        return;
    }
    if ((static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_094[1] & 0x80) != 0 ||
        (object->field_0xe23 & 1) != 0 || object->character_context == 0x12)
        return;
    bool start = false;
    if (object->apiobj.field_0x27d != 0 &&
        (object->character_context == -1 || (CInfo[object->character_context].flags & 4) != 0)) {
        if (((object->apiobj.flags_low & 0x80) != 0 && pressed) || (object->pad_gamepad->allocated_5a & 4) != 0 ||
            (held && previous_force))
            start = true;
    }
    if (start) {
        if ((object->pad_gamepad->allocated_5a & 4) == 0 || object->gizforce_target == NULL) {
            GizForce_FindBestForceTarget(WORLD->giz_force_sys, object);
        }
        if (object->gizforce_target != NULL) {
            object->context_animation_timer = 0.5f;
            object->character_context = 8;
            NewRumble(object->pad_gamepad->pad, 0.6f, 0);
            object->gizforce_target->using_object = object;
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
            object->force_heading = GizForces_AngleToForce(&object->apiobj.position, object->gizforce_target);
            if ((object->apiobj.flags_low & 0x80) != 0) {
                Hint_SetComplete(0x259);
                if ((object->gizforce_target->config_flags & 0x10) != 0)
                    Hint_SetComplete(0x623);
                LSW_HintConditions |= 1;
            }
        }
    } else if ((object->apiobj.flags_low & 0x80) != 0)
        GizForce_FindBestForceTarget(WORLD->giz_force_sys, object);
    GIZFORCE_s *force = object->gizforce_target;
    if (force != NULL && (object->apiobj.flags_low & 0x80) != 0 && (object->field_0xe22 & 2) == 0 &&
        object->apiobj.field_0x27d != 0) {
        object->field_0xe22 |= 2;
        if (object->gizforce_target_object == NULL) {
            object->force_glow_candidate = force;
            object->force_glow_candidate_kind = 0;
            SetHeadTarget(object, &force->file_position, 1, 2.0f, 5.0f, 10.0f);
        } else {
            object->force_glow_candidate_kind = 1;
            object->force_glow_candidate = object->gizforce_target_object;
            object->field_0xd8c = force->horizontal_range;
            SetHeadTarget(object, NuSpecialGetDrawPos(&object->gizforce_target_object->special), 1, 2.0f, 5.0f, 10.0f);
        }
        SetHeadTarget(object, &force->file_position, 1, 2.0f, 5.0f, 10.0f);
    }
    if (object->character_context == 8 && object->id == id_LUKESKYWALKERDAGOBAH && dagobah_training != 0 &&
        object->field_0xcc0 == NULL && FreePlay == 0) {
        EndForce(object, 0);
        if (AnimPlaying(&object->apiobj.anim_packet, 0xb, 1, 1) == NULL &&
            AnimPlaying(&object->apiobj.anim_packet, 0x27, 1, 1) == NULL) {
            PlaySfx("JForcePush", &object->apiobj.collision_position);
            PlayGruntSfx(object);
        }
        NewBuzzFrames(object->pad_gamepad->pad, 2, 0);
        if (object->gizforce_target != NULL) {
            object->gizforce_target->using_object = NULL;
            object->apiobj.movement_facing_angle =
                GizForces_AngleToForce(&object->apiobj.collision_position, object->gizforce_target);
        }
        object->field_0xe23 |= 1;
        GameCam_HitRoll();
    }
}

i32 FaceOpponent(GameObject_s *object, NUVEC *position) {
    if (position == NULL) {
        GameObject_s *target = object->force_target;
        if (target != NULL) {
            if ((target->apiobj.field_0x1f8 & 0x1001) != 0x1001) {
                return 0;
            }
            position = &target->apiobj.position;
            if (target->apiobj.field_0x287 != 0) {
                return 0;
            }
        } else {
            if (object->blowup_target == NULL) {
                return 0;
            }
            position = &object->blowup_target->mid_position;
        }
    }
    object->apiobj.movement_facing_angle =
        NuAtan2D(position->x - object->apiobj.position.x, position->z - object->apiobj.position.z);
    return 1;
}

i32 ForcePushed_SetTargetMom(GameObject_s *object, float *seek_rate) {
    GameObject_s *source = object->force_target;
    if (source == NULL) {
        object->target_velocity.x = object->target_velocity.z = 0.0f;
        return 0;
    }
    if ((source->action_flags & 0x180) != 0) {
        object->target_velocity.x = object->target_velocity.z = 0.0f;
        if (source->character_context == 0x1b && source->field_0x7a3 == 1) {
            NUVEC target = {source->external_force.x,
                            source->external_force.y - object->character_bottom * object->apiobj.field_0xa8,
                            source->external_force.z};
            NUVEC direction;
            float distance_squared = NuVecDistSqr(&target, &object->apiobj.position, &direction);
            if (!(0.1f * 0.1f > distance_squared) &&
                (!(0.2f * 0.2f > distance_squared) || (NuFsqrt(distance_squared) - 0.1f) / 0.1f > 0.0f)) {
                NuVecNorm(&direction, &direction);
                NuVecScale(&object->target_velocity, &direction, 1.0f);
            }
            *seek_rate = 20.0f;
            return 1;
        }
        return 0;
    }
    if (object->action_movement_state == 4 && Cheat_IsOn(0x13) != 0) {
        u16 angle = NuAtan2D(object->apiobj.pos_x - source->apiobj.pos_x, object->apiobj.pos_z - source->apiobj.pos_z);
        object->target_velocity.x = -NU_SIN_LUT(angle) * 2.5f;
        object->target_velocity.z = -NU_COS_LUT(angle) * 2.5f;
        return 0;
    }
    GameObject_s *nearest = NULL;
    if (ForcePush_SuperPush != 0 && static_cast<i8>(object->apiobj.flags_low) >= 0 &&
        (source->action_flags & 0x380) == 0) {
        float push_x = object->apiobj.position.x - source->apiobj.position.x;
        float push_z = object->apiobj.position.z - source->apiobj.position.z;
        float nearest_distance_squared = 2.25f;
        GameObject_s *candidate = Obj;
        for (i32 index = 0; index < HIGHGAMEOBJECT; ++index, ++candidate) {
            if ((candidate->apiobj.field_0x1f8 & 0x1001) != 0x1001 || candidate->apiobj.field_0x287 != 0 ||
                candidate == source || candidate == object || candidate->apiobj.field_0x27c != -1 ||
                (candidate->field_0xefb & 8) != 0 || CannotKill(candidate) != 0 ||
                (candidate->apiobj.character_data->model_flags & 0x4002010) != 0x10) {
                continue;
            }
            GAMECHARACTERDATA *character =
                static_cast<GAMECHARACTERDATA *>(candidate->apiobj.character_data->field11_0x24);
            if ((character->flags_090 & 0x40) != 0 || (character->flags_094[1] & 2) != 0 ||
                candidate->id == id_GONKDROID || candidate->apiobj.collision_min.y > object->apiobj.collision_max.y ||
                object->apiobj.collision_min.y > candidate->apiobj.collision_max.y) {
                continue;
            }
            float dx = candidate->apiobj.position.x - object->apiobj.position.x;
            float dz = candidate->apiobj.position.z - object->apiobj.position.z;
            if (!(0.0f > push_x * dx + push_z * dz)) {
                float distance_squared = dx * dx + dz * dz;
                if (distance_squared < nearest_distance_squared) {
                    nearest_distance_squared = distance_squared;
                    nearest = candidate;
                }
            }
        }
    }
    float dx;
    float dz;
    if (nearest != NULL) {
        dz = nearest->apiobj.pos_z - object->apiobj.pos_z;
        dx = nearest->apiobj.pos_x - object->apiobj.pos_x;
    } else {
        dz = object->apiobj.pos_z - source->apiobj.pos_z;
        dx = object->apiobj.pos_x - source->apiobj.pos_x;
    }
    u16 angle = NuAtan2D(dx, dz);
    object->target_velocity.x = NU_SIN_LUT(angle) * 2.5f;
    object->target_velocity.z = NU_COS_LUT(angle) * 2.5f;
    return 0;
}

i32 ForcePushed_YRotation(GameObject_s *object) {
    GameObject_s *source = object->force_target;
    if (source != NULL && source->character_context == 0x1b && (source->action_flags & 0x380) == 0 &&
        (object->apiobj.velocity.x != 0.0f || object->apiobj.velocity.z != 0.0f)) {
        object->apiobj.movement_facing_angle = NuAtan2D(object->apiobj.velocity.x, object->apiobj.velocity.z) + 0x8000;
        if (object->action_movement_state == 4 && Cheat_IsOn(0x13) != 0) {
            object->apiobj.movement_facing_angle += 0x8000;
        }
    } else {
        FaceOpponent(object, NULL);
    }
    return 0;
}

i32 ForcePushed_SuperPush_Occurring(GameObject_s *first, GameObject_s *second) {
    if (first->character_context == 0x1c) {
        if (first->action_movement_state != 0) {
            return 0;
        }
        GameObject_s *source = first->force_target;
        if (source == NULL || source->character_context != 0x1b || (source->action_flags & 0x380) != 0 ||
            second->id == id_GONKDROID || (second->apiobj.field_0x1f8 & 0x1001) != 0x1001 ||
            second->apiobj.field_0x287 != 0 || source == second || first == second ||
            second->apiobj.field_0x27c != -1 || (second->field_0xefb & 8) != 0 || CannotKill(second) != 0) {
            return 0;
        }
        CHARACTERDATA *character = second->apiobj.character_data;
        if ((character->model_flags & 0x4002010) != 0x10) {
            return 0;
        }
        GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
        if ((data->flags_090 & 0x40) != 0 || (data->flags_094[1] & 2) != 0) {
            return 0;
        }
        return 1;
    }
    if (second->character_context != 0x1c || second->action_movement_state != 0) {
        return 0;
    }
    GameObject_s *source = second->force_target;
    if (source == NULL || source->character_context != 0x1b || (source->action_flags & 0x380) != 0 ||
        first->id == id_GONKDROID || (first->apiobj.field_0x1f8 & 0x1001) != 0x1001 || first->apiobj.field_0x287 != 0 ||
        first == second || first == source || first->apiobj.field_0x27c != -1 || (first->field_0xefb & 8) != 0 ||
        CannotKill(first) != 0) {
        return 0;
    }
    CHARACTERDATA *character = first->apiobj.character_data;
    if ((character->model_flags & 0x4002010) != 0x10) {
        return 0;
    }
    GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
    if ((data->flags_090 & 0x40) != 0 || (data->flags_094[1] & 2) != 0) {
        return 0;
    }
    return 1;
}

static void ForcePushed_MoveCode(GameObject_s *object) {
    NuFmax(NuFmax(0.4f, 1.2f), 0.8f);
    if (object->character_context != 0x1c)
        return;
    GameObject_s *source = object->force_target;
    if (source == NULL || source->character_context != 0x1b || (source->field_0xe21 & 1) != 0 ||
        (object->action_flags & 0x280) != 0)
        return;
    if (object->action_movement_state == 4 && Cheat_IsOn(0x13)) {
        if ((source->apiobj.field_0x1e4 & object->apiobj.field_0x1ec) != 0 ||
            (source->apiobj.field_0x1e8 & object->apiobj.field_0x1f0) != 0) {
            i16 angle =
                NuAtan2D(source->apiobj.pos_x - object->apiobj.pos_x, source->apiobj.pos_z - object->apiobj.pos_z);
            objhitobj_killparts_yrot = &angle;
            if (ObjHitObj(NULL, object, -1, 0, 0, 1) == 2) {
                NewRumble(source->pad_gamepad->pad, 0.5f, 0);
                NewBuzz(source->pad_gamepad->pad, 0.1f, 0);
                GameCam_HitJudder();
            }
        }
    } else if (ForcePush_SuperPush) {
        GameObject_s *target = Obj;
        for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++target) {
            if ((target->apiobj.field_0x1f8 & 0x1001) != 0x1001 || target->apiobj.field_0x287 != 0 ||
                target == source || target == object || target->apiobj.field_0x27c != -1 ||
                (target->field_0xefb & 8) != 0 || CannotKill(target) ||
                (target->apiobj.character_data->model_flags & 0x4002010) != 0x10 ||
                (static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_090 & 0x40) !=
                    0 ||
                (static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_094[1] & 2) != 0)
                continue;
            if (target->id == id_GONKDROID) {
                if (target->character_context == 0x17)
                    continue;
                if ((target->apiobj.field_0x1e4 & object->apiobj.field_0x1ec) == 0 &&
                    (target->apiobj.field_0x1e8 & object->apiobj.field_0x1f0) == 0)
                    continue;
                DeactivatePlayer(target, DEACTIVATEDTIME, NULL);
            } else {
                if ((target->apiobj.field_0x1e4 & object->apiobj.field_0x1ec) == 0 &&
                    (target->apiobj.field_0x1e8 & object->apiobj.field_0x1f0) == 0)
                    continue;
                i16 angle =
                    NuAtan2D(target->apiobj.pos_x - object->apiobj.pos_x, target->apiobj.pos_z - object->apiobj.pos_z);
                objhitobj_killparts_yrot = &angle;
                if (ObjHitObj(NULL, target, -1, 0, 0, 1) != 2)
                    continue;
                if (static_cast<u8>(source->apiobj.field_0x27c) < 2 && target->apiobj.field_0x27c == -1 && Arcade)
                    Arcade_AIKilled(source->apiobj.field_0x27c);
            }
            NewRumble(source->pad_gamepad->pad, 0.5f, 0);
            NewBuzz(source->pad_gamepad->pad, 0.1f, 0);
            GameCam_HitJudder();
        }
    }
}

i32 ZapTarget(GameObject_s *object) {
    if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0)
        return 0;
    u32 flags = object->apiobj.character_data->model_flags;
    if ((flags & 0x10) == 0)
        return 0;
    void **animations = object->apiobj.character_model->model_data_b;
    if (animations[0x41] == NULL)
        return 0;
    i8 context = object->character_context;
    if ((CInfo[context].flags & 0x8000) != 0)
        return 0;
    if ((flags & 0x20) != 0 && (animations[0x42] == NULL || animations[0x43] == NULL || animations[0x44] == NULL))
        return 0;
    if (context == 0x15)
        return 0;
    if (context == 0x17)
        return 0;
    if (context == 0x41)
        return 0;
    if (context == 0x33)
        return 0;
    if (context == 0x3b)
        return 0;
    return context != 0x39;
}

static void DeactivatedCode(GameObject_s *object) {
    if (object->character_context == 0x41) {
        if (AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0) == NULL)
            return;
        object->airborne_action_duration += FRAMETIME;
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer <= 0.0f)
            object->character_context = -1;
        return;
    }
    if ((object->apiobj.character_data->model_flags & 0x20000000) != 0) {
        bool controlled = false;
        for (i32 i = 0; i < 8; ++i) {
            if (Player[i] != NULL && Player[i]->character_context == 0x51 && Player[i]->field_0x788 != NULL &&
                static_cast<TECHNO *>(Player[i]->field_0x788)->controlled_object == object)
                controlled = true;
        }
        if (controlled || object->field_0xcc0 != NULL || (object->apiobj.flags_low & 0x80) != 0) {
            if (object->character_context == 0x17)
                ActivatePlayer(object);
        } else if (object->character_context != 0x2b && object->character_context != 0x3e) {
            if (object->character_context != 0x17)
                DeactivatePlayer(object, 1.0e9f, NULL);
            else
                object->context_animation_timer = 1.0e9f;
        }
    }
    if (object->character_context != 0x17)
        return;
    if (object->apiobj.character_model->model_data_b[object->context_animation] == NULL) {
        object->airborne_action_duration += FRAMETIME;
    } else {
        if (AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0) == NULL)
            return;
        float elapsed = object->airborne_action_duration + FRAMETIME;
        object->airborne_action_duration = elapsed;
        if (object->context_animation == 0x81) {
            if (AnimDuration(object->id, 0x81, 0.0f, 0.0f, 1) <= elapsed) {
                if (object->apiobj.character_model->model_data_b[0x41] == NULL) {
                    ActivatePlayer(object);
                    return;
                }
                object->context_animation = 0x41;
                if (object->context_animation_timer != 1.0e9f)
                    object->context_animation_timer -= object->airborne_action_duration;
                object->airborne_action_duration = 0.0f;
            }
        } else if ((static_cast<CHARACTERANIM_s *>(
                        object->apiobj.character_model->model_data_a[object->context_animation])
                        ->flags &
                    2) == 0 &&
                   AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1) <= elapsed) {
            ActivatePlayer(object);
            return;
        }
    }
    if (object->context_animation_timer != 1.0e9f && object->context_animation_timer < object->airborne_action_duration)
        ActivatePlayer(object);
    else if ((object->apiobj.character_data->model_flags & 0x20) != 0)
        SetProtocolDroidDeactivatedAction(object);
    if (!(object->field_0x768 > 0.0f))
        return;
    object->field_0x768 -= FRAMETIME;
    if (!(object->field_0x768 <= 0.0f))
        return;
    i32 kind;
    f32 duration;
    f32 range;
    if (object->action_movement_state == 2) {
        kind = 2;
        duration = DEACTIVATEDTIME;
        range = 9.0f;
    } else {
        if (!ForcePush_SuperMindTrick || object->action_movement_state != 1)
            return;
        kind = 1;
        duration = 5.0f;
        range = 1.0f;
    }
    GameObject_s *targets[10];
    i32 count = 0;
    GameObject_s *candidate = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT && count < 10; ++i, ++candidate) {
        GameObject_s *target = candidate;
        if (kind == 1) {
            if ((target->apiobj.field_0x1f8 & 0x1001) != 0x1001 || target->apiobj.field_0x287 != 0 ||
                target->apiobj.field_0x27d == 0 || target == object || target->apiobj.field_0x27c != -1 ||
                (target->field_0xefb & 8) != 0 || CannotKill(target) ||
                target->apiobj.character_model->model_data_b[0x41] == NULL)
                continue;
            i32 context = target->character_context;
            if (context == 0x3c || context == 0x39 || context == 0x3b || context == 0x17 || context == 0x41 ||
                context == 0x0f || context == 0x47 || context == 0x46 ||
                (target->apiobj.character_data->model_flags & 0x10) != 0 ||
                (static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_090 & 0x8000) !=
                    0 ||
                (CInfo[context].flags & 0x8000) != 0 ||
                (static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_094[1] & 2) != 0)
                continue;
        } else {
            if (!ZapTarget(target) || target == object || target->apiobj.field_0x27c != -1 ||
                (target->field_0xefb & 8) != 0 || CannotKill(target) ||
                (static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_090 & 0x8040) !=
                    0 ||
                target->character_context == 0x0f || target->character_context == 0x3c ||
                target->character_context == 0x47 || target->character_context == 0x46)
                continue;
        }
        if (NuVecDistSqr(&target->apiobj.collision_position, &object->apiobj.collision_position, NULL) < range)
            targets[count++] = target;
    }
    i32 selected = count == 1 ? 0 : count > 1 ? qrand() / (0xffff / count + 1) : -1;
    if (selected == -1)
        return;
    GameObject_s *target = targets[selected];
    if (DeactivatePlayer(target, duration, NULL)) {
        target->action_movement_state = kind;
        target->context_variant_flags = (target->context_variant_flags & ~1) | (object->context_variant_flags & 1);
        if (kind == 2) {
            PlaySfx("R2Zap", &object->apiobj.collision_position);
            AddGameDebris(WORLD->debris_sys, (target->context_variant_flags & 1) != 0 ? 1 : 3,
                          &target->apiobj.collision_position);
            GameCam_HitJudder();
        }
        target->field_0x768 = qrand() * 1.5259022e-05f * 0.3f + 0.2f;
        if (target->id == id_JAWA)
            PlaySfx("Jawa_Dizzy", &target->apiobj.collision_position);
    }
}

static void DrawLightningBolts(GameObject_s *, GameObject_s *, i32) {
}

static void ForcePushCode(GameObject_s *object, i32 held, i32) {
    if (object->character_context != 0x1b)
        return;
    AlertSurroundingCreatures(object, &object->apiobj.collision_position);
    GameObject_s *initial_target = object->force_target;
    if ((object->pad_gamepad->allocated_5a & 0x10) != 0)
        held = 1;
    object->field_0xe22 |= 2;
    GameObject_s *target = initial_target;
    if (target != NULL) {
        if (target->character_context == 0x0f) {
            object->force_target = NULL;
            EndForce(object, 0);
            target = object->force_target;
        } else if ((target->apiobj.field_0x1f8 & 0x1001) != 0x1001 || target->apiobj.field_0x287 != 0) {
            object->force_target = NULL;
            target = NULL;
        } else if ((object->field_0xe21 & 6) == 0 && target->character_context != 0x1c) {
            object->force_target = NULL;
            EndForce(object, 0);
            target = object->force_target;
        }
    }
    if (target != NULL) {
        SetObjAsHeadTarget(object, target, 2, 1.0f, 0.0f, 0.0f);
        SetObjAsHeadTarget(target, object, 2, 1.0f, 0.0f, 0.0f);
        if ((object->field_0xe21 & 4) != 0) {
            object->force_glow_candidate = target;
            object->force_glow_candidate_kind = 2;
            if ((object->field_0xe21 & 1) != 0)
                DrawLightningBolts(object, target, 0);
        } else {
            NewRumble(target->pad_gamepad->pad, qrand() * 1.5259022e-05f * 0.4f, 0);
            if ((object->field_0xe21 & 1) != 0)
                DrawLightningBolts(object, target, 1);
            object->force_glow_candidate = target;
            object->force_glow_candidate_kind = 2;
            if (target->character_context != 0x1c || (target->apiobj.character_data->model_flags & 0x20) != 0 ||
                target->apiobj.character_model->model_data_b[target->context_animation] == NULL ||
                CurrentAnim(&target->apiobj.anim_packet) == target->context_animation)
                object->context_animation_timer += FRAMETIME;
            NewRumble(object->pad_gamepad->pad, qrand() * 1.5259022e-05f * 0.4f, 0);
            f32 duration = (object->field_0xe21 & 2) != 0                                          ? 0.4f
                           : ((object->field_0xe20 & 0x80) != 0 || (object->field_0xe21 & 1) != 0) ? 1.2f
                                                                                                   : 0.8f;
            object->field_0x7a3 = 0;
            bool throw_target = false;
            if ((object->apiobj.flags_low & 0x80) != 0 && (object->apiobj.field_0x1f4 & 0x40000) == 0 &&
                ForcePush_Waft && (target->apiobj.flags_low & 0x80) == 0 && object->context_animation_timer >= 0.2f &&
                object->pad_gamepad->input_magnitude > 0.0f &&
                ((object->field_0xe21 & 1) == 0 || (GetGameCharacterData(target)->flags_090 & 0x4000) == 0) &&
                ((object->field_0xe20 & 0x80) != 0 || (object->field_0xe21 & 1) != 0)) {
                object->field_0x7a3 = 1;
                object->external_force.x = NU_SIN_LUT(object->apiobj.facing_angle) * 0.5f + object->apiobj.position.x;
                object->external_force.y = (object->apiobj.collision_max.y - object->apiobj.collision_min.y) * 0.75f +
                                           object->apiobj.collision_min.y;
                object->external_force.z = NU_COS_LUT(object->apiobj.facing_angle) * 0.5f + object->apiobj.position.z;
                u16 angle = NuAtan2D(target->apiobj.position.x - object->apiobj.position.x,
                                     target->apiobj.position.z - object->apiobj.position.z);
                i32 difference = RotDiff(GamePad_InputAngle(object, object->pad_gamepad), angle);
                if ((difference < 0 ? -difference : difference) < 0xaaa)
                    throw_target = true;
                else if (duration - 0.3f <= object->context_animation_timer)
                    object->context_animation_timer = duration - 0.3f;
            }
            if (object->context_animation_timer >= duration ||
                (((object->field_0xe20 & 0x80) == 0 && (object->field_0xe21 & 3) == 0) &&
                 object->context_animation_timer >= 0.2f && initial_target->field_0x1084 != 0 &&
                 NuFabs(initial_target->contact_normal.y) < NU_SIN_LUT(0x6aaa))) {
                if (throw_target) {
                    object->force_target = NULL;
                    object->character_context = -1;
                    object->pad_gamepad->allocated_5a &= ~0x10;
                    target->force_target = NULL;
                    object->field_0xd14 = 0;
                    EndForce(target, 0);
                    PlaySfx("JForcePush", &object->apiobj.collision_position);
                    KillRumble(object);
                    PlayHurtSfx(target);
                    GameCam_NewShake(NULL, 0.75f, 0.75f, 1.0f);
                    target->character_context = 0x5f;
                    if (target->apiobj.character_model->model_data_b[0xb8] != NULL) {
                        target->context_animation = 0xb8;
                        ResetAnimPacket(&target->apiobj.anim_packet, 0xffff);
                    } else if (target->apiobj.character_model->model_data_b[5] != NULL)
                        target->context_animation = 5;
                    target->current_hp = 1;
                    target->context_animation_timer = 0.0f;
                    target->airborne_action_duration = 2.0f;
                    u16 angle = NuAtan2D(target->apiobj.pos_x - object->apiobj.pos_x,
                                         target->apiobj.pos_z - object->apiobj.pos_z);
                    f32 speed = (qrand() * 1.5259022e-05f * 0.4f + 0.8f) * DIEAIRSPEED;
                    target->external_force.x = NU_SIN_LUT(angle) * speed;
                    target->external_force.z = NU_COS_LUT(angle) * speed;
                    target->context_variant_flags &= ~1;
                    target->apiobj.velocity.y = (qrand() * 1.5259022e-05f * 0.4f + 0.8f) * DIEAIRJUMPSPEED;
                    target->hit_variant =
                        static_cast<u8>(object->apiobj.field_0x27c) <= 1 ? object->apiobj.field_0x27c : -1;
                    SetFlicker(target, 0.4f);
                } else {
                    bool deactivate = true;
                    if ((object->field_0xe21 & 2) != 0) {
                        if (DeactivatePlayer(target, 5.0f, NULL)) {
                            target->action_movement_state = 1;
                            target->field_0x768 = qrand() * 1.5259022e-05f * 0.3f + 0.2f;
                            if (target->id == id_JAWA)
                                PlaySfx("Jawa_Dizzy", &target->apiobj.collision_position);
                        }
                    } else if (target->id == id_GONKDROID ||
                               ((target->apiobj.character_data->model_flags & 0x10) != 0 && target->hitpoints >= 2))
                        DeactivatePlayer(target, DEACTIVATEDTIME, NULL);
                    else {
                        bool immune = FreePlay != 0 && (object->field_0xe21 & 1) != 0 &&
                                      (GetGameCharacterData(target)->flags_090 & 0x4000) != 0;
                        i32 damage = immune                             ? 0
                                     : target->apiobj.field_0x27c == -1 ? static_cast<i8>(target->current_hp)
                                                                        : 1;
                        u16 flags =
                            WORLD->current_level == BLOCKADERUNNERB_LDATA ? (object->apiobj.field_0x1f4 >> 10) & 1 : 0;
                        if (ObjHitObj(object, target, damage, flags, 0, 1) != 2 && (object->field_0xe20 & 0x80) != 0 &&
                            !immune)
                            target->fall_animation_timer = 0.2f;
                        if (object->force_target != NULL) {
                            object->force_target->force_target = NULL;
                            EndForce(object->force_target, 0);
                        }
                        object->force_target = NULL;
                        object->pad_gamepad->allocated_5a &= ~0x10;
                        object->field_0xd14 = 0;
                        deactivate = false;
                    }
                    if (deactivate) {
                        NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                        GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
                    }
                }
                object->airborne_action_duration = 0.0f;
                GameCam_HitJudder();
            }
        }
    }
    if (object->airborne_action_duration >= 0.2f && held != 0 && object->force_target != NULL) {
        object->airborne_action_duration =
            object->airborne_action_duration - FRAMETIME > 0.2f ? object->airborne_action_duration - FRAMETIME : 0.2f;
    } else if ((object->apiobj.field_0x1f4 & 0x40000) == 0) {
        object->airborne_action_duration -= FRAMETIME;
        if (object->airborne_action_duration <= 0.0f) {
            target = object->force_target;
            if (object->context_animation_timer < 0.4f) {
                ReleaseForce(object, 0);
                if ((object->field_0xe20 & 0x80) == 0 || target == NULL)
                    return;
            } else {
                if (target == NULL) {
                    ReleaseForce(object, 0);
                    return;
                }
                if ((target->apiobj.field_0x1f8 & 1) != 0 && target->apiobj.field_0x287 == 0 &&
                    (GetGameCharacterData(target)->uses_weapon_action == 4 || target->id == id_BATTLEDROIDSECURITY ||
                     target->id == id_GONKDROID || target->id == id_PKDROID || target->id == id_PITDROID)) {
                    DeactivatePlayer(target, DEACTIVATEDTIME * 0.5f, NULL);
                    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
                }
                ReleaseForce(object, 0);
                if ((object->field_0xe20 & 0x80) == 0)
                    return;
            }
            target->fall_animation_timer = 0.2f;
        }
    }
}

void NewRumbleAllPlayers(f32, f32, i32, i32);
void AddPartDebris(PARTDEBSYS_s *, i32, NUVEC *);

static void ForceThrowCode(GameObject_s *object, i32 pressed, i32) {
    if (object->character_context == 0x12) {
        AlertSurroundingCreatures(object, &object->apiobj.collision_position);
        if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL &&
            CurrentAnim(&object->apiobj.anim_packet) != object->context_animation)
            return;
        f32 previous_time = object->context_animation_timer;
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer <= 0.0f) {
            object->character_context = -1;
            object->gizforce_target = NULL;
            return;
        }
        NewRumble(object->pad_gamepad->pad, qrand() * 1.5259022e-05f * 0.5f, 0);
        PART_s *part = static_cast<PART_s *>(object->field_0x788);
        if (part == NULL) {
            GIZFORCE_s *force = object->gizforce_target;
            if (force != NULL) {
                object->field_0xe22 |= 2;
                if ((object->context_flags & 0x40) == 0) {
                    if (object->gizforce_target_object == NULL) {
                        object->force_glow_candidate = force;
                        object->force_glow_candidate_kind = 0;
                        SetHeadTarget(object, &force->file_position, 1, 2.0f, 5.0f, 10.0f);
                    } else {
                        object->force_glow_candidate = object->gizforce_target_object;
                        object->force_glow_candidate_kind = 1;
                        object->field_0xd8c = force->horizontal_range;
                        SetHeadTarget(object, NuSpecialGetDrawPos(&object->gizforce_target_object->special), 1, 2.0f,
                                      5.0f, 10.0f);
                    }
                    SetHeadTarget(object, &force->file_position, 1, 2.0f, 5.0f, 10.0f);
                    object->force_heading = GizForces_AngleToForce(&object->apiobj.position, object->gizforce_target);
                } else if (force->anim_set != NULL && force->anim_set->object_count == 1) {
                    part = Part_FindFromHSpecial(&force->anim_set->objects->special);
                    if (part != NULL) {
                        object->force_glow_candidate = part;
                        object->force_glow_candidate_kind = 3;
                        SetHeadTarget(object, &part->position, 2, 1.0f, 0.0f, 0.0f);
                        object->force_heading = NuAtan2D(part->position.x - object->apiobj.position.x,
                                                         part->position.z - object->apiobj.position.z);
                        object->force_glow_object = object->force_glow_candidate;
                        object->force_glow_kind = object->force_glow_candidate_kind;
                    }
                }
            }
        } else if ((part->active & 1) == 0)
            object->character_context = -1;
        else {
            object->force_heading =
                NuAtan2D(part->position.x - object->apiobj.position.x, part->position.z - object->apiobj.position.z);
            SetHeadTarget(object, &part->position, 2, 1.0f, 0.0f, 0.0f);
        }
        if (previous_time <= 1.0f || object->context_animation_timer > 1.0f)
            return;
        f32 speed, gravity;
        if (WORLD->current_level == MAULF_LDATA) {
            speed = MaulF_ForceThrowSpeed;
            gravity = MaulF_ForceThrowGravity;
        } else if (WORLD->current_level == DOOKUC_LDATA) {
            speed = DookuC_ForceThrowSpeed;
            gravity = DookuC_ForceThrowGravity;
        } else {
            speed = ForceThrowSpeed;
            gravity = ForceThrowGravity;
        }
        object->field_0x788 = GizForce_Throw(object, object->gizforce_target, speed, gravity, 0);
        object->context_flags |= 0x40;
        return;
    }
    if (!pressed && (object->pad_gamepad->allocated_5a & 4) == 0)
        return;
    if ((object->apiobj.field_0x1f8 & 0x180) == 0x80)
        GizForce_FindBestForceTarget(WORLD->giz_force_sys, object);
    if (object->gizforce_target == NULL || (object->gizforce_target->config_flags & 0x80) == 0)
        return;
    object->character_context = 0x12;
    object->context_animation = 0x27;
    if (object->apiobj.character_model->model_data_b[0x27] == NULL)
        object->context_animation = 0xb;
    object->context_flags &= ~0x40;
    object->field_0x788 = NULL;
    object->force_throw_target = NULL;
    object->context_animation_timer = 2.0f;
    f32 distance = 1.0e9f;
    for (i32 i = 0; i < 8; ++i) {
        GameObject_s *target = Player[i];
        if (target == NULL || (target->apiobj.field_0x1f8 & 0x1001) != 0x1001 || target->apiobj.field_0x287 != 0 ||
            (target->apiobj.character_data->model_flags & 0x80000) != 0)
            continue;
        f32 next_distance = NuVecDistSqr(&object->apiobj.position, &target->apiobj.position, NULL);
        if (object->force_throw_target == NULL ||
            (((object->apiobj.flags_low & 0x80) != 0 || (object->force_throw_target->apiobj.flags_low & 0x80) == 0) &&
             next_distance < distance)) {
            object->force_throw_target = target;
            distance = next_distance;
        }
    }
    NewRumble(object->pad_gamepad->pad, 0.6f, 0);
    object->gizforce_target->using_object = object;
    GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
    object->force_heading = GizForces_AngleToForce(&object->apiobj.position, object->gizforce_target);
}

static void ForceDeflectCode(GameObject_s *object, i32 pressed, i32 held, i32) {
    i32 waiting = objInNetWaitContext(object, 0x1d);
    if (object->character_context == 0x1d) {
        object->field_0xe22 |= 2;
        object->context_animation_timer += FRAMETIME;
        if ((object->apiobj.flags_low & 0x80) != 0)
            NewRumble(object->pad_gamepad->pad, qrand() * 1.5259022e-05f * 0.6f, 0);
        if (object->force_part != NULL) {
            PART_s *part = object->force_part;
            part->force_flags |= 1;
            part->flags &= ~0x4000;
            if (object->context_animation_timer < 0.2f)
                part->velocity.y = SeekValF(part->velocity.y, 2.0f, 6.0f);
            part = object->force_part;
            NUVEC direction;
            if (part->owner == NULL || part->owner->apiobj.field_0x27c != -1)
                NuVecRotateY(&direction, &v001, object->apiobj.facing_angle);
            else {
                NuVecSub(&direction, &part->owner->apiobj.collision_position, &part->position);
                NuVecNorm(&direction, &direction);
            }
            NUVEC velocity;
            NuVecScale(&velocity, &direction, 4.0f);
            SeekVec(&object->force_part->velocity, &object->force_part->velocity, &velocity, 3.0f);
            object->force_glow_candidate = object->force_part;
            object->force_glow_candidate_kind = 3;
            SetHeadTarget(object, &object->force_part->position, 2, 1.0f, 0.0f, 0.0f);
            part = object->force_part;
            if (part->move_callback == BobaRocket_Move) {
                part->move_callback = NULL;
                part->update_callback = NULL;
                part->flags = (part->flags & ~0x4000) | 0x80;
                NewPartRotation(part);
            } else if (part->move_callback == Boulder_Move && object->context_animation_timer >= 0.5f)
                Boulder_Kill(part, 0);
        }
        if ((held != 0 || (object->field_0xefd & 1) != 0) && object->force_part != NULL &&
            object->airborne_action_duration >= 0.2f && object->context_animation_timer < 1.5f) {
            object->airborne_action_duration = object->airborne_action_duration - FRAMETIME > 0.2f
                                                   ? object->airborne_action_duration - FRAMETIME
                                                   : 0.2f;
            return;
        }
        object->airborne_action_duration -= FRAMETIME;
        if (object->airborne_action_duration <= 0.0f)
            ReleaseForce(object, 0);
        return;
    }
    PART_s *assist = NULL;
    if ((object->apiobj.flags_low & 0x80) == 0) {
        if ((object->field_0xefd & 1) == 0 || player == NULL || player->force_part == NULL)
            return;
        assist = player->force_part;
    }
    if (object->apiobj.field_0x27d == 0)
        return;
    i32 context = object->character_context;
    if (context != -1 && (CInfo[context].flags & 4) == 0 && context != 6 && context != 7 &&
        !objInNetWaitContext(object, 0x1d))
        return;
    PART_s *part;
    if (waiting)
        part = object->force_part;
    else if (assist != NULL)
        part = assist;
    else {
        f32 range = TouchHacks::GetIncomingPartRange();
        f32 radius = object->apiobj.field_0x1e0;
        if (radius <= object->apiobj.field_0x1dc)
            radius = object->apiobj.field_0x1dc;
        part = FindIncomingPart(object, &object->apiobj.collision_position, radius, 0x800a, range);
    }
    if (part == NULL)
        return;
    object->field_0xe22 |= 2;
    object->force_glow_candidate = part;
    object->force_glow_candidate_kind = 3;
    SetHeadTarget(object, &part->position, 2, 1.0f, 0.0f, 0.0f);
    if (assist == NULL && pressed == 0 && !waiting)
        return;
    object->force_part = part;
    part->flags |= 0x10000;
    part->gravity = 0.0f;
    object->context_animation_timer = 0.0f;
    object->airborne_action_duration = 0.3f;
    object->force_target = NULL;
    object->blowup_target = NULL;
    u8 mask = 1u << (object->apiobj.field_0x27c & 31);
    object->character_context = 0x1d;
    if (part->force_player_mask != -1)
        mask |= part->force_player_mask;
    part->force_player_mask = mask;
    GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
    PlaySfx("JForcePush", &object->apiobj.collision_position);
}

void Move_WEIRDO(GameObject_s *object) {
    GAMEPAD_s *pad = object->pad_gamepad;
    u32 action_held = pad->buttons_held & GAMEPAD_ACTION;
    u32 action_pressed = pad->buttons_pressed & GAMEPAD_ACTION;
    u32 jump_held = pad->buttons_held & GAMEPAD_JUMP;
    u32 jump_pressed = pad->buttons_pressed & GAMEPAD_JUMP;
    u32 special_held = pad->buttons_held & GAMEPAD_SPECIAL;
    u32 special_pressed = pad->buttons_pressed & GAMEPAD_SPECIAL;
    u32 tag_pressed = pad->buttons_pressed & GAMEPAD_TAG;
    i32 hit_effect = object->blade_index == -1 ? -1 : BladeTab[object->blade_index].hit_effect;
    i32 glow_model;
    if (SuperWeirdo(object) && (object->apiobj.character_data->model_flags & 8) == 0)
        glow_model = 0xdf;
    else
        glow_model = object->blade_index == -1 ? -1 : BladeTab[object->blade_index].glow_model;
    DropInOutCode(object);
    ApplyGravity(object, NULL, 0.0f, 8.0f, NULL);
    u16 weapon = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->weapon_model & 0xfffd;
    i32 has_sabre = weapon == 0x65 || weapon == 0x69;
    if (object->suit != NULL)
        Signal_MoveCode(WORLD, object);
    TakeHitCode(object);
    FloatCode(object);
    SlideCode(object);
    FlattenCode(object);
    Hang_MoveCode(object);
    Ledge_MoveCode(WORLD, object);
    if (LedgeTerrain_On)
        LedgeTerrain_MoveCode(object);
    Climb_MoveCode(object);
    TightRope_MoveCode(object, jump_pressed);
    ForcePushed_MoveCode(object);
    ForcedBackCode(object);
    Tube_MoveCode(object, WORLD);
    DeactivatedCode(object);
    PushCode(object, 1);
    BackFlipCode(object);
    TakeOverCode(object, tag_pressed);
    Glide_MoveCode(object);
    u32 jump_animations;
    if (!has_sabre) {
        hit_effect = -1;
        jump_animations = SuperWeirdo(object) ? 0x109 : 0x208;
    } else
        jump_animations = SuperWeirdo(object) ? 0x11b : 0x1b;
    JumpCode(object, jump_pressed, jump_held, jump_animations, action_pressed, action_held, hit_effect);
    GizPanel_MoveCode(WORLD, object, special_pressed);
    HatMachine_MoveCode(WORLD, object, special_pressed);
    ZipUp_MoveCode(object, special_pressed);
    if ((object->apiobj.character_data->model_flags & 0x100000) != 0)
        Grapple_MoveCode(object);
    BuildIt_MoveCode(object);
    if (has_sabre || SuperWeirdo(object)) {
        ForceDeflectCode(object, special_pressed, special_held, 0);
        ForcePushCode(object, special_held, 0);
        FindForcePushTarget(object, special_pressed, 1);
        ForceThrowCode(object, special_pressed, 0);
        ForceCode(object, special_pressed, special_held, 0);
        FindForcePushTarget(object, special_pressed, 2);
        if (object->apiobj.field_0x287 == 0 && FadeSys.fade == 0.0f) {
            if (object->force_glow_step > 0.0f)
                object->force_glow_step -= FRAMETIME;
            else
                ForceGlowCode(object, glow_model);
        } else
            ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    }
    ForcePushed_MoveCode(object);
    Lever_MoveCode(WORLD, object);
    if ((object->apiobj.character_data->model_flags & 0x40000) != 0 || SuperWeirdo(object))
        Teleport_MoveCode(object, special_pressed);
    i32 detonator_used = ThermalDetonator_MoveCode(object);
    SuperCarry_MoveCode(WORLD, object);
    GizmoBlowupCheckProximity(WORLD, object);
    ComboRotateCode(object, action_held);
    WeaponOutCode(object);
    WeaponInCode(object);
    WeaponScalingCode(object);
    SpecialMove_VictimCode(object);
    HoldCode(object);
    Attracto_MoveCode(WORLD, object);
    SecurityDoor_MoveCode(WORLD, object);
    if (has_sabre) {
        BlockCode(object, action_pressed, action_held, jump_pressed, 0);
        SwipeCode(object, action_pressed, action_held);
        if (detonator_used == 1)
            special_pressed = 0;
        LightSabreComboCode(object, action_pressed, action_held, jump_pressed, special_pressed);
        JediLightCode(object);
    } else {
        BlockCode(object, action_pressed, action_held, 0, 0);
        PunchCode(object, action_pressed, action_held, 1, 0, 0.0f);
        DodgeCode(object, action_pressed, jump_pressed);
        if ((object->apiobj.character_data->model_flags & 0x80) != 0) {
            if (detonator_used == 1)
                special_pressed = 0;
            ShootCode(object, action_pressed, special_pressed, 1, 1, 0);
        } else if (object->batarang != NULL)
            Batarang_MoveCode(object);
    }
    if (static_cast<i8>(static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_094[0]) <
        0) {
        CommunicateCode(object, GAMEPAD_SPECIAL & object->pad_gamepad->buttons_pressed, 0);
    }
    if (object->character_context == 0x14) {
        f32 *frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
        if (frame != NULL) {
            if ((object->field_0xe22 & 4) == 0) {
                f32 event = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                if (event >= 1.0f && event <= *frame) {
                    object->field_0xe22 |= 4;
                    object->field_0xe20 |= 0x10;
                    PlaySfx("JgRocket", &object->apiobj.collision_position);
                }
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f)
                object->character_context = -1;
        }
    } else if (object->character_context == -1 && (object->pad_gamepad->buttons_pressed & GAMEPAD_TAG) != 0 &&
               ((object->apiobj.character_data->model_flags & 0x1000000) != 0 || object->field_0x108e == 6)) {
        FireBountyHunterRocket(object);
    }
    if (object->character_context == -1 && object->field_0xe31 == 0 && object->apiobj.field_0x27d != 0 &&
        object->apiobj.field_0x27e == 0 &&
        ((object->movement_runtime_flags & 4) != 0 ||
         (object->fall_animation_timer >= 0.2f &&
          (object->pad_gamepad->input_magnitude == 0.0f ||
           ((object->apiobj.flags_low & 0x80) == 0 && object->apiobj.character_model->model_data_b[0x59] != NULL)))))
        StartFallLand(object, -1);
    if (object->apiobj.field_0x27d != 0)
        object->movement_runtime_flags &= ~4;
    HeadMovement(object);
    CloakMovement(object);
    HairMovement(object);
}

void Move_JEDI(GameObject_s *object) {
    const u32 action_mask = GAMEPAD_ACTION;
    GAMEPAD_s *pad = object->pad_gamepad;
    const u32 pressed = pad->buttons_pressed;
    const u32 held = pad->buttons_held;
    const u32 jump_mask = GAMEPAD_JUMP;
    const u32 special_mask = GAMEPAD_SPECIAL;

    if (object->id == id_BODYGUARD) {
        KeepWeaponOut(object);
    }
    DropInOutCode(object);
    if ((object->field_0xe20 & GAMEOBJECT_E20_FLAG_MOVEMENT_DISABLED) != 0) {
        return;
    }

    ApplyGravity(object, NULL, 0.0f, 8.0f, NULL);
    i32 glow_model;
    i32 hit_effect;
    if (object->id == id_BOB) {
        if ((object->field_0xefd & 2) != 0) {
            glow_model = 0xdd;
            hit_effect = 2;
        } else {
            glow_model = 0xe1;
            hit_effect = 3;
        }
    } else if (AnakinGreenSabre(object)) {
        glow_model = 0xdd;
        hit_effect = 2;
    } else {
        hit_effect = object->blade_index == -1 ? -1 : BladeTab[object->blade_index].hit_effect;
        if (SuperWeirdo(object) && (object->apiobj.character_data->model_flags & 8) == 0) {
            glow_model = 0xdf;
        } else {
            glow_model = object->blade_index == -1 ? -1 : BladeTab[object->blade_index].glow_model;
        }
    }
    if (object->suit != NULL) {
        Signal_MoveCode(WORLD, object);
    }
    TakeHitCode(object);
    FloatCode(object);
    SlideCode(object);
    FlattenCode(object);
    Hang_MoveCode(object);
    Ledge_MoveCode(WORLD, object);
    if (LedgeTerrain_On != 0) {
        LedgeTerrain_MoveCode(object);
    }
    const i32 jump_pressed = pressed & jump_mask;
    Climb_MoveCode(object);
    TightRope_MoveCode(object, jump_pressed);
    ForcePushed_MoveCode(object);
    ForcedBackCode(object);
    Tube_MoveCode(object, WORLD);
    DeactivatedCode(object);
    PushCode(object, 1);
    BackFlipCode(object);
    if (object->id != id_GRIEVOUS && object->id != id_BODYGUARD) {
        TakeOverCode(object, object->pad_gamepad->buttons_pressed & GAMEPAD_TAG);
    }
    Glide_MoveCode(object);

    u32 jump_animations;
    if (object->id == id_IMPERIALGUARD)
        jump_animations = 0x10;
    else if (object->id == id_BODYGUARD)
        jump_animations = 0x10;
    else
        jump_animations = 0x1b;
    GAMECHARACTERDATA *game_character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    if ((game_character->flags_090 & 0x00400000) != 0) {
        jump_animations |= 0x100;
    }
    const i32 action_pressed = pressed & action_mask;
    const i32 action_held = held & action_mask;
    const i32 special_pressed = pressed & special_mask;
    JumpCode(object, jump_pressed, held & jump_mask, jump_animations, action_pressed, action_held, hit_effect);

    GizPanel_MoveCode(WORLD, object, special_pressed);
    HatMachine_MoveCode(WORLD, object, special_pressed);
    ZipUp_MoveCode(object, special_pressed);
    BuildIt_MoveCode(object);
    Lever_MoveCode(WORLD, object);
    if ((static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_094[1] & 0x80) == 0) {
        ForceDeflectCode(object, special_pressed, held & special_mask, 0);
        ForcePushCode(object, held & special_mask, 0);
        FindForcePushTarget(object, special_pressed, 1);
        ForceThrowCode(object, special_pressed, 0);
        ForceCode(object, special_pressed, held & special_mask, 0);
        FindForcePushTarget(object, special_pressed, 2);
        if (object->apiobj.field_0x287 == 0 && FadeSys.fade == 0.0f) {
            if (object->force_glow_step > 0.0f)
                object->force_glow_step -= FRAMETIME;
            else
                ForceGlowCode(object, glow_model);
        } else
            ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    }
    if ((object->apiobj.character_data->model_flags & 0x00040000) != 0) {
        Teleport_MoveCode(object, special_pressed);
    }
    ThermalDetonator_MoveCode(object);
    if (object->suit != NULL && (static_cast<SUIT_s *>(object->suit)->store_flag & 4) != 0)
        Detonator_MoveCode(object);
    ComboRotateCode(object, action_held);
    WeaponOutCode(object);
    WeaponInCode(object);
    WeaponScalingCode(object);
    HoldCode(object);
    BlockCode(object, action_pressed, action_held, jump_pressed, 0);
    SwipeCode(object, action_pressed, action_held);
    LightSabreComboCode(object, action_pressed, action_held, jump_pressed, special_pressed);
    JediLightCode(object);
    if ((static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_090 & 0x8000) != 0 &&
        WORLD->debris_sys->entries[124].effect != -1)
        AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[124].effect, &object->apiobj.collision_position, 6,
                                          FRAMETIME, 0, 0, NULL);
    if (object->character_context == -1 && object->field_0xe31 == 0 && object->apiobj.field_0x27d != 0 &&
        object->apiobj.field_0x27e == 0) {
        if (!(object->fall_animation_timer >= 0.2f)) {
            if ((object->movement_runtime_flags & 4) != 0)
                StartFallLand(object, -1);
        } else if ((object->movement_runtime_flags & 4) != 0 || object->pad_gamepad->input_magnitude == 0.0f ||
                   ((object->apiobj.flags_low & 0x80) == 0 &&
                    object->apiobj.character_model->model_data_b[0x59] != NULL))
            StartFallLand(object, -1);
    }
    if (object->apiobj.field_0x27d != 0)
        object->movement_runtime_flags &= ~4;
    HeadMovement(object);
    CloakMovement(object);
    HairMovement(object);
    f32 force_volume = 0.0f;
    if ((object->character_context == 0x1b && object->field_0x7a3 == 1) || object->character_context == 0x1d ||
        object->character_context == 8) {
        force_volume = 1.0f;
    }
    object->force_use_volume = SeekLinearF(object->force_use_volume, force_volume, FRAMETIME);
    if (object->force_use_volume > 0.0f) {
        PlaySfxAndSetVolume("JForceUse", &object->apiobj.collision_position, object->force_use_volume);
    }
    if ((object->apiobj.flags_low & 0x80) != 0 && force_volume == 1.0f &&
        !Cheat_PowerUpActive(object->apiobj.field_0x27c)) {
        ConstantRumble(object, qrand() * 1.5259022e-05f * 0.5f, object->apiobj.field_0x289 * 0.3f);
    }
}

void MovePlayer_NETWORK(GameObject_s *object) {
    APIOBJECT_s &api = object->apiobj;
    if ((api.character_data->model_flags & 0x2000) != 0)
        object->in_narrow_socket = ObjInNarrowSock(object, WORLD->sock_sys, WORLD->level_idx);

    f32 turn_override = 0.0f;
    if ((api.flags_low & 0x80) != 0 && object->character_context == 0x1b && (object->field_0xe21 & 2) == 0) {
        if (object->field_0x7a3 != 1)
            FaceOpponent(object, NULL);
        turn_override = 0.333f;
    }
    i32 direct_turn = object->character_context == 0x33 ? 1 : object->snap_facing;
    f32 turn_rate;
    if (SuperCarry_Carrying(object))
        turn_rate = object->field_0x768 * 8.0f;
    else if (object->character_context == 0x1c || object->character_context == 0x33)
        turn_rate = 8.0f;
    else if (turn_override != 0.0f)
        turn_rate = turn_override * 8.0f;
    else {
        turn_rate = static_cast<GAMECHARACTERDATA *>(api.character_data->field11_0x24)->turn_rate * 8.0f;
        if (object->id == id_ATAT && WORLD->current_level == SPEEDERCHASEA_LDATA)
            turn_rate += turn_rate;
    }

    if ((object->field_0xefd & 0x80) != 0 && (CInfo[object->character_context].flags & 1) == 0)
        api.movement_facing_angle += 0x8000;
    if ((object->field_0xefd & 4) != 0) {
        api.facing_angle = api.movement_facing_angle;
        api.field_0x276 = api.movement_facing_angle;
    } else if (direct_turn == 0 &&
               (turn_override != 0.0f || (CInfo[object->character_context].parameter & 2) != 0 ||
                (static_cast<GAMECHARACTERDATA *>(api.character_data->field11_0x24)->flags_090 & 0x100) != 0 ||
                SuperCarry_Carrying(object))) {
        if (object->character_context == 0x4b &&
            (static_cast<GAMECHARACTERDATA *>(api.character_data->field11_0x24)->flags_090 & 0x100) == 0)
            turn_rate *= 0.333f;
        api.facing_angle =
            TurnRot(api.facing_angle, api.movement_facing_angle, static_cast<i32>(turn_rate * 3.0f * 16384.0f), NULL);
        api.field_0x276 = SeekRot(api.field_0x276, api.facing_angle, 10.0f);
        api.movement_facing_angle = api.facing_angle;
    } else {
        api.facing_angle = SeekRot(api.facing_angle, api.movement_facing_angle, turn_rate);
        api.field_0x276 = api.facing_angle;
    }
    if (object->torpedo != NULL && (api.field_0x1f8 & 0x1000) != 0 && api.field_0x287 == 0 &&
        (object->field_0xe20 & 0x20) == 0)
        Torpedo_UpdateJobbies(object);
    SpecialMove_VictimCode(object);
    i32 glow_model;
    if (SuperWeirdo(object) && (api.character_data->model_flags & 8) == 0)
        glow_model = 0xdf;
    else {
        glow_model = object->blade_index == -1 ? 0xdf : BladeTab[object->blade_index].glow_model;
        if (glow_model == -1)
            glow_model = 0xdf;
    }
    ForcePushCode(object, 0, 0);
    object->field_0xd8c = 1.0f;
    if (api.field_0x287 == 0 && FadeSys.fade == 0.0f) {
        if (object->force_glow_step > 0.0f)
            object->force_glow_step -= FRAMETIME;
        else
            ForceGlowCode(object, glow_model);
    } else
        ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    if ((object->field_0xe20 & 0x20) == 0)
        TorpedoCode(object, 0, 0.0f);
    PeriscodeCode(object);
    MovePlayer_TWIST(object);
    if (PodLevel(WORLD->area)) {
        Move_POD(object);
        KeepOnScreen(object);
    }
    if (object->id == id_SNAKE) {
        if (object->field_0x10b8 == NULL)
            CreateSnakeBody(object, 0xb);
        if (object->field_0x10b8 != NULL)
            UpdateSnakeBody(object);
    }
    object->field_0xefd &= ~0x40;
    api.field_0x214 = api.field_0x218;
    api.start_position = api.position;
    api.initial_position = api.collision_position;
    api.field_0x27e = api.field_0x27d;
    object->previous_movement_angle = api.field_0x276;
    Teleport_NetMoveCode(object);
    if (api.field_0x27c != -1)
        TractorBeamCode(object);
    api.previous_velocity.x = api.velocity.x;
    api.previous_velocity.y = api.velocity.y;
    api.previous_velocity.z = api.velocity.z;
    object->target_velocity = api.velocity;
    object->field_0xefd &= ~4;
    object->field_0x107a = -1;
    api.field_0x1fa |= 1;
    api.flags_low &= ~4;
    APIObjectVelocities(object);
    GameObjectOrigin(object);
    AddSurfaceRipples(object);
}

void MoveToMarker::BlowUp() {
}

void MoveToMarker::FadeOut() {
}

MoveToMarker::MoveToMarker(MechObjectInterface &) {
}

void MoveToMarker::Process(float) {
}

void MoveToMarker::Render() {
}

extern u8 show_lever_hint;

struct _vuv_s;
static __used__ void MakeWingFormation(_vuv_s *, _vuv_s *, f32, i32) {
}

static __used__ void AtatPart_Stop(PART_s *) {
}

static __used__ void AtatPart_Update(PART_s *) {
}

i32 show_autojump_hint;

i32 Slam_Start(GameObject_s *object, f32 speed) {
    if (LEGOCONTEXT_JUMP == -1)
        return 0;
    if ((object->apiobj.character_data->model_flags & 8) != 0)
        SetWeaponOut(object);
    object->character_context = LEGOCONTEXT_JUMP;
    object->action_movement_state = 4;
    object->context_animation_timer = 0.0f;
    object->context_animation = LEGOACT_SLAM;
    if ((object->apiobj.flags_low & 0x80) != 0)
        object->field_0xef9 |= 8;
    object->jump_sequence = 0;
    object->apiobj.velocity.y = speed;
    PlayJumpSfx(object, 4);
    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
    object->jump_flags |= 1;
    if ((object->apiobj.flags_low & 0x80) != 0)
        Hint_SetComplete(0x60a);
    return 1;
}

void StartLunge(GameObject_s *object, f32 speed, f32 height) {
    object->context_animation_timer = 0.0f;
    object->context_flags &= ~0x80;
    object->character_context = 0;
    object->action_movement_state = 3;
    object->context_animation = 0x1f;
    if ((object->apiobj.flags_low & 0x80) != 0)
        object->field_0xef9 |= 8;
    object->jump_flags |= 1;
    object->apiobj.velocity.y = speed;
    object->jump_sequence = 0;
    if (height > 0.0f)
        MakeJumpReachHeight(object, height, 0);
    PlayJumpSfx(object, 3);
    if ((object->apiobj.character_data->model_flags & 8) != 0)
        SetWeaponOut(object);
}

extern "C" {
    extern i16 id_ATST, id_MINIATST, id_ATST_LOWRES, id_ATAT, id_MINIATAT, id_MINIATTE;
}

i32 CanObjSlide(GameObject_s *object, i32) {
    const i8 surface = object->apiobj.field_0x281;
    if (static_cast<u8>(surface) >= 32)
        return 0;
    if ((TerSurface[surface].flags & 0x400) != 0) {
        if (surface == 5 && CanMagnetClimbFn != NULL) {
            if (CanMagnetClimbFn(object) == 0)
                return 1;
        } else {
            return 1;
        }
    }
    return 0;
}

i32 StartSlide(GameObject_s *object, i32 check_contact) {
    if (VehicleArea || object->ignore_slide_terrain || object->field_0x7a5 == 0x2b || object->field_0x7a5 == 0x1f ||
        object->id == id_ATST || object->id == id_MINIATST || object->id == id_ATST_LOWRES || object->id == id_ATAT ||
        object->id == id_MINIATAT || object->id == id_MINIATTE)
        return 0;
    i32 can_slide = CanObjSlide(object, static_cast<i8>(object->apiobj.field_0x281));
    if (check_contact && object->apiobj.field_0x27d == 0) {
        if (object->field_0x1084 == 0 || can_slide == 0 ||
            !(object->apiobj.collision_position.y > object->contact_position.y))
            return 0;
        can_slide = CanObjSlide(object, static_cast<i8>(object->field_0x6b0));
    }
    if (can_slide == 0)
        return 0;
    object->field_0x7a5 = 0x33;
    object->field_0xe31 = 0;
    object->context_animation = object->apiobj.character_model->model_data_b[106] != NULL ? 106 : 5;
    object->airborne_action_duration = 0.25f;
    return 1;
}

i32 CanStepBack(GameObject_s *object) {
    i8 context = object->character_context;
    if ((CInfo[context].flags & 0x10000) != 0 ||
        (LEGOCONTEXT_JUMP != -1 && LEGOCONTEXT_JUMP == context && object->action_movement_state == 3)) {
        return 1;
    }
    GAMECHARACTERDATA *character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    if ((character->flags_090 & 0x8000) != 0 && LEGOCONTEXT_COMBO != 0 && LEGOCONTEXT_COMBO == context) {
        return 1;
    }
    return 0;
}

void FlattenCode(GameObject_s *) {
}

i32 Glide_Start(GameObject_s *object) {
    if (LEGOCONTEXT_GLIDE == -1)
        return 0;
    object->character_context = 0x4f;
    object->context_animation = 0x93;
    object->context_animation_timer = 0.0f;
    object->airborne_action_duration = 0.4f;
    object->field_0x7a3 = 0;
    object->field_0x788 = NULL;
    return 1;
}

void JetPackCode(GameObject_s *, i32, i32, i32) {
}

float SeekLinearF(float current, float target, float step) {
    if (current > target) {
        current -= step;
        if (current < target) {
            current = target;
        }
    } else if (current < target) {
        current += step;
        if (current > target) {
            current = target;
        }
    }
    return current;
}

void StartLaunch(GameObject_s *object) {
    if (object->field_0x7a5 == 0x2c)
        return;
    Player_ClearContext(object, 1);
    Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    object->field_0x7a5 = 0x2c;
    object->field_0x7a3 = 0;
    object->saved_position = object->launch_origin = object->apiobj.position;
}

static i32 getvehiclehoverheight_hothbattlehack;

float GetVehicleHoverHeight(GameObject_s *object, float *separation_offset) {
    float height = object->hover_height_override;
    LEVELDATA *level = WORLD->current_level;
    if (height == 1.0e9f) {
        height = level->hover_height;
        if (height == 0.0f) {
            height = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field_0x28;
            if (object->id == id_SPEEDERBIKE && level == SPEEDERCHASEA_LDATA && disable_narrow_socks != 0) {
                height *= 0.5f;
            }
        }
    }

    bool separate;
    if (level == SPEEDERCHASEA_LDATA) {
        separate = disable_narrow_socks == 0 && object->apiobj.field_0x27c != -1 && object->movement_spline == NULL &&
                   Player[0] != NULL && Player[0]->takeover_source != NULL && Player[0]->id == id_SPEEDERBIKE &&
                   Player[1] != NULL && Player[1]->takeover_source != NULL && Player[0]->id == Player[1]->id;
    } else {
        separate = static_cast<i8>(object->apiobj.flags_low) < 0 && Player[0] != NULL &&
                   (Player[0]->apiobj.character_data->model_flags & 0x2000) != 0 &&
                   (static_cast<i8>(Player[0]->apiobj.flags_low) < 0 || Player[0]->character_context == 0x24) &&
                   Player[1] != NULL && (Player[1]->apiobj.character_data->model_flags & 0x2000) != 0 &&
                   (static_cast<i8>(Player[1]->apiobj.flags_low) < 0 || Player[1]->character_context == 0x24);
    }
    if (separate) {
        float combined_height = (Player[0]->apiobj.field_0x1e0 + Player[1]->apiobj.field_0x1e0) * 0.6f;
        float combined_radius = Player[0]->apiobj.field_0x1dc + Player[1]->apiobj.field_0x1dc;
        float inner_distance = combined_radius * combined_radius;
        float outer_radius = combined_radius * 3.0f;
        float outer_distance = outer_radius * outer_radius;
        float distance = NuVecXZDistSqr(&Player[0]->apiobj.position, &Player[1]->apiobj.position, NULL);
        float blend;
        if (distance < inner_distance) {
            blend = 1.0f;
        } else if (distance < outer_distance) {
            blend = 1.0f - (distance - inner_distance) / (outer_distance - inner_distance);
        } else {
            blend = 0.0f;
        }
        float offset = blend * combined_height;
        float correction = 0.0f;
        float adjusted_height = height - offset;
        if (adjusted_height < offset) {
            correction = offset - adjusted_height;
        }
        if (Player[0] == object) {
            adjusted_height = height + offset;
            if (separation_offset != NULL) {
                *separation_offset = offset;
            }
        } else if (separation_offset != NULL) {
            *separation_offset = -offset;
        }
        height = adjusted_height + correction;
        level = WORLD->current_level;
    } else if (separation_offset != NULL) {
        *separation_offset = 0.0f;
    }

    i32 hoth_battle = 0;
    if (level == HOTHBATTLEA_LDATA && (TerLayer[static_cast<i8>(object->apiobj.field_0x27f)].flags & 1) != 0) {
        height = 0.0f;
        hoth_battle = 1;
    }
    getvehiclehoverheight_hothbattlehack = hoth_battle;
    return height;
}

static __used__ i32 IsAFallAnim(i32 animation) {
    if (animation == 5)
        return 1;
    if (animation == 0x28)
        return 1;
    if (animation == 0x4b)
        return 1;
    if (animation == 0x4c)
        return 1;
    return animation == 0x74;
}

void ApplyGravity(GameObject_s *object, float *gravity, float hover_height, float seek_rate, float *ground_height) {
    float extra_hover_offset = applygravity_extrahoveroffset;
    applygravity_extrahoveroffset = 0.0f;
    if (object->movement_spline != NULL) {
        return;
    }
    i32 flags = object->apiobj.flags_low;
    if ((flags & 0x20) != 0 || object->move_override != NULL) {
        return;
    }
    i32 context = object->character_context;
    float acceleration;
    if ((CInfo[context].flags & 0x400) != 0) {
        if (context != 0x44 || static_cast<u16>(object->context_animation - 5) > 1) {
            return;
        }
    } else if (context == 0x5f) {
        if ((object->context_variant_flags & 1) != 0) {
            return;
        }
    } else if (context == 0x4b) {
        if (object->suit == NULL || (static_cast<SUIT_s *>(object->suit)->store_flag & 0x10) == 0) {
            return;
        }
        acceleration = -1.0f;
        gravity = &acceleration;
    }
    if ((flags & 4) != 0 || static_cast<i8>(object->field_0xefb) < 0 || context == 0x2c) {
        return;
    }
    if ((object->field_0xe20 & 0x20) != 0) {
        object->apiobj.velocity.y = 0.0f;
        return;
    }
    CHARACTERDATA *character = object->apiobj.character_data;
    if ((character->model_flags & 0x2000) == 0 && static_cast<u8>(context - 0x23) < 2) {
        object->apiobj.velocity.y = 0.0f;
        return;
    }
    WORLDINFO_s *world = WORLD;
    if ((WORLD->current_level->flags & 0x40000) != 0 && gravity == NULL &&
        static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->gravity == 0.0f) {
        return;
    }
    if (object->field_0x1086 == 4) {
        return;
    }

    float target_ground_height;
    if (context == 0x1e || context == 0x22 || context == 0x42) {
        hover_height = 0.2f;
        seek_rate = 8.0f;
    } else if (context == 0x1c) {
        GameObject_s *target = object->force_target;
        if (target != NULL && (target->action_flags & 0x180) != 0) {
            if (target->field_0x7a3 == 1) {
                return;
            }
            if ((target->field_0xe21 & 4) == 0 &&
                ((target->field_0xe21 & 1) == 0 ||
                 (static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->flags_090 & 0x4000) == 0)) {
                target_ground_height = target->apiobj.lower_position.y;
                ground_height = &target_ground_height;
                hover_height = (target->apiobj.upper_position.y - target_ground_height) * 0.75f;
                seek_rate = 8.0f;
            }
        }
    } else if (context == 0x5d && object->field_0x768 != 1.0e9f) {
        float target_velocity = (object->apiobj.position.y - object->field_0x768) *
                                static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->gravity * 0.333f;
        object->apiobj.velocity.y = SeekValF(object->apiobj.velocity.y, target_velocity, 8.0f);
        return;
    }

    float height = object->hover_height_override == 1.0e9f ? hover_height : object->hover_height_override;
    bool falling_hover = false;
    if (height != 0.0f && object->fall_recovery_timer > 0.0f && WORLD->current_level != E2VEHICLEBONUSA_LDATA) {
        if ((GUNSHIP_ADATA != NULL && GUNSHIP_ADATA == WORLD->area) ||
            (BONUS_GUNSHIP_ADATA != NULL && BONUS_GUNSHIP_ADATA == WORLD->area)) {
            height = 0.0f;
        } else if (static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->field275_0x116 == 2 &&
                   static_cast<i8>(flags) < 0) {
            if (object->fall_hover_height != 2.0e6f) {
                falling_hover = true;
                if (object->fall_recovery_timer >= 0.6f) {
                    object->fall_hover_height -= FRAMETIME * 3.0f;
                }
            }
        } else {
            height -= object->fall_recovery_timer * 0.5f;
        }
    }
    if (context != 0x33) {
        if ((object->apiobj.field_0x27d & 1) != 0 && object->field_0x1084 == 0 && object->apiobj.velocity.y == 0.0f &&
            object->surface_normal.y > NuTrigTable[0x21c7]) {
            object->apiobj.field_0x27d &= ~1;
        }
        if (height == 0.0f && (object->apiobj.field_0x27d & 1) != 0) {
            object->apiobj.velocity.y = 0.0f;
            return;
        }
    }

    bool hold_jump = false;
    if (gravity != NULL) {
        acceleration = *gravity;
    } else {
        if (object->apiobj.field_0x218 != 2.0e6f && height != 0.0f && context != 0x2b) {
            if (falling_hover) {
                object->terrain_origin_floor_offset = 0.0f;
                float target_velocity = (object->apiobj.position.y - object->fall_hover_height) *
                                        static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->gravity * 0.333f;
                object->apiobj.velocity.y = SeekValF(object->apiobj.velocity.y, target_velocity, seek_rate);
                return;
            }
            if ((object->apiobj.field_0x27f == 0x10 || object->apiobj.field_0x27f == 7) &&
                (VehicleArea != 0 ||
                 (static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->flags_090 & 0x40) == 0 ||
                 object->takeover_source != NULL)) {
                float separation;
                GetVehicleHoverHeight(object, &separation);
                separation += object->apiobj.water_height;
                if (object->hover_height_override != 1.0e9f) {
                    separation += object->hover_height_override;
                }
                float offset = VehicleTurnOrLoopOffset(object);
                float target_velocity;
                if (object->apiobj.model_draw_result != 0 || FRAMETIME < 0.016666668f) {
                    target_velocity =
                        (object->apiobj.collision_position.y - (offset + separation)) *
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->gravity * 0.333f;
                } else {
                    target_velocity =
                        ((object->apiobj.collision_position.y - (offset + separation)) *
                         static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->gravity) /
                        (FRAMETIME * 180.0f);
                }
                object->apiobj.velocity.y = SeekValF(object->apiobj.velocity.y, target_velocity, seek_rate);
                return;
            }
            if (PODSPRINT_ADATA != NULL && PODSPRINT_ADATA == world->area && static_cast<i8>(flags) < 0) {
                TUBE *tube = Tube_InAnyCylinder(world, object, 1);
                if (tube != NULL) {
                    ground_height = &tube->top;
                }
            }
            float floor =
                ground_height != NULL && *ground_height != 2.0e6f ? *ground_height : object->apiobj.field_0x218;
            if (object->apiobj.water_height != 2.0e6f && object->apiobj.water_height > floor &&
                object->apiobj.field_0x27f != 5 && object->apiobj.field_0x27f != 7 &&
                object->apiobj.field_0x27f != 0x10) {
                floor = object->apiobj.water_height;
            }
            floor += extra_hover_offset;
            float offset = VehicleTurnOrLoopOffset(object);
            character = object->apiobj.character_data;
            floor += offset;
            height += offset;
            GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
            if ((data->flags_094[0] & 0x20) != 0 || (PODSPRINT_ADATA != NULL && PODSPRINT_ADATA == WORLD->area) ||
                !(GamePlayTimer.time_elapsed >= 1.0f) || !(object->apiobj.collision_origin.y > height * 5.0f + floor)) {
                object->terrain_origin_floor_offset = height;
                float pull = PODSPRINT_ADATA != NULL && PODSPRINT_ADATA == WORLD->area ? -35.0f : data->gravity;
                float target_velocity;
                if (object->apiobj.model_draw_result != 0 || FRAMETIME < 0.016666668f) {
                    target_velocity = (object->apiobj.collision_origin.y - (floor + height)) * pull * 0.333f;
                } else {
                    target_velocity =
                        ((object->apiobj.collision_origin.y - (floor + height)) * pull) / (FRAMETIME * 180.0f);
                }
                object->apiobj.velocity.y = SeekValF(object->apiobj.velocity.y, target_velocity, seek_rate);
                return;
            }
        }

        bool default_gravity = true;
        if ((object->apiobj.field_0x218 == 2.0e6f || height == 0.0f) && context == 0) {
            if (object->action_movement_state == 3) {
                acceleration = -10.0f;
                default_gravity = false;
            } else if (object->action_movement_state == 4) {
                acceleration = SLAMGRAVITY;
                default_gravity = false;
            } else if (object->action_movement_state == 5) {
                if (object->apiobj.field_0x281 == 0x1a) {
                    object->field_0xe31 = 3;
                    object->jump_variant_timer = 0.0f;
                } else if ((object->pad_gamepad->buttons_held & GAMEPAD_JUMP) == 0) {
                    if (object->field_0xe31 == 2) {
                        object->field_0xe31 = 3;
                    }
                    object->jump_variant_timer = 0.0f;
                } else if (object->field_0xe31 == 3) {
                    acceleration = -1.0f;
                    default_gravity = false;
                } else if (object->field_0xe31 == 2) {
                    object->apiobj.velocity.y = 0.0f;
                    return;
                } else {
                    hold_jump = true;
                }
            }
        }
        if (default_gravity) {
            acceleration = static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->gravity;
            if (object->character_context == 0x2b) {
                if (object->field_0xe36 == 3) {
                    acceleration = -acceleration;
                } else if (object->field_0xe36 == 2) {
                    acceleration = -0.25f;
                } else if (object->field_0xe36 == 4) {
                    acceleration = -2.0f;
                }
            }
        }
        float middle = (object->apiobj.position.y - object->character_bottom * object->apiobj.field_0xa8) +
                       (object->character_top - object->character_bottom) * 0.5f * object->apiobj.field_0xa8;
        if ((object->apiobj.field_0x27d == 0 && object->field_0x1084 != 0 && object->contact_position.y < middle &&
             object->apiobj.anim_packet.blending == 0 &&
             object->apiobj.character_model->model_data_b[object->apiobj.anim_packet.animation_index] != NULL &&
             IsAFallAnim(object->apiobj.anim_packet.animation_index) != 0) ||
            object->fall_acceleration_timer > 0.0f) {
            acceleration += acceleration;
            if (acceleration > -5.0f) {
                acceleration = -5.0f;
            }
        }
    }
    float previous_velocity = object->apiobj.velocity.y;
    if ((object->apiobj.field_0x27d & 0xfd) == 0 || object->id == id_DRAGBOMB) {
        object->apiobj.velocity.y = acceleration * FRAMETIME + previous_velocity;
    }
    if (object->character_context == 0x2b) {
        if (object->field_0xe36 == 3) {
            if (object->apiobj.velocity.y < 0.1f) {
                object->apiobj.velocity.y = 0.1f;
            }
        } else if (object->apiobj.velocity.y > -0.1f) {
            object->apiobj.velocity.y = -0.1f;
        }
    }
    if (hold_jump && previous_velocity > 0.0f && object->apiobj.velocity.y <= 0.0f) {
        float duration = static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->field_0x48;
        if (duration > 0.0f) {
            object->apiobj.velocity.y = 0.0f;
            object->jump_variant_timer = (TouchHacks::TouchControlsActive ? 1.3f : 1.0f) * duration;
            if (static_cast<i8>(object->apiobj.flags_low) >= 0) {
                object->jump_variant_timer *= 1.5f;
            }
            object->field_0xe31 = 2;
        } else {
            object->field_0xe31 = 3;
        }
    }
}

i32 SetObjTarget(GameObject_s *object, GameObject_s *target) {
    object->attack_target_position = target->apiobj.collision_position;
    if ((object->apiobj.character_data->model_flags & 0x2000) == 0 && (object->apiobj.field_0x1f4 & 1) != 0)
        object->attack_target_position.y += Bolt_ObjTargetPosYAdjust(target);
    object->attack_target_velocity = v000;
    object->field_0xe21 |= 8;
    object->collision_target = VehicleArea != 0 ? NULL : target;
    return 1;
}

i32 SnapPosTaken(WORLDINFO_s *world, pushblock_s *, nuvec_s *position, i32 excluded_index) {
    for (i32 i = 0; i < world->push_block_count; ++i) {
        if (i == excluded_index) {
            continue;
        }
        pushblock_s *block = &world->push_blocks[i];
        if ((block->packed_state_flags & 0x02000300) == 0) {
            continue;
        }
        const f32 dx = block->position->x - position->x;
        const f32 dy = block->position->y - position->y;
        const f32 dz = block->position->z - position->z;
        if (dx * dx + dy * dy + dz * dz <= 0.0025f) {
            return 1;
        }
    }
    return 0;
}

void StartFlatten(GameObject_s *source, GameObject_s *target) {
    if (target->character_context == 0x33) {
        return;
    }
    if (target->character_context == 0x3d) {
        return;
    }
    GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24);
    if ((data->flags_090 & 0x40) != 0) {
        return;
    }
    if (data->field_0x28 != 0.0f && target->field_0xe31 == 1) {
        return;
    }
    if ((source->apiobj.field_0x1f8 & 4) != 0) {
        return;
    }
    if (target->apiobj.field_0x27d == 0) {
        return;
    }

    Player_ClearContext(target, 0);
    Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(target->player_packet));
    target->character_context = 0x3d;
    target->context_animation_timer = 0.0f;
    target->apiobj.velocity.x *= 0.5f;
    target->apiobj.velocity.z *= 0.5f;
    target->context_animation = target->apiobj.character_model->model_data_b[0x85] != NULL ? 0x85 : 5;
    target->airborne_action_duration = static_cast<f32>(qrand()) * (1.0f / 65535.0f) + 1.0f;

    NUVEC direction;
    NuVecSub(&direction, &target->apiobj.collision_position, &source->apiobj.collision_position);
    u16 angle = static_cast<u16>(NuAtan2D(direction.x, direction.z));
    i32 reverse = 0;
    if (direction.x * target->facing_direction.x + direction.z * target->facing_direction.z < 0.0f) {
        angle += 0x8000;
        reverse = 1;
    }
    target->field_0x7a3 = reverse;
    if (target->context_animation != 0x85) {
        target->apiobj.field_0x276 = angle;
        target->apiobj.facing_angle = angle;
        target->apiobj.movement_facing_angle = angle;
    }
    target->delayed_turn_timer = 0.0f;
    NewBuzzFrames(source->pad_gamepad->pad, 1, 0);
    data = static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24);
    GameAudio_PlaySfxById(data->sfx_hurt, &target->apiobj.collision_position, 0, 0);
    NewBuzz(target->pad_gamepad->pad, 0.1f, 0);
}

void Hang_MoveCode(GameObject_s *) {
}

void HoldCode_Copy(GameObject_s *object) {
    if (LEGOCONTEXT_HOLD != -1 && object->character_context == LEGOCONTEXT_HOLD) {
        if (object->context_animation_timer > 0.0f)
            object->context_animation_timer -= FRAMETIME;
        else if ((object->pad_gamepad->buttons_held & GAMEPAD_ACTION) == 0)
            object->character_context = -1;
    }
}

void SetHeadTarget(GameObject_s *object, NUVEC *position, i8 priority, f32 time, f32 minimum_delay, f32 maximum_delay) {
    if (position == NULL) {
        object->head_target = NULL;
        object->head_target_priority = 0;
        object->head_target_timer = 0.0f;
        object->head_target_delay = 0.0f;
    } else {
        if (object->head_target == NULL ||
            (position != object->head_target && object->head_target_priority <= priority)) {
            i32 random = qrand();
            object->head_target = position;
            object->head_target_priority = priority;
            f32 delay = maximum_delay * random * 1.5259022e-05f + (1.0f - random * 1.5259022e-05f) * minimum_delay;
            object->head_target_delay = delay;
            object->head_target_timer = time + delay;
        }
        object->head_target_position = *object->head_target;
    }
}

extern i32 DoubleJump_JediSlam;
extern f32 SLAMJUMPSPEED;
void PlayLandSfx(GameObject_s *, i32, i32);
void AddWaterSplash(GameObject_s *, NUVEC *);

i32 StartBackFlip(GameObject_s *object) {
    if (LEGOCONTEXT_BACKFLIP == -1 || LEGOACT_BACKFLIP == -1)
        return 0;
    if (object->apiobj.character_model->model_data_b[LEGOACT_BACKFLIP] == NULL)
        return 0;
    object->character_context = LEGOCONTEXT_BACKFLIP;
    ResetAnimPacket(&object->apiobj.anim_packet, -1);
    object->context_animation = LEGOACT_BACKFLIP;
    const f32 duration = AnimDuration(object->id, LEGOACT_BACKFLIP, 0.0f, 0.0f, 1);
    object->field_0xe22 |= 0x10;
    object->context_variant_flags &= ~0x80;
    object->delayed_turn_timer = 0.0f;
    object->airborne_input_timer = 0.0f;
    object->context_animation_timer = duration <= 0.0f ? 1.0f : duration;
    PlayJumpSfx(object, 2);
    return 1;
}


static void ClearLastSafeTakeoverSource(GameObject_s *object) {
    if (object->takeover_source != NULL &&
        (LEGOCONTEXT_GETIN == -1 || object->character_context != LEGOCONTEXT_GETIN) &&
        (LEGOCONTEXT_BEENTAKENOVER == -1 || object->character_context != LEGOCONTEXT_BEENTAKENOVER))
        object->takeover_source = NULL;
}

void UpdateLastSafePosition(GameObject_s *object) {
    APIOBJECT &api = object->apiobj;
    object->field_0xf02 &= ~0x40;
    if (api.field_0x287 == 0 && (LEGOCONTEXT_DOOMED == -1 || object->character_context != LEGOCONTEXT_DOOMED) &&
        (api.flags_low & 4) == 0) {
        api.field_0x1c0 = api.position;
        GameObject_s *terrain_object = object;
        if (object->field_0xcc0 != NULL && LEGOCONTEXT_BEENTAKENOVER != -1 &&
            object->character_context == LEGOCONTEXT_BEENTAKENOVER)
            terrain_object = object->field_0xcc0;
        const i32 surface = static_cast<i8>(terrain_object->apiobj.field_0x281);
        i32 unsafe;
        if (surface != -1 && (TerSurface[surface].flags & 0xc041) != 0) {
            unsafe = 1;
        } else {
            const i8 layer = terrain_object->apiobj.field_0x27f;
            unsafe = 0;
            if (layer != -1 && ((TerLayer[layer].flags & 1) != 0 || layer == 9))
                unsafe = 1;
        }
        if (unsafe && VehicleArea != 0 && BonusArea != 0)
            unsafe =
                !(static_cast<GAMECHARACTERDATA *>(terrain_object->apiobj.character_data->field11_0x24)->field_0x28 >
                  0.0f);
        if (WORLD->current_level == NEWTOWN_LDATA && terrain_object->field_0xe31 == 1 &&
            terrain_object->apiobj.position.y > 0.7f)
            unsafe = true;
        do {
            if ((object->field_0xf00 & 4) != 0)
                break;
            const i8 context = object->character_context;
            if ((CInfo[context].parameter & 8) != 0)
                break;
            if (api.field_0x27d == 0 &&
                (context == -1 || (context != LEGOCONTEXT_BEENTAKENOVER && context != LEGOCONTEXT_EATEN &&
                                   context != LEGOCONTEXT_GETIN)) &&
                VehicleArea == 0 && object->id != id_TRAININGREMOTE) {
                const u32 flags = api.character_data->model_flags;
                if (((flags & 0x2000) == 0 ||
                     !(static_cast<GAMECHARACTERDATA *>(api.character_data->field11_0x24)->field_0x28 > 0.0f)) &&
                    ((flags & 0x8000) == 0 || object->field_0xe31 != 1))
                    break;
            }
            if (!(object->surface_normal.y > 0.9f) || api.field_0x218 == 2000000.0f)
                break;
            const i16 platform = object->field_0x1078;
            i16 shadow_platform;
            if (!((platform == -1 || platform == LevSafePlatID[0] || platform == LevSafePlatID[1]) &&
                  ((shadow_platform = api.supporting_platform_id) == -1 || shadow_platform == LevSafePlatID[0] ||
                   shadow_platform == LevSafePlatID[1])) &&
                (LastSafePosExtraFn == NULL || !LastSafePosExtraFn(object)))
                break;
            if (!(api.lower_position.y > api.field_0x218 - 0.1f) || unsafe ||
                (VehicleArea != 0 && TouchHacks::CheckForAboutToRunIntoKillTerrain(*object, 1.0f)))
                break;
            object->field_0xf02 |= 0x40;
            api.last_safe_position = api.position;
            object->fall_recovery_timer = 0.0f;
            if ((object->ai.path_info.flags & 1) != 0 && (object->ai.path_info.connection->traversal_flags[1] |
                                                          object->ai.path_info.connection->traversal_flags[0]) == 0) {
                ClearLastSafeTakeoverSource(object);
                api.respawn_position = api.last_safe_position;
                api.flags_high |= 0x20;
            }
            if (static_cast<u8>(api.field_0x27c) < 2 && object->field_0xda8 <= 0.0f &&
                GameCam->blend_time >= GameCam->blend_duration && object->sock_position.location.sock != -1) {
                ClearLastSafeTakeoverSource(object);
                object->last_safe_camera_position = api.last_safe_position;
            }
            goto safe_position_done;
        } while (false);
        {
            i8 edge_layer;
            if ((api.flags_low & 0x80) != 0 && (edge_layer = api.field_0x27f) != -1 &&
                (TerLayer[edge_layer].flags & 1) != 0 && !NoLayerKill(object)) {
                if (object->fall_recovery_timer <= 0.0f)
                    object->fall_hover_height = api.position.y;
                object->fall_recovery_timer += FRAMETIME;
            } else {
                object->fall_recovery_timer = 0.0f;
            }
            if ((WORLD->current_level->flags & 0x40000) != 0) {
                api.last_safe_position = api.position;
            }
        }
    }
safe_position_done:
    if ((object->field_0xf03 & 1) == 0 && (api.flags_low & 0x80) == 0 &&
        (LEGOCONTEXT_EATEN == -1 || object->character_context != LEGOCONTEXT_EATEN) && (api.flags_high & 0x20) != 0) {
        if ((object->ai.path_info.flags & 1) != 0)
            ClearLastSafeTakeoverSource(object);
        api.last_safe_position = api.respawn_position;
    }
    object->field_0xf00 &= ~4;
}

void FindSlamOrigin(GameObject_s *object, NUVEC *start, NUVEC *end) {
    if (FindSlamOrigin_UseCPosFn != NULL && FindSlamOrigin_UseCPosFn(object)) {
        start->x = object->apiobj.collision_position.x;
        start->z = object->apiobj.collision_position.z;
    } else {
        const i32 joint =
            static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->streak_joints[0][0];
        if (joint == -1 || object->apiobj.field_0x288 == 0 || object->apiobj.character_model == NULL ||
            object->apiobj.character_model->points_of_interest[joint] == NULL) {
            start->x = object->apiobj.position.x;
            start->z = object->apiobj.position.z;
        } else {
            start->x = object->joint_matrices[joint].m30;
            start->z = object->joint_matrices[joint].m32;
        }
    }
    start->y = object->apiobj.field_0x218;
    if (end != NULL) {
        const i32 joint =
            static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->streak_joints[0][1];
        if (joint == -1 || object->apiobj.field_0x288 == 0 || object->apiobj.character_model == NULL ||
            object->apiobj.character_model->points_of_interest[joint] == NULL) {
            *end = *start;
        } else {
            *end = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3);
        }
    }
}

void JumpCode(GameObject_s *object, i32 jump_pressed, i32 jump_held, u32 animation_set, i32 action_pressed,
              i32 action_held, i32 hit_effect) {
    if ((object->movement_runtime_flags & 0x10) != 0) {
        return;
    }
    WORLDINFO *world = WorldInfo_CurrentlyActive();

    const u8 player_flag = object->apiobj.flags_low & 0x80;
    i32 flip_kind = 0;
    bool force_flip = false;
    if (player_flag != 0 || (object->field_0xef9 & 1) != 0) {
        if (LEGOACT_BACKFLIP != -1 && object->apiobj.character_model->model_data_b[LEGOACT_BACKFLIP] != NULL)
            flip_kind = 1;
        else if ((animation_set & 2) != 0 && LEGOACT_FLIP != -1 &&
                 object->apiobj.character_model->model_data_b[LEGOACT_FLIP] != NULL)
            flip_kind = 2;
    }
    if (object->jump_reentry_timer > 0.0f)
        object->jump_reentry_timer -= FRAMETIME;
    f32 chain_timer = object->jump_chain_timer;
    if (chain_timer > 0.0f) {
        chain_timer -= FRAMETIME;
        object->jump_chain_timer = chain_timer;
    }

    if (object->character_context != -1 &&
        (object->character_context == LEGOCONTEXT_LAND_JUMP || object->character_context == LEGOCONTEXT_LAND_JUMP2 ||
         object->character_context == LEGOCONTEXT_LAND_FLIP ||
         object->character_context == LEGOCONTEXT_LAND_COMBOJUMP ||
         object->character_context == LEGOCONTEXT_LAND_LUNGE || object->character_context == LEGOCONTEXT_LAND_SLAM ||
         object->character_context == LEGOCONTEXT_LAND_COMBATROLL)) {
        if (object->character_context == LEGOCONTEXT_LAND_LUNGE && (object->context_flags & 0x40) == 0) {
            const f32 frame = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
            if (frame > 0.0f) {
                f32 *time = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
                if (time != NULL && *time >= frame) {
                    if ((object->apiobj.character_data->model_flags & 8) == 0)
                        Punch_Hit(object, object->force_target, PUNCHCHARGAP, PUNCHCHARGAP);
                    else
                        ComboHitFrame(object, 1);
                }
            }
        } else if (object->character_context == LEGOCONTEXT_LAND_COMBATROLL) {
            const f32 frame = AnimStopFrame(object->apiobj.character_model, object->context_animation);
            if (frame > 0.0f) {
                f32 *time = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
                if (time != NULL && frame <= *time)
                    object->field_0x7a3 = 1;
            }
        }
        if (object->character_context == LEGOCONTEXT_LAND_COMBATROLL &&
            (object->action_input_state & 0xff00ff00) == 0x01000100)
            goto jump_takeoff;
        if (jump_pressed != 0 && object->character_context != LEGOCONTEXT_LAND_LUNGE &&
            object->character_context != LEGOCONTEXT_LAND_SLAM &&
            object->character_context != LEGOCONTEXT_LAND_COMBATROLL)
            goto jump_takeoff;
        if (object->character_context != LEGOCONTEXT_LAND_LUNGE && object->character_context != LEGOCONTEXT_LAND_SLAM &&
            ((object->apiobj.flags_low & 0x80) != 0 || object->character_context != LEGOCONTEXT_LAND_JUMP ||
             object->context_animation == -1 ||
             (object->context_animation != LEGOACT_FALLLAND &&
              object->context_animation != LEGOACT_BACKPACKFALLLAND)) &&
            (object->character_context != LEGOCONTEXT_LAND_COMBATROLL ||
             ((static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_094[2] & 8) != 0 &&
              (object->action_input_state & 0xff00ff00) != 0x01000000)) &&
            object->pad_gamepad->input_magnitude > 0.0f && (object->movement_runtime_flags & 8) == 0) {
            object->character_context = -1;
            return;
        }
        if (object->character_context == LEGOCONTEXT_LAND_COMBATROLL) {
            if (jump_pressed != 0)
                object->landing_followup = 1;
            else if (action_pressed != 0 && LEGOACT_COMBATROLL_FIRE != -1 &&
                     object->apiobj.character_model->model_data_b[LEGOACT_COMBATROLL_FIRE] != NULL)
                object->landing_followup = 2;
        }
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer > 0.0f && object->character_context == LEGOCONTEXT_LAND_SLAM) {
            i32 count = ParticlesPerSecond(25.0f, FRAMETIME);
            NUVEC start, end;
            FindSlamOrigin(object, &start, &end);
            AddGameDebrisRot(world->debris_sys, hit_effect, &start, ParticlesPerSecond(20.0f, FRAMETIME), 0, 0);
            while (count > 0) {
                const f32 t = qrand() * 1.5259022e-05f;
                NUVEC position;
                position.x = (end.x - start.x) * t + start.x;
                position.y = (end.y - start.y) * t + start.y;
                position.z = (end.z - start.z) * t + start.z;
                AddGameDebrisRot(world->debris_sys, hit_effect, &position, 1, 0, 0);
                --count;
            }
            return;
        }
        if (!(object->context_animation_timer > 0.0f)) {
            if (Jump_EndOfLandContextFn != NULL)
                Jump_EndOfLandContextFn(object);
            if (object->character_context == LEGOCONTEXT_LAND_COMBATROLL) {
                if (object->landing_followup == 2) {
                    object->action_suppressed = 0;
                    const u16 angle = GamePad_InputAngle(object, object->pad_gamepad);
                    object->apiobj.field_0x276 = angle;
                    object->apiobj.movement_facing_angle = angle;
                    object->apiobj.facing_angle = angle;
                    StartQuickShoot(object, LEGOACT_COMBATROLL_FIRE);
                    object->quick_shoot_timer = 0.1f;
                    return;
                }
                if (object->landing_followup == 1)
                    goto jump_takeoff;
                object->character_context = -1;
                return;
            }
            if (object->character_context == LEGOCONTEXT_LAND_LUNGE && (object->context_flags & 0x40) == 0 &&
                (object->apiobj.character_data->model_flags & 8) != 0)
                ComboHitFrame(object, 1);
            if ((object->character_context == LEGOCONTEXT_LAND_LUNGE ||
                 object->character_context == LEGOCONTEXT_LAND_SLAM) &&
                action_held != 0) {
                StartHold(object);
                return;
            }
            object->character_context = CHARACTER_CONTEXT_NONE;
        }
        return;
    }

    if (LEGOCONTEXT_JUMP == -1)
        return;
    if (object->character_context != LEGOCONTEXT_JUMP) {
        if ((animation_set & 0x80) == 0 && object->fall_acceleration_timer <= 0.0f &&
            (jump_pressed != 0 || ((object->field_0xef9 & 1) != 0 && flip_kind != 0)) &&
            (object->apiobj.field_0x27d != 0 || object->ground_contact_grace_timer > 0.0f) &&
            (CInfo[object->character_context].flags & 0x1000) != 0 &&
            ((animation_set & 4) == 0 ||
             (!(chain_timer > 0.0f) && (object->apiobj.is_underwater == 0 || object->communicate_blend == 0.0f)))) {
        jump_takeoff:
            if (Jump_PreventJumpFn != NULL && Jump_PreventJumpFn(object))
                return;
            if (LEGOCONTEXT_WEAPONOUT != -1 && object->character_context == LEGOCONTEXT_WEAPONOUT)
                FastWeaponOut(object, 0);
            else if (LEGOCONTEXT_WEAPONIN != -1 && object->character_context == LEGOCONTEXT_WEAPONIN)
                FastWeaponIn(object, 0);
            if (object->apiobj.field_0x27d != 0)
                object->field_0xf03 |= 0x20;
            if (object->jump_reentry_timer <= 0.0f)
                object->jump_sequence = 0;
            if (flip_kind == 0 ||
                (object->delayed_turn_timer <= 0.0f && !force_flip && (object->field_0xef9 & 1) == 0)) {
                object->action_movement_state = (animation_set & 4) != 0 ? 5 : 0;
                if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) && LEGOACT_EXTRA_JUMP != -1 &&
                    object->apiobj.character_model->model_data_b[LEGOACT_EXTRA_JUMP] != NULL)
                    object->context_animation = LEGOACT_EXTRA_JUMP;
                else
                    object->context_animation = LEGOACT_JUMP;
                if (CanMagnetClimbFn != NULL && CanMagnetClimbFn(object) && LEGOACT_MAGNET_JUMP != -1 &&
                    object->apiobj.character_model->model_data_b[LEGOACT_MAGNET_JUMP] != NULL)
                    object->context_animation = LEGOACT_MAGNET_JUMP;
                object->jump_start_height = object->apiobj.collision_min.y;
                if ((animation_set & 1) == 0)
                    object->jump_sequence = 1;
                else
                    ++object->jump_sequence;
                if ((animation_set & 1) != 0 && object->jump_sequence > 1) {
                    if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) && LEGOACT_EXTRA_JUMP2 != -1 &&
                        object->apiobj.character_model->model_data_b[LEGOACT_EXTRA_JUMP2] != NULL)
                        object->context_animation = LEGOACT_EXTRA_JUMP2;
                    else if (LEGOACT_JUMP2 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP2] != NULL)
                        object->context_animation = LEGOACT_JUMP2;
                    else
                        object->context_animation = LEGOACT_JUMP;
                    object->apiobj.velocity.y =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)
                            ->second_jump_speed;
                    ResetAnimPacket(&object->apiobj.anim_packet, object->context_animation);
                    object->context_variant_flags &= ~0x40;
                    if ((animation_set & 0x100) != 0)
                        MakeJumpReachHeight(object, HIGHJUMPHEIGHT, 1);
                } else {
                    if ((animation_set & 0x40) != 0 &&
                        (object->context_animation == -1 ||
                         object->apiobj.character_model->model_data_b[object->context_animation] == NULL))
                        object->context_animation = LEGOACT_FALL;
                    object->apiobj.velocity.y =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->jump_speed;
                    if (object->action_movement_state != 5)
                        ResetAnimPacket(&object->apiobj.anim_packet, object->context_animation);
                    if ((animation_set & 0x101) == 0x100)
                        MakeJumpReachHeight(object, HIGHJUMPHEIGHT, 1);
                    if ((object->apiobj.flags_low & 0x80) != 0)
                        Hint_SetComplete(0x604);
                }
            } else {
                if (flip_kind == 1 && StartBackFlip(object))
                    return;
                object->action_movement_state = 1;
                object->context_animation = LEGOACT_FLIP;
                ResetAnimPacket(&object->apiobj.anim_packet, LEGOACT_FLIP);
                object->apiobj.velocity.y =
                    static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->second_jump_speed;
            }
            const u8 variant_flags = object->context_variant_flags;
            object->context_animation_timer = 0.0f;
            object->jump_flags &= ~1;
            object->character_context = LEGOCONTEXT_JUMP;
            object->context_variant_flags = variant_flags & ~0x10;
            if (object->action_movement_state == 1 && LEGOACT_FLIP != -1 &&
                object->apiobj.character_model->model_data_b[LEGOACT_FLIP] != NULL)
                object->context_animation = LEGOACT_FLIP;
            object->field_0xe22 |= 0x10;
            object->apiobj.field_0x27d = 0;
            object->field_0x105c = 0;
            object->delayed_turn_timer = 0.0f;
            if ((animation_set & 4) != 0) {
                object->jump_variant_timer =
                    static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field_0x48;
                if ((object->apiobj.flags_low & 0x80) == 0)
                    object->jump_variant_timer *= 1.5f;
                object->field_0xe31 = 1;
            }
            object->airborne_input_timer = 0.0f;
            object->airborne_collision_target = NULL;
            object->context_variant_flags = variant_flags & 0x6f;
            PlayJumpSfx(object, object->jump_sequence > 1);
            if (object->apiobj.field_0x27f == 1 && object->apiobj.collision_min.y < object->apiobj.water_height)
                AddWaterSplash(object, &object->apiobj.collision_position);
            return;
        }
    } else {
        i8 variant_flags = object->context_variant_flags;
        if ((variant_flags & 0x10) != 0)
            object->movement_runtime_flags |= 0x80;
        if ((object->field_0xe22 & 0x10) != 0 && jump_held == 0) {
            object->field_0xe22 &= ~0x10;
        }

        const f32 vertical_speed = object->apiobj.velocity.y;
        if (vertical_speed < 0.0f) {
            object->airborne_collision_target = NULL;
        }
        const u8 movement_state = object->action_movement_state;
        if (movement_state == 5 && object->field_0xe31 == 2 && object->jump_variant_timer > 0.0f) {
            object->jump_variant_timer -= FRAMETIME;
            if (object->jump_variant_timer <= 0.0f)
                object->field_0xe31 = 3;
        }
        object->context_animation_timer += FRAMETIME;
        if (movement_state == 8 && variant_flags >= 0 &&
            object->apiobj.character_model->model_data_b[object->context_animation] != NULL &&
            object->context_animation_timer >= object->airborne_action_duration && LEGOACT_COMBATROLL_FALL != -1 &&
            object->apiobj.character_model->model_data_b[LEGOACT_COMBATROLL_FALL] != NULL) {
            variant_flags |= 0x80;
            object->context_variant_flags = variant_flags;
        }
        if ((movement_state == 0 || movement_state == 2 || movement_state == 8) &&
            object->context_animation_timer >= 2.0f && object->apiobj.start_position.y <= object->apiobj.position.y &&
            object->apiobj.field_0x27d == 0) {
            object->character_context = -1;
            object->jump_reentry_timer = 0.0f;
            object->fall_animation_timer = 0.2f;
            return;
        }

        bool touch_action = false;
        if (MechInputTouchSystem::s_actualTouchMode == 2 && player_flag != 0)
            touch_action = (object->jump_input_flags & 0x20) != 0;
        else
            object->jump_input_flags &= ~0x20;
        bool landed;
        if (movement_state == 5) {
            landed = (object->apiobj.field_0x27d & 1) != 0 ||
                     (player_flag == 0 && object->field_0x1084 != 0 && object->field_0xe31 == 3);
        } else {
            landed = (object->apiobj.field_0x27d != 0 && object->context_animation_timer >= 0.1f) ||
                     ((movement_state == 3 || movement_state == 4) && object->context_animation_timer >= 2.5f);
        }
        if (!landed) {
            GAMECHARACTERDATA *game_character =
                static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
            if ((touch_action || action_pressed != 0) && (vertical_speed > -1.25f || touch_action) &&
                variant_flags >= 0) {
                if (movement_state == 0 && object->jump_sequence < 2 &&
                    (game_character->field275_0x116 != 0 || (object->apiobj.character_data->model_flags & 8) != 0) &&
                    LEGOACT_LUNGE != -1 && object->apiobj.character_model->model_data_b[LEGOACT_LUNGE] != NULL) {
                    object->field_0x780 = NULL;
                    object->blowup_target = NULL;
                    StartLunge(object, 0.0f, object->apiobj.field_0x1e0);
                    return;
                }
            }
            if (DoubleJump_JediSlam != 0 && (animation_set & 0x10) != 0 &&
                (touch_action || (action_pressed != 0 && vertical_speed > -1.25f && variant_flags >= 0)) &&
                ((movement_state == 0 && (object->jump_sequence == 2 || (game_character->field_0x98 & 0x20) != 0)) ||
                 movement_state == 1 || movement_state == 2) &&
                LEGOACT_SLAM != -1 && object->apiobj.character_model->model_data_b[LEGOACT_SLAM] != NULL) {
                if (Slam_Start(object, SLAMJUMPSPEED) != 0) {
                    PlaySabreSfx(NULL, object, NULL, 0);
                    if (object->action_movement_state == 1 || object->action_movement_state == 2) {
                        ResetAnimPacket(&object->apiobj.anim_packet, -1);
                        if (object->action_movement_state == 2 && (object->context_variant_flags & 0x20) == 0) {
                            object->apiobj.field_0x276 += 0x8000;
                            object->apiobj.facing_angle += 0x8000;
                            object->apiobj.movement_facing_angle += 0x8000;
                        }
                    }
                    return;
                }
            }
            const bool buffered_second_jump = (object->jump_input_flags & 0x10) != 0;
            const bool can_start_second_jump =
                (jump_pressed != 0 || buffered_second_jump) && (animation_set & 8) != 0 &&
                object->action_movement_state == PLAYER_JUMP_MOVEMENT_BASIC && object->jump_sequence <= 1 &&
                (object->apiobj.velocity.y > -1.25f || buffered_second_jump);
            if (can_start_second_jump) {
                if (LEGOCONTEXT_DOOMED != -1 && object->character_context == LEGOCONTEXT_DOOMED)
                    return;
                game_character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                if ((game_character->flags_094[1] & 0x20) != 0) {
                    object->jump_variant_timer = 0.0f;
                    object->character_context = -1;
                    object->field_0xe31 = 1;
                    FastWeaponOut(object, 1);
                    return;
                }
                if ((animation_set & 0x110) == 0 && (game_character->flags_090 & 0x40400000) == 0)
                    return;
                if ((animation_set & 0x100) != 0) {
                    MakeJumpReachHeight(object, HIGHJUMPHEIGHT, 1);
                } else if ((animation_set & 0x200) != 0) {
                    MakeJumpReachHeight(object, game_character->jump_height, 0);
                } else if (DoubleJump_AlwaysReachJump2Height != 0 || object->apiobj.velocity.y > 0.0f) {
                    MakeJumpReachHeight(object, game_character->second_jump_height, 0);
                } else {
                    object->apiobj.velocity.y = 0.0f;
                }
                if (CanGlideFn != NULL && CanGlideFn(object) && Glide_Start(object))
                    return;
                object->jump_sequence++;
                object->context_variant_flags |= 0x40;
                game_character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                if ((animation_set & 0x110) == 0 && (game_character->flags_090 & 0x400000) == 0) {
                    if ((game_character->flags_090 & 0x40000000) != 0) {
                        object->action_movement_state = 8;
                        object->context_variant_flags &= ~0x80;
                        object->context_animation = LEGOACT_COMBATROLL_JUMP;
                        object->airborne_action_duration =
                            AnimDuration(object->id, LEGOACT_COMBATROLL_JUMP, 0.0f, 0.0f, 1);
                    }
                } else {
                    object->action_movement_state = 0;
                    i16 action;
                    if (LEGOACT_JUMP3 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP3] != NULL)
                        action = LEGOACT_JUMP3;
                    else if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) && LEGOACT_EXTRA_JUMP2 != -1 &&
                             object->apiobj.character_model->model_data_b[LEGOACT_EXTRA_JUMP2] != NULL)
                        action = LEGOACT_EXTRA_JUMP2;
                    else if (LEGOACT_JUMP2 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP2] != NULL)
                        action = LEGOACT_JUMP2;
                    else
                        action = LEGOACT_JUMP;
                    object->context_animation = action;
                    if ((object->apiobj.flags_low & 0x80) != 0)
                        Hint_SetComplete(0x606);
                }
                object->context_animation_timer = 0.0f;
                object->field_0xe22 |= 0x10;
                PlayJumpSfx(object, 1);
                if ((animation_set & 0x10) == 0) {
                    object->context_variant_flags |= 0x10;
                }
                return;
            }
            return;
        }
        if (player_flag != 0 && (object->action_movement_state != 5 || (object->apiobj.field_0x27d & 1) != 0))
            object->field_0xef9 |= 8;
        const bool restart_jump = jump_held != 0 && (object->field_0xe22 & 0x10) == 0 &&
                                  object->action_movement_state != 3 && object->action_movement_state != 4 &&
                                  object->action_movement_state != 5 && object->action_movement_state != 8;
        if (restart_jump && (object->action_movement_state == 1 || object->action_movement_state == 2) &&
            object->pad_gamepad->input_magnitude > 0.0f && (object->field_0xe22 & 0x20) != 0) {
            i32 difference = RotDiff(object->apiobj.movement_facing_angle, object->current_input_angle);
            if (difference < 0)
                difference = -difference;
            force_flip = object->action_movement_state == 1 ? difference > 0x6aaa : difference < 0x1555;
        }
        {
            if (object->action_movement_state == 3) {
                if (LEGOCONTEXT_LAND_LUNGE != -1 && LEGOACT_LUNGELAND != -1 &&
                    object->apiobj.character_model->model_data_b[LEGOACT_LUNGELAND] != NULL) {
                    NewRumble(object->pad_gamepad->pad, 0.6f, 0);
                    object->character_context = LEGOCONTEXT_LAND_LUNGE;
                    object->context_animation = LEGOACT_LUNGELAND;
                    object->context_animation_timer = AnimDuration(object->id, LEGOACT_LUNGELAND, 0.0f, 0.0f, 1);
                    object->context_flags &= ~0x40;
                    object->jump_reentry_timer = 0.0f;
                    if (!restart_jump)
                        ResetAnimPacket(&object->apiobj.anim_packet, -1);
                    if ((object->apiobj.character_data->model_flags & 8) == 0 &&
                        AnimListFrame(object->apiobj.character_model, object->context_animation, 0) == 0.0f)
                        Punch_Hit(object, object->force_target, PUNCHCHARGAP, PUNCHCHARGAP);
                    PlayLandSfx(object, 1, 0);
                } else {
                    if ((object->apiobj.character_data->model_flags & 8) == 0) {
                        Punch_Hit(object, object->force_target, PUNCHCHARGAP, PUNCHCHARGAP);
                        object->character_context = -1;
                        object->jump_reentry_timer = 0.2f;
                        if (object->action_movement_state == 5)
                            object->jump_chain_timer = 0.0f;
                    } else {
                        object->character_context = -1;
                        object->jump_reentry_timer = 0.2f;
                    }
                    PlayLandSfx(object, 0, 0);
                }
            } else if (object->action_movement_state == 4) {
                if (LEGOCONTEXT_LAND_SLAM != -1 && LEGOACT_SLAMLAND != -1 &&
                    object->apiobj.character_model->model_data_b[LEGOACT_SLAMLAND] != NULL) {
                    GameCam_Judder(GameCam, 0.5f, 0, &object->apiobj.collision_position);
                    NewRumble(object->pad_gamepad->pad, 0.75f, 0);
                    object->slam_debris_effect = hit_effect;
                    if (Slam_GetDebrisFn != NULL)
                        object->slam_debris_effect = Slam_GetDebrisFn(object, hit_effect);
                    object->character_context = LEGOCONTEXT_LAND_SLAM;
                    object->context_animation = LEGOACT_SLAMLAND;
                    object->context_animation_timer = AnimDuration(object->id, LEGOACT_SLAMLAND, 0.0f, 0.0f, 1);
                    if (object->context_animation_timer <= 0.0f)
                        object->context_animation_timer = 0.75f;
                    object->jump_reentry_timer = 0.0f;
                    if (!restart_jump) {
                        ResetAnimPacket(&object->apiobj.anim_packet, -1);
                        ResetMiniAnimPacket(&object->mini_animation, -1);
                    }
                    PlayLandSfx(object, 2, 0);
                } else {
                    object->character_context = -1;
                    object->jump_reentry_timer = 0.2f;
                    PlayLandSfx(object, 0, 0);
                }
            } else if (object->action_movement_state == 8) {
                if (LEGOCONTEXT_LAND_COMBATROLL != -1 &&
                    object->apiobj.character_model->model_data_b[LEGOACT_COMBATROLL_LAND] != NULL) {
                    NewRumble(object->pad_gamepad->pad, 0.4f, 0);
                    object->character_context = LEGOCONTEXT_LAND_COMBATROLL;
                    object->field_0x7a3 = 0;
                    object->context_animation = LEGOACT_COMBATROLL_LAND;
                    object->context_animation_timer = AnimDuration(object->id, LEGOACT_COMBATROLL_LAND, 0.0f, 0.0f, 1);
                    object->context_flags &= ~0x40;
                    object->jump_reentry_timer = 0.0f;
                    if (!restart_jump)
                        ResetAnimPacket(&object->apiobj.anim_packet, -1);
                    object->landing_followup = 0;
                    if (jump_held != 0 && (object->field_0xe22 & 0x10) == 0)
                        object->landing_followup = 1;
                    else if (action_held != 0 && LEGOACT_COMBATROLL_FIRE != -1 &&
                             object->apiobj.character_model->model_data_b[LEGOACT_COMBATROLL_FIRE] != NULL)
                        object->landing_followup = 2;
                    PlayLandSfx(object, 4, 0);
                } else {
                    object->character_context = -1;
                    object->jump_reentry_timer = 0.2f;
                    PlayLandSfx(object, 0, 0);
                }
            } else if (object->action_movement_state == 2 && LEGOCONTEXT_LAND_COMBOJUMP != -1 &&
                       LEGOACT_COMBOLAND != -1 &&
                       object->apiobj.character_model->model_data_b[LEGOACT_COMBOLAND] != NULL) {
                if ((object->context_variant_flags & 0x20) == 0) {
                    object->apiobj.field_0x276 += 0x8000;
                    object->apiobj.facing_angle += 0x8000;
                    object->apiobj.movement_facing_angle += 0x8000;
                    if (!restart_jump)
                        ResetAnimPacket(&object->apiobj.anim_packet, -1);
                }
                object->jump_reentry_timer = 0.0f;
                if (object->pad_gamepad->input_magnitude == 0.0f) {
                    object->character_context = LEGOCONTEXT_LAND_COMBOJUMP;
                    object->context_animation =
                        (object->context_variant_flags & 0x20) != 0 && LEGOACT_FLIPLAND != -1 &&
                                object->apiobj.character_model->model_data_b[LEGOACT_FLIPLAND] != NULL
                            ? LEGOACT_FLIPLAND
                            : LEGOACT_COMBOLAND;
                    object->context_animation_timer =
                        AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
                } else {
                    object->character_context = -1;
                }
                PlayLandSfx(object, 0, 0);
            } else if (object->action_movement_state == 1 && LEGOCONTEXT_LAND_FLIP != -1 && LEGOACT_FLIPLAND != -1 &&
                       object->apiobj.character_model->model_data_b[LEGOACT_FLIPLAND] != NULL) {
                object->jump_reentry_timer = 0.0f;
                if (object->pad_gamepad->input_magnitude == 0.0f) {
                    object->character_context = LEGOCONTEXT_LAND_FLIP;
                    object->context_animation = LEGOACT_FLIPLAND;
                    object->context_animation_timer = AnimDuration(object->id, LEGOACT_FLIPLAND, 0.0f, 0.0f, 1);
                    if (!restart_jump)
                        ResetAnimPacket(&object->apiobj.anim_packet, -1);
                } else {
                    object->character_context = -1;
                }
                PlayLandSfx(object, 0, 0);
            } else {
                bool second_landing = false;
                if (object->action_movement_state == 0 && object->jump_sequence >= 2 && LEGOCONTEXT_LAND_JUMP2 != -1) {
                    i16 action = LEGOACT_LAND2;
                    if ((object->context_variant_flags & 0x40) != 0 &&
                        (action == -1 || object->apiobj.character_model->model_data_b[action] == NULL))
                        action = LEGOACT_LAND3;
                    second_landing = action != -1 && object->apiobj.character_model->model_data_b[action] != NULL;
                }
                if (second_landing) {
                    object->jump_reentry_timer = 0.0f;
                    if (object->pad_gamepad->input_magnitude != 0.0f) {
                        object->character_context = -1;
                    } else if (object->fall_animation_timer >= 0.2f) {
                        StartFallLand(object, -1);
                    } else {
                        object->character_context = LEGOCONTEXT_LAND_JUMP2;
                        if ((object->context_variant_flags & 0x40) != 0 && LEGOACT_LAND3 != -1 &&
                            object->apiobj.character_model->model_data_b[LEGOACT_LAND3] != NULL)
                            object->context_animation = LEGOACT_LAND3;
                        else if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) &&
                                 LEGOACT_EXTRA_LAND2 != -1 &&
                                 object->apiobj.character_model->model_data_b[LEGOACT_EXTRA_LAND2] != NULL)
                            object->context_animation = LEGOACT_EXTRA_LAND2;
                        else
                            object->context_animation = LEGOACT_LAND2;
                        object->context_animation_timer =
                            AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
                        if (!restart_jump)
                            ResetAnimPacket(&object->apiobj.anim_packet, -1);
                    }
                } else if ((object->action_movement_state == 0 || object->action_movement_state == 6 ||
                            object->action_movement_state == 7 || object->action_movement_state == 9) &&
                           LEGOCONTEXT_LAND_JUMP != -1 && LEGOACT_LAND != -1 &&
                           object->apiobj.character_model->model_data_b[LEGOACT_LAND] != NULL &&
                           object->pad_gamepad->input_magnitude == 0.0f) {
                    if (object->fall_animation_timer >= 0.2f) {
                        StartFallLand(object, -1);
                    } else {
                        object->character_context = LEGOCONTEXT_LAND_JUMP;
                        if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) && LEGOACT_EXTRA_LAND != -1 &&
                            object->apiobj.character_model->model_data_b[LEGOACT_EXTRA_LAND] != NULL)
                            object->context_animation = LEGOACT_EXTRA_LAND;
                        else
                            object->context_animation = LEGOACT_LAND;
                        if (CanMagnetClimbFn != NULL && CanMagnetClimbFn(object) && LEGOACT_MAGNET_JUMP != -1 &&
                            object->apiobj.character_model->model_data_b[LEGOACT_MAGNET_JUMP] != NULL)
                            object->context_animation = LEGOACT_MAGNET_JUMP;
                        object->context_animation_timer =
                            AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
                        object->jump_reentry_timer = object->jump_sequence > 1 ? 0.0f : 0.2f;
                        if (!restart_jump)
                            ResetAnimPacket(&object->apiobj.anim_packet, -1);
                    }
                } else {
                    object->character_context = -1;
                    object->jump_reentry_timer = 0.2f;
                    if (object->action_movement_state == 5)
                        object->jump_chain_timer = 0.0f;
                }
                PlayLandSfx(object, 0, 0);
            }
        }
        if (restart_jump) {
            UpdateLastSafePosition(object);
            goto jump_takeoff;
        }
        return;
    }
}

void ForcedBackCode(GameObject_s *) {
}

void Glide_MoveCode(GameObject_s *) {
}

i32 SetObjOnSurface(GameObject_s *object, i32 mode) {
    APIOBJECT &api = object->apiobj;
    if (api.field_0x218 == 2000000.0f || (WorldInfo_CurrentlyActive()->current_level->flags & LEVEL_IN_SPACE) != 0) {
        return 0;
    }

    const f32 lower_bound = object->character_bottom * api.field_0xa8;
    f32 position_y;
    if (mode == 2) {
        position_y = api.collision_min.y - lower_bound + 0.1f - 0.0999f;
        api.position.y = position_y;
        api.initial_position.y = position_y;
        api.collision_position.y = position_y;
    } else if (mode == 0) {
        position_y = api.field_0x218 - lower_bound + 0.1f - 0.0999f;
        api.position.y = position_y;
        api.initial_position.y = position_y;
        api.collision_position.y = position_y;
        api.velocity.y = -1.0f;
        return 1;
    } else {
        position_y = api.position.y;
    }

    if (position_y + lower_bound <= api.field_0x218 + 0.1f) {
        position_y = api.field_0x218 - lower_bound + 0.1f - 0.0999f;
        api.position.y = position_y;
        api.initial_position.y = position_y;
        api.collision_position.y = position_y;
        api.velocity.y = -1.0f;
        return 1;
    }

    api.velocity.y = -1.0f;
    return 0;
}

void TurnCodeCamSafe(GameObject_s *object, numtx_s *matrix) {
    static i32 TURNPITCH __attribute__((used)) = 200;
    static i16 myang __attribute__((used));
    TURNPITCH = 200;
    myang = 20;
    const f32 time = object->context_animation_timer;
    const f32 duration = object->airborne_action_duration;
    const f32 turn_duration = duration * 0.95f;
    if (time < turn_duration) {
        const f32 pitch_duration = duration * 0.5f * 0.5f;
        if (time < pitch_duration) {
            const f32 phase = 1.0f - (1.0f / pitch_duration) * time;
            const i32 index = static_cast<i32>(phase * 16384.0f + 32768.0f + 16384.0f);
            const f32 wave = NuTrigTable[(index >> 1) & 0x7fff];
            NuMtxPreRotateX(matrix, static_cast<i16>(static_cast<i32>((wave + 1.0f) * -3640.0f)));
        }
        const f32 phase = 1.0f - (1.0f / turn_duration) * time;
        const i32 index = static_cast<i32>(phase * 32768.0f + 16384.0f);
        const f32 wave = NuTrigTable[(index >> 1) & 0x7fff];
        NuMtxPreRotateZ(matrix, static_cast<i16>(static_cast<i32>((1.0f - (wave + 1.0f) * 0.5f) * 32768.0f)));
    }
}

void RotateGameMatrix(numtx_s *matrix, i32 order, u16 x, u16 y, u16 z) {
    switch (order) {
        case 0:
            if (x != 0)
                NuMtxRotateX(matrix, x);
            if (y != 0)
                NuMtxRotateY(matrix, y);
            if (z != 0)
                NuMtxRotateZ(matrix, z);
            break;
        case 1:
            if (y != 0)
                NuMtxRotateY(matrix, y);
            if (x != 0)
                NuMtxRotateX(matrix, x);
            if (z != 0)
                NuMtxRotateZ(matrix, z);
            break;
        case 2:
            if (y != 0)
                NuMtxRotateY(matrix, y);
            if (z != 0)
                NuMtxRotateZ(matrix, z);
            if (x != 0)
                NuMtxRotateX(matrix, x);
            break;
        case 3:
            if (z != 0)
                NuMtxRotateZ(matrix, z);
            if (x != 0)
                NuMtxRotateX(matrix, x);
            if (y != 0)
                NuMtxRotateY(matrix, y);
            break;
    }
}

i32 Hang_SetTargetMom(GameObject_s *object) {
    if (object->pad_gamepad->input_magnitude > 0.0f) {
        f32 speed;
        if (LEGOACT_HANG_MOVE != -1 && object->apiobj.character_model->model_data_b[LEGOACT_HANG_MOVE] != NULL) {
            speed = AnimSpeed(object->apiobj.character_model, LEGOACT_HANG_MOVE);
            if (speed < 0.25f)
                speed = 0.25f;
        } else {
            speed = 0.5f;
        }
        object->target_velocity.x = NU_SIN_LUT(object->apiobj.movement_facing_angle) * speed;
        object->target_velocity.z = NU_COS_LUT(object->apiobj.movement_facing_angle) * speed;
    } else {
        object->target_velocity.x = object->target_velocity.z = 0.0f;
    }
    object->target_velocity.y = 0.5f;
    return 1;
}

// Original 0x15aae0, 330 bytes.
i32 Glide_SetTargetMom(GameObject_s *object) {
    f32 speed;
    if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL)
        speed = AnimSpeed(object->apiobj.character_model, object->context_animation);
    else
        speed = object->apiobj.character_data->game_character->run_speed;
    if (object->field_0x788 != NULL) {
        speed *= 0.25f;
        object->target_velocity.x = NU_SIN_LUT(object->apiobj.movement_facing_angle) * speed;
        object->target_velocity.z = NU_COS_LUT(object->apiobj.movement_facing_angle) * speed;
    } else {
        object->target_velocity.x = NU_SIN_LUT(object->apiobj.movement_facing_angle) * speed;
        object->target_velocity.z = NU_COS_LUT(object->apiobj.movement_facing_angle) * speed;
        f32 descent = static_cast<i8>(object->apiobj.field_0x1f8) < 0 ? -0.5f : -0.375f;
        object->target_velocity.y = descent;
        if (object->context_animation_timer < 0.5f)
            object->target_velocity.y = (object->context_animation_timer + object->context_animation_timer) * descent;
    }
    return 1;
}

void SetObjAsHeadTarget(GameObject_s *object, GameObject_s *target, signed char, float, float, float) {
    if (target != NULL && object != NULL && target->apiobj.character_data != NULL) {
        i32 joint = GetGameCharacterData(target)->head_locator;
        NUVEC *position = joint == -1 ? &target->apiobj.collision_position
                                      : reinterpret_cast<NUVEC *>(&target->joint_matrices[joint].m30);
        SetHeadTarget(object, position, 2, 1.0f, 0.0f, 0.0f);
    }
}

// Original 0x166f30, 403 bytes.
i32 Slide_SetTargetMom(GameObject_s *object, u16 input_angle, float input_speed) {
    u16 direction;
    if (object->apiobj.field_0x27d != 0 &&
        (fabsf(object->surface_normal.x) > 0.001f || fabsf(object->surface_normal.z) > 0.001f))
        direction = NuAtan2D(object->surface_normal.x, object->surface_normal.z);
    else
        direction = object->apiobj.movement_facing_angle;
    object->target_velocity.x = NU_SIN_LUT(direction) * 1.2f;
    object->target_velocity.z = NU_COS_LUT(direction) * 1.2f;
    if (object->apiobj.field_0x27d != 0 && input_speed > 0.0f) {
        NUVEC lateral;
        lateral.x = NU_SIN_LUT(input_angle) * input_speed;
        lateral.y = 0.0f;
        lateral.z = NU_COS_LUT(input_angle) * input_speed;
        NuVecRotateY(&lateral, &lateral, -static_cast<i32>(direction));
        lateral.z = 0.0f;
        NuVecRotateY(&lateral, &lateral, direction);
        object->target_velocity.x += lateral.x;
        object->target_velocity.z += lateral.z;
    }
    return 0;
}

i32 StepBackFromTarget(GameObject_s *object) {
    NUVEC *target_position;
    float inner_distance;
    float outer_distance;
    if (object->blowup_target != NULL) {
        target_position = &object->blowup_target->mid_position;
        if ((LEGOCONTEXT_COMBO != -1 && LEGOCONTEXT_COMBO == object->character_context) ||
            (LEGOCONTEXT_JUMP != -1 && LEGOCONTEXT_JUMP == object->character_context &&
             object->action_movement_state == 3)) {
            float target_radius = object->blowup_target->target_scale;
            inner_distance = object->apiobj.collision_radius * 1.75f + target_radius;
            outer_distance = object->apiobj.collision_radius * 2.25f + target_radius;
        } else {
            inner_distance = object->apiobj.collision_radius * 0.75f + object->blowup_target->target_scale;
            outer_distance = object->apiobj.collision_radius * 1.25f + object->blowup_target->target_scale;
        }
    } else {
        GAMECHARACTERDATA *character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
        if ((character->flags_090 & 0x8000) == 0 || object->force_target == NULL) {
            return 0;
        }
        if ((LEGOCONTEXT_COMBO == -1 || LEGOCONTEXT_COMBO != object->character_context) &&
            (LEGOCONTEXT_JUMP == -1 || LEGOCONTEXT_JUMP != object->character_context ||
             object->action_movement_state != 3)) {
            return 0;
        }
        float target_radius = object->force_target->apiobj.field_0x1dc;
        target_position = &object->force_target->apiobj.collision_position;
        inner_distance = object->apiobj.field_0x1dc * 1.75f + target_radius;
        outer_distance = object->apiobj.field_0x1dc * 2.25f + target_radius;
    }
    float speed = AnimSpeed(object->apiobj.character_model, object->context_animation);
    if (speed != 0.0f) {
        float distance_squared = NuVecXZDistSqr(&object->apiobj.collision_position, target_position, NULL);
        if (!(distance_squared >= outer_distance * outer_distance)) {
            float blend = 1.0f;
            if (!(inner_distance * inner_distance >= distance_squared)) {
                blend = 1.0f - (NuFsqrt(distance_squared) - inner_distance) / (outer_distance - inner_distance);
            }
            speed = (-speed - speed) * blend + speed;
            object->target_velocity.x = NU_SIN_LUT(object->apiobj.movement_facing_angle) * speed;
            object->target_velocity.z = speed * NU_COS_LUT(object->apiobj.movement_facing_angle);
            return 1;
        }
    }
    return 0;
}

float FindGunshipHoverHeight(GameObject_s *object) {
    float height = 2.0f;
    if (object->apiobj.field_0x27c < 2) {
        float upper_height;
        float lower_height;
        if (WORLD->current_level == BONUS_GUNSHIPA_LDATA) {
            upper_height = 2.8f;
            lower_height = 1.2f;
        } else {
            upper_height = 2.05f;
            lower_height = 0.45f;
            height = 1.25f;
        }
        if (Player[0] != NULL && static_cast<i8>(Player[0]->apiobj.flags_low) < 0 && Player[1] != NULL &&
            static_cast<i8>(Player[1]->apiobj.flags_low) < 0) {
            height = object != Player[0] ? lower_height : upper_height;
        }
    }
    return height;
}

void ApplyGravity_Network(GameObject_s *object) {
    CHARACTERDATA *character = object->apiobj.character_data;
    if (character->model_flags == 0x2000) {
        GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
        if ((data->flags_094[0] & 0x20) == 0) {
            if (data->field275_0x116 == 0x15) {
                ApplyGravity(object, NULL, FindGunshipHoverHeight(object), 8.0f, NULL);
            } else {
                float height;
                if (data->field_0x28 > 0.0f && ((height = GetVehicleHoverHeight(object, NULL)) != 0.0f ||
                                                getvehiclehoverheight_hothbattlehack != 0)) {
                    ApplyGravity(object, NULL, height, 10.0f, NULL);
                } else {
                    height =
                        object->character_context == 0x17
                            ? 0.0f
                            : static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field_0x28;
                    ApplyGravity(object, NULL, height, 8.0f, NULL);
                }
            }
        }
    } else {
        if (object->apiobj.field_0x27d != 0 ||
            static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->field_0x48 <= object->jump_variant_timer ||
            object->apiobj.field_0x281 == 0x1a) {
            if (static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->field275_0x116 == 2 &&
                object->field_0xe31 == 1) {
                ApplyGravity(object, NULL, static_cast<GAMECHARACTERDATA *>(character->field11_0x24)->field_0x28, 8.0f,
                             NULL);
            } else {
                ApplyGravity(object, NULL, 0.0f, 8.0f, NULL);
            }
        }
    }
}

extern "C" i16 id_TIEFIGHTER, id_XWING;
void VehicleCollisionCode(GameObject_s *object) {
    static f32 magdif;
    if (object->character_context != -1 && object->character_context != 0x3a)
        return;
    if (GamePlayTimer.time_elapsed < 1.0f)
        return;
    LEVELDATA_s *level = WORLD->current_level;
    LEVELDATA_s *speeder_level = SPEEDERCHASEA_LDATA;
    if (object->apiobj.field_0x27c == -1 || (object->apiobj.flags_low & 0x80) == 0 || !object->field_0x1084)
        return;
    NUVEC normal, reverse_velocity, debris_position;
    debris_position = object->contact_position;
    i32 random = qrand();
    debris_position.y = object->contact_position.y - 0.5f * object->apiobj.field_0x1e0 +
                        random * (object->apiobj.field_0x1e0 / 65535.0f);
    i32 debris_type = -1;
    if (object->id == id_SPEEDERBIKE) {
        qrand();
        qrand();
        debris_type = 108;
    } else if (object->id == id_TIEFIGHTER || object->id == id_XWING)
        debris_type = 100;
    if (debris_type != -1) {
        AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[debris_type].effect, &debris_position, 52,
                                          FRAMETIME, 0, 0, NULL);
        debris_type = 101;
    }
    if (object->contact_normal.y <= 0.574f && object->contact_normal.y >= -0.574f) {
        if (object->apiobj.horizontal_velocity_magnitude >
            ((GAMECHARACTERDATA_s *)object->apiobj.character_data->field11_0x24)->field_0x10) {
            NuVecNorm(&normal, &object->contact_normal);
            reverse_velocity = object->reset_velocity;
            NuVecNorm(&reverse_velocity, &reverse_velocity);
            reverse_velocity.x = -reverse_velocity.x;
            reverse_velocity.z = -reverse_velocity.z;
            i16 angle = 0x4000 - NuASin(reverse_velocity.x * normal.x + reverse_velocity.z * normal.z);
            if (level == speeder_level && angle <= 0x71b) {
                if (object->apiobj.horizontal_velocity_magnitude >
                    (((GAMECHARACTERDATA_s *)object->apiobj.character_data->field11_0x24)->tiptoe_speed +
                     ((GAMECHARACTERDATA_s *)object->apiobj.character_data->field11_0x24)->walk_speed) *
                        0.5f)
                    goto emit_debris;
            } else if (angle > 0x1fff)
                return;
            magdif = (1.0f / ((GAMECHARACTERDATA_s *)object->apiobj.character_data->field11_0x24)->run_speed) *
                     (object->pre_terrain_speed - object->post_terrain_speed);
            if (!(magdif > 0.6f))
                return;
        emit_debris:
            if (debris_type != -1)
                AddGameDebris(WORLD->debris_sys, debris_type, &object->contact_position);
        }
    }
}

float VehicleTurnOrLoopOffset(GameObject_s *object) {
    if (object->character_context == 0x2a) {
        return (1.0f -
                NU_SIN_LUT((1.0f - object->context_animation_timer / object->airborne_action_duration) * 65536.0f +
                           16384.0f)) *
               0.5f * static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field_0x88 * 0.5f;
    } else if (object->character_context == 0x36) {
        return (1.0f -
                NU_SIN_LUT((1.0f - object->context_animation_timer / object->airborne_action_duration) * 65536.0f +
                           16384.0f)) *
               0.5f * static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field_0x88;
    } else if (object->character_context == 0x3a) {
        if (static_cast<i8>(object->apiobj.flags_low) < 0 && PODSPRINT_ADATA != NULL &&
            PODSPRINT_ADATA == WORLD->area) {
            return NU_SIN_LUT((1.0f - object->context_animation_timer / object->airborne_action_duration) * 32768.0f) *
                   0.25f;
        }
    }
    return 0.0f;
}

i32 WallShuffle_SetTargetMom(GameObject_s *object, u16 input_angle) {
    object->target_velocity.x = NU_SIN_LUT(object->apiobj.movement_facing_angle);
    object->target_velocity.z = NU_COS_LUT(object->apiobj.movement_facing_angle);
    u16 sideways_angle = object->apiobj.movement_facing_angle + 0x4000;
    f32 direction = PushingTowardsAngle(input_angle, sideways_angle);
    f32 speed;
    if (direction > NuTrigTable[0x3555]) {
        if (LEGOACT_WALLSHUFFLE_RIGHT != -1 &&
            object->apiobj.character_model->model_data_b[LEGOACT_WALLSHUFFLE_RIGHT] != NULL) {
            object->context_animation = LEGOACT_WALLSHUFFLE_RIGHT;
            speed = AnimSpeed(object->apiobj.character_model, LEGOACT_WALLSHUFFLE_RIGHT);
        } else {
            speed = 0.5f;
        }
    } else if (direction < -NuTrigTable[0x3555]) {
        if (LEGOACT_WALLSHUFFLE_LEFT != -1 &&
            object->apiobj.character_model->model_data_b[LEGOACT_WALLSHUFFLE_LEFT] != NULL) {
            object->context_animation = LEGOACT_WALLSHUFFLE_LEFT;
            speed = -AnimSpeed(object->apiobj.character_model, LEGOACT_WALLSHUFFLE_LEFT);
        } else {
            speed = -0.5f;
        }
    } else {
        object->context_animation = LEGOACT_WALLSHUFFLE_IDLE;
        return 0;
    }
    if (speed != 0.0f) {
        object->target_velocity.x += NU_SIN_LUT(sideways_angle) * speed;
        object->target_velocity.z += NU_COS_LUT(sideways_angle) * speed;
    }
    return 0;
}

void SetMoveAndAnimateFunctions(u32 model_flag_mask, u32 model_flag_value, u32 game_flag_mask, u32 game_flag_value,
                                i32 movement_type, void *move_function, void *animate_function, void *draw_function) {
    const CHARACTERUPDATEFN move = reinterpret_cast<CHARACTERUPDATEFN>(move_function);
    const CHARACTERUPDATEFN animate = reinterpret_cast<CHARACTERUPDATEFN>(animate_function);
    const CHARACTERUPDATEFN draw = reinterpret_cast<CHARACTERUPDATEFN>(draw_function);

    for (i32 character_index = 0; character_index < CHARCOUNT; ++character_index) {
        CHARACTERDATA &character = CDataList[character_index];
        GAMECHARACTERDATA &game_character = GCDataList[character_index];

        if ((character.model_flags & model_flag_mask) != model_flag_value ||
            (game_character.flags_090 & game_flag_mask) != game_flag_value ||
            (movement_type != -1 && static_cast<i8>(game_character.field275_0x116) != movement_type)) {
            continue;
        }

        if (move != NULL) {
            character.move_fn = move;
        }
        if (animate != NULL) {
            character.animate_fn = animate;
        }
        if (draw != NULL) {
            character.draw_fn = draw;
        }
    }
}

u16 SeekRot(u16 current, u16 target, f32 rate) {
    const f32 blend = MIN(1.0f, rate * FRAMETIME);
    i32 difference = static_cast<i32>(target) - static_cast<i32>(current);
    if (difference > 0x8000) {
        difference -= 0x10000;
    } else if (difference < -0x8000) {
        difference += 0x10000;
    }
    // Original 0x490a3d converts to signed i32 before retaining the low
    // 16 bits.  Interpolation across zero can produce a negative angle.
    const i32 angle = static_cast<i32>(static_cast<f32>(current) + static_cast<f32>(difference) * blend);
    return static_cast<u16>(angle);
}

void SeekVec(NUVEC *result, NUVEC *current, NUVEC *target, f32 rate) {
    const f32 blend = MIN(1.0f, rate * FRAMETIME);
    result->x = current->x + (target->x - current->x) * blend;
    result->y = current->y + (target->y - current->y) * blend;
    result->z = current->z + (target->z - current->z) * blend;
}

u16 TurnRot(u16 current, u16 target, i32 speed, i32 *difference_out) {
    if (target == current) {
        return target;
    }

    const i32 step = static_cast<i32>(static_cast<f32>(speed) * FRAMETIME);
    const i32 difference = RotDiff(current, target);
    if (difference_out != NULL) {
        *difference_out = difference;
    }
    if (difference > 0) {
        if (step < difference) {
            return static_cast<u16>(current + step);
        }
    } else if (difference < -step) {
        return static_cast<u16>(current - step);
    }
    return target;
}

void HoldCode(GameObject_s *object) {
    if (LEGOCONTEXT_HOLD != -1 && object->character_context == LEGOCONTEXT_HOLD) {
        if (object->context_animation_timer > 0.0f)
            object->context_animation_timer -= FRAMETIME;
        else if ((object->pad_gamepad->buttons_held & GAMEPAD_ACTION) == 0)
            object->character_context = -1;
    }
}

f32 SeekValF(f32 current, f32 target, f32 rate) {
    const f32 blend = MIN(1.0f, rate * FRAMETIME);
    return current + (target - current) * blend;
}

void TurnCode(GameObject_s *, i32, GAMEPAD_s *) {
}

void FloatCode(GameObject_s *) {
}

void SlideCode(GameObject_s *object) {
    if (object->field_0x7a5 != 0x33) {
        StartSlide(object, 1);
    } else {
        if (object->apiobj.field_0x27d != 0 && CanObjSlide(object, static_cast<i8>(object->apiobj.field_0x281)) != 0) {
            object->airborne_action_duration = 0.25f;
        } else if (object->airborne_action_duration > 0.0f) {
            object->airborne_action_duration -= FRAMETIME;
            if (object->airborne_action_duration <= 0.0f) {
                object->field_0x7a5 = -1;
            }
        }

        if (object->apiobj.field_0x27d != 0) {
            PlaySfx("Char_Slide_Lp", &object->apiobj.lower_position);
        }
    }
}

void StartHold(GameObject_s *object) {
    if (CanStartHoldFn == NULL)
        return;
    if (CanStartHoldFn(object) == 2) {
        object->hold_timer = 0.0f;
        object->character_context = -1;
        return;
    }
    if (LEGOCONTEXT_HOLD != -1 && NewBlockAction(object) != 0) {
        object->context_animation_timer = 0.3f;
        object->character_context = LEGOCONTEXT_HOLD;
        PlaySabreSfx(NULL, object, NULL, 0);
        return;
    }
    object->character_context = -1;
}

void StartTurn(GameObject_s *) {
}

static void GrabCode(GameObject_s *object) {
    if (object->character_context != 0x38)
        return;
    f32 previous_time = object->context_animation_timer;
    f32 time = FRAMETIME + previous_time;
    object->context_animation_timer = time;
    f32 duration = object->airborne_action_duration;
    if (time >= duration) {
        object->character_context = -1;
        return;
    }
    GameObject_s *target = object->force_target;
    i16 animation = object->context_animation;
    if (animation == 0x1f) {
        if (target == NULL)
            return;
    } else if (target == NULL) {
        object->character_context = -1;
        return;
    }
    if ((object->context_flags & 0x40) != 0)
        return;
    bool hit = false;
    if (object->apiobj.character_model->model_data_b[animation] == NULL) {
        f32 halfway = duration * 0.5f;
        if (halfway > previous_time && time >= halfway)
            hit = true;
    } else {
        f32 *frame = AnimPlaying(&object->apiobj.anim_packet, animation, 1, 0);
        if (frame != NULL) {
            f32 start = AnimListFrame(object->apiobj.character_model, object->context_animation, 1);
            if (start >= 1.0f && *frame >= start) {
                f32 end = AnimListFrame(object->apiobj.character_model, object->context_animation, 2);
                if (end >= 1.0f && end >= *frame)
                    hit = true;
            }
        }
    }
    if (!hit)
        return;
    i32 joint = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->weapon_joints[0];
    if (object->apiobj.field_0x288 != 0 && joint != -1 &&
        object->apiobj.character_model->points_of_interest[joint] != NULL &&
        NuVecDistSqr(NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3), &target->apiobj.collision_position, NULL) >
            0.7f * 0.7f)
        return;
    if (target->id == id_GAMORREANGUARD && object->context_animation == 0x1f) {
        object->context_flags |= 0x40;
        if (target->character_context == 0x1c && target->force_target != NULL) {
            target->force_target->force_target = NULL;
        }
        Player_ClearContext(target, 1);
        Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(target->player_packet));
        target->character_context = 0x39;
        target->context_animation = 5;
        target->force_target = target;
        target->force_throw_target = NULL;
        PlayDieSfx(target);
        object->field_0xe24 |= 1;
    } else if ((static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_090 & 0x8000) ==
               0) {
        object->context_flags |= 0x40;
        GameAudio_PlaySfx(0x4a, &target->apiobj.collision_position, 0, 0);
        objhitobj_noimpactsfx = 1;
        ObjHitObj(object, target, 2, 0, 0, 1);
        u16 angle = NuAtan2D(target->apiobj.collision_position.x - object->apiobj.collision_position.x,
                             target->apiobj.collision_position.z - object->apiobj.collision_position.z);
        f32 momentum = static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->field_0x18;
        f32 sine = NU_SIN_LUT(angle);
        f32 cosine = NU_COS_LUT(angle);
        target->apiobj.velocity.x += sine * momentum;
        target->apiobj.velocity.z += cosine * momentum;
    }
}

extern i16 id_RANCOR;
extern i16 id_DEWBACK;
extern i16 id_BIGGUN;
extern i16 id_BANTHA;
extern i16 id_ATAT;
extern i16 id_CATAPULT;
extern i16 id_FIRETRUCK;
extern i32 TERRAINMASK_NONWEAPON;
void PushAway(NUVEC *, f32, NUVEC *, NUVEC *, GameObject_s *, GameObject_s *, f32, u32);
i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);

struct AWKWARDSHAPESPHERE_s {
    i16 *character_id;
    i16 joint;
    u16 flags;
    f32 radius;
};
DECOMP_ASSERT(sizeof(AWKWARDSHAPESPHERE_s) == 12, "Awkward shape sphere size");
DECOMP_ASSERT(offsetof(AWKWARDSHAPESPHERE_s, radius) == 8, "Awkward shape sphere radius offset");

static AWKWARDSHAPESPHERE_s AwkwardShapeSphere[] = {
    {&id_RANCOR, 4, 0, 0.3f},    {&id_DEWBACK, 1, 1, 0.2f},  {&id_DEWBACK, 2, 0, 0.125f}, {&id_DEWBACK, 3, 0, 0.1f},
    {&id_DEWBACK, 4, 0, 0.075f}, {&id_BIGGUN, 3, 0, 0.175f}, {&id_BIGGUN, 4, 0, 0.175f},  {&id_BIGGUN, 5, 0, 0.175f},
    {&id_BANTHA, 1, 1, 0.25f},   {&id_ATAT, 7, 1, 1.2f},     {&id_CATAPULT, 1, 0, 0.25f}, {&id_FIRETRUCK, 1, 1, 0.3f},
    {&id_FIRETRUCK, 2, 1, 0.3f}, {NULL, 0, 0, 0.0f},
};

static void AwkwardShapeCode(GameObject_s *object, i32) {
    i32 joints[8] = {};
    u32 flags[8] = {};
    f32 radii[8] = {};
    if (object->apiobj.field_0x288 == 0 || object->apiobj.model_draw_result == 0)
        return;
    if (WORLD->area != NULL && (WORLD->area->flags & 1) != 0 && object->id == id_ATAT)
        return;
    i32 count = 0;
    if (object->id == id_SPEEDERBIKE) {
        if (WORLD->current_level != SPEEDERCHASEA_LDATA || disable_narrow_socks == 0)
            return;
        joints[0] = 2;
        radii[0] = 0.325f;
        count = 1;
    } else {
        for (AWKWARDSHAPESPHERE_s *sphere = AwkwardShapeSphere; sphere->character_id != NULL && count < 8; ++sphere) {
            if (*sphere->character_id == object->id) {
                joints[count] = sphere->joint;
                flags[count] = sphere->flags;
                radii[count] = sphere->radius;
                ++count;
            }
        }
    }
    for (i32 i = 0; i < count; ++i) {
        if (object->apiobj.character_model->points_of_interest[joints[i]] != NULL) {
            f32 radius = radii[i];
            NUVEC *position = NUMTX_GET_ROW_VEC(&object->joint_matrices[joints[i]], 3);
            PushAway(position, radius, NULL, NULL, NULL, object, 2.0f, 5);
            if (flags[i] != 0 &&
                (static_cast<i8>(object->apiobj.flags_low) < 0 || static_cast<i8>(object->field_0xf00) < 0)) {
                NUVEC displacement;
                NUVEC extension;
                NUVEC hit_position;
                NuVecSub(&displacement, position, &object->apiobj.collision_position);
                NuVecNorm(&extension, &displacement);
                NuVecScale(&extension, &extension, radius);
                NuVecAdd(&displacement, &displacement, &extension);
                if (GameRayCast(&object->apiobj.collision_position, &displacement, radius * 0.5f,
                                TERRAINMASK_NONWEAPON | 0x1f)) {
                    NuVecAdd(&hit_position, &object->apiobj.collision_position, &displacement);
                    PushAway(&hit_position, 10.0f, NULL, NULL, object, NULL, 3.0f, 1);
                }
            }
        }
    }
}

static void CommunicateCode(GameObject_s *object, i32 pressed, i32) {
    i8 context = object->character_context;
    if (context != 0x1a) {
        if (pressed == 0)
            return;
        if (context != -1)
            return;
        if (static_cast<i8>(
                static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_094[0]) >= 0)
            return;
        if (object->apiobj.character_model->model_data_b[0x6b] == NULL)
            return;
        object->context_animation_timer = 0.0f;
        object->character_context = 0x1a;
        object->context_animation = 0x6b;
        object->airborne_action_duration = AnimDuration(object->id, 0x6b, 0.0f, 0.0f, 1);
        return;
    }
    f32 *frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
    if (frame == NULL)
        return;
    f32 *events = AnimListFrameArray(object->apiobj.character_model, object->context_animation);
    if (events != NULL && events[0] >= 1.0f && events[1] >= 1.0f && events[2] >= 1.0f && events[3] >= 1.0f) {
        f32 time = *frame;
        if (!(events[0] >= time)) {
            if (time < events[1])
                object->communicate_blend = (time - events[0]) / (events[1] - events[0]);
            else if (time < events[2])
                object->communicate_blend = 1.0f;
            else if (time < events[3])
                object->communicate_blend = 1.0f - (time - events[2]) / (events[3] - events[2]);
            else
                object->communicate_blend = 0.0f;
        } else
            object->communicate_blend = 0.0f;
    }
    object->context_animation_timer += FRAMETIME;
    if (object->context_animation_timer >= object->airborne_action_duration) {
        object->character_context = -1;
        object->communicate_blend = 0.0f;
        if (static_cast<i8>(object->apiobj.flags_low) < 0 && Cheat_IsOn(10)) {
            GameObject_s *target = Obj;
            for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++target) {
                if (ZapTarget(target) && object != target && target->apiobj.field_0x27c == -1 &&
                    (target->field_0xefb & 8) == 0 && !CannotKill(target)) {
                    u32 flags =
                        static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24)->flags_090;
                    if ((flags & 0x40) != 0)
                        continue;
                    i8 context = target->character_context;
                    if (context == 0xf)
                        continue;
                    if (context == 0x3c)
                        continue;
                    if (context == 0x47)
                        continue;
                    if (context == 0x46)
                        continue;
                    if ((flags & 0x8000) == 0 &&
                        NuVecDistSqr(&object->apiobj.collision_position, &target->apiobj.collision_position, NULL) <
                            9.0f &&
                        DeactivatePlayer(target, DEACTIVATEDTIME, object)) {
                        PlaySfx("R2Zap", &target->apiobj.collision_position);
                        AddGameDebris(WORLD->debris_sys, 1, &target->apiobj.collision_position);
                        GameCam_HitJudder();
                        target->action_movement_state = 2;
                        f32 random = static_cast<f32>(qrand()) * (1.0f / 65535.0f);
                        target->context_variant_flags |= 1;
                        target->field_0x768 = random * 0.3f + 0.2f;
                        return;
                    }
                }
            }
        }
    }
}

static __used__ void PunchCode(GameObject_s *, i32, i32, i32, i32, f32) {
}

static __used__ void ShootThisFrame(GameObject_s *object, i32 bolt_id, i32 flags) {
    if (object == Player[0] && nextShootTarget.Get() != NULL)
        nextShootTarget = NuMechPtr<MechObjectInterface, 4>();
    if ((object->apiobj.field_0x1f4 & 0x40000) != 0 && object->apiobj.field_0x27c != -1)
        return;
    NewBuzzFrames(object->pad_gamepad->pad, (object->apiobj.character_data->model_flags & 0x2000) != 0 ? 1 : 2, 0);
    object->field_0xef9 |= 8;
    object->quick_shoot_bolt_id = bolt_id;
    object->quick_shoot_flags = flags;
    if ((object->apiobj.character_data->game_character->flags_098[0] & 2) != 0) {
        extern void SetWeaponIn(GameObject_s *);
        SetWeaponIn(object);
    }
    if (object->field_0x7e4 != NULL && object->field_0x7e4[8] == 2 && object->field_0x7e8 != 0)
        --object->field_0x7e8;
}

i32 PlayerItem_GotAmmo(PLAYERITEM_s *);
i32 UnderPlayerControl(GameObject_s *);
extern "C" f32 animduration_blendouttime;
extern i16 id_GEONOSIAN, id_MINIATST, id_ATST_LOWRES, id_ATAT, id_MINIATAT, id_MINIATTE, id_SENTRYDROID;
void Move_CANNON(GameObject_s *);
void SetWeaponIn(GameObject_s *);
GIZMOBLOWUP_s *GizmoBlowUp_Target(GameObject_s *, NUVEC *, NUVEC *, f32, f32, i32, i32, i32);
i32 GizmoSys_SetBestBoltTarget(GIZMOSYS *, void *, GameObject_s *, NUVEC *, NUVEC *, f32, f32, i32, i32, i32);

static __used__ i32 ShootCode(GameObject_s *object, i32 pressed, i32 special_pressed, i32 weapon_mode,
                              i32 allow_airborne, i32 fire_mode) {
    GameObject_s *target = NULL;
    GIZMOBLOWUP_s *blowup = NULL;
    if (object == Player[0] && nextShootTarget.Get() != NULL && pressed != 0) {
        target = nextShootTarget->GetCharacterObject();
        if (object == Player[0] && nextShootTarget.Get() != NULL)
            blowup = nextShootTarget->GetGizBlowup();
    }
    const i32 bolt_id =
        object->id == id_GEONOSIAN && (object->field_0xefd & 2) != 0 ? 0x13 : BoltType_FindIDByCreature(object, 0);
    if (object->quick_shoot_timer > 0.0f)
        object->quick_shoot_timer -= FRAMETIME;
    if (object->character_context == 0x0a) {
        if (AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0) == NULL)
            return 0;
        const bool ammunition = PlayerItem_GotAmmo(reinterpret_cast<PLAYERITEM_s *>(&object->field_0x7e4)) != 0;
        if (ammunition && pressed != 0 && !(object->quick_shoot_timer > 0.0f) && object->context_animation == 0x57 &&
            object->action_suppressed <= 2) {
            if (object->pad_gamepad->input_magnitude > 0.0f) {
                const u16 angle = GamePad_InputAngle(object, object->pad_gamepad);
                object->apiobj.movement_facing_angle = angle;
                object->apiobj.facing_angle = angle;
                object->apiobj.field_0x276 = angle;
            }
            StartQuickShoot(object, 0x57);
            if (object->action_suppressed == 3)
                object->quick_shoot_timer = 0.5f;
            return 0;
        }
        if (static_cast<u16>(object->context_animation - 0x5a) <= 2) {
            if ((object->field_0xe22 & 4) == 0) {
                const f32 event_frame = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                if (event_frame > 1.0f) {
                    const f32 frame = object->apiobj.anim_packet.blending != 0
                                          ? object->apiobj.anim_packet.blend_target_time
                                          : object->apiobj.anim_packet.current_time;
                    if (frame >= event_frame) {
                        if (ammunition) {
                            ShootThisFrame(object, bolt_id, 1);
                            object->field_0xe21 |= 8;
                        }
                        object->field_0xe22 |= 4;
                    }
                }
            }
        } else if (object->reserved_e30 == 0) {
            const f32 event_frame = AnimListFrame(object->apiobj.character_model, object->context_animation, 1);
            if (event_frame > 1.0f) {
                const f32 frame = object->apiobj.anim_packet.blending != 0
                                      ? object->apiobj.anim_packet.blend_target_time
                                      : object->apiobj.anim_packet.current_time;
                if (frame >= event_frame) {
                    if (ammunition)
                        ShootThisFrame(object, bolt_id, 2);
                    object->reserved_e30 = 1;
                }
            }
        }
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer > 0.0f)
            return 0;
        object->field_0xe21 &= ~8;
        object->character_context = -1;
        if (static_cast<u16>(object->context_animation - 0x5a) <= 2 && (object->field_0xe22 & 4) == 0 && ammunition)
            ShootThisFrame(object, bolt_id, 1);
        return 0;
    }

    i32 context = object->character_context;
    if ((object->field_0xef8 & 8) == 0 || pressed == 0 || (CInfo[context].flags & 0x20) != 0 ||
        (static_cast<i8>(object->apiobj.flags_low) >= 0 &&
         (context == 6 || context == 7 ||
          (context == 1 && (object->context_animation == 0xb3 || object->context_animation == 0x59))))) {
        if (weapon_mode == 0 || ((object->pad_gamepad->allocated_5a & 4) == 0 && special_pressed == 0) ||
            (object->field_0xe22 & 1) == 0 || object->weapon_scale_state != 0 || context == 6 || context == 7 ||
            context == 0x47 || context == 0x46 || context == 0x0b || context == 0x2e ||
            (object->apiobj.character_data->game_character->uses_weapon_action == 2 && object->field_0xe31 == 1) ||
            context == 8 || context == 0x1b || context == 0x1d || TouchHacks::ShouldKeepWeaponOut(*object))
            return 0;
        if (weapon_mode == 2) {
            SetWeaponIn(object);
        } else {
            CHARACTERDATA *data = object->apiobj.character_data;
            const i32 animation =
                data->game_character->uses_weapon_action == 0 && (data->model_flags & 0x80) != 0 ? 0x7e : 0x10;
            if (object->apiobj.field_0x27d != 0 && object->pad_gamepad->input_magnitude == 0.0f &&
                object->apiobj.character_model->model_data_b[animation] != NULL &&
                (object->character_context == -1 || (CInfo[object->character_context].flags & 4) != 0))
                SlowWeaponIn(object);
            else
                FastWeaponIn(object, 1);
        }
        return 0;
    }
    if (static_cast<i8>(object->apiobj.flags_low) < 0 && (context == 6 || context == 7) &&
        (object->apiobj.character_data->game_character->flags_094[1] & 0x10) != 0)
        return 0;
    if (BonusWinner != -1)
        return 0;
    if (weapon_mode != 0 && (object->field_0xe22 & 1) == 0) {
        if (weapon_mode == 2) {
            SetWeaponOut(object);
        } else {
            CHARACTERDATA *data = object->apiobj.character_data;
            const i32 animation =
                data->game_character->uses_weapon_action == 0 && (data->model_flags & 0x80) != 0 ? 0x7f : 0x11;
            if (object->weapon_scale_state == 0 && object->apiobj.field_0x27d != 0 &&
                object->pad_gamepad->input_magnitude == 0.0f &&
                object->apiobj.character_model->model_data_b[animation] != NULL &&
                (context == -1 || (CInfo[context].flags & 4) != 0))
                SlowWeaponOut(object);
            else
                FastWeaponOut(object, 1);
            return 0;
        }
    }
    if (object->character_context == -1 && object->field_0xe31 == 1)
        fire_mode = 2;
    if (!PlayerItem_GotAmmo(reinterpret_cast<PLAYERITEM_s *>(&object->field_0x7e4)) ||
        object->quick_shoot_timer > 0.0f || object->apiobj.field_0x287 != 0)
        return 0;
    if (fire_mode == 0 && object->apiobj.field_0x27d == 0) {
        if (object->character_context != 0 || allow_airborne == 0 ||
            (object->apiobj.character_data->game_character->flags_094[2] & 0x40) != 0)
            return 0;
    } else if (object->character_context == 0x25) {
        return 0;
    }
    if (object->character_context == 0 && object->jump_sequence > 1)
        return 0;
    if (Cheat_IsOn(0x0f)) {
        const u8 kind = object->apiobj.character_data->game_character->uses_weapon_action;
        if (kind == 8 || kind == 1)
            return 0;
    }
    const i16 previous_animation = object->context_animation;
    object->context_animation = fire_mode == 2 ? 0x3c : 0x16;
    if (UnderPlayerControl(object)) {
        NUVEC position = object->apiobj.collision_position;
        NUVEC direction;
        BoltSys->shoot_direction(object, &direction);
        NUVEC forward = direction;
        const f32 speed = BoltType_FindByID(bolt_id, WORLD)->field_10;
        const f32 range = speed * BoltType_FindByID(bolt_id, WORLD)->field_14;
        const f32 range_squared = range * range;
        if (target == NULL && blowup == NULL)
            target = TargetGameObject(object, &position, &direction, range, range_squared, 0, 1, 0, bolt_id);
        if (target == NULL) {
            if (static_cast<i8>(object->apiobj.flags_low) < 0 && object->pad_gamepad->input_magnitude == 0.0f &&
                object->apiobj.field_0x27d != 0 && object->character_context != 0) {
                if (object->apiobj.character_model->model_data_b[0x5b] != NULL &&
                    AnimPlaying(&object->apiobj.anim_packet, 0x5b, 1, 1) == NULL) {
                    direction.x = forward.z;
                    direction.z = -forward.x;
                    target = TargetGameObject(object, &position, &direction, range, range_squared, 0, 1, 1, bolt_id);
                    if (target != NULL)
                        object->context_animation = 0x5b;
                }
                if (target == NULL && object->apiobj.character_model->model_data_b[0x5a] != NULL &&
                    AnimPlaying(&object->apiobj.anim_packet, 0x5a, 1, 1) == NULL) {
                    direction.x = -forward.z;
                    direction.z = forward.x;
                    target = TargetGameObject(object, &position, &direction, range, range_squared, 0, 1, 1, bolt_id);
                    if (target != NULL)
                        object->context_animation = 0x5a;
                }
                if (target == NULL && object->apiobj.character_model->model_data_b[0x5c] != NULL &&
                    AnimPlaying(&object->apiobj.anim_packet, 0x5c, 1, 1) == NULL) {
                    direction.x = -forward.x;
                    direction.z = -forward.z;
                    target = TargetGameObject(object, &position, &direction, range, range_squared, 0, 1, 1, bolt_id);
                    if (target != NULL)
                        object->context_animation = 0x5c;
                }
            }
            if (target == NULL) {
                if (blowup != NULL) {
                    SetGizmoBlowUpTarget(object, blowup);
                } else if (!GizmoSys_SetBestBoltTarget(WORLD->gizmo_sys, WORLD, object, &position, &forward, range,
                                                       range_squared, 1, 0, bolt_id)) {
                    blowup = GizmoBlowUp_Target(object, &position, &forward, range, range_squared, 1, 0, bolt_id);
                    if (blowup != NULL) {
                        SetGizmoBlowUpTarget(object, blowup);
                    } else {
                        PART_s *part = TargetPart(object, &position, &forward, range, range_squared, 1, bolt_id);
                        if (part != NULL)
                            SetPartTarget(object, part);
                        else
                            target = TargetGameObject(object, &position, &forward, range, range_squared, 0x200, 1, 0,
                                                      bolt_id);
                    }
                }
            }
        }
        if (target != NULL)
            SetObjTarget(object, target);
    }
    object->quick_shoot_timer = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 0);
    object->field_0xe22 &= ~4;
    object->reserved_e30 = 0;
    if ((object->id == id_ATST || object->id == id_MINIATST || object->id == id_ATST_LOWRES || object->id == id_ATAT ||
         object->id == id_MINIATAT || object->id == id_MINIATTE) &&
        AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 1) == NULL &&
        AnimPlaying(&object->apiobj.anim_packet, 1, 0, 0) == NULL)
        goto immediate_shot;
    if (static_cast<u16>(object->context_animation - 0x5a) <= 2) {
        if (AnimListFrame(object->apiobj.character_model, object->context_animation, 0) > 1.0f) {
            object->context_animation_timer = object->quick_shoot_timer - animduration_blendouttime;
            if (object->context_animation_timer > 0.0f) {
                object->character_context = 0x0a;
                goto cooldown;
            }
            object->quick_shoot_timer = 0.0f;
            goto finish;
        }
    } else {
        GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
        if ((data->flags_098[0] & 4) == 0 &&
            ((data->uses_weapon_action == 4 && object->apiobj.anim_packet.animation_index == 3) ||
             (object->apiobj.anim_packet.animation_index == 0x17 &&
              object->apiobj.character_model->model_data_b[0x17] != NULL) ||
             (data->uses_weapon_action == 0 && object->apiobj.anim_packet.animation_index == 0x73 &&
              object->apiobj.character_model->model_data_b[0x73] != NULL) ||
             (object->apiobj.anim_packet.animation_index == 6 &&
              object->apiobj.character_model->model_data_b[6] != NULL)))
            goto immediate_shot;
    }
    {
        const f32 duration = object->quick_shoot_timer - animduration_blendouttime;
        if (duration > 0.0f) {
            object->character_context = 0x0a;
            object->context_animation_timer = duration;
            ResetAnimPacket(&object->apiobj.anim_packet, -1);
            object->apiobj.anim_packet.flags |= 0x10;
            SetWeaponOut(object);
            if (object->id == id_CATAPULT) {
                NewRumble(object->pad_gamepad->pad, 0.6f, 0);
                GameCam_NewShake(GameCam, 0.5f, 0.5f, 1.0f);
            }
        }
        if ((object->apiobj.character_data->game_character->flags_098[0] & 8) == 0) {
            const i32 flags =
                duration > 0.0f
                    ? 1 + 2 * !(AnimListFrame(object->apiobj.character_model, object->context_animation, 1) > 1.0f)
                    : 3;
            ShootThisFrame(object, bolt_id, flags);
        }
    }
    goto cooldown;
immediate_shot:
    if ((object->apiobj.character_data->game_character->flags_098[0] & 8) == 0) {
        ShootThisFrame(object, bolt_id, 3);
        object->apiobj.velocity.x *= 0.5f;
        object->apiobj.velocity.z *= 0.5f;
    }
cooldown:
    if (object->quick_shoot_timer <= 0.0f) {
        object->quick_shoot_timer = (qrand() * (1.0f / 65535.0f)) * 0.2f + 0.2f;
        if (object->id == id_SENTRYDROID)
            object->quick_shoot_timer *= 5.0f;
    }
    if (static_cast<i8>(object->apiobj.flags_low) < 0 && target != NULL && target->apiobj.field_0x27c == -1)
        Hint_SetComplete(0x277);
finish:
    if (object->character_context == 0x0a) {
        object->context_flags &= ~0x40;
        if (object->apiobj.character_data->move_fn == Move_CANNON) {
            object->apiobj.facing_angle = object->apiobj.field_0x276;
            object->apiobj.movement_facing_angle = object->apiobj.field_0x276;
        }
    } else {
        object->context_animation = previous_animation;
    }
    return 1;
}

static __used__ void DodgeCode(GameObject_s *, i32, i32) {
}

extern i16 id_EWOK;
extern i16 id_WICKET;
extern i16 id_CAPTAINTARPALS;
extern i16 id_SKELETON;
void StartJetPackFall(GameObject_s *, i32);
void KillGameObject(GameObject_s *, i32, i32);

void Move_CHARACTER(GameObject_s *object) {
    i32 jump = GAMEPAD_JUMP;
    i32 action = GAMEPAD_ACTION;
    i32 special = GAMEPAD_SPECIAL;
    i32 tag = GAMEPAD_TAG;
    GAMEPAD_s *pad = object->pad_gamepad;
    i32 pressed = pad->buttons_pressed;
    i32 held = pad->buttons_held;
    if (object->id == id_GAMORREANGUARD || object->id == id_EWOK || object->id == id_WICKET ||
        object->id == id_CAPTAINTARPALS)
        KeepWeaponOut(object);
    DropInOutCode(object);
    if (BonusArea && VehicleArea && (object->apiobj.character_data->model_flags & 0x4000000) != 0 &&
        object->torpedo != NULL)
        object->torpedo->count = 0;
    if ((object->field_0xe20 & 0x20) != 0)
        return;

    CHARACTERDATA *character = object->apiobj.character_data;
    GAMECHARACTERDATA *runtime = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
    bool jetpack_airborne = false;
    if (static_cast<i8>(object->apiobj.flags_low) < 0 && runtime->uses_weapon_action == 2 &&
        object->character_context != 0x2b && object->field_0xe31 == 1) {
        f32 time = object->apiobj.horizontal_velocity_magnitude / runtime->movement_speed * FRAMETIME +
                   object->jump_variant_timer;
        object->jump_variant_timer = time;
        if (object->apiobj.field_0x27d != 0) {
            object->field_0xe31 = 0;
            object->character_context = -1;
        } else if (time >= runtime->field_0x48 || object->apiobj.field_0x281 == 0x1a) {
            StartJetPackFall(object, 0);
            character = object->apiobj.character_data;
        } else {
            object->apiobj.velocity.y = 0.0f;
            jetpack_airborne = true;
        }
    }
    if (!jetpack_airborne) {
        runtime = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
        if (runtime->uses_weapon_action == 2 && object->field_0xe31 == 1) {
            ApplyGravity(object, NULL, runtime->field_0x28, 8.0f, NULL);
        } else
            ApplyGravity(object, NULL, 0.0f, 8.0f, NULL);
    }
    if (object->suit != NULL)
        Signal_MoveCode(WORLD, object);
    TakeHitCode(object);
    FloatCode(object);
    SlideCode(object);
    FlattenCode(object);
    Hang_MoveCode(object);
    Ledge_MoveCode(WORLD, object);
    if (LedgeTerrain_On)
        LedgeTerrain_MoveCode(object);
    i32 jump_pressed = jump & pressed;
    Climb_MoveCode(object);
    TightRope_MoveCode(object, jump_pressed);
    ForcePushed_MoveCode(object);
    ForcedBackCode(object);
    Tube_MoveCode(object, WORLD);
    DeactivatedCode(object);
    PushCode(object, 1);
    BackFlipCode(object);
    if (object->id != id_SKELETON)
        TakeOverCode(object, tag & pressed);
    i32 action_pressed = action & pressed;
    i32 action_held = action & held;
    Glide_MoveCode(object);
    runtime = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    if (runtime->uses_weapon_action == 2 && object->field_0xe31 == 1) {
        JetPackCode(object, jump_pressed, 0, 0);
    } else if ((runtime->flags_094[0] & 4) == 0) {
        u32 animations = object->id == id_GAMORREANGUARD ? 0x20 : 8;
        if ((runtime->flags_090 & 0x400000) != 0)
            animations |= 0x101;
        JumpCode(object, jump_pressed, jump & held, animations, action_pressed, action_held, -1);
    }
    i32 special_pressed = special & pressed;
    GizPanel_MoveCode(WORLD, object, special_pressed);
    HatMachine_MoveCode(WORLD, object, special_pressed);
    ZipUp_MoveCode(object, special_pressed);
    if ((object->apiobj.character_data->model_flags & 0x100000) != 0)
        Grapple_MoveCode(object);
    BuildIt_MoveCode(object);
    Lever_MoveCode(WORLD, object);
    ThermalDetonator_MoveCode(object);
    if ((object->apiobj.character_data->model_flags & 0x40000) != 0)
        Teleport_MoveCode(object, special_pressed);
    SuperCarry_MoveCode(WORLD, object);
    GizmoBlowupCheckProximity(WORLD, object);
    WeaponOutCode(object);
    WeaponInCode(object);
    WeaponScalingCode(object);
    SpecialMove_VictimCode(object);
    HoldCode(object);
    BlockCode(object, action_pressed, action_held, 0, (object->apiobj.character_data->model_flags & 0x80) == 0);
    PunchCode(object, action_pressed, action_held, object->apiobj.character_data->model_flags & 0x80, 0, 0.0f);
    DodgeCode(object, action_pressed, jump_pressed);
    Attracto_MoveCode(WORLD, object);
    SecurityDoor_MoveCode(WORLD, object);
    if ((object->apiobj.character_data->model_flags & 0x80) != 0) {
        ShootCode(object, action_pressed, special_pressed, 1, 1, 0);
    } else if (object->batarang != NULL)
        Batarang_MoveCode(object);
    if (static_cast<i8>(static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_094[0]) <
        0) {
        CommunicateCode(object, GAMEPAD_SPECIAL & object->pad_gamepad->buttons_pressed, 0);
    }
    if (object->character_context == 0x14) {
        f32 *frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
        if (frame != NULL) {
            if ((object->field_0xe22 & 4) == 0) {
                f32 event = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                if (event >= 1.0f && event <= *frame) {
                    object->field_0xe22 |= 4;
                    object->field_0xe20 |= 0x10;
                    PlaySfx("JgRocket", &object->apiobj.collision_position);
                }
            }
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f)
                object->character_context = -1;
        }
    } else if (object->character_context == -1 && (object->pad_gamepad->buttons_pressed & GAMEPAD_TAG) != 0 &&
               ((object->apiobj.character_data->model_flags & 0x1000000) != 0 || object->field_0x108e == 6)) {
        FireBountyHunterRocket(object);
    }
    if (object->character_context == -1 && object->field_0xe31 == 0 && object->apiobj.field_0x27d != 0 &&
        object->apiobj.field_0x27e == 0) {
        if ((object->movement_runtime_flags & 4) != 0 ||
            (object->fall_animation_timer >= 0.2f && (object->pad_gamepad->input_magnitude == 0.0f ||
                                                      (static_cast<i8>(object->apiobj.flags_low) >= 0 &&
                                                       object->apiobj.character_model->model_data_b[0x59] != NULL))))
            StartFallLand(object, -1);
    }
    if (object->apiobj.field_0x27d != 0)
        object->movement_runtime_flags &= ~4;
    if (object->character_context == 0x3f) {
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer > 0.0f) {
            f32 *frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
            if (frame != NULL) {
                f32 event = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                if (event >= 1.0f && event <= *frame) {
                    if (object->force_target != NULL)
                        KillGameObject(object->force_target, 2, 0);
                    object->field_0xe24 &= ~1;
                    object->context_flags |= 0x40;
                }
            }
        } else {
            object->character_context = -1;
            if ((object->context_flags & 0x40) == 0) {
                if (object->force_target != NULL)
                    KillGameObject(object->force_target, 2, 0);
                object->field_0xe24 &= ~1;
                object->context_flags |= 0x40;
            }
        }
    }
    GrabCode(object);
    if (object->id == id_RANCOR)
        AwkwardShapeCode(object, 0);
    HeadMovement(object);
    CloakMovement(object);
    HairMovement(object);
}

static __used__ void PooCode(GameObject_s *) {
}

void Buck_MoveCode(GameObject_s *, i32);

void Move_BEAST(GameObject_s *object) {
    DropInOutCode(object);
    if ((object->field_0xe20 & 0x20) != 0)
        return;
    ApplyGravity(object, NULL, 0.0f, 8.0f, NULL);
    KeepWeaponOut(object);
    TakeHitCode(object);
    FloatCode(object);
    SlideCode(object);
    FlattenCode(object);
    ForcePushed_MoveCode(object);
    ForcedBackCode(object);
    Buck_MoveCode(object, 0);
    DeactivatedCode(object);
    if (object->character_context == -1 && object->field_0xe31 == 0 && object->apiobj.field_0x27d != 0 &&
        object->apiobj.field_0x27e == 0) {
        if ((object->movement_runtime_flags & 4) != 0 ||
            (object->fall_animation_timer >= 0.2f && (object->pad_gamepad->input_magnitude == 0.0f ||
                                                      (static_cast<i8>(object->apiobj.flags_low) >= 0 &&
                                                       object->apiobj.character_model->model_data_b[0x59] != NULL))))
            StartFallLand(object, -1);
    }
    if (object->apiobj.field_0x27d != 0)
        object->movement_runtime_flags &= ~4;
    if (object->character_context == 0x3f) {
        object->context_animation_timer -= FRAMETIME;
        bool hit = false;
        if (object->context_animation_timer <= 0.0f) {
            object->character_context = -1;
            hit = (object->context_flags & 0x40) == 0;
        } else {
            f32 *frame = AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0);
            if (frame != NULL) {
                f32 event = AnimListFrame(object->apiobj.character_model, object->context_animation, 0);
                hit = event >= 1.0f && event <= *frame;
            }
        }
        if (hit) {
            if (object->force_target != NULL)
                KillGameObject(object->force_target, 2, 0);
            object->field_0xe24 &= ~1;
            object->context_flags |= 0x40;
        }
    }
    GrabCode(object);
    PunchCode(object, GAMEPAD_ACTION & object->pad_gamepad->buttons_pressed, 0, 0, 1,
              static_cast<i8>(object->apiobj.flags_low) < 0 ? 1.0f : 2.0f);
    PooCode(object);
    AwkwardShapeCode(object, 0);
    GizmoBlowupCheckProximity(WORLD, object);
}

void ResetForceBack() {
    ForceBackObj = NULL;
    ForceBackPos = NULL;
}

void SetForceBack(GameObject_s *object, nuvec_s *position, float radius, i32 type) {
    ForceBackRadius = radius;
    ForceBackObj = object;
    ForceBackPos = object != NULL ? &object->apiobj.collision_position : position;
    ForceBackRadius2 = radius * radius;
    ForceBackType = type;
}

void DrawForceBackEffect(nuhspecial_s *special) {
    if (special == NULL || !NuSpecialExistsFn(special)) {
        return;
    }
    if (ForceBackObj != NULL && ForceBackType != 3) {
        NuSpecialSetVisibility(special, 1);
        NUMTX matrix = *NuSpecialGetDrawMtx(special);
        NUVEC position;
        position.x = ForceBackObj->apiobj.lower_position.x;
        position.y = 0.005f + ForceBackObj->apiobj.field_0x218;
        position.z = ForceBackObj->apiobj.lower_position.z;
        NUVEC scale;
        scale.x = scale.y = scale.z = ForceBackRadius;
        NuMtxSetTranslation(&matrix, &position);
        NuMtxPreScale(&matrix, &scale);
        NuSpecialSetDrawMtx(special, &matrix);
        NuSpecialUpdate(special);
    } else {
        NuSpecialSetVisibility(special, 0);
    }
}
