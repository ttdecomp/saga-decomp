#include "nu2api/nu3d/NuRenderDevice.h"

#include "globals.h"
#include "nu2api/nu3d/android/NuGLES2Extensions.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nuplatform/nudevicespecs.hpp"
#include "nu2api/nuplatform/nuplatform.h"

#include <GLES2/gl2.h>
#include <pthread.h>
#include <string.h>

// The calling thread's EGL context slot; slot 3 belongs to the main render thread.
thread_local i32 gt_glContextIndex = -1;
i32 g_nextGLContextIndex;

i32 _NuCheckGLErrors(const char * /*file*/) {
    // Original is a no-op in release; kept as a hook for debug builds.
    return 0;
}

void NuRenderDevice::SetThisTreadAsRender() {
    // Historical typo preserved: "Tread" for "Thread". Index 3 is the
    // main render thread's slot.
    gt_glContextIndex = 3;
}

void NuRenderSetThisTreadAsRender() {
    g_renderDevice.SetThisTreadAsRender();
}

void SAGA_HOST_WEAK NuRenderDevice::BeginCriticalSection(const char * /*file*/, i32 /*line*/) {
    pthread_mutex_lock(&this->mutex2);
    const i32 previous_lock_count = this->lock_count++;
    if (previous_lock_count == 0) {
        if (gt_glContextIndex == -1) {
            gt_glContextIndex = g_nextGLContextIndex;
            g_nextGLContextIndex = (g_nextGLContextIndex + 1) % 4;
        }

        LOG_DEBUG("this->egl_display: %p, this->pbuffers[%d]: %p, this->contexts[%d]: %p", this->egl_display,
                  gt_glContextIndex, this->pbuffers[gt_glContextIndex], gt_glContextIndex,
                  this->contexts[gt_glContextIndex]);
        eglMakeCurrent(this->egl_display, this->pbuffers[gt_glContextIndex], this->pbuffers[gt_glContextIndex],
                       this->contexts[gt_glContextIndex]);
    }
}

void BeginCriticalSectionGL(const char *file, i32 line) {
    g_renderDevice.BeginCriticalSection(file, line);
}

void SAGA_HOST_WEAK NuRenderDevice::EndCriticalSection(const char * /*file*/, i32 /*line*/) {
    if (--this->lock_count == 0) {
        const i32 context_index = gt_glContextIndex;
        const bool render_state_requires_detach = static_cast<u32>(this->field50_0x50 - 2) <= 1;
        const i32 application_status = NuCore::GetApplicationState()->GetStatus();
        if (render_state_requires_detach || !this->field54_0x54 || context_index != 0 || application_status == 1) {
            eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
    }
    pthread_mutex_unlock(&this->mutex2);
}

void SAGA_HOST_WEAK NuRenderDevice::SwapBuffers() {
    if (NuCore::GetApplicationState()->GetStatus() == 1) {
        return;
    }

    g_renderDevice.BeginCriticalSection("none", -1);
    eglSwapBuffers(this->egl_display, this->pbuffers[3]);
    g_renderDevice.EndCriticalSection("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp",
                                      0x485);
}

void NuRenderDeviceSwapBuffers() {
    g_renderDevice.SwapBuffers();
}

void EndCriticalSectionGL(const char *file, i32 line) {
    g_renderDevice.EndCriticalSection(file, line);
}

void NuRenderDevice::ResizeDevice(i32 w, i32 h, i32, bool, bool, bool, bool) {
    g_renderDevice.BeginCriticalSection("none", -1);
    width = w;
    height = h;
    DetermineBackBufferResolution(w, h);
    nominal_aspect_ratio = DetermineNominalAspectRatio(width, height);
    aspect_ratio = static_cast<f32>(width) / static_cast<f32>(height);
    g_renderDevice.EndCriticalSection("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp",
                                      0x452);
}

void NuRenderDevice::Initialize() {
    // Wait for the EGL display / window surface to become valid. On
    // Android this is signalled from the Java activity thread via
    // OnWindowCreated() → InitialiseOpenGLContext().
    while (!this->context_valid) {
        NuThreadSleep(1);
    }

    FrameEnd();

    pthread_mutexattr_t attrs;
    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&this->mutex2, &attrs);
    NuGLES2ExtensionsInit();

    BeginCriticalSection("none", -1);

    // Probe the chosen EGL config for logging / diagnostics. The
    // attribute ids are the standard EGL_*_SIZE values:
    //  0x3024 EGL_RED_SIZE, 0x3022 EGL_BLUE_SIZE, 0x3023 EGL_GREEN_SIZE,
    //  0x3021 EGL_ALPHA_SIZE, 0x3025 EGL_DEPTH_SIZE, 0x3026 EGL_STENCIL_SIZE.
    NuRenderInspectEGLConfig(this->egl_display, this->egl_config);

    this->nominal_aspect_ratio = DetermineNominalAspectRatio(this->width, this->height);
    this->aspect_ratio = static_cast<f32>(this->width) / static_cast<f32>(this->height);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &this->max_texture_units);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &this->max_texture_size);

    this->extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

    bool has_dxt1 = false;
    bool has_atc = false;
    bool has_pvrtc = false;
    bool has_etc1 = false;

    if (this->extensions != nullptr) {
        const bool dxt1_ext = IsExtensionSupported("EXT_texture_compression_dxt1");
        const bool dxt1_gl = IsExtensionSupported("GL_EXT_texture_compression_dxt1");
        has_etc1 = IsExtensionSupported("GL_OES_compressed_ETC1_RGB8_texture");
        has_pvrtc = IsExtensionSupported("GL_IMG_texture_compression_pvrtc");
        has_dxt1 = dxt1_ext | dxt1_gl;
        has_atc = IsExtensionSupported("GL_AMD_compressed_ATC_texture");
    }

    memset(this->enabled_extensions, 0, sizeof(this->enabled_extensions));

    // Always available (uncompressed RGBA).
    this->enabled_extensions[NUTEX_RGBA32] = 1;

    // Compressed families — enabled iff the driver advertises support.
    this->enabled_extensions[NUTEX_DXT1] = has_dxt1;
    this->enabled_extensions[NUTEX_DX1A] = has_dxt1;
    this->enabled_extensions[NUTEX_DXT5] = has_dxt1;
    this->enabled_extensions[NUTEX_ETC1] = has_etc1;
    this->enabled_extensions[NUTEX_PVRTC2] = has_pvrtc;
    this->enabled_extensions[NUTEX_PVRTC2A] = has_pvrtc;
    this->enabled_extensions[NUTEX_PVRTC4] = has_pvrtc;
    this->enabled_extensions[NUTEX_PVRTC4A] = has_pvrtc;
    this->enabled_extensions[NUTEX_ATCA] = has_atc;
    this->enabled_extensions[NUTEX_ATC] = has_atc;

    if (this->extensions != nullptr) {
        this->oes_packed_depth_stencil = IsExtensionSupported("GL_OES_packed_depth_stencil");
        this->oes_depth24 = IsExtensionSupported("GL_OES_depth24");
        this->oes_depth_texture = IsExtensionSupported("GL_OES_depth_texture");
    }

    // Choose the runtime texture-compression platform. g_forceETC1 forces
    // ETC1 even when better formats are available; otherwise prefer
    // S3TC → PVRTC → ATC → ETC1.
    if (g_forceETC1 == 0 || !has_etc1) {
        if (has_dxt1) {
            NuPlatform::Get()->SetCurrentPlatform(ANDROID_S3TC_PLATFORM);
        } else if (has_pvrtc) {
            NuPlatform::Get()->SetCurrentPlatform(ANDROID_PVRTC_PLATFORM);
        } else if (has_atc) {
            NuPlatform::Get()->SetCurrentPlatform(ANDROID_ATITC_PLATFORM);
        } else {
            NuPlatform::Get()->SetCurrentPlatform(ANDROID_ETC1_PLATFORM);
        }
    } else {
        NuPlatform::Get()->SetCurrentPlatform(ANDROID_ETC1_PLATFORM);
    }

    NuDeviceSpecs::Create();
    EndCriticalSection("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp", 0x194);

    _NuCheckGLErrors("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp");

    this->value = true;
}
