#pragma once

#include "decomp.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nu3d/nuhspecial.h"

struct EDAIPATH_s;
struct EDAIPATHWALL_s;
struct eduimenu_s;
struct AISYS_s;
struct AIPATH_s;

struct EditorNamedEntry {
    NULISTLNK link;
    char name[0x10];
};

struct AIEDITOR_RENDER_STATE {
    u8 unknown_00[0x04];
    eduimenu_s *main_menu;
    void *enter_context;
    void *enter_owner;
    union {
        NUVEC selection_position;
        NUVEC cursor_position;
    };
    u8 unknown_1c[0x28 - 0x1c];
    NUVEC camera_position;
    union {
        u8 unknown_34[0x64 - 0x34];
        struct {
            u8 unknown_34_to_3c[8];
            nuhspecial_s cursor_platform;
            u8 unknown_48_to_64[0x64 - 0x48];
        };
    };
    AISYS_s *ai_system;
    u8 unknown_68[0x3fe8 - 0x68];
    EDAIPATH_s *current_path;
    union {
        NULISTHDR free_paths;
        u8 unknown_3fec[8];
    };
    NULISTHDR paths;
    union {
        u8 unknown_3ffc[0x31304 - 0x3ffc];
        struct {
            u8 unknown_3ffc_to_30ffc[0x30ffc - 0x3ffc];
            NULISTHDR free_path_nodes;
            u8 unknown_31004_to_31304[0x31304 - 0x31004];
        };
    };
    union {
        NULISTHDR free_shared_nodes;
        NULISTHDR free_shared_path_nodes;
    };
    union {
        NULISTHDR path_walls;
        NULISTHDR shared_path_nodes;
    };
    union {
        u8 unknown_31314[0x36930 - 0x31314];
        struct {
            u32 unknown_31314_word;
            AIPATH_s *runtime_path;
            u8 unknown_3131c_to_36930[0x36930 - 0x3131c];
        };
    };
    EditorNamedEntry *mode_selection_36930;
    u8 unknown_36934[0x37a48 - 0x36934];
    void *mode_selection_37a48;
    u8 unknown_37a4c[0x3c260 - 0x37a4c];
    void *mode_selection_3c260;
    u8 unknown_3c264[0x40888 - 0x3c264];
    EditorNamedEntry *current_route;
    u8 unknown_4088c[0x42e9c - 0x4088c];
    void *mode_selection_42e9c;
    u8 unknown_42ea0[0x42ea8 - 0x42ea0];
    u8 flags;
    u8 unknown_42ea9[3];
    void *enter_extra_1;
    void *enter_extra_2;
    void *enter_extra_3;
};

DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, current_path) == 0x3fe8, "editor current path offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, selection_position) == 0x10, "editor selection position offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, free_paths) == 0x3fec, "editor free path list offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, main_menu) == 0x04, "editor main menu offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, ai_system) == 0x64, "editor AI system offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, enter_context) == 0x08, "editor enter context offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, enter_owner) == 0x0c, "editor enter owner offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, paths) == 0x3ff4, "editor path list offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, free_shared_nodes) == 0x31304, "editor free shared node list offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, path_walls) == 0x3130c, "editor path wall list offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, mode_selection_36930) == 0x36930, "editor mode selection offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, mode_selection_37a48) == 0x37a48, "editor mode selection offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, mode_selection_3c260) == 0x3c260, "editor mode selection offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, current_route) == 0x40888, "editor current route offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, mode_selection_42e9c) == 0x42e9c, "editor mode selection offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, flags) == 0x42ea8, "editor flags offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, enter_extra_1) == 0x42eac, "editor enter extra offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, enter_extra_2) == 0x42eb0, "editor enter extra offset");
DECOMP_ASSERT(offsetof(AIEDITOR_RENDER_STATE, enter_extra_3) == 0x42eb4, "editor enter extra offset");

extern "C" AIEDITOR_RENDER_STATE *aieditor;
