#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/gizmo/base/TeleportObjectInterface.h"
#include "nu2api/nu3d/nuspline.h"
#include "legoapi/gizmos/object/gizpanel.h"
#include "MechInputTouch_types.h"
#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/gizmo/base/GizForceObjectInterface.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/nuvec.h"

extern i16 id_RANCOR, id_ANAKINJEDI;
i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);
void CalcAveragePosAndRad(GIZBUILDIT_s &, VuVec &, f32 &, bool);
bool CalculateRayBoxIntersection(VuVec const &, VuVec const &, VuVec const &, VuVec const &, f32, f32 &);
extern "C" void NewRayCastGetImpactNormal(NUVEC *);
f32 CalcCapsuleIntersectDistance(VuVec const &, VuVec const &, f32, VuVec const &, f32);

i32 MechInputTouchSystem::s_baseControlMode = 1;
i32 MechInputTouchSystem::s_actualTouchMode = 2;

char const *MechInputTouchSystem::GetName() {
    return "MechInputTouchSystem";
}

void MechAutoJumpGetBest(JumpTriggerPacket const &, i32) {
}

void MechAutoJumpSetIsUsing(GameObject_s &, MechAutoJumpConnection &) {
}

void MechTouchUITagButton_OnClick_Callback(MechTouchUIElement &, TouchHolder &) {
}

void MechTouchUIPauseButton_OnClick_Callback(MechTouchUIElement &, TouchHolder &) {
}

void MechTouchUIPartySelector_OnRelease_Callback(MechTouchUIElement &, TouchHolder &) {
}

void MechInputTouchSystem::AddChangeLayoutButtons(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::ChooseTouchLayout(bool) {
}

void MechInputTouchSystem::ConvertToScreenCoords(float, float, float &, float &) {
}

void MechInputTouchSystem::CouldTouchBeLockedBy(u32, MechInputTouchButton *) {
}

void MechInputTouchSystem::CreateGamePanels() {
}

void MechInputTouchSystem::CreateGamePlayLayoutBlank(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::CreateGamePlayLayoutConsoleMode(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::CreateGamePlayLayoutGestureBased(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::CreateGamePlayLayoutGestureBased_Cavalry(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::CreateGamePlayLayoutGestureBased_DeathStarTurret(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::CreateGamePlayLayoutGestureBased_Podrace(NuVirtualTouchDevice &, i32) {
}

void MechInputTouchSystem::CreateGamePlayLayoutGestureBased_SpeederChase(NuVirtualTouchDevice &, i32) {
}

f32 MechInputTouchSystem::DetermineMoveDir2D(GameObject_s &object, VuVec const &target, bool flatten,
                                             VuVec &direction) {
    VuVec object_position(object.apiobj.collision_position.x, object.apiobj.collision_position.y,
                          object.apiobj.collision_position.z, 1.0f);
    VuVec object_screen;
    NuCameraTransformScreenClip(&object_screen.xyz, &object_position.xyz, 1, NULL);

    VuVec target_position = target;
    if (flatten) {
        target_position.y = object_position.y;
    }

    VuVec target_screen;
    NuCameraTransformScreenClip(&target_screen.xyz, &target_position.xyz, 1, NULL);

    direction = VuVec(target_screen.x - object_screen.x, target_screen.y - object_screen.y, 0.0f, 0.0f);

    f32 magnitude = NuVecMag(&direction.xyz);
    if (magnitude > 0.001f) {
        f32 scale = 1.0f / magnitude;
        direction.x *= scale;
        direction.y *= scale;
        direction.z *= scale;
    } else {
        direction = VuVec(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return magnitude;
}

void MechInputTouchSystem::FindTargetForce(WORLDINFO_s *world, GameObject_s &object, VuVec const &start,
                                           VuVec const &direction, f32 &best_distance,
                                           MechObjectInterface *&best_target, bool &found, bool keep_current_target) {
    GIZFORCESYS_s *force_system = world->giz_force_sys;
    if (force_system == NULL || force_system->visible_force_count == 0) {
        return;
    }

    for (i32 index = 0; index < force_system->visible_force_count; ++index) {
        GIZFORCE_s *force = force_system->visible_forces[index];
        if (force == NULL || !TouchHacks::CanUseGizForce(object, *force)) {
            continue;
        }

        if ((force->config_flags & GIZFORCE_CONFIG_TARGET_ANIMATION_OBJECTS) != 0) {
            for (GAMEANIMOBJ_s *animation = force->anim_set->objects; animation != NULL; animation = animation->next) {
                if ((animation->flags & 1) != 0 || (object.force_glow_object != animation && keep_current_target)) {
                    continue;
                }

                NUVEC *draw_position = NuSpecialGetDrawPos(&animation->special);
                if (draw_position == NULL) {
                    continue;
                }

                f32 radius = animation->current_frame;
                if (radius <= 0.0f) {
                    radius = NuSpecialGetOriginRadius(&animation->special);
                }
                VuVec target_position(draw_position->x, draw_position->y, draw_position->z, 1.0f);
                f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, target_position, radius);
                if (best_distance > distance) {
                    best_distance = distance;
                    static_cast<GizForceObjectInterface *>(force->GetMechObjectInterface())->selected_object =
                        animation;
                    best_target = force->GetMechObjectInterface();
                    found = true;
                }
            }
            continue;
        }

        if (object.force_glow_object != force && keep_current_target) {
            continue;
        }

        f32 radius = force->strength_0x6c > 0.0f ? force->strength_0x6c : force->radius;
        radius = MAX(0.2f, radius);
        VuVec target_position(force->position.x, force->position.y, force->position.z, 1.0f);
        f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, target_position, radius);
        if (best_distance > distance) {
            best_distance = distance;
            static_cast<GizForceObjectInterface *>(force->GetMechObjectInterface())->selected_object = NULL;
            best_target = force->GetMechObjectInterface();
            found = true;
        }
    }
}

MechObjectInterface *MechInputTouchSystem::FindTargetObject(GameObject_s &object, VuVec const &touch, i32 flags,
                                                            MechObjectInterface *required_target,
                                                            MechTempPosInterface *temporary) {
    MechObjectInterface *best = NULL;
    f32 best_distance = VehicleArea ? 5000.0f : 25.0f;
    VuVec start, end, direction;
    NuCameraCalcRay((0.0f + touch.x + 1.0f) * 0.5f, 1.0f - (0.0f + touch.y + 1.0f) * 0.5f, &start.xyz, &end.xyz, NULL);
    direction.x = end.x - start.x;
    direction.y = end.y - start.y;
    direction.z = end.z - start.z;
    NuVecNorm(&direction.xyz, &direction.xyz);
    WORLDINFO_s *world = WORLD;
    if (world != NULL) {
        if ((flags & 0x10) && TouchHacks::CanUseGizForce(object)) {
            bool found = false;
            PART_s *part = static_cast<PART_s *>(object.force_glow_object);
            if (part != NULL && object.force_glow_kind == 3) {
                if (!(flags & 0x2000) || (part->flags & 0x8000)) {
                    f32 radius = part->radius;
                    if (part->flags & 0x8000)
                        radius *= 6.0f;
                    VuVec center(part->position.x, part->position.y, part->position.z, 1.0f);
                    f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, radius);
                    if (best_distance > distance) {
                        best_distance = distance;
                        best = part->GetMechObjectInterface();
                        found = true;
                    }
                }
            } else if (!(flags & 0x2000)) {
                FindTargetForce(world, object, start, direction, best_distance, best, found, true);
            }
            if (found)
                flags = 0;
            else if (!(flags & 0x2000))
                FindTargetForce(world, object, start, direction, best_distance, best, found, false);
        }
        if ((flags & 4) && world->gizmo_blowups != NULL) {
            GIZMOBLOWUP_s *item = world->gizmo_blowups;
            for (i32 index = 0; index < world->gizmo_blowup_count; ++index, ++item) {
                nuhspecial_s *special = item->override_special;
                if (special == NULL || !NuSpecialExistsFn(special))
                    special = &item->type->special;
                if (item->field_0x125[0] != 0 || (i8)item->state_flags >= 0 || item->type == NULL ||
                    !NuSpecialExistsFn(special))
                    continue;
                bool sphere = item->field_0x124 != 0 || (world->current_level == MOSEISLEYB_LDATA && index <= 31) ||
                              (world->current_level == VADERB_LDATA && index <= 1) ||
                              (world->current_level == CLOUDCITYESCAPEA_LDATA && index == 0) || VehicleArea;
                f32 distance;
                bool hit;
                if (sphere) {
                    NUVEC *position = item->field_0x120 ? static_cast<NUVEC *>(item->field_0x120) : &item->mid_position;
                    VuVec center(position->x, position->y, position->z, 1.0f);
                    f32 radius = item->field_0x128 > 0.0f ? item->field_0x128 : item->target_scale;
                    if (VehicleArea)
                        radius *= 3.0f;
                    distance =
                        CalcCapsuleIntersectDistance(start, direction, best_distance + best_distance, center, radius);
                    hit = distance != 1000000000.0f;
                } else {
                    // bounds preserve the original special's fourth lanes.
                    VuVec minimum, maximum;
                    minimum.x = special->display_special->bounds_min.x + item->mid_position.x;
                    minimum.y = special->display_special->bounds_min.y + item->mid_position.y;
                    minimum.z = special->display_special->bounds_min.z + item->mid_position.z;
                    minimum.w = special->display_special->bounds_min.w;
                    maximum.x = special->display_special->bounds_max.x + item->mid_position.x;
                    maximum.y = special->display_special->bounds_max.y + item->mid_position.y;
                    maximum.z = special->display_special->bounds_max.z + item->mid_position.z;
                    maximum.w = special->display_special->bounds_max.w;
                    hit = CalculateRayBoxIntersection(minimum, maximum, start, direction, 30.0f, distance);
                }
                if (hit && best_distance + best_distance > distance) {
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
            }
        }
        if ((flags & 0x20) && TouchHacks::CanUseBuildIt(object)) {
            GIZBUILDITSYS_s *system = world->giz_buildit_sys;
            if (system != NULL && system->buildits != NULL) {
                GIZBUILDIT_s *item = system->buildits;
                for (i32 index = 0; index < system->count; ++index, ++item) {
                    if (!(item->availability_flags & 1) || item->build_state == 2 || !(item->availability_flags & 2))
                        continue;
                    VuVec center;
                    f32 radius;
                    CalcAveragePosAndRad(*item, center, radius, true);
                    radius = radius > 0.2f ? radius : 0.2f;
                    f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, radius);
                    if (best_distance > distance) {
                        best_distance = distance;
                        best = item->GetMechObjectInterface();
                    }
                }
            }
        }
        if ((flags & 8) && TouchHacks::CanUseLever(object)) {
            LEVER_s *item = world->levers;
            for (i32 index = 0; index < world->nlevers; ++index, ++item) {
                if (item == NULL || (item->flags & 0x93) != 0x90 || item->pull_progress != 0.0f)
                    continue;
                VuVec center(item->position.x, item->position.y, item->position.z, 1.0f);
                f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, 0.2f);
                if (best_distance > distance) {
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
            }
        }
        if ((flags & 0x100) && world->hat_machine_sys != NULL && TouchHacks::CanUseHatMachine(object)) {
            HATMACHINE_s *item = world->hat_machine_sys->machines;
            for (i32 index = 0; index < world->hat_machine_sys->count; ++index, ++item) {
                if (item == NULL || (item->flags & 15) != 12)
                    continue;
                VuVec center(item->position.x, item->position.y, item->position.z, 1.0f);
                f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, 0.2f);
                if (best_distance > distance) {
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
            }
        }
        if ((flags & 0x40) && TouchHacks::CanUseTeleport(object)) {
            TELEPORT_s *item = world->teleports;
            for (i32 index = 0; index < world->teleport_count; ++index, ++item) {
                if (item == NULL || item->enabled == 0 || item->active != 0)
                    continue;
                NUVEC *point = &item->path->pts[0];
                VuVec center(point->x, point->y + object.apiobj.scaled_height * 0.5f, point->z, 1.0f);
                f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, 0.4f);
                if (best_distance > distance) {
                    static_cast<TeleportObjectInterface *>(item->GetMechObjectInterface())->index = 0;
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
                if (!(item->flags & 1)) {
                    point = &item->path->pts[item->path->length - 1];
                    center = VuVec(point->x, point->y + object.apiobj.scaled_height * 0.5f, point->z, 1.0f);
                    distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, 0.4f);
                    if (best_distance > distance) {
                        static_cast<TeleportObjectInterface *>(item->GetMechObjectInterface())->index = 1;
                        best_distance = distance;
                        best = item->GetMechObjectInterface();
                    }
                }
            }
        }
        if ((flags & 0x200) && world->giz_panel_sys != NULL && world->giz_panel_sys->panels != NULL) {
            GIZPANELSYS_s *system = world->giz_panel_sys;
            GIZPANEL_s *item = system->panels;
            for (i32 index = 0; index < system->count; ++index, ++item) {
                if ((item->flags & 11) != 8 || !GizPanel_CanUsePanel(&object, item))
                    continue;
                VuVec center(item->position.x, item->position.y, item->position.z, 1.0f);
                f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, 0.2f);
                if (best_distance > distance) {
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
            }
        }
        if ((flags & 0x400) && world->giz_turret_sys != NULL) {
            GIZTURRETSYS_s *system = world->giz_turret_sys;
            GIZTURRET_s *item = system->turrets;
            for (i32 index = 0; index < system->count; ++index, ++item) {
                if (item == NULL || (item->flags & 0x22) != 2)
                    continue;
                VuVec center(item->field_0x3c.x, item->field_0x3c.y, item->field_0x3c.z, 1.0f);
                f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, item->field_0x140);
                if (best_distance > distance) {
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
            }
        }
    }
    if (flags & 1) {
        GameObject_s *candidate = Obj;
        GameObject_s *selected = NULL;
        bool selected_nonplayer = false;
        f32 selected_distance = 1000000000.0f;
        f32 weighted_distance = 1000000000.0f;
        for (i32 index = 0; index < HIGHGAMEOBJECT; ++index, ++candidate) {
            if (candidate->field_0xcc0 == &object || (candidate->apiobj.object_flags & 0x1001) != 0x1001 ||
                ((flags & 0x4000) && candidate == &object) || candidate->apiobj.field_0x287 != 0 ||
                (CInfo[(i8)candidate->character_context].flags & 0x8000))
                continue;
            if (WORLD->current_level == HOTHESCAPEA_LDATA &&
                (candidate->apiobj.character_data->game_character->flags_090 & 0x40) &&
                !TouchHacks::CanTagVehicle(object, *candidate))
                continue;
            if (candidate->id == id_RANCOR && best != NULL)
                continue;
            bool large = (VehicleArea && best == NULL) ||
                         ((candidate->field_0xefb & 8) && candidate->id != id_RANCOR) ||
                         WORLD->current_level == MAULA_LDATA;
            if (WORLD->current_level == VADERC_LDATA && candidate->id == id_ANAKINJEDI &&
                !(candidate->apiobj.object_flags & 0x80) && vader_c.final_fight_message->value > 0.0f)
                large = true;
            VuVec center(candidate->apiobj.collision_position.x, candidate->apiobj.collision_position.y,
                         candidate->apiobj.collision_position.z, 1.0f);
            if (!large)
                center.y = candidate->apiobj.scaled_height * 0.25f + center.y;
            f32 radius =
                (large ? 2.5f : ((candidate->apiobj.object_flags & 0x80) ? 1.0f : 1.3f)) * candidate->field_0x1008;
            if (WORLD->current_level == DEATHSTAR2BATTLEB_LDATA && (candidate->apiobj.object_flags & 0x80))
                radius *= 0.8f;
            f32 distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, radius);
            if (!large && !(candidate->apiobj.character_data->model_flags & 0x2000) && distance == 1000000000.0f) {
                center.y -= candidate->apiobj.scaled_height * 0.5f;
                distance = CalcCapsuleIntersectDistance(start, direction, best_distance, center, radius);
            }
            bool nonplayer = !(candidate->apiobj.field_0x1f4 & 1);
            f32 weighted = selected != NULL && selected_nonplayer && !nonplayer ? distance * 0.5f : distance;
            if (weighted_distance > weighted) {
                if ((flags & 0x1000) && !selected_nonplayer && selected != NULL && nonplayer) {
                    selected_nonplayer = false;
                } else {
                    weighted_distance = weighted;
                    selected_distance = distance;
                    selected = candidate;
                    selected_nonplayer = nonplayer;
                }
            }
        }
        if (selected != NULL && !(selected_nonplayer && best != NULL && (flags & 0x1000))) {
            if (flags & 0x800)
                selected_distance *= 0.25f;
            if (best_distance > selected_distance) {
                best = selected->GetMechObjectInterface();
                best_distance = selected_distance;
            }
        }
    }
    if (flags & 2) {
        GIZOBSTACLESYS_s *system = WORLD->giz_obstacle_sys;
        if (system != NULL && system->obstacles != NULL) {
            GIZOBSTACLE_s *item = system->obstacles;
            for (i32 index = 0; index < system->count; ++index, ++item) {
                if (item == NULL || (item->field_a1_0xa1 & 1))
                    continue;
                if (!(item->config_flags & 0x1800) && item->mode != 2)
                    continue;
                LEVELDATA_s *level = WORLD->current_level;
                if ((level == ENDORBATTLEA_LDATA && index == 3) ||
                    (level == GUNGAN_B_LDATA && (index == 0 || index == 2 || index == 3)) ||
                    (level == ENDORBATTLED_LDATA && index == 5) ||
                    (level == FACTORYD_LDATA && (index == 4 || index == 2)) ||
                    (level == DEATHSTARRESCUEB_LDATA && (index == 2 || index == 6 || index == 12)) ||
                    (level == HOTHESCAPEC_LDATA && (u32)(index - 18) <= 4) || (level == CRUISERB_LDATA && index == 0) ||
                    (level == CRUISERD_LDATA && index == 19) || (level == JEDI_B_LDATA && index == 50) ||
                    (level == TATOOINEB_LDATA && index == 37))
                    continue;
                if ((item->progress_flags & 3) != 3 || (i8)item->runtime_flags < 0 || item->anim_set == NULL ||
                    item->anim_set->state == 2 || item->proximity_output != 0 || item->mode == 1 || item->mode == 3)
                    continue;
                VuVec center(item->evaluated_position.x, item->evaluated_position.y, item->evaluated_position.z, 1.0f);
                f32 distance =
                    CalcCapsuleIntersectDistance(start, direction, best_distance, center, item->field_0x58) * 1.5f;
                if (best_distance > distance) {
                    best_distance = distance;
                    best = item->GetMechObjectInterface();
                }
            }
        }
    }
    if ((flags & 0x80) && temporary != NULL && best == NULL) {
        VuVec position = start;
        if (best_distance > 0.0f) {
            f32 remaining = best_distance;
            do {
                i32 step = static_cast<i32>(remaining);
                if (step >= 7)
                    step = 7;
                remaining -= static_cast<f32>(step);
                VuVec delta;
                delta.x = direction.x * static_cast<f32>(step);
                delta.y = direction.y * static_cast<f32>(step);
                delta.z = direction.z * static_cast<f32>(step);
                if (GameRayCast(&position.xyz, &delta.xyz, 0.0f, 0)) {
                    f32 distance = NuVecMag(&delta.xyz);
                    VuVec normal = VuVec_Zero;
                    NewRayCastGetImpactNormal(&normal.xyz);
                    if (WORLD->current_level != DEATHSTARESCAPEB_LDATA)
                        remaining = -1.0f;
                    if (best_distance > distance && normal.y > 0.75f) {
                        best_distance = distance;
                        temporary->position =
                            VuVec(position.x + delta.x, position.y + delta.y, position.z + delta.z, 0.0f);
                        best = temporary;
                        remaining = -1.0f;
                    }
                }
                position.x = delta.x * 1.1f + position.x;
                position.y = delta.y * 1.1f + position.y;
                position.z = delta.z * 1.1f + position.z;
            } while (remaining > 0.0f && best == NULL);
        }
    }
    return required_target == NULL || required_target == best ? best : NULL;
}

void MechInputTouchSystem::Init() {
}

MechInputTouchSystem::MechInputTouchSystem() {
}

void MechInputTouchSystem::ProcessEvenWhenPaused(ThingProcessData *) {
}

void MechInputTouchSystem::ResetAllOwners() {
}

void MechInputTouchSystem::SetTouchLockedBy(u32, MechInputTouchButton *, bool) {
}

void MechInputTouchSystem::TouchLockedBy(u32) {
}
