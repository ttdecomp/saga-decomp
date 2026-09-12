#pragma once

#include "decomp.h"

struct AIEditorScriptSelection {
    u8 unknown_000[0x68];
    u32 flags;
    u8 unknown_06c[4];
    f32 script_params[4];
};
struct numtl_s;

struct aieditor_settings_s {
    char current_path_name[0x10];
    char current_area_name[0x10];
    char current_route_name[0x10];
    char current_script_name[0x44 - 0x30];
    i32 current_path_type;
    f32 path_height_offset;
    f32 current_script_params[4];
    u32 current_script_flags;
    union {
        u8 path_display_flags;
        u8 path_flags;
        struct {
            u8 unknown_060_bit0 : 1;
            u8 draw_all_paths : 1;
            u8 unknown_060_bit2 : 1;
            u8 show_creatures_display : 1;
            u8 snap_height_display : 1;
            u8 unknown_060_bit5 : 1;
            u8 stop_platforms : 1;
            u8 solid_path_display : 1;
        };
    };
    union {
        u8 path_draw_flags;
        u8 path_flags2;
        struct {
            u8 solid_antinode_display : 1;
            u8 draw_wallsplines : 1;
            u8 show_creatures_set : 1;
            u8 unknown_061_bits3_7 : 5;
        };
    };
    u8 unknown_062[0x68 - 0x62];
    numtl_s *path_material;
    numtl_s *route_material;
    numtl_s *overlay_material;
    void *external_display_a;
    void *external_display_b;
    struct EditorMode {
        char name[0x20];
        void (*enter)();
        void (*callback_24)();
        void (*callback_28)();
        void (*leave)();
    } modes[8];
    i16 mode_count;
    u16 current_mode;
};

DECOMP_ASSERT(sizeof(aieditor_settings_s) == 0x200, "aieditor settings size");
DECOMP_ASSERT(offsetof(AIEditorScriptSelection, flags) == 0x68, "editor script selection flags offset");
DECOMP_ASSERT(offsetof(AIEditorScriptSelection, script_params) == 0x70, "editor script selection params offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, current_script_name) == 0x30, "editor current script name offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, current_script_params) == 0x4c, "editor current script params offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, path_height_offset) == 0x48, "editor path height offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, current_script_flags) == 0x5c, "editor current script flags offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, path_material) == 0x68, "editor path material offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, route_material) == 0x6c, "editor route material offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, overlay_material) == 0x70, "editor overlay material offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, external_display_a) == 0x74, "editor display callback offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, external_display_b) == 0x78, "editor display callback offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, mode_count) == 0x1fc, "editor mode count offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, current_mode) == 0x1fe, "editor current mode offset");
DECOMP_ASSERT(offsetof(aieditor_settings_s, modes) == 0x7c, "editor modes offset");
DECOMP_ASSERT(sizeof(aieditor_settings_s::EditorMode) == 0x30, "editor mode size");

extern aieditor_settings_s aieditorsettings;
