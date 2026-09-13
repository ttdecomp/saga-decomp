#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "gameapi/edtools/edstubs.h"
#include "globals.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nucamera.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern NUGLOBALRNDRSTATE render_state;
extern i32 back_rgba[2];
extern f32 MainRenderTime;
void BackDrop_Draw(f32 alpha, i32 flags);
extern "C" {
    extern i32 DEBPAGE_AREA;
    extern i32 DEBPAGE_CHARACTER;
    extern i32 DEBPAGE_GENERAL;
    void DebFreeAllCreatedEffects(void);
    void DebrisSetRenderGroup(i32 group);
    i32 NuRndrBeginScene(i32 flags);
    void NuRndrClear(u32 flags, u32 colour, f32 depth);
    void NuRndrGradClear(i32 flags, i32 top_colour, i32 bottom_colour, f32 depth);
    void NuRndrEndScene(void);
}

void OctreeRndr(unsigned char *, nuoctreenode_s *, i32) {
}

void AddCameraRain(WORLDINFO_s *, i32) {
}

void Particles_Stop(WORLDINFO_s *world) {
    if (world->page_anim != -1) {
        edanimStopPage(world->page_anim);
    }
    if (world->page_pp != -1) {
        edppStopPage((i8)world->page_pp);
    }
    if (DEBPAGE_CHARACTER != -1) {
        edppStopPage((i8)DEBPAGE_CHARACTER);
    }
    if (DEBPAGE_GENERAL != -1) {
        edppStopPage((i8)DEBPAGE_GENERAL);
    }
    if (DEBPAGE_AREA != -1) {
        edppStopPage((i8)DEBPAGE_AREA);
    }
    DebFreeAllCreatedEffects();
}

void Particles_Start(WORLDINFO_s *world) {
    if (DEBPAGE_AREA != -1) {
        edppStartPage((i8)DEBPAGE_AREA);
    }
    if (world->page_pp != -1) {
        edppStartPage((i8)world->page_pp);
    }
    DebrisSetRenderGroup(1);
    if (world->page_anim >= 0) {
        edanimStartPage(world->page_anim);
    }
}

void Particles_DumpAreaPage() {
}

void Particles_LoadAreaPage(char *) {
}

void NoRender() {
    pNuCam->mtx = numtx_identity;
    NuCameraSet(pNuCam);
    NuRndrBeginScene(-1);

    if (back_rgba[0] == back_rgba[1]) {
        NuRndrClear(0xf00, back_rgba[0], 1.0f);
    } else {
        NuRndrGradClear(0xf00, back_rgba[0], back_rgba[1], 1.0f);
    }

    if (MainRenderTime != 1.0f) {
        BackDrop_Draw(1.0f - MainRenderTime, 0);
    }
    NuRndrEndScene();
}
