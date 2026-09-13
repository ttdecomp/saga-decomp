#include "legoapi/gizmos/object/gizbuildits.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/characters/core/playeritems.h"
#include "legoapi/audio/audio.h"
#include "legoapi/render/fx/parts.h"
#include "legoapi/misc/utilities.h"
#include "nu2api/nu3d/nuportal.h"
#include "legoapi/world/world.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "batman.h"
#include "decomp.h"
#include "globals.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/menus/core/gamehint.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/numtx.h"
#include <math.h>
#include <string.h>
#include "MechInputTouch/MechInputTouch_types.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/core/character.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/nu3d/nutex.h"

f32 GIZBUILDITWOBBLEJUMPHEIGHT = 0.05f;
i16 GizBuilditGDeb[6] = {0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51};
i32 gizbuildit_gizmotype_id = -1;
static f32 gizhopsfxwait = 0.0f;


namespace {

    enum : i32 {
        GIZBUILDIT_PROGRESS_CAPACITY = 64,
        GIZBUILDIT_PROGRESS_WORDS = GIZBUILDIT_PROGRESS_CAPACITY / 32,
    };

    struct GIZBUILDITPROGRESS_s {
        u32 completed[GIZBUILDIT_PROGRESS_WORDS];
        u32 active[GIZBUILDIT_PROGRESS_WORDS];
        u32 visible[GIZBUILDIT_PROGRESS_WORDS];
        u32 reward_released[GIZBUILDIT_PROGRESS_WORDS];
        u8 built_object_count[GIZBUILDIT_PROGRESS_CAPACITY];
    };

    DECOMP_ASSERT(sizeof(GIZBUILDITPROGRESS_s) == 0x60, "BuildIt progress ABI");

    static void ClearBuildItProgress(GIZBUILDITPROGRESS_s *progress) {
        if (progress == NULL) {
            return;
        }

        memset(progress, 0, sizeof(*progress));
        for (i32 word = 0; word < GIZBUILDIT_PROGRESS_WORDS; ++word) {
            progress->active[word] = 0xffffffff;
            progress->visible[word] = 0xffffffff;
        }
    }

    enum BUILDIT_GAMEPLAY_CONSTANTS : i32 {
        BUILDIT_HINT_ID = 0x25c,
        BUILDIT_SFX_COMPLETE = 0x3a,
        BUILDIT_SFX_PLACE_PIECE = 0x3b,
    };

    static const f32 BUILDIT_FINISH_DURATION = 0.6f;

    static GameObject_s *FindActiveBuilder(GIZBUILDIT_s *buildit) {
        if (LEGOCONTEXT_BUILDIT == -1) {
            return NULL;
        }

        for (i32 index = 0; index < 8; ++index) {
            GameObject_s *player = Player[index];
            if (player != NULL && static_cast<i8>(player->apiobj.field_0x1f8) < 0 &&
                player->build_context == LEGOCONTEXT_BUILDIT && player->field_0x788 == buildit) {
                return player;
            }
        }
        return NULL;
    }

    static NUVEC BuildItPieceEndPosition(GIZBUILDIT_s *buildit, GAMEANIMOBJ_s *object) {
        if (buildit->linked_buildit != NULL) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
            data->draw_mtx = data->end_mtx;
            return {data->draw_mtx.m30, data->draw_mtx.m31, data->draw_mtx.m32};
        }

        NuSpecialSetVisibility(&object->special, 1);
        if (object->instance_animation != NULL) {
            object->instance_animation->playing = 0;
            object->instance_animation->ltime = object->end_frame;
            EvalAnim2(&object->special, object->end_frame);
        }
        NUVEC *position = NuSpecialGetDrawPos(&object->special);
        return position != NULL ? *position : buildit->position;
    }

    static void EmitBuildItDebris(WORLDINFO *world, GIZBUILDIT_s *buildit) {
        for (i32 index = 0; index < buildit->anim_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            const i16 debris_type = GizBuilditGDeb[qrand() / 0x2aab];
            AddGameDebris(world->debris_sys, debris_type, NUMTX_GET_ROW_VEC(&data->end_mtx, 3));
        }
    }

} // namespace

void (*GizBuildIt_FinishFn)(GIZBUILDIT_s *) = NULL;
i32 (*GizBuildit_AutoBuildPosFn)(void *, NUVEC *, NUVEC *, u16 *) = NULL;

i32 (*GizBuildIt_CanStartBuildingFn)(GIZBUILDIT_s *, GameObject_s *);
GIZBUILDIT_s *GizBuildIt_FindNearest(WORLDINFO_s *, GameObject_s *, BUILDIT_FIND_ENUM, i32);

NuMechPtr<MechObjectInterface, 4> nextBuildit;


static NUVEC *GizmoBuildit_GetPos(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
        if (buildit != NULL) {
            return &buildit->position;
        }
    }
    return NULL;
}

static void GizBuildIts_EarlyUpdate(void *, void *data, float) {
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    if (buildit_sys == NULL) {
        return;
    }

    for (i32 index = 0; index < buildit_sys->count; ++index) {
        GIZBUILDIT_s &buildit = buildit_sys->buildits[index];
        buildit.availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_INTERACTING);
        buildit.builders_active = 0;
    }
}

static i32 GizBuildIts_GetMaxGizmos(void *buildit) {
    WORLDINFO *world = static_cast<WORLDINFO *>(buildit);
    if (world == NULL) {
        return 0;
    }
    return world->current_level->max_buildits;
}

static char *GizmoBuildit_GetGizmoName(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
        if (buildit != NULL) {
            return buildit->name;
        }
    }
    return NULL;
}

static i32 GizmoBuildit_GetOutput(GIZMO *gizmo, i32, i32 force_read) {
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
    if ((buildit->availability_flags & GIZBUILDIT_AVAILABILITY_LINKED) == GIZBUILDIT_AVAILABILITY_LINKED ||
        force_read != 0) {
        return buildit->build_state == GIZBUILDIT_BUILD_COMPLETE;
    }
    return 0;
}

static char *GizmoBuildit_GetOutputName(GIZMO *, i32) {
    return const_cast<char *>("Finished");
}

static i32 GizmoBuildit_GetNumOutputs(GIZMO *) {
    return 1;
}

static void GizBuildIts_PostLoad(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    if (buildit_sys == NULL) {
        return;
    }

    for (i32 index = 0; index < buildit_sys->count; ++index) {
        GIZBUILDIT_s &buildit = buildit_sys->buildits[index];
        if ((buildit.availability_flags & GIZBUILDIT_AVAILABILITY_HAS_BLOWUP_TYPE) != 0) {
            buildit.blowup_type = static_cast<i16>(GizmoBlowupGetTypeFromNameTableId(world, buildit.blowup_type));
            buildit.availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_HAS_BLOWUP_TYPE);
        }
    }
}

static void *GizBuildIts_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, sizeof(GIZBUILDITSYS_s)));

    buildit_sys->max_animation_objects = world->current_level->max_buildit_objs;
    buildit_sys->capacity = world->current_level->max_buildits;
    buildit_sys->buildits = static_cast<GIZBUILDIT_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, buildit_sys->capacity * sizeof(GIZBUILDIT_s)));
    buildit_sys->anim_pool = GameAnimSet_CreateObjectPool(&world->giz_buffer, &world->unknown_0108, 0xc8,
                                                          buildit_sys->max_animation_objects);
    buildit_sys->anim_objects = static_cast<GAMEANIMOBJ_s **>(GameBufferAlloc(
        &world->giz_buffer, &world->unknown_0108, buildit_sys->max_animation_objects * sizeof(GAMEANIMOBJ_s *)));

    for (i32 i = 0; i < buildit_sys->capacity; ++i) {
        buildit_sys->buildits[i].anim_set =
            GameAnimSet_Create(&world->giz_buffer, &world->unknown_0108, buildit_sys->anim_pool, world->game_anim_sys);
    }
    world->giz_buildit_sys = buildit_sys;
    return buildit_sys;
}

static void GizBuildIts_ClearProgress(void *, void *progress_ptr) {
    ClearBuildItProgress(static_cast<GIZBUILDITPROGRESS_s *>(progress_ptr));
}

static void *GizBuildIts_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(GIZBUILDITPROGRESS_s));
}

static void GizBuildIts_Draw(void *world_ptr, void *data, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    if (buildit_sys == NULL) {
        return;
    }

    for (i32 index = 0; index < buildit_sys->count; ++index) {
        GIZBUILDIT_s &buildit = buildit_sys->buildits[index];
        if ((buildit.availability_flags & GIZBUILDIT_AVAILABILITY_VISIBLE) == 0 ||
            (buildit.room_index >= 0 && world->rooms_visible_ptr[buildit.room_index] == 0)) {
            continue;
        }

        if ((buildit.state_flags & GIZBUILDIT_STATE_DISABLE_SHADOW_RENDERING) != 0) {
            EnableShadowMapRendering(0);
        } else {
            ResetShadowMapRendering();
        }

        if (buildit.linked_buildit == NULL) {
            ResetShadowMapRendering();
            if ((buildit.state_flags & GIZBUILDIT_STATE_DRAW_REFLECTION) != 0) {
                GameAnimSet_DrawReflection(buildit.anim_set, 2, buildit.field_0x54, NULL);
            }
            continue;
        }

        if (buildit.anim_set == NULL) {
            continue;
        }

        f32 reflection_plane = 0.0f;
        GAMEANIMOBJ_s *object = buildit.anim_set->objects;
        if ((buildit.state_flags & GIZBUILDIT_STATE_DRAW_REFLECTION) != 0 && object != NULL &&
            object->object_data != NULL) {
            GIZBUILDITANIMDATA_s *first_data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
            reflection_plane = first_data->start_mtx.m31 + buildit.field_0x54;
        }

        while (object != NULL) {
            GIZBUILDITANIMDATA_s *object_data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
            object_data->was_drawn = static_cast<u8>(NuSpecialDrawAt(&object->special, &object_data->draw_mtx));

            if ((buildit.state_flags & GIZBUILDIT_STATE_DRAW_REFLECTION) != 0 && (object->flags & 2) == 0) {
                NUMTX reflection_mtx;
                if (MatrixReflection(&object_data->draw_mtx, 2, reflection_plane, world->current_level->unknown_0cc,
                                     &reflection_mtx) != 0) {
                    NuSpecialDrawAt(&object->special, &reflection_mtx);
                }
            }
            object = object->next;
        }
    }
    ResetShadowMapRendering();
}

static void GizBuildIts_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *, void *data) {
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    if (buildit_sys != NULL) {
        if (buildit_sys->count != 0) {
            i32 i = 0;
            do {
                if (NuStrLen(buildit_sys->buildits[i].name) != 0) {
                    AddGizmo(gizmo_sys, type_id, NULL, &buildit_sys->buildits[i]);
                }
                ++i;
            } while (buildit_sys->count > i);
        }
    }
}

static void GizBuildIts_StoreProgress(void *, void *data, void *progress_ptr) {
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    GIZBUILDITPROGRESS_s *progress = static_cast<GIZBUILDITPROGRESS_s *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    ClearBuildItProgress(progress);
    i32 count = buildit_sys->count;
    if (count > GIZBUILDIT_PROGRESS_CAPACITY) {
        count = GIZBUILDIT_PROGRESS_CAPACITY;
    }

    for (i32 index = 0; index < count; ++index) {
        const GIZBUILDIT_s &buildit = buildit_sys->buildits[index];
        const i32 word = index >> 5;
        const u32 bit = 1u << (index & 31);
        if (buildit.build_state != GIZBUILDIT_BUILD_IDLE) {
            progress->completed[word] |= bit;
        }
        if ((buildit.availability_flags & GIZBUILDIT_AVAILABILITY_VISIBLE) == 0) {
            progress->visible[word] &= ~bit;
        }
        if ((buildit.availability_flags & GIZBUILDIT_AVAILABILITY_ACTIVE) == 0) {
            progress->active[word] &= ~bit;
        }
        if ((buildit.field_0x83 & GIZBUILDIT_RUNTIME_REWARD_RELEASED) != 0) {
            progress->reward_released[word] |= bit;
        }
        progress->built_object_count[index] = buildit.built_object_count;
    }
}

GIZBUILDIT_s *GizBuildIt_Find(WORLDINFO_s *world, char *name) {
    GIZBUILDITSYS_s *buildit_sys = world->giz_buildit_sys;
    if (buildit_sys != NULL && buildit_sys->count != 0) {
        GIZBUILDIT_s *buildit = buildit_sys->buildits;
        for (i32 i = 0; i < world->giz_buildit_sys->count; ++i, ++buildit) {
            if (NuStrICmp(buildit->name, name) == 0) {
                return buildit;
            }
        }
    }

    return NULL;
}

GIZBUILDIT_s *GizBuildIt_FindNearest(WORLDINFO_s *world, GameObject_s *player, BUILDIT_FIND_ENUM mode, i32 shadow) {
    GIZBUILDITSYS_s *system = world->giz_buildit_sys;
    if (system == NULL || player == NULL)
        return NULL;
    NUVEC position = player->apiobj.collision_position;
    const f32 bottom = player->apiobj.lower_position.y;
    const f32 top = player->apiobj.upper_position.y;
    GIZBUILDIT_s *nearest = NULL;
    f32 nearest_distance = 1000000.0f;
    GIZBUILDIT_s *buildit = system->buildits;
    for (i32 i = 0; i < world->giz_buildit_sys->count; ++i, ++buildit) {
        if (mode != BUILDIT_FIND_ANY) {
            if ((buildit->availability_flags & GIZBUILDIT_AVAILABILITY_VISIBLE) == 0 ||
                (buildit->availability_flags & GIZBUILDIT_AVAILABILITY_ACTIVE) == 0 ||
                ((buildit->state_flags & 0x80) != 0 && shadow == 0))
                continue;
            if (mode == BUILDIT_FIND_AVAILABLE) {
                if (buildit->build_state != 0 || LEGOCONTEXT_BUILDIT == -1)
                    continue;
                i32 occupied = 0;
                for (i32 p = 0; p < 8; ++p) {
                    if (Player[p] != NULL && Player[p]->build_context == LEGOCONTEXT_BUILDIT &&
                        Player[p]->field_0x788 == buildit)
                        occupied = 1;
                }
                if (occupied != 0)
                    continue;
            }
        }
        const f32 height = static_cast<f32>(static_cast<u32>(buildit->progress)) / 100.0f * buildit->bounds_radius;
        if (top < buildit->start_position.y - height || buildit->start_position.y + height < bottom)
            continue;
        const f32 radius = buildit->bounds_radius + 0.3f;
        const f32 distance = NuVecXZDistSqr(&buildit->start_position, &position, NULL);
        if (distance < radius * radius && distance < nearest_distance) {
            nearest_distance = distance;
            nearest = buildit;
        }
    }
    return nearest;
}

GIZBUILDIT_s *GizBuildIt_AnyReacting(WORLDINFO_s *world) {
    GIZBUILDITSYS_s *system = world->giz_buildit_sys;
    if (system != NULL) {
        GIZBUILDIT_s *buildit = system->buildits;
        for (i32 i = 0; i < system->count; ++i, ++buildit) {
            if ((buildit->availability_flags &
                 (GIZBUILDIT_AVAILABILITY_VISIBLE | GIZBUILDIT_AVAILABILITY_INTERACTING)) ==
                    (GIZBUILDIT_AVAILABILITY_VISIBLE | GIZBUILDIT_AVAILABILITY_INTERACTING) &&
                buildit->builders_active == 0 && (buildit->availability_flags & GIZBUILDIT_AVAILABILITY_ACTIVE) != 0) {
                return buildit;
            }
        }
    }
    return NULL;
}

i32 GizBuildIt_AtStart(GIZBUILDIT_s *buildit) {
    return buildit != NULL && buildit->builders_active == 0 && buildit->built_object_count == 0;
}

i32 GizBuildIt_AtEnd(GIZBUILDIT_s *buildit) {
    return buildit != NULL && buildit->build_state == GIZBUILDIT_BUILD_COMPLETE;
}

void GizBuildIt_TurnOff(GIZBUILDIT_s *buildit) {
    if (buildit == NULL) {
        return;
    }
    if (buildit->linked_buildit == NULL) {
        GameAnimSet_SetVisibility(buildit->anim_set, 0);
    }
    buildit->availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_VISIBLE);
}

void GizBuildIt_Finish(GIZBUILDIT_s *buildit) {
    LOG_INFO_IF(buildit->build_state != GIZBUILDIT_BUILD_COMPLETE, "build completed name=%s buildit=%p", buildit->name,
                (void *)buildit);
    buildit->build_state = GIZBUILDIT_BUILD_COMPLETE;
    if ((buildit->state_flags & GIZBUILDIT_STATE_TURN_OFF_WHEN_COMPLETE) != 0) {
        GizBuildIt_TurnOff(buildit);
    }
    if (GizBuildIt_FinishFn != NULL) {
        GizBuildIt_FinishFn(buildit);
    }
}

void GizBuildIt_SetHeadTarget(GIZBUILDIT_s *buildit, GameObject_s *player) {
    if (buildit->anim_object_count != 0) {
        u32 index = buildit->built_object_count;
        if (buildit->anim_object_count <= index)
            index = buildit->anim_object_count - 1;
        GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
        SetHeadTarget(player, reinterpret_cast<NUVEC *>(&data->start_mtx.m30), 0, 2.0f, 1.0f, 2.0f);
    }
}

void GizMoveAttractoBuildItPiece(GIZBUILDIT_s *buildit, GAMEANIMOBJ_s *piece) {
    f32 progress = 1.0f - buildit->step_timer / buildit->step_duration;
    GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(piece->object_data);
    NUMTX *draw = NuSpecialGetDrawMtx(&piece->special);
    if (progress > 0.0f) {
        NUMTX result;
        NUMTX from = *draw;
        from.m30 = from.m31 = from.m32 = 0.0f;
        NUMTX to = data->end_mtx;
        to.m30 = to.m31 = to.m32 = 0.0f;
        NUQUAT rotation, from_rotation, to_rotation;
        NuMtxToQuat(&from, &from_rotation);
        NuMtxToQuat(&to, &to_rotation);
        NuQuatSlerp(&rotation, &from_rotation, &to_rotation, progress);
        NuQuatToMtx(&rotation, &result);
        NUVEC delta = {data->end_mtx.m30 - draw->m30, data->end_mtx.m31 - draw->m31, data->end_mtx.m32 - draw->m32};
        delta.x *= progress;
        delta.y *= progress;
        delta.z *= progress;
        result.m30 = delta.x + draw->m30;
        result.m31 = delta.y + draw->m31;
        result.m32 = delta.z + draw->m32;
        result.m31 += 0.1f * NU_SIN_LUT(progress * 32768.0f);
        *draw = result;
    }
    if (buildit->linked_buildit != NULL) {
        data->draw_mtx = *draw;
    } else {
        NuSpecialSetDrawMtx(&piece->special, draw);
    }
}

void GizDrawBuildItPiece(GameObject_s *player, i32 draw_reflection) {
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit == NULL || buildit->builders_active == 0 || buildit->build_state != 0 ||
        buildit->anim_object_count == 0) {
        return;
    }
    f32 progress = 1.0f - buildit->step_timer / buildit->step_duration;
    GAMEANIMOBJ_s *object = buildit->anim_objects[buildit->built_object_count];
    GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
    NUMTX_ALIGNED16 matrix = data->start_mtx;
    if (progress > 0.0f) {
        NUMTX_ALIGNED16 start = data->start_mtx;
        NUMTX_ALIGNED16 end = data->end_mtx;
        start.m30 = start.m31 = start.m32 = 0.0f;
        end.m30 = end.m31 = end.m32 = 0.0f;
        NUQUAT from, to, rotation;
        NuMtxToQuat(&start, &from);
        NuMtxToQuat(&end, &to);
        NuQuatSlerp(&rotation, &from, &to, progress);
        NUMTX_ALIGNED16 interpolated;
        NuQuatToMtx(&rotation, &interpolated);
        interpolated.m30 = matrix.m30 + (data->end_mtx.m30 - matrix.m30) * progress;
        interpolated.m31 = matrix.m31 + (data->end_mtx.m31 - matrix.m31) * progress;
        interpolated.m32 = matrix.m32 + (data->end_mtx.m32 - matrix.m32) * progress;
        interpolated.m31 += 0.1f * NU_SIN_LUT(static_cast<i32>(32768.0f * progress));
        matrix = interpolated;
    }
    if (buildit->linked_buildit != NULL) {
        data->draw_mtx = matrix;
    } else {
        NuSpecialDrawAt(&object->special, &matrix);
        if (draw_reflection != 0) {
            extern i32 MatrixReflection(NUMTX *, i32, f32, f32, NUMTX *);
            NUMTX_ALIGNED16 reflection;
            if (MatrixReflection(&matrix, player->field_0x1087, player->field_0x1020, WORLD->current_level->unknown_0cc,
                                 &reflection) != 0) {
                NuRndrStartReflectionRender(0);
                NuSpecialDrawAt(&object->special, &reflection);
                NuRndrEndReflectionRender();
            }
        }
    }
}

void GizGetBuildItPlayerPos(GameObject_s *player, nuvec_s *position, nuvec_s *target) {
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit == NULL || buildit->anim_set == NULL || buildit->anim_object_count == 0) {
        return;
    }
    i32 count = buildit->anim_object_count;
    i32 index = buildit->built_object_count;
    if (index >= count) {
        index = count - 1;
    }
    if (position != NULL) {
        i32 batch_size = buildit->field_0x78;
        index = (index / batch_size) * batch_size;
        i32 batch_count = 0;
        while (batch_count < batch_size && index + batch_count < count) {
            ++batch_count;
        }
        NUVEC total = v000;
        i32 end = index + batch_count;
        for (; index < end; ++index) {
            buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            NuVecAdd(&total, &total, NUMTX_GET_ROW_VEC(&data->start_mtx, 3));
        }
        if (batch_count > 1) {
            NuVecScale(position, &total, 1.0f / batch_count);
        } else {
            *position = total;
        }
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        position->x = (position->x + buildit->start_position.x) * 0.5f;
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        position->z = (position->z + buildit->start_position.z) * 0.5f;
        if (index >= count) {
            index = count - 1;
        }
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
        f32 dx = position->x - data->end_mtx.m30;
        f32 dz = position->z - data->end_mtx.m32;
        f32 inverse_distance = 1.0f / NuFsqrt(dx * dx + dz * dz);
        position->x += dx * inverse_distance * player->apiobj.field_0x1dc;
        position->z += dz * inverse_distance * player->apiobj.field_0x1dc;
    }
    if (target != NULL) {
        buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
        GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
        *target = *NUMTX_GET_ROW_VEC(&data->end_mtx, 3);
    }
}

void GizBuildItPushAwayFromStart(GameObject_s *player, GIZBUILDIT_s *buildit) {
    NUVEC minimum = v000, maximum = v000;
    if (buildit == NULL || player == NULL || buildit->anim_set == NULL || buildit->anim_object_count == 0)
        return;
    i32 count = buildit->anim_object_count;
    NUVEC start_position = buildit->start_position;
    f32 player_x = player->apiobj.collision_position.x;
    f32 player_z = player->apiobj.collision_position.z;
    NUVEC centre;
    f32 radius;
    minimum.x = minimum.z = 1000000000.0f;
    maximum.x = maximum.z = -1000000000.0f;
    if (buildit->state_flags & 0x10) {
        f32 nearest_distance = 1000000000.0f;
        i32 nearest = 0;
        for (i32 i = 0; i < count; ++i) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[i]->object_data);
            f32 dx = data->start_mtx.m30 - player_x;
            f32 dz = data->start_mtx.m32 - player_z;
            f32 distance = dx * dx + dz * dz;
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = i;
                centre = *NUMTX_GET_ROW_VEC(&data->start_mtx, 3);
            }
        }
        NUVEC radius_centre;
        NuSpecialGetBounds(&buildit->anim_objects[nearest]->special, &minimum, &maximum);
        NuSpecialGetRadius(&buildit->anim_objects[nearest]->special, &radius_centre, &radius);
    } else {
        centre = start_position;
        for (i32 i = 0; i < count; ++i) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[i]->object_data);
            f32 dx = data->start_mtx.m30 - start_position.x;
            f32 dz = data->start_mtx.m32 - start_position.z;
            maximum.x = NuFmax(maximum.x, dx);
            maximum.z = NuFmax(maximum.z, dz);
            minimum.x = NuFmin(minimum.x, dx);
            minimum.z = NuFmin(minimum.z, dz);
        }
        radius = NuFsqrt(maximum.x * maximum.x + maximum.z * maximum.z);
    }
    maximum.x += centre.x;
    maximum.z += centre.z;
    minimum.x += centre.x;
    minimum.z += centre.z;
    PushAway(&centre, radius, &minimum, &maximum, player, NULL, 1.0f, 0);
}

void GizBuildItPushAwayFromEnd(GameObject_s *player) {
    if (WORLD->giz_buildit_sys == NULL)
        return;
    GIZBUILDIT_s *buildit = WORLD->giz_buildit_sys->buildits;
    f32 player_x = player->apiobj.collision_position.x;
    f32 player_z = player->apiobj.collision_position.z;
    f32 radius_squared = -1000000000.0f;
    i32 nearest = 0;
    for (i32 i = 0; i < WORLD->giz_buildit_sys->count; ++i, ++buildit) {
        NUVEC centre = v000;
        if (!(buildit->state_flags & 8) || !(buildit->availability_flags & 2) || buildit->built_object_count == 0)
            continue;
        NUVEC end_position = buildit->position;
        NUVEC minimum, maximum;
        minimum.x = minimum.z = 1000000000.0f;
        maximum.x = maximum.z = -1000000000.0f;
        f32 radius;
        if (buildit->state_flags & 0x10) {
            f32 nearest_distance = 1000000000.0f;
            for (i32 j = 0; j < buildit->built_object_count; ++j) {
                GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[j]->object_data);
                f32 dx = data->end_mtx.m30 - player_x;
                f32 dz = data->end_mtx.m32 - player_z;
                f32 distance = dx * dx + dz * dz;
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest = j;
                    centre = *NUMTX_GET_ROW_VEC(&data->end_mtx, 3);
                }
            }
            NUVEC radius_centre;
            NuSpecialGetBounds(&buildit->anim_objects[nearest]->special, &minimum, &maximum);
            NuSpecialGetRadius(&buildit->anim_objects[nearest]->special, &radius_centre, &radius);
        } else {
            centre.y = end_position.y;
            for (i32 j = 0; j < buildit->built_object_count; ++j) {
                GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[j]->object_data);
                centre.x += data->end_mtx.m30;
                centre.z += data->end_mtx.m32;
                f32 dx = data->end_mtx.m30 - end_position.x;
                f32 dz = data->end_mtx.m32 - end_position.z;
                maximum.x = NuFmax(maximum.x, dx);
                maximum.z = NuFmax(maximum.z, dz);
                minimum.x = NuFmin(minimum.x, dx);
                minimum.z = NuFmin(minimum.z, dz);
                f32 distance = dx * dx + dz * dz;
                if (distance > radius_squared)
                    radius_squared = distance;
            }
            centre.x /= buildit->built_object_count;
            centre.z /= buildit->built_object_count;
            radius = NuFsqrt(radius_squared);
        }
        maximum.x += centre.x;
        maximum.y = centre.y + 0.01f;
        maximum.z += centre.z;
        minimum.x += centre.x;
        minimum.y = centre.y;
        minimum.z += centre.z;
        PushAway(&centre, radius, &minimum, &maximum, player, NULL, 1.0f, 0);
    }
}

f32 GizBuildItMul(GameObject_s *player) {
    if (static_cast<i8>(player->apiobj.field_0x1f8) < 0 && Player_HasFastBuild(player)) {
        return 3.0f;
    }

    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    i32 build_amount = player->build_button_taps;
    const i32 completed_count = buildit->built_object_count;
    const i32 total_count = buildit->anim_object_count;
    if ((completed_count - build_amount + total_count) / 2 <= completed_count) {
        build_amount = total_count - completed_count;
    }
    if (build_amount < 0) {
        build_amount = 0;
    } else if (build_amount > 10) {
        build_amount = 10;
    }
    return static_cast<f32>(build_amount) / 10.0f + 1.0f;
}

void GizBuildIt_SetStepTime(GIZBUILDIT_s *buildit, GameObject_s *player) {
    buildit->step_duration = 0.3f;
    if (player != NULL) {
        buildit->step_duration /= GizBuildItMul(player);
    }
    buildit->step_timer = buildit->step_duration;
}

void GizBuildIt_SetToEnd(GIZBUILDIT_s *buildit) {
    if (buildit == NULL) {
        return;
    }

    buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_VISIBLE;
    if (buildit->linked_buildit == NULL) {
        GameAnimSet_SetVisibility(buildit->anim_set, 1);
        GameAnimSet_JumpToEnd(buildit->anim_set);
        GizBuildIt_SetStepTime(buildit, NULL);
    } else {
        for (i32 index = 0; index < buildit->anim_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            data->draw_mtx = data->start_mtx;
        }
    }

    buildit->built_object_count = buildit->anim_set != NULL ? buildit->anim_object_count : 0;
    buildit->builders_active = 0;
    buildit->availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_INTERACTING);
    GizBuildIt_Finish(buildit);
}

void GizBuildIt_SetToStart(GIZBUILDIT_s *buildit, i32 emit_debris, i32 keep_built_pieces) {
    if (buildit == NULL || buildit->anim_set == NULL) {
        return;
    }

    const u8 restored_built_count = keep_built_pieces != 0 ? buildit->built_object_count : 0;
    const i32 first_piece_to_reset = restored_built_count;
    buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_VISIBLE;

    if (emit_debris != 0) {
        for (i32 index = 0; index < buildit->built_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            NUVEC position = {data->end_mtx.m30, data->end_mtx.m31, data->end_mtx.m32};
            AddGameDebris(WORLD->debris_sys, GizBuilditGDeb[qrand() / 0x2aab], &position);
        }
    }

    if (buildit->linked_buildit != NULL) {
        for (i32 index = first_piece_to_reset; index < buildit->anim_object_count; ++index) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(buildit->anim_objects[index]->object_data);
            data->draw_mtx = data->start_mtx;
        }
    } else {
        GameAnimSet_SetVisibility(buildit->anim_set, 1);
        for (i32 index = first_piece_to_reset; index < buildit->anim_object_count; ++index) {
            GAMEANIMOBJ_s *object = buildit->anim_objects[index];
            object->instance_animation->playing = 0;
            object->instance_animation->ltime = object->start_frame;
        }
        GizBuildIt_SetStepTime(buildit, NULL);
    }

    buildit->builders_active = 0;
    buildit->built_object_count = restored_built_count;
    buildit->availability_flags &= static_cast<u8>(~GIZBUILDIT_AVAILABILITY_INTERACTING);
    buildit->build_state = GIZBUILDIT_BUILD_IDLE;
}

void ReleaseBuildIt(GameObject_s *player, i32 completed) {
    const i32 build_context = LEGOCONTEXT_BUILDIT;
    if (build_context == -1 || build_context != player->build_context) {
        return;
    }

    player->build_context = -1;
    const f32 completion_blend = completed != 0 ? 0.6f : 0.0f;
    GameCam_Blend(GameCam, 0.5f, completion_blend, 1);
}

static void GizBuildIts_LateUpdate(void *world_ptr, void *data, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    if (buildit_sys == NULL) {
        return;
    }

    if (gizhopsfxwait > 0.0f) {
        gizhopsfxwait -= FRAMETIME;
    }

    GIZBUILDIT_s *current = buildit_sys->buildits;
    for (i32 index = 0; index < buildit_sys->count; ++index, ++current) {
        GIZBUILDIT_s &buildit = *current;
        if ((buildit.availability_flags & 2) == 0 || (buildit.availability_flags & 1) == 0 ||
            buildit.anim_set == NULL || buildit.anim_object_count == 0) {
            continue;
        }

        if (buildit.build_state == GIZBUILDIT_BUILD_FINISHING) {
            buildit.step_timer += FRAMETIME;
            f32 finish_progress;
            if (buildit.step_timer >= BUILDIT_FINISH_DURATION) {
                Hint_SetComplete(LEGOHINT_BUILD);
                GizBuildIt_Finish(&buildit);
                NUVEC effect_position;
                NuVecAdd(&effect_position, &buildit.position, &buildit.effect_position);
                GameAudio_PlaySfx(BUILDIT_SFX_COMPLETE, &effect_position, 0, 0);
                NewRumbleAllPlayers(0.75f, 0.1f, 0, 0);
                GameCam_Judder(GameCam, -0.4f, 0, NULL);
                if ((buildit.field_0x83 & GIZBUILDIT_RUNTIME_REWARD_RELEASED) == 0) {
                    buildit.field_0x83 |= GIZBUILDIT_RUNTIME_REWARD_RELEASED;
                    const i32 hearts = ReleaseHearts();
                    if (buildit.field_0x5e != 0 || hearts != 0) {
                        NUVEC direction;
                        NuVecRotateX(&direction, &v010, static_cast<u16>(buildit.field_0x60));
                        NuVecRotateY(&direction, &direction, static_cast<u16>(buildit.field_0x62));
                        AddPickups(static_cast<u16>(buildit.field_0x5e), hearts, 0, 0, &effect_position, &direction,
                                   2.0f, -1, 1.75f, 2000000.0f, NULL, 1, 0, true);
                    }
                }
                if (buildit.blowup_type != -1) {
                    GizmoBlowUpTypeBlowUp(world, buildit.blowup_type, &buildit.position);
                    GameAnimSet_SetVisibility(buildit.anim_set, 0);
                }
                finish_progress = 1.0f;
            } else {
                finish_progress = buildit.step_timer / BUILDIT_FINISH_DURATION;
            }
            for (i32 piece_index = 0; piece_index < buildit.anim_object_count; ++piece_index) {
                GAMEANIMOBJ_s *piece = buildit.anim_objects[piece_index];
                GIZBUILDITANIMDATA_s *piece_data = static_cast<GIZBUILDITANIMDATA_s *>(piece->object_data);
                NUVEC position = buildit.linked_buildit != NULL ? *NUMTX_GET_ROW_VEC(&piece_data->end_mtx, 3)
                                                                : *NuSpecialGetPos(&piece->special);
                const f32 offset = 0.2f * NU_SIN_LUT(32768.0f * finish_progress);
                NUVEC displacement;
                if ((buildit.state_flags & GIZBUILDIT_STATE_ROTATING_WOBBLE) != 0) {
                    displacement.x = 0.0f;
                    displacement.y = 0.0f;
                    displacement.z = offset;
                    const i32 angle =
                        static_cast<i32>(static_cast<u32>(static_cast<u16>(buildit.field_0x5c)) << 16) / 360;
                    NuVecRotateY(&displacement, &displacement, angle);
                } else {
                    displacement.x = 0.0f;
                    displacement.y = offset;
                    displacement.z = 0.0f;
                }
                NuVecAdd(&position, &position, &displacement);
                if (buildit.linked_buildit != NULL) {
                    *NUMTX_GET_ROW_VEC(&piece_data->draw_mtx, 3) = position;
                } else {
                    NuSpecialSetDrawPos(&piece->special, &position);
                }
            }
            continue;
        }
        if (buildit.build_state != GIZBUILDIT_BUILD_IDLE) {
            continue;
        }

        {
            const bool automatic_build = (buildit.state_flags & GIZBUILDIT_STATE_AUTO_BUILD_STATIC_OBJECTS) != 0;
            if (!automatic_build && buildit.builders_active == 0) {
                GizBuildIt_SetStepTime(&buildit, NULL);
                if (buildit.linked_buildit == NULL) {
                    GAMEANIMOBJ_s *next_piece = buildit.anim_objects[buildit.built_object_count];
                    NuSpecialSetVisibility(&next_piece->special, 1);
                }
                goto idle_wobble;
            }

            if (automatic_build && GizBuildit_AutoBuildPosFn != NULL) {
                NUVEC centre;
                u16 facing;
                if (GizBuildit_AutoBuildPosFn(world, &buildit.start_position, &centre, &facing) != 0) {
                    f32 radius;
                    u16 angle;
                    if (buildit.step_timer > 4.5f) {
                        radius = 0.5f;
                        angle = facing - 0x4000;
                    } else {
                        const f32 phase = ((4.5f - buildit.step_timer) / 4.5f) * 16384.0f + 32768.0f;
                        const f32 progress = 1.0f + NU_SIN_LUT(phase + 16384.0f);
                        angle = static_cast<i32>(static_cast<f32>(static_cast<i32>(facing) - 0x4000) +
                                                 (((5.0f * progress) * 360.0f) * 65536.0f) / 360.0f);
                        radius = 0.5f - progress * 0.3f;
                    }
                    const u16 angle_step =
                        buildit.anim_object_count != 0
                            ? static_cast<i32>(65536.0f / static_cast<f32>(buildit.anim_object_count))
                            : 0;
                    for (i32 piece_index = buildit.anim_object_count - 1; piece_index >= buildit.built_object_count;
                         --piece_index) {
                        angle += angle_step;
                        NUVEC target = centre;
                        target.x += radius * NU_COS_LUT(angle);
                        target.z += radius * NU_SIN_LUT(angle);
                        GAMEANIMOBJ_s *orbit_piece = buildit.anim_objects[piece_index];
                        NUVEC *position = NuSpecialGetDrawPos(&orbit_piece->special);
                        target.x = SeekValF(position->x, target.x, 6.0f);
                        target.y = SeekValF(position->y, target.y, 6.0f);
                        target.z = SeekValF(position->z, target.z, 6.0f);
                        if (buildit.linked_buildit != NULL) {
                            GIZBUILDITANIMDATA_s *piece_data =
                                static_cast<GIZBUILDITANIMDATA_s *>(orbit_piece->object_data);
                            *NUMTX_GET_ROW_VEC(&piece_data->draw_mtx, 3) = target;
                        } else {
                            NuSpecialSetDrawPos(&orbit_piece->special, &target);
                            NuSpecialUpdate(&orbit_piece->special);
                        }
                    }
                }
            }
            GameObject_s *builder = automatic_build ? NULL : FindActiveBuilder(&buildit);
            const f32 previous_step_timer = buildit.step_timer;
            buildit.step_timer -= FRAMETIME;
            GAMEANIMOBJ_s *piece = buildit.anim_objects[buildit.built_object_count];

            if (buildit.step_timer > 0.0f) {
                if (previous_step_timer == buildit.step_duration) {
                    NUVEC *position = NuSpecialGetDrawPos(&piece->special);
                    const i16 debris_type = GizBuilditGDeb[qrand() / 0x2aab];
                    AddGameDebris(world->debris_sys, debris_type, position);
                }
                if (automatic_build) {
                    GizMoveAttractoBuildItPiece(&buildit, piece);
                } else if (buildit.linked_buildit == NULL) {
                    NuSpecialSetVisibility(&piece->special, 0);
                }
                goto idle_wobble;
            }

            NUVEC piece_position = BuildItPieceEndPosition(&buildit, piece);
            GameAudio_PlaySfx(BUILDIT_SFX_PLACE_PIECE, &piece_position, 0, 0);
            if (automatic_build && buildit.linked_buildit == NULL) {
                NuSpecialSetDrawPos(&piece->special, NuSpecialGetPos(&piece->special));
            }
            ++buildit.built_object_count;

            if (buildit.built_object_count == buildit.anim_object_count) {
                buildit.build_state = GIZBUILDIT_BUILD_FINISHING;
                buildit.step_timer = 0.0f;
                EmitBuildItDebris(world, &buildit);
                if (builder != NULL) {
                    NewBuzz(builder->pad_gamepad->pad, 0.1f, 0);
                }
            } else if (buildit.built_object_count < buildit.anim_object_count) {
                GizBuildIt_SetStepTime(&buildit, builder);
                if (builder != NULL) {
                    NewBuzzFrames(builder->pad_gamepad->pad, 1, 0);
                }
            }

            if (builder != NULL) {
                if (buildit.build_state == GIZBUILDIT_BUILD_IDLE &&
                    ((builder->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0 ||
                     (builder->apiobj.field_0x1f4 & 0x40000) != 0)) {
                    builder->field_0xe21 ^= 0x40;
                    if (builder->build_button_taps < 10) {
                        ++builder->build_button_taps;
                    }
                } else {
                    ReleaseBuildIt(builder, buildit.build_state);
                }
            }
        }
    idle_wobble:
        if (buildit.build_state != GIZBUILDIT_BUILD_IDLE ||
            (buildit.state_flags & GIZBUILDIT_STATE_AUTO_BUILD_STATIC_OBJECTS) != 0 ||
            ((buildit.state_flags & GIZBUILDIT_STATE_DISABLE_WOBBLE_WITHOUT_SHADOWS) != 0 && ShadowMode == 0)) {
            continue;
        }
        f32 probability = 0.1f - static_cast<f32>(buildit.anim_object_count - buildit.built_object_count) * 0.002f;
        if (probability < 0.01f)
            probability = 0.01f;
        if ((buildit.availability_flags & 4) == 0)
            probability *= 0.1f;
        for (i32 piece_index = 0; piece_index < buildit.anim_object_count; ++piece_index) {
            if (buildit.linked_buildit != NULL && piece_index < buildit.built_object_count)
                continue;
            GAMEANIMOBJ_s *piece = buildit.anim_objects[piece_index];
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(piece->object_data);
            NUMTX matrix = buildit.linked_buildit != NULL ? data->start_mtx : *NuSpecialGetMtx(&piece->special);
            if (data->wobble_time > 0.0f) {
                data->wobble_time -= FRAMETIME;
                if (data->wobble_time <= 0.0f && gizhopsfxwait <= 0.0f &&
                    ((buildit.availability_flags & 4) != 0 || qrand() <= 0x7fff)) {
                    GameAudio_PlaySfx(0x3c, &buildit.start_position, 0, 0);
                    gizhopsfxwait = (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) * 0.2f + 0.1f;
                }
            } else if (piece_index >= buildit.built_object_count &&
                       (buildit.linked_buildit != NULL ? data->was_drawn != 0
                                                       : NuSpecialGetOnScreenFn(&piece->special) != 0)) {
                if (probability > static_cast<f32>(qrand()) * (1.0f / 65535.0f)) {
                    data->wobble_time = 0.2f;
                    data->wobble_axis = qrand() / 0x2aab;
                }
            }
            if (data->wobble_time > 0.0f) {
                const f32 phase = data->wobble_time / 0.2f;
                i32 angle = static_cast<i32>(3640.0f * NU_SIN_LUT((1.0f - phase) * 65536.0f));
                if ((data->wobble_axis & 1) != 0)
                    angle = -angle;
                NUVEC axis, normal;
                if (data->wobble_axis <= 1)
                    NuMtxGetXAxis(&matrix, &axis);
                else if (data->wobble_axis <= 3)
                    NuMtxGetYAxis(&matrix, &axis);
                else
                    NuMtxGetZAxis(&matrix, &axis);
                NuVecNorm(&normal, &axis);
                const f32 vertical = fabsf(NuVecDot(&normal, &v010));
                const f32 factor = 1.0f - (1.0f - NU_SIN_LUT(vertical * 16384.0f + 16384.0f)) * vertical;
                angle = static_cast<i32>(static_cast<f32>(angle) * factor);
                if (data->wobble_axis <= 1)
                    NuMtxPreRotateX(&matrix, angle);
                else if (data->wobble_axis <= 3)
                    NuMtxPreRotateY(&matrix, angle);
                else
                    NuMtxPreRotateZ(&matrix, angle);
                matrix.m31 += (NU_SIN_LUT(phase * 32768.0f) * GIZBUILDITWOBBLEJUMPHEIGHT) * buildit.interaction_radius;
            }
            if (buildit.linked_buildit != NULL) {
                data->draw_mtx = matrix;
            } else {
                NuSpecialSetDrawMtx(&piece->special, &matrix);
                NuSpecialUpdate(&piece->special);
            }
        }
    }
}

void GizBuildit_SetVisibility(GIZBUILDIT_s *buildit, i32 visible) {
    if (buildit == NULL) {
        return;
    }

    GameAnimSet_SetVisibility(buildit->anim_set, visible);
    buildit->availability_flags = static_cast<u8>((buildit->availability_flags & ~GIZBUILDIT_AVAILABILITY_VISIBLE) |
                                                  (visible != 0 ? GIZBUILDIT_AVAILABILITY_VISIBLE : 0));
}

static void GizmoBuildit_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo != NULL) {
        GizBuildit_SetVisibility(static_cast<GIZBUILDIT_s *>(gizmo->object), visible);
    }
}

void GizBuildit_Reset(GIZBUILDIT_s *buildit, void *world_ptr) {
    buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_ACTIVE | GIZBUILDIT_AVAILABILITY_VISIBLE;
    buildit->field_0x83 &= static_cast<u8>(~GIZBUILDIT_RUNTIME_REWARD_RELEASED);

    buildit->start_position = buildit->file_position;
    buildit->position = buildit->file_position;
    buildit->bounds_radius = NuTrigTable[0x3000];

    if (buildit->anim_set != NULL && buildit->anim_set->objects != NULL) {
        NUVEC start_min = {1000000000.0f, 1000000000.0f, 1000000000.0f};
        NUVEC start_max = {-1000000000.0f, -1000000000.0f, -1000000000.0f};
        NUVEC end_min = start_min;
        NUVEC end_max = start_max;

        for (GAMEANIMOBJ_s *object = buildit->anim_set->objects; object != NULL; object = object->next) {
            GIZBUILDITANIMDATA_s *data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
            memset(data, 0, sizeof(*data));

            NUMTX evaluated;
            EvalAnim(&object->special, object->start_frame, &evaluated, 1);
            data->start_mtx = evaluated;
            if (buildit->linked_buildit != NULL) {
                data->start_mtx.m30 -= buildit->linked_buildit->file_position.x;
                data->start_mtx.m31 -= buildit->linked_buildit->file_position.y;
                data->start_mtx.m32 -= buildit->linked_buildit->file_position.z;
                NuMtxRotateY(&data->start_mtx, buildit->field_0x7c);
                data->start_mtx.m30 += buildit->file_position.x;
                data->start_mtx.m31 += buildit->file_position.y;
                data->start_mtx.m32 += buildit->file_position.z;
                data->draw_mtx = data->start_mtx;
            }

            EvalAnim(&object->special, object->end_frame, &evaluated, 1);
            data->end_mtx = evaluated;
            if (buildit->linked_buildit != NULL) {
                data->end_mtx.m30 -= buildit->linked_buildit->file_position.x;
                data->end_mtx.m31 -= buildit->linked_buildit->file_position.y;
                data->end_mtx.m32 -= buildit->linked_buildit->file_position.z;
                NuMtxRotateY(&data->end_mtx, buildit->field_0x7c);
                data->end_mtx.m30 += buildit->file_position.x;
                data->end_mtx.m31 += buildit->file_position.y;
                data->end_mtx.m32 += buildit->file_position.z;
            }

            if (data->start_mtx.m30 < start_min.x)
                start_min.x = data->start_mtx.m30;
            if (data->start_mtx.m31 < start_min.y)
                start_min.y = data->start_mtx.m31;
            if (data->start_mtx.m32 < start_min.z)
                start_min.z = data->start_mtx.m32;
            if (data->start_mtx.m30 > start_max.x)
                start_max.x = data->start_mtx.m30;
            if (data->start_mtx.m31 > start_max.y)
                start_max.y = data->start_mtx.m31;
            if (data->start_mtx.m32 > start_max.z)
                start_max.z = data->start_mtx.m32;

            if (data->end_mtx.m30 < end_min.x)
                end_min.x = data->end_mtx.m30;
            if (data->end_mtx.m31 < end_min.y)
                end_min.y = data->end_mtx.m31;
            if (data->end_mtx.m32 < end_min.z)
                end_min.z = data->end_mtx.m32;
            if (data->end_mtx.m30 > end_max.x)
                end_max.x = data->end_mtx.m30;
            if (data->end_mtx.m31 > end_max.y)
                end_max.y = data->end_mtx.m31;
            if (data->end_mtx.m32 > end_max.z)
                end_max.z = data->end_mtx.m32;

            data->wobble_time = 0.2f;
        }

        buildit->start_position.x = (start_min.x + start_max.x) * 0.5f;
        buildit->start_position.y = (start_min.y + start_max.y) * 0.5f;
        buildit->start_position.z = (start_min.z + start_max.z) * 0.5f;
        buildit->position.x = (end_min.x + end_max.x) * 0.5f;
        buildit->position.y = (end_min.y + end_max.y) * 0.5f;
        buildit->position.z = (end_min.z + end_max.z) * 0.5f;

        f32 span = start_max.x - start_min.x;
        if (span < start_max.z - start_min.z) {
            span = start_max.z - start_min.z;
        }
        buildit->bounds_radius = span * NuTrigTable[0x3000];
    }

    if (buildit->field_0x5c == 0) {
        buildit->state_flags &= static_cast<u16>(~GIZBUILDIT_STATE_ROTATING_WOBBLE);
    } else {
        buildit->state_flags |= GIZBUILDIT_STATE_ROTATING_WOBBLE;
    }
    buildit->build_state = GIZBUILDIT_BUILD_IDLE;
    buildit->built_object_count = 0;

    if (world_ptr != NULL) {
        WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
        buildit->room_index = world->current_gscn != NULL
                                  ? static_cast<i16>(NuPortalWhichRoom(world->current_gscn, &buildit->file_position))
                                  : -1;
    }
}

static i32 GizBuildIts_Load(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    const u8 version = static_cast<u8>(EdFileReadChar());
    const u16 buildit_count = static_cast<u16>(EdFileReadShort());
    static_cast<GIZBUILDITSYS_s *>(data)->count = buildit_count;
    if (buildit_count == 0) {
        return 1;
    }

    char linked_name[16];
    char blowup_name[32];
    i32 anim_object_index = 0;
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDITSYS_s *>(data)->buildits;
    for (i32 buildit_index = 0; buildit_index < static_cast<GIZBUILDITSYS_s *>(data)->count;
         ++buildit_index, ++buildit) {
        EdFileRead(buildit->name, sizeof(buildit->name));
        EdFileReadNuVec(&buildit->file_position);
        GizmoFileReadGameAnimSet(buildit->anim_set, world, NULL, version, "GizBuildit", buildit->name);

        buildit->interaction_radius = EdFileReadFloat();
        if (version <= 6) {
            (void)EdFileReadFloat();
        }
        buildit->field_0x5e = EdFileReadShort();
        buildit->field_0x5c = EdFileReadShort();
        buildit->field_0x78 = static_cast<u8>(EdFileReadChar());
        buildit->progress = static_cast<u8>(EdFileReadChar());
        if (version >= 6) {
            buildit->field_0x54 = EdFileReadFloat();
        }

        buildit->blowup_type = -1;
        if (version > 6) {
            if (version == 7) {
                buildit->blowup_type = EdFileReadShort();
            } else {
                const i32 blowup_name_length = static_cast<i8>(EdFileReadChar());
                if (blowup_name_length != 0) {
                    EdFileRead(blowup_name, blowup_name_length);
                    buildit->blowup_type = static_cast<i16>(GizmoBlowupGetNameTableId(blowup_name));
                    if (buildit->blowup_type != -1) {
                        buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_HAS_BLOWUP_TYPE;
                    }
                }
            }

            buildit->field_0x60 = EdFileReadShort();
            buildit->field_0x62 = EdFileReadShort();
            EdFileReadNuVec(&buildit->effect_position);
        }

        if (version <= 8) {
            buildit->activation_radius =
                world->area != NULL && (world->area->flags & AREAFLAG_VEHICLE_AREA) != 0 ? 12.0f : 1.75f;
        } else {
            buildit->activation_radius = EdFileReadFloat();
        }

        if (version <= 3) {
            buildit->state_flags = static_cast<i16>(static_cast<i8>(EdFileReadChar()));
            if (version > 1 && EdFileReadChar() != 0) {
                buildit->state_flags |= GIZBUILDIT_STATE_AUTO_BUILD_STATIC_OBJECTS;
            }
            if (version == 3 && EdFileReadChar() != 0) {
                buildit->state_flags |= 0x80;
            }
        } else {
            buildit->state_flags = static_cast<u16>(EdFileReadShort());
        }

        if (version > 4) {
            buildit->field_0x7c = static_cast<u16>(EdFileReadShort());
            if (EdFileReadChar() != 0) {
                EdFileRead(linked_name, sizeof(linked_name));
                for (i32 linked_index = 0; linked_index < static_cast<GIZBUILDITSYS_s *>(data)->count; ++linked_index) {
                    if (NuStrICmp(static_cast<GIZBUILDITSYS_s *>(data)->buildits[linked_index].name, linked_name) ==
                        0) {
                        buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_LINKED;
                        buildit->linked_buildit = &static_cast<GIZBUILDITSYS_s *>(data)->buildits[linked_index];
                        break;
                    }
                }
            }
        }

        buildit->anim_object_count = 0;
        buildit->anim_objects = &static_cast<GIZBUILDITSYS_s *>(data)->anim_objects[anim_object_index];
        for (GAMEANIMOBJ_s *object = buildit->anim_set->objects; object != NULL; object = object->next) {
            if ((object->animation != NULL && object->instance_animation != NULL) ||
                (buildit->state_flags & GIZBUILDIT_STATE_AUTO_BUILD_STATIC_OBJECTS) != 0) {
                buildit->anim_objects[buildit->anim_object_count++] = object;
                ++anim_object_index;
            }
        }

        GizBuildit_Reset(buildit, world);
    }

    return 1;
}

static void GizBuildIts_Reset(void *world_ptr, void *data, void *progress_ptr) {
    GIZBUILDITSYS_s *buildit_sys = static_cast<GIZBUILDITSYS_s *>(data);
    GIZBUILDITPROGRESS_s *progress = static_cast<GIZBUILDITPROGRESS_s *>(progress_ptr);

    for (i32 index = 0; index < buildit_sys->count; ++index) {
        GIZBUILDIT_s &buildit = buildit_sys->buildits[index];
        GizBuildit_Reset(&buildit, world_ptr);

        if (index >= GIZBUILDIT_PROGRESS_CAPACITY || progress == NULL) {
            continue;
        }

        const i32 word = index >> 5;
        const u32 bit = 1u << (index & 31);
        buildit.build_state =
            (progress->completed[word] & bit) != 0 ? GIZBUILDIT_BUILD_COMPLETE : GIZBUILDIT_BUILD_IDLE;
        buildit.availability_flags =
            static_cast<u8>((buildit.availability_flags & ~GIZBUILDIT_AVAILABILITY_LINKED) |
                            ((progress->active[word] & bit) != 0 ? GIZBUILDIT_AVAILABILITY_ACTIVE : 0) |
                            ((progress->visible[word] & bit) != 0 ? GIZBUILDIT_AVAILABILITY_VISIBLE : 0));
        buildit.field_0x83 =
            static_cast<u8>((buildit.field_0x83 & ~GIZBUILDIT_RUNTIME_REWARD_RELEASED) |
                            ((progress->reward_released[word] & bit) != 0 ? GIZBUILDIT_RUNTIME_REWARD_RELEASED : 0));
        buildit.built_object_count = progress->built_object_count[index];
    }
}

static void GizmoBuildit_Activate(GIZMO *gizmo, i32 active) {
    if (gizmo == NULL) {
        return;
    }

    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(gizmo->object);
    buildit->availability_flags = static_cast<u8>((buildit->availability_flags & ~GIZBUILDIT_AVAILABILITY_ACTIVE) |
                                                  (active != 0 ? GIZBUILDIT_AVAILABILITY_ACTIVE : 0));
    if (active == 0) {
        return;
    }

    if ((buildit->state_flags & GIZBUILDIT_STATE_REISSUE_REWARD_ON_ACTIVATE) != 0) {
        buildit->field_0x83 &= static_cast<u8>(~GIZBUILDIT_RUNTIME_REWARD_RELEASED);
    }
    if (buildit->linked_buildit == NULL) {
        GameAnimSet_JumpToStart(buildit->anim_set);
    }

    const u8 reward_released = buildit->field_0x83 & GIZBUILDIT_RUNTIME_REWARD_RELEASED;
    GizBuildit_Reset(buildit, NULL);
    buildit->field_0x83 =
        static_cast<u8>((buildit->field_0x83 & ~GIZBUILDIT_RUNTIME_REWARD_RELEASED) | reward_released);

    if ((buildit->state_flags & GIZBUILDIT_STATE_AUTO_BUILD_STATIC_OBJECTS) == 0 || GizBuildit_AutoBuildPosFn == NULL) {
        return;
    }

    NUVEC scatter_position;
    if (GizBuildit_AutoBuildPosFn(NULL, &buildit->start_position, &scatter_position, NULL) == 0) {
        return;
    }

    buildit->step_timer = 5.0f;
    for (i32 index = 0; index < buildit->anim_object_count; ++index) {
        scatter_position.x += NuFloatRand(NULL) / 5.0f - 0.1f;
        scatter_position.y += NuFloatRand(NULL) / 5.0f - 0.1f;
        scatter_position.z += NuFloatRand(NULL) / 5.0f - 0.1f;

        GAMEANIMOBJ_s *object = buildit->anim_objects[index];
        if (buildit->linked_buildit == NULL) {
            NuSpecialSetDrawPos(&object->special, &scatter_position);
        } else {
            GIZBUILDITANIMDATA_s *object_data = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data);
            object_data->draw_mtx.m30 = scatter_position.x;
            object_data->draw_mtx.m31 = scatter_position.y;
            object_data->draw_mtx.m32 = scatter_position.z;
        }
        AddGameDebris(WORLD->debris_sys, GizBuilditGDeb[qrand() / 0x2aab], &scatter_position);
    }
}

i32 GizBuildIts_UpdateHint(HINT_s *hint) {
    if (hint->completion_flags[MechInputTouchSystem::s_baseControlMode] != 0)
        return 0;
    return GizBuildIt_AnyReacting(WorldInfo_CurrentlyActive()) != NULL;
}

void GizBuildIt_KillParts(GIZBUILDIT_s *buildit) {
    if (buildit == NULL)
        return;
    NUVEC velocity = v000;
    if (buildit->anim_set == NULL)
        return;
    for (GAMEANIMOBJ_s *object = buildit->anim_set->objects; object != NULL; object = object->next) {
        ADDPART_s params = Default_ADDPART;
        NUMTX_ALIGNED16 matrix = static_cast<GIZBUILDITANIMDATA_s *>(object->object_data)->end_mtx;
        params.matrix = &matrix;
        params.velocity = &velocity;
        params.special = &object->special;
        params.stop_fn = PartStop_Flickerer;
        params.draw_fn = PartDraw_Flickerer;
        params.field_3c = PartImpact_Brick;
        params.field_14 = 0.2f;
        params.field_18 = 0.2f;
        params.gravity = -4.0f;
        params.flags = 0x90;
        params.field_c4 = 1;
        params.time_step = FRAMETIME;
        PART_s *part = AddPart(&params);
        if (part != NULL) {
            f32 random = static_cast<f32>(qrand()) * 1.5259021893143654e-05f;
            part->field_100 = random + random + 7.0f;
        }
    }
}

u32 GizBuildIts_TotalScore(void *world) {
    GIZBUILDITSYS_s *system = static_cast<WORLDINFO_s *>(world)->giz_buildit_sys;
    u32 total = 0;
    if (system != NULL) {
        GIZBUILDIT_s *item = system->buildits;
        if (item != NULL) {
            for (i32 i = 0; i < system->count; ++i, ++item)
                total += item->completion_score;
        }
    }
    return total;
}

void CalcAveragePosAndRad(GIZBUILDIT_s &buildit, VuVec &position, float &radius, bool include_built) {
    u32 first = 0;
    if (!include_built)
        first = buildit.built_object_count;
    position = VuVec(0, 0, 0, 1);
    radius = 0.0f;
    i32 count = 0;
    for (u32 i = first; i < buildit.anim_object_count; ++i) {
        GAMEANIMOBJ_s *object = buildit.anim_objects[i];
        if (object != NULL) {
            NUMTX *matrix = NuSpecialGetMtx(&object->special);
            ++count;
            position.x += matrix->m30;
            position.y += matrix->m31;
            position.z += matrix->m32;
        }
    }
    position.x /= static_cast<f32>(count);
    position.y /= static_cast<f32>(count);
    position.z /= static_cast<f32>(count);
    f32 maximum = 0.0f;
    for (u32 i = first; i < buildit.anim_object_count; ++i) {
        GAMEANIMOBJ_s *object = buildit.anim_objects[i];
        if (object != NULL) {
            NUMTX *matrix = NuSpecialGetMtx(&object->special);
            const f32 x = position.x - matrix->m30;
            const f32 z = position.z - matrix->m32;
            const f32 distance = x * x + 0.0f + z * z;
            if (maximum <= distance)
                maximum = distance;
        }
    }
    if (maximum > 0.0f)
        maximum = NuFsqrt(maximum);
    radius = maximum;
    if (buildit.radius_scale > 0.0f)
        radius = maximum * buildit.radius_scale;
}

MechObjectInterface *GIZBUILDIT_s::GetMechObjectInterface() {
    if (mech_object_interface != NULL)
        return mech_object_interface;
    new GizBuildItObjectInterface(*this);
    return mech_object_interface;
}

void GIZBUILDIT_s::ClearMechObjectInterface() {
    if (mech_object_interface != NULL)
        delete mech_object_interface;
}

ADDGIZMOTYPE *GizBuildIts_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizBuildit";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0x60;
    addtype.fns.early_update_fn = GizBuildIts_EarlyUpdate;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizBuildIts_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizmoBuildit_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizBuildIts_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = GizBuildIts_LateUpdate;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = GizBuildIts_Draw;
    addtype.fns.get_gizmo_name_fn = GizmoBuildit_GetGizmoName;
    addtype.fns.get_output_fn = GizmoBuildit_GetOutput;
    addtype.fns.get_output_name_fn = GizmoBuildit_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizmoBuildit_GetNumOutputs;
    addtype.fns.activate_fn = GizmoBuildit_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = GizmoBuildit_SetVisibility;
    addtype.fns.allocate_progress_data_fn = GizBuildIts_AllocateProgressData;
    addtype.fns.clear_progress_fn = GizBuildIts_ClearProgress;
    addtype.fns.store_progress_fn = GizBuildIts_StoreProgress;
    addtype.fns.reset_fn = GizBuildIts_Reset;
    addtype.fns.reserve_buffer_space_fn = GizBuildIts_ReserveBufferSpace;
    addtype.fns.load_fn = GizBuildIts_Load;
    addtype.fns.post_load_fn = GizBuildIts_PostLoad;
    addtype.fns.add_level_sfx_fn = NULL;
    gizbuildit_gizmotype_id = type_id;

    return &addtype;
}

void ForceBuildItToUseNext(GIZBUILDIT_s &buildit) {
    nextBuildit = NuMechPtr<MechObjectInterface, 4>(buildit.GetMechObjectInterface());
}

void BuildIt_MoveCode(GameObject_s *player) {
    if (LEGOCONTEXT_BUILDIT == -1)
        return;
    if ((player->apiobj.character_data->model_flags & 0x2010) != 0)
        return;
    if (player->build_context != LEGOCONTEXT_BUILDIT) {
        if (LEGOACT_BUILD == -1 || player->apiobj.character_model->model_data_b[LEGOACT_BUILD] == NULL)
            return;
        if (AnimPlaying(&player->apiobj.anim_packet, LEGOACT_BUILD, 1, 1) != NULL)
            return;
        if (player->apiobj.field_0x27d == 0)
            return;
        if (ObjLandReady(player) == 0 && objInNetWaitContext(player, LEGOCONTEXT_BUILDIT) == 0)
            return;
        if (player->touch_task != NULL && player->touch_task->GetHashId().value != MechTouchTaskBuildIt::HashId.value)
            return;
        GIZBUILDIT_s *buildit = nextBuildit.Get() != NULL ? nextBuildit->GetGizBuildit() : NULL;
        if (buildit == NULL)
            buildit = GizBuildIt_FindNearest(WORLD, player, BUILDIT_FIND_AVAILABLE, ShadowMode);
        if (objInNetWaitContext(player, LEGOCONTEXT_BUILDIT) != 0) {
            player->context_animation_timer -= FRAMETIME;
            if (player->context_animation_timer <= 0.0f) {
                player->build_context = -1;
                player->big_jump_data = NULL;
            }
        }
        if (buildit == NULL || static_cast<i8>(player->apiobj.flags_low) >= 0)
            return;
        if (player->apiobj.field_0x27d != 0 && (buildit->state_flags & 4) != 0) {
            GizBuildItPushAwayFromStart(player, buildit);
        }
        GizBuildIt_SetHeadTarget(buildit, player);
        buildit->availability_flags |= GIZBUILDIT_AVAILABILITY_INTERACTING;
        if ((player->pad_gamepad->buttons_pressed & GAMEPAD_SPECIAL) != 0 ||
            (objInNetWaitContext(player, LEGOCONTEXT_BUILDIT) != 0 &&
             (player->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0)) {
            if (GizBuildIt_CanStartBuildingFn != NULL && GizBuildIt_CanStartBuildingFn(buildit, player) == 0)
                return;
            nextBuildit.Reset();
            player->context_animation_timer = 0.4f;
            player->field_0x788 = buildit;
            player->build_button_taps = 0;
            player->build_context = LEGOCONTEXT_BUILDIT;
            player->context_animation = LEGOACT_BUILD;
            player->field_0xe21 &= ~0x40;
            GizBuildIt_SetStepTime(buildit, player);
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
            Hint_SetComplete(LEGOHINT_BUILD);
        }
        if (static_cast<i8>(player->apiobj.flags_low) < 0 && player->apiobj.field_0x27d != 0) {
            GizBuildItPushAwayFromEnd(player);
        }
        return;
    }
    if (BoltSys->stop_targeting != NULL)
        BoltSys->stop_targeting(player, &player->apiobj.collision_position);
    GIZBUILDIT_s *buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit == NULL)
        return;
    GizBuildIt_SetHeadTarget(buildit, player);
    buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if ((buildit->state_flags & 0x80) != 0 && ShadowMode == 0) {
        player->build_context = -1;
        buildit->builders_active = 1;
        static_cast<GIZBUILDIT_s *>(player->field_0x788)->availability_flags |= GIZBUILDIT_AVAILABILITY_INTERACTING;
    }
    const i32 animation = player->context_animation;
    if (player->apiobj.character_model->model_data_b[animation] != NULL &&
        AnimPlaying(&player->apiobj.anim_packet, animation, 1, 0) == NULL)
        return;
    buildit = static_cast<GIZBUILDIT_s *>(player->field_0x788);
    if (buildit->build_state == 0 && (player->pad_gamepad->buttons_held & GAMEPAD_SPECIAL) != 0 &&
        player->context_animation_timer < 0.2f) {
        player->context_animation_timer = 0.2f;
    } else {
        player->context_animation_timer -= FRAMETIME;
        if (player->context_animation_timer <= 0.0f) {
            player->build_context = -1;
            return;
        }
    }
    if (player->build_context != -1) {
        buildit->builders_active = 1;
        static_cast<GIZBUILDIT_s *>(player->field_0x788)->availability_flags |= GIZBUILDIT_AVAILABILITY_INTERACTING;
    }
}
