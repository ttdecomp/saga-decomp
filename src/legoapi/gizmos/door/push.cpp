#include "legoapi/gizmos/door/push.h"

#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "globals.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/numem.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nuvec.h"

#include <string.h>

struct PUSHPROGRESS {
    u32 visible_mask;
    u32 state_mask;
    u32 position_mask;
    NUVEC positions[16];
    NUVEC end_positions[2][16];
};
DECOMP_ASSERT(sizeof(PUSHPROGRESS) == 0x24c, "push-block progress size");

enum PUSHBLOCK_CONFIG_FLAGS {
    PUSHBLOCK_FLAG_VISIBLE = 1 << 2,
    PUSHBLOCK_FLAG_STATE = 1 << 1,
};

enum PUSHBLOCK_RUNTIME_FLAGS {
    PUSHBLOCK_RUNTIME_MOVING = 1 << 0,
    PUSHBLOCK_RUNTIME_SEEKING = 1 << 1,
    PUSHBLOCK_RUNTIME_RESET_POSITION = 1 << 2,
    PUSHBLOCK_RUNTIME_RESET_HEIGHT = 1 << 3,
};

i32 pushblock_gizmotype_id;

extern "C" char *NuIToA(i32 value, char *buffer, i32 radix);
void ResetPushProgress(WORLDINFO_s *world, void *progress);
i32 GizPushBlock_EndFrameCompleted(pushblock_s *push_block, i32 output_index);
void GizObstacles_AddTrigger(NUVEC *position);
void MoveBlocks(WORLDINFO_s *world, pushblock_s *push_block, i32 index, NUVEC *velocity);
void PushSeekComplete(pushblock_s *push_block, i32 snap_index);

static i32 Push_GetMaxGizmos(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    return world != NULL ? world->current_level->max_push_blocks : 0;
}

static void Push_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    for (i32 index = 0; index < world->push_block_count; ++index) {
        if (NuStrLen(world->push_blocks[index].name) != 0) {
            AddGizmo(gizmo_sys, type_id, NULL, &world->push_blocks[index]);
        }
    }
}

#include <math.h>
#include "nu2api/numath/nutrig.h"
extern "C" TERRAIN_SURFACE_s TerSurface[32];
extern i32 LEGOHINT_PUSHBLOCKS;
static NUVEC hothbtestpos = {29.0f, 0.0f, 13.0f};
i32 SnapPosTaken(WORLDINFO_s *, pushblock_s *, NUVEC *, i32);
i32 OtherBlockInRange(WORLDINFO_s *, pushblock_s *, NUVEC *, i32);
pushblock_s *BlockInBlock(WORLDINFO_s *, pushblock_s *, i32, pushblock_s **);
i32 TerrainBlockOnBlock(WORLDINFO_s *, pushblock_s *, NUVEC *, f32 *);
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
f32 SeekValF(f32, f32, f32);
i32 GameAudio_GetPlrSfxBits(void *);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
void Hint_SetComplete(i32);
void AddShoveObject(nuhspecial_s *, i16);
extern "C" {
    void PlatOnOff(i32, i32);
    void NewTerrPlatformsOff();
    i32 ShadowInfo();
    i32 EShadowInfo();
}
void UpdatePushBlocks(void *world_ptr, void *, float) {
    static f32 snap_time = 0.25f;
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (!world || !world->push_block_count || !world->push_blocks)
        return;
    snap_time = 0.25f;
    i32 target_seen = 0;
    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *p = &world->push_blocks[index];
        i32 visible = NuSpecialGetVisibilityFn(&p->special) & 1;
        p->flags_0ca = (p->flags_0ca & ~4) | (visible << 2);
        if (!visible)
            continue;
        GizObstacles_AddTrigger(p->position);
        if ((p->runtime_flags_0c8 & 0x81) == 0x81)
            p->runtime_flags_0c8 &= 0x3f;
        if ((i8)p->runtime_flags_0c8 < 0)
            p->runtime_flags_0c8 |= 0x40;
        if ((p->runtime_flags_0c9 & 1) || (p->flags_0cb & 2))
            continue;
        if (p->snap_positions) {
            if (p->runtime_flags_0c9 & 2) {
                if (p->snap_timer <= snap_time) {
                    f32 fraction = NU_SIN_LUT((i32)((p->snap_timer / snap_time) * 16384.0f + 49152.0f + 16384.0f));
                    i32 ix = (p->packed_state_flags >> 15) & 7;
                    p->position->x = p->snap_origin.x + (p->snap_positions[ix].x - p->snap_origin.x) * fraction;
                    p->position->y = p->snap_origin.y + (p->snap_positions[ix].y - p->snap_origin.y) * fraction;
                    p->position->z = p->snap_origin.z + (p->snap_positions[ix].z - p->snap_origin.z) * fraction;
                    NuSpecialUpdate(&p->special);
                    for (i32 i = 0; i < p->end_position_count; ++i) {
                        NUMTX *m = NuSpecialGetInstanceMtx(&p->end_position_specials[i]);
                        NUVEC *target = &p->snap_positions[(p->packed_state_flags >> 15) & 7];
                        m->m32 = p->end_position_origins[i].z + (target->z - p->end_position_origins[i].z) * fraction;
                        m->m30 = p->end_position_origins[i].x + (target->x - p->end_position_origins[i].x) * fraction;
                        NuSpecialUpdate(&p->end_position_specials[i]);
                    }
                    p->snap_timer += FRAMETIME;
                    continue;
                }
                PushSeekComplete(p, (p->packed_state_flags >> 15) & 7);
            } else {
                for (i32 i = 1; i < ((p->runtime_flags_0c9 >> 4) & 7); ++i) {
                    NUVEC *target = &p->snap_positions[i];
                    NUVEC delta = {target->x - p->position->x, target->y - p->position->y, target->z - p->position->z};
                    f32 radius = p->snap_distance;
                    if (world->current_level == HOTHESCAPEB_LDATA &&
                        NuVecXZDistSqr(target, &hothbtestpos, NULL) < 0.0625f)
                        radius *= 2.5f;
                    else
                        radius += radius;
                    if (!(radius * radius > (delta.y * delta.y + delta.x * delta.x) + delta.z * delta.z))
                        continue;
                    if (SnapPosTaken(world, p, target, index))
                        continue;
                    if ((p->flags_0cb & 0x10) || OtherBlockInRange(world, p, &p->snap_positions[i], index)) {
                        target_seen = 1;
                        continue;
                    }
                    p->runtime_flags_0c9 |= 2;
                    if (p->flags_0cb & 0x20)
                        p->flags_0cb |= 0x10;
                    p->snap_origin.x = p->position->x;
                    p->snap_origin.z = p->position->z;
                    for (i32 j = 0; j < p->end_position_count; ++j)
                        p->end_position_origins[j] = *NuSpecialGetDrawPos(&p->end_position_specials[j]);
                    p->packed_state_flags = (p->packed_state_flags & 0xfffc7fff) | ((i & 7) << 15);
                    p->snap_timer = FRAMETIME;
                    target_seen = 1;
                    break;
                }
                if (!target_seen) {
                    if (p->flags_0cb & 0x20) {
                        p->flags_0cb &= ~0x10;
                        p->packed_state_flags &= 0xfffc7fff;
                    }
                    if (!(p->flags_0cb & 0x40))
                        p->completion_flags &= 0xf807;
                }
            }
        }
        if (!(p->runtime_flags_0c8 & 0x40) && ((p->runtime_flags_0c8 & 3) == 2 || p->snap_origin.x != p->position->x ||
                                               p->snap_origin.z != p->position->z)) {
            p->target_velocity.z = p->target_velocity.x = p->velocity.z = p->velocity.x = 0;
        }
        if (!(p->runtime_flags_0c8 & 0x49))
            continue;
        if (p->runtime_flags_0c8 & 0x41) {
            i32 surface = p->terrain_info[0];
            bool same = p->terrain_info[1] == surface && p->terrain_info[2] == surface && p->terrain_info[3] == surface;
            if (same) {
                if ((u32)surface <= 31 && (TerSurface[surface].flags & 0x20000))
                    p->runtime_flags_0c8 |= 0x40;
                else
                    p->runtime_flags_0c8 &= ~0x40;
            }
            if (p->pushing_object) {
                if (!(p->runtime_flags_0c8 & 0x40))
                    p->target_velocity.z = p->target_velocity.x = 0;
                f32 x = p->pushing_object->target_velocity.x, z = p->pushing_object->target_velocity.z;
                if (fabsf(x) > fabsf(z)) {
                    p->velocity.z = p->target_velocity.z = 0;
                    p->target_velocity.x = x * FRAMETIME;
                } else {
                    p->velocity.x = p->target_velocity.x = 0;
                    p->target_velocity.z = z * FRAMETIME;
                }
            } else if (same) {
                p->target_velocity.x *= TerSurface[surface].movement_scale;
                p->target_velocity.z *= TerSurface[surface].movement_scale;
            } else
                p->target_velocity.x = p->velocity.x = p->target_velocity.z = p->velocity.z = 0;
            p->velocity.x = SeekValF(p->velocity.x, p->target_velocity.x, 1.0f);
            p->velocity.z = SeekValF(p->velocity.z, p->target_velocity.z, 1.0f);
            if (p->velocity.x != 0) {
                if (fabsf(p->velocity.x) <= 0.0001f) {
                    p->runtime_flags_0c8 &= 0x3f;
                    p->velocity.x = p->target_velocity.x = 0;
                }
            } else if (fabsf(p->velocity.z) <= 0.0001f) {
                p->runtime_flags_0c8 &= 0x3f;
                p->velocity.z = p->target_velocity.z = 0;
            }
        }
        if (p->runtime_flags_0c8 & 8) {
            p->target_velocity.y = 10.0f * FRAMETIME;
            p->target_velocity.x = p->target_velocity.z = p->velocity.x = 0;
            p->velocity.y = SeekValF(p->velocity.y, p->target_velocity.y, 1.0f);
            p->velocity.z = 0;
        } else
            p->target_velocity.y = 0;
    }
    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *p = &world->push_blocks[index];
        if ((p->flags_0ca & 4) && !(p->packed_state_flags & 0x04000100) && !(p->flags_0cb & 2) &&
            (p->runtime_flags_0c8 & 0x69))
            MoveBlocks(world, p, index, &p->velocity);
    }
    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *p = &world->push_blocks[index];
        if (!(p->flags_0ca & 4) || (p->flags_0cb & 2))
            continue;
        if (!NuSpecialGetVisibilityFn(&p->special) || !(p->runtime_flags_0c8 & 0x4d))
            continue;
        NUVEC corners[4];
        f32 left = fabsf(p->bounds_min.x) - 0.006f, right = fabsf(p->bounds_max.x) - 0.006f;
        f32 back = fabsf(p->bounds_min.z) - 0.006f, front = fabsf(p->bounds_max.z) - 0.006f;
        for (i32 i = 0; i < 4; ++i)
            corners[i] = *p->position;
        f32 y = (corners[0].y - fabsf(p->bounds_min.y) + 0.001f) + 0.025f;
        corners[0].x -= left;
        corners[0].z -= back;
        corners[1].x += right;
        corners[1].z -= back;
        corners[2].x += right;
        corners[2].z += front;
        corners[3].x -= left;
        corners[3].z += front;
        if (p->ground_height > y)
            y = p->ground_height + 0.25f;
        for (i32 i = 0; i < 4; ++i)
            corners[i].y = y;
        PlatOnOff(p->platform_id, 0);
        bool platforms_off = p->supporting_block || p->previous_supporting_block;
        f32 heights[4];
        for (i32 i = 0; i < 4; ++i) {
            if (platforms_off)
                NewTerrPlatformsOff();
            heights[i] = GameShadow(NULL, &corners[i], 0.1f, -1);
            p->terrain_info[i] = ShadowInfo();
            p->extra_terrain_info[i] = EShadowInfo();
        }
        PlatOnOff(p->platform_id, 1);
        BlockInBlock(world, p, index, &p->supporting_block);
        if (p->supporting_block || p->previous_supporting_block) {
            p->previous_supporting_block = p->supporting_block;
            f32 support[4];
            TerrainBlockOnBlock(world, p, corners, support);
            for (i32 i = 0; i < 4; ++i)
                if (support[i] != -10000.0f)
                    heights[i] = support[i];
        }
        f32 low = p->ground_height - 0.01f, high = p->ground_height + 0.01f;
        if ((low > heights[0] && low > heights[1] && low > heights[2] && low > heights[3]) ||
            (heights[0] > high && heights[1] > high && heights[2] > high && heights[3] > high)) {
            p->runtime_flags_0c8 |= 8;
            p->ground_height = ((heights[0] + heights[1]) + heights[2] + heights[3]) * 0.25f + 0.01f;
            p->ground_offset = (p->position->y + p->bounds_min.y) - p->ground_height;
        }
        if (p->supporting_block) {
            p->ground_height = p->position->y + p->supporting_block->bounds_max.y;
            p->ground_offset = (p->position->y + p->bounds_min.y) - p->ground_height;
        }
        if (p->ground_offset > 0.01f)
            p->runtime_flags_0c8 |= 8;
        else if (p->runtime_flags_0c8 & 8) {
            p->runtime_flags_0c9 |= 8;
            p->settled_height = p->ground_height;
            p->runtime_flags_0c8 &= ~8;
        }
        i32 safe = 0;
        for (i32 i = 0; i < 4; ++i)
            if (p->extra_terrain_info[i] == 8 || p->terrain_info[i] == 14)
                ++safe;
        bool reset = safe != 4;
        if (world->current_level == HOTHESCAPEB_LDATA && NuVecXZDistSqr(p->position, &hothbtestpos, NULL) < 1.0f) {
            if (!(corners[0].z < 13.2f && corners[1].z < 13.2f && corners[2].z < 13.2f && corners[3].z < 13.2f))
                reset = true;
        }
        if (reset)
            p->runtime_flags_0c9 |= 4;
    }
    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *p = &world->push_blocks[index];
        if (!(p->flags_0ca & 4) || (p->runtime_flags_0c9 & 1) || (p->flags_0cb & 2))
            continue;
        if (!(p->runtime_flags_0c9 & 4) && (p->runtime_flags_0c8 & 1))
            GameAudio_PlaySfx(72, p->position, GameAudio_GetPlrSfxBits(p->pushing_object), 0);
        if (p->runtime_flags_0c9 & 0xc) {
            if (p->runtime_flags_0c9 & 4) {
                p->position->x = p->snap_origin.x;
                p->position->z = p->snap_origin.z;
                p->target_velocity.x = p->target_velocity.z = p->velocity.x = p->velocity.z = 0;
                memcpy(p->extra_terrain_info, &p->previous_extra_terrain_info, 4);
                p->runtime_flags_0c8 &= 0x3f;
                NuSpecialUpdate(&p->special);
                for (i32 i = 0; i < p->end_position_count; ++i)
                    if (NuSpecialExistsFn(&p->end_position_specials[i])) {
                        NUMTX *m = NuSpecialGetInstanceMtx(&p->end_position_specials[i]);
                        if ((usize)m != 0xffffffd0u) {
                            m->m30 = p->snap_origin.x;
                            m->m32 = p->snap_origin.z;
                            NuSpecialUpdate(&p->end_position_specials[i]);
                        }
                    }
                p->runtime_flags_0c8 &= ~1;
                p->runtime_flags_0c9 &= ~4;
            }
            if (p->runtime_flags_0c9 & 8) {
                p->ground_height = p->settled_height;
                p->position->y = p->settled_height - p->bounds_min.y;
                p->target_velocity.y = p->velocity.y = 0;
                p->runtime_flags_0c8 &= ~8;
                p->runtime_flags_0c9 &= ~8;
                p->ground_offset = (p->position->y + p->bounds_min.y) - p->settled_height;
                NuSpecialUpdate(&p->special);
            }
        } else if (p->runtime_flags_0c8 & 1) {
            if ((p->runtime_flags_0c8 & 3) == 1 && p->pushing_object)
                GameAudio_PlaySfx(71, &p->pushing_object->apiobj.collision_position,
                                  GameAudio_GetPlrSfxBits(p->pushing_object), 0);
            if (LEGOHINT_PUSHBLOCKS != -1 && p->pushing_object &&
                (p->pushing_object->apiobj.object_flags & 0x81) == 0x81)
                Hint_SetComplete(LEGOHINT_PUSHBLOCKS);
        }
    }
    for (i32 index = 0; index < world->push_block_count; ++index) {
        pushblock_s *p = &world->push_blocks[index];
        if (!(p->flags_0ca & 4))
            continue;
        if (p->pushing_object) {
            if (p->platform_id != -1 && (p->runtime_flags_0c8 & 9)) {
                p->pushing_object->field_0xefc |= 1;
                AddShoveObject(&p->special, (i16)p->platform_id);
            } else
                p->pushing_object->field_0xefc &= ~1;
        }
        if (!(p->packed_state_flags & 0x220)) {
            i32 safe = 0;
            for (i32 i = 0; i < 4; ++i)
                if (p->extra_terrain_info[i] == 8 || p->terrain_info[i] == 14)
                    ++safe;
            if (safe == 4) {
                memcpy(&p->previous_extra_terrain_info, p->extra_terrain_info, 4);
                p->snap_origin = *p->position;
            }
        }
        p->pushing_object = NULL;
        p->runtime_flags_0c8 = (p->runtime_flags_0c8 & ~2) | ((p->runtime_flags_0c8 & 1) << 1);
        p->runtime_flags_0c8 = (p->runtime_flags_0c8 & 0xea) | ((p->runtime_flags_0c8 << 1) & 0x10);
    }
}

static char *GizPush_GetGizmoName(GIZMO *gizmo) {
    if (gizmo == NULL) {
        return NULL;
    }
    return static_cast<pushblock_s *>(gizmo->object)->name;
}

i32 GizPush_GetOutput(GIZMO *gizmo, i32 output_index, i32) {
    pushblock_s *push_block = static_cast<pushblock_s *>(gizmo->object);
    if (push_block == NULL) {
        return 0;
    }
    return GizPushBlock_EndFrameCompleted(push_block, output_index) != 0;
}

char *GizPush_GetOutputName(GIZMO *gizmo, i32 output_index) {
    static char output_name[13] = "Any Complete";

    pushblock_s *push_block = static_cast<pushblock_s *>(gizmo->object);
    if (output_index < 0 || output_index > push_block->output_count) {
        return NULL;
    }
    if (output_index == 0) {
        return const_cast<char *>("Any Complete");
    }
    NuIToA(output_index, output_name, 10);
    NuStrCat(output_name, " Complete");
    return output_name;
}

i32 GizPush_GetNumOutputs(GIZMO *gizmo) {
    return static_cast<pushblock_s *>(gizmo->object)->output_count;
}

static void Push_Activate(GIZMO *gizmo, i32) {
    UNIMPLEMENTED();
}

static void Push_SetVisibility(GIZMO *gizmo, i32) {
    UNIMPLEMENTED();
}

static i32 Pushblocks_BoltHitPlat(void *, void *, BOLT *, unsigned char *) {
    UNIMPLEMENTED();
    return {};
}

static void *Push_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, 0x24c);
}

static void Push_ClearProgress(void *, void *progress_ptr) {
    PUSHPROGRESS *progress = static_cast<PUSHPROGRESS *>(progress_ptr);
    if (progress != NULL) {
        progress->state_mask = 0;
        progress->visible_mask = ~0u;
        progress->position_mask = 0;
        NuMemSet128(progress->positions, 0, 0);
    }
}

static void Push_StoreProgress(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    PUSHPROGRESS *progress = static_cast<PUSHPROGRESS *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    Push_ClearProgress(NULL, progress);
    if (world == NULL || world->push_blocks == NULL || world->push_block_count <= 0) {
        return;
    }

    for (i32 index = 0; index < world->push_block_count && index < 16; ++index) {
        pushblock_s *push_block = &world->push_blocks[index];
        const u32 mask = 1u << (index & 31);
        const i32 mask_index = index >> 5;
        u32 *visible_masks = &progress->visible_mask;
        u32 *state_masks = &progress->state_mask;
        u32 *position_masks = &progress->position_mask;

        if ((push_block->flags_0ca & PUSHBLOCK_FLAG_VISIBLE) == 0) {
            visible_masks[mask_index] &= ~mask;
        }
        if ((push_block->flags_0cb & PUSHBLOCK_FLAG_STATE) != 0) {
            state_masks[mask_index] |= mask;
        }

        NUMTX *matrix =
            NuSpecialExistsFn(&push_block->special) != 0 ? NuSpecialGetInstanceMtx(&push_block->special) : NULL;
        progress->positions[index] = matrix != NULL ? *reinterpret_cast<NUVEC *>(&matrix->m30) : v000;

        for (i32 end_index = 0; end_index < push_block->end_position_count; ++end_index) {
            nuhspecial_s *special = &push_block->end_position_specials[end_index];
            matrix = NuSpecialExistsFn(special) != 0 ? NuSpecialGetInstanceMtx(special) : NULL;
            progress->end_positions[end_index][index] =
                matrix != NULL ? *reinterpret_cast<NUVEC *>(&matrix->m30) : v000;
        }
        position_masks[mask_index] |= mask;
    }
}

static void Push_Reset(void *world, void *, void *progress) {
    ResetPushProgress(static_cast<WORLDINFO_s *>(world), progress);
}

static void *PushBlocks_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    world->push_blocks = NULL;
    world->push_block_count = 0;
    if (world->current_level->max_push_blocks != 0) {
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        world->push_blocks = static_cast<pushblock_s *>(world->giz_buffer.void_ptr);
        world->giz_buffer.addr += world->current_level->max_push_blocks * sizeof(pushblock_s);
    }
    return world->push_blocks;
}

static i32 edpush_Load(void *world_ptr, void *) {
    static i32 version = -1;

    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world->push_block_count != 0) {
        return 0;
    }

    version = EdFileReadInt();
    world->push_block_count = EdFileReadInt();
    i32 valid_count = 0;
    char special_name[256];
    for (i32 file_index = 0; file_index < world->push_block_count; ++file_index) {
        pushblock_s *push_block = &world->push_blocks[valid_count];
        memset(special_name, 0, sizeof(special_name));
        EdFileRead(special_name, EdFileReadChar());

        i32 special_missing = 1;
        if (Gizmo_FindNuSpecial(world->current_gscn, &push_block->special, special_name, 1, world->gizmo_sys,
                                const_cast<char *>("PushBlock"), push_block->name) != 0) {
            NuStrCpy(push_block->name, NuSpecialGetName(&push_block->special));
            special_missing = 0;
        }

        push_block->snap_distance = EdFileReadFloat();
        push_block->flags_0cb &= ~0x04;
        if (EdFileReadChar() != 0) {
            push_block->flags_0cb |= 0x04;
        }
        push_block->flags_0cb &= ~0x08;
        if (EdFileReadChar() != 0) {
            push_block->flags_0cb |= 0x08;
        }
        push_block->flags_0ca &= ~0x10;
        if (EdFileReadChar() != 0) {
            push_block->flags_0ca |= 0x10;
        }
        push_block->flags_0ca &= ~0x20;
        if (EdFileReadChar() != 0) {
            push_block->flags_0ca |= 0x20;
        }

        if (version > 3) {
            push_block->flags_0cb &= ~0x20;
            if (EdFileReadChar() != 0) {
                push_block->flags_0cb |= 0x20;
            }
            push_block->flags_0cb &= ~0x40;
            if (EdFileReadChar() != 0) {
                push_block->flags_0cb |= 0x40;
            }
        }
        if (version > 4) {
            push_block->flags_0ca &= ~0x40;
            if (EdFileReadChar() != 0) {
                push_block->flags_0ca |= 0x40;
            }
            push_block->flags_0ca &= ~0x80;
            if (EdFileReadChar() != 0) {
                push_block->flags_0ca |= 0x80;
            }
        }

        push_block->end_position_count = 0;
        if (version > 2) {
            const u8 end_position_count = static_cast<u8>(EdFileReadChar());
            push_block->end_position_count = end_position_count;
            for (i32 end_index = 0; end_index < end_position_count; ++end_index) {
                memset(special_name, 0, sizeof(special_name));
                EdFileRead(special_name, EdFileReadChar());
                special_name[16] = '\0';
                if (Gizmo_FindNuSpecial(world->current_gscn, &push_block->end_position_specials[end_index],
                                        special_name, 1, world->gizmo_sys, const_cast<char *>("PushBlockLinkObject"),
                                        push_block->name) == 0) {
                    --push_block->end_position_count;
                }
            }
        }
        if (special_missing == 0) {
            ++valid_count;
        }
    }
    world->push_block_count = valid_count;
    return 1;
}

ADDGIZMOTYPE *Push_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "PushBlocks";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0x24c;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = Push_GetMaxGizmos;
    addtype.fns.get_pos_fn = NULL;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = Push_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = Pushblocks_BoltHitPlat;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = UpdatePushBlocks;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = NULL;
    addtype.fns.get_gizmo_name_fn = GizPush_GetGizmoName;
    addtype.fns.get_output_fn = GizPush_GetOutput;
    addtype.fns.get_output_name_fn = GizPush_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizPush_GetNumOutputs;
    addtype.fns.activate_fn = Push_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = Push_SetVisibility;
    addtype.fns.allocate_progress_data_fn = Push_AllocateProgressData;
    addtype.fns.clear_progress_fn = Push_ClearProgress;
    addtype.fns.store_progress_fn = Push_StoreProgress;
    addtype.fns.reset_fn = Push_Reset;
    addtype.fns.reserve_buffer_space_fn = PushBlocks_ReserveBufferSpace;
    addtype.fns.load_fn = edpush_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;
    pushblock_gizmotype_id = type_id;

    return &addtype;
}
