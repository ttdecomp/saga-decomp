#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nu2api_nucore_types.h"
#include "nu2api/nucore/nuvuvec.hpp"

enum DDSCAPS : u32 {
    DDSCAPS2_CUBEMAP = 0x200, // Required for a cubemap
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000
};

i32 NuDDSGetTextureDescription(const char *dds_data, NUTEXFORMAT &out_format, i32 &out_width, i32 &out_height,
                               i32 &out_depth, i32 &out_mip_count, bool &out_is_cube_map, bool *out_has_four_cc)

{
    dds_header_s *header = (dds_header_s *)dds_data;

    if (header->magic[0] != 'D')
        return 0;
    if (header->magic[1] != 'D')
        return 0;
    if (header->magic[2] != 'S')
        return 0;

    u32 four_cc = header->ddspf.dw_four_cc;
    *out_has_four_cc = (four_cc != 0);

    u32 flags = header->ddspf.dw_flags;

    if ((flags & 0x40) != 0) {
        out_format = (NUTEXFORMAT)((header->ddspf.dw_four_cc == 0x18) * 8 + 7);
    } else if ((flags & 0x20) != 0) {
        out_format = NUTEX_PAL8;
    } else if ((flags & 0x8) != 0) {
        out_format = NUTEX_PAL4;
    } else {
        switch (four_cc) {
            case 0x31545844:
                out_format = NUTEX_DXT1;
                break;
            case 0x41315844:
                out_format = NUTEX_DX1A;
                break;
            case 0x32545844:
                out_format = NUTEX_DXT2;
                break;
            case 0x33545844:
                out_format = NUTEX_DXT3;
                break;
            case 0x34545844:
                out_format = NUTEX_DXT4;
                break;
            case 0x35545844:
                out_format = NUTEX_DXT5;
                break;
            case 0x00000071:
                out_format = NUTEX_FLOAT16;
                break;
            case 0x00000074:
                out_format = NUTEX_FLOAT32;
                break;
            case 0x344c4150:
                out_format = NUTEX_PAL4;
                break;
            case 0x384c4150:
                out_format = NUTEX_PAL8;
                break;
            case 0x4e4e4142:
                out_format = NUTEX_BANN;
                break;
            case 0x31435445:
                out_format = NUTEX_ETC1;
                break;
            case 0x41435445:
                out_format = NUTEX_ETCA;
                break;
            case 0x31325450:
                out_format = NUTEX_PVRTC2;
                break;
            case 0x41325450:
                out_format = NUTEX_PVRTC2A;
                break;
            case 0x31435450:
                out_format = NUTEX_PVRTC4;
                break;
            case 0x41435450:
                out_format = NUTEX_PVRTC4A;
                break;
            case 0x41435441:
                out_format = NUTEX_ATCA;
                break;
            case 0x20435441:
                out_format = NUTEX_ATC;
                break;
            case 0:
                out_format = (NUTEXFORMAT)((header->ddspf.dw_rgb_bit_count != 0x20) * 8 + 7);
                break;
            default:
                break;
        }
    }

    out_is_cube_map = false;
    out_width = 0;
    out_height = 0;
    out_depth = 0;

    out_width = header->dw_width;
    out_height = header->dw_height;

    if ((header->dw_flags & 0x800000) != 0) {
        out_depth = header->dw_depth;
    }

    u32 mipmap_count = header->dw_mip_map_count;
    if (mipmap_count == 0) {
        mipmap_count = (header->dw_flags & 0x20000) == 0;
    }
    out_mip_count = mipmap_count;

    if ((header->dw_caps & 8) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP_POSITIVEY) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP_POSITIVEX) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ) == 0)
        return 1;
    if ((header->dw_caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY) == 0)
        return 1;

    out_is_cube_map = true;
    return 1;
}

i32 NuDDSGetSize(char const *dds_data) {
    NUTEXFORMAT format;
    i32 width;
    i32 height;
    i32 depth;
    i32 mip_count;
    bool is_cube_map;
    bool has_four_cc = false;
    NuDDSGetTextureDescription(dds_data, format, width, height, depth, mip_count, is_cube_map, &has_four_cc);

    i32 mip_width;
    i32 mip_height;
    i32 mip_size;
    NuDDSGetMipLevel(width, height, depth, format, mip_count, is_cube_map, mip_count, is_cube_map ? 6 : 0,
                     mip_width, mip_height, mip_size);

    i32 palette_size = 0;
    if (format == NUTEX_PAL8) {
        palette_size = 0x200;
    } else if (format == NUTEX_PAL4) {
        palette_size = 0x20;
    }
    return sizeof(dds_header_s) + palette_size + mip_width + mip_height;
}

void NuDDSGetMipLevel(i32, i32, i32, NUTEXFORMAT, i32, bool, i32, i32, i32 &, i32 &, i32 &) {
}

void NuDDSSetTextureDescription(char *, NUTEXFORMAT, i32, i32, i32, i32, nutexturetype_e) {
}
