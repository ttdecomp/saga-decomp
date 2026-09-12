#include "nu2api/numath/nuquat.h"

#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

void NuQuatFromEulerXYZ(NUQUAT *out, NUANG psi, NUANG theta, NUANG phi) {
    f32 sin_psi_over_2;
    f32 cos_psi_over_2;
    f32 sin_theta_over_2;
    f32 cos_theta_over_2;
    f32 sin_phi_over_2;
    f32 cos_phi_over_2;
    f32 sin_phi_times_sin_theta;
    f32 cos_phi_times_cos_theta;
    f32 sin_phi_times_cos_theta;
    f32 cos_phi_times_sin_theta;

    psi /= 2;
    theta /= 2;
    phi /= 2;

    cos_psi_over_2 = NU_COS_LUT(psi);
    sin_psi_over_2 = NU_SIN_LUT(psi);

    sin_theta_over_2 = NU_SIN_LUT(theta);
    cos_theta_over_2 = NU_COS_LUT(theta);

    cos_phi_over_2 = NU_COS_LUT(phi);
    sin_phi_over_2 = NU_SIN_LUT(phi);

    cos_phi_times_cos_theta = cos_phi_over_2 * cos_theta_over_2;
    sin_phi_times_sin_theta = sin_phi_over_2 * sin_theta_over_2;

    out->w = cos_phi_times_cos_theta * cos_psi_over_2 + sin_phi_times_sin_theta * sin_psi_over_2;
    out->x = cos_phi_times_cos_theta * sin_psi_over_2 - sin_phi_times_sin_theta * cos_psi_over_2;

    cos_phi_times_sin_theta = cos_phi_over_2 * sin_theta_over_2;
    sin_phi_times_cos_theta = sin_phi_over_2 * cos_theta_over_2;

    out->y = cos_phi_times_sin_theta * cos_psi_over_2 + sin_phi_times_cos_theta * sin_psi_over_2;
    out->z = sin_phi_times_cos_theta * cos_psi_over_2 - cos_phi_times_sin_theta * sin_psi_over_2;
}

void NuQuatToMtx(NUQUAT *quat, NUMTX *out) {
    f32 w_sq;
    f32 x_sq;
    f32 y_sq;
    f32 z_sq;
    f32 xy_times_2;
    f32 xz_times_2;
    f32 xw_times_2;
    f32 yz_times_2;
    f32 yw_times_2;
    f32 zw_times_2;

    w_sq = quat->w * quat->w;
    x_sq = quat->x * quat->x;
    y_sq = quat->y * quat->y;
    z_sq = quat->z * quat->z;

    xy_times_2 = (quat->x * quat->y) + (quat->x * quat->y);
    xz_times_2 = (quat->x * quat->z) + (quat->x * quat->z);
    xw_times_2 = (quat->x * quat->w) + (quat->x * quat->w);
    yz_times_2 = (quat->y * quat->z) + (quat->y * quat->z);
    yw_times_2 = (quat->y * quat->w) + (quat->y * quat->w);
    zw_times_2 = (quat->z * quat->w) + (quat->z * quat->w);

    out->m00 = w_sq + x_sq - y_sq - z_sq;
    out->m10 = xy_times_2 - zw_times_2;
    out->m20 = xz_times_2 + yw_times_2;
    out->m30 = 0.0f;

    out->m01 = xy_times_2 + zw_times_2;
    out->m11 = w_sq - x_sq + y_sq - z_sq;
    out->m21 = yz_times_2 - xw_times_2;
    out->m31 = 0.0f;

    out->m02 = xz_times_2 - yw_times_2;
    out->m12 = yz_times_2 + xw_times_2;
    out->m22 = w_sq - x_sq - y_sq + z_sq;
    out->m32 = 0.0f;

    out->m03 = 0.0f;
    out->m13 = 0.0f;
    out->m23 = 0.0f;
    out->m33 = 1.0f;
}

void NuQuatInv(NUQUAT *out, NUQUAT *quat) {
    f32 recip_sq;

    recip_sq = 1.0f / (quat->w * quat->w + quat->x * quat->x + quat->y * quat->y + quat->z * quat->z);

    out->w = quat->w * recip_sq;
    out->x = -quat->x * recip_sq;
    out->y = -quat->y * recip_sq;
    out->z = -quat->z * recip_sq;
}

void NuQuatLerp(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t) {
    out->x = (to->x - from->x) * t + from->x;
    out->y = (to->y - from->y) * t + from->y;
    out->z = (to->z - from->z) * t + from->z;
    out->w = (to->w - from->w) * t + from->w;
}

void NuQuatSlerp(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t) {
    f32 _unused;
    NUQUAT to_prime;
    f32 scale;
    f32 omega;
    f32 sin_omega;
    f32 from_factor;
    f32 to_factor;

    // Even though this same number is used later in the function, the
    // assignment here has nothing to do with it.
    _unused = 1e-05f;

    scale = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;

    if (scale < 0.0f) {
        scale = -scale;

        to_prime.x = -to->x;
        to_prime.y = -to->y;
        to_prime.z = -to->z;
        to_prime.w = -to->w;
    } else {
        to_prime.x = to->x;
        to_prime.y = to->y;
        to_prime.z = to->z;
        to_prime.w = to->w;
    }

    if (1.0f - scale > 1e-05f) {
        omega = NuACos(scale);

        sin_omega = NU_SIN_LUT((i32)omega);
        from_factor = NuFdiv(NU_SIN_LUT((i32)((1.0f - t) * omega)), sin_omega);
        to_factor = NuFdiv(NU_SIN_LUT((i32)(t * omega)), sin_omega);
    } else {
        from_factor = 1.0f - t;
        to_factor = t;
    }

    out->x = from->x * from_factor + to_prime.x * to_factor;
    out->y = from->y * from_factor + to_prime.y * to_factor;
    out->z = from->z * from_factor + to_prime.z * to_factor;
    out->w = from->w * from_factor + to_prime.w * to_factor;
}

void NuQuatCubicInt(NUQUAT *out, NUQUAT *m, NUQUAT *a, NUQUAT *b, NUQUAT *c, f32 t) {
    NuQuatHarmonize(m, a);
    NuQuatHarmonize(a, b);
    NuQuatHarmonize(b, c);

    out->x = NuCubicInterpolation(m->x, a->x, b->x, c->x, t);
    out->y = NuCubicInterpolation(m->y, a->y, b->y, c->y, t);
    out->z = NuCubicInterpolation(m->z, a->z, b->z, c->z, t);
    out->w = NuCubicInterpolation(m->w, a->w, b->w, c->w, t);
}

void NuQuatHermiteInt(NUQUAT *out, NUQUAT *m, NUQUAT *q0, NUQUAT *q1, NUQUAT *q2, f32 t) {
    NuQuatHarmonize(m, q0);
    NuQuatHarmonize(q0, q1);
    NuQuatHarmonize(q1, q2);

    out->x = NuHermiteInterpolation(m->x, q0->x, q1->x, q2->x, t);
    out->y = NuHermiteInterpolation(m->y, q0->y, q1->y, q2->y, t);
    out->z = NuHermiteInterpolation(m->z, q0->z, q1->z, q2->z, t);
    out->w = NuHermiteInterpolation(m->w, q0->w, q1->w, q2->w, t);
}

void NuQuatHarmonize(NUQUAT *a, NUQUAT *b) {
    f32 dot = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;

    if (dot < 0.0f) {
        b->x = -b->x;
        b->y = -b->y;
        b->z = -b->z;
        b->w = -b->w;
    }
}

f32 NuCubicInterpolation(f32 m, f32 y0, f32 y1, f32 y2, f32 t) {
    f32 a;
    f32 b;
    f32 c;
    f32 d;
    f32 t_cb;
    f32 t_sq;

    a = -m / 6.0f + y0 / 2.0f + -y1 / 2.0f + y2 / 6.0f;
    b = m / 2.0f - y0 + y1 / 2.0f;
    c = -m / 3.0f + -y0 / 2.0f + y1 + -y2 / 6.0f;
    d = y0;

    t_sq = t * t;
    t_cb = t * t_sq;

    return a * t_cb + b * t_sq + c * t + d;
}

f32 NuHermiteInterpolation(f32 m, f32 y0, f32 y1, f32 y2, f32 t) {
    f32 m0;
    f32 m1;
    f32 t_sq;
    f32 t_cb;
    f32 h0;
    f32 h1;
    f32 h2;
    f32 h3;

    m0 = (y1 - m) / 2.0f;
    m1 = (y2 - y0) / 2.0f;

    t_sq = t * t;
    t_cb = t * t_sq;

    h0 = (t_cb + t_cb - t_sq * 3.0f) + 1.0f;
    h1 = t_cb - (t_sq + t_sq) + t;
    h2 = t_cb * -2.0f + t_sq * 3.0f;
    h3 = t_cb - t_sq;

    return h0 * y0 + h1 * m0 + h2 * y1 + h3 * m1;
}

void NuQuatAdd(NUQUAT *out, NUQUAT *q0, NUQUAT *q1) {
    out->w = q0->w + q1->w;
    out->x = q0->x + q1->x;
    out->y = q0->y + q1->y;
    out->z = q0->z + q1->z;
}

void NuQuatSub(NUQUAT *out, NUQUAT *q0, NUQUAT *q1) {
    out->w = q0->w - q1->w;
    out->x = q0->x - q1->x;
    out->y = q0->y - q1->y;
    out->z = q0->z - q1->z;
}

void NuQuatMul(NUQUAT *out, NUQUAT *q0, NUQUAT *q1) {
    NUQUAT result;
    result.w = q1->w * q0->w - q1->x * q0->x - q1->y * q0->y - q1->z * q0->z;
    result.x = q1->w * q0->x + q1->x * q0->w + q1->y * q0->z - q1->z * q0->y;
    result.y = q1->w * q0->y + q1->y * q0->w + q1->z * q0->x - q1->x * q0->z;
    out->z = q1->w * q0->z + q1->z * q0->w + q1->x * q0->y - q1->y * q0->x;
    out->x = result.x;
    out->y = result.y;
    out->w = result.w;
}

f32 NuQuatMagnitude(NUQUAT *q) {
    return q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z;
}

void NuQuatNormalise(NUQUAT *out, NUQUAT *q) {
    f32 magnitude_squared = q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z;
    if (magnitude_squared > 0.0f) {
        f32 inverse_magnitude = NuFdiv(1.0f, NuFsqrt(magnitude_squared));
        out->w = q->w * inverse_magnitude;
        out->x = q->x * inverse_magnitude;
        out->y = q->y * inverse_magnitude;
        out->z = q->z * inverse_magnitude;
    } else {
        *out = *q;
    }
}

void NuQuatNeg2(NUQUAT *out, NUQUAT *in) {
    out->x = -in->x;
    out->y = -in->y;
    out->z = -in->z;
    out->w = -in->w;
}

void NuQuatBlend(NUQUAT *out, NUQUAT *q0, NUQUAT *q1, f32 blendA, f32 blendB) {
    out->x = q0->x * blendA + q1->x * blendB;
    out->y = q0->y * blendA + q1->y * blendB;
    out->z = q0->z * blendA + q1->z * blendB;
    out->w = q0->w * blendA + q1->w * blendB;
}

void NuQuatLerp2(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t) {
    f32 dot = to->x * from->x + to->y * from->y + to->z * from->z + to->w * from->w;
    if (dot < 0.0f) {
        out->x = (to->x + from->x) * t - from->x;
        out->y = (to->y + from->y) * t - from->y;
        out->z = (to->z + from->z) * t - from->z;
        out->w = (to->w + from->w) * t - from->w;
    } else {
        out->x = (to->x - from->x) * t + from->x;
        out->y = (to->y - from->y) * t + from->y;
        out->z = (to->z - from->z) * t + from->z;
        out->w = (to->w - from->w) * t + from->w;
    }
}

f32 NuQuatDot(NUQUAT *q0, NUQUAT *q1) {
    return q0->x * q1->x + q0->y * q1->y + q0->z * q1->z + q0->w * q1->w;
}

void NuQuatSlerpFast(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t) {
    f32 threshold = 0.85f;
    f32 dot = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;
    NUQUAT negative;
    NUQUAT *target;
    if (dot < 0.0f) {
        dot = -dot;
        target = &negative;
        target->x = -to->x;
        target->y = -to->y;
        target->z = -to->z;
        target->w = -to->w;
    } else {
        target = to;
    }
    if (dot > threshold) {
        f32 inverse_t = 1.0f - t;
        out->x = from->x * inverse_t + target->x * t;
        out->y = from->y * inverse_t + target->y * t;
        out->z = from->z * inverse_t + target->z * t;
        out->w = from->w * inverse_t + target->w * t;
        NuQuatNormalise(out, out);
    } else {
        dot = dot < 0.995f ? dot : 0.995f;
        f32 a = 2.2184808254241943f + (dot * 0.223403662443161f) * dot;
        f32 b = dot * 2.4418842792510986f;
        f32 angle = 1.5707963705062866f + (NuFsqrt(a - b) - NuFsqrt(a + b)) + dot * 0.6391287446022034f;
        f32 inverse_sine = 1.0f / NuFsqrt(1.0f - dot * dot);
        f32 u = (1.0f - t) * angle - 1.5707963705062866f;
        u = u * u;
        f32 v = t * angle - 1.5707963705062866f;
        v = v * v;
        f32 sin_u = 0.9999999403953552f +
                    (((u * 2.3154014343163e-05f - 0.0013853708514943719f) * u + 0.04166358336806297f) * u -
                     0.4999990463256836f) *
                        u;
        f32 sin_v = 0.9999999403953552f +
                    (((v * 2.3154014343163e-05f - 0.0013853708514943719f) * v + 0.04166358336806297f) * v -
                     0.4999990463256836f) *
                        v;
        f32 from_factor = sin_u * inverse_sine;
        f32 to_factor = sin_v * inverse_sine;
        out->x = from->x * from_factor + target->x * to_factor;
        out->y = from->y * from_factor + target->y * to_factor;
        out->z = from->z * from_factor + target->z * to_factor;
        out->w = from->w * from_factor + target->w * to_factor;
    }
}

void NuQuatSlerp_Accurate(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t) {
    f32 scale;
    f32 from_factor;
    f32 to_factor;
    f32 unused = 0.0f;
    f32 omega;
    f32 sin_omega;
    NUQUAT to_prime;
    scale = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;
    if (scale < 0.0f) {
        scale = -scale;
        to_prime.x = -to->x;
        to_prime.y = -to->y;
        to_prime.z = -to->z;
        to_prime.w = -to->w;
    } else {
        to_prime.x = to->x;
        to_prime.y = to->y;
        to_prime.z = to->z;
        to_prime.w = to->w;
    }
    if (1.0f - scale > 0.0f) {
        omega = 1.5707963705062866f - NuASin_Accurate(scale);
        sin_omega = NuSin_Accurate(omega);
        from_factor = NuSin_Accurate((1.0f - t) * omega) / sin_omega;
        to_factor = NuSin_Accurate(t * omega) / sin_omega;
    } else {
        from_factor = 1.0f - t;
        to_factor = t;
    }
    out->x = from->x * from_factor + to_prime.x * to_factor;
    out->y = from->y * from_factor + to_prime.y * to_factor;
    out->z = from->z * from_factor + to_prime.z * to_factor;
    out->w = from->w * from_factor + to_prime.w * to_factor;
}

struct nuqtentry_s {
    i16 count;
    i16 field_02;
    u8 *data;
    u32 field_08;
};

struct nuqthdr_s {
    u32 field_00[5];
    nuqtentry_s *entries;
    i32 entry_count;
    u32 field_1c;
    u8 *data;
};

static void NuQTFixAddress(nuqthdr_s *header) {
    uintptr_t base = (uintptr_t)header;
    header->entries = (nuqtentry_s *)((u8 *)header->entries + base);
    header->data = header->data + base;
    for (i32 index = 0; index < header->entry_count; ++index) {
        if (header->entries[index].count > 0)
            header->entries[index].data = header->entries[index].data + base;
    }
}

static void NuQTUnfixAddress(nuqthdr_s *header) {
    uintptr_t base = -(uintptr_t)header;
    for (i32 index = 0; index < header->entry_count; ++index) {
        if (header->entries[index].count > 0)
            header->entries[index].data = header->entries[index].data + base;
    }
    header->entries = (nuqtentry_s *)((u8 *)header->entries + base);
    header->data = header->data + base;
}
