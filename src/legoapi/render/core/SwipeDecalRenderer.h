#pragma once

#include "decomp.h"
#include "nu2api/nucore/fixed_width.h"

struct TouchHolder;

struct SwipeDecalRenderer {
    enum Style : i32 {};
    SwipeDecalRenderer(TouchHolder &, i32, SwipeDecalRenderer::Style);
    void Process(float);
    void Render();

    // The original constructor allocates 0x48 bytes; members are not yet recovered.
    u8 unresolved_state[0x48];
};
DECOMP_ASSERT(sizeof(SwipeDecalRenderer) == 0x48, "Swipe decal renderer ABI");
