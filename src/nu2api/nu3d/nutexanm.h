#pragma once
#include "decomp.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nufile/nufpar.h"
#include <string.h>

typedef struct numtl_s numtl_s;
typedef struct nutexanimprog_s nutexanimprog_s;
typedef struct nutexanimenv_s nutexanimenv_s;
typedef struct nutexanim_s nutexanim_s;
typedef struct nutexanimlist_s nutexanimlist_s;

struct nutexanimprog_s {
    nutexanimprog_s *next;
    nutexanimprog_s *previous;
    char name[32];
    i32 on_signal[32];
    i32 off_signal[32];
    u32 on_mask;
    u32 off_mask;
    i16 label_ids[32];
    i16 label_offsets[32];
    i32 label_count;
    i16 instruction_count;
    u8 flags;
    u8 reserved_1b7;
    u16 mask;
    i16 instructions[1];
};

struct nutexanimenv_s {
    nutexanimprog_s *program;
    i32 instruction_index;
    i32 loop_counts[16];
    i32 loop_starts[16];
    i32 loop_depth;
    i32 return_stack[16];
    i32 call_depth;
    i32 wait_base;
    i32 wait_random;
    i32 wait_remaining;
    numtl_s *material;
    u16 *texture_ids;
    i32 texture_index;
    u8 flags;
    u8 reserved_e9[3];
};

struct nutexanim_s {
    struct nutexanim_s *next;
    nutexanim_s *previous;
    u16 *texture_ids;
    i16 texture_count;
    u8 flags;
    u8 reserved_0f;
    numtl_s *material;
    struct nutexanimenv_s *env;
    char *name;
    char *program_name;
};

#ifdef __cplusplus
static_assert(sizeof(void *) != 4 || offsetof(nutexanim_s, next) == 0x00, "texture animation next offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanim_s, env) == 0x14, "texture animation environment offset");
static_assert(sizeof(void *) != 4 || sizeof(nutexanim_s) == 0x20, "texture animation size");
static_assert(sizeof(void *) != 4 || offsetof(nutexanim_s, program_name) == 0x1c,
              "texture animation program name offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, instruction_index) == 0x04,
              "texture animation instruction offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, loop_depth) == 0x88,
              "texture animation loop depth offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, call_depth) == 0xcc,
              "texture animation call depth offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, material) == 0xdc, "texture animation material offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, texture_ids) == 0xe0,
              "texture animation texture table offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, texture_index) == 0xe4,
              "texture animation texture index offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimenv_s, flags) == 0xe8, "texture animation flags offset");
static_assert(sizeof(void *) != 4 || sizeof(nutexanimenv_s) == 0xec, "texture animation environment size");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimprog_s, on_signal) == 0x28, "texture program on signal offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimprog_s, off_signal) == 0xa8,
              "texture program off signal offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimprog_s, label_ids) == 0x130, "texture program label IDs offset");
static_assert(sizeof(void *) != 4 || offsetof(nutexanimprog_s, instructions) == 0x1ba,
              "texture program instructions offset");
static_assert(sizeof(void *) != 4 || sizeof(nutexanimprog_s) == 0x1bc, "texture program size");
#endif

struct nutexanimlist_s {
    nutexanim_s *first;
    nutexanimlist_s *next;
    nutexanimlist_s *previous;
};

extern i32 nta_labels[64];
extern nutexanimlist_s ntalsysbuff[64];
extern nutexanimlist_s *ntal_first;
extern nutexanimlist_s *ntal_free;

void NuTexAnimProgInit(nutexanimprog_s *program);
nutexanimprog_s *NuTexAnimProgParseFile(i32 file, VARIPTR *buffer, VARIPTR end, i32 flags);

#ifdef __cplusplus
extern "C" {
#endif
    extern i32 g_texAnimCriticalSection;
    void NuTexAnimEnvReset(nutexanimenv_s *env);
    void NuTexAnimEnvProc(nutexanimenv_s *env);
    nutexanimenv_s *NuTexAnimEnvCreate(VARIPTR *buffer, numtl_s *material, u16 *ids, nutexanimprog_s *program);
    nutexanimprog_s *NuTexAnimProgFind(char *name);
    nutexanimprog_s *NuTexAnimProgCreate(VARIPTR *buffer, i32 instruction_count, char *name);
    void NuTexAnimProgDestroy(nutexanimprog_s *program);
    nutexanimprog_s *NuTexAnimProgRead(VARIPTR *buffer, char *path);
    void NuTexAnimProgWrite(char *path, nutexanimprog_s *program);
    nutexanimprog_s *NuTexAnimProgReadScript(char *path, VARIPTR *buffer);
    void NuTexAnimProgAssembleEnd(nutexanimprog_s *program);
    void NuTexAnimAddList(nutexanim_s *anim);
    void NuTexAnimProcessList(nutexanim_s *anim);
#ifdef __cplusplus
}
#endif
