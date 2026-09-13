#include "legogame/target.h"

#include <stdarg.h>

#include "batman.h"
#include "globals.h"
#include "legogame/game.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nuandroid/nuphoneos.h"
#include "nu2api/nucore/nuapi.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/nucore/nuvideo.h"
#include "nu2api/nusound/nusound.h"

extern "C" {
    void *DVD = NULL;

    struct nupad_s;
    nupad_s *Game_NuPad_Store[2];

    nupad_s **Game_NuPad;

};

i32 app_tbgameset;
i32 app_tbplayerset;
i32 app_tbaiset;
i32 app_tbdrawset;

static u8 s_SystemPausedTracks[2];

static void TouchCallback(const NuPhoneOSMessageData *message) {
    NuPad_Interface_TouchScreenInput(
        message->touch_id, message->x, message->y, message->pressure, message->touch_action == NUPHONE_TOUCH_DOWN,
        message->touch_action == NUPHONE_TOUCH_UP, message->touch_action == NUPHONE_TOUCH_MOVE,
        message->touch_action == NUPHONE_TOUCH_CANCEL);
}

static void SystemPauseCallback(const NuPhoneOSMessageData *) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    const i32 dropin_context = LEGOCONTEXT_DROPIN;
    LEVELDATA_s *title_level = TITLES_LDATA;
    const f32 autosave_post_delay = memcard_autosavepostdelay;
    const f32 autosave_pre_delay = memcard_autosavepredelay;
    const i32 new_mode = NewMode;
    const i32 menu_level = GameMenuLevel;
    LEVELDATA_s *new_level = NewLData;
    const f32 fade = FadeSys.fade;
    const i32 cutscene_waiting = CutSceneWaiting;
    const i32 editor_is_active = editor_active;
    const i32 cutscene_stops_game = CUTSTOPGAME;
    const f32 game_time = GameTimer.time_elapsed;
    const i32 timer_updates = GameTimer.update_count;
    const i32 mini_cut_camera = MiniCutCam;
    const i32 game_is_paused = Paused;
    const i32 autosave_started = memcard_autosavestarted;
    GameObject_s *pause_player;

    if (game_is_paused == 0 && timer_updates != 0 && autosave_pre_delay <= 0.0f && new_mode == 0 &&
        (((pause_player = Player[0]) != NULL && static_cast<i8>(pause_player->apiobj.field_0x1f8) < 0 &&
          (dropin_context == -1 || dropin_context != static_cast<i8>(pause_player->field_0x7a5)) && new_level == NULL &&
          fade == 0.0f && editor_is_active == 0 && game_time > 0.0f && world != NULL &&
          world->current_level != title_level && GameMenu[menu_level].menu == -1 && cutscene_waiting == 0 &&
          cutscene_stops_game == 0 && mini_cut_camera == 0 && autosave_started == 0 && autosave_post_delay <= 0.0f) ||
         ((pause_player = Player[1]) != NULL && static_cast<i8>(pause_player->apiobj.field_0x1f8) < 0 &&
          (dropin_context == -1 || dropin_context != static_cast<i8>(pause_player->field_0x7a5)) && new_level == NULL &&
          fade == 0.0f && editor_is_active == 0 && game_time > 0.0f && world != NULL &&
          world->current_level != title_level && GameMenu[menu_level].menu == -1 && cutscene_waiting == 0 &&
          cutscene_stops_game == 0 && mini_cut_camera == 0 && autosave_started == 0 && autosave_post_delay <= 0.0f))) {
        PauseGame(pause_player->pad_gamepad - GamePad);
    }

    if (NuSound3GetStereoStreamStatus(0) == NUSOUND_STEREO_STREAM_PLAYING) {
        s_SystemPausedTracks[0] = 1;
        NuSound3PauseStereoStream(0);
    }
    if (NuSound3GetStereoStreamStatus(1) == NUSOUND_STEREO_STREAM_PLAYING) {
        s_SystemPausedTracks[1] = 1;
        NuSound3PauseStereoStream(1);
    }
}

static void SystemDidBecomeActiveCallback(const NuPhoneOSMessageData *) {
    if (s_SystemPausedTracks[0] != 0) {
        NuSound3ResumeStereoStream(0);
        s_SystemPausedTracks[0] = 0;
    }
    if (s_SystemPausedTracks[1] != 0) {
        NuSound3ResumeStereoStream(1);
        s_SystemPausedTracks[1] = 0;
    }
    NuPad_Interface_ResetAllTouches();
}

void InitOnce(i32 argc, char **param_2) {
    NuPhoneOSRegisterEventCallback(PHONE_EVENT_TOUCH, TouchCallback);
    NuPhoneOSRegisterEventCallback(PHONE_EVENT_PAUSE, SystemPauseCallback);
    NuPhoneOSRegisterEventCallback(PHONE_EVENT_BECOME_ACTIVE, SystemDidBecomeActiveCallback);

    if (NuIOS_IsLowEndDevice()) {
        SUPERBUFFERSIZE -= 0x38370;
    }

    i32 size = SUPERBUFFERSIZE;

    permbuffer_base.void_ptr = NU_ALLOC(size, 4, 1, "", NUMEMORY_CATEGORY_NONE);
    superbuffer_end.void_ptr = (void *)(SUPERBUFFERSIZE + (usize)permbuffer_base.void_ptr);
    original_permbuffer_base.void_ptr = permbuffer_base.void_ptr;
    InitGameBeforeConfig();

    Game_NuPad = &Game_NuPad_Store[0];

#define SETUP(cmd, ...) cmd, ##__VA_ARGS__

    NuInitHardware(&permbuffer_base, &superbuffer_end, 0,                      //
                   SETUP(NUAPI_SETUP_HOSTFS, 0),                               //
                   SETUP(NUAPI_SETUP_SWAPMODE, NUVIDEO_SWAPMODE_ASYNC),        //
                   SETUP(NUAPI_SETUP_STREAMSIZE, 0x200000),                    //
                   SETUP(NUAPI_SETUP_VIDEOMODE, (PAL == 0) ? 0xdeadbeef : 8),  //
                   SETUP(NUAPI_SETUP_RESOLUTION, 512, (PAL == 0) ? 224 : 256), //
                   SETUP(NUAPI_SETUP_GLASSRPLANE, 1),                          //
                   SETUP(NUAPI_SETUP_CDDVDMODE, &DVD),                         //
                   SETUP((NOSOUND == 0) ? NUAPI_SETUP_AUDIO : NUAPI_SETUP_AUDIO_DISABLED, 0, 0x640, 0, 0),
                   SETUP(NUAPI_SETUP_PAD0, &Game_NuPad_Store[0]), //
                   SETUP(NUAPI_SETUP_PAD1, &Game_NuPad_Store[1]), //
                   SETUP(NUAPI_SETUP_0x46, 1),                    //
                   SETUP(NUAPI_SETUP_0x47, 1),                    //
                   SETUP(NUAPI_SETUP_0x49, 1),                    //
                   SETUP(NUAPI_SETUP_0x4b, 1),                    //
                   NUAPI_SETUP_END                                //
    );

    pNuCam = NuCameraCreate();
    Game.options_save.field11_0xb = static_cast<u8>(NuIOS_IsWidescreen());
    WidescreenCode(Game.options_save.field11_0xb);
    InitPanel(Game.options_save.field11_0xb);

    app_tbgameset = NuTimeBarCreateSet(0);
    app_tbplayerset = NuTimeBarCreateSet(0);
    app_tbaiset = NuTimeBarCreateSet(0);
    app_tbdrawset = NuTimeBarCreateSet(0);
}
