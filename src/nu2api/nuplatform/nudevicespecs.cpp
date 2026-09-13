#include "nudevicespecs.hpp"

#include <GLES2/gl2.h>
#include <cstdlib>
#include <cstring>

#include "globals.h"
#include "nu2api/nucore/nustring.h"

NuDeviceSpecs *NuDeviceSpecs::ms_instance = NULL;

NuDeviceSpecs::NuDeviceSpecs() : specs(-2) {
    DetermineDeviceSpecs();
}

void NuDeviceSpecs::Create() {
    if (ms_instance != NULL) {
        return;
    }

    ms_instance = (NuDeviceSpecs *)malloc(sizeof(NuDeviceSpecs));
    ms_instance->DetermineDeviceSpecs();
}

void NuDeviceSpecs::Exists() {
}

void NuDeviceSpecs::Destroy() {
    if (ms_instance != NULL) {
        free(ms_instance);
        ms_instance = NULL;
    }
}

NuDeviceSpecs::~NuDeviceSpecs() {
}

void NuDeviceSpecs::DetermineDeviceSpecs() {
    char *vendor = (char *)glGetString(GL_VENDOR);
    char *renderer = (char *)glGetString(GL_RENDERER);
    char *version = (char *)glGetString(GL_VERSION);

    LOG_DEBUG("vendor: %s, renderer: %s, version: %s", vendor, renderer, version);

    glGetString(GL_SHADING_LANGUAGE_VERSION);

    this->specs = 3;

    bool bVar5 = false;
    if (NuStrIStr(renderer, "Adreno") != NULL) {
        bVar5 = NuStrIStr(renderer, "320") != NULL;
    }

    if (strstr(version, " 1.") != NULL) {
        this->specs = -1;
        return;
    }

    if (strstr(version, " 3.") == NULL) {
        if (strstr(version, " 2.") == NULL && !bVar5) {
            this->specs = 3;
            return;
        }
    } else if (!bVar5) {
        this->specs = 3;
        if (NuStrIStr(vendor, "NVIDIA") == NULL) {
            return;
        }
        if (NuStrIStr(renderer, "Tegra") == NULL) {
            return;
        }
        g_forceSysMemVbs = 1;
        return;
    }

    this->specs = 2;
    if (NuStrIStr(vendor, "Qualcomm") != NULL) {
        this->specs = 0;
        if (NuStrIStr(renderer, "Adreno") == NULL) {
            return;
        }
        if (NuStrIStr(renderer, "320") != NULL) {
            this->specs = 2;
        }
        if (NuStrIStr(renderer, "330") == NULL) {
            return;
        }
        this->specs = 2;
        return;
    }

    if (NuStrIStr(vendor, "Imagination") == NULL) {
        if (NuStrIStr(vendor, "NVIDIA") != NULL) {
            this->specs = 0;
            vendor = NuStrIStr(renderer, "Tegra");
            if (vendor == NULL) {
                return;
            }
            this->specs = 3;
            i32 iVar4 = NuStrICmp(renderer, "NVIDIA Tegra 3");
            if ((iVar4 != 0) && (iVar4 = NuStrICmp(renderer, "NVIDIA Tegra 2"), iVar4 != 0)) {
                return;
            }
            goto LAB_0010151e;
        }

        if (NuStrIStr(vendor, "ARM") == NULL) {
            return;
        }
        this->specs = 0;
        if (NuStrIStr(renderer, "Mali-T628") != NULL) {
            this->specs = 0;
        }
        vendor = "Mali-400";
    } else {
        this->specs = 0;
        vendor = "544MP";
    }

    vendor = NuStrIStr(renderer, vendor);
    if (vendor == NULL) {
        return;
    }

LAB_0010151e:
    this->specs = 0;
    return;
}
