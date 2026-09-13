#pragma once

#include "nu2api/nucore/fixed_width.h"
u32 GizObstacles_TotalScore(void *world);

#include "legoapi/gizmo/base/gizmo.h"

extern i32 obstacle_gizmotype_id;

enum GIZOBSTACLE_OUTPUT : i32 {
    GIZOBSTACLE_OUTPUT_AT_END = 0,
    GIZOBSTACLE_OUTPUT_NOT_AT_START = 1,
    GIZOBSTACLE_OUTPUT_PROXIMITY = 2,
    GIZOBSTACLE_OUTPUT_AT_START = 3,
    GIZOBSTACLE_OUTPUT_PLAYING_FORWARD = 4,
};

#ifdef __cplusplus

typedef struct GIZOBSTACLE_s GIZOBSTACLE;
typedef struct GIZOBSTACLESYS_s GIZOBSTACLESYS;

using GIZOBSTACLEUPDATEFN = void (*)(GIZOBSTACLE_s *);

extern GIZOBSTACLEUPDATEFN gizobstacleupdatefns[8];

void GizObstacle_Stop(GIZOBSTACLE_s *obstacle);
void GizObstacle_EvalAveragePosAndRadius(GIZOBSTACLE_s *obstacle, i32 state);
void GizObstacles_AddTrigger(NUVEC *position);
i32 GizObstacles_Hit(void *world, GIZOBSTACLE_s *obstacle, NUVEC *position, i32 player, i32 flags);
void GizObstacle_SetTechnoControlled(GIZOBSTACLE_s *obstacle, f32 speed);
void GizObstacle_SetDefaultSFXFn_LSW(void *world, GIZOBSTACLE_s *obstacle);
i32 GizObstacle_CheckExcludeFlagsFn_LSW(GIZOBSTACLE_s *obstacle, GameObject_s *object);
GIZOBSTACLE_s *GizObstacle_FindByName(GIZOBSTACLESYS_s *system, char *name);
void GizObstacle_JumpToStart(GIZOBSTACLE_s *obstacle);
void GizObstacle_JumpToEnd(GIZOBSTACLE_s *obstacle);
void GizObstacle_PlayForwards(GIZOBSTACLE_s *obstacle);
void GizObstacle_PlayBackwards(GIZOBSTACLE_s *obstacle);

ADDGIZMOTYPE *GizObstacles_RegisterGizmo(i32 type_id);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
