// Original NuGetVertexDeclaration @0x2fc9a0. Cache interleaved vertex layouts.
#include <string.h>

#include "nu2api/nu3d/android/nuvertexformat_android.h"

#include "nu2api/nu3d/numtl.h"

// g_vertexFormatPool: original static bss 0x11b90a0 (_ZL18g_vertexFormatPool),
// 256 entries of 0x140 bytes each.
static NuVertexFormatPoolEntryPS g_vertexFormatPool[256];

// g_allocatedDescriptorCount: original static bss 0x11cd0a0
// (_ZL26g_allocatedDescriptorCount).
static i32 g_allocatedDescriptorCount;

static_assert(sizeof(NuVertDeclAttribPS) == 0x18, "attrib record must be 6 dwords");
static_assert(sizeof(NuVertexFormatPS) == 0x13c, "format array must be mask + 13 attrib records");
static_assert(sizeof(NuVertexFormatPoolEntryPS) == 0x140, "pool record stride must be 0x140");

NuVertexFormatPS *NuGetVertexDeclaration(NUVERTEXDESCRIPTOR vtx_desc) {
    const i32 count = g_allocatedDescriptorCount;
    for (i32 i = 0; i < count; ++i) {
        if (memcmp(&g_vertexFormatPool[i].key, &vtx_desc, sizeof(vtx_desc)) == 0)
            return &g_vertexFormatPool[i].format;
    }
    if (static_cast<u32>(count) > 255)
        return NULL;
    NuVertexFormatPoolEntryPS *record = &g_vertexFormatPool[count];
    NuVertexFormatPS *format = &record->format;
    const u32 descriptor = vtx_desc.flags;
    i16 offset = 0;
    u32 mask = format->attrib_mask;
#define VERTEX_ATTRIBUTE(slot, gl_type, components, normalize, bytes)                                                  \
    format->attribs[slot].type = gl_type;                                                                              \
    format->attribs[slot].size = components;                                                                           \
    format->attribs[slot].normalized = normalize;                                                                      \
    format->attribs[slot].unknown_0c = 0;                                                                              \
    format->attribs[slot].offset = offset;                                                                             \
    mask |= 1u << slot;                                                                                                \
    offset += bytes
    VERTEX_ATTRIBUTE(0, 0x1406, 3, 0, 12);
    if (descriptor & 0x880008) {
        VERTEX_ATTRIBUTE(3, 0x1401, 4, 1, 4);
    } else if (descriptor & 4) {
        VERTEX_ATTRIBUTE(3, 0x1406, 3, 0, 12);
    }
    if (descriptor & 0x1000020) {
        VERTEX_ATTRIBUTE(4, 0x1401, 4, 1, 4);
    } else if (descriptor & 0x10) {
        VERTEX_ATTRIBUTE(4, 0x1406, 3, 0, 12);
    }
    if (descriptor & 0x2000080) {
        VERTEX_ATTRIBUTE(5, 0x1401, 4, 1, 4);
    } else if (descriptor & 0x40) {
        VERTEX_ATTRIBUTE(5, 0x1406, 3, 0, 12);
    }
    if (descriptor & 0x100) {
        VERTEX_ATTRIBUTE(1, 0x1401, 4, 1, 4);
    }
    if (descriptor & 0x600) {
        VERTEX_ATTRIBUTE(2, 0x1401, 4, 1, 4);
    }
    const u32 texture_mode = (descriptor >> 11) & 7;
    if (descriptor & 0x8000000) {
        switch (texture_mode) {
            case 1: {
                VERTEX_ATTRIBUTE(6, 0x8d61, 2, 0, 4);
                break;
            }
            case 2: {
                VERTEX_ATTRIBUTE(6, 0x8d61, 4, 0, 8);
                break;
            }
            case 3: {
                VERTEX_ATTRIBUTE(6, 0x8d61, 4, 0, 8);
                VERTEX_ATTRIBUTE(7, 0x8d61, 2, 0, 4);
                break;
            }
            case 4: {
                VERTEX_ATTRIBUTE(6, 0x8d61, 4, 0, 8);
                VERTEX_ATTRIBUTE(7, 0x8d61, 4, 0, 8);
                break;
            }
        }
    } else {
        switch (texture_mode) {
            case 1: {
                VERTEX_ATTRIBUTE(6, 0x1406, 2, 0, 8);
                break;
            }
            case 2: {
                VERTEX_ATTRIBUTE(6, 0x1406, 4, 0, 16);
                break;
            }
            case 3: {
                VERTEX_ATTRIBUTE(6, 0x1406, 4, 0, 16);
                VERTEX_ATTRIBUTE(7, 0x1406, 2, 0, 8);
                break;
            }
            case 4: {
                VERTEX_ATTRIBUTE(6, 0x1406, 4, 0, 16);
                VERTEX_ATTRIBUTE(7, 0x1406, 4, 0, 16);
                break;
            }
            case 5: {
                VERTEX_ATTRIBUTE(6, 0x1406, 2, 0, 8);
                VERTEX_ATTRIBUTE(7, 0x1406, 2, 0, 8);
                break;
            }
        }
    }
    if (descriptor & 0x8000) {
        VERTEX_ATTRIBUTE(10, 0x1401, 4, 1, 4);
    } else if (descriptor & 0x4000) {
        VERTEX_ATTRIBUTE(10, 0x1406, 2, 0, 8);
    }
    if (descriptor & 0x20000) {
        VERTEX_ATTRIBUTE(11, 0x1401, 4, 0, 4);
    } else if (descriptor & 0x10000) {
        VERTEX_ATTRIBUTE(11, 0x1406, 3, 0, 12);
    }
    if (descriptor & 0x4000000) {
        VERTEX_ATTRIBUTE(8, 0x1401, 4, 1, 4);
        VERTEX_ATTRIBUTE(9, 0x1401, 4, 1, 4);
    }
    i32 extra_stride = 0;
    if (descriptor & 0x400000) {
        format->attribs[12].type = 0x1406;
        format->attribs[12].size = 3;
        format->attribs[12].normalized = 0;
        format->attribs[12].unknown_0c = 1;
        format->attribs[12].offset = 0;
        mask |= 0x1000;
        extra_stride = 12;
    }
    for (i32 i = 0; i < 13; ++i) {
        if (mask & (1u << i))
            format->attribs[i].stride = format->attribs[i].unknown_0c ? extra_stride : offset;
    }
    format->attrib_mask = mask;
    record->key = descriptor;
    g_allocatedDescriptorCount = count + 1;
    return format;
#undef VERTEX_ATTRIBUTE
}
