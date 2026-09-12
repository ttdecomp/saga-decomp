#pragma once

#include "nu2api/nucore/common.h"

struct eduimenu_s;
struct nupad_s;
struct edui_interact_s;
typedef void (*EdUiMenuCallback)(eduimenu_s *menu, eduimenu_s *parent);

enum EdUiItemFlags {
    EDUI_ITEM_HIGHLIGHTED = 0x01,
    EDUI_ITEM_DISABLED = 0x02,
};

enum EdUiItemType {
    EDUI_ITEM_TOGGLE = 3,
    EDUI_ITEM_EXPANDER = 19,
};

enum EdUiExpanderFlags {
    EDUI_EXPANDER_OPEN = 1,
};

enum EdUiAnalogPad {
    EDUI_ANALOG_PAD_NONE = 0,
    EDUI_ANALOG_PAD_LEFT = 1,
    EDUI_ANALOG_PAD_RIGHT = 2,
};

enum EdUiCursorButtons {
    EDUI_CURSOR_SECONDARY = 0x10,
    EDUI_CURSOR_PRIMARY = 0x40,
};

// Fields not yet traced retain their original byte ranges.
struct eduiitem_s {
    eduiitem_s *next;
    eduiitem_s *previous;
    i32 type;
    union {
        i32 data;
        void *data_ptr;
        u8 unknown_0c[4];
    };
    u8 unknown_10;
    union {
        u8 flags;
        struct {
            u8 highlighted : 1;
            u8 disabled : 1;
            u8 unknown_flags : 6;
        };
    };
    u8 unknown_12[6];
    i32 selection_group;
    u8 unknown_1c[8];
    char *text;
    u8 unknown_28[0x14];
    i32 (*process)(eduimenu_s *, eduiitem_s *, f32, nupad_s *);
    void *field_40;
    void (*destroy)(eduimenu_s *, eduiitem_s *);
};

struct eduimenu_s {
    eduiitem_s *first;
    eduiitem_s *last;
    eduiitem_s *selected;
    eduiitem_s *field_0c;
    eduiitem_s *field_10;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
    i32 field_24;
    i32 field_28;
    void *font;
    EdUiMenuCallback callback;
    char *title;
    u32 flags : 3;
    u32 unknown_flags : 29;
    eduimenu_s *child;
    eduimenu_s *parent;
};

struct ed_module_s {
    ed_module_s *next;
    ed_module_s *previous;
    u8 unknown_08[0x08];
    void (*close)();
    void (*activate)();
    void (*deactivate)();
};

struct edui_slider_s : eduiitem_s {
    u32 field_48;
    void (*changed)(eduimenu_s *, eduiitem_s *, u32);
    f32 normalized_value;
    f32 value;
    f32 minimum;
    f32 range;
    i32 change_timer;
    char *format;
    f32 granularity;
};

struct edui_prop_s : eduiitem_s {
    u8 unknown_48[0x1c];
    char *property_text;
    u8 unknown_68[0x18];
};

struct edui_interact_s {
    f32 x, y, width, height;
    eduimenu_s *menu;
    eduiitem_s *item;
    i32 (*callback)(edui_interact_s *);
    u32 buttons;
    i32 field_20;
};

extern "C" {
    i32 edmainActivate(ed_module_s *module, i32 notify);
    ed_module_s *edmainCurrent(void);
    i32 edmainRegister(ed_module_s *module);
    void edmainClose(void);
    eduimenu_s *edGetMainMenu(void);
    void eduiGetCursorCoords(f32 *x, f32 *y);
    void eduiSetCursorCoords(f32 x, f32 y);
    void eduiGetCursorDelta(f32 *x, f32 *y);
    void eduiProcessCursor(f32 delta_time, nupad_s *pad);
    void eduiProcessCursorDefault(f32 delta_time, nupad_s *pad);
    i32 eduiUsedAlgPad(nupad_s *pad);
    extern i32 bUsingMenuFocus;
    i32 eduiGetUsingMenuFocus(void);
    void eduiSetUsingMenuFocus(i32 enabled);
    eduimenu_s *eduiGetActiveMenu(void);
    eduimenu_s *eduiGetActiveMenuParent(void);
    eduimenu_s *eduiGetTopLevelParent(eduimenu_s *menu);
    void eduiSetActiveMenu(eduimenu_s *menu);
    void eduiSetDefaultActiveMenu(eduimenu_s *menu);
    i32 eduiClearActiveMenu(void);
    i32 eduiCheckForPadMenuCancel(eduimenu_s *menu, nupad_s *pad);
    i32 eduiMenuAttach(eduimenu_s *menu, eduimenu_s *child);
    i32 eduiMenuDetach(eduimenu_s *menu);
    i32 eduiMenuIsActive(eduimenu_s *menu);
    void eduiMenuEnsureSelection(eduimenu_s *menu);
    void eduiMenuHighlight(eduimenu_s *menu, eduiitem_s *item);
    i32 eduiItemSetText(eduiitem_s *item, char *text);
    i32 eduiItemPropSetText(edui_prop_s *item, char *text);
    void eduiItemSliderSetVal(edui_slider_s *item, f32 value);
    void eduiItemSliderSetValEx(edui_slider_s *item, f32 value, i32 reset_timer, i32 notify);
    void eduiItemSliderSetGranularity(edui_slider_s *item, f32 granularity);
    void eduiItemSliderSetFmt(edui_slider_s *item, char *format);
    void eduiMenuDestroy(eduimenu_s *menu);
    void eduiMenuDestroyItems(eduimenu_s *menu);
    void eduicbMenuCloseAllexpanders(eduimenu_s *menu);
    void eduicbMenuOpenAllexpanders(eduimenu_s *menu);
    void eduiMenuSelectFirstEntry(eduimenu_s *menu);
    i32 eduiMenuProcessSelectedItem(eduimenu_s *menu, f32 delta_time, nupad_s *pad);
    i32 eduiMenuProcess(eduimenu_s *menu, f32 delta_time, nupad_s *pad);
    i32 eduiMenuProcessAux(eduimenu_s *menu, f32 delta_time, nupad_s *pad);
    void eduiMenuProcessInput(eduimenu_s *menu, f32 delta_time, nupad_s *pad, i32 item_result);
    i32 eduiProcessInteracts(eduimenu_s *menu, nupad_s *pad);
    void eduiFlushInteracts(void);
    i32 eduiCursorOverMenu(eduimenu_s *menu);
    void cbInteractMenuScrollTo(eduimenu_s *menu, char *text);
    void cbInteractMenuKeySelect(eduimenu_s *menu);
    eduimenu_s *eduiMenuCreate(i32 x, i32 y, i32 width, i32 height, void *font, EdUiMenuCallback callback, char *title);
    extern eduiitem_s *edui_last_item;
    eduiitem_s *eduiMenuAddItem(eduimenu_s *menu, eduiitem_s *item);
    void eduiMenuAddItemFirst(eduimenu_s *menu, eduiitem_s *item);
    void eduiMenuAddItemLast(eduimenu_s *menu, eduiitem_s *item);
    void eduiMenuAddItemAfter(eduimenu_s *menu, eduiitem_s *item, eduiitem_s *after);
    void eduiMenuAddItemBefore(eduimenu_s *menu, eduiitem_s *item, eduiitem_s *before);
    void eduiMenuRemoveItem(eduimenu_s *menu, eduiitem_s *item);
    i32 eduiMenuItemMoveUp(eduimenu_s *menu, eduiitem_s *item);
    i32 eduiMenuItemMoveDown(eduimenu_s *menu, eduiitem_s *item);
}
