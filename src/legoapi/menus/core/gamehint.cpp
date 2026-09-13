#include "legoapi/legoapi_types.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "globals.h"
#include "legoapi/world/world.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/render/core/render.h"
#include "gamelib/util/gamelib_util_types.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/nucore/nustring.h"
#include "legoapi/world/mission.h"

u32 LSW_HintConditions;
HINTSYS_s hintsys = {};
HINTUIBUTTON_s *hintUIButton = NULL;
f32 AlphaBlendTime = 1.0f;
extern f32 ICONX;
f32 ICONXPOS_TOUCHYFEELY = -ICONX;
f32 ICONYPOS_TOUCHYFEELY = -0.699999988f;
f32 ICONXPOS_VIRTUALS = -0.61500001f;
f32 ICONYPOS_VIRTUALS = 0.47f;

struct HintScalarTransition {
    f32 *target;
    f32 from, to, elapsed, duration, delay, value;
    HintScalarTransition() : target(&value), elapsed(0.0f), duration(-1.0f), delay(0.0f) {
    }
    void Update(f32 dt) {
        if (!(duration < 0.0f) && !(elapsed >= duration + delay)) {
            elapsed += dt;
            if (elapsed > duration + delay)
                elapsed = duration + delay;
            if (elapsed >= delay)
                *target = ((elapsed - delay) / duration) * (to - from) + from;
        }
    }
};
struct HintVectorTransition {
    VuVec *target;
    VuVec from, to;
    f32 elapsed, duration, delay;
    VuVec value;
    HintVectorTransition() : target(&value), elapsed(0.0f), duration(-1.0f), delay(0.0f) {
    }
    void Update(f32 dt) {
        if (!(duration < 0.0f) && !(elapsed >= duration + delay)) {
            elapsed += dt;
            if (elapsed > duration + delay)
                elapsed = duration + delay;
            if (elapsed >= delay) {
                const f32 amount = (elapsed - delay) / duration;
                target->w = 0.0f;
                target->y = amount * (to.y - from.y) + from.y;
                target->z = amount * (to.z - from.z) + from.z;
                target->x = amount * (to.x - from.x) + from.x;
            }
        }
    }
};
HintVectorTransition hintIconPos;
HintScalarTransition hintIconScale;
HintScalarTransition hintYPop;
f32 (*Hint_AlphaTargetFn)();
i32 (*Hub_PanelBusyFn)();
extern i32 only_process_this_hint_id;
f32 SeekLinearF(f32, f32, f32);
void Hint_SetHint(HINT_s *, i32, i32);
i32 Text_ExpandButtonString(char *, char *);

extern i32 NewMode, Paused, editor_active, CutSceneWaiting, PANELOFF;
extern FadeSystem FadeSys;
i32 GetMenuID();
void MechHintUIButton_OnClick_Callback(MechTouchUIElement &, TouchHolder &);

struct MechHintUIButton : MechTouchUITexButton {
    HINT_s *active_hint;
    HINT_s *pending_hint;
    f32 display_elapsed;
    f32 *slide_target;
    f32 slide_from, slide_to, slide_elapsed, slide_duration, slide_delay, slide;
    u8 pulse_active;
    f32 pulse_remaining;

    MechHintUIButton(VuVec const &pos, f32 radius) : MechTouchUITexButton(pos, radius) {
        slide_elapsed = 0.0f;
        slide_duration = -1.0f;
        slide_delay = 0.0f;
        active_hint = NULL;
        pending_hint = NULL;
        slide_target = &slide;
        on_click = MechHintUIButton_OnClick_Callback;
    }
    ~MechHintUIButton() override {
    }
    void __attribute__((weak)) Process(float) override;
    void __attribute__((weak)) Render() override;
    void StartPulse() {
        static f32 PulseAgainTime = 2.0f;
        pulse_active = 1;
        pulse_remaining = PulseAgainTime;
        if (visible)
            MechSystems::Get()->NewRadarPulse(position, false);
    }
};
DECOMP_ASSERT(sizeof(MechHintUIButton) == 0xa8, "MechHintUIButton size");

void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);

void MechHintUIButton_OnClick_Callback(MechTouchUIElement &element, TouchHolder &) {
    MechHintUIButton &button = static_cast<MechHintUIButton &>(element);
    hintsys.active_hint = button.active_hint;
    if (button.active_hint != NULL) {
        if (button.active_hint->on_display != NULL)
            button.active_hint->on_display(button.active_hint);
        if (button.active_hint != NULL) {
            const f32 blend = AlphaBlendTime;
            button.pending_hint = NULL;
            button.slide_from = *button.slide_target;
            button.slide_to = -1.5f;
            button.slide_elapsed = 0.0f;
            button.slide_delay = 0.0f;
            button.active_hint = NULL;
            button.slide_duration = blend;
            button.alpha_from = *button.alpha_target;
            button.pulse_active = 0;
            button.alpha_to = 0.0f;
            button.alpha_elapsed = 0.0f;
            button.alpha_delay = 0.0f;
            button.alpha_duration = blend;
        }
    }
    const f32 x = button.slide;
    const f32 y = button.position.y;
    hintIconPos.from = VuVec(x, y, 1.0f, 1.0f);
    hintIconPos.to = VuVec(-ICONX, -0.7f, 1.0f, 1.0f);
    hintIconPos.elapsed = 0.0f;
    hintIconPos.duration = 0.5f;
    hintIconPos.delay = 0.0f;
    hintIconPos.target->z = 1.0f;
    hintIconPos.target->w = 1.0f;
    hintIconPos.target->x = x;
    hintIconPos.target->y = y;
    const f32 scale = button.scale;
    hintIconScale.from = scale;
    hintIconScale.to = 0.5f;
    hintIconScale.elapsed = 0.0f;
    hintIconScale.duration = 0.5f;
    hintIconScale.delay = 0.0f;
    *hintIconScale.target = scale;
    hintYPop.value = 0.0f;
    hintYPop.from = 0.0f;
    hintYPop.to = 32768.0f;
    hintYPop.elapsed = 0.0f;
    hintYPop.duration = 0.4f;
    hintYPop.delay = 0.0f;
    *hintYPop.target = 0.0f;
    *button.slide_target = button.slide_to;
    button.slide = -1.5f;
    button.slide_to = -1.5f;
    button.slide_elapsed = button.slide_duration;
    GameAudio_PlaySfx(0x30, NULL, 0, 0);
}

void initHintSys() {
    hintsys.current_hint = 0;
    hintsys.state = 2;
    hintsys.active_hint = NULL;
    MechHintUIButton *button =
        new MechHintUIButton(VuVec(ICONXPOS_TOUCHYFEELY, ICONYPOS_TOUCHYFEELY, 0.0f, 1.0f), 0.075f);
    button->UpdateTexture(MechInputTouchVirtualConsoleController::s_textures[3]);
    button->alpha = 0.0f;
    button->alpha_to = 0.0f;
    button->slide = -1.5f;
    button->visible = 0;
    button->slide_to = -1.5f;
    button->pulse_active = 0;
    button->slide_elapsed = button->slide_duration;
    hintUIButton = reinterpret_cast<HINTUIBUTTON_s *>(button);
    button->alpha_elapsed = button->alpha_duration;
    MechSystems::Get()->TouchUI().AddUIElement(*button);
}

void MechHintUIButton::Render() {
    if (NewMode != 0 || NewLData != NULL)
        return;
    LEVEL_OBJECT_RUNTIME *icon = &WORLD->lev_objs[0xd4];
    if (icon->active == 0)
        return;
    const f32 opacity = alpha;
    const u16 angle = static_cast<u16>(static_cast<i32>((NuFmod(GameTimer.time_elapsed, 2.5f) / 2.5f) * 65536.0f));
    DrawPanel3DObject(position.x, position.y, 1.0f, scale, scale, scale, 0, angle, 0, &icon->special, 2, opacity);
}

void MechHintUIButton::Process(float elapsed) {
    if (active_hint != NULL) {
        display_elapsed += elapsed;
        if (MechSystems::Get()->PlayerButton().selector != NULL ||
            (active_hint->display_duration > 0.0f && display_elapsed >= active_hint->display_duration)) {
            if (active_hint != NULL) {
                const f32 blend = AlphaBlendTime;
                pending_hint = NULL;
                slide_from = *slide_target;
                slide_to = -1.5f;
                slide_elapsed = 0.0f;
                slide_delay = 0.0f;
                active_hint = NULL;
                slide_duration = blend;
                alpha_from = *alpha_target;
                pulse_active = 0;
                alpha_to = 0.0f;
                alpha_elapsed = 0.0f;
                alpha_delay = 0.0f;
                alpha_duration = blend;
            }
        }
    }
    HINT_s *next_hint = pending_hint;
    const u8 was_pulsing = pulse_active;
    scale = scale_to = hovered ? 0.6f : 0.5f;
    scale_elapsed = scale_duration;
    if (!(slide_duration < 0.0f) && !(slide_elapsed >= slide_duration + slide_delay)) {
        slide_elapsed += elapsed;
        if (slide_elapsed > slide_duration + slide_delay)
            slide_elapsed = slide_duration + slide_delay;
        if (slide_elapsed >= slide_delay)
            *slide_target = ((slide_elapsed - slide_delay) / slide_duration) * (slide_to - slide_from) + slide_from;
    }
    position.x = slide;
    if (next_hint != NULL && (slide_duration < 0.0f || slide_elapsed >= slide_duration + slide_delay)) {
        active_hint = next_hint;
        pending_hint = NULL;
        display_elapsed = 0.0f;
        const bool touch = TouchHacks::TouchControlsActive || MechInputTouchSystem::s_actualTouchMode == 7;
        position.y = touch ? ICONYPOS_TOUCHYFEELY : ICONYPOS_VIRTUALS;
        const f32 blend = AlphaBlendTime;
        slide_from = *slide_target;
        slide_to = touch ? ICONXPOS_TOUCHYFEELY : ICONXPOS_VIRTUALS;
        slide_elapsed = 0.0f;
        slide_delay = 0.0f;
        slide_duration = blend;
        alpha_from = *alpha_target;
        alpha_to = 1.0f;
        alpha_elapsed = 0.0f;
        alpha_duration = blend;
        alpha_delay = 0.0f;
    }
    if (was_pulsing) {
        pulse_remaining -= elapsed;
        if (0.0f >= pulse_remaining)
            StartPulse();
    }
    MechTouchUITexButton::Process(elapsed);
    if (visible) {
        visible = 0;
        if (NewMode == 0 && NewLData == NULL && editor_active == 0 && GameTimer.time_elapsed > 0.0f &&
            GameTimer.update_count != 0 && WORLD != NULL && WORLD->current_level != TITLES_LDATA &&
            CutSceneWaiting == 0 && Paused == 0 && CUTSTOPGAME == 0 && MiniCutCam == 0 && PANELOFF == 0 &&
            WORLD->current_level != STATUS_LDATA && (WORLD->current_level->flags & 0x400) == 0)
            visible = 1;
    }
    disabled = (GetMenuID() != -1 && GetMenuID() != 13) || Paused != 0 || NewMode != 0 || NewLData != NULL ||
               CUTSTOPGAME != 0 || MiniCutCam != 0 || FadeSys.fade > 0.0f || player == NULL;
    if (!pulse_active && (slide_duration < 0.0f || slide_elapsed >= slide_duration + slide_delay) && slide > -1.0f)
        StartPulse();
}

void CLEAR_HINT_COMPLETE(i32 hint_id);
i32 HINT_COMPLETE(i32 hint_id);
void SET_HINT_COMPLETE(i32 hint_id);
void Hint_SaveGameState(HINT_s *hint);
void initHintSys();
i32 qrand();

void Hint_Reset() {
    HINT_s *hint = hintsys.hints;
    if (hint == NULL) {
        return;
    }

    while (hint->control_mode_ids[0] != -1) {
        hint->field_0x20 = 0;
        hint++;
    }

    hintsys.state = 2;
    hintsys.current_hint = 0;

    HINTUIBUTTON_s *button = hintUIButton;
    if (button->field_0x78 != NULL) {
        f32 alpha_blend_time = AlphaBlendTime;
        button->field_0x7c = 0;
        f32 field_0x84 = *button->field_0x84;
        f32 *field_0x40 = button->field_0x40;
        button->field_0x8c = -1.5f;
        button->field_0x90 = 0.0f;
        button->field_0x98 = 0.0f;
        button->field_0x88 = field_0x84;
        button->field_0x78 = NULL;
        button->field_0x94 = alpha_blend_time;
        f32 field_0x40_value = *field_0x40;
        button->field_0xa0 = 0;
        button->field_0x48 = 0.0f;
        button->field_0x4c = 0.0f;
        button->field_0x54 = 0.0f;
        button->field_0x44 = field_0x40_value;
        button->field_0x50 = alpha_blend_time;
    }

    hintsys.field_0x1c = 0;
}

void Hint_Process(float elapsed) {
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    const i32 only_hint = only_process_this_hint_id;
    only_process_this_hint_id = -1;
    hintIconPos.Update(elapsed);
    hintIconScale.Update(elapsed);
    hintYPop.Update(elapsed);
    if (hintsys.hints == NULL)
        return;

    bool available =
        HINTS_ON != 0 && CUTSTOPGAME == 0 && MiniCutCam != 1 && MiniCutCam != 2 && MiniCutCam != 3 && SuperStory == 0 &&
        BonusArea == 0 && (world->current_level->flags & 0x200) == 0 &&
        (world->area == NULL || (world->area->flags & 0x124) == 0) && GetMenuID() == -1 && FadeSys.fade == 0.0f &&
        (world->area == NULL || world->area != HUB_ADATA || Hub_PanelBusyFn == NULL || Hub_PanelBusyFn() == 0);
    if (ChallengeMode != 0)
        available = false;
    else if (Mission_Active(NULL) != NULL)
        available = false;
    if (hintUIButton->field_0x78 != NULL || hintUIButton->field_0x7c != NULL)
        available = false;

    for (HINT_s *hint = hintsys.hints; hint->control_mode_ids[0] != -1; ++hint) {
        if (hintsys.active_hint != hint && hint->field_0x20 > 0.0f)
            hint->field_0x20 -= elapsed;
        if (only_hint != -1 && hint->control_mode_ids[0] != only_hint)
            continue;
        const i32 control_mode = MechInputTouchSystem::s_baseControlMode;
        const i32 id = hint->control_mode_ids[control_mode];
        if (id == -1 || TTab[id] == NULL || !available || hintUIButton->field_0x78 != NULL ||
            hintUIButton->field_0x7c != NULL || hintsys.active_hint != NULL ||
            hint->completion_flags[control_mode] != 0 || hint->availability_fn == NULL)
            continue;
        if (hint->availability_fn(hint) == 0 || hint->field_0x20 > 0.0f ||
            hint->completion_flags[MechInputTouchSystem::s_baseControlMode] == 1)
            continue;
        Hint_SetHint(hint, 0, 0);
    }

    const f32 target = Hint_AlphaTargetFn != NULL ? Hint_AlphaTargetFn() : 1.0f;
    hintsys.alpha = SeekLinearF(hintsys.alpha, target, 3.0f * FRAMETIME);
    HINT_s *hint = hintsys.active_hint;
    if (hint != NULL) {
        hintsys.display_elapsed += elapsed;
        if ((hint->display_duration > 0.0f && hintsys.display_elapsed >= hint->display_duration) ||
            MechSystems::Get()->PlayerButton().selector != NULL)
            hintsys.active_hint = NULL;
    }
}

void Hint_SetHint(HINT_s *hint, i32 force, i32 allow_completed) {
    if (hintsys.active_hint != NULL && hintsys.active_hint == hint)
        return;
    HINTUIBUTTON_s *button = hintUIButton;
    if (button->field_0x78 != NULL && button->field_0x78 == hint)
        return;
    if (hint == NULL) {
        hintsys.active_hint = NULL;
        if (button->field_0x78 != NULL) {
            const f32 blend_time = AlphaBlendTime;
            button->field_0x7c = NULL;
            const f32 position = *button->field_0x84;
            f32 *alpha = button->field_0x40;
            button->field_0x8c = -1.5f;
            button->field_0x90 = 0.0f;
            button->field_0x98 = 0.0f;
            button->field_0x88 = position;
            button->field_0x78 = NULL;
            button->field_0x94 = blend_time;
            const f32 alpha_value = *alpha;
            button->field_0xa0 = 0;
            button->field_0x48 = 0.0f;
            button->field_0x4c = 0.0f;
            button->field_0x54 = 0.0f;
            button->field_0x44 = alpha_value;
            button->field_0x50 = blend_time;
        }
        return;
    }
    if (force == 0) {
        if (hint->completion_flags[MechInputTouchSystem::s_baseControlMode] != 0 && allow_completed == 0)
            return;
        if (hint->field_0x20 > 0.0f)
            return;
    }
    if ((hint->flags & 8) != 0) {
        if (button->field_0x78 != NULL) {
            const f32 blend_time = AlphaBlendTime;
            button->field_0x7c = NULL;
            const f32 position = *button->field_0x84;
            f32 *alpha = button->field_0x40;
            button->field_0x8c = -1.5f;
            button->field_0x90 = 0.0f;
            button->field_0x98 = 0.0f;
            button->field_0x88 = position;
            button->field_0x78 = NULL;
            button->field_0x94 = blend_time;
            const f32 alpha_value = *alpha;
            button->field_0xa0 = 0;
            button->field_0x48 = 0.0f;
            button->field_0x4c = 0.0f;
            button->field_0x54 = 0.0f;
            button->field_0x44 = alpha_value;
            button->field_0x50 = blend_time;
        }
        hintsys.active_hint = hint;
        if (hint->on_display != NULL)
            hint->on_display(hint);
    } else if (button->field_0x78 != hint) {
        const f32 blend_time = AlphaBlendTime;
        button->field_0x7c = hint;
        const f32 position = *button->field_0x84;
        f32 *alpha = button->field_0x40;
        button->field_0x8c = -1.5f;
        button->field_0x90 = 0.0f;
        button->field_0x98 = 0.0f;
        button->field_0x88 = position;
        button->field_0x94 = blend_time;
        const f32 alpha_value = *alpha;
        button->field_0xa0 = 0;
        button->field_0x48 = 0.0f;
        button->field_0x4c = 0.0f;
        button->field_0x54 = 0.0f;
        button->field_0x44 = alpha_value;
        button->field_0x50 = blend_time;
    }
    hintsys.current_hint = 0;
    hintsys.field_0x1c = 0;
    if (force == 0 && (hint->flags & 1) != 0) {
        hint->completion_flags[MechInputTouchSystem::s_baseControlMode] = 1;
        Hint_SaveGameState(hint);
    }
    i32 state;
    do {
        state = qrand() / 16384;
    } while (state == hintsys.state);
    hintsys.state = static_cast<u8>(state);
    hint->field_0x20 = hint->repeat_delay;
    hintsys.field_0x1c = 0;
}

HINT_s *Hint_FindHint(i32 hint_id) {
    HINT_s *hint = hintsys.hints;
    if (hint == NULL) {
        return NULL;
    }

    while (hint->control_mode_ids[0] != -1) {
        if (hint->control_mode_ids[0] == hint_id || hint->control_mode_ids[1] == hint_id) {
            return hint;
        }
        ++hint;
    }
    return NULL;
}

void Hint_SaveBits(i32 hint_id, i32 complete) {
    if (complete != 0) {
        SET_HINT_COMPLETE(hint_id);
        return;
    }
    CLEAR_HINT_COMPLETE(hint_id);
}

i32 Hint_CurrentId() {
    HINT_s *hint = hintsys.active_hint;
    if (hint == NULL) {
        return -1;
    }
    return hint->control_mode_ids[MechInputTouchSystem::s_baseControlMode];
}

void Hint_ResetHint(i32 hint_id, i32 reset_completed) {
    HINT_s *hint = hintsys.hints;
    if (hint == NULL || hint->control_mode_ids[0] == -1) {
        return;
    }

    i32 control_mode = MechInputTouchSystem::s_baseControlMode;
    while (hint->control_mode_ids[control_mode] != hint_id) {
        hint++;
        if (hint->control_mode_ids[0] == -1) {
            return;
        }
    }

    if (reset_completed == 1) {
        hint->completion_flags[control_mode] = 0;
    }
    hintsys.current_hint = 0;
}

i32 Hint_isComplete(HINT_s *hint) {
    return hint->completion_flags[MechInputTouchSystem::s_baseControlMode];
}

i32 Hint_isComplete(i32 hint_id) {
    HINT_s *hint = hintsys.hints;
    if (hint == NULL) {
        return 0;
    }

    hint = Hint_FindHint(hint_id);
    if (hint == NULL) {
        return 0;
    }
    return hint->completion_flags[MechInputTouchSystem::s_baseControlMode];
}

void Hint_SetComplete(HINT_s *hint) {
    if (hint == NULL) {
        return;
    }

    const i32 control_mode = MechInputTouchSystem::s_baseControlMode;
    if (hint->completion_flags[control_mode] == 0) {
        hint->completion_flags[control_mode] = 1;
        Hint_SaveGameState(hint);
    }
}

void Hint_SetComplete(i32 hint_id) {
    if (hint_id == -1) {
        return;
    }

    HINT_s *hint = Hint_FindHint(hint_id);
    if (hint == NULL || (hint->flags & 0x40) == 0) {
        return;
    }

    const i32 control_mode = MechInputTouchSystem::s_baseControlMode;
    if (hint->completion_flags[control_mode] == 0) {
        hint->completion_flags[control_mode] = 1;
        Hint_SaveGameState(hint);
    }
}

i32 Hint_isAvailable(i32 hint_id) {
    HINT_s *active_hint = hintsys.active_hint;
    if (active_hint != NULL && active_hint->control_mode_ids[MechInputTouchSystem::s_baseControlMode] == hint_id) {
        return 0;
    }
    if (hint_id < 0 || TTab[hint_id] == NULL) {
        return 0;
    }

    HINT_s *hint = Hint_FindHint(hint_id);
    if (hint == NULL || hint->completion_flags[MechInputTouchSystem::s_baseControlMode] != 0) {
        return 0;
    }
    return !(hint->field_0x20 > 0.0f);
}

void Hint_CancelCurrent() {
    HINT_s *hint = hintsys.active_hint;
    if (hint != NULL) {
        if (hint->display_duration != 0.0f && hint->display_duration - 0.5f > hintsys.display_elapsed)
            hintsys.display_elapsed = hint->display_duration - 0.5f;
        hint->field_0x20 = 0.0f;
    }
    HINTUIBUTTON_s *button = hintUIButton;
    if (button->field_0x78 != NULL) {
        const f32 blend_time = AlphaBlendTime;
        button->field_0x7c = NULL;
        const f32 position = *button->field_0x84;
        f32 *alpha = button->field_0x40;
        button->field_0x8c = -1.5f;
        button->field_0x90 = 0.0f;
        button->field_0x98 = 0.0f;
        button->field_0x88 = position;
        button->field_0x78 = NULL;
        button->field_0x94 = blend_time;
        const f32 alpha_value = *alpha;
        button->field_0xa0 = 0;
        button->field_0x48 = 0.0f;
        button->field_0x4c = 0.0f;
        button->field_0x54 = 0.0f;
        button->field_0x44 = alpha_value;
        button->field_0x50 = blend_time;
    }
}

void Hint_ExpandButtons(char *input, char *output) {
    char token[256];
    char expanded[64];

    *output = '\0';
    while (*input != '\0') {
        if (*input != '[') {
            *output = *input;
            output[1] = '\0';
            ++input;
            ++output;
            continue;
        }

        token[0] = '[';
        i32 token_length = 1;
        while (input[token_length] != ']' && input[token_length] != '\0') {
            token[token_length] = input[token_length];
            ++token_length;
        }

        char literal = '[';
        if (input[token_length] != '\0') {
            token[token_length] = ']';
            token[token_length + 1] = '\0';
            token_length = NuStrLen(token);
            if (token_length > 0 && Text_ExpandButtonString(token, expanded) != 0) {
                char *character = expanded;
                while (*character != '\0') {
                    *output = *character;
                    output[1] = '\0';
                    ++character;
                    ++output;
                }
                NuStrCat(output, expanded);
                input += token_length;
                continue;
            }
            literal = *input;
        }

        *output = literal;
        output[1] = '\0';
        ++input;
        ++output;
    }
    *output = '\0';
}

void Hint_SaveGameState(HINT_s *hint) {
    HINT_s *candidate = hintsys.hints;
    if (candidate == NULL) {
        return;
    }
    if (hintsys.save_bits == NULL) {
        return;
    }
    if (candidate->control_mode_ids[0] == -1) {
        return;
    }

    const u16 hint_id = static_cast<u16>(hint->control_mode_ids[0]);
    i32 index = 0;
    u16 candidate_id = static_cast<u16>(candidate->control_mode_ids[0]);
    if (candidate_id == hint_id) {
        goto found;
    }
    do {
        ++candidate;
        ++index;
        candidate_id = static_cast<u16>(candidate->control_mode_ids[0]);
        if (candidate_id == 0xffff) {
            return;
        }
    } while (candidate_id != hint_id);

found:
    Hint_SaveBits(index, 1);
}

void Hint_SetHintFromId(i32 hint_id, i32 force, i32 allow_completed) {
    if (hintsys.active_hint != NULL &&
        hintsys.active_hint->control_mode_ids[MechInputTouchSystem::s_baseControlMode] == hint_id)
        return;
    if (hint_id < 0 || TTab[hint_id] == NULL) {
        HINTUIBUTTON_s *button = hintUIButton;
        if (button->field_0x78 != NULL) {
            const f32 blend_time = AlphaBlendTime;
            button->field_0x7c = NULL;
            const f32 position = *button->field_0x84;
            f32 *alpha = button->field_0x40;
            button->field_0x8c = -1.5f;
            button->field_0x90 = 0.0f;
            button->field_0x98 = 0.0f;
            button->field_0x88 = position;
            button->field_0x78 = NULL;
            button->field_0x94 = blend_time;
            const f32 alpha_value = *alpha;
            button->field_0xa0 = 0;
            button->field_0x48 = 0.0f;
            button->field_0x4c = 0.0f;
            button->field_0x54 = 0.0f;
            button->field_0x44 = alpha_value;
            button->field_0x50 = blend_time;
        }
        return;
    }
    HINT_s *hint = Hint_FindHint(hint_id);
    if (hint != NULL)
        Hint_SetHint(hint, force, allow_completed);
}

void Hint_LoadAllGameState() {
    HINTSYS_s *system = &hintsys;
    HINT_s *hint = system->hints;
    if (hint == NULL || system->save_bits == NULL || hint->control_mode_ids[0] == -1 || system->save_bit_count <= 0) {
        return;
    }

    i32 index = 0;
    const i32 control_mode = MechInputTouchSystem::s_baseControlMode;
    do {
        if (HINT_COMPLETE(index) != 0) {
            hint->completion_flags[control_mode] = 1;
        } else {
            hint->completion_flags[control_mode] = 0;
        }
        ++hint;
        if (hint->control_mode_ids[0] == -1) {
            return;
        }
        ++index;
    } while ((index >> 5) < system->save_bit_count);
}

void Hint_SaveAllGameState() {
    HINT_s *hint = hintsys.hints;
    if (hint == NULL) {
        return;
    }
    u32 *save_bits = hintsys.save_bits;
    if (save_bits == NULL || hint->control_mode_ids[0] == -1) {
        return;
    }

    i32 index = 0;
    const i32 control_mode = MechInputTouchSystem::s_baseControlMode;
    do {
        if (hint->completion_flags[control_mode] != 0) {
            Hint_SaveBits(index, 1);
        } else {
            Hint_SaveBits(index, 0);
        }
        ++hint;
        ++index;
    } while (hint->control_mode_ids[0] != -1);
}

void Hint_ClearHintsAndDoneFlags() {
    HINT_s *hint = hintsys.hints;
    if (hint == NULL || hint->control_mode_ids[0] == -1) {
        return;
    }

    i32 control_mode = MechInputTouchSystem::s_baseControlMode;
    do {
        hint->completion_flags[control_mode] = 0;
        hint->flags &= ~0x42;
        hint++;
    } while (hint->control_mode_ids[0] != -1);
}

void Hint_Draw(i32 viewport) {
    if (hintsys.active_hint != NULL && hintsys.update_fn != NULL)
        hintsys.update_fn(hintsys.active_hint, viewport);
}

// Static game message and hint helpers. Stubbed to satisfy the symbol baseline.

static __used__ void EndRedBrickMessage(GAMEMESSAGE_s *) {
}

static __used__ int GameMsg_GetExtraObj(GAMEMESSAGE_s *) {
    return 0;
}

static __used__ void GameMsg_EndDelay_Game(GAMEMESSAGE_s *) {
}

static __used__ void GameMsg_Draw_MiniKitDetector(GAMEMESSAGE_s *, nuvec_s *, float) {
}

pushblock_s *NearestPushBlock(WORLDINFO_s *, nuvec_s *, f32);

i32 Push_UpdateHints(HINT_s *) {
    if (LEGOACT_PUSH == -1)
        return 0;
    for (i32 i = 0; i < 8; ++i) {
        GameObject_s *object = Player[i];
        if (object != NULL && static_cast<i8>(object->apiobj.flags_low) < 0 &&
            object->apiobj.character_model->model_data_b[LEGOACT_PUSH] != NULL &&
            NearestPushBlock(WORLD, &object->apiobj.collision_position, 2.0f) != NULL)
            return 1;
    }
    return 0;
}

void Percent_UpdateHint(HINT_s *) {
}

void RegisterWithHintSys(void (*update_fn)(HINT_s *, i32), HINT_s *hints, u32 *save_bits, i32 save_bit_count) {
    hintsys.update_fn = update_fn;
    hintsys.hints = hints;
    hintsys.save_bits = save_bits;
    hintsys.save_bit_count = save_bit_count;
    initHintSys();
}
