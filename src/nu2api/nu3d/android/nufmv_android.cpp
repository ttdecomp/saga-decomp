#include "nu2api/nu3d/android/nufmv_android.h"
#include "nu2api/nucore/nuvuvec.hpp"

extern "C" {
i32 fmv_force_vidmem;
i32 fmv_exitframe;
i32 fmv_pause;
i32 fmv_playing;
i32 queue_cnt;

void NuFmvInit(void) {
}

i32 NuFmvPlayV(i32 option, ...) {
    return 1;
}

// The original wrapper forwards eight 32-bit arguments through a tagged
// option list. Option meanings beyond this layout remain unrecovered.
i32 NuFmvPlay(u32 argument0, i32 enabled, u32 argument2, u32 argument3, u32 argument4, u32 argument5, u32 argument6,
              u32 argument7) {
    return NuFmvPlayV(2, argument0, enabled != 0 ? 3 : 0, 4, argument2, 5, argument3, 6, argument4, 7, argument5, 8,
                      argument6, argument7, 1);
}
}
