#pragma once

#include "nu2api/nucore/common.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec4.h"
#include "nu2api/numath/nuvec.h"

struct nuframebuffer_s;
struct nugscn_s;
struct nushaderprogram_s;
struct VuVec;
struct VuMtx {
    NUMTX matrix;
};
struct nucamera_s;
struct nuvec_s;
struct numtx_s;

struct NuDynamicLight {
    struct RenderSet {
        RenderSet();
        NUMTX view, projection, warp;
        NUMTX shadow_transform;
        union {
            u8 reserved_100[0x110 - 0x100];
            struct {
                f32 parameter_100, parameter_104;
                u32 reserved_108[2];
            };
        };
        union {
            f32 warp_factor;
            f32 parameter_110;
        };
        NUVEC4 corners[8];
        NUVEC4 shadow_planes[12];
        i32 shadow_plane_count;
        NUVEC4 far_plane;
        NUVEC4 capsule_center, capsule_end;
        f32 capsule_radius;
        union {
            struct {
                NUDISPLAYLIST display_lists[2];
                nurndrstate_s *render_states[2];
                NUDISPLAYLISTITEM *list_items[2];
                NUDISPLAYLISTITEM *scene_cursor[2];
                NUDISPLAYLISTITEM *scene_first[2];
                NUDISPLAYLISTITEM *scene_end[2];
            };
            struct {
                u8 reserved_28c[0x10];
                i32 field_29c;
                i32 field_2a0;
                u8 reserved_2a4[0x3c];
                i32 field_2e0;
                i32 field_2e4;
                u8 reserved_2e8[0x54];
            };
        };
        union {
            u32 reserved_33c[8];
            nugscn_s *render_scenes[8];
        };
        union {
            i32 geometry_count;
            i32 render_scene_count;
        };
    };
    NuDynamicLight();
    static NUMTX cacheCameraView, cacheCameraProj;
    void addShadowCasterScene(nugscn_s *);
    void bindShaderResources(nushaderprogram_s *);
    NuDynamicLight *clone(variptr_u *, variptr_u);
    static void computeBoundingSpace(VuVec const *, VuMtx *);
    static void computeClippingPlanes(VuMtx const &, bool, VuVec &, VuVec &, VuVec &, VuVec &, VuVec &, VuVec &);
    static void computeFrustumCube(nucamera_s const *, VuVec *, VuVec *);
    static void computeLightSpace(nuvec_s *, nuvec_s *, numtx_s *, numtx_s *);
    static i32 computeShadowClippingPlanes(VuVec const &, VuVec const *, VuVec *);
    static void computeShadowFrustrumCapsule(VuVec const &, VuVec const *, VuVec &, VuVec &, float &);
    void computeWarpEffect(NuDynamicLight::RenderSet &);
    static NuDynamicLight *create();
    static void destroy(NuDynamicLight *);
    void refreshShadowTransform(NuDynamicLight::RenderSet &);
    void renderShadowMap(i32, nuframebuffer_s *);
    void resetGeometry();
    void setCameraViewProj(numtx_s *, numtx_s *);
    void setupCustomCameraFrustum(nucamera_s *, float const *, i32);
    bool testShadowExtrusion(VuVec const &, VuVec const &, i32);
    i32 testShadowExtrusions(VuVec const &, VuVec const &);

    RenderSet render_sets[2];
    union {
        u32 reserved_6c0[3];
        f32 split_distances[3];
    };
    union {
        i32 render_set_capacity;
        i32 render_set_count;
    };
    i32 active_render_set_count;
    union {
        f32 reserved_6d4[20];
        struct {
            NUVEC4 split_planes[2];
            NUVEC4 camera_position, camera_forward, reserved_714;
        };
        struct {
            u8 reserved_6d4_bytes[0x40];
            NUVEC position;
            f32 position_w;
        };
    };
    NUVEC direction;
    union {
        f32 reserved_730;
        f32 direction_w;
    };
    NUMTX view;
    NUMTX projection;
    i32 parameter_4;
    i32 parameter_5;
    union {
        u32 reserved_7bc;
        i32 enabled;
    };
    union {
        f32 reserved_7c0[2];
        struct {
            f32 parameter_7c0;
            f32 parameter_7c4;
        };
    };
    union {
        NUVEC4 reserved_7c8;
        struct {
            f32 parameter_7c8;
            f32 parameter_7cc;
            f32 parameter_7d0;
            f32 parameter_7d4;
        };
    };
    i32 used_on_specials;
};
DECOMP_ASSERT(offsetof(NuDynamicLight, parameter_4) == 0x7b4, "Dynamic light parameter 4 offset");
DECOMP_ASSERT(sizeof(NuDynamicLight::RenderSet) == 0x360, "Dynamic light render-set stride");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, parameter_100) == 0x100, "Dynamic light render parameter 100 offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, parameter_104) == 0x104, "Dynamic light render parameter 104 offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, parameter_110) == 0x110, "Dynamic light render parameter 110 offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, render_scenes) == 0x33c, "Dynamic light render-scene list offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, render_scene_count) == 0x35c,
              "Dynamic light render-scene count offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, render_set_count) == 0x6cc, "Dynamic light render-set count offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, position) == 0x714, "Dynamic light position offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, direction) == 0x724, "Dynamic light direction offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, view) == 0x734, "Dynamic light view matrix offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, enabled) == 0x7bc, "Dynamic light enabled offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, parameter_7c0) == 0x7c0, "Dynamic light parameter 7c0 offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, parameter_7d4) == 0x7d4, "Dynamic light parameter 7d4 offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, parameter_5) == 0x7b8, "Dynamic light parameter 5 offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, used_on_specials) == 0x7d8, "Dynamic light special-use flag offset");

DECOMP_ASSERT(sizeof(NuDynamicLight::RenderSet) == 0x360, "Dynamic light render set stride");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, warp_factor) == 0x110, "Dynamic light warp factor offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, direction) == 0x724, "Dynamic light direction offset");

DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, shadow_plane_count) == 0x254,
              "Dynamic light shadow plane count offset");

DECOMP_ASSERT(sizeof(VuMtx) == 0x40, "Vector matrix size");

DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, far_plane) == 0x258, "Dynamic light far plane offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, capsule_radius) == 0x288, "Dynamic light capsule radius offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, display_lists) == 0x28c, "Dynamic light display lists offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, geometry_count) == 0x35c, "Dynamic light geometry count offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, render_set_capacity) == 0x6cc, "Dynamic light render set capacity offset");
DECOMP_ASSERT(sizeof(NuDynamicLight) == 0x7dc, "Dynamic light state size before arena alignment");

DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, render_states) == 0x314,
              "Dynamic light render state pointers offset");
DECOMP_ASSERT(offsetof(NuDynamicLight::RenderSet, list_items) == 0x31c, "Dynamic light list item pointers offset");

extern "C" void NuDynamicLightSetupCustomCameraFrustum(NuDynamicLight *, nucamera_s *, const f32 *, i32);

DECOMP_ASSERT(offsetof(NuDynamicLight, split_distances) == 0x6c0, "Dynamic light split distances offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, split_planes) == 0x6d4, "Dynamic light split planes offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, camera_position) == 0x6f4, "Dynamic light camera position offset");
DECOMP_ASSERT(offsetof(NuDynamicLight, camera_forward) == 0x704, "Dynamic light camera forward offset");

extern "C" {
    NuDynamicLight *NuDynamicLightCreate();
    NuDynamicLight *NuDynamicLightClone(NuDynamicLight *, VARIPTR *, VARIPTR);
    void NuDynamicLightDestroy(NuDynamicLight *);
    void NuDynamicLightResetGeometry(NuDynamicLight *);
    void NuDynamicLightAddRenderScene(NuDynamicLight *, i32, i32);
}

extern "C" void NuDynamicLightAddShadowCasterScene(NuDynamicLight *, nugscn_s *);
