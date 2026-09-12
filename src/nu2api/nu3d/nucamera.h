#pragma once

#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"

typedef struct nucamera_s {
    NUMTX mtx;
    f32 fov;
    f32 aspect;
    f32 near_clip;
    f32 far_clip;

    f32 unknown_50;
    f32 unknown_54;
    f32 unknown_58;
    f32 unknown_5c;
    f32 unknown_60;
    f32 unknown_64;

    f32 portal_nearclip;

    NUVEC scale;
} NUCAMERA;

struct NuCameraReflect {
    u32 unknown_00;
    NUMTX mtx;
    u32 unknown_44[16];
};

typedef struct nuclipplanes_s {
    NUMTX frustum_planes;
    NUMTX scissor_planes;
    NUMTX near_far_planes;
    NUVEC4 near_plane;
    NUMTX abs_frustum_planes;
    NUMTX abs_scissor_planes;
} NUCLIPPLANES;

enum NUCAMERA_CLIP_FLAGS {
    NUCAMERA_CLIP_PROJECTION_VALID = 1 << 0,
};

enum NUCAMERA_EXTENTS_FLAGS {
    NUCAMERA_EXTENTS_SKIP_SCISSOR = 1 << 0,
    NUCAMERA_EXTENTS_SKIP_NEAR = 1 << 1,
};

enum NUCAMERA_OUTCODE {
    NUCAMERA_OUT_LEFT = 1 << 0,
    NUCAMERA_OUT_RIGHT = 1 << 1,
    NUCAMERA_OUT_TOP = 1 << 2,
    NUCAMERA_OUT_BOTTOM = 1 << 3,
    NUCAMERA_OUT_FAR = 1 << 4,
    NUCAMERA_OUT_NEAR = 1 << 5,
};

typedef struct nucameraclip_s {
    NUMTX clip;
    NUMTX projection;
    u8 flags;
    f32 far_clip;
    f32 near_clip;
    f32 x_scale;
    f32 y_scale;
} NUCAMERACLIP;

typedef struct nucamerastate_s {
    NUCAMERA camera;
    NUMTX view;
    NUMTX projection;
    NUMTX screen;
    NUMTX view_projection;
    NUMTX view_projection_scissor;
    NUMTX view_projection_viewport;
    NUMTX viewport;
    NUMTX view_projection_screen;
    NUMTX projection_screen;
    i32 effects;
    f32 zx, zy, zxs, zys;
    u32 unused_2cc;
    NUCLIPPLANES planes;
} NUCAMERASTATE;

#ifdef __cplusplus

void NuCameraBuildClipPlanes(void);

extern "C" {
    extern i32 cam_state_count;
    extern NUCAMERASTATE cam_state[16];
    i32 NuCameraSaveState(void);
    void NuCameraCalcClipMtx(NUCAMERACLIP *clip, NUCAMERA *camera, i32 use_cached_projection);
    void NuCameraClearStateBuffer(void);
    void NuCameraRestoreState(i32 handle);
    extern i32 camfx;
    extern NUVEC cam_axes;
    extern i32 force_camera_farclip;
    void NuCameraSetAxes(NUVEC *axes);
    void NuCameraGetAxes(NUVEC *axes);
    void NuCameraForceFarclip(i32 enabled);
    void NuCameraGetClippingRatios(f32 *horizontal, f32 *vertical);
    i32 NuCameraClipTestPointScissor(NUVEC *point);
    i32 NuCameraClipTestExtentsGeneric(NUVEC *min, NUVEC *max, NUMTX *world_mtx, f32 far_clip, i32 flags,
                                       f32 *min_depth);
    i32 NuCameraClipTestPointVport(NUVEC *point);
    void NuCameraTransformScreen(NUVEC *screen, NUVEC *world, i32 count, NUMTX *matrix);
    void NuCameraTransformScreenVU0(NUVEC4 *screen, NUVEC4 *world, i32 count, NUMTX *matrix);
    void NuCameraTransformView(NUVEC *view, NUVEC *world, i32 count, NUMTX *matrix);
    f32 NuCameraCalcAperture(f32 focal_length, f32 root_fstop);
    f32 NuCameraCalcRootFStop(f32 focal_length, f32 aperture);
    f32 NuCameraFOVToFocalLen(f32 fov);
    f32 NuCameraFocalLenToFOV(f32 focal_length);
#endif
    extern NUMTX clip_planes;
    extern NUCAMERA global_camera;

    extern NUMTX pmtx;
    extern NUMTX smtx;
    extern NUMTX vmtx;

    extern NUMTX vpmtx;

    extern NUMTX vpc_sci_mtx;
    extern NUMTX vpc_vport_mtx;
    extern NUMTX pc_vport_mtx;
    extern f32 nucamera_farclip_hack;

    extern NUCLIPPLANES ClipPlanes;

    // Separate legacy plane storage used by NuCameraIntersectsAABB.
    extern i32 TreeInitialised;
    extern NUMTX CamSpaceFrustrumPlanes, CamSpaceScissorPlanes;
    extern NUMTX FrustrumPlanes, ScissorPlanes;
    extern NUVEC4 NearPlane, AbsNearPlane;
    extern NUMTX AbsFrustrumPlanes, AbsScissorPlanes;
    void BuildCamSpaceClipPlanes(void);
    void BuildWorldSpaceClipPlanes(void);

    extern f32 zx;
    extern f32 zy;

    extern f32 zxs;
    extern f32 zys;

    NUCAMERA *NuCameraCreate(void);
    void NuCameraDestroy(NUCAMERA *cam);

    void NuCameraInitClipTestVU0();

    void NuCameraGet(NUCAMERA *out);

    void NuCameraSet(NUCAMERA *cam);
    void NuCameraSetEx(NUCAMERA *cam, i32 fast);
    void NuCameraSetVPortClipMtx(NUMTX *out, NUMTX *view, NUCAMERA *camera, i32 fast);
    void NuCameraSetScissorClipMtx(NUMTX *out, NUMTX *view, NUCAMERA *camera, i32 fast);
    void NuCameraSetProjectionMtx(NUMTX *mtx, f32 fov, f32 aspect, f32 near_clip, f32 far_clip);

    NUCAMERA *NuCameraGetCam(void);

    NUMTX *NuCameraGetMtx(void);
    void NuCameraGetClipMtx(NUMTX *viewport, NUMTX *scissor);
    NUMTX *NuCameraGetProjectionMtx(void);
    NUMTX *NuCameraGetScalingMtx(void);
    NUMTX *NuCameraGetClippingMtx(void);
    NUMTX *NuCameraGetPCMtx(void);
    NUMTX *NuCameraGetPCSMtx(void);
    NUMTX *NuCameraGetVPCSMtx(void);
    extern NUMTX cmtx;
    extern NUMTX psmtx;
    extern NUMTX vpsmtx;
    NUMTX *NuCameraGetViewMtx(void);
    NUMTX *NuCameraGetVPMtx(void);
    NUMTX *NuCameraGetVPCMtx(void);

    NUMTX *NuCameraGetClipPlanes(void);
    void NuCameraGetPosition(NUVEC *v);
    void NuCameraGetTrans(NUVEC *v);

    f32 NuCameraDist(NUVEC *v);
    f32 NuCameraDistSqr(NUVEC *v);

    i32 NuCameraClipTestExtents(NUVEC *min, NUVEC *max, NUMTX *world_mtx, f32 far_clip, i32 should_clip_to_screen);
    i32 NuCameraClipTestExtentsAxisAligned(NUVEC *center, NUVEC *extent, f32 far_clip);
    i32 NuCameraClipTestSphere(NUVEC *pnt, float radius, NUMTX *world_mtx);
    i32 NuCameraClipTestPoints(NUVEC *points, i32 count, NUMTX *world_mtx);

    void NuCameraCalcRay(float screen_x, float screen_y, NUVEC *ray_start, NUVEC *ray_end, NUCAMERA *cam);
    void NuCameraRayCast(NUVEC *pnt, f32 x, f32 y);
    void NuCameraTransformScreenClip(NUVEC *screen_points, NUVEC *world_points, i32 point_count, NUMTX *matrix);
#ifdef __cplusplus
}
#endif
