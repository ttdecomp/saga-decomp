#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/menus/core/gamehint.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" i32 NuPortalWhichRoom(NUGSCN *scene, NUVEC *position);
extern i32 LEGOCONTEXT_BUILDIT;
void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode);

void ReleaseBuildIt(GameObject_s *player, i32 completed) {
    const i32 build_context = LEGOCONTEXT_BUILDIT;
    if (build_context == -1 || build_context != player->build_context) {
        return;
    }

    player->build_context = -1;
    const f32 completion_blend = completed != 0 ? 0.6f : 0.0f;
    GameCam_Blend(GameCam, 0.5f, completion_blend, 1);
}

extern NuMechPtr<MechObjectInterface, 4> nextBuildit;
i32 (*GizBuildIt_CanStartBuildingFn)(GIZBUILDIT_s *, GameObject_s *);
GIZBUILDIT_s *GizBuildIt_FindNearest(WORLDINFO_s *, GameObject_s *, BUILDIT_FIND_ENUM, i32);
void GizBuildIt_SetHeadTarget(GIZBUILDIT_s *, GameObject_s *);
void GizBuildIt_SetStepTime(GIZBUILDIT_s *, GameObject_s *);
void GizBuildItPushAwayFromStart(GameObject_s *, GIZBUILDIT_s *);
void GizBuildItPushAwayFromEnd(GameObject_s *);

void BuildIt_MoveCode(GameObject_s *player) {
    if (LEGOCONTEXT_BUILDIT == -1)
        return;
    if ((player->apiobj.character_data->model_flags & 0x2010) != 0)
        return;
    if (player->build_context != LEGOCONTEXT_BUILDIT) {
        if (LEGOACT_BUILD == -1 || player->apiobj.character_model->model_data_b[LEGOACT_BUILD] == NULL)
            return;
        if (AnimPlaying(&player->apiobj.anim_packet, LEGOACT_BUILD, 1, 1) != NULL)
            return;
        if (player->apiobj.field_0x27d == 0)
            return;
        if (ObjLandReady(player) == 0 && objInNetWaitContext(player, LEGOCONTEXT_BUILDIT) == 0)
            return;
        if (player->touch_task != NULL && player->touch_task->GetHashId().value != MechTouchTaskBuildIt::HashId.value)
            return;
        GIZBUILDIT_s *buildit = nextBuildit.Get() != NULL ? nextBuildit->GetGizBuildit() : NULL;
        if (buildit == NULL)
            buildit = GizBuildIt_FindNearest(WORLD, player, BUILDIT_FIND_AVAILABLE, ShadowMode);
        if (objInNetWaitContext(player, LEGOCONTEXT_BUILDIT) != 0) {
            player->context_animation_timer -= FRAMETIME;
            if (player->context_animation_timer <= 0.0f) {
                player->build_context = -1;
                player->big_jump_data = NULL;
            }
        }
        if (buildit == NULL || static_cast<i8>(player->apiobj.flags_low) >= 0)
            return;
        if (player->apiobj.field_0x27d != 0 && (buildit->state_flags & 4) != 0) {
            GizBuildItPushAwayFromStart(player, buildit);
        }
        GizBuildIt_SetHeadTarget(buildit, player);
        buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_INTERACTING;
        if ((player->pad_gamepad->buttons_pressed & GAMEPAD_SPECIAL) != 0 ||
            (objInNetWaitContext(player, LEGOCONTEXT_BUILDIT) != 0 &&
             (player->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0)) {
            if (GizBuildIt_CanStartBuildingFn != NULL && GizBuildIt_CanStartBuildingFn(buildit, player) == 0)
                return;
            nextBuildit.Reset();
            player->context_animation_timer = 0.4f;
            player->field_0x788 = buildit;
            player->build_button_taps = 0;
            player->build_context = LEGOCONTEXT_BUILDIT;
            player->context_animation = LEGOACT_BUILD;
            player->field_0xe21 &= ~0x40;
            GizBuildIt_SetStepTime(buildit, player);
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
            Hint_SetComplete(LEGOHINT_BUILD);
        }
        if (static_cast<i8>(player->apiobj.flags_low) < 0 && player->apiobj.field_0x27d != 0) {
            GizBuildItPushAwayFromEnd(player);
        }
        return;
    }
    if (BoltSys->stop_targeting != NULL)
        BoltSys->stop_targeting(player, &player->apiobj.collision_position);
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit == NULL)
        return;
    GizBuildIt_SetHeadTarget(buildit, player);
    buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if ((buildit->state_flags & 0x80) != 0 && ShadowMode == 0) {
        player->build_context = -1;
        buildit->builders_active = 1;
        static_cast<GIZBUILDIT_s *>(player->field_0x788)->availability_flags |= GIZBUILDIT_AVAILABILITY_INTERACTING;
    }
    const i32 animation = player->context_animation;
    if (player->apiobj.character_model->model_data_b[animation] != NULL &&
        AnimPlaying(&player->apiobj.anim_packet, animation, 1, 0) == NULL)
        return;
    buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit->build_state == 0 && (player->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0 &&
        player->context_animation_timer < 0.2f) {
        player->context_animation_timer = 0.2f;
    } else {
        player->context_animation_timer -= FRAMETIME;
        if (player->context_animation_timer <= 0.0f) {
            player->build_context = -1;
            return;
        }
    }
    if (player->build_context != -1) {
        buildit->builders_active = 1;
        static_cast<GIZBUILDIT_s *>(player->field_0x788)->availability_flags |= GIZBUILDIT_AVAILABILITY_INTERACTING;
    }
}

void GizBuildit_Reset(GIZBUILDIT_s *buildit, void *world_ptr) {
    buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_ACTIVE | GIZBUILDIT_AVAILABILITY_VISIBLE;
    buildit->field_0x83 &= static_cast<u8>(~GIZBUILDIT_RUNTIME_REWARD_RELEASED);

    buildit->start_position = buildit->file_position;
    buildit->position = buildit->file_position;
    buildit->bounds_radius = NuTrigTable[0x3000];

    if (buildit->anim_set != NULL && buildit->anim_set->objects != NULL) {
        NUVEC start_min = {1000000000.0f, 1000000000.0f, 1000000000.0f};
        NUVEC start_max = {-1000000000.0f, -1000000000.0f, -1000000000.0f};
        NUVEC end_min = start_min;
        NUVEC end_max = start_max;

        for (GAMEANIMOBJ_s *object = buildit->anim_set->objects; object != NULL; object = object->next) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
            memset(data, 0, sizeof(*data));

            NUMTX evaluated;
            EvalAnim(&object->special, object->start_frame, &evaluated, 1);
            data->start_mtx = evaluated;
            if (buildit->linked_buildit != NULL) {
                data->start_mtx.m30 -= buildit->linked_buildit->file_position.x;
                data->start_mtx.m31 -= buildit->linked_buildit->file_position.y;
                data->start_mtx.m32 -= buildit->linked_buildit->file_position.z;
                NuMtxRotateY(&data->start_mtx, buildit->field_0x7c);
                data->start_mtx.m30 += buildit->file_position.x;
                data->start_mtx.m31 += buildit->file_position.y;
                data->start_mtx.m32 += buildit->file_position.z;
                data->draw_mtx = data->start_mtx;
            }

            EvalAnim(&object->special, object->end_frame, &evaluated, 1);
            data->end_mtx = evaluated;
            if (buildit->linked_buildit != NULL) {
                data->end_mtx.m30 -= buildit->linked_buildit->file_position.x;
                data->end_mtx.m31 -= buildit->linked_buildit->file_position.y;
                data->end_mtx.m32 -= buildit->linked_buildit->file_position.z;
                NuMtxRotateY(&data->end_mtx, buildit->field_0x7c);
                data->end_mtx.m30 += buildit->file_position.x;
                data->end_mtx.m31 += buildit->file_position.y;
                data->end_mtx.m32 += buildit->file_position.z;
            }

            if (data->start_mtx.m30 < start_min.x)
                start_min.x = data->start_mtx.m30;
            if (data->start_mtx.m31 < start_min.y)
                start_min.y = data->start_mtx.m31;
            if (data->start_mtx.m32 < start_min.z)
                start_min.z = data->start_mtx.m32;
            if (data->start_mtx.m30 > start_max.x)
                start_max.x = data->start_mtx.m30;
            if (data->start_mtx.m31 > start_max.y)
                start_max.y = data->start_mtx.m31;
            if (data->start_mtx.m32 > start_max.z)
                start_max.z = data->start_mtx.m32;

            if (data->end_mtx.m30 < end_min.x)
                end_min.x = data->end_mtx.m30;
            if (data->end_mtx.m31 < end_min.y)
                end_min.y = data->end_mtx.m31;
            if (data->end_mtx.m32 < end_min.z)
                end_min.z = data->end_mtx.m32;
            if (data->end_mtx.m30 > end_max.x)
                end_max.x = data->end_mtx.m30;
            if (data->end_mtx.m31 > end_max.y)
                end_max.y = data->end_mtx.m31;
            if (data->end_mtx.m32 > end_max.z)
                end_max.z = data->end_mtx.m32;

            data->wobble_time = 0.2f;
        }

        buildit->start_position.x = (start_min.x + start_max.x) * 0.5f;
        buildit->start_position.y = (start_min.y + start_max.y) * 0.5f;
        buildit->start_position.z = (start_min.z + start_max.z) * 0.5f;
        buildit->position.x = (end_min.x + end_max.x) * 0.5f;
        buildit->position.y = (end_min.y + end_max.y) * 0.5f;
        buildit->position.z = (end_min.z + end_max.z) * 0.5f;

        f32 span = start_max.x - start_min.x;
        if (span < start_max.z - start_min.z) {
            span = start_max.z - start_min.z;
        }
        buildit->bounds_radius = span * NuTrigTable[0x3000];
    }

    if (buildit->field_0x5c == 0) {
        buildit->state_flags &= static_cast<u16>(~GIZBUILDIT_STATE_ROTATING_WOBBLE);
    } else {
        buildit->state_flags |= GIZBUILDIT_STATE_ROTATING_WOBBLE;
    }
    buildit->build_state = GIZBUILDIT_BUILD_IDLE;
    buildit->built_object_count = 0;

    if (world_ptr != NULL) {
        WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
        buildit->room_index = world->current_gscn != NULL
                                  ? static_cast<i16>(NuPortalWhichRoom(world->current_gscn, &buildit->file_position))
                                  : -1;
    }
}

void GizDrawBuildItPiece(GameObject_s *player, i32 draw_reflection) {
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit == NULL || buildit->builders_active == 0 || buildit->build_state != 0 ||
        buildit->anim_object_count == 0) {
        return;
    }
    f32 progress = 1.0f - buildit->step_timer / buildit->step_duration;
    GAMEANIMOBJ_s *object = buildit->anim_objects[buildit->built_object_count];
    GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
    NUMTX_ALIGNED16 matrix = data->start_mtx;
    if (progress > 0.0f) {
        NUMTX_ALIGNED16 start = data->start_mtx;
        NUMTX_ALIGNED16 end = data->end_mtx;
        start.m30 = start.m31 = start.m32 = 0.0f;
        end.m30 = end.m31 = end.m32 = 0.0f;
        NUQUAT from, to, rotation;
        NuMtxToQuat(&start, &from);
        NuMtxToQuat(&end, &to);
        NuQuatSlerp(&rotation, &from, &to, progress);
        NUMTX_ALIGNED16 interpolated;
        NuQuatToMtx(&rotation, &interpolated);
        interpolated.m30 = matrix.m30 + (data->end_mtx.m30 - matrix.m30) * progress;
        interpolated.m31 = matrix.m31 + (data->end_mtx.m31 - matrix.m31) * progress;
        interpolated.m32 = matrix.m32 + (data->end_mtx.m32 - matrix.m32) * progress;
        interpolated.m31 += 0.1f * NU_SIN_LUT(static_cast<i32>(32768.0f * progress));
        matrix = interpolated;
    }
    if (buildit->linked_buildit != NULL) {
        data->draw_mtx = matrix;
    } else {
        NuSpecialDrawAt(&object->special, &matrix);
        if (draw_reflection != 0) {
            extern i32 MatrixReflection(NUMTX *, i32, f32, f32, NUMTX *);
            NUMTX_ALIGNED16 reflection;
            if (MatrixReflection(&matrix, player->field_0x1087, player->field_0x1020, WORLD->current_level->unknown_0cc,
                                 &reflection) != 0) {
                NuRndrStartReflectionRender(0);
                NuSpecialDrawAt(&object->special, &reflection);
                NuRndrEndReflectionRender();
            }
        }
    }
}

NuMechPtr<MechObjectInterface, 4> nextBuildit;

void ForceBuildItToUseNext(GIZBUILDIT_s &buildit) {
    nextBuildit = NuMechPtr<MechObjectInterface, 4>(buildit.GetMechObjectInterface());
}

void GizGetBuildItPlayerPos(GameObject_s *player, nuvec_s *position, nuvec_s *target) {
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit == NULL || buildit->anim_set == NULL || buildit->anim_object_count == 0) {
        return;
    }
    i32 count = buildit->anim_object_count;
    i32 index = buildit->built_object_count;
    if (index >= count) {
        index = count - 1;
    }
    if (position != NULL) {
        i32 batch_size = buildit->field_0x78;
        index = (index / batch_size) * batch_size;
        i32 batch_count = 0;
        while (batch_count < batch_size && index + batch_count < count) {
            ++batch_count;
        }
        NUVEC total = v000;
        i32 end = index + batch_count;
        for (; index < end; ++index) {
            buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            NuVecAdd(&total, &total, NUMTX_GET_ROW_VEC(&data->start_mtx, 3));
        }
        if (batch_count > 1) {
            NuVecScale(position, &total, 1.0f / batch_count);
        } else {
            *position = total;
        }
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        position->x = (position->x + buildit->start_position.x) * 0.5f;
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        position->z = (position->z + buildit->start_position.z) * 0.5f;
        if (index >= count) {
            index = count - 1;
        }
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
        f32 dx = position->x - data->end_mtx.m30;
        f32 dz = position->z - data->end_mtx.m32;
        f32 inverse_distance = 1.0f / NuFsqrt(dx * dx + dz * dz);
        position->x += dx * inverse_distance * player->apiobj.field_0x1dc;
        position->z += dz * inverse_distance * player->apiobj.field_0x1dc;
    }
    if (target != NULL) {
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
        *target = *NUMTX_GET_ROW_VEC(&data->end_mtx, 3);
    }
}

void GizBuildit_SetVisibility(GIZBUILDIT_s *buildit, i32 visible) {
    if (buildit == NULL) {
        return;
    }

    GameAnimSet_SetVisibility(buildit->anim_set, visible);
    buildit->availability_flags = static_cast<u8>((buildit->availability_flags & ~GIZBUILDIT_AVAILABILITY_VISIBLE) |
                                                  (visible != 0 ? GIZBUILDIT_AVAILABILITY_VISIBLE : 0));
}

void GizMoveAttractoBuildItPiece(GIZBUILDIT_s *buildit, GAMEANIMOBJ_s *piece) {
    f32 progress = 1.0f - buildit->step_timer / buildit->step_duration;
    GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(piece->object_data);
    NUMTX *draw = NuSpecialGetDrawMtx(&piece->special);
    if (progress > 0.0f) {
        NUMTX result;
        NUMTX from = *draw;
        from.m30 = from.m31 = from.m32 = 0.0f;
        NUMTX to = data->end_mtx;
        to.m30 = to.m31 = to.m32 = 0.0f;
        NUQUAT rotation, from_rotation, to_rotation;
        NuMtxToQuat(&from, &from_rotation);
        NuMtxToQuat(&to, &to_rotation);
        NuQuatSlerp(&rotation, &from_rotation, &to_rotation, progress);
        NuQuatToMtx(&rotation, &result);
        NUVEC delta = {data->end_mtx.m30 - draw->m30, data->end_mtx.m31 - draw->m31, data->end_mtx.m32 - draw->m32};
        delta.x *= progress;
        delta.y *= progress;
        delta.z *= progress;
        result.m30 = delta.x + draw->m30;
        result.m31 = delta.y + draw->m31;
        result.m32 = delta.z + draw->m32;
        result.m31 += 0.1f * NU_SIN_LUT(progress * 32768.0f);
        *draw = result;
    }
    if (buildit->linked_buildit != NULL) {
        data->draw_mtx = *draw;
    } else {
        NuSpecialSetDrawMtx(&piece->special, draw);
    }
}
