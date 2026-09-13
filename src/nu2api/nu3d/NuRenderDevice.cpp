#include "nu2api/nu3d/NuRenderDevice.h"

#include "decomp.h"
#include "globals.h"
#include "java/native_window.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nuplatform/nudevicespecs.hpp"
#include "nu2api/nuplatform/nuplatform.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

// ---------------------------------------------------------------------------
// NuRenderDevice — Android GLES2 render device
//
// Original TU: nu2api.saga/nu3d/android/NuRenderDevice_gles2.cpp
//  (see doc/source-structure.md). This file merges the device lifetime,
//  EGL config/context management, extension probing, and the re-entrant
//  GL critical section that serialises all GL calls on Android.
// ---------------------------------------------------------------------------

NuRenderDevice g_renderDevice{};

// Android render work is dispatched across a small ring of shared EGL
// contexts (indices 0..3). Index 3 is the "main" / window context; the
// others are worker contexts. gt_glContextIndex tracks which slot the
// calling thread was assigned.
thread_local i32 gt_glContextIndex = -1;

i32 g_nextGLContextIndex;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

NuRenderDeviceGen::NuRenderDeviceGen() : value(false) {
}

static inline void InitRecursiveMutex(pthread_mutex_t *mutex) {
    pthread_mutexattr_t attrs;
    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mutex, &attrs);
    pthread_mutexattr_destroy(&attrs);
}

NuRenderDevice::NuRenderDevice() : NuRenderDeviceGen() {
    this->lock_count = 0;
    InitRecursiveMutex(&this->mutex);
    this->field10_0x10 = 0;
    this->field14_0x14 = 0;

    // is_not_amazon_kf — true unless the device is an Amazon Kindle Fire
    // (manufacturer "Amazon" + model prefix "KF"). Kindle Fire gets a
    // down-scaled backbuffer path in DetermineBackBufferResolution().
    this->is_not_amazon_kf = true;
    this->field48_0x45 = true;
    this->focus = false;
    this->nominal_aspect_ratio = 0;
    this->aspect_ratio = 1.3333334f;
    this->context_valid = false;
    this->egl_display = EGL_NO_DISPLAY;
    this->current_context_index = -1;

    for (i32 i = 0; i < 4; i++) {
        this->contexts[i] = EGL_NO_CONTEXT;
    }

    // field54_0x54 — original bool that selects whether worker contexts
    // get private 1x1 pbuffers (true) or alias the window surface (false).
    // Always true on this build.
    this->field54_0x54 = true;

    this->backing_width = 1280;
    this->width = 1280;
    this->backing_height = 720;
    this->height = 720;
}

// ---------------------------------------------------------------------------
// Optional GLES2 extensions (loaded via eglGetProcAddress)
// ---------------------------------------------------------------------------

void NuGLES2ExtensionsInit();

__attribute__((weak)) void NuRenderInspectEGLConfig(EGLDisplay display, EGLConfig config) {
    EGLint config_attribs[6] = {};
    eglGetConfigAttrib(display, config, EGL_RED_SIZE, &config_attribs[0]);
    eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &config_attribs[1]);
    eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &config_attribs[2]);
    eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE, &config_attribs[3]);
    eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &config_attribs[4]);
    eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &config_attribs[5]);
}

// ---------------------------------------------------------------------------
// Extension string helpers
// ---------------------------------------------------------------------------

bool NuRenderDevice::IsExtensionSupported(const char *wanted) {
    // Reject malformed queries: empty string or embedded space would
    // otherwise match substrings incorrectly.
    if (wanted[0] == '\0' || strchr(wanted, ' ') != nullptr) {
        return false;
    }

    const usize wanted_len = strlen(wanted);
    const char *cursor = this->extensions;

    while (true) {
        const char *found = strstr(cursor, wanted);
        if (found == nullptr) {
            return false;
        }

        const bool at_start = (found == cursor);
        const bool preceded_by_space = at_start || found[-1] == ' ';
        const char terminator = found[wanted_len];
        // Terminator is either end-of-string ('\0'), space, or other
        // whitespace — the original uses a case-insensitive space check
        // via (c & 0xDF) != 0 which effectively treats '\0' as
        // terminator and space as boundary.
        const bool followed_by_boundary = (terminator == '\0' || terminator == ' ');

        if (preceded_by_space && followed_by_boundary) {
            return true;
        }

        // Whole-word match failed — continue searching past this occurrence.
        cursor = found + wanted_len;
    }
}

i32 _NuCheckGLErrors(const char * /*file*/) {
    // Original is a no-op in release; kept as a hook for debug builds.
    return 0;
}

// ---------------------------------------------------------------------------
// Device bring-up
// ---------------------------------------------------------------------------

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

void NuRenderDevice::FrameEnd() {
    // No-op in this build — original flushed per-frame bookkeeping.
}

void NuRenderDevice::SetThisTreadAsRender() {
    // Historical typo preserved: "Tread" for "Thread". Index 3 is the
    // main render thread's slot.
    gt_glContextIndex = 3;
}

// ---------------------------------------------------------------------------
// Re-entrant GL critical section
// ---------------------------------------------------------------------------

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

void NuRenderDevice::OnWindowCreated(ANativeWindow *window) {
    InitialiseOpenGLContext(window);
    CheckForRenderWindowInitialisation();
}

// ---------------------------------------------------------------------------
// EGL config selection + backbuffer sizing
// ---------------------------------------------------------------------------

EGLConfig SAGA_HOST_WEAK NuRenderDevice::SelectEGLConfig() {
    // Preferred EGL config: 565 colour, 24-bit depth, GLES2 conformant,
    // pbuffer + window capable.
    pthread_mutex_lock(&this->mutex);
    EGLint attrib_list[] = {
        EGL_DEPTH_SIZE,
        24, //
        EGL_LEVEL,
        0, //
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_CONFORMANT,
        EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE,
        5, //
        EGL_GREEN_SIZE,
        6, //
        EGL_RED_SIZE,
        5, //
        EGL_ALPHA_SIZE,
        0, //
        EGL_STENCIL_SIZE,
        0, //
        EGL_NONE,
    };

    EGLConfig configs[32];
    i32 num_configs;
    EGLBoolean ok = eglChooseConfig(this->egl_display, attrib_list, configs, 32, &num_configs);

    if (num_configs == 0 || ok == EGL_FALSE) {
        // Retry with 16-bit depth if 24-bit is unavailable (older devices).
        attrib_list[1] = 16; // EGL_DEPTH_SIZE value slot
        eglChooseConfig(this->egl_display, attrib_list, configs, 32, &num_configs);
    }

    pthread_mutex_unlock(&this->mutex);

    return configs[0];
}

void NuRenderDevice::DetermineBackBufferResolution(i32 width, i32 height) {
    this->backing_width = static_cast<u32>(width);
    this->backing_height = static_cast<u32>(height);

    NuDeviceSpecs::Create();
    // On low-spec non-Kindle devices, clamp width to 1280 to keep fill
    // rate in check and scale height to preserve aspect.
    if (this->is_not_amazon_kf && NuDeviceSpecs::ms_instance->specs < 3 && this->backing_width > 1280) {
        this->backing_width = 1280;
        this->backing_height = static_cast<u32>(static_cast<f32>(height) / static_cast<f32>(width) * 1280.0f);
    }
}

void SAGA_HOST_WEAK NuRenderDevice::InitialiseOpenGLContext(ANativeWindow *window_) {
    EGLNativeWindowType window = reinterpret_cast<EGLNativeWindowType>(window_);

    pthread_mutex_lock(&this->mutex);

    this->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // Re-evaluate Kindle Fire detection from the live device properties.
    this->is_not_amazon_kf =
        (NuStrICmp(g_deviceManufacturer, "Amazon") != 0 || NuStrNICmp(g_deviceModel, "KF", 2) != 0);

    LOG_DEBUG("this->context_valid: %d, this->native_window: %p, window: %p", this->context_valid, this->native_window,
              window);

    if (!this->context_valid) {
        this->native_window = window;

        EGLint major = 0;
        EGLint minor = 0;
        eglInitialize(this->egl_display, &major, &minor);
        eglBindAPI(EGL_OPENGL_ES_API);

        this->egl_config = SelectEGLConfig();

        this->pbuffers[3] = eglCreateWindowSurface(this->egl_display, this->egl_config, this->native_window, nullptr);

        this->attrib_list[0] = EGL_CONTEXT_CLIENT_VERSION;
        this->attrib_list[1] = 2;
        this->attrib_list[2] = EGL_NONE;

        this->contexts[3] = eglCreateContext(this->egl_display, this->egl_config, EGL_NO_CONTEXT, this->attrib_list);

        // Worker contexts share resources with the main context. When
        // field54_0x54 is set they each get a private 1x1 pbuffer so GL
        // calls don't need the window surface.
        if (this->field54_0x54) {
            const EGLint pbuffer_attribs[] = {
                EGL_WIDTH,          1, //
                EGL_HEIGHT,         1, //
                EGL_TEXTURE_TARGET, EGL_NO_TEXTURE,
                EGL_TEXTURE_FORMAT, EGL_NO_TEXTURE,
                EGL_NONE,
            };
            this->pbuffers[0] = eglCreatePbufferSurface(this->egl_display, this->egl_config, pbuffer_attribs);
            this->contexts[0] =
                eglCreateContext(this->egl_display, this->egl_config, this->contexts[3], this->attrib_list);
            this->pbuffers[1] = eglCreatePbufferSurface(this->egl_display, this->egl_config, pbuffer_attribs);
            this->contexts[1] =
                eglCreateContext(this->egl_display, this->egl_config, this->contexts[3], this->attrib_list);
            this->pbuffers[2] = eglCreatePbufferSurface(this->egl_display, this->egl_config, pbuffer_attribs);
        } else {
            // Alias the window surface.
            this->pbuffers[0] = this->pbuffers[3];
            this->contexts[0] =
                eglCreateContext(this->egl_display, this->egl_config, this->contexts[3], this->attrib_list);
            this->pbuffers[1] = this->pbuffers[3];
            this->contexts[1] =
                eglCreateContext(this->egl_display, this->egl_config, this->contexts[3], this->attrib_list);
            this->pbuffers[2] = this->pbuffers[3];
        }

        this->contexts[2] = eglCreateContext(this->egl_display, this->egl_config, this->contexts[3], this->attrib_list);

        // Make worker 0 current briefly to query the actual window size
        // and set up the nominal backbuffer dimensions.
        eglMakeCurrent(this->egl_display, this->pbuffers[0], this->pbuffers[0], this->contexts[0]);
        i32 drawable_w = 0;
        i32 drawable_h = 0;
        eglQuerySurface(this->egl_display, this->pbuffers[3], EGL_WIDTH, &drawable_w);
        eglQuerySurface(this->egl_display, this->pbuffers[3], EGL_HEIGHT, &drawable_h);
        this->width = static_cast<u32>(drawable_w);
        this->height = static_cast<u32>(drawable_h);
        DetermineBackBufferResolution(drawable_w, drawable_h);

        EGLint visual_id;
        eglGetConfigAttrib(this->egl_display, this->egl_config, EGL_NATIVE_VISUAL_ID, &visual_id);
        ANativeWindow_setBuffersGeometry(reinterpret_cast<ANativeWindow *>(this->native_window), this->backing_width,
                                         this->backing_height, visual_id);
        eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        g_backingWidth = static_cast<i32>(this->backing_width);
        g_backingHeight = static_cast<i32>(this->backing_height);
        nurndr_pixel_width = static_cast<i32>(this->width);
        nurndr_pixel_height = static_cast<i32>(this->height);

        this->context_valid = true;

    } else if (this->native_window != window) {
        // Keep the shared contexts and replace the Android window surface.
        eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(this->egl_display, this->pbuffers[3]);
        eglGetError();
        this->egl_config = SelectEGLConfig();
        this->pbuffers[3] = eglCreateWindowSurface(this->egl_display, this->egl_config, window, nullptr);
        this->native_window = window;
        if (this->pbuffers[3] != EGL_NO_SURFACE) {
            if (!eglMakeCurrent(this->egl_display, this->pbuffers[3], this->pbuffers[3], this->contexts[3])) {
                eglGetError();
            }
        }
        eglMakeCurrent(this->egl_display, this->pbuffers[3], this->pbuffers[3], this->contexts[3]);
        eglQuerySurface(this->egl_display, this->pbuffers[3], EGL_WIDTH,
                        reinterpret_cast<EGLint *>(&this->drawable_width));
        eglQuerySurface(this->egl_display, this->pbuffers[3], EGL_HEIGHT,
                        reinterpret_cast<EGLint *>(&this->drawable_height));
        this->width = this->drawable_width;
        this->height = this->drawable_height;
        DetermineBackBufferResolution(this->width, this->height);
        EGLint visual_id;
        eglGetConfigAttrib(this->egl_display, this->egl_config, EGL_NATIVE_VISUAL_ID, &visual_id);
        ANativeWindow_setBuffersGeometry(reinterpret_cast<ANativeWindow *>(this->native_window), this->backing_width,
                                         this->backing_height, visual_id);
        eglMakeCurrent(this->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    LOG_DEBUG("this->egl_display: %p, this->pbuffers = {%p, %p, %p, %p}, this->contexts = {%p, %p, %p, %p}",
              this->egl_display, this->pbuffers[0], this->pbuffers[1], this->pbuffers[2], this->pbuffers[3],
              this->contexts[0], this->contexts[1], this->contexts[2], this->contexts[3]);

    pthread_mutex_unlock(&this->mutex);
}

void NuRenderDevice::CheckForRenderWindowInitialisation() {
    // Once the window is live and focused, transition the application
    // state machine so the engine starts submitting frames.
    if (g_appWindow != 0 && this->field48_0x45 == '\0' && this->focus) {
        if (NuCore::GetApplicationState()->GetStatus() != NUAPPLICATIONSTATUS{}) {
            NuCore::GetApplicationState()->SetStatus(NUAPPLICATIONSTATUS{});
        }
    }
}

void NuRenderDevice::OnWindowDestroy() {
    NuCore::GetApplicationState()->SetStatus(NUAPPLICATIONSTATUS_RENDERING);
}

void NuRenderDevice::OnAppResume() {
    field48_0x45 = false;
    CheckForRenderWindowInitialisation();
}

void NuRenderDevice::OnAppPaused() {
    NuCore::GetApplicationState()->SetStatus(NUAPPLICATIONSTATUS_RENDERING);
    field48_0x45 = true;
}

void NuRenderDevice::OnGainedFocus() {
    focus = true;
    CheckForRenderWindowInitialisation();
}

void NuRenderDevice::OnLostFocus() {
    focus = false;
    NuCore::GetApplicationState()->SetStatus(NUAPPLICATIONSTATUS_RENDERING);
}

void NuRenderDevice::OnAppStarted() {
}

void NuRenderDevice::OnAppRestarted() {
    CheckForRenderWindowInitialisation();
}

void NuRenderDevice::OnAppStopped() {
    NuCore::GetApplicationState()->SetStatus(NUAPPLICATIONSTATUS_RENDERING);
}

bool NuRenderDevice::IsContextValid() const {
    return context_valid;
}

bool NuRenderDevice::MultiThreadRender() const {
    return field50_0x50 == 1 || field50_0x50 == 2;
}

i32 NuRenderDevice::DetermineNominalAspectRatio(u32 w, u32 h) const {
    f32 ratio = static_cast<f32>(w) / static_cast<f32>(h);
    i32 result = 0;
    f32 error = MIN(fabsf(ratio - 1.3333333730697632f), 1000.0f);
    f32 widescreen_error = fabsf(ratio - 1.7777777910232544f);
    if (error > widescreen_error) {
        error = widescreen_error;
        result = 1;
    }
    if (error > fabsf(ratio - 1.6f))
        result = 2;
    return result;
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

void NuRenderDevice::PreInitialize() {
}

void NuRenderDevice::OpenglErrorCallback(u32, u32, u32, u32, i32, char const *, void *) {
}

// ---------------------------------------------------------------------------
// C linkage helpers — the engine calls these without needing the C++ type
// ---------------------------------------------------------------------------

void NuRenderSetThisTreadAsRender() {
    g_renderDevice.SetThisTreadAsRender();
}

void BeginCriticalSectionGL(const char *file, i32 line) {
    g_renderDevice.BeginCriticalSection(file, line);
}

void EndCriticalSectionGL(const char *file, i32 line) {
    g_renderDevice.EndCriticalSection(file, line);
}

void NuRenderDeviceSwapBuffers() {
    g_renderDevice.SwapBuffers();
}

i32 NuRenderDeviceIsContextValid() {
    return g_renderDevice.IsContextValid();
}

// ---------------------------------------------------------------------------
// Stubs for iOS/legacy vertex paths that are not used on this platform
// ---------------------------------------------------------------------------

struct numtl_s;
typedef struct NuVertexFormatPS NuVertexFormatPS;

static __used__ i32 NuIOS_GetOrCreateVAO(u32, u32, u32, NuVertexFormatPS *) {
    return 0;
}
