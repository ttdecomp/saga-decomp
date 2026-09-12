#pragma once

#include "gameapi/edtools/edgra.h"
#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/core/startup/main.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/menus/screens/movies.h"
#include "legoapi/render/core/render.h"
#include "legoapi/render/light/shadow.h"
#include "legoapi/world/area.h"
#include "legoapi/world/areas.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "legogame/game.h"
#include "legogame/startup.h"
#include "legogame/target.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numusic/numusic.h"
#include "nu2api/nusound/nusound.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "legoapi/world/world_shared.h"
#include "legoapi/world/levels/levels.h"

// ----------------------------------------------------------------------
// Main game loop (module batman.cpp).
//
// NuMain is the TCS application entry point installed from the platform
// layer: it owns the full main loop (area setup, per-frame update/draw of
// world, characters, menus and cut scenes, frame-rate smoothing) and the
// level-transition tail.  `Trailer` and `uberShader2` are the TU's data.
// ----------------------------------------------------------------------

extern "C" i32 NuMain(i32 argc, char **argv);

extern char *Trailer[3];
extern char uberShader2[];

// ----------------------------------------------------------------------
// Frame / render helpers (nu2api render + editor terrain swap)
// ----------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    float NuFrameEnd(void);
    i32 NuRndrBeginScene(i32);
    void NuRndrEndScene(void);
    void NuRndrEndSceneEx(i32);
    void NuRndrClear(i32, i32, float);
    void NuRndrGradClear(i32, i32, i32, float);
    void NuRndrSwapStreamBuffers(void);
    void NuRndrGlobalFrameCountPause(i32);
    void NuRndrShadowOnOff(i32);
    void NuRndrLine3dDbgFlush(void);
    void NuRndrShadPolys(struct numtl_s *material);
    void NuRndrFx(i32, void *);
    void NuLgtLaserDraw(i32);
    void NuMtlAnimateSetSpeedScale(float);
    void NuMtlAnimateSetMask(i32);
    void NuTexAnimSetMask(i32);
    void NuWaterSpeed(float);
    void NuWaterReset(void);
    void NuWindDraw(void);
    void NuFadeObjDraw(void);
    void edGraEnableTerrainSwap(void);
    void edGraDisableTerrainSwap(void);
    void edgraStopPage(i8);
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------
// World / level frame stepping
// ----------------------------------------------------------------------
void GameTiming(WORLDINFO_s *, float *);
void GameDisplaySettings(LEVELDATADISPLAY *, i32 *);
void LevelStreaming_Update(WORLDINFO_s *);
void UpdateCutBorders(void);
void DrawCutBorders(i32);
void FixUpLayers(void);
void ClearLevData(void);
void WorldInfo_StreamLevel(bgprocinfo_s *);
void LoadAreaCharacters(void);
void WorldInfo_ReArrangeBuffers(i32, i32);
void WorldInfo_Activate(void);
void WorldInfo_DrawScene(WORLDINFO_s *);
void WorldInfo_ClearAllIfScreenFaded(void);

// ----------------------------------------------------------------------
// Debris / particles / parts
// ----------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    void DebrisGlassInit(void);
    void Debris(i32);
    void DebrisSetTimeIncrement(float);
    void DebrisSetCutSceneMode(i32);
    void DebrisDraw(i32, i32);
    void DebrisDrawGlass(void);
    void DrawParts(i32);
    void UpdateParts(float);
    void SortDebrisRenderStack(void);
    void UpdateDebrisRenderStackPriority(void);
    void TerrainTrackFlush(void);
    void RestoreGameCut(void);
    void rtlFrameUpdate(float);
    void rtlProcessLights(void *, float);
    void SoundUpdate(float);
    void SoundKillAll(void);
    void NuBridgeDraw(i32);
    void NuBridgeUpdate(NUVEC *);
    void NuCameraSet(nucamera_s *);
    void NuIOS_RecordFlurryEvent(char *);
    void edanimUpdateObjects(float);
    i32 MenuInMemoryCard(void);
#ifdef __cplusplus
}
#endif

i32 GetMenuID(void);

void Particles_Start(WORLDINFO_s *);
void Particles_Stop(WORLDINFO_s *);
void Parts_Start(WORLDINFO_s *);
void Parts_Stop(WORLDINFO_s *);
void UpdateSpecialSfx(WORLDINFO_s *);
void DebrisKillPlayers(void);
void AddCameraRain(WORLDINFO_s *, i32);
void UpdateRippleSet(ripple_set_s *);
void DrawRippleSet(ripple_set_s *);
void UpdateExplosions(void);
void DrawExplosions(void);
void DrawStreaks(void);
void UpdateStreaks(float);
void ZipUps_DrawLines(void);
void DrawCables(void);
void UpdateCables(void);
void Detonators_Update(void);
void Detonators_Draw(void);
void Batarangs_Update(void);
void Batarangs_Draw(void);
void SpeedBlur_Update(void);
void SpeedBlur_Apply(WORLDINFO_s *);
void SetDepthOfField(void);
void BurnoutApply(i32);
void EnableShadowMapRendering(i32);
void ResetShadowMapRendering(void);
void DrawGameObjects(void);
i32 DrawGameObjectsProcess(void);
void ManageGameObjects(void);
void UpdateGameObjects(WORLDINFO_s *);
void GameObjectStuffAfterAnimation(void);
void CharShadows_Update(void);
void CharShadows_Draw(void);
void UpdateCoinPacket(COINPACKET_s *, i32, i32);
void UpdateGameMenu(GAMEPAD_s *, i32);
void ViewCamDraw(void);
void ViewCamSetActive(i32, GAMEPAD_s *);
void DrawParallax(nuhspecial_s *);
void BackDrop_Update(float);
void BackDrop_Draw(float, i32);
void UpdateStats(void);
void Mission_Clear(MISSIONSYS_s *);
char IsGrabbingScreen(void);
void SetTexAnimSignals(void);
void ClearGizAIMessageSys(GIZAIMESSAGESYS_s *);
void ResetGizAIMessageSys(GIZAIMESSAGESYS_s *);

void DoInput(WORLDINFO_s *);
void PortalDoors_Update(WORLDINFO_s *);
void GameAISysStartFrame(AISYS_s *);
void Shadow_SetMode(void);
void GizmoPickups_SetOnOff(void);
void ProcessMusicChanges(LEVELDATA_s *, OPTIONSSAVE_s *);
i32 GamePlayMusic(LEVELDATA_s *, i32, OPTIONSSAVE_s *);
void UpdateLevelSfx(WORLDINFO_s *, i32);
void UpdateRumble(RUMBLEPACKET *);
void PauseGame(i32);
void ClearPause(void);
void NoRender(void);
void rtlProcessLights(void *, float);
void GameAnimSys_Update(GAMEANIMSYS_s *);

#ifdef __cplusplus
extern "C" {
#endif
    void NuWindUpdateArray(NUVEC **);
    void NuFadeObjUpdateArray(NUVEC **);
#ifdef __cplusplus
}
#endif
void CharPlatforms_Update(CHARPLATFORMSYS_s *);
void Grabber_Update(WORLDINFO_s *);
void Grabber_Draw(WORLDINFO_s *);
void TrafficAnimSys_Update(TRAFFICANIMSYS_s *);
void TrafficAnimSys_Draw(TRAFFICANIMSYS_s *);
void ProcessGizFlow(GIZFLOW_s *, float);
void AIPathCnxControlSysUpdate(AIPATHCNXCONTROLSYS_s *);
void ShoveObjectSysReset(void);
void Teleports_UpdateAfterGameObjects(WORLDINFO_s *);
void Teleports_UpdateBeforeGameObjects(WORLDINFO_s *);
void Pulses_Update(PULSESYS_s *);
void Level_Update(WORLDINFO_s *);
void Bolts_Update(WORLDINFO_s *);
void Bolts_Draw(WORLDINFO_s *);
void Tag_UpdateTransfers(i32, i32, i32);
void UpdateRepeatSfx(void);
void GizmoSysLateUpdate(GIZMOSYS_s *, void *, float);
void UpdateRadios(void);
void Hint_Process(float);
void Hint_CancelCurrent(void);
void Cheats_Update(void);
void UpdateGameMenu(GAMEPAD_s *, i32);
i32 MakePlayerList(i32);
void Faders_Draw(WORLDINFO_s *);
void Level_Draw(WORLDINFO_s *);
void SpecialMiniKits_Draw(WORLDINFO_s *);
void GameObjectToCameraDistances(void);
void SetCameraMatrices(void);
void MoveGameCamera(GAMECAMERA_s *);
void DrawTimer(i32, i32, i32);
void GameFog_Update(WORLDINFO_s *);
void GameFog_Set(void);
void SetLevelLights(void *, float);
void SetSpotLightMode(void);
void RenderShadowLights(i32);
void NuRndrShadPolys(struct numtl_s *material);
void NuLgtLaserDraw(i32);

// ----------------------------------------------------------------------
// Cut scenes
// ----------------------------------------------------------------------
void CutScenes_Start(WORLDINFO_s *);
void CutScenes_Update(WORLDINFO_s *, i32);
void CutScenes_Draw(WORLDINFO_s *);
void CutScenes_Stop(CUTSYS *);
void CutScenes_End(void);
void CutScenes_BGLoadManager(void);
void FindGameCutScenes(void);
void UpdateBackgroundMusic(void);
void WidescreenCode(i32);
void InitPanel(i32);
void NewMenu(i32, i32, i32);

// ----------------------------------------------------------------------
// Player / character misc
// ----------------------------------------------------------------------
void LSW_SetIndy(i32);
void ResetCharacterBuffer(i32);
void GameAISysSetGame(void);
void GizmoSysSetGame(void);
void LoadAreaCharacters(void);
void RememberPlayerIDs(i32, i32, i32);
i32 UpdateAchievements(STATUSPACKET_s *);
void InitStatusScreen(WORLDINFO_s *);

// ----------------------------------------------------------------------
// Music / audio
// ----------------------------------------------------------------------
void MusicClearAll(void);
void legoSetMusicVolume(float);
float GameSetMusicVolume(OPTIONSSAVE_s *);

#ifdef __cplusplus
extern "C" {
#endif
    i32 NuSound3SetReverb(i32 mode);
    void NuSound3SetDPL(i32, i32);
    void NuSound3SetRumblePads(nupad_s *, nupad_s *);
    void NuSound3UpdateRumble(float);
    void NuSound3Update(void);
    void NuSound3StopRumble(void);
    void NuSound3FlushLoops(void);
#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------
// Misc game systems
// ----------------------------------------------------------------------
void ResetTimer(TIMER_s *, float);
void ResetFrameCounters(void);
void UpdateFrameCounters(void);
void CheckResetBits(void);
void EndChallenge(i32, i32);
i32 Game_Exit(i32);
void GameCam_HitRoll(void);
void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);
void FreeGameObjectLights(void);
void ClearUpAreaData(void);
void StoreStatusTakeOverObjectSys(void);
void ReleaseAllTakeOvers(void);
void StoreLevelProgress(WORLDINFO_s *);
void ClearAreaProgress(i32, i32);
void Hub_MakeModelList(void);
void TerrainPlatformOldUpdate(void);
void TerrainPlatformNewUpdate(void);
void GrabStillScreen(void);
void HandleStillRender(void);
void PanelRender(WORLDINFO_s *);
void InitCables(WORLDINFO_s *);
void InitSnakes(WORLDINFO_s *);
void NewArea(void);
i32 NuIOS_AreInAppPurchasesAvailable(void);
i32 NuIOS_CanMakeInAppPurchases(void);

// ----------------------------------------------------------------------
// Frame bookkeeping globals (owned by other TUs; declared here since the
// main loop reads/writes them).
// ----------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
    extern MENU GameMenu[10];
    extern i32 GameMenuLevel;
    extern GAMEPAD_s GamePad[64];
    extern MENUFNINFO MenuInfo[100];
    extern OPTIONSSAVE TempOptions;
    extern i32 abort_load;
    extern f32 AddCoinDelay[2];
    extern i32 adaptivedifficulty[3];
    extern i32 back_rgba[2];
    extern TIMER BonusTimer;
    extern f32 brickimpactwait;
    extern i32 BURNOUTON;
    extern VARIPTR characterbuffer_base;
    extern VARIPTR characterbuffer_end;
    extern VARIPTR characterbuffer_ptr;
    extern f32 chattersfxwait;
    extern i32 clear_screen_onstill;
    extern f32 coinimpactwait;
    extern i32 COMPLEXSHADOWS;
    extern i32 CUTDRAWWORLD;
    extern i32 CutSceneWaiting;
    extern i32 dagobah_training;
    extern i32 DoubleScore;
    extern i32 drawcharactermodel_nobsa;
    extern i32 DRAWCMODELCALLS;
    extern i32 editor_active;
    extern i32 enable_zero_frametime;
    extern i32 FinishLoop_On;
    extern LANGUAGEDATA Game_LanguageList[7];
    extern OPTIONSSAVE *Game_OptionsSave;
    extern i32 (*GamePads_IgnoreInputFn)(void);
    extern i32 g_introState;
    extern i32 gone_through_door_to_new_level;
    extern i32 Grass_Available;
    extern f32 g_val;
    extern i32 highallocaddr;
    extern i32 HubMainRenderTimeHack;
    extern AREADATA *LastAData;
    extern i32 loadareacharacters_loadedlevel;
    extern f32 MainRenderTargetTime;
    extern f32 MainRenderTime;
    extern i32 makefreeplaymodellist;
    extern i32 memcard_autosavedisabled;
    extern i32 memcard_autosaveenabled;
    extern i32 menu_i_pack;
    extern i32 newlevel_resumecutaudio;
    extern i32 NewMode;
    extern i32 nurndr_tritot_this_frame;
    extern void (*NuSoundAppTerminateCallback)(void);
    extern i32 nuvideo_global_vbcnt;
    extern i32 PANELOFF;
    extern i32 party_cant_be_under_cover;
    extern i32 peak_poly_count;
    extern i32 PlayTrailer;
    extern NUVEC plr_lastpos;
    extern i32 poly_count;
    extern i32 RAYCASTCALLS;
    extern i32 reset_area;
    extern i32 reset_load;
    extern i32 ResetOldFStop;
    extern ripple_set_s *ripples;
    extern f32 sabrerubwait;
    extern i32 save_paused;
    extern i32 screendump;
    extern i32 SHADOWCALLS;
    extern numtl_s *ShadowMat;
    extern STATUSPACKET_s StatusPacket;
    extern u8 status_plr_active[8];
    extern i32 SuperStory;
    extern i32 TERRAINCALLS;
    extern f32 tieoffsfxwait;
    extern f32 tieonsfxwait;
    extern i32 waiting_for_character;
#ifdef __cplusplus
}
#endif
