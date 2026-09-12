#include "legoapi/gizmos/door/securitydoors.h"

#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

extern "C" void NewTerrPlatformsOff(void);
extern "C" void PlatInstRotate(i32 platform_id, i32 enabled);
void FindAnglesZX(NUVEC *normal, u16 *x_rotation, u16 *z_rotation);

struct SECURITYDOORPROGRESS {
    i32 state[2];
    i32 activated;
};

static i32 SecurityDoors_GetMaxGizmos(void *door) {
    WORLDINFO *world = static_cast<WORLDINFO *>(door);
    return world != NULL ? world->current_level->max_security_doors : 0;
}

static void SecurityDoors_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_data, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    for (i32 i = 0; i < world->security_door_count; ++i) {
        SECURITYDOOR *door = &world->security_doors[i];
        if (NuStrLen(door->name) != 0) {
            AddGizmo(gizmo_sys, type_id, NULL, door);
        }
    }
}

static void SecurityDoors_Update(void *world_data, void *, float) {
    if (world_data == NULL) {
        return;
    }

    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    SECURITYDOOR *door = world->security_doors;
    if (door == NULL || world->security_door_count <= 0) {
        return;
    }

    for (i32 i = 0; i < world->security_door_count; ++i, ++door) {
        if (door->state == 2 && door->opening < 1.0f) {
            door->opening += FRAMETIME;
            if (door->opening >= 1.0f) {
                door->opening = 1.0f;
                door->opened = 1;
            }
        }

        i32 left_angle;
        i32 right_angle;
        if (door->opening <= 0.0f) {
            left_angle = 0;
            right_angle = 0;
        } else if (door->opening >= 1.0f) {
            left_angle = -0x4000;
            right_angle = 0x4000;
        } else {
            f32 phase = NuTrigTable[(static_cast<i32>(door->opening * 32768.0f + 16384.0f) >> 1) & 0x7fff];
            right_angle = static_cast<u16>(static_cast<i32>((1.0f - (phase + 1.0f) * 0.5f) * 16384.0f));
            left_angle = -right_angle;
        }

        NUVEC translation = {0.2667f, 0.0f, 0.0f};
        NuMtxSetRotationY(&door->leaf_matrix[0], left_angle);
        NuMtxTranslate(&door->leaf_matrix[0], &translation);
        NuMtxRotateY(&door->leaf_matrix[0], door->yaw);
        NuMtxTranslate(&door->leaf_matrix[0], &door->position);

        translation.x = -0.2667f;
        NuMtxSetRotationY(&door->leaf_matrix[1], right_angle);
        NuMtxTranslate(&door->leaf_matrix[1], &translation);
        NuMtxRotateY(&door->leaf_matrix[1], door->yaw);
        NuMtxTranslate(&door->leaf_matrix[1], &door->position);
    }
}

static void SecurityDoors_Draw(void *world_data, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    SECURITYDOOR *door = world->security_doors;
    if (door == NULL) {
        return;
    }

    const u16 rotation = static_cast<u16>(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 5.0f) / 5.0f * 65536.0f));
    const f32 pulse_phase = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f) * 2.0f * 65536.0f;
    const f32 pulse = 0.8f + NU_SIN_LUT(static_cast<i32>(pulse_phase)) * 0.2f;

    for (i32 i = 0; i < world->security_door_count; ++i, ++door) {
        door->flags &= ~8;
        if (door->visible) {
            if (world->lev_objs[81].active != 0) {
                NUMTX matrix = door->leaf_matrix[0];
                if (NuSpecialDrawAt(&world->lev_objs[81].special, &matrix) != 0) {
                    door->flags |= 8;
                }
            }
            if (world->lev_objs[82].active != 0) {
                NUMTX matrix = door->leaf_matrix[1];
                if (NuSpecialDrawAt(&world->lev_objs[82].special, &matrix) != 0) {
                    door->flags |= 8;
                }
            }
        }

        if (door->active && !door->opened && world->lev_objs[85].active != 0) {
            NUMTX matrix;
            NuMtxSetRotationY(&matrix, rotation);
            if (door->terrain_angle_x != 0) {
                NuMtxRotateZ(&matrix, door->terrain_angle_x);
            }
            if (door->terrain_angle_z != 0) {
                NuMtxRotateX(&matrix, door->terrain_angle_z);
            }
            NuMtxTranslate(&matrix, &door->player_position);

            GameObject_s *nearest_player;
            f32 distance_squared;
            if (FindNearestPlayerToVec(&door->position, &nearest_player, distance_squared, true, 0x2000000)) {
                const f32 distance_phase = NuFmin(distance_squared / 51.0f, 1.0f) * 2.0f - 3640.0f + 16384.0f;
                const f32 alpha = pulse - NU_SIN_LUT(static_cast<i32>(distance_phase));
                NuSpecialDrawAtAlpha(&world->lev_objs[85].special, &matrix, alpha);
            }
        }
    }
}

static char *SecurityDoor_GetGizmoName(GIZMO *gizmo) {
    return gizmo != NULL ? static_cast<SECURITYDOOR *>(gizmo->object)->name : NULL;
}

static i32 SecurityDoor_GetOutput(GIZMO *gizmo, i32, i32) {
    return static_cast<SECURITYDOOR *>(gizmo->object)->opened;
}

static char *SecurityDoor_GetOutputName(GIZMO *, i32) {
    return "Opened";
}

static i32 SecurityDoor_GetNumOutputs(GIZMO *) {
    return 1;
}

static void SecurityDoor_Activate(GIZMO *gizmo, i32 active) {
    if (gizmo != NULL) {
        static_cast<SECURITYDOOR *>(gizmo->object)->active = active != 0;
    }
}

static void SecurityDoor_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo != NULL) {
        static_cast<SECURITYDOOR *>(gizmo->object)->visible = visible != 0;
    }
}

static NUVEC *SecurityDoor_GetPos(GIZMO *gizmo) {
    return gizmo != NULL ? &static_cast<SECURITYDOOR *>(gizmo->object)->position : NULL;
}

static void *SecurityDoors_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, 12);
}

static void SecurityDoors_ClearProgress(void *, void *progress_data) {
    SECURITYDOORPROGRESS *progress = (SECURITYDOORPROGRESS *)progress_data;
    if (progress == NULL) {
        return;
    }

    progress->state[0] = -1;
    progress->state[1] = -1;
    progress->activated = 0;
}

static void SecurityDoors_StoreProgress(void *world_data, void *, void *progress_data) {
    SECURITYDOORPROGRESS *progress = static_cast<SECURITYDOORPROGRESS *>(progress_data);
    if (progress == NULL) {
        return;
    }

    progress->state[0] = -1;
    progress->state[1] = -1;
    progress->activated = 0;

    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    if (world == NULL || world->security_doors == NULL || world->security_door_count <= 0) {
        return;
    }

    SECURITYDOOR *door = world->security_doors;
    for (i32 i = 0; i < world->security_door_count && i < 32; ++i, ++door) {
        const u32 mask = 1U << i;
        const i32 word = i >> 5;
        if (!door->visible) {
            progress->state[0 + word] &= ~mask;
        }
        if (!door->active) {
            progress->state[1 + word] &= ~mask;
        }
        if (door->opened) {
            reinterpret_cast<u32 *>(&progress->activated)[word] |= mask;
        }
    }
}

static void SecurityDoors_Reset(void *world_data, void *, void *progress_data) {
    if (world_data == NULL) {
        return;
    }

    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    SECURITYDOOR *door = world->security_doors;
    if (door == NULL || world->security_door_count <= 0) {
        return;
    }

    SECURITYDOORPROGRESS *progress = static_cast<SECURITYDOORPROGRESS *>(progress_data);
    for (i32 i = 0; i < world->security_door_count; ++i, ++door) {
        door->player_position.x = 0.0f;
        door->player_position.z = 0.0f;
        door->player_position.y = 0.4f;
        NuVecRotateY(&door->player_position, &door->player_position, door->yaw);
        NuVecAdd(&door->player_position, &door->player_position, &door->position);

        NewTerrPlatformsOff();
        f32 height = GameShadow(NULL, &door->player_position, 5.0f, -1);
        if (height == 2000000.0f) {
            door->player_position.y = 2000000.0f;
        } else {
            door->player_position.y = height + 0.01f;
            FindAnglesZX(&v001, &door->terrain_angle_z, &door->terrain_angle_x);
        }

        door->opening = 0.0f;
        door->state = 0;
        door->active = 1;
        door->visible = 1;
        door->opened = 0;
        door->flags &= ~8;

        NUVEC translation = {0.2667f, 0.0f, 0.0f};
        NuMtxSetRotationY(&door->leaf_matrix[0], 0);
        NuMtxTranslate(&door->leaf_matrix[0], &translation);
        NuMtxRotateY(&door->leaf_matrix[0], door->yaw);
        NuMtxTranslate(&door->leaf_matrix[0], &door->position);

        translation.x = -0.2667f;
        NuMtxSetRotationY(&door->leaf_matrix[1], 0);
        NuMtxTranslate(&door->leaf_matrix[1], &translation);
        NuMtxRotateY(&door->leaf_matrix[1], door->yaw);
        NuMtxTranslate(&door->leaf_matrix[1], &door->position);

        PlatInstRotate(door->platform_id[0], 1);
        PlatInstRotate(door->platform_id[1], 1);

        if (progress != NULL && i <= 31) {
            const u32 mask = 1U << i;
            door->visible = (progress->state[0] & mask) != 0;
            door->active = (progress->state[1] & mask) != 0;
            door->opened = (progress->activated & mask) != 0;
            if (door->opened) {
                door->opening = 1.0f;
                door->state = 2;

                translation.x = 0.2667f;
                NuMtxSetRotationY(&door->leaf_matrix[0], -0x4000);
                NuMtxTranslate(&door->leaf_matrix[0], &translation);
                NuMtxRotateY(&door->leaf_matrix[0], door->yaw);
                NuMtxTranslate(&door->leaf_matrix[0], &door->position);

                translation.x = -0.2667f;
                NuMtxSetRotationY(&door->leaf_matrix[1], 0x4000);
                NuMtxTranslate(&door->leaf_matrix[1], &translation);
                NuMtxRotateY(&door->leaf_matrix[1], door->yaw);
                NuMtxTranslate(&door->leaf_matrix[1], &door->position);
            }
        }
    }
}

static void *SecurityDoors_ReserveBufferSpace(void *world_data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    world->security_doors = NULL;
    world->security_door_count = 0;

    const i32 count = world->current_level->max_security_doors;
    if (count == 0) {
        return NULL;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 16);
    world->security_doors = static_cast<SECURITYDOOR *>(world->giz_buffer.void_ptr);
    world->giz_buffer.addr += sizeof(SECURITYDOOR) * count;
    return world->security_doors;
}

static i32 SecurityDoors_Load(void *world_data, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_data);
    if (world->security_door_count != 0)
        return 0;
    EdFileReadInt();
    world->security_door_count = EdFileReadInt();
    SECURITYDOOR *door = world->security_doors;
    for (i32 i = 0; i < world->security_door_count; ++i, ++door) {
        EdFileRead(door->name, 16);
        EdFileReadNuVec(&door->position);
        door->yaw = EdFileReadShort();
    }
    return 1;
}

ADDGIZMOTYPE *SecurityDoors_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "SecurityDoor";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0xc;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = SecurityDoors_GetMaxGizmos;
    addtype.fns.get_pos_fn = SecurityDoor_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = SecurityDoors_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = SecurityDoors_Update;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = SecurityDoors_Draw;
    addtype.fns.get_gizmo_name_fn = SecurityDoor_GetGizmoName;
    addtype.fns.get_output_fn = SecurityDoor_GetOutput;
    addtype.fns.get_output_name_fn = SecurityDoor_GetOutputName;
    addtype.fns.get_num_outputs_fn = SecurityDoor_GetNumOutputs;
    addtype.fns.activate_fn = SecurityDoor_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = SecurityDoor_SetVisibility;
    addtype.fns.allocate_progress_data_fn = SecurityDoors_AllocateProgressData;
    addtype.fns.clear_progress_fn = SecurityDoors_ClearProgress;
    addtype.fns.store_progress_fn = SecurityDoors_StoreProgress;
    addtype.fns.reset_fn = SecurityDoors_Reset;
    addtype.fns.reserve_buffer_space_fn = SecurityDoors_ReserveBufferSpace;
    addtype.fns.load_fn = SecurityDoors_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;

    return &addtype;
}
