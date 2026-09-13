#include "nu2api/nucore/NuMemoryManager.h"

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nustring.h"

#include "decomp.h"

#define HEADER_MGR_HI_MASK 0xf8000000
#define ALLOC_MASK 0x78000000
#define BLOCK_SIZE_MASK ~ALLOC_MASK
#define BLOCK_SIZE(header_val) ((header_val) & BLOCK_SIZE_MASK) * 4
#define END_TAG(block, size) (usize *)((usize)block + size - 4)
#define END_TAG_HI(block, size) (usize *)((usize)block + size - 8)

#define STRANDED_DUMP_SUFFIX "_stranded.txt"

u32 NuMemoryManager::m_flags;
pthread_mutex_t *NuMemoryManager::m_globalCriticalSection;
pthread_mutex_t NuMemoryManager::m_globalCriticalSectionBuff;
u32 NuMemoryManager::m_headerSize;

// A table of values for counting leading zeros in the binary representation of
// a 32-bit integer.
static char g_clzTable[] = {
    0x00, 0x1f, 0x09, 0x1e, 0x03, 0x08, 0x12, 0x1d, 0x02, 0x05, 0x07, 0x0e, 0x0c, 0x11, 0x16, 0x1c,
    0x01, 0x0a, 0x04, 0x13, 0x06, 0x0f, 0x0d, 0x17, 0x0b, 0x14, 0x10, 0x18, 0x15, 0x19, 0x1a, 0x1b,
};

static inline u32 CountLeadingZeros(u32 value) {
    // An almost-branchless algorithm for calculating the leading zeros of a
    // binary value by exploiting de Bruijn sequences. The value is hashed
    // using a perfect hash function and the hash provides the index to a table
    // of leading zero counts.
    value |= value >> 0x01;
    value |= value >> 0x02;
    value |= value >> 0x04;
    value |= value >> 0x08;
    value |= value >> 0x10;

    if (value != 0) {
        return g_clzTable[((value + 1) * 0x07dcd629) >> 0x1b];
    }

    return 0x20;
}

NuMemoryManager *NuMemoryManager::m_memoryManagers[0x100];

NuMemoryManager::NuMemoryManager(IEventHandler *event_handler, IErrorHandler *error_handler, const char *name,
                                 const char **category_names, u32 category_count) {
    pthread_mutexattr_t attrs;

    this->event_handler = event_handler;
    this->error_handler = error_handler;

    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&this->mutex, &attrs);
    pthread_mutexattr_destroy(&attrs);

    pthread_mutexattr_init(&attrs);
    pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&this->error_mutex, &attrs);
    pthread_mutexattr_destroy(&attrs);

    this->category_names = category_names;
    this->category_count = category_count;

    // The default initializers almost get these right, but they're out of order
    // and compile fails when trying to specify designated initializers out of
    // order.
    this->initial_state_ctx.used_block_count = 0;
    this->initial_state_ctx.id = 0;
    this->initial_state_ctx.next = 0;
    this->initial_state_ctx.name = 0;

    this->stranded_ctx.used_block_count = 0;
    this->stranded_ctx.id = 0;
    this->stranded_ctx.next = 0;
    this->stranded_ctx.name = 0;

    this->unknown_1814 = 0x4000;
    this->unknown_1818 = 0;

    this->is_zombie = false;

    strcpy(this->name, name);

    this->pages = NULL;

    this->stranded_blocks = NULL;
    this->stranded_block_count = 0;

    memset(this->small_bins, 0, sizeof(this->small_bins));
    memset(this->large_bins, 0, sizeof(this->large_bins));

    memset(this->small_bin_has_free_map, 0, sizeof(this->small_bin_has_free_map));

    this->unknown_0e2c = 0;
    this->unknown_0e30 = 0;

    this->large_bin_has_free_map = 0;
    this->large_bin_dirty_map = 0;

    memset(&this->stats, 0, sizeof(Stats));

    this->stats.min_free_bytes = this->stats.free_frag_bytes;

    this->initial_state_ctx.name = "Initial State";
    this->cur_ctx = &this->initial_state_ctx;

    this->stranded_ctx.name = "Stranded";

    this->override_category = 1;
    this->override_category_bg_thread = 1;

    this->stranded_ctx.id = 31;

    if (m_globalCriticalSection == NULL) {
        pthread_mutexattr_init(&attrs);
        pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_globalCriticalSectionBuff, &attrs);
        pthread_mutexattr_destroy(&attrs);

        m_globalCriticalSection = &m_globalCriticalSectionBuff;
    }

    pthread_mutex_lock(m_globalCriticalSection);

    for (i32 i = 0; i != 0x100; i++) {
        if (m_memoryManagers[i] == NULL) {
            this->idx = i;
            m_memoryManagers[i] = this;

            break;
        }
    }

    pthread_mutex_unlock(m_globalCriticalSection);
}

NuMemoryManager::~NuMemoryManager() {
    PopContext(POP_DEBUG_MODE_FREE_STRANDED_BLOCKS);
    ReleaseUnreferencedPages();

    pthread_mutex_lock(m_globalCriticalSection);

    m_memoryManagers[this->idx] = NULL;

    pthread_mutex_unlock(m_globalCriticalSection);

    pthread_mutex_destroy(&this->error_mutex);
    pthread_mutex_destroy(&this->mutex);
}

void NuMemoryManager::SetFlags(u32 flags) {
    m_flags = flags;

    if ((flags & MEM_MANAGER_EXTENDED_DEBUG) != 0) {
        m_flags |= MEM_MANAGER_DEBUG;
        m_headerSize = sizeof(ExtendedDebugHeader);
    } else if ((flags & MEM_MANAGER_DEBUG) != 0) {
        m_headerSize = sizeof(DebugHeader);
    } else {
        m_headerSize = sizeof(Header);
    }
}

void *NuMemoryManager::_BlockAlloc(u32 size, u32 alignment, u32 flags, const char *name, u16 category) {
    void *ptr = this->_TryBlockAlloc(size, alignment, flags, name, category);

    if (ptr == NULL) {
        this->____WE_HAVE_RUN_OUT_OF_MEMORY_____(size, name);
    }

    return ptr;
}

void *NuMemoryManager::_TryBlockAlloc(u32 size, u32 alignment, u32 flags, const char *name, u16 category) {
    ValidateAllocAlignment(alignment);
    ValidateAllocSize(size);

    alignment = MAX(alignment, 4);
    const u32 requested_size = CalculateBlockSize(size);
    const u32 minimum_fragment_size = m_headerSize + sizeof(FreeHeader);

    pthread_mutex_lock(&this->mutex);

    FreeHeader *fragment = NULL;
    Header *allocated = NULL;
    u32 allocated_size = 0;

    // The original uses the small/large availability maps to reach the same
    // bins quickly. Scanning the bin chains keeps the allocator semantics
    // straightforward while those search optimisations are reconstructed.
    for (u32 bin = 0; bin < 256 && allocated == NULL; ++bin) {
        for (FreeHeader *candidate = this->small_bins[bin].next; candidate != NULL; candidate = candidate->next) {
            const usize begin = (usize)candidate;
            const usize end = begin + BLOCK_SIZE(candidate->block_header.value);
            usize user = ALIGN(begin + m_headerSize, alignment);
            usize header_address = user - m_headerSize;
            u32 prefix_size = header_address - begin;
            if (prefix_size != 0 && prefix_size < minimum_fragment_size) {
                user = ALIGN(begin + m_headerSize + minimum_fragment_size, alignment);
                header_address = user - m_headerSize;
                prefix_size = header_address - begin;
            }
            if (header_address + requested_size <= end) {
                fragment = candidate;
                allocated = (Header *)header_address;
                allocated_size = requested_size;
                break;
            }
        }
    }
    for (u32 bin = 0; bin < 22 && allocated == NULL; ++bin) {
        for (FreeHeader *candidate = this->large_bins[bin].next; candidate != NULL; candidate = candidate->next) {
            const usize begin = (usize)candidate;
            const usize end = begin + BLOCK_SIZE(candidate->block_header.value);
            usize user = ALIGN(begin + m_headerSize, alignment);
            usize header_address = user - m_headerSize;
            u32 prefix_size = header_address - begin;
            if (prefix_size != 0 && prefix_size < minimum_fragment_size) {
                user = ALIGN(begin + m_headerSize + minimum_fragment_size, alignment);
                header_address = user - m_headerSize;
            }
            if (header_address + requested_size <= end) {
                fragment = candidate;
                allocated = (Header *)header_address;
                allocated_size = requested_size;
                break;
            }
        }
    }

    if (allocated == NULL) {
        pthread_mutex_unlock(&this->mutex);
        if (this->event_handler == NULL ||
            !this->event_handler->AllocatePage(this, requested_size + 0x4000, 0x1fffffff)) {
            return NULL;
        }
        return _TryBlockAlloc(size, alignment, flags, name, category);
    }

    const usize fragment_begin = (usize)fragment;
    const usize fragment_end = fragment_begin + BLOCK_SIZE(fragment->block_header.value);
    const usize allocated_begin = (usize)allocated;
    const u32 prefix_size = allocated_begin - fragment_begin;
    u32 suffix_size = fragment_end - (allocated_begin + allocated_size);

    BinUnlink(fragment);

    if (prefix_size != 0) {
        fragment->block_header.value = prefix_size / 4;
        *END_TAG(fragment, prefix_size) = fragment->block_header.value;
        BinLink(fragment, true);
    }

    if (suffix_size != 0 && suffix_size < minimum_fragment_size) {
        allocated_size += suffix_size;
        suffix_size = 0;
    }
    if (suffix_size != 0) {
        FreeHeader *suffix = (FreeHeader *)(allocated_begin + allocated_size);
        suffix->block_header.value = suffix_size / 4;
        *END_TAG(suffix, suffix_size) = suffix->block_header.value;
        suffix->next = NULL;
        suffix->prev = NULL;
        BinLink(suffix, true);
    }

    allocated->value = allocated_size / 4;
    ConvertToUsedBlock((FreeHeader *)allocated, alignment, flags, name, category);
    pthread_mutex_unlock(&this->mutex);
    return ClearUsedBlock(allocated, flags);
}

void NuMemoryManager::ConvertToUsedBlock(FreeHeader *header, u32 alignment, u32 flags, const char *name, u16 category) {
    u32 align_mask;
    u32 header_value;
    u32 aligned;
    usize *end_tag;
    u32 manager_idx;

    align_mask = -(CountLeadingZeros(alignment >> 2) << 0x1b);
    header_value = header->block_header.value & BLOCK_SIZE_MASK;

    manager_idx = this->idx;
    aligned = header_value | align_mask;

    header->block_header.value = aligned;

    // Set the end tag of the block. This is used for detecting corruption and
    // clearing the block.
    end_tag = END_TAG(header, header_value * 4);
    if ((align_mask & ALLOC_MASK) == 0) {
        *end_tag = aligned & BLOCK_SIZE_MASK;
    } else if (manager_idx >= 0x1d) {
        *end_tag = aligned | HEADER_MGR_HI_MASK;
        *(u32 *)((usize)header + BLOCK_SIZE(header->block_header.value) - 8) = manager_idx;
    } else {
        *end_tag = ((manager_idx + 1) << 0x1b) | aligned & BLOCK_SIZE_MASK;
    }

    this->stats.used_block_count++;
    this->stats.min_free_bytes = MIN(this->stats.min_free_bytes, this->stats.free_frag_bytes);

    if ((m_flags & MEM_MANAGER_DEBUG) != 0) {
        DebugHeader *debug = (DebugHeader *)header;
        debug->name = name;
        debug->category = category;
        debug->flags.alloc_flags = flags;
        debug->flags.ctx_id = static_cast<u16>(this->cur_ctx->id);
        debug->flags.unknown = debug->flags.ctx_id;

        this->stats.bytes_alloc_by_category[category] += BLOCK_SIZE(header->block_header.value);

        if ((m_flags & MEM_MANAGER_EXTENDED_DEBUG) != 0) {
            ExtendedDebugHeader *extended = (ExtendedDebugHeader *)header;

            memset(&extended->extended_info, 0, sizeof(ExtendedDebugInfo));
            extended->unknown = 0;
        }
    }
}

void *NuMemoryManager::ClearUsedBlock(Header *header, u32 flags) {
    u32 size;
    void *ptr;
    u32 size_minus_header;
    usize *end_tag;
    u32 tag_value;
    u32 manager_idx;
    u32 available_size;

    ptr = (void *)((usize)header + m_headerSize);
    size = BLOCK_SIZE(header->value);
    size_minus_header = size - m_headerSize;

    end_tag = END_TAG(header, size);

    tag_value = *end_tag >> 0x1b;
    if (tag_value != 0x1f) {
        manager_idx = tag_value - 1;
    } else {
        manager_idx = end_tag[-1];
    }

    available_size = manager_idx <= 0x1d ? size_minus_header - 4 : size_minus_header - 8;

    if ((flags & MEM_ALLOC_SET_TO_ZERO) != 0) {
        memset(ptr, 0, available_size);
    }

    return ptr;
}

void NuMemoryManager::____WE_HAVE_RUN_OUT_OF_MEMORY_____(u32 size, const char *name) {
    u32 largest_fragment_size;
    u32 free_bytes;
    char requested_str[14];
    char largest_fragment_size_str[14];
    char free_bytes_str[14];

    largest_fragment_size = CalculateLargestFragmentSize();
    free_bytes = CalculateFreeBytes();

    NuStrFormatSize(requested_str, sizeof(requested_str), size, true);
    NuStrFormatSize(largest_fragment_size_str, sizeof(largest_fragment_size_str), largest_fragment_size, true);
    NuStrFormatSize(free_bytes_str, sizeof(free_bytes_str), free_bytes, true);

    pthread_mutex_lock(&this->error_mutex);

    m_flags |= MEM_MANAGER_IN_ERROR_STATE;

    snprintf(this->error_msg, sizeof(this->error_msg),
             "** Out of memory **\nRequested = %s\nLargest Fragment = %s\nFree Bytes = %s\nFrom: %s\n", requested_str,
             largest_fragment_size_str, free_bytes_str, name);

    this->error_handler->HandleError(this, MEM_ERROR_OUT_OF_MEMORY, this->error_msg);

    pthread_mutex_unlock(&this->error_mutex);

    SetZombie();

    Dump(0x11, "memdump_out_of_memory.txt");
}

void NuMemoryManager::SetZombie() {
    this->is_zombie = true;
}

void NuMemoryManager::BlockFree(void *ptr, u32 flags) {

    NuMemoryManager *manager;

    if (ptr == NULL) {
        return;
    }

    manager = this;

    // Loop until the correct manager is located. Should be on the second pass
    // at latest.
    while (true) {
        Header *header;
        usize *end_tag;
        u32 tag_value;
        u32 manager_idx;

        manager->ValidateAddress(ptr, __FUNCTION__);

        header = (Header *)((usize)ptr - m_headerSize);

        manager->ValidateBlockIsAllocated(header, __FUNCTION__);
        manager->ValidateBlockFlags(header, flags, __FUNCTION__);
        manager->ValidateBlockEndTags(header, __FUNCTION__);

        end_tag = END_TAG(header, BLOCK_SIZE(header->value));

        tag_value = *end_tag >> 0x1b;
        if (tag_value != 0x1f) {
            manager_idx = tag_value - 1;
        } else {
            manager_idx = end_tag[-1];
        }

        if (manager->idx == manager_idx) {
            u32 header_value;
            Header *right;
            u32 left_end;
            FreeHeader *final;

            manager->ValidateBlockIsPaged(ptr, __FUNCTION__);

            pthread_mutex_lock(&manager->mutex);

            manager->stats.used_block_count--;

            header_value = header->value & BLOCK_SIZE_MASK;

            header->value = header_value;
            *END_TAG(header, header_value * 4) = header_value;

            if ((m_flags & MEM_MANAGER_DEBUG) != 0) {
                manager->stats.bytes_alloc_by_category[((DebugHeader *)header)->category] -= BLOCK_SIZE(header->value);
            }

            right = (Header *)((usize)header + BLOCK_SIZE(header->value));
            if ((right->value & ALLOC_MASK) == 0) {
                manager->ValidateBlockEndTags(right, "BlockFree[R]");
                manager->BinUnlink((FreeHeader *)right);
                header->value = (BLOCK_SIZE(header->value) + BLOCK_SIZE(right->value)) / 4;
                *END_TAG(header, BLOCK_SIZE(header->value)) = header->value;
            }

            final = (FreeHeader *)header;

            left_end = *(u32 *)((usize)header - 4);
            if ((left_end & HEADER_MGR_HI_MASK) == 0) {
                final = (FreeHeader *)((usize)header - BLOCK_SIZE(left_end));
                manager->ValidateBlockEndTags(&final->block_header, "BlockFree[L]");
                manager->BinUnlink(final);
                final->block_header.value = (BLOCK_SIZE(final->block_header.value) + BLOCK_SIZE(header->value)) / 4;
                *END_TAG(final, BLOCK_SIZE(final->block_header.value)) = final->block_header.value;
            }

            manager->BinLink(final, false);

            if (manager->stranded_blocks != NULL) {
                for (i32 i = 0; i < manager->stranded_block_count; i++) {
                    if (ptr == manager->stranded_blocks[i]) {
                        manager->stranded_blocks[i] = NULL;
                    }
                }
            }

            pthread_mutex_unlock(&manager->mutex);
            return;
        } else {
            manager = m_memoryManagers[manager_idx];
        }
    }
}

void *NuMemoryManager::_BlockReAlloc(void *ptr, u32 size, u32 alignment, u32 flags, const char *name, u16 category) {
    if (ptr == NULL) {
        return _BlockAlloc(size, alignment, flags, name, category);
    }
    if (size == 0) {
        BlockFree(ptr, flags);
        return NULL;
    }

    const u32 old_size = GetBlockSize(ptr);
    if (size <= old_size) {
        return ptr;
    }

    void *replacement = _TryBlockAlloc(size, alignment, flags, name, category);
    if (replacement != NULL && (flags & 0x40) == 0) {
        memcpy(replacement, ptr, old_size);
        BlockFree(ptr, flags);
    }
    return replacement;
}

inline void NuMemoryManager::MergeBlocks(Header *left, Header *right, const char *caller) {
    u32 combined_values;
    u32 alloc_value;
    u32 new_value;
    u32 combined_no_hi_bit;
    usize *end_tag;

    ValidateBlockEndTags(right, caller);

    BinUnlink((FreeHeader *)right);

    combined_values = (left->value & BLOCK_SIZE_MASK) + (right->value & BLOCK_SIZE_MASK);
    alloc_value = left->value & ALLOC_MASK;
    new_value = combined_values & 0x3fffffff | alloc_value;
    combined_no_hi_bit = combined_values & BLOCK_SIZE_MASK;

    left->value = new_value;

    u32 manager_idx = this->idx;

    end_tag = END_TAG(left, combined_no_hi_bit * 4);
    if ((combined_values & ALLOC_MASK) == 0 && alloc_value == 0) {
        *end_tag = combined_no_hi_bit;
    } else if (manager_idx < 0x1e) {
        *end_tag = ((manager_idx + 1) << 0x1b) | combined_no_hi_bit;
    } else {
        *end_tag = new_value | HEADER_MGR_HI_MASK;
        *END_TAG_HI(left, BLOCK_SIZE(left->value)) = manager_idx;
    }
}

void NuMemoryManager::AddPage(void *ptr, u32 size, bool _unknown) {
    Page *page = (Page *)ALIGN((usize)ptr, 4);
    const usize allocation_end = ((usize)ptr + size) & ~(usize)3;
    memset(page, 0, sizeof(Page));

    page->original_ptr = ptr;
    page->size = size;
    page->first_header = (Header *)((usize)page + 0x20);
    page->end = (u32 *)(allocation_end - 4);
    page->is_external = _unknown;

    // Allocated zero-sized fence blocks prevent coalescing beyond the page.
    Header *left_fence = (Header *)((usize)page + sizeof(Page));
    left_fence->value = 0x08000000;
    Header *right_fence = (Header *)(allocation_end - 4);
    right_fence->value = 0x08000000;

    FreeHeader *free_block = (FreeHeader *)page->first_header;
    const u32 free_size = (usize)right_fence - (usize)free_block;
    free_block->block_header.value = free_size / 4;
    free_block->next = NULL;
    free_block->prev = NULL;
    *END_TAG(free_block, free_size) = free_block->block_header.value;

    this->stats.unknown_00 += free_size;

    pthread_mutex_lock(&this->mutex);
    BinLink(free_block, true);

    page->next = this->pages;
    page->prev = NULL;
    if (page->next != NULL) {
        page->next->prev = page;
    }
    this->pages = page;
    pthread_mutex_unlock(&this->mutex);
}

void NuMemoryManager::SetBlockDebugCategory(void *ptr, u16 category) {
    DebugHeader *header;

    if (ptr != NULL && (m_flags & MEM_MANAGER_DEBUG) != 0) {
        ValidateAddress(ptr, __FUNCTION__);

        header = (DebugHeader *)((usize)ptr - m_headerSize);

        ValidateBlockIsAllocated(&header->block_header, __FUNCTION__);
        ValidateBlockEndTags(&header->block_header, __FUNCTION__);

        header->category = category;
    }
}

void NuMemoryManager::ReleaseUnreferencedPages() {
    Page *page;

    pthread_mutex_lock(&this->mutex);

    for (page = this->pages; page != NULL; page = page->next) {
    }

    pthread_mutex_unlock(&this->mutex);
}

u32 NuMemoryManager::CalculateBlockSize(u32 size) {
    u32 aligned;
    i32 block_size;
    u32 mem_size;

    aligned = ALIGN(size, 0x4);
    mem_size = MAX(aligned, 8);

    block_size = mem_size + m_headerSize + 4;
    if (this->idx > 29) {
        block_size = mem_size + m_headerSize + 8;
    }

    return block_size;
}

u32 NuMemoryManager::GetLargeBinIndex(u32 size) {
    // This could be negative if the size passed in were small enough, but in
    // practice this function is only called for allocations of sufficient size.
    return 0x15 - CountLeadingZeros(size);
}

u32 NuMemoryManager::GetSmallBinIndex(u32 size) {
    return size >> 2;
}

void NuMemoryManager::BinLink(NuMemoryManager::FreeHeader *header, bool keep_sorted) {
    u32 size;
    u32 bin_idx;

    size = BLOCK_SIZE(header->block_header.value);

    if (size < 0x400) {
        bin_idx = GetSmallBinIndex(size);
        BinLinkAfterNode(&this->small_bins[bin_idx], header);
        this->small_bin_has_free_map[bin_idx >> 5] |= bin_idx & 0x1f;
    } else {
        bin_idx = GetLargeBinIndex(size);

        if (keep_sorted) {
            FreeHeader *after;
            FreeHeader *next;

            next = &this->large_bins[bin_idx];

            do {
                after = next;
                next = next->next;
            } while (next != NULL && BLOCK_SIZE(next->block_header.value) <= size);

            BinLinkAfterNode(after, header);
        } else {
            BinLinkAfterNode(&this->large_bins[bin_idx], header);
            this->large_bin_dirty_map |= 1 << bin_idx;
        }

        this->large_bin_has_free_map |= 1 << (0x1f - bin_idx);
    }

    StatsAddFragment(header);
}

void NuMemoryManager::BinUnlink(NuMemoryManager::FreeHeader *header) {
    FreeHeader *next;
    u32 size;
    u32 bin_idx;

    // Remove the header from the linked list.
    next = header->next;
    if (next != NULL) {
        next->prev = header->prev;
    }

    if (header->prev != NULL) {
        header->prev->next = next;
    }

    header->next = NULL;
    header->prev = NULL;

    // Update maps of bins with free blocks.
    size = BLOCK_SIZE(header->block_header.value);

    if (size < 0x400) {
        bin_idx = GetSmallBinIndex(size);

        if (this->small_bins[bin_idx].next == NULL) {
            this->small_bin_has_free_map[bin_idx >> 5] &= ~(bin_idx & 0x1f);
        }
    } else {
        bin_idx = GetLargeBinIndex(size);

        if (this->large_bins[bin_idx].next == NULL) {
            // Rotate left.
            u32 shift = 0x1f - bin_idx;
            this->large_bin_has_free_map &= ((u32)-2 << shift) | ((u32)-2 >> (0x20 - shift));
        }
    }

    StatsRemoveFragment(header);
}

void NuMemoryManager::BinLinkAfterNode(NuMemoryManager::FreeHeader *after, NuMemoryManager::FreeHeader *header) {
    FreeHeader *next;

    next = after->next;
    header->prev = after;
    header->next = next;

    if (after->next != NULL) {
        after->next->prev = header;
    }

    after->next = header;
}

bool NuMemoryManager::PopContext(NuMemoryManager::PopDebugMode debug_mode) {
    char stranded_dump_path[128];
    char ctx_name[128];
    char largest_leak[128];
    char leak_value[257];
    u32 stranded_block_count;
    u32 stranded_bytes_count;
    u32 leak_count;
    u32 _unknown;
    Header *largest_stranded;
    Context *top_ctx;

    Validate();

    pthread_mutex_lock(&this->mutex);

    stranded_block_count = 0;

    memset(stranded_dump_path, 0, sizeof(stranded_dump_path));
    memset(ctx_name, 0, sizeof(ctx_name));
    memset(leak_value, 0, sizeof(leak_value));
    memset(largest_leak, 0, sizeof(largest_leak));

    stranded_bytes_count = 0;

    if ((m_flags & MEM_MANAGER_DEBUG) == 0) {
        leak_count = 0;

        if (this->stats.used_block_count > this->cur_ctx->used_block_count) {
            leak_count = this->stats.used_block_count - this->cur_ctx->used_block_count;
            strcpy(ctx_name, this->cur_ctx->name);
        }
    } else {
        _unknown = 0;
        largest_stranded = NULL;

        StrandBlocksForContext(this->cur_ctx, stranded_block_count, _unknown, largest_stranded, stranded_bytes_count);

        this->stats.unknown_18 = _unknown;

        if (stranded_block_count != 0 && debug_mode != POP_DEBUG_MODE_NONE) {
            usize len;
            DebugHeader *largest_stranded_dbg;
            const char *block_name;

            strcpy(stranded_dump_path, this->name);

            len = strlen(stranded_dump_path);

            stranded_dump_path[len] = '_';
            stranded_dump_path[len + 1] = '\0';

            strcpy(stranded_dump_path + len + 1, this->cur_ctx->name);

            len = strlen(stranded_dump_path);

            strncpy(stranded_dump_path + len, STRANDED_DUMP_SUFFIX, strlen(STRANDED_DUMP_SUFFIX) + 1);

            if (len != -strlen(STRANDED_DUMP_SUFFIX)) {
                char *cursor = stranded_dump_path;

                do {
                    if (*cursor == '/' || *cursor == '\\' || *cursor == ':' || *cursor == ' ') {
                        *cursor = '_';
                    }

                    cursor++;
                } while (cursor != stranded_dump_path + len + strlen(STRANDED_DUMP_SUFFIX));
            }

            Dump(0x38, stranded_dump_path);

            strcpy(ctx_name, this->cur_ctx->name);

            largest_stranded_dbg = (DebugHeader *)largest_stranded;

            block_name = largest_stranded_dbg->name;
            if (block_name != NULL) {
                block_name = NuStrStripPath(block_name);
            } else {
                block_name = "UNKNOWN";
            }

            strcpy(largest_leak, block_name);

            if ((largest_stranded_dbg->flags.alloc_flags & MEM_ALLOC_UNKNOWN_4) != 0) {
                u32 value_len;
                i32 i;

                value_len = MIN(BLOCK_SIZE(largest_stranded->value) - m_headerSize - 4, 0x100);

                for (i = 0; i != value_len; i++) {
                    char byte;

                    if ((unsigned char)largest_leak[i] - 0x30 < 10 || (unsigned char)largest_leak[i] - 0x41 < 0x1a) {
                        byte = *(char *)((usize)largest_stranded + m_headerSize + i);
                    } else {
                        byte = '_';
                    }

                    leak_value[i] = byte;
                }

                leak_value[i] = '\0';
            }

            leak_count = 0;
        }
    }

    ReleaseUnreferencedPages();

    top_ctx = this->cur_ctx;
    if (top_ctx != &this->initial_state_ctx) {
        this->cur_ctx = top_ctx->next;

        BlockFree(top_ctx, 0);
    }

    pthread_mutex_unlock(&this->mutex);

    if (debug_mode != POP_DEBUG_MODE_NONE) {
        if (stranded_block_count != 0) {
            pthread_mutex_lock(&this->error_mutex);

            if (leak_value[0] != '\0') {
                snprintf(this->error_msg, sizeof(this->error_msg),
                         "%u stranded memory blocks(s) [%u bytes] detected popping context %s\nReport dumped "
                         "[%s]\nLargest leak [%s]\nValue [%s]\n",
                         stranded_block_count, stranded_bytes_count, ctx_name, stranded_dump_path, largest_leak,
                         leak_value);
            } else {
                snprintf(this->error_msg, sizeof(this->error_msg),
                         "%u stranded memory blocks(s) [%u bytes] detected popping context %s\nReport dumped "
                         "[%s]\nLargest leak [%s]\n",
                         stranded_block_count, stranded_bytes_count, ctx_name, stranded_dump_path, largest_leak);
            }

            this->error_handler->HandleError(this, MEM_ERROR_LEAK_DETECTED, this->error_msg);

            pthread_mutex_unlock(&this->error_mutex);
        }

        if (debug_mode == POP_DEBUG_MODE_FREE_STRANDED_BLOCKS) {
            FreeStrandedBlocks();
        }
    }

    return stranded_block_count == 0;
}

void NuMemoryManager::Validate() {
}

void NuMemoryManager::ValidateAddress(void *ptr, const char *caller) {
    // Addresses should always be aligned to at minimum 4 bytes.
    if (((usize)ptr & 3) != 0) {
        char address[19];

        NuStrFormatAddress(address, sizeof(address), ptr);

        pthread_mutex_lock(&this->error_mutex);

        m_flags |= MEM_MANAGER_IN_ERROR_STATE;

        snprintf(this->error_msg, sizeof(this->error_msg), "Bad pointer detected in %s\nAddress: %s\n", caller,
                 address);

        this->error_handler->HandleError(this, MEM_ERROR_BAD_POINTER, this->error_msg);

        pthread_mutex_unlock(&this->error_mutex);
    }
}

void NuMemoryManager::ValidateAllocAlignment(u32 alignment) {
}

void NuMemoryManager::ValidateAllocSize(u32 size) {
}

void NuMemoryManager::ValidateBlockEndTags(Header *header, const char *caller) {
    u32 block_size;
    u32 end_tag_size;
    u32 header_size;
    u8 *data;
    char addr[19];

    header_size = m_headerSize;
    block_size = BLOCK_SIZE(header->value);
    end_tag_size = BLOCK_SIZE(*END_TAG(header, block_size));
    if (block_size != end_tag_size) {
        data = (u8 *)header + header_size;

        NuStrFormatAddress(addr, sizeof(addr), data);

        pthread_mutex_lock(&this->error_mutex);

        m_flags |= MEM_MANAGER_IN_ERROR_STATE;

        snprintf(this->error_msg, sizeof(this->error_msg),
                 "Memory corruption detected in %s (end tag mismatch). Possible corruption.\nAllocation: %s, Size: "
                 "%u\nBlock Size: %u, Footer Size: 0x%08X\n[%02X %02X %02X %02X %02X %02X %02X %02X ...]\n",
                 caller, addr, block_size - header_size - 4, block_size, end_tag_size, data[0], data[1], data[2],
                 data[3], data[4], data[5], data[6], data[7]);

        this->error_handler->HandleError(this, MEM_ERROR_CORRUPTION, this->error_msg);

        pthread_mutex_unlock(&this->error_mutex);
    }
}

void NuMemoryManager::ValidateBlockFlags(Header *header, u32 flags, const char *caller) {
}

void NuMemoryManager::ValidateBlockIsAllocated(Header *header, const char *caller) {
    u32 block_size;
    u32 alloc_size;
    u8 *data;
    char addr[19];

    if ((header->value & ALLOC_MASK) == 0) {
        block_size = BLOCK_SIZE(header->value);
        alloc_size = block_size - m_headerSize - 4;
        data = (u8 *)header + m_headerSize;

        NuStrFormatAddress(addr, sizeof(addr), data);

        pthread_mutex_lock(&this->error_mutex);

        m_flags |= MEM_MANAGER_IN_ERROR_STATE;

        snprintf(this->error_msg, sizeof(this->error_msg),
                 "Expected allocated block in %s. Might be free already, or is corrupt.\nAllocation: %s, Size: "
                 "%u\nBlock Size: %u\n[%02X %02X %02X %02X %02X %02X %02X %02X ...]\n",
                 caller, addr, alloc_size, block_size, data[0], data[1], data[2], data[3], data[4], data[5], data[6],
                 data[7]);

        this->error_handler->HandleError(this, MEM_ERROR_UNALLOCATED_BLOCK, this->error_msg);

        pthread_mutex_unlock(&this->error_mutex);
    }
}

void NuMemoryManager::ValidateBlockIsPaged(void *block, const char *caller) {
}

void NuMemoryManager::StatsAddFragment(NuMemoryManager::FreeHeader *header) {
    this->stats.free_frag_bytes += BLOCK_SIZE(header->block_header.value);
    this->stats.frag_count++;
    this->stats.max_frag_count = MAX(this->stats.max_frag_count, this->stats.frag_count);
}

void NuMemoryManager::StatsRemoveFragment(NuMemoryManager::FreeHeader *header) {
    this->stats.free_frag_bytes -= BLOCK_SIZE(header->block_header.value);
    this->stats.frag_count--;
}

u32 NuMemoryManager::CalculateLargestFragmentSize() {
    u32 largest_frag_unadjusted;
    u32 largest_frag_size;
    FreeHeader *largest_frag;

    if ((m_flags & MEM_MANAGER_IN_ERROR_STATE) != 0) {
        return 0;
    }

    pthread_mutex_lock(&this->mutex);

    largest_frag = FindLargestFragment();
    if (largest_frag == NULL) {
        pthread_mutex_unlock(&this->mutex);

        return 0;
    }

    largest_frag_unadjusted = BLOCK_SIZE(largest_frag->block_header.value);

    pthread_mutex_unlock(&this->mutex);

    largest_frag_unadjusted -= m_headerSize;

    largest_frag_size = this->idx < 0x1e ? largest_frag_unadjusted - 4 : largest_frag_unadjusted - 8;

    return largest_frag_size;
}

u32 NuMemoryManager::CalculateFreeBytes() {
    u32 free_bytes = 0;

    if ((m_flags & MEM_MANAGER_IN_ERROR_STATE) != 0) {
        return 0;
    }

    pthread_mutex_lock(&this->mutex);

    for (i32 i = 0x15; i >= 0; i--) {
        for (FreeHeader *header = &this->large_bins[i]; header != NULL; header = header->next) {
            free_bytes += BLOCK_SIZE(header->block_header.value);
        }
    }

    for (i32 i = 0xff; i >= 0; i--) {
        for (FreeHeader *header = &this->small_bins[i]; header != NULL; header = header->next) {
            free_bytes += BLOCK_SIZE(header->block_header.value);
        }
    }

    pthread_mutex_unlock(&this->mutex);

    return free_bytes;
}

NuMemoryManager::FreeHeader *NuMemoryManager::FindLargestFragment() {
    FreeHeader *frag;

    if ((m_flags & MEM_MANAGER_IN_ERROR_STATE) != 0) {
        return NULL;
    }

    pthread_mutex_lock(&this->mutex);

    for (i32 i = 0x15; i >= 0; i--) {
        FreeHeader *header = &this->large_bins[i];

        do {
            frag = header;
            header = header->next;
        } while (header != NULL);

        if (BLOCK_SIZE(frag->block_header.value) != 0) {
            goto done;
        }
    }

    for (i32 i = 0xff; i >= 0; i--) {
        frag = this->small_bins[i].next;

        if (frag != NULL) {
            break;
        }
    }

done:
    pthread_mutex_unlock(&this->mutex);

    return frag;
}

void NuMemoryManager::Dump(u32 _unknown, const char *filepath) {
}

void NuMemoryManager::StrandBlocksForContext(Context *ctx, u32 &stranded_block_count, u32 &_unknown,
                                             Header *&largest_stranded, u32 &stranded_bytes_count) {
}

void NuMemoryManager::FreeStrandedBlocks() {
}

void NuMemoryManager::IErrorHandler::HandleError(NuMemoryManager *manager, ErrorCode code, const char *msg) {
}

i32 NuMemoryManager::IErrorHandler::OpenDump(NuMemoryManager *manager, const char *filename, u32 &id) {
    return 0;
}

void NuMemoryManager::IErrorHandler::CloseDump(NuMemoryManager *manager, u32 id) {
}

void NuMemoryManager::IErrorHandler::Dump(NuMemoryManager *manager, u32 id, const char *msg) {
}

void NuMemoryManager::ClearBlockDebugContext(void *) {
}

void NuMemoryManager::DumpBlock(u32, NuSymbolQuery *, NuMemoryManager::Header *, u32, u32, u32) {
}

void NuMemoryManager::DumpBlocksForContext(u32, NuSymbolQuery *, NuMemoryManager::Context *, u32) {
}

void NuMemoryManager::FindAndTouchMatchingBlocks(NuMemoryManager::DebugHeader *, u32 *, u32) {
}

u32 NuMemoryManager::GetAllocatedBytes() {
    return GetPagedBytes() - GetFreeBytes();
}

u32 NuMemoryManager::GetBlockAlignment(void *ptr) {
    ValidateAddress(ptr, __FUNCTION__);

    Header *header = reinterpret_cast<Header *>(reinterpret_cast<usize>(ptr) - m_headerSize);
    ValidateBlockIsAllocated(header, __FUNCTION__);
    ValidateBlockEndTags(header, __FUNCTION__);

    return 2u << ((header->value & 0x78000000) >> 27);
}

void NuMemoryManager::GetBlockDebugBackTrace(void *, void **) {
}

void NuMemoryManager::GetBlockDebugContext(void *) {
}

SAGA_HOST_WEAK u32 NuMemoryManager::GetBlockSize(void *ptr) {
    ValidateAddress(ptr, __FUNCTION__);

    Header *header = (Header *)((usize)ptr - m_headerSize);
    ValidateBlockIsAllocated(header, __FUNCTION__);
    ValidateBlockEndTags(header, __FUNCTION__);

    u32 total_size = (header->value & 0x87ffffff) * 4;
    u32 payload_size = total_size - m_headerSize;
    u32 *end_tag = (u32 *)((usize)header + total_size - 4);
    u32 manager_index = *end_tag >> 27;
    if (manager_index == 0x1f) {
        manager_index = *(end_tag - 1);
    } else {
        manager_index--;
    }

    return payload_size - (manager_index > 0x1d ? 8 : 4);
}

u32 NuMemoryManager::GetCategoryAllocatedBytes(u16 category) {
    return stats.bytes_alloc_by_category[category];
}

u32 NuMemoryManager::GetCurrentContextID() const {
    return cur_ctx != NULL ? cur_ctx->id : 0;
}

const char *NuMemoryManager::GetCurrentContextName() const {
    return cur_ctx != NULL ? cur_ctx->name : NULL;
}

const char *NuMemoryManager::GetDebugName() const {
    return name != NULL ? name : "null";
}

u32 NuMemoryManager::GetFreeBytes() const {
    u32 fragment_overhead = m_headerSize + 4;
    if (idx > 0x1d) {
        fragment_overhead += 4;
    }
    return stats.free_frag_bytes - fragment_overhead * stats.frag_count;
}

u32 NuMemoryManager::GetNumFreeFragments() const {
    return stats.frag_count;
}

u16 NuMemoryManager::GetOverrideCategory() {
    return override_category;
}

u16 NuMemoryManager::GetOverrideCategoryBGThread() {
    return override_category_bg_thread;
}

u32 NuMemoryManager::GetPagedBytes() {
    u32 paged_bytes = 0;

    if ((m_flags & MEM_MANAGER_IN_ERROR_STATE) != 0) {
        return paged_bytes;
    }

    pthread_mutex_lock(&mutex);
    for (Page *page = pages; page != NULL; page = page->next) {
        paged_bytes += page->size;
    }
    pthread_mutex_unlock(&mutex);

    return paged_bytes;
}

u32 NuMemoryManager::GetSmallBinSize(u32 index) {
    return index * 4;
}

bool NuMemoryManager::IsZombie() {
    return is_zombie;
}

void NuMemoryManager::MergeLargeBinSegments(NuMemoryManager::FreeHeader *, NuMemoryManager::FreeHeader *) {
}

void NuMemoryManager::PushContext(char const *) {
}

void NuMemoryManager::ReleaseExternalPage(void *) {
}

void NuMemoryManager::SetBlockDebugContext(void *, u32) {
}

void NuMemoryManager::SetBlockDebugName(void *, char const *) {
}

void NuMemoryManager::SetOverrideCategory(u16 category) {
    override_category = category;
}

void NuMemoryManager::SetOverrideCategoryBGThread(u16 category) {
    override_category_bg_thread = category;
}

void NuMemoryManager::SortLargeBin(u32) {
}

void NuMemoryManager::SortLargeBinSegment(NuMemoryManager::FreeHeader *, u32) {
}

void NuMemoryManager::UnTouchAllBlocks() {
}

void NuMemoryManager::ValidateBlock(void *) {
}

void NuMemoryManager::ValidateBlockDeferredContent(NuMemoryManager::Header *, char const *) {
}

void NuMemoryManager::VisitManagers(NuMemoryManager::IVisitor *visitor) {
    pthread_mutex_lock(m_globalCriticalSection);
    for (NuMemoryManager *manager : m_memoryManagers) {
        if (manager != NULL) {
            visitor->Visit(manager);
        }
    }
    pthread_mutex_unlock(m_globalCriticalSection);
}

void NuMemoryManager::VisitPages(NuMemoryManager::IPageVisitor *visitor) {
    pthread_mutex_lock(&mutex);
    for (Page *page = pages; page != NULL; page = page->next) {
        visitor->Visit(this, page->original_ptr, page->size);
    }
    pthread_mutex_unlock(&mutex);
}

void NuMemoryManager::_MultiBlockAlloc(u32, u32, u32, void **, u32, char const *, u16) {
}
