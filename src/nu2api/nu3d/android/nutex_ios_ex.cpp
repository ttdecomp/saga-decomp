#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/nutex.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <squish.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "decomp.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nuplatform/nuplatform.h"

i32 g_currentTexUnit = -1;
i32 g_loadDefaultTexture;
GLuint g_lastBound2DTexIds[16];
GLuint g_lastBoundCubeTexIds[16];
GLuint g_earlyColorFramebuffer;
GLuint g_defaultFramebuffer;
GLuint g_currentFramebuffer;
GLuint g_earlyColorMSAAFramebuffer;
SAGA_HOST_WEAK GLuint g_earlyColorTexture = 0;

u32 g_textureHash;
i32 comeFromHash;
i32 g_logoTexture;
const char *g_textureName;
i32 g_fileSize;

extern i32 g_loadingCharacterInHub;
extern "C" const i16 *_toupper_tab_;

extern "C" __attribute__((weak)) void NuIOS_UploadCompressedTexture(GLenum target, GLint level, GLenum internal_format,
                                                                    GLsizei width, GLsizei height, GLint border,
                                                                    GLsizei image_size, const void *data) {
    glCompressedTexImage2D(target, level, internal_format, width, height, border, image_size, data);
}

__attribute__((weak)) bool NuIOS_TextureFormatSupported(i32 format) {
    return g_renderDevice.enabled_extensions[format];
}

GLuint NuIOS_CreateGLTexFromPlatfomSpecificFile(const char *filename) {
    static u8 buffer[0x600081];
    NUFILE file = NuFileOpen((char *)filename, NUFILE_READ);
    g_textureName = filename;
    if (file == 0) {
        return 0;
    }

    i32 remaining = NuFileOpenSize(file);
    g_fileSize = remaining;
    NuThreadCriticalSectionBegin(g_textureLoadBufferCriticalSection);
    const i32 chunk_limit = g_loadingCharacterInHub != 0 ? 0x4000 : static_cast<i32>(sizeof(buffer));
    u8 *dst = buffer;
    do {
        i32 chunk;
        if (bgProcIsBgThread() != 0) {
            chunk = remaining > chunk_limit ? chunk_limit : remaining;
            remaining -= chunk;
        } else {
            chunk = remaining;
            remaining = 0;
        }
        NuFileRead(file, dst, chunk);
        dst += chunk;
        if (g_loadingCharacterInHub != 0 && bgProcIsBgThread() != 0) {
            NuIOS_YieldThread();
        }
    } while (remaining != 0);
    NuFileOpenSize(file);
    NuFileClose(file);

    i32 width = 0;
    i32 height = 0;
    GLuint texture = NuIOS_CreateGLTexFromPlatformInMemory(buffer, &width, &height, false);
    NuThreadCriticalSectionEnd(g_textureLoadBufferCriticalSection);
    return texture;
}

GLuint NuIOS_CreateGLTexFromPlatfomSpecificForecPVR(const char *filename) {
    static u8 buffer[0x20004c];
    NUFILE file = NuFileOpen((char *)filename, NUFILE_READ);
    if (file == 0) {
        return 0;
    }

    i32 remaining = NuFileOpenSize(file);
    NuThreadCriticalSectionBegin(g_textureLoadBufferCriticalSection);
    const i32 chunk_limit = g_loadingCharacterInHub != 0 ? 0x4000 : static_cast<i32>(sizeof(buffer));
    u8 *dst = buffer;
    do {
        i32 chunk;
        if (bgProcIsBgThread() != 0) {
            chunk = remaining > chunk_limit ? chunk_limit : remaining;
            remaining -= chunk;
        } else {
            chunk = remaining;
            remaining = 0;
        }
        NuFileRead(file, dst, chunk);
        dst += chunk;
        if (g_loadingCharacterInHub != 0 && bgProcIsBgThread() != 0) {
            NuIOS_YieldThread();
        }
    } while (remaining != 0);
    NuFileOpenSize(file);
    NuFileClose(file);

    GLuint texture = NuIOS_CreateGLTexFromPlatformInMemory(buffer, nullptr, nullptr, true);
    NuThreadCriticalSectionEnd(g_textureLoadBufferCriticalSection);
    return texture;
}

GLuint NuIOS_CreateGLTexFromHash(u32 hash) {
    char filename[0x10c];
    GLuint texture = 0;

    g_textureHash = hash;
    comeFromHash = 1;
    if (hash == 0x280145f0) {
        g_logoTexture = 1;
    }

    switch (NuPlatform::Get()->GetCurrentPlatform()) {
        case IOS_PLATFORM:
        case ANDROID_PVRTC_PLATFORM:
            snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0X%08X.PVR", hash);
            texture = NuIOS_CreateGLTexFromPlatfomSpecificFile(filename);
            break;
        case ANDROID_ATITC_PLATFORM:
            snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0x%08x.atitc", hash);
            texture = NuIOS_CreateGLTexFromPlatfomSpecificFile(filename);
            if (texture == 0) {
                snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0X%08X.PVRNC", hash);
                texture = NuIOS_CreateGLTexFromPlatfomSpecificForecPVR(filename);
            }
            break;
        case ANDROID_S3TC_PLATFORM:
        case ANDROID_ETC1_PLATFORM: {
            const char *extension = NuPlatform::Get()->GetCurrentPlatform() == ANDROID_S3TC_PLATFORM ? "S3TC" : "ETC1";
            snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0X%08X.%s", hash, extension);
            texture = NuIOS_CreateGLTexFromPlatfomSpecificFile(filename);
            if (texture == 0) {
                snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0X%08X.PVRNC", hash);
                texture = NuIOS_CreateGLTexFromPlatfomSpecificForecPVR(filename);
            }
            break;
        }
        default:
            snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0X%08X.PVR", hash);
            texture = NuIOS_CreateGLTexFromPlatfomSpecificFile(filename);
            if (texture == 0) {
                snprintf(filename, sizeof(filename), "SHAREDTEXTURES/0X%08X.PVRNC", hash);
                texture = NuIOS_CreateGLTexFromPlatfomSpecificForecPVR(filename);
            }
            break;
    }

    g_textureHash = 0;
    g_logoTexture = 0;
    return texture;
}

GLuint NuIOS_CreateGLTexFromFile(const char *filename) {
    const i32 platform = NuPlatform::Get()->GetCurrentPlatform();
    if (platform == ANDROID_ATITC_PLATFORM) {
        goto load_android_texture;
    }
    if (platform >= ANDROID_ATITC_PLATFORM) {
        if (platform == ANDROID_PVRTC_PLATFORM) {
            return NuIOS_CreateGLTexFromPlatfomSpecificFile(filename);
        }
        if (platform <= ANDROID_ETC1_PLATFORM) {
            goto load_android_texture;
        }
    }
    return 0;

load_android_texture:
    char fixed_filename[0x200];
    char extension[0x200];
    NuStrCpy(extension, NuPlatform::Get()->GetCurrentTextureExtension());
    NuStrFixExtPlatform(fixed_filename, const_cast<char *>(filename), extension, sizeof(fixed_filename),
                        const_cast<char *>("MOB"));

    if (g_datfileMode == 0) {
        char external_filename[0x200] = "mnt/sdcard/TTGames/com.tt.LegoStarWarsSaga/files/androidTextures/";
        const usize path_length = strlen(external_filename);
        strcpy(&external_filename[path_length], fixed_filename);
        for (usize index = path_length; index < strlen(external_filename); ++index) {
            i32 character = external_filename[index];
            SAGA_UPPERCASE_CHAR(character, _toupper_tab_);
            external_filename[index] = static_cast<char>(character);
            if (external_filename[index] == '\\') {
                external_filename[index] = '/';
            }
        }
        NuStrCpy(fixed_filename, external_filename);
    }

    GLuint texture = NuIOS_CreateGLTexFromPlatfomSpecificFile(fixed_filename);
    if (texture != 0) {
        return texture;
    }
    return NuIOS_CreateGLTexFromPlatfomSpecificForecPVR(filename);
}

GLuint NuIOS_CreateGLTexFromPlatformInMemory(void *data, i32 *width, i32 *height, bool is_pvrtc) {
    const i32 platform = NuPlatform::Get()->GetCurrentPlatform();
    if (!is_pvrtc) {
        if (platform > ANDROID_ETC1_PLATFORM) {
            return loadDefaultTexture(0, 0, 0x20, GL_TEXTURE_2D, GL_TEXTURE_2D);
        }

        u32 platform_bit = 1;
        platform_bit <<= platform;
        if ((platform_bit & 0x1a00) == 0) {
            if ((platform_bit & 0x0500) == 0) {
                return loadDefaultTexture(0, 0, 0x20, GL_TEXTURE_2D, GL_TEXTURE_2D);
            }
            const GLuint texture = NuIOS_CreateGLTexFromPVRInMemory(data, width, height);
            if (texture != 0) {
                return texture;
            }
        } else {
            const GLuint texture = NuIOS_CreateGLTexFromMemoryDDS(data, width, height);
            if (texture != 0) {
                return texture;
            }
        }
    } else {
        const GLuint texture = NuIOS_CreateGLTexFromPVRInMemory(data, width, height);
        if (texture != 0) {
            return texture;
        }
    }
    return loadDefaultTexture(0, 0, 0x20, GL_TEXTURE_2D, GL_TEXTURE_2D);
}

GLuint loadDefaultTexture(GLuint texture, GLint level, GLsizei size, GLenum texture_type, GLenum target) {
    isize pixel_count = size * size;
    u8 *pixels = (u8 *)malloc(pixel_count * 4);
    u8 *p1 = pixels + 8;
    u8 *p2 = pixels + 4;
    for (i32 i = 0; i < pixel_count; i += 2) {
        i32 row = i / size;

        if (row & 1) {
            p1[-8] = 0x52;
            p1[-7] = 0x52;
            p1[-6] = 0x99;
            p1[-5] = 0xFF;
            p2[3] = 0xFF;
            p2[0] = 0x7A;
            p2[1] = 0x7A;
            p2[2] = 0x7A;
        } else {
            p1[-8] = 0x7A;
            p1[-7] = 0x7A;
            p1[-6] = 0x7A;
            p1[-5] = 0xFF;
            p2[3] = 0xFF;
            p2[0] = 0x52;
            p2[1] = 0x52;
            p2[2] = 0x99;
        }
        p1 += 8;
        p2 += 8;
    }

    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 552);

    GLuint textures[3];
    textures[0] = texture;
    if (texture == 0) {
        glGenTextures(1, textures);
    }

    if (texture_type == GL_TEXTURE_2D) {
        glActiveTexture(GL_TEXTURE0);
        g_currentTexUnit = 0;
        glBindTexture(GL_TEXTURE_2D, textures[0]);
    } else {
        if (g_currentTexUnit != 0) {
            glActiveTexture(GL_TEXTURE0);
            g_currentTexUnit = 0;
        }
        if (g_lastBoundCubeTexIds[0] != textures[0]) {
            glBindTexture(GL_TEXTURE_CUBE_MAP, textures[0]);
            g_lastBoundCubeTexIds[0] = textures[0];
        }
    }

    glTexImage2D(target, level, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    NuCheckGLErrorsFL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 574);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    NuCheckGLErrorsFL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 583);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 585);
    free(pixels);

    return textures[0];
}

GLuint NuIOS_CreateGLTexFromMemoryDDS(void *ddsPointer, i32 *out_width, i32 *out_height) {
    GLuint texID = 0;
    unsigned char *decompressedBuffer = nullptr;
    unsigned char *pixelData = nullptr;

    NUTEXFORMAT format;
    i32 depth = 0;
    i32 mipCount = 0;
    bool isCubemap = false;
    bool hasFourCC = false;

    bool success = NuDDSGetTextureDescription((char *)ddsPointer, format, *out_width, *out_height, depth, mipCount,
                                              isCubemap, &hasFourCC);

    if (success && (*out_width != 0 || *out_height != 0 || mipCount > 1)) {
        texID = CreateTexturePS();
        i32 bpp;
        u32 glInternalFormat;
        u32 glType;
        u32 glFormat;
        bool isCompressed;
        NUTEXFORMAT nativeFormat;

        GetNativeTextureFormat(format, bpp, glInternalFormat, glType, glFormat, isCompressed, nativeFormat);

        pixelData = (unsigned char *)ddsPointer + sizeof(dds_header_s);

        if (format == NUTEX_DXT5) {
            decompressedBuffer = nullptr;
            DecompressTextureToRGBA((unsigned char *)ddsPointer, 0, decompressedBuffer);

            format = NUTEX_RGBA32;
            isCompressed = false;
            glInternalFormat = GL_RGBA;
            glFormat = GL_RGBA;

            pixelData = decompressedBuffer;
        }
        UnlockTexturePS(texID, pixelData, *out_width, *out_height, depth, isCubemap, mipCount, format, glFormat,
                        glInternalFormat, glType, isCompressed);
    }

    return texID;
}

GLuint CreateTexturePS(void) {
    GLuint tex;

    tex = 0;
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 0x1ad);
    glGenTextures(1, &tex);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 0x1b1);
    return tex;
}

void GetNativeTextureFormat(NUTEXFORMAT inFormat, i32 &outBpp, u32 &outInternalFormat, u32 &outType, u32 &outFormat,
                            bool &outIsCompressed, NUTEXFORMAT &outFormatEnum) {
    i32 formatToCheck = inFormat;

    if (inFormat == NUTEX_DXT1) {
        if (g_renderDevice.enabled_extensions[NUTEX_DXT1]) {
            outBpp = 0;
        } else if (g_renderDevice.enabled_extensions[NUTEX_ETC1]) {
            inFormat = NUTEX_ETC1;
            formatToCheck = NUTEX_ETC1;
            outBpp = 0;
        } else {
            outIsCompressed = false;
            inFormat = NUTEX_RGBA32;
            outBpp = 0;
        }
    } else {
        outIsCompressed = false;
        outBpp = 0;
    }

    switch (inFormat) {
        case NUTEX_DXT1:                                         // 1
            outInternalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT; // 0x83F0
            outIsCompressed = true;
            outFormat = GL_RGB;         // 0x1907
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_DX1A:                                          // 2
            outInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; // 0x83F1
            outIsCompressed = true;
            outFormat = GL_RGBA;        // 0x1908
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_DXT5:                 // 6
            outInternalFormat = GL_RGBA; // 0x1908
            outIsCompressed = true;
            outFormat = GL_RGBA;        // 0x1908
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case 0x10:
            outInternalFormat = GL_LUMINANCE; // 0x1909
            outBpp = 8;
            outFormat = GL_LUMINANCE;   // 0x1909
            outType = GL_UNSIGNED_BYTE; // 0x1401
            outFormatEnum = inFormat;
            return;

        case NUTEX_ETC1:                          // 0x11
            outInternalFormat = GL_ETC1_RGB8_OES; // 0x8D64
            outIsCompressed = true;
            outFormat = GL_RGB;         // 0x1907
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_PVRTC2:                                          // 0x14
            outInternalFormat = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG; // 0x8C01
            outIsCompressed = true;
            outFormat = GL_RGB;         // 0x1907
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_PVRTC2A:                                          // 0x15
            outInternalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG; // 0x8C03
            outIsCompressed = true;
            outFormat = GL_RGBA;        // 0x1908
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_PVRTC4:                                          // 0x16
            outInternalFormat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG; // 0x8C00
            outIsCompressed = true;
            outFormat = GL_RGB;         // 0x1907
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_PVRTC4A:                                          // 0x17
            outInternalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG; // 0x8C02
            outIsCompressed = true;
            outFormat = GL_RGBA;        // 0x1908
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_ATCA:                                            // 0x18
            outInternalFormat = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD; // 0x87EE
            outIsCompressed = true;
            outFormat = GL_RGBA;        // 0x1908
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case NUTEX_ATC:                         // 0x19
            outInternalFormat = GL_ATC_RGB_AMD; // 0x8C92
            outIsCompressed = true;
            outFormat = GL_RGB;         // 0x1907
            outType = GL_UNSIGNED_BYTE; // 0x1401
            break;

        case 0x66:
            outInternalFormat = GL_RGB; // 0x1907
            outBpp = 24;                // 0x18
            outFormat = GL_RGB;         // 0x1907
            outType = GL_UNSIGNED_BYTE; // 0x1401
            outFormatEnum = inFormat;
            return;

        case 0x69:
        case 0x72:
            outFormatEnum = inFormat;
            return;

        default:
            outInternalFormat = GL_RGBA;
            outBpp = 32;
            outFormat = GL_RGBA;
            outType = GL_UNSIGNED_BYTE;
            outFormatEnum = inFormat;
            return;
    }

    if (!NuIOS_TextureFormatSupported(formatToCheck)) {
        while (true) {
        }
    }

    outFormatEnum = inFormat;
}

void GetTextureFormatInfo(NUTEXFORMAT texture, u32 &param_2, u32 &param_3) {
    param_2 = 1;
    switch (texture) {
        case NUTEX_DXT1:
        case NUTEX_DX1A:
            param_2 = 4;
            param_3 = 4;
            return;
        case NUTEX_DXT2:
        case NUTEX_DXT3:
        case NUTEX_DXT4:
        case NUTEX_ETC1:
            param_2 = 4;
            param_3 = 8;
            return;
        case NUTEX_DXT5:
            param_2 = 4;
            param_3 = 8;
            return;
        case NUTEX_RGBA32:
        case 0x72:
        case 0x74:
            param_3 = 0x20;
            return;
        case NUTEX_FLOAT16:
        case 0x6e:
        case 0x75:
        case 0x76:
            param_3 = 0x40;
            return;
        case NUTEX_FLOAT32:
        case 0x13:
            param_3 = 0x80;
            return;
        case 0x10:
            param_3 = 8;
            return;
        case NUTEX_PVRTC2:
        case NUTEX_PVRTC2A:
            param_2 = 8;
            param_3 = 2;
            return;
        case NUTEX_PVRTC4:
        case NUTEX_PVRTC4A:
            param_2 = 8;
            param_3 = 4;
            return;
        case 0x6c:
            param_3 = 0x10;
            return;
    }
}

i32 GetMipLevelSize(NUTEXFORMAT param_1, i32 unclamped_width, i32 unclamped_height) {
    u32 block_size;
    u32 bpp;

    GetTextureFormatInfo(param_1, block_size, bpp);
    u32 width = 1;
    if (unclamped_width > 0) {
        width = unclamped_width;
    }
    u32 height = unclamped_height;
    if (unclamped_height <= 0) {
        height = 1;
    }
    u32 div_w = width / block_size;
    u32 multiplier = (block_size * block_size * bpp) >> 3;
    u32 m_w = 1;
    if (div_w != 0) {
        m_w = div_w;
    }
    u32 div_h = height / block_size;
    u32 m_h = 1;
    if (div_h != 0) {
        m_h = div_h;
    }
    u32 base_area = m_w * m_h;
    return base_area * multiplier;
}

i32 GetMipOffset(NuHardwareTexture *tex, i32 targetMip, i32 targetSlice, NUTEXFORMAT overrideFormat) {
    if (tex->depth == 0) {
        tex->depth = 1;
    }

    NUTEXFORMAT format = (overrideFormat != 0) ? overrideFormat : tex->format;
    bool isAutoMode = (targetMip < 0 && targetSlice < 0);
    i32 currentMipLimit = targetMip;
    i32 currentSliceLimit = targetSlice;

    if (isAutoMode) {
        currentMipLimit = tex->mips;
        currentSliceLimit = (tex->flags & 1) ? 5 : tex->depth;
    }

    i32 totalOffset = 0;
    i32 currentSlice = 0;

    while (true) {
        i32 innerMipLimit = currentMipLimit;

        if ((tex->flags & 1) && (currentSlice < targetSlice)) {
            innerMipLimit = tex->mips;
        }

        i32 currentMip = 0;
        while (true) {
            i32 w = tex->width >> currentMip;
            i32 h = tex->height >> currentMip;

            i32 levelSize = GetMipLevelSize(format, w, h);

            if (currentMip == innerMipLimit) {
                if (!isAutoMode && tex->depth > 1) {
                    totalOffset += levelSize * targetSlice;
                }
                break;
            }

            i32 d = tex->depth >> currentMip;
            if (d < 1)
                d = 1;

            totalOffset += d * levelSize;
            currentMip++;
        }

        if (!(tex->flags & 1) || currentSlice == currentSliceLimit) {
            break;
        }

        currentSlice++;
    }

    return totalOffset;
}

#define SQUISH_KDXT1 (1 << 0) // 1
#define SQUISH_KDXT3 (1 << 1) // 2
#define SQUISH_KDXT5 (1 << 2) // 4

static const i32 CSWTCH_249[] = {SQUISH_KDXT1, SQUISH_KDXT3, SQUISH_KDXT5};

void DecompressTextureToRGBA(unsigned char *ddsData, u32 size, unsigned char *&outBuffer) {
    i32 squishFlags;
    NuMemory *v8;
    NuMemoryManager *threadMem;
    static unsigned char *buffer = nullptr;

    NuHardwareTexture tex;

    NUTEXFORMAT format;
    i32 width, height, depth;
    i32 mips;
    bool isCubemap;
    bool unkDesc;

    NuDDSGetTextureDescription((char *)ddsData, format, width, height, depth, mips, isCubemap, &unkDesc);

    squishFlags = 1;
    if ((u32)(format - 4) < 3) {
        squishFlags = CSWTCH_249[format - 4];
    }

    tex.format = format;
    tex.width = width;
    tex.height = height;
    tex.depth = depth;

    tex.flags = (tex.flags & 0x80) | (isCubemap & 1);

    tex.lp_raw_data = nullptr;
    tex.lp_device_resource = nullptr;
    tex.data_size = 0;
    tex.ref_count = 0;
    tex.checksum = 0;
    tex.unknown = 0;
    tex.mips = mips;
    tex.type = 0;

    ddsData += 128;

    GetMipOffset(&tex, -1, -1, NUTEX_RGBA32);

    if (buffer == nullptr) {
        v8 = NuMemoryGet();
        threadMem = v8->GetThreadMem();
        buffer = (unsigned char *)threadMem->_BlockAlloc(0x6000000, 4, 1, "DecompressBuffer", 0);
    }

    outBuffer = buffer;

    i32 faceLimit = isCubemap ? 6 : 1;

    for (i32 face = 0; face < faceLimit; face++) {
        if (tex.mips != 0) {
            for (i32 mip = 0; mip < tex.mips; mip++) {
                i32 mipOffset = GetMipOffset(&tex, mip, face, NUTEX_UNKNOWN);
                i32 rgbaOffset = GetMipOffset(&tex, mip, face, NUTEX_RGBA32);
                squish::DecompressImage(outBuffer + rgbaOffset, tex.width >> mip, tex.height >> mip,
                                        ddsData + mipOffset, squishFlags);
            }
        }
    }
}

i32 GetMipOffset(i32 width, i32 height, NUTEXFORMAT format, i32 depth, bool isCubemap, i32 mips, i32 targetMip,
                 i32 targetSlice) {
    i32 totalOffset = 0;
    i32 currentSlice = 0;
    i32 mipLimit;
    i32 sliceLimit;
    bool isAuto;

    if (depth == 0) {
        depth = 1;
    }

    isAuto = targetMip < 0 && targetSlice < 0;

    if (isAuto != 0) {
        sliceLimit = 5;
        mipLimit = mips;
        if (!isCubemap) {
            sliceLimit = depth;
        }
    } else {
        mipLimit = targetMip;
        sliceLimit = targetSlice;
    }

    bool isCompressed;
    i32 minBlocks;
    i32 bytesPerBlockOrPixel;
    i32 blockWidth;

    if (format > 0 && format < 119) {
        const i32 formatIndex = format - 1;
        isCompressed = FormatIsCompressedTable[formatIndex];
        minBlocks = FormatMinBlocksYTable[formatIndex];
        bytesPerBlockOrPixel = FormatBytesPerElementTable[formatIndex];
        blockWidth = FormatBlockWidthTable[formatIndex];
    } else {
        isCompressed = false;
        minBlocks = 1;
        bytesPerBlockOrPixel = 0;
        blockWidth = 4;
    }

    while (true) {
        i32 innerMipLimit = mipLimit;

        if (isCubemap && (currentSlice < targetSlice)) {
            innerMipLimit = mips;
        }

        i32 m = 0;

        while (true) {
            i32 w = width >> m;
            if (w < 1)
                w = 1;

            i32 h = height >> m;
            if (h < 1)
                h = 1;

            i32 levelSize;

            if (!isCompressed) {
                levelSize = w * h * bytesPerBlockOrPixel;
            } else {
                i32 blocksY = h >> 2;
                if (blocksY < minBlocks)
                    blocksY = minBlocks;

                i32 blocksX = w / blockWidth;
                if (blocksX < minBlocks)
                    blocksX = minBlocks;

                levelSize = blocksX * blocksY * bytesPerBlockOrPixel;
            }

            if (m == innerMipLimit) {
                if (isAuto == 0 && depth > 1) {
                    totalOffset += levelSize * targetSlice;
                }
                break;
            }

            i32 d = depth >> m;
            if (d <= 0)
                d = 1;

            totalOffset += d * levelSize;
            m++;
        }

        if (!isCubemap || (currentSlice == sliceLimit)) {
            break;
        }

        currentSlice++;
    }

    return totalOffset;
}

void UnlockTexturePS(u32 texID, void *pixels, i32 width, i32 height, i32 depth, bool isCubemap, i32 mips,
                     NUTEXFORMAT format, u32 &glFormat, u32 &glInternalFormat, u32 glType, bool isCompressed) {
    u32 faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    const i32 faceCount = isCubemap ? 6 : 1;

    for (i32 loopCounter = 0; loopCounter < faceCount; ++loopCounter, ++faceTarget) {
        if (mips != 0) {
            u32 texTarget = GL_TEXTURE_2D;
            if (isCubemap) {
                texTarget = faceTarget;
            }

            i32 mip = 0;
            i32 mipWidth;
            i32 hLimit;

            for (i32 mip = 0; mip < mips; ++mip) {
                mipWidth = width >> mip;
                if (mipWidth < 1) {
                    mipWidth = 1;
                }

                i32 mipHeight = height >> mip;
                hLimit = 1;
                if (mipHeight > 0) {
                    hLimit = mipHeight;
                }

                i32 currentOffset = GetMipOffset(width, height, format, depth, isCubemap, mips, mip,
                                                 faceTarget - GL_TEXTURE_CUBE_MAP_POSITIVE_X);

                unsigned char *mipData = (unsigned char *)pixels + currentOffset;

                if (!isCompressed) {
                    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                           0x58e);

                    u32 bindTarget = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
                    glBindTexture(bindTarget, texID);

                    if (g_loadDefaultTexture == 0) {
                        glTexImage2D(texTarget, mip, glInternalFormat, mipWidth, hLimit, 0, glFormat, glType, mipData);
                    } else {
                        loadDefaultTexture(texID, mip, mipWidth, GL_TEXTURE_2D, GL_TEXTURE_2D);
                    }
                    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                         0x5b3);

                } else {
                    i32 nextOffset;
                    i32 sizeOffset;
                    i32 targetSlice;

                    if (mips - 1 == mip) {
                        nextOffset = GetMipOffset(width, height, format, depth, isCubemap, mips, -1, -1);
                        sizeOffset = mips - 1;
                        targetSlice = isCubemap ? 5 : 0;
                    } else {
                        nextOffset = GetMipOffset(width, height, format, depth, isCubemap, mips, mip + 1, 0);
                        sizeOffset = mip;
                        targetSlice = 0;
                    }

                    sizeOffset = GetMipOffset(width, height, format, depth, isCubemap, mips, sizeOffset, targetSlice);
                    i32 mipSize = nextOffset - sizeOffset;

                    if (!isCubemap) {
                        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                               0x56e);
                        glBindTexture(GL_TEXTURE_2D, texID);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        if (g_loadDefaultTexture == 0) {
                            NuIOS_UploadCompressedTexture(GL_TEXTURE_2D, mip, glInternalFormat, mipWidth, hLimit, 0,
                                                          mipSize, mipData);
                        } else {
                            loadDefaultTexture(texID, mip, mipWidth, GL_TEXTURE_2D, GL_TEXTURE_2D);
                        }
                        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                             0x589);
                    } else {
                        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                               0x54b);
                        glBindTexture(GL_TEXTURE_CUBE_MAP, texID);
                        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                        if (g_loadDefaultTexture == 0) {
                            NuIOS_UploadCompressedTexture(faceTarget, mip, glInternalFormat, mipWidth, hLimit, 0,
                                                          mipSize, mipData);
                        } else {
                            loadDefaultTexture(texID, mip, mipWidth, GL_TEXTURE_CUBE_MAP, faceTarget);
                        }
                        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                             0x566);

                        g_loadDefaultTexture = 0;
                    }
                }
                if (mipWidth == 1 && hLimit == 1) {
                    break;
                }
            }
        }
    }

    if (!isCompressed) {
        BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 0x5bf);
        u32 bindTarget = isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        glGenerateMipmap(bindTarget);
        EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 0x5c1);
    }

    glFlush();
    g_loadDefaultTexture = 0;
}
