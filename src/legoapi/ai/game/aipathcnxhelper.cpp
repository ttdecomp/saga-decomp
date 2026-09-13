#include "decomp.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "gameapi/ai/aisys/aisys.h"
#include "globals.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/gizmos/door/zipups.h"
#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/gizmos/object/newblowup.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nuanim3.h"

#include <string.h>
#include "nu2api/nucore/nulist.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" void *AIPAthFindPathCnx(AISYS_s *, AIPATH_s *, char *, char *, i32 *);
extern void *CutScene_FindInst(CUTSYS *, char *);
extern FLOWBOX_s *FlowBoxFindByName(GIZFLOW_s *, char *);

void AIPathCalcExtents(AIPATH *path) {
    const f32 max_float = 3.402823466e+38f;
    path->bounds_min.x = max_float;
    path->bounds_min.y = max_float;
    path->bounds_min.z = max_float;
    path->bounds_max.x = -max_float;
    path->bounds_max.y = -max_float;
    path->bounds_max.z = -max_float;

    for (i32 node_index = 0; node_index < path->node_count; ++node_index) {
        AIPATHNODE &node = path->nodes[node_index];
        const f32 min_x = node.position.x - node.radius;
        const f32 min_z = node.position.z - node.radius;
        const f32 max_x = node.position.x + node.radius;
        const f32 max_z = node.position.z + node.radius;

        if (min_x < path->bounds_min.x) {
            path->bounds_min.x = min_x;
        }
        if (node.min_height < path->bounds_min.y) {
            path->bounds_min.y = node.min_height;
        }
        if (min_z < path->bounds_min.z) {
            path->bounds_min.z = min_z;
        }
        if (max_x > path->bounds_max.x) {
            path->bounds_max.x = max_x;
        }
        if (node.max_height > path->bounds_max.y) {
            path->bounds_max.y = node.max_height;
        }
        if (max_z > path->bounds_max.z) {
            path->bounds_max.z = max_z;
        }
    }

    NUVEC margin;
    NuVecSub(&margin, &path->bounds_max, &path->bounds_min);
    NuVecScale(&margin, &margin, 0.1f);
    NuVecSub(&path->bounds_min, &path->bounds_min, &margin);
    NuVecAdd(&path->bounds_max, &path->bounds_max, &margin);
}

i32 AIPathCheckExtents(AIPATH *path, NUVEC *position) {
    if (path->bounds_min.x > position->x || path->bounds_min.y > position->y || path->bounds_min.z > position->z ||
        position->x > path->bounds_max.x || position->y > path->bounds_max.y) {
        return 0;
    }
    return position->z <= path->bounds_max.z;
}

void pathEditorDrawNode(nuvec_s *, float, float, float, u32, numtl_s *, i32, i32) {
}

void (*AIPathCnxHelperSysInitFn)(WORLDINFO_s *) = NULL;

void AIPathCnxHelperSysReset(WORLDINFO_s *world, AIPATHCNXHELPERSYS_s *system) {
    if (system != NULL) {
        if (system->helper_count != 0) {
            memset(system->helpers, 0, system->helper_count * sizeof(*system->helpers));
            system->helper_count = 0;
        }
        if (AIPathCnxHelperSysInitFn != NULL) {
            AIPathCnxHelperSysInitFn(world);
        }
    }
}

AIPATHCNXHELPER_s *AIPathCnxHelperSys_Find(AIPATHCNXHELPERSYS_s *system, GameObject_s *object, AIPATHCNX_s *connection,
                                           u8 direction, u8 type,
                                           i32 (*filter)(AIPATHCNXHELPER_s *, GameObject_s *, AIPATHCNX_s *, u8, u8)) {
    if (connection == NULL && object != NULL) {
        connection = object->ai.path_info.connection;
        direction = object->ai.path_info.direction;
    }
    if (connection == NULL || system == NULL) {
        return NULL;
    }

    for (i32 index = 0; index < system->helper_count; ++index) {
        AIPATHCNXHELPER_s *helper = &system->helpers[index];
        if (helper->connection == connection && (helper->direction == 0xff || helper->direction == direction) &&
            helper->type == type && (filter == NULL || filter(helper, object, connection, direction, type) == 0)) {
            return helper;
        }
    }
    return NULL;
}

void AIPathCnxControlSysReset(AIPATHCNXCONTROLSYS_s *system) {
    if (system == NULL) {
        return;
    }

    system->available_controllers.head = NULL;
    system->available_controllers.tail = NULL;
    system->active_controllers.head = NULL;
    system->active_controllers.tail = NULL;
    memset(system->controllers, 0, system->controller_count * sizeof(*system->controllers));
    for (i32 i = 0; i < system->controller_count; ++i) {
        NuLinkedListAppend(&system->available_controllers, &system->controllers[i].links);
    }
}

void AIPathCnxControlSysUpdate(AIPATHCNXCONTROLSYS_s *system) {
    if (system == NULL)
        return;
    for (NULISTLNK *node = NuLinkedListGetHead(&system->active_controllers); node != NULL;
         node = NuLinkedListGetNext(&system->active_controllers, node)) {
        AIPATHCNXCONTROLLER_s *controller = reinterpret_cast<AIPATHCNXCONTROLLER_s *>(node);
        controller->flags &= ~0x20;
        bool on = true;
        switch (controller->target_type) {
            case 0: {
                if ((controller->flags & 4) != 0 && NuSpecialGetVisibilityFn(&controller->special) == NULL) {
                    on = false;
                    break;
                }
                i32 frame = 0;
                nuinstanim_s *animation = NuSpecialGetInstAnim(&controller->special);
                if (animation != NULL) {
                    nuanimdata_s *data = controller->special.scene->instance_animation_data[animation->anim_ix];
                    if (data != NULL) {
                        float end = NuAnimEndFrameOld(data);
                        i32 current = static_cast<i32>(animation->ltime);
                        if (current > 0) {
                            frame = current < 1024 ? current : 1024;
                            i32 end_frame = static_cast<i32>(end);
                            frame = frame < end_frame ? frame : end_frame;
                            --frame;
                        }
                    }
                }
                on = (controller->on_frames[frame / 32] & (1u << (frame % 32))) != 0;
                break;
            }
            case 1: {
                const u8 *instance = static_cast<const u8 *>(controller->target);
                float end = *reinterpret_cast<f32 *>(*reinterpret_cast<u8 *const *>(instance + 0x58) + 8);
                i32 frame = static_cast<i32>(*reinterpret_cast<const f32 *>(instance + 0x90));
                if (frame > 0) {
                    i32 end_frame = static_cast<i32>(end);
                    frame = frame < end_frame ? frame : end_frame;
                    frame = frame < 1024 ? frame : 1024;
                    --frame;
                } else {
                    frame = 0;
                }
                on = (controller->on_frames[frame / 32] & (1u << (frame % 32))) != 0;
                break;
            }
            case 2:
                on = static_cast<GIZBUILDIT_s *>(controller->target)->build_state == 2;
                break;
            case 3:
                on = GizmoGetOutput(WORLD->gizmo_sys, static_cast<GIZMO *>(controller->target),
                                    controller->gizmo_output, 1) != 0;
                break;
            case 4:
                on = (static_cast<GIZMOBLOWUP_s *>(controller->target)->status_flags & 1) == 0;
                break;
            case 5: {
                i32 frame = static_cast<i32>(fakeanimframe[controller->fake_animation_id]);
                if (frame > 0) {
                    i32 end_frame = static_cast<i32>(fakeanimendframe[controller->fake_animation_id]);
                    frame = frame < end_frame ? frame : end_frame;
                    frame = frame < 1024 ? frame : 1024;
                    --frame;
                } else {
                    frame = 0;
                }
                on = (controller->on_frames[frame / 32] & (1u << (frame % 32))) != 0;
                break;
            }
            case 6:
                on = (static_cast<FLOWBOX_s *>(controller->target)->state_flags & 2) != 0;
                break;
            case 7:
                on = GizForce_Complete(static_cast<GIZFORCE_s *>(controller->target)) != 0;
                break;
            case 8:
                on = static_cast<GIZOBSTACLE_s *>(controller->target)->anim_set->state == GAMEANIMSET_STATE_AT_END;
                break;
            case 9:
                on = (static_cast<ZIPUP *>(controller->target)->flags & (ZIPUP_FLAG_ACTIVE | ZIPUP_FLAG_VISIBLE)) ==
                     (ZIPUP_FLAG_ACTIVE | ZIPUP_FLAG_VISIBLE);
                break;
        }
        if (on) {
            controller->flags |= 0x20;
            controller->connection->open = (controller->flags >> 3) & 1;
            controller->connection->traversal_flags[controller->flags & 1] &= ~controller->off_flags;
            controller->connection->traversal_flags[controller->flags & 1] |= controller->on_flags;
            if ((controller->flags & 2) != 0) {
                controller->connection->traversal_flags[(~controller->flags) & 1] &= ~controller->off_flags;
                controller->connection->traversal_flags[(~controller->flags) & 1] |= controller->on_flags;
            }
        } else {
            controller->connection->open = (controller->flags >> 4) & 1;
            controller->connection->traversal_flags[controller->flags & 1] &= ~controller->on_flags;
            controller->connection->traversal_flags[controller->flags & 1] |= controller->off_flags;
            if ((controller->flags & 2) != 0) {
                controller->connection->traversal_flags[(~controller->flags) & 1] &= ~controller->on_flags;
                controller->connection->traversal_flags[(~controller->flags) & 1] |= controller->off_flags;
            }
        }
    }
}

AIPATHCNXCONTROLLER_s *AIPathCnxControllerCreate(AIPATHCNXCONTROLSYS_s *control_system, AISYS_s *ai_system,
                                                 AIPATH_s *path, char *from, char *to, i32 target_type,
                                                 char *target_name, i32 fake_animation_id, i32 gizmo_output) {
    nuhspecial_s special = {};
    if (from == NULL || control_system == NULL || to == NULL) {
        return NULL;
    }

    i32 direction;
    AIPATHCNX *connection = static_cast<AIPATHCNX *>(AIPAthFindPathCnx(ai_system, path, from, to, &direction));
    void *target = NULL;
    if (connection != NULL) {
        switch (target_type) {
            case 0:
                if (target_name != NULL) {
                    NuSpecialFind(ai_system->scene, &special, target_name, 1);
                }
                break;
            case 1:
                target = CutScene_FindInst(WORLD->cutscene_sys, target_name);
                break;
            case 2:
                target = GizBuildIt_Find(WORLD, target_name);
                break;
            case 3:
                target = GizmoFindByName(WORLD->gizmo_sys, -1, target_name);
                break;
            case 4:
            case 7:
            case 8:
            case 9: {
                i32 gizmo_type = target_type == 4   ? blowup_gizmotype_id
                                 : target_type == 7 ? force_gizmotype_id
                                 : target_type == 8 ? obstacle_gizmotype_id
                                                    : zipup_gizmotype_id;
                GIZMO *gizmo = GizmoFindByName(WORLD->gizmo_sys, gizmo_type, target_name);
                target = gizmo != NULL ? gizmo->object : NULL;
                break;
            }
            case 5:
                if (fake_animation_id != 0) {
                    return NULL;
                }
                break;
            case 6:
                target = FlowBoxFindByName(WORLD->giz_flow, target_name);
                break;
            default:
                break;
        }
    }
    if (target == NULL && NuSpecialExistsFn(&special) == 0 && target_type != 5) {
        return NULL;
    }

    NULISTLNK *node = NuLinkedListGetHead(&control_system->available_controllers);
    if (node == NULL) {
        return NULL;
    }
    NuLinkedListRemove(&control_system->available_controllers, node);
    NuLinkedListAppend(&control_system->active_controllers, node);

    AIPATHCNXCONTROLLER_s *controller = reinterpret_cast<AIPATHCNXCONTROLLER_s *>(node);
    for (u8 index = 0; index < ai_system->path_sys->path_count; ++index) {
        if (ai_system->path_sys->paths[index] == path) {
            controller->path_index = index;
            break;
        }
    }
    controller->connection = connection;
    controller->flags = static_cast<u8>((controller->flags & ~1) | (direction & 1));
    controller->gizmo_output = gizmo_output;
    if (target != NULL) {
        controller->target = target;
    } else if (NuSpecialExistsFn(&special) != 0) {
        controller->special = special;
    } else {
        controller->fake_animation_id = fake_animation_id;
    }
    controller->target_type = static_cast<u8>(target_type);
    return controller;
}

void AIPathCnxControllerDestroy(AIPATHCNXCONTROLSYS_s *system, AIPATHCNXCONTROLLER_s *controller) {
    if (system == NULL || controller == NULL) {
        return;
    }
    NuLinkedListRemove(&system->active_controllers, &controller->links);
    memset(controller, 0, sizeof(*controller));
    NuLinkedListAppend(&system->available_controllers, &controller->links);
}

void AIPathCnxSetTemporaryBlock(AIPATH_s *path, char *from_name, char *to_name, i32 blocked) {
    if (from_name == NULL || path == NULL || to_name == NULL) {
        return;
    }
    AIPATHNODE *from = AIPathFindNode(NULL, path, from_name);
    AIPATHNODE *to = AIPathFindNode(NULL, path, to_name);
    if (to == NULL || from == NULL) {
        return;
    }
    const i32 to_index = to - path->nodes;
    for (i32 index = 0; index < from->connection_count; ++index) {
        AIPATHCNX *connection = from->connections[index];
        i32 direction;
        if (connection->node_indices[0] == to_index) {
            direction = 1;
        } else if (connection->node_indices[1] == to_index) {
            direction = 0;
        } else {
            continue;
        }
        if (blocked != 0) {
            connection->traversal_flags[direction] |= 0x80000000u;
        } else {
            connection->traversal_flags[direction] &= 0x7fffffffu;
        }
        return;
    }
}

AIPATHCNXHELPER_s *AIPathCnxHelperSys_AddHelper(AIPATHCNXHELPERSYS_s *system, AIPATHCNX_s *connection, u8 direction,
                                                void *target, u8 type) {
    if (system == NULL) {
        return NULL;
    }

    i16 helper_count = system->helper_count;
    if (helper_count >= system->field_0x00 || target == NULL || connection == NULL) {
        return NULL;
    }

    AIPATHCNXHELPER_s *helper = system->helpers;
    for (i32 index = 0; index < helper_count; ++index, ++helper) {
        if (helper->connection == connection && helper->type == type && helper->target == target) {
            if (helper->direction != direction) {
                helper->direction = 0xff;
            }
            return helper;
        }
    }

    helper = &system->helpers[helper_count];
    ++helper_count;
    system->helper_count = helper_count;
    helper->connection = connection;
    helper->target = target;
    helper->direction = direction;
    helper->type = type;
    return helper;
}

void pathEditorDrawConnectionInfo(nuvec_s *, float, nuvec_s *, u32, i32) {
}

void AIPathCnxControllerSetOnRange(AIPATHCNXCONTROLLER_s *controller, i32 start_frame, i32 end_frame) {
    if (controller == NULL) {
        return;
    }

    i32 animation_end = 1;
    switch (controller->target_type) {
        case 0: {
            nuinstanim_s *animation = NuSpecialGetInstAnim(&controller->special);
            if (animation != NULL) {
                nuanimdata_s *data = controller->special.scene->instance_animation_data[animation->anim_ix];
                if (data != NULL)
                    animation_end = static_cast<i32>(NuAnimEndFrameOld(data));
            }
            break;
        }
        case 1: {
            u8 *animation = *reinterpret_cast<u8 **>(static_cast<u8 *>(controller->target) + 0x58);
            animation_end = static_cast<i32>(*reinterpret_cast<f32 *>(animation + 8));
            break;
        }
        case 2:
        case 3:
        case 4:
        case 6:
            return;
        case 5:
            animation_end = static_cast<i32>(fakeanimendframe[controller->fake_animation_id]);
            break;
        default:
            animation_end = 1;
            break;
    }
    if (animation_end > 1024) {
        animation_end = 1024;
    }

    i32 first = start_frame == -1 ? animation_end : start_frame == -2 ? animation_end - 1 : start_frame;
    i32 last = end_frame == -1 ? animation_end : end_frame == -2 ? animation_end - 1 : end_frame;
    first = first > 0 ? first - 1 : 0;
    last = last < animation_end ? last : animation_end;
    for (i32 frame = first; frame < last; ++frame) {
        controller->on_frames[frame >> 5] |= 1u << (frame & 31);
    }
}
