#include "nu2api/nu3d/numtl.h"
#include "decomp.h"
#include "nu2api/nu3d/nu2api_nu3d_types.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nucore/nustring.h"

#include "nu2api/numath/numtx.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "nu2api/nucore/nustring.h"

// Original 0x2a56a0, 81 bytes.
bool LinkShaderProgram(u32 program) {
    glLinkProgram(program);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    return linked != 0;
}

// Original 0x2a5700, 171 bytes.
bool ValidateShaderProgram(u32 program) {
    glValidateProgram(program);
    GLint log_length;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 0) {
        char *log = static_cast<char *>(malloc(log_length));
        glGetProgramInfoLog(program, log_length, &log_length, log);
        free(log);
    }
    GLint valid;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);
    return valid != 0;
}

// Weak COMDAT helper emitted by the original shader implementation. Keeping
// the real C++ tag (`GLSLParameter`) is ABI-significant: the tag, not a typedef
// alias, is encoded in the symbol name.
void __attribute__((weak)) GLSLParameter::setElementsMatrix(i32 first_element, i32 count, const f32 *values) {
    NUMTX transposed[32];
    for (i32 i = 0; i < count; ++i) {
        NuMtxTranspose(&transposed[i], const_cast<NUMTX *>(reinterpret_cast<const NUMTX *>(values) + i));
    }

    i32 vector_count = count * 4;
    const i32 vectors_remaining = (element_count_and_setter >> 2) - first_element * 4;
    if (vector_count > vectors_remaining) {
        vector_count = vectors_remaining;
    }
    glUniform4fv(location + first_element * 4, vector_count, reinterpret_cast<const f32 *>(transposed));
}

#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/nustring.h"

struct nushaderuniform_e {
    i32 name_kind;
};

struct GLSLTypeInfo {
    GLenum gl_type;
    u32 parameter_type;
    u32 setter_class;
    u32 element_count;
};

extern "C" {
    extern i32 g_semanticMaskCount;
    extern NUSHADERUSAGEMASK g_semanticMasks[128];
}

extern u32 g_boundShader;
extern void (*g_glConstantSetterTable[4])(u32 location, i32 count, const void *values);

static f32 water_theta_step = 0.26666668f;

namespace {
    static const GLSLTypeInfo kGLSLTypeInfo[] = {
        {GL_FLOAT, 1, 0, 1},      {GL_FLOAT_VEC2, 1, 1, 1}, {GL_FLOAT_VEC3, 1, 2, 1},
        {GL_FLOAT_VEC4, 1, 3, 1}, {GL_FLOAT_MAT2, 3, 1, 2}, {GL_FLOAT_MAT3, 3, 2, 3},
        {GL_FLOAT_MAT4, 3, 3, 4}, {GL_SAMPLER_2D, 4, 0, 1}, {GL_SAMPLER_CUBE, 4, 0, 1},
    };

    static NUSHADERUSAGEMASK *GetUsageMask(const NUSHADERUSAGEMASK *mask) {
        for (i32 i = 0; i < g_semanticMaskCount; ++i) {
            if (memcmp(&g_semanticMasks[i], mask, sizeof(*mask)) == 0) {
                return &g_semanticMasks[i];
            }
        }
        NUSHADERUSAGEMASK *result = &g_semanticMasks[g_semanticMaskCount++];
        *result = *mask;
        return result;
    }
} // namespace

extern "C" const GLSLTypeInfo *GetGLSLTypeInfo(GLenum type) {
    if (type == GL_FLOAT)
        return &kGLSLTypeInfo[0];
    if (type == GL_FLOAT_VEC2)
        return &kGLSLTypeInfo[1];
    if (type == GL_FLOAT_VEC3)
        return &kGLSLTypeInfo[2];
    if (type == GL_FLOAT_VEC4)
        return &kGLSLTypeInfo[3];
    if (type == GL_FLOAT_MAT2)
        return &kGLSLTypeInfo[4];
    if (type == GL_FLOAT_MAT3)
        return &kGLSLTypeInfo[5];
    if (type == GL_FLOAT_MAT4)
        return &kGLSLTypeInfo[6];
    if (type == GL_SAMPLER_2D)
        return &kGLSLTypeInfo[7];
    if (type == GL_SAMPLER_CUBE)
        return &kGLSLTypeInfo[8];
    return NULL;
}

extern "C" {
    i32 g_semanticMaskCount;
    NUSHADERUSAGEMASK g_semanticMasks[128];
}

void NuShaderObjectBaseCreate(NUSHADEROBJECTBASE *shader) {
    shader->field0 = -1;
    shader->field1 = 0;
    shader->key = 0;
    shader->field3 = 0;
}

void NuShaderObjectGLSLCreate(NUSHADEROBJECTGLSL *shader) {
    NuShaderObjectBaseCreate(&shader->base);
}

void NuShaderObjectCreate(NUSHADEROBJECT *shader) {
    NuShaderObjectGLSLCreate(&shader->glsl);

    for (i32 i = 0; i < NUSHADEROBJECT_PARAMETERS_COUNT; ++i) {
        shader->parameters[i].semantic = i;
        shader->parameters[i].location = -1;
    }
}

// Original 0x30ba90. Parameter metadata is populated by the GLSL semantic
// probe; the low nibble selects scalar/vector/matrix upload behaviour.
extern "C" void NuShaderObjectSetElementsfv(NUSHADEROBJECT *shader, i32 semantic, i32 first_element, i32 count,
                                            const f32 *values) {
    GLSLParameter &parameter = shader->parameters[semantic];
    if (parameter.location < 0) {
        return;
    }

    switch (parameter.type_and_flags & 0x0f) {
        case 2:
            glUniform4fv(parameter.location + first_element, count, values);
            break;
        case 1:
            g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location + first_element, count,
                                                                            values);
            break;
        case 3:
            parameter.setElementsMatrix(first_element, count, values);
            break;
    }
}

// Original 0x30bb60. The caller has already transposed matrix payloads, so
// matrix parameters are uploaded as their contiguous vec4 rows instead of
// passing through GLSLParameter::setElementsMatrix a second time.
extern "C" void NuShaderObjectSetElementsfv_transpose(NUSHADEROBJECT *shader, i32 semantic, i32 first_element,
                                                      i32 count, const f32 *values) {
    GLSLParameter &parameter = shader->parameters[semantic];
    if (parameter.location < 0) {
        return;
    }

    switch (parameter.type_and_flags & 0x0f) {
        case 2:
            glUniform4fv(parameter.location + first_element, count, values);
            break;
        case 1:
            g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location + first_element, count,
                                                                            values);
            break;
        case 3: {
            i32 vector_count = count * 4;
            const i32 vectors_remaining = (parameter.element_count_and_setter >> 2) - first_element * 4;
            if (vector_count > vectors_remaining) {
                vector_count = vectors_remaining;
            }
            glUniform4fv(parameter.location + first_element * 4, vector_count, values);
            break;
        }
    }
}

void NuShaderObjectBaseDestroy(NUSHADEROBJECTBASE *shader) {
    shader->field1 = 0;
}

void NuShaderObjectGLSLDestroy(NUSHADEROBJECTGLSL *shader) {
    if (shader->fragment_shader != 0) {
        glDeleteShader(shader->fragment_shader);
        shader->fragment_shader = 0;
    }

    if (shader->vertex_shader != 0) {
        glDeleteShader(shader->vertex_shader);
        shader->vertex_shader = 0;
    }

    if (shader->program != 0) {
        glDeleteProgram(shader->program);
        shader->program = 0;
    }
}

void NuShaderObjectDestroy(NUSHADEROBJECT *shader) {
}

void NuShaderObjectBaseInit(NUSHADEROBJECTBASE *shader, NUSHADEROBJECTKEY *key, i32 unk) {
    memcpy(&shader->key, key, sizeof(shader->key));
    shader->field0 = unk;
}

void NuShaderObjectUnInit(NUSHADEROBJECT *shader) {
}

void NuShaderObjectBaseUnInit(NUSHADEROBJECTBASE *shader) {
    shader->field0 = -1;
}

void NuShaderObjectBaseSetWaterSpeed(f32 speed) {
    water_theta_step = speed * 0.1f;
}

i32 NuShaderObjectBindAttributeLocationsGLSL(GLuint program) {
    static GLchar infoLog[0x2000];

    GLint params;

    glBindAttribLocation(program, 0, "cg_Vertex");
    glBindAttribLocation(program, 1, "COLOR");
    glBindAttribLocation(program, 2, "SPECULAR");
    glBindAttribLocation(program, 3, "NORMAL");
    glBindAttribLocation(program, 4, "TANGENT");
    glBindAttribLocation(program, 5, "TEXCOORD4");
    glBindAttribLocation(program, 6, "TEXCOORD0");
    glBindAttribLocation(program, 7, "TEXCOORD1");
    glBindAttribLocation(program, 8, "TEXCOORD6");
    glBindAttribLocation(program, 9, "TEXCOORD7");
    glBindAttribLocation(program, 10, "BLENDWEIGHT0");
    glBindAttribLocation(program, 11, "BLENDINDICES0");
    glBindAttribLocation(program, 12, "TEXCOORD5");

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &params);
    if (params) { // weird register swap issue with the matching here...
        return 1;
    }

    glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
    return 0;
}

bool NuShaderObjectCombineGLSLShadersIntoProgram(GLuint *program_dest, GLuint vertex_shader, GLuint fragment_shader) {
    // these were most definitely macros
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 228);
    GLuint program = glCreateProgram();
    *program_dest = program;
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 230);
    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }

    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 232);
    glAttachShader(*program_dest, vertex_shader);
    glAttachShader(*program_dest, fragment_shader);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 235);
    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }

    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 237);
    i32 bind_result = NuShaderObjectBindAttributeLocationsGLSL(*program_dest);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 239);
    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }

    return bind_result;
}

bool NuShaderObjectGenerateGLSLShader(GLuint *shader_dest, GLenum shader_type, const GLchar *shader_source,
                                      GLint shader_source_length) {
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 197);
    GLuint shader = glCreateShader(shader_type);
    *shader_dest = shader;
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 200);
    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }

    GLint params = 0;
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 204);
    glShaderSource(shader, 1, &shader_source, &shader_source_length);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &params);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 208);
    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }

    return 1;
}

// original 0x30b970 — Init(obj,key,i,uint,uint,version) forwards to
// InitGLSL(obj,key,i,vshader,pshader) and then probes semantics.
void NuShaderObjectInit(nushaderobject_s *obj, nushaderobjectkey_s const *key, i32 param, u32 vshader, u32 pshader,
                        eSHADERVERSION) {
    NuShaderObjectInitGLSL((nushaderobjectglsl_s *)obj, (nushaderobjectkey_s const *)key, param, vshader, pshader);
    NuShaderObjectGLSLProbeSemantics(obj);
}

// original 0x30b050 — BaseInit, stash both shader objects, then build and
// link the GL program; destroy the object when linking failed.
void NuShaderObjectInitGLSL(nushaderobjectglsl_s *obj, nushaderobjectkey_s const *key, i32 param, u32 vshader,
                            u32 pshader) {
    NuShaderObjectBaseInit(&obj->base, (NUSHADEROBJECTKEY *)key, param);
    obj->fragment_shader = pshader; // +0x18
    obj->vertex_shader = vshader;   // +0x14
    if (!NuShaderObjectCombineGLSLShadersIntoProgram(&obj->program, vshader, pshader)) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                               298);
        NuShaderObjectGLSLDestroy(obj);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 300);
        if (bgProcIsBgThread())
            NuIOS_YieldThread();
    }
}

void NuShaderObjectInitGLSL(nushaderobjectglsl_s *obj, nushaderobjectkey_s const *key, i32 param, char const *vsource,
                            i32 vsize, char const *psource, i32 psize) {
    NuShaderObjectBaseInit(&obj->base, (NUSHADEROBJECTKEY *)key, param);
    if (!NuShaderObjectGenerateGLSLShader(&obj->vertex_shader, GL_VERTEX_SHADER, vsource, vsize)) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                               256);
        NuShaderObjectGLSLDestroy(obj);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 258);
        if (bgProcIsBgThread())
            NuIOS_YieldThread();
        return;
    }
    if (psource) {
        if (!NuShaderObjectGenerateGLSLShader(&obj->fragment_shader, GL_FRAGMENT_SHADER, psource, psize)) {
            BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                                   266);
            NuShaderObjectGLSLDestroy(obj);
            EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                                 268);
            if (bgProcIsBgThread())
                NuIOS_YieldThread();
            return;
        }
    } else {
        obj->fragment_shader = 0;
    }
    if (!NuShaderObjectCombineGLSLShadersIntoProgram(&obj->program, obj->vertex_shader, obj->fragment_shader)) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                               279);
        NuShaderObjectGLSLDestroy(obj);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 281);
        if (bgProcIsBgThread())
            NuIOS_YieldThread();
    }
}

void NuShaderObjectInitGLSL(nushaderobjectglsl_s *obj, nushaderobjectkey_s const *key, i32 param, char const *vsource,
                            i32 vsize, u32 pshader) {
    NuShaderObjectBaseInit(&obj->base, (NUSHADEROBJECTKEY *)key, param);
    obj->fragment_shader = pshader;
    if (!NuShaderObjectGenerateGLSLShader(&obj->vertex_shader, GL_VERTEX_SHADER, vsource, vsize)) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                               317);
        NuShaderObjectGLSLDestroy(obj);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 319);
        if (bgProcIsBgThread())
            NuIOS_YieldThread();
        return;
    }
    if (!NuShaderObjectCombineGLSLShadersIntoProgram(&obj->program, obj->vertex_shader, obj->fragment_shader)) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                               325);
        NuShaderObjectGLSLDestroy(obj);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 327);
        if (bgProcIsBgThread())
            NuIOS_YieldThread();
    }
}

void NuShaderObjectInitGLSL(nushaderobjectglsl_s *obj, nushaderobjectkey_s const *key, i32 param, u32 vshader,
                            char const *psource, i32 psize) {
    NuShaderObjectBaseInit(&obj->base, (NUSHADEROBJECTKEY *)key, param);
    obj->vertex_shader = vshader;
    if (psource) {
        if (!NuShaderObjectGenerateGLSLShader(&obj->fragment_shader, GL_FRAGMENT_SHADER, psource, psize)) {
            BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                                   346);
            NuShaderObjectGLSLDestroy(obj);
            EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                                 348);
            if (bgProcIsBgThread())
                NuIOS_YieldThread();
            return;
        }
    } else {
        obj->fragment_shader = 0;
    }
    if (!NuShaderObjectCombineGLSLShadersIntoProgram(&obj->program, obj->vertex_shader, obj->fragment_shader)) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp",
                               359);
        NuShaderObjectGLSLDestroy(obj);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp", 361);
        if (bgProcIsBgThread())
            NuIOS_YieldThread();
    }
}

i32 NuShaderObjectGLSLGetSemanticIndex(const char *name, nushaderuniform_e &uniform) {
    for (i32 semantic = 0; semantic < 0x65; ++semantic) {
        if (NuStrCmp(name + 1, g_shaderUniforms[semantic].vertex_name) == 0) {
            uniform.name_kind = 1;
            return semantic;
        }
        if (NuStrCmp(name + 1, g_shaderUniforms[semantic].fragment_name) == 0) {
            uniform.name_kind = 2;
            return semantic;
        }
    }
    return -1;
}

extern "C" GLSLParameter *NuShaderObjectGLSLAllocateParameter(NUSHADEROBJECT *shader, i32 semantic) {
    return &shader->parameters[semantic];
}

// Original 0x30b560, retaining the complete active-uniform walk, sampler-unit
// assignment, and parameter metadata construction.
extern "C" void NuShaderObjectGLSLProbeSemantics(NUSHADEROBJECT *shader) {
    if (shader->glsl.program == 0) {
        return;
    }

    static const char *source_path =
        "i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp";
    BeginCriticalSectionGL(source_path, 582);

    GLint uniform_count = 0;
    glGetProgramiv(shader->glsl.program, GL_ACTIVE_UNIFORMS, &uniform_count);
    NUSHADERUSAGEMASK usage_mask = {};
    i32 sampler_count = 0;
    for (GLint i = 0; i < uniform_count; ++i) {
        char uniform_name[256];
        GLint array_size = 0;
        GLenum type = 0;
        glGetActiveUniform(shader->glsl.program, i, sizeof(uniform_name), NULL, &array_size, &type, uniform_name);
        char *array_suffix = strchr(uniform_name, '[');
        if (array_suffix != NULL) {
            *array_suffix = '\0';
        }

        nushaderuniform_e uniform;
        const i32 semantic = NuShaderObjectGLSLGetSemanticIndex(uniform_name, uniform);
        if (semantic < 0) {
            if (type == GL_SAMPLER_2D) {
                i32 lightmap_unit = -1;
                if (NuStrCmp(uniform_name, "_lightmap0") == 0) {
                    lightmap_unit = 0;
                } else if (NuStrCmp(uniform_name, "_lightmap1") == 0) {
                    lightmap_unit = 1;
                } else if (NuStrCmp(uniform_name, "_lightmap2") == 0) {
                    lightmap_unit = 2;
                }
                if (lightmap_unit >= 0) {
                    const GLint location = glGetUniformLocation(shader->glsl.program, uniform_name);
                    glUseProgram(shader->glsl.program);
                    glUniform1i(location, lightmap_unit);
                    glUseProgram(0);
                    g_boundShader = 0;
                }
            }
            continue;
        }

        GLSLParameter &parameter = *NuShaderObjectGLSLAllocateParameter(shader, semantic);
        parameter.element_count_and_setter = (parameter.element_count_and_setter & 3) | 4;
        usage_mask.semantics[semantic >> 5] |= 1u << (semantic & 31);

        const GLSLTypeInfo *type_info = GetGLSLTypeInfo(type);
        if (type_info != NULL) {
            parameter.type_and_flags = (parameter.type_and_flags & 0xf0) | (type_info->parameter_type & 0x0f);
            parameter.element_count_and_setter = (type_info->setter_class & 3) | (type_info->element_count << 2);
        }

        if ((parameter.type_and_flags & 0x0f) == 4) {
            const GLint location = glGetUniformLocation(shader->glsl.program, uniform_name);
            const i32 texture_unit = sampler_count++ + 3;
            glUseProgram(shader->glsl.program);
            glUniform1i(location, texture_unit);
            glUseProgram(0);
            g_boundShader = 0;
            parameter.location = static_cast<i16>(texture_unit | 0x800);
        } else {
            parameter.location = glGetUniformLocation(shader->glsl.program, uniform_name);
        }
        parameter.array_size = array_size;
        const u8 setter_class = parameter.element_count_and_setter & 3;
        const u8 element_count = parameter.element_count_and_setter >> 2;
        parameter.element_count_and_setter = setter_class | (element_count * array_size << 2);
    }

    shader->usage_mask = GetUsageMask(&usage_mask);
    EndCriticalSectionGL(source_path, 676);
    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }
}

// Original specialized bodies: 360 bytes at 0x2a5390 and 412 bytes at 0x2a5500.
static i32 GetHLSLRegisterIndex(const char *source, const char *uniform_name, bool texture) {
    char search_name[256];
    sprintf(search_name, "_%s ", uniform_name + 1);
    const char *constants = strstr(source, "//NU2API CONSTANTS :");
    const char *match = strstr(source, search_name);
    const char *attributes = strstr(source, "//NU2API ATTRIBS :");
    if (constants == NULL || match == NULL || (attributes != NULL && match >= attributes)) {
        return -1;
    }
    const char *number = match + NuStrLen(search_name);
    if (texture) {
        if (NuStrNICmp(number, "TEXUNIT", NuStrLen("TEXUNIT")) == 0) {
            number += NuStrLen("TEXUNIT");
        }
    } else if (NuToUpper(static_cast<u8>(*number)) == 'C') {
        ++number;
    }
    if (!isdigit(static_cast<unsigned char>(*number))) {
        return -1;
    }
    i32 index = NuAToI(const_cast<char *>(number));
    const char *declaration = strstr(number, uniform_name + 1);
    if (declaration == NULL || declaration <= number) {
        return -1;
    }
    for (const char *cursor = declaration - 1; *cursor != '\n'; --cursor) {
        if (NuStrNCmp(cursor, "//var ", NuStrLen("//var ")) != 0) {
            return index;
        }
        if (cursor == number) {
            break;
        }
    }
    return -1;
}

// The 16-byte type alignment supplies the trailing padding in the original 0x810-byte pool.
struct __attribute__((aligned(16))) ShaderProgramPool {
    NUSHADERPROGRAM programs[64];
    u8 occupied[8];
    i32 next;
};
DECOMP_ASSERT(sizeof(ShaderProgramPool) == 0x810, "Shader program pool ABI");
DECOMP_ASSERT(offsetof(ShaderProgramPool, occupied) == 0x800, "Shader program occupancy offset");
DECOMP_ASSERT(offsetof(ShaderProgramPool, next) == 0x808, "Shader program cursor offset");
static ShaderProgramPool programPool;
extern "C" {
    NUSHADERPROGRAMPARAMETER g_uniformParameterRecordStorage[1024];
}
static i32 g_uniformParameterRecordAllocator;
static char uniformName[256];

extern "C" NUSHADERPROGRAM *NuShaderProgramCreateIOS(const char *vertex_source, const char *fragment_source) {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    NuStrLen(vertex_source);
    char *precision = const_cast<char *>(strstr(vertex_source, "precision mediump float;"));
    if (precision != NULL) {
        memcpy(precision, "precision highp float;  ", NuStrLen("precision highp float;  "));
    }
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);
    GLint compiled = 0;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == 0) {
        glDeleteShader(vertex_shader);
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    NuStrLen(fragment_source);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);
    compiled = 0;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == 0) {
        glDeleteShader(fragment_shader);
        return NULL;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    LinkShaderProgram(program);
    NuShaderObjectBindAttributeLocationsGLSL(program);
    NUSHADERPROGRAM *result = NULL;
    const i32 start = programPool.next;
    for (i32 pass = 0; pass < 2 && result == NULL; ++pass) {
        const i32 end = pass == 0 ? 64 : start;
        for (i32 slot = pass == 0 ? start : 0; slot < end; ++slot) {
            if ((programPool.occupied[slot / 8] & (1 << (slot & 7))) == 0) {
                programPool.occupied[slot / 8] |= 1 << (slot & 7);
                programPool.next = (slot + 1) % 64;
                result = &programPool.programs[slot];
                break;
            }
        }
    }
    result->vertex_shader = vertex_shader;
    result->fragment_shader = fragment_shader;
    result->program = program;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &result->parameter_count);
    if (result->parameter_count != 0) {
        result->parameters = &g_uniformParameterRecordStorage[g_uniformParameterRecordAllocator];
        g_uniformParameterRecordAllocator += result->parameter_count;
    } else {
        result->parameters = NULL;
    }
    i32 removed = 0;
    for (i32 i = 0; i < result->parameter_count; ++i) {
        GLint count;
        GLenum type;
        glGetActiveUniform(result->program, i, sizeof(uniformName), NULL, &count, &type, uniformName);
        char *array_suffix = strchr(uniformName, '[');
        if (array_suffix != NULL) {
            *array_suffix = '\0';
        }

        NUSHADERPROGRAMPARAMETER *parameter = &result->parameters[i - removed];
        i32 register_index = GetHLSLRegisterIndex(vertex_source, uniformName, false);
        u16 stage_flag = 0;
        if (register_index == -1) {
            register_index = GetHLSLRegisterIndex(fragment_source, uniformName, false);
            stage_flag = 0x8000;
        }
        parameter->register_index = stage_flag | static_cast<u16>(register_index);
        parameter->location = glGetUniformLocation(result->program, uniformName);
        const GLSLTypeInfo *type_info = GetGLSLTypeInfo(type);
        if (type_info == NULL) {
            ++removed;
        } else if (type_info->parameter_type == 4) {
            const i32 texture_unit = GetHLSLRegisterIndex(fragment_source, uniformName, true);
            if (texture_unit != -1) {
                const GLint location = glGetUniformLocation(result->program, uniformName);
                glUseProgram(result->program);
                glUniform1i(location, texture_unit);
                glUseProgram(0);
                g_boundShader = 0;
            }
            ++removed;
        } else {
            parameter->setter = type_info->setter_class;
        }
    }
    result->parameter_count -= removed;
    g_uniformParameterRecordAllocator -= removed;
    return result;
}

// Additional overloads present in original (char* shader sources)
void NuShaderObjectInit(nushaderobject_s *obj, nushaderobjectkey_s const *key, i32 param, const char *vshader,
                        i32 vsize, u32 pshader, eSHADERVERSION) {
    NuShaderObjectInitGLSL((nushaderobjectglsl_s *)obj, key, param, vshader, vsize, pshader);
    NuShaderObjectGLSLProbeSemantics(obj);
}

void NuShaderObjectInit(nushaderobject_s *obj, nushaderobjectkey_s const *key, i32 param, u32 vshader,
                        const char *pshader, i32 psize, eSHADERVERSION) {
    NuShaderObjectInitGLSL((nushaderobjectglsl_s *)obj, key, param, vshader, pshader, psize);
    NuShaderObjectGLSLProbeSemantics(obj);
}

extern "C" void NuShaderObjectInit(nushaderobject_s *obj, nushaderobjectkey_s const *key, i32 param,
                                   const char *vsource, i32 vsize) {
    const char *psource = "precision lowp float;\nvoid main() { gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0); }";
    NuShaderObjectInitGLSL((nushaderobjectglsl_s *)obj, key, param, vsource, vsize, psource, NuStrLen(psource));
    NuShaderObjectGLSLProbeSemantics(obj);
}

extern "C" NUSHADEROBJECT *NuShaderObjectUnserialize(VARIPTR *buffer) {
    usize address = (buffer->addr + 3) & ~static_cast<usize>(3);
    address += *reinterpret_cast<u32 *>(address);
    address = (address + 7) & ~static_cast<usize>(3);
    buffer->addr = address + sizeof(NUSHADEROBJECT);
    return reinterpret_cast<NUSHADEROBJECT *>(address);
}

#include "nu2api/nucore/nuapi.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nutrig.h"

extern "C" void NuShaderObjectBaseUpdateWaterTable(NUSHADEROBJECT *shader, numtl_s *mtl) {
    static NUVEC4 waterTable[32];
    static i32 lastintsame = -1;
    static numtl_s *prev_mtl;
    static f32 theta = 0.7f;
    i32 frame;
    memcpy(&frame, &nuapi.frame_count, sizeof(frame));
    const f32 *material = reinterpret_cast<const f32 *>(mtl);
    if (frame != lastintsame)
        theta = material[0x60 / 4] * water_theta_step + theta;
    if (frame != lastintsame || mtl != prev_mtl) {
        NUMTX inverse;
        NUVEC scale = {0.5f, 0.5f, 0.5f};
        NuMtxInvR(&inverse, &global_camera.mtx);
        NuMtxScale(&inverse, &scale);
        inverse.m03 = inverse.m13 = inverse.m23 = 0.0f;
        const f32 amplitude = 0.1f * material[0x6c / 4];
        u32 seed = 17;
        NuRandFloatSeeded(&seed);
        for (i32 i = 0; i < 32; ++i) {
            NUVEC displacement;
            f32 phase = (NuRandFloatSeeded(&seed) * 0.4f + 0.8f) * theta;
            i32 angle = static_cast<i32>((NuRandFloatSeeded(&seed) * 6.283f + phase) * 10430.3779296875f);
            displacement.x = (amplitude * NuTrigTable[(angle >> 1) & 0x7fff]) * 4.0f;
            phase = (NuRandFloatSeeded(&seed) * 0.8f + 0.6f) * theta;
            angle = static_cast<i32>((NuRandFloatSeeded(&seed) * 5.717f + phase) * 10430.3779296875f);
            displacement.y = (amplitude * NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff]) * 4.0f;
            phase = (NuRandFloatSeeded(&seed) * 0.4f + 0.7f) * theta;
            angle = static_cast<i32>((NuRandFloatSeeded(&seed) * 6.283f + phase) * 10430.3779296875f);
            displacement.z = amplitude * NuTrigTable[(angle >> 1) & 0x7fff];
            waterTable[i].w = 0.25f * displacement.x;
            NuVecMtxTransformH(reinterpret_cast<NUVEC *>(&waterTable[i]), &displacement, &inverse);
        }
    }
    NuShaderObjectSetElementsfv(shader, 31, 0, 32, reinterpret_cast<const f32 *>(waterTable));
    prev_mtl = mtl;
    memcpy(&lastintsame, &nuapi.frame_count, sizeof(lastintsame));
}

#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nu3d/android/nutex_android.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nutex.h"

static char g_clzTable[] = {
    0, 31, 9, 30, 3, 8,  18, 29, 2,  5,  7,  14, 12, 17, 22, 28,
    1, 10, 4, 19, 6, 15, 13, 23, 11, 20, 16, 24, 21, 25, 26, 27,
};

static u32 ShaderCountLeadingZeros(u32 value) {
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value ? g_clzTable[((value + 1) * 0x07dcd629u) >> 27] : 32;
}

static void NuShaderObjectGLSLSetCustomSetterParameters(nushaderobjectglsl_s *glsl, NuShaderUsageMask_s &mask,
                                                        numtl_s *mtl) {
    NUSHADEROBJECT *shader = reinterpret_cast<NUSHADEROBJECT *>(glsl);
    const NUSHADERUSAGEMASK *usage = &mask;
    auto unpackColour = [](u32 packed, f32 *colour) {
        colour[0] = static_cast<f32>(packed & 0xff) / 255.0f;
        colour[1] = static_cast<f32>((packed >> 8) & 0xff) / 255.0f;
        colour[2] = static_cast<f32>((static_cast<i32>(packed) >> 16) & 0xff) / 255.0f;
        colour[3] = static_cast<f32>(static_cast<i32>(packed >> 24)) / 255.0f;
    };

    const u8 *material = reinterpret_cast<const u8 *>(mtl);
    auto materialFloat = [material](usize offset) { return *reinterpret_cast<const f32 *>(material + offset); };
    auto materialU32 = [material](usize offset) { return *reinterpret_cast<const u32 *>(material + offset); };

    // Each semantic has its own upload storage, as in the original setter.
    // The numeric suffix identifies its shader semantic.
    f32 values21[4];
    f32 values22[4];
    f32 values23[1];
    f32 values24[4];
    f32 values25[4];
    f32 values26[4];
    f32 values27[4];
    f32 values28[1];
    f32 values29[1];
    f32 values30[4];
    f32 values32[4];
    f32 values33[4];
    f32 values34[4];
    f32 values35[4];
    f32 values36[4];
    f32 values37[4];
    f32 values38[4];
    f32 values39[4];
    f32 values40[1];
    f32 values45[4];
    f32 values46[4];
    f32 values47[1];
    f32 values48[4];
    f32 values49[16];
    f32 values50[4];
    f32 values51[4];
    f32 values52[2];
    // Material semantics 21..52 occupy one shifted 32-bit mask.
    u32 active = (usage->semantics[0] >> 21) | (usage->semantics[1] << 11);
    for (;;) {
        const u32 leading = ShaderCountLeadingZeros(active);
        if (leading > 31) {
            break;
        }
        const i32 semantic = 52 - leading;
        GLSLParameter &parameter = shader->parameters[semantic];
        const f32 *four_values;
        switch (semantic) {
            case 21: {
                unpackColour(materialU32(0x11c), values21);
                four_values = values21;
                goto upload_four;
            }
            case 22: {
                unpackColour(materialU32(0x120), values22);
                values22[3] = materialFloat(0x1b4);
                four_values = values22;
                goto upload_four;
            }
            case 23: {
                values23[0] = material[0xfb] == 0 ? 1.0f : -1.0f;
                g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location, 1, values23);
                break;
            }
            case 24: {
                values24[0] = materialFloat(0x134);
                values24[1] = 1.0f;
                values24[2] = 0.035f * materialFloat(0x138);
                values24[3] = materialFloat(0x14c);
                four_values = values24;
                goto upload_four;
            }
            case 25: {
                values25[2] = 0.0f;
                values25[3] = 0.0f;
                values25[0] = materialFloat(0xf0);
                values25[1] = materialFloat(0x284) / materialFloat(0x274);
                four_values = values25;
                goto upload_four;
            }
            case 26: {
                values26[0] = materialFloat(0x130);
                values26[1] = materialFloat(0x12c);
                values26[2] = materialFloat(0x144);
                values26[3] = materialFloat(0x148);
                four_values = values26;
                goto upload_four;
            }
            case 27: {
                values27[3] = 0.0f;
                values27[0] = materialFloat(0x140);
                values27[1] = materialFloat(0x13c);
                values27[2] = materialFloat(0x248);
                four_values = values27;
                goto upload_four;
            }
            case 28: {
                values28[0] = materialFloat(0x114);
                g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location, 1, values28);
                break;
            }
            case 29: {
                values29[0] = materialFloat(0x118);
                g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location, 1, values29);
                break;
            }
            case 30: {
                values30[2] = 0.0f;
                values30[3] = 0.0f;
                values30[0] = materialFloat(0x1b8);
                values30[1] = materialFloat(0x1bc);
                four_values = values30;
                goto upload_four;
            }
            case 31:
                NuShaderObjectBaseUpdateWaterTable(shader, mtl);
                break;
            case 32: {
                unpackColour(materialU32(0xc8), values32);
                four_values = values32;
                goto upload_four;
            }
            case 33: {
                unpackColour(materialU32(0xcc), values33);
                four_values = values33;
                goto upload_four;
            }
            case 34: {
                unpackColour(materialU32(0xd0), values34);
                four_values = values34;
                goto upload_four;
            }
            case 35: {
                unpackColour(materialU32(0xd4), values35);
                four_values = values35;
                goto upload_four;
            }
            case 36: {
                values36[0] = materialFloat(0xd8);
                values36[1] = materialFloat(0xdc);
                values36[2] = materialFloat(0xe0);
                values36[3] = materialFloat(0xe4);
                four_values = values36;
                goto upload_four;
            }
            case 37: {
                unpackColour(materialU32(0x128), values37);
                glUniform3fv(parameter.location, 1, values37);
                break;
            }
            case 38: {
                unpackColour(materialU32(0xf4), values38);
                glUniform3fv(parameter.location, 1, values38);
                break;
            }
            case 39: {
                unpackColour(materialU32(0x124), values39);
                values39[3] = materialFloat(0x158);
                four_values = values39;
                goto upload_four;
            }
            case 40: {
                values40[0] = (materialFloat(0x150) - 1.0f) * 0.1f;
                g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location, 1, values40);
                break;
            }
            case 41:
                glUniform2fv(parameter.location, 1, reinterpret_cast<const f32 *>(material + 0x1d0));
                break;
            case 42:
                glUniform2fv(parameter.location, 1, reinterpret_cast<const f32 *>(material + 0x1d8));
                break;
            case 43:
                glUniform2fv(parameter.location, 1, reinterpret_cast<const f32 *>(material + 0x1e0));
                break;
            case 44:
                glUniform2fv(parameter.location, 1, reinterpret_cast<const f32 *>(material + 0x1e8));
                break;
            case 45: {
                values45[0] = 0.05f;
                values45[1] = 0.32f * materialFloat(0x60);
                values45[2] = 0.2f;
                values45[3] = 0.8f;
                four_values = values45;
                goto upload_four;
            }
            case 46: {
                values46[0] = materialFloat(0x60);
                values46[1] = materialFloat(0x64);
                values46[2] = materialFloat(0x68);
                values46[3] = (values46[2] * values46[1]) * 0.2f;
                four_values = values46;
                goto upload_four;
            }
            case 47: {
                values47[0] = materialFloat(0x154);
                g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location, 1, values47);
                break;
            }
            case 48: {
                values48[2] = 0.0f;
                values48[3] = 0.0f;
                values48[0] = materialFloat(0x260);
                values48[1] = materialFloat(0x264);
                four_values = values48;
                goto upload_four;
            }
            case 49: {
                for (i32 colour = 0; colour < 4; ++colour) {
                    unpackColour(materialU32(0x250 + colour * 4), values49 + colour * 4);
                }
                glUniform4fv(parameter.location, 4, values49);
                break;
            }
            case 50: {
                values50[0] = 1.0f / materialFloat(0x290);
                values50[1] = materialFloat(0x288);
                values50[2] = materialFloat(0x28c);
                values50[3] = materialFloat(0x294);
                four_values = values50;
                goto upload_four;
            }
            case 51: {
                values51[0] = 0.1f * materialFloat(0x274);
                values51[1] = materialFloat(0x27c);
                values51[2] = materialFloat(0x280);
                values51[3] = materialFloat(0x278);
                four_values = values51;
                goto upload_four;
            }
            case 52: {
                // Original .L35 at 0x30a8d8.  GLES has no fixed-function
                // alpha test, so generated shaders consume the current
                // render-state comparison as (sign, adjusted reference).
                const f32 alpha_ref = static_cast<f32>(g_alphaRef) * (1.0f / 255.0f);
                if (g_alphaTestEnabled == 0) {
                    values52[0] = 0.0f;
                    values52[1] = -1.0f;
                } else if (g_alphaFunc == 2) {
                    values52[0] = -1.0f;
                    values52[1] = static_cast<f32>(0u - g_alphaRef) * (1.0f / 255.0f) - (1.0f / 255.0f);
                    if (values52[1] <= 0.0f) {
                        values52[1] = 0.0f;
                    }
                } else if (g_alphaFunc == 3) {
                    values52[0] = -1.0f;
                    values52[1] = static_cast<f32>(0u - g_alphaRef) * (1.0f / 255.0f);
                } else if (g_alphaFunc == 5) {
                    values52[0] = 1.0f;
                    values52[1] = alpha_ref;
                } else if (g_alphaFunc == 6) {
                    values52[0] = 1.0f;
                    values52[1] = alpha_ref + (1.0f / 255.0f);
                } else {
                    values52[0] = 0.0f;
                    values52[1] = -1.0f;
                }
                glUniform2fv(parameter.location, 1, values52);
                break;
            }
        }
        goto next_semantic;
    upload_four:
        glUniform4fv(parameter.location, 1, four_values);
    next_semantic:
        active &= ~(1u << (31 - leading));
    }
}

extern "C" void NuShaderObjectGLSLSetupMaterial(NUSHADEROBJECT *shader, struct numtl_s *mtl) {

    // Target 0x30cba0 walks the active texture semantics and binds each map to
    // the unit encoded by ProbeSemantics.  Keeping this driven by the usage
    // mask is important for multi-sampler character materials.
    static numtl_s *lastMtl;
    static NUSHADEROBJECT *lastObject;
    static i32 lastFrame;
    NUSHADERUSAGEMASK dirty;
    NuShaderGetDirtyMask(&dirty, shader);
    const NUSHADERUSAGEMASK *usage = &dirty;
    {
        if (lastMtl != mtl || lastObject != shader || lastFrame != 0) {
            u32 textures = (usage->semantics[0] & 0xfffff) << 11;
            for (;;) {
                const u32 leading = ShaderCountLeadingZeros(textures);
                if (leading > 31) {
                    break;
                }
                const i32 semantic = 20 - leading;

                GLSLParameter &parameter = shader->parameters[semantic];
#define BIND_MATERIAL_2D(field)                                                                                        \
    glActiveTexture(GL_TEXTURE0 + (static_cast<u16>(parameter.location) & 0x7ff));                                     \
    g_currentTexUnit = static_cast<u16>(parameter.location) & 0x7ff;                                                   \
    glBindTexture(GL_TEXTURE_2D, (field) != 0 ? NuTexGetNative(field)->platform.gl_tex : 0)
                switch (semantic) {
                    case 0:
                        BIND_MATERIAL_2D(mtl->shader_desc.diffuse_map_tex_id[0]);
                        break;
                    case 1:
                        BIND_MATERIAL_2D(mtl->shader_desc.diffuse_map_tex_id[1]);
                        break;
                    case 2:
                        BIND_MATERIAL_2D(mtl->shader_desc.diffuse_map_tex_id[2]);
                        break;
                    case 3:
                        BIND_MATERIAL_2D(mtl->shader_desc.diffuse_map_tex_id[3]);
                        break;
                    case 4:
                        BIND_MATERIAL_2D(mtl->shader_desc.specular_map_tid);
                        break;
                    case 5:
                        BIND_MATERIAL_2D(mtl->shader_desc.lightmap_tex_id[0]);
                        break;
                    case 6:
                        BIND_MATERIAL_2D(mtl->shader_desc.normal_map_tid);
                        break;
                    case 7:
                        BIND_MATERIAL_2D(mtl->shader_desc.lightmap_tex_id[1]);
                        break;
                    case 9:
                        BIND_MATERIAL_2D(mtl->shader_desc.vtf_height_map_tid);
                        break;
                    case 12:
                        BIND_MATERIAL_2D(mtl->shader_desc.vtf_normal_map_tid);
                        break;
                    case 16:
                        BIND_MATERIAL_2D(mtl->shader_desc.shine_map_ps2_tid);
                        break;
                    case 19:
                        BIND_MATERIAL_2D(mtl->shader_desc.field_1e4);
                        break;
                    case 20:
                        BIND_MATERIAL_2D(mtl->shader_desc.field_1e8);
                        break;
                    case 18:
                        if (NuWindCurrent(nuapi.wind) >= 0) {
                            const u32 texture_unit = static_cast<u16>(parameter.location) & 0x7ff;
                            NuTexSetTextureWithStagePS(NuTexGetNative(NuWindCurrent(nuapi.wind)), texture_unit);
                        }
                        break;
                    case 13: {
                        if (g_currentTexUnit != (static_cast<u16>(parameter.location) & 0x7ff)) {
                            glActiveTexture(GL_TEXTURE0 + (static_cast<u16>(parameter.location) & 0x7ff));
                            g_currentTexUnit = static_cast<u16>(parameter.location) & 0x7ff;
                        }
#define CUBE_TEXTURE_NAME()                                                                                            \
    (mtl->shader_desc.unknown_198 != 0 ? NuTexGetNative(mtl->shader_desc.unknown_198)->platform.gl_tex : 0)
                        const GLuint previous_texture =
                            g_lastBoundCubeTexIds[static_cast<u16>(parameter.location) & 0x7ff];
                        if (previous_texture != CUBE_TEXTURE_NAME()) {
                            glBindTexture(GL_TEXTURE_CUBE_MAP, CUBE_TEXTURE_NAME());
                            const u32 texture_unit = static_cast<u16>(parameter.location) & 0x7ff;
                            g_lastBoundCubeTexIds[texture_unit] = CUBE_TEXTURE_NAME();
                        }
#undef CUBE_TEXTURE_NAME
                        break;
                    }
                    case 14: {
                        if (g_currentTexUnit != (static_cast<u16>(parameter.location) & 0x7ff)) {
                            glActiveTexture(GL_TEXTURE0 + (static_cast<u16>(parameter.location) & 0x7ff));
                            g_currentTexUnit = static_cast<u16>(parameter.location) & 0x7ff;
                        }
#define CUBE_TEXTURE_NAME()                                                                                            \
    ((mtl->shader_desc.flags & 0x50000) != 0                                                                           \
         ? g_LegoEnvTexture                                                                                            \
         : (mtl->shader_desc.envmap_cubic_tid > 0 ? NuTexGetNative(mtl->shader_desc.envmap_cubic_tid)->platform.gl_tex \
                                                  : 0))
                        const GLuint previous_texture =
                            g_lastBoundCubeTexIds[static_cast<u16>(parameter.location) & 0x7ff];
                        if (previous_texture != CUBE_TEXTURE_NAME()) {
                            glBindTexture(GL_TEXTURE_CUBE_MAP, CUBE_TEXTURE_NAME());
                            const u32 texture_unit = static_cast<u16>(parameter.location) & 0x7ff;
                            g_lastBoundCubeTexIds[texture_unit] = CUBE_TEXTURE_NAME();
                        }
#undef CUBE_TEXTURE_NAME
                        break;
                    }
                }
#undef BIND_MATERIAL_2D
                textures &= ~(1u << (31 - leading));
            }

            NuShaderObjectGLSLSetCustomSetterParameters(&shader->glsl, dirty, mtl);
            lastMtl = mtl;
            lastObject = shader;
            lastFrame = 0;
        }

        // The original continues through the non-material shader semantics
        // (0x35..0x59) and uploads the values accumulated by
        // NuShaderManagerSetfv.  These include the world/view/projection
        // matrices and the current light state.  Use the locations and setter
        // classes recorded by NuShaderObjectGLSLProbeSemantics rather than
        // looking up a hand-picked set of generated GLSL names.
        auto nextSemantic = [usage](u32 start) {
            u32 word = start >> 5;
            u32 bits = usage->semantics[word] >> (start & 31);
            while (!bits) {
                if (++word >= 3) {
                    return 128u;
                }
                start = word * 32;
                bits = usage->semantics[word];
            }
            return start + static_cast<u32>(__builtin_ctz(bits));
        };
        for (u32 semantic = nextSemantic(0x35); semantic <= 0x59; semantic = nextSemantic(semantic + 1)) {

            GLSLParameter &parameter = shader->parameters[semantic];

            const nu2api::ShaderUniformRecord &uniform = nu2api::g_shaderUniforms[semantic];
            const i32 count = static_cast<i32>(uniform.data.metadata[0]);
            const f32 *values = reinterpret_cast<const f32 *>(uniform.data.values);

            switch (parameter.type_and_flags & 0x0f) {
                case 1:
                    g_glConstantSetterTable[parameter.element_count_and_setter & 3](parameter.location, count, values);
                    break;
                case 2:
                    glUniform4fv(parameter.location, count, values);
                    break;
                case 3:
                    parameter.setElementsMatrix(0, count, values);
                    break;
            }
        }
    }
}

// Original 0x30bd60: apply sampler state to every active material texture.
extern "C" void NuShaderObjectGLSLSetupTextureStates(NUSHADEROBJECT *shader, numtl_s *mtl) {
    static const char source[] = "i:/SagaTouch-Android_9176564/nu2api.saga/shaderbuilder/android/nushaderobject.cpp";
    static const GLint wrap_modes[4] = {GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT, GL_CLAMP_TO_EDGE};
    NuCheckGLErrorsFL(source, 0x479);
    u32 active = shader->usage_mask->semantics[0] & 0xfffff;
    for (i32 semantic = 0; active; ++semantic, active >>= 1) {
        if (!(active & 1)) {
            continue;
        }
        GLSLParameter &parameter = shader->parameters[semantic];
#define BIND_STATE_TEXTURE(field)                                                                                      \
    glActiveTexture(GL_TEXTURE0 + (static_cast<u16>(parameter.location) & 0x7ff));                                     \
    g_currentTexUnit = static_cast<u16>(parameter.location) & 0x7ff;                                                   \
    glBindTexture(GL_TEXTURE_2D, (field) != 0 ? NuTexGetNative(field)->platform.gl_tex : 0)
#define REPEAT_STATE()                                                                                                 \
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);                                                      \
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
#define NEAREST_STATE()                                                                                                \
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);                                                 \
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
        switch (semantic) {
            case 0: {
                BIND_STATE_TEXTURE(mtl->shader_desc.diffuse_map_tex_id[0]);
                const u8 wraps = reinterpret_cast<const u8 *>(mtl)[0x41];
                const u32 wrap_t = (wraps >> 2) & 3;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_modes[wraps & 3]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_modes[wrap_t]);
                NuCheckGLErrorsFL(source, 0x491);
                break;
            }
            case 1: {
                BIND_STATE_TEXTURE(mtl->shader_desc.diffuse_map_tex_id[1]);
                const u8 wraps = reinterpret_cast<const u8 *>(mtl)[0x41];
                const u32 wrap_t = (wraps >> 2) & 3;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_modes[wraps & 3]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_modes[wrap_t]);
                NuCheckGLErrorsFL(source, 0x49a);
                break;
            }
            case 2: {
                BIND_STATE_TEXTURE(mtl->shader_desc.diffuse_map_tex_id[2]);
                const u8 wraps = reinterpret_cast<const u8 *>(mtl)[0x41];
                const u32 wrap_t = (wraps >> 2) & 3;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_modes[wraps & 3]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_modes[wrap_t]);
                break;
            }
            case 3: {
                BIND_STATE_TEXTURE(mtl->shader_desc.diffuse_map_tex_id[3]);
                const u8 wraps = reinterpret_cast<const u8 *>(mtl)[0x41];
                const u32 wrap_t = (wraps >> 2) & 3;
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_modes[wraps & 3]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_modes[wrap_t]);
                NuCheckGLErrorsFL(source, 0x4ab);
                break;
            }
            case 4:
                BIND_STATE_TEXTURE(mtl->shader_desc.specular_map_tid);
                REPEAT_STATE();
                NuCheckGLErrorsFL(source, 0x4b5);
                break;
            case 5:
                BIND_STATE_TEXTURE(mtl->shader_desc.lightmap_tex_id[0]);
                REPEAT_STATE();
                NuCheckGLErrorsFL(source, 0x4bf);
                break;
            case 6:
                BIND_STATE_TEXTURE(mtl->shader_desc.normal_map_tid);
                REPEAT_STATE();
                NuCheckGLErrorsFL(source, 0x4c9);
                break;
            case 7:
                BIND_STATE_TEXTURE(mtl->shader_desc.lightmap_tex_id[1]);
                REPEAT_STATE();
                NuCheckGLErrorsFL(source, 0x4d3);
                break;
            case 9:
                BIND_STATE_TEXTURE(mtl->shader_desc.vtf_height_map_tid);
                REPEAT_STATE();
                NEAREST_STATE();
                NuCheckGLErrorsFL(source, 0x4e6);
                break;
            case 12:
                BIND_STATE_TEXTURE(mtl->shader_desc.vtf_normal_map_tid);
                REPEAT_STATE();
                NuCheckGLErrorsFL(source, 0x4f8);
                break;
            case 13: {
                if (g_currentTexUnit != (static_cast<u16>(parameter.location) & 0x7ff)) {
                    glActiveTexture(GL_TEXTURE0 + (static_cast<u16>(parameter.location) & 0x7ff));
                    g_currentTexUnit = static_cast<u16>(parameter.location) & 0x7ff;
                }
#define STATE_CUBE_NAME()                                                                                              \
    (mtl->shader_desc.unknown_198 != 0 ? NuTexGetNative(mtl->shader_desc.unknown_198)->platform.gl_tex : 0)
                const GLuint previous = g_lastBoundCubeTexIds[static_cast<u16>(parameter.location) & 0x7ff];
                if (previous != STATE_CUBE_NAME()) {
                    glBindTexture(GL_TEXTURE_CUBE_MAP, STATE_CUBE_NAME());
                    const u32 unit = static_cast<u16>(parameter.location) & 0x7ff;
                    g_lastBoundCubeTexIds[unit] = STATE_CUBE_NAME();
                }
#undef STATE_CUBE_NAME
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                NuCheckGLErrorsFL(source, 0x502);
                break;
            }
            case 14: {
                const u32 unit = static_cast<u16>(parameter.location) & 0x7ff;
                glGetError();
                const GLuint texture = (mtl->shader_desc.flags & 0x50000) != 0
                                           ? g_LegoEnvTexture
                                           : (mtl->shader_desc.envmap_cubic_tid > 0
                                                  ? NuTexGetNative(mtl->shader_desc.envmap_cubic_tid)->platform.gl_tex
                                                  : 0);
                glGetError();
                glActiveTexture(GL_TEXTURE0 + unit);
                glGetError();
                glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
                glGetError();
                NuCheckGLErrorsFL(source, 0x51f);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                NuCheckGLErrorsFL(source, 0x522);
                break;
            }
            case 16:
                BIND_STATE_TEXTURE(mtl->shader_desc.shine_map_ps2_tid);
                NEAREST_STATE();
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                NuCheckGLErrorsFL(source, 0x52e);
                break;
            case 18:
                if (NuWindCurrent(nuapi.wind) >= 0) {
                    const u32 unit = static_cast<u16>(parameter.location) & 0x7ff;
                    NuTexSetTextureWithStagePS(NuTexGetNative(NuWindCurrent(nuapi.wind)), unit);
                    REPEAT_STATE();
                    NEAREST_STATE();
                    NuCheckGLErrorsFL(source, 0x549);
                }
                break;
            case 19:
                BIND_STATE_TEXTURE(mtl->shader_desc.field_1e4);
                REPEAT_STATE();
                NEAREST_STATE();
                NuCheckGLErrorsFL(source, 0x556);
                break;
            case 20:
                BIND_STATE_TEXTURE(mtl->shader_desc.field_1e8);
                REPEAT_STATE();
                NEAREST_STATE();
                NuCheckGLErrorsFL(source, 0x560);
                break;
        }
#undef BIND_STATE_TEXTURE
#undef REPEAT_STATE
#undef NEAREST_STATE
    }
}
