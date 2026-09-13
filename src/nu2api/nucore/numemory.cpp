#include "decomp.h"
#include <new>
#include <pthread.h>

#include "nu2api/nucore/numemory.h"

#include "nu2api/nucore/NuMemoryManager.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nuthread.h"

static NuMemory *g_memory = NULL;

const char *g_categoryNames[51] = {
    "NONE",          "INITIAL",      "HARDWAREINIT",  "NUFILE",      "NUFONT",         "NUSCENE",       "NURENDER",
    "NUSOUND",       "NUDEBUG",      "CHARS",         "CHARANIMS",   "SPLITSCREEN",    "DYNOTERRAIN",   "DYNODATA",
    "DYNOSYSTEM",    "AI",           "PARTICLE",      "VFX",         "TECH",           "SCRIPT",        "RESOURCE",
    "GAMEFRAMEWORK", "LEVEL",        "CUTSCENE",      "KRAWLIE",     "GAME",           "EDITOR",        "ANIMATION",
    "MANAGERS",      "ENGINE",       "G_LEGOSETS",    "L_LEGOSETS",  "TECH_GIN",       "TECH_GIZMO",    "TECH_GADGET",
    "TECH_FLOW",     "TECH_ANIM",    "TECH_LIGHTING", "TECH_AI",     "TECH_AI_EDMESH", "TECH_AI_GRAPH", "TECH_CAMERA",
    "TECH_AUDIO",    "TECH_PICKUPS", "TECH_KRAWLIE",  "TECH_STREAM", "TECH_GRASS",     "TECH_MAINLED",  "TECH_FIXUP",
    "RUNTIME",       "TTSHARED",
};

bool g_disallowGlobalNew = false;

NuMemory::NuMemory(void **buf) {
    void *ptr;

    this->tls_index = -1;

    NuMemoryManager::SetFlags(0);

    ptr = (void *)ALIGN((usize)*buf, 0x8);

    this->error_handler = new (ptr) MemErrorHandler();
    ptr = (void *)((usize)ptr + sizeof(MemErrorHandler));

    this->mem1_event_handler = new (ptr) NuMemoryPS::Mem1EventHandler();
    ptr = (void *)((usize)ptr + sizeof(NuMemoryPS::Mem1EventHandler));

    this->mem1_manager = (NuMemoryManager *)ptr;
    if (this->mem1_manager != NULL) {
        new (this->mem1_manager) NuMemoryManager(this->mem1_event_handler, this->error_handler, "MEM1", g_categoryNames,
                                                 sizeof(g_categoryNames) / sizeof(char *));
    }
    ptr = (void *)((usize)this->mem1_manager + sizeof(NuMemoryManager));

    this->mem2_event_handler = new (ptr) NuMemoryPS::Mem2EventHandler();
    ptr = (void *)((usize)ptr + sizeof(NuMemoryPS::Mem2EventHandler));

    this->mem2_manager = (NuMemoryManager *)ptr;
    if (this->mem2_manager != NULL) {
        new (this->mem2_manager) NuMemoryManager(this->mem2_event_handler, this->error_handler, "MEM2", g_categoryNames,
                                                 sizeof(g_categoryNames) / sizeof(char *));
    }
    ptr = (void *)((usize)this->mem2_manager + sizeof(NuMemoryManager));

    this->fixed_pool_event_handler = new (ptr) FixedPoolEventHandler();
    ptr = (void *)((usize)ptr + sizeof(FixedPoolEventHandler));

    this->dynamic_pool_event_handler = new (ptr) DynamicPoolEventHandler();
    ptr = (void *)((usize)ptr + sizeof(DynamicPoolEventHandler));

    this->unknown = 0;
}

NuMemoryManager *NuMemory::GetThreadMem() {
    this->InitalizeThreadLocalStorage();

    if (this->tls_index != -1) {
        NuThreadBase *thread = NuCore::m_threadManager->GetCurrentThread();

        if (thread != NULL) {
            NuMemoryManager *manager = (NuMemoryManager *)thread->GetLocalStorage(this->tls_index);

            if (manager != NULL) {
                return manager;
            }
        }
    }

    return this->mem1_manager;
}

NuMemoryManager *NuMemory::SetThreadMem(NuMemoryManager *manager) {
    NuThreadBase *thread;
    void *storage;

    InitalizeThreadLocalStorage();

    thread = NuCore::m_threadManager->GetCurrentThread();

    storage = thread->GetLocalStorage(this->tls_index);
    thread->SetLocalStorage(this->tls_index, manager);

    return (NuMemoryManager *)storage;
}

static char g_memoryBuffer[0x10000];

NuMemory *NuMemoryGet() {
    void *mem;
    char *aligned;

    if (g_memory != NULL) {
        return g_memory;
    }

    aligned = (char *)ALIGN((usize)g_memoryBuffer, 0x8);

    mem = aligned + sizeof(NuMemory);
    g_memory = new (aligned) NuMemory(&mem);
    return g_memory;
}

void NuMemory::InitalizeThreadLocalStorage() {
    if (this->tls_index == -1 && NuCore::m_threadManager != NULL) {
        this->tls_index = NuCore::m_threadManager->AllocTLS();
    }
}

i32 NuMemory::FixedPoolEventHandler::AllocatePage(NuMemoryPool *pool, u32 _unknown, u32 _unknown2,
                                                  const char *_unknown3) {
    return 0;
}

i32 NuMemory::FixedPoolEventHandler::ReleasePage(NuMemoryPool *pool, void *ptr) {
    NuMemoryGet()->GetThreadMem()->BlockFree(ptr, 0);

    return 1;
}

void NuMemory::FixedPoolEventHandler::ForceReleasePage(NuMemoryPool *pool, void *ptr) {
    NuMemoryGet()->GetThreadMem()->BlockFree(ptr, 0);
}

void *NuMemory::FixedPoolEventHandler::AllocateLargeBlock(NuMemoryPool *pool, u32 size, u32 alignment,
                                                          const char *name) {
    return NuMemoryGet()->GetThreadMem()->_BlockAlloc(size, alignment, 0, name, NUMEMORY_CATEGORY_NONE);
}

void NuMemory::FixedPoolEventHandler::FreeLargeBlock(NuMemoryPool *pool, void *ptr) {
    NuMemoryGet()->GetThreadMem()->BlockFree(ptr, 0);
}

i32 NuMemory::DynamicPoolEventHandler::AllocatePage(NuMemoryPool *pool, u32 _unknown, u32 alignment, const char *name) {
    void *page = NuMemoryGet()->GetThreadMem()->_BlockAlloc(0x4000, alignment, 0, name, NUMEMORY_CATEGORY_NONE);
    pool->AddPage(page, 0x4000);

    return 1;
}

i32 NuMemory::DynamicPoolEventHandler::ReleasePage(NuMemoryPool *pool, void *ptr) {
    NuMemoryGet()->GetThreadMem()->BlockFree(ptr, 0);

    return 1;
}

void NuMemory::DynamicPoolEventHandler::ForceReleasePage(NuMemoryPool *pool, void *ptr) {
    NuMemoryGet()->GetThreadMem()->BlockFree(ptr, 0);
}

void *NuMemory::DynamicPoolEventHandler::AllocateLargeBlock(NuMemoryPool *pool, u32 size, u32 alignment,
                                                            const char *name) {
    return NuMemoryGet()->GetThreadMem()->_BlockAlloc(size, alignment, 0, name, NUMEMORY_CATEGORY_NONE);
}

void NuMemory::DynamicPoolEventHandler::FreeLargeBlock(NuMemoryPool *pool, void *ptr) {
    NuMemoryGet()->GetThreadMem()->BlockFree(ptr, 0);
}

NuMemoryManager *NuMemory::CreateMemoryManager(NuMemoryManager::IEventHandler *event_handler, const char *name) {
    NuMemoryManager *manager = (NuMemoryManager *)GetThreadMem()->_BlockAlloc(
        sizeof(NuMemoryManager), 4, 0, "i:/SagaTouch-Android_9176564/nu2api.2013/numemory/numemory.cpp:501", 0);

    if (manager != NULL) {
        new (manager) NuMemoryManager(event_handler, this->error_handler, name, NULL, 0);
    }

    return manager;
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

void NuMemoryPool::InterlockedPop(NuMemoryPool::FreeBlock volatile **) {
}

void NuMemoryPool::InterlockedPush(NuMemoryPool::FreeBlock volatile **, void *) {
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

void NuMemoryManager::VisitManagers(NuMemoryManager::IVisitor *) {
}

void NuMemoryManager::VisitPages(NuMemoryManager::IPageVisitor *) {
}

void NuMemoryManager::_MultiBlockAlloc(u32, u32, u32, void **, u32, char const *, u16) {
}

void NuMemory::CreateDynamicMemoryPool(u32, char const *) {
}

void NuMemory::CreateFixedMemoryPool(u32, u32, char const *) {
}

void NuMemory::CreateMemoryPool(NuMemoryPool::IEventHandler *, u32, char const *) {
}

void NuMemory::DestroyMemoryManager(NuMemoryManager *) {
}

void NuMemory::DestroyMemoryPool(NuMemoryPool *) {
}

void NuMemory::MoveFreeMem2IntoMem1() {
}

void NuMemory::SetSoakTestMode() {
    in_soak_test_mode = true;
}

typedef struct NUHEAPBLOCK NUHEAPBLOCK;
