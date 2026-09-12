#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/motion.h"
#include "legoapi/menus/core/text.h"
#include "gameapi/gui/apimenu.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "legoapi/menus/screens/shop.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/nushader_plain.h"
#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufile.h"

#include <GLES2/gl2.h>
#include <stdio.h>
#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void ClearScreen() {
}

void RenderQuads(i16 *) {
}

void InitAlphaList() {
    char name[16];

    for (i32 i = 0; i < 26; ++i) {
        memset(name, 0, sizeof(name));
        sprintf(name, "shop_%c", i + 'a');
        NuSpecialFind(WORLD->current_gscn, &atoz0to9icon[i], name, 1);
    }

#define FIND_SHOP_DIGIT(index_, digit_)                                                                                \
    memset(name, 0, sizeof(name));                                                                                     \
    sprintf(name, "shop_%c", digit_);                                                                                  \
    NuSpecialFind(WORLD->current_gscn, &atoz0to9icon[index_], name, 1)

    FIND_SHOP_DIGIT(26, '0');
    FIND_SHOP_DIGIT(27, '1');
    FIND_SHOP_DIGIT(28, '2');
    FIND_SHOP_DIGIT(29, '3');
    FIND_SHOP_DIGIT(30, '4');
    FIND_SHOP_DIGIT(31, '5');
    FIND_SHOP_DIGIT(32, '6');
    FIND_SHOP_DIGIT(33, '7');
    FIND_SHOP_DIGIT(34, '8');
    FIND_SHOP_DIGIT(35, '9');

#undef FIND_SHOP_DIGIT
}

f32 GetAspectRatio() {
    return static_cast<f32>(g_backingWidth) / static_cast<f32>(g_backingHeight);
}

static u8 ScreenGrabNeeded;
static i32 pause_rt;
NUMTL *pause_rndr_mtl;
extern i32 pause_rndr_on;
extern i32 pause_fade;
static i32 old_pause_state;
i32 (*PauseRenderOffFn)(void);
i32 cut_waiting_for_new_level;

extern FadeSystem FadeSys;
extern i32 Paused;
extern i32 waiting_for_level;
extern i32 GAMEDEMO;
extern "C" {
    extern f32 MainRenderTime;
    extern i32 back_rgba[2];
    extern i32 clear_screen_onstill;
    extern i32 gone_through_door_to_new_level;
    extern i32 screendump;
}
void BackDrop_Draw(f32, i32);
void DrawStillScreen(i32);

extern "C" i32 NuRndrBeginScene(i32);
extern "C" void NuRndrEndScene(void);
extern "C" void NuBackbufferCopy(i32);
extern "C" void NuRndrClear(i32, i32, f32);
extern "C" void NuRndrGradClear(i32, i32, i32, f32);

void NeedScreenGrab(i32 needed) {
    ScreenGrabNeeded = needed != 0;
}

void ClearStill() {
    old_pause_state = 0;
    Paused = 0;
}

extern f32 CameraZoom;
extern "C" f32 NuIOS_GetAspectRatio(void);

void WidescreenCode(i32) {
    pNuCam->aspect = 1.0f / NuIOS_GetAspectRatio();
    pNuCam->fov = (1.0f / NuIOS_GetAspectRatio() + 0.75f) * 0.5f * (1.0f / CameraZoom);
    SmartTextSetWidescreen(1.3333334f / NuIOS_GetAspectRatio(), 1.0f);
}

void GrabStillScreen() {
    if (ScreenGrabNeeded != 0) {
        ScreenGrabNeeded = 0;
        NuRndrBeginScene(-1);
        NuBackbufferCopy(pause_rt);
        NuRndrEndScene();
        pause_fade = 0;
    }
}

SAGA_HOST_WEAK void InitStillRender(variptr_u *, variptr_u) {
    static NUNATIVETEX nativePauseTex;

    pause_rt = NuTexGenTexture(&nativePauseTex);
    nativePauseTex.ref_count = 1;
    memset(nativePauseTex.checksum, 0, sizeof(nativePauseTex.checksum));
    nativePauseTex.width = g_backingWidth;
    nativePauseTex.height = g_backingHeight;
    nativePauseTex.image_data = NULL;
    nativePauseTex.size = 0;

    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/legoapi.saga/screen.cpp", 0x561);
    glGenTextures(1, &nativePauseTex.platform.gl_tex);
    glActiveTexture(GL_TEXTURE0);
    g_currentTexUnit = 0;
    glBindTexture(GL_TEXTURE_2D, nativePauseTex.platform.gl_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_backingWidth, g_backingHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/legoapi.saga/screen.cpp", 0x56d);

    pause_rndr_mtl = NuMtlCreate3D(1);
    pause_rndr_mtl->sort_pri = 0x7fff;
    pause_rndr_mtl->shader_desc.flags = 0x1000;
    pause_rndr_mtl->opacity = 1.0f;
    u8 *attrib = reinterpret_cast<u8 *>(&pause_rndr_mtl->attribs);
    attrib[1] = static_cast<u8>((attrib[1] & 0x30) | 0xc5);
    attrib[2] = static_cast<u8>((attrib[2] & 0xfc) | 6);
    attrib[0] = static_cast<u8>((attrib[0] & 0xc0) | 0x11);
    attrib[1] = 0xe5;
    pause_rndr_mtl->tex_id = static_cast<i16>(pause_rt);
    pause_rndr_mtl->shader_desc.diffuse_color[0] = -1;
    pause_rndr_mtl->shader_desc.unknown_a8 = 1;
    pause_rndr_mtl->shader_desc.vtx_desc.flags |= 0x40800;
    NuMtlUpdate(pause_rndr_mtl);
    pause_rndr_on = 0;
}

i8 IsGrabbingScreen() {
    return ScreenGrabNeeded;
}

bool LoadShaderSource(char **source, i32 *size, u32 key, bool pixel_stage) {
    static char storage[0x4000];

    *source = NULL;
    *size = 0;

    char path[256];
    sprintf(path, "%s/0x%08x.ios_%s", "builtshaders/ios", key, pixel_stage ? "pcode" : "vcode");

    NUFILE file = NuFileOpen(path, NUFILE_READ);
    if (file == 0) {
        return false;
    }

    *size = NuFileOpenSize(file);
    NuFileRead(file, storage, *size);
    NuFileClose(file);

    if (!pixel_stage) {
        char *precision = strstr(storage, "precision mediump float;");
        if (precision != NULL) {
            const char *replacement = "precision highp float;  ";
            memcpy(precision, replacement, NuStrLen(replacement));
        }
    } else {
        char *precision = strstr(storage, "precision lowp float;");
        if (precision != NULL && strstr(storage, "_envmap_samplerCube") != NULL) {
            const char *replacement = "precision mediump float;";
            memcpy(precision, replacement, NuStrLen(replacement));
        }
    }

    storage[*size] = '\0';
    *source = storage;
    return true;
}

void UpdateCutBorders() {
    f32 target_scale = 1.0f;

    if (CUTSTOPGAME == 0 && (LEGOCAMMODE_DOORCUT == -1 || GameCam->mode != LEGOCAMMODE_DOORCUT)) {
        if (LEGOCAMMODE_OBSTACLE == -1 || GameCam->mode != LEGOCAMMODE_OBSTACLE || ObstacleCamBorders == 0) {
            target_scale = 0.0f;
        }
    }

    CutBorderScale = SeekLinearF(CutBorderScale, target_scale, FRAMETIME * 2.0f);
}

void HandleStillRender() {
    GetMenuID();
    if (pause_rndr_on != 0 && FadeSys.pending_type == FADE_TYPE_NONE) {
        NuRndrBeginScene(-1);
        if (MainRenderTime < 1.0f) {
            if (back_rgba[0] == back_rgba[1]) {
                NuRndrClear(0xf00, back_rgba[0], 1.0f);
            } else {
                NuRndrGradClear(0xf00, back_rgba[0], back_rgba[1], 1.0f);
            }
            BackDrop_Draw(1.0f - MainRenderTime, 0);
        } else {
            NuRndrClear(0x300, 0, 1.0f);
        }
        NuRndrEndScene();
    }

    if (grab_screen_image != 2 && pause_rndr_on != 0 && pause_rt != 0 && FadeSys.pending_type == FADE_TYPE_NONE) {
        DrawStillScreen(clear_screen_onstill);
    }

    if (Paused != 0) {
        if (old_pause_state == 0 && FadeSys.pending_type == FADE_TYPE_NONE) {
            NeedScreenGrab(1);
        }
    }
    if (Paused == 0 && old_pause_state != 0) {
        pause_rndr_on = 0;
    }
    old_pause_state = Paused;

    if ((waiting_for_level == -1 ||
         (gone_through_door_to_new_level == 0 && cut_waiting_for_new_level == 0 && waiting_for_new_level == 0)) &&
        (NewLData != CREDITS_LDATA || LastLData != STATUS_LDATA) &&
        (NewLData != STATUS_LDATA ||
         ((WORLD->level_sub_id == -1 || (WORLD->area[WORLD->level_sub_id].flags & 2) == 0) && GAMEDEMO == 0)) &&
        NewLData != CREDITS_LDATA &&
        (NewLData != HUB_LDATA || WORLD->level_sub_id == -1 || (WORLD->area[WORLD->level_sub_id].flags & 2) == 0) &&
        !(MainRenderTime > 0.0f && MainRenderTime < 1.0f)) {
        if (Paused == 0 || IsGrabbingScreen() != 0) {
            pause_rndr_on = static_cast<u32>(grab_screen_image) > 1;
        } else if (PauseRenderOffFn != NULL && PauseRenderOffFn() != 0) {
            pause_rndr_on = 0;
        } else {
            pause_rndr_on = 1;
        }
    } else {
        pause_rndr_on = 1;
    }

    if (screendump != 0) {
        pause_rndr_on = 0;
    }
    clear_screen_onstill = 1;
    grab_screen_image = 0;
}

void PreRenderFlashHack() {
}

void UCStretchToCorners(i16 *, i16 *) {
}

void PostRenderFlashHack() {
}

bool LookupPreloadedShaderObject(u32 key, u32 **shader, LoadedUniqueShaderRecord *records, u32 count) {
    i32 upper = static_cast<i32>(count) - 1;
    if (upper < 0) {
        return false;
    }

    i32 index = upper / 2;
    LoadedUniqueShaderRecord *record = &records[index];
    if (record->key == key) {
        *shader = &record->gl_shader;
        return true;
    }

    i32 lower = 0;
    while (true) {
        if (key > record->key) {
            lower = index + 1;
        } else {
            upper = index - 1;
        }
        if (lower > upper) {
            return false;
        }

        index = (lower + upper) / 2;
        record = &records[index];
        if (record->key == key) {
            *shader = &record->gl_shader;
            return true;
        }
    }
}
