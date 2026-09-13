#pragma once

#include "nu2api/nucore/fixed_width.h"
u32 GizTurrets_TotalScore(void *world);

#include "legoapi/gizmo/base/gizmo.h"

extern i32 turret_gizmotype_id;

#ifdef __cplusplus

typedef struct GIZTURRET_s GIZTURRET;
struct GIZTURRETSYS_s;
struct GameObject_s;
struct HINT_s;
struct numtx_s;

enum GIZTURRET_FLAGS : u8 {
    GIZTURRET_FLAG_ACTIVE = 1 << 1,
    GIZTURRET_FLAG_VISIBLE = 1 << 2,
    GIZTURRET_FLAG_UPDATE_DISABLED = 1 << 5,
    GIZTURRET_FLAG_FIRED_THIS_FRAME = 1 << 7,
};

enum GIZTURRET_ANIMATION_FLAGS : u8 {
    GIZTURRET_ANIMATION_FLAG_DRAW_REFLECTION = 1 << 4,
};

enum GIZTURRET_RUNTIME_FLAGS : u8 {
    GIZTURRET_RUNTIME_FLAG_BLOWUP_NAME_ID = 1 << 0,
    GIZTURRET_RUNTIME_FLAG_ROTATION_SFX_PLAYING = 1 << 3,
};

ADDGIZMOTYPE *GizTurrets_RegisterGizmo(i32 type_id);
GameObject_s *GizTurret_GetTgt(GIZTURRET_s *turret, numtx_s *matrix);
void GizTurret_CalculateInterceptVector(NUVEC *origin, numtx_s *matrix, NUVEC *target, NUVEC *velocity,
                                        f32 speed, NUVEC *intercept, NUVEC *intercept_velocity, u32 fallback);
void GizTurrets_Hit(void *world, GIZTURRET_s *turret, NUVEC *position, i32 player, i32 flags);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
