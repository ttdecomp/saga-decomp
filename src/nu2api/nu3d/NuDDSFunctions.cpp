#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nu2api_nucore_types.h"
#include "nu2api/nucore/nuvuvec.hpp"

#include <cstring>

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

void NuDDSGetMipLevel(i32 width, i32 height, i32 depth, NUTEXFORMAT format, i32 mip_count, bool is_cube_map,
                      i32 level, i32 face, i32 &out_width, i32 &out_height, i32 &out_size) {
    if (width == 0 && height == 0 && mip_count == 1) {
        out_width = 0;
        out_height = 0;
        out_size = 0;
        return;
    }

    // These are the DDS file format's block rules. Other texture APIs differ for a few formats.
    static const u8 compressed_formats[128] = {
        1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static const u8 block_widths[128] = {
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 8, 8, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static const u8 minimum_block_rows[128] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    // Bit rate per pixel, including the effective rate of a compressed block.
    static const u8 bits_per_pixel[128] = {
        4, 4, 0, 0, 0, 8, 32, 64, 128, 0, 0, 8, 4, 16, 24, 0,
        4, 8, 0, 2, 2, 4, 4, 8, 4, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 32, 0, 0, 2, 0, 0, 32, 0, 0, 0
    };

    bool compressed = false;
    i32 block_width = 4;
    i32 minimum_blocks = 1;
    i32 bpp = 0;
    if (format > NUTEX_UNKNOWN && format < 120) {
        i32 index = format - 1;
        compressed = compressed_formats[index];
        block_width = block_widths[index];
        minimum_blocks = minimum_block_rows[index];
        bpp = bits_per_pixel[index];
    }

    if (depth <= 0)
        depth = 1;

    out_width = 0;
    out_height = 0;
    i32 total_size = 0;
    i32 previous_size = 0;
    i32 mip_width = width;
    i32 mip_height = height;
    i32 mip_depth = depth;

    if (compressed) {
        i32 minimum_width = minimum_blocks * 4;
        i32 minimum_height = minimum_blocks * block_width;
        for (i32 mip = 0; mip <= mip_count; ++mip) {
            if (previous_size * 8 != bpp) {
                total_size += previous_size;
                if (mip <= level)
                    out_width = total_size;
            }

            i32 current_width = mip_width > minimum_width ? mip_width : minimum_width;
            i32 current_height = mip_height > minimum_height ? mip_height : minimum_height;
            i32 current_size = current_width * current_height * bpp * mip_depth / 8;
            if (mip <= level)
                out_height = current_size;
            previous_size = current_size;

            mip_width >>= 1;
            mip_height >>= 1;
            if (mip_depth != 1)
                mip_depth >>= 1;
        }
    } else {
        for (i32 mip = 0; mip <= mip_count; ++mip) {
            if (previous_size * 8 != bpp) {
                total_size += previous_size;
                if (mip <= level)
                    out_width = total_size;
            }

            i32 current_width = mip_width > 0 ? mip_width : 1;
            i32 current_height = mip_height > 0 ? mip_height : 1;
            i32 current_size = current_width * current_height * bpp * mip_depth / 8;
            if (mip <= level)
                out_height = current_size;
            previous_size = current_size;

            mip_width >>= 1;
            mip_height >>= 1;
            if (mip_depth != 1)
                mip_depth >>= 1;
        }
    }

    if (is_cube_map)
        out_width += total_size * face;
    out_size = compressed ? (block_width * 4 * bpp) / 8 : bpp / 8;
}

void NuDDSSetTextureDescription(char *dds_data, NUTEXFORMAT format, i32 width, i32 height, i32 depth, i32 mip_count,
                                nutexturetype_e texture_type) {
    dds_header_s *header = (dds_header_s *)dds_data;
    header->magic[0] = 'D';
    header->magic[1] = 'D';
    header->magic[2] = 'S';
    header->magic[3] = ' ';
    memset(dds_data + 4, 0, sizeof(dds_header_s) - 4);

    switch (format) {
        case NUTEX_DXT1:
        case NUTEX_DX1A:
            header->ddspf.dw_four_cc = 0x31545844;
            break;
        case NUTEX_DXT2:
            header->ddspf.dw_four_cc = 0x32545844;
            break;
        case NUTEX_DXT3:
            header->ddspf.dw_four_cc = 0x33545844;
            break;
        case NUTEX_DXT4:
            header->ddspf.dw_four_cc = 0x34545844;
            break;
        case NUTEX_DXT5:
            header->ddspf.dw_four_cc = 0x35545844;
            break;
        case NUTEX_RGBA32:
            header->ddspf.dw_flags |= 0x40;
            header->ddspf.dw_rgb_bit_count = 32;
            header->ddspf.dw_four_cc = 0;
            break;
        case NUTEX_FLOAT16:
            header->ddspf.dw_four_cc = 0x71;
            break;
        case NUTEX_FLOAT32:
            header->ddspf.dw_four_cc = 0x74;
            break;
        case NUTEX_PAL8:
            header->ddspf.dw_four_cc = 0x384c4150;
            header->ddspf.dw_flags |= 0x20;
            break;
        case NUTEX_PAL4:
            header->ddspf.dw_four_cc = 0x344c4150;
            header->ddspf.dw_flags |= 0x8;
            break;
        case NUTEX_BANN:
            header->ddspf.dw_four_cc = 0x4e4e4142;
            break;
        case NUTEX_RGB24:
            header->ddspf.dw_flags |= 0x40;
            header->ddspf.dw_rgb_bit_count = 24;
            header->ddspf.dw_four_cc = 0;
            break;
        case NUTEX_ETC1:
            header->ddspf.dw_four_cc = 0x31435445;
            break;
        case NUTEX_ETCA:
            header->ddspf.dw_four_cc = 0x41435445;
            break;
        case NUTEX_PVRTC2:
            header->ddspf.dw_four_cc = 0x31325450;
            break;
        case NUTEX_PVRTC2A:
            header->ddspf.dw_four_cc = 0x41325450;
            break;
        case NUTEX_PVRTC4:
            header->ddspf.dw_four_cc = 0x31435450;
            break;
        case NUTEX_PVRTC4A:
            header->ddspf.dw_four_cc = 0x41435450;
            break;
        case NUTEX_ATCA:
            header->ddspf.dw_four_cc = 0x41435441;
            break;
        case NUTEX_ATC:
            header->ddspf.dw_four_cc = 0x20435441;
            break;
        default:
            break;
    }

    header->dw_flags |= 6;
    header->dw_width = width;
    header->dw_height = height;
    if (depth > 1) {
        header->dw_flags |= 0x800000;
        header->dw_depth = depth;
    }
    header->dw_mip_map_count = 1;
    if (mip_count > 1) {
        header->dw_mip_map_count = mip_count;
        header->dw_flags |= 0x20000;
    }
    if (texture_type == 3) {
        header->dw_caps |= 8;
        header->dw_caps2 |= 0xfe00;
    }
}
