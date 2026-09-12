#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/gizmos/object/gizpanel.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/gamehint.h"
#include "legoapi/world/world_shared.h"
#include "legoapi/world/mission.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numusic/sfx.h"

extern NUVEC nusound_special_positions[5];
extern "C" void PlaySfxById(i32 sfx_id, nuvec_s *position);
extern "C" void PlaySfx(char *, nuvec_s *);
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);
void FastWeaponIn(GameObject_s *, i32);
void MakeBaddiesForgetAboutParty(i32);
void SetProtocolDroidInterfaceAction(GameObject_s *);
void GizPanel_PlaySfx(char *, nuvec_s *, i32);
void NewBuzz(nupad_s *, f32, i32);
void NewRumble(nupad_s *, f32, i32);
static __used__ u8 storm_panel_active;
static __used__ u8 droid_panel_active;
static __used__ u8 bountyhunter_panel_active;

extern "C" f32 GIZPANEL_PLAYERPOSLIFT;
extern "C" {
    i32 DeletePlatinst(i32 platform_id);
    i16 NewPlatPickupInst(void *object, i32 object_type);
    void PlatInstRotate(i32 platform_id, i32 enabled);
}

f32 GameShadow(GameObject_s *object, NUVEC *position, f32 probe_height, i32 terrain_mask);
void FindAnglesZX(NUVEC *normal, u16 *x_rotation, u16 *z_rotation);
i32 SuperWeirdo(GameObject_s *object);
extern i16 id_DARTHVADER, id_GRANDMOFFTARKIN, id_IMPERIALOFFICER, id_IMPERIALSHUTTLEPILOT;
void GizPanel_GetAbsTargetPos(GIZPANEL_s *panel, nuvec_s *target_position, i32 player_position);

void GizPanel_Use(GameObject_s &object, GIZPANEL_s &panel) {
    object.field_0x788 = &panel;
    object.field_0x768 = 0.0f;
    object.delayed_turn_timer = 0.0f;
    object.apiobj.movement_facing_angle = panel.y_rotation;
    object.field_0xe21 &= ~0x10;
    object.character_context = 0x0b;
    object.field_0x7a3 = 0;
    FastWeaponIn(&object, 0);
    object.movement_runtime_flags |= 2;
    GIZPANEL_s *active_panel = static_cast<GIZPANEL_s *>(object.field_0x788);
    if (active_panel->model_variant == 2) {
        object.context_animation = object.apiobj.character_model->model_data_b[0x46] != NULL ? 0x46 : 0x45;
        if (static_cast<i8>(object.apiobj.object_flags) < 0) {
            Hint_SetComplete(0x261);
            Hint_SetComplete(0x26a);
        }
    } else if (active_panel->model_variant == 3) {
        object.context_animation = object.apiobj.character_model->model_data_b[0x45] != NULL
                                       ? 0x45
                                       : (object.apiobj.character_model->model_data_b[0x46] != NULL ? 0x46 : 0x45);
        MakeBaddiesForgetAboutParty(1);
        if (static_cast<i8>(object.apiobj.object_flags) < 0) {
            Hint_SetComplete(0x260);
            Hint_SetComplete(0x269);
        }
    } else {
        object.context_animation = 0x18;
        if (active_panel->model_variant == 1) {
            if (!InStory() || (static_cast<GIZPANEL_s *>(object.field_0x788)->draw_flags & 4) == 0)
                GizPanel_PlaySfx("TC14_VLA", &object.apiobj.collision_position, 1 << object.apiobj.field_0x27c);
            if ((object.apiobj.character_data->model_flags & 0x20) != 0)
                SetProtocolDroidInterfaceAction(&object);
        } else {
            if (!InStory() || (static_cast<GIZPANEL_s *>(object.field_0x788)->draw_flags & 4) == 0)
                GizPanel_PlaySfx("R2D2_VLA", &object.apiobj.collision_position, 1 << object.apiobj.field_0x27c);
        }
        if (static_cast<i8>(object.apiobj.object_flags) < 0) {
            Hint_SetComplete(0x25f);
            if (static_cast<GIZPANEL_s *>(object.field_0x788)->model_variant == 0)
                Hint_SetComplete(0x625);
            else
                Hint_SetComplete(0x624);
        }
        LSW_HintConditions |= 4;
    }
    f32 duration = AnimDuration(object.id, object.context_animation, 0.0f, 0.0f, 1);
    object.field_0xdb0 = 0.0f;
    if (duration <= 0.0f)
        duration = 2.0f;
    object.context_animation_timer = duration;
}

void GizPanel_Reset(GIZPANEL_s *panel) {
    NUVEC *floor_position = &panel->floor_position;
    floor_position->y = 0.0f;
    floor_position->x = 0.0f;
    floor_position->z = 0.4f;
    NuVecRotateY(floor_position, floor_position, panel->y_rotation + 0x8000);
    NuVecAdd(floor_position, floor_position, &panel->position);

    NUVEC target_position;
    GizPanel_GetAbsTargetPos(panel, &target_position, 0);
    target_position.y = panel->position.y;
    floor_position->y = GameShadow(NULL, floor_position, 0.2f, -1);
    const f32 target_floor = GameShadow(NULL, &target_position, 0.2f, -1);
    panel->target_offset.y = target_floor;
    if (target_floor != 2000000.0f) {
        panel->target_offset.y = target_floor + GIZPANEL_PLAYERPOSLIFT;
        FindAnglesZX(&ShadNorm, &panel->target_pitch, &panel->target_roll);
    }

    panel->activation_time = 0.0f;
    panel->arm_x_rotation = 0;
    panel->flags =
        static_cast<GIZPANEL_FLAGS>((panel->flags & 0xfc) | GIZPANEL_FLAG_VISIBLE | GIZPANEL_FLAG_TRACK_PLAYER);
    NuMtxSetRotationY(&panel->matrix, panel->y_rotation);
    NuMtxTranslate(&panel->matrix, &panel->position);
}

void GizPanel_PlaySfx(char *name, nuvec_s *position, i32 player_bits) {
    if (position == NULL || name == NULL)
        return;
    const i16 sfx_id = static_cast<i16>(GetSfxId(name));
    if (sfx_id == -1)
        return;
    if (player_bits == 0) {
        PlaySfxById(sfx_id, position);
    } else {
        if ((player_bits & 1) != 0) {
            nusound_special_positions[1] = *position;
            PlaySfxById(sfx_id, &nusound_special_positions[1]);
            nusound_special_positions[1] = nusound_special_positions[0];
        }
        if ((player_bits & 2) != 0) {
            nusound_special_positions[2] = *position;
            PlaySfxById(sfx_id, &nusound_special_positions[2]);
            nusound_special_positions[2] = nusound_special_positions[0];
        }
    }
}

void GizPanel_MoveCode(WORLDINFO_s *world, GameObject_s *object, i32 use) {
    if (use != 0 && static_cast<i8>(object->apiobj.object_flags) >= 0 && world->current_level == MOSEISLEYA_LDATA)
        use = 0;
    if (object->field_0xdb0 > 0.0f)
        object->field_0xdb0 -= FRAMETIME;
    storm_panel_active = 0;
    droid_panel_active = 0;
    bountyhunter_panel_active = 0;
    if (object->character_context == 0x0b && object->field_0x788 != NULL) {
        if (object->apiobj.character_model->model_data_b[object->context_animation] == NULL ||
            AnimPlaying(&object->apiobj.anim_packet, object->context_animation, 1, 0) != NULL) {
            object->field_0x768 += FRAMETIME;
            if (object->field_0x768 > 1.0f)
                object->field_0x768 = 1.0f;
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                object->character_context = -1;
                static_cast<GIZPANEL_s *>(object->field_0x788)->flags =
                    static_cast<GIZPANEL_FLAGS>(static_cast<GIZPANEL_s *>(object->field_0x788)->flags & ~1);
                static_cast<GIZPANEL_s *>(object->field_0x788)->flags =
                    static_cast<GIZPANEL_FLAGS>(static_cast<GIZPANEL_s *>(object->field_0x788)->flags | 2);
                object->field_0x788 = NULL;
            }
            if ((object->field_0xe21 & 0x10) == 0)
                object->field_0xe21 |= 0x10;
        } else {
            object->context_animation_timer -= FRAMETIME;
            if (object->context_animation_timer <= 0.0f) {
                if (object->field_0x788 != NULL) {
                    NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                    if (static_cast<GIZPANEL_s *>(object->field_0x788)->model_variant == 2)
                        PlaySfx("Hunter_Granted", &object->apiobj.collision_position);
                    if (static_cast<GIZPANEL_s *>(object->field_0x788)->model_variant == 3)
                        PlaySfx("Trooper_Granted", &object->apiobj.collision_position);
                } else {
                    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
                }
                if (static_cast<i8>(object->apiobj.object_flags) < 0) {
                    const u8 variant = static_cast<GIZPANEL_s *>(object->field_0x788)->model_variant;
                    if (variant <= 1) {
                        if (Mission_Active(NULL) != NULL) {
                            Hint_SetComplete(0x2b9);
                        } else {
                            Hint_SetComplete(0x25f);
                            LSW_HintConditions |= 4;
                            if (static_cast<GIZPANEL_s *>(object->field_0x788)->model_variant == 0)
                                Hint_SetComplete(0x625);
                            else
                                Hint_SetComplete(0x624);
                        }
                    } else if (variant == 3) {
                        Hint_SetComplete(0x260);
                    } else if (variant == 2) {
                        Hint_SetComplete(0x261);
                    }
                }
                object->character_context = -1;
                static_cast<GIZPANEL_s *>(object->field_0x788)->flags =
                    static_cast<GIZPANEL_FLAGS>(static_cast<GIZPANEL_s *>(object->field_0x788)->flags & ~1);
                static_cast<GIZPANEL_s *>(object->field_0x788)->flags =
                    static_cast<GIZPANEL_FLAGS>(static_cast<GIZPANEL_s *>(object->field_0x788)->flags | 2);
                object->field_0x788 = NULL;
            } else if ((object->apiobj.character_data->model_flags & 0x20) != 0) {
                SetProtocolDroidInterfaceAction(object);
            }
        }
        return;
    }
    if (object->character_context != 1 && object->character_context != -1 && object->character_context != 2 &&
        !objInNetWaitContext(object, 0x0b))
        return;
    f32 distance;
    GIZPANEL_s *panel = GizPanel_FindNearest(WORLD, &object->apiobj.position, object, &distance, 1);
    if (panel != NULL) {
        const f32 radius = (0.25f + object->apiobj.field_0x1dc) * panel->target_scale;
        if (distance < radius * radius &&
            (use != 0 || (object->panel_use_request == 1 && object->big_jump_data != NULL)))
            GizPanel_Use(*object, *panel);
    } else if (use != 0 && static_cast<i8>(object->apiobj.object_flags) < 0 &&
               (object->apiobj.character_data->model_flags & 0x20) != 0 && object->field_0xdb0 <= 0.0f) {
        GizPanel_PlaySfx("TC14_VLN", &object->apiobj.collision_position, 1 << object->apiobj.field_0x27c);
        object->field_0xdb0 = 0.5f;
    }
}

i32 GizPanel_BeingUsed(GIZPANEL_s *panel) {
    return panel->flags & 1;
}

GIZPANEL_s *GizPanel_FindByName(WORLDINFO_s *world, char *name) {
    if (world != NULL && world->giz_panel_sys != NULL) {
        GIZPANEL_s *panel = world->giz_panel_sys->panels;
        for (i32 index = 0; index < world->giz_panel_sys->count; ++index, ++panel) {
            if (NuStrICmp(panel->name, name) == 0)
                return panel;
        }
    }
    return NULL;
}

i32 GizPanel_UpdateHint(HINT_s *hint) {
    bool storm = false;
    bool droid = false;
    bool bountyhunter = false;
    i32 droid_type = -1;
    for (i32 index = 0; index < 8; ++index) {
        if (Player[index] == NULL)
            continue;
        f32 distance;
        GIZPANEL_s *panel = GizPanel_FindNearest(WORLD, &Player[index]->apiobj.position, Player[index], &distance, 0);
        if (panel == NULL || static_cast<u8>(panel->flags & 0x0b) != 8)
            continue;
        f32 radius = (1.0f + Player[index]->apiobj.field_0x1dc) * panel->target_scale;
        if (distance < radius * radius) {
            switch (panel->model_variant) {
                case 0:
                case 1:
                    droid = true;
                    droid_type = panel->model_variant;
                    break;
                case 2:
                    bountyhunter = true;
                    break;
                case 3:
                    storm = true;
                    break;
            }
        }
    }
    if (storm && (hint->control_mode_ids[0] == 0x269 || hint->control_mode_ids[0] == 0x260)) {
        if (AvailableToPlayer(0, 1, 5, 0) != 0)
            return hint->control_mode_ids[0] == 0x260;
        return hint->control_mode_ids[0] == 0x269;
    }
    if (bountyhunter && (hint->control_mode_ids[0] == 0x26a || hint->control_mode_ids[0] == 0x261)) {
        if (AvailableToPlayer(0x1000000, -1, 6, 0) != 0)
            return hint->control_mode_ids[0] == 0x261;
        return hint->control_mode_ids[0] == 0x26a;
    }
    if (droid) {
        if (hint->control_mode_ids[0] == 0x624)
            return droid_type == 1 && FreePlay != 0 && AvailableToPlayer(0x30, -1, 0, 1) == 0;
        if (hint->control_mode_ids[0] == 0x625)
            return droid_type == 0 && FreePlay != 0 && AvailableToPlayer(0x50, -1, 0, 1) == 0;
        if (hint->control_mode_ids[0] == 0x25f)
            return (LSW_HintConditions & 2) != 0;
    }
    return 0;
}

i32 GizPanel_CanUsePanel(GameObject_s *object, GIZPANEL_s *panel) {
    if (panel == NULL || object == NULL)
        return 0;
    if (SuperWeirdo(object) != 0)
        return 1;
    switch (panel->model_variant) {
        case 0:
            return (object->apiobj.character_data->model_flags & 0x40) != 0 ||
                   (object->apiobj.character_data->model_flags & 0x1000010) == 0x1000010;
        case 1:
            return (object->apiobj.character_data->model_flags & 0x20) != 0 ||
                   (object->apiobj.character_data->model_flags & 0x1000010) == 0x1000010;
        case 2:
            return (object->apiobj.character_data->model_flags & 0x1000000) != 0 || object->field_0x108e == 6;
        case 3:
            if (object->field_0x108e == 5 ||
                (static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->flags_090 & 0x40000) !=
                    0)
                return 1;
            if (GCDataList[object->id].uses_weapon_action == 8 || GCDataList[object->id].uses_weapon_action == 1)
                return 1;
            return object->id == id_DARTHVADER || object->id == id_THEEMPEROR || object->id == id_GRANDMOFFTARKIN ||
                   object->id == id_IMPERIALOFFICER || object->id == id_IMPERIALSHUTTLEPILOT;
        default:
            return 0;
    }
}

GIZPANEL_s *GizPanel_FindNearest(WORLDINFO_s *world, nuvec_s *position, GameObject_s *object, float *distance_squared,
                                 i32 check_eligibility) {
    if (world == NULL || world->giz_panel_sys == NULL)
        return NULL;
    GIZPANEL_s *nearest = NULL;
    f32 nearest_distance = 1.0e9f;
    if (object != NULL && check_eligibility != 0) {
        for (i32 index = 0; index < world->giz_panel_sys->count; ++index) {
            GIZPANEL_s *panel = &world->giz_panel_sys->panels[index];
            if ((panel->flags & 0x0f) != 0x0c || panel->floor_position.y == 2000000.0f)
                continue;
            if (GizPanel_CanUsePanel(object, panel) == 0)
                continue;
            NUVEC target;
            GizPanel_GetAbsTargetPos(panel, &target, 0);
            const f32 distance = NuVecDistSqr(position, &target, NULL);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = panel;
            }
        }
    } else if (object != NULL) {
        for (i32 index = 0; index < world->giz_panel_sys->count; ++index) {
            GIZPANEL_s *panel = &world->giz_panel_sys->panels[index];
            if ((panel->flags & 0x0f) != 0x0c || panel->floor_position.y == 2000000.0f)
                continue;
            NUVEC target;
            GizPanel_GetAbsTargetPos(panel, &target, 0);
            const f32 distance = NuVecDistSqr(position, &target, NULL);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = panel;
            }
        }
    } else {
        for (i32 index = 0; index < world->giz_panel_sys->count; ++index) {
            GIZPANEL_s *panel = &world->giz_panel_sys->panels[index];
            const f32 distance = NuVecDistSqr(position, &panel->position, NULL);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = panel;
            }
        }
    }
    if (distance_squared != NULL)
        *distance_squared = nearest_distance;
    return nearest;
}

void GizPanel_GetAbsPlayerPos(GIZPANEL_s *panel, nuvec_s *position) {
    if (position != NULL) {
        if (panel != NULL && static_cast<u8>(panel->model_variant - 2) <= 1)
            GizPanel_GetAbsTargetPos(panel, position, 0);
        else
            *position = panel->floor_position;
    }
}

void GizPanel_GetAbsTargetPos(GIZPANEL_s *panel, nuvec_s *target_position, i32 player_position) {
    if (target_position == NULL || panel == NULL) {
        return;
    }

    NUVEC offset;
    if (player_position != 0) {
        if (panel->model_variant == 0) {
            offset = {-0.04f, panel->target_offset.y, -0.3f};
        } else if (panel->model_variant == 1) {
            offset = {0.035f, panel->target_offset.y, -0.25f};
        } else {
            offset = panel->target_offset;
        }
    } else {
        offset = panel->target_offset;
    }

    NuVecRotateY(&offset, &offset, panel->y_rotation);
    offset.x += panel->position.x;
    offset.z += panel->position.z;
    *target_position = offset;
}

#include "legoapi/gizmo/base/GizPanelObjectInterface.h"

void GIZPANEL_s::ClearMechObjectInterface() {
    delete mech_object_interface;
}

MechObjectInterface *GIZPANEL_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL) {
        new GizPanelObjectInterface(*this);
    }
    return mech_object_interface;
}
