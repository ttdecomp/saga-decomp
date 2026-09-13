#include "nu2api/nu3d/android/nugscn_android.h"

#include <GLES2/gl2.h>
#include <string.h>

#include "legoapi/characters/core/character.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/numath/numath.h"

i32 g_vaoLifetimeMutex = -1;
u32 g_lastBoundVAO;

static void NuIOSBindVAO(u32 vao_handle) {
    if (vao_handle != g_lastBoundVAO) {
        g_lastBoundVAO = vao_handle;
    }
}

void *NuGScnBufferAllocAligned(i32, i32) {
    return NULL;
}

extern "C" void NuGScnRndr3(NUGSCN *scene) {
    NuDisplaySceneRndr(scene->display_list);
}

void NuGScnLoadShadersPS(char *, variptr_u *, variptr_u) {
}

static u32 UploadDataToGLBuffer(NUFILE file, u32 size, GLenum target, usize *buffer_handle, VARIPTR *buf,
                                VARIPTR buf_end) {
    GLuint gl_buf = 0;
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nugscn_android.c", 0x56);
    glGenBuffers(1, &gl_buf);
    *buffer_handle = gl_buf;
    NuIOSBindVAO(0);
    glBindBuffer(target, gl_buf);
    glBufferData(target, size, 0, GL_STATIC_DRAW);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nugscn_android.c", 0x5d);

    if (bgProcIsBgThread()) {
        NuIOS_YieldThread();
    }

    u32 chunk_limit = g_loadingCharacterInHub != 0 ? 0x4000 : 0x10000;
    u32 buf_size = buf_end.char_ptr - buf->char_ptr;
    u32 max_chunk_size = NuMin(chunk_limit, buf_size);
    u32 largest_chunk = 0;

    for (u32 n = 0, chunk_size = 0; n < size; n += chunk_size) {
        chunk_size = NuMin(max_chunk_size, size - n);
        NuFileRead(file, buf->void_ptr, chunk_size);

        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nugscn_android.c", 0x73);
        NuIOSBindVAO(0);
        glBindBuffer(target, gl_buf);
        if (chunk_size == size) {
            glBufferData(target, chunk_size, buf->void_ptr, GL_STATIC_DRAW);
        } else {
            glBufferSubData(target, n, chunk_size, buf->void_ptr);
        }
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nugscn_android.c", 0x80);

        if (bgProcIsBgThread()) {
            NuIOS_YieldThread();
        }
        largest_chunk = NuMax(largest_chunk, chunk_size);
    }
    return largest_chunk;
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

i32 NuGScnUploadGfxDataFromFilePS(VARIPTR *buf, VARIPTR buf_end, i32 file) {
    VARIPTR max_buf = *buf;
    i32 section_size = 0;
    i32 bytes_read = 0;

    memset(&g_VideoResHeader, 0, sizeof(g_VideoResHeader));
    bytes_read += NuFileRead(file, &section_size, sizeof(section_size));
    bytes_read += NuGScnReadTexturesPS(file, buf, buf_end);

    bytes_read += NuFileRead(file, &g_VideoResHeader.nvertex_buffers, sizeof(g_VideoResHeader.nvertex_buffers));
    g_VideoResHeader.vertex_buffers = BUFFER_ALLOC_ARRAY(buf, g_VideoResHeader.nvertex_buffers, usize);
    for (u32 i = 0; i < g_VideoResHeader.nvertex_buffers; ++i) {
        u32 size = 0;
        bytes_read += NuFileRead(file, &size, sizeof(size));
        if (size == 0) {
            g_VideoResHeader.vertex_buffers[i] = 0;
            continue;
        }

        u32 keep_in_memory = size & 0x80000000;
        size &= 0x7fffffff;
        if (keep_in_memory != 0) {
            g_VideoResHeader.vertex_buffers[i] = buf->addr;
            buf->addr += size;
            bytes_read += NuFileRead(file, reinterpret_cast<void *>(g_VideoResHeader.vertex_buffers[i]), size);
        } else {
            u32 largest =
                UploadDataToGLBuffer(file, size, GL_ARRAY_BUFFER, &g_VideoResHeader.vertex_buffers[i], buf, buf_end);
            bytes_read += size;
            max_buf.addr = NuMax(max_buf.addr, buf->addr + largest);
        }
    }

    bytes_read += NuFileRead(file, &g_VideoResHeader.nindex_buffers, sizeof(g_VideoResHeader.nindex_buffers));
    g_VideoResHeader.index_buffers = BUFFER_ALLOC_ARRAY(buf, g_VideoResHeader.nindex_buffers, usize);
    for (u32 i = 0; i < g_VideoResHeader.nindex_buffers; ++i) {
        u32 size = 0;
        bytes_read += NuFileRead(file, &size, sizeof(size));
        if (size == 0) {
            g_VideoResHeader.index_buffers[i] = 0;
            continue;
        }
        u32 largest =
            UploadDataToGLBuffer(file, size, GL_ELEMENT_ARRAY_BUFFER, &g_VideoResHeader.index_buffers[i], buf, buf_end);
        bytes_read += size;
        max_buf.addr = NuMax(max_buf.addr, buf->addr + largest);
    }

    i32 total_size = section_size + 4;
    u8 padding;
    while (bytes_read < total_size) {
        bytes_read += NuFileRead(file, &padding, 1);
    }
    if (buf->addr < max_buf.addr) {
        memset(buf->void_ptr, 0, max_buf.addr - buf->addr);
    }
    return total_size;
}

extern "C" void NuGScnFixupTIDsPS(NUGSCN *scene) {
    if (scene->display_list == NULL) {
        return;
    }

    for (i32 i = 0; i < scene->display_list->nitems; ++i) {
        NUDISPLAYLISTITEM *item = &scene->display_list->items[i];
        if (item->type == 0xb0) {
            i32 *packet = static_cast<i32 *>(item->next);
            if (packet[0] == 2) {
                packet[0] = 1;
                for (i32 texture = 0; texture < 3; ++texture) {
                    packet[texture + 2] = NuGScnFixupTID(scene, packet[texture + 2]);
                }
            }
            packet[1] = NuGScnFixupTID(scene, packet[1]);
        } else if (item->type == 0xae || item->type == 0xaf) {
            i32 *packet = static_cast<i32 *>(item->next);
            if (packet != NULL) {
                for (i32 texture = 0; texture < 3; ++texture) {
                    packet[texture] = NuGScnFixupTID(scene, packet[texture]);
                }
            }
        }
    }
}

extern "C" void NuGScnRestoreTIDsPS(NUGSCN *scene) {
    if (scene->display_list == NULL) {
        return;
    }

    for (i32 i = 0; i < scene->display_list->nitems; ++i) {
        NUDISPLAYLISTITEM *item = &scene->display_list->items[i];
        if (item->type == 0xb0) {
            i32 *packet = static_cast<i32 *>(item->next);
            if (packet[0] == 2) {
                for (i32 texture = 0; texture < 3; ++texture) {
                    packet[texture + 2] = NuGScnRestoreTID(scene, packet[texture + 2]);
                }
            }
            packet[1] = NuGScnRestoreTID(scene, packet[1]);
        } else if (item->type == 0xae || item->type == 0xaf) {
            i32 *packet = static_cast<i32 *>(item->next);
            if (packet != NULL) {
                for (i32 texture = 0; texture < 3; ++texture) {
                    packet[texture] = NuGScnRestoreTID(scene, packet[texture]);
                }
            }
        }
    }
}

static void PreWarmGeomsAndBakeVAOs(nudisplayscene_s *scene, nunativegscene_s *) {
    for (i32 clip_index = 0; clip_index < scene->nclip_objects; ++clip_index) {
        u8 *clip = reinterpret_cast<u8 *>(&scene->clip_objects[clip_index]);
        u32 nitems = *reinterpret_cast<u32 *>(clip);
        u32 *materials = *reinterpret_cast<u32 **>(clip + 4);
        i32 *items = *reinterpret_cast<i32 **>(clip + 8);
        for (u32 item_index = 0; item_index < nitems; ++item_index) {
            NUDISPLAYLISTITEM *item = &scene->items[items[item_index]];
            if (item->type == 0x8f) {
                continue;
            }
            g_boundMaterial = scene->mtls[materials[item_index]];
            g_LastMtl = g_boundMaterial;
            u8 vertex_flags = reinterpret_cast<u8 *>(&g_boundMaterial->shader_desc.vtx_desc)[2];
            if ((vertex_flags & 0x10) == 0) {
                if ((vertex_flags & 0x20) == 0) {
                    NuIOS_SetVertexFormat(reinterpret_cast<usize>(g_boundMaterial->vertex_decl));
                } else {
                    g_boundVertexFormat = reinterpret_cast<usize>(g_nuFaceOnVertexFormat);
                }
            } else {
                g_boundVertexFormat = reinterpret_cast<usize>(g_nuDebrisVertexFormat);
            }
            NuIOSDLPreWarmGeomCallback(item->next);
        }
    }
}

extern "C" void NuGScnFixupPS(NUGSCN *scene) {
    struct NativeScene {
        u16 nvertex_buffers;
        u16 pad_02;
        usize *vertex_buffers;
        u16 nindex_buffers;
        u16 pad_0a;
        usize *index_buffers;
        NUDISPLAYLISTGEOM **geometries;
        i32 ngeometries;
        struct NativeVertexStream **vertex_streams;
        i32 nvertex_streams;
    };

    struct NativeVertexStream {
        u32 unknown_00;
        u32 unknown_04;
        usize vertex_buffer;
    };

    NativeScene *native_scene = reinterpret_cast<NativeScene *>(scene->field437_0x1d0);
    i32 dynamic_indices[64];
    i32 ndynamic = 0;
    for (i32 i = 0; i < native_scene->ngeometries; ++i) {
        NUDISPLAYLISTGEOM *geometry = native_scene->geometries[i];
        i32 vertex_index = static_cast<i32>(geometry->index_buffer);
        i32 index_index = static_cast<i32>(geometry->vertex_buffer);
        geometry->index_buffer = static_cast<u32>(g_VideoResHeader.index_buffers[vertex_index]);
        geometry->vertex_buffer = g_VideoResHeader.vertex_buffers[index_index];
        if (geometry->vertex_buffer == 0) {
            geometry->index_count = 0;
            geometry->vertex_count = 0;
        }
        if (geometry->immediate != 0) {
            BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nugscn_android.c", 0x25b);
            NuIOSBindVAO(0);
            glGenBuffers(1, reinterpret_cast<GLuint *>(&geometry->vertex_format));
            glBindBuffer(GL_ARRAY_BUFFER, geometry->vertex_format);
            glBufferData(GL_ARRAY_BUFFER, geometry->vertex_stride * geometry->vertex_count, NULL, GL_DYNAMIC_DRAW);
            EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nugscn_android.c", 0x262);
            if (bgProcIsBgThread()) {
                NuIOS_YieldThread();
            }
            dynamic_indices[ndynamic++] = index_index;
        } else {
            geometry->vertex_format = 0;
        }
    }
    if (scene != NULL && scene->display_list != NULL && scene->display_list->name != NULL &&
        NuStrIStr(scene->display_list->name, "cloudcityescape_c") != NULL && native_scene != NULL &&
        native_scene->ngeometries > 0 && native_scene->geometries[native_scene->ngeometries - 1] != NULL) {
        NUDISPLAYLISTGEOM *geometry = native_scene->geometries[native_scene->ngeometries - 1];
        geometry->index_count = 0;
        geometry->vertex_count = 0;
    }
    for (i32 i = 0; i < ndynamic; ++i) {
        g_VideoResHeader.vertex_buffers[dynamic_indices[i]] = 0;
    }
    for (i32 i = 0; i < native_scene->nvertex_streams; ++i) {
        NativeVertexStream *stream = native_scene->vertex_streams[i];
        i32 index = static_cast<i32>(stream->vertex_buffer);
        stream->vertex_buffer = g_VideoResHeader.vertex_buffers[index];
    }
    native_scene->nvertex_buffers = g_VideoResHeader.nvertex_buffers;
    native_scene->nindex_buffers = g_VideoResHeader.nindex_buffers;
    memcpy(native_scene->vertex_buffers, g_VideoResHeader.vertex_buffers,
           g_VideoResHeader.nvertex_buffers * sizeof(*native_scene->vertex_buffers));
    memcpy(native_scene->index_buffers, g_VideoResHeader.index_buffers,
           g_VideoResHeader.nindex_buffers * sizeof(*native_scene->index_buffers));

    for (i32 i = 0; i < scene->nummtl; ++i) {
        NuMtlUpdate(scene->mtls[i]);
    }
    NuPortalMaxDepth(scene, scene->max_portals);
    NuThreadCriticalSectionBegin(g_vaoLifetimeMutex);
    NuIOS_ResetVAODuplicateFinder();
    PreWarmGeomsAndBakeVAOs(scene->display_list, scene->field437_0x1d0);
    NuThreadCriticalSectionEnd(g_vaoLifetimeMutex);
}

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

void NuGScnDestroyPS(nugscn_s *) {
}

extern "C" void NuGSceneSetCrossFade(void) {
}

extern "C" void NuGSceneSetCrossFadeAlpha(void) {
}

extern "C" void NuGSceneProcessCrossFade(void) {
}
