#include "nu2api/nu3d/glutils.h"

#include "nu2api/nucore/nuvuvec.hpp"
#include "nu2api/nucore/numemory.h"

NUMTL *CreateSubtractiveTexture(char *name) {
    NUTEXBITMAP *texture = NuTexReadBitmap(name);
    NUMTL *material = NuMtlCreate(1);
    material->diffuse_color.r = 1.0f;
    material->diffuse_color.g = 1.0f;
    material->diffuse_color.b = 1.0f;
    material->attribs.cull_mode = 2;
    material->attribs.z_mode = 1;
    material->opacity = 0.999f;
    material->attribs.alpha_mode = 3;
    if (texture != NULL) {
        material->tex_id = static_cast<i16>(NuTexCreate(&texture->texture));
        NU_FREE(texture->pixels);
        NU_FREE(texture);
    }
    NuMtlUpdate(material);
    return material;
}

NUMTL *CreateAlphaBlendTexture(VARIPTR *buffer, VARIPTR buffer_end, char *name, i32 disable_depth_write, i32 alpha_mode,
                               i32 sort_priority, i32 depth_mode) {
    const i32 texture_id = NuTexRead(name, buffer, &buffer_end);
    buffer->addr = ALIGN(buffer->addr, 0x10);
    NUMTL *material = NuMtlCreate3D(1);
    material->diffuse_color.r = 1.0f;
    material->diffuse_color.g = 1.0f;
    material->diffuse_color.b = 1.0f;
    material->attribs.cull_mode = 2;
    material->attribs.z_mode = static_cast<u32>(depth_mode) & 3;
    material->attribs.unknown_2_1_2 = 2;
    material->opacity = 0.999f;
    material->attribs.alpha_mode = static_cast<u32>(alpha_mode) & 0xf;
    material->attribs.unknown_4_8 = disable_depth_write != 0;
    material->tex_id = static_cast<i16>(texture_id);
    material->sort_pri = static_cast<i16>(sort_priority);
    NuMtlUpdate(material);
    return material;
}
