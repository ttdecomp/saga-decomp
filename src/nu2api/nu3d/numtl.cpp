#include "nu2api/nu3d/numtl.h"

#include <string.h>

#include "decomp.h"
#include "gamelib/util/gamelib_util_types.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nuvertexformat_android.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nulst.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nufile/nu2api_nufile_types.h"

// Shader manager API (transcribed in nushadermanager_plain.cpp).
extern "C" void *NuShaderManagerRetrieveShader(NUSHADERMTLDESC *desc, void *mtl);
extern "C" void *NuShaderManagerRetrieveShaderVariant(NUSHADERMTLDESC *desc, void *mtl, i32 variant);
extern "C" NUSHADEROBJECT *NuShaderManagerGetShaderById(i32 id);
extern "C" void NuShaderManagerReleaseShader(NUSHADEROBJECT *shader);

extern "C" i32 NuSpecialForceToAlpha(nuhspecial_s *special) {
    if (special->scene == NULL) {
        return 0;
    }
    NUDLDLISTSCENE *scene = special->scene->display_list;
    NUCLIPOBJECT *object = special->display_special->clip_objects;
    if (object->nmaterials <= 0) {
        return 0;
    }
    NUMTL *material = scene->mtls[object->material_ids[0]];
    if (material == NULL) {
        return 0;
    }
    i32 index = 0;
    do {
        do {
            material->attribs.alpha_mode = NUMTL_ALPHA_MODE_ALPHA;
            NuMtlUpdate(material);
            material = material->next;
        } while (material != NULL);
        ++index;
        if (object->nmaterials <= index) {
            break;
        }
        material = scene->mtls[object->material_ids[index]];
    } while (material != NULL);
    return 1;
}

static i32 max_materials;
static NUMTL *material_list;
static NULSTHDR *g_overrideList;
struct MTL_OVERRIDE_RECORD {
    NUMTL *material;
    NUMTL *original;
    u32 field_8;
};
DECOMP_ASSERT(sizeof(MTL_OVERRIDE_RECORD) == 12, "Material override payload ABI");

extern "C" void NuMtlInitOverride(i32 count, VARIPTR *buffer, VARIPTR *end) {
    g_overrideList = NuLstCreateBuff(count, sizeof(MTL_OVERRIDE_RECORD), buffer, *end, 16);
}

extern "C" void NuMtlDestroy(NUMTL *mtl) {
    mtl->is_used = false;
    NuDisplayListDestroyMtl(mtl);
    NULNKHDR *node = NuLstGetNext(g_overrideList, NULL);
    while (node != NULL) {
        MTL_OVERRIDE_RECORD *entry = reinterpret_cast<MTL_OVERRIDE_RECORD *>(node);
        node = NuLstGetNext(g_overrideList, node);
        if (entry->original == mtl) {
            entry->material->is_used = false;
            NuDisplayListDestroyMtl(entry->material);
            NuLstFree(reinterpret_cast<NULNKHDR *>(entry));
        }
    }
}
i32 numtl_renderplane;

NUMTL *numtl_defaultmtl2d;
NUMTL *numtl_defaultmtl3d;

extern "C" i32 NuMtlSetCurrentRenderPlane(i32 render_plane) {
    i32 previous_render_plane = numtl_renderplane;
    numtl_renderplane = render_plane;
    return previous_render_plane;
}

void NuMtlInitEx(VARIPTR *buf, i32 mtl_count) {
    NuMtlInitExPS(buf);
    max_materials = mtl_count;
    material_list = (NUMTL *)ALIGN(buf->addr, 0x10);
    buf->addr = (usize)material_list + mtl_count * sizeof(NUMTL);
    memset(material_list, 0, mtl_count * sizeof(NUMTL));
    numtl_defaultmtl3d = NuMtlCreate3D(1);
    numtl_defaultmtl2d = NuMtlCreate(1);

    NUSHADERMTLDESC desc2d;
    memset(&desc2d, 0, sizeof(desc2d));
    desc2d.byte4 |= 0x10;
    desc2d.flags = 0x1000;
    desc2d.diffuse_color[0] = 0xffffffff;
    desc2d.unknown_24 = 1.0f;
    desc2d.vtx_desc.has_position = 1;
    desc2d.vtx_desc.has_diffuse = 1;
    desc2d.vtx_desc.has_no_transform = 1;
    NUSHADERMTLDESC desc3d;
    memset(&desc3d, 0, sizeof(desc3d));
    desc3d.byte4 |= 0x10;
    desc3d.flags = 0x1000;
    desc3d.diffuse_color[0] = 0xffffffff;
    desc3d.unknown_24 = 1.0f;
    desc3d.vtx_desc.has_position = 1;
    desc3d.vtx_desc.has_diffuse = 1;
    NuMtlSetShaderDescPS(numtl_defaultmtl2d, &desc2d);
    NuMtlSetShaderDescPS(numtl_defaultmtl3d, &desc3d);

    char package_path[512];
    i32 package = AndroidOBBUtils::LookupPackagePath(package_path, NuFileDeviceAndroidOBBType::MAIN);
    if (package == 1) {
        NuDatSet(NuDatOpen(package_path, buf, NULL));
    } else if (package == 2) {
        if (g_apkFileDevice == NULL) {
            NuFile::InitData init = {};
            g_apkFileDevice = new NuFileDeviceAndroidAPK("apk:", init);
            NuDatSet(NuDatOpen(package_path, buf, NULL));
        }
    }
    package = AndroidOBBUtils::LookupPackagePath(package_path, NuFileDeviceAndroidOBBType::PATCH);
    if (package == 1) {
        NuDatSet(NuDatOpen(package_path, buf, NULL));
    } else if (package == 2) {
        if (g_apkFileDevice == NULL) {
            NuFile::InitData init = {};
            g_apkFileDevice = new NuFileDeviceAndroidAPK("apk:", init);
            NuDatSet(NuDatOpen(package_path, buf, NULL));
        }
    }
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/numtl_gen.c", 0xd5);
    NuTexInitExPS(buf);
    numtl_defaultmtl2d->attribs.alpha_mode = 1;
    NuMtlUpdate(numtl_defaultmtl2d);
    NuMtlUpdate(numtl_defaultmtl3d);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/numtl_gen.c", 0xee);
}

void DefaultMtl(NUMTL *mtl) {
    // The attribs bytes (0x40-0x44 of NUMTL; attribs lives at 0x40 as a flat
    // 8-byte bitfield word) are set with direct byte/word ops in the original,
    // and diffuse r/g/b (0x54/0x58/0x5c) + opacity (0x70) default to 1.0f.
    f32 one = 1.0f;
    u8 *b = (u8 *)&mtl->attribs;

    u8 v41 = b[1];
    v41 &= 0x0f;
    b[4] |= 0x10;
    u8 v40 = b[0];
    v40 &= 0xc0;
    v41 |= 0x20;
    v40 |= 0x10;
    b[1] = v41;
    u8 v42 = b[2];
    v42 &= 0x88;
    b[0] = v40;
    v42 |= 0x16;
    b[2] = v42;

    mtl->diffuse_color.r = one;
    mtl->diffuse_color.g = one;
    mtl->diffuse_color.b = one;
    mtl->opacity = one;

    u16 w42 = *(u16 *)&b[2];
    w42 &= 0x807f;
    w42 |= 0x3f80;
    *(u16 *)&b[2] = w42;
}

void NuShaderMtlDescInit(NUSHADERMTLDESC *desc) {
    i32 i;

    if (desc == NULL) {
        return;
    }

    memset(desc, 0, sizeof(NUSHADERMTLDESC));

    desc->unknown_24 = 1.0f;
    desc->byte4 |= 0x10;

    desc->flags = 0x1000;

    desc->diffuse_color[0] = RGBA_TO_NUCOLOUR32(0xff, 0x80, 0x80, 0xff);

    for (i = 0; i < 4; i++) {
        desc->tex_anim_data[i] = -1;
    }

    desc->vtx_desc.has_position = 1;
    desc->vtx_desc.has_diffuse = 1;
}

NUMTL *NuMtlCreate(i32 count) {
    i32 i;
    i32 j;
    NUMTL *mtl;
    NUMTL *next;

    next = NULL;

    for (i = 0; i < count; i++) {
        mtl = NULL;

        for (j = 0; j < max_materials; j++) {
            if (!material_list[j].is_used && material_list[j].display_list == NULL) {
                mtl = &material_list[j];
                break;
            }
        }

        memset(mtl, 0, sizeof(NUMTL));

        DefaultMtl(mtl);

        mtl->is_used = true;
        mtl->unknown_0_4 = true;
        mtl->renderplane = numtl_renderplane;
        mtl->attribs.unknown_6_128 = true;

        mtl->next = next;
        next = mtl;
    }

    NuMtlCreatePS(mtl, 0);

    return mtl;
}

// original 0x2f24a0 — same allocation sweep as NuMtlCreate, but WITHOUT
// the attribs byte6 bit7 set (asm has no `orb $0x80,0x46` here). The material
// is queued for dynamic display-list creation and the platform pass runs with
// is_3d=1 (no has_no_transform bit). Byte6 clear keeps NuMtlUpdatePS on the
// RetrieveShaderVariant path instead of the vtx_desc bit2 poke path.
extern "C" NUMTL *NuMtlCreate3D(i32 count) {
    i32 i;
    i32 j;
    NUMTL *mtl;
    NUMTL *next;

    next = NULL;

    for (i = 0; i < count; i++) {
        mtl = NULL;

        for (j = 0; j < max_materials; j++) {
            if (!material_list[j].is_used && material_list[j].display_list == NULL) {
                mtl = &material_list[j];
                break;
            }
        }

        memset(mtl, 0, sizeof(NUMTL));

        DefaultMtl(mtl);

        mtl->is_used = true;
        mtl->unknown_0_4 = true;
        mtl->renderplane = numtl_renderplane;
        // NB: no unknown_6_128 here — original Create3D leaves byte6 clear.

        mtl->next = next;
        next = mtl;
    }

    NuDisplayListCreateMtl(mtl);

    NuMtlCreatePS(mtl, 1);

    return mtl;
}

extern "C" NUMTL *NuMtlCreateEx(i32 count, u8 render_plane) {
    NUMTL *mtl;
    NUMTL *next = NULL;
    for (i32 i = 0; i < count; ++i) {
        mtl = NULL;
        for (i32 j = 0; j < max_materials; ++j) {
            if (!material_list[j].is_used && material_list[j].display_list == NULL) {
                mtl = &material_list[j];
                break;
            }
        }
        memset(mtl, 0, sizeof(NUMTL));
        DefaultMtl(mtl);
        mtl->is_used = true;
        mtl->unknown_0_4 = true;
        mtl->renderplane = render_plane;
        mtl->attribs.unknown_6_128 = true;
        mtl->next = next;
        next = mtl;
    }
    NuMtlCreatePS(mtl, 0);
    return mtl;
}

extern "C" NUMTL *NuMtlCreateEx3D(i32 count, u8 render_plane) {
    NUMTL *mtl;
    NUMTL *next = NULL;
    for (i32 i = 0; i < count; ++i) {
        mtl = NULL;
        for (i32 j = 0; j < max_materials; ++j) {
            if (!material_list[j].is_used && material_list[j].display_list == NULL) {
                mtl = &material_list[j];
                break;
            }
        }
        memset(mtl, 0, sizeof(NUMTL));
        DefaultMtl(mtl);
        mtl->is_used = true;
        mtl->unknown_0_4 = true;
        mtl->renderplane = render_plane;
        NuDisplayListCreateMtl(mtl);
        mtl->next = next;
        next = mtl;
    }
    NuMtlCreatePS(mtl, 1);
    return mtl;
}

void NuMtlUpdate(NUMTL *mtl) {
    NuMtlUpdatePS(mtl);
    mtl->version++;
}

void NuMtlAddEx(numtl_s *, i32) {
}

// original 0x29bc50 — refresh the material's shader desc, (re)acquire its
// shader objects and rebuild the platform vertex declaration.
void NuMtlUpdatePS(numtl_s *mtl) {
    if (0 < mtl->tex_id) {
        mtl->shader_desc.diffuse_map_tex_id[0] = mtl->tex_id;

        i32 count = mtl->shader_desc.unknown_a8;
        if (count == 0) {
            count = 1;
        }
        mtl->shader_desc.unknown_a8 = count;

        // Vtx-desc tex-unit nibble: at least one unit.
        u8 b = ((u8 *)&mtl->shader_desc.vtx_desc)[1];
        u8 units = (b >> 3) & 7;
        if (units == 0) {
            units = 1;
        }
        ((u8 *)&mtl->shader_desc.vtx_desc)[1] = (b & 199) | (units << 3);
    }

    if ((((u8 *)&mtl->shader_desc.vtx_desc)[3] & 4) != 0) {
        mtl->shader_desc.flagsbits_1bb |= 0x80;
        mtl->shader_desc.flags |= 0x200000;
    }

    if (0 < mtl->shader_desc.shader_id) {
        NuShaderManagerReleaseShader(NuShaderManagerGetShaderById(mtl->shader_desc.shader_id));
    }
    if (0 < mtl->shader_desc.shader_variant_id) {
        NuShaderManagerReleaseShader(NuShaderManagerGetShaderById(mtl->shader_desc.shader_variant_id));
    }

    if (((char *)&mtl->attribs)[6] < 0 && mtl->display_list == NULL) {
        if (mtl->shader_desc.blend_op2 != 0xff) {
            ((u8 *)&mtl->shader_desc.vtx_desc)[2] |= 4;
            void *shader = NuShaderManagerRetrieveShader(&mtl->shader_desc, mtl);
            if (shader != NULL) {
                mtl->shader_desc.shader_id = *(i16 *)shader;
            }
        }
    } else if (mtl->shader_desc.blend_op2 != 0xff) {
        void *shader = NuShaderManagerRetrieveShaderVariant(&mtl->shader_desc, mtl, 0x10);
        if (shader != NULL) {
            mtl->shader_desc.shader_variant_id = -1;
            mtl->shader_desc.shader_id = *(i16 *)shader;
        }
    }

    mtl->vertex_decl = NuGetVertexDeclaration(mtl->shader_desc.vtx_desc);

    if (mtl->tex_id == 0) {
        i32 packed = ((i32)(mtl->diffuse_color.r * 255.0f) & 0xff) | 0xff000000 |
                     (((i32)(mtl->diffuse_color.g * 255.0f) & 0xff) << 8) |
                     (((i32)(mtl->diffuse_color.b * 255.0f) & 0xff) << 16);
        mtl->shader_desc.diffuse_color[0] = (NUCOLOUR32)packed;
    }
}

void NuMtlSetUVOffsetPS(numtl_s *mtl, u32 layer, float u, float v) {
    mtl->shader_desc.tex_anim_offsets[layer][0] = u;
    mtl->shader_desc.tex_anim_offsets[layer][1] = v;
}

void NuMtlDisableCulling() {
}
