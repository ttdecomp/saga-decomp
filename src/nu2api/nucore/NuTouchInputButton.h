#pragma once

#include "nu2api/nucore/NuTouchInputElement.h"

struct NuInputTouchData;

struct NuTouchInputButton : NuTouchInputElement {
    NuTouchInputButton(i32, u32);
    NuTouchInputButton(i32, u32, float, float, float, float);
    void Render() override;
    void Update(NuInputTouchData const *) override;
    u8 IsPressed() const override {
        return pressed;
    }
    bool pressed;
    u32 captured_touch_id;
    bool touch_captured;
};
