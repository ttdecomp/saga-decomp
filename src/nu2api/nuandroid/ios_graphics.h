#pragma once

#include "nu2api/nucore/common.h"

#ifdef __cplusplus
i32 NuCheckGLErrorsFL(const char *file, i32 line);

extern i32 g_isLowEndDevice;

extern i32 g_backingWidth;
extern i32 g_backingHeight;

extern "C" {
#endif
    i32 NuIOS_IsLowEndDevice(void);
    f32 NuIOS_GetAspectRatio(void);
    i32 NuIOS_IsWidescreen(void);

    char *NuIOS_GetDocumentsPath(void);
    char *NuIOS_GetAppBundlePath(void);

    u32 NuIOS_YieldThread(void);

    void NuIOS_InitRenderThread();
    i32 NuIOS_ShouldUseMSAA(void);
    void NuIOSInitOpenGLES(void);
    void NuIOS_AllocateSystemFramebuffers(void);
    void Nu360_dxClear(u32 clear_flags, u32 colour);
    void NuIOS_WaitUntilAllowedToRender(void);
    void NuIOS_SetRenderIncomplete(void);
    void NuIOS_SetRenderComplete(void);
    void NuIOS_WaitForRenderThreadCompletion(void);
    void NuIOS_WakeRenderThread(void);

    extern i64 g_renderStartTime; // @0x66c9a8
#ifdef __cplusplus
}
#endif
