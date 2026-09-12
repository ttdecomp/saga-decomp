#include "globals.h"

extern GAMESAVE_s TempGame;
#include "batman.h"
#include "gameapi/gui/apimenu.h"
#include "legoapi/characters/core/character.h"
#include "gameapi/ai/aisys/aisys.h"
#include "legoapi/render/fx.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/gizmo/base/GizObstacleObjectInterface.h"
#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/menus/screens/gamemenuall.h"
#include "legoapi/props/doors/door.h"
#include "legoapi/render/core/render.h"
#include "legoapi/world/area.h"
#include "legoapi/world/mission.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/world/levels/episode.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuqfnt.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"

#include <stdio.h>
#include <string.h>

extern void Customiser_LoadAll(CUSTOMISER *, WORLDINFO_s *);
extern void Customiser_Init(CUSTOMISER *);
extern void Customiser_Reset(CUSTOMISER *);
extern void Customiser_Draw3D(CUSTOMISER *);
extern void Customiser_Update(CUSTOMISER *, WORLDINFO_s *);
extern void Store_RestorePurchases();
void Hub_ResetPanel();
extern void Store_HubDrawFloorTargets(WORLDINFO_s *);
extern void Store_HubInitFloorTargets(WORLDINFO_s *);
extern void InitShop(WORLDINFO_s *);
extern void DrawShop3D(WORLDINFO_s *);
extern void Draw3DObjectMtx(WORLDINFO_s *, i32, NUMTX *);
extern void CutScenePlayer_Reset();
extern GIZBUILDIT_s *GizBuildIt_Find(WORLDINFO_s *, char *);
extern void GizBuildIt_SetToEnd(GIZBUILDIT_s *);
extern GIZBUILDIT_s *GizBuildIt_FindNearest(WORLDINFO_s *, GameObject_s *, BUILDIT_FIND_ENUM, i32);
extern FadeSystem FadeSys;
extern GAMESAVE_s TempGame;
extern i32 shop_save_done;
extern i32 shop_quit;
extern f32 ShopLockedScale;
extern HINTSYS_s hintsys;
extern i32 only_process_this_hint_id;
extern u8 show_unlock_shop_hint;
extern void *GetHintFromUIButton();
extern "C" void NuIOS_RecordFlurryEvent(char *);
bool HubShopUnlocked();
bool HubCustomiserUnlocked();
extern i32 Customiser_MenuAvailable(CUSTOMISER *);
extern GAMESAVE_s OldCustomiseGame;
extern i32 customiser_save_done;
extern i32 customiser_quit;
extern i32 customiser_changed;
extern u8 show_unlock_customiser_hint;
extern i32 Missions_PartyAvailable(MISSIONSYS_s *);
extern "C" i32 TriggerAutoSave();
extern AREADATA *E1VEHICLE_ADATA;
extern u8 show_unlock_minikitviewer_hint;
bool HubMinikitViewerUnlocked();
void Hub_ResetPanel();
void Hub_ActivateDoorMenu(LEVELDATA_s **);
bool FreePlayUnlocked();
extern "C" void PlaySfxAndSetPitch(char *, NUVEC *, f32);
AILOCATOR_s *LocalGetNearestLocator(AILOCATOR_s **, i32, f32, NUVEC *, f32, i32, f32, f32);
extern i32 Episode_CountOpenAreas(i32, i32, AREASAVE_s *);
extern void UpdateCharacterLoad();
extern i32 Missions_PartyAvailable(MISSIONSYS *);
extern void ResetForceBack();
extern void Hint_CancelCurrent();
extern i32 shop_save_done;
extern i32 shop_quit;
extern "C" void NuIOS_RecordFlurryEvent(char *);
bool HubShopUnlocked();
extern f32 SeekLinearF(f32, f32, f32);
extern i32 qrand();
extern void Door_GoThrough(WORLDINFO_s *, DOOR_s *, i32);
extern STOREPACK StorePack[11];
extern GameObject_s *FindGameObject(i32, u32, i32, i32, i32);
extern void Store_RootPackCustodian(i32, GameObject_s *);
extern void NeedScreenGrab(i32);
extern void BackDrop_ResetColours();
extern void NewGameMode();
extern f32 MainRenderTargetTime;
extern void ResetIconWibble();
extern void MakeFreePlayModelList(i32 first_model, i32 second_model, i32 area, i32 level, i32 include_bonus);
extern void GameDrawMenuEntry(MENU *menu, char *text);
extern i32 GameAudio_GetSfxId(i32 sfx);
extern i32 MenuSFX;
extern char **TTab;
extern i16 tPLAYER1;
extern i16 tPLAYER2;
extern i16 tPLAY;
extern f32 ICONX;
extern f32 ICONSIZE;
extern i32 DrawPanel3DObjectNoAlpha(f32, f32, f32, f32, f32, f32, u16, u16, u16, nuhspecial_s *, i32);
extern void NewLevelFromMenu(LEVELDATA_s *level, i32 menu_id, i32 menu_y, i32 remember_hub);

// These two arrays are generic level-loader state rather than hub-owned state.
extern u64 LevHSpecialExists;
extern GIZBUILDIT_s *LevBuildIt[4];

typedef void (*HUBCALLBACK)(WORLDINFO_s *);

HUBCALLBACK Hub_UpdateAIFn = NULL;
HUBCALLBACK Hub_ResetAIFn = NULL;
HUBCALLBACK Hub_InitAIFn = NULL;

u8 Hub_LowEnd_IconsInsteadOfModels = 0;
f32 Hub_PreventDropOutTime = 0.0f;
f32 Hub_HologramTargetAlpha = 0.0f;
f32 Hub_HologramAlpha = 0.0f;
f32 hub_jabbaawake = 0.0f;
i32 hub_new_level = 0;
f32 hub_buildit_time = 0.0f;
i32 hub_buildit = 0;
f32 hub_area_time = 0.0f;
i32 hub_area = 0;
f32 hub_episode_time = 0.0f;
i32 hub_episode = 0;
f32 hub_minikitarea_opentime = 0.0f;
f32 hub_minikitarea_time = 0.0f;
i32 hub_minikitarea = 0;
i32 last_hub_area = 0;
f32 statstime = 0.0f;
f32 cointotaltime = 0.0f;
f32 goldbricktime = 0.0f;
u8 hub_custodians_finished_loading = 0;
i32 hub_freeplay_area = 0;
i32 freeplaymode = 0;
i32 hub_selectmode = 0;
f32 selectmodetime = 0.0f;
i32 selectmodemode = 0;
i32 freeplay_selected[2] = {};
f32 uprepeattime[2] = {};
f32 rightrepeattime[2] = {};
f32 downrepeattime[2] = {};
f32 leftrepeattime[2] = {};
u8 uprepeatcount[2] = {};
u8 rightrepeatcount[2] = {};
u8 downrepeatcount[2] = {};
u8 leftrepeatcount[2] = {};
u8 hub_makefreeplaylist_addotherid = 0;

static i32 buildits_reset = 0;
static GIZMO *hub_minikitviewer_gizmo = NULL;
NUGSPLINE *hub_minikitviewer_camspl = NULL;
static f32 freeplaytime = 0.0f;
static f32 freeplayduration = 0.0f;
static f32 selectmodeduration = 0.0f;
static i32 hub_bonusarea;
static i32 hub_bonusepisode;
static i32 bonusmodemode;
static i32 fpcount = 0;
static APICHARACTERMODELLIST_s fplist[341] = {};
static f32 stats_xscale = 1.0f;
static i32 TJTYPEA = 96;
static i32 hub_drawminikitcount_charkit = 0;
static i8 i_selectminikitepisode = 0;
static i32 hub_minikitviewer_area = 0;
static f32 hub_minikitviewer_alpha = 0.0f;
extern f32 Hub_PadSpeed[2];
extern u16 Hub_PadAngle[2];
f32 hub_minikitviewer_movewait;
f32 hub_minikitviewer_move;
static NUVEC hub_minikitviewer_pos = {-27.0f, 0.1f, -22.125f};
void NewRumbleAllPlayers(f32, f32, i32, i32);
extern "C" void PlaySfx(char *, NUVEC *);
void PushAway(NUVEC *, f32, NUVEC *, NUVEC *, GameObject_s *, GameObject_s *, f32, u32);

static char *EpisodeNumerals[6] = {"I", "II", "III", "IV", "V", "VI"};

u32 HUB_EPISODER = 255;
u32 HUB_EPISODEG = 191;
u32 HUB_EPISODEB = 0;
f32 HUB_EPISODETITLESIZE = 0.5f;
f32 HUB_EPISODESUBTITLESIZE = 0.6f;

extern f32 HUB_EPISODETITLEY, HUB_EPISODESUBTITLEY;
extern f32 PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITSCALE, PANEL_MINIKITY, PANEL_MINIKITCOUNTY, PANEL3DMULX;
extern i32 newgamecam;
extern f32 newgamecamtime;
extern i16 tMAP, tSTORY, tFREEPLAY, tCHAPTER, tBONUS2, tMINIKITS, tMINIKIT;
extern i16 tBOUNTYHUNTERMISSIONS, tBOUNTYHUNTERMISSIONS2, tSTORYCLIPS2;
extern i32 EpMiniKitCount, EpMiniKitTotal, EpCharKitCount, EpCharKitTotal, EpBuildUpCount, EpBuildUpTotal;
extern i32 EpStoryBuildUpCount, EpStoryBuildUpTotal, EpFreePlayBuildUpCount, EpFreePlayBuildUpTotal;
extern i32 EpRedBrickCount, EpRedBrickTotal, EpGoldBrickCount, EpGoldBrickTotal, EpCompleteCount;
extern u8 MENUNORMALR, MENUNORMALG, MENUNORMALB, MENUENTRYR, MENUENTRYG, MENUENTRYB;
extern u8 MENUFLASH0R, MENUFLASH0G, MENUFLASH0B, MENUFLASH1R, MENUFLASH1G, MENUFLASH1B;
extern f32 menu_pulse, menu_pulsate;
extern i32 menu_flash;
extern AREADATA *SENATE_ADATA;
void DrawShopPanel();
i32 Missions_NumCompleted(MISSIONSYS *, MISSIONSAVE *, i32);
void DrawBuildUpBar(f32, f32, i32, i32, f32, f32, f32, u16);
void Hub_DrawImportantBrick(i32, f32, f32, f32, i32, i32);
void Hub_DrawAreaStats(f32, i32, i32);
static void Hub_DrawMiniKitCount(f32, f32, i32, i32, f32);
static void Hub_DrawSuperBonusStats(AREADATA *, f32);
static void Hub_DrawArcadeStats(f32);

static void Hub_DrawBonusStats(f32 alpha, i32 area, i32 episode, i32) {
    if (episode == -1 && area != -1)
        episode = static_cast<i8>(ADataList[area].episode_index);
    if (episode != -1) {
        Episode_FindAreaFromFlags(&EDataList[episode], 5, 4);
        Episode_FindAreaFromFlags(&EDataList[episode], 5, 5);
        SmartTextEx(TTab[tSTORYCLIPS2], 0.0f, HUB_EPISODESUBTITLEY, 1.0f, HUB_EPISODESUBTITLESIZE,
                    HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, 0, HUB_EPISODER, HUB_EPISODEG, HUB_EPISODEB, 1.7f,
                    1, NULL, 0, static_cast<i32>(alpha * 128.0f));
    }
}

static const NUVEC Hub_PercentPos = {-26.713f, 0.7f, -48.777f};
static const NUVEC ClipsSignOffset = {-0.114f, -0.0385f, -0.045f};

static inline void Hub_SetStatsTextMtx(NUMTX *mtx, i32 angle) {
    const f32 sine = NU_SIN_LUT(angle);
    const f32 cosine = NU_COS_LUT(angle);
    mtx->m00 = cosine;
    mtx->m01 = 0.0f;
    mtx->m02 = -sine;
    mtx->m03 = 0.0f;
    mtx->m10 = 0.0f;
    mtx->m11 = 1.0f;
    mtx->m12 = 0.0f;
    mtx->m13 = 0.0f;
    mtx->m20 = sine;
    mtx->m21 = 0.0f;
    mtx->m22 = cosine;
    mtx->m23 = 0.0f;
    mtx->m30 = 0.0f;
    mtx->m31 = 0.0f;
    mtx->m32 = 0.0f;
    mtx->m33 = 1.0f;
}

void Hub_ClearStats();
void Hub_UpdateMiniKits(WORLDINFO_s *);
void Hub_InitFreePlaySelect(i32, i32, i32);
void WipeBackToHub();
static void Hub_MakeFreePlayList(i32 first_model, i32 second_model);

enum HUB_DOOR_MENU_ID {
    HUB_DOOR_MENU_STANDARD = 15,
    HUB_DOOR_MENU_BONUS = 16,
    HUB_DOOR_MENU_VEHICLE = 17,
    HUB_DOOR_MENU_EPISODE_BONUS = 18,
};

enum HUB_VEHICLE_COLLECTION_MASK : u32 {
    HUB_VEHICLE_COLLECTION_ALLOWED = 0x04002000,
    HUB_VEHICLE_COLLECTION_REQUIRED = 0x00002000,
};

enum HUB_FREEPLAY_MODEL_FLAGS : u32 {
    HUB_FREEPLAY_MODEL_VEHICLE = 0x00002000,
    HUB_FREEPLAY_MODEL_MINIKIT = 0x04000000,
};

enum HUB_AUDIO_EVENT {
    HUB_AUDIO_EVENT_OPEN_DOOR_MENU = 0x2d,
};

enum {
    HUB_VEHICLE_ID_CAPACITY = 346,
};

struct HUBAREAINFO_s {
    const char *area_name;
    const char *door_name;
    const char *lock_name;
    const char *bonus_gizmo_name;
    const char *bonus_gizmo_name_2;
    f32 panel_offset;
    f32 panel_scale;
    i32 flags;
    AREADATA_s *area;
    GIZMO *door;
    nuhspecial_s lock;
    GIZMO *bonus_gizmo;
    GIZMO *bonus_gizmo_2;
};

struct HUBEPISODEINFO_s {
    i16 episode;
    i16 flags;
    const char *door_name;
    const char *lock_on_name;
    const char *lock_off_name;
    u8 force_open;
    EPISODEDATA *data;
    GIZMO *door;
    nuhspecial_s lock_on;
    nuhspecial_s lock_off;
};

#if UINTPTR_MAX == 0xffffffff
DECOMP_ASSERT(sizeof(HUBAREAINFO_s) == 0x3c, "HUBAREAINFO_s size");
DECOMP_ASSERT(sizeof(HUBEPISODEINFO_s) == 0x34, "HUBEPISODEINFO_s size");
#endif

// The original table is ordered exactly like areas.txt.  The last two entries
// are the network/bonus doors and the null record terminates every hub scan.
static HUBAREAINFO_s HubAreaInfo[] = {{"negotiations", "de1_1", "lock_1_1_on", NULL, NULL, 0.1f, 1.0f, 1},
                                      {"gungan", "de1_2", "lock_1_2_on", NULL, NULL, 0.025f, 0.8f, 1},
                                      {"palacerescue", "de1_3", "lock_1_3_on", NULL, NULL, 0.0f, 0.5f, 1},
                                      {"podsprint", "de1_4", "lock_1_4_on", NULL, NULL, 0.025f, 1.0f, 1},
                                      {"retakepalace", "de1_5", "lock_1_5_on", NULL, NULL, 0.075f, 1.0f, 1},
                                      {"maul", "de1_6", "lock_1_6_on", NULL, NULL, 0.225f, 1.0f, 1},
                                      {"e1vehiclebonus", "de1_7", "lock_1_7_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"pursuit", "de2_1", "lock_2_1_on", NULL, NULL, 0.125f, 1.0f, 1},
                                      {"kamino", "de2_2", "lock_2_2_on", NULL, NULL, 0.075f, 1.0f, 1},
                                      {"factory", "de2_3", "lock_2_3_on", NULL, NULL, 0.25f, 1.0f, 1},
                                      {"jedi", "de2_4", "lock_2_4_on", NULL, NULL, 0.0125f, 1.0f, 1},
                                      {"gunship", "de2_5", "lock_2_5_on", NULL, NULL, 0.11f, 1.0f, 1},
                                      {"dooku", "de2_6", "lock_2_6_on", NULL, NULL, 0.42f, 0.5f, 1},
                                      {"e2vehiclebonus", "de2_7", "lock_2_7_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"dogfight", "de3_1", "lock_3_1_on", NULL, NULL, 0.275f, 1.0f, 1},
                                      {"cruiser", "de3_2", "lock_3_2_on", NULL, NULL, 0.02f, 1.0f, 1},
                                      {"grievous", "de3_3", "lock_3_3_on", NULL, NULL, 0.1375f, 1.0f, 1},
                                      {"kashyyyk", "de3_4", "lock_3_4_on", NULL, NULL, 0.01f, 1.0f, 1},
                                      {"temple", "de3_5", "lock_3_5_on", NULL, NULL, 0.025f, 1.0f, 1},
                                      {"vader", "de3_6", "lock_3_6_on", NULL, NULL, 0.21f, 1.0f, 1},
                                      {"e3vehiclebonus", "de3_7", "lock_3_7_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"blockaderunner", "de4_1", "lock_4_1_on", NULL, NULL, 0.16f, 1.0f, 1},
                                      {"tatooine", "de4_2", "lock_4_2_on", NULL, NULL, 0.01f, 1.0f, 1},
                                      {"moseisley", "de4_3", "lock_4_3_on", NULL, NULL, 0.02f, 1.0f, 1},
                                      {"deathstarrescue", "de4_4", "lock_4_4_on", NULL, NULL, 0.17f, 1.0f, 1},
                                      {"deathstarescape", "de4_5", "lock_4_5_on", NULL, NULL, 0.025f, 1.0f, 1},
                                      {"deathstarbattle", "de4_6", "lock_4_6_on", NULL, NULL, 0.165f, 1.0f, 1},
                                      {"e4vehiclebonus", "de4_7", "lock_4_7_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"hothbattle", "de5_1", "lock_5_1_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"hothescape", "de5_2", "lock_5_2_on", NULL, NULL, 0.01f, 1.0f, 1},
                                      {"asteroidchase", "de5_3", "lock_5_3_on", NULL, NULL, 0.1825f, 1.0f, 1},
                                      {"dagobah", "de5_4", "lock_5_4_on", NULL, NULL, 0.165f, 1.0f, 1},
                                      {"cloudcitytrap", "de5_5", "lock_5_5_on", NULL, NULL, 0.1f, 1.0f, 1},
                                      {"cloudcityescape", "de5_6", "lock_5_6_on", NULL, NULL, 0.0125f, 1.0f, 1},
                                      {"e5vehiclebonus", "de5_7", "lock_5_7_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"jabbaspalace", "de6_1", "lock_6_1_on", NULL, NULL, 0.01f, 1.0f, 1},
                                      {"sarlaccpit", "de6_2", "lock_6_2_on", NULL, NULL, 0.01f, 0.8f, 1},
                                      {"speederchase", "de6_3", "lock_6_3_on", NULL, NULL, 0.165f, 1.0f, 1},
                                      {"endorbattle", "de6_4", "lock_6_4_on", NULL, NULL, 0.01f, 1.0f, 1},
                                      {"emperorfight", "de6_5", "lock_6_5_on", NULL, NULL, 0.245f, 1.0f, 1},
                                      {"deathstar2battle", "de6_6", "lock_6_6_on", NULL, NULL, 0.1825f, 1.0f, 1},
                                      {"e6vehiclebonus", "de6_7", "lock_6_7_on", NULL, NULL, 0.0f, 1.0f, 1},
                                      {"podrace", "de7_1", "lock_7_1_on", "frame_1", NULL, 0.0f, 0.0f, 1},
                                      {"anakinsflight", "de7_2", "lock_7_2_on", "frame_2", NULL, 0.0f, 0.0f, 1},
                                      {"bonus_gunship", "de7_3", "lock_7_3_on", "frame_3", NULL, 0.0f, 0.0f, 1},
                                      {"anewhope", "de7_4", "lock_7_4_on", "frame_4", NULL, 0.0f, 0.0f, 1},
                                      {"bonus2", "de7_5", "lock_7_5_on", "frame_5", NULL, 0.0f, 0.0f, 1},
                                      {"bonus", "de7_6", "lock_7_6_on", "frame_6", NULL, 0.0f, 0.0f, 1},
                                      {"losttemple", "de7_7", "lock_7_7_on", NULL, NULL, 0.0f, 0.0f, 0x101},
                                      {"senate", "network_door", NULL, NULL, NULL, 0.0f, 0.0f, 0x101},
                                      {}};

static HUBEPISODEINFO_s HubEpisodeInfo[] = {
    {0, 1, "DE1", "lock_1_on", "lock_1_off"},    {1, 1, "DE2", "lock_2_on", "lock_2_off"},
    {2, 1, "DE3", "lock_3_on", "lock_3_off"},    {3, 1, "DE4", "lock_4_on", "lock_4_off"},
    {4, 1, "DE5", "lock_5_on", "lock_5_off"},    {5, 1, "DE6", "lock_6_on", "lock_6_off"},
    {8, 1, "DE7", "lock_7_on", "lock_7_off", 1}, {6, 2, "hologram", "lock_8_on", "lock_8_off"},
    {7, 2, "jabba", "lock_B_on", "lock_B_off"},  {-1}};

f32 HUB_AREAPANELX_ONETRUEJEDIGOLDBRICK[6] = {-0.3f, 0.0f, 0.6f, -0.6f, 0.0f, 0.3f};
static f32 HUB_AREAPANELX_TWOTRUEJEDIGOLDBRICKS[6] = {-0.15f, -0.45f, 0.75f, -0.75f, 0.45f, 0.15f};
static f32 HUB_AREAPANELX_1TRUEJEDIGB_NOCHALLENGE[6] = {-0.201f, 0.201f, 0.6f, -0.6f, 0.0f, 0.0f};
f32 *HUB_AREAPANELX = HUB_AREAPANELX_ONETRUEJEDIGOLDBRICK;

static inline void Hub_DrawEpisodeCompletionSign(WORLDINFO_s *world, i32 episode) {
    nuhspecial_s *special = &HubAreaInfo[episode * 7 + 6].lock;
    if (NuSpecialExistsFn(special) == 0) {
        return;
    }

    NUMTX draw_mtx = *NuSpecialGetDrawMtx(special);
    const f32 direction_x = draw_mtx.m20;
    const f32 direction_z = draw_mtx.m22;
    NuMtxSetTranslation(&draw_mtx, const_cast<NUVEC *>(&ClipsSignOffset));
    NuMtxRotateY(&draw_mtx, NuAtan2D(direction_x, direction_z) + NUANG_90DEG);
    NuMtxTranslate(&draw_mtx, NuSpecialGetDrawPos(special));
    Draw3DObjectMtx(world, 316, &draw_mtx);
    Draw3DObjectMtx(world, Episode_IsComplete(&EDataList[episode], NULL) != 0 ? 320 : 319, &draw_mtx);
}

void Hub_Draw3D(WORLDINFO_s *world) {
    Store_HubDrawFloorTargets(world);

    for (i32 episode = 0; episode < 6; episode++) {
        Hub_DrawEpisodeCompletionSign(world, episode);
    }

    Customiser_Draw3D(CharacterCustomiser);
    DrawShop3D(world);
    Hub_DrawMiniKits(world);

    if (GameCam->sock_position.location.sock == 0) {
        if (Store_IsPackUnlocked(6) == 0) {
            if (NuSpecialExistsFn(&LevHSpecial[16]) != 0 && NuSpecialExistsFn(&LevHSpecial[4]) != 0) {
                NUMTX draw_mtx = *NuSpecialGetDrawMtx(&LevHSpecial[16]);
                NuSpecialDrawAt(&LevHSpecial[4], &draw_mtx);
            }
        }

        nuhspecial_s *display = &LevHSpecial[Store_IsPackUnlocked(5) != 0 ? 5 : 4];
        if (NuSpecialExistsFn(display) != 0) {
            NuSpecialDrawAt(display, &LevMtx);
        }
    }

    if (QFont3DZ != NULL) {
        const i32 spin = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 5.0f) / 5.0f * 65536.0f);
        const i32 angle = static_cast<i32>(NU_SIN_LUT(spin) * 2730.0f + 8192.0f);
        const i32 pulse = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 3.21f) / 3.21f * 65536.0f);
        const f32 bob = NU_SIN_LUT(pulse) * 0.01f;
        const i32 alpha = static_cast<i32>(Hub_HologramAlpha * 15.0f + 56.0f);
        const u32 colour = (static_cast<u32>(alpha) << 24) | 0xff7f00;

        char text[128];
        sprintf(text, "%.1f%%", static_cast<f32>(Game.completion * 100) / COMPLETIONPOINTS);
        Text_LocaliseDecimalPoint(text);

        NUMTX text_mtx;
        Hub_SetStatsTextMtx(&text_mtx, angle);
        NuMtxTranslate(&text_mtx, const_cast<NUVEC *>(&Hub_PercentPos));
        text_mtx.m31 += bob;

        NuQFntPushPrintMode(NUQFNT_CSMODE_ABSOLUTE);
        NuQFntSet(QFont3DZ);
        NuQFntSetMtx(QFont3DZ, &text_mtx);
        NuQFntSetCoordinateSystem(NUQFNT_CSMODE_ABSOLUTE);
        NuQFntSetColour(QFont3DZ, colour);
        NuQFntSetScale(QFont3DZ, 0.00375f, 0.005f);
        NuQFntMove(QFont3DZ, NuQFntPrintLenU(QFont3DZ, text) * -0.5f, 0.0f, 0.0f);
        NuQFntPrintU(QFont3DZ, text);
        NuQFntPopPrintMode();

        Text_MakeTime(Game.field30_0x7c2c, 1, 1, 1, text);
        Text_LocaliseDecimalPoint(text);
        Hub_SetStatsTextMtx(&text_mtx, angle);
        NuMtxTranslate(&text_mtx, const_cast<NUVEC *>(&Hub_PercentPos));
        text_mtx.m31 += bob + 0.16f;

        NuQFntPushPrintMode(NUQFNT_CSMODE_ABSOLUTE);
        NuQFntSet(QFont3DZ);
        NuQFntSetMtx(QFont3DZ, &text_mtx);
        NuQFntSetCoordinateSystem(NUQFNT_CSMODE_ABSOLUTE);
        NuQFntSetColour(QFont3DZ, colour);
        NuQFntSetScale(QFont3DZ, 0.0028124998f, 0.00375f);
        NuQFntMove(QFont3DZ, NuQFntPrintLenU(QFont3DZ, text) * -0.5f, 0.0f, 0.0f);
        NuQFntPrintU(QFont3DZ, text);
        NuQFntPopPrintMode();

        if (NuSpecialExistsFn(&LevHSpecial[15]) != 0) {
            Hub_SetStatsTextMtx(&text_mtx, angle);
            NuMtxTranslate(&text_mtx, NuSpecialGetDrawPos(&LevHSpecial[15]));
            NuSpecialDrawAtAlpha(&LevHSpecial[15], &text_mtx, Hub_HologramAlpha * 0.2f + 0.8f);
        }
    }
}

i32 Hub_InMenu() {
    const i32 menu = GetMenuID();
    if (menu == 8 || menu == 12 || menu == 13 || menu == 14 || (menu >= 15 && menu <= 23)) {
        return 1;
    }
    return 0;
}

void Hub_Update(WORLDINFO_s *world) {
    const i32 menu = GetMenuID();

    if (HubStartDoor != NULL) {
        DOOR_s *door = static_cast<DOOR_s *>(HubStartDoor);
        hub_new_level = door->level;
        Door_GoThrough(world, door, 1);
        HubStartDoor = NULL;
    }

    show_unlock_shop_hint = 0;
    show_unlock_customiser_hint = 0;
    show_unlock_minikitviewer_hint = 0;

    // Loading completion is tracked separately from the per-frame spawn pass.
    if (hub_custodians_finished_loading == 0) {
        if (BGLOAD == 0) {
            hub_custodians_finished_loading = 1;
        } else {
            i32 required = 0;
            i32 loaded = 0;
            for (i32 pack = 0; pack < 11; ++pack) {
                STOREPACK &store_pack = StorePack[pack];
                if (Store_IsPackUnlocked(pack) != 0 || store_pack.id == NULL) {
                    continue;
                }
                const i32 id = *store_pack.id;
                if (id == -1) {
                    continue;
                }
                ++required;
                if (APICharacterLoaded(id) != NULL) {
                    ++loaded;
                }
            }
            if (loaded == required) {
                hub_custodians_finished_loading = 1;
            }
        }
    }

    STOREPACK *current_pack = StorePack;
    for (i32 pack = 0; pack < 11; ++pack, ++current_pack) {
        STOREPACK &store_pack = *current_pack;
        if (store_pack.id != NULL) {
            const i32 id = *store_pack.id;
            if (id != -1 && Store_IsPackUnlocked(pack) == 0 &&
                (store_pack.field44_0x32 == 0xff ||
                 store_pack.field44_0x32 == static_cast<u8>(GameCam->sock_position.location.sock)) &&
                APICharacterLoaded(id) != NULL && FindGameObject(id, 0, 0, 0, 0) == NULL) {
                AIPATHINFO_s *path_info = NULL;
                AILOCATORSET *locator_set = AIPathFindLocatorSet(world->ai_sys, store_pack.custodian_locator_set);
                if (locator_set != NULL) {
                    AILOCATOR *locators[64] = {};
                    AILocatorSet_CheckLocatorsStillAssigned(world->ai_sys, locator_set);
                    i32 count = 0;
                    for (i32 i = 0; i < locator_set->locator_count && count < 64; ++i) {
                        if (locator_set->assigned[i] == 0xff) {
                            locators[count++] = &world->ai_sys->locators[locator_set->locator_entries[i]];
                        }
                    }
                    if (count != 0) {
                        AILOCATOR *locator = LocalGetNearestLocator(locators, count, 0.0f, &player->apiobj.position,
                                                                    1000000000.0f, 0, 1000000000.0f, 1000000000.0f);
                        if (locator != NULL) {
                            path_info = reinterpret_cast<AIPATHINFO_s *>(&locator->path);
                        }
                    }
                }
                GameObject_s *custodian =
                    AddDynamicCreature(id, &store_pack.custodian_position, store_pack.custodian_angle,
                                       const_cast<char *>("party"), path_info, NULL, 1, NULL, NULL, 0, 1);
                if (custodian != NULL) {
                    custodian->field_0xee8 = store_pack.custodian_position.x;
                    custodian->field_0xeec = store_pack.custodian_position.z;
                    custodian->field_0x106e = store_pack.custodian_angle;
                    Store_RootPackCustodian(pack, custodian);
                    AddGameDebris(world->debris_sys, 92, &custodian->apiobj.collision_position);
                }
            }
        }
    }
    if (buildits_reset == 0) {
        i32 buildit_index = 0;
        for (i32 i = 0; HubAreaInfo[i].area_name != NULL; ++i) {
            if (HubAreaInfo[i].bonus_gizmo != NULL) {
                if ((Game.field_0x7c26[2] & (1U << (buildit_index & 31))) != 0) {
                    GizBuildIt_SetToEnd(static_cast<GIZBUILDIT_s *>(HubAreaInfo[i].bonus_gizmo->object));
                }
                ++buildit_index;
            }
        }
        if ((Game.field_0x7c26[2] & 0x80) != 0 && LevGizmo[0] != NULL) {
            GizBuildIt_SetToEnd(static_cast<GIZBUILDIT_s *>(LevGizmo[0]->object));
        }
        ++buildits_reset;
    }

    const bool memory_card_menu = MenuInMemoryCard() != 0;
    if (memory_card_menu) {
        Hub_ClearStats();
        Hub_PreventDropOutTime = 1.0f;
    } else if (menu >= 12 && menu <= 14) {
        Hub_PreventDropOutTime = 1.0f;
    }

    const f32 pulse_time = NuFmod(GlobalTimer.time_elapsed_mod_seconds, 0.5f);
    TJTYPEA = static_cast<i32>(NU_SIN_LUT(static_cast<u16>((pulse_time + pulse_time) * 65536.0f)) * 16.0f + 80.0f);
    if (menu == 8) {
        hub_jabbaawake = 1.0f;
    } else if (hub_jabbaawake > 0.0f) {
        hub_jabbaawake -= FRAMETIME * 0.333f;
        if (hub_jabbaawake < 0.0f) {
            hub_jabbaawake = 0.0f;
        }
    }

    Hub_HologramAlpha = SeekLinearF(Hub_HologramAlpha, Hub_HologramTargetAlpha, FRAMETIME * 20.0f);
    if (Hub_HologramAlpha == Hub_HologramTargetAlpha) {
        Hub_HologramTargetAlpha = static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 2.0f - 1.0f;
    }
    if (Hub_PreventDropOutTime > 0.0f) {
        Hub_PreventDropOutTime -= FRAMETIME;
    }

    UpdateCharacterLoad();
    bool shop_available = false;
    if (SHOPACTIVE != 0) {
        DrawCoinTotalTime = 1.0f;
    } else if (GetMenuID() != 12) {
        for (i32 player_index = 0; player_index < 2; ++player_index) {
            GameObject_s *player = Player[player_index];
            if (player != NULL && static_cast<i8>(player->apiobj.flags_low) < 0 && player->apiobj.field_0x27d != 0 &&
                player->pad_gamepad->input_magnitude == 0.0f && player->pad_gamepad->previous_input_magnitude == 0.0f &&
                (player->field_0x7a5 == 0x31 || player->field_0x7a5 == 0xff || player->field_0x7a5 == 0x32) &&
                player->apiobj.field_0x281 == 0x0e) {
                shop_available = true;
            }
        }
        if (shop_available && !HubShopUnlocked()) {
            if (GetHintFromUIButton() != NULL ||
                (hintsys.active_hint != NULL && hintsys.active_hint->control_mode_ids[0] != 0x619)) {
                Hint_CancelCurrent();
                only_process_this_hint_id = 0x619;
            }
            show_unlock_shop_hint = 1;
            shop_available = false;
        }
    }
    if (LevTime[0] > 0.0f) {
        if (MenuInMemoryCard() == 0) {
            LevTime[0] -= FRAMETIME;
        }
    } else if (LevLock[0] != 0) {
        if (!shop_available) {
            LevLock[0] = 0;
        }
    } else if (GetMenuID() == -1 && shop_available) {
        SHOPACTIVE = 1;
        TempGame = Game;
        MakeMenuPacket();
        Hint_CancelCurrent();
        NewMenu(13, -1, -1);
        shop_save_done = 0;
        shop_quit = 0;
        ShopNameAlpha = 0.0f;
        ShopLockedScale = 1.0f;
        NuIOS_RecordFlurryEvent(const_cast<char *>("hubshop_enter"));
    }
    bool customiser_available = false;
    if (Customiser_MenuAvailable(CharacterCustomiser) != 0) {
        for (i32 player_index = 0; player_index < 2; ++player_index) {
            GameObject_s *player = Player[player_index];
            if (player != NULL && static_cast<i8>(player->apiobj.flags_low) < 0 && player->apiobj.field_0x27d != 0 &&
                player->pad_gamepad->input_magnitude == 0.0f && player->pad_gamepad->previous_input_magnitude == 0.0f &&
                (player->field_0x7a5 == 0x31 || player->field_0x7a5 == 0xff || player->field_0x7a5 == 0x32) &&
                player->apiobj.field_0x281 == 0x0f) {
                customiser_available = true;
            }
        }
        if (customiser_available && !HubCustomiserUnlocked()) {
            if (GetHintFromUIButton() != NULL ||
                (hintsys.active_hint != NULL && hintsys.active_hint->control_mode_ids[0] != 0x61a)) {
                Hint_CancelCurrent();
                only_process_this_hint_id = 0x61a;
            }
            show_unlock_customiser_hint = 1;
            customiser_available = false;
        }
    }
    if (LevTime[1] > 0.0f) {
        if (MenuInMemoryCard() == 0) {
            LevTime[1] -= FRAMETIME;
        }
    } else if (LevLock[1] != 0) {
        if (!customiser_available) {
            LevLock[1] = 0;
        }
    } else if (GetMenuID() == -1 && customiser_available) {
        OldCustomiseGame = Game;
        MakeMenuPacket();
        NewMenu(12, -1, -1);
        Hint_CancelCurrent();
        customiser_save_done = 0;
        customiser_quit = 0;
        customiser_changed = 0;
    }
    Customiser_Update(CharacterCustomiser, world);
    const i32 missions_available = Missions_PartyAvailable(NULL);
    bool mission_menu_available = false;
    if (missions_available != 0) {
        for (i32 player_index = 0; player_index < 2; ++player_index) {
            GameObject_s *player = Player[player_index];
            if (player != NULL && static_cast<i8>(player->apiobj.flags_low) < 0 && player->apiobj.field_0x27d != 0 &&
                player->pad_gamepad->input_magnitude == 0.0f && player->pad_gamepad->previous_input_magnitude == 0.0f &&
                (player->field_0x7a5 == 0x31 || player->field_0x7a5 == 0xff || player->field_0x7a5 == 0x32) &&
                player->apiobj.field_0x281 == 0x14) {
                mission_menu_available = true;
            }
        }
    }
    if (LevLock[3] != 0) {
        if (!mission_menu_available) {
            LevLock[3] = 0;
        }
    } else if (LevTime[3] > 0.0f) {
        if (MenuInMemoryCard() == 0) {
            LevTime[3] -= FRAMETIME;
        }
    } else if (GetMenuID() == -1 && mission_menu_available) {
        MakeMenuPacket();
        NewMenu(8, -1, -1);
        Hint_CancelCurrent();
    }
    for (i32 pack = 5; pack <= 6; ++pack) {
        GIZOBSTACLE_s *obstacle = LevGizObst[12 - pack];
        if (obstacle != NULL) {
            if (Store_IsPackUnlocked(pack) != 0) {
                obstacle->progress_flags |= 1;
                obstacle->runtime_flags &= static_cast<u8>(~8);
            } else {
                obstacle->progress_flags &= static_cast<u8>(~1);
                obstacle->runtime_flags |= 8;
            }
        }
    }
    if (FadeSys.fade == 0.0f) {
        i32 selected_episode = -1;
        for (i32 i = 0; static_cast<u16>(HubEpisodeInfo[i].episode) <= 8; ++i) {
            HUBEPISODEINFO_s &episode = HubEpisodeInfo[i];
            if (episode.door != NULL &&
                GizmoGetOutput(world->gizmo_sys, episode.door, static_cast<u16>(episode.flags), 0) != 0) {
                selected_episode = episode.episode;
                break;
            }
        }
        for (i32 i = 0; static_cast<u16>(HubEpisodeInfo[i].episode) <= 8; ++i) {
            HUBEPISODEINFO_s &episode = HubEpisodeInfo[i];
            if (episode.door == NULL) {
                continue;
            }
            GIZOBSTACLE_s *door = static_cast<GIZOBSTACLE_s *>(episode.door->object);
            if (selected_episode != -1 && selected_episode != episode.episode) {
                door->runtime_flags |= 8;
            } else if (static_cast<u8>(episode.force_open) != 0 ||
                       Episode_CountOpenAreas(episode.episode, -1, Game_AreaSave) != 0) {
                door->runtime_flags &= static_cast<u8>(~8);
            }
        }
        if (missions_available != 0) {
            // Original 0x1b762b writes the low byte of the Jabba entry's override.
            HubEpisodeInfo[8].force_open = (HubEpisodeInfo[8].force_open & ~0xff) | 1;
        }
        const i32 gold_bricks = Game.field_0x7c26[0];
        i32 buildit_index = 0;
        for (i32 i = 0; HubAreaInfo[i].area_name != NULL; ++i) {
            GIZMO *gizmo = HubAreaInfo[i].bonus_gizmo;
            if (gizmo == NULL) {
                continue;
            }
            if (GizmoGetOutput(world->gizmo_sys, gizmo, 0, 0) == 0) {
                GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
                if (gold_bricks < buildit->anim_object_count) {
                    GizmoSetVisibility(world->gizmo_sys, gizmo, 0, 0);
                    if (buildit->linked_buildit != NULL) {
                        buildit = buildit->linked_buildit;
                    }
                    for (i32 piece = 0; piece < buildit->anim_object_count; ++piece) {
                        if (buildit->anim_objects[piece] != NULL) {
                            NuSpecialSetVisibility(&buildit->anim_objects[piece]->special, piece < gold_bricks);
                        }
                    }
                } else if ((buildit->availability_flags & GIZBUILDIT_AVAILABILITY_ACTIVE) == 0) {
                    GizmoActivate(world->gizmo_sys, gizmo, 1, 0);
                }
            }
            if (GizmoGetOutput(world->gizmo_sys, gizmo, 0, 0) != 0) {
                const u32 bit = 1U << (buildit_index & 31);
                if ((Game.field_0x7c26[2] & bit) == 0) {
                    Game.field_0x7c26[2] = static_cast<u8>(Game.field_0x7c26[2] | bit);
                    TriggerAutoSave();
                }
            }
            ++buildit_index;
        }
        if (hub_episode_time == 0.0f) {
            hub_episode = selected_episode;
        }
        const f32 target = hub_episode != -1 && hub_episode == selected_episode ? 1.0f : 0.0f;
        hub_episode_time = SeekLinearF(hub_episode_time, target, FRAMETIME * 2.0f);
    } else {
        hub_episode_time = 0.0f;
        hub_episode = -1;
    }
    if (GameCam->sock_position.location.sock == 0) {
        NuSpecialSetVisibility(&LevHSpecial[16], Store_IsPackUnlocked(6));
    } else if (GameCam->sock_position.location.sock == 7) {
        if (NuSpecialExistsFn(&LevHSpecial[6]) != 0) {
            NuSpecialSetVisibility(&LevHSpecial[6], missions_available == 0);
        }
        if (NuSpecialExistsFn(&LevHSpecial[7]) != 0) {
            NuSpecialSetVisibility(&LevHSpecial[7], missions_available != 0);
        }
    }
    i32 completed_areas;
    Episode_IsComplete(HubEpisodeInfo[0].data, &completed_areas);
    if (completed_areas == 6) {
        HubAreaInfo[6].flags = (HubAreaInfo[6].flags & ~0xff00) | 0x100;
    }
    Episode_IsComplete(HubEpisodeInfo[1].data, &completed_areas);
    if (completed_areas == 6) {
        HubAreaInfo[13].flags = (HubAreaInfo[13].flags & ~0xff00) | 0x100;
    }
    Episode_IsComplete(HubEpisodeInfo[2].data, &completed_areas);
    if (completed_areas == 6) {
        HubAreaInfo[20].flags = (HubAreaInfo[20].flags & ~0xff00) | 0x100;
    }
    Episode_IsComplete(HubEpisodeInfo[3].data, &completed_areas);
    if (completed_areas == 6) {
        HubAreaInfo[27].flags = (HubAreaInfo[27].flags & ~0xff00) | 0x100;
    }
    Episode_IsComplete(HubEpisodeInfo[4].data, &completed_areas);
    if (completed_areas == 6) {
        HubAreaInfo[34].flags = (HubAreaInfo[34].flags & ~0xff00) | 0x100;
    }
    Episode_IsComplete(HubEpisodeInfo[5].data, &completed_areas);
    if (completed_areas == 6) {
        HubAreaInfo[41].flags = (HubAreaInfo[41].flags & ~0xff00) | 0x100;
    }
    i32 selected_area = -1;
    for (i32 i = 0; HubAreaInfo[i].area_name != NULL; ++i) {
        HUBAREAINFO_s &area = HubAreaInfo[i];
        if (area.area != NULL && area.door != NULL && GizmoGetOutput(world->gizmo_sys, area.door, 1, 0) != 0) {
            selected_area = area.area->index;
            break;
        }
    }
    if (selected_area != -1) {
        for (i32 i = 0; HubAreaInfo[i].area_name != NULL; ++i) {
            HUBAREAINFO_s &area = HubAreaInfo[i];
            if (area.door != NULL && area.area != NULL && area.area->index != selected_area) {
                static_cast<GIZOBSTACLE_s *>(area.door->object)->runtime_flags |= 8;
            }
        }
    } else {
        for (i32 i = 0; HubAreaInfo[i].area_name != NULL; ++i) {
            HUBAREAINFO_s &area = HubAreaInfo[i];
            if (area.door == NULL) {
                continue;
            }
            if (Episode_CountOpenAreas(static_cast<i8>(area.area->episode_index), area.area->index, Game_AreaSave) !=
                    0 ||
                (area.bonus_gizmo != NULL && GizmoGetOutput(world->gizmo_sys, area.bonus_gizmo, 0, 0) != 0) ||
                ((area.flags >> 8) & 0xff) == 1) {
                static_cast<GIZOBSTACLE_s *>(area.door->object)->runtime_flags &= static_cast<u8>(~8);
                NuSpecialSetVisibility(&area.lock, 1);
            }
        }
        static NUVEC minikit_hubpos = {-27.0f, 0.0f, -24.6f};
        if (E1VEHICLE_ADATA != NULL && NuVecXZDistSqr(&player->apiobj.position, &minikit_hubpos, NULL) < 0.25f) {
            selected_area = E1VEHICLE_ADATA->index;
            if (!HubMinikitViewerUnlocked()) {
                if (GetHintFromUIButton() != NULL ||
                    (hintsys.active_hint != NULL && hintsys.active_hint->control_mode_ids[0] != 0x61b)) {
                    Hint_CancelCurrent();
                    only_process_this_hint_id = 0x61b;
                }
                show_unlock_minikitviewer_hint = 1;
                selected_area = -1;
            }
        }
    }
    const i32 area_menu = GetMenuID();
    if (FadeSys.fade == 0.0f && area_menu == -1) {
        if (hub_episode != -1 && hub_episode_time > 0.0f) {
            selected_area = -1;
        }
        if (hub_area_time == 0.0f) {
            hub_area = selected_area;
            last_hub_area = selected_area;
        }
        const f32 target = hub_area != -1 && hub_area == selected_area ? 1.0f : 0.0f;
        hub_area_time = SeekLinearF(hub_area_time, target, FRAMETIME * 2.0f);
        if (hub_area_time == 1.0f && VEHICLES_ADATA != NULL && VEHICLES_ADATA->index == hub_area &&
            PLATFORM_LDATA != NULL) {
            NewLData = PLATFORM_LDATA;
            Hub_ActivateDoorMenu(&NewLData);
        }
    } else {
        Hub_ResetPanel();
    }
    for (i32 i = 0; HubAreaInfo[i].area_name != NULL; ++i) {
        HUBAREAINFO_s &area = HubAreaInfo[i];
        if (area.area == NULL || area.bonus_gizmo_2 == NULL ||
            GizmoGetOutput(world->gizmo_sys, area.bonus_gizmo_2, 1, 0) == 0) {
            continue;
        }
        NUVEC *position = GizmoGetPos(world->gizmo_sys, area.bonus_gizmo_2);
        if (position == NULL) {
            continue;
        }
        if (Player[0] != NULL && static_cast<i8>(Player[0]->apiobj.flags_low) < 0 &&
            NuVecDistSqr(&Player[0]->apiobj.position, position, NULL) < 0.25f) {
            break;
        }
        if (Player[1] != NULL && static_cast<i8>(Player[1]->apiobj.flags_low) < 0 &&
            NuVecDistSqr(&Player[1]->apiobj.position, position, NULL) < 0.25f) {
            break;
        }
    }
    i32 minikit_candidate = -1;
    if (hub_minikitviewer_gizmo != NULL && GizmoGetOutput(world->gizmo_sys, hub_minikitviewer_gizmo, 1, 0) != 0 &&
        hub_minikitviewer_area != -1 && FreePlayUnlocked()) {
        minikit_candidate = hub_minikitviewer_area;
    }
    if (LevLock[4] != 0) {
        if (minikit_candidate == -1) {
            LevLock[4] = 0;
        }
    } else if (LevTime[4] > 0.0f) {
        LevTime[4] -= FRAMETIME;
    }
    if (FadeSys.fade != 0.0f) {
        hub_minikitarea = -1;
        hub_minikitarea_time = 0.0f;
        hub_minikitarea_opentime = 0.0f;
    } else if (LevLock[4] == 0) {
        i32 selected_minikit = -1;
        if (!(hub_episode != -1 && hub_episode_time > 0.0f) &&
            !(hub_area != -1 && hub_area_time > 0.0f &&
              (E1VEHICLE_ADATA == NULL || hub_area != E1VEHICLE_ADATA->index)) &&
            minikit_candidate != -1) {
            for (i32 i = 0; i < 2; ++i) {
                if (Player[i] != NULL && static_cast<i8>(Player[i]->apiobj.flags_low) < 0 &&
                    Player[i]->pad_gamepad->input_magnitude == 0.0f) {
                    selected_minikit = hub_minikitviewer_area;
                }
            }
        }
        if (hub_minikitarea_time == 0.0f) {
            if (hub_minikitarea != selected_minikit) {
                hub_minikitarea_opentime = 0.0f;
            }
            hub_minikitarea = selected_minikit;
        }
        f32 target = 0.0f;
        if (hub_minikitarea != -1 && hub_minikitarea == selected_minikit) {
            hub_minikitarea_opentime += FRAMETIME;
            target = 1.0f;
        }
        if (GetMenuID() == 14) {
            target = 1.0f;
        }
        hub_minikitarea_time = SeekLinearF(hub_minikitarea_time, target, FRAMETIME * 2.0f);
        if (hub_minikitarea_opentime >= 1.0f) {
            MakeMenuPacket();
            Hint_CancelCurrent();
            NewMenu(14, -1, -1);
            PlaySfxAndSetPitch(const_cast<char *>("JForcePush"), NULL, 1.5f);
        }
    }
    Hub_UpdateMiniKits(world);

    GIZBUILDIT_s *nearest_buildit = GizBuildIt_FindNearest(WORLD, Player[0], BUILDIT_FIND_ANY, ShadowMode);
    if (FadeSys.fade == 0.0f) {
        i32 selected_buildit = -1;
        const bool player_can_build =
            Player[0] != NULL && static_cast<i8>(Player[0]->apiobj.flags_low) < 0 && Player[0]->field_0x7a5 != 0x2d;
        for (i32 i = 0; HubAreaInfo[i].area_name != NULL && selected_buildit == -1; ++i) {
            GIZMO *gizmo = HubAreaInfo[i].bonus_gizmo;
            if (gizmo != NULL && GizmoGetOutput(world->gizmo_sys, gizmo, 0, 0) == 0 && player_can_build) {
                GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
                if ((buildit->availability_flags & GIZBUILDIT_AVAILABILITY_INTERACTING) != 0 ||
                    (nearest_buildit != NULL && nearest_buildit == buildit)) {
                    selected_buildit = i;
                }
            }
        }
        if (LevGizmo[0] != NULL) {
            if (selected_buildit == -1 && GizmoGetOutput(world->gizmo_sys, LevGizmo[0], 0, 0) == 0) {
                if (player_can_build &&
                    ((LevBuildIt[0]->availability_flags & GIZBUILDIT_AVAILABILITY_INTERACTING) != 0 ||
                     (nearest_buildit != NULL && nearest_buildit == LevBuildIt[0]))) {
                    selected_buildit = 999;
                }
                // Original 0x1b7edc / 0x1b8880: the fountain displays one
                // progress piece per two gold bricks until all 160 are earned.
                const f32 gold_bricks = static_cast<f32>(Game.field_0x7c26[0]);
                if (gold_bricks < 160.0f) {
                    GizmoSetVisibility(world->gizmo_sys, LevGizmo[0], 0, 0);
                    const i32 visible_pieces = static_cast<i32>(static_cast<f32>(Game.field_0x7c26[0]) * 0.5f);
                    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(LevGizmo[0]->object);
                    if (buildit->linked_buildit != NULL) {
                        buildit = buildit->linked_buildit;
                    }
                    for (i32 i = 0; i < buildit->anim_object_count; ++i) {
                        if (buildit->anim_objects[i] != NULL) {
                            NuSpecialSetVisibility(&buildit->anim_objects[i]->special, i < visible_pieces ? 1 : 0);
                        }
                    }
                } else if ((static_cast<GIZBUILDIT_s *>(LevGizmo[0]->object)->availability_flags &
                            GIZBUILDIT_AVAILABILITY_ACTIVE) == 0) {
                    GizmoActivate(world->gizmo_sys, LevGizmo[0], 1, 0);
                }
            } else if (GizmoGetOutput(world->gizmo_sys, LevGizmo[0], 0, 0) != 0) {
                Game.field_0x7c26[2] |= 0x80;
            }
        }
        if (hub_buildit_time == 0.0f) {
            hub_buildit = selected_buildit;
        }
        const f32 target = hub_buildit != -1 && hub_buildit == selected_buildit ? 1.0f : 0.0f;
        hub_buildit_time = SeekLinearF(hub_buildit_time, target, FRAMETIME * 2.0f);
    } else {
        hub_buildit_time = 0.0f;
        hub_buildit = -1;
    }
    if (Hub_UpdateAIFn != NULL) {
        Hub_UpdateAIFn(world);
    }

    if (hub_episode != -1 || hub_area != -1 || hub_minikitarea != -1 || hub_buildit != -1) {
        Hint_CancelCurrent();
    } else {
        const i32 active_menu = GetMenuID();
        if (active_menu != -1 && active_menu != 13 && (active_menu != 15 || hub_new_level == -1)) {
            Hint_CancelCurrent();
        }
    }
    ResetForceBack();
}

void Hub_DrawPanel(WORLDINFO_s *) {
    char text[256];
    char title[128];
    if (newgamecam) {
        f32 alpha = 0.0f;
        if (newgamecamtime < 2.0f)
            alpha = 0.0f;
        else if (newgamecamtime < 2.5f)
            alpha = (newgamecamtime - 2.0f) * 2.0f;
        else if (newgamecamtime < 7.5f)
            alpha = 1.0f;
        else if (newgamecamtime < 8.0f)
            alpha = 1.0f - (newgamecamtime - 7.5f) * 2.0f;
        if (alpha > 0.0f)
            SmartTextEx(TTab[tMAP], 0.0f, (HUB_EPISODETITLEY + HUB_EPISODESUBTITLEY) * 0.5f, 1.0f, 0.8f, 0.8f, 0.8f, 0,
                        HUB_EPISODER, HUB_EPISODEG, HUB_EPISODEB, 1.7f, 1, NULL, 0, static_cast<i32>(alpha * 128.0f));
    }
    DrawShopPanel();

    if (hub_episode != -1 && hub_episode_time > 0.0f) {
        const i32 alpha = static_cast<i32>(128.0f * hub_episode_time);
        if (hub_episode == 6) {
            i32 mini_count = 0, mini_total = 0, char_count = 0, char_total = 0;
            i32 buildup_count = 0, buildup_total = 0, story_count = 0, story_total = 0;
            i32 free_count = 0, free_total = 0, red_count = 0, red_total = 0;
            for (i32 i = 0; i < EPISODECOUNT; ++i) {
                Episode_CountOpenAreas(i, -1, Game.area_save);
                mini_count += EpMiniKitCount;
                mini_total += EpMiniKitTotal;
                char_count += EpCharKitCount;
                char_total += EpCharKitTotal;
                buildup_count += EpBuildUpCount;
                buildup_total += EpBuildUpTotal;
                if (BOTHTRUEJEDIGOLDBRICKS) {
                    story_count += EpStoryBuildUpCount;
                    story_total += EpStoryBuildUpTotal;
                    free_count += EpFreePlayBuildUpCount;
                    free_total += EpFreePlayBuildUpTotal;
                }
                red_count += EpRedBrickCount;
                red_total += EpRedBrickTotal;
            }
            stats_xscale = 0.8f;
            if (BOTHTRUEJEDIGOLDBRICKS) {
                DrawBuildUpBar(HUB_AREAPANELX[1], HUB_EPISODESUBTITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100, 100,
                               NU_SIN_LUT(static_cast<i32>(16384.0f * hub_episode_time)), 1.0f, 1.0f, 0);
                sprintf(text, "%i/%i", story_count, story_total);
                Text3DEx(text, HUB_AREAPANELX[1], HUB_EPISODESUBTITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE * stats_xscale,
                         PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(alpha));
                SmartTextEx(TTab[tSTORY], HUB_AREAPANELX[1], 0.235f + HUB_EPISODESUBTITLEY, 1.0f, 0.45f, 0.45f, 0.45f,
                            0, 255, 255, 255, 0.4f, 1, NULL, 0,
                            static_cast<i32>(static_cast<f32>(TJTYPEA) * hub_episode_time));
                DrawBuildUpBar(HUB_AREAPANELX[4], HUB_EPISODESUBTITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100, 100,
                               NU_SIN_LUT(static_cast<i32>(16384.0f * hub_episode_time)), 1.0f, 1.0f, 0);
                sprintf(text, "%i/%i", free_count, free_total);
                Text3DEx(text, HUB_AREAPANELX[4], HUB_EPISODESUBTITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE * stats_xscale,
                         PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(alpha));
                SmartTextEx(TTab[tFREEPLAY], HUB_AREAPANELX[4], 0.235f + HUB_EPISODESUBTITLEY, 1.0f, 0.45f, 0.45f,
                            0.45f, 0, 255, 255, 255, 0.4f, 1, NULL, 0,
                            static_cast<i32>(static_cast<f32>(TJTYPEA) * hub_episode_time));
            } else {
                DrawBuildUpBar(HUB_AREAPANELX[1], HUB_EPISODESUBTITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100, 100,
                               NU_SIN_LUT(static_cast<i32>(16384.0f * hub_episode_time)), 1.0f, 1.0f, 0);
                sprintf(text, "%i/%i", buildup_count, buildup_total);
                Text3DEx(text, HUB_AREAPANELX[1], HUB_EPISODESUBTITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE * stats_xscale,
                         PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(alpha));
            }
            Hub_DrawMiniKitCount(HUB_AREAPANELX[0], HUB_EPISODESUBTITLEY, mini_count, mini_total, hub_episode_time);
            Hub_DrawImportantBrick(210, HUB_AREAPANELX[2], HUB_EPISODESUBTITLEY, hub_episode_time, red_count,
                                   red_total);
            Hub_DrawImportantBrick(211, HUB_AREAPANELX[3], HUB_EPISODESUBTITLEY, hub_episode_time, Game.gold_bricks,
                                   GOLDBRICKPOINTS);
            hub_drawminikitcount_charkit = 1;
            Hub_DrawMiniKitCount(HUB_AREAPANELX[5], HUB_EPISODESUBTITLEY, char_count, char_total, hub_episode_time);
            stats_xscale = 1.0f;
        } else if (hub_episode == 7) {
            if (MissionSys) {
                const i32 total = MissionSys->count;
                const i32 count = Missions_NumCompleted(MissionSys, &Game.mission_save, 0);
                Hub_DrawImportantBrick(211, 0.0f, HUB_EPISODETITLEY, hub_episode_time, count, total);
                SmartTextEx(TTab[tBOUNTYHUNTERMISSIONS2 ? tBOUNTYHUNTERMISSIONS2 : tBOUNTYHUNTERMISSIONS], 0.0f,
                            HUB_EPISODESUBTITLEY, 1.0f, HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE,
                            HUB_EPISODESUBTITLESIZE, 0, HUB_EPISODER, HUB_EPISODEG, HUB_EPISODEB, 1.7f, 1, NULL, 0,
                            alpha);
            }
        } else if (hub_episode == 8) {
            i32 gold_count = 0, gold_total = 0, buildup_count = 0, buildup_total = 0;
            for (i32 i = 0; i < AREACOUNT; ++i) {
                AREADATA *area = &ADataList[i];
                if (area == HUB_ADATA || (area->flags & 0x22) || area->episode_index != 0xff || (area->flags & 0x2010))
                    continue;
                if (area->flags & 0x100) {
                    if (GOLDBRICKFORSUPERBONUS) {
                        ++gold_total;
                        if (Game.area_save[i].area_complete)
                            ++gold_count;
                    }
                    continue;
                }
                if (area->flags & 4 || area->flags & 0x800)
                    continue;
                ++gold_total;
                if (Game.area_save[i].area_complete)
                    ++gold_count;
                if (area->flags & 0x4000) {
                    ++gold_total;
                    ++buildup_total;
                    if (Game.area_save[i].story_buildup_complete || Game.area_save[i].freeplay_buildup_complete) {
                        ++gold_count;
                        ++buildup_count;
                    }
                }
            }
            if (buildup_total) {
                Hub_DrawImportantBrick(211, -0.201f, HUB_EPISODETITLEY, hub_episode_time, gold_count, gold_total);
                DrawBuildUpBar(0.201f, HUB_EPISODETITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100, 100,
                               NU_SIN_LUT(static_cast<i32>(16384.0f * hub_episode_time)), 1.0f, 1.0f, 0);
                sprintf(text, "%i/%i", buildup_count, buildup_total);
                Text3DEx(text, 0.201f, HUB_EPISODETITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE,
                         PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127,
                         static_cast<u8>(static_cast<i32>(128.0f * hub_episode_time)));
            } else {
                f32 x = 0.0f;
                if (Game.indy_unlocked) {
                    Hub_DrawImportantBrick(251, 0.1f, HUB_EPISODETITLEY - 0.225f, hub_episode_time, -1, -1);
                    x = -0.1f;
                }
                Hub_DrawImportantBrick(211, x, HUB_EPISODETITLEY, hub_episode_time, gold_count, gold_total);
            }
            SmartTextEx(TTab[tBONUS2], 0.0f, HUB_EPISODESUBTITLEY, 1.0f, HUB_EPISODESUBTITLESIZE,
                        HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, 0, HUB_EPISODER, HUB_EPISODEG, HUB_EPISODEB,
                        1.7f, 1, NULL, 0, alpha);
        } else if (hub_episode >= 0 && hub_episode < EPISODECOUNT) {
            EPISODEDATA *episode = &EDataList[hub_episode];
            const f32 phase = hub_episode_time;
            Episode_CountOpenAreas(hub_episode, -1, Game.area_save);
            if (EpCompleteCount) {
                if (BOTHTRUEJEDIGOLDBRICKS) {
                    DrawBuildUpBar(HUB_AREAPANELX[1], HUB_EPISODETITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100,
                                   100, NU_SIN_LUT(static_cast<i32>(16384.0f * phase)), 1.0f, 1.0f, 0);
                    sprintf(text, "%i/%i", EpStoryBuildUpCount, EpStoryBuildUpTotal);
                    Text3DEx(text, HUB_AREAPANELX[1], HUB_EPISODETITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE,
                             PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(alpha));
                    SmartTextEx(TTab[tSTORY], HUB_AREAPANELX[1], 0.235f + HUB_EPISODETITLEY, 1.0f, 0.45f, 0.45f, 0.45f,
                                0, 255, 255, 255, 0.4f, 1, NULL, 0,
                                static_cast<i32>(static_cast<f32>(TJTYPEA) * phase));
                    DrawBuildUpBar(HUB_AREAPANELX[4], HUB_EPISODETITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100,
                                   100, NU_SIN_LUT(static_cast<i32>(16384.0f * phase)), 1.0f, 1.0f, 0);
                    sprintf(text, "%i/%i", EpFreePlayBuildUpCount, EpFreePlayBuildUpTotal);
                    Text3DEx(text, HUB_AREAPANELX[4], HUB_EPISODETITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE,
                             PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(alpha));
                    SmartTextEx(TTab[tFREEPLAY], HUB_AREAPANELX[4], 0.235f + HUB_EPISODETITLEY, 1.0f, 0.45f, 0.45f,
                                0.45f, 0, 255, 255, 255, 0.4f, 1, NULL, 0,
                                static_cast<i32>(static_cast<f32>(TJTYPEA) * phase));
                } else {
                    DrawBuildUpBar(HUB_AREAPANELX[1], HUB_EPISODETITLEY + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 100,
                                   100, NU_SIN_LUT(static_cast<i32>(16384.0f * phase)), 1.0f, 1.0f, 0);
                    sprintf(text, "%i/%i", EpBuildUpCount, EpBuildUpTotal);
                    Text3DEx(text, HUB_AREAPANELX[1], HUB_EPISODETITLEY, 1.0f, PANEL_MINIKITCOUNTSCALE,
                             PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(alpha));
                }
                Hub_DrawMiniKitCount(HUB_AREAPANELX[0], HUB_EPISODETITLEY, EpMiniKitCount, EpMiniKitTotal, phase);
                Hub_DrawImportantBrick(210, HUB_AREAPANELX[2], HUB_EPISODETITLEY, phase, EpRedBrickCount,
                                       EpRedBrickTotal);
                Hub_DrawImportantBrick(211, HUB_AREAPANELX[3], HUB_EPISODETITLEY, phase, EpGoldBrickCount,
                                       EpGoldBrickTotal);
                if (Store_IsPackUnlocked(8)) {
                    hub_drawminikitcount_charkit = 1;
                    Hub_DrawMiniKitCount(HUB_AREAPANELX[5], HUB_EPISODETITLEY, EpCharKitCount, EpCharKitTotal, phase);
                }
            } else if (episode->name_id != -1) {
                SmartTextEx(TTab[episode->name_id], 0.0f, HUB_EPISODETITLEY, 1.0f, HUB_EPISODETITLESIZE,
                            HUB_EPISODETITLESIZE, HUB_EPISODETITLESIZE, 0, HUB_EPISODER, HUB_EPISODEG, HUB_EPISODEB,
                            1.7f, 1, NULL, 0, alpha);
            }
            if (episode->text_id != -1)
                SmartTextEx(TTab[episode->text_id], 0.0f, HUB_EPISODESUBTITLEY, 1.0f, HUB_EPISODESUBTITLESIZE,
                            HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, 0, HUB_EPISODER, HUB_EPISODEG,
                            HUB_EPISODEB, 1.7f, 1, NULL, 0, alpha);
        }
    }
    if (hub_area != -1 && hub_area_time > 0.0f) {
        AREADATA *area = &ADataList[hub_area];
        if (GameCam->mode == 7 && (area->flags & 5) == 5) {
            i32 count = 0, total = 0;
            if (Game_AreaSave) {
                for (i32 i = 0; i < AREACOUNT; ++i) {
                    if ((ADataList[i].flags & 5) == 5) {
                        ++total;
                        if (Game_AreaSave[i].area_complete ||
                            Game_AreaSave[i].challenge_trial_time > static_cast<f32>(ADataList[i].challenge_trial_time))
                            ++count;
                    }
                }
            }
            Hub_DrawImportantBrick(211, 0.0f, HUB_EPISODETITLEY, hub_area_time, count, total);
            SmartTextEx(TTab[tMINIKITS ? tMINIKITS : tMINIKIT], 0.0f, HUB_EPISODESUBTITLEY, 1.0f,
                        HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, 0, HUB_EPISODER,
                        HUB_EPISODEG, HUB_EPISODEB, 1.7f, 1, NULL, 0, static_cast<i32>(128.0f * hub_area_time));
        } else if (area->flags & 0x100) {
            Hub_DrawSuperBonusStats(area, hub_area_time);
        } else if (area->flags & 4) {
            if (SENATE_ADATA && SENATE_ADATA->index == hub_area)
                Hub_DrawArcadeStats(hub_area_time);
            else
                Hub_DrawBonusStats(hub_area_time, hub_area, -1, -1);
        } else {
            Hub_DrawAreaStats(hub_area_time, hub_area, -1);
        }
    }
    if (GetMenuID() == 14) {
        const f32 phase = hub_minikitviewer_alpha;
        const i32 index = hub_minikitviewer_area;
        AREADATA *area = &ADataList[index];
        AREASAVE_s *save = &Game.area_save[index];
        const i32 alpha = static_cast<i32>(phase * 128.0f);
        if (save->minikit_complete) {
            const f32 scale = HUB_EPISODETITLESIZE;
            if (area->minikit_id != -1 && CDataList[area->minikit_id].name_id != -1)
                SmartTextEx(TTab[CDataList[area->minikit_id].name_id], 0.0f, HUB_EPISODETITLEY, 1.0f,
                            HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, HUB_EPISODESUBTITLESIZE, 0, HUB_EPISODER,
                            HUB_EPISODEG, HUB_EPISODEB, 1.7f, 1, NULL, 0, alpha);
            sprintf(title, "(%s)", TTab[area->name_id]);
            if (scale > 0.0f)
                SmartTextEx(title, 0.0f, HUB_EPISODESUBTITLEY, 1.0f, scale, scale, scale, 0, HUB_EPISODER, HUB_EPISODEG,
                            HUB_EPISODEB, 1.7f, 1, NULL, 0, alpha);
        } else if (save->minikit_count) {
            const f32 scale = HUB_EPISODESUBTITLESIZE;
            Hub_DrawMiniKitCount(0.0f, HUB_EPISODETITLEY, save->minikit_count, 10, phase);
            char *name = TTab[area->name_id];
            if (scale > 0.0f && name)
                SmartTextEx(name, 0.0f, HUB_EPISODESUBTITLEY, 1.0f, scale, scale, scale, 0, HUB_EPISODER, HUB_EPISODEG,
                            HUB_EPISODEB, 1.7f, 1, NULL, 0, alpha);
        } else if (area->area_index != 0xff && area->episode_index != 0xff) {
            const f32 scale = HUB_EPISODESUBTITLESIZE;
            const i8 episode = area->episode_index;
            Hub_DrawMiniKitCount(0.0f, HUB_EPISODETITLEY, 0, 10, phase);
            sprintf(title, "%s, %s %i", TTab[EDataList[episode].name_id], TTab[tCHAPTER],
                    static_cast<i8>(ADataList[index].area_index) + 1);
            if (scale > 0.0f)
                SmartTextEx(title, 0.0f, HUB_EPISODESUBTITLEY, 1.0f, scale, scale, scale, 0, HUB_EPISODER, HUB_EPISODEG,
                            HUB_EPISODEB, 1.7f, 1, NULL, 0, alpha);
        }
        MENU *menu = &GameMenu[GameMenuLevel];
        if (!Game_AreaSave || Game_AreaSave[index].minikit_complete) {
            NUVEC minimum, maximum;
            NuSpecialGetBounds(&WORLD->lev_objs[167].special, &minimum, &maximum);
            const f32 icon_size = ICONSIZE;
            const f32 text_scale = (icon_size / 0.35f) * 0.95f;
            const f32 spacing = 1.2f * (icon_size * (maximum.x - minimum.x)) / PANEL3DMULX;
            f32 x = -spacing * 2.5f;
            const f32 y = HUB_EPISODETITLEY + (HUB_EPISODETITLEY - HUB_EPISODESUBTITLEY) * 1.5f;
            f32 grow = 1.0f;
            if (!TestForController()) {
                const f32 elapsed = GlobalTimer.time_elapsed - (LastTouchTime + 2.0f);
                if (elapsed > 4.0f) {
                    f32 wave = NU_SIN_LUT(static_cast<i32>((NuFmod(elapsed, 4.0f) * 0.25f) * 65536.0f)) - 0.8f;
                    if (wave < 0.0f)
                        wave = 0.0f;
                    grow = wave + 1.0f;
                }
            }
            for (i32 i = 0; i < 6; ++i) {
                i32 red, green, blue;
                f32 size = 1.0f;
                if (i == i_selectminikitepisode) {
                    size = 1.2f;
                    if (TestForController()) {
                        if (menu_pulsate > 0.0f) {
                            red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulsate +
                                                   static_cast<u32>(MENUFLASH1R) * (1.0f - menu_pulsate));
                            green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulsate +
                                                     static_cast<u32>(MENUFLASH1G) * (1.0f - menu_pulsate));
                            blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulsate +
                                                    static_cast<u32>(MENUFLASH1B) * (1.0f - menu_pulsate));
                        } else {
                            red = menu_flash ? MENUFLASH0R : MENUFLASH1R;
                            green = menu_flash ? MENUFLASH0G : MENUFLASH1G;
                            blue = menu_flash ? MENUFLASH0B : MENUFLASH1B;
                        }
                    } else {
                        if (menu_pulse > 0.0f) {
                            red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulse +
                                                   static_cast<u32>(MENUNORMALR) * (1.0f - menu_pulse));
                            green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulse +
                                                     static_cast<u32>(MENUNORMALG) * (1.0f - menu_pulse));
                            blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulse +
                                                    static_cast<u32>(MENUNORMALB) * (1.0f - menu_pulse));
                        } else {
                            red = MENUENTRYR;
                            green = MENUENTRYG;
                            blue = MENUENTRYB;
                        }
                    }
                } else {
                    if (menu_pulse > 0.0f) {
                        red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulse +
                                               static_cast<u32>(MENUNORMALR) * (1.0f - menu_pulse));
                        green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulse +
                                                 static_cast<u32>(MENUNORMALG) * (1.0f - menu_pulse));
                        blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulse +
                                                static_cast<u32>(MENUNORMALB) * (1.0f - menu_pulse));
                    } else {
                        red = MENUENTRYR;
                        green = MENUENTRYG;
                        blue = MENUENTRYB;
                    }
                }
                size *= grow;
                f32 opacity = phase;
                if (!Episode_CountOpenAreas(i, -1, Game_AreaSave))
                    opacity *= 0.25f;
                Text3DEx(EpisodeNumerals[i], x, y, 1.0f, text_scale * size, text_scale * size, text_scale * size, 0,
                         red, green, blue, static_cast<u8>(static_cast<i32>(128.0f * opacity)));
                const i32 bonus = Episode_FindAreaFromFlags(&EDataList[i], 5, 5);
                i32 object = 167;
                if (bonus != -1 && Game_AreaSave &&
                    (Game_AreaSave[bonus].area_complete || static_cast<f32>(ADataList[bonus].challenge_trial_time) >
                                                               Game_AreaSave[bonus].challenge_trial_time))
                    object = 168;
                const f32 scale = icon_size * size;
                DrawPanel3DObject(x, y, 1.0f, scale, scale, scale, 0, 0, 0, &WORLD->lev_objs[object].special, 0,
                                  opacity);
                menu->item_x[i] = x;
                menu->item_y[i] = y;
                menu->item_width[i] = scale * 0.5f;
                menu->item_height[i] = 0.0f;
                x += spacing;
            }
        } else {
            for (i32 i = 0; i < 6; ++i)
                menu->item_width[i] = 0.0f;
        }
    }
    if (hub_buildit != -1 && hub_buildit_time > 0.0f) {
        i32 total;
        if (hub_buildit == 999)
            total = LevBuildIt[0]->anim_object_count * 2;
        else {
            GIZMO *gizmo = HubAreaInfo[hub_buildit].bonus_gizmo;
            if (!gizmo)
                return;
            total = static_cast<GIZBUILDIT_s *>(gizmo->object)->anim_object_count;
        }
        const i32 count = total > Game.gold_bricks ? Game.gold_bricks : total;
        Hub_DrawImportantBrick(211, 0.0f, HUB_EPISODESUBTITLEY, hub_buildit_time, count, total);
    }
}

i32 Hub_PanelBusy() {
    if (hub_episode != -1) {
        return 1;
    }
    if (hub_area != -1) {
        return 1;
    }
    if (hub_minikitarea != -1) {
        return 1;
    }
    if (hub_buildit != -1) {
        return 1;
    }

    if (GetMenuID() == 20) {
        return 1;
    }
    if (GetMenuID() == 21) {
        return 1;
    }
    if (GetMenuID() == 22) {
        return 1;
    }
    if (GetMenuID() == 23) {
        return 1;
    }
    return 0;
}

void Hub_UpdateKit() {
    MENU *menu = &GameMenu[GameMenuLevel];
    i32 right = 0, left = 0, cancel = 0;
    if (hub_minikitviewer_movewait <= 0.0f) {
        for (i32 i = 0; i < 2; ++i) {
            if (MenuPacket.active_player[i]) {
                if ((GamePad[i].buttons_held & GAMEPAD_TOGGLELEFT) == 0) {
                    if ((GamePad[i].buttons_held & GAMEPAD_TOGGLERIGHT) != 0)
                        ++right;
                } else if ((GamePad[i].buttons_held & GAMEPAD_TOGGLERIGHT) == 0)
                    ++left;
                if ((GamePad[i].buttons_pressed & GAMEPAD_MENUCANCEL) != 0)
                    cancel = 1;
            }
        }
    }
    if (menu->input_activity) {
        if (menu->left_pressed)
            ++left;
        if (menu->right_pressed)
            ++right;
        if (menu->cancel_pressed)
            cancel = 1;
    }
    i32 previous = hub_minikitviewer_area;
    if (left && !right) {
        hub_minikitviewer_move = -1.0f;
        i32 index = previous;
        i32 attempts = 0;
        do {
            --index;
            // Original backward traversal wraps to AREACOUNT itself.
            if (index < 0)
                index = AREACOUNT;
            if (++attempts >= AREACOUNT)
                break;
        } while ((ADataList[index].flags & 0x10) == 0);
        hub_minikitviewer_area = index;
    } else if (right && !left) {
        hub_minikitviewer_move = 1.0f;
        i32 index = previous;
        i32 attempts = 0;
        do {
            ++index;
            if (index >= AREACOUNT)
                index = 0;
            if (++attempts >= AREACOUNT)
                break;
        } while ((ADataList[index].flags & 0x10) == 0);
        hub_minikitviewer_area = index;
    }
    if (hub_minikitviewer_area != previous) {
        hub_minikitviewer_movewait = 1.0f;
        hub_minikitviewer_alpha = 0.0f;
        PlaySfx(const_cast<char *>("JForcePush"), &hub_minikitviewer_pos);
    }
    if (cancel) {
        GameAudio_PlaySfx(49, NULL, 0, 0);
        MenuReset();
        hub_minikitarea = -1;
        hub_minikitarea_time = 0.0f;
        hub_minikitarea_opentime = 0.0f;
        LevLock[4] = 1;
        LevTime[4] = 0.6f;
    }
}

void Hub_CallBarman(GameObject_s *) {
}

void Hub_ClearStats() {
    statstime = 0.0f;
    cointotaltime = 0.0f;
    goldbricktime = 0.0f;
}

i32 hub_forceshopsave = 0;

void Hub_ResetPanel() {
    hub_episode = -1;
    hub_episode_time = 0.0f;
    hub_area = -1;
    hub_area_time = 0.0f;
    hub_buildit = -1;
    hub_buildit_time = 0.0f;
    hub_minikitarea = -1;
    hub_minikitarea_time = 0.0f;
}

bool HubShopUnlocked() {
    return true;
}

i32 Hub_BonusBuildIt(GIZBUILDIT_s *buildit) {
    for (HUBAREAINFO_s *info = HubAreaInfo; info->area_name != NULL; ++info) {
        if (info->bonus_gizmo != NULL && info->bonus_gizmo->object == buildit) {
            return 1;
        }
    }
    return 0;
}

#include "nu2api/nu3d/nurndr.h"
extern f32 hub_minikitviewer_movewait, hub_minikitviewer_move;
f32 HUB_MINIKITVIEWER_REFLECTY = 0.09f;
i32 RotDiff(u16, u16);
i32 MatrixReflection(NUMTX *, i32, f32, f32, NUMTX *);

static inline void MiniKitSetRotationX(NUMTX *m, NUANG a) {
    const f32 cosine = NU_COS_LUT(a);
    const f32 sine = NU_SIN_LUT(a);
    m->m00 = 1.0f;
    m->m11 = cosine;
    m->m12 = sine;
    m->m21 = -sine;
    m->m22 = cosine;
    m->m01 = 0.0f;
    m->m02 = 0.0f;
    m->m03 = 0.0f;
    m->m10 = 0.0f;
    m->m13 = 0.0f;
    m->m20 = 0.0f;
    m->m23 = 0.0f;
    m->m30 = 0.0f;
    m->m31 = 0.0f;
    m->m32 = 0.0f;
    m->m33 = 1.0f;
}

static inline void MiniKitRotateX(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m01 = m->m01;
    f32 m11 = m->m11;
    f32 m21 = m->m21;
    f32 m31 = m->m31;

    m->m01 = m01 * cosx - m->m02 * sinx;
    m->m02 = m01 * sinx + m->m02 * cosx;
    m->m11 = m11 * cosx - m->m12 * sinx;
    m->m12 = m11 * sinx + m->m12 * cosx;
    m->m21 = m21 * cosx - m->m22 * sinx;
    m->m22 = m21 * sinx + m->m22 * cosx;
    m->m31 = m31 * cosx - m->m32 * sinx;
    m->m32 = m31 * sinx + m->m32 * cosx;
}

static inline void MiniKitRotateY(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m10 = m->m10;
    f32 m20 = m->m20;
    f32 m30 = m->m30;

    m->m00 = m00 * cosx + m->m02 * sinx;
    m->m02 = m->m02 * cosx - m00 * sinx;
    m->m10 = m10 * cosx + m->m12 * sinx;
    m->m12 = m->m12 * cosx - m10 * sinx;
    m->m20 = m20 * cosx + m->m22 * sinx;
    m->m22 = m->m22 * cosx - m20 * sinx;
    m->m30 = m30 * cosx + m->m32 * sinx;
    m->m32 = m->m32 * cosx - m30 * sinx;
}

static inline void MiniKitRotateZ(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m10 = m->m10;
    f32 m20 = m->m20;
    f32 m30 = m->m30;

    m->m00 = m00 * cosx - m->m01 * sinx;
    m->m01 = m00 * sinx + m->m01 * cosx;
    m->m10 = m10 * cosx - m->m11 * sinx;
    m->m11 = m10 * sinx + m->m11 * cosx;
    m->m20 = m20 * cosx - m->m21 * sinx;
    m->m21 = m20 * sinx + m->m21 * cosx;
    m->m30 = m30 * cosx - m->m31 * sinx;
    m->m31 = m30 * sinx + m->m31 * cosx;
}

void Hub_DrawMiniKits(WORLDINFO_s *world) {
    if (world->minikit_pieces_buf == NULL || world->hub_minikits == NULL)
        return;
    i32 areas[72];
    i32 count = 0;
    i32 selected = -1;
    for (i32 area = 0; area < AREACOUNT; ++area) {
        if (ADataList[area].flags & 0x10) {
            areas[count] = area;
            if (area == hub_minikitviewer_area)
                selected = count;
            ++count;
        }
    }
    for (i32 index = 0; index < count; ++index) {
        const i32 area = areas[index];
        HUBMINIKITPIECES_s *pieces = world->minikit_pieces_buf[area];
        if (pieces == NULL || ADataList[area].minikit_id == -1)
            continue;
        HUBAREAINFO_s *info;
        for (info = HubAreaInfo; info->area_name != NULL; ++info) {
            if (info->area != NULL && area == info->area->index)
                break;
        }
        if (info->area_name == NULL)
            info = NULL;
        f32 x;
        if (area == areas[(selected + count - 2) % count])
            x = -4.0f;
        else if (area == areas[(selected + count - 1) % count])
            x = -2.0f;
        else if (area == areas[selected])
            x = 0.0f;
        else if (area == areas[(selected + 1) % count])
            x = 2.0f;
        else if (area == areas[(selected + 2) % count])
            x = 4.0f;
        else
            continue;
        x += hub_minikitviewer_pos.x;
        if (hub_minikitviewer_movewait > 0.0f) {
            x += (1.0f - (1.0f + NU_SIN_LUT((i32)(hub_minikitviewer_movewait * 32768.0f + 16384.0f))) * 0.5f) * 2.0f *
                 hub_minikitviewer_move;
        }
        HUBMINIKIT_s *kit = &world->hub_minikits[area];
        const f32 scale = kit->scale;
        const f32 lift = (1.0f - (1.0f + NU_SIN_LUT((i32)(scale * 32768.0f + 16384.0f))) * 0.5f) * 0.333f;
        const u16 phase_x = (i32)(NU_SIN_LUT(kit->phase_x) * 910.0f);
        const u16 phase_z = (i32)(NU_SIN_LUT(kit->phase_z) * 910.0f);
        const u16 phase_y = (i32)(NU_SIN_LUT(kit->phase_y) * 910.0f);
        const u16 rotation = (i32)(RotDiff(0, phase_y) * scale) + kit->rotation;
        const f32 bob = NU_SIN_LUT(kit->phase_rotation) * 0.025f * scale;
        const f32 tilt_scale = (info ? info->panel_scale : 1.0f) * scale;
        NUMTX matrix, piece_matrix, reflected;
        MiniKitSetRotationX(&matrix, (i32)(RotDiff(0, kit->tilt) * tilt_scale));
        MiniKitRotateX(&matrix, (i32)(RotDiff(0, phase_x) * scale));
        MiniKitRotateZ(&matrix, (i32)(RotDiff(0, phase_z) * scale));
        MiniKitRotateY(&matrix, rotation);
        matrix.m30 = x + kit->offset_x;
        matrix.m31 = hub_minikitviewer_pos.y;
        matrix.m32 = hub_minikitviewer_pos.z;
        matrix.m31 += lift + bob + (info ? info->panel_offset : 0.0f);
        matrix.m32 += kit->offset_z;
        NuMtxPreRotateZ(&matrix, (i32)(RotDiff(0, kit->rotation_velocity) * scale));
        for (i32 piece = 0; piece < pieces->piece_count; ++piece) {
            if (pieces->pieces[piece].enabled && NuSpecialExistsFn(&pieces->pieces[piece].special) &&
                piece < kit->displayed_piece_count) {
                piece_matrix = pieces->pieces[piece].matrix;
                NuMtxMulVU0(&piece_matrix, &piece_matrix, &matrix);
                const i32 drawn = NuSpecialDrawAt(&pieces->pieces[piece].special, &piece_matrix);
                if (HUB_MINIKITVIEWER_REFLECTY != 2000000.0f &&
                    MatrixReflection(&piece_matrix, 2, HUB_MINIKITVIEWER_REFLECTY, 2000000.0f, &reflected)) {
                    NuRndrStartReflectionRender(drawn);
                    NuSpecialDrawAt(&pieces->pieces[piece].special, &reflected);
                    NuRndrEndReflectionRender();
                }
            }
        }
        if (NuSpecialExistsFn(&pieces->base.special)) {
            NuMtxSetRotationY(&piece_matrix, rotation);
            piece_matrix.m30 = x;
            piece_matrix.m31 = 0.143f;
            piece_matrix.m32 = hub_minikitviewer_pos.z;
            NuSpecialDrawAtAlpha(&pieces->base.special, &piece_matrix, 1.0f - 0.5f * kit->scale);
        }
    }
    SetLevelLights(world->rtl_set, 1.0f);
}

f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);

void Hub_InitMiniKits(WORLDINFO_s *world) {
    HUBMINIKIT_s *kit = world->hub_minikits;
    if (kit != NULL) {
        for (i32 area_index = 0; area_index < AREACOUNT; ++area_index, ++kit) {
            kit->position.y = 2000000.0f;
            kit->scale = 0.0f;
            kit->field_0x10 = 0.0f;
            kit->rotation = qrand();
            for (HUBAREAINFO_s *info = HubAreaInfo; info->area_name != NULL; ++info) {
                if (info->area != NULL && area_index == info->area->index)
                    break;
            }
            const f32 height = GameShadow(NULL, &kit->position, 5.0f, -1);
            if (height != 2000000.0f)
                kit->position.y = height;
            kit->phase_x = qrand();
            kit->phase_y = qrand();
            kit->phase_z = qrand();
            kit->phase_rotation = qrand();
            kit->rate_x = static_cast<i32>(static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 5461.0f + 5461.0f);
            kit->rate_y = static_cast<i32>(static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 5461.0f + 5461.0f);
            kit->rate_z = static_cast<i32>(static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 5461.0f + 5461.0f);
            kit->rate_rotation = static_cast<i32>(static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 10922.0f + 10922.0f);
            kit->velocity_x = 0.0f;
            kit->rotation_velocity = 0;
            kit->offset_x = 0.0f;
            kit->velocity_z = 0.0f;
            kit->target_rotation = kit->rotation;
            kit->offset_z = 0.0f;
            kit->tilt = 0;
            kit->target_piece_count = 0;
            kit->displayed_piece_count = 0;
        }
    }
}

void Hub_DrawAreaStats(float, i32, i32) {
}

void Hub_DrawStarField() {
}

void Hub_MakeModelList() {
    Hub_ModelList[0] = {static_cast<i16>(PlayerID[0]), 1};
    Hub_ModelList[1] = {static_cast<i16>(PlayerID[1]), 1};

    i32 count = 2;
    if (HUB_ADATA != NULL && HUB_ADATA->hub_player_ids != NULL) {
        i16 *model_id = HUB_ADATA->hub_player_ids;
        while (count < 5 && *model_id != -1) {
            Hub_ModelList[count++] = {*model_id++, 1};
        }
    }
    Hub_ModelList[count].model_id = -1;

    PlayerList[0] = Hub_ModelList[0].model_id;
    PlayerList[1] = Hub_ModelList[1].model_id;
    PlayerList[2] = -1;
}

void Hub_UpdateMiniKits(WORLDINFO_s *world) {
    i32 count[72];
    i32 viewer = 0;
    f32 speed = 0.0f;
    u16 angle = 0;
    if (GetMenuID() == 14) {
        viewer = 1;
        MENU *menu = &GameMenu[GameMenuLevel];
        if (hub_minikitviewer_movewait <= 0.0f) {
            if (Hub_PadSpeed[0] > 0.0f && Hub_PadSpeed[1] > 0.0f) {
                speed = (Hub_PadSpeed[0] + Hub_PadSpeed[1]) * 0.5f;
                angle = Hub_PadAngle[0] + RotDiff(Hub_PadAngle[0], Hub_PadAngle[1]) * 0.5f + GameCam->yaw;
            } else if (Hub_PadSpeed[0] > 0.0f) {
                speed = Hub_PadSpeed[0];
                angle = Hub_PadAngle[0] + GameCam->yaw;
            } else if (Hub_PadSpeed[1] > 0.0f) {
                speed = Hub_PadSpeed[1];
                angle = Hub_PadAngle[1] + GameCam->yaw;
            }
            i32 confirm = 0;
            if (menu->input_activity && menu->confirm_pressed) {
                if (menu->selected_item == i_selectminikitepisode)
                    confirm = 1;
                else
                    i_selectminikitepisode = menu->selected_item;
            }
            if (confirm || (GamePad[0].buttons_pressed & GAMEPAD_MENUSELECT)) {
                if (Game_AreaSave == NULL || Game_AreaSave[hub_minikitviewer_area].minikit_complete) {
                    if (Episode_CountOpenAreas(i_selectminikitepisode, -1, Game_AreaSave)) {
                        i32 area = Episode_FindAreaFromFlags(&EDataList[i_selectminikitepisode], 5, 5);
                        if (area != -1) {
                            hub_new_level = ADataList[area].levels[0];
                            Hub_MakeFreePlayList(ADataList[hub_minikitviewer_area].minikit_id, -1);
                            makeplayerlist_freeplay = 1;
                            NewLData = &LDataList[hub_new_level];
                            NextArea_FreePlay = 1;
                            FreePlay = 1;
                            GameAudio_PlaySfx(48, NULL, 0, 0);
                        }
                    } else {
                        GameAudio_PlaySfx(50, NULL, 0, 0);
                        GameCam_HitRoll();
                    }
                }
            } else if ((GamePad[0].buttons_pressed & GAMEPAD_DLEFT) && i_selectminikitepisode > 0) {
                --i_selectminikitepisode;
            } else if ((GamePad[0].buttons_pressed & GAMEPAD_DRIGHT) && i_selectminikitepisode < 5) {
                ++i_selectminikitepisode;
            }
        }
    }
    if (world->minikit_pieces_buf == NULL || world->hub_minikits == NULL)
        return;
    if (viewer) {
        if (hub_minikitviewer_alpha < 1.0f) {
            hub_minikitviewer_alpha += FRAMETIME * 2.0f;
            if (hub_minikitviewer_alpha > 1.0f)
                hub_minikitviewer_alpha = 1.0f;
        }
    } else
        hub_minikitviewer_alpha = 0.0f;
    if (hub_minikitviewer_movewait > 0.0f) {
        hub_minikitviewer_movewait -= FRAMETIME;
        if (hub_minikitviewer_movewait <= 0.0f)
            hub_minikitviewer_move = 0.0f;
    }
    PlaySfx("ui_hover_lp", &hub_minikitviewer_pos);
    HUBMINIKIT_s *kit = world->hub_minikits;
    for (i32 i = 0; i < AREACOUNT; ++i, ++kit) {
        HUBMINIKITPIECES_s *pieces = world->minikit_pieces_buf[i];
        if (pieces == NULL)
            continue;
        count[i] = Game.area_save[i].minikit_count;
        if (count[i] > pieces->piece_count)
            count[i] = pieces->piece_count;
        kit->scale = 1.0f;
        f32 vx = (0.0f - kit->offset_x) * 5.0f;
        f32 vz = (0.0f - kit->offset_z) * 5.0f;
        kit->velocity_x = SeekValF(kit->velocity_x, vx, 10.0f);
        kit->velocity_z = SeekValF(kit->velocity_z, vz, 10.0f);
        kit->offset_x += kit->velocity_x * FRAMETIME;
        kit->offset_z += kit->velocity_z * FRAMETIME;
        u16 old_rotation = kit->rotation;
        if (i == hub_minikitviewer_area) {
            if (speed > 0.0f)
                kit->target_rotation =
                    TurnRot(kit->target_rotation, angle + 0x8000, (u16)(32768.0f * kit->scale), NULL);
            kit->target_piece_count = count[i];
            if (viewer && count[i] >= pieces->piece_count &&
                ((MenuPacket.active_player[0] && (GamePad[0].buttons_held & GAMEPAD_ACTION)) ||
                 (MenuPacket.active_player[1] && (GamePad[1].buttons_held & GAMEPAD_ACTION))))
                kit->target_piece_count = 1;
        } else if (count[i] != 0)
            kit->target_piece_count = count[i];
        else {
            kit->target_piece_count = 0;
            kit->displayed_piece_count = 0;
        }
        kit->rotation = SeekRot(kit->rotation, kit->target_rotation, 3.0f);
        i32 rate = (i32)((f32)RotDiff(old_rotation, kit->rotation) / FRAMETIME);
        rate = (MAX(-65536, (MIN(65536, rate))));
        rate /= 8;
        rate = (MAX(-4551, (MIN(4551, rate))));
        kit->rotation_velocity = SeekRot(kit->rotation_velocity, rate, 8.0f);
        i32 tilt = 0;
        if (i == hub_minikitviewer_area && speed > 0.0f) {
            i32 diff = RotDiff(old_rotation, kit->rotation);
            f32 angular_speed = (f32)(diff < 0 ? -diff : diff) / FRAMETIME;
            if (angular_speed < 32768.0f)
                tilt = -(i32)((1.0f - angular_speed * (1.0f / 32768.0f)) * 4551.0f);
        }
        kit->tilt = SeekRot(kit->tilt, tilt, 3.0f);
        for (i32 j = 0; j < pieces->piece_count; ++j) {
            pieces->pieces[j].enabled = 0;
            if (NuSpecialExistsFn(&pieces->pieces[j])) {
                NuSpecialSetVisibility(&pieces->pieces[j], 0);
                if (j < count[i])
                    pieces->pieces[j].enabled = 1;
            }
        }
        kit->phase_x = (i32)((f32)kit->phase_x + kit->rate_x * FRAMETIME);
        if (kit->phase_x > 65536)
            kit->phase_x -= 65536;
        kit->phase_y = (i32)((f32)kit->phase_y + kit->rate_y * FRAMETIME);
        if (kit->phase_y > 65536)
            kit->phase_y -= 65536;
        kit->phase_z = (i32)((f32)kit->phase_z + kit->rate_z * FRAMETIME);
        if (kit->phase_z > 65536)
            kit->phase_z -= 65536;
        kit->phase_rotation = (i32)((f32)kit->phase_rotation + kit->rate_rotation * FRAMETIME);
        if (kit->phase_rotation > 65536)
            kit->phase_rotation -= 65536;
        i32 model = ADataList[i].minikit_id;
        if (model != -1 && world->hub_minikits != NULL) {
            HUBMINIKIT_s *current = &world->hub_minikits[i];
            HUBAREAINFO_s *info = HubAreaInfo;
            for (; info->area_name != NULL; ++info) {
                if (info->area != NULL && info->area->index == i)
                    break;
            }
            if (info->area_name == NULL)
                info = NULL;
            CHARACTERDATA *data = &CDataList[model];
            current->radius = data->collision_radius;
            current->height_ratio = (data->bounds_max_y - data->bounds_min_y) / (data->collision_radius * 2.0f);
            current->collision_center.x = current->position.x + current->offset_x;
            current->collision_center.y = current->position.y + (data->bounds_max_y + data->bounds_min_y) * 0.5f;
            current->collision_center.z = current->position.z + current->offset_z;
            current->collision_center.y +=
                (1.0f - (NU_SIN_LUT((i32)(32768.0f * current->scale + 16384.0f)) + 1.0f) * 0.5f) * 0.333f;
            current->collision_center.y += 0.025f * NU_SIN_LUT(current->phase_rotation) * current->scale;
            current->collision_center.y += info ? info->panel_offset : 0.0f;
            current->bounds_min.x = current->collision_center.x - current->radius;
            current->bounds_min.y = current->collision_center.y - current->radius * current->height_ratio;
            current->bounds_min.z = current->collision_center.z - current->radius;
            current->bounds_max.x = current->collision_center.x + current->radius;
            current->bounds_max.y = current->collision_center.y + current->radius * current->height_ratio;
            current->bounds_max.z = current->collision_center.z + current->radius;
        }
    }
    kit = world->hub_minikits;
    for (i32 i = 0; i < AREACOUNT; ++i, ++kit) {
        if (world->minikit_pieces_buf[i] == NULL)
            continue;
        if (kit->displayed_piece_count != kit->target_piece_count &&
            (i32)(GameTimer.time_elapsed / 0.15f) != (i32)(GameTimer.last_time_elapsed / 0.15f)) {
            if (kit->displayed_piece_count < kit->target_piece_count)
                ++kit->displayed_piece_count;
            else if (kit->displayed_piece_count > kit->target_piece_count)
                --kit->displayed_piece_count;
            if (i == hub_minikitarea)
                PlaySfx("LegoForm", &kit->position);
            NewRumbleAllPlayers(0.0f, 0.0f, 1, 0);
        }
        if (count[i] > 0 && kit->displayed_piece_count != 0) {
            PushAway(&kit->collision_center, kit->radius, &kit->bounds_min, &kit->bounds_max, NULL, NULL, 2.5f, 5);
            AIAntinodeCreateSingleFrame(&kit->collision_center, kit->radius);
        }
    }
}

void Hub_LockUnlockDoors(WORLDINFO_s *) {
}

void Hub_ActivateDoorMenu(LEVELDATA_s **level) {
    if ((*level)->area_index == -1) {
        return;
    }

    const i32 area = last_hub_area;
    if (area != (*level)->area_index) {
        *level = NULL;
        return;
    }

    MakeMenuPacket();
    hub_new_level = ADataList[area].levels[0];
    *level = NULL;
    hub_episode_time = 0.0f;
    hub_area_time = 0.0f;
    MainRenderTargetTime = 0.0f;
    NeedScreenGrab(1);
    BackDrop_ResetColours();

    i32 menu_id;
    if (area != -1 && (ADataList[area].flags & AREAFLAG_BONUS_AREA) != 0) {
        menu_id = HUB_DOOR_MENU_BONUS;
        if (ADataList[area].episode_index != -1) {
            menu_id = HUB_DOOR_MENU_EPISODE_BONUS;
        }
    } else if (VEHICLES_ADATA != NULL && area == VEHICLES_ADATA->index) {
        i16 vehicle_ids[HUB_VEHICLE_ID_CAPACITY];
        i32 vehicle_count;
        Collection_GetIDList(&VehicleCollection, HUB_VEHICLE_COLLECTION_ALLOWED, HUB_VEHICLE_COLLECTION_REQUIRED,
                             vehicle_ids, &vehicle_count, NULL, 0);
        menu_id = HUB_DOOR_MENU_VEHICLE;
        hub_freeplaysource = -1;
        Hub_InitFreePlaySelect(area, vehicle_count, -1);
    } else {
        menu_id = HUB_DOOR_MENU_STANDARD;
        if (LOSTTEMPLE_ADATA != NULL && area == LOSTTEMPLE_ADATA->index) {
            menu_id = HUB_DOOR_MENU_EPISODE_BONUS;
        }
    }

    GameAudio_PlaySfx(HUB_AUDIO_EVENT_OPEN_DOOR_MENU, NULL, 0, 0);
    Hint_CancelCurrent();
    NewMenu(menu_id, -1, -1);
}

bool HubCustomiserUnlocked() {
    return true;
}

static const char *Hub_FreePlayModelName(i32 model) {
    if (model < 0 || model >= CHARCOUNT) {
        return "";
    }
    if (CDataList[model].name_id >= 0) {
        return TTab[CDataList[model].name_id];
    }
    if (GlobalCharacterNameFn != NULL) {
        const char *name = GlobalCharacterNameFn(model);
        if (name != NULL) {
            return name;
        }
    }
    return "";
}

void Hub_DrawFreePlaySelect() {
    MENU *menu = &GameMenu[GameMenuLevel];
    char text[256];

    snprintf(text, sizeof(text), "%s: %s", TTab[tPLAYER1], Hub_FreePlayModelName(MenuPacket.player_model[0]));
    GameDrawMenuEntry(menu, text);
    snprintf(text, sizeof(text), "%s: %s", TTab[tPLAYER2], Hub_FreePlayModelName(MenuPacket.player_model[1]));
    GameDrawMenuEntry(menu, text);
    GameDrawMenuEntry(menu, TTab[tPLAY]);

    if (MenuPacket.player_model[0] >= 0 && MenuPacket.player_model[0] < CHARCOUNT) {
        DrawCharIcon(MenuPacket.player_model[0], -ICONX, STATSPOSY, 0.0f, ICONSIZE, 0xa6, 1.0f, 1.0f, 1, NULL);
    }
    if (MenuPacket.player_model[1] >= 0 && MenuPacket.player_model[1] < CHARCOUNT) {
        DrawCharIcon(MenuPacket.player_model[1], ICONX, STATSPOSY, 0.0f, ICONSIZE, 0xa5, 1.0f, 1.0f, 1, NULL);
    }
}

extern f32 PANEL_REDBRICKSCALE;
void Hub_DrawImportantBrick(i32 object, f32 x, f32 y, f32 alpha, i32 count, i32 total) {
    char text[32];
    const i32 rotation = static_cast<i32>((NuFmod(GlobalTimer.time_elapsed, 4.0f) * 0.25f) * 65536.0f) & 0xffff;
    const f32 scale = NU_SIN_LUT(static_cast<i32>(alpha * 16384.0f)) * PANEL_REDBRICKSCALE;
    const u16 tilt = static_cast<i32>(1820.0f * NuTrigTable[rotation & 0x7fff]) - 910;
    DrawPanel3DObjectNoAlpha(x, y + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 1.0f, scale, scale, scale, tilt, rotation, 0,
                             &WORLD->lev_objs[object].special, 2);
    if (total >= 0 && count >= 0) {
        if (total == 1)
            NuStrCpy(text, count == 1 ? "$" : "X");
        else
            sprintf(text, "%i/%i", count, total);
        Text3DEx(text, x, y, 1.0f, PANEL_MINIKITCOUNTSCALE * stats_xscale, PANEL_MINIKITCOUNTSCALE,
                 PANEL_MINIKITCOUNTSCALE, 0, 255, 0, 127, static_cast<u8>(static_cast<i32>(128.0f * alpha)));
    }
}

void Hub_InitFreePlaySelect(i32 area, i32 first_model, i32 second_model) {
    COLLECTION_s *collection = GetFreePlayCollection(area);
    if (collection == &VehicleCollection) {
        if (first_model == -1)
            first_model = VehicleCollection.list[0].id;
        if (second_model == -1) {
            second_model = VehicleCollection.list[1].id;
            goto make_default_pair;
        }
        goto make_selected_pair;
    }
    if (collection == &MiniKitCollection) {
        if (first_model == -1)
            first_model = MiniKitCollection.list[0].id;
        if (second_model == -1) {
            second_model = MiniKitCollection.list[1].id;
            goto make_default_pair;
        }
        goto make_selected_pair;
    }
    Hub_MakeFreePlayList(-1, -1);
    goto use_player_models;

make_selected_pair:
    Hub_MakeFreePlayList(first_model, second_model);
    if (first_model == -1)
        goto use_player_models;
    goto store_model_pair;

make_default_pair:
    Hub_MakeFreePlayList(first_model, second_model);
    if (first_model == -1 || second_model == -1)
        goto use_player_models;

store_model_pair:
    MenuPacket.player_model[0] = static_cast<i16>(first_model);
    MenuPacket.player_model[1] = static_cast<i16>(second_model);
    goto initialize_selection;

use_player_models:
    MenuPacket.player_model[0] = Player[0] != NULL ? Player[0]->id : -1;
    MenuPacket.player_model[1] = Player[1] != NULL ? Player[1]->id : -1;

initialize_selection:
    freeplaytime = 0.0f;
    freeplaymode = 0;
    freeplayduration = 1.0f;
    freeplay_selected[0] = 0;
    freeplay_selected[1] = 0;
    uprepeattime[0] = 0.0f;
    rightrepeattime[0] = 0.0f;
    uprepeattime[1] = 0.0f;
    rightrepeattime[1] = 0.0f;
    downrepeattime[0] = 0.0f;
    leftrepeattime[0] = 0.0f;
    downrepeattime[1] = 0.0f;
    leftrepeattime[1] = 0.0f;
    ResetIconWibble();
    hub_freeplay_area = area;
}

bool HubMinikitViewerUnlocked() {
    return true;
}

void Hub_UpdateFreePlaySelect() {
    MENU *menu = &GameMenu[GameMenuLevel];

    if (menu->cancel_pressed != 0) {
        MenuSFX = GameAudio_GetSfxId(0x31);
        WipeBackToHub();
        return;
    }

    if (menu->selected_item < 2 && fpcount > 0 && (menu->left_pressed != 0 || menu->right_pressed != 0)) {
        const i32 player = menu->selected_item;
        const i32 current = MenuPacket.player_model[player];
        i32 index = -1;
        for (i32 i = 0; i < fpcount; ++i) {
            if (fplist[i].model_id == current) {
                index = i;
                break;
            }
        }

        if (menu->right_pressed != 0) {
            index = (index + 1) % fpcount;
        } else {
            index = index <= 0 ? fpcount - 1 : index - 1;
        }
        MenuPacket.player_model[player] = fplist[index].model_id;
        freeplay_selected[player] = index;
        MenuSFX = GameAudio_GetSfxId(0x2f);
        ResetIconWibble();
        return;
    }

    if (menu->confirm_pressed == 0 || menu->selected_item != 2) {
        return;
    }

    i32 first_model = MenuPacket.player_model[0];
    i32 second_model = MenuPacket.player_model[1];
    if ((first_model < 0 || first_model >= CHARCOUNT) && fpcount > 0) {
        first_model = fplist[0].model_id;
    }
    if ((second_model < 0 || second_model >= CHARCOUNT) && fpcount > 0) {
        second_model = fplist[fpcount > 1 ? 1 : 0].model_id;
    }
    if (first_model < 0 || first_model >= CHARCOUNT) {
        MenuSFX = GameAudio_GetSfxId(0x32);
        return;
    }
    if (second_model < 0 || second_model >= CHARCOUNT) {
        second_model = first_model;
    }

    MenuPacket.player_model[0] = static_cast<i16>(first_model);
    MenuPacket.player_model[1] = static_cast<i16>(second_model);
    MakeFreePlayModelList(first_model, second_model, hub_freeplay_area, -1, 1);
    makeplayerlist_freeplay = 1;
    NextArea_FreePlay = 1;
    FreePlay = 1;
    loadareacharacters_no_backdrop_reset = 1;

    LEVELDATA_s *level = Area_FindNextPlayLevel(hub_new_level);
    if (level == NULL) {
        MenuSFX = GameAudio_GetSfxId(0x32);
        return;
    }
    MenuSFX = GameAudio_GetSfxId(0x30);
    NewLevelFromMenu(level, -1, -1, 1);
}

static void Hub_FindSpecial(WORLDINFO_s *world, nuhspecial_s *special, const char *name) {
    NuSpecialFind(world->current_gscn, special, const_cast<char *>(name), 1);
}

void Hub_Init(WORLDINFO_s *world) {
    static NUVEC arcadelightpos = {-22.49f, 0.79f, -49.9975f};

    Store_RestorePurchases();
    Hub_LowEnd_IconsInsteadOfModels = 0;

    if (BOTHTRUEJEDIGOLDBRICKS != 0) {
        HUB_AREAPANELX = HUB_AREAPANELX_TWOTRUEJEDIGOLDBRICKS;
    } else if (Store_IsPackUnlocked(8) == 0) {
        HUB_AREAPANELX = HUB_AREAPANELX_1TRUEJEDIGB_NOCHALLENGE;
    } else {
        HUB_AREAPANELX = HUB_AREAPANELX_ONETRUEJEDIGOLDBRICK;
    }

    for (HUBAREAINFO_s *info = HubAreaInfo; info->area_name != NULL; ++info) {
        info->door = GizmoFindByName(world->gizmo_sys, -1, const_cast<char *>(info->door_name));
        info->bonus_gizmo = GizmoFindByName(world->gizmo_sys, -1, const_cast<char *>(info->bonus_gizmo_name));
        info->bonus_gizmo_2 = GizmoFindByName(world->gizmo_sys, -1, const_cast<char *>(info->bonus_gizmo_name_2));
        info->area = Area_FindByName(const_cast<char *>(info->area_name), NULL);
        Hub_FindSpecial(world, &info->lock, info->lock_name);
    }

    for (HUBEPISODEINFO_s *info = HubEpisodeInfo; info->episode != -1; ++info) {
        info->data = info->episode >= 0 && info->episode < EPISODECOUNT ? &EDataList[info->episode] : NULL;
        info->door = GizmoFindByName(world->gizmo_sys, -1, const_cast<char *>(info->door_name));
        Hub_FindSpecial(world, &info->lock_on, info->lock_on_name);
        Hub_FindSpecial(world, &info->lock_off, info->lock_off_name);
    }

    hub_minikitviewer_gizmo =
        GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, const_cast<char *>("conveyor_test"));
    hub_minikitviewer_camspl = NuSplineFind(world->current_gscn, const_cast<char *>("mini_cam_e1_1"));
    LevGizObst[7] = GizObstacle_FindByName(world->giz_obstacle_sys, const_cast<char *>("network_door"));
    LevGizObst[6] = GizObstacle_FindByName(world->giz_obstacle_sys, const_cast<char *>("DE7"));

    static const char *const lock_names[] = {
        "lock_4_off", "lock_4_on",    "lock_5_off",  "lock_5_on",    "lock_6_off",  "lock_6_on",    "lock_b_off",
        "lock_b_on",  "lock_4_7_off", "lock_4_7_on", "lock_5_7_off", "lock_5_7_on", "lock_6_7_off", "lock_6_7_on",
    };
    for (i32 i = 0; i < 14; ++i) {
        Hub_FindSpecial(world, &LevHSpecial[i], lock_names[i]);
    }

    if (NuSpecialFind(world->current_gscn, &LevHSpecial[15], const_cast<char *>("holo"), 1) != 0) {
        NuSpecialSetVisibility(&LevHSpecial[15], 0);
    }
    if (NuSpecialFind(world->current_gscn, &LevHSpecial[16], const_cast<char *>("lock_7_on"), 1) != 0 &&
        Store_IsPackUnlocked(6) == 0) {
        NuSpecialSetVisibility(&LevHSpecial[16], 0);
    }
    NuSpecialFindMulti(world->current_gscn, &LevHSpecial[20], const_cast<char *>("ps2_only_"), 32, 0);
    NuSpecialFindMulti(world->current_gscn, &LevHSpecial[52], const_cast<char *>("psp_only_"), 32, 0);
    Hub_FindSpecial(world, &LevHSpecial[84], "fake_wall");

    Hub_ResetPanel();
    InitShop(world);
    Hub_LockUnlockDoors(world);
    Hub_InitMiniKits(world);
    LevTime[0] = 3.0f;
    LevTime[1] = 3.0f;
    Hub_HologramAlpha = 0.0f;
    Hub_HologramTargetAlpha = 0.0f;
    Customiser_Init(CharacterCustomiser);
    if (Hub_InitAIFn != NULL) {
        Hub_InitAIFn(world);
    }

    hub_startoutsidebonusdoor_area = -1;
    CutScenePlayer_Reset();
    LevBuildIt[0] = GizBuildIt_Find(world, const_cast<char *>("fountain"));
    LevGizmo[0] = GizmoFindByName(world->gizmo_sys, -1, const_cast<char *>("fountain"));
    if (LevBuildIt[0] == NULL || LevGizmo[0] == NULL) {
        LevBuildIt[0] = NULL;
        LevGizmo[0] = NULL;
    }
    LevGizmo[1] = GizmoFindByName(world->gizmo_sys, -1, const_cast<char *>("garageLever"));
    Arcade = 0;
    NuMtxSetRotationZ(&LevMtx, 0x4000);
    NuMtxRotateY(&LevMtx, 0xf600);
    NuMtxTranslate(&LevMtx, &arcadelightpos);
    Store_HubInitFloorTargets(world);
    hub_custodians_finished_loading = 0;
}

// The level callback carries the streaming-buffer arguments used by most
// level loaders, but the original hub loader only forwards the world to the
// persistent character customiser.
void Hub_Load(WORLDINFO_s *world, variptr_u *, variptr_u *) {
    Customiser_LoadAll(CharacterCustomiser, world);
}

static void Hub_SetDoorState(GIZMO *door, nuhspecial_s *lock, i32 open) {
    if (door != NULL && door->object != NULL) {
        GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(door->object);
        if (open != 0) {
            obstacle->runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_BLOCKED);
        } else {
            obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_BLOCKED;
        }
    }
    NuSpecialSetVisibility(lock, open != 0);
}

void Hub_Reset(WORLDINFO_s *world) {
    Customiser_Reset(CharacterCustomiser);
    if (Hub_ResetAIFn != NULL) {
        Hub_ResetAIFn(world);
    }

    for (i32 i = 20; i < 52; ++i) {
        NuSpecialSetVisibility(&LevHSpecial[i], static_cast<i32>((LevHSpecialExists >> i) & 1));
    }

    if (NuSpecialExistsFn(&LevHSpecial[84]) != 0) {
        NuSpecialSetVisibility(&LevHSpecial[84], 0);
    }

    for (HUBAREAINFO_s *info = HubAreaInfo; info->area_name != NULL; ++info) {
        if (info->door == NULL || info->area == NULL) {
            continue;
        }
        i32 open = Episode_CountOpenAreas(info->area->episode_index, info->area->index, Game_AreaSave);
        Hub_SetDoorState(info->door, &info->lock, open);
    }

    for (HUBEPISODEINFO_s *info = HubEpisodeInfo; info->episode != -1; ++info) {
        if (info->door == NULL) {
            continue;
        }
        i32 open = info->force_open;
        if (open == 0) {
            open = Episode_CountOpenAreas(info->episode, -1, Game_AreaSave);
        }
        Hub_SetDoorState(info->door, &info->lock_on, open);
    }
    if (Store_IsPackUnlocked(6) == 0) {
        NuSpecialSetVisibility(&HubEpisodeInfo[8].lock_on, 0);
    }

    hub_jabbaawake = 0.0f;
    buildits_reset = 0;
    Hub_HologramAlpha = 0.0f;
    Hub_HologramTargetAlpha = 0.0f;
    Hub_PreventDropOutTime = 0.0f;
}

// Private hub menu and drawing helpers.

static __used__ void Hub_DrawArcadeStats(float) {
}

static void Hub_DrawMiniKitCount(f32 x, f32 y, i32 count, i32 total, f32 alpha) {
    const i32 object = hub_drawminikitcount_charkit ? 207 : 206;
    hub_drawminikitcount_charkit = 0;
    char text[128];
    if (total == 1)
        NuStrCpy(text, count == 1 ? "$" : "X");
    else
        sprintf(text, "%i/%i", count, total);

    Text3DEx(text, x, y, 1.0f, PANEL_MINIKITCOUNTSCALE * stats_xscale, PANEL_MINIKITCOUNTSCALE, PANEL_MINIKITCOUNTSCALE,
             0, 255, 0, 127, static_cast<u8>(static_cast<i32>(128.0f * alpha)));
    const u16 rotation = (NuFmod(GlobalTimer.time_elapsed, 4.0f) * 0.25f) * 65536.0f;
    const f32 scale = NU_SIN_LUT(static_cast<i32>(alpha * 16384.0f)) * PANEL_MINIKITSCALE;
    const u16 tilt = 1820.0f * NuTrigTable[rotation & 0x7fff];
    DrawPanel3DObjectNoAlpha(x, y + PANEL_MINIKITY - PANEL_MINIKITCOUNTY, 1.0f, scale, scale, scale, tilt, rotation, 0,
                             &WORLD->lev_objs[object].special, 2);
}

static void Hub_MakeFreePlayList(i32 first_model, i32 second_model) {
    fpcount = 0;
    const i32 area = LDataList[hub_new_level].area_index;

    if (hub_makefreeplaylist_addotherid != 0) {
        const i32 other_player = PlayerID[1];
        if (second_model == -1 && other_player != -1 && other_player != PlayerID[0] && other_player != first_model) {
            second_model = other_player;
        }
        hub_makefreeplaylist_addotherid = 0;
    }

    MakeFreePlayModelList(first_model, second_model, area, -1, 1);
    if ((ADataList[area].flags & (AREAFLAG_VEHICLE_AREA | AREAFLAG_BONUS_AREA)) ==
        (AREAFLAG_VEHICLE_AREA | AREAFLAG_BONUS_AREA)) {
        for (i32 index = 0; index < FreePlayModelCount; ++index) {
            const i32 model = FreePlayModelList[index].model_id;
            if ((CDataList[model].model_flags & HUB_FREEPLAY_MODEL_MINIKIT) == 0 ||
                InCollectList_Index(model, MiniKitCollection.list, MiniKitCollection.count_y) == -1 ||
                PlayerList[0] == model || PlayerList[1] == model || Collection_Got(model) == 0) {
                continue;
            }
            fplist[fpcount++] = FreePlayModelList[index];
        }
    } else {
        const i32 selectable_count = FreePlayResidentCount + FreePlayBonusCount;
        for (i32 index = 2; index < selectable_count + 2 && FreePlayModelList[index].model_id != -1; ++index) {
            const i32 model = FreePlayModelList[index].model_id;
            const bool is_vehicle = (CDataList[model].model_flags & HUB_FREEPLAY_MODEL_VEHICLE) != 0;
            const bool area_uses_vehicles = area != -1 && (ADataList[area].flags & AREAFLAG_VEHICLE_AREA) != 0;
            if ((area == -1 || is_vehicle == area_uses_vehicles) && Collection_Got(model) != 0) {
                fplist[fpcount++] = FreePlayModelList[index];
            }
        }
    }

    // MakeFreePlayModelList is still only partially reconstructed. Until it supplies
    // the resident/bonus tail, use the same area collection the original selector
    // filters so the UI has the unlocked roster rather than an empty list.
    if (fpcount == 0) {
        COLLECTION_s *collection = GetFreePlayCollection(area);
        if (collection != NULL && collection->list != NULL) {
            for (i32 index = 0; index < collection->count_y && fpcount < 340; ++index) {
                const i32 model = collection->list[index].id;
                if (model >= 0 && model < CHARCOUNT && Collection_Got(model) != 0) {
                    fplist[fpcount].model_id = static_cast<i16>(model);
                    fplist[fpcount].count = 1;
                    ++fpcount;
                }
            }
        }
    }

    fplist[fpcount].model_id = -1;
    if (fpcount > 3) {
        for (i32 shuffle = 0; shuffle < 64; ++shuffle) {
            const i32 first_offset = qrand() / (0xffff / (fpcount - 2) + 1);
            const i32 first_index = first_offset + 2;
            const i32 second_offset = qrand() / (0xffff / (fpcount - 3) + 1);
            const i32 second_index = (second_offset + first_offset) % (fpcount - 2) + 2;
            const APICHARACTERMODELLIST_s saved = fplist[first_index];
            fplist[first_index] = fplist[second_index];
            fplist[second_index] = saved;
        }
    }
}

static __used__ void Hub_UpdateSelectMode() {
    MENU *menu = &GameMenu[GameMenuLevel];

    if (selectmodemode == 2 || selectmodemode == 3) {
        selectmodetime += FRAMETIME;
        if (selectmodetime < selectmodeduration) {
            return;
        }

        if (selectmodemode == 3) {
            WipeBackToHub();
            return;
        }

        if (NewLData != NULL) {
            return;
        }
        NextArea_FreePlay = 0;
        FreePlay = 0;
        NewLData = &LDataList[hub_new_level];
        loadareacharacters_no_backdrop_reset = 1;
        const FADETYPE fade = {FADE_TYPE_STILL};
        FadeSys.SetFade(fade, 0);
        FinishLoop_On = 0;
        return;
    }

    if (menu->cancel_pressed != 0) {
        MenuSFX = GameAudio_GetSfxId(0x31);
        selectmodetime = 0.0f;
        selectmodemode = 3;
        selectmodeduration = 0.6f;
        return;
    }
    if (menu->confirm_pressed == 0) {
        hub_selectmode = menu->selected_item;
        return;
    }

    const i32 area = LDataList[hub_new_level].area_index;
    hub_selectmode = menu->selected_item;
    if (hub_selectmode == 0) {
        MenuSFX = GameAudio_GetSfxId(0x30);
        selectmodetime = 0.0f;
        selectmodemode = 2;
        selectmodeduration = 0.6f;
        return;
    }
    if (hub_selectmode == 1 && area >= 0 && area < AREACOUNT &&
        (LOSTTEMPLE_ADATA == NULL || area != LOSTTEMPLE_ADATA->index) && FreePlayUnlocked() &&
        (ADataList[area].flags & AREAFLAG_NO_FREEPLAY) == 0 && Game_AreaSave != NULL &&
        Game_AreaSave[area].area_complete != 0) {
        MenuSFX = GameAudio_GetSfxId(0x30);
        hub_freeplaysource = 0;
        Hub_InitFreePlaySelect(area, -1, -1);
        NewMenu(17, -1, -1);
        return;
    }

    MenuSFX = GameAudio_GetSfxId(0x32);
}

void MenuUpdateSelectMode(MENU_s *) {
    Hub_UpdateSelectMode();
}

extern AREADATA *E1CHARACTER_ADATA, *E2CHARACTER_ADATA, *E3CHARACTER_ADATA, *E4CHARACTER_ADATA, *E5CHARACTER_ADATA,
    *E6CHARACTER_ADATA, *UTAPAU_ADATA, *HOTH_ADATA;
extern i16 id_PALPATINE, id_LAMASU, id_WOOKIEE, id_RANCOR, id_JAWA, id_WAMPA, id_BAT, id_HANINCARBONITE, id_EWOK;
struct ARCADE_LEVEL_s {
    AREADATA **area;
    i16 *character_id;
};
ARCADE_LEVEL_s ArcadeLevel[12] = {{&E1CHARACTER_ADATA, &id_GUNGAN},
                                  {&SENATE_ADATA, &id_PALPATINE},
                                  {&BONUSKAMINO_ADATA, &id_LAMASU},
                                  {&E2CHARACTER_ADATA, &id_GEONOSIAN},
                                  {&UTAPAU_ADATA, &id_GRIEVOUS},
                                  {&BONUSKASHYYYK_ADATA, &id_WOOKIEE},
                                  {&E3CHARACTER_ADATA, &id_RANCOR},
                                  {&E4CHARACTER_ADATA, &id_JAWA},
                                  {&HOTH_ADATA, &id_WAMPA},
                                  {&BONUSDAGOBAH_ADATA, &id_BAT},
                                  {&E5CHARACTER_ADATA, &id_HANINCARBONITE},
                                  {&E6CHARACTER_ADATA, &id_EWOK}};
extern i16 tSUPERSTORY, tCHARACTERBONUS, tVEHICLEBONUS;
i32 hub_bonusmode = 0;
extern f32 text3d_width, text3d_height;
void MenuInitBonusMode(MENU_s *) {
    bonusmodemode = 0;
    hub_bonusmode = 0;
    hub_bonusarea = LDataList[hub_new_level].area_index;
    hub_bonusepisode = static_cast<i8>(ADataList[hub_bonusarea].episode_index);
    bonusmodearcade = SENATE_ADATA != NULL && hub_bonusarea == SENATE_ADATA->index;
}
static __used__ void Hub_DrawBonusModeMenu(int selected, float alpha) {
    char text[3][128];
    if (bonusmodearcade) {
        const i32 full_opacity = static_cast<i32>(alpha * 128.0f);
        f32 y = 0.3062499761581421f;
        MENU *menu = &GameMenu[GameMenuLevel];
        AREADATA *area;
        for (i32 i = 0; i < 3; ++i) {
            i16 *label;
            memcpy(&label, reinterpret_cast<const u8 *>(&ArcadeItem) + i * 8, sizeof(label));
            if (label)
                NuStrCpy(text[0], TTab[*label]);
            else
                text[0][0] = 0;
            i32 opacity = full_opacity;
            if (i == 0) {
                if (text[0][0])
                    NuStrCat(text[0], ": ");
                area = *ArcadeLevel[ArcadeItem.level].area;
                if (area && area->name_id != 0 && TTab[area->name_id])
                    NuStrCat(text[0], TTab[area->name_id]);
                else
                    NuStrCat(text[0], "?");
            } else if (i == 1) {
                if (text[0][0])
                    NuStrCat(text[0], ": ");
                char *mode_text = TTab[*Arcade_Mode[static_cast<i8>(ArcadeItem.field_c_0xc)].text];
                NuStrCat(text[0], mode_text ? mode_text : "?");
                if (area && Game_LevelSave && area->levels[0] != -1) {
                    const i32 level = area->levels[0];
                    if ((reinterpret_cast<LEVELSAVE_s *>(Game_LevelSave)[level].arcade_flags &
                         (1u << (static_cast<i8>(ArcadeItem.field_c_0xc) & 31))) != 0)
                        NuStrCat(text[0], " ~2$");
                    if ((reinterpret_cast<LEVELSAVE_s *>(Game_LevelSave)[level].arcade_flags & 7) == 7) {
                        const i32 rotation =
                            static_cast<i32>((NuFmod(GlobalTimer.time_elapsed, 4.0f) * 0.25f) * 65536.0f) & 0xffff;
                        const f32 scale = NU_SIN_LUT(static_cast<i32>(alpha * 16384.0f)) * PANEL_REDBRICKSCALE;
                        const u16 tilt = static_cast<i32>(1820.0f * NuTrigTable[rotation & 0x7fff]) - 0x1555;
                        DrawPanel3DObjectNoAlpha(0.0f, y + 0.35f, 1.0f, scale, scale, scale, tilt, rotation, 0,
                                                 &WORLD->lev_objs[211].special, 2);
                    }
                }
            } else if (*ArcadeLevel[ArcadeItem.level].area == NULL)
                opacity = full_opacity / 3;
            i32 red, green, blue;
            if (selected && i == hub_bonusmode && TestForController()) {
                if (menu_pulsate > 0.0f) {
                    red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulsate +
                                           static_cast<u32>(MENUFLASH1R) * (1.0f - menu_pulsate));
                    green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulsate +
                                             static_cast<u32>(MENUFLASH1G) * (1.0f - menu_pulsate));
                    blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulsate +
                                            static_cast<u32>(MENUFLASH1B) * (1.0f - menu_pulsate));
                } else {
                    red = menu_flash ? MENUFLASH0R : MENUFLASH1R;
                    green = menu_flash ? MENUFLASH0G : MENUFLASH1G;
                    blue = menu_flash ? MENUFLASH0B : MENUFLASH1B;
                }
            } else if (menu_pulse > 0.0f) {
                red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulse +
                                       static_cast<u32>(MENUNORMALR) * (1.0f - menu_pulse));
                green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulse +
                                         static_cast<u32>(MENUNORMALG) * (1.0f - menu_pulse));
                blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulse +
                                        static_cast<u32>(MENUNORMALB) * (1.0f - menu_pulse));
            } else {
                red = MENUENTRYR;
                green = MENUENTRYG;
                blue = MENUENTRYB;
            }

            smarttextex_drawmessagebox = 1;
            SmartTextEx(text[0], 0.0f, y, 1.0f, 0.7f, 0.7f, 0.7f, 0, static_cast<u8>(red), static_cast<u8>(green),
                        static_cast<u8>(blue), 1.7f, 1, 0, 0, opacity);
            menu->item_x[i] = 0.0f;
            menu->item_y[i] = y;
            menu->item_width[i] = text3d_width;
            menu->item_height[i] = text3d_height;
            menu->item_column[i] = 0;
            menu->item_row[i] = i;

            y -= 0.175f;
        }
        return;
    }
    const i32 menu_level = GameMenuLevel;
    if (hub_new_level == -1 || LDataList[hub_new_level].area_index == -1 ||
        (ADataList[LDataList[hub_new_level].area_index].flags & 0x100) == 0)
        return;
    i32 count;
    if (hub_bonusepisode != -1) {
        NuStrCpy(text[0], TTab[tSUPERSTORY]);
        NuStrCpy(text[1], TTab[tCHARACTERBONUS]);
        NuStrCpy(text[2], TTab[tVEHICLEBONUS]);
        const i32 character_area = Episode_FindAreaFromFlags(&EDataList[hub_bonusepisode], 5, 4);
        const i32 vehicle_area = Episode_FindAreaFromFlags(&EDataList[hub_bonusepisode], 5, 5);
        if (character_area != -1 && ADataList[character_area].name_id != -1 &&
            TTab[ADataList[character_area].name_id]) {
            const i32 name = ADataList[character_area].name_id;
            NuStrCat(text[1], " (");
            NuStrCat(text[1], TTab[name]);
            NuStrCat(text[1], ")");
        }
        if (vehicle_area != -1 && ADataList[vehicle_area].name_id != -1 && TTab[ADataList[vehicle_area].name_id]) {
            const i32 name = ADataList[vehicle_area].name_id;
            NuStrCat(text[2], " (");
            NuStrCat(text[2], TTab[name]);
            NuStrCat(text[2], ")");
        }
        count = 3;
    } else {
        NuStrCpy(text[0], TTab[hub_bonusarea != -1 ? ADataList[hub_bonusarea].name_id : -1]);
        count = 1;
    }
    f32 y = (count - 1) * 0.1f + 0.125f;
    const i32 full_opacity = static_cast<i32>(alpha * 128.0f);
    MENU *menu = &GameMenu[menu_level];
    for (i32 i = 0; i < count; ++i) {
        i32 red, green, blue;
        if (selected && i == hub_bonusmode && TestForController()) {
            if (menu_pulsate > 0.0f) {
                red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulsate +
                                       static_cast<u32>(MENUFLASH1R) * (1.0f - menu_pulsate));
                green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulsate +
                                         static_cast<u32>(MENUFLASH1G) * (1.0f - menu_pulsate));
                blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulsate +
                                        static_cast<u32>(MENUFLASH1B) * (1.0f - menu_pulsate));
            } else {
                red = menu_flash ? MENUFLASH0R : MENUFLASH1R;
                green = menu_flash ? MENUFLASH0G : MENUFLASH1G;
                blue = menu_flash ? MENUFLASH0B : MENUFLASH1B;
            }
        } else if (menu_pulse > 0.0f) {
            red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulse +
                                   static_cast<u32>(MENUNORMALR) * (1.0f - menu_pulse));
            green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulse +
                                     static_cast<u32>(MENUNORMALG) * (1.0f - menu_pulse));
            blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulse +
                                    static_cast<u32>(MENUNORMALB) * (1.0f - menu_pulse));
        } else {
            red = MENUENTRYR;
            green = MENUENTRYG;
            blue = MENUENTRYB;
        }

        i32 opacity = full_opacity;
        if (i == 2 && Collection_GetIDList(&MiniKitCollection, 0x4000000, 0x4000000, NULL, NULL, NULL, 0) <= 0)
            opacity = full_opacity / 4;
        smarttextex_drawmessagebox = 1;
        SmartTextEx(text[i], 0.0f, y, 1.0f, 0.7f, 0.7f, 0.7f, 0, static_cast<u8>(red), static_cast<u8>(green),
                    static_cast<u8>(blue), 1.7f, 1, 0, 0, opacity);
        menu->item_x[i] = 0.0f;
        menu->item_y[i] = y;
        menu->item_width[i] = text3d_width;
        menu->item_height[i] = text3d_height;
        menu->item_column[i] = 0;
        menu->item_row[i] = i;

        y -= 0.2f;
    }
}

void Hint_SetHintFromId(i32, i32, i32);
void Hint_Draw(i32);
extern f32 text3d_width, text3d_height;
extern i16 tREPLAYSTORY, tCHALLENGE, tLOCKED;
static __used__ void Hub_DrawSelectModeMenu(int selected, float alpha) {
    const i32 area = LDataList[hub_new_level].area_index;
    const i32 complete = Game.area_save[area].area_complete;
    const i32 lost_temple = LOSTTEMPLE_ADATA != NULL && area == LOSTTEMPLE_ADATA->index;
    i32 unlocked = FreePlayUnlocked();
    if (unlocked != 1 && area != -1) {
        unlocked = ADataList[area].episode_index == 0xff && (ADataList[area].flags & 0x4000) != 0;
    }
    i32 text_ids[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    i32 opacity[10];
    i32 count = 0;
    if (!lost_temple) {
        text_ids[count] = complete ? tREPLAYSTORY : tSTORY;
        opacity[count++] = 128;
        if ((ADataList[area].flags & 0x1000) == 0) {
            text_ids[count] = tFREEPLAY;
            opacity[count++] = complete && unlocked ? 128 : 48;
            if ((ADataList[area].flags & 0x10) != 0 && Store_IsPackUnlocked(8)) {
                text_ids[count] = tCHALLENGE;
                opacity[count++] = complete && Store_IsPackUnlocked(8) ? 128 : 48;
            }
        }
    }
    f32 y = (count - 1) * 0.1f + 0.125f;
    if (text_ids[1] == tFREEPLAY && !unlocked)
        y += 0.2f;
    MENU *menu = &GameMenu[GameMenuLevel];
    for (i32 i = 0; i < count; ++i) {
        i32 red, green, blue;
        if (selected && i == hub_selectmode && TestForController()) {
            if (menu_pulsate > 0.0f) {
                red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulsate +
                                       static_cast<u32>(MENUFLASH1R) * (1.0f - menu_pulsate));
                green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulsate +
                                         static_cast<u32>(MENUFLASH1G) * (1.0f - menu_pulsate));
                blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulsate +
                                        static_cast<u32>(MENUFLASH1B) * (1.0f - menu_pulsate));
            } else {
                red = menu_flash ? MENUFLASH0R : MENUFLASH1R;
                green = menu_flash ? MENUFLASH0G : MENUFLASH1G;
                blue = menu_flash ? MENUFLASH0B : MENUFLASH1B;
            }
        } else if (menu_pulse > 0.0f) {
            red = static_cast<i32>(static_cast<u32>(MENUFLASH0R) * menu_pulse +
                                   static_cast<u32>(MENUNORMALR) * (1.0f - menu_pulse));
            green = static_cast<i32>(static_cast<u32>(MENUFLASH0G) * menu_pulse +
                                     static_cast<u32>(MENUNORMALG) * (1.0f - menu_pulse));
            blue = static_cast<i32>(static_cast<u32>(MENUFLASH0B) * menu_pulse +
                                    static_cast<u32>(MENUNORMALB) * (1.0f - menu_pulse));
        } else {
            red = MENUENTRYR;
            green = MENUENTRYG;
            blue = MENUENTRYB;
        }
        const i32 text_id = text_ids[i];
        char *text = TTab[text_id];
        char buffer[128];
        if (!unlocked) {
            if (alpha == 1.0f) {
                Hint_SetHintFromId(0x164, 0, 1);
                Hint_Draw(-1);
            } else {
                Hint_CancelCurrent();
            }
            if (text_id == tFREEPLAY) {
                sprintf(buffer, "%s (%s)", text, TTab[tLOCKED]);
                text = buffer;
            }
        }
        smarttextex_drawmessagebox = 1;
        SmartTextEx(text, 0.0f, y, 1.0f, 0.7f, 0.7f, 0.7f, 0, static_cast<u8>(red), static_cast<u8>(green),
                    static_cast<u8>(blue), 1.7f, 1, 0, 0, static_cast<i32>(opacity[i] * alpha));
        menu->item_x[i] = 0.0f;
        menu->item_y[i] = y;
        menu->item_width[i] = text3d_width;
        menu->item_height[i] = text3d_height;
        menu->item_column[i] = 0;
        menu->item_row[i] = i;
        y -= 0.2f;
    }
}

static __used__ void Hub_DrawSuperBonusStats(AREADATA_s *, float) {
}

void WipeBackToHub() {
    NewMenu(-1, -1, -1);
    ResetTimer(&JoinInTimer, 0.0f);
    NuStrCpy(Door_ExitName, Door_Last->name);
    StartDoorPositions();
    NewGameMode();
    bonusmodearcade = 0;
    HubMainRenderTimeHack = 1;
}
