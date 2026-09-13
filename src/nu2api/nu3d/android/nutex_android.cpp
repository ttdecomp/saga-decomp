#include "nu2api/nu3d/android/nutex_android.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <GLES2/gl2.h>

#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nuplatform/nuplatform.h"

#define TEX_PATH "mnt/sdcard/TTGames/com.wb.lego.tcs/files/androidTextures/"

i32 NuTexCreate(NUTEX *tex) {
    return 0;
}

i32 NuTexRead(char *name, VARIPTR *buf, VARIPTR *buf_end) {
    i32 file_size;
    char *ext;
    bool is_pvrtc_supported;
    char filename[1024];
    char filepath[1024];
    FILE *file;
    NUFILE file_handle;
    i32 extra_bytes;
    NUNATIVETEX *tex;
    char *tex_data_ptr;
    char *data;
    i32 bytes_read;
    i32 tex_id;

    bytes_read = 0;
    file = NULL;

    ext = NuPlatform::Get()->GetCurrentTextureExtension();

    if (NuPlatform::Get()->GetCurrentPlatform() == IOS_PLATFORM ||
        NuPlatform::Get()->GetCurrentPlatform() == ANDROID_PVRTC_PLATFORM) {
        NuStrFixExtPlatform(filename, name, "tex", sizeof(filename), "ios");
    } else {
        NuStrFixExtPlatform(filename, name, ext, sizeof(filename), "MOB");
    }

    g_datfileMode = 1;

    extra_bytes = 0;

    if (g_datfileMode != 0) {
        file_handle = NuFileOpen(filename, NUFILE_READ);
        if (file_handle == 0) {
            return 0;
        }

        file_size = NuFileOpenSize(file_handle);
    } else {
        i32 path_len;

        memcpy(filepath, TEX_PATH, sizeof(filepath));

        extra_bytes = 1;

        path_len = strlen(filepath);
        strcat(filepath, filename);

        for (i32 i = path_len; i < strlen(filepath); i++) {
            filepath[i] = toupper(filepath[i]);

            if (filepath[i] == '\\') {
                filepath[i] = '/';
            }
        }

        file = fopen(filepath, "rb");
        if (file != NULL) {
            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
        } else {
            return 0;
        }
    }

    tex = (NUNATIVETEX *)ALIGN(buf->addr, 0x4);
    buf->addr = ALIGN(buf->addr, 0x4);
    buf->addr += sizeof(NUNATIVETEX);

    tex_data_ptr = buf->char_ptr;

    data = (char *)ALIGN(buf->addr, 0x4);
    buf->addr = ALIGN(buf->addr, 0x4);
    buf->addr += file_size + extra_bytes;

    tex->image_data = data;
    tex->size = file_size;

    if (g_datfileMode != 0) {
        NuFileRead(file_handle, data, file_size);
        NuFileClose(file_handle);
    } else {
        if (file_size > 0) {
            bytes_read = fread(data, 1, file_size, file);
            fclose(file);
        }
    }

    tex_id = NuTexCreateNative(tex, false);

    memset(tex_data_ptr, 0, data + file_size - tex_data_ptr);
    buf->void_ptr = tex_data_ptr;

    return tex_id;
}

void NuTexCreatePS(NUNATIVETEX *tex, bool is_pvrtc) {
    if (tex == NULL || tex->image_data == NULL || tex->size == 0) {
        return;
    }

    tex->platform.gl_tex = NuIOS_CreateGLTexFromPlatformInMemory(tex->image_data, &tex->width, &tex->height, is_pvrtc);
    tex->image_data = NULL;
    tex->size = 0;
}

void NuTexDestroyPS(NUNATIVETEX *tex) {
    if (tex != NULL && tex->platform.gl_tex != 0) {
        NudxFw_D3DBeginCriticalSection();

        glDeleteTextures(1, &tex->platform.gl_tex);
        tex->platform.gl_tex = 0;

        NudxFw_D3DEndCriticalSection();
    }
}

i32 g_textureLoadBufferCriticalSection;

GLuint g_LegoEnvTexture;
GLuint g_PhongEnvTexture;
GLuint g_whiteTexture;

void NuTexInitExPS(VARIPTR *buf) {
    GLuint white;

    g_textureLoadBufferCriticalSection = NuThreadCreateCriticalSection();

    g_LegoEnvTexture = NuIOS_CreateGLTexFromFile("pc/stuff/legocubemap_ios.tex");
    g_PhongEnvTexture = NuIOS_CreateGLTexFromFile("pc/stuff/phongmap_ios.tex");

    glGenTextures(1, &g_whiteTexture);
    glActiveTexture(GL_TEXTURE0);

    g_currentTexUnit = 0;

    glBindTexture(GL_TEXTURE_2D, g_whiteTexture);

    white = 0xffffffff;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void NuTexDisplayTexturePage(i32 page, f32 depth, i32 alpha) {
}

i32 NuTexGetReqSize(i32 tex_id, i32 level) {
    return 0;
}

i32 NuTexReserve(i32 size) {
    return -1;
}

void NuTexUnReserve() {
}

NUTEXBITMAP *NuTexReadBitmap(char *) {
    return NULL;
}

void NuTexAssignAddr(i32, i32) {
}

extern "C" i32 NuTexCreateEx(void) {
    return 0;
}

void NuTexReadTex(void) {
}

void NuTexSetTextureWithStagePS(NUNATIVETEX *tex, GLuint stage) {
    GLuint gl_tex;

    glActiveTexture(GL_TEXTURE0 + stage);

    g_currentTexUnit = stage;

    glBindTexture(GL_TEXTURE_2D, tex != NULL ? tex->platform.gl_tex : 0);
}
