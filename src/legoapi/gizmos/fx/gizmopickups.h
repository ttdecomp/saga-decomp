#pragma once

#include "legoapi/gizmo/base/gizmo.h"

extern i32 gizmopickup_typeid;
extern f32 AreaPickupScale;
extern i32 PickUpFlickerTest;
extern i32 PickUpFlickerFrames;
extern i32 PickupFlickerFrame;

#ifdef __cplusplus

typedef struct GIZMOPICKUP_s GIZMOPICKUP;

ADDGIZMOTYPE *GizmoPickups_RegisterGizmo(i32 type_id);
GIZMOPICKUP_s *GizmoPickup_FindByName(WORLDINFO_s *world, char *name);
i32 GizmoPickup_BeenTurnedOn(GIZMOPICKUP_s *pickup);
u32 GizmoPickups_TotalScore(void *world);
void AddPickups(i32 coin_count, i32 heart_count, i32 pickup_count, i32 unknown, NUVEC *position, NUVEC *direction,
                f32 speed, i32 model, f32 radius, f32 duration, GameObject_s *owner, i32 flags, i32 extra,
                bool visible);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
