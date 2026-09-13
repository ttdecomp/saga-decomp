#include "nu2api/nu3d/NuRenderDevice.h"

#include <SDL3/SDL.h>

#include "decomp.h"
#include "globals.h"
#include "host/platform/graphics.hpp"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nustring.h"

namespace {
    SDL_Window *host_window;
    SDL_GLContext host_main_context;
} // namespace

void HostSetSDLGraphics(SDL_Window *window, void *context) {
    host_window = window;
    host_main_context = reinterpret_cast<SDL_GLContext>(context);
}

extern "C" f32 __real_NuFrameEnd(void);

extern "C" f32 __wrap_NuFrameEnd(void) {
    if (nuapi.max_fps != 0) {
        HostPaceFrame(&nuapi.time, 1.0f / static_cast<f32>(nuapi.max_fps));
    }
    return __real_NuFrameEnd();
}

void NuRenderDevice::InitialiseOpenGLContext(ANativeWindow *window) {
    pthread_mutex_lock(&this->mutex);

    this->is_not_amazon_kf =
        (NuStrICmp(g_deviceManufacturer, "Amazon") != 0 || NuStrNICmp(g_deviceModel, "KF", 2) != 0);

    if (!this->context_valid) {
        this->native_window = reinterpret_cast<EGLNativeWindowType>(window);
        this->contexts[3] = reinterpret_cast<EGLContext>(host_main_context);

        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        for (i32 index = 0; index < 3; ++index) {
            SDL_GL_MakeCurrent(host_window, host_main_context);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
            SDL_GLContext context = SDL_GL_CreateContext(host_window);
            if (context == nullptr) {
                LOG_ERR("SDL_GL_CreateContext(%d) failed: %s", index, SDL_GetError());
                SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);
                pthread_mutex_unlock(&this->mutex);
                return;
            }
            this->contexts[index] = reinterpret_cast<EGLContext>(context);
        }
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

        SDL_GL_MakeCurrent(host_window, host_main_context);
        if (!SDL_GL_SetSwapInterval(0)) {
            LOG_WARN("SDL_GL_SetSwapInterval(0) failed: %s", SDL_GetError());
        }
        i32 drawable_width = 0;
        i32 drawable_height = 0;
        SDL_GetWindowSizeInPixels(host_window, &drawable_width, &drawable_height);
        this->width = static_cast<u32>(drawable_width);
        this->height = static_cast<u32>(drawable_height);
        DetermineBackBufferResolution(drawable_width, drawable_height);

        g_backingWidth = static_cast<i32>(this->backing_width);
        g_backingHeight = static_cast<i32>(this->backing_height);
        nurndr_pixel_width = static_cast<i32>(this->width);
        nurndr_pixel_height = static_cast<i32>(this->height);
        this->context_valid = true;
        SDL_GL_MakeCurrent(host_window, nullptr);
    }

    pthread_mutex_unlock(&this->mutex);
}

void NuRenderDevice::BeginCriticalSection(const char *, i32) {
    pthread_mutex_lock(&this->mutex2);
    const i32 previous_lock_count = this->lock_count++;
    if (previous_lock_count != 0) {
        return;
    }
    if (gt_glContextIndex == -1) {
        gt_glContextIndex = g_nextGLContextIndex;
        g_nextGLContextIndex = (g_nextGLContextIndex + 1) % 4;
    }
    SDL_GLContext context = reinterpret_cast<SDL_GLContext>(this->contexts[gt_glContextIndex]);
    if (!SDL_GL_MakeCurrent(host_window, context)) {
        LOG_ERR("SDL_GL_MakeCurrent(%d) failed: %s", gt_glContextIndex, SDL_GetError());
    }
}

void NuRenderDevice::EndCriticalSection(const char *, i32) {
    if (--this->lock_count == 0) {
        const i32 context_index = gt_glContextIndex;
        const bool render_state_requires_detach = static_cast<u32>(this->field50_0x50 - 2) <= 1;
        const i32 application_status = NuCore::GetApplicationState()->GetStatus();
        if (render_state_requires_detach || !this->field54_0x54 || context_index != 0 || application_status == 1) {
            SDL_GL_MakeCurrent(host_window, nullptr);
        }
    }
    pthread_mutex_unlock(&this->mutex2);
}

void NuRenderDevice::SwapBuffers() {
    if (NuCore::GetApplicationState()->GetStatus() == 1) {
        return;
    }

    BeginCriticalSection("none", -1);
    i32 width = 0;
    i32 height = 0;
    SDL_GetWindowSizeInPixels(host_window, &width, &height);
    HostCaptureCurrentFramebuffer(width, height);
    SDL_GL_SwapWindow(host_window);
    EndCriticalSection("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp", 0x485);
}
