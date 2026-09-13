#include "nu2api/nucore/nuthread.h"

NuThreadManager::NuThreadManager() {
    this->bitflags = 0;
    this->thread = NuThreadInitPS();
}

i32 NuThreadManager::AllocTLS() {
    for (i32 i = 0; i < 32; i++) {
        if ((this->bitflags >> (i & 0x1f) & 1) == 0) {
            this->bitflags |= 1 << ((u8)i & 0x1f);
            return i;
        }
    }

    return -1;
}

NuThread *NuThreadManager::CreateThread(void (*thread_fn)(void *), void *fn_arg, i32 priority, const char *name,
                                        i32 stack_size, NUTHREADCAFECORE cafe_core, NUTHREADXBOX360CORE xbox360_core) {
    static i32 ThreadPriorityMap[] = {0, 1, 2, 3, 4};

    NuThreadCreateParameters params;

    stack_size = stack_size == 0 ? 0x8000 : stack_size;

    params.thread_fn = thread_fn;
    params.fn_arg = fn_arg;

    params.priority = ThreadPriorityMap[priority + 2];

    params.name = name;

    params.stack_size = stack_size;

    params.is_suspended = false;

    params.cafe_core = cafe_core;
    params.xbox360_core = xbox360_core;

    params.use_current = false;

    return new NuThread(params);
}

NuThreadBase *NuThreadManager::GetCurrentThread() {
    return NuThreadGetCurrentThread();
}

NuThread *NuThreadManager::CreateThreadSuspended(void (*thread_fn)(void *), void *fn_arg, i32 priority,
                                                 const char *name, i32 stack_size, NUTHREADCAFECORE cafe_core,
                                                 NUTHREADXBOX360CORE xbox360_core) {
    static i32 ThreadPriorityMap[] = {0, 1, 2, 3, 4};

    NuThreadCreateParameters params;
    params.thread_fn = thread_fn;
    params.fn_arg = fn_arg;
    params.priority = ThreadPriorityMap[priority + 2];
    params.name = name;
    params.stack_size = stack_size;
    params.is_suspended = true;
    params.cafe_core = cafe_core;
    params.xbox360_core = xbox360_core;
    params.use_current = false;
    return new NuThread(params);
}

void NuThreadManager::FreeTLS(i32 index) {
    bitflags &= ~(1U << index);
}

NuThread *NuThreadManager::GetMainThread() {
    return thread;
}
