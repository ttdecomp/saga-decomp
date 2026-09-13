#pragma once

#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nurndr.h"

enum NUMTL_ALPHA_MODE {
    NUMTL_ALPHA_MODE_ALPHA = 1,
};

typedef struct numtlattrib_s {
    u32 alpha_mode : 4;
    u32 filter_mode : 2;

    u32 unknown_0_64_128 : 2;

    u32 unknown_1_1_2 : 2;
    u32 unknown_1_4_8 : 2;

    u32 cull_mode : 2;
    u32 z_mode : 2;

    u32 unknown_2_1_2 : 2;
    u32 unknown_2_4 : 1;
    u32 unknown_2_8 : 1;

    u32 alpha_test : 3;
    u32 alpha_ref : 8;
    u32 alpha_fail : 2;

    u32 uv_mode : 1;

    u32 unknown_4_8 : 1;
    u32 unknown_4_16 : 1;
    u32 unknown_4_32 : 1;
    u32 unknown_4_64 : 1;
    u32 unknown_4_128 : 1;

    u32 unknown_5_1 : 1;
    u32 unknown_5_2 : 1;
    u32 unknown_5_4 : 1;
    u32 unknown_5_8_16 : 2;
    u32 unknown_5_32 : 1;

    u32 layers : 8;

    u32 unknown_6_64 : 1;
    u32 unknown_6_128 : 1;

    u32 padding : 8;
} NUMTLATTRIB;

enum NUTEXANIM_MODE : u8 {
    NUTEXANIM_MODE_NONE = 0,
    NUTEXANIM_MODE_LINEAR = 2,
    NUTEXANIM_MODE_SINE = 3,
    NUTEXANIM_MODE_COSINE = 4,
};

typedef struct nutexanimdata_s {
    NUTEXANIM_MODE u_mode;
    NUTEXANIM_MODE v_mode;
    u8 padding_02[2];
    f32 u_amplitude;
    f32 v_amplitude;
    f32 u_frequency;
    f32 v_frequency;
} NUTEXANIMDATA;

typedef struct nuvertexdescriptor_s {
    union {
        struct {
            u32 has_position : 1;

            u32 unknown_0_2 : 1;

            u32 has_normal : 1;
            u32 has_packed_normal : 1;
            u32 has_tangent : 1;
            u32 has_packed_tangent : 1;
            u32 has_binormal : 1;
            u32 has_packed_binormal : 1;

            u32 has_diffuse : 1;

            u32 unknown_1_2_4 : 2;
            u32 tex_coord_mode : 3;
            u32 unknown_1_64 : 1;
            u32 unknown_1_128 : 1;

            u32 unknown_2_1 : 1;
            u32 unknown_2_2 : 1;

            u32 has_no_transform : 1;

            u32 unknown_2_8 : 1;
            u32 unknown_2_16 : 1;
            u32 unknown_2_32 : 1;
            u32 unknown_2_64 : 1;
            u32 unknown_2_128 : 1;

            u32 unknown_3_1_2_3 : 3;
            u32 has_half_uvs : 1;
        };

        u32 flags;
    };
} NUVERTEXDESCRIPTOR;

// Exact layout from the original binary (Ghidra struct nushadermtldesc_s,
// 520 bytes, embedded at numtl_s+0xB4 .. +0x2BC).
typedef struct nushadermtldesc_s {
    u32 flags; // 0x000

    i32 diffuse_map_tex_id[4];   // 0x004
    NUCOLOUR32 diffuse_color[4]; // 0x014

    f32 unknown_24;         // 0x024
    u8 unknown_28[0x0c];    // 0x028..0x033
    i32 lightmap_tex_id[2]; // 0x034
    u8 unknown_3c[0x08];    // 0x03C..0x043

    u8 blend_op2;  // 0x044 (0xff = no shader retrieval)
    u8 blend_op3;  // 0x045
    u8 blend_op4;  // 0x046
    u8 unknown_47; // 0x047

    i32 specular_map_tid;   // 0x048
    i32 normal_map_tid;     // 0x04C
    i32 envmap_cubic_tid;   // 0x050
    i32 shine_map_ps2_tid;  // 0x054
    i32 vtf_height_map_tid; // 0x058
    i32 vtf_normal_map_tid; // 0x05C

    u8 unknown_60[0x48]; // 0x060..0x0A7
    u8 unknown_a8;       // 0x0A8 (texture-count-ish, forced >= 1)
    u8 unknown_a9[0x63]; // 0x0A9..0x10B

    i32 tex_anim_data[4];       // 0x10C
    f32 tex_anim_offsets[4][2]; // 0x11C

    NUVERTEXDESCRIPTOR vtx_desc; // 0x13C

    i16 shader_id;         // 0x140 — assigned by NuMtlUpdatePS
    i16 shader_variant_id; // 0x142 (-1 when unvarianted)

    NUTEXANIMDATA tex_anim_desc[4]; // 0x144
    u8 unknown_194[4];              // 0x194..0x197
    i32 unknown_198;                // 0x198

    u8 unknown_19c[0x18]; // 0x19C..0x1B3
    u8 unknown_1b4;       // 0x1B4
    u8 filler_1b5[3];     // 0x1B5..0x1B7

    u8 flagsbits_1b8; // 0x1B8 (bitfield dword, accessed per-byte)
    u8 byte4;         // 0x1B9 (legacy name: alpha-test select bits)
    u8 flagsbits_1ba; // 0x1BA
    u8 flagsbits_1bb; // 0x1BB

    u32 field_1bc;        // 0x1BC
    u8 unknown_1c0[0x24]; // 0x1C0..0x1E3
    i32 field_1e4;        // 0x1E4
    i32 field_1e8;        // 0x1E8
    u8 unknown_1ec[0x1C]; // 0x1EC..0x207
} NUSHADERMTLDESC;

typedef struct numtl_s {
    i16 is_used : 1;
    i16 unknown_0_2 : 1;
    i16 unknown_0_4 : 1;
    i16 unknown_0_8 : 1;

    i16 renderplane : 8;

    struct numtl_s *unknown_04;

    // Type uncertain.
    i32 unknown_08;

    char filler1[0x24];

    struct numtl_s *next;

    u8 unknown_34[0x8];

    NUDISPLAYLIST *display_list;

    NUMTLATTRIB attribs;

    char filler2[0xc]; // 0x48-0x53

    NUCOLOUR3 diffuse_color;

    char filler3[0x10]; // 0x60-0x6F

    f32 opacity;

    i16 tex_id;
    i16 sort_pri;

    u8 unknown_78[3];
    u8 disable_u_animation; // 0x7b
    u8 disable_v_animation; // 0x7c
    u8 unknown_7d[3];

    f32 delta_u;
    f32 delta_v;

    u8 unknown_88[0x11];
    i8 particle_type_tag; // 0x99 (-105 marks the untextured debris material)
    u8 unknown_9a[0x1a];

    NUSHADERMTLDESC shader_desc;

    void *vertex_decl; // 0x2BC — NuGetVertexDeclaration result (read by the
                       // material display-list callback at mtl+700)

    u16 version; // 0x2C0, bumped by NuMtlUpdate, read by display-list submit

    u16 filler5; // 0x2C2

} NUMTL;

#ifdef __cplusplus

void DefaultMtl(NUMTL *mtl);
void NuMtlCreatePS(NUMTL *mtl, i32 is_3d);
void NuMtlSetUVOffsetPS(NUMTL *mtl, u32 layer, f32 u, f32 v);
extern "C" NUMTL *NuMtlCreate3D(i32 count);
extern "C" NUMTL *NuMtlCreateEx3D(i32 count, u8 render_plane);
extern "C" NUMTL *NuMtlCreateEx(i32 count, u8 render_plane);
extern "C" void NuMtlInitOverride(i32 count, VARIPTR *buffer, VARIPTR *end);
extern "C" void NuMtlDestroy(NUMTL *mtl);
void NuMtlInsert(NUMTL *mtl, i32 plane);
extern "C" void NuMtlSetRenderPlane(NUMTL *mtl, i8 plane);
extern "C" void NuMtlCopy(NUMTL *destination, const NUMTL *source);
extern "C" void NuDisplayListCreateMtl(NUMTL *mtl);
extern "C" void NuDisplayListDestroyMtl(NUMTL *mtl);
void NuMtlUpdatePS(NUMTL *mtl);

extern "C" {
#endif
    extern NUMTL *numtl_defaultmtl2d;
    extern NUMTL *numtl_defaultmtl3d;

    void NuMtlInitEx(VARIPTR *buf, i32 mtl_count);

    void NuShaderMtlDescInit(NUSHADERMTLDESC *desc);
    void NuMtlSetShaderDescPS(NUMTL *mtl, NUSHADERMTLDESC *desc);
    i32 NuMtlSetCurrentRenderPlane(i32 render_plane);
    NUMTL *NuMtlCreate(i32 count);
    void NuMtlUpdate(NUMTL *mtl);
#ifdef __cplusplus
}

void NuMtlInitExPS(VARIPTR *buf);

#endif

static_assert(sizeof(NUSHADERMTLDESC) == 0x208, "nushadermtldesc_s size");
static_assert(sizeof(NUTEXANIMDATA) == 0x14, "nutexanimdata_s size");
static_assert(offsetof(NUSHADERMTLDESC, lightmap_tex_id) == 0x34, "nushadermtldesc_s lightmap texture offset");
static_assert(offsetof(NUSHADERMTLDESC, vtx_desc) == 0x13c, "nushadermtldesc_s vtx_desc offset");
static_assert(offsetof(NUSHADERMTLDESC, byte4) == 0x1b9, "nushadermtldesc_s byte4 offset");
