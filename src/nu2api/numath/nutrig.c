#include "nu2api/numath/nutrig.h"

#include "nu2api/nucore/common.h"
#include "nu2api/numath/nufloat.h"

#define PI_OVER_4 0.785398f
#define NEG_1_OVER_6 -0.166667f
#define NEG_3_OVER_40 -0.075f
#define NEG_5_OVER_112 -0.0446429f
#define NEG_35_OVER_1152 -0.0303819f
#define MAX_SHORT_OVER_PI 10430.4f

#define POW2(x) ((x) * (x))
#define POW3(x) ((x) * POW2(x))
#define POW4(x) (POW2(x) * POW2(x))
#define POW5(x) (POW2(x) * POW3(x))

static short NuASin(f32 sin) {
    f32 abs;
    f32 sqrt;
    f32 unknown_a;
    f32 unknown_b;
    f32 unknown_c;
    f32 unknown_d;

    abs = NuFabs(sin);
    sqrt = NuFsqrt(1.0f - sin * sin);

    unknown_a = MIN(sqrt, abs);

    unknown_b = MAX(MIN((abs - 0.70710677f) * 3.40282e+38f, 1.0f), -1.0f);

    unknown_c = MIN(sin * 3.40282e+38f, 1.0f);
    unknown_c = MAX(unknown_c, -1.0f);

    unknown_d = unknown_b * unknown_c + unknown_c;

    return (NEG_3_OVER_40 * POW2(unknown_b * unknown_c * unknown_a) * POW3(unknown_b * unknown_c * unknown_a) +
            (unknown_d * PI_OVER_4 - (unknown_b * unknown_c * unknown_a) +
             NEG_1_OVER_6 * (unknown_b * unknown_c * unknown_a) * POW2(unknown_b * unknown_c * unknown_a)) +
            NEG_5_OVER_112 * POW3(unknown_b * unknown_c * unknown_a) * POW4(unknown_b * unknown_c * unknown_a) +
            NEG_35_OVER_1152 * POW4(unknown_b * unknown_c * unknown_a) * POW5(unknown_b * unknown_c * unknown_a)) *
           MAX_SHORT_OVER_PI;
}

short NuACos(f32 cos) {
    return 0x4000 - NuASin(cos);
}

NUANG NuAngAdd(NUANG a, NUANG b) {
    NUANG sum = (a + b) & 0xffff;
    if (sum > 0x7fff) {
        sum -= 0x10000;
    }
    return sum;
}

NUANG NuAngSub(NUANG a, NUANG b) {
    NUANG difference = (a - b) & 0xffff;
    if (difference > 0x7fff) {
        difference -= 0x10000;
    }
    return difference;
}

void NuAng2AltSol(NUANG *out_x, NUANG *out_y, NUANG *out_z, NUANG x, NUANG y, NUANG z) {
    *out_x = x + 0x8000;
    *out_y = -0x8000 - y;
    *out_z = z + 0x8000;
}
