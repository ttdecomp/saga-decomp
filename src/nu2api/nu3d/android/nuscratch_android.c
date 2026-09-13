// Android scratch allocator. The original TU owns the backing store and
// repeats the alignment/push sequence in each public allocation entry point.
#include "nu2api/nucore/numem.h"

extern "C" {
u8 PS2_SCRATCH_BASE[0x8000];
}

#include "nu2api/nucore/nuvuvec.hpp"

extern "C" {
static u8 *ps2_scratch_free;

void NuScratchReset(void) {
    ps2_scratch_free = PS2_SCRATCH_BASE;
}

void *NuScratchAlloc32(i32 size) {
    if (ps2_scratch_free == nullptr)
        NuScratchReset();
    u8 *previous = ps2_scratch_free;
    u8 *allocation = reinterpret_cast<u8 *>((reinterpret_cast<usize>(ps2_scratch_free) + 3) & ~usize(3));
    ps2_scratch_free = allocation + ((size + 3) & ~3);
    *reinterpret_cast<u8 **>(ps2_scratch_free) = previous;
    ps2_scratch_free += sizeof(previous);
    return allocation;
}

void *NuScratchAlloc64(i32 size) {
    if (ps2_scratch_free == nullptr)
        NuScratchReset();
    u8 *previous = ps2_scratch_free;
    u8 *allocation = reinterpret_cast<u8 *>((reinterpret_cast<usize>(ps2_scratch_free) + 7) & ~usize(7));
    ps2_scratch_free = allocation + ((size + 3) & ~3);
    *reinterpret_cast<u8 **>(ps2_scratch_free) = previous;
    ps2_scratch_free += sizeof(previous);
    return allocation;
}

void *NuScratchAlloc128(i32 size) {
    if (ps2_scratch_free == nullptr)
        NuScratchReset();
    u8 *previous = ps2_scratch_free;
    u8 *allocation = reinterpret_cast<u8 *>((reinterpret_cast<usize>(ps2_scratch_free) + 15) & ~usize(15));
    ps2_scratch_free = allocation + ((size + 3) & ~3);
    *reinterpret_cast<u8 **>(ps2_scratch_free) = previous;
    ps2_scratch_free += sizeof(previous);
    return allocation;
}

void NuScratchRelease(void) {
    ps2_scratch_free = *reinterpret_cast<u8 **>(ps2_scratch_free - sizeof(ps2_scratch_free));
}
}
