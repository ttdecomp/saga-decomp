#include "decomp.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "nu2api/nu3d/nutex.h"
#include "legoapi/characters/core/players.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/world/area.h"
#include "legoapi/world/mission.h"
#include "legoapi/core/input/gamepads.h"
#include "gamelib/util/gamelib_util_types.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

i32 only_process_this_hint_id = -1;
static __used__ i32 Attack_UpdateHint(HINT_s *) {
    if (player == NULL || player->field_0x7a5 != 0xff || VehicleArea != 0)
        return 0;
    const u32 flags = player->apiobj.character_data->model_flags;
    if ((flags & 0x2010) != 0 || (flags & 0x88) == 0)
        return 0;
    return player->ai.opponent != NULL;
}

u8 show_unlock_shop_hint = 0;
u8 show_unlock_customiser_hint = 0;
u8 show_unlock_minikitviewer_hint = 0;

i32 HINT_COMPLETE(i32 hint_id) {
    if (hint_id < 0) {
        return 0;
    }

    i32 word = hint_id >> 5;
    if (word > 2) {
        return 0;
    }
    if (MechInputTouchSystem::s_baseControlMode != 0) {
        word += 3;
    }
    return Game.hint_completion_bits[word] & (1U << (hint_id & 0x1f));
}

i32 Tilt_UpdateHint(HINT_s *) {
    return 0;
}

void CurrentHintAlpha() {
}

i32 Dodge_UpdateHint(HINT_s *hint) {
    if (WORLD->area != NULL && WORLD->area == HUB_ADATA)
        return 0;
    for (i32 i = 0; i < 8; ++i) {
        GameObject *object = Player[i];
        if (object == NULL)
            continue;
        const u32 flags = object->apiobj.character_data->model_flags;
        if ((flags & 0x2000) != 0)
            return 0;
        if ((object->apiobj.field_0x1f8 & 0x80) == 0 || object->incoming_bolt == NULL)
            continue;
        if (hint->control_mode_ids[0] == 0x265) {
            if ((flags & 8) == 0 && (object->apiobj.character_model->model_data_b[0x4f] != NULL ||
                                     object->apiobj.character_model->model_data_b[0x26] != NULL))
                return 1;
        } else if (hint->control_mode_ids[0] == 0x5dd && (flags & 8) != 0) {
            return 1;
        }
    }
    return 0;
}

void SET_HINT_COMPLETE(i32 hint_id) {
    if (hint_id < 0) {
        return;
    }

    i32 word = hint_id >> 5;
    if (word > 2) {
        return;
    }
    if (MechInputTouchSystem::s_baseControlMode != 0) {
        word += 3;
    }
    Game.hint_completion_bits[word] |= 1U << (hint_id & 0x1f);
}

void CLEAR_HINT_COMPLETE(i32 hint_id) {
    if (hint_id < 0) {
        return;
    }

    i32 word = hint_id >> 5;
    if (word > 2) {
        return;
    }
    if (MechInputTouchSystem::s_baseControlMode != 0) {
        word += 3;
    }
    Game.hint_completion_bits[word] &= ~(1U << (hint_id & 0x1f));
}

i32 DragBomb_UpdateHint(HINT_s *hint) {
    if (VehicleArea == 0 || player == NULL)
        return 0;
    if ((player->apiobj.character_data->player_config->flags_090 & 0x400) == 0 ||
        player->force_glow_candidate_kind == 0 || player->force_glow_candidate == NULL || !(player->field_0xd80 > 0.0f))
        return 0;
    const i32 id = static_cast<GameObject_s *>(player->force_glow_candidate)->id;
    if (hint->control_mode_ids[0] == 0x5ee) {
        if (id == id_DRAGBOMB)
            return 1;
    } else if (hint->control_mode_ids[0] == 0x28e) {
        if (id != id_DRAGBOMB)
            return 1;
    }
    return 0;
}

// Original local callbacks referenced by Hints_LSW.
extern u8 show_lever_hint;
extern i32 show_autojump_hint;
extern AREADATA_s *PODRACE_ADATA, *PODSPRINT_ADATA, *BONUS_GUNSHIP_ADATA;

static __used__ i32 Jump_UpdateHint(HINT_s *hint) {
    if (player == NULL || player->field_0x7a5 != 0xff || VehicleArea != 0)
        return 0;
    const CHARACTERDATA *data = player->apiobj.character_data;
    const u32 flags = data->model_flags;
    if ((flags & 0x2010) != 0 || (data->player_config->flags_094 & 4) != 0)
        return 0;
    switch (hint->control_mode_ids[0]) {
        case 0x604:
            return 1;
        case 0x629:
            if (MechInputTouchSystem::s_baseControlMode == 0)
                return 0;
            // This hint shares the double-jump eligibility check below.
        case 0x606:
            if ((flags & 8) != 0 || (data->player_config->flags_090 & 0x400000) != 0)
                return 1;
            break;
        case 0x26b:
        case 0x608:
            if ((flags & 8) != 0)
                return 1;
            break;
    }
    return 0;
}

static __used__ i32 Move_UpdateHint(HINT_s *hint) {
    if (MechInputTouchSystem::s_baseControlMode == 0 || player == NULL || player->apiobj.field_0x27d == 0 ||
        player->field_0x7a5 != 0xff || VehicleArea != 0 || !TouchHacks::TouchControlsActive)
        return 0;
    switch (hint->control_mode_ids[0]) {
        case 0x5f1:
        case 0x5f2:
            return MechInputTouchSystem::s_actualTouchMode == 2;
        case 0x5f3:
            return 1;
    }
    return 0;
}

static __used__ i32 Lever_UpdateHint(HINT_s *) {
    if (show_lever_hint != 0) {
        show_lever_hint = 0;
        return true;
    }
    return false;
}

static __used__ i32 AutoJump_UpdateHint(HINT_s *hint) {
    if (hint->control_mode_ids[0] != show_autojump_hint)
        return false;
    if (SuperOptions.touch_controls == 0 || TestForController()) {
        show_autojump_hint = 0;
        return false;
    }
    return true;
}

static __used__ i32 VehicleStuff_UpdateHint(HINT_s *hint) {
    if (VehicleArea == 0)
        return 0;
    AREADATA_s *area = WORLD->area;
    if (area == NULL || (area->flags & 4) != 0 || player == NULL || player->field_0x7a5 != 0xff)
        return 0;
    if (hint->control_mode_ids[0] != 0x617 || area == PODRACE_ADATA || area == PODSPRINT_ADATA ||
        area == BONUS_GUNSHIP_ADATA)
        return 0;
    const GAMECHARACTERDATA *data = player->apiobj.character_data->game_character;
    if (data->field_0x88 > 0.0f)
        return (data->flags_090 & 0x10000) == 0;
    return 0;
}

static __used__ i32 HoldTag_UpdateHint(HINT_s *) {
    if (MechInputTouchSystem::s_baseControlMode == 0 || WORLD == NULL || WORLD->area == HUB_ADATA) {
        return 0;
    }
    return VehicleArea == 0;
}

u8 show_hatmachine_hint;

static __used__ i32 HatMachine_UpdateHint(HINT_s *hint) {
    if (show_hatmachine_hint == 5 && hint->control_mode_ids[0] == 0x627) {
        show_hatmachine_hint = 0;
        return TTab[0x627] != NULL;
    }
    if (show_hatmachine_hint == 6 && hint->control_mode_ids[0] == 0x628) {
        show_hatmachine_hint = 0;
        return TTab[0x628] != NULL;
    }
    return 0;
}

extern HINTUIBUTTON_s *hintUIButton;

void *GetHintFromUIButton() {
    return hintUIButton != NULL ? hintUIButton->field_0x78 : NULL;
}

extern HINT_s Hints_LSW[55];
void DrawHint_LSW(HINT_s *, i32);
void RegisterWithHintSys(void (*)(HINT_s *, i32), HINT_s *, u32 *, i32);
i32 id_HINT_LSW_AUTOJUMP = -1;
i32 id_HINT_LSW_AUTOJUMP_FAIL = -1;

void initGameHintSys_LSW() {
    RegisterWithHintSys(DrawHint_LSW, Hints_LSW, Game.hint_completion_bits, 3);
    reinterpret_cast<u8 *>(&LSW_HintConditions)[0] &= ~7U;
    id_HINT_LSW_AUTOJUMP = 0x620;
    id_HINT_LSW_AUTOJUMP_FAIL = 0x621;
}

i32 Cheat_IsOn(i32);

i32 SmartBomb_UpdateHint(HINT_s *) {
    if (VehicleArea == 0 || BonusArea != 0)
        return 0;
    if (!Cheat_IsOn(0x14))
        return 0;
    if (player == NULL)
        return 0;
    return player->field_0x7a5 == 0xff;
}

extern AREADATA *JABBASPALACE_ADATA;

i32 ShinyMetal_UpdateHint(HINT_s *hint) {
    if (player == NULL)
        return 0;
    if (hint->control_mode_ids[0] == 0x283 && JABBASPALACE_ADATA != NULL &&
        WORLD->current_level == JABBASPALACEB_LDATA && Game.area_save[JABBASPALACE_ADATA->index].area_complete == 0 &&
        Mission_Active(NULL) == NULL && (LevGizObst[0] == NULL || LevGizObst[0]->anim_set->state != 0) &&
        LevBlowUp[0] != NULL && (LevBlowUp[0]->status_flags & 1) == 0) {
        if (player != NULL &&
            NuVecDistSqr(&player->apiobj.collision_position, &LevBlowUp[0]->mid_position, NULL) < 25.0f)
            return 1;
        if (player2 != NULL &&
            NuVecDistSqr(&player2->apiobj.collision_position, &LevBlowUp[0]->mid_position, NULL) < 25.0f)
            return 1;
    }
    if (FreePlay == 0) {
        if (hint->control_mode_ids[0] == 0x2b8) {
            if ((player->apiobj.character_data->model_flags & 0x1000000) == 0 && player->field_0x108e != 6)
                return 0;
        } else if (hint->control_mode_ids[0] != 0x283 || AvailableToPlayer(0x1000000, -1, 6, 1) == 0) {
            return 0;
        }
    }
    GIZMOBLOWUP_s *blowup = WORLD->gizmo_blowups;
    if (blowup == NULL)
        return 0;
    for (i32 i = 0; i < WORLD->gizmo_blowup_count; ++i, ++blowup) {
        if ((blowup->draw_flags & 2) == 0 || (blowup->status_flags & 0x804001) != 0x804000)
            continue;
        if (NuVecDistSqr(&player->apiobj.position, &blowup->mid_position, NULL) < 4.0f) {
            if ((player->apiobj.character_data->model_flags & 0x1000000) != 0 || player->field_0x108e == 6)
                return hint->control_mode_ids[0] == 0x2b8;
            if (AvailableToPlayer(0x1000000, -1, FreePlay == 0 ? 6 : 0, 1) != 0)
                return hint->control_mode_ids[0] == 0x283;
            return hint->control_mode_ids[0] == 0x61d;
        }
    }
    return 0;
}

void CurrentHintButtonScale() {
}

void initGameHintSys_Batman() {
}

void IndyUnlocked_UpdateHint(HINT_s *) {
}

static __used__ i32 Sith_UpdateHint(HINT_s *) {
    if (FreePlay == 0 || (WORLD->area != NULL && WORLD->area == HUB_ADATA) || player == NULL ||
        player->field_0x7a5 != 0xff)
        return 0;
    if (WORLD->giz_force_sys == NULL || WORLD->giz_force_sys->visible_force_count == 0 ||
        AvailableToPlayer(0xc, -1, 0, 1) != 0)
        return 0;
    for (i32 i = 0; i < WORLD->giz_force_sys->visible_force_count; ++i) {
        GIZFORCE_s *force = WORLD->giz_force_sys->visible_forces[i];
        if ((force->config_flags & 0x10) != 0 && NuVecDistSqr(&player->apiobj.position, &force->position, NULL) < 6.25f)
            return 1;
    }
    return 0;
}

static __used__ void PlayerButton_PlayHint(HINT_s *) {
    const VuVec &position = MechSystems::Get()->PlayerButton().position;
    MechSystems::Get()->NewRadarPulse(position, false);
}

static __used__ i32 PlayerButton_UpdateHint(HINT_s *hint) {
    if (WORLD == NULL || WORLD->area == HUB_ADATA || VehicleArea != 0)
        return 0;
    if (hint->control_mode_ids[0] == 0x5f4) {
        if (FreePlay == 0)
            return 1;
    } else if (hint->control_mode_ids[0] == 0x5f5) {
        if (FreePlay != 0)
            return 1;
    }
    return 0;
}

static __used__ i32 UnlockHubStuff_UpdateHint(HINT_s *hint) {
    if (WORLD == NULL || WORLD->current_level != HUB_LDATA)
        return 0;
    switch (hint->control_mode_ids[0]) {
        case 0x619:
            if (show_unlock_shop_hint != 0)
                return 1;
            break;
        case 0x61a:
            if (show_unlock_customiser_hint != 0)
                return 1;
            break;
        case 0x61b:
            if (show_unlock_minikitviewer_hint != 0)
                return 1;
            break;
    }
    return 0;
}

i32 GizForce_UpdateHint(HINT_s *);
i32 ZipUps_UpdateHint(HINT_s *);
i32 Tag_UpdateHint(HINT_s *);
i32 Push_UpdateHints(HINT_s *);
i32 Teleport_UpdateHints(HINT_s *);
i32 GizPanel_UpdateHint(HINT_s *);
i32 Shop_UpdateHint(HINT_s *);
HINT_s Hints_LSW[55] = {
    {{356, 356}, 12, {255, 0, 0}, 0, 6.0f, 2.0f, {NULL}, NULL, {0, 0, 0, 0}, 0.0f},
    {{601, 375}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizForce_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1548, 1549}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Lever_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1568, 1568}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {AutoJump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1569, 1569}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {AutoJump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{604, 374}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizBuildIts_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{631, 1553}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Attack_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{613, 1500}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Dodge_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1501, 1502}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Dodge_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{610, 1503}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {ZipUps_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{602, -1}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{615, 615}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Push_UpdateHints}, NULL, {0, 0, 0, 0}, 0.0f},
    {{616, 1507}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Teleport_UpdateHints}, NULL, {0, 0, 0, 0}, 0.0f},
    {{617, 617}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{618, 618}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{605, 1508}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{606, 379}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{611, 1509}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{650, 377}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{651, 378}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{643, 643}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {ShinyMetal_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1505, 1506}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {SmartBomb_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{607, 1510}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{608, 1511}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{609, 1512}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1516, 1516}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {Shop_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1515, 1515}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {Shop_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1514, 1514}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {Shop_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1518, 1519}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {DragBomb_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{654, 1520}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {DragBomb_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1521, 1521}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {Move_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1523, 1523}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {Move_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1540, 1541}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Jump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1522, 1522}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {Move_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1542, 1543}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Jump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1524, 1524}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {PlayerButton_UpdateHint}, PlayerButton_PlayHint, {0, 0, 0, 0}, 0.0f},
    {{1525, 1525}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {PlayerButton_UpdateHint}, PlayerButton_PlayHint, {0, 0, 0, 0}, 0.0f},
    {{1544, 1545}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Jump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1546, 1547}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Jump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1526, 1526}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {HoldTag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1561, 1561}, 8, {255, 0, 0}, 0, 6.0f, 2.0f, {UnlockHubStuff_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1562, 1562}, 8, {255, 0, 0}, 0, 6.0f, 2.0f, {UnlockHubStuff_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1563, 1563}, 8, {255, 0, 0}, 0, 6.0f, 2.0f, {UnlockHubStuff_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{649, 1558}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {Tag_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1559, 1560}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {VehicleStuff_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1565, 1565}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {ShinyMetal_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{696, 1566}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {ShinyMetal_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1570, 1570}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {Teleport_UpdateHints}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1571, 1571}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {Sith_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1572, 1572}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1573, 1573}, 32, {255, 0, 0}, 0, 6.0f, 2.0f, {GizPanel_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1575, 1575}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {HatMachine_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1576, 1576}, 0, {255, 0, 0}, 0, 6.0f, 2.0f, {HatMachine_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{1577, 1577}, 16, {255, 0, 0}, 0, 6.0f, 2.0f, {Jump_UpdateHint}, NULL, {0, 0, 0, 0}, 0.0f},
    {{-1, -1}, 0, {255, 0, 0}, 1000, 1.0f, 1.0f, {NULL}, NULL, {0, 0, 0, 0}, 0.0f},
};
