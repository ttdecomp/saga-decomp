#include "decomp.h"
#include "gameapi/ai/aisys/aisys.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/core/charconfig.h"
#include "legoapi/gizmo/base/gizactions.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/props/doors/door.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nurand.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmos/object/lever.h"
#include "legoapi/gizmos/object/gizpanel.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspline.h"
#include "legoapi/render/fx/spline_position.h"

void Action_Sebulba(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

i32 Action_SetState(AISYS_s *, AISCRIPTPROCESS_s *processor, AIPACKET_s *, char **params, i32 param_count,
                    i32 is_first_time, float) {
    if (is_first_time == 0 || param_count == 0) {
        return 0;
    }

    processor->next_state = AIStateFind(params[0], processor->script);
    processor->unknown_flag_4 = 0;
    for (i32 param_index = 1; param_index < param_count; ++param_index) {
        if (NuStrICmp(params[param_index], "KeepBlockedMessages") == 0) {
            processor->unknown_flag_4 = 1;
        }
    }
    return 0;
}

f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
void AISysGetPathPos2(AISYS_s *, NUVEC *, AIPATHINFO_s *, NUVEC *, AIPATH_s *, i32);

i32 Action_UsePanel(AISYS_s *system, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params, i32 param_count,
                    i32 first_time, f32 elapsed) {
    if (packet == NULL || packet->owner == NULL || packet->owner->apiobj.objptr == NULL)
        return 1;
    GameObject_s *object = packet->owner->apiobj.objptr;
    if (first_time != 0) {
        for (i32 index = 0; index < param_count; ++index) {
            char *name = NuStrIStr(params[index], "name=");
            if (name == NULL)
                continue;
            GIZMO *gizmo = GizmoFindByName(WORLD->gizmo_sys, gizpanel_gizmotype_id, name + 5);
            if (gizmo == NULL || gizmo->object == NULL)
                continue;
            GIZPANEL *panel = static_cast<GIZPANEL *>(gizmo->object);
            processor->action_data_3 = panel;
            if ((panel->flags & 2) != 0)
                continue;
            processor->action_pos = panel->floor_position;
            f32 height = GameShadow(NULL, &processor->action_pos, 5.0f, -1);
            if (height != 2000000.0f)
                processor->action_pos.y = height;
            AISysGetPathPos2(system, &processor->action_pos, &processor->path_info, &processor->action_pos, NULL, 0xff);
        }
    }
    GIZPANEL *panel = static_cast<GIZPANEL *>(processor->action_data_3);
    if (panel == NULL || (panel->flags & 2) != 0)
        return 1;
    AIMoveInstruction(packet, &processor->action_pos, 0.0f, &processor->path_info, 1, 0.0f);
    if (GizPanel_CanUsePanel(object, panel) != 0) {
        f32 distance = NuVecDistSqr(&packet->terrain_origin, &processor->action_pos, NULL);
        if (distance < ai_moveradius * ai_moveradius) {
            packet->movement_look_target = &panel->position;
            object->pad_gamepad->buttons_pressed |= GAMEPAD_SPECIAL;
        }
    } else if (FreePlay != 0) {
        processor->action_timer -= elapsed;
        if (processor->action_timer < 0.0f) {
            processor->action_timer = 0.5f;
            object->pad_gamepad->buttons_pressed |= GAMEPAD_TOGGLERIGHT;
        }
    }
    return object->field_0x7a5 == 0x0b && object->field_0x788 == panel;
}

void Action_CameraCut(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_CreatePod(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_PullLever(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_UseTechno(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_NewSebulba(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_SetLapTime(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_MoveForward(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

void Action_EndCameraCut(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

i32 Action_FollowPlayer(AISYS_s *sys, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params, i32 param_count,
                        i32 first_time, float) {
    if (packet == NULL) {
        return 1;
    }

    if (first_time != 0) {
        for (i32 index = 0; index < param_count; ++index) {
            if (AIActionParseSpeedFn != NULL && AIActionParseSpeedFn(params[index], &packet->goal_speed_mode) != 0) {
                continue;
            }
            if (NuStrICmp(params[index], "ignore_radius") == 0) {
                processor->action_data_1 |= 2;
            } else if (NuStrICmp(params[index], "can_go_off_path") == 0) {
                processor->action_data_1 |= 1;
            } else {
                packet->movement_instruction_parameter = AIParamToFloatEx(packet, processor, params[index]);
            }
        }
    }

    if (sys->player_1 != NULL && sys->player_1->ai != NULL) {
        FollowAPIObject(&packet->owner->apiobj, sys->player_1, processor->action_data_1,
                        packet->movement_instruction_parameter);
    }
    return 0;
}

void Action_PlayCutScene(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

i32 Action_SetVisibility(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **params, i32 param_count, i32 first_time,
                         float) {
    if (first_time == 0) {
        return 1;
    }

    nuhspecial_s special = {};
    i32 visible = 1;
    for (i32 index = 0; index < param_count; ++index) {
        char *value = NuStrIStr(params[index], "name=");
        if (value != NULL) {
            NuSpecialFind(WORLD->current_gscn, &special, value + 5, 1);
        } else if (NuStrIStr(params[index], "FALSE") != NULL) {
            visible = 0;
        }
    }
    if (NuSpecialExistsFn(&special) != 0) {
        NuSpecialSetVisibility(&special, visible);
    }
    return 1;
}

i32 Action_HelpWithTriggers(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float);

i32 Action_UseTriggerSet(AISYS_s *system, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params,
                         i32 param_count, i32 first_time, f32 elapsed) {
    if (packet == NULL || packet->owner == NULL || packet->owner->apiobj.objptr == NULL || system == NULL ||
        system->player_1 == NULL)
        return 1;
    GameObject_s *object = packet->owner->apiobj.objptr;
    if (first_time != 0 && WORLD->ai_trigger_set_sys != NULL) {
        for (i32 i = 0; i < param_count; ++i) {
            char *value = NuStrIStr(params[i], "set=");
            if (value != NULL) {
                i32 index = static_cast<i32>(AIParamToFloat(processor, value + 4)) - 1;
                if (static_cast<u32>(index) < 32) {
                    processor->action_data_3 = &WORLD->ai_trigger_set_sys->sets[index];
                }
            }
        }
    }
    AITRIGGERSET_s *set = static_cast<AITRIGGERSET_s *>(processor->action_data_3);
    if (set != NULL) {
        object->active_trigger_set = set;
        set->flags |= 2;
        Action_HelpWithTriggers(system, processor, packet, params, param_count, first_time, elapsed);
    }
    return 0;
}

void Action_BoulderSection(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

i32 Action_ReleaseLocator(AISYS_s *sys, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char **params, i32 param_count,
                          i32 first_time, float) {
    if (first_time == 0) {
        return 1;
    }

    GameObject_s *object = packet != NULL && packet->owner != NULL ? packet->owner->apiobj.objptr : NULL;
    for (i32 index = 0; index < param_count; ++index) {
        char *value = NuStrIStr(params[index], "character=");
        if (value != NULL) {
            object = GetNamedGameObject(sys, value + sizeof("character=") - 1);
        }
    }
    if (object != NULL) {
        object->ai.locator = NULL;
    }
    return 1;
}

void GameCameraMakeMiniCut3(u32, float, i32, i32, i32, void *, i32, NUVEC *, float, float, float, float, float, float,
                            float, i32, nugspline_s *, char, char);

i32 Action_DynamicCameraCut(AISYS_s *system, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params,
                            i32 param_count, i32 first_time, float) {
    NUVEC target_position = {1000000000.0f, 1000000000.0f, 1000000000.0f};
    NUVEC camera_position = {1000000000.0f, 1000000000.0f, 1000000000.0f};
    NUVEC camera_offset = {0.0f, 0.0f, 0.0f};
    if (first_time != 0) {
        u32 flags = 0;
        f32 range = 1.0f;
        i32 rotation_x = 0, rotation_y = 0, rotation_z = 0;
        f32 start_time = 0.0f, blend_in_time = 0.0f, end_time = 1000000000.0f;
        f32 blend_out_time = 0.0f, blend_time = 0.0f, hold_time = 0.0f, max_time = 0.0f;
        i32 borders = 1, use_current_position = 0;
        char mode = -1, interpolation = -1;
        NUGSPLINE *spline = NULL;
        char *target_locator = NULL, *target_character = NULL, *target_object = NULL, *camera_locator = NULL;
        for (i32 index = 0; index < param_count; ++index) {
            char *value;
            if (NuStrICmp("use_current_campos", params[index]) == 0) {
                flags |= 0x100;
                use_current_position = 1;
            } else if ((value = NuStrIStr(params[index], "attach_to_spline=")) != NULL) {
                flags |= 0x2000;
                spline = NuSplineFind(WORLD->current_gscn, value + 17);
            } else if ((value = NuStrIStr(params[index], "spline_dist_offset=")) != NULL) {
                flags |= 0x4000;
                range = AIParamToFloat(processor, value + 17);
            } else if ((value = NuStrIStr(params[index], "start_time=")) != NULL) {
                flags |= 0x200;
                start_time = AIParamToFloat(processor, value + 11);
            } else if ((value = NuStrIStr(params[index], "blend_time=")) != NULL) {
                flags |= 0x800;
                blend_time = AIParamToFloat(processor, value + 11);
            } else if ((value = NuStrIStr(params[index], "blend_in_time=")) != NULL) {
                blend_in_time = AIParamToFloat(processor, value + 14);
            } else if ((value = NuStrIStr(params[index], "blend_out_time=")) != NULL) {
                blend_out_time = AIParamToFloat(processor, value + 15);
            } else if ((value = NuStrIStr(params[index], "end_time=")) != NULL) {
                flags |= 0x400;
                end_time = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "max_time=")) != NULL) {
                max_time = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "hold_time=")) != NULL) {
                flags |= 0x1000;
                hold_time = AIParamToFloat(processor, value + 10);
            } else if ((value = NuStrIStr(params[index], "tgt_locator=")) != NULL) {
                target_locator = value + 12;
            } else if ((value = NuStrIStr(params[index], "tgt_character=")) != NULL) {
                target_character = value + 14;
            } else if ((value = NuStrIStr(params[index], "tgt_obj=")) != NULL) {
                target_object = value + 8;
            } else if ((value = NuStrIStr(params[index], "cam_locator=")) != NULL) {
                camera_locator = value + 12;
            } else if ((value = NuStrIStr(params[index], "range=")) != NULL) {
                flags |= 1;
                range = AIParamToFloat(processor, value + 6);
            } else if ((value = NuStrIStr(params[index], "cont_roty=")) != NULL) {
                flags |= 0x20;
                rotation_y =
                    static_cast<i32>(static_cast<i32>(AIParamToFloat(processor, value + 10)) * 182.0444488525390625f);
            } else if ((value = NuStrIStr(params[index], "roty=")) != NULL) {
                flags |= 0x4;
                rotation_y =
                    static_cast<i32>(static_cast<i32>(AIParamToFloat(processor, value + 5)) * 182.0444488525390625f);
            } else if ((value = NuStrIStr(params[index], "cont_rotx=")) != NULL) {
                flags |= 0x10;
                rotation_x =
                    static_cast<i32>(static_cast<i32>(AIParamToFloat(processor, value + 10)) * 182.0444488525390625f);
            } else if ((value = NuStrIStr(params[index], "rotx=")) != NULL) {
                flags |= 0x2;
                rotation_x =
                    static_cast<i32>(-static_cast<i32>(AIParamToFloat(processor, value + 5)) * 182.0444488525390625f);
            } else if ((value = NuStrIStr(params[index], "cont_rotz=")) != NULL) {
                flags |= 0x40;
                rotation_z =
                    static_cast<i32>(static_cast<i32>(AIParamToFloat(processor, value + 10)) * 182.0444488525390625f);
            } else if ((value = NuStrIStr(params[index], "rotz=")) != NULL) {
                flags |= 0x8;
                rotation_z =
                    static_cast<i32>(-static_cast<i32>(AIParamToFloat(processor, value + 5)) * 182.0444488525390625f);
            } else if ((value = NuStrIStr(params[index], "campos_x=")) != NULL) {
                camera_position.x = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "campos_y=")) != NULL) {
                camera_position.y = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "campos_z=")) != NULL) {
                camera_position.z = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "tgtpos_x=")) != NULL) {
                target_position.x = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "tgtpos_y=")) != NULL) {
                target_position.y = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "tgtpos_z=")) != NULL) {
                target_position.z = AIParamToFloat(processor, value + 9);
            } else if ((value = NuStrIStr(params[index], "dcampos_x=")) != NULL) {
                camera_offset.x = AIParamToFloat(processor, value + 10);
            } else if ((value = NuStrIStr(params[index], "dcampos_y=")) != NULL) {
                camera_offset.y = AIParamToFloat(processor, value + 10);
            } else if ((value = NuStrIStr(params[index], "dcampos_z=")) != NULL) {
                camera_offset.z = AIParamToFloat(processor, value + 10);
            } else if (NuStrICmp("no_borders", params[index]) == 0) {
                borders = 0;
            } else if (NuStrICmp("mode=follow", params[index]) == 0) {
                mode = 0;
            } else if (NuStrICmp("mode=static", params[index]) == 0) {
                mode = 1;
            } else if (NuStrICmp("linear", params[index]) == 0) {
                interpolation = 0;
            } else if (NuStrICmp("slowfastslow", params[index]) == 0) {
                interpolation = 2;
            } else if (NuStrICmp("slowfast", params[index]) == 0) {
                interpolation = 1;
            } else if (NuStrICmp("fastslowfast", params[index]) == 0) {
                interpolation = 4;
            } else if (NuStrICmp("fastslow", params[index]) == 0) {
                interpolation = 3;
            }
        }
        NUVEC *target = NULL;
        if (target_locator != NULL) {
            AILOCATOR *locator = AIPathFindLocator(system, target_locator);
            if (locator != NULL)
                target = &locator->position;
        } else if (target_character != NULL) {
            GameObject_s *object;
            if (packet != NULL && packet->owner != NULL && NuStrICmp(target_character, "myself") == 0)
                object = packet->owner->apiobj.objptr;
            else
                object = GetNamedGameObject(system, target_character);
            if (object != NULL)
                target = &object->apiobj.collision_position;
        } else if (target_object != NULL) {
            nuhspecial_s special;
            if (NuSpecialFind(WORLD->current_gscn, &special, target_object, 1) != 0)
                target = NuSpecialGetDrawPos(&special);
        } else if (target_position.x != 1000000000.0f && target_position.y != 1000000000.0f &&
                   target_position.z != 1000000000.0f) {
            target = &target_position;
        }
        if (target != NULL)
            flags |= 0x80;

        i32 have_camera_position = 0;
        if (use_current_position != 0) {
            NUMTX *matrix = NuCameraGetMtx();
            if (matrix != NULL) {
                camera_position.x = matrix->m30;
                camera_position.y = matrix->m31;
                camera_position.z = matrix->m32;
                have_camera_position = 1;
            }
        } else if (camera_position.x != 1000000000.0f && camera_position.y != 1000000000.0f &&
                   camera_position.z != 1000000000.0f) {
            have_camera_position = 1;
        } else if (camera_locator != NULL) {
            AILOCATOR *locator = AIPathFindLocator(system, camera_locator);
            if (locator != NULL) {
                camera_position = locator->position;
                have_camera_position = 1;
            }
        }
        if (spline != NULL) {
            PointAlongSpline(spline, 0.0f, &camera_position, NULL, NULL, 0);
            if (mode == -1)
                mode = 2;
        } else if (have_camera_position != 0) {
            flags |= 0x100;
            NuVecAdd(&camera_position, &camera_position, &camera_offset);
            if (mode == -1)
                mode = 1;
        } else {
            if (target != NULL) {
                camera_position.x = 0.0f;
                camera_position.y = 0.0f;
                camera_position.z = range;
                NuVecRotateX(&camera_position, &camera_position, rotation_x);
                NuVecRotateY(&camera_position, &camera_position, rotation_y);
                NuVecAdd(&camera_position, &camera_position, target);
            }
            if (mode == -1)
                mode = 0;
        }
        GameCameraMakeMiniCut3(flags, range, rotation_x, rotation_y, rotation_z, target, 0, &camera_position,
                               start_time, blend_in_time, end_time, blend_out_time, blend_time, hold_time, max_time,
                               borders, spline, mode, interpolation);
    }
    return 1;
}

f32 party_follow_offsets[8];
extern "C" i32 party_under_cover;
extern f32 drop_back_in_timer;

i32 Action_GameFollowPlayer(AISYS_s *sys, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params,
                            i32 param_count, i32 first_time, float) {
    if (packet == NULL || packet->owner == NULL || packet->owner->apiobj.objptr == NULL || sys == NULL ||
        sys->player_1 == NULL) {
        return 1;
    }
    GameObject_s *object = packet->owner->apiobj.objptr;
    if (first_time != 0) {
        for (i32 i = 0; i < param_count; ++i) {
            if (AIActionParseSpeedFn != NULL && AIActionParseSpeedFn(params[i], &packet->goal_speed_mode) != 0) {
                continue;
            }
            if (NuStrICmp(params[i], "ignore_radius") == 0) {
                processor->action_data_1 |= 2;
            } else if (NuStrICmp(params[i], "can_go_off_path") == 0) {
                processor->action_data_1 |= 1;
            } else if (NuStrICmp(params[i], "hold_special_button") == 0) {
                processor->action_data_2 = 1;
            } else if (NuStrICmp(params[i], "nearest") == 0) {
                if (player != NULL && player2 != NULL) {
                    f32 first = NuVecDist(&packet->owner->apiobj.position, &player->apiobj.position, NULL);
                    f32 second = NuVecDist(&packet->owner->apiobj.position, &player2->apiobj.position, NULL);
                    if (second < first) {
                        processor->action_data_3 = player2;
                    } else {
                        processor->action_data_3 = player;
                    }
                } else {
                    processor->action_data_3 = player;
                }
            } else if (packet->movement_instruction_parameter == 0.0f) {
                packet->movement_instruction_parameter = AIParamToFloat(processor, params[i]);
            }
        }
    }
    if (processor->action_data_3 == NULL && player == NULL) {
        return 0;
    }
    APIOBJECT_s *owner = &packet->owner->apiobj;
    f32 distance;
    if (owner->field_0x27c != -1) {
        distance = drop_back_in_timer > 0.0f
                       ? 0.01f
                       : packet->movement_instruction_parameter + party_follow_offsets[owner->field_0x27c];
    } else {
        distance = packet->movement_instruction_parameter;
        if ((owner->field_0x1f4 & 0x10001) != 0 &&
            (party_under_cover != 0 || (player->apiobj.character_data->model_flags & 0x80000) != 0)) {
            distance = 1.5f > distance ? 1.5f : distance;
        }
    }
    FollowAPIObject(owner, &player->apiobj, processor->action_data_1, distance);
    if (processor->action_data_2 != 0) {
        GAMEPAD_s *pad = object->pad_gamepad;
        pad->buttons_pressed |= GAMEPAD_SPECIAL;
        pad->buttons_held |= GAMEPAD_SPECIAL;
    }
    return 0;
}

struct GIZSPINNER_s;
f32 GizSpinner_GetNearestTargetPoint(GIZSPINNER_s *, NUVEC *, NUVEC *, NUVEC *, i32);
void GameObjectSetCanUse(GameObject_s *, void *, u8, u8, f32);
void ClearSpecialMove(GameObject_s *);
extern i32 spinner_gizmotype_id;
extern i32 LEGOCONTEXT_GRAPPLE;
extern u32 GAMEPAD_SPECIAL, GAMEPAD_JUMP, GAMEPAD_TOGGLERIGHT;
extern f32 ai_moveradius;

i32 Action_HelpWithTriggers(AISYS_s *, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **, i32, i32,
                            f32 elapsed) {
    AITRIGGERSETSYS_s *system = WORLD->ai_trigger_set_sys;
    if (system == NULL || packet == NULL || packet->owner == NULL || packet->owner->apiobj.objptr == NULL)
        return 0;
    GameObject_s *object = packet->owner->apiobj.objptr;
    u8 object_index = packet->owner->apiobj.field_0x289;
    i32 trigger_index = system->field_0x42c0[object_index];
    if (trigger_index == -1)
        return 0;
    AITRIGGERSET_s *set = &system->sets[system->field_0x4280[object_index]];
    AITRIGGERSET_TARGET *target = &set->targets[trigger_index];
    GIZMO_s *gizmo = set->triggers[trigger_index];
    NUVEC spinner_position, spinner_direction;
    NUVEC *spinner_origin = NULL;
    if (gizmo->type_id == spinner_gizmotype_id) {
        spinner_origin = &packet->owner->apiobj.collision_position;
        GIZSPINNER_s *spinner = static_cast<GIZSPINNER_s *>(gizmo->object);
        f32 result =
            GizSpinner_GetNearestTargetPoint(spinner, spinner_origin, &spinner_position, &spinner_direction, 1);
        if (result == -1.0f)
            result =
                GizSpinner_GetNearestTargetPoint(spinner, spinner_origin, &spinner_position, &spinner_direction, 0);
        if (result != -1.0f) {
            NuVecAddScale(&spinner_position, &spinner_position, &spinner_direction, 0.5f);
            AIMoveInstruction(packet, &spinner_position, 0.0f, &target->path, 1, 0.0f);
        }
    } else if (gizmo->type_id == grapple_gizmotype_id) {
        GRAPPLE *grapple = static_cast<GRAPPLE *>(gizmo->object);
        if (object->character_context == LEGOCONTEXT_GRAPPLE && object->field_0x788 == grapple) {
            object->field_0x1094 = 5;
            GameObjectSetCanUse(object, grapple, 1, 50, 0.0f);
            ClearSpecialMove(object);
            for (i32 i = 0; i < 2; ++i) {
                GameObject_s *other = Player[i];
                if (other == NULL || other->character_context != 0x46)
                    continue;
                for (i32 j = 0; j < set->trigger_count; ++j) {
                    if (set->triggers[j] != NULL && set->triggers[j]->object == other->field_0x788) {
                        if (player->apiobj.position.y > object->apiobj.position.y + 0.05f)
                            object->field_0x107e = 1;
                        else if (object->apiobj.position.y - 0.05f > player->apiobj.position.y)
                            object->field_0x107e = 2;
                        return 0;
                    }
                }
            }
            return 0;
        }
        GameObjectSetCanUse(object, grapple, 1, 50, 0.0f);
        AIMoveInstruction(packet, &target->position, 0.0f, &target->path, 1, 0.0f);
    } else {
        AIMoveInstruction(packet, &target->position, 0.0f, &target->path, 1, 0.0f);
    }
    gizmo = set->triggers[trigger_index];
    if (gizmo == NULL)
        return 0;
    NUVEC offset;
    if (gizmo->type_id == lever_gizmotype_id) {
        if (object->apiobj.character_model->model_data_b[0x5d] == NULL)
            goto change_character;
        LEVER_s *lever = static_cast<LEVER_s *>(gizmo->object);
        if (NuVecXZDistSqr(&packet->terrain_origin, &target->position, &offset) < ai_moveradius * ai_moveradius) {
            packet->movement_look_target = &lever->position;
            object->pad_gamepad->buttons_down_08 |= GAMEPAD_SPECIAL;
        }
    } else if (gizmo->type_id == spinner_gizmotype_id) {
        if (object->apiobj.field_0x27d != 0 &&
            NuVecXZDistSqr(spinner_origin, &spinner_position, NULL) < packet->mover_height) {
            NuVecAddScale(&spinner_position, &spinner_position, &spinner_direction, -0.5f);
            AIMoveInstruction(packet, &spinner_position, 0.0f, &target->path, 1, 0.0f);
            object->field_0xf02 |= 2;
            object->apiobj.respawn_timer = 0.0f;
        }
    } else if (gizmo->type_id == force_gizmotype_id) {
        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
        if (CharCategory_IsCategory(object, (force->config_flags & 0x10) != 0 ? 1 : 0) == 0)
            goto change_character;
        if (NuVecXZDistSqr(&packet->terrain_origin, &target->position, &offset) < ai_moveradius * ai_moveradius) {
            packet->movement_look_target = &force->position;
            object->pad_gamepad->allocated_5a |= 4;
            object->gizforce_target = force;
        }
    } else if (gizmo->type_id == grapple_gizmotype_id) {
        GRAPPLE *grapple = static_cast<GRAPPLE *>(gizmo->object);
        if (processor->action_data_1 != 0) {
            if (object->character_context != 0) {
                processor->action_data_1 = 1;
                return 0;
            }
            GameObjectSetCanUse(object, grapple, 1, 50, 0.0f);
            AIMoveInstruction(packet, &grapple->ground_position, 0.0f, &target->path, 7,
                              packet->movement_instruction_parameter);
            if (processor->action_data_1 != 0)
                return 0;
        }
        if (NuVecXZDistSqr(&packet->terrain_origin, &target->position, &offset) < ai_moveradius * ai_moveradius) {
            processor->action_data_1 = 1;
            GameObjectSetCanUse(object, grapple, 1, 50, 0.0f);
            object->pad_gamepad->buttons_down_08 |= GAMEPAD_JUMP;
        }
    }
    return 0;
change_character:
    if (FreePlay != 0) {
        processor->action_timer -= elapsed;
        if (processor->action_timer < 0.0f) {
            processor->action_timer = 0.5f;
            object->pad_gamepad->buttons_down_08 |= GAMEPAD_TOGGLERIGHT;
        }
    }
    return 0;
}

void Action_MushroomCollapse(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

i32 Action_GetLocatorFromSet(AISYS_s *sys, AISCRIPTPROCESS_s *processor, AIPACKET_s *packet, char **params,
                             i32 param_count, i32 first_time, float) {
    if (first_time == 0) {
        return 1;
    }

    GameObject_s *object = packet != NULL && packet->owner != NULL ? packet->owner->apiobj.objptr : NULL;
    AILOCATORSET *locator_set = processor->unknown_a8;
    i32 ignore_assigned = -1;
    i32 next = 0;
    char *set_name = NULL;
    i32 use_player = 0;
    i32 use_opponent = 0;
    i32 use_second_player = 0;
    i32 first = 0;
    i32 looping = 0;
    i32 finish_at_end = 0;
    i32 random = 0;
    i32 furthest = 0;
    f32 max_range = 0.0f;
    f32 off_screen_radius = 0.0f;

    for (i32 index = 0; index < param_count; ++index) {
        char *value = NuStrIStr(params[index], "character=");
        if (value != NULL) {
            object = GetNamedGameObject(sys, value + sizeof("character=") - 1);
            continue;
        }
        value = NuStrIStr(params[index], "max_range=");
        if (value != NULL) {
            max_range = AIParamToFloat(processor, value + sizeof("max_range=") - 1);
            continue;
        }
        value = NuStrIStr(params[index], "max_player_range=");
        if (value != NULL) {
            max_range = AIParamToFloat(processor, value + sizeof("max_player_range=") - 1);
            use_player = true;
            continue;
        }
        value = NuStrIStr(params[index], "max_opponent_range=");
        if (value != NULL) {
            max_range = AIParamToFloat(processor, value + sizeof("max_opponent_range=") - 1);
            use_opponent = true;
            continue;
        }
        value = NuStrIStr(params[index], "off_screen_radius=");
        if (value != NULL) {
            off_screen_radius = AIParamToFloat(processor, value + sizeof("off_screen_radius=") - 1);
            continue;
        }
        value = NuStrIStr(params[index], "name=");
        if (value != NULL) {
            set_name = value + sizeof("name=") - 1;
            continue;
        }

        if (NuStrICmp(params[index], "random") == 0) {
            random = true;
        } else if (NuStrICmp(params[index], "next") == 0) {
            next = true;
        } else if (NuStrICmp(params[index], "first") == 0) {
            first = true;
        } else if (NuStrICmp(params[index], "looping") == 0) {
            looping = true;
        } else if (NuStrICmp(params[index], "finish_at_end") == 0) {
            finish_at_end = true;
        } else if (NuStrICmp(params[index], "ignore_assigned=TRUE") == 0) {
            ignore_assigned = 1;
        } else if (NuStrICmp(params[index], "ignore_assigned=FALSE") == 0) {
            ignore_assigned = 0;
        } else if (NuStrICmp(params[index], "furthest_from_opponent") == 0) {
            furthest = true;
            use_opponent = true;
        } else if (NuStrICmp(params[index], "furthest_from_either_player") == 0) {
            furthest = true;
            use_player = true;
            use_second_player = true;
        } else if (NuStrICmp(params[index], "nearest_either_player") == 0) {
            use_player = true;
            use_second_player = true;
        }
    }

    if (ignore_assigned == -1) {
        ignore_assigned = next ^ 1;
    }
    if (object == NULL) {
        return 1;
    }
    if (set_name != NULL) {
        locator_set = AIPathFindLocatorSet(WORLD->ai_sys, set_name);
    }
    if (locator_set == NULL) {
        return 1;
    }

    APIOBJECT *target = &object->apiobj;
    NUVEC *reference_position;
    if (use_player) {
        reference_position = &player->apiobj.position;
    } else if (use_opponent && object->ai.opponent != NULL) {
        reference_position = &static_cast<APIOBJECT *>(object->ai.opponent)->position;
    } else {
        reference_position = &target->position;
    }
    NUVEC *second_position =
        use_second_player && player2 != NULL ? &player2->apiobj.position : static_cast<NUVEC *>(NULL);

    if (first) {
        if (locator_set->locator_count != 0) {
            object->apiobj.ai->locator = &sys->locators[locator_set->locator_entries[0]];
            locator_set->assigned[0] = target->field_0x289;
        }
        return 1;
    }

    if (next) {
        if (locator_set->locator_count < 2) {
            return 1;
        }
        if (ignore_assigned != 0) {
            AILocatorSet_CheckLocatorsStillAssigned(sys, locator_set);
        }

        i32 direction = (target->field_0x1fa & 0x10) == 0 ? 1 : -1;
        i32 current_index = 0;
        AILOCATOR *current_locator = object->apiobj.ai->locator;
        if (current_locator != NULL) {
            for (current_index = 0; current_index < locator_set->locator_count; ++current_index) {
                if (current_locator == &sys->locators[locator_set->locator_entries[current_index]]) {
                    locator_set->assigned[current_index] = 0xff;
                    break;
                }
            }
        }
        if (current_locator == NULL || current_index == locator_set->locator_count) {
            current_index = qrand() / (0xffff / locator_set->locator_count + 1);
        }

        i32 candidate = current_index;
        i32 passed_start = 0;
        // A bouncing traversal can cross its starting slot before visiting
        // the other end. Keep that slot as the fallback after both legs.
        while (true) {
            candidate += direction;
            if (looping) {
                if (candidate >= locator_set->locator_count) {
                    if (finish_at_end) {
                        object->apiobj.ai->locator = NULL;
                        return 1;
                    }
                    candidate = 0;
                } else if (candidate < 0) {
                    candidate = locator_set->locator_count - 1;
                }
            } else {
                if (candidate >= locator_set->locator_count) {
                    if (finish_at_end) {
                        object->apiobj.ai->locator = NULL;
                        return 1;
                    }
                    target->field_0x1fa |= 0x10;
                    direction = -1;
                    candidate = locator_set->locator_count - 2;
                } else if (candidate < 0) {
                    target->field_0x1fa &= static_cast<u8>(~0x10);
                    direction = 1;
                    candidate = 1;
                }
            }
            if (candidate == current_index) {
                if (looping || passed_start) {
                    break;
                }
                passed_start = 1;
            } else if (ignore_assigned == 0 || locator_set->assigned[candidate] == 0xff) {
                break;
            }
        }
        object->apiobj.ai->locator = &sys->locators[locator_set->locator_entries[candidate]];
        locator_set->assigned[candidate] = target->field_0x289;
    } else if (random) {
        AILocatorSet_AssignRandomLocator(sys, locator_set, target, max_range, reference_position, off_screen_radius,
                                         ignore_assigned);
    } else if (furthest) {
        AILocatorSet_AssignFurthestLocator(sys, locator_set, target, max_range, reference_position, second_position,
                                           off_screen_radius, ignore_assigned);
    } else {
        AILocatorSet_AssignNearestLocator(sys, locator_set, target, max_range, reference_position, second_position,
                                          off_screen_radius, ignore_assigned);
    }
    return 1;
}

i32 Action_AssignLocatorInSet(AISYS_s *sys, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char **params, i32 param_count,
                              i32 first_time, float) {
    if (first_time == 0) {
        return 1;
    }

    GameObject_s *object = packet != NULL && packet->owner != NULL ? packet->owner->apiobj.objptr : NULL;
    AILOCATOR *locator = NULL;
    AILOCATORSET *locator_set = NULL;
    u8 assignment = object != NULL ? object->apiobj.field_0x289 : 0xff;

    for (i32 index = 0; index < param_count; ++index) {
        char *param = params[index];
        if (NuStrICmp(param, "locator=mylocator") == 0) {
            locator = object != NULL ? object->ai.locator : NULL;
            continue;
        }
        char *value = NuStrIStr(param, "locator");
        if (value != NULL) {
            locator = AIPathFindLocator(sys, value + NuStrLen("locator") + 1);
            continue;
        }
        value = NuStrIStr(param, "character");
        if (value != NULL) {
            object = GetNamedGameObject(sys, value + NuStrLen("character") + 1);
            assignment = object != NULL ? object->apiobj.field_0x289 : 0xff;
            continue;
        }
        value = NuStrIStr(param, "set");
        if (value != NULL) {
            locator_set = AIPathFindLocatorSet(sys, value + NuStrLen("set") + 1);
            continue;
        }
        if (NuStrICmp(param, "reserve") == 0) {
            assignment = 0x80;
        }
    }

    if (locator == NULL || locator_set == NULL) {
        return 1;
    }
    const i32 locator_index = locator - sys->locators;
    for (i32 index = 0; index < locator_set->locator_count; ++index) {
        if (locator_set->locator_entries[index] == locator_index) {
            locator_set->assigned[index] = assignment;
            break;
        }
    }
    return 1;
}

void Action_SpeederBeingChased(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char **, i32, i32, float) {
}

namespace {
    struct GizmosAIRegistryCallbacks {
        GizmosAIRegistryCallbacks() {
            lego_aiactiondefs[LEGO_AI_ACTION_RELEASE_LOCATOR].eval_fn = Action_ReleaseLocator;
            lego_aiactiondefs[LEGO_AI_ACTION_ASSIGN_LOCATOR].eval_fn = Action_AssignLocatorInSet;
            lego_aiactiondefs[LEGO_AI_ACTION_GET_LOCATOR_FROM_SET].eval_fn = Action_GetLocatorFromSet;
        }
    };

    GizmosAIRegistryCallbacks gizmos_ai_registry_callbacks;
} // namespace

// Static GIZFLOW/FLOWBOX action callbacks (GizAction*/GizActions*). Moved from
// gizactions_stubs.cpp to satisfy the symbol baseline.

static __used__ void GizAction_SetAIState(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_HitBlowup(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_PlayForce(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_PlayRadio(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_EnableSock(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

void ReleaseTakeOver(GameObject_s *, i32);

static __used__ void GizAction_ActivateChar(GIZFLOW_s *flow, FLOWBOX_s *, char **params, int count) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    char *name = NULL;
    i32 activate = 1;
    GIZBUILDIT_s *buildit = NULL;
    for (i32 i = 0; i < count; ++i) {
        char *value = NuStrIStr(params[i], "name=");
        if (value != NULL) {
            name = value + 5;
        } else if (NuStrICmp(params[i], "TRUE") == 0) {
            activate = 1;
        } else if (NuStrICmp(params[i], "FALSE") == 0) {
            activate = 0;
        } else if ((value = NuStrIStr(params[i], "buildit=")) != NULL) {
            GIZMO *gizmo = GizmoFindByName(flow->gizmo_sys, gizbuildit_gizmotype_id, value + 8);
            if (gizmo != NULL && gizmo->object != NULL) {
                buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
            }
        }
    }
    if (name != NULL && activate != 0) {
        if (buildit != NULL) {
            if (ActivateCharacter(name, &buildit->position, buildit->field_0x7c) == NULL) {
                GizBuildIt_KillParts(buildit);
            }
        } else {
            ActivateCharacter(name, NULL, 0);
        }
    } else if (activate == 0 && name != NULL) {
        if (Player[0] != NULL && (Player[0]->apiobj.field_0x1f8 & 0x1000) != 0 && Player[0]->apiobj.field_0x287 == 0 &&
            Player[0]->ai.field_0x134 != 0xff) {
            AICREATURE *creature = &world->ai_sys->creatures[Player[0]->ai.field_0x134];
            if (creature != NULL && NuStrICmp(creature->name, name) == 0)
                ReleaseTakeOver(Player[0], 0);
        }
        if (Player[1] != NULL && (Player[1]->apiobj.field_0x1f8 & 0x1000) != 0 && Player[1]->apiobj.field_0x287 == 0 &&
            Player[1]->ai.field_0x134 != 0xff) {
            // The original second-player branch also reads player zero's creature index.
            AICREATURE *creature = &world->ai_sys->creatures[Player[0]->ai.field_0x134];
            if (creature != NULL && NuStrICmp(creature->name, name) == 0)
                ReleaseTakeOver(Player[1], 0);
        }
        DeactivateCharacter(name);
    }
}

static __used__ void GizAction_SetAIMessage(GIZFLOW_s *, FLOWBOX_s *, char **params, int count) {
    f32 value = 0.0f;
    i32 mode = 0;
    char *name = NULL;
    for (i32 index = 0; index < count; ++index) {
        char *argument = NuStrIStr(params[index], "Name");
        if (argument != NULL)
            name = argument + NuStrLen("Name") + 1;
        else if ((argument = NuStrIStr(params[index], "Val")) != NULL)
            value = NuAToF(argument + NuStrLen("Val") + 1);
        else if ((argument = NuStrIStr(params[index], "increment=")) != NULL) {
            value = NuAToF(argument + 10);
            mode = 1;
        } else if ((argument = NuStrIStr(params[index], "decrement=")) != NULL) {
            value = NuAToF(argument + 10);
            mode = -1;
        }
    }
    GIZAIMESSAGE_s *message = CheckGizAIMessage(gizaimessagesys, name, NULL);
    switch (mode) {
        case 0:
            message->value = value;
            break;
        case 1:
            message->value = value + message->value;
            break;
        case -1:
            message->value = message->value - value;
            break;
    }
}

static __used__ void GizActions_PlaySpecial(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizAction_ActivateGizmo(GIZFLOW_s *flow, FLOWBOX_s *, char **params, int count) {
    char *name = NULL;
    i32 type = -1;
    i32 active = 1;
    for (i32 index = 0; index < count; ++index) {
        char *value = NuStrIStr(params[index], "name=");
        if (value != NULL)
            name = value + 5;
        else if ((value = NuStrIStr(params[index], "type=")) != NULL)
            type = GizmoGetTypeIDByName(flow->gizmo_sys, value + 5);
        else if (NuStrIStr(params[index], "FALSE") != NULL)
            active = 0;
    }
    if (name != NULL) {
        GIZMO *gizmo = GizmoFindByName(flow->gizmo_sys, type, name);
        if (gizmo != NULL)
            GizmoActivate(flow->gizmo_sys, gizmo, active, 1);
    }
}

static __used__ void GizAction_SetVisibility(GIZFLOW_s *, FLOWBOX_s *, char **params, int count) {
    nuhspecial_s special = {};
    i32 visible = 1;
    for (i32 index = 0; index < count; ++index) {
        char *value = NuStrIStr(params[index], "name=");
        if (value != NULL)
            NuSpecialFind(WORLD->current_gscn, &special, value + 5, 1);
        else if (NuStrIStr(params[index], "FALSE") != NULL)
            visible = 0;
    }
    if (NuSpecialExistsFn(&special))
        NuSpecialSetVisibility(&special, visible);
}

static __used__ void GizAction_TurnOnFlowBox(GIZFLOW_s *flow, FLOWBOX_s *, char **params, int count) {
    char *name = NULL;
    i32 enabled = 1;
    for (i32 index = 0; index < count; ++index) {
        char *value = NuStrIStr(params[index], "name=");
        if (value != NULL)
            name = value + 5;
        else if (NuStrICmp(params[index], "FALSE") == 0)
            enabled = 0;
    }
    if (name != NULL && flow != NULL) {
        for (i32 index = 0; index < flow->flowbox_count; ++index) {
            if (flow->flowboxes[index].name != NULL && NuStrICmp(flow->flowboxes[index].name, name) == 0)
                flow->flowboxes[index].state_flags_low = (flow->flowboxes[index].state_flags_low & ~1) | (enabled & 1);
        }
    }
}

static __used__ void GizActions_ActivateBelt(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_GoToNewLevel(GIZFLOW_s *, FLOWBOX_s *, char **params, int param_count) {
    if (param_count <= 0) {
        return;
    }

    char *cutscene_name = NULL;
    LEVELDATA *level = NULL;

    for (i32 param_index = 0; param_index < param_count; ++param_index) {
        char *value = NuStrIStr(params[param_index], "level=");
        if (value != NULL) {
            level = Level_FindByName(value + NuStrLen("level="), NULL);
        } else {
            value = NuStrIStr(params[param_index], "cutscene=");
            if (value != NULL) {
                cutscene_name = value + NuStrLen("cutscene=");
            }
        }
    }

    if (FreePlay == 0 && cutscene_name != NULL && NewCutScene(NULL, WORLD->cutscene_sys, cutscene_name, 0) != NULL) {
        return;
    }

    if (level != NULL && netclient == 0) {
        GoToNewLevel(level->idx);
    }
}

static __used__ void GizActions_PlayCutscene(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_PlayObstacle(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizAction_ActivateEffect(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_CompleteLevel(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_GoThroughDoor(GIZFLOW_s *, FLOWBOX_s *, char **params, int param_count) {
    char *door_name = NULL;

    for (i32 param_index = 0; param_index < param_count; ++param_index) {
        char *value = NuStrIStr(params[param_index], "Name");
        if (value != NULL) {
            door_name = value + NuStrLen("Name=");
        }
    }

    if (door_name != NULL) {
        DOOR_s *door = Door_FindByName(WORLD, door_name);
        if (door != NULL) {
            Door_GoThrough(WORLD, door, 1);
        }
    }
}

static __used__ void GizAction_ChangeTechnoTgt(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizAction_ActivatePartEffect(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizAction_SetGizmoVisibility(GIZFLOW_s *flow, FLOWBOX_s *, char **params, int count) {
    char *name = NULL;
    i32 type = -1;
    i32 visible = 1;
    for (i32 index = 0; index < count; ++index) {
        char *value = NuStrIStr(params[index], "name=");
        if (value != NULL) {
            name = value + 5;
        } else if ((value = NuStrIStr(params[index], "type=")) != NULL) {
            type = GizmoGetTypeIDByName(flow->gizmo_sys, value + 5);
        } else if (NuStrIStr(params[index], "FALSE") != NULL) {
            visible = 0;
        }
    }
    if (name != NULL) {
        GIZMO *gizmo = GizmoFindByName(flow->gizmo_sys, type, name);
        if (gizmo != NULL)
            GizmoSetVisibility(flow->gizmo_sys, gizmo, visible, 1);
    }
}

static __used__ void GizAction_SetPickupVisibility(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

static __used__ void GizActions_ChangeObstTriggerType(GIZFLOW_s *, FLOWBOX_s *, char **, int) {
}

GIZACTIONDEFN_s game_gizactiondefs[] = {
    {"SetVisibility", GizAction_SetVisibility},
    {"SetGizmoVisibility", GizAction_SetGizmoVisibility},
    {"SetPickupVisibility", GizAction_SetPickupVisibility},
    {"ActivateGizmo", GizAction_ActivateGizmo},
    {"ActivateChar", GizAction_ActivateChar},
    {"TurnOnFlowBox", GizAction_TurnOnFlowBox},
    {"ActivateEffect", GizAction_ActivateEffect},
    {"ActivatePartEffect", GizAction_ActivatePartEffect},
    {"ChangeTechnoTarget", GizAction_ChangeTechnoTgt},
    {"SetAIMessage", GizAction_SetAIMessage},
    {"SetAIState", GizAction_SetAIState},
    {"CompleteLevel", GizActions_CompleteLevel},
    {"GoToNewLevel", GizActions_GoToNewLevel},
    {"GoThroughDoor", GizActions_GoThroughDoor},
    {"PlayObstacle", GizActions_PlayObstacle},
    {"PlaySpecial", GizActions_PlaySpecial},
    {"PlayForce", GizActions_PlayForce},
    {"ChangeObstTriggerType", GizActions_ChangeObstTriggerType},
    {"PlayRadio", GizActions_PlayRadio},
    {"PlayCutscene", GizActions_PlayCutscene},
    {"HitBlowup", GizActions_HitBlowup},
    {"ActivateBelt", GizActions_ActivateBelt},
    {"EnableSock", GizActions_EnableSock},
    {NULL, NULL},
};
