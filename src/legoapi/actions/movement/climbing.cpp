#include "legoapi/legoapi_types.h"
#include "decomp.h"
#include "nu2api/numath/nutrig.h"
#include "legoapi/core/input/gamepads.h"
#include <math.h>
#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/world/world.h"
#include "gameapi/ai/aisys/aisys.h"
#include <string.h>
struct CLIMBOBJECT_s;

void StartJump(GameObject_s *, i32);
void FindAnglesXY(NUVEC *, u16 *, u16 *);
void Climb_SetMagnetAction(GameObject_s *);
void Climb_SetMagnetDrawOffsetTarget(GameObject_s *, NUVEC *);

f32 MAGNETOFFSET;

void Climb_MoveCode(GameObject_s *object) {
    if (LEGOCONTEXT_CLIMB == -1)
        return;
    if (object->character_context != LEGOCONTEXT_CLIMB) {
        bool can_enter = object->character_context == -1 ||
                         (LEGOCONTEXT_WALLSHUFFLE != -1 && object->character_context == LEGOCONTEXT_WALLSHUFFLE);
        if (!can_enter && LEGOCONTEXT_JUMP != -1 && object->character_context == LEGOCONTEXT_JUMP) {
            can_enter = object->context_animation_timer >= 1.0f ||
                        (object->context_animation_timer >= 0.1f && object->apiobj.velocity.y <= 0.0f);
        }
        if (!can_enter && LEGOCONTEXT_GLIDE != -1 && object->character_context == LEGOCONTEXT_GLIDE)
            can_enter = object->field_0x788 == NULL;
        if (!can_enter || !(object->pad_gamepad->input_magnitude > 0.0f) || object->field_0x1084 == 0 ||
            !(fabsf(object->contact_normal.y) < NuTrigTable[0x3000]) ||
            CanClimbSurface(object, static_cast<i8>(object->field_0x6b0)) == 0)
            return;
        const NUVEC normal = object->contact_normal;
        const u8 surface = object->field_0x6b0;
        NUVEC forward;
        NuVecRotateY(&forward, &v001, object->apiobj.movement_facing_angle);
        const f32 x = object->contact_position.x - object->apiobj.position.x;
        const f32 z = object->contact_position.z - object->apiobj.position.z;
        if (!(forward.x * x + forward.z * z > 0.0f))
            return;
        object->apiobj.movement_facing_angle = NuAtan2D(x, z);
        object->apiobj.velocity.y = 0.0f;
        object->character_context = LEGOCONTEXT_CLIMB;
        object->field_0x7a6 = surface;
        object->airborne_action_duration = 0.25f;
        if (surface == 5) {
            Climb_SetMagnetAction(object);
            Climb_SetMagnetDrawOffsetTarget(object, &object->zipup_swing_position);
        } else {
            object->context_animation = LEGOACT_CLIMB_IDLE;
        }
        object->external_force = normal;
        object->launch_origin = object->contact_position;
        FindAnglesXY(&object->external_force, &object->magnet_surface_angle, NULL);
        object->apiobj.velocity.z = 0.0f;
        object->apiobj.velocity.x = 0.0f;
        return;
    }

    if ((object->pad_gamepad->buttons_pressed & GAMEPAD_JUMP) != 0 && object->field_0x7a6 != 5) {
        StartJump(object, 0);
        object->movement_runtime_flags |= 0x10;
        const u16 angle = object->apiobj.movement_facing_angle;
        const f32 speed = object->apiobj.character_data->game_character->run_speed;
        object->apiobj.velocity.x = -NU_SIN_LUT(angle) * speed;
        object->apiobj.velocity.z = -NU_COS_LUT(angle) * speed;
        return;
    }
    const u16 input_angle = GamePad_InputAngle(object, object->pad_gamepad);
    if ((object->apiobj.field_0x27d & 2) != 0 &&
        (object->apiobj.field_0x281 == object->field_0x7a6 ||
         (object->pad_gamepad->input_magnitude > 0.0f &&
          PushingTowardsAngle(input_angle, object->apiobj.movement_facing_angle) < 0.0f))) {
        object->character_context = -1;
    } else if (object->field_0x1084 != 0 && object->field_0x6b0 == object->field_0x7a6) {
        object->airborne_action_duration = 0.25f;
        object->apiobj.movement_facing_angle = NuAtan2D(object->contact_position.x - object->apiobj.position.x,
                                                        object->contact_position.z - object->apiobj.position.z);
        object->external_force = object->contact_normal;
        object->launch_origin = object->contact_position;
        FindAnglesXY(&object->external_force, &object->magnet_surface_angle, NULL);
    } else {
        object->airborne_action_duration -= FRAMETIME;
        if (object->airborne_action_duration <= 0.0f)
            object->character_context = -1;
    }
    if (object->character_context != LEGOCONTEXT_CLIMB)
        return;
    if (object->field_0x7a6 == 5) {
        Climb_SetMagnetAction(object);
        Climb_SetMagnetDrawOffsetTarget(object, &object->zipup_swing_position);
        object->movement_runtime_flags |= 0x20;
        if (object->pad_gamepad->input_magnitude > 0.0f)
            object->tertiary_lean_angle =
                SeekRot(object->tertiary_lean_angle,
                        static_cast<u16>(RotDiff(object->apiobj.movement_facing_angle, input_angle)), 9.0f);
    } else {
        if (object->pad_gamepad->input_magnitude > 0.0f) {
            i32 difference =
                RotDiff(object->apiobj.movement_facing_angle, GamePad_InputAngle(object, object->pad_gamepad));
            if (difference < 0)
                difference = -difference;
            object->context_animation = difference > 0x4000 ? LEGOACT_CLIMB_DOWN : LEGOACT_CLIMB_UP;
            if (object->apiobj.character_model->model_data_b[object->context_animation] != NULL)
                return;
        }
        object->context_animation = LEGOACT_CLIMB_IDLE;
    }
}

#include "legoapi/characters/motion/gameanim.h"

// Original 0x4f26a0, 498 bytes.
i32 Climb_SetTargetMom(GameObject_s *object, u16 input_angle) {
    f32 speed = 0.0f;
    i16 animation = object->context_animation;
    if (animation != -1 &&
        (animation == LEGOACT_CLIMB_UP || animation == LEGOACT_CLIMB_DOWN || animation == LEGOACT_CLIMB_LEFT ||
         animation == LEGOACT_CLIMB_RIGHT || animation == LEGOACT_MAGNET_WALK_METAL)) {
        if (object->apiobj.character_model->model_data_b[animation] != NULL)
            speed = fabsf(AnimSpeed(object->apiobj.character_model, animation));
    }
    if (speed == 0.0f)
        speed = 0.5f;
    u16 angle = object->apiobj.movement_facing_angle;
    object->target_velocity.x = NuTrigTable[angle >> 1] * speed;
    object->target_velocity.y = 0.0f;
    object->target_velocity.z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
    if (object->pad_gamepad->input_magnitude > 0.0f) {
        angle += 0x4000;
        f32 lateral = PushingTowardsAngle(input_angle, angle);
        object->target_velocity.x += NuTrigTable[angle >> 1] * lateral * speed;
        object->target_velocity.z += lateral * NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff] * speed;
        f32 vertical = PushingTowardsAngle(input_angle, object->apiobj.movement_facing_angle);
        object->target_velocity.y += vertical * speed;
    }
    return 1;
}

void Climb_SetMagnetAction(GameObject_s *object) {
    object->context_animation = LEGOACT_IDLE;
    if (object->pad_gamepad->input_magnitude > 0.0f) {
        if (LEGOACT_MAGNET_WALK_METAL != -1)
            object->context_animation = LEGOACT_MAGNET_WALK_METAL;
        else if (LEGOACT_WALK != -1)
            object->context_animation = LEGOACT_WALK;
    }
}

// Original 0x4f1f90, 134 bytes.
void Climb_UpdateMagnetRotation(GameObject_s *object) {
    object->field_0x1086 = 0;
    object->apiobj.pitch_angle =
        SeekRot(object->apiobj.pitch_angle, static_cast<u16>(-0x4000 - object->magnet_surface_angle), 6.0f);
    object->apiobj.roll_angle = SeekRot(object->apiobj.roll_angle, 0, 10.0f);
}

void Climb_SetMagnetDrawOffsetTarget(GameObject_s *object, nuvec_s *offset) {
    NuVecScale(offset, &object->external_force, MAGNETOFFSET);
    NuVecAdd(offset, offset, &object->launch_origin);
    NuVecSub(offset, offset, &object->apiobj.position);
}

extern "C" TERRAIN_SURFACE_s TerSurface[32];
extern "C" i8 NewRayCastGetImpactTerrainType();
extern i32 TERRAINMASK_NONWEAPON, TERRAINMASK_NONDROID;
u32 LEGO_AIPATHCNX_MAGNETCLIMB, LEGO_AIPATHCNX_CLIMB;
i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);

static __used__ void ClimbObject_FindNormal(CLIMBOBJECT_s *object) {
    AIPATHNODE *first = &object->path->nodes[object->connection->node_indices[0]];
    AIPATHNODE *second = &object->path->nodes[object->connection->node_indices[1]];
    AIPATHNODE *node = second->position.y > first->position.y ? first : second;
    for (i32 angle = 0; angle < 0x20000; angle += 0x4000) {
        NUVEC displacement = {node->radius + 0.5f, 0.0f, 0.0f};
        NuVecRotateY(&displacement, &displacement, angle);
        NUVEC origin = {node->position.x, (node->max_height + node->min_height) * 0.5f, node->position.z};
        if (GameRayCast(&origin, &displacement, 0.0f, TERRAINMASK_NONWEAPON | TERRAINMASK_NONDROID | 0x5f) != 0 &&
            (TerSurface[NewRayCastGetImpactTerrainType()].flags & 0x10000) != 0) {
            object->flags |= 1;
            object->normal = ShadNorm;
            object->impact_x = origin.x + displacement.x;
            object->impact_z = origin.z + displacement.z;
            return;
        }
    }
}

i32 CanClimbSurface(GameObject_s *object, i32 surface) {
    i32 result = 0;
    if (static_cast<u32>(surface) < 32 && (TerSurface[surface].flags & 0x10000) != 0) {
        result = 1;
        if (surface == 5 && (CanMagnetClimbFn == NULL || CanMagnetClimbFn(object) == 0))
            result = 0;
    }
    return result;
}

CLIMBOBJECT_s *FindClimbObject(CLIMBOBJECTSYS_s *system, AIPATHCNX_s *connection) {
    if (system == NULL)
        return NULL;
    CLIMBOBJECT_s *object = system->objects;
    for (i32 index = 0; index < system->count; ++index, ++object) {
        if (object->connection != connection)
            continue;
        if ((object->flags & 2) != 0)
            return NULL;
        if ((object->flags & 1) != 0)
            return object;
        ClimbObject_FindNormal(object);
        if ((object->flags & 1) != 0)
            return object;
        object->flags |= 2;
    }
    return NULL;
}

void InitClimbObjectSys(WORLDINFO_s *world) {
    if (world == NULL || world->ai_sys == NULL || world->ai_sys->path_sys == NULL || world->climb_object_sys == NULL)
        return;
    world->climb_object_sys->count = 0;
    memset(world->climb_object_sys->objects, 0, world->climb_object_sys->capacity * sizeof(CLIMBOBJECT_s));
    for (i32 index = 0; index < world->ai_sys->path_sys->path_count; ++index) {
        AIPATH *path = world->ai_sys->path_sys->paths[index];
        if (path == NULL)
            continue;
        AIPATHCNX *connection = path->connections;
        for (i32 connection_index = 0; connection_index < path->connection_count; ++connection_index, ++connection) {
            CLIMBOBJECTSYS_s *system = world->climb_object_sys;
            if (system->count < system->capacity &&
                (connection->traversal_flags[0] & (LEGO_AIPATHCNX_MAGNETCLIMB | LEGO_AIPATHCNX_CLIMB)) != 0) {
                CLIMBOBJECT_s *object = &system->objects[system->count++];
                object->connection = connection;
                object->path = path;
                ClimbObject_FindNormal(object);
            }
        }
    }
}
