#include "nu2api/nu3d/android/nurain_android.h"
#include "nu2api/nucore/nuvuvec.hpp"

extern "C" {
i32 NuRainKey = -1;
i32 testrain = 1;
f32 NuRainOldY = 123456.0f;
f32 NuRainTwist;
f32 NuRainWob;
i32 NuRainInactive;
f32 NuRainFall;
}

extern "C" void NuRainProcess(void) {
}

void NuRainDrawDrop(i32, i32, f32, f32, i32) {
}

void NuRainDrawShape(f32) {
}

extern "C" void NuRainDraw(i32) {
}

extern "C" void NuRainSetFall(f32 fall) {
    if (fall < 0.0f)
        NuRainFall = 0.0f;
    else if (fall > 1.0f)
        NuRainFall = 1.0f;
    else
        NuRainFall = fall;
}
