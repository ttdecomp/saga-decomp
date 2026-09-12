#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/gizmo/base/GizTurretObjectInterface.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"

extern "C" i16 id_GRABCONTROL;
void CalculateInterceptVector(NUVEC *, NUVEC *, NUVEC *, f32, NUVEC *, NUVEC *);

i32 gizturret_test_ang = 3640;

void GizTurrets_Hit(void *, GIZTURRET_s *, nuvec_s *, i32, i32) {
}

GameObject_s *GizTurret_GetTgt(GIZTURRET_s *, numtx_s *matrix) {
    NUVEC forward = {0.0f, 0.0f, -1.0f};
    if (WORLD != NULL && player != NULL && WORLD->current_level == DEATHSTARRESCUEE_LDATA &&
        player->id == id_GRABCONTROL && TouchHacks::TouchControlsActive) {
        return NULL;
    }

    NuVecMtxRotate(&forward, &forward, matrix);
    GameObject_s *best = NULL;
    f32 best_distance = 1000000000.0f;
    GameObject_s *object = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
            (object->apiobj.field_0x1f4 & 1) == 0 || object->apiobj.model_draw_result == 0) {
            continue;
        }

        NUVEC direction;
        const f32 distance =
            NuVecDistSqr(&object->apiobj.collision_position, reinterpret_cast<NUVEC *>(&matrix->m30), &direction);
        if (best_distance > distance) {
            const f32 length = NuFsqrt(distance);
            const f32 scale = length > 0.0f ? 1.0f / length : 0.0f;
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
            if (NuVecDot(&direction, &forward) <= NU_COS_LUT(gizturret_test_ang)) {
                continue;
            }
            best = object;
            best_distance = distance;
        }
    }
    return best;
}

GIZTURRET_s *GizTurret_FindByName(GIZTURRETSYS_s *system, char *name) {
    if (name == NULL || system == NULL || system->count == 0) {
        return NULL;
    }

    GIZTURRET_s *turret = system->turrets;
    i32 i = 0;
    do {
        if (NuStrICmp(turret->name, name) == 0) {
            return turret;
        }
        ++i;
        ++turret;
    } while (system->count > i);
    return NULL;
}

GIZTURRET_s *GizTurret_FindNearest(GIZTURRETSYS_s *system, nuvec_s *position, GameObject_s *, f32 *distance, i32) {
    if (system == NULL) {
        return NULL;
    }

    GIZTURRET_s *nearest = NULL;
    f32 nearest_distance = 1000000000.0f;
    GIZTURRET_s *turret = system->turrets;
    for (i32 i = 0; i < system->count; ++i, ++turret) {
        if ((turret->flags & 4) != 0 && (turret->flags & 2) != 0) {
            const f32 current_distance = NuVecDistSqr(position, &turret->position, NULL);
            if (current_distance < nearest_distance) {
                nearest = turret;
                nearest_distance = current_distance;
            }
        }
    }
    if (distance != NULL) {
        *distance = nearest_distance;
    }
    return nearest;
}

i32 GizTurrets_UpdateHint(HINT_s *) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    GIZTURRETSYS_s *system = world->giz_turret_sys;
    if ((world->area != NULL && world->area == HUB_ADATA) || VehicleArea != 0 || system == NULL) {
        return 0;
    }

    if (system->count == 0) {
        return 0;
    }

    GIZTURRET_s *turret = system->turrets;
    i32 i = 0;
    i32 result = 0;
    do {
        if ((turret->flags & 6) == 6) {
            if ((turret->behavior_flags & 0x4010) == 0x4000) {
                if (turret->field_0xe4 != NULL) {
                    if (36.0f > NuVecDistSqr(&GameCam->pos, &turret->position, NULL)) {
                        result = 1;
                        break;
                    }
                }
            }
        }
        ++i;
        ++turret;
    } while (system->count > i);
    return result;
}

GIZTURRET_s *GizTurret_FindByController(GIZTURRETSYS_s *system, GameObject_s &controller) {
    if (system != NULL) {
        const i32 count = system->count;
        GIZTURRET_s *turret = system->turrets;
        if (count != 0) {
            for (i32 i = 0; i < count; ++i, ++turret) {
                if (turret->controller == &controller) {
                    return turret;
                }
            }
        }
    }
    return NULL;
}

void GizTurrets_OpponentSelection(GIZTURRETSYS_s *, i32, APIOBJECT_s **, i32, APIOBJECT_s **) {
}

void GizTurret_CalculateInterceptVector(nuvec_s *origin, numtx_s *matrix, nuvec_s *target, nuvec_s *velocity, f32 speed,
                                        nuvec_s *intercept_out, nuvec_s *velocity_out, u32 fallback) {
    NUVEC forward = {0.0f, 0.0f, -1.0f};
    NuVecMtxRotate(&forward, &forward, matrix);

    NUVEC intercept;
    NUVEC intercept_velocity;
    CalculateInterceptVector(origin, target, velocity, speed, &intercept, &intercept_velocity);

    NUVEC direction;
    const f32 distance = NuVecDistSqr(&intercept, origin, &direction);
    const f32 length = NuFsqrt(distance);
    const f32 scale = length > 0.0f ? 1.0f / length : 0.0f;
    direction.x *= scale;
    direction.y *= scale;
    direction.z *= scale;

    if (NuVecDot(&direction, &forward) > NU_COS_LUT(gizturret_test_ang) || fallback == 0) {
        if (intercept_out != NULL) {
            *intercept_out = intercept;
        }
        if (velocity_out != NULL) {
            *velocity_out = intercept_velocity;
        }
    } else {
        if (intercept_out != NULL) {
            *intercept_out = forward;
        }
        if (velocity_out != NULL) {
            *velocity_out = *origin;
        }
    }
}

void GIZTURRET_s::ClearMechObjectInterface() {
    if (mech_object_interface != NULL) {
        delete mech_object_interface;
    }
}

MechObjectInterface *GIZTURRET_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL) {
        new GizTurretObjectInterface(*this);
    }
    return mech_object_interface;
}

// Static turret anim-set reader. Moved from gizmisc_stubs.cpp.

static __used__ void GizTurret_ReadAnimSetData(GAMEANIMOBJ_s *, unsigned char) {
}
