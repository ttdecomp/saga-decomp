#pragma once
#include "decomp.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nu3d/nuhspecial.h"
#include "gameapi/ai/aisys/aisys.h"
struct EDAIPATHNODE_s;
struct EDAISHAREDPATHNODE_s {
    NULISTLNK link;
    i8 reference_count;
};
struct EDAIPATHCNX_s {
    EDAIPATHNODE_s *node;
    u32 flags;
    f32 distance;
};
struct EDAIPATHNODE_s {
    NULISTLNK link;
    char name[16];
    NUVEC position;
    f32 radius;
    f32 height_min;
    f32 height_max;
    EDAIPATHCNX_s connections[8];
    i16 index;
    u8 flags;
    u8 unknown_93[5];
    nuhspecial_s platform;
    NUVEC platform_position;
    EDAISHAREDPATHNODE_s *shared_node;
};
struct EDAIPATH_s {
    NULISTLNK link;
    char name[16];
    NULISTHDR nodes;
    EDAIPATHNODE_s *current_node;
    EDAIPATHNODE_s *nearest_node;
    u32 unknown_28;
    u16 flags;
    i16 runtime_start;
    i16 runtime_end;
    i16 runtime_nearest;
    i16 node_count;
    u8 unknown_36[2];
};
struct AIEDITOR_RENDER_STATE {
    u8 unknown_0[0x10];
    NUVEC cursor_position;
    u8 unknown_1c[0x28 - 0x1c];
    NUVEC camera_position;
    u8 unknown_34[8];
    nuhspecial_s cursor_platform;
    u8 unknown_48[0x3fe8 - 0x48];
    EDAIPATH_s *current_path;
    u8 unknown_3fec[8];
    NULISTHDR paths;
    u8 unknown_3ffc[0x30ffc - 0x3ffc];
    NULISTHDR free_path_nodes;
    u8 unknown_31004[0x31304 - 0x31004];
    NULISTHDR free_shared_path_nodes;
    NULISTHDR shared_path_nodes;
    u32 unknown_31314;
    AIPATH_s *runtime_path;
    u8 unknown_3131c[0x42ea8 - 0x3131c];
    u8 flags;
};
struct aieditor_settings_s {
    u8 enabled;
    u8 unknown_001[0x10 - 1];
    u8 show_creatures;
    u8 unknown_011[0x20 - 0x11];
    u8 snap_height;
    u8 unknown_021[0x44 - 0x21];
    i32 current_path_type;
    u8 unknown_048[0x60 - 0x48];
    u8 path_flags;
    u8 path_flags2;
    u8 unknown_062[0x200 - 0x62];
};
DECOMP_ASSERT(sizeof(EDAIPATHNODE_s) == 0xb4, "editor path node");
DECOMP_ASSERT(sizeof(EDAIPATH_s) == 0x38, "editor path");
DECOMP_ASSERT(sizeof(aieditor_settings_s) == 0x200, "editor settings");
extern "C" {
    extern AIEDITOR_RENDER_STATE *aieditor;
}
extern aieditor_settings_s aieditorsettings;

struct nupad_s;
struct eduimenu_s;
typedef void AIEDITORPATHNODECALLBACK(EDAIPATHNODE_s *node);
extern "C" {
    extern AIEDITORPATHNODECALLBACK *AIPathNodeDeletedFn;
    void InitFn_AIPathNodeDeleted(AIEDITORPATHNODECALLBACK *function);
}
eduimenu_s *pathEditor_Process(nupad_s *pad);
