#pragma once

/// @file
/// @brief Functions and types related to vectors in 3-space.

#include "nu2api/nucore/common.h"
#include "nu2api/numath/nuang.h"

struct numtx_s;

#ifdef __cplusplus
struct nuvec_s;
i32 NuVecClipTestPointVU0(nuvec_s *point, numtx_s *matrix);
#endif

/// @brief A vector in 3-dimensional space.
typedef struct nuvec_s {
    /// @brief The x component of the vector.
    f32 x;

    /// @brief The y component of the vector.
    f32 y;

    /// @brief The z component of the vector.
    f32 z;
} NUVEC;

typedef NUVEC NUVEC_ALIGNED16 __attribute__((aligned(16)));

/// @relatesalso nuvec_s
/// @brief The vector `(0, 0, 0)`.
extern NUVEC v000;
extern NUVEC nuvec_zero;

/// @relatesalso nuvec_s
/// @brief The vector `(1, 0, 0)`.
extern NUVEC v100;
extern NUVEC nuvec_x;

/// @relatesalso nuvec_s
/// @brief The vector `(0, 1, 0)`.
extern NUVEC v010;

/// @relatesalso nuvec_s
/// @brief The vector `(0, 0, 1)`.
extern NUVEC v001;

/// @relatesalso nuvec_s
/// @brief The vector `(1, 1, 1)`.
extern NUVEC v111;

#ifdef __cplusplus
extern "C" {
#endif
    /// @relatesalso nuvec_s
    /// @brief Negates a vector.
    /// @param[out] v The negated vector, i.e. `-v0`.
    /// @param v0 The vector to negate.
    void NuVecNeg(NUVEC *v, NUVEC *v0);
    NUVEC NuBezierCubicPatchEvaluate(NUVEC *control, f32 u, f32 v);
    NUVEC NuBezierCubicPatchEvaluatePartials(NUVEC *control, f32 u, f32 v, i32 order_u, i32 order_v);
    void NuCubicToBez3(f32 *cubic, f32 *bezier);
    void NuBez3Subdiv(f32 *control, f32 t, f32 *left, f32 *right);
    void NuBez3ToCubic(f32 *bezier, f32 *cubic);
    void NuBezierCubicPatchPartialsU(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsUU(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsUUV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsUVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    NUVEC NuBezierQuadraticTrianglePartialsUU(NUVEC *control);
    void NuBezierCubicPatchPartialsUUVVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsUUVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsUUU(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    void NuBezierCubicPatchPartialsVVV(NUVEC *control, NUVEC *out0, NUVEC *out1, NUVEC *out2, NUVEC *out3);
    NUVEC NuBezierQuadraticTriangleEvaluateBarycentric(NUVEC *control, f32 u, f32 v, f32 w);
    NUVEC NuBezierQuadraticTriangleEvaluateParametric(NUVEC *control, f32 u, f32 v);
    NUVEC NuBezierQuadraticTrianglePartialsVV(NUVEC *control);
    NUVEC NuBezierQuadraticTrianglePartialsUeV(NUVEC *control);

    /// @relatesalso nuvec_s
    /// @brief Adds two vectors.
    /// @param[out] v The vector resulting from adding `v0` and `v1`, i.e.
    ///               `v = v0 + v1`.
    /// @param v0 The first vector to add.
    /// @param v1 The second vector to add.
    void NuVecAdd(NUVEC *v, NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Subtracts two vectors.
    /// @param[out] v The vector resulting from subtracting `v1` from `v0`, i.e.
    ///               `v = v0 - v1`.
    /// @param v0 The vector from which to subtract (the _minuend_).
    /// @param v1 The vector to subtract (the _subtrahend_).
    void NuVecSub(NUVEC *v, NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Scales a vector by a scalar value.
    /// @param[out] v The scaled vector, i.e. `v = k * v0`.
    /// @param v0 The vector to scale.
    /// @param k The scalar value by which to scale `v0`.
    void NuVecScale(NUVEC *v, NUVEC *v0, f32 k);

    /// @relatesalso nuvec_s
    /// @brief Scales a vector and adds it to a second vector.
    /// @param[out] v The vector resulting from addition, i.e.
    //                `v = v0 + k * v1`.
    /// @param v0 The vector to add unmodified.
    /// @param v1 The vector to scale and add.
    /// @param k The scalar value by which to scale `v1`.
    void NuVecAddScale(NUVEC *v, NUVEC *v0, NUVEC *v1, f32 k);

    /// @relatesalso nuvec_s
    /// @brief Scales a vector and adds it to a second vector.
    /// @param[in,out] v The vector to add unmodified and the final result of
    ///                  addition, i.e. `v += k * v0`.
    /// @param v0 The vector to scale and add.
    /// @param k The scalar value by which to scale `v0`.
    void NuVecScaleAccum(NUVEC *v, NUVEC *v0, f32 k);

    /// @relatesalso nuvec_s
    /// @brief Scales a vector by the inverse of a scalar value.
    /// @param[out] v The scaled vector, i.e. `v = (1 / k) * v0`.
    /// @param v0 The vector to scale.
    /// @param k The scalar value, the inverse of which to scale `v0` by. May be
    ///          zero, in which case the vector is scaled by 0.
    void NuVecInvScale(NUVEC *v, NUVEC *v0, f32 k);

    /// @relatesalso nuvec_s
    /// @brief Computes the cross product of two vectors.
    /// @param[out] v The value of the cross product.
    /// @param v0 The first vector.
    /// @param v1 The second vector.
    void NuVecCross(NUVEC *v, NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Computes the cross product of two vectors defined relative to a
    ///        single point.
    /// @details Given three points `basepnt`, `v0`, and `v1`, computes the
    ///          cross product of `v0 - basepnt` and `v1 - basepnt`.
    /// @param[out] v The value of the cross product.
    /// @param basepnt The point to which `v0` and `v1` are relative.
    /// @param v0 The first relative point.
    /// @param v1 The second relative point.
    void NuVecCrossRel(NUVEC *v, NUVEC *basepnt, NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Computes the dot product of two vectors.
    /// @param v0 The first vector.
    /// @param v1 The second vector.
    /// @return The dot product of the two vectors.
    f32 NuVecDot(NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Constructs the component-wise maximum of two vectors.
    /// @details Each component of the resulting vector is the larger of the two
    ///          values of the input vector, e.g. `v.x = max(v0.x, v1.x)`.
    /// @param[out] v The constructed vector.
    /// @param v0 The first vector.
    /// @param v1 The second vector.
    /// @sa NuVecMin
    void NuVecMax(NUVEC *v, NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Constructs the component-wise minimum of two vectors.
    /// @details Each component of the resulting vector is the smaller of the
    ///          two values of the input vector, e.g. `v.x = min(v0.x, v1.x)`.
    /// @param[out] v The constructed vector.
    /// @param v0 The first vector.
    /// @param v1 The second vector.
    /// @sa NuVecMax
    void NuVecMin(NUVEC *v, NUVEC *v0, NUVEC *v1);

    /// @relatesalso nuvec_s
    /// @brief Computes the magnitude of a vector.
    /// @param v0 The vector.
    /// @return The magnitude of the vector.
    /// @sa NuVecMagSqr
    f32 NuVecMag(NUVEC *v0);

    /// @relatesalso nuvec_s
    /// @brief Computes the squared magnitude of a vector.
    /// @param v0 The vector.
    /// @return The magnitude of the vector, squared.
    /// @sa NuVecMag
    f32 NuVecMagSqr(NUVEC *v0);

    /// @relatesalso nuvec_s
    /// @brief Computes the magnitude of the vector formed by the `x` and `z`
    ///        components of a vector.
    /// @param v0 The vector.
    /// @return The magnitude of the 2-dimensional vector `(x, z)`.
    f32 NuVecMagXZ(NUVEC *v0);

    /// @relatesalso nuvec_s
    /// @brief Normalizes a vector.
    /// @param[out] v The normalized vector.
    /// @param v0 The vector to normalize.
    /// @return The magnitude of the vector prior to normalization.
    f32 NuVecNorm(NUVEC *v, NUVEC *v0);

    /// @relatesalso nuvec_s
    /// @brief Rotates a vector about the Y axis by a given angle.
    /// @param[out] v The rotated vector.
    /// @param v0 The vector to rotate.
    /// @param a The rotation angle in NUANG units.
    void NuVecRotateY(NUVEC *v, NUVEC *v0, NUANG a);
    void NuVecRotateX(NUVEC *v, NUVEC *v0, NUANG a);
    void NuVecRotateZ(NUVEC *v, NUVEC *v0, NUANG a);
    /// @brief Builds a horizontal vector of the given magnitude and Y rotation.
    void NuVecRotateYValZ(NUVEC *v, f32 magnitude, NUANG a);
    void NuVecRotateYValX(NUVEC *v, f32 magnitude, NUANG a);
    f32 NuVecDiffSqrVU0(NUVEC *a, NUVEC *b);
    f32 NuVecDiffVU0(NUVEC *a, NUVEC *b);
    f32 NuVecMagVU0(NUVEC *v);
    f32 NuVecNormVU0(NUVEC *out, NUVEC *v);
    void NuVecMtxRotateValX(NUVEC *out, f32 value, struct numtx_s *matrix);
    void NuVecMtxRotateValY(NUVEC *out, f32 value, struct numtx_s *matrix);
    void NuVecMtxRotateValZ(NUVEC *out, f32 value, struct numtx_s *matrix);
    void NuVecInvMtxRotateValX(NUVEC *out, f32 value, struct numtx_s *matrix);
    void NuVecInvMtxRotateValY(NUVEC *out, f32 value, struct numtx_s *matrix);
    void NuVecInvMtxRotateValZ(NUVEC *out, f32 value, struct numtx_s *matrix);
    void NuVecInvMtxScale(NUVEC *out, NUVEC *v, struct numtx_s *matrix);
    void NuVecInvMtxTranslate(NUVEC *out, NUVEC *v, struct numtx_s *matrix);
    void NuVecInvMtxTransformVU0(NUVEC *out, NUVEC *v, struct numtx_s *matrix);
    void NuVecMtxRotateH(NUVEC *out, NUVEC *v, struct numtx_s *matrix);

    /// @relatesalso nuvec_s
    /// @brief Computes the surface normal of a triangle.
    /// @param[out] v The normal vector.
    /// @param v0 The first vertex of the triangle.
    /// @param v1 The second vertex of the triangle.
    /// @param v2 The third vertex of the triangle.
    void NuVecSurfaceNormal(NUVEC *v, NUVEC *v0, NUVEC *v1, NUVEC *v2);

    /// @relatesalso nuvec_s
    /// @brief Computes the distance between two point vectors.
    /// @param v0 The first point.
    /// @param v1 The second point.
    /// @param[out] d The vector `v0 - v1`. May be null.
    /// @return The distance between the two points.
    /// @sa NuVecDistSqr
    f32 NuVecDist(NUVEC *v0, NUVEC *v1, NUVEC *d);

    /// @relatesalso nuvec_s
    /// @brief Computes the squared distance between two point vectors.
    /// @param v0 The first point.
    /// @param v1 The second point.
    /// @param[out] d The vector `v0 - v1`. May be null.
    /// @return The distance between the two points, squared.
    /// @sa NuVecDist
    f32 NuVecDistSqr(NUVEC *v0, NUVEC *v1, NUVEC *d);

    /// @relatesalso nuvec_s
    /// @brief Computes the distance between the vectors formed by the `x` and
    //         `z` components of two vectors.
    /// @param v0 The first point.
    /// @param v1 The second point.
    /// @param[out] d The vector `v0 - v1` with its `y` component set to zero.
    //                May be null.
    /// @return The distance between the two-dimensional vectors `(v0.x, v0.z)`
    ///         and `(v1.x, v1.z)`.
    /// @sa NuVecXZDistSqr
    f32 NuVecXZDist(NUVEC *v0, NUVEC *v1, NUVEC *d);

    /// @relatesalso nuvec_s
    /// @brief Computes the squared distance between the vectors formed by the
    ///        `x` and `z` components of two vectors.
    /// @param v0 The first point.
    /// @param v1 The second point.
    /// @param[out] d The vector `v0 - v1` with its `y` component set to zero.
    //                May be null.
    /// @return The distance between the two-dimensional vectors `(v0.x, v0.z)`
    ///         and `(v1.x, v1.z)`, squared.
    /// @sa NuVecXZDist
    f32 NuVecXZDistSqr(NUVEC *v0, NUVEC *v1, NUVEC *d);

    /// @relatesalso nuvec_s
    /// @brief Computes the linear interpolation of two points at a given ratio.
    /// @param[out] vt The vector to store the result in
    /// @param v0 The first point.
    /// @param v1 The second point.
    /// @param t The interpolation ratio.
    void NuVecLerp(NUVEC *vt, NUVEC *v1, NUVEC *v0, f32 t);

    /// @relatesalso nuvec_s
    /// @brief Compares two vectors for equivalence with a specified tolerance.
    /// @param a The first vector.
    /// @param b The second vector.
    /// @param tolerance The difference in values vector components may exhibit
    ///                  and still be considered equivalent.
    /// @return 1 if the vectors are equivalent, 0 otherwise.
    i32 NuVecCompareTolerance(NUVEC *a, NUVEC *b, f32 tolerance);

    /// @relatesalso nuvec_s
    /// @relatesalso numtx_s
    /// @brief Transforms a vector with the given transformation matrix.
    /// @param[out] out The transformed vector.
    /// @param v The vector to transform.
    /// @param m The transformation matrix.
    /// @sa NuVecMtxRotate, NuVecMtxScale, NuVecMtxTranslate,
    //      NuVecMtxTransformBlock
    void NuVecMtxTransform(NUVEC *out, NUVEC *v, struct numtx_s *m);
    void NuVecMtxTransformVU0(NUVEC *out, NUVEC *v, struct numtx_s *m);
    f32 NuVecMtxTransformH(NUVEC *out, NUVEC *v, struct numtx_s *m);
    void NuVecInvMtxRotate(NUVEC *out, NUVEC *v, struct numtx_s *m);
    void NuVecInvMtxTransform(NUVEC *out, NUVEC *v, struct numtx_s *m);

    /// @relatesalso nuvec_s
    /// @relatesalso numtx_s
    /// @brief Rotates a vector with the given transformation matrix.
    /// @param[out] out The rotated vector.
    /// @param v The vector to rotate.
    /// @param m The transformation matrix.
    /// @sa NuVecMtxTransform
    void NuVecMtxRotate(NUVEC *out, NUVEC *v, struct numtx_s *m);

    /// @relatesalso nuvec_s
    /// @relatesalso numtx_s
    /// @brief Scales a vector with the given transformation matrix.
    /// @param[out] out The scaled vector.
    /// @param v The vector to scale.
    /// @param m The transformation matrix.
    /// @sa NuVecMtxTransform
    void NuVecMtxScale(NUVEC *out, NUVEC *v, struct numtx_s *m);

    /// @relatesalso nuvec_s
    /// @relatesalso numtx_s
    /// @brief Translates a vector with the given transformation matrix.
    /// @param[out] out The translated vector.
    /// @param v The vector to translate.
    /// @param m The transformation matrix.
    /// @sa NuVecMtxTransform
    void NuVecMtxTranslate(NUVEC *out, NUVEC *v, struct numtx_s *m);

    /// @relatesalso nuvec_s
    /// @brief Computes the intersection of two lines, if any.
    /// @param pnt0,v0 The definition of the first line as point and direction.
    /// @param pnt1,v1 The definition of the second line as point and direction.
    /// @param[out] s The `x` coordinate of the intersection, if any.
    /// @param[out] t The `y` coordinate of the intersection, if any.
    /// @return 1 if the lines intersect, 0 otherwise.
    i32 NuLineLineIntersect(NUVEC *pnt0, NUVEC *v0, NUVEC *pnt1, NUVEC *v1, f32 *s, f32 *t);
    i32 NuPointRelToBoundingBox(NUVEC *point, NUVEC *maximum, NUVEC *minimum);
    void NuClipXPlane(NUVEC *out, NUVEC *point, NUVEC *direction, f32 *plane);
    void NuClipYPlane(NUVEC *out, NUVEC *point, NUVEC *direction, f32 *plane);
    void NuClipZPlane(NUVEC *out, NUVEC *point, NUVEC *direction, f32 *plane);
    i32 BoundingBoxToLine(NUVEC *minimum, NUVEC *maximum, struct numtx_s *matrix, NUVEC *start, NUVEC *end,
                          f32 expansion, NUVEC *intersection);
#ifdef __cplusplus
}

/// @relatesalso nuvec_s
/// @relatesalso numtx_s
/// @brief Transforms `count` vectors with the given transformation matrix.
/// @param[out] out An array of `count` transformed vectors.
/// @param v An array of `count` vectors to transform.
/// @param m The transformation matrix.
/// @param count The number of vectors in the arrays `out` and `v`.
/// @sa NuVecMtxTransform
void NuVecMtxTransformBlock(NUVEC *out, NUVEC *v, struct numtx_s *m, i32 count);
u32 NuVecToRGBA(NUVEC *v, f32 alpha);

#endif
