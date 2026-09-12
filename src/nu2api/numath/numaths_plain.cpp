
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec4.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nucore/numemory.h"
#include <float.h>
#include <string.h>

f32 NuBez3EvaluateX(NUVEC4 *control, f32 t) {
    NUVEC4 powers;
    NUVEC4 coefficients;
    NUMTX basis = {-1.0f, 3.0f, -3.0f, 1.0f, 3.0f, -6.0f, 3.0f, 0.0f, -3.0f, 3.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    powers.x = t * t * t;
    powers.y = t * t;
    powers.z = t;
    powers.w = 1.0f;
    NuVec4MtxTransformH(&coefficients, control, &basis);
    return NuVec4Dot(&coefficients, &powers);
}

extern "C" {

    f32 NuLog10(f32 x) {
        f32 exponent;
        f32 i;
        f32 position;
        i32 index;
        f32 fraction;
        static i32 first_time = 1;
        static f32 *logtable;
        if (first_time) {
            logtable = (f32 *)NU_ALLOC(9000 * sizeof(f32), 4, 1, "", 0);
            if (!logtable)
                return 0.0f;
            for (i = 1000.0f; i <= 9999.0f; i += 1.0f)
                logtable[(i32)i - 1000] = log10(i / 1000.0f);
            first_time = 0;
        }
        if (x <= 0.0f)
            return FLT_MAX;
        exponent = 3.0f;
        if (x >= 10000.0f) {
            while (x > 10000.0f) {
                x *= 0.1f;
                exponent += 1.0f;
            }
        } else if (x < 1000.0f) {
            while (x < 1000.0f) {
                x *= 10.0f;
                exponent -= 1.0f;
            }
        }
        position = x - 1000.0f;
        index = (i32)position;
        fraction = position - index;
        if (index < 8999)
            return logtable[index + 1] * fraction + logtable[index] * (1.0f - fraction) + exponent;
        return logtable[index] + exponent;
    }

    f32 NuExp10(f32 x) {
        f32 result;
        i32 exponent;
        f32 i;
        f32 position;
        i32 index;
        f32 fraction;
        static i32 first_time = 1;
        static f32 *alogtable;
        if (first_time) {
            alogtable = (f32 *)NU_ALLOC(10000 * sizeof(f32), 4, 1, "", 0);
            if (!alogtable)
                return 0.0f;
            for (i = 0.0f; i <= 9999.0f; i += 1.0f)
                alogtable[(i32)i] = pow(10.0, i / 10000.0f);
            first_time = 0;
        }
        if (x == FLT_MAX)
            return FLT_MAX;
        exponent = (i32)x;
        x -= exponent;
        if (x < 0.0f) {
            --exponent;
            x += 1.0f;
        }
        position = x * 10000.0f;
        index = (i32)position;
        fraction = position - index;
        if (index < 9999)
            result = alogtable[index + 1] * fraction + alogtable[index] * (1.0f - fraction);
        else
            result = alogtable[index];
        if (exponent > 0) {
            while (exponent > 0) {
                result *= 10.0f;
                --exponent;
            }
        } else if (exponent < 0) {
            while (exponent < 0) {
                result *= 0.1f;
                ++exponent;
            }
        }
        return result;
    }

    NUVEC NuBezierCubicPatchEvaluatePartials(NUVEC *control, f32 u, f32 v, i32 order_u, i32 order_v) {
        u32 i, j;
        f32 remaining_u = 1.0f - u;
        f32 remaining_v = 1.0f - v;
        f32 weights_u[4], weights_v[4];
        NUVEC result;
        switch (order_u) {
            case 0:
                weights_u[0] = remaining_u * remaining_u * remaining_u;
                weights_u[1] = 3.0f * remaining_u * remaining_u * u;
                weights_u[2] = 3.0f * remaining_u * u * u;
                weights_u[3] = u * u * u;
                break;
            case 1:
                weights_u[0] = -3.0f * remaining_u * remaining_u;
                weights_u[1] = 9.0f * u * u - 9.0f;
                weights_u[2] = 6.0f * u - 9.0f * u * u;
                weights_u[3] = 3.0f * u * u;
                break;
            case 2:
                weights_u[0] = 6.0f - 6.0f * u;
                weights_u[1] = 18.0f * u - 12.0f;
                weights_u[2] = 6.0f - 18.0f * u;
                weights_u[3] = 6.0f * u;
                break;
            case 3:
                weights_u[0] = -6.0f;
                weights_u[1] = 18.0f;
                weights_u[2] = -18.0f;
                weights_u[3] = 6.0f;
                break;
        }
        switch (order_v) {
            case 0:
                weights_v[0] = remaining_v * remaining_v * remaining_v;
                weights_v[1] = 3.0f * remaining_v * remaining_v * v;
                weights_v[2] = 3.0f * remaining_v * v * v;
                weights_v[3] = v * v * v;
                break;
            case 1:
                weights_v[0] = -3.0f * remaining_v * remaining_v;
                weights_v[1] = 9.0f * v * v - 9.0f;
                weights_v[2] = 6.0f * v - 9.0f * v * v;
                weights_v[3] = 3.0f * v * v;
                break;
            case 2:
                weights_v[0] = 6.0f - 6.0f * v;
                weights_v[1] = 18.0f * v - 12.0f;
                weights_v[2] = 6.0f - 18.0f * v;
                weights_v[3] = 6.0f * v;
                break;
            case 3:
                weights_v[0] = -6.0f;
                weights_v[1] = 18.0f;
                weights_v[2] = -18.0f;
                weights_v[3] = 6.0f;
                break;
        }
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                NuVecScaleAccum(&result, control + i + j * 4, weights_u[i] * weights_v[j]);
            }
        }
        return result;
    }

    NUVEC NuBezierCubicPatchEvaluate(NUVEC *control, f32 u, f32 v) {
        u32 i, j;
        f32 remaining_u = 1.0f - u;
        f32 remaining_v = 1.0f - v;
        f32 weights_u[4], weights_v[4];
        NUVEC result;
        weights_u[0] = remaining_u * remaining_u * remaining_u;
        weights_u[1] = 3.0f * remaining_u * remaining_u * u;
        weights_u[2] = 3.0f * remaining_u * u * u;
        weights_u[3] = u * u * u;
        weights_v[0] = remaining_v * remaining_v * remaining_v;
        weights_v[1] = 3.0f * remaining_v * remaining_v * v;
        weights_v[2] = 3.0f * remaining_v * v * v;
        weights_v[3] = v * v * v;
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 4; ++j) {
                NuVecScaleAccum(&result, control + i + j * 4, weights_u[i] * weights_v[j]);
            }
        }
        return result;
    }

    void NuBez3Subdiv(f32 *control, f32 t, f32 *left, f32 *right) {
        f32 remaining = 1.0f - t;
        f32 middle;
        left[0] = control[0];
        left[1] = control[0] * remaining + control[1] * t;
        middle = control[1] * remaining + control[2] * t;
        left[2] = left[1] * remaining + middle * t;
        right[3] = control[3];
        right[2] = control[2] * remaining + control[3] * t;
        right[1] = middle * remaining + right[2] * t;
        left[3] = right[0] = left[2] * remaining + right[1] * t;
    }

    void NuCubicToBez3(f32 *cubic, f32 *bezier) {
        bezier[0] = cubic[3];
        bezier[1] = cubic[2] / 3.0f;
        bezier[2] = (cubic[1] + 2.0f * cubic[2]) / 3.0f;
        // The original writes offset 0x10, leaving element 3 untouched.
        bezier[4] = cubic[0] + cubic[1] + cubic[2] - cubic[3];
    }

    void NuBez3ToCubic(f32 *bezier, f32 *cubic) {
        cubic[0] = bezier[0] + 3.0f * bezier[1] - 3.0f * bezier[2] + bezier[3];
        cubic[1] = 3.0f * bezier[2] - 6.0f * bezier[1];
        // These repeated writes to element 0 are present in the original.
        cubic[0] = 3.0f * bezier[1];
        cubic[0] = bezier[0];
    }

    void NuBezierCubicPatchPartialsU(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -3.0f);
        NuVecScaleAccum(out0, &control[1], 3.0f);
        NuVecScale(out1, &control[12], -3.0f);
        NuVecScaleAccum(out1, &control[13], 3.0f);
        NuVecScale(out2, &control[2], -3.0f);
        NuVecScaleAccum(out2, &control[3], 3.0f);
        NuVecScale(out3, &control[14], -3.0f);
        NuVecScaleAccum(out3, &control[15], 3.0f);
    }

    void NuBezierCubicPatchPartialsUU(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], 6.0f);
        NuVecScaleAccum(out0, &control[1], -12.0f);
        NuVecScaleAccum(out0, &control[2], 6.0f);
        NuVecScale(out1, &control[12], 6.0f);
        NuVecScaleAccum(out1, &control[13], -12.0f);
        NuVecScaleAccum(out1, &control[14], 6.0f);
        NuVecScale(out2, &control[1], 6.0f);
        NuVecScaleAccum(out2, &control[2], -12.0f);
        NuVecScaleAccum(out2, &control[3], 6.0f);
        NuVecScale(out3, &control[13], 6.0f);
        NuVecScaleAccum(out3, &control[14], -12.0f);
        NuVecScaleAccum(out3, &control[15], 6.0f);
    }

    void NuBezierCubicPatchPartialsUUV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -18.0f);
        NuVecScaleAccum(out0, &control[1], 36.0f);
        NuVecScaleAccum(out0, &control[2], -18.0f);
        NuVecScaleAccum(out0, &control[4], 18.0f);
        NuVecScaleAccum(out0, &control[5], -36.0f);
        NuVecScaleAccum(out0, &control[6], 18.0f);
        NuVecScale(out1, &control[8], -18.0f);
        NuVecScaleAccum(out1, &control[9], 36.0f);
        NuVecScaleAccum(out1, &control[10], -18.0f);
        NuVecScaleAccum(out1, &control[12], 18.0f);
        NuVecScaleAccum(out1, &control[13], -36.0f);
        NuVecScaleAccum(out1, &control[14], 18.0f);
        NuVecScale(out2, &control[1], -18.0f);
        NuVecScaleAccum(out2, &control[2], 36.0f);
        NuVecScaleAccum(out2, &control[3], -18.0f);
        NuVecScaleAccum(out2, &control[5], 18.0f);
        NuVecScaleAccum(out2, &control[6], -36.0f);
        NuVecScaleAccum(out2, &control[7], 18.0f);
        NuVecScale(out3, &control[9], -18.0f);
        NuVecScaleAccum(out3, &control[10], 36.0f);
        NuVecScaleAccum(out3, &control[11], -18.0f);
        NuVecScaleAccum(out3, &control[13], 18.0f);
        NuVecScaleAccum(out3, &control[14], -36.0f);
        NuVecScaleAccum(out3, &control[15], 18.0f);
    }

    void NuBezierCubicPatchPartialsUVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -18.0f);
        NuVecScaleAccum(out0, &control[1], 18.0f);
        NuVecScaleAccum(out0, &control[4], 36.0f);
        NuVecScaleAccum(out0, &control[5], -36.0f);
        NuVecScaleAccum(out0, &control[8], -18.0f);
        NuVecScaleAccum(out0, &control[9], 18.0f);
        NuVecScale(out1, &control[4], -18.0f);
        NuVecScaleAccum(out1, &control[5], 18.0f);
        NuVecScaleAccum(out1, &control[8], 36.0f);
        NuVecScaleAccum(out1, &control[9], -36.0f);
        NuVecScaleAccum(out1, &control[12], -18.0f);
        NuVecScaleAccum(out1, &control[13], 18.0f);
        NuVecScale(out2, &control[2], -18.0f);
        NuVecScaleAccum(out2, &control[3], 18.0f);
        NuVecScaleAccum(out2, &control[6], 36.0f);
        NuVecScaleAccum(out2, &control[7], -36.0f);
        NuVecScaleAccum(out2, &control[10], -18.0f);
        NuVecScaleAccum(out2, &control[11], 18.0f);
        NuVecScale(out3, &control[6], -18.0f);
        NuVecScaleAccum(out3, &control[7], 18.0f);
        NuVecScaleAccum(out3, &control[10], 36.0f);
        NuVecScaleAccum(out3, &control[11], -36.0f);
        NuVecScaleAccum(out3, &control[14], -18.0f);
        NuVecScaleAccum(out3, &control[15], 18.0f);
    }

    void NuBezierCubicPatchPartialsV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -3.0f);
        NuVecScaleAccum(out0, &control[4], 3.0f);
        NuVecScale(out1, &control[8], -3.0f);
        NuVecScaleAccum(out1, &control[12], 3.0f);
        NuVecScale(out2, &control[3], -3.0f);
        NuVecScaleAccum(out2, &control[7], 3.0f);
        NuVecScale(out3, &control[11], -3.0f);
        NuVecScaleAccum(out3, &control[15], 3.0f);
    }

    void NuBezierCubicPatchPartialsVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], 6.0f);
        NuVecScaleAccum(out0, &control[4], -12.0f);
        NuVecScaleAccum(out0, &control[8], 6.0f);
        NuVecScale(out1, &control[4], 6.0f);
        NuVecScaleAccum(out1, &control[8], -12.0f);
        NuVecScaleAccum(out1, &control[12], 6.0f);
        NuVecScale(out2, &control[3], 6.0f);
        NuVecScaleAccum(out2, &control[7], -12.0f);
        NuVecScaleAccum(out2, &control[11], 6.0f);
        NuVecScale(out3, &control[7], 6.0f);
        NuVecScaleAccum(out3, &control[11], -12.0f);
        NuVecScaleAccum(out3, &control[15], 6.0f);
    }

    void NuBezierCubicPatchPartialsUUU(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -6.0f);
        NuVecScaleAccum(out0, &control[1], 18.0f);
        NuVecScaleAccum(out0, &control[2], -18.0f);
        NuVecScaleAccum(out0, &control[3], 6.0f);
        *out2 = *out0;
        NuVecScale(out3, &control[12], -6.0f);
        NuVecScaleAccum(out3, &control[13], 18.0f);
        NuVecScaleAccum(out3, &control[14], -18.0f);
        NuVecScaleAccum(out3, &control[15], 6.0f);
        *out1 = *out3;
    }

    void NuBezierCubicPatchPartialsVVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -6.0f);
        NuVecScaleAccum(out0, &control[4], 18.0f);
        NuVecScaleAccum(out0, &control[8], -18.0f);
        NuVecScaleAccum(out0, &control[12], 6.0f);
        *out1 = *out0;
        NuVecScale(out3, &control[3], -6.0f);
        NuVecScaleAccum(out3, &control[7], 18.0f);
        NuVecScaleAccum(out3, &control[11], -18.0f);
        NuVecScaleAccum(out3, &control[15], 6.0f);
        *out2 = *out3;
    }

    void NuBezierCubicPatchPartialsUUVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], 36.0f);
        NuVecScaleAccum(out0, &control[1], -72.0f);
        NuVecScaleAccum(out0, &control[2], 36.0f);
        NuVecScaleAccum(out0, &control[4], -72.0f);
        NuVecScaleAccum(out0, &control[5], 144.0f);
        NuVecScaleAccum(out0, &control[6], -72.0f);
        NuVecScaleAccum(out0, &control[8], 36.0f);
        NuVecScaleAccum(out0, &control[9], -72.0f);
        NuVecScaleAccum(out0, &control[10], 36.0f);
        NuVecScale(out1, &control[4], 36.0f);
        NuVecScaleAccum(out1, &control[5], -72.0f);
        NuVecScaleAccum(out1, &control[6], 36.0f);
        NuVecScaleAccum(out1, &control[8], -72.0f);
        NuVecScaleAccum(out1, &control[9], 144.0f);
        NuVecScaleAccum(out1, &control[10], -72.0f);
        NuVecScaleAccum(out1, &control[12], 36.0f);
        NuVecScaleAccum(out1, &control[13], -72.0f);
        NuVecScaleAccum(out1, &control[14], 36.0f);
        NuVecScale(out2, &control[1], 36.0f);
        NuVecScaleAccum(out2, &control[2], -72.0f);
        NuVecScaleAccum(out2, &control[3], 36.0f);
        NuVecScaleAccum(out2, &control[5], -72.0f);
        NuVecScaleAccum(out2, &control[6], 144.0f);
        NuVecScaleAccum(out2, &control[7], -72.0f);
        NuVecScaleAccum(out2, &control[9], 36.0f);
        NuVecScaleAccum(out2, &control[10], -72.0f);
        NuVecScaleAccum(out2, &control[11], 36.0f);
        NuVecScale(out3, &control[5], 36.0f);
        NuVecScaleAccum(out3, &control[6], -72.0f);
        NuVecScaleAccum(out3, &control[7], 36.0f);
        NuVecScaleAccum(out3, &control[9], -72.0f);
        NuVecScaleAccum(out3, &control[10], 144.0f);
        NuVecScaleAccum(out3, &control[11], -72.0f);
        NuVecScaleAccum(out3, &control[13], 36.0f);
        NuVecScaleAccum(out3, &control[14], -72.0f);
        NuVecScaleAccum(out3, &control[15], 36.0f);
    }

    void NuBezierCubicPatchPartialsUUVVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3) {
        NuVecScale(out0, &control[0], -36.0f);
        NuVecScaleAccum(out0, &control[1], 72.0f);
        NuVecScaleAccum(out0, &control[2], -36.0f);
        NuVecScaleAccum(out0, &control[4], 108.0f);
        NuVecScaleAccum(out0, &control[5], -216.0f);
        NuVecScaleAccum(out0, &control[6], 108.0f);
        NuVecScaleAccum(out0, &control[8], -108.0f);
        NuVecScaleAccum(out0, &control[9], 216.0f);
        NuVecScaleAccum(out0, &control[10], -108.0f);
        NuVecScaleAccum(out0, &control[12], 36.0f);
        NuVecScaleAccum(out0, &control[13], -72.0f);
        NuVecScaleAccum(out0, &control[14], 36.0f);
        *out1 = *out0;
        NuVecScale(out3, &control[1], -36.0f);
        NuVecScaleAccum(out3, &control[2], 72.0f);
        NuVecScaleAccum(out3, &control[3], -36.0f);
        NuVecScaleAccum(out3, &control[5], 108.0f);
        NuVecScaleAccum(out3, &control[6], -216.0f);
        NuVecScaleAccum(out3, &control[7], 108.0f);
        NuVecScaleAccum(out3, &control[9], -108.0f);
        NuVecScaleAccum(out3, &control[10], 216.0f);
        NuVecScaleAccum(out3, &control[11], -108.0f);
        NuVecScaleAccum(out3, &control[13], 36.0f);
        NuVecScaleAccum(out3, &control[14], -72.0f);
        NuVecScaleAccum(out3, &control[15], 36.0f);
        *out2 = *out3;
    }

    NUVEC NuBezierQuadraticTriangleEvaluateBarycentric(NUVEC *control, f32 u, f32 v, f32 w) {
        NUVEC result;
        NuVecScale(&result, &control[0], u * u);
        NuVecScaleAccum(&result, &control[1], 2.0f * u * v);
        NuVecScaleAccum(&result, &control[2], 2.0f * u * w);
        NuVecScaleAccum(&result, &control[3], v * v);
        NuVecScaleAccum(&result, &control[4], 2.0f * v * w);
        NuVecScaleAccum(&result, &control[5], w * w);
        return result;
    }

    NUVEC NuBezierQuadraticTriangleEvaluateParametric(NUVEC *control, f32 u, f32 v) {
        f32 w = 1.0f - u - v;
        NUVEC result;
        NuVecScale(&result, &control[0], v * v);
        NuVecScaleAccum(&result, &control[1], 2.0f * v * w);
        NuVecScaleAccum(&result, &control[2], 2.0f * u * v);
        NuVecScaleAccum(&result, &control[3], w * w);
        NuVecScaleAccum(&result, &control[4], 2.0f * w * u);
        NuVecScaleAccum(&result, &control[5], u * u);
        return result;
    }

    i32 NuMiscNextPow2Exp(i32 value) {
        i32 shift = 0;
        while ((1 << shift) < value)
            ++shift;
        return shift;
    }

    i32 NuMiscPow2Exp(i32 value) {
        i32 shift = 0;
        while (value > 1) {
            value >>= 1;
            ++shift;
        }
        return shift;
    }

    void NuMtxInvVU0(NUMTX *out, NUMTX *in) {
        f32 x = -in->m30;
        f32 y = -in->m31;
        f32 z = -in->m32;
        f32 temp = in->m01;
        out->m01 = in->m10;
        out->m10 = temp;
        temp = in->m02;
        out->m02 = in->m20;
        out->m20 = temp;
        temp = in->m12;
        out->m12 = in->m21;
        out->m21 = temp;
        out->m00 = in->m00;
        out->m11 = in->m11;
        out->m22 = in->m22;
        out->m30 = out->m00 * x + out->m10 * y + out->m20 * z;
        out->m31 = out->m01 * x + out->m11 * y + out->m21 * z;
        out->m32 = out->m02 * x + out->m12 * y + out->m22 * z;
        out->m03 = out->m13 = out->m23 = 0.0f;
        out->m33 = 1.0f;
    }

    void NuMtxMulArrayVU0(NUMTX *out, NUMTX *left, NUMTX *right, i32 count) {
        for (i32 i = 0; i < count; ++i)
            NuMtxMulH(&out[i], &left[i], &right[i]);
    }

    void NuMtxMulnVU0(NUMTX *out, NUMTX *left, NUMTX **right) {
        NuMtxMul(out, left, *right);
    }

    void NuMtxPreScaleUVU0(NUMTX *matrix, f32 scale) {
        matrix->m00 *= scale;
        matrix->m01 *= scale;
        matrix->m02 *= scale;
        matrix->m10 *= scale;
        matrix->m11 *= scale;
        matrix->m12 *= scale;
        matrix->m20 *= scale;
        matrix->m21 *= scale;
        matrix->m22 *= scale;
    }

    NUVEC NuBezierQuadraticTrianglePartialsUU(NUVEC *control) {
        NUVEC result;
        NuVecScale(&result, &control[3], 2.0f);
        NuVecScaleAccum(&result, &control[4], -4.0f);
        NuVecScaleAccum(&result, &control[5], 2.0f);
        return result;
    }

    NUVEC NuBezierQuadraticTrianglePartialsVV(NUVEC *control) {
        NUVEC result;
        NuVecScale(&result, &control[0], 2.0f);
        NuVecScaleAccum(&result, &control[1], -4.0f);
        NuVecScaleAccum(&result, &control[3], 2.0f);
        return result;
    }

    NUVEC NuBezierQuadraticTrianglePartialsUeV(NUVEC *control) {
        NUVEC result;
        NuVecScale(&result, &control[0], 2.0f);
        NuVecScaleAccum(&result, &control[2], -4.0f);
        NuVecScaleAccum(&result, &control[5], 2.0f);
        return result;
    }

    f32 NuCeil(f32 value) {
        f32 lower = NuFloor(value);
        return lower != value ? lower + 1.0f : value;
    }

    f32 NuFrsqrt(f32 value) {
        return value <= 0.0f ? 0.0f : 1.0f / sqrtf(value);
    }

    i32 NuMiscNextPow2(i32 value) {
        i32 shift = 0;
        while ((1 << shift) < value)
            ++shift;
        return 1 << shift;
    }

    f32 NuHalfToFloat(i16 value) {
        u16 bits = value;
        if (bits == 0)
            return 0.0f;
        i32 sign = bits >> 15;
        i32 exponent = (bits >> 10) & 31;
        i32 mantissa = bits & 0x3ff;
        exponent += 112;
        union {
            u32 bits;
            f32 value;
        } result;
        result.bits = (u32(sign) << 31) | (exponent << 23) | (mantissa << 13);
        return result.value;
    }

    i16 NuFloatToHalf(f32 value) {
        i32 exponent;
        u32 bits = *reinterpret_cast<const u32 *>(&value);
        i32 sign = bits >> 31;
        exponent = (bits >> 23) & 0xff;
        i32 mantissa = bits & 0x7fffff;
        // The original clamps the exponent and truncates the mantissa;
        // it does not implement IEEE half rounding or denormal handling.
        exponent -= 112;
        if (exponent < 0)
            exponent = 0;
        if (exponent > 31)
            exponent = 31;
        i16 result = (sign << 15) | ((exponent & 31) << 10) | (mantissa >> 13);
        return *reinterpret_cast<const u16 *>(&result);
    }
    void NuHalfVec4ToNuVec4(NUHALFVEC4 *v, NUVEC4 *out) {
        out->x = NuHalfToFloat(v->x);
        out->y = NuHalfToFloat(v->y);
        out->z = NuHalfToFloat(v->z);
        out->w = NuHalfToFloat(v->w);
    }
    void NuVec4Lerp(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b, f32 t) {
        out->x = b->x + (a->x - b->x) * t;
        out->y = b->y + (a->y - b->y) * t;
        out->z = b->z + (a->z - b->z) * t;
        out->w = b->w + (a->w - b->w) * t;
    }
    f32 NuVec4Mag(NUVEC4 *v) {
        return NuFsqrt(v->w * v->w + v->x * v->x + v->y * v->y + v->z * v->z);
    }
    f32 NuVec4MagSqr(NUVEC4 *v) {
        return v->w * v->w + v->x * v->x + v->y * v->y + v->z * v->z;
    }
    void NuVec4Max(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b) {
        if (a->x > b->x)
            out->x = a->x;
        else
            out->x = b->x;
        if (a->y > b->y)
            out->y = a->y;
        else
            out->y = b->y;
        if (a->z > b->z)
            out->z = a->z;
        else
            out->z = b->z;
        if (a->w > b->w)
            out->w = a->w;
        else
            out->w = b->w;
    }
    void NuVec4Min(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b) {
        if (a->x < b->x)
            out->x = a->x;
        else
            out->x = b->x;
        if (a->y < b->y)
            out->y = a->y;
        else
            out->y = b->y;
        if (a->z < b->z)
            out->z = a->z;
        else
            out->z = b->z;
        if (a->w < b->w)
            out->w = a->w;
        else
            out->w = b->w;
    }
    void NuVec4MtxInvRotVU0(NUVEC4 *out, NUVEC4 *v, NUMTX *m) {
        f32 y = v->x * m->m10 + v->y * m->m11 + v->z * m->m12;
        f32 z = v->x * m->m20 + v->y * m->m21 + v->z * m->m22;
        f32 w = v->x * m->m30 + v->y * m->m31 + v->z * m->m32;
        out->x = v->x * m->m00 + v->y * m->m01 + v->z * m->m02;
        out->y = y;
        out->z = z;
        out->w = w;
    }
    void NuVec4MtxInvTransformVU0(NUVEC4 *out, NUVEC *v, NUMTX *matrix) {
        NUMTX inverse;
        NuMtxInv(&inverse, matrix);
        NuVec4MtxTransform(out, v, &inverse);
    }
    void NuVec4MtxRotateVU0(NUVEC4 *out, NUVEC4 *v, NUMTX *m) {
        f32 x = v->x * m->m00 + v->y * m->m10 + v->z * m->m20;
        f32 y = v->x * m->m01 + v->y * m->m11 + v->z * m->m21;
        f32 z = v->x * m->m02 + v->y * m->m12 + v->z * m->m22;
        out->x = x;
        out->y = y;
        out->z = z;
        out->w = v->w;
    }
    void NuVec4MtxTransformHVU0(NUVEC4 *out, NUVEC4 *v, NUMTX *matrix) {
        NuVec4MtxTransformH(out, v, matrix);
    }
    void NuVec4MtxTransformVU0(NUVEC4 *out, NUVEC4 *in, NUMTX *matrix) {
        f32 x = in->x * matrix->m00 + in->y * matrix->m10 + in->z * matrix->m20 + in->w * matrix->m30;
        f32 y = in->x * matrix->m01 + in->y * matrix->m11 + in->z * matrix->m21 + in->w * matrix->m31;
        f32 z = in->x * matrix->m02 + in->y * matrix->m12 + in->z * matrix->m22 + in->w * matrix->m32;
        f32 w = in->x * matrix->m03 + in->y * matrix->m13 + in->z * matrix->m23 + in->w * matrix->m33;
        out->x = x;
        out->y = y;
        out->z = z;
        out->w = w;
    }
    void NuVec4MtxTransformVU0x2(NUVEC4 *out, NUVEC4 *v, NUMTX *m) {
        for (i32 i = 0; i < 2; ++i) {
            f32 x = v->x * m->m00 + v->y * m->m10 + v->z * m->m20 + v->w * m->m30;
            f32 y = v->x * m->m01 + v->y * m->m11 + v->z * m->m21 + v->w * m->m31;
            f32 z = v->x * m->m02 + v->y * m->m12 + v->z * m->m22 + v->w * m->m32;
            f32 w = v->x * m->m03 + v->y * m->m13 + v->z * m->m23 + v->w * m->m33;
            out->x = x;
            out->y = y;
            out->z = z;
            out->w = w;
            ++out;
            ++v;
        }
    }
    void NuVec4MtxTransformVU0x3(NUVEC4 *out, NUVEC4 *v, NUMTX *m) {
        for (i32 i = 0; i < 3; ++i) {
            f32 x = v->x * m->m00 + v->y * m->m10 + v->z * m->m20 + v->w * m->m30;
            f32 y = v->x * m->m01 + v->y * m->m11 + v->z * m->m21 + v->w * m->m31;
            f32 z = v->x * m->m02 + v->y * m->m12 + v->z * m->m22 + v->w * m->m32;
            f32 w = v->x * m->m03 + v->y * m->m13 + v->z * m->m23 + v->w * m->m33;
            out->x = x;
            out->y = y;
            out->z = z;
            out->w = w;
            ++out;
            ++v;
        }
    }
    void NuVec4MtxTransformVU0x4(NUVEC4 *out, NUVEC4 *v, NUMTX *m) {
        for (i32 i = 0; i < 4; ++i) {
            f32 x = v->x * m->m00 + v->y * m->m10 + v->z * m->m20 + v->w * m->m30;
            f32 y = v->x * m->m01 + v->y * m->m11 + v->z * m->m21 + v->w * m->m31;
            f32 z = v->x * m->m02 + v->y * m->m12 + v->z * m->m22 + v->w * m->m32;
            f32 w = v->x * m->m03 + v->y * m->m13 + v->z * m->m23 + v->w * m->m33;
            out->x = x;
            out->y = y;
            out->z = z;
            out->w = w;
            ++out;
            ++v;
        }
    }
    void NuVec4ScaleAccum(NUVEC4 *out, NUVEC4 *v, f32 scale) {
        out->x += v->x * scale;
        out->y += v->y * scale;
        out->z += v->z * scale;
        out->w += v->w * scale;
    }
    void NuVec4ScaleXYZVU0(NUVEC4 *out, NUVEC4 *v, f32 scale) {
        NuVec4Scale(out, v, scale);
    }
    void NuVec4Sub(NUVEC4 *out, NUVEC4 *a, NUVEC4 *b) {
        out->x = a->x - b->x;
        out->y = a->y - b->y;
        out->z = a->z - b->z;
        out->w = a->w - b->w;
    }
    void NuVec4ToNuHalfVec4(NUVEC4 *v, NUHALFVEC4 *out) {
        out->x = NuFloatToHalf(v->x);
        out->y = NuFloatToHalf(v->y);
        out->z = NuFloatToHalf(v->z);
        out->w = NuFloatToHalf(v->w);
    }
    void NuVecConvertToIntVU0(void) {
    }
    f32 NuVecDiffSqrVU0(NUVEC *a, NUVEC *b) {
        f32 x = a->x - b->x;
        f32 y = a->y - b->y;
        f32 z = a->z - b->z;
        f32 squared = x * x + y * y + z * z;
        return squared;
    }
    f32 NuVecDiffVU0(NUVEC *a, NUVEC *b) {
        f32 x = a->x - b->x;
        f32 y = a->y - b->y;
        f32 z = a->z - b->z;
        f32 squared = x * x + y * y + z * z;
        return NuFsqrt(squared);
    }
    // The reference ignores the scalar argument and rereads the destination
    // axis after each store. Preserve that sequential behavior.
    void NuVecInvMtxRotateValX(NUVEC *out, f32, NUMTX *matrix) {
        out->x = out->x * matrix->m00;
        out->y = out->x * matrix->m10;
        out->z = out->x * matrix->m20;
    }
    void NuVecInvMtxRotateValY(NUVEC *out, f32, NUMTX *matrix) {
        out->x = out->y * matrix->m01;
        out->y = out->y * matrix->m11;
        out->z = out->y * matrix->m21;
    }
    void NuVecInvMtxRotateValZ(NUVEC *out, f32, NUMTX *matrix) {
        out->x = out->z * matrix->m02;
        out->y = out->z * matrix->m12;
        out->z = out->z * matrix->m22;
    }
    void NuVecInvMtxScale(NUVEC *out, NUVEC *v, NUMTX *matrix) {
        out->x = v->x / matrix->m00;
        out->y = v->y / matrix->m11;
        out->z = v->z / matrix->m22;
    }
    void NuVecInvMtxTransformVU0(NUVEC *out, NUVEC *v, NUMTX *m) {
        f32 x = v->x - m->m30;
        f32 y = v->y - m->m31;
        f32 z = v->z - m->m32;
        out->x = m->m00 * x + m->m01 * y + m->m02 * z;
        out->y = m->m10 * x + m->m11 * y + m->m12 * z;
        out->z = m->m20 * x + m->m21 * y + m->m22 * z;
    }
    void NuVecInvMtxTranslate(NUVEC *out, NUVEC *v, NUMTX *matrix) {
        out->x = v->x - matrix->m30;
        out->y = v->y - matrix->m31;
        out->z = v->z - matrix->m32;
    }
    f32 NuVecMagVU0(NUVEC *v) {
        f32 magnitude = NuFsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
        return magnitude;
    }
    void NuVecMtxRotateH(NUVEC *out, NUVEC *v, NUMTX *m) {
        f32 reciprocal_w = 1.0f / (v->x * m->m03 + v->y * m->m13 + v->z * m->m23);
        f32 y = (v->x * m->m01 + v->y * m->m11 + v->z * m->m21) * reciprocal_w;
        f32 z = (v->x * m->m02 + v->y * m->m12 + v->z * m->m22) * reciprocal_w;
        out->x = (v->x * m->m00 + v->y * m->m10 + v->z * m->m20) * reciprocal_w;
        out->y = y;
        out->z = z;
    }
    void NuVecMtxRotateValX(NUVEC *out, f32 value, NUMTX *matrix) {
        out->x = matrix->m00 * value;
        out->y = matrix->m01 * value;
        out->z = matrix->m02 * value;
    }
    void NuVecMtxRotateValY(NUVEC *out, f32 value, NUMTX *matrix) {
        out->x = matrix->m10 * value;
        out->y = matrix->m11 * value;
        out->z = matrix->m12 * value;
    }
    void NuVecMtxRotateValZ(NUVEC *out, f32 value, NUMTX *matrix) {
        out->x = matrix->m20 * value;
        out->y = matrix->m21 * value;
        out->z = matrix->m22 * value;
    }
    void NuVecMtxTransformVU0(NUVEC *out, NUVEC *input, NUMTX *matrix) {
        NuVecMtxTransform(out, input, matrix);
    }
    f32 NuVecNormVU0(NUVEC *out, NUVEC *v) {
        f32 magnitude = NuFsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
        NUVEC normalized;
        if (magnitude != 0.0f) {
            normalized.x = v->x / magnitude;
            normalized.y = v->y / magnitude;
            normalized.z = v->z / magnitude;
        } else {
            normalized.x = 0.0f;
            normalized.y = 0.0f;
            normalized.z = 0.0f;
        }
        out->x = normalized.x;
        out->y = normalized.y;
        out->z = normalized.z;
        return magnitude;
    }
    void NuVecRotateYValX(NUVEC *out, f32 value, NUANG angle) {
        f32 cosine = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff];
        f32 sine = NuTrigTable[(angle >> 1) & 0x7fff];
        out->x = value * cosine;
        out->y = 0.0f;
        out->z = -value * sine;
    }
}

u32 NuVecToRGBA(NUVEC *v, f32 alpha) {
    u32 red = (v->x + 1.0f) * 127.5f;
    u32 green = (v->y + 1.0f) * 127.5f;
    u32 blue = (v->z + 1.0f) * 127.5f;
    u32 opacity = alpha * 255.0f;
    return (opacity << 24) + (red << 16) + (green << 8) + blue;
}
