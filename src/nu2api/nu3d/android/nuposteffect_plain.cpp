// Post-effect state and framebuffer clear — Android GLES2 path.
//
// The Android render thread does not dispatch NuPostEffectRender. Retained
// generic filter controls are reconstructed here without adding a new pass.
//
// Transcribed originals:
//   Nu360_dxClear               0x317070  lives in ios_graphics.cpp
//   NuFramebufferClear          0x2a2720  thin forwarder to Nu360_dxClear
//   NuFramebufferSwapBuffers    0x2a2700  no-op on Android/host (swap owned by EGL)
//   NuPostEffectReset           0x2ab8b0
//   NuPostEffectEnd             0x2ab8d0
//   NuPostEffectIsInitialised   0x2ab9c0
//   NuPostEffectAddDynamicLight 0x2abc40

#include <GLES2/gl2.h>
#include <new>
#include <string.h>

#include "decomp.h"
#include "nu2api/nu3d/android/nuposteffect_plain.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/NuMainFilter.h"
#include "nu2api/nucore/NuDeferredFilter.h"
#include "nu2api/nucore/NuMotionFilter.h"
#include "nu2api/nucore/NuMotionAccumFilter.h"
#include "nu2api/nucore/NuSpeedBlurFilter.h"
#include "nu2api/nucore/NuCopyFilter.h"
#include "nu2api/nucore/NuPostFilter.h"
#include "nu2api/nu3d/nupostresources.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nuandroid/ios_graphics.h"

// ──────────────────────────────────────────────────────────────────────────────
// Post-effect globals
// ──────────────────────────────────────────────────────────────────────────────

// Bitmask of which post-effects were successfully initialised.  Tested in
// renderThread_processRenderScenes (nurenderthread.cpp) against the per-slot
// masks — a disabled effect has its scene slot cleared before the frame is
// dispatched:
//
//   0x04  -> scn.unknown_58   (Bloom / main filter path)
//   0x08  -> scn.unknown_ac
//   0x10  -> scn.unknown_e4   (camera motion blur)
//   0x20  -> scn.unknown_48   (deferred shading)
//   0x40  -> scn.unknown_178
//   0x80  -> scn.unknown_188  (motion blur)
//
// The enum below names the bits as they appear in the original binary.
enum NuPostEffectFlag : i32 {
    kEffect_BloomOrMain = 0x04,
    kEffect_Slot_AC = 0x08,
    kEffect_CameraMotionBlur = 0x10,
    kEffect_Deferred = 0x20,
    kEffect_Slot_178 = 0x40,
    kEffect_MotionBlur = 0x80,
};

i32 g_effectFlags; // bss - see masks above
static i32 g_effectsRan = 0;
static u8 g_lastFrameEffect = 0;

NuDeferredFilter *deferredFilter;
static NuMainFilter *mainFilter;
static NuMotionFilter *motionFilter;
static NuMotionAccumFilter *motionAccumFilter;
static NuSpeedBlurFilter *speedBlurFilter;
static NuCopyFilter *copyFilter;
static u8 filterMem[0x800];
static u8 *filterCursor = filterMem;
static nueffecttex_s *g_backBufferCopy;
u32 g_posteffect_shaderEngineFlag;

template <typename T> static T *AllocatePostFilter() {
    T *filter = reinterpret_cast<T *>(filterCursor);
    filterCursor += sizeof(T);
    memset(filter, 0, sizeof(T));
    return new (filter) T;
}

// Original proxy descriptor is 12 bytes, including two one-byte flags.

static NuProxyBuffer s_proxyColorBuffer;
static NuProxyBuffer s_proxyNormalBuffer;
static NuProxyBuffer s_proxyVelocityBuffer;
static NuProxyBuffer s_proxyDepthBuffer;
static NuProxyBuffer s_proxyDepthRTBuffer;

static constexpr i32 kProxyKind_Color = 0;
static constexpr i32 kProxyKind_Normal = 1;
static constexpr i32 kProxyKind_Velocity = 2;
static constexpr i32 kProxyKind_Depth = 4;

static inline void ResetProxyBuffer(NuProxyBuffer *proxy, i32 kind) {
    proxy->texture = NULL;
    proxy->kind = kind;
    proxy->enabled = true;
    proxy->resolved = false;
}

static inline void FilterEnd(NuPostFilterGen *filter) {
    if (filter == nullptr) {
        return;
    }
    filter->resetAll();
}

// ── Post-effect API ─────────────────────────────────────────────────────────

// original 0x2ab9c0
extern "C" bool NuPostEffectIsInitialised(u32 mask) {
    return (g_effectFlags & static_cast<i32>(mask)) != 0;
}

// original 0x2ab8b0 — per-frame reset called at the top of
// renderThread_processRenderScenes before the safe scene list is walked.
extern "C" void NuPostEffectReset(void) {
    g_effectsRan = 0;
    g_lastFrameEffect = 0;
}

// original 0x2abc40 — feeds a dynamic light handle into the deferred filter's
// light list.  The handle is an opaque light pointer; the original checks
// *(i32*)(light+0x7bc) != 0 before calling (see nurenderthread.cpp).
extern "C" void NuPostEffectAddDynamicLight(void *light) {
    if (deferredFilter == nullptr) {
        return;
    }
    i32 count = deferredFilter->dynamic_light_count;
    deferredFilter->dynamic_lights[count] = static_cast<NuDynamicLight *>(light);
    deferredFilter->dynamic_light_count = count + 1;
}

// original 0x2ab8d0 — resetAll (vtable+0x20) for each allocated filter,
// followed by the four original proxy resets. Speed blur and the depth-RT
// proxy are deliberately absent from this reset sequence.
extern "C" void NuPostEffectEnd(void) {
    FilterEnd(deferredFilter);
    FilterEnd(mainFilter);
    FilterEnd(motionFilter);
    FilterEnd(motionAccumFilter);

    ResetProxyBuffer(&s_proxyColorBuffer, kProxyKind_Color);
    ResetProxyBuffer(&s_proxyNormalBuffer, kProxyKind_Normal);
    ResetProxyBuffer(&s_proxyVelocityBuffer, kProxyKind_Velocity);
    ResetProxyBuffer(&s_proxyDepthBuffer, kProxyKind_Depth);
}

extern "C" void NuPostEffectInit(u32 flags, void *buffer, void *buffer_end) {
    g_effectFlags = flags;
    if (flags & 0x0c)
        mainFilter = AllocatePostFilter<NuMainFilter>();
    if (flags & 0x10) {
        motionFilter = AllocatePostFilter<NuMotionFilter>();
        g_posteffect_shaderEngineFlag |= 0x20;
    }
    if (flags & 0x40)
        motionAccumFilter = AllocatePostFilter<NuMotionAccumFilter>();
    if (flags & 0x80)
        speedBlurFilter = AllocatePostFilter<NuSpeedBlurFilter>();
    if (flags & 0x20) {
        deferredFilter = AllocatePostFilter<NuDeferredFilter>();
        g_posteffect_shaderEngineFlag |= 0x10;
    }
    copyFilter = AllocatePostFilter<NuCopyFilter>();
    NuEffectTexLockVP(buffer, buffer_end);
    NuPostFilterGen::initSharedTextureResources(100, 100);
    NuPostFilterGen *filters[] = {mainFilter, motionFilter, motionAccumFilter, speedBlurFilter, deferredFilter};
    for (i32 i = 0; i < 5; ++i) {
        if (filters[i] != NULL)
            filters[i]->initTextureResources(100, 100);
    }
    if (flags & 1)
        g_backBufferCopy = NuEffectTexCreate2D(100, 100, 1, 1, 2);
    copyFilter->initTextureResources(100, 100);
    NuEffectTexUnlockVP();
    NuPostFilter::initSharedResources(100, 100);
    for (i32 i = 0; i < 5; ++i) {
        if (filters[i] != NULL)
            filters[i]->initResources();
    }
    copyFilter->initResources();
    NuPostEffectReset();
}

extern "C" void NuPostEffectDestroy() {
    NuPostFilterGen *filters[] = {mainFilter,      motionFilter,   motionAccumFilter,
                                  speedBlurFilter, deferredFilter, copyFilter};
    for (i32 i = 0; i < 6; ++i) {
        if (filters[i] != NULL) {
            filters[i]->destroyTextureResources();
            filters[i]->destroyResources();
        }
    }
    mainFilter = NULL;
    motionFilter = NULL;
    motionAccumFilter = NULL;
    speedBlurFilter = NULL;
    deferredFilter = NULL;
    copyFilter = NULL;
    NuPostFilterGen::destroySharedTextureResources();
    NuPostFilterGen::destroySharedResources();
    filterCursor = filterMem;
}

extern "C" void NuPostEffectRender(nuframebuffer_s *output) {
    nuframebuffer_s *bound = NuFramebufferGetBound();
    if (output == NULL)
        output = NuFramebufferGetObject(1);
    NuFramebufferGetObject(0);
    NuProxyBuffer *proxies[] = {&s_proxyColorBuffer, &s_proxyNormalBuffer, &s_proxyVelocityBuffer,
                                &s_proxyDepthRTBuffer, &s_proxyDepthBuffer};
    const i32 attachments[] = {0, 1, 2, 2, 4};
    for (i32 i = 0; i < 5; ++i) {
        proxies[i]->texture = NuFramebufferGetAttachedTex(bound, attachments[i], NULL, NULL);
        proxies[i]->kind = attachments[i];
        proxies[i]->enabled = proxies[i]->resolved = true;
    }
    NuFramebufferResolveMultisample(0);
    NuPostFilterGen::portColorBuffer.set(proxies[0]);
    NuPostFilterGen::portNormalBuffer.set(proxies[1]);
    NuPostFilterGen::portVelocityBuffer.set(proxies[2]);
    NuPostFilterGen::portDepthRTBuffer.set(proxies[3]);
    NuPostFilterGen::portDepthBuffer.set(proxies[4]);
    NuPostFilterGen *candidates[] = {speedBlurFilter, deferredFilter, mainFilter, motionFilter, motionAccumFilter};
    NuPostFilterGen *filters[5];
    i32 count = 0;
    for (i32 i = 0; i < 5; ++i) {
        if (candidates[i] != NULL && candidates[i]->isEnabled()) {
            filters[count++] = candidates[i];
            NuFramebufferAttachTex2D(candidates[i]->getInputFbo(), 0, proxies[0]->texture, 0);
        }
    }
    g_boundShader = 0;
    glUseProgram(0);
    g_currentShaderProgram = NULL;
    for (i32 i = 0; i < count; ++i) {
        NuPostFilterGen::portOutFramebuffer.set(i + 1 < count ? filters[i + 1]->getInputFbo() : output);
        filters[i]->render();
        filters[i]->reset();
    }
    ++g_effectsRan;
}

extern "C" nueffecttex_s *NuPostEffectGetDepthBuffer(i32 frame) {
    static i32 depthFrameId = -1;
    static nueffecttex_s *depthBufferCopy;
    if (depthFrameId != frame) {
        depthFrameId = frame;
        if (s_proxyDepthBuffer.texture == NULL) {
            NuFramebufferResolve(4, false);
            depthBufferCopy = NuFramebufferGetAttachedTex(NuFramebufferGetBound(), 4, NULL, NULL);
        } else {
            NuPostResolve(&s_proxyDepthBuffer);
            depthBufferCopy = s_proxyDepthBuffer.texture;
        }
    }
    return depthBufferCopy;
}

extern "C" nueffecttex_s *NuPostEffectGetBackBuffer(i32 frame) {
    static i32 backFrameId = -1;
    static nueffecttex_s *backBufferCopy;
    if (backFrameId != frame) {
        backFrameId = frame;
        nuframebuffer_s *bound = NuFramebufferGetBound();
        backBufferCopy = g_backBufferCopy;
        NuFramebufferEnableGuards(bound, true);
        NuFramebufferCopyTex2D(0, backBufferCopy, 0, 0, 0, backBufferCopy->width, backBufferCopy->height);
        NuFramebufferEnableGuards(bound, false);
    }
    return backBufferCopy;
}

extern "C" void NuPostEffectBloom(const NuBloomParameters *parameters) {
    if (mainFilter != NULL)
        mainFilter->bloom = parameters;
}

extern "C" void NuPostEffectDepthOfField(const NuDepthOfFieldParameters *parameters) {
    if (mainFilter == NULL)
        return;
    mainFilter->dof_strength = parameters->strength;
    mainFilter->dof_near = parameters->near_distance;
    mainFilter->dof_far = parameters->far_distance;
    mainFilter->dof_bias = parameters->bias;
    mainFilter->dof_mode = parameters->mode;
}

extern "C" void NuPostEffectDeferredShading(const f32 *parameters) {
    if (deferredFilter == NULL)
        return;
    for (i32 i = 0; i < 4; ++i)
        deferredFilter->parameters[i] = parameters[i];
}

extern "C" void NuPostEffectMotionBlur(const NUMTX *previous, const NUMTX *current, f32 scale, f32 maximum,
                                       f32 falloff) {
    if (mainFilter == NULL)
        return;
    mainFilter->motion_previous = *previous;
    mainFilter->motion_current = *current;
    mainFilter->motion_scale = scale;
    mainFilter->motion_maximum = maximum;
    mainFilter->motion_falloff = falloff;
    motionFilter->previous = *previous;
    motionFilter->current = *current;
    motionFilter->scale = scale;
    motionFilter->maximum = maximum;
    motionFilter->falloff = falloff;
}

extern "C" void NuPostEffectAccumulationMotionBlur(i32 frames, f32 blend, i32 mode) {
    if (mainFilter == NULL)
        return;
    mainFilter->accumulation_frames = frames;
    mainFilter->accumulation_blend = blend;
    mainFilter->accumulation_mode = mode;
    if (motionAccumFilter != NULL) {
        motionAccumFilter->frames = frames;
        motionAccumFilter->blend = blend;
        motionAccumFilter->mode = mode;
    }
}

extern "C" void NuPostEffectSpeedBlur(const NuSpeedBlurParameters *parameters) {
    if (speedBlurFilter != NULL)
        speedBlurFilter->parameters = parameters;
}

extern "C" void NuPostEffectEnable(u32 mask) {
    if ((g_effectFlags & mask) == 0)
        return;
    switch (mask) {
        case 4:
            if (!mainFilter->bloom_enabled) {
                ++mainFilter->active_filter_count;
                mainFilter->bloom_enabled = true;
            }
            break;
        case 8:
            if (!mainFilter->dof_enabled) {
                ++mainFilter->active_filter_count;
                mainFilter->dof_enabled = true;
            }
            break;
        case 0x10:
            motionFilter->enabled = true;
            break;
        case 0x20:
            deferredFilter->enabled = true;
            break;
        case 0x40:
            motionAccumFilter->enabled = true;
            break;
        case 0x80:
            speedBlurFilter->enabled = true;
            break;
    }
}

extern "C" void NuPostEffectDisable(u32 mask) {
    switch (mask) {
        case 4:
            if (mainFilter->bloom_enabled) {
                --mainFilter->active_filter_count;
                mainFilter->bloom_enabled = false;
            }
            break;
        case 8:
            if (mainFilter->dof_enabled) {
                --mainFilter->active_filter_count;
                mainFilter->dof_enabled = false;
            }
            break;
        // The original routes both 0x10 and 0x40 to motionFilter (0x2abaf8).
        case 0x10:
        case 0x40:
            motionFilter->enabled = false;
            break;
        case 0x20:
            deferredFilter->enabled = false;
            break;
        case 0x80:
            speedBlurFilter->enabled = false;
            break;
    }
}

extern "C" bool NuPostEffectIsEnabled(u32 mask) {
    if ((g_effectFlags & mask) == 0)
        return false;
    switch (mask) {
        case 4:
            return mainFilter->bloom_enabled;
        case 8:
            return mainFilter->dof_enabled;
        case 0x10:
            return motionFilter->isEnabled();
        case 0x20:
            return deferredFilter->isEnabled();
        case 0x40:
            return motionAccumFilter->isEnabled();
        case 0x80:
            return speedBlurFilter->isEnabled();
    }
    return false;
}

extern "C" i32 NuPostEffectGetActiveDynamicLightCount() {
    return deferredFilter != NULL ? deferredFilter->dynamic_light_count : 0;
}

extern "C" i32 motionBlurAccumActiveThisFrame;
extern "C" f32 NuPostEffectTiming(i32 *last_frame) {
    if (motionAccumFilter != NULL && motionBlurAccumActiveThisFrame != 0) {
        motionBlurAccumActiveThisFrame = 0;
        return motionAccumFilter->GetTiming(last_frame);
    }
    *last_frame = 1;
    return 0.0f;
}

// ──────────────────────────────────────────────────────────────────────────────
// Framebuffer clear / swap
// ──────────────────────────────────────────────────────────────────────────────

// original 0x2a2720 — Android forwarder; the engine calls this per scene
// when scn.clear_flags != 0 (see nurenderthread.cpp).
extern "C" void NuFramebufferClear(u32 clear_flags, u32 colour) {
    Nu360_dxClear(clear_flags, colour);
}

// original 0x2a2700 — no-op on Android/host.  The actual EGL swap is owned
// by NuRenderDeviceSwapBuffers() on the render thread (nurenderthread.cpp:
// renderThread_main).  Kept as an empty definition so the original call site
// links without ifdef.
extern "C" void NuFramebufferSwapBuffers(void) {
}
