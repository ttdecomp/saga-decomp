#include <stddef.h>

#include "MechInputTouch_types.h"

extern u32 colourPurple;

void MechInputTouchButton::ClearTouchLocked(bool force) {
    if (!force) {
        MechSystems::Get()->input_touch_system.SetTouchLockedBy(touch_locked_by, NULL, false);
    }
    touch_locked_by = 0xff;
}

bool MechInputTouchButton::CouldTouchBeLockedBy(u32 touch_id) {
    return MechSystems::Get()->input_touch_system.CouldTouchBeLockedBy(touch_id, this);
}

i32 MechInputTouchButton::FindPossibleTriggeringIndexFromID(u32 touch_id) {
    if (possible_triggering_touch_ids[0] == touch_id)
        return 0;
    if (possible_triggering_touch_ids[1] == touch_id)
        return 1;
    if (possible_triggering_touch_ids[2] == touch_id)
        return 2;
    if (possible_triggering_touch_ids[3] == touch_id)
        return 3;
    if (possible_triggering_touch_ids[4] == touch_id)
        return 4;
    if (possible_triggering_touch_ids[5] == touch_id)
        return 5;
    if (possible_triggering_touch_ids[6] == touch_id)
        return 6;
    if (possible_triggering_touch_ids[7] == touch_id)
        return 7;
    if (possible_triggering_touch_ids[8] == touch_id)
        return 8;
    return possible_triggering_touch_ids[9] == touch_id ? 9 : -1;
}

MechInputTouchButton::MechInputTouchButton(NuTouchInputElement::TYPE type, i32 index, i32 id)
    : NuTouchInputElement(type, index, 0) {
    touch_locked_by = 0xff;
    has_pending_touches = false;
    button_id = id;
    possible_triggering_touch_ids[0] = 0xff;
    possible_triggering_touches[0] = false;
    possible_triggering_touch_ids[1] = 0xff;
    possible_triggering_touches[1] = false;
    possible_triggering_touch_ids[2] = 0xff;
    possible_triggering_touches[2] = false;
    possible_triggering_touch_ids[3] = 0xff;
    possible_triggering_touches[3] = false;
    possible_triggering_touch_ids[4] = 0xff;
    possible_triggering_touches[4] = false;
    possible_triggering_touch_ids[5] = 0xff;
    possible_triggering_touches[5] = false;
    possible_triggering_touch_ids[6] = 0xff;
    possible_triggering_touches[6] = false;
    possible_triggering_touch_ids[7] = 0xff;
    possible_triggering_touches[7] = false;
    possible_triggering_touch_ids[8] = 0xff;
    possible_triggering_touches[8] = false;
    possible_triggering_touch_ids[9] = 0xff;
    possible_triggering_touches[9] = false;
}

MechInputTouchButton::MechInputTouchButton(i32 index, u32 id, float x, float y, float width, float height, i32 button)
    : NuTouchInputElement(NuTouchInputElement::TYPE_BUTTON, index, id, x, y, width, height) {
    touch_locked_by = 0xff;
    has_pending_touches = false;
    button_id = button;
    possible_triggering_touch_ids[0] = 0xff;
    possible_triggering_touches[0] = false;
    possible_triggering_touch_ids[1] = 0xff;
    possible_triggering_touches[1] = false;
    possible_triggering_touch_ids[2] = 0xff;
    possible_triggering_touches[2] = false;
    possible_triggering_touch_ids[3] = 0xff;
    possible_triggering_touches[3] = false;
    possible_triggering_touch_ids[4] = 0xff;
    possible_triggering_touches[4] = false;
    possible_triggering_touch_ids[5] = 0xff;
    possible_triggering_touches[5] = false;
    possible_triggering_touch_ids[6] = 0xff;
    possible_triggering_touches[6] = false;
    possible_triggering_touch_ids[7] = 0xff;
    possible_triggering_touches[7] = false;
    possible_triggering_touch_ids[8] = 0xff;
    possible_triggering_touches[8] = false;
    possible_triggering_touch_ids[9] = 0xff;
    possible_triggering_touches[9] = false;
}

__attribute__((weak)) void MechInputTouchButton::Render() {
}

__attribute__((weak)) void MechInputTouchButton::Update(NuInputTouchData const *) {
}

__attribute__((weak)) char const *MechInputTouchButton::GetName() {
    return "UNKNOWN";
}

__attribute__((weak)) char const *MechInputTouchButton::GetDebugText() {
    return "";
}

void MechInputTouchButton::Reset() {
    if (has_pending_touches) {
        possible_triggering_touch_ids[0] = 0xff;
        possible_triggering_touches[0] = false;
        possible_triggering_touch_ids[1] = 0xff;
        possible_triggering_touches[1] = false;
        possible_triggering_touch_ids[2] = 0xff;
        possible_triggering_touches[2] = false;
        possible_triggering_touch_ids[3] = 0xff;
        possible_triggering_touches[3] = false;
        possible_triggering_touch_ids[4] = 0xff;
        possible_triggering_touches[4] = false;
        possible_triggering_touch_ids[5] = 0xff;
        possible_triggering_touches[5] = false;
        possible_triggering_touch_ids[6] = 0xff;
        possible_triggering_touches[6] = false;
        possible_triggering_touch_ids[7] = 0xff;
        possible_triggering_touches[7] = false;
        possible_triggering_touch_ids[8] = 0xff;
        possible_triggering_touches[8] = false;
        possible_triggering_touch_ids[9] = 0xff;
        possible_triggering_touches[9] = false;
        has_pending_touches = false;
    }

    if (touch_locked_by != 0xff) {
        ClearTouchLocked(false);
    }
}

void MechInputTouchButton::SetTouchLocked(u32 touch_id, bool allow_new) {
    touch_locked_by = touch_id;
    MechSystems::Get()->input_touch_system.SetTouchLockedBy(touch_id, this, allow_new);
}

MechInputTouchButtonFaker::MechInputTouchButtonFaker(i32, u32, float, float, float, float) {
}

void MechInputTouchButtonFaker::Render() {
}

void MechInputTouchButtonFaker::Update(NuInputTouchData const *) {
}

MechInputTouchMainDummyStick::MechInputTouchMainDummyStick(MechInputTouchMainController &main_controller,
                                                           NuTouchInputElement::TYPE type)
    : NuTouchInputElement(type, colourPurple, 0), controller(&main_controller) {
}

MechInputTouchMainDummyButton::MechInputTouchMainDummyButton(MechInputTouchMainController &main_controller, u32 id,
                                                             MechInputTouchMainController::eButtonTypes type)
    : NuTouchInputElement(TYPE_BUTTON, colourPurple, id, 0.0f, 0.0f, 0.0f, 0.0f), controller(&main_controller),
      button_type(static_cast<u32>(type)) {
}

MechInputTouchButtonControlled::MechInputTouchButtonControlled(MechInputTouchMainController &, i32 index)
    : MechInputTouchButton(NuTouchInputElement::TYPE(), 2, 0), controller_index(index) {
}

__attribute__((weak)) bool MechInputTouchButtonControlled::ControlledUpdate(NuInputTouchData const *) {
    return has_pending_touches;
}

__attribute__((weak)) void MechInputTouchButtonControlled::ControlledRender() {
}

__attribute__((weak)) void MechInputTouchButtonControlled::Reset() {
    ControlledReset();
}

__attribute__((weak)) void MechInputTouchButtonControlled::ControlledReset() {
    MechInputTouchButton::Reset();
}
