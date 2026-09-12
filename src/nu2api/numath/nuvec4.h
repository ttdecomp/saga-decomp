#pragma once

#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"

typedef struct nuvec4_s {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
} NUVEC4;

typedef NUVEC4 NUVEC4_ALIGNED16 __attribute__((aligned(16)));

typedef struct nuhalfvec4_s {
    i16 x;
    i16 y;
    i16 z;
    i16 w;
} NUHALFVEC4;

#ifdef __cplusplus
f32 NuBez3EvaluateX(NUVEC4 *control, f32 t);
extern "C" {
#endif
    void NuVec4MtxTransform(NUVEC4 *v, NUVEC *v0, NUMTX *m0);
    void NuVec4ToNuHalfVec4(NUVEC4 *v, NUHALFVEC4 *out);
    void NuHalfVec4ToNuVec4(NUHALFVEC4 *v, NUVEC4 *out);
    void NuVec4Add(NUVEC4 *v, NUVEC4 *v0, NUVEC4 *v1);
    void NuVec4Scale(NUVEC4 *v, NUVEC4 *v0, f32 k);
    f32 NuVec4Dot(NUVEC4 *v0, NUVEC4 *v1);
    void NuVec4Lerp(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b, f32 t);
    f32 NuVec4Mag(NUVEC4 *v);
    f32 NuVec4MagSqr(NUVEC4 *v);
    void NuVec4Max(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b);
    void NuVec4Min(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b);
    void NuVec4ScaleAccum(NUVEC4 *out, NUVEC4 *v, f32 scale);
    void NuVec4ScaleXYZVU0(NUVEC4 *out, NUVEC4 *v, f32 scale);
    void NuVec4Sub(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b);
    void NuVec4MtxTransformH(NUVEC4 *v, NUVEC4 *v0, NUMTX *m0);
    void NuVec4MtxTransformVU0(NUVEC4 *out, NUVEC4 *in, NUMTX *matrix);
    void NuVec4MtxInvRotVU0(NUVEC4 *out, NUVEC4 *v, NUMTX *matrix);
    void NuVec4MtxInvTransformVU0(NUVEC4 *out, NUVEC *v, NUMTX *matrix);
    void NuVec4MtxRotateVU0(NUVEC4 *out, NUVEC4 *v, NUMTX *matrix);
    void NuVec4MtxTransformHVU0(NUVEC4 *out, NUVEC4 *v, NUMTX *matrix);
    void NuVec4MtxTransformVU0x2(NUVEC4 *out, NUVEC4 *v, NUMTX *matrix);
    void NuVec4MtxTransformVU0x3(NUVEC4 *out, NUVEC4 *in, NUMTX *matrix);
    void NuVec4MtxTransformVU0x4(NUVEC4 *out, NUVEC4 *v, NUMTX *matrix);
#ifdef __cplusplus
}
#endif
