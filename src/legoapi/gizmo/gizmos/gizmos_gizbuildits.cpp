#include "decomp.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuvec.h"

void (*GizBuildIt_FinishFn)(GIZBUILDIT_s *) = NULL;
i32 (*GizBuildit_AutoBuildPosFn)(void *, NUVEC *, NUVEC *, u16 *) = NULL;
i16 GizBuilditGDeb[6] = {0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51};
i32 LEGOCONTEXT_BUILDIT = 0x2d;

void GizBuildIt_TurnOff(GIZBUILDIT_s *buildit);
void GizBuildIt_SetStepTime(GIZBUILDIT_s *buildit, GameObject_s *player);

f32 GizBuildItMul(GameObject_s *player) {
    if (static_cast<i8>(player->apiobj.field_0x1f8) < 0 && Player_HasFastBuild(player)) {
        return 3.0f;
    }

    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    i32 build_amount = player->build_button_taps;
    const i32 completed_count = buildit->built_object_count;
    const i32 total_count = buildit->anim_object_count;
    if ((completed_count - build_amount + total_count) / 2 <= completed_count) {
        build_amount = total_count - completed_count;
    }
    if (build_amount < 0) {
        build_amount = 0;
    } else if (build_amount > 10) {
        build_amount = 10;
    }
    return static_cast<f32>(build_amount) / 10.0f + 1.0f;
}

GIZBUILDIT_s *GizBuildIt_Find(WORLDINFO_s *world, char *name) {
    GIZBUILDITSYS_s *buildit_sys = world->giz_buildit_sys;
    if (buildit_sys != NULL && buildit_sys->count != 0) {
        GIZBUILDIT_s *buildit = buildit_sys->buildits;
        for (i32 i = 0; i < world->giz_buildit_sys->count; ++i, ++buildit) {
            if (NuStrICmp(buildit->name, name) == 0) {
                return buildit;
            }
        }
    }

    return NULL;
}

i32 GizBuildIt_AtEnd(GIZBUILDIT_s *buildit) {
    return buildit != NULL && buildit->build_state == GIZBUILDIT_BUILD_COMPLETE;
}

void GizBuildIt_Finish(GIZBUILDIT_s *buildit) {
    LOG_INFO_IF(buildit->build_state != GIZBUILDIT_BUILD_COMPLETE, "build completed name=%s buildit=%p", buildit->name,
                (void *)buildit);
    buildit->build_state = GIZBUILDIT_BUILD_COMPLETE;
    if ((buildit->state_flags & GIZBUILDIT_STATE_TURN_OFF_WHEN_COMPLETE) != 0) {
        GizBuildIt_TurnOff(buildit);
    }
    if (GizBuildIt_FinishFn != NULL) {
        GizBuildIt_FinishFn(buildit);
    }
}

i32 GizBuildIt_AtStart(GIZBUILDIT_s *buildit) {
    return buildit != NULL && buildit->builders_active == 0 && buildit->built_object_count == 0;
}

void GizBuildIt_TurnOff(GIZBUILDIT_s *buildit) {
    if (buildit == NULL) {
        return;
    }
    if (buildit->linked_buildit == NULL) {
        GameAnimSet_SetVisibility(buildit->anim_set, 0);
    }
    buildit->availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_VISIBLE);
}

void GizBuildIt_SetToEnd(GIZBUILDIT_s *buildit) {
    if (buildit == NULL) {
        return;
    }

    buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_VISIBLE;
    if (buildit->linked_buildit == NULL) {
        GameAnimSet_SetVisibility(buildit->anim_set, 1);
        GameAnimSet_JumpToEnd(buildit->anim_set);
        GizBuildIt_SetStepTime(buildit, NULL);
    } else {
        for (i32 index = 0; index < buildit->anim_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            data->draw_mtx = data->start_mtx;
        }
    }

    buildit->built_object_count = buildit->anim_set != NULL ? buildit->anim_object_count : 0;
    buildit->builders_active = 0;
    buildit->availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_INTERACTING);
    GizBuildIt_Finish(buildit);
}

extern ADDPART_s Default_ADDPART;
extern f32 FRAMETIME;
extern "C" PART_s *AddPart(ADDPART_s *);
void PartStop_Flickerer(PART_s *);
i32 PartDraw_Flickerer(PART_s *);
void PartImpact_Brick(PART_s *);

void GizBuildIt_KillParts(GIZBUILDIT_s *buildit) {
    if (buildit == NULL)
        return;
    NUVEC velocity = v000;
    if (buildit->anim_set == NULL)
        return;
    for (GAMEANIMOBJ_s *object = buildit->anim_set->objects; object != NULL; object = object->next) {
        ADDPART_s params = Default_ADDPART;
        NUMTX_ALIGNED16 matrix = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data)->end_mtx;
        params.matrix = &matrix;
        params.velocity = &velocity;
        params.special = &object->special;
        params.stop_fn = PartStop_Flickerer;
        params.draw_fn = PartDraw_Flickerer;
        params.field_3c = PartImpact_Brick;
        params.field_14 = 0.2f;
        params.field_18 = 0.2f;
        params.gravity = -4.0f;
        params.flags = 0x90;
        params.field_c4 = 1;
        params.time_step = FRAMETIME;
        PART_s *part = AddPart(&params);
        if (part != NULL) {
            f32 random = static_cast<f32>(qrand()) * 1.5259021893143654e-05f;
            part->field_100 = random + random + 7.0f;
        }
    }
}

void GizBuildIt_SetToStart(GIZBUILDIT_s *buildit, i32 emit_debris, i32 keep_built_pieces) {
    if (buildit == NULL || buildit->anim_set == NULL) {
        return;
    }

    const u8 restored_built_count = keep_built_pieces != 0 ? buildit->built_object_count : 0;
    const i32 first_piece_to_reset = restored_built_count;
    buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_VISIBLE;

    if (emit_debris != 0) {
        for (i32 index = 0; index < buildit->built_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            NUVEC position = {data->end_mtx.m30, data->end_mtx.m31, data->end_mtx.m32};
            AddGameDebris(WORLD->debris_sys, GizBuilditGDeb[qrand() / 0x2aab], &position);
        }
    }

    if (buildit->linked_buildit != NULL) {
        for (i32 index = first_piece_to_reset; index < buildit->anim_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            data->draw_mtx = data->start_mtx;
        }
    } else {
        GameAnimSet_SetVisibility(buildit->anim_set, 1);
        for (i32 index = first_piece_to_reset; index < buildit->anim_object_count; ++index) {
            GAMEANIMOBJ_s *object = buildit->anim_objects[index];
            object->instance_animation->playing = 0;
            object->instance_animation->ltime = object->start_frame;
        }
        GizBuildIt_SetStepTime(buildit, NULL);
    }

    buildit->builders_active = 0;
    buildit->built_object_count = restored_built_count;
    buildit->availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_INTERACTING);
    buildit->build_state = GIZBUILDIT_BUILD_IDLE;
}

GIZBUILDIT_s *GizBuildIt_AnyReacting(WORLDINFO_s *world) {
    GIZBUILDITSYS_s *system = world->giz_buildit_sys;
    if (system != NULL) {
        GIZBUILDIT_s *buildit = system->buildits;
        for (i32 i = 0; i < system->count; ++i, ++buildit) {
            if ((buildit->availability_flags &
                 (GIZBUILDIT_AVAILABILITY_VISIBLE | GIZBUILDIT_AVAILABILITY_INTERACTING)) ==
                    (GIZBUILDIT_AVAILABILITY_VISIBLE | GIZBUILDIT_AVAILABILITY_INTERACTING) &&
                buildit->builders_active == 0 && (buildit->availability_flags & GIZBUILDIT_AVAILABILITY_ACTIVE) != 0) {
                return buildit;
            }
        }
    }
    return NULL;
}

GIZBUILDIT_s *GizBuildIt_FindNearest(WORLDINFO_s *world, GameObject_s *player, BUILDIT_FIND_ENUM mode, i32 shadow) {
    GIZBUILDITSYS_s *system = world->giz_buildit_sys;
    if (system == NULL || player == NULL)
        return NULL;
    NUVEC position = player->apiobj.collision_position;
    const f32 bottom = player->apiobj.lower_position.y;
    const f32 top = player->apiobj.upper_position.y;
    GIZBUILDIT_s *nearest = NULL;
    f32 nearest_distance = 1000000.0f;
    GIZBUILDIT_s *buildit = system->buildits;
    for (i32 i = 0; i < world->giz_buildit_sys->count; ++i, ++buildit) {
        if (mode != BUILDIT_FIND_ANY) {
            if ((buildit->availability_flags & GIZBUILDIT_AVAILABILITY_VISIBLE) == 0 ||
                (buildit->availability_flags & GIZBUILDIT_AVAILABILITY_ACTIVE) == 0 ||
                ((buildit->state_flags & 0x80) != 0 && shadow == 0))
                continue;
            if (mode == BUILDIT_FIND_AVAILABLE) {
                if (buildit->build_state != 0 || LEGOCONTEXT_BUILDIT == -1)
                    continue;
                i32 occupied = 0;
                for (i32 p = 0; p < 8; ++p) {
                    if (Player[p] != NULL && Player[p]->build_context == LEGOCONTEXT_BUILDIT &&
                        Player[p]->field_0x788 == buildit)
                        occupied = 1;
                }
                if (occupied != 0)
                    continue;
            }
        }
        const f32 height = static_cast<f32>(static_cast<u32>(buildit->progress)) / 100.0f * buildit->bounds_radius;
        if (top < buildit->start_position.y - height || buildit->start_position.y + height < bottom)
            continue;
        const f32 radius = buildit->bounds_radius + 0.3f;
        const f32 distance = NuVecXZDistSqr(&buildit->start_position, &position, NULL);
        if (distance < radius * radius && distance < nearest_distance) {
            nearest_distance = distance;
            nearest = buildit;
        }
    }
    return nearest;
}

void GizBuildIt_SetStepTime(GIZBUILDIT_s *buildit, GameObject_s *player) {
    buildit->step_duration = 0.3f;
    if (player != NULL) {
        buildit->step_duration /= GizBuildItMul(player);
    }
    buildit->step_timer = buildit->step_duration;
}

i32 GizBuildIts_UpdateHint(HINT_s *hint) {
    if (hint->completion_flags[MechInputTouchSystem::s_baseControlMode] != 0)
        return 0;
    return GizBuildIt_AnyReacting(WorldInfo_CurrentlyActive()) != NULL;
}

void SetHeadTarget(GameObject_s *, NUVEC *, i8, f32, f32, f32);

void GizBuildIt_SetHeadTarget(GIZBUILDIT_s *buildit, GameObject_s *player) {
    if (buildit->anim_object_count != 0) {
        u32 index = buildit->built_object_count;
        if (buildit->anim_object_count <= index)
            index = buildit->anim_object_count - 1;
        GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
        SetHeadTarget(player, reinterpret_cast<NUVEC *>(&data->start_mtx.m30), 0, 2.0f, 1.0f, 2.0f);
    }
}

void PushAway(NUVEC *, f32, NUVEC *, NUVEC *, GameObject_s *, GameObject_s *, f32, u32);

void GizBuildItPushAwayFromEnd(GameObject_s *player) {
    if (WORLD->giz_buildit_sys == NULL)
        return;
    GIZBUILDIT_s *buildit = WORLD->giz_buildit_sys->buildits;
    f32 player_x = player->apiobj.collision_position.x;
    f32 player_z = player->apiobj.collision_position.z;
    f32 radius_squared = -1000000000.0f;
    i32 nearest = 0;
    for (i32 i = 0; i < WORLD->giz_buildit_sys->count; ++i, ++buildit) {
        NUVEC centre = v000;
        if (!(buildit->state_flags & 8) || !(buildit->availability_flags & 2) || buildit->built_object_count == 0)
            continue;
        NUVEC end_position = buildit->position;
        NUVEC minimum, maximum;
        minimum.x = minimum.z = 1000000000.0f;
        maximum.x = maximum.z = -1000000000.0f;
        f32 radius;
        if (buildit->state_flags & 0x10) {
            f32 nearest_distance = 1000000000.0f;
            for (i32 j = 0; j < buildit->built_object_count; ++j) {
                GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[j]->object_data);
                f32 dx = data->end_mtx.m30 - player_x;
                f32 dz = data->end_mtx.m32 - player_z;
                f32 distance = dx * dx + dz * dz;
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest = j;
                    centre = *NUMTX_GET_ROW_VEC(&data->end_mtx, 3);
                }
            }
            NUVEC radius_centre;
            NuSpecialGetBounds(&buildit->anim_objects[nearest]->special, &minimum, &maximum);
            NuSpecialGetRadius(&buildit->anim_objects[nearest]->special, &radius_centre, &radius);
        } else {
            centre.y = end_position.y;
            for (i32 j = 0; j < buildit->built_object_count; ++j) {
                GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[j]->object_data);
                centre.x += data->end_mtx.m30;
                centre.z += data->end_mtx.m32;
                f32 dx = data->end_mtx.m30 - end_position.x;
                f32 dz = data->end_mtx.m32 - end_position.z;
                maximum.x = NuFmax(maximum.x, dx);
                maximum.z = NuFmax(maximum.z, dz);
                minimum.x = NuFmin(minimum.x, dx);
                minimum.z = NuFmin(minimum.z, dz);
                f32 distance = dx * dx + dz * dz;
                if (distance > radius_squared)
                    radius_squared = distance;
            }
            centre.x /= buildit->built_object_count;
            centre.z /= buildit->built_object_count;
            radius = NuFsqrt(radius_squared);
        }
        maximum.x += centre.x;
        maximum.y = centre.y + 0.01f;
        maximum.z += centre.z;
        minimum.x += centre.x;
        minimum.y = centre.y;
        minimum.z += centre.z;
        PushAway(&centre, radius, &minimum, &maximum, player, NULL, 1.0f, 0);
    }
}

void GizBuildItPushAwayFromStart(GameObject_s *player, GIZBUILDIT_s *buildit) {
    NUVEC minimum = v000, maximum = v000;
    if (buildit == NULL || player == NULL || buildit->anim_set == NULL || buildit->anim_object_count == 0)
        return;
    i32 count = buildit->anim_object_count;
    NUVEC start_position = buildit->start_position;
    f32 player_x = player->apiobj.collision_position.x;
    f32 player_z = player->apiobj.collision_position.z;
    NUVEC centre;
    f32 radius;
    minimum.x = minimum.z = 1000000000.0f;
    maximum.x = maximum.z = -1000000000.0f;
    if (buildit->state_flags & 0x10) {
        f32 nearest_distance = 1000000000.0f;
        i32 nearest = 0;
        for (i32 i = 0; i < count; ++i) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[i]->object_data);
            f32 dx = data->start_mtx.m30 - player_x;
            f32 dz = data->start_mtx.m32 - player_z;
            f32 distance = dx * dx + dz * dz;
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = i;
                centre = *NUMTX_GET_ROW_VEC(&data->start_mtx, 3);
            }
        }
        NUVEC radius_centre;
        NuSpecialGetBounds(&buildit->anim_objects[nearest]->special, &minimum, &maximum);
        NuSpecialGetRadius(&buildit->anim_objects[nearest]->special, &radius_centre, &radius);
    } else {
        centre = start_position;
        for (i32 i = 0; i < count; ++i) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[i]->object_data);
            f32 dx = data->start_mtx.m30 - start_position.x;
            f32 dz = data->start_mtx.m32 - start_position.z;
            maximum.x = NuFmax(maximum.x, dx);
            maximum.z = NuFmax(maximum.z, dz);
            minimum.x = NuFmin(minimum.x, dx);
            minimum.z = NuFmin(minimum.z, dz);
        }
        radius = NuFsqrt(maximum.x * maximum.x + maximum.z * maximum.z);
    }
    maximum.x += centre.x;
    maximum.z += centre.z;
    minimum.x += centre.x;
    minimum.z += centre.z;
    PushAway(&centre, radius, &minimum, &maximum, player, NULL, 1.0f, 0);
}

void GIZBUILDIT_s::ClearMechObjectInterface() {
    if (mech_object_interface != NULL)
        delete mech_object_interface;
}

MechObjectInterface *GIZBUILDIT_s::GetMechObjectInterface() {
    if (mech_object_interface != NULL)
        return mech_object_interface;
    new GizBuildItObjectInterface(*this);
    return mech_object_interface;
}
