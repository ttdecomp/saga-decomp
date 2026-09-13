#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nutex.h"
#include "legoapi/world/world.h"
#include "gameapi/ai/aisys/aisys.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/gizmos/object/lever.h"
#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/gizmos/door/spinner.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "nu2api/numath/nutrig.h"
#include "globals.h"
#include "legoapi/characters/core/charconfig.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void AITriggerSetSysReset(AITRIGGERSETSYS_s *system) {
    if (system == NULL) {
        return;
    }

    memset(system, 0, sizeof(*system));
    for (i32 i = 0; i < 64; ++i) {
        system->field_0x4280[i] = -1;
        system->field_0x42c0[i] = -1;
    }
    for (i32 i = 0; i < 32; ++i) {
        for (i32 j = 0; j < 8; ++j) {
            system->sets[i].trigger_indices[j] = -1;
        }
    }
}

extern "C" void *AISysBufferAlloc(VARIPTR *, VARIPTR *, u32);

AITRIGGERSETSYS_s *AITriggerSetSysCreate(VARIPTR *buffer, VARIPTR *buffer_end) {
    AITRIGGERSETSYS_s *system =
        static_cast<AITRIGGERSETSYS_s *>(AISysBufferAlloc(buffer, buffer_end, sizeof(AITRIGGERSETSYS_s)));
    AITriggerSetSysReset(system);
    return system;
}

AITRIGGERSET_s *AITriggerSetCreate(AITRIGGERSETSYS_s *system, FLOWBOX_s *box) {
    if (system != NULL) {
        for (i32 index = 0; index < 32; ++index) {
            AITRIGGERSET_s *set = &system->sets[index];
            if (!(set->flags & 1)) {
                set->flags |= 1;
                set->flowbox = box;
                return set;
            }
        }
    }
    return NULL;
}

i32 AITriggerSetAddTrigger(AISYS_s *, AITRIGGERSET_s *, GIZMO_s *);
void AISysGetPathPos2(AISYS_s *, nuvec_s *, AIPATHINFO_s *, nuvec_s *, AIPATH_s *, i32);

void AITriggerSysAutoSetUp(WORLDINFO_s *world, AITRIGGERSETSYS_s *system) {
    AITRIGGERSET_s *groups[32] = {};
    if (world != NULL && world->giz_flow != NULL) {
        GIZFLOW_s *flow = world->giz_flow;
        FLOWBOX_s *box = flow->flowboxes;
        for (i32 index = 0; index < flow->flowbox_count; ++index, ++box) {
            if (box->type == 0 && box->ai_trigger_group != 0 && box->ai_trigger_group <= 32) {
                i32 group_index = box->ai_trigger_group - 1;
                FLOWBOXGIZMODATA_s *data = box->data;
                AITRIGGERSET_s *set = groups[group_index];
                if (set == NULL) {
                    set = AITriggerSetCreate(system, NULL);
                    groups[group_index] = set;
                }
                if (set != NULL) {
                    for (i32 gizmo_index = 0; gizmo_index < data->gizmo_count; ++gizmo_index) {
                        AITriggerSetAddTrigger(world->ai_sys, set, data->gizmos[gizmo_index]->gizmo);
                    }
                }
            }
        }
    }
}

i32 AITriggerSetAddTrigger(AISYS_s *system, AITRIGGERSET_s *set, GIZMO_s *gizmo) {
    if (set == NULL || system == NULL || !(set->flags & 1) || set->trigger_count >= 8 || gizmo == NULL)
        return 0;

    char *name = GizmoGetName(gizmo);
    AILOCATOR *locator = name != NULL ? AIPathFindLocator(system, name) : NULL;
    if (gizmo->type_id == lever_gizmotype_id) {
        LEVER_s *lever = static_cast<LEVER_s *>(gizmo->object);
        set->gizmos[set->trigger_count] = gizmo;
        if (locator != NULL) {
            set->locators[set->trigger_count] = *locator;
        } else {
            set->locators[set->trigger_count].position = lever->floor_position;
            AISysGetPathPos2(system, &set->locators[set->trigger_count].position,
                             &set->locators[set->trigger_count].path_info, &set->locators[set->trigger_count].position,
                             NULL, 0xff);
            set->locators[set->trigger_count].direction =
                NuAtan2D(lever->position.x - lever->floor_position.x, lever->position.z - lever->floor_position.z);
        }
    } else if (gizmo->type_id == obstacle_gizmotype_id) {
        GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
        if (obstacle->mode != 1 && obstacle->mode != 2 && obstacle->mode != 5 && obstacle->mode != 6 &&
            obstacle->mode != 7)
            return 0;
        set->gizmos[set->trigger_count] = gizmo;
        if (locator != NULL) {
            set->locators[set->trigger_count] = *locator;
        } else {
            set->locators[set->trigger_count].position = obstacle->secondary_position;
            AISysGetPathPos2(system, &set->locators[set->trigger_count].position,
                             &set->locators[set->trigger_count].path_info, &set->locators[set->trigger_count].position,
                             NULL, 0xff);
        }
        if (set->locators[set->trigger_count].path_info.connection == NULL)
            return 0;
        ++set->trigger_count;
        return 1;
    } else if (gizmo->type_id == spinner_gizmotype_id) {
        GIZSPINNER_s *spinner = static_cast<GIZSPINNER_s *>(gizmo->object);
        set->gizmos[set->trigger_count] = gizmo;
        if (locator != NULL) {
            set->locators[set->trigger_count] = *locator;
        } else {
            set->locators[set->trigger_count].position = spinner->position;
            AISysGetPathPos2(system, &set->locators[set->trigger_count].position,
                             &set->locators[set->trigger_count].path_info, &set->locators[set->trigger_count].position,
                             NULL, 0xff);
        }
    } else if (gizmo->type_id == force_gizmotype_id) {
        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
        set->gizmos[set->trigger_count] = gizmo;
        if (locator != NULL) {
            set->locators[set->trigger_count] = *locator;
        } else {
            set->locators[set->trigger_count].position = force->file_position;
            f32 height = GameShadow(NULL, &set->locators[set->trigger_count].position, 5.0f, -1);
            if (height != 2000000.0f && height > set->locators[set->trigger_count].position.y)
                set->locators[set->trigger_count].position.y = height;
            AISysGetPathPos2(system, &set->locators[set->trigger_count].position,
                             &set->locators[set->trigger_count].path_info, &set->locators[set->trigger_count].position,
                             NULL, 0xff);
        }
    } else if (gizmo->type_id == grapple_gizmotype_id) {
        GRAPPLE_s *grapple = static_cast<GRAPPLE_s *>(gizmo->object);
        set->gizmos[set->trigger_count] = gizmo;
        if (locator != NULL) {
            set->locators[set->trigger_count] = *locator;
        } else {
            set->locators[set->trigger_count].position = grapple->ground_position;
            f32 height = GameShadow(NULL, &set->locators[set->trigger_count].position, 5.0f, -1);
            if (height != 2000000.0f && height > set->locators[set->trigger_count].position.y)
                set->locators[set->trigger_count].position.y = height;
            AISysGetPathPos2(system, &set->locators[set->trigger_count].position,
                             &set->locators[set->trigger_count].path_info, &set->locators[set->trigger_count].position,
                             NULL, 0xff);
            set->gizmos[set->trigger_count] = gizmo;
        }
    } else {
        return 0;
    }
    if (set->locators[set->trigger_count].path_info.connection == NULL)
        return 0;
    ++set->trigger_count;
    return 1;
}

i32 GameObjectUsingLever(GameObject_s *object, LEVER_s *lever) {
    if (object->character_context == 0x4a)
        return object->field_0x788 == lever;
    return 0;
}

GameObject_s *Grapple_Occupied(GRAPPLE_s *, GameObject_s *, AIPATHCNX_s *);

void AITriggerSetSysProcess(AITRIGGERSETSYS_s *system) {
    if (system == NULL || player == NULL)
        return;
    // The original accumulates route membership across all sets in this pass.
    u64 character_mask = 0;
    AITRIGGERSET_s *set = system->sets;
    for (i32 set_index = 0; set_index < 32; ++set_index, ++set) {
        if (!(set->flags & 1))
            continue;
        i32 complete = set->flowbox != NULL ? (set->flowbox->state_flags_low >> 1) & 1 : 0;
        i32 has_assignments = 0;
        for (i32 index = 0; index < set->trigger_count; ++index) {
            i32 assigned = set->trigger_indices[index];
            if (assigned != -1)
                has_assignments = 1;
            u16 bit = 1u << index;
            GIZMO_s *gizmo = set->gizmos[index];
            bool release = false;
            if (gizmo == NULL) {
                set->field_0x20e &= ~bit;
                release = true;
            } else if (gizmo->type_id == lever_gizmotype_id) {
                LEVER_s *lever = static_cast<LEVER_s *>(gizmo->object);
                if (GameObjectUsingLever(player, lever) || (player2 != NULL && GameObjectUsingLever(player2, lever))) {
                    set->field_0x20e |= bit;
                } else if ((set->field_0x20e & bit) && !Lever_BeingPulled(lever)) {
                    set->field_0x20e &= ~bit;
                }
                if (Lever_BeingPulled(lever) && set->trigger_indices[index] != -1) {
                    GameObject_s *object = &Obj[set->trigger_indices[index]];
                    if ((object->apiobj.object_flags & 0x1001) != 0x1001 || !GameObjectUsingLever(object, lever))
                        release = true;
                }
            } else if (gizmo->type_id == obstacle_gizmotype_id) {
                GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
                if (obstacle->triggering_object == player ||
                    (player2 != NULL && obstacle->triggering_object == player2)) {
                    set->field_0x20e |= bit;
                } else if (set->field_0x20e & bit) {
                    if (obstacle->anim_set != NULL &&
                        (obstacle->anim_set->state == 3 || obstacle->anim_set->state == 0 ||
                         obstacle->anim_set->state == 4))
                        set->field_0x20e &= ~bit;
                } else if ((obstacle->progress_flags & 3) != 3) {
                    release = true;
                }
            } else if (gizmo->type_id == spinner_gizmotype_id) {
                GIZSPINNER_s *spinner = static_cast<GIZSPINNER_s *>(gizmo->object);
                if ((spinner->flags & 6) || ((spinner->flags & 8) && !ShadowMode) || spinner->field_70 == 1.0f ||
                    (spinner->state_flags & 0x20)) {
                    set->field_0x20e &= ~bit;
                } else if ((player->character_context == 0x28 && player->field_0x788 == spinner) ||
                           (player2 != NULL && player2->character_context == 0x28 && player2->field_0x788 == spinner)) {
                    set->field_0x20e |= bit;
                } else if ((set->field_0x20e & bit) && spinner->room_index == -1) {
                    set->field_0x20e &= ~bit;
                }
            } else if (gizmo->type_id == force_gizmotype_id) {
                GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
                if (GizForce_GameObjUsingForce(player, force) || GizForce_GameObjUsingForce(player2, force)) {
                    set->field_0x20e |= bit;
                } else if (set->field_0x20e & bit) {
                    set->field_0x20e &= ~bit;
                } else if (GizForce_Complete(force)) {
                    release = true;
                }
            } else if (gizmo->type_id == grapple_gizmotype_id) {
                GRAPPLE_s *grapple = static_cast<GRAPPLE_s *>(gizmo->object);
                GameObject_s *object = assigned != -1 ? &Obj[assigned] : NULL;
                if (((i8)player->character_context == LEGOCONTEXT_GRAPPLE && player->field_0x788 == grapple) ||
                    (player2 != NULL && (i8)player2->character_context == LEGOCONTEXT_GRAPPLE &&
                     player2->field_0x788 == grapple)) {
                    set->field_0x20e |= bit;
                } else if (set->field_0x20e & bit) {
                    set->field_0x20e &= ~bit;
                } else if (Grapple_Occupied(grapple, object, NULL)) {
                    release = true;
                }
            } else {
                set->field_0x20e &= ~bit;
            }
            if (release && set->trigger_indices[index] != -1) {
                system->field_0x4280[set->trigger_indices[index]] = -1;
                system->field_0x42c0[set->trigger_indices[index]] = -1;
                set->trigger_indices[index] = -1;
            }
        }
        i32 player_count = player2 != NULL ? 2 : 1;
        if ((set->field_0x20e != 0 || (set->flags & 2)) && !complete) {
            if (has_assignments && set->field_0x20e == 0) {
                for (i32 index = 0; index < set->trigger_count; ++index) {
                    i32 assigned = set->trigger_indices[index];
                    if (assigned != -1 && Obj[assigned].trigger_set != set) {
                        system->field_0x4280[assigned] = -1;
                        system->field_0x42c0[set->trigger_indices[index]] = -1;
                        set->trigger_indices[index] = -1;
                    }
                }
            }
            i32 assigned_count = 0;
            for (i32 index = 0; index < set->trigger_count && assigned_count + player_count < set->trigger_count;
                 ++index) {
                if (!(set->field_0x20e & (1u << index)) && set->trigger_indices[index] == -1) {
                    GIZMO_s *gizmo = set->gizmos[index];
                    bool available = false;
                    if (gizmo->type_id == lever_gizmotype_id) {
                        available = (static_cast<LEVER_s *>(gizmo->object)->flags & 0x82) == 0x80;
                    } else if (gizmo->type_id == obstacle_gizmotype_id) {
                        GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(gizmo->object);
                        available = (obstacle->progress_flags & 1) && obstacle->anim_set->state != 2;
                    } else if (gizmo->type_id == spinner_gizmotype_id) {
                        available = static_cast<GIZSPINNER_s *>(gizmo->object)->room_index == -1;
                    } else if (gizmo->type_id == force_gizmotype_id) {
                        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
                        available =
                            (force->progress_flags & 1) && force->using_object == NULL && force->field_0x3c == 0;
                    } else if (gizmo->type_id == grapple_gizmotype_id) {
                        available = Grapple_Occupied(static_cast<GRAPPLE_s *>(gizmo->object), NULL, NULL) == NULL;
                    }
                    if (available) {
                        AILOCATOR *locator = &set->locators[index];
                        u64 routes = locator->path_info.connection->route_mask;
                        if (routes != 0) {
                            AIPATH_s *path = locator->path_info.path;
                            for (i32 route = 0; route < path->route_count; ++route)
                                if ((routes >> (route & 63)) & 1)
                                    character_mask |= path->routes[route].character_masks[0];
                        }
                        i32 best = -1;
                        f32 best_distance = 1000000000.0f;
                        GameObject_s *object = Obj;
                        for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index, ++object) {
                            if (system->field_0x42c0[object_index] != -1 ||
                                (object->apiobj.object_flags & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
                                (object->apiobj.flags_low & 0x80) || (object->field_0xeff & 1) ||
                                (character_mask && !(character_mask & object->ai.character_type_mask)) ||
                                !(object->field_0xf02 & 4) || (!set->field_0x20e && object->trigger_set != set))
                                continue;
                            if (!FreePlay) {
                                GIZMO_s *target = set->gizmos[index];
                                if (target->type_id == lever_gizmotype_id && !(object->field_0xefe & 0x80))
                                    continue;
                                if (target->type_id == force_gizmotype_id) {
                                    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(target->object);
                                    if (force->config_flags & 0x10) {
                                        if (!CharCategory_IsCategory(object, 1))
                                            continue;
                                        if (!(force->config_flags & 0x10) && !CharCategory_IsCategory(object, 0))
                                            continue;
                                    } else if (!CharCategory_IsCategory(object, 0)) {
                                        continue;
                                    }
                                } else if (target->type_id == obstacle_gizmotype_id) {
                                    if (static_cast<GIZOBSTACLE_s *>(target->object)->mode == 7)
                                        continue;
                                } else if (target->type_id == grapple_gizmotype_id && !(object->ai.capabilities & 8)) {
                                    continue;
                                }
                            }
                            NUVEC difference;
                            f32 distance = NuVecDistSqr(&locator->position, &object->ai.terrain_origin, &difference);
                            if (distance < best_distance) {
                                best_distance = distance;
                                best = object_index;
                            }
                        }
                        if (best != -1) {
                            set->trigger_indices[index] = best;
                            system->field_0x4280[best] = set_index;
                            system->field_0x42c0[best] = index;
                        }
                    }
                }
                if (set->trigger_indices[index] != -1)
                    ++assigned_count;
            }
        } else if (has_assignments) {
            for (i32 index = 0; index < set->trigger_count; ++index) {
                if (set->trigger_indices[index] != -1) {
                    system->field_0x4280[set->trigger_indices[index]] = -1;
                    system->field_0x42c0[set->trigger_indices[index]] = -1;
                    set->trigger_indices[index] = -1;
                }
            }
        }
        set->flags &= ~2;
    }
}
