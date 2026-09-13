#include "nu2api/nucore/nuthread.h"

#include <pthread.h>

#include "nu2api/nucore/common.h"

#ifdef ANDROID
pthread_key_t g_currentThreadSpecificKey;
#endif

// The original nuthread.c constructs this TU's six homogeneous vectors.
#include "nu2api/nucore/nuvuvec.hpp"

static char NuThread_CriticalSectionsUsed[16];
static pthread_mutex_t NuThread_CriticalSections[16];

#ifdef ANDROID
static char NuThread_ThreadsUsed[16];
static NULEGACYTHREADDATA NuThread_Threads[16];

static void ThreadMain(u64 thread_data_value) {
    NULEGACYTHREADDATA *thread_data = reinterpret_cast<NULEGACYTHREADDATA *>(static_cast<usize>(thread_data_value));
    pthread_setspecific(g_currentThreadSpecificKey, thread_data);
    thread_data->thread_fn(thread_data->fn_arg);
    pthread_exit(NULL);
}

i32 NuGetCurrentThreadId(void) {
    return (NULEGACYTHREADDATA *)pthread_getspecific(g_currentThreadSpecificKey) - NuThread_Threads;
}

void DataDestructor(void *data) {
}

void NuIOSThreadInit() {
    void *data;

    pthread_key_create(&g_currentThreadSpecificKey, DataDestructor);

    data = NuThread_Threads;
    pthread_setspecific(g_currentThreadSpecificKey, data);
}
#endif

i32 NuThreadCreateCriticalSection(void) {
    i32 index = -1;
    for (i32 i = 0; i < 16; i++) {
        if (NuThread_CriticalSectionsUsed[i] == 0) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return -1;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, 1);
    pthread_mutex_init(&NuThread_CriticalSections[index], &attr);
    pthread_mutexattr_destroy(&attr);
    NuThread_CriticalSectionsUsed[index] = true;
    return index;
}

void NuThreadDestroyCriticalSection(i32 index) {
    pthread_mutex_destroy(&NuThread_CriticalSections[index]);
    NuThread_CriticalSectionsUsed[index] = false;
}

i32 NuThreadCriticalSectionBegin(i32 index) {
    return pthread_mutex_lock(&NuThread_CriticalSections[index]);
}

i32 NuThreadCriticalSectionEnd(i32 index) {
    return pthread_mutex_unlock(&NuThread_CriticalSections[index]);
}

#ifdef ANDROID
i32 NuThreadCreate(void (*function)(void *), void *argument) {
    i32 i;
    i32 index = -1;
    for (i = 0; i < 16; i++) {
        if (NuThread_ThreadsUsed[i] == 0) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return -1;

    NuThread_Threads[index].thread_fn = function;
    NuThread_Threads[index].fn_arg = argument;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, 0x10000);
    i32 result = pthread_create(&NuThread_Threads[index].thread, &attributes,
                                reinterpret_cast<void *(*)(void *)>(ThreadMain), &NuThread_Threads[index]);
    pthread_attr_destroy(&attributes);
    NuThread_ThreadsUsed[index] = 1;
    return index;
}
#endif

void NuThreadSignalSend(void) {
}

void NuThreadSignalRecieve(void) {
}

void NuEnableVBlank() {
}

void NuDisableVBlank() {
}

nuthreaddisableswapfn *NuThreadDisableThreadSwap = &NuDisableVBlank;
nuthreadenableswapfn *NuThreadEnableThreadSwap = &NuEnableVBlank;
