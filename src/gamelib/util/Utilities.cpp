#include "gamelib/util/Utilities.h"

#include "nu2api/nucore/nutime.h"

static u32 frameStartTimeMS;
static NUTIME frameStartTime;

u32 UtilGetTime(void) {
    NUTIME now;
    NuTimeGet(&now);
    return static_cast<u32>(NuTimeMilliSeconds(&now));
}

u32 UtilGetFrameStartTime(void) {
    return frameStartTimeMS;
}

void UtilFrameStart(void) {
    NuTimeGet(&frameStartTime);
    frameStartTimeMS = static_cast<u32>(NuTimeMilliSeconds(&frameStartTime));
}
