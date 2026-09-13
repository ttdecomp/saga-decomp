#pragma once

#include "nu2api/nucore/common.h"

typedef struct nutime_s {
    u32 low;
    i32 high;
} NUTIME;

#ifdef __cplusplus

void NuTimeGetTicksPS(u32 *low, u32 *high);
void NuTimeGetMicrosecondsPS(u32 *low, u32 *high);
void NuTimeGetTicksPerSecondPS(u32 *low, u32 *high);

u64 NuGetCurrentTimeMilisecondsPS(void);

extern "C" {
#endif
    void NuTimeInitPS(void);

    void NuTimeGet(NUTIME *t);

    f32 NuTimeGetFrameTime(void);
    void NuTimeForceFrameTime(f32 frame_time);
    void NuTimeWait(f32 milliseconds);

    f32 NuTimeSeconds(NUTIME *t);
    f32 NuTimeScanlines(NUTIME *t);
    f32 NuTimeMilliSeconds(NUTIME *t);
    f32 NuTimeMicroSeconds(NUTIME *t);

    void NuTimeSub(NUTIME *t, NUTIME *a, NUTIME *b);
#ifdef __cplusplus
}
#endif
