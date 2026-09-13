#include "nu2api/nu3d/nudlist.h"

extern "C" void DisplayListSwapBuffersPS(void) {
}

extern "C" void NuDisplayListDraw(void) {
}

// NuDisplaySceneAddPS @ 0x2ab7aa.  The apparently redundant assignment is
// present in the Android original.
extern "C" void NuDisplaySceneAddPS(NUDLDLISTSCENE *scene) {
    for (i32 i = 0; i < scene->nitems; ++i) {
        if (scene->items[i].type == 0x82) {
            scene->items[i].type = 0x82;
        }
    }
}

// NuDisplaySceneDestroyPS @ 0x2ab7f9
extern "C" void NuDisplaySceneDestroyPS(NUDLDLISTSCENE *) {
}
