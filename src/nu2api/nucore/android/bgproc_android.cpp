#include "nu2api/nucore/bgproc.h"

#include <stdarg.h>
#include <string.h>

#include "nu2api/nucore/android/NuThread_android.h"
#include "nu2api/nucore/nucore.hpp"
#include "nu2api/nucore/nulst.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nucore/nuvuvec.hpp"

NuThreadBase *g_bgProcThread;

static NULSTHDR *procinfo_pool;
static i32 g_bgCritSec;

i32 multithreaded = 1;

i32 g_performingBgProcWorkCritSec = -1;

char *g_CrashDumpId = "CRASHDUMP_ID=LEGOSAGAANDROID_ANDROID_Feb_17_2014_01_01_01$";

static char dummyBuf[0x80];
static BGPROCINFO *cur_pi;

static NuThreadSemaphore events[2] = {
    NuThreadSemaphore(1),
    NuThreadSemaphore(1),
};

static void bgThreadMain(void *data) {
    NuStrNCpy(dummyBuf, g_CrashDumpId, 0x20);

    events[0].Wait();

    NuThreadCriticalSectionBegin(g_bgCritSec);
    cur_pi = (BGPROCINFO *)NuLstGetNext(procinfo_pool, NULL);
    NuThreadCriticalSectionEnd(g_bgCritSec);

    while (true) {
        cur_pi->work_started = true;

        NuThreadCriticalSectionBegin(g_performingBgProcWorkCritSec);

        if (cur_pi->do_fn != NULL) {
            (*cur_pi->do_fn)(cur_pi);
        }

        if (cur_pi->ack_fn != NULL) {
            (*cur_pi->ack_fn)(cur_pi);
        }

        NuThreadCriticalSectionEnd(g_performingBgProcWorkCritSec);

        NuThreadCriticalSectionBegin(g_bgCritSec);
        NuLstFree((NULNKHDR *)cur_pi);

        while (true) {
            cur_pi = (BGPROCINFO *)NuLstGetNext(procinfo_pool, NULL);

            NuThreadCriticalSectionEnd(g_bgCritSec);

            if (cur_pi != NULL) {
                break;
            }

            events[0].Wait();

            NuThreadCriticalSectionBegin(g_bgCritSec);
        }
    }
}

void bgProcInit() {
    static i32 init;

    if (init == 0) {
        init = 1;

        procinfo_pool = NuLstCreate(0x10, sizeof(BGPROCINFO));

        g_bgCritSec = NuThreadCreateCriticalSection();
        g_performingBgProcWorkCritSec = NuThreadCreateCriticalSection();

        g_bgProcThread = NuCore::m_threadManager->CreateThread(
            bgThreadMain, NULL, 0, "bgProc", 0x100000, NUTHREADCAFECORE_UNKNOWN_1, NUTHREADXBOX360CORE_UNKNOWN_1);
    }
}

BGPROCINFO *bgPostRequest(bgprocdofn *do_fn, bgprocackfn *ack_fn, void *data, i32 data_size) {
    BGPROCINFO local;
    BGPROCINFO *info;

    if (!multithreaded) {
        local.work_started = false;
        local.unknown_flag_2 = false;

        local.do_fn = do_fn;
        local.ack_fn = ack_fn;

        local.vars = NULL;
        local.var_count = 0;

        if (data_size != 0 && data != NULL) {
            memcpy(local.data, data, data_size);
        }

        if (do_fn != NULL) {
            (*do_fn)(&local);
        }

        if (ack_fn != NULL) {
            (*ack_fn)(&local);
        }

        // ORIG_BUG: This is returned uninitialized.
        return info;
    }

    NuThreadCriticalSectionBegin(g_bgCritSec);

    info = (BGPROCINFO *)NuLstAllocTail(procinfo_pool);
    if (info != NULL) {
        info->work_started = false;
        info->unknown_flag_2 = false;

        info->do_fn = do_fn;
        info->ack_fn = ack_fn;

        info->vars = NULL;
        info->var_count = 0;

        if (data_size != 0 && data != NULL) {
            memcpy(info->data, data, data_size);
        }
    }

    NuThreadCriticalSectionEnd(g_bgCritSec);

    events[0].Signal();

    return info;
}

BGPROCINFO *bgPostRequestV(bgprocdofn *do_fn, bgprocackfn *ack_fn, i32 first_type, ...) {
    NuThreadCriticalSectionBegin(g_bgCritSec);

    BGPROCINFO *info = reinterpret_cast<BGPROCINFO *>(NuLstAllocTail(procinfo_pool));
    if (info != NULL) {
        info->do_fn = do_fn;
        info->ack_fn = ack_fn;
        info->work_started = false;
        info->unknown_flag_2 = false;
        info->vars = reinterpret_cast<BGVAR *>(info->data);
        info->var_count = 0;

        BGVAR *var = info->vars;
        char *string_end = reinterpret_cast<char *>(&info->vars);
        BGPROCARGTYPE type = static_cast<BGPROCARGTYPE>(first_type);
        va_list args;
        va_start(args, first_type);

        while (type != BGPROC_ARG_END) {
            var->type = type;
            switch (type) {
                case BGPROC_ARG_I32:
                    var->value.i32_value = va_arg(args, i32);
                    break;
                case BGPROC_ARG_FLOAT:
                    // C varargs promote float to double.
                    var->value.float_value = static_cast<f32>(va_arg(args, f64));
                    break;
                case BGPROC_ARG_STRING: {
                    char *string = va_arg(args, char *);
                    string_end -= NuStrLen(string) + 1;
                    var->value.string_value = string_end;
                    NuStrCpy(string_end, string);
                    break;
                }
                case BGPROC_ARG_U32:
                    var->value.u32_value = va_arg(args, u32);
                    break;
                case BGPROC_ARG_POINTER:
                    var->value.pointer_value = va_arg(args, void *);
                    break;
                case BGPROC_ARG_BOOL:
                    var->value.i32_value = va_arg(args, i32);
                    break;
                case BGPROC_ARG_NONE:
                case BGPROC_ARG_END:
                    break;
            }

            ++info->var_count;
            ++var;
            type = static_cast<BGPROCARGTYPE>(va_arg(args, i32));
        }
        va_end(args);
    }

    NuThreadCriticalSectionEnd(g_bgCritSec);
    events[0].Signal();
    return info;
}

BGPROCINFO *bgGetProcActive(void) {
    if (cur_pi != NULL) {
        return cur_pi;
    }

    return (BGPROCINFO *)NuLstGetNext(procinfo_pool, NULL);
}

i32 bgProcIsBgThread(void) {
    return NuCore::m_threadManager->GetCurrentThread() == g_bgProcThread;
}
