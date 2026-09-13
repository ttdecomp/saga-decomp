#pragma once

#include "decomp.h"

extern "C" {
extern i32 NuRainKey;
extern i32 testrain;
extern f32 NuRainOldY;
extern f32 NuRainTwist;
extern f32 NuRainWob;
extern i32 NuRainInactive;
extern f32 NuRainFall;

void NuRainProcess(void);
void NuRainDraw(i32 reserved_texture);
void NuRainSetFall(f32 fall);
}

void NuRainDrawDrop(i32, i32, f32, f32, i32);
void NuRainDrawShape(f32);
