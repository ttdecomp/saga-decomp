#pragma once

#include "nu2api/nu3d/numtl.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/numtx.h"

typedef enum {
    NUPRIM_SCALEMODE_PS2 = 0,
    NUPRIM_SCALEMODE_NORMALISED = 1,
    NUPRIM_SCALEMODE_ABSOLUTE = 2
} NUPRIMSCALEMODE;

#ifdef __cplusplus
extern "C" {
#endif
    void NuPrimInit(VARIPTR *buffer, VARIPTR buffer_end);

    void NuPrim3DBegin(u32 prim_type, u32 vtx_fmt, NUMTL *mtl, NUMTX *world_mtx);
    void NuPrim2DBegin(u32 prim_type, u32 vtx_fmt, NUMTL *mtl);
    void NuPrim2DAddXYZ(f32 x, f32 y, f32 z);
    void NuPrim2DEnd(void);
    void NuPrim3DEnd(void);

    void NuPrimSetCoordinateSystem(NUPRIMSCALEMODE scale_mode);

    // Immediate-mode stream state shared by the Android primitive unit and
    // drawing callers across the renderer.
    // The original bss holds these adjacent: g_NuPrim_StreamBufferPtr @0x99b540
    // (points at the VARIPTR cursor of the current display-list vertex buffer),
    // g_NuPrim_VertexCount @0x99b544.
    extern VARIPTR *g_NuPrim_StreamBufferPtr;
    extern i32 g_NuPrim_VertexCount;
    extern char g_NuPrim_NeedsHalfUVs;
    extern char g_NuPrim_NeedsOverbrightening;

    extern i32 NuPrimCSPos;
    extern NUPRIMSCALEMODE NuPrimCoordSystemStack[16];
    extern f32 NuPrim_XScale;
    extern f32 NuPrim_YScale;
    extern f32 NuPrim_XBias;
    extern f32 NuPrim_YBias;
#ifdef __cplusplus
}
#endif
