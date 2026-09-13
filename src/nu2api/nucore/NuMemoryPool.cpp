#include "nu2api/nucore/NuMemoryPool.h"
#include "nu2api/nucore/numemory.h"

#include <string.h>

NuMemoryPool *NuMemoryPool::m_firstPool;
pthread_mutex_t NuMemoryPool::m_globalCriticalSection;

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

NuMemoryPool::FreeBlock volatile *NuMemoryPool::Merge(FreeBlock volatile *a, FreeBlock volatile *b) {
    FreeBlock volatile *head = NULL;
    FreeBlock volatile *tail = NULL;
    while (a != NULL && b != NULL) {
        if (reinterpret_cast<usize>(a) < reinterpret_cast<usize>(b)) {
            if (tail != NULL) {
                tail->next = a;
            } else {
                head = a;
            }
            tail = a;
            a = a->next;
        } else {
            if (tail != NULL) {
                tail->next = b;
            } else {
                head = b;
            }
            tail = b;
            b = b->next;
        }
    }
    while (a != NULL || b != NULL) {
        if (a != NULL) {
            if (tail != NULL) {
                tail->next = a;
            } else {
                head = a;
            }
            tail = a;
            a = a->next;
        } else {
            if (tail != NULL) {
                tail->next = b;
            } else {
                head = b;
            }
            tail = b;
            b = b->next;
        }
    }
    tail->next = NULL;
    return head;
}

NuMemoryPool::Page *NuMemoryPool::Merge(Page *a, Page *b) {
    Page *head = NULL;
    Page *tail = NULL;
    while (a != NULL && b != NULL) {
        if (reinterpret_cast<usize>(a->ptr) < reinterpret_cast<usize>(b->ptr)) {
            if (tail != NULL) {
                tail->next = a;
            } else {
                head = a;
            }
            tail = a;
            a = a->next;
        } else {
            if (tail != NULL) {
                tail->next = b;
            } else {
                head = b;
            }
            tail = b;
            b = b->next;
        }
    }
    while (a != NULL || b != NULL) {
        if (a != NULL) {
            if (tail != NULL) {
                tail->next = a;
            } else {
                head = a;
            }
            tail = a;
            a = a->next;
        } else {
            if (tail != NULL) {
                tail->next = b;
            } else {
                head = b;
            }
            tail = b;
            b = b->next;
        }
    }
    tail->next = NULL;
    return head;
}

NuMemoryPool::FreeBlock volatile *NuMemoryPool::MergeSort(FreeBlock volatile *head, u32 count) {
    if (count <= 1) {
        return head;
    }

    u32 left_count = count / 2;
    u32 right_count = count - left_count;
    FreeBlock volatile *left_end = head;
    FreeBlock volatile *right = NULL;
    for (u32 i = 0; i < left_count; ++i) {
        right = left_end->next;
        if (i + 1 < left_count) {
            left_end = right;
        }
    }
    left_end->next = NULL;
    FreeBlock volatile *sorted_left = MergeSort(head, left_count);
    FreeBlock volatile *sorted_right = MergeSort(right, right_count);
    return Merge(sorted_left, sorted_right);
}

NuMemoryPool::Page *NuMemoryPool::MergeSort(Page *head, u32 count) {
    if (count <= 1) {
        return head;
    }

    u32 left_count = count / 2;
    u32 right_count = count - left_count;
    Page *left_end = head;
    Page *right = NULL;
    for (u32 i = 0; i < left_count; ++i) {
        right = left_end->next;
        if (i + 1 < left_count) {
            left_end = right;
        }
    }
    left_end->next = NULL;
    Page *sorted_left = MergeSort(head, left_count);
    Page *sorted_right = MergeSort(right, right_count);
    return Merge(sorted_left, sorted_right);
}

NuMemoryPool::NuMemoryPool(IEventHandler *handler, u32 size, const char *debug_name) {
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &attributes);
    pthread_mutexattr_destroy(&attributes);

    name = debug_name;
    event_handler = handler;
    block_size = size;
    free_bytes = 0;
    large_block_bytes = 0;
    pages = NULL;
    page_list_stable = true;
    memset(reserved_0x1c, 0, sizeof(reserved_0x1c));

    pthread_mutex_lock(&m_globalCriticalSection);
    next = m_firstPool;
    m_firstPool = this;
    pthread_mutex_unlock(&m_globalCriticalSection);
}

void *NuMemoryPool::PageAlloc(u32 size, const char *name) {
    pthread_mutex_lock(&mutex);
    page_list_stable = false;

    Page *page = pages;
    if (page == NULL || size > page->size - page->offset) {
        if (page != NULL) {
            Page *previous = page;
            Page *candidate = page->next;
            while (candidate != NULL) {
                // The original rotates the first later page that is also full;
                // usable later pages are left in place.
                if (size > candidate->size - candidate->offset) {
                    previous->next = candidate->next;
                    candidate->next = pages;
                    pages = candidate;
                    break;
                }
                previous = candidate;
                candidate = candidate->next;
            }
        }
        event_handler->AllocatePage(this, size, block_size, name);
        page = pages;
    }

    u32 offset = page->offset;
    void *result = reinterpret_cast<void *>(reinterpret_cast<usize>(page->ptr) + offset);
    page->offset = (offset + size + block_size - 1) & -block_size;
    ++page->allocation_count;

    page_list_stable = true;
    pthread_mutex_unlock(&mutex);
    return result;
}

void NuMemoryPool::ReleaseAllPages() {
    pthread_mutex_lock(&mutex);
    page_list_stable = false;

    for (Page *page = pages; page != NULL;) {
        Page *next_page = page->next;
        event_handler->ForceReleasePage(this, page->ptr);
        InterlockedSub(&free_bytes, page->size);
        NU_FREE(page);
        page = next_page;
    }
    pages = NULL;
    memset(reserved_0x1c, 0, sizeof(reserved_0x1c));
    page_list_stable = true;
    pthread_mutex_unlock(&mutex);
}

void NuMemoryPool::ReleaseUnreferencedPages() {
    pthread_mutex_lock(&mutex);
    page_list_stable = false;

    u32 page_count = 0;
    for (Page *page = pages; page != NULL; page = page->next) {
        ++page_count;
    }
    if (page_count > 1) {
        pages = MergeSort(pages, page_count);
    }

    FreeBlock volatile **free_lists = reinterpret_cast<FreeBlock volatile **>(reserved_0x1c);
    for (u32 i = 0; i < 256; ++i) {
        u32 block_count = 0;
        for (FreeBlock volatile *block = free_lists[i]; block != NULL; block = block->next) {
            ++block_count;
        }
        if (block_count > 1) {
            free_lists[i] = MergeSort(free_lists[i], block_count);
        }
    }

    FreeBlock volatile *cursors[256];
    memcpy(cursors, free_lists, sizeof(cursors));

    Page *previous = NULL;
    Page *recycled = NULL;
    u32 visited_count = 0;
    u32 released_count = 0;
    u32 recycled_count = 0;
    for (Page *page = pages; page != NULL;) {
        ++visited_count;
        usize page_begin = reinterpret_cast<usize>(page->ptr);
        usize page_end = page_begin + page->size;
        u32 free_block_count = 0;
        for (u32 i = 0; i < 256; ++i) {
            FreeBlock volatile *block = cursors[i];
            while (block != NULL && reinterpret_cast<usize>(block) < page_end) {
                block = block->next;
                ++free_block_count;
            }
            cursors[i] = block;
        }

        if (free_block_count != page->allocation_count) {
            previous = page;
            page = page->next;
            continue;
        }

        for (u32 i = 0; i < 256; ++i) {
            FreeBlock volatile *block = free_lists[i];
            FreeBlock volatile *previous_block = NULL;
            while (block != NULL && reinterpret_cast<usize>(block) < page_end) {
                FreeBlock volatile *next_block = block->next;
                if (reinterpret_cast<usize>(block) >= page_begin) {
                    if (previous_block != NULL) {
                        previous_block->next = next_block;
                    } else {
                        free_lists[i] = next_block;
                    }
                } else {
                    previous_block = block;
                }
                block = next_block;
            }
        }

        Page *next_page = page->next;
        if (previous != NULL) {
            previous->next = next_page;
        } else {
            pages = next_page;
        }
        if (event_handler->ReleasePage(this, page->ptr)) {
            InterlockedSub(&free_bytes, page->size);
            NU_FREE(page);
            ++released_count;
        } else {
            page->allocation_count = 0;
            page->offset = 0;
            page->next = recycled;
            recycled = page;
            ++recycled_count;
        }
        page = next_page;
    }

    while (recycled != NULL) {
        Page *next_page = recycled->next;
        recycled->next = pages;
        pages = recycled;
        recycled = next_page;
    }

    visited_page_count = visited_count;
    released_page_count = released_count;
    recycled_page_count = recycled_count;
    unknown_0x434 = 0;
    unknown_0x438 = 0;
    unknown_0x43c = 0;
    page_list_stable = true;
    pthread_mutex_unlock(&mutex);
}

void NuMemoryPool::ReleaseUnreferencedPages_OLD() {
}

void NuMemoryPool::VisitPools(IVisitor *visitor) {
    pthread_mutex_lock(&m_globalCriticalSection);
    for (NuMemoryPool *pool = m_firstPool; pool != NULL; pool = pool->next) {
        visitor->Visit(pool);
    }
    pthread_mutex_unlock(&m_globalCriticalSection);
}

NuMemoryPool::~NuMemoryPool() {
    ReleaseUnreferencedPages();

    pthread_mutex_lock(&m_globalCriticalSection);
    if (m_firstPool == this) {
        m_firstPool = next;
    } else {
        NuMemoryPool *previous = m_firstPool;
        while (previous != NULL && previous->next != this) {
            previous = previous->next;
        }
        if (previous != NULL) {
            previous->next = next;
        }
    }
    pthread_mutex_unlock(&m_globalCriticalSection);

    pthread_mutex_destroy(&mutex);
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
