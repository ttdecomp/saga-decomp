#include "decomp.h"

#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numath/nuvec.h"

static void VuQuatCopy(NUQUAT *dst, NUQUAT *src) {
    *dst = *src;
}

static void VuQuatBlend(NUQUAT *out, NUQUAT *a, NUQUAT *b, f32 t, f32 w) {
    out->x = a->x * t + b->x * w;
    out->y = a->y * t + b->y * w;
    out->z = a->z * t + b->z * w;
    out->w = a->w * t + b->w * w;
}

static f32 VuQuatDot(NUQUAT *a, NUQUAT *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

static void VuQuatLerp(NUQUAT *out, NUQUAT *a, NUQUAT *b, f32 t) {
    f32 w = 1.0f - t;
    out->x = a->x * w + b->x * t;
    out->y = a->y * w + b->y * t;
    out->z = a->z * w + b->z * t;
    out->w = a->w * w + b->w * t;
}

static void VuQuatNeg2(NUQUAT *out, NUQUAT *in) {
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
    out->w = -in->w;
}

static void VuQuatNormalise(NUQUAT *out, NUQUAT *in) {
    f32 magnitude = in->w * in->w + in->x * in->x + in->y * in->y + in->z * in->z;
    f32 scale = NuFdiv(1.0f, NuFsqrt(magnitude));
    out->w = in->w * scale;
    out->x = in->x * scale;
    out->y = in->y * scale;
    out->z = in->z * scale;
}

static void VuVecMtxMul(NUVEC *out, NUVEC *v, NUMTX *m) {
    (void)out;
    (void)v;
    (void)m;
}

static void VuVecSet(f32 *out, f32 x, f32 y, f32 z, f32 w) {
    (void)out;
    (void)x;
    (void)y;
    (void)z;
    (void)w;
}

static void VuMtxTranspose(NUMTX *dst, NUMTX *src) {
    NuMtxTranspose(dst, src);
}

/* The original calls the VU helpers in this math unit, rather than the
 * similarly named NuQuat helpers used by the
 * former game-utility copy. */
void VuQuatSlerpFast(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t) {
    f32 dot;
    f32 threshold = 0.85f;
    dot = VuQuatDot(from, to);
    NUQUAT target;
    if (dot < 0.0f) {
        dot = -dot;
        VuQuatNeg2(&target, to);
    } else {
        VuQuatCopy(&target, to);
    }

    if (dot > threshold) {
        VuQuatLerp(out, from, &target, t);
        VuQuatNormalise(out, out);
        return;
    }

    dot = dot < 0.995f ? dot : 0.995f;

    const f32 estimate = dot * 0.22340366f * dot + 2.2184808f;
    const f32 linear = dot * 2.4418843f;
    const f32 angle = NuFsqrt(estimate - linear) - NuFsqrt(estimate + linear) + 1.5707964f + dot * 0.63912874f;
    const f32 inverse_sine = 1.0f / NuFsqrt(1.0f - dot * dot);

    f32 from_angle = (1.0f - t) * angle - 1.5707964f;
    from_angle *= from_angle;
    f32 to_angle = t * angle - 1.5707964f;
    to_angle *= to_angle;

    const f32 from_poly =
        ((((from_angle * 2.3154014e-5f - 0.0013853709f) * from_angle + 0.041663583f) * from_angle - 0.49999905f) *
             from_angle +
         0.99999994f);
    const f32 to_poly =
        ((((to_angle * 2.3154014e-5f - 0.0013853709f) * to_angle + 0.041663583f) * to_angle - 0.49999905f) * to_angle +
         0.99999994f);
    const f32 from_weight = from_poly * inverse_sine;
    const f32 to_weight = to_poly * inverse_sine;
    VuQuatBlend(out, from, &target, from_weight, to_weight);
}
