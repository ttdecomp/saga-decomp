#pragma once

#include "nu2api/nucore/common.h"

typedef void (*NUONLINEACHIEVEMENTCALLBACK)(i32, i32);
extern "C" i32 g_signedinUser;
void NuOnlineResetProfiles();
i32 NuOnlineAchievementAchievedProfile(i32 profile, i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback);
void NuOnlineSetPresenceModeProfilePS(i32 profile, i32 mode);
void NuOnlineSetDefaultPresenceModeProfilePS(i32 profile, i32 mode);
void NuOnlineSetContextProfilePS(i32 profile, i32 context, i32 value);
void NuOnlineSetDefaultContextProfilePS(i32 profile, i32 context, i32 value);
void NuOnlineSetPropertyProfilePS(i32 profile, i32 property, i32 size, void *data);

extern "C" {
    void NuOnlineSetContextExPS(i32 player, i32 context, i32 value);
    void NuOnlineSetDefaultContextExPS(i32 player, i32 context, i32 value);
    void NuOnlineSetPropertyExPS(i32 player, i32 property, i32 size, void *data);
    void NuOnlineSetPresenceModeExPS(i32 player, i32 mode);
    void NuOnlineSetDefaultPresenceModeExPS(i32 player, i32 mode);
    void NuOnlineSetPropertyPS(i32 property, i32 size, void *data);
    void NuOnlineSetContextPS(i32 context, i32 value);
    void NuOnlineSetDefaultContextPS(i32 context, i32 value);
    void NuOnlineSetPresenceModePS(i32 mode);
    void NuOnlineSetDefaultPresenceModePS(i32 mode);
    void NuOnlineSetProfilePlayer(void);
    i32 NuOnlineAchievementAchieved(i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback);
    i32 NuOnlineAchievementAchievedPS(i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback);
    i32 NuOnlineAchievementAchievedEx(i32 player, i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback);
    i32 NuOnlineAchievementAchievedExPS(i32 player, i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback);
    void NuOnlineSetContext(void);
    void NuOnlineSetDefaultContext(void);
    i32 NuOnlineSignInPlayerPS(void);
    void NuOnlineInit(void);
    void NuOnlineInitPS(void);
    i32 NuOnlineHasPlayerSignedIn(void);
    i32 NuOnlineHasPlayerSignedInPS(void);
    i32 NuOnlineHasPlayerSignedInEx(void);
    i32 NuOnlineHasPlayerSignedInExPS(void);
    // The original forwards one 32-bit argument; its meaning is not yet recovered.
    i32 NuOnlineHasPlayerDownloaded(u32 argument);
    i32 NuOnlineHasPlayerDownloadedPS(u32 argument);
}
