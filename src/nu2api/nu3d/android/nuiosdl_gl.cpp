// GLES2 display-list geometry backend and shared render-context state.
//
// This is the iOS/Android counterpart to the PS2/PICA display-list
// consumer.  The game thread builds display lists (nudlist.cpp) that
// are later drained on the render thread; each list item is a callback
// into this TU:
//
//   NuIOSDLGeom2DCallback                        original 0x293ad6
//   NuIOS_BindVertexAttributesImmediate           original 0x2939fe
//   NuIOS_BindVertexAttributesImmediateOverride   original 0x293a65
//   NuIOS_BindVertexAttributesInternal            original 0x293841
//
// Material callbacks and their file-local shader/refraction state live in
// numtl_android.cpp, matching the original material translation unit.

#include "nuiosdl_gl.h"

#include <GLES2/gl2.h>
#include <string.h>

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nutex_android.h"
#include "nu2api/nu3d/android/nudlist_callbacks.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuapi.h"

// ---------------------------------------------------------------------------
// Shared render-context globals owned by this TU.
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


// ---------------------------------------------------------------------------
// Cross-TU imports.
// ---------------------------------------------------------------------------

extern "C" void NuShaderManagerBindShader(NUSHADEROBJECT *shader);
extern "C" void NuShaderObjectGLSLSetupMaterial(NUSHADEROBJECT *shader_obj, numtl_s *mtl);
extern "C" NUSHADEROBJECT *NuShaderManagerGetShaderById(i32 id);
extern "C" NUSHADEROBJECT *NuShaderManagerGetCurrentShader(void);

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
// GL state helpers.
// ---------------------------------------------------------------------------

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
        NuIOS_BindVertexAttributesOverrideDataLayout(0, reinterpret_cast<const u32 *>(g_nuFaceOnVertexFormat));
        glDrawArrays(GL_TRIANGLES, packet->first_vertex, packet->face_count * 6);
    } else {
        return;
    }
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

// Original 0x294764. The skin packet begins with the number of palette
// matrices followed by their contiguous 4x4 values.
void NuIOSDLSkinMtxCallback(void *data) {
    i32 *packet = static_cast<i32 *>(data);
    const i32 matrix_count = *packet++;
    NUSHADEROBJECT *shader = NuShaderManagerGetCurrentShader();
    if (shader != NULL) {
        NuShaderObjectSetElementsfv(shader, 0x5a, 0, matrix_count * 4, reinterpret_cast<const f32 *>(packet));
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

// Original 0x294d93. The packet stores a count followed by up to eight vec4
// vertex-offset entries for semantic 0x50.
void NuIOSDLVertexOffsetsCallback(void *arg) {
    const i32 *packet = static_cast<const i32 *>(arg);
    i32 count = packet[0];
    if (count > 8) {
        count = 8;
    }
    NuShaderManagerSetElementsfv(0x50, 0, count, reinterpret_cast<const f32 *>(packet + 1));
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

// Original 0x2951b0. Publish the packed fog colour and range to the shader.
void NuIOSDLFogCallback(void *arg) {
    const NUFOGSTATE *fog = static_cast<const NUFOGSTATE *>(arg);
    if (fog->enabled != 0) {
        const u32 colour = fog->colour;
        const f32 fog_colour[4] = {
            static_cast<f32>(colour & 0xff) / 255.0f,
            static_cast<f32>((colour >> 8) & 0xff) / 255.0f,
            static_cast<f32>((colour >> 16) & 0xff) / 255.0f,
            static_cast<f32>(colour >> 24) / 255.0f,
        };
        const f32 fog_params[4] = {
            fog->near_distance,
            fog->far_distance,
            fog->far_distance - fog->near_distance,
            fog->density,
        };
        NuShaderManagerSetfv(0x47, fog_colour);
        NuShaderManagerSetfv(0x48, fog_params);
    } else {
        const f32 fog_params[4] = {100000.0f, 0.0f, 100000.0f, 0.0f};
        NuShaderManagerSetfv(0x48, fog_params);
    }
}

// Original 0x295420 -- legacy packet containing three texture ids.
void NuIOSDLLightmapOld(void *arg) {
    const i32 *texture_ids = static_cast<const i32 *>(arg);
    for (i32 index = 0; index < 3; ++index) {
        const i32 texture_id = texture_ids[index] > 0 ? texture_ids[index] : 1;
        NUNATIVETEX *texture = NuTexGetNative(texture_id);
        glActiveTexture(GL_TEXTURE0 + index);
        g_currentTexUnit = index;
        glBindTexture(GL_TEXTURE_2D, texture->platform.gl_tex != 0 ? texture->platform.gl_tex : g_whiteTexture);
    }

    const f32 shader_offset[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    NuShaderManagerSetfv(0x58, shader_offset);
}

// Original 0x2954f0 -- legacy three-lightmap packet followed by a UV offset.
void NuIOSDLLightmapOffsetOld(void *arg) {
    const i32 *texture_ids = static_cast<const i32 *>(arg);
    for (i32 index = 0; index < 3; ++index) {
        const i32 texture_id = texture_ids[index] > 0 ? texture_ids[index] : 1;
        NUNATIVETEX *texture = NuTexGetNative(texture_id);
        glActiveTexture(GL_TEXTURE0 + index);
        g_currentTexUnit = index;
        glBindTexture(GL_TEXTURE_2D, texture->platform.gl_tex != 0 ? texture->platform.gl_tex : g_whiteTexture);
    }

    const f32 *offset = reinterpret_cast<const f32 *>(texture_ids + 3);
    const f32 shader_offset[4] = {offset[0], -offset[1], 0.0f, 0.0f};
    NuShaderManagerSetfv(0x58, shader_offset);
}

// Original 0x2955ee -- lightmap display-list packet. Mode 1 installs one
// lightmap; mode 2 walks the packet's three lightmap ids. The latter selects
// texture unit zero for each entry in the original binary.
void NuIOSDLLightmap(void *arg) {
    i32 *packet = static_cast<i32 *>(arg);
    const i32 mode = packet[0];

    if (mode == 1) {
        const i32 texture_id = packet[1] > 0 ? packet[1] : 1;
        NUNATIVETEX *texture = NuTexGetNative(texture_id);
        glActiveTexture(GL_TEXTURE0);
        g_currentTexUnit = 0;
        glBindTexture(GL_TEXTURE_2D, texture->platform.gl_tex != 0 ? texture->platform.gl_tex : g_whiteTexture);
    } else if (mode == 2) {
        for (i32 index = 0; index < 3; ++index) {
            const i32 texture_id = packet[index + 2] > 0 ? packet[index + 2] : 1;
            NUNATIVETEX *texture = NuTexGetNative(texture_id);
            glActiveTexture(GL_TEXTURE0);
            g_currentTexUnit = 0;
            glBindTexture(GL_TEXTURE_2D, texture->platform.gl_tex != 0 ? texture->platform.gl_tex : g_whiteTexture);
        }
    } else {
        return;
    }

    const f32 *offset = reinterpret_cast<const f32 *>(packet + 5);
    const f32 shader_offset[4] = {offset[0], -offset[1], 0.0f, 0.0f};
    NuShaderManagerSetfv(0x58, shader_offset);
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
