#include "nu2api/nu3d/NuRenderDevice.h"

#include <emscripten/html5_webgl.h>

#include "decomp.h"
#include "host/platform/graphics.hpp"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/nucore.hpp"

extern u32 g_activeAttributes;

namespace {
    bool host_msaa_enabled = true;

    struct WasmPresentResources {
        GLuint program = 0;
        GLint position = -1;
        GLint texcoord = -1;
        GLint texture = -1;
        GLuint vertex_buffer = 0;
    };

    void wasm_create_present_resources(WasmPresentResources &resources) {
        if (resources.program != 0) {
            return;
        }

        const char *vertex_source =
            "attribute vec2 a_position; attribute vec2 a_texcoord; varying vec2 v_texcoord; "
            "void main() { gl_Position = vec4(a_position, 0.0, 1.0); v_texcoord = a_texcoord; }";
        const char *fragment_source = "precision mediump float; varying vec2 v_texcoord; uniform sampler2D u_texture; "
                                      "void main() { gl_FragColor = texture2D(u_texture, v_texcoord); }";

        const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vertex_source, nullptr);
        glCompileShader(vertex_shader);
        const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_source, nullptr);
        glCompileShader(fragment_shader);

        resources.program = glCreateProgram();
        glAttachShader(resources.program, vertex_shader);
        glAttachShader(resources.program, fragment_shader);
        glBindAttribLocation(resources.program, 0, "a_position");
        glBindAttribLocation(resources.program, 1, "a_texcoord");
        glLinkProgram(resources.program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        resources.position = glGetAttribLocation(resources.program, "a_position");
        resources.texcoord = glGetAttribLocation(resources.program, "a_texcoord");
        resources.texture = glGetUniformLocation(resources.program, "u_texture");

        const f32 vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
        };
        glGenBuffers(1, &resources.vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    void wasm_present_texture(GLuint texture, i32 width, i32 height) {
        static WasmPresentResources resources;
        wasm_create_present_resources(resources);

        for (u32 attribute = 0, mask = g_activeAttributes; mask != 0; ++attribute, mask >>= 1) {
            if ((mask & 1) != 0) {
                glDisableVertexAttribArray(attribute);
            }
        }
        g_activeAttributes = 0;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glUseProgram(resources.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(resources.texture, 0);
        glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
        glEnableVertexAttribArray(resources.position);
        glVertexAttribPointer(resources.position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), nullptr);
        glEnableVertexAttribArray(resources.texcoord);
        glVertexAttribPointer(resources.texcoord, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32),
                              reinterpret_cast<void *>(2 * sizeof(f32)));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(resources.position);
        glDisableVertexAttribArray(resources.texcoord);
        glUseProgram(0);
    }
} // namespace

void HostSetMsaaEnabled(bool enabled) {
    host_msaa_enabled = enabled;
}

void NuRenderInspectEGLConfig(EGLDisplay, EGLConfig) {
}

void NuRenderDevice::BeginCriticalSection(const char *, i32) {
    pthread_mutex_lock(&this->mutex2);
    const i32 previous_lock_count = this->lock_count++;
    if (previous_lock_count == 0) {
        if (gt_glContextIndex == -1) {
            gt_glContextIndex = g_nextGLContextIndex;
            g_nextGLContextIndex = (g_nextGLContextIndex + 1) % 4;
        }
        emscripten_webgl_make_context_current(reinterpret_cast<uintptr_t>(this->contexts[gt_glContextIndex]));
    }
}

void NuRenderDevice::EndCriticalSection(const char *, i32) {
    --this->lock_count;
    pthread_mutex_unlock(&this->mutex2);
}

void NuRenderDevice::SwapBuffers() {
    if (NuCore::GetApplicationState()->GetStatus() == 1) {
        return;
    }

    const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = reinterpret_cast<uintptr_t>(this->contexts[3]);
    if (context != 0 && emscripten_webgl_make_context_current(context) == EMSCRIPTEN_RESULT_SUCCESS) {
        i32 width = static_cast<i32>(this->width);
        i32 height = static_cast<i32>(this->height);
        emscripten_webgl_get_drawing_buffer_size(context, &width, &height);
        if (g_earlyColorTexture != 0 && glIsTexture(g_earlyColorTexture)) {
            wasm_present_texture(g_earlyColorTexture, width, height);
        }
        emscripten_webgl_commit_frame();
    }
}

void NuRenderDevice::InitialiseOpenGLContext(ANativeWindow *) {
    pthread_mutex_lock(&this->mutex);
    if (!this->context_valid) {
        EmscriptenWebGLContextAttributes attributes;
        emscripten_webgl_init_context_attributes(&attributes);
        attributes.alpha = false;
        attributes.depth = true;
        attributes.stencil = false;
        attributes.antialias = host_msaa_enabled;
        attributes.majorVersion = 2;
        attributes.minorVersion = 0;
        attributes.explicitSwapControl = true;
        attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
        attributes.renderViaOffscreenBackBuffer = true;

        const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#canvas", &attributes);
        if (context == 0 || emscripten_webgl_make_context_current(context) != EMSCRIPTEN_RESULT_SUCCESS) {
            LOG_ERR("failed to create WebGL context");
            pthread_mutex_unlock(&this->mutex);
            return;
        }

        const EGLContext stored_context = reinterpret_cast<EGLContext>(context);
        const EGLSurface stored_surface = reinterpret_cast<EGLSurface>(static_cast<uintptr_t>(1));
        for (i32 i = 0; i < 4; ++i) {
            this->contexts[i] = stored_context;
            this->pbuffers[i] = stored_surface;
        }
        this->egl_display = reinterpret_cast<EGLDisplay>(static_cast<uintptr_t>(1));

        i32 width = 0;
        i32 height = 0;
        emscripten_webgl_get_drawing_buffer_size(context, &width, &height);
        this->width = static_cast<u32>(width);
        this->height = static_cast<u32>(height);
        DetermineBackBufferResolution(width, height);
        g_backingWidth = static_cast<i32>(this->backing_width);
        g_backingHeight = static_cast<i32>(this->backing_height);
        nurndr_pixel_width = width;
        nurndr_pixel_height = height;
        this->context_valid = true;
        emscripten_webgl_make_context_current(0);
    }
    pthread_mutex_unlock(&this->mutex);
}
