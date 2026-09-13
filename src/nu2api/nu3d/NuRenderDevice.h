#pragma once

#include "nu2api/nucore/common.h"

#include <EGL/egl.h>
#include <pthread.h>

#ifdef __cplusplus

struct ANativeWindow;

class NuRenderDeviceGen {
  protected:
    bool value;
    NuRenderDeviceGen();
};

class NuRenderDevice : NuRenderDeviceGen {
    i32 lock_count;
    pthread_mutex_t mutex;
    pthread_mutex_t mutex2;
    i32 field10_0x10;
    i32 field14_0x14;
    const char *extensions;
    bool is_not_amazon_kf;
    u8 field1d_0x1d[3];
    i32 max_texture_units;
    i32 max_texture_size;

  public:
    // ABI-visible to the platform texture implementation at offset 0x28.
    u8 enabled_extensions[26];

  private:
    bool oes_packed_depth_stencil;
    bool oes_depth24;
    bool oes_depth_texture;
    bool field48_0x45;
    bool focus;
    bool field47_0x47;
    EGLNativeWindowType native_window;
    i32 field4c_0x4c;
    i32 field50_0x50;
    bool field54_0x54;
    u8 field55_0x55[3];
    EGLint attrib_list[3];
    u8 field64_0x64[0x14];
    EGLDisplay egl_display;
    EGLConfig egl_config;
    i32 current_context_index;
    EGLSurface pbuffers[4];
    EGLContext contexts[4];
    volatile bool context_valid;
    u8 fielda5_0xa5[3];
    u32 drawable_width;
    u32 drawable_height;
    u32 width;
    u32 height;
    u32 backing_width;
    u32 backing_height;
    i32 nominal_aspect_ratio;
    f32 aspect_ratio;

    void DetermineBackBufferResolution(i32 width, i32 height);
    EGLConfig SelectEGLConfig();
    bool IsExtensionSupported(const char *exts);

    void FrameEnd();

  public:
    NuRenderDevice();

    void Initialize();

    void SetThisTreadAsRender();
    void BeginCriticalSection(const char *file, i32 line);
    void EndCriticalSection(const char *file, i32 line);
    void SwapBuffers();

    void OnWindowCreated(ANativeWindow *window);

    void InitialiseOpenGLContext(ANativeWindow *window);
    void CheckForRenderWindowInitialisation();

    void OnAppPaused();
    void OnAppRestarted();
    void OnAppResume();
    void OnAppStarted();
    void OnAppStopped();
    void OnGainedFocus();
    void OnLostFocus();
    void OnWindowDestroy();
    void PreInitialize();
    void ResizeDevice(i32 width, i32 height, i32 _a, bool _b, bool _c, bool _d, bool _e);

    bool IsContextValid() const;
    bool MultiThreadRender() const;
    i32 DetermineNominalAspectRatio(u32 width, u32 height) const;
    void OpenglErrorCallback(u32 source, u32 type, u32 id, u32 severity, i32 len, char const *msg, void *user_param);
};

extern NuRenderDevice g_renderDevice;
extern thread_local i32 gt_glContextIndex;
extern i32 g_nextGLContextIndex;
void NuRenderInspectEGLConfig(EGLDisplay display, EGLConfig config);
i32 _NuCheckGLErrors(const char *file);

extern "C" {
#endif
    void NuRenderSetThisTreadAsRender(void);
    void BeginCriticalSectionGL(const char *file, i32 line);
    void EndCriticalSectionGL(const char *file, i32 line);
    void NuRenderDeviceSwapBuffers(void);
    i32 NuRenderDeviceIsContextValid(void);
#ifdef __cplusplus
}
#endif
