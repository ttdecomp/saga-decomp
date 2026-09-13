#include "nu2api_nucore_types.h"
#include "nu2api/nu3d/android/nudlist_callbacks.h"
#include "nu2api/nu3d/nulgtlaser.h"
#include "nu2api/nu3d/nuprim.h"
#include "nu2api/numath/nuvec4.h"
#include "nu2api/numath/nutrig.h"
extern "C" {
    i32 NuRndrBeginSceneEx(i32, i32, i32);
    void NuRndrEndSceneEx(i32);
    extern i32 NuPrimCSPos;
    extern NUPRIMSCALEMODE NuPrimCoordSystemStack[];
    extern i32 nurndr_pixel_width;
}

#include "nu2api/nu3d/android/nutex_ios_ex.h"
#include "nu2api/nu3d/android/nutex_android.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/nushader.h"
#include "nu2api/nu3d/nushader_plain.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuquat.h"

#include <GLES2/gl2.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

extern "C" void BeginCriticalSectionGL(const char *, i32);
extern "C" void EndCriticalSectionGL(const char *, i32);
extern i32 bgProcIsBgThread(void);

extern "C" f32 *NuAnimCurveExtractAllNodeCurves_3(ani3_animheader_s *, i32, f32, char *);
extern "C" void NuAnimData2CalcTime(nuanimdata2_s *, f32, nuanimtime_s *);
extern "C" f32 NuAnimCurve2CalcValEx(nuanimcurve2_s *, nuanimtime_s *, u32);
void NuGCutRigidCalcMtx_3(NUGCUTRIGID_s *, f32, numtx_s *);

i32 GetIntCurveVal(ani3_animheader_s *animation, f32 *values, i32 curve) {
    if (animation->curve_types[curve] == 10) {
        return reinterpret_cast<i32 *>(values)[curve];
    }
    const f32 value = values[curve];
    return static_cast<i32>(value < 0.0f ? value - 0.5f : value + 0.5f);
}

extern "C" void NuAnimBuffCreateScratch(nuanimbuff_s *buffer);
extern "C" void NuAnimBuffDestroyScratch(nuanimbuff_s *buffer);
extern "C" void NuAnimBuffAccumulate_3(nuanimbuff_s *buffer, ani3_animheader_s *animation, f32 time, i32 overwrite,
                                       f32 blend, i32 first_joint, nuhgobj_s *object, NUVEC *root_translation);
extern "C" void NuAnimBuffEvaluate_3(nuanimbuff_s *buffer, nuhgobj_s *object, NUMTX *matrices,
                                     ani3_animheader_s *animation, NUHGOBJROOTFN root_fn, NUVEC *root_translation,
                                     void *root_data);

void NuHGobjRead(variptr_u *, char *) {
}

static void NuHtmlFlush(i32) {
}

void NuErrorPrint(char *message) {
    printf("%s", message);
}

void NuFntFindEnd(nutex_s *, i32 *, i32 *, i32, i32) {
}

void NuWindFreeGrp(NuWindGType *group) {
    if (group != NULL) {
        group->in_use = 0;
    }
}



void NuFntFindStart(nutex_s *, i32 *, i32 *, i32, i32) {
}

void NuFntPrintChar(char) {
}

void NuQFntSetMtx2d(void *, numtx_s *) {
}

void NuWarningPrint(char *message) {
    printf("%s", message);
}

void NuDebugMsgPrint(char *message) {
    printf("%s", message);
}

void NuVpSetDestRect(float, float, float, float) {
}

extern "C" u8 CutSceneBoundingBoxTrackRoot;

// Original @0x2da1a0. The public type is nugscn_s in the mangled symbol, but
// GHG objects carry the hierarchy layout consumed here.
i32 NuCameraClipHGobj(nugscn_s *scene, numtx_s *world_matrix, numtx_s *root_matrix) {
    nuhgobj_s *object = reinterpret_cast<nuhgobj_s *>(scene);
    if (CutSceneBoundingBoxTrackRoot == 0) {
        return NuCameraClipTestExtents(&object->bounds_min, &object->bounds_max, world_matrix, 0.0f, 0);
    }

    NUVEC half_extents;
    NuVecSub(&half_extents, &object->bounds_max, &object->bounds_min);
    NuVecScale(&half_extents, &half_extents, 0.5f);
    NUVEC min;
    NuVecNeg(&min, &half_extents);

    NUMTX translated_world = *world_matrix;
    NuMtxPreTranslate(&translated_world, NUMTX_GET_ROW_VEC(root_matrix, 3));
    return NuCameraClipTestExtents(&min, &half_extents, &translated_world, 0.0f, 0);
}

void NuFntDumpReadable(nufnt_s *, char *) {
}

// NuIOS_SetCullMode is transcribed in android/nuiosdl_gl.cpp (original 0x29c110).

static inline u16 NuLgtArcHalf(f32 value) {
    union {
        f32 value;
        u32 bits;
    } conversion = {value};
    i32 sign = conversion.bits >> 31;
    i32 exponent = (conversion.bits >> 23) & 0xff;
    i32 mantissa = conversion.bits & 0x7fffff;
    // Preserve the original truncation and exponent clamp, including subnormals.
    exponent -= 112;
    if (exponent < 0)
        exponent = 0;
    if (exponent > 31)
        exponent = 31;
    return (sign << 15) | ((exponent & 31) << 10) | (mantissa >> 13);
}
static inline void NuLgtArcVertex(NUVEC4 *position, i32 colour, f32 u, f32 v) {
    g_NuPrim_StreamBufferPtr->u32_ptr[3] =
        g_NuPrim_NeedsOverbrightening ? colour : ((colour >> 1) & 0x7f7f7f) | (colour & 0xff000000);
    if (g_NuPrim_NeedsHalfUVs) {
        ((u16 *)g_NuPrim_StreamBufferPtr->u8_ptr)[8] = NuLgtArcHalf(u);
        ((u16 *)g_NuPrim_StreamBufferPtr->u8_ptr)[9] = NuLgtArcHalf(v);
    } else {
        g_NuPrim_StreamBufferPtr->f32_ptr[4] = u;
        g_NuPrim_StreamBufferPtr->f32_ptr[5] = v;
    }
    NuPrim2DAddXYZ(position->x, position->y, position->z);
}
void NuLgtArcLaserDraw(i32 paused) {
    f32 v0 = NuLgtArcMtl[0].v0, v1 = NuLgtArcMtl[0].v1;
    if (paused)
        NuLgtArcLaserCnt = NuLgtArcLaserOldCnt;
    if (!NuLgtArcMtl[0].material || NuLgtArcLaserCnt == 0) {
        NuLgtArcLaserOldCnt = 0;
        return;
    }
    NUVEC4 *vertices = (NUVEC4 *)NuScratchAlloc32(0xc0);
    NUMTX *clip = NuCameraGetVPCSMtx();
    if (!NuRndrBeginSceneEx(-1, -2, 0)) {
        NuScratchRelease();
        return;
    }
    ++NuPrimCSPos;
    NuPrimSetCoordinateSystem(NUPRIM_SCALEMODE_PS2);
    NuPrim2DBegin(0, 5, NuLgtArcMtl[0].material);
    for (i32 i = 0; i < NuLgtArcLaserCnt; ++i) {
        NULGTARCLASER *laser = &NuLgtArcLaserData[i];
        vertices[4].x = laser->start.x;
        vertices[4].y = laser->start.y;
        vertices[4].z = laser->start.z;
        vertices[4].w = 1.0f;
        NuVec4MtxTransformVU0(&vertices[4], &vertices[4], clip);
        vertices[5].x = laser->end.x;
        vertices[5].y = laser->end.y;
        vertices[5].z = laser->end.z;
        vertices[5].w = 1.0f;
        NuVec4MtxTransformVU0(&vertices[5], &vertices[5], clip);
        if (!(vertices[4].w >= 0.5f) && !(vertices[5].w >= 0.5f))
            continue;
        if (vertices[4].w < 0.5f) {
            f32 numerator = vertices[5].w - 0.5f;
            f32 denominator = vertices[5].w - vertices[4].w;
            vertices[6].x = (laser->start.x - laser->end.x) * numerator / denominator + laser->end.x;
            vertices[6].y = (laser->start.y - laser->end.y) * numerator / denominator + laser->end.y;
            vertices[6].z = (laser->start.z - laser->end.z) * numerator / denominator + laser->end.z;
        } else {
            vertices[6].x = laser->start.x;
            vertices[6].y = laser->start.y;
            vertices[6].z = laser->start.z;
        }
        vertices[6].w = 1.0f;
        f32 dx = vertices[6].x - laser->end.x;
        f32 dy = vertices[6].y - laser->end.y;
        f32 dz = vertices[6].z - laser->end.z;
        sqrt((double)(dx * dx + dy * dy + dz * dz));
        if (vertices[5].w < 0.5f) {
            f32 numerator = vertices[4].w - 0.5f;
            f32 denominator = vertices[4].w - vertices[5].w;
            vertices[7].x = (laser->end.x - laser->start.x) * numerator / denominator + laser->start.x;
            vertices[7].y = (laser->end.y - laser->start.y) * numerator / denominator + laser->start.y;
            vertices[7].z = (laser->end.z - laser->start.z) * numerator / denominator + laser->start.z;
        } else {
            vertices[7].x = laser->end.x;
            vertices[7].y = laser->end.y;
            vertices[7].z = laser->end.z;
        }
        vertices[7].w = 1.0f;
        NuVec4MtxTransformVU0(&vertices[4], &vertices[6], clip);
        NuVec4ScaleXYZVU0(&vertices[4], &vertices[4], 1.0f / vertices[4].w);
        NuVec4MtxTransformVU0(&vertices[5], &vertices[7], clip);
        NuVec4ScaleXYZVU0(&vertices[5], &vertices[5], 1.0f / vertices[5].w);
        NUVEC perpendicular = {(vertices[5].y - vertices[4].y) * ((f32)nurndr_pixel_width / 240.0f),
                               vertices[4].x - vertices[5].x, 0.0f};
        NuVecNorm(&perpendicular, &perpendicular);
        perpendicular.x *= (((f32)nurndr_pixel_width * 150.0f) / 240.0f) * laser->width;
        perpendicular.y *= laser->width * 150.0f;
        vertices[7].x -= vertices[6].x;
        vertices[7].y -= vertices[6].y;
        vertices[7].z -= vertices[6].z;
        vertices[7].w -= vertices[6].w;
        f32 length =
            NuFsqrt(vertices[7].x * vertices[7].x + vertices[7].y * vertices[7].y + vertices[7].z * vertices[7].z);
        if (!(length > 0.0f))
            continue;
        vertices[10].x = laser->bend.x;
        vertices[10].y = laser->bend.y;
        vertices[10].z = laser->bend.z;
        f32 factor =
            vertices[10].x * vertices[10].x + vertices[10].y * vertices[10].y + vertices[10].z * vertices[10].z;
        if (factor > 0.0f)
            factor = laser->bend_amount / NuFsqrt(factor);
        vertices[10].x *= factor;
        vertices[10].y *= factor;
        vertices[10].z *= factor;
        NuLgtSeed = laser->seed;
        f32 step = laser->segment_length / length;
        step = 1.0f < step ? 1.0f : step;
        f32 u = NuLgtArcMtl[0].u0, u1 = NuLgtArcMtl[0].u1;
        vertices[4].w = 1.0f;
        vertices[4].x = vertices[6].x;
        vertices[4].y = vertices[6].y;
        vertices[4].z = vertices[6].z;
        NuVec4MtxTransformVU0(&vertices[4], &vertices[4], clip);
        NuVec4ScaleXYZVU0(&vertices[4], &vertices[4], 1.0f / vertices[4].w);
        vertices[1].x = vertices[3].x = vertices[4].x;
        vertices[1].y = vertices[3].y = vertices[4].y;
        vertices[1].z = vertices[3].z = vertices[4].z;
        f32 progress = 0.0f;
        do {
            f32 next = progress + step;
            if (next >= 1.0f) {
                vertices[5].x = vertices[6].x + vertices[7].x;
                vertices[5].y = vertices[6].y + vertices[7].y;
                vertices[5].z = vertices[6].z + vertices[7].z;
                u = NuLgtArcMtl[0].u1 + (1.0f - progress) * (NuLgtArcMtl[0].u0 - NuLgtArcMtl[0].u1) / step;
            } else {
                i32 angle = ((i32)(32768.0f * next) >> 1) & 0x7fff;
                vertices[5].x = next * vertices[7].x + vertices[6].x + vertices[10].x * NuTrigTable[angle];
                vertices[5].y = next * vertices[7].y + vertices[6].y + vertices[10].y * NuTrigTable[angle];
                vertices[5].z = next * vertices[7].z + vertices[6].z + vertices[10].z * NuTrigTable[angle];
            }
            vertices[5].w = 1.0f;
            NuVec4MtxTransformVU0(&vertices[5], &vertices[5], clip);
            f32 reciprocal = 1.0f / vertices[5].w;
            NuVec4ScaleXYZVU0(&vertices[5], &vertices[5], reciprocal);
            f32 jitter_x = 0.0f, jitter_y = 0.0f;
            if (1.0f - 1.5f * step > progress && NuLgtSeed != 0) {
                f32 random = (f32)((NuLgtRand() & 0xff) - 128);
                jitter_x = perpendicular.x * random * laser->wobble * reciprocal;
                jitter_y = perpendicular.y * random * laser->wobble * reciprocal;
            }
            vertices[0].x = vertices[5].x - perpendicular.x * reciprocal + jitter_x;
            vertices[0].y = vertices[5].y - perpendicular.y * reciprocal + jitter_y;
            vertices[0].z = vertices[5].z;
            vertices[2].x = perpendicular.x * reciprocal + vertices[5].x + jitter_x;
            vertices[2].y = perpendicular.y * reciprocal + vertices[5].y + jitter_y;
            vertices[2].z = vertices[5].z;
            if (!(vertices[0].x < -50.0f && vertices[1].x < 50.0f) &&
                !(vertices[0].x > 700.0f && vertices[1].x > 700.0f) &&
                !(vertices[0].y < -50.0f && vertices[1].y < -50.0f) &&
                !(vertices[0].y > 530.0f && vertices[1].y > 530.0f)) {
                NuLgtArcVertex(&vertices[0], laser->colour, u, v0);
                NuLgtArcVertex(&vertices[1], laser->colour, u1, v0);
                NuLgtArcVertex(&vertices[2], laser->colour, u, v1);
                NuLgtArcVertex(&vertices[1], laser->colour, u1, v0);
                NuLgtArcVertex(&vertices[3], laser->colour, u1, v1);
                NuLgtArcVertex(&vertices[2], laser->colour, u, v1);
            }
            vertices[1].x = vertices[0].x;
            vertices[1].y = vertices[0].y;
            vertices[1].z = vertices[0].z;
            vertices[3].x = vertices[2].x;
            vertices[3].y = vertices[2].y;
            vertices[3].z = vertices[2].z;
            progress = next;
        } while (1.0f > progress);
    }
    NuPrim2DEnd();
    NuPrimSetCoordinateSystem(NuPrimCoordSystemStack[--NuPrimCSPos]);
    NuRndrEndSceneEx(0);
    NuLgtArcLaserOldCnt = NuLgtArcLaserCnt;
    NuLgtArcLaserCnt = 0;
    if (!paused)
        ++NuLgtArcLaserFrame;
    NuScratchRelease();
}

void NuVpSetSourceRect(float, float, float, float) {
}

void NuFrameEndBgLoadPS(i32) {
}

void NuGCutRigidCalcMtx(NUGCUTRIGID_s *rigid, float frame, numtx_s *mtx) {
    if (rigid->animation == NULL) {
        *mtx = rigid->base_matrix;
        return;
    }
    if (*reinterpret_cast<u32 *>(rigid->animation) + 0xbeb1b6ccU < 2) {
        NuGCutRigidCalcMtx_3(rigid, frame, mtx);
        return;
    }

    nuanimdata2_s *animation = rigid->animation;
    nuanimcurve2_s *curves = animation->curves;
    u8 *curve_types = animation->curve_types;
    u8 flags = *animation->node_flags;
    nuanimtime_s time;
    NuAnimData2CalcTime(animation, frame, &time);

    auto evaluate = [&](u32 curve) {
        u8 type = curve_types[curve];
        return type == 0 ? curves[curve].data.constant : NuAnimCurve2CalcValEx(&curves[curve], &time, type);
    };

    if ((flags & 1) == 0) {
        NuMtxSetIdentity(mtx);
    } else {
        NUANGVEC angles = {
            static_cast<NUANG>(evaluate(3) * 10430.378f),
            static_cast<NUANG>(evaluate(4) * 10430.378f),
            static_cast<NUANG>(evaluate(5) * 10430.378f),
        };
        NuMtxSetRotateXYZ(mtx, &angles);
    }
    if ((flags & 8) != 0) {
        NUVEC scale = {evaluate(6), evaluate(7), evaluate(8)};
        NuMtxPreScale(mtx, &scale);
    }
    NUVEC translation = {evaluate(0), evaluate(1), evaluate(2)};
    NuMtxTranslate(mtx, &translation);
    mtx->m02 = -mtx->m02;
    mtx->m12 = -mtx->m12;
    mtx->m20 = -mtx->m20;
    mtx->m21 = -mtx->m21;
    mtx->m23 = -mtx->m23;
    mtx->m32 = -mtx->m32;
    NuMtxTranslate(mtx, reinterpret_cast<NUVEC *>(&rigid->base_matrix.m30));
}

// NuIOSDLMtlCallback is transcribed in android/nuiosdl_gl.cpp (original 0x29c480).

// original 0x2f87d0
void NuDisplayListCreate(nudisplayscene_s *raw_scene, variptr_u *buffer, variptr_u, i32 item_count, i32 material_count,
                         i32, i32, i32 sort_priority_count, i32, i32 allocate_materials) {
    NUDLDLISTSCENE *scene = raw_scene;
    scene->nitems = item_count;
    scene->nmtls = material_count;

    scene->items = reinterpret_cast<NUDISPLAYLISTITEM *>(ALIGN(buffer->addr, 0x10));
    buffer->addr = reinterpret_cast<usize>(scene->items + item_count);

    scene->mtls = reinterpret_cast<NUMTL **>(buffer->void_ptr);
    buffer->addr += material_count * sizeof(NUMTL *);
    scene->dlist_mtls = reinterpret_cast<NUDISPLAYLIST **>(ALIGN(buffer->addr, 0x10));
    buffer->addr = reinterpret_cast<usize>(scene->dlist_mtls + material_count);
    memset(scene->mtls, 0, material_count * sizeof(NUMTL *));
    memset(scene->dlist_mtls, 0, material_count * sizeof(NUDISPLAYLIST *));

    if (allocate_materials != 0) {
        NUMTL *materials = reinterpret_cast<NUMTL *>(ALIGN(buffer->addr, 0x10));
        buffer->addr = reinterpret_cast<usize>(materials + material_count);
        for (i32 i = 0; i < material_count; ++i) {
            scene->mtls[i] = &materials[i];
        }
    }

    NUDISPLAYLIST *display_lists = reinterpret_cast<NUDISPLAYLIST *>(ALIGN(buffer->addr, 0x10));
    buffer->addr = reinterpret_cast<usize>(display_lists + material_count);
    memset(display_lists, 0, material_count * sizeof(NUDISPLAYLIST));

    NUDISPLAYLISTITEM *first_items = reinterpret_cast<NUDISPLAYLISTITEM *>(ALIGN(buffer->addr, 0x10));
    buffer->addr = reinterpret_cast<usize>(first_items + material_count);
    memset(first_items, 0, material_count * sizeof(NUDISPLAYLISTITEM));

    for (i32 i = 0; i < material_count; ++i) {
        NUDISPLAYLIST *display_list = &display_lists[i];
        scene->dlist_mtls[i] = display_list;
        display_list->state = reinterpret_cast<NURNDRSTATE *>(ALIGN(buffer->addr, 4));
        buffer->addr = reinterpret_cast<usize>(display_list->state + 1);
        display_list->first = &first_items[i];

        display_list->state->mtl = NULL;
        display_list->state->tex_id = -1;
        display_list->state->global_id = -1;
        display_list->state->lights_id = -1;
        display_list->state->camera_id = -1;
        display_list->state->fog_id = -1;
        display_list->state->konst_id = -1;
        display_list->state->reflection_id = -1;

        display_list->first->type = 0x8d;
        display_list->first->id = 1;
        display_list->first->next = NULL;
    }
    scene->local_state = NULL;

    i32 used_size = ((((material_count + 7) >> 3) & ~0xf) + 0x10);
    scene->mtl_used[0] = reinterpret_cast<u8 *>(ALIGN(buffer->addr, 0x10));
    scene->mtl_used[1] = scene->mtl_used[0] + used_size;
    scene->nsort_pris = sort_priority_count;
    scene->sort_pris = reinterpret_cast<NUSORTPRI *>(scene->mtl_used[1] + used_size);
    buffer->addr = reinterpret_cast<usize>(scene->sort_pris + sort_priority_count);
    memset(scene->sort_pris, 0, sort_priority_count * sizeof(NUSORTPRI));

    for (i32 i = 0; i < sort_priority_count; ++i) {
        scene->sort_pris[i].display_scene = scene;
        scene->sort_pris[i].nmtls = 0;
        scene->sort_pris[i].mtl_first = 0;
    }

    for (i32 i = 0; i < material_count; ++i) {
        NUDISPLAYLIST *display_list = scene->dlist_mtls[i];
        display_list->scene_buffer = 0;
        display_list->mtl_last = display_list->first;
        display_list->scene_next = display_list->scene_first[0];
    }
    scene->flags &= 0xf1;
}

void NuFadeSetFxCodeMtls(nugeom_s *, unsigned char *) {
}

void NuGCutRigidCalcMtx_3(NUGCUTRIGID_s *rigid, float frame, numtx_s *mtx) {
    ani3_animheader_s *animation = reinterpret_cast<ani3_animheader_s *>(rigid->animation);
    f32 *values = NuAnimCurveExtractAllNodeCurves_3(animation, 0, frame, NULL);
    u8 flags = *animation->node_flags;
    if ((flags & 1) == 0) {
        NuMtxSetIdentity(mtx);
    } else {
        NUANGVEC angles;
        angles.x = static_cast<NUANG>(values[3] * 10430.378f);
        angles.y = static_cast<NUANG>(values[4] * 10430.378f);
        angles.z = static_cast<NUANG>(values[5] * 10430.378f);
        NuMtxSetRotateXYZ(mtx, &angles);
    }
    if ((flags & 8) != 0) {
        NUVEC scale = {values[6], values[7], values[8]};
        NuMtxPreScale(mtx, &scale);
    }
    NUVEC translation = {values[0], values[1], values[2]};
    NuMtxTranslate(mtx, &translation);
    mtx->m02 = -mtx->m02;
    mtx->m12 = -mtx->m12;
    mtx->m20 = -mtx->m20;
    mtx->m21 = -mtx->m21;
    mtx->m23 = -mtx->m23;
    mtx->m32 = -mtx->m32;
    NuMtxTranslate(mtx, reinterpret_cast<NUVEC *>(&rigid->base_matrix.m30));
}


void NuGCutSceneSysInitVfx(i32 (*)(char const *), i32 (*)(i32, VuMtx *), void (*)(i32), void (*)(i32, VuMtx *)) {
}

// NuIOSDLGeom2DCallback is transcribed in android/nuiosdl_gl.cpp (original 0x29d1a0).

i32 NuIOS_GetInAppProduct(i32, NuIOS_InAppProduct *) {
    return 0;
}






void NuGCutCharAnimProcess_3(NUGCUTCHAR_s *character, f32 frame, NUMTX *matrix, i32 *visible, u32 *animation_index,
                             f32 *animation_rate, f32 *blend_time, f32 *animation_start_frame, i32 *layer_mask) {
    ani3_animheader_s *animation = reinterpret_cast<ani3_animheader_s *>(character->animation);
    f32 *values = NuAnimCurveExtractAllNodeCurves_3(animation, 0, frame, NULL);
    const u16 curve_count = animation->curve_count;

    *visible = curve_count < 7 ? character->flags & 1 : GetIntCurveVal(animation, values, 6);
    if (animation_index != NULL) {
        *animation_index = curve_count < 8 ? character->animation_index : GetIntCurveVal(animation, values, 7);
    }
    if (animation_start_frame != NULL) {
        if (animation_index != NULL && *animation_index != 0 && *animation_index != 0xff) {
            *animation_start_frame = curve_count < 11 ? static_cast<f32>(character->animation_start_frame) : values[10];
        } else {
            *animation_start_frame = 0.0f;
        }
    }
    if (*visible == 0) {
        return;
    }
    if (layer_mask != NULL) {
        *layer_mask = curve_count < 12 ? -1 : GetIntCurveVal(animation, values, 11);
    }

    if ((animation->node_flags[0] & NUANIM_NODE_HAS_ROTATION) != 0) {
        NUANGVEC rotation = {
            static_cast<NUANG>(values[3] * 10430.378f),
            static_cast<NUANG>(values[4] * 10430.378f),
            static_cast<NUANG>(values[5] * 10430.378f),
        };
        NuMtxSetRotateXYZ(matrix, &rotation);
    } else {
        NuMtxSetIdentity(matrix);
    }
    NuMtxTranslate(matrix, reinterpret_cast<NUVEC *>(values));
    matrix->m02 = -matrix->m02;
    matrix->m12 = -matrix->m12;
    matrix->m20 = -matrix->m20;
    matrix->m21 = -matrix->m21;
    matrix->m23 = -matrix->m23;
    matrix->m32 = -matrix->m32;

    NUVEC scale = NuMtxGetScale(&character->base_matrix);
    NuMtxPreScale(matrix, &scale);
    if (animation_rate != NULL) {
        *animation_rate = curve_count < 10 ? character->animation_rate : values[9];
    }
    if (blend_time != NULL) {
        *blend_time = curve_count < 9 ? static_cast<f32>(character->blend_time) : values[8];
    }
}



i32 NuIOS_GetPurchaseResult() {
    return 0;
}

void NuLightMotionBlurEffect(i32, float) {
}

void NuIOS_DisplaySystemAlert(char const *) {
}

i32 NuIOS_IsProductPurchased(char *) {
    return 0;
}


void NuGCutRigidForceInstanced(NUGCUTSCENE_s *) {
}

void NuIOSDLReflectionCallback(void *) {
}

i32 NuIOS_GetInAppProductByID(char *, NuIOS_InAppProduct *) {
    return 0;
}

void NuIOS_GetShaderProgramKey(ShaderObjectKey const &) {
}

void NuSpecialFindByPlatformID(nugscn_s *, nuhspecial_s *, i32) {
}


void NuIOS_GetNumInAppPurchases() {
}

i32 NuIOS_PurchaseInAppProduct(char *) {
    return 0;
}

// Original @0x2ce760.
void NuHGobjEvalAnimBlend2Root_3(nugscn_s *scene, ani3_animheader_s *animation_a, f32 time_a,
                                 ani3_animheader_s *animation_b, f32 time_b, f32 blend, i32 override_count,
                                 NUJOINTANIM_s *overrides, NUMTX *matrices, NUHGOBJROOTFN root_fn, void *root_data) {
    nuhgobj_s *object = reinterpret_cast<nuhgobj_s *>(scene);
    nuanimbuff_s buffer;
    NUVEC root_a = {0.0f, 0.0f, 0.0f};
    NUVEC root_b = {0.0f, 0.0f, 0.0f};
    NUVEC root_translation = {0.0f, 0.0f, 0.0f};

    NuAnimBuffCreateScratch(&buffer);
    const i32 use_quaternions = NuAnimGetUseQuatsFlag() | ((animation_a->format_flags | animation_b->format_flags) &
                                                           ANI3_FORMAT_QUATERNION_ROTATION);
    NuAnimPushSetUseQuatsFlag(use_quaternions);
    NuAnimBuffAccumulate_3(&buffer, animation_a, time_a, 1, 0.0f, 0, object, &root_a);
    NuAnimBuffAccumulate_3(&buffer, animation_b, time_b, 0, blend, 0, object, &root_b);
    if (override_count != 0 && JointProcAnimFn != NULL) {
        JointProcAnimFn(&buffer, object, override_count, overrides);
    }
    NuAnimBuffEvaluate_3(&buffer, object, matrices, animation_a, root_fn, &root_translation, root_data);
    NuAnimBuffDestroyScratch(&buffer);
    NuAnimPopUseQuatsFlag();
}

i32 NuIOS_CanMakeInAppPurchases() {
    return 0;
}

void NuIOS_RestoreInAppPurchases() {
}


void NuCameraTransformScissorClip(nuvec_s *, nuvec_s *, i32, numtx_s *) {
}

// NuDebrisRendererFlushBuffers is transcribed in android/nuptl_android.c (original 0x296f35).


void NuIOS_CopyBackbufferToTexture(nunativetex_s *texture, bool) {
    texture->width = g_backingWidth;
    texture->height = g_backingHeight;
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nurenderthread.cpp", 0x128);
    glActiveTexture(GL_TEXTURE0);
    g_currentTexUnit = 0;
    glBindTexture(GL_TEXTURE_2D, texture->platform.gl_tex);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, g_backingWidth, g_backingHeight, 0);
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nurenderthread.cpp", 299);
}

i32 NuIOS_IsProductPurchasedByNum(i32) {
    return 0;
}

static i32 g_vaoRecordCount;

void NuIOS_ResetVAODuplicateFinder() {
    g_vaoRecordCount = 0;
}

void NuIOS_CateInAppPurchaseManager() {
}

void NuDynamicLightingGetParameterfv(nudeferredshadingenum_e, float *) {
}

i32 NuIOS_GetInAppProductIdentifier(i32, NuIOS_InAppProduct *) {
    return 0;
}

i32 NuIOS_PurchaseInAppProductByNum(i32) {
    return 0;
}

void NuIOSDLDeferredTransformCallback(void *) {
}

i32 NuIOS_AreInAppPurchasesAvailable() {
    return 0;
}

GLuint NuIOS_CreateGLTexFromPVRInMemory(void *data, i32 *out_width, i32 *out_height) {
    extern i32 comeFromHash;
    extern i32 g_loadDefaultTexture;
    extern i32 g_loadingCharacterInHub;

    static const GLenum cube_faces[6] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    };

    comeFromHash = 0;

    u8 *header = (u8 *)data;
    u8 *pixels = header + 0x34 + *(u32 *)(header + 0x30);
    const u32 pixel_format = *(u32 *)(header + 0x08);
    const u32 channel_bits = *(u32 *)(header + 0x0c);
    const i32 height = *(i32 *)(header + 0x18);
    const i32 width = *(i32 *)(header + 0x1c);
    const u32 depth = *(u32 *)(header + 0x20);
    const u32 surfaces = *(u32 *)(header + 0x24);
    const u32 faces = *(u32 *)(header + 0x28);
    const u32 mip_count = *(u32 *)(header + 0x2c);

    GLenum internal_format = 0;
    GLenum format = 0;
    GLenum type = 0;
    i32 bits_per_pixel = 0;
    bool compressed = false;

    if (channel_bits == 0) {
        static const GLenum compressed_formats[6] = {
            0x8c01, // GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
            0x8c03, // GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
            0x8c00, // GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
            0x8c02, // GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
            0x9137, // GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG
            0x9138, // GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG
        };
        if (pixel_format >= 6) {
            return 0;
        }
        internal_format = compressed_formats[pixel_format];
        bits_per_pixel = pixel_format < 2 ? 2 : 4;
        compressed = true;
    } else if (pixel_format == 0x61626772 && channel_bits == 0x08080808) { // "rgba", 8 bits each
        internal_format = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_BYTE;
        bits_per_pixel = 32;
    } else if (pixel_format == 0x61626772 && channel_bits == 0x04040404) { // "rgba", 4 bits each
        internal_format = GL_RGBA;
        format = GL_RGBA;
        type = GL_UNSIGNED_SHORT_4_4_4_4;
        bits_per_pixel = 16;
    }

    if (out_width != NULL) {
        *out_width = width;
    }
    if (out_height != NULL) {
        *out_height = height;
    }

    const GLenum texture_target = faces > 1 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GLuint texture = 0;
    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 0x2e7);
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    g_currentTexUnit = 0;
    glBindTexture(texture_target, texture);
    if (faces > 1) {
        g_lastBoundCubeTexIds[0] = texture;
    }
    glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, mip_count < 2 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (faces > 1) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp", 0x2fe);

    if (g_loadingCharacterInHub != 0 && bgProcIsBgThread() != 0) {
        NuIOS_YieldThread();
    }

    usize offset = 0;
    for (u32 mip = 0; mip < mip_count; ++mip) {
        const i32 mip_width = width >> mip;
        const i32 mip_height = height >> mip;
        usize mip_size = (usize)mip_width * (usize)mip_height * (usize)bits_per_pixel / 8;
        if (mip_size < 0x20) {
            mip_size = 0x20;
        }

        for (u32 surface = 0; surface < surfaces; ++surface) {
            for (u32 face = 0; face < faces; ++face) {
                const GLenum target = faces > 1 ? cube_faces[face] : GL_TEXTURE_2D;
                for (u32 z = 0; z < depth; ++z) {
                    BeginCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                           0x31d);
                    glActiveTexture(GL_TEXTURE0);
                    g_currentTexUnit = 0;
                    glBindTexture(texture_target, texture);

                    if (g_loadDefaultTexture == 0) {
                        if (compressed) {
                            glCompressedTexImage2D(target, mip, internal_format, mip_width, mip_height, 0, mip_size,
                                                   pixels + offset);
                        } else {
                            glTexImage2D(target, mip, internal_format, mip_width, mip_height, 0, format, type,
                                         pixels + offset);
                        }
                    } else {
                        loadDefaultTexture(texture, mip, mip_width, texture_target, target);
                    }
                    EndCriticalSectionGL("i:/SagaTouch-Android_9176564/nu2api.saga/nu3d/android/nutex_ios_ex.cpp",
                                         0x341);

                    if (bgProcIsBgThread() != 0) {
                        NuIOS_YieldThread();
                    }
                    offset += mip_size;
                }
            }
        }
    }

    return texture;
}


void NuDynamicLightTestShadowExtrusions(nudynamiclight_s *, _vuv_s const *, _vuv_s const *, i32 *) {
}


void NuRenderContextForceSamplerStatePS(i32, d3dsamplerstate_u const *) {
}

void NuGCutSceneRemapFocusIdToLocaterNum(NUGCUTSCENE_s *cutscene, variptr_u *buffer) {
    if (cutscene->version <= 4 || cutscene->camera_system == NULL ||
        cutscene->camera_system->focus_state_animation == NULL || cutscene->locator_system == NULL) {
        return;
    }

    buffer->addr = ALIGN(buffer->addr, 2);
    cutscene->focus_camera_indices = reinterpret_cast<u16 *>(buffer->void_ptr);
    NUGCUTLOCATORSYS_s *system = cutscene->locator_system;
    for (u32 i = 0; i < system->locator_count; ++i) {
        NUGCUTLOCATOR_s *locator = &system->locators[i];
        if ((system->types[locator->type_index].flags & 8) != 0) {
            *reinterpret_cast<u16 *>(buffer->void_ptr) = static_cast<u16>(i);
            buffer->void_ptr = reinterpret_cast<u16 *>(buffer->void_ptr) + 1;
        }
    }
}

void NuIOSDLDeferredTransformParamsCallback(void *) {
}


f32 NuATanf(f32 value) {
    return atanf(value);
}

f32 NuATan2f(f32 y, f32 x) {
    return atan2f(y, x);
}

void NuFntSave(nufnt_s *, i32, char *) {
}

extern "C" {
    u32 NuLgtSeed = 12345;
}

i32 NuLgtRand() {
    NuLgtSeed = (NuLgtSeed * 0x24cd + 1) & 0xffff;
    return NuLgtSeed;
}

void NuMemory::MemErrorHandler::CloseDump(NuMemoryManager *, u32) {
}

void NuMemory::MemErrorHandler::Dump(NuMemoryManager *, u32, char const *) {
}

void NuMemory::MemErrorHandler::HandleError(NuMemoryManager *, NuMemoryManager::ErrorCode, char const *) {
}

void NuMemory::MemErrorHandler::OpenDump(NuMemoryManager *, char const *, u32 &) {
}

void NuNetEmu::EmuPacket::AddPayload(void *, i32) {
}

NuNetEmu::EmuPacket::EmuPacket(nunetaddr_s *) {
}

NuNetEmu::EmuPacket::~EmuPacket() {
}

void NuNetEmu::PackStats::Draw(float, float, float, float, NetSmallStats::eInfo) const {
}

static __used__ void NuErrorFunction(char *, ...) {
}
