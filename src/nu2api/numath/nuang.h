#pragma once

#include "nu2api/nucore/common.h"

typedef i32 NUANG;

#define NUANG_90DEG (NUANG)(0x4000)
#define NUANG_180DEG (NUANG)(0x8000)
#define NUANG_270DEG (NUANG)(0xC000)
#define NUANG_360DEG (NUANG)(0x10000)

struct nuangvec_s {
    NUANG x;
    NUANG y;
    NUANG z;
};

typedef struct nuangvec_s NUANGVEC;

#ifdef __cplusplus
extern "C" {
#endif
    NUANG NuAngAdd(NUANG a, NUANG b);
    NUANG NuAngSub(NUANG a, NUANG b);
    void NuAng2AltSol(NUANG *out_x, NUANG *out_y, NUANG *out_z, NUANG x, NUANG y, NUANG z);
#ifdef __cplusplus
}
#endif
