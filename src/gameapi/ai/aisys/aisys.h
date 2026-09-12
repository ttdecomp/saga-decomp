#pragma once

#include "gameapi/ai/aisys/aipath.h"
#include "gameapi/ai/aisys/aiscript_types.h"
#include "legoapi/items/base/apiobject.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuhspecial.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/nuvec.h"

struct AIMESSAGESYS_s;
struct AIMESSAGE_s;

typedef i32 (*PREPARINGSPECIALMOVEFN)(AIPACKET_s *, APIOBJECT_s *, i32);
extern PREPARINGSPECIALMOVEFN PreparingForSpecialMoveFn;
i32 TryToTeleportToNextNode(GameObject_s *, AIPATHNODE_s *, i32);
void SetSpecialMove(GameObject_s *, AIPATHNODE_s *, AIPATHNODE_s *, char);
void ClearSpecialMove(GameObject_s *);

typedef struct AIPATHCNX_s {
    union {
        u32 traversal_flags[2];
        struct {
            u32 node_a;
            u32 node_b;
        };
    };
    union {
        u32 original_traversal_flags[2];
        struct {
            u32 previous_node_a;
            u32 previous_node_b;
        };
    };
    union {
        u8 node_indices[2];
        struct {
            u8 direction_a;
            u8 direction_b;
        };
    };
    union {
        i16 rotation;
        i16 flags;
    };
    union {
        u16 route_mask;
        i16 game_flags;
    };
    u8 open;
    u8 last_search_checksum;
    union {
        f32 distance;
        f32 width;
    };
    union {
        f32 horizontal_distance;
        f32 cost;
    };
    f32 max_horizontal_distance;
} AIPATHCNX;

typedef struct AIROUTE_s {
} AIROUTE;

typedef struct AIANTINODE_s AIANTINODE;

typedef struct AIPATHNODE_s {
    char *name;
    NUVEC position;
    f32 radius;
    f32 radius_squared;
    f32 min_height;
    f32 min_height_offset;
    f32 max_height;
    f32 max_height_offset;
    u8 connection_count;
    u8 flags;
    u8 has_special;
    u8 runtime_flags;
    i16 path_flags;
    u8 distance_cache_nodes[2];
    u8 special_route_index;
    u8 padding_0x31[3];
    AIPATHCNX **connections;
    f32 distance_cache[2];
    union {
        nuhspecial_s special_handle;
        struct {
            void *special_scene;
            void *special;
            void *display_special;
        };
    };
    NUVEC special_position;
    u16 route_membership_mask;
    u16 route_boundary_mask;
} AIPATHNODE;

typedef struct AIPATHROUTE_s {
    char *name;
    u8 *node_routes;
    u8 *node_directions;
    u8 *exit_nodes;
    u8 route_count;
    u8 exit_node_count;
    u8 padding_0x12[2];
    u8 **route_nodes;
    union SAGA_HOST_PACKED_ALIGN4 {
        u32 character_mask[4];
        u64 character_masks[2];
    };
} AIPATHROUTE;

typedef struct AIPATHSPECIALROUTE_s {
    u8 path_count;
    u8 padding_0x01[3];
    struct AIPATH_s **paths;
} AIPATHSPECIALROUTE;

typedef struct AIPATHNODELINK_s {
    u8 node_index;
    u8 padding_0x01;
    i16 special_route_index;
} AIPATHNODELINK;

typedef struct AIPATH_s {
    char name[0x10];
    u8 node_count;
    u8 flags;
    u16 connection_count;
    u8 route_count;
    u8 index;
    u8 padding_0x16[2];
    // Per-frame path bookkeeping. Dynamic special nodes are updated once per
    // bit, while characters mark the node volume they currently occupy.
    u8 updated_node_bits[0x20];
    u8 previous_inside_node_bits[0x20];
    u8 inside_node_bits[0x20];
    u8 search_checksum;
    u8 search_reset_node;
    u8 special_route_count;
    u8 padding_0x7b;
    AIPATHNODE *nodes;
    AIPATHCNX *connections;
    u8 **route_matrix;
    AIPATHROUTE *routes;
    AIPATHNODELINK *special_routes;
    NUVEC bounds_min;
    NUVEC bounds_max;
} AIPATH;

enum AIPATH_CONNECTION_FLAGS : u32 {
    // Bit 29 is connection metadata and does not request a character capability.
    AIPATH_CONNECTION_FLAG_NO_CAPABILITY_REQUIRED = 0x20000000u,
    AIPATH_CONNECTION_CAPABILITY_MASK = 0xdfffffffu,
    AIPATH_CONNECTION_SPECIAL_MASK = 0xd8000000u,
    // Recomputed when either endpoint is attached to a moving special.
    AIPATH_CONNECTION_FLAG_DYNAMIC_TOO_LONG = 0x08000000u,
    AIPATH_CONNECTION_FLAG_SPECIAL_UNAVAILABLE = 0x10000000u,
    // Endpoint flags with these high bits require the packet's route to be
    // selected again instead of accepting the connection directly.
    AIPATH_CONNECTION_FLAG_RESELECT_ROUTE = 0x98000000u,
};

typedef struct AIPATHSYS_s {
    u8 path_count;
    u8 padding_0x01;
    u16 special_route_count;
    AIPATH **paths;
    AIPATH *active_path;
    AIPATHSPECIALROUTE *special_routes;
} AIPATHSYS;

typedef struct AILOCATORSET_s {
    char name[0x10];
    i8 locator_count;
    u8 padding_0x11[3];
    u8 *locator_entries;
    u8 *assigned;
} AILOCATORSET;

typedef struct AIAREA_s {
    char name[0x10];
    union {
        NUVEC position;
        struct {
            // Legacy loader aliases retained while the AI file parser is
            // reconstructed.  These three values are the area's origin.
            f32 min_x;
            f32 min_y;
            f32 min_z;
        };
    };
    union {
        struct {
            f32 half_width;
            f32 height;
            f32 half_depth;
        };
        struct {
            // Legacy loader aliases for the three local-space extents.
            f32 max_x;
            f32 max_y;
            f32 max_z;
        };
    };
    union {
        i16 rotation;
        i16 flags;
    };
    u8 runtime_flags;
    u8 game_flags;
    u8 padding_0x2c[8];
    struct AISYS_s *system;
    u8 padding_0x38[4];
} AIAREA;

enum AIAREA_RUNTIME_FLAGS : u8 {
    AIAREA_RUNTIME_PLAYER_PRESENT = 0x01,
    AIAREA_RUNTIME_OBJECT_STATE_CLEAR = 0x02,
    AIAREA_RUNTIME_OBJECT_STATE_SET = 0x04,
    AIAREA_RUNTIME_CHARACTER_SLOT_SEEN = 0x08,
};

typedef struct AICREATURE_s {
    char name[0x10];

    char script_name[0x10];

    NUVEC pos;
    NUANG y_rot;

    AIPATHINFO path_info;

    i32 flags;

    u8 set;

    i16 type;

    u8 count;
    u8 count_across;

    u8 padding_0x52[6];

    u32 active_mask; // 0x58, one enabled bit per spawn entry

    f32 x_spacing;
    f32 z_spacing;

    f32 script_params[4];

    AIAREA *activate_area;
    AIAREA *area;

    AILOCATOR *locator;
    AILOCATOR *respawn_locator;

    u8 activation_difficulty;

    char min_respawn_count;
    char max_respawn_count;

    u8 activate_type;

    f32 min_respawn_time;
    f32 max_respawn_time;

    f32 start_stagger;

    f32 view_distance;
    f32 hear_distance;

    f32 max_view_height;
    f32 min_view_height;
} AICREATURE;

typedef struct AIROW_s {
    AIPATHINFO path_info;

    NUVEC pos;
    NUANG y_rot;

    AIPATHCNX *next_connection;
    u8 next_direction;

    u8 is_alive;

    char padding[2];

    u32 is_clockwise : 1;
    u32 is_turning : 1;
} AIROW;

typedef struct AIGROUP_s {
    APIOBJECT *leader;

    i16 rotation_speed;

    u8 row_count;
    u8 member_count;
    u8 count_across;

    APIOBJECT *members[16];

    i32 member_is_alive;

    u32 is_used : 1;
    u32 can_respawn : 1;
    u32 is_reversed : 1;
    u32 is_in_formation : 1;
    u32 is_row_turning : 1;

    AIROW rows[4];

    f32 radius;
    f32 x_spacing;
    f32 z_spacing;
    f32 max_speed;
} AIGROUP;

struct AIANTINODE_s {
    NUVEC position;
    f32 radius;
    union {
        f32 min_y;
        f32 height;
    };
    f32 min_y_offset;
    union {
        f32 max_y;
        f32 max_height;
    };
    f32 max_y_offset;
    union {
        nuhspecial_s special_handle;
        struct {
            void *special_scene;
            void *special;
            void *display_special;
        };
    };
    NUVEC special_position;
    i32 flags;
    i32 rotation_offset;
    f32 base_radius;
    f32 base_height;
    u8 enabled;
    u8 game_flags;
    u8 type;
    u8 has_special;
    union SAGA_HOST_PACKED_ALIGN4 {
        struct {
            u8 special_type;
            u8 padding_0x4d[7];
        };
        u32 user_data[2];
        u64 excluded_character_types;
    };
};

enum AIPATHNODE_RUNTIME_FLAGS : u8 {
    AIPATHNODE_RUNTIME_POSITION_CHANGED = 0x02,
    AIPATHNODE_RUNTIME_SPECIAL_UNAVAILABLE = 0x04,
};

typedef struct AISYS_s {
    void *storage;
    VARIPTR storage_end;
    VARIPTR storage_cursor;
    i32 storage_size;

    AIPATHSYS *path_sys;

    i32 creature_count;
    AICREATURE *creatures;

    NULISTHDR scripts;

    i32 locator_count;
    AILOCATOR *locators;

    i32 locator_set_count;
    AILOCATORSET *locator_sets;

    i32 area_count;
    AIAREA *areas;

    AIGROUP groups[16];

    i32 antinode_count;
    AIANTINODE *antinodes;

    u8 next_area_check;
    u8 goody_idx;

    i16 has_done_reset : 1;
    i16 unknown_flag_2 : 1;
    i16 unknown_flag_4 : 1;

    // The AI system for game-specific logic.
    void *game_sys;

    APIOBJECT *player_1;
    APIOBJECT *player_2;

    NUGSCN *scene;
} AISYS;
DECOMP_ASSERT(offsetof(AISYS, player_1) == 0x138c, "AISYS first player offset");
DECOMP_ASSERT(offsetof(AISYS, player_2) == 0x1390, "AISYS second player offset");

DECOMP_ASSERT(sizeof(AICREATURE) == 0xa4, "AICREATURE size");
DECOMP_ASSERT(sizeof(AIAREA) == 0x3c, "AIAREA size");
DECOMP_ASSERT(offsetof(AIAREA, runtime_flags) == 0x2a, "AIAREA runtime flags offset");
DECOMP_ASSERT(sizeof(AILOCATOR) == 0x3c, "AILOCATOR size");
DECOMP_ASSERT(offsetof(AILOCATOR, position) == 0x10, "AILOCATOR position offset");
DECOMP_ASSERT(offsetof(AILOCATOR, flags) == 0x1c, "AILOCATOR rotation offset");
DECOMP_ASSERT(offsetof(AILOCATOR, path_info) == 0x20, "AILOCATOR path cursor offset");
DECOMP_ASSERT(offsetof(AILOCATOR, path_info.direction) == 0x28, "AILOCATOR path direction offset");
DECOMP_ASSERT(offsetof(AILOCATOR, path_info.dist) == 0x30, "AILOCATOR path distance offset");
DECOMP_ASSERT(offsetof(AILOCATOR, path_info.width) == 0x34, "AILOCATOR path width offset");
DECOMP_ASSERT(offsetof(AILOCATOR, locator_flags) == 0x38, "AILOCATOR flags offset");
DECOMP_ASSERT(sizeof(AILOCATORSET) == 0x1c, "AILOCATORSET size");
DECOMP_ASSERT(sizeof(AIPATHCNX) == 0x24, "AIPATHCNX size");
DECOMP_ASSERT(sizeof(AIPATHNODE) == 0x5c, "AIPATHNODE size");
DECOMP_ASSERT(offsetof(AIPATHNODE, runtime_flags) == 0x2b, "AIPATHNODE runtime flags offset");
DECOMP_ASSERT(offsetof(AIPATHNODE, special_handle) == 0x40, "AIPATHNODE special handle offset");
DECOMP_ASSERT(sizeof(AIPATHROUTE) == 0x28, "AIPATHROUTE size");
DECOMP_ASSERT(offsetof(AIPATHROUTE, character_masks) == 0x18, "AIPATHROUTE character masks offset");
DECOMP_ASSERT(sizeof(AIPATHNODELINK) == 0x04, "AIPATHNODELINK size");
DECOMP_ASSERT(offsetof(AIPATHNODELINK, node_index) == 0x00, "AIPATHNODELINK node index offset");
DECOMP_ASSERT(offsetof(AIPATHNODELINK, special_route_index) == 0x02, "AIPATHNODELINK route index offset");
DECOMP_ASSERT(offsetof(AIPATHNODE, special_route_index) == 0x30, "AIPATHNODE special route index offset");
DECOMP_ASSERT(offsetof(AIPATHNODE, route_membership_mask) == 0x58, "AIPATHNODE route membership offset");
DECOMP_ASSERT(offsetof(AIPATHNODE, route_boundary_mask) == 0x5a, "AIPATHNODE route boundary offset");
DECOMP_ASSERT(offsetof(AIPATHROUTE, node_routes) == 0x04, "AIPATHROUTE node mapping offset");
DECOMP_ASSERT(offsetof(AIPATHROUTE, exit_nodes) == 0x0c, "AIPATHROUTE exit nodes offset");
DECOMP_ASSERT(offsetof(AIPATHROUTE, exit_node_count) == 0x11, "AIPATHROUTE exit count offset");
DECOMP_ASSERT(offsetof(AIPATHROUTE, route_nodes) == 0x14, "AIPATHROUTE connection matrix offset");
DECOMP_ASSERT(sizeof(AIPATH) == 0xa8, "AIPATH size");
DECOMP_ASSERT(offsetof(AIPATH, updated_node_bits) == 0x18, "AIPATH updated-node bits offset");
DECOMP_ASSERT(offsetof(AIPATH, inside_node_bits) == 0x58, "AIPATH occupied-node bits offset");
DECOMP_ASSERT(offsetof(AIPATH, previous_inside_node_bits) == 0x38, "AIPATH previous occupied-node bits offset");
DECOMP_ASSERT(sizeof(AIPATHSYS) == 0x10, "AIPATHSYS size");
DECOMP_ASSERT(sizeof(AIANTINODE) == 0x54, "AIANTINODE size");
DECOMP_ASSERT(offsetof(AIANTINODE, user_data) == 0x4c, "AIANTINODE exclusion mask offset");
DECOMP_ASSERT(offsetof(AIANTINODE, radius) == 0xc, "AIANTINODE radius offset");
DECOMP_ASSERT(offsetof(AIANTINODE, min_y) == 0x10, "AIANTINODE lower bound offset");
DECOMP_ASSERT(offsetof(AIANTINODE, max_y) == 0x18, "AIANTINODE upper bound offset");
DECOMP_ASSERT(offsetof(AIANTINODE, special_handle) == 0x20, "AIANTINODE special handle offset");
DECOMP_ASSERT(offsetof(AIANTINODE, rotation_offset) == 0x3c, "AIANTINODE rotation offset");
DECOMP_ASSERT(offsetof(AIANTINODE, excluded_character_types) == 0x4c, "AIANTINODE exclusion mask offset");
DECOMP_ASSERT(offsetof(AICREATURE, type) == 0x4e, "AICREATURE type offset");
DECOMP_ASSERT(offsetof(AICREATURE, count) == 0x50, "AICREATURE count offset");
DECOMP_ASSERT(offsetof(AICREATURE, active_mask) == 0x58, "AICREATURE active-mask offset");
DECOMP_ASSERT(offsetof(AICREATURE, area) == 0x78, "AICREATURE area offset");
DECOMP_ASSERT(offsetof(AICREATURE, start_stagger) == 0x90, "AICREATURE stagger offset");
DECOMP_ASSERT(sizeof(AIROW) == 0x34, "AIROW size");
DECOMP_ASSERT(offsetof(AIROW, pos) == 0x18, "AIROW position offset");
DECOMP_ASSERT(offsetof(AIROW, is_alive) == 0x2d, "AIROW live-member mask offset");
DECOMP_ASSERT(sizeof(AIGROUP) == 0x134, "AIGROUP size");
DECOMP_ASSERT(offsetof(AIGROUP, member_is_alive) == 0x4c, "AIGROUP live-member mask offset");
DECOMP_ASSERT(sizeof(AIROW) == 0x34, "AIROW size");
DECOMP_ASSERT(offsetof(AIROW, pos) == 0x18, "AIROW position offset");
DECOMP_ASSERT(offsetof(AIROW, y_rot) == 0x24, "AIROW rotation offset");
DECOMP_ASSERT(offsetof(AIGROUP, row_count) == 0x6, "AIGROUP row count offset");
DECOMP_ASSERT(offsetof(AIGROUP, rows) == 0x54, "AIGROUP rows offset");
DECOMP_ASSERT(offsetof(AIGROUP, count_across) == 0x8, "AIGROUP count-across offset");
DECOMP_ASSERT(offsetof(AIGROUP, x_spacing) == 0x128, "AIGROUP spacing offset");
DECOMP_ASSERT(sizeof(AISYS) == 0x1398, "AISYS size");
DECOMP_ASSERT(offsetof(AISYS, creature_count) == 0x14, "AISYS creature-count offset");
DECOMP_ASSERT(offsetof(AISYS, creatures) == 0x18, "AISYS creatures offset");
DECOMP_ASSERT(offsetof(AISYS, groups) == 0x3c, "AISYS groups offset");
DECOMP_ASSERT(offsetof(AISYS, scene) == 0x1394, "AISYS scene offset");

typedef struct AIPACKET_s AIPACKET;

typedef i32 AIACTIONFN(AISYS *, AISCRIPTPROCESS *, AIPACKET *, char **, i32, i32, f32);
typedef f32 AICONDITIONFN(AISYS *, AISCRIPTPROCESS *, AIPACKET *, char *, void *);
typedef void *AICONDITIONINITFN(AISYS *, char *, AISCRIPT *);

typedef struct AISCRIPTACTIONDEF_s {
    char *name;
    AIACTIONFN *eval_fn;
    char unknown;
    char is_game_action;
    i16 idx;
} AIACTIONDEF;

typedef struct AISCRIPTCONDITIONDEF_s {
    char *name;
    AICONDITIONFN *eval_fn;
    AICONDITIONINITFN *init_fn;
} AICONDITIONDEF;

// Shared registry indices let each callback-owning translation unit install
// its local callbacks without changing their linkage.  Keep these values in
// lockstep with the table definitions.
enum AISCRIPT_REGISTRY_INDEX {
    API_AI_ACTION_IDLE = 0,
    API_AI_ACTION_RESET_TIMER = 2,
    API_AI_ACTION_FOLLOW_PLAYER = 11,
    API_AI_ACTION_IGNORE_WALL_SPLINES = 17,
    API_AI_ACTION_FOLLOW_OPPONENT = 13,
    API_AI_ACTION_SET_VIEW_DISTANCE = 27,
    API_AI_ACTION_SET_MAX_VIEW_HEIGHT = 28,
    API_AI_ACTION_SET_MIN_VIEW_HEIGHT = 29,
    API_AI_ACTION_SET_HEAR_DISTANCE = 30,
    API_AI_ACTION_SET_MOVE_RADIUS = 31,
    API_AI_ACTION_GO_TO_LOCATOR = 35,
    API_AI_ACTION_SET_LOCATOR = 36,
    API_AI_ACTION_FOLLOW_PATH = 38,
    API_AI_ACTION_OVERRIDE_ANIMATION = 40,
    API_AI_ACTION_RETURN_TO_STATE = 52,
    API_AI_CONDITION_LOCATOR_RANGE = 2,
    API_AI_CONDITION_LOCATOR_RANGE_XZ = 3,
    API_AI_CONDITION_LOCATOR_RANGE_Y = 4,
    API_AI_CONDITION_TIMER = 5,
    API_AI_CONDITION_RANDOM = 6,
    API_AI_CONDITION_GOT_LOCATOR = 7,
    API_AI_CONDITION_GOT_LOCATOR_SET = 8,
    API_AI_CONDITION_CURRENT_LOCATOR_IS = 9,
    API_AI_CONDITION_PLAYER_RANGE = 11,
    API_AI_CONDITION_GOT_OPPONENT = 25,
    API_AI_CONDITION_OPPONENT_IS_A_THREAT = 26,
    API_AI_CONDITION_OPPONENT_RANGE = 28,
    API_AI_CONDITION_NEAREST_OPPONENT_RANGE = 29,
    API_AI_CONDITION_YAW_TO_OPPONENT = 30,
    API_AI_CONDITION_OPPONENT_BELOW = 31,
    API_AI_CONDITION_ORIGIN_RANGE = 32,

    LEGO_AI_ACTION_SET_DOOMED_ESCAPE_LOCATOR = 11,
    LEGO_AI_ACTION_SNAP_TO_SOCK_POSITION = 13,
    LEGO_AI_ACTION_CAN_SHOOT_OFF_SCREEN = 17,
    LEGO_AI_ACTION_SET_BOLTS_DONT_GET_DEFLECTED_BACK = 22,
    LEGO_AI_ACTION_CAN_SHOOT_OBSTRUCTIONS = 23,
    LEGO_AI_ACTION_CAN_HIT_FORCE_OBJECTS = 32,
    LEGO_AI_ACTION_PLAYER_SPEEDER_HACK = 34,
    LEGO_AI_ACTION_CHAR_CLIP_TO_BLOB_SHADOWS = 47,
    LEGO_AI_ACTION_DEFLECT_PLAYERS_PART = 55,
    LEGO_AI_ACTION_SET_AI_OVERRIDE_CONTROL = 59,
    LEGO_AI_ACTION_SET_LAST_SAFE_PATH_POS = 60,
    LEGO_AI_ACTION_DONT_SET_STOPPED_FLAG = 62,
    LEGO_AI_ACTION_PRESS_SPECIAL_BUTTON = 63,
    LEGO_AI_ACTION_PRESS_ACTION_BUTTON = 65,
    LEGO_AI_ACTION_DONT_AVOID_CHARACTER = 69,
    LEGO_AI_ACTION_SET_ZERO_ACCELERATION = 74,
    LEGO_AI_ACTION_CREATE_CREATURES = 78,
    LEGO_AI_ACTION_CREATE_SPLINE_CREATURES = 80,
    LEGO_AI_ACTION_FOLLOW_CHARACTER = 131,
    LEGO_AI_ACTION_FOLLOW_PLAYER = 132,
    LEGO_AI_ACTION_MOVE_FORWARD = 133,
    LEGO_AI_ACTION_SET_CURRENT_SPEED = 82,
    LEGO_AI_ACTION_USE_CURRENT_SPEED = 94,
    LEGO_AI_ACTION_SET_MAX_MOVEMENT_RANGE = 95,
    LEGO_AI_ACTION_SET_DEFAULT_MOVEMENT_RANGE = 96,
    LEGO_AI_ACTION_SET_GRAVITY_HEIGHT = 97,
    LEGO_AI_ACTION_ALWAYS_TRIGGER_OBSTACLE = 136,
    LEGO_AI_ACTION_CAN_TRIGGER_OBSTACLE = 137,
    LEGO_AI_ACTION_PLAY_GIZ_OBSTACLE = 138,
    LEGO_AI_ACTION_PLAY_OBSTACLE = 139,
    LEGO_AI_ACTION_SET_OBSTACLE_TO_END = 141,
    LEGO_AI_ACTION_RELEASE_LOCATOR = 147,
    LEGO_AI_ACTION_ASSIGN_LOCATOR = 148,
    LEGO_AI_ACTION_GET_LOCATOR_FROM_SET = 149,
    LEGO_AI_ACTION_MOVE_AWAY_FROM_LAST_ATTACKER = 150,
    LEGO_AI_ACTION_SET_LAST_ATTACKER = 153,
    LEGO_AI_ACTION_SET_USE_ONE_AT_ONCE = 179,
    LEGO_AI_ACTION_GIZMO_SET_VISIBILITY = 191,
    LEGO_AI_ACTION_NEW_SEBULBA = 207,
    LEGO_AI_ACTION_SELECT_RANDOM_SPLINE = 79,
    LEGO_AI_ACTION_LAUNCH = 81,
    LEGO_AI_ACTION_SET_RUN_SPEED = 83,
    LEGO_AI_ACTION_SET_WALK_SPEED = 84,
    LEGO_AI_ACTION_SET_HIT_POINTS = 85,
    LEGO_AI_ACTION_SET_SHIELD_HIT_POINTS = 86,
    LEGO_AI_ACTION_SET_MESSAGE = 87,
    LEGO_AI_ACTION_COPY_MESSAGE = 88,
    LEGO_AI_ACTION_SET_SCRIPT_PARAM = 89,
    LEGO_AI_ACTION_ADD_PART = 90,
    LEGO_AI_ACTION_ADD_PART_DEBRIS = 91,

    LEGO_AI_CONDITION_OFF_SCREEN_TIMER = 7,
    LEGO_AI_CONDITION_SPECIAL_AT_START = 31,
    LEGO_AI_CONDITION_FORCE_COMPLETE = 56,
    LEGO_AI_CONDITION_FORCE_FINISHED = 57,
    LEGO_AI_CONDITION_CATEGORY_IS = 71,
    LEGO_AI_CONDITION_I_AM_A = 62,
    LEGO_AI_CONDITION_PARTY_UNDER_COVER = 44,
    LEGO_AI_CONDITION_PREFERS_BRAWLING = 4,
    LEGO_AI_CONDITION_NEAREST_PARTY_RANGE = 151,
    LEGO_AI_CONDITION_NEAREST_PARTY_XZ_RANGE = 152,
    LEGO_AI_CONDITION_OPPONENT_IS_A = 63,
    LEGO_AI_CONDITION_PLAYER_1_IS = 74,
    LEGO_AI_CONDITION_EITHER_PLAYER_IS = 73,
    LEGO_AI_CONDITION_PLAYER_2_IS = 75,
    LEGO_AI_CONDITION_LOCATOR_ON_SCREEN = 18,
    LEGO_AI_CONDITION_TURRET_ALIVE = 54,
    LEGO_AI_CONDITION_ACTIVE = 2,
    LEGO_AI_CONDITION_DEBUG = 1,
    LEGO_AI_CONDITION_GLYN_TEST = 0,
    LEGO_AI_CONDITION_HOVER_PHASE = 21,
    LEGO_AI_CONDITION_SPAWN_COUNT = 16,
    LEGO_AI_CONDITION_BEEN_ALERTED = 14,
    LEGO_AI_CONDITION_ON_GROUND = 13,
    LEGO_AI_CONDITION_COLLIDING = 28,
    LEGO_AI_CONDITION_GOT_VICTIM = 143,
    LEGO_AI_CONDITION_MY_SET = 145,
    LEGO_AI_CONDITION_BEING_TOWED = 165,
    LEGO_AI_CONDITION_BEEN_HIT = 20,
    LEGO_AI_CONDITION_IS_ALIVE = 5,
    LEGO_AI_CONDITION_ON_OBJECT = 8,
    LEGO_AI_CONDITION_CONTEXT = 78,
    LEGO_AI_CONDITION_IN_SWAMP = 171,
    LEGO_AI_CONDITION_GOT_GUN = 3,
    LEGO_AI_CONDITION_SIDE = 150,
    LEGO_AI_CONDITION_X_POS = 24,
    LEGO_AI_CONDITION_Y_POS = 25,
    LEGO_AI_CONDITION_Z_POS = 26,
    LEGO_AI_CONDITION_TAKE_OVER_RANGE = 137,
    LEGO_AI_CONDITION_TAKE_OVER_TARGET_IN_TRIGGER_AREA = 138,
    LEGO_AI_CONDITION_IN_SAME_TRIGGER_AREA_AS_NEAREST_PLAYER = 172,
    LEGO_AI_CONDITION_EITHER_PLAYER_LOCATOR_RANGE_XZ = 12,
    LEGO_AI_CONDITION_EITHER_PLAYER_ON_OBJECT = 11,
    LEGO_AI_CONDITION_CHARACTER_EXISTS = 131,
    LEGO_AI_CONDITION_BUILD_IT_COMPLETE = 60,
    LEGO_AI_CONDITION_HAS_TAKE_OVER = 136,
    LEGO_AI_CONDITION_SHOP_ACTIVE = 149,
    LEGO_AI_CONDITION_SCREEN_WIPE = 146,
    LEGO_AI_CONDITION_CAN_HEAR_RADIO = 164,
    LEGO_AI_CONDITION_BEHIND_CAMERA = 17,
    LEGO_AI_CONDITION_NUM_BADDIES = 82,
    LEGO_AI_CONDITION_BLOWUP_BLOWNUP = 61,
    LEGO_AI_CONDITION_ANIM_SPEED_MUL = 161,
    LEGO_AI_CONDITION_GIZMO_OUTPUT_0 = 155,
    LEGO_AI_CONDITION_GIZMO_OUTPUT_1 = 156,
    LEGO_AI_CONDITION_GIZMO_OUTPUT_2 = 157,
    LEGO_AI_CONDITION_GIZMO_OUTPUT_3 = 158,
    LEGO_AI_CONDITION_GIZMO_VISIBILITY = 159,
    LEGO_AI_CONDITION_FLOW_BOX_COMPLETE = 163,
    LEGO_AI_CONDITION_AREA_COMPLETE = 169,
    LEGO_AI_CONDITION_I_AM_A_GOODIE_BADDIE = 69,
    LEGO_AI_CONDITION_I_AM_A_GOODY = 66,
    LEGO_AI_CONDITION_I_AM_A_BADDY = 67,
    LEGO_AI_CONDITION_I_AM_A_NEUTRAL = 68,
    LEGO_AI_CONDITION_I_AM_A_PARTY_CHARACTER = 70,
    LEGO_AI_CONDITION_PLAYER_CATEGORY_IS = 72,
    LEGO_AI_CONDITION_NUM_IN_SET_ALIVE = 77,
    LEGO_AI_CONDITION_BEEN_TO_LEVEL = 84,
    LEGO_AI_CONDITION_MESSAGE = 86,
    LEGO_AI_CONDITION_CUT_SCENE_FINISHED = 89,
    LEGO_AI_CONDITION_FREEPLAY = 103,
    LEGO_AI_CONDITION_AI_OVERRIDE_CONTROL = 109,
    LEGO_AI_CONDITION_BEEN_SPAWNED = 120,
    LEGO_AI_CONDITION_CANNOT_REACH_DESTINATION = 124,
    LEGO_AI_CONDITION_FINISHED_SPLINE = 99,
    LEGO_AI_CONDITION_MUSIC_ON = 167,
    LEGO_AI_CONDITION_CHARACTER_LOADED = 168,
    LEGO_AI_CONDITION_SHOULD_ATTACK_OPPONENT = 170,
    LEGO_AI_CONDITION_IN_HUB_AREA = 174,
    LEGO_AI_CONDITION_IS_LOW_END_DEVICE = 175,
    LEGO_AI_CONDITION_RANDOM_MAP_CHARS_AVAILABLE = 176,
    LEGO_AI_CONDITION_FORCE_AT_END = 36,
    LEGO_AI_CONDITION_FORCE_AT_START = 35,
    LEGO_AI_CONDITION_BUILDIT_COMPLETE = 60,
};

typedef i32 GAMEPARAMTOFLOAT(AIPACKET *, AISCRIPTPROCESS *, char *, f32 *);
typedef i32 AICHARACTERTYPEID(char *name);
// Character lookups return -1 when no entry exists.  This must remain a
// signed integer callback: the level-character lookup is also installed as
// the default special-route lookup during game AI initialisation.
typedef i32 AISPECIALROUTECHARACTERTYPEID(char *name);
typedef f32 AICHARACTERDISTANCE(i32 character_type);
typedef void GAMEAILOAD(AISYS *system, i32 version, NUGSCN *scene, VARIPTR *buf, VARIPTR *buf_end);
typedef i32 AIACTIONPARSESPEED(char *name, u8 *speed);
typedef void SCRIPTPROCESSFIRSTTIMEACTION(AISYS *, AIPACKET *, AISCRIPTPROCESS *);
typedef u32 AIBIGJUMPTODESTINATION(APIOBJECT *object, NUVEC *destination);
typedef u32 AIRESPAWNONPATH(APIOBJECT *object);
typedef void AICLEARCREATURES(void);
typedef APIOBJECT *APIOBJECTFROMOBJID(u8 object_id);
typedef i32 AIFINDALTERNATIVESPECIALOBJECT(AISYS *system, struct nuhspecial_s *special);
typedef APIOBJECT *AIGETNAMEDAPIOBJECT(AISYS *system, char *name);
typedef NUVEC *AIGETCREATUREORIGIN(AISYS *system, AIPACKET *packet);

#ifdef __cplusplus
extern "C" {
#endif
    extern i32 AiParseExpressionFailed;

    extern AICONDITIONDEF api_aiconditiondefs[];
    extern AICONDITIONDEF lego_aiconditiondefs[];

    extern AIACTIONDEF api_aiactiondefs[];
    extern AIACTIONDEF *game_aiactiondefs;
    extern AIACTIONDEF lego_aiactiondefs[];

    extern NULISTHDR global_aiscripts;

    extern i32 ai_usepackfile;
    extern i32 ai_onlyusepackfile;

    extern GAMEPARAMTOFLOAT *GameParamToFloatFn;
    extern AICHARACTERTYPEID *GlobalCharacterTypeIDFn;
    extern AISPECIALROUTECHARACTERTYPEID *SpecialRouteCharacterTypeIDFn;
    extern AICHARACTERDISTANCE *GetViewRangeFn;
    extern AICHARACTERDISTANCE *GetHearDistanceFn;
    extern AICHARACTERDISTANCE *GetMaxViewHeightFn;
    extern AICHARACTERDISTANCE *GetMinViewHeightFn;
    extern GAMEAILOAD *GameAILoadFn;
    extern AIACTIONPARSESPEED *AIActionParseSpeedFn;
    extern SCRIPTPROCESSFIRSTTIMEACTION *ScriptProcessFirstTimeActionFn;
    extern AIBIGJUMPTODESTINATION *AIBigJumpToDestinationFn;
    extern AIRESPAWNONPATH *AIRespawnOnPathFn;
    extern AICLEARCREATURES *ClearAICreaturesFn;
    extern APIOBJECTFROMOBJID *APIOBJECTFromObjIDFn;
    extern AIFINDALTERNATIVESPECIALOBJECT *FindAlternativeSpecialObjectFn;
    extern AIGETNAMEDAPIOBJECT *GetNamedAPIObjectFn;
    extern AIGETCREATUREORIGIN *GetAICreatureOriginFn;
    extern char *AiLevelPathName;
    extern AISCRIPTPROCESS *pSetStateDebugee;

    void AiSysSetStateDebugee(AISCRIPTPROCESS *processor);
    void AiSysUsePackFile(i32 enabled);
    void AiSysOnlyUsePakFile(i32 enabled);
    void InitFn_GameParamToFloat(GAMEPARAMTOFLOAT *function);

    void InitFn_AIActionParseSpeed(AIACTIONPARSESPEED *function);
    void InitFn_AIBigJumpToDestination(AIBIGJUMPTODESTINATION *function);
    void InitFn_AIRespawnOnPath(AIRESPAWNONPATH *function);
    void InitFn_ScriptProcessFirstTimeAction(SCRIPTPROCESSFIRSTTIMEACTION *function);
    void InitFn_APIOBJECTFromObjIDFn(APIOBJECTFROMOBJID *function);
    void InitFn_ClearAICreatures(AICLEARCREATURES *function);
    void InitFn_FindAlternativeSpecialObjectFn(AIFINDALTERNATIVESPECIALOBJECT *function);
    void InitFn_GameAILoad(GAMEAILOAD *function);
    void InitFn_GetAICreatureOrigin(AIGETCREATUREORIGIN *function);
    void InitFn_GetNamedAPIObject(AIGETNAMEDAPIOBJECT *function);

    void AIScriptLoadAll(char *path, VARIPTR *buf, VARIPTR *buf_end, AISYS *sys);
    void AIScriptLoadAllPakFile(void *pak, char *path, VARIPTR *buf, VARIPTR *buf_end, AISYS *sys);

    void AIScriptInitConditions(AISYS *sys);
    void AIScriptForceParamReEval(AISCRIPTPROCESS *processor);
    char *AIScriptNameFromIx(AISYS *system, i32 index);
    void AIScriptSetLevelPath(char *path);

    void AIScriptProcessorInit(AISYS *sys, AIPACKET *packet, AISCRIPTPROCESS *processor, AICREATURE *creature,
                               char *script_name, char *start_state_name, i32 can_use_default, AISCRIPT *script,
                               AISTATE *start_state);

    AISCRIPT *AIScriptFind(AISYS *sys, char *name, i32 can_use_default, i32 check_level_scripts,
                           i32 check_global_scripts);

    void AIScriptClearInterrupt(AISCRIPTPROCESS *processor, char *state_name);
    i32 AIScriptSetInterrupt(AISCRIPTPROCESS *processor, u8 priority, u8 id, char *state_name, f32 time);
    void AIScriptSetState(AISCRIPTPROCESS *processor, AISTATE *state);
    i32 AIScriptSetStateByName(AISCRIPTPROCESS *processor, char *name);
    i32 AIScriptSetBaseScriptStateByName(AISCRIPTPROCESS *processor, char *name);

    AISTATE *AIStateFind(char *name, AISCRIPT *script);

    void RegisterAIScriptActions(AIACTIONDEF *action_defs);
    void RegisterAIScriptConditions(AICONDITIONDEF *cond_defs);

    f32 AIParamToFloat(AISCRIPTPROCESS *processor, char *param);
    f32 AIParamToFloatEx(AIPACKET *packet, AISCRIPTPROCESS *processor, char *param);

    AIPATH *AISysFindPath(AISYS *system, char *name);
    AILOCATOR *AIPathFindLocator(AISYS *system, char *name);
    AILOCATORSET *AIPathFindLocatorSet(AISYS *system, char *name);
    void AILocatorSet_AssignNearestLocator(AISYS *system, AILOCATORSET *locator_set, APIOBJECT *object, f32 max_range,
                                           NUVEC *position, NUVEC *second_position, f32 off_screen_radius,
                                           i32 ignore_assigned);
    void AILocatorSet_AssignFurthestLocator(AISYS *system, AILOCATORSET *locator_set, APIOBJECT *object, f32 max_range,
                                            NUVEC *position, NUVEC *second_position, f32 off_screen_radius,
                                            i32 ignore_assigned);
    void AILocatorSet_AssignRandomLocator(AISYS *system, AILOCATORSET *locator_set, APIOBJECT *object, f32 max_range,
                                          NUVEC *position, f32 off_screen_radius, i32 ignore_assigned);
    void AILocatorSet_CheckLocatorsStillAssigned(AISYS *system, AILOCATORSET *locator_set);
    void AISysCharacterSetPath(AIPACKET *packet, AIPATH *path);
    i32 AISysSetLevelPath(AISYS *system, char *path_name);
    void AISysFindRoute(AIPACKET *packet);
    void AISysCharacterSetPathCnx(AIPACKET *packet, NUVEC *position, AIPATHCNX *connection, i32 direction);
    void CalculateLocatorDirection(i32 direction, struct numtx_s *matrix, NUVEC *out);
    u32 AISysGetPathColour(i32 index);
    i32 AISysGetPathColourCount(void);
    void AISysGetPathPos(AISYS *system, NUVEC *position, AIPATHINFO *info, AIPATH *path, i32 checks);
    void AISysGetPathPosEx(AISYS *system, NUVEC *position, AIPATHINFO *info, AIPATH *path, i32 checks,
                           i32 search_all_paths, f32 *nearest_distance_squared);
    i32 WithinConnection(AISYS *system, NUVEC *position, AIPATH *path, AIPATHCNX *connection, i32 checks,
                         AIPATHCNX *previous_connection, i32 route, i32 ground, AIPATHINFO *path_info, f32 radius,
                         i32 update_once);
    f32 AIPathNodeDistanceToPathNode(AIPATH *path, i32 start_node, i32 destination_node, i32 route,
                                     u32 excluded_route_mask);
    i32 AISysGetCharacterWaypoint(const AIPACKET *packet, NUVEC *position);
    void AISysGetCharacterPathPos(AISYS *system, APIOBJECT *object, AIPACKET *packet, i32 checks, i32 ground);
    void AISysUpdateCharacterPathPos(AISYS *system, APIOBJECT *object, AIPACKET *packet, i32 checks, f32 elapsed);
    void AISysCharacterMovement(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);
    void AISysProcessCharacter(AISYS *system, APIOBJECT *object, AIPACKET *packet, i32 checks, f32 elapsed,
                               i32 use_three_dimensions, i32 process_ai);
    void AISysProcess(AISYS *system, APIOBJECT *player_1, APIOBJECT *player_2);
    void AIFormationFollow(AIPACKET *packet);
    void DestroyAIGroup(AIGROUP *group);
    AIMESSAGESYS_s *CreateAIMessageSys(VARIPTR *cursor, VARIPTR *end, i32 count);
    void ResetAIMessageSys(AIMESSAGESYS_s *system);
    void ClearAIMessageSys(AIMESSAGESYS_s *system);
    AIMESSAGE_s *CheckAIMessage(AIMESSAGESYS_s *system, char *name, AIMESSAGE_s *message);
    f32 GetAIMessage(AIMESSAGESYS_s *system, char *name, AIMESSAGE_s *message);
    void SetAIMessage(AIMESSAGESYS_s *system, char *name, f32 value, AIMESSAGE_s *message);
    AIMESSAGE_s *QueryAIMessage(AIMESSAGESYS_s *system, AIMESSAGE_s *message);
    void RemoveAIMessage(AIMESSAGESYS_s *system, char *name, AIMESSAGE_s *message);
    AILOCALMESSAGE_s *FindLocalAIMessage(AISCRIPTPROCESS *processor, char *name);
    void AddLocalAIMessage(AISCRIPTPROCESS *processor, AILOCALMESSAGE_s *message, char *name);
    void AIAntinodeMove(AIANTINODE *antinode, NUVEC *position, f32 radius, f32 below, f32 above);
    AIANTINODE *AIAntinodeCreate(NUVEC *position, f32 radius);
    AIANTINODE *AIAntinodeCreateSingleFrame(NUVEC *position, f32 radius);
    void AIAntinodeDestroy(AIANTINODE *antinode);
    void AIAntinodeCullSingleFrame(void);
    void AISysCreatureAntinodeInteraction(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable);
    void AISysCreatureInteraction3D(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable,
                                    f32 delta_time);
    void AISysCreatureInteraction2D(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable,
                                    f32 delta_time);
    void LEGO_AISysCreatureInteraction2D(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable,
                                         f32 delta_time);
    extern void (*checkantinodefns[3])(APIOBJECT *, AIANTINODE *, NUVEC *, f32);
    void AISetPathHeightTol(f32 tolerance);
    void AISysSetPathCylinderCheck(i32 enabled);
    AIPATHCNX *AIPathFindPathCnxFromIX(AISYS *system, AIPATH *path, u8 from_index, u8 to_index);
    void AIMoveInstruction(AIPACKET *packet, NUVEC *destination, f32 stopping_distance, AIPATHINFO *path_info, i32 mode,
                           f32 movement_parameter);
    void FollowAPIObject(APIOBJECT *object, APIOBJECT *target, i32 flags, f32 movement_parameter);
    void FindAIDirectionedRandomPointOnNetwork2D(AIPACKET *packet, NUVEC *position, NUVEC *direction, f32 distance,
                                                 f32 offset, i32 flags, NUVEC *result);
    void AIScriptProcess(AISYS *system, APIOBJECT *object, AIPACKET *packet, AISCRIPTPROCESS *processor, f32 elapsed);
#ifdef __cplusplus
}
#endif

f32 AiParseExpression(char *expr);

void AIScriptOpenPakFileParse(AISCRIPT **script_ref, void *pak, char *filename, char *path, VARIPTR *buf,
                              VARIPTR *buf_end);

#ifdef __cplusplus
bool AISysNodeCanReachThisJumpConnection(GameObject_s &object, AIPATH_s &path, unsigned char node_index,
                                         AIPATHCNX_s &connection, i32 direction);
bool AISysCharacterCanReachThisJumpConnection(GameObject_s &object, AIPATH_s &path, AIPATHCNX_s &connection,
                                              i32 direction);
u32 DoSomeChecks(GameObject_s &object, AIPATH_s &path, AIPATHCNX_s &connection, i32 direction);
#endif
