#pragma once

#include "nu2api/nucore/common.h"

#ifdef __cplusplus
void *renderThread_main(void *arg);

extern "C" {
#endif
    void NuRenderThreadLock(void);
    void NuRenderThreadUnlock(void);
    void NuRenderThreadPrepareRender(void);
    void NuRenderThreadStartRender(void);
    i32 NuRenderThreadIsLocked(void);
    i32 NuRenderThreadIsCurrentThread(void);
    void NuRenderThreadCreate(void);
    void NuRenderThreadDestroy(void);

    i32 renderThread_processRenderScenes(void);
#ifdef __cplusplus
}
#endif
