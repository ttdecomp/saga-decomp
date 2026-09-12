#pragma once

#include "nu2api/nucore/fixed_width.h"

struct NuInputTouchData;

struct NuTouchInputElement {
    enum TYPE : u32 { TYPE_LEFT_STICK = 0, TYPE_RIGHT_STICK = 1, TYPE_BUTTON = 2 };
    NuTouchInputElement(NuTouchInputElement::TYPE, i32, u32);
    NuTouchInputElement(NuTouchInputElement::TYPE, i32, u32, float, float, float, float);

    virtual ~NuTouchInputElement() {
    }
    virtual void Render() = 0;
    virtual void UpdateButtons(i32) {
    }
    virtual void Update(NuInputTouchData const *) = 0;
    virtual u8 IsPressed() const {
        return 0;
    }
    virtual float GetStickX() const {
        return 0.0f;
    }
    virtual float GetStickY() const {
        return 0.0f;
    }
    virtual void Deactivate() {
    }
    virtual void Activate() {
    }

    float x;
    float y;
    float width;
    float height;
    u32 field_0x14;
    u32 field_0x18;
    u32 field_0x1c;
    u32 field_0x20;
    u32 id;
    i32 index;
    u32 type;
};
