#include "gamelib/nuwind/nuwind.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"

// The legacy wind-group routines have unoptimized code in the reference.
extern "C" {
    struct NuPlainSpecialHandleLayout {
        NUGSCN *scene;
        void *special;
        void *display_special;
    };
    extern "C++" NuWindGType *NuWindAllocateGrp();
    extern "C++" void NuWindFreeGrp(NuWindGType *);
    void NuWindUpdateArray(NUVEC **);
    static i32 maxwindmats;
    static i32 maxgroups;
    NuWindGType *NuWindGroup;
    NuWindGType *NuWindCurGrp;
    i32 NuWindWave;
    i32 NuWindDir;
    i32 NuWindDir2;
    u32 NuWindQS = 0x1365;

    NuWindGType *NuWindCreateMtx(u32 *source, NUMTX *matrices, i16 count, f32 value20, f32 value24, i32 flags,
                                 f32 near_distance, f32 far_distance) {
        NuWindGType *group = NuWindAllocateGrp();
        NUMTX *matrix = matrices;
        if (group == NULL || matrix == NULL) {
            NuWindFreeGrp(group);
            return NULL;
        }
        group->matrices = matrices;
        group->unknown_0x04 = source[0];
        group->unknown_0x08 = source[2];
        group->matrix_count = count;
        group->unknown_0x20 = value20;
        group->unknown_0x24 = value24;
        group->flags = flags;
        group->near_distance = near_distance;
        group->far_distance = far_distance;
        group->extended_radius_squared = far_distance * far_distance;
        group->distance_range = group->far_distance - group->near_distance;
        group->unknown_0x48 = 0.2f;
        group->unknown_0x40 = 0.025f;
        group->unknown_0x44 = 0.0006250000442378223f;
        f32 min_x, min_y, min_z, max_x, max_y, max_z;
        min_x = min_y = min_z = 10000000.0f;
        max_x = max_y = max_z = -10000000.0f;
        for (i32 index = 0; index < count; ++index, ++matrix) {
            if (min_x > matrix->m30)
                min_x = matrix->m30;
            if (min_y > matrix->m31)
                min_y = matrix->m31;
            if (min_z > matrix->m32)
                min_z = matrix->m32;
            if (matrix->m30 > max_x)
                max_x = matrix->m30;
            if (matrix->m31 > max_y)
                max_y = matrix->m31;
            if (matrix->m32 > max_z)
                max_z = matrix->m32;
        }
        group->center.x = 0.5f * (max_x + min_x);
        group->center.y = 0.5f * (max_y + min_y);
        group->center.z = 0.5f * (max_z + min_z);
        group->radius_squared = 1.5f + ((0.5f * (max_x - min_x)) * ((max_x - min_x) * 0.5f) +
                                        ((max_y - min_y) * 0.5f) * (0.5f * (max_y - min_y)) +
                                        ((max_z - min_z) * 0.5f) * (0.5f * (max_z - min_z)));
        group->extended_radius_squared += group->radius_squared;
        group->radius = NuFsqrt(group->radius_squared);
        return group;
    }

    void NuWindDraw(void) {
        NuPlainSpecialHandleLayout special;
        special.special = NULL;
        NuWindGType *group = NuWindGroup;
        for (i32 index = 0; index < maxgroups; ++index, ++group) {
            if (group->in_use == 0 || group->visible == 0)
                continue;
            NuWindCurGrp = group;
            group->drawn = 0;
            NUMTX *matrix = group->matrices;
            f32 near_squared = group->near_distance * group->near_distance;
            f32 far_squared = group->far_distance * group->far_distance;
            for (i32 instance = 0; instance < group->matrix_count; ++instance, ++matrix) {
                f32 saved_m23 = matrix->m23;
                f32 saved_m33 = matrix->m33;
                matrix->m33 = 1.0f;
                f32 dx = matrix->m30 - global_camera.mtx.m30;
                f32 dy = matrix->m31 - global_camera.mtx.m31;
                f32 dz = matrix->m32 - global_camera.mtx.m32;
                f32 distance_squared = dx * dx + dy * dy + dz * dz;
                if (far_squared > distance_squared) {
                    matrix->m23 = near_squared > distance_squared ? 0.0f : 1.0e-11f;
                    special.scene = reinterpret_cast<NUGSCN *>((usize)group->unknown_0x04);
                    special.display_special = reinterpret_cast<void *>((usize)group->unknown_0x08);
                    if (NuSpecialDrawAt(&special, matrix) != 0)
                        group->drawn = 1;
                }
                matrix->m23 = saved_m23;
                matrix->m33 = saved_m33;
            }
        }
    }

    void NuWindInit(void) {
        NuWindWave = 0;
        NuWindDir = 0;
        NuWindDir2 = 0;
        for (i32 index = 0; index < maxgroups; ++index) {
            NuWindGroup[index].in_use = 0;
        }
    }

    u32 NuWindRand(void) {
        NuWindQS = (NuWindQS * 0x24cd + 1) & 0xffff;
        return NuWindQS;
    }

    extern "C++" NuWindGType *NuWindAllocateGrp() {
        for (i32 index = 0; index < maxgroups; ++index) {
            if (NuWindGroup[index].in_use == 0) {
                NuWindGroup[index].in_use = 1;
                return &NuWindGroup[index];
            }
        }
        return NULL;
    }

    void NuWindSetup(VARIPTR *buffer, VARIPTR buffer_end, i32 matrix_count, i32 group_count) {
        maxwindmats = matrix_count;
        maxgroups = group_count;
        buffer->addr = (buffer->addr + 15) & ~(usize)15;
        NuWindGroup = static_cast<NuWindGType *>(buffer->void_ptr);
        buffer->addr += group_count * sizeof(NuWindGType);
    }

    void NuWindUpdate(NUVEC *wind) {
        NUVEC *winds[8];
        for (i32 index = 1; index <= 7; ++index) {
            winds[index] = NULL;
        }
        winds[0] = wind;
        NuWindUpdateArray(winds);
    }

    void NuWindUpdateArray(NUVEC **positions) {
        NuWindGType *group = NuWindGroup;
        NuWindWave = (i32)((u32)NuWindWave + 501);
        NuWindDir = (i32)((u32)NuWindDir + 133);
        NuWindDir2 = (i32)((u32)NuWindDir2 + 377);
        f32 wind_x = NU_SIN_LUT((i32)(16384.0f + NU_SIN_LUT(NuWindDir) * 8192.0f)) * 0.75f +
                     NU_SIN_LUT((i32)((u32)NuWindDir2 + 0x4000)) * 0.15f;
        f32 wind_z = NU_SIN_LUT((i32)(NU_SIN_LUT(NuWindDir) * 8192.0f)) * 0.75f - NU_SIN_LUT(NuWindDir2) * 0.15f;
        i32 any_interaction = 0;
        i32 nearby[8];
        for (i32 index = 0; index < maxgroups; ++index, ++group) {
            if (group->in_use == 0)
                continue;
            f32 dx = group->center.x - global_camera.mtx.m30;
            f32 dy = group->center.y - global_camera.mtx.m31;
            f32 dz = group->center.z - global_camera.mtx.m32;
            f32 distance_squared = dz * dz + (dx * dx + dy * dy);
            if (group->extended_radius_squared > distance_squared) {
                group->visible = 1;
                if (group->drawn == 0 || !(group->unknown_0x20 > 0.0f))
                    continue;
                if (group->flags != 0) {
                    for (i32 object = 0; object < 8; ++object) {
                        if (positions[object] == NULL) {
                            nearby[object] = 0;
                            continue;
                        }
                        f32 x = group->center.x - positions[object]->x;
                        f32 y = group->center.y - positions[object]->y;
                        f32 z = group->center.z - positions[object]->z;
                        if (group->radius_squared > z * z + (x * x + y * y)) {
                            nearby[object] = 1;
                            any_interaction = 1;
                        } else {
                            nearby[object] = 0;
                        }
                    }
                }
                NUMTX *matrix = group->matrices;
                for (i32 instance = 0; instance < group->matrix_count; ++instance, ++matrix) {
                    i32 closest = -1;
                    f32 contact_radius = 0.0f;
                    f32 contact_height = 0.0f;
                    f32 contact_x = 0.0f;
                    f32 contact_z = 0.0f;
                    f32 contact_squared = 0.0f;
                    bool interacting = group->flags != 0 && any_interaction != 0;
                    if (interacting) {
                        f32 closest_squared = 1000000.0f;
                        for (i32 object = 0; object < 8; ++object) {
                            if (positions[object] == NULL)
                                continue;
                            f32 x = matrix->m30 - positions[object]->x;
                            f32 z = matrix->m32 - positions[object]->z;
                            f32 squared = x * x + z * z;
                            if (closest_squared > squared) {
                                closest_squared = squared;
                                closest = object;
                            }
                        }
                        if (closest == -1)
                            break;
                        f32 height = positions[closest]->y - matrix->m31;
                        if (height > (group->unknown_0x24 * matrix->m33) / group->unknown_0x20) {
                            contact_radius = 0.0f;
                            contact_height = (matrix->m33 * 0.2f) / group->unknown_0x20;
                        } else {
                            contact_radius = 0.45f;
                            if (0.0f > height) {
                                contact_height = (matrix->m33 * 0.2f) / group->unknown_0x20;
                            } else {
                                contact_height = (matrix->m33 * (height * 0.5f + 0.2f)) / group->unknown_0x20;
                            }
                        }
                        matrix->m10 *= matrix->m23;
                        matrix->m12 *= matrix->m23;
                        contact_x = (matrix->m30 + matrix->m10 * contact_height) - positions[closest]->x;
                        contact_z = (matrix->m32 + matrix->m12 * contact_height) - positions[closest]->z;
                        contact_squared = contact_x * contact_x + contact_z * contact_z;
                    }
                    if (!interacting) {
                        f32 target_x =
                            matrix->m33 * wind_x *
                            (NU_SIN_LUT((i32)((f32)NuWindWave + (matrix->m30 + matrix->m32) * 8192.0f)) * 0.5f + 1.0f);
                        f32 target_z =
                            matrix->m33 * wind_z *
                            (NU_SIN_LUT((i32)(16384.0f + ((f32)NuWindWave + (matrix->m30 + matrix->m32) * 8192.0f))) *
                                 0.5f +
                             1.0f);

                        matrix->m10 += (target_x - matrix->m10) * 0.2f;
                        matrix->m12 += (target_z - matrix->m12) * 0.2f;
                    } else if (contact_squared >= contact_radius * contact_radius) {
                        f32 target_x =
                            matrix->m33 * wind_x *
                            (NU_SIN_LUT((i32)((f32)NuWindWave + (matrix->m30 + matrix->m32) * 8192.0f)) * 0.5f + 1.0f);
                        f32 target_z =
                            matrix->m33 * wind_z *
                            (NU_SIN_LUT((i32)(16384.0f + ((f32)NuWindWave + (matrix->m30 + matrix->m32) * 8192.0f))) *
                                 0.5f +
                             1.0f);

                        f32 x = 0.2f * (target_x - matrix->m10);
                        f32 z = 0.2f * (target_z - matrix->m12);
                        f32 squared = x * x + z * z;
                        if (squared > 0.000144f) {
                            f32 scale = 0.012f / NuFsqrt(squared);
                            x *= scale;
                            z *= scale;
                        }
                        matrix->m10 += x;
                        matrix->m12 += z;
                    } else {
                        if (contact_squared == 0.0f) {
                            contact_x = contact_radius;
                            contact_z = 0.0f;
                        } else {
                            f32 scale = contact_radius / NuFsqrt(contact_squared);
                            contact_x *= scale;
                            contact_z *= scale;
                        }
                        contact_x = (0.3f * ((positions[closest]->x + contact_x) - matrix->m30)) / contact_height;
                        contact_z = (0.3f * ((positions[closest]->z + contact_z) - matrix->m32)) / contact_height;
                        f32 squared = contact_x * contact_x + contact_z * contact_z;
                        if (squared > 0.25f) {
                            f32 scale = 0.5f / NuFsqrt(squared);
                            contact_x *= scale;
                            contact_z *= scale;
                        }
                        contact_x = group->unknown_0x48 * (contact_x - matrix->m10);
                        contact_z = group->unknown_0x48 * (contact_z - matrix->m12);
                        squared = contact_x * contact_x + contact_z * contact_z;
                        if (squared > group->unknown_0x44) {
                            f32 scale = group->unknown_0x40 / NuFsqrt(squared);
                            contact_x *= scale;
                            contact_z *= scale;
                        }
                        matrix->m10 += contact_x;
                        matrix->m12 += contact_z;
                        f32 target_x =
                            matrix->m33 * wind_x *
                            (NU_SIN_LUT((i32)((f32)NuWindWave + (matrix->m30 + matrix->m32) * 8192.0f)) * 0.5f + 1.0f);
                        f32 target_z =
                            matrix->m33 * wind_z *
                            (NU_SIN_LUT((i32)(16384.0f + ((f32)NuWindWave + (matrix->m30 + matrix->m32) * 8192.0f))) *
                                 0.5f +
                             1.0f);
                        matrix->m10 += (target_x - matrix->m10) * 0.05f;
                        matrix->m12 += (target_z - matrix->m12) * 0.05f;
                    }
                    f32 scale = 1.0f - NuFsqrt(matrix->m10 * matrix->m10 + matrix->m12 * matrix->m12) * 0.35f;
                    if (0.05f > scale)
                        scale = 0.05f;
                    matrix->m10 *= scale;
                    matrix->m12 *= scale;
                    matrix->m23 = 1.0f / scale;
                    if (0.19f > scale)
                        scale = 0.19f;
                    matrix->m11 = (matrix->m33 / group->unknown_0x20) * scale;
                }
            } else {
                group->visible = 0;
            }
        }
    }
}
