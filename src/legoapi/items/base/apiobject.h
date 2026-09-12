#pragma once

#include "decomp.h"
#include "legoapi/render/fx/edsplines.h"

struct GIZMOBLOWUP_s;
struct SNAKEBODY_s;
struct PART_s;
struct BOLT_s;
#include "gameapi/ai/aisys/aipath.h"
#include "gameapi/ai/aisys/aiscript_types.h"
#include "legoapi/items/base/animpacket.h"
#include "legoapi/render/fx/spline_position.h"
#include "legoapi/props/system/socksys.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuhspecial.h"
#include "nu2api/numath/numtx.h"

struct GameObject_s;
struct GizForceLOSState_s {
    u32 words[396]; // 12 visibility bitset words followed by 384 update counters
};
DECOMP_ASSERT(sizeof(GizForceLOSState_s) == 0x630, "Force LOS cache ABI");
struct MechObjectInterface;
struct MechAddonCollection;
struct GAMEANIMOBJ_s;
struct GIZFORCE_s;
struct CABLE_s;
struct characterdata_s;
struct CHARACTERANIM_s;
struct AIAREA_s;
struct AILOCATOR_s;
struct AILOCATORSET_s;
struct AIGROUP_s;
struct AIPATHNODE_s;
struct AISCRIPTPROCESS_s;
struct APIOBJECT_s;

enum CHARACTER_CONTEXT : i8 {
    CHARACTER_CONTEXT_JUMP = 0,
    CHARACTER_CONTEXT_LAND_JUMP = 1,
    CHARACTER_CONTEXT_LAND_JUMP_2 = 2,
    CHARACTER_CONTEXT_LAND_FLIP = 3,
    CHARACTER_CONTEXT_LAND_COMBO_JUMP = 4,
    CHARACTER_CONTEXT_COMBO = 5,
    CHARACTER_CONTEXT_FORCE = 0x08,
    CHARACTER_CONTEXT_FORCE_THROW = 0x12,
    CHARACTER_CONTEXT_FORCE_PUSH = 0x1b,
    CHARACTER_CONTEXT_FORCE_DEFLECT = 0x1d,
    CHARACTER_CONTEXT_DROP_IN = 0x23,
    CHARACTER_CONTEXT_DROP_OUT = 0x24,
    CHARACTER_CONTEXT_DOOMED = 0x2b,
    CHARACTER_CONTEXT_BUILD_IT = 0x2d,
    CHARACTER_CONTEXT_LINKED_OBJECT = 0x3b,
    CHARACTER_CONTEXT_NONE = -1,
};

enum APIOBJECT_FLAGS {
    APIOBJECT_FLAG_IN_USE = 0x0001,
    APIOBJECT_FLAG_GROUNDED = 0x0004,
    APIOBJECT_FLAG_SHADOW_AT_OBJECT_HEIGHT = 0x0010,
    APIOBJECT_FLAG_PLAYER_ACTIVE = 0x0080,
    // Set for every live character object, including AI creatures.  Player
    // ownership is represented separately by Player[]/field_0x287.
    APIOBJECT_FLAG_CHARACTER = 0x1000,
    APIOBJECT_FLAG_PLAYER_CHARACTER = APIOBJECT_FLAG_CHARACTER,
    APIOBJECT_FLAG_RESPAWN_ENABLED = 0x2000,
    APIOBJECT_FLAG_AI_PLAYER_MASK = 0x0180,
};

enum APIOBJECT_HIGH_FLAGS : u8 {
    APIOBJECT_HIGH_FLAG_CHARACTER = APIOBJECT_FLAG_CHARACTER >> 8,
    APIOBJECT_HIGH_FLAG_PLAYER_CHARACTER = APIOBJECT_HIGH_FLAG_CHARACTER,
    APIOBJECT_HIGH_FLAG_RESPAWN_ENABLED = APIOBJECT_FLAG_RESPAWN_ENABLED >> 8,
};

enum AI_RESPAWN_FLAGS : u8 {
    AI_RESPAWN_FLAG_DISABLED = 0x01,
};

enum APIOBJECT_MOTION_FLAGS {
    APIOBJECT_MOTION_FLAG_AI_CONTROLLED = 0x0400,
};

enum APIOBJECT_STATE_FLAGS : u32 {
    // Doors_Check tests byte +0x1f6 bit 2, which is bit 18 of the runtime
    // state word at +0x1f4.  It is not part of the u16 object flags at +0x1f8.
    APIOBJECT_STATE_FLAG_IGNORE_DOORS = 0x00040000,
};

enum APIOBJECT_TERRAIN_CONTACT_FLAGS : u8 {
    APIOBJECT_TERRAIN_CONTACT_FLOOR = 0x01,
    APIOBJECT_TERRAIN_CONTACT_NEAR_FLOOR = 0x02,
};

enum GAMEOBJECT_F03_FLAGS : u8 {
    // Allows the object to satisfy an obstacle's terrain/platform trigger
    // even when APIOBJECT::field_0x27d is clear.
    GAMEOBJECT_F03_FLAG_OBSTACLE_TERRAIN_VALID = 0x20,
};

enum GAMEOBJECT_E22_FLAGS : u8 {
    GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION = 0x01,
    GAMEOBJECT_E22_FLAG_INPUT_ANGLE_VALID = 0x20,
};

enum GAMEOBJECT_E23_FLAGS : u8 {
    GAMEOBJECT_E23_FLAG_FORCE_WEAPON_IDLE = 0x01,
};

enum GAMEOBJECT_E24_FLAGS : u8 {
    // The render evaluation has refreshed joint_matrices for model-origin
    // collision and attachment queries.
    GAMEOBJECT_E24_FLAG_JOINT_MATRICES_UPDATED = 0x08,
};

enum GAMEOBJECT_E20_FLAGS : u8 {
    GAMEOBJECT_E20_FLAG_MOVEMENT_DISABLED = 0x20,
    GAMEOBJECT_E20_FLAG_COMBO_MOVEMENT = 0x40,
};

enum GAMEOBJECT_ACTION_FLAGS : u16 {
    GAMEOBJECT_ACTION_FLAG_FORCE_PUSH_WEAPON_IDLE_MASK = 0x0580,
};

enum GAMEOBJECT_EF8_FLAGS : u8 {
    GAMEOBJECT_EF8_FLAG_KEEP_WEAPON_OUT = 0x10,
};

enum GAMEOBJECT_CONTEXT_FLAGS : u8 {
    GAMEOBJECT_CONTEXT_FLAGS_COMBO_START_RETAIN_MASK = 0xa3,
};

enum WEAPON_SCALE_STATE : u8 {
    WEAPON_SCALE_IDLE = 0,
    WEAPON_SCALE_EXTENDING = 1,
    WEAPON_SCALE_RETRACTING = 2,
};

enum GAMEOBJECT_MOVEMENT_FLAGS : u8 {
    GAMEOBJECT_MOVEMENT_FLAG_FACE_REVERSED = 0x04,
    GAMEOBJECT_MOVEMENT_FLAG_BACKWARDS = 0x40,
    GAMEOBJECT_MOVEMENT_FLAG_REVERSE_VELOCITY = 0x80,
};
struct CHARACTERMODEL_s {
    i16 model_id;                         // 0x00
    u8 flags;                             // 0x02
    u8 field_0x3;                         // 0x03
    nuhgobj_s *hierarchy;                 // 0x04
    void **model_data_a;                  // 0x08
    void **model_data_b;                  // 0x0c
    void **model_data_c;                  // 0x10
    nuhgobjpoi_s *points_of_interest[16]; // 0x14
};

DECOMP_ASSERT(sizeof(CHARACTERMODEL_s) == 0x54, "CHARACTERMODEL_s size");
DECOMP_ASSERT(offsetof(CHARACTERMODEL_s, model_data_b) == 0xc, "Character animation table offset");

struct CHARACTER_SHADOW_s {
    NUVEC position;
    u16 x_rotation;
    u16 z_rotation;
    f32 opacity;
};

DECOMP_ASSERT(sizeof(CHARACTER_SHADOW_s) == 0x14, "CHARACTER_SHADOW_s ABI");

typedef struct COINPACKET_s {
    u32 coins;              // 0x00
    f32 scale;              // 0x04
    u16 lastcoin;           // 0x08
    u8 active;              // 0x0a
    u8 field_0xb;           // 0x0b
    f32 double_score_timer; // 0x0c
} COINPACKET;
typedef COINPACKET COINPACKET_s;

typedef struct TORPEDOPACKET_s {
    u8 count;     // 0x00
    u8 field_0x1; // 0x01  (bit0 = in use)
    u8 field_02, field_03;
    u8 target_type; // 0x04: blowup, turret, or obstacle
    u8 pad_05[3];
    f32 field_08;
    f32 ricochet_time;         // 0x0c
    f32 steal_timer;           // 0x10
    u32 pickup_data[5];        // 0x14
    u32 pickup_flags[5];       // 0x28
    NUVEC pickup_positions[5]; // 0x3c
    NUVEC ricochet_position;   // 0x78
    void *target;              // 0x84; selected by target_type
} TORPEDOPACKET;
DECOMP_ASSERT(offsetof(TORPEDOPACKET, ricochet_position) == 0x78, "Torpedo ricochet position offset");
DECOMP_ASSERT(offsetof(TORPEDOPACKET, target) == 0x84, "Torpedo target offset");

// Player/enemy bookkeeping block, base 0x2c0 within a GameObject.
typedef struct AIPACKET_s {
    union {
        AISCRIPTPROCESS script_process;
        struct {
            u8 pad0[0xa0];
            AIAREA_s *area; // 0xa0
            union {
                AILOCATOR_s *locator; // 0xa4
                void *field_0x364;
            };
            AILOCATORSET_s *locator_set; // 0xa8
            u8 pad0b[0xb0 - 0xac];
            u8 creature_set; // 0xb0
            u8 pad0c[0xc8 - 0xb1];
        };
    };
    void *field_0xc8;
    AISCRIPTPROCESS_s *alternate_script_process;
    GameObject_s *owner; // 0xd0
    union {
        void *nearest_opponent;
        APIOBJECT_s *nearest_opponent_object;
        GameObject_s **primary_target_ref;
    };
    f32 nearest_opponent_metric; // 0xd8
    union {
        u32 field_0xdc;
        APIOBJECT_s *pending_nearest_opponent;
    };
    union {
        u32 field_0xe0;
        f32 target_metric_e0;
        f32 pending_nearest_metric;
        f32 primary_target_limit;
    };
    union {
        void *opponent;
        APIOBJECT_s *opponent_object;
        GameObject_s **action_target_ref;
    };
    f32 opponent_metric; // 0xe8
    union {
        u32 field_0xec;
        APIOBJECT_s *pending_opponent;
    };
    union {
        u32 field_0xf0;
        f32 target_metric_f0;
        f32 pending_opponent_metric;
        f32 action_target_limit;
    };
    GameObject_s *dont_avoid_character; // 0xf4
    union {
        u8 pad_f8[0x104 - 0xf8];
        NUVEC creature_origin; // 0xf8, cached formation position
    };
    union {
        NUVEC movement_destination; // 0x104
        NUVEC reset_position;
    };
    union {
        f32 movement_stopping_distance; // 0x110
        u32 field_0x110;
    };
    NUVEC movement_position; // 0x114
    f32 mover_height;        // 0x120
    union {
        i16 inside_path_node; // 0x3e4 overall (-1 when not inside a node)
        i16 field_0x124;
    };
    i16 animation_override_from; // 0x126 (0xe9 means every ordinary animation)
    i16 animation_override_to;   // 0x128
    u8 pad1b[0x12c - 0x12a];
    union __attribute__((packed, aligned(4))) {
        u64 character_type_mask; // 0x12c, character classes accepted by path routes
        struct {
            u32 character_type_mask_low;
            u32 character_type_mask_high;
        };
    };
    u8 field_0x134;           // 0x3f4 overall: source creature index
    u8 path_connection_state; // 0x3f5 overall
    u16 available_routes;     // 0x3f6 overall
    union {
        u8 current_route; // 0x3f8 overall
        u8 field_0x138;
    };
    union {
        u8 next_route; // 0x3f9 overall
        u8 field_0x139;
    };
    u8 reset_mode;       // 0x3fa overall: AI reset/activation state
    u8 goal_speed_mode;  // 0x3fb overall: walk/run/tiptoe speed selector
    u8 movement_stopped; // 0x13c, suppresses synthesized AI movement input
    u8 field_0x13d;
    union {
        u8 divert_search_cursor;
        u8 divert_search_index;
    }; // 0x13e
    union {
        u8 divert_node;
        u8 divert_node_index;
    }; // 0x13f
    AIGROUP_s *group;      // 0x400 overall
    u8 group_member_index; // 0x404 overall
    u8 group_column;       // 0x405 overall
    union {
        u8 group_row;
        u8 group_member;
    }; // 0x406 overall
    u8 movement_target_direction;       // 0x147
    NUVEC terrain_origin;               // 0x148
    AIPATHINFO path_info;               // 0x154
    NUVEC last_path_position;           // 0x16c
    AIPATHNODE_s *goal_path_node;       // 0x178
    f32 movement_instruction_parameter; // 0x17c
    union {
        void *field_0x180;
        AIPATHNODE_s *special_move_node;
    };
    AIPATHCNX_s *movement_target;                // 0x184
    f32 antinode_timer;                          // 0x188
    AIPATHCNX_s *intersection_connection;        // 0x18c
    AIPATHCNX_s *intersection_target_connection; // 0x190
    NUVEC right_diversion;                       // 0x194
    NUVEC left_diversion;                        // 0x1a0
    AILOCATOR_s *respawn_locator;                // 0x1ac
    NUVEC fallback_destination;                  // 0x1b0
    f32 fallback_stopping_distance;              // 0x1bc
    f32 movement_parameter;                      // 0x1c0
    u8 pad_1c4[0x1c8 - 0x1c4];
    AIPATHINFO fallback_path_info; // 0x1c8
    union {
        u32 frame_flags; // 0x1e0
        NUVEC *movement_look_target;
    };
    union {
        u8 movement_flags; // 0x1e4
        u8 field_0x1e4;
    };
    union {
        u8 field_0x1e5;
        struct {
            u8 check_wall_splines : 1;
            u8 circle_clockwise : 1;
            u8 circle_active : 1;
            u8 opponent_is_threat : 1;
            u8 packet_flags_1e5_4_6 : 3;
            u8 antinode_clockwise : 1;
        };
    };
    union {
        u8 runtime_flags;
        u8 field_0x1e6;
    };
    union {
        u8 movement_event_flags;
        u8 field_0x1e7;
        struct {
            u8 movement_event_low : 2;
            u8 movement_range_mode : 2;
            u8 movement_event_high : 4;
        };
    };
    u8 navigation_flags; // 0x1e8
    u8 pad_1e9[0x1ec - 0x1e9];
    f32 movement_target_radius; // 0x1ec
    u32 capabilities;           // 0x1f0
    u32 frame_state;            // 0x1f4
    u32 respawn_count;          // 0x1f8
    f32 spawn_delay;            // 0x1fc
    u32 field_0x200;
    f32 time_off_path; // 0x204
} AIPACKET;

typedef AIPACKET PAI;

enum AIPACKET_NAVIGATION_FLAGS : u8 {
    AIPACKET_NAVIGATION_FLAG_TRANSIENT = 0x01,
    // Search every loaded path instead of only the active level path.
    AIPACKET_NAVIGATION_FLAG_SEARCH_ALL_PATHS = 0x02,
    AIPACKET_NAVIGATION_FLAG_USE_SPECIAL_ROUTES = 0x04,
};

enum AIPACKET_MOVEMENT_MODE : u8 {
    AIPACKET_MOVEMENT_NONE = 0,
    AIPACKET_MOVEMENT_TO_DESTINATION = 1,
    AIPACKET_MOVEMENT_RETREAT = 2,
    AIPACKET_MOVEMENT_CIRCLE = 3,
    AIPACKET_MOVEMENT_WANDER = 4,
    AIPACKET_MOVEMENT_FORMATION = 5,
    AIPACKET_MOVEMENT_AVOIDING_CAMERA = 6,
    AIPACKET_MOVEMENT_DIRECT = 7,
    AIPACKET_MOVEMENT_MODE_MASK = 7,
    AIPACKET_MOVEMENT_OPTION_TRANSIENT = 0x08,
    AIPACKET_MOVEMENT_DIVERSION_RIGHT = 0x20,
    AIPACKET_MOVEMENT_DIVERSION_LEFT = 0x40,
};

enum AIPACKET_RUNTIME_FLAGS : u8 {
    AIPACKET_RUNTIME_INITIALISED = 0x01,
    AIPACKET_RUNTIME_SPECIAL_MOVE = 0x08,
    AIPACKET_RUNTIME_ROUTE_SELECTED = 0x10,
    AIPACKET_RUNTIME_PATH_BLOCKED = 0x20,
    AIPACKET_RUNTIME_USING_PATH_WAYPOINT = 0x40,
};

enum AIPACKET_MOVEMENT_SOURCE_FLAGS : u8 {
    AIPACKET_MOVEMENT_SOURCE_MASK = 0x0c,
    AIPACKET_MOVEMENT_SOURCE_CREATURE = 0x04,
    AIPACKET_MOVEMENT_SOURCE_LOCATOR = 0x08,
    AIPACKET_MOVEMENT_SOURCE_ACTIVE = 0x10,
    AIPACKET_PATH_CONNECTION_CHANGED = 0x20,
    AIPACKET_MOVEMENT_FORCE_PATH_REFRESH = 0x02,
    AIPACKET_MOVEMENT_SPECIAL_HANDLED = 0x80,
};

typedef struct APIOBJECT_s {
    GameObject_s *objptr;              // 0x00
    PAI *ai;                           // 0x04
    ANIMPACKET_s anim_packet;          // 0x08
    CHARACTERMODEL_s *character_model; // 0x50
    characterdata_s *character_data;   // 0x54
    u16 facing_angle;                  // 0x58
    u16 movement_facing_angle;         // 0x5a
    NUVEC position;                    // 0x5c
    NUVEC velocity;                    // 0x68
    NUVEC previous_velocity;           // 0x74
    union {
        struct {
            f32 pos_x;
            f32 pos_y;
            f32 pos_z;
        };
        NUVEC collision_position; // 0x80
    };
    NUVEC start_position;   // 0x8c
    NUVEC initial_position; // 0x98
    f32 scaled_radius;      // 0xa4
    f32 field_0xa8;         // 0xa8
    f32 collision_radius;   // 0xac
    f32 collision_height;   // 0xb0
    f32 scaled_height;      // 0xb4
    NUMTX field_0xb8;       // 0xb8
    NUMTX field_0xf8;       // 0xf8
    NUMTX field_0x138;      // 0x138
    NUVEC collision_min;    // 0x178
    NUVEC collision_max;    // 0x184
    union {
        NUVEC upper_position; // 0x190
        struct {
            f32 field_0x190;
            f32 field_0x194;
            f32 field_0x198;
        };
    };
    union {
        NUVEC lower_position; // 0x19c
        f32 field_0x19c[3];
    };
    NUVEC collision_origin; // 0x1a8
    union {
        NUVEC last_safe_position; // 0x1b4
        f32 previous_position[3];
    };
    NUVEC field_0x1c0;      // 0x1c0  alternate camera position used by vehicle type 0x2b
    NUVEC respawn_position; // 0x1cc
    union {
        f32 respawn_timer;
        f32 movement_stuck_time;
    }; // 0x1d8
    f32 field_0x1dc; // 0x1dc
    f32 field_0x1e0; // 0x1e0
    union __attribute__((packed, aligned(4))) {
        struct {
            u32 field_0x1e4;
            u32 field_0x1e8;
        };
        u64 collision_identity_mask; // 0x1e4
    };
    union __attribute__((packed, aligned(4))) {
        struct {
            u32 field_0x1ec;
            u32 field_0x1f0;
        };
        u64 colliding_objects_mask;
        u64 collision_contact_mask; // 0x1ec
    };
    u32 field_0x1f4; // 0x1f4
    union {
        u32 object_flags; // 0x1f8, complete flag word
        struct {
            u8 flags_low;
            union {
                u8 flags_high;
                struct {
                    u8 : 1;
                    u8 force_los_visible : 1;
                    u8 skip_los_raycast : 1;
                    u8 : 1;
                    u8 character : 1;
                    u8 respawn_enabled : 1;
                    u8 use_cached_los : 1;
                    u8 : 1;
                };
            };
            union {
                u8 field_0x1fa;
                struct {
                    u8 object_flag_1fa_0 : 1;
                    u8 ignore_antinodes : 1;
                    u8 object_flags_1fa_2_7 : 6;
                };
            };
            u8 field_0x1fb;
        };
        u16 field_0x1f8;
        struct {
            u8 : 7;
            u8 player_controlled : 1;
            u8 script_enabled : 1;
            u8 : 7;
        };
        struct {
            u16 in_use : 1;
            u16 other_object_flags : 15;
        };
    };
    union {
        struct {
            f32 field_0x1fc;
            f32 field_0x200;
            f32 field_0x204;
        };
        NUVEC movement_direction; // 0x1fc
    };
    union {
        undefined field_0x208[0x214 - 0x208];
        nuhspecial_s antinode_special;
        nuhspecial_s collision_special;
    };
    f32 field_0x214;
    f32 field_0x218;                   // 0x218
    f32 water_height;                  // 0x21c
    f32 field_0x220;                   // 0x220
    f32 velocity_magnitude;            // 0x224
    f32 horizontal_velocity_magnitude; // 0x228
    f32 viewdistance;                  // 0x22c
    f32 heardistance;                  // 0x230
    f32 maxviewheight;                 // 0x234
    f32 minviewheight;                 // 0x238
    union {
        undefined field_0x23c[4];
        f32 visibility_range_extension;
    }; // 0x23c
    NUVEC previous_animation_root;                    // 0x240
    NUVEC previous_blend_target_root;                 // 0x24c
    f32 previous_animation_root_time;                 // 0x258
    f32 previous_blend_target_root_time;              // 0x25c
    CHARACTERANIM_s *previous_animation_root_info;    // 0x260
    CHARACTERANIM_s *previous_blend_target_root_info; // 0x264
    NUVEC animation_root_delta;                       // 0x268
    u16 pitch_angle;                                  // 0x274
    u16 field_0x276;                                  // 0x276
    u16 roll_angle;                                   // 0x278
    i16 supporting_platform_id;                       // 0x27a (-1 = no supporting character platform)
    union {
        u32 packed_contact_state; // 0x27c
        struct {
            char field_0x27c; // player/character slot (0xff = none)
            u8 field_0x27d;   // terrain/contact flags
            u8 field_0x27e;   // previous terrain/contact flags
            u8 field_0x27f;
        };
    };
    u8 field_0x280;       // 0x280
    u8 field_0x281;       // 0x281
    u8 is_underwater;     // 0x282
    u8 intersects_water;  // 0x283
    u8 model_draw_result; // 0x284
    u8 field_0x285;
    u8 field_0x286;
    u8 field_0x287; // 0x287  owner/controller player index
    u8 field_0x288; // 0x288
    u8 field_0x289; // 0x289
    union {
        undefined field_0x28a[0x0a];
        struct {
            union {
                u8 pad_28a[4];
                struct {
                    u16 surface_effect_count;
                    u16 surface_effect_id;
                };
            };
            union {
                u16 movement_request_flags;
                u16 collision_priority;
            };
            u16 resolved_collision_priority;
            u16 reserved_0x292;
        };
    };
    union {
        APIOBJECT_s *collision_link;
        APIOBJECT_s *collision_excluded_object;
    }; // 0x294, paired objects do not collide with each other
    union __attribute__((packed, aligned(4))) {
        struct {
            union {
                u32 collision_mask_low;
                u32 collision_exclusion_mask_low;
            }; // 0x298
            union {
                u32 collision_mask_high;
                u32 collision_exclusion_mask_high;
            }; // 0x29c
        };
        u64 collision_exclusion_mask;
    };
    union __attribute__((packed, aligned(4))) {
        u64 ai_awareness_mask;
        struct {
            u32 field387_0x2a0;
            u32 field388_0x2a4;
        };
    };
    union __attribute__((packed, aligned(4))) {
        u64 ai_area_mask;
        struct {
            union {
                u32 ai_area_mask_low;
                u32 field_0x2a8;
            };
            union {
                u32 ai_area_mask_high;
                u32 field_0x2ac;
            };
        };
    };
} APIOBJECT;

DECOMP_ASSERT(offsetof(APIOBJECT, collision_identity_mask) == 0x1e4, "APIOBJECT collision identity mask offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_contact_mask) == 0x1ec, "APIOBJECT collision contact mask offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_exclusion_mask) == 0x298, "APIOBJECT collision exclusion mask offset");
DECOMP_ASSERT(offsetof(APIOBJECT, field_0x1f4) == 0x1f4, "APIOBJECT flags after contact mask offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_excluded_object) == 0x294, "APIOBJECT collision exclusion pointer offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_exclusion_mask_low) == 0x298, "APIOBJECT collision exclusion mask offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_exclusion_mask_high) == 0x29c,
              "APIOBJECT collision exclusion high mask offset");

extern "C" void APIObjectCollisions(i32 count, APIOBJECT **objects, NUVEC *minimums, NUVEC *maximums,
                                    i32 (*collide)(APIOBJECT *, APIOBJECT *));
extern "C" i32 APIObjectCollision(APIOBJECT *first, APIOBJECT *second);
extern "C" i32 APIObjectCollision2D(APIOBJECT *first, APIOBJECT *second);

struct APIOBJECTSYS_s {
    u32 object_size;
    APIOBJECT *objects;
    union __attribute__((packed, aligned(4))) {
        u8 state[0x214 - 8];
        u64 line_of_sight[64];
        u32 hostility_masks[64][2]; // 0x008, one 64-bit mask per object slot
        struct {
            u8 state_008[0x208 - 8];
            i32 los_source_index;
            i32 los_target_index;
            union {
                u8 flags_210;
                u8 runtime_flags;
                struct {
                    u8 skip_los_raycast : 1;
                    u8 : 7;
                };
            };
            u8 state_211[3];
        };
    };
};

DECOMP_ASSERT(sizeof(APIOBJECTSYS_s) == 0x214, "APIOBJECTSYS size");
DECOMP_ASSERT(offsetof(APIOBJECTSYS_s, flags_210) == 0x210, "APIOBJECTSYS flags offset");
DECOMP_ASSERT(offsetof(APIOBJECTSYS_s, los_source_index) == 0x208, "APIOBJECTSYS LOS source index offset");
DECOMP_ASSERT(offsetof(APIOBJECTSYS_s, los_target_index) == 0x20c, "APIOBJECTSYS LOS target index offset");
DECOMP_ASSERT(offsetof(APIOBJECTSYS_s, hostility_masks) == 8, "APIOBJECTSYS hostility masks offset");

extern "C" APIOBJECT *APIObjectCreate(APIOBJECTSYS_s *system);
extern "C" void APIObjectDestroy(APIOBJECTSYS_s *system, APIOBJECT *object);
extern "C" void APIObjectDestroyAll(APIOBJECTSYS_s *system);
extern "C" void APIObjectLOSChecks(APIOBJECTSYS_s *system, i32 checks, i32 source_count, APIOBJECT **sources,
                                   i32 target_count, APIOBJECT **targets, f32 ray_step);
extern "C" i32 QuickNewRayCast(NUVEC *position, NUVEC *movement, f32 radius, i32 scan_flags, f32 max_distance,
                               f32 step);
extern "C" void APIObjectSetUsed(APIOBJECT *object, i32 index, i32 used);
extern "C" void APIObjectVelocities(GameObject_s *object);
extern "C" i32 APIObjectCollision(APIOBJECT *first, APIOBJECT *second);
extern "C" i32 APIObjectCollision2D(APIOBJECT *first, APIOBJECT *second);
extern "C" void APIObjectCollisions(i32 count, APIOBJECT **objects, NUVEC *minimums, NUVEC *maximums,
                                    i32 (*collision_callback)(APIOBJECT *, APIOBJECT *));

struct rtldata_s {
    union {
        u8 data[0x144];
        struct {
            u8 unknown_000[0x78];
            NUCOLOUR3 intensity[3]; // 0x078
            NUVEC direction[3];     // 0x09c
            NUVEC ambient;          // 0x0c0
            u8 unknown_0cc[0x78];
        };
    };
};

DECOMP_ASSERT(sizeof(rtldata_s) == 0x144, "rtldata_s size");
DECOMP_ASSERT(offsetof(rtldata_s, intensity) == 0x78, "rtldata_s intensity offset");
DECOMP_ASSERT(offsetof(rtldata_s, direction) == 0x9c, "rtldata_s direction offset");
DECOMP_ASSERT(offsetof(rtldata_s, ambient) == 0xc0, "rtldata_s ambient offset");

struct OBJECTLIGHTINGSTATE_s {
    NUVEC ambient;          // 0x00
    NUCOLOUR3 intensity[3]; // 0x0c
    NUVEC direction[3];     // 0x30
};

DECOMP_ASSERT(sizeof(OBJECTLIGHTINGSTATE_s) == 0x54, "OBJECTLIGHTINGSTATE_s size");

typedef struct GameObject_s {
    union {
        APIOBJECT apiobj; // 0x0000 .. 0x02b0
        struct {
            u8 apiobject_prefix[offsetof(APIOBJECT, ai_area_mask_low)];
            union {
                u32 field_0x2a8;
                u32 ai_area_mask_low;
            };
            union {
                u32 field_0x2ac;
                u32 ai_area_mask_high;
            };
        };
    };
    union {
        u8 pad_2b0[0x10]; // 0x02b0 .. 0x02c0
        struct {
            f32 idle_total_time;         // 0x02b0
            f32 idle_animation_time;     // 0x02b4
            f32 idle_animation_limit;    // 0x02b8
            i16 idle_animation;          // 0x02bc
            i16 previous_idle_animation; // 0x02be
        };
    };
    union {
        PAI ai; // 0x02c0 .. 0x04c8
        struct {
            u8 ai_packet_prefix[0x1f8];
            u32 ai_respawn_count; // 0x04b8
            f32 ai_spawn_delay;   // 0x04bc
            u8 ai_packet_suffix[8];
        };
    };
    rtldata_s light_data;                 // 0x04c8
    OBJECTLIGHTINGSTATE_s lighting_state; // 0x060c
    union {
        SOCKPOSITION sock_position; // 0x0660
        struct {
            u8 sock_location_flags; // 0x0660
            u8 field_0x661;         // 0x0661, signed socket index (0xff = none)
            i16 sock_segment;       // 0x0662
            u8 pad_664[0x686 - 0x664];
            u16 yrot; // 0x0686, aliases sock_position.midpoint_rotation.y
            u8 pad_688[0x68c - 0x688];
            f32 field_0x68c; // 0x068c, aliases sock_position.distance
            u8 pad_690[0x698 - 0x690];
        };
    };
    NUVEC contact_position;    // 0x0698
    NUVEC contact_normal;      // 0x06a4
    u8 field_0x6b0;            // 0x06b0 terrain/contact state
    u8 pad_6b1[0x6b4 - 0x6b1]; // 0x06b1 .. 0x06b4
    union {
        u8 player_packet[0x780 - 0x6b4]; // 0x06b4, PLAYERPACKET_s begins here
        struct {
            u8 carry_prefix[0x738 - 0x6b4];
            NUVEC carried_object_basis[3]; // 0x738: rotation rows, or offsets for aligned carrying
        };
        CHARACTER_SHADOW_s character_shadows[5];
        struct {
            u8 suspension_prefix[0x718 - 0x6b4];
            struct {
                f32 height;
                f32 velocity;
            } suspension[4];
        };
        struct {
            u8 player_packet_prefix[0x738 - 0x6b4];
            union {
                NUVEC external_force;
                NUVEC zipup_entry_position; // 0x738, position before the whip start animation
            };
            union {
                u8 pad_744[0x768 - 0x744];
                NUVEC launch_origin; // 0x744, saved on entering launch context
                struct {
                    union {
                        NUVEC tightrope_position;
                        NUVEC context_destination;
                    };
                    union {
                        NUVEC tightrope_offset;
                        NUVEC context_position_offset;
                    };
                };
                struct {
                    NUVEC zipup_start_position; // 0x744, selected endpoint
                    NUVEC zipup_swing_position; // 0x750, end of the swing animation
                    union {
                        NUVEC zipup_landing_position;
                        NUVEC carried_object_drop_position;
                    }; // 0x75c
                };
            };
            f32 field_0x768; // 0x0768
            union {
                f32 context_animation_timer; // 0x076c
                f32 drop_transition_time;
            };
            f32 field_0x770; // 0x0770
            union {
                f32 airborne_action_duration; // 0x0774
                f32 drop_transition_duration;
            };
            f32 airborne_action_timer; // 0x0778
            f32 jump_start_height;     // 0x077c
        };
    };
    union {
        void *field_0x780;
        GameObject_s *force_target;
        GameObject_s *takeover_entry_target;
    };
    GIZMOBLOWUP_s *blowup_target; // 0x0784
    void *field_0x788;            // 0x0788
    union {
        u8 pad_78c[0x790 - 0x78c];
        i32 panel_use_request;
        i32 tube_entry_state;
    };
    union {
        void *big_jump_data;
        void *tube_entry_data;
    }; // 0x0790
    union {
        u16 context_x_rotation;
        u16 magnet_surface_angle;
        u16 tightrope_x_rotation;
    }; // 0x794
    union {
        u16 takeover_start_angle;
        u16 carried_object_angle;
    }; // 0x796
    union {
        u16 context_z_rotation;
        u16 grapple_swing_phase;
        u16 tightrope_z_rotation;
    }; // 0x798
    i16 context_animation;        // 0x079a, action-owned animation index
    i16 queued_context_animation; // 0x079c, base action used by combo branches
    u8 combo_branch;              // 0x079e, selected offset from the base combo action
    u8 pad_79f[0x7a0 - 0x79f];    // 0x079f .. 0x07a0
    union {
        struct {
            u8 combo_input_latched; // 0x07a0
            u8 landing_followup;    // 0x07a1
            u8 combo_stage;         // 0x07a2
            u8 field_0x7a3;         // 0x07a3, affects camera-look stick selection
        };
        u32 action_input_state;
    };
    union {
        struct {
            union {
                u8 build_button_taps; // 0x07a4, capped Build-It acceleration input
                u8 grapple_swing_degrees;
            };
            union {
                u8 field_0x7a5;
                u8 character_id_0x7a5; // character selector used by ObjLookingWithLeftStick
                i8 character_context;  // 0x07a5, current action owner (-1 when unowned)
                i8 build_context;      // Build-It alias retained for its existing callers
            };
            u8 field_0x7a6;
            i8 field_0x7a7;
        };
        u32 movement_context_state; // 0x07a4, packed action context and variant state
    };
    u8 action_movement_state; // 0x07a8
    u8 jump_sequence;         // 0x07a9, 1 for the first jump and 2 for the second
    u8 field_0x7aa;
    i8 hit_variant;
    u8 context_flags;         // 0x07ac
    i8 context_variant_flags; // 0x07ad
    u8 jump_flags;            // 0x07ae
    u8 pad_7af;
    union {
        f32 tag_cooldown;
        f32 tag_context_timer;
    }; // 0x07b0
    union {
        i8 tag_player_index;
        i8 tag_target_player;
    }; // 0x07b4
    union {
        u8 tag_flags;
        u8 tag_context_flags;
    }; // 0x07b5
    u8 pad_7b6[2];
    union {
        GameObject_s *pending_tag_target;
        GameObject_s *tag_target;
    }; // 0x07b8
    union {
        u8 pad_7bc[4];
        SOCKROT sock_angles; // 0x07bc, blended midpoint pitch and yaw
    };
    union {
        u8 mini_anim_packet[0x24]; // 0x07c0 .. 0x07e4
        MINIANIMPACKET_s mini_animation;
    };
    u8 *field_0x7e4; // 0x07e4, record with kind byte at +8
    u8 field_0x7e8;
    u8 pad_7e9[3];
    COINPACKET *coinpacket;                // 0x07ec
    GizForceLOSState_s *gizforce_los_info; // 0x07f0
    union {
        NUMTX joint_matrices[16]; // 0x07f4 .. 0x0bf4
        struct {
            NUMTX field_0x7f4;
            NUMTX remaining_joint_matrices[15];
        };
    };
    union {
        u8 pad_bf4[0xc34 - 0xbf4]; // 0x0bf4 .. 0x0c34
        NUMTX vehicle_orientation;
    };
    union {
        u32 field_0xc34;
        f32 current_speed_mul;
    }; // 0x0c34
    f32 field_0xc38; // 0x0c38
    u32 field_0xc3c;
    NUVEC weapon_trail_offset; // 0x0c40
    union {
        struct {
            f32 head_target_facing;
            u8 pad_c50[4];
            f32 field_0xc54;
        };
        NUVEC camera_screen_position; // 0x0c4c
    };
    NUVEC force_glow_position;
    NUVEC last_safe_camera_position; // 0x0c64
    NUVEC attack_target_position;    // 0xc70
    NUVEC attack_target_velocity;    // 0xc7c
    union {
        u8 pad_c88[0xc94 - 0xc88];
        NUVEC suspension_normal;
    };
    struct GAMEPAD_s *pad_gamepad; // 0x0c94  (originally inside PLAYERPACKET_s)
    SOCKPOSITION *oldpos;          // 0x0c98
    u32 directional_input_state;
    PART_s *incoming_part;
    PART_s *blocked_part;
    union {
        void *field_0xca8;
        PART_s *force_part;
    };
    void *suit;             // 0x0cac
    void *batarang;         // 0x0cb0
    TORPEDOPACKET *torpedo; // 0x0cb4
    union {
        GameObject_s *collision_target;
        GIZMOBLOWUP_s *attack_blowup_target;
        PART_s *attack_part_target;
        void *attack_gizmo_target;
    };
    u32 field_0xcbc;
    GameObject_s *field_0xcc0; // 0x0cc0
    GameObject_s *takeover_source;
    void *sabre_streaks[4][2]; // 0x0cc8
    GameObject_s *block_attacker;
    GameObject_s *incoming_melee;
    GameObject_s *incoming_special;
    BOLT_s *incoming_bolt;
    BOLT_s *blocked_bolt;
    u32 field_0xcfc;
    GameObject_s *force_throw_target;
    void *force_glow_previous;
    void *force_glow_object;
    void *force_glow_candidate;
    GameObject_s *airborne_collision_target; // 0x0d10
    union {
        u32 field_0xd14;
        GameObject_s *force_push_target;
    }; // 0x0d14
    f32 ground_contact_grace_timer; // 0x0d18, keeps airborne animation briefly after contact
    f32 jump_reentry_timer;         // 0x0d1c
    f32 airborne_input_timer;       // 0x0d20
    f32 field_0xd24;                // 0x0d24
    union {
        u8 pad_d28[4];
        f32 timer_d28;
    };
    f32 communicate_blend;
    f32 weapon_scale;           // 0x0d30, 0.0 retracted .. 1.0 extended
    f32 weapon_scale_rate;      // 0x0d34
    f32 sabre_collision_radius; // 0x0d38
    f32 weapon_out_timer;       // 0x0d3c
    f32 delayed_turn_timer;     // 0x0d40
    f32 combo_input_timer;      // 0x0d44
    f32 ai_combo_cooldown;
    union {
        u8 pad_d4c[8];
        struct {
            u8 reserved_d4c[4];
            f32 timer_d50;
        };
    };
    f32 quick_shoot_timer; // 0x0d54
    union {
        u8 pad_d58[4];
        f32 torpedo_fire_cooldown;
    };
    union {
        i32 pause_input_state; // 0x0d5c, cleared when entering pause
        f32 timer_d5c;
        f32 hud_icon_timer; // countdown controlling the portrait blink during death/drop transitions
        f32 tag_state;      // PLAYERPACKET_s + 0x6a8, tag/input transition state
    };
    union {
        u8 pad_d60[4];
        f32 targeted_flash;
    }; // 0x0d60 .. 0x0d64
    f32 jump_variant_timer;               // 0x0d64
    f32 jump_chain_timer;                 // 0x0d68
    f32 field_0xd6c;                      // 0x0d6c  surface/contact state
    f32 movement_animation_hold_timer;    // 0x0d70
    f32 movement_animation_release_timer; // 0x0d74
    f32 field_0xd78;                      // 0x0d78
    f32 terrain_origin_floor_offset;      // 0x0d7c
    union {
        f32 field_0xd80;
        f32 torpedo_target_timer;
    }; // 0x0d80
    f32 force_glow_target;
    f32 force_glow_step;
    f32 field_0xd8c; // 0x0d8c
    f32 force_hold_time;
    f32 force_use_volume;
    f32 turn_braking;           // 0xd98, horizontal momentum attenuation in turning context
    f32 fall_recovery_timer;    // 0x0d9c
    f32 nearby_floor_distance;  // 0x0da0, sentinel when no floor is nearby
    f32 input_toggle_hold_time; // 0x0da4, reset when entering pause
    f32 field_0xda8;            // 0x0da8
    f32 fall_animation_timer;   // 0x0dac
    f32 field_0xdb0;
    f32 fall_hover_height; // 0x0db4
    u8 pad_db8[4];
    f32 field_0xdbc; // 0x0dbc
    union {
        u8 pad_dc0[0xdc8 - 0xdc0];
        struct {
            f32 sabre_contact_sfx_timer;
            union {
                u8 reserved_dc4[4];
                f32 platform_separation_timer;
                f32 field_0xdc4;
            };
        };
    };
    f32 field_0xdc8; // 0x0dc8
    union {
        u8 pad_dcc[4];
        f32 thrust_effect_scale; // 0x0dcc, scales the character's thrust-locator models
    };
    f32 takeover_timer; // 0x0dd0
    union {
        u8 pad_dd4[4];
        f32 interaction_arrow_blend;
    };
    f32 block_cooldown;
    f32 field_0xddc;
    f32 field_0xde0; // 0x0de0
    f32 hold_timer;  // 0x0de4
    union {
        u8 pad_de8[0xdec - 0xde8];
        f32 ai_jump_timer;
    };
    f32 field_0xdec; // 0x0dec
    union {
        u8 pad_df0[0xdf8 - 0xdf0];
        struct {
            f32 pre_terrain_speed;
            f32 post_terrain_speed;
        };
    };
    NUVEC reset_velocity; // 0x0df8 .. 0x0e04
    i32 dynamic_light_id;
    u16 delayed_turn_target_angle; // 0x0e08
    u16 force_heading;
    u16 current_input_angle; // 0x0e0c
    union {
        u8 pad_e0e[2];
        u16 previous_boundary_angle; // 0xe0e, vehicle boundary steering hysteresis
    };
    i16 previous_block_animation; // 0x0e10
    u8 pad_e12[2];
    i16 held_movement_animation;     // 0x0e14
    i16 released_movement_animation; // 0x0e16
    i16 movement_lean_angle;         // 0x0e18
    i16 secondary_lean_angle;        // 0x0e1a
    i16 tertiary_lean_angle;         // 0x0e1c
    i16 field_0xe1e;                 // 0x0e1e
    union {
        struct {
            u8 field_0xe20;
            u8 field_0xe21;
        };
        u16 action_flags; // 0x0e20
    };
    u8 field_0xe22;            // 0x0e22
    u8 field_0xe23;            // 0x0e23
    u8 field_0xe24;            // 0x0e24
    u8 movement_runtime_flags; // 0x0e25
    union {
        u8 pad_e26[0xe2b - 0xe26];
        struct {
            u8 flicker_flags;
            u8 reserved_e27[4];
        };
    };
    u8 force_glow_kind;
    u8 force_glow_candidate_kind;
    u8 blocked_attack_stage;
    u8 block_latch;
    union {
        u8 pad_e2f[2];
        struct {
            u8 quick_shoot_flags;
            u8 reserved_e30;
        };
    };
    u8 field_0xe31; // 0x0e31
    union {
        u8 field_0xe32;
        WEAPON_SCALE_STATE weapon_scale_state; // 0x0e32
    };
    u8 sabre_flags;  // 0x0e33
    u8 sabre_damage; // 0x0e34
    u8 field_0xe35;
    u8 field_0xe36;       // 0x0e36
    u8 field_0xe37;       // 0x0e37
    u8 field_0xe38;       // 0x0e38
    u8 bolt_fire_phase;   // 0x0e39
    u8 action_suppressed; // 0x0e3a
    u8 in_narrow_socket;
    union {
        u8 pad_e3c[3];
        struct {
            u8 reserved_e3c[2];
            u8 quick_shoot_bolt_id;
        };
    };
    i8 slam_debris_effect; // 0x0e3f
    u8 combo_alternate;    // 0x0e40, alternates the opening saber action
    u8 field_0xe41;        // 0x0e41  current surface type
    i8 blade_index;        // 0x0e42
    i8 blade_states[4];    // 0x0e43
    u8 pad_e47[0xe4c - 0xe47];
    struct MechTouchTask *touch_task;           // 0x0e4c
    MechObjectInterface *mech_object_interface; // 0x0e50
    MechAddonCollection *addons;                // 0x0e54
    NUVEC field_0xe58;
    NUVEC field_0xe64;
    union {
        nugspline_s *movement_spline; // 0x0e70
        struct {
            u8 padding_e70[7];
            u8 movement_spline_finished;
        };
        SPLINEPOS_s movement_spline_position;
    };
    NUVEC movement_spline_offset; // 0x0e90
    union {
        u8 pad_e9c[0xeb0 - 0xe9c];
        struct {
            u8 padding_render_offset[4];
            NUVEC render_offset; // 0x0ea0, world-space displacement added before rendering
        };
        struct {
            u8 padding_e9c[0x10];
            GameObject_s *script_fire_target;
        };
    };
    GameObject_s *takeover_target;       // 0x0eb0, reciprocal target link used by AI takeover actions
    void (*field_0xeb4)(GameObject_s *); // 0x0eb4, one-shot callback consumed by object management
    NUVEC *context_target_position;      // 0x0eb8
    union __attribute__((packed, aligned(4))) {
        u64 ai_seen_mask;
        struct {
            u32 field_0xebc;
            u32 field_0xec0;
        };
    };
    union __attribute__((packed, aligned(4))) {
        u64 ai_opponent_exclusion_mask;
        struct {
            u32 field_0xec4;
            u32 field_0xec8;
        };
    };
    union {
        u32 field_0xecc;
        APIOBJECT_s *alert_target;
    };
    union {
        f32 field_0xed0;
        f32 alert_target_timer;
    };
    union {
        u32 field_0xed4;
        f32 current_speed_multiplier;
    }; // 0x0ed4
    union {
        f32 field_0xed8;
        f32 timer_ed8;
    }; // 0x0ed8
    union {
        u8 pad_edc[0xee0 - 0xedc];
        f32 jump_destination_distance;
    }; // 0x0edc .. 0x0ee0
    union {
        f32 field_0xee0;
        f32 run_speed_override;
    }; // 0xee0
    union {
        u8 pad_ee4[4];
        f32 walk_speed_override;
    }; // 0xee4
    f32 field_0xee8; // 0x0ee8
    f32 field_0xeec; // 0x0eec
    f32 field_0xef0;
    f32 pause_context_state; // 0x0ef4, icon timer, also cleared when entering pause
    u8 field_0xef8;          // 0x0ef8
    u8 field_0xef9;          // 0x0ef9
    u8 field_0xefa;          // 0x0efa
    u8 field_0xefb;          // 0x0efb, bit 3 requests the two-row hit-point layout
    u8 field_0xefc;          // 0x0efc
    union {
        u8 field_0xefd;
        struct {
            u8 : 1;
            u8 random_layer_variant : 1;
            u8 snap_facing : 1;
            u8 : 5;
        };
    }; // 0x0efd
    u8 field_0xefe; // 0x0efe
    u8 field_0xeff; // 0x0eff
    union {
        u8 field_0xf00; // 0x0f00
        struct {
            u8 : 7;
            u8 awkward_shape_override : 1;
        };
    };
    union {
        u8 field_0xf01; // 0x0f01
        struct {
            u8 : 2;
            u8 ignore_slide_terrain : 1;
            u8 : 1;
            u8 spline_follow_terrain : 1;
            u8 : 3;
        };
    };
    union {
        u8 field_0xf02;
        struct {
            u8 : 4;
            u8 force_repeat_requested : 1;
            u8 : 3;
        };
    };
    u8 field_0xf03; // GAMEOBJECT_F03_FLAGS
    union {
        u8 field_0xf04;
        u8 jump_input_flags; // 0x0f04, buffered airborne action inputs
    };
    u8 edge_stop_requests; // 0xf05
    u8 pad_f06[2];
    union {
        void *use_target;
        void *can_use_object;
        u32 field_0xf08;
    }; // 0xf08
    union {
        u32 field_0xf0c;
        struct {
            u8 use_action;
            union {
                u8 use_action_frames;
                u8 use_attach_frames;
            };
            u8 pad_f0e[2];
        };
    };
    union {
        f32 use_distance;
        f32 use_action_parameter;
        u32 field_0xf10;
    }; // 0xf10
    union {
        u8 pad_f14[4];
        u32 field_0xf14;
    };
    f32 big_jump_height; // 0xf18, nonnegative arc height set on entering big jump
    f32 field_0xf1c;     // 0x0f1c
    union {
        AILOCATOR_s *doomed_escape_locator;
        u32 field_0xf20;
    }; // 0xf20
    NUVEC target_velocity;  // 0x0f24
    NUVEC surface_normal;   // 0x0f30
    NUVEC facing_direction; // 0x0f3c
    union {
        u8 pad_f48[0xfe4 - 0xf48];
        NUJOINTANIM_s joint_modifiers[3];
        struct {
            union {
                u8 field_0xf48[0x68];
                u8 player_ai_reset_data[0x68];
            };
            union {
                u8 pad_fb0[0xfe4 - 0xfb0];
                u8 reserved_fb0[0xfe4 - 0xfb0];
            };
        };
    };
    NUVEC *head_target;
    NUVEC head_target_position;
    f32 head_target_timer;
    f32 head_target_delay;
    union {
        f32 field_0xffc;
        f32 character_bottom; // 0x0ffc, unscaled lower character bound
    };
    union {
        f32 field_0x1000;
        f32 character_top; // 0x1000, unscaled upper character bound
    };
    f32 field_0x1004; // 0x1004
    f32 field_0x1008; // 0x1008
    f32 spawn_protection_timer;
    f32 collision_y_scale; // 0x1010
    union {
        u8 pad_1014[4];
        u32 field_0x1014;
        f32 timer_1014;
    };
    f32 field_0x1018; // 0x1018
    f32 field_0x101c; // 0x101c
    f32 field_0x1020; // 0x1020
    union {
        f32 field_0x1024;
        f32 flicker_timer;
        f32 flicker_time;
    };
    f32 ai_update_distance;    // 0x1028, distance used to select the staggered AI cadence
    f32 shadow_opacity;        // 0x102c
    f32 shadow_radius;         // 0x1030
    f32 hover_height_override; // 0x1034
    f32 field_0x1038;
    union {
        u8 pad_103c[4];
        f32 terrain_impact_speed; // 0x103c, negative incoming velocity dot contact normal
    };
    f32 animation_speed_multiplier; // 0x1040
    union {
        u8 pad_1044[0x1048 - 0x1044];
        u32 field_0x1044;
        f32 camera_shake_strength; // 0x1044, POD camera shake contribution
    };
    f32 fall_acceleration_timer; // 0x1048
    CABLE_s *cable;              // 0x104c
    u32 field_0x1050;            // 0x1050
    u32 field_0x1054;            // 0x1054
    union {
        u8 pad_1058[4];
        u32 extra_layer_mask;
        u32 field_0x1058;
    }; // 0x1058
    u16 field_0x105c;            // 0x105c terrain query flags
    u16 field_0x105e;            // 0x105e surface x rotation
    u16 field_0x1060;            // 0x1060 surface z rotation
    u16 field_0x1062;            // 0x1062 previous surface x rotation
    u16 field_0x1064;            // 0x1064 previous surface z rotation
    u16 shadow_joint_mask;       // 0x1066
    u16 field_0x1068;            // 0x1068 reflection x rotation
    u16 field_0x106a;            // 0x106a reflection z rotation
    u16 previous_movement_angle; // 0x106c
    u16 field_0x106e;            // 0x106e
    i16 id;                      // 0x1070
    union {
        u8 pad_1072[0x1076 - 0x1072];
        struct {
            i16 route_character_id;
            i16 route_suit_index;
        };
    };
    i16 room_id;      // 0x1076, portal room containing the character
    i16 field_0x1078; // 0x1078 reflected/platform terrain id
    i16 field_0x107a; // 0x107a terrain id
    i16 field_0x107c; // 0x107c
    u8 field_0x107e;
    u8 field_0x107f;
    f32 field_0x1080;
    u8 field_0x1084;     // 0x1084
    u8 use_model_origin; // 0x1085
    u8 field_0x1086;     // 0x1086
    u8 field_0x1087;     // 0x1087
    u8 field_0x1088;     // 0x1088
    u8 field_0x1089;     // 0x1089
    u8 hitpoints;        // 0x108a
    i8 current_hp;       // 0x108b, signed in pickup and tag health comparisons
    i8 head_target_priority;
    union {
        u8 pad_108d;
        u8 route_start_index;
    };
    u8 field_0x108e; // 0x108e
    u8 field_0x108f;
    u8 one_at_once_player; // 0x1090 (0xff when no attack slot is assigned)
    u8 attack_override;    // 0x1091
    u8 field_0x1092;       // 0x1092
    u8 field_0x1093;       // 0x1093
    u8 field_0x1094;
    union {
        u8 pad_1095[3];
        struct {
            u8 route_search_index;
            u8 pad_1096[2];
        };
    };
    u32 field_0x1098;
    union {
        u32 field_0x109c; // 0x109c
        f32 special_move_timer;
        f32 special_move_progress;
    };
    union {
        AIPATHNODE_s *special_move_node;
        AIPATHNODE_s *special_move_next_node;
    }; // 0x10a0
    union {
        u8 pad_10a4[0x10b0 - 0x10a4];
        NUVEC special_move_look_position;
    };
    void *opponent;              // 0x10b0
    GameObject_s *last_attacker; // 0x10b4
    union {
        SNAKEBODY_s *snake_body; // 0x10b8
        void *field_0x10b8;
    };
    void (*move_override)(GameObject_s *); // 0x10bc
    union {
        u8 pad_10c0[0x10c4 - 0x10c0];
        u32 field_0x10c0;
    };
    f32 ai_elapsed_time; // 0x10c4, accumulated until the next AI update
    union {
        NUVEC ai_update_position;
        NUVEC saved_position; // 0x10c8
        struct {
            f32 field_0x10c8;
            f32 field_0x10cc;
            f32 field_0x10d0;
        };
    };
    f32 vertical_velocity;                 // 0x10d4
    GIZFORCE_s *gizforce_target;           // 0x10d8
    GAMEANIMOBJ_s *gizforce_target_object; // 0x10dc
    union {
        u8 pad_10e0[0x10e4 - 0x10e0];
        struct AITRIGGERSET_s *trigger_set;
        struct AITRIGGERSET_s *active_trigger_set;
    };
    void ClearAddons();
    void ClearMechObjectInterface();
    MechAddonCollection *GetAddons(bool);
    MechObjectInterface *GetMechObjectInterface();
    bool IsRunningTaskType(struct HashedKey const &);
    void KillTasks();
} GameObject;

DECOMP_ASSERT(offsetof(GameObject_s, movement_spline) == 0xe70, "GameObject movement spline offset");
DECOMP_ASSERT(offsetof(GameObject_s, movement_spline_position) == 0xe70, "GameObject spline position offset");
DECOMP_ASSERT(offsetof(GameObject_s, takeover_target) == 0xeb0, "GameObject reciprocal takeover target offset");
DECOMP_ASSERT(sizeof(GameObject_s) == 0x10e4, "GameObject size");
DECOMP_ASSERT(offsetof(GameObject_s, turn_braking) == 0xd98, "GameObject turn braking offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xdb0) == 0xdb0, "GameObject protocol-droid movement field offset");
DECOMP_ASSERT(sizeof(AIPACKET) == 0x208, "AIPACKET size");
DECOMP_ASSERT(offsetof(AIPACKET, opponent_metric) == 0xe8, "AI opponent metric offset");
DECOMP_ASSERT(offsetof(AIPACKET, target_metric_e0) == 0xe0, "AI target metric e0 offset");
DECOMP_ASSERT(offsetof(AIPACKET, target_metric_f0) == 0xf0, "AI target metric f0 offset");
static_assert(sizeof(void *) != 4 || sizeof(AIPACKET) == 0x208, "AIPACKET 32-bit size");
static_assert(sizeof(void *) != 4 || sizeof(GameObject_s) == 0x10e4, "GameObject 32-bit size");
static_assert(sizeof(void *) != 4 || offsetof(GameObject_s, hold_timer) == 0xde4, "GameObject hold timer offset");
static_assert(sizeof(void *) != 4 || offsetof(GameObject_s, previous_block_animation) == 0xe10,
              "GameObject previous block animation offset");
static_assert(sizeof(void *) != 4 || offsetof(GameObject_s, held_movement_animation) == 0xe14,
              "GameObject held movement animation offset");
static_assert(sizeof(void *) != 4 || offsetof(GameObject_s, released_movement_animation) == 0xe16,
              "GameObject released movement animation offset");
static_assert(sizeof(void *) != 4 || offsetof(AIPACKET, character_type_mask_low) == 0x12c,
              "AIPACKET character mask 32-bit offset");
DECOMP_ASSERT(offsetof(AIPACKET, alternate_script_process) == 0xcc, "AIPACKET alternate script processor offset");
DECOMP_ASSERT(offsetof(AIPACKET, opponent_object) == 0xe4, "AIPACKET opponent offset");
DECOMP_ASSERT(offsetof(AIPACKET, opponent_metric) == 0xe8, "AIPACKET opponent range offset");
DECOMP_ASSERT(offsetof(AIPACKET, pending_nearest_opponent) == 0xdc, "AIPACKET pending nearest opponent offset");
DECOMP_ASSERT(offsetof(AIPACKET, pending_nearest_metric) == 0xe0, "AIPACKET pending nearest range offset");
DECOMP_ASSERT(offsetof(AIPACKET, pending_opponent) == 0xec, "AIPACKET pending opponent offset");
DECOMP_ASSERT(offsetof(AIPACKET, pending_opponent_metric) == 0xf0, "AIPACKET pending opponent range offset");
DECOMP_ASSERT(offsetof(AIPACKET, dont_avoid_character) == 0xf4, "AIPACKET character avoidance exception offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai) + offsetof(AIPACKET, dont_avoid_character) == 0x3b4,
              "GameObject character avoidance exception offset");
DECOMP_ASSERT(offsetof(AIPACKET, movement_destination) == 0x104, "AIPACKET destination offset");
DECOMP_ASSERT(offsetof(AIPACKET, animation_override_from) == 0x126, "AIPACKET animation override source offset");
DECOMP_ASSERT(offsetof(AIPACKET, animation_override_to) == 0x128, "AIPACKET animation override target offset");
DECOMP_ASSERT(offsetof(AIPACKET, movement_target_direction) == 0x147, "AIPACKET target direction offset");
DECOMP_ASSERT(offsetof(AIPACKET, movement_target) == 0x184, "AIPACKET movement target offset");
DECOMP_ASSERT(offsetof(AIPACKET, intersection_connection) == 0x18c, "AIPACKET intersection connection offset");
DECOMP_ASSERT(offsetof(AIPACKET, intersection_target_connection) == 0x190, "AIPACKET intersection target offset");
DECOMP_ASSERT(offsetof(AIPACKET, right_diversion) == 0x194, "AIPACKET right diversion offset");
DECOMP_ASSERT(offsetof(AIPACKET, left_diversion) == 0x1a0, "AIPACKET left diversion offset");
DECOMP_ASSERT(offsetof(AIPACKET, movement_position) == 0x114, "AIPACKET movement position offset");
DECOMP_ASSERT(offsetof(AIPACKET, path_info) == 0x154, "AIPACKET path-info offset");
DECOMP_ASSERT(offsetof(AIPACKET, last_path_position) == 0x16c, "AIPACKET last path position offset");
DECOMP_ASSERT(offsetof(AIPACKET, goal_path_node) == 0x178, "AIPACKET goal-node offset");
DECOMP_ASSERT(offsetof(AIPACKET, navigation_flags) == 0x1e8, "AIPACKET navigation flags offset");
DECOMP_ASSERT(offsetof(AIPACKET, movement_stopped) == 0x13c, "AIPACKET movement stop offset");
DECOMP_ASSERT(offsetof(AIPACKET, divert_search_index) == 0x13e, "AIPACKET diversion search offset");
DECOMP_ASSERT(offsetof(AIPACKET, divert_node_index) == 0x13f, "AIPACKET diversion node offset");
DECOMP_ASSERT(offsetof(AIPACKET, movement_target_radius) == 0x1ec, "AIPACKET target-radius offset");
DECOMP_ASSERT(offsetof(AIPACKET, capabilities) == 0x1f0, "AIPACKET capabilities offset");
DECOMP_ASSERT(offsetof(AIPACKET, group) == 0x140, "AIPACKET formation group offset");
DECOMP_ASSERT(offsetof(AIPACKET, group_member_index) == 0x144, "AIPACKET formation member index offset");
DECOMP_ASSERT(offsetof(AIPACKET, group_column) == 0x145, "AIPACKET formation column offset");
DECOMP_ASSERT(offsetof(AIPACKET, group_row) == 0x146, "AIPACKET formation row offset");
DECOMP_ASSERT(offsetof(AIPACKET, fallback_path_info) == 0x1c8, "AIPACKET fallback path-info offset");
DECOMP_ASSERT(offsetof(AIPACKET, time_off_path) == 0x204, "AIPACKET off-path timer offset");
DECOMP_ASSERT(offsetof(APIOBJECT, anim_packet) == 0x08, "APIOBJECT animation packet offset");
DECOMP_ASSERT(offsetof(APIOBJECT, facing_angle) == 0x58, "APIOBJECT facing angle offset");
DECOMP_ASSERT(offsetof(APIOBJECT, velocity) == 0x68, "APIOBJECT velocity offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_velocity) == 0x74, "APIOBJECT previous velocity offset");
DECOMP_ASSERT(offsetof(APIOBJECT, last_safe_position) == 0x1b4, "APIOBJECT last safe position offset");
DECOMP_ASSERT(offsetof(APIOBJECT, field_0x1c0) == 0x1c0, "APIOBJECT alternate camera position offset");
DECOMP_ASSERT(offsetof(APIOBJECT, field_0xb8) == 0xb8, "APIOBJECT primary matrix offset");
DECOMP_ASSERT(offsetof(APIOBJECT, field_0x214) == 0x214, "APIOBJECT reset distance offset");
DECOMP_ASSERT(offsetof(APIOBJECT, water_height) == 0x21c, "APIOBJECT water height offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_animation_root) == 0x240, "APIOBJECT previous animation root offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_animation_root_time) == 0x258,
              "APIOBJECT previous animation root time offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_blend_target_root) == 0x24c, "APIOBJECT previous blend-target root offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_blend_target_root_time) == 0x25c,
              "APIOBJECT previous blend-target root time offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_animation_root_info) == 0x260,
              "APIOBJECT previous animation root-info offset");
DECOMP_ASSERT(offsetof(APIOBJECT, previous_blend_target_root_info) == 0x264,
              "APIOBJECT previous blend-target root-info offset");
DECOMP_ASSERT(offsetof(APIOBJECT, animation_root_delta) == 0x268, "APIOBJECT animation root delta offset");
DECOMP_ASSERT(offsetof(APIOBJECT, movement_direction) == 0x1fc, "APIOBJECT movement direction offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_special) == 0x208, "APIOBJECT collision special offset");
DECOMP_ASSERT(offsetof(APIOBJECT, field_0x1fa) == 0x1fa, "APIOBJECT antinode flags offset");
DECOMP_ASSERT(offsetof(APIOBJECT, object_flags) == 0x1f8, "APIOBJECT complete flag word offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_priority) == 0x28e, "APIOBJECT collision priority offset");
DECOMP_ASSERT(offsetof(APIOBJECT, resolved_collision_priority) == 0x290,
              "APIOBJECT resolved collision priority offset");
DECOMP_ASSERT(offsetof(APIOBJECT, collision_position) == 0x80, "APIOBJECT collision position offset");
DECOMP_ASSERT(offsetof(APIOBJECT, antinode_special) == 0x208, "APIOBJECT antinode special offset");
DECOMP_ASSERT(offsetof(APIOBJECT, pitch_angle) == 0x274, "APIOBJECT pitch angle offset");
DECOMP_ASSERT(offsetof(APIOBJECT, supporting_platform_id) == 0x27a, "APIOBJECT supporting platform id offset");
DECOMP_ASSERT(offsetof(APIOBJECT, movement_stuck_time) == 0x1d8, "APIOBJECT movement stuck timer offset");
DECOMP_ASSERT(offsetof(APIOBJECT, model_draw_result) == 0x284, "APIOBJECT model draw result offset");
DECOMP_ASSERT(offsetof(GameObject_s, apiobj.ai_area_mask_low) == 0x2a8, "GameObject AI area mask offset");
DECOMP_ASSERT(offsetof(GameObject_s, sock_position) == 0x660, "GameObject socket position offset");
DECOMP_ASSERT(offsetof(GameObject_s, sock_angles) == 0x7bc, "GameObject socket angles offset");
DECOMP_ASSERT(offsetof(GameObject_s, camera_screen_position) == 0xc4c, "GameObject screen position offset");
DECOMP_ASSERT(offsetof(GameObject_s, extra_layer_mask) == 0x1058, "GameObject extra layer mask offset");
DECOMP_ASSERT(offsetof(GameObject_s, contact_position) == 0x698, "GameObject contact position offset");
DECOMP_ASSERT(offsetof(GameObject_s, sabre_contact_sfx_timer) == 0xdc0, "GameObject sabre contact sound timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, contact_normal) == 0x6a4, "GameObject contact normal offset");
DECOMP_ASSERT(offsetof(GameObject_s, terrain_impact_speed) == 0x103c, "GameObject terrain impact speed offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x6b0) == 0x6b0, "GameObject terrain contact state offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x780) == 0x780, "GameObject field_0x780 offset");
DECOMP_ASSERT(offsetof(GameObject_s, takeover_source) == 0xcc4, "GameObject takeover source offset");
DECOMP_ASSERT(offsetof(GameObject_s, script_fire_target) == 0xeac, "GameObject script firing target offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xf14) == 0xf14, "GameObject character-switch preserved state offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xf20) == 0xf20, "GameObject character-switch preserved state offset");
DECOMP_ASSERT(offsetof(GameObject_s, spawn_protection_timer) == 0x100c, "GameObject spawn protection offset");
DECOMP_ASSERT(offsetof(GameObject_s, flicker_timer) == 0x1024, "GameObject flicker timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, flicker_flags) == 0xe26, "GameObject flicker flags offset");
DECOMP_ASSERT(offsetof(AIPACKET, primary_target_ref) == 0xd4, "AI primary target reference offset");
DECOMP_ASSERT(offsetof(AIPACKET, action_target_ref) == 0xe4, "AI action target reference offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x788) == 0x788, "GameObject field_0x788 offset");
DECOMP_ASSERT(offsetof(GameObject_s, trigger_set) == 0x10e0, "GameObject trigger set offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_animation) == 0x79a, "GameObject context animation offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x768) == 0x768, "GameObject interaction blend offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_animation_timer) == 0x76c, "GameObject interaction timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, airborne_action_duration) == 0x774, "GameObject interaction duration offset");
DECOMP_ASSERT(offsetof(GameObject_s, queued_context_animation) == 0x79c, "GameObject queued context animation offset");
DECOMP_ASSERT(offsetof(GameObject_s, combo_branch) == 0x79e, "GameObject combo branch offset");
DECOMP_ASSERT(offsetof(GameObject_s, combo_input_latched) == 0x7a0, "GameObject combo input latch offset");
DECOMP_ASSERT(offsetof(GameObject_s, edge_stop_requests) == 0xf05, "GameObject edge-stop request offset");
DECOMP_ASSERT(offsetof(GameObject_s, action_input_state) == 0x7a0, "GameObject packed action input offset");
DECOMP_ASSERT(offsetof(GameObject_s, landing_followup) == 0x7a1, "GameObject landing follow-up offset");
DECOMP_ASSERT(offsetof(GameObject_s, combo_stage) == 0x7a2, "GameObject combo stage offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x7a3) == 0x7a3, "GameObject camera-look selector offset");
DECOMP_ASSERT(offsetof(GameObject_s, build_button_taps) == 0x7a4, "GameObject Build-It tap count offset");
DECOMP_ASSERT(offsetof(GameObject_s, grapple_swing_phase) == 0x798, "GameObject grapple swing phase offset");
DECOMP_ASSERT(offsetof(GameObject_s, magnet_surface_angle) == 0x794, "GameObject magnet surface angle offset");
DECOMP_ASSERT(offsetof(GameObject_s, grapple_swing_degrees) == 0x7a4, "GameObject grapple swing amplitude offset");
DECOMP_ASSERT(offsetof(GameObject_s, movement_context_state) == 0x7a4, "GameObject packed movement context offset");
DECOMP_ASSERT(offsetof(GameObject_s, build_context) == 0x7a5, "GameObject Build-It context offset");
DECOMP_ASSERT(offsetof(GameObject_s, character_context) == 0x7a5, "GameObject character context offset");
DECOMP_ASSERT(offsetof(GameObject_s, action_movement_state) == 0x7a8, "GameObject action movement state offset");
DECOMP_ASSERT(offsetof(GameObject_s, external_force) == 0x738, "GameObject external force offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_destination) == 0x744, "GameObject context destination offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_position_offset) == 0x750, "GameObject context position offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_x_rotation) == 0x794, "GameObject context X rotation offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_z_rotation) == 0x798, "GameObject context Z rotation offset");
DECOMP_ASSERT(offsetof(GameObject_s, carried_object_basis) == 0x738, "GameObject carried basis offset");
DECOMP_ASSERT(offsetof(GameObject_s, carried_object_angle) == 0x796, "GameObject carried angle offset");
DECOMP_ASSERT(offsetof(GameObject_s, hit_variant) == 0x7ab, "GameObject hit variant offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_flags) == 0x7ac, "GameObject context flags offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_variant_flags) == 0x7ad, "GameObject context variant flags offset");
DECOMP_ASSERT(offsetof(GameObject_s, mini_anim_packet) == 0x7c0, "GameObject mini animation packet offset");
DECOMP_ASSERT(offsetof(GameObject_s, joint_matrices) == 0x7f4, "GameObject joint-matrix offset");
DECOMP_ASSERT(offsetof(GameObject_s, vehicle_orientation) == 0xbf4, "GameObject vehicle orientation offset");
DECOMP_ASSERT(offsetof(GameObject_s, active_trigger_set) == 0x10e0, "GameObject active trigger-set offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xcc0) == 0xcc0, "GameObject linked object offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x1014) == 0x1014, "GameObject AI override reset field offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xca8) == 0xca8, "GameObject field_0xca8 offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_throw_target) == 0xd00, "GameObject Force throw target offset");
DECOMP_ASSERT(offsetof(GameObject_s, airborne_collision_target) == 0xd10,
              "GameObject airborne collision target offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xd14) == 0xd14, "GameObject input state offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_glow_step) == 0xd88, "GameObject force glow step offset");
DECOMP_ASSERT(offsetof(GameObject_s, use_action) == 0xf0c, "GameObject use action offset");
DECOMP_ASSERT(offsetof(GameObject_s, can_use_object) == 0xf08, "GameObject can-use object offset");
DECOMP_ASSERT(offsetof(GameObject_s, use_action_frames) == 0xf0d, "GameObject use-action frames offset");
DECOMP_ASSERT(offsetof(GameObject_s, use_action_parameter) == 0xf10, "GameObject use-action parameter offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_use_volume) == 0xd94, "GameObject force volume offset");
DECOMP_ASSERT(offsetof(GameObject_s, incoming_part) == 0xca0, "GameObject incoming part offset");
DECOMP_ASSERT(offsetof(GameObject_s, collision_target) == 0xcb8, "GameObject collision target offset");
DECOMP_ASSERT(offsetof(GameObject_s, block_attacker) == 0xce8, "GameObject block attacker offset");
DECOMP_ASSERT(offsetof(GameObject_s, incoming_bolt) == 0xcf4, "GameObject incoming bolt offset");
DECOMP_ASSERT(offsetof(GameObject_s, block_cooldown) == 0xdd8, "GameObject block cooldown offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai_jump_timer) == 0xde8, "GameObject AI jump timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, route_character_id) == 0x1072, "GameObject route character offset");
DECOMP_ASSERT(offsetof(GameObject_s, route_suit_index) == 0x1074, "GameObject route suit offset");
DECOMP_ASSERT(offsetof(GameObject_s, route_start_index) == 0x108d, "GameObject route search start offset");
DECOMP_ASSERT(offsetof(GameObject_s, route_search_index) == 0x1095, "GameObject route search cursor offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai_combo_cooldown) == 0xd48, "GameObject AI combo cooldown offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_glow_position) == 0xc58, "GameObject force glow position offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_glow_candidate) == 0xd0c, "GameObject force glow candidate offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_heading) == 0xe0a, "GameObject force heading offset");
DECOMP_ASSERT(offsetof(GameObject_s, dynamic_light_id) == 0xe04, "GameObject dynamic light offset");
DECOMP_ASSERT(offsetof(GameObject_s, force_glow_kind) == 0xe2b, "GameObject force glow kind offset");
DECOMP_ASSERT(offsetof(GameObject_s, head_target) == 0xfe4, "GameObject head target offset");
DECOMP_ASSERT(offsetof(GameObject_s, head_target_priority) == 0x108c, "GameObject head priority offset");
DECOMP_ASSERT(offsetof(GameObject_s, blocked_attack_stage) == 0xe2d, "GameObject blocked stage offset");
DECOMP_ASSERT(offsetof(GameObject_s, ground_contact_grace_timer) == 0xd18,
              "GameObject ground-contact grace timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, weapon_scale) == 0xd30, "GameObject weapon scale offset");
DECOMP_ASSERT(offsetof(GameObject_s, sabre_flags) == 0xe33, "GameObject sabre flags offset");
static_assert(sizeof(void *) != 4 || offsetof(GameObject_s, sabre_flags) == 0xe33,
              "GameObject sabre flags must preserve the 32-bit ABI");
static_assert(sizeof(void *) != 4 || offsetof(GameObject_s, field_0xf00) == 0xf00,
              "GameObject update flags must preserve the 32-bit ABI");
DECOMP_ASSERT(offsetof(GameObject_s, sabre_damage) == 0xe34, "GameObject sabre damage offset");
DECOMP_ASSERT(offsetof(GameObject_s, blowup_target) == 0x784, "GameObject blowup target offset");
DECOMP_ASSERT(offsetof(GameObject_s, sabre_streaks) == 0xcc8, "GameObject sabre streaks offset");
DECOMP_ASSERT(offsetof(GameObject_s, sabre_collision_radius) == 0xd38, "GameObject sabre radius offset");
DECOMP_ASSERT(offsetof(GameObject_s, blade_states) == 0xe43, "GameObject blade states offset");
DECOMP_ASSERT(offsetof(GameObject_s, weapon_trail_offset) == 0xc40, "GameObject weapon trail offset");
DECOMP_ASSERT(offsetof(GameObject_s, weapon_scale_rate) == 0xd34, "GameObject weapon scale rate offset");
DECOMP_ASSERT(offsetof(GameObject_s, weapon_out_timer) == 0xd3c, "GameObject weapon timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, combo_input_timer) == 0xd44, "GameObject combo input timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, reset_velocity) == 0xdf8, "GameObject reset velocity offset");
DECOMP_ASSERT(offsetof(GameObject_s, pre_terrain_speed) == 0xdf0, "GameObject pre-terrain speed offset");
DECOMP_ASSERT(offsetof(GameObject_s, interaction_arrow_blend) == 0xdd4, "GameObject interaction arrow blend offset");
DECOMP_ASSERT(offsetof(GameObject_s, platform_separation_timer) == 0xdc4,
              "GameObject platform separation timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, post_terrain_speed) == 0xdf4, "GameObject post-terrain speed offset");
DECOMP_ASSERT(offsetof(GameObject_s, delayed_turn_timer) == 0xd40, "GameObject delayed turn timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, pause_input_state) == 0xd5c, "GameObject pause input state offset");
DECOMP_ASSERT(offsetof(GameObject_s, input_toggle_hold_time) == 0xda4, "GameObject toggle hold time offset");
DECOMP_ASSERT(offsetof(GameObject_s, flicker_flags) == 0xe26, "GameObject flicker flags offset");
DECOMP_ASSERT(offsetof(GameObject_s, flicker_time) == 0x1024, "GameObject flicker time offset");
DECOMP_ASSERT(offsetof(GameObject_s, nearby_floor_distance) == 0xda0, "GameObject nearby-floor offset");
DECOMP_ASSERT(offsetof(GameObject_s, fall_animation_timer) == 0xdac, "GameObject fall animation timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, movement_animation_hold_timer) == 0xd70,
              "GameObject movement animation hold timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, movement_animation_release_timer) == 0xd74,
              "GameObject movement animation release timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, pause_context_state) == 0xef4, "GameObject pause context state offset");
DECOMP_ASSERT(offsetof(GameObject_s, delayed_turn_target_angle) == 0xe08, "GameObject delayed turn target offset");
DECOMP_ASSERT(offsetof(GameObject_s, current_input_angle) == 0xe0c, "GameObject input angle offset");
DECOMP_ASSERT(offsetof(GameObject_s, movement_lean_angle) == 0xe18, "GameObject movement lean offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xe1e) == 0xe1e, "GameObject force glow index offset");
DECOMP_ASSERT(offsetof(GameObject_s, touch_task) == 0xe4c, "GameObject touch task offset");
DECOMP_ASSERT(offsetof(GameObject_s, weapon_scale_state) == 0xe32, "GameObject weapon scale state offset");
DECOMP_ASSERT(offsetof(GameObject_s, combo_alternate) == 0xe40, "GameObject alternate combo offset");
DECOMP_ASSERT(offsetof(GameObject_s, context_target_position) == 0xeb8, "GameObject context target offset");
DECOMP_ASSERT(offsetof(GameObject_s, facing_direction) == 0xf3c, "GameObject facing direction offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xf03) == 0xf03, "GameObject obstacle terrain flag offset");
DECOMP_ASSERT(offsetof(GameObject_s, jump_destination_distance) == 0xedc,
              "GameObject jump destination distance offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xf02) == 0xf02, "GameObject Force repeat flag offset");
DECOMP_ASSERT(offsetof(GameObject_s, in_narrow_socket) == 0xe3b, "GameObject narrow socket offset");
DECOMP_ASSERT(offsetof(GameObject_s, surface_normal) == 0xf30, "GameObject surface normal offset");
DECOMP_ASSERT(offsetof(GameObject_s, suspension) == 0x718, "GameObject suspension offset");
DECOMP_ASSERT(sizeof(((GameObject_s *)0)->suspension) == 0x20, "GameObject suspension size");
DECOMP_ASSERT(offsetof(GameObject_s, suspension_normal) == 0xc88, "GameObject suspension normal offset");
DECOMP_ASSERT(offsetof(GameObject_s, doomed_escape_locator) == 0xf20, "GameObject doomed escape locator offset");
DECOMP_ASSERT(offsetof(GameObject_s, target_velocity) == 0xf24, "GameObject target velocity offset");
DECOMP_ASSERT(offsetof(GameObject_s, character_bottom) == 0xffc, "GameObject lower bound offset");
DECOMP_ASSERT(offsetof(GameObject_s, character_top) == 0x1000, "GameObject upper bound offset");
DECOMP_ASSERT(offsetof(GameObject_s, previous_movement_angle) == 0x106c, "GameObject previous movement angle offset");
DECOMP_ASSERT(offsetof(GameObject_s, apiobj.collision_radius) == 0xac, "GameObject collision radius offset");
DECOMP_ASSERT(offsetof(GameObject_s, apiobj.collision_min) == 0x178, "GameObject collision bounds offset");
DECOMP_ASSERT(offsetof(GameObject_s, apiobj.start_position) == 0x8c, "GameObject previous frame position offset");
DECOMP_ASSERT(offsetof(GameObject_s, joint_matrices[0].m30) == 0x824, "GameObject pivot point position offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai.terrain_origin) == 0x408, "GameObject terrain origin offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai.path_info) == 0x414, "GameObject AI path-info offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai.reset_position) == 0x3c4, "GameObject AI reset-position offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai_respawn_count) == 0x4b8, "GameObject AI respawn-count offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai_spawn_delay) == 0x4bc, "GameObject AI spawn-delay offset");
DECOMP_ASSERT(offsetof(GameObject_s, collision_y_scale) == 0x1010, "GameObject collision Y scale offset");
DECOMP_ASSERT(offsetof(GameObject_s, cable) == 0x104c, "GameObject cable offset");
DECOMP_ASSERT(offsetof(GameObject_s, animation_speed_multiplier) == 0x1040,
              "GameObject animation movement speed offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x105c) == 0x105c, "GameObject terrain flags offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x1084) == 0x1084, "GameObject surface reset flag offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x1088) == 0x1088, "GameObject matrix selector offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0x10b8) == 0x10b8, "GameObject snake data offset");
DECOMP_ASSERT(offsetof(GameObject_s, move_override) == 0x10bc, "GameObject movement override offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai_elapsed_time) == 0x10c4, "GameObject AI elapsed-time offset");
DECOMP_ASSERT(offsetof(GameObject_s, ai_update_distance) == 0x1028, "GameObject AI update-distance offset");
DECOMP_ASSERT(offsetof(GameObject_s, character_shadows) == 0x6b4, "GameObject character-shadow offset");
DECOMP_ASSERT(offsetof(GameObject_s, shadow_opacity) == 0x102c, "GameObject shadow-opacity offset");
DECOMP_ASSERT(offsetof(GameObject_s, shadow_radius) == 0x1030, "GameObject shadow-radius offset");
DECOMP_ASSERT(offsetof(GameObject_s, shadow_joint_mask) == 0x1066, "GameObject shadow-joint-mask offset");
DECOMP_ASSERT(offsetof(GameObject_s, vertical_velocity) == 0x10d4, "GameObject vertical velocity offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xd24) == 0xd24, "GameObject model-origin state offset");
DECOMP_ASSERT(offsetof(GameObject_s, timer_d28) == 0xd28, "GameObject D28 timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, timer_d50) == 0xd50, "GameObject D50 timer offset");
DECOMP_ASSERT(offsetof(GameObject_s, flicker_flags) == 0xe26, "GameObject flicker flags offset");
DECOMP_ASSERT(offsetof(GameObject_s, communicate_blend) == 0xd2c, "GameObject communicate blend offset");
DECOMP_ASSERT(offsetof(GameObject_s, thrust_effect_scale) == 0xdcc, "GameObject thrust-effect scale offset");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xefd) == 0xefd, "GameObject character flags offset");
DECOMP_ASSERT(offsetof(GameObject_s, terrain_origin_floor_offset) == 0xd7c, "GameObject terrain-origin floor offset");
DECOMP_ASSERT(offsetof(GameObject_s, gizforce_target) == 0x10d8, "GameObject Force target offset");
DECOMP_ASSERT(offsetof(GameObject_s, gizforce_target_object) == 0x10dc, "GameObject Force object target offset");

typedef struct GameObject_s GameObject_s;

DECOMP_ASSERT(offsetof(GameObject_s, movement_spline_position) == 0xe70, "GameObject spline position offset");
DECOMP_ASSERT(offsetof(GameObject_s, movement_spline_offset) == 0xe90, "GameObject spline offset");

DECOMP_ASSERT(offsetof(GameObject_s, launch_origin) == 0x744, "GameObject launch origin offset");
DECOMP_ASSERT(offsetof(GameObject_s, big_jump_height) == 0xf18, "GameObject big jump height offset");
DECOMP_ASSERT(offsetof(GameObject_s, zipup_entry_position) == 0x738, "GameObject zipup entry offset");
DECOMP_ASSERT(offsetof(GameObject_s, zipup_start_position) == 0x744, "GameObject zipup start offset");
DECOMP_ASSERT(offsetof(GameObject_s, zipup_swing_position) == 0x750, "GameObject zipup swing offset");
DECOMP_ASSERT(offsetof(GameObject_s, zipup_landing_position) == 0x75c, "GameObject zipup landing offset");

DECOMP_ASSERT(offsetof(GameObject_s, saved_position) == 0x10c8, "GameObject saved position offset");

DECOMP_ASSERT(offsetof(GameObject_s, run_speed_override) == 0xee0, "GameObject run speed override offset");
DECOMP_ASSERT(offsetof(GameObject_s, walk_speed_override) == 0xee4, "GameObject walk speed override offset");

DECOMP_ASSERT(offsetof(GameObject_s, carried_object_drop_position) == 0x75c, "GameObject carry drop position offset");
