#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "globals.h"

#include "nu2api/numath/nurand.h"

#include <stdlib.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern i32 qseed;
extern i32 GAMERAND;
extern "C" void DebrisSetSeed(i32);
extern "C" void NuPartSetSeed(i32);

void HashString(unsigned char *) {
}

void ResetSeeds() {
    srand48(0);
    qseed = 0x3039;
    GAMERAND = 0x1f3ad27f;
    NuRandSeed(0);
    DebrisSetSeed(0);
    NuRandSetSeed(NULL, 0);
    NuPartSetSeed(0);
}

void makenuvec4(float, float, float, float) {
}

i32 i_temp_xrot;

void FindAnglesXY(nuvec_s *direction, u16 *x_rotation, u16 *y_rotation) {
    temp_yrot = NuAtan2D(direction->x, direction->z);
    if (y_rotation != NULL)
        *y_rotation = temp_yrot;
    i_temp_xrot = -NuAtan2D(direction->y, NuFsqrt(direction->x * direction->x + direction->z * direction->z));
    temp_xrot = i_temp_xrot;
    if (x_rotation != NULL)
        *x_rotation = temp_xrot;
}

void FindAnglesZX(nuvec_s *normal, u16 *x_rotation, u16 *z_rotation) {
    const i32 x_angle = NuAtan2D(normal->z, normal->y);
    if (x_rotation != NULL) {
        *x_rotation = static_cast<u16>(x_angle);
    }
    temp_xrot = static_cast<i16>(x_angle);

    NUVEC rotated;
    const u16 truncated_x_angle = static_cast<u16>(x_angle);
    NuVecRotateX(&rotated, normal, static_cast<NUANG>(-static_cast<i32>(truncated_x_angle)));
    const i32 z_angle = -NuAtan2D(rotated.x, rotated.y);
    if (z_rotation != NULL) {
        *z_rotation = static_cast<u16>(z_angle);
    }
    temp_zrot = static_cast<i16>(z_angle);
}

void getNumDigits(i32) {
}

void LineCrossedXZ(float, float, float, float, float, float, float, float) {
}

void ScaleAndClamp(i32) {
}

void VecRotateAxis(nuvec_s *vector, u16 angle, nuvec_s *axis) {
    NuVecNorm(axis, axis);
    const f32 cosine = NuTrigTable[((static_cast<u32>(angle) + 0x4000) >> 1) & 0x7fff];
    const f32 sine = NuTrigTable[angle >> 1];
    const f32 complement = 1.0f - cosine;
    const f32 x = axis->x;
    const f32 y = axis->y;
    const f32 z = axis->z;
    const NUVEC source = *vector;
    const f32 tx = complement * x;
    const f32 ty = complement * y;
    const f32 tz = complement * z;
    const f32 xy = tx * y;
    const f32 xz = tx * z;
    const f32 yz = ty * z;
    const f32 sx = sine * x;
    const f32 sy = sine * y;
    const f32 sz = z * sine;
    vector->x = ((tx * x + cosine) * source.x + 0.0f) + (xy - sz) * source.y + (xz + sy) * source.z;
    vector->y = ((xy + sz) * source.x + 0.0f) + (y * ty + cosine) * source.y + (yz - sx) * source.z;
    vector->z = ((xz - sy) * source.x + 0.0f) + (sx + yz) * source.y + (tz * z + cosine) * source.z;
}

i32 SolveQuadratic(f32 a, f32 b, f32 c, f32 *first, f32 *second) {
    if (a == 0.0f) {
        if (b == 0.0f)
            return 0;
        *first = *second = -c / b;
        return 1;
    }
    const f32 four_ac = 4.0f * a * c;
    const f32 b_squared = b * b;
    if (four_ac > b_squared)
        return 0;
    if (four_ac == b_squared) {
        *first = *second = -b / (a + a);
        return 1;
    }
    const f32 root = NuFsqrt(b_squared - four_ac);
    *first = (-b - root) / (a + a);
    *second = (root - b) / (a + a);
    return 1;
}

f32 XZLinesClosest(nuvec_s *a, nuvec_s *b, nuvec_s *c, nuvec_s *d, f32 *first, f32 *second) {
    NUVEC ac __attribute__((aligned(16)));
    NUVEC bc __attribute__((aligned(16)));
    NUVEC ca __attribute__((aligned(16)));
    NUVEC da __attribute__((aligned(16)));
    NUVEC direction;
    NuVecSub(&direction, b, a);
    i32 angle = NuAtan2D(direction.x, direction.z);
    NuVecSub(&ca, c, a);
    NuVecRotateY(&ca, &ca, -angle);
    NuVecSub(&da, d, a);
    NuVecRotateY(&da, &da, -angle);
    NuVecSub(&direction, d, c);
    angle = NuAtan2D(direction.x, direction.z);
    NuVecSub(&ac, a, c);
    NuVecRotateY(&ac, &ac, -angle);
    NuVecSub(&bc, b, c);
    NuVecRotateY(&bc, &bc, -angle);

    if (NuFsign(ca.x) != NuFsign(da.x) && NuFsign(ac.x) != NuFsign(bc.x)) {
        *first = NuFabs(ac.x) / (NuFabs(ac.x) + NuFabs(bc.x));
        *second = NuFabs(ca.x) / (NuFabs(ca.x) + NuFabs(da.x));
        return 0.0f;
    }

    f32 ab_length = NuVecXZDist(b, a, NULL);
    f32 cd_length = NuVecXZDist(d, c, NULL);
    f32 closest = 1000000000.0f;
    f32 distance, fraction;
    if (ca.z < 0.0f) {
        distance = NuVecXZDist(a, &ca, NULL);
        fraction = 0.0f;
    } else if (ca.z > ab_length) {
        distance = NuVecXZDist(b, &ca, NULL);
        fraction = 1.0f;
    } else {
        fraction = ca.z / ab_length;
        distance = NuFabs(ca.x);
    }
    if (distance < closest) {
        *first = fraction;
        *second = 0.0f;
        closest = distance;
    }
    if (da.z < 0.0f) {
        distance = NuVecXZDist(a, &da, NULL);
        fraction = 0.0f;
    } else if (da.z > ab_length) {
        distance = NuVecXZDist(b, &da, NULL);
        fraction = 1.0f;
    } else {
        fraction = da.z / ab_length;
        distance = NuFabs(da.x);
    }
    if (distance < closest) {
        *first = fraction;
        *second = 1.0f;
        closest = distance;
    }
    if (ac.z < 0.0f) {
        distance = NuVecXZDist(c, &ac, NULL);
        fraction = 0.0f;
    } else if (ac.z > cd_length) {
        distance = NuVecXZDist(d, &ac, NULL);
        fraction = 1.0f;
    } else {
        fraction = ac.z / cd_length;
        distance = NuFabs(ac.x);
    }
    if (distance < closest) {
        *second = fraction;
        *first = 0.0f;
        closest = distance;
    }
    if (bc.z < 0.0f) {
        distance = NuVecXZDist(c, &bc, NULL);
        fraction = 0.0f;
    } else if (bc.z > cd_length) {
        distance = NuVecXZDist(d, &bc, NULL);
        fraction = 1.0f;
    } else {
        fraction = bc.z / cd_length;
        distance = NuFabs(bc.x);
    }
    if (distance < closest) {
        *second = fraction;
        *first = 1.0f;
        closest = distance;
    }
    return closest;
}

i32 LineIntersectXY(nuvec_s *a, nuvec_s *b, nuvec_s *c, nuvec_s *d, nuvec_s *first, nuvec_s *second) {
    const f32 ax = a->x, ay = a->y;
    const f32 dx = b->x - ax, dy = b->y - ay;
    const f32 ex = d->x - c->x, ey = d->y - c->y;
    const f32 denominator = ey * dx - ex * dy;
    if (denominator == 0.0f)
        return 0;
    const f32 ox = ax - c->x, oy = ay - c->y;
    const f32 t = (ex * oy - ey * ox) / denominator;
    const f32 u = (oy * dx - ox * dy) / denominator;
    if (first != NULL) {
        first->x = dx * t + ax;
        first->y = dy * t + ay;
    }
    if (second != NULL) {
        second->x = (d->x - c->x) * u + c->x;
        second->y = (d->y - c->y) * u + c->y;
    }
    return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
}

void MakeThrowVector(NUVEC *result, NUVEC *origin, NUVEC *target, NUVEC *target_velocity, f32 speed, f32 gravity) {
    NUVEC predicted = *target;
    NUVEC delta;
    NUVEC horizontal_origin;
    horizontal_origin.x = origin->x;
    horizontal_origin.y = 0.0f;
    horizontal_origin.z = origin->z;
    NuVecSub(&delta, &predicted, &horizontal_origin);
    NuVecAddScale(&predicted, target, target_velocity, NuVecMag(&delta) / speed);
    horizontal_origin.x = origin->x;
    horizontal_origin.y = 0.0f;
    horizontal_origin.z = origin->z;
    NuVecSub(&delta, &predicted, &horizontal_origin);
    NuVecAddScale(&predicted, target, target_velocity, NuVecMag(&delta) / speed);
    horizontal_origin.x = origin->x;
    horizontal_origin.y = 0.0f;
    horizontal_origin.z = origin->z;
    NuVecSub(&delta, &predicted, &horizontal_origin);
    NuVecAddScale(&predicted, target, target_velocity, NuVecMag(&delta) / speed);
    const f32 time = NuVecXZDist(&predicted, origin, NULL) / speed;
    const f32 vertical_speed = (predicted.y - origin->y) / (FRAMETIME + time) - (gravity * 0.5f) * time;
    const u16 angle = NuAtan2D(predicted.x - origin->x, predicted.z - origin->z);
    result->x = speed * NU_SIN_LUT(angle);
    result->y = vertical_speed;
    result->z = speed * NU_COS_LUT(angle);
}

i32 OnOrInsidePlane(nuvec_s *point, nuvec_s *plane_point, nuvec_s *plane_normal, nuvec_s *corrected_point,
                    float normal_offset, float *distance_out) {
    NUVEC test_point;
    if (normal_offset != 0.0f) {
        NuVecScale(&test_point, plane_normal, normal_offset);
        NuVecAdd(&test_point, &test_point, point);
    } else {
        test_point = *point;
    }

    const f32 distance = (test_point.x - plane_point->x) * plane_normal->x +
                         (test_point.y - plane_point->y) * plane_normal->y +
                         (test_point.z - plane_point->z) * plane_normal->z;
    if (distance_out != NULL) {
        *distance_out = distance;
    }
    if (!(distance <= 0.0f)) {
        return 0;
    }

    if (corrected_point != NULL) {
        NuVecScale(corrected_point, plane_normal, 0.5f - distance);
        NuVecAdd(corrected_point, corrected_point, point);
    }
    return 1;
}

void PackCharIntoInt(char, char, char, char) {
}

NUVEC TerCrossProduct(NUVEC *a, NUVEC *b) {
    NUVEC result;
    result.x = a->y * b->z - a->z * b->y;
    result.y = a->z * b->x - b->z * a->x;
    result.z = a->x * b->y - b->x * a->y;
    return result;
}

f32 DistanceToLineXZ(NUVEC *position, NUVEC *first, NUVEC *second) {
    u16 angle = -NuAtan2D(second->x - first->x, second->z - first->z);
    return NuFabs((position->x - first->x) * NU_COS_LUT(angle) + (position->z - first->z) * NU_SIN_LUT(angle));
}

i32 MatrixReflection(numtx_s *, i32, float, float, numtx_s *) {
    return 0;
}

void OnOrOutsidePlane(nuvec_s *, nuvec_s *, nuvec_s *) {
}

i32 PackShortIntoInt(i16 high, i16 low) {
    i32 packed = high;
    packed <<= 16;
    packed |= static_cast<u16>(low);
    return packed;
}

void RatioAlongLineXZ(nuvec_s *, nuvec_s *, nuvec_s *) {
}

i32 XZLinesIntersect(nuvec_s *, nuvec_s *, nuvec_s *, nuvec_s *, float *, float *) {
    return 0;
}

void GetRotationAngles(nuvec_s *, u16 *, u16 *) {
}

void UnpackCharFromInt(i32, char &, char &, char &, char &) {
}

void RatioBetweenPlanes(nuvec_s *, nuvec_s *, nuvec_s *, nuvec_s *, nuvec_s *) {
}

void UnpackShortFromInt(i32, i16 &, i16 &) {
}

void AnglesBetweenPoints(nuvec_s *, nuvec_s *, u16 *, u16 *) {
}

bool LineIntersectCircle(NUVEC *origin, NUVEC *direction, NUVEC *center, f32 radius_squared) {
    f32 x = center->x - origin->x;
    f32 z = center->z - origin->z;
    f32 projection = direction->x * x + direction->z * z;
    if (projection >= 0.0f)
        return x * x + z * z - projection * projection <= radius_squared;
    return false;
}

i32 LineIntersectSphere(NUVEC *origin, NUVEC *direction, NUVEC *center, f32 radius_squared, f32 *distance_squared) {
    f32 x = center->x - origin->x;
    f32 y = center->y - origin->y;
    f32 z = center->z - origin->z;
    f32 projection = direction->x * x + direction->y * y + direction->z * z;
    if (projection < 0.0f)
        return 0;
    f32 distance = x * x + y * y + z * z - projection * projection;
    if (radius_squared < distance)
        return 0;
    if (distance_squared != NULL)
        *distance_squared = distance;
    return 1;
}

void LineToPlaneDistance(VuVec &, VuVec &, VuVec &) {
}

void LineToPointDistance(VuVec &, VuVec &, VuVec &, VuVec *) {
}

void RatioBetweenEdgesXZ(nuvec_s *, nuvec_s *, nuvec_s *, nuvec_s *, nuvec_s *) {
}

bool SphereSphereOverlap(NUVEC *a, f32 radius_a, NUVEC *b, f32 radius_b) {
    const f32 x = b->x - a->x;
    const f32 y = b->y - a->y;
    const f32 z = b->z - a->z;
    const f32 radius = radius_b + radius_a;
    return x * x + y * y + z * z <= radius * radius;
}

void LineToPlaneIntersecion(VuVec &, VuVec &, VuVec &, VuVec *) {
}

void CalculateInterceptVector(NUVEC *origin, NUVEC *target, NUVEC *velocity, f32 speed, NUVEC *direction,
                              NUVEC *intercept) {
    NUVEC prediction = *target;
    {
        NuVecSub(direction, &prediction, origin);
        f32 distance = NuVecMag(direction);
        f32 time = distance;
        if (distance == 0.0f || speed == 0.0f)
            time = 0.0f;
        else
            time /= speed;
        NuVecAddScale(&prediction, target, velocity, time);
    }
    {
        NuVecSub(direction, &prediction, origin);
        f32 distance = NuVecMag(direction);
        f32 time = distance;
        if (distance == 0.0f || speed == 0.0f)
            time = 0.0f;
        else
            time /= speed;
        NuVecAddScale(&prediction, target, velocity, time);
    }
    {
        NuVecSub(direction, &prediction, origin);
        f32 distance = NuVecMag(direction);
        f32 time = distance;
        if (distance == 0.0f || speed == 0.0f)
            time = 0.0f;
        else
            time /= speed;
        NuVecAddScale(&prediction, target, velocity, time);
    }
    NuVecSub(direction, &prediction, origin);
    if (intercept != NULL)
        *intercept = prediction;
}

void LineToSphereIntersection(VuVec &, VuVec &, VuVec &, float, VuVec *, VuVec *) {
}

i32 MatrixReflectionVU0_AXISY(numtx_s *, float, float, numtx_s *) {
    return 0;
}

i32 SphereSphereOverlapScaleY(nuvec_s *position_a, float radius_a, float y_radius_a, nuvec_s *position_b,
                              float radius_b, float y_radius_b) {
    f32 dx = position_b->x - position_a->x;
    f32 dy = position_b->y - position_a->y;
    f32 dz = position_b->z - position_a->z;
    if (radius_b != y_radius_b || radius_a != y_radius_a) {
        dy *= (radius_b + radius_a) / (y_radius_b + y_radius_a);
    }
    f32 radius = radius_b + radius_a;
    return dx * dx + dy * dy + dz * dz <= radius * radius;
}

void IToX(char *, i32) {
}

void XToI(char *) {
}

void IsTok(char const *, char const *) {
}

void CapVec(nuvec_s *, float, nuvec_s *) {
}

void I64ToX(char *, i64) {
}

void XToI64(char *) {
}

i32 RotDiff(u16 current, u16 target) {
    i32 difference = static_cast<u32>(target) - static_cast<u32>(current);
    if (difference > 0x8000) {
        difference -= 0x10000;
    } else if (difference < -0x8000) {
        difference += 0x10000;
    }
    return difference;
}

void rawClip(VuVec const *, VuVec *, i32, VuVec const &) {
}

i32 getqseed() {
    return qseed;
}

f32 clampBias(f32 bias) {
    if (bias < -16.0f) {
        return -16.0f;
    }
    return 15.96875f < bias ? 15.96875f : bias;
}

i32 findrange(nugscn_s *scene, i32 first_joint) {
    // This helper's original C++ ABI names nugscn_s, but animation callers
    // pass the hierarchy object whose joint table starts at the same offsets.
    nuhgobj_s *object = reinterpret_cast<nuhgobj_s *>(scene);
    const i32 joint_count = object->joint_count;
    nuhgobjjoint_s *joints = object->joints;
    const i32 parent_index = joints[first_joint].parent_index;
    i32 end_joint = first_joint + 1;
    while (end_joint < joint_count && joints[end_joint].parent_index != parent_index) {
        ++end_joint;
    }
    return end_joint - 1;
}

void makenuvec(float, float, float) {
}

static __used__ i32 MatchExtension(char *, char *, i32) {
    return 0;
}

static __used__ int icomp(const void *, const void *) {
    return 0;
}

static __used__ i32 sort32a(void const *, void const *) {
    return 0;
}
