#pragma once

#include "legoapi/gizmos/trigger/ai.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuhspecial.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/common.h"

struct GIZFORCESYS_s;
struct GIZOBSTACLESYS_s;
struct GIZBUILDITSYS_s;
struct GIZMOPICKUPRUNTIMESYS_s;
struct FADER_s;
struct RIPPLEEFFECT_s;
struct SPECIALMINIKITSYS_s;
struct TIGHTROPE_s;
struct GUIDELINE_s;
struct SECURITYDOOR_s;

struct AREADATA_s;
struct LEVEL_PROGRESS_s;
struct APIDEBRISSYS_s;
struct PARTDEBSYS_s;
struct GAMEANIMSYS_s;
struct SOCKSYS;
struct APIOBJECTSYS_s;
struct burnset_s;
struct rtlset;
struct CUTSYS;
struct GIZMOSYS_s;
struct GIZFLOW_s;
struct CHARPLATFORMSYS_s;
struct AISYS_s;
struct AIPATHCNXCONTROLSYS_s;
struct AIPATHCNXHELPERSYS_s;
struct AITRIGGERSETSYS_s;
struct CLIMBOBJECTSYS_s;
struct MechAutoJumpManager;
struct DOOR_s;
struct TELEPORT_s;
struct PORTALDOOR_s;
struct GIZOBSTACLESYS_s;
struct GIZTURRETSYS_s;
struct GIZSPINNER_s;
struct pushblock_s;
struct ZIPUP_s;
struct GIZMOBLOWUPTYPE_s;
struct GIZMOBLOWUP_s;
struct GIZRANDOMSYS_s;
struct EDGIZSHADOW_s;
struct GAMEANIMOBJPOOL_s;
struct GRABBER_s;
struct PULSESYS_s;
struct pushblock_s;
struct LEVER_s;
struct GIZPANELSYS_s;
struct TECHNO_s;
struct HATMACHINESYS_s;
struct SIGNAL_s;
struct SECURITYDOOR_s;
struct GIZRANDOMSYS_s;
struct GIZSPECIALSYS_s;
struct GAMEANTINODESYS_s;
struct GIZBOMBGENSYS_s;
struct GIZTORPMACHINESYS_s;
struct GIZSPECIALSYS_s;
struct TRAFFICANIMSYS_s;
struct PLUGSYS_s;
struct GIZTIMER_s;
struct MINICUT_s;
struct MINICUTPART_s;
struct TIMER_s;
struct LEVEL_OBJECT_RUNTIME_s;
struct spacelevel_s;
struct CHARSCENE_s;

struct BOLTTYPE_s {
    char name[16];
    f32 field_10;
    f32 field_14;
    f32 field_18;
    f32 field_1c;
    f32 field_20;
    f32 field_24;
    i32 field_28;
    union {
        i32 field_2c;
        struct {
            i16 field_2c_lo;
            i16 debris_id;
        };
    };
    union {
        i32 field_30;
        i16 moving_debris[2];
    };
    union {
        i32 field_34;
        i16 moving_debris_counts[2];
    };
    i16 field_38;
    i16 field_3a;
    u8 field_3c;
    u8 field_3d;
    u16 field_3e;
    u32 field_40;
    void (*init_callback)(BOLT_s *);              // 0x44
    void (*update_callback)(BOLT_s *);            // 0x48
    void (*end_callback)(BOLT_s *);               // 0x4c
    void (*ricochet_callback)(BOLT_s *, NUVEC *); // 0x50
    f32 (*scale_callback)(BOLT_s *);              // 0x54
    char *shoot_sfx;
    char *hit_sfx;
    u32 field_60;
    i16 shoot_sfx_id;
    i16 hit_sfx_id;
    union {
        u8 pad_68[0x3c];
        struct {
            nuhspecial_s object_special;
            nuhspecial_s glow_special;
            nuhspecial_s reference_object_special;
            nuhspecial_s reference_glow_special;
            nuhspecial_s shadow_special;
        } specials;
    };
};
DECOMP_ASSERT(sizeof(BOLTTYPE_s) == 0xa4, "BOLTTYPE_s size");
DECOMP_ASSERT(offsetof(BOLTTYPE_s, end_callback) == 0x4c, "BOLTTYPE end callback offset");
DECOMP_ASSERT(offsetof(BOLTTYPE_s, hit_sfx) == 0x5c, "BOLTTYPE hit sound offset");
DECOMP_ASSERT(offsetof(BOLTTYPE_s, hit_sfx_id) == 0x66, "BOLTTYPE hit sound id offset");

struct LEVELSFXENTRY_s {
    NUVEC position;
    i16 id;
    i8 references;
    u8 reserved_0f;
};
DECOMP_ASSERT(sizeof(LEVELSFXENTRY_s) == 0x10, "LEVELSFXENTRY_s size");

struct SPECIALSFXEVENT_s {
    f32 previous_frame;
    u8 flags;
    u8 reserved_05;
    i16 sfx_id;
    f32 trigger_frame;
    SPECIALSFXEVENT_s *next;
};
DECOMP_ASSERT(sizeof(SPECIALSFXEVENT_s) == 0x10, "SPECIALSFXEVENT_s size");

struct specialsfx_s {
    u8 flags;
    u8 reserved_01;
    i8 event_count;
    i8 animation_playing;
    nuhspecial_s special;
    SPECIALSFXEVENT_s *events;
};
DECOMP_ASSERT(sizeof(specialsfx_s) == 0x14, "specialsfx_s size");

// A portal-position spline stores camera/player points as position/target
// pairs (six floats per point).  MoveGameCamera uses portal_places[2] for
// the title-screen camera.
typedef struct portalpos_s {
    i16 count;
    i16 pad_02;
    f32 *weights;
    f32 *positions;
} PORTALPOS;

struct HUBMINIKITPIECE_s {
    nuhspecial_s special;
    NUMTX matrix;
    u8 reserved_0x4c[0x66 - 0x4c];
    u8 enabled;
    u8 reserved_0x67;
};
DECOMP_ASSERT(sizeof(HUBMINIKITPIECE_s) == 0x68, "Hub minikit piece size");
DECOMP_ASSERT(offsetof(HUBMINIKITPIECE_s, matrix) == 0xc, "Hub minikit piece matrix offset");
DECOMP_ASSERT(offsetof(HUBMINIKITPIECE_s, enabled) == 0x66, "Hub minikit piece enabled offset");
struct HUBMINIKITPIECES_s {
    u32 unknown_00;
    HUBMINIKITPIECE_s *pieces;
    u8 piece_count;
    u8 reserved_09[3];
    HUBMINIKITPIECE_s base;
};
DECOMP_ASSERT(sizeof(HUBMINIKITPIECES_s) == 0x74, "Hub minikit pieces size");
DECOMP_ASSERT(offsetof(HUBMINIKITPIECES_s, base) == 0xc, "Hub minikit base offset");
struct HUBMINIKIT_s {
    NUVEC position;
    f32 scale;
    f32 field_0x10;
    i32 phase_x, rate_x;
    i32 phase_y, rate_y;
    i32 phase_z, rate_z;
    i32 phase_rotation, rate_rotation;
    u16 rotation, rotation_velocity, tilt, target_rotation;
    u8 displayed_piece_count, target_piece_count;
    u8 reserved_0x3e[2];
    NUVEC collision_center, bounds_min, bounds_max;
    f32 radius, height_ratio;
    f32 offset_x, offset_z, velocity_x, velocity_z;
};
DECOMP_ASSERT(sizeof(HUBMINIKIT_s) == 0x7c, "Hub minikit runtime size");
DECOMP_ASSERT(offsetof(HUBMINIKIT_s, displayed_piece_count) == 0x3c, "Hub displayed piece count offset");
DECOMP_ASSERT(offsetof(HUBMINIKIT_s, collision_center) == 0x40, "Hub minikit collision center offset");
DECOMP_ASSERT(offsetof(HUBMINIKIT_s, radius) == 0x64, "Hub minikit radius offset");

typedef struct MINIKIT {
    void *gscn;
    char filler[0x14];
    struct CHARSCENE_s *character_scenes;
} MINIKIT;
DECOMP_ASSERT(offsetof(MINIKIT, character_scenes) == 0x18, "MINIKIT character scenes offset");
DECOMP_ASSERT(sizeof(MINIKIT) == 0x1c, "MINIKIT size");

typedef struct PORTALDOOR_s {
    nuhspecial_s special;
    u16 flags;
    u8 portal_id;
    u8 pad_0f;
} PORTALDOOR;

DECOMP_ASSERT(sizeof(PORTALDOOR) == 0x10, "PORTALDOOR size");

struct LEVEL_PROGRESS_DATA_s {
    u32 words[0xa00];
};
DECOMP_ASSERT(sizeof(LEVEL_PROGRESS_DATA_s) == 0x2800, "Level progress data size");

// Layout matches the original WORLDINFO_s (0x51b0 = 20912 bytes).
// Field offsets verified against the original binary disassembly.
typedef struct WORLDINFO_s {
    char name[0x80]; // 0x000  level name / path scratch (Reset builds "levels\\..." here)

    char config_file[0x80]; // 0x080  level config path/name
    void *buffer_start;     // 0x100  start of this world's bump buffer (saved across Reset)

    VARIPTR giz_buffer;   // 0x104
    VARIPTR unknown_0108; // 0x108  end of the bump buffer
    i32 config_count;     // 0x10c  level config keyword count / gate

    i32 loaded; // 0x110  set to 1 at end of WorldInfo_Load

    i32 field_0x114;
    u32 reset_flags; // 0x118

    i32 level_idx;    // 0x11c
    i32 level_sub_id; // 0x120  from LEVELDATA.unknown_0af
    i32 area_sub_id;  // 0x124  from AREADATA.episode_index

    struct LEVELDATA_s *current_level; // 0x128
    AREADATA_s *area;                  // 0x12c
    LEVEL_PROGRESS_s *level_progress;  // 0x130
    APIDEBRISSYS_s *debris_sys;        // 0x134
    PARTDEBSYS_s *part_debris_sys;     // 0x138
    NUGSCN *current_gscn;              // 0x13c
    NUGSCN *scene;                     // 0x140

    f32 level_min[3];                    // 0x144  CalculateWorldSize output
    f32 level_max[3];                    // 0x150
    LEVEL_PROGRESS_DATA_s progress_data; // 0x15c .. 0x295c  level progress store

    void *terrain; // 0x295c  LoadTerrainFile result

    GAMEANIMSYS_s *game_anim_sys;            // 0x2960
    NUGSCN *icons_gscn;                      // 0x2964
    MINIKIT minikit;                         // 0x2968
    HUBMINIKITPIECES_s **minikit_pieces_buf; // 0x2984
    struct SOCKSYS *sock_sys;                // 0x2988
    APIOBJECTSYS_s *api_object_sys;          // 0x298c

    u8 room_visibility_flag; // 0x2990
    u8 rooms_visible[0x100]; // 0x2991 .. 0x2a91
    u8 *rooms_visible_ptr;   // 0x2a94

    i32 page_pp;                      // 0x2a98  edpp page handle
    i32 page_part;                    // 0x2a9c  edpart page handle
    i32 page_anim;                    // 0x2aa0  edanim page handle
    i32 page_grass;                   // 0x2aa4  edgra page handle
    i32 page_bridge;                  // 0x2aa8  edbri page handle
    burnset_s *burnset;               // 0x2aac
    CUTSYS *cutscene_sys;             // 0x2ab0
    rtlset *rtl_set;                  // 0x2ab4  level real-time-light set
    i32 rtl_id;                       // 0x2ab8  rtlFindByUserId result
    void *light_dir;                  // 0x2abc  rtlGetDirection out-pointer
    LEVEL_OBJECT_RUNTIME_s *lev_objs; // 0x2ac0  level-object array
    union {
        struct portalpos_s **portal_places;
        struct nugspline_s **camera_splines;
    }; // 0x2ac4

    GIZMOSYS_s *gizmo_sys; // 0x2ac8
    GIZFLOW_s *giz_flow;   // 0x2acc

    char filler4[0x2adc - 0x2ad0]; // 0x2ad0 .. 0x2adc

    CHARPLATFORMSYS_s *char_platform_sys; // 0x2adc

    HUBMINIKIT_s *hub_minikits; // 0x2ae0
    i32 ai_loaded;              // 0x2ae4
    AISYS_s *ai_sys;            // 0x2ae8
    i32 processor_count;
    LEVELSCRIPTPROCESS processors[32];

    AIPATHCNXCONTROLSYS_s *ai_path_cnx_control_sys;
    AIPATHCNXHELPERSYS_s *ai_path_cnx_helper_sys;
    AITRIGGERSETSYS_s *ai_trigger_set_sys;
    CLIMBOBJECTSYS_s *climb_object_sys;
    MechAutoJumpManager *mech_auto_jump_manager;

    TELEPORT_s *teleports;                 // 0x4684
    i32 teleport_count;                    // 0x4688
    struct ZIPUP_s *zipups;                // 0x468c
    i32 zipup_count;                       // 0x4690
    struct TUBE_s *tubes;                  // 0x4694
    i32 tube_count;                        // 0x4698
    DOOR_s *doors;                         // 0x469c
    i32 door_count;                        // 0x46a0
    DOOR_s *start_door;                    // 0x46a4
    GIZOBSTACLESYS_s *giz_obstacle_sys;    // 0x46a8
    GIZBUILDITSYS_s *giz_buildit_sys;      // 0x46ac
    GIZFORCESYS_s *giz_force_sys;          // 0x46b0
    char filler7a[0x46bc - 0x46b4];        // 0x46b4 .. 0x46bc
    GIZTURRETSYS_s *giz_turret_sys;        // 0x46bc
    pushblock_s *push_blocks;              // 0x46c0
    i32 push_block_count;                  // 0x46c4
    char filler7b[0x46d0 - 0x46c8];        // 0x46c8 .. 0x46d0
    i32 special_sfx_count;                 // 0x46d0
    specialsfx_s *special_sfx;             // 0x46d4
    i32 special_sfx_event_count;           // 0x46d8
    SPECIALSFXEVENT_s *special_sfx_events; // 0x46dc
    char filler7c[0x46e4 - 0x46e0];        // 0x46e0 .. 0x46e4
    GIZSPINNER_s *spinners;                // 0x46e4
    i32 spinner_count;                     // 0x46e8
    GAMEANIMOBJPOOL_s *spinner_anim_pool;  // 0x46ec

    GRABBER_s *grabber; // 0x46f0

    union {
        struct {
            union {
                u8 reserved_46f4[0x4714 - 0x46f4];
                struct {
                    u32 field_46f4;
                    RIPPLEEFFECT_s *ripple_effects;
                    union {
                        u32 field_46fc;
                        i32 ripple_effect_count;
                    };
                    union {
                        struct {
                            i32 water_ripple_effect;
                            i32 sabre_ripple_effect;
                        };
                        i32 ripple_effect_indices[2];
                    };
                    u8 reserved_4708[0xc];
                };
            };
            LEVELSFXENTRY_s level_sfx[64]; // 0x4714
            u8 reserved_4b14[0x5038 - 0x4b14];
        };
        struct {
            u8 reserved_to_4b14[0x4b14 - 0x46f4];
            i32 level_sfx_count;      // 0x4b14
            BOLTTYPE_s bolt_types[8]; // 0x4b18
        };
    };
    FADER_s *faders;            // 0x5038
    i32 fader_count;            // 0x503c
    void *mini_trooper_packets; // 0x5040
    void *mini_trooper_teams;   // 0x5044
    void *mini_trooper_storage; // 0x5048

    PORTALDOOR *portal_doors; // 0x504c
    i32 portal_door_count;    // 0x5050

    PULSESYS_s *pulses_sys; // 0x5054

    SPECIALMINIKITSYS_s *special_minikits; // 0x5058
    TIGHTROPE_s *tightropes;               // 0x505c
    i32 tightrope_count;                   // 0x5060
    SIGNAL_s *signals;                     // 0x5064
    i32 signal_count;                      // 0x5068

    LEVER_s *levers; // 0x506c
    i32 nlevers;     // 0x5070

    GIZPANELSYS_s *giz_panel_sys;     // 0x5074
    HATMACHINESYS_s *hat_machine_sys; // 0x5078
    TECHNO_s *technos;                // 0x507c
    i32 ntechnos;                     // 0x5080
    struct GRAPPLE_s *grapples;       // 0x5084
    i32 grapple_count;                // 0x5088
    u8 reserved_508c[0x8];            // 0x508c .. 0x5094
    void *shards;                     // 0x5094
    i32 shard_count;                  // 0x5098
    void *attractos;                  // 0x509c
    i32 attracto_count;               // 0x50a0
    GUIDELINE_s *guidelines;          // 0x50a4
    i32 guideline_count;              // 0x50a8
    void *ledges;                     // 0x50ac
    i32 ledge_count;                  // 0x50b0
    SECURITYDOOR_s *security_doors;   // 0x50b4
    i32 security_door_count;          // 0x50b8
    union {
        GIZMOPICKUPRUNTIMESYS_s *gizmo_pickup_sys;
        GIZMOPICKUPRUNTIMESYS_s *pickup_sys;
    }; // 0x50bc

    i32 gizmo_blowup_type_count;           // 0x50c0
    i32 gizmo_blowup_count;                // 0x50c4
    GIZMOBLOWUPTYPE_s *gizmo_blowup_types; // 0x50c8
    GIZMOBLOWUP_s *gizmo_blowups;          // 0x50cc

    union {
        u32 field_0x50d0;
        i32 field_50d0;
    };
    GIZMOBLOWUP_s **blowup_target_candidates; // 0x50d4

    GIZRANDOMSYS_s *giz_randoms;

    char filler12a[0x50ec - 0x50dc];
    NUMTX *gizmo_blowup_mtx_buffer;            // 0x50ec, aligned 0x8000-byte animation matrix arena
    MINICUT_s *minicuts;                       // 0x50f0
    MINICUTPART_s *minicut_parts;              // 0x50f4
    i32 minicut_count;                         // 0x50f8
    struct GIZTIMER_s *giz_timers;             // 0x50fc
    i32 giz_timers_count;                      // 0x5100
    GIZTORPMACHINESYS_s *giz_torp_machine_sys; // 0x5104
    char filler13b[0x510c - 0x5108];
    GIZSPECIALSYS_s *giz_special_sys;     // 0x510c
    GAMEANTINODESYS_s *game_antinode_sys; // 0x5110
    EDGIZSHADOW_s *shadow_editor;         // 0x5114
    GIZBOMBGENSYS_s *giz_bombgen_sys;     // 0x5118
    char filler14d[0x5120 - 0x511c];

    union {
        void *level_specific_data;
        void *podrace;
        spacelevel_s *space_level;
    }; // 0x5120

    union {
        char filler14b[0x516c - 0x5124];
        struct {
            struct CUSTOMPIECERESOURCE *customiser_resources[9];
            u8 customiser_resource_tail[0x24];
        };
    };

    TRAFFICANIMSYS_s *trafficanim_sys; // 0x516c
    PLUGSYS_s *plug_sys;               // 0x5170

    union {
        i32 field_5174;
        i32 field_0x5174;
        u32 progress_flag_5174;
    };
    char filler15[0x51b0 - 0x5178];
} WORLDINFO;
DECOMP_ASSERT(offsetof(WORLDINFO, field_0x5174) == 0x5174, "WORLDINFO saved level state offset");

DECOMP_ASSERT(offsetof(WORLDINFO, camera_splines) == 0x2ac4, "WORLDINFO camera splines offset");
DECOMP_ASSERT(offsetof(WORLDINFO, field_50d0) == 0x50d0, "WORLDINFO reset field offset");
DECOMP_ASSERT(offsetof(WORLDINFO, field_5174) == 0x5174, "WORLDINFO saved reset field offset");

DECOMP_ASSERT(offsetof(WORLDINFO, gizmo_blowup_type_count) == 0x50c0, "WORLDINFO blowup type count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, gizmo_blowup_types) == 0x50c8, "WORLDINFO blowup types offset");
DECOMP_ASSERT(offsetof(WORLDINFO, portal_doors) == 0x504c, "WORLDINFO portal-door array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, portal_door_count) == 0x5050, "WORLDINFO portal-door count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, pulses_sys) == 0x5054, "WORLDINFO pulse system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, level_sfx) == 0x4714, "WORLDINFO level SFX array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, ripple_effects) == 0x46f8, "WORLDINFO ripple effects offset");
DECOMP_ASSERT(offsetof(WORLDINFO, water_ripple_effect) == 0x4700, "WORLDINFO water ripple offset");
DECOMP_ASSERT(offsetof(WORLDINFO, sabre_ripple_effect) == 0x4704, "WORLDINFO sabre ripple offset");
DECOMP_ASSERT(offsetof(WORLDINFO, level_sfx_count) == 0x4b14, "WORLDINFO level SFX count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, bolt_types) == 0x4b18, "WORLDINFO bolt types offset");
DECOMP_ASSERT(offsetof(WORLDINFO, faders) == 0x5038, "WORLDINFO fader array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, special_minikits) == 0x5058, "WORLDINFO special minikit system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, signals) == 0x5064, "WORLDINFO signal array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, guidelines) == 0x50a4, "WORLDINFO guideline array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, levers) == 0x506c, "WORLDINFO lever array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, nlevers) == 0x5070, "WORLDINFO lever count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, giz_panel_sys) == 0x5074, "WORLDINFO giz-panel system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, hat_machine_sys) == 0x5078, "WORLDINFO hat-machine system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, technos) == 0x507c, "WORLDINFO techno array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, ntechnos) == 0x5080, "WORLDINFO techno count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, tubes) == 0x4694, "WORLDINFO tube array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, tube_count) == 0x4698, "WORLDINFO tube count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, teleports) == 0x4684, "WORLDINFO teleport array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, teleport_count) == 0x4688, "WORLDINFO teleport count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, zipups) == 0x468c, "WORLDINFO zip-up array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, zipup_count) == 0x4690, "WORLDINFO zip-up count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, grapples) == 0x5084, "WORLDINFO grapple array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, grapple_count) == 0x5088, "WORLDINFO grapple count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, plug_sys) == 0x5170, "WORLDINFO plug-system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, game_antinode_sys) == 0x5110, "WORLDINFO game-antinode system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, gizmo_blowups) == 0x50cc, "WORLDINFO blowup array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, minicuts) == 0x50f0, "WORLDINFO minicut array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, minicut_parts) == 0x50f4, "WORLDINFO minicut-part array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, minicut_count) == 0x50f8, "WORLDINFO minicut count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, giz_timers) == 0x50fc, "WORLDINFO gizmo-timer array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, giz_timers_count) == 0x5100, "WORLDINFO gizmo-timer count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, spinners) == 0x46e4, "WORLDINFO spinner array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, spinner_count) == 0x46e8, "WORLDINFO spinner count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, spinner_anim_pool) == 0x46ec, "WORLDINFO spinner animation pool offset");
DECOMP_ASSERT(offsetof(WORLDINFO, push_blocks) == 0x46c0, "WORLDINFO push-block array offset");
DECOMP_ASSERT(offsetof(WORLDINFO, push_block_count) == 0x46c4, "WORLDINFO push-block count offset");
DECOMP_ASSERT(offsetof(WORLDINFO, gizmo_pickup_sys) == 0x50bc, "WORLDINFO pickup system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, giz_torp_machine_sys) == 0x5104, "WORLDINFO torpedo-machine system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, giz_bombgen_sys) == 0x5118, "WORLDINFO bomb-generator system offset");
DECOMP_ASSERT(offsetof(WORLDINFO, current_gscn) == 0x13c, "WORLDINFO current scene offset");
DECOMP_ASSERT(sizeof(WORLDINFO) == 0x51b0, "WORLDINFO ABI");

extern void (*WorldInfo_InitMenuFn)(WORLDINFO *, i32 *, i32 *);
extern void (*WorldInfo_InitLastFn)(WORLDINFO *);
extern TIMER_s LevelTimer;

#ifdef __cplusplus
extern "C" {
#endif
    extern WORLDINFO WorldInfo[];

    /// @brief Global pointer to the currently active world info
    extern WORLDINFO *WORLD;

    extern f32 g_BgLoadDelayHackTimer;

    extern i32 waiting_for_level;
    extern i32 level_already_loaded;
    extern i32 next_level;
#ifdef __cplusplus
}

void WorldInfo_InitOnce(void);

void WorldInfo_Init(WORLDINFO *world);

void WorldInfo_Load(WORLDINFO *world);
i32 WorldInfo_Reset(WORLDINFO *world, i32 level_idx);

void WorldInfo_Activate(void);
void WorldInfo_StreamLevel(BGPROCINFO *bg_info);

WORLDINFO *WorldInfo_CurrentlyActive(void);
WORLDINFO *WorldInfo_CurrentlyLoading(void);

i32 WorldInfo_OtherLevel(WORLDINFO *world);

void WorldInfo_Register(WORLDINFO *world);

void WorldInfo_ClearAllIfScreenFaded(void);

void WorldInfo_LoadObjectAnimFile(WORLDINFO *world);

void WorldInfo_DrawScene(WORLDINFO *world);

void WorldInfo_UpdateRoomVisibility(WORLDINFO *world, i32 param);
void WorldInfo_ReArrangeBuffers(i32 area1, i32 area2);

#endif
