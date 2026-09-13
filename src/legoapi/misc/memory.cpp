#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/android/nuptl_android.h"

#include <string.h>

// MemoryManager::AllocPool @0x425740.
//
// The manager is initialized by LoadPermData. Its pointer-bearing fields use
// native pointer width while retaining the original layout in the 32-bit build.
// Pools are 32-byte classes; a freed block is pushed on its class list and
// recycled by the pop path below (memset of the requested size only), while a
// fresh carve rounds up to the full 32-byte block and updates the counters.
void *MemoryManager::AllocPool(u32 size, i32 zero) {
    const i32 adjusted_size =
        static_cast<i32>(size - 1) < 0 ? static_cast<i32>(size + 0x1e) : static_cast<i32>(size - 1);
    const i32 size_class = adjusted_size >> 5;

    void **free_head = &free_lists[size_class];
    void *item = *free_head;
    if (item != NULL) {
        *free_head = *reinterpret_cast<void **>(item);
        if (zero != 0) {
            memset(item, 0, size);
        }
        return item;
    }

    const usize block = static_cast<usize>(size_class + 1) << 5;
    if (*end_cell - *cursor_cell <= block) {
        return NULL;
    }
    const usize aligned = ALIGN(*cursor_cell, 0x10);
    *cursor_cell = aligned;
    item = reinterpret_cast<void *>(aligned);
    *cursor_cell = aligned + block;
    memset(item, 0, block);
    allocated += block;
    remaining -= block;
    high_water = *cursor_cell;
    return item;
}

// MemoryManager::FreePool @0x425810: pushes the block onto its size class's
// free list (the next AllocPool of that class pops it, first word = next).
void MemoryManager::FreePool(void *item, u32 size) {
    const i32 adjusted_size =
        static_cast<i32>(size - 1) < 0 ? static_cast<i32>(size + 0x1e) : static_cast<i32>(size - 1);
    const i32 size_class = adjusted_size >> 5;

    void **free_head = &free_lists[size_class];
    *reinterpret_cast<void **>(item) = *free_head;
    *free_head = item;
}

void DebFreeChunksInstantly(i32 *);
void RemoveDebrisEffectFromStack(debkeydatatype_s *);
void RemoveAnyChunkControls(i32 *);
void AddChunkControlToStack(debris_chunk_control_s *, debris_chunk_control_s **);
void DebrisGetControlStackLock();
void DebrisReleaseControlStackLock();
void RemoveChunkFromRenderStack(particlechunkrendertype_s *, particlechunkrendertype_s **);

extern "C" {

    extern debkeydatatype_s *debkeydata;
    extern i16 *freedebkeys;
    extern i32 freedebkeyptr;
    extern i32 maxdebkeys;
    extern debinftype **debtab;
    extern i32 debrischunks;
    extern i32 debrischunksglass;
    extern i32 freedebchkptr;
    extern i32 freedebchkptrg;
    extern dma_particle_chunk_s **freedebchunks;
    extern dma_particle_chunk_s **freedebchunksglass;
    extern particlechunkrendertype_s *ParticleChunkToRender;
    extern particlechunkrendertype_s *ParticleChunkRenderStack[5];


    void CrashDataPtr(void) {
    }

    i32 DebAlloc(void) {
        if (freedebkeyptr >= maxdebkeys) {
            return -1;
        }

        const i32 slot = freedebkeys[freedebkeyptr++];
        debkeydatatype_s *key = debkeydata + slot;
        for (i32 i = 0; i < 32; ++i) {
            key->particle_chunks[i] = NULL;
        }
        key->allocated_chunk_count = 0;
        key->field_18a = 0;
        key->particle_count = 0;
        key->previous_allocated_chunk_count = 0;
        key->previous_particle_count = 0;
        key->controlled_chunk_count = 0;
        key->field_2f2 = -1;
        key->gscene = NULL;
        key->field_2f4 = 1;
        key->field_2f6 = 1;
        key->field_2f7 = 1;
        key->field_32c = 0;
        key->allocation_index = slot;
        key->field_2fa = 0;
        return slot;
    }

    extern debris_chunk_control_s **freechunkcontrols;
    extern i32 freechunkcontrolsptr;
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
    extern f32 globaltime, panelglobaltime;

    void DebFree(i32 *handle) {
        if (*handle == -1)
            return;
        debkeydatatype_s *key = &debkeydata[*handle];
        if (key->allocation_locked != 0)
            return;
        key->allocation_locked = 1;
        for (i32 i = freedebkeyptr; i < maxdebkeys; ++i) {
            if (freedebkeys[i] == *handle) {
                key->allocation_locked = 0;
                *handle = -1;
                return;
            }
        }
        if (key->effect_index == 0) {
            key->allocation_locked = 0;
            return;
        }
        debinftype *effect = debtab[key->effect_index];
        if (key->allocated_chunk_count != 0) {
            DebrisGetControlStackLock();
            for (i32 i = 0; i < key->allocated_chunk_count; ++i) {
                RemoveAnyChunkControls(reinterpret_cast<i32 *>(key->particle_chunks[i]));
                debris_chunk_control_s *control = freechunkcontrols[freechunkcontrolsptr];
                control->particle_chunk = key->particle_chunks[i];
                control->active = effect->particle_type == 7 ? 7 : 0;
                control->owner = NULL;
                const i32 panel = effect->time_group == 4;
                control->expiry_time = effect->particle_lifetime + (panel ? panelglobaltime : globaltime) +
                                       static_cast<i8>(effect->trail_count) * effect->trail_time;
                AddChunkControlToStack(control, &debris_chunk_control_stack[panel]);
                ++freechunkcontrolsptr;
            }
            DebrisReleaseControlStackLock();
            for (i32 i = 0; i < debrischunks + debrischunksglass; ++i) {
                particlechunkrendertype_s *render = &ParticleChunkToRender[i];
                if (render->particle_chunk == key->particle_chunks[0]) {
                    render->key = NULL;
                    render->effect = effect;
                    render->effect_orientation = key->effect_orientation;
                    render->position = key->position;
                    break;
                }
            }
        }
        i32 linked = 0;
        for (i32 i = 0; i < 8; ++i) {
            if (effect->particle_keys[i] == *handle) {
                effect->particle_keys[i] = -1;
                linked = 1;
            }
        }
        if (linked == 0)
            RemoveDebrisEffectFromStack(key);
        for (i32 i = 0; i < key->allocated_chunk_count; ++i)
            key->particle_chunks[i] = NULL;
        key->allocated_chunk_count = 0;
        key->effect_index = 0;
        key->allocation_index = -1;
        freedebkeys[--freedebkeyptr] = static_cast<i16>(*handle);
        key->allocation_locked = 0;
        *handle = -1;
    }

    void DebFreeAllChunksInstantly(void) {
    }

    void DebFreeInstantly(i32 *handle) {
        if (handle == NULL || *handle == -1) {
            return;
        }

        const i32 index = *handle;
        debkeydatatype_s *key = debkeydata + index;
        if (key->allocation_locked != 0) {
            return;
        }
        key->allocation_locked = 1;

        // A key that has not yet been linked into an effect's active list can
        // be returned immediately.  The original scans the free-key tail to
        // distinguish that case from a live key.
        if (freedebkeyptr < maxdebkeys) {
            i32 p = freedebkeyptr;
            while (p < maxdebkeys && freedebkeys[p] != index) {
                ++p;
            }
            if (p < maxdebkeys) {
                key->allocation_locked = 0;
                *handle = -1;
                return;
            }
        }

        const i16 effect_index = key->effect_index;
        if (effect_index == 0) {
            key->allocation_locked = 0;
            return;
        }

        debinftype *effect = debtab[effect_index];
        DebFreeChunksInstantly(handle);
        const i32 freed = *handle;
        bool linked = false;
        for (i32 i = 0; i != 8; ++i) {
            i16 *owner = &effect->particle_keys[i];
            if (*owner == freed) {
                *owner = -1;
                linked = true;
            }
        }
        if (!linked) {
            RemoveDebrisEffectFromStack(debkeydata + freed);
        }

        --freedebkeyptr;
        debkeydata[freed].effect_index = 0;
        debkeydata[freed].allocation_index = -1;
        freedebkeys[freedebkeyptr] = static_cast<i16>(freed);
        debkeydata[freed].allocation_locked = 0;
        *handle = -1;
    }

    void DebFreeOrphansInstantly(debinftype *effect) {
        DebrisGetControlStackLock();
        for (i32 i = 0; i < debrischunks + debrischunksglass; ++i) {
            particlechunkrendertype_s *chunk = &ParticleChunkToRender[i];
            if (chunk->key != NULL || chunk->effect != effect)
                continue;
            RemoveAnyChunkControls(reinterpret_cast<i32 *>(chunk->particle_chunk));
            debris_chunk_control_s *control = freechunkcontrols[freechunkcontrolsptr];
            control->particle_chunk = ParticleChunkToRender[i].particle_chunk;
            control->active = effect->particle_type == 7 ? 9 : 2;
            control->owner = NULL;
            i32 stack = effect->time_group == 4 ? 1 : 0;
            control->expiry_time = stack != 0 ? panelglobaltime : globaltime;
            AddChunkControlToStack(control, &debris_chunk_control_stack[stack]);
            ++freechunkcontrolsptr;
            RemoveChunkFromRenderStack(&ParticleChunkToRender[i],
                                       &ParticleChunkRenderStack[static_cast<i8>(effect->time_group)]);
            ParticleChunkToRender[i].particle_chunk = NULL;
            ParticleChunkToRender[i].effect = NULL;
        }
        DebrisReleaseControlStackLock();
    }

    void DebReAlloc2(debkeydatatype_s *key);

    void DebReAlloc(debkeydatatype_s *key, i32 particle_count) {
        debinftype *effect = debtab[key->effect_index];
        key->previous_particle_count = static_cast<i16>(particle_count);
        i32 chunk_count = effect->particle_type == 7 ? (particle_count + 11) / 12 : (particle_count + 31) / 32;
        if (chunk_count > 32) {
            chunk_count = 32;
            key->previous_particle_count = effect->particle_type == 7 ? 0x180 : 0x400;
        }
        key->previous_allocated_chunk_count = static_cast<i16>(chunk_count);
        if (key->allocated_chunk_count == chunk_count) {
            key->particle_count = key->previous_particle_count;
        } else if (chunk_count != 0 && key->allocated_chunk_count == 0) {
            DebReAlloc2(key);
        }
    }

    extern "C++" void AddChunkToRenderStack(particlechunkrendertype_s *, particlechunkrendertype_s **);

    void DebReAlloc2(debkeydatatype_s *key) {
        debinftype *effect = debtab[key->effect_index];
        const i32 old_chunk_count = key->allocated_chunk_count;
        const i32 requested_chunk_count = key->previous_allocated_chunk_count;
        const i32 additional_chunks = requested_chunk_count - old_chunk_count;
        if (additional_chunks <= 0) {
            return;
        }

        const bool glass = effect->particle_type == 7;
        i32 &free_chunk_count = glass ? freedebchkptrg : freedebchkptr;
        const i32 available_chunk_count = glass ? debrischunksglass : debrischunks;
        dma_particle_chunk_s **free_chunks = glass ? freedebchunksglass : freedebchunks;
        if (available_chunk_count <= free_chunk_count + additional_chunks) {
            return;
        }

        const i32 particles_per_chunk = glass ? 12 : 32;
        for (i32 i = 0; i < additional_chunks; ++i) {
            dma_particle_chunk_s *chunk = free_chunks[free_chunk_count + i];
            key->particle_chunks[old_chunk_count + i] = chunk;
            for (i32 particle_index = 0; particle_index < particles_per_chunk; ++particle_index) {
                chunk->particles[particle_index].start_time = 0.0f;
                chunk->particles[particle_index].inverse_lifetime = 32768.0f;
            }
        }

        if (old_chunk_count == 0) {
            const i32 render_chunk_count = debrischunks + debrischunksglass;
            particlechunkrendertype_s *render_chunk = NULL;
            for (i32 i = 0; i < render_chunk_count; ++i) {
                if (ParticleChunkToRender[i].particle_chunk == NULL) {
                    render_chunk = &ParticleChunkToRender[i];
                    break;
                }
            }
            if (render_chunk != NULL) {
                render_chunk->particle_chunk = key->particle_chunks[0];
                render_chunk->effect = effect;
                render_chunk->key = key;
                render_chunk->render_priority = key->render_priority;
                if (key->field_2f6 != 0) {
                    AddChunkToRenderStack(render_chunk, &ParticleChunkRenderStack[effect->time_group]);
                }
            }
        }

        free_chunk_count += additional_chunks;
        key->allocated_chunk_count = static_cast<i16>(requested_chunk_count);
        key->particle_count = key->previous_particle_count;
        LinkDmaParticalSets(key->particle_chunks, requested_chunk_count);
    }

} // extern "C"
