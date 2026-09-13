#pragma once

#include "decomp.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/numtx.h"

#define NURNDR_STREAM_MAX_BUFFERS 2

typedef struct rndrstream_s RNDRSTREAM;

extern i32 nurndr_nforced_mtls;
extern struct numtl_s **nurndr_forced_mtl_table;
extern struct numtl_s *nurndr_forced_mtl;

typedef i32 NUCOLOUR32;

typedef struct NURND_VERTEX3D {
    NUVEC position;
    NUVEC normal;
    u32 colour;
    f32 u, v;
} NURND_VERTEX3D;
DECOMP_ASSERT(sizeof(NURND_VERTEX3D) == 0x24, "Immediate 3D vertex ABI");
DECOMP_ASSERT(offsetof(NURND_VERTEX3D, colour) == 0x18, "Immediate 3D colour offset");

#define RGBA_TO_NUCOLOUR32(r, g, b, a) ((u8)(a) << 0x18) | ((u8)(b) << 0x10) | ((u8)(g) << 0x08) | ((u8)(r) << 0x00);

typedef struct nucolour3_s {
    f32 r;
    f32 g;
    f32 b;
} NUCOLOUR3;

typedef struct nucolour4_s {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
} NUCOLOUR4;

typedef struct NURND_SHADOW_s {
    NUVEC position;
    f32 radius;
    u16 opacity;
    u16 x_rotation;
    u16 y_rotation;
    u16 z_rotation;
} NURND_SHADOW_s;

DECOMP_ASSERT(sizeof(NURND_SHADOW_s) == 0x18, "NURND_SHADOW_s ABI");

extern i32 g_backingWidth;
extern i32 g_backingHeight;

#ifdef __cplusplus

void NuRndrStreamInit(i32 stream_buffer_size, VARIPTR *buffer);
void NuRndrParticleSetRepeat(NUVEC *position);
void NuLightBurnoutEffect(i32 mode, f32 threshold, f32 intensity, f32 flare);
// axes[0] is the center; axes[1..3] are the three shape basis vectors.
void NuRndrCalcRandEllipsePos(struct nuvec4_s *position, NUMTX *matrix, NUVEC *axes);
void NuRndrCalcRandCylinderPos(struct nuvec4_s *position, NUMTX *matrix, NUVEC *axes);

extern "C" {
    extern i32 g_minmiplevel;
    extern f32 g_mipmapbias;
    void NuRndrSetGlobalMinMipLevel(i32 level);
    void NuRndrSetGlobalMipMapBias(f32 bias);
    void NuTextureBlendEffect(i32 arg0, i32 arg1, struct nuvec4_s *parameters);
    void NuRndrAxisArrowsMtx(NUMTX *matrix, f32 length, struct numtl_s *material);
    void NuRndrAxisArrows(NUVEC *position, void *unused, f32 length, struct numtl_s *material);
    void NuRndrBoundingBox(NUVEC *minimum, NUVEC *maximum, NUMTX *matrix, i32 colour);
    void NuRndrAxes(NUMTX *matrix, f32 length);
    void NuRndrAxisBright(NUMTX *matrix, f32 length, i32 brightness);
    void NuRndrCircle(f32 x, f32 y, f32 radius, f32 aspect, i32 count, f32 u0, f32 v0, f32 u1, f32 v1, i32 colour,
                      struct numtl_s *material);
    i32 NuRndrHighResScreenGrab(char *prefix, f32 scale, f32 a, f32 b, f32 c, i32 number);
    void NuRndrScreenGrabTileInit(void *, i32, f32, f32, f32);
    void NuRndrScreenGrabTileDeInit(void *);
    void NuRndrScreenGrabTileBegin(void **);
    void NuRndrScreenGrabTileEnd(void **);
    void NuRndrSolidTri(NUVEC *a, NUVEC *b, NUVEC *c, i32 colour);
    void NuRndrWireTri(NUVEC *a, NUVEC *b, NUVEC *c, i32 colour);
    void NuRndrLineRect2di(i32 x, i32 y, i32 width, i32 height, i32 colour, struct numtl_s *material);
    extern i32 global_GobjIsReflectedGeometry;
    extern i32 global_GobjIsShadowReceive;
    i32 NuRndIsReflectionGobj(void);
    i32 NuRndIsShadowReceiveRenderGobj(void);
    extern i32 global_GobjWasDrawnUnreflected;
    extern f32 global_windspeed;
    extern f32 global_windscale;
    i32 NuRndrWasDrawnUnreflectedGobj(void);
    void NuRndrStartShadowReceiveRender(void);
    void NuRndrEndShadowReceiveRender(void);
    i32 NuRndrGetCullDebug(void);
    void NuRndrSetWind(f32 speed, f32 scale);
    void NuRndrAnglesZX(NUVEC *direction, NUVEC *angles);
#endif
    extern i32 NuRndrStopUpdate;
    extern NUVEC NuRndrDebBase;
    extern NUVEC NuRndrDebRange;
    extern NUVEC NuRndrDebRangeInv;
    void NuRndrSetDebBaseRange(NUVEC *base, NUVEC *range);
    void NuRndrSetDebBox(NUVEC *range);

    extern i32 nurndr_pixel_width;
    extern i32 nurndr_pixel_height;
    void NuRndrShadPolys(struct numtl_s *material);
    extern i32 NuRndrShadowCnt;
    extern NURND_SHADOW_s NuRndrShadPolDat[128];

    void NuRndrInitEx(i32 stream_buffer_size, VARIPTR *buffer);
    i32 NuRndrSwapScreenEx(i32 mode, void (*callback)(void));
    void NuRndr3dLine(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, i32 colour);
    i32 NuRndrTri3dClip(NURND_VERTEX3D *vertices, i32 count, NUMTX *matrix, struct numtl_s *material);
    i32 NuRndrStrip3d(NURND_VERTEX3D *vertices, struct numtl_s *material, NUMTX *matrix, i32 count);
    i32 NuRndrTriStrip3dClip(NURND_VERTEX3D *vertices, i32 count, NUMTX *matrix, struct numtl_s *material);

    i32 NuRndrSetViewMtx(NUMTX *vpcs_mtx, NUMTX *viewport_vpc_mtx, NUMTX *scissor_vpc_mtx);
    i32 NuRndrStateUpdateCameraState(void);

    i32 NuRndrSetAmbientLightPS(const NUCOLOUR3 *colour);
    i32 NuRndrSetDirectionalLightsPS(const NUVEC *dir0, const NUCOLOUR3 *colour0, const NUVEC *dir1,
                                     const NUCOLOUR3 *colour1, const NUVEC *dir2, const NUCOLOUR3 *colour2);
    i32 NuRndrSetFxMtx(NUMTX *matrix);
    i32 NuRndrSetSpecularLightPS(const NUVEC *direction, const NUCOLOUR4 *intensity);
    void NuRndrStartReflectionRender(i32 clear_depth);
    void NuRndrEndReflectionRender(void);

    void FaceYDirStream(i32 y_angle);
    void NuRndrAddShadow(NUVEC *position, f32 radius, i32 opacity, i32 x_rotation, i32 y_rotation, i32 z_rotation);
    f32 *NuRndrCreateBlendShapeDeformerWeightsArray(i32 count);
#ifdef __cplusplus
}

f32 **NuRndrCreateBlendShapeDWAPointers(i32 count);
#endif

extern f32 circle_scale_radius;
