#pragma once

#include "decomp.h"
#include "nu2api/nucore/common.h"

#include "nu2api/nusound/nusound_system.hpp"

// Android x86 naturally uses 4-byte alignment; the host needs the attribute
// to retain the target offsets of the embedded 64-bit fields.
class SAGA_HOST_PACKED_ALIGN4 NuSoundBuffer {
  public:
    struct SAGA_HOST_PACKED_ALIGN4 Context {
        u64 read_size;
        u64 size2;
        u64 size3;
        i32 field5_0x18;
        u8 flags;
        u8 padding_0x1d[3];
        i32 field5_0x20;

        Context() : read_size(0), size2(0), size3(0), field5_0x18(0), flags(1), field5_0x20(0) {
        }
    };

  private:
    u64 size;
    void *address;
    NuSoundMemoryBuffer *memory_buffer;
    i32 lock_count;
    i32 allocated;
    NuSoundSystem::MemoryDiscipline memory_discipline;
    Context context;

  public:
    NuSoundBuffer();
    NuSoundBuffer(char *name, u64 size);
    ~NuSoundBuffer();

    void Free();

    i32 Provide(char *address, u64 size);

    i32 Allocate(u64 size, NuSoundSystem::MemoryDiscipline disc);

    void Lock();
    void Unlock();

    void SetCurrentContext(Context &context);

    Context &GetCurrentContext();

    void *GetAddress() const;

    bool IsAllocated() const;

    u64 GetBufferSize() const;

    bool IsLocked() const;

    void *GetSegmentAddress(unsigned int index, unsigned int segments, unsigned int alignment) const;
    u32 GetSegmentSize(unsigned int segments, unsigned int alignment) const;
};
