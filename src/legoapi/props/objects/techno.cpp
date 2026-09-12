#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/base/animpacket.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/audio/audio.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/nustring.h"
#include "globals.h"
#include "legoapi/world/world.h"
#include "legoapi/gizmos/object/technos.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;
void *Technos_FindTgt(TECHNO_s *techno);
TECHNO *Techno_FindNearest(WORLDINFO_s *world, nuvec_s *position, GameObject_s *object, f32 *distance);
void Technos_MoveTarget(TECHNO_s *techno, GameObject_s *object);
void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode);
void GizObstacle_SetTechnoControlled(GIZOBSTACLE_s *obstacle, f32 speed);

static f32 TechnoMoveSpeed[2];

i32 Techno_isReady(TECHNO_s *techno) {
    if (techno == NULL) {
        return 0;
    }

    if (techno->controlled_object == NULL) {
        Technos_FindTgt(techno);
    }

    switch (techno->target_mode) {
        case 1: {
            GameObject_s *object = static_cast<GameObject_s *>(techno->controlled_object);
            return object != NULL && (object->apiobj.field_0x1f8 & 0x1001) == 0x1001;
        }
        case 2:
            return NuSpecialGetVisibilityFn(techno->controlled_object) != 0;
        case 3:
            return techno->controlled_object != NULL;
        default:
            return 0;
    }
}

NUVEC *Technos_TgtPos(TECHNO_s *techno) {
    if (techno == NULL) {
        return NULL;
    }

    if (techno->controlled_object == NULL) {
        Technos_FindTgt(techno);
    }

    switch (techno->target_mode) {
        case 1:
            return &static_cast<GameObject_s *>(techno->controlled_object)->apiobj.collision_position;
        case 2:
            return NuSpecialGetPos(techno->controlled_object);
        case 3:
            return GizmoGetPos(WORLD->gizmo_sys, static_cast<GIZMO *>(techno->controlled_object));
        default:
            return NULL;
    }
}

void Techno_MoveCode(WORLDINFO_s *world, GameObject_s *object) {
    if (object->character_context != 0x51) {
        if ((static_cast<i8>(object->apiobj.flags_low) < 0 || object->use_action == 2) && object->suit != NULL &&
            (static_cast<SUIT_s *>(object->suit)->flags & 0x20) != 0) {
            f32 distance;
            TECHNO *techno = Techno_FindNearest(world, &object->apiobj.lower_position, object, &distance);
            if (techno == NULL) {
                return;
            }

            f32 range = object->apiobj.field_0x1dc + 2000000.0f;
            f32 hint_range = range * 2.5f;
            if (hint_range * hint_range > distance) {
                techno->flags |= TECHNO_FLAG_USED_THIS_FRAME;
            }

            if (object->apiobj.field_0x27d == 0 || ObjLandReady(object) == 0 || !(range * range > distance) ||
                ((object->pad_gamepad->buttons_pressed & GAMEPAD_SPECIAL) == 0 && object->use_action != 2)) {
                return;
            }

            if (Techno_isReady(techno) != 0) {
                object->character_context = 0x51;
                object->field_0x788 = techno;
                object->apiobj.movement_facing_angle = techno->y_rotation + 0x8000;
                object->context_animation = 0x99;
                object->context_animation_timer = 0.0f;
                GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
                if (static_cast<u8>(object->apiobj.field_0x27c) < 2) {
                    TechnoMoveSpeed[static_cast<u8>(object->apiobj.field_0x27c)] = 0.0f;
                }
            } else {
                GameAudio_PlaySfx(0x32, &techno->position, 0, 0);
            }
        }
    } else {
        TECHNO *techno = static_cast<TECHNO *>(object->field_0x788);
        techno->flags |= TECHNO_FLAG_USED_THIS_FRAME;

        if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL &&
            AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0) == NULL) {
            return;
        }

        object->context_animation_timer += FRAMETIME;
        if ((object->pad_gamepad->buttons_pressed & GAMEPAD_TAG) != 0 ||
            (static_cast<i8>(object->apiobj.flags_low) >= 0 && object->use_action != 2)) {
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
            object->character_context = -1;
            object->tag_flags |= 1;
            object->apiobj.movement_facing_angle += 0x8000;
            Technos_MoveTarget(techno, NULL);
        } else {
            Technos_MoveTarget(techno, object);
        }
    }
}

void *Technos_FindTgt(TECHNO_s *techno) {
    if (techno == NULL || techno->controlled_object != NULL) {
        return techno != NULL ? techno->controlled_object : NULL;
    }

    switch (techno->target_mode) {
        case 0: {
            void *target = GetNamedGameObject(WORLD->ai_sys, techno->target_name);
            if (target != NULL) {
                techno->controlled_object = target;
                techno->target_mode = 1;
                break;
            }
            target = GizmoFindByName(WORLD->gizmo_sys, -1, techno->target_name);
            if (target != NULL) {
                techno->controlled_object = target;
                techno->target_mode = 3;
                break;
            }
            if (NuSpecialFind(WORLD->current_gscn, reinterpret_cast<nuhspecial_s *>(techno->target_special_storage),
                              techno->target_name, 0) != 0) {
                techno->target_mode = 2;
                techno->controlled_object = techno->target_special_storage;
                break;
            }
            techno->target_mode = 0;
            techno->controlled_object = NULL;
            break;
        }
        case 1:
            techno->controlled_object = GetNamedGameObject(WORLD->ai_sys, techno->target_name);
            if (techno->controlled_object == NULL) {
                techno->target_mode = 0;
            }
            break;
        case 2:
            techno->controlled_object = techno->target_special_storage;
            if (NuSpecialFind(WORLD->current_gscn, reinterpret_cast<nuhspecial_s *>(techno->target_special_storage),
                              techno->target_name, 0) == 0) {
                techno->target_mode = 0;
                techno->controlled_object = NULL;
            }
            break;
        case 3:
            techno->controlled_object = GizmoFindByName(WORLD->gizmo_sys, -1, techno->target_name);
            if (techno->controlled_object == NULL) {
                techno->target_mode = 0;
            }
            break;
        default:
            techno->target_mode = 0;
            break;
    }
    return techno->controlled_object;
}

TECHNO *Techno_FindNearest(WORLDINFO_s *world, nuvec_s *position, GameObject_s *object, float *distance) {
    TECHNO *nearest = NULL;
    f32 nearest_distance = 1000000000.0f;

    TECHNO *techno = world->technos;
    for (i32 i = 0; i < world->ntechnos; ++i, ++techno) {
        f32 candidate_distance;
        if (object != NULL) {
            if ((techno->flags & (TECHNO_FLAG_ACTIVE | TECHNO_FLAG_VISIBLE)) !=
                    (TECHNO_FLAG_ACTIVE | TECHNO_FLAG_VISIBLE) ||
                techno->ground_position.y == 2000000.0f) {
                continue;
            }
            candidate_distance = NuVecDistSqr(position, &techno->ground_position, NULL);
        } else {
            candidate_distance = NuVecDistSqr(position, &techno->position, NULL);
        }
        if (candidate_distance < nearest_distance) {
            nearest_distance = candidate_distance;
            nearest = techno;
        }
    }

    if (distance != NULL) {
        *distance = nearest_distance;
    }
    return nearest;
}

void Technos_MoveTarget(TECHNO_s *techno, GameObject_s *object) {
    f32 speed = 0.0f;
    if (object != NULL) {
        if (static_cast<i8>(object->apiobj.flags_low) >= 0 && object->use_action == 2) {
            speed = 1.0f;
        } else if ((techno->enabled & 1) != 0) {
            if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_WAGGLED) != 0) {
                speed = 1.0f;
            }
        } else {
            if ((techno->enabled & 2) != 0) {
                speed = object->pad_gamepad->waggle_magnitude;
            } else if ((techno->enabled & 4) != 0) {
                speed = object->pad_gamepad->input_direction_x;
            } else if ((techno->enabled & 8) != 0) {
                speed = object->pad_gamepad->input_direction_z;
            }
            if ((techno->output & 1) != 0 && speed < 0.0f) {
                speed = 0.0f;
            }
        }
    }

    if (techno->controlled_object == NULL) {
        Technos_FindTgt(techno);
    }
    if (techno->target_mode != 3) {
        return;
    }

    GIZMO *gizmo = static_cast<GIZMO *>(techno->controlled_object);
    if (NuStrICmp(gizmotypes->types[gizmo->type_id].name, "GIZOBSTACLE") != 0) {
        return;
    }

    GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
    if ((techno->enabled & 2) != 0 && object != NULL && static_cast<u8>(object->apiobj.field_0x27c) < 2) {
        f32 rate = object->pad_gamepad->waggle_magnitude != 0.0f ? 3.0f * FRAMETIME : 2.0f * FRAMETIME;
        TechnoMoveSpeed[static_cast<i8>(object->apiobj.field_0x27c)] =
            SeekLinearF(TechnoMoveSpeed[static_cast<i8>(object->apiobj.field_0x27c)], speed, rate);
        GizObstacle_SetTechnoControlled(obstacle, TechnoMoveSpeed[static_cast<i8>(object->apiobj.field_0x27c)]);
    } else {
        GizObstacle_SetTechnoControlled(obstacle, speed);
    }
}

i32 GizTechno_CanUseTechno(GameObject_s *, TECHNO_s *) {
    return 1;
}
