#pragma once

#include <stddef.h>

typedef enum PLATFORMS_SUPPORTED_e {
    DEFAULT_PLATFORM = 0,
    XBOX_PLATFORM = 1,
    GAMECUBE_PLATFORM = 2,
    PSP_PLATFORM = 3,
    PC_PLATFORM = 4,
    PS3_PLATFORM = 5,
    X360_PLATFORM = 6,
    WII_PLATFORM = 7,
    IOS_PLATFORM = 8,
    ANDROID_ATITC_PLATFORM = 9,
    ANDROID_PVRTC_PLATFORM = 10,
    ANDROID_S3TC_PLATFORM = 11,
    ANDROID_ETC1_PLATFORM = 12,
    NUM_PLATFORMS_SUPPORTED = 13
} PLATFORMS_SUPPORTED;

#ifdef __cplusplus
extern "C" {
#endif
    extern char *g_fontExtension;
    extern char *g_platformName;
#ifdef __cplusplus
}

class NuPlatform {
  private:
    static NuPlatform *ms_instance;

    char *name;
    char *texture_extension;
    char *font_extension;
    PLATFORMS_SUPPORTED current_platform;

  public:
    NuPlatform();
    ~NuPlatform();

    static NuPlatform *Get(void) {
        return NuPlatform::ms_instance;
    }

    static void Create(void);
    void Destroy();
    void Exists();

    char *GetCurrentTextureExtension() const {
        return this->texture_extension;
    }

    PLATFORMS_SUPPORTED GetCurrentPlatform() const {
        return this->current_platform;
    }

    void SetCurrentPlatform(PLATFORMS_SUPPORTED platform);
};

#endif
