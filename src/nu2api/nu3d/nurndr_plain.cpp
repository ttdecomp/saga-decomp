#include <stdio.h>
#include "nu2api/nufile/nufile.h"
#include "nu2api/nu3d/nuvport.h"
// nurndr_plain.cpp — Host "plain" renderer backend.
//
// This TU is the host replacement for the PS2 rendering TU (nurndr).
// It owns renderer scene/present paths and primitive drawing helpers:
//
//   1. Scene lifecycle  — NuRndrBeginScene / NuRndrClear / NuRndrEndScene
//      builds the current `nudisplayscene_s` (0x218 bytes in the original BSS)
//      and queues it into a 16-slot ring consumed by the render thread.
//
//   2. Primitive drawing helpers that call the immediate-mode API in
//      android/nuprim_android.c, including quad expansion there.
//
//   3. Frame present   — NuRndrSwapScreen / NuRndrSwapScreenEx.  Flushes
//      debris, swaps the display-list and stream buffers, kicks the render
//      thread, then paces the game thread until the application status
//      leaves the "running" state (or the host render fence completes).
//
// All other entry points from the original TU are retained as link stubs
// until their subsystems are transcribed.  Their signatures are not yet
// recovered, so they are left as `void(void)`.

#include <float.h>
#include <string.h>
#include "nu2api/nu3d/nuprim_internal.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nucore.hpp"
#include "globals.h"
#include "nu2api/nu3d/nutexanm.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nuprim.h"
#include "nu2api/nu3d/nuvport.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nu3d/android/nuptl_android.h"

extern "C" void NuLgtLaserDraw(i32 paused);
void NuLgtArcLaserDraw(i32 paused);

// ---------------------------------------------------------------------------
// Scene state — mirrors original BSS layout
// ---------------------------------------------------------------------------

static constexpr i32 kSceneRingCapacity = 16;

extern "C" {
    i32 PS2_REZ_W = 1280;
    i32 PS2_REZ_H = 720;
    // Scene currently being built.  Size is 0x218 bytes; original lives in BSS
    // and is referenced as a plain object by all render/present code.
    struct nudisplayscene_s currentScene = {0};

    // Deferred ring: NuRndrEndScene copies the completed scene here; the render
    // thread drains it.  Stride is 0x218, 16 slots (0x2180 bytes total) in the
    // original.
    i32 sceneParametersCount = 0;
    struct nudisplayscene_s sceneParameters[kSceneRingCapacity] = {0};

    // Shared renderer state block (original BSS @0x119b900, 0x1b0 bytes).
    NUGLOBALRNDRSTATE render_state = {};
} // extern "C"

// Swap/present pacing flags (original BSS).
volatile bool g_isBlockedInSwapScreen = false;
extern i32 rndr_blend_shape_deformer_wt_cnt;
extern i32 rndr_blend_shape_deformer_wt_ptrs_cnt;

// ---------------------------------------------------------------------------
// Immediate-mode 2D stream state
// ---------------------------------------------------------------------------

// Vertex count shared with the primitive implementation (see nuprim.h).
i32 g_NuPrim_VertexCount;

// Display-list cursor for the 2D stream.  Defined in nudlist.cpp.
extern VARIPTR *display_list_buffer;


// ---------------------------------------------------------------------------
// Forward declarations for C-visible helpers
// ---------------------------------------------------------------------------

extern "C" {
    i32 NuDisplayListAddRenderScene(void);
    i32 NuDynamicLightIsEnabled(void *);
    void NuDynamicLightAddRenderScene(void *, i32, i32);
    void RndrStateSetConstAlphaTint(i32 alpha_enabled, i32 tint_enabled, f32 alpha, const NUCOLOUR3 *tint, NUMTL *mtl);
    void DisplayListUpdateRenderState(void *list, void *state);
    void NuDisplayListLinkMtl(nudisplaylist_s *list, NUMTL *mtl);
    VARIPTR *NuDisplayListLinkItems(nudisplaylist_s *list, i32 count);
}

void NuDebrisRendererFlushBuffers(void);

extern "C" {
    void NuRndrSwapStreamBuffers(void);
    void NuRenderThreadPrepareRender(void);
    void NuRenderThreadStartRender(void);
    void NuShaderManagerBindShader(NUSHADEROBJECT *shader);
    void NuDisplayListCheckBuffer(void);
    void NuDisplayListResetBuffer(void);
    void NuRenderThreadLock(void);
    void NuRenderThreadUnlock(void);
}

// ---------------------------------------------------------------------------
// Immediate-mode 2D API
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Scene lifecycle
// ---------------------------------------------------------------------------

extern "C" i32 NuRndrBeginScene(i32 /*begin_flags*/) {
    // Clear the in-flight scene to a known baseline.  Field names below
    // still use the original unknown_* tags because the 0x218-byte layout
    // has not been fully recovered; values match the PS2 initial defaults.
    currentScene.unknown_4c = 0;
    currentScene.render_scene_id = 0xffffffff;
    currentScene.clear_flags = 0;
    currentScene.state_ptr = nullptr;
    currentScene.unknown_24 = nullptr;
    currentScene.unknown_28 = 0;
    currentScene.unknown_58 = 0;
    currentScene.unknown_38 = 0;
    currentScene.unknown_3c = 0;
    currentScene.unknown_40 = 0xffffffff;
    currentScene.unknown_48 = 0;
    currentScene.unknown_ac = 0;
    currentScene.texture_blend_enabled = 0;
    currentScene.unknown_e4 = 0;
    currentScene.unknown_e0 = 0;
    currentScene.unknown_178 = 0;
    currentScene.unknown_174 = 0;
    currentScene.unknown_188 = 0;
    currentScene.unknown_214 = 0;
    return 1;
}

extern "C" i32 NuRndrBeginSceneEx(i32 begin_flags, i32, i32) {
    return NuRndrBeginScene(begin_flags);
}

extern "C" void NuRndrClear(i32 clear_flags, i32 bg_colour, f32 alpha) {
    auto *scene = &currentScene;

    if (NuIOS_IsLowEndDevice() && g_BackgroundUsedFogColour) {
        bg_colour = g_BackgroundColour;
    }

    scene->clear_alpha = alpha;
    scene->clear_flags |= clear_flags;
    scene->bg_colour = bg_colour;
    NuVpGetPosition2(&scene->vp_x, &scene->vp_y);
    NuVpGetSize2(&scene->vp_w, &scene->vp_h);
}

extern "C" void NuRndrEndScene(void) {
    auto *scn = &currentScene;

    scn->render_scene_id = NuDisplayListAddRenderScene();
    i32 has_dynamic_light = scn->unknown_38;

    // If a dynamic light was attached to this scene, hand it to the light
    // manager now that the render-scene id is known.
    if (has_dynamic_light != 0 && scn->render_scene_id != -1) {
        if (NuDynamicLightIsEnabled(scn->unknown_3c)) {
            NuDynamicLightAddRenderScene(scn->unknown_3c, scn->unknown_40, scn->render_scene_id);
        } else {
            scn->unknown_38 = 0;
        }
        scn->render_scene_id = -1;
    }

    // Queue this scene for the render thread. processRenderScenes drains the
    // array and resets the count at the end of the frame.
    sceneParameters[sceneParametersCount++] = *scn;
}

extern "C" void NuRndrEndSceneEx(i32) {
    NuRndrEndScene();
}

// ---------------------------------------------------------------------------
// Frame present / swap
// ---------------------------------------------------------------------------

// Original 0x2967db — swap display-list and stream buffers, kick the render
// thread, then pace the game thread until the app leaves the running state.
extern "C" SAGA_HOST_WEAK i32 NuRndrSwapScreen(i32 /*mode*/) {
    NuRenderThreadLock();
    rndr_blend_shape_deformer_wt_cnt = 0x3f00;
    rndr_blend_shape_deformer_wt_ptrs_cnt = 0x800;
    NuRenderThreadPrepareRender();
    NuShaderManagerBindShader(0);
    NuDebrisRendererFlushBuffers();
    NuDisplayListSwapBuffersEndFrame();
    NuRndrSwapStreamBuffers();
    NuDisplayListSwapBuffersBeginFrame();
    NuDisplayListCheckBuffer();
    NuDisplayListResetBuffer();
    NuRenderThreadUnlock();
    NuRenderThreadStartRender();

    // Status 1 suspends presentation until the lifecycle makes the app active.
    // On Android this is released by the activity lifecycle
    // (nativeSetSurface / nativeOnPause flip NUAPPLICATIONSTATUS).
    for (;;) {
        NuApplicationState *state = NuCore::GetApplicationState();
        if (state->GetStatus() != 1) {
            break;
        }
        g_isBlockedInSwapScreen = 1;
        NuThreadSleep(1);
    }
    g_isBlockedInSwapScreen = 0;

    return 1;
}

// Original 0x296888
extern "C" i32 NuRndrSwapScreenEx(i32 mode, void (*callback)(void)) {
    if (callback != nullptr) {
        callback();
    }
    return NuRndrSwapScreen(mode);
}

// ---------------------------------------------------------------------------
// Link stubs — retained for compatibility, not yet implemented
// ---------------------------------------------------------------------------
//
// Every function below exists in the original binary.  Signatures have not
// been recovered, so they are kept as void(void) stubs.  Grouped by
// subsystem so it is obvious what is still missing.

// Scene / GScn
using NUGSCNVIDEOMEMFN = void (*)(NUGSCN *);

NUGSCNVIDEOMEMFN gscene_to_video_mem;
NUGSCNVIDEOMEMFN video_mem_to_gscene;

extern "C" void NuGScnFromVideoMem(NUGSCNVIDEOMEMFN callback) {
    video_mem_to_gscene = callback;
}
extern "C" void NuGScnReadForMultiRender(void) {
}
extern "C" void NuGScnRndr(NUGSCN *scene) {
    if (scene->additional_scenes != NULL && scene->rendered_additional_scene_count > 0) {
        NuDisplaySceneRndr(scene->additional_scenes[scene->rendered_additional_scene_count - 1]->display_list);
    } else {
        NuDisplaySceneRndr(scene->display_list);
    }
    ++scene->rendered_additional_scene_count;
}
extern "C" void NuGScnToVideoMem(NUGSCNVIDEOMEMFN callback) {
    gscene_to_video_mem = callback;
}

// Material
static u32 mtl_animation_mask = 0x00ffffff;

extern "C" void NuDisplayListAnimateMtls(f32 frame_time);

extern "C" void NuMtlAnimate(f32 frame_time) {
    NuDisplayListAnimateMtls(frame_time * mtl_animation_speed_scale);
}
extern "C" void NuMtlAnimateSetMask(i32 mask) {
    mtl_animation_mask = mask;
}
extern "C" void NuMtlAnimateSetSpeedScale(f32 speed_scale) {
    mtl_animation_speed_scale = speed_scale;
}
extern "C" void NuMtlAnimateShaderMtlTextures(void) {
}
extern "C" void NuMtlCopy(void) {
}
static void NuMtlCreate3D(void) {
}
extern "C" void NuMtlCreateBuff(void) {
}
extern "C" void NuMtlCreateBuff3D(void) {
}

extern "C" void NuMtlFindVariantMtl(void) {
}
extern "C" void NuMtlFindVariantMtlFromDesc(void) {
}
extern "C" void NuMtlRegisterForOverride(void) {
}
extern "C" void NuMtlSetRenderPlane(void) {
}
static void NuMtlSetRenderStatesPS(void) {
}
extern "C" void NuMtlSpecialSetUV(void) {
}

// Debug / visualisation geometry
static inline void NuRndrPrimSetColour(i32 colour);
static inline void NuRndrPrimPosition(f32 x, f32 y, f32 z);
static inline void NuRndrPrimUV(f32 u, f32 v);
extern "C" void NuRndr3dLine(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, i32 colour) {
    NuPrim3DBegin(2, 5, NULL, &numtx_identity);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(0.0f, 0.0f);
    NuRndrPrimPosition(x0, y0, z0);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(1.0f, 1.0f);
    NuRndrPrimPosition(x1, y1, z1);
    NuPrim3DEnd();
}
extern "C" void NuRndrAnglesZX(NUVEC *direction, NUVEC *angles) {
    NUVEC rotated;
    angles->x = (f32)NuAtan2D(direction->z, direction->y);
    NuVecRotateX(&rotated, direction, -(i32)angles->x);
    angles->z = -(f32)NuAtan2D(rotated.x, rotated.y);
}

void NuLightBurnoutEffect(i32, f32 threshold, f32 intensity, f32 flare) {
    currentScene.unknown_28 = 1;
    currentScene.burnout_intensity = intensity <= 2.0f ? (i32)(127.0f * intensity) : 255;
    currentScene.burnout_flare = flare;
    currentScene.burnout_threshold = threshold;
    currentScene.bloom.enabled = 1;
    currentScene.bloom.directional = 0;
    currentScene.bloom.unknown_18 = 0.0f;
    currentScene.bloom.near_angle = 0.0f;
    currentScene.bloom.near_scale = 0.0f;
    currentScene.bloom.far_angle = 180.0f;
    currentScene.bloom.far_scale = intensity;
    currentScene.bloom.intensity = 1.0f;
    currentScene.bloom.blur_iterations = flare;
    currentScene.bloom.blend = 0.0f;
    currentScene.bloom.threshold = threshold;
}
extern "C" void NuRndrAxes(NUMTX *matrix, f32 length) {
    NuRndrAxisBright(matrix, length, 255);
}
static inline void NuRndrPrimSetColour(i32 colour);
static inline void NuRndrPrimPosition(f32 x, f32 y, f32 z);
extern "C" void NuRndrAxisArrowsMtx(NUMTX *matrix, f32 length, NUMTL *material) {
    f32 base = length * 0.8f;
    f32 width = length * 0.1f;
    NuPrim3DBegin(2, 5, material, matrix);
    NuRndrPrimSetColour(0xff0000ff);
    NuRndrPrimPosition(0, 0, 0);
    NuRndrPrimSetColour(0xff0000ff);
    NuRndrPrimPosition(length, 0, 0);
    NuRndrPrimSetColour(0xff00ff00);
    NuRndrPrimPosition(0, 0, 0);
    NuRndrPrimSetColour(0xff00ff00);
    NuRndrPrimPosition(0, length, 0);
    NuRndrPrimSetColour(0xffff0000);
    NuRndrPrimPosition(0, 0, 0);
    NuRndrPrimSetColour(0xffff0000);
    NuRndrPrimPosition(0, 0, length);
    NuPrim3DEnd();
    NuPrim3DBegin(3, 5, material, matrix);
    NuRndrPrimSetColour(0xff0000ff);
    NuRndrPrimPosition(base, -width, 0);
    NuRndrPrimSetColour(0xff0000ff);
    NuRndrPrimPosition(length, 0, 0);
    NuRndrPrimSetColour(0xff0000ff);
    NuRndrPrimPosition(base, width, 0);
    NuPrim3DEnd();
    NuPrim3DBegin(3, 5, material, matrix);
    NuRndrPrimSetColour(0xff00ff00);
    NuRndrPrimPosition(0, base, width);
    NuRndrPrimSetColour(0xff00ff00);
    NuRndrPrimPosition(0, length, 0);
    NuRndrPrimSetColour(0xff00ff00);
    NuRndrPrimPosition(0, base, -width);
    NuPrim3DEnd();
    NuPrim3DBegin(3, 5, material, matrix);
    NuRndrPrimSetColour(0xffff0000);
    NuRndrPrimPosition(0, width, base);
    NuRndrPrimSetColour(0xffff0000);
    NuRndrPrimPosition(0, 0, length);
    NuRndrPrimSetColour(0xffff0000);
    NuRndrPrimPosition(0, -width, base);
    NuPrim3DEnd();
}
extern "C" void NuRndrAxisArrows(NUVEC *position, void *, f32 length, NUMTL *material) {
    NUMTX matrix;
    NuMtxSetIdentity(&matrix);
    NuMtxTranslate(&matrix, position);
    NuRndrAxisArrowsMtx(&matrix, length, material);
}

static inline void NuRndrPrimSetColour(i32 colour);
static inline void NuRndrPrimPosition(f32 x, f32 y, f32 z);
extern "C" void NuRndrAxisBright(NUMTX *matrix, f32 length, i32 brightness) {
    f32 base = length * 0.8f;
    f32 width = length * 0.1f;
    i32 red = 0xff000000u | (brightness & 255);
    i32 green = 0xff000000u | ((static_cast<u32>(brightness) << 8) & 0xffff);
    i32 blue = 0xff000000u | ((brightness & 255) << 16);
    NuPrim3DBegin(2, 5, NULL, matrix);
    NuRndrPrimSetColour(red);
    NuRndrPrimPosition(0, 0, 0);
    NuRndrPrimSetColour(red);
    NuRndrPrimPosition(length, 0, 0);
    NuRndrPrimSetColour(green);
    NuRndrPrimPosition(0, 0, 0);
    NuRndrPrimSetColour(green);
    NuRndrPrimPosition(0, length, 0);
    NuRndrPrimSetColour(blue);
    NuRndrPrimPosition(0, 0, 0);
    NuRndrPrimSetColour(blue);
    NuRndrPrimPosition(0, 0, length);
    NuPrim3DEnd();
    NuPrim3DBegin(3, 5, NULL, matrix);
    NuRndrPrimSetColour(red);
    NuRndrPrimPosition(base, -width, 0);
    NuRndrPrimSetColour(red);
    NuRndrPrimPosition(length, 0, 0);
    NuRndrPrimSetColour(red);
    NuRndrPrimPosition(base, width, 0);
    NuPrim3DEnd();
    NuPrim3DBegin(3, 5, NULL, matrix);
    NuRndrPrimSetColour(green);
    NuRndrPrimPosition(0, base, width);
    NuRndrPrimSetColour(green);
    NuRndrPrimPosition(0, length, 0);
    NuRndrPrimSetColour(green);
    NuRndrPrimPosition(0, base, -width);
    NuPrim3DEnd();
    NuPrim3DBegin(3, 5, NULL, matrix);
    NuRndrPrimSetColour(blue);
    NuRndrPrimPosition(0, width, base);
    NuRndrPrimSetColour(blue);
    NuRndrPrimPosition(0, 0, length);
    NuRndrPrimSetColour(blue);
    NuRndrPrimPosition(0, -width, base);
    NuPrim3DEnd();
}

static inline void NuRndrPrimSetColour(i32 colour);
static inline void NuRndrPrimPosition(f32 x, f32 y, f32 z);

extern "C" void NuRndrBoundingBox(NUVEC *minimum, NUVEC *maximum, NUMTX *matrix, i32 colour) {
    NuPrim3DBegin(3, 5, NULL, matrix);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, minimum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, maximum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, maximum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, minimum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, minimum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, minimum->y, maximum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, maximum->y, maximum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, maximum->y, maximum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, minimum->y, maximum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, minimum->y, maximum->z);
    NuPrim3DEnd();
    NuPrim3DBegin(2, 5, NULL, matrix);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, maximum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(minimum->x, maximum->y, maximum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, maximum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, maximum->y, maximum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, minimum->y, minimum->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(maximum->x, minimum->y, maximum->z);
    NuPrim3DEnd();
}
f32 circle_scale_radius = 0.5f;
static inline void NuRndrPrimSetColour(i32 colour);
static inline void NuRndrPrimUV(f32 u, f32 v);
extern "C" void NuRndrCircle(f32 x, f32 y, f32 radius, f32 aspect, i32 count, f32 u0, f32 v0, f32 u1, f32 v1,
                             i32 colour, numtl_s *material) {
    NuPrim2DBegin(0, 7, material);
    radius *= circle_scale_radius;
    u1 -= u0;
    v1 -= v0;
    f32 previous_x = radius * 0.0f * aspect + x;
    f32 previous_y = radius + y;
    f32 previous_u = 0.0f + u0;
    f32 previous_v = v1 + v0;
    f32 step = 6.284f / static_cast<f32>(count - 1);
    for (i32 i = 0; i < count; ++i) {
        f32 angle = static_cast<f32>(i) * step;
        f32 sine = NuSinf(angle);
        f32 cosine = NuCosf(angle);
        f32 next_x = sine * radius * aspect + x;
        f32 next_y = cosine * radius + y;
        f32 next_u = (0.5f * sine + 0.5f) * u1 + u0;
        f32 next_v = v1 * (0.5f - 0.5f * cosine) + v0;
        NuRndrPrimSetColour(colour);
        NuRndrPrimUV(0.5f * u1 + u0, 0.5f * v1 + v0);
        NuPrim2DAddXYZ(static_cast<f32>(PS2_VREZ_W) * x, static_cast<f32>(PS2_VREZ_H) * y, 0.0f);
        NuRndrPrimSetColour(colour);
        NuRndrPrimUV(previous_u, previous_v);
        NuPrim2DAddXYZ(static_cast<f32>(PS2_VREZ_W) * previous_x, static_cast<f32>(PS2_VREZ_H) * previous_y, 0.0f);
        NuRndrPrimSetColour(colour);
        NuRndrPrimUV(next_u, next_v);
        NuPrim2DAddXYZ(static_cast<f32>(PS2_VREZ_W) * next_x, static_cast<f32>(PS2_VREZ_H) * next_y, 0.0f);
        previous_x = next_x;
        previous_y = next_y;
        previous_u = next_u;
        previous_v = next_v;
    }
    NuPrim2DEnd();
}

extern "C" void NuRndrEndReflectionRender(void) {
    NuSpecialReflection(0);
}
extern "C" void NuRndrEndShadowReceiveRender(void) {
    global_GobjIsShadowReceive = 0;
}
extern "C" void NuRndrFx(i32 paused, void *) {
    if (NuRndrBeginSceneEx(-1, -2, 0) != 0) {
        NuLgtLaserDraw(paused);
        NuLgtArcLaserDraw(paused);
        NuRndrEndSceneEx(0);
    }
}
extern "C" i32 NuRndrGetCullDebug(void) {
    return 0;
}
extern i32 global_frame_count;
extern "C" i32 NuRndrGlobalFrameCount(void) {
    return global_frame_count;
}
extern "C" void NuRndrGlobalFrameCountPause(i32 paused) {
    global_frame_count_paused = paused;
}

// NuRndrRect2di and NuRndrGradRect2di write the attributes for the current
// vertex before handing its position to NuPrim2DAddXYZ.  The latter advances
// the stream cursor, so doing this in one helper preserves the ordering of
// the original immediate-mode implementation.
static inline u32 NuRndrPrimColour(u32 colour) {
    if (g_NuPrim_NeedsOverbrightening != 0) {
        return colour;
    }
    return (colour & 0xff000000u) | ((colour >> 1) & 0x007f7f7fu);
}

static inline void NuRndrPrimAttributes(u32 colour, bool u_one, bool v_one) {
    u8 *vertex = reinterpret_cast<u8 *>(g_NuPrim_StreamBufferPtr->addr);
    *reinterpret_cast<u32 *>(vertex + 0xc) = NuRndrPrimColour(colour);
    if (g_NuPrim_NeedsHalfUVs != 0) {
        *reinterpret_cast<u16 *>(vertex + 0x10) = u_one ? 0x3c00 : 0;
        *reinterpret_cast<u16 *>(vertex + 0x12) = v_one ? 0x3c00 : 0;
    } else {
        *reinterpret_cast<f32 *>(vertex + 0x10) = u_one ? 1.0f : 0.0f;
        *reinterpret_cast<f32 *>(vertex + 0x14) = v_one ? 1.0f : 0.0f;
    }
}

extern "C" void NuRndrGradRect2di(i32 x, i32 y, i32 w, i32 h, i32 *colour, numtl_s *mtl) {
    const f32 sx = static_cast<f32>(x) * 0.2f;
    const f32 sy = static_cast<f32>(y) * 0.2f;
    const f32 sw = static_cast<f32>(w) * 0.2f;
    const f32 sh = static_cast<f32>(h) * 0.2f;

    NuPrim2DBegin(1, 7, mtl);
    NuRndrPrimAttributes(static_cast<u32>(colour[0]), false, false);
    NuPrim2DAddXYZ(sx, sy, 0.0f);
    NuRndrPrimAttributes(static_cast<u32>(colour[1]), true, false);
    NuPrim2DAddXYZ(sx + sw, sy, 0.0f);
    NuRndrPrimAttributes(static_cast<u32>(colour[2]), false, true);
    NuPrim2DAddXYZ(sx, sy + sh, 0.0f);
    NuRndrPrimAttributes(static_cast<u32>(colour[3]), true, true);
    NuPrim2DAddXYZ(sx + sw, sy + sh, 0.0f);
    NuPrim2DEnd();
}
static inline u16 NuRndrFloatToHalf(f32 value) {
    union {
        f32 value;
        u32 bits;
    } conversion = {value};
    i32 mantissa = conversion.bits & 0x7fffff;
    i32 sign = conversion.bits >> 31;
    i32 exponent = static_cast<i32>((conversion.bits >> 23) & 0xff) - 0x70;
    u16 half_exponent = 0;
    if (exponent >= 0) {
        half_exponent = 0x7c00;
        if (exponent < 0x20) {
            half_exponent = static_cast<u16>(exponent * 0x400);
        }
    }
    return static_cast<u16>(mantissa >> 13) | static_cast<u16>(sign << 15) | half_exponent;
}

static inline void NuRndrPrimUV(f32 u, f32 v) {
    u8 *vertex = reinterpret_cast<u8 *>(g_NuPrim_StreamBufferPtr->addr);
    if (g_NuPrim_NeedsHalfUVs != 0) {
        *reinterpret_cast<u16 *>(vertex + 0x10) = NuRndrFloatToHalf(u);
        *reinterpret_cast<u16 *>(vertex + 0x12) = NuRndrFloatToHalf(v);
    } else {
        *reinterpret_cast<f32 *>(vertex + 0x10) = u;
        *reinterpret_cast<f32 *>(vertex + 0x14) = v;
    }
}

static inline void NuRndrPrimSetColour(i32 colour) {
    if (g_NuPrim_NeedsOverbrightening)
        ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color = colour;
    else
        ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color =
            ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
}

extern "C" void NuRndrGradRectUV2di(i32 x, i32 y, i32 w, i32 h, f32 u0, f32 v0, f32 u1, f32 v1, u32 *colours,
                                    numtl_s *mtl) {
    const f32 sx = static_cast<f32>(x) * 0.0625f;
    const f32 sy = static_cast<f32>(y) * 0.0625f;
    const f32 width = static_cast<f32>(w) * 0.0625f;
    const f32 height = static_cast<f32>(h) * 0.0625f;

    NuPrim2DBegin(1, 7, mtl);
    NuRndrPrimSetColour(colours[0]);
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = u0;
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x14) = v0;
    } else {
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = NuRndrFloatToHalf(u0);
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x12) = NuRndrFloatToHalf(v0);
    }
    NuPrim2DAddXYZ(sx, sy, 0.0f);
    NuRndrPrimSetColour(colours[1]);
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = u1;
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x14) = v0;
    } else {
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = NuRndrFloatToHalf(u1);
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x12) = NuRndrFloatToHalf(v0);
    }
    const f32 ex = sx + width;
    NuPrim2DAddXYZ(ex, sy, 0.0f);
    NuRndrPrimSetColour(colours[2]);
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = u0;
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x14) = v1;
    } else {
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = NuRndrFloatToHalf(u0);
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x12) = NuRndrFloatToHalf(v1);
    }
    const f32 ey = sy + height;
    NuPrim2DAddXYZ(sx, ey, 0.0f);
    NuRndrPrimSetColour(colours[3]);
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = u1;
        *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x14) = v1;
    } else {
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = NuRndrFloatToHalf(u1);
        *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x12) = NuRndrFloatToHalf(v1);
    }
    NuPrim2DAddXYZ(ex, ey, 0.0f);
    NuPrim2DEnd();
}
static i32 dump_state = -1;
struct ScreenGrabTileParams {
    i8 channels[4];
    u16 width;
};
extern "C" void NuRndrScreenGrabTileInit(void *, i32, f32, f32, f32);
extern "C" void NuRndrScreenGrabTileDeInit(void *);
extern "C" void NuRndrScreenGrabTileBegin(void **);
extern "C" void NuRndrScreenGrabTileEnd(void **);
extern "C" i32 NuRndrDoingScreenGrab;
extern "C" void NuVpResetRegions();
extern "C" i32 NuRndrHighResScreenGrab(char *prefix, f32 scale, f32 a, f32 b, f32 c, i32 number) {
    static f32 yTiles, xTiles, yPos, xPos;
    static ScreenGrabTileParams params;
    static NUFILE fh;
    static i32 header_size, delay, xOffsetHack;
    void *tile = NULL;
    f32 remainder = NuFmod(scale, 0.5f);
    scale -= remainder;
    if (remainder > 0.25f)
        scale += 0.5f;
    f32 tiles;
    if (scale < 0.5f) {
        scale = 0.5f;
        tiles = 1.0f;
    } else if (scale > 4.0f) {
        scale = 4.0f;
        tiles = 6.0f;
    } else {
        f32 n = scale / 0.75f;
        tiles = static_cast<f32>(static_cast<i32>(n));
        if (n > tiles)
            tiles += 1.0f;
    }
    yTiles = tiles;
    xTiles = tiles;
    if (dump_state == -1) {
        NuRndrDoingScreenGrab = 1;
        NuRndrScreenGrabTileInit(&params, static_cast<i32>(tiles * tiles), b, a, c);
        char path[256];
        if (number >= 0)
            sprintf(path, "%s%d.bmp", prefix, number);
        else {
            sprintf(path, "%s.bmp", prefix);
            i32 index = 0;
            while (NuFileSize(path) > 0)
                sprintf(path, "%s%03d.bmp", prefix, index++);
        }
        struct __attribute__((packed)) BitmapHeader {
            u16 type;
            u32 file_size;
            u16 reserved1, reserved2;
            u32 offset;
            u32 info_size, width, height;
            u16 planes, bits;
            u32 compression, image_size, xppm, yppm, colours, important;
        } header;
        header.type = 0x4d42;
        header.file_size = PS2_REZ_W * 12 * PS2_REZ_H + 56;
        header.reserved1 = header.reserved2 = 0;
        header.offset = 56;
        header.info_size = 40;
        header.width = static_cast<u32>(static_cast<f32>(PS2_REZ_W) * scale);
        header.height = static_cast<u32>(static_cast<f32>(PS2_REZ_H) * scale);
        header.planes = 1;
        header.bits = 24;
        header.compression = 0;
        header.image_size = header.width * header.height * 3;
        header.xppm = header.yppm = 1;
        header.colours = header.important = 0;
        fh = NuFileOpen(path, static_cast<NUFILEMODE>(1));
        if (fh != 0) {
            NuFileWrite(fh, &header, 16);
            NuFileWrite(fh, &header.info_size, 40);
            NuFileSeek(fh, static_cast<u32>(header.image_size - 1), static_cast<NUFILESEEK>(1));
            u8 padding = 0;
            NuFileWrite(fh, &padding, 1);
            dump_state = 0;
            header_size = header.offset;
            delay = 3;
            xOffsetHack = 0;
            xPos = yPos = 0.0f;
            f32 w = static_cast<f32>(PS2_VREZ_W), h = static_cast<f32>(PS2_VREZ_H);
            f32 sx = w * 0.0f * 0.75f, sy = h * 0.0f * 0.75f;
            NuVpSetRegions((sx + -0.125f * w) / scale, (sy + -0.125f * h) / scale, (w + sx + -0.125f * w) / scale,
                           (h + sy + -0.125f * h) / scale, 0, 0, w, h);
            return 1;
        }
        NuRndrScreenGrabTileDeInit(&params);
        dump_state = -1;
        yPos = xPos = yTiles = xTiles = 0.0f;
        NuVpResetRegions();
        return 0;
    }
    if (delay != 0) {
        --delay;
        return 1;
    }
    NuRndrScreenGrabTileBegin(&tile);
    if (tile == NULL) {
        NuRndrScreenGrabTileDeInit(&params);
        dump_state = -1;
        NuFileClose(fh);
        yPos = xPos = yTiles = xTiles = 0.0f;
        NuVpResetRegions();
        return 0;
    }
    u8 *pixels = static_cast<u8 *>(tile);
    i32 bytes = params.channels[3] == -1 ? 3 : 4;
    if (!(bytes == 3 && params.channels[0] == 0 && params.channels[1] == 1 && params.channels[2] == 2)) {
        u8 *out = pixels;
        for (i32 y = 0; y < PS2_REZ_H; ++y) {
            for (i32 x = 0; x < bytes * PS2_REZ_W; x += bytes) {
                u8 rgba[4];
                u8 *in = static_cast<u8 *>(tile) + params.width * y * bytes + x;
                rgba[0] = in[0];
                rgba[1] = in[1];
                rgba[2] = in[2];
                rgba[3] = in[3];
                out[0] = rgba[params.channels[0]];
                out[1] = rgba[params.channels[1]];
                out[2] = rgba[params.channels[2]];
                out += 3;
            }
        }
    }
    f32 w = static_cast<f32>(PS2_REZ_W), h = static_cast<f32>(PS2_REZ_H);
    i32 width = static_cast<i32>(xPos == xTiles - 1.0f ? scale * w - ((xTiles - 1.0f) * 0.75f) * w : 0.75f * w);
    f32 full_height = scale * h;
    i32 height = static_cast<i32>(yPos == yTiles - 1.0f ? full_height - ((yTiles - 1.0f) * 0.75f) * h : 0.75f * h);
    u8 *data = pixels + static_cast<u32>(((static_cast<f32>(params.width) * 3.0f) * 0.125f) * h) +
               static_cast<u32>((0.125f * w) * 3.0f) +
               static_cast<u32>(((static_cast<f32>(xOffsetHack * 3) * w) / static_cast<f32>(PS2_VREZ_W)) * scale);
    if (yPos == yTiles - 1.0f)
        data += static_cast<u32>(((0.75f * h - static_cast<f32>(height)) * w) * 3.0f);
    i64 offset = static_cast<i64>(header_size) +
                 static_cast<i32>((((full_height - (0.75f * yPos) * h) * w) * scale) * 3.0f) +
                 static_cast<i32>(((0.75f * xPos) * w) * 3.0f) -
                 static_cast<i32>((static_cast<f32>(height * PS2_REZ_W) * scale) * 3.0f);
    for (i32 y = 0; y < height; ++y) {
        NuFileSeek(fh, offset, static_cast<NUFILESEEK>(0));
        offset += static_cast<i32>((static_cast<f32>(PS2_REZ_W) * scale) * 3.0f);
        NuFileWrite(fh, data, width * 3);
        data += params.width * 3;
    }
    NuRndrScreenGrabTileEnd(&tile);
    ++dump_state;
    if (!(static_cast<f32>(dump_state) < yTiles * xTiles)) {
        NuRndrScreenGrabTileDeInit(&params);
        dump_state = -1;
        NuFileClose(fh);
        yPos = xPos = yTiles = xTiles = 0.0f;
        NuVpResetRegions();
        NuRndrDoingScreenGrab = 0;
        return 0;
    }
    xPos += 1.0f;
    if (xPos >= xTiles) {
        xPos = 0.0f;
        yPos += 1.0f;
    }
    w = static_cast<f32>(PS2_VREZ_W);
    h = static_cast<f32>(PS2_VREZ_H);
    f32 x = (xPos * w) * 0.75f;
    f32 left = (-0.125f * w + x) / scale;
    f32 right = (x + w + -0.125f * w) / scale;
    xOffsetHack = 0;
    if (right > w) {
        xOffsetHack = static_cast<i32>(right - w);
        left -= static_cast<f32>(xOffsetHack);
        right = w;
    }
    f32 y = (yPos * h) * 0.75f;
    NuVpSetRegions(left, (y + -0.125f * h) / scale, right, (h + y + -0.125f * h) / scale, 0, 0, w, h);
    delay = 3;
    return 1;
}

struct NuLineVertex2D {
    f32 x, y;
    f32 unused[2];
    i32 colour;
    f32 u, v;
};

extern "C" i32 NuRndrLine2d(NuLineVertex2D *vertices, NUMTL *material) {
    NuPrim2DBegin(2, 7, material);
    NuRndrPrimSetColour(vertices[0].colour);
    f32 u = vertices[0].u;
    f32 v = vertices[0].v;
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)&((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->uv[0] = u;
        *(f32 *)&((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->uv[1] = v;
    } else {
        u16 *uv = (u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10);
        uv[0] = NuRndrFloatToHalf(u);
        uv[1] = NuRndrFloatToHalf(v);
    }
    NuPrim2DAddXYZ(vertices[0].x, vertices[0].y, 0.0f);
    NuRndrPrimSetColour(vertices[1].colour);
    u = vertices[1].u;
    v = vertices[1].v;
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)&((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->uv[0] = u;
        *(f32 *)&((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->uv[1] = v;
    } else {
        u16 *uv = (u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10);
        uv[0] = NuRndrFloatToHalf(u);
        uv[1] = NuRndrFloatToHalf(v);
    }
    NuPrim2DAddXYZ(vertices[1].x, vertices[1].y, 0.0f);
    NuPrim2DEnd();
    return 1;
}
extern "C" void NuRndrLine2di(i32 x0, i32 y0, i32 x1, i32 y1, i32 colour, NUMTL *material) {
    const f32 sx = static_cast<f32>(x0) * 0.0625f;
    const f32 sy = static_cast<f32>(y0) * 0.0625f;
    const f32 ex = static_cast<f32>(x1) * 0.0625f;
    const f32 ey = static_cast<f32>(y1) * 0.0625f;
    NuPrim2DBegin(2, 7, material);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(0.0f, 0.0f);
    NuPrim2DAddXYZ(sx, sy, 0.0f);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(1.0f, 1.0f);
    NuPrim2DAddXYZ(ex, ey, 0.0f);
    NuPrim2DEnd();
}
struct NuLineVertex {
    NUVEC position;
    NUVEC normal;
    i32 colour;
    f32 u, v;
};

extern "C" void NuRndrLine3d(NuLineVertex *vertices, NUMTL *material, NUMTX *matrix) {
    NuPrim3DBegin(2, 7, material, matrix);
    for (i32 i = 0; i < 2; ++i) {
        if (!g_NuPrim_NeedsOverbrightening)
            ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color =
                ((vertices[i].colour >> 1) & 0x007f7f7f) | (vertices[i].colour & 0xff000000);
        else
            ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color = vertices[i].colour;
        NuRndrPrimUV(vertices[i].u, vertices[i].v);
        f32 x = vertices[i].position.x;
        f32 y = vertices[i].position.y;
        f32 z = vertices[i].position.z;
        PrimVertexRaw *vertex = (PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr;
        vertex->x = x;
        vertex->y = y;
        vertex->z = z;
        g_NuPrim_StreamBufferPtr->u8_ptr += sizeof(PrimVertexRaw);
        ++g_NuPrim_VertexCount;
    }
    NuPrim3DEnd();
}
extern "C" void NuRndrLine3dDbg(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, i32 colour) {
    NuPrim3DBegin(2, 5, nullptr, &numtx_identity);
    if (!g_NuPrim_NeedsOverbrightening)
        ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color =
            ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
    else
        ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color = colour;
    PrimVertexRaw *vertex = (PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr;
    vertex->x = x0;
    vertex->y = y0;
    vertex->z = z0;
    g_NuPrim_StreamBufferPtr->u8_ptr += sizeof(PrimVertexRaw);
    ++g_NuPrim_VertexCount;
    if (!g_NuPrim_NeedsOverbrightening)
        ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color =
            ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
    else
        ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color = colour;
    vertex = (PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr;
    vertex->x = x1;
    vertex->y = y1;
    vertex->z = z1;
    g_NuPrim_StreamBufferPtr->u8_ptr += sizeof(PrimVertexRaw);
    ++g_NuPrim_VertexCount;
    NuPrim3DEnd();
}
extern "C" void NuRndrLineRect2di(i32 x, i32 y, i32 width, i32 height, i32 colour, NUMTL *material) {
    f32 sx = (f32)x * 0.0625f;
    f32 sy = (f32)y * 0.0625f;
    f32 sw = (f32)width * 0.0625f;
    f32 sh = (f32)height * 0.0625f;
    NuPrim2DBegin(3, 7, material);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(0.0f, 0.0f);
    NuPrim2DAddXYZ(sx, sy, 0.0f);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(1.0f, 0.0f);
    f32 ex = sx + sw;
    NuPrim2DAddXYZ(ex, sy, 0.0f);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(1.0f, 1.0f);
    f32 ey = sy + sh;
    NuPrim2DAddXYZ(ex, ey, 0.0f);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(0.0f, 1.0f);
    NuPrim2DAddXYZ(sx, ey, 0.0f);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(0.0f, 0.0f);
    NuPrim2DAddXYZ(sx, sy, 0.0f);
    NuPrim2DEnd();
}
extern "C" i32 NuRndrLineStrip2d(NuLineVertex2D *vertices, NUMTL *material, i32 count) {
    NuPrim2DBegin(2, 7, material);
    for (i32 i = 0; i < count; ++i) {
        if (!g_NuPrim_NeedsOverbrightening)
            ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color = 0xff7f7f7f;
        else
            ((PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr)->color = 0xffffffff;
        u8 *vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
        if (!g_NuPrim_NeedsHalfUVs) {
            *(f32 *)(vertex + 0x10) = 0.0f;
            *(f32 *)(vertex + 0x14) = 0.0f;
        } else {
            *(u16 *)(vertex + 0x10) = 0;
            *(u16 *)(vertex + 0x12) = 0;
        }
        NuPrim2DAddXYZ(vertices[i].x * 0.0625f, vertices[i].y * 0.0625f, 0.0f);
    }
    NuPrim2DEnd();
    return 1;
}
extern "C" void NuRndrLineStrip2di(i32 *positions, f32 *uvs, i32 count, i32 colour, NUMTL *material) {
    NuPrim2DBegin(2, 7, material);
    for (i32 i = 0; i < count; ++i) {
        NuRndrPrimSetColour(colour);
        if (uvs) {
            f32 u = uvs[i * 2];
            f32 v = uvs[i * 2 + 1];
            if (!g_NuPrim_NeedsHalfUVs) {
                *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = u;
                *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x14) = v;
            } else {
                *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = NuRndrFloatToHalf(u);
                *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x12) = NuRndrFloatToHalf(v);
            }
        }
        NuPrim2DAddXYZ((f32)positions[i * 2] * 0.0625f, (f32)positions[i * 2 + 1] * 0.0625f, 0.0f);
    }
    NuPrim2DEnd();
}

extern "C" void NuRndrRect(f32 x, f32 y, f32 z, f32 width, f32 height, f32 u0, f32 v0, f32 u1, f32 v1, i32 colour,
                           NUMTL *material) {
    NuPrim2DBegin(4, 7, material);
    u8 *vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)(vertex + 0x10) = u0;
        *(f32 *)(vertex + 0x14) = v0;
    } else {
        *(u16 *)(vertex + 0x10) = NuRndrFloatToHalf(u0);
        *(u16 *)(vertex + 0x12) = NuRndrFloatToHalf(v0);
    }
    i32 adjusted_colour = colour;
    if (!g_NuPrim_NeedsOverbrightening)
        adjusted_colour = ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
    *(i32 *)(vertex + 0x0c) = adjusted_colour;
    NuPrim2DAddXYZ(x, y, z);
    vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
    if (!g_NuPrim_NeedsHalfUVs) {
        *(f32 *)(vertex + 0x10) = u1;
        *(f32 *)(vertex + 0x14) = v1;
    } else {
        *(u16 *)(vertex + 0x10) = NuRndrFloatToHalf(u1);
        *(u16 *)(vertex + 0x12) = NuRndrFloatToHalf(v1);
    }
    adjusted_colour = colour;
    if (!g_NuPrim_NeedsOverbrightening)
        adjusted_colour = ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
    *(i32 *)(vertex + 0x0c) = adjusted_colour;
    NuPrim2DAddXYZ(x + width, y + height, z);
    NuPrim2DEnd();
}
extern "C" void NuRndrRect2d(f32 x, f32 y, f32 z, f32 width, f32 height, i32 colour, NUMTL *material) {
    NuPrim2DBegin(4, 7, material);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(0.0f, 0.0f);
    NuPrim2DAddXYZ(x, y, z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimUV(1.0f, 1.0f);
    NuPrim2DAddXYZ(x + width, y + height, z);
    NuPrim2DEnd();
}
extern "C" void NuRndrRect2di(i32 x, i32 y, i32 w, i32 h, i32 colour, numtl_s *mtl) {
    const f32 sx = static_cast<f32>(x) * 0.2f;
    const f32 sy = static_cast<f32>(y) * 0.2f;
    const f32 sw = static_cast<f32>(w) * 0.2f;
    const f32 sh = static_cast<f32>(h) * 0.2f;

    NuPrim2DBegin(4, 7, mtl);
    NuRndrPrimAttributes(static_cast<u32>(colour), false, false);
    NuPrim2DAddXYZ(sx, sy, 0.0f);
    NuRndrPrimAttributes(static_cast<u32>(colour), true, false);
    NuPrim2DAddXYZ(sx + sw, sy + sh, 0.0f);
    NuPrim2DEnd();
}
extern "C" void NuRndrRectUV2di(i32 x, i32 y, i32 w, i32 h, f32 u0, f32 v0, f32 u1, f32 v1, i32 colour, numtl_s *mtl) {
    const f32 sx = static_cast<f32>(x) * 0.0625f;
    const f32 sy = static_cast<f32>(y) * 0.0625f;
    const f32 sw = static_cast<f32>(w) * 0.0625f;
    const f32 sh = static_cast<f32>(h) * 0.0625f;

    NuPrim2DBegin(4, 7, mtl);
    u8 *vertex;
    if (!g_NuPrim_NeedsHalfUVs) {
        vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
        *reinterpret_cast<f32 *>(vertex + 0x10) = u0;
        *reinterpret_cast<f32 *>(vertex + 0x14) = v0;
    } else {
        vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
        *reinterpret_cast<u16 *>(vertex + 0x10) = NuRndrFloatToHalf(u0);
        *reinterpret_cast<u16 *>(vertex + 0x12) = NuRndrFloatToHalf(v0);
    }
    const char *needs_overbrightening = &g_NuPrim_NeedsOverbrightening;
    i32 adjusted_colour = colour;
    if (!*needs_overbrightening) {
        adjusted_colour = ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
    }
    *reinterpret_cast<u32 *>(vertex + 0x0c) = adjusted_colour;
    NuPrim2DAddXYZ(sx, sy, 0.0f);

    if (!g_NuPrim_NeedsHalfUVs) {
        vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
        *reinterpret_cast<f32 *>(vertex + 0x10) = u1;
        *reinterpret_cast<f32 *>(vertex + 0x14) = v1;
    } else {
        vertex = g_NuPrim_StreamBufferPtr->u8_ptr;
        *reinterpret_cast<u16 *>(vertex + 0x10) = NuRndrFloatToHalf(u1);
        *reinterpret_cast<u16 *>(vertex + 0x12) = NuRndrFloatToHalf(v1);
    }
    adjusted_colour = colour;
    if (!*needs_overbrightening) {
        adjusted_colour = ((colour >> 1) & 0x007f7f7f) | (colour & 0xff000000);
    }
    *reinterpret_cast<u32 *>(vertex + 0x0c) = adjusted_colour;
    NuPrim2DAddXYZ(sx + sw, sy + sh, 0.0f);
    NuPrim2DEnd();
}
extern "C" i32 NuRndrSetAmbientLightPS(const NUCOLOUR3 *colour) {
    render_state.ambient_intensity = *colour;
    render_state.light_state = nullptr;
    render_state.state.global_id++;
    render_state.state.lights_id++;
    return 1;
}
extern "C" i32 NuRndrSetAmbientLightSpecular(const NUCOLOUR4 *colour) {
    render_state.global_specular = colour->a;
    NuRndrSetAmbientLightPS(reinterpret_cast<const NUCOLOUR3 *>(colour));
    return 0;
}
extern "C" void NuRndrSetCullDebug(void) {
}
extern "C" {
    i32 NuRndrStopUpdate;
    NUVEC NuRndrDebBase;
    NUVEC NuRndrDebRange;
    NUVEC NuRndrDebRangeInv;
}

extern "C" void NuRndrSetDebBaseRange(NUVEC *base, NUVEC *range) {
    NuRndrDebBase = *base;
    NuRndrDebRange = *range;
    NuRndrDebRangeInv.x = 1.0f / NuRndrDebRange.x;
    NuRndrDebRangeInv.y = 1.0f / NuRndrDebRange.y;
    NuRndrDebRangeInv.z = 1.0f / NuRndrDebRange.z;
}

extern "C" void NuRndrSetDebBox(NUVEC *range) {
    static NUMTX cammtx;
    if (NuRndrStopUpdate == 0) {
        cammtx = global_camera.mtx;
    }
    const NUVEC size = *range;
    // The original uses a narrow near rectangle and a wider far rectangle,
    // transformed by the cached camera before finding their world bounds.
    const NUVEC right = {size.x * cammtx.m00, size.x * cammtx.m01, size.x * cammtx.m02};
    const NUVEC up = {size.y * cammtx.m10, size.y * cammtx.m11, size.y * cammtx.m12};
    const NUVEC forward = {size.z * cammtx.m20, size.z * cammtx.m21, size.z * cammtx.m22};
    NUVEC corner0 = {((right.x * -0.2f + cammtx.m30) + up.x * -0.2f) + forward.x * -0.05f,
                     ((right.y * -0.2f + cammtx.m31) + up.y * -0.2f) + forward.y * -0.05f,
                     ((right.z * -0.2f + cammtx.m32) + up.z * -0.2f) + forward.z * -0.05f};
    NUVEC corner1 = {((right.x * 0.2f + cammtx.m30) + up.x * -0.2f) + forward.x * -0.05f,
                     ((right.y * 0.2f + cammtx.m31) + up.y * -0.2f) + forward.y * -0.05f,
                     ((right.z * 0.2f + cammtx.m32) + up.z * -0.2f) + forward.z * -0.05f};
    NUVEC corner2 = {((right.x * -0.2f + cammtx.m30) + up.x * 0.2f) + forward.x * -0.05f,
                     ((right.y * -0.2f + cammtx.m31) + up.y * 0.2f) + forward.y * -0.05f,
                     ((right.z * -0.2f + cammtx.m32) + up.z * 0.2f) + forward.z * -0.05f};
    NUVEC corner3 = {((right.x * 0.2f + cammtx.m30) + up.x * 0.2f) + forward.x * -0.05f,
                     ((right.y * 0.2f + cammtx.m31) + up.y * 0.2f) + forward.y * -0.05f,
                     ((right.z * 0.2f + cammtx.m32) + up.z * 0.2f) + forward.z * -0.05f};
    NUVEC corner4 = {((right.x * -0.5f + cammtx.m30) + up.x * -0.5f) + forward.x * 0.8f,
                     ((right.y * -0.5f + cammtx.m31) + up.y * -0.5f) + forward.y * 0.8f,
                     ((right.z * -0.5f + cammtx.m32) + up.z * -0.5f) + forward.z * 0.8f};
    NUVEC corner5 = {((right.x * 0.5f + cammtx.m30) + up.x * -0.5f) + forward.x * 0.8f,
                     ((right.y * 0.5f + cammtx.m31) + up.y * -0.5f) + forward.y * 0.8f,
                     ((right.z * 0.5f + cammtx.m32) + up.z * -0.5f) + forward.z * 0.8f};
    NUVEC corner6 = {((right.x * -0.5f + cammtx.m30) + up.x * 0.5f) + forward.x * 0.8f,
                     ((right.y * -0.5f + cammtx.m31) + up.y * 0.5f) + forward.y * 0.8f,
                     ((right.z * -0.5f + cammtx.m32) + up.z * 0.5f) + forward.z * 0.8f};
    NUVEC corner7 = {((right.x * 0.5f + cammtx.m30) + up.x * 0.5f) + forward.x * 0.8f,
                     ((right.y * 0.5f + cammtx.m31) + up.y * 0.5f) + forward.y * 0.8f,
                     ((right.z * 0.5f + cammtx.m32) + up.z * 0.5f) + forward.z * 0.8f};
    NUVEC minimum = corner0;
    NUVEC maximum = corner0;
    minimum.x = corner1.x < minimum.x ? corner1.x : minimum.x;
    maximum.x = corner1.x > maximum.x ? corner1.x : maximum.x;
    minimum.y = corner1.y < minimum.y ? corner1.y : minimum.y;
    maximum.y = corner1.y > maximum.y ? corner1.y : maximum.y;
    minimum.z = corner1.z < minimum.z ? corner1.z : minimum.z;
    maximum.z = corner1.z > maximum.z ? corner1.z : maximum.z;
    minimum.x = corner2.x < minimum.x ? corner2.x : minimum.x;
    maximum.x = corner2.x > maximum.x ? corner2.x : maximum.x;
    minimum.y = corner2.y < minimum.y ? corner2.y : minimum.y;
    maximum.y = corner2.y > maximum.y ? corner2.y : maximum.y;
    minimum.z = corner2.z < minimum.z ? corner2.z : minimum.z;
    maximum.z = corner2.z > maximum.z ? corner2.z : maximum.z;
    minimum.x = corner3.x < minimum.x ? corner3.x : minimum.x;
    maximum.x = corner3.x > maximum.x ? corner3.x : maximum.x;
    minimum.y = corner3.y < minimum.y ? corner3.y : minimum.y;
    maximum.y = corner3.y > maximum.y ? corner3.y : maximum.y;
    minimum.z = corner3.z < minimum.z ? corner3.z : minimum.z;
    maximum.z = corner3.z > maximum.z ? corner3.z : maximum.z;
    minimum.x = corner4.x < minimum.x ? corner4.x : minimum.x;
    maximum.x = corner4.x > maximum.x ? corner4.x : maximum.x;
    minimum.y = corner4.y < minimum.y ? corner4.y : minimum.y;
    maximum.y = corner4.y > maximum.y ? corner4.y : maximum.y;
    minimum.z = corner4.z < minimum.z ? corner4.z : minimum.z;
    maximum.z = corner4.z > maximum.z ? corner4.z : maximum.z;
    minimum.x = corner5.x < minimum.x ? corner5.x : minimum.x;
    maximum.x = corner5.x > maximum.x ? corner5.x : maximum.x;
    minimum.y = corner5.y < minimum.y ? corner5.y : minimum.y;
    maximum.y = corner5.y > maximum.y ? corner5.y : maximum.y;
    minimum.z = corner5.z < minimum.z ? corner5.z : minimum.z;
    maximum.z = corner5.z > maximum.z ? corner5.z : maximum.z;
    minimum.x = corner6.x < minimum.x ? corner6.x : minimum.x;
    maximum.x = corner6.x > maximum.x ? corner6.x : maximum.x;
    minimum.y = corner6.y < minimum.y ? corner6.y : minimum.y;
    maximum.y = corner6.y > maximum.y ? corner6.y : maximum.y;
    minimum.z = corner6.z < minimum.z ? corner6.z : minimum.z;
    maximum.z = corner6.z > maximum.z ? corner6.z : maximum.z;
    minimum.x = corner7.x < minimum.x ? corner7.x : minimum.x;
    maximum.x = corner7.x > maximum.x ? corner7.x : maximum.x;
    minimum.y = corner7.y < minimum.y ? corner7.y : minimum.y;
    maximum.y = corner7.y > maximum.y ? corner7.y : maximum.y;
    minimum.z = corner7.z < minimum.z ? corner7.z : minimum.z;
    maximum.z = corner7.z > maximum.z ? corner7.z : maximum.z;
    NuRndrDebBase.x = (maximum.x + minimum.x) * 0.5f - size.x * 0.5f;
    NuRndrDebBase.y = (maximum.y + minimum.y) * 0.5f - size.y * 0.5f;
    NuRndrDebBase.z = (maximum.z + minimum.z) * 0.5f - size.z * 0.5f;
    NuRndrDebRange = size;
    NuRndrDebRangeInv.x = 1.0f / size.x;
    NuRndrDebRangeInv.y = 1.0f / size.y;
    NuRndrDebRangeInv.z = 1.0f / size.z;
}
extern "C" i32 NuRndrSetDirectionalLightsPS(const NUVEC *dir0, const NUCOLOUR3 *colour0, const NUVEC *dir1,
                                            const NUCOLOUR3 *colour1, const NUVEC *dir2, const NUCOLOUR3 *colour2) {
    NUMTX *view = NuCameraGetViewMtx();
    const NUVEC *directions[3] = {dir0, dir1, dir2};
    const NUCOLOUR3 *colours[3] = {colour0, colour1, colour2};
    for (i32 i = 0; i < 3; i++) {
        render_state.light_intensity[i] = *colours[i];
        NuVecMtxRotate(&render_state.light_direction[i], const_cast<NUVEC *>(directions[i]), view);
        NuVecNorm(&render_state.light_direction[i], &render_state.light_direction[i]);
    }
    render_state.light_state = nullptr;
    render_state.state.global_id++;
    render_state.state.lights_id++;
    return 1;
}
extern "C" {
    i32 g_minmiplevel = 13;
    f32 g_mipmapbias;
}

extern "C" void NuRndrSetGlobalMinMipLevel(i32 level) {
    g_minmiplevel = level;
}
extern "C" void NuRndrSetGlobalMipMapBias(f32 bias) {
    g_mipmapbias = bias;
}
extern "C" void NuRndrStateSetSpecularLight(const NUMTX *matrix, const NUCOLOUR3 *colour) {
    if (matrix != nullptr) {
        render_state.specular_mtx = *matrix;
    }
    if (colour != nullptr) {
        render_state.specular_colour = *colour;
    }
    render_state.light_state = nullptr;
    render_state.state.global_id++;
    render_state.state.lights_id++;
}

extern "C" void NuRndrStateSetSpecularLightEx(const NUVEC *direction, const NUMTX *matrix, const NUCOLOUR3 *colour) {
    render_state.specular_mtx = *matrix;
    render_state.specular_colour = *colour;
    render_state.specular_intensity = *direction;
    render_state.light_state = nullptr;
    render_state.state.global_id++;
    render_state.state.lights_id++;
}

f32 global_windspeed = 1.0f;
f32 global_windscale = 1.0f;
extern "C" void NuRndrSetWind(f32 speed, f32 scale) {
    global_windspeed = speed;
    global_windscale = scale;
}
static inline void NuRndrPrimPosition(f32 x, f32 y, f32 z) {
    PrimVertexRaw *vertex = (PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr;
    vertex->x = x;
    vertex->y = y;
    vertex->z = z;
    g_NuPrim_StreamBufferPtr->u8_ptr += sizeof(PrimVertexRaw);
    ++g_NuPrim_VertexCount;
}

extern "C" void NuRndrGrid(NUVEC *centre, NUVEC *size, i32 columns, i32 rows) {
    NUMTX matrix = numtx_identity;
    matrix.m30 = centre->x;
    matrix.m31 = 0.0f;
    matrix.m32 = centre->z;
    matrix.m00 = size->x * 0.5f;
    matrix.m11 = 1.0f;
    matrix.m22 = size->z * 0.5f;
    f32 dx = 2.0f / (f32)columns;
    f32 dz = 2.0f / (f32)rows;
    NuPrim3DBegin(2, 5, NULL, &matrix);
    f32 x = -1.0f;
    for (i32 i = 0; i <= columns; ++i, x += dx) {
        NuRndrPrimSetColour(0xff00ff00);
        NuRndrPrimPosition(x, 0.0f, -1.0f);
        NuRndrPrimSetColour(0xff00ff00);
        NuRndrPrimPosition(x, 0.0f, 1.0f);
    }
    f32 z = -1.0f;
    for (i32 i = 0; i <= rows; ++i, z += dz) {
        NuRndrPrimSetColour(0xff00ff00);
        NuRndrPrimPosition(-1.0f, 0.0f, z);
        NuRndrPrimSetColour(0xff00ff00);
        NuRndrPrimPosition(1.0f, 0.0f, z);
    }
    NuPrim3DEnd();
}

extern "C" void NuRndrSolidTri(NUVEC *a, NUVEC *b, NUVEC *c, i32 colour) {
    NuPrim3DBegin(0, 5, NULL, NULL);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(a->x, a->y, a->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(b->x, b->y, b->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(c->x, c->y, c->z);
    NuPrim3DEnd();
}
extern "C" void NuRndrSphere(NUVEC *centre, f32 radius, i32 colour, f32 vertical_scale) {
    f32 radius_squared = radius * radius;
    for (i32 band = 0; band < 8; ++band) {
        f32 lower_radius = radius * NU_SIN_LUT((band < 4 ? band : 8 - band) * 4096);
        f32 upper_radius = radius * NU_SIN_LUT((band < 3 ? band + 1 : 7 - band) * 4096);
        f32 lower_y = NuFsqrt(radius_squared - lower_radius * lower_radius) * vertical_scale;
        f32 upper_y = NuFsqrt(radius_squared - upper_radius * upper_radius) * vertical_scale;
        if (band > 4)
            lower_y = -lower_y;
        if (band > 3)
            upper_y = -upper_y;
        lower_y += centre->y;
        upper_y += centre->y;
        for (i32 angle = 0; angle < 65536; angle += 2048) {
            NuRndrLine3dDbg(centre->x + lower_radius * NU_COS_LUT(angle), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle), centre->x + upper_radius * NU_COS_LUT(angle),
                            upper_y, centre->z + upper_radius * NU_SIN_LUT(angle), colour);
            NuRndrLine3dDbg(centre->x + lower_radius * NU_COS_LUT(angle), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle),
                            centre->x + lower_radius * NU_COS_LUT(angle + 2048), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle + 2048), colour);
        }
    }
}
extern "C" void NuRndrSphereEx(NUVEC *centre, f32 radius, i32 colour, f32 vertical_scale, i32 angle_step) {
    f32 radius_squared = radius * radius;
    for (i32 band = 0; band < 8; ++band) {
        f32 lower_radius = radius * NU_SIN_LUT((band < 4 ? band : 8 - band) * 4096);
        f32 upper_radius = radius * NU_SIN_LUT((band < 3 ? band + 1 : 7 - band) * 4096);
        f32 lower_y = NuFsqrt(radius_squared - lower_radius * lower_radius) * vertical_scale;
        f32 upper_y = NuFsqrt(radius_squared - upper_radius * upper_radius) * vertical_scale;
        if (band > 4)
            lower_y = -lower_y;
        if (band > 3)
            upper_y = -upper_y;
        lower_y += centre->y;
        upper_y += centre->y;
        for (i32 angle = 0; angle < 65536; angle += angle_step) {
            NuRndrLine3dDbg(centre->x + lower_radius * NU_COS_LUT(angle), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle), centre->x + upper_radius * NU_COS_LUT(angle),
                            upper_y, centre->z + upper_radius * NU_SIN_LUT(angle), colour);
            NuRndrLine3dDbg(centre->x + lower_radius * NU_COS_LUT(angle), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle),
                            centre->x + lower_radius * NU_COS_LUT(angle + angle_step), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle + angle_step), colour);
        }
    }
}
extern "C" void NuRndrSphereMtx(NUMTX *matrix, i32 colour, i32 segments, NUMTL *material) {
    f32 step = 65536.0f / (f32)segments;
    NuPrim3DBegin(3, 5, material, matrix);
    f32 longitude = 0.0f;
    for (i32 ring = 0; ring < segments / 2; ++ring, longitude += step) {
        f32 sin_longitude = NU_SIN_LUT(longitude);
        f32 cos_longitude = NU_SIN_LUT(longitude + 16384.0f);
        f32 angle = 0.0f;
        for (i32 i = 0; i <= segments; ++i, angle += step) {
            f32 radius = NU_SIN_LUT(angle);
            NuRndrPrimSetColour(colour);
            NuRndrPrimPosition(radius * sin_longitude, NU_SIN_LUT(angle + 16384.0f), radius * cos_longitude);
        }
    }
    NuPrim3DEnd();
    f32 latitude = step;
    for (i32 ring = 1; ring < segments / 2; ++ring, latitude += step) {
        f32 radius = NU_SIN_LUT(latitude);
        f32 y = NU_SIN_LUT(latitude + 16384.0f);
        NuPrim3DBegin(3, 5, material, matrix);
        f32 angle = 0.0f;
        for (i32 i = 0; i <= segments; ++i, angle += step) {
            NuRndrPrimSetColour(colour);
            NuRndrPrimPosition(NU_SIN_LUT(angle) * radius, y, NU_SIN_LUT(angle + 16384.0f) * radius);
        }
        NuPrim3DEnd();
    }
}
extern "C" void NuRndrSphereTRS(NUVEC *translation, NUANGVEC *rotation, NUVEC *scale, i32 colour, i32 segments,
                                NUMTL *material) {
    NUMTX matrix;
    if (rotation)
        NuMtxSetRotateXYZ(&matrix, rotation);
    else
        matrix = numtx_identity;
    if (translation)
        NuMtxTranslate(&matrix, translation);
    if (scale)
        NuMtxPreScale(&matrix, scale);
    NuRndrSphereMtx(&matrix, colour, segments, material);
}
extern "C" void NuRndrStartReflectionRender(i32) {
    NuSpecialReflection(1);
}
extern "C" void NuRndrStartShadowReceiveRender(void) {
    global_GobjIsShadowReceive = 1;
}

void *RndrStateBuildKonstState(NUGLOBALRNDRSTATE *state);
extern "C" void *RndrStateBuildFogState(NUGLOBALRNDRSTATE *state);

static void *RndrStateBuildLightState(NUGLOBALRNDRSTATE *state) {
    VARIPTR *buffer = NuDisplayListGetBuffer();
    auto *packet = static_cast<NULIGHTSTATE *>(buffer->void_ptr);
    buffer->addr += sizeof(NULIGHTSTATE);

    packet->ambient_intensity = {state->ambient_intensity.r, state->ambient_intensity.g, state->ambient_intensity.b,
                                 1.0f};
    for (i32 i = 0; i < 3; i++) {
        packet->light_intensity[i] = {state->light_intensity[i].r, state->light_intensity[i].g,
                                      state->light_intensity[i].b, 1.0f};
        packet->light_direction[i] = {state->light_direction[i].x, state->light_direction[i].y,
                                      state->light_direction[i].z, 1.0f};
    }
    packet->specular_mtx = state->specular_mtx;
    packet->specular_colour = state->specular_colour;
    packet->specular_intensity = state->specular_intensity;
    return packet;
}

// Camera-state portion of original DisplayListUpdateRenderState @0x2fd5d0.
// The remaining light/fog/konst branches are independent state builders and
// are left for their respective subsystem transcriptions.
extern "C" void DisplayListUpdateRenderState(void *display_list, void *state) {
    auto *dl = static_cast<NUDISPLAYLIST *>(display_list);
    auto *global = static_cast<NUGLOBALRNDRSTATE *>(state);
    if (global == nullptr || dl->state->global_id == global->state.global_id) {
        return;
    }

    if (dl->state->lights_id != global->state.lights_id) {
        if (global->light_state == nullptr) {
            global->light_state = RndrStateBuildLightState(global);
        }
        NuDisplayListLinkItem(dl, 0x94, global->light_state);
        dl->state->lights_id = global->state.lights_id;
    }

    if (dl->state->camera_id != global->state.camera_id) {
        if (global->camera_state == nullptr) {
            struct CameraPacket {
                i32 id;
                NUMTX view;
                NUMTX projection;
                f32 viewport[4];
            };

            VARIPTR *buffer = NuDisplayListGetBuffer();
            auto *packet = static_cast<CameraPacket *>(buffer->void_ptr);
            global->camera_state = packet;
            packet->id = nuapi.frame_count + (global->state.camera_id + 5) * (global->state.global_id + 13);
            packet->view = global->view;
            memset(&packet->projection, 0, sizeof(packet->projection));
            packet->projection.m00 = global->proj_00;
            packet->projection.m11 = global->proj_11;
            packet->projection.m22 = global->proj_22;
            packet->projection.m23 = global->proj_23;
            packet->projection.m32 = global->proj_32;
            packet->projection.m20 = global->proj_20;
            packet->projection.m21 = global->proj_21;
            packet->viewport[0] = global->vpx;
            packet->viewport[1] = global->vpy;
            packet->viewport[2] = global->vpw;
            packet->viewport[3] = global->vph;
            buffer->addr += sizeof(CameraPacket);
        }
        NuDisplayListLinkItem(dl, 0x9a, global->camera_state);
        dl->state->camera_id = global->state.camera_id;
    }
    if (dl->state->fog_id != global->state.fog_id) {
        if (global->fog_state == nullptr) {
            global->fog_state = RndrStateBuildFogState(global);
        }
        NuDisplayListLinkItem(dl, 0xa6, global->fog_state);
        dl->state->fog_id = global->state.fog_id;
    }
    if (dl->state->konst_id != global->state.konst_id) {
        if (global->konst_state == nullptr) {
            global->konst_state = RndrStateBuildKonstState(global);
        }
        NuDisplayListLinkItem(dl, 0xa5, global->konst_state);
        dl->state->konst_id = global->state.konst_id;
    }
    dl->state->global_id = global->state.global_id;
}
extern "C" i32 NuRndrStrip3d(NURND_VERTEX3D *vertices, numtl_s *material, NUMTX *matrix, i32 count) {
    if (count == 0)
        return 1;
    NuPrim3DBegin(1, 7, material, matrix);
    for (i32 i = 0; i < count; ++i) {
        NURND_VERTEX3D *source = &vertices[i];
        u8 *vertex = reinterpret_cast<u8 *>(g_NuPrim_StreamBufferPtr->addr);
        *reinterpret_cast<u32 *>(vertex + 0xc) = NuRndrPrimColour(source->colour);
        if (g_NuPrim_NeedsHalfUVs != 0) {
            *reinterpret_cast<u16 *>(vertex + 0x10) = NuRndrFloatToHalf(source->u);
            *reinterpret_cast<u16 *>(vertex + 0x12) = NuRndrFloatToHalf(source->v);
        } else {
            *reinterpret_cast<f32 *>(vertex + 0x10) = source->u;
            *reinterpret_cast<f32 *>(vertex + 0x14) = source->v;
        }
        *reinterpret_cast<NUVEC *>(vertex) = source->position;
        g_NuPrim_StreamBufferPtr->addr += 0x18;
    }
    if (count > 0)
        g_NuPrim_VertexCount += count;
    NuPrim3DEnd();
    return 1;
}
extern "C" i32 NuRndrTri3dClip(NURND_VERTEX3D *vertices, i32 count, NUMTX *matrix, numtl_s *material) {
    if (count == 0)
        return 1;
    NuPrim3DBegin(0, 7, material, matrix);
    for (i32 i = 0; i < count; ++i) {
        NuRndrPrimSetColour(vertices[i].colour);
        NuRndrPrimUV(vertices[i].u, vertices[i].v);
        PrimVertexRaw *vertex = (PrimVertexRaw *)g_NuPrim_StreamBufferPtr->void_ptr;
        vertex->x = vertices[i].position.x;
        vertex->y = vertices[i].position.y;
        vertex->z = vertices[i].position.z;
        g_NuPrim_StreamBufferPtr->addr += 24;
    }
    if (count > 0)
        g_NuPrim_VertexCount += count;
    NuPrim3DEnd();
    return 1;
}
extern "C" void NuRndrTriStrip2di(i32 *positions, f32 *uvs, i32 count, i32 colour, NUMTL *material) {
    NuPrim2DBegin(1, 7, material);
    for (i32 i = 0; i < count; ++i) {
        NuRndrPrimSetColour(colour);
        if (uvs) {
            f32 u = uvs[i * 2];
            f32 v = uvs[i * 2 + 1];
            if (!g_NuPrim_NeedsHalfUVs) {
                *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = u;
                *(f32 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x14) = v;
            } else {
                *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x10) = NuRndrFloatToHalf(u);
                *(u16 *)(g_NuPrim_StreamBufferPtr->u8_ptr + 0x12) = NuRndrFloatToHalf(v);
            }
        }
        NuPrim2DAddXYZ((f32)positions[i * 2] * 0.0625f, (f32)positions[i * 2 + 1] * 0.0625f, 0.0f);
    }
    NuPrim2DEnd();
}
extern "C" i32 NuRndrTriStrip3dClip(NURND_VERTEX3D *vertices, i32 count, NUMTX *matrix, numtl_s *material) {
    return NuRndrStrip3d(vertices, material, matrix, count);
}
i32 global_GobjWasDrawnUnreflected;
extern "C" i32 NuRndrWasDrawnUnreflectedGobj(void) {
    return global_GobjWasDrawnUnreflected;
}
extern "C" void NuRndrWireTri(NUVEC *a, NUVEC *b, NUVEC *c, i32 colour) {
    NuPrim3DBegin(3, 5, NULL, NULL);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(a->x, a->y, a->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(b->x, b->y, b->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(c->x, c->y, c->z);
    NuRndrPrimSetColour(colour);
    NuRndrPrimPosition(a->x, a->y, a->z);
    NuPrim3DEnd();
}

// Shader / texture / vertex state
extern "C" {
    void *g_boundLightPacket;
    void *g_boundCameraPacket;
    ShaderPacketStateMapping g_packetToShaderStateMappings[2] = {
        {{{0, 0, 0, 0}}, &g_boundLightPacket},
        {{{0, 0, 0, 0}}, &g_boundCameraPacket},
    };
}
extern "C" void NuShaderGetDirtyMask(NUSHADERUSAGEMASK *mask, NUSHADEROBJECT *shader) {
    memmove(mask, shader->usage_mask, sizeof(*mask));
    const i32 frame = NuRndrGlobalFrameCount();
    if (shader->last_uniform_frame != frame) {
        shader->last_uniform_frame = frame;
        return;
    }
    void *light_packet = *g_packetToShaderStateMappings[0].packet;
    if (shader->last_light_packet == light_packet) {
        for (i32 i = 0; i < 4; ++i)
            mask->semantics[i] &= ~g_packetToShaderStateMappings[0].mask.semantics[i];
    } else {
        shader->last_light_packet = light_packet;
    }
    void *camera_packet = *g_packetToShaderStateMappings[1].packet;
    if (shader->last_camera_packet == camera_packet) {
        for (i32 i = 0; i < 4; ++i)
            mask->semantics[i] &= ~g_packetToShaderStateMappings[1].mask.semantics[i];
    } else {
        shader->last_camera_packet = camera_packet;
    }
}
extern "C" nu2api::ShaderUniformRecord *NuShaderUniformGetByString(const char *name) {
    for (i32 i = 0; i < 101; ++i) {
        if ((g_shaderUniforms[i].fragment_name && strcmp(g_shaderUniforms[i].fragment_name, name) == 0) ||
            (g_shaderUniforms[i].vertex_name && strcmp(g_shaderUniforms[i].vertex_name, name) == 0))
            return &g_shaderUniforms[i];
    }
    return NULL;
}
extern "C" void NuTexDestroy(i32) {
}
static void NuTexGenTexture(void) {
}
extern "C" void NuTextureBlendEffect(i32 arg0, i32 arg1, NUVEC4 *parameters) {
    currentScene.texture_blend_arg0 = arg0;
    currentScene.texture_blend_arg1 = arg1;
    currentScene.texture_blend_enabled = 1;
    currentScene.texture_blend_parameters = *parameters;
}
extern "C" NUSPECIALVERTEXSTATES *NuVertexStatesCreate(VARIPTR *buffer, i32 count) {
    buffer->addr = ALIGN(buffer->addr, 4);
    NUSPECIALVERTEXSTATES *states = (NUSPECIALVERTEXSTATES *)buffer->void_ptr;
    buffer->u8_ptr += sizeof(NUSPECIALVERTEXSTATES);
    states->count = count;
    states->flags = 0;
    i32 blocks = count / 16;
    if (count & 15)
        ++blocks;
    states->block_count = blocks;
    i32 size = blocks * 16;
    states->values = (i8 *)ALIGN(buffer->addr, 16);
    buffer->addr = ALIGN(buffer->addr, 16) + size;
    for (i32 i = 0; i < size; ++i)
        states->values[i] = 0;
    return states;
}
extern "C" void NuVertexStatesSetGroupState(NUSPECIALVERTEXSTATES *states, i32 group, i32 value) {
    states->values[group] = value;
}
