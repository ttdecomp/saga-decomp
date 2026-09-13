#pragma once

#include "decomp.h"

struct nuqtdim_s {
    f32 x0;
    f32 x1;
    f32 y0;
    f32 y1;
};

struct nuqtentry_s {
    i16 count;
    i16 child;
    u8 *data;
    u32 field_08;
};

struct nuqthdr_s {
    u32 field_00[5];
    nuqtentry_s *entries;
    i32 entry_count;
    i32 entry_capacity;
    u8 *data;
    i32 data_used;
    i32 data_capacity;
    i32 element_size;
    u32 field_30;
    u32 field_34;
};

DECOMP_ASSERT(sizeof(nuqtdim_s) == 0x10, "NuQT dimensions size");
DECOMP_ASSERT(sizeof(nuqtentry_s) == 0xc, "NuQT entry size");
DECOMP_ASSERT(sizeof(nuqthdr_s) == 0x38, "NuQT header size");
