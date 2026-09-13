#pragma once

#include "nu2api/nucore/common.h"

typedef enum NUVIDEO_SWAPMODE {
    NUVIDEO_SWAPMODE_INVALID = 0,
    NUVIDEO_SWAPMODE_ASYNC = 1,
    NUVIDEO_SWAPMODE_FIELDSYNC = 2,
    NUVIDEO_SWAPMODE_FRAMESYNC = 3,
    NUVIDEO_SWAPMODE_MIN2FIELD = 4,
    NUVIDEO_SWAPMODE_ROLLING = 5,
    NUVIDEO_SWAPMODE_ROLLING2FRAMES = 6,
    NUVIDEO_SWAPMODE_CNT = 7
} NUVIDEO_SWAPMODE;

enum {
    NUVIDEOMODE_UNDEFINED = 0,
    NUVIDEOMODE_UNUSED1 = 1,
    NUVIDEOMODE_NTSC = 2,
    NUVIDEOMODE_PAL = 3,
    NUVIDEOMODE_PALFF = 4,
    NUVIDEOMODE_NTSCFF = 5,
    NUVIDEOMODE_NTSC_J = 6
};

#ifdef __cplusplus
i32 NuVideoGetAspectPS(void);
void NuVideoSetBrightnessPS();
void NuVideoSetSwapModePS(NUVIDEO_SWAPMODE video_swap_mode);

extern "C" {
#endif
    i32 NuVideoGetMode(void);
    void NuVideoSetMode(i32 mode);
    i32 NuVideoGetAspect(void);
    NUVIDEO_SWAPMODE NuVideoGetSwapMode(void);
    void NuVideoRollingFrameRateReset(void);
    void NuVideoSetBrightness(f32 brightness);
    void NuVideoSetResolution(i32 width, i32 height);
    NUVIDEO_SWAPMODE NuVideoSetSwapMode(NUVIDEO_SWAPMODE video_swap_mode);
    void NuPs2VideoScreenDump(char *filename, i32 format, f32 scale_x, f32 scale_y, i32 face, i32 x, i32 y);
#ifdef __cplusplus
}
#endif
