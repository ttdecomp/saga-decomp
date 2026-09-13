#include <string.h>

#include "decomp.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nuvertexformat_android.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nutex.h"
#include <GLES2/gl2.h>
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/android/nutex_android.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nu3d/nushader_plain.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nucore/nuapi.h"

u8 DebrisGlassIOS_Tex[] = {
    0x50, 0x56, 0x52, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x14, 0x54, 0x64, 0xc1,
    0xfa, 0xe0, 0xb0, 0x64, 0x64, 0x90, 0xb0, 0x01, 0xca, 0x4d, 0xc8, 0x01, 0x50, 0x40, 0x50, 0xa1, 0xb4, 0x4a, 0xb6,
    0xff, 0x6a, 0x46, 0x51, 0xf5, 0xd1, 0x9f, 0xe5, 0xf0, 0xe4, 0xa0, 0xa0, 0x03, 0xba, 0x3a, 0xe1, 0xf1, 0xf4, 0xf0,
    0xf0, 0x03, 0xbe, 0x73, 0xca, 0xee, 0x6b, 0x6f, 0x0f, 0x99, 0xa5, 0xdb, 0xcd, 0x0a, 0x06, 0x07, 0x07, 0xbd, 0x98,
    0x7f, 0xc9, 0x04, 0x42, 0x06, 0x02, 0xa3, 0x99, 0x0c, 0x82, 0x4e, 0xde, 0x88, 0xef, 0xb1, 0x9a, 0x5a, 0xa7, 0xf8,
    0xf8, 0x30, 0x1c, 0xc0, 0xa6, 0xc0, 0xb6, 0x3e, 0x1f, 0x2e, 0x1e, 0xe1, 0xb7, 0x82, 0x99, 0xfc, 0xe9, 0xa5, 0xe5,
    0xfb, 0xb0, 0x1c, 0xab, 0xd0, 0xd0, 0x90, 0xd0, 0xbd, 0xa0, 0x7c, 0xa6, 0x2f, 0x0b, 0x1f, 0x0f, 0x87, 0xb6, 0xb2,
    0xa6, 0x0f, 0x0f, 0x1b, 0x0a, 0x49, 0xa5, 0xf8, 0x99, 0xf5, 0xf5, 0xf4, 0xf4, 0x01, 0x96, 0x16, 0xc2, 0xb5, 0x74,
    0xb0, 0x60, 0x61, 0x9d, 0xd0, 0xb1, 0x0a, 0x0b, 0x1b, 0x1b, 0x1d, 0x91, 0x1f, 0xba, 0xab, 0x7b, 0xbf, 0xaf, 0x3d,
    0xc1, 0x3c, 0xaa, 0x50, 0x90, 0x50, 0x90, 0xa1, 0xb9, 0x65, 0x82, 0xa4, 0x65, 0xa8, 0x09, 0xc1, 0xfd, 0x60, 0xc2,
    0x2f, 0x6f, 0xaf, 0xff, 0x29, 0xdd, 0x17, 0xbe, 0x16, 0x16, 0x33, 0x46, 0xc1, 0xbc, 0x87, 0xc1, 0xe1, 0xf4, 0xf8,
    0xf4, 0xff, 0xa0, 0xbd, 0xb9, 0xf0, 0xf0, 0xf0, 0xce, 0xfc, 0xbc, 0x1c, 0xbd, 0x06, 0x0b, 0x1e, 0x0b, 0xc3, 0xa4,
    0x18, 0xb5, 0x0b, 0x07, 0x15, 0x05, 0x63, 0xc0, 0x95, 0xd9, 0x84, 0xd7, 0x9b, 0xeb, 0xcf, 0xad, 0x5f, 0xcc, 0x10,
    0x15, 0x26, 0x1a, 0x41, 0x99, 0xed, 0xcd, 0x00, 0x00, 0x04, 0x52, 0xa1, 0xc4, 0x09, 0x9e, 0x40, 0x50, 0x35, 0xf1,
    0xc1, 0xa1, 0x40, 0xc6, 0x00, 0x40, 0xe4, 0xe8, 0x41, 0xe5, 0x59, 0xd9, 0xf8, 0xe8, 0xe8, 0xf4, 0x0f, 0xf7, 0xbf,
    0xb0, 0x01, 0x01, 0x6b, 0x1f, 0xe1, 0x92, 0xef, 0x9a, 0x7f, 0x3f, 0x7f, 0x2f, 0xa1, 0x85, 0x7b, 0xa2, 0xe8, 0xf8,
    0xe8, 0xf4, 0x67, 0xa6, 0xbf, 0xa9, 0xf5, 0xf4, 0xf0, 0x00, 0xc1, 0xc9, 0x2d, 0xb2, 0x7f, 0x2f, 0x7f, 0x2f, 0x01,
    0xb8, 0x3c, 0xbd, 0x2e, 0x1f, 0x1b, 0x01, 0x41, 0xa5, 0xb6, 0xc8, 0x01, 0x1c, 0xac, 0xbe, 0x89, 0xb6, 0xdc, 0xb5,
    0xfc, 0x7e, 0xb8, 0x28, 0x01, 0xb9, 0x5c, 0xb1, 0x01, 0x2c, 0xac, 0xbe, 0x45, 0xbe, 0xfb, 0xb5, 0xfd, 0x7e, 0xb8,
    0x28, 0x61, 0xad, 0x3d, 0xb5, 0x88, 0xff, 0xee, 0xff, 0x01, 0xca, 0x9a, 0xad, 0x88, 0xff, 0xee, 0xff, 0x21, 0xd2,
    0x9a, 0xad, 0x88, 0xff, 0xee, 0xff, 0x21, 0xd2, 0x9a, 0xad, 0x88, 0xff, 0xee, 0xff, 0x01, 0xca, 0x9a, 0xad, 0x00,
    0xff, 0x00, 0xff, 0xad, 0xcd, 0xbb, 0xb5, 0x00, 0xff, 0x00, 0xff, 0x8d, 0xcd, 0x9b, 0xb5, 0x00, 0xff, 0x00, 0xff,
    0x8d, 0xcd, 0x9b, 0xb5, 0x00, 0xff, 0x00, 0xff, 0x8d, 0xcd, 0xbb, 0xb5, 0x54, 0x55, 0x55, 0x55, 0x95, 0xc1, 0xb4,
    0xc5, 0x54, 0x55, 0x55, 0x55, 0x95, 0xc1, 0xb4, 0xc5, 0x54, 0x55, 0x55, 0x55, 0x95, 0xc1, 0xb4, 0xc5, 0x54, 0x55,
    0x55, 0x55, 0x95, 0xc1, 0xb4, 0xc5, 0x00, 0xff, 0x00, 0xff, 0x8d, 0xcd, 0xbb, 0xb5, 0x54, 0x55, 0x55, 0x55, 0x95,
    0xc1, 0xb4, 0xc5, 0x54, 0x55, 0x55, 0x55};

static i32 g_DebrisGlassDistortTID;

NuVertexFormatPS *g_nuDebrisVertexFormat;
NuVertexFormatPS *g_nuFaceOnVertexFormat;
NuVertexFormatPS *g_nuFallbackVertexFormat;
NuVertexFormatPS *g_nuFullscreenVertexFormat;
NuVertexFormatPS *g_nuPrimVertexFormat;

void NuMtlInitExPS(VARIPTR *buf) {
    NUNATIVETEX *tex;
    NUVERTEXDESCRIPTOR vtx_desc;

    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/numtl_android.cpp", 0x2ed);

    tex = (NUNATIVETEX *)buf->void_ptr;
    buf->addr = (usize)tex + sizeof(NUNATIVETEX);

    memset(tex, 0, sizeof(NUNATIVETEX));

    tex->size = sizeof(DebrisGlassIOS_Tex);
    tex->image_data = DebrisGlassIOS_Tex;
    g_DebrisGlassDistortTID = NuTexCreateNative(tex, true);

    vtx_desc.flags = 0;
    vtx_desc.has_position = true;

    g_nuFullscreenVertexFormat = NuGetVertexDeclaration(vtx_desc);

    vtx_desc.has_diffuse = true;
    vtx_desc.tex_coord_mode = 1;

    g_nuPrimVertexFormat = NuGetVertexDeclaration(vtx_desc);
    g_nuDebrisVertexFormat = NuGetVertexDeclaration(vtx_desc);
    g_nuFallbackVertexFormat = NuGetVertexDeclaration(vtx_desc);

    vtx_desc.tex_coord_mode = 5;
    g_nuFaceOnVertexFormat = NuGetVertexDeclaration(vtx_desc);

    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/numtl_android.cpp", 0x305);
}

void NuMtlCreatePS(NUMTL *mtl, i32 is_3d) {
    NuShaderMtlDescInit(&mtl->shader_desc);

    if (mtl->tex_id > 0) {
        mtl->shader_desc.diffuse_map_tex_id[0] = mtl->tex_id;
        mtl->shader_desc.unknown_a8 = 1;
        mtl->shader_desc.vtx_desc.tex_coord_mode = 1;
    } else {
        mtl->shader_desc.diffuse_color[0] = RGBA_TO_NUCOLOUR32(
            mtl->diffuse_color.r * 128.0f, mtl->diffuse_color.g * 128.0f, mtl->diffuse_color.b * 128.0f, 0xff);
    }

    if (!is_3d) {
        mtl->shader_desc.vtx_desc.has_no_transform = 1;
    }
}

void NuMtlSetShaderDescPS(NUMTL *mtl, NUSHADERMTLDESC *desc) {
    mtl->shader_desc = *desc;
}

// Material display-list backend (original numtl_android.cpp, 0x29bf60..).
extern "C" {
    NUSHADERPROGRAM *g_ps3default_2d_t0xc0;
}
static inline i32 NuApiFrameCount() { return nuapi.frame_count; }
static inline usize ptrToUsize(const void *p) { return reinterpret_cast<usize>(p); }

// Shader programmes cached per TU (original file-statics at 0x99b440..).
static NUSHADERPROGRAM *g_faceonProgram = nullptr;      // _ZL15g_faceonProgram
static NUSHADERPROGRAM *g_faceonDecalProgram = nullptr; // _ZL20g_faceonDecalProgram
static NUSHADERPROGRAM *g_debrisProgram = nullptr;      // _ZL15g_debrisProgram
static NUSHADERPROGRAM *g_debrisGlassProgram = nullptr;

#include "nuios_shader_sources.inc"

// Refraction texture used by glass debris — lazily allocated.
static i32 NuIOSDLMtlCallback_refractionRT = 0;                 // @0x99b480
static NUNATIVETEX NuIOSDLMtlCallback_nativeRefractionTex = {}; // @0x99b4a0
static i32 NuIOSDLMtlCallback_lastFrameCount = -1;


// ---------------------------------------------------------------------------
// Material-variant helpers — raw offsets from the original binary.
//
// The header's NUMTL/NUSHADERMTLDESC layout has drifted from the shipped
// binary, so the variant selectors are still addressed by absolute byte
// offset with the original address in the comment.  Named accessors keep
// call-sites readable while preserving the exact bytes the original tested.
// ---------------------------------------------------------------------------

static inline u8 MaterialVariantFlags(const numtl_s *mtl) {
    // Original: *(u8*)((u8*)mtl + 0x1F2) bits 0x10 = debris, 0x20 = face-on.
    return *(const u8 *)((const u8 *)mtl + 0x1F2);
}

static inline char FaceOnDecalSelector(const numtl_s *mtl) {
    // Original: *(char*)((u8*)mtl + 0x268) — maps to shader_desc.unknown_1b4
    // (use mtl->shader_desc.unknown_1b4 when the struct is fully typed).
    return *(const char *)((const u8 *)mtl + 0x268);
}

static inline char DebrisGlassSelector(const numtl_s *mtl) {
    // Original: *(char*)((u8*)mtl + 0x99) == -0x69 => glass debris path.
    return *(const char *)((const u8 *)mtl + 0x99);
}

static constexpr char kGlassDebrisMarker = (char)-0x69; // 0x97


// original 0x29c110 — mirrors GL cull state, flipping front/back when the
// reflection pass is active.
void NuIOS_SetCullMode(i32 mode) {
    static i32 s_prevCullMode = 0;                         // @0x628c50
    static i32 s_prevReflection = 0;                       // @0x628c60
    static const u32 kGlCullFace[2] = {GL_BACK, GL_FRONT}; // @0x57bcec

    if (mode == s_prevCullMode && s_prevReflection == g_renderingReflection) {
        return;
    }
    s_prevReflection = g_renderingReflection;

    // Mode 2 = double-sided: disable culling entirely.
    if (mode == 2) {
        glDisable(GL_CULL_FACE);
        s_prevCullMode = 2;
        return;
    }

    if (s_prevCullMode == 2) {
        glEnable(GL_CULL_FACE);
    }

    // Reflection XORs the winding, so the back/front choice is toggled.
    u32 idx = (u32)(mode + g_renderingReflection) & 1;
    glCullFace(kGlCullFace[idx]);
    s_prevCullMode = (i32)idx;
}

// Blend / alpha-test translation — original 0x29c1c0.
enum : u32 {
    kBlendOpaque = 0,
    kBlendAlpha = 1,        // srcA * src + (1-srcA) * dst
    kBlendAdd = 2,          // srcA * src + dst
    kBlendMax = 3,          // GL_MAX per channel (glow)
    kBlendAlphaTest10 = 10, // opaque + alpha-test (0x43 ref, func GEQUAL)
};

extern "C" void NuMtlSetRenderStatesPS(numtl_s *mtl) {
    u8 alpha_ref_byte = mtl->attribs.alpha_ref;
    bool isDebris = (mtl->shader_desc.vtx_desc.flags & 0x100000) != 0;

    if (!isDebris) {
        u32 alphaSel = (u32)(mtl->attribs.alpha_test & 7); // (bytes[0x42]>>4)&7
        if (alphaSel > 1) {
            if (alphaSel == 5) {
                g_alphaFunc = 5; // GEQUAL
                g_alphaTestEnabled = 1;
                g_alphaRef = alpha_ref_byte;
            } else {
                g_alphaFunc = 6; // GREATER
                g_alphaTestEnabled = 1;
                g_alphaRef = 0;
            }
        } else if (g_alphaTestEnabled != 0) {
            g_alphaTestEnabled = 0;
        }
    } else {
        // Debris materials force a minimal alpha-test.
        g_alphaFunc = 6;
        g_alphaTestEnabled = 1;
        g_alphaRef = 2;
    }

    // ---- blend mode ----
    u32 blend = mtl->attribs.alpha_mode & 0xf; // bytes[0x40] & 0xf
    switch (blend) {
        case kBlendOpaque:
            glDisable(GL_BLEND);
            break;
        case kBlendAlpha:
            glEnable(GL_BLEND);
            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case kBlendAdd:
            glEnable(GL_BLEND);
            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_SRC_ALPHA, GL_ONE);
            break;
        case kBlendMax:
            glEnable(GL_BLEND);
            // 0x800b is GL_MAX on desktop GL; GLES2 exposes it via EXT.
            glBlendEquationSeparate((GLenum)0x800b, GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ONE, GL_ONE);
            break;
        case kBlendAlphaTest10:
            glDisable(GL_BLEND);
            g_alphaTestEnabled = 1;
            g_alphaFunc = 5;
            g_alphaRef = alpha_ref_byte;
            break;
        default:
            break;
    }

    g_lastAlphaBlend = blend;
    g_lastAlphaRef = alpha_ref_byte;

    NuIOS_SetCullMode(mtl->attribs.cull_mode);
}


// original 0x29c070
extern "C" void NuIOS_SetVertexFormat(usize fmt) {
    g_boundVertexFormat = fmt;
}

// Helpers for the debris constant block (original walks the programme's
// i16 param table: pairs of {semantic, loc|class}).
namespace {
    constexpr i16 kParamViewProj = 0;        // semantic 0
    constexpr i16 kParamView = 0x0c;         // semantic 12
    constexpr i16 kParamKonstColourA = 0x30; // semantic 48
    constexpr i16 kParamTerminator = (i16)-0x8000;

} // namespace

// original 0x29bf60, 265 bytes — the four special-material programs are built from
// shader strings compiled into libTTapp.so rather than from scene resources.
extern "C" void NuIOSMtlInit(void) {
    g_packetToShaderStateMappings[0].mask.semantics[0] = 0;
    g_packetToShaderStateMappings[0].mask.semantics[1] = 0x0fe00000;
    g_packetToShaderStateMappings[0].mask.semantics[2] = 0x00806800;
    g_packetToShaderStateMappings[0].mask.semantics[3] = 0;
    g_packetToShaderStateMappings[1].mask.semantics[0] = 0;
    g_packetToShaderStateMappings[1].mask.semantics[1] = 0x60000000;
    g_packetToShaderStateMappings[1].mask.semantics[2] = 0x00400600;
    g_packetToShaderStateMappings[1].mask.semantics[3] = 0;
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/numtl_android.cpp", 0x1d0);
    g_ps3default_2d_t0xc0 = nullptr;
    g_faceonProgram = NuShaderProgramCreateIOS(reinterpret_cast<const char *>(FaceOn_vx),
                                               reinterpret_cast<const char *>(FaceOn_Hi_px));
    g_faceonDecalProgram = NuShaderProgramCreateIOS(reinterpret_cast<const char *>(FaceOn_vx),
                                                    reinterpret_cast<const char *>(FaceOn_Hi_px));
    g_debrisProgram =
        NuShaderProgramCreateIOS(reinterpret_cast<const char *>(debris_vx), reinterpret_cast<const char *>(debris_px));
    g_debrisGlassProgram = NuShaderProgramCreateIOS(reinterpret_cast<const char *>(debris_vx),
                                                    reinterpret_cast<const char *>(debris_glass_px));
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/numtl_android.cpp", 0x1f3);
}


// original 0x29c480 — per-material display-list callback.  Selects the GL
// programme, vertex format, and textures for the three material families:
//
//   * standard   (variantFlags & 0x10 == 0, & 0x20 == 0) — shader-manager
//     programme keyed by mtl->shader_desc.shader_id.
//   * face-on    (0x20 != 0) — billboard programmes g_faceonProgram /
//     g_faceonDecalProgram, driven by shader_desc.unknown_1b4.
//   * debris     (0x10 != 0) — g_debrisProgram or g_debrisGlassProgram;
//     glass additionally copies the backbuffer into refractionRT and binds
//     the distortion map at stage 1.
//
void NuIOSDLMtlCallback(void *arg) {
    auto *mtl = (numtl_s *)arg;

    g_boundMaterial = mtl;
    NUSHADEROBJECT *shaderId = NuShaderManagerGetShaderById(mtl->shader_desc.shader_id);
    g_LastMtl = mtl;
    g_renderContext_materialInUse = mtl;
    NuIOS_SetVertexFormat(ptrToUsize(mtl->vertex_decl));

    u8 variantFlags = MaterialVariantFlags(mtl);

    const bool isDebris = (variantFlags & 0x10) != 0;
    const bool isFaceOn = (variantFlags & 0x20) != 0;

    if (!isDebris) {
        if (!isFaceOn) {
            // ---- Standard material ----
            if (shaderId != 0) {
                g_boundShader = 0;
                glUseProgram(0);
                g_currentShaderProgram = nullptr;
                NuShaderManagerBindShader(shaderId);
                // BindShader may clobber the format; restore it.
                NuIOS_SetVertexFormat(ptrToUsize(mtl->vertex_decl));
            }
        } else {
            // ---- Face-on / billboard ----
            NuShaderManagerBindShader(0);
            g_boundVertexFormat = ptrToUsize(g_nuFaceOnVertexFormat);

            char decalSel = FaceOnDecalSelector(mtl);
            NUSHADERPROGRAM *program = (decalSel == '\0') ? g_faceonProgram : g_faceonDecalProgram;

            g_boundShader = program != nullptr ? program->program : 0;
            glUseProgram(g_boundShader);
            g_currentShaderProgram = program;

            NUNATIVETEX *tex = NuTexGetNative(mtl->tex_id);
            if (tex != nullptr) {
                NuTexSetTextureWithStagePS(tex, 0);
            }
        }
    } else {
        // ---- Debris ----
        if (DebrisGlassSelector(mtl) == kGlassDebrisMarker) {
            if (NuIOSDLMtlCallback_refractionRT == 0) {
                NuIOSDLMtlCallback_refractionRT = NuTexGenTexture(&NuIOSDLMtlCallback_nativeRefractionTex);
                memset(&NuIOSDLMtlCallback_nativeRefractionTex, 0, 8);
            }
            if (NuApiFrameCount() != NuIOSDLMtlCallback_lastFrameCount) {
                NuIOS_CopyBackbufferToTexture(&NuIOSDLMtlCallback_nativeRefractionTex, true);
                NuIOSDLMtlCallback_lastFrameCount = NuApiFrameCount();
            }
        }

        NUSHADERPROGRAM *program =
            DebrisGlassSelector(mtl) == kGlassDebrisMarker ? g_debrisGlassProgram : g_debrisProgram;

        g_boundVertexFormat = ptrToUsize(g_nuDebrisVertexFormat);
        NuShaderManagerBindShader(0);

        g_boundShader = program != nullptr ? program->program : 0;
        glUseProgram(g_boundShader);
        g_currentShaderProgram = program;

        for (i32 index = 0; index < program->parameter_count; ++index) {
            const NUSHADERPROGRAMPARAMETER *parameter = &program->parameters[index];
            if (parameter->register_index == static_cast<u16>(kParamViewProj)) {
                const u32 location = parameter->location_and_setter & 0x0fff;
                const u32 setter = parameter->location_and_setter >> 12;
                g_glConstantSetterTable[setter](location, 4, g_renderContext_viewProj);
                break;
            }
        }
        for (i32 index = 0; index < program->parameter_count; ++index) {
            const NUSHADERPROGRAMPARAMETER *parameter = &program->parameters[index];
            if (parameter->register_index == static_cast<u16>(kParamView)) {
                const u32 location = parameter->location_and_setter & 0x0fff;
                const u32 setter = parameter->location_and_setter >> 12;
                g_glConstantSetterTable[setter](location, 4, g_renderContext_view);
                break;
            }
        }
        for (i32 index = 0; index < program->parameter_count; ++index) {
            const NUSHADERPROGRAMPARAMETER *parameter = &program->parameters[index];
            if (parameter->register_index == static_cast<u16>(kParamKonstColourA)) {
                const u32 location = parameter->location_and_setter & 0x0fff;
                const u32 setter = parameter->location_and_setter >> 12;
                g_glConstantSetterTable[setter](location, 1, nu2api::g_shaderUniforms[72].data.values);
                break;
            }
        }
        for (i32 index = 0; index < program->parameter_count; ++index) {
            const NUSHADERPROGRAMPARAMETER *parameter = &program->parameters[index];
            if (parameter->register_index == static_cast<u16>(kParamTerminator)) {
                const u32 location = parameter->location_and_setter & 0x0fff;
                const u32 setter = parameter->location_and_setter >> 12;
                g_glConstantSetterTable[setter](location, 1, nu2api::g_shaderUniforms[71].data.values);
                break;
            }
        }

        if (DebrisGlassSelector(mtl) == kGlassDebrisMarker) {
            glActiveTexture(GL_TEXTURE0);
            g_currentTexUnit = 0;
            glBindTexture(GL_TEXTURE_2D, NuIOSDLMtlCallback_nativeRefractionTex.platform.gl_tex);
            NUNATIVETEX *distort = NuTexGetNative(g_DebrisGlassDistortTID);
            NuTexSetTextureWithStagePS(distort, 1);
        } else {
            NUNATIVETEX *tex = NuTexGetNative(mtl->tex_id);
            NuTexSetTextureWithStagePS(tex, 0);
        }
    }

    NuRenderContextSetZFunc(mtl->attribs.z_mode);
    g_renderingReflection = 0;
    NuMtlSetRenderStatesPS(mtl);
}
