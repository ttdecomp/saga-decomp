#include "gameapi_edtools_types.h"
#include "gameapi/edtools/edui.h"
#include "legoapi/legoapi_types.h"

extern "C" {
    extern part_typedesc_s *edpart_nearest_type;
    i32 edpart_set_part = 5;
}

// Particle editor subsystem stubs (static, internal linkage).

static void edpartInit() {
}
static void edpartProc(float, nupad_s *) {
}
static void edpartApply() {
}
static void edpartClose() {
}
static void edpartEnter() {
}
static void edpartRender() {
}
static void edpartSelType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartCopyType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartDataMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartEmitMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartGravMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartMoveList(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartTintMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeGrav(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeName(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartCutOffMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartDeleteType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSScaleMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetSoundID(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSoundXMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSoundsMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSwitchMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeTintB(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeTintG(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeTintR(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartEmitVelMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetSwitchId(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSoundIDMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartVarEmitMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartAddLevelType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeBounce(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeCutOff(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeIvalOn(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeSScale(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartEmitTimeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartInstanceMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartToggleFilter(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartVarStartMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartCancelOptMenu(eduimenu_s *, eduimenu_s *) {
}
static void edpartChangeEmitVel(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeGenRate(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeIvalOff(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeMaxLife(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeVarEmit(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartDieDebrisMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartLevelTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartScaleTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetSwitchType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartAddGeneralType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartApplyScaleType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartCancelDataMenu(eduimenu_s *, eduimenu_s *) {
}
static void edpartCancelEmitMenu(eduimenu_s *, eduimenu_s *) {
}
static void edpartCancelGravMenu(eduimenu_s *, eduimenu_s *) {
}
static void edpartCancelTintMenu(eduimenu_s *, eduimenu_s *) {
}
static void edpartCancelTypeMenu(eduimenu_s *, eduimenu_s *) {
}
static void edpartChangeNameMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeVarStart(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartImpactPartMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetScaleFactor(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSwitchTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangeIvalOnRan(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartChangePartIndex(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpart_nearest_type != NULL && edpart_set_part == 5) {
        edpart_nearest_type->impact_part = item->data;
    }
}
static void edpartDebrisScaleMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartFileLoadEffects(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartFileSaveEffects(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartGeneralTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetDistribution(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetInstanceType(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartSetSoundControl(eduimenu_s *, eduiitem_s *, u32) {
}
static void edpartCancelCutOffMenu(eduimenu_s *, eduimenu_s *) {
}
static void edppRender() {
}
