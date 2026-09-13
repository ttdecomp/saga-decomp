#include "legoapi/gizmos/trigger/gizrandom.h"

#include "decomp.h"
#include "globals.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/nucore/nustring.h"

static char gizrandom_output_name[] = "Random Output";

i32 gizrandom_gizmotype_id = -1;

i32 GizRandom_GetMaxGizmos(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL || world->current_level == NULL) {
        return 0;
    }
    return world->current_level->max_giz_randoms;
}

void GizRandom_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZRANDOMSYS *random_system = world->giz_randoms;
    for (i32 index = 0; index < random_system->count; ++index) {
        GIZRANDOM *random = &random_system->randoms[index];
        if (NuStrLen(random->name) != 0) {
            AddGizmo(gizmo_sys, type_id, NULL, random);
        }
    }
}

char *GizRandom_GetGizmoName(GIZMO *gizmo) {
    return gizmo != NULL ? static_cast<GIZRANDOM *>(gizmo->object)->name : NULL;
}

i32 GizRandom_GetOutput(GIZMO *gizmo, i32 output_index, i32) {
    GIZRANDOM *random = static_cast<GIZRANDOM *>(gizmo->object);
    return (random->flags & GIZRANDOM_FLAG_ACTIVE) != 0 && random->selected_output == output_index;
}

char *GizRandom_GetOutputName(GIZMO *, i32) {
    return gizrandom_output_name;
}

i32 GizRandom_GetNumOutputs(GIZMO *gizmo) {
    return static_cast<GIZRANDOM *>(gizmo->object)->output_count;
}

void GizRandom_Activate(GIZMO *gizmo, i32 active) {
    GIZRANDOM *random = static_cast<GIZRANDOM *>(gizmo->object);
    if (active == 0) {
        random->flags &= ~GIZRANDOM_FLAG_ACTIVE;
        return;
    }

    random->flags |= GIZRANDOM_FLAG_ACTIVE;
    random->selected_output = -1;
    const i32 roll = static_cast<i32>(static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 100.0f) + 1;
    i32 cumulative_weight = 0;
    for (i32 index = 0; index < random->output_count; ++index) {
        cumulative_weight += random->output_weights[index];
        if (roll <= cumulative_weight) {
            random->selected_output = index;
            break;
        }
    }
}

void *GizRandom_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    world->giz_randoms = NULL;
    if (world->current_level->max_giz_randoms == 0) {
        return NULL;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    GIZRANDOMSYS *random_system = reinterpret_cast<GIZRANDOMSYS *>(world->giz_buffer.addr);
    world->giz_randoms = random_system;
    world->giz_buffer.addr += sizeof(*random_system);
    random_system->randoms = reinterpret_cast<GIZRANDOM *>(world->giz_buffer.addr);
    world->giz_buffer.addr += world->current_level->max_giz_randoms * sizeof(GIZRANDOM);
    random_system->capacity = world->current_level->max_giz_randoms;
    return random_system;
}

ADDGIZMOTYPE *GizRandom_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizRandom";
    addtype.prefix = "rnd_";
    addtype.fns.unknown1 = 0;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizRandom_GetMaxGizmos;
    addtype.fns.get_pos_fn = NULL;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizRandom_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = NULL;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = NULL;
    addtype.fns.get_gizmo_name_fn = GizRandom_GetGizmoName;
    addtype.fns.get_output_fn = GizRandom_GetOutput;
    addtype.fns.get_output_name_fn = GizRandom_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizRandom_GetNumOutputs;
    addtype.fns.activate_fn = GizRandom_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = NULL;
    addtype.fns.allocate_progress_data_fn = NULL;
    addtype.fns.clear_progress_fn = NULL;
    addtype.fns.store_progress_fn = NULL;
    addtype.fns.reset_fn = NULL;
    addtype.fns.reserve_buffer_space_fn = GizRandom_ReserveBufferSpace;
    addtype.fns.load_fn = NULL;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;
    gizrandom_gizmotype_id = type_id;

    return &addtype;
}

GIZMO *createGizRandom(void *, i32 output_count, i32 *output_weights, char *name) {
    WORLDINFO *world = WorldInfo_CurrentlyLoading();
    if (world == NULL || world->giz_randoms->count == world->current_level->max_giz_randoms) {
        return NULL;
    }

    GIZRANDOM *random = &world->giz_randoms->randoms[world->giz_randoms->count];
    random->output_count = output_count;
    for (i32 index = 0; index < output_count; ++index) {
        random->output_weights[index] = output_weights[index];
    }
    NuStrNCpy(random->name, name, sizeof(random->name));
    ++world->giz_randoms->count;
    return AddGizmo(world->gizmo_sys, gizrandom_gizmotype_id, NULL, random);
}
