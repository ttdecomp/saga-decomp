#include "nu2api/nucore/nutime.h"

#include <time.h>

#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuvuvec.hpp"

static u64 g_startTime;

void NuTimeInitPS(void) {
    g_startTime = NuGetCurrentTimeMilisecondsPS();
}

void NuTimeGetTicksPS(u32 *low, u32 *high) {
    struct timespec ts;
    u64 ticks;

    clock_gettime(CLOCK_REALTIME, &ts);

    ticks = ts.tv_nsec / 1000 + (u64)ts.tv_sec * 1000000;

    *high = ticks >> 32;
    *low = ticks;
}

void NuTimeGetMicrosecondsPS(u32 *low, u32 *high) {
    struct timespec ts;
    u64 ticks;

    clock_gettime(CLOCK_REALTIME, &ts);

    ticks = ts.tv_nsec / 1000 + (u64)ts.tv_sec * 1000000;

    *high = ticks >> 32;
    *low = ticks;
}

void NuTimeGetTicksPerSecondPS(u32 *low, u32 *high) {
    u64 ticks_per_second = 1000000;
    *high = ticks_per_second >> 32;
    *low = ticks_per_second;
}

u64 NuGetCurrentTimeMilisecondsPS(void) {
    struct timespec ts;

    clock_gettime(0, &ts);

    return (u64)ts.tv_nsec + (u64)ts.tv_sec * 1000;
}

extern "C" void NuTimeGetTime(void) {
}
