#pragma once

#include <pthread.h>

#include "nu2api/nucore/common.h"

class NuMemoryPool {
  public:
    class IEventHandler {
      public:
        virtual i32 AllocatePage(NuMemoryPool *pool, u32 _unknown, u32 _unknown2, const char *_unknown3) = 0;
        virtual i32 ReleasePage(NuMemoryPool *pool, void *ptr) = 0;
        virtual void ForceReleasePage(NuMemoryPool *pool, void *ptr) = 0;
        virtual void *AllocateLargeBlock(NuMemoryPool *pool, u32 size, u32 alignment, const char *_unknown3) = 0;
        virtual void FreeLargeBlock(NuMemoryPool *pool, void *ptr) = 0;
    };

  private:
    struct FreeBlock {
        FreeBlock volatile *next;
    };

    struct Page {
        Page *next;
        u32 size;
        void *ptr;
        u32 offset;
        u32 allocation_count;
    };

  public:
    class IVisitor {
      public:
        virtual void Visit(NuMemoryPool *pool) = 0;
        virtual ~IVisitor() = default;
    };

    void AddPage(void *ptr, u32 size);

    NuMemoryPool(IEventHandler *event_handler, u32 size, const char *name);
    ~NuMemoryPool();

    u32 GetAllocatedBytes();
    const char *GetDebugName() const;
    u32 GetFreeBytes();
    u32 GetLargeBlockBytes();
    u32 GetPagedBytes();
    FreeBlock volatile *InterlockedPop(FreeBlock volatile **out_head);
    void InterlockedPush(FreeBlock volatile **head, void *block);
    FreeBlock volatile *Merge(FreeBlock volatile *a, FreeBlock volatile *b);
    Page *Merge(Page *a, Page *b);
    FreeBlock volatile *MergeSort(FreeBlock volatile *list, u32 count);
    Page *MergeSort(Page *list, u32 count);
    void *PageAlloc(u32 size, const char *name);
    void ReleaseAllPages();
    void ReleaseUnreferencedPages();
    void ReleaseUnreferencedPages_OLD();
    static void VisitPools(IVisitor *visitor);

  private:
    static NuMemoryPool *m_firstPool;
    static pthread_mutex_t m_globalCriticalSection;

    NuMemoryPool *next;
    const char *name;
    IEventHandler *event_handler;
    u32 block_size;
    u32 free_bytes;
    u32 large_block_bytes;
    Page *pages;
    u8 reserved_0x1c[0x400];
    volatile bool page_list_stable;
    u8 reserved_0x41d[3];
    pthread_mutex_t mutex;
    u32 visited_page_count;
    u32 released_page_count;
    u32 recycled_page_count;
    u32 unknown_0x430;
    u32 unknown_0x434;
    u32 unknown_0x438;
    u32 unknown_0x43c;

    static void InterlockedAdd(volatile u32 *augend, u32 addend);
    static void InterlockedSub(volatile u32 *minuend, u32 subtrahend);
};
