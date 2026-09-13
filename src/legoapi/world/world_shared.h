#ifndef LEGOAPI_WORLD_SHARED_H
#define LEGOAPI_WORLD_SHARED_H

// Shared internal declarations for the world-loading subsystem split.  This
// header is the glue between the WorldInfo module (world.cpp) and the moved-out
// subsystem files (render, terrain, sfx, gizmo_sys, ai_sys, game_object,
// cutscene, levelstubs).  It declares the cross-file entry points and the few
// globals that are shared between translation units.

#include "globals.h"
#include "legoapi/world/level.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/numath/nuvec.h"

// Struct tags for helper functions whose C++ mangled names must match the
// original libTTapp.so exactly.  Forward declarations only — the stubs never
// dereference these.
struct GIZMOSYS_s;
struct NUGCUTSCENE_s;
struct instNUGCUTSCENE_s;
struct GIZFLOW_s;
struct COLLECTION_s;
struct APIOBJECTSYS_s;
struct SCENEPROGRESS_s;

// --- Shared globals across the world-loading subsystems ---

// World-loader global (defined in world.cpp, used by terrain.cpp)
extern LEVELDATA *PLATFORM_LDATA;

// Terrain subsystem (defined in terrain.cpp, used by world.cpp)
extern char *debris_name[147];

// Gizmo subsystem (defined in gizmo_sys.cpp, used by world.cpp)
extern COLLECTION_s MiniKitCollection;

// Cutscene subsystem (defined in cutscene.cpp, used by world.cpp)
extern i32 CUTCAM;

// World data (defined in world.cpp)
extern WORLDINFO WorldInfo[2];
extern WORLDINFO *WORLD;
extern NUGSCN *things_scene;
extern void *things_scene_terrain;

// --- render.cpp — nu3d / NuGScn ---
extern "C" {
    void NuDisplaySceneRndr(void *scene);
    i32 NuPortalVisibility(NUGSCN *scene);
    void NuGScnRndr3(NUGSCN *scene);
    void NuGScnUpdate(NUGSCN *gscn, f32 frame_delta);
    NUGSCN *NuGScnRead(VARIPTR *buf, VARIPTR buf_end, char *path);
    void NuGScnRemove(NUGSCN *scene);
    void NuGScnReadFromMemory(NUGSCN *scene);
    NUGSCN *NuGHGFixup(NUGSCN *scene, void *);
    void NuRndrInitWorld(void);
}
i32 NuGScnUploadGfxDataFromFilePS(VARIPTR *buf, VARIPTR *buf_end, i32 file);
// The trailing flags argument is passed as 1 by every caller in the
// original binary; the original implementation never reads it.
i32 NuSpecialFind(NUGSCN *scene, nuhspecial_s *dest, char *name, i32 flags);
void SetCameraZoom(f32 zoom);

// --- chris.cpp ---
void ChrisAllocLevelStuff(WORLDINFO_s *world);

// --- terrain.cpp — terrain / debris / grass / bridge / particles ---
extern "C" {
    void DebrisSetThinningLevel(f32 level);
    void DebrisSetForcedThinning(i32 forced);
    void DebrisSetDetailLevel(i32 level);
    void noterraininit(void);
    void TerrainSetCur(void *terrain);
    void TerrSetPlatScanDist(f32 dist);
    void TerrainPlatformOldUpdate(void);
    void TerrainPlatformNewUpdate(void);
    void TerrainSetWallDeflectYScale(f32 scale);
    void NewTerrainScaleYMask(NUVEC *position, NUVEC *movement, u8 *hit_flags, i32 object_index, f32 radius,
                              f32 collision_radius, f32 object_scale, i32 embedded_retry, i32 scan_flags,
                              i32 terrain_mask);
    void *TerrainInitEx(i32 param1, void *buf, void *buf_end, i32 param2, char *path, void *gscn, i32 param3,
                        u32 param4, u32 param5, u32 param6);
}

// Results written by the original terrain query pipeline.
extern i16 TerrImpact;
extern i32 terrhitflags;
extern NUVEC TerrImpactPos;
extern NUVEC TerrImpactNormal;
extern NUVEC ShadNorm;
void *InitPartDebris(VARIPTR *buf, VARIPTR *buf_end, i32 param1, i32 param2, char **param3, i32 page);
void LoadTerrainFile(WORLDINFO *world);
void LoadGrassFile(WORLDINFO *world);
void LoadBridgeFile(WORLDINFO *world);
void LoadPartFile(WORLDINFO *world);
void Particles_Load(WORLDINFO *world, char **debris_name, i32 count, i32 flags);

// --- sfx.cpp — sound / SFX / quiet tables ---
extern "C" void ResetSounds(void);
void SetLevelSfxBits(WORLDINFO *world);
void ResetLevSfx(WORLDINFO *world);
i32 ActionFromQuiet(i32 idx);
i32 AmbientFromQuiet(i32 idx);
bool InitSpecialSfx(WORLDINFO *world);
void LoadSpecialSfxFile(WORLDINFO *world);

// --- gizmo_sys.cpp — gizmo / minikit / collection ---
GIZMOSYS_s *CreateGizmoSys(void *world, VARIPTR *buf, VARIPTR *buf_end);
void LoadGizmoSys(GIZMOSYS_s *gizmo_sys, void *world, char *config_file);
i32 LoadEditorSplines(char *path, VARIPTR *buf, VARIPTR *buf_end);
void Hub_LoadAndFixUpMiniKits(WORLDINFO *world, VARIPTR *buf, VARIPTR *buf_end);
void MiniKit_Load(MINIKIT *minikit, i32 id, VARIPTR *buf, VARIPTR *buf_end, void *param);
void MiniKit_InitPieces(MINIKIT *minikit, i32 count, VARIPTR *buf, VARIPTR *buf_end);
void CharacterMiniKits_Load(COLLECTION_s *collection, WORLDINFO *world, VARIPTR *buf, VARIPTR *buf_end);
void GizmoSysAddGizmos(GIZMOSYS_s *gizmo_sys, GIZFLOW_s *giz_flow, void *world);
void *LoadGizFlow(void *world, GIZMOSYS_s *gizmo_sys, char *path, VARIPTR *buf, VARIPTR *buf_end);

// --- ai_sys.cpp — AI system + API object system ---
extern "C" {
    void *AISysLoad(void *buf, void *buf_end, i32 size, void *gscn, char *dir, char *name, char *param);
    APIOBJECTSYS_s *APIObjectSysInit(i32 size, VARIPTR *buf, VARIPTR *buf_end);
}
void *AIPathCnxControlSysCreate(VARIPTR *buf, VARIPTR *buf_end, i32 count);
void *AIPathCnxHelperSysCreate(VARIPTR *buf, VARIPTR *buf_end, i32 count);
AITRIGGERSETSYS_s *AITriggerSetSysCreate(VARIPTR *buf, VARIPTR *buf_end);
void GameAIScriptAddLevelSfx(WORLDINFO *world, NULISTHDR *scripts);
void *CreateClimbObjectSys(VARIPTR *buf, VARIPTR *buf_end, i32 count);

// --- game_object.cpp — game objects / api objects ---
void ClearGameObjects(APIOBJECTSYS_s *api_object_sys);
GameObject_s *AddGameObject(i32 id);
i32 InitCreature(GameObject_s *obj, i32 id, i32 param);
void InitGameObjectLights(void);

// --- cutscene.cpp — cutscenes / character scenes ---
i32 InStory(void);
extern "C" {
    NUGCUTSCENE_s *NuGCutSceneLoad(char *name, VARIPTR *buf, VARIPTR *buf_end, i32 flags);
    void NuGCutSceneFixUp(NUGCUTSCENE_s *cutscene, NUGSCN *scene, i32 flags, i8 area);
    void NuGCutSceneFixUpExtra(NUGCUTSCENE_s *cutscene, NUGSCN *scene);
    instNUGCUTSCENE_s *instNuGCutSceneCreate(NUGCUTSCENE_s *cutscene, NUGSCN *scene, void *extra, char *name,
                                             VARIPTR *buf, i32 flags);
}
void *CutScenes_Load(char *config, NUGSCN *gscn1, NUGSCN *gscn2, i32 param1, VARIPTR *buf, VARIPTR *buf_end, i32 param2,
                     i32 param3, WORLDINFO *world);
void CharScenes_LevelLoad(WORLDINFO *world);

// --- levelstubs.cpp — WorldInfo_Load dependency stubs ---
void LevObj_FixUpPlatIDs(WORLDINFO *world);
void Doors_Init(WORLDINFO *world);
void LevelSplines_InitForLevel(WORLDINFO *world);
void LevelObjects_InitForLevel(WORLDINFO *world);
void BoltTypes_Init(WORLDINFO *world);
void BoltTypes_Configure(WORLDINFO *world, char *config);
void EquivalentObjects_Configure(WORLDINFO *world, char *config);
void Teleports_Configure(WORLDINFO *world, char *config);
void Doors_Configure(WORLDINFO *world, char *config);
void Faders_Configure(WORLDINFO *world, char *config);
void CharPlatforms_Configure(WORLDINFO *world, char *config);
void Grabber_Configure(WORLDINFO *world, char *config);
void Pulses_Configure(WORLDINFO *world, char *config);
void TrafficAnimSys_Configure(WORLDINFO *world, char *config);
void SpecialMiniKits_Configure(WORLDINFO *world, char *config);
void GizForceSFX_Configure(WORLDINFO *world, char *config);
void RippleEffects_Configure(WORLDINFO *world, char *config);
void PortalDoors_Configure(WORLDINFO *world, char *config);
void LoadLights(WORLDINFO *world, char *path);
GAMEANIMSYS_s *GameAnimSys_Create(VARIPTR *buf, VARIPTR *buf_end);
void *GameAntnode_CreateSys(WORLDINFO *world, VARIPTR *buf, VARIPTR *buf_end, i32 count);
extern "C" {
    void SockSys_Configure(void *sock_sys, char *config, i32 param, void *buf, void *buf_end, void *gscn);
    void rtlResetDynamic(void);
    void SetPartRTLSet(usize rtl_set);
    i32 rtlFindByUserId(usize rtl_set, i32 user_id);
    void rtlGetDirection(usize rtl_set, i32 id, void **out);
    void NewMenu(i32 menu_id, i32 menu_y, i32 param3);
}

#endif // LEGOAPI_WORLD_SHARED_H
