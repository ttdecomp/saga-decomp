// Original nuscratch_android.c, optimized Android scratch/clear unit.
// The scratch allocator functions are still in nucore_plain.cpp; this
// file starts with the original clear wrapper and its function-local cache.

#include <GLES2/gl2.h>

#include "nu2api/nu3d/android/nuposteffect_plain.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"

// Nu-side DX-style clear flags, translated to GL masks below.
static constexpr u32 kNuClear_Color = 0x100;
static constexpr u32 kNuClear_Depth = 0x200;
static constexpr u32 kNuClear_Stencil = 0x800;

// Original 0x317070. The packed colour has RGBA bytes from least to most
// significant; clearing depth also restores writable GL depth state.
extern "C" void Nu360_dxClear(u32 clear_flags, u32 colour) {
    // Original local BSS: _ZZ13Nu360_dxClearE10lastColour.
    static u32 lastColour;
    GLbitfield glMask = 0;

    if ((clear_flags & kNuClear_Color) != 0) {
        glMask |= GL_COLOR_BUFFER_BIT;
        if (colour != lastColour) {
            const float r = static_cast<float>(colour & 0xff) / 255.0f;
            const float g = static_cast<float>((colour >> 8) & 0xff) / 255.0f;
            const float b = static_cast<float>((colour >> 0x10) & 0xff) / 255.0f;
            const float a = static_cast<float>(colour >> 0x18) / 255.0f;
            glClearColor(r, g, b, a);
            lastColour = colour;
        }
    }
    if ((clear_flags & kNuClear_Depth) != 0) {
        glMask |= GL_DEPTH_BUFFER_BIT;
        if (g_renderContext_zFunc != 2) {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }
        g_renderContext_zFunc = 2;
    }
    if ((clear_flags & kNuClear_Stencil) != 0) {
        glMask |= GL_STENCIL_BUFFER_BIT;
    }
    glClear(glMask);
}
