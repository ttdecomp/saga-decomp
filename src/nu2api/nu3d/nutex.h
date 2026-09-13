#pragma once

#include <pthread.h>
#include <stddef.h>

#include "decomp.h"
#include "nu2api/nucore/common.h"

#include "nu2api/nu3d/android/nutex_android.h"

struct nugscn_s;
enum nutexturetype_e : i32;

typedef enum nutextype_e {
    NUTEX_RTT24 = 15,
} NUTEXTYPE;

typedef struct nutex_s {
    NUTEXTYPE type;
    i32 width;
    i32 height;
} NUTEX;

struct nutexmanager_s;

extern nutexmanager_s *g_texman;
extern i32 streamOff;

typedef struct nunativetex_s {
    i32 width;
    i32 height;
    unsigned char checksum[16];
    i32 ref_count;
    void *image_data;
    u32 size;
    NUNATIVETEXPS platform;
} NUNATIVETEX;

enum NUTEXFORMAT : i32 {
    NUTEX_UNKNOWN = 0,
    NUTEX_DXT1 = 1,
    NUTEX_DX1A = 2, // DXT1 with Alpha (A1XD)
    NUTEX_DXT2 = 3,
    NUTEX_DXT3 = 4,
    NUTEX_DXT4 = 5,
    NUTEX_DXT5 = 6,
    NUTEX_RGBA32 = 7,   // Calculated from (bVar3 * 8 + 7) for 32-bit
    NUTEX_FLOAT16 = 8,  // 0x71 is D3DFMT_A16B16G16R16F
    NUTEX_FLOAT32 = 9,  // 0x74 is D3DFMT_A32B32G32R32F
    NUTEX_PAL8 = 12,    // 0xC  (PAL8)
    NUTEX_PAL4 = 13,    // 0xD  (PAL4)
    NUTEX_BANN = 14,    // 0xE  (BANN - likely a custom banner format)
    NUTEX_RGB24 = 15,   // Calculated from (bVar3 * 8 + 7) for 24-bit
    NUTEX_ETC1 = 17,    // 0x11 (ETC1)
    NUTEX_ETCA = 18,    // 0x12 (ETC with Alpha)
    NUTEX_PVRTC2 = 20,  // 0x14 (PT21)
    NUTEX_PVRTC2A = 21, // 0x15 (PT2A)
    NUTEX_PVRTC4 = 22,  // 0x16 (PTC1)
    NUTEX_PVRTC4A = 23, // 0x17 (PTCA)
    NUTEX_ATCA = 24,    // 0x18 (ACTA)
    NUTEX_ATC = 25      // 0x19 (ATC)
};

struct __attribute__((packed)) dds_pixelformat_s {
    u32 dw_size;
    u32 dw_flags;
    u32 dw_four_cc;
    u32 dw_rgb_bit_count;
    u32 dw_r_bit_mask;
    u32 dw_g_bit_mask;
    u32 dw_b_bit_mask;
    u32 dw_a_bit_mask;
};

struct __attribute__((packed)) dds_header_s {
    char magic[4];
    u32 dw_size;
    u32 dw_flags;
    u32 dw_height;
    u32 dw_width;
    u32 dw_pitch_or_linear_size;
    u32 dw_depth;
    u32 dw_mip_map_count;
    u32 dw_reserved1[11];
    struct dds_pixelformat_s ddspf;
    u32 dw_caps;
    u32 dw_caps2;
    u32 dw_caps3;
    u32 dw_caps4;
    u32 dw_reserved2;
};

#ifdef __cplusplus
extern "C" {
#endif
    extern pthread_mutex_t criticalSection;
    extern i32 max_textures;

    void NuTexInitEx(VARIPTR *buf, i32 max_tex_count);

    i32 NuTexRead(char *name, VARIPTR *buf, VARIPTR *buf_end);

    i32 NuTexCreate(NUTEX *tex);
    i32 NuTexCreateNative(NUNATIVETEX *tex, bool is_pvrtc);

    void NuTexDestroy(i32 tex_id);
    void NuTexCleartid(i32 tex_id);
    void NuTexCreateFramebufferCopy(void *buffer, void *buffer_end);
    i32 NuTexReserveNative(NUNATIVETEX *texture, i32 tex_id);

    NUNATIVETEX *NuTexGetNative(i32 tex_id);

    void NuTexAddReference(i32 tex_id, struct nugscn_s *scene);
    void NuTexRemoveReference(i32 tex_id, struct nugscn_s *scene);
    i32 NuTexGetRefCount(i32 tex_id);
    i32 NuTexResolveReference(struct nugscn_s *scene, i32 tex_id);

    i32 NuTexWidth(i32 tex_id);
    i32 NuTexHeight(i32 tex_id);

    void NuTexDisplayTexturePage(i32 page, float depth, i32 alpha);
#ifdef __cplusplus
}
#endif

void NuTexInitExPS(VARIPTR *buf);

nutexmanager_s *NuTexGetManager();
void NuTexManagerInit(VARIPTR *buf, VARIPTR buf_end);

void NuTexCreatePS(NUNATIVETEX *tex, bool is_pvrtc);

void NuTexDestroyPS(NUNATIVETEX *tex);

void NuChecksumAsHex(u8 *checksum, char *out);
void NuTexHiresFilename(i32 tex_id, char *filename);
i32 NuTexSwapHires(i32 tex_id_lo, i32 tex_id_hi);

void NuTexLoadHires(i32 tex_id);
void NuTexUnloadHires(i32 tex_id);

i32 NuTexGetReqSize(i32 tex_id, i32 level);

i32 NuTexReserve(i32 size);
void NuTexUnReserve();

i32 NuDDSGetTextureDescription(const char *dds_data, NUTEXFORMAT &out_format, i32 &out_width, i32 &out_height,
                               i32 &out_depth, i32 &out_mip_count, bool &out_is_cube_map, bool *out_has_four_cc);
void NuDDSSetTextureDescription(char *dds_data, NUTEXFORMAT format, i32 width, i32 height, i32 depth,
                                i32 mip_count, nutexturetype_e texture_type);
void NuDDSGetMipLevel(i32 width, i32 height, i32 depth, NUTEXFORMAT format, i32 mip_count, bool is_cube_map,
                      i32 level, i32 face, i32 &out_width, i32 &out_height, i32 &out_size);
i32 NuDDSGetSize(char const *dds_data);
void GetNativeTextureFormat(NUTEXFORMAT inFormat, i32 &outBpp, u32 &outInternalFormat, u32 &outType, u32 &outFormat,
                            bool &outIsCompressed, NUTEXFORMAT &outFormatEnum);
