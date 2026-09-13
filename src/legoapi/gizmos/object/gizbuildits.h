#pragma once

#include "nu2api/nucore/fixed_width.h"
u32 GizBuildIts_TotalScore(void *world);

#include "legoapi/gizmo/base/gizmo.h"

extern i32 gizbuildit_gizmotype_id;

#ifdef __cplusplus

typedef struct GIZBUILDIT_s GIZBUILDIT;
struct VuVec;
struct GameObject_s;
struct WORLDINFO_s;
struct GAMEANIMOBJ_s;

extern i32 LEGOCONTEXT_BUILDIT;
extern i16 GizBuilditGDeb[6];
extern f32 GIZBUILDITWOBBLEJUMPHEIGHT;
extern void (*GizBuildIt_FinishFn)(GIZBUILDIT_s *);
extern i32 (*GizBuildIt_CanStartBuildingFn)(GIZBUILDIT_s *, GameObject_s *);
extern i32 (*GizBuildit_AutoBuildPosFn)(void *, nuvec_s *, nuvec_s *, u16 *);

ADDGIZMOTYPE *GizBuildIts_RegisterGizmo(i32 type_id);
void CalcAveragePosAndRad(GIZBUILDIT_s &buildit, VuVec &position, f32 &radius, bool include_built);
GIZBUILDIT_s *GizBuildIt_AnyReacting(WORLDINFO_s *world);
void GizBuildIt_KillParts(GIZBUILDIT_s *buildit);
void GizGetBuildItPlayerPos(GameObject_s *player, nuvec_s *position, nuvec_s *target);
void GizBuildItPushAwayFromStart(GameObject_s *player, GIZBUILDIT_s *buildit);
void GizBuildItPushAwayFromEnd(GameObject_s *player);
void GizBuildit_Reset(GIZBUILDIT_s *buildit, void *world);
void GizBuildit_SetVisibility(GIZBUILDIT_s *buildit, i32 visible);
void GizBuildIt_Finish(GIZBUILDIT_s *buildit);
void GizMoveAttractoBuildItPiece(GIZBUILDIT_s *buildit, GAMEANIMOBJ_s *piece);
void GizBuildIt_SetStepTime(GIZBUILDIT_s *buildit, GameObject_s *player);
void GizBuildIt_TurnOff(GIZBUILDIT_s *buildit);
void GizBuildIt_SetHeadTarget(GIZBUILDIT_s *buildit, GameObject_s *player);
void ReleaseBuildIt(GameObject_s *player, i32 completed);
void BuildIt_MoveCode(GameObject_s *player);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
