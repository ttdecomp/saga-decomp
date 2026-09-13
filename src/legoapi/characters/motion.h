#ifndef LEGOAPI_CHARACTERS_MOTION_H
#define LEGOAPI_CHARACTERS_MOTION_H

extern int ForcePush_Waft;
extern int ForcePush_SuperPush;
extern int ForcePush_SuperMindTrick;
extern float DIEAIRSPEED;
extern float DIEAIRJUMPSPEED;

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/numechptr.hpp"

// Character motion / animation / camera helpers (module legoapi/characters).
void Move_BEAST(GameObject_s *object);
void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode);
void GameCam_Judder(GAMECAMERA_s *camera, f32 amount, i32 axis, NUVEC *source);
void SetHeadTarget(GameObject_s *object, NUVEC *position, i8 priority, f32 time, f32 minimum_delay, f32 maximum_delay);
void PushAway(NUVEC *position, f32 radius, NUVEC *minimum, NUVEC *maximum, GameObject_s *object,
              GameObject_s *excluded, f32 strength, u32 flags);
i32 SetObjTarget(GameObject_s *object, GameObject_s *target);
i32 SetPartTarget(GameObject_s *object, PART_s *target);
PART_s *TargetPart(GameObject_s *, NUVEC *, NUVEC *, f32, f32, i32, i32);
GameObject_s *TargetGameObject(GameObject_s *, NUVEC *, NUVEC *, f32, f32, u32, i32, i32, i32);
i32 SetGizmoBlowUpTarget(GameObject_s *object, GIZMOBLOWUP_s *target);
f32 Bolt_ObjTargetPosYAdjust(GameObject_s *object);
i32 BoltType_FindIDByCreature(GameObject_s *object, i32 fallback);
i32 GetShootDirection_LSW(GameObject_s *, NUVEC *);
extern CHARPIVOT CharPivot_LSW[32];
void CharPivot_Init(CHARPIVOT *table);
void CharPivot_Check(GameObject_s *object, NUVEC *velocity);
extern i32 LEGOCONTEXT_TUBE;
i32 ObjInTube(GameObject_s *object);
i32 MovingBackwards(GameObject_s *object);
i32 Conveyor_AdjustSpeed(NUVEC *velocity);
extern i32 LEGOCONTEXT_PUSHSPINNER;
extern i32 LEGOCONTEXT_PUSHOBSTACLE;
extern i16 LEGOACT_WALLSHUFFLE_LEFT;
extern i16 LEGOACT_WALLSHUFFLE_RIGHT;
extern i16 LEGOACT_WALLSHUFFLE_IDLE;
extern i16 LEGOACT_HANG_MOVE;
extern f32 LEDGETERRAINLOOKAHEAD;
i32 Hang_SetTargetMom(GameObject_s *object);
i32 LedgeTerrain_SetTargetMom(GameObject_s *object);
i32 WallShuffle_SetTargetMom(GameObject_s *object, u16 input_angle);
f32 PushingTowardsAngle(u16 input_angle, u16 direction);
extern i16 LEGOACT_CLIMB_IDLE;
extern i16 LEGOACT_CLIMB_UP;
extern i16 LEGOACT_CLIMB_DOWN;
extern i16 LEGOACT_CLIMB_LEFT;
extern i16 LEGOACT_CLIMB_RIGHT;
extern i16 LEGOACT_MAGNET_WALK_METAL;
extern i16 LEGOACT_IDLE;
extern i16 LEGOACT_WALK;
extern i32 LEGOCONTEXT_CLIMB;
extern i32 LEGOCONTEXT_LEDGETERRAIN;
extern i16 LEGOACT_LEDGE_GRAB;
extern i16 LEGOACT_LEDGE_IDLE;
extern i16 LEGOACT_LEDGE_LEFT;
extern i16 LEGOACT_LEDGE_RIGHT;
extern f32 MAGNETOFFSET;
i32 Climb_SetTargetMom(GameObject_s *object, u16 input_angle);
i32 Glide_SetTargetMom(GameObject_s *object);
i32 Slide_SetTargetMom(GameObject_s *object, u16 input_angle, f32 input_speed);
extern i16 LEGOACT_SUPERCARRY_WALK;
i32 SuperCarry_SetTargetMom(GameObject_s *object, f32 input_speed);
i32 SuperCarry_YRotation(GameObject_s *object, u16 input_angle);
i32 TightRope_SetTargetMom(GameObject_s *object);
void Climb_UpdateMagnetRotation(GameObject_s *object);
f32 SpecialMove_GetDistanceApart(i32 index);
void SpecialMove_Attacker_SetTargetMom(GameObject_s *object);
NUVEC *Grabber_GetGrabPos(GRABBER_s *grabber, NUMTX *matrix);
i32 CanClimbSurface(GameObject_s *object, i32 surface);
i32 Pushing(GameObject_s *object, u16 *normal_angle, i32 *surface, i32 *angle_difference);
void SetPushAngle(GameObject_s *object);
GAMEPAD_s *ViewCamGetGamePad();
NUVEC *ViewCamGetTgt();
i32 ViewCamGetMode();
void ViewCamSetActive(i32 mode, GAMEPAD_s *gamepad);
extern MechObjectInterface *forceNextAttackOpponent;
extern NuMechPtr<MechObjectInterface, 4> nextShootTarget;
void ForceNextShootTarget(MechObjectInterface &target);
extern f32 ComboOpponent_Range2;
extern i32 ComboOpponent_Behind;
extern f32 PlayerOpponent_Range2;
extern f32 GizmoBlowUpOpponent_Range2;
extern i32 GizmoBlowUpOpponent_Behind;
GameObject_s *ObjOpponent(GameObject_s *, f32, f32, i32, i32, i32);
GIZMOBLOWUP_s *GizmoBlowUpOpponent(GameObject_s *, f32, f32, f32, i32, u32, u32, u32);

i32 CanObjSlide(GameObject_s *object, i32 surface);
i32 StartSlide(GameObject_s *object, i32 check_contact);

#ifdef __cplusplus
extern "C" {
#endif
    float AnimEndFrame(void *animset, i32 unknown);
#ifdef __cplusplus
}
#endif
float SeekLinearF(float current, float target, float step);
float SeekValF(float current, float target, float step);
i32 RotDiff(u16 current, u16 target);
i32 ObjLandReady(GameObject_s *object);
i32 objInNetWaitContext(GameObject_s *object, i32 context);
u16 TurnRot(u16 current, u16 target, i32 speed, i32 *difference);
u16 SeekRot(u16 current, u16 target, float rate);
void SeekVec(NUVEC *result, NUVEC *current, NUVEC *target, float rate);
void GameCam_NewShake(GAMECAMERA_s *camera, float amount, float duration, float speed);
void GameCam_HitJudder(void);
void GameCam_Reset(GAMECAMERA_s *camera);
void ChrisAllocLevelStuff(WORLDINFO_s *world);
void PodKeyReset(void);
void StartLaunch(GameObject_s *object);
void PodLoseSpeed(GameObject_s *obj, i32 type, i32 unknown);
void DrawMeleeTargets(i16 *targets, char *icon, float *a, i32 count);
extern i32 LEGOCONTEXT_BIGJUMP;
extern i16 LEGOACT_COMBOJUMP;
extern void (*BigJump_EndOfLandFn)(GameObject_s *);
extern i32 (*BigJump_JumpActionFn)(GameObject_s *);
extern i32 (*BigJump_LandActionFn)(GameObject_s *);
i32 StartBigJump(GameObject_s *object, nuvec_s *destination, i32 mode, f32 height_scale, f32 speed_scale, i32 animation,
                 i8 flags);
GameObject_s *AddDynamicCreature(i32 model, nuvec_s *position, i32 angle, char *script_name, AIPATHINFO_s *path_info,
                                 AIGROUP_s *group, i32 set_on_surface, nugspline_s *spline, nuvec_s *spline_offset,
                                 i32 spline_mode, i32 creature_set);

#endif
