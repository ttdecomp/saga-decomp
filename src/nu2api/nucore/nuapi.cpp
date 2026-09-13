#include "nu2api/nucore/nuapi.h"

#include <stdarg.h>
#include <string.h>

extern "C" i32 Nu360GetCommandLine(char **arguments, i32 capacity) {
    i32 i;
    i32 count;
    i32 done;
    char *dest;
    char *source;
    // The Android original has no command-line provider, but retains this parser.
    char *command_line = NULL;
    i32 length;
    count = 0;
    if (command_line != NULL) {
        i = 0;
        done = 0;
        dest = arguments[0];
        source = command_line;
        if (*source == '\0') {
            return 0;
        }
        count = 0;
        length = strlen(source);
        while (i < length && !done) {
            if (*source == '\0' || *source == ' ') {
                if (*source == '\0') {
                    done = 1;
                    *dest = '\0';
                } else if (count >= capacity - 1) {
                    done = 1;
                    *dest = '\0';
                } else {
                    count++;
                    dest = arguments[count];
                }
            } else {
                *dest = *source;
                dest++;
            }
            source++;
            i++;
        }
        *dest = '\0';
    }
    return count + 1;
}

#include "decomp.h"

#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nuprim.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuvport.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/numath/nutrig.h"

// Nucore bootstrap helpers; display-list initialization is declared in nudlist.h.
extern "C" {
    void NuFramebufferInitEx(void);
    void NuPostEffectInit(u32, void *, void *);
    void NuAnimInit(i32 max_joints, VARIPTR *buf, VARIPTR buf_end);
}

void NuRndrInitGeneric(void); // nurndr.cpp (C++ linkage)
void bgProcInit(void);        // bgproc_android.cpp

NUAPI nuapi;

i32 nuapi_use_target_manager;
char *nuapi_target_manager_mac_address;

// Frame-end hooks (original bss @0x6bdaec / 0x6bdaf0 / 0x6bdad0).
void (*preRenderFlashingHack)(void) = NULL;
void (*postRenderFlashingHack)(void) = NULL;
void (*nuapi_endframe_callbackfn)(void) = NULL;

float nuapi_forced_frame_time;
i32 nuapi_max_fps = 60;

static i32 NUAPI_PADREC_DEFAULT_BUFFERSIZE = 0x500000;

void NuAPIInit(void) {
    memset(&nuapi, 0, sizeof(NUAPI));

    nuapi.fps = 60.0f;
    nuapi.video_mode = NUVIDEOMODE_NTSC;
    nuapi.video_swap_mode = NUVIDEO_SWAPMODE_ROLLING;
    nuapi.language = 1;
    nuapi.video_aspect = 0;
    nuapi.disable_os_menu_freeze = 0;
    nuapi.forced_frame_time = nuapi_forced_frame_time;
    nuapi.max_fps = nuapi_max_fps;

    NuTimeGet(&nuapi.time);

    nuapi.pad_record.buf_size = NUAPI_PADREC_DEFAULT_BUFFERSIZE;
    nuapi.pad_record.end_record_buttons = 0;
    nuapi.pad_record.end_play_buttons = 0;

    NuWindInitialise(nuapi.wind);
}

void NuCommandLine(i32 *argc, char ***argv) {
}

void NuDisableOSMenuFreeze(void) {
    nuapi.disable_os_menu_freeze = 1;
}

i32 NuInitHardware(VARIPTR *buf, VARIPTR *buf_end, i32 heap_size, ...) {
    i32 hostfs = 0;
    i32 streamsize = 0x200000;
    NUPAD *pad0 = NULL;
    NUPAD *pad1 = NULL;
    i32 videomode = 2;
    i32 resolution_x = 0;
    i32 resolution_y = 0;
    NUVIDEO_SWAPMODE swapmode = NUVIDEO_SWAPMODE_FIELDSYNC;
    i32 flags = 0;

    NuAPIInit();

    va_list args;
    va_start(args, heap_size);

    i32 setup_tok;
    do {
        setup_tok = va_arg(args, i32);

        LOG_DEBUG("NuInitHardware setup=%d", setup_tok);

        switch (setup_tok) {
            case NUAPI_SETUP_HOSTFS:
                hostfs = va_arg(args, i32);
                break;
            case NUAPI_SETUP_STREAMSIZE:
                streamsize = va_arg(args, i32);
                break;
            case NUAPI_SETUP_AUDIO:
            case NUAPI_SETUP_AUDIO_DISABLED:
                va_arg(args, i32);
                va_arg(args, i32);
                va_arg(args, i32);
                va_arg(args, i32);
                break;
            case NUAPI_SETUP_PAD0:
                pad0 = va_arg(args, NUPAD *);
                break;
            case NUAPI_SETUP_PAD1:
                pad1 = va_arg(args, NUPAD *);
                break;
            case NUAPI_SETUP_VIDEOMODE:
                videomode = va_arg(args, i32);
                break;
            case NUAPI_SETUP_RESOLUTION:
                resolution_x = va_arg(args, i32);
                resolution_y = va_arg(args, i32);
                break;
            case NUAPI_SETUP_SWAPMODE:
                swapmode = (NUVIDEO_SWAPMODE)va_arg(args, i32);
                break;
            case NUAPI_SETUP_0x46:
                if (va_arg(args, i32) != 0) {
                    flags |= 0x4;
                }
                break;
            case NUAPI_SETUP_0x47:
                if (va_arg(args, i32) != 0) {
                    flags |= 0x8;
                }
                break;
            case NUAPI_SETUP_0x49:
                if (va_arg(args, i32) != 0) {
                    flags |= 0x20;
                }
                break;
            case NUAPI_SETUP_0x4b:
                if (va_arg(args, i32) != 0) {
                    flags |= 0x80;
                }
                break;
            default:
                if (NuInitHardwareParseArgsPS(setup_tok, va_arg(args, char **)) == 0) {
                    switch (setup_tok) {
                        case NUAPI_SETUP_CDDVDMODE:
                            break;
                        case NUAPI_SETUP_GLASSRPLANE:
                            break;
                    }
                }
                break;
        }
    } while (setup_tok != NUAPI_SETUP_END);

    va_end(args);

    // Original tail order (nuapi TU): NuInitHardwarePS, NuVideoGetAspectPS,
    // NuFileInitEx, NuTrigInit, NuRndrInitEx(streamsize, buf), NuPrimInit,
    // NuVpInit, NuTexInitEx, NuDisplayListInit, NuFramebufferInitEx,
    // NuPostEffectInit, NuMtlInitEx, NuRndrInitGeneric, NuAnimInit, ...
    NuInitHardwarePS(buf, buf_end, heap_size);
    NuVideoGetAspectPS();
    NuFileInitEx(0, 0, 0);
    NuTrigInit();
    NuRndrInitEx(streamsize, buf);
    NuPrimInit(buf, *buf_end);
    NuVpInit();
    NuTexInitEx(buf, 0xbb8);
    NuDisplayListInit(buf, *buf_end);
    NuFramebufferInitEx();
    NuPostEffectInit(flags | 1, buf, buf_end->void_ptr);
    NuMtlInitEx(buf, 512);
    NuRndrInitGeneric();
    NuAnimInit(0xa0, buf, *buf_end);
    NuTimeInitPS();
    NuMtlInitOverride(128, buf, buf_end);
    bgProcInit(); // starts the background-loading thread used by bgPostRequest

    return 0;
}
