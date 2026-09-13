#pragma once

#include "legoapi/world/world.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/input/timer.h"
#include "nu2api/nucore/common.h"

// Forward declarations for tags referenced by the level-loading externs and
// entry-point prototypes declared at the end of this header.  Full definitions
// live in the headers that own them.
struct GameObject_s;
struct GIZFORCE_s;
struct nuvec_s;
struct numtx_s;
struct CUTINFO;
struct CUTSYS;
struct MISSIONSYS_s;
struct CUSTOMISER;
struct TORPEDOPACKET_s;
struct SOCKPOSITION_s;
struct AREADATA_s;

struct LEVELSCRIPTPROGRESS_s {
    char name[16];
    f32 params[4];
};
typedef LEVELSCRIPTPROGRESS_s LEVELSCRIPT_PROGRESS_s;
void LevelScriptReStoreProgress(WORLDINFO_s *, LEVELSCRIPTPROCESS_s *);
DECOMP_ASSERT(sizeof(LEVELSCRIPTPROGRESS_s) == 0x20, "Saved script progress ABI");

struct LEVEL_PROGRESS_s {
    LEVEL_PROGRESS_DATA_s data;
    union {
        i32 flags;
        struct {
            u8 flags_low;
            u8 flags_upper[3];
        };
    };
    // One bit per object slot. ResetAICreatures uses this saved mask to keep
    // creatures that were permanently removed from being recreated.
    u32 disabled_ai_object_mask[2];
    f32 grabber_field_0x48c;
    f32 grabber_field_0x484;
    f32 grabber_field_0x494;
    u8 pad_2818[4];
    u32 played_cutscene_mask;
    LEVELSCRIPTPROGRESS_s scripts[32];
    GIZFLOWPROGRESS_s giz_flow_progress;
    char disabled_effect_names[12][16];
};

DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, played_cutscene_mask) == 0x281c, "LEVEL_PROGRESS cutscene mask offset");
DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, flags_low) == 0x2800, "LEVEL_PROGRESS low flags offset");
DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, giz_flow_progress) == 0x2c20, "LEVEL_PROGRESS flow progress offset");
DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, scripts) == 0x2820, "Saved level scripts offset");
DECOMP_ASSERT(sizeof(LEVEL_PROGRESS_s) == 0x2e24, "LEVEL_PROGRESS size");
DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, scripts) == 0x2820, "Saved script progress offset");
DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, giz_flow_progress) == 0x2c20, "LEVEL_PROGRESS gizmo flow offset");
DECOMP_ASSERT(offsetof(LEVEL_PROGRESS_s, disabled_effect_names) == 0x2d64, "LEVEL_PROGRESS disabled effects offset");

typedef struct LEVELDATADISPLAY {
    f32 unknown_00;
    f32 unknown_04;

    f32 far_clip;
    f32 fog_start;
    f32 particle_thin;

    i16 unknown_14;

    char bg_red_top;
    char bg_red_bottom;

    char bg_green_top;
    char bg_green_bottom;

    char bg_blue_top;
    char bg_blue_bottom;
} LEVELDATADISPLAY;

enum LEVEL_FLAGS : u32 {
    LEVEL_CONFIG_LOADED = 1 << 0,               // 0x1  set once level config has been (re)loaded
    LEVEL_GAMEPLAY = 1 << 1,                    // 0x2  playable level (has world: SockSys/AI loaded)
    LEVEL_UNKNOWN_FLAG_4 = 1 << 2,              // 0x4  set by default on level_start
    LEVEL_TERRAIN = 1 << 3,                     // 0x8  level has a terrain file
    LEVEL_FLAT_TERRAIN = 1 << 4,                // 0x10
    LEVEL_INTRO = 1 << 5,                       // 0x20
    LEVEL_MIDTRO = 1 << 6,                      // 0x40
    LEVEL_OUTRO = 1 << 7,                       // 0x80
    LEVEL_FIX_STROBING_ANIMS = 1 << 8,          // 0x100
    LEVEL_TEST = 1 << 9,                        // 0x200
    LEVEL_STATUS = 1 << 10,                     // 0x400
    LEVEL_DOUBLE_SCORE = 1 << 11,               // 0x800
    LEVEL_METAL = 1 << 12,                      // 0x1000
    LEVEL_SHOW_COIN_TOTAL = 1 << 13,            // 0x2000
    LEVEL_CAMERA_RAIN = 1 << 14,                // 0x4000
    LEVEL_TERRAIN_RAIN = 1 << 15,               // 0x8000
    LEVEL_NEWGAME = 1 << 16,                    // 0x10000
    LEVEL_LOADGAME = 1 << 17,                   // 0x20000
    LEVEL_IN_SPACE = 1 << 18,                   // 0x40000
    LEVEL_PICKUPS_TO_PANEL = 1 << 19,           // 0x80000
    LEVEL_FORGET_TAKEOVERS = 1 << 20,           // 0x100000
    LEVEL_NARROW_SOCKS = 1 << 21,               // 0x200000
    LEVEL_OVERRIDE_NO_PICKUP_GRAVITY = 1 << 22, // 0x400000
    LEVEL_HIDE_ICONS = 1 << 23,                 // 0x800000 (the config keyword is inverted)
};

typedef struct LEVELDATA_s {
    char dir[0x40];
    char name[0x20];

    i16 unknown_060;

    i16 idx;

    u32 flags;

    void (*load_fn)(WORLDINFO *, VARIPTR *, VARIPTR *);
    void (*init_fn)(WORLDINFO *);
    void (*reset_fn)(WORLDINFO *);
    void (*update_fn)(WORLDINFO *);
    void (*always_update_fn)(WORLDINFO *);
    void (*draw_fn)(WORLDINFO *);
    void (*draw_status_fn)(WORLDINFO *);

    LEVELDATADISPLAY data_display;

    i16 music_index;

    i16 unknown_0a2;
    i16 max_ter_platforms;
    u16 max_ter_groups;
    i16 unknown_0a8;
    i16 unknown_0aa;

    char mipmap_mode;

    char blob_shadow_alpha;
    union {
        char unknown_0ae;
        i8 episode_index;
    };
    i8 area_index;

    f32 cam_tilt;

    f32 hover_height;

    char unknown_0b8;
    char unknown_0b9;
    char unknown_0ba;
    char unknown_0bb;
    char unknown_0bc;
    char unknown_0bd;
    char unknown_0be;
    char unknown_0bf;

    f32 unknown_0c0;
    f32 cam_pullback_dist;
    f32 cam_lateral_dist;
    f32 unknown_0cc;

    char unknown_0d0;
    char unknown_0d1;
    char unknown_0d2;
    char unknown_0d3;

    char area_level_index;

    char blob_shadow_fade_near;
    char blob_shadow_fade_far;

    char cam_pos_seek;
    char cam_angle_seek;

    union {
        char camera_judder_distance;
        char reflection_range;
    };
    char unknown_0da;
    char unknown_0db;

    f32 conveyor_x_speed;
    f32 conveyor_z_speed;

    u16 max_antinodes;
    u16 max_gizmo_blowups;
    u16 max_gizmo_blowup_types;
    u16 max_pickups;
    u16 max_obstacle_objs;
    u16 max_buildit_objs;
    u16 max_force_objs;
    u16 max_bombgen_objs;

    u8 max_tightropes;
    u8 max_giz_timers;
    u8 max_signals;
    u8 max_levers;
    u8 max_technos;
    u8 max_zipups;
    u8 max_grapples;
    u8 max_obstacles;
    u8 max_buildits;
    u8 max_shards;
    u8 max_spinners;
    u8 max_minicuts;
    u8 max_minicut_parts;
    u8 max_giz_specials;
    u8 max_attractos;
    u8 max_climb_objs;
    u8 max_guidelines;
    u8 max_ledges;
    u8 max_security_doors;
    u8 max_tubes;
    u8 max_giz_panels;
    u8 max_hat_machines;
    u8 max_force;
    u8 max_push_blocks;
    u8 max_push_block_end_pos;
    u8 max_doors;
    u8 max_teleports;
    u8 max_giz_randoms;
    u8 max_torp_machines;
    u8 max_spinner_anim_objs;
    u8 max_turrets;
    u8 max_bombgens;
    u8 max_bridges;
    u8 max_plugs;

    u8 field89_0x116;
    u8 field90_0x117;
    i16 field91_0x118;
    u8 field92_0x11a;
    u8 field93_0x11b;

    f32 unknown_11c;
    f32 unknown_120;

    f32 wind_speed;
    f32 wind_size;

    i32 music_tracks[3][2];
} LEVELDATA;

typedef struct LEVELOBJECT {
    u8 kind;
    u8 pad_01;
    u8 pad_02;
    u8 pad_03;
    char *name;
} LEVELOBJECT;

enum LEVEL_OBJECT_SCENE_KIND : u8 {
    LEVEL_OBJECT_SCENE_THINGS = 0,
    LEVEL_OBJECT_SCENE_LEVEL = 1,
    LEVEL_OBJECT_SCENE_AREA = 2,
    LEVEL_OBJECT_SCENE_CHARACTER_ICON = 3,
    LEVEL_OBJECT_SCENE_SAVE_ICON = 4,
    LEVEL_OBJECT_SCENE_VEHICLE = 5,
    LEVEL_OBJECT_SCENE_BUTTON = 6,
};

// Runtime counterpart to an LEVELOBJECT table entry. The first twelve bytes
// are a normal Nu special handle; the loader adds its terrain platform id and
// the active flag consumed by Draw3DObjectMtx.
typedef struct LEVEL_OBJECT_RUNTIME_s {
    nuhspecial_s special; // 0x00
    i16 platform_id;      // 0x0c
    u8 active;            // 0x0e
    u8 pad_0f;
} LEVEL_OBJECT_RUNTIME;

DECOMP_ASSERT(sizeof(LEVEL_OBJECT_RUNTIME) == 0x10, "LEVEL_OBJECT_RUNTIME size");

typedef struct LEVELFIXUP {
    char *name;
    LEVELDATA **level;
    void (*load_fn)(WORLDINFO *, VARIPTR *, VARIPTR *);
    void (*init_fn)(WORLDINFO *);
    void (*reset_fn)(WORLDINFO *);
    void (*update_fn)(WORLDINFO *);
    void (*always_update_fn)(WORLDINFO *);
    void (*draw_fn)(WORLDINFO *);
    void (*draw_status_fn)(WORLDINFO *);
} LEVELFIXUP;

#ifdef __cplusplus
extern "C" {
#endif
    extern LEVELDATA *LDataList;
    extern LEVELDATA *levelconfig_ldata;

    extern LEVELDATA *NEWGAME_LDATA;
    extern LEVELDATA *LOADGAME_LDATA;

    extern LEVELFIXUP LevFixUp;

    extern i32 LEVELCOUNT;
#ifdef __cplusplus
}

typedef void LEVELSETDEFAULTSFN(LEVELDATA *);

LEVELDATA *Levels_ConfigureList(char *file, VARIPTR *buf, VARIPTR *buf_end, i32 max_level_count, i32 *level_count_out,
                                LEVELSETDEFAULTSFN *set_defaults_fn);

void Level_SetDefaults(LEVELDATA *level);

LEVELDATA *Level_FindByName(char *name, i32 *idx_out);

void Level_Draw(WORLDINFO *world);

void Level_Update(WORLDINFO *world);

i32 LevelObject_GetReflection(i32 objId);

char *LevelObject_FindNameFromIndex(i32 index);
i32 LevelObject_FindIndexFromName(char *name);
i32 LevelObject_FindIndexFromName_RefOnly(char *name);

i32 LevelObject_AddExtra(char *name, i32 kind);

void ClearLevelProgress(i32 index, WORLDINFO *world);

void FixUpLevels(LEVELFIXUP *fixup);

void Levels_FixUp(LEVELFIXUP *fixup);

typedef struct nufpcomjmp_s nufpcomjmp_s;

void Level_RegisterGameConfigKeywords(nufpcomjmp_s *beforeLoadKeywords, nufpcomjmp_s *afterLoadKeywords);

void GoToNewLevel(i32 levelIdx);

void LevelConfig_BeforeLoad(LEVELDATA *level, char *buffer, nufpcomjmp_s *keywords);
void LevelConfig_AfterLoad(LEVELDATA *level, char *buffer, nufpcomjmp_s *keywords);

void Level_LoadConfigFile(WORLDINFO *world);

// --- Cross-TU function prototypes (level-loading entry points) ---
extern void CompleteLevel(WORLDINFO_s *);
void StoreLevelProgress(WORLDINFO_s *);
i32 KillBoss(i32, i32, float);
void KillBossNewLevel(i32, i32, float, i32);
void KillBossCompleteLevel(i32, i32, float);
i32 KillBossPlayCutScene(i32, i32, float, char *name);
extern CUTINFO *NewCutScene(CUTINFO *, CUTSYS *, char *, i32);
void *SetLevelHack(i32);
void ResetLevel(WORLDINFO_s *, char *, i32);
extern i8 BoltType_FindIDByName(char *, WORLDINFO_s *);
extern BOLTTYPE_s *BoltType_FindByID(i32, WORLDINFO_s *);
extern BOLT_s *Bolt_Add(GameObject_s *, nuvec_s *, numtx_s *, i32, i32);
void TBOPENFN(char *, i32);
void TBCLOSEFN(char *, i32);
void InitMiniSnowTroopers(WORLDINFO_s *, i32, i32, i32);
void UpdateMiniSnowTroopers(WORLDINFO_s *);

void LevelStreaming_DoorOverride(WORLDINFO_s *, struct LEVELDATA_s *, float, float *);
#endif
