#include "nuscreen.hpp"

#include <cstddef>
#include <cstdlib>

NuScreen *NuScreen::ms_instance = NULL;

bool NuScreen::Exists() {
}

void NuScreen::Create() {
    if (ms_instance != NULL) {
        return;
    }

    ms_instance = (NuScreen *)malloc(sizeof(NuScreen));
}

void NuScreen::SetSceeenDimensions(f32 width, f32 height) {
    this->width = width;
    this->height = height;
}

void NuScreen::Destroy() {
    if (ms_instance != NULL) {
        free(ms_instance);
        ms_instance = NULL;
    }
}

NuScreen::NuScreen() {
    width = 0.0f;
    height = 0.0f;
}

NuScreen::~NuScreen() {
}
