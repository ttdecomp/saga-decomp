#pragma once

#include "decomp.h"
#include "nu2api/nucore/common.h"

#include <pthread.h>

class NuSoundMemoryBuffer {
    friend class NuSoundMemoryManager;

    void *address;
    u32 size : 30;
    bool alloced : 1;
    bool locked : 1;
    NuSoundMemoryBuffer *prev;
    NuSoundMemoryBuffer *next;

  private:
    static pthread_mutex_t s_cs;

    static void BeginCriticalSection();
    static void EndCriticalSection();

  public:
    NuSoundMemoryBuffer();
    ~NuSoundMemoryBuffer();

    void SetNext(NuSoundMemoryBuffer *next);
    void SetPrev(NuSoundMemoryBuffer *prev);
    void SetSize(u32 size);
    void SetAddress(void *address);
    void SetAlloced(bool alloced);
    void *Lock(const char *name);
    void Unlock();

    void *GetAddress();
    NuSoundMemoryBuffer *GetNext();
    NuSoundMemoryBuffer *GetPrev();
    u32 GetSize();
    bool IsAlloced();
    bool IsLocked();
    const char *GetLockReason(); // 0x321360: lock-reason getter, always NULL on device
} __attribute__((packed));       // Preserves target member-access codegen despite the same natural x86 size.

DECOMP_ASSERT(sizeof(NuSoundMemoryBuffer) == 0x10, "NuSoundMemoryBuffer size");

class NuSoundMemoryManager {
    void *memory;
    u32 size;
    u32 align;
    u32 field3_0xc;
    u16 name_length;
    u16 name_length2;
    const char *name;
    NuSoundMemoryBuffer *free_list_head;
    void *memory2;
    u32 size2;
    u32 free_bytes;
    pthread_mutex_t mutex;
    u8 flags;
    u32 free_count;

  public:
    NuSoundMemoryManager();
    ~NuSoundMemoryManager();

    u32 Init(const char *name, void *memory, u32 size, u32 align, u32 param_5);

    static NuSoundMemoryBuffer *PopFreeBuffer();
    void PushFreeBuffer(NuSoundMemoryBuffer *buffer);

    void EnableDefragOnAlloc(bool value);

    NuSoundMemoryBuffer *Alloc(u32 size);
    NuSoundMemoryBuffer *Defragment(u32 size);
    u32 GetFree();
    void Free(NuSoundMemoryBuffer *buffer);
    NuSoundMemoryBuffer *MergeFreeBuffer(NuSoundMemoryBuffer *buffer);
    NuSoundMemoryBuffer *SplitFreeBuffer(NuSoundMemoryBuffer *buffer, u32 size, NuSoundMemoryBuffer **remainder);

    bool Release();

    u32 GetSize(); // 0x3223b0: the pool's total byte size (device +0x20)
    u32 GetUsed(); // 0x322590: GetSize() minus the free counter

    // Debug/diagnostic helpers (device addresses in nusound_memorymanager.cpp).
    void *AllocAddress(u32 size);
    bool CheckList();
    u32 CountAdjacentFreeBuffers(NuSoundMemoryBuffer *buffer);
    void EnableDebug(bool enable);
    void EnableDefragOnFree(bool enable);
    void FreeAddress(void *address);
    NuSoundMemoryBuffer *MoveLargestTrailingBufferIntoBuffer(NuSoundMemoryBuffer *buffer, NuSoundMemoryBuffer **out_a,
                                                             NuSoundMemoryBuffer **out_b);
    void OutputList();
    void OutputMap();
    void RenderMap(f32 x, f32 y, f32 scale);
    NuSoundMemoryBuffer *SwapOrMergeAdjacentBuffers(NuSoundMemoryBuffer *buffer);
    bool SwapSimilarBuffers(NuSoundMemoryBuffer *a, NuSoundMemoryBuffer *b);

  private:
    NuSoundMemoryBuffer *CheckAndMergeFreeBufferNext(NuSoundMemoryBuffer *buffer);
    NuSoundMemoryBuffer *CheckAndMergeFreeBufferPrev(NuSoundMemoryBuffer *buffer);
};
