// GLES2 display-list callbacks and immediate GL state — see nuiosdl_gl.cpp.
#pragma once

#include "decomp.h"
#include "nu2api/numath/numtx.h"

struct numtl_s;
struct nunativetex_s;

struct NuVertexFormatPS;

struct NuFaceOnTransformPacket {
    NUMTX world;
    f32 magnitude;
    NUMTX face_on;
};

struct NuFaceOnDrawPacket {
    u32 reserved;
    i32 face_count;
    u32 vertex_buffer;
    i32 first_vertex;
};

// Globals defined in nuiosdl_gl.cpp (original bss 0x99b440..).
extern u32 g_boundShader;
struct nushaderprogram_s;
extern nushaderprogram_s *g_currentShaderProgram;
extern numtl_s *g_boundMaterial;
extern numtl_s *g_renderContext_materialInUse;
extern numtl_s *g_LastMtl;
extern usize g_boundVertexFormat;
extern NuVertexFormatPS *g_nuFaceOnVertexFormat;
extern NuVertexFormatPS *g_nuDebrisVertexFormat;
extern u32 g_activeAttributes;
extern i32 g_renderContext_zFunc;
extern u32 g_alphaRef;
extern u32 g_alphaFunc;
extern i32 g_alphaTestEnabled;
extern u32 g_lastAlphaRef;
extern u32 g_lastAlphaBlend;
extern i32 g_renderingReflection;
extern "C" f32 g_renderContext_viewProj[16];
extern "C" f32 g_renderContext_view[16];
void NuIOS_CopyBackbufferToTexture(nunativetex_s *tex, bool depth);

void NuIOS_SetCullMode(i32 mode);
extern "C" void NuMtlSetRenderStatesPS(numtl_s *mtl);
extern "C" void NuRenderContextSetZFunc(i32 zfunc);
extern "C" void NuIOS_SetVertexFormat(usize fmt);
void NuIOSDLMtlCallback(void *arg);
void NuIOSDLPreWarmGeomCallback(void *arg);
void NuIOS_ResetVAODuplicateFinder();
void NuIOSDLGeom2DCallback(void *arg);
