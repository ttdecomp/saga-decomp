#include "legoapi/gizmos/trigger/giztimer.h"

#include "gameapi/edtools/edfile.h"
#include "legoapi/world/level.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/nucore/nustring.h"

i32 giztimer_gizmotype_id = -1;

i32 GizTimer_GetMaxGizmos(void *world_info) {
    WORLDINFO *world = (WORLDINFO *)world_info;

    if (world == NULL || world->current_level == NULL) {
        return 0;
    }

    return world->current_level->max_giz_timers;
}

void GizTimer_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_info, void *unknown) {
    WORLDINFO *world = (WORLDINFO *)world_info;

    for (i32 i = 0; i < world->giz_timers_count; i++) {
        if (NuStrLen(world->giz_timers[i].name) == 0) {
            continue;
        }

        AddGizmo(gizmo_sys, type_id, NULL, &world->giz_timers[i]);
    }
}

void GizTimer_Update(void *world_info, void *, float delta_time) {
    WORLDINFO *world = (WORLDINFO *)world_info;

    for (i32 i = 0; i < world->giz_timers_count; i++) {
        GIZTIMER *timer = &world->giz_timers[i];

        if (timer->time_remaining >= 0.0f) {
            timer->time_remaining -= delta_time;
        }
    }
}

char *GizTimer_GetGizmoName(GIZMO *gizmo) {
    if (gizmo == NULL) {
        return NULL;
    }

    GIZTIMER *timer = (GIZTIMER *)gizmo->object;

    return timer->name;
}

i32 GizTimer_GetOutput(GIZMO *gizmo, i32, i32) {
    GIZTIMER *timer = (GIZTIMER *)gizmo->object;

    if (timer->flags & 1) {
        return timer->time_remaining <= 0.0f;
    }

    return 0;
}

char *GizTimer_GetOutputName(GIZMO *gizmo, i32 output_index) {
    return "Ping";
}

i32 GizTimer_GetNumOutputs(GIZMO *gizmo) {
    return 1;
}

void GizTimer_Activate(GIZMO *gizmo, i32 unknown) {
    // can't get this stupid function to match
    GIZTIMER *timer = (GIZTIMER *)gizmo->object;

    if (timer->flags & 2) {
        timer->time_remaining = QRAND_FLOAT() * timer->start_time;

    } else {
        timer->time_remaining = timer->start_time;
    }

    timer->flags = (timer->flags & ~1) | (unknown != 0);
}

void *GizTimer_ReserveBufferSpace(void *world_info) {
    WORLDINFO *world = (WORLDINFO *)world_info;

    world->giz_timers = NULL;
    world->giz_timers_count = 0;

    GIZTIMER *buffer = NULL;
    if (world->current_level->max_giz_timers != 0) {
        buffer = BUFFER_ALLOC_ARRAY(&world->giz_buffer, world->current_level->max_giz_timers, GIZTIMER);
        world->giz_timers = buffer;
    }

    return buffer;
}

i32 GizTimer_Load(void *world_info, void *) {
    NUVEC vec;
    char buffer[16];

    WORLDINFO *world = (WORLDINFO *)world_info;
    if (world->giz_timers_count == 0) {
        EdFileReadInt();
        i32 count = EdFileReadInt();

        for (i32 i = 0; i < count; i++) {
            i32 length = EdFileReadInt();
            EdFileRead(buffer, length);
            EdFileReadFloat();
            EdFileReadUnsignedShort();
            EdFileReadNuVec(&vec);
        }

        return 1;
    }

    return 0;
}

GIZMO *createGizTimer(void *, float time, i32 random_time, char *name) {
    WORLDINFO *world = WorldInfo_CurrentlyLoading();
    if (world == NULL || world->giz_timers == NULL || world->giz_timers_count == world->current_level->max_giz_timers)
        return NULL;
    GIZTIMER *timer = &world->giz_timers[world->giz_timers_count];
    timer->start_time = time;
    timer->random_time = random_time;
    NuStrNCpy(timer->name, name, sizeof(timer->name));
    ++world->giz_timers_count;
    return AddGizmo(world->gizmo_sys, giztimer_gizmotype_id, NULL, timer);
}

ADDGIZMOTYPE *GizTimer_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizTimer";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0;
    addtype.fns.get_max_gizmos_fn = GizTimer_GetMaxGizmos;
    addtype.fns.add_gizmos_fn = GizTimer_AddGizmos;
    addtype.fns.early_update_fn = GizTimer_Update;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_pos_fn = NULL;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = NULL;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = NULL;
    addtype.fns.get_gizmo_name_fn = GizTimer_GetGizmoName;
    addtype.fns.get_output_fn = GizTimer_GetOutput;
    addtype.fns.get_output_name_fn = GizTimer_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizTimer_GetNumOutputs;
    addtype.fns.activate_fn = GizTimer_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = NULL;
    addtype.fns.allocate_progress_data_fn = NULL;
    addtype.fns.clear_progress_fn = NULL;
    addtype.fns.store_progress_fn = NULL;
    addtype.fns.reset_fn = NULL;
    addtype.fns.reserve_buffer_space_fn = GizTimer_ReserveBufferSpace;
    addtype.fns.load_fn = GizTimer_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;
    giztimer_gizmotype_id = type_id;

    return &addtype;
}
