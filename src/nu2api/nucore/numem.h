#pragma once

#include "nu2api/nucore/common.h"

typedef struct numemblklink_s {
    struct numemblklink_s *next;
} NUMEMBLKLINK;

typedef struct numemexternal_s {
    VARIPTR *cursor;
    VARIPTR end;
} NUMEMEXTERNAL;

typedef struct numemdiscardable_s {
    i32 capacity;
    i32 remaining;
    u8 *cursor;
    u32 reserved;
} NUMEMDISCARDABLE;

typedef enum numemblkflags_e { NUMEMBLK_EXTERNAL_STORAGE = 1 } NUMEMBLKFLAGS;

typedef struct numemblk_s {
    NUMEMBLKLINK *free_list;
    u32 stride;
    i32 capacity;
    i16 flags;
    u16 free_count;
} NUMEMBLK;

#ifdef __cplusplus
void NuMemBlkCheckFreeList(NUMEMBLK *pool);
NUMEMEXTERNAL *NuMemGetExternal(void);
void NuMemFree(void *ptr);
void *NuMemAlloc(i32 size);
void *NuMemValidateFn(void);
void NuMemFlushFn(void);
void NuMemDumpFn(i32 mode);
extern "C" {
#endif
    void NuMemSet128(void *ptr, u32 value, isize size);
    void NuMemCopy128(void *dest, const void *source, i32 count);
    void *NuMemAllocFn(u32 size);
    void *NuMemReAllocFn(void *ptr, u32 size);
    void NuMemFreeFn(void *ptr);
    void NuAllocHighInit(usize buffer, u32 size);
    void *NuAllocHigh(u32 size);
    void NuFreeHigh(void *ptr);
    void NuMemSetExternal(VARIPTR *cursor, VARIPTR *end);
    void NuMemSetHeap(void *heap);
    isize NuMemGetPeakAllocAddr(void);
    NUMEMDISCARDABLE *NuMemSetDiscardable(NUMEMDISCARDABLE *buffer);
    void NuMemFlushDiscardable(NUMEMDISCARDABLE *buffer);
    NUMEMDISCARDABLE *NuMemCreateDiscardable(i32 size);
    void NuMemDestroyDiscardable(NUMEMDISCARDABLE *buffer);
    i32 NuMemBlkSize(i32 element_size, i32 count, i32 alignment_mask);
    void *NuMemBlkAlloc(NUMEMBLK *pool);
    NUMEMBLK *NuMemBlkCreateEx(u32 element_size, i32 count, u32 alignment_mask, void *storage);
    void NuMemBlkFree(NUMEMBLK *pool, void *block);
    void NuMemBlkDestroy(NUMEMBLK *pool);
    NUMEMBLK *NuMemBlkCreate(u32 element_size, i32 count, u32 alignment_mask);
    NUMEMBLK *NuMemBlkCreateVari(u32 element_size, i32 count, u32 alignment_mask, VARIPTR *buffer);
    void *NuScratchAlloc32(i32 size);
    void NuScratchRelease(void);
#ifdef __cplusplus
}
#endif
