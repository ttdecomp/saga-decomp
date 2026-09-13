#include <stdlib.h>

#include "nu2api/nucore/common.h"

#include "nu2api/nuplatform/nuplatform.h"

NuPlatform *NuPlatform::ms_instance = NULL;

void NuPlatform::Create(void) {
    if (ms_instance == NULL) {
        ms_instance = (NuPlatform *)malloc(sizeof(NuPlatform));
        ms_instance->current_platform = DEFAULT_PLATFORM;
    }
}

char *g_fontExtension = NULL;
char *g_platformName = NULL;

void NuPlatform::SetCurrentPlatform(PLATFORMS_SUPPORTED platform) {
    this->current_platform = platform;

    switch (platform) {
        case DEFAULT_PLATFORM:
            this->name = "DEFAULT_PLATFORM";
            break;
        case XBOX_PLATFORM:
            this->name = "XBOX_PLATFORM";
            break;
        case GAMECUBE_PLATFORM:
            this->name = "GAMECUBE_PLATFORM";
            break;
        case PSP_PLATFORM:
            this->name = "PSP_PLATFORM";
            break;
        case PC_PLATFORM:
            this->name = "PC_PLATFORM";
            break;
        case PS3_PLATFORM:
            this->name = "PS3_PLATFORM";
            break;
        case X360_PLATFORM:
            this->name = "X360_PLATFORM";
            break;
        case WII_PLATFORM:
            this->name = "WII_PLATFORM";
            break;
        case IOS_PLATFORM:
            this->name = "IOS_PLATFORM";
            break;
        case ANDROID_ATITC_PLATFORM:
            this->name = "ANDROID_ATITC_PLATFORM";
            break;
        case ANDROID_PVRTC_PLATFORM:
            this->name = "ANDROID_PVRTC_PLATFORM";
            break;
        case ANDROID_S3TC_PLATFORM:
            this->name = "ANDROID_S3TC_PLATFORM";
            break;
        case ANDROID_ETC1_PLATFORM:
            this->name = "ANDROID_ETC1_PLATFORM";
            break;
        case NUM_PLATFORMS_SUPPORTED:
            this->name = "NUM_PLATFORMS_SUPPORTED";
            break;
        default:
            this->name = NULL;
    }
    if (platform == ANDROID_PVRTC_PLATFORM) {
        this->name = "IOS";
    LAB_00101158:
        g_fontExtension = "ANDROID_PVRTC_FNT";
        this->texture_extension = "ANDROID_PVRTC_TEX";
        this->font_extension = "ANDROID_PVRTC_FNT";
    } else {
        if (platform == ANDROID_ATITC_PLATFORM) {
            g_fontExtension = "ANDROID_ATITC_FNT";
            this->texture_extension = "ANDROID_ATITC_TEX";
            this->font_extension = "ANDROID_ATITC_FNT";
            goto LAB_001010e6;
        }

        if (8 < (i32)platform) {
            if (platform == ANDROID_PVRTC_PLATFORM)
                goto LAB_00101158;
            if (platform == ANDROID_S3TC_PLATFORM) {
                g_fontExtension = "ANDROID_S3TC_FNT";
                this->texture_extension = "ANDROID_S3TC_TEX";
                this->font_extension = "ANDROID_S3TC_FNT";
                goto LAB_001010e6;
            }
            if (platform == ANDROID_ETC1_PLATFORM) {
                g_fontExtension = "ANDROID_ETC1_FNT";
                this->texture_extension = "ANDROID_ETC1_TEX";
                this->font_extension = "ANDROID_ETC1_FNT";
                goto LAB_001010e6;
            }
        }
        g_fontExtension = "fnt";
        this->font_extension = "fnt";
        this->texture_extension = "TEX";
    }

LAB_001010e6:
    g_platformName = this->name;
}
