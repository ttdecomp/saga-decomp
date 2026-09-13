#include "nu2api/nu3d/nuprim.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nurndrstat.h"

char g_NuPrim_NeedsHalfUVs;
char g_NuPrim_NeedsOverbrightening;

// Points at the VARIPTR cursor of the current display-list vertex buffer
// (original bss @0x99b540; set by NuPrim2DBegin/NuPrim3DBegin).
VARIPTR *g_NuPrim_StreamBufferPtr;

i32 NuPrimCSPos;
NUPRIMSCALEMODE NuPrimCoordSystemStack[16];

f32 NuPrim_XScale = 1.0f;
f32 NuPrim_YScale = 1.0f;
f32 NuPrim_XBias;
f32 NuPrim_YBias;
