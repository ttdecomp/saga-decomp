#include "nu2api/nu3d/nucamera.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuvec4.h"
#include <stdio.h>

i32 NuBridgeAlloc(void);
void ropesegment(numtl_s *, NUVEC *, i32, i32) {
}

void NuBrdigeDrawRope(numtl_s *material, NUVEC *first, NUVEC *second, i32, i32 *boundaries, i32 colour) {
    for (i32 i = 0; boundaries[i] < boundaries[i + 1]; ++i) {
        ropesegment(material, &first[boundaries[i]], boundaries[i + 1] - boundaries[i], colour);
        ropesegment(material, &second[boundaries[i]], boundaries[i + 1] - boundaries[i], colour);
    }
}

extern "C" {
    struct NUBRIDGE {
        u8 active;
        u8 unknown_01;
        i16 idle_frames;
        NUGSCN *scene;
        void *terrain;
        NUVEC edge_positions[24][2];
        NUVEC edge_motion[24][2];
        NUMTX platform_matrices[24];
        nuhspecial_s special;
        nuhspecial_s second_special;
        NUVEC centre;
        f32 cull_radius_squared;
        i16 platforms[24];
        u8 in_range;
        u8 was_drawn;
        i8 platform_count;
        i8 field_ae7;
        u32 colour;
        u16 contact_frames;
        i16 rotation_y;
        f32 width, spring_strength, gravity, damping;
        f32 load_multiplier, field_b04, field_b08;
    };
    DECOMP_ASSERT(sizeof(NUBRIDGE) == 0xb0c, "NUBRIDGE size");
    NUBRIDGE Bridges[8];
    i32 NuBridgeProc;
    void NuBridgeDraw(numtl_s *material) {
        i32 b, i;
        NUBRIDGE *bridge;
        i32 n, group;
        NUVEC4 point;
        NUVEC first[512], second[512];
        NUMTX matrix;
        i32 boundaries[8];
        NUVEC4 offsets[2];
        if (NuBridgeProc) {
            bridge = Bridges;
            n = 0;
            group = 0;
            boundaries[group] = 0;
            for (b = 0; b < 8; ++b, ++bridge) {
                if (bridge->active && bridge->in_range) {
                    bridge->was_drawn = 0;
                    NuMtxSetRotationY(&matrix, bridge->rotation_y);
                    offsets[0].x = 0.0f;
                    offsets[0].y = 0.0f;
                    offsets[0].z = -bridge->field_b04;
                    offsets[0].w = 1.0f;
                    offsets[1].x = 0.0f;
                    offsets[1].y = 0.0f;
                    offsets[1].z = bridge->field_b04;
                    offsets[1].w = 1.0f;
                    for (i = 0; i < bridge->platform_count; ++i) {
                        if (NuSpecialExistsFn(&bridge->special)) {
                            if (NuSpecialDrawAt(&bridge->special, &bridge->platform_matrices[i]))
                                bridge->was_drawn = 1;
                        }
                        if (i % bridge->field_ae7 == 0 && NuSpecialExistsFn(&bridge->second_special)) {
                            NuVec4MtxTransformVU0(&point, &offsets[0], &bridge->platform_matrices[i]);
                            if (n < 512) {
                                first[n].x = point.x;
                                first[n].y = point.y + bridge->field_b08;
                                first[n].z = point.z;
                            }
                            matrix.m30 = point.x;
                            matrix.m31 = point.y;
                            matrix.m32 = point.z;
                            NuSpecialDrawAt(&bridge->second_special, &matrix);
                            point.x = 0.0f;
                            point.y = 0.0f;
                            point.z = 1.0f;
                            point.w = 1.0f;
                            NuVec4MtxTransformVU0(&point, &offsets[1], &bridge->platform_matrices[i]);
                            if (n < 512) {
                                second[n].x = point.x;
                                second[n].y = point.y + bridge->field_b08;
                                second[n].z = point.z;
                                ++n;
                            }
                            matrix.m30 = point.x;
                            matrix.m31 = point.y;
                            matrix.m32 = point.z;
                            NuSpecialDrawAt(&bridge->second_special, &matrix);
                        }
                    }
                    if (boundaries[group] != n) {
                        ++group;
                        boundaries[group] = n;
                    }
                }
            }
            boundaries[group + 1] = n;
            if (n > 1)
                NuBrdigeDrawRope(material, first, second, n, boundaries, Bridges[0].colour);
        }
    }
    void NuBridgeOn(i32 enabled) {
        NuBridgeProc = enabled;
    }
    void *TerrainGetCur(void);
    void TerrainSetCur(void *terrain);
    i32 DeletePlatinst(i32 index);
    i32 NewPlatInst(NUMTX *matrix, i32 instance);
    i32 NuBridgeCreate(NUGSCN *scene, nuhspecial_s *first, nuhspecial_s *second, NUVEC *start, NUVEC *end, f32 width,
                       i16 rotation, f32 af4, f32 afc, f32 af8, f32 b00, i32 count, f32 b04, f32 b08, i32 ae7,
                       u32 colour) {
        if (count > 24)
            printf("Too many sections/n");
        NuBridgeOn(1);
        i32 index = NuBridgeAlloc();
        if (index != -1) {
            NUBRIDGE *bridge = &Bridges[index];
            bridge->special = *first;
            bridge->second_special = *second;
            bridge->scene = scene;
            bridge->active = 1;
            bridge->idle_frames = 0;
            bridge->terrain = TerrainGetCur();
            bridge->platform_count = count;
            bridge->spring_strength = af4;
            bridge->damping = afc;
            bridge->width = width;
            bridge->gravity = af8;
            bridge->load_multiplier = b00;
            bridge->field_b04 = b04;
            bridge->field_b08 = b08;
            bridge->field_ae7 = ae7;
            bridge->colour = colour;
            bridge->centre.x = (end->x + start->x) * 0.5f;
            bridge->centre.y = (end->y + start->y) * 0.5f;
            bridge->centre.z = (end->z + start->z) * 0.5f;
            NUVEC delta = {end->x - start->x, end->y - start->y, end->z - start->z};
            bridge->cull_radius_squared = (delta.x * 0.5f) * (delta.x * 0.5f) + (delta.y * 0.5f) * (delta.y * 0.5f) +
                                          (delta.z * 0.5f) * (delta.z * 0.5f) + 1.0f;
            bridge->in_range = 0;
            f32 inverse_length = 1.0f / NuFsqrt(delta.x * delta.x + delta.z * delta.z);
            NUVEC side;
            side.x = -delta.z * inverse_length;
            side.y = 0.0f;
            side.z = delta.x * inverse_length;
            bridge->rotation_y = rotation;
            for (i32 i = 0; i < count; ++i) {
                if (NuSpecialExistsFn(&bridge->special)) {
                    bridge->platforms[i] =
                        NewPlatInst(&bridge->platform_matrices[i], NuSpecialGetInstanceix(&bridge->special));
                } else {
                    bridge->platforms[i] = -1;
                }
                NuMtxSetIdentity(&bridge->platform_matrices[i]);
                NuMtxPreRotateY(&bridge->platform_matrices[i], bridge->rotation_y);
                bridge->platform_matrices[i].m30 = delta.x * i / (count - 1) + start->x;
                bridge->platform_matrices[i].m31 = delta.y * i / (count - 1) + start->y;
                bridge->platform_matrices[i].m32 = delta.z * i / (count - 1) + start->z;
                bridge->edge_positions[i][0].x = delta.x * i / (count - 1) + start->x - side.x * width * 0.5f;
                bridge->edge_positions[i][0].y = delta.y * i / (count - 1) + start->y;
                bridge->edge_positions[i][0].z = delta.z * i / (count - 1) + start->z - side.z * width * 0.5f;
                bridge->edge_positions[i][1].x = delta.x * i / (count - 1) + start->x + side.x * width * 0.5f;
                bridge->edge_positions[i][1].y = delta.y * i / (count - 1) + start->y;
                bridge->edge_positions[i][1].z = delta.z * i / (count - 1) + start->z + side.z * width * 0.5f;
                bridge->edge_motion[i][0].x = 0.0f;
                bridge->edge_motion[i][0].y = 0.0f;
                bridge->edge_motion[i][0].z = 0.0f;
                bridge->edge_motion[i][1].x = 0.0f;
                bridge->edge_motion[i][1].y = 0.0f;
                bridge->edge_motion[i][1].z = 0.0f;
                bridge->contact_frames = 0;
            }
        }
        return index;
    }
    void NuBridgeInit(void) {
        i32 index;
        i32 i;
        NUBRIDGE *bridge = Bridges;
        void *terrain = TerrainGetCur();
        for (index = 0; index < 8; ++index, ++bridge) {
            if (bridge->active) {
                TerrainSetCur(bridge->terrain);
                for (i = 0; i < bridge->platform_count; ++i) {
                    if (NuSpecialExistsFn(&bridge->special))
                        DeletePlatinst(bridge->platforms[i]);
                }
                bridge->active = 0;
            }
        }
        TerrainSetCur(terrain);
    }
    void NuBridgeRemove(i32 index) {
        if (index >= 0 && index < 8) {
            void *terrain = TerrainGetCur();
            NUBRIDGE *bridge = &Bridges[index];
            if (bridge->active) {
                TerrainSetCur(bridge->terrain);
                for (i32 i = 0; i < bridge->platform_count; ++i) {
                    if (NuSpecialExistsFn(&bridge->special))
                        DeletePlatinst(bridge->platforms[i]);
                }
                bridge->active = 0;
            }
            TerrainSetCur(terrain);
        }
    }
}

i32 NuBridgeAlloc(void) {
    i32 index;
    NUBRIDGE *bridge = Bridges;
    for (index = 0; index < 8; ++index, ++bridge) {
        if (!bridge->active)
            return index;
    }
    return -1;
}

extern "C" i32 PlatInstGetHit(i32 index);
extern "C" void NuBridgeUpdate(NUVEC *position) {
    i32 b, i;
    NUBRIDGE *bridge;
    f32 t, side;
    i32 n;
    f32 left, right;
    NUVEC delta;
    if (NuBridgeProc) {
        bridge = Bridges;
        for (b = 0; b < 8; ++b, ++bridge) {
            if (bridge->active) {
                if ((bridge->centre.x - global_camera.mtx.m30) * (bridge->centre.x - global_camera.mtx.m30) +
                        (bridge->centre.y - global_camera.mtx.m31) * (bridge->centre.y - global_camera.mtx.m31) +
                        (bridge->centre.z - global_camera.mtx.m32) * (bridge->centre.z - global_camera.mtx.m32) <
                    global_camera.far_clip * global_camera.far_clip + bridge->cull_radius_squared) {
                    bridge->in_range = 1;
                    if (bridge->was_drawn) {
                        if (bridge->contact_frames) {
                            delta.x = (bridge->edge_positions[bridge->platform_count - 1][0].x +
                                       bridge->edge_positions[bridge->platform_count - 1][1].x -
                                       bridge->edge_positions[0][0].x - bridge->edge_positions[0][1].x) *
                                      0.5f;
                            delta.z = (bridge->edge_positions[bridge->platform_count - 1][0].z +
                                       bridge->edge_positions[bridge->platform_count - 1][1].z -
                                       bridge->edge_positions[0][0].z - bridge->edge_positions[0][1].z) *
                                      0.5f;
                            t = delta.x * (position->x -
                                           (bridge->edge_positions[0][0].x + bridge->edge_positions[0][1].x) * 0.5f) +
                                delta.z * (position->z -
                                           (bridge->edge_positions[0][0].z + bridge->edge_positions[0][1].z) * 0.5f);
                            t = t * (bridge->platform_count - 1) / (delta.x * delta.x + delta.z * delta.z);
                            if (t < 0.0f)
                                t = 0.0f;
                            if (t > bridge->platform_count - 1.0f)
                                t = bridge->platform_count - 1.0f;
                            n = t;
                            t -= n;
                            side = 2.0f / (NuFsqrt(delta.x * delta.x + delta.z * delta.z) * bridge->width);
                            side =
                                side *
                                (delta.z * (position->x -
                                            (bridge->edge_positions[0][0].x + bridge->edge_positions[0][1].x) * 0.5f) +
                                 delta.x * ((bridge->edge_positions[0][0].z + bridge->edge_positions[0][1].z) * 0.5f -
                                            position->z));
                            if (side > 1.0f)
                                side = 1.0f;
                            if (side < -1.0f)
                                side = -1.0f;
                            left = bridge->gravity * (3.0f - side) * 0.25f;
                            right = bridge->gravity * (side + 3.0f) * 0.25f;
                            if (t == 0.0f) {
                                if (n > 0 && n < bridge->platform_count - 1) {
                                    bridge->edge_motion[n][0].y =
                                        bridge->edge_motion[n][0].y + (left * bridge->load_multiplier);
                                    bridge->edge_motion[n][1].y =
                                        bridge->edge_motion[n][1].y + (right * bridge->load_multiplier);
                                }
                            } else {
                                if (n > 0 && n < bridge->platform_count - 1) {
                                    bridge->edge_motion[n][0].y =
                                        bridge->edge_motion[n][0].y + (left * bridge->load_multiplier * (1.0f - t));
                                    bridge->edge_motion[n][1].y =
                                        bridge->edge_motion[n][1].y + (right * bridge->load_multiplier * (1.0f - t));
                                }
                                if (n + 1 < bridge->platform_count - 1) {
                                    bridge->edge_motion[n + 1][0].y =
                                        bridge->edge_motion[n + 1][0].y + (left * bridge->load_multiplier * t);
                                    bridge->edge_motion[n + 1][1].y =
                                        bridge->edge_motion[n + 1][1].y + (right * bridge->load_multiplier * t);
                                }
                            }
                            --bridge->contact_frames;
                        } else {
                            left = 0.0f;
                        }
                        if (PlatInstGetHit(bridge->platforms[0]))
                            bridge->contact_frames = 5;
                        if (PlatInstGetHit(bridge->platforms[bridge->platform_count - 1]))
                            bridge->contact_frames = 5;
                        for (i = 1; i < bridge->platform_count - 1; ++i) {
                            if (PlatInstGetHit(bridge->platforms[i]))
                                bridge->contact_frames = 5;
                        }
                        if (bridge->contact_frames)
                            bridge->idle_frames = 0;
                        else if (bridge->idle_frames < 16384)
                            ++bridge->idle_frames;
                        if (bridge->idle_frames < 180) {
                            for (i = 1; i < bridge->platform_count - 1; ++i) {
                                bridge->edge_motion[i][0].x =
                                    bridge->edge_motion[i][0].x + (-bridge->edge_motion[i][0].x * bridge->damping);
                                bridge->edge_motion[i][0].y =
                                    bridge->edge_motion[i][0].y +
                                    (bridge->gravity - bridge->edge_motion[i][0].y * bridge->damping);
                                bridge->edge_motion[i][0].z =
                                    bridge->edge_motion[i][0].z + (-bridge->edge_motion[i][0].z * bridge->damping);
                                bridge->edge_motion[i][1].x =
                                    bridge->edge_motion[i][1].x + (-bridge->edge_motion[i][1].x * bridge->damping);
                                bridge->edge_motion[i][1].y =
                                    bridge->edge_motion[i][1].y +
                                    (bridge->gravity - bridge->edge_motion[i][1].y * bridge->damping);
                                bridge->edge_motion[i][1].z =
                                    bridge->edge_motion[i][1].z + (-bridge->edge_motion[i][1].z * bridge->damping);
                                bridge->edge_motion[i][0].x =
                                    NuFabs((bridge->edge_positions[i - 1][0].x - bridge->edge_positions[i][0].x)) *
                                        (bridge->edge_positions[i - 1][0].x - bridge->edge_positions[i][0].x) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i - 1][0].x - bridge->edge_positions[i][0].x) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][0].x;
                                bridge->edge_motion[i][0].y =
                                    NuFabs((bridge->edge_positions[i - 1][0].y - bridge->edge_positions[i][0].y)) *
                                        (bridge->edge_positions[i - 1][0].y - bridge->edge_positions[i][0].y) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i - 1][0].y - bridge->edge_positions[i][0].y) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][0].y;
                                bridge->edge_motion[i][0].z =
                                    NuFabs((bridge->edge_positions[i - 1][0].z - bridge->edge_positions[i][0].z)) *
                                        (bridge->edge_positions[i - 1][0].z - bridge->edge_positions[i][0].z) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i - 1][0].z - bridge->edge_positions[i][0].z) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][0].z;
                                bridge->edge_motion[i][0].x =
                                    NuFabs((bridge->edge_positions[i + 1][0].x - bridge->edge_positions[i][0].x)) *
                                        (bridge->edge_positions[i + 1][0].x - bridge->edge_positions[i][0].x) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i + 1][0].x - bridge->edge_positions[i][0].x) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][0].x;
                                bridge->edge_motion[i][0].y =
                                    NuFabs((bridge->edge_positions[i + 1][0].y - bridge->edge_positions[i][0].y)) *
                                        (bridge->edge_positions[i + 1][0].y - bridge->edge_positions[i][0].y) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i + 1][0].y - bridge->edge_positions[i][0].y) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][0].y;
                                bridge->edge_motion[i][0].z =
                                    NuFabs((bridge->edge_positions[i + 1][0].z - bridge->edge_positions[i][0].z)) *
                                        (bridge->edge_positions[i + 1][0].z - bridge->edge_positions[i][0].z) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i + 1][0].z - bridge->edge_positions[i][0].z) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][0].z;
                                bridge->edge_motion[i][1].x =
                                    NuFabs((bridge->edge_positions[i - 1][1].x - bridge->edge_positions[i][1].x)) *
                                        (bridge->edge_positions[i - 1][1].x - bridge->edge_positions[i][1].x) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i - 1][1].x - bridge->edge_positions[i][1].x) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][1].x;
                                bridge->edge_motion[i][1].y =
                                    NuFabs((bridge->edge_positions[i - 1][1].y - bridge->edge_positions[i][1].y)) *
                                        (bridge->edge_positions[i - 1][1].y - bridge->edge_positions[i][1].y) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i - 1][1].y - bridge->edge_positions[i][1].y) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][1].y;
                                bridge->edge_motion[i][1].z =
                                    NuFabs((bridge->edge_positions[i - 1][1].z - bridge->edge_positions[i][1].z)) *
                                        (bridge->edge_positions[i - 1][1].z - bridge->edge_positions[i][1].z) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i - 1][1].z - bridge->edge_positions[i][1].z) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][1].z;
                                bridge->edge_motion[i][1].x =
                                    NuFabs((bridge->edge_positions[i + 1][1].x - bridge->edge_positions[i][1].x)) *
                                        (bridge->edge_positions[i + 1][1].x - bridge->edge_positions[i][1].x) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i + 1][1].x - bridge->edge_positions[i][1].x) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][1].x;
                                bridge->edge_motion[i][1].y =
                                    NuFabs((bridge->edge_positions[i + 1][1].y - bridge->edge_positions[i][1].y)) *
                                        (bridge->edge_positions[i + 1][1].y - bridge->edge_positions[i][1].y) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i + 1][1].y - bridge->edge_positions[i][1].y) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][1].y;
                                bridge->edge_motion[i][1].z =
                                    NuFabs((bridge->edge_positions[i + 1][1].z - bridge->edge_positions[i][1].z)) *
                                        (bridge->edge_positions[i + 1][1].z - bridge->edge_positions[i][1].z) *
                                        bridge->spring_strength * 0.5f +
                                    (bridge->edge_positions[i + 1][1].z - bridge->edge_positions[i][1].z) *
                                        bridge->spring_strength +
                                    bridge->edge_motion[i][1].z;
                            }
                            for (i = 1; i < bridge->platform_count - 1; ++i) {
                                bridge->edge_positions[i][0].x =
                                    bridge->edge_positions[i][0].x + (bridge->edge_motion[i][0].x);
                                bridge->edge_positions[i][0].y =
                                    bridge->edge_positions[i][0].y + (bridge->edge_motion[i][0].y);
                                bridge->edge_positions[i][0].z =
                                    bridge->edge_positions[i][0].z + (bridge->edge_motion[i][0].z);
                                bridge->edge_positions[i][1].x =
                                    bridge->edge_positions[i][1].x + (bridge->edge_motion[i][1].x);
                                bridge->edge_positions[i][1].y =
                                    bridge->edge_positions[i][1].y + (bridge->edge_motion[i][1].y);
                                bridge->edge_positions[i][1].z =
                                    bridge->edge_positions[i][1].z + (bridge->edge_motion[i][1].z);
                                NuMtxSetRotationY(&bridge->platform_matrices[i], bridge->rotation_y);
                                left = NuFsqrt((bridge->edge_positions[i][1].x - bridge->edge_positions[i][0].x) *
                                                   (bridge->edge_positions[i][1].x - bridge->edge_positions[i][0].x) +
                                               (bridge->edge_positions[i][1].z - bridge->edge_positions[i][0].z) *
                                                   (bridge->edge_positions[i][1].z - bridge->edge_positions[i][0].z));
                                NuMtxPreRotateX(
                                    &bridge->platform_matrices[i],
                                    NuAtan2D((bridge->edge_positions[i][1].y - bridge->edge_positions[i][0].y), left));
                                left = NuFsqrt(
                                    (bridge->edge_positions[i + 1][0].x + bridge->edge_positions[i + 1][1].x -
                                     bridge->edge_positions[i - 1][0].x - bridge->edge_positions[i - 1][1].x) *
                                        (bridge->edge_positions[i + 1][0].x + bridge->edge_positions[i + 1][1].x -
                                         bridge->edge_positions[i - 1][0].x - bridge->edge_positions[i - 1][1].x) +
                                    (bridge->edge_positions[i + 1][0].z + bridge->edge_positions[i + 1][1].z -
                                     bridge->edge_positions[i - 1][0].z - bridge->edge_positions[i - 1][1].z) *
                                        (bridge->edge_positions[i + 1][0].z + bridge->edge_positions[i + 1][1].z -
                                         bridge->edge_positions[i - 1][0].z - bridge->edge_positions[i - 1][1].z));
                                NuMtxPreRotateZ(
                                    &bridge->platform_matrices[i],
                                    NuAtan2D((bridge->edge_positions[i + 1][0].y + bridge->edge_positions[i + 1][1].y -
                                              bridge->edge_positions[i - 1][0].y - bridge->edge_positions[i - 1][1].y),
                                             left));
                                bridge->platform_matrices[i].m30 =
                                    (bridge->edge_positions[i][0].x + bridge->edge_positions[i][1].x) * 0.5f;
                                bridge->platform_matrices[i].m31 =
                                    (bridge->edge_positions[i][0].y + bridge->edge_positions[i][1].y) * 0.5f;
                                bridge->platform_matrices[i].m32 =
                                    (bridge->edge_positions[i][0].z + bridge->edge_positions[i][1].z) * 0.5f;
                            }
                            NuMtxSetRotationY(&bridge->platform_matrices[0], bridge->rotation_y);
                            left = NuFsqrt((bridge->edge_positions[0][1].x - bridge->edge_positions[0][0].x) *
                                               (bridge->edge_positions[0][1].x - bridge->edge_positions[0][0].x) +
                                           (bridge->edge_positions[0][1].z - bridge->edge_positions[0][0].z) *
                                               (bridge->edge_positions[0][1].z - bridge->edge_positions[0][0].z));
                            NuMtxPreRotateX(
                                &bridge->platform_matrices[0],
                                NuAtan2D((bridge->edge_positions[0][1].y - bridge->edge_positions[0][0].y), left));
                            left = NuFsqrt((bridge->edge_positions[1][0].x + bridge->edge_positions[1][1].x -
                                            bridge->edge_positions[0][0].x - bridge->edge_positions[0][1].x) *
                                               (bridge->edge_positions[1][0].x + bridge->edge_positions[1][1].x -
                                                bridge->edge_positions[0][0].x - bridge->edge_positions[0][1].x) +
                                           (bridge->edge_positions[1][0].z + bridge->edge_positions[1][1].z -
                                            bridge->edge_positions[0][0].z - bridge->edge_positions[0][1].z) *
                                               (bridge->edge_positions[1][0].z + bridge->edge_positions[1][1].z -
                                                bridge->edge_positions[0][0].z - bridge->edge_positions[0][1].z));
                            right = (bridge->edge_positions[0][0].y + bridge->edge_positions[0][1].y +
                                     bridge->edge_positions[1][0].y + bridge->edge_positions[1][1].y) *
                                    0.25f;
                            NuMtxPreRotateZ(
                                &bridge->platform_matrices[0],
                                NuAtan2D(bridge->edge_positions[1][0].y + bridge->edge_positions[1][1].y - right * 2.0f,
                                         left));
                            bridge->platform_matrices[0].m30 =
                                (bridge->edge_positions[0][0].x + bridge->edge_positions[0][1].x) * 0.5f;
                            bridge->platform_matrices[0].m31 = right;
                            bridge->platform_matrices[0].m32 =
                                (bridge->edge_positions[0][0].z + bridge->edge_positions[0][1].z) * 0.5f;
                            NuMtxSetRotationY(&bridge->platform_matrices[bridge->platform_count - 1],
                                              bridge->rotation_y);
                            left = NuFsqrt((bridge->edge_positions[bridge->platform_count - 2][1].x -
                                            bridge->edge_positions[bridge->platform_count - 2][0].x) *
                                               (bridge->edge_positions[bridge->platform_count - 2][1].x -
                                                bridge->edge_positions[bridge->platform_count - 2][0].x) +
                                           (bridge->edge_positions[bridge->platform_count - 2][1].z -
                                            bridge->edge_positions[bridge->platform_count - 2][0].z) *
                                               (bridge->edge_positions[bridge->platform_count - 2][1].z -
                                                bridge->edge_positions[bridge->platform_count - 2][0].z));
                            // The original applies the last endpoint tilt to matrix zero.
                            NuMtxPreRotateX(&bridge->platform_matrices[0],
                                            NuAtan2D((bridge->edge_positions[bridge->platform_count - 2][1].y -
                                                      bridge->edge_positions[bridge->platform_count - 2][0].y),
                                                     left));
                            left = NuFsqrt((bridge->edge_positions[bridge->platform_count - 1][0].x +
                                            bridge->edge_positions[bridge->platform_count - 1][1].x -
                                            bridge->edge_positions[bridge->platform_count - 2][0].x -
                                            bridge->edge_positions[bridge->platform_count - 2][1].x) *
                                               (bridge->edge_positions[bridge->platform_count - 1][0].x +
                                                bridge->edge_positions[bridge->platform_count - 1][1].x -
                                                bridge->edge_positions[bridge->platform_count - 2][0].x -
                                                bridge->edge_positions[bridge->platform_count - 2][1].x) +
                                           (bridge->edge_positions[bridge->platform_count - 1][0].z +
                                            bridge->edge_positions[bridge->platform_count - 1][1].z -
                                            bridge->edge_positions[bridge->platform_count - 2][0].z -
                                            bridge->edge_positions[bridge->platform_count - 2][1].z) *
                                               (bridge->edge_positions[bridge->platform_count - 1][0].z +
                                                bridge->edge_positions[bridge->platform_count - 1][1].z -
                                                bridge->edge_positions[bridge->platform_count - 2][0].z -
                                                bridge->edge_positions[bridge->platform_count - 2][1].z));
                            right = (bridge->edge_positions[bridge->platform_count - 1][0].y +
                                     bridge->edge_positions[bridge->platform_count - 1][1].y +
                                     bridge->edge_positions[bridge->platform_count - 2][0].y +
                                     bridge->edge_positions[bridge->platform_count - 2][1].y) *
                                    0.25f;
                            NuMtxPreRotateZ(&bridge->platform_matrices[bridge->platform_count - 1],
                                            NuAtan2D(bridge->edge_positions[bridge->platform_count - 1][0].y +
                                                         bridge->edge_positions[bridge->platform_count - 1][1].y -
                                                         right * 2.0f,
                                                     left));
                            bridge->platform_matrices[bridge->platform_count - 1].m30 =
                                (bridge->edge_positions[bridge->platform_count - 1][0].x +
                                 bridge->edge_positions[bridge->platform_count - 1][1].x) *
                                0.5f;
                            bridge->platform_matrices[bridge->platform_count - 1].m31 = right;
                            bridge->platform_matrices[bridge->platform_count - 1].m32 =
                                (bridge->edge_positions[bridge->platform_count - 1][0].z +
                                 bridge->edge_positions[bridge->platform_count - 1][1].z) *
                                0.5f;
                        }
                    }
                } else {
                    bridge->in_range = 0;
                }
            }
        }
    }
}
