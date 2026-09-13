#pragma once

#include "decomp.h"
#include "legoapi/gizmo/base/gizmo.h"

#ifdef __cplusplus

typedef struct MINICUTPART_s {
    char name[0x20];
    NUVEC *resolved_position; // 0x20
    NUVEC position;           // 0x24
    f32 field_0x30;
    i16 field_0x34;
    i16 field_0x36;
    i16 field_0x38;
    u8 reserved_0x3a[2];
    f32 field_0x3c;
    f32 field_0x40;
} MINICUTPART;

DECOMP_ASSERT(sizeof(MINICUTPART) == 0x44, "MINICUTPART ABI");
DECOMP_ASSERT(offsetof(MINICUTPART, resolved_position) == 0x20, "MINICUTPART resolved-position offset");

typedef struct MINICUT_s {
    char name[0x10];
    MINICUTPART *parts; // 0x10
    i32 part_count;     // 0x14
    u8 played;          // 0x18
    u8 reserved_19;
    u16 guid;           // 0x1a
    f32 field_0x1c;
    f32 field_0x20;
    f32 field_0x24;
    f32 field_0x28;
    f32 field_0x2c;
} MINICUT;

DECOMP_ASSERT(sizeof(MINICUT) == 0x30, "MINICUT ABI");
DECOMP_ASSERT(offsetof(MINICUT, parts) == 0x10, "MINICUT parts offset");
DECOMP_ASSERT(offsetof(MINICUT, part_count) == 0x14, "MINICUT part-count offset");
DECOMP_ASSERT(offsetof(MINICUT, guid) == 0x1a, "MINICUT guid offset");

ADDGIZMOTYPE *MiniCut_RegisterGizmo(i32 type_id);
i32 GizMiniCut_GetGuid(GIZMO_s *gizmo);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
