#include "MechInputTouch_types.h"
#include "legoapi/audio/audio.h"

#include <string.h>

#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "globals.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/render/core/render.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/numtl.h"

void RndrTexQuad(f32, f32, f32, f32, i32, numtl_s *, i32);

extern i32 CutSceneWaiting;
extern i32 editor_active;
extern i32 NewMode;
extern i32 PANELOFF;
extern i32 Paused;
f32 TagButtonSize;

i32 GetMenuID();
float GetAspectRatio();
void PlayerButton_OnClick_Callback_NextButton(MechTouchUIElement &, TouchHolder &);
void PlayerButton_OnHold_Callback(MechTouchUIElement &, TouchHolder &);
void PlayerButton_OnLeave_Callback(MechTouchUIElement &, TouchHolder &);
void MechTouchUIPauseButton_OnClick_Callback(MechTouchUIElement &, TouchHolder &);

bool MechInputTouchGestureTracker::OnDown(GameObject_s &, TouchHolder &) {
    return false;
}

bool MechInputTouchGestureTracker::OnRelease(GameObject_s &, TouchHolder &) {
    return false;
}

bool MechInputTouchGestureTracker::OnClick(GameObject_s &, TouchHolder &) {
    return false;
}

bool MechInputTouchGestureTracker::OnDoubleClick(GameObject_s &, TouchHolder &) {
    return false;
}

bool MechInputTouchGestureTracker::OnHold(GameObject_s &, TouchHolder &) {
    return false;
}

bool MechInputTouchGestureTracker::OnSwipe(GameObject_s &, TouchHolder &, i32) {
    return false;
}

void MechTouchUIElement::Process(float) {
}

void MechTouchUIElement::Render() {
}

bool MechTouchUI::AddUIElement(MechTouchUIElement &element) {
    for (i32 i = 0; i < 32; ++i) {
        if (elements[i] == NULL) {
            elements[i] = &element;
            return true;
        }
    }
    return false;
}

void MechTouchUI::Init() {
    MechSystems *systems = MechSystems::Get();
    systems->gesture_tracking_system.RegisterGestureTracker(*this, 100);
}

MechTouchUI::MechTouchUI() {
    memset(elements, 0, sizeof(elements));
}

bool MechTouchUI::OnClick(GameObject_s &, TouchHolder &holder) {
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element != NULL && element->owner == &holder) {
            if (element->hovered != 0 && element->on_click != NULL) {
                if (element->disabled == 0) {
                    element->on_click(*element, holder);
                } else {
                    GameAudio_PlaySfx(0x32, NULL, 0, 0);
                }
            }
            return true;
        }
    }
    return false;
}

bool MechTouchUI::OnDoubleClick(GameObject_s &, TouchHolder &holder) {
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element != NULL && element->owner == &holder) {
            return true;
        }
    }
    return false;
}

bool MechTouchUI::OnDown(GameObject_s &, TouchHolder &holder) {
    MechTouchUIElement *element = PickElement(holder.down_position);
    if (element != NULL) {
        element->owner = &holder;
        if (element->on_down != NULL && element->disabled == 0) {
            element->on_down(*element, holder);
        }
    }
    return element != NULL;
}

bool MechTouchUI::OnHold(GameObject_s &, TouchHolder &holder) {
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element != NULL && element->owner == &holder) {
            if (PickElement(holder.touch_position) == element && elements[i]->on_hold != NULL &&
                elements[i]->disabled == 0) {
                elements[i]->on_hold(*elements[i], holder);
            }
            holder.field_0x0[7] = 1;
            return true;
        }
    }
    return false;
}

bool MechTouchUI::OnRelease(GameObject_s &, TouchHolder &holder) {
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element != NULL && element->owner == &holder) {
            if (element->on_release != NULL && element->disabled == 0) {
                element->on_release(*element, holder);
            }
            elements[i]->owner = NULL;
        }
    }
    return false;
}

MechTouchUIElement *MechTouchUI::PickElement(NuVec2 &point) {
    MechTouchUIElement *picked = NULL;
    float picked_depth = -1000000000.0f;
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element == NULL || (element->visible == 0 && PANELOFF == 0) || element->disabled != 0) {
            continue;
        }

        if (element->rectangular == 0) {
            const float x = point.x - element->position.x;
            const float y = point.y - element->position.y;
            if (1.0f <
                x * x / (element->radius_x * element->radius_x) + y * y / (element->radius_y * element->radius_y)) {
                continue;
            }
        } else {
            const float half_x = element->radius_x * 0.5f;
            const float half_y = element->radius_y * 0.5f;
            if (point.x < element->position.x - half_x || point.x > element->position.x + half_x ||
                point.y < element->position.y - half_y || point.y > element->position.y + half_y) {
                continue;
            }
        }
        if (element->position.z > picked_depth) {
            picked = element;
            picked_depth = element->position.z;
        }
    }
    return picked;
}

void MechTouchUI::Process(float dt) {
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element == NULL) {
            continue;
        }
        element->Process(dt);
        const bool was_hovered = element->hovered != 0;
        if (element->owner != NULL && PickElement(element->owner->touch_position) == element) {
            element->hovered = 1;
        } else {
            element->hovered = 0;
            if (was_hovered && element->on_leave != NULL && element->owner != NULL) {
                element->on_leave(*element, *element->owner);
            }
        }
    }
}

bool MechTouchUI::RemoveUIElement(MechTouchUIElement &element) {
    for (i32 i = 0; i < 32; ++i) {
        if (elements[i] == &element) {
            element.hovered = 0;
            element.owner = NULL;
            elements[i] = NULL;
            return true;
        }
    }
    return false;
}

void MechTouchUI::Render() {
    if (CUTSTOPGAME != 0 && GetMenuID() == -1) {
        return;
    }
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUIElement *element = elements[i];
        if (element != NULL && element->visible != 0) {
            element->Render();
        }
    }
}

MechTouchUI::~MechTouchUI() {
}

MechTouchUICharIcon::MechTouchUICharIcon(MechTouchUIPartySelector &party, VuVec const &pos, i32 id, float scale)
    : MechTouchUIElement(pos, scale), character_id(id), icon_scale(scale), alpha_target(&icon_alpha),
      alpha_duration(-1.0f), selector(&party) {
    alpha_elapsed = 0.0f;
    field_0x45 = 0;
    alpha_delay = 0.0f;
    selected = 0;
    icon_alpha = 0.0f;
    rectangular = 1;
    alpha_end = 0.0f;
    field_0x46 = 0;
    alpha_start = 0.0f;
}

void MechTouchUICharIcon::Process(float) {
}

void MechTouchUICharIcon::Render() {
    f32 scale = icon_scale;
    if ((hovered != 0 || selected != 0) && disabled == 0)
        scale *= 1.15f;
    const f32 alpha = icon_alpha;
    const i32 frame = hovered != 0 || selected != 0 ? 0xa6 : 0xa7;
    DrawCharIcon(character_id, position.x, position.y, position.z, scale, frame, alpha, alpha, 1, NULL);
    if (hovered != 0 || selected != 0) {
        f32 width = radius_y * GetAspectRatio();
        f32 x = selector->player_button->position.x + static_cast<f32>(selector->icon_count) * width + width * 0.5f;
        SmartTextEx(TTab[CDataList[character_id].name_id], x, position.y, position.z, 0.4f, 0.4f, 0.4f, 2, 255, 255,
                    255, 0.7f, 1, NULL, 0, static_cast<i32>(alpha * 255.0f));
    }
}

void MechTouchUICharIcon::SetupDisabled() {
}

void MechTouchUITagButton::FadeOut() {
    if (fading_out == 0) {
        fading_out = 1;
        if (1.0f > second_fade.value) {
            second_fade.from = *second_fade.target;
            second_fade.to = 0.0f;
            second_fade.elapsed = 0.0f;
            second_fade.duration = 0.2f;
            second_fade.delay = 0.0f;
        }
        first_fade.from = *first_fade.target;
        first_fade.to = 0.0f;
        first_fade.elapsed = 0.0f;
        first_fade.duration = 0.2f;
        first_fade.delay = 0.0f;
    }
}

MechTouchUITagButton::MechTouchUITagButton(GameObject_s &object, TouchHolder &holder)
    : MechTouchUIElement(VuVec(object.camera_screen_position.x, object.camera_screen_position.y,
                               object.camera_screen_position.z, 1.0f),
                         TagButtonSize),
      target_object(object.GetMechObjectInterface()), touch_holder(&holder) {
    position.z = 0.0f;
    rectangular = 0;
    tag_state = 2;
    fading_out = 0;
    touched = 0;
    enabled = 1;

    first_fade.target = &first_fade.value;
    first_fade.from = 0.0f;
    first_fade.to = 1.0f;
    first_fade.elapsed = 0.0f;
    first_fade.duration = 0.3f;
    first_fade.delay = 0.0f;
    first_fade.value = 0.0f;

    hover_animation.target = &hover_animation.value;
    hover_animation.to = 0.0f;
    hover_animation.elapsed = -1.0f;
    hover_animation.duration = -1.0f;
    hover_animation.delay = 0.0f;
    hover_animation.value = 0.0f;

    size_animation.target = &size_animation.value;
    size_animation.to = 1.0f;
    size_animation.elapsed = -1.0f;
    size_animation.duration = -1.0f;
    size_animation.delay = 0.0f;
    size_animation.value = 1.0f;

    second_fade.target = &second_fade.value;
    second_fade.from = 0.0f;
    second_fade.to = 1.0f;
    second_fade.elapsed = 0.0f;
    second_fade.duration = 0.8f;
    second_fade.delay = 0.0f;
    second_fade.value = 0.0f;
    field_0xbc = 0.0f;

    timer_animation.target = &timer_animation.value;
    timer_animation.to = 0.0f;
    timer_animation.elapsed = -1.0f;
    timer_animation.duration = -1.0f;
    timer_animation.delay = 0.0f;
    timer_animation.value = 0.0f;
    timer = 1.0f;
}

void MechTouchUITagButton::Process(float) {
}

void MechTouchUITagButton::Render() {
}

MechTouchUITagButton::~MechTouchUITagButton() {
}

MechTouchUITexButton::MechTouchUITexButton(VuVec const &pos, float radius) : MechTouchUIElement(pos, radius) {
    rectangular = 0;
    alpha_target = &alpha;
    alpha_elapsed = 0.0f;
    alpha_duration = -1.0f;
    alpha_delay = 0.0f;
    scale_target = &scale;
    scale_elapsed = 0.0f;
    scale_duration = -1.0f;
    scale_delay = 0.0f;
    alpha = alpha_to = alpha_from = 1.0f;
    scale = scale_to = scale_from = 1.0f;
    material = NuMtlCreate(1);
    material->attribs.cull_mode = 2;
    material->attribs.z_mode = 1;
    material->attribs.alpha_mode = 1;
    material->attribs.unknown_2_1_2 = 2;
    material->attribs.alpha_test = 1;
    material->diffuse_color.r = 0.0f;
    material->diffuse_color.g = 0.0f;
    material->diffuse_color.b = 0.0f;
    material->opacity = 0.0f;
    material->sort_pri = 255;
}

void MechTouchUITexButton::Process(float) {
    f32 frame_time = FRAMETIME;
    if (!(scale_duration < 0.0f) && !(scale_elapsed >= scale_duration + scale_delay)) {
        scale_elapsed += frame_time;
        if (scale_elapsed > scale_duration + scale_delay)
            scale_elapsed = scale_duration + scale_delay;
        if (scale_elapsed >= scale_delay)
            *scale_target = ((scale_elapsed - scale_delay) / scale_duration) * (scale_to - scale_from) + scale_from;
    }
    if (!(alpha_duration < 0.0f) && !(alpha_elapsed >= alpha_duration + alpha_delay)) {
        alpha_elapsed += frame_time;
        if (alpha_elapsed > alpha_duration + alpha_delay)
            alpha_elapsed = alpha_duration + alpha_delay;
        if (alpha_elapsed >= alpha_delay)
            *alpha_target = ((alpha_elapsed - alpha_delay) / alpha_duration) * (alpha_to - alpha_from) + alpha_from;
    }
    visible = alpha > 0.001f;
}

void MechTouchUITexButton::Render() {
    RndrTexQuad((position.x + 1.0f) * 0.5f, (1.0f - position.y) * 0.5f, scale * radius_x, radius_y * scale,
                static_cast<i32>((static_cast<u32>(static_cast<i32>(alpha * 128.0f)) << 24) | 0x808080), material, 0);
}

void MechTouchUITexButton::UpdateTexture(i16 texture) {
    material->tex_id = texture;
    NuMtlUpdate(material);
}

MechTouchUITexButton::~MechTouchUITexButton() {
    NuMtlDestroy(material);
    material = NULL;
}

MechTouchUIPauseButton::MechTouchUIPauseButton() : MechTouchUIElement(VuVec(0.7725f, 0.7525f, 0.0f, 1.0f), 0.16f) {
    on_click = MechTouchUIPauseButton_OnClick_Callback;
    disable_timer = 0.0f;
    skip_prompt_timer = 0.0f;
}

void MechTouchUIPauseButton::Process(float dt) {
    const float next_disable_timer = disable_timer - dt;
    visible = 0;
    if (next_disable_timer < -10.0f) {
        disable_timer = -1.0f;
        disabled = 0;
    } else {
        disable_timer = next_disable_timer;
        disabled = next_disable_timer > 0.0f;
    }

    if (Paused == 0) {
        skip_prompt_timer -= dt;
    }
    if (MechInputTouchMenuController::AnyTouchesThisFrame > 0) {
        skip_prompt_timer = 3.0f;
    }

    if (NewMode != 0 || NewLData != NULL || editor_active != 0 || GameTimer.time_elapsed <= 0.0f ||
        GameTimer.update_count == 0 || WORLD == NULL || WORLD->current_level == TITLES_LDATA || CutSceneWaiting != 0) {
        return;
    }

    if (CUTSTOPGAME != 0) {
        if (CutScene_IsSkippable(static_cast<CUTINFO *>(CutStopInfo)) == 0 || skip_prompt_timer <= 0.0f) {
            return;
        }
    }

    if (MiniCutCam != 0 || (PANELOFF != 0 && GetMenuID() == -1) || WORLD->current_level == STATUS_LDATA ||
        (WORLD->current_level->flags & LEVEL_STATUS) != 0 || memcard_autosavestarted != 0 ||
        memcard_autosavepostdelay > 0.0f || memcard_autosavepredelay > 0.0f) {
        return;
    }

    visible = 1;
}

void MechTouchUIPauseButton::Render() {
    if (NewMode != 0 || NewLData != NULL || editor_active != 0 || GameTimer.time_elapsed <= 0.0f ||
        GameTimer.update_count == 0 || WORLD == NULL || WORLD->current_level == TITLES_LDATA ||
        WORLD->current_level == CREDITS_LDATA) {
        return;
    }

    if (Paused != 0) {
        const i32 menu_id = GetMenuID();
        DrawTouchPrompt(
            const_cast<char *>(menu_id == LEGO_MENU_PAUSE_MAIN || menu_id == LEGO_MENU_PAUSE_CUTSCENE ? ">" : "<<"),
            NULL, hovered != 0, true);
        return;
    }

    if (GameMenu[GameMenuLevel].menu != -1 || CutSceneWaiting != 0 || MiniCutCam != 0 || memcard_autosavestarted != 0 ||
        memcard_autosavepostdelay > 0.0f || memcard_autosavepredelay > 0.0f ||
        (CUTSTOPGAME != 0 && CutScene_IsSkippable(static_cast<CUTINFO *>(CutStopInfo)) == 0)) {
        if (GetMenuID() != -1) {
            DrawTouchPrompt(const_cast<char *>("<<"), NULL, hovered != 0, true);
        }
        return;
    }

    DrawTouchPrompt(const_cast<char *>("II"), NULL, hovered != 0, false);
}

MechTouchUIPlayerButton::MechTouchUIPlayerButton() : MechTouchUIElement(VuVec(-0.7725f, 0.7525f, 0.0f, 1.0f), 0.16f) {
    on_click = PlayerButton_OnClick_Callback_NextButton;
    on_hold = PlayerButton_OnHold_Callback;
    on_leave = PlayerButton_OnLeave_Callback;
    selector = NULL;
    chooser_mode = 1;
}

void MechTouchUIPlayerButton::Process(float) {
}

void MechTouchUIPlayerButton::SetupTargetIds() {
}

void MechTouchUIPlayerButton::ShowChooser() {
    if (FreePlay == 0 && target_ids[1] == -1) {
        return;
    }
    if (selector != NULL) {
        delete selector;
        selector = NULL;
    }
    selector = new MechTouchUIPartySelector(*this, FreePlay != 0 ? free_play_target_ids : target_ids);
    GameAudio_PlaySfx(0x30, NULL, 0, 0);
}

void MechTouchUIPlayerButton::TriggerTagNext() {
}

void MechTouchUIPartySelector::BlendOut() {
    field_0x88 = 1;
    for (i32 i = 0; i < 32; ++i) {
        MechTouchUICharIcon *icon = icons[i];
        if (icon != NULL) {
            icon->field_0x45 = 1;
            bool selected = icon->hovered != 0 && icon->disabled == 0;
            f32 delay = selected ? 0.4f : 0.0f;
            icon->selected = selected;
            icon->alpha_start = *icon->alpha_target;
            icon->alpha_end = 0.0f;
            icon->alpha_elapsed = 0.0f;
            icon->alpha_duration = 0.3f;
            icon->alpha_delay = delay;
        }
    }
}

bool MechTouchUIPartySelector::BlendedOut() {
    if (field_0x88 == 0)
        return false;
    for (i32 i = 0; i < 32; ++i) {
        if (icons[i] != NULL && icons[i]->icon_alpha > 0.001f)
            return false;
    }
    return true;
}

void MechTouchUIPartySelector::Cleanup() {
    for (i32 i = 0; i < 32; ++i) {
        if (icons[i] != NULL) {
            MechSystems::Get()->TouchUI().RemoveUIElement(*icons[i]);
            delete icons[i];
            icons[i] = NULL;
        }
    }
}

MechTouchUIPartySelector::MechTouchUIPartySelector(MechTouchUIPlayerButton &, i32 *) {
}

MechTouchUIPartySelector::~MechTouchUIPartySelector() {
    Cleanup();
}
