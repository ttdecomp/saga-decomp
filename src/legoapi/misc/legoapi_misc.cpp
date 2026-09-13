#include "decomp.h"
#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/startup/game.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/world/levels/episode.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include <math.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern void RestoreOptions(void);
extern void *CutStopInfo;
extern "C" {
    extern i32 Paused;
    extern i32 NewMode;
    extern i32 CutSceneWaiting;
    extern i32 editor_active;
    extern i32 (*GamePads_IgnoreInputFn)(void);
    extern GAMEPAD_s GamePad[64];
    extern FadeSystem FadeSys;
}

// Original: 43 bytes.
i32 CircleLevel(LEVELDATA_s *level) {
    return BONUS_GUNSHIPB_LDATA != NULL && level == BONUS_GUNSHIPB_LDATA;
}

void CurrentStart(GameObject_s *, i32, i32) {
}

void NewRumble(nupad_s *, f32, i32);
void NewRumbleAllPlayers(f32, f32, i32, i32);

void ConstantRumble(GameObject_s *object, float strength, float phase) {
    phase = NuFmod(phase + GameTimer.time_elapsed, 1.25f);
    f32 weight = 0.0f;
    if (phase < 1.0f) {
        i32 angle = static_cast<i32>(phase * 32768.0f + 16384.0f);
        weight = 1.0f - fabsf(NuTrigTable[(angle >> 1) & 0x7fff]);
    }
    strength = weight * strength;
    if (object == NULL) {
        NewRumbleAllPlayers(strength, 0.0f, 0, 0);
    } else if ((object->apiobj.flags_low & 0x80) != 0) {
        NewRumble(object->pad_gamepad->pad, strength, 0);
    }
}

extern i32 AllMiniKitsDone(AREASAVE_s *save);

COLLECTID *CollectIDUnlocked(i32 id) {
    i32 index = InCollectList_Index(id, CollectList, CollectCount);
    if (index == -1) {
        return NULL;
    }

    COLLECTID *entry = &CollectList[index];
    if (Game_CharacterSave != NULL && (Game_CharacterSave[id] & SAVE_CHARACTER_UNLOCKED) != 0) {
        return entry;
    }

    switch (entry->type) {
        case 0:
            return entry;
        case 2:
            if (static_cast<i8>(entry->field2_0x3) == -1 || Game_AreaSave == NULL) {
                return NULL;
            }
            return Game_AreaSave[entry->field2_0x3].area_complete != 0 ? entry : NULL;
        case 3:
            if (Episodes_Completed() != EPISODECOUNT) {
                return NULL;
            }
            return Game_100PercentComplete() != 0 ? entry : NULL;
        case 4:
            return AllMiniKitsDone(Game_AreaSave) != 0 ? entry : NULL;
        case 6:
            if (Game_CompletionSave == NULL ||
                reinterpret_cast<STATUSCOLLECT_s *>(Game_CompletionSave)->gold_bricks < entry->field6_0xa) {
                return NULL;
            }
            return entry;
        case 7:
            return Game_100PercentComplete() != 0 ? entry : NULL;
        case 8:
            return Store_IsPackUnlocked(static_cast<i8>(entry->field2_0x3)) != 0 ? entry : NULL;
        default:
            return NULL;
    }
}

void ClearLastSafeTakeOver(GameObject_s *object) {
    if (object == NULL || (object->field_0xefa & 0x10) != 0) {
        return;
    }
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *candidate = &Obj[i];
        if (candidate != NULL && (candidate->apiobj.field_0x1f8 & 0x1001) == 0x1001 &&
            candidate->takeover_source == object) {
            candidate->takeover_source = NULL;
        }
    }
}

void GetNativeTextureFormatName(NUTEXFORMAT) {
}

void CatIToX(char *, i32) {
}

void DoInput(WORLDINFO_s *world) {
    if (world == NULL) {
        world = WorldInfo_CurrentlyActive();
    }

    const i32 player_0_input = ReadPad(0);
    const i32 player_1_input = ReadPad(1);
    const i32 player_state_changed = PlayersDropInOut();

    for (i32 player_index = 0; player_index < 2; ++player_index) {
        const i32 input_result = player_index == 0 ? player_0_input : player_1_input;
        if (GamePads_IgnoreInputFn != NULL && GamePads_IgnoreInputFn() != 0) {
            continue;
        }
        if (input_result <= 1 || player_state_changed != 0) {
            continue;
        }

        GameObject_s *player = Player[player_index];
        if (player == NULL || static_cast<i8>(player->apiobj.field_0x1f8) >= 0 ||
            (LEGOCONTEXT_DROPIN != -1 && static_cast<i8>(player->field_0x7a5) == LEGOCONTEXT_DROPIN) ||
            (GamePad[player_index].buttons_pressed & GAMEPAD_START) == 0) {
            continue;
        }
        if (NewMode != 0 || NewLData != NULL || FadeSys.fade != 0.0f || editor_active != 0 ||
            GameTimer.time_elapsed <= 0.0f || world == NULL || world->current_level == NULL ||
            world->current_level == TITLES_LDATA) {
            continue;
        }

        const bool player_can_resume = pause_i_pad == -1 || pause_i_pad == player_index;
        if (Paused != 0 || (GameMenu[GameMenuLevel].menu != -1 && NetPaused != 0)) {
            if (player_can_resume) {
                ResumeGame(1, 1);
                RestoreOptions();
            }
            continue;
        }
        if (GameMenu[GameMenuLevel].menu != -1 || CutSceneWaiting != 0 || MiniCutCam != 0 ||
            memcard_autosavestarted != 0 || memcard_autosavepostdelay > 0.0f || memcard_autosavepredelay > 0.0f ||
            GameTimer.update_count == 0) {
            continue;
        }
        if (CUTSTOPGAME != 0 && !CutScene_IsSkippable(static_cast<CUTINFO *>(CutStopInfo))) {
            continue;
        }

        PauseGame(static_cast<i32>(player->pad_gamepad - GamePad));
    }
}

void CatI64ToX(char *, i64) {
}

void DieRumble(GameObject_s *) {
}

void charToInt(char const *) {
}

static __used__ i32 _fseek64_wrap(__sFILE *, i64, i32) {
    return 0;
}
