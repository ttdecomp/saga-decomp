#pragma once

#include "nu2api/nucore/common.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"
#include "nu2api/nu3d/nurndr.h"

struct numtl_s;
typedef struct numtl_s NUMTL;

// Per-material render-state cache (original type `nurndrstate_s`, 52 bytes).
// Field offsets follow the Ghidra DB exactly; they are the targets of the
// reset block in NuDisplayListBeforeFrame @0x2a9ea0 (stores at +0x8, +0x14,
// +0x1e, +0x20, +0x24, +0x2c, +0x2e, +0x32).
typedef struct nurndrstate_s {
    i32 global_state_bottom; // 0x00 gs_bot
    i32 global_state_top;    // 0x04 gs_top
    NUMTL *mtl;              // 0x08
    u16 unused;              // 0x0c
    u8 *unknown_10;          // 0x10 mpg scratch pointer
    i32 tex_id;              // 0x14 tid (-1 == none)
    i32 global_state_ptr;    // 0x18 gs_ptr
    u16 texture_slots;       // 0x1c
    i16 konst_id;            // 0x1e (-1 == none)
    i32 global_id;           // 0x20 (-1 == none)
    i16 lights_id;           // 0x24 (-1 == none)
    i16 lights_type;         // 0x26
    i16 lights_specular_id;  // 0x28
    i16 lights_fx_id;        // 0x2a
    i16 camera_id;           // 0x2c (-1 == none)
    i16 fog_id;              // 0x2e (-1 == none)
    i16 vertex_groups_id;    // 0x30 (-1 == none)
    i16 reflection_id;       // 0x32 (-1 == none)
} NURNDRSTATE;

// Global renderer state cached while display lists are built (original type
// `nuglobalrndrstate_s`, 0x1b0 bytes).  Camera data begins at 0x110; the
// display-list camera packet is materialised lazily from these fields.
typedef struct nuglobalrndrstate_s {
    NURNDRSTATE state;            // 0x000
    void *light_state;            // 0x034
    NUCOLOUR3 ambient_intensity;  // 0x038
    NUVEC light_direction[3];     // 0x044
    NUCOLOUR3 light_intensity[3]; // 0x068
    f32 global_specular;          // 0x08c
    NUMTX specular_mtx;           // 0x090
    NUCOLOUR3 specular_colour;    // 0x0d0
    NUVEC specular_intensity;     // 0x0dc
    void *lights_state_diffuse;   // 0x0e8
    void *lights_state_specular;  // 0x0ec
    void *lights_state_fx;        // 0x0f0
    u32 const_alpha_enabled;      // 0x0f4
    f32 const_alpha;              // 0x0f8
    NUMTL *const_alpha_mtl;       // 0x0fc
    u32 const_tint_enabled;       // 0x100
    NUCOLOUR3 const_tint;         // 0x104
    void *camera_state;           // 0x110
    u8 unknown_114[0x0c];         // 0x114
    NUMTX view;                   // 0x120
    f32 proj_00;                  // 0x160
    f32 proj_11;                  // 0x164
    f32 proj_22;                  // 0x168
    f32 proj_23;                  // 0x16c
    f32 proj_32;                  // 0x170
    void *fog_state;              // 0x174
    i32 fog_enabled;              // 0x178
    f32 fog_near;                 // 0x17c
    f32 fog_far;                  // 0x180
    u32 fog_rgba;                 // 0x184
    f32 fog_density;              // 0x188
    void *konst_state;            // 0x18c
    f32 vpx;                      // 0x190
    f32 vpy;                      // 0x194
    f32 vpw;                      // 0x198
    f32 vph;                      // 0x19c
    i32 reflection;               // 0x1a0
    void *reflection_state;       // 0x1a4
    f32 proj_20;                  // 0x1a8
    f32 proj_21;                  // 0x1ac
} NUGLOBALRNDRSTATE;

extern "C" {
    extern NUGLOBALRNDRSTATE render_state;
    void NuRndrStateInit(void);
    i32 NuRndrStateGetFogEnabled(void);
    void RndrStateSetReflection(i32 reflection);
    void NuRndrStateSetFogEnabled(i32 enabled);
    void NuRndrStateSetFogState(f32 near_distance, f32 far_distance, u32 colour, f32 density);
}

// Game-facing light selection state.  The first 0x54 bytes are the ambient
// colour followed by the three directional-light vectors and colours.  The
// specular direction/intensity and control integers follow at 0x54..0x74.
// SetSpecularLight uses 0x60 for a missing direction and 0x74 for an active intensity.
typedef struct nulightingstate_s {
    NUCOLOUR3 ambient;            // 0x00
    NUVEC direction[3];           // 0x0c
    NUCOLOUR3 intensity[3];       // 0x30
    NUVEC specular_direction;     // 0x54
    i32 field_0x60;               // 0x60
    NUCOLOUR4 specular_intensity; // 0x64
    i32 field_0x74;               // 0x74
} NULIGHTINGSTATE;

extern "C" NULIGHTINGSTATE NuRndrLightingStateCurrent;

// Display-list light packet built from NUGLOBALRNDRSTATE (original size 0xcc).
typedef struct nulightstate_s {
    NUCOLOUR4 ambient_intensity;
    NUCOLOUR4 light_intensity[3];
    NUVEC4 light_direction[3];
    NUMTX specular_mtx;
    NUCOLOUR3 specular_colour;
    f32 padding_bc;
    NUVEC specular_intensity;
} NULIGHTSTATE;

// Display-list fog packet built from NUGLOBALRNDRSTATE (original size 0x14).
// When enabled, the remaining fields carry the packed colour, near/far
// distances, and exponential density used by NuIOSDLFogCallback.
typedef struct nufogstate_s {
    i32 enabled;
    u32 colour;
    f32 near_distance;
    f32 far_distance;
    f32 density;
} NUFOGSTATE;

#ifdef __cplusplus
#if !defined(__x86_64__) // the tidy pre-pass parses as 64-bit host; real build is i686
static_assert(sizeof(NURNDRSTATE) == 52, "nurndrstate_s size");
static_assert(offsetof(NURNDRSTATE, mtl) == 0x08, "state.mtl");
static_assert(offsetof(NURNDRSTATE, tex_id) == 0x14, "state.tex_id");
static_assert(offsetof(NURNDRSTATE, konst_id) == 0x1e, "state.konst_id");
static_assert(offsetof(NURNDRSTATE, global_id) == 0x20, "state.global_id");
static_assert(offsetof(NURNDRSTATE, lights_id) == 0x24, "state.lights_id");
static_assert(offsetof(NURNDRSTATE, camera_id) == 0x2c, "state.camera_id");
static_assert(offsetof(NURNDRSTATE, fog_id) == 0x2e, "state.fog_id");
static_assert(offsetof(NURNDRSTATE, reflection_id) == 0x32, "state.reflection_id");
static_assert(sizeof(NUGLOBALRNDRSTATE) == 0x1b0, "nuglobalrndrstate_s size");
static_assert(sizeof(NULIGHTINGSTATE) == 0x78, "nulightingstate_s size");
static_assert(sizeof(NUFOGSTATE) == 0x14, "nufogstate_s size");
static_assert(offsetof(NUGLOBALRNDRSTATE, camera_state) == 0x110, "global camera_state");
static_assert(offsetof(NUGLOBALRNDRSTATE, view) == 0x120, "global view");
static_assert(offsetof(NUGLOBALRNDRSTATE, vpx) == 0x190, "global viewport");
#endif
#endif
