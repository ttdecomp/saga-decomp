#include "legoapi/core/startup/main.h"
#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/levels/levels.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/android/NuThread_android.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" {
    extern i32 FinishLoop_On;
    i32 NuRndrBeginScene(i32 flags);
    void NuRndrClear(i32 flags, i32 colour, f32 depth);
    void NuRndrEndScene(void);
    f32 NuFrameEnd(void);
    void edGraEnableTerrainSwap(void);
    void edGraDisableTerrainSwap(void);
}

void BackDrop_Draw(f32 alpha, i32 flags);
void NewGame(void);

void FinishLoop(i32 frames) {
    if (!FinishLoop_On) {
        return;
    }

    if (finishloop_backdroponly != 0) {
        pNuCam->mtx = numtx_identity;
        NuCameraSet(pNuCam);

        do {
            NuFrameBegin();
            NuRndrBeginScene(-1);
            NuRndrClear(0xb00, 0, 1.0f);
            BackDrop_Draw(1.0f, 1);
            NuRndrEndScene();
            edGraEnableTerrainSwap();
            NuFrameEnd();
            edGraDisableTerrainSwap();
        } while (--frames > 0);
        finishloop_backdroponly = 0;
        return;
    }

    if (FadeSys.pending_type == FADE_TYPE_NONE) {
        do {
            NuFrameBegin();
            NuRndrBeginScene(-1);
            NuRndrClear(0xb00, 0, 1.0f);
            NuRndrEndScene();
            edGraEnableTerrainSwap();
            NuFrameEnd();
            edGraDisableTerrainSwap();
        } while (--frames > 0);
    } else {
        do {
            NuFrameBegin();
            FadeSys.Draw();
            edGraEnableTerrainSwap();
            NuFrameEnd();
            edGraDisableTerrainSwap();
        } while (--frames > 0);
    }
}

void bgSuspendMain(i32) {
}

void EndOfDemo(i32) {
    NewLData = TITLES_LDATA;
    new_level_from_menu = 1;
    NewGame();
}
