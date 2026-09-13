#ifndef GAMELIB_UTIL_CRC16_H
#define GAMELIB_UTIL_CRC16_H

#include "nu2api/nucore/fixed_width.h"

struct CRC16 {
    CRC16();

    static u32 hash(unsigned char const *data, i32 length);
    static u32 hashInverse(unsigned char const *data, i32 length);

    static CRC16 instance;
    static u32 crcTable[256];
};

#endif
