#include "legoapi/legoapi_types.h"
#include "legoapi/core/input/gamepads.h"

bool ClickToPressStartGestureTracker::OnClick(GameObject_s &, TouchHolder &) {
    GamePad[0].buttons_down_08 |= GAMEPAD_START;
    MechSystems::SkipTextScroll = 1;
    return true;
}
