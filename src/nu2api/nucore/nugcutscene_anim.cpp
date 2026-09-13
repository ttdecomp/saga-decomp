#include "nu2api/nucore/nugcutscene.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/numath/numtx.h"

void NuGCutLocatorCalcMtx_3(NUGCUTLOCATOR_s *locator, numtx_s *mtx, float frame) {
    ani3_animheader_s *animation = reinterpret_cast<ani3_animheader_s *>(locator->animation);
    f32 *values = NuAnimCurveExtractAllNodeCurves_3(animation, 0, frame, NULL);
    if ((*animation->node_flags & 1) == 0) {
        NuMtxSetIdentity(mtx);
    } else {
        NUANGVEC angles = {
            static_cast<NUANG>(values[3] * 10430.378f),
            static_cast<NUANG>(values[4] * 10430.378f),
            static_cast<NUANG>(values[5] * 10430.378f),
        };
        NuMtxSetRotateXYZ(mtx, &angles);
    }
    NUVEC translation = {values[0], values[1], values[2]};
    NuMtxTranslate(mtx, &translation);
    mtx->m02 = -mtx->m02;
    mtx->m12 = -mtx->m12;
    mtx->m20 = -mtx->m20;
    mtx->m21 = -mtx->m21;
    mtx->m23 = -mtx->m23;
    mtx->m32 = -mtx->m32;
    NuMtxTranslate(mtx, reinterpret_cast<NUVEC *>(&locator->base_matrix.m30));
}

extern "C" i32 NuGCutLocatorCalcMtx(NUGCUTLOCATOR_s *locator, float frame, NUMTX *mtx, nuanimtime_s *time) {
    if (locator->animation == NULL) {
        *mtx = locator->base_matrix;
        return 0;
    }
    if (*reinterpret_cast<u32 *>(locator->animation) + 0xbeb1b6ccU < 2) {
        NuGCutLocatorCalcMtx_3(locator, mtx, frame);
        return 0;
    }
    nuanimdata2_s *animation = locator->animation;
    auto evaluate = [&](u32 curve) {
        u8 type = animation->curve_types[curve];
        return type == 0 ? animation->curves[curve].data.constant
                         : NuAnimCurve2CalcValEx(&animation->curves[curve], time, type);
    };
    if ((*animation->node_flags & 1) == 0) {
        NuMtxSetIdentity(mtx);
    } else {
        NUANGVEC angles = {
            static_cast<NUANG>(evaluate(3) * 10430.378f),
            static_cast<NUANG>(evaluate(4) * 10430.378f),
            static_cast<NUANG>(evaluate(5) * 10430.378f),
        };
        NuMtxSetRotateXYZ(mtx, &angles);
    }
    NUVEC translation = {evaluate(0), evaluate(1), evaluate(2)};
    NuMtxTranslate(mtx, &translation);
    mtx->m02 = -mtx->m02;
    mtx->m12 = -mtx->m12;
    mtx->m20 = -mtx->m20;
    mtx->m21 = -mtx->m21;
    mtx->m23 = -mtx->m23;
    mtx->m32 = -mtx->m32;
    NuMtxTranslate(mtx, reinterpret_cast<NUVEC *>(&locator->base_matrix.m30));
    return 0;
}

i32 NuGCutLocatorIsVisble_3(NUGCUTLOCATOR_s *locator, float frame, float *scale, float *rate) {
    static char locator_scale_filter[] = {6, 7, 8, static_cast<char>(0xff)};
    static char locator_filter[] = {6, 7, static_cast<char>(0xff)};
    static char sfx_filter[] = {3, static_cast<char>(0xff)};
    ani3_animheader_s *animation = reinterpret_cast<ani3_animheader_s *>(locator->animation);
    char *filter = animation->curve_count == 4 ? sfx_filter
                                               : (animation->curve_count == 8 ? locator_filter : locator_scale_filter);
    f32 *values = NuAnimCurveExtractAllNodeCurves_3(animation, 0, frame, filter);
    if (animation->curve_count == 4) {
        f32 visible = values[3];
        return static_cast<i32>(visible < 0.0f ? visible - 0.5f : visible + 0.5f);
    }
    f32 visible = values[7];
    i32 result = static_cast<i32>(visible < 0.0f ? visible - 0.5f : visible + 0.5f);
    if (result != 0 && scale != NULL) {
        *scale = values[6];
    }
    if (rate != NULL) {
        *rate = animation->curve_count < 9 ? 1.0f : values[8];
    }
    return result;
}

extern "C" i32 NuGCutLocatorIsVisble(NUGCUTLOCATOR_s *locator, float frame, nuanimtime_s *time, float *scale,
                                     float *rate) {
    if (locator->animation == NULL) {
        const i32 visible = locator->flags & 8;
        if (scale != NULL && visible != 0) {
            *scale = locator->locator_scale;
        }
        return visible;
    }
    if (*reinterpret_cast<u32 *>(locator->animation) + 0xbeb1b6ccU < 2) {
        return NuGCutLocatorIsVisble_3(locator, frame, scale, rate);
    }
    nuanimdata2_s *animation = locator->animation;
    if (rate != NULL) {
        *rate = 1.0f;
    }
    nuanimcurve2_s *curves = animation->curves;
    u8 *curve_types = animation->curve_types;
    if (animation->curve_count == 4) {
        const i8 type = curve_types[3];
        if (type == 0) {
            return static_cast<i32>(curves[3].data.constant);
        }
        return static_cast<i32>(NuAnimCurve2CalcValEx(&curves[3], time, static_cast<u32>(type)));
    }
    const i8 visible_type = curve_types[7];
    i32 visible;
    if (visible_type == 0) {
        visible = static_cast<i32>(curves[7].data.constant);
    } else {
        visible = static_cast<i32>(NuAnimCurve2CalcValEx(&curves[7], time, static_cast<u32>(visible_type)));
    }
    if (visible != 0 && scale != NULL) {
        const i8 scale_type = curve_types[6];
        *scale = scale_type == 0 ? curves[6].data.constant
                                 : NuAnimCurve2CalcValEx(&curves[6], time, static_cast<u32>(scale_type));
    }
    return visible;
}
