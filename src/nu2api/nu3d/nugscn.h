#pragma once
#include "nu2api/numath/nuvec4.h"

#include "decomp.h"

#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/nupostparams.h"

struct nunativetex_s;
struct nudldlistscene_s;
struct nuinstanim_s;
struct nuhspecial_s;
struct numtx_s;
struct NUFRUSTRUM;
typedef struct nuanimdata_s nuanimdata_s;

typedef struct nuanimendlookup_s {
    u16 count;
    u16 end_frame;
    f32 *times;
    u8 *values;
} nuanimendlookup_s;
DECOMP_ASSERT(sizeof(nuanimendlookup_s) == 0xc, "nuanimendlookup_s ABI");

typedef struct nuvideoresheader_s {
    u16 nvertex_buffers;
    u16 pad_02;
    usize *vertex_buffers;
    u16 nindex_buffers;
    u16 pad_0a;
    usize *index_buffers;
    u16 ntextures;
    u16 pad_12;
    u32 *textures;
    i32 texture_hashes;
} NUVIDEORESHEADER;

extern NUVIDEORESHEADER g_VideoResHeader;

struct nudisplayscene_s {
    u32 render_scene_id;  // 0x00
    void *state_ptr;      // 0x04
    u32 clear_flags;      // 0x08
    u32 bg_colour;        // 0x0c
    f32 clear_alpha;      // 0x10
    f32 vp_x;             // 0x14
    f32 vp_y;             // 0x18
    f32 vp_w;             // 0x1c
    f32 vp_h;             // 0x20
    void *unknown_24;     // 0x24
    u32 unknown_28;       // 0x28
    u8 burnout_intensity; // 0x2c
    u8 pad2d[3];
    f32 burnout_flare;     // 0x30
    f32 burnout_threshold; // 0x34
    u32 unknown_38;        // 0x38
    void *unknown_3c;      // dynamic light
    i32 unknown_40;        // 0x40
    u32 unknown_44;        // 0x44
    u32 unknown_48;        // 0x48
    f32 unknown_4c;        // 0x4c — deferred-shading parameter 0
    f32 unknown_50;        // 0x50 — deferred-shading parameter 1
    f32 unknown_54;        // 0x54 — deferred-shading parameter 2
    union {
        NuBloomParameters bloom;
        struct {
            u32 unknown_58;
            u8 pad5c[0x18];
            u8 flags;
            u8 pad75[0x37];
        };
    };
    union {
        NuDepthOfFieldParameters dof;
        struct {
            u32 unknown_ac;
            u8 padb0[0x14];
        };
    };
    u32 texture_blend_enabled;       // 0xc4
    i32 texture_blend_arg0;          // 0xc8
    i32 texture_blend_arg1;          // 0xcc
    NUVEC4 texture_blend_parameters; // 0xd0
    u32 unknown_e0;                  // 0xe0
    u32 unknown_e4;                  // 0xe4
    NUMTX motion_previous;           // 0xe8
    NUMTX motion_current;            // 0x128
    f32 motion_scale;                // 0x168
    f32 motion_maximum;              // 0x16c
    f32 motion_falloff;              // 0x170
    u32 unknown_174;                 // 0x174
    u32 unknown_178;                 // 0x178
    i32 accumulation_frames;         // 0x17c
    i32 accumulation_mode;           // 0x180
    f32 accumulation_blend;          // 0x184
    union {
        NuSpeedBlurParameters speed_blur;
        struct {
            u32 unknown_188;
            u8 pad18c[0x88];
        };
    };
    u32 unknown_214; // 0x214
};

enum {
    NU_DISPLAYSCENE_FLAG_NEEDS_BUILD = 0x10,
};

// NuVisiEvaluate returns a view beginning at NUGSCN::visibility_result_instance_count.
typedef struct nuvisibilityresult_s {
    i32 instance_count;            // 0x00
    u8 pad_04[4];                  // 0x04
    struct nugscn_s *source_scene; // 0x08
    void *occlusion_data;          // 0x0c
    u8 pad_10[4];                  // 0x10
    void *instance_tree;           // 0x14
    void *portal_marker;           // 0x18
    u8 *portal_bits;               // 0x1c
    void *visibility_context;      // 0x20
    u8 *instance_tree_bits;        // 0x24
    u8 state;                      // 0x28
} NuVisibilityResult;

DECOMP_ASSERT(offsetof(NuVisibilityResult, portal_marker) == 0x18, "visibility portal marker offset");
DECOMP_ASSERT(offsetof(NuVisibilityResult, portal_bits) == 0x1c, "visibility portal bits offset");
DECOMP_ASSERT(offsetof(NuVisibilityResult, instance_tree_bits) == 0x24, "visibility tree bits offset");
DECOMP_ASSERT(offsetof(NuVisibilityResult, state) == 0x28, "visibility state offset");

typedef struct nugscn_s {
    i32 *texture_ids;
    i32 ntextures;
    struct nunativetex_s **textures;
    struct numtl_s **mtls;
    i32 nummtl;
    undefined field8_0x14;
    undefined field9_0x15;
    undefined field10_0x16;
    undefined field11_0x17;
    undefined field12_0x18;
    undefined field13_0x19;
    undefined field14_0x1a;
    undefined field15_0x1b;
    i32 num_instances; // 0x1c
    u8 *instances;     // 0x20, 0x50-byte legacy instance records
    i32 numspecial;
    struct nuspecial_s *specials;
    undefined field26_0x2c;
    undefined field27_0x2d;
    undefined field28_0x2e;
    undefined field29_0x2f;
    i32 numsplines;
    struct nugspline_s *splines;
    undefined pad_38[8];
    struct nugscn_s **additional_scenes;         // 0x40
    i32 rendered_additional_scene_count;         // 0x44
    i16 num_instance_animations;                 // 0x48
    i16 num_instance_animation_data;             // 0x4a
    struct nuinstanim_s *instance_animations;    // 0x4c, 0x60-byte entries
    struct numtx_s *instance_animation_matrices; // 0x50, 0x40-byte entries
    nuanimdata_s **instance_animation_data;      // 0x54
    undefined pad_58[8];
    i32 num_texture_anims;
    void *texture_anims;
    u16 *texture_anim_ids;
    u32 max_portals;   // 0x6c
    NUPORTAL *portals; // 0x70, 0x20-byte entries
    i32 num_rooms;     // 0x74
    NUROOM *rooms;     // 0x78, 0x18-byte entries
    undefined pad_7c[0x20];
    struct NUFRUSTRUM *portal_frusta[16]; // 0x9c, traversal work list
    i32 num_portal_frusta;                // 0xdc
    i32 camera_room;                      // 0xe0
    i32 portal_depth;                     // 0xe4, maximum recursive depth
    undefined pad_e8[0x0c];
    i32 portal_instance_count;      // 0xf4, nonzero when portal visibility data is present
    NUPORTALSPHERE *portal_spheres; // 0xf8, one 0x10-byte sphere per instance
    NUPORTALBOX *portal_boxes;      // 0xfc, one 0x20-byte box per instance
    undefined pad_100[0x10];
    struct nudldlistscene_s *display_list;
    undefined pad_114[0x0c];
    i32 visibility_result_instance_count; // 0x120
    undefined pad_124[4];
    struct nugscn_s *visibility_source_scene; // 0x128
    void *occlusion_data;                     // 0x12c
    undefined pad_130[4];
    void *instance_visibility_tree;     // 0x134
    void *portal_visibility_marker;     // 0x138, enables the portal result at 0x13c
    u8 *instance_visibility_flags;      // 0x13c, shared portal-visibility result buffer
    void *visibility_context;           // 0x140
    u8 *instance_tree_visibility_flags; // 0x144
    u8 visibility_state;                // 0x148
    undefined field302_0x149;
    undefined field303_0x14a;
    undefined field304_0x14b;
    undefined field305_0x14c;
    undefined field306_0x14d;
    undefined field307_0x14e;
    undefined field308_0x14f;
    undefined field309_0x150;
    undefined field310_0x151;
    undefined field311_0x152;
    undefined field312_0x153;
    undefined field313_0x154;
    undefined field314_0x155;
    undefined field315_0x156;
    undefined field316_0x157;
    undefined field317_0x158;
    undefined field318_0x159;
    undefined field319_0x15a;
    undefined field320_0x15b;
    undefined field321_0x15c;
    undefined field322_0x15d;
    undefined field323_0x15e;
    undefined field324_0x15f;
    undefined field325_0x160;
    undefined field326_0x161;
    undefined field327_0x162;
    undefined field328_0x163;
    undefined field329_0x164;
    undefined field330_0x165;
    undefined field331_0x166;
    undefined field332_0x167;
    undefined field333_0x168;
    undefined field334_0x169;
    undefined field335_0x16a;
    undefined field336_0x16b;
    undefined field337_0x16c;
    undefined field338_0x16d;
    undefined field339_0x16e;
    undefined field340_0x16f;
    undefined field341_0x170;
    undefined field342_0x171;
    undefined field343_0x172;
    undefined field344_0x173;
    undefined field345_0x174;
    undefined field346_0x175;
    undefined field347_0x176;
    undefined field348_0x177;
    undefined field349_0x178;
    undefined field350_0x179;
    undefined field351_0x17a;
    undefined field352_0x17b;
    undefined field353_0x17c;
    undefined field354_0x17d;
    undefined field355_0x17e;
    undefined field356_0x17f;
    undefined field357_0x180;
    undefined field358_0x181;
    undefined field359_0x182;
    undefined field360_0x183;
    undefined field361_0x184;
    undefined field362_0x185;
    undefined field363_0x186;
    undefined field364_0x187;
    undefined field365_0x188;
    undefined field366_0x189;
    undefined field367_0x18a;
    undefined field368_0x18b;
    undefined field369_0x18c;
    undefined field370_0x18d;
    undefined field371_0x18e;
    undefined field372_0x18f;
    undefined field373_0x190;
    undefined field374_0x191;
    undefined field375_0x192;
    undefined field376_0x193;
    undefined field377_0x194;
    undefined field378_0x195;
    undefined field379_0x196;
    undefined field380_0x197;
    undefined field381_0x198;
    undefined field382_0x199;
    undefined field383_0x19a;
    undefined field384_0x19b;
    undefined field385_0x19c;
    undefined field386_0x19d;
    undefined field387_0x19e;
    undefined field388_0x19f;
    undefined field389_0x1a0;
    undefined field390_0x1a1;
    undefined field391_0x1a2;
    undefined field392_0x1a3;
    undefined field393_0x1a4;
    undefined field394_0x1a5;
    undefined field395_0x1a6;
    undefined field396_0x1a7;
    undefined field397_0x1a8;
    undefined field398_0x1a9;
    undefined field399_0x1aa;
    undefined field400_0x1ab;
    undefined field401_0x1ac;
    undefined field402_0x1ad;
    undefined field403_0x1ae;
    undefined field404_0x1af;
    undefined field405_0x1b0;
    undefined field406_0x1b1;
    undefined field407_0x1b2;
    undefined field408_0x1b3;
    undefined field409_0x1b4;
    undefined field410_0x1b5;
    undefined field411_0x1b6;
    undefined field412_0x1b7;
    undefined field413_0x1b8;
    undefined field414_0x1b9;
    undefined field415_0x1ba;
    undefined field416_0x1bb;
    undefined field417_0x1bc;
    undefined field418_0x1bd;
    undefined field419_0x1be;
    undefined field420_0x1bf;
    undefined field421_0x1c0;
    undefined field422_0x1c1;
    undefined field423_0x1c2;
    undefined field424_0x1c3;
    undefined field425_0x1c4;
    undefined field426_0x1c5;
    undefined field427_0x1c6;
    undefined field428_0x1c7;
    undefined field429_0x1c8;
    undefined field430_0x1c9;
    undefined field431_0x1ca;
    undefined field432_0x1cb;
    undefined field433_0x1cc;
    undefined field434_0x1cd;
    undefined field435_0x1ce;
    undefined field436_0x1cf;
    struct nunativegscene_s *field437_0x1d0;
    undefined field438_0x1d4;
    undefined field439_0x1d5;
    undefined field440_0x1d6;
    undefined field441_0x1d7;
    undefined field442_0x1d8;
    undefined field443_0x1d9;
    undefined field444_0x1da;
    undefined field445_0x1db;
    undefined field446_0x1dc;
    undefined field447_0x1dd;
    undefined field448_0x1de;
    undefined field449_0x1df;
    nuanimendlookup_s *animation_end_frames; // 0x1e0
    undefined field454_0x1e4;
    undefined field455_0x1e5;
    undefined field456_0x1e6;
    undefined field457_0x1e7;
    undefined field458_0x1e8;
    undefined field459_0x1e9;
    undefined field460_0x1ea;
    undefined field461_0x1eb;
    undefined field462_0x1ec;
    undefined field463_0x1ed;
    undefined field464_0x1ee;
    undefined field465_0x1ef;
    undefined field466_0x1f0;
    undefined field467_0x1f1;
    undefined field468_0x1f2;
    undefined field469_0x1f3;
    undefined field470_0x1f4;
    undefined field471_0x1f5;
    undefined field472_0x1f6;
    undefined field473_0x1f7;
} NUGSCN;

DECOMP_ASSERT(offsetof(NUGSCN, instance_animations) == 0x4c, "NUGSCN instance animation array offset");
DECOMP_ASSERT(offsetof(NUGSCN, num_instance_animations) == 0x48, "NUGSCN instance animation count offset");
DECOMP_ASSERT(offsetof(NUGSCN, num_instance_animation_data) == 0x4a, "NUGSCN instance animation-data count offset");
DECOMP_ASSERT(offsetof(NUGSCN, instance_animation_matrices) == 0x50, "NUGSCN instance animation matrix array offset");
DECOMP_ASSERT(offsetof(NUGSCN, instance_animation_data) == 0x54, "NUGSCN instance animation data offset");
DECOMP_ASSERT(offsetof(NUGSCN, max_portals) == 0x6c, "NUGSCN portal-count offset");
DECOMP_ASSERT(offsetof(NUGSCN, portals) == 0x70, "NUGSCN portal-array offset");
DECOMP_ASSERT(offsetof(NUGSCN, num_rooms) == 0x74, "NUGSCN room-count offset");
DECOMP_ASSERT(offsetof(NUGSCN, rooms) == 0x78, "NUGSCN room-array offset");
DECOMP_ASSERT(offsetof(NUGSCN, portal_frusta) == 0x9c, "NUGSCN portal-frustum array offset");
DECOMP_ASSERT(offsetof(NUGSCN, num_portal_frusta) == 0xdc, "NUGSCN portal-frustum count offset");
DECOMP_ASSERT(offsetof(NUGSCN, camera_room) == 0xe0, "NUGSCN camera-room offset");
DECOMP_ASSERT(offsetof(NUGSCN, portal_depth) == 0xe4, "NUGSCN portal-depth offset");
DECOMP_ASSERT(offsetof(NUGSCN, portal_instance_count) == 0xf4, "NUGSCN portal-instance count offset");
DECOMP_ASSERT(offsetof(NUGSCN, display_list) == 0x110, "NUGSCN display-list offset");
DECOMP_ASSERT(offsetof(NUGSCN, visibility_result_instance_count) == 0x120, "NUGSCN visibility result offset");
DECOMP_ASSERT(offsetof(NUGSCN, portal_visibility_marker) == 0x138, "NUGSCN portal-visibility marker offset");
DECOMP_ASSERT(offsetof(NUGSCN, instance_visibility_flags) == 0x13c, "NUGSCN portal-visibility buffer offset");
DECOMP_ASSERT(offsetof(NUGSCN, animation_end_frames) == 0x1e0, "NUGSCN animation end-frame table offset");

#ifdef __cplusplus
void *NuGScnBufferAllocAligned(i32 size, i32 alignment);
void ReadInstAnimBlock(i32 file, NUGSCN *scene);
void ReadInstAnimBlockDlist(i32 file, NUGSCN *scene);
i32 NuGScnReadTexturesPS(i32 file, VARIPTR *buf, VARIPTR buf_end);
void NuGScnCreatePS(NUGSCN *scene, VARIPTR *buf, VARIPTR *buf_end);
i32 NuGScnFixupTID(NUGSCN *scene, i32 tid);
void NuGScnFixupTIDs(NUGSCN *scene);
i32 NuGScnRestoreTID(NUGSCN *scene, i32 tid);
void NuGScnRestoreTIDs(NUGSCN *scene);

extern "C" {
    void NuGScnRndr(NUGSCN *scene);
#endif

    void NuGScnRndr3(NUGSCN *scene);
    NUGSCN *NuGScnRead(VARIPTR *buf, VARIPTR buf_end, char *path);
    void NuGScnRemove(NUGSCN *scene);
    void NuGScnFixupPS(NUGSCN *scene);
    void NuGScnFixupTIDsPS(NUGSCN *scene);
    void NuGScnRestoreTIDsPS(NUGSCN *scene);
    // The trailing flags argument is passed as 1 by every caller in the
    // original binary; the original implementation never reads it.
    i32 NuSpecialFind(NUGSCN *scene, struct nuhspecial_s *dest, char *name, i32 flags);

#ifdef __cplusplus
}
#endif
