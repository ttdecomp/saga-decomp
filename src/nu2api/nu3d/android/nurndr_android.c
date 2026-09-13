#include "nu2api/nu3d/nurndr.h"
#include "decomp.h"
#include "globals.h"
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nu3d/android/nuptl_android.h"
#include "nu2api/nu3d/android/nurenderthread.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nushader_plain.h"

extern "C" {
// Original 0x295fa9: private to this renderer translation unit.
static void NuDisplayListResetBuffer(void) {
    display_list_buffer = reinterpret_cast<VARIPTR *>(&rndrstream_free);
    display_list_buffer_end = reinterpret_cast<VARIPTR *>(rndrstream_end.addr);
}

// Original 0x295fd7.
static void NuDisplayListCheckBuffer(void) {
}
}

extern "C" void NuRndrInitWorld(void) {
}

extern "C" i32 NuRndrSetViewMtx(NUMTX *vpcs_mtx, NUMTX *viewport_vpc_mtx, NUMTX *scissor_vpc_mtx) {
    return 0;
}

extern "C" void FaceYDirStream(i32 y_angle) {
}

extern "C" void NuRndrPspDraw(void) {
}

extern "C" i32 NuRndrSetFxMtx(NUMTX *matrix) {
    return 1;
}

extern "C" void NuWaterOverride(void) {
}

extern "C" void NuRndrShadowDirCol(const NUVEC *direction, u32 colour, f32 near_distance, f32 far_distance) {
}

extern "C" void NuPolyShadowInit(void) {
}

// Original 0x2967db: present the frame, then pace until the app is active.
extern "C" SAGA_HOST_WEAK i32 NuRndrSwapScreen(i32 /*mode*/) {
    NuRenderThreadLock();
    rndr_blend_shape_deformer_wt_cnt = 0x3f00;
    rndr_blend_shape_deformer_wt_ptrs_cnt = 0x800;
    NuRenderThreadPrepareRender();
    NuShaderManagerBindShader(0);
    NuDebrisRendererFlushBuffers();
    NuDisplayListSwapBuffersEndFrame();
    NuRndrSwapStreamBuffers();
    NuDisplayListSwapBuffersBeginFrame();
    NuDisplayListCheckBuffer();
    NuDisplayListResetBuffer();
    NuRenderThreadUnlock();
    NuRenderThreadStartRender();

    for (;;) {
        if (NuCore::GetApplicationState()->GetStatus() != 1) {
            break;
        }
        g_isBlockedInSwapScreen = 1;
        NuThreadSleep(1);
    }
    g_isBlockedInSwapScreen = 0;
    return 1;
}

// Original 0x296888.
extern "C" i32 NuRndrSwapScreenEx(i32 mode, void (*callback)(void)) {
    if (callback != nullptr) {
        callback();
    }
    return NuRndrSwapScreen(mode);
}
