#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nuvport.h"
#include "nu2api/nu3d/numtl.h"

#include <string.h>

#include "nu2api/nucore/numemory.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"

extern "C" u32 NuWindRand(void);

struct nugobj_s;
struct PartHeader;
struct uv1debdata;
struct nuvec4_s;

i32 nurndr_pixel_width;
i32 nurndr_pixel_height;
i32 nurndr_nforced_mtls;
NUMTL **nurndr_forced_mtl_table;
NUMTL *nurndr_forced_mtl;

i32 global_GobjIsReflectedGeometry;
i32 global_GobjIsShadowReceive;

i32 NuRndIsReflectionGobj(void) {
    return global_GobjIsReflectedGeometry;
}

i32 NuRndIsShadowReceiveRenderGobj(void) {
    return global_GobjIsShadowReceive;
}

void NuRndrInitEx(i32 stream_buffer_size, VARIPTR *buffer) {
    NuRndrStreamInit(stream_buffer_size, buffer);
}

u32 rndrstream_nbuffers = 2;
i32 nurndr_maxstreamsize;
VARIPTR rndrstream_end;
VARIPTR rndrstream_free;
static VARIPTR rndrstream[NURNDR_STREAM_MAX_BUFFERS];

static i32 rndrstream_buffid;
static i32 rndrstream_used;     // _ZL15rndrstream_used @0x11b8380
static i32 rndrstream_max_used; // _ZL19rndrstream_max_used @0x11b8390

// original 0x2e6e90 — rotate to the next stream buffer, recording the high
// watermark. The infinite-loop tail is the original's buffer-overflow trap.
extern "C" void NuRndrSwapStreamBuffers(void) {
    i32 last_used;

    last_used = (i32)((i64)rndrstream_free.addr - (i64)rndrstream[rndrstream_buffid].addr);
    if (rndrstream_max_used < last_used) {
        rndrstream_max_used = last_used;
    }
    if (last_used > nurndr_maxstreamsize) {
        for (;;) {
        }
    }
    rndrstream_buffid++;
    if (rndrstream_buffid == rndrstream_nbuffers) {
        rndrstream_buffid = 0;
    }
    rndrstream_free.addr = ALIGN(rndrstream[rndrstream_buffid].addr, 16);
    rndrstream_end.addr = rndrstream[rndrstream_buffid].addr + nurndr_maxstreamsize;
    rndrstream_used = last_used;
}

void NuRndrStreamInit(i32 stream_buffer_size, VARIPTR *buffer) {
    i32 i;

    if (buffer == NULL) {
        for (i = 0; i < rndrstream_nbuffers; i++) {
            rndrstream[i].void_ptr = NU_ALLOC(stream_buffer_size, 4, 1, "", NUMEMORY_CATEGORY_NONE);
            memset(rndrstream[i].void_ptr, 0, stream_buffer_size);
        }
    } else {
        for (i = 0; i < rndrstream_nbuffers; i++) {
            buffer->addr = ALIGN(buffer->addr, 0x80);
            rndrstream[i].void_ptr = buffer->void_ptr;

            memset(buffer->void_ptr, 0, stream_buffer_size);

            buffer->void_ptr = (void *)(buffer->addr + stream_buffer_size);
        }
    }

    rndrstream_buffid = 0;
    nurndr_maxstreamsize = stream_buffer_size;
    rndrstream_end.addr = rndrstream[0].addr + stream_buffer_size;
    rndrstream_free.addr = ALIGN(rndrstream[0].addr, 16);
}

void NuRndrRectUV2d(float, float, float, float, float, float, float, float, float, i32, numtl_s *) {
}

void NuRndrRectUV2diZ(i32, i32, i32, i32, float, float, float, float, i32, numtl_s *, i32) {
}

void NuRndrInitGeneric() {
    NuVpResetRegions();
}

void NuRndrGradRect2diZ(i32, i32, i32, i32, i32 *, numtl_s *, i32) {
}

extern NUMTX NuRndr_DebrisMtx;
extern NUVEC4 NuRndr_DebrisPlane;
void NuRndrParticleSetRepeat(nuvec_s *position);

void NuRndrParticleDraw(variptr_u *, PartHeader *header, uv1debdata *data, float time, numtx_s *matrix, i32 *,
                        float clip_distance, i32 mode, numtl_s *, float, float) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_chunk_s *>(data)->particles;
    for (i32 index = 0; index != 32; ++index, ++particle) {
        f32 age = time - particle->start_time;
        u32 frame = static_cast<u32>(particle->inverse_lifetime * age);
        if (frame > 62) {
            continue;
        }
        u32 next_frame = frame + 1;
        f32 fraction = particle->inverse_lifetime * age - static_cast<f32>(frame);
        debris_particle_frame_s *first = &header->frames[frame];
        debris_particle_frame_s *second = &header->frames[next_frame];
        NUVEC position;
        position.x = particle->position.x + particle->momentum.x * age;
        position.y = particle->position.y + particle->momentum.y * age + header->gravity * age * age * 0.945f;
        position.z = particle->position.z + particle->momentum.z * age;
        NUVEC rotated;
        rotated.x = position.x * matrix->m00 + position.y * matrix->m10 + position.z * matrix->m20;
        rotated.y = position.x * matrix->m01 + position.y * matrix->m11 + position.z * matrix->m21;
        rotated.z = position.x * matrix->m02 + position.y * matrix->m12 + position.z * matrix->m22;
        NuRndr_DebrisMtx.m30 = rotated.x + matrix->m30;
        NuRndr_DebrisMtx.m31 = rotated.y + matrix->m31;
        NuRndr_DebrisMtx.m32 = rotated.z + matrix->m32;
        if (mode == 6 || mode == 7) {
            NuRndrParticleSetRepeat(reinterpret_cast<NUVEC *>(&NuRndr_DebrisMtx.m30));
        }
        f32 distance = NuRndr_DebrisPlane.w +
                       (NuRndr_DebrisMtx.m32 * NuRndr_DebrisPlane.z +
                        (NuRndr_DebrisMtx.m30 * NuRndr_DebrisPlane.x + NuRndr_DebrisMtx.m31 * NuRndr_DebrisPlane.y));
        if (clip_distance > distance) {
            continue;
        }
        NUVEC offset;
        offset.x = first->position.x * (1.0f - fraction) + second->position.x * fraction;
        offset.y = first->position.y * (1.0f - fraction) + second->position.y * fraction;
        offset.z = first->position.z * (1.0f - fraction) + second->position.z * fraction;
        NUVEC extent;
        extent.x = first->extent.x * (1.0f - fraction) + second->extent.x * fraction;
        extent.y = first->extent.y * (1.0f - fraction) + second->extent.y * fraction;
        extent.z = first->extent.z * (1.0f - fraction) + second->extent.z * fraction;
        NUVEC texture_offset;
        texture_offset.x = first->texture_offset.x * (1.0f - fraction) + second->texture_offset.x * fraction;
        texture_offset.y = first->texture_offset.y * (1.0f - fraction) + second->texture_offset.y * fraction;
        texture_offset.z = first->texture_offset.z * (1.0f - fraction) + second->texture_offset.z * fraction;
        NuVecMtxTransform(&offset, &offset, &NuRndr_DebrisMtx);
        NuVecMtxTransform(&extent, &extent, &NuRndr_DebrisMtx);
        NuVecMtxTransform(&texture_offset, &texture_offset, &NuRndr_DebrisMtx);
        // The original ends here without submitting geometry. Its subsequent
        // colour conversions only write dead stack locals.
    }
}

void NuRndrRectUV2dNoScale(float, float, float, float, float, float, float, float, i32, numtl_s *) {
}

void NuRndrParticleSetRepeat(nuvec_s *position) {
    NUVEC repeat = {
        (position->x - NuRndrDebBase.x) / NuRndrDebRange.x,
        (position->y - NuRndrDebBase.y) / NuRndrDebRange.y,
        (position->z - NuRndrDebBase.z) / NuRndrDebRange.z,
    };
    repeat.x -= static_cast<f32>(static_cast<i32>(repeat.x + 65536.0f) - 65536);
    repeat.y -= static_cast<f32>(static_cast<i32>(repeat.y + 65536.0f) - 65536);
    repeat.z -= static_cast<f32>(static_cast<i32>(repeat.z + 65536.0f) - 65536);
    position->x = NuRndrDebBase.x + repeat.x * NuRndrDebRange.x;
    position->y = NuRndrDebBase.y + repeat.y * NuRndrDebRange.y;
    position->z = NuRndrDebBase.z + repeat.z * NuRndrDebRange.z;
}

void NuRndrCalcRandEllipsePos(nuvec4_s *position, numtx_s *matrix, nuvec_s *axes) {
    f32 x = (f32)(i32)NuWindRand() - 32768.0f;
    f32 y = (f32)(i32)NuWindRand() - 32768.0f;
    f32 z = (f32)(i32)NuWindRand() - 32768.0f;
    f32 scale = 1.0f / NuFsqrt(x * x + y * y + z * z);
    x *= scale;
    y *= scale;
    z *= scale;
    position->x = axes[1].x * x + axes[2].x * y + axes[3].x * z + axes[0].x;
    position->y = axes[1].y * x + axes[2].y * y + axes[3].y * z + axes[0].y;
    position->z = axes[1].z * x + axes[2].z * y + axes[3].z * z + axes[0].z;
    position->w = 1.0f;
    NuVec4MtxTransformVU0(position, position, matrix);
}

void NuRndrCalcRandCylinderPos(nuvec4_s *position, numtx_s *matrix, nuvec_s *axes) {
    f32 x = (f32)((i32)NuWindRand() - 32768) * (1.0f / 32768.0f);
    f32 y = (f32)(i32)NuWindRand() - 32768.0f;
    f32 z = (f32)(i32)NuWindRand() - 32768.0f;
    f32 scale = 1.0f / NuFsqrt(y * y + z * z);
    y *= scale;
    z *= scale;
    position->x = axes[1].x * x + axes[2].x * y + axes[3].x * z + axes[0].x;
    position->y = axes[1].y * x + axes[2].y * y + axes[3].y * z + axes[0].y;
    position->z = axes[1].z * x + axes[2].z * y + axes[3].z * z + axes[0].z;
    position->w = 1.0f;
    NuVec4MtxTransformVU0(position, position, matrix);
}

extern "C" {
    i32 NuRndrSetAmbientLightPS(const NUCOLOUR3 *);
    i32 NuRndrSetDirectionalLightsPS(const NUVEC *, const NUCOLOUR3 *, const NUVEC *, const NUCOLOUR3 *, const NUVEC *,
                                     const NUCOLOUR3 *);

    static __used__ void NuRndrSetAmbientLight(NUCOLOUR3 *colour) {
        NuRndrSetAmbientLightPS(colour);
    }

    static __used__ void NuRndrSetDirectionalLights(NUVEC *dir0, NUCOLOUR3 *colour0, NUVEC *dir1, NUCOLOUR3 *colour1,
                                                    NUVEC *dir2, NUCOLOUR3 *colour2) {
        NuRndrSetDirectionalLightsPS(dir0, colour0, dir1, colour1, dir2, colour2);
    }

    static __used__ void NuRndrSetSpecularLight(nuvec_s *direction, nucolour4_s *intensity) {
        if (direction) {
            NuRndrLightingStateCurrent.specular_direction = *direction;
            NuRndrLightingStateCurrent.field_0x60 = 0;
        } else {
            NuRndrLightingStateCurrent.field_0x60 = 1;
        }
        if (intensity) {
            NuRndrLightingStateCurrent.specular_intensity = *intensity;
            NuRndrLightingStateCurrent.field_0x74 = 1;
        } else {
            NuRndrLightingStateCurrent.field_0x74 = 0;
        }
        NuRndrSetSpecularLightPS(direction, intensity);
    }
}
