#include "gameapi_edtools_types.h"
#include "gameapi/edtools/edui.h"

extern edanim_param_s AnimParams[64];
extern i32 edanim_nearest;
extern i32 edanim_nearest_param_id;
extern i32 edanim_nearest_particle;
extern i32 edanim_nearest_sound;
extern i32 edanim_particle_mode;
extern i32 edanim_particle_type;
extern i32 edanim_sound_type;

// Animation editor subsystem stubs (static, internal linkage).

static void edanimInit() {
}
static void edanimProc(float, nupad_s *) {
}
static void edanimClose() {
}
static void edanimEnter() {
    edanim_nearest = -1;
    edanim_nearest_param_id = -1;
    edanim_nearest_particle = -1;
    edanim_particle_mode = 0;
    edanim_particle_type = -1;
    edanim_sound_type = -1;
}
static void edanimRender() {
}
static void edanimcbCubeMap(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbFileLoad(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbFileSave(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbMCTBMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSoundMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbBouncyMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSwitchMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSetSwitchId(eduimenu_s *, eduiitem_s *item, u32) {
    if (edanim_nearest == -1 || edanim_nearest_param_id == -1) {
        return;
    }
    AnimParams[edanim_nearest_param_id].field_010 = static_cast<i32>(static_cast<edui_slider_s *>(item)->value);
}
static void edanimcbMCTBCardType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbParticleMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSetSoundType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSetSwitchVar(eduimenu_s *, eduiitem_s *item, u32) {
    if (edanim_nearest == -1 || edanim_nearest_param_id == -1) {
        return;
    }
    AnimParams[edanim_nearest_param_id].field_014 = static_cast<edui_slider_s *>(item)->value;
}
static void edanimcbCancelOptMenu(eduimenu_s *, eduimenu_s *) {
}
static void edanimcbSetSwitchType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSoundTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbCancelMCTBMenu(eduimenu_s *, eduimenu_s *) {
}
static void edanimcbLocalSoundMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbMCTBCardFormat(eduimenu_s *, eduiitem_s *, u32) {
}
static void edanimcbSetSoundTiming(eduimenu_s *, eduiitem_s *item, u32) {
    AnimParams[edanim_nearest_param_id].sound_values[edanim_nearest_sound] = static_cast<edui_slider_s *>(item)->value;
}
static void edanimcbSetSwitchDelay(eduimenu_s *, eduiitem_s *item, u32) {
    if (edanim_nearest == -1 || edanim_nearest_param_id == -1) {
        return;
    }
    AnimParams[edanim_nearest_param_id].field_018 = static_cast<edui_slider_s *>(item)->value;
}
static void edanimcbSwitchTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
