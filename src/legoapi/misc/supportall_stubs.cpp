#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/characters/core/character.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nu3d/android/nuptl_android.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"

void DisplayListGenerateTransforms(nudisplayscene_s *) {
}

void bgprocIsFrozen() {
}

void DisplayListCreateGeomItemPS(variptr_u *, void *, numtl_s *) {
}

void DisplayListCreateInstSurfGeomPS(variptr_u *, numtx_s *) {
}

// Flag-sensitive moves from supportall.cpp (-O2): these match at the
// default flag. GetBuffer stays a call and float scheduling matches.
void *RndrStateBuildKonstState(nuglobalrndrstate_s *state) {
    VARIPTR *buffer = NuDisplayListGetBuffer();
    f32 *konst = static_cast<f32 *>(buffer->void_ptr);
    f32 *result = konst;

    if (state->const_tint_enabled == 0) {
        konst[0] = 1.0f;
        konst[1] = 1.0f;
        konst[2] = 1.0f;
    } else {
        konst[0] = state->const_tint.r;
        konst[1] = state->const_tint.g;
        konst[2] = state->const_tint.b;
    }
    konst[3] = state->const_alpha_enabled == 0 ? 1.0f : state->const_alpha;
    buffer->addr += sizeof(f32) * 4;
    return result;
}
void RootFnEx(NUMTX *matrix, void *data, NUVEC *sampled_root, NUVEC *, NUVEC *translation, f32, i32 include_y) {
    APIOBJECT *object = static_cast<APIOBJECT *>(data);

    if (object->previous_animation_root_time > object->anim_packet.current_time) {
        object->previous_animation_root = *sampled_root;
    }

    object->animation_root_delta.x = sampled_root->x - object->previous_animation_root.x;
    object->animation_root_delta.y = include_y ? sampled_root->y - object->previous_animation_root.y : 0.0f;
    object->animation_root_delta.z = sampled_root->z - object->previous_animation_root.z;
    NuVecMtxRotate(&object->animation_root_delta, &object->animation_root_delta, &object->field_0xb8);
    if (!include_y) {
        object->animation_root_delta.y = 0.0f;
    }

    object->previous_animation_root = *sampled_root;
    object->previous_animation_root_time = object->anim_packet.current_time;

    if (object->anim_packet.requested_animation != -1) {
        CHARACTERANIM_s *animation = static_cast<CHARACTERANIM_s *>(
            object->character_model->model_data_a[object->anim_packet.requested_animation]);
        matrix->m30 = translation->x + animation->root_translation.x;
        matrix->m32 = translation->z + animation->root_translation.z;
        if (include_y) {
            matrix->m31 = translation->y + animation->root_translation.y;
        }
    } else {
        matrix->m30 = translation->x;
        if (include_y) {
            matrix->m31 = translation->y;
        }
        matrix->m32 = translation->z;
    }

    if (include_y && object->animation_root_delta.y == 0.0f) {
        object->animation_root_delta.y = 1.0e-11f;
    } else if (object->animation_root_delta.x == 0.0f && object->animation_root_delta.z == 0.0f) {
        object->animation_root_delta.x = 1.0e-11f;
    }
}
