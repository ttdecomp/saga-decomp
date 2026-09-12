#include "decomp.h"
#include "gameapi/edtools/edui.h"
#include "nu2api/nu3d/nuqfnt.h"
#include "gameapi/edtools/gameapi_edtools_types.h"
#include "legoapi/render/core/render.h"
#include "legoapi/render/core/rtl.h"

struct nuvtx_tc1_s;
struct numtl_s;
struct numtx_s;

typedef rtlfog_s EDRTLFOG_s;

static i32 numsegs = 16;
static i32 curFogLoc = -1;

static EDRTLFOG_s *SelectPrevFog() {
    i32 index;
    i32 count = 0;
    index = curFogLoc - 1;
    if (index < 0) {
        index = 32;
    }
    if (curr_set != NULL) {
        while (index != curFogLoc) {
            if (curr_set->fog[index].type != 0) {
                curFogLoc = index;
                return &curr_set->fog[index];
            }
            if (index == 0) {
                index = 32;
            }
            ++count;
            if (count > 31) {
                break;
            }
            --index;
        }
    }
    return NULL;
}

static EDRTLFOG_s *SelectNextFog() {
    i32 index;
    i32 count = 0;
    if (curFogLoc == -1 || curFogLoc == 31) {
        index = 0;
    } else {
        index = curFogLoc + 1;
    }
    if (curr_set != NULL) {
        while (index != curFogLoc) {
            if (curr_set->fog[index].type != 0) {
                curFogLoc = index;
                return &curr_set->fog[index];
            }
            if (index > 31) {
                index = -1;
            }
            ++count;
            if (count > 31) {
                break;
            }
            ++index;
        }
    }
    return NULL;
}

// RTL editor subsystem stubs (static, internal linkage).

static void edrtlClose() {
}
static void edrtlEnter() {
}
static void edrtlLeave() {
}
static void edrtlRender() {
}
static void edrtlProcFog(float, nupad_s *) {
}
static void edrtlProcRTL(float, nupad_s *) {
}
static void edrtlDrawFogs() {
}

extern "C" void edrtlDrawFog(EDRTLFOG_s *fog) {
    if (fog != NULL) {
        i32 colour = (fog->colour & 0xffffff) | 0x80000000;
        switch (fog->type) {
            default:
                break;
            case 1:
                RndrOSphere(&fog->position, fog->radius, colour, numsegs, 0);
                break;
        }
    }
}
static void edrtlDrawHelp() {
}
static void edrtlProcBurn(float, nupad_s *) {
}
static void edrtlSaveUndo() {
}
static void edrtlDrawCursor() {
}
static void edrtlDrawLights() {
}
static void edrtlRndrLine3d(nuvtx_tc1_s *, numtl_s *, numtx_s *) {
}
static void edrtlBurnSetMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlDrawFogInfo() {
}
static void edrtlDrawRTLInfo() {
}
static void edrtlBurnMainMenu() {
}
static void edrtlDrawBurnInfo() {
}
static void edrtlDrawBurnouts() {
}
static void edrtlSetBurnRadius(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlBurnRadiusMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlInvalidateUndo() {
}
static void edrtlSetBurnFalloff(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlBurnoutFileLoad(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlBurnoutFileSave(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlSetBurnsetFlare(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlBurnDefaultsMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static void edrtlSetBurnsetRadius(eduimenu_s *, eduiitem_s *, u32) {
}

extern NUQFNT *system_qfont;
extern "C" {
    void CreateColourPicker();
    void cbCancelSubMenu(eduimenu_s *, eduimenu_s *);
    void cbCancelSubMenuFromItem(eduimenu_s *, eduiitem_s *, u32);
    void cbTriggerSubMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbModifierAdjust(eduimenu_s *, eduiitem_s *, u32);
    void cbNearClipAtCursor(eduimenu_s *, eduiitem_s *, u32);
    void eduiMenuFitWidth(eduimenu_s *, i32);
    eduiitem_s *eduiItemSelCreate(usize, u32 *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    eduiitem_s *eduiItemCheckCreate(usize, u32 *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    eduiitem_s *eduiItemToggleCreate(usize, u32 *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    eduiitem_s *eduiItemSliderCreate(usize, u32 *, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), f32, f32, f32,
                                     char *);
    eduiitem_s *eduiItemSliderCreateInt(usize, u32 *, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), i32, i32, i32,
                                        char *);
}

f32 edrtl_text_scale = 1.0f;
static eduimenu_s *associd_menu;
static char *camdir_id_names[16] = {"ID 0", "ID 1", "ID 2",  "ID 3",  "ID 4",  "ID 5",  "ID 6",  "ID 7",
                                    "ID 8", "ID 9", "ID 10", "ID 11", "ID 12", "ID 13", "ID 14", "ID 15"};
static eduiitem_s *associd_items[16];
static eduimenu_s *excludeid_menu;
static eduiitem_s *excludeid_items[16];
static eduimenu_s *userid_menu;
static char *userid_names[16] = {"NO ID", "ID 1", "ID 2",  "ID 3",  "ID 4",  "ID 5",  "ID 6",  "ID 7",
                                 "ID 8",  "ID 9", "ID 10", "ID 11", "ID 12", "ID 13", "ID 14", "ID 15"};
static eduiitem_s *userid_items[16];
static eduimenu_s *type_menu;
static eduiitem_s *point_item;
static eduiitem_s *jonflicker_item;
static eduiitem_s *pointflicker_item;
static eduiitem_s *pointblend_item;
static eduiitem_s *ambient_item;
static eduiitem_s *directional_item;
static eduiitem_s *antilight_item;
static eduiitem_s *camdir_item;
static eduimenu_s *modifier_menu;
static char *default_modifier_names[] = {"default"};
char **modifier_names = default_modifier_names;
static eduiitem_s *modifier_items[32];
eduimenu_s *modifier_adj_menu;
static eduiitem_s *modifier_adj_items[32];
static eduimenu_s *flicker_menu;
static eduiitem_s *t_hi_item;
static eduiitem_s *rt_hi_item;
static eduiitem_s *t_low_item;
static eduiitem_s *rt_low_item;
static eduimenu_s *jonflicker_menu;
static eduiitem_s *jon_t_hi_item;
static eduiitem_s *jon_rt_hi_item;
static eduiitem_s *jon_t_low_item;
static eduimenu_s *prop_menu;
static eduiitem_s *colour_item;
static eduiitem_s *flicker_item;
static eduiitem_s *associd_item;
static eduiitem_s *multiplier_item;
static eduiitem_s *groupid_item;
static eduiitem_s *modifierid_item;
static eduiitem_s *castshadow_item;
static eduiitem_s *hasspecular_item;
static eduimenu_s *hide_menu;
static eduiitem_s *hide_point_item;
static eduiitem_s *hide_pointflicker_item;
static eduiitem_s *hide_pointblend_item;
static eduiitem_s *hide_ambient_item;
static eduiitem_s *hide_directional_item;
static eduiitem_s *hide_antilight_item;
static eduiitem_s *hide_camdir_item;
static eduimenu_s *delete_menu;
static eduimenu_s *global_confirm_menu;
static eduimenu_s *global_menu;
static eduiitem_s *global_scale_item;
static eduimenu_s *load_confirm_menu;
static eduimenu_s *main_menu;
static eduiitem_s *prop_item;
static eduiitem_s *copy_item;
static eduiitem_s *paste_item;
static eduiitem_s *pasteinto_item;
static eduiitem_s *copytogroup_item;
static eduiitem_s *undo_item;
static eduiitem_s *redo_item;
static eduimenu_s *fog_menu;
static eduiitem_s *fogcol_item;
static eduiitem_s *fogalpha_item;
static eduiitem_s *fogdensity_item;
static eduiitem_s *fogdensitywii_item;
static eduiitem_s *fogstart_item;
static eduiitem_s *fogend_item;
static eduiitem_s *fogstartpsp_item;
static eduiitem_s *fogendpsp_item;
static eduiitem_s *hazecol_item;
static eduiitem_s *hazedensity_item;
static eduiitem_s *blurdensity_item;
static eduiitem_s *fogadjrng_item;
static eduiitem_s *fogadjnear_item;
static eduiitem_s *fogadjfar_item;
static eduiitem_s *dof_fstop_item;
static eduimenu_s *fog_main_menu;
static eduiitem_s *fog_item;
static eduiitem_s *fog_copy_item;
static eduiitem_s *fog_paste_item;
static eduiitem_s *fog_pasteinto_item;
i32 modifier_cnt = 1;
static void cbAssocID(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbExcludeID(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbUserID(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbLightType(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbModifierType(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbLowColour(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbHighTime(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbRHighTime(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbLowTime(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbRLowTime(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbCancelLightProperties(eduimenu_s *, eduimenu_s *) {
}
static void cbHighColour(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbMultiplier(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbGroupID(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbToggleCastShadow(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbToggleHasSpecular(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbHideType(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbCancelDeleteMenu(eduimenu_s *, eduimenu_s *) {
}
static void cbDeleteYes(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbDeleteNo(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbScaleAllMultipliersUp(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbScaleAllMultipliersDown(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbLoad(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbCancelMenu(eduimenu_s *, eduimenu_s *) {
}
static void cbLightProperties(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbCopyLight(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbPasteLight(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbPasteIntoLight(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbCopyToGroup(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbUndoLight(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbRedoLight(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbNoZBuffer(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbSave(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbSetControls(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogColour(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogAlpha(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogDensity(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogDensityWii(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogStart(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogEnd(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogStartPSP(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogEndPSP(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbHazeColour(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbHazeDensity(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbBlurDensity(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogAdjRng(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogAdjNear(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbFogAdjFar(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbDOFFStop(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbCopyFog(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbPasteFog(eduimenu_s *, eduiitem_s *, u32) {
}
static void cbPasteIntoFog(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void InitUI() {
    static i32 initialised;
    i32 i;
    u32 colours[4] = {0x80000000, 0x800000ff, 0x80808080, 0x80f0f0f0};
    if (initialised)
        return;
    NuQFntPushCoordinateSystem(static_cast<NUQFNT_CSMODE>(1));
    NuQFntSetScale(system_qfont, edrtl_text_scale, edrtl_text_scale);
    initialised = 1;
    CreateColourPicker();
    associd_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Assoc ID");
    for (i = 0; i <= 15; ++i) {
        associd_items[i] =
            eduiMenuAddItem(associd_menu, eduiItemToggleCreate(i, colours, 0, (i + 1), cbAssocID, camdir_id_names[i]));
    }
    eduiMenuFitWidth(associd_menu, 5);
    excludeid_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Exclude ID");
    for (i = 0; i <= 15; ++i) {
        excludeid_items[i] = eduiMenuAddItem(
            excludeid_menu, eduiItemToggleCreate(i, colours, 0, (i + 1), cbExcludeID, camdir_id_names[i]));
    }
    eduiMenuFitWidth(excludeid_menu, 5);
    userid_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "User ID");
    for (i = 0; i <= 15; ++i) {
        userid_items[i] =
            eduiMenuAddItem(userid_menu, eduiItemCheckCreate(i, colours, 0, 1, cbUserID, userid_names[i]));
    }
    type_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Light Type");
    point_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(2, colours, 0, 2, cbLightType, "Point"));
    jonflicker_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(8, colours, 0, 2, cbLightType, "Jon Flicker"));
    pointflicker_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(3, colours, 0, 2, cbLightType, "Point Flicker"));
    pointblend_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(6, colours, 0, 2, cbLightType, "Point Blend"));
    ambient_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(1, colours, 0, 2, cbLightType, "Ambient"));
    directional_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(4, colours, 0, 2, cbLightType, "Directional"));
    antilight_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(7, colours, 0, 2, cbLightType, "Antilight"));
    camdir_item = eduiMenuAddItem(type_menu, eduiItemCheckCreate(5, colours, 0, 2, cbLightType, "Camdir"));
    eduiMenuFitWidth(type_menu, 5);
    modifier_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Modifiers");
    for (i = 0; i < modifier_cnt; ++i) {
        modifier_items[i] =
            eduiMenuAddItem(modifier_menu, eduiItemCheckCreate(i, colours, 0, 2, cbModifierType, modifier_names[i]));
    }
    eduiMenuFitWidth(modifier_menu, 5);
    modifier_adj_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Adjust Modifiers");
    for (i = 0; i < modifier_cnt; ++i) {
        modifier_adj_items[i] =
            eduiMenuAddItem(modifier_adj_menu, eduiItemSliderCreate(i, colours, 0, &cbModifierAdjust, 0.0f, 1.0f, 1.0f,
                                                                    modifier_names[i]));
        eduiItemSliderSetGranularity(static_cast<edui_slider_s *>(modifier_adj_items[i]), 0.10000000149011612f);
    }
    eduiMenuFitWidth(modifier_adj_menu, 5);
    flicker_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Flicker Light Options");
    eduiMenuAddItem(flicker_menu, eduiItemSelCreate(0, colours, 0, 1, cbLowColour, "Low Colour..."));
    t_hi_item =
        eduiMenuAddItem(flicker_menu, eduiItemSliderCreate(0, colours, 0, cbHighTime, 0.0f, 4.0f, 4.0f, "High Time"));
    rt_hi_item = eduiMenuAddItem(
        flicker_menu, eduiItemSliderCreate(0, colours, 0, cbRHighTime, 0.0f, 4.0f, 4.0f, "Random High Time"));
    t_low_item =
        eduiMenuAddItem(flicker_menu, eduiItemSliderCreate(0, colours, 0, cbLowTime, 0.0f, 4.0f, 4.0f, "Low Time"));
    rt_low_item = eduiMenuAddItem(flicker_menu,
                                  eduiItemSliderCreate(0, colours, 0, cbRLowTime, 0.0f, 4.0f, 4.0f, "Random Low Time"));
    eduiMenuFitWidth(flicker_menu, 5);
    jonflicker_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Flicker Light Options");
    eduiMenuAddItem(jonflicker_menu, eduiItemSelCreate(0, colours, 0, 1, cbLowColour, "Low Colour..."));
    jon_t_hi_item =
        eduiMenuAddItem(jonflicker_menu, eduiItemSliderCreate(0, colours, 0, cbHighTime, 0.0f, 10.0f, 4.0f, "Step"));
    jon_rt_hi_item = eduiMenuAddItem(
        jonflicker_menu, eduiItemSliderCreate(0, colours, 0, cbRHighTime, 0.0f, 10.0f, 4.0f, "Random Step"));
    jon_t_low_item =
        eduiMenuAddItem(jonflicker_menu, eduiItemSliderCreate(0, colours, 0, cbLowTime, 0.0f, 4.0f, 4.0f, "Duration"));
    eduiMenuFitWidth(jonflicker_menu, 5);
    prop_menu = eduiMenuCreate(100, 70, 180, 300, 0, cbCancelLightProperties, "Light Properties");
    eduiMenuAddItem(prop_menu,
                    eduiItemSelCreate(reinterpret_cast<usize>(type_menu), colours, 0, 1, &cbTriggerSubMenu, "Type..."));
    colour_item = eduiMenuAddItem(prop_menu, eduiItemSelCreate(0, colours, 0, 1, cbHighColour, "Colour..."));
    flicker_item = eduiMenuAddItem(prop_menu, eduiItemSelCreate(reinterpret_cast<usize>(flicker_menu), colours, 0, 1,
                                                                &cbTriggerSubMenu, "Flicker Light Options..."));
    associd_item = eduiMenuAddItem(prop_menu, eduiItemSelCreate(reinterpret_cast<usize>(associd_menu), colours, 0, 1,
                                                                &cbTriggerSubMenu, "Associate ID(s)..."));
    eduiMenuAddItem(prop_menu, eduiItemSelCreate(reinterpret_cast<usize>(excludeid_menu), colours, 0, 1,
                                                 &cbTriggerSubMenu, "Exclude ID(s)..."));
    eduiMenuAddItem(prop_menu, eduiItemSelCreate(reinterpret_cast<usize>(userid_menu), colours, 0, 1, &cbTriggerSubMenu,
                                                 "User ID..."));
    multiplier_item =
        eduiMenuAddItem(prop_menu, eduiItemSliderCreate(0, colours, 0, cbMultiplier, 0.5f, 127.0f, 1.0f, "Multiplier"));
    eduiItemSliderSetGranularity(static_cast<edui_slider_s *>(multiplier_item), 0.10000000149011612f);
    eduiItemSliderSetFmt(static_cast<edui_slider_s *>(multiplier_item), "%.1f");
    groupid_item = eduiMenuAddItem(prop_menu, eduiItemSliderCreateInt(0, colours, 0, cbGroupID, 0, 255, 0, "Group ID"));
    modifierid_item = eduiMenuAddItem(prop_menu, eduiItemSelCreate(reinterpret_cast<usize>(modifier_menu), colours, 0,
                                                                   1, &cbTriggerSubMenu, "Modifier..."));
    castshadow_item =
        eduiMenuAddItem(prop_menu, eduiItemToggleCreate(0, colours, 0, 2, cbToggleCastShadow, "Cast Shadow"));
    hasspecular_item =
        eduiMenuAddItem(prop_menu, eduiItemToggleCreate(0, colours, 0, 3, cbToggleHasSpecular, "Has Specular"));
    eduiMenuFitWidth(prop_menu, 5);
    hide_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Hide Lights");
    hide_point_item = eduiMenuAddItem(hide_menu, eduiItemToggleCreate(2, colours, 0, 1, cbHideType, "Point"));
    hide_pointflicker_item =
        eduiMenuAddItem(hide_menu, eduiItemToggleCreate(3, colours, 0, 2, cbHideType, "Point Flicker"));
    hide_pointblend_item =
        eduiMenuAddItem(hide_menu, eduiItemToggleCreate(6, colours, 0, 2, cbHideType, "Point Blend"));
    hide_ambient_item = eduiMenuAddItem(hide_menu, eduiItemToggleCreate(1, colours, 0, 3, cbHideType, "Ambient"));
    hide_directional_item =
        eduiMenuAddItem(hide_menu, eduiItemToggleCreate(4, colours, 0, 4, cbHideType, "Directional"));
    hide_antilight_item = eduiMenuAddItem(hide_menu, eduiItemToggleCreate(7, colours, 0, 4, cbHideType, "Antilight"));
    hide_camdir_item = eduiMenuAddItem(hide_menu, eduiItemToggleCreate(5, colours, 0, 4, cbHideType, "Camdir"));
    eduiMenuFitWidth(hide_menu, 5);
    delete_menu = eduiMenuCreate(100, 70, 180, 300, 0, cbCancelDeleteMenu, "Delete Light?");
    eduiMenuAddItem(delete_menu, eduiItemSelCreate(0, colours, 0, 1, cbDeleteYes, "Yes"));
    eduiMenuAddItem(delete_menu, eduiItemSelCreate(0, colours, 0, 1, cbDeleteNo, "No"));
    eduiMenuFitWidth(delete_menu, 5);
    global_confirm_menu = eduiMenuCreate(100, 70, 180, 300, 0, cbCancelDeleteMenu, "Global Change Applied");
    eduiMenuAddItem(global_confirm_menu, eduiItemSelCreate(0, colours, 0, 1, &cbCancelSubMenuFromItem, "Okay"));
    eduiMenuFitWidth(global_confirm_menu, 5);
    global_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Global Adjustments");
    global_scale_item =
        eduiMenuAddItem(global_menu, eduiItemSliderCreate(0, colours, 0, 0, 2.0f, 8.0f, 2.0f, "Scale Amount"));
    eduiItemSliderSetGranularity(static_cast<edui_slider_s *>(global_scale_item), 1.0f);
    eduiItemSliderSetFmt(static_cast<edui_slider_s *>(global_scale_item), "%.0f");
    eduiMenuAddItem(global_menu,
                    eduiItemSelCreate(0, colours, 0, 1, cbScaleAllMultipliersUp, "Scale All Multipliers Up"));
    eduiMenuAddItem(global_menu,
                    eduiItemSelCreate(0, colours, 0, 1, cbScaleAllMultipliersDown, "Scale All Multipliers Down"));
    eduiMenuFitWidth(global_menu, 5);
    load_confirm_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Load lights?");
    eduiMenuAddItem(load_confirm_menu, eduiItemSelCreate(0, colours, 0, 1, cbLoad, "Yes"));
    eduiMenuAddItem(load_confirm_menu, eduiItemSelCreate(0, colours, 0, 1, &cbCancelSubMenuFromItem, "No"));
    eduiMenuFitWidth(delete_menu, 5);
    main_menu = eduiMenuCreate(100, 70, 180, 300, 0, cbCancelMenu, "Light Editor Options");
    prop_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(reinterpret_cast<usize>(prop_menu), colours, 0, 1,
                                                             cbLightProperties, "Light Properties..."));
    copy_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbCopyLight, "Copy"));
    paste_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbPasteLight, "Paste New"));
    pasteinto_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbPasteIntoLight, "Paste Into"));
    copytogroup_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbCopyToGroup, "Copy To Group"));
    undo_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbUndoLight, "Undo"));
    redo_item = eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbRedoLight, "Redo"));
    undo_item->disabled = 1;
    redo_item->disabled = 1;
    eduiMenuAddItem(main_menu, eduiItemToggleCreate(0, colours, 0, 2, cbNoZBuffer, "No Z Buffer"));
    eduiMenuAddItem(main_menu, eduiItemToggleCreate(0, colours, 0, 3, &cbNearClipAtCursor, "Near Clip At Cursor"));
    eduiMenuAddItem(main_menu, eduiItemSelCreate(reinterpret_cast<usize>(modifier_adj_menu), colours, 0, 1,
                                                 &cbTriggerSubMenu, "Adjust Modifiers..."));
    eduiMenuAddItem(main_menu, eduiItemSelCreate(reinterpret_cast<usize>(hide_menu), colours, 0, 1, &cbTriggerSubMenu,
                                                 "Hide Lights..."));
    eduiMenuAddItem(main_menu, eduiItemSelCreate(reinterpret_cast<usize>(global_menu), colours, 0, 1, &cbTriggerSubMenu,
                                                 "Global Adjustments..."));
    eduiMenuAddItem(main_menu, eduiItemSelCreate(0, colours, 0, 1, cbSave, "Save Lights/Fog"));
    eduiMenuAddItem(main_menu, eduiItemSelCreate(reinterpret_cast<usize>(load_confirm_menu), colours, 0, 1,
                                                 &cbTriggerSubMenu, "Load Lights/Fog"));
    eduiMenuAddItem(main_menu, eduiItemCheckCreate(1, colours, 1, 4, cbSetControls, "Ralph Controls"));
    eduiMenuAddItem(main_menu, eduiItemCheckCreate(0, colours, 0, 4, cbSetControls, "Steve Controls"));
    eduiMenuFitWidth(main_menu, 5);
    fog_menu = eduiMenuCreate(100, 70, 180, 300, 0, &cbCancelSubMenu, "Fog Settings");
    fogcol_item = eduiMenuAddItem(fog_menu, eduiItemSelCreate(0, colours, 0, 1, cbFogColour, "Fog Colour..."));
    fogalpha_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogAlpha, 0.0f, 128.0f, 1.0f, "Fog Density (alpha)"));
    fogdensity_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogDensity, 0.0f, 4.0f, 1.0f, "Fog Density (new)"));
    fogdensitywii_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogDensityWii, 0.0f, 4.0f, 1.0f, "Fog Density (Wii)"));
    static_cast<edui_slider_s *>(fogdensity_item)->granularity = 1.9999999494757503e-05f;
    fogstart_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogStart, 0.0f, 2000.0f, 4.0f, "Fog Start (PS2)"));
    fogend_item =
        eduiMenuAddItem(fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogEnd, 0.0f, 2000.0f, 4.0f, "Fog End (PS2)"));
    fogstartpsp_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogStartPSP, 0.0f, 2000.0f, 4.0f, "Fog Start (PSP)"));
    fogendpsp_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogEndPSP, 0.0f, 2000.0f, 4.0f, "Fog End (PSP)"));
    static_cast<edui_slider_s *>(fogstart_item)->granularity = 0.125f;
    static_cast<edui_slider_s *>(fogend_item)->granularity = 10.0f;
    static_cast<edui_slider_s *>(fogstartpsp_item)->granularity = 0.125f;
    static_cast<edui_slider_s *>(fogendpsp_item)->granularity = 10.0f;
    hazecol_item = eduiMenuAddItem(fog_menu, eduiItemSelCreate(0, colours, 0, 1, cbHazeColour, "Haze Colour..."));
    hazedensity_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbHazeDensity, 0.0f, 128.0f, 1.0f, "Haze Density"));
    blurdensity_item =
        eduiMenuAddItem(fog_menu, eduiItemSliderCreate(0, colours, 0, cbBlurDensity, 0.0f, 1.0f, 1.0f, "Blur Density"));
    fogadjrng_item = eduiMenuAddItem(
        fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogAdjRng, 0.0f, 2000.0f, 1000.0f, "Fog Distance Range"));
    fogadjnear_item =
        eduiMenuAddItem(fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogAdjNear, 0.0010000000474974513f, 2000.0f,
                                                       0.10000000149011612f, "Near Clip (editor only)"));
    fogadjfar_item = eduiMenuAddItem(fog_menu, eduiItemSliderCreate(0, colours, 0, cbFogAdjFar, 0.0010000000474974513f,
                                                                    5000.0f, 1000.0f, "Far Clip (editor only)"));
    dof_fstop_item =
        eduiMenuAddItem(fog_menu, eduiItemSliderCreate(0, colours, 0, cbDOFFStop, 1.0f, 21.0f, 1.0f, "DOF - F-Stop"));
    static_cast<edui_slider_s *>(dof_fstop_item)->granularity = 0.10000000149011612f;
    eduiMenuFitWidth(fog_menu, 5);
    fog_main_menu = eduiMenuCreate(100, 70, 180, 300, 0, cbCancelMenu, "Fog Editor Options");
    fog_item = eduiMenuAddItem(fog_main_menu, eduiItemSelCreate(reinterpret_cast<usize>(fog_menu), colours, 0, 1,
                                                                &cbTriggerSubMenu, "Fog Settings..."));
    fog_copy_item = eduiMenuAddItem(fog_main_menu, eduiItemSelCreate(0, colours, 0, 1, cbCopyFog, "Copy"));
    fog_paste_item = eduiMenuAddItem(fog_main_menu, eduiItemSelCreate(0, colours, 0, 1, cbPasteFog, "Paste New"));
    fog_pasteinto_item =
        eduiMenuAddItem(fog_main_menu, eduiItemSelCreate(0, colours, 0, 1, cbPasteIntoFog, "Paste Into"));
    eduiMenuAddItem(fog_main_menu, eduiItemSelCreate(0, colours, 0, 1, cbSave, "Save Lights/Fog"));
    eduiMenuAddItem(fog_main_menu, eduiItemSelCreate(0, colours, 0, 1, cbLoad, "Load Lights/Fog"));
    eduiMenuAddItem(fog_main_menu, eduiItemCheckCreate(1, colours, 1, 4, cbSetControls, "Ralph Controls"));
    eduiMenuAddItem(fog_main_menu, eduiItemCheckCreate(0, colours, 0, 4, cbSetControls, "Steve Controls"));
    eduiMenuFitWidth(fog_main_menu, 5);
    NuQFntPopCoordinateSystem();
}
