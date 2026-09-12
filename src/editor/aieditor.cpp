
#include "decomp.h"
#include "editor/edpath.h"
#include "gameapi/ai/aisys/aisys.h"
#include "gameapi/edtools/edui.h"
#include "gameapi/edtools/edcam.h"
#include "legoapi/items/base/apiobject.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/numtl.h"

#include <string.h>

extern "C" void aieditor_ClearMainMenu(void);
extern "C" void aieditor_SetMode(i32 mode);
extern "C" void AISYSRebuildFromEditorData(void);
extern "C" i32 aieditor_Register(const char *, void (*)(), void (*)(), void (*)(), void (*)());
extern "C" void aieditor_RegisterDefaultPathCnxTypes();
extern "C" eduiitem_s *eduiItemCheckCreate(i32, const void *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32),
                                           char *);
extern "C" eduiitem_s *eduiItemSelCreate(i32, const void *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32),
                                         char *);
extern "C" void aieditor_cbCancelMainMenu(eduimenu_s *, eduimenu_s *);

struct nupad_s;
void pathEditor_Enter();
void pathEditor_Render(i32, i32, f32, f32);
void routeEditor_Process(nupad_s *);
void routeEditor_Render(i32, i32, f32, f32);
void areaEditor_Enter();
void areaEditor_Process(nupad_s *);
void areaEditor_Render(i32, i32, f32, f32);
void locatorEditor_Enter();
void locatorEditor_Process(nupad_s *);
void locatorEditor_Render(i32, i32, f32, f32);
void creatureEditor_Enter();
void creatureEditor_Process(nupad_s *);
void creatureEditor_Render(i32, i32, f32, f32);
void antinodeEditor_Enter();
void antinodeEditor_Process(nupad_s *);
void antinodeEditor_Render(i32, i32, f32, f32);

extern "C" {
i32 AIEDITOR_PATHS;
i32 AIEDITOR_ROUTES = -1;
i32 AIEDITOR_AREAS;
i32 AIEDITOR_LOCATORS;
i32 AIEDITOR_CREATURES;
i32 AIEDITOR_ANTINODES;
}

aieditor_settings_s aieditorsettings;
extern "C" {
extern void *ed_fnt;
}
struct EditorItemColours {
    u32 normal;
    u32 selected;
    u32 disabled;
    u32 background;
};
static EditorItemColours attr = {0x80000000, 0x80ff0000, 0x80808080, 0x80404040};
struct AIPATHCNXTYPE_s {
    u32 connection_flag;
    void *context;
    char name[0x40];
    u32 flags;
};
DECOMP_ASSERT(sizeof(AIPATHCNXTYPE_s) == 0x4c, "editor path connection type size");
static i32 naipathcnxtypes;
static AIPATHCNXTYPE_s aipathcnxtypes[32];
static __used__ void aieditor_cbSetEditorMode(eduimenu_s *, eduiitem_s *item, unsigned int) {
    if ((u32)item->data < (u32)aieditorsettings.mode_count) {
        aieditor_SetMode(item->data);
        aieditor_ClearMainMenu();
    }
}

static __used__ void aieditor_cbCancelSelectEditorMode(eduimenu_s *menu, eduimenu_s *) {
    eduiMenuDestroy(menu);
}

static __used__ void aieditor_cbCancelSaveMenu(eduimenu_s *, eduiitem_s *, unsigned int) {
    aieditor_ClearMainMenu();
}

extern "C" {

    typedef void AIEDITORMOVEPLAYERS(NUVEC *position);
    AIEDITORMOVEPLAYERS *AIEditorMovePlayersFn;

    void aieditor_SetCurrentScript(char *, const AIEditorScriptSelection *);
    void aieditor_RegisterPathCnxType(const char *, u32, void *, u32);

    void InitFn_AIEditorMovePlayers(AIEDITORMOVEPLAYERS *function) {
        AIEditorMovePlayersFn = function;
    }

    eduimenu_s *aieditor_AddMessage(char *title, char *message) {
        eduimenu_s *outer = eduiMenuCreate(0xc8, 0x46, 0xf0, 0x10e, ed_fnt, aieditor_cbCancelMainMenu, title);
        if (outer != nullptr) {
            eduimenu_s *inner = eduiMenuCreate(0x78, 0x5a, 0x1b8, 0xfa, ed_fnt, nullptr, title);
            if (inner != nullptr) {
                eduiitem_s *item = eduiItemSelCreate(1, &attr, 0, 0, aieditor_cbCancelSaveMenu, message);
                eduiMenuAddItem(inner, item);
                eduiMenuAttach(outer, inner);
            }
        }
        return outer;
    }

    void aieditor_ClearAllPathCnxTypes(void) {
        naipathcnxtypes = 0;
        memset(aipathcnxtypes, 0, sizeof(aipathcnxtypes));
    }

    void aieditor_ClearMainMenu(void) {
        if (aieditor->main_menu != nullptr) {
            eduiMenuDestroy(aieditor->main_menu);
            aieditor->main_menu = nullptr;
        }
    }

    void aieditor_Enter(AISYS_s *system, NUVEC *camera_position, void *context, void *owner, void *extra_1,
                        void *extra_2, void *extra_3) {
        AIEDITOR_RENDER_STATE *state = aieditor;
        state->flags |= 2;
        state->ai_system = system;
        state->enter_context = context;
        state->enter_owner = owner;
        state->enter_extra_1 = extra_1;
        state->enter_extra_2 = extra_2;
        state->enter_extra_3 = extra_3;
        if (camera_position != nullptr) {
            edcamSetPos(camera_position);
        }
        for (i32 index = 0; index < aieditorsettings.mode_count; ++index) {
            void (*enter)() = aieditorsettings.modes[index].enter;
            if (enter != nullptr) {
                enter();
            }
        }
        aieditorsettings.snap_height_display = 1;
    }

    void aieditor_Init(AIEDITOR_RENDER_STATE *state, void **external_display_a, void **external_display_b) {
        aieditor = state;
        memset(&aieditorsettings, 0, sizeof(aieditorsettings));

        NUMTL *path_material = NuMtlCreate3D(1);
        aieditorsettings.path_material = path_material;
        path_material->opacity = 1.0f;
        path_material->diffuse_color.r = 0.5f;
        path_material->diffuse_color.g = 0.5f;
        path_material->diffuse_color.b = 0.5f;
        path_material->attribs.alpha_mode = 0;
        path_material->attribs.cull_mode = 2;
        path_material->attribs.z_mode = 0;
        NuMtlUpdate(path_material);

        NUMTL *route_material = NuMtlCreate3D(1);
        aieditorsettings.route_material = route_material;
        route_material->opacity = 1.0f;
        route_material->diffuse_color.r = 0.5f;
        route_material->diffuse_color.g = 0.5f;
        route_material->diffuse_color.b = 0.5f;
        route_material->attribs.cull_mode = 2;
        route_material->attribs.z_mode = 0;
        route_material->attribs.alpha_mode = 2;
        NuMtlUpdate(route_material);

        NUMTL *overlay_material = NuMtlCreate(1);
        aieditorsettings.overlay_material = overlay_material;
        overlay_material->diffuse_color.r = 0.5f;
        overlay_material->diffuse_color.g = 0.5f;
        overlay_material->diffuse_color.b = 0.5f;
        overlay_material->attribs.alpha_mode = 0;
        overlay_material->attribs.cull_mode = 2;
        overlay_material->attribs.z_mode = 3;
        overlay_material->opacity = 1.0f;
        NuMtlUpdate(overlay_material);

        aieditorsettings.current_path_type = -1;
        aieditorsettings.current_script_flags = 0;
        aieditorsettings.draw_all_paths = 0;
        aieditorsettings.unknown_060_bit0 = 0;
        aieditorsettings.show_creatures_display = 1;
        aieditorsettings.snap_height_display = 1;
        aieditorsettings.solid_path_display = 0;
        aieditorsettings.unknown_060_bit5 = 0;
        aieditorsettings.solid_antinode_display = 1;
        aieditorsettings.draw_wallsplines = 1;
        aieditorsettings.show_creatures_set = 0;
        aieditorsettings.current_script_params[0] = 0.0f;
        aieditorsettings.current_script_params[1] = 0.0f;
        aieditorsettings.current_script_params[2] = 0.0f;
        aieditorsettings.current_script_params[3] = 0.0f;
        aieditorsettings.mode_count = 0;
        memset(aieditorsettings.modes, 0, sizeof(aieditorsettings.modes));
        aieditorsettings.path_height_offset = 0.01f;

        AIEDITOR_PATHS =
            aieditor_Register("AIEDITOR_PATHS", pathEditor_Enter, reinterpret_cast<void (*)()>(pathEditor_Process),
                              reinterpret_cast<void (*)()>(pathEditor_Render), nullptr);
        AIEDITOR_ROUTES =
            aieditor_Register("AIEDITOR_ROUTES", nullptr, reinterpret_cast<void (*)()>(routeEditor_Process),
                              reinterpret_cast<void (*)()>(routeEditor_Render), nullptr);
        AIEDITOR_AREAS =
            aieditor_Register("AIEDITOR_AREAS", areaEditor_Enter, reinterpret_cast<void (*)()>(areaEditor_Process),
                              reinterpret_cast<void (*)()>(areaEditor_Render), nullptr);
        AIEDITOR_LOCATORS = aieditor_Register("AIEDITOR_LOCATORS", locatorEditor_Enter,
                                              reinterpret_cast<void (*)()>(locatorEditor_Process),
                                              reinterpret_cast<void (*)()>(locatorEditor_Render), nullptr);
        AIEDITOR_CREATURES = aieditor_Register("AIEDITOR_CREATURES", creatureEditor_Enter,
                                               reinterpret_cast<void (*)()>(creatureEditor_Process),
                                               reinterpret_cast<void (*)()>(creatureEditor_Render), nullptr);
        AIEDITOR_ANTINODES = aieditor_Register("AIEDITOR_ANTINODES", antinodeEditor_Enter,
                                               reinterpret_cast<void (*)()>(antinodeEditor_Process),
                                               reinterpret_cast<void (*)()>(antinodeEditor_Render), nullptr);
        aieditor_RegisterDefaultPathCnxTypes();
        aieditorsettings.external_display_a = *external_display_a;
        aieditorsettings.external_display_b = *external_display_b;
    }

    void aieditor_Leave(void) {
        edmainGetCamera()->near_clip = 0.15f;
        for (i32 index = 0; index < aieditorsettings.mode_count; ++index) {
            void (*leave)() = aieditorsettings.modes[index].leave;
            if (leave != nullptr) {
                leave();
            }
        }
        AIEDITOR_RENDER_STATE *state = aieditor;
        if (state->mode_selection_36930 != nullptr) {
            strcpy(aieditorsettings.current_area_name, state->mode_selection_36930->name);
        }
        if (state->current_path != nullptr) {
            strcpy(aieditorsettings.current_path_name, state->current_path->name);
        }
        if (state->current_route != nullptr) {
            strcpy(aieditorsettings.current_route_name, state->current_route->name);
        }
        AISYSRebuildFromEditorData();
        AIScriptInitConditions(aieditor->ai_system);
        aieditor->flags &= ~u8(2);
    }

    void aieditor_Proc(void) {
    }

    i32 aieditor_Register(const char *name, void (*enter)(), void (*callback_24)(), void (*callback_28)(),
                          void (*leave)()) {
        if (aieditorsettings.mode_count > 7) {
            return -1;
        }
        i32 index = aieditorsettings.mode_count;
        ++aieditorsettings.mode_count;
        aieditor_settings_s::EditorMode &mode = aieditorsettings.modes[index];
        NuStrCpy(mode.name, name);
        mode.enter = enter;
        mode.callback_24 = callback_24;
        mode.callback_28 = callback_28;
        mode.leave = leave;
        return index;
    }

    void aieditor_RegisterDefaultPathCnxTypes(void) {
        aieditor_RegisterPathCnxType("Permanent Block", 0x40000000, nullptr, 0);
        aieditor_RegisterPathCnxType("Temporary Block", 0x80000000, nullptr, 0);
        aieditor_RegisterPathCnxType("Link Obstacle", 0x20000000, nullptr, 1);
    }

    void aieditor_RegisterPathCnxType(const char *name, u32 connection_flag, void *context, u32 flags) {
        if (name == nullptr) {
            return;
        }
        usize length = strlen(name);
        if (length > 63 || connection_flag == 0 || naipathcnxtypes >= 32) {
            return;
        }
        AIPATHCNXTYPE_s &type = aipathcnxtypes[naipathcnxtypes++];
        memcpy(type.name, name, length + 1);
        type.connection_flag = connection_flag;
        type.context = context;
        type.flags = flags;
    }

    void aieditor_Render(void) {
    }

    void aieditor_Reset(void) {
        aieditorsettings.current_path_type = -1;
        aieditorsettings.current_area_name[0] = 0;
        aieditorsettings.current_path_name[0] = 0;
        aieditorsettings.current_route_name[0] = 0;
        aieditor_SetCurrentScript((char *)"default", 0);
    }

    void aieditor_Save(void) {
    }

    void aieditor_SetCurrentScript(char *name, const AIEditorScriptSelection *selection) {
        if (NuStrICmp(name, aieditorsettings.current_script_name) != 0) {
            strcpy(aieditorsettings.current_script_name, name);
        }
        if (selection != nullptr) {
            aieditorsettings.current_script_flags = selection->flags & 0x1e;
            memcpy(aieditorsettings.current_script_params, selection->script_params,
                   sizeof(aieditorsettings.current_script_params));
            return;
        }
        AISCRIPT *script = AIScriptFind(aieditor->ai_system, aieditorsettings.current_script_name, 1, 1, 1);
        if (script != nullptr) {
            for (i32 i = 0; i < 4; ++i) {
                aieditorsettings.current_script_params[i] = script->params[i].default_val;
            }
        } else {
            memset(aieditorsettings.current_script_params, 0, sizeof(aieditorsettings.current_script_params));
        }
        aieditorsettings.current_script_flags = 0;
    }

    void aieditor_SetMode(i32 mode) {
        u16 previous_mode = aieditorsettings.current_mode;
        aieditorsettings.current_mode = mode;
        AIEDITOR_RENDER_STATE *state = aieditor;
        if (state != nullptr && previous_mode != (u16)mode) {
            if (state->current_path != nullptr) {
                state->current_path->current_node = nullptr;
            }
            state->mode_selection_36930 = nullptr;
            state->mode_selection_37a48 = nullptr;
            state->mode_selection_3c260 = nullptr;
            state->mode_selection_42e9c = nullptr;
        }
    }

    void aieditor_cbCancelMainMenu(eduimenu_s *, eduimenu_s *) {
        aieditor_ClearMainMenu();
    }

    void aieditor_cbDrawAllToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.draw_all_paths = item->highlighted;
    }

    void aieditor_cbGoToPlayer(void) {
        AISYS_s *system = aieditor->ai_system;
        if (system != nullptr && system->player_1 != nullptr) {
            edcamSetPos(&system->player_1->position);
        }
        aieditor_ClearMainMenu();
    }

    void aieditor_cbMovePlayer(void) {
        if (AIEditorMovePlayersFn != nullptr) {
            AIEditorMovePlayersFn(&aieditor->camera_position);
        }
        aieditor_ClearMainMenu();
    }

    void aieditor_cbSave(void) {
    }

    void aieditor_cbShowCreaturesSetToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.show_creatures_set = item->highlighted;
    }

    void aieditor_cbShowCreaturesToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.show_creatures_display = item->highlighted;
    }

    void aieditor_cbSnapHeightToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.snap_height_display = item->highlighted;
    }

    void aieditor_cbSolidAntinodeDisplayToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.solid_antinode_display = item->highlighted;
    }

    void aieditor_cbSolidPathDisplayToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.solid_path_display = item->highlighted;
    }

    void aieditor_cbStopPlatformsToggle(eduimenu_s *, eduiitem_s *item, u32) {
        aieditorsettings.stop_platforms = item->highlighted;
    }

    void aieditor_cvSelectEditorMode(eduimenu_s *parent) {
        eduimenu_s *menu = eduiMenuCreate(0xdc, 0x46, 0xf0, 0xfa, ed_fnt, aieditor_cbCancelSelectEditorMode,
                                          (char *)"Select Editor Mode");
        if (menu == nullptr) {
            return;
        }
        for (i32 index = 0; index < aieditorsettings.mode_count; ++index) {
            if ((i16)aieditorsettings.current_mode == index) {
                eduiitem_s *item = eduiItemCheckCreate(index, &attr, 1, 1, aieditor_cbSetEditorMode,
                                                       aieditorsettings.modes[index].name);
                eduiMenuAddItem(menu, item);
                menu->selected = edui_last_item;
            } else {
                eduiitem_s *item = eduiItemCheckCreate(index, &attr, 0, 1, aieditor_cbSetEditorMode,
                                                       aieditorsettings.modes[index].name);
                eduiMenuAddItem(menu, item);
            }
            eduiMenuAttach(parent, menu);
        }
    }

} // extern "C"
