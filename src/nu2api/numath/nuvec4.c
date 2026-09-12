#include "nu2api/numath/nuvec4.h"

#include "nu2api/numath/numtx.h"

void NuVec4MtxTransform(NUVEC4 *v, NUVEC *v0, NUMTX *m0) {
    f32 y = v0->x * m0->m01 + v0->y * m0->m11 + v0->z * m0->m21 + m0->m31;
    f32 z = v0->x * m0->m02 + v0->y * m0->m12 + v0->z * m0->m22 + m0->m32;
    f32 w = v0->x * m0->m03 + v0->y * m0->m13 + v0->z * m0->m23 + m0->m33;
    v->x = v0->x * m0->m00 + v0->y * m0->m10 + v0->z * m0->m20 + m0->m30;
    v->y = y;
    v->z = z;
    v->w = w;
}

void NuVec4Add(NUVEC4 *v, NUVEC4 *v0, NUVEC4 *v1) {
    v->w = v0->w + v1->w;
    v->x = v0->x + v1->x;
    v->y = v0->y + v1->y;
    v->z = v0->z + v1->z;
}

void NuVec4Scale(NUVEC4 *v, NUVEC4 *v0, f32 k) {
    v->x = v0->x * k;
    v->y = v0->y * k;
    v->z = v0->z * k;
    v->w = v0->w * k;
}

f32 NuVec4Dot(NUVEC4 *v0, NUVEC4 *v1) {
    return v0->w * v1->w + v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void NuVec4MtxTransformH(NUVEC4 *v, NUVEC4 *v0, NUMTX *m0) {
    f32 x = v0->x * m0->m00 + v0->y * m0->m10 + v0->z * m0->m20 + v0->w * m0->m30;
    f32 y = v0->x * m0->m01 + v0->y * m0->m11 + v0->z * m0->m21 + v0->w * m0->m31;
    f32 z = v0->x * m0->m02 + v0->y * m0->m12 + v0->z * m0->m22 + v0->w * m0->m32;
    f32 w = v0->x * m0->m03 + v0->y * m0->m13 + v0->z * m0->m23 + v0->w * m0->m33;

    v->x = x / w;
    v->y = y / w;
    v->z = z / w;
    v->w = w;
}
