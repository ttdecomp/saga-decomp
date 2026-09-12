#pragma once

#include "nu2api/numath/nuang.h"
#include "nu2api/numath/nuvec.h"

struct nuquat_s;

typedef struct numtx_s {
    f32 m00;
    f32 m01;
    f32 m02;
    f32 m03;
    f32 m10;
    f32 m11;
    f32 m12;
    f32 m13;
    f32 m20;
    f32 m21;
    f32 m22;
    f32 m23;
    f32 m30;
    f32 m31;
    f32 m32;
    f32 m33;
} NUMTX;

extern NUMTX numtx_zero;
extern NUMTX numtx_identity;

#define NUMTX_GET_ROW_VEC(mtx, row) ((NUVEC *)&((mtx)[0].m##row##0))

#ifdef __cplusplus
extern "C" {
#endif
    void NuMtxSetRotationXYVU0(NUMTX *matrix, NUANGVEC *angles);
    /// @brief Initializes a matrix to the empty matrix
    /// @details Initializes the matrix m to the empty matrix.
    /// @param m The matrix to initialize to the empty matrix
    /// @return void
    void NuMtxSetZero(NUMTX *m);

    /// @brief Initializes a matrix to the identity matrix
    /// @details Initializes the matrix m to the identity matrix.
    /// @param m The matrix to initialize to the identity matrix
    /// @return void
    void NuMtxSetIdentity(NUMTX *m);

    /// @brief Initializes a matrix to a translation matrix
    /// @details Initializes the matrix m to a translation matrix with the translation vector t.
    /// @param m The matrix to initialize to a translation matrix
    /// @param t The translation vector to initialize the matrix to
    /// @return void
    void NuMtxSetTranslation(NUMTX *m, NUVEC *t);

    /// @brief Initializes a matrix to a translation matrix with the negative translation vector
    /// @details Initializes the matrix m to a translation matrix with the negative translation vector t.
    /// @param m The matrix to initialize to a translation matrix with the negative translation vector
    /// @param t The translation vector to initialize the matrix to with the negative translation vector
    /// @return void
    void NuMtxSetTranslationNeg(NUMTX *m, NUVEC *t);

    /// @brief Initializes a matrix to a scale matrix
    /// @details Initializes the matrix m to a scale matrix with the scale vector s.
    /// @param m The matrix to initialize to a scale matrix
    /// @param s The scale vector to initialize the matrix to
    /// @return void
    void NuMtxSetScale(NUMTX *m, NUVEC *s);

    /// @brief Initializes a matrix to a rotation matrix around the x-axis
    /// @details Initializes the matrix m to a rotation matrix around the x-axis with the angle a.
    /// @param m The matrix to initialize to a rotation matrix around the x-axis
    /// @param a The angle to initialize the matrix to with the rotation around the x-axis
    /// @return void
    void NuMtxSetRotationX(NUMTX *m, NUANG a);

    /// @brief Initializes a matrix to a rotation matrix around the y-axis
    /// @details Initializes the matrix m to a rotation matrix around the y-axis with the angle a.
    /// @param m The matrix to initialize to a rotation matrix around the y-axis
    /// @param a The angle to initialize the matrix to with the rotation around the y-axis
    /// @return void
    void NuMtxSetRotationY(NUMTX *m, NUANG a);

    /// @brief Initializes a matrix to a rotation matrix around the z-axis
    /// @details Initializes the matrix m to a rotation matrix around the z-axis with the angle a.
    /// @param m The matrix to initialize to a rotation matrix around the z-axis
    /// @param a The angle to initialize the matrix to with the rotation around the z-axis
    /// @return void
    void NuMtxSetRotationZ(NUMTX *m, NUANG a);

    /// @brief Initializes a matrix to a rotation matrix around an axis
    /// @details Initializes the matrix m to a rotation matrix around the axis ax with the angle a.
    /// @param m The matrix to initialize to a rotation matrix around the axis
    /// @param ang The angle to initialize the matrix to with the rotation around the axis
    /// @param ax The axis to initialize the matrix to with the rotation around the axis
    /// @return void
    void NuMtxSetRotationAxis(NUMTX *m, NUANG ang, NUVEC *ax);

    /// @brief Translates a matrix by a translation vector
    /// @details Translates the matrix m by the translation vector t.
    /// @param m The matrix to translate
    /// @param t The translation vector to translate the matrix by
    /// @return void
    void NuMtxTranslate(NUMTX *m, NUVEC *t);

    /// @brief Translates a matrix opposite to the direction of the translation vector
    /// @details Translates the matrix m opposite to the direction of the translation vector t.
    /// @param m The matrix to translate
    /// @param t The translation vector to translate the matrix by opposite to the direction of the translation vector
    /// @return void
    void NuMtxTranslateNeg(NUMTX *m, NUVEC *t);

    void NuMtxPreTranslate(NUMTX *m, NUVEC *t);
    void NuMtxPreTranslateX(NUMTX *m, f32 tx);
    void NuMtxPreTranslateNeg(NUMTX *m, NUVEC *t);
    void NuMtxScale(NUMTX *m, NUVEC *s);
    void NuMtxScaleU(NUMTX *m, f32 s);
    void NuMtxPreScaleU(NUMTX *m, f32 s);
    NUVEC NuMtxGetScale(NUMTX *m);
    void NuMtxPreScale(NUMTX *m, NUVEC *s);
    void NuMtxPreScaleVU0(NUMTX *m, NUVEC *s);
    void NuMtxPreScaleX(NUMTX *m, f32 ScaleX);
    void NuMtxRotateX(NUMTX *m, NUANG a);
    void NuMtxPreRotateX(NUMTX *m, NUANG a);
    void NuMtxRotateY(NUMTX *m, NUANG a);
    void NuMtxPreRotateY(NUMTX *m, NUANG a);
    void NuMtxRotateZ(NUMTX *m, NUANG a);
    void NuMtxPreRotateZ(NUMTX *m, NUANG a);
    void NuMtxPreRotateY180(NUMTX *m);
    void NuMtxPreRotateY180X(NUMTX *m, NUANG a);
    void NuMtxPreSkewYX(NUMTX *Mtx, f32 SkewVal);
    void NuMtxTransposeR(NUMTX *m, NUMTX *m0);
    void NuMtxTranspose(NUMTX *m, NUMTX *m0);
    void NuMtxInv(NUMTX *m, NUMTX *m0);
    void NuMtxInvVU0(NUMTX *out, NUMTX *in);
    void NuMtxMulArrayVU0(NUMTX *out, NUMTX *left, NUMTX *right, i32 count);
    void NuMtxMulnVU0(NUMTX *out, NUMTX *left, NUMTX **right);
    void NuMtxPreScaleUVU0(NUMTX *matrix, f32 scale);
    void NuMtxInvR(NUMTX *m, NUMTX *m0);
    f32 NuMtxDet3(NUMTX *m);
    void NuMtxLookAtX(NUMTX *m, NUVEC *pnt);
    void NuMtxLookAtY(NUMTX *m, NUVEC *pnt);
    void NuMtxLookAtZ(NUMTX *m, NUVEC *pnt);
    void NuMtxInvLookAtX(NUMTX *m, NUVEC *pnt);
    void NuMtxInvLookAtY(NUMTX *m, NUVEC *pnt);
    void NuMtxInvLookAtZ(NUMTX *m, NUVEC *pnt);
    void NuMtxAddR(NUMTX *m, NUMTX *m0, NUMTX *m1);
    void NuMtxSubR(NUMTX *m, NUMTX *m0, NUMTX *m1);
    void NuMtxSkewSymmetric(NUMTX *m, NUVEC *v);
    void NuMtxGetXAxis(NUMTX *m, NUVEC *x);
    void NuMtxGetYAxis(NUMTX *m, NUVEC *y);
    void NuMtxGetZAxis(NUMTX *m, NUVEC *z);
    void NuMtxGetTranslation(NUMTX *m, NUVEC *t);
    i32 NuMtxCompare(NUMTX *a, NUMTX *b);
    void NuMtxTruncate24Bit(NUMTX *trunc, NUMTX *mtx);
    void NuMtxRotateAng(NUANG ang, f32 x, f32 z, f32 *rx, f32 *rz);
    void NuMtxGetEulerXYZ(NUMTX *Mat, NUANG *x, NUANG *y, NUANG *z);
    void NuMtxLookAtD3D(NUMTX *mtx, NUVEC *eye, NUVEC *center, NUVEC *up);
    void NuMtxSetPerspectiveD3D(NUMTX *mtx, f32 fovy, f32 aspect, f32 zNear, f32 zFar);
    void NuMtxSetPerspectiveBlend(NUMTX *mtx, f32 fovy, f32 aspect, f32 zNear, f32 zFar);
    void NuMtxSetFrustumD3D(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
    void NuMtxSetFrustumBlend(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
    void NuMtxSetOrthoD3D(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
    void NuMtxSetOrthoBlend(NUMTX *mtx, f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
    void NuMtxGetPerspectiveD3D(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar);
    void NuMtxGetPerspectiveBlend(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar);
    void NuMtxGetFrustumD3D(NUMTX *mtx, f32 *l, f32 *r, f32 *b, f32 *t, f32 *n, f32 *f);
    void NuMtxGetFrustumBlend(NUMTX *mtx, f32 *l, f32 *r, f32 *b, f32 *t, f32 *n, f32 *f);
    void NuMtxSetRotateXYZ(NUMTX *m, NUANGVEC *a);
    void NuMtxSetRotateXYZVU0(NUMTX *m, NUANGVEC *a);
    void NuMtxMul(NUMTX *m, NUMTX *m0, NUMTX *m1);
    void NuMtxMulH(NUMTX *m, NUMTX *m0, NUMTX *m1);
    void NuMtxMulR(NUMTX *m, NUMTX *m0, NUMTX *m1);
    void NuMtxMulRVU0(NUMTX *result, NUMTX *left, NUMTX *right);
    void NuMtxMulVU0(NUMTX *result, NUMTX *left, NUMTX *right);
    void NuMtxScaleVU0(NUMTX *matrix, NUVEC *scale);
    void NuMtxInvRSS(NUMTX *inv, NUMTX *T);
    void NuMtxInvRSSH(NUMTX *inv, NUMTX *T);
    void NuMtxInvH(NUMTX *mi, NUMTX *m0);
    void NuMtxAlignX(NUMTX *m, NUVEC *v);
    void NuMtxAlignY(NUMTX *m, NUVEC *v);
    void NuMtxAlignZ(NUMTX *m, NUVEC *v);
    void NuMtxOrth(NUMTX *m);
    void NuMtxVecToEulerXYZ(NUVEC *XVec, NUVEC *ZVec, NUANG *x, NUANG *y, NUANG *z);
    f32 NuMtxSSE(NUMTX *a, NUMTX *b);

    void NuMtx24BitCorrection(NUMTX *correction, NUMTX *matrix);
    void NuMtxCalcFaceOn(NUMTX *m, NUVEC *v);
    void NuMtxCalcCheapFaceOn(NUMTX *m, NUVEC *v);
    void NuMtxCalcDebrisFaceOn(NUMTX *m);
    void NuMtxCalcFaceY(NUMTX *m, NUVEC *v);
    void NuMtxGetPerspectivePS3(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar);
    void NuMtxLookAtInverseD3D(NUMTX *mtx, NUVEC *eye, NUVEC *center, NUVEC *up);
    void NuMtxToQuat(NUMTX *m, struct nuquat_s *out);
#ifdef __cplusplus
}

void NuMtxCalcCheapFaceY(NUMTX *m, NUVEC *v);
void NuMtxCalcCheapFaceY_v2(NUMTX *m, NUVEC *v);
void NuMtxCalcCheapFaceOnDebug(NUMTX *m, NUVEC *v);
void NuMtxGetPerspectiveOGL(NUMTX *mtx, f32 *fovy, f32 *aspect, f32 *zNear, f32 *zFar);

#endif
