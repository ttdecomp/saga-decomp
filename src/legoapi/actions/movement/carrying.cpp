#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/gamepads.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/nutrig.h"


// Original 0x4fd340, 390 bytes.
i32 SuperCarry_YRotation(GameObject_s *object, u16 input_angle) {
    if (object->field_0x7a3 == 0) {
        object->apiobj.facing_angle = SeekRot(object->apiobj.facing_angle, object->apiobj.movement_facing_angle, 12.0f);
        object->apiobj.field_0x276 = object->apiobj.facing_angle;
    } else if (object->field_0x7a3 == 5) {
        if (object->force_target != NULL) {
            object->apiobj.movement_facing_angle =
                NuAtan2D(object->force_target->apiobj.collision_position.x - object->apiobj.collision_position.x,
                         object->force_target->apiobj.collision_position.z - object->apiobj.collision_position.z);
        }
        object->apiobj.facing_angle = SeekRot(object->apiobj.facing_angle, object->apiobj.movement_facing_angle, 8.0f);
        object->apiobj.field_0x276 = object->apiobj.facing_angle;
    } else if (object->field_0x7a3 == 2 || object->field_0x7a3 == 3 || object->field_0x7a3 == 6) {
        if (object->pad_gamepad->input_magnitude > 0.0f) {
            object->apiobj.facing_angle = TurnRot(object->apiobj.facing_angle, input_angle,
                                                  static_cast<i32>(16384.0f * object->field_0x768 * 8.0f), NULL);
        }
        object->apiobj.field_0x276 = SeekRot(object->apiobj.field_0x276, object->apiobj.facing_angle, 10.0f);
        object->apiobj.movement_facing_angle = object->apiobj.facing_angle;
    }
    return 1;
}

// Original 0x4fd200, 312 bytes.
i32 SuperCarry_SetTargetMom(GameObject_s *object, float input_speed) {
    object->target_velocity.x = 0.0f;
    object->target_velocity.z = 0.0f;
    if (input_speed > 0.0f) {
        f32 speed;
        if ((object->field_0x7a3 == 1 || object->field_0x7a3 == 3) && object->context_animation != -1 &&
            object->apiobj.character_model->model_data_b[object->context_animation] != NULL) {
            speed = AnimSpeed(object->apiobj.character_model, object->context_animation);
        } else if (object->field_0x7a3 == 3 || object->field_0x7a3 == 6) {
            if (object->field_0x7a3 == 6 && LEGOACT_SUPERCARRY_WALK != 0 &&
                object->apiobj.character_model->model_data_b[LEGOACT_SUPERCARRY_WALK] != NULL)
                speed = AnimSpeed(object->apiobj.character_model, LEGOACT_SUPERCARRY_WALK);
            else
                speed = 0.5f * object->apiobj.character_data->game_character->run_speed;
        } else {
            return 0;
        }
        if (speed > 0.0f) {
            object->target_velocity.x = NU_SIN_LUT(object->apiobj.facing_angle) * speed;
            object->target_velocity.z = NU_COS_LUT(object->apiobj.facing_angle) * speed;
        }
    }
    return 0;
}

i32 ObjLandReady(GameObject_s *object) {
    if (object == NULL) {
        return 0;
    }
    const i32 context = object->character_context;
    if ((CInfo[context].flags & 0x1004) != 0 &&
        (LEGOCONTEXT_LAND_COMBATROLL == -1 || LEGOCONTEXT_LAND_COMBATROLL != context || object->field_0x7a3 != 0)) {
        if (LEGOCONTEXT_WALLSHUFFLE != -1 && LEGOCONTEXT_WALLSHUFFLE == context) {
            return 0;
        }
        return 1;
    }
    return 0;
}

void LetGoOfBalloon(GameObject_s *) {
}

i32 MovingBackwards(GameObject_s *object) {
    GAMEPAD_s *pad = object->pad_gamepad;
    if (pad->operator_data == NULL || pad->input_magnitude == 0.0f) {
        return 0;
    }

    const u16 camera_yaw = GameCam->input_yaw;
    const i32 difference = RotDiff(static_cast<u16>(pad->input_angle + camera_yaw), object->apiobj.field_0x276);
    return static_cast<u32>(difference + 0x4000) > 0x8000;
}
