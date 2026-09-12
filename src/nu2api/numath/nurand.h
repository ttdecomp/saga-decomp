#include "nu2api/nucore/common.h"

typedef struct nunrand_s {
    i32 value;
} NURAND;

#ifdef __cplusplus
extern "C" {
#endif
    extern u32 fseed;

    i32 NuRand(NURAND *rand);
    f32 NuRandFloat();
    f32 NuRandFloatSeeded(u32 *seed);
    u32 NuRandInt();
    u32 NuRandIntSeeded(u32 *seed);
    void NuRandSeed(u32 seed);
    void NuRandSetSeed(NURAND *rand, i32 seed);
    u32 NuRandGetSeed(void);

    f32 NuFloatRand(NURAND *rand);
#ifdef __cplusplus
}
#endif
