#include "nu2api/nu3d/nucamera.h"

#include "decomp.h"

#include <stdlib.h>
#include <string.h>

#include "nu2api/nucore/numemory.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"

NUCAMERA global_camera;
i32 cam_state_count;
NUCAMERASTATE cam_state[16];
NUVEC cam_axes = {1.0f, 1.0f, 1.0f};
i32 force_camera_farclip;
NUMTX cmtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
NUMTX psmtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
NUMTX vpsmtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
f32 nucamera_farclip_hack = 0.1f;
NUMTX pc_vport_mtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

NUMTX clip_planes = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

NUMTX pmtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
NUMTX smtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
NUMTX vmtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

NUMTX vpmtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

NUMTX vpc_sci_mtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

NUMTX vpc_vport_mtx = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

NUCLIPPLANES ClipPlanes;

f32 zx;
f32 zy;

f32 zxs;
f32 zys;

NUCAMERA *NuCameraCreate() {
    NUCAMERA *cam = NU_ALLOC_T(NUCAMERA, 1, "", NUMEMORY_CATEGORY_NONE);

    NuMtxSetIdentity(&cam->mtx);

    cam->near_clip = 0.15f;
    cam->far_clip = 10000.0f;
    cam->portal_nearclip = 0.0f;
    cam->fov = 0.75f;
    cam->aspect = 0.75f;

    cam->unknown_58 = 1.0f;
    cam->unknown_5c = 1.0f;
    cam->unknown_50 = 0.0f;
    cam->unknown_54 = 0.0f;

    cam->scale.z = 1.0f;
    cam->scale.y = 1.0f;
    cam->scale.x = 1.0f;

    cam->unknown_64 = 0.0f;
    cam->unknown_60 = 0.0f;

    return cam;
}

void NuCameraDestroy(NUCAMERA *cam) {
    if (cam != NULL) {
        NuMemoryGet()->GetThreadMem()->BlockFree(cam, 0);
    }
}

f32 NuCameraDist(NUVEC *v) {
    return NuVecDist(v, NUMTX_GET_ROW_VEC(&global_camera.mtx, 3), NULL);
}

f32 NuCameraDistSqr(NUVEC *v) {
    return NuVecDistSqr(v, NUMTX_GET_ROW_VEC(&global_camera.mtx, 3), NULL);
}

void NuCameraGet(NUCAMERA *out) {
    *out = global_camera;
}

NUCAMERA *NuCameraGetCam(void) {
    return &global_camera;
}

NUMTX *NuCameraGetMtx(void) {
    return &global_camera.mtx;
}

void NuMtxCalcCheapFaceY_v2(NUMTX *m, NUVEC *v) {
    NUMTX *camera = NuCameraGetMtx();
    NuVecCross(NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(camera, 2), v);
    NuVecNorm(NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(m, 0));
    *NUMTX_GET_ROW_VEC(m, 1) = *v;
    NuVecCross(NUMTX_GET_ROW_VEC(m, 2), NUMTX_GET_ROW_VEC(m, 0), NUMTX_GET_ROW_VEC(m, 1));
    m->m33 = 1.0f;
    m->m23 = 0.0f;
    m->m13 = 0.0f;
    m->m03 = 0.0f;
    m->m32 = 0.0f;
    m->m31 = 0.0f;
    m->m30 = 0.0f;
}

void NuMtxCalcFaceOn(NUMTX *m, NUVEC *v) {
    NUVEC world_up = {0.0f, 1.0f, 0.0f};
    NUVEC right;
    NUVEC up;
    NUVEC forward;
    NUVEC camera_position;

    NuMtxGetTranslation(NuCameraGetMtx(), &camera_position);
    NuVecSub(&forward, &camera_position, v);
    NuVecNorm(&forward, &forward);

    f32 projection = NuVecDot(&world_up, &forward);
    up.x = world_up.x - forward.x * projection;
    up.y = world_up.y - forward.y * projection;
    up.z = world_up.z - forward.z * projection;
    NuVecNorm(&up, &up);
    NuVecCross(&right, &up, &forward);

    m->m00 = right.x;
    m->m10 = up.x;
    m->m20 = forward.x;
    m->m01 = right.y;
    m->m11 = up.y;
    m->m21 = forward.y;
    m->m02 = right.z;
    m->m12 = up.z;
    m->m22 = forward.z;
    m->m33 = 1.0f;
    m->m23 = 0.0f;
    m->m13 = 0.0f;
    m->m03 = 0.0f;
    m->m30 = v->x;
    m->m31 = v->y;
    m->m32 = v->z;
}

void NuMtxCalcFaceY(NUMTX *m, NUVEC *v) {
    NUVEC world_up = {0.0f, 1.0f, 0.0f};
    NUVEC right;
    NUVEC up;
    NUVEC forward;
    NUVEC camera_position;

    NuMtxGetTranslation(NuCameraGetMtx(), &camera_position);
    NuVecSub(&forward, &camera_position, v);
    NuVecCross(&right, &world_up, &forward);
    NuVecNorm(&right, &right);
    NuVecCross(&up, &forward, &right);
    NuVecNorm(&up, &up);

    m->m00 = right.x;
    m->m10 = world_up.x;
    m->m20 = up.x;
    m->m01 = right.y;
    m->m11 = world_up.y;
    m->m21 = up.y;
    m->m02 = right.z;
    m->m12 = world_up.z;
    m->m22 = up.z;
    m->m33 = 1.0f;
    m->m23 = 0.0f;
    m->m13 = 0.0f;
    m->m03 = 0.0f;
    m->m30 = v->x;
    m->m31 = v->y;
    m->m32 = v->z;
}

void NuCameraGetClipMtx(NUMTX *viewport, NUMTX *scissor) {
    if (viewport != NULL) {
        memcpy(viewport, &vpc_vport_mtx, sizeof(NUMTX));
    }

    if (scissor != NULL) {
        memcpy(scissor, &vpc_sci_mtx, sizeof(NUMTX));
    }
}

NUMTX *NuCameraGetProjectionMtx(void) {
    return &pmtx;
}

NUMTX *NuCameraGetScalingMtx(void) {
    return &smtx;
}

NUMTX *NuCameraGetViewMtx(void) {
    return &vmtx;
}

void NuMtxCalcDebrisFaceOn(NUMTX *m) {
    NUMTX *view = NuCameraGetViewMtx();
    m->m00 = -view->m00;
    m->m10 = view->m01;
    m->m20 = -view->m02;
    m->m01 = -view->m10;
    m->m11 = view->m11;
    m->m21 = -view->m12;
    m->m02 = -view->m20;
    m->m12 = view->m21;
    m->m22 = -view->m22;
    m->m23 = 0.0f;
    m->m13 = 0.0f;
    m->m03 = 0.0f;
    m->m33 = 1.0f;
}

void NuMtxCalcCheapFaceY(NUMTX *m, NUVEC *v) {
    NUMTX *view = NuCameraGetViewMtx();
    NUVEC right;
    NUVEC up;

    right.x = -view->m00;
    right.y = 0.0f;
    right.z = -view->m20;
    NuVecNorm(&right, &right);

    m->m00 = right.x;
    m->m01 = right.y;
    m->m02 = right.z;

    up.x = 0.0f;
    up.y = 1.0f;
    up.z = 0.0f;
    m->m11 = up.y;
    m->m10 = up.x;
    m->m12 = up.z;

    NUVEC forward = {-view->m02, 0.0f, -view->m22};
    NuVecNorm(&forward, &forward);

    m->m20 = forward.x;
    m->m21 = forward.y;
    m->m22 = forward.z;
    m->m33 = 1.0f;
    m->m23 = 0.0f;
    m->m13 = 0.0f;
    m->m03 = 0.0f;
    m->m30 = v->x;
    m->m31 = v->y;
    m->m32 = v->z;
}

void NuMtxCalcCheapFaceOn(NUMTX *m, NUVEC *v) {
    NUMTX *view = NuCameraGetViewMtx();
    m->m00 = -view->m00;
    m->m10 = view->m01;
    m->m20 = -view->m02;
    m->m01 = -view->m10;
    m->m11 = view->m11;
    m->m21 = -view->m12;
    m->m02 = -view->m20;
    m->m12 = view->m21;
    m->m22 = -view->m22;
    m->m03 = 0.0f;
    m->m13 = 0.0f;
    m->m23 = 0.0f;
    m->m30 = v->x;
    m->m31 = v->y;
    m->m32 = v->z;
    m->m33 = 1.0f;
}

NUMTX *NuCameraGetVPMtx(void) {
    return &vpmtx;
}

NUMTX *NuCameraGetVPCMtx(void) {
    return &vpc_vport_mtx;
}

NUMTX *NuCameraGetClipPlanes(void) {
    return &clip_planes;
}

SAGA_HOST_WEAK i32 NuCameraClipTestSphere(NUVEC *pnt, f32 radius, NUMTX *world_mtx) {
    NUCAMERA *cam = NuCameraGetCam();
    NUMTX *view = NuCameraGetViewMtx();

    NUVEC pnt2;
    if (world_mtx != NULL) {
        NuVecMtxTransform(&pnt2, pnt, world_mtx);
        NuVecMtxTransform(&pnt2, &pnt2, view);
    } else {
        NuVecMtxTransform(&pnt2, pnt, view);
    }

    if (0.0f > (pnt2.z - cam->near_clip) + radius) {
        return 1;
    }
    if (0.0f > (cam->far_clip - pnt2.z) + radius) {
        return 1;
    }

    NUVEC4 g_dot;
    NuVec4MtxTransform(&g_dot, &pnt2, NuCameraGetClipPlanes());
    if (-radius > g_dot.x) {
        return 1;
    }
    if (-radius > g_dot.y) {
        return 1;
    }
    if (-radius > g_dot.z) {
        return 1;
    }
    if (-radius > g_dot.w) {
        return 1;
    }

    return 0;
}

void NuCameraGetPosition(NUVEC *v) {
    memcpy(v, &global_camera.mtx.m30, sizeof(NUVEC));
}

void NuCameraGetTrans(NUVEC *v) {
    memcpy(v, &global_camera.mtx.m30, sizeof(NUVEC));
}

void NuCameraCalcRay(float screen_x, float screen_y, NUVEC *ray_start, NUVEC *ray_end, NUCAMERA *cam) {
    NUVEC near_point;
    NUVEC far_point;

    if (cam == NULL) {
        cam = &global_camera;
    }

    float tan = NU_TAN_LUT(cam->fov * 0.5f * 10430.378f);
    near_point = far_point = {
        .x = (screen_x + screen_x - 1.0f) * tan / cam->aspect,
        .y = (1.0f - (screen_y + screen_y)) * tan,
        .z = 1.0f,
    };

    near_point.x *= cam->near_clip;
    near_point.y *= cam->near_clip;
    near_point.z *= cam->near_clip;
    far_point.x *= cam->far_clip;
    far_point.y *= cam->far_clip;
    far_point.z *= cam->far_clip;

    NuVecMtxTransform(ray_start, &near_point, &cam->mtx);
    NuVecMtxTransform(ray_end, &far_point, &cam->mtx);
}

void NuCameraRayCast(NUVEC *pnt, f32 x, f32 y) {
    pnt->x = x * zx;
    pnt->y = y * zy;
    pnt->z = 1.0f;

    NuVecMtxRotate(pnt, pnt, &global_camera.mtx);
}

void NuCameraBuildClipPlanes(void) {
    f32 near_dist;
    f32 near_dist_sq;

    f32 left_right;
    f32 top_bottom;

    f32 left_right_inv;
    f32 top_bottom_inv;

    NUMTX frustum_planes;
    NUMTX scissor_planes;

    f32 dot;

    near_dist = global_camera.near_clip;
    near_dist_sq = global_camera.near_clip * global_camera.near_clip;

    // Build the frustum planes.
    left_right = zx * near_dist;
    top_bottom = zy * near_dist;

    left_right_inv = 1.0f / NuFsqrt(left_right * left_right + near_dist_sq);
    top_bottom_inv = 1.0f / NuFsqrt(top_bottom * top_bottom + near_dist_sq);

    NuMtxSetZero(&frustum_planes);

    frustum_planes.m00 = -near_dist * left_right_inv;
    frustum_planes.m20 = left_right * left_right_inv;

    frustum_planes.m01 = -frustum_planes.m00;
    frustum_planes.m21 = frustum_planes.m20;

    frustum_planes.m12 = -near_dist * top_bottom_inv;
    frustum_planes.m22 = top_bottom * top_bottom_inv;

    frustum_planes.m13 = -frustum_planes.m12;
    frustum_planes.m23 = frustum_planes.m22;

    // Build the scissor planes.
    left_right = zxs * near_dist;
    top_bottom = zys * near_dist;

    left_right_inv = 1.0f / NuFsqrt(left_right * left_right + near_dist_sq);
    top_bottom_inv = 1.0f / NuFsqrt(top_bottom * top_bottom + near_dist_sq);

    NuMtxSetZero(&scissor_planes);

    scissor_planes.m00 = -near_dist * left_right_inv;
    scissor_planes.m20 = left_right * left_right_inv;

    scissor_planes.m01 = -scissor_planes.m00;
    scissor_planes.m21 = scissor_planes.m20;

    scissor_planes.m12 = -near_dist * top_bottom_inv;
    scissor_planes.m22 = top_bottom * top_bottom_inv;

    scissor_planes.m13 = -scissor_planes.m12;
    scissor_planes.m23 = scissor_planes.m22;

    NuMtxMulH(&ClipPlanes.frustum_planes, &vmtx, &frustum_planes);
    NuMtxMulH(&ClipPlanes.scissor_planes, &vmtx, &scissor_planes);

    dot = global_camera.mtx.m20 * global_camera.mtx.m30 + global_camera.mtx.m21 * global_camera.mtx.m31 +
          global_camera.mtx.m22 * global_camera.mtx.m32;

    memcpy(&ClipPlanes.near_plane, NUMTX_GET_ROW_VEC(&global_camera.mtx, 2), sizeof(NUVEC));
    ClipPlanes.near_plane.w = -dot - global_camera.near_clip;

    ClipPlanes.near_far_planes.m00 = -global_camera.mtx.m20;
    ClipPlanes.near_far_planes.m10 = -global_camera.mtx.m21;
    ClipPlanes.near_far_planes.m20 = -global_camera.mtx.m22;
    ClipPlanes.near_far_planes.m30 = dot + global_camera.far_clip;

    ClipPlanes.near_far_planes.m01 = ClipPlanes.near_plane.x;
    ClipPlanes.near_far_planes.m11 = ClipPlanes.near_plane.y;
    ClipPlanes.near_far_planes.m21 = ClipPlanes.near_plane.z;
    ClipPlanes.near_far_planes.m31 = ClipPlanes.near_plane.w;

    ClipPlanes.near_far_planes.m02 = NuFabs(ClipPlanes.near_far_planes.m00);
    ClipPlanes.near_far_planes.m12 = NuFabs(ClipPlanes.near_far_planes.m10);
    ClipPlanes.near_far_planes.m22 = NuFabs(ClipPlanes.near_far_planes.m20);
    ClipPlanes.near_far_planes.m32 = NuFabs(ClipPlanes.near_far_planes.m30);

    ClipPlanes.near_far_planes.m03 = NuFabs(ClipPlanes.near_far_planes.m01);
    ClipPlanes.near_far_planes.m13 = NuFabs(ClipPlanes.near_far_planes.m11);
    ClipPlanes.near_far_planes.m23 = NuFabs(ClipPlanes.near_far_planes.m21);
    ClipPlanes.near_far_planes.m33 = NuFabs(ClipPlanes.near_far_planes.m31);

    ClipPlanes.abs_frustum_planes.m00 = NuFabs(ClipPlanes.frustum_planes.m00);
    ClipPlanes.abs_frustum_planes.m01 = NuFabs(ClipPlanes.frustum_planes.m01);
    ClipPlanes.abs_frustum_planes.m02 = NuFabs(ClipPlanes.frustum_planes.m02);
    ClipPlanes.abs_frustum_planes.m03 = NuFabs(ClipPlanes.frustum_planes.m03);

    ClipPlanes.abs_frustum_planes.m10 = NuFabs(ClipPlanes.frustum_planes.m10);
    ClipPlanes.abs_frustum_planes.m11 = NuFabs(ClipPlanes.frustum_planes.m11);
    ClipPlanes.abs_frustum_planes.m12 = NuFabs(ClipPlanes.frustum_planes.m12);
    ClipPlanes.abs_frustum_planes.m13 = NuFabs(ClipPlanes.frustum_planes.m13);

    ClipPlanes.abs_frustum_planes.m20 = NuFabs(ClipPlanes.frustum_planes.m20);
    ClipPlanes.abs_frustum_planes.m21 = NuFabs(ClipPlanes.frustum_planes.m21);
    ClipPlanes.abs_frustum_planes.m22 = NuFabs(ClipPlanes.frustum_planes.m22);
    ClipPlanes.abs_frustum_planes.m23 = NuFabs(ClipPlanes.frustum_planes.m23);

    ClipPlanes.abs_frustum_planes.m30 = 0.0f;
    ClipPlanes.abs_frustum_planes.m31 = 0.0f;
    ClipPlanes.abs_frustum_planes.m32 = 0.0f;
    ClipPlanes.abs_frustum_planes.m33 = 0.0f;

    ClipPlanes.abs_scissor_planes.m00 = NuFabs(ClipPlanes.scissor_planes.m00);
    ClipPlanes.abs_scissor_planes.m01 = NuFabs(ClipPlanes.scissor_planes.m01);
    ClipPlanes.abs_scissor_planes.m02 = NuFabs(ClipPlanes.scissor_planes.m02);
    ClipPlanes.abs_scissor_planes.m03 = NuFabs(ClipPlanes.scissor_planes.m03);

    ClipPlanes.abs_scissor_planes.m10 = NuFabs(ClipPlanes.scissor_planes.m10);
    ClipPlanes.abs_scissor_planes.m11 = NuFabs(ClipPlanes.scissor_planes.m11);
    ClipPlanes.abs_scissor_planes.m12 = NuFabs(ClipPlanes.scissor_planes.m12);
    ClipPlanes.abs_scissor_planes.m13 = NuFabs(ClipPlanes.scissor_planes.m13);

    ClipPlanes.abs_scissor_planes.m20 = NuFabs(ClipPlanes.scissor_planes.m20);
    ClipPlanes.abs_scissor_planes.m21 = NuFabs(ClipPlanes.scissor_planes.m21);
    ClipPlanes.abs_scissor_planes.m22 = NuFabs(ClipPlanes.scissor_planes.m22);
    ClipPlanes.abs_scissor_planes.m23 = NuFabs(ClipPlanes.scissor_planes.m23);

    ClipPlanes.abs_scissor_planes.m30 = 0.0f;
    ClipPlanes.abs_scissor_planes.m31 = 0.0f;
    ClipPlanes.abs_scissor_planes.m32 = 0.0f;
    ClipPlanes.abs_scissor_planes.m33 = 0.0f;
}

extern "C" {
    i32 TreeInitialised;
    NUMTX CamSpaceFrustrumPlanes;
    NUMTX CamSpaceScissorPlanes;
    NUMTX FrustrumPlanes;
    NUMTX ScissorPlanes;
    NUVEC4 NearPlane;
    NUVEC4 AbsNearPlane;
    NUMTX AbsFrustrumPlanes;
    NUMTX AbsScissorPlanes;
}

extern "C" void BuildCamSpaceClipPlanes(void) {
    f32 near_dist = global_camera.near_clip;
    f32 near_sq = near_dist * near_dist;
    TreeInitialised = 1;
    f32 x = zx * near_dist;
    f32 y = zy * near_dist;
    f32 x_inv = 1.0f / NuFsqrt(x * x + near_sq);
    f32 y_inv = 1.0f / NuFsqrt(y * y + near_sq);
    NuMtxSetZero(&CamSpaceFrustrumPlanes);
    CamSpaceFrustrumPlanes.m00 = -near_dist * x_inv;
    CamSpaceFrustrumPlanes.m01 = -CamSpaceFrustrumPlanes.m00;
    CamSpaceFrustrumPlanes.m20 = -x * x_inv;
    CamSpaceFrustrumPlanes.m21 = CamSpaceFrustrumPlanes.m20;
    CamSpaceFrustrumPlanes.m22 = -y * y_inv;
    CamSpaceFrustrumPlanes.m23 = CamSpaceFrustrumPlanes.m22;
    CamSpaceFrustrumPlanes.m12 = near_dist * y_inv;
    CamSpaceFrustrumPlanes.m13 = -CamSpaceFrustrumPlanes.m12;
    x = zxs * near_dist;
    y = zys * near_dist;
    x_inv = 1.0f / NuFsqrt(x * x + near_sq);
    y_inv = 1.0f / NuFsqrt(y * y + near_sq);
    NuMtxSetZero(&CamSpaceScissorPlanes);
    CamSpaceScissorPlanes.m00 = -near_dist * x_inv;
    CamSpaceScissorPlanes.m01 = -CamSpaceScissorPlanes.m00;
    CamSpaceScissorPlanes.m20 = -x * x_inv;
    CamSpaceScissorPlanes.m21 = CamSpaceScissorPlanes.m20;
    CamSpaceScissorPlanes.m12 = near_dist * y_inv;
    CamSpaceScissorPlanes.m22 = -y * y_inv;
    CamSpaceScissorPlanes.m13 = -CamSpaceScissorPlanes.m12;
    CamSpaceScissorPlanes.m23 = CamSpaceScissorPlanes.m22;
}

extern "C" void BuildWorldSpaceClipPlanes(void) {
    NuMtxMulH(&FrustrumPlanes, &vmtx, &CamSpaceFrustrumPlanes);
    NuMtxMulH(&ScissorPlanes, &vmtx, &CamSpaceScissorPlanes);
    f32 dot = global_camera.mtx.m30 * global_camera.mtx.m20 + global_camera.mtx.m31 * global_camera.mtx.m21 +
              global_camera.mtx.m32 * global_camera.mtx.m22;
    NearPlane.x = global_camera.mtx.m20;
    NearPlane.y = global_camera.mtx.m21;
    NearPlane.z = global_camera.mtx.m22;
    NearPlane.w = -dot;
    AbsNearPlane.x = NuFabs(NearPlane.x);
    AbsNearPlane.y = NuFabs(NearPlane.y);
    AbsNearPlane.z = NuFabs(NearPlane.z);
    AbsNearPlane.w = 0;
    AbsFrustrumPlanes.m00 = NuFabs(FrustrumPlanes.m00);
    AbsFrustrumPlanes.m01 = NuFabs(FrustrumPlanes.m01);
    AbsFrustrumPlanes.m02 = NuFabs(FrustrumPlanes.m02);
    AbsFrustrumPlanes.m03 = NuFabs(FrustrumPlanes.m03);
    AbsFrustrumPlanes.m10 = NuFabs(FrustrumPlanes.m10);
    AbsFrustrumPlanes.m11 = NuFabs(FrustrumPlanes.m11);
    AbsFrustrumPlanes.m12 = NuFabs(FrustrumPlanes.m12);
    AbsFrustrumPlanes.m13 = NuFabs(FrustrumPlanes.m13);
    AbsFrustrumPlanes.m20 = NuFabs(FrustrumPlanes.m20);
    AbsFrustrumPlanes.m21 = NuFabs(FrustrumPlanes.m21);
    AbsFrustrumPlanes.m22 = NuFabs(FrustrumPlanes.m22);
    AbsFrustrumPlanes.m23 = NuFabs(FrustrumPlanes.m23);
    AbsFrustrumPlanes.m30 = AbsFrustrumPlanes.m31 = AbsFrustrumPlanes.m32 = AbsFrustrumPlanes.m33 = 0;
    AbsScissorPlanes.m00 = NuFabs(ScissorPlanes.m00);
    AbsScissorPlanes.m01 = NuFabs(ScissorPlanes.m01);
    AbsScissorPlanes.m02 = NuFabs(ScissorPlanes.m02);
    AbsScissorPlanes.m03 = NuFabs(ScissorPlanes.m03);
    AbsScissorPlanes.m10 = NuFabs(ScissorPlanes.m10);
    AbsScissorPlanes.m11 = NuFabs(ScissorPlanes.m11);
    AbsScissorPlanes.m12 = NuFabs(ScissorPlanes.m12);
    AbsScissorPlanes.m13 = NuFabs(ScissorPlanes.m13);
    AbsScissorPlanes.m20 = NuFabs(ScissorPlanes.m20);
    AbsScissorPlanes.m21 = NuFabs(ScissorPlanes.m21);
    AbsScissorPlanes.m22 = NuFabs(ScissorPlanes.m22);
    AbsScissorPlanes.m23 = NuFabs(ScissorPlanes.m23);
    AbsScissorPlanes.m30 = AbsScissorPlanes.m31 = AbsScissorPlanes.m32 = AbsScissorPlanes.m33 = 0;
}
