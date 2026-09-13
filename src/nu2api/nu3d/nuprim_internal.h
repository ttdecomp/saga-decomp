#pragma once

#include "nu2api/nucore/common.h"

// Immediate-mode vertex; UV storage is either two floats or packed half UVs.
struct PrimVertexRaw {
    f32 x, y, z;
    u32 color;
    u32 uv[2];
};
static_assert(sizeof(PrimVertexRaw) == 0x18, "PrimVertexRaw must be 0x18 bytes");
