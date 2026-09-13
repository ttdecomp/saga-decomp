#pragma once

#include "decomp.h"

extern "C" {
extern i32 fmv_force_vidmem;
extern i32 fmv_exitframe;
extern i32 fmv_pause;
extern i32 fmv_playing;
extern i32 queue_cnt;

void NuFmvInit(void);
i32 NuFmvPlayV(i32 option, ...);
i32 NuFmvPlay(u32 argument0, i32 enabled, u32 argument2, u32 argument3, u32 argument4, u32 argument5, u32 argument6,
              u32 argument7);
}
