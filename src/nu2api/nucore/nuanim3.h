#pragma once

#include "decomp.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/fixed_width.h"
#include "nu2api/numath/nuvec.h"

struct ani3_animheader_s;
struct nuanimbuff_s;
struct nuanimdata_s;
struct nuanimtime_s;
struct numtx_s;
struct nuhgobjjoint_s;
struct NUJOINTANIM_s;
struct nuquat_s;
struct nugscn_s;
using NUANIMBUFFEVALUATECB = void (*)(struct numtx_s *, void *, nuquat_s *);

void NuAnimBuffEvaluate_3_QuatB(numtx_s *base, nuanimbuff_s *buffer, nugscn_s *scene, numtx_s *matrices,
                                ani3_animheader_s *animation,
                                void (*root_fn)(numtx_s *, void *, nuvec_s *, nuvec_s *, nuvec_s *, float),
                                nuvec_s *root_translation, void *root_data);
u32 NuAnimGetAnimDataSizeANI3(ani3_animheader_s *animation);
void NuAnimRelocatePtrsANI3(ani3_animheader_s *animation, i32 offset);

enum ANI3_MAGIC : u32 {
    ANI3_MAGIC_VERSION_4 = 0x414e4934,
    ANI3_MAGIC_VERSION_5 = 0x414e4935,
};

enum ANI3_FORMAT_FLAGS : u8 {
    ANI3_FORMAT_QUATERNION_ROTATION = 1 << 0,
    ANI3_FORMAT_QUATERNION_STORES_W = 1 << 1,
};

enum NUANIM_NODE_FLAGS : u8 {
    NUANIM_NODE_HAS_ROTATION = 1 << 0,
    NUANIM_NODE_HAS_TRANSLATION = 1 << 1,
    NUANIM_NODE_HAS_SCALE = 1 << 3,
    NUANIM_NODE_COMPENSATE_PARENT_SCALE = 1 << 4,
};

struct ani3_scalemin_s {
    float scale;
    float minimum;
};

struct nuanimcurvedata_s {
    u32 *key_mask;
    u16 *key_offsets;
    void *key_data;
};

union nuanimcurve2data_u {
    float constant;
    nuanimcurvedata_s *curvedata;
};

struct nuanimkey_s {
    f32 time, reciprocal_span, tangent, value;
};
struct nuanimcurve_s {
    u8 key_mask[4];
    nuanimkey_s *keys;
    i32 key_count;
    u32 flags;
};
struct nuanimcurveset_s {
    u32 flags;
    f32 *constants;
    nuanimcurve_s **curves;
    u8 curve_count;
    u8 padding[3];
};

struct nuanimcurve2_s {
    nuanimcurve2data_u data;
};

struct nuanimdata2_s {
    float duration;
    u16 node_count;
    u16 curve_count;
    i16 chunk_count;
    u16 field_0a;
    nuanimcurve2_s *curves;
    u8 *curve_types;
    u8 *node_flags;
};

#ifdef __cplusplus
extern "C" {
#endif
    void *NuAnimData2Fixup(i32 file_size, void **data);
    void *NuAnimData2LoadBuffEx(char *path, VARIPTR *buf, VARIPTR *buf_end, void **result);
    void *NuAnimData2LoadBuff(char *path, VARIPTR *buf, VARIPTR *buf_end);
    void *NuAnimData2LoadBuffFromPAK(void *data, i32 file_size);
    f32 *NuAnimCurveExtractAllNodeCurves_3(ani3_animheader_s *animation, i32 node, f32 frame, char *curve_mask);
    void NuAnimData2CalcMatrix(struct nuanimdata_s *animation, i32 node, f32 frame, struct numtx_s *matrix);
    void NuAnimBuffEvaluateCallback(NUANIMBUFFEVALUATECB callback, void **data, i32 *joints);
    void *NuAnimBuffCreate(i32 max_joints, VARIPTR *buf);
    void NuAnimCurveSetDestroy(void *curve_set, i32 destroy_curves);
    extern NUANIMBUFFEVALUATECB AnimBuffEvalCB;
    extern void **AnimBuffEvalData;
    extern i32 *AnimBuffEvalJoint;
    i32 NuAnimSetUseQuatsFlag(i32 enabled);
    i32 NuAnimGetUseQuatsFlag(void);
    i32 NuAnimPushSetUseQuatsFlag(i32 enabled);
    i32 NuAnimPopUseQuatsFlag(void);
    void *NuAnimGetAnimLOD(void *animation, i32 lod);
    i32 NuAnimNumNodes(void *animation);
    f32 NuAnimCurve2CalcValEx(nuanimcurve2_s *curve, nuanimtime_s *time, u32 type);
    f32 NuAnimCurveCalcVal2(nuanimcurve_s *curve, nuanimtime_s *time);
    void NuAnimCurveSetApplyToMatrix(nuanimcurveset_s *set, nuanimtime_s *time, struct numtx_s *matrix);
    void NuAnimCurve2SetApplyToJointTransLoc(nuanimcurve2_s *curves, i8 *types, i8 flags, nuanimtime_s *time,
                                             nuhgobjjoint_s *joint, NUVEC *scale, NUVEC *parent_scale, numtx_s *matrix,
                                             NUJOINTANIM_s *override_anim, NUVEC *root_translation,
                                             NUVEC *locator_translation);
    void NuAnimCurve2SetApplyToJoint(nuanimcurve2_s *curves, i8 *types, i8 flags, nuanimtime_s *time,
                                     nuhgobjjoint_s *joint, NUVEC *scale, NUVEC *parent_scale, numtx_s *matrix,
                                     NUJOINTANIM_s *override_anim);
    void NuAnimCurveSetApplyBlendToJoint2(nuanimcurveset_s *first, nuanimtime_s *first_time, nuanimcurveset_s *second,
                                          nuanimtime_s *second_time, f32 blend, nuhgobjjoint_s *joint, NUVEC *scale,
                                          NUVEC *parent_scale, numtx_s *matrix, NUJOINTANIM_s *override_anim);
    f32 NuAnimEndFrame(void *animation);
    f32 NuAnimEndFrameOld(void *animation);
    void ANI_Ani3ExtractAllNodeCurves(ani3_animheader_s *anim, f32 frame, f32 *values, i32 node, char *curve_mask);
    i32 ANI_SimpleAni3PlayerV4Joint(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                    i32 first_joint);
    void ANI_SimpleAni3PlayerV4Joint_Blend(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend,
                                           i32 joint_count, i32 first_joint, NUVEC *root_translation);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
i32 ANI_SimpleAni3PlayerV4Joint_Quat3(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                      i32 first_joint);
i32 ANI_SimpleAni3PlayerV4Joint_Quat3W(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                       i32 first_joint);
extern void *globalbuffer;
extern i32 MaxAnimJoints;
#endif

struct nuanimtime_s {
    float time;
    float time_offset;
    i32 chunk;
    u32 time_mask;
    u32 time_byte;
    u32 chunk_frame;
};

struct ani3_animheader_s {
    u32 magic;
    u16 node_count;
    u16 key_count;
    u16 key_stride;
    u16 frame_count;
    u16 curve_count;
    u16 first_frame;
    u8 end_frame;
    u8 constant_index;
    union {
        u16 field_12;
        struct {
            u8 field_12_low;
            u8 format_flags;
        };
    };
    u16 next_block;
    u16 declared_end_frame;
    u8 pad_18[4];
    float minimum;
    float scale;
    ani3_scalemin_s *scale_min;
    i16 *constants;
    u16 *curve_types;
    u8 *keys;
    u8 *node_flags;
    void *field_38;
};

// One decompressed joint sample in the ANI4/ANI5 animation buffer.  The
// player stores translation, Euler rotation and scale as three aligned
// vectors; NuAnimBuffEvaluate_3 consumes the same 0x30-byte stride.
struct nuanimbuffjoint_s {
    NUVEC translation;
    f32 translation_w;
    NUVEC rotation;
    f32 rotation_w;
    NUVEC scale;
    f32 scale_w;
};

enum NUANIMBUFF_JOINT_FLAGS : u8 {
    NUANIMBUFF_JOINT_ROTATION = 0x01,
    NUANIMBUFF_JOINT_TRANSLATION = 0x02,
    NUANIMBUFF_JOINT_SCALE = 0x08,
    NUANIMBUFF_JOINT_CANCEL_PARENT_SCALE = 0x10,
    NUANIMBUFF_JOINT_BIND_MATRIX = 0x20,
};

struct nuanimbuff_s {
    i32 joint_count;
    i16 max_joints;
    u8 use_quaternions;
    u8 pad_07;
    nuanimbuffjoint_s *joints;
    u8 *joint_flags;
};

DECOMP_ASSERT(sizeof(ani3_animheader_s) == 0x3c, "ANI4 header must match the original x86 layout");
DECOMP_ASSERT(sizeof(nuanimcurvedata_s) == 0x0c, "animation curve data must match the original x86 layout");
DECOMP_ASSERT(sizeof(nuanimcurve2_s) == 0x04, "animation curve must match the original x86 layout");
DECOMP_ASSERT(sizeof(nuanimdata2_s) == 0x18, "animation header must match the original x86 layout");
DECOMP_ASSERT(sizeof(nuanimtime_s) == 0x18, "animation time must match the original x86 layout");
DECOMP_ASSERT(sizeof(nuanimbuffjoint_s) == 0x30, "animation-buffer joint must match the original x86 layout");
DECOMP_ASSERT(sizeof(nuanimbuff_s) == 0x10, "animation buffer must match the original x86 layout");
