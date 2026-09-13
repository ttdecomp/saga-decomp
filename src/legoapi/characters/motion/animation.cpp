#include "decomp.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/nu3d/nutex.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

i32 RedirectAnim(char *path, ANIMREDIRECT *redirects, ANIMLIST_s *animation_list, char *directory) {
    CHARACTERANIM_s *animation = reinterpret_cast<CHARACTERANIM_s *>(animation_list);
    for (ANIMREDIRECT *redirect = redirects; redirect->name != NULL; ++redirect) {
        if (redirect->animation_id == animation->animation_id && NuStrICmp(redirect->name, animation->name) == 0) {
            NuStrCpy(path, directory);
            NuStrCat(path, animation->name);
            animation->flags &= ~CHARACTER_ANIMATION_FLAG_BSA;
            return 1;
        }
    }
    return 0;
}

extern "C" {
    extern i16 id_JEDISTARFIGHTERYELLOWEP3;
    extern i16 id_JEDISTARFIGHTERREDEP3;
    extern i16 id_TIEINTERCEPTOR;
}

i32 NeedsPretendAnim(GameObject_s *object) {
    return object->apiobj.character_model->model_data_b[object->apiobj.anim_packet.requested_animation] == NULL ||
           object->id == id_JEDISTARFIGHTERREDEP3 || object->id == id_JEDISTARFIGHTERYELLOWEP3 ||
           object->id == id_TIEINTERCEPTOR;
}

extern void *NuGScnBufferAllocAligned(i32 size, i32 alignment);

namespace {
    struct LegacyInstanceAnimationLink {
        NUMTX matrix;
        u8 pad_40[8];
        nuinstanim_s *animation;
    };

    DECOMP_ASSERT(sizeof(LegacyInstanceAnimationLink) == 0x4c, "legacy instance animation link ABI");
    DECOMP_ASSERT(offsetof(LegacyInstanceAnimationLink, animation) == 0x48, "legacy instance animation pointer offset");
} // namespace

void ReadInstAnimBlock(i32 file, nugscn_s *scene) {
    scene->num_instance_animations = static_cast<i16>(NuFileReadInt(file));
    const i32 indexed_format = NuFileReadInt(file);
    scene->instance_animations = static_cast<nuinstanim_s *>(NuMemFileAddr(file));

    if (indexed_format == 0) {
        nuinstanim_s *animation = scene->instance_animations;
        if (animation != NULL) {
            for (i32 instance_index = 0; instance_index < scene->num_instances; ++instance_index) {
                LegacyInstanceAnimationLink *instance =
                    reinterpret_cast<LegacyInstanceAnimationLink *>(scene->instances + instance_index * 0x50);
                if (instance->animation == NULL) {
                    continue;
                }

                instance->animation = animation;
                animation->mtx = instance->matrix;
                animation->instance_ix = static_cast<u16>(instance_index);
                ++animation;
            }
        }
    } else if (scene->instance_animations != NULL && scene->num_instance_animations > 0) {
        for (i32 animation_index = 0; animation_index < scene->num_instance_animations; ++animation_index) {
            nuinstanim_s *animation = &scene->instance_animations[animation_index];
            LegacyInstanceAnimationLink *instance =
                reinterpret_cast<LegacyInstanceAnimationLink *>(scene->instances + animation->instance_ix * 0x50);
            instance->animation = animation;
            animation->mtx = instance->matrix;
        }
    }

    if (scene->num_instance_animations != 0) {
        scene->instance_animation_matrices = static_cast<NUMTX *>(
            NuGScnBufferAllocAligned(static_cast<i32>(scene->num_instance_animations) * sizeof(NUMTX), 0x10));
    }
}

void ReadInstAnimBlockDlist(i32 file, nugscn_s *scene) {
    NUDLDLISTSCENE *display_list = scene->display_list;
    scene->num_instance_animations = static_cast<i16>(NuFileReadInt(file));
    NuFileReadInt(file);
    scene->instance_animations = static_cast<nuinstanim_s *>(NuMemFileAddr(file));

    if (scene->instance_animations != NULL && display_list->nspecials > 0) {
        NUDISPLAYSPECIAL *specials = static_cast<NUDISPLAYSPECIAL *>(display_list->specials);
        for (i32 special_index = 0; special_index < display_list->nspecials; ++special_index) {
            NUDISPLAYSPECIAL *special = &specials[special_index];
            const isize animation_index = reinterpret_cast<isize>(special->instance_animation);
            if (animation_index == -1) {
                special->instance_animation = NULL;
                continue;
            }

            nuinstanim_s *animation = &scene->instance_animations[animation_index];
            special->instance_animation = animation;
            animation->mtx = special->instance_mtx;
            animation->instance_ix = static_cast<u16>(special_index);
        }
    }

    if (scene->num_instance_animations != 0) {
        scene->instance_animation_matrices = static_cast<NUMTX *>(
            NuGScnBufferAllocAligned(static_cast<i32>(scene->num_instance_animations) * sizeof(NUMTX), 0x10));
    }
}

void EvaluateJointOrientationMtx(nugscn_s *scene, i32 joint_index, numtx_s *matrix) {
    nuhgobj_s *object = reinterpret_cast<nuhgobj_s *>(scene);
    NuMtxSetIdentity(matrix);

    while (true) {
        nuhgobjjoint_s *joint = &object->joints[joint_index];
        NUMTX *bind_matrix = &joint->animation_bind_matrix;
        if (bind_matrix->m00 != 1.0f || bind_matrix->m11 != 1.0f || bind_matrix->m22 != 1.0f) {
            NuMtxMulVU0(matrix, bind_matrix, matrix);
        }

        if (joint->parent_index == 0xff) {
            break;
        }
        joint_index = joint->parent_index;
    }

    matrix->m02 = -matrix->m02;
    matrix->m12 = -matrix->m12;
    matrix->m20 = -matrix->m20;
    matrix->m21 = -matrix->m21;
    matrix->m23 = -matrix->m23;
    matrix->m32 = -matrix->m32;
    NuMtxTransposeR(matrix, matrix);
}
