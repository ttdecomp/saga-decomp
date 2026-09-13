#pragma once

#include <stdint.h>

#include "nu2api/nucore/common.h"

#if defined(__cplusplus) && !defined(HOST_BUILD) && !defined(__clang__)
#define DECOMP_ASSERT(condition, message) static_assert(condition, message)
#else
#define DECOMP_ASSERT(condition, message)
#endif

DECOMP_ASSERT(sizeof(void *) == 4, "Unsupported pointer size");

// Define undefined* types from ghidra
typedef u8 undefined;
typedef u8 byte;
typedef u8 dwfenc;
typedef u32 dword;
typedef u64 longlong;
typedef u8 uchar;
typedef u64 ulonglong;
typedef u8 undefined1;
typedef u16 undefined2;
typedef u32 undefined3;
typedef u32 undefined4;
typedef u64 undefined5;
typedef u64 undefined6;
typedef u64 undefined7;
typedef u64 undefined8;
typedef u16 ushort;
typedef u16 word;

struct APICHARACTERMODELLIST_s {
    i16 model_id;
    i16 count;
};

enum AREA_FLAGS {
    AREAFLAG_NONE = 0x0,
    AREAFLAG_SINGLE_BUFFER = 0x8,
    AREAFLAG_MINIKIT = 0x10,
    AREAFLAG_NO_GOLDBRICK = 0x800,
    AREAFLAG_TRUE_JEDI = 0x4000,
    AREAFLAG_TEST_AREA = 0x20,
    AREAFLAG_HUB_AREA = 0x40,
    AREAFLAG_OVERRIDE_THINGS_SCENE = 0x400,
    AREAFLAG_VEHICLE_AREA = 0x1,
    AREAFLAG_ENDING_AREA = 0x2,
    AREAFLAG_BONUS_AREA = 0x4,
    AREAFLAG_SUPER_BONUS_AREA = 0x100 | AREAFLAG_BONUS_AREA,
    AREAFLAG_NO_CHARACTER_COLLISION = 0x80,
    AREAFLAG_NOPICKUPGRAVITY = 0x200,
    AREAFLAG_NO_COMPLETION_POINTS = 0x2000,
    AREAFLAG_NO_FREEPLAY = 0x1000,
};

#define __used__ __attribute__((used)) // NOLINT(readability-identifier-naming)

// Reconstructed platform defaults can be overridden by the host harness, but
// the corresponding symbols in the Android target are strong.
#ifdef HOST_BUILD
#define SAGA_HOST_WEAK __attribute__((weak))
#define SAGA_HOST_PACKED_ALIGN4 __attribute__((packed, aligned(4)))
#else
#define SAGA_HOST_WEAK
#define SAGA_HOST_PACKED_ALIGN4
#endif

#ifdef HOST_BUILD
#define SAGA_HOST_STATIC static
#define SAGA_HOST_SAFE_MEMSET(dest, value, size)                                                                       \
    do {                                                                                                               \
        usize saga_memset_size = (size);                                                                               \
        if (saga_memset_size != 0) {                                                                                   \
            memset((dest), (value), saga_memset_size);                                                                 \
        }                                                                                                              \
    } while (0)
#define SAGA_HOST_SAFE_MEMMOVE(dest, source, size)                                                                     \
    do {                                                                                                               \
        usize saga_memmove_size = (size);                                                                              \
        if (saga_memmove_size != 0) {                                                                                  \
            memmove((dest), (source), saga_memmove_size);                                                              \
        }                                                                                                              \
    } while (0)
#define SAGA_UPPERCASE_CHAR(character, table) ((character) = toupper((unsigned char)(character)))

#include <stdio.h>
#include <time.h>

enum log_level {
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
};

static const char *log_level_names[] = {
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_WARN] = "WARN",
    [LOG_LEVEL_INFO] = "INFO",
    [LOG_LEVEL_DEBUG] = "DEBUG",
};
static const char *log_level_colors[] = {
    [LOG_LEVEL_ERROR] = "\x1b[31m", // Red
    [LOG_LEVEL_WARN] = "\x1b[33m",  // Yellow
    [LOG_LEVEL_INFO] = "\x1b[32m",  // Green
    [LOG_LEVEL_DEBUG] = "\x1b[90m", // Bright Black (Gray)
};
static const char *__ansi_reset = "\x1b[0m";

#include <stdarg.h>

static const enum log_level LOG_LEVEL = LOG_LEVEL_INFO;

static void __used__ _saga_log(enum log_level level, const char *file, i32 line, const char *func, const char *fmt,
                               ...) {
    if (level > LOG_LEVEL) {
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm = {};
    struct tm *local_tm = localtime(&ts.tv_sec);
    if (local_tm != NULL) {
        tm = *local_tm;
    }
    char time[32];
    snprintf(time, sizeof(time), "%02d:%02d:%02d.%06.0f", tm.tm_hour, tm.tm_min, tm.tm_sec, (float)ts.tv_nsec / 1e3);

    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s %s[%s] %s:%d: %s: ", time, log_level_colors[level], log_level_names[level], file, line, func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "%s\n", __ansi_reset);
    va_end(args);
}

#define LOG(level, ...) _saga_log(level, __FILENAME__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERR(...) LOG(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARN(...) LOG(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO(...) LOG(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO_IF(condition, ...)                                                                                    \
    do {                                                                                                               \
        if (condition) {                                                                                               \
            LOG_INFO(__VA_ARGS__);                                                                                     \
        }                                                                                                              \
    } while (0)

#define UNIMPLEMENTED(...) LOG_ERR("UNIMPLEMENTED: %s", #__VA_ARGS__)

#else

#define SAGA_HOST_STATIC
#define SAGA_HOST_SAFE_MEMSET(dest, value, size) memset((dest), (value), (size))
#define SAGA_HOST_SAFE_MEMMOVE(dest, source, size) memmove((dest), (source), (size))
#define SAGA_UPPERCASE_CHAR(character, table)                                                                          \
    do {                                                                                                               \
        if ((u32)(character) <= 0xff) {                                                                                \
            (character) = (table)[(character) + 1];                                                                    \
        }                                                                                                              \
    } while (0)

#define UNIMPLEMENTED(...)

#define LOG(_, ...)
#define LOG_ERR(...)
#define LOG_WARN(...)
#define LOG_INFO(...)
#define LOG_DEBUG(...)
#define LOG_INFO_IF(condition, ...)

#endif
