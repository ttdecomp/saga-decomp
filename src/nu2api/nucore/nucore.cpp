#include <float.h>
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nuapi.h"

#include <new>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "nu2api/nucore/nustring.h"

#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nucore/NuCopyFilter.h"
#include "nu2api/nucore/NuDataPortManager.h"
#include "nu2api/nucore/NuDeferredFilter.h"
#include "nu2api/nucore/NuDeferredFilterGen.h"
#include "nu2api/nucore/NuDeviceSpecs.h"
#include "nu2api/nucore/NuDynamicLight.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/numtl.h"
extern "C" void DisplaySceneRndrSpecials(NUDLDLISTSCENE *, i32, void *);
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nucore/NuMainFilter.h"
#include "nu2api/nucore/NuMainFilterGen.h"
#include "nu2api/nucore/NuMotionAccumFilter.h"
#include "nu2api/nucore/NuMotionAccumFilterGen.h"
#include "nu2api/nucore/NuMotionFilter.h"
#include "nu2api/nucore/NuMotionFilterGen.h"
#include "nu2api/nucore/NuNetEmu.h"
#include "nu2api/nucore/NuPlatform.h"
#include "nu2api/nucore/NuPostFilter.h"
#include "nu2api/nucore/NuPostFilterGen.h"
#include "nu2api/nucore/NuSpeedBlurFilter.h"
#include "nu2api/nucore/NuSpeedBlurFilterGen.h"
#include "nu2api/nucore/NuVoiceAndroid.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nu3d/nupostresources.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nu3d/android/nuiosdl_gl.h"
#include "nu2api/nu3d/android/nupostshaders.h"
#include "nu2api/numath/nuvec4.h"
#include "nu2api/numath/nufloat.h"

extern "C" f32 g_renderContext_kTint[4];
extern const f32 nuvec4_one[4];
extern "C" void NuShaderManagerSetfv(i32, const f32 *);
struct VuVec {
    f32 x, y, z, w;
};
static f32 motionFactorPan = 0.01f;
static f32 motionFactorPull = -0.024f;
static f32 motionFactorPanClamp = 0.02f;

extern "C" void NuSpeedBlurSetMotionFactors(f32 pan, f32 pull, f32 clamp) {
    motionFactorPan = pan;
    motionFactorPull = pull;
    motionFactorPanClamp = clamp;
}

u32 NuPostFilter::m_fullscreenVertexBuffer, NuPostFilter::m_fullscreenIndexBuffer;
u32 NuPostFilter::m_fullscreenVertexFormat, NuPostFilter::m_fullscreenGridVertexBuffer;
u32 NuPostFilter::m_fullscreenGridIndexBuffer;
i32 NuPostFilter::m_quadGridPrimCount;
extern u32 g_lastBoundVAO;
extern void *g_nuFullscreenVertexFormat;
extern "C" f32 g_renderContext_projection[16];
extern "C" f32 g_renderContext_view[16];
extern "C" void NuRenderContextSetViewProj(NUMTX *, NUMTX *);
extern "C" void NuFramebufferClear(u32, u32);
extern "C" f32 NuPow(f32, f32);
extern "C" f32 NuLog2(f32);

static void PostBindProgram(nushaderprogram_s *program) {
    g_boundShader = program != NULL ? program->program : 0;
    glUseProgram(g_boundShader);
    g_currentShaderProgram = program;
}

// These draw sequences are inlined into the original generic filters.
// NuPostFilterGen::renderQuad/Grid themselves are Android no-ops.
static void PostDrawQuad(bool grid = false) {
    g_lastBoundVAO = 0;
    glBindBuffer(GL_ARRAY_BUFFER,
                 grid ? NuPostFilter::m_fullscreenGridVertexBuffer : NuPostFilter::m_fullscreenVertexBuffer);
    NuIOS_SetVertexFormat(reinterpret_cast<usize>(g_nuFullscreenVertexFormat));
    if (grid) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NuPostFilter::m_fullscreenGridIndexBuffer);
        glDrawElements(GL_TRIANGLES, NuPostFilter::m_quadGridPrimCount * 3, GL_UNSIGNED_SHORT, NULL);
    } else {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

NuDataPortManager NuPostFilterGen::resourceManager;
NuPostDataPort NuPostFilterGen::portOutFramebuffer = {-1, NULL};
NuPostDataPort NuPostFilterGen::portColorBuffer = {-1, NULL};
NuPostDataPort NuPostFilterGen::portNormalBuffer = {-1, NULL};
NuPostDataPort NuPostFilterGen::portVelocityBuffer = {-1, NULL};
NuPostDataPort NuPostFilterGen::portDepthRTBuffer = {-1, NULL};
NuPostDataPort NuPostFilterGen::portDepthBuffer = {-1, NULL};
nuframebuffer_s *NuPostFilterGen::blurFbo, *NuPostFilterGen::copyFbo;
nueffecttex_s *NuPostFilterGen::workTex;
nushaderprogram_s *NuPostFilterGen::copyTexProgram, *NuPostFilterGen::copyTexLodProgram;
nushaderprogram_s *NuPostFilterGen::copyTexColorDepthProgram, *NuPostFilterGen::blendTexProgram;
nushaderprogram_s *NuPostFilterGen::blur5x5Program, *NuPostFilterGen::blur7x7Program;
nushaderprogram_s *NuPostFilterGen::blurGuardProgram;

NuApplicationState *NuCore::m_applicationState;
NuThreadManager *NuCore::m_threadManager;

void NuCore::Initialize() {
    GetApplicationState();

    m_threadManager = new NuThreadManager();
}

NuApplicationState *NuCore::GetApplicationState(void) {
    if (m_applicationState != NULL) {
        return m_applicationState;
    }

    NuApplicationState *state = NU_ALLOC_T(NuApplicationState, 1, "", NUMEMORY_CATEGORY_NONE);
    if (state != NULL) {
        new (state) NuApplicationState();
    }

    m_applicationState = state;

    return state;
}

void NuCopyFilter::destroyResources() {
    NuFramebufferDestroy(copy_fbo);
    NuPostFilterGen::destroyResources();
}

void NuCopyFilter::initResources() {
    NuPostFilterGen::initResources();
    copy_fbo = NuFramebufferCreate();
    NuFramebufferAttachTex2D(copy_fbo, 0, workTex, 0);
    input_fbo = copy_fbo;
}

void NuCopyFilter::render(nuframebuffer_s *output) {
    nueffecttex_s *color = NuFramebufferGetAttachedTex(input_fbo, 0, NULL, NULL);
    if (copy_texture != NULL)
        copy(color, copy_texture, output);
    else
        copy(color, output);
}

void NuCopyFilter::reset() {
    input_fbo = copy_fbo;
    copy_texture = NULL;
}

void NuMainFilter::initResources() {
    NuMainFilterGen::initResources();
    programs[15] = NuShaderProgramCreateIOS(blur7x7_vx, blur7x7dof_px);
}

void NuPostFilter::initSharedResources(i32, i32) {
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nupostfilter.cpp", 0x2a);
    NuPostFilterGen::initSharedResources();
    NuPostFilterGen::copyTexProgram = NuShaderProgramCreateIOS(default_vx, copytex_px);
    NuPostFilterGen::copyTexLodProgram = NULL;
    NuPostFilterGen::blur5x5Program = NULL;
    NuPostFilterGen::blur7x7Program = NULL;
    NuPostFilterGen::blurGuardProgram = NULL;
    const f32 vertices[12] = {-1, 1, 0, -1, -1, 0, 1, 1, 0, 1, -1, 0};
    glGenBuffers(1, &m_fullscreenVertexBuffer);
    if (g_lastBoundVAO != 0)
        g_lastBoundVAO = 0;
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &m_fullscreenGridVertexBuffer);
    glGenBuffers(1, &m_fullscreenGridIndexBuffer);
    i32 rows = static_cast<i32>((static_cast<f32>(nurndr_pixel_height) / nurndr_pixel_width) * 16.0f);
    i32 size = (rows + 1) * 17 * 3 * sizeof(f32);
    f32 *grid = static_cast<f32 *>(NU_ALLOC(size, 4, 1, "", NUMEMORY_CATEGORY_NONE));
    // The original expands each fixed-width row into 17 vertex writes.
    f32 *row = grid;
    for (i32 y = 0; y <= rows; ++y, row += 51) {
        const f32 fraction = static_cast<f32>(y) / rows;
        const f32 screen_y = 1.0f - (fraction + fraction);
#define GRID_VERTEX(column)                                                                                            \
    row[(column) * 3] = -1.0f + (column) * 0.125f;                                                                     \
    row[(column) * 3 + 1] = screen_y;                                                                                  \
    row[(column) * 3 + 2] = 0.0f
        GRID_VERTEX(0);
        GRID_VERTEX(1);
        GRID_VERTEX(2);
        GRID_VERTEX(3);
        GRID_VERTEX(4);
        GRID_VERTEX(5);
        GRID_VERTEX(6);
        GRID_VERTEX(7);
        GRID_VERTEX(8);
        GRID_VERTEX(9);
        GRID_VERTEX(10);
        GRID_VERTEX(11);
        GRID_VERTEX(12);
        GRID_VERTEX(13);
        GRID_VERTEX(14);
        GRID_VERTEX(15);
        GRID_VERTEX(16);
#undef GRID_VERTEX
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_fullscreenGridVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, size, grid, GL_STATIC_DRAW);
    NU_FREE(grid);
    u16 *indices = static_cast<u16 *>(NU_ALLOC(rows * 192, 4, 1, "", NUMEMORY_CATEGORY_NONE));
    // Each row contains 16 cells, with the original two-triangle winding.
    u16 *index_row = indices;
    i32 vertex = 0;
    for (i32 y = 0; y < rows; ++y, index_row += 96, vertex += 17) {
#define GRID_CELL(column)                                                                                              \
    index_row[(column) * 6] = vertex + (column);                                                                       \
    index_row[(column) * 6 + 1] = vertex + (column) + 1;                                                               \
    index_row[(column) * 6 + 2] = vertex + (column) + 17;                                                              \
    index_row[(column) * 6 + 3] = vertex + (column) + 17;                                                              \
    index_row[(column) * 6 + 4] = vertex + (column) + 1;                                                               \
    index_row[(column) * 6 + 5] = vertex + (column) + 18
        GRID_CELL(0);
        GRID_CELL(1);
        GRID_CELL(2);
        GRID_CELL(3);
        GRID_CELL(4);
        GRID_CELL(5);
        GRID_CELL(6);
        GRID_CELL(7);
        GRID_CELL(8);
        GRID_CELL(9);
        GRID_CELL(10);
        GRID_CELL(11);
        GRID_CELL(12);
        GRID_CELL(13);
        GRID_CELL(14);
        GRID_CELL(15);
#undef GRID_CELL
    }
    if (g_lastBoundVAO != 0)
        g_lastBoundVAO = 0;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_fullscreenGridIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rows * 192, indices, GL_STATIC_DRAW);
    NU_FREE(indices);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nupostfilter.cpp", 0x7d);
}

void NuPostFilter::renderFrustum(numtx_s *) {
}

#include "nu2api/nu3d/nurndrstat.h"
template <typename T> struct LightObjectPool {
    struct __attribute__((aligned(16))) Slot {
        // Construction is explicit; reserving the pool must not construct every light.
        u8 storage[sizeof(T)];
    } slots[8];
    i8 occupied;
    i32 next;

    T *allocate() {
        i32 i;
        for (i = next; i < 8; ++i) {
            if ((occupied & (1 << (i & 7))) == 0) {
                T *result = reinterpret_cast<T *>(slots[i].storage);
                occupied |= 1 << (i & 7);
                next = (i + 1) % 8;
                return result;
            }
        }
        for (i = 0; i < next; ++i) {
            if ((occupied & (1 << (i & 7))) == 0) {
                T *result = reinterpret_cast<T *>(slots[i].storage);
                occupied |= 1 << (i & 7);
                next = (i + 1) % 8;
                return result;
            }
        }
        return NULL;
    }
};
static LightObjectPool<NUDISPLAYLISTITEM> dlistItemPool;
static LightObjectPool<NURNDRSTATE> rndrStatePool;
static LightObjectPool<NuDynamicLight> dynamicLightPool;
DECOMP_ASSERT(sizeof(dynamicLightPool) == 16144, "Dynamic light pool size");
DECOMP_ASSERT(sizeof(dlistItemPool) == 144, "Dynamic light list item pool size");
DECOMP_ASSERT(sizeof(rndrStatePool) == 528, "Dynamic light render state pool size");

NuDynamicLight::RenderSet::RenderSet() {
    parameter_100 = 0.01f;
    geometry_count = 0;
    shadow_plane_count = 0;
    parameter_104 = 0.001f;
    warp_factor = 1.0f;
    for (i32 i = 0; i < 2; ++i) {
        list_items[i] = dlistItemPool.allocate();
        render_states[i] = rndrStatePool.allocate();
        memset(&display_lists[i], 0, sizeof(display_lists[i]));
        NUDISPLAYLISTITEM *item = list_items[i];
        display_lists[i].first = item;
        item->type = 0x8d;
        item->next = NULL;
        item->id = 1;
        display_lists[i].mtl_last = display_lists[i].first;
        display_lists[i].state = render_states[i];
        NuDisplayListReset(&display_lists[i]);
    }
}
NuDynamicLight::NuDynamicLight() {
    reserved_7bc = 0;
    render_set_capacity = 2;
    active_render_set_count = 0;
    parameter_4 = 0;
    parameter_5 = 0;
    used_on_specials = 0;
    reserved_7c0[1] = 4.0f;
    reserved_7c8.x = 0.25f;
    reserved_7c0[0] = 0.8f;
    reserved_7c8.y = 0.0f;
    reserved_7c8.z = 100.0f;
    reserved_7c8.w = 110.0f;
}

void NuDynamicLight::addShadowCasterScene(nugscn_s *scene) {
    if (render_set_capacity > 0) {
        RenderSet *current = render_sets;
        RenderSet *end = render_sets + render_set_capacity;
        do {
            RenderSet &set = *current;
            set.scene_cursor[0] = NULL;
            set.scene_first[0] = NULL;
            set.scene_end[0] = NULL;
            set.scene_cursor[1] = NULL;
            set.scene_first[1] = NULL;
            set.scene_end[1] = NULL;
        } while (++current != end);
    }
    NUDLDLISTSCENE *dl = scene->display_list;
    i32 instance = 0;
    for (i32 lod = 0; lod < dl->nclip_objects; ++instance, ++lod) {
        if ((dl->visibility_flags[instance] & 0x21) == 0x21) {
            if (dl->lod_ranges[lod] != 0.0f) {
                f32 dx = global_camera.mtx.m30 - dl->clip_bounds[instance].center.x;
                f32 dy = global_camera.mtx.m31 - dl->clip_bounds[instance].center.y;
                f32 dz = global_camera.mtx.m32 - dl->clip_bounds[instance].center.z;
                f32 distance = dx * dx + dy * dy + dz * dz;
                while (dl->lod_ranges[lod] > distance)
                    ++lod;
            }
            NUCLIPOBJECT *object = &dl->clip_objects[lod];
            for (i32 i = 0; i < render_set_capacity; ++i) {
                RenderSet &set = render_sets[i];
                NUPORTALBOX &box = scene->portal_boxes[instance];
                f32 sx = set.capsule_end.x - set.capsule_center.x;
                f32 sy = set.capsule_end.y - set.capsule_center.y;
                f32 sz = set.capsule_end.z - set.capsule_center.z;
                f32 length = sx * sx + sy * sy + sz * sz;
                f32 radius = set.capsule_radius + box.first_w;
                f32 ox = box.first.x - set.capsule_center.x;
                f32 oy = box.first.y - set.capsule_center.y;
                f32 oz = box.first.z - set.capsule_center.z;
                f32 dot = sx * ox + sy * oy + sz * oz;
                f32 projection = (1.0f / length) * dot;
                f32 positive = (0.0f < projection ? 1.0f : 0.0f) * projection;
                f32 t = (positive <= 1.0f ? 1.0f : 0.0f) * positive + (1.0f < positive ? 1.0f : 0.0f);
                f32 distance = (ox * ox + oy * oy + oz * oz) + (length * t) * t - (dot + dot) * t;
                if (distance > radius * radius)
                    continue;
                VuVec minimum, maximum;
                minimum.x = box.first.x - box.second.x;
                minimum.y = box.first.y - box.second.y;
                minimum.z = box.first.z - box.second.z;
                maximum.x = box.first.x + box.second.x;
                maximum.y = box.first.y + box.second.y;
                maximum.z = box.first.z + box.second.z;
                if (!testShadowExtrusion(minimum, maximum, i))
                    continue;
                for (i32 j = 0; j < object->nmaterials; ++j) {
                    NUMTL *material = scene->mtls[object->material_ids[j]];
                    i32 channel = material->shader_desc.unknown_1b4 != 0;
                    if ((usize)set.scene_cursor[channel] >= (usize)set.scene_end[channel]) {
                        NUDISPLAYLISTITEM *cursor =
                            (NUDISPLAYLISTITEM *)((display_list_buffer->addr + 15) & ~(usize)15);
                        set.scene_cursor[channel] = cursor;
                        display_list_buffer->addr = (usize)(cursor + 49);
                        if (set.scene_end[channel]) {
                            set.scene_end[channel]->next = cursor;
                            set.scene_end[channel]->type = 0x8d;
                            set.scene_end[channel]->id = 1;
                        } else
                            set.scene_first[channel] = cursor;
                        set.scene_end[channel] = cursor + 48;
                        material = scene->mtls[object->material_ids[j]];
                    }
                    set.scene_cursor[channel][0].id = 3;
                    set.scene_cursor[channel][0].type = 0x80;
                    set.scene_cursor[channel][0].next = material;
                    set.scene_cursor[channel][1] = dl->items[object->indices[j] - 1];
                    set.scene_cursor[channel][2] = dl->items[object->indices[j]];
                    set.scene_cursor[channel][1].id = 3;
                    set.scene_cursor[channel][2].id = 3;
                    set.scene_cursor[channel] += 3;
                }
            }
        }
        while (dl->lod_ranges[lod] != 0.0f)
            ++lod;
    }
    DisplaySceneRndrSpecials(dl, 0, NULL);
    for (i32 i = 0; i < render_set_capacity; ++i) {
        RenderSet &set = render_sets[i];
        if (set.scene_first[0]) {
            NUDISPLAYLIST *list = &set.display_lists[0];
            NuDisplayListLinkItems(list, 1);
            list->items->type = 0x80;
            list->items->id = 3;
            list->items->next = scene->mtls[0];
            ++list->items;
            NuDisplayListLinkList(list, set.scene_first[0], set.scene_cursor[0]);
        }
        if (set.scene_first[1]) {
            NUDISPLAYLIST *list = &set.display_lists[1];
            NuDisplayListLinkItems(list, 1);
            list->items->type = 0x80;
            list->items->id = 3;
            list->items->next = scene->mtls[0];
            ++list->items;
            NuDisplayListLinkList(list, set.scene_first[1], set.scene_cursor[1]);
        }
    }
}

static inline void PostBlurSetVertexParam(nushaderprogram_s *, u32, const f32 *, i32);
void NuDynamicLight::bindShaderResources(nushaderprogram_s *program) {
    NUMTX inverse;
    NuMtxInv(&inverse, &view);
    NUVEC light;
    if (parameter_4 == 0) {
        light.x = 0.0f;
        light.y = 0.0f;
        light.z = -1.0f;
        NuVecMtxRotate(&light, &light, &inverse);
        NuVecMtxRotate(&light, &light, reinterpret_cast<NUMTX *>(g_renderContext_view));
        NuVecNorm(&light, &light);
    } else {
        light.x = inverse.m30;
        light.y = inverse.m31;
        light.z = inverse.m32;
        NuVecMtxTransformH(&light, &light, reinterpret_cast<NUMTX *>(g_renderContext_view));
    }
    PostBlurSetVertexParam(program, 0x8085, &light.x, 4);
    NUVEC4 parameters;
    parameters.x = reserved_7c0[0];
    parameters.y = reserved_7c0[1] > 0.0f ? 1.0f / reserved_7c0[1] : 0.0f;
    parameters.z = 1.0f - reserved_7c8.x;
    parameters.w = reserved_7c8.y;
    PostBlurSetVertexParam(program, 0x8086, &parameters.x, 4);
    NUVEC4 split = {active_render_set_count == 1 ? 1000.0f : split_distances[1], 0, 0, 0};
    PostBlurSetVertexParam(program, 0x8088, &split.x, 4);
    NUMTX bias __attribute__((aligned(16))) = {0.5f, 0, 0, 0, 0, -0.5f, 0, 0, 0, 0, 0.5f, 0, 0.5f, 0.5f, 0.5f, 1};
    i32 semantic = 0x8e;
    for (i32 i = 0; i < active_render_set_count; ++i, semantic += 7) {
        RenderSet &set = render_sets[i];
        NUMTX shadow;
        NuMtxTranspose(&shadow, &set.shadow_transform);
        PostBlurSetVertexParam(program, (semantic - 5) | 0x8000, &shadow.m00, 16);
        NUMTX transform;
        NuMtxMulH(&transform, &inverse, &set.warp);
        NuMtxMulH(&transform, &transform, &bias);
        NuMtxInvH(&transform, &transform);
        NUVEC4 row_z = {transform.m02, transform.m12, transform.m22, transform.m32};
        NUVEC4 row_w = {transform.m03, transform.m13, transform.m23, transform.m33};
        PostBlurSetVertexParam(program, (semantic - 1) | 0x8000, &row_z.x, 4);
        PostBlurSetVertexParam(program, semantic | 0x8000, &row_w.x, 4);
        NUVEC4 shadow_parameters;
        shadow_parameters.x = set.parameter_100;
        shadow_parameters.y = 1.0f - set.parameter_104;
        shadow_parameters.z = reserved_7c8.z;
        shadow_parameters.w = reserved_7c8.w - reserved_7c8.z;
        PostBlurSetVertexParam(program, (semantic + 1) | 0x8000, &shadow_parameters.x, 4);
    }
}

static inline void CloneLightVector(NUVEC4 &destination, const NUVEC4 &source) {
    destination.x = source.x;
    destination.y = source.y;
    destination.z = source.z;
    destination.w = source.w;
}
NuDynamicLight *NuDynamicLight::clone(variptr_u *arena, variptr_u) {
    NuDynamicLight *copy = reinterpret_cast<NuDynamicLight *>((arena->addr + 31) & ~usize(31));
    arena->addr = reinterpret_cast<usize>(copy) + ((sizeof(NuDynamicLight) + 31) & ~usize(31));
    RenderSet *destination_ptr = copy->render_sets;
    const RenderSet *source_ptr = render_sets;
    do {
        RenderSet &destination = *destination_ptr;
        const RenderSet &source = *source_ptr;
        destination.view = source.view;
        destination.projection = source.projection;
        destination.warp = source.warp;
        destination.shadow_transform = source.shadow_transform;
        memcpy(destination.reserved_100, source.reserved_100, sizeof(source.reserved_100));
        destination.warp_factor = source.warp_factor;
        CloneLightVector(destination.corners[0], source.corners[0]);
        CloneLightVector(destination.corners[1], source.corners[1]);
        CloneLightVector(destination.corners[2], source.corners[2]);
        CloneLightVector(destination.corners[3], source.corners[3]);
        CloneLightVector(destination.corners[4], source.corners[4]);
        CloneLightVector(destination.corners[5], source.corners[5]);
        CloneLightVector(destination.corners[6], source.corners[6]);
        CloneLightVector(destination.corners[7], source.corners[7]);
        CloneLightVector(destination.shadow_planes[0], source.shadow_planes[0]);
        CloneLightVector(destination.shadow_planes[1], source.shadow_planes[1]);
        CloneLightVector(destination.shadow_planes[2], source.shadow_planes[2]);
        CloneLightVector(destination.shadow_planes[3], source.shadow_planes[3]);
        CloneLightVector(destination.shadow_planes[4], source.shadow_planes[4]);
        CloneLightVector(destination.shadow_planes[5], source.shadow_planes[5]);
        CloneLightVector(destination.shadow_planes[6], source.shadow_planes[6]);
        CloneLightVector(destination.shadow_planes[7], source.shadow_planes[7]);
        CloneLightVector(destination.shadow_planes[8], source.shadow_planes[8]);
        CloneLightVector(destination.shadow_planes[9], source.shadow_planes[9]);
        CloneLightVector(destination.shadow_planes[10], source.shadow_planes[10]);
        CloneLightVector(destination.shadow_planes[11], source.shadow_planes[11]);
        destination.shadow_plane_count = source.shadow_plane_count;
        CloneLightVector(destination.far_plane, source.far_plane);
        CloneLightVector(destination.capsule_center, source.capsule_center);
        CloneLightVector(destination.capsule_end, source.capsule_end);
        destination.capsule_radius = source.capsule_radius;
        memcpy(destination.display_lists, source.display_lists, sizeof(source.display_lists));
        {
            nurndrstate_s *first = source.render_states[0];
            nurndrstate_s *second = source.render_states[1];
            destination.render_states[1] = second;
            destination.render_states[0] = first;
        }
        {
            NUDISPLAYLISTITEM *first = source.list_items[0];
            NUDISPLAYLISTITEM *second = source.list_items[1];
            destination.list_items[1] = second;
            destination.list_items[0] = first;
        }
        {
            NUDISPLAYLISTITEM *first = source.scene_cursor[0];
            NUDISPLAYLISTITEM *second = source.scene_cursor[1];
            destination.scene_cursor[1] = second;
            destination.scene_cursor[0] = first;
        }
        {
            NUDISPLAYLISTITEM *first = source.scene_first[0];
            NUDISPLAYLISTITEM *second = source.scene_first[1];
            destination.scene_first[1] = second;
            destination.scene_first[0] = first;
        }
        {
            NUDISPLAYLISTITEM *first = source.scene_end[0];
            NUDISPLAYLISTITEM *second = source.scene_end[1];
            destination.scene_end[1] = second;
            destination.scene_end[0] = first;
        }
        destination.reserved_33c[0] = source.reserved_33c[0];
        destination.reserved_33c[1] = source.reserved_33c[1];
        destination.reserved_33c[2] = source.reserved_33c[2];
        destination.reserved_33c[3] = source.reserved_33c[3];
        destination.reserved_33c[4] = source.reserved_33c[4];
        destination.reserved_33c[5] = source.reserved_33c[5];
        destination.reserved_33c[6] = source.reserved_33c[6];
        destination.reserved_33c[7] = source.reserved_33c[7];
        destination.geometry_count = source.geometry_count;
        ++source_ptr;
        ++destination_ptr;
    } while (destination_ptr != copy->render_sets + 2);
    copy->reserved_6c0[0] = reserved_6c0[0];
    copy->reserved_6c0[1] = reserved_6c0[1];
    copy->reserved_6c0[2] = reserved_6c0[2];
    copy->render_set_capacity = render_set_capacity;
    copy->active_render_set_count = active_render_set_count;
    copy->reserved_6d4[0] = reserved_6d4[0];
    copy->reserved_6d4[1] = reserved_6d4[1];
    copy->reserved_6d4[2] = reserved_6d4[2];
    copy->reserved_6d4[3] = reserved_6d4[3];
    copy->reserved_6d4[4] = reserved_6d4[4];
    copy->reserved_6d4[5] = reserved_6d4[5];
    copy->reserved_6d4[6] = reserved_6d4[6];
    copy->reserved_6d4[7] = reserved_6d4[7];
    copy->reserved_6d4[8] = reserved_6d4[8];
    copy->reserved_6d4[9] = reserved_6d4[9];
    copy->reserved_6d4[10] = reserved_6d4[10];
    copy->reserved_6d4[11] = reserved_6d4[11];
    copy->reserved_6d4[12] = reserved_6d4[12];
    copy->reserved_6d4[13] = reserved_6d4[13];
    copy->reserved_6d4[14] = reserved_6d4[14];
    copy->reserved_6d4[15] = reserved_6d4[15];
    copy->reserved_6d4[16] = reserved_6d4[16];
    copy->reserved_6d4[17] = reserved_6d4[17];
    copy->reserved_6d4[18] = reserved_6d4[18];
    copy->reserved_6d4[19] = reserved_6d4[19];
    copy->direction.x = direction.x;
    copy->direction.y = direction.y;
    copy->direction.z = direction.z;
    copy->reserved_730 = reserved_730;
    copy->view = view;
    copy->projection = projection;
    copy->parameter_4 = parameter_4;
    copy->parameter_5 = parameter_5;
    copy->reserved_7bc = reserved_7bc;
    copy->reserved_7c0[0] = reserved_7c0[0];
    copy->reserved_7c0[1] = reserved_7c0[1];
    CloneLightVector(copy->reserved_7c8, reserved_7c8);
    copy->used_on_specials = used_on_specials;
    for (i32 i = 0; i < render_set_capacity; ++i) {
        RenderSet &set = render_sets[i];
        set.geometry_count = 0;
        NuDisplayListReset(&set.display_lists[0]);
        NUDISPLAYLISTITEM *item0 = set.display_lists[0].first;
        item0->type = 0x8d;
        item0->next = NULL;
        item0->id = 1;
        NuDisplayListReset(&set.display_lists[1]);
        NUDISPLAYLISTITEM *item1 = set.display_lists[1].first;
        item1->type = 0x8d;
        item1->id = 1;
        item1->next = NULL;
    }
    return copy;
}

static inline void LightBoundsPoint(const VuVec &point, NUVEC &minimum, NUVEC &maximum) {
    if (point.x < minimum.x)
        minimum.x = point.x;
    else if (point.x > maximum.x)
        maximum.x = point.x;
    if (point.y < minimum.y)
        minimum.y = point.y;
    else if (point.y > maximum.y)
        maximum.y = point.y;
    if (point.z < minimum.z)
        minimum.z = point.z;
    else if (point.z > maximum.z)
        maximum.z = point.z;
}
void NuDynamicLight::computeBoundingSpace(const VuVec *points, VuMtx *matrix) {
    NUVEC minimum = {FLT_MAX, FLT_MAX, FLT_MAX};
    NUVEC maximum = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    LightBoundsPoint(points[0], minimum, maximum);
    LightBoundsPoint(points[1], minimum, maximum);
    LightBoundsPoint(points[2], minimum, maximum);
    LightBoundsPoint(points[3], minimum, maximum);
    LightBoundsPoint(points[4], minimum, maximum);
    LightBoundsPoint(points[5], minimum, maximum);
    LightBoundsPoint(points[6], minimum, maximum);
    LightBoundsPoint(points[7], minimum, maximum);
    NuMtxSetOrthoBlend(&matrix->matrix, minimum.x, maximum.x, minimum.y, maximum.y, minimum.z, maximum.z);
}

static inline void LightClipTransform(VuVec &result, const VuVec &point, const NUMTX &matrix) {
    result.x = point.x * matrix.m00 + point.y * matrix.m10 + point.z * matrix.m20 + matrix.m30;
    result.y = point.x * matrix.m01 + point.y * matrix.m11 + point.z * matrix.m21 + matrix.m31;
    result.z = point.x * matrix.m02 + point.y * matrix.m12 + point.z * matrix.m22 + matrix.m32;
    result.w = point.x * matrix.m03 + point.y * matrix.m13 + point.z * matrix.m23 + matrix.m33;
    f32 reciprocal = 1.0f / result.w;
    result.x *= reciprocal;
    result.y *= reciprocal;
    result.z *= reciprocal;
}
static inline void LightClipPlane(VuVec &plane, const VuVec &origin, const VuVec &a, const VuVec &b) {
    NUVEC first = {a.x - origin.x, a.y - origin.y, a.z - origin.z};
    NUVEC second = {b.x - origin.x, b.y - origin.y, b.z - origin.z};
    plane.x = first.y * second.z - first.z * second.y;
    plane.y = first.z * second.x - first.x * second.z;
    plane.z = first.x * second.y - first.y * second.x;
    NuVecNorm((NUVEC *)&plane, (NUVEC *)&plane);
    plane.w = -(plane.x * origin.x + plane.y * origin.y + plane.z * origin.z);
}
void NuDynamicLight::computeClippingPlanes(const VuMtx &matrix, bool zero_near, VuVec &left, VuVec &right,
                                           VuVec &bottom, VuVec &top, VuVec &near_plane, VuVec &far_plane) {
    const VuVec symmetric[8] = {{-1, -1, -1, 1}, {-1, 1, -1, 1}, {1, 1, -1, 1}, {1, -1, -1, 1},
                                {-1, -1, 1, 1},  {-1, 1, 1, 1},  {1, 1, 1, 1},  {1, -1, 1, 1}};
    const VuVec positive[8] = {{-1, -1, 0, 1}, {-1, 1, 0, 1}, {1, 1, 0, 1}, {1, -1, 0, 1},
                               {-1, -1, 1, 1}, {-1, 1, 1, 1}, {1, 1, 1, 1}, {1, -1, 1, 1}};
    const VuVec *source = zero_near ? positive : symmetric;
    VuVec corners[8];
    LightClipTransform(corners[0], source[0], matrix.matrix);
    LightClipTransform(corners[1], source[1], matrix.matrix);
    LightClipTransform(corners[2], source[2], matrix.matrix);
    LightClipTransform(corners[3], source[3], matrix.matrix);
    LightClipTransform(corners[4], source[4], matrix.matrix);
    LightClipTransform(corners[5], source[5], matrix.matrix);
    LightClipTransform(corners[6], source[6], matrix.matrix);
    LightClipTransform(corners[7], source[7], matrix.matrix);
    LightClipPlane(left, corners[0], corners[1], corners[4]);
    LightClipPlane(right, corners[6], corners[2], corners[7]);
    LightClipPlane(top, corners[6], corners[5], corners[2]);
    LightClipPlane(bottom, corners[0], corners[4], corners[3]);
    LightClipPlane(far_plane, corners[6], corners[7], corners[5]);
    LightClipPlane(near_plane, corners[0], corners[3], corners[1]);
}

void NuDynamicLight::computeFrustumCube(nucamera_s const *camera, VuVec *corners, VuVec *far_plane) {
    i32 angle = static_cast<i32>((camera->fov * 0.5f) * 10430.3779296875f);
    f32 near_z = camera->near_clip;
    f32 near_y = near_z * NU_SIN_LUT(angle) / NU_COS_LUT(angle);
    f32 near_x = near_y / camera->aspect;
    f32 ratio = camera->far_clip / near_z;
    corners[0].w = corners[1].w = corners[2].w = corners[3].w = 1.0f;
    corners[4].w = corners[5].w = corners[6].w = corners[7].w = 0.0f;
    corners[0].x = -near_x;
    corners[0].y = -near_y;
    corners[0].z = near_z;
    corners[1].x = -near_x;
    corners[1].y = near_y;
    corners[1].z = near_z;
    corners[2].x = near_x;
    corners[2].y = near_y;
    corners[2].z = near_z;
    corners[3].x = near_x;
    corners[3].y = -near_y;
    corners[3].z = near_z;
    corners[4].x = corners[0].x * ratio;
    corners[4].y = corners[0].y * ratio;
    corners[4].z = corners[0].z * ratio;
    corners[5].x = corners[1].x * ratio;
    corners[5].y = corners[1].y * ratio;
    corners[5].z = corners[1].z * ratio;
    corners[6].x = corners[2].x * ratio;
    corners[6].y = corners[2].y * ratio;
    corners[6].z = corners[2].z * ratio;
    corners[7].x = corners[3].x * ratio;
    corners[7].y = corners[3].y * ratio;
    corners[7].z = corners[3].z * ratio;
    NUMTX matrix;
    memcpy(&matrix, &camera->mtx, sizeof(matrix));
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[0]), reinterpret_cast<NUVEC *>(&corners[0]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[1]), reinterpret_cast<NUVEC *>(&corners[1]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[2]), reinterpret_cast<NUVEC *>(&corners[2]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[3]), reinterpret_cast<NUVEC *>(&corners[3]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[4]), reinterpret_cast<NUVEC *>(&corners[4]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[5]), reinterpret_cast<NUVEC *>(&corners[5]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[6]), reinterpret_cast<NUVEC *>(&corners[6]), &matrix);
    NuVecMtxTransform(reinterpret_cast<NUVEC *>(&corners[7]), reinterpret_cast<NUVEC *>(&corners[7]), &matrix);
    NUVEC edge1 = {corners[5].x - corners[4].x, corners[5].y - corners[4].y, corners[5].z - corners[4].z};
    NUVEC edge2 = {corners[6].x - corners[4].x, corners[6].y - corners[4].y, corners[6].z - corners[4].z};
    far_plane->x = edge1.y * edge2.z - edge1.z * edge2.y;
    far_plane->y = edge1.z * edge2.x - edge1.x * edge2.z;
    far_plane->z = edge1.x * edge2.y - edge1.y * edge2.x;
    NuVecNorm(reinterpret_cast<NUVEC *>(far_plane), reinterpret_cast<NUVEC *>(far_plane));
    far_plane->w = -(far_plane->x * corners[4].x + far_plane->y * corners[4].y + far_plane->z * corners[4].z);
}

void NuDynamicLight::computeLightSpace(NUVEC *direction, NUVEC *camera, NUMTX *view, NUMTX *inverse) {
    NuMtxSetIdentity(inverse);
    NUVEC *up = reinterpret_cast<NUVEC *>(&inverse->m10);
    up->z = -direction->z;
    up->y = -direction->y;
    up->x = -direction->x;
    NUVEC cross;
    cross.z = up->y * camera->x - up->x * camera->y;
    cross.y = up->x * camera->z - up->z * camera->x;
    cross.x = up->z * camera->y - up->y * camera->z;
    NUVEC forward;
    forward.z = up->x * cross.y - up->y * cross.x;
    forward.y = up->z * cross.x - up->x * cross.z;
    forward.x = up->y * cross.z - up->z * cross.y;
    f32 length = NuFsqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
    forward.z /= length;
    forward.y /= length;
    forward.x /= length;
    NUVEC *axis = reinterpret_cast<NUVEC *>(&inverse->m20);
    axis->z = forward.z;
    axis->y = forward.y;
    axis->x = forward.x;
    NUVEC *right = reinterpret_cast<NUVEC *>(&inverse->m00);
    right->x = up->y * axis->z - up->z * axis->y;
    right->y = up->z * axis->x - up->x * axis->z;
    right->z = up->x * axis->y - up->y * axis->x;
    NuMtxInv(view, inverse);
}

static inline NUVEC ShadowCross(const VuVec &a, const VuVec &b, const VuVec &origin) {
    NUVEC first = {b.x - origin.x, b.y - origin.y, b.z - origin.z};
    NUVEC second = {a.x - origin.x, a.y - origin.y, a.z - origin.z};
    NUVEC normal;
    normal.y = first.x * second.z - first.z * second.x;
    normal.z = second.x * first.y - first.x * second.y;
    normal.x = second.y * first.z - second.z * first.y;
    return normal;
}
static inline void ShadowFace(const VuVec &origin, const VuVec &a, const VuVec &b, const NUVEC &extrusion,
                              VuVec *planes, i32 &count, bool &front) {
    NUVEC normal = ShadowCross(a, b, origin);
    front = extrusion.x * normal.x + extrusion.y * normal.y + extrusion.z * normal.z >= 0.0f;
    if (!front) {
        VuVec &plane = planes[count++];
        plane.x = -normal.x;
        plane.y = -normal.y;
        plane.z = -normal.z;
        plane.w = 0.0f;
        NuVecNorm((NUVEC *)&plane, (NUVEC *)&plane);
        plane.w = -(plane.x * origin.x + plane.y * origin.y + plane.z * origin.z);
    }
}
i32 NuDynamicLight::computeShadowClippingPlanes(const VuVec &direction, const VuVec *corners, VuVec *planes) {
    NUVEC extrusion = {direction.x * -120.0f, direction.y * -120.0f, direction.z * -120.0f};
    i32 count = 0;
    bool front[6];
    ShadowFace(corners[0], corners[1], corners[2], extrusion, planes, count, front[0]);
    ShadowFace(corners[3], corners[2], corners[6], extrusion, planes, count, front[1]);
    ShadowFace(corners[7], corners[6], corners[5], extrusion, planes, count, front[2]);
    ShadowFace(corners[4], corners[5], corners[1], extrusion, planes, count, front[3]);
    ShadowFace(corners[1], corners[5], corners[6], extrusion, planes, count, front[4]);
    ShadowFace(corners[4], corners[0], corners[3], extrusion, planes, count, front[5]);
    static const i32 edge_faces[12][2] = {{3, 0}, {4, 0}, {1, 0}, {5, 0}, {2, 3}, {2, 4},
                                          {2, 1}, {2, 5}, {5, 3}, {3, 4}, {4, 1}, {1, 5}};
    static const i32 edge_vertices[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                             {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (i32 edge = 0; edge < 12 && count <= 8; ++edge) {
        bool first_front = front[edge_faces[edge][0]];
        if (first_front == front[edge_faces[edge][1]])
            continue;
        const VuVec &first = corners[edge_vertices[edge][0]];
        const VuVec &second = corners[edge_vertices[edge][1]];
        VuVec extruded = {extrusion.x + first.x, extrusion.y + first.y, extrusion.z + first.z, 0.0f};
        VuVec &plane = planes[count++];
        if (first_front) {
            NUVEC normal = ShadowCross(second, extruded, first);
            plane.x = normal.x;
            plane.y = normal.y;
            plane.z = normal.z;
            NuVecNorm((NUVEC *)&plane, (NUVEC *)&plane);
            plane.w = -(plane.x * first.x + plane.y * first.y + plane.z * first.z);
        } else {
            NUVEC normal = ShadowCross(second, first, extruded);
            plane.x = normal.x;
            plane.y = normal.y;
            plane.z = normal.z;
            NuVecNorm((NUVEC *)&plane, (NUVEC *)&plane);
            plane.w = -(extruded.x * plane.x + extruded.y * plane.y + extruded.z * plane.z);
        }
    }
    return count;
}

static inline void CapsuleAdd(VuVec &sum, const VuVec &point) {
    sum.x += point.x;
    sum.y += point.y;
    sum.z += point.z;
}
static inline float CapsuleDistance(const VuVec &center, const VuVec &point, float &radius) {
    float x = center.x - point.x, y = center.y - point.y, z = center.z - point.z;
    float distance = x * x + y * y + z * z;
    radius = distance > radius ? distance : radius;
    return distance;
}
void NuDynamicLight::computeShadowFrustrumCapsule(const VuVec &direction, const VuVec *corners, VuVec &center,
                                                  VuVec &end, float &radius) {
    center.x = corners[0].x;
    center.y = corners[0].y;
    center.z = corners[0].z;
    center.w = corners[0].w;
    CapsuleAdd(center, corners[1]);
    CapsuleAdd(center, corners[2]);
    CapsuleAdd(center, corners[3]);
    CapsuleAdd(center, corners[4]);
    CapsuleAdd(center, corners[5]);
    CapsuleAdd(center, corners[6]);
    CapsuleAdd(center, corners[7]);
    center.x *= 0.125f;
    center.y *= 0.125f;
    center.z *= 0.125f;
    radius = 0.0f;
    CapsuleDistance(center, corners[0], radius);
    CapsuleDistance(center, corners[1], radius);
    CapsuleDistance(center, corners[2], radius);
    CapsuleDistance(center, corners[3], radius);
    CapsuleDistance(center, corners[4], radius);
    CapsuleDistance(center, corners[5], radius);
    CapsuleDistance(center, corners[6], radius);
    float distance = CapsuleDistance(center, corners[7], radius);
    // The original takes the final corner distance here, after the maximum stores.
    radius = NuFsqrt(distance);
    VuVec result = {center.x - direction.x * 100.0f, center.y - direction.y * 100.0f, center.z - direction.z * 100.0f,
                    0.0f};
    end.x = result.x;
    end.y = result.y;
    end.z = result.z;
    end.w = 0.0f;
}

static inline NUVEC WarpTransformPoint(const NUVEC4 &point, const NUMTX &matrix) {
    NUMTX copy;
    copy.m00 = matrix.m00;
    copy.m01 = matrix.m01;
    copy.m02 = matrix.m02;
    copy.m03 = matrix.m03;
    copy.m10 = matrix.m10;
    copy.m11 = matrix.m11;
    copy.m12 = matrix.m12;
    copy.m13 = matrix.m13;
    copy.m20 = matrix.m20;
    copy.m21 = matrix.m21;
    copy.m22 = matrix.m22;
    copy.m23 = matrix.m23;
    copy.m30 = matrix.m30;
    copy.m31 = matrix.m31;
    copy.m32 = matrix.m32;
    copy.m33 = matrix.m33;
    NUVEC output;
    NuVecMtxTransformH(&output, (NUVEC *)&point, &copy);
    return output;
}
static inline void WarpAccumulatePoint(const NUVEC4 &point, const NUMTX &matrix, NUVEC &output, f32 &min_z, f32 &min_y,
                                       f32 &max_z, f32 &max_y) {
    output = WarpTransformPoint(point, matrix);
    min_z = output.z < min_z ? output.z : min_z;
    min_y = output.y < min_y ? output.y : min_y;
    max_z = max_z < output.z ? output.z : max_z;
    max_y = max_y < output.y ? output.y : max_y;
}
static inline void WarpProjectBounds(NUVEC &point, NUVEC &translation, f32 near_plane, f32 &left, f32 &bottom,
                                     f32 &right, f32 &top) {
    NUVEC shifted;
    NuVecAdd(&shifted, &point, &translation);
    f32 x = shifted.x * near_plane / shifted.z;
    f32 y = shifted.y * near_plane / shifted.z;
    if (left > x)
        left = x;
    else if (x > right)
        right = x;
    if (bottom > y)
        bottom = y;
    else if (y > top)
        top = y;
}
void NuDynamicLight::computeWarpEffect(NuDynamicLight::RenderSet &set) {
    NUMTX render_view = set.view;
    NUMTX camera_view = cacheCameraView;
    NUMTX render_view_projection, inverse_view_projection, inverse_render_view, inverse_camera_view;
    NuMtxMulH(&render_view_projection, &render_view, &set.projection);
    NuMtxInvH(&inverse_view_projection, &render_view_projection);
    NuMtxInv(&inverse_render_view, &render_view);
    NuMtxInv(&inverse_camera_view, &camera_view);
    NUVEC light_forward = {inverse_render_view.m20, inverse_render_view.m21, inverse_render_view.m22};
    NUVEC camera_forward = {inverse_camera_view.m20, inverse_camera_view.m21, inverse_camera_view.m22};
    NUMTX light_space, inverse_light_space;
    computeLightSpace(&light_forward, &camera_forward, &light_space, &inverse_light_space);
    NUVEC4 points[10];
    for (i32 i = 0; i < 8; ++i)
        points[i] = set.corners[i];
    points[8].x = direction.x * 200.0f + points[0].x;
    points[8].y = direction.y * 200.0f + points[0].y;
    points[8].z = direction.z * 200.0f + points[0].z;
    points[8].w = 0.0f;
    points[9].x = points[0].x - direction.x * 200.0f;
    points[9].y = points[0].y - direction.y * 200.0f;
    points[9].z = points[0].z - direction.z * 200.0f;
    points[9].w = 0.0f;
    NUVEC transformed[10];
    f32 min_z = FLT_MAX, min_y = FLT_MAX, max_z = -FLT_MAX, max_y = -FLT_MAX;
    WarpAccumulatePoint(points[0], light_space, transformed[0], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[1], light_space, transformed[1], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[2], light_space, transformed[2], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[3], light_space, transformed[3], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[4], light_space, transformed[4], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[5], light_space, transformed[5], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[6], light_space, transformed[6], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[7], light_space, transformed[7], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[8], light_space, transformed[8], min_z, min_y, max_z, max_y);
    WarpAccumulatePoint(points[9], light_space, transformed[9], min_z, min_y, max_z, max_y);
    f32 fovy, aspect, camera_near, camera_far;
    NuMtxGetPerspectiveD3D(&cacheCameraProj, &fovy, &aspect, &camera_near, &camera_far);
    f32 depth = max_z - min_z;
    camera_far = depth;
    f32 weight = 1.0f - fabsf(camera_forward.x * light_forward.x + camera_forward.y * light_forward.y +
                              camera_forward.z * light_forward.z);
    f32 near_plane = (NuFsqrt(depth * camera_near) + camera_near) * set.warp_factor / weight;
    NUVEC camera_position = {inverse_camera_view.m30, inverse_camera_view.m31, inverse_camera_view.m32};
    NUVEC camera_light;
    NuVecMtxTransformH(&camera_light, &camera_position, &light_space);
    NUVEC translation = {-camera_light.x, -((max_y + min_y) * 0.5f), -(min_z - near_plane)};
    f32 left = FLT_MAX, bottom = FLT_MAX, right = -FLT_MAX, top = -FLT_MAX;
    WarpProjectBounds(transformed[0], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[1], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[2], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[3], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[4], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[5], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[6], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[7], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[8], translation, near_plane, left, bottom, right, top);
    WarpProjectBounds(transformed[9], translation, near_plane, left, bottom, right, top);
    NUMTX offset, frustum;
    NuMtxSetTranslation(&offset, &translation);
    NuMtxSetFrustumBlend(&frustum, left, right, bottom, top, near_plane, depth + near_plane);
    NUMTX axes = {1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1};
    NUMTX shifted_light, projected_light, result;
    NuMtxMulH(&shifted_light, &light_space, &offset);
    NuMtxMulH(&projected_light, &shifted_light, &frustum);
    NuMtxMulH(&result, &projected_light, &axes);
    set.warp = result;
}

NuDynamicLight *NuDynamicLight::create() {
    NuDynamicLight *light = dynamicLightPool.allocate();
    new (light) NuDynamicLight;
    return light;
}

void NuDynamicLight::destroy(NuDynamicLight *light) {
    i32 index = (reinterpret_cast<u8 *>(light) - reinterpret_cast<u8 *>(dynamicLightPool.slots)) /
                i32(sizeof(dynamicLightPool.slots[0]));
    dynamicLightPool.next = index;
    dynamicLightPool.occupied &= ~(1 << (index & 7));
}

void NuDynamicLight::refreshShadowTransform(RenderSet &set) {
    NUMTX inverse_camera;
    NuMtxInvH(&inverse_camera, &cacheCameraView);
    computeWarpEffect(set);
    NUMTX bias = {0.5f, 0, 0, 0, 0, -0.5f, 0, 0, 0, 0, 0.5f, 0, 0.5f, 0.5f, 0.5f, 1};
    NuMtxMulH(&set.shadow_transform, &inverse_camera, &set.warp);
    NuMtxMulH(&set.shadow_transform, &set.shadow_transform, &bias);
}

void NuDynamicLight::renderShadowMap(i32 index, nuframebuffer_s *) {
    RenderSet &set = render_sets[index];
    if (set.display_lists[0].mtl_last == set.display_lists[0].first &&
        set.display_lists[1].mtl_last == set.display_lists[1].first && set.geometry_count == 0)
        return;
    refreshShadowTransform(set);
    NuDisplayListSetItemTable(1);
    NUMTX bias = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0.5f, 0, 0, 0, 0.5f, 1};
    NUMTX matrix;
    NuMtxMulH(&matrix, &set.warp, &bias);
    NuRenderContextSetViewProj(&matrix, &numtx_identity);
    if (set.display_lists[0].mtl_last != set.display_lists[0].first) {
        NUDISPLAYLISTITEM *item = set.display_lists[0].mtl_last;
        item->type = 0x84;
        item->id = 4;
        item->next = NULL;
        if (g_renderContext_zFunc != 0) {
            glEnable(0xb71);
            glDepthMask(1);
            glDepthFunc(0x203);
        }
        g_renderContext_zFunc = 0;
        memcpy(g_renderContext_kTint, nuvec4_one, sizeof(g_renderContext_kTint));
        NuShaderManagerSetfv(0x44, nuvec4_one);
        NuDisplayListDrawItems(set.display_lists[0].first);
        set.display_lists[0].mtl_last = set.display_lists[0].first;
    }
    if (set.display_lists[1].mtl_last != set.display_lists[1].first) {
        NUDISPLAYLISTITEM *item = set.display_lists[1].mtl_last;
        item->type = 0x84;
        item->id = 4;
        item->next = NULL;
        if (g_renderContext_zFunc != 0) {
            glEnable(0xb71);
            glDepthMask(1);
            glDepthFunc(0x203);
        }
        g_renderContext_zFunc = 0;
        memcpy(g_renderContext_kTint, nuvec4_one, sizeof(g_renderContext_kTint));
        NuShaderManagerSetfv(0x44, nuvec4_one);
        NuDisplayListDrawItems(set.display_lists[1].first);
        set.display_lists[1].mtl_last = set.display_lists[1].first;
    }
    for (i32 i = 0; i < set.geometry_count; ++i)
        NuDisplayListDrawRenderScene(set.reserved_33c[i]);
    NuDisplayListSetItemTable(0);
}

void NuDynamicLight::resetGeometry() {
    for (i32 i = 0; i < render_set_capacity; ++i) {
        RenderSet &set = render_sets[i];
        set.geometry_count = 0;
        NuDisplayListReset(&set.display_lists[0]);
        NUDISPLAYLISTITEM *item0 = set.display_lists[0].first;
        item0->type = 0x8d;
        item0->next = NULL;
        item0->id = 1;
        NuDisplayListReset(&set.display_lists[1]);
        NUDISPLAYLISTITEM *item1 = set.display_lists[1].first;
        item1->type = 0x8d;
        item1->id = 1;
        item1->next = NULL;
    }
}

void NuDynamicLight::setCameraViewProj(NUMTX *view, NUMTX *projection) {
    cacheCameraView = *view;
    cacheCameraProj = *projection;
}

static inline void SetLightCameraVector(NUVEC4 &destination, f32 x, f32 y, f32 z, f32 w) {
    destination.x = x;
    destination.y = y;
    destination.z = z;
    destination.w = w;
}
void NuDynamicLight::setupCustomCameraFrustum(NUCAMERA *camera, const f32 *splits, i32 split_count) {
    NuMtxSetPerspectiveD3D(&cacheCameraProj, (180.0f * camera->fov) / 3.1415927410125732f, 1.0f / camera->aspect,
                           camera->near_clip, camera->far_clip);
    NuMtxInv(&cacheCameraView, &camera->mtx);
    SetLightCameraVector(camera_position, camera->mtx.m30, camera->mtx.m31, camera->mtx.m32, 1.0f);
    SetLightCameraVector(camera_forward, camera->mtx.m20, camera->mtx.m21, camera->mtx.m22, 0.0f);
    i32 interval_count = split_count - 1;
    active_render_set_count = interval_count;
    for (i32 i = 0; i < split_count; ++i) {
        split_distances[i] = splits[i];
    }
    f32 near_clip = camera->near_clip;
    f32 far_clip = camera->far_clip;
    for (i32 i = 0; i < interval_count; ++i) {
        RenderSet &set = render_sets[i];
        camera->near_clip = splits[i];
        camera->far_clip = splits[i + 1];
        computeFrustumCube(camera, reinterpret_cast<VuVec *>(set.corners), reinterpret_cast<VuVec *>(&set.far_plane));
        set.shadow_plane_count = computeShadowClippingPlanes(reinterpret_cast<const VuVec &>(direction),
                                                             reinterpret_cast<const VuVec *>(set.corners),
                                                             reinterpret_cast<VuVec *>(set.shadow_planes));
        set.view = view;
        set.projection = projection;
        computeShadowFrustrumCapsule(reinterpret_cast<const VuVec &>(direction),
                                     reinterpret_cast<const VuVec *>(set.corners),
                                     reinterpret_cast<VuVec &>(set.capsule_center),
                                     reinterpret_cast<VuVec &>(set.capsule_end), set.capsule_radius);
    }
    for (i32 i = 0; i < interval_count; ++i) {
        split_planes[i].x = 0.0f;
        split_planes[i].y = 0.0f;
        split_planes[i].z = 1.0f / splits[i + 1];
        split_planes[i].w = -1.0f;
    }
    camera->near_clip = near_clip;
    camera->far_clip = far_clip;
}

bool NuDynamicLight::testShadowExtrusion(const VuVec &minimum, const VuVec &maximum, i32 set_index) {
    const RenderSet &set = render_sets[set_index];
    const f32 min_x = minimum.x, min_y = minimum.y, min_z = minimum.z;
    const f32 max_y = maximum.y, max_x = maximum.x, max_z = maximum.z;
    const i32 count = set.shadow_plane_count;
    for (i32 i = 0; i < count; ++i) {
        const NUVEC4 &plane = set.shadow_planes[i];
        f32 x0 = min_x * plane.x;
        f32 y0 = min_y * plane.y;
        f32 z0 = min_z * plane.z;
        f32 xy00 = x0 + y0;
        if ((xy00 + z0) + plane.w > 0.0f)
            continue;
        f32 y1 = max_y * plane.y;
        f32 xy01 = x0 + y1;
        if ((xy01 + z0) + plane.w > 0.0f)
            continue;
        f32 x1 = max_x * plane.x;
        f32 xy11 = y1 + x1;
        if ((z0 + xy11) + plane.w > 0.0f)
            continue;
        f32 xy10 = y0 + x1;
        if ((z0 + xy10) + plane.w > 0.0f)
            continue;
        f32 z1 = max_z * plane.z;
        if ((xy00 + z1) + plane.w > 0.0f)
            continue;
        if ((xy01 + z1) + plane.w > 0.0f)
            continue;
        if ((xy11 + z1) + plane.w > 0.0f)
            continue;
        if ((xy10 + z1) + plane.w > 0.0f)
            continue;
        return false;
    }
    return true;
}

i32 NuDynamicLight::testShadowExtrusions(const VuVec &minimum, const VuVec &maximum) {
    i32 result = 0;
    for (i32 i = 0; i < active_render_set_count; ++i) {
        if (testShadowExtrusion(minimum, maximum, i))
            result |= 1 << i;
    }
    return result;
}

void NuMotionFilter::initResources() {
}

NuMainFilterGen::NuMainFilterGen() {
    dof_strength = dof_near = dof_far = 1.0f;
    dof_mode = 3;
    dof_bias = 0.0f;
    bloom = NULL;
    dof_blur = 3.0f;
    blur_radius = 5.0f;
    blur_gain = 2.1f;
    downsample_lod = 0;
    motion_scale = motion_maximum = 0.0f;
    motion_falloff = 1.0f;
}

void NuMainFilterGen::destroyResources() {
    NuFramebufferDestroy(blur_fbo);
    blur_fbo = NULL;
    NuPostFilterGen::destroyResources();
}

void NuMainFilterGen::destroyTextureResources() {
}

void NuMainFilterGen::initResources() {
    NuPostFilterGen::initResources();
    blur_fbo = NuFramebufferCreate();
    dof_enabled = bloom_enabled = motion_blur_enabled = false;
    active_filter_count = 0;
}

void NuMainFilterGen::initTextureResources(i32 width, i32 height) {
    blur_texture = NuEffectTexCreate2D(width / 2, height / 2, 2, 1, 2);
    downsample_lod = 0;
    i32 w = width, h = height;
    while (w >= 128 && h >= 128 && downsample_lod < 3) {
        w >>= 1;
        h >>= 1;
        ++downsample_lod;
    }
    downsample_texture = NuEffectTexCreate2D(w, h, 1, 1, 2);
    if (height < 704) {
        dof_blur -= 1.0f;
        blur_radius -= 0.85f;
        blur_gain -= 0.5f;
    }
}

void NuMainFilterGen::preprocessBlurTextures(nueffecttex_s *color, nueffecttex_s *normal) {
    if (!dof_enabled && !bloom_enabled)
        return;
    copy(color, 1, color, 0, normal == NULL ? copyTexProgram : programs[16], normal);
    i32 levels = downsample_lod;
    if (dof_enabled) {
        i32 size = color->width < color->height ? color->width : color->height;
        levels = static_cast<i32>(NuLog2(static_cast<f32>(size))) - 6;
    }
    blur7x7Loopback(color, 1, color, 2, 1, levels, true, 1.0f, blur7x7Program);
    if (bloom_enabled) {
        f32 threshold[4] = {bloom->threshold < 0 ? 0 : (bloom->threshold > 1 ? 1 : bloom->threshold), 0, 0, 0};
        const f32 scale_bias[4] = {1, 1, 0, 0};
        PostBindProgram(programs[17]);
        NuShaderProgramSetVertexParamfv(programs[17], 0xae, threshold, 4);
        NuShaderProgramSetVertexParamfv(programs[17], 0xaf, scale_bias, 4);
        copy(downsample_texture, 0, color, downsample_lod, programs[17], NULL);
        i32 passes = static_cast<i32>(bloom->blur_iterations);
        if (passes > 0)
            blur7x7Loopback(downsample_texture, 0, downsample_texture, 0, passes, 1, true, 1.0f, blur7x7Program);
        f32 remainder = bloom->blur_iterations - passes;
        if (remainder > 0)
            blur7x7Loopback(downsample_texture, 0, downsample_texture, 0, 1, 1, true, remainder, blur7x7Program);
    }
}

void NuMainFilterGen::preprocessDofMotionBlur(nueffecttex_s *) {
    i32 selection = 1;
    if (dof_enabled) {
        u32 bias_bits;
        memcpy(&bias_bits, &dof_bias, sizeof(bias_bits));
        selection = bias_bits == 1 ? (motion_blur_enabled ? 4 : 3) : (motion_blur_enabled ? 2 : 0);
    }
    nushaderprogram_s *program = programs[selection];
    PostBindProgram(program);
    if (dof_enabled) {
        f32 fov, aspect, near_z, far_z;
        NuMtxGetPerspectiveD3D(reinterpret_cast<NUMTX *>(g_renderContext_projection), &fov, &aspect, &near_z, &far_z);
        f32 projection_scale = far_z / (far_z - near_z);
        f32 strength_near = dof_strength * dof_near;
        f32 product = strength_near * dof_far;
        f32 span = dof_far - dof_near;
        f32 denominator = near_z * projection_scale * span;
        f32 values[4] = {product / denominator, strength_near / span - (projection_scale * product) / denominator,
                         dof_blur, 0};
        NuShaderProgramSetVertexParamfv(program, 0x83, values, 4);
    }
    if (motion_blur_enabled) {
        NUMTX bias = {0.5f, 0, 0, 0, 0, -0.5f, 0, 0, 0, 0, 1, 0, 0.5f, 0.5f, 0, 1};
        NUMTX inverse_bias, inverse_previous, transform;
        NuMtxInvH(&inverse_bias, &bias);
        NuMtxInvH(&inverse_previous, &motion_previous);
        NuMtxMulH(&transform, &inverse_bias, &inverse_previous);
        NuMtxMulH(&transform, &transform, &motion_current);
        NuMtxMulH(&transform, &transform, &bias);
        NuMtxTranspose(&transform, &transform);
        NuShaderProgramSetFragmentParamfv(program, 0x84, reinterpret_cast<f32 *>(&transform), 16);
    }
    if (dof_enabled || motion_blur_enabled) {
        i32 width, height;
        NuEffectTexGetDimension(blur_texture, 0, &width, &height);
        NuFramebufferAttachTex2D(blur_fbo, 0, blur_texture, 0);
        NuFramebufferBind(blur_fbo);
        NuRenderContextSetViewport(0, 0, width, height);
        PostDrawQuad();
        NuFramebufferResolveAll(true);
        if (dof_mode > 0)
            blur7x7Loopback(blur_texture, 0, blur_texture, 1, dof_mode, 1, true, 1.5f, programs[15]);
    }
}

static inline void PostBlurSetVertexParam(nushaderprogram_s *, u32, const f32 *, i32);
static inline void PostMainDrawGrid();

void NuMainFilterGen::render() {
    nuframebuffer_s *output = static_cast<nuframebuffer_s *>(portOutFramebuffer.get());
    NuProxyBuffer *color = static_cast<NuProxyBuffer *>(portColorBuffer.get());
    NuProxyBuffer *normal = static_cast<NuProxyBuffer *>(portNormalBuffer.get());
    NuProxyBuffer *depth = static_cast<NuProxyBuffer *>(portDepthBuffer.get());
    NuPostResolve(color);
    if (normal->texture != NULL)
        NuPostResolve(normal);
    nueffecttex_s *destination = NuFramebufferGetAttachedTex(output, 0, NULL, NULL);
    nueffecttex_s *depth_destination = NuFramebufferGetAttachedTex(output, 4, NULL, NULL);
    if (bloom_enabled || dof_enabled)
        preprocessBlurTextures(color->texture, normal->texture);
    if (dof_enabled || motion_blur_enabled)
        preprocessDofMotionBlur(depth->texture);
    i32 selection = (dof_enabled ? 4 : 0) | (bloom_enabled ? (bloom->directional ? 1 : 2) : 0);
    nushaderprogram_s *program = NULL;
    switch (selection) {
        case 1:
            program = programs[8];
            break;
        case 2:
            program = programs[6];
            break;
        case 4:
            program = programs[5];
            break;
        case 5:
            program = programs[9];
            break;
        case 6:
            program = programs[7];
            break;
    }
    g_boundShader = program != NULL ? program->program : 0;
    glUseProgram(g_boundShader);
    g_currentShaderProgram = program;
    if (dof_enabled) {
        f32 params[4] = {dof_blur, 1, 0, 0};
        f32 jitter[4] = {static_cast<f32>(lrand48()) * 4.656613e-10f, static_cast<f32>(lrand48()) * 4.656613e-10f, 0,
                         0};
        PostBlurSetVertexParam(program, 0x80, params, 4);
        PostBlurSetVertexParam(program, 0x81, jitter, 4);
    }
    if (bloom_enabled) {
        f32 params[4] = {blur_gain * bloom->intensity, bloom->blend, 0, 0};
        PostBlurSetVertexParam(program, 0x82, params, 4);
        f32 angle[4] = {bloom->intensity * bloom->near_scale, (bloom->far_scale - bloom->near_scale) * bloom->intensity,
                        bloom->near_angle / 180.0f, 180.0f / (bloom->far_angle - bloom->near_angle)};
        PostBlurSetVertexParam(program, 0x88, angle, 4);
        if (bloom->directional) {
            const f32 pi = 3.1415927f;
            f32 directional[4] = {bloom->direction_near_scale * bloom->intensity,
                                  (bloom->direction_far_scale - bloom->direction_near_scale) * bloom->intensity,
                                  (bloom->direction_near_angle * pi) / 180.0f,
                                  180.0f / ((bloom->direction_far_angle - bloom->direction_near_angle) * pi)};
            PostBlurSetVertexParam(program, 0x89, directional, 4);
            NUVEC4 direction = {bloom->direction.x, bloom->direction.y, bloom->direction.z, 1};
            NuVecNorm(reinterpret_cast<NUVEC *>(&direction), reinterpret_cast<NUVEC *>(&direction));
            direction.w = bloom->direction_bias;
            PostBlurSetVertexParam(program, 0x8a, &direction.x, 4);
        }
    }
    NuFramebufferBind(output);
    NuRenderContextSetViewport(0, 0, destination->width, destination->height);
    PostMainDrawGrid();
    color->texture = destination;
    color->kind = 0;
    color->enabled = true;
    color->resolved = false;
    portColorBuffer.set(color);
    if (depth_destination != NULL) {
        depth->texture = depth_destination;
        depth->kind = 4;
        depth->enabled = true;
        depth->resolved = false;
        portDepthBuffer.set(depth);
    }
}

void NuMainFilterGen::reset() {
    enabled = false;
    dof_enabled = false;
    bloom_enabled = false;
    motion_blur_enabled = false;
    active_filter_count = 0;
}

void NuPostFilterGen::GetSampleOffsets_GaussBlur5x5(i32 width, i32 height, VuVec *samples, float scale) {
    const i32 offsets[13][2] = {{-2, 0}, {-1, -1}, {-1, 0}, {-1, 1}, {0, -2}, {0, -1}, {0, 0},
                                {0, 1},  {0, 2},   {1, -1}, {1, 0},  {1, 1},  {2, 0}};
    const f32 weights[13] = {0.053990968f, 0.14676267f, 0.24197073f, 0.14676267f,  0.053990968f,
                             0.24197073f,  0.3989423f,  0.24197073f, 0.053990968f, 0.14676267f,
                             0.24197073f,  0.14676267f, 0.053990968f};
    f32 dx = 1.0f / width, dy = 1.0f / height;
    for (i32 i = 0; i < 13; ++i) {
        samples[i].x = offsets[i][0] * dx;
        samples[i].y = offsets[i][1] * dy;
        samples[i].z = weights[i] / 2.1698399f * scale;
        samples[i].w = 0.0f;
    }
}

void NuPostFilterGen::blend(nueffecttex_s *, nueffecttex_s *, nuframebuffer_s *output) {
    NuFramebufferBind(output);
    PostBindProgram(blendTexProgram);
    PostDrawQuad();
}

void NuPostFilterGen::blur5x5(nueffecttex_s *source, i32 source_lod, nueffecttex_s *textures, i32 first_lod,
                              i32 iterations, i32 levels, bool mip_chain) {
    PostBindProgram(blur5x5Program);
    for (i32 level = first_lod; level < first_lod + levels; ++level) {
        nueffecttex_s *input = level == first_lod ? source : (mip_chain ? textures : textures + level - 1);
        i32 input_lod = level == first_lod ? source_lod : (mip_chain ? level - 1 : 0);
        nueffecttex_s *output = mip_chain ? textures : textures + level;
        i32 output_lod = mip_chain ? level : 0;
        for (i32 pass = 0; pass < iterations; ++pass) {
            i32 width, height, out_width, out_height;
            NuEffectTexGetDimension(input, input_lod, &width, &height);
            NuEffectTexGetDimension(output, output_lod, &out_width, &out_height);
            VuVec samples[13];
            GetSampleOffsets_GaussBlur5x5(width, height, samples, 1.0f);
            f32 scale_bias[4] = {1, 1, 0.0f / width, 0.0f / height};
            NuFramebufferAttachTex2D(blurFbo, 0, output, output_lod);
            NuFramebufferBind(blurFbo);
            NuRenderContextSetViewport(0, 0, out_width, out_height);
            NuShaderProgramSetVertexParamfv(blur5x5Program, 0xa0, &samples[0].x, 52);
            NuShaderProgramSetVertexParamfv(blur5x5Program, 0xad, scale_bias, 4);
            PostDrawQuad();
            NuFramebufferResolve(0, true);
            input = output;
            input_lod = output_lod;
        }
    }
}

// Original generic blur inlines these immediate GL operations.
extern void (*g_glConstantSetterTable[4])(u32, i32, const void *);

struct PostBlurVertexAttribute {
    u32 type, size, normalized, reserved, offset, stride;
};
struct PostBlurVertexFormat {
    u32 mask;
    PostBlurVertexAttribute attributes[13];
};

static inline void PostBlurSetVertexParam(nushaderprogram_s *program, u32 index, const f32 *values, i32 count) {
    for (i32 i = 0; i < program->parameter_count; ++i) {
        nushaderprogramparameter_s *parameter = &program->parameters[i];
        if (parameter->register_index == index) {
            g_glConstantSetterTable[parameter->setter](parameter->location, count / 4, values);
            break;
        }
    }
}

static inline void PostBlurDrawQuad() {
    if (g_lastBoundVAO != 0)
        g_lastBoundVAO = 0;
    glBindBuffer(GL_ARRAY_BUFFER, NuPostFilter::m_fullscreenVertexBuffer);
    PostBlurVertexFormat *format = static_cast<PostBlurVertexFormat *>(g_nuFullscreenVertexFormat);
    g_boundVertexFormat = reinterpret_cast<usize>(format);
    u32 active = format->mask;
    u32 disable = g_activeAttributes & ~active;
    u32 enable = active & ~g_activeAttributes;
    g_activeAttributes = active;
    for (u32 i = 0; (active | disable | enable) != 0; ++i, active >>= 1, disable >>= 1, enable >>= 1) {
        if ((active & 1) != 0) {
            PostBlurVertexAttribute *attribute = &format->attributes[i];
            if ((enable & 1) != 0)
                glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, attribute->size, attribute->type, static_cast<u8>(attribute->normalized),
                                  attribute->stride, reinterpret_cast<void *>(attribute->offset));
        } else if ((disable & 1) != 0) {
            glDisableVertexAttribArray(i);
        }
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
static inline void PostMainDrawGrid() {
    if (g_lastBoundVAO != 0)
        g_lastBoundVAO = 0;
    glBindBuffer(GL_ARRAY_BUFFER, NuPostFilter::m_fullscreenGridVertexBuffer);
    PostBlurVertexFormat *format = static_cast<PostBlurVertexFormat *>(g_nuFullscreenVertexFormat);
    g_boundVertexFormat = reinterpret_cast<usize>(format);
    u32 active = format->mask;
    u32 disable = g_activeAttributes & ~active;
    u32 enable = active & ~g_activeAttributes;
    g_activeAttributes = active;
    for (u32 i = 0; (active | disable | enable) != 0; ++i, active >>= 1, disable >>= 1, enable >>= 1) {
        if ((active & 1) != 0) {
            PostBlurVertexAttribute *attribute = &format->attributes[i];
            if ((enable & 1) != 0)
                glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, attribute->size, attribute->type, static_cast<u8>(attribute->normalized),
                                  attribute->stride, reinterpret_cast<void *>(attribute->offset));
        } else if ((disable & 1) != 0) {
            glDisableVertexAttribArray(i);
        }
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NuPostFilter::m_fullscreenGridIndexBuffer);
    glDrawElements(GL_TRIANGLES, NuPostFilter::m_quadGridPrimCount * 3, GL_UNSIGNED_SHORT, NULL);
}

void NuPostFilterGen::blur7x7Loopback(nueffecttex_s *source, i32 source_lod, nueffecttex_s *textures, i32 first_lod,
                                      i32 iterations, i32 levels, bool mip_chain, float radius,
                                      nushaderprogram_s *program) {
    {
        nushaderprogram_s *bound = program;
        g_boundShader = bound != NULL ? bound->program : 0;
        glUseProgram(g_boundShader);
        g_currentShaderProgram = bound;
    }
    for (i32 level = first_lod; level < first_lod + levels; ++level) {
        nueffecttex_s *input = level == first_lod ? source : (mip_chain ? textures : textures + level - 1);
        i32 input_lod = level == first_lod ? source_lod : (mip_chain ? level - 1 : 0);
        nueffecttex_s *output = mip_chain ? textures : textures + level;
        i32 output_lod = mip_chain ? level : 0;
        for (i32 pass = 0; pass < iterations; ++pass) {
            i32 width, height, out_width, out_height;
            NuEffectTexGetDimension(input, input_lod, &width, &height);
            NuEffectTexGetDimension(output, output_lod, &out_width, &out_height);
            f32 dx = radius / width;
            f32 dy = radius / height;
            VuVec horizontal[7] = {
                {dx * 0.0f, 0.0f, .34f, 0.0f},  {dx * 1.0f, 0.0f, .18f, 0.0f},  {dx * 2.0f, 0.0f, .10f, 0.0f},
                {dx * 3.0f, 0.0f, .05f, 0.0f},  {dx * -1.0f, 0.0f, .18f, 0.0f}, {dx * -2.0f, 0.0f, .10f, 0.0f},
                {dx * -3.0f, 0.0f, .05f, 0.0f},
            };
            VuVec vertical[7] = {
                {0.0f, dy * 0.0f, .34f, 0.0f},  {0.0f, dy * 1.0f, .18f, 0.0f},  {0.0f, dy * 2.0f, .10f, 0.0f},
                {0.0f, dy * 3.0f, .05f, 0.0f},  {0.0f, dy * -1.0f, .18f, 0.0f}, {0.0f, dy * -2.0f, .10f, 0.0f},
                {0.0f, dy * -3.0f, .05f, 0.0f},
            };
            f32 bias[4] = {1, 1, 0.0f / width, 0.0f / height};
            f32 output_bias[4] = {1, 1, 0.0f / out_width, 0.0f / out_height};
            NuFramebufferAttachTex2D(NuPostFilterGen::blurFbo, 0, output, output_lod);
            NuFramebufferBind(NuPostFilterGen::blurFbo);
            NuRenderContextSetViewport(0, 0, out_width, out_height);
            PostBlurSetVertexParam(program, 0xa0, &horizontal[0].x, 28);
            PostBlurSetVertexParam(program, 0xad, bias, 4);
            PostBlurDrawQuad();
            NuFramebufferResolve(0, true);
            NuFramebufferAttachTex2D(NuPostFilterGen::blurFbo, 0, output, output_lod);
            NuFramebufferBind(NuPostFilterGen::blurFbo);
            NuRenderContextSetViewport(0, 0, out_width, out_height);
            PostBlurSetVertexParam(program, 0xa0, &vertical[0].x, 28);
            PostBlurSetVertexParam(program, 0xad, output_bias, 4);
            PostBlurDrawQuad();
            NuFramebufferResolve(0, true);
            input = output;
            input_lod = output_lod;
        }
    }
}

void NuPostFilterGen::blur7x7Separate(nueffecttex_s *source, i32 source_lod, nueffecttex_s *textures, i32 first_lod,
                                      i32 iterations, i32 levels, bool mip_chain, float radius,
                                      nushaderprogram_s *program) {
    i32 guard = static_cast<i32>(ceil(radius)) * 3;
    i32 work_width = workTex->width;
    i32 work_height = workTex->height;
    {
        nushaderprogram_s *bound = program;
        g_boundShader = bound != NULL ? bound->program : 0;
        glUseProgram(g_boundShader);
        g_currentShaderProgram = bound;
    }
    for (i32 level = first_lod; level < first_lod + levels; ++level) {
        nueffecttex_s *input = level == first_lod ? source : (mip_chain ? textures : textures + level - 1);
        i32 input_lod = level == first_lod ? source_lod : (mip_chain ? level - 1 : 0);
        nueffecttex_s *output = mip_chain ? textures : textures + level;
        i32 output_lod = mip_chain ? level : 0;
        for (i32 pass = 0; pass < iterations; ++pass) {
            i32 width, height, out_width, out_height;
            NuEffectTexGetDimension(input, input_lod, &width, &height);
            NuEffectTexGetDimension(output, output_lod, &out_width, &out_height);
            f32 dx = radius / width;
            f32 dy = radius / height;
            VuVec horizontal[7] = {
                {dx * 0.0f, 0.0f, .34f, 0.0f},  {dx * 1.0f, 0.0f, .18f, 0.0f},  {dx * 2.0f, 0.0f, .10f, 0.0f},
                {dx * 3.0f, 0.0f, .05f, 0.0f},  {dx * -1.0f, 0.0f, .18f, 0.0f}, {dx * -2.0f, 0.0f, .10f, 0.0f},
                {dx * -3.0f, 0.0f, .05f, 0.0f},
            };
            VuVec vertical[7] = {
                {0.0f, dy * 0.0f, .34f, 0.0f},  {0.0f, dy * 1.0f, .18f, 0.0f},  {0.0f, dy * 2.0f, .10f, 0.0f},
                {0.0f, dy * 3.0f, .05f, 0.0f},  {0.0f, dy * -1.0f, .18f, 0.0f}, {0.0f, dy * -2.0f, .10f, 0.0f},
                {0.0f, dy * -3.0f, .05f, 0.0f},
            };
            f32 bias[4] = {1, 1, 0.0f / width, 0.0f / height};
            nueffecttex_s *work = NuPostFilterGen::workTex;
            NuFramebufferAttachTex2D(NuPostFilterGen::blurFbo, 0, work, 0);
            NuFramebufferBind(NuPostFilterGen::blurFbo);
            NuRenderContextSetViewport(0, 0, out_width, out_height);
            PostBlurSetVertexParam(program, 0xa0, &horizontal[0].x, 28);
            PostBlurSetVertexParam(program, 0xad, bias, 4);
            PostBlurDrawQuad();
            {
                {
                    nushaderprogram_s *bound = NuPostFilterGen::blurGuardProgram;
                    g_boundShader = bound != NULL ? bound->program : 0;
                    glUseProgram(g_boundShader);
                    g_currentShaderProgram = bound;
                }
                if (out_height < work_height) {
                    const f32 edge[4] = {1, 0, 0, 1};
                    PostBlurSetVertexParam(NuPostFilterGen::blurGuardProgram, 0x80, edge, 4);
                    NuRenderContextSetViewport(0, out_height, out_width, guard);
                    PostBlurDrawQuad();
                }
                if (out_width < work_width) {
                    const f32 edge[4] = {0, 1, 1, 0};
                    NuRenderContextSetViewport(out_width, 0, guard, out_height);
                    PostBlurSetVertexParam(NuPostFilterGen::blurGuardProgram, 0x80, edge, 4);
                    PostBlurDrawQuad();
                }
            }
            NuFramebufferResolve(0, true);
            NuFramebufferAttachTex2D(NuPostFilterGen::blurFbo, 0, output, output_lod);
            NuFramebufferBind(NuPostFilterGen::blurFbo);
            NuRenderContextSetViewport(0, 0, out_width, out_height);
            {
                nushaderprogram_s *bound = program;
                g_boundShader = bound != NULL ? bound->program : 0;
                glUseProgram(g_boundShader);
                g_currentShaderProgram = bound;
            }
            bias[0] = static_cast<f32>(out_width) / work_width;
            bias[1] = static_cast<f32>(out_height) / work_height;
            bias[2] = 0.0f / work_width;
            bias[3] = 0.0f / work_height;
            PostBlurSetVertexParam(program, 0xa0, &vertical[0].x, 28);
            PostBlurSetVertexParam(program, 0xad, bias, 4);
            PostBlurDrawQuad();
            NuFramebufferResolve(0, true);
            input = output;
            input_lod = output_lod;
        }
    }
}

void NuPostFilterGen::copy(nueffecttex_s *destination, i32 lod, nueffecttex_s *, i32, nushaderprogram_s *program,
                           nueffecttex_s *) {
    i32 width, height;
    NuEffectTexGetDimension(destination, lod, &width, &height);
    NuFramebufferAttachTex2D(copyFbo, 0, destination, lod);
    NuFramebufferBind(copyFbo);
    NuRenderContextSetViewport(0, 0, width, height);
    PostBindProgram(program);
    PostDrawQuad();
    NuFramebufferResolve(0, true);
}

void NuPostFilterGen::copy(nueffecttex_s *color, nueffecttex_s *, nuframebuffer_s *output) {
    NuFramebufferBind(output);
    PostBindProgram(copyTexColorDepthProgram);
    if (NuFramebufferGetWidth(output) == color->width)
        NuFramebufferGetHeight(output);
    PostDrawQuad();
}

void NuPostFilterGen::copy(nueffecttex_s *, nuframebuffer_s *output) {
    NuFramebufferBind(output);
    PostBindProgram(copyTexProgram);
    PostDrawQuad();
}

void NuPostFilterGen::copyDepth(nueffecttex_s *, nuframebuffer_s *) {
}

void NuPostFilterGen::destroyResources() {
    NuFramebufferDestroy(input_fbo);
    input_fbo = NULL;
}

void NuPostFilterGen::destroySharedResources() {
}

void NuPostFilterGen::destroySharedTextureResources() {
}

void NuPostFilterGen::initResources() {
    input_fbo = NuFramebufferCreate();
}

void NuPostFilterGen::initSharedResources() {
    NuPostDataPort *ports[] = {&portOutFramebuffer, &portColorBuffer,   &portNormalBuffer,
                               &portVelocityBuffer, &portDepthRTBuffer, &portDepthBuffer};
    const char *names[] = {"postEffect.outFramebuffer", "postEffect.colorBuffer",   "postEffect.normalBuffer",
                           "postEffect.velocityBuffer", "postEffect.depthRTBuffer", "postEffect.depthBuffer"};
    for (i32 i = 0; i < 6; ++i) {
        if (ports[i]->index >= 0)
            --ports[i]->manager->entries[ports[i]->index].references;
        ports[i]->manager = &resourceManager;
        ports[i]->index = resourceManager.registerPort(names[i], NULL);
    }
    blurFbo = NuFramebufferCreate();
    copyFbo = NuFramebufferCreate();
}

void NuPostFilterGen::initSharedTextureResources(i32 width, i32 height) {
    workTex = NuEffectTexCreate2D(width, height, 1, 0x11, 2);
}

void NuPostFilterGen::renderFrustum(numtx_s *) {
}

void NuPostFilterGen::renderQuad() {
}

void NuPostFilterGen::renderQuadGrid() {
}

__attribute__((weak)) void NuPostFilterGen::reset() {
    enabled = false;
}

__attribute__((weak)) void NuPostFilterGen::resetAll() {
}

void NuDeferredFilter::initResources() {
}

i32 NuDataPortManager::registerPort(char const *name, void *data) {
    for (i32 i = 0; i < 256; ++i) {
        if (NuStrCmp(entries[i].name, name) == 0) {
            entries[i].data = data;
            return i;
        }
    }
    for (i32 i = 0; i < 256; ++i) {
        if (entries[i].references == 0) {
            memmove(entries[i].name, name, NuStrLen(name) + 1);
            entries[i].data = data;
            ++entries[i].references;
            return i;
        }
    }
    return -1;
}

NuMotionFilterGen::NuMotionFilterGen() {
    scale = maximum = 0.0f;
    falloff = 1.0f;
}

void NuMotionFilterGen::render() {
    nuframebuffer_s *output = static_cast<nuframebuffer_s *>(portOutFramebuffer.get());
    NuProxyBuffer *color = static_cast<NuProxyBuffer *>(portColorBuffer.get());
    NuProxyBuffer *velocity = static_cast<NuProxyBuffer *>(portVelocityBuffer.get());
    NuPostResolve(color);
    NuPostResolve(velocity);
    nueffecttex_s *destination = NuFramebufferGetAttachedTex(output, 0, NULL, NULL);
    PostBindProgram(programs[0]);
    f32 ratio = maximum / scale;
    f32 params[4] = {ratio, 0.5f, 0, 0};
    f32 weights[7][4];
    for (i32 i = 0; i < 7; ++i) {
        weights[i][0] = (NuPow(static_cast<f32>(i + 1) / 7.0f, falloff) - 0.5f) * ratio;
        weights[i][1] = weights[i][0];
        weights[i][2] = weights[i][3] = 0.0f;
    }
    NuShaderProgramSetFragmentParamfv(programs[0], 0x8a, params, 4);
    NuShaderProgramSetFragmentParamfv(programs[0], 0x8b, &weights[0][0], 28);
    NuFramebufferBind(output);
    PostDrawQuad();
    color->texture = destination;
    color->kind = 4;
    color->enabled = true;
    color->resolved = false;
    portColorBuffer.set(color);
}

void NuSpeedBlurFilter::initResources() {
}

NuDeferredFilterGen::NuDeferredFilterGen() {
    for (i32 i = 0; i < 6; ++i)
        shadow_fbos[i] = NULL;
    light_fbo = NULL;
    enabled = false;
    sample_count = 4;
    parameters[0] = 0.0f;
    parameters[1] = 0.5f;
}

void NuDeferredFilterGen::destroyResources() {
    for (i32 i = 0; i < 6; ++i)
        NuFramebufferDestroy(shadow_fbos[i]);
    NuFramebufferDestroy(light_fbo);
    NuPostFilterGen::destroyResources();
}

void NuDeferredFilterGen::destroyTextureResources() {
    for (i32 i = 0; i < 4; ++i)
        textures[i] = NULL;
}

void NuDeferredFilterGen::initResources() {
    NuPostFilterGen::initResources();
    for (i32 i = 0; i < 6; ++i)
        shadow_fbos[i] = NuFramebufferCreate();
    light_fbo = NuFramebufferCreate();
    NuFramebufferAttachTex2D(light_fbo, 0, textures[0], 0);
    dynamic_light_count = deferred_geometry_count = 0;
    resetAll();
}

void NuDeferredFilterGen::initTextureResources(i32 width, i32 height) {
    textures[2] = NuEffectTexCreate2D(768, 1344, 1, 1, 4);
    textures[3] = NuEffectTexCreate2D(640, 2048, 1, 1, 4);
    textures[4] = textures[5] = NULL;
    textures[0] = NuEffectTexCreate2D(width / 2, height, sample_count, 1, 2);
    textures[1] = NuEffectTexCreate2D(1, 1, 1, 0, 2);
}

void NuDeferredFilterGen::render() {
    nuframebuffer_s *output = static_cast<nuframebuffer_s *>(portOutFramebuffer.get());
    NuProxyBuffer *color = static_cast<NuProxyBuffer *>(portColorBuffer.get());
    NuProxyBuffer *normal = static_cast<NuProxyBuffer *>(portNormalBuffer.get());
    NuProxyBuffer *depth_rt = static_cast<NuProxyBuffer *>(portDepthRTBuffer.get());
    NuProxyBuffer *depth = static_cast<NuProxyBuffer *>(portDepthBuffer.get());
    NuPostResolve(color);
    NuPostResolve(normal);
    NuPostResolve(depth_rt);
    NuPostResolve(depth);
    nueffecttex_s *destination = NuFramebufferGetAttachedTex(output, 0, NULL, NULL);
    f32 last_sample = static_cast<f32>(sample_count) - 1.0f;
    bool first = true;
    for (i32 i = 0; i < dynamic_light_count; ++i) {
        NuDynamicLight *light = dynamic_lights[i];
        if (light->reserved_7bc == 0)
            continue;
        NUMTX view, projection;
        memcpy(&view, g_renderContext_view, sizeof(view));
        memcpy(&projection, g_renderContext_projection, sizeof(projection));
        i32 shadow_count = light->active_render_set_count;
        for (i32 shadow = 0; shadow < shadow_count; ++shadow) {
            // The original selects consecutive texture members beginning at 0x30.
            nueffecttex_s *shadow_texture = textures[shadow + 2];
            NuFramebufferAttachTex2D(shadow_fbos[shadow], 4, shadow_texture, 0);
            NuFramebufferBind(shadow_fbos[shadow]);
            i32 width, height;
            NuEffectTexGetDimension(shadow_texture, 0, &width, &height);
            NuRenderContextSetViewport(0, 0, width, height);
            NuFramebufferClear(0x300, 0xffffffff);
            light->renderShadowMap(shadow, shadow_fbos[shadow]);
            NuFramebufferResolveAll(true);
        }
        NuRenderContextSetViewProj(&view, &projection);
        NuFramebufferBind(light_fbo);
        if (first)
            NuFramebufferClear(0x900, 0x00ff0000);
        i32 type = light->parameter_5;
        nushaderprogram_s *program = programs[type == 0 || type == 1 ? 0 : 1];
        if (!first) {
            g_boundShader = 0;
            glUseProgram(0);
            g_currentShaderProgram = NULL;
        }
        {
            nushaderprogram_s *bound = program;
            g_boundShader = bound != NULL ? bound->program : 0;
            glUseProgram(g_boundShader);
            g_currentShaderProgram = bound;
        }
        light->bindShaderResources(program);
        PostBlurDrawQuad();
        NuFramebufferResolve(0, false);
        first = false;
    }
    copy(textures[0], 1, textures[0], 0, copyTexProgram, NULL);
    blur7x7Loopback(textures[0], 1, textures[0], 2, 2, sample_count - 2, true, 1.0f, blur7x7Program);
    NuFramebufferBind(output);
    i32 width = NuFramebufferGetWidth(output);
    i32 height = NuFramebufferGetHeight(output);
    NuRenderContextSetViewport(0, 0, width, height);
    {
        nushaderprogram_s *bound = programs[2];
        g_boundShader = bound != NULL ? bound->program : 0;
        glUseProgram(g_boundShader);
        g_currentShaderProgram = bound;
    }
    f32 params[4] = {parameters[1], last_sample, parameters[2], parameters[3]};
    PostBlurSetVertexParam(programs[2], 0x80a0, params, 4);
    PostBlurDrawQuad();
    color->texture = destination;
    color->kind = 0;
    color->enabled = true;
    color->resolved = false;
    portColorBuffer.set(color);
}

void NuDeferredFilterGen::renderStencilMask(NuDynamicLight &) {
}

void NuDeferredFilterGen::resetAll() {
    for (i32 i = 0; i < dynamic_light_count; ++i) {
        dynamic_lights[i]->resetGeometry();
    }
    dynamic_light_count = 0;
    deferred_geometry_count = 0;
}

void NuMotionAccumFilter::initResources() {
}

NuSpeedBlurFilterGen::NuSpeedBlurFilterGen() {
}

void NuSpeedBlurFilterGen::computeSpeedBlur(VuVec &result) {
    NUMTX previous_inverse, current_inverse;
    NuMtxInvH(&previous_inverse, const_cast<NUMTX *>(&parameters->previous));
    NuMtxInvH(&current_inverse, const_cast<NUMTX *>(&parameters->current));
    NUVEC4 motion;
    NuVec4MtxTransformH(&motion, reinterpret_cast<NUVEC4 *>(&previous_inverse) + 3,
                        const_cast<NUMTX *>(&parameters->current));
    if (motion.z < 0.0f) {
        NuVec4MtxTransformH(&motion, reinterpret_cast<NUVEC4 *>(&current_inverse) + 3,
                            const_cast<NUMTX *>(&parameters->previous));
        motion.x = -motion.x;
        motion.y = -motion.y;
        motion.z = -motion.z;
    }
    NuVec4Scale(&motion, &motion, 1.0f / motion.w);
    motion.x *= motionFactorPan;
    motion.y *= motionFactorPan;
    motion.z *= motionFactorPull;
    NuVec4Scale(&motion, &motion, parameters->scale);
    result.x = motion.x < -motionFactorPanClamp ? -motionFactorPanClamp
                                                : (motion.x > motionFactorPanClamp ? motionFactorPanClamp : motion.x);
    result.y = motion.y < -motionFactorPanClamp ? -motionFactorPanClamp
                                                : (motion.y > motionFactorPanClamp ? motionFactorPanClamp : motion.y);
    result.z = motion.z;
    result.w = 0.0f;
}

void NuSpeedBlurFilterGen::destroyTextureResources() {
}

void NuSpeedBlurFilterGen::initTextureResources(i32 width, i32 height) {
    texture = NuEffectTexCreate2D(width / 2, height / 2, 1, 1, 2);
}

void NuSpeedBlurFilterGen::render() {
    nuframebuffer_s *output = static_cast<nuframebuffer_s *>(portOutFramebuffer.get());
    NuProxyBuffer *color = static_cast<NuProxyBuffer *>(portColorBuffer.get());
    NuProxyBuffer *depth = static_cast<NuProxyBuffer *>(portDepthBuffer.get());
    NuPostResolve(color);
    NuPostResolve(depth);
    struct {
        f32 depth[4];
        VuVec motion;
    } values;
    // The original supplies two vec4 registers; only the first two depth
    // components and the motion vector are assigned.
    values.depth[0] = 0.0f;
    values.depth[1] = parameters->scale * 100000.0f;
    computeSpeedBlur(values.motion);
    nueffecttex_s *destination = NuFramebufferGetAttachedTex(output, 0, NULL, NULL);
    if (texture != NULL)
        copy(texture, 0, color->texture, 0, copyTexProgram, NULL);
    PostBindProgram(programs[0]);
    NuShaderProgramSetVertexParamfv(programs[0], 0x80, values.depth, 8);
    NuFramebufferBind(output);
    PostDrawQuad(true);
    color->texture = destination;
    color->kind = 0;
    color->enabled = true;
    color->resolved = false;
    portColorBuffer.set(color);
}

f32 NuMotionAccumFilterGen::GetTiming(i32 *last_frame) {
    const f32 frame_time = nuapi.forced_frame_time;
    if (frames != 0 && frame_time != 0.0f) {
        if (blend > 1.0f) {
            blend = 1.0f;
        } else if (blend < 0.0f) {
            blend = 0.0f;
        }
        *last_frame = current_frame == frames - 1;
        return frame_time / frames;
    }
    *last_frame = 1;
    return 0.0f;
}

NuMotionAccumFilterGen::NuMotionAccumFilterGen() {
    blend = 1.0f;
    frames = mode = 0;
}

void NuMotionAccumFilterGen::destroyResources() {
    NuFramebufferDestroy(accumulation_fbo);
    accumulation_fbo = NULL;
    NuPostFilterGen::destroyResources();
}

void NuMotionAccumFilterGen::destroyTextureResources() {
}

void NuMotionAccumFilterGen::initResources() {
    NuPostFilterGen::initResources();
    accumulation_fbo = NuFramebufferCreate();
    NuFramebufferAttachTex2D(accumulation_fbo, 0, accumulation_texture, 0);
}

void NuMotionAccumFilterGen::initTextureResources(i32 width, i32 height) {
    accumulation_texture = NuEffectTexCreate2D(width, height, 1, 1, 2);
}

void NuMotionAccumFilterGen::render() {
    nuframebuffer_s *output = static_cast<nuframebuffer_s *>(portOutFramebuffer.get());
    NuProxyBuffer *color = static_cast<NuProxyBuffer *>(portColorBuffer.get());
    NuPostResolve(color);
    nueffecttex_s *destination = NuFramebufferGetAttachedTex(output, 0, NULL, NULL);
    f32 weights[256], sum = 0.0f;
    f32 exponent;
    memcpy(&exponent, &mode, sizeof(exponent));
    for (i32 i = 0; i < frames; ++i) {
        weights[i] = NuPow(static_cast<f32>(i + 1) / frames, exponent);
        sum += weights[i];
    }
    f32 reciprocal_sum = 1.0f / sum;
    for (i32 i = 0; i < frames; ++i)
        weights[i] *= reciprocal_sum;
    sum = 0.0f;
    for (i32 i = 0; i < frames; ++i) {
        sum += weights[i];
        weights[i] /= sum;
    }
    current_frame = current_frame + 1 == frames ? 0 : current_frame + 1;
    PostBindProgram(program);
    f32 params[4] = {weights[current_frame], 0, 0, 0};
    NuShaderProgramSetFragmentParamfv(program, 0x8a, params, 4);
    i32 width, height;
    NuEffectTexGetDimension(accumulation_texture, 0, &width, &height);
    NuFramebufferBind(accumulation_fbo);
    NuRenderContextSetViewport(0, 0, width, height);
    PostDrawQuad();
    NuFramebufferResolve(0, true);
    PostBindProgram(program);
    NuShaderProgramSetFragmentParamfv(program, 0x8a, params, 4);
    NuFramebufferBind(output);
    PostDrawQuad();
    color->texture = destination;
    color->kind = 4;
    color->enabled = true;
    color->resolved = false;
    portColorBuffer.set(color);
}

void NuNetEmu::FindPacket(nunetaddr_s *, i32) {
}

NuNetEmu::NuNetEmu() {
}

void NuNetEmu::RecvFrom(void *, i32, nunetaddr_s &) {
}

void NuNetEmu::SendTo(void *, i32, nunetaddr_s *, i32) {
}

void NuNetEmu::SetConditions(NuNetEmu::eConditions) {
}

void NuNetEmu::SplitSendPacket(NuNetEmu::EmuPacket *) {
}

void NuNetEmu::Update() {
}

NUMTX NuDynamicLight::cacheCameraView;
NUMTX NuDynamicLight::cacheCameraProj;
