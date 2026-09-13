#include "nu2api/nucore/NuMemoryPool.h"
#include "nu2api/nucore/numemory.h"

void NuMemoryPool::AddPage(void *ptr, u32 size) {
    Page *page = NU_ALLOC_T(Page, NuMemoryManager::MEM_ALLOC_SET_TO_ZERO, "", NUMEMORY_CATEGORY_NONE);
    page->size = size;
    page->ptr = ptr;
    page->offset = 0;
    page->allocation_count = 0;

    pthread_mutex_lock(&mutex);
    page_list_stable = false;
    page->next = pages;
    pages = page;
    page_list_stable = true;
    pthread_mutex_unlock(&mutex);

    InterlockedAdd(&free_bytes, page->size);
}

u32 NuMemoryPool::GetAllocatedBytes() {
    return GetPagedBytes() - free_bytes;
}

const char *NuMemoryPool::GetDebugName() const {
    return name;
}

u32 NuMemoryPool::GetFreeBytes() {
    return free_bytes;
}

u32 NuMemoryPool::GetLargeBlockBytes() {
    return large_block_bytes;
}

u32 NuMemoryPool::GetPagedBytes() {
    u32 paged_bytes = 0;

    pthread_mutex_lock(&mutex);
    page_list_stable = false;
    for (Page *page = pages; page != NULL; page = page->next) {
        paged_bytes += page->size;
    }
    page_list_stable = true;
    pthread_mutex_unlock(&mutex);

    return paged_bytes;
}

void NuMemoryPool::Merge(NuMemoryPool::FreeBlock volatile *, NuMemoryPool::FreeBlock volatile *) {
}

void NuMemoryPool::Merge(NuMemoryPool::Page *, NuMemoryPool::Page *) {
}

void NuMemoryPool::MergeSort(NuMemoryPool::FreeBlock volatile *, u32) {
}

void NuMemoryPool::MergeSort(NuMemoryPool::Page *, u32) {
}

NuMemoryPool::NuMemoryPool(NuMemoryPool::IEventHandler *, u32, char const *) {
}

void NuMemoryPool::PageAlloc(u32, char const *) {
}

void NuMemoryPool::ReleaseAllPages() {
}

void NuMemoryPool::ReleaseUnreferencedPages() {
}

void NuMemoryPool::ReleaseUnreferencedPages_OLD() {
}

void NuMemoryPool::VisitPools(NuMemoryPool::IVisitor *) {
}

NuMemoryPool::~NuMemoryPool() {
}

void NuMemoryPool::InterlockedAdd(volatile u32 *augend, u32 addend) {
    u32 expected;
    u32 actual;

    do {
        expected = *augend;
        actual = __sync_val_compare_and_swap(augend, expected, addend + expected);
    } while (actual != expected);
}

void NuMemoryPool::InterlockedSub(volatile u32 *minuend, u32 subtrahend) {
    u32 expected;
    u32 actual;

    do {
        expected = *minuend;
        actual = __sync_val_compare_and_swap(minuend, expected, expected - subtrahend);
    } while (actual != expected);
}

void NuMemoryPool::InterlockedPush(FreeBlock volatile **head, void *ptr) {
    FreeBlock volatile *block = static_cast<FreeBlock *>(ptr);
    FreeBlock volatile *expected;
    FreeBlock volatile *actual;
    do {
        expected = *head;
        block->next = expected;
        actual = __sync_val_compare_and_swap(head, expected, block);
    } while (actual != expected);
}

NuMemoryPool::FreeBlock volatile *NuMemoryPool::InterlockedPop(FreeBlock volatile **head) {
    FreeBlock volatile *expected;
    FreeBlock volatile *actual;
    do {
        expected = *head;
        if (expected == NULL) {
            return NULL;
        }
        actual = __sync_val_compare_and_swap(head, expected, expected->next);
    } while (actual != expected);
    return expected;
}
