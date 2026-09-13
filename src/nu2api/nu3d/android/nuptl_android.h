#pragma once

#include "decomp.h"
#include "nu2api/nu3d/android/nudlist_callbacks.h"

struct PartHeader;
struct debinftype;
struct dma_particle_chunk_s;
struct uv1debdata;
struct numtl_s;
struct nunativedebrisdata_s;

extern "C" PartHeader *CreateDmaPartEffectList(void *memory, i32 *size);
extern "C" dma_particle_chunk_s *CreateDmaParticleSet(void *memory, i32 *size);
extern "C" dma_particle_chunk_s *CreateDmaParticleSetGlass(void *memory, i32 *size);
extern "C" void LinkDmaParticalSets(dma_particle_chunk_s **chunks, i32 count);
extern "C" void NuRndrSetParticleRotation(NUMTX *rotation);
void NuRndrParticleSetRepeat(NUVEC *position);
void NuRndrParticleDraw(VARIPTR *buffer, PartHeader *header, uv1debdata *data, f32 time, NUMTX *matrix,
                        i32 *indices, f32 clip_distance, i32 mode, numtl_s *material, f32 a, f32 b);
extern "C" void GenericDebinfoDmaTypeUpdate(debinftype *effect);
extern "C" void NuRndrParticleGroup(uv1debdata *chunks, PartHeader *header, numtl_s *material, f32 time,
                                     NUMTX *matrix, i32 particle_type, f32 a, f32 b, f32 c, f32 near_clip);
void BuildDebrisVerts(PartHeader *header, uv1debdata *chunk_data, numtl_s *material, f32 time, NUMTX *matrix,
                      i32 particle_type, f32 a, f32 b, f32 c, f32 near_clip);
void AddParticleGroupToDisplayList(nunativedebrisdata_s *group);

i32 NuDebrisRendererNextBuffer();
void NuDebrisRendererFlushBuffers();
extern "C" void NuInitDebrisRenderer(VARIPTR *buffer, VARIPTR buffer_end);
