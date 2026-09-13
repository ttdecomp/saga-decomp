#pragma once

#include "nu2api/nucore/fixed_width.h"
u32 GizForce_TotalScore(void *world);

#include "legoapi/gizmo/base/gizmo.h"

extern i32 force_gizmotype_id;

#ifdef __cplusplus

typedef struct GIZFORCE_s GIZFORCE;
struct GameObject_s;
struct GIZFORCESYS_s;
struct WORLDINFO_s;
struct PART_s;
i32 GizForce_GameObjUsingForce(GameObject_s *object, GIZFORCE_s *force);
void ForceLightning_Origin(GameObject_s *object, NUVEC *primary, NUVEC *secondary);

void GizForce_ResetLOS(GameObject_s *object);
GIZFORCE_s *GizForce_FindByName(GIZFORCESYS_s *force_sys, char *name);
GIZFORCE_s *GizForces_FindForce(WORLDINFO_s *world, char *name);
i32 GizForce_StoodOnForce(GIZFORCE_s *force, GameObject_s *object);
u16 GizForces_AngleToForce(NUVEC *position, GIZFORCE_s *force);
i32 GizForce_FindBestForceTarget(GIZFORCESYS_s *force_sys, GameObject_s *object);
PART_s *GizForce_Throw(GameObject_s *object, GIZFORCE_s *force, f32 speed, f32 gravity, i32 flags);

void GizForce_PlayForwards(GIZFORCE_s *force);
void GizForce_PlayBackwards(GIZFORCE_s *force);
void GizForce_SetVisibility(GIZFORCE_s *force, i32 visibility);
i32 GizForce_AnimComplete(GIZFORCE_s *force);
i32 GizForce_Complete(GIZFORCE_s *force);
struct HINT_s;
i32 GizForce_UpdateHint(HINT_s *hint);

ADDGIZMOTYPE *GizForce_RegisterGizmo(i32 type_id);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
