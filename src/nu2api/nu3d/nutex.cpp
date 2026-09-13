#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nutexanm.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nupostresources.h"

#include "decomp.h"
#include "nu2api/nu3d/android/nutex_android.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nustring.h"
#include <pthread.h>
#include <string.h>
#include "nu2api/nucore/numemory.h"

struct nutextureformat_e {};
struct nutexanimprog_s;

struct nutexmanager_s {
    u8 reserved[0x40];
};

DECOMP_ASSERT(sizeof(nutexmanager_s) == 0x40, "texture manager size");

nutexmanager_s *g_texman;
i32 streamOff;

extern "C" void NuTexAnimEnvReset(nutexanimenv_s *env);

void NuChecksumAsHex(u8 *checksum, char *out) {
    i32 i;
    char hex_digits[] = "0123456789abcdef";

    for (i = 0; i < 16; i++) {
        u8 check_digit = checksum[i];
        i32 least_sig_digit = check_digit >> 4;

        out[i * 2] = hex_digits[least_sig_digit];
        out[i * 2 + 1] = hex_digits[(u8)(check_digit - (least_sig_digit << 4))];
    }

    out[32] = '\0';
}

void NuTexHiresFilename(i32 tex_id, char *filename) {
    NUNATIVETEX *tex;
    char checksum_hex[33];

    tex = NuTexGetNative(tex_id);

    NuStrCpy(filename, "c:\\temp\\stream\\textures\\");
    NuChecksumAsHex(tex->checksum, checksum_hex);
    NuStrCat(filename, checksum_hex);
    NuStrCat(filename, ".tex");
}

i32 NuTexSwapHires(i32 tex_id_lo, i32 tex_id_hi) {
    return 0;
}

void NuTexLoadHires(i32 tex_id) {
    char hires_path[2048];
    i32 tex_id_hi;

    NuTexHiresFilename(tex_id, hires_path);
    tex_id_hi = NuTexRead(hires_path, NULL, NULL);
    NuTexSwapHires(tex_id, tex_id_hi);
}

void NuTexUnloadHires(i32 tex_id) {
}

void NuTexAddReference(i32 tex_id, NUGSCN *) {
    NUNATIVETEX *tex;

    tex = NuTexGetNative(tex_id);
    if (tex != NULL) {
        tex->ref_count++;
    }
}

void NuTexRemoveReference(i32 tex_id, NUGSCN *) {
    NUNATIVETEX *tex;

    tex = NuTexGetNative(tex_id);
    if (tex != NULL) {
        tex->ref_count--;
    }
}

i32 NuTexGetRefCount(i32 tex_id) {
    NUNATIVETEX *tex;

    tex = NuTexGetNative(tex_id);
    if (tex != NULL) {
        return tex->ref_count;
    }

    return 0;
}

i32 max_textures;
static NUNATIVETEX **texture_list;
static i32 *texture_order;
static i32 gTextureLoadCount;

void NuTexCleartid(i32 tex_id) {
    if (tex_id != 0) {
        if (NuTexGetRefCount(tex_id) != -1) {
            NuEffectTexGetEffectFromNative(tex_id);
            NuTexDestroyPS(texture_list[tex_id - 1]);
        }
        texture_list[tex_id - 1] = NULL;
    }
}

void NuTexCreateFramebufferCopy(void *buffer, void *buffer_end) {
    NuEffectTexLockVP(buffer, buffer_end);
    nuframebuffer_s *framebuffer = NuFramebufferGetObject(1);
    i32 width = NuFramebufferGetWidth(framebuffer);
    i32 height = NuFramebufferGetHeight(framebuffer);
    nueffecttex_s *texture = NuEffectTexCreate2D(width, height, 1, 1, 1);
    NuEffectTexUnlockVP();
    NuEffectTexMapNative(texture);
}

i32 NuTexReserveNative(NUNATIVETEX *texture, i32 tex_id) {
    if (texture == NULL)
        return 0;
    if (tex_id != 0) {
        texture_list[tex_id - 1] = texture;
        return tex_id;
    }
    for (i32 i = 0; i < max_textures; ++i) {
        if (texture_list[i] == NULL) {
            texture_list[i] = texture;
            return i + 1;
        }
    }
    return 0;
}

void NuTexInitEx(VARIPTR *buf, i32 max_tex_count) {
    max_textures = max_tex_count;

    texture_list = (NUNATIVETEX **)ALIGN(buf->addr, 0x4);
    buf->addr = (usize)texture_list + max_tex_count * sizeof(NUNATIVETEX *);
    memset(texture_list, 0, max_tex_count * sizeof(NUNATIVETEX *));

    texture_order = (i32 *)ALIGN(buf->addr, 0x4);
    buf->addr = (usize)texture_order + max_tex_count * sizeof(i32);
    memset(texture_order, 0, max_tex_count * sizeof(i32));

    gTextureLoadCount = 0;
}

pthread_mutex_t criticalSection = PTHREAD_MUTEX_INITIALIZER;

i32 NuTexCreateNative(NUNATIVETEX *tex, bool is_pvrtc) {
    i32 i;

    if (tex == NULL) {
        return 0;
    }

    pthread_mutex_lock(&criticalSection);

    for (i32 i = 0; i < max_textures; i++) {
        if (texture_list[i] == NULL) {
            texture_list[i] = tex;
            texture_order[i] = gTextureLoadCount++;

            pthread_mutex_unlock(&criticalSection);

            NuTexCreatePS(tex, is_pvrtc);

            return i + 1;
        }
    }

    pthread_mutex_unlock(&criticalSection);

    return 0;
}

extern "C" i32 NuTexGenTexture(NUNATIVETEX *tex) {
    for (i32 i = 0; i < max_textures; ++i) {
        if (texture_list[i] == NULL) {
            texture_list[i] = tex;
            return i + 1;
        }
    }
    return 0;
}

NUNATIVETEX *NuTexGetNative(i32 tex_id) {
    if (tex_id > 0) {
        return texture_list[tex_id - 1];
    }

    return NULL;
}

extern "C" i32 NuTexResolveReference(NUGSCN *scene, i32 tex_id) {
    if ((tex_id & 0x4000) == 0) {
        return scene->texture_ids[tex_id];
    }
    if (max_textures == 0) {
        return 0;
    }

    const i32 reference_index = tex_id & 0x3fff;
    NUNATIVETEX *reference = scene->textures[reference_index];
    i32 newest_texture_id = 0;
    i32 newest_texture_order = 0;

    for (i32 index = 0; index < max_textures; ++index) {
        NUNATIVETEX *texture = texture_list[index];
        if (texture == NULL || texture->ref_count < 0) {
            continue;
        }

        bool checksum_matches = true;
        for (i32 byte = 0; byte < 16; ++byte) {
            if (texture->checksum[byte] != reference->checksum[byte]) {
                checksum_matches = false;
                break;
            }
        }
        if (checksum_matches && static_cast<u32>(texture_order[index]) > static_cast<u32>(newest_texture_order)) {
            newest_texture_id = index + 1;
            newest_texture_order = texture_order[index];
        }
    }

    if (newest_texture_id != 0) {
        NuTexAddReference(newest_texture_id, scene);
        scene->texture_ids[reference_index] = newest_texture_id;
    }
    return newest_texture_id;
}

i32 NuTexWidth(i32 tex_id) {
    return texture_list[tex_id - 1]->width;
}

i32 NuTexHeight(i32 tex_id) {
    return texture_list[tex_id - 1]->height;
}


void NuTexRemap(i32, i32) {
}

nutexmanager_s *NuTexGetManager() {
    return g_texman;
}

void NuTexManagerInit(VARIPTR *buf, VARIPTR) {
    g_texman = reinterpret_cast<nutexmanager_s *>(ALIGN(buf->addr, 0x10));
    buf->addr = reinterpret_cast<usize>(g_texman + 1);
}

void NuTextureCreate3D(i32, i32, i32, i32, i32, nutextureformat_e) {
}

void NuTexManagerStream(nugscn_s *) {
    streamOff = 1;
}

i32 NuTexGetUnresolvedTextureTIDPS() {
    return 0;
}
