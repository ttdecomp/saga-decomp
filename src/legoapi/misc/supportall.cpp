#include "nu2api/nu3d/nuprim.h"
#include "nu2api/nu3d/nuvport.h"
#include "nu2api/nu3d/android/nuptl_android.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/core/config/cheat.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "decomp.h"
#include "nu2api/nucore/nustring.h"
#include "globals.h"
#include "legoapi/core/input/qrand.h"
#include "gameapi/ai/aisys/aisys.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/world/mission.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/core/render.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/level.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/world/area.h"
#include "legoapi/world/mission.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/world/world.h"
#include "legoapi/world/mission.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "nu2api/nu3d/nuspecial.h"

void Hint_SetHintFromId(i32, i32, i32);
void MakeBaddiesForgetAboutParty(i32);
void ResetRadios();
void SuperCounters_FixUpGizmos(WORLDINFO_s *);
void AITriggerSetSysReset(AITRIGGERSETSYS_s *);
void AITriggerSysAutoSetUp(WORLDINFO_s *, AITRIGGERSETSYS_s *);
void ResetPlayer(GameObject_s *, i32, nuvec_s *, i32);
f32 GetVehicleAreaRememberSpeed();
void CharPlatforms_Reset(CHARPLATFORMSYS_s *);
void SetSoundFadeDist(WORLDINFO_s *, OPTIONSSAVE_s *);
void GameCameraMakeMiniCut(nugspline_s *, f32, f32, f32, f32, i32, i32);
void Cheats_TurnOff(i32);
void CutScene_StartAudio();
void oneAtOnce_SetNumAttackers(i32);
void ResetGizFlow(GIZFLOW_s *, GIZFLOWPROGRESS_s *);
void EffectOffProgress_Reset(LEVEL_PROGRESS_s *);
extern GameObject_s *alert_obj;
extern f32 alert_timer;
extern f32 LevelNameMul, LevelNameTime;
extern rtldata_s lev_rtldata;
extern "C" {
    extern f32 chattersfxwait, tieonsfxwait, tieoffsfxwait;
    extern i32 party_under_cover, nbaddies_can_see_players;
    extern i32 gone_through_door_to_new_level;
    extern i32 FalconDebKey[2];
    extern f32 TargetDist_Near2, TargetDist_Mid2;
    extern u16 TargetDeg_Near, TargetDeg_Mid, TargetDeg_Far;
    extern i32 makebaddiesforgetinresetbits;
    extern i32 reset_reimport;
    extern u32 arcade_placed_stud_total;
}

void CutScenes_Reset(WORLDINFO_s *);
void ClearLevelProgress(i32, WORLDINFO_s *);
void Cheats_Reset(void);
void ResetScene(nugscn_s *, SCENEPROGRESS_s *);
void GizmoBlowupVisibilityOverrides(WORLDINFO_s *);
void SetTexAnimSignals(void);
void Customiser_SetUpCharacterData(CUSTOMISER *);
void Surfaces_Reset(void);
void ResetStreaks(void);
void Bolts_Reset(void);
void Batarangs_Reset(void);
void Detonators_Reset(void);
void ResetExplosions(void);
void ShoveObjectSysReset(void);
void Panel_Clear(void);
void ResetGameMessages(void);
void Tag_ResetTransfers(void);
void Tag_SetMode(i32 mode);
u32 TotalLevelCoinTally(WORLDINFO_s *, u32 *, u32 *, u32 *, u32 *, u32 *, u32 *, u32 *);
void Cheats_TurnOff(i32);
void GameCameraMakeMiniCut(nugspline_s *, f32, f32, f32, f32, i32, i32);
extern i32 bonusmodearcade;
extern i32 gone_through_door_to_new_level;
u32 arcade_placed_stud_total = 0;
extern f32 DEFAULT_MOVE_RANGE;
extern f32 drop_back_in_timer;
extern i32 last_chatter_sfx;
i32 FalconDebKey[2] = {-1, -1};
extern f32 LevelNameTime;
extern f32 LevelNameMul;
extern rtldata_s lev_rtldata;
void Hint_Reset(void);
void Hint_CancelCurrent(void);
void Hint_SetHintFromId(i32, i32, i32);
void Teleports_Reset(WORLDINFO_s *);
void TrafficAnimSys_Reset(TRAFFICANIMSYS_s *);
void Pulses_Reset(PULSESYS_s *);
void ResetRepeatSfx(void);
void ResetRippleSet(ripple_set_s *);
void Grabber_Reset(WORLDINFO_s *);
void Faders_Reset(WORLDINFO_s *);
void InitGameMode(void);
void GameAnimSys_ReStoreProgress(GAMEANIMSYS_s *system, i32 progress_index);
void ResetForceBack(void);
void ClearAICreatures(void);
void ReStoreStatusTakeOverObjectSys(i32 restore_progress);
void InitPlayerAI(GameObject_s *object);
void ResetPlayer(GameObject_s *, i32, nuvec_s *, i32);
f32 GetVehicleAreaRememberSpeed();
void ResetRadios();
void SuperCounters_FixUpGizmos(WORLDINFO_s *);
void AITriggerSetSysReset(AITRIGGERSETSYS_s *);
void AITriggerSysAutoSetUp(WORLDINFO_s *, AITRIGGERSETSYS_s *);
void CharPlatforms_Reset(CHARPLATFORMSYS_s *);
void CutScene_StartAudio();
void oneAtOnce_SetNumAttackers(i32);
void SetSoundFadeDist(WORLDINFO_s *, OPTIONSSAVE_s *);
void ResetGizFlow(GIZFLOW_s *, GIZFLOWPROGRESS_s *);
void EffectOffProgress_Reset(LEVEL_PROGRESS_s *);
extern GAMECAMERA_s *GameCam;
extern ripple_set_s *ripples;
extern "C" void ResetParts(void);
extern "C" void NuSound3StopRumble(void);
extern "C" f32 chattersfxwait;
extern "C" f32 tieonsfxwait;
extern "C" f32 tieoffsfxwait;
extern GameObject_s *alert_obj;
extern f32 alert_timer;
void NuDisplayListCreate(nudisplayscene_s *, variptr_u *, variptr_u, i32, i32, i32, i32, i32, i32, i32);

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

static i32 TBGAMECOUNT;
static i32 TBDRAWCOUNT;
static i32 TBPLAYERCOUNT;
static i32 TBAICOUNT;

f32 TargetDist_Near2;
f32 TargetDist_Mid2;
u16 TargetDeg_Near;
u16 TargetDeg_Mid;
u16 TargetDeg_Far;
extern i32 party_under_cover;
i32 makebaddiesforgetinresetbits;
void MakeBaddiesForgetAboutParty(i32);
extern i32 nbaddies_can_see_players;
i32 reset_reimport;

void CatchUpCode(GameObject_s *, float, float, i32) {
}

struct TexQuadVertex {
    f32 x, y, z;
    u32 colour;
    union {
        f32 uv[2];
        u16 half_uv[4];
    };
};
static inline void TexQuadSubmit(NUVEC const &point, i32 colour, i32 u, i32 v) {
    TexQuadVertex *vertex = static_cast<TexQuadVertex *>(g_NuPrim_StreamBufferPtr->void_ptr);
    if (g_NuPrim_NeedsOverbrightening)
        vertex->colour = colour;
    else
        vertex->colour = ((colour >> 1) & 0x7f7f7f) | (colour & 0xff000000);
    if (g_NuPrim_NeedsHalfUVs) {
        vertex->half_uv[0] = u ? 0x3c00 : 0;
        vertex->half_uv[1] = v ? 0x3c00 : 0;
    } else {
        vertex->uv[0] = static_cast<f32>(u);
        vertex->uv[1] = static_cast<f32>(v);
    }
    NuPrim2DAddXYZ(static_cast<f32>(PS2_VREZ_W) * point.x, static_cast<f32>(PS2_VREZ_H) * point.y, 0.0f);
}
void RndrTexQuad(f32 x, f32 y, f32 width, f32 height, i32 colour, numtl_s *material, i32 angle) {
    NUVEC points[4] = {};
    points[0].x = -0.5f;
    points[0].y = -0.5f;
    points[1].x = 0.5f;
    points[1].y = -0.5f;
    points[2].x = -0.5f;
    points[2].y = 0.5f;
    points[3].x = 0.5f;
    points[3].y = 0.5f;
    NuVecRotateZ(&points[0], &points[0], angle);
    NuVecRotateZ(&points[1], &points[1], angle);
    NuVecRotateZ(&points[2], &points[2], angle);
    NuVecRotateZ(&points[3], &points[3], angle);
    points[0].x *= width;
    points[0].y *= height;
    points[1].x *= width;
    points[1].y *= height;
    points[2].x *= width;
    points[2].y *= height;
    points[3].x *= width;
    points[3].y *= height;
    points[0].x += x;
    points[0].y += y;
    points[1].x += x;
    points[1].y += y;
    points[2].x += x;
    points[2].y += y;
    points[3].x += x;
    points[3].y += y;
    NuPrim2DBegin(1, 7, material);
    TexQuadSubmit(points[0], colour, 0, 0);
    TexQuadSubmit(points[1], colour, 1, 0);
    TexQuadSubmit(points[2], colour, 0, 1);
    TexQuadSubmit(points[3], colour, 1, 1);
    NuPrim2DEnd();
}

i32 SuperWeirdo(GameObject_s *object) {
    if ((object->apiobj.flags_low & 0x80) != 0 && (Game.field_0x7c26[1] & 1) != 0 && CharacterCustomiser != NULL &&
        (object->id == CharacterCustomiser->character_ids[0] || object->id == CharacterCustomiser->character_ids[1])) {
        return 1;
    }
    return 0;
}

void bgProcClose() {
}

void BurnoutApply(i32) {
}

void bgprocFreeze() {
}

extern i32 PDEBCOUNT;
void AddPartDebris(PARTDEBSYS_s *system, i32 index, nuvec_s *position) {
    if (index >= 0 && system != NULL && index < PDEBCOUNT) {
        const i32 type = system->entries[index].type_id;
        if (type != -1) {
            AddFiniteShotPART(type, position, 1);
        }
    }
}

void FindSlamOrigin(GameObject_s *, NUVEC *, NUVEC *);
EXPLOSION *AddExplosion(NUVEC *, f32, f32, GameObject_s *, i32, i32);

void AddSlamDebris(GameObject_s *object) {
    NUVEC position;
    FindSlamOrigin(object, &position, NULL);
    f32 radius = 0.6f;
    f32 strength = 0.3f;
    i32 flags = 0x17;
    i32 damage = 1;
    if ((object->apiobj.flags_low & 0x80) != 0 && Cheat_IsOn(0x16)) {
        radius = 1.0f;
        strength = 0.5f;
        flags = 0x37;
        damage = 2;
    }
    EXPLOSION *explosion = AddExplosion(&position, radius, strength, object, object->slam_debris_effect, flags);
    if (explosion != NULL)
        explosion->field_0x32 = damage;
}

void CloakMovement(GameObject_s *) {
}

void RndrTexQuad3D(VuMtx const &, i32, numtl_s *) {
}

void CheckResetBits() {
    if ((ResetBits & RESETBIT_CLEAR_LEVEL_PROGRESS) != 0 && WORLD->current_level->area_level_index != -1) {
        ClearLevelProgress(WORLD->current_level->area_level_index, WORLD);
    }

    Cheats_Reset();
    if (WORLD->level_progress != NULL) {
        ResetScene(WORLD->current_gscn, reinterpret_cast<SCENEPROGRESS_s *>(WORLD->level_progress));
        WORLD->level_progress->flags |= 1;
    }

    GizmoBlowupVisibilityOverrides(WORLD);
    texanimbits = 0;
    SetTexAnimSignals();
    SetTexAnimSignals();
    CutScenes_Reset(WORLD);

    if (NOSOUND == 0) {
        if ((ResetBits & RESETBIT_USE_CUSTOMISER_SETUP) == 0) {
            InitGameMode();
        }
        if (NOSOUND == 0) {
            Customiser_SetUpCharacterData(CharacterCustomiser);
        }
    }

    Surfaces_Reset();
    ResetStreaks();
    Bolts_Reset();
    Batarangs_Reset();
    Detonators_Reset();
    ResetParts();
    ResetExplosions();
    ShoveObjectSysReset();
    Panel_Clear();
    GameCam_Reset(GameCam);
    ResetGameMessages();
    Tag_ResetTransfers();
    Hint_Reset();
    Hint_CancelCurrent();
    Teleports_Reset(WORLD);
    TrafficAnimSys_Reset(WORLD->trafficanim_sys);
    Pulses_Reset(WORLD->pulses_sys);
    NuSound3StopRumble();
    ResetTimer(&GameTimer, 0.0f);
    ResetTimer(&GamePlayTimer, 0.0f);
    ResetTimer(&PauseTimer, 0.0f);
    ResetTimer(&JoinInTimer, 0.0f);
    if ((ResetBits & RESETBIT_CLEAR_LEVEL_PROGRESS) != 0 && MissionSys != NULL) {
        ResetTimer(&MissionSys->timer, 0.0f);
    }
    Hint_SetHintFromId(-1, 0, 0);
    ResetRepeatSfx();
    DEFAULT_MOVE_RANGE = 0.0f;
    alert_obj = NULL;
    alert_timer = 0.0f;
    drop_back_in_timer = 0.0f;
    ResetRippleSet(ripples);
    Grabber_Reset(WORLD);
    Faders_Reset(WORLD);
    chattersfxwait = 3.0f;
    last_chatter_sfx = -1;
    tieonsfxwait = 1.0f;
    tieoffsfxwait = 1.0f;
    FalconDebKey[0] = -1;
    FalconDebKey[1] = -1;
    LevelNameMul = 0.0f;
    LevelNameTime = LevelChange != 0 ? 3.0f : 0.0f;
    memset(&lev_rtldata, 0, sizeof(lev_rtldata));
    if (VehicleArea != 0) {
        if (WORLD->current_level == ASTEROIDCHASED_LDATA || WORLD->current_level == DEATHSTAR2BATTLEA_LDATA) {
            TargetDist_Near2 = 6.25f;
            TargetDist_Mid2 = 100.0f;
            TargetDeg_Near = 0x38e3;
            TargetDeg_Mid = 0x31c7;
            TargetDeg_Far = 0x2aaa;
        } else {
            TargetDist_Near2 = 6.25f;
            TargetDist_Mid2 = 100.0f;
            TargetDeg_Near = 0x2aaa;
            TargetDeg_Mid = 0x1c71;
            TargetDeg_Far = 0x0e38;
        }
    } else {
        TargetDist_Near2 = 0.25f;
        TargetDist_Mid2 = 1.0f;
        TargetDeg_Near = 0x2aaa;
        TargetDeg_Mid = 0x2000;
        TargetDeg_Far = 0x1555;
    }
    party_under_cover = 0;
    nbaddies_can_see_players = 0;
    if (reset_restart != 0) {
        LevTime[0] = 0.0f;
        LevTime[1] = 0.0f;
        BonusCoinTotal = 0;
    }

    const i32 progress_index = WORLD->current_level->area_level_index;
    if (WORLD->api_object_sys != NULL)
        WORLD->api_object_sys->flags_210 &= ~1;
    if ((ResetBits & RESETBIT_CLEAR_LEVEL_PROGRESS) != 0) {
        GizmoSysClearLevelProgress(WORLD, progress_index);
    }

    GameAnimSys_ReStoreProgress(WORLD->game_anim_sys, progress_index);
    GizmoSysReset(WORLD->gizmo_sys, WORLD, progress_index);

    if ((ResetBits & RESETBIT_REINITIALISE_LEVEL) != 0) {
        DrawBossHitPoints(NULL);
        ResetForceBack();
        ClearAICreatures();
        GameAISysReset(WORLD->ai_sys);
        ReStoreStatusTakeOverObjectSys(1);
        AIScriptInitConditions(WORLD->ai_sys);

        for (i32 player_index = 0; player_index < 8; ++player_index) {
            if (Player[player_index] == NULL)
                continue;
            InitPlayerAI(Player[player_index]);
            char script_name[32];
            if (FreePlay != 0) {
                if (AIScriptFind(WORLD->ai_sys, const_cast<char *>("Freeplay"), 0, 1, 0) != NULL)
                    strcpy(script_name, "Freeplay");
                else
                    goto party_script;
            } else if (Mission_Active(NULL) != NULL) {
                if (AIScriptFind(WORLD->ai_sys, const_cast<char *>("Mission"), 0, 1, 0) != NULL)
                    strcpy(script_name, "Mission");
                else
                    goto party_script;
            } else {
                if (AIScriptFind(WORLD->ai_sys, Player[player_index]->apiobj.character_data->file, 0, 1, 0) != NULL) {
                    sprintf(script_name, Player[player_index]->apiobj.character_data->file);
                } else {
                    if ((Player[player_index]->apiobj.character_data->model_flags & 8) != 0)
                        strcpy(script_name, "Jedi");
                    else if ((Player[player_index]->apiobj.character_data->model_flags & 0x80) != 0)
                        strcpy(script_name, "Blaster");
                    else
                        strcpy(script_name, "NoWeapon");
                    if (AIScriptFind(WORLD->ai_sys, script_name, 0, 1, 0) == NULL) {
                    party_script:
                        if (AIScriptFind(WORLD->ai_sys, const_cast<char *>("party"), 0, 1, 0) != NULL)
                            strcpy(script_name, "party");
                        else
                            strcpy(script_name, "GeneralParty");
                    }
                }
            }
            AIScriptProcessorInit(WORLD->ai_sys, &Player[player_index]->ai,
                                  reinterpret_cast<AISCRIPTPROCESS *>(&Player[player_index]->ai), NULL, script_name,
                                  NULL, 1, NULL, NULL);
            if (FreePlay == 0 && Mission_Active(NULL) == NULL && Player[player_index]->field_0xcc0 == NULL &&
                AIScriptSetBaseScriptStateByName(reinterpret_cast<AISCRIPTPROCESS *>(&Player[player_index]->ai),
                                                 const_cast<char *>("InActive")) != 0) {
                Player[player_index]->apiobj.flags_high &= ~0x10;
            }
        }
    } else {
        ReStoreStatusTakeOverObjectSys(0);
    }

    if (makebaddiesforgetinresetbits != 0)
        MakeBaddiesForgetAboutParty(0);
    if ((ResetBits & 2) != 0) {
        for (i32 i = 0; i < 8; ++i) {
            if (Player[i] != NULL)
                Player[i]->field_0xe38 = 4;
        }
    }
    if ((ResetBits & 4) != 0) {
        for (i32 i = 0; i < 8; ++i) {
            if (Player[i] != NULL) {
                if (Player[i]->apiobj.player_controlled)
                    Player[i]->spawn_protection_timer = 2.5f;
                else
                    Player[i]->spawn_protection_timer = 0.0f;
            }
        }
    }
    ResetRadios();
    SpecialMiniKits_Reset(WORLD);
    SuperCounters_FixUpGizmos(WORLD);
    AITriggerSetSysReset(WORLD->ai_trigger_set_sys);
    AITriggerSysAutoSetUp(WORLD, WORLD->ai_trigger_set_sys);
    if ((ResetBits & 0x10) != 0) {
        for (i32 i = 0; i < 8; ++i) {
            if (Player[i] != NULL)
                ResetPlayer(Player[i], 1, NULL, 0);
        }
        VehicleAreaRememberSpeed = GetVehicleAreaRememberSpeed();
    }

    LevHSpecialExists = 0;
    for (i32 i = 0; i < 88; ++i) {
        if (NuSpecialExistsFn(&LevHSpecial[i]) != 0) {
            LevHSpecialExists |= static_cast<u64>(1) << (i & 63);
        }
    }
    WORLD->field_0x50d0 = 0;
    if (WORLD->current_level->reset_fn != NULL) {
        WORLD->current_level->reset_fn(WORLD);
    }
    CharPlatforms_Reset(WORLD->char_platform_sys);
    SetSoundFadeDist(WORLD, &Game.options_save);
    TempOptions.field3_0x3 = Game.options_save.field3_0x3;
    TempOptions.field4_0x4 = Game.options_save.field4_0x4;
    TempOptions.field5_0x5 = Game.options_save.field5_0x5;

    if (WORLD->level_progress != NULL && (WORLD->level_progress->flags & 2) == 0) {
        nugspline_s *spline = reinterpret_cast<nugspline_s *>(WORLD->portal_places[1]);
        if (spline != NULL && gone_through_door_to_new_mode == 0 && gone_through_door_to_new_level == 0 &&
            come_from_an_editor == 0) {
            GameCameraMakeMiniCut(spline, 0.0f, 2.0f, 0.0f, 1.0f, 0, 1);
        }
    }
    gone_through_door_to_new_mode = 0;
    come_from_an_editor = 0;
    if (newmode_cutinfo != NULL) {
        NewCutScene(newmode_cutinfo, WORLD->cutscene_sys, NULL, 0);
        newmode_cutinfo = NULL;
    }

    if (BonusArea != 0)
        Cheats_TurnOff(1);
    BonusCoinTarget = TotalLevelCoinTally(WORLD, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (BonusCoinTarget > 1000000u || (BonusArea != 0 && VehicleArea != 0 && BonusCoinTarget != 1000000u))
        BonusCoinTarget = 1000000u;
    if (netclient != 0)
        BonusCoinTarget = 1000000u;

    // Reset requests are edge-triggered. Leaving the bits set would rebuild the
    // level's AI and gizmo state again on every frame through Batman().
    ResetBits = 0;
    reset_reimport = 0;
    reset_restart = 0;
    CutScene_StartAudio();
    if (NOSOUND == 0)
        WORLD->reset_flags = 1;
    LookAtBoth = 0;
    oneAtOnce_SetNumAttackers(1);
    if (WORLD->level_progress != NULL) {
        if ((WORLD->level_progress->flags & 2) != 0)
            WORLD->field_0x5174 = (WORLD->level_progress->flags >> 2) & 1;
        ResetGizFlow(WORLD->giz_flow, &WORLD->level_progress->giz_flow_progress);
    } else {
        ResetGizFlow(WORLD->giz_flow, NULL);
    }
    EffectOffProgress_Reset(WORLD->level_progress);
    if (HUB_ADATA != NULL && WORLD->area == HUB_ADATA) {
        Tag_SetMode(3);
    } else {
        Tag_SetMode(1);
    }
    WeaponInOut_NoAIJediSfx = WORLD->area != NULL && WORLD->area == JEDI_ADATA ? 1 : 0;
    bonusmodearcade = 0;
    if (WORLD->area != NULL && (WORLD->area->flags & 0x10) != 0) {
        if (AreaGlobals.values.field_0x14 < AreaGlobals.values.field_0x0c) {
            AreaGlobals.values.field_0x14 = AreaGlobals.values.field_0x0c < 11 ? AreaGlobals.values.field_0x0c : 10;
        }
    }
    arcade_placed_stud_total = ((GizmoPickups_TotalScore(WORLD) * 85u) / 100u) / 1000u * 1000u;
}

void bgProcAbortAll() {
}

void bgprocUnFreeze() {
}

extern AREADATA_s *PODRACE_ADATA;
extern AREADATA_s *BONUS_GUNSHIP_ADATA;
extern AREADATA_s *GUNSHIP_ADATA;
extern i16 id_LANDSPEEDER;
void PodDust(WORLDINFO_s *, GameObject_s *);
extern "C" void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);

void AddSurfaceDebris(GameObject_s *object) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (object->field_0x7a5 == 0x2b)
        return;
    i32 layer = static_cast<i8>(object->apiobj.field_0x27f);
    if (layer != -1 && (TerLayer[layer].flags & 1) != 0)
        return;
    f32 surface_y = object->apiobj.field_0x218;
    if (surface_y == 2000000.0f)
        return;
    if ((PODRACE_ADATA != NULL && world->area == PODRACE_ADATA) || world->current_level == PODSPRINTA_LDATA) {
        PodDust(world, object);
        return;
    }
    i32 effect;
    f32 rate;
    f32 minimum_rate;
    f32 height;
    if (object->id == id_LANDSPEEDER) {
        effect = 102;
        rate = 20.0f;
        minimum_rate = 0.0f;
        height = 0.0f;
    } else if (WORLD->area != NULL && WORLD->area == HOTHBATTLE_ADATA) {
        effect = 86;
        rate = 100.0f;
        minimum_rate = 10.0f;
        height = 3.0f;
    } else if ((BONUS_GUNSHIP_ADATA != NULL && world->area == BONUS_GUNSHIP_ADATA) ||
               (GUNSHIP_ADATA != NULL && world->area == GUNSHIP_ADATA)) {
        effect = 141;
        rate = 75.0f;
        minimum_rate = 0.0f;
        height = 0.0f;
    } else {
        return;
    }
    f32 speed = object->apiobj.horizontal_velocity_magnitude;
    f32 run_speed = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->run_speed;
    if (world->debris_sys->entries[effect].effect == -1 || run_speed <= 0.0f)
        return;
    f32 height_scale = 1.0f;
    if (height > 0.0f) {
        height_scale = 1.0f - (object->apiobj.collision_min.y - surface_y) * (1.0f / height);
        if (height_scale <= 0.0f)
            return;
        if (height_scale > 1.0f)
            height_scale = 1.0f;
    }
    f32 emission_rate = (speed / run_speed) * rate;
    if (emission_rate < minimum_rate)
        emission_rate = minimum_rate;
    emission_rate *= height_scale;
    NUVEC current = {object->apiobj.position.x, surface_y, object->apiobj.position.z};
    NUVEC previous;
    if (object->apiobj.field_0x214 == 2000000.0f) {
        previous = current;
    } else {
        previous.x = object->apiobj.start_position.x;
        previous.y = object->apiobj.field_0x214;
        previous.z = object->apiobj.start_position.z;
    }
    i32 count = ParticlesPerSecond(emission_rate, FRAMETIME);
    if (count <= 0)
        return;
    NUVEC delta = {current.x - previous.x, current.y - previous.y, current.z - previous.z};
    do {
        f32 fraction = qrand() * (1.0f / 65535.0f);
        NUVEC position = {previous.x + delta.x * fraction, previous.y + delta.y * fraction,
                          previous.z + delta.z * fraction};
        AddVariableShotDebrisEffect(world->debris_sys->entries[effect].effect, &position, 1, 0, 0);
    } while (--count != 0);
}

extern NUMTX NuRndr_DebrisMtx;
extern NUVEC4 NuRndr_DebrisPlane;
extern nunativedebrisdata_s *g_ParticleGroup;
extern void *g_pVBData;
extern u32 g_CurrentVBVertexCount;
extern u32 g_FrameVertexCount;
extern u32 g_VBMaxVertexCount;
extern u32 g_CurrentDebriVBIndex;
extern i32 g_UseSysMemVB;
extern i32 NuDebrisRendererNextBuffer();
extern void NuRndrParticleSetRepeat(NUVEC *position);

void bgprocIsFreezing() {
}

extern "C" void DebFree(i32 *);
void DebFreeWithoutKey(debkeydatatype_s *key) {
    i32 handle = key->allocation_index;
    DebFree(&handle);
}

void DebrisKillPlayers() {
}

void RndrUnfilledCircle(float, float, float, float, float, i32, float, float, numtl_s *) {
}

void DebrisProcessSpheres(uv1deb *data, float time, debinftype *effect, debkeydatatype_s *key, i32 finite) {
    if (!(time >= key->sphere_next_time))
        return;
    if (key->sphere_skip_count > 0) {
        --key->sphere_skip_count;
        return;
    }
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    i32 index = key->field_2c8;
    key->process_spheres[index].position.x = particle->position.x + key->position.x;
    key->process_spheres[index].position.y = particle->position.y + key->position.y;
    key->process_spheres[index].position.z = particle->position.z + key->position.z;
    key->process_spheres[index].time = time;
    key->process_spheres[index].momentum = particle->momentum;
    if (++key->field_2c8 >= static_cast<i8>(effect->process_spheres))
        key->field_2c8 = 0;
    if (finite != 0)
        key->sphere_skip_count = key->field_2c8;
    key->sphere_next_time =
        time + effect->particle_lifetime / static_cast<f32>(static_cast<i8>(effect->process_spheres));
}

void DisplayListPrintItem(nudisplaylistitem_s *, i32, i32, i32 *, i32) {
}

// Debug-capture output helpers consumed by NuDisplayListCaptureSortPriority.
// Transcribed from the original C-linkage symbols:
//   NuHtmlBegin    0x2d5ca0   NuHtmlFlush   0x2d5c30
//   NuHtmlWrite    0x2d5cd0   NuHtmlHeading1 0x2d5d40
static char nudl_html_buf[0x1000]; // flush threshold leaves room for formatted output
static char *nudl_html_cursor;     // original @0xb9d720-rel
static char *nudl_html_end;        // original @0xb9d730-rel
extern "C" {
    NUFILE hfh;
    char unknown[8] = "unknown";
}

void NuHtmlFlush(i32 force) {
    if ((nudl_html_cursor > nudl_html_end) | force) {
        NuFileWriteString(hfh, nudl_html_buf);
        nudl_html_cursor = nudl_html_buf;
        nudl_html_end = nudl_html_buf + 0xc00;
    }
}

extern "C" void NuHtmlBegin(void *file) {
    hfh = static_cast<NUFILE>(reinterpret_cast<usize>(file));
    nudl_html_cursor = nudl_html_buf;
    nudl_html_end = nudl_html_buf + 0xc00;
}

extern "C" void NuHtmlWrite(const char *text, ...) {
    if (text == NULL || text[0] == '\0') {
        text = unknown;
    }
    va_list ap;
    va_start(ap, text);
    vsprintf(nudl_html_cursor, text, ap);
    va_end(ap);
    nudl_html_cursor += NuStrLen(nudl_html_cursor);
    NuHtmlFlush(0);
}

extern "C" void NuHtmlHeading1(const char *fmt, ...) {
    if (fmt == NULL || fmt[0] == '\0') {
        fmt = unknown;
    }
    NuHtmlWrite("<table width=100%c bgcolor=#CFCFE5><tr><td><font face=arial size=+3> ", '%');
    va_list ap;
    va_start(ap, fmt);
    vsprintf(nudl_html_cursor, fmt, ap);
    va_end(ap);
    nudl_html_cursor += NuStrLen(nudl_html_cursor);
    NuHtmlWrite("</font></table>\n");
}

extern "C" void NuHtmlBanner(void) {
    NuHtmlWrite("<table   width=100%c height=20 bgcolor=#FF0000><tr><td><font face=arial size=+2></font></table>", '%');
}

extern "C" void NuHtmlHeading2(const char *fmt, ...) {
    if (fmt == NULL || fmt[0] == '\0') {
        fmt = unknown;
    }
    NuHtmlWrite("<table width=100%c bgcolor=#DFDFE5><tr><td><font face=arial size=+2> ", '%');
    va_list ap;
    va_start(ap, fmt);
    vsprintf(nudl_html_cursor, fmt, ap);
    va_end(ap);
    nudl_html_cursor += NuStrLen(nudl_html_cursor);
    NuHtmlWrite("</font></table>\n");
}

extern "C" void NuHtmlHeading3(const char *fmt, ...) {
    if (fmt == NULL || fmt[0] == '\0') {
        fmt = unknown;
    }
    NuHtmlWrite("<table width=100%c bgcolor=#FFDFE5><tr><td><font face=arial size=+0> ", '%');
    va_list ap;
    va_start(ap, fmt);
    vsprintf(nudl_html_cursor, fmt, ap);
    va_end(ap);
    nudl_html_cursor += NuStrLen(nudl_html_cursor);
    NuHtmlWrite("</font></table>\n");
}

extern "C" void NuHtmlEnd(void) {
    NuHtmlFlush(1);
    hfh = 0;
}

void NuHtmlTitle(char *title) {
    NuHtmlWrite("<HR><Center><B>");
    NuHtmlWrite(title);
    NuHtmlWrite("</B></Center><HR>");
    NuHtmlWrite("<P><P>");
}

extern "C" void NuHtmlBitmap(char *filename, i32 width, i32 height, char *caption, char *source) {
    char tag[256];
    NuHtmlWrite("<P>");
    if (height != 0 && width != 0) {
        sprintf(tag, "<IMG ALIGN=\"left\" HEIGHT=\"%d\" WIDTH=\"%d\" SRC=\"%s\">", height, width, filename);
    } else {
        sprintf(tag, "<IMG ALIGN=\"left\" SRC=\"%s\">", filename);
    }
    NuHtmlWrite(tag);
    if (caption != NULL) {
        NuHtmlWrite(caption);
    }
    NuHtmlWrite("<BR CLEAR=\"all\"> <P>&nbsp;<P>");
    if (source != NULL && NuStrCmp(filename, source) != 0) {
        NuFileCopy(filename, source);
    }
}

void NuHtmlGraphArray(char **strings) {
    char *text = *strings++;
    while (text != NULL) {
        NuHtmlWrite(text);
        text = *strings++;
    }
}

void AddChunkToRenderStack(particlechunkrendertype_s *chunk, particlechunkrendertype_s **stack) {
    chunk->previous = NULL;
    chunk->next = NULL;

    particlechunkrendertype_s *current = *stack;
    if (current != NULL) {
        const u16 priority = static_cast<u16>(chunk->render_priority);
        const u16 current_priority = static_cast<u16>(current->render_priority);
        if (priority <= current_priority &&
            (priority != current_priority || current->effect->status < chunk->effect->status)) {
            particlechunkrendertype_s *next = current->next;
            while (next != NULL) {
                const u16 next_priority = static_cast<u16>(next->render_priority);
                if (next_priority <= priority &&
                    (priority != next_priority || chunk->effect->status <= next->effect->status)) {
                    break;
                }
                current = next;
                next = next->next;
            }
            current->next = chunk;
            chunk->previous = current;
            chunk->next = next;
            if (next != NULL) {
                next->previous = chunk;
            }
            return;
        }
        chunk->next = current;
        current->previous = chunk;
    }
    *stack = chunk;
}

void AddChunkControlToStack(debris_chunk_control_s *control, debris_chunk_control_s **stack) {
    debris_chunk_control_s *current = *stack;
    while (current != NULL && current->expiry_time < control->expiry_time) {
        stack = &current->next;
        current = current->next;
    }
    control->next = current;
    *stack = control;
}

extern "C" debkeydatatype_s *debris_keystack;

void AddDebrisEffectToStack(debkeydatatype_s *key) {
    if (key == NULL) {
        return;
    }
    if (debris_keystack != NULL) {
        debris_keystack->next = key;
    }
    key->previous = debris_keystack;
    debris_keystack = key;
}

extern "C" {
    extern debkeydatatype_s *debkeydata;
    extern debinftype **debtab;
    extern debris_chunk_control_s **freechunkcontrols;
    extern i32 freechunkcontrolsptr;
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
    extern particlechunkrendertype_s *ParticleChunkToRender;
    extern particlechunkrendertype_s *ParticleChunkRenderStack[5];
    extern i32 debrischunks;
    extern i32 debrischunksglass;
    extern f32 globaltime;
    extern f32 panelglobaltime;
}
void DebrisGetControlStackLock(void);
void DebrisReleaseControlStackLock(void);
void RemoveChunkFromRenderStack(particlechunkrendertype_s *, particlechunkrendertype_s **);

void DebFreeChunksInstantly(i32 *handle) {
    if (handle == NULL || *handle == -1) {
        return;
    }
    debkeydatatype_s *key = &debkeydata[*handle];
    if (key->effect_index == 0 || key->allocated_chunk_count == 0) {
        return;
    }
    debinftype *effect = debtab[key->effect_index];

    DebrisGetControlStackLock();
    for (i32 i = 0; i < key->allocated_chunk_count; ++i) {
        debris_chunk_control_s *control = freechunkcontrols[freechunkcontrolsptr++];
        control->particle_chunk = key->particle_chunks[i];
        control->active = effect->particle_type == 7 ? 9 : 2;
        control->owner = NULL;
        const bool panel_time = effect->time_group == 4;
        control->expiry_time = panel_time ? panelglobaltime : globaltime;
        AddChunkControlToStack(control, &debris_chunk_control_stack[panel_time ? 1 : 0]);
    }
    DebrisReleaseControlStackLock();

    const i32 render_chunk_count = debrischunks + debrischunksglass;
    for (i32 i = 0; i < render_chunk_count; ++i) {
        particlechunkrendertype_s *render_chunk = &ParticleChunkToRender[i];
        if (render_chunk->particle_chunk == key->particle_chunks[0]) {
            if (key->field_2f6 != 0) {
                RemoveChunkFromRenderStack(render_chunk, &ParticleChunkRenderStack[effect->time_group]);
            }
            render_chunk->particle_chunk = NULL;
            render_chunk->effect = NULL;
            render_chunk->key = NULL;
            break;
        }
    }

    for (i32 i = 0; i < key->allocated_chunk_count; ++i) {
        key->particle_chunks[i] = NULL;
    }
    key->particle_count = 0;
    key->allocated_chunk_count = 0;
    key->previous_particle_count = 0;
    key->previous_allocated_chunk_count = 0;
    key->controlled_chunk_count = 0;
    key->field_18a = 0;
}

extern "C" {
    extern debkeydatatype_s *debkeydata;
    extern debinftype **debtab;
    extern i16 *freedebkeys;
    extern i32 freedebkeyptr;
    extern dma_particle_chunk_s **freedebchunks;
    extern dma_particle_chunk_s **freedebchunksglass;
    extern i32 freedebchkptr;
    extern i32 freedebchkptrg;
    extern i32 debrischunks;
    extern i32 debrischunksglass;
    extern debris_chunk_control_s **freechunkcontrols;
    extern i32 freechunkcontrolsptr;
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
    extern particlechunkrendertype_s *ParticleChunkToRender;
    extern particlechunkrendertype_s *ParticleChunkRenderStack[5];
    extern f32 globaltime;
    extern f32 panelglobaltime;
    extern f32 timeincrement;
    extern f32 debris_thinning_level;
    extern i32 forced_debris_thinning;
}

extern "C" void DebReAlloc2(debkeydatatype_s *);
extern "C" void DebReAlloc(debkeydatatype_s *, i32);
void RemoveChunkFromRenderStack(particlechunkrendertype_s *, particlechunkrendertype_s **);
void DebrisReleaseControlStackLock(void);

void DebrisProcessAllocation() {
    for (debkeydatatype_s *key = debris_keystack; key != NULL; key = key->previous) {
        if (key->previous_particle_count != key->particle_count) {
            DebReAlloc2(key);
        }
    }
}

extern "C" {
    f32 CameraEmitterDistance(NUVEC *);
    void SetSfxBit_On(i32);
    void PlaySfxByIdEx(i32, NUVEC *, f32, f32);
    extern i32 debris_render_group;
    extern u32 debrisseed;
}
extern i32 debris_detail_level;

static inline void DebrisEmissionSound(debkeydatatype_s *key, debinftype *effect, i32 event, f32 volume) {
    for (i32 i = 0; i < 4; ++i) {
        if (effect->sound_data[i * 3] != -1 && effect->sound_data[i * 3 + 1] == event)
            PlaySfxByIdEx(effect->sound_data[i * 3], &key->position, volume, 1.0f);
    }
}

extern u8 object_switches[0x80];
extern "C" void DebrisStartOffsetEx(debkeydatatype_s *, f32);

void DebrisProcessTriggers() {
    i32 switch_changes[32][2];
    i32 last_switch_change = -1;
    for (debkeydatatype_s *key = debris_keystack; key != NULL;) {
        debinftype *effect = debtab[key->effect_index];
        debkeydatatype_s *next = key->previous;
        if (key->trigger_first == 1 && key->field_184 != 2 && key->trigger_second != -1) {
            switch (object_switches[key->trigger_second]) {
                case 0:
                    key->field_184 = 0;
                    break;
                case 1:
                    key->field_184 = 1;
                    break;
                case 3:
                    last_switch_change = (last_switch_change + 1) & 31;
                    switch_changes[last_switch_change][0] = key->trigger_second;
                    switch_changes[last_switch_change][1] = 0;
                    break;
                case 2:
                    last_switch_change = (last_switch_change + 1) & 31;
                    key->field_184 = 2;
                    switch_changes[last_switch_change][0] = key->trigger_second;
                    switch_changes[last_switch_change][1] = 3;
                    goto restart_emission;
                case 4: {
                    last_switch_change = (last_switch_change + 1) & 31;
                    key->field_184 = 2;
                    switch_changes[last_switch_change][0] = key->trigger_second;
                    switch_changes[last_switch_change][1] = 0;
                restart_emission:
                    DebrisStartOffsetEx(key, 0.0f);
                    f32 shift = key->emission_time - globaltime;
                    key->field_1d8 = 0;
                    key->emission_epoch = globaltime;
                    key->emission_time -= shift;
                    key->field_1e4 -= shift;
                    if (key->process_collision_sound != 0) {
                        key->cutoff_distance = CameraEmitterDistance(&key->position);
                        f32 range = effect->sound_range_override;
                        if (range == 0.0f)
                            range = effect->sound_range;
                        if (range == 0.0f)
                            range = effect->clip_extent;
                        f32 volume = 0.0f;
                        if (range > key->cutoff_distance) {
                            if (effect->sound_data[0] != -1)
                                SetSfxBit_On(effect->sound_data[0]);
                            if (effect->sound_data[3] != -1)
                                SetSfxBit_On(effect->sound_data[3]);
                            if (effect->sound_data[6] != -1)
                                SetSfxBit_On(effect->sound_data[6]);
                            if (effect->sound_data[9] != -1)
                                SetSfxBit_On(effect->sound_data[9]);
                            volume = (range - key->cutoff_distance) / range;
                        }
                        if (range > key->cutoff_distance) {
                            if (effect->sound_data[0] != -1 && effect->sound_data[1] == 1)
                                PlaySfxByIdEx(effect->sound_data[0], &key->position, volume, 1.0f);
                            if (effect->sound_data[3] != -1 && effect->sound_data[4] == 1)
                                PlaySfxByIdEx(effect->sound_data[3], &key->position, volume, 1.0f);
                            if (effect->sound_data[6] != -1 && effect->sound_data[7] == 1)
                                PlaySfxByIdEx(effect->sound_data[6], &key->position, volume, 1.0f);
                            if (effect->sound_data[9] != -1 && effect->sound_data[10] == 1)
                                PlaySfxByIdEx(effect->sound_data[9], &key->position, volume, 1.0f);
                        }
                    }
                    break;
                }
            }
        }
        key = next;
    }
    for (i32 i = 0; i <= last_switch_change; ++i)
        object_switches[switch_changes[i][0]] = switch_changes[i][1];
}

void DebrisProcessGeneration() {
    for (debkeydatatype_s *key = debris_keystack; key != NULL;) {
        debkeydatatype_s *next = key->previous;
        debinftype *effect = debtab[key->effect_index];
        const f32 now = effect->time_group == 4 ? panelglobaltime : globaltime;
        if (key->field_1d8 != 0) {
            key = next;
            continue;
        }
        key->cutoff_distance = CameraEmitterDistance(&key->position);
        const f32 near_distance = *reinterpret_cast<f32 *>(&effect->fields_030[4]);
        const i16 render_group = *reinterpret_cast<i16 *>(key->fields_2f0);
        if (key->field_184 != 0) {
            if (key->cutoff_distance < near_distance ||
                (effect->clip_extent > 0.0f && key->cutoff_distance > effect->clip_extent)) {
                if (effect->use_explicit_clip_box == 0 && effect->time_group != 4)
                    key->field_184 = 0;
            }
            if (debris_render_group != 0 && render_group != 0 && debris_render_group != render_group)
                key->field_184 = 0;
            if ((static_cast<i8>(key->field_1da) & debris_detail_level) == 0)
                key->field_184 = 0;
        }
        f32 sound_range = effect->sound_range_override;
        if (sound_range == 0.0f)
            sound_range = effect->sound_range;
        if (sound_range == 0.0f)
            sound_range = effect->clip_extent;
        f32 volume = 1.0f;
        if (sound_range > key->cutoff_distance) {
            for (i32 i = 0; i < 4; ++i)
                if (effect->sound_data[i * 3] != -1)
                    SetSfxBit_On(effect->sound_data[i * 3]);
            volume = (sound_range - key->cutoff_distance) / sound_range;
        }
        if (key->process_collision_sound != 0 && key->field_184 != 0 && sound_range > key->cutoff_distance)
            DebrisEmissionSound(key, effect, 4, volume);
        if (effect->status == 0)
            key->field_184 = 0;
        if (effect->status == 2)
            key->field_184 = 1;
        if (key->field_2f4 == 0)
            key->field_184 = 0;
        else {
            if (key->field_2f4 == 2)
                key->field_184 = 1;
            if (key->field_184 != 0 && key->previous_allocated_chunk_count == 0) {
                DebReAlloc(key, effect->max_particles);
                const f32 thinning =
                    forced_debris_thinning != 0
                        ? debris_thinning_level
                        : (effect->thinning < debris_thinning_level ? effect->thinning : debris_thinning_level);
                DebReAlloc(key, static_cast<i32>(effect->max_particles / thinning));
            }
        }
        const f32 elapsed = now - key->last_update_time;
        key->emission_position.x = key->emitter_momentum.x * elapsed;
        key->emission_position.y = key->emitter_momentum.y * elapsed;
        key->emission_position.z = key->emitter_momentum.z * elapsed;
        const f32 frequency = static_cast<f32>(effect->frequency);
        const f32 thinning =
            forced_debris_thinning != 0
                ? debris_thinning_level
                : (effect->thinning < debris_thinning_level ? effect->thinning : debris_thinning_level);
        const f32 emission_interval = frequency > 0.0f ? 1.0f / (frequency / thinning) : 0.0f;
        f32 emission_time = key->emission_epoch + emission_interval;
        for (i32 emission = 1; emission != 100 && emission_time < now + timeincrement; ++emission) {
            f32 pause = 0.0f;
            i32 transitions = 100;
            while (emission_time >= key->field_1e4 && emission_time >= key->emission_time && --transitions != 0) {
                if (key->emission_time < key->field_1e4) {
                    key->previous_emission_time = key->emission_time;
                    pause =
                        effect->emission_pause_random + NuRandFloatSeeded(&debrisseed) * effect->start_offset_random;
                    key->emission_time = key->field_1e4 + pause;
                    if (key->field_184 == 2)
                        key->field_184 = 0;
                    if (key->process_collision_sound != 0 && key->field_184 != 0 && sound_range > key->cutoff_distance)
                        DebrisEmissionSound(key, effect, 2, volume);
                } else {
                    key->field_1e4 = key->emission_time + effect->emission_period_random +
                                     NuRandFloatSeeded(&debrisseed) * effect->emission_pause;
                    if (key->field_1d4 > 0) {
                        if (--key->field_1d4 == 0) {
                            DebFreeWithoutKey(key);
                            emission_time += 99999.0f;
                            key->process_collision_sound = 0;
                        }
                    }
                    if (key->field_1d4 < 0 && ++key->field_1d4 == 0) {
                        key->field_2f4 = 0;
                        key->field_184 = 0;
                    }
                    if (key->process_collision_sound != 0 && key->field_184 != 0 && sound_range > key->cutoff_distance)
                        DebrisEmissionSound(key, effect, 1, volume);
                    pause = 0.0f;
                }
            }
            if (pause > 0.0f) {
                emission_time = key->emission_time - emission_interval;
                key->emission_epoch = emission_time;
            } else if (key->allocated_chunk_count > 0 && key->field_184 != 0) {
                uv1deb *particle = key->generator(key, effect, emission_time);
                if (effect->process_spheres != 0 && particle != NULL && emission == 1)
                    DebrisProcessSpheres(particle, emission_time, effect, key, 0);
                if (key->process_collision_sound != 0 && sound_range > key->cutoff_distance)
                    DebrisEmissionSound(key, effect, 3, volume);
            }
            emission_time += emission_interval;
        }
        if (key->field_184 == 0) {
            if (key->field_2f4 != 0 && key->trigger_second == -1 && key->cutoff_distance >= near_distance &&
                (effect->clip_extent == 0.0f || key->cutoff_distance <= effect->clip_extent) &&
                (debris_render_group == 0 || render_group == 0 || debris_render_group == render_group) &&
                (static_cast<i8>(key->field_1da) & debris_detail_level) != 0)
                key->field_184 = 1;
            else if (key->previous_allocated_chunk_count != 0)
                DebReAlloc(key, 0);
        }
        key = next;
    }
}

void DisplayListRenderBuffer() {
}

void DebrisGetControlStackLock() {
}

static particlechunkrendertype_s *FindParticleRenderChunk(dma_particle_chunk_s *particle_chunk) {
    const i32 count = debrischunks + debrischunksglass;
    for (i32 i = 0; i < count; ++i) {
        if (ParticleChunkToRender[i].particle_chunk == particle_chunk) {
            return &ParticleChunkToRender[i];
        }
    }
    return NULL;
}

static void ReleaseChunkControl(debris_chunk_control_s *control) {
    --freechunkcontrolsptr;
    freechunkcontrols[freechunkcontrolsptr] = control;
    control->particle_chunk = NULL;
}

i32 SolveQuadratic(f32, f32, f32, f32 *, f32 *);
void DebrisProcessControlChunks(i32 panel_time) {
    const f32 now = panel_time != 0 ? panelglobaltime : globaltime;
    debris_chunk_control_s **stack = &debris_chunk_control_stack[panel_time];

    DebrisGetControlStackLock();
    while (*stack != NULL && (*stack)->expiry_time <= now) {
        debris_chunk_control_s *control = *stack;
        *stack = control->next;
        control->next = NULL;

        if (control->active == 5) {
            debinftype *effect = debtab[control->effect_index];
            dma_particle_s *particle = &control->particle_chunk->particles[control->particle_index];
            const f32 time = control->collision_time;
            const f32 old_y = particle->momentum.y;
            const f32 gravity_velocity = 2.0f * (time * effect->field_0a0);
            const f32 new_y = -control->restitution * (old_y + gravity_velocity) - gravity_velocity;
            particle->momentum.y = new_y;
            particle->position.y -= time * new_y - time * old_y;
            f32 first, second;
            if (SolveQuadratic(effect->field_0a0, new_y, particle->position.y - control->collision_plane, &first,
                               &second) != 0) {
                const f32 next_time = first > second ? first : second;
                if (effect->particle_lifetime > next_time) {
                    const f32 interval = next_time - control->collision_time;
                    control->collision_time = next_time;
                    if (interval > 0.02f) {
                        control->expiry_time += interval;
                        AddChunkControlToStack(control, stack);
                        continue;
                    }
                }
            }
            ReleaseChunkControl(control);
            continue;
        }

        if (control->active == 6) {
            NUVEC normal = {1.0f, 0.0f, 0.0f};
            dma_particle_s *particle = &control->particle_chunk->particles[control->particle_index];
            NuVecRotateY(&normal, &normal, control->rotation_y);
            const f32 old_x = particle->momentum.x;
            const f32 old_z = particle->momentum.z;
            const f32 impulse = -(1.0f + control->restitution) * (old_x * normal.x + old_z * normal.z);
            const f32 new_x = impulse * normal.x + old_x;
            const f32 new_z = impulse * normal.z + old_z;
            particle->momentum.x = new_x;
            particle->momentum.z = new_z;
            particle->position.x -= new_x * control->collision_time - old_x * control->collision_time;
            particle->position.z -= new_z * control->collision_time - old_z * control->collision_time;
            ReleaseChunkControl(control);
            continue;
        }

        if (control->active == 0 || control->active == 7) {
            particlechunkrendertype_s *render = FindParticleRenderChunk(control->particle_chunk);
            if (render != NULL) {
                RemoveChunkFromRenderStack(render, &ParticleChunkRenderStack[render->effect->time_group]);
                render->particle_chunk = NULL;
                render->effect = NULL;
                render->key = NULL;
            }
            control->expiry_time += 0.1f;
            control->active = control->active == 7 ? 9 : 2;
            AddChunkControlToStack(control, stack);
            continue;
        }

        if (control->active == 1) {
            debkeydatatype_s *key = control->owner;
            debinftype *effect = debtab[key->effect_index];
            i32 chunk_index = 0;
            while (chunk_index < key->allocated_chunk_count &&
                   key->particle_chunks[chunk_index] != control->particle_chunk) {
                ++chunk_index;
            }

            const i32 old_chunk_count = key->allocated_chunk_count;
            for (i32 i = chunk_index; i + 1 < old_chunk_count; ++i) {
                key->particle_chunks[i] = key->particle_chunks[i + 1];
            }
            key->particle_chunks[old_chunk_count - 1] = NULL;
            --key->previous_allocated_chunk_count;
            --key->controlled_chunk_count;
            --key->allocated_chunk_count;

            if (key->allocated_chunk_count == 0) {
                key->particle_count = 0;
                key->previous_particle_count = 0;
                for (i32 slot = 0; slot < 8; ++slot) {
                    const i16 key_index = effect->particle_keys[slot];
                    if (key_index != -1 && &debkeydata[key_index] == key) {
                        key->effect_index = 0;
                        key->allocation_index = -1;
                        --freedebkeyptr;
                        freedebkeys[freedebkeyptr] = key_index;
                        effect->particle_keys[slot] = -1;
                    }
                }
                key->particle_chunks[0] = NULL;
            } else {
                const i32 particles_per_chunk = effect->particle_type == 7 ? 12 : 32;
                key->particle_count -= static_cast<i16>(particles_per_chunk);
                key->previous_particle_count -= static_cast<i16>(particles_per_chunk);
                if ((chunk_index + 1) * particles_per_chunk < key->field_18a) {
                    key->field_18a -= static_cast<i16>(particles_per_chunk);
                }
                LinkDmaParticalSets(key->particle_chunks, key->allocated_chunk_count);
            }

            if (chunk_index == 0) {
                particlechunkrendertype_s *render_chunk = FindParticleRenderChunk(control->particle_chunk);
                if (render_chunk != NULL) {
                    render_chunk->particle_chunk = key->particle_chunks[0];
                    if (key->allocated_chunk_count == 0) {
                        RemoveChunkFromRenderStack(render_chunk, &ParticleChunkRenderStack[effect->time_group]);
                        render_chunk->effect = NULL;
                        render_chunk->key = NULL;
                    }
                }
            }

            control->expiry_time += 0.1f;
            control->active = effect->particle_type == 7 ? 9 : 2;
            AddChunkControlToStack(control, stack);
            continue;
        }

        if (control->active == 2 || control->active == 9) {
            LinkDmaParticalSets(&control->particle_chunk, 1);
            control->active = control->active == 9 ? 8 : 3;
            control->expiry_time += 0.1f;
            AddChunkControlToStack(control, stack);
            continue;
        }

        if (control->active == 3) {
            --freedebchkptr;
            freedebchunks[freedebchkptr] = control->particle_chunk;
            for (dma_particle_s &particle : control->particle_chunk->particles) {
                particle.start_time = 0.0f;
                particle.inverse_lifetime = 128.0f;
            }
            ReleaseChunkControl(control);
            continue;
        }

        if (control->active == 8) {
            --freedebchkptrg;
            freedebchunksglass[freedebchkptrg] = control->particle_chunk;
            for (i32 i = 0; i < 12; ++i) {
                control->particle_chunk->particles[i].start_time = 0.0f;
                control->particle_chunk->particles[i].inverse_lifetime = 128.0f;
            }
            ReleaseChunkControl(control);
            continue;
        }

        // Unrecognized control states are removed from the active stack in the original.
    }
    DebrisReleaseControlStackLock();
}

// original 0x2ff660
void DisplayListCreateDynMtlList(variptr_u *buffer, variptr_u buffer_end) {
    NUDLIST_MANAGER *manager = &global_dlist_manager;
    NUDLDLISTSCENE *scene = &manager->dyn_mtl_dlist;

    NuDisplayListCreate(reinterpret_cast<nudisplayscene_s *>(scene), buffer, buffer_end, 0x400, 0x80, 0, 0, 0x80, 0, 0);
    scene->nsort_pris = 0;
    scene->name = const_cast<char *>("Dynamic Material Display Scene");

    NUDISPLAYLISTITEM *material_item = scene->items;
    for (i32 i = 0; i < 0x80; ++i) {
        NUDISPLAYLIST *display_list = scene->dlist_mtls[i];
        display_list->mtl_item = material_item;
        display_list->dyn_geom = material_item + 6;
        display_list->dlist = scene;
        display_list->mtl_id = i;
        material_item += 8;
    }

    manager->nnew_materials = 0;
    manager->ndel_materials = 0;
    manager->new_materials = reinterpret_cast<NUMTL **>(ALIGN(buffer->addr, 0x10));
    manager->del_materials = manager->new_materials + 0x80;
    manager->material_used = reinterpret_cast<u8 *>(manager->new_materials + 0x100);
    manager->mtl_buffers_used = reinterpret_cast<u8 *>(manager->new_materials + 0x120);
    buffer->addr = reinterpret_cast<usize>(manager->new_materials + 0x140);
    memset(manager->material_used, 0, 0x80);
    memset(manager->mtl_buffers_used, 0, 0x80);

    manager->mtlbuff.addr = ALIGN(buffer->addr, 0x10);
    manager->mtlbuffend.addr = manager->mtlbuff.addr + 0x4000;
    *buffer = manager->mtlbuffend;

    NUDISPLAYLIST *list = &manager->dlist_2d;
    list->first->type = 0x8d;
    list->first->id = 1;
    list->first->next = nullptr;
    list->mtl_last = list->first;
    list->state = reinterpret_cast<NURNDRSTATE *>(ALIGN(buffer->addr, 4));
    buffer->addr = reinterpret_cast<usize>(list->state + 1);
    NuDisplayListReset(list);
    scene->flags |= NUDL_SCENE_FLAG_NEEDS_BUILD;
}

extern "C" {
    extern PartHeader **DmaDebTypes;
    extern i32 freeDmaDebType;
    extern i32 EDPP_MAX_TYPES;
    void edppDeleteEffect(i32);
}

void DebrisCleanUpDmaDebTypeTables() {
    static i32 count1;
    static i32 count2 = 1;
    if (++count1 > 5) {
        count1 = 0;
        debinftype *effect = debtab[count2];
        if (effect != NULL && effect->native_data != NULL) {
            f32 now = effect->time_group == 4 ? panelglobaltime : globaltime;
            if (now > effect->last_render_time + 5.0f) {
                DmaDebTypes[--freeDmaDebType] = effect->native_data;
                debtab[count2]->native_data = NULL;
                if (debtab[count2]->scale != 1.0f)
                    edppDeleteEffect(count2);
            }
        }
        if (++count2 >= EDPP_MAX_TYPES)
            count2 = 1;
    }
}

void DebrisReleaseControlStackLock() {
}

void RndrStateBuildReflectionState(nuglobalrndrstate_s *) {
}

void xxxNuDisplayListUpdateSpecial(nuhspecial_s *) {
}

void DebrisSingleCollisionCheckScaleYFlag(i32, nuvec_s *, float, float, unsigned char) {
}

void DebrisSingleTorusCollisionCheckScaleYFlag(i32, nuvec_s *, float, float, unsigned char) {
}

void unref(unsigned char *, unsigned char *) {
}

void TBRESET() {
    TBGAMECOUNT = 0;
    TBDRAWCOUNT = 0;
    TBPLAYERCOUNT = 0;
    TBAICOUNT = 0;
}

void TBOPENFN(char *, i32) {
}

void RndrArrow(float, float, float, i32, i32) {
}

void TBCLOSEFN(char *, i32) {
}
