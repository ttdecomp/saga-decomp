#pragma once

#include <GLES2/gl2.h>

#include "decomp.h"
#include "nu2api/nucore/common.h"

struct nushaderobjectkey_s;

typedef struct nushaderobjectkey_s NUSHADEROBJECTKEY;

struct nushaderobjectbase_s {
    i32 field0;
    i32 field1;
    u32 key;
    i32 field3;
};

typedef struct nushaderobjectbase_s NUSHADEROBJECTBASE;

struct nushaderobjectglsl_s {
    NUSHADEROBJECTBASE base;
    GLuint program;
    GLuint vertex_shader;
    GLuint fragment_shader;
};

typedef struct nushaderobjectglsl_s NUSHADEROBJECTGLSL;

// original 0x30b050 — BaseInit + stash shaders + combine/link program.
void NuShaderObjectInitGLSL(nushaderobjectglsl_s *obj, nushaderobjectkey_s const *key, i32 param, u32 vshader,
                            u32 pshader);

struct GLSLParameter {
    i16 location;
    u8 element_count_and_setter;
    u8 array_size;
    u8 semantic;
    u8 type_and_flags;
    u8 reserved[2];

#ifdef __cplusplus
    void setElementsMatrix(i32 first_element, i32 count, const f32 *values) __attribute__((weak));
#endif
};

typedef struct NuShaderUsageMask_s {
    u32 semantics[4];
} NUSHADERUSAGEMASK;

DECOMP_ASSERT(sizeof(GLSLParameter) == 8, "GLSL parameter metadata size");
DECOMP_ASSERT(offsetof(GLSLParameter, semantic) == 4, "GLSL parameter semantic offset");

#define NUSHADEROBJECT_PARAMETERS_COUNT 91

struct nushaderobject_s {
    NUSHADEROBJECTGLSL glsl;
    NUSHADERUSAGEMASK *usage_mask;
    i32 last_uniform_frame;   // 0x20
    void *last_light_packet;  // 0x24
    void *last_camera_packet; // 0x28
    GLSLParameter parameters[NUSHADEROBJECT_PARAMETERS_COUNT];
    // Present in both serialized objects and the original manager slot stride.
    u8 unknown_0x304[4];
};
DECOMP_ASSERT(offsetof(nushaderobject_s, parameters) == 0x2c, "Shader parameter table offset");
DECOMP_ASSERT(offsetof(nushaderobject_s, unknown_0x304) == 0x304, "Shader object tail offset");
DECOMP_ASSERT(sizeof(nushaderobject_s) == 0x308, "Shader object size");

typedef nushaderobject_s NUSHADEROBJECT;

struct ShaderPacketStateMapping {
    NUSHADERUSAGEMASK mask;
    void **packet;
};
DECOMP_ASSERT(sizeof(ShaderPacketStateMapping) == 0x14, "Shader packet-state mapping ABI");
DECOMP_ASSERT(offsetof(nushaderobject_s, last_uniform_frame) == 0x20, "Shader frame offset");
DECOMP_ASSERT(offsetof(nushaderobject_s, last_light_packet) == 0x24, "Shader light packet offset");
DECOMP_ASSERT(offsetof(nushaderobject_s, last_camera_packet) == 0x28, "Shader camera packet offset");
extern "C" {
    extern void *g_boundLightPacket;
    extern void *g_boundCameraPacket;
    extern ShaderPacketStateMapping g_packetToShaderStateMappings[2];
    void NuShaderGetDirtyMask(NUSHADERUSAGEMASK *mask, NUSHADEROBJECT *shader);
}

struct nushaderprogramparameter_s {
    u16 register_index;
    union {
        u16 location_and_setter;
        struct {
            u16 location : 12;
            u16 setter : 4;
        };
    };
};
DECOMP_ASSERT(sizeof(nushaderprogramparameter_s) == 4, "Shader program parameter ABI");

typedef nushaderprogramparameter_s NUSHADERPROGRAMPARAMETER;

struct nushaderprogram_s {
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    i32 parameter_count;
    nushaderprogramparameter_s *parameters;
    u32 unused[3];
};

typedef nushaderprogram_s NUSHADERPROGRAM;

#ifdef __cplusplus
i32 NuShaderObjectBindAttributeLocationsGLSL(GLuint program);
bool NuShaderObjectCombineGLSLShadersIntoProgram(GLuint *program_dest, GLuint vertex_shader, GLuint fragment_shader);
bool NuShaderObjectGenerateGLSLShader(GLuint *shader_dest, GLenum shader_type, const GLchar *shader_source,
                                      GLint shader_source_length);

extern "C" {
#endif
    void NuShaderObjectBaseCreate(NUSHADEROBJECTBASE *shader);
    void NuShaderObjectGLSLCreate(NUSHADEROBJECTGLSL *shader);
    void NuShaderObjectGLSLProbeSemantics(NUSHADEROBJECT *shader);
    void NuShaderObjectCreate(NUSHADEROBJECT *shader);
    void NuShaderObjectBaseDestroy(NUSHADEROBJECTBASE *shader);
    void NuShaderObjectGLSLDestroy(NUSHADEROBJECTGLSL *shader);
    void NuShaderObjectDestroy(NUSHADEROBJECT *shader);
    NUSHADEROBJECT *NuShaderObjectUnserialize(VARIPTR *buffer);
    void NuShaderObjectBaseInit(NUSHADEROBJECTBASE *shader, NUSHADEROBJECTKEY *key, i32 unk);
    void NuShaderObjectUnInit(NUSHADEROBJECT *shader);
    void NuShaderObjectBaseUnInit(NUSHADEROBJECTBASE *shader);
    void NuShaderObjectBaseSetWaterSpeed(f32 speed);
    void NuShaderObjectSetElementsfv(NUSHADEROBJECT *shader_object, i32 semantic, i32 first_element, i32 count,
                                     const f32 *values);
    void NuShaderObjectSetElementsfv_transpose(NUSHADEROBJECT *shader_object, i32 semantic, i32 first_element,
                                               i32 count, const f32 *values);
    NUSHADERPROGRAM *NuShaderProgramCreateIOS(const char *vertex_source, const char *fragment_source);
    void NuShaderProgramSetVertexParamfv(NUSHADERPROGRAM *program, u32 register_index, const f32 *values,
                                         i32 component_count);
    void NuShaderProgramSetFragmentParamfv(NUSHADERPROGRAM *program, u32 register_index, const f32 *values,
                                           i32 component_count);
#ifdef __cplusplus
}
#endif

namespace nu2api {
    union ShaderUniformRecord {
        struct {
            const char *vertex_name;
            const char *fragment_name;
            u32 metadata[5];
            u32 values[16];
        } data;
        struct {
            const char *vertex_name;
            const char *fragment_name;
            i32 value_count;
            i32 field_c;
            i32 stage_mask;
            i32 register_index;
            i32 field_18;
            f32 values[16];
        };
        u8 raw[0x5c];
    };
    extern "C" ShaderUniformRecord g_shaderUniforms[101];
} // namespace nu2api

using nu2api::g_shaderUniforms;
extern "C" nu2api::ShaderUniformRecord *NuShaderUniformGetByString(const char *name);
using nu2api::ShaderUniformRecord;

DECOMP_ASSERT(sizeof(nu2api::ShaderUniformRecord) == 0x5c, "Shader uniform record size");
DECOMP_ASSERT(offsetof(nu2api::ShaderUniformRecord, data.metadata) == 8, "Shader uniform metadata offset");
DECOMP_ASSERT(offsetof(nu2api::ShaderUniformRecord, data.values) == 0x1c, "Shader uniform values offset");

struct numtl_s;
extern "C" void NuShaderObjectBaseUpdateWaterTable(NUSHADEROBJECT *shader, numtl_s *mtl);

extern "C" void NuShaderObjectGLSLSetupTextureStates(NUSHADEROBJECT *shader, numtl_s *mtl);
