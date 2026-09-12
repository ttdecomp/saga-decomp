// GLES2 display-list backend — material, cull, and vertex-format state.
//
// This is the iOS/Android counterpart to the PS2/PICA display-list
// consumer.  The game thread builds display lists (nudlist.cpp) that
// are later drained on the render thread; each list item is a callback
// into this TU:
//
//   NuIOSDLMtlCallback                           original 0x29c480
//   NuIOSDLGeom2DCallback                        original 0x293ad6
//   NuMtlSetRenderStatesPS                       original 0x29c1c0
//   NuIOS_SetCullMode                            original 0x29c110
//   NuIOS_SetVertexFormat                        original 0x29c070
//   NuIOS_BindVertexAttributesImmediate           original 0x2939fe
//   NuIOS_BindVertexAttributesImmediateOverride   original 0x293a65
//   NuIOS_BindVertexAttributesInternal            original 0x293841
//
// Original bss laid the per-TU shader programmes and refraction state
// at 0x99b440.. (g_faceonProgram / g_faceonDecalProgram /
// g_debrisProgram / g_DebrisGlassDistortTID / refractionRT …).

#include "nuiosdl_gl.h"

#include <GLES2/gl2.h>
#include <string.h>

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nutex_android.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuapi.h"

// ---------------------------------------------------------------------------
// Engine globals owned by this TU (original bss 0x99b440.. / 0x119b..).
// ---------------------------------------------------------------------------

u32 g_boundShader = 0;
NUSHADERPROGRAM *g_currentShaderProgram = nullptr;
numtl_s *g_boundMaterial = nullptr;
numtl_s *g_renderContext_materialInUse = nullptr;
numtl_s *g_LastMtl = nullptr;
usize g_boundVertexFormat = 0;
u32 g_activeAttributes = 0;
i32 g_renderContext_zFunc = 0;
u32 g_alphaRef = 0;
u32 g_alphaFunc = 0;
i32 g_alphaTestEnabled = 0;
u32 g_lastAlphaRef = 0;
u32 g_lastAlphaBlend = 0;
i32 g_renderingReflection = 0; // original bss @0x99b360 — flips cull when reflecting.

// The original helper at 0x293168 updates the shared renderer cache.
extern u32 g_lastBoundVAO;
static void NuIOSBindVAO(u32 vao) {
    if (vao != g_lastBoundVAO) {
        g_lastBoundVAO = vao;
    }
}

// Shader programmes cached per TU (original file-statics at 0x99b440..).
static NUSHADERPROGRAM *g_faceonProgram = nullptr;      // _ZL15g_faceonProgram
static NUSHADERPROGRAM *g_faceonDecalProgram = nullptr; // _ZL20g_faceonDecalProgram
static NUSHADERPROGRAM *g_debrisProgram = nullptr;      // _ZL15g_debrisProgram
static NUSHADERPROGRAM *g_debrisGlassProgram = nullptr;

#include "nuios_shader_sources.inc"

i32 g_DebrisGlassDistortTID = 0; // _ZL23g_DebrisGlassDistortTID @0x99b4c8

extern u32 g_DebriVB[8];
extern void *g_DebriSysMemVB[2][64];
extern u32 g_readBufferIndex;

// Refraction texture used by glass debris — lazily allocated.
static i32 NuIOSDLMtlCallback_refractionRT = 0;                 // @0x99b480
static NUNATIVETEX NuIOSDLMtlCallback_nativeRefractionTex = {}; // @0x99b4a0
static i32 NuIOSDLMtlCallback_lastFrameCount = -1;

// ---------------------------------------------------------------------------
// Cross-TU imports.
// ---------------------------------------------------------------------------

extern "C" void NuShaderManagerBindShader(NUSHADEROBJECT *shader);
extern "C" void NuShaderObjectGLSLSetupMaterial(NUSHADEROBJECT *shader_obj, numtl_s *mtl);
extern "C" NUSHADEROBJECT *NuShaderManagerGetShaderById(i32 id);
extern "C" NUSHADEROBJECT *NuShaderManagerGetCurrentShader(void);

extern i32 g_currentTexUnit; // nutex_ios_ex.cpp
extern NUAPI nuapi;

static inline isize PtrToArgInt(const void *p) {
    union {
        const void *ptr;
        isize val;
    } u = {p};
    return u.val;
}

static inline i32 NuApiFrameCount() {
    // Original 0x29c90e reads the counter at nuapi + 0x3c.
    return nuapi.frame_count;
}

static inline usize ptrToUsize(const void *p) {
    return reinterpret_cast<usize>(p);
}
static inline const void *usizeToPtr(usize value) {
    return reinterpret_cast<const void *>(value);
}

__attribute__((weak)) GLenum NuIOS_PlatformVertexAttributeType(GLenum type) {
    return type;
}

__attribute__((weak)) isize NuIOS_PlatformPrepareImmediateVertexData(isize data_address, usize) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return data_address;
}

NUNATIVETEX *NuTexGetNative(i32 tex_id);
void NuTexSetTextureWithStagePS(NUNATIVETEX *tex, u32 stage);
i32 NuTexGenTexture(NUNATIVETEX *tex);
void NuIOS_CopyBackbufferToTexture(NUNATIVETEX *tex, bool depth);

extern NuVertexFormatPS *g_nuPrimVertexFormat;   // 2D immediate stream format
extern NuVertexFormatPS *g_nuFaceOnVertexFormat; // billboard format
extern NuVertexFormatPS *g_nuDebrisVertexFormat; // debris format

// View matrices from the render context (original bss g_renderContext_viewProj
// @0x119bd00, g_renderContext_view @0x119bdc0).  Debris shaders need both.
extern "C" f32 g_renderContext_viewProj[16];
extern "C" f32 g_renderContext_view[16];
extern "C" f32 g_renderContext_world[16];
extern "C" f32 g_renderContext_kTint[4];
extern void (*g_glConstantSetterTable[4])(u32 loc, i32 count, const void *vals);
extern "C" void NuShaderManagerSetfv(i32 semantic, const f32 *values);
extern "C" void NuShaderManagerSetElementsfv(i32 semantic, i32 first_element, i32 count, const f32 *values);
extern "C" void NuShaderManagerSetElementsfv_transpose(i32 semantic, i32 first_element, i32 count, const f32 *values);
extern "C" void NuRenderContextSetViewProj(NUMTX *view, NUMTX *projection);

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

// ---------------------------------------------------------------------------
// GL state helpers.
// ---------------------------------------------------------------------------

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

// original 0x2a3860
extern "C" void NuRenderContextSetZFunc(i32 zfunc) {
    if (zfunc == g_renderContext_zFunc) {
        return;
    }

    switch (zfunc) {
        case 0: // depth test + write, LEQUAL
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            break;
        case 1: // depth test, no write (decal / transparent)
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            break;
        case 2: // no depth test, write enabled
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            break;
        case 3: // no depth test, no write (UI / 2D)
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            break;
        default:
            break;
    }
    g_renderContext_zFunc = zfunc;
}

// ---------------------------------------------------------------------------
// Vertex attribute binding — original 0x293841 / 0x2939fe / 0x293a65.
// ---------------------------------------------------------------------------

// Original attribute word array layout (leading dword = active mask,
// then 6 dwords per location):
//   [+0] mask
//   [+1] GL type         (e.g. GL_FLOAT)
//   [+2] component count (1..4)
//   [+3] normalized flag (GL_TRUE/GL_FALSE)
//   [+4] unused / padding
//   [+5] byte offset from vertex base
//   [+6] stride (bytes between vertices)
struct VertexAttribRecord {
    u32 gl_type;
    u32 comp_count;
    u8 normalized;
    u8 reserved[3];
    u32 pad;
    u32 byte_offset;
    u32 stride;
};

extern "C" {
    // Original 0x293841, 307 bytes.
    static void NuIOS_BindVertexAttributesInternal(isize dataAddr, usize baseVertex, const u32 *fmtWords, u32 mask) {
        i32 loc = 0;
        mask &= fmtWords[0];
        u32 toDisable = g_activeAttributes & ~mask;
        u32 toEnable = ~g_activeAttributes & mask;
        g_activeAttributes = mask;
        do {
            if (mask & 1) {
                const VertexAttribRecord *rec = reinterpret_cast<const VertexAttribRecord *>(fmtWords + loc * 6 + 1);
                if (toEnable & 1) {
                    glEnableVertexAttribArray(loc);
                }
                const void *address =
                    reinterpret_cast<const void *>(dataAddr + rec->byte_offset + baseVertex * rec->stride);
                const GLenum type = NuIOS_PlatformVertexAttributeType(static_cast<GLenum>(rec->gl_type));
                glVertexAttribPointer(loc, (GLint)rec->comp_count, type, (GLboolean)rec->normalized,
                                      (GLsizei)rec->stride, address);
            } else if (toDisable & 1) {
                glDisableVertexAttribArray(loc);
            }

            ++loc;
            mask >>= 1;
            toEnable >>= 1;
            toDisable >>= 1;
        } while ((mask | toEnable | toDisable) != 0);
    }
}

// original 0x2939fe — bind using the currently bound vertex format.
extern "C" {
    static void NuIOS_BindVertexAttributesImmediate(isize, isize dataAddr) {
        NuIOSBindVAO(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        NuIOS_BindVertexAttributesInternal(dataAddr, 0, reinterpret_cast<const u32 *>(g_boundVertexFormat),
                                           *reinterpret_cast<const u32 *>(g_boundVertexFormat));
    }

    static void NuIOS_BindVertexAttributes(isize dataAddr, usize baseVertex);
}

void NuIOSDLDebrisCallback(void *data) {
    nunativedebrisdata_s *packet = static_cast<nunativedebrisdata_s *>(data);
    if (packet->vertex_count == 0) {
        return;
    }
    g_boundVertexFormat = ptrToUsize(g_nuDebrisVertexFormat);
    if (packet->use_system_memory_vb == 0) {
        NuIOSBindVAO(0);
        glBindBuffer(GL_ARRAY_BUFFER, g_DebriVB[g_readBufferIndex * 4 + packet->vertex_buffer_index]);
        NuIOS_BindVertexAttributes(0, 0);
    } else {
        NuIOS_BindVertexAttributesImmediate(
            0, PtrToArgInt(g_DebriSysMemVB[g_readBufferIndex][packet->vertex_buffer_index]));
    }
    glDrawArrays(GL_TRIANGLES, packet->first_vertex, packet->vertex_count);
}

extern "C" {
    static void NuIOS_BindVertexAttributes(isize, usize baseVertex) {
        NuIOS_BindVertexAttributesInternal(0, baseVertex, reinterpret_cast<const u32 *>(g_boundVertexFormat),
                                           *reinterpret_cast<const u32 *>(g_boundVertexFormat));
    }

    // original 0x293a65 — bind immediate data with an explicit record layout.
    // The first argument is unused; the active mask still comes from the current
    // bound vertex format, while `fmt` supplies the attribute records.
    static void NuIOS_BindVertexAttributesImmediateOverrideDataLayout(isize, isize dataAddr, const u32 *fmt) {
        NuIOSBindVAO(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        const u32 *bound_format = static_cast<const u32 *>(usizeToPtr(g_boundVertexFormat));
        NuIOS_BindVertexAttributesInternal(dataAddr, 0, fmt, bound_format[0]);
    }
}

// ---------------------------------------------------------------------------
// Display-list callbacks.
// ---------------------------------------------------------------------------

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

extern "C" {
    NUSHADERPROGRAM *g_ps3default_2d_t0xc0;
}

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

extern "C" {
    // Original 0x293337, 76 bytes.
    static void NuRenderContextSetKTint(f32 *values) {
        *reinterpret_cast<NUVEC4 *>(g_renderContext_kTint) = *reinterpret_cast<NUVEC4 *>(values);
        NuShaderManagerSetfv(0x44, values);
    }

    // Original 0x293383, 22 bytes.
    static NUVEC4 *NuRenderContextGetKTint(void) {
        return reinterpret_cast<NUVEC4 *>(g_renderContext_kTint);
    }

    // Original 0x293399, 24 bytes.
    static numtl_s *NuRenderContextGetMaterialInUse(void) {
        return g_renderContext_materialInUse;
    }

    // Original 0x2933b1, 215 bytes.
    static void NuRenderContextSetZFunc_inline(i32 mode) {
        if (g_renderContext_zFunc != mode) {
            switch (mode) {
                case 0:
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                    glDepthFunc(GL_LEQUAL);
                    break;
                case 1:
                    glEnable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    glDepthFunc(GL_LEQUAL);
                    break;
                case 2:
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_TRUE);
                    break;
                case 3:
                    glDisable(GL_DEPTH_TEST);
                    glDepthMask(GL_FALSE);
                    break;
            }
        }
        g_renderContext_zFunc = mode;
    }

    // Original 0x293488, 620 bytes. Negation precedes unsigned-to-float conversion.
    static void NuIOS_GetAlphaTestParameters(f32 parameters[2]) {
        if (g_alphaTestEnabled != 0) {
            switch (g_alphaFunc) {
                case 5:
                    parameters[0] = 1.0f;
                    parameters[1] = static_cast<f32>(g_alphaRef) * (1.0f / 255.0f);
                    break;
                case 3:
                    parameters[0] = -1.0f;
                    parameters[1] = static_cast<f32>(-g_alphaRef) * (1.0f / 255.0f);
                    break;
                case 6:
                    parameters[0] = 1.0f;
                    parameters[1] = static_cast<f32>(g_alphaRef) * (1.0f / 255.0f) + (1.0f / 255.0f);
                    break;
                case 2:
                    parameters[0] = -1.0f;
                    parameters[1] = static_cast<f32>(-g_alphaRef) * (1.0f / 255.0f) - (1.0f / 255.0f) > 0.0f
                                        ? static_cast<f32>(-g_alphaRef) * (1.0f / 255.0f) - (1.0f / 255.0f)
                                        : 0.0f;
                    break;
                default:
                    parameters[0] = 0.0f;
                    parameters[1] = -1.0f;
                    break;
            }
        } else {
            parameters[0] = 0.0f;
            parameters[1] = -1.0f;
        }
    }
}

// Original 0x2936f4, 163 bytes.
extern "C" void NuShaderProgramSetVertexParamfv(NUSHADERPROGRAM *program, u32 register_index, const f32 *values,
                                                i32 component_count) {
    for (i32 i = 0; i < program->parameter_count; ++i) {
        NUSHADERPROGRAMPARAMETER *parameter = &program->parameters[i];
        if (parameter->register_index == register_index) {
            g_glConstantSetterTable[parameter->setter](parameter->location, (component_count + 3) / 4, values);
            break;
        }
    }
}

// Original 0x293797, 170 bytes.
extern "C" void NuShaderProgramSetFragmentParamfv(NUSHADERPROGRAM *program, u32 register_index, const f32 *values,
                                                  i32 component_count) {
    register_index |= 0x8000;
    for (i32 i = 0; i < program->parameter_count; ++i) {
        NUSHADERPROGRAMPARAMETER *parameter = &program->parameters[i];
        if (parameter->register_index == register_index) {
            g_glConstantSetterTable[parameter->setter](parameter->location, (component_count + 3) / 4, values);
            break;
        }
    }
}

// Original 0x294a9e, 628 bytes — matrices and constants consumed by FaceOn_vx.
void NuIOSDLFaceOnTransformCallback(void *arg) {
    f32 opacity = 1.0f;
    VARIPTR packet;
    packet.void_ptr = arg;
    f32 values[4];
    values[0] = static_cast<NUMTX *>(packet.void_ptr)->m30;
    values[1] = static_cast<NUMTX *>(packet.void_ptr)->m31;
    values[2] = static_cast<NUMTX *>(packet.void_ptr)->m32;
    values[3] = 1.0f;
    static_cast<NUMTX *>(packet.void_ptr)->m30 = 0.0f;
    static_cast<NUMTX *>(packet.void_ptr)->m31 = 0.0f;
    static_cast<NUMTX *>(packet.void_ptr)->m32 = 0.0f;
    NUSHADERPROGRAM *program = g_currentShaderProgram;
    if (program != nullptr) {
        opacity = static_cast<NUMTX *>(packet.void_ptr)->m33;
        static_cast<NUMTX *>(packet.void_ptr)->m33 = 1.0f;

        NuShaderProgramSetVertexParamfv(program, 0x50, static_cast<const f32 *>(packet.void_ptr), 16);
        packet.char_ptr += sizeof(NUMTX);
        NuShaderProgramSetVertexParamfv(program, 0x59, values, 4);
        values[0] = *packet.f32_ptr++;
        values[1] = 1.0f;
        values[2] = values[3] = 0.0f;
        NuShaderProgramSetVertexParamfv(program, 0x54, values, 4);
        NuShaderProgramSetVertexParamfv(program, 0x55, static_cast<const f32 *>(packet.void_ptr), 16);
        NuShaderProgramSetVertexParamfv(program, 0, g_renderContext_viewProj, 16);
        NuShaderProgramSetVertexParamfv(program, 0xc, static_cast<const f32 *>(packet.void_ptr), 16);
        f32 alpha_test[2];
        NuIOS_GetAlphaTestParameters(alpha_test);
        NuShaderProgramSetFragmentParamfv(program, 0x70, alpha_test, 2);
        NUVEC4 tint = *NuRenderContextGetKTint();
        if (opacity < 1.0f) {
            tint.w *= opacity;
            NuRenderContextSetZFunc_inline(1);
            NuShaderProgramSetVertexParamfv(program, 0x28, &tint.x, 4);
        } else {
            numtl_s *material = NuRenderContextGetMaterialInUse();
            NuRenderContextSetZFunc_inline(material->attribs.z_mode);
            NuShaderProgramSetVertexParamfv(program, 0x28, &tint.x, 4);
        }
    }
}

extern "C" {
    // Original 0x2939bb, 67 bytes. Override layout, retaining the bound attribute mask.
    static void NuIOS_BindVertexAttributesOverrideDataLayout(usize, const u32 *format) {
        NuIOS_BindVertexAttributesInternal(0, 0, format, *reinterpret_cast<const u32 *>(g_boundVertexFormat));
    }
}

// original 0x295393, 141 bytes — every face-on entry is expanded to two triangles in
// the scene vertex buffer.
void NuIOSDLFaceOnCallback(void *arg) {
    if (arg != nullptr) {
        auto *packet = static_cast<NuFaceOnDrawPacket *>(arg);
        NuIOSBindVAO(0);
        glBindBuffer(GL_ARRAY_BUFFER, packet->vertex_buffer);
        NuIOS_BindVertexAttributesOverrideDataLayout(0, static_cast<const u32 *>(g_nuFaceOnVertexFormat));
        glDrawArrays(GL_TRIANGLES, packet->first_vertex, packet->face_count * 6);
    } else {
        return;
    }
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

// original 0x293ad6 — 2D geometry callback.  Binds the 2D vertex format
// and issues the draw for the geom item built by nurndr_plain's Prim2D.
void NuIOSDLGeom2DCallback(void *arg) {
    struct Geom2DItem {
        u32 prim_type;
        u32 pad0;
        u16 pad1;
        u16 vertex_count; // at +0xa
        u32 pad2;
        u8 vertices[0]; // tightly packed PrimVertexRaw array
    };

    auto *geom = (Geom2DItem *)arg;
    if (geom->vertex_count == 0) {
        return;
    }

    NUSHADEROBJECT *shader = NuShaderManagerGetShaderById(g_LastMtl ? g_LastMtl->shader_desc.shader_id : -1);

    if (shader == NULL || shader->glsl.program == 0) {
        return;
    }

    NuIOSBindVAO(0);
    NuShaderObjectGLSLSetupMaterial(shader, g_LastMtl);

    static const u32 kPrimModes[5] = {
        GL_TRIANGLES,      // 0
        GL_TRIANGLE_STRIP, // 1
        GL_LINES,          // 2
        GL_LINE_LOOP,      // 3
        GL_TRIANGLES,      // 4 — quads expanded to triangles by NuPrim2DAddXYZ
    };

    u32 pt = geom->prim_type;
    if (pt < 5) {
        NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, PtrToArgInt(geom->vertices),
                                                              (const u32 *)g_nuPrimVertexFormat);
        glDrawArrays((GLenum)kPrimModes[pt], 0, (GLsizei)geom->vertex_count);
    }
}

// original 0x2a430d — 3D geometry callback
void NuIOSDLGeomCallback(void *arg) {
    auto *geom = static_cast<NUDISPLAYLISTGEOM *>(arg);
    i32 primitive_count = geom->index_count;
    i32 vertex_count = 0;
    NUSHADEROBJECT *shader;
    usize vertex_format;
    isize immediate_vertices = reinterpret_cast<isize>(arg);
    immediate_vertices += sizeof(NUDISPLAYLISTGEOM);
    shader = NuShaderManagerGetCurrentShader();
    if (shader == NULL || shader->glsl.program == 0) {
        return;
    }

    NuShaderObjectGLSLSetupMaterial(shader, g_LastMtl);
    switch (geom->primitive_type) {
        case 6:
            vertex_count = primitive_count + 2;
            if (geom->immediate != 0) {
                if (geom->dynamic_vertex_data != nullptr) {
                    NuIOSBindVAO(0);
                    glBindBuffer(GL_ARRAY_BUFFER, geom->vertex_format);
                    glBufferData(GL_ARRAY_BUFFER, geom->vertex_count * geom->vertex_stride, nullptr, GL_DYNAMIC_DRAW);
                    glBufferData(GL_ARRAY_BUFFER, geom->vertex_count * geom->vertex_stride, geom->dynamic_vertex_data,
                                 GL_DYNAMIC_DRAW);
                    NuIOS_BindVertexAttributes(0, 0);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->index_buffer);
                    glDrawElements(GL_TRIANGLE_STRIP, vertex_count, GL_UNSIGNED_SHORT,
                                   (const void *)(usize)(geom->first_index * 2));
                } else {
                    NuIOSBindVAO(0);
#ifdef __EMSCRIPTEN__
                    // WebGL requires buffer-backed attributes instead of client arrays.
                    NuIOS_BindVertexAttributes(NuIOS_PlatformPrepareImmediateVertexData(
                                                   geom->vertex_buffer + geom->base_vertex * geom->vertex_stride,
                                                   geom->vertex_count * geom->vertex_stride),
                                               0);
#else
                    NuIOS_BindVertexAttributesImmediate(0,
                                                        geom->base_vertex * geom->vertex_stride + geom->vertex_buffer);
#endif
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->index_buffer);
                    glDrawElements(GL_TRIANGLE_STRIP, vertex_count, GL_UNSIGNED_SHORT,
                                   (const void *)(usize)(geom->first_index * 2));
                }
            } else {
                vertex_format = geom->vertex_format;
                NuIOS_SetVertexFormat(vertex_format);
                NuIOSBindVAO(0);
                glBindBuffer(GL_ARRAY_BUFFER, geom->vertex_buffer);
                NuIOS_BindVertexAttributes(0, geom->base_vertex);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->index_buffer);
                glDrawElements(GL_TRIANGLE_STRIP, vertex_count, GL_UNSIGNED_SHORT,
                               (const void *)(usize)(geom->first_index * 2));
            }
            break;
        case 0:
            vertex_count = geom->vertex_count;
            NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, immediate_vertices,
                                                                  (const u32 *)g_nuPrimVertexFormat);
            glDrawArrays(GL_TRIANGLES, 0, vertex_count);
            break;
        case 1:
            vertex_count = geom->vertex_count;
            NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, immediate_vertices,
                                                                  (const u32 *)g_nuPrimVertexFormat);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);
            break;
        case 2:
            vertex_count = geom->vertex_count;
            NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, immediate_vertices,
                                                                  (const u32 *)g_nuPrimVertexFormat);
            glDrawArrays(GL_LINES, 0, vertex_count);
            break;
        case 3:
            vertex_count = geom->vertex_count;
            NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, immediate_vertices,
                                                                  (const u32 *)g_nuPrimVertexFormat);
            glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
            break;
        case 5:
            vertex_count = geom->vertex_count;
            NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, immediate_vertices,
                                                                  (const u32 *)g_nuPrimVertexFormat);
            glDrawArrays(GL_TRIANGLE_FAN, 0, vertex_count);
            break;
        case 0x32:
            vertex_count = geom->vertex_count;
            NuIOS_BindVertexAttributesImmediateOverrideDataLayout(0, immediate_vertices,
                                                                  (const u32 *)g_nuPrimVertexFormat);
            glDrawArrays(GL_POINTS, 0, vertex_count);
            break;
        default:
            break;
    }
    NuIOSBindVAO(0);
}

extern "C" {
    // original 0x2931b9, 303 bytes.
    static void NuRenderContextSetWorld(NUMTX *world) {
        NUMTX transforms[3];
        transforms[0] = *world;
        NuMtxMulH(&transforms[1], world, reinterpret_cast<NUMTX *>(g_renderContext_viewProj));
        NuMtxMul(&transforms[2], world, reinterpret_cast<NUMTX *>(g_renderContext_view));
        NuShaderManagerSetElementsfv(0x52, 0, 3, reinterpret_cast<const f32 *>(transforms));
        NuShaderManagerSetfv(0x3c, reinterpret_cast<const f32 *>(world));
    }

    // original 0x2932e8, 79 bytes. The render-stream matrix is transposed in place.
    static void NuRenderContextSetWorld_transpose(NUMTX *world) {
        NuMtxTranspose(world, world);
        NuShaderManagerSetElementsfv_transpose(0x3c, 0, 1, reinterpret_cast<const f32 *>(world));
    }
}

// original 0x2947cc, 258 bytes — installs a display-list world transform and applies
// the per-instance opacity to the current tint.
// Original 0x293ad1, 5 bytes: deliberately empty on this platform.
static void Nu360SetObjectShadowFactor(f32) {
}

void NuIOSDLTransformCallback(void *arg) {
    auto *world = static_cast<NUMTX *>(arg);
    NUVEC4 tint = *NuRenderContextGetKTint();
    const f32 opacity = world->m33;
    const f32 shadow_factor = world->m23;

    if (opacity < 1.0f) {
        tint.w *= opacity;
        NuRenderContextSetZFunc_inline(1);
        NuShaderManagerSetfv(0x44, &tint.x);
    } else {
        numtl_s *material = NuRenderContextGetMaterialInUse();
        NuRenderContextSetZFunc_inline(material->attribs.z_mode);
        NuShaderManagerSetfv(0x44, &tint.x);
    }
    Nu360SetObjectShadowFactor(shadow_factor);

    Nu360SetObjectShadowFactor(shadow_factor);
    world->m33 = 1.0f;
    world->m23 = 0.0f;
    NuRenderContextSetWorld(world);
    world->m33 = opacity;
    world->m23 = shadow_factor;
}

// original 0x294935, 258 bytes — dynamic special transforms are stored transposed in
// the render stream. Restore the ordinary world matrix before publishing it
// to the shader state.
void NuIOSDLTransformParamsCallback(void *arg) {
    auto *stream_matrix = static_cast<NUMTX *>(arg);
    NUVEC4 tint = *NuRenderContextGetKTint();
    const f32 opacity = stream_matrix->m33;
    const f32 shadow_factor = stream_matrix->m32;
    numtl_s *material = NuRenderContextGetMaterialInUse();

    if (opacity < 1.0f) {
        tint.w *= opacity;
        NuRenderContextSetZFunc_inline(1);
        NuShaderManagerSetfv(0x44, &tint.x);
    } else {
        NuRenderContextSetZFunc_inline(material->attribs.z_mode);
        NuShaderManagerSetfv(0x44, &tint.x);
    }
    Nu360SetObjectShadowFactor(shadow_factor);

    Nu360SetObjectShadowFactor(shadow_factor);
    stream_matrix->m33 = 1.0f;
    stream_matrix->m32 = 0.0f;
    NuRenderContextSetWorld_transpose(stream_matrix);
    stream_matrix->m33 = opacity;
    stream_matrix->m32 = shadow_factor;
}

void NuIOSDLKonstCallback(void *arg) {
    NuRenderContextSetKTint(static_cast<f32 *>(arg));
}

// Original 0x294d12, 129 bytes. Two header words precede the group data.
void NuIOSDLVertexGroupsCallback(void *arg) {
    const i32 max_groups = 32;
    VARIPTR packet;
    packet.void_ptr = arg;
    i32 group_count = *packet.u32_ptr++;
    i32 flags = *packet.u32_ptr++;
    const f32 *values = static_cast<const f32 *>(packet.void_ptr);
    i32 vector_count = (group_count + 3) / 4;
    NuShaderManagerSetElementsfv(0x51, 0, vector_count, values);
}

// original 0x294dfe, 692 bytes — installs the light packet produced by
// RndrStateBuildLightState into the shader semantic state.
void NuIOSDLLightsCallback(void *arg) {
    g_boundLightPacket = arg;
    auto *lights = static_cast<NULIGHTSTATE *>(arg);
    NuShaderManagerSetfv(0x35, reinterpret_cast<const f32 *>(&lights->ambient_intensity));
    NuShaderManagerSetfv(0x39, reinterpret_cast<const f32 *>(&lights->light_direction[0]));
    NuShaderManagerSetfv(0x3a, reinterpret_cast<const f32 *>(&lights->light_direction[1]));
    NuShaderManagerSetfv(0x3b, reinterpret_cast<const f32 *>(&lights->light_direction[2]));
    NuShaderManagerSetfv(0x36, reinterpret_cast<const f32 *>(&lights->light_intensity[0]));
    NuShaderManagerSetfv(0x37, reinterpret_cast<const f32 *>(&lights->light_intensity[1]));
    NuShaderManagerSetfv(0x38, reinterpret_cast<const f32 *>(&lights->light_intensity[2]));
    NuShaderManagerSetfv(0x4b, reinterpret_cast<const f32 *>(&lights->specular_mtx));

    NUVEC4 average_direction = {
        lights->light_direction[0].x + lights->light_direction[1].x + lights->light_direction[2].x,
        lights->light_direction[0].y + lights->light_direction[1].y + lights->light_direction[2].y,
        lights->light_direction[0].z + lights->light_direction[1].z + lights->light_direction[2].z,
        1.0f,
    };
    NuVecNorm(reinterpret_cast<NUVEC *>(&average_direction), reinterpret_cast<NUVEC *>(&average_direction));
    NuShaderManagerSetfv(0x4e, &average_direction.x);

    const f32 max_r = lights->light_intensity[1].r <= lights->light_intensity[2].r ? lights->light_intensity[2].r
                                                                                   : lights->light_intensity[1].r;
    const f32 max_g = lights->light_intensity[1].g <= lights->light_intensity[2].g ? lights->light_intensity[2].g
                                                                                   : lights->light_intensity[1].g;
    const f32 max_b = lights->light_intensity[1].b <= lights->light_intensity[2].b ? lights->light_intensity[2].b
                                                                                   : lights->light_intensity[1].b;
    f32 average_colour[4] = {
        lights->light_intensity[0].r <= max_r ? max_r : lights->light_intensity[0].r,
        lights->light_intensity[0].g <= max_g ? max_g : lights->light_intensity[0].g,
        lights->light_intensity[0].b <= max_b ? max_b : lights->light_intensity[0].b,
        1.0f,
    };
    NuShaderManagerSetfv(0x4d, average_colour);
    f32 specular_intensity[4] = {
        lights->specular_intensity.x,
        lights->specular_intensity.y,
        lights->specular_intensity.z,
        1.0f,
    };
    NuShaderManagerSetfv(0x57, specular_intensity);
}

extern "C" void NuRenderContextSetViewport(i32 x, i32 y, i32 width, i32 height);

// original 0x2a5128, 204 bytes — camera packets carry view/projection matrices at +4
// and +0x44 respectively. The viewport tail is deliberately a no-op on this
// platform, matching NuRenderContextSetViewport in the original.
void NuIOSDLCameraCallback(void *arg) {
    struct NuIOSCameraPacket {
        i32 id;
        NUMTX view;
        NUMTX projection;
        f32 viewport[4];
    };
    static i32 last_id = -1;
    g_boundCameraPacket = arg;
    auto *packet = static_cast<NuIOSCameraPacket *>(arg);
    if (packet->id != last_id) {
        NUMTX *view = &packet->view;
        NUMTX *projection = view + 1;
        f32 *viewport = reinterpret_cast<f32 *>(projection + 1);
        last_id = packet->id;
        NuRenderContextSetViewProj(view, projection);
        NuRenderContextSetViewport(static_cast<i32>(viewport[0]), static_cast<i32>(viewport[1]),
                                   static_cast<i32>(viewport[2]), static_cast<i32>(viewport[3]));
    } else {
        return;
    }
}

// original 0x294233 — records the material's vertex format on static geometry
// while a scene is being fixed up.
void NuIOSDLPreWarmGeomCallback(void *arg) {
    if (g_LastMtl == nullptr || g_LastMtl->shader_desc.blend_op2 == 0xff) {
        return;
    }
    NUSHADEROBJECT *shader = NuShaderManagerGetShaderById(static_cast<i16>(g_LastMtl->shader_desc.shader_id));
    if (shader == NULL || shader->glsl.program == 0) {
        return;
    }

    NuShaderObjectGLSLSetupTextureStates(shader, g_LastMtl);
    auto *geometry = static_cast<NUDISPLAYLISTGEOM *>(arg);
    NuIOSBindVAO(0);
    if (geometry->primitive_type == 6 && geometry->immediate == 0) {
        geometry->vertex_format = g_boundVertexFormat;
    }
}
