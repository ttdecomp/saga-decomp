#include "nu2api/nucore/nuonline.h"
#include "nu2api/nucore/nupad.h"
// This translation unit owns the header's six VuVec statics in the original ELF.
#include "nu2api/nucore/nuvuvec.hpp"

void NuOnlineResetProfiles() {
    g_signedinUser = -1;
}

void NuOnlineSetContextProfilePS(i32, i32, i32) {}
void NuOnlineSetPropertyProfilePS(i32, i32, i32, void *) {}
void NuOnlineSetPresenceModeProfilePS(i32, i32) {}
i32 NuOnlineAchievementAchievedProfile(i32, i32, NUONLINEACHIEVEMENTCALLBACK) { return 0; }
void NuOnlineSetDefaultContextProfilePS(i32, i32, i32) {}
void NuOnlineSetDefaultPresenceModeProfilePS(i32, i32) {}

extern "C" {
    i32 NuOnlineAchievementAchievedExPS(i32 player, i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback) {
        if ((u32)player > 2) return 0;
        return NuOnlineAchievementAchievedProfile(g_nupadMapping[player].port, achievement, callback);
    }
    i32 NuOnlineAchievementAchievedPS(i32 achievement, NUONLINEACHIEVEMENTCALLBACK callback) {
        return NuOnlineAchievementAchievedProfile(g_signedinUser, achievement, callback);
    }
    i32 NuOnlineHasPlayerDownloadedPS(u32) { return 0; }
    i32 NuOnlineHasPlayerSignedInExPS(void) { return 0; }
    i32 NuOnlineHasPlayerSignedInPS(void) { return g_signedinUser != -1; }
    void NuOnlineInitPS(void) {}
    void NuOnlineSetContextExPS(i32 player, i32 context, i32 value) {
        if ((u32)player <= 2) NuOnlineSetContextProfilePS(g_nupadMapping[player].port, context, value);
    }
    void NuOnlineSetContextPS(i32 context, i32 value) { NuOnlineSetContextProfilePS(g_signedinUser, context, value); }
    void NuOnlineSetDefaultContextExPS(i32 player, i32 context, i32 value) {
        if ((u32)player <= 2) NuOnlineSetDefaultContextProfilePS(g_nupadMapping[player].port, context, value);
    }
    void NuOnlineSetDefaultContextPS(i32 context, i32 value) {
        NuOnlineSetDefaultContextProfilePS(g_signedinUser, context, value);
    }
    void NuOnlineSetDefaultPresenceModeExPS(i32 player, i32 mode) {
        if ((u32)player <= 2) NuOnlineSetDefaultPresenceModeProfilePS(g_nupadMapping[player].port, mode);
    }
    void NuOnlineSetDefaultPresenceModePS(i32 mode) {
        NuOnlineSetDefaultPresenceModeProfilePS(g_signedinUser, mode);
    }
    void NuOnlineSetPresenceModeExPS(i32 player, i32 mode) {
        if ((u32)player <= 2) NuOnlineSetPresenceModeProfilePS(g_nupadMapping[player].port, mode);
    }
    void NuOnlineSetPresenceModePS(i32 mode) { NuOnlineSetPresenceModeProfilePS(g_signedinUser, mode); }
    void NuOnlineSetProfilePlayer(void) {}
    void NuOnlineSetPropertyExPS(i32 player, i32 property, i32 size, void *data) {
        if ((u32)player <= 2) NuOnlineSetPropertyProfilePS(g_nupadMapping[player].port, property, size, data);
    }
    void NuOnlineSetPropertyPS(i32 property, i32 size, void *data) {
        NuOnlineSetPropertyProfilePS(g_signedinUser, property, size, data);
    }
    i32 NuOnlineSignInPlayerPS(void) { return 0; }
}
