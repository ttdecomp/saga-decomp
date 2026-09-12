#ifndef LEGOAPI_SOCKSYS_H
#define LEGOAPI_SOCKSYS_H

#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/numtx.h"
#include "decomp.h"

typedef struct SOCKROT {
    u16 x;
    u16 y;
} SOCKROT;

enum SOCK_FLAGS : u16 {
    SOCK_FLAG_MISSING_C_OR_D = 0x0001,
    SOCK_FLAG_CLAMP_TARGET_Y = 0x0020,
    SOCK_FLAG_PROJECT_CAMERA_FROM_PLAYER = 0x0080,
    SOCK_FLAG_SCALE_LATERAL_OFFSET = 0x0200,
    // Two-player pullback follows the camera/target line in XZ only.
    SOCK_FLAG_TWO_PLAYER_PLANAR_PULLBACK = 0x0400,
    SOCK_FLAG_CAMERA_DISTANCE_XZ = 0x2000,
    // Two-player pullback is driven by vertical rather than spatial separation.
    SOCK_FLAG_TWO_PLAYER_VERTICAL_SEPARATION = 0x4000,
};

typedef struct SOCKLOCATION_s {
    u8 unknown_00;
    i8 sock;
    i16 segment;
} SOCKLOCATION;

typedef struct SOCKSEGMENT_s {
    NUVEC min;               // 0x00
    NUVEC max;               // 0x0c
    NUVEC planes[6];         // 0x18
    NUVEC midpoint;          // 0x60
    NUVEC next_midpoint;     // 0x6c
    f32 length;              // 0x78
    f32 distance_from_start; // 0x7c
} SOCKSEGMENT;

// A resolved position within the socket system. The original passes this
// record between the player, camera and level-streaming systems.
typedef struct SOCKPOSITION_s {
    SOCKLOCATION location;     // 0x00
    f32 ratio;                 // 0x04
    NUVEC midpoint;            // 0x08
    NUVEC camera_position;     // 0x14
    SOCKROT camera_rotation;   // 0x20
    SOCKROT midpoint_rotation; // 0x24
    union {
        struct {
            i16 next_segment;        // 0x28
            i8 candidate_count;      // 0x2a
            u8 flags;                // 0x2b
            f32 distance;            // 0x2c
            f32 normalized_distance; // 0x30
            u32 candidate_mask;      // 0x34
        };
        // StartDoorPositions deliberately reuses the tail of OldPlrSPos[7]
        // as its fallback position/angle record in the original binary.
        struct {
            NUVEC *position;
            u8 door_fallback_padding[8];
            i16 angle;
            u8 door_fallback_tail[2];
        } door_fallback;
    };
} SOCKPOSITION;

DECOMP_ASSERT(sizeof(SOCKROT) == 0x4, "SOCKROT size");
DECOMP_ASSERT(sizeof(SOCKLOCATION) == 0x4, "SOCKLOCATION size");
DECOMP_ASSERT(sizeof(SOCKSEGMENT) == 0x80, "SOCKSEGMENT size");
DECOMP_ASSERT(sizeof(SOCKPOSITION) == 0x38, "SOCKPOSITION size");

// SockSys — spline-based rail camera system used during world loading. A
// SOCKSYS is carved out of the giz buffer: an 8-byte header followed by a
// fixed array of 64 SOCK entries (0x4f00 bytes total).
//
// Each SOCK describes one camera "socket": rails in the scene are named
// `sock_cam_00`..`sock_cam_3f` (plus `sock_a_`, `sock_b_`, `sock_c_`,
// `sock_d_`, `sock_mid_`, `sock_left_`, `sock_right_`, `sock_look_`,
// `sock_lateral_`, `sock_trackin_`, `sock_limit_`) and are resolved against
// the scene splines by SockSysFindInScene.
typedef struct SOCK {
    NUGSPLINE *cam;     // 0x00 — sock_cam_ rail; NULL until found in scene
    NUGSPLINE *a;       // 0x04 — sock_a_ rail spline
    NUGSPLINE *b;       // 0x08 — sock_b_ rail spline
    NUGSPLINE *c;       // 0x0c — sock_c_ rail spline (optional)
    NUGSPLINE *d;       // 0x10 — sock_d_ rail spline (optional)
    NUGSPLINE *mid;     // 0x14 — sock_mid_ centre spline (optional)
    NUGSPLINE *left;    // 0x18 — sock_left_ rail spline (optional)
    NUGSPLINE *right;   // 0x1c — sock_right_ rail spline (optional)
    NUGSPLINE *look;    // 0x20 — sock_look_ spline (optional)
    NUGSPLINE *lateral; // 0x24 — sock_lateral_ spline (optional)
    NUGSPLINE *trackin; // 0x28 — sock_trackin_ spline (optional)
    NUGSPLINE *limit;   // 0x2c — sock_limit_ spline (optional)
    u16 length;         // 0x30 — rail point count - 1
    u8 valid;           // 0x32 — 1 once the socket has been populated
    union {
        u8 unknown_33;
        u8 looping;
    }; // 0x33
    SOCKSEGMENT *segments;  // 0x34 — generated data for each rail segment
    SOCKROT *cam_rotations; // 0x38 — generated camera-rail rotations
    SOCKROT *mid_rotations; // 0x3c — generated midpoint-rail rotations
    NUVEC min;              // 0x40 — min of the A/B(/C/D) rail points
    NUVEC max;              // 0x4c — max of the A/B(/C/D) rail points
    NUVEC center;           // 0x58 — midpoint of min and max
    f32 extent;             // 0x64 — half of the smaller of the x/z extents
    u16 flags;              // 0x68 — SOCK_FLAGS
    u8 unknown_6a;          // 0x6a
    u8 unknown_6b;          // 0x6b
    u8 unknown_6c;          // 0x6c
    u8 look_ahead_segments; // 0x6d
    u16 input_yaw;          // 0x6e — controller angle offset on camera-relative sockets
    union {
        struct {
            u8 unknown_70, unknown_71, unknown_72, unknown_73;
        };
        f32 current_speed; // 0x70
    };
    union {
        struct {
            u8 unknown_74;
            u8 unknown_75;
            u8 unknown_76;
            u8 unknown_77;
        };
        f32 mid_force_inner_radius; // 0x74
    };
    union {
        struct {
            u8 unknown_78;
            u8 unknown_79;
            u8 unknown_7a;
            u8 unknown_7b;
        };
        f32 mid_force_outer_radius; // 0x78
    };
    f32 unknown_7c;                 // 0x7c — default 1.0
    f32 unknown_80;                 // 0x80 — default 1.0
    f32 unknown_84;                 // 0x84 — default 1.0
    f32 look_ratio_xz;              // 0x88 — default 1.0
    f32 look_ratio_y;               // 0x8c — default 1.0
    f32 camera_position_seek;       // 0x90 — default 5.0
    f32 camera_angle_seek;          // 0x94 — default 5.0
    f32 unknown_98;                 // 0x98
    f32 camera_local_x_ratio;       // 0x9c
    f32 camera_vertical_ratio;      // 0xa0
    f32 camera_lateral_ratio;       // 0xa4
    NUVEC camera_arena_offset;      // 0xa8
    f32 camera_rail_offset;         // 0xb4
    NUVEC camera_arena_blend;       // 0xb8
    f32 camera_distance_to_target;  // 0xc4
    f32 camera_pullback_ratio;      // 0xc8
    f32 single_player_pullback;     // 0xcc
    f32 two_player_pullback;        // 0xd0
    f32 camera_height_above_ground; // 0xd4
    f32 camera_shake;               // 0xd8
    u32 overlap_exclusion_mask[2];  // 0xdc — sockets whose bounds do not overlap
    char name[16];                  // 0xe4 — zero-padded socket index, e.g. "03"
    f32 overlap_blend_ratio;        // 0xf4 — default 0.5
    u32 unknown_f8;                 // 0xf8 — array of scene specials shown on the socket
    u16 unknown_fc;                 // 0xfc — count of specials at 0xf8
    u8 unknown_fe;                  // 0xfe
    u8 unknown_ff;                  // 0xff
    struct {
        u8 value;
        i8 edge;
    } blend_entries[8]; // 0x100
    union {
        u32 unknown_110;
        u32 blend_count;
    }; // 0x110
    u8 unknown_114[40]; // 0x114
} SOCK;

DECOMP_ASSERT(offsetof(SOCK, looping) == 0x33, "SOCK loop flag offset");
DECOMP_ASSERT(offsetof(SOCK, length) == 0x30, "SOCK rail length offset");
DECOMP_ASSERT(offsetof(SOCKPOSITION, midpoint_rotation) == 0x24, "SOCKPOSITION midpoint rotation offset");

typedef struct SOCKSYS {
    SOCK *sock; // 0x0 — array of 64 SOCK entries
    i32 count;  // 0x4 — number of sockets with a valid rail
} SOCKSYS;
DECOMP_ASSERT(sizeof(SOCK) == 0x13c, "SOCK movement ABI");
DECOMP_ASSERT(offsetof(SOCK, mid_force_inner_radius) == 0x74, "SOCK inner force radius offset");
DECOMP_ASSERT(offsetof(SOCK, mid_force_outer_radius) == 0x78, "SOCK outer force radius offset");

#ifdef __cplusplus
extern "C" {
    SOCK *FindSock(SOCKSYS *system, char *name);
    void SockOff(SOCKSYS *system, i32 index);
    void SockOn(SOCKSYS *system, i32 index);
    void SetSockPostion(SOCKSYS *system, SOCKPOSITION *position, i32 index, i32 segment, f32 ratio);
    void MoveSockPosition(SOCKSYS *system, SOCKPOSITION *source, f32 distance, SOCKPOSITION *result);
    f32 MidDistanceFromSockStart(SOCKSYS *system, SOCKPOSITION *position);
#endif

    void SockSysFindInScene(SOCKSYS *sock_sys, NUGSCN *gscn);
    SOCKSYS *SockSysInit(VARIPTR *buf, VARIPTR buf_end, NUGSCN *gscn);
    void SockSys_GenerateData(SOCKSYS *sock_sys, VARIPTR *buf, VARIPTR buf_end);
    void SockSysPointAlongSpline(NUVEC *result, NUGSPLINE *spline, i32 segment, i32 next_segment, f32 ratio);
    void SockRotationMatrix(SOCKSYS *system, SOCKPOSITION *position, NUMTX *out, i32 stride, i32 mode);
    void SetSockBit(SOCK *sock, i32 index);
    void ComplexSockPosition(SOCKSYS *sock_sys, NUVEC *position, i32 prior_sock, i32 prior_segment,
                             SOCKPOSITION *result);
    void ComplexSockAngles(SOCKROT *angles);
    i32 SockSysCameraWithOverlapBlend(SOCKSYS *sock_sys, NUVEC *fallback_camera_position, i32 socket_changed,
                                      NUVEC *player_camera_positions, NUVEC *player_positions, i32 player_count,
                                      SOCKPOSITION *camera_socket_position, NUVEC *camera_position,
                                      NUVEC *camera_target, f32 *overlap_blend, f32 *position_seek, f32 *angle_seek,
                                      f32 *camera_shake, f32 *separation_scale);
    i32 SockSysCamera(SOCKSYS *sock_sys, NUVEC *fallback_camera_position, i32 socket_changed,
                      NUVEC *player_camera_positions, NUVEC *player_positions, i32 player_count,
                      SOCKPOSITION *camera_socket_position, NUVEC *camera_position, NUVEC *camera_target,
                      f32 *overlap_blend, f32 *position_seek, f32 *angle_seek, f32 *camera_shake,
                      f32 *separation_scale);

#ifdef __cplusplus
}
#endif

#endif // LEGOAPI_SOCKSYS_H
