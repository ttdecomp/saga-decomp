#include "legoapi/gizmos/traps/shards.h"

#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/world/level.h"
#include <string.h>
#include "globals.h"
#include "legoapi/items/objects/gameobjects.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/numtx.h"

i32 qrand();
void Attracto_GetSuctionPos(GameObject_s *, NUVEC *);
f32 SeekValF(f32, f32, f32);
void NewBuzzFrames(nupad_s *, i32, i32);

struct SHARDPROGRESS_s {
    u32 collected[4];
    u32 active[4];
    u32 visible[4];
};
DECOMP_ASSERT(sizeof(SHARDPROGRESS_s) == 0x30, "Shard progress ABI");

static i32 Shards_GetMaxGizmos(void *world_info) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_info);
    return world != NULL ? world->current_level->max_shards : 0;
}

static void Shards_AddGizmos(GIZMOSYS *gizmo_sys, i32 type, void *context, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(context);
    for (i32 i = 0; i < world->shard_count; ++i) {
        SHARD *shard = &static_cast<SHARD *>(world->shards)[i];
        if (NuStrLen(shard->name) != 0)
            AddGizmo(gizmo_sys, type, NULL, &static_cast<SHARD *>(world->shards)[i]);
    }
}

static void Shards_UpdateBeforeCharacters(void *world_info, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_info);
    for (i32 i = 0; i < world->shard_count; ++i) {
        static_cast<SHARD *>(world->shards)[i].state_flags &= ~0x20;
    }
}

static void Shards_UpdateAfterCharacters(void *context, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(context);
    SHARD *shard = static_cast<SHARD *>(world->shards);
    for (i32 i = 0; i < world->shard_count; ++i, ++shard) {
        if (shard->state_flags & 2)
            NuCameraTransformScreenClip(&shard->screen_position, &shard->current_position, 1, NULL);
        if (shard->state_flags & 4) {
            if (shard->collector == NULL) {
                shard->state_flags &= ~0x2c;
                shard->collection_time = 0.0f;
                continue;
            }
            NUVEC target, velocity;
            Attracto_GetSuctionPos(shard->collector, &target);
            NuVecSub(&velocity, &target, &shard->current_position);
            NuVecScale(&velocity, &velocity, 3.0f);
            shard->collection_time += FRAMETIME;
            f32 acceleration;
            if (shard->collection_time < 1.0f)
                acceleration = 5.0f;
            else if (shard->collection_time < 2.0f)
                acceleration = (shard->collection_time - 1.0f) * 5.0f + 5.0f;
            else
                acceleration = 10.0f;
            shard->collection_velocity.x = SeekValF(shard->collection_velocity.x, velocity.x, acceleration);
            shard->collection_velocity.y = SeekValF(shard->collection_velocity.y, velocity.y, acceleration);
            shard->collection_velocity.z = SeekValF(shard->collection_velocity.z, velocity.z, acceleration);
            shard->current_position.x += shard->collection_velocity.x * FRAMETIME;
            shard->current_position.y += shard->collection_velocity.y * FRAMETIME;
            shard->current_position.z += shard->collection_velocity.z * FRAMETIME;
            if (NuVecDistSqr(&shard->current_position, &target, NULL) < 0.2f * 0.2f) {
                shard->state_flags = (shard->state_flags | 8) & ~4;
                ++shard->collector->field_0x106e;
                NewBuzzFrames(shard->collector->pad_gamepad->pad, 1, 0);
            }
        } else if (shard->state_flags & 0x20) {
            NUVEC offset;
            NuVecRotateZ(&offset, &v010, shard->angle_z);
            NuVecRotateZ(&offset, &offset, shard->angle_x);
            NuVecScale(&offset, &offset, (f32)qrand() * (1.0f / 65535.0f) * 0.0333f);
            NuVecAdd(&shard->current_position, &shard->position, &offset);
        } else {
            shard->current_position = shard->position;
        }
    }
}

static void Shards_Draw(void *context, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(context);
    SHARD *shard = static_cast<SHARD *>(world->shards);
    if (shard == NULL)
        return;
    for (i32 i = 0; i < world->shard_count; ++i, ++shard) {
        shard->state_flags &= ~0x10;
        if ((shard->state_flags & 0x0a) != 2)
            continue;
        NUMTX_ALIGNED16 matrix;
        if (shard->state_flags & 4) {
            u16 spin = (u16)(i32)((f32)(i32)shard->spin_angle + 98304.0f * shard->collection_time);
            NuMtxSetRotationY(&matrix, spin);
            u16 tumble = (u16)(i32)(60620.0f * shard->collection_time);
            u16 angle_z = shard->angle_z;
            if ((shard->state_flags & 0x40) == 0)
                angle_z += tumble;
            if (angle_z != 0)
                NuMtxRotateZ(&matrix, angle_z);
            u16 angle_x = shard->angle_x;
            if (shard->state_flags & 0x40)
                angle_x += tumble;
            if (angle_x != 0)
                NuMtxRotateX(&matrix, angle_x);
        } else {
            NuMtxSetRotationY(&matrix, shard->spin_angle);
            if (shard->angle_z != 0)
                NuMtxRotateZ(&matrix, shard->angle_z);
            if (shard->angle_x != 0)
                NuMtxRotateX(&matrix, shard->angle_x);
        }
        NuMtxTranslate(&matrix, &shard->current_position);
        LEVEL_OBJECT_RUNTIME_s *model = &world->lev_objs[shard->model_index];
        if (model->active != 0) {
            i32 drawn = NuSpecialDrawAt(&model->special, &matrix);
            shard->state_flags = (shard->state_flags & ~0x10) | ((drawn & 1) << 4);
        }
    }
}

static char *Shard_GetGizmoName(GIZMO *gizmo) {
    return gizmo != NULL ? static_cast<SHARD *>(gizmo->object)->name : NULL;
}

static i32 Shard_GetOutput(GIZMO *gizmo, i32, i32) {
    return (static_cast<SHARD *>(gizmo->object)->state_flags >> 3) & 1;
}

static char *Shard_GetOutputName(GIZMO *gizmo, i32 output_index) {
    return "Got";
}

static i32 Shard_GetNumOutputs(GIZMO *gizmo) {
    return 1;
}

static void Shard_Activate(GIZMO *gizmo, i32 active) {
    if (gizmo != NULL) {
        SHARD *shard = static_cast<SHARD *>(gizmo->object);
        shard->state_flags = (shard->state_flags & ~1) | (active != 0);
    }
}

static void Shard_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo != NULL) {
        SHARD *shard = static_cast<SHARD *>(gizmo->object);
        shard->state_flags = (shard->state_flags & ~2) | ((visible != 0) << 1);
    }
}

static void *Shards_AllocateProgressData(VARIPTR *start, VARIPTR *end) {
    return GizmoBufferAlloc(start, end, sizeof(SHARDPROGRESS_s));
}

static void Shards_ClearProgress(void *, void *data) {
    if (data != NULL) {
        SHARDPROGRESS_s *progress = static_cast<SHARDPROGRESS_s *>(data);
        memset(progress->collected, 0, sizeof(progress->collected));
        memset(progress->active, 0xff, sizeof(progress->active));
        memset(progress->visible, 0xff, sizeof(progress->visible));
    }
}

static void Shards_StoreProgress(void *context, void *, void *data) {
    if (data == NULL)
        return;
    SHARDPROGRESS_s *progress = static_cast<SHARDPROGRESS_s *>(data);
    Shards_ClearProgress(NULL, progress);
    WORLDINFO *world = static_cast<WORLDINFO *>(context);
    if (world != NULL && world->shards != NULL) {
        SHARD *shard = static_cast<SHARD *>(world->shards);
        for (i32 i = 0; i < world->shard_count && i < 128; ++i, ++shard) {
            i32 word = i >> 5;
            u32 mask = 1u << (i & 31);
            if (shard->state_flags & 8)
                progress->collected[word] |= mask;
            if (!(shard->state_flags & 2))
                progress->visible[word] &= ~mask;
            if (!(shard->state_flags & 1))
                progress->active[word] &= ~mask;
        }
    }
}

static void Shards_Reset(void *context, void *, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(context);
    if (world == NULL || world->shards == NULL)
        return;
    SHARDPROGRESS_s *progress = static_cast<SHARDPROGRESS_s *>(data);
    SHARD *shard = static_cast<SHARD *>(world->shards);
    for (i32 i = 0; i < world->shard_count; ++i, ++shard) {
        SHARD *current = &static_cast<SHARD *>(world->shards)[i];
        current->state_flags = (current->state_flags | 3) & ~0x18;
        current->model_index = qrand() / 9363;
        LEVEL_OBJECT_RUNTIME_s *models = world->lev_objs;
        for (i32 attempt = 0; attempt < 7 && models[current->model_index + 65].active == 0; ++attempt) {
            current->model_index++;
            if (current->model_index == 7)
                current->model_index = 0;
        }
        current->model_index += 65;
        current->spin_angle = qrand();
        current->current_position = current->position;
        current->collector = NULL;
        current->state_flags &= ~0x24;
        current->collection_time = 0.0f;
        if (i < 128 && progress != NULL) {
            u32 mask = 1u << (i & 31);
            i32 word = i >> 5;
            shard->state_flags = (shard->state_flags & ~8) | ((progress->collected[word] & mask) != 0 ? 8 : 0);
            shard->state_flags = (shard->state_flags & ~2) | ((progress->visible[word] & mask) != 0 ? 2 : 0);
            shard->state_flags = (shard->state_flags & ~1) | ((progress->active[word] & mask) != 0);
        }
    }
}

static void *Shards_ReserveBufferSpace(void *context) {
    WORLDINFO *world = static_cast<WORLDINFO *>(context);
    world->shards = NULL;
    world->shard_count = 0;
    if (world->current_level->max_shards != 0) {
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        world->shards = world->giz_buffer.void_ptr;
        world->giz_buffer.addr += world->current_level->max_shards * sizeof(SHARD);
    }
    return world->shards;
}

static i32 Shards_Load(void *world_info, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_info);
    if (world->shard_count != 0) {
        return 0;
    }
    i32 version = EdFileReadInt();
    world->shard_count = EdFileReadInt();
    for (i32 i = 0; i < world->shard_count; ++i) {
        EdFileRead(static_cast<SHARD *>(world->shards)[i].name, 16);
        EdFileReadNuVec(&static_cast<SHARD *>(world->shards)[i].position);
        if (version > 1) {
            SHARD *shard = &static_cast<SHARD *>(world->shards)[i];
            shard->angle_x = EdFileReadShort();
            shard = &static_cast<SHARD *>(world->shards)[i];
            shard->angle_z = EdFileReadShort();
        } else {
            static_cast<SHARD *>(world->shards)[i].angle_x = 0;
            static_cast<SHARD *>(world->shards)[i].angle_z = 0;
        }
    }
    return 1;
}

ADDGIZMOTYPE *Shards_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "Shard";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0x30;
    addtype.fns.early_update_fn = Shards_UpdateBeforeCharacters;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = Shards_GetMaxGizmos;
    addtype.fns.get_pos_fn = NULL;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = Shards_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = Shards_UpdateAfterCharacters;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = Shards_Draw;
    addtype.fns.get_gizmo_name_fn = Shard_GetGizmoName;
    addtype.fns.get_output_fn = Shard_GetOutput;
    addtype.fns.get_output_name_fn = Shard_GetOutputName;
    addtype.fns.get_num_outputs_fn = Shard_GetNumOutputs;
    addtype.fns.activate_fn = Shard_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = Shard_SetVisibility;
    addtype.fns.allocate_progress_data_fn = Shards_AllocateProgressData;
    addtype.fns.clear_progress_fn = Shards_ClearProgress;
    addtype.fns.store_progress_fn = Shards_StoreProgress;
    addtype.fns.reset_fn = Shards_Reset;
    addtype.fns.reserve_buffer_space_fn = Shards_ReserveBufferSpace;
    addtype.fns.load_fn = Shards_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;

    return &addtype;
}
