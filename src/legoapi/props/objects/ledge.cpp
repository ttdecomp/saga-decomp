#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/gizmos/transport/ledges.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/gamepads.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

f32 LEDGETERRAINLOOKAHEAD = 0.02f;
i32 LedgeTerrain_CheckAnims = 1;
extern "C" TERRAIN_SURFACE_s TerSurface[32];
void StartJump(GameObject_s *, i32);
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);

static i32 LedgeTerrain_Attach(GameObject_s *object, u16 facing, NUVEC *position, u16 *wall_angle) {
    *wall_angle = NuAtan2D(object->contact_normal.x, object->contact_normal.z);
    i32 difference = RotDiff(facing, *wall_angle);
    if (difference < 0)
        difference = -difference;
    if (difference < 0x4000)
        return 0;

    u16 inward_angle = *wall_angle + 0x8000;
    f32 distance = object->apiobj.field_0x1dc + LEDGETERRAINLOOKAHEAD;
    position->x = NU_SIN_LUT(inward_angle) * distance + object->apiobj.position.x;
    position->y = object->apiobj.upper_position.y;
    position->z = NU_COS_LUT(inward_angle) * distance + object->apiobj.position.z;
    f32 height = GameShadow(NULL, position, 5.0f, -1);
    if (height == 2000000.0f)
        return 0;
    if (NuTrigTable[0x2555] > ShadNorm.y)
        return 0;
    f32 height_difference = position->y - height;
    if (height_difference > 0.025f || height_difference < -0.1f)
        return 0;
    if (object->apiobj.field_0x220 != 2000000.0f && height > object->apiobj.field_0x220)
        return 0;
    position->y = height;
    return 1;
}

LEDGE *Ledge_FindNearest(WORLDINFO_s *world, NUVEC *position, GameObject_s *object, f32 *distance_squared) {
    LEDGE *nearest = NULL;
    f32 best_distance = 1000000000.0f;
    LEDGE *ledge = static_cast<LEDGE *>(world->ledges);
    for (i32 i = 0; i < world->ledge_count; ++i, ++ledge) {
        if (object != NULL) {
            if ((ledge->state_flags & 3) != 3)
                continue;
            if ((ledge->flags & 1) && ShadowMode == 0)
                continue;
        }
        f32 distance = NuVecDistSqr(position, &ledge->position, NULL);
        if (distance < best_distance) {
            nearest = ledge;
            best_distance = distance;
        }
    }
    if (distance_squared != NULL)
        *distance_squared = best_distance;
    return nearest;
}

void LedgeTerrain_MoveCode(GameObject_s *object) {
    if (LEGOCONTEXT_LEDGETERRAIN == -1)
        return;
    NUVEC position;
    u16 wall_angle;
    if (object->character_context != LEGOCONTEXT_LEDGETERRAIN) {
        if (object->apiobj.field_0x27d != 0 || !(object->apiobj.velocity.y <= 0.0f))
            return;
        if (LedgeTerrain_CheckAnims &&
            (LEGOACT_LEDGE_IDLE == -1 || object->apiobj.character_model->model_data_b[LEGOACT_LEDGE_IDLE] == NULL))
            return;
        if (object->character_context != -1 &&
            !(LEGOCONTEXT_CLIMB != -1 && object->character_context == LEGOCONTEXT_CLIMB)) {
            if (LEGOCONTEXT_JUMP == -1 || object->character_context != LEGOCONTEXT_JUMP ||
                !(object->context_animation_timer >= 0.1f))
                return;
        }
        if (!(object->apiobj.field_0x1f8 & 0x80) && !(object->field_0xf01 & 0x80))
            return;
        if (!(object->pad_gamepad->input_magnitude > 0.0f))
            return;
        u16 facing = GamePad_InputAngle(object, object->pad_gamepad);
        if (!object->field_0x1084)
            return;
        u8 surface = object->field_0x6b0;
        if (surface < 32 && (TerSurface[surface].flags & 0x581))
            return;
        if (object->apiobj.collision_position.y - 0.05f >= object->contact_position.y ||
            fabsf(object->contact_normal.y) > NuTrigTable[0x3000])
            return;
        if (!LedgeTerrain_Attach(object, facing, &position, &wall_angle))
            return;
        object->field_0x7a3 = 0;
        object->context_animation_timer = 0.0f;
        object->character_context = LEGOCONTEXT_LEDGETERRAIN;
        object->apiobj.velocity = v000;
        if (LEGOACT_LEDGE_GRAB != -1 && object->apiobj.character_model->model_data_b[LEGOACT_LEDGE_GRAB] != NULL) {
            object->context_animation = LEGOACT_LEDGE_GRAB;
            object->field_0x768 = AnimDuration(object->id, LEGOACT_LEDGE_GRAB, 0.0f, 0.0f, 1);
        } else {
            object->field_0x768 = 0.1f;
            object->context_animation = LEGOACT_LEDGE_IDLE;
            ResetAnimPacket(&object->apiobj.anim_packet, LEGOACT_LEDGE_IDLE);
        }
        object->external_force.x = object->apiobj.position.x;
        object->external_force.y = position.y;
        object->external_force.z = object->apiobj.position.z;
        object->apiobj.movement_facing_angle = wall_angle + 0x8000;
        object->launch_origin = position;
        object->airborne_action_duration = 0.25f;
        return;
    }
    if (object->pad_gamepad->buttons_pressed & GAMEPAD_JUMP) {
        StartJump(object, 0);
        object->movement_runtime_flags |= 0x10;
        f32 height = 0.1f + object->external_force.y - object->jump_start_height;
        if (height > 0.0f)
            object->apiobj.velocity.y =
                NuFsqrt(-2.0f * object->apiobj.character_data->game_character->gravity * height);
        return;
    }
    if (object->apiobj.field_0x27d & 2) {
        object->character_context = -1;
        return;
    }
    if (object->field_0x7a3 || object->context_animation != LEGOACT_LEDGE_GRAB ||
        AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0)) {
        object->context_animation_timer += FRAMETIME;
    }
    if (!object->field_0x7a3 && object->context_animation_timer >= object->field_0x768)
        object->field_0x7a3 = 1;
    f32 travelled = 0.0f;
    u8 surface = object->field_0x6b0;
    bool attached = object->field_0x1084 && !(surface < 32 && (TerSurface[surface].flags & 0x581)) &&
                    !(object->apiobj.collision_position.y - 0.05f >= object->contact_position.y) &&
                    !(fabsf(object->contact_normal.y) > NuTrigTable[0x3000]) &&
                    LedgeTerrain_Attach(object, object->apiobj.movement_facing_angle, &position, &wall_angle);
    if (attached) {
        f32 x = object->external_force.x;
        f32 z = object->external_force.z;
        object->apiobj.movement_facing_angle = wall_angle + 0x8000;
        object->external_force.y = position.y;
        object->launch_origin = position;
        object->external_force.x = object->apiobj.position.x;
        object->external_force.z = object->apiobj.position.z;
        object->airborne_action_duration = 0.25f;
        x = object->apiobj.position.x - x;
        z = object->apiobj.position.z - z;
        travelled = NuFsqrt(x * x + z * z);
    } else {
        object->airborne_action_duration -= FRAMETIME;
        if (object->airborne_action_duration <= 0.0f) {
            object->character_context = -1;
            return;
        }
    }
    if (!object->field_0x7a3)
        return;
    if (!(object->pad_gamepad->input_magnitude > 0.0f)) {
        object->context_animation = LEGOACT_LEDGE_IDLE;
        return;
    }
    u16 input = GamePad_InputAngle(object, object->pad_gamepad);
    u16 sideways = object->apiobj.movement_facing_angle + 0x4000;
    f32 push = PushingTowardsAngle(input, sideways);
    i16 action;
    f32 speed;
    if (push > NuTrigTable[0x3555]) {
        action = LEGOACT_LEDGE_RIGHT;
        speed = 0.5f;
    } else if (push < -NuTrigTable[0x3555]) {
        action = LEGOACT_LEDGE_LEFT;
        speed = -0.5f;
    } else {
        object->context_animation = LEGOACT_LEDGE_IDLE;
        return;
    }
    if (action != -1 && object->apiobj.character_model->model_data_b[action] != NULL) {
        object->context_animation = action;
        f32 animation_speed = fabsf(AnimSpeed(object->apiobj.character_model, action));
        speed = speed < 0.0f ? -animation_speed : animation_speed;
        if (speed == 0.0f)
            return;
        if (LedgeTerrain_CheckAnims && !AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0))
            return;
    } else if (LedgeTerrain_CheckAnims) {
        return;
    }
    if (speed < 0.0f)
        travelled = -travelled;
    f32 distance = speed * FRAMETIME + travelled;
    NUVEC offset = {NU_SIN_LUT(sideways) * distance, 0.0f, NU_COS_LUT(sideways) * distance};
    NuVecAdd(&object->external_force, &object->external_force, &offset);
    NuVecAdd(&object->launch_origin, &object->launch_origin, &offset);
}

i32 LedgeTerrain_SetTargetMom(GameObject_s *object) {
    object->target_velocity.x = (NU_SIN_LUT(object->apiobj.movement_facing_angle) * LEDGETERRAINLOOKAHEAD +
                                 object->external_force.x - object->apiobj.upper_position.x) *
                                10.0f;
    object->target_velocity.z = (NU_COS_LUT(object->apiobj.movement_facing_angle) * LEDGETERRAINLOOKAHEAD +
                                 object->external_force.z - object->apiobj.upper_position.z) *
                                10.0f;
    object->target_velocity.y = (object->external_force.y - object->apiobj.upper_position.y) * 10.0f;
    return 1;
}
