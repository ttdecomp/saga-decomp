#include "decomp.h"
#include "globals.h"

#include <string.h>

#include "gameapi/gui/apimenu.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/customiser.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "legoapi/world/levels/episode.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numusic/numusic.h"

#include <stdio.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern f32 GetVehicleAreaRememberSpeed(void);
extern void AveragePlayerCurrentSpeedMul(void);
extern void SetPlayer(void);
extern void ResetPlayer(GameObject_s *, i32, nuvec_s *, i32);
extern void GameFog_Reset(void);
extern void ConfigureComplexShadow(GameObject_s *);
extern void ResetAdaptiveDifficulty(void);
extern void Cheats_TurnOff(i32);
extern void Hint_ClearHintsAndDoneFlags(void);
extern void GamePad_InitButtons(void);
extern void FinishWeirdoNames(i32);
extern void Store_UnlockPack(i32, bool);
extern void ReCalculateCompletionPoints(void);
extern void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);
extern "C" void NuSound3StopRumble(void);
extern i16 id_DEFAULTCHARACTER[2];
extern "C" i32 NewMode;
extern "C" i32 Paused;
extern "C" i32 memcard_autosavedisabled;
extern "C" i32 memcard_autosaveenabled;

namespace {

    constexpr f32 kSuperStoryTimeLimit = 3600.0f;
    constexpr i32 kSuperStoryScoreTarget = 100000;
    constexpr i32 kPrimaryCustomNameTextId = 0xcc;
    constexpr i32 kSecondaryCustomNameTextId = 0xcd;

} // namespace

void ClearPause() {
    Paused = 0;
    NetPaused = 0;
}

void ResumeGame(i32 play_sound, i32 resume_music) {
    Paused = 0;
    NetPaused = 0;
    MenuRememberCursor(&GameMenu[GameMenuLevel]);
    MenuReset();
    music_man.SetFader(1.0f, 0.5f);
    if (resume_music != 0) {
        music_man.ResumeTrack(0x10);
    }
    if (play_sound != 0) {
        GameAudio_PlaySfx(0x37, NULL, 0, 0);
    }
    if (ResumeGame_ExtraCodeFn != NULL) {
        ResumeGame_ExtraCodeFn();
    }
}

void InitGameMode() {
    if ((WORLD->current_level->flags & LEVEL_GAMEPLAY) != 0) {
        MenuReset();
    }

    GameObject_s *player = Player[0];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[1];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[2];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[3];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[4];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[5];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[6];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }
    player = Player[7];
    if (player != NULL && (player->field_0xeff & 1) == 0) {
        ResetPlayer(player, 1, NULL, 1);
    }

    f32 remember_speed = GetVehicleAreaRememberSpeed();
    if (VehicleAreaRememberSpeed < remember_speed) {
        VehicleAreaRememberSpeed = remember_speed;
    }
    AveragePlayerCurrentSpeedMul();
    SetPlayer();

    AREA_GLOBAL_VALUES &area_state = AreaGlobals.values;
    area_state.field_0x14 = WORLD->area != NULL ? area_state.field_0x0c : 0;
    area_state.field_0x20 = area_state.field_0x1c;
    NewLData = NULL;
    NewMode = 0;

    if (Player[0] != NULL && Player[0]->coinpacket != NULL) {
        GIZMO_PICKUP_TYPE *pickup_type = &GizmoPickupType[GetRandomCoinType()];
        Player[0]->coinpacket->lastcoin = pickup_type->first_model_id;
        if (pickup_type->random_model_count != 0) {
            Player[0]->coinpacket->lastcoin += qrand() / (0xffff / pickup_type->random_model_count + 1);
        }
    }

    if (Player[1] != NULL && Player[1]->coinpacket != NULL) {
        GIZMO_PICKUP_TYPE *pickup_type = &GizmoPickupType[GetRandomCoinType()];
        Player[1]->coinpacket->lastcoin = pickup_type->first_model_id;
        if (pickup_type->random_model_count != 0) {
            Player[1]->coinpacket->lastcoin += qrand() / (0xffff / pickup_type->random_model_count + 1);
        }
    }

    GameObject_s *object = Obj;
    const u16 player_object_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_PLAYER_CHARACTER;
    for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index, ++object) {
        if ((object->apiobj.field_0x1f8 & player_object_flags) == player_object_flags) {
            object->field_0xefc |= 0x80;
        }
    }

    GameFog_Reset();
    ConfigureComplexShadow(NULL);
}

void IncreaseScore(u32 *total, u64 amount, i32 apply_multiplier) {
    if (apply_multiplier != 0 && BonusArea == 0) {
        if (Cheats_CheckFlags(4) != 0) {
            amount *= 2;
        }
        if (Cheats_CheckFlags(8) != 0) {
            amount *= 4;
        }
        if (Cheats_CheckFlags(0x10) != 0) {
            amount *= 6;
        }
        if (Cheats_CheckFlags(0x20) != 0) {
            amount *= 8;
        }
        if (Cheats_CheckFlags(0x40) != 0) {
            amount *= 10;
        }
    }
    amount += *total;
    if (amount > 4000000000ULL) {
        amount = 4000000000ULL;
    }
    *total = static_cast<u32>(amount);
}

void RegisterHelpers() {
}

void NewGame() {
    TempOptions = Game.options_save;
    memset(&Game, 0, sizeof(Game));
    Game.options_save = TempOptions;

    MenuLoadOccurred = 0;
    MenuSaveOccurred = 0;
    ResetAdaptiveDifficulty();
    Game.save_version = 5;

    if (EDataList != NULL) {
        Game.area_save[EDataList[0].area_ids[0]].complete = 1;
    } else {
        Game.area_save[0].complete = 1;
    }

    if (SENATE_ADATA != NULL) {
        Game.area_save[SENATE_ADATA->index].complete = 1;
    }
    if (UTAPAU_ADATA != NULL) {
        Game.area_save[UTAPAU_ADATA->index].complete = 1;
    }
    if (HOTH_ADATA != NULL) {
        Game.area_save[HOTH_ADATA->index].complete = 1;
    }
    if (BONUSDAGOBAH_ADATA != NULL) {
        Game.area_save[BONUSDAGOBAH_ADATA->index].complete = 1;
    }
    if (BONUSKAMINO_ADATA != NULL) {
        Game.area_save[BONUSKAMINO_ADATA->index].complete = 1;
    }
    if (BONUSKASHYYYK_ADATA != NULL) {
        Game.area_save[BONUSKASHYYYK_ADATA->index].complete = 1;
    }

    Cheats_TurnOff(0);
    Hint_ClearHintsAndDoneFlags();
    LSW_HintConditions &= ~7u;
    GamePad_InitButtons();
    Tag_DoneFirst = 0;
    Tag_DoneAny = 0;

    for (i32 i = 0; i < AREACOUNT; ++i) {
        Game.area_save[i].challenge_trial_time = static_cast<f32>(ADataList[i].challenge_trial_time);
    }

    Game.episode_save[0].superstory_time_limit = kSuperStoryTimeLimit;
    Game.episode_save[1].superstory_time_limit = kSuperStoryTimeLimit;
    Game.episode_save[2].superstory_time_limit = kSuperStoryTimeLimit;
    Game.episode_save[3].superstory_time_limit = kSuperStoryTimeLimit;
    Game.episode_save[4].superstory_time_limit = kSuperStoryTimeLimit;
    Game.episode_save[5].superstory_time_limit = kSuperStoryTimeLimit;
    Game.episode_save[0].superstory_score_target = kSuperStoryScoreTarget;
    Game.episode_save[1].superstory_score_target = kSuperStoryScoreTarget;
    Game.episode_save[2].superstory_score_target = kSuperStoryScoreTarget;
    Game.episode_save[3].superstory_score_target = kSuperStoryScoreTarget;
    Game.episode_save[4].superstory_score_target = kSuperStoryScoreTarget;
    Game.episode_save[5].superstory_score_target = kSuperStoryScoreTarget;

    Customiser_CopyDefaultPiecesToSave(CharacterCustomiser, &Game.customizer);
    if (TTab != NULL) {
        if (TTab[kPrimaryCustomNameTextId] != NULL) {
            NuStrCpy(Game.customizer.primary_name, TTab[kPrimaryCustomNameTextId]);
        }
        if (TTab[kSecondaryCustomNameTextId] != NULL) {
            NuStrCpy(Game.customizer.secondary_name, TTab[kSecondaryCustomNameTextId]);
        }
        FinishWeirdoNames(-1);
    }

    if (id_DEFAULTCHARACTER[0] != -1) {
        Game.character_save[id_DEFAULTCHARACTER[0]] |= SAVE_CHARACTER_AVAILABLE | SAVE_CHARACTER_UNLOCKED;
    }
    if (id_DEFAULTCHARACTER[1] != -1) {
        Game.character_save[id_DEFAULTCHARACTER[1]] |= SAVE_CHARACTER_AVAILABLE | SAVE_CHARACTER_UNLOCKED;
    }
    if (id_WEIRDO1 != -1) {
        Game.character_save[id_WEIRDO1] |= SAVE_CHARACTER_AVAILABLE | SAVE_CHARACTER_UNLOCKED;
    }
    if (id_WEIRDO1 != -1 && CDataList != NULL) {
        CDataList[id_WEIRDO1].field0_0x0 = kPrimaryCustomNameTextId;
    }
    if (id_WEIRDO2 != -1) {
        Game.character_save[id_WEIRDO2] |= SAVE_CHARACTER_AVAILABLE | SAVE_CHARACTER_UNLOCKED;
    }
    if (id_WEIRDO2 != -1 && CDataList != NULL) {
        CDataList[id_WEIRDO2].field0_0x0 = kSecondaryCustomNameTextId;
    }

    Game.customizer.primary_name_unlocked = 1;
    Game.customizer.secondary_name_unlocked = 1;

    u8 previous_group = 0;
    for (i32 i = 0; i < 10; ++i) {
        if (Suit[i].group != previous_group) {
            Game.initial_store_pack_flags |= 1u << i;
        }
        previous_group = Suit[i].group;
    }

    for (i32 i = 0; i < 11; ++i) {
        if ((static_cast<u16>(SuperOptions.field0_0x0) & (1u << i)) != 0) {
            Store_UnlockPack(i, true);
        }
    }

    ReCalculateCompletionPoints();
}

void PauseGame(i32 pad_index) {
    Paused = 1;
    music_man.SetFader(0.0f, 0.5f);
    music_man.PauseTrack(0x10);
    NuSound3StopRumble();

    if (memcard_autosaveenabled != 0 && memcard_autosavedisabled != 0) {
        NewMenu(0x3f3, 0, -1);
    } else if (CUTSTOPGAME != 0) {
        NewMenu(LEGOMENU_PAUSECUT, 0, -1);
    } else {
        NewMenu(LEGOMENU_PAUSEMAIN, 0, -1);
    }

    ResetTimer(&PauseTimer, 0.0f);
    GameAudio_PlaySfx(0x36, NULL, 0, 0);
    DoubleScoreTime = 0.0f;
    ResetTimer(&JoinInTimer, 0.0f);
    pause_i_pad = pad_index;

    for (i32 i = 0; i < 8; ++i) {
        if (Player[i] != NULL) {
            Player[i]->hud_icon_timer = 0.0f;
            Player[i]->pause_context_state = 0;
            Player[i]->input_toggle_hold_time = TOGGLEHOLDTIME;
        }
    }

    if (PauseGame_ExtraCodeFn != NULL) {
        PauseGame_ExtraCodeFn();
    }
}
