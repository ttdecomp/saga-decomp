#pragma once
#include "decomp.h"
#include "nu2api/numath/nuvec.h"
// A command stores one duration, integer, position, or object/spline pointer.
struct MINICAMCOMMAND_s {
    union {
        i32 state_words[7];
        struct {
            i32 type;
            f32 duration;
            i32 argument;
            NUVEC position;
            NUVEC *target;
        };
    };
};
DECOMP_ASSERT(sizeof(MINICAMCOMMAND_s) == 0x1c, "MINICAMCOMMAND_s size");
struct MINICAM_s {
    MINICAMCOMMAND_s commands[32];
    u8 command_count;
    u8 current_command;
    union {
        u8 flags[2];
        struct {
            u8 mode;
            u8 easing;
        };
    };
    union {
        i32 command_state[27];
        struct {
            u32 reserved_384;
            f32 distance;
            u16 pitch, yaw, roll, reserved_392;
            f32 target_distance;
            u16 target_pitch, target_yaw, target_roll, reserved_39e;
            NUVEC focus_velocity;
            NUVEC position_velocity;
            f32 start_distance;
            u16 start_pitch, start_yaw, start_roll, reserved_3c2;
            NUVEC *focus;
            NUVEC focus_offset;
            NUVEC target;
            NUVEC position;
            union {
                NUVEC *position_source;
                struct nugspline_s *position_spline;
            };
        };
    };
    f32 delta_time;
    union {
        i32 field_0x3f4;
        f32 duration;
    };
    union {
        i32 field_0x3f8;
        f32 elapsed_time;
    };
};
DECOMP_ASSERT(sizeof(MINICAM_s) == 0x3fc, "MINICAM_s size");
DECOMP_ASSERT(offsetof(MINICAM_s, command_count) == 0x380, "MINICAM command count offset");
DECOMP_ASSERT(offsetof(MINICAM_s, current_command) == 0x381, "MINICAM command index offset");
DECOMP_ASSERT(offsetof(MINICAM_s, delta_time) == 0x3f0, "MINICAM delta time offset");
DECOMP_ASSERT(offsetof(MINICAM_s, focus) == 0x3c4, "MINICAM focus offset");
DECOMP_ASSERT(offsetof(MINICAM_s, position) == 0x3e0, "MINICAM position offset");
extern MINICAM_s MiniCam;
void Minicam_Update();
void Minicam_AddDeltas(f32 duration);
void Minicam_CalcCamPos();
void Minicam_ClearDeltas();
void Minicam_ResetForNewCut();
void Minicam_ResetForNextCommand();
void MiniCam_ChangeMode(i32 mode);
void Minicam_InitSystem();
void Minicam_AddCommand(i32 type, f32 duration, i32 argument, void *target, nuvec_s position);
