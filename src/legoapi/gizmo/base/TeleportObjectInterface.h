#pragma once
#include "MechInputTouch/MechInputTouch_types.h"

struct VuVec;
struct TELEPORT_s;

struct TeleportObjectInterface : MechObjectInterface {
    TELEPORT_s &teleport;
    i32 index;

    TeleportObjectInterface(TELEPORT_s &, i32);
    void GetPos(VuVec &, i32) const override;
    f32 GetRadius() const override;
    const char *GetTargetName() const override;
    i32 GetObjectType() const override {
        return 8;
    }
    void *GetTgtVoidPtr() override {
        return &teleport;
    }
    TELEPORT_s *GetTeleport() override {
        return &teleport;
    }
    void TargetedFlash() override;
    ~TeleportObjectInterface() override;
};
DECOMP_ASSERT(sizeof(TeleportObjectInterface) == 0x10, "Teleport interface ABI");
