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

NuMemoryPool *NuMemory::CreateDynamicMemoryPool(u32 size, const char *name) {
    NuMemoryPool *pool = static_cast<NuMemoryPool *>(GetThreadMem()->_BlockAlloc(
        sizeof(NuMemoryPool), 4, 0, "i:/SagaTouch-Android_9176564/nu2api.2013/numemory/numemory.cpp:539", 0));
    new (pool) NuMemoryPool(dynamic_pool_event_handler, size, name);
    return pool;
}

NuMemoryPool *NuMemory::CreateFixedMemoryPool(u32 size, u32 block_size, const char *name) {
    NuMemoryPool *pool = static_cast<NuMemoryPool *>(GetThreadMem()->_BlockAlloc(
        sizeof(NuMemoryPool), 4, 0, "i:/SagaTouch-Android_9176564/nu2api.2013/numemory/numemory.cpp:517", 0));
    new (pool) NuMemoryPool(fixed_pool_event_handler, block_size, name);
    if (size != 0) {
        void *page = GetThreadMem()->_BlockAlloc(
            size, block_size, 0, "i:/SagaTouch-Android_9176564/nu2api.2013/numemory/numemory.cpp:521", 0);
        pool->AddPage(page, size);
    }
    return pool;
}

NuMemoryPool *NuMemory::CreateMemoryPool(NuMemoryPool::IEventHandler *handler, u32 size, const char *name) {
    NuMemoryPool *pool = static_cast<NuMemoryPool *>(GetThreadMem()->_BlockAlloc(
        sizeof(NuMemoryPool), 4, 0, "i:/SagaTouch-Android_9176564/nu2api.2013/numemory/numemory.cpp:531", 0));
    new (pool) NuMemoryPool(handler, size, name);
    return pool;
}

void NuMemory::DestroyMemoryManager(NuMemoryManager *manager) {
    manager->~NuMemoryManager();
    GetThreadMem()->BlockFree(manager, 0);
}

void NuMemory::DestroyMemoryPool(NuMemoryPool *pool) {
    pool->~NuMemoryPool();
    GetThreadMem()->BlockFree(pool, 0);
}

u32 NuMemory::MoveFreeMem2IntoMem1() {
    u32 size = mem2_manager->CalculateLargestFragmentSize();
    if (size != 0) {
        void *page = mem2_manager->_BlockAlloc(
            size, 4, 0, "i:/SagaTouch-Android_9176564/nu2api.2013/numemory/numemory.cpp:487", 0);
        mem1_manager->AddPage(page, size, false);
    }
    return size;
}

void NuMemory::SetSoakTestMode() {
    in_soak_test_mode = true;
}

typedef struct NUHEAPBLOCK NUHEAPBLOCK;
