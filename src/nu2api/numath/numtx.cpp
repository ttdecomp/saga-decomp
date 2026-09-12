#include <limits.h>

#include "nu2api/numath/numtx.h"

#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nu3d/nucamera.h"

NUMTX numtx_zero = {0};

// clang-format off
NUMTX numtx_identity = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};
// clang-format on

void NuMtxSetZero(NUMTX *m) {
    *m = numtx_zero;
}

void NuMtxSetIdentity(NUMTX *m) {
    *m = numtx_identity;
}

void NuMtxSetTranslation(NUMTX *m, NUVEC *t) {
    m->m30 = t->x;
    m->m31 = t->y;
    m->m32 = t->z;
    m->m01 = m->m02 = m->m03 = m->m10 = m->m12 = m->m13 = m->m20 = m->m21 = m->m23 = 0.0f;
    m->m00 = m->m11 = m->m22 = m->m33 = 1.0f;
}

void NuMtxSetTranslationNeg(NUMTX *m, NUVEC *t) {
    m->m30 = -t->x;
    m->m31 = -t->y;
    m->m32 = -t->z;
    m->m01 = m->m02 = m->m03 = m->m10 = m->m12 = m->m13 = m->m20 = m->m21 = m->m23 = 0.0f;
    m->m00 = m->m11 = m->m22 = m->m33 = 1.0f;
}

void NuMtxSetScale(NUMTX *m, NUVEC *s) {
    m->m00 = s->x;
    m->m11 = s->y;
    m->m22 = s->z;
    m->m01 = m->m02 = m->m03 = m->m10 = m->m12 = m->m13 = m->m20 = m->m21 = m->m23 = m->m30 = m->m31 = m->m32 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxSetRotationX(NUMTX *m, NUANG a) {
    m->m11 = m->m22 = NU_COS_LUT(a);
    m->m12 = NU_SIN_LUT(a);
    m->m21 = -m->m12;
    m->m00 = 1.0f;
    m->m01 = m->m02 = m->m03 = m->m23 = m->m10 = m->m20 = m->m13 = m->m30 = m->m31 = m->m32 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxSetRotationY(NUMTX *m, NUANG a) {
    m->m00 = m->m22 = NU_COS_LUT(a);
    m->m20 = NU_SIN_LUT(a);
    m->m02 = -m->m20;
    m->m11 = 1.0f;
    m->m01 = m->m10 = m->m03 = m->m23 = m->m12 = m->m21 = m->m13 = m->m30 = m->m31 = m->m32 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxSetRotationZ(NUMTX *m, NUANG a) {
    m->m00 = m->m11 = NU_COS_LUT(a);
    m->m01 = NU_SIN_LUT(a);
    m->m10 = -m->m01;
    m->m22 = 1.0;
    m->m02 = m->m12 = m->m03 = m->m23 = m->m20 = m->m21 = m->m13 = m->m30 = m->m31 = m->m32 = 0.0f;
    m->m33 = 1.0;
}

void NuMtxSetRotationAxis(NUMTX *m, NUANG ang, NUVEC *ax) {
    NUANG angle = ang >> 1; // get half angle
    f32 s = NU_SIN_LUT(angle);
    f32 t = NU_COS_LUT(angle);

    f32 a = ax->x * s;
    f32 _2a = a + a;
    f32 b = ax->y * s;
    f32 _2b = b + b;
    f32 c = ax->z * s;
    f32 _2c = c + c;

    f32 _2sa = t * _2a;
    f32 _2aa = a * _2a;
    f32 _2sb = t * _2b;
    f32 _2ab = a * _2b;
    f32 _2bb = b * _2b;
    f32 _2sc = t * _2c;
    f32 _2ac = a * _2c;
    f32 _2bc = b * _2c;
    f32 _2cc = c * _2c;

    m->m00 = 1.0f - (_2bb + _2cc);
    m->m11 = 1.0f - (_2aa + _2cc);
    m->m22 = 1.0f - (_2aa + _2bb);
    m->m33 = 1.0f;
    m->m01 = _2ab + _2sc;
    m->m02 = _2ac - _2sb;
    m->m03 = 0.0f;
    m->m10 = _2ab - _2sc;
    m->m12 = _2bc + _2sa;
    m->m13 = 0.0f;
    m->m20 = _2ac + _2sb;
    m->m21 = _2bc - _2sa;
    m->m23 = 0.0f;
    m->m30 = m->m31 = m->m32 = 0.0f;
}

void NuMtxTranslate(NUMTX *m, NUVEC *t) {
    m->m30 += t->x;
    m->m31 += t->y;
    m->m32 += t->z;
}

void NuMtxTranslateNeg(NUMTX *m, NUVEC *t) {
    m->m30 -= t->x;
    m->m31 -= t->y;
    m->m32 -= t->z;
}

void NuMtxPreTranslate(NUMTX *m, NUVEC *t) {
    m->m30 += t->x * m->m00 + t->y * m->m10 + t->z * m->m20;
    m->m31 += t->x * m->m01 + t->y * m->m11 + t->z * m->m21;
    m->m32 += t->x * m->m02 + t->y * m->m12 + t->z * m->m22;
}

void NuMtxPreTranslateX(NUMTX *m, f32 tx) {
    m->m30 = m->m30 + m->m00 * tx;
    m->m31 = m->m31 + m->m01 * tx;
    m->m32 = m->m32 + m->m02 * tx;
}

void NuMtxPreTranslateNeg(NUMTX *m, NUVEC *t) {
    m->m30 -= t->x * m->m00 + t->y * m->m10 + t->z * m->m20;
    m->m31 -= t->x * m->m01 + t->y * m->m11 + t->z * m->m21;
    m->m32 -= t->x * m->m02 + t->y * m->m12 + t->z * m->m22;
}

void NuMtxScale(NUMTX *m, NUVEC *s) {
    m->m00 *= s->x;
    m->m01 *= s->y;
    m->m02 *= s->z;
    m->m10 *= s->x;
    m->m11 *= s->y;
    m->m12 *= s->z;
    m->m20 *= s->x;
    m->m21 *= s->y;
    m->m22 *= s->z;
    m->m30 *= s->x;
    m->m31 *= s->y;
    m->m32 *= s->z;
}

void NuMtxScaleU(NUMTX *m, f32 s) {
    m->m00 *= s;
    m->m01 *= s;
    m->m02 *= s;
    m->m10 *= s;
    m->m11 *= s;
    m->m12 *= s;
    m->m20 *= s;
    m->m21 *= s;
    m->m22 *= s;
    m->m30 *= s;
    m->m31 *= s;
    m->m32 *= s;
}

void NuMtxPreScaleU(NUMTX *m, f32 s) {
    m->m00 *= s;
    m->m01 *= s;
    m->m02 *= s;
    m->m10 *= s;
    m->m11 *= s;
    m->m12 *= s;
    m->m20 *= s;
    m->m21 *= s;
    m->m22 *= s;
}

NUVEC NuMtxGetScale(NUMTX *m) {
    NUVEC scale;

    scale.x = NuFsqrt(m->m00 * m->m00 + m->m01 * m->m01 + m->m02 * m->m02);
    scale.y = NuFsqrt(m->m10 * m->m10 + m->m11 * m->m11 + m->m12 * m->m12);
    scale.z = NuFsqrt(m->m20 * m->m20 + m->m21 * m->m21 + m->m22 * m->m22);

    return scale;
}

void NuMtxPreScale(NUMTX *m, NUVEC *s) {
    m->m00 *= s->x;
    m->m01 *= s->x;
    m->m02 *= s->x;
    m->m10 *= s->y;
    m->m11 *= s->y;
    m->m12 *= s->y;
    m->m20 *= s->z;
    m->m21 *= s->z;
    m->m22 *= s->z;
}

void NuMtxPreScaleX(NUMTX *m, f32 ScaleX) {
    m->m00 = m->m00 * ScaleX;
    m->m01 = m->m01 * ScaleX;
    m->m02 = m->m02 * ScaleX;
}

void NuMtxRotateX(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m01 = m->m01;
    f32 m11 = m->m11;
    f32 m21 = m->m21;
    f32 m31 = m->m31;

    m->m01 = m01 * cosx - m->m02 * sinx;
    m->m02 = m01 * sinx + m->m02 * cosx;
    m->m11 = m11 * cosx - m->m12 * sinx;
    m->m12 = m11 * sinx + m->m12 * cosx;
    m->m21 = m21 * cosx - m->m22 * sinx;
    m->m22 = m21 * sinx + m->m22 * cosx;
    m->m31 = m31 * cosx - m->m32 * sinx;
    m->m32 = m31 * sinx + m->m32 * cosx;
}

void NuMtxPreRotateX(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m10 = m->m10;
    f32 m11 = m->m11;
    f32 m12 = m->m12;

    m->m10 = cosx * m10 + m->m20 * sinx;
    m->m11 = cosx * m11 + m->m21 * sinx;
    m->m12 = cosx * m12 + m->m22 * sinx;
    m->m20 = m->m20 * cosx - sinx * m10;
    m->m21 = m->m21 * cosx - sinx * m11;
    m->m22 = m->m22 * cosx - sinx * m12;
}

void NuMtxRotateY(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m10 = m->m10;
    f32 m20 = m->m20;
    f32 m30 = m->m30;

    m->m00 = m00 * cosx + m->m02 * sinx;
    m->m02 = m->m02 * cosx - m00 * sinx;
    m->m10 = m10 * cosx + m->m12 * sinx;
    m->m12 = m->m12 * cosx - m10 * sinx;
    m->m20 = m20 * cosx + m->m22 * sinx;
    m->m22 = m->m22 * cosx - m20 * sinx;
    m->m30 = m30 * cosx + m->m32 * sinx;
    m->m32 = m->m32 * cosx - m30 * sinx;
}

void NuMtxPreRotateY(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m01 = m->m01;
    f32 m02 = m->m02;

    m->m00 = cosx * m00 - m->m20 * sinx;
    m->m01 = cosx * m01 - m->m21 * sinx;
    m->m02 = cosx * m02 - m->m22 * sinx;
    m->m20 = sinx * m00 + m->m20 * cosx;
    m->m21 = sinx * m01 + m->m21 * cosx;
    m->m22 = sinx * m02 + m->m22 * cosx;
}

void NuMtxRotateZ(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m10 = m->m10;
    f32 m20 = m->m20;
    f32 m30 = m->m30;

    m->m00 = m00 * cosx - m->m01 * sinx;
    m->m01 = m00 * sinx + m->m01 * cosx;
    m->m10 = m10 * cosx - m->m11 * sinx;
    m->m11 = m10 * sinx + m->m11 * cosx;
    m->m20 = m20 * cosx - m->m21 * sinx;
    m->m21 = m20 * sinx + m->m21 * cosx;
    m->m30 = m30 * cosx - m->m31 * sinx;
    m->m31 = m30 * sinx + m->m31 * cosx;
}

void NuMtxPreRotateZ(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m01 = m->m01;
    f32 m02 = m->m02;

    m->m00 = cosx * m00 + m->m10 * sinx;
    m->m01 = cosx * m01 + m->m11 * sinx;
    m->m02 = cosx * m02 + m->m12 * sinx;
    m->m10 = m->m10 * cosx - sinx * m00;
    m->m11 = m->m11 * cosx - sinx * m01;
    m->m12 = m->m12 * cosx - sinx * m02;
}

void NuMtxPreRotateY180(NUMTX *m) {
    m->m00 = -m->m00;
    m->m01 = -m->m01;
    m->m02 = -m->m02;
    m->m20 = -m->m20;
    m->m21 = -m->m21;
    m->m22 = -m->m22;
}

void NuMtxPreRotateY180X(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m10 = m->m10;
    f32 m11 = m->m11;
    f32 m12 = m->m12;

    m->m00 = -m->m00;
    m->m01 = -m->m01;
    m->m02 = -m->m02;
    m->m10 = cosx * m10 - m->m20 * sinx;
    m->m11 = cosx * m11 - m->m21 * sinx;
    m->m12 = cosx * m12 - m->m22 * sinx;
    m->m20 = -(m->m20 * cosx) - sinx * m10;
    m->m21 = -(m->m21 * cosx) - sinx * m11;
    m->m22 = -(m->m22 * cosx) - sinx * m12;
}

void NuMtxPreSkewYX(NUMTX *Mtx, f32 SkewVal) {
    Mtx->m00 = Mtx->m00 + Mtx->m10 * SkewVal;
    Mtx->m01 = Mtx->m01 + Mtx->m11 * SkewVal;
    Mtx->m02 = Mtx->m02 + Mtx->m12 * SkewVal;
}

void NuMtxTransposeR(NUMTX *m, NUMTX *m0) {
    f32 t;

    t = m0->m01;
    m->m01 = m0->m10;
    m->m10 = t;
    t = m0->m02;
    m->m02 = m0->m20;
    m->m20 = t;
    t = m0->m12;
    m->m12 = m0->m21;
    m->m21 = t;
    m->m00 = m0->m00;
    m->m11 = m0->m11;
    m->m22 = m0->m22;
    m->m30 = m0->m30;
    m->m31 = m0->m31;
    m->m32 = m0->m32;
    m->m33 = m0->m33;
}

void NuMtxTranspose(NUMTX *m, NUMTX *m0) {
    f32 t;

    t = m0->m01;
    m->m01 = m0->m10;
    m->m10 = t;
    t = m0->m02;
    m->m02 = m0->m20;
    m->m20 = t;
    t = m0->m03;
    m->m03 = m0->m30;
    m->m30 = t;
    t = m0->m12;
    m->m12 = m0->m21;
    m->m21 = t;
    t = m0->m13;
    m->m13 = m0->m31;
    m->m31 = t;
    t = m0->m23;
    m->m23 = m0->m32;
    m->m32 = t;
    m->m00 = m0->m00;
    m->m11 = m0->m11;
    m->m22 = m0->m22;
    m->m33 = m0->m33;
}

void NuMtxInv(NUMTX *m, NUMTX *m0) {
    f32 t;

    f32 tx = -m0->m30;
    f32 ty = -m0->m31;
    f32 tz = -m0->m32;

    t = m0->m01;
    m->m01 = m0->m10;
    m->m10 = t;
    t = m0->m02;
    m->m02 = m0->m20;
    m->m20 = t;
    t = m0->m12;
    m->m12 = m0->m21;
    m->m21 = t;
    m->m00 = m0->m00;
    m->m11 = m0->m11;
    m->m22 = m0->m22;
    m->m30 = m->m00 * tx + m->m10 * ty + m->m20 * tz;
    m->m31 = m->m01 * tx + m->m11 * ty + m->m21 * tz;
    m->m32 = m->m02 * tx + m->m12 * ty + m->m22 * tz;
    m->m23 = 0.0f;
    m->m13 = m->m23;
    m->m03 = m->m13;
    m->m33 = 1.0f;
}

void NuMtxInvR(NUMTX *m, NUMTX *m0) {
    f32 t;

    t = m0->m01;
    m->m01 = m0->m10;
    m->m10 = t;
    t = m0->m02;
    m->m02 = m0->m20;
    m->m20 = t;
    t = m0->m12;
    m->m12 = m0->m21;
    m->m21 = t;
    m->m00 = m0->m00;
    m->m11 = m0->m11;
    m->m22 = m0->m22;
    m->m23 = 0.0f;
    m->m13 = m->m23;
    m->m03 = m->m13;
    m->m32 = 0.0f;
    m->m31 = m->m32;
    m->m30 = m->m31;
    m->m33 = 1.0f;
}

f32 NuMtxDet3(NUMTX *m) {
    return m->m00 * (m->m11 * m->m22 - m->m12 * m->m21) - m->m01 * (m->m10 * m->m22 - m->m12 * m->m20) +
           m->m02 * (m->m10 * m->m21 - m->m11 * m->m20);
}

void NuMtxLookAtX(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = pnt->x - m->m30;
    v.y = pnt->y - m->m31;
    v.z = pnt->z - m->m32;

    NuVecNorm(&v, &v);
    NuMtxAlignX(m, &v);
}

void NuMtxLookAtY(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = pnt->x - m->m30;
    v.y = pnt->y - m->m31;
    v.z = pnt->z - m->m32;

    NuVecNorm(&v, &v);
    NuMtxAlignY(m, &v);
}

void NuMtxLookAtZ(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = pnt->x - m->m30;
    v.y = pnt->y - m->m31;
    v.z = pnt->z - m->m32;

    NuVecNorm(&v, &v);
    NuMtxAlignZ(m, &v);
}

void NuMtxInvLookAtX(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = m->m30 - pnt->x;
    v.y = m->m31 - pnt->y;
    v.z = m->m32 - pnt->z;

    NuVecNorm(&v, &v);
    NuMtxAlignX(m, &v);
}

void NuMtxInvLookAtY(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = m->m30 - pnt->x;
    v.y = m->m31 - pnt->y;
    v.z = m->m32 - pnt->z;

    NuVecNorm(&v, &v);
    NuMtxAlignY(m, &v);
}

void NuMtxInvLookAtZ(NUMTX *m, NUVEC *pnt) {
    NUVEC v;

    v.x = m->m30 - pnt->x;
    v.y = m->m31 - pnt->y;
    v.z = m->m32 - pnt->z;

    NuVecNorm(&v, &v);
    NuMtxAlignZ(m, &v);
}

void NuMtxAddR(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    m->m00 = m0->m00 + m1->m00;
    m->m01 = m0->m01 + m1->m01;
    m->m02 = m0->m02 + m1->m02;
    m->m03 = 0.0f;
    m->m10 = m0->m10 + m1->m10;
    m->m11 = m0->m11 + m1->m11;
    m->m12 = m0->m12 + m1->m12;
    m->m13 = 0.0f;
    m->m20 = m0->m20 + m1->m20;
    m->m21 = m0->m21 + m1->m21;
    m->m22 = m0->m22 + m1->m22;
    m->m23 = 0.0f;
    m->m30 = 0.0f;
    m->m31 = 0.0f;
    m->m32 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxSubR(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    m->m00 = m0->m00 - m1->m00;
    m->m01 = m0->m01 - m1->m01;
    m->m02 = m0->m02 - m1->m02;
    m->m03 = 0.0f;
    m->m10 = m0->m10 - m1->m10;
    m->m11 = m0->m11 - m1->m11;
    m->m12 = m0->m12 - m1->m12;
    m->m13 = 0.0f;
    m->m20 = m0->m20 - m1->m20;
    m->m21 = m0->m21 - m1->m21;
    m->m22 = m0->m22 - m1->m22;
    m->m23 = 0.0f;
    m->m30 = 0.0f;
    m->m31 = 0.0f;
    m->m32 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxSkewSymmetric(NUMTX *m, NUVEC *v) {
    m->m00 = 0.0;
    m->m01 = -v->z;
    m->m02 = v->y;
    m->m03 = 0.0;
    m->m10 = v->z;
    m->m11 = 0.0;
    m->m12 = -v->x;
    m->m13 = 0.0;
    m->m20 = -v->y;
    m->m21 = v->x;
    m->m22 = 0.0;
    m->m23 = 0.0;
    m->m30 = 0.0;
    m->m31 = 0.0;
    m->m32 = 0.0;
    m->m33 = 1.0;
}

void NuMtxGetXAxis(NUMTX *m, NUVEC *x) {
    x->x = m->m00;
    x->y = m->m01;
    x->z = m->m02;
}

void NuMtxGetYAxis(NUMTX *m, NUVEC *y) {
    y->x = m->m10;
    y->y = m->m11;
    y->z = m->m12;
}

void NuMtxGetZAxis(NUMTX *m, NUVEC *z) {
    z->x = m->m20;
    z->y = m->m21;
    z->z = m->m22;
}

void NuMtxGetTranslation(NUMTX *m, NUVEC *t) {
    t->x = m->m30;
    t->y = m->m31;
    t->z = m->m32;
}

i32 NuMtxCompare(NUMTX *a, NUMTX *b) {
    f32 *aa = &a->m00;
    f32 *bb = &b->m00;

    for (i32 i = 0; i < 16; i++) {
        if (*aa < *bb) {
            return -1;
        }
        if (*aa > *bb) {
            return 1;
        }
        aa += 1;
        bb += 1;
    }

    return 0;
}

void NuMtxTruncate24Bit(NUMTX *trunc, NUMTX *mtx) {
    // was this really worth it?
    // struct padding and uints bigger than 4 bytes be damned
    u32 *trunc_int = (u32 *)trunc;
    u32 *mtx_int = (u32 *)mtx;

    trunc_int[0] = mtx_int[0] & 0xffffff00;
    trunc_int[1] = mtx_int[1] & 0xffffff00;
    trunc_int[2] = mtx_int[2] & 0xffffff00;
    trunc_int[3] = mtx_int[3] & 0xffffff00;

    trunc_int[4] = mtx_int[4] & 0xffffff00;
    trunc_int[5] = mtx_int[5] & 0xffffff00;
    trunc_int[6] = mtx_int[6] & 0xffffff00;
    trunc_int[7] = mtx_int[7] & 0xffffff00;

    trunc_int[8] = mtx_int[8] & 0xffffff00;
    trunc_int[9] = mtx_int[9] & 0xffffff00;
    trunc_int[10] = mtx_int[10] & 0xffffff00;
    trunc_int[11] = mtx_int[11] & 0xffffff00;

    trunc_int[12] = mtx_int[12] & 0xffffff00;
    trunc_int[13] = mtx_int[13] & 0xffffff00;
    trunc_int[14] = mtx_int[14] & 0xffffff00;
    trunc_int[15] = mtx_int[15] & 0xffffff00;
}

void NuMtxRotateAng(NUANG ang, f32 x, f32 z, f32 *rx, f32 *rz) {
    f32 sinx = NU_SIN_LUT(ang);
    f32 cosx = NU_COS_LUT(ang);

    *rx = x * cosx - z * sinx;
    *rz = x * sinx + z * cosx;
}

void NuMtxGetEulerXYZ(NUMTX *Mat, NUANG *x, NUANG *y, NUANG *z) {
    NUVEC XVec;
    NUVEC ZVec;

    NuMtxGetXAxis(Mat, &XVec);
    NuMtxGetZAxis(Mat, &ZVec);
    NuMtxVecToEulerXYZ(&XVec, &ZVec, x, y, z);
}

void NuMtxLookAtD3D(NUMTX *mtx, NUVEC *eye, NUVEC *center, NUVEC *up) {
    NUVEC ax;
    NUVEC ay;
    NUVEC az;

    NuVecSub(&az, center, eye);
    NuVecNorm(&az, &az);
    NuVecCross(&ax, up, &az);
    NuVecNorm(&ax, &ax);
    NuVecCross(&ay, &az, &ax);
    NuVecNorm(&ay, &ay);

    mtx->m00 = ax.x;
    mtx->m01 = ay.x;
    mtx->m02 = az.x;
    mtx->m03 = 0.0f;
    mtx->m10 = ax.y;
    mtx->m11 = ay.y;
    mtx->m12 = az.y;
    mtx->m13 = 0.0f;
    mtx->m20 = ax.z;
    mtx->m21 = ay.z;
    mtx->m22 = az.z;
    mtx->m23 = 0.0f;
    mtx->m30 = (-eye->x * ax.x - eye->y * ax.y) - eye->z * ax.z;
    mtx->m31 = (-eye->x * ay.x - eye->y * ay.y) - eye->z * ay.z;
    mtx->m32 = (-eye->x * az.x - eye->y * az.y) - eye->z * az.z;
    mtx->m33 = 1.0f;
}

void NuMtxSetPerspectiveD3D(NUMTX *mtx, f32 fovy, f32 aspect, f32 zNear, f32 zFar) {
    f32 tanFovy2 = NU_SIN_LUT((i32)((fovy / 2.0f) * (USHRT_MAX / 360.0f))) /
                   NU_COS_LUT((i32)((fovy / 2.0f) * (USHRT_MAX / 360.0f))); // USHRT_MAX / 360.0f is indices per degree
    mtx->m00 = 1.0 / (aspect * tanFovy2);
    mtx->m01 = 0.0;
    mtx->m02 = 0.0;
    mtx->m03 = 0.0;
    mtx->m10 = 0.0;
    mtx->m11 = 1.0 / tanFovy2;
    mtx->m12 = 0.0;
    mtx->m13 = 0.0;
    mtx->m20 = 0.0;
    mtx->m21 = 0.0;
    mtx->m22 = zFar / (zFar - zNear);
    mtx->m23 = 1.0;
    mtx->m30 = 0.0;
    mtx->m31 = 0.0;
    mtx->m32 = (-zFar * zNear) / (zFar - zNear);
    mtx->m33 = 0.0;
}

void NuMtxSetPerspectiveBlend(NUMTX *mtx, f32 fovy, f32 aspect, f32 zNear, f32 zFar) {
    f32 tanFovy2 = NU_SIN_LUT((i32)((fovy / 2.0f) * (USHRT_MAX / 360.0f))) /
                   NU_COS_LUT((i32)((fovy / 2.0f) * (USHRT_MAX / 360.0f))); // USHRT_MAX / 360.0f is indices per degree
    mtx->m00 = 1.0f / (aspect * tanFovy2);
    mtx->m01 = 0.0f;
    mtx->m02 = 0.0f;
    mtx->m03 = 0.0f;
    mtx->m10 = 0.0f;
    mtx->m11 = 1.0f / tanFovy2;
    mtx->m12 = 0.0f;
    mtx->m13 = 0.0f;
    mtx->m20 = 0.0f;
    mtx->m21 = 0.0f;
    mtx->m22 = (zFar + zNear) / (zFar - zNear);
    mtx->m23 = 1.0f;
    mtx->m30 = 0.0f;
    mtx->m31 = 0.0f;
    mtx->m32 = (zFar * -2.0f * zNear) / (zFar - zNear);
    mtx->m33 = 0.0f;
}

void NuMtxSetFrustumD3D(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mtx->m00 = (n + n) / (r - l);
    mtx->m01 = 0.0f;
    mtx->m02 = 0.0f;
    mtx->m03 = 0.0f;
    mtx->m10 = 0.0f;
    mtx->m11 = (n + n) / (t - b);
    mtx->m12 = 0.0f;
    mtx->m13 = 0.0f;
    mtx->m20 = (r + l) / (l - r);
    mtx->m21 = (t + b) / (b - t);
    mtx->m22 = f / (f - n);
    mtx->m23 = 1.0f;
    mtx->m30 = 0.0f;
    mtx->m31 = 0.0f;
    mtx->m32 = (-f * n) / (f - n);
    mtx->m33 = 0.0f;
}

void NuMtxSetFrustumBlend(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mtx->m00 = (n + n) / (r - l);
    mtx->m01 = 0.0f;
    mtx->m02 = 0.0f;
    mtx->m03 = 0.0f;
    mtx->m10 = 0.0f;
    mtx->m11 = (n + n) / (t - b);
    mtx->m12 = 0.0f;
    mtx->m13 = 0.0f;
    mtx->m20 = (r + l) / (l - r);
    mtx->m21 = (t + b) / (b - t);
    mtx->m22 = (f + n) / (f - n);
    mtx->m23 = 1.0f;
    mtx->m30 = 0.0f;
    mtx->m31 = 0.0f;
    mtx->m32 = (f * -2.0f * n) / (f - n);
    mtx->m33 = 0.0f;
}

void NuMtxSetOrthoD3D(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mtx->m00 = 2.0f / (r - l);
    mtx->m01 = 0.0f;
    mtx->m02 = 0.0f;
    mtx->m03 = 0.0f;
    mtx->m10 = 0.0f;
    mtx->m11 = 2.0f / (t - b);
    mtx->m12 = 0.0f;
    mtx->m13 = 0.0f;
    mtx->m20 = 0.0f;
    mtx->m21 = 0.0f;
    mtx->m22 = 1.0f / (f - n);
    mtx->m23 = 0.0f;
    mtx->m30 = (r + l) / (l - r);
    mtx->m31 = (t + b) / (b - t);
    mtx->m32 = (f + n) / (n - f);
    mtx->m33 = 1.0f;
}

void NuMtxSetOrthoBlend(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    mtx->m00 = 2.0f / (r - l);
    mtx->m01 = 0.0f;
    mtx->m02 = 0.0f;
    mtx->m03 = 0.0f;
    mtx->m10 = 0.0f;
    mtx->m11 = 2.0f / (t - b);
    mtx->m12 = 0.0f;
    mtx->m13 = 0.0f;
    mtx->m20 = 0.0f;
    mtx->m21 = 0.0f;
    mtx->m22 = 2.0f / (f - n);
    mtx->m23 = 0.0f;
    mtx->m30 = (r + l) / (l - r);
    mtx->m31 = (t + b) / (b - t);
    mtx->m32 = (f + n) / (n - f);
    mtx->m33 = 1.0f;
}

void NuMtxGetPerspectiveD3D(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar) {
    f32 Q = mtx->m22;
    *zNear = -mtx->m32 / Q;
    *zFar = (*zNear * Q) / (Q - 1.0f);
    *aspect = mtx->m11 / mtx->m00;
    *fovy = NuAtan2(1.0f / mtx->m11, 1.0f) * 360.0f / (f32)M_PI;
}

void NuMtxGetPerspectiveBlend(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar) {
    f32 Q = (mtx->m22 + 1.0f) * 0.5f;
    *zNear = -(mtx->m32 * 0.5f) / Q;
    *zFar = (*zNear * Q) / (Q - 1.0f);
    *aspect = mtx->m11 / mtx->m00;
    *fovy = NuAtan2(1.0f / mtx->m11, 1.0f) * 360.0f / (f32)M_PI;
}

void NuMtxGetFrustumD3D(NUMTX *mtx, f32 *l, f32 *r, f32 *b, f32 *t, f32 *n, f32 *f) {
    f32 Q = mtx->m22;
    f32 zNear = -mtx->m32 / Q;

    if (f != NULL) {
        *f = (Q * zNear) / (Q - 1.0f);
    }
    if (l != NULL) {
        *l = ((-1.0f - mtx->m20) * zNear) / mtx->m00;
    }
    if (r != NULL) {
        *r = ((1.0f - mtx->m20) * zNear) / mtx->m00;
    }
    if (b != NULL) {
        *b = ((-1.0f - mtx->m21) * zNear) / mtx->m11;
    }
    if (t != NULL) {
        *t = ((1.0f - mtx->m21) * zNear) / mtx->m11;
    }
    if (n != NULL) {
        *n = zNear;
    }
}

void NuMtxGetFrustumBlend(NUMTX *mtx, f32 *l, f32 *r, f32 *b, f32 *t, f32 *n, f32 *f) {
    f32 Q = (mtx->m22 + 1.0f) * 0.5f;
    f32 zNear = -(mtx->m32 * 0.5f) / Q;

    if (f != NULL) {
        *f = (Q * zNear) / (Q - 1.0f);
    }
    if (l != NULL) {
        *l = ((0.5f - mtx->m20) * zNear) / mtx->m00;
    }
    if (r != NULL) {
        *r = ((-1.0f - mtx->m20) * zNear) / mtx->m00;
    }
    if (b != NULL) {
        *b = ((0.5f - mtx->m21) * zNear) / mtx->m11;
    }
    if (t != NULL) {
        *t = ((-1.0f - mtx->m21) * zNear) / mtx->m11;
    }
    if (n != NULL) {
        *n = zNear;
    }
}

void NuMtxSetRotateXYZ(NUMTX *m, NUANGVEC *a) {
    f32 cosx = NU_COS_LUT(a->x);
    f32 sinx = NU_SIN_LUT(a->x);
    f32 cosy = NU_COS_LUT(a->y);
    f32 siny = NU_SIN_LUT(a->y);
    f32 cosz = NU_COS_LUT(a->z);
    f32 sinz = NU_SIN_LUT(a->z);

    m->m00 = cosy * cosz;
    m->m01 = cosy * sinz;
    m->m02 = -siny;
    m->m03 = 0.0f;
    m->m10 = sinx * siny * cosz - cosx * sinz;
    m->m11 = sinx * siny * sinz + cosx * cosz;
    m->m12 = sinx * cosy;
    m->m13 = 0.0f;
    m->m20 = cosx * siny * cosz + sinx * sinz;
    m->m21 = cosx * siny * sinz - sinx * cosz;
    m->m22 = cosx * cosy;
    m->m23 = 0.0f;
    m->m30 = 0.0f;
    m->m31 = 0.0f;
    m->m32 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxMul(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NUMTX gm;

    if ((m == m0) || (m == m1)) {
        gm.m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20;
        gm.m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21;
        gm.m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22;
        gm.m03 = 0.0f;
        gm.m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20;
        gm.m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21;
        gm.m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22;
        gm.m13 = 0.0f;
        gm.m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20;
        gm.m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21;
        gm.m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22;
        gm.m23 = 0.0f;
        gm.m30 = m0->m30 * m1->m00 + m0->m31 * m1->m10 + m0->m32 * m1->m20 + m1->m30;
        gm.m31 = m0->m30 * m1->m01 + m0->m31 * m1->m11 + m0->m32 * m1->m21 + m1->m31;
        gm.m32 = m0->m30 * m1->m02 + m0->m31 * m1->m12 + m0->m32 * m1->m22 + m1->m32;
        gm.m33 = 1.0f;
        *m = gm;
    } else {
        m->m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20;
        m->m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21;
        m->m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22;
        m->m03 = 0.0f;
        m->m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20;
        m->m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21;
        m->m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22;
        m->m13 = 0.0f;
        m->m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20;
        m->m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21;
        m->m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22;
        m->m23 = 0.0f;
        m->m30 = m0->m30 * m1->m00 + m0->m31 * m1->m10 + m0->m32 * m1->m20 + m1->m30;
        m->m31 = m0->m30 * m1->m01 + m0->m31 * m1->m11 + m0->m32 * m1->m21 + m1->m31;
        m->m32 = m0->m30 * m1->m02 + m0->m31 * m1->m12 + m0->m32 * m1->m22 + m1->m32;
        m->m33 = 1.0f;
    }
}

void NuMtxMulH(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NUMTX gm;

    if ((m == m0) || (m == m1)) {
        gm.m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20 + m0->m03 * m1->m30;
        gm.m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21 + m0->m03 * m1->m31;
        gm.m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22 + m0->m03 * m1->m32;
        gm.m03 = m0->m00 * m1->m03 + m0->m01 * m1->m13 + m0->m02 * m1->m23 + m0->m03 * m1->m33;
        gm.m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20 + m0->m13 * m1->m30;
        gm.m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21 + m0->m13 * m1->m31;
        gm.m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22 + m0->m13 * m1->m32;
        gm.m13 = m0->m10 * m1->m03 + m0->m11 * m1->m13 + m0->m12 * m1->m23 + m0->m13 * m1->m33;
        gm.m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20 + m0->m23 * m1->m30;
        gm.m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21 + m0->m23 * m1->m31;
        gm.m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22 + m0->m23 * m1->m32;
        gm.m23 = m0->m20 * m1->m03 + m0->m21 * m1->m13 + m0->m22 * m1->m23 + m0->m23 * m1->m33;
        gm.m30 = m0->m30 * m1->m00 + m0->m31 * m1->m10 + m0->m32 * m1->m20 + m0->m33 * m1->m30;
        gm.m31 = m0->m30 * m1->m01 + m0->m31 * m1->m11 + m0->m32 * m1->m21 + m0->m33 * m1->m31;
        gm.m32 = m0->m30 * m1->m02 + m0->m31 * m1->m12 + m0->m32 * m1->m22 + m0->m33 * m1->m32;
        gm.m33 = m0->m30 * m1->m03 + m0->m31 * m1->m13 + m0->m32 * m1->m23 + m0->m33 * m1->m33;
        *m = gm;
    } else {
        m->m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20 + m0->m03 * m1->m30;
        m->m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21 + m0->m03 * m1->m31;
        m->m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22 + m0->m03 * m1->m32;
        m->m03 = m0->m00 * m1->m03 + m0->m01 * m1->m13 + m0->m02 * m1->m23 + m0->m03 * m1->m33;
        m->m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20 + m0->m13 * m1->m30;
        m->m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21 + m0->m13 * m1->m31;
        m->m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22 + m0->m13 * m1->m32;
        m->m13 = m0->m10 * m1->m03 + m0->m11 * m1->m13 + m0->m12 * m1->m23 + m0->m13 * m1->m33;
        m->m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20 + m0->m23 * m1->m30;
        m->m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21 + m0->m23 * m1->m31;
        m->m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22 + m0->m23 * m1->m32;
        m->m23 = m0->m20 * m1->m03 + m0->m21 * m1->m13 + m0->m22 * m1->m23 + m0->m23 * m1->m33;
        m->m30 = m0->m30 * m1->m00 + m0->m31 * m1->m10 + m0->m32 * m1->m20 + m0->m33 * m1->m30;
        m->m31 = m0->m30 * m1->m01 + m0->m31 * m1->m11 + m0->m32 * m1->m21 + m0->m33 * m1->m31;
        m->m32 = m0->m30 * m1->m02 + m0->m31 * m1->m12 + m0->m32 * m1->m22 + m0->m33 * m1->m32;
        m->m33 = m0->m30 * m1->m03 + m0->m31 * m1->m13 + m0->m32 * m1->m23 + m0->m33 * m1->m33;
    }
}

void NuMtxMulR(NUMTX *m, NUMTX *m0, NUMTX *m1) {
    NUMTX gm;

    if ((m == m0) || (m == m1)) {
        gm.m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20;
        gm.m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21;
        gm.m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22;
        gm.m03 = 0.0;
        gm.m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20;
        gm.m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21;
        gm.m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22;
        gm.m13 = 0.0;
        gm.m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20;
        gm.m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21;
        gm.m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22;
        gm.m23 = 0.0;
        gm.m30 = gm.m31 = gm.m32 = 0.0f;
        gm.m33 = 1.0;
        *m = gm;
    } else {
        m->m00 = m0->m00 * m1->m00 + m0->m01 * m1->m10 + m0->m02 * m1->m20;
        m->m01 = m0->m00 * m1->m01 + m0->m01 * m1->m11 + m0->m02 * m1->m21;
        m->m02 = m0->m00 * m1->m02 + m0->m01 * m1->m12 + m0->m02 * m1->m22;
        m->m03 = 0.0;
        m->m10 = m0->m10 * m1->m00 + m0->m11 * m1->m10 + m0->m12 * m1->m20;
        m->m11 = m0->m10 * m1->m01 + m0->m11 * m1->m11 + m0->m12 * m1->m21;
        m->m12 = m0->m10 * m1->m02 + m0->m11 * m1->m12 + m0->m12 * m1->m22;
        m->m13 = 0.0;
        m->m20 = m0->m20 * m1->m00 + m0->m21 * m1->m10 + m0->m22 * m1->m20;
        m->m21 = m0->m20 * m1->m01 + m0->m21 * m1->m11 + m0->m22 * m1->m21;
        m->m22 = m0->m20 * m1->m02 + m0->m21 * m1->m12 + m0->m22 * m1->m22;
        m->m23 = 0.0;
        m->m30 = m->m31 = m->m32 = 0.0f;
        m->m33 = 1.0;
    }
}

void NuMtxInvRSS(NUMTX *inv, NUMTX *T) {
    NUMTX gm;

    f32 det = T->m00 * (T->m11 * T->m22 - T->m12 * T->m21) - T->m01 * (T->m10 * T->m22 - T->m12 * T->m20) +
              T->m02 * (T->m10 * T->m21 - T->m11 * T->m20);
    f32 invdet = det == 0.0f ? 0.0f : 1.0f / det;

    gm.m00 = (T->m11 * T->m22 - T->m12 * T->m21) * invdet;
    gm.m10 = (T->m10 * T->m22 - T->m12 * T->m20) * -invdet;
    gm.m20 = (T->m10 * T->m21 - T->m11 * T->m20) * invdet;
    gm.m01 = (T->m01 * T->m22 - T->m02 * T->m21) * -invdet;
    gm.m11 = (T->m00 * T->m22 - T->m02 * T->m20) * invdet;
    gm.m21 = (T->m00 * T->m21 - T->m01 * T->m20) * -invdet;
    gm.m02 = (T->m01 * T->m12 - T->m02 * T->m11) * invdet;
    gm.m12 = (T->m00 * T->m12 - T->m02 * T->m10) * -invdet;
    gm.m22 = (T->m00 * T->m11 - T->m01 * T->m10) * invdet;
    gm.m03 = 0.0f;
    gm.m13 = 0.0f;
    gm.m23 = 0.0f;
    gm.m33 = 1.0f;
    gm.m30 = 0.0f;
    gm.m31 = 0.0f;
    gm.m32 = 0.0f;
    *inv = gm;
}

void NuMtxInvRSSH(NUMTX *inv, NUMTX *T) {
    NUMTX gm;

    f32 tx = -T->m30;
    f32 ty = -T->m31;
    f32 tz = -T->m32;
    f32 det = T->m00 * (T->m11 * T->m22 - T->m12 * T->m21) - T->m01 * (T->m10 * T->m22 - T->m12 * T->m20) +
              T->m02 * (T->m10 * T->m21 - T->m11 * T->m20);
    f32 invdet = 1.0f / det;

    gm.m00 = (T->m11 * T->m22 - T->m12 * T->m21) * invdet;
    gm.m10 = (T->m10 * T->m22 - T->m12 * T->m20) * -invdet;
    gm.m20 = (T->m10 * T->m21 - T->m11 * T->m20) * invdet;
    gm.m01 = (T->m01 * T->m22 - T->m02 * T->m21) * -invdet;
    gm.m11 = (T->m00 * T->m22 - T->m02 * T->m20) * invdet;
    gm.m21 = (T->m00 * T->m21 - T->m01 * T->m20) * -invdet;
    gm.m02 = (T->m01 * T->m12 - T->m02 * T->m11) * invdet;
    gm.m12 = (T->m00 * T->m12 - T->m02 * T->m10) * -invdet;
    gm.m22 = (T->m00 * T->m11 - T->m01 * T->m10) * invdet;
    gm.m30 = gm.m00 * tx + gm.m10 * ty + gm.m20 * tz;
    gm.m31 = gm.m01 * tx + gm.m11 * ty + gm.m21 * tz;
    gm.m32 = gm.m02 * tx + gm.m12 * ty + gm.m22 * tz;
    gm.m03 = 0.0f;
    gm.m13 = 0.0f;
    gm.m23 = 0.0f;
    gm.m33 = 1.0f;
    *inv = gm;
}

void NuMtxInvH(NUMTX *mi, NUMTX *m0) {
    // there's some weird stack alignment stuff going on here...
    // also not sure if this semantically matches the original implementation
    i32 p[4];
    i32 j;
    i32 k;
    i32 i;
    f32 a[4][4];
    i32 n;
    f32 local_1c;
    f32 localm20;
    f32 localm28;
    f32 localm30;

    n = 4;

    a[0][0] = m0->m00;
    a[0][1] = m0->m01;
    a[0][2] = m0->m02;
    a[0][3] = m0->m03;
    a[1][0] = m0->m10;
    a[1][1] = m0->m11;
    a[1][2] = m0->m12;
    a[1][3] = m0->m13;
    a[2][0] = m0->m20;
    a[2][1] = m0->m21;
    a[2][2] = m0->m22;
    a[2][3] = m0->m23;
    a[3][0] = m0->m30;
    a[3][1] = m0->m31;
    a[3][2] = m0->m32;
    a[3][3] = m0->m33;

    for (i = 0; i < n; i++) {
        localm20 = 0.0f;
        p[i] = 0;

        for (j = i; j < n; j++) {
            local_1c = 0.0f;

            for (k = i; k < n; k++) {
                local_1c += NuFabs(a[j][k]);
            }

            localm28 = NuFdiv(NuFabs(a[j][i]), local_1c);

            if (localm20 < localm28) {
                localm20 = localm28;
                p[i] = j;
            }
        }

        if (localm20 == 0.0f) {
            return;
        }

        if (p[i] != i) {
            for (k = 0; k < n; k++) {
                f32 f;

                f = a[i][k];
                a[i][k] = a[p[i]][k];
                a[p[i]][k] = f;
            }
        }

        localm30 = a[i][i];

        for (k = 0; k < n; k++) {
            if (k != i) {
                a[i][k] = -a[i][k] / localm30;
                for (j = 0; j < n; j++) {
                    if (j != i) {
                        a[j][k] += a[j][i] * a[i][k];
                    }
                }
            }
        }

        for (j = 0; j < n; j++) {
            a[j][i] = a[j][i] / localm30;
        }

        a[i][i] = 1.0f / localm30;
    }

    for (i = n - 1; i >= 0; i--) {
        if (p[i] != i) {
            for (j = 0; j < n; j++) {
                f32 f;

                f = a[j][i];
                a[j][i] = a[j][p[i]];
                a[j][p[i]] = f;
            }
        }
    }

    mi->m00 = a[0][0];
    mi->m01 = a[0][1];
    mi->m02 = a[0][2];
    mi->m03 = a[0][3];
    mi->m10 = a[1][0];
    mi->m11 = a[1][1];
    mi->m12 = a[1][2];
    mi->m13 = a[1][3];
    mi->m20 = a[2][0];
    mi->m21 = a[2][1];
    mi->m22 = a[2][2];
    mi->m23 = a[2][3];
    mi->m30 = a[3][0];
    mi->m31 = a[3][1];
    mi->m32 = a[3][2];
    mi->m33 = a[3][3];
}

void NuMtxAlignX(NUMTX *m, NUVEC *v) {
    m->m00 = v->x;
    m->m01 = v->y;
    m->m02 = v->z;
    m->m20 = m->m01 * m->m12 - m->m02 * m->m11;
    m->m21 = m->m02 * m->m10 - m->m00 * m->m12;
    m->m22 = m->m00 * m->m11 - m->m01 * m->m10;

    f32 s = NuFsqrt(m->m20 * m->m20 + m->m21 * m->m21 + m->m22 * m->m22);
    s = NuFdiv(1.0f, s);

    m->m20 = m->m20 * s;
    m->m21 = m->m21 * s;
    m->m22 = m->m22 * s;
    m->m10 = m->m21 * m->m02 - m->m22 * m->m01;
    m->m11 = m->m22 * m->m00 - m->m20 * m->m02;
    m->m12 = m->m20 * m->m01 - m->m21 * m->m00;
}

void NuMtxAlignY(NUMTX *m, NUVEC *v) {
    m->m10 = v->x;
    m->m11 = v->y;
    m->m12 = v->z;
    m->m00 = m->m11 * m->m22 - m->m12 * m->m21;
    m->m01 = m->m12 * m->m20 - m->m10 * m->m22;
    m->m02 = m->m10 * m->m21 - m->m11 * m->m20;

    f32 s = NuFsqrt(m->m00 * m->m00 + m->m01 * m->m01 + m->m02 * m->m02);
    s = NuFdiv(1.0f, s);

    m->m00 = m->m00 * s;
    m->m01 = m->m01 * s;
    m->m02 = m->m02 * s;
    m->m20 = m->m01 * m->m12 - m->m02 * m->m11;
    m->m21 = m->m02 * m->m10 - m->m00 * m->m12;
    m->m22 = m->m00 * m->m11 - m->m01 * m->m10;
}

void NuMtxAlignZ(NUMTX *m, NUVEC *v) {
    f32 xLenSq = m->m00 * m->m00 + m->m01 * m->m01 + m->m02 * m->m02;
    f32 yLenSq = m->m10 * m->m10 + m->m11 * m->m11 + m->m12 * m->m12;
    f32 zLenSq = m->m20 * m->m20 + m->m21 * m->m21 + m->m22 * m->m22;

    f32 len = NuFsqrt(NuFdiv(zLenSq, v->x * v->x + v->y * v->y + v->z * v->z));
    m->m20 = v->x * len;
    m->m21 = v->y * len;
    m->m22 = v->z * len;

    if (NuFabs(NuVecDot(NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2))) > 0.86602540378f /* cos(30 deg) */) {
        NuVecCross(NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2), NUMTX_GET_ROW_VEC(m, 0));
        len = NuFsqrt(NuFdiv(yLenSq, m->m10 * m->m10 + m->m11 * m->m11 + m->m12 * m->m12));
        m->m10 = m->m10 * len;
        m->m11 = m->m11 * len;
        m->m12 = m->m12 * len;
        NuVecCross(NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2));
        len = NuFsqrt(NuFdiv(xLenSq, m->m00 * m->m00 + m->m01 * m->m01 + m->m02 * m->m02));
        m->m00 = m->m00 * len;
        m->m01 = m->m01 * len;
        m->m02 = m->m02 * len;
    } else {
        NuVecCross(NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2));
        len = NuFsqrt(NuFdiv(xLenSq, m->m00 * m->m00 + m->m01 * m->m01 + m->m02 * m->m02));
        m->m00 = m->m00 * len;
        m->m01 = m->m01 * len;
        m->m02 = m->m02 * len;
        NuVecCross(NUMTX_GET_ROW_VEC(m, 1), NUMTX_GET_ROW_VEC(m, 2), NUMTX_GET_ROW_VEC(m, 0));
        len = NuFsqrt(NuFdiv(yLenSq, m->m10 * m->m10 + m->m11 * m->m11 + m->m12 * m->m12));
        m->m10 = m->m10 * len;
        m->m11 = m->m11 * len;
        m->m12 = m->m12 * len;
    }
}

void NuMtxOrth(NUMTX *m) {
    f32 magnitude;

    magnitude = 1.0f / NuFsqrt(m->m00 * m->m00 + m->m01 * m->m01 + m->m02 * m->m02);
    m->m00 = m->m00 * magnitude;
    m->m01 = m->m01 * magnitude;
    m->m02 = m->m02 * magnitude;

    magnitude = 1.0f / NuFsqrt(m->m10 * m->m10 + m->m11 * m->m11 + m->m12 * m->m12);
    m->m10 = m->m10 * magnitude;
    m->m11 = m->m11 * magnitude;
    m->m12 = m->m12 * magnitude;
    m->m20 = m->m01 * m->m12 - m->m02 * m->m11;
    m->m21 = m->m02 * m->m10 - m->m00 * m->m12;
    m->m22 = m->m00 * m->m11 - m->m01 * m->m10;
    m->m10 = m->m21 * m->m02 - m->m22 * m->m01;
    m->m11 = m->m22 * m->m00 - m->m20 * m->m02;
    m->m12 = m->m20 * m->m01 - m->m21 * m->m00;
}

void NuMtxVecToEulerXYZ(NUVEC *XVec, NUVEC *ZVec, NUANG *x, NUANG *y, NUANG *z) {
    NUVEC XVec2;
    NUVEC ZVec2;

    *z = NuAtan2D(XVec->y, XVec->x);
    NuMtxRotateAng(-*z, XVec->x, XVec->y, &XVec2.x, &XVec2.y);
    XVec2.z = XVec->z;
    NuMtxRotateAng(-*z, ZVec->x, ZVec->y, &ZVec2.x, &ZVec2.y);
    ZVec2.z = ZVec->z;
    *y = -NuAtan2D(XVec2.z, XVec2.x);
    NuMtxRotateAng(*y, ZVec2.x, ZVec2.z, &ZVec2.x, &ZVec2.z);
    *x = -NuAtan2D(ZVec2.y, ZVec2.z);
}

f32 NuMtxSSE(NUMTX *a, NUMTX *b) {
    f32 sse;

    sse = (a->m00 - b->m00) * (a->m00 - b->m00);
    sse += (a->m01 - b->m01) * (a->m01 - b->m01);
    sse += (a->m02 - b->m02) * (a->m02 - b->m02);
    sse += (a->m03 - b->m03) * (a->m03 - b->m03);
    sse += (a->m10 - b->m10) * (a->m10 - b->m10);
    sse += (a->m11 - b->m11) * (a->m11 - b->m11);
    sse += (a->m12 - b->m12) * (a->m12 - b->m12);
    sse += (a->m13 - b->m13) * (a->m13 - b->m13);
    sse += (a->m20 - b->m20) * (a->m20 - b->m20);
    sse += (a->m21 - b->m21) * (a->m21 - b->m21);
    sse += (a->m22 - b->m22) * (a->m22 - b->m22);
    sse += (a->m23 - b->m23) * (a->m23 - b->m23);
    sse += (a->m30 - b->m30) * (a->m30 - b->m30);
    sse += (a->m31 - b->m31) * (a->m31 - b->m31);
    sse += (a->m32 - b->m32) * (a->m32 - b->m32);
    sse += (a->m33 - b->m33) * (a->m33 - b->m33);

    return sse;
}

void NuMtx24BitCorrection(NUMTX *correction, NUMTX *matrix) {
    f32 best_error = 3.4028234663852886e+38f;
    i32 element;
    i32 pass;
    f32 step = 1.0f;
    NUMTX truncated;
    NUMTX product;
    NuMtxTruncate24Bit(&truncated, matrix);
    *correction = numtx_identity;
    for (pass = 0; pass < 100; ++pass) {
        for (element = 0; element < 16; ++element) {
            if (element % 4 == 3)
                continue;
            f32 previous;
            f32 error;
            f32 *value = reinterpret_cast<f32 *>(correction) + element;
            previous = *value;
            *value += step;
            reinterpret_cast<u8 *>(value)[0] = 0;
            NuMtxMulH(&product, correction, &truncated);
            error = NuMtxSSE(&product, matrix);
            if (error >= best_error)
                *value = previous;
            else
                best_error = error;
            previous = *value;
            *value -= step;
            reinterpret_cast<u8 *>(value)[0] = 0;
            NuMtxMulH(&product, correction, &truncated);
            error = NuMtxSSE(&product, matrix);
            if (error >= best_error)
                *value = previous;
            else
                best_error = error;
        }
        step *= 0.9f;
    }
}

void NuMtxCalcCheapFaceOnDebug(NUMTX *m, NUVEC *v) {
    NUMTX camera = {
        -0.9711849093437195f,  0.0f,
        -0.23832732439041138f, 0.0f,
        0.08866473287343979f,  0.9282209873199463f,
        -0.3613092005252838f,  0.0f,
        0.22122041881084442f,  -0.37202924489974976f,
        -0.9014742374420166f,  0.0f,
        -0.6754902601242065f,  1.3077397346496582f,
        -0.7606481909751892f,  1.0f,
    };
    m->m00 = -camera.m00;
    m->m10 = camera.m01;
    m->m20 = -camera.m02;
    m->m01 = -camera.m10;
    m->m11 = camera.m11;
    m->m21 = -camera.m12;
    m->m02 = -camera.m20;
    m->m12 = camera.m21;
    m->m22 = -camera.m22;
    m->m03 = m->m13 = m->m23 = 0.0f;
    m->m33 = 1.0f;
    m->m30 = v->x;
    m->m31 = v->y;
    m->m32 = v->z;
}

void NuMtxGetPerspectivePS3(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar) {
    f32 Q = mtx->m22;
    f32 B = mtx->m32;

    *zNear = -B / (Q + 1.0f);
    *zFar = (*zNear * B) / ((*zNear + *zNear) + B);
    *aspect = mtx->m11 / mtx->m00;
    *fovy = NuAtan2(1.0f / mtx->m11, 1.0f) * 360.0f / (f32)M_PI;
}

void NuMtxGetPerspectiveOGL(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar) {
    f32 depth = mtx->m22;
    *zNear = (depth + depth) / mtx->m32 - 1.0f;
    *zFar = (*zNear * depth) / (depth - 1.0f);
    *aspect = mtx->m11 / mtx->m00;
    *fovy = NuAtan2(1.0f / mtx->m11, 1.0f) * 360.0f / (f32)M_PI;
}

void NuMtxLookAtInverseD3D(NUMTX *mtx, NUVEC *eye, NUVEC *center, NUVEC *up) {
    NUVEC ax;
    NUVEC ay;
    NUVEC az;

    NuVecSub(&az, center, eye);
    NuVecNorm(&az, &az);
    NuVecCross(&ax, up, &az);
    NuVecNorm(&ax, &ax);
    NuVecCross(&ay, &az, &ax);
    NuVecNorm(&ay, &ay);

    mtx->m00 = ax.x;
    mtx->m01 = ax.y;
    mtx->m02 = ax.z;
    mtx->m03 = 0.0f;
    mtx->m10 = ay.x;
    mtx->m11 = ay.y;
    mtx->m12 = ay.z;
    mtx->m13 = 0.0f;
    mtx->m20 = az.x;
    mtx->m21 = az.y;
    mtx->m22 = az.z;
    mtx->m23 = 0.0f;
    mtx->m30 = eye->x;
    mtx->m31 = eye->y;
    mtx->m32 = eye->z;
    mtx->m33 = 1.0f;
}

void NuMtxToQuat(NUMTX *m, struct nuquat_s *out) {
    f32 s;
    i32 i;
    f32 trace;
    i32 j, k;
    i32 next[3] = {1, 2, 0};
    f32 q[4];
    trace = reinterpret_cast<f32 *>(m)[0] + reinterpret_cast<f32 *>(m)[5] + reinterpret_cast<f32 *>(m)[10];

    if (trace > 0.0f) {
        s = NuFsqrt(trace + 1.0f);
        out->w = s * 0.5f;
        s = 0.5f / s;
        out->x = (reinterpret_cast<f32 *>(m)[6] - reinterpret_cast<f32 *>(m)[9]) * s;
        out->y = (reinterpret_cast<f32 *>(m)[8] - reinterpret_cast<f32 *>(m)[2]) * s;
        out->z = (reinterpret_cast<f32 *>(m)[1] - reinterpret_cast<f32 *>(m)[4]) * s;
    } else {
        i = 0;
        if (reinterpret_cast<f32 *>(m)[5] > reinterpret_cast<f32 *>(m)[0])
            i = 1;
        if (reinterpret_cast<f32 *>(m)[10] > (&m->m00)[(i << 2) + i])
            i = 2;
        j = next[i];
        k = next[j];
        s = NuFsqrt(((&m->m00)[(i << 2) + i] - ((&m->m00)[(j << 2) + j] + (&m->m00)[(k << 2) + k])) + 1.0f);
        q[i] = s * 0.5f;
        if (s != 0.0f)
            s = 0.5f / s;
        q[3] = ((&m->m00)[(j << 2) + k] - (&m->m00)[(k << 2) + j]) * s;
        q[j] = ((&m->m00)[(i << 2) + j] + (&m->m00)[(j << 2) + i]) * s;
        q[k] = ((&m->m00)[(i << 2) + k] + (&m->m00)[(k << 2) + i]) * s;
        out->x = q[0];
        out->y = q[1];
        out->z = q[2];
        out->w = q[3];
    }
}

void NuMtxSetRotationXYVU0(NUMTX *matrix, NUANGVEC *angles) {
    f32 cx = NU_COS_LUT(angles->x);
    f32 sx = NU_SIN_LUT(angles->x);
    f32 cy = NU_COS_LUT(angles->y);
    f32 sy = NU_SIN_LUT(angles->y);
    f32 cz = NU_COS_LUT(0);
    f32 sz = NU_SIN_LUT(0);
    matrix->m00 = cy * cz;
    matrix->m01 = cy * sz;
    matrix->m02 = -sy;
    matrix->m03 = 0.0f;
    matrix->m10 = sx * sy * cz - cx * sz;
    matrix->m11 = sx * sy * sz + cx * cz;
    matrix->m12 = sx * cy;
    matrix->m13 = 0.0f;
    matrix->m20 = cx * sy * cz + sx * sz;
    matrix->m21 = cx * sy * sz - sx * cz;
    matrix->m22 = cx * cy;
    matrix->m23 = 0.0f;
    matrix->m30 = 0.0f;
    matrix->m31 = 0.0f;
    matrix->m32 = 0.0f;
    matrix->m33 = 1.0f;
}
