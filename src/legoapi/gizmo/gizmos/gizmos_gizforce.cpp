#include "decomp.h"
#include "globals.h"
#include "legoapi/audio/sfx.h"
#include "nu2api/numath/numtx.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>

namespace {
    enum : u8 {
        GIZFORCE_CHARACTER_CONTEXT = 8,
    };
}

extern ADDPART_s Default_ADDPART;
extern "C" PART_s *AddPart(ADDPART_s *);
void MakeThrowVector(NUVEC *, NUVEC *, NUVEC *, NUVEC *, f32, f32);
void PartCollide_3D(PART_s *);
void PartKill_ForceThrow(PART_s *, i32);
void NewRumble(nupad_s *, f32, i32);

PART_s *GizForce_Throw(GameObject_s *object, GIZFORCE_s *force, float speed, float gravity, i32) {
    static NUVEC darth_centre = {19.05f, 4.0f, -105.23f};
    nuhspecial_s *special = &force->anim_set->objects->special;
    NUMTX local_matrix;
    NUMTX *matrix;
    if (WORLD->current_level == MAULF_LDATA) {
        NuMtxSetTranslation(&local_matrix, &darth_centre);
        matrix = &local_matrix;
    } else
        matrix = NuSpecialGetMtx(special);
    if (object == NULL || object->force_throw_target == NULL)
        return NULL;
    NUVEC velocity;
    MakeThrowVector(&velocity, (NUVEC *)&matrix->m30, &object->force_throw_target->apiobj.collision_position, &v000,
                    speed, gravity);
    ADDPART_s params = Default_ADDPART;
    params.matrix = matrix;
    params.velocity = &velocity;
    params.field_90 = object->apiobj.field_0x289;
    params.field_40 = PartCollide_3D;
    params.field_44 = PartKill_ForceThrow;
    params.special = special;
    params.gravity = gravity;
    params.owner = object;
    params.flags = 0xc39b;
    params.time_step = FRAMETIME;
    params.field_18 = 0.15f;
    params.field_14 = 0.15f;
    PART_s *part = AddPart(&params);
    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
    PlaySfx("JForcePush", &object->apiobj.collision_position);
    GameAnimSet_SetVisibility(force->anim_set, 0);
    force->field_0xaa |= 1;
    return part;
}

i32 GizForce_Complete(GIZFORCE_s *force) {
    if ((force->config_flags & GIZFORCE_CONFIG_WAIT_FOR_FORCE_RANGE) != 0 ||
        (force->force_range > 0.0f && (force->runtime_flags & GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE) == 0)) {
        return 0;
    }
    if ((force->config_flags & GIZFORCE_CONFIG_ALONG_SOCKET) != 0 &&
        (force->runtime_flags & GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN) == 0) {
        return 0;
    }
    return GizForce_AnimComplete(force);
}

void GizForce_ResetLOS(GameObject_s *object) {
    if (object->gizforce_los_info != NULL) {
        memset(object->gizforce_los_info, 0, sizeof(*object->gizforce_los_info));
    }
}

GIZFORCE_s *GizForce_FindByName(GIZFORCESYS_s *force_sys, char *name) {
    GIZFORCE_s *force = NULL;
    if (name == NULL || force_sys == NULL) {
        return force;
    }
    force = force_sys->forces;
    for (i32 index = 0; index < force_sys->count; ++index, ++force) {
        if (NuStrICmp(force->name, name) == 0) {
            return force;
        }
    }
    return force;
}

i32 GizForce_UpdateHint(HINT_s *) {
    for (i32 i = 0; i < 2; ++i) {
        GameObject_s *object = Player[i];
        if (object != NULL && (object->apiobj.flags_low & 0x80) != 0 && object->field_0xd80 > 0.0f &&
            object->force_glow_object != NULL && object->force_glow_kind == 0)
            return 1;
    }
    return 0;
}

GIZFORCE_s *GizForces_FindForce(WORLDINFO_s *world, char *name) {
    GIZMO *gizmo = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, name);
    return gizmo != NULL ? static_cast<GIZFORCE_s *>(gizmo->object) : NULL;
}

i32 GizForce_AnimComplete(GIZFORCE_s *force) {
    if (force != NULL && force->anim_set != NULL) {
        if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) == 0) {
            if (force->anim_set->state == GAMEANIMSET_STATE_AT_END) {
                return 1;
            }
        } else if (force->anim_set->state == GAMEANIMSET_STATE_AT_START) {
            return 1;
        }
        return 0;
    }
    return 1;
}

void GizForce_PlayForwards(GIZFORCE_s *force) {
    if (force == NULL) {
        return;
    }
    if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) == 0) {
        GameAnimSet_SetRepeating(force->anim_set, 0);
        f32 speed = force->animation_speed;
        if (speed < 0.0f) {
            GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
            return;
        }
        GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
        return;
    }
    GameAnimSet_SetRepeating(force->anim_set, 0);
    f32 speed = -force->animation_speed;
    if (force->animation_speed >= 0.0f) {
        GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
        return;
    }
    GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
}

i32 GizForce_StoodOnForce(GIZFORCE_s *force, GameObject_s *object) {
    i32 result = 0;
    GAMEANIMOBJ_s *anim_object;
    if ((force->runtime_flags & GIZFORCE_RUNTIME_HAS_PLATFORM) != 0 && object->field_0x1078 != -1 &&
        (anim_object = force->anim_set->objects) != NULL) {
        while (object->field_0x1078 != static_cast<GIZFORCEANIMDATA_s *>(anim_object->object_data)->platform_id) {
            anim_object = anim_object->next;
            if (anim_object == NULL) {
                return 0;
            }
        }
        result = 1;
    }
    return result;
}

void GizForce_PlayBackwards(GIZFORCE_s *force) {
    if (force == NULL) {
        return;
    }
    if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) == 0) {
        GameAnimSet_SetRepeating(force->anim_set, 0);
        f32 speed = -force->animation_speed;
        if (force->animation_speed < 0.0f) {
            GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
            return;
        }
        GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
        return;
    }
    GameAnimSet_SetRepeating(force->anim_set, 0);
    f32 speed = force->animation_speed;
    if (speed >= 0.0f) {
        GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
        return;
    }
    GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
}

void GizForce_SetVisibility(GIZFORCE_s *force, i32 visibility) {
    if (force != NULL) {
        GameAnimSet_SetVisibility(force->anim_set, visibility);
        force->progress_flags =
            static_cast<u8>((force->progress_flags & ~GIZFORCE_PROGRESS_VISIBLE) | ((visibility != 0) << 1));
    }
}

u16 GizForces_AngleToForce(nuvec_s *position, GIZFORCE_s *force) {
    return NuAtan2D(force->position.x - position->x, force->position.z - position->z);
}

i32 GizForce_GameObjUsingForce(GameObject_s *object, GIZFORCE_s *force) {
    return force != NULL && object != NULL && object->field_0x7a5 == GIZFORCE_CHARACTER_CONTEXT &&
           object->gizforce_target == force;
}

i32 SuperWeirdo(GameObject_s *);
i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);
extern "C" i32 TerrainPlatId();

i32 GizForce_FindBestForceTarget(GIZFORCESYS_s *force_sys, GameObject_s *object) {
    struct ForceTarget {
        GIZFORCE_s *force;
        GAMEANIMOBJ_s *animation;
        f32 distance;
        i32 index;
    };
    static ForceTarget possible_forcetargets[384];
    if (object == NULL || force_sys == NULL) {
        return 0;
    }
    i32 super_force = SuperWeirdo(object) || ((object->apiobj.flags_low & 0x80) != 0 && Cheat_IsOn(25));
    object->gizforce_target = NULL;
    object->gizforce_target_object = NULL;
    const f32 facing_x = NU_SIN_LUT(object->apiobj.movement_facing_angle);
    const f32 facing_z = NU_COS_LUT(object->apiobj.movement_facing_angle);
    i32 count = 0;
    for (i32 index = 0; index < force_sys->visible_force_count; ++index) {
        GIZFORCE_s *force = force_sys->visible_forces[index];
        if (force->using_object != NULL || force->field_0x3c_bits != 0) {
            continue;
        }
        if ((force->config_flags & 0x10) != 0 && (object->apiobj.character_data->model_flags & 4) == 0 && !super_force)
            continue;
        if (force->group == NULL) {
            if (GizForce_Complete(force) != 0) {
                continue;
            }
        } else if (force->group->count != 0) {
            GIZFORCE_s *last = force->group->forces[force->group->count - 1];
            if (last != force) {
                if ((force->group->field_0x24 & 1) != 0 || force->anim_set->state == GAMEANIMSET_STATE_AT_END)
                    continue;
                if (last != NULL && ((last->anim_set->flags & 7) != 0 || last->using_object != NULL))
                    continue;
            }
        }
        if (GizForce_StoodOnForce(force, object) != 0) {
            continue;
        }

        NUVEC_ALIGNED16 delta;
        f32 distance = NuVecDistSqr(&force->position, &object->apiobj.collision_position, &delta);
        if (distance > force->interaction_radius * force->interaction_radius)
            continue;
        if ((force->config_flags & GIZFORCE_CONFIG_TARGET_ANIMATION_OBJECTS) != 0) {
            for (GAMEANIMOBJ_s *anim_object = force->anim_set->objects; anim_object != NULL;
                 anim_object = anim_object->next) {
                if ((anim_object->flags & 1) != 0 ||
                    (object->force_glow_previous != NULL && object->force_glow_previous != anim_object)) {
                    continue;
                }
                distance = NuVecDistSqr(NuSpecialGetDrawPos(&anim_object->special), &object->apiobj.collision_position,
                                        &delta);
                if (delta.x * facing_x + delta.z * facing_z < 0.0f)
                    continue;
                possible_forcetargets[count].force = force;
                possible_forcetargets[count].animation = anim_object;
                possible_forcetargets[count].distance = distance;
                possible_forcetargets[count].index = anim_object - force_sys->anim_pool->objects;
                ++count;
            }
            continue;
        }
        if (delta.x * facing_x + delta.z * facing_z < 0.0f ||
            (object->force_glow_previous != NULL && object->force_glow_previous != force))
            continue;
        possible_forcetargets[count].force = force;
        possible_forcetargets[count].animation = NULL;
        possible_forcetargets[count].distance = distance;
        possible_forcetargets[count].index =
            force->anim_set->objects != NULL ? force->anim_set->objects - force_sys->anim_pool->objects : -1;
        ++count;
    }
    if (count == 0)
        return 0;
    GizForceLOSState_s *los = object->gizforce_los_info;
    if (los != NULL) {
        ForceTarget *oldest = NULL;
        f32 oldest_time = 1.0e9f;
        for (i32 i = 0; i < count; ++i) {
            f32 time = static_cast<f32>(los->words[possible_forcetargets[i].index + 12]);
            if (time < oldest_time) {
                oldest_time = time;
                oldest = &possible_forcetargets[i];
            }
        }
        if (oldest != NULL) {
            i32 index = oldest->index;
            los->words[index + 12] = LevelTimer.update_count;
            if ((oldest->force->config_flags & 0x800) == 0) {
                los->words[index >> 5] |= 1u << (index & 31);
            } else {
                los->words[index >> 5] &= ~(1u << (index & 31));
                NUVEC direction;
                i32 clear = 0;
                if (oldest->animation != NULL) {
                    NuVecSub(&direction, NuSpecialGetDrawPos(&oldest->animation->special),
                             &object->apiobj.collision_position);
                    clear = GameRayCast(&object->apiobj.collision_position, &direction, 0.0f, 0) == 0;
                    if (!clear && TerrainPlatId() != -1)
                        clear = static_cast<GIZFORCEANIMDATA_s *>(oldest->animation->object_data)->platform_id ==
                                TerrainPlatId();
                } else if ((oldest->force->field_0xaa & 0x80) == 0) {
                    NuVecSub(&direction, &oldest->force->position, &object->apiobj.collision_position);
                    clear = GameRayCast(&object->apiobj.collision_position, &direction, 0.0f, 0) == 0;
                    if (!clear && (oldest->force->runtime_flags & 1) != 0 && TerrainPlatId() != -1) {
                        for (GAMEANIMOBJ_s *animation = oldest->force->anim_set->objects; animation != NULL;
                             animation = animation->next) {
                            if (static_cast<GIZFORCEANIMDATA_s *>(animation->object_data)->platform_id ==
                                TerrainPlatId()) {
                                clear = 1;
                                break;
                            }
                        }
                    }
                }
                if (clear)
                    object->gizforce_los_info->words[index >> 5] |= 1u << (index & 31);
            }
        }
    }
    ForceTarget *best = NULL;
    f32 best_distance = 1.0e9f;
    los = object->gizforce_los_info;
    for (i32 i = 0; i < count; ++i) {
        ForceTarget *target = &possible_forcetargets[i];
        if (los != NULL && (target->force == NULL || (target->force->field_0xaa & 0x80) == 0) && target->index != -1 &&
            (los->words[target->index >> 5] & (1u << (target->index & 31))) == 0)
            continue;
        f32 distance = target->distance;
        if (object->force_glow_previous != NULL &&
            (object->force_glow_previous == target->force || object->force_glow_previous == target->animation))
            distance = -1.0f;
        if (distance < best_distance) {
            best_distance = distance;
            best = target;
        }
    }
    if (best != NULL) {
        object->gizforce_target = best->force;
        object->gizforce_target_object = best->animation;
    }
    return 0;
}
