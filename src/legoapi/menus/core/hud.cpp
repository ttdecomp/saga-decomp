#include "MechInputTouch/MechInputTouch_types.h"
void RndrTexQuad(f32, f32, f32, f32, i32, numtl_s *, i32);
#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/render/core/render.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nutrig.h"

struct spacelevel_s;

extern FadeSystem FadeSys;
extern f32 statstime;
extern f32 cointotaltime;

void CoinTotal_Draw(i32 total, f32 y, f32 scale, i32 remember_positions, f32 icon_phase, i32 red, i32 green, i32 blue);
void DrawSuperStoryTime(f32 x, f32 timer, f32 target, i32 flags, i32 draw_icon);
void Text_MakeScore(u32 score, char *text);

HudRadarPulse::HudRadarPulse(VuVec const &initial_position) : position(initial_position) {
    pulses[1].delay = 0.2f;
    pulses[2].delay = 0.4f;

    pulses[0].delay_finished = 0;
    pulses[0].finished = 0;
    pulses[0].angle = 0;
    pulses[0].delay = 0.0f;
    pulses[1].delay_finished = 0;
    pulses[0].radius = 0.0f;
    pulses[1].finished = 0;
    pulses[0].speed = 0.75f;
    pulses[1].angle = 0;
    pulses[1].radius = 0.0f;
    pulses[2].delay_finished = 0;
    pulses[1].speed = 0.75f;
    pulses[2].finished = 0;
    pulses[2].radius = 0.0f;
    pulses[2].angle = 0;
    pulses[2].speed = 0.75f;

    active = 1;
    paused = 0;
}

i32 HudRadarPulse::IsFinished() {
    if (pulses[0].delay_finished && pulses[0].finished && pulses[1].delay_finished && pulses[1].finished &&
        pulses[2].delay_finished)
        return pulses[2].finished;
    return 0;
}

extern i32 NewMode;
extern i32 editor_active;
extern i32 CutSceneWaiting;
extern "C" i32 Paused;
void HudRadarPulse::Process(float delta) {
    if (active) {
        active = 0;
        if (NewMode == 0 && NewLData == NULL && editor_active == 0 && GameTimer.time_elapsed > 0.0f &&
            GameTimer.update_count != 0 && WORLD != NULL && CutSceneWaiting == 0 &&
            (Paused == 0 || GetMenuID() == 0x19 || GetMenuID() == 0x15) &&
            (CUTSTOPGAME == 0 || CutScene_IsSkippable(static_cast<CUTINFO *>(CutStopInfo))) && MiniCutCam == 0 &&
            (paused || (WORLD->current_level != TITLES_LDATA && WORLD->current_level != STATUS_LDATA &&
                        !(WORLD->current_level->flags & 0x400) && memcard_autosavestarted == 0 &&
                        !(memcard_autosavepostdelay > 0.0f) && !(memcard_autosavepredelay > 0.0f))))
            active = 1;
    }
    for (i32 i = 0; i < 3; ++i) {
        HudRadarPulseStage &pulse = pulses[i];
        if (!pulse.delay_finished) {
            pulse.delay -= delta;
            if (pulse.delay <= 0.0f)
                pulse.delay_finished = 1;
        }
        if (pulse.delay_finished && !pulse.finished) {
            pulse.radius += pulse.speed * delta;
            float speed = pulse.speed - 0.4f * delta;
            pulse.speed = speed > 0.0f ? speed : 0.0f;
            pulse.angle += static_cast<i32>(32768.0f * delta);
            if (pulse.angle > 0x7fff)
                pulse.finished = 1;
        }
    }
}

void HudRadarPulse::Render() {
    if (active) {
        if (pulses[0].delay_finished && !pulses[0].finished) {
            f32 x = position.x + 1.0f;
            f32 y = 1.0f - position.y;
            numtl_s *material = MechSystems::Get()->radar_pulse_material;
            i32 alpha = static_cast<i32>(90.0f * NU_SIN_LUT(pulses[0].angle));
            f32 radius = pulses[0].radius;
            f32 width = GetAspectRatio() * radius;
            RndrTexQuad(x * 0.5f, y * 0.5f, width, radius, static_cast<i32>((static_cast<u32>(alpha) << 24) | 0x808080),
                        material, 0);
        }
        if (pulses[1].delay_finished && !pulses[1].finished) {
            f32 x = position.x + 1.0f;
            f32 y = 1.0f - position.y;
            numtl_s *material = MechSystems::Get()->radar_pulse_material;
            i32 alpha = static_cast<i32>(90.0f * NU_SIN_LUT(pulses[1].angle));
            f32 radius = pulses[1].radius;
            f32 width = GetAspectRatio() * radius;
            RndrTexQuad(x * 0.5f, y * 0.5f, width, radius, static_cast<i32>((static_cast<u32>(alpha) << 24) | 0x808080),
                        material, 0);
        }
        if (pulses[2].delay_finished && !pulses[2].finished) {
            f32 x = position.x + 1.0f;
            f32 y = 1.0f - position.y;
            numtl_s *material = MechSystems::Get()->radar_pulse_material;
            i32 alpha = static_cast<i32>(90.0f * NU_SIN_LUT(pulses[2].angle));
            f32 radius = pulses[2].radius;
            f32 width = GetAspectRatio() * radius;
            RndrTexQuad(x * 0.5f, y * 0.5f, width, radius, static_cast<i32>((static_cast<u32>(alpha) << 24) | 0x808080),
                        material, 0);
        }
    }
}

static __used__ void RefreshUI() {
}

static __used__ void DrawSpaceLevel(spacelevel_s *) {
}

static __used__ void DrawEpisodesMenu(int, float) {
}

namespace {
    void _NuTimeBarSlotBegin(void) {
    }

    void _NuTimeBarSlotEnd(void) {
    }
} // namespace
