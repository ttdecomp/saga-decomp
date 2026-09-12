#include "nu2api/numath/nurand.h"

u32 fseed;

static NURAND global_rand = {1};

i32 NuRand(NURAND *rand) {
    i32 value;

    if (rand != NULL) {
        if (rand->value == 0) {
            rand->value = 1;
        }
    } else {
        rand = &global_rand;
    }

    rand->value ^= 0x075bd924;

    value = rand->value / 0x1f31d;
    rand->value = (rand->value + value * -0x1f31d) * 0x41a7 + value * -0xb14;

    if (rand->value < 0) {
        rand->value += 0x7fffffff;
    }

    rand->value ^= 0x075bd924;

    return rand->value;
}

f32 NuRandFloat() {
    return NuRandFloatSeeded(&fseed);
}

f32 NuRandFloatSeeded(u32 *seed) {
    union {
        u32 int_val;
        f32 float_val;
    } value;

    *seed = *seed * 0x019660d + 0x3c6ef35f;
    value.int_val = *seed & 0x07fffff | 0x3f800000;

    return value.float_val - 1.0f;
}

u32 NuRandInt() {
    return NuRandIntSeeded(&fseed);
}

u32 NuRandIntSeeded(u32 *seed) {
    *seed = *seed * 0x019660d + 0x3c6ef35f;

    return *seed;
}

void NuRandSeed(u32 seed) {
    fseed = seed;
}

void NuRandSetSeed(NURAND *rand, i32 seed) {
    if (rand == NULL) {
        rand = &global_rand;
    }

    rand->value = seed;
}

u32 NuRandGetSeed(void) {
    return fseed;
}

f32 NuFloatRand(NURAND *rand) {
    return NuRand(rand) / 2.1474836e+09f;
}
