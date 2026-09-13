#include "nu2api/nuandroid/ios_graphics.h"

#include <GLES2/gl2.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <time.h>

#include "decomp.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"

// ---------------------------------------------------------------------------
// ios_graphics — iOS/Android GLES2 platform glue
//
// Original iOS path managed its own EGL / framebuffer and a dedicated render
// thread.  On Android the same code is reused: the "iOS" name is historical.
// This file contains platform plumbing, thread handoff, frame pacing, and
// path helpers.
// ---------------------------------------------------------------------------

// Backing drawable size driven by NuRenderDevice::InitialiseOpenGLContext.
i32 g_backingWidth;
i32 g_backingHeight;

// ---------------------------------------------------------------------------
// Framebuffer setup
// ---------------------------------------------------------------------------

void NuIOSInitOpenGLES(void) {
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nuandroid/ios_graphics.cpp", 308);
    NuIOS_AllocateSystemFramebuffers();
    glFrontFace(GL_CW);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nuandroid/ios_graphics.cpp", 312);
}

SAGA_HOST_WEAK void NuIOS_AllocateSystemFramebuffers(void) {
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nuandroid/ios_graphics.cpp", 106);
    NuCheckGLErrorsFL("i:/SagaTouch-Android_9176564/nu2api.saga/nuandroid/ios_graphics.cpp", 108);

    // Texture-cache shadow state — driver may have been torn down.
    memset(g_lastBound2DTexIds, 0, sizeof(g_lastBound2DTexIds));
    memset(g_lastBoundCubeTexIds, 0, sizeof(g_lastBoundCubeTexIds));

    g_earlyColorFramebuffer = 0;

    if (NuIOS_ShouldUseMSAA()) {
        glGenFramebuffers(1, &g_earlyColorMSAAFramebuffer);
    }

    g_defaultFramebuffer = 0;
    g_currentFramebuffer = 0;

    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nuandroid/ios_graphics.cpp", 260);
}

// ---------------------------------------------------------------------------
// Diagnostics / caps
// ---------------------------------------------------------------------------

i32 NuIOS_IsLowEndDevice(void) {
    return g_isLowEndDevice;
}

i32 NuIOS_ShouldUseMSAA(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// Filesystem shims — original iOS used NSBundle / Documents.
// ---------------------------------------------------------------------------

SAGA_HOST_WEAK char *NuIOS_GetDocumentsPath(void) {
    return "res/";
}

char *NuIOS_GetAppBundlePath(void) {
    static char s_bundlePath[4096];

    if (s_bundlePath[0] == '\0') {
        strcpy(s_bundlePath, "dummyPath");
    }
    return s_bundlePath;
}

i32 g_bgloadAsFastAsPossibleHack;
extern "C" i32 NuIOS_IsMidRangeDevice(void);

u32 NuIOS_YieldThread(void) {
    static u32 count;

    u32 frequency;
    if (NuIOS_IsLowEndDevice()) {
        if (g_bgloadAsFastAsPossibleHack == 0) {
            return sched_yield();
        }
        frequency = 8;
    } else if (NuIOS_IsMidRangeDevice()) {
        frequency = g_bgloadAsFastAsPossibleHack != 0 ? 8 : 2;
    } else {
        frequency = g_bgloadAsFastAsPossibleHack != 0 ? 4 : 2;
    }

    if (++count % frequency == 0) {
        return sched_yield();
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Render-thread handoff
//
// Game thread  ──Wakes──► render thread (via g_wakeRenderThread)
// Render thread ──Signals──► game thread "allowed to render" observability
// Render thread ──Signals──► game thread "frame done" (g_renderThreadDoneThread)
//
// Three independent mutex/cond pairs mirror the original iOS implementation.
// The split keeps the "awaiting wake" debug flag independent from the actual
// wake/completion signals.
// ---------------------------------------------------------------------------

static pthread_mutex_t g_wakeRenderMutex;
static pthread_cond_t g_wakeRenderCondition;
static pthread_mutex_t g_renderThreadDoneThreadMutex;
static pthread_cond_t g_renderThreadDoneThreadCondition;
static pthread_mutex_t g_awaitingRenderWakeMutex;
static pthread_cond_t g_awaitingRenderWakeCondition;
static i32 g_awaitingRenderWake;
static i32 g_wakeRenderThread;
static i32 g_renderThreadDoneThread;

// Timestamp when the current frame's render was kicked — original
// @0x66c9a8, set in NuIOS_WakeRenderThread and sampled in
// NuIOS_WaitForRenderThreadCompletion for 60 Hz pacing.
i64 g_renderStartTime;

// Original 0xe3450 adds raw nanoseconds to seconds scaled by 1000.
extern "C" i64 getCurrentTime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (i64)ts.tv_sec * 1000 + (i64)ts.tv_nsec;
}

void NuIOS_InitRenderThread() {
    pthread_mutex_init(&g_wakeRenderMutex, nullptr);
    pthread_cond_init(&g_wakeRenderCondition, nullptr);
    pthread_mutex_init(&g_renderThreadDoneThreadMutex, nullptr);
    pthread_cond_init(&g_renderThreadDoneThreadCondition, nullptr);
    pthread_mutex_init(&g_awaitingRenderWakeMutex, nullptr);
    pthread_cond_init(&g_awaitingRenderWakeCondition, nullptr);
}

// Render thread: block until the game thread wakes us for the next frame.
void NuIOS_WaitUntilAllowedToRender(void) {
    // Observability: tell the game thread we are now parked and awaiting
    // the wake signal (used by debug overlays / asserts on iOS).
    pthread_mutex_lock(&g_awaitingRenderWakeMutex);
    g_awaitingRenderWake = 1;
    pthread_cond_signal(&g_awaitingRenderWakeCondition);
    pthread_mutex_unlock(&g_awaitingRenderWakeMutex);

    pthread_mutex_lock(&g_wakeRenderMutex);
    while (g_wakeRenderThread == 0) {
        pthread_cond_wait(&g_wakeRenderCondition, &g_wakeRenderMutex);
    }
    g_wakeRenderThread = 0;

    pthread_mutex_lock(&g_awaitingRenderWakeMutex);
    g_awaitingRenderWake = 0;
    pthread_mutex_unlock(&g_awaitingRenderWakeMutex);
    pthread_mutex_unlock(&g_wakeRenderMutex);
}

void NuIOS_SetRenderIncomplete(void) {
    pthread_mutex_lock(&g_renderThreadDoneThreadMutex);
    g_renderThreadDoneThread = 0;
    pthread_mutex_unlock(&g_renderThreadDoneThreadMutex);
}

void NuIOS_SetRenderComplete(void) {
    pthread_mutex_lock(&g_renderThreadDoneThreadMutex);
    if (g_renderThreadDoneThread == 0) {
        g_renderThreadDoneThread = 1;
        pthread_cond_signal(&g_renderThreadDoneThreadCondition);
    }
    pthread_mutex_unlock(&g_renderThreadDoneThreadMutex);
}

// original 0xe34b0 — pace the remainder of the 16 ms frame budget since
// g_renderStartTime was stamped in NuIOS_WakeRenderThread, then join on the
// render-thread completion flag.  Sleeps < 2 ms are skipped to avoid
// nanosleep overhead dominating the frame.
void NuIOS_WaitForRenderThreadCompletion(void) {
    constexpr i64 kFrameBudgetMs = 16;
    constexpr i64 kMinSleepMs = 2;

    i64 elapsed_ms = getCurrentTime();
    elapsed_ms -= g_renderStartTime;
    i64 remaining_ms = kFrameBudgetMs;
    remaining_ms -= elapsed_ms;

    if (remaining_ms >= 0) {
        if (remaining_ms > kFrameBudgetMs) {
            remaining_ms = kFrameBudgetMs;
        }
        if (remaining_ms >= kMinSleepMs) {
            struct timespec sleep_time = {};
            sleep_time.tv_nsec = remaining_ms * 1000000L;
            nanosleep(&sleep_time, nullptr);
        }
    }

    pthread_mutex_lock(&g_renderThreadDoneThreadMutex);
    while (g_renderThreadDoneThread == 0) {
        pthread_cond_wait(&g_renderThreadDoneThreadCondition, &g_renderThreadDoneThreadMutex);
    }
    g_renderThreadDoneThread = 0;
    pthread_mutex_unlock(&g_renderThreadDoneThreadMutex);
}

// original 0xe3590 — kick the render thread for the next frame.
void NuIOS_WakeRenderThread(void) {
    g_renderStartTime = getCurrentTime();

    pthread_mutex_lock(&g_wakeRenderMutex);
    if (g_wakeRenderThread == 0) {
        g_wakeRenderThread = 1;
        pthread_cond_signal(&g_wakeRenderCondition);
    }
    pthread_mutex_unlock(&g_wakeRenderMutex);
}
