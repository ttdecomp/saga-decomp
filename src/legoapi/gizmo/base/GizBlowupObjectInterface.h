#pragma once
#include "MechInputTouch/MechInputTouch_types.h"

struct VuVec;
struct GIZMOBLOWUP_s;
struct WORLDINFO_s;

struct GizBlowupObjectInterface : MechObjectInterface {
    GizBlowupObjectInterface(GIZMOBLOWUP_s &);
    void GetPos(VuVec &, i32) const override;
    f32 GetRadius() const override;
    const char *GetTargetName() const override;
    i32 GetObjectType() const override {
        return 4;
    }
    bool IsDead() override;
    void TargetedFlash() override;
    void *GetTgtVoidPtr() override {
        return blowup;
    }
    GIZMOBLOWUP_s *GetGizBlowup() override {
        return blowup;
    }
    virtual ~GizBlowupObjectInterface();
    GIZMOBLOWUP_s *blowup;
};
DECOMP_ASSERT(sizeof(GizBlowupObjectInterface) == 12, "GizBlowupObjectInterface ABI");

GIZMOBLOWUP_s *GizmoBlowUp_FindByName(WORLDINFO_s *, char *);
GIZMOBLOWUP_s *GizmoBlowUp_FindFromPlatID(WORLDINFO_s *, i32);
GIZMOBLOWUP_s *GizmoBlowUp_Target(GameObject_s *, nuvec_s *, nuvec_s *, f32, f32, i32, i32, i32);
