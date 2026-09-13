#pragma once

#include "nu2api/nucore/common.h"
#include "nu2api/numath/numtx.h"
#include "decomp.h"

struct nudisplayscene_s;
struct nugscn_s;
struct ani3_animheader_s;
struct nuanimdata2_s;
struct nuanimbuff_s;
using NUHGOBJROOTFN = void (*)(NUMTX *, void *, NUVEC *, NUVEC *, NUVEC *, f32);

enum NUJOINTANIM_FLAGS : u8 {
    NUJOINTANIM_ROTATION = 1 << 0,
    NUJOINTANIM_TRANSLATION = 1 << 1,
    NUJOINTANIM_SCALE = 1 << 2,
    NUJOINTANIM_LIMIT_ROTATION_X = 1 << 3,
    NUJOINTANIM_LIMIT_ROTATION_Y = 1 << 4,
    NUJOINTANIM_LIMIT_ROTATION_Z = 1 << 5,
};

struct NUJOINTANIM_s {
    NUVEC rotation;
    NUVEC translation;
    NUVEC scale;
    i16 rotation_limit_start[3];
    i16 rotation_limit_end[3];
    u8 joint_index;
    NUJOINTANIM_FLAGS flags;
    u8 pad_32[2];
};

using NUJOINTPROCANIMFN = void (*)(nuanimbuff_s *, struct nuhgobj_s *, i32, NUJOINTANIM_s *);

struct nuhgobjpoi_s {
    union {
        u8 data[0x50];
        struct {
            NUMTX local_matrix;
            u32 field_0x40;
            u8 joint_index;
            u8 field_0x45[0x0b];
        };
    };
};

struct nuhgobjjoint_s {
    // Local matrix used when an animation node inherits its unanimated bind
    // orientation.  The hierarchy also owns a separate bind-matrix array at
    // 0x170 for rest-pose evaluation.
    NUMTX animation_bind_matrix;
    u8 data_0x40[0x10];
    u8 parent_index;
    u8 data_0x51[0xf];
};

struct nuhgobjjointoverride_s {
    f32 rotation_x;
    f32 rotation_y;
    f32 rotation_z;
    NUVEC translation;
    u8 data_0x18[0x18];
    u8 joint_index;
    u8 data_0x31[3];
};

struct nuhgobjrender_s {
    char *name;
    void **rigid_specials;
    void *smooth_skin_special;
    void **alternate_rigid_specials;
    void *alternate_smooth_skin_special;
};

struct nuhgobjshadowgroup_s {
    u8 (*ellipses)[0x30];
    u8 (*cylinders)[0x40];
    void *field_0x8;
    u8 ellipse_count;
    u8 cylinder_count;
    u8 field_0xe;
    u8 joint_index;
};

struct nuhgobj_s {
    u8 data[0x110];
    nudisplayscene_s *display_list; // 0x110
    u8 data_0x114[0x54];
    i32 joint_count;              // 0x168
    nuhgobjjoint_s *joints;       // 0x16c
    NUMTX *bind_matrices;         // 0x170
    NUMTX *inverse_bind_matrices; // 0x174
    i32 joint_override_map_count; // 0x178
    u8 *joint_override_map;       // 0x17c
    u8 data_0x180[4];
    nuhgobjpoi_s *points_of_interest; // 0x184
    i32 point_of_interest_count;      // 0x188
    u8 *point_of_interest_map;        // 0x18c
    i32 render_count;                 // 0x190
    nuhgobjrender_s *render_parts;    // 0x194
    union {
        u8 data_0x198[0x14];
        struct {
            u32 field_0x198;
            nuhgobjshadowgroup_s *shadow_groups;
            u8 suppress_shadow_surface_points;
            u8 data_0x1a1[0x0b];
        };
    };
    NUVEC bounds_min; // 0x1ac
    NUVEC bounds_max; // 0x1b8
};

DECOMP_ASSERT(sizeof(nuhgobjpoi_s) == 0x50, "nuhgobjpoi_s size");
DECOMP_ASSERT(offsetof(nuhgobjpoi_s, joint_index) == 0x44, "nuhgobjpoi_s joint offset");
DECOMP_ASSERT(sizeof(nuhgobjjoint_s) == 0x60, "nuhgobjjoint_s size");
DECOMP_ASSERT(sizeof(nuhgobjjointoverride_s) == 0x34, "nuhgobjjointoverride_s size");
DECOMP_ASSERT(sizeof(NUJOINTANIM_s) == 0x34, "NUJOINTANIM_s size");
DECOMP_ASSERT(offsetof(NUJOINTANIM_s, joint_index) == 0x30, "NUJOINTANIM_s joint index offset");
DECOMP_ASSERT(offsetof(NUJOINTANIM_s, flags) == 0x31, "NUJOINTANIM_s flags offset");
DECOMP_ASSERT(sizeof(nuhgobjrender_s) == 0x14, "nuhgobjrender_s size");
DECOMP_ASSERT(sizeof(nuhgobjshadowgroup_s) == 0x10, "nuhgobjshadowgroup_s size");
DECOMP_ASSERT(offsetof(nuhgobj_s, shadow_groups) == 0x19c, "nuhgobj_s shadow groups offset");
DECOMP_ASSERT(sizeof(nuhgobj_s) == 0x1c4, "nuhgobj_s size");

// NuHGobj system (module nu2api/nucore, nucore_plain.cpp).

#ifdef __cplusplus
extern "C" {
#endif
    extern NUJOINTPROCANIMFN JointProcAnimFn;
    i32 NuHGobjRndrRandShadowSurfacePoints(nuhgobj_s *object, NUMTX *world_matrix, NUMTX *joint_matrices, i32 count,
                                           NUVEC *positions, i32 exclusion_mask);
    i32 NuHGobjReversibleCharacters(i32 enabled);
    i32 NuHGobjForceShadowsOnCharacters(i32 enabled);
    void NuAnimBuffProceduralAnimation(nuanimbuff_s *buffer, nuhgobj_s *object, i32 override_count,
                                       NUJOINTANIM_s *overrides);
    nuhgobj_s *NuGHGRead(char *path, VARIPTR *buf, VARIPTR buf_end);
    void NuHGobjDestroy(nuhgobj_s *object);
    nuhgobjpoi_s *NuHGobjGetPOI(nuhgobj_s *object, i32 index);
    void NuHGobjPOIMtx(nuhgobj_s *object, u8 index, NUMTX *world_matrix, NUMTX *joint_matrices, NUMTX *result);
    i32 NuHGobjGetLayerIndex(char *name, nuhgobj_s *object);
    void NuHGobjEval(nuhgobj_s *object, i32 override_count, nuhgobjjointoverride_s *overrides, NUMTX *matrices);
    void NuHGobjEvalAnim2(nuhgobj_s *object, ani3_animheader_s *animation, f32 time, i32 override_count,
                          NUJOINTANIM_s *overrides, NUMTX *matrices);
    void NuHGobjEvalAnim2Root(nuhgobj_s *object, ani3_animheader_s *animation, f32 time, i32 override_count,
                              NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data);
    void NuHGobjEvalAnim2Root_3(nuhgobj_s *object, ani3_animheader_s *animation, f32 time, i32 override_count,
                                NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data);
    void NuHGobjEvalAnimBlend2(nuhgobj_s *object, ani3_animheader_s *animation_a, f32 time_a,
                               ani3_animheader_s *animation_b, f32 time_b, f32 blend, i32 override_count,
                               NUJOINTANIM_s *overrides, NUMTX *matrices);
    void NuHGobjEvalAnimBlend2Root(nuhgobj_s *object, ani3_animheader_s *animation_a, f32 time_a,
                                   ani3_animheader_s *animation_b, f32 time_b, f32 blend, i32 override_count,
                                   NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data);
    void **NuHGobjEvalDwa2(i32 render_count, i16 *render_indices, nuanimdata2_s *animation, f32 frame);
    void **NuHGobjEvalDwaBlend(i32 render_count, i16 *render_indices, void *animation_a, f32 frame_a, void *animation_b,
                               f32 frame_b, f32 blend);
    void **NuHGobjEvalDwaBlend2(i32 render_count, i16 *render_indices, nuanimdata2_s *animation_a, f32 frame_a,
                                nuanimdata2_s *animation_b, f32 frame_b, f32 blend);
    i32 NuHGobjRndrMtxDwa(nuhgobj_s *object, NUMTX *world_matrix, i32 render_count, i16 *render_indices,
                          NUMTX *joint_matrices, void **blend_values, i32 render_flags);
#ifdef __cplusplus
}

void NuHGobjEvalAnimBlend2Root_3(nugscn_s *object, ani3_animheader_s *animation_a, f32 time_a,
                                 ani3_animheader_s *animation_b, f32 time_b, f32 blend, i32 override_count,
                                 NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data);
#endif
