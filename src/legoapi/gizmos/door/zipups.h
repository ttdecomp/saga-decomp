#pragma once

#include "decomp.h"
#include "legoapi/characters/motion/animation_ids.h"
#include "legoapi/gizmo/base/gizmo.h"

extern i32 zipup_gizmotype_id;

#ifdef __cplusplus

struct GameObject_s;
struct numtl_s;

enum ZIPUP_FLAGS {
    ZIPUP_FLAG_CONFIG_0 = 1 << 0,
    ZIPUP_FLAG_CONFIG_1 = 1 << 1,
    ZIPUP_FLAG_CONFIG_2 = 1 << 2,
    ZIPUP_FLAG_CONFIG_3 = 1 << 3,
    ZIPUP_FLAG_CONFIG_4 = 1 << 4,
    ZIPUP_FLAG_CONFIG_5 = 1 << 5,
    ZIPUP_FLAG_ACTIVE = 1 << 6,
    ZIPUP_FLAG_VISIBLE = 1 << 7,
};

enum ZIPUP_RUNTIME_FLAGS {
    ZIPUP_RUNTIME_FLAG_OCCUPIED = 1 << 0,
};

typedef struct ZIPUP_s {
    char name[16];               // 0x00
    NUVEC lower_position;        // 0x10
    NUVEC hook_origin;           // 0x1c
    NUVEC upper_position;        // 0x28
    u16 hook_x_rotation;         // 0x34
    u16 hook_y_rotation;         // 0x36
    NUVEC hook_position;         // 0x38
    NUVEC rider_start_offset;    // 0x44, selected endpoint relative to hook, adjusted for rider height
    NUVEC rider_target_position; // 0x50
    GameObject_s *occupant;      // 0x5c
    u16 pitch_adjustment;        // 0x60, scaled by the rider transition time
    u16 direction;               // 0x62
    u16 facing_angle;            // 0x64
    u8 reserved_0x66[2];         // 0x66 .. 0x68
    union {
        u8 flags;
        struct {
            u8 configuration_flags : 6;
            u8 active : 1;
            u8 visible : 1;
        };
    };
    u8 runtime_flags;            // 0x69
    u8 reserved_0x6a[2];         // 0x6a .. 0x6c
    f32 lower_ground_height;     // 0x6c
    f32 upper_ground_height;     // 0x70
    u16 lower_ground_x_rotation; // 0x74
    u16 upper_ground_x_rotation; // 0x76
    u16 lower_ground_z_rotation; // 0x78
    u16 upper_ground_z_rotation; // 0x7a
} ZIPUP;

DECOMP_ASSERT(sizeof(ZIPUP) == 0x7c, "ZIPUP ABI");
DECOMP_ASSERT(offsetof(ZIPUP, rider_start_offset) == 0x44, "ZIPUP rider start offset");
DECOMP_ASSERT(offsetof(ZIPUP, rider_target_position) == 0x50, "ZIPUP rider target offset");
DECOMP_ASSERT(offsetof(ZIPUP, occupant) == 0x5c, "ZIPUP occupant offset");
DECOMP_ASSERT(offsetof(ZIPUP, direction) == 0x62, "ZIPUP direction offset");
DECOMP_ASSERT(offsetof(ZIPUP, pitch_adjustment) == 0x60, "ZIPUP pitch adjustment offset");
DECOMP_ASSERT(offsetof(ZIPUP, flags) == 0x68, "ZIPUP flags offset");
DECOMP_ASSERT(offsetof(ZIPUP, lower_ground_height) == 0x6c, "ZIPUP lower ground height offset");

ADDGIZMOTYPE *ZipUps_RegisterGizmo(i32 type_id);
ZIPUP *ZipUp_FindNearest(WORLDINFO_s *world, NUVEC *position, f32 radius, f32 *distance, i32 *endpoint,
                         GameObject_s *object, bool touch);
void InitRopeMtl(char *name, VARIPTR *buffer, VARIPTR *buffer_end);
void DrawRopeSingle(NUVEC *start, NUVEC *end, f32 amount, numtl_s *material, f32 time, f32 grow_time,
                    f32 spacing, f32 scale);
extern numtl_s *ropemtl;

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
