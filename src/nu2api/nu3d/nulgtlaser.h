#pragma once

#include "decomp.h"
#include "nu2api/numath/nuvec.h"

struct NULGTLASER {
    NUVEC start;
    NUVEC end;
    f32 width;
    f32 segment_length;
    f32 width_wobble;
    f32 end_width_ratio;
    f32 length;
    u8 type;
    u8 arc;
    u8 padding[2];
    u32 colour;
    i32 seed;
};
DECOMP_ASSERT(sizeof(NULGTLASER) == 0x38, "Lightning record size");
DECOMP_ASSERT(offsetof(NULGTLASER, type) == 0x2c, "Lightning type offset");
DECOMP_ASSERT(offsetof(NULGTLASER, seed) == 0x34, "Lightning seed offset");

struct NULGTARCMATERIAL {
    struct numtl_s *material;
    f32 u0, v0, u1, v1;
};
DECOMP_ASSERT(sizeof(NULGTARCMATERIAL) == 0x14, "Lightning material size");
i32 NuLgtRand();
void NuLgtSetArcMatEx(i32 type, struct numtl_s *material, f32 u0, f32 v0, f32 u1, f32 v1);
extern "C" {
    extern NULGTARCMATERIAL NuLgtArcMtl[4];
    extern i32 NuLgtLaserOldCnt;
    extern u32 NuLgtSeed;
    void NuLgtLaserDraw(i32 paused);
    void NuLgtSetArcMat(struct numtl_s *material, f32 u0, f32 v0, f32 u1, f32 v1);
    extern i32 NuLgtLaserCnt;
    extern i32 NuLgtArcLaserCnt;
    extern i32 NuLgtArcLaserFrame;
    extern NULGTLASER NuLgtLaserData[64];
    void NuLgtLaser(i32 type, f32 width, f32 segment_length, f32 width_wobble, NUVEC *start, NUVEC *delta, u32 colour,
                    f32 end_width, f32 length);
}

struct NULGTARCLASER {
    NUVEC start, end, bend;
    f32 width, segment_length, wobble, bend_amount;
    u8 type;
    u8 padding[3];
    i32 colour, flags, seed;
};
DECOMP_ASSERT(sizeof(NULGTARCLASER) == 0x44, "Arc lightning record size");
DECOMP_ASSERT(offsetof(NULGTARCLASER, colour) == 0x38, "Arc lightning colour offset");
void NuLgtArcLaserEx(i32 type, NUVEC *start, NUVEC *end, NUVEC *bend, f32 width, f32 segment_length, f32 wobble,
                     f32 bend_amount, i32 colour, i32 flags);
extern "C" {
    extern NULGTARCLASER NuLgtArcLaserData[16];
    extern i32 NuLgtArcLaserOldCnt;
    void NuLgtArcLaser(i32 type, NUVEC *start, NUVEC *end, NUVEC *bend, f32 width, f32 segment_length, f32 wobble,
                       f32 bend_amount, i32 colour);
}
