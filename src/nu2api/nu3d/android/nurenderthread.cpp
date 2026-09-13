// Render-thread scene processing — transcribed originals.
//
//   NuRenderThreadPrepareRender        original 0x2a5ee0 (game thread)
//   NuRenderThreadStartRender          original 0x2a6080 (game thread)
//   renderThread_processRenderScenes   original 0x2a61a0 (render thread)

#include "nu2api/nu3d/android/nurenderthread.h"

#include <GLES2/gl2.h>
#include <pthread.h>
#include <string.h>

#include "decomp.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nu3d/android/nuposteffect_plain.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nuthread.h"

static volatile i32 renderThreadCS;
i64 getCurrentTime();
static i32 renderThreadIsLocked;
pthread_t g_renderThread;
thread_local i32 gt_currentThreadId = -1;

extern "C" i32 NuRenderThreadIsCurrentThread(void) {
    return gt_currentThreadId == 0;
}

// Original file-static double buffers (bss 0x119db.. / 0x119fd..).
static nurenderscene_s sceneParameters_safe[16];
static i32 sceneParametersCount_safe;
static void *dynamicLights_safe[64];
static i32 dynamicLightsCount_safe;

// Special-vertex offset table copied for the render thread every frame.
extern "C" {
    VARIPTR nuspecial_vertex_offsets;
    i32 nuspecial_vertex_noffsets;
}

// Game-thread scene-parameter queue (defined in nurndr_plain.cpp).
extern "C" {
    extern struct nurenderscene_s sceneParameters[16];
    extern i32 sceneParametersCount;
}
static VARIPTR nuspecial_vertex_offsets_safe;
static i32 nuspecial_vertex_noffsets_safe;

// Render-context accumulators reset/read by the frame tail (bss 0x119bb..).
extern "C" {
    f32 g_renderContext_gpuTime;
    f32 g_renderContext_postEffectTime;
    f32 g_renderContext_3dTime;
    f32 g_renderContext_kTint[4];
}
extern const f32 nuvec4_one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
f32 g_renderContext_viewProj[16];
f32 g_renderContext_viewProjInverse[16];
f32 g_renderContext_view[16];
f32 g_renderContext_projection[16];
f32 g_renderContext_world[16];
f32 g_renderContext_position[4];

extern "C" i32 NuDynamicLightIsEnabled(void *light) {
    return light != NULL;
}
extern "C" void NuShaderManagerSetfv(i32 id, const f32 *values);
NUNATIVETEX *NuTexGetNative(i32 tex_id);
void NuIOS_CopyBackbufferToTexture(NUNATIVETEX *tex, bool depth);
extern "C" i32 NuDynamicLightIsEnabled(void *light);
void NuDisplayListDrawRenderScene(i32 render_scene_id);
extern "C" void NuDisplayListDraw2D(void);
i32 global_frame_count = 0;
i32 global_frame_count_paused = 0;

extern "C" void NuRenderThreadLock(void) {
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nurenderthread.cpp", 134);
    NuThreadCriticalSectionBegin(renderThreadCS);
    renderThreadIsLocked++;
}

extern "C" void NuRenderThreadUnlock(void) {
    renderThreadIsLocked--;
    NuThreadCriticalSectionEnd(renderThreadCS);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nurenderthread.cpp", 153);
}

extern "C" i32 NuRenderThreadIsLocked(void) {
    return renderThreadIsLocked;
}

void NuRenderThreadCreate(void) {
    NuIOS_InitRenderThread();
    pthread_create(&g_renderThread, NULL, renderThread_main, NULL);
    renderThreadCS = NuThreadCreateCriticalSection();
}

void NuRenderThreadDestroy(void) {
    NuThreadDestroyCriticalSection(renderThreadCS);
}

void *renderThread_main(void *arg) {
    (void)arg;
    NuRenderSetThisTreadAsRender();
    NuIOSInitOpenGLES();

    for (;;) {
        NuIOS_WaitUntilAllowedToRender();
        NuIOS_SetRenderIncomplete();
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nurenderthread.cpp", 259);
        NuRenderThreadLock();
        glFrontFace(GL_CW);
        renderThread_processRenderScenes();
        NuRenderThreadUnlock();
        NuRenderDeviceSwapBuffers();
        NuIOS_SetRenderComplete();
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nurenderthread.cpp", 269);
    }
}

// original 0x2a5ee0 — runs on the GAME thread from NuRndrSwapScreen: snapshots
// the scene-parameter array into the safe double buffer, collects the dynamic
// lights referenced by those scenes and resets the game-side counter.
extern "C" void NuRenderThreadPrepareRender(void) {
    // Align the display-list stream cursor (original rounds up to 16).
    display_list_buffer->addr = (usize)((display_list_buffer->addr + 0xf) & ~(usize)0xf);

    nuspecial_vertex_noffsets_safe = nuspecial_vertex_noffsets;
    nuspecial_vertex_offsets_safe.addr = display_list_buffer->addr;
    display_list_buffer->addr += (usize)nuspecial_vertex_noffsets << 4;
    SAGA_HOST_SAFE_MEMMOVE(nuspecial_vertex_offsets_safe.void_ptr, nuspecial_vertex_offsets.void_ptr,
                           (usize)nuspecial_vertex_noffsets << 4);

    dynamicLightsCount_safe = 0;
    sceneParametersCount_safe = sceneParametersCount;

    for (i32 i = 0; i < sceneParametersCount; i++) {
        sceneParameters_safe[i] = sceneParameters[i];

        if (sceneParameters[i].unknown_38 != 0) {
            void *light = sceneParameters[i].unknown_3c;
            if (NuDynamicLightIsEnabled(light)) {
                bool found = false;
                for (i32 j = 0; j < dynamicLightsCount_safe; j++) {
                    if (dynamicLights_safe[j] == light) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    dynamicLights_safe[dynamicLightsCount_safe++] = light;
                }
            }
        }
    }
    sceneParametersCount = 0;
}

// original 0x2a6080
extern "C" void NuRenderThreadStartRender(void) {
    g_renderStartTime = getCurrentTime();
    NuIOS_WakeRenderThread();
}

// original 0x2a61a0 — runs on the RENDER thread. Drains the safe scene list:
// invalidates post-effect slots whose effects are not initialised, feeds the
// dynamic lights to the post-effect system, binds the early-colour (MSAA)
// framebuffer, then per scene: optional clear, optional render-scene draw,
// optional backbuffer copy-back into a texture. Finishes with Draw2D, the
// post-effect end and the frame bookkeeping/tail state resets.
i32 renderThread_processRenderScenes(void) {
    static f32 times;
    bool drew = false;

    NuThreadCriticalSectionBegin(renderThreadCS);
    renderThreadIsLocked++;
    times += nuapi.frametime;
    _NuTimeBarSlotBegin(-1, 4, "CPU_QUEUE_DRAW");

    for (i32 i = 0; i < sceneParametersCount_safe; i++) {
        nurenderscene_s &scn = sceneParameters_safe[i];

        if (scn.unknown_48 != 0 && !NuPostEffectIsInitialised(0x20))
            scn.unknown_48 = 0;
        if (scn.unknown_58 != 0 && !NuPostEffectIsInitialised(4))
            scn.unknown_58 = 0;
        if (scn.unknown_ac != 0 && !NuPostEffectIsInitialised(8))
            scn.unknown_ac = 0;
        if (scn.unknown_e4 != 0 && !NuPostEffectIsInitialised(0x10))
            scn.unknown_e4 = 0;
        if (scn.unknown_178 != 0 && !NuPostEffectIsInitialised(0x40))
            scn.unknown_178 = 0;
        if (scn.unknown_188 != 0 && !NuPostEffectIsInitialised(0x80))
            scn.unknown_188 = 0;
    }

    NuPostEffectReset();
    for (i32 i = 0; i < dynamicLightsCount_safe; i++) {
        NuPostEffectAddDynamicLight(dynamicLights_safe[i]);
    }

    g_currentFramebuffer = g_earlyColorFramebuffer;
    if (NuIOS_ShouldUseMSAA()) {
        g_currentFramebuffer = g_earlyColorMSAAFramebuffer;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, g_currentFramebuffer);
    glViewport(0, 0, g_backingWidth, g_backingHeight);

    for (i32 i = 0; i < sceneParametersCount_safe; i++) {
        nurenderscene_s &scn = sceneParameters_safe[i];

        if (scn.clear_flags != 0) {
            NuFramebufferClear(scn.clear_flags, scn.bg_colour);
            drew = true;
        }
        if (scn.render_scene_id != -1) {
            NuDisplayListDrawRenderScene(scn.render_scene_id);
            drew = true;
        }
        if (scn.unknown_214 != 0) {
            NUNATIVETEX *tex = NuTexGetNative((i32)scn.unknown_214);
            NuIOS_CopyBackbufferToTexture(tex, true);
        }
    }

    NuDisplayListDraw2D();
    NuPostEffectEnd();

    if (global_frame_count_paused == 0) {
        global_frame_count++;
    }

    i64 cpu_ns = _NuTimeBarSlotEnd(-1, 4);
    f32 cpu_ms = (f32)((f64)(i32)cpu_ns / 1e6);

    NuFramebufferSwapBuffers();
    g_boundShader = 0;
    glUseProgram(0);
    g_currentShaderProgram = 0;
    g_renderContext_kTint[0] = nuvec4_one[0];
    g_renderContext_kTint[1] = nuvec4_one[1];
    g_renderContext_kTint[2] = nuvec4_one[2];
    g_renderContext_kTint[3] = nuvec4_one[3];
    NuShaderManagerSetfv(0x44, &nuvec4_one[0]);

    f32 cpu_max_ms = cpu_ms > 0.0f ? cpu_ms : 0.0f;
    NuTimeBarSlotReset(-1, 1);
    NuTimeBarSlotSet(-1, 1, (i32)(g_renderContext_gpuTime * 1e6));
    NuTimeBarSlotSetName(-1, 1, "GPU(TOTAL)");
    NuTimeBarSlotReset(-1, 3);
    NuTimeBarSlotSet(-1, 3, (i32)(g_renderContext_postEffectTime * 1e6));
    NuTimeBarSlotSetName(-1, 3, "GPU(FX)");
    NuTimeBarSlotReset(-1, 2);
    NuTimeBarSlotSet(-1, 2, (i32)(g_renderContext_3dTime * 1e6));
    NuTimeBarSlotSetName(-1, 2, "GPU(3D)");
    NuTimeBarSlotReset(-1, 0);
    NuTimeBarSlotSet(-1, 0, (i32)(cpu_max_ms * 1e6));
    NuTimeBarSlotSetName(-1, 0, "CPUs(max)");

    renderThreadIsLocked--;
    NuThreadCriticalSectionEnd(renderThreadCS);
    sceneParametersCount_safe = 0;
    return drew ? 1 : 0;
}
