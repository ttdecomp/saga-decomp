#include "nu2api/nucore/android/NuThread_android.h"

#include <pthread.h>
#include <string.h>

#include "java/java.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"

static thread_local NuThreadBase *g_currentThread;
static thread_local pthread_t g_currentPthread;

NuThreadBase *NuThreadGetCurrentThread() {
    return g_currentThread;
}

void *NuThreadBase::GetLocalStorage(u32 idx) const {
    return this->local_storage[idx];
}

void NuThreadBase::SetLocalStorage(u32 idx, void *storage) {
    this->local_storage[idx] = storage;
}

NuThreadBase::NuThreadBase(const NuThreadCreateParameters &params) {
    this->thread_fn = params.thread_fn;
    this->fn_arg = params.fn_arg;

    this->name[0] = '\0';

    memset(this->local_storage, 0, sizeof(this->local_storage));
}

NuThreadBase::~NuThreadBase() {
}

void NuThreadBase::SetDebugName(const char *name) {
    NuStrNCpy(this->name, name, 0x20);
}

void (*NuThreadBase::GetThreadFn() const)(void *) {
    return this->thread_fn;
}

void *NuThreadBase::GetParam() const {
    return this->fn_arg;
}

const char *NuThreadBase::GetDebugName() const {
    return this->name;
}

NuThread::NuThread(const NuThreadCreateParameters &params) : NuThreadBase(params) {
    sched_param scheduling;
    pthread_attr_t attrs;

    if (params.use_current) {
        g_currentPthread = pthread_self();

        return;
    }

    switch (params.priority) {
        case 0:
            scheduling.sched_priority = -2;
            break;
        case 1:
            scheduling.sched_priority = -1;
            break;
        case 2:
            scheduling.sched_priority = 0;
            break;
        case 3:
            scheduling.sched_priority = 1;
            break;
        case 4:
            scheduling.sched_priority = 2;
            break;
    }

    this->is_suspended = params.is_suspended;

    pthread_attr_init(&attrs);
    pthread_attr_setschedparam(&attrs, &scheduling);

    pthread_create(&g_currentPthread, &attrs, &ThreadMain, this);

    pthread_attr_destroy(&attrs);

    SetDebugName(params.name);
}

NuThread::~NuThread() {
}

void NuThread::SetDebugName(const char *name) {
    NuThreadBase::SetDebugName(name);
}

void *NuThread::ThreadMain(void *thread) {
    JNIEnv *jni_env;
    NuThread *self;

    jni_env = NULL;

    g_javaVM->GetEnv((void **)&jni_env, JNI_VERSION_1_4);
    g_javaVM->AttachCurrentThread(&jni_env, NULL);

    self = ((NuThread *)thread);
    g_currentThread = self;

    while (self->is_suspended) {
        NuThreadSleep(1);
    }

    void (*threadFn)(void *) = self->GetThreadFn();
    void *param = self->GetParam();

    (*threadFn)(param);

    delete self;

    g_javaVM->DetachCurrentThread();

    return NULL;
}

NuThreadSemaphore::NuThreadSemaphore(i32 max_signals) {
    this->signaled_count = 0;
    this->max_signals = max_signals;

    pthread_mutex_init(&this->mutex, NULL);
    pthread_cond_init(&this->condition, NULL);
}

NuThreadSemaphore::~NuThreadSemaphore() {
    pthread_mutex_destroy(&this->mutex);
    pthread_cond_destroy(&this->condition);
}

void NuThreadSemaphore::Signal() {
    pthread_mutex_lock(&this->mutex);

    if (this->signaled_count < this->max_signals) {
        this->signaled_count++;

        pthread_cond_signal(&this->condition);
    }

    pthread_mutex_unlock(&this->mutex);
}

i32 NuThreadSemaphore::TryWait() {
    i32 did_wait;

    pthread_mutex_lock(&this->mutex);

    did_wait = 0;
    if (this->signaled_count > 0) {
        this->signaled_count--;
        did_wait = 1;
    }

    pthread_mutex_unlock(&this->mutex);

    return did_wait;
}

void NuThreadSemaphore::Wait() {
    pthread_mutex_lock(&this->mutex);

    while (this->signaled_count == 0) {
        pthread_cond_wait(&this->condition, &this->mutex);
    }

    this->signaled_count--;

    pthread_mutex_unlock(&this->mutex);
}

void NuThread::Resume() {
    this->is_suspended = false;
}

NuThread *NuThreadInitPS() {
    pthread_t current;
    i32 policy;
    sched_param scheduling;
    NuThreadCreateParameters params;
    NuThread *thread;

    current = pthread_self();

    policy = 0;
    pthread_getschedparam(current, &policy, &scheduling);

    params.thread_fn = NULL;
    params.fn_arg = NULL;

    params.priority = 1;

    params.name = "";

    params.stack_size = 0;

    params.is_suspended = false;

    params.cafe_core = NUTHREADCAFECORE_UNKNOWN_1;
    params.xbox360_core = NUTHREADXBOX360CORE_UNKNOWN_1;

    params.use_current = true;

    thread = new NuThread(params);
    thread->SetDebugName("Main");

    g_currentThread = thread;

    return thread;
}

void NuThreadSleep(i32 seconds) {
    timespec time;

    if (seconds < 1000) {
        time.tv_sec = 0;
    } else {
        time.tv_sec = seconds / 1000;
        seconds %= 1000;
    }

    time.tv_nsec = seconds * 1000000;

    nanosleep(&time, NULL);
}
