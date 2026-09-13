#include "legoapi/gizmos/trigger/minicut.h"

#include "decomp.h"
#include "gameapi/ai/aisys/aisys.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nuvec.h"

#include <stdio.h>
#include <string.h>

extern i32 editor_active;
extern i32 MiniCutCam;
void GameCameraMakeMiniCut3(u32, float, i32, i32, i32, void *, i32, NUVEC *, float, float, float, float, float, float,
                            float, i32, nugspline_s *, char, char);

static i32 GizMiniCut_GetMaxGizmos(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    return world->current_level->max_minicuts;
}

static void GizMiniCut_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world->minicuts == NULL || world->minicut_count <= 0) {
        return;
    }

    for (i32 index = 0; index < world->minicut_count; ++index) {
        AddGizmo(gizmo_sys, type_id, NULL, &world->minicuts[index]);
    }
}

static char *GizMiniCut_GetGizmoName(GIZMO *gizmo) {
    return gizmo != NULL ? static_cast<char *>(gizmo->object) : NULL;
}

static i32 GizMiniCut_GetOutput(GIZMO *gizmo, i32 output_index, i32) {
    if (gizmo == NULL || gizmo->object == NULL)
        return 0;
    MINICUT *minicut = static_cast<MINICUT *>(gizmo->object);
    if (output_index == 0)
        return MiniCutCam == 0 ? minicut->played : 0;
    if (output_index == 1)
        return minicut->played;
    return 0;
}

static char *GizMiniCut_GetOutputName(GIZMO *gizmo, i32 output_index) {
    if (output_index == 0)
        return "Played";
    if (output_index == 1)
        return "Playing";
    return "Unknown!";
}

static i32 GizMiniCut_GetNumOutputs(GIZMO *gizmo) {
    return 2;
}

void GizMiniCut_Activate(GIZMO *gizmo, i32 active) {
    if (active == 0)
        return;
    MINICUT *minicut = static_cast<MINICUT *>(gizmo->object);
    for (i32 index = 0; index < minicut->part_count; ++index) {
        MINICUTPART *part = &minicut->parts[index];
        NUVEC position;
        position.z = part->field_0x30;
        position.x = 0.0f;
        position.y = 0.0f;
        NuVecRotateX(&position, &position, static_cast<u16>(part->field_0x34));
        NuVecRotateY(&position, &position, static_cast<u16>(part->field_0x36));
        NuVecAdd(&position, &position, part->resolved_position);
        u32 flags = index == 0 ? 0x28f : 0x8f;
        if (index == minicut->part_count - 1)
            flags |= 0x400;
        if (part->field_0x3c >= 0.0f)
            flags |= 0x800;
        if (part->field_0x40 > 0.0f)
            flags |= 0x1000;
        GameCameraMakeMiniCut3(flags, part->field_0x30, static_cast<u16>(part->field_0x34),
                               static_cast<u16>(part->field_0x36), static_cast<u16>(part->field_0x38),
                               part->resolved_position, 0, &position, minicut->field_0x1c, minicut->field_0x24,
                               minicut->field_0x20, minicut->field_0x28, part->field_0x3c, part->field_0x40,
                               minicut->field_0x2c, 0, NULL, -1, 2);
        if (editor_active == 0)
            minicut->played = 1;
    }
}

static NUVEC *GizMiniCut_GetPos(GIZMO *gizmo) {
    if (gizmo == NULL || gizmo->object == NULL) {
        return NULL;
    }
    MINICUT *minicut = static_cast<MINICUT *>(gizmo->object);
    if (minicut->part_count == 0) {
        return NULL;
    }
    return minicut->parts[0].resolved_position;
}

static i32 GizMiniCut_UsingSpecial(GIZMO **, void *, i32, char *) {
    UNIMPLEMENTED();
    return 0;
}

void GizMiniCut_Reset(void *world_ptr, void *, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    for (i32 minicut_index = 0; minicut_index < world->minicut_count; ++minicut_index) {
        MINICUT *minicut = &world->minicuts[minicut_index];
        for (i32 part_index = 0; part_index < minicut->part_count; ++part_index) {
            MINICUTPART *part = &minicut->parts[part_index];
            GameObject_s *object = GetNamedGameObject(world->ai_sys, part->name);
            if (object != NULL) {
                part->resolved_position = &object->apiobj.collision_position;
                continue;
            }

            nuhspecial_s special;
            NuSpecialFind(world->current_gscn, &special, part->name, 0);
            if (NuSpecialExistsFn(&special) != 0) {
                part->resolved_position = NuSpecialGetDrawPos(&special);
            } else {
                AILOCATOR *locator = AIPathFindLocator(world->ai_sys, part->name);
                if (locator != NULL) {
                    part->resolved_position = &locator->position;
                }
            }
            if (part->resolved_position == NULL) {
                part->resolved_position = &part->position;
            }
        }
    }
}

static void *GizMiniCut_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    world->minicuts = NULL;
    world->minicut_count = 0;
    if (world->current_level->max_minicuts == 0) {
        return NULL;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    world->minicuts = static_cast<MINICUT *>(world->giz_buffer.void_ptr);
    world->giz_buffer.addr += world->current_level->max_minicuts * sizeof(MINICUT);
    memset(world->minicuts, 0, world->current_level->max_minicuts * sizeof(MINICUT));

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    world->minicut_parts = static_cast<MINICUTPART *>(world->giz_buffer.void_ptr);
    const i32 part_capacity = world->current_level->max_minicuts * world->current_level->max_minicut_parts;
    world->giz_buffer.addr += part_capacity * sizeof(MINICUTPART);
    memset(world->minicut_parts, 0, part_capacity * sizeof(MINICUTPART));

    for (i32 index = 0; index < world->current_level->max_minicuts; ++index) {
        MINICUT *minicut = &world->minicuts[index];
        minicut->field_0x24 = 2.5f;
        minicut->field_0x28 = 2.5f;
        minicut->field_0x20 = 10.0f;
        minicut->field_0x2c = 10.0f;
        minicut->field_0x1c = 0.0f;
        minicut->parts = &world->minicut_parts[index * world->current_level->max_minicut_parts];
        minicut->part_count = 0;
        sprintf(minicut->name, "Minicut %i", index + 1);
    }
    return world->minicuts;
}

i32 GizMiniCut_Load(void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    EdFileReadInt();
    world->minicut_count = EdFileReadInt();
    if (world->minicut_count > world->current_level->max_minicuts) {
        world->minicut_count = world->current_level->max_minicuts;
    }

    for (i32 minicut_index = 0; minicut_index < world->minicut_count; ++minicut_index) {
        MINICUT *minicut = &world->minicuts[minicut_index];
        EdFileRead(minicut->name, static_cast<i8>(EdFileReadChar()));
        minicut->field_0x1c = EdFileReadFloat();
        minicut->field_0x20 = EdFileReadFloat();
        minicut->field_0x24 = EdFileReadFloat();
        minicut->field_0x28 = EdFileReadFloat();
        minicut->field_0x2c = EdFileReadFloat();
        minicut->part_count = static_cast<i8>(EdFileReadChar());

        for (i32 part_index = 0; part_index < minicut->part_count; ++part_index) {
            MINICUTPART *part = &minicut->parts[part_index];
            EdFileRead(part->name, static_cast<i8>(EdFileReadChar()));
            part->position.x = EdFileReadFloat();
            part->position.y = EdFileReadFloat();
            part->position.z = EdFileReadFloat();
            part->field_0x30 = EdFileReadFloat();
            part->field_0x34 = EdFileReadShort();
            part->field_0x36 = EdFileReadShort();
            part->field_0x38 = EdFileReadShort();
            part->field_0x3c = EdFileReadFloat();
            part->field_0x40 = EdFileReadFloat();
        }
    }
    return 1;
}

i32 GizMiniCut_GetGuid(GIZMO_s *gizmo) {
    if (gizmo == NULL || gizmo->object == NULL) {
        return -1;
    }
    return static_cast<MINICUT *>(gizmo->object)->guid;
}

ADDGIZMOTYPE *MiniCut_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "MiniCut";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizMiniCut_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizMiniCut_GetPos;
    addtype.fns.using_special_fn = GizMiniCut_UsingSpecial;
    addtype.fns.add_gizmos_fn = GizMiniCut_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = NULL;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = NULL;
    addtype.fns.get_gizmo_name_fn = GizMiniCut_GetGizmoName;
    addtype.fns.get_output_fn = GizMiniCut_GetOutput;
    addtype.fns.get_output_name_fn = GizMiniCut_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizMiniCut_GetNumOutputs;
    addtype.fns.activate_fn = GizMiniCut_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = NULL;
    addtype.fns.allocate_progress_data_fn = NULL;
    addtype.fns.clear_progress_fn = NULL;
    addtype.fns.store_progress_fn = NULL;
    addtype.fns.reset_fn = GizMiniCut_Reset;
    addtype.fns.reserve_buffer_space_fn = GizMiniCut_ReserveBufferSpace;
    addtype.fns.load_fn = GizMiniCut_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;

    return &addtype;
}
