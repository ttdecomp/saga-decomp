#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nucore/nu2api_nucore_types.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/numath/nuquat.h"

#include <string.h>

extern "C" f32 NuAnimEndFrame(void *animation_data) {
    ani3_animheader_s *animation = static_cast<ani3_animheader_s *>(animation_data);
    if (animation->magic != ANI3_MAGIC_VERSION_4 && animation->magic != ANI3_MAGIC_VERSION_5) {
        return *static_cast<f32 *>(animation_data);
    }

    return static_cast<f32>(animation->frame_count) + static_cast<f32>(animation->first_frame);
}

extern "C" f32 NuAnimEndFrameOld(void *animation_data) {
    ani3_animheader_s *animation = static_cast<ani3_animheader_s *>(animation_data);
    if (animation->magic != ANI3_MAGIC_VERSION_4 && animation->magic != ANI3_MAGIC_VERSION_5) {
        return *static_cast<f32 *>(animation_data);
    }

    const u32 declared_end_frame = animation->declared_end_frame;
    if (declared_end_frame == 0) {
        return static_cast<f32>(animation->frame_count + animation->first_frame);
    }
    return static_cast<f32>(declared_end_frame);
}

void NuAnimBuffInit(i32 max_joints, variptr_u *buf, variptr_u) {
    MaxAnimJoints = max_joints;
    globalbuffer = NuAnimBuffCreate(max_joints, buf);
}

nuanimdatachunk_s *NuAnimDataChunkCreate(i32 curve_set_count) {
    NuMemoryManager *memory = NuMemoryGet()->GetThreadMem();
    u8 *data = static_cast<u8 *>(memory->_BlockAlloc(0x14, 4, 1, "", 0));
    *reinterpret_cast<i32 *>(data) = curve_set_count;
    *reinterpret_cast<void **>(data + 4) = NULL;
    *reinterpret_cast<void **>(data + 8) = NULL;
    *reinterpret_cast<void **>(data + 0xc) = NULL;
    *reinterpret_cast<void **>(data + 0x10) = NULL;

    const u32 array_size = static_cast<u32>(curve_set_count) * sizeof(void *);
    void **curve_sets = static_cast<void **>(memory->_BlockAlloc(array_size, 4, 1, "", 0));
    *reinterpret_cast<void ***>(data + 8) = curve_sets;
    memset(curve_sets, 0, array_size);
    return reinterpret_cast<nuanimdatachunk_s *>(data);
}

void NuAnimDataChunkDestroy(nuanimdatachunk_s *chunk) {
    u8 *data = reinterpret_cast<u8 *>(chunk);
    const i32 curve_set_count = *reinterpret_cast<i32 *>(data);
    void **curve_sets = *reinterpret_cast<void ***>(data + 8);
    const i32 destroy_curves = *reinterpret_cast<void **>(data + 0x10) == NULL;

    for (i32 index = 0; index < curve_set_count; ++index) {
        if (curve_sets[index] != NULL) {
            NuAnimCurveSetDestroy(curve_sets[index], destroy_curves);
        }
    }

    void *curve_data = *reinterpret_cast<void **>(data + 0xc);
    if (curve_data != NULL) {
        NuMemoryGet()->GetThreadMem()->BlockFree(curve_data, 0);
    }
    void *shared_data = *reinterpret_cast<void **>(data + 0x10);
    if (shared_data != NULL) {
        NuMemoryGet()->GetThreadMem()->BlockFree(shared_data, 0);
    }
    if (curve_sets != NULL) {
        NuMemoryGet()->GetThreadMem()->BlockFree(curve_sets, 0);
    }
    NuMemoryGet()->GetThreadMem()->BlockFree(chunk, 0);
}

void NuAnimRelocatePtrsANI3(ani3_animheader_s *animation, i32 offset) {
    ani3_animheader_s *block = animation;
    do {
        if (block->scale_min != NULL) {
            block->scale_min = reinterpret_cast<ani3_scalemin_s *>(reinterpret_cast<usize>(block->scale_min) + offset);
        }
        if (block->constants != NULL) {
            block->constants = reinterpret_cast<i16 *>(reinterpret_cast<usize>(block->constants) + offset);
        }
        if (block->curve_types != NULL) {
            block->curve_types = reinterpret_cast<u16 *>(reinterpret_cast<usize>(block->curve_types) + offset);
        }
        if (block->keys != NULL) {
            block->keys = reinterpret_cast<u8 *>(reinterpret_cast<usize>(block->keys) + offset);
        }
        if (block->node_flags != NULL) {
            block->node_flags = reinterpret_cast<u8 *>(reinterpret_cast<usize>(block->node_flags) + offset);
        }
        if (block->field_38 != NULL) {
            block->field_38 = reinterpret_cast<void *>(reinterpret_cast<usize>(block->field_38) + offset);
        }
        if (block->next_block == 0) {
            break;
        }
        block = reinterpret_cast<ani3_animheader_s *>(reinterpret_cast<u8 *>(block) + block->next_block);
    } while (true);
}

u32 NuAnimGetAnimDataSizeANI3(ani3_animheader_s *animation) {
    ani3_animheader_s *block = animation;
    while (block->next_block != 0) {
        block = reinterpret_cast<ani3_animheader_s *>(reinterpret_cast<u8 *>(block) + block->next_block);
    }
    if (block->node_flags == NULL) {
        return 0;
    }
    return reinterpret_cast<usize>(block->node_flags) + block->node_count - reinterpret_cast<usize>(animation);
}

void NuAnimBuffEvaluate_3_QuatB(numtx_s *base, nuanimbuff_s *buffer, nugscn_s *scene, numtx_s *matrices,
                                ani3_animheader_s *animation, NUHGOBJROOTFN root_fn, nuvec_s *root_translation,
                                void *root_data) {
    nuhgobj_s *object = reinterpret_cast<nuhgobj_s *>(scene);
    i32 count = animation->node_count < object->joint_count ? animation->node_count : object->joint_count;
    NUQUAT *quaternions = static_cast<NUQUAT *>(NuScratchAlloc32((count + 1) * 16));
    quaternions = reinterpret_cast<NUQUAT *>(ALIGN(reinterpret_cast<usize>(quaternions), 16));
    NUVEC *scales = static_cast<NUVEC *>(NuScratchAlloc32(count * sizeof(NUVEC)));
    if (!buffer)
        buffer = static_cast<nuanimbuff_s *>(globalbuffer);
    void *callback_data[256];
    if (AnimBuffEvalData && AnimBuffEvalJoint) {
        memset(callback_data, 0, object->joint_count * sizeof(void *));
        for (i32 index = 0; AnimBuffEvalData[index]; ++index) {
            i32 mapped = AnimBuffEvalJoint[index];
            if (mapped >= 0 && mapped < object->joint_override_map_count) {
                u8 joint_index = object->joint_override_map[mapped];
                if (joint_index != 255)
                    callback_data[joint_index] = AnimBuffEvalData[index];
            }
        }
    }
    // Slot255 and the root/nonroot scale selection follow the original evaluator.
    NUVEC root_scale;
    if (base) {
        root_scale = NuMtxGetScale(base);
        NuMtxToQuat(base, &quaternions[255]);
    } else {
        root_scale.x = root_scale.y = root_scale.z = 1.0f;
        quaternions[255] = NUQUAT{0, 0, 0, 1};
    }
    NUVEC root_values = {0, 0, 0};
    nuanimbuffjoint_s *joint = buffer->joints;
    NUQUAT *rotation = quaternions;
    NUMTX *matrix = matrices;
    NUVEC *scale = scales;
    const u8 *joint_flags = buffer->joint_flags;
    for (i32 index = 0; index < count; ++index, ++joint, ++rotation, ++matrix, ++scale) {
        u8 parent = object->joints[index].parent_index;
        u8 flags = *joint_flags++;
        if (flags & 1)
            *rotation = *reinterpret_cast<NUQUAT *>(&joint->rotation);
        else
            *rotation = NUQUAT{0, 0, 0, 1};
        NUVEC *parent_scale;
        if (parent != 255) {
            const NUQUAT q = *rotation, p = quaternions[parent];
            rotation->z = (p.w * q.z + q.w * p.z + p.x * q.y) - q.x * p.y;
            rotation->x = (p.w * q.x + q.w * p.x + p.y * q.z) - p.z * q.y;
            rotation->y = (p.w * q.y + q.w * p.y + p.z * q.x) - p.x * q.z;
            rotation->w = ((p.w * q.w - q.x * p.x) - q.y * p.y) - q.z * p.z;
            parent_scale = &root_scale;
        } else
            parent_scale = &scales[parent];
        NuQuatToMtx(rotation, matrix);
        if (flags & 8) {
            scale->x = joint->scale.x * parent_scale->x;
            scale->y = joint->scale.y * parent_scale->y;
            scale->z = joint->scale.z * parent_scale->z;
            NuMtxPreScaleVU0(matrix, scale);
        } else
            *scale = *parent_scale;
        if (flags & 16)
            scale->x = scale->y = scale->z = 1.0f;
        if (flags & 2) {
            root_values.x = joint->translation.x;
            root_values.y = joint->translation.y;
            root_values.z = -joint->translation.z;
            if (parent != 255)
                NuVecMtxTransform(reinterpret_cast<NUVEC *>(&matrix->m30), &joint->translation, &matrices[parent]);
            else if (base)
                NuVecMtxTransform(reinterpret_cast<NUVEC *>(&matrix->m30), &joint->translation, base);
            else {
                matrix->m30 = joint->translation.x;
                matrix->m31 = joint->translation.y;
                matrix->m32 = joint->translation.z;
            }
        } else if (parent != 255) {
            matrix->m30 = matrices[parent].m30;
            matrix->m31 = matrices[parent].m31;
            matrix->m32 = matrices[parent].m32;
            matrix->m33 = matrices[parent].m33;
        } else if (base) {
            matrix->m30 = base->m30;
            matrix->m31 = base->m31;
            matrix->m32 = base->m32;
            matrix->m33 = base->m33;
        }
        if (parent == 255) {
            if (root_fn)
                root_fn(matrix, root_data, &root_values, &root_values, root_translation, 0.0f);
            root_fn = NULL;
            if (flags & 64)
                scale->x = scale->y = scale->z = 1.0f;
        }
        if (AnimBuffEvalCB && callback_data[index])
            AnimBuffEvalCB(matrix, callback_data[index], rotation);
    }
    // Callbacks observe the engine coordinates; reflect the completed hierarchy afterward.
    for (i32 index = 0; index < count; ++index) {
        NUMTX *matrix = &matrices[index];
        matrix->m02 = -matrix->m02;
        matrix->m12 = -matrix->m12;
        matrix->m20 = -matrix->m20;
        matrix->m21 = -matrix->m21;
        matrix->m23 = -matrix->m23;
        matrix->m32 = -matrix->m32;
    }
    for (i32 index = count; index < object->joint_count; ++index)
        NuMtxSetIdentity(&matrices[index]);
    AnimBuffEvalCB = NULL;
    AnimBuffEvalData = NULL;
    AnimBuffEvalJoint = NULL;
    NuScratchRelease();
    NuScratchRelease();
}
