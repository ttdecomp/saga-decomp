#include "nu2api/nu3d/nugscn.h"

#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nufile/nufile.h"

#include <string.h>

SAGA_HOST_WEAK void NuGScnCreatePS(nugscn_s *scene, variptr_u *, variptr_u *) {
    NUNATIVETEX **textures = scene->textures;
    if (g_VideoResHeader.texture_hashes == 0) {
        for (i32 i = 0; i < scene->ntextures; ++i) {
            NUNATIVETEX *texture = textures[i];
            texture->image_data = nullptr;
            texture->size = 0;
            texture->platform.gl_tex = g_VideoResHeader.textures[i];
        }
    } else {
        for (u32 i = 0; i < g_VideoResHeader.ntextures; ++i) {
            const u32 hash = g_VideoResHeader.textures[i];
            if (hash != 0) {
                g_VideoResHeader.textures[i] = NuIOS_CreateGLTexFromHash(hash);
            }
            NUNATIVETEX *texture = textures[i];
            texture->image_data = nullptr;
            texture->size = hash;
            texture->platform.gl_tex = g_VideoResHeader.textures[i];
        }
    }
}

i32 NuGScnFixupTID(nugscn_s *scene, i32 tid) {
    if (tid == -1) {
        return 0;
    }
    if ((tid & 0x4000) == 0) {
        return scene->texture_ids[tid];
    }

    i32 resolved_tid = NuTexResolveReference(scene, tid);
    if (resolved_tid != 0) {
        return resolved_tid;
    }

    extern i32 NuTexGetUnresolvedTextureTIDPS();
    resolved_tid = NuTexGetUnresolvedTextureTIDPS();
    NuTexAddReference(resolved_tid, scene);
    return resolved_tid;
}

void NuGScnDestroyPS(nugscn_s *) {
}

extern "C" void NuTexAnimRemoveList(void *texture_anims);

extern "C" void NuGScnRemove(nugscn_s *scene) {
    if (scene->texture_anims != nullptr) {
        NuTexAnimRemoveList(scene->texture_anims);
    }

    NuGScnRestoreTIDs(scene);
    for (i32 i = 0; i < scene->ntextures; ++i) {
        if (scene->textures[i]->ref_count >= 0) {
            NuTexDestroy(scene->texture_ids[i]);
        }
    }

    if (scene->additional_scenes != nullptr) {
        for (NUGSCN **additional = scene->additional_scenes; *additional != nullptr; ++additional) {
            NuDisplaySceneDestroy((*additional)->display_list);
        }
    }
    NuDisplaySceneDestroy(scene->display_list);
    NuGScnDestroyPS(scene);
}

void NuGScnFixupTIDs(nugscn_s *scene) {
    for (i32 i = 0; i < scene->nummtl; ++i) {
        NUMTL *material = scene->mtls[i];
        material->tex_id = (i16)NuGScnFixupTID(scene, material->tex_id);

        NUSHADERMTLDESC &shader = material->shader_desc;
        shader.specular_map_tid = NuGScnFixupTID(scene, shader.specular_map_tid);
        shader.lightmap_tex_id[0] = NuGScnFixupTID(scene, shader.lightmap_tex_id[0]);
        shader.normal_map_tid = NuGScnFixupTID(scene, shader.normal_map_tid);
        shader.lightmap_tex_id[1] = NuGScnFixupTID(scene, shader.lightmap_tex_id[1]);
        shader.envmap_cubic_tid = NuGScnFixupTID(scene, shader.envmap_cubic_tid);
        shader.unknown_198 = NuGScnFixupTID(scene, shader.unknown_198);
        shader.shine_map_ps2_tid = NuGScnFixupTID(scene, shader.shine_map_ps2_tid);
        for (i32 layer = 0; layer < 4; ++layer) {
            shader.diffuse_map_tex_id[layer] = NuGScnFixupTID(scene, shader.diffuse_map_tex_id[layer]);
        }
        shader.vtf_height_map_tid = NuGScnFixupTID(scene, shader.vtf_height_map_tid);
        shader.vtf_normal_map_tid = NuGScnFixupTID(scene, shader.vtf_normal_map_tid);
        shader.field_1e4 = NuGScnFixupTID(scene, shader.field_1e4);
        shader.field_1e8 = NuGScnFixupTID(scene, shader.field_1e8);

        NUMTL *updated_material = scene->mtls[i];
        reinterpret_cast<u8 *>(updated_material)[0x46] &= 0xbf;
        NuMtlUpdate(updated_material);
    }

    NuGScnFixupTIDsPS(scene);
}

i32 NuGScnRestoreTID(nugscn_s *scene, i32 tid) {
    if (tid == 0) {
        return -1;
    }

    extern i32 NuTexGetUnresolvedTextureTIDPS();
    if (tid == NuTexGetUnresolvedTextureTIDPS()) {
        NuTexRemoveReference(tid, scene);
        return -1;
    }

    NUNATIVETEX *texture = NuTexGetNative(tid);
    for (i32 i = 0; i < scene->ntextures; ++i) {
        NUNATIVETEX *scene_texture = scene->textures[i];
        bool checksum_matches = true;
        for (i32 byte = 0; byte < 16; ++byte) {
            if (scene_texture->checksum[byte] != texture->checksum[byte]) {
                checksum_matches = false;
                break;
            }
        }
        if (checksum_matches && scene->texture_ids[i] == tid) {
            if (scene_texture->ref_count < 0) {
                NuTexRemoveReference(tid, scene);
                return i | 0x4000;
            }
            return i;
        }
    }

    return -1;
}

void NuGScnRestoreTIDs(nugscn_s *scene) {
    for (i32 i = 0; i < scene->nummtl; ++i) {
        if (scene->ntextures == 0) {
            continue;
        }

        NUMTL *material = scene->mtls[i];
        material->tex_id = (i16)NuGScnRestoreTID(scene, material->tex_id);

        NUSHADERMTLDESC &shader = material->shader_desc;
        shader.specular_map_tid = NuGScnRestoreTID(scene, shader.specular_map_tid);
        shader.lightmap_tex_id[0] = NuGScnRestoreTID(scene, shader.lightmap_tex_id[0]);
        shader.normal_map_tid = NuGScnRestoreTID(scene, shader.normal_map_tid);
        shader.lightmap_tex_id[1] = NuGScnRestoreTID(scene, shader.lightmap_tex_id[1]);
        shader.envmap_cubic_tid = NuGScnRestoreTID(scene, shader.envmap_cubic_tid);
        shader.unknown_198 = NuGScnRestoreTID(scene, shader.unknown_198);
        shader.shine_map_ps2_tid = NuGScnRestoreTID(scene, shader.shine_map_ps2_tid);
        for (i32 layer = 0; layer < 4; ++layer) {
            shader.diffuse_map_tex_id[layer] = NuGScnRestoreTID(scene, shader.diffuse_map_tex_id[layer]);
        }
        shader.vtf_height_map_tid = NuGScnRestoreTID(scene, shader.vtf_height_map_tid);
        shader.vtf_normal_map_tid = NuGScnRestoreTID(scene, shader.vtf_normal_map_tid);
        shader.field_1e4 = NuGScnRestoreTID(scene, shader.field_1e4);
        shader.field_1e8 = NuGScnRestoreTID(scene, shader.field_1e8);
    }

    NuGScnRestoreTIDsPS(scene);
}

void NuGScnMtlLayerMask(nugscn_s *scene, unsigned char mask) {
    NUDLDLISTSCENE *display_list = scene->display_list;
    display_list->material_layer_mask = mask;
}

SAGA_HOST_WEAK i32 NuGScnReadTexturesPS(i32 file, variptr_u *buf, variptr_u buf_end) {
    (void)buf_end;
    i32 bytes_read = 0;
    bytes_read += NuFileRead(file, &g_VideoResHeader.ntextures, sizeof(g_VideoResHeader.ntextures));

    g_VideoResHeader.texture_hashes = g_VideoResHeader.ntextures & 0x8000;
    g_VideoResHeader.ntextures &= 0x7fff;
    g_VideoResHeader.textures = buf->u32_ptr;
    buf->u32_ptr += g_VideoResHeader.ntextures;
    memset(g_VideoResHeader.textures, 0, (usize)g_VideoResHeader.ntextures * sizeof(u32));

    if (g_VideoResHeader.texture_hashes != 0) {
        bytes_read += NuFileRead(file, g_VideoResHeader.textures, (i32)g_VideoResHeader.ntextures * (i32)sizeof(u32));
        return bytes_read;
    }

    for (u32 i = 0; i < g_VideoResHeader.ntextures; ++i) {
        i32 texture_header[6];
        bytes_read += NuFileRead(file, texture_header, sizeof(texture_header));
        u32 size = (u32)texture_header[5];
        if (size == 0) {
            g_VideoResHeader.textures[i] = 0;
            if (texture_header[0] < 0) {
                for (i32 j = 1; j < 6; ++j) {
                    g_VideoResHeader.textures[i + j] = 0;
                }
                i += 5;
            }
            continue;
        }
        if (texture_header[0] == 0) {
            g_VideoResHeader.textures[i] = 0;
            continue;
        }

        NUNATIVETEX texture = {};
        texture.image_data = buf->void_ptr;
        texture.size = size;
        buf->addr += size;
        bytes_read += NuFileRead(file, texture.image_data, size);

        NudxFw_D3DBeginCriticalSection();
        bool is_pvrtc = texture_header[0] < 0;
        NuTexCreatePS(&texture, is_pvrtc);
        g_VideoResHeader.textures[i] = texture.platform.gl_tex;
        NudxFw_D3DEndCriticalSection();
        buf->addr -= size;

        if (is_pvrtc) {
            for (i32 j = 1; j < 6; ++j) {
                g_VideoResHeader.textures[i + j] = 0;
            }
            i += 5;
        }
    }
    return bytes_read;
}
