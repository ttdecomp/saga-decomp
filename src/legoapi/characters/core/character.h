#pragma once

#include "decomp.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nuvec.h"

struct CHARFIXUP {
    char *name;
    i16 *id;
};

typedef struct CHARFIXUP CHARFIXUP;

struct GameObject_s;
struct BLADE_s {
    i16 model;
    i16 glow_model;
    i16 hit_effect;
    i16 effect_6;
    i16 effect_8;
    u8 colour[3];
    u8 padding[3];
};
DECOMP_ASSERT(sizeof(BLADE_s) == 16, "BLADE size");
DECOMP_ASSERT(offsetof(BLADE_s, colour) == 10, "BLADE colour offset");
extern BLADE_s BladeTab[4];
struct nugscn_s;
struct nuhgobj_s;
struct CHARACTERMODEL_s;
struct ANIMPACKET_s;
struct NUJOINTANIM_s;
struct numtx_s;
struct WORLDINFO_s;
struct bgprocinfo_s;
using CHARACTERUPDATEFN = void (*)(GameObject_s *);
using MAKELAYERLISTFN = i32 (*)(CHARACTERMODEL_s *, i16 *, u32);

typedef char *CHARACTERNAMEFN(u8 character_index);
typedef char *GLOBALCHARACTERNAMEFN(i32 character_type);
typedef i32 CHARACTERGLOBALIDFN(u8 character_index);
typedef void *CHARACTERHGOBJFN(i32 character_type);
typedef void CHARACTERRENDERFN(struct nuvec_s *position, i16 angle, i32 character_type, i32 flags,
                               struct EDCREATURE_s *creature);
typedef f32 CHARACTERGOALSPEEDFN(struct APIOBJECT_s *object);
typedef i32 CHARACTERTYPEIDFN(char *name);
typedef f32 CHARACTERDISTANCEFN(i32 character_type);

struct CHARACTERANIM_s {
    char *name;
    u32 flags;
    union {
        i16 animation_id;
        i16 action_id;
    };
    u8 field_0x0a;
    u8 priority;
    i8 minimum_repetitions;
    i8 maximum_repetitions;
    u8 field_0x0e[2];
    f32 blend_in_time;     // 0x10
    f32 blend_out_time;    // 0x14
    f32 playback_rate;     // 0x18
    f32 movement_speed;    // 0x1c
    f32 movement_rate_cap; // 0x20
    f32 action_speed;      // 0x24
    union {
        f32 event_frames[4]; // 0x28
        struct {
            f32 event_frame_1;
            f32 event_frame_2;
            f32 event_frame_3;
            f32 event_frame_4;
        };
    };
    f32 stop_frame; // 0x38
    union {
        u8 field_0x3c[0x0c];
        NUVEC root_translation;
    };
    u16 misc_flags; // 0x48
    u8 locator;     // 0x4a
    u8 field_0x4b;
};

DECOMP_ASSERT(sizeof(CHARACTERANIM_s) == 0x4c, "CHARACTERANIM_s size");
DECOMP_ASSERT(offsetof(CHARACTERANIM_s, blend_in_time) == 0x10, "CHARACTERANIM_s blend-in offset");
DECOMP_ASSERT(offsetof(CHARACTERANIM_s, playback_rate) == 0x18, "CHARACTERANIM_s playback-rate offset");
DECOMP_ASSERT(offsetof(CHARACTERANIM_s, root_translation) == 0x3c, "CHARACTERANIM_s root translation offset");
DECOMP_ASSERT(offsetof(CHARACTERANIM_s, misc_flags) == 0x48, "CHARACTERANIM_s misc-flags offset");

enum CHARACTER_ANIMATION_CONFIG_FLAGS : u32 {
    CHARACTER_ANIMATION_FLAG_CYCLING = 0x00000002,
    CHARACTER_ANIMATION_FLAG_DEFAULT = 0x00000004,
    CHARACTER_ANIMATION_FLAG_BSA = 0x00000008,
    CHARACTER_ANIMATION_FLAG_NO_HEAD_TURN = 0x00000040,
    CHARACTER_ANIMATION_FLAG_FOOTSTEPS = 0x00000100,
    CHARACTER_ANIMATION_FLAG_ALLOW_WEAPON_TRANSITION = 0x00000400,
};

struct CHARSCENE_s {
    nugscn_s *scene;
    nuhspecial_s special_scene;
};

DECOMP_ASSERT(sizeof(CHARSCENE_s) == 0x10, "CHARSCENE_s size");

struct GAMECHARACTERLAYER_s {
    char name[0x18];
    i16 mask_bit;
    i16 hierarchy_layer_index;
};

DECOMP_ASSERT(sizeof(GAMECHARACTERLAYER_s) == 0x1c, "GAMECHARACTERLAYER_s size");

enum GAMECHARACTER_FLAGS_090 : u32 {
    GAMECHARACTER_FLAG_GRAB_DISABLED = 0x8000,
    GAMECHARACTER_FLAG_CAN_SUPER_CARRY = 0x800000,
};

struct GAMECHARACTERDATA_s {
    MAKELAYERLISTFN make_layer_list; // 0x00
    GAMECHARACTERLAYER_s *layers;    // 0x04
    union {
        u32 field_0x08;
        i8 *layer_lookup;
    };
    f32 field_0x0c;
    f32 field_0x10;
    union {
        f32 field_0x14;
        f32 tiptoe_speed;
    };
    union {
        f32 field_0x18;
        f32 walk_speed;
    };
    union {
        f32 movement_speed;
        f32 run_speed; // 0x1c, ordinary directional top speed
    };
    union {
        f32 field_0x20;
        f32 backwards_speed_multiplier;
    };
    union {
        f32 field_0x24;
        f32 gravity;
    };
    f32 field_0x28;
    union {
        f32 field_0x2c;
        f32 jump_speed;
    };
    union {
        f32 field_0x30;
        f32 jump_duration;
    };
    union {
        f32 field_0x34;
        f32 jump_height;
    };
    union {
        f32 field_0x38;
        f32 second_jump_speed;
    };
    union {
        f32 field_0x3c;
        f32 second_jump_duration;
    };
    union {
        f32 field_0x40;
        f32 second_jump_height;
    };
    union {
        f32 field_0x44;
        f32 velocity_seek_rate;
    };
    f32 field_0x48;
    f32 field_0x4c;
    f32 field_0x50;
    f32 field_0x54;
    f32 field_0x58;
    f32 field_0x5c;
    f32 field_0x60;
    f32 viewdistance;  // 0x64
    f32 heardistance;  // 0x68
    f32 maxviewheight; // 0x6c
    f32 minviewheight; // 0x70
    union {
        f32 field_0x74;
        f32 turn_rate;
    };
    f32 field_0x78;
    f32 field_0x7c;
    f32 field_0x80;
    f32 field_0x84;
    f32 field_0x88;
    f32 field_0x8c;
    u32 flags_090;
    union {
        u32 field_0x94;
        u8 flags_094[4];
    };
    union {
        u32 field_0x98;
        u8 flags_098[4];
    };
    u32 layer_mask_special; // 0x9c
    u32 layer_mask;         // 0xa0, high-detail hierarchy render-part visibility mask
    u32 layer_mask_medium;  // 0xa4
    u32 layer_mask_low;     // 0xa8
    u32 layer_mask_dead;    // 0xac
    u32 ride_layers_off;    // 0xb0
    f32 field_0xb4;
    f32 field_0xb8;
    union {
        f32 field_0xbc;
        f32 ai_update_distance_0;
    };
    union {
        f32 field_0xc0;
        f32 ai_update_distance_1;
    };
    union {
        u32 field_0xc4;
        f32 ai_update_distance_2;
    };
    union {
        u32 field_0xc8;
        f32 ai_update_distance_3;
    };
    union {
        u32 field_0xcc;
        struct {
            u8 ai_update_interval_0;
            u8 ai_update_interval_1;
            u8 ai_update_interval_2;
            u8 ai_update_interval_3;
        };
    };
    union {
        struct {
            u32 field_0xd0;
            u32 field_0xd4;
            u32 field_0xd8;
        };
        i16 sfx_misc[6];
    };
    union {
        u32 field_0xdc;
        struct {
            i16 sfx_die;
            i16 sfx_hurt;
        };
    };
    union {
        u32 field_0xe0;
        struct {
            i16 sfx_grunt;
            i16 sfx_engine;
        };
    };
    union {
        u32 field_0xe4;
        struct {
            i16 sfx_shoot;
            i16 sfx_footstep;
        };
    };
    union {
        u32 field_0xe8;
        struct {
            i16 sfx_chatter;
            i16 sfx_sabre;
        };
    };
    union {
        u32 field_0xec;
        struct {
            i16 weapon_model;
            union {
                i16 field_0xee;
                u16 score;
            };
        };
    };
    union {
        u32 field_0xf0;
        struct {
            u16 shadow_locators;
            u16 thrust_locators;
        };
    };
    u8 hitpoints; // 0xf4
    u8 field_0xf5;
    u8 field_0xf6;
    u8 field_0xf7;
    union {
        u32 field_0xf8;
        i8 weapon_joints[4];
    };
    union {
        u32 field_0xfc;
        i8 weapon_shoot_joints[4];
    };
    union {
        struct {
            u32 field_0x100;
            u32 field_0x104;
        };
        i8 streak_joints[4][2];
    };
    union {
        u32 field_0x108;
        struct {
            i8 hand_locators[2];
            i8 grapple_locators[2];
        };
    };
    union {
        u32 field_0x10c;
        struct {
            i8 rocket_locator;
            i8 shield_locator;
            i8 head_locator;
            i8 collision_locator;
        };
    };
    union {
        u32 field_0x110;
        struct {
            i8 thingy_locator;
            i8 helmet_locator;
            i8 throw_locator;
            i8 ride_locator;
        };
    };
    union {
        u16 field_0x114;
        struct {
            i8 poo_locator;
            i8 hair_layer;
        };
    };
    union {
        u8 field275_0x116;
        u8 uses_weapon_action;
    };
    u8 field_0x117;
    union {
        u32 field_0x118;
        struct {
            i8 head_joint;
            i8 cloak_joint;
            i8 cloak_joint_2;
            i8 place_locator;
        };
    };
    union {
        u16 field_0x11c;
        struct {
            i8 cape_layer;
            i8 extra_character_locator;
        };
    };
    u8 field_0x11e;
    u8 layer_count; // 0x11f
};
typedef struct GAMECHARACTERDATA_s GAMECHARACTERDATA;

DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, layer_lookup) == 0x08, "GAMECHARACTER layer lookup offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, flags_094) == 0x94, "GAMECHARACTER packed flags offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, sfx_misc) == 0xd0, "GAMECHARACTER miscellaneous sounds offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, sfx_sabre) == 0xea, "GAMECHARACTER sabre sound offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, shadow_locators) == 0xf0, "GAMECHARACTER shadow locators offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, weapon_shoot_joints) == 0xfc, "GAMECHARACTER shooting locators offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, collision_locator) == 0x10f, "GAMECHARACTER collision locator offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, head_joint) == 0x118, "GAMECHARACTER head joint offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA_s, extra_character_locator) == 0x11d,
              "GAMECHARACTER attachment locator offset");

enum CHARACTER_MODEL_FLAGS : u32 {
    CHARACTER_MODEL_FLAG_CONFIGURED = 0x00000001,
    CHARACTER_MODEL_FLAG_JEDI_BADDIE = 0x00000004,
    CHARACTER_MODEL_FLAG_JEDI = 0x00000008,
    CHARACTER_MODEL_FLAG_HIGH_JUMP = 0x00000020,
    CHARACTER_MODEL_FLAG_ALTERNATE_WEAPON = 0x00000080,
    CHARACTER_MODEL_FLAG_DISABLE_BLOB_SHADOW = 0x00010000,
    CHARACTER_MODEL_FLAG_OLD_HEAD_MOVEMENT = 0x00020000,
};

enum GAMECHARACTER_FLAGS : u32 {
    GAMECHARACTER_FLAG_DISABLE_TIPTOE = 0x00000008,
    GAMECHARACTER_FLAG_ANIMATION_SPEED_FROM_VELOCITY = 0x00100000,
    GAMECHARACTER_FLAG_EXTENDED_JUMP_ANIMATIONS = 0x00400000,
};

DECOMP_ASSERT(sizeof(GAMECHARACTERDATA) == 0x120, "GAMECHARACTERDATA size");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, weapon_model) == 0xec, "GAMECHARACTERDATA weapon model offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, score) == 0xee, "GAMECHARACTERDATA score offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, weapon_joints) == 0xf8, "GAMECHARACTERDATA weapon joints offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, streak_joints) == 0x100, "GAMECHARACTERDATA streak joints offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, tiptoe_speed) == 0x14, "GAMECHARACTERDATA tiptoe speed offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, walk_speed) == 0x18, "GAMECHARACTERDATA walk speed offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, run_speed) == 0x1c, "GAMECHARACTERDATA run speed offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, ai_update_distance_0) == 0xbc, "GAMECHARACTERDATA AI update distance offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, ai_update_interval_0) == 0xcc, "GAMECHARACTERDATA AI update interval offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, gravity) == 0x24, "GAMECHARACTERDATA gravity offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, jump_speed) == 0x2c, "GAMECHARACTERDATA jump speed offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, second_jump_speed) == 0x38, "GAMECHARACTERDATA second jump speed offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, velocity_seek_rate) == 0x44, "GAMECHARACTERDATA velocity seek offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, turn_rate) == 0x74, "GAMECHARACTERDATA turn rate offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, layers) == 0x04, "GAMECHARACTERDATA layer-list offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, layer_mask_special) == 0x9c, "GAMECHARACTERDATA special layer-mask offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, layer_mask) == 0xa0, "GAMECHARACTERDATA layer-mask offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, layer_mask_dead) == 0xac, "GAMECHARACTERDATA dead layer-mask offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, hitpoints) == 0xf4, "GAMECHARACTERDATA hitpoints offset");
DECOMP_ASSERT(offsetof(GAMECHARACTERDATA, layer_count) == 0x11f, "GAMECHARACTERDATA layer-count offset");

struct PLAYERCHARACTERCONFIG_s {
    u8 unknown_00[0x30];
    f32 reset_scale;
    u8 unknown_34[0x4c - 0x34];
    f32 collision_origin_radius; // 0x4c; animated collision-origin sphere radius
    u8 unknown_50[0x8c - 0x50];
    f32 shadow_radius; // 0x8c; values below 99 override the character radius
    u32 flags_090;     // 0x90; bit 0x400 suppresses the offscreen Force glow
    u32 flags_094;
    u8 unknown_98[0xf0 - 0x98];
    u16 shadow_joint_mask; // 0xf0
    u8 unknown_f2[0xf6 - 0xf2];
    u8 blob_shadow_alpha; // 0xf6; 0xff selects the current level's alpha
    u8 unknown_f7;
    i8 weapon_joints[4];       // 0xf8
    i8 weapon_shoot_joints[4]; // 0xfc
    u8 unknown_100[8];
    i8 hand_joints[2];  // 0x108; Force-lightning origins
    i8 grapple_joint_a; // 0x10a; primary hand/rope attachment joint
    i8 grapple_joint_b; // 0x10b; optional secondary hand/rope attachment joint
    u8 unknown_10c;
    i8 collision_origin_joint; // 0x10d; alternate animated collision-origin joint
    u8 unknown_10e;
    i8 model_origin_joint; // 0x10f; primary animated model-origin joint
    u8 unknown_110[0x117 - 0x110];
    u8 variant;
};

DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, shadow_radius) == 0x8c, "PLAYERCHARACTERCONFIG shadow-radius offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, weapon_joints) == 0xf8, "PLAYERCHARACTERCONFIG weapon joints offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, weapon_shoot_joints) == 0xfc,
              "PLAYERCHARACTERCONFIG shooting joints offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, hand_joints) == 0x108, "PLAYERCHARACTERCONFIG hand joints offset");
DECOMP_ASSERT(sizeof(PLAYERCHARACTERCONFIG_s) == 0x118, "PLAYERCHARACTERCONFIG size");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, collision_origin_radius) == 0x4c,
              "PLAYERCHARACTERCONFIG collision-origin radius offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, shadow_joint_mask) == 0xf0,
              "PLAYERCHARACTERCONFIG shadow-joint-mask offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, blob_shadow_alpha) == 0xf6,
              "PLAYERCHARACTERCONFIG blob-shadow-alpha offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, grapple_joint_a) == 0x10a,
              "PLAYERCHARACTERCONFIG grapple joint A offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, grapple_joint_b) == 0x10b,
              "PLAYERCHARACTERCONFIG grapple joint B offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, collision_origin_joint) == 0x10d,
              "PLAYERCHARACTERCONFIG collision-origin joint offset");
DECOMP_ASSERT(offsetof(PLAYERCHARACTERCONFIG_s, model_origin_joint) == 0x10f,
              "PLAYERCHARACTERCONFIG model-origin joint offset");

struct characterdata_s { /* PlaceHolder Structure */
    union {
        i32 field0_0x0;
        i32 name_id;
    };
    u32 model_flags;
    char *dir;
    char *file;
    CHARACTERANIM_s *animations;
    undefined2 field5_0x14;
    undefined field6_0x16;
    undefined field7_0x17;
    CHARACTERUPDATEFN move_fn;
    CHARACTERUPDATEFN animate_fn;
    CHARACTERUPDATEFN draw_fn; // 0x20
    union {
        void *field11_0x24;
        PLAYERCHARACTERCONFIG_s *player_config;
        GAMECHARACTERDATA_s *game_character;
    };
    union {
        undefined4 field12_0x28;
        struct CHARACTER_EFFECT_s *effects;
    };
    f32 field13_0x2c;
    union {
        f32 field14_0x30;
        f32 collision_radius;
    };
    union {
        f32 field15_0x34;
        f32 bounds_min_y;
    };
    union {
        f32 field16_0x38;
        f32 bounds_max_y;
    };
    union {
        f32 field17_0x3c;
        f32 model_scale;
    };
    byte flags;
    undefined field19_0x41;
    i16 field20_0x42;
    union {
        u32 field21_0x44;
        u32 ai_path_capabilities;
    };
    undefined4 field22_0x48;
};
typedef struct characterdata_s CHARACTERDATA;

DECOMP_ASSERT(sizeof(CHARACTERDATA) == 0x4c, "CHARACTERDATA size");
DECOMP_ASSERT(offsetof(CHARACTERDATA, model_flags) == 0x04, "CHARACTERDATA model-flags offset");
DECOMP_ASSERT(offsetof(CHARACTERDATA, animations) == 0x10, "CHARACTERDATA animation-table offset");
DECOMP_ASSERT(offsetof(CHARACTERDATA, collision_radius) == 0x30, "CHARACTERDATA collision-radius offset");
DECOMP_ASSERT(offsetof(CHARACTERDATA, model_scale) == 0x3c, "CHARACTERDATA model-scale offset");

extern "C" i32 MakeLayerList_Index(CHARACTERMODEL_s *model, i16 *layers, u32 mask);
extern "C" void StoreLocatorCoordinates(CHARACTERMODEL_s *model, NUMTX *world_matrix, NUMTX *joint_matrices,
                                        NUVEC *positions, NUMTX *matrices);
extern "C" void APITransparentCharDraw(nuhgobj_s *object, NUMTX *world_matrix, i32 render_count, i16 *render_indices,
                                       NUMTX *joint_matrices, void **dwa, i32 render_flags);
extern "C" i32 APIDrawCharacterModel(CHARACTERMODEL_s *model, CHARACTERDATA *character_data, ANIMPACKET_s *animation,
                                     numtx_s *matrix, numtx_s *secondary_matrix, numtx_s *reflection_matrix,
                                     NUVEC *locator_positions, numtx_s *auxiliary_matrix, GameObject_s *object,
                                     u32 flags, NUJOINTANIM_s *joint_overrides, i32 joint_override_count,
                                     WORLDINFO_s *world, f32 far_clip, numtx_s *output_matrices, i32 value_15,
                                     void *level_model);

extern i32 CHARCOUNT;
extern CHARACTERDATA *CDataList;
extern GAMECHARACTERDATA *GCDataList;
extern GAMECHARACTERDATA GCDATA_DEFAULT;

#ifdef __cplusplus
extern "C" {
#endif
    extern CHARACTERNAMEFN *LevelCharacterNameFn;
    extern CHARACTERNAMEFN *SpecialRouteCharacterNameFn;
    extern CHARACTERGLOBALIDFN *LevelCharacterGlobalIDFn;
    extern GLOBALCHARACTERNAMEFN *GlobalCharacterNameFn;
    extern CHARACTERHGOBJFN *GlobalCharacterHGobjFn;
    extern CHARACTERRENDERFN *GlobalCharacterRenderFn;
    extern CHARACTERGOALSPEEDFN *GetCharacterGoalSpeedFn;
    extern CHARACTERTYPEIDFN *LevelCharacterTypeIDFn;

    void InitFn_GetCharacterGoalSpeedFn(CHARACTERGOALSPEEDFN *function);
    void InitFn_GetHearDistance(CHARACTERDISTANCEFN *function);
    void InitFn_GetViewRange(CHARACTERDISTANCEFN *function);
    void InitFn_GlobalCharacterHGobj(CHARACTERHGOBJFN *function);
    void InitFn_GlobalCharacterName(GLOBALCHARACTERNAMEFN *function);
    void InitFn_GlobalCharacterRender(CHARACTERRENDERFN *function);
    void InitFn_GlobalCharacterTypeID(CHARACTERTYPEIDFN *function);
    void InitFn_GlobalGetMaxViewHeight(CHARACTERDISTANCEFN *function);
    void InitFn_GlobalGetMinViewHeight(CHARACTERDISTANCEFN *function);
    void InitFn_LevelCharacterGlobalID(CHARACTERGLOBALIDFN *function);
    void InitFn_LevelCharacterName(CHARACTERNAMEFN *function);
    void InitFn_LevelCharacterTypeID(CHARACTERTYPEIDFN *function);
    void InitFn_SpecialRouteCharacterName(CHARACTERNAMEFN *function);
    void InitFn_SpecialRouteCharacterTypeID(CHARACTERTYPEIDFN *function);

    void APITransparentInit();
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

i32 CharIDFromName(char *name);

extern "C" {
#endif
    extern i32 g_loadingCharacterInHub;
    extern i32 hub_character_ready;
    extern u8 Hub_LowEnd_IconsInsteadOfModels;

    CHARACTERDATA *ConfigureCharacterList(char *file, VARIPTR *bufferStart, VARIPTR *bufferEnd, i32 count,
                                          i32 *countDest, i32 count2, GAMECHARACTERDATA **dest);
#ifdef __cplusplus
}
#endif

// Model id globals (assigned from the character name table in character.cpp)
extern "C" {
    extern i16 id_KAADU;
    extern i16 id_GUNGAN;
    extern i16 id_FALUMPASET;
    extern i16 id_DARTHMAUL;
    extern i16 id_ANAKINSPOD;
    extern i16 id_STAP;
    extern i16 id_SPEEDERBIKE;
    extern i16 id_BATTLEDROID;
    extern i16 id_ROYALGUARD;
    extern i16 id_CLONEARC;
    extern i16 id_IMPERIALSHUTTLE;
    extern i16 id_NABOOSTARFIGHTER;
    extern i16 id_XWING;
    extern i16 id_SNOWSPEEDER;
    extern i16 id_MILLENNIUMFALCON;
    extern i16 id_NEW_REPUBLIC_GUNSHIP;
    extern i16 id_COUNTDOOKU;
    extern i16 id_OBIWANKENOBIJEDIMASTER;
    extern i16 id_GRIEVOUS;
    extern i16 id_THEEMPEROR;
    extern i16 id_SERVICECAR;
    extern i16 id_JANGOFETT;
    extern i16 id_MOSEISLEYCITIZEN;
    extern i16 id_CANTINAALIEN;
    extern i16 id_BARMAN;
    extern i16 id_TRAININGREMOTE;
    extern i16 id_CANTINABAND;
    extern i16 id_WEIRDO1;
    extern i16 id_WEIRDO2;
    extern i16 id_JABBA;
    extern i16 id_WOMPRAT;
    extern i16 id_DRAGBOMB;
    extern i16 id_DROIDEKA;
    extern i16 id_CLOUDCITYCITIZEN;
    extern i16 id_GEONOSIAN;
    extern i16 id_BOB;
    extern i16 id_WHIP;
    extern i16 id_JARJAR;
    extern i16 id_PADMECLAWED;
    extern i16 id_SHAAKTI;
    extern i16 id_LUMINARA;
    extern i16 id_SUPERBATTLEDROID;
    extern i16 id_BATTLEDROIDSECURITY;
    extern i16 id_BATTLEDROIDGEONOSIAN;
}

void LoadSingleCharacter(bgprocinfo_s *info);
void UpdateCharacterLoad(void);
i32 NewPlayerCharacter(GameObject_s *object, i32 new_id, i32 old_id, i32 mode);
