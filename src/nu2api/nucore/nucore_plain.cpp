// Nucore plain — C-linkage surface for the original libTTapp.so nucore TU.
#include "nu2api/nu3d/nulgtlaser.h"
#include "nu2api/nu3d/nuprim.h"
#include "nu2api/numath/nuvec4.h"
extern "C" {
    NULGTARCMATERIAL NuLgtArcMtl[4] = {};
    i32 NuLgtLaserOldCnt;
}
#include "nu2api/nucore/nuonline.h"
struct NUGCUTLOCATORFNENTRY_s;
extern "C" NUGCUTLOCATORFNENTRY_s *locatorfns;
//
// This file provides the C-callable export table that the original binary
// exposes from its single large nucore translation unit. Every symbol below
// is present in the ELF dynamic symbol table; the few with non-trivial
// bodies are faithful transcriptions (original addresses cited inline) while
// the remainder are pending-transcription stand-ins that preserve link
// compatibility until their real bodies land in a domain file.
//
// Faithfully transcribed in this TU:
//   NuDisplayListInit              @0x29ad60 — anchors the static 2D list
//   NuDisplayListLinkItems         @0x29ae31 — appends N items + NEXT term.
//   NuDisplayListLinkMtl           @0x2e8cc0 — minimal 2D-path mtl link
//   NuCameraSet                    via NuCameraSetEx(cam,0) (matrix work stubbed)
//   NuIOS_GetAspectRatio           inline ratio from nuapi screen dims
//   NuIOS_GetDeviceLanguage        @0xe3640  — exact locale ladder
//   NuFrameEnd                     @0x2e??? — frame pacing + swap + pad tick
// Transcribed elsewhere (stub removed here, comment left as breadcrumb):
//   Nu360_dxClear                  → nuposteffect_plain.cpp  @0x317070
//   NuDisplayListSwapBuffers*      → nudlist.cpp             @0x2eb5d0/0x2eaef0
//   NuHtmlBegin                    → legoapi/misc/supportall @0x2d5ca0
//   NuIOS_SetVertexFormat          → nuiosdl_gl.cpp          @0x29c070
//   NuIOS_Wait/WakeRenderThread    → ios_graphics.cpp        @0xe34b0/0xe3590
//   NuRenderContextSetZFunc        → nuiosdl_gl.cpp          @0x2a3860
//
// The remaining ~350 symbols are grouped by subsystem below so a reader can
// tell at a glance which domain still needs decompilation. Each group is
// alphabetical; every stub is an empty body that matches the original linkage.

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/numem.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nuqfnt.h"

void NuErrorPrint(char *);
void NuDebugMsgPrint(char *);
extern i32 nuspecial_draw_state;
extern NUQFNT *system_qfont;
i32 UnicodeToIndexFast(VUCHARIDX *map, i32 count, u16 unicode);
void NuLgtArcLaserEx(i32 type, NUVEC *start, NUVEC *end, NUVEC *control, f32 width, f32 segment_length, f32 wobble,
                     f32 duration, i32 colour, i32 flags);

#include "decomp.h"
#include "java/java.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nucore/nugcutscene.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/nuptrblock.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nupostresources.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "nu2api/nu3d/nuvport.h"
#include "nu2api/nu3d/nuocclusion.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuscreen.hpp"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nucore/NuDynamicLight.h"
#include "nu2api/nucore/nuvuvec.hpp"
#include "nu2api/numath/numtx.h"
#include "globals.h"

struct ani3_animheader_s;
struct nuanimdatachunk_s;
nuanimdatachunk_s *NuAnimDataChunkCreate(i32 curve_set_count);

extern "C" {
    static i32 isBitCountTable;
    void buildBitCountTable(void) {
        for (u32 value = 0; value < 256; ++value) {
            BitCountTable[value] = 0;
            for (u32 bit = 0; bit < 8; ++bit) {
                if ((value >> bit) & 1)
                    ++BitCountTable[value];
            }
        }
        isBitCountTable = 1;
    }

    u8 CutSceneBoundingBoxTrackRoot = 0;
    i32 NuGCutAudioStream = 0;
    NUANIMBUFFEVALUATECB AnimBuffEvalCB = NULL;
    void **AnimBuffEvalData = NULL;
    i32 *AnimBuffEvalJoint = NULL;
    i32 nuspecial_shadowLightCount = 0;
    i32 nuspecial_shadowLightHaveClipOverrides = 0;
    void *nuspecial_shadowLight[4];
    i32 nuspecial_shadowLightClipOverride[4];
    GLuint g_colorRenderbuffer;
    GLuint g_depthRenderbuffer;
    extern i32 g_writingSaveCriticalSection;
}

extern "C" void ANI_FixUpAddrs(ani3_animheader_s *, isize, i32);
extern "C" void ANI_Ani3ExtractAllNodeCurves(ani3_animheader_s *, f32, f32 *, i32, char *);
extern "C" {
    i32 nuspecial_clip_state = -1;
}

namespace {
    struct NuPlainSpecialHandleLayout {
        NUGSCN *scene;
        void *special;
        void *display_special;
    };

    struct NuPlainLegacySpecialLayout {
        u8 pad_00[0x40];
        u8 *instance;
        char *name;
        u32 flags;
    };

    struct NuPlainDisplaySpecialLayout {
        NUMTX mtx;
        NUMTX draw_mtx;
        NUVEC min;
        f32 min_w;
        NUVEC max;
        f32 max_w;
        NUVEC center;
        f32 radius;
        NUCLIPOBJECT *clip_objects;
        char *name;
        u32 flags;
        f32 *clip_range;
        i32 instance_ix;
        nuinstanim_s *instance_animation;
        i16 wind_speed;
        i16 wind_scale;
        u32 pad_cc;
    };

    struct NuPlainLegacySceneLayout {
        u8 pad_00[0x18];
        void **objects;
        i32 instance_count;
        u8 *instances;
    };

    struct NuPlainLegacyInstanceBoundsLayout {
        u8 pad_00[0x40];
        i16 object_index;
    };

    struct NuPlainLegacyMaterialLink {
        NuPlainLegacyMaterialLink *next;
        NUMTL *material;
    };

    struct NuPlainLegacyObjectBoundsLayout {
        NuPlainLegacyMaterialLink *materials;
        f32 origin_radius;
        u8 pad_08[4];
        NUVEC minimum;
        NUVEC maximum;
        NUVEC center;
        f32 radius;
        u8 pad_34[4];
        NuPlainLegacyObjectBoundsLayout *next;
    };

} // namespace

static i32 NuTimeBar_EngineEnabled;
static i32 NuTimeBar_GpuFrameOutEnabled;
static i32 clip_special_objects = 1;

using NUHGOBJVIDEOMEMFN = void (*)(nuhgobj_s *);

NUHGOBJVIDEOMEMFN hgobj_to_video_mem;
NUHGOBJVIDEOMEMFN video_mem_to_hgobj;

extern "C" {
    i32 nuspecial_const_tint_enabled;
    NUCOLOUR3 nuspecial_const_tint = {1.0f, 1.0f, 1.0f};
    i32 nuspecial_const_alpha_enabled;
    f32 nuspecial_const_alpha = 1.0f;
    i32 nuspecial_reflection;
    extern NUGLOBALRNDRSTATE render_state;
    void RndrStateSetConstAlphaTint(i32 alpha_enabled, i32 tint_enabled, f32 alpha, const NUCOLOUR3 *tint, NUMTL *mtl);
}

// C++-linkage helpers defined in sibling TUs.
void DisplayListCreateDynMtlList(VARIPTR *buf, VARIPTR buf_end); // supportall.cpp
void NuPadRecordEndFrame(void);                                  // nupad_interface.cpp
void bgSuspendMain(i32);                                         // main.cpp
void NuAnimBuffInit(i32, VARIPTR *, VARIPTR);                    // nu2api_nucore_misc.cpp
i32 NuCameraClipHGobj(nugscn_s *, NUMTX *, NUMTX *);             // nu2api_nucore_misc.cpp
i32 findrange(nugscn_s *, i32);                                  // utilities.cpp

extern "C" {

    // ---------------------------------------------------------------------------
    // Display-list bootstrap (original nucore TU file-statics)
    // ---------------------------------------------------------------------------

    i32 NuHasError(void);
    void NuMtlAnimate(f32 frame_time);
    void NuTexAnimProcess(f32 frame_time);
    void NuWindAnimate(NUWIND *wind, f32 frametime);
    void NuTimeBarSetRender(i32 set);
    void NuShaderManagerSetfv(i32 semantic, const f32 *values);
    void *NuScratchAlloc32(i32 size);
    void NuScratchRelease(void);

    extern VARIPTR *display_list_buffer_end;
    extern VARIPTR rndrstream_free;
    extern VARIPTR rndrstream_end;

    i32 NuThreadCreateCriticalSection(void);

    // Shared with the nudlist TU (original file-static in this TU).
    static void nudlist_SetNext(nudisplaylistitem_s *item, void *next) {
        item->next = next;
    }

    void NuDisplayListResetBuffer(void);

    // The static 2D display list's stream-area base lives at manager+0x4C8
    // (nudisplaylist_s+0x10 of the embedded 2D list at manager+0x4B8) and points
    // at manager+0x4FC.
    static const usize NUDLIST_2D_STREAM_BASE_OFFSET = 0x4C8;
    static const usize NUDLIST_2D_STREAM_AREA_OFFSET = 0x4FC;
    static const usize NUDLIST_2D_CRITSEC_OFFSET = 0x5EC;

    // original 0x29ad60
    void NuDisplayListInit(VARIPTR *buf, VARIPTR *buf_end) {
        u8 *mgr = (u8 *)&global_dlist_manager;
        // The static 2D list starts anchored on the stream-head sentinel.
        // DisplayListCreateDynMtlList initialises the sentinel and mtl_last.
        *(u8 **)(mgr + NUDLIST_2D_STREAM_BASE_OFFSET) = mgr + NUDLIST_2D_STREAM_AREA_OFFSET;

        DisplayListCreateDynMtlList(buf, *buf_end);
        NuDisplayListResetBuffer();

        *(i32 *)(mgr + NUDLIST_2D_CRITSEC_OFFSET) = NuThreadCreateCriticalSection();
    }

    // ---------------------------------------------------------------------------
    // Redirected symbols — real bodies live elsewhere (kept as comments)
    // ---------------------------------------------------------------------------

    // Nu360_dxClear(u32,u32) is transcribed in nuposteffect_plain.cpp (original 0x317070).
    // NuDisplayListSwapBuffersBeginFrame / EndFrame are transcribed in full
    // in nudlist.cpp (originals 0x2eb5d0 / 0x2eaef0).
    // NuHtmlBegin(void*) is transcribed in legoapi/misc/supportall.cpp (original 0x2d5ca0).
    // NuIOS_SetVertexFormat is transcribed in nuiosdl_gl.cpp (original 0x29c070).
    // NuIOS_WaitForRenderThreadCompletion is transcribed in ios_graphics.cpp (original 0xe34b0).
    // NuIOS_WakeRenderThread is transcribed in ios_graphics.cpp (original 0xe3590).
    // NuRenderContextSetZFunc is transcribed in nuiosdl_gl.cpp (original 0x2a3860).

    // ---------------------------------------------------------------------------
    // Camera (thin wrappers; full matrix work is in NuCameraSetEx)
    // ---------------------------------------------------------------------------

    f32 NuCameraCalcAperture(f32 focal_length, f32 root_fstop) {
        f32 fstop = root_fstop * root_fstop;
        if (fstop == 0.0f || focal_length == 0.0f)
            return 0.0f;
        return focal_length / fstop;
    }
    void NuCameraCalcClipMtx(NUCAMERACLIP *clip, NUCAMERA *camera, i32 use_cached_projection) {
        static NUMTX vmtx;
        if (use_cached_projection == 0 || (clip->flags & NUCAMERA_CLIP_PROJECTION_VALID) == 0) {
            clip->flags |= NUCAMERA_CLIP_PROJECTION_VALID;
            i32 angle = (i32)(0.5f * camera->fov * 10430.378f);
            f32 cotangent = NuTrigTable[(angle + 0x4000) >> 1 & 0x7fff] / NuTrigTable[angle >> 1 & 0x7fff];
            f32 far_clip = clip->far_clip;
            f32 near_clip = clip->near_clip;
            f32 x = clip->x_scale * camera->aspect * cotangent;
            f32 y = cotangent * clip->y_scale;
            memset(&clip->projection, 0, sizeof(clip->projection));
            clip->projection.m00 = x;
            clip->projection.m11 = y;
            clip->projection.m22 = 2.0f / (far_clip - near_clip);
            clip->projection.m23 = 1.0f;
            clip->projection.m32 = -2.0f * near_clip / (far_clip - near_clip);
        }
        NuMtxInvVU0(&vmtx, &camera->mtx);
        NuMtxMulVU0(&clip->clip, &vmtx, &clip->projection);
    }
    f32 NuCameraCalcRootFStop(f32 focal_length, f32 aperture) {
        f32 ratio;
        if (aperture == 0.0f || focal_length == 0.0f)
            ratio = 0.0f;
        else
            ratio = focal_length / aperture;
        return NuFsqrt(ratio);
    }
    void NuCameraClearStateBuffer(void) {
        cam_state_count = 0;
    }
    i32 NuCameraClipTestExtentsGeneric(NUVEC *min, NUVEC *max, NUMTX *world_mtx, f32 far_clip, i32 flags,
                                       f32 *min_depth) {
        NUMTX transform;
        NUVEC corners[8];
        NUVEC view[8];
        char results[8];
        if (far_clip == 0.0f)
            far_clip = global_camera.far_clip;
        if (world_mtx == NULL || world_mtx == &numtx_identity)
            transform = vmtx;
        else
            NuMtxMul(&transform, world_mtx, &vmtx);
        corners[0].x = min->x;
        corners[0].y = min->y;
        corners[0].z = min->z;
        corners[1].x = max->x;
        corners[1].y = max->y;
        corners[1].z = max->z;
        corners[2].x = min->x;
        corners[2].y = min->y;
        corners[2].z = max->z;
        corners[3].x = max->x;
        corners[3].y = max->y;
        corners[3].z = min->z;
        corners[4].x = min->x;
        corners[4].y = max->y;
        corners[4].z = min->z;
        corners[5].x = min->x;
        corners[5].y = max->y;
        corners[5].z = max->z;
        corners[6].x = max->x;
        corners[6].y = min->y;
        corners[6].z = min->z;
        corners[7].x = max->x;
        corners[7].y = min->y;
        corners[7].z = max->z;
        for (i32 i = 0; i < 8; ++i)
            NuVecMtxTransform(&view[i], &corners[i], &transform);
        memset(results, 0, sizeof(results));
        *min_depth = 3.4028234663852886e+38f;
        for (i32 i = 0; i < 8; ++i) {
            *min_depth = view[i].z < *min_depth ? view[i].z : *min_depth;
            if ((flags & NUCAMERA_EXTENTS_SKIP_NEAR) == 0 && view[i].z < global_camera.near_clip)
                results[i] |= NUCAMERA_OUT_NEAR;
            if (view[i].z > far_clip)
                results[i] |= NUCAMERA_OUT_FAR;
            if (view[i].x < -view[i].z * zx)
                results[i] |= NUCAMERA_OUT_LEFT;
            if (view[i].x > view[i].z * zx)
                results[i] |= NUCAMERA_OUT_RIGHT;
            if (view[i].y < -view[i].z * zy)
                results[i] |= NUCAMERA_OUT_BOTTOM;
            if (view[i].y > view[i].z * zy)
                results[i] |= NUCAMERA_OUT_TOP;
        }
        if ((results[0] & results[1] & results[2] & results[3] & results[4] & results[5] & results[6] & results[7]) !=
            0)
            return 0;
        if ((results[0] | results[1] | results[2] | results[3] | results[4] | results[5] | results[6] | results[7]) ==
            0)
            return 1;
        if ((flags & NUCAMERA_EXTENTS_SKIP_SCISSOR) != 0)
            return 2;
        memset(results, 0, sizeof(results));
        for (i32 i = 0; i < 8; ++i) {
            if ((flags & NUCAMERA_EXTENTS_SKIP_NEAR) == 0 && view[i].z < global_camera.near_clip)
                results[i] |= NUCAMERA_OUT_NEAR;
            if (view[i].z > far_clip)
                results[i] |= NUCAMERA_OUT_FAR;
            if (view[i].x < -view[i].z * zxs)
                results[i] |= NUCAMERA_OUT_LEFT;
            if (view[i].x > view[i].z * zxs)
                results[i] |= NUCAMERA_OUT_RIGHT;
            if (view[i].y < -view[i].z * zys)
                results[i] |= NUCAMERA_OUT_BOTTOM;
            if (view[i].y > view[i].z * zys)
                results[i] |= NUCAMERA_OUT_TOP;
        }
        if ((results[0] & results[1] & results[2] & results[3] & results[4] & results[5] & results[6] & results[7]) !=
            0)
            return 1;
        if ((results[0] | results[1] | results[2] | results[3] | results[4] | results[5] | results[6] | results[7]) ==
            0)
            return 1;
        return 2;
    }
    i32 NuCameraClipTestPointScissor(NUVEC *point) {
        return NuVecClipTestPointVU0(point, &vpc_sci_mtx) == 0;
    }
    i32 NuCameraClipTestPointVport(NUVEC *point) {
        return NuVecClipTestPointVU0(point, &vpc_vport_mtx) == 0;
    }
    i32 NuCameraClipTestPoints(NUVEC *points, i32 count, NUMTX *world_mtx) {
        enum CLIP_POINT_FLAGS {
            CLIP_POINT_LEFT = 0x01,
            CLIP_POINT_RIGHT = 0x02,
            CLIP_POINT_TOP = 0x04,
            CLIP_POINT_BOTTOM = 0x08,
            CLIP_POINT_FAR = 0x10,
            CLIP_POINT_NEAR = 0x20,
        };

        NUMTX clip_matrix;
        if (world_mtx != NULL) {
            NuMtxMulH(&clip_matrix, world_mtx, &vmtx);
        } else {
            clip_matrix = vmtx;
        }

        i32 common_flags = -1;
        for (i32 index = 0; index < count; ++index) {
            NUVEC transformed;
            NuVecMtxTransform(&transformed, &points[index], &clip_matrix);

            i32 flags = 0;
            if (transformed.z < global_camera.near_clip) {
                flags |= CLIP_POINT_NEAR;
            }
            if (transformed.z > global_camera.far_clip) {
                flags |= CLIP_POINT_FAR;
            }
            if (transformed.x < -transformed.z * zx) {
                flags |= CLIP_POINT_LEFT;
            }
            if (transformed.x > transformed.z * zx) {
                flags |= CLIP_POINT_RIGHT;
            }
            if (transformed.y < -transformed.z * zy) {
                flags |= CLIP_POINT_BOTTOM;
            }
            if (transformed.y > transformed.z * zy) {
                flags |= CLIP_POINT_TOP;
            }
            common_flags &= flags;
        }
        return common_flags;
    }
    f32 NuCameraFOVToFocalLen(f32 fov) {
        i32 angle = (i32)(0.5f * fov * 10430.378f);
        return 18.0f / (NuTrigTable[angle >> 1 & 0x7fff] / NuTrigTable[(angle + 0x4000) >> 1 & 0x7fff]);
    }
    f32 NuCameraFocalLenToFOV(f32 focal_length) {
        return NuAtan2DAF(18.0f / focal_length, 1.0f) * 0.0000958738019107841f * 2.0f;
    }
    void NuCameraForceFarclip(i32 enabled) {
        force_camera_farclip = enabled;
    }
    void NuCameraGetAxes(NUVEC *axes) {
        *axes = cam_axes;
    }
    NUMTX *NuCameraGetClippingMtx(void) {
        return &cmtx;
    }
    void NuCameraGetClippingRatios(f32 *horizontal, f32 *vertical) {
        *horizontal = zx;
        *vertical = zy;
    }
    NUMTX *NuCameraGetPCMtx(void) {
        return &pc_vport_mtx;
    }
    NUMTX *NuCameraGetPCSMtx(void) {
        return &psmtx;
    }
    NUMTX *NuCameraGetVPCSMtx(void) {
        return &vpsmtx;
    }
    void NuCameraIntersectsAABB(void) {
    }
    i32 prev_lock;
    NUCAMERA locked_camera;
    NUCAMERA cam_copy;
    i32 camfx;
    NuCameraReflect global_reflect;
    DECOMP_ASSERT(sizeof(NuCameraReflect) == 0x84, "camera reflection state size");
    DECOMP_ASSERT(__builtin_offsetof(NuCameraReflect, mtx) == 4, "camera reflection matrix offset");

    void NuCameraLock(i32 lock) {
        if (lock != prev_lock && prev_lock == 0) {
            locked_camera = *NuCameraGetCam();
        }
        if (lock != 0) {
            cam_copy = *NuCameraGetCam();
            NuCameraSet(&locked_camera);
        }
        prev_lock = lock;
    }
    extern nudisplayscene_s currentScene;
    i32 NuRndrDoingScreenGrab;
    i32 motionBlurAccumActiveThisFrame;

    void NuCameraMotionBlurEffect(const NUMTX *previous, const NUMTX *current, f32 scale, f32 maximum, f32 falloff) {
        currentScene.unknown_e4 = 1;
        currentScene.motion_previous = *previous;
        currentScene.motion_current = *current;
        currentScene.motion_scale = scale;
        currentScene.motion_maximum = maximum;
        currentScene.motion_falloff = falloff;
    }
    void NuCameraMotionBlurParams(const NUMTX *current) {
        currentScene.unknown_e0 = 1;
        currentScene.motion_current = *current;
    }
    void NuCameraRelock(void) {
        if (prev_lock == 1)
            NuCameraSet(&locked_camera);
    }
    i32 NuCameraSaveState(void) {
        if (cam_state_count >= 16)
            return 0;
        NUCAMERASTATE *state = &cam_state[cam_state_count];
        ++cam_state_count;
        state->camera = global_camera;
        state->view = vmtx;
        state->projection = pmtx;
        state->screen = smtx;
        state->view_projection = vpmtx;
        state->view_projection_scissor = vpc_sci_mtx;
        state->view_projection_viewport = vpc_vport_mtx;
        state->viewport = pc_vport_mtx;
        state->view_projection_screen = vpsmtx;
        state->projection_screen = psmtx;
        state->effects = camfx;
        state->zx = zx;
        state->zy = zy;
        state->zxs = zxs;
        state->zys = zys;
        state->planes = ClipPlanes;
        return cam_state_count;
    }
    void NuCameraSet(NUCAMERA *cam) {
        NuCameraSetEx(cam, 0);
    }
    void NuCameraSetAxes(NUVEC *axes) {
        cam_axes = *axes;
    }
    extern i32 PS2_REZ_W;
    extern i32 PS2_REZ_H;
    i32 PS2_SREZ_W = 4096;
    i32 PS2_SREZ_H = 4096;
    static i32 current_clip_scissor_to_viewport;
    void NuCameraSetEx(NUCAMERA *cam, i32 fast) {
        global_camera = *cam;
        FaceYDirStream(NuAtan2D(-global_camera.mtx.m20, -global_camera.mtx.m22));

        if (fast == 0) {
            NuVpUpdate();
        }

        if (camfx == 0) {
            NuMtxInv(&vmtx, &global_camera.mtx);
            NuMtxScale(&vmtx, &global_camera.scale);
        } else {
            NUMTX mirror;
            NUMTX camera_inverse;
            NUMTX reflect_inverse;
            NuMtxSetIdentity(&mirror);
            mirror.m11 = -1.0f;
            NuMtxInv(&camera_inverse, &global_camera.mtx);
            NuMtxInv(&reflect_inverse, &global_reflect.mtx);
            vmtx = reflect_inverse;
            if (camfx == 1)
                NuMtxMul(&vmtx, &vmtx, &mirror);
            NuMtxMul(&vmtx, &vmtx, &global_reflect.mtx);
            NuMtxMul(&vmtx, &vmtx, &camera_inverse);
        }

        if (fast == 0) {
            NuCameraSetProjectionMtx(&pmtx, global_camera.fov, global_camera.aspect, global_camera.near_clip,
                                     global_camera.far_clip);
            pmtx.m00 *= global_camera.unknown_58;
            pmtx.m11 *= global_camera.unknown_5c;
            pmtx.m20 += global_camera.unknown_50;
            pmtx.m21 += global_camera.unknown_54;
        }

        NuCameraSetVPortClipMtx(&vpc_vport_mtx, &vmtx, &global_camera, fast);
        if (current_clip_scissor_to_viewport != 0) {
            current_clip_scissor_to_viewport = 0;
            fast = 0;
        }
        NuCameraSetScissorClipMtx(&vpc_sci_mtx, &vmtx, &global_camera, fast);
        NuVpGetScalingMtx(&smtx);
        NuMtxMulH(&vpmtx, &vmtx, &pmtx);
        NuMtxMulH(&vpsmtx, &vpmtx, &smtx);
        NuMtxMulH(&psmtx, &pmtx, &smtx);

        if (fast == 0) {
            i32 angle = static_cast<i32>(global_camera.fov * 0.5f * 10430.378f);
            clip_planes.m22 = NuTrigTable[angle >> 1 & 0x7fff];
            clip_planes.m12 = NuTrigTable[(angle + 0x4000) >> 1 & 0x7fff];
            zy = clip_planes.m22 / clip_planes.m12;
            zx = zy / global_camera.aspect;

            // PS2_SREZ_W/H are 4096; PS2_REZ_W/H track the current render
            // dimensions. These ratios define the scissor frustum.
            zxs = static_cast<f32>(PS2_SREZ_W) * zx / static_cast<f32>(PS2_REZ_W);
            zys = static_cast<f32>(PS2_SREZ_H) * zy / static_cast<f32>(PS2_REZ_H);

            clip_planes.m13 = -clip_planes.m12;
            clip_planes.m02 = 0.0f;
            clip_planes.m03 = 0.0f;
            clip_planes.m32 = 0.0f;
            clip_planes.m33 = 0.0f;
            clip_planes.m23 = clip_planes.m22;

            f32 side = zy / cam->aspect;
            clip_planes.m01 = 1.0f / NuFsqrt(side * side + 1.0f);
            clip_planes.m00 = -clip_planes.m01;
            clip_planes.m10 = 0.0f;
            clip_planes.m11 = 0.0f;
            clip_planes.m20 = side * clip_planes.m01;
            clip_planes.m21 = clip_planes.m20;
            clip_planes.m30 = 0.0f;
            clip_planes.m31 = 0.0f;
            NuCameraBuildClipPlanes();
        }

        NuRndrSetViewMtx(&vpsmtx, &vpc_vport_mtx, &vpc_sci_mtx);
        NuRndrLightingStateCurrent.field_0x60 = 1;
        NuRndrLightingStateCurrent.field_0x74 = 0;
        NuRndrSetSpecularLightPS(NULL, NULL);
        NuRndrStateUpdateCameraState();
        if (NuOcclusionManagerIsInitialised())
            NuOcclusionManagerOnCameraSet();
    }
    void NuCameraSetReflect(NUCAMERA *camera, NuCameraReflect *reflect) {
        global_reflect = *reflect;
        NuCameraSet(camera);
        *reflect = global_reflect;
    }
    static void NuCameraBuildClipProjection(NUMTX *projection, NUCAMERA *camera, f32 x_scale, f32 y_scale) {
        f32 far_clip = camera->unknown_64;
        if (far_clip == 0.0f)
            far_clip = camera->far_clip;
        far_clip -= nucamera_farclip_hack;
        f32 near_clip = camera->unknown_60;
        if (near_clip == 0.0f)
            near_clip = camera->near_clip;
        const i32 angle = static_cast<i32>(camera->fov * 0.5f * 10430.378f);
        const f32 cotangent = NuTrigTable[(angle + 0x4000) >> 1 & 0x7fff] / NuTrigTable[angle >> 1 & 0x7fff];
        const f32 x = camera->aspect * cotangent * x_scale;
        const f32 y = cotangent * y_scale;
        memset(projection, 0, sizeof(*projection));
        projection->m00 = x;
        projection->m11 = y;
        projection->m23 = 1.0f;
        projection->m22 = (far_clip + near_clip) / (far_clip - near_clip);
        projection->m32 = (far_clip * -2.0f * near_clip) / (far_clip - near_clip);
    }
    void NuCameraSetScissorClipMtx(NUMTX *out, NUMTX *view, NUCAMERA *camera, i32 fast) {
        static NUMTX cmtx;
        if (fast == 0) {
            NUVIEWPORT *viewport = NuVpGetCurrentViewport();
            NuCameraBuildClipProjection(&cmtx, camera, viewport->scissor_width, viewport->scissor_height);
        }
        NuMtxMulVU0(out, view, &cmtx);
    }
    void NuCameraSetVPortClipMtx(NUMTX *out, NUMTX *view, NUCAMERA *camera, i32 fast) {
        if (fast == 0) {
            NUVIEWPORT *viewport = NuVpGetCurrentViewport();
            NuCameraBuildClipProjection(&pc_vport_mtx, camera, viewport->clip_width, viewport->clip_height);
        }
        NuMtxMulVU0(out, view, &pc_vport_mtx);
    }
    void NuCameraTransformScreen(NUVEC *screen, NUVEC *world, i32 count, NUMTX *matrix) {
        NUMTX transform;
        NUVEC *end = world + count;
        if (matrix == NULL)
            transform = vpsmtx;
        else
            NuMtxMulH(&transform, matrix, &vpsmtx);
        while (world < end)
            NuVecMtxTransformH(screen++, world++, &transform);
    }
    void NuCameraTransformScreenClip(NUVEC *screen, NUVEC *world, i32 count, NUMTX *matrix) {
        NUVEC *end = world + count;
        NUMTX transform;
        if (matrix == NULL)
            transform = vpc_vport_mtx;
        else
            NuMtxMulH(&transform, matrix, &vpc_vport_mtx);
        for (; world < end; ++world, ++screen)
            NuVecMtxTransformH(screen, world, &transform);
    }
    void NuCameraTransformScreenVU0(NUVEC4 *screen, NUVEC4 *world, i32 count, NUMTX *matrix) {
        NUMTX transform;
        NUVEC4 *end = world + count;
        if (matrix == NULL)
            transform = vpsmtx;
        else
            NuMtxMulVU0(&transform, matrix, &vpsmtx);
        while (world < end)
            NuVec4MtxTransformHVU0(screen++, world++, &transform);
    }
    void NuCameraTransformView(NUVEC *view, NUVEC *world, i32 count, NUMTX *matrix) {
        NUMTX transform;
        NUVEC *end = world + count;
        if (matrix == NULL)
            transform = vmtx;
        else
            NuMtxMul(&transform, matrix, &vmtx);
        while (world < end)
            NuVecMtxTransform(view++, world++, &transform);
    }
    void NuCameraUnlock(void) {
        if (prev_lock == 1)
            NuCameraSet(&cam_copy);
    }

    // ---------------------------------------------------------------------------
    // Display list — faithful helpers + pending stubs
    // ---------------------------------------------------------------------------

    void NuDisplayListBeginCriticalSection(void);
    void NuDisplayListEndCriticalSection(void);

    void NuDisplayListAnimateMtls(f32 frame_time) {
        static f32 sinetime = 0.0f;
        sinetime += frame_time;

        NuDisplayListBeginCriticalSection();
        for (NUMTLANIMSET *set = global_dlist_manager.mtlanim_list; set != NULL; set = set->next) {
            NUDLDLISTSCENE *scene = set->scene;
            for (i32 index = 0; index < set->material_count; ++index) {
                NUMTL *material = scene->mtls[set->material_indices[index]];
                if (material->disable_u_animation == 1 || material->disable_v_animation == 1) {
                    continue;
                }

                for (u32 layer = 0; layer < 4; ++layer) {
                    if (material->shader_desc.tex_anim_data[layer] == -1) {
                        continue;
                    }

                    NUTEXANIMDATA &animation = material->shader_desc.tex_anim_desc[layer];
                    f32 u = 0.0f;
                    f32 v = 0.0f;

                    switch (animation.u_mode) {
                        case NUTEXANIM_MODE_LINEAR:
                            material->shader_desc.tex_anim_offsets[layer][0] += frame_time * animation.u_frequency;
                            material->shader_desc.tex_anim_offsets[layer][0] -=
                                NuFloor(material->shader_desc.tex_anim_offsets[layer][0]);
                            u = material->shader_desc.tex_anim_offsets[layer][0];
                            break;
                        case NUTEXANIM_MODE_SINE:
                            u = NU_SIN_LUT(static_cast<i32>(animation.u_frequency * sinetime * 65536.0f)) *
                                animation.u_amplitude;
                            break;
                        case NUTEXANIM_MODE_COSINE:
                            u = NU_COS_LUT(static_cast<i32>(animation.u_frequency * sinetime * 65536.0f)) *
                                animation.u_amplitude;
                            break;
                        default:
                            break;
                    }

                    switch (animation.v_mode) {
                        case NUTEXANIM_MODE_LINEAR:
                            material->shader_desc.tex_anim_offsets[layer][1] += frame_time * animation.v_frequency;
                            material->shader_desc.tex_anim_offsets[layer][1] -=
                                NuFloor(material->shader_desc.tex_anim_offsets[layer][1]);
                            v = material->shader_desc.tex_anim_offsets[layer][1];
                            break;
                        case NUTEXANIM_MODE_SINE:
                            v = NU_SIN_LUT(static_cast<i32>(animation.v_frequency * sinetime * 65536.0f)) *
                                animation.v_amplitude;
                            break;
                        case NUTEXANIM_MODE_COSINE:
                            v = NU_COS_LUT(static_cast<i32>(animation.v_frequency * sinetime * 65536.0f)) *
                                animation.v_amplitude;
                            break;
                        default:
                            break;
                    }

                    NuMtlSetUVOffsetPS(material, layer, u, v);
                }
            }
        }
        NuDisplayListEndCriticalSection();
    }
    void NuDisplayListBeginCriticalSection(void) {
        NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);
    }
    void NuDisplayListClipSpecials(i32 enabled) {
        clip_special_objects = enabled;
    }
    void DisplayListCreateFxItemPS(void *item, i32 type);
    void DisplayListCreateFxList(VARIPTR *buffer, VARIPTR end, i32 count) {
        global_dlist_manager.max_fx = count;
        global_dlist_manager.fx_used = buffer->u8_ptr;
        buffer->addr += count;
        buffer->addr = ALIGN(buffer->addr, 4);
        global_dlist_manager.fx_sort_pris = (NUSORTPRI *)buffer->void_ptr;
        buffer->addr += count * sizeof(NUSORTPRI);
        buffer->addr = ALIGN(buffer->addr, 16);
        global_dlist_manager.fx_items = buffer->void_ptr;
        buffer->addr += count * 0x20;
        for (i32 i = 0; i < count; ++i) {
            global_dlist_manager.fx_used[i] = 0;
            NUDISPLAYLISTITEM *terminator = (NUDISPLAYLISTITEM *)global_dlist_manager.fx_items + i * 2 + 1;
            terminator->type = 0x84;
            terminator->next = NULL;
            terminator->id = 4;
        }
    }
    i32 NuDisplayListCreateFx(i32 type, i32 priority, i32 layer) {
        i32 index = -1;
        for (i32 i = 0; i < global_dlist_manager.max_fx; ++i) {
            if (!global_dlist_manager.fx_used[i]) {
                index = i;
                break;
            }
        }
        if (index == -1)
            return 0;
        NUDISPLAYLISTITEM *item = (NUDISPLAYLISTITEM *)((u8 *)global_dlist_manager.fx_items + index * 0x20);
        global_dlist_manager.fx_used[index] = 1;
        NUSORTPRI *record = &global_dlist_manager.fx_sort_pris[index];
        record->display_scene = NULL;
        record->sort_pri = priority + (layer << 17);
        record->items = item;
        record->field_18 = 0;
        record->nmtls = 0;
        NUSORTPRI *previous = NULL;
        NUSORTPRI *current = global_dlist_manager.sort_list;
        if (!current) {
            global_dlist_manager.sort_list = record;
            record->sys_next = NULL;
        } else {
            while (current && record->sort_pri > current->sort_pri) {
                previous = current;
                current = current->sys_next;
            }
            if (previous) {
                record->sys_next = current;
                previous->sys_next = record;
            } else {
                record->sys_next = global_dlist_manager.sort_list;
                global_dlist_manager.sort_list = record;
            }
        }
        ++global_dlist_manager.nused_sort_pris;
        DisplayListCreateFxItemPS(item, type);
        return index + 1;
    }
    // original 0x2f8bb0 — defer dynamic material display-list construction to
    // DisplayListLinkDynamicMtls at the next render-buffer swap.
    void NuDisplayListCreateMtl(NUMTL *mtl) {
        NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);
        if (global_dlist_manager.nnew_materials != 0x80) {
            global_dlist_manager.new_materials[global_dlist_manager.nnew_materials] = mtl;
            global_dlist_manager.nnew_materials++;
            NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
        }
    }
    void DisplayListDestroyFxItemPS(void *item);
    void NuDisplayListDestroyFx(i32 handle) {
        i32 index = handle - 1;
        global_dlist_manager.fx_used[index] = 0;
        NUSORTPRI *record = &global_dlist_manager.fx_sort_pris[index];
        NUSORTPRI *previous = global_dlist_manager.sort_list;
        if (record == previous) {
            global_dlist_manager.sort_list = record->sys_next;
        } else {
            while (previous->sys_next != record)
                previous = previous->sys_next;
            previous->sys_next = record->sys_next;
        }
        --global_dlist_manager.nused_sort_pris;
        DisplayListDestroyFxItemPS((u8 *)global_dlist_manager.fx_items + index * 0x20);
    }
    void NuDisplayListDestroyMtl(NUMTL *mtl) {
        NuThreadCriticalSectionBegin(global_dlist_manager.loading_critical_section);
        if (mtl->display_list != NULL && global_dlist_manager.dyn_mtl_dlist.nmtls != 0) {
            for (u32 i = 0; i < global_dlist_manager.dyn_mtl_dlist.nmtls; ++i) {
                if (global_dlist_manager.dyn_mtl_dlist.mtls[i] == mtl) {
                    if (i != static_cast<u32>(-1) && global_dlist_manager.ndel_materials != 0x80) {
                        global_dlist_manager.del_materials[global_dlist_manager.ndel_materials] = mtl;
                        ++global_dlist_manager.ndel_materials;
                    }
                    break;
                }
            }
        }
        NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
    }
    void NuDisplayListDrawAll(void) {
        NuDisplayListCaptureBegin();
        if (global_dlist_manager.nrender_scenes == 0)
            NuDisplayListAddRenderScene();
        for (i32 i = 0; i < global_dlist_manager.nrender_scenes; ++i)
            NuDisplayListDrawRenderScene(i);
        NuDisplayListDraw2D();
        NuDisplayListCaptureEnd();
    }
    void NuDisplayListEndCriticalSection(void) {
        NuThreadCriticalSectionEnd(global_dlist_manager.loading_critical_section);
    }
    VARIPTR *NuDisplayListLinkItemVP(nudisplaylist_s *list, u8 type, void *call_addr, VARIPTR *buf) {
        auto *call = reinterpret_cast<nudisplaylistitem_s *>(buf->void_ptr);
        list->mtl_last->next = call;
        call->type = type;
        call->id = 3;
        call->next = call_addr != nullptr ? call_addr : call + 2;

        auto *next = call + 1;
        next->type = 0x8d;
        next->id = 1;
        next->next = list->dyn_geom + 1;
        list->mtl_last = next;
        buf->addr += sizeof(nudisplaylistitem_s) * 2;
        return call_addr != nullptr ? nullptr : buf;
    }
    void NuDisplayListLinkItem(nudisplaylist_s *list, u8 type, void *call_addr) {
        NuDisplayListLinkItemVP(list, type, call_addr, NuDisplayListGetBuffer());
    }
    // Append `count` item slots from the shared stream buffer, then a NEXT
    // terminator. Original 0x29ae31.
    VARIPTR *NuDisplayListLinkItems(nudisplaylist_s *list, i32 count) {
        VARIPTR *buf = NuDisplayListGetBuffer();
        nudlist_SetNext(list->mtl_last, buf->void_ptr);
        list->items = reinterpret_cast<nudisplaylistitem_s *>(buf->void_ptr);
        buf->addr += count * sizeof(nudisplaylistitem_s);

        auto *terminator = reinterpret_cast<nudisplaylistitem_s *>(buf->void_ptr);
        terminator->type = 0x8d;
        terminator->id = 1;
        nudlist_SetNext(terminator, list->dyn_geom + 1);
        list->mtl_last = terminator;
        buf->addr += 0x10;
        return buf;
    }
} // extern "C"

// Local helpers matching original static display-list setters (t local symbols)

static __attribute__((used)) void NuDisplayListSetID(nudisplaylistitem_s *item, unsigned char id) {
    item->id = id;
}
static __attribute__((used)) void NuDisplayListAddItem(nudisplaylist_s *list, unsigned char id, void *item) {
    (void)list;
    (void)id;
    (void)item;
}
static __attribute__((used)) void NuDisplayListSetItem(nudisplaylistitem_s *item, unsigned char a, unsigned char b,
                                                       void *c) {
    (void)item;
    (void)a;
    (void)b;
    (void)c;
}
static __attribute__((used)) void NuDisplayListSetNext(nudisplaylistitem_s *item, void *next) {
    item->next = next;
}
static __attribute__((used)) void NuDisplayListSetID_CNT(nudisplaylistitem_s *item) {
    item->id = 0;
}
static __attribute__((used)) void NuDisplayListSetID_RET(nudisplaylistitem_s *item) {
    item->id = 4;
}
static __attribute__((used)) void NuDisplayListSetID_CALL(nudisplaylistitem_s *item) {
    (void)item;
}
static __attribute__((used)) void NuDisplayListSetID_NEXT(nudisplaylistitem_s *item) {
    item->id = 1;
}

extern "C" {
    // original 0x2e8cc0
    void NuDisplayListLinkMtl(nudisplaylist_s *list, NUMTL *mtl) {
        if (list->state->mtl == mtl) {
            return;
        }

        VARIPTR *buf = NuDisplayListGetBuffer();
        buf->addr = ALIGN(buf->addr, 0x10);
        list->mtl_last->next = buf->void_ptr;

        nudisplaylistitem_s *item;
        if (mtl->tex_id < 1 || mtl->tex_id == list->state->tex_id) {
            item = reinterpret_cast<nudisplaylistitem_s *>(buf->void_ptr);
            buf->addr += 0x40;
        } else {
            auto *bytes = reinterpret_cast<u8 *>(buf->void_ptr);
            buf->addr += 0x60;

            auto *nop = reinterpret_cast<nudisplaylistitem_s *>(bytes);
            nop[0].type = 0x87;
            nop[0].id = 0;
            nop[0].next = nullptr;
            nop[1].type = 0x87;
            nop[1].id = 0;
            nop[1].next = nullptr;
            item = nop + 2;
            list->state->tex_id = mtl->tex_id;
        }

        item[0].type = 0x80;
        item[0].id = 3;
        item[0].next = mtl;
        item[1].type = 0x87;
        item[1].id = 0;
        item[1].next = nullptr;
        item[2].type = 0x87;
        item[2].id = 0;
        item[2].next = nullptr;
        item[3].type = 0x8d;
        item[3].id = 1;
        item[3].next = nullptr;

        list->mtl_last = item + 3;
        list->state->mtl = mtl;
    }
    void NuDisplayListLinkList(NUDISPLAYLIST *list, NUDISPLAYLISTITEM *first, NUDISPLAYLISTITEM *last) {
        list->mtl_last->next = first;
        NUDISPLAYLISTITEM *continuation = list->dyn_geom + 1;
        last->type = 0x8d;
        last->next = continuation;
        last->id = 1;
        list->mtl_last = last;
    }
    void *NuDisplayListPrepareFaceonPS(VARIPTR *, void *faceon, NUMTX *) {
        return faceon;
    }
    void *DisplayListCreateFaceonTransformPS(VARIPTR *buffer, NUMTX *transform, NUMTL *mtl, void *faceon);
    void *DisplayListCreateGeomTransformPS(VARIPTR *buffer, NUMTX *transform, NUMTL *mtl, void *next, void *tx);

    // Original local helper cloned as DisplayListProcessSkin.isra.36 by GCC.
    // It appends the world transform, skin palette, and geometry calls as one
    // contiguous run in the material display list.
    static void DisplayListProcessSkin(NUMTL *, NUDISPLAYLIST *list, NUDISPLAYLISTITEM *geometry,
                                       NUDISPLAYLISTITEM **first_and_last, NUMTX *world_matrix, void **transform_packet,
                                       NUMTX *skin_matrices, DEFORMERWEIGHTSARRAY *deformer_weights,
                                       i32 shadow_caster) {
        display_list_buffer->addr = ALIGN(display_list_buffer->addr, 0x10);
        VARIPTR *buffer = NuDisplayListLinkItems(list, 3);

        *transform_packet = DisplayListCreateGeomTransformPS(buffer, world_matrix, list->dlist->mtls[list->mtl_id],
                                                             geometry->next, *transform_packet);
        NUDISPLAYLISTITEM *item = list->items;
        item->type = 0x8c;
        item->id = 3;
        item->next = *transform_packet;
        list->items = item + 1;
        first_and_last[0] = item;

        NUDISPLAYLISTGEOM *render_geometry;
        void *skin_packet =
            DisplayListCreateSkinTransformPS(buffer, skin_matrices, deformer_weights,
                                             static_cast<NUDISPLAYLISTGEOM *>(geometry->next), &render_geometry);
        item = list->items;
        item->type = 0x99;
        item->id = 3;
        item->next = skin_packet;
        list->items = item + 1;

        item = list->items;
        item->type = 0x98;
        item->id = 3;
        item->next = render_geometry;
        list->items = item + 1;
        first_and_last[1] = item;

        DisplayListSetAlphaPS(first_and_last[0], first_and_last[1], 1.0f);
        DisplayListSetShadowCasterFlagPS(first_and_last[0], first_and_last[1], shadow_caster);
    }

    // Original local helper cloned as DisplayListProcessLightmapped.isra.34.
    // Lightmapped geometry carries an AE/AF/B0 setup command two entries before
    // its geometry command. Keep that setup command adjacent to the dynamic
    // transform and geometry entries when the special is submitted.
    static void DisplayListProcessLightmapped(NUMTL *, NUDISPLAYLIST *list, NUDISPLAYLISTITEM *geometry,
                                              NUDISPLAYLISTITEM **first_and_last, NUMTX *world_matrix,
                                              void **transform_packet, f32 alpha) {
        display_list_buffer->addr = ALIGN(display_list_buffer->addr, 0x10);
        VARIPTR *buffer = NuDisplayListLinkItems(list, 3);

        NUDISPLAYLISTITEM *item = list->items;
        item->type = geometry[-2].type;
        item->id = 3;
        item->next = geometry[-2].next;
        list->items = item + 1;
        first_and_last[0] = item;

        *transform_packet = DisplayListCreateGeomTransformPS(buffer, world_matrix, list->dlist->mtls[list->mtl_id],
                                                             geometry->next, *transform_packet);
        item = list->items;
        item->type = 0x8c;
        item->id = 3;
        item->next = *transform_packet;
        list->items = item + 1;
        first_and_last[1] = item;

        item = list->items;
        item->type = 0x82;
        item->id = 3;
        item->next = geometry->next;
        list->items = item + 1;
        first_and_last[2] = item;

        DisplayListSetAlphaPS(first_and_last[1], first_and_last[2], alpha);
    }

    i32 NuDisplayListRndrSpecial(nuhspecial_s *special_handle, NUMTX *mtx, i32 skinned, NUMTX *skin_matrices,
                                 DEFORMERWEIGHTSARRAY *blend_values) {
        (void)skinned;

        if (special_handle == NULL || mtx == NULL) {
            return 0;
        }

        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special_handle);
        NuPlainDisplaySpecialLayout *special = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
        if (handle->scene == NULL || special == NULL) {
            return 0;
        }

        NUDLDLISTSCENE *scene = reinterpret_cast<NUDLDLISTSCENE *>(handle->scene->display_list);
        i32 clip_state = nuspecial_clip_state;
        if (clip_state == -1) {
            clip_state = NuCameraClipTestExtents(&special->min, &special->max, mtx, 0.0f, 0);
        }
        if (clip_state == 0) {
            return 0;
        }

        NUVEC center;
        center.x = (special->min.x + special->max.x) * 0.5f;
        center.y = (special->min.y + special->max.y) * 0.5f;
        center.z = (special->min.z + special->max.z) * 0.5f;
        NuVecMtxTransform(&center, &center, mtx);
        f32 distance_sqr = NuCameraDistSqr(&center);
        if (distance_sqr < 0.0f) {
            distance_sqr = 0.0f;
        }

        NUCLIPOBJECT *clip_object = special->clip_objects;
        if (special->clip_range != NULL && special->clip_range[0] != 0.0f) {
            i32 lod = 0;
            while (distance_sqr < special->clip_range[lod]) {
                ++lod;
            }
            clip_object += lod;
        }

        f32 distance_alpha = 1.0f;
        if (scene->fade_ranges != NULL && (scene->visibility_flags[special->instance_ix] & 0x40) != 0) {
            const f32 fade_start = scene->fade_ranges[special->instance_ix * 2];
            const f32 fade_end = scene->fade_ranges[special->instance_ix * 2 + 1];
            if (fade_end <= fade_start) {
                if (distance_sqr <= fade_end * fade_end) {
                    return 0;
                }
                distance_alpha = (NuFsqrt(distance_sqr) - fade_end) / (fade_start - fade_end);
                if (distance_alpha > 1.0f) {
                    distance_alpha = 1.0f;
                }
            } else if (fade_start * fade_start < distance_sqr) {
                distance_alpha = (fade_end - NuFsqrt(distance_sqr)) / (fade_end - fade_start);
                if (distance_alpha < 0.0f) {
                    distance_alpha = 0.0f;
                }
                if (distance_alpha == 0.0f) {
                    return 0;
                }
            }
        }

        void *transform_packet = NULL;
        for (u32 i = 0; i < *reinterpret_cast<u32 *>(clip_object); ++i) {
            u32 *material_indices = *reinterpret_cast<u32 **>(reinterpret_cast<u8 *>(clip_object) + 4);
            i32 *item_indices = *reinterpret_cast<i32 **>(reinterpret_cast<u8 *>(clip_object) + 8);
            u32 material_index = material_indices[i];
            NUDISPLAYLISTITEM *geometry = scene->items + item_indices[i];

            // A clip entry names the head of a material-variant chain. The
            // original submits every material linked through NUMTL::next.
            for (NUMTL *material = scene->mtls[material_index]; material != NULL; material = material->next) {
                NUDISPLAYLIST *list = material->display_list;
                if (list == NULL) {
                    continue;
                }

                scene->flags |= NUDL_SCENE_FLAG_CLIP_MATERIALS;
                const i32 used_material = list->mtl_id;
                u8 *used = scene->mtl_used[scene->render_buffer >> 7];
                used[used_material >> 3] |= static_cast<u8>(1U << (used_material & 7));

                RndrStateSetConstAlphaTint(nuspecial_const_alpha_enabled, nuspecial_const_tint_enabled,
                                           nuspecial_const_alpha, &nuspecial_const_tint, material);
                DisplayListUpdateRenderState(list, &render_state);

                if (geometry->type == 0x8f) {
                    VARIPTR *buffer = NuDisplayListLinkItems(list, 2);
                    NUDISPLAYLISTITEM *items = list->items;
                    items[0].type = 0x90;
                    items[0].id = 3;
                    items[0].next = DisplayListCreateFaceonTransformPS(buffer, mtx, material, geometry->next);
                    items[1].type = 0x8f;
                    items[1].id = 3;
                    items[1].next = NuDisplayListPrepareFaceonPS(buffer, geometry->next, mtx);
                    list->items = items + 2;
                    DisplayListSetAlphaPS(items, items + 1, distance_alpha);
                } else if (skin_matrices != NULL) {
                    NUDISPLAYLISTITEM *first_and_last[2];
                    i32 shadow_caster = 0;
                    if ((scene->visibility_flags[special->instance_ix] & 0x20) != 0 && nuspecial_reflection == 0) {
                        shadow_caster = 1;
                    }
                    DisplayListProcessSkin(material, list, geometry, first_and_last, mtx, &transform_packet,
                                           skin_matrices, blend_values, shadow_caster);
                } else {
                    const isize geometry_index = geometry - scene->items;
                    const bool has_lightmap_command =
                        geometry_index > 1 &&
                        (geometry[-2].type == 0xae || geometry[-2].type == 0xaf || geometry[-2].type == 0xb0);
                    if (static_cast<i32>(material->shader_desc.flags) < 0 && has_lightmap_command) {
                        NUDISPLAYLISTITEM *first_and_last[3];
                        DisplayListProcessLightmapped(material, list, geometry, first_and_last, mtx, &transform_packet,
                                                      distance_alpha);
                    } else {
                        VARIPTR *buffer = NuDisplayListLinkItems(list, 2);
                        NUDISPLAYLISTITEM *items = list->items;
                        items[0].type = 0x8c;
                        items[0].id = 3;
                        items[0].next = DisplayListCreateGeomTransformPS(buffer, mtx, material, geometry->next, NULL);
                        items[1].type = 0x82;
                        items[1].id = 3;
                        items[1].next = geometry->next;
                        list->items = items + 2;
                        DisplayListSetAlphaPS(items, items + 1, distance_alpha);
                    }
                }
            }
        }
        RndrStateSetConstAlphaTint(0, 0, 0.0f, NULL, NULL);
        return clip_state;
    }
    void DisplayListSetFxItemParamPS(void *item, i32 parameter, f32 value, i32 mode);
    void NuDisplayListSetFxParam(i32 handle, i32 parameter, f32 value, i32 mode) {
        void *item = (u8 *)global_dlist_manager.fx_items + (handle - 1) * 0x20;
        DisplayListSetFxItemParamPS(item, parameter, value, mode);
    }

    // ---------------------------------------------------------------------------
    // Scene / render-scene
    // ---------------------------------------------------------------------------

    void NuDisplaySceneDebug(void) {
    }

    // ---------------------------------------------------------------------------
    // Frame lifecycle (NuFrameEnd is faithful; neighbours are stubs)
    // ---------------------------------------------------------------------------

    // Per-frame animation/pad update hooks (originals take the frame time).
    void NuOcclusionManagerEndFrame(void);
    void NuPad_Interface_Render(void);
    void NuPadUpdatePads(void);

    // Frame-end callbacks (original bss @0x6bdaec/0x6bdaf0/0x6bdad0).
    extern void (*preRenderFlashingHack)(void);
    extern void (*postRenderFlashingHack)(void);
    extern void (*nuapi_endframe_callbackfn)(void);

    // Faithful transcription of the original frame-end pump. Waits for the
    // target frame interval, ticks material/tex/wind anims, swaps screens
    // via NuRndrSwapScreenEx, then advances nuapi clocks and pad state.

    // ---------------------------------------------------------------------------
    // iOS / platform
    // ---------------------------------------------------------------------------

    void NuIOS_AwardAchievement(void) {
    }
    void NuIOS_CheckCurrentFramebuffer(void) {
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            glCheckFramebufferStatus(GL_FRAMEBUFFER);
        }
    }
    void NuIOS_DeallocateSystemRenderbuffer(GLuint renderbuffer);
    void NuIOS_DeallocateSystemFramebuffers(void) {
        NuIOS_DeallocateSystemRenderbuffer(g_colorRenderbuffer);
        if (g_earlyColorFramebuffer != 0) {
            glDeleteFramebuffers(1, &g_earlyColorFramebuffer);
            g_earlyColorFramebuffer = 0;
        }
        if (g_earlyColorTexture != 0) {
            glDeleteTextures(1, &g_earlyColorTexture);
            g_earlyColorTexture = 0;
        }
        if (g_defaultFramebuffer != 0) {
            glDeleteFramebuffers(1, &g_defaultFramebuffer);
            g_defaultFramebuffer = 0;
        }
        if (g_colorRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &g_colorRenderbuffer);
            g_colorRenderbuffer = 0;
        }
        if (g_depthRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &g_depthRenderbuffer);
            g_depthRenderbuffer = 0;
        }
        g_currentFramebuffer = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    void NuIOS_DeallocateSystemRenderbuffer(GLuint) {
    }
    void NuIOS_FreeMemoryForSuspend(void) {
        const char *source_path = "i:/SagaTouch-Android_9176564/nu2api.saga/nuandroid/ios_graphics.cpp";
        BeginCriticalSectionGL(source_path, 270);
        NuIOS_DeallocateSystemFramebuffers();
        glReleaseShaderCompiler();
        glFinish();
        EndCriticalSectionGL(source_path, 279);
        NuThreadCriticalSectionBegin(g_performingBgProcWorkCritSec);
        NuThreadCriticalSectionBegin(g_writingSaveCriticalSection);
    }
    // Locale string filled by the platform layer (Java nativeSetLanguage on
    // device; LANG environment on host) and cached result index.
    char g_language[16] = {0}; // original bss @0x66f440
    i32 g_languageIndex = -1;  // original .data @0x616b80

    // original 0xe3640 — exact locale-matching ladder of the original.
    i32 NuIOS_GetDeviceLanguage(void) {
        if (g_languageIndex != -1) {
            return g_languageIndex;
        }
        auto matches = [&](const char *suffix, i32 n) {
            for (i32 k = 0; k < n; k++) {
                if (g_language[k] != suffix[k]) {
                    return false;
                }
            }
            return true;
        };
        if (matches("en-us", 5))
            return g_languageIndex = 0x12;
        if (matches("en-", 3))
            return g_languageIndex = 1;
        if (matches("fr-ca", 5))
            return g_languageIndex = 2;
        if (matches("fr-", 3))
            return g_languageIndex = 2;
        if (matches("it-", 3))
            return g_languageIndex = 5;
        if (matches("de-", 3))
            return g_languageIndex = 4;
        if (matches("es-mx", 5))
            return g_languageIndex = 3;
        if (matches("es-", 3))
            return g_languageIndex = 3;
        if (matches("ja-", 3))
            return g_languageIndex = 0;
        if (matches("ko-", 3))
            return g_languageIndex = 0xd;
        if (matches("nl-", 3))
            return g_languageIndex = 6;
        if (matches("pt-br", 5))
            return g_languageIndex = 0x10;
        if (matches("pt-", 3))
            return g_languageIndex = 7;
        if (strncmp(g_language, "zh-", 3) == 0)
            return g_languageIndex = 0x13;
        if (strncmp(g_language, "hu-", 3) == 0)
            return g_languageIndex = 1;
        if (strncmp(g_language, "ru-", 3) == 0)
            return g_languageIndex = 0xc;
        if (strncmp(g_language, "pl-", 3) == 0)
            return g_languageIndex = 0xb;
        if (strncmp(g_language, "cs-", 3) == 0)
            return g_languageIndex = 10;
        if (strncmp(g_language, "el-", 3) == 0)
            return g_languageIndex = 9;
        if (strncmp(g_language, "da-", 3) == 0)
            return g_languageIndex = 8;
        if (strncmp(g_language, "no-", 3) == 0)
            return g_languageIndex = 0xf;
        if (strncmp(g_language, "sv-", 3) == 0)
            return g_languageIndex = 0xe;
        if (strncmp(g_language, "fi-", 3) == 0)
            return g_languageIndex = 0x11;
        return -1;
    }
    i32 NuIOS_HardwareSupportsRetina(void) {
        return 1;
    }
    i32 NuIOS_IsLowestEndDevice(void) {
        return g_isLowestEndDevice;
    }
    i32 NuIOS_IsMidRangeDevice(void) {
        return 0;
    }
    i32 NuIOS_IsSmallScreen(void) {
        NuScreen *screen = NuScreen::Get();
        return screen->GetWidth() * screen->GetHeight() < 10000.0f;
    }
    void NuIOS_RecordFlurryEvent(char *event_name) {
        JNIEnv *env = NULL;
        if (g_javaVM->functions->GetEnv(g_javaVM, (void **)&env, JNI_VERSION_1_6) < 0) {
            return;
        }

        jclass activity_class = g_activityClass;
        jstring event = env->functions->NewStringUTF(env, event_name);
        jmethodID method =
            env->functions->GetStaticMethodID(env, activity_class, "FlurryEvent", "(Ljava/lang/String;)V");
        env->CallStaticVoidMethod(activity_class, method, event);
        env->functions->DeleteLocalRef(env, event);
    }
    void NuIOS_ShowAchievements(void) {
    }

    // ---------------------------------------------------------------------------
    // Animation / curves / data
    // ---------------------------------------------------------------------------

    // Original @0x2c6560.  The first contribution overwrites the buffer;
    // subsequent contributions use the blend player.
    void NuAnimBuffAccumulate_3(nuanimbuff_s *buffer, ani3_animheader_s *animation, f32 time, i32 overwrite, f32 blend,
                                i32 first_joint, nuhgobj_s *object, NUVEC *root_translation) {
        if (buffer == NULL) {
            buffer = static_cast<nuanimbuff_s *>(globalbuffer);
        }
        buffer->joint_count = object->joint_count;

        i32 end_joint = object->joint_count;
        if (first_joint != 0) {
            end_joint = findrange(reinterpret_cast<nugscn_s *>(object), first_joint) + 1;
        }
        if (overwrite != 0) {
            ANI_SimpleAni3PlayerV4Joint(animation, time - 1.0f, buffer, end_joint - first_joint, first_joint);
            if (root_translation != NULL) {
                root_translation->x = buffer->joints[0].translation.x;
                root_translation->y = buffer->joints[0].translation.y;
                root_translation->z = -buffer->joints[0].translation.z;
            }
        } else {
            ANI_SimpleAni3PlayerV4Joint_Blend(animation, time - 1.0f, buffer, blend, end_joint - first_joint,
                                              first_joint, root_translation);
        }
    }
    void *NuAnimBuffCreate(i32 max_joints, VARIPTR *buf) {
        u32 *anim_buffer = reinterpret_cast<u32 *>(ALIGN(buf->addr, 0x10));
        buf->void_ptr = anim_buffer + 4;
        anim_buffer[2] = buf->addr;
        buf->addr = ALIGN(buf->addr, 0x10) + max_joints * 0x30;
        anim_buffer[3] = buf->addr;
        buf->addr += max_joints;
        reinterpret_cast<u16 *>(anim_buffer)[2] = static_cast<u16>(max_joints);
        anim_buffer[0] = 0;
        return anim_buffer;
    }
    // Original @0x2bbf90.  Scratch buffers are nested allocations from the
    // engine's LIFO scratch arena, so destruction deliberately releases them
    // in the reverse order.
    void NuAnimBuffCreateScratch(nuanimbuff_s *buffer) {
        if (buffer == NULL) {
            return;
        }

        nuanimbuff_s *global = static_cast<nuanimbuff_s *>(globalbuffer);
        buffer->joint_count = 0;
        buffer->max_joints = global->max_joints;
        buffer->use_quaternions = 0;
        buffer->pad_07 = 0;
        buffer->joints = static_cast<nuanimbuffjoint_s *>(NuScratchAlloc32((buffer->max_joints * 3 + 3) * 0x10));
        buffer->joints = reinterpret_cast<nuanimbuffjoint_s *>(ALIGN(reinterpret_cast<usize>(buffer->joints), 0x10));
        buffer->joint_flags = static_cast<u8 *>(NuScratchAlloc32(buffer->max_joints));
    }
    // Original @0x2bc000.
    void NuAnimBuffDestroyScratch(nuanimbuff_s *buffer) {
        if (buffer == NULL) {
            return;
        }
        NuScratchRelease();
        NuScratchRelease();
        buffer->max_joints = 0;
        buffer->joints = NULL;
        buffer->joint_flags = NULL;
    }
    void NuAnimBuffEvaluateCallback(NUANIMBUFFEVALUATECB callback, void **data, i32 *joints) {
        AnimBuffEvalCB = callback;
        AnimBuffEvalData = data;
        AnimBuffEvalJoint = joints;
    }
    // Original @0x2bd180.  Evaluate the decompressed per-joint samples into
    // hierarchy matrices while carrying scale through the parent chain.
    void NuAnimBuffEvaluate_3(nuanimbuff_s *buffer, nuhgobj_s *object, NUMTX *matrices, ani3_animheader_s *animation,
                              NUHGOBJROOTFN root_fn, NUVEC *root_translation, void *root_data) {
        if (buffer == NULL) {
            buffer = static_cast<nuanimbuff_s *>(globalbuffer);
        }

        if (buffer->use_quaternions != 0) {
            NuAnimBuffEvaluate_3_QuatB(NULL, buffer, reinterpret_cast<nugscn_s *>(object), matrices, animation, root_fn,
                                       root_translation, root_data);
            return;
        }

        static NUVEC scale_array[256];

        void *callback_data[256];
        if (AnimBuffEvalData != NULL && AnimBuffEvalJoint != NULL) {
            memset(callback_data, 0, object->joint_count * sizeof(void *));
            for (i32 callback_index = 0; AnimBuffEvalData[callback_index] != NULL; ++callback_index) {
                const i32 override_index = AnimBuffEvalJoint[callback_index];
                if (override_index >= 0 && override_index < object->joint_override_map_count) {
                    const u8 joint_index = object->joint_override_map[override_index];
                    if (joint_index != 0xff) {
                        callback_data[joint_index] = AnimBuffEvalData[callback_index];
                    }
                }
            }
        }

        const i32 evaluated_count =
            animation->node_count < object->joint_count ? animation->node_count : object->joint_count;
        scale_array[0xff] = {1.0f, 1.0f, 1.0f};
        NUVEC root_values = {0.0f, 0.0f, 0.0f};
        for (i32 joint_index = 0; joint_index < evaluated_count; ++joint_index) {
            const nuanimbuffjoint_s &joint = buffer->joints[joint_index];
            const u8 flags = buffer->joint_flags[joint_index];
            const u8 parent_index = object->joints[joint_index].parent_index;

            NUMTX &local_matrix = matrices[joint_index];
            if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
                NUANGVEC angles = {
                    static_cast<NUANG>(joint.rotation.x * 10430.378f),
                    static_cast<NUANG>(joint.rotation.y * 10430.378f),
                    static_cast<NUANG>(joint.rotation.z * 10430.378f),
                };
                NuMtxSetRotateXYZVU0(&local_matrix, &angles);
                if ((flags & NUANIMBUFF_JOINT_BIND_MATRIX) != 0) {
                    NuMtxMulRVU0(&local_matrix, &local_matrix, &object->joints[joint_index].animation_bind_matrix);
                }
            } else if ((flags & NUANIMBUFF_JOINT_BIND_MATRIX) != 0) {
                local_matrix = object->joints[joint_index].animation_bind_matrix;
            } else {
                NuMtxSetIdentity(&local_matrix);
            }

            if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
                scale_array[joint_index] = joint.scale;
                NuMtxPreScaleVU0(&local_matrix, &scale_array[joint_index]);
                scale_array[joint_index].x *= scale_array[parent_index].x;
                scale_array[joint_index].y *= scale_array[parent_index].y;
                scale_array[joint_index].z *= scale_array[parent_index].z;
            } else {
                scale_array[joint_index] = scale_array[object->joints[joint_index].parent_index];
            }

            if ((flags & NUANIMBUFF_JOINT_CANCEL_PARENT_SCALE) != 0) {
                const NUVEC &parent_scale = scale_array[parent_index];
                NUVEC inverse_parent_scale;
                if (parent_scale.x != 0.0f && parent_scale.y != 0.0f && parent_scale.z != 0.0f) {
                    inverse_parent_scale = {1.0f / parent_scale.x, 1.0f / parent_scale.y, 1.0f / parent_scale.z};
                    scale_array[joint_index].x *= inverse_parent_scale.x;
                    scale_array[joint_index].y *= inverse_parent_scale.y;
                    scale_array[joint_index].z *= inverse_parent_scale.z;
                } else {
                    inverse_parent_scale = {0.0f, 0.0f, 0.0f};
                    scale_array[joint_index] = {0.0f, 0.0f, 0.0f};
                }
                NuMtxScaleVU0(&local_matrix, &inverse_parent_scale);
            }

            if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
                NUVEC translation = joint.translation;
                root_values = joint.translation;
                root_values.z = -root_values.z;
                NuMtxTranslate(&local_matrix, &translation);
            }

            local_matrix.m02 = -local_matrix.m02;
            local_matrix.m12 = -local_matrix.m12;
            local_matrix.m20 = -local_matrix.m20;
            local_matrix.m21 = -local_matrix.m21;
            local_matrix.m23 = -local_matrix.m23;
            local_matrix.m32 = -local_matrix.m32;

            if (root_fn != NULL) {
                root_fn(&local_matrix, root_data, &root_values, &root_values, root_translation, 0.0f);
            }

            if (parent_index != 0xff) {
                NuMtxMulVU0(&matrices[joint_index], &local_matrix, &matrices[parent_index]);
            }

            else if ((flags & 0x40) != 0) {
                scale_array[joint_index] = {1.0f, 1.0f, 1.0f};
            }
            if (AnimBuffEvalCB != NULL && callback_data[joint_index] != NULL) {
                AnimBuffEvalCB(&matrices[joint_index], callback_data[joint_index], 0);
            }
            root_fn = NULL;
        }

        for (i32 joint_index = evaluated_count; joint_index < object->joint_count; ++joint_index) {
            NuMtxSetIdentity(&matrices[joint_index]);
        }

        AnimBuffEvalCB = NULL;
        AnimBuffEvalData = NULL;
        AnimBuffEvalJoint = NULL;
    }
    // Original @0x2bd900. Apply the character's per-joint procedural offsets
    // to a decompressed ANI4/ANI5 pose before hierarchy evaluation.
    void NuAnimBuffProceduralAnimation(nuanimbuff_s *buffer, nuhgobj_s *object, i32 override_count,
                                       NUJOINTANIM_s *overrides) {
        if (buffer == NULL) {
            buffer = static_cast<nuanimbuff_s *>(globalbuffer);
        }

        constexpr f32 kRadiansToNuAngle = 10430.378f;
        constexpr f32 kNuAngleToRadians = 0.0000958738f;

        for (i32 override_index = 0; override_index < override_count; ++override_index) {
            NUJOINTANIM_s &joint_override = overrides[override_index];
            if (joint_override.joint_index >= object->joint_override_map_count) {
                continue;
            }

            const u8 joint_index = object->joint_override_map[joint_override.joint_index];
            if (joint_index == 0xff) {
                continue;
            }

            nuanimbuffjoint_s &joint = buffer->joints[joint_index];
            const u8 flags = joint_override.flags;
            if ((flags & NUJOINTANIM_ROTATION) != 0) {
                joint.rotation.x += joint_override.rotation.x;
                joint.rotation.y += joint_override.rotation.y;
                joint.rotation.z += joint_override.rotation.z;

                i32 angles[3] = {
                    static_cast<i32>(joint.rotation.x * kRadiansToNuAngle),
                    static_cast<i32>(joint.rotation.y * kRadiansToNuAngle),
                    static_cast<i32>(joint.rotation.z * kRadiansToNuAngle),
                };
                const u8 limit_flags[3] = {
                    NUJOINTANIM_LIMIT_ROTATION_X,
                    NUJOINTANIM_LIMIT_ROTATION_Y,
                    NUJOINTANIM_LIMIT_ROTATION_Z,
                };
                for (i32 axis = 0; axis < 3; ++axis) {
                    if ((flags & limit_flags[axis]) != 0) {
                        const u32 wrapped = static_cast<u32>(angles[axis]) & 0xffff;
                        angles[axis] =
                            wrapped >= 0x8000 ? static_cast<i32>(wrapped) - 0x10000 : static_cast<i32>(wrapped);
                        if (joint_override.rotation_limit_start[axis] >= angles[axis] &&
                            angles[axis] < joint_override.rotation_limit_end[axis]) {
                            angles[axis] = joint_override.rotation_limit_end[axis];
                        }
                    }
                }
                joint.rotation = {
                    static_cast<f32>(angles[0]) * kNuAngleToRadians,
                    static_cast<f32>(angles[1]) * kNuAngleToRadians,
                    static_cast<f32>(angles[2]) * kNuAngleToRadians,
                };
            }

            if ((flags & NUJOINTANIM_SCALE) != 0) {
                joint.scale.x += joint_override.scale.x;
                joint.scale.y += joint_override.scale.y;
                joint.scale.z += joint_override.scale.z;
            }
            if ((flags & NUJOINTANIM_TRANSLATION) != 0) {
                joint.translation.x += joint_override.translation.x;
                joint.translation.y += joint_override.translation.y;
                joint.translation.z += joint_override.translation.z;
            }
        }
    }
    f32 NuAnimCurve2CalcValEx(nuanimcurve2_s *curve, nuanimtime_s *time, u32 type) {
        nuanimcurvedata_s *data = curve->data.curvedata;
        u32 *key_mask = data->key_mask + time->chunk;
        if (type == 4) {
            i32 frame = static_cast<i32>(NuFloor(time->time_offset));
            return static_cast<f32>((*key_mask >> ((frame - 1) & 0x1f)) & 1);
        }

        u32 key = 0;
        switch (time->time_byte) {
            case 0:
                key = BitCountTable[reinterpret_cast<u8 *>(key_mask)[0] & time->time_mask];
                break;
            case 1:
                key = BitCountTable[reinterpret_cast<u8 *>(key_mask)[0]] +
                      BitCountTable[reinterpret_cast<u8 *>(key_mask)[1] & time->time_mask];
                break;
            case 2:
                key = BitCountTable[reinterpret_cast<u8 *>(key_mask)[0]] +
                      BitCountTable[reinterpret_cast<u8 *>(key_mask)[1]] +
                      BitCountTable[reinterpret_cast<u8 *>(key_mask)[2] & time->time_mask];
                break;
            case 3:
                key = BitCountTable[reinterpret_cast<u8 *>(key_mask)[0]] +
                      BitCountTable[reinterpret_cast<u8 *>(key_mask)[1]] +
                      BitCountTable[reinterpret_cast<u8 *>(key_mask)[2]] +
                      BitCountTable[reinterpret_cast<u8 *>(key_mask)[3] & time->time_mask];
                break;
        }
        u32 key_offset = data->key_offsets[time->chunk];
        u8 *key_data = static_cast<u8 *>(data->key_data);

        switch (type) {
            case 1: {
                f32 *first = reinterpret_cast<f32 *>(key_data + (key + key_offset - 1) * 0x10);
                f32 span = first[4] - first[0];
                f32 value_delta = first[2] - first[6];
                f32 t = (time->time - first[0]) * first[1];
                f32 tangent0 = first[3] * span;
                f32 tangent1 = first[7] * span;
                return (((((value_delta * 2.0f + tangent0 + tangent1) * t + value_delta * -3.0f) - tangent0 * 2.0f) -
                         tangent1) *
                            t +
                        tangent0) *
                           t +
                       first[2];
            }
            case 2: {
                f32 *header = reinterpret_cast<f32 *>(key_data);
                u8 *first = key_data + (key + key_offset + 1) * 4;
                f32 first_time = static_cast<f32>(static_cast<u32>(first[3]));
                f32 span = static_cast<f32>(static_cast<u32>(first[7])) - first_time;
                f32 inverse_span = span == 0.0f ? 0.0f : 1.0f / span;
                f32 tangent0 = static_cast<f32>(*reinterpret_cast<i8 *>(first + 2)) * header[0] * span;
                f32 tangent1 = static_cast<f32>(*reinterpret_cast<i8 *>(first + 6)) * header[0] * span;
                f32 value0 = static_cast<f32>(*reinterpret_cast<i16 *>(first)) * header[1];
                f32 value_delta = value0 - static_cast<f32>(*reinterpret_cast<i16 *>(first + 4)) * header[1];
                f32 t = ((time->time - 1.0f) - first_time) * inverse_span;
                return (((((value_delta * 2.0f + tangent0 + tangent1) * t - value_delta * 3.0f) - tangent0 * 2.0f) -
                         tangent1) *
                            t +
                        tangent0) *
                           t +
                       value0;
            }
            case 3:
                return *reinterpret_cast<f32 *>(key_data + (key + key_offset - 1) * 8);
            case 5: {
                f32 *header = reinterpret_cast<f32 *>(key_data);
                f32 header_0 = header[0];
                f32 header_1 = header[1];
                f32 header_2 = header[2];
                i16 *first = reinterpret_cast<i16 *>(key_data + (key + key_offset) * 6 + 0x0c);
                f32 span = static_cast<f32>(static_cast<u32>(static_cast<u16>(first[5]))) -
                           static_cast<f32>(static_cast<u32>(static_cast<u16>(first[2])));
                f32 tangent0 = static_cast<f32>(static_cast<i8>(first[1])) * header_0 * span;
                f32 tangent1 = static_cast<f32>(static_cast<i8>(first[4])) * header_0 * span;
                f32 value0 = static_cast<f32>(first[0]) * header_1 + header_2;
                f32 value_delta = value0 - (static_cast<f32>(first[3]) * header_1 + header_2);
                f32 inverse_span = 1.0f / span;
                f32 t = ((time->time - 1.0f) - static_cast<f32>(static_cast<u32>(static_cast<u16>(first[2])))) *
                        inverse_span;
                return (((((value_delta * 2.0f + tangent0 + tangent1) * t - value_delta * 3.0f) - tangent0 * 2.0f) -
                         tangent1) *
                            t +
                        tangent0) *
                           t +
                       value0;
            }
            case 6: {
                f32 *header = reinterpret_cast<f32 *>(key_data);
                f32 header_3 = header[3];
                f32 header_2 = header[2];
                f32 header_0 = header[0];
                f32 header_1 = header[1];
                u8 *first = key_data + (key + key_offset + 3) * 4;
                f32 first_time = static_cast<f32>(static_cast<u32>(first[3])) * header_3;
                f32 next_time = static_cast<f32>(static_cast<u32>(first[7])) * header_3;
                if (next_time == first_time) {
                    next_time = first_time + 1.0f;
                }
                f32 span = next_time - first_time;
                f32 tangent0 = static_cast<f32>(*reinterpret_cast<i8 *>(first + 2)) * header_0 * span;
                f32 tangent1 = static_cast<f32>(*reinterpret_cast<i8 *>(first + 6)) * header_0 * span;
                f32 value0 = static_cast<f32>(*reinterpret_cast<i16 *>(first)) * header_1 + header_2;
                f32 value_delta =
                    value0 - (static_cast<f32>(*reinterpret_cast<i16 *>(first + 4)) * header_1 + header_2);
                f32 inverse_span = 1.0f / span;
                f32 t = ((time->time - 1.0f) - first_time) * inverse_span;
                return (((((value_delta * 2.0f + tangent0 + tangent1) * t - value_delta * 3.0f) - tangent0 * 2.0f) -
                         tangent1) *
                            t +
                        tangent0) *
                           t +
                       value0;
            }
            default:
                return 0.0f;
        }
    }
    void NuAnimCurve2SetApplyToJointTransLoc(nuanimcurve2_s *curves, i8 *types, i8 flags, nuanimtime_s *time,
                                             nuhgobjjoint_s *joint, NUVEC *scale, NUVEC *parent_scale, NUMTX *matrix,
                                             NUJOINTANIM_s *override_anim, NUVEC *root_translation,
                                             NUVEC *locator_translation) {
        NUVEC translation, temporary;
        NUANGVEC angles;
        if (root_translation)
            root_translation->x = root_translation->y = root_translation->z = 0.0f;
        if (locator_translation)
            locator_translation->x = locator_translation->y = locator_translation->z = 0.0f;
        u8 override_flags, rotate, translate, rescale;
        if (override_anim) {
            override_flags = override_anim->flags;
            rotate = override_flags & 1;
            rescale = override_flags & 4;
            translate = override_flags & 2;
        } else
            override_flags = rotate = translate = rescale = 0;
        if ((flags & 1) || rotate) {
            NUVEC rotation = {0.0f, 0.0f, 0.0f};
            if (flags & 1) {
                rotation.x = types[3] ? NuAnimCurve2CalcValEx(curves + 3, time, types[3]) : curves[3].data.constant;
                rotation.y = types[4] ? NuAnimCurve2CalcValEx(curves + 4, time, types[4]) : curves[4].data.constant;
                rotation.z = types[5] ? NuAnimCurve2CalcValEx(curves + 5, time, types[5]) : curves[5].data.constant;
            }
            if (rotate) {
                rotation.x += override_anim->rotation.x;
                rotation.y += override_anim->rotation.y;
                rotation.z += override_anim->rotation.z;
            }
            angles.x = static_cast<i32>(rotation.x * 10430.3779296875f);
            angles.y = static_cast<i32>(rotation.y * 10430.3779296875f);
            angles.z = static_cast<i32>(rotation.z * 10430.3779296875f);
            if (rotate) {
                if (override_flags & 8) {
                    angles.x &= 0xffff;
                    if (angles.x > 32767)
                        angles.x -= 65536;
                    if (angles.x > override_anim->rotation_limit_start[0])
                        angles.x = override_anim->rotation_limit_start[0];
                    else if (angles.x < override_anim->rotation_limit_end[0])
                        angles.x = override_anim->rotation_limit_end[0];
                }
                if (override_flags & 16) {
                    angles.y &= 0xffff;
                    if (angles.y > 32767)
                        angles.y -= 65536;
                    if (angles.y > override_anim->rotation_limit_start[1])
                        angles.y = override_anim->rotation_limit_start[1];
                    else if (angles.y < override_anim->rotation_limit_end[1])
                        angles.y = override_anim->rotation_limit_end[1];
                }
                if (override_flags & 32) {
                    angles.z &= 0xffff;
                    if (angles.z > 32767)
                        angles.z -= 65536;
                    if (angles.z > override_anim->rotation_limit_start[2])
                        angles.z = override_anim->rotation_limit_start[2];
                    else if (angles.z < override_anim->rotation_limit_end[2])
                        angles.z = override_anim->rotation_limit_end[2];
                }
            }
            NuMtxSetRotateXYZVU0(matrix, &angles);
        } else
            NuMtxSetIdentity(matrix);
        if (flags & 0x20)
            NuMtxMulRVU0(matrix, matrix, &joint->animation_bind_matrix);
        if ((flags & 8) || rescale) {
            if (flags & 8) {
                scale->x = types[6] ? NuAnimCurve2CalcValEx(curves + 6, time, types[6]) : curves[6].data.constant;
                scale->y = types[7] ? NuAnimCurve2CalcValEx(curves + 7, time, types[7]) : curves[7].data.constant;
                scale->z = types[8] ? NuAnimCurve2CalcValEx(curves + 8, time, types[8]) : curves[8].data.constant;
            } else
                scale->x = scale->y = scale->z = 0.0f;
            if (rescale) {
                scale->x += override_anim->scale.x;
                scale->y += override_anim->scale.y;
                scale->z += override_anim->scale.z;
            }
            NuMtxPreScaleVU0(matrix, scale);
            scale->x *= parent_scale->x;
            scale->y *= parent_scale->y;
            scale->z *= parent_scale->z;
        } else
            *scale = *parent_scale;
        if ((flags & 0x10) && parent_scale) {
            if (parent_scale->x != 0.0f && parent_scale->y != 0.0f && parent_scale->z != 0.0f) {
                temporary.x = 1.0f / parent_scale->x;
                temporary.y = 1.0f / parent_scale->y;
                temporary.z = 1.0f / parent_scale->z;
            } else
                temporary.x = temporary.y = temporary.z = 0.0f;
            NuMtxScaleVU0(matrix, &temporary);
            scale->x *= temporary.x;
            scale->y *= temporary.y;
            scale->z *= temporary.z;
        }
        translation.x = types[0] ? NuAnimCurve2CalcValEx(curves, time, types[0]) : curves[0].data.constant;
        translation.y = types[1] ? NuAnimCurve2CalcValEx(curves + 1, time, types[1]) : curves[1].data.constant;
        translation.z = types[2] ? NuAnimCurve2CalcValEx(curves + 2, time, types[2]) : curves[2].data.constant;
        if (root_translation) {
            root_translation->x = translation.x;
            root_translation->y = translation.y;
            root_translation->z = -translation.z;
        }
        if (translate) {
            translation.x += override_anim->translation.x;
            translation.y += override_anim->translation.y;
            translation.z += override_anim->translation.z;
        }
        NuMtxTranslate(matrix, &translation);
        if (joint->data_0x51[0] & 8) {
            NUVEC *pivot = reinterpret_cast<NUVEC *>(joint->data_0x40);
            NuMtxPreTranslate(matrix, pivot);
            temporary.x = -pivot->x;
            temporary.y = -pivot->y;
            temporary.z = -pivot->z;
            NuMtxTranslate(matrix, &temporary);
            if (locator_translation) {
                NuVecMtxRotate(locator_translation, pivot, matrix);
                NuVecSub(locator_translation, locator_translation, pivot);
                locator_translation->z = -locator_translation->z;
            }
        }
        matrix->m02 = -matrix->m02;
        matrix->m12 = -matrix->m12;
        matrix->m20 = -matrix->m20;
        matrix->m21 = -matrix->m21;
        matrix->m23 = -matrix->m23;
        matrix->m32 = -matrix->m32;
    }
    void NuAnimCurve2SetApplyToJoint(nuanimcurve2_s *curves, i8 *types, i8 flags, nuanimtime_s *time,
                                     nuhgobjjoint_s *joint, NUVEC *scale, NUVEC *parent_scale, NUMTX *matrix,
                                     NUJOINTANIM_s *override_anim) {
        NuAnimCurve2SetApplyToJointTransLoc(curves, types, flags, time, joint, scale, parent_scale, matrix,
                                            override_anim, NULL, NULL);
    }
    void NuAnimCurve2SetApplyToMatrix_3(ani3_animheader_s *animation, i32 node, f32 frame, NUMTX *matrix) {
        u8 node_flags = animation->node_flags[node];
        f32 *values = NuAnimCurveExtractAllNodeCurves_3(animation, node, frame, NULL);

        if ((node_flags & NUANIM_NODE_HAS_ROTATION) != 0) {
            NUANGVEC rotation;
            rotation.x = static_cast<NUANG>(values[3] * 10430.378f);
            rotation.y = static_cast<NUANG>(values[4] * 10430.378f);
            rotation.z = static_cast<NUANG>(values[5] * 10430.378f);
            NuMtxSetRotateXYZVU0(matrix, &rotation);
        } else {
            NuMtxSetIdentity(matrix);
        }

        if ((node_flags & NUANIM_NODE_HAS_SCALE) != 0) {
            NuMtxPreScaleVU0(matrix, reinterpret_cast<NUVEC *>(&values[6]));
        }

        NuMtxTranslate(matrix, reinterpret_cast<NUVEC *>(values));

        // ANI3 animation data uses the opposite handedness for Z.
        matrix->m02 = -matrix->m02;
        matrix->m12 = -matrix->m12;
        matrix->m20 = -matrix->m20;
        matrix->m21 = -matrix->m21;
        matrix->m23 = -matrix->m23;
        matrix->m32 = -matrix->m32;
    }
    f32 NuAnimCurveCalcVal2(nuanimcurve_s *curve, nuanimtime_s *time) {
        i32 key = 0;
        switch (time->time_byte) {
            case 0:
                key = BitCountTable[curve->key_mask[0] & time->time_mask];
                break;
            case 1:
                key = BitCountTable[curve->key_mask[0]] + BitCountTable[curve->key_mask[1] & time->time_mask];
                break;
            case 2:
                key = BitCountTable[curve->key_mask[0]];
                key += BitCountTable[curve->key_mask[1]];
                key += BitCountTable[curve->key_mask[2] & time->time_mask];
                break;
            case 3:
                key = BitCountTable[curve->key_mask[0]];
                key += BitCountTable[curve->key_mask[1]];
                key += BitCountTable[curve->key_mask[2]];
                key += BitCountTable[curve->key_mask[3] & time->time_mask];
                break;
        }
        nuanimkey_s *first = &curve->keys[key - 1];
        if (curve->flags & 1) {
            if ((curve->flags & 2) && key <= curve->key_count && time->time - first->time > first[1].time - time->time)
                return first[1].value;
            return first->value;
        }
        f32 value = first->value;
        f32 span = first[1].time - first->time;
        f32 delta = value - first[1].value;
        f32 t = (time->time - first->time) * first->reciprocal_span;
        f32 tangent0 = first->tangent * span;
        f32 tangent1 = first[1].tangent * span;
        return (((((delta + delta + tangent0 + tangent1) * t + delta * -3.0f) - (tangent0 + tangent0) - tangent1) * t +
                 tangent0) *
                t) +
               value;
    }
    void *NuAnimCurveCreate(void) {
        return NULL;
    }
    void NuAnimCurveDestroy(void *curve) {
        void *keys = *reinterpret_cast<void **>(static_cast<u8 *>(curve) + 4);
        if (keys != NULL) {
            NU_FREE(keys);
        }
        NU_FREE(curve);
    }
    f32 *NuAnimCurveExtractAllNodeCurves_3(ani3_animheader_s *anim, i32 node, f32 frame, char *curve_mask) {
        f32 *values = *reinterpret_cast<f32 **>(reinterpret_cast<u8 *>(globalbuffer) + 8);
        ANI_Ani3ExtractAllNodeCurves(anim, frame - 1.0f, values, node, curve_mask);
        return values;
    }
    void NuAnimCurveSetApplyBlendToJoint2(nuanimcurveset_s *first, nuanimtime_s *first_time, nuanimcurveset_s *second,
                                          nuanimtime_s *second_time, f32 blend, nuhgobjjoint_s *joint, NUVEC *scale,
                                          NUVEC *parent_scale, NUMTX *matrix, NUJOINTANIM_s *override_anim) {
        f32 inverse_blend = 1.0f - blend;
        u8 flags = override_anim ? override_anim->flags : 0;
        u8 rotate = flags & 1, rescale = flags & 4, translate = flags & 2;
        NUANGVEC angles;
        NUVEC r0, r1, s0, s1, translation, temporary;
        if ((first->flags & 1) || (second->flags & 1) || rotate) {
            for (i32 component = 0; component < 3; ++component) {
                reinterpret_cast<f32 *>(&r0)[component] =
                    first->curves[component + 3] ? NuAnimCurveCalcVal2(first->curves[component + 3], first_time)
                                                 : first->constants[component + 3];
                reinterpret_cast<f32 *>(&r1)[component] =
                    second->curves[component + 3] ? NuAnimCurveCalcVal2(second->curves[component + 3], second_time)
                                                  : second->constants[component + 3];
            }
            r0.x -= r0.x - r1.x > 3.1415927410125732f ? 6.2831854820251465f : 0.0f;
            r1.x -= r1.x - r0.x > 3.1415927410125732f ? 6.2831854820251465f : 0.0f;
            r0.y -= r0.y - r1.y > 3.1415927410125732f ? 6.2831854820251465f : 0.0f;
            r1.y -= r1.y - r0.y > 3.1415927410125732f ? 6.2831854820251465f : 0.0f;
            r0.z -= r0.z - r1.z > 3.1415927410125732f ? 6.2831854820251465f : 0.0f;
            r1.z -= r1.z - r0.z > 3.1415927410125732f ? 6.2831854820251465f : 0.0f;
            r0.x = r0.x * inverse_blend + r1.x * blend;
            r0.y = r0.y * inverse_blend + r1.y * blend;
            r0.z = r0.z * inverse_blend + r1.z * blend;
            if (rotate) {
                angles.x = static_cast<i32>((r0.x + override_anim->rotation.x) * 10430.3779296875f);
                angles.y = static_cast<i32>((r0.y + override_anim->rotation.y) * 10430.3779296875f);
                angles.z = static_cast<i32>((r0.z + override_anim->rotation.z) * 10430.3779296875f);
                if (flags & 8) {
                    angles.x &= 0xffff;
                    if (angles.x > 32767)
                        angles.x -= 65536;
                    if (angles.x > override_anim->rotation_limit_start[0])
                        angles.x = override_anim->rotation_limit_start[0];
                    else if (angles.x < override_anim->rotation_limit_end[0])
                        angles.x = override_anim->rotation_limit_end[0];
                }
                if (flags & 16) {
                    angles.y &= 0xffff;
                    if (angles.y > 32767)
                        angles.y -= 65536;
                    if (angles.y > override_anim->rotation_limit_start[1])
                        angles.y = override_anim->rotation_limit_start[1];
                    else if (angles.y < override_anim->rotation_limit_end[1])
                        angles.y = override_anim->rotation_limit_end[1];
                }
                if (flags & 32) {
                    angles.z &= 0xffff;
                    if (angles.z > 32767)
                        angles.z -= 65536;
                    if (angles.z > override_anim->rotation_limit_start[2])
                        angles.z = override_anim->rotation_limit_start[2];
                    else if (angles.z < override_anim->rotation_limit_end[2])
                        angles.z = override_anim->rotation_limit_end[2];
                }
            } else {
                angles.x = static_cast<i32>(r0.x * 10430.3779296875f);
                angles.y = static_cast<i32>(r0.y * 10430.3779296875f);
                angles.z = static_cast<i32>(r0.z * 10430.3779296875f);
            }
            NuMtxSetRotateXYZVU0(matrix, &angles);
        } else
            NuMtxSetIdentity(matrix);
        if ((first->flags & 0x20) || (second->flags & 0x20))
            NuMtxMulRVU0(matrix, matrix, &joint->animation_bind_matrix);
        if ((first->flags & 8) || (second->flags & 8) || rescale) {
            for (i32 component = 0; component < 3; ++component) {
                reinterpret_cast<f32 *>(&s0)[component] =
                    first->curves[component + 6] ? NuAnimCurveCalcVal2(first->curves[component + 6], first_time)
                                                 : first->constants[component + 6];
                reinterpret_cast<f32 *>(&s1)[component] =
                    second->curves[component + 6] ? NuAnimCurveCalcVal2(second->curves[component + 6], second_time)
                                                  : second->constants[component + 6];
            }
            scale->x = blend * s0.x + inverse_blend * s1.x;
            scale->y = blend * s0.y + inverse_blend * s1.y;
            scale->z = blend * s0.z + inverse_blend * s1.z;
            if (rescale) {
                scale->x += override_anim->scale.x;
                scale->y += override_anim->scale.y;
                scale->z += override_anim->scale.z;
            }
            NuMtxPreScaleVU0(matrix, scale);
            scale->x *= parent_scale->x;
            scale->y *= parent_scale->y;
            scale->z *= parent_scale->z;
        } else
            *scale = *parent_scale;
        if ((first->flags | second->flags) & 0x10) {
            if (parent_scale->x != 0.0f && parent_scale->y != 0.0f && parent_scale->z != 0.0f) {
                temporary.x = 1.0f / parent_scale->x;
                temporary.y = 1.0f / parent_scale->y;
                temporary.z = 1.0f / parent_scale->z;
            } else
                temporary.x = temporary.y = temporary.z = 0.0f;
            NuMtxScaleVU0(matrix, &temporary);
            scale->x *= temporary.x;
            scale->y *= temporary.y;
            scale->z *= temporary.z;
        }
        for (i32 component = 0; component < 3; ++component) {
            reinterpret_cast<f32 *>(&translation)[component] =
                (first->curves[component] ? NuAnimCurveCalcVal2(first->curves[component], first_time)
                                          : first->constants[component]) *
                inverse_blend;
            reinterpret_cast<f32 *>(&translation)[component] +=
                (second->curves[component] ? NuAnimCurveCalcVal2(second->curves[component], second_time)
                                           : second->constants[component]) *
                blend;
        }
        if (translate) {
            translation.x += override_anim->translation.x;
            translation.y += override_anim->translation.y;
            translation.z += override_anim->translation.z;
        }
        NuMtxTranslate(matrix, &translation);
        if (joint->data_0x51[0] & 8) {
            NUVEC *pivot = reinterpret_cast<NUVEC *>(joint->data_0x40);
            NuMtxPreTranslate(matrix, pivot);
            temporary.x = -pivot->x;
            temporary.y = -pivot->y;
            temporary.z = -pivot->z;
            NuMtxTranslate(matrix, &temporary);
        }
        matrix->m02 = -matrix->m02;
        matrix->m12 = -matrix->m12;
        matrix->m20 = -matrix->m20;
        matrix->m21 = -matrix->m21;
        matrix->m23 = -matrix->m23;
        matrix->m32 = -matrix->m32;
    }
    void NuAnimCurveSetApplyToMatrix(nuanimcurveset_s *set, nuanimtime_s *time, NUMTX *matrix) {
        NUVEC rotation, translation, scale;
        NUANGVEC angles;
        if (set->flags & 1) {
            if (set->curves[3] != NULL) {
                rotation.x = NuAnimCurveCalcVal2(set->curves[3], time);
            } else {
                rotation.x = set->constants[3];
            }
            if (set->curves[4] != NULL) {
                rotation.y = NuAnimCurveCalcVal2(set->curves[4], time);
            } else {
                rotation.y = set->constants[4];
            }
            if (set->curves[5] != NULL) {
                rotation.z = NuAnimCurveCalcVal2(set->curves[5], time);
            } else {
                rotation.z = set->constants[5];
            }
            angles.x = static_cast<NUANG>(rotation.x * 10430.378f);
            angles.y = static_cast<NUANG>(rotation.y * 10430.378f);
            angles.z = static_cast<NUANG>(rotation.z * 10430.378f);
            NuMtxSetRotateXYZVU0(matrix, &angles);
        } else {
            NuMtxSetIdentity(matrix);
        }
        if (set->flags & 8) {
            if (set->curves[6] != NULL) {
                scale.x = NuAnimCurveCalcVal2(set->curves[6], time);
            } else {
                scale.x = set->constants[6];
            }
            if (set->curves[7] != NULL) {
                scale.y = NuAnimCurveCalcVal2(set->curves[7], time);
            } else {
                scale.y = set->constants[7];
            }
            if (set->curves[8] != NULL) {
                scale.z = NuAnimCurveCalcVal2(set->curves[8], time);
            } else {
                scale.z = set->constants[8];
            }
            NuMtxPreScaleVU0(matrix, &scale);
        }
        if (set->curves[0] != NULL) {
            translation.x = NuAnimCurveCalcVal2(set->curves[0], time);
        } else {
            translation.x = set->constants[0];
        }
        if (set->curves[1] != NULL) {
            translation.y = NuAnimCurveCalcVal2(set->curves[1], time);
        } else {
            translation.y = set->constants[1];
        }
        if (set->curves[2] != NULL) {
            translation.z = NuAnimCurveCalcVal2(set->curves[2], time);
        } else {
            translation.z = set->constants[2];
        }
        NuMtxTranslate(matrix, &translation);
        matrix->m02 = -matrix->m02;
        matrix->m12 = -matrix->m12;
        matrix->m20 = -matrix->m20;
        matrix->m21 = -matrix->m21;
        matrix->m23 = -matrix->m23;
        matrix->m32 = -matrix->m32;
    }
    void *NuAnimCurveSetCreate(i32 curve_count) {
        if (curve_count == 0) {
            return NULL;
        }

        u8 *curve_set = static_cast<u8 *>(NU_ALLOC(0x10, 4, 1, "", 0));
        memset(curve_set, 0, 0x10);
        curve_set[0xc] = static_cast<u8>(curve_count);

        const u32 array_size = static_cast<u32>(curve_count) * sizeof(void *);
        void **curves = static_cast<void **>(NU_ALLOC(array_size, 4, 1, "", 0));
        *reinterpret_cast<void ***>(curve_set + 8) = curves;
        memset(curves, 0, array_size);

        void *curve_flags = NU_ALLOC(array_size, 4, 1, "", 0);
        *reinterpret_cast<void **>(curve_set + 4) = curve_flags;
        memset(curve_flags, 0, array_size);
        return curve_set;
    }
    void NuAnimCurveSetDestroy(void *curve_set, i32 destroy_curves) {
        if (curve_set == NULL) {
            return;
        }

        u8 *set = static_cast<u8 *>(curve_set);
        void **curves = *reinterpret_cast<void ***>(set + 8);
        if (curves != NULL) {
            if (destroy_curves != 0) {
                const i32 count = *reinterpret_cast<i8 *>(set + 0xc);
                for (i32 index = 0; index < count; ++index) {
                    if (curves[index] != NULL) {
                        NuAnimCurveDestroy(curves[index]);
                    }
                }
            }
            NU_FREE(curves);
        }

        void *curve_flags = *reinterpret_cast<void **>(set + 4);
        if (curve_flags != NULL) {
            NU_FREE(curve_flags);
        }
        NU_FREE(curve_set);
    }
    void NuAnimData2CalcMatrix(nuanimdata_s *animation, i32 node, f32 frame, numtx_s *matrix) {
        NuAnimCurve2SetApplyToMatrix_3(reinterpret_cast<ani3_animheader_s *>(animation), node, frame, matrix);
    }
    void NuAnimData2CalcTime(nuanimdata2_s *anim, f32 frame, nuanimtime_s *time) {
        u32 magic = *reinterpret_cast<u32 *>(anim);
        if (magic + 0xbeb1b6ccU < 2) {
            ani3_animheader_s *ani3 = reinterpret_cast<ani3_animheader_s *>(anim);
            if (frame < 1.0f) {
                time->time = 1.0f;
            } else if (frame < static_cast<f32>(ani3->frame_count)) {
                time->time = frame;
            } else {
                time->time = static_cast<f32>(ani3->frame_count) - 0.01f;
            }
            return;
        }

        i32 chunk;
        f32 clamped_frame;
        if (anim->duration <= frame) {
            if (anim->duration == 1.0f) {
                time->time = 1.0f;
                time->chunk = 0;
                time->time_byte = 0;
                time->time_mask = 1;
                return;
            }
            clamped_frame = anim->duration - 0.01f;
            time->time = clamped_frame;
            chunk = (static_cast<i32>(clamped_frame) - 1) >> 5;
        } else if (frame < 1.0f) {
            clamped_frame = 1.0f;
            time->time = 1.0f;
            chunk = 0;
        } else {
            clamped_frame = frame;
            time->time = frame;
            chunk = (static_cast<i32>(frame) - 1) >> 5;
        }
        time->chunk = chunk;
        if (anim->chunk_count <= chunk) {
            chunk = anim->chunk_count - 1;
            time->chunk = chunk;
        }
        time->time_offset = clamped_frame - static_cast<f32>(chunk << 5);
        i32 chunk_frame = static_cast<i32>(NuFloor(time->time_offset)) - 1;
        time->chunk_frame = static_cast<u32>(chunk_frame);
        i32 byte_frame = chunk_frame < 0 ? chunk_frame + 7 : chunk_frame;
        time->time_byte = static_cast<u32>((byte_frame >> 3) & 0xff);
        time->time_mask = (1u << ((chunk_frame & 7) + 1)) - 1u;
    }
    void *NuAnimData2FixPtrs(void *data, isize delta, isize external_delta, i32 flags) {

        buildBitCountTable();

        if (data == NULL) {
            return NULL;
        }
        nuanimdata2_s *anim = reinterpret_cast<nuanimdata2_s *>(reinterpret_cast<usize>(data) + delta);
        if (*reinterpret_cast<u32 *>(&anim->duration) + 0xbeb1b6ccU < 2) {
            ANI_FixUpAddrs(reinterpret_cast<ani3_animheader_s *>(anim),
                           external_delta == 0 ? static_cast<isize>(reinterpret_cast<usize>(anim)) : delta, flags);
            return anim;
        }

        if (anim->curves != NULL) {
            anim->curves = reinterpret_cast<nuanimcurve2_s *>(reinterpret_cast<usize>(anim->curves) + delta);
        }
        if (anim->curve_types != NULL) {
            anim->curve_types = reinterpret_cast<u8 *>(reinterpret_cast<usize>(anim->curve_types) + delta);
        }
        if (anim->node_flags != NULL) {
            anim->node_flags = reinterpret_cast<u8 *>(reinterpret_cast<usize>(anim->node_flags) + delta);
        }
        i32 curve_count = static_cast<i32>(anim->curve_count) * static_cast<i32>(anim->node_count);
        for (i32 i = 0; i < curve_count; ++i) {
            if (anim->curve_types[i] != 0) {
                nuanimcurvedata_s *curve = anim->curves[i].data.curvedata;
                if (curve != NULL) {
                    curve = reinterpret_cast<nuanimcurvedata_s *>(reinterpret_cast<usize>(curve) + delta);
                    anim->curves[i].data.curvedata = curve;
                } else {
                    continue;
                }
                if (curve->key_mask != NULL) {
                    curve->key_mask = reinterpret_cast<u32 *>(reinterpret_cast<usize>(curve->key_mask) + delta);
                }
                if (curve->key_offsets != NULL) {
                    curve->key_offsets = reinterpret_cast<u16 *>(reinterpret_cast<usize>(curve->key_offsets) + delta);
                }
                if (curve->key_data != NULL) {
                    curve->key_data = reinterpret_cast<u8 *>(reinterpret_cast<usize>(curve->key_data) + delta);
                }
            }
        }
        return anim;
    }
    void *NuAnimData2Fixup(i32 file_size, void **data) {
        u32 *header = static_cast<u32 *>(*data);
        u32 magic = header[0];
        if (static_cast<i32>(header[1]) > static_cast<i32>(0x414e4934)) {
            return NuPtrBlockFix(header);
        }

        if (magic == 0x414e4933 || magic == 0x414e4934) {
            ANI_FixUpAddrs(reinterpret_cast<ani3_animheader_s *>(header),
                           static_cast<isize>(reinterpret_cast<usize>(header)), 0);
            return header;
        }

        header[0] = static_cast<u32>(file_size);
        const usize relocation_delta = reinterpret_cast<usize>(header) - static_cast<usize>(header[1]);
        header[2] = reinterpret_cast<usize>(
            NuAnimData2FixPtrs(reinterpret_cast<void *>(static_cast<usize>(header[2])), (isize)relocation_delta, 0, 0));
        header = static_cast<u32 *>(*data);
        header[1] = reinterpret_cast<usize>(header);
        return reinterpret_cast<void *>(static_cast<usize>(header[2]));
    }

    void *NuAnimData2LoadBuffEx(char *path, VARIPTR *buf, VARIPTR *buf_end, void **result) {
        buf->addr = ALIGN(buf->addr, 0x10);
        const i32 file_size = NuFileLoadBuffer(path, buf->void_ptr, static_cast<i32>(buf_end->addr - buf->addr));
        if (file_size == 0) {
            if (NuFileGetLastError() == -1) {
                *buf = *buf_end;
            }
            *result = NULL;
            return NULL;
        }

        void *data = buf->void_ptr;
        if (static_cast<i32>(static_cast<u32 *>(data)[1]) > static_cast<i32>(0x414e4934)) {
            data = NuPtrBlockFix(data);
            buf->void_ptr = data;
            buf->addr += static_cast<usize>(file_size);
            *result = data;
            return data;
        }

        *result = data;
        buf->addr += static_cast<usize>(file_size);
        return NuAnimData2Fixup(file_size, result);
    }

    void *NuAnimData2LoadBuff(char *path, VARIPTR *buf, VARIPTR *buf_end) {
        void *result;
        return NuAnimData2LoadBuffEx(path, buf, buf_end, &result);
    }

    void *NuAnimData2LoadBuffFromPAK(void *data, i32 file_size) {
        if (file_size == 0) {
            return NULL;
        }
        if (static_cast<i32>(static_cast<u32 *>(data)[1]) > static_cast<i32>(0x414e4934)) {
            return NuPtrBlockFix(data);
        }
        return NuAnimData2Fixup(file_size, &data);
    }
    void NuAnimData2Relocate(void) {
    }
    void NuAnimDataCalcTime(void *animation, f32 frame, nuanimtime_s *time) {
        u8 *data = static_cast<u8 *>(animation);
        const f32 duration = *reinterpret_cast<f32 *>(data);
        i32 chunk;
        f32 clamped_frame;
        if (duration <= frame) {
            if (duration == 1.0f) {
                time->time = 1.0f;
                time->chunk = 0;
                time->time_byte = 0;
                time->time_mask = 1;
                return;
            }
            clamped_frame = duration - 0.01f;
            time->time = clamped_frame;
            chunk = (static_cast<i32>(clamped_frame) - 1) >> 5;
        } else if (frame < 1.0f) {
            clamped_frame = 1.0f;
            time->time = 1.0f;
            chunk = 0;
        } else {
            clamped_frame = frame;
            time->time = frame;
            chunk = (static_cast<i32>(frame) - 1) >> 5;
        }

        time->chunk = chunk;
        const i32 chunk_count = *reinterpret_cast<i32 *>(data + 8);
        if (chunk_count <= chunk) {
            chunk = chunk_count - 1;
            time->chunk = chunk;
        }
        time->time_offset = clamped_frame - static_cast<f32>(chunk << 5);
        const i32 chunk_frame = static_cast<i32>(NuFloor(time->time_offset)) - 1;
        time->chunk_frame = static_cast<u32>(chunk_frame);
        time->time_byte = static_cast<u32>(((chunk_frame < 0 ? chunk_frame + 7 : chunk_frame) >> 3) & 0xff);
        time->time_mask = (1u << ((chunk_frame % 8) + 1)) - 1u;
    }
    void *NuAnimDataCreate(i32 chunk_count) {
        const u32 allocation_size = static_cast<u32>(chunk_count) * sizeof(void *) + 0x10;
        u8 *animation = static_cast<u8 *>(NU_ALLOC(allocation_size, 4, 1, "", 0));
        memset(animation, 0, allocation_size);
        *reinterpret_cast<i32 *>(animation + 8) = chunk_count;
        *reinterpret_cast<void ***>(animation + 0xc) = reinterpret_cast<void **>(animation + 0x10);
        return animation;
    }
    extern "C++" void NuAnimDataChunkDestroy(nuanimdatachunk_s *chunk);

    void NuAnimDataDestroy(void *animation) {
        u8 *data = static_cast<u8 *>(animation);
        const i32 chunk_count = *reinterpret_cast<i32 *>(data + 8);
        nuanimdatachunk_s **chunks = *reinterpret_cast<nuanimdatachunk_s ***>(data + 0xc);
        for (i32 index = 0; index < chunk_count; ++index) {
            NuAnimDataChunkDestroy(chunks[index]);
        }

        void *name = *reinterpret_cast<void **>(data + 4);
        if (name != NULL) {
            NU_FREE(name);
        }
        NU_FREE(animation);
    }
    i32 NuAnimDataFindVersion(char *path) {
        u32 header[3] = {};
        NUFILE file = NuFileOpen(path, NUFILE_READ);
        if (file == 0) {
            return -1;
        }
        NuFileRead(file, header, sizeof(header));
        NuFileClose(file);
        return static_cast<i32>(header[0]);
    }
    static inline void *NuLegacyRelocatePointer(void *pointer, isize delta) {
        return pointer == NULL ? NULL : reinterpret_cast<void *>(reinterpret_cast<usize>(pointer) + delta);
    }
    void *NuAnimDataFixPtrs(void *animation, isize delta) {

        if (isBitCountTable == 0)
            buildBitCountTable();
        animation = NuLegacyRelocatePointer(animation, delta);
        u8 *data = static_cast<u8 *>(animation);
        void *&name = *reinterpret_cast<void **>(data + 4);
        name = NuLegacyRelocatePointer(name, delta);
        void **&chunks = *reinterpret_cast<void ***>(data + 0xc);
        chunks = static_cast<void **>(NuLegacyRelocatePointer(chunks, delta));
        if (chunks != NULL) {
            i32 chunk_count = *reinterpret_cast<i32 *>(data + 8);
            for (i32 i = 0; i < chunk_count; ++i) {
                chunks[i] = NuLegacyRelocatePointer(chunks[i], delta);
                u8 *chunk = static_cast<u8 *>(chunks[i]);
                if (chunk != NULL) {
                    void **&sets = *reinterpret_cast<void ***>(chunk + 8);
                    sets = static_cast<void **>(NuLegacyRelocatePointer(sets, delta));
                    if (sets != NULL) {
                        i32 set_count = *reinterpret_cast<i32 *>(chunk);
                        for (i32 j = 0; j < set_count; ++j) {
                            sets[j] = NuLegacyRelocatePointer(sets[j], delta);
                            u8 *set = static_cast<u8 *>(sets[j]);
                            if (set != NULL) {
                                void *&constants = *reinterpret_cast<void **>(set + 4);
                                constants = NuLegacyRelocatePointer(constants, delta);
                                void **&curves = *reinterpret_cast<void ***>(set + 8);
                                curves = static_cast<void **>(NuLegacyRelocatePointer(curves, delta));
                                if (curves != NULL) {
                                    for (i32 k = 0; k < *reinterpret_cast<i8 *>(set + 0xc); ++k) {
                                        curves[k] = NuLegacyRelocatePointer(curves[k], delta);
                                        u8 *curve = static_cast<u8 *>(curves[k]);
                                        if (curve != NULL) {
                                            void *&keys = *reinterpret_cast<void **>(curve + 4);
                                            keys = NuLegacyRelocatePointer(keys, delta);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return animation;
    }
    void *NuAnimDataLoadBuff(char *path, VARIPTR *buf, VARIPTR *buf_end) {
        buf->addr = ALIGN(buf->addr, 16);
        u8 *file_data = static_cast<u8 *>(buf->void_ptr);
        const i32 file_size = NuFileLoadBuffer(path, file_data, static_cast<i32>(buf_end->addr - buf->addr));
        buf->addr += static_cast<usize>(file_size);

        const isize delta = reinterpret_cast<isize>(file_data) - *reinterpret_cast<isize *>(file_data + 4);
        *reinterpret_cast<isize *>(file_data + 4) = delta;
        void *animation = NuAnimDataFixPtrs(*reinterpret_cast<void **>(file_data + 8), delta);
        *reinterpret_cast<void **>(file_data + 8) = animation;
        return animation;
    }
    void *NuAnimDataRead(NUFILE file) {

        if (isBitCountTable == 0) {
            buildBitCountTable();
        }

        char *name = NULL;
        const i32 name_size = NuFileReadInt(file);
        if (name_size != 0) {
            name = static_cast<char *>(NU_ALLOC(name_size, 4, 1, "", 0));
            NuFileRead(file, name, name_size);
        }

        const f32 duration = NuFileReadFloat(file);
        const i32 chunk_count = NuFileReadInt(file);
        u8 *animation = static_cast<u8 *>(NuAnimDataCreate(chunk_count));
        *reinterpret_cast<f32 *>(animation) = duration;
        *reinterpret_cast<char **>(animation + 4) = name;

        for (i32 chunk_index = 0; chunk_index < *reinterpret_cast<i32 *>(animation + 8); ++chunk_index) {
            const i32 curve_set_count = NuFileReadInt(file);
            void **chunk_slot = *reinterpret_cast<void ***>(animation + 0xc) + chunk_index;
            u8 *chunk = reinterpret_cast<u8 *>(NuAnimDataChunkCreate(curve_set_count));
            *chunk_slot = chunk;
            *reinterpret_cast<i32 *>(chunk) = curve_set_count;

            u8 *curve_data = NULL;
            const i32 curve_data_count = NuFileReadInt(file);
            if (curve_data_count != 0) {
                const u32 curve_data_size = static_cast<u32>(curve_data_count) * 0x10;
                curve_data = static_cast<u8 *>(NU_ALLOC(curve_data_size, 4, 1, "", 0));
                *reinterpret_cast<void **>(chunk + 0xc) = curve_data;
                NuFileRead(file, curve_data, curve_data_size);
                curve_data = *reinterpret_cast<u8 **>(chunk + 0xc);
            } else {
                *reinterpret_cast<void **>(chunk + 0xc) = NULL;
            }

            u8 *shared_curves = NULL;
            const i32 shared_curve_count = NuFileReadInt(file);
            if (shared_curve_count != 0) {
                const u32 shared_curve_size = static_cast<u32>(shared_curve_count) * 0x10;
                shared_curves = static_cast<u8 *>(NU_ALLOC(shared_curve_size, 4, 1, "", 0));
                *reinterpret_cast<void **>(chunk + 0x10) = shared_curves;
                NuFileRead(file, shared_curves, shared_curve_size);
            } else {
                *reinterpret_cast<void **>(chunk + 0x10) = NULL;
            }

            for (i32 set_index = 0; set_index < curve_set_count; ++set_index) {
                const i32 curve_count = NuFileReadChar(file);
                if (curve_count == 0) {
                    continue;
                }

                void **set_slot = *reinterpret_cast<void ***>(chunk + 8) + set_index;
                *set_slot = NuAnimCurveSetCreate(curve_count);
                u8 *curve_set = static_cast<u8 *>((*reinterpret_cast<void ***>(chunk + 8))[set_index]);
                *reinterpret_cast<i32 *>(curve_set) = NuFileReadInt(file);
                for (i32 curve_index = 0;
                     curve_index < *reinterpret_cast<i8 *>(
                                       static_cast<u8 *>((*reinterpret_cast<void ***>(chunk + 8))[set_index]) + 0xc);
                     ++curve_index) {
                    curve_set = static_cast<u8 *>((*reinterpret_cast<void ***>(chunk + 8))[set_index]);
                    f32 *value = *reinterpret_cast<f32 **>(curve_set + 4) + curve_index;
                    *value = NuFileReadFloat(file);
                }
            }

            u8 *shared_cursor = *reinterpret_cast<u8 **>(chunk + 0x10);
            u8 *curve_data_cursor = curve_data;
            for (i32 set_index = 0; set_index < *reinterpret_cast<i32 *>(chunk); ++set_index) {
                u8 *curve_set = static_cast<u8 *>((*reinterpret_cast<void ***>(chunk + 8))[set_index]);
                if (curve_set == NULL) {
                    continue;
                }
                for (i32 curve_index = 0;
                     curve_index < *reinterpret_cast<i8 *>(
                                       static_cast<u8 *>((*reinterpret_cast<void ***>(chunk + 8))[set_index]) + 0xc);
                     ++curve_index) {
                    curve_set = static_cast<u8 *>((*reinterpret_cast<void ***>(chunk + 8))[set_index]);
                    if ((*reinterpret_cast<f32 **>(curve_set + 4))[curve_index] == FLT_MAX) {
                        (*reinterpret_cast<void ***>(curve_set + 8))[curve_index] = shared_cursor;
                        ++*reinterpret_cast<i32 *>(chunk + 4);
                        *reinterpret_cast<void **>(shared_cursor + 4) = curve_data_cursor;
                        curve_data_cursor += *reinterpret_cast<i32 *>(shared_cursor + 8) << 4;
                        shared_cursor += 0x10;
                    }
                }
            }
        }
        return animation;
    }
    void *NuAnimGetAnimLOD(void *animation, i32 lod) {
        if (lod < 0 || animation == NULL) {
            return NULL;
        }

        u8 *lod_animation = static_cast<u8 *>(animation);
        while (lod-- != 0) {
            const u16 next_lod = *reinterpret_cast<u16 *>(lod_animation + 0x14);
            if (next_lod == 0) {
                return NULL;
            }
            lod_animation += next_lod;
        }
        return lod_animation;
    }
    i32 NuAnimGetUseQuatsFlag(void) {
        return ForceEulerToQuat;
    }
    void NuAnimInit(i32 max_joints, VARIPTR *buf, VARIPTR buf_end) {

        buildBitCountTable();
        NuAnimBuffInit(max_joints, buf, buf_end);
    }
    i32 NuAnimNumNodes(void *animation) {
        const ani3_animheader_s *header = static_cast<const ani3_animheader_s *>(animation);
        if (header->magic == ANI3_MAGIC_VERSION_4 || header->magic == ANI3_MAGIC_VERSION_5) {
            return header->node_count;
        }
        return *reinterpret_cast<const i16 *>(static_cast<const u8 *>(animation) + 4);
    }
    i32 NuAnimPopUseQuatsFlag(void) {
        if (NumQuatPushes == 0) {
            return 0;
        }
        ForceEulerToQuat = QuatPushes[--NumQuatPushes];
        return ForceEulerToQuat;
    }
    i32 NuAnimPushSetUseQuatsFlag(i32 enabled) {
        const i32 previous = ForceEulerToQuat;
        if (NumQuatPushes > 3) {
            return 0;
        }
        QuatPushes[NumQuatPushes++] = ForceEulerToQuat;
        ForceEulerToQuat = static_cast<u8>(enabled);
        return previous;
    }
    i32 NuAnimSetUseQuatsFlag(i32 enabled) {
        const i32 previous = ForceEulerToQuat;
        ForceEulerToQuat = static_cast<u8>(enabled);
        return previous;
    }

    // ---------------------------------------------------------------------------
    // Bridge / file / heap / memory
    // ---------------------------------------------------------------------------

    void NuDatClose(NUDATHDR *header) {
        extern NUDATFILEINFO dat_file_infos[20];

        for (i32 index = 0; index < 20; ++index) {
            NUDATOPENFILEINFO *open_file = &header->open_files[index];
            if (open_file->dat_file != 0) {
                NuFileClose(open_file->dat_file);
            }
            if (open_file->info_idx >= 0) {
                dat_file_infos[open_file->info_idx].is_used = 0;
            }
        }
        if (header->unknown != 0) {
            NU_FREE(header);
        }
    }
    i32 nufile_lsn_allowed = 1;

    i32 NuDatGetFileInfo(NUDATHDR *header, char *name, i64 *position, i32 *length) {
        if (header == NULL) {
            return -1;
        }

        const i32 index = NuDatFileFindTree(header, name);
        if (index < 0) {
            return -1;
        }

        const NUDATFINFO *file = &header->file_info[index];
        const i64 file_position = NuDatCalcPos(header, file->file_offset);
        if (position != NULL) {
            if (nufile_lsn_allowed != 0 && header->unknown2 != 0) {
                *position = header->unknown2 + file_position / 0x800;
            } else {
                *position = file_position;
            }
        }
        if (length != NULL) {
            *length = file->file_len;
        }
        return index;
    }
    void *NuMemAllocFn(u32 size) {
        return NU_ALLOC(size, 4, 1, "", 0);
    }
    void NuMemFreeFn(void *ptr) {
        NU_FREE(ptr);
    }
    void *NuMemReAllocFn(void *ptr, u32 size) {
        return NuMemoryGet()->GetThreadMem()->_BlockReAlloc(ptr, size, 4, 1, "", 0);
    }
    u8 PS2_SCRATCH_BASE[0x8000];
    static u8 *ps2_scratch_free;

    // Original @0x316d41.
    void NuScratchReset(void) {
        ps2_scratch_free = PS2_SCRATCH_BASE;
    }

    static void *NuScratchAllocAligned(i32 size, usize alignment) {
        if (ps2_scratch_free == NULL) {
            NuScratchReset();
        }
        u8 *previous = ps2_scratch_free;
        u8 *allocation = reinterpret_cast<u8 *>(ALIGN(reinterpret_cast<usize>(ps2_scratch_free), alignment));
        ps2_scratch_free = allocation + ALIGN(size, 4);
        *reinterpret_cast<u8 **>(ps2_scratch_free) = previous;
        ps2_scratch_free += sizeof(previous);
        return allocation;
    }

    // Original @0x316e45 / 0x316d5d / 0x316dd1.
    void *NuScratchAlloc128(i32 size) {
        return NuScratchAllocAligned(size, 16);
    }
    void *NuScratchAlloc32(i32 size) {
        return NuScratchAllocAligned(size, 4);
    }
    void *NuScratchAlloc64(i32 size) {
        return NuScratchAllocAligned(size, 8);
    }

    // Original @0x316eb9.
    void NuScratchRelease(void) {
        ps2_scratch_free = *reinterpret_cast<u8 **>(ps2_scratch_free - sizeof(ps2_scratch_free));
    }
    void *NuPtrBlockRead(NUFILE file) {
        void *block = NuMemFileAddr(file);
        return NuPtrBlockFix(block);
    }

    void NuSysDirClose(void) {
    }
    i32 NuSysDirOpen(void) {
        return 0;
    }
    i32 NuSysDirRead(void) {
        return 0;
    }

    // ---------------------------------------------------------------------------
    // Math / geometry
    // ---------------------------------------------------------------------------

    f32 NuLog2(f32 value) {
        return NuLog10(value) * 3.321928f;
    }
    // Original @0x2babf0.  The VU0 entry point is an ordinary CPU wrapper in
    // this build; animation evaluation uses it to combine an animated joint
    // rotation with that joint's bind-pose matrix.
    void NuMtxMulRVU0(NUMTX *result, NUMTX *left, NUMTX *right) {
        NuMtxMulR(result, left, right);
    }
    void NuMtxMulVU0(NUMTX *result, NUMTX *left, NUMTX *right) {
        NuMtxMulH(result, left, right);
    }
    void NuMtxPreScaleVU0(NUMTX *matrix, NUVEC *scale) {
        NuMtxPreScale(matrix, scale);
    }
    // Original @0x2bad80.  Preserve the public VU0-shaped entry point while
    // sharing the scalar matrix implementation used by the host build.
    void NuMtxScaleVU0(NUMTX *matrix, NUVEC *scale) {
        NuMtxScale(matrix, scale);
    }
    void NuMtxSetRotateXYZVU0(NUMTX *matrix, NUANGVEC *angles) {
        NuMtxSetRotateXYZ(matrix, angles);
    }
    static f32 pow_x[32], pow_y[32], pow_rv[32];
    static i32 pow_cache_free;
    f32 NuPow(f32 x, f32 y) {
        if (x == 0.0f)
            return 0.0f;
        static i32 first_time = 1;
        if (first_time != 0) {
            first_time = 0;
            for (i32 i = 0; i < 32; ++i)
                pow_x[i] = FLT_MAX;
        }
        for (i32 i = 0; i < 32; ++i) {
            if (pow_x[i] == x && pow_y[i] == y)
                return pow_rv[i];
        }
        f32 result = static_cast<f32>(exp(static_cast<double>(y) * log(static_cast<double>(x))));
        pow_x[pow_cache_free] = x;
        pow_y[pow_cache_free] = y;
        pow_rv[pow_cache_free] = result;
        pow_cache_free = (pow_cache_free + 1) & 31;
        return result;
    }
    i32 NuPower2(i32 value) {
        i32 power = value > 127 ? 128 : 1;
        while (power < value)
            power += power;
        return power;
    }
    void NuEulerXYZFromQuat(void) {
    }

    // ---------------------------------------------------------------------------
    // Fonts / text
    // ---------------------------------------------------------------------------

    void NuFntClose(void) {
    }
    void *NuFntCreate(void) {
        return NULL;
    }
    void NuFntDestroy(void) {
    }
    void NuFntGetScreenHeight(void) {
    }
    void NuFntInit(void) {
    }
    void *NuFntLoadPtr(void) {
        return NULL;
    }
    void NuFntMoveAbs(void) {
    }
    void NuFntMoveRel(void) {
    }
    void NuFntPointSize(void) {
    }
    void NuFntPos(void) {
    }
    void NuFntPrint(void) {
    }
    void NuFntPrintEx(void) {
    }
    void NuFntPrintLen(void) {
    }
    void NuFntPrintLenV(void) {
    }
    void NuFntPrintV(void) {
    }
    void NuFntScale(void) {
    }
    void NuFntSet(void) {
    }
    void NuFntSetFixedWidthNumerals(void) {
    }
    void NuFntSetPen(void) {
    }
    void NuFntToLower(void) {
    }
    void NuFntToUpper(void) {
    }
    void NuFntWrite(void) {
    }
    void NuQFntCreate(void) {
    }
    void NuQFntDestroy(VUFNT *font) {
        if (font != NULL) {
            NUMTL *material = font->mtl;
            i32 texture_id = material->tex_id;
            NuMtlDestroy(material);
            if ((font->flags & 1) == 0) {
                NuTexDestroy(texture_id);
            }
        }
    }
    void NuQFntEncodeUnicodeString(NUQFNT *font, u16 *text) {
        if (font == NULL) {
            font = system_qfont;
        }
        if (font == NULL) {
            return;
        }
        VUFNT *vufnt = static_cast<VUFNT *>(font);
        while (*text != 0) {
            i32 index = UnicodeToIndexFast(vufnt->unicode_map, vufnt->unicode_count, *text);
            *text = static_cast<u16>(static_cast<u16>(index) == 0xffff ? '?' : index);
            ++text;
        }
    }
    NUQFNT_CSMODE NuQFntGetCoordinateSystem(void) {
        return NuQFntCSMode;
    }
    u32 NuQFntGetPrintMode(void) {
        return NuQFntMode;
    }
    f32 NuQFntHeightScale(void) {
        return qfnt_height_scale;
    }
    f32 NuQFntLenScale(void) {
        return qfnt_len_scale;
    }
    void NuQFntMove2d(NUQFNT *font, f32 x, f32 y, f32 z) {
        NuQFntPushPrintMode(2);
        NuQFntMove(font, x, y, z);
        NuQFntPopPrintMode();
    }
    void NuQFntPopCoordinateSystem(void) {
        if (NuQFntCSModeStackIndex > 0) {
            --NuQFntCSModeStackIndex;
            NuQFntSetCoordinateSystem(NuQFntCSModeStack[NuQFntCSModeStackIndex]);
        }
    }
    void NuQFntPrint2dU(NUQFNT *font, char *text) {
        NuQFntPushPrintMode(2);
        NuQFntPrintU(font, text);
        NuQFntPopPrintMode();
    }
    void NuQFntPrint2dW(NUQFNT *font, u16 *text) {
        NuQFntPushPrintMode(2);
        NuQFntPrintW(font, text);
        NuQFntPopPrintMode();
    }
    void NuQFntPrint3DU(NUQFNT *font, char *text) {
        NuQFntPushPrintMode(4);
        NuQFntPrintU(font, text);
        NuQFntPopPrintMode();
    }
    void NuQFntPrint3DW(NUQFNT *font, u16 *text) {
        NuQFntPushPrintMode(4);
        NuQFntPrintW(font, text);
        NuQFntPopPrintMode();
    }
    void NuQFntPrintEx(NUQFNT *font, i32 x, i32 y, i32 alignment, const char *format, ...) {
        char text[1024];
        NuQFntPushPrintMode(2);
        va_list arguments;
        va_start(arguments, format);
        vsprintf(text, format, arguments);
        va_end(arguments);
        i32 width = static_cast<i32>(NuQFntPrintLenU(font, text));
        if (alignment == 0x20) {
            x -= width;
        } else if (alignment == 0x40) {
            x -= width / 2;
        }
        NuQFntMove(font, static_cast<f32>(x), static_cast<f32>(y), 0.0f);
        NuQFntPrintU(font, text);
        NuQFntPopPrintMode();
    }
    f32 NuQFntPrintLenV(NUQFNT *font, const char *format, va_list arguments) {
        char text[1024];
        vsprintf(text, format, arguments);
        return NuQFntPrintLenU(font, text);
    }
    void NuQFntPrintV(NUQFNT *font, const char *format, va_list arguments) {
        char text[1024];
        vsprintf(text, format, arguments);
        NuQFntPrintU(font, text);
    }
    void NuQFntPushCoordinateSystem(NUQFNT_CSMODE mode) {
        if (NuQFntCSModeStackIndex < 16) {
            NuQFntCSModeStack[NuQFntCSModeStackIndex] = NuQFntCSMode;
            ++NuQFntCSModeStackIndex;
        }
        NuQFntSetCoordinateSystem(mode);
    }
    void NuQFntSet2d(NUQFNT *font) {
        NuQFntPushPrintMode(2);
        NuQFntSet(font);
        NuQFntPopPrintMode();
    }
    void NuQFntSetColour2d(NUQFNT *font, u32 colour) {
        NuQFntPushPrintMode(2);
        NuQFntSetColour(font, colour);
        NuQFntPopPrintMode();
    }
    void NuQFntSetScale2d(NUQFNT *font, f32 x_scale, f32 y_scale);
    void NuQFntSetPointSize(NUQFNT *font, f32 width, f32 height) {
        if (font == NULL) {
            font = system_qfont;
        }
        if (font == NULL) {
            return;
        }
        VUFNT *vufnt = static_cast<VUFNT *>(font);
        u16 glyph_index = NuQFntEncodeUnicodeChar(font, 'M');
        f32 x_scale;
        f32 y_scale;
        if (glyph_index == 0xffff) {
            y_scale = height * 18.0f / vufnt->height;
            x_scale = y_scale / height;
        } else {
            x_scale = width * 7.0f / vufnt->glyphs[glyph_index].width;
            y_scale = height * 18.0f / vufnt->height;
        }
        NuQFntSetScale(font, x_scale, y_scale);
        NuQFntSetScale2d(font, x_scale, y_scale);
    }
    void NuQFntSetPrintMode(u32 mode) {
        NuQFntMode = mode;
    }
    void NuQFntSetScale2d(NUQFNT *font, f32 x_scale, f32 y_scale) {
        NuQFntPushPrintMode(2);
        NuQFntSetScale(font, x_scale, y_scale);
        NuQFntPopPrintMode();
    }
    void NuQFntWrite(void) {
    }
    void NuQFntWriteUniversalFont(void) {
    }

    // ---------------------------------------------------------------------------
    // Rendering / materials / effects (host has GL paths elsewhere)
    // ---------------------------------------------------------------------------

    void NuAccumulationMotionBlurEffect(i32 frames, f32 blend, i32 mode) {
        currentScene.accumulation_blend = blend;
        currentScene.unknown_174 = 1;
        currentScene.unknown_178 = 1;
        currentScene.accumulation_mode = mode;
        currentScene.accumulation_frames = frames;
        motionBlurAccumActiveThisFrame = 1;
    }
    void NuAccumulationMotionBlurParams(i32 frames, f32 blend, i32 mode) {
        currentScene.accumulation_blend = blend;
        currentScene.unknown_174 = 1;
        currentScene.accumulation_frames = frames;
        currentScene.accumulation_mode = mode;
    }
    extern nudisplayscene_s currentScene;
    void NuBackbufferCopy(i32 texture_id) {
        currentScene.unknown_214 = static_cast<u32>(texture_id);
    }
    void NuDeferredShadingRender(void) {
        currentScene.unknown_48 = 1;
    }
    void NuDeferredShadingSetParameterf(i32 parameter, f32 value) {
        switch (parameter) {
            case 0:
                currentScene.unknown_4c = value;
                break;
            case 1:
                currentScene.unknown_50 = value;
                break;
            case 2:
                currentScene.unknown_54 = value;
                break;
        }
    }
    void NuDepthOfFieldEffect(f32 strength, f32 near_distance, f32 far_distance) {
        currentScene.dof.enabled = 1;
        currentScene.dof.strength = strength;
        currentScene.dof.near_distance = near_distance;
        currentScene.dof.far_distance = far_distance;
        currentScene.dof.bias = 0.0f;
        currentScene.dof.mode = 3;
        if (NuRndrDoingScreenGrab != 0) {
            currentScene.dof.enabled = 0;
        }
    }
    void NuDepthOfFieldEffect1(f32 near_distance, f32 far_distance) {
        near_distance =
            (far_distance - near_distance) * (near_distance / (near_distance + far_distance)) + near_distance;
        NuDepthOfFieldEffect(1.0f, (far_distance * near_distance) / ((far_distance - near_distance) + far_distance),
                             near_distance);
    }
    void NuDepthOfFieldEffect2(f32 near_distance, f32 far_distance, f32 strength) {
        near_distance =
            (far_distance - near_distance) * (near_distance / (near_distance + far_distance)) + near_distance;
        NuDepthOfFieldEffect(strength, (far_distance * near_distance) / ((far_distance - near_distance) + far_distance),
                             near_distance);
    }
    void NuDepthOfFieldEffectEx(const NuDepthOfFieldParameters *parameters) {
        currentScene.dof = *parameters;
        if (NuRndrDoingScreenGrab != 0) {
            currentScene.dof.enabled = 0;
        }
    }
    void *NuEffectTexCreate1D(void) {
        return NULL;
    }
    void NuEffectTex360Create2D_aliased(void) {
    }
    void NuEffectTex360Create3D_aliased(void) {
    }
    void NuEffectTexCreateCube(void) {
    }
    void NuEffectTexCreateCube_aliased(void) {
    }
    void NuEffectTexCreateFromNativeTex(void) {
    }
    void NuEffectTexGetDimension(nueffecttex_s *texture, i32 lod, i32 *width, i32 *height) {
        const i16 *dimensions = reinterpret_cast<const i16 *>(texture);
        *width = (dimensions[1] >> lod) & ~1;
        *height = (dimensions[2] >> lod) & ~1;
    }
    struct NativeEffectTexture {
        NUNATIVETEX native;
        nueffecttex_s *effect;
        i32 unknown_2c;
    };
    DECOMP_ASSERT(sizeof(NativeEffectTexture) == 0x30, "Native effect texture stride");
    DECOMP_ASSERT(offsetof(NativeEffectTexture, effect) == 0x28, "Native effect texture link");

    struct NativeEffectTexturePool {
        NativeEffectTexture entries[32];
        u8 unknown_600[16];
    };
    DECOMP_ASSERT(sizeof(NativeEffectTexturePool) == 0x610, "Native effect texture pool size");
    static NativeEffectTexturePool nativeTexPool;

    nueffecttex_s *NuEffectTexGetEffectFromNative(i32 tex_id) {
        NativeEffectTexture *texture = (NativeEffectTexture *)NuTexGetNative(tex_id);
        isize index = ((u8 *)texture - (u8 *)nativeTexPool.entries) / (isize)sizeof(NativeEffectTexture);
        if ((u32)index < 32)
            return texture->effect;
        return NULL;
    }
    void *NuEffectTexGetLockedVP(void) {
        return NULL;
    }
    void NuEffectTexLockVP(void *buffer, void *buffer_end) {
        (void)buffer;
        (void)buffer_end;
    }
    void NuEffectTexMapNative(nueffecttex_s *texture) {
    }
    void NuEffectTexUnlockVP(void) {
    }
    void NuEffectTexUnmapNative(void) {
    }
    void NuFramebufferAttachTex2D(nuframebuffer_s *, i32, nueffecttex_s *, i32) {
    }
    void NuFramebufferBind(nuframebuffer_s *) {
    }
    static void NuFramebufferClear(void) {
    }
    void NuFramebufferCopyTex2D(i32, nueffecttex_s *, i32, i32, i32, i32, i32) {
    }
    void NuFramebufferDestroy(nuframebuffer_s *) {
    }
    void NuFramebufferDrawBuffers(void) {
    }
    void NuFramebufferEnableGuards(nuframebuffer_s *, bool) {
    }
    nueffecttex_s *NuFramebufferGetAttachedTex(nuframebuffer_s *, i32, i32 *, i32 *) {
        return NULL;
    }
    void *NuFramebufferGetBackBuffer(void) {
        return NULL;
    }
    nuframebuffer_s *NuFramebufferGetBound(void) {
        return NULL;
    }
    nuframebuffer_s *NuFramebufferGetDefault(void) {
        return NULL;
    }
    nuframebuffer_s *NuFramebufferGetFrontBuffer(void) {
        return NULL;
    }
    i32 NuFramebufferGetHeight(nuframebuffer_s *framebuffer) {
        return *reinterpret_cast<const i32 *>(reinterpret_cast<const u8 *>(framebuffer) + 0xe0);
    }
    nuframebuffer_s *NuFramebufferGetObject(i32) {
        return NULL;
    }
    i32 NuFramebufferGetSamples(nuframebuffer_s *framebuffer) {
        return *reinterpret_cast<const i32 *>(reinterpret_cast<const u8 *>(framebuffer) + 0xe8);
    }
    i32 NuFramebufferGetWidth(nuframebuffer_s *framebuffer) {
        return *reinterpret_cast<const i32 *>(reinterpret_cast<const u8 *>(framebuffer) + 0xdc);
    }
    void NuFramebufferInitEx(void) {
    }
    void NuFramebufferResolve(i32, bool) {
    }
    void NuFramebufferResolveAll(bool) {
    }
    void NuFramebufferResolveMultisample(i32) {
        NuFramebufferResolveAll(true);
    }
    void NuFramebufferSetClearColor(void) {
    }
    static void NuFramebufferSwapBuffers(void) {
    }
    void NuLightFogX(f32 near_distance, f32 far_distance, u32 colour, f32, f32, i32, f32 density) {
        NuRndrStateSetFogEnabled(1);
        NuRndrStateSetFogState(near_distance, far_distance, colour, density);
    }
    i32 speedblur_enabled = 1;
    f32 NuLightsx, NuLightsy;
    void NuLightSpeedBlur(i32 reuse_camera, f32 scale) {
        static NUMTX _viewProj, _preViewProj;
        if (speedblur_enabled == 0)
            return;
        currentScene.speed_blur.enabled = 1;
        if (reuse_camera == 0) {
            NuMtxMulH(&_preViewProj, &currentScene.speed_blur.current, NuCameraGetProjectionMtx());
            NUMTX *projection = NuCameraGetProjectionMtx();
            NuMtxMulH(&_viewProj, NuCameraGetViewMtx(), projection);
        }
        currentScene.speed_blur.previous = _viewProj;
        currentScene.speed_blur.current = _preViewProj;
        currentScene.speed_blur.unknown_84 = nuapi.frametime;
        currentScene.speed_blur.scale = scale;
    }
    void NuLightSpeedBlurOldCameraPos(const NUMTX *camera) {
        currentScene.speed_blur.current = *camera;
        NuMtxInv(&currentScene.speed_blur.current, &currentScene.speed_blur.current);
    }
    void NuLightSpeedBlurScale(f32 x, f32 y) {
        NuLightsx = x;
        NuLightsy = y;
    }
    NULGTARCLASER NuLgtArcLaserData[16];
    i32 NuLgtArcLaserOldCnt;
    void NuLgtArcLaser(i32 type, NUVEC *start, NUVEC *end, NUVEC *bend, f32 width, f32 segment_length, f32 wobble,
                       f32 bend_amount, i32 colour) {
        NuLgtArcLaserEx(type, start, end, bend, width, segment_length, wobble, bend_amount, colour, 0);
    }
    i32 NuLgtLaserCnt;
    i32 NuLgtArcLaserCnt;
    i32 NuLgtArcLaserFrame;
    NULGTLASER NuLgtLaserData[64];

    void NuLgtLaser(i32 type, f32 width, f32 segment_length, f32 width_wobble, NUVEC *start, NUVEC *delta, u32 colour,
                    f32 end_width, f32 length) {
        if (NuLgtLaserCnt > 63)
            return;
        NULGTLASER *laser = &NuLgtLaserData[NuLgtLaserCnt];
        laser->width_wobble = width_wobble;
        laser->segment_length = segment_length;
        laser->arc = 0;
        laser->width = width;
        laser->type = static_cast<u8>(type);
        laser->start = *start;
        laser->length = length;
        laser->end.x = delta->x + start->x;
        laser->end.y = delta->y + start->y;
        laser->end.z = delta->z + start->z;
        laser->colour = colour;
        laser->end_width_ratio = end_width / width;
        // The original tests the arc cursor here, even for a straight laser.
        if ((NuLgtArcLaserFrame & 1) == 0 || NuLgtLaserData[NuLgtArcLaserCnt].seed == 0) {
            NuLgtLaserData[NuLgtLaserCnt].seed = NuLgtRand();
        }
        NuLgtRand();
        NuLgtRand();
        NuLgtRand();
        NuLgtRand();
        NuLgtRand();
        NuLgtRand();
        ++NuLgtLaserCnt;
    }
    static inline u16 NuLgtHalf(f32 value) {
        union {
            f32 value;
            u32 bits;
        } conversion = {value};
        i32 sign = conversion.bits >> 31;
        i32 exponent = (conversion.bits >> 23) & 0xff;
        i32 mantissa = conversion.bits & 0x7fffff;
        // Preserve the original truncation and exponent clamp, including subnormals.
        exponent -= 112;
        if (exponent < 0)
            exponent = 0;
        if (exponent > 31)
            exponent = 31;
        return (sign << 15) | ((exponent & 31) << 10) | (mantissa >> 13);
    }
    static inline void NuLgtVertex(NUVEC4 *position, i32 colour, f32 u, f32 v) {
        g_NuPrim_StreamBufferPtr->u32_ptr[3] =
            g_NuPrim_NeedsOverbrightening ? colour : ((colour >> 1) & 0x7f7f7f) | (colour & 0xff000000);
        if (g_NuPrim_NeedsHalfUVs) {
            ((u16 *)g_NuPrim_StreamBufferPtr->u8_ptr)[8] = NuLgtHalf(u);
            ((u16 *)g_NuPrim_StreamBufferPtr->u8_ptr)[9] = NuLgtHalf(v);
        } else {
            g_NuPrim_StreamBufferPtr->f32_ptr[4] = u;
            g_NuPrim_StreamBufferPtr->f32_ptr[5] = v;
        }
        NuPrim2DAddXYZ(position->x, -position->y, position->z * 0.91f);
    }
    void NuLgtLaserDraw(i32 paused) {
        if (paused)
            NuLgtLaserCnt = NuLgtLaserOldCnt;
        if (NuLgtArcMtl[0].material == NULL || NuLgtLaserCnt == 0) {
            NuLgtLaserOldCnt = 0;
            return;
        }
        f32 v0 = NuLgtArcMtl[0].v0;
        f32 v1 = NuLgtArcMtl[0].v1;
        u32 saved_seed = NuLgtSeed;
        // Four strip corners, two projected endpoints, and two clipped world endpoints.
        NUVEC4 *vertices = (NUVEC4 *)NuScratchAlloc32(0x80);
        NUMTX clip_matrix;
        NuCameraGetClipMtx(&clip_matrix, NULL);
        f32 aspect = (f32)PS2_REZ_W / (f32)PS2_REZ_H;
        for (i32 i = 0; i < NuLgtLaserCnt; ++i) {
            NULGTLASER *laser = &NuLgtLaserData[i];
            vertices[4].x = laser->start.x;
            vertices[4].y = laser->start.y;
            vertices[4].z = laser->start.z;
            vertices[4].w = 1.0f;
            NuVec4MtxTransformVU0(&vertices[4], &vertices[4], &clip_matrix);
            vertices[5].x = laser->end.x;
            vertices[5].y = laser->end.y;
            vertices[5].z = laser->end.z;
            vertices[5].w = 1.0f;
            NuVec4MtxTransformVU0(&vertices[5], &vertices[5], &clip_matrix);
            if (!(vertices[4].w >= 0.5f) && !(vertices[5].w >= 0.5f))
                continue;
            i32 red = laser->colour & 0xff;
            i32 green = (laser->colour >> 8) & 0xff;
            i32 blue = (laser->colour >> 16) & 0xff;
            i32 alpha = laser->colour >> 24;
            if (vertices[4].w < 0.5f) {
                f32 numerator = vertices[5].w - 0.5f;
                f32 denominator = vertices[5].w - vertices[4].w;
                if (laser->arc) {
                    red = (i32)((f32)red * numerator / denominator);
                    green = (i32)((f32)green * numerator / denominator);
                    blue = (i32)((f32)blue * numerator / denominator);
                    alpha = (i32)((f32)alpha * numerator / denominator);
                }
                vertices[6].x = (laser->start.x - laser->end.x) * numerator / denominator + laser->end.x;
                vertices[6].y = (laser->start.y - laser->end.y) * numerator / denominator + laser->end.y;
                vertices[6].z = (laser->start.z - laser->end.z) * numerator / denominator + laser->end.z;
            } else {
                vertices[6].x = laser->start.x;
                vertices[6].y = laser->start.y;
                vertices[6].z = laser->start.z;
            }
            vertices[6].w = 1.0f;
            f32 dx = vertices[6].x - laser->end.x;
            f32 dy = vertices[6].y - laser->end.y;
            f32 dz = vertices[6].z - laser->end.z;
            f32 remaining = NuFsqrt(dx * dx + dy * dy + dz * dz);
            if (vertices[5].w < 0.5f) {
                f32 numerator = vertices[4].w - 0.5f;
                f32 denominator = vertices[4].w - vertices[5].w;
                vertices[7].x = (laser->end.x - laser->start.x) * numerator / denominator + laser->start.x;
                vertices[7].y = (laser->end.y - laser->start.y) * numerator / denominator + laser->start.y;
                vertices[7].z = (laser->end.z - laser->start.z) * numerator / denominator + laser->start.z;
            } else {
                vertices[7].x = laser->end.x;
                vertices[7].y = laser->end.y;
                vertices[7].z = laser->end.z;
            }
            vertices[7].w = 1.0f;
            NuVec4MtxTransformVU0(&vertices[4], &vertices[6], &clip_matrix);
            NuVec4ScaleXYZVU0(&vertices[4], &vertices[4], 1.0f / vertices[4].w);
            NuVec4MtxTransformVU0(&vertices[5], &vertices[7], &clip_matrix);
            NuVec4ScaleXYZVU0(&vertices[5], &vertices[5], 1.0f / vertices[5].w);
            if (vertices[4].x < -1.0f && vertices[5].x < -1.0f)
                continue;
            if (vertices[4].x > 1.0f && vertices[5].x > 1.0f)
                continue;
            if (vertices[4].y < -1.0f && vertices[5].y < -1.0f)
                continue;
            if (vertices[4].y > 1.0f && vertices[5].y > 1.0f)
                continue;
            NUVEC perpendicular = {(vertices[5].y - vertices[4].y) * aspect, vertices[4].x - vertices[5].x, 0.0f};
            NuVecNorm(&perpendicular, &perpendicular);
            perpendicular.x *= laser->width * aspect;
            perpendicular.y *= laser->width;
            vertices[7].x -= vertices[6].x;
            vertices[7].y -= vertices[6].y;
            vertices[7].z -= vertices[6].z;
            vertices[7].w -= vertices[6].w;
            f32 length =
                NuFsqrt(vertices[7].x * vertices[7].x + vertices[7].y * vertices[7].y + vertices[7].z * vertices[7].z);
            if (!(length > 0.0f))
                continue;
            f32 step = laser->segment_length / length;
            if (step > 1.0f)
                step = 1.0f;
            // Per-laser seeds make repeated and paused rendering deterministic.
            NuLgtSeed = laser->seed;
            f32 u0 = NuLgtArcMtl[0].u0;
            f32 u1 = NuLgtArcMtl[0].u1;
            vertices[4].x = vertices[6].x;
            vertices[4].y = vertices[6].y;
            vertices[4].z = vertices[6].z;
            vertices[4].w = 1.0f;
            NuVec4MtxTransformVU0(&vertices[4], &vertices[4], &clip_matrix);
            f32 width_scale = 1.0f / vertices[4].w;
            NuVec4ScaleXYZVU0(&vertices[4], &vertices[4], width_scale);
            if (laser->length > remaining) {
                if (remaining < 0.0f)
                    width_scale *= laser->end_width_ratio;
                else
                    width_scale += (laser->end_width_ratio * width_scale - width_scale) * (laser->length - remaining) /
                                   laser->length;
            }
            vertices[1].x = vertices[4].x - perpendicular.x * width_scale;
            vertices[1].y = vertices[4].y - perpendicular.y * width_scale;
            vertices[1].z = vertices[4].z;
            vertices[3].x = vertices[4].x + perpendicular.x * width_scale;
            vertices[3].y = vertices[4].y + perpendicular.y * width_scale;
            vertices[3].z = vertices[4].z;
            remaining -= laser->segment_length;
            f32 fraction = 0.0f;
            i32 start_alpha = alpha;
            for (;;) {
                f32 next_fraction = fraction + step;
                i32 end_alpha = start_alpha;
                if (next_fraction < 1.0f) {
                    if (laser->arc)
                        end_alpha = (i32)((1.0f - fraction - step) * (f32)alpha);
                    vertices[5].x = next_fraction * vertices[7].x + vertices[6].x;
                    vertices[5].y = next_fraction * vertices[7].y + vertices[6].y;
                    vertices[5].z = next_fraction * vertices[7].z + vertices[6].z;
                } else {
                    if (laser->arc)
                        end_alpha = 0;
                    vertices[5].x = vertices[6].x + vertices[7].x;
                    vertices[5].y = vertices[6].y + vertices[7].y;
                    vertices[5].z = vertices[6].z + vertices[7].z;
                    u0 = NuLgtArcMtl[0].u1 + (1.0f - fraction) * (NuLgtArcMtl[0].u0 - NuLgtArcMtl[0].u1) / step;
                }
                vertices[5].w = 1.0f;
                NuVec4MtxTransformVU0(&vertices[5], &vertices[5], &clip_matrix);
                width_scale = 1.0f / vertices[5].w;
                NuVec4ScaleXYZVU0(&vertices[5], &vertices[5], width_scale);
                if (laser->length > remaining) {
                    if (remaining < 0.0f)
                        width_scale *= laser->end_width_ratio;
                    else
                        width_scale += (width_scale * laser->end_width_ratio - width_scale) *
                                       (laser->length - remaining) / laser->length;
                }
                f32 wobble_x = 0.0f, wobble_y = 0.0f;
                if (1.0f - 1.5f * step > fraction && laser->seed != 0) {
                    f32 random = (f32)((NuLgtRand() & 0xff) - 128);
                    wobble_x = perpendicular.x * random * laser->width_wobble * width_scale;
                    wobble_y = perpendicular.y * random * laser->width_wobble * width_scale;
                }
                vertices[0].x = vertices[5].x - perpendicular.x * width_scale + wobble_x;
                vertices[0].y = vertices[5].y - perpendicular.y * width_scale + wobble_y;
                vertices[0].z = vertices[5].z;
                vertices[2].x = vertices[5].x + perpendicular.x * width_scale + wobble_x;
                vertices[2].y = vertices[5].y + perpendicular.y * width_scale + wobble_y;
                vertices[2].z = vertices[5].z;
                ++NuPrimCSPos;
                NuPrimSetCoordinateSystem(NUPRIM_SCALEMODE_NORMALISED);
                NuPrim2DBegin(1, 7, NuLgtArcMtl[0].material);
                NuLgtVertex(&vertices[0],
                            ((red & 255) | ((green & 255) << 8) | ((blue & 255) << 16)) | ((u32)end_alpha << 24), u0,
                            v0);
                NuLgtVertex(&vertices[1],
                            ((red & 255) | ((green & 255) << 8) | ((blue & 255) << 16)) | ((u32)start_alpha << 24), u1,
                            v0);
                NuLgtVertex(&vertices[2],
                            ((red & 255) | ((green & 255) << 8) | ((blue & 255) << 16)) | ((u32)end_alpha << 24), u0,
                            v1);
                NuLgtVertex(&vertices[3],
                            ((red & 255) | ((green & 255) << 8) | ((blue & 255) << 16)) | ((u32)start_alpha << 24), u1,
                            v1);
                NuPrim2DEnd();
                NuPrimSetCoordinateSystem(NuPrimCoordSystemStack[--NuPrimCSPos]);
                vertices[1].x = vertices[0].x;
                vertices[1].y = vertices[0].y;
                vertices[1].z = vertices[0].z;
                vertices[3].x = vertices[2].x;
                vertices[3].y = vertices[2].y;
                vertices[3].z = vertices[2].z;
                remaining -= laser->segment_length;
                if (!(1.0f > next_fraction))
                    break;
                fraction = next_fraction;
                start_alpha = end_alpha;
            }
        }
        NuLgtSeed = saved_seed;
        NuLgtLaserOldCnt = NuLgtLaserCnt;
        NuLgtLaserCnt = 0;
        NuScratchRelease();
    }

    void NuLgtSetArcMat(NUMTL *material, f32 u0, f32 v0, f32 u1, f32 v1) {
        NuLgtSetArcMatEx(0, material, u0, v0, u1, v1);
    }
    void NuPostBloom(i32, const NuBloomParameters *parameters) {
        currentScene.bloom = *parameters;
    }
    // This entry point is empty in the original Android binary.
    void NuRainSetFall(void) {
    }
    void NuRenderContextInit(void) {
        extern f32 g_renderContext_viewProj[16];
        extern f32 g_renderContext_view[16];
        extern f32 g_renderContext_projection[16];
        extern f32 g_renderContext_world[16];
        memcpy(g_renderContext_viewProj, &numtx_identity, sizeof(numtx_identity));
        memcpy(g_renderContext_view, &numtx_identity, sizeof(numtx_identity));
        memcpy(g_renderContext_projection, &numtx_identity, sizeof(numtx_identity));
        memcpy(g_renderContext_world, &numtx_identity, sizeof(numtx_identity));
    }
    void NuRenderContext360BeginGameTime(void) {
    }
    void NuRenderContext360EndGameTime(void) {
    }
    void NuRenderContextSetAlphaBlend(void) {
    }
    __attribute__((weak)) void NuRenderContextSetViewProj(NUMTX *view, NUMTX *projection) {
        extern f32 g_renderContext_viewProj[16];
        extern f32 g_renderContext_viewProjInverse[16];
        extern f32 g_renderContext_view[16];
        extern f32 g_renderContext_projection[16];
        extern f32 g_renderContext_position[4];

        NUVEC scale = {
            g_NuVpRegion.projection_x_scale,
            g_NuVpRegion.projection_y_scale,
            1.0f,
        };
        NUVEC translation = {
            g_NuVpRegion.projection_x_offset,
            g_NuVpRegion.projection_y_offset,
            0.0f,
        };
        NUMTX scale_mtx;
        NUMTX translation_mtx;
        NUMTX adjusted_projection;
        NuMtxSetScale(&scale_mtx, &scale);
        NuMtxSetTranslation(&translation_mtx, &translation);
        NuMtxMulH(&adjusted_projection, projection, &scale_mtx);
        NuMtxMulH(&adjusted_projection, &adjusted_projection, &translation_mtx);

        memcpy(g_renderContext_view, view, sizeof(NUMTX));
        memcpy(g_renderContext_projection, &adjusted_projection, sizeof(NUMTX));

        NUMTX inverse_view;
        NuMtxInv(&inverse_view, view);
        g_renderContext_position[0] = inverse_view.m30 / inverse_view.m33;
        g_renderContext_position[1] = inverse_view.m31 / inverse_view.m33;
        g_renderContext_position[2] = inverse_view.m32 / inverse_view.m33;
        g_renderContext_position[3] = 1.0f;

        NuMtxMulH(reinterpret_cast<NUMTX *>(g_renderContext_viewProj), view, &adjusted_projection);
        NuMtxInvH(reinterpret_cast<NUMTX *>(g_renderContext_viewProjInverse),
                  reinterpret_cast<NUMTX *>(g_renderContext_viewProj));

        // OpenGL's clip-space depth is [-w,+w], while the engine camera
        // packet contains the original D3D-style [0,+w] projection.
        NUMTX depth_remap = numtx_identity;
        depth_remap.m22 = 2.0f;
        depth_remap.m32 = -1.0f;
        NuMtxMulH(reinterpret_cast<NUMTX *>(g_renderContext_viewProj),
                  reinterpret_cast<NUMTX *>(g_renderContext_viewProj), &depth_remap);

        NuShaderManagerSetfv(0x3d, g_renderContext_view);
        NuShaderManagerSetfv(0x3e, g_renderContext_viewProj);
        NuShaderManagerSetfv(0x56, g_renderContext_position);

        f32 fov;
        f32 aspect;
        f32 near_clip;
        f32 far_clip;
        f32 perspective[4];
        NuMtxGetPerspectiveD3D(projection, &fov, &aspect, &near_clip, &far_clip);
        perspective[0] = near_clip;
        perspective[1] = far_clip;
        perspective[2] = far_clip - near_clip;
        perspective[3] = perspective[2] / far_clip;
        NuShaderManagerSetfv(0x49, perspective);

        f32 frustum[4];
        NuMtxGetFrustumD3D(projection, &frustum[0], &frustum[1], &frustum[2], &frustum[3], &near_clip, &far_clip);
        frustum[1] -= frustum[0];
        frustum[3] -= frustum[2];
        NuShaderManagerSetfv(0x4a, frustum);
    }
    // Original 0x2a33d0, 9 bytes: this platform deliberately does nothing.
    void NuRenderContextSetViewport(i32, i32, i32, i32) {
    }
    void NuSpecialAddShadowLight(void) {
    }
    void NuSpecialBurstDrawAt(void) {
    }
    void NuSpecialClear(void *special) {
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        handle->scene = NULL;
        handle->special = NULL;
        handle->display_special = NULL;
    }
    void NuSpecialClearShadowClipTestResults(void) {
        nuspecial_shadowLightHaveClipOverrides = 0;
    }
    void NuSpecialClearShadowLights(void) {
    }
    i32 NuSpecialClipTestExtents(void *special, void *matrix_arg) {
        NUMTX *matrix = static_cast<NUMTX *>(matrix_arg);
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->special != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(handle->scene);
            NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
            NuPlainLegacyInstanceBoundsLayout *instance =
                reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
            NuPlainLegacyObjectBoundsLayout *object =
                static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
            return NuCameraClipTestExtents(&object->minimum, &object->maximum, matrix, 0.0f, 0);
        }
        NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
        return NuCameraClipTestExtents(&display->min, &display->max, matrix, 0.0f, 0);
    }
    i32 NuSpecialClipTestShadowLights(NUVEC *, NUVEC *, i32) {
        return 0;
    }
    i32 NuSpecialCompare(nuhspecial_s *first, nuhspecial_s *second) {
        if (first->special != NULL && first->special == second->special) {
            return 1;
        }
        if (first->display_special != NULL && first->display_special == second->display_special) {
            return 1;
        }
        return 0;
    }
    void NuSpecialConstAlpha(i32 enabled, f32 alpha) {
        if (enabled != 0) {
            nuspecial_const_alpha = alpha;
            nuspecial_draw_state |= 1;
        } else {
            nuspecial_draw_state &= ~1;
        }
        nuspecial_const_alpha_enabled = enabled;
    }
    void NuSpecialConstTint(i32 enabled, NUVEC *tint) {
        if (enabled != 0) {
            memcpy(&nuspecial_const_tint, tint, sizeof(nuspecial_const_tint));
            nuspecial_draw_state |= 2;
        } else {
            nuspecial_draw_state &= ~2;
        }
        nuspecial_const_tint_enabled = enabled;
    }
    i32 NuSpecialDrawAt(void *special, NUMTX *mtx) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle == NULL || handle->scene == NULL || handle->display_special == NULL) {
            return 0;
        }
        return NuDisplayListRndrSpecial(reinterpret_cast<nuhspecial_s *>(special), mtx, 0, NULL, NULL);
    }
    i32 NuSpecialDrawAtAlpha(void *special, NUMTX *mtx, f32 alpha) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->scene == NULL || alpha <= 0.0f) {
            return 0;
        }
        if (alpha < 1.0f) {
            NuSpecialConstAlpha(1, alpha);
            i32 result = NuDisplayListRndrSpecial(reinterpret_cast<nuhspecial_s *>(special), mtx, 0, NULL, NULL);
            NuSpecialConstAlpha(0, 0.0f);
            return result;
        }
        return NuDisplayListRndrSpecial(reinterpret_cast<nuhspecial_s *>(special), mtx, 0, NULL, NULL);
    }
    i32 NuSpecialDrawSmoothSkin(void *special, NUMTX *skin_matrices, NUMTX *world_matrix) {
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->scene == NULL || handle->display_special == NULL) {
            return 0;
        }
        return NuDisplayListRndrSpecial(reinterpret_cast<nuhspecial_s *>(special), world_matrix, 2, skin_matrices,
                                        NULL);
    }
    i32 NuSpecialDrawSmoothSkinDwa(void *special, NUMTX *skin_matrices, NUMTX *world_matrix,
                                   DEFORMERWEIGHTSARRAY *blend_values) {
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->scene == NULL || handle->display_special == NULL) {
            return 0;
        }
        return NuDisplayListRndrSpecial(reinterpret_cast<nuhspecial_s *>(special), world_matrix, 2, skin_matrices,
                                        blend_values);
    }
    i32 NuSpecialDrawWith(void *special, NUMTX *mtx) {
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->scene == NULL) {
            return 0;
        }
        NUMTX combined;
        if (handle->display_special != NULL) {
            NuMtxMul(&combined, static_cast<NUMTX *>(handle->display_special), mtx);
            return NuDisplayListRndrSpecial(reinterpret_cast<nuhspecial_s *>(special), &combined, 0, NULL, NULL);
        }
        NuMtxMul(&combined, static_cast<NUMTX *>(handle->special), mtx);
        return 0;
    }
    i32 NuSpecialFindMulti(NUGSCN *, nuhspecial_s *, char *, i32, i32) {
        return 0;
    }
    void NuSpecialFindMultiWC(void) {
    }
    i32 NuSpecialGetActiveShadowLights(void) {
        return nuspecial_shadowLightCount;
    }
    void NuSpecialGetBounds(void *special, NUVEC *minimum, NUVEC *maximum) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->special == NULL) {
            NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
            if (display != NULL) {
                *minimum = display->min;
                *maximum = display->max;
            }
            return;
        }

        NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(handle->scene);
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
        NuPlainLegacyInstanceBoundsLayout *instance =
            reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
        NuPlainLegacyObjectBoundsLayout *object =
            static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
        while (object->next != NULL) {
            object = object->next;
        }
        *minimum = object->minimum;
        *maximum = object->maximum;
    }
    NUMTX *NuSpecialGetDrawMtx(void *special) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
        if (legacy != NULL) {
            NUMTX *instance = reinterpret_cast<NUMTX *>(legacy->instance);
            NUMTX *draw_mtx = *reinterpret_cast<NUMTX **>(legacy->instance + 0x48);
            return draw_mtx != NULL ? draw_mtx : instance;
        }
        NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
        if (display != NULL) {
            usize draw_mtx = reinterpret_cast<usize>(display->instance_animation);
            if (draw_mtx != 0 && draw_mtx != static_cast<usize>(-1)) {
                return reinterpret_cast<NUMTX *>(display->instance_animation);
            }
            return &display->draw_mtx;
        }
        return NULL;
    }
    NUVEC *NuSpecialGetDrawPos(void *special) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
        if (display != NULL) {
            usize instance_animation = reinterpret_cast<usize>(display->instance_animation);
            NUVEC *draw_position = NUMTX_GET_ROW_VEC(&display->draw_mtx, 3);
            NUVEC *animated_position = reinterpret_cast<NUVEC *>(instance_animation + offsetof(NUMTX, m30));
            return instance_animation != 0 && instance_animation != static_cast<usize>(-1)
                       ? animated_position
                       : draw_position;
        }

        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
        if (legacy == NULL) {
            return NULL;
        }
        NUMTX *instance = reinterpret_cast<NUMTX *>(legacy->instance);
        NUMTX *draw_mtx = *reinterpret_cast<NUMTX **>(legacy->instance + 0x48);
        NUVEC *instance_position = NUMTX_GET_ROW_VEC(instance, 3);
        NUVEC *draw_position = reinterpret_cast<NUVEC *>(reinterpret_cast<usize>(draw_mtx) + offsetof(NUMTX, m30));
        return draw_mtx != NULL ? draw_position : instance_position;
    }
    i32 NuSpecialGetInstanceix(nuhspecial_s *special) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
        if (legacy != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(handle->scene);
            for (i32 i = 0; i < scene->instance_count; ++i) {
                if (scene->instances + i * 0x50 == legacy->instance) {
                    return i;
                }
            }
            return -1;
        }

        NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
        return display != NULL ? display->instance_ix : -1;
    }
    NUMTL *NuSpecialGetMtl(nuhspecial_s *special, i32 index) {
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(special->special);
        if (legacy != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(special->scene);
            NuPlainLegacyInstanceBoundsLayout *instance =
                reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
            NuPlainLegacyObjectBoundsLayout *object =
                static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
            while (object->next != NULL) {
                object = object->next;
            }
            NuPlainLegacyMaterialLink *link = object->materials;
            while (index != 0) {
                if (link == NULL) {
                    return NULL;
                }
                --index;
                link = link->next;
            }
            return link->material;
        } else if (special->display_special != NULL) {
            NUDISPLAYSPECIAL *display = special->display_special;
            i32 level = 0;
            while (display->clip_range[level] != 0.0f) {
                ++level;
            }
            return special->scene->display_list->mtls[display->clip_objects[level].material_ids[index]];
        }
        return NULL;
    }
    NUMTX *NuSpecialGetMtx(void *special) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->display_special != NULL) {
            return static_cast<NUMTX *>(handle->display_special);
        }
        return static_cast<NUMTX *>(handle->special);
    }
    f32 NuSpecialGetOriginRadius(void *special) {
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
        if (legacy != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(handle->scene);
            NuPlainLegacyInstanceBoundsLayout *instance =
                reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
            NuPlainLegacyObjectBoundsLayout *object =
                static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
            return object->origin_radius;
        }
        return NuVecMag(&static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special)->center) +
               static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special)->radius;
    }
    NUVEC *NuSpecialGetPos(void *special) {
        NuPlainSpecialHandleLayout *handle = static_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->display_special != NULL) {
            return reinterpret_cast<NUVEC *>(&static_cast<NUMTX *>(handle->display_special)->m30);
        }
        if (handle->special != NULL) {
            return reinterpret_cast<NUVEC *>(&static_cast<NUMTX *>(handle->special)->m30);
        }
        return NULL;
    }
    void NuSpecialGetRadius(void *special, NUVEC *position, f32 *radius) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle->special != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(handle->scene);
            NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
            NuPlainLegacyInstanceBoundsLayout *instance =
                reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
            NuPlainLegacyObjectBoundsLayout *object =
                static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
            *radius = object->radius;
            *position = object->center;
        } else {
            NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
            *position = display->center;
            *radius = display->radius;
        }
    }
    i32 NuSpecialGetShadowClipTestResult(i32 index) {
        if (nuspecial_shadowLightHaveClipOverrides != 0) {
            return nuspecial_shadowLightClipOverride[index];
        }
        return -1;
    }
    void *NuSpecialGetShadowLight(i32 index) {
        return nuspecial_shadowLight[index];
    }
    i32 NuSpecialHasActiveShadowLights(void) {
        return nuspecial_shadowLightCount > 0;
    }
    i32 NuSpecialHaveShadowClipTestResults(void) {
        return nuspecial_shadowLightHaveClipOverrides;
    }
    void NuSpecialList(void) {
    }
    i32 NuSpecialNumMtls(nuhspecial_s *special) {
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(special->special);
        if (legacy != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(special->scene);
            NuPlainLegacyInstanceBoundsLayout *instance =
                reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
            NuPlainLegacyObjectBoundsLayout *object =
                static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
            while (object->next != NULL) {
                object = object->next;
            }
            i32 count = 0;
            for (NuPlainLegacyMaterialLink *link = object->materials; link != NULL; link = link->next) {
                ++count;
            }
            return count;
        }
        if (special->display_special != NULL) {
            NUDISPLAYSPECIAL *display = special->display_special;
            i32 level = 0;
            while (display->clip_range[level] != 0.0f) {
                ++level;
            }
            return display->clip_objects[level].nmaterials;
        }
        return 0;
    }
    void NuSpecialSetAlphaTest(void) {
    }
    void NuSpecialSetBounds(nuhspecial_s *special, NUVEC *minimum, NUVEC *maximum) {
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(special->special);
        if (legacy != NULL) {
            NuPlainLegacySceneLayout *scene = reinterpret_cast<NuPlainLegacySceneLayout *>(special->scene);
            NuPlainLegacyInstanceBoundsLayout *instance =
                reinterpret_cast<NuPlainLegacyInstanceBoundsLayout *>(legacy->instance);
            NuPlainLegacyObjectBoundsLayout *object =
                static_cast<NuPlainLegacyObjectBoundsLayout *>(scene->objects[instance->object_index]);
            while (object != NULL) {
                object->minimum = *minimum;
                object->maximum = *maximum;
                object = object->next;
            }
        } else {
            NuPlainDisplaySpecialLayout *display =
                reinterpret_cast<NuPlainDisplaySpecialLayout *>(special->display_special);
            display->min = *minimum;
            display->max = *maximum;
        }
    }
    i32 NuSpecialSetClipping(i32 enabled, i32 state) {
        i32 previous = nuspecial_clip_state;
        nuspecial_clip_state = enabled != 0 ? state : -1;
        return previous;
    }
    void NuSpecialSetDrawMtx(void *special, NUMTX *mtx) {
        NuPlainSpecialHandleLayout *handle = reinterpret_cast<NuPlainSpecialHandleLayout *>(special);
        if (handle == NULL || handle->scene == NULL) {
            return;
        }
        NuPlainLegacySpecialLayout *legacy = static_cast<NuPlainLegacySpecialLayout *>(handle->special);
        if (legacy != NULL) {
            if (legacy->instance != NULL) {
                *reinterpret_cast<NUMTX *>(legacy->instance) = *mtx;
            }
            return;
        }
        NuPlainDisplaySpecialLayout *display = static_cast<NuPlainDisplaySpecialLayout *>(handle->display_special);
        if (display != NULL) {
            display->draw_mtx = *mtx;
            display->flags |= 0x400;
        }
    }
    void NuSpecialSetRenderPlane(void) {
    }
    void NuSpecialVertexOffsets(i32 count, VARIPTR offsets) {
        nuspecial_vertex_offsets = offsets;
        nuspecial_vertex_noffsets = count;
    }
    void NuSpecialVertexStates(NUSPECIALVERTEXSTATES *states) {
        nuspecial_vertex_states = states;
        ++render_state.state.global_id;
        ++render_state.state.vertex_groups_id;
    }

    void NuTimeBarSlotLastValue(void) {
    }
    void NuTimeBarSlotLastValueMicroseconds(void) {
    }
    void NuTimeBarSlotSetEx(void) {
    }

    // ---------------------------------------------------------------------------
    // Light / wind / particles / debris
    // ---------------------------------------------------------------------------

    void NuDynamicLightAddRenderScene(NuDynamicLight *light, i32 index, i32 scene_id) {
        NuDynamicLight::RenderSet &set = light->render_sets[index];
        set.reserved_33c[set.geometry_count++] = scene_id;
    }
    void NuDynamicLightAddShadowCasterScene(NuDynamicLight *light, nugscn_s *scene) {
        light->addShadowCasterScene(scene);
    }
    void NuDynamicLightBeginCapture(NuDynamicLight *light, i32 render_set) {
        currentScene.unknown_3c = light;
        currentScene.unknown_38 = 1;
        currentScene.unknown_40 = render_set;
    }
    NuDynamicLight *NuDynamicLightClone(NuDynamicLight *light, VARIPTR *arena, VARIPTR end) {
        return light->clone(arena, end);
    }
    NuDynamicLight *NuDynamicLightCreate() {
        return NuDynamicLight::create();
    }
    void NuDynamicLightDestroy(NuDynamicLight *light) {
        NuDynamicLight::destroy(light);
    }
    void NuDynamicLightEndCapture(void) {
    }
    i32 NuDynamicLightGetActiveRenderSetCount(NuDynamicLight *light) {
        return light->active_render_set_count;
    }
    void NuDynamicLightGetDList(void) {
    }
    f32 NuDynamicLightGetParameterf(NuDynamicLight *light, i32 parameter) {
        switch (parameter) {
            case 0: return light->parameter_7c0;
            case 1: return light->parameter_7c4;
            case 2: return light->parameter_7c8;
            case 3: return light->parameter_7cc;
            case 6:
            case 7: return light->render_sets[0].parameter_110;
            case 8: return light->render_sets[1].parameter_110;
            case 9:
            case 10:
            case 11:
            case 15:
            case 16: return light->render_sets[1].parameter_104;
            case 12:
            case 13:
            case 14:
            case 17:
            case 18: return light->render_sets[0].parameter_100;
            case 19: return light->parameter_7d0;
            case 20: return light->parameter_7d4;
            default: return 0.0f;
        }
    }
    i32 NuDynamicLightGetParameteri(NuDynamicLight *light, i32 parameter) {
        switch (parameter) {
            case 4:
                return light->parameter_4;
            case 5:
                return light->parameter_5;
            default:
                return 0;
        }
    }
    NUMTX *NuDynamicLightGetProjection(NuDynamicLight *light) {
        return &light->projection;
    }
    NUMTX *NuDynamicLightGetView(NuDynamicLight *light) {
        return &light->view;
    }
    i32 NuDynamicLightIsUsedOnSpecials(NuDynamicLight *light) {
        return light->used_on_specials;
    }
    void NuDynamicLightLookAt(NuDynamicLight *light, NUVEC *eye, NUVEC *target, NUVEC *up) {
        NUVEC direction;
        direction.z = target->z - eye->z;
        direction.y = target->y - eye->y;
        direction.x = target->x - eye->x;
        light->direction_w = 0.0f;
        light->direction = direction;
        NuVecNorm(&light->direction, &light->direction);
        NUVEC position;
        position.z = eye->z;
        position.y = eye->y;
        position.x = eye->x;
        light->position_w = 1.0f;
        light->position = position;
        NuMtxLookAtD3D(&light->view, eye, target, up);
    }
    void NuDynamicLightResetGeometry(NuDynamicLight *light) {
        light->resetGeometry();
    }
    void NuDynamicLightSetDirectional(NuDynamicLight *light, f32 left, f32 right, f32 bottom, f32 top, f32 near_plane,
                                      f32 far_plane) {
        light->parameter_4 = 0;
        NuMtxSetOrthoBlend(&light->projection, left, right, bottom, top, near_plane, far_plane);
    }
    void NuDynamicLightSetEnabled(NuDynamicLight *light, i32 enabled) {
        if (enabled != 0 && light->enabled == 0 && light->render_set_count > 0) {
            NuDynamicLight::RenderSet *set = light->render_sets;
            NuDynamicLight::RenderSet *end = set + light->render_set_count;
            do {
                set->field_2a0 = set->field_29c;
                set->field_2e4 = set->field_2e0;
                ++set;
            } while (set != end);
        }
        light->enabled = enabled;
    }
    void NuDynamicLightSetParameterf(NuDynamicLight *light, i32 parameter, f32 value) {
        switch (parameter) {
            case 0: light->parameter_7c0 = value; break;
            case 1: light->parameter_7c4 = value; break;
            case 2: light->parameter_7c8 = value; break;
            case 3: light->parameter_7cc = value; break;
            case 6:
            case 7: light->render_sets[0].parameter_110 = value; break;
            case 8: light->render_sets[1].parameter_110 = value; break;
            case 9:
            case 10:
            case 11: light->render_sets[0].parameter_104 = value; break;
            case 12:
            case 13:
            case 14: light->render_sets[0].parameter_100 = value; break;
            case 15:
            case 16: light->render_sets[1].parameter_104 = value; break;
            case 17:
            case 18: light->render_sets[1].parameter_100 = value; break;
            case 19: light->parameter_7d0 = value; break;
            case 20: light->parameter_7d4 = value; break;
        }
    }
    void NuDynamicLightSetParameteri(NuDynamicLight *light, i32 parameter, i32 value) {
        if (parameter == 4) {
            light->parameter_4 = value;
        } else if (parameter == 5) {
            light->parameter_5 = value;
        }
    }
    void NuDynamicLightSetUsedOnSpecials(NuDynamicLight *light, i32 enabled) {
        light->used_on_specials = enabled;
    }
    void NuDynamicLightSetupCustomCameraFrustum(NuDynamicLight *light, NUCAMERA *camera, const f32 *splits, i32 count) {
        light->setupCustomCameraFrustum(camera, splits, count);
    }
    void NuDynamicLightTestShadowExtrusionExtent(void) {
    }
    void NuDynamicLightTestShadowExtrusions(NuDynamicLight *light, const VuVec *first, const VuVec *second) {
        VuVec first_copy;
        VuVec second_copy;
        second_copy.x = second->x;
        second_copy.y = second->y;
        second_copy.z = second->z;
        second_copy.w = second->w;
        first_copy.x = first->x;
        first_copy.y = first->y;
        first_copy.z = first->z;
        first_copy.w = first->w;
        light->testShadowExtrusions(first_copy, second_copy);
    }
    void NuDynamicLightTestShadowExtrusionsExtent(NuDynamicLight *light, const NUVEC *center, const NUVEC *extent) {
        VuVec minimum;
        VuVec maximum;
        minimum.x = center->x - extent->x;
        minimum.y = center->y - extent->y;
        minimum.z = center->z - extent->z;
        maximum.x = center->x + extent->x;
        maximum.y = center->y + extent->y;
        maximum.z = center->z + extent->z;
        light->testShadowExtrusions(minimum, maximum);
    }
    void NuDynamicLightTestShadowExtrusionsSpecial(NuDynamicLight *light, void *special, NUMTX *matrix) {
        VuVec minimum;
        VuVec maximum;
        NuSpecialGetBounds(special, &minimum.xyz, &maximum.xyz);
        NuVecMtxTransform(&minimum.xyz, &minimum.xyz, matrix);
        NuVecMtxTransform(&maximum.xyz, &maximum.xyz, matrix);
        light->testShadowExtrusions(minimum, maximum);
    }
    void NuWindAnimate(NUWIND *wind, f32 frametime) {
        if (wind != NULL) {
            wind->unk2.z += (1.0f / 256.0f) * wind->unk2.y * frametime;
            if (wind->unk2.z >= 1.0f) {
                wind->unk2.z = NuFmod(wind->unk2.z, 1.0f);
            }
            f32 scaled_time = 5.0f * frametime;
            wind->unk2.w = frametime + wind->unk2.w;
            wind->unk3 = scaled_time + wind->unk3;
        }
    }
    extern "C++" NuWindGType *NuWindAllocateGrp();
    extern "C++" void NuWindFreeGrp(NuWindGType *group);

    i32 NuWindLoad(NUWIND *wind, i32 index, char *name, VARIPTR *buffer, VARIPTR *buffer_end) {
        if (wind != NULL) {
            if ((u32)index < 8) {
                if (wind->unk0[index] >= 0) {
                    NuTexDestroy(wind->unk0[index]);
                }
                i32 texture = NuTexRead(name, buffer, buffer_end);
                if (texture != 0) {
                    wind->unk0[index] = texture;
                    return texture;
                }
                wind->unk0[index] = -1;
                return -1;
            }
        }
        return -1;
    }

    void NuWindSetCurrent(NUWIND *wind, i32 index) {
        if ((u32)index > 7 || wind == NULL) {
            nuapi.wind = NULL;
        } else if (wind->unk0[index] >= 0) {
            wind->unk1 = index;
            nuapi.wind = wind;
        }
    }
    void NuWindSetSpeed(NUWIND *wind, f32 speed) {
        if (wind != NULL) {
            wind->unk2.y = 1.0f <= speed ? speed : 1.0f;
        }
    }
    void NuWindSetWorldSize(NUWIND *wind, f32 size) {
        if (wind != NULL) {
            wind->unk2.x = 1.0f <= size ? size : 1.0f;
        }
    }

    void NuWindUnload(NUWIND *wind, i32 index) {
        if (wind != NULL && wind->unk0[index] >= 0) {
            NuTexDestroy(wind->unk0[index]);
            wind->unk0[index] = -1;
        }
    }
    void NuWindUpdateArray(NUVEC **);

    extern "C" f32 partglobaltime;
    void NuPartResetGlobalTime(void) {
        partglobaltime = 0;
    }
    // ---------------------------------------------------------------------------
    // Gobj / hierarchy / scene graph
    // ---------------------------------------------------------------------------

    // Original @0x2fb930. Hierarchies are backed by graphics scenes, so their
    // destruction follows the ordinary scene-removal path.
    void NuHGobjDestroy(nuhgobj_s *object) {
        NuGScnRemove(reinterpret_cast<nugscn_s *>(object));
    }
    // Original @0x2cce60. Build bind-pose joint matrices, applying the optional
    // per-joint rotation/translation overrides before parent concatenation.
    void NuHGobjEval(nuhgobj_s *object, i32 override_count, nuhgobjjointoverride_s *overrides, NUMTX *matrices) {
        nuhgobjjointoverride_s *override_by_joint[256];
        memset(override_by_joint, 0, static_cast<usize>(object->joint_count) * sizeof(*override_by_joint));

        for (i32 i = 0; i < override_count; ++i) {
            const u8 override_index = overrides[i].joint_index;
            if (override_index < object->joint_override_map_count) {
                const u8 joint_index = object->joint_override_map[override_index];
                if (joint_index != 0xff) {
                    override_by_joint[joint_index] = &overrides[i];
                }
            }
        }

        for (i32 joint_index = 0; joint_index < object->joint_count; ++joint_index) {
            NUMTX local_matrix = object->bind_matrices[joint_index];
            nuhgobjjointoverride_s *joint_override = override_by_joint[joint_index];
            if (joint_override != NULL) {
                constexpr f32 kRadiansToNuAngle = 10430.378f;
                NUANGVEC angles = {
                    static_cast<NUANG>(joint_override->rotation_x * kRadiansToNuAngle),
                    static_cast<NUANG>(joint_override->rotation_y * kRadiansToNuAngle),
                    static_cast<NUANG>(joint_override->rotation_z * kRadiansToNuAngle),
                };
                NUMTX override_matrix;
                NuMtxSetRotateXYZVU0(&override_matrix, &angles);
                NuMtxTranslate(&override_matrix, &joint_override->translation);
                NuMtxMulVU0(&local_matrix, &local_matrix, &override_matrix);
            }

            const u8 parent_index = object->joints[joint_index].parent_index;
            if (parent_index == 0xff) {
                matrices[joint_index] = local_matrix;
            } else {
                NuMtxMulVU0(&matrices[joint_index], &local_matrix, &matrices[parent_index]);
            }
        }
    }
    void NuHGobjEvalAnim(void) {
    }
    // Original @0x2cd730.
    void NuHGobjEvalAnim2(nuhgobj_s *object, ani3_animheader_s *animation, f32 time, i32 override_count,
                          NUJOINTANIM_s *overrides, NUMTX *matrices) {
        NuHGobjEvalAnim2Root(object, animation, time, override_count, overrides, matrices, NULL, NULL);
    }
    i32 ddmaxjoints;
    void NuHGobjEvalAnim2Root(nuhgobj_s *object, ani3_animheader_s *animation, f32 time, i32 override_count,
                              NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data) {
        if (animation->magic == 0x414e4934 || animation->magic == 0x414e4935) {
            NuHGobjEvalAnim2Root_3(object, animation, time, override_count, overrides, matrices, root_fn, root_data);
            return;
        }
        if (object->joint_count > ddmaxjoints) {
            ddmaxjoints = object->joint_count;
        }
        nuanimdata2_s *legacy = reinterpret_cast<nuanimdata2_s *>(animation);
        NUVEC scales[256];
        NUJOINTANIM_s *joint_overrides[256];
        NUMTX local_matrix __attribute__((aligned(16)));
        nuanimtime_s anim_time;
        NUVEC translation, locator_translation;
        scales[255] = {1.0f, 1.0f, 1.0f};
        NuAnimData2CalcTime(legacy, time, &anim_time);
        if (override_count != 0) {
            memset(joint_overrides, 0, object->joint_count * sizeof(*joint_overrides));
            for (u8 i = 0; i < override_count; ++i) {
                if (overrides[i].joint_index < object->joint_override_map_count) {
                    u8 index = object->joint_override_map[overrides[i].joint_index];
                    if (index != 255) {
                        joint_overrides[index] = &overrides[i];
                    }
                }
            }
        }
        for (u8 i = 0; i < object->joint_count; ++i) {
            NUMTX *output = &matrices[i];
            if (i >= static_cast<i16>(legacy->node_count)) {
                *output = numtx_identity;
                continue;
            }
            NUJOINTANIM_s *override_anim = NULL;
            if (override_count != 0) {
                override_anim = joint_overrides[i];
            }
            nuanimcurve2_s *curves = &legacy->curves[i * static_cast<i16>(legacy->curve_count)];
            i8 *types = reinterpret_cast<i8 *>(legacy->curve_types) + i * static_cast<i16>(legacy->curve_count);
            u8 flags = legacy->node_flags[i];
            NUVEC *scale = &scales[i];
            if (i == 0) {
                NuAnimCurve2SetApplyToJointTransLoc(curves, types, static_cast<i8>(flags), &anim_time,
                                                    &object->joints[i], scale, &scales[object->joints[i].parent_index],
                                                    &local_matrix, override_anim, &translation, &locator_translation);
                if (root_fn != NULL) {
                    root_fn(&local_matrix, root_data, &translation, NULL, &locator_translation, 0.0f);
                }
            } else {
                NuAnimCurve2SetApplyToJoint(curves, types, static_cast<i8>(flags), &anim_time, &object->joints[i],
                                            scale, &scales[object->joints[i].parent_index], &local_matrix,
                                            override_anim);
            }
            u8 parent = object->joints[i].parent_index;
            if (parent != 255) {
                NuMtxMulVU0(output, &local_matrix, &matrices[parent]);
            } else {
                if ((flags & 0x40) != 0) {
                    *scale = {1.0f, 1.0f, 1.0f};
                }
                *output = local_matrix;
            }
        }
    }

    // Original @0x2cd150.
    void NuHGobjEvalAnim2Root_3(nuhgobj_s *object, ani3_animheader_s *animation, f32 time, i32 override_count,
                                NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data) {
        nuanimbuff_s buffer;
        NUVEC root_translation = {0.0f, 0.0f, 0.0f};
        NuAnimBuffCreateScratch(&buffer);
        NuAnimBuffAccumulate_3(&buffer, animation, time, 1, 0.0f, 0, object, NULL);
        if (override_count != 0 && JointProcAnimFn != NULL) {
            JointProcAnimFn(&buffer, object, override_count, overrides);
        }
        NuAnimBuffEvaluate_3(&buffer, object, matrices, animation, root_fn, &root_translation, root_data);
        NuAnimBuffDestroyScratch(&buffer);
    }
    void NuHGobjEvalAnimBlend(void) {
    }
    // Original @0x2ce980.
    void NuHGobjEvalAnimBlend2(nuhgobj_s *object, ani3_animheader_s *animation_a, f32 time_a,
                               ani3_animheader_s *animation_b, f32 time_b, f32 blend, i32 override_count,
                               NUJOINTANIM_s *overrides, NUMTX *matrices) {
        NuHGobjEvalAnimBlend2Root(object, animation_a, time_a, animation_b, time_b, blend, override_count, overrides,
                                  matrices, NULL, NULL);
    }
    // Original @0x2ce8e0. ANI4/ANI5 are the only accepted animation formats.
    void NuHGobjEvalAnimBlend2Root(nuhgobj_s *object, ani3_animheader_s *animation_a, f32 time_a,
                                   ani3_animheader_s *animation_b, f32 time_b, f32 blend, i32 override_count,
                                   NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data) {
        if (animation_a == NULL || (animation_a->magic != 0x414e4934 && animation_a->magic != 0x414e4935)) {
            return;
        }
        NuHGobjEvalAnimBlend2Root_3(reinterpret_cast<nugscn_s *>(object), animation_a, time_a, animation_b, time_b,
                                    blend, override_count, overrides, matrices, root_fn, root_data);
    }
    void NuHGobjEvalDwa(void) {
    }
    void **NuHGobjEvalDwa2(i32 render_count, i16 *render_indices, nuanimdata2_s *animation, f32 frame) {
        if (animation == NULL || render_count == 0) {
            return NULL;
        }

        nuanimtime_s time;
        NuAnimData2CalcTime(animation, frame, &time);

        usize clear_size;
        if (render_indices == NULL) {
            render_count = 1;
            clear_size = 1;
        } else {
            clear_size = (static_cast<usize>(render_count) * sizeof(void *) + 0xf) >> 4;
        }

        f32 **weights_by_render = NuRndrCreateBlendShapeDWAPointers(render_count);
        if (weights_by_render == NULL) {
            return NULL;
        }
        memset(weights_by_render, 0, clear_size);

        for (i32 render = 0; render < render_count; ++render) {
            i32 node = render_indices == NULL ? 0 : render_indices[render];
            if (node < 0) {
                continue;
            }
            if (node >= NuAnimNumNodes(animation)) {
                weights_by_render[render] = NuRndrCreateBlendShapeDeformerWeightsArray(0);
                continue;
            }

            i32 curve_count = static_cast<i16>(animation->curve_count);
            f32 *weights = NuRndrCreateBlendShapeDeformerWeightsArray(curve_count);
            weights_by_render[render] = weights;
            nuanimcurve2_s *curves = animation->curves + curve_count * node;
            u8 *types = animation->curve_types + curve_count * node;
            if (weights == NULL || curve_count <= 0) {
                continue;
            }
            for (i32 curve = 0; curve < curve_count; ++curve) {
                if (types[curve] == 0) {
                    weights[curve + 1] = curves[curve].data.constant;
                } else {
                    weights[curve + 1] = NuAnimCurve2CalcValEx(&curves[curve], &time, types[curve]);
                }
            }
        }
        return reinterpret_cast<void **>(weights_by_render);
    }
    struct NuLegacyDwaChunk {
        i32 node_count;
        i32 reserved_04;
        nuanimcurveset_s **curve_sets;
    };
    static inline NuLegacyDwaChunk *NuLegacyDwaGetChunk(void *animation, i32 index) {
        NuLegacyDwaChunk **chunks = *reinterpret_cast<NuLegacyDwaChunk ***>(static_cast<u8 *>(animation) + 0xc);
        return chunks[index];
    }
    void **NuHGobjEvalDwaBlend(i32 render_count, i16 *render_indices, void *animation_a, f32 frame_a, void *animation_b,
                               f32 frame_b, f32 blend) {
        bool has_a = animation_a != NULL;
        bool has_b = animation_b != NULL;
        if ((!has_a && !has_b) || render_count == 0)
            return NULL;
        nuanimtime_s time_a __attribute__((aligned(16)));
        nuanimtime_s time_b __attribute__((aligned(16)));
        if (has_a)
            NuAnimDataCalcTime(animation_a, frame_a, &time_a);
        if (has_b)
            NuAnimDataCalcTime(animation_b, frame_b, &time_b);
        f32 **weights_by_render;
        if (render_indices != NULL) {
            weights_by_render = NuRndrCreateBlendShapeDWAPointers(render_count);
            memset(weights_by_render, 0, (static_cast<usize>(render_count) * sizeof(void *) + 15) >> 4);
        } else {
            weights_by_render = NuRndrCreateBlendShapeDWAPointers(1);
            memset(weights_by_render, 0, 1);
            render_count = 1;
        }
        if (weights_by_render == NULL)
            return NULL;
        for (i32 render = 0; render < render_count; ++render) {
            i32 node = render_indices == NULL ? 0 : render_indices[render];
            if (node < 0)
                continue;
            i32 count_a = 0;
            if (has_a) {
                NuLegacyDwaChunk *chunk = NuLegacyDwaGetChunk(animation_a, time_a.chunk);
                if (node < chunk->node_count && chunk->curve_sets[node])
                    count_a = static_cast<i8>(chunk->curve_sets[node]->curve_count);
            }
            i32 count_b = 0;
            if (has_b) {
                NuLegacyDwaChunk *chunk = NuLegacyDwaGetChunk(animation_b, time_b.chunk);
                if (node < chunk->node_count && chunk->curve_sets[node])
                    count_b = static_cast<i8>(chunk->curve_sets[node]->curve_count);
            }
            i32 curve_count = count_a < count_b ? count_b : count_a;
            f32 *weights = NuRndrCreateBlendShapeDeformerWeightsArray(curve_count);
            weights_by_render[render] = weights;
            if (weights == NULL || curve_count <= 0)
                continue;
            for (i32 curve = 0; curve < curve_count; ++curve) {
                f32 value_a = 0.0f;
                if (curve < count_a) {
                    nuanimcurveset_s *set = NuLegacyDwaGetChunk(animation_a, time_a.chunk)->curve_sets[node];
                    value_a =
                        set->curves[curve] ? NuAnimCurveCalcVal2(set->curves[curve], &time_a) : set->constants[curve];
                }
                f32 value_b = 0.0f;
                if (curve < count_b) {
                    nuanimcurveset_s *set = NuLegacyDwaGetChunk(animation_b, time_b.chunk)->curve_sets[node];
                    value_b =
                        set->curves[curve] ? NuAnimCurveCalcVal2(set->curves[curve], &time_b) : set->constants[curve];
                }
                weights[curve + 1] = value_b * blend + (1.0f - blend) * value_a;
            }
        }
        return reinterpret_cast<void **>(weights_by_render);
    }
    void **NuHGobjEvalDwaBlend2(i32 render_count, i16 *render_indices, nuanimdata2_s *animation_a, f32 frame_a,
                                nuanimdata2_s *animation_b, f32 frame_b, f32 blend) {
        const bool has_a = animation_a != NULL;
        const bool has_b = animation_b != NULL;
        if ((!has_a && !has_b) || render_count == 0) {
            return NULL;
        }

        nuanimtime_s time_a;
        nuanimtime_s time_b;
        if (has_a) {
            NuAnimData2CalcTime(animation_a, frame_a, &time_a);
        }
        if (has_b) {
            NuAnimData2CalcTime(animation_b, frame_b, &time_b);
        }

        usize clear_size;
        if (render_indices == NULL) {
            render_count = 1;
            clear_size = 1;
        } else {
            clear_size = (static_cast<usize>(render_count) * sizeof(void *) + 0xf) >> 4;
        }
        f32 **weights_by_render = NuRndrCreateBlendShapeDWAPointers(render_count);
        if (weights_by_render == NULL) {
            return NULL;
        }
        memset(weights_by_render, 0, clear_size);

        for (i32 render = 0; render < render_count; ++render) {
            i32 node = render_indices == NULL ? 0 : render_indices[render];
            if (node < 0) {
                continue;
            }

            i32 count_a = 0;
            nuanimcurve2_s *curves_a = NULL;
            u8 *types_a = NULL;
            if (has_a && node < NuAnimNumNodes(animation_a)) {
                count_a = static_cast<i16>(animation_a->curve_count);
                curves_a = animation_a->curves + count_a * node;
                types_a = animation_a->curve_types + count_a * node;
            }

            i32 count_b = 0;
            nuanimcurve2_s *curves_b = NULL;
            u8 *types_b = NULL;
            if (has_b && node < NuAnimNumNodes(animation_b)) {
                count_b = static_cast<i16>(animation_b->curve_count);
                curves_b = animation_b->curves + count_b * node;
                types_b = animation_b->curve_types + count_b * node;
            }

            i32 curve_count = count_a < count_b ? count_b : count_a;
            f32 *weights = NuRndrCreateBlendShapeDeformerWeightsArray(curve_count);
            weights_by_render[render] = weights;
            if (weights == NULL || curve_count <= 0) {
                continue;
            }

            for (i32 curve = 0; curve < curve_count; ++curve) {
                f32 value_a = 0.0f;
                if (curve < count_a) {
                    value_a = types_a[curve] == 0 ? curves_a[curve].data.constant
                                                  : NuAnimCurve2CalcValEx(&curves_a[curve], &time_a, types_a[curve]);
                }
                f32 value_b = 0.0f;
                if (curve < count_b) {
                    value_b = types_b[curve] == 0 ? curves_b[curve].data.constant
                                                  : NuAnimCurve2CalcValEx(&curves_b[curve], &time_b, types_b[curve]);
                }
                weights[curve + 1] = value_b * blend + (1.0f - blend) * value_a;
            }
        }
        return reinterpret_cast<void **>(weights_by_render);
    }
    i32 NuHGobjForceShadowsOnCharacters(i32 enabled) {
        i32 previous = nuapi.force_shadows_on_characters;
        nuapi.force_shadows_on_characters = enabled;
        return previous;
    }
    void NuHGobjFromVideoMem(NUHGOBJVIDEOMEMFN callback) {
        video_mem_to_hgobj = callback;
    }
    nuhgobjpoi_s *NuHGobjGetPOI(nuhgobj_s *object, i32 index) {
        const u8 mapped_index = static_cast<u8>(index);
        if (mapped_index >= object->point_of_interest_count) {
            return NULL;
        }
        const u8 point_index = object->point_of_interest_map[mapped_index];
        if (point_index == 0xff) {
            return NULL;
        }
        return &object->points_of_interest[point_index];
    }
    void NuHGobjJointMtx(nuhgobj_s *object, u8 index, NUMTX *world_matrix,
                         NUMTX *joint_matrices, NUMTX *result) {
        u8 joint_index = object->joint_override_map[index];
        NuMtxMulVU0(result, &joint_matrices[joint_index], world_matrix);
    }
    void NuHGobjPOILocalMtxFromIX(nuhgobj_s *object, u8 index, NUMTX *joint_matrices, NUMTX *result) {
        nuhgobjpoi_s *point = &object->points_of_interest[index];
        if (point->joint_index == 0xff) {
            *result = point->local_matrix;
        } else {
            NuMtxMulVU0(result, &point->local_matrix, &joint_matrices[point->joint_index]);
        }
    }
    void NuHGobjPOIMtx(nuhgobj_s *object, u8 index, NUMTX *world_matrix, NUMTX *joint_matrices, NUMTX *result) {
        nuhgobjpoi_s *point = &object->points_of_interest[object->point_of_interest_map[index]];
        NuMtxMulVU0(result, &point->local_matrix, &joint_matrices[point->joint_index]);
        NuMtxMulVU0(result, result, world_matrix);
    }
    void NuHGobjPOIMtxFromIX(void) {
    }
    i32 NuHGobjReversibleCharacters(i32 enabled) {
        i32 previous = nuapi.reversible_characters;
        nuapi.reversible_characters = enabled;
        return previous;
    }
    i32 NuHGobjRndr(nuhgobj_s *object, NUMTX *world_matrix, i32 render_count, i16 *render_indices) {
        NUMTX joint_matrices[256];
        NuHGobjEval(object, 0, NULL, joint_matrices);
        return NuHGobjRndrMtxDwa(object, world_matrix, render_count, render_indices,
                                 joint_matrices, NULL, 0);
    }
    // Original @0x2f56a0. Draw rigid hierarchy pieces at their evaluated joint
    // matrices, then build skin matrices for the smooth hierarchy pieces.
    i32 NuHGobjRndrMtxDwa(nuhgobj_s *object, NUMTX *world_matrix, i32 render_count, i16 *render_indices,
                          NUMTX *joint_matrices, void **blend_values, i32) {
        i32 clip_state = nuspecial_clip_state;
        if (clip_state == -1) {
            clip_state = NuCameraClipHGobj(reinterpret_cast<nugscn_s *>(object), world_matrix, joint_matrices);
        }

        i32 shadow_clip = 0;
        if (NuSpecialHasActiveShadowLights() != 0) {
            NUVEC bounds_min;
            NUVEC bounds_max;
            NuVecMtxTransform(&bounds_min, &object->bounds_min, world_matrix);
            NuVecMtxTransform(&bounds_max, &object->bounds_max, world_matrix);
            shadow_clip = NuSpecialClipTestShadowLights(&bounds_min, &bounds_max, 1);
        }
        if ((clip_state | shadow_clip) == 0) {
            NuSpecialClearShadowClipTestResults();
            return 0;
        }

        const i32 previous_clip_state = NuSpecialSetClipping(1, clip_state);
        i32 drawn = 0;

        for (i32 render_index = 0; render_index < render_count; ++render_index) {
            const i32 part_index = render_indices != NULL ? render_indices[render_index] : 0;
            if (part_index < 0 || part_index >= object->render_count) {
                continue;
            }

            nuhgobjrender_s &part = object->render_parts[part_index];
            NUMTX *skin_matrices = NULL;
            if (part.smooth_skin_special != NULL || part.alternate_smooth_skin_special != NULL) {
                display_list_buffer->addr = ALIGN(display_list_buffer->addr, 0x80);
                skin_matrices = reinterpret_cast<NUMTX *>(display_list_buffer->addr);
                display_list_buffer->addr += static_cast<usize>(object->joint_count) * sizeof(NUMTX);
            }

            void *blend_value = blend_values != NULL ? blend_values[render_index] : NULL;

            if (part.rigid_specials != NULL) {
                for (i32 joint_index = 0; joint_index < object->joint_count; ++joint_index) {
                    if (part.rigid_specials[joint_index] != NULL) {
                        NUMTX draw_matrix;
                        NuMtxMulVU0(&draw_matrix, &joint_matrices[joint_index], world_matrix);
                        drawn |= NuSpecialDrawAt(part.rigid_specials[joint_index], &draw_matrix);
                    }
                }
            }

            if (part.alternate_rigid_specials != NULL) {
                for (i32 joint_index = 0; joint_index < object->joint_count; ++joint_index) {
                    if (part.alternate_rigid_specials[joint_index] != NULL) {
                        NUMTX draw_matrix;
                        NuMtxMulVU0(&draw_matrix, &joint_matrices[joint_index], world_matrix);
                        drawn |= NuSpecialDrawAt(part.alternate_rigid_specials[joint_index], &draw_matrix);
                    }
                }
            }

            if (part.smooth_skin_special != NULL || part.alternate_smooth_skin_special != NULL) {
                for (i32 joint_index = 0; joint_index < object->joint_count; ++joint_index) {
                    NuMtxMulH(&skin_matrices[joint_index], &object->inverse_bind_matrices[joint_index],
                              &joint_matrices[joint_index]);
                }
                if (part.smooth_skin_special != NULL) {
                    drawn |= NuSpecialDrawSmoothSkinDwa(part.smooth_skin_special, skin_matrices, world_matrix,
                                                        static_cast<DEFORMERWEIGHTSARRAY *>(blend_value));
                }
                if (part.alternate_smooth_skin_special != NULL) {
                    drawn |= NuSpecialDrawSmoothSkinDwa(part.alternate_smooth_skin_special, skin_matrices, world_matrix,
                                                        static_cast<DEFORMERWEIGHTSARRAY *>(blend_value));
                }
            }
        }

        NuSpecialSetClipping(1, previous_clip_state);
        NuSpecialClearShadowClipTestResults();
        return drawn;
    }
    u32 NuWindRand(void);
    i32 NuHGobjRndrRandShadowSurfacePoints(nuhgobj_s *object, NUMTX *world_matrix, NUMTX *joint_matrices, i32 count,
                                           NUVEC *positions, i32 exclusion_mask) {
        if (NuCameraClipTestExtents(&object->bounds_min, &object->bounds_max, world_matrix, 0.0f, 0) == 0) {
            return 0;
        }
        nuhgobjshadowgroup_s *groups = object->shadow_groups;
        if (groups == NULL || object->suppress_shadow_surface_points != 0) {
            return 1;
        }
        i32 total = 0;
        for (nuhgobjshadowgroup_s *group = groups; group->joint_index != 0xff; ++group) {
            for (i32 i = 0; i < group->ellipse_count; ++i) {
                if ((*reinterpret_cast<u32 *>(&group->ellipses[i][4]) & 1) == 0) {
                    ++total;
                }
            }
            for (i32 i = 0; i < group->cylinder_count; ++i) {
                if ((*reinterpret_cast<u32 *>(&group->cylinders[i][4]) & 1) == 0) {
                    ++total;
                }
            }
        }
        if (total == 0) {
            return 1;
        }
        i16 counts[256];
        for (i32 i = 0; i < total; ++i) {
            counts[i] = 0;
        }
        for (i32 i = 0; i < count; ++i) {
            ++counts[(static_cast<i32>(NuWindRand()) >> 8) % total];
        }
        if (count > 0) {
            groups = object->shadow_groups;
        }
        i32 index = 0;
        NUMTX combined;
        NUVEC4 point;
        for (nuhgobjshadowgroup_s *group = groups; group->joint_index != 0xff; ++group) {
            if (count == 0) {
                continue;
            }
            i32 matrix_ready = 0;
            for (i32 i = 0; i < group->ellipse_count; ++i) {
                if (exclusion_mask != 0 && (*reinterpret_cast<u32 *>(&group->ellipses[i][4]) & exclusion_mask) != 0) {
                    continue;
                }
                i32 n = counts[index];
                if (n != 0) {
                    if (matrix_ready == 0) {
                        NuMtxMulVU0(&combined, &joint_matrices[group->joint_index], world_matrix);
                    }
                    for (i32 j = 0; j < n; ++j) {
                        NuRndrCalcRandEllipsePos(&point, &combined, reinterpret_cast<NUVEC *>(group->ellipses[i]));
                        --count;
                        positions[count].x = point.x;
                        positions[count].y = point.y;
                        positions[count].z = point.z;
                    }
                    matrix_ready = 1;
                }
                ++index;
            }
            for (i32 i = 0; i < group->cylinder_count; ++i) {
                // Original 0x2cfcf7 uses the ellipse table for this filter too.
                if (exclusion_mask != 0 && (*reinterpret_cast<u32 *>(&group->ellipses[i][4]) & exclusion_mask) != 0) {
                    continue;
                }
                i32 n = counts[index];
                if (n != 0) {
                    if (matrix_ready == 0) {
                        NuMtxMulVU0(&combined, &joint_matrices[group->joint_index], world_matrix);
                    }
                    for (i32 j = 0; j < n; ++j) {
                        NuRndrCalcRandCylinderPos(&point, &combined, reinterpret_cast<NUVEC *>(group->cylinders[i]));
                        --count;
                        positions[count].x = point.x;
                        positions[count].y = point.y;
                        positions[count].z = point.z;
                    }
                    matrix_ready = 1;
                }
                ++index;
            }
        }
        return 1;
    }
    void NuHGobjSetClippingRootTrackerOverride(i32 enabled) {
        CutSceneBoundingBoxTrackRoot = static_cast<u8>(enabled);
    }
    void NuHGobjToVideoMem(NUHGOBJVIDEOMEMFN callback) {
        hgobj_to_video_mem = callback;
    }
    void NuGCutCharAnimProcess(NUGCUTCHAR_s *character, f32 frame, NUMTX *matrix, i32 *visible, u32 *animation_index,
                               f32 *animation_rate, f32 *blend_time, f32 *animation_start_frame, i32 *layer_mask) {
        nuanimdata2_s *animation = character->animation;
        if (animation != NULL) {
            const u32 magic = *reinterpret_cast<u32 *>(animation);
            if (magic == ANI3_MAGIC_VERSION_4 || magic == ANI3_MAGIC_VERSION_5) {
                NuGCutCharAnimProcess_3(character, frame, matrix, visible, animation_index, animation_rate, blend_time,
                                        animation_start_frame, layer_mask);
                return;
            }

            nuanimtime_s time;
            NuAnimData2CalcTime(animation, frame, &time);
            animation = character->animation;
            nuanimcurve2_s *curves = animation->curves;
            i8 *types = reinterpret_cast<i8 *>(animation->curve_types);
            const u8 node_flags = animation->node_flags[0];
#define NUGCUT_CURVE_VALUE(curve)                                                                                      \
    (types[curve] == 0 ? curves[curve].data.constant : NuAnimCurve2CalcValEx(&curves[curve], &time, types[curve]))

            *visible = static_cast<i16>(character->animation->curve_count) < 7
                           ? character->flags & 1
                           : static_cast<i32>(NUGCUT_CURVE_VALUE(6));
            if (animation_index != NULL) {
                if (static_cast<i16>(character->animation->curve_count) < 8) {
                    *animation_index = character->animation_index;
                } else {
                    const f32 value = NUGCUT_CURVE_VALUE(7);
                    *animation_index = value < 0.0f ? 0xff : static_cast<i32>(value);
                }
            }
            if (animation_start_frame != NULL) {
                if (animation_index != NULL && *animation_index != 0 && *animation_index != 0xff) {
                    *animation_start_frame = static_cast<i16>(character->animation->curve_count) < 11
                                                 ? static_cast<f32>(character->animation_start_frame)
                                                 : NUGCUT_CURVE_VALUE(10);
                } else {
                    *animation_start_frame = 0.0f;
                }
            }
            if (*visible == 0) {
                return;
            }
            if (layer_mask != NULL) {
                *layer_mask = static_cast<i16>(character->animation->curve_count) < 12
                                  ? -1
                                  : static_cast<i32>(NUGCUT_CURVE_VALUE(11));
            }

            if ((node_flags & NUANIM_NODE_HAS_ROTATION) != 0) {
                const f32 rx = NUGCUT_CURVE_VALUE(3);
                const f32 ry = NUGCUT_CURVE_VALUE(4);
                const f32 rz = NUGCUT_CURVE_VALUE(5);
                NUANGVEC rotation = {
                    static_cast<NUANG>(rx * 10430.378f),
                    static_cast<NUANG>(ry * 10430.378f),
                    static_cast<NUANG>(rz * 10430.378f),
                };
                NuMtxSetRotateXYZ(matrix, &rotation);
            } else {
                NuMtxSetIdentity(matrix);
            }
            NUVEC translation = {NUGCUT_CURVE_VALUE(0), NUGCUT_CURVE_VALUE(1), NUGCUT_CURVE_VALUE(2)};
            NuMtxTranslate(matrix, &translation);
            matrix->m02 = -matrix->m02;
            matrix->m12 = -matrix->m12;
            matrix->m20 = -matrix->m20;
            matrix->m21 = -matrix->m21;
            matrix->m23 = -matrix->m23;
            matrix->m32 = -matrix->m32;
            NUVEC scale;
            scale = NuMtxGetScale(&character->base_matrix);
            NuMtxPreScale(matrix, &scale);
            if (animation_rate != NULL) {
                *animation_rate = static_cast<i16>(character->animation->curve_count) < 10 ? character->animation_rate
                                                                                           : NUGCUT_CURVE_VALUE(9);
            }
            if (blend_time != NULL) {
                *blend_time = static_cast<i16>(character->animation->curve_count) < 9
                                  ? static_cast<f32>(character->blend_time)
                                  : NUGCUT_CURVE_VALUE(8);
            }
        } else {
            *visible = character->flags & 1;
            if (animation_index != NULL) {
                *animation_index = character->animation_index;
            }
            *matrix = character->base_matrix;
            if (animation_rate != NULL) {
                *animation_rate = character->animation_rate;
            }
            if (blend_time != NULL) {
                *blend_time = static_cast<f32>(character->blend_time);
            }
        }
    }
#undef NUGCUT_CURVE_VALUE
    void NuGCutSceneDestroy(NUGCUTSCENE_s *cutscene) {
        if (cutscene->character_system != NULL && NuCutSceneDestroyCharacters != NULL) {
            NuCutSceneDestroyCharacters(cutscene);
        }
    }
    void NuGCutSceneLoadAddr(void) {
    }
    void NuGCutSceneSysInit(NUGCUTLOCATORFNENTRY_s *locator_functions) {
        locatorfns = locator_functions;
    }
    void NuGCutSetCutAudioStream(i32 stream) {
        NuGCutAudioStream = stream;
    }
    void NuGSceneSetCrossFade(void) {
    }
    void NuGHGRelocate(void) {
    }

    // ---------------------------------------------------------------------------
    // Input / pad / key / mouse
    // ---------------------------------------------------------------------------

    struct NUKEYCHAR {
        u32 key;
        u8 normal;
        u8 shifted;
        u8 padding[2];
    };
    NUKEYCHAR NuKeyChar[65] = {
        {0x2, 0x31, 0x21, {0, 0}},  {0x3, 0x32, 0x22, {0, 0}},  {0x4, 0x33, 0xa3, {0, 0}},  {0x5, 0x34, 0x24, {0, 0}},
        {0x6, 0x35, 0x25, {0, 0}},  {0x7, 0x36, 0x5e, {0, 0}},  {0x8, 0x37, 0x26, {0, 0}},  {0x9, 0x38, 0x2a, {0, 0}},
        {0xa, 0x39, 0x28, {0, 0}},  {0xb, 0x30, 0x29, {0, 0}},  {0xc, 0x2d, 0x5f, {0, 0}},  {0xd, 0x3d, 0x2b, {0, 0}},
        {0xf, 0x09, 0x09, {0, 0}},  {0x10, 0x71, 0x51, {0, 0}}, {0x11, 0x77, 0x57, {0, 0}}, {0x12, 0x65, 0x45, {0, 0}},
        {0x13, 0x72, 0x52, {0, 0}}, {0x14, 0x74, 0x54, {0, 0}}, {0x15, 0x79, 0x59, {0, 0}}, {0x16, 0x75, 0x55, {0, 0}},
        {0x17, 0x69, 0x49, {0, 0}}, {0x18, 0x6f, 0x4f, {0, 0}}, {0x19, 0x70, 0x50, {0, 0}}, {0x1a, 0x5b, 0x7b, {0, 0}},
        {0x1b, 0x5d, 0x7d, {0, 0}}, {0x1e, 0x61, 0x41, {0, 0}}, {0x1f, 0x73, 0x53, {0, 0}}, {0x20, 0x64, 0x44, {0, 0}},
        {0x21, 0x66, 0x46, {0, 0}}, {0x22, 0x67, 0x47, {0, 0}}, {0x23, 0x68, 0x48, {0, 0}}, {0x24, 0x6a, 0x4a, {0, 0}},
        {0x25, 0x6b, 0x4b, {0, 0}}, {0x26, 0x6c, 0x4c, {0, 0}}, {0x27, 0x3b, 0x3a, {0, 0}}, {0x28, 0x27, 0x40, {0, 0}},
        {0x2b, 0x5c, 0x3f, {0, 0}}, {0x2c, 0x7a, 0x5a, {0, 0}}, {0x2d, 0x78, 0x58, {0, 0}}, {0x2e, 0x63, 0x43, {0, 0}},
        {0x2f, 0x76, 0x56, {0, 0}}, {0x30, 0x62, 0x42, {0, 0}}, {0x31, 0x6e, 0x4e, {0, 0}}, {0x32, 0x6d, 0x4d, {0, 0}},
        {0x33, 0x2c, 0x3c, {0, 0}}, {0x34, 0x2e, 0x3e, {0, 0}}, {0x35, 0x2f, 0x00, {0, 0}}, {0x37, 0x2a, 0x2a, {0, 0}},
        {0x39, 0x20, 0x20, {0, 0}}, {0x47, 0x37, 0x37, {0, 0}}, {0x48, 0x38, 0x38, {0, 0}}, {0x49, 0x39, 0x39, {0, 0}},
        {0x4a, 0x2d, 0x2d, {0, 0}}, {0x4b, 0x34, 0x34, {0, 0}}, {0x4c, 0x35, 0x35, {0, 0}}, {0x4d, 0x36, 0x36, {0, 0}},
        {0x4e, 0x2b, 0x2b, {0, 0}}, {0x4f, 0x31, 0x31, {0, 0}}, {0x50, 0x32, 0x32, {0, 0}}, {0x51, 0x33, 0x33, {0, 0}},
        {0x52, 0x30, 0x30, {0, 0}}, {0x53, 0x2e, 0x2e, {0, 0}}, {0x8d, 0x3d, 0x3d, {0, 0}}, {0xb5, 0x2f, 0x2f, {0, 0}},
        {0x0, 0x00, 0x00, {0, 0}},
    };
    i32 NuKeyToAscii(u32 key, i32 shifted) {
        for (NUKEYCHAR *entry = NuKeyChar; entry->key != 0; ++entry) {
            if (entry->key == key)
                return shifted ? entry->shifted : entry->normal;
        }
        return 0;
    }
    void NuPad_Interface_TouchScreenInput(i32, i32, i32, i32, i32, i32, i32, i32) {
    }
    i32 NuPs2ApplyDeadZone(i32 raw_value, i32 dead_zone) {
        i32 value = raw_value - 128;
        if (value > 0) {
            if (value < dead_zone) {
                value = 0;
            } else {
                value = (value - dead_zone) * 255 / (255 - dead_zone);
            }
        } else if (value > -dead_zone) {
            value = 0;
        } else {
            value = (value + dead_zone) * 255 / (255 - dead_zone);
        }
        return value;
    }
    void NuPs2VideoScreenDump(void) {
    }
    // ---------------------------------------------------------------------------
    // Culling / visibility / portals / occlusion
    // ---------------------------------------------------------------------------

    i32 clipTestSphere(NUPORTALSPHERE *sphere, NUFRUSTRUM *frustum);

    i32 NuPortalClipTest(NUGSCN *scene, NUVEC *position, f32 radius, i16 room_id) {
        if (scene->max_portals == 0 || scene->camera_room == room_id) {
            return 1;
        }
        for (i32 i = 0; i < scene->num_portal_frusta; ++i) {
            NUFRUSTRUM *frustum = scene->portal_frusta[i];
            if (frustum != NULL && frustum->room_id == room_id) {
                NUPORTALSPHERE sphere = {*position, radius};
                i32 result = clipTestSphere(&sphere, frustum);
                if (result != 0) {
                    return result;
                }
            }
        }
        return 0;
    }
    i32 NuPortalEnabled(i32 enabled) {
        const i32 previous = portals_enabled;
        portals_enabled = enabled;
        return previous;
    }
    i32 NuPortalNumRooms(NUGSCN *scene) {
        return scene != NULL ? static_cast<u16>(scene->num_rooms) : 0;
    }
    void NuPortalResetActive(NUGSCN *scene) {
        for (u32 i = 0; i < scene->max_portals; ++i) {
            scene->portals[i].is_active |= NUPORTAL_FLAG_ACTIVE | NUPORTAL_FLAG_DEFAULT_ACTIVE;
        }
    }
    i32 NuPortalRoomClipTest(NUGSCN *scene, i16 room_id) {
        if (scene == NULL || scene->max_portals == 0) {
            return 1;
        }
        if (scene->num_portal_frusta <= 0) {
            return 0;
        }
        for (i32 i = 0; i < scene->num_portal_frusta; ++i) {
            if (scene->portal_frusta[i]->room_id == room_id) {
                return 1;
            }
        }
        return 0;
    }
    void NuPortalRoomClipTestAll(NUGSCN *scene, u8 *room_visibility) {
        if (scene == NULL || scene->max_portals == 0) {
            return;
        }
        for (i32 i = 0; i < scene->num_rooms; ++i) {
            room_visibility[i] = 0;
        }
        room_visibility[scene->camera_room] = 1;
        for (i32 i = 0; i < scene->num_portal_frusta; ++i) {
            const i16 room_id = scene->portal_frusta[i]->room_id;
            if (room_id >= 0) {
                room_visibility[room_id] = 1;
            }
        }
    }
    i32 NuPortalWhichRoom(NUGSCN *scene, NUVEC *position) {
        if (position == NULL || scene == NULL) {
            return -1;
        }

        i16 candidates[2] = {0, 0};
        if (scene->num_rooms <= 0) {
            return -1;
        }

        i16 candidate_count = 0;
        for (i32 room_index = 0; room_index < scene->num_rooms; ++room_index) {
            NUROOM *room = &scene->rooms[room_index];
            f32 plane_distance = 0.0f;
            i32 plane_count = room->plane_count;
            NUPLANE *plane = room->planes;
            while (plane_count != 0) {
                plane_distance = plane->a * position->x + plane->b * position->y + plane->c * position->z + plane->d;
                if (plane_distance > 0.0f) {
                    break;
                }
                ++plane;
                --plane_count;
            }
            if (plane_distance > 0.0f) {
                continue;
            }

            if (candidate_count == 2) {
                NUROOM &first = scene->rooms[candidates[0]];
                NUROOM &second = scene->rooms[candidates[1]];
                if (first.priority < second.priority) {
                    candidates[0] = static_cast<i16>(room_index);
                } else {
                    candidates[1] = static_cast<i16>(room_index);
                }
                candidate_count = 3;
                continue;
            }

            candidates[candidate_count++] = static_cast<i16>(room_index);
            if (candidate_count != 2) {
                continue;
            }

            NUROOM &first = scene->rooms[candidates[0]];
            NUROOM &second = scene->rooms[candidates[1]];
            if ((first.flags & NUROOM_FLAG_OVERLAPPING) != 0 || (second.flags & NUROOM_FLAG_OVERLAPPING) != 0) {
                continue;
            }
            break;
        }

        if (candidate_count == 1) {
            return candidates[0];
        }
        if (candidate_count == 0) {
            return -1;
        }

        NUROOM &first = scene->rooms[candidates[0]];
        NUROOM &second = scene->rooms[candidates[1]];
        for (i32 first_portal = 0; first_portal < first.portal_count; ++first_portal) {
            const i16 portal_index = first.portal_indices[first_portal];
            for (i32 second_portal = 0; second_portal < second.portal_count; ++second_portal) {
                if (second.portal_indices[second_portal] != portal_index) {
                    continue;
                }

                const NUPORTAL &portal = scene->portals[portal_index];
                const f32 side = portal.plane.a * position->x + portal.plane.b * position->y +
                                 portal.plane.c * position->z + portal.plane.d;
                return side < 0.0f ? portal.front_room : portal.back_room;
            }
        }
        return -1;
    }
    void NuVisiBoxTree(void) {
    }
    i32 VisiSysCameraLock;
    i32 LoadedOcclusionData;
    i32 do_InstTree = 1;
    i32 do_occlusion = 1;
    i32 do_visibility = 1;

    void NuVisiInstTree(void *, NUGSCN *);
    void NuVisiOcclusion(void *);

    void *NuVisiEvaluate(NUGSCN *scene, void *visibility_context) {
        void *result = NULL;
        if (visibility_context != NULL && do_visibility != 0) {
            result = &scene->visibility_result_instance_count;
            NuCameraLock(VisiSysCameraLock);
            scene->visibility_context = NULL;
            scene->instance_tree_visibility_flags = NULL;
            NUGSCN *source_scene = scene->visibility_source_scene;
            scene->visibility_result_instance_count =
                source_scene == NULL ? scene->num_instances : source_scene->num_instances;
            scene->visibility_state &= 0xea;
            LoadedOcclusionData = result != NULL;
            if (do_occlusion != 0 && scene->occlusion_data != NULL) {
                NuVisiOcclusion(result);
            }
            if (scene->portal_visibility_marker == NULL || portals_enabled == 0) {
                if (scene->instance_visibility_tree != NULL && do_InstTree != 0) {
                    scene->visibility_context = visibility_context;
                    NuVisiInstTree(result, scene);
                    scene->visibility_state |= 1;
                }
            } else {
                NuPortalVisibility(scene);
            }
            NuCameraUnlock();
        }
        return result;
    }
    void NuVisiInstTree(void *, NUGSCN *) {
    }
    void NuVisiOcclusion(void *) {
    }
    void NuVisiOctree(void) {
    }
    void NuOcclusionManagerAddOccluderOBB(const NUVEC *minimum, const NUVEC *maximum, const NUMTX *matrix) {
        g_OcclusionManager.AddOccluder(minimum, maximum, matrix);
    }
    void NuOcclusionManagerAddOccluderQuad(const NUVEC *a, const NUVEC *b, const NUVEC *c, const NUVEC *d) {
        g_OcclusionManager.AddOccluder(a, b, c, d);
    }
    void NuOcclusionManagerAddOccluderSphere(const NUVEC *center, f32 radius) {
        g_OcclusionManager.AddOccluder(center, radius);
    }
    void NuOcclusionManagerEndFrame(void) {
        g_OcclusionManager.EndFrame();
    }
    void NuOcclusionManagerInit(u32 capacity, VARIPTR *buffer, VARIPTR buffer_end) {
        g_OcclusionManager.Init(capacity, buffer, buffer_end);
    }
    bool NuOcclusionManagerIsEnabled(void) {
        return g_OcclusionManager.initialized && g_OcclusionManager.enabled;
    }
    bool NuOcclusionManagerIsInitialised(void) {
        return g_OcclusionManager.initialized;
    }
    i32 NuOcclusionManagerIsOccludedOBB(const NUVEC *minimum, const NUVEC *maximum, const NUMTX *matrix) {
        return g_OcclusionManager.IsOccludedOBB(minimum, maximum, matrix);
    }
    i32 NuOcclusionManagerIsOccludedSphere(const NUVEC *center, f32 radius) {
        return g_OcclusionManager.IsOccludedSphere(center, radius);
    }
    void NuOcclusionManagerOnCameraSet(void) {
        g_OcclusionManager.OnCameraSet();
    }
    void NuOcclusionManagerRenderStats(void) {
    }
    void NuOcclusionManagerRenderZPass(void) {
        g_OcclusionManager.RenderZPass();
    }
    void NuOcclusionManagerSetEnabled(i32 enabled) {
        g_OcclusionManager.SetEnabled(enabled != 0);
    }
    void NuOcclusionManagerSetOccluderDotProductThreshold(f32 threshold) {
        g_OcclusionManager.unknown_158 = threshold;
    }
    void NuOcclusionManagerSetOccluderScreenSpaceThreshold(f32 threshold) {
        g_OcclusionManager.unknown_15c = threshold;
    }
    void NuInvalidateClipRanges(nudldlistscene_s *scene) {
        for (i32 index = 0; index < scene->nclip_objects; ++index) {
            if (scene->lod_ranges[index] != 0.0f) {
                scene->lod_ranges[index] = FLT_MAX;
            }
            scene->far_clip_ranges[index] = FLT_MAX;
        }
    }

    // ---------------------------------------------------------------------------
    // Viewport
    // ---------------------------------------------------------------------------

    // ---------------------------------------------------------------------------
    // Strings / conversion / Unicode
    // ---------------------------------------------------------------------------

    void NuQTAddElement(void) {
    }
    void NuQTCreate(void) {
    }
    void NuQTRead(void) {
    }
    void NuQTWrite(void) {
    }

    // ---------------------------------------------------------------------------
    // Containers / lists / params
    // ---------------------------------------------------------------------------

    // ---------------------------------------------------------------------------
    // Debug / error / html / profiling
    // ---------------------------------------------------------------------------

    void NuErrorSleep(void) {
    }

    void NuHtmlHBarGraph(void) {
    }
    void NuHtmlHLineGraph(void) {
    }
    void NuHtmlVBarGraph(void) {
    }
    // Profiling timebar sets are a deferred subsystem (the real one is
    // NuTimeBarCreateSet @0x2d7450 -> CreateSetEx @0x2d73f0 -> CreateTimeBar
    // @0x2a9860). Consumers only ever hand the returned handle to the
    // NuTimeBarSlot* stubs, so NULL behaves like profiling disabled.
    void *NuTimeBarCreateSet(i32) {
        return NULL;
    }
    void NuTimeBarCreateSetEx2(void) {
    }
    void NuTimeBarDestroySet(void) {
    }
    void NuTimeBarEnable(i32 enabled) {
        NuTimeBar_EngineEnabled = enabled;
    }
    void NuTimeBarIndicateGpuFrameOut(i32 enabled) {
        NuTimeBar_GpuFrameOutEnabled = enabled;
    }
    void NuTimeBarInit(void) {
        VARIPTR unused = {};
        NuTimeBarInitEx(NULL, unused);
    }
    extern "C++" {
        static i32 NuTimeBar_PeakReset;
    }
    void NuTimeBarResetPeaks(void) {
        NuTimeBar_PeakReset = 1;
    }
    void NuTimeBarSetRender(i32) {
    }
    void NuTimeBarSetRenderHorizontal(void) {
    }
    void NuTimeBarSetScaleY(void) {
    }

    // ---------------------------------------------------------------------------
    // Time
    // ---------------------------------------------------------------------------

    void NuTimeGetSinceStartFrame(void) {
    }
    void NuTimeGetStartFrame(void) {
    }
    void NuTimeStartFrame(void) {
    }

    // ---------------------------------------------------------------------------
    // Thread / misc OS
    // ---------------------------------------------------------------------------

#ifndef ANDROID
    void NuGetCurrentThreadId(void) {
    }
#endif
#ifndef ANDROID
    void NuThreadCreate(void) {
    }
#endif
    static f32 nu2api_paused;
    void NuPause(i32 paused) {
        nu2api_paused = (f32)paused;
    }
    void NuPhoneOSMessagePost(void) {
    }
    void NuPhoneOSMessagePump(void) {
    }
    NUGCUTSCENEGETHGOBJFN NuCutSceneGetHGObj;

    void NuSetGetHGObjFromIndxFn(NUGCUTSCENEGETHGOBJFN function) {
        NuCutSceneGetHGObj = function;
    }
    void NuSetCutSceneCharacterCreateDataFn(NUGCUTSCENECHARACTERCREATEDATAFN function) {
        NuCutSceneCharacterCreateData = function;
    }
    NUGCUTSCENECHARACTERDESTROYDATAFN NuCutSceneCharacterDestroyData;

    void NuSetCutSceneCharacterDestroyDataFn(NUGCUTSCENECHARACTERDESTROYDATAFN function) {
        NuCutSceneCharacterDestroyData = function;
    }
    void NuSetCutSceneCharacterEvalFn(NUGCUTSCENECHARACTEREVALFN function) {
        NuCutSceneCharacterEval = function;
    }
    void NuSetCutSceneCharacterProcessFn(NUGCUTSCENECHARACTERPROCESSFN function) {
        NuCutSceneCharacterProcess = function;
    }
    void NuSetCutSceneCharacterReleaseFn(NUGCUTSCENECHARACTERRELEASEFN function) {
        NuCutSceneCharacterRelease = function;
    }
    void NuSetCutSceneCharacterRenderFn(NUGCUTSCENECHARACTERRENDERFN function) {
        NuCutSceneCharacterRender = function;
    }
    void (*NuCutSceneDestroyCharacters)(NUGCUTSCENE_s *);
    void NuSetCutSceneDestroyCharactersFn(void (*callback)(NUGCUTSCENE_s *)) {
        NuCutSceneDestroyCharacters = callback;
    }
    void NuSetCutSceneFindCharactersFn(NUGCUTSCENEFINDCHARACTERSFN function) {
        NuCutSceneFindCharacters = function;
    }
    void NuSetCutSceneRequestSFXFn(NUGCUTSCENEREQUESTSFXFN function) {
        NuCutSceneRequestSFX = function;
    }
    void NuSetCutSceneResetCharactersFn(NUGCUTSCENERESETCHARACTERSFN function) {
        NuCutSceneResetCharactersFn = function;
    }
    NUGCUTSCENERIGIDCOLLISIONCHECKFN NuCutSceneRigidCollisionCheck;
    void NuSetCutSceneRigidCollisionCheckFn(NUGCUTSCENERIGIDCOLLISIONCHECKFN callback) {
        NuCutSceneRigidCollisionCheck = callback;
    }
    void NuSetCutSceneRigidPostRenderFn(NUGCUTSCENERIGIDPOSTRENDERFN function) {
        NuCutSceneRigidPostRender = function;
    }
    void NuSetCutSceneSFXFixUpFn(NUGCUTSCENESFXFIXUPFN function) {
        NuCutSceneSFXFixUp = function;
    }
    void NuSetCutSceneSFXUpdateFn(NUGCUTSCENESFXUPDATEFN function) {
        NuCutSceneSFXUpdate = function;
    }

    // ---------------------------------------------------------------------------
    // Spline / online / net / other gameplay support
    // ---------------------------------------------------------------------------

    void NuSplineList(void) {
    }
    i32 NuOnlineAchievementAchievedExPS(i32 player, i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback) {
        // Preserve the original unsigned bound, including its acceptance of index 2.
        if ((u32)player > 2) {
            return 0;
        }
        return NuOnlineAchievementAchievedProfile(g_nupadMapping[player].port, achievement, callback);
    }
    i32 NuOnlineAchievementAchievedPS(i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback) {
        extern i32 g_signedinUser;
        return NuOnlineAchievementAchievedProfile(g_signedinUser, achievement, callback);
    }
    i32 NuOnlineHasPlayerDownloadedPS(u32) {
        return 0;
    }
    i32 NuOnlineHasPlayerSignedInExPS(void) {
        return 0;
    }
    extern i32 g_signedinUser;
    i32 NuOnlineHasPlayerSignedInPS(void) {
        return g_signedinUser != -1;
    }
    void NuOnlineInitPS(void) {
    }
    void NuOnlineSetContextExPS(i32 player, i32 context, i32 value) {
        if ((u32)player <= 2) {
            NuOnlineSetContextProfilePS(g_nupadMapping[player].port, context, value);
        }
    }
    void NuOnlineSetContextPS(i32 context, i32 value) {
        NuOnlineSetContextProfilePS(g_signedinUser, context, value);
    }
    void NuOnlineSetDefaultContextExPS(i32 player, i32 context, i32 value) {
        if ((u32)player <= 2) {
            NuOnlineSetDefaultContextProfilePS(g_nupadMapping[player].port, context, value);
        }
    }
    void NuOnlineSetDefaultContextPS(i32 context, i32 value) {
        NuOnlineSetDefaultContextProfilePS(g_signedinUser, context, value);
    }
    void NuOnlineSetDefaultPresenceModeExPS(i32 player, i32 mode) {
        if ((u32)player <= 2) {
            NuOnlineSetDefaultPresenceModeProfilePS(g_nupadMapping[player].port, mode);
        }
    }
    void NuOnlineSetDefaultPresenceModePS(i32 mode) {
        NuOnlineSetDefaultPresenceModeProfilePS(g_signedinUser, mode);
    }
    void NuOnlineSetPresenceModeExPS(i32 player, i32 mode) {
        if ((u32)player <= 2) {
            NuOnlineSetPresenceModeProfilePS(g_nupadMapping[player].port, mode);
        }
    }
    void NuOnlineSetPresenceModePS(i32 mode) {
        NuOnlineSetPresenceModeProfilePS(g_signedinUser, mode);
    }
    void NuOnlineSetProfilePlayer(void) {
    }
    void NuOnlineSetPropertyExPS(i32 player, i32 property, i32 size, void *data) {
        if ((u32)player <= 2) {
            NuOnlineSetPropertyProfilePS(g_nupadMapping[player].port, property, size, data);
        }
    }
    void NuOnlineSetPropertyPS(i32 property, i32 size, void *data) {
        NuOnlineSetPropertyProfilePS(g_signedinUser, property, size, data);
    }
    i32 NuOnlineSignInPlayerPS(void) {
        return 0;
    }
    void NuFmvInit(void) {
    }
    i32 NuFmvPlayV(i32 option, ...) {
        return 1;
    }
    // The original wrapper forwards eight 32-bit arguments through a tagged
    // option list. Option meanings beyond this layout remain unrecovered.
    i32 NuFmvPlay(u32 argument0, i32 enabled, u32 argument2, u32 argument3, u32 argument4, u32 argument5, u32 argument6,
                  u32 argument7) {
        return NuFmvPlayV(2, argument0, enabled != 0 ? 3 : 0, 4, argument2, 5, argument3, 6, argument4, 7, argument5, 8,
                          argument6, argument7, 1);
    }
    extern void (*nuapi_endframe_callbackfn)(void);
    void NuRegisterEndFrameCallBackFn(void (*callback)(void)) {
        nuapi_endframe_callbackfn = callback;
    }

    // ---------------------------------------------------------------------------
    // Rendering extras
    // ---------------------------------------------------------------------------

    i32 NuStreamInit(void) {
        return 0;
    }

    // ---------------------------------------------------------------------------
    // PS2 / Xbox legacy shims
    // ---------------------------------------------------------------------------

} // extern "C"

struct nupad_s;
struct nuframebuffer_s;
struct nushaderobject_s;
union variptr_u;

void Nu360ConfigureSMBSharing(char **) {
}
void NuFramebuffer360EndZPass() {
}
bool NuFramebuffer360HasZPass() {
    return false;
}
void NuFramebuffer360BeginZPass(i32) {
}
i32 NuFramebuffer360GetTileCount(nuframebuffer_s *) {
    return 0;
}
void NuShaderObject360LoadShader(nushaderobject_s *) {
}
void NuShaderObject360LoadPackFile(char *, variptr_u *, variptr_u) {
}
void NuShaderObject360UnloadShader(nushaderobject_s *) {
}

void NuLgtSetArcMatEx(i32 type, numtl_s *material, f32 u0, f32 v0, f32 u1, f32 v1) {
    if (type > 3)
        return;
    NuLgtArcMtl[type].material = material;
    NuLgtArcMtl[type].u0 = u0;
    NuLgtArcMtl[type].v0 = v0;
    NuLgtArcMtl[type].u1 = u1;
    NuLgtArcMtl[type].v1 = v1;
}

void NuLgtArcLaserEx(i32 type, NUVEC *start, NUVEC *end, NUVEC *bend, f32 width, f32 segment_length, f32 wobble,
                     f32 bend_amount, i32 colour, i32 flags) {
    i32 index = NuLgtArcLaserCnt;
    if (index >= 16)
        return;
    NULGTARCLASER *laser = &NuLgtArcLaserData[index];
    laser->type = (u8)type;
    laser->start = *start;
    laser->end = *end;
    laser->bend = *bend;
    laser->width = width;
    laser->segment_length = segment_length;
    laser->wobble = wobble;
    laser->bend_amount = bend_amount;
    u32 red = ((u32)colour & 0xff) * 2;
    u32 green = ((u32)colour & 0xff00) * 2;
    u32 blue = ((u32)colour & 0xff0000) >> 15;
    u32 alpha = ((u32)colour >> 24) * 2;
    if (red > 255)
        red = 255;
    if (green > 0xff00)
        green = 0xff00;
    if (blue > 255)
        blue = 255;
    if (alpha > 255)
        alpha = 255;
    laser->colour = blue | (red << 16) | green | (alpha << 24);
    laser->flags = flags;
    if ((NuLgtArcLaserFrame & 1) == 0 || laser->seed == 0)
        laser->seed = NuLgtRand();
    for (i32 i = 0; i < 6; ++i)
        NuLgtRand();
    ++NuLgtArcLaserCnt;
}
