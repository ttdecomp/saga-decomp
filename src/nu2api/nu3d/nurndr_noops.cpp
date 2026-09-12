#include "legoapi/legoapi_types.h"

extern "C" i32 NuRndrSetBlendData(void) {
    return 0;
}

i32 NuRndrFlickerBeginScene(void) {
    return 1;
}

extern "C" void NuRndrLine3dDbgFlush(void) {
}

extern "C" void NuRndrShadowOnOff(i32 enabled) {
    (void)enabled;
}

extern "C" void NuRndrScreenGrabTileInit(void *, i32, f32, f32, f32) {
}

extern "C" void NuRndrScreenGrabTileDeInit(void *) {
}

extern "C" void NuRndrScreenGrabTileBegin(void **) {
}

extern "C" void NuRndrScreenGrabTileEnd(void **) {
}
