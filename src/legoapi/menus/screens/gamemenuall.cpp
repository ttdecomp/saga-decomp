#include "decomp.h"
#include "gameapi/edtools/edui.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "MechInputTouch/MechInputTouch_types.h"
#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/menus/screens/gamemenuall.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/mission.h"
#include "legoapi/audio/audio.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/render/core/render.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuqfnt.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nutrig.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

MENUPACKET_s MenuPacket = {};
static f32 MissionsAlpha;
static i32 hub_mission;
static f32 MissionIconScale[20] __attribute__((aligned(16)));
static f32 MissionIconTargetX[20] __attribute__((aligned(16)));
static f32 MissionIconX[20] __attribute__((aligned(16)));
extern i32 NextArea_FreePlay;
void InitMission(MISSIONSYS *, i32);
extern "C" GAMEPAD_s GamePad[64];
extern u32 GAMEPAD_MENUSELECT, GAMEPAD_MENUCANCEL, GAMEPAD_DLEFT, GAMEPAD_DRIGHT;
extern u32 GAMEPAD_TOGGLELEFT, GAMEPAD_TOGGLERIGHT;
extern f32 ICONSIZE, ICONX, DROPINALPHA, HUB_EPISODETITLEY;
extern i16 tSELECT, tSELECTED, tSELECTING, tEXIT, tCANCEL;
void DrawCharIcon(i32, f32, f32, f32, f32, i32, f32, f32, i32, nuhspecial_s *);
void Hub_DrawImportantBrick(i32, f32, f32, f32, i32, i32);
void DrawPlayerIconPrompts(i32, i32, f32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32);
f32 GetAspectRatio();
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);

extern "C" void NewMenu(i32 menu_id, i32 menu_y, i32 param3);
extern "C" void BackupMenu(void);
extern "C" void BackupMenuNoFn(void);
extern "C" bool TestForController(void);
extern "C" void PlaySfxById(i32 sfx_id, nuvec_s *position);
extern "C" void NuIOS_RecordFlurryEvent(char *event_name);
extern "C" void DrawMenuButtonPrompts(i32 confirm_prompt, i32 cancel_prompt, i32 enabled, u8 red, u8 green, u8 blue,
                                      u8 alpha);
extern "C" void DrawMenuButtonPromptsEx(i32 confirm_prompt, i32 cancel_prompt, i32 flags, i32 enabled, u8 red, u8 green,
                                        u8 blue, u8 alpha);
i32 GameAudio_GetSfxId(i32 sfx);
extern i32 SAVESLOTS;
extern i32 MenuSFX;
extern i32 MENUSFX_MENUSELECT;
extern i32 MENUSFX_MENUBACK;
extern i32 MENUSFX_MENUNOENTRY;
extern i32 MENUSFX_MENUMOVE;
extern void (*drawslotsfn)(MENU *, f32);
extern char *apitxt_LOADGAME;
extern char *apitxt_SAVEGAME;
extern char *apitxt_CONFIRMSAVE;
extern char *apitxt_CONFIRMOVERWRITE;
extern char *apitxt_CONFIRMLOAD;
extern char *apitxt_DOYOUWANTTOABORT;
extern char *apitxt_DOYOUWANTTOABORTLOAD;
extern char *apitxt_RETRY;
extern char *apitxt_SLOT;
extern char *apitxt_NODATAAVAILABLE;
extern char *apiGameName;
extern char *apitxt_YES;
extern char *apitxt_NO;
extern char **TTab;
extern i16 tSELECTCONTROLS;
extern i16 tCONTROLSCANBECHANGED;
extern i16 tCLASSIC;
extern i16 tTOUCHSCREEN;
extern i16 tCONTROLLERCONNECTED;
extern i16 tCONTROLS;
extern i16 tCONSOLE;
extern i16 tTOUCH;
extern i16 tSURROUNDSOUND;
extern i16 tAUDIOVOLUME;
extern i16 tMUSIC;
extern i16 tWIDESCREEN;
extern i16 tON;
extern i16 tOFF;
extern i16 tACCEPT;
extern i16 tBACK;
extern i16 tCONTINUE;
extern i16 tCANCEL;
extern i16 tSTORY;
extern i16 tFREEPLAY;
extern i16 tREPLAYSTORY;
extern i16 tLOCKED;
extern i16 tAVAILABLETOBUY;
extern i16 tPOWERBRICK;
extern i16 tHOWTOPLAY;
extern OPTIONSSAVE TempOptions;
extern i32 GAMEDEMO;
extern i32 menu_flash;
extern f32 text3d_height;
extern f32 text3d_width;
extern i32 Paused;
extern f32 PauseMenus_X;
extern i32 PauseMenus_Align;
void GameDrawMenuEntry(MENU *menu, char *text);
i32 GetParentMenuID(void);
void RestoreOptions(void);
f32 GameSetSoundVolume(OPTIONSSAVE *options);
f32 GameSetMusicVolume(OPTIONSSAVE *options);
void SfxCheckMusicOnOff(OPTIONSSAVE *options);
void NewGame(void);
void NuIOS_RestoreInAppPurchases(void);
extern i32 startnewgame;
extern i32 startnewgame_initiated;
extern f32 newgamecamtime;
extern i32 newgamecam;
extern "C" u8 MENUNORMALR;
extern "C" u8 MENUNORMALG;
extern "C" u8 MENUNORMALB;
extern "C" f32 sfx_wait;
extern "C" f32 MENUBOTY;
void Draw_AUTOSAVEWARNING();
void Draw_NODATAAVAILABLE();
void Draw_NOMEMORYCARD();
extern "C" void Draw_NOTENOUGHSPACE(void);
extern "C" void Draw_SPACENEEDED(void);

i32 memcard_cardchanged;
i32 ButtonScaleMode;
i32 Menu_InLoadFlow;
i32 Menu_InWarningFlow;
i32 Menu_LastFlow;
i32 MenuLoadStarted;
i32 memcard_slot;
i32 Menu_InSaveFlow;
i32 Menu_DisableCancel;
i32 Menu_InHeaderSave;
i32 Menu_Overwriteflag;
i32 memcard_slotsused;
i32 memcard_saveneeded;
i32 memcard_savestarted;
i32 memcard_savefailed;
i32 memcard_loadneeded;
i32 memcard_loadstarted;
i32 memcard_loadfailed;
i32 memcard_loadcorrupt;
f32 memcard_savemessage_delay;
f32 memcard_saveresult_delay;
f32 memcard_loadmessage_delay;
f32 memcard_loadresult_delay;
i32 SAVESIZE = 50;

extern i32 memcard_autosave;
extern i32 memcard_autosaveenabled;
extern i32 memcard_autosavedisabled;
extern i32 header_r;
extern i32 header_g;
extern i32 header_b;
extern i32 MenuDisableHeaders;

static i32 lastslot;
static i32 slideleft;
static i32 slideright;

char *extrasavetext;
f32 extrasavealpha = 1.0f;
u8 RAP_WARNING_B = 0xff;
u8 RAP_WARNING_G;
u8 RAP_WARNING_R = 0xff;

static i32 g_enableButtonPrompts = 1;

extern i32 hub_new_level;
extern i32 hub_selectmode;
extern f32 selectmodetime;
extern i32 selectmodemode;
extern i32 FinishLoop_On;
extern FadeSystem FadeSys;
bool FreePlayUnlocked();
void Hub_DrawFreePlaySelect();
void Hub_InitFreePlaySelect(i32 area, i32 first_model, i32 second_model);
void Hub_UpdateFreePlaySelect();
void WipeBackToHub();
void NewLevelFromMenu(LEVELDATA_s *level, i32 menu_id, i32 menu_y, i32 remember_hub);

static bool MenuAreaAllowsFreePlay(i32 area) {
    return area >= 0 && area < AREACOUNT && (LOSTTEMPLE_ADATA == NULL || area != LOSTTEMPLE_ADATA->index) &&
           FreePlayUnlocked() && (ADataList[area].flags & AREAFLAG_NO_FREEPLAY) == 0 && Game_AreaSave != NULL &&
           Game_AreaSave[area].area_complete != 0;
}

extern CHEATSYSTEM CheatSystem;
void Cheat_SetOn(i32 cheat, i32 enabled, i32 update_save);
static f32 updateextras_current_y = 0.0f;

static bool MenuCheatUnlocked(i32 cheat) {
    if (cheat < 0 || cheat >= CheatSystem.cheats_count || cheat >= 64) {
        return false;
    }
    const u32 *unlocked = reinterpret_cast<const u32 *>(Game.field_0x7c00);
    return (unlocked[cheat >> 5] & (1u << (cheat & 31))) != 0;
}

void MenuDrawLoad(MENU_s *menu) {
    NuStrCpy(MenuHeader, apitxt_LOADGAME);
    header_r = MENUHEADERR;
    header_g = MENUHEADERG;
    header_b = MENUHEADERB;
    if (drawslotsfn != NULL && MenuAlpha > 0.2f && MenuStopDraw == 0) {
        drawslotsfn(menu, 0.0f);
    }
    ++menu->draw_item;
    Draw_CANCEL(menu);
}

void MenuDrawSave(MENU_s *menu) {
    NuStrCpy(MenuHeader, apitxt_SAVEGAME);
    header_r = MENUHEADERR;
    header_g = MENUHEADERG;
    header_b = MENUHEADERB;
    if (drawslotsfn != NULL && MenuAlpha > 0.2f && MenuStopDraw == 0) {
        drawslotsfn(menu, extrasavetext != NULL ? 0.1f : 0.0f);
    }
    ++menu->draw_item;

    if (extrasavetext != NULL) {
        SmartTextEx(extrasavetext, 0.0f, -0.25f, 1.0f, 0.5f, 0.5f, 0.5f, 0, RAP_WARNING_R, RAP_WARNING_G, RAP_WARNING_B,
                    1.9f, 4, NULL, 0, static_cast<i32>(static_cast<f32>(MenuA) * extrasavealpha));
    }
    if (Menu_DisableCancel == 0) {
        Draw_CANCEL(menu);
    }
}

void MenuExitLoad(MENU_s *) {
    Menu_InLoadFlow = 0;
}

void MenuExitSave(MENU_s *) {
    Menu_InSaveFlow = 0;
}

void MenuDrawClips(MENU_s *) {
}

void MenuDrawHints(MENU_s *menu) {
    NuStrCpy(MenuHeader, TTab[tHOWTOPLAY]);
    GameDrawMenuEntry(menu, TTab[tBACK]);
}

void MenuDrawStore(MENU_s *) {
}

void MenuEnterLoad(MENU_s *menu) {
    memcard_cardchanged = 0;
    i32 last_column = SAVESLOTS - 1;
    if (SAVESLOTS > 6) {
        last_column = memcard_slotsused - 1;
    }
    menu->state = 2;
    menu->last_column = static_cast<i16>(last_column);
    menu->last_row = 1;
    lastslot = 0;
    slideleft = 0;
    slideright = 0;
    Menu_InLoadFlow = 1;
    Menu_LastFlow = 3;
    MenuLoadOccurred = 0;
    MenuLoadStarted = 0;

    if (saveload_filecorrupt != 0) {
        NewMenu(1020, -1, -1);
    } else if (saveload_cardtype != 2) {
        NewMenu(1004, -1, -1);
    } else if (saveload_savepresent == 0 || saveload_cardformatted == 0) {
        NewMenu(1005, -1, -1);
    }
}

void MenuEnterSave(MENU_s *menu) {
    MenuSaveOccurred = 0;
    memcard_cardchanged = 0;
    Menu_DisableCancel = 0;
    Menu_InSaveFlow = 1;
    Menu_LastFlow = 2;

    if (SAVESLOTS <= 6) {
        menu->last_column = static_cast<i16>(SAVESLOTS - 1);
    } else {
        menu->last_column = static_cast<i16>(memcard_slotsused);
        if (SAVESLOTS == memcard_slotsused || saveload_freespace < SAVESIZE_ADDITIONAL) {
            --menu->last_column;
        }
    }
    menu->state = 2;
    menu->last_row = 1;
    lastslot = 0;
    slideleft = 0;
    slideright = 0;

    NuStrCpy(MenuHeader, apitxt_SAVEGAME);
    header_r = MENUHEADERR;
    header_g = MENUHEADERG;
    header_b = MENUHEADERB;

    if (saveload_cardtype != 2) {
        NewMenu(1002, -1, -1);
    } else if (saveload_filecorrupt != 0) {
        NewMenu(1020, -1, -1);
    } else if (saveload_cardformatted == 0) {
        NewMenu(1006, 1, -1);
    } else if (saveload_savepresent == 0 && saveload_freespace < SAVESIZE) {
        NewMenu(1003, -1, -1);
    } else if (Menu_InHeaderSave != 0) {
        memcard_slot = -1;
        NewMenu(1008, 1, -1);
    }
}

void MenuExitStore(MENU_s *) {
}

void MenuInitClips(MENU_s *) {
}

void MenuInitStore(MENU_s *) {
}

void MenuStartLoad() {
    MenuLoadStarted = 1;
    memcard_loadneeded = 1;
    memcard_loadstarted = 0;
    memcard_loadfailed = 0;
    memcard_loadcorrupt = 0;
}

void MenuStartSave() {
    memcard_saveneeded = 1;
    memcard_savestarted = 0;
    memcard_savefailed = 0;
}

void RenderFileSel() {
}

void MakeMenuPacket() {
    for (i32 player_index = 0; player_index < 2; ++player_index) {
        GameObject_s *object = Player[player_index];
        if (object == NULL) {
            MenuPacket.player_model[player_index] = -1;
            MenuPacket.active_player[player_index] = 0;
        } else {
            MenuPacket.player_model[player_index] = object->id;
            MenuPacket.active_player[player_index] = (object->apiobj.flags_low & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0;
        }
        MenuPacket.reserved_0[player_index] = 0;
        MenuPacket.reserved_1[player_index] = 0;
    }
}

void MenuDrawExtras(MENU_s *menu) {
    char text[256];
    menu->draw_y = -updateextras_current_y * MENUDY * menu->item_scale;
    for (i32 cheat = 0; cheat < CheatSystem.cheats_count; ++cheat) {
        CHEAT &entry = CheatSystem.cheats[cheat];
        const char *value;
        f32 alpha = 0.5f;
        bool locked_power_brick = false;

        if (MenuCheatUnlocked(cheat)) {
            value = TTab[Cheat_IsOn(cheat) != 0 ? tON : tOFF];
            if (BonusArea == 0 || Cheat_CheckFlags(cheat, 0x200000) == 0) {
                alpha = 1.0f;
            }
        } else if (cheat < 8 || entry.area == 0xff || Game.area_save[entry.area].field_0x5[1] != 0) {
            value = TTab[tAVAILABLETOBUY];
        } else {
            snprintf(text, sizeof(text), "%s %i: %s", TTab[tPOWERBRICK], cheat - 7, TTab[tLOCKED]);
            locked_power_brick = true;
        }

        if (!locked_power_brick) {
            const i16 text_id = entry.text_id != NULL ? *entry.text_id : -1;
            const char *name = text_id >= 0 && TTab[text_id] != NULL ? TTab[text_id] : entry.name;
            snprintf(text, sizeof(text), "%s: %s", name != NULL ? name : "", value);
        } else {
            dme_rgb = 1;
            dme_r = 0xdf;
            dme_g = 0x3f;
            dme_b = 0;
        }

        if (menu->draw_y > 0.6f) {
            alpha = menu->draw_y > 0.9f ? 0.0f : alpha * (1.0f - (menu->draw_y - 0.6f) / 0.3f);
        }
        if (Paused != 0) {
            dme_align = PauseMenus_Align;
            menu->draw_x = PauseMenus_X;
        }
        dme_sy = menu->item_scale;
        DrawMenuEntryEx(menu, text, static_cast<i32>(static_cast<f32>(MenuA) * alpha));
    }
}

void MenuDrawNoData(MENU_s *menu) {
    Draw_NODATAAVAILABLE();
    Draw_CANCEL(menu);
}

void MenuDrawSaving(MENU_s *) {
    Draw_AUTOSAVEWARNING();
}

void MenuUpdateLoad(MENU_s *menu) {
    if (memcard_cardchanged != 0 || saveload_cardtype != 2) {
        MenuEnterLoad(menu);
        return;
    }

    if (menu->confirm_pressed != 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        if (menu->selected_row == 0) {
            memcard_slot = menu->selected_column - menu->first_column;
            if (saveload_slotused[memcard_slot] == 0) {
                MenuSFX = MENUSFX_MENUNOENTRY;
                return;
            }

            MenuStartLoad();
            NewMenu(1014, 0, -1);
            return;
        }

        if (menu->selected_row == menu->last_row) {
            BackupMenu();
        }
        return;
    }

    if (menu->cancel_pressed != 0) {
        BackupMenu();
        MenuSFX = MENUSFX_MENUSELECT;
    }
}

void MenuUpdateSave(MENU_s *menu) {
    MenuInfo[menu->menu].wrap = Menu_DisableCancel == 0;

    if (memcard_cardchanged != 0 || saveload_cardtype != 2) {
        MenuEnterSave(menu);
    }

    if (menu->confirm_pressed != 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        if (menu->selected_row == 0) {
            const i32 slot = menu->selected_column - menu->first_column;
            if (saveload_slotused[slot] != 0) {
                memcard_slot = slot;
                NewMenu(1008, 1, -1);
            } else if (saveload_freespace < SAVESIZE_ADDITIONAL) {
                MenuSFX = MENUSFX_MENUNOENTRY;
            } else {
                memcard_slot = slot;
                MenuStartSave();
                NewMenu(1009, 0, -1);
            }
        } else if (menu->selected_row == menu->last_row && Menu_DisableCancel == 0) {
            BackupMenu();
        }
    } else if (menu->cancel_pressed != 0 && Menu_DisableCancel == 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        BackupMenu();
    }
}

void ProcessFileSel(float, nupad_s *) {
}

void RenderFileSel3(i32) {
}

void EndMissionsMenu() {
    LevLock[3] = 1;
    LevTime[3] = 0.6f;
    MenuRememberCursor(&GameMenu[GameMenuLevel]);
    MenuReset();
    ResetTimer(&JoinInTimer, 0.0f);
}

static NUGSCN **IconScene;
static char IconPath[0x40];

void IconScenes_Dump() {
    if (IconScene == NULL) {
        return;
    }

    for (i32 i = 0; i < CHARCOUNT; ++i) {
        if (IconScene[i] != NULL) {
            NuGScnRemove(IconScene[i]);
            IconScene[i] = NULL;
        }
    }
}

void IconScenes_Init(char *path, variptr_u *buf, variptr_u *) {
    const usize table_size = static_cast<usize>(CHARCOUNT) * sizeof(*IconScene);
    IconScene = reinterpret_cast<NUGSCN **>(ALIGN(buf->addr, 4));
    buf->addr = reinterpret_cast<usize>(IconScene) + table_size;
    memset(IconScene, 0, table_size);

    if (path != NULL && NuStrLen(path) <= 0x3f) {
        NuStrCpy(IconPath, path);
    }
}

void IconScenes_Load(APICHARACTERMODELLIST_s *list, i32, variptr_u *buf, variptr_u *buf_end) {
    if (list == NULL || IconScene == NULL) {
        return;
    }

    for (; list->model_id != -1; ++list) {
        const i32 character_id = list->model_id;
        if (IconScene[character_id] != NULL || list->count == 0) {
            continue;
        }

        const i32 object_index = CDataList[character_id].field20_0x42;
        if (object_index == -1) {
            continue;
        }

        char path[0x100];
        NuStrCpy(path, IconPath);
        NuStrCat(path, LevelObject_FindNameFromIndex(object_index));
        NuStrCat(path, ".gsc");
        IconScene[character_id] = NuGScnRead(buf, *buf_end, path);
        if (IconScene[character_id] != NULL && IconScene[character_id]->display_list != NULL) {
            IconScene[character_id]->display_list->flags |= NU_DISPLAYSCENE_FLAG_NEEDS_BUILD;
        }
    }
}

void MenuDrawLoading(MENU_s *) {
    Draw_AUTOSAVEWARNING();
}

void MenuDrawOptions(MENU_s *menu) {
    char text[256];

    menu->item_scale = 1.0f;
    dme_sy = 1.0f;

    if (TestForController() != 0) {
        DrawMenuEntryEx(menu, TTab[tCONTROLLERCONNECTED], MenuA / 2);
    } else {
        NuStrCpy(text, TTab[tCONTROLS]);
        NuStrCat(text, ": ");
        NuStrCat(text, TTab[MechInputTouchSystem::s_baseControlMode == 0 ? tCONSOLE : tTOUCH]);
        GameDrawMenuEntry(menu, text);
    }

    NuStrCpy(text, TTab[tSURROUNDSOUND]);
    NuStrCat(text, ": ");
    NuStrCat(text, TTab[TempOptions.field2_0x2 != 0 ? tON : tOFF]);
    dme_sy = menu->item_scale;
    GameDrawMenuEntry(menu, text);

    sprintf(text, "%s: %i/%i", TTab[tAUDIOVOLUME], TempOptions.field5_0x5, 10);
    dme_sy = menu->item_scale;
    GameDrawMenuEntry(menu, text);

    NuStrCpy(text, TTab[tMUSIC]);
    NuStrCat(text, ": ");
    NuStrCat(text, TTab[TempOptions.field6_0x6 != 0 ? tON : tOFF]);
    dme_sy = menu->item_scale;
    GameDrawMenuEntry(menu, text);

    if (GAMEDEMO == 0) {
        sprintf(text, "%s: %s", TTab[tWIDESCREEN], TTab[TempOptions.field11_0xb != 0 ? tON : tOFF]);
        dme_sy = menu->item_scale;
        GameDrawMenuEntry(menu, text);
    }

    if (memcmp(&TempOptions, &Game.options_save, sizeof(TempOptions)) == 0) {
        NuStrCpy(text, TTab[tBACK]);
    } else if (menu->selected_item != menu->draw_item && menu_flash != 0) {
        text[0] = '\0';
    } else {
        NuStrCpy(text, TTab[tACCEPT]);
    }
    dme_sy = menu->item_scale;
    GameDrawMenuEntry(menu, text);
}

void MenuExitOptions(MENU_s *) {
    RestoreOptions();
}

void MenuIsAvailable() {
}

void MenuUpdateClips(MENU_s *) {
}

void MenuUpdateHints(MENU_s *menu) {
    if (menu->cancel_pressed != 0 || menu->confirm_pressed != 0) {
        BackupMenu();
        MenuSFX = GameAudio_GetSfxId(0x31);
    }
}

void MenuUpdateStore(MENU_s *) {
}

void ProcessFileSel3(float, nupad_s *) {
}

void MenuDrawDeleting(MENU_s *) {
}

void MenuDrawEpisodes(MENU_s *) {
}

void MenuDrawFreePlay(MENU_s *) {
    if (MenuStopDraw == 0) {
        Hub_DrawFreePlaySelect();
    }
}

void MenuDrawMissions(MENU_s *menu) {
    if (MenuStopDraw)
        return;
    const f32 alpha = MenuAlpha;
    SmartTextEx(TTab[MissionSys->missions[hub_mission].name_id], 0.0f, -0.525f, 1.0f, 0.625f, 0.625f, 0.625f, 0, 255,
                191, 0, 1.7f, 1, 0, 0, static_cast<i32>(255.0f * MissionsAlpha * alpha));
    if (Game.mission_save.completed[hub_mission] != 0 && alpha * MissionsAlpha > 0.0f) {
        const f32 opacity = alpha * MissionsAlpha;
        Hub_DrawImportantBrick(211, -0.15f, HUB_EPISODETITLEY + 0.1f, opacity, -1, -1);
        char time[64], text[64];
        Text_MakeTime(Game.mission_save.best_times[hub_mission], 0, 1, 1, time);
        sprintf(text, "~0(~2%s~0)", time);
        Text3DEx(text, 0.15f, -0.365f, 1.0f, 0.5f, 0.5f, 0.5f, 0, 0, 255, 0, static_cast<u8>(opacity * 255.0f));
    }
    for (i32 i = 0; i < MissionSys->count; ++i) {
        f32 size = MissionIconScale[i] * 0.075f + 0.125f;
        f32 x = MissionIconX[i];
        if (fabsf(x) > 1.0f)
            size = 0.0f;
        else {
            f32 opacity = (MissionIconScale[i] * 0.3f + 0.7f) * alpha;
            if (fabsf(x) > 0.5f)
                opacity *= 1.0f - (fabsf(x) - 0.5f) * 2.0f;
            if (opacity > 0.0f)
                DrawCharIcon(MissionSys->missions[i].find_char, x, -0.725f, 0.0f, size,
                             Game.mission_save.completed[i] >= 1 ? 168 : 167, opacity,
                             (i == hub_mission && menu_flash ? 0.7f : 1.0f) * opacity, 1, NULL);
            if (opacity == 0.0f)
                size = 0.0f;
        }
        menu->item_x[i] = x;
        menu->item_y[i] = -0.725f;
        menu->item_width[i] = size;
        menu->item_height[i] = size / GetAspectRatio();
        menu->item_column[i] = i;
        menu->item_row[i] = 0;
    }
    f32 opacity = MenuPacket.active_player[0] ? 1.0f : DROPINALPHA;
    DrawCharIcon(MenuPacket.player_model[0], -ICONX, STATSPOSY, 0.0f, ICONSIZE, 166, opacity, opacity, 1, NULL);
    i32 select[2], cancel[2], status[2];
    if (MenuPacket.active_player[0] && MenuPacket.active_player[1]) {
        // Original 0x2563e2–0x25640a / 0x25661f initializes only player 0.
        // The second player's prompt locals are also uninitialized in the binary.
        if (MenuPacket.reserved_1[0]) {
            select[0] = -1;
            cancel[0] = tCANCEL;
            status[0] = tSELECTED;
        } else {
            select[0] = tSELECT;
            cancel[0] = tEXIT;
            status[0] = tSELECTING;
        }
    } else {
        select[0] = select[1] = tSELECT;
        cancel[0] = cancel[1] = tEXIT;
        status[0] = status[1] = tSELECTING;
    }
    DrawPlayerIconPrompts(MenuPacket.active_player[0], select[0], 1.0f, -1, cancel[0], -1, status[0],
                          MenuPacket.active_player[1], select[1], 1.0f, -1, cancel[1], -1, status[1]);
}

void MenuEnterOptions(MENU_s *) {
    const i32 parent_menu = GetParentMenuID();
    if (parent_menu == 1 || parent_menu == 2) {
        TempOptions = Game.options_save;
    }
}

void MenuInitEpisodes(MENU_s *) {
}

void MenuInitFreePlay(MENU_s *menu) {
    menu->first_row = 0;
    menu->last_row = 2;
    menu->selected_row = 0;
    menu->selected_item = 0;
}

void MenuInitMissions(MENU_s *) {
    MissionsAlpha = 0.0f;
    if (MissionSys != NULL) {
        for (i32 i = 0; i < MissionSys->count; ++i)
            MissionIconScale[i] = i == hub_mission ? 1.0f : 0.0f;
        for (i32 i = 0; i < MissionSys->count; ++i) {
            f32 x = 0.0f;
            if (i < hub_mission)
                x = -0.035f - (hub_mission - i) * 0.12f;
            else if (i != hub_mission)
                x = 0.035f + (i - hub_mission) * 0.12f;
            MissionIconTargetX[i] = MissionIconX[i] = x;
        }
    }
}

void MenuUpdateExtras(MENU_s *menu) {
    if (menu->cancel_pressed != 0) {
        BackupMenu();
        MenuSFX = GameAudio_GetSfxId(0x31);
    }
    if (menu->confirm_pressed != 0) {
        const i32 cheat = menu->selected_item;
        if (!MenuCheatUnlocked(cheat) || (BonusArea != 0 && Cheat_CheckFlags(cheat, 0x200000) != 0)) {
            MenuSFX = GameAudio_GetSfxId(0x32);
        } else {
            Cheat_SetOn(cheat, Cheat_IsOn(cheat) == 0, 1);
            MenuSFX = GameAudio_GetSfxId(0x30);
        }
    }
    updateextras_current_y = SeekValF(updateextras_current_y, static_cast<f32>(menu->selected_item), 5.0f);
}

void MenuUpdateNoData(MENU_s *menu) {
    if (menu->confirm_pressed != 0 || menu->cancel_pressed != 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        BackupMenuNoFn();
        BackupMenu();
    }
}

void MenuUpdateSaving(MENU_s *) {
    if (memcard_saveneeded != 0) {
        memcard_savemessage_delay = 1.0f;
        memcard_saveresult_delay = 1.0f;
        if (memcard_cardchanged != 0 || saveload_autosave == -1) {
            memcard_saveneeded = 0;
            memcard_savefailed = 1;
        }
        return;
    }

    if (memcard_savestarted != 0) {
        memcard_saveresult_delay = 1.0f;
        return;
    }
    if (memcard_savemessage_delay > 0.0f || memcard_saveresult_delay > 0.0f) {
        return;
    }

    if (memcard_savefailed == 0 && memcard_autosave != 0 && saveload_autosave != -1) {
        BackupMenuNoFn();
        BackupMenuNoFn();
        memcard_autosaveenabled = 1;
        memcard_autosavedisabled = 0;
        return;
    }
    BackupMenuNoFn();
    BackupMenu();
}

void MenuDrawBonusMode(MENU_s *) {
}


void MenuUpdateLoading(MENU_s *) {
    if (memcard_loadneeded != 0) {
        memcard_loadmessage_delay = 1.0f;
        memcard_loadresult_delay = 1.0f;
        if (memcard_cardchanged != 0 || saveload_autosave == -1) {
            memcard_loadneeded = 0;
            memcard_loadfailed = 1;
        }
        return;
    }

    if (memcard_loadstarted != 0) {
        memcard_loadresult_delay = 1.0f;
        return;
    }
    if (memcard_loadmessage_delay > 0.0f || memcard_loadresult_delay > 0.0f) {
        return;
    }

    if (memcard_loadfailed == 0 && memcard_loadcorrupt == 0 && (memcard_autosave == 0 || saveload_autosave == -1)) {
        BackupMenuNoFn();
        BackupMenu();
        return;
    }
    if (memcard_loadfailed == 0 && memcard_loadcorrupt == 0) {
        BackupMenuNoFn();
        BackupMenuNoFn();
        memcard_autosaveenabled = 1;
        memcard_autosavedisabled = 0;
        return;
    }
    BackupMenu();
}

void MenuUpdateOptions(MENU_s *menu) {
    GameSetSoundVolume(&TempOptions);
    GameSetMusicVolume(&TempOptions);

    if (menu->cancel_pressed != 0) {
        MenuSFX = GameAudio_GetSfxId(0x31);
        BackupMenu();
        return;
    }

    if (menu->selected_item == 2) {
        if (menu->left_pressed != 0 && TempOptions.field5_0x5 > 0) {
            --TempOptions.field5_0x5;
        } else if (menu->right_pressed != 0 && TempOptions.field5_0x5 < 10) {
            ++TempOptions.field5_0x5;
        }
        menu->selected_item_column = TempOptions.field5_0x5;
    }

    if (menu->confirm_pressed == 0) {
        return;
    }

    MenuSFX = GameAudio_GetSfxId(0x30);
    const i32 accept_row = GAMEDEMO != 0 ? 4 : 5;
    if (menu->selected_item == accept_row) {
        MenuSFX = GameAudio_GetSfxId(memcmp(&TempOptions, &Game.options_save, sizeof(TempOptions)) == 0 ? 0x31 : 0x30);
        Game.options_save = TempOptions;
        BackupMenu();
        SfxCheckMusicOnOff(&Game.options_save);
        return;
    }

    switch (menu->selected_item) {
        case 0:
            if (TestForController() != 0) {
                MenuSFX = GameAudio_GetSfxId(0x31);
            } else {
                SuperOptions.touch_controls = SuperOptions.touch_controls == 0;
                MechSystems::Get()->input_touch_system.control_mode = SuperOptions.touch_controls == 0 ? 1 : 2;
            }
            break;
        case 1:
            TempOptions.field2_0x2 = TempOptions.field2_0x2 == 0;
            break;
        case 3:
            TempOptions.field6_0x6 = TempOptions.field6_0x6 == 0;
            break;
        case 4:
            TempOptions.field11_0xb = TempOptions.field11_0xb == 0;
            break;
        default:
            break;
    }
}

NUGSCN *IconScene_FindById(i32 character_id) {
    if (character_id < 0 || character_id >= CHARCOUNT) {
        return NULL;
    }
    return IconScene[character_id];
}

void MenuDrawDebugStore(MENU_s *) {
}

void MenuDrawEndMission(MENU_s *) {
}

void MenuDrawFormatting(MENU_s *) {
}

void MenuDrawInsertCard(MENU_s *) {
}

void MenuDrawLoadCancel(MENU_s *menu) {
    NuStrCpy(MenuHeader, apitxt_LOADGAME);
    MenuText3DEx(apitxt_DOYOUWANTTOABORTLOAD, 0.0f, -0.3f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                 MENUNORMALR, MENUNORMALG, MENUNORMALB, MenuA);
    menu->draw_y = MENUBOTY - MENUDY;
    DrawMenuEntry(menu, apitxt_YES);
    DrawMenuEntry(menu, apitxt_NO);
}

void MenuDrawSaveCancel(MENU_s *menu) {
    NuStrCpy(MenuHeader, apitxt_SAVEGAME);
    MenuText3DEx(apitxt_DOYOUWANTTOABORT, 0.0f, -0.3f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                 MENUNORMALR, MENUNORMALG, MENUNORMALB, MenuA);
    menu->draw_y = MENUBOTY - MENUDY;
    DrawMenuEntry(menu, apitxt_YES);
    DrawMenuEntry(menu, apitxt_NO);
}

void MenuDrawSelectMode(MENU_s *menu) {
    const i32 area = LDataList[hub_new_level].area_index;
    const bool free_play_available = MenuAreaAllowsFreePlay(area);

    if (area >= 0 && area < AREACOUNT && ADataList[area].name_id >= 0) {
        NuStrCpy(MenuHeader, TTab[ADataList[area].name_id]);
    }

    GameDrawMenuEntry(menu, TTab[free_play_available ? tREPLAYSTORY : tSTORY]);
    if (free_play_available) {
        GameDrawMenuEntry(menu, TTab[tFREEPLAY]);
    } else {
        DrawMenuEntryEx(menu, TTab[tFREEPLAY], MenuA / 2);
    }
}

void MenuInitSelectMode(MENU_s *menu) {
    const i32 area = LDataList[hub_new_level].area_index;
    hub_selectmode = MenuAreaAllowsFreePlay(area) ? 1 : 0;
    menu->selected_row = static_cast<i16>(hub_selectmode);
    menu->selected_item = hub_selectmode;
    selectmodemode = 0;
}

void MenuUpdateDeleting(MENU_s *) {
}

void MenuUpdateEpisodes(MENU_s *) {
}

void MenuUpdateFreePlay(MENU_s *) {
    Hub_UpdateFreePlaySelect();
}

void MenuUpdateMissions(MENU_s *menu) {
    const i32 previous = hub_mission;
    i32 start = 0, cancel = 0, left = 0, right = 0;
    if (MissionSys != NULL) {
        if (MenuPacket.active_player[0]) {
            const u32 pressed = GamePad[0].buttons_pressed;
            if (pressed & GAMEPAD_MENUSELECT) {
                if (!MenuPacket.active_player[1]) {
                    start = 1;
                    goto collected_input;
                }
                if (!MenuPacket.reserved_1[0]) {
                    MenuPacket.reserved_1[0] = 1;
                    if (MenuPacket.reserved_1[1]) {
                        start = 1;
                        goto collected_input;
                    }
                    GameAudio_PlaySfx(0x30, NULL, 0, 0);
                }
            } else if (pressed & GAMEPAD_MENUCANCEL) {
                if (!MenuPacket.active_player[1] || !MenuPacket.reserved_1[0]) {
                    cancel = 1;
                    goto collected_input;
                }
                MenuPacket.reserved_1[0] = 0;
                GameAudio_PlaySfx(0x31, NULL, 0, 0);
            } else if (MenuPacket.reserved_1[0] == 0 && MenuPacket.reserved_1[1] == 0) {
                const u32 directions = GamePad[0].unknown_10;
                if ((directions & GAMEPAD_DLEFT) || (pressed & (GAMEPAD_DLEFT | GAMEPAD_TOGGLELEFT)))
                    left = 1;
                else if ((directions & GAMEPAD_DRIGHT) || (pressed & (GAMEPAD_DRIGHT | GAMEPAD_TOGGLERIGHT)))
                    right = 1;
            }
        }
        if (MenuPacket.active_player[1]) {
            const u32 pressed = GamePad[1].buttons_pressed;
            if (pressed & GAMEPAD_MENUSELECT) {
                if (!MenuPacket.active_player[0]) {
                    start = 1;
                    goto collected_input;
                }
                if (!MenuPacket.reserved_1[1]) {
                    MenuPacket.reserved_1[1] = 1;
                    if (MenuPacket.reserved_1[0]) {
                        start = 1;
                        goto collected_input;
                    }
                    GameAudio_PlaySfx(0x30, NULL, 0, 0);
                }
            } else if (pressed & GAMEPAD_MENUCANCEL) {
                if (!MenuPacket.active_player[0] || !MenuPacket.reserved_1[1]) {
                    cancel = 1;
                    goto collected_input;
                }
                MenuPacket.reserved_1[1] = 0;
                GameAudio_PlaySfx(0x31, NULL, 0, 0);
            } else if (MenuPacket.reserved_1[0] == 0 && MenuPacket.reserved_1[1] == 0) {
                const u32 directions = GamePad[1].unknown_10;
                if ((directions & GAMEPAD_DLEFT) || (pressed & (GAMEPAD_DLEFT | GAMEPAD_TOGGLELEFT)))
                    left = 1;
                else if ((directions & GAMEPAD_DRIGHT) || (pressed & (GAMEPAD_DRIGHT | GAMEPAD_TOGGLERIGHT)))
                    right = 1;
            }
        }
    } else
        cancel = 1;
collected_input:
    if (menu->input_activity) {
        if (menu->confirm_pressed) {
            if (hub_mission == menu->selected_row)
                start = 1;
            else
                hub_mission = menu->selected_row;
        }
        if (menu->cancel_pressed)
            cancel = 1;
        if (menu->left_pressed) {
            hub_mission -= static_cast<i32>(menu->horizontal_scroll_distance / 0.15f);
            if (hub_mission < 0)
                hub_mission = 0;
        }
        if (menu->right_pressed) {
            hub_mission += static_cast<i32>(menu->horizontal_scroll_distance / 0.15f);
            if (hub_mission >= MissionSys->count)
                hub_mission = MissionSys->count - 1;
        }
    }
    if (start) {
        InitMission(MissionSys, hub_mission);
        GameAudio_PlaySfx(0x30, NULL, 0, 0);
        NextArea_FreePlay = FreePlay = 0;
        NewLData = &LDataList[MissionSys->mission->level];
    } else if (cancel) {
        GameAudio_PlaySfx(0x31, NULL, 0, 0);
        EndMissionsMenu();
    } else if (left) {
        if (hub_mission > 0)
            --hub_mission;
    } else if (right && hub_mission < MissionSys->count - 1)
        ++hub_mission;
    if (hub_mission != previous) {
        GameAudio_PlaySfx(0x2f, NULL, 0, 0);
        MissionsAlpha = 0.0f;
    } else {
        MissionsAlpha += FRAMETIME * 2.0f;
        if (MissionsAlpha > 1.0f)
            MissionsAlpha = 1.0f;
    }
    if (MissionSys != NULL) {
        for (i32 i = 0; i < MissionSys->count; ++i)
            MissionIconScale[i] = SeekValF(MissionIconScale[i], i == hub_mission ? 1.0f : 0.0f, 5.0f);
        for (i32 i = 0; i < MissionSys->count; ++i) {
            f32 x = 0.0f;
            if (i < hub_mission)
                x = -0.035f - (hub_mission - i) * 0.12f;
            else if (i != hub_mission)
                x = 0.035f + (i - hub_mission) * 0.12f;
            MissionIconTargetX[i] = x;
            MissionIconX[i] = SeekValF(MissionIconX[i], x, 5.0f);
        }
    }
}

void MenuDrawCardWarning(MENU_s *) {
}

void MenuDrawFileCorrupt(MENU_s *) {
    char message[1024];
    sprintf(message, apitxt_NODATAAVAILABLE, apiGameName);
    MenuSmartTextEx(message, 0.0f, 0.0f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0, MENUNORMALR, MENUNORMALG,
                    MENUNORMALB, 1.3f, 3, NULL, 0, MenuA);
}

void MenuDrawLoadConfirm(MENU_s *menu) {
    NuStrCpy(MenuHeader, apitxt_LOADGAME);
    MenuText3DEx(apitxt_CONFIRMLOAD, 0.0f, -0.3f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0, MENUNORMALR,
                 MENUNORMALG, MENUNORMALB, MenuA);
    menu->draw_y = MENUBOTY - MENUDY;
    DrawMenuEntry(menu, apitxt_YES);
    DrawMenuEntry(menu, apitxt_NO);
}

void MenuDrawSaveConfirm(MENU_s *menu) {
    NuStrCpy(MenuHeader, apitxt_SAVEGAME);
    header_r = MENUHEADERR;
    header_g = MENUHEADERG;
    header_b = MENUHEADERB;
    if (Menu_Overwriteflag != 0) {
        MenuSmartTextEx(apitxt_CONFIRMOVERWRITE, 0.0f, 0.0f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                        MENUNORMALR, MENUNORMALG, MENUNORMALB, 1.6f, 3, NULL, 0, MenuA);
    } else {
        MenuSmartTextEx(apitxt_CONFIRMSAVE, 0.0f, -0.3f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                        MENUNORMALR, MENUNORMALG, MENUNORMALB, 1.6f, 3, NULL, 0, MenuA);
    }

    menu->draw_y = MENUBOTY - MENUDY;
    DrawMenuEntry(menu, apitxt_YES);
    DrawMenuEntry(menu, apitxt_NO);
}

void MenuEnterHeaderSave(MENU_s *) {
    Menu_InHeaderSave = 1;
    BackupMenuNoFn();
    NewMenu(1000, -1, -1);
}

void MenuEnterInsertCard(MENU_s *) {
}

void MenuExitCardWarning(MENU_s *) {
    Menu_InWarningFlow = 0;
}

void MenuUpdateBonusMode(MENU_s *) {
}

void MenuDrawEndChallenge(MENU_s *) {
}

void MenuDrawFormatCancel(MENU_s *) {
}

void MenuDrawNoMemoryCard(MENU_s *menu) {
    Draw_NOMEMORYCARD();
    menu->draw_y = MENUBOTY - MENUDY;
    DrawMenuEntry(menu, apitxt_RETRY);
    DrawMenuEntry(menu, apitxt_SLOT);
}

void MenuDrawStoreHolding(MENU_s *) {
}

void MenuEnterCardWarning(MENU_s *) {
}

void MenuEnterSaveConfirm(MENU_s *) {
    const i32 slot = Menu_InHeaderSave != 0 ? SAVESLOTS : memcard_slot;
    Menu_Overwriteflag = saveload_slotused[slot] != 0;
    if (Menu_Overwriteflag == 0) {
        MenuStartSave();
        BackupMenuNoFn();
        NewMenu(1009, 0, -1);
    }
}

void MenuExitStoreHolding(MENU_s *) {
}

void MenuInitStoreHolding(MENU_s *) {
}

void MenuUpdateDebugStore(MENU_s *) {
}

void MenuUpdateEndMission(MENU_s *) {
}

void MenuUpdateFormatting(MENU_s *) {
}

void MenuUpdateInsertCard(MENU_s *) {
}

void MenuUpdateLoadCancel(MENU_s *menu) {
    if (menu->confirm_pressed != 0) {
        if (menu->selected_row == 0) {
            BackupMenuNoFn();
        }
        BackupMenu();
        MenuSFX = MENUSFX_MENUSELECT;
    } else if (menu->cancel_pressed != 0) {
        BackupMenu();
        MenuSFX = MENUSFX_MENUSELECT;
    }
}

void MenuUpdateSaveCancel(MENU_s *menu) {
    if (menu->confirm_pressed != 0) {
        if (menu->selected_row == 0) {
            BackupMenuNoFn();
        }
        BackupMenu();
        MenuSFX = MENUSFX_MENUSELECT;
    } else if (menu->cancel_pressed != 0) {
        BackupMenu();
        MenuSFX = MENUSFX_MENUSELECT;
    }
}

void MenuDrawDeleteConfirm(MENU_s *) {
}

void MenuDrawFormatConfirm(MENU_s *) {
}

void MenuDrawStorePurchase(MENU_s *) {
}

void MenuEnterNoMemoryCard(MENU_s *) {
}

void MenuEnterStartNewGame(MENU_s *) {
    memcard_autosaveenabled = 0;
    saveload_autosave = -1;
    BackupMenu();
}

void MenuExitStorePurchase(MENU_s *) {
}

void MenuInitStorePurchase(MENU_s *) {
}

void MenuUpdateCardWarning(MENU_s *) {
}

void MenuUpdateFileCorrupt(MENU_s *) {
}

void MenuUpdateLoadConfirm(MENU_s *menu) {
    if (menu->confirm_pressed != 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        if (menu->selected_row == 1) {
            BackupMenu();
        } else {
            MenuStartLoad();
            BackupMenuNoFn();
            NewMenu(1014, 0, -1);
        }
    } else if (menu->cancel_pressed != 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        BackupMenu();
    }
}

void MenuUpdateSaveConfirm(MENU_s *menu) {
    if (menu->confirm_pressed != 0) {
        MenuSFX = MENUSFX_MENUSELECT;
        if (menu->selected_row == 1) {
            BackupMenu();
        } else {
            MenuStartSave();
            BackupMenuNoFn();
            NewMenu(1009, 0, -1);
        }
    } else if (menu->cancel_pressed != 0) {
        BackupMenu();
        MenuSFX = MENUSFX_MENUSELECT;
    }
}

void MenuDrawAutoSaveCancel(MENU_s *) {
}

void MenuDrawNotEnoughSpace(MENU_s *) {
    Draw_NOTENOUGHSPACE();
    Draw_SPACENEEDED();
}

void MenuDrawRestoreNewGame(MENU_s *menu) {
    menu->draw_y = -0.25f;
    GameDrawMenuEntry(menu, TTab[tCONTINUE]);
    GameDrawMenuEntry(menu, TTab[tCANCEL]);
}

void MenuDrawSelectControls(MENU_s *menu) {
    MenuSmartTextEx(TTab[tSELECTCONTROLS], 0.0f, 0.5f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                    MENUNORMALR, MENUNORMALG, MENUNORMALB, 1.4f, 2, NULL, 0, MenuA);
    MenuSmartTextEx(TTab[tCONTROLSCANBECHANGED], 0.0f, -0.5f, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                    MENUNORMALR, MENUNORMALG, MENUNORMALB, 1.4f, 2, NULL, 0, MenuA);
    DrawMenuEntry(menu, TTab[tCLASSIC]);
    DrawMenuEntry(menu, TTab[tTOUCHSCREEN]);
}

void MenuDrawStoreRestoring(MENU_s *) {
}

void MenuExitStoreRestoring(MENU_s *) {
}

void MenuInitStoreRestoring(MENU_s *) {
}

void MenuUpdateEndChallenge(MENU_s *) {
}

void MenuUpdateFormatCancel(MENU_s *) {
}

void MenuUpdateNoMemoryCard(MENU_s *) {
}

void MenuUpdateStoreHolding(MENU_s *) {
}

void MenuDrawAutoSaveWarning(MENU_s *) {
}

void MenuDrawDoNotRemoveCard(MENU_s *) {
}

void MenuDrawViewTextStrings(MENU_s *) {
}

void MenuEnterAutoSaveCancel(MENU_s *) {
}

void MenuUpdateDeleteConfirm(MENU_s *) {
}

void MenuUpdateFormatConfirm(MENU_s *) {
}

void MenuUpdateStorePurchase(MENU_s *) {
}

void MenuEnterAutoSaveWarning(MENU_s *) {
}

void MenuUpdateAutoSaveCancel(MENU_s *) {
}

void MenuUpdateNotEnoughSpace(MENU_s *) {
}

void MenuUpdateRestoreNewGame(MENU_s *menu) {
    const i32 initiated = startnewgame_initiated;
    startnewgame = 0;

    if (initiated != 0 || saveload_autosave != -1) {
        startnewgame_initiated = 0;
        startnewgame = 0;
        const i32 save_occurred = MenuSaveOccurred;
        NewGame();
        MenuSaveOccurred = save_occurred;
        NewLData = NEWGAME_LDATA;
        UsePlayerList = 2;
        newgamecam = 1;
        newgamecamtime = 0.0f;
        return;
    }

    if (menu->confirm_pressed == 0) {
        return;
    }

    if (menu->selected_item == 0) {
        MenuSFX = GameAudio_GetSfxId(0x30);
        NuIOS_RecordFlurryEvent("mainmenu_restore");
        NuIOS_RestoreInAppPurchases();
        NewMenu(22, -1, -1);
    } else if (menu->selected_item == 1) {
        MenuSFX = GameAudio_GetSfxId(0x30);
        if (PlayerProgress[0].active == 0 && PlayerProgress[1].active == 0) {
            PlayerProgress[0].active = 1;
        }
        NewMenu(1000, -1, -1);
    } else if (menu->selected_item == 2) {
        BackupMenu();
    }

    if (startnewgame != 0) {
        startnewgame = 0;
        const i32 save_occurred = MenuSaveOccurred;
        NewGame();
        MenuSaveOccurred = save_occurred;
        NewLData = NEWGAME_LDATA;
        UsePlayerList = 2;
        newgamecam = 1;
        newgamecamtime = 0.0f;
    }
}

void MenuUpdateSelectControls(MENU_s *menu) {
    if (menu->confirm_pressed != 0) {
        MenuSFX = GameAudio_GetSfxId(0x30);
        SuperOptions.touch_controls = menu->selected_row != 0;
        MechSystems::Get()->input_touch_system.control_mode = SuperOptions.touch_controls != 0 ? 2 : 1;
        TriggerExtraDataSave();
        BackupMenuNoFn();
        NewMenu(1000, -1, -1);
    } else if (menu->cancel_pressed != 0) {
        BackupMenu();
    }
}

void MenuUpdateStoreRestoring(MENU_s *) {
}

void MenuUpdateAutoSaveWarning(MENU_s *) {
}

void MenuUpdateDoNotRemoveCard(MENU_s *) {
}

void MenuUpdateViewTextStrings(MENU_s *) {
}

extern "C" {

    f32 MENUDY = -0.1066400036f;
    f32 MENUBOTY = -0.75f;
    f32 MENUTOPY = 0.8f;
    f32 dme_sx = 1.0f;
    f32 dme_sy = 1.0f;
    f32 MENUENTRYEXWIDTH = 1.6f;
    u8 MENUFLASH1R = 0x7f;
    u8 MENUFLASH1G;
    u8 MENUFLASH1B = 0xff;
    u8 MENUFLASH0R;
    u8 MENUFLASH0G = 0xff;
    u8 MENUFLASH0B = 0xff;
    u8 MENUWARNR = 0xff;
    u8 MENUWARNG;
    u8 MENUWARNB;
    u8 MENUENTRYR;
    u8 MENUENTRYG = 0x7f;
    u8 MENUENTRYB = 0xff;
    u8 MENUNORMALR = 0xff;
    u8 MENUNORMALG = 0x7f;
    u8 MENUNORMALB;
    u8 dme_r;
    u8 dme_g;
    u8 dme_b;
    i32 dme_align;
    i32 dme_rgb;
    f32 menu_pulse;
    f32 menu_pulse_speed;
    i32 menu_pulse_angle;
    f32 menu_pulsate;
    f32 menu_pulsate_speed;
    i32 menu_pulsate_angle;
    void (*headerdrawfn)(void) = DrawMenuHeader;
    void (*predrawfn)(MENU *) = NULL;

    static u8 MenuColourLerp(u8 first, u8 second, f32 amount) {
        return static_cast<u8>(
            static_cast<i32>(static_cast<f32>(first) * amount + static_cast<f32>(second) * (1.0f - amount)));
    }

    void BackupMenu(void) {
        if (GameMenuLevel == 0) {
            return;
        }

        MENU *menu = &GameMenu[GameMenuLevel];
        const i16 menu_index = menu->menu;
        if (menu_index != -1 && MenuInfo[menu_index].exit_fn != NULL) {
            MenuInfo[menu_index].exit_fn(menu);
        }
        menu->menu = -1;

        --GameMenuLevel;
        menu = &GameMenu[GameMenuLevel];
        menu->menu_time = 0.0f;
        if (menu->menu != -1 && MenuInfo[menu->menu].enter_fn != NULL) {
            MenuInfo[menu->menu].enter_fn(menu);
        }
    }

    void BackupMenuNoFn(void) {
        if (GameMenuLevel == 0) {
            return;
        }

        MENU *menu = &GameMenu[GameMenuLevel];
        const i16 menu_index = menu->menu;
        if (menu_index != -1 && MenuInfo[menu_index].exit_fn != NULL) {
            MenuInfo[menu_index].exit_fn(menu);
        }
        MenuRememberCursor(menu);
        menu->field_b4 = 0;
        menu->field_b8 = 1;
        menu->field_bc = 0;
        menu->menu = -1;

        --GameMenuLevel;
        MENU *parent = &GameMenu[GameMenuLevel];
        parent->menu_time = 0.0f;
        parent->field_b8 = 1;
        parent->field_b4 = 1;
        parent->field_b0 = 0;
        parent->field_c0 = 0;
        if (parent->menu != -1) {
            MenuAlpha = 0.0f;
            MenuA = 0;
            MenuValidated = 0;
        }
    }

    void CreateColourPicker(void) {
    }

    void CreateTestMenu(void) {
    }

    void DrawMenu(i32 paused) {
        if (memcard_autosavestarted != 0 || memcard_autosavepostdelay > 0.0f || memcard_autosavepredelay > 0.0f) {
            if (memcard_drawasiconfn != NULL) {
                memcard_drawasiconfn();
            }
            return;
        }

        MENU *menu = &GameMenu[GameMenuLevel];
        i16 menu_index = menu->menu;
        if (menu_index == -1)
            return;

        MenuValidated = 1;
        f32 saved_button_baseline;
        f32 saved_baseline;
        if (SmartTextFont != NULL) {
            saved_baseline = SmartTextFont->baseline;
            SmartTextFont->baseline = 0.0f;
        } else if (QFont2D != NULL) {
            saved_baseline = QFont2D->baseline;
            QFont2D->baseline = 0.0f;
        } else {
            saved_baseline = 0.0f;
        }

        if (QFont2DButtons != NULL) {
            saved_button_baseline = QFont2DButtons->baseline;
            QFont2DButtons->baseline = 0.0f;
        } else {
            saved_button_baseline = 0.0f;
        }

        MenuHeader[0] = '\0';
        menu->draw_item = menu->first_row;
        menu->item_scale = 1.0f;
        menu->draw_x = 0.0f;
        menu->centre_offset = static_cast<f32>(menu->last_row - menu->first_row) * MENUDY * 0.5f;
        menu->draw_y = -menu->centre_offset;
        menu->draw_z = 1.0f;
        menu->field_a0 = 0.0f;
        menu->paused = paused;
        if (predrawfn != NULL) {
            predrawfn(menu);
            menu_index = menu->menu;
        }
        if (MenuInfo[menu_index].draw_fn != NULL)
            MenuInfo[menu_index].draw_fn(menu);
        if (headerdrawfn != NULL)
            headerdrawfn();
        if (MenuAlpha >= 0.5f && g_enableButtonPrompts != 0) {
            const MENUFNINFO &info = MenuInfo[menu->menu];
            DrawMenuButtonPrompts(info.confirm_prompt, info.cancel_prompt, 1, MENUNORMALR, MENUNORMALG, MENUNORMALB,
                                  static_cast<u8>(MenuA));
        }

        if (SmartTextFont != NULL) {
            SmartTextFont->baseline = saved_baseline;
        } else if (QFont2D != NULL) {
            QFont2D->baseline = saved_baseline;
        }
        if (QFont2DButtons != NULL) {
            QFont2DButtons->baseline = saved_button_baseline;
        }
    }

    void DrawMenuBottomMessage(void) {
    }

    void DrawMenuButtonPrompts(i32 confirm_prompt, i32 cancel_prompt, i32 enabled, u8 red, u8 green, u8 blue,
                               u8 alpha) {
        DrawMenuButtonPromptsEx(confirm_prompt, cancel_prompt, 0, enabled, red, green, blue, alpha);
    }

    void DrawMenuButtonPromptsEx(i32, i32, i32, i32, u8, u8, u8, u8) {
    }

    void DrawMenuEntry(MENU *menu, char *text) {
        DrawMenuEntryEx(menu, text, static_cast<u8>(MenuA));
    }

    void DrawMenuEntryEx(MENU *menu, char *text, i32 alpha) {
        i32 draw_alpha = -128;
        f32 scale = menu->item_scale_x;
        if (static_cast<u8>(alpha) <= 128) {
            draw_alpha = alpha;
        }
        if (scale == 0.0f) {
            scale = MENUTEXTSCALE;
        }
        f32 spacing = menu->item_spacing;
        const f32 y_scale = dme_sy;
        scale *= y_scale;
        if (spacing == 0.0f) {
            spacing = MENUDY;
        }
        spacing *= y_scale;
        u8 red;
        u8 green;
        u8 blue;

        if (menu->item_offsets != NULL) {
            Text3DEx2(text, menu->draw_x + menu->item_offsets[0] * menu->item_offset_scale,
                      menu->draw_y + menu->item_offsets[1] * menu->item_offset_scale, menu->draw_z, scale * dme_sx,
                      scale, scale, dme_align, 0, 0, 0, static_cast<u8>(draw_alpha) >> 2);
        }

        if (menu->draw_item >= menu->first_row && menu->draw_item <= menu->last_row) {
            if (menu->draw_item == menu->selected_row && TestForController() != 0) {
                if (TestForController() == 0) {
                    if (menu_pulse > 0.0f) {
                        red = MenuColourLerp(MENUFLASH0R, MENUNORMALR, menu_pulse);
                        green = MenuColourLerp(MENUFLASH0G, MENUNORMALG, menu_pulse);
                        blue = MenuColourLerp(MENUFLASH0B, MENUNORMALB, menu_pulse);
                    } else {
                        red = MENUENTRYR;
                        green = MENUENTRYG;
                        blue = MENUENTRYB;
                    }
                } else if (menu_pulsate > 0.0f) {
                    red = MenuColourLerp(MENUFLASH0R, MENUFLASH1R, menu_pulsate);
                    green = MenuColourLerp(MENUFLASH0G, MENUFLASH1G, menu_pulsate);
                    blue = MenuColourLerp(MENUFLASH0B, MENUFLASH1B, menu_pulsate);
                } else if (menu_flash != 0) {
                    red = MENUFLASH0R;
                    green = MENUFLASH0G;
                    blue = MENUFLASH0B;
                } else {
                    red = MENUFLASH1R;
                    green = MENUFLASH1G;
                    blue = MENUFLASH1B;
                }
            } else if (dme_rgb != 0) {
                red = dme_r;
                green = dme_g;
                blue = dme_b;
            } else if (menu_pulse > 0.0f) {
                red = MenuColourLerp(MENUFLASH0R, MENUNORMALR, menu_pulse);
                green = MenuColourLerp(MENUFLASH0G, MENUNORMALG, menu_pulse);
                blue = MenuColourLerp(MENUFLASH0B, MENUNORMALB, menu_pulse);
            } else {
                red = MENUENTRYR;
                green = MENUENTRYG;
                blue = MENUENTRYB;
            }

            if (menu->item_offsets != NULL) {
                Text3DEx(text, menu->draw_x, menu->draw_y, menu->draw_z, scale * dme_sx, scale, scale, dme_align, red,
                         green, blue, draw_alpha);
            } else {
                smarttextex_drawmessagebox = 1;
                MenuSmartTextEx(text, menu->draw_x, menu->draw_y, menu->draw_z, scale * dme_sx, scale, scale,
                                static_cast<u32>(dme_align), red, green, blue, MENUENTRYEXWIDTH, 1, NULL, 0,
                                draw_alpha);
            }

            const i32 item = menu->draw_item;
            menu->item_x[item] = menu->draw_x;
            menu->item_y[item] = menu->draw_y;
            menu->item_width[item] = text3d_width;
            menu->item_height[item] = text3d_height;
            menu->item_column[item] = 0;
            menu->item_row[item] = item;
        }
        ++menu->draw_item;
        dme_sx = 1.0f;
        dme_sy = 1.0f;
        dme_align = 0;
        dme_rgb = 0;
        menu->draw_y += spacing;
    }

    void DrawMenuHeader(void) {
        if (MenuDisableHeaders == 0) {
            MenuSmartTextEx(MenuHeader, 0.0f, MENUTOPY, 1.0f, MENUTEXTSCALE, MENUTEXTSCALE, MENUTEXTSCALE, 0,
                            static_cast<u8>(header_r), static_cast<u8>(header_g), static_cast<u8>(header_b), 1.2f, 1,
                            NULL, 0, MenuA);
        }
    }

    void DrawMenuHeaderMessage(void) {
    }

    void DrawMenuTopMessage(void) {
    }

    void Draw_CANCEL(MENU *menu) {
        extern char *apitxt_CANCEL;
        menu->draw_y = MENUBOTY;
        DrawMenuEntry(menu, apitxt_CANCEL);
    }

    void Draw_CHECKINGMEMORYCARD(void) {
    }

    void Draw_DONOTREMOVEMEMORYCARD(void) {
    }

    void Draw_NOTENOUGHSPACE(void) {
    }

    void Draw_SPACENEEDED(void) {
    }

    void FileSelKill(void) {
    }

    void FlushMenuHighlights(void) {
    }

    i32 MenuCurrentID(void) {
        return MenuInfo[GameMenu[GameMenuLevel].menu].id;
    }

    void MenuDrawBackground(void) {
    }

    i32 MenuGetSlotNum(void) {
        return memcard_slot;
    }

    void MenuInCriticalMemoryCard(void) {
    }

    i32 MenuInMemoryCard(void) {
        if (GameMenuLevel == -1 || MenuValidated == 0)
            return 0;

        const i32 menu_id = MenuInfo[GameMenu[GameMenuLevel].menu].id;
        return (u32)(menu_id - LEGO_MENU_MEMORY_CARD_FIRST) <=
               (LEGO_MENU_MEMORY_CARD_LAST - LEGO_MENU_MEMORY_CARD_FIRST);
    }

    i32 MenuInMemoryCardLoad(void) {
        if (MenuValidated == 0) {
            return 0;
        }
        return Menu_InLoadFlow;
    }

    i32 MenuInMemoryCardWarning(void) {
        if (MenuValidated == 0) {
            return 0;
        }
        return Menu_InWarningFlow;
    }

    void MenuRegisterSoundFX(i32 move, i32 select, i32 back, i32 no_entry) {
        MENUSFX_MENUMOVE = move;
        MENUSFX_MENUSELECT = select;
        MENUSFX_MENUBACK = back;
        MENUSFX_MENUNOENTRY = no_entry;
    }

    void MenuRememberCursor(MENU *menu) {
        if (menu->menu == -1)
            return;

        MENUFNINFO &info = MenuInfo[menu->menu];
        if (info.memory_x != -1)
            info.memory_x = menu->selected_column;
        if (info.memory_y != -1)
            info.memory_y = menu->selected_row;
    }

    void MenuRepeat(i32 *held, i32 *repeat, f32 *repeat_time, u8 *repeat_count, f32 interval, f32 elapsed) {
        if (*held == 0) {
            *repeat_time = 0.5f;
            *repeat_count = 0;
            return;
        }

        if (*repeat_time > 0.0f) {
            *repeat_time -= elapsed;
            if (*repeat_time > 0.0f)
                return;

            *repeat = 1;
            *repeat_time = interval;
            ++*repeat_count;
        }
    }

    void MenuSetColours(u8 normal_r, u8 normal_g, u8 normal_b, u8 entry_r, u8 entry_g, u8 entry_b, u8 header_r,
                        u8 header_g, u8 header_b, u8 warn_r, u8 warn_g, u8 warn_b, u8 flash0_r, u8 flash0_g,
                        u8 flash0_b, u8 flash1_r, u8 flash1_g, u8 flash1_b) {
        MENUNORMALR = normal_r;
        MENUNORMALG = normal_g;
        MENUNORMALB = normal_b;
        MENUENTRYR = entry_r;
        MENUENTRYG = entry_g;
        MENUENTRYB = entry_b;
        MENUHEADERR = header_r;
        MENUHEADERG = header_g;
        MENUHEADERB = header_b;
        MENUWARNR = warn_r;
        MENUWARNG = warn_g;
        MENUWARNB = warn_b;
        MENUFLASH0R = flash0_r;
        MENUFLASH0G = flash0_g;
        MENUFLASH0B = flash0_b;
        MENUFLASH1R = flash1_r;
        MENUFLASH1G = flash1_g;
        MENUFLASH1B = flash1_b;
    }

    void MenuSetHeaderDrawFn(void (*draw_fn)(void)) {
        headerdrawfn = draw_fn;
    }

    void MenuSetPreDrawFn(void (*draw_fn)(MENU *)) {
        predrawfn = draw_fn;
    }

    void MenuSetPulsateSpeed(f32 speed) {
        menu_pulsate_speed = speed;
    }

    void MenuSetTopBottom(void) {
    }

    void MessageBoxInitMtl(void) {
    }

    void PetesHackOfDeath(void) {
    }

    void ProcessFileSel2(void) {
    }

    void RemapAddr(void *new_base, void *old_base, void **address) {
        *address = static_cast<u8 *>(new_base) + (static_cast<u8 *>(*address) - static_cast<u8 *>(old_base));
    }

    void RenderFileSel2(void) {
    }

    void SetButtonScaleMode(i32 mode) {
        ButtonScaleMode = mode;
    }

    void StartFileSel(void) {
    }

    void TestMenu(void) {
    }

    i32 UpdateMenu(u32 primary_held, u32 primary_pressed, u32 alternate_held, u32 alternate_pressed, f32 elapsed,
                   u32 confirm_mask, u32 cancel_mask, u32 start_mask, u32 select_mask) {
        MenuResult = 0;
        if (MenuValidated == 0)
            return MenuResult;

        MENU *menu = &GameMenu[GameMenuLevel];
        if (MenuFadeEnabled == 0) {
            MenuAlpha = 1.0f;
        } else if (MenuAlpha < 1.0f) {
            MenuAlpha += elapsed + elapsed;
            if (MenuAlpha > 1.0f)
                MenuAlpha = 1.0f;
        }

        const i32 alpha_angle = static_cast<i32>(MenuAlpha * 16384.0f);
        MenuA = static_cast<i32>(NuTrigTable[(alpha_angle >> 1) & 0x7fff] * 128.0f);

        if (menu_pulsate_speed > 0.0f) {
            menu_pulsate_angle += static_cast<i32>(elapsed * 65536.0f * menu_pulsate_speed);
            menu_pulsate = (NuTrigTable[(menu_pulsate_angle >> 1) & 0x7fff] + 1.0f) * 0.5f;
        } else {
            menu_pulsate = 0.0f;
        }
        if (menu_pulse_speed > 0.0f && TestForController() == 0) {
            menu_pulse_angle += static_cast<i32>(elapsed * 65536.0f * menu_pulse_speed);
            menu_pulse = (NuTrigTable[(menu_pulse_angle >> 1) & 0x7fff] - 0.75f) * 4.0f;
        } else {
            menu_pulse = 0.0f;
        }

        MenuSFX = -1;
        if (sfx_wait > 0.0f)
            sfx_wait -= elapsed;
        if (menu->menu == -1)
            return MenuResult;

        const u32 directions = 0xf000;
        u32 held = primary_held | alternate_held;
        if ((primary_held & directions) != 0)
            held = primary_held;
        u32 pressed = primary_pressed | alternate_pressed;
        if ((primary_pressed & directions) != 0)
            pressed = primary_pressed;

        if (MenuAlpha < 0.5f) {
            held = 0;
            pressed = 0;
        }

        i32 up_pressed = (pressed & 0x1000) != 0;
        i32 down_pressed = (pressed & 0x4000) != 0;
        i32 left_pressed = (pressed & 0x8000) != 0;
        i32 right_pressed = (pressed & 0x2000) != 0;
        i32 up_held = (held & 0x1000) != 0;
        i32 down_held = (held & 0x4000) != 0;
        i32 left_held = (held & 0x8000) != 0;
        i32 right_held = (held & 0x2000) != 0;

        if (up_pressed != 0 && down_pressed != 0)
            up_pressed = down_pressed = 0;
        if (left_pressed != 0 && right_pressed != 0)
            left_pressed = right_pressed = 0;

        menu->input_activity = 0;
        if (menu->move_left != 0) {
            left_pressed = 1;
            menu->move_left = 0;
            menu->input_activity = 1;
        }
        if (menu->move_right != 0) {
            right_pressed = 1;
            menu->move_right = 0;
            menu->input_activity = 1;
        }
        if (menu->move_up != 0) {
            up_pressed = 1;
            menu->move_up = 0;
            menu->input_activity = 1;
        }
        if (menu->move_down != 0) {
            down_pressed = 1;
            menu->move_down = 0;
            menu->input_activity = 1;
        }

        i32 confirm_pressed = (pressed & confirm_mask) != 0;
        i32 cancel_pressed = (pressed & cancel_mask) != 0;
        u32 start_pressed = pressed & start_mask;
        const u32 select_pressed = pressed & select_mask;
        u32 action_04_pressed = pressed & 4;
        u32 action_08_pressed = pressed & 8;
        if (action_04_pressed != 0 && action_08_pressed != 0)
            action_04_pressed = action_08_pressed = 0;
        if (confirm_pressed != 0 && cancel_pressed != 0)
            confirm_pressed = cancel_pressed = 0;
        if (menu->unk == 0.0f)
            confirm_pressed = cancel_pressed = 0;

        if (menu->input_disabled != 0) {
            up_pressed = down_pressed = 0;
            confirm_pressed = cancel_pressed = 0;
            start_pressed = 0;
        }

        MenuRepeat(&up_held, &up_pressed, &menu->repeat_up_time, &menu->repeat_up_count, 0.1f, elapsed);
        MenuRepeat(&down_held, &down_pressed, &menu->repeat_down_time, &menu->repeat_down_count, 0.1f, elapsed);
        MenuRepeat(&left_held, &left_pressed, &menu->repeat_left_time, &menu->repeat_left_count, 0.1f, elapsed);
        MenuRepeat(&right_held, &right_pressed, &menu->repeat_right_time, &menu->repeat_right_count, 0.1f, elapsed);

        if (menu->transition_duration > menu->transition_time) {
            menu->transition_time += elapsed;
            if (menu->transition_time >= menu->transition_duration) {
                menu->transition_time = menu->transition_duration;
                if (menu->flags_17 != -1)
                    NewMenu(menu->flags_17, -1, -1);
            }
        }

        const i16 old_column = menu->selected_column;
        const i16 old_row = menu->selected_row;
        const MENUFNINFO &info = MenuInfo[menu->menu];
        const bool wrap = info.wrap != 0;

        bool row_clamped = false;
        if (menu->selected_row < menu->first_row) {
            menu->selected_row = menu->first_row;
            row_clamped = true;
        } else if (menu->selected_row > menu->last_row) {
            menu->selected_row = menu->last_row;
            row_clamped = true;
        }

        bool row_moved = false;
        if (down_pressed != 0) {
            if (wrap) {
                ++menu->selected_row;
                if (menu->selected_row > menu->last_row)
                    menu->selected_row = menu->first_row;
                row_moved = true;
            } else if (menu->selected_row < menu->last_row) {
                ++menu->selected_row;
                row_moved = true;
            }
        }
        if (up_pressed != 0) {
            if (wrap) {
                --menu->selected_row;
                if (menu->selected_row < menu->first_row)
                    menu->selected_row = menu->last_row;
                row_moved = true;
            } else if (menu->selected_row > menu->first_row) {
                --menu->selected_row;
                row_moved = true;
            }
        }
        if (menu->selected_row != old_row) {
            menu->unk = 0.0f;
            if (!row_clamped && row_moved)
                MenuSFX = MENUSFX_MENUMOVE;
        }

        const bool column_navigation = menu->state == 1 || (menu->state == 2 && menu->selected_row == 0) ||
                                       (menu->state == 3 && menu->selected_row != menu->last_row);
        if (column_navigation) {
            if (menu->selected_column < menu->first_column)
                menu->selected_column = menu->first_column;
            else if (menu->selected_column > menu->last_column)
                menu->selected_column = menu->last_column;

            if (right_pressed != 0 && menu->selected_column < menu->last_column)
                ++menu->selected_column;
            if (left_pressed != 0 && menu->selected_column > menu->first_column)
                --menu->selected_column;
            if (menu->selected_column != old_column)
                MenuSFX = MENUSFX_MENUMOVE;
        }

        menu->selected_item = menu->selected_row;
        menu->selected_item_column = menu->selected_column;
        menu->previous_item = column_navigation ? old_column : old_row;
        menu->buttons_held = held;
        menu->buttons_pressed = pressed;
        menu->column_count = menu->last_column - menu->first_column + 1;
        menu->row_count = menu->last_row - menu->first_row + 1;
        menu->up_pressed = up_pressed;
        menu->down_pressed = down_pressed;
        menu->left_pressed = left_pressed;
        menu->right_pressed = right_pressed;
        menu->confirm_pressed = confirm_pressed;
        menu->cancel_pressed = cancel_pressed;
        menu->up_held = up_held;
        menu->down_held = down_held;
        menu->left_held = left_held;
        menu->right_held = right_held;
        menu->start_pressed = start_pressed;
        menu->select_pressed = select_pressed;
        menu->action_04_pressed = action_04_pressed;
        menu->action_08_pressed = action_08_pressed;
        menu->menu_time += elapsed;
        menu->unk += elapsed;

        if (menu->queued_item != -1) {
            menu->selected_item = menu->queued_item;
            menu->selected_column = static_cast<i16>(menu->queued_column);
            menu->selected_row = static_cast<i16>(menu->queued_row);
            menu->confirm_pressed = 1;
            menu->queued_item = -1;
            menu->input_activity = 1;
        }
        if (menu->close_requested != 0) {
            menu->cancel_pressed = 1;
            menu->close_requested = 0;
            menu->input_activity = 1;
        }

        if (info.update_fn != NULL)
            info.update_fn(menu);
        if (MenuSFX != -1 && menu->input_disabled == 0) {
            PlaySfxById(MenuSFX, 0);
            MenuSFX = -1;
        }
        return MenuResult;
    }

    void cbCancelSubMenu(void) {
    }

    void cbCancelSubMenuFromItem(void) {
    }

    i32 cbCompateDirentByDateAsc(NUFILE_INFO *first, NUFILE_INFO *second) {
        return first->year - second->year;
    }

    i32 cbCompateDirentByDateDec(NUFILE_INFO *first, NUFILE_INFO *second) {
        return second->year - first->year;
    }

    void cbCompateDirentByNameAsc(void) {
    }

    void cbCompateDirentByNameDec(void) {
    }

    void cbCompateDirentBySizeAsc(void) {
    }

    void cbCompateDirentBySizeDec(void) {
    }

    i32 cbInteractMenuScrollDown(edui_interact_s *interact) {
        eduimenu_s *menu = interact->menu;
        if (menu->field_10) {
            if (menu->field_10->next)
                menu->field_10 = menu->field_10->next;
            menu->selected = menu->field_10;
        }
        return 0;
    }

    i32 cbInteractMenuScrollUp(edui_interact_s *interact) {
        eduimenu_s *menu = interact->menu;
        if (menu->field_0c) {
            if (menu->field_0c->previous)
                menu->field_0c = menu->field_0c->previous;
            menu->selected = menu->field_0c;
        }
        return 0;
    }

    void cbInteractMenuTitle(void) {
    }

    void cbModifierAdjust(void) {
    }

    void cbTriggerSubMenu(void) {
    }

} // extern "C"
