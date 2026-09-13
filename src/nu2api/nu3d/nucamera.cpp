#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>

NUMTX clip_test_mtx;

void NuCameraSetProjectionMtx(NUMTX *mtx, f32 fov, f32 aspect, f32 near_clip, f32 far_clip) {
    near_clip = near_clip < 0.1f ? 0.1f : near_clip;
    i32 angle = (i32)(fov / 2.0f * 10430.378f);
    f32 cotangent = NuTrigTable[(angle + 0x4000) >> 1 & 0x7fff] / NuTrigTable[angle >> 1 & 0x7fff];
    f32 x_scale = aspect * cotangent;
    f32 y_scale = cotangent;
    f32 depth = far_clip / (far_clip - near_clip);
    memset(mtx, 0, sizeof(*mtx));
    mtx->m00 = x_scale;
    mtx->m11 = y_scale;
    mtx->m22 = depth;
    mtx->m23 = 1.0f;
    mtx->m32 = -depth * near_clip;
}

void NuCameraRestoreState(i32 handle) {
    i32 index = handle - 1;
    if (index < cam_state_count) {
        NUCAMERASTATE *state = &cam_state[index];
        global_camera = state->camera;
        vmtx = state->view;
        pmtx = state->projection;
        smtx = state->screen;
        vpmtx = state->view_projection;
        vpc_sci_mtx = state->view_projection_scissor;
        vpc_vport_mtx = state->view_projection_viewport;
        pc_vport_mtx = state->viewport;
        vpsmtx = state->view_projection_screen;
        psmtx = state->projection_screen;
        camfx = state->effects;
        zx = state->zx;
        zy = state->zy;
        zxs = state->zxs;
        zys = state->zys;
        // The original restores the derived matrices but not ClipPlanes or
        // the stack count; the handle remains available for subsequent use.
        FaceYDirStream(NuAtan2D(-global_camera.mtx.m20, -global_camera.mtx.m22));
        NuRndrSetViewMtx(&vpsmtx, &vpc_vport_mtx, &vpc_sci_mtx);
    }
}

SAGA_HOST_WEAK i32 NuCameraClipTestExtents(NUVEC *min, NUVEC *max, NUMTX *world_mtx, f32 far_clip,
                                           i32 should_clip_to_screen) {
    NUVEC extents[8];
    NUVEC clip_space_extents[8];
    char results[8];
    i32 i;

    if (far_clip == 0.0f) {
        if (global_camera.unknown_64 != 0.0f) {
            far_clip = global_camera.unknown_64;
        } else {
            far_clip = global_camera.far_clip;
        }
    }

    if (world_mtx == NULL || world_mtx == &numtx_identity) {
        clip_test_mtx = vmtx;
    } else {
        NuMtxMul(&clip_test_mtx, world_mtx, &vmtx);
    }

    extents[0].x = min->x;
    extents[0].y = min->y;
    extents[0].z = min->z;

    extents[1].x = max->x;
    extents[1].y = max->y;
    extents[1].z = max->z;

    extents[2].x = min->x;
    extents[2].y = min->y;
    extents[2].z = max->z;

    extents[3].x = max->x;
    extents[3].y = max->y;
    extents[3].z = min->z;

    extents[4].x = min->x;
    extents[4].y = max->y;
    extents[4].z = min->z;

    extents[5].x = min->x;
    extents[5].y = max->y;
    extents[5].z = max->z;

    extents[6].x = max->x;
    extents[6].y = min->y;
    extents[6].z = min->z;

    extents[7].x = max->x;
    extents[7].y = min->y;
    extents[7].z = max->z;

    NuVecMtxTransformBlock(clip_space_extents, extents, &clip_test_mtx, 8);
    memset(results, 0, sizeof(results));

    for (i = 0; i < 8; i++) {
        if (clip_space_extents[i].x > clip_space_extents[i].z * zx) {
            results[i] |= 0x02;
        }

        if (clip_space_extents[i].y > clip_space_extents[i].z * zy) {
            results[i] |= 0x04;
        }

        if (clip_space_extents[i].z > far_clip) {
            results[i] |= 0x10;
        }

        if (clip_space_extents[i].x < -clip_space_extents[i].z * zx) {
            results[i] |= 0x01;
        }

        if (clip_space_extents[i].y < -clip_space_extents[i].z * zy) {
            results[i] |= 0x08;
        }

        if (clip_space_extents[i].z < global_camera.near_clip) {
            results[i] |= 0x20;
        }
    }

    if ((results[0] & results[1] & results[2] & results[3] & results[4] & results[5] & results[6] & results[7]) != 0) {
        return 0;
    }

    if ((results[0] | results[1] | results[2] | results[3] | results[4] | results[5] | results[6] | results[7]) == 0) {
        return 1;
    }

    return 2;
}

// The display-scene format stores each axis-aligned bound as center + extent,
// not as minimum + maximum corners.  The target tests those values directly
// against the world-space planes built by NuCameraBuildClipPlanes.  Treating
// them as corners makes large bounds (notably the Cantina floor and walls)
// appear outside the camera even while the camera is inside them.
SAGA_HOST_WEAK i32 NuCameraClipTestExtentsAxisAligned(NUVEC *center, NUVEC *extent, f32 far_clip) {
    auto transformPlanes = [](const NUVEC &point, const NUMTX &planes, f32 out[4]) {
        out[0] = point.x * planes.m00 + point.y * planes.m10 + point.z * planes.m20 + planes.m30;
        out[1] = point.x * planes.m01 + point.y * planes.m11 + point.z * planes.m21 + planes.m31;
        out[2] = point.x * planes.m02 + point.y * planes.m12 + point.z * planes.m22 + planes.m32;
        out[3] = point.x * planes.m03 + point.y * planes.m13 + point.z * planes.m23 + planes.m33;
    };
    auto projectExtent = [](const NUVEC &value, const NUMTX &absolute_planes, f32 out[4]) {
        out[0] = value.x * absolute_planes.m00 + value.y * absolute_planes.m10 + value.z * absolute_planes.m20;
        out[1] = value.x * absolute_planes.m01 + value.y * absolute_planes.m11 + value.z * absolute_planes.m21;
        out[2] = value.x * absolute_planes.m02 + value.y * absolute_planes.m12 + value.z * absolute_planes.m22;
        out[3] = value.x * absolute_planes.m03 + value.y * absolute_planes.m13 + value.z * absolute_planes.m23;
    };

    f32 distance[6];
    f32 radius[6];
    transformPlanes(*center, ClipPlanes.frustum_planes, distance);
    projectExtent(*extent, ClipPlanes.abs_frustum_planes, radius);

    NUMTX near_far_planes = ClipPlanes.near_far_planes;
    if (far_clip != 0.0f) {
        near_far_planes.m30 += far_clip - global_camera.far_clip;
    }
    distance[4] = center->x * near_far_planes.m00 + center->y * near_far_planes.m10 + center->z * near_far_planes.m20 +
                  near_far_planes.m30;
    distance[5] = center->x * near_far_planes.m01 + center->y * near_far_planes.m11 + center->z * near_far_planes.m21 +
                  near_far_planes.m31;
    radius[4] = extent->x * near_far_planes.m02 + extent->y * near_far_planes.m12 + extent->z * near_far_planes.m22;
    radius[5] = extent->x * near_far_planes.m03 + extent->y * near_far_planes.m13 + extent->z * near_far_planes.m23;

    for (i32 plane = 0; plane < 6; ++plane) {
        if (distance[plane] < -radius[plane]) {
            return 0;
        }
    }

    for (i32 plane = 0; plane < 4; ++plane) {
        if (radius[plane] > distance[plane]) {
            f32 scissor_distance[4];
            f32 scissor_radius[4];
            transformPlanes(*center, ClipPlanes.scissor_planes, scissor_distance);
            projectExtent(*extent, ClipPlanes.abs_scissor_planes, scissor_radius);
            for (i32 scissor_plane = 0; scissor_plane < 4; ++scissor_plane) {
                if (scissor_radius[scissor_plane] > scissor_distance[scissor_plane]) {
                    return 2;
                }
            }
            break;
        }
    }
    return 1;
}
