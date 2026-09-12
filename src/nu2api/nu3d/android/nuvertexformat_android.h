#pragma once

#include "nu2api/nu3d/numtl.h"

// Original NuGetVertexDeclaration (libTTapp.so vaddr 0x2fc9a0, size 0xd91) fills a
// pool of vertex declarations. One pool record ("NuVertexFormatPS" in the original
// PS2-era naming) is 0x13c bytes and is addressed as an array of dwords by the
// attribute-binding code:
//
//   [+0x000]             u32            enabled-attribute bitmask (bit i -> attribs[i])
//   [+0x004 + i*0x18]    attrib record  one per attribute LOCATION i (13 slots):
//                          +0x00 GLenum type
//                          +0x04 component count
//                          +0x08 normalized byte (three padding bytes follow)
//                          +0x0c unknown (nonzero marks special stride handling)
//                          +0x10 byte offset into the vertex
//                          +0x14 stride
//
// Callers (NuIOS_BindVertexAttributesInternal) index it as
// fmt[0] = mask, fmt[i*6+1] = type, fmt[i*6+2] = count, fmt[i*6+3] = normalized,
// fmt[i*6+5] = offset, fmt[i*6+6] = stride.
typedef struct NuVertDeclAttribPS {
    u32 type;      // +0x00 GLenum (GL_FLOAT 0x1406, GL_UNSIGNED_BYTE 0x1401, GL_HALF_FLOAT 0x8d61)
    u32 size;      // +0x04 component count
    u8 normalized; // +0x08 GLboolean
    u8 reserved_09[3];
    u32 unknown_0c;   // +0x0c nonzero on location 12 (duplicate position): forces fixed stride
    u32 offset;       // +0x10
    u32 stride;       // +0x14 total vertex stride (written for every enabled attribute)
} NuVertDeclAttribPS; // 0x18 bytes

typedef struct NuVertexFormatPS {
    u32 attrib_mask;                // +0x000
    NuVertDeclAttribPS attribs[13]; // +0x004 locations 0..12
} NuVertexFormatPS;                 // 0x13c bytes

// One g_vertexFormatPool entry (original static bss 0x11b90a0, 256 * 0x140 bytes):
// the descriptor key followed by the attribute array the function returns.
typedef struct NuVertexFormatPoolEntryPS {
    u32 key;                 // +0x000 NUVERTEXDESCRIPTOR bitfield
    NuVertexFormatPS format; // +0x004
} NuVertexFormatPoolEntryPS; // 0x140 bytes

#ifdef __cplusplus
extern "C" {
#endif
    NuVertexFormatPS *NuGetVertexDeclaration(NUVERTEXDESCRIPTOR vtx_desc);
#ifdef __cplusplus
}
#endif
