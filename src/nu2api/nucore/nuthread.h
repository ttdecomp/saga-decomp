#pragma once

#include "nu2api/nucore/common.h"

#include <pthread.h>

typedef enum NUTHREADCAFECORE {
    NUTHREADCAFECORE_UNKNOWN_1 = 1,
    NUTHREADCAFECORE_UNKNOWN_2 = 2,
} NUTHREADCAFECORE;

typedef enum NUTHREADXBOX360CORE {
    NUTHREADXBOX360CORE_UNKNOWN_1 = 1,
    NUTHREADXBOX360CORE_UNKNOWN_2 = 2,
} NUTHREADXBOX360CORE;

typedef union nuthread_core_u {
    NUTHREADCAFECORE cafe_core;
    NUTHREADXBOX360CORE xbox360_core;
    i32 value;
} NUTHREAD_CORE;

struct NULEGACYTHREADDATA {
    pthread_t thread;
    void (*thread_fn)(void *);
    void *fn_arg;
};

typedef void nuthreadenableswapfn();
typedef void nuthreaddisableswapfn();

#ifdef __cplusplus
extern "C" {
#endif
    extern u32 nu_current_thread_id;
#ifdef ANDROID
    extern pthread_key_t g_currentThreadSpecificKey;
    i32 NuGetCurrentThreadId(void);
    i32 NuThreadCreate(void (*function)(void *), void *argument);
#endif
    extern nuthreadenableswapfn *NuThreadEnableThreadSwap;
    extern nuthreaddisableswapfn *NuThreadDisableThreadSwap;

    i32 NuThreadCreateCriticalSection();
    void NuThreadDestroyCriticalSection(i32 index);

    i32 NuThreadCriticalSectionBegin(i32 index);
    i32 NuThreadCriticalSectionEnd(i32 index);

    void NuEnableVBlank();
    void NuDisableVBlank();
    void NuThreadSignalSend(void);
    void NuThreadSignalRecieve(void);
#ifdef __cplusplus
}

struct NuThreadCreateParameters {
    void (*thread_fn)(void *);
    void *fn_arg;
    i32 priority;
    const char *name;
    i32 stack_size;
    bool is_suspended;
    NUTHREADCAFECORE cafe_core;
    NUTHREADXBOX360CORE xbox360_core;
    bool use_current;
};

class NuThreadBase {
  protected:
    void (*thread_fn)(void *);
    void *fn_arg;
    char name[32];
    void *local_storage[32];

  public:
    NuThreadBase(const NuThreadCreateParameters &params);

    void *GetLocalStorage(u32 idx) const;
    void SetLocalStorage(u32 idx, void *storage);

    void SetDebugName(const char *name);
    const char *GetDebugName() const;

    void (*GetThreadFn() const)(void *);
    void *GetParam() const;

  protected:
    ~NuThreadBase();
};

class NuThread : public NuThreadBase {
  private:
    bool is_suspended;

    static void *ThreadMain(void *self);

  public:
    NuThread(const NuThreadCreateParameters &params);

    void Resume();
    void SetDebugName(const char *name);

  protected:
    ~NuThread();
};

class NuThreadManager {
  public:
    NuThreadManager();

  private:
    NuThread *thread;
    u32 bitflags;

  public:
    i32 AllocTLS();
    NuThreadBase *GetCurrentThread();
    NuThread *GetMainThread();
    void FreeTLS(i32 idx);

    NuThread *CreateThread(void (*thread_fn)(void *), void *fn_arg, i32 priority, const char *name, i32 stack_size,
                           NUTHREADCAFECORE cafe_core, NUTHREADXBOX360CORE xbox360_core);
    NuThread *CreateThreadSuspended(void (*thread_fn)(void *), void *fn_arg, i32 priority, const char *name,
                                    i32 stack_size, NUTHREADCAFECORE cafe_core, NUTHREADXBOX360CORE xbox360_core);
};

class NuCriticalSection {
  public:
    pthread_mutex_t mutex;

    NuCriticalSection(const char *name) {
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&this->mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    ~NuCriticalSection() {
        pthread_mutex_destroy(&this->mutex);
    }

    void Lock() {
        pthread_mutex_lock(&this->mutex);
    }

    void Unlock() {
        pthread_mutex_unlock(&this->mutex);
    }
};

NuThreadBase *NuThreadGetCurrentThread();
void NuThreadSleep(i32 seconds);
NuThread *NuThreadInitPS();

#endif

#ifdef ANDROID

#ifdef __cplusplus
extern "C" {
#endif
    void NuIOSThreadInit(void);
#ifdef __cplusplus
}
#endif

#endif
