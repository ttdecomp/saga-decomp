#include "globals.h"
#include "nu2api/nu3d/nutexanm.h"
#include <string.h>
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/nufile/nufile.h"

static u32 nta_sig_old;
static u32 nta_sig_off;
static u32 nta_sig_on;
static i32 nta_iframetime;
static u16 nta_script_mask;
static nutexanimprog_s *sys_progs;
static nutexanimprog_s *parprog;
static char labtab[64][21];
static i32 labtabcnt;
static char xdeflabtab[256][21];
static i32 xdeflabtabcnt;
i32 nta_labels[64];
nutexanimlist_s ntalsysbuff[64];
nutexanimlist_s *ntal_first;
nutexanimlist_s *ntal_free;

extern "C" void NuTexAnimSetMask(i32 mask) {
    script_mask = static_cast<u16>(mask);
}

extern "C" void NuTexAnimSetSignals(u32 signals) {
    const u32 previous_signals = nta_sig_old;
    nta_sig_off = (previous_signals | signals) ^ signals;
    nta_sig_on = ~previous_signals & signals;
    nta_sig_old = signals;
}

static i32 NuTexAnimLabelIndex(char *name, char (*table)[21], i32 *count) {
    if (strlen(name) > 20)
        name[20] = '\0';
    for (i32 i = 0; i < *count; ++i) {
        if (NuStrICmp(table[i], name) == 0)
            return i;
    }
    NuStrCpy(table[(*count)++], name);
    return *count - 1;
}

static void pftaRepeat(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i32 value1 = NuFParGetInt(parser);
    if (value0 == 0)
        value0 = -1;
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 13;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(value0);
    parprog->instructions[static_cast<i16>(index + 2)] = static_cast<i16>(value1);
    parprog->instruction_count = index + 3;
}

static void pftaRepend(nufpar_s *parser) {
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 14;
    parprog->instruction_count = index + 1;
}

static void pftaTexAdj(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i32 value1 = NuFParGetInt(parser);
    i32 value2 = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 2;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(value0);
    parprog->instructions[static_cast<i16>(index + 2)] = static_cast<i16>(value1);
    parprog->instructions[static_cast<i16>(index + 3)] = static_cast<i16>(value2);
    parprog->instruction_count = index + 4;
}

static void pftaTexAdjR(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i32 value1 = NuFParGetInt(parser);
    i32 value2 = NuFParGetInt(parser);
    i32 value3 = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 3;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(value0);
    parprog->instructions[static_cast<i16>(index + 2)] = static_cast<i16>(value1);
    parprog->instructions[static_cast<i16>(index + 3)] = static_cast<i16>(value2);
    parprog->instructions[static_cast<i16>(index + 4)] = static_cast<i16>(value3);
    parprog->instruction_count = index + 5;
}

static void pftaUntiltex(nufpar_s *parser) {
    NuFParGetWord(parser);
    char *comparison = parser->word_buf;
    i16 condition = 0;
    if (comparison[0] == '<') {
        condition = comparison[1] == '=' ? 3 : (comparison[1] == '>' ? 5 : 1);
    } else if (comparison[0] == '>') {
        condition = comparison[1] == '=' ? 4 : 2;
    } else if (comparison[0] == '!') {
        condition = 5;
    }
    i32 value = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 15;
    parprog->instructions[static_cast<i16>(index + 1)] = condition;
    parprog->instructions[static_cast<i16>(index + 2)] = static_cast<i16>(value);
    parprog->instruction_count = index + 3;
}

static void pftaScriptMask(nufpar_s *parser) {
    parprog->mask = static_cast<u16>(NuFParGetInt(parser));
}

static void pftaScriptname(nufpar_s *parser) {
    NuFParGetWord(parser);
    parser->word_buf[32] = '\0';
    strcpy(parprog->name, parser->word_buf);
}

static void pftaOn(nufpar_s *parser) {
    i32 signal = NuFParGetInt(parser);
    parprog->on_signal[signal] = parprog->instruction_count;
    parprog->on_mask |= 1u << signal;
}

static void pftaEnd(nufpar_s *parser) {
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 16;
    parprog->instruction_count = index + 1;
}

static void pftaOff(nufpar_s *parser) {
    i32 signal = NuFParGetInt(parser);
    parprog->off_signal[signal] = parprog->instruction_count;
    parprog->off_mask |= 1u << signal;
}

static void pftaRet(nufpar_s *parser) {
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 12;
    parprog->instruction_count = index + 1;
}

static void pftaTex(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 0;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(value0);
    parprog->instruction_count = index + 2;
}

static void pftaBtex(nufpar_s *parser) {
    NuFParGetWord(parser);
    char *comparison = parser->word_buf;
    i16 condition = 0;
    if (comparison[0] == '<') {
        condition = comparison[1] == '=' ? 3 : (comparison[1] == '>' ? 5 : 1);
    } else if (comparison[0] == '>') {
        condition = comparison[1] == '=' ? 4 : 2;
    } else if (comparison[0] == '!') {
        condition = 5;
    }
    i32 value = NuFParGetInt(parser);
    NuFParGetWord(parser);
    i32 label = NuTexAnimLabelIndex(parser->word_buf, labtab, &labtabcnt);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 11;
    parprog->instructions[static_cast<i16>(index + 1)] = condition;
    parprog->instructions[static_cast<i16>(index + 2)] = static_cast<i16>(value);
    parprog->instructions[static_cast<i16>(index + 3)] = static_cast<i16>(label);
    parprog->instruction_count = index + 4;
}

static void pftaGoto(nufpar_s *parser) {
    NuFParGetWord(parser);
    i32 label = NuTexAnimLabelIndex(parser->word_buf, labtab, &labtabcnt);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 9;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(label);
    parprog->instruction_count = index + 2;
}

static void pftaGosub(nufpar_s *parser) {
    NuFParGetWord(parser);
    i32 label = NuTexAnimLabelIndex(parser->word_buf, labtab, &labtabcnt);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 10;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(label);
    parprog->instruction_count = index + 2;
}

static void pftaLabel(nufpar_s *parser) {
    NuFParGetWord(parser);
    i32 label = NuTexAnimLabelIndex(parser->word_buf, labtab, &labtabcnt);
    nta_labels[label] = parprog->instruction_count;
}

static void pftaRate(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i32 value1 = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 7;
    parprog->instructions[static_cast<i16>(index + 1)] =
        static_cast<i16>(static_cast<i32>(value0 * (1.0f / 60.0f) * 4096.0f));
    parprog->instructions[static_cast<i16>(index + 2)] =
        static_cast<i16>(static_cast<i32>(value1 * (1.0f / 60.0f) * 4096.0f));
    parprog->instruction_count = index + 3;
}

static void pftaTexR(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 1;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(value0);
    parprog->instruction_count = index + 2;
}

static void pftaWait(nufpar_s *parser) {
    i32 value0 = NuFParGetInt(parser);
    i32 value1 = NuFParGetInt(parser);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 4;
    parprog->instructions[static_cast<i16>(index + 1)] =
        static_cast<i16>(static_cast<i32>(value0 * (1.0f / 60.0f) * 4096.0f));
    parprog->instructions[static_cast<i16>(index + 2)] =
        static_cast<i16>(static_cast<i32>(value1 * (1.0f / 60.0f) * 4096.0f));
    parprog->instruction_count = index + 3;
}

static void pftaXDef(nufpar_s *parser) {
    NuFParGetWord(parser);
    i32 label = NuTexAnimLabelIndex(parser->word_buf, xdeflabtab, &xdeflabtabcnt);
    i32 index = parprog->label_count;
    parprog->label_ids[index] = static_cast<i16>(label);
    parprog->label_offsets[index] = parprog->instruction_count;
    parprog->label_count = index + 1;
}

static void pftaXRef(nufpar_s *parser) {
    NuFParGetWord(parser);
    i32 label = NuTexAnimLabelIndex(parser->word_buf, xdeflabtab, &xdeflabtabcnt);
    i16 index = parprog->instruction_count;
    parprog->instructions[index] = 17;
    parprog->instructions[static_cast<i16>(index + 1)] = static_cast<i16>(label);
    parprog->instruction_count = index + 2;
}

static NUFPCOMJMP nutexanimcomtab[] = {
    {const_cast<char *>("repeat"), pftaRepeat},
    {const_cast<char *>("repend"), pftaRepend},
    {const_cast<char *>("texadj"), pftaTexAdj},
    {const_cast<char *>("texadjr"), pftaTexAdjR},
    {const_cast<char *>("untiltex"), pftaUntiltex},
    {const_cast<char *>("scriptmask"), pftaScriptMask},
    {const_cast<char *>("scriptname"), pftaScriptname},
    {const_cast<char *>("on"), pftaOn},
    {const_cast<char *>("end"), pftaEnd},
    {const_cast<char *>("off"), pftaOff},
    {const_cast<char *>("ret"), pftaRet},
    {const_cast<char *>("tex"), pftaTex},
    {const_cast<char *>("btex"), pftaBtex},
    {const_cast<char *>("goto"), pftaGoto},
    {const_cast<char *>("gosub"), pftaGosub},
    {const_cast<char *>("label"), pftaLabel},
    {const_cast<char *>("rate"), pftaRate},
    {const_cast<char *>("texr"), pftaTexR},
    {const_cast<char *>("wait"), pftaWait},
    {const_cast<char *>("xdef"), pftaXDef},
    {const_cast<char *>("xref"), pftaXRef},
    {NULL, NULL},
};

void NuTexAnimProgInit(nutexanimprog_s *program) {
    if (program == NULL)
        return;
    for (i32 i = 0; i < 32; ++i) {
        program->off_signal[i] = -1;
        program->on_signal[i] = -1;
    }
    program->instruction_count = 0;
    program->on_mask = 0;
    program->off_mask = 0;
    program->name[0] = '\0';
    program->label_count = 0;
    program->flags &= ~1;
    program->mask = 1;
}

void NuTexAnimResetList(nutexanim_s *anim) {
    for (; anim != NULL; anim = anim->next) {
        if (anim->env != NULL) {
            NuTexAnimEnvReset(anim->env);
        }
    }
}

nutexanimprog_s *NuTexAnimProgParseFile(i32 file, VARIPTR *buffer, VARIPTR, i32) {
    nutexanimprog_s *program;
    if (buffer == NULL) {
        program = static_cast<nutexanimprog_s *>(NU_ALLOC(0x400, 4, 1, "", 0));
    } else {
        program = reinterpret_cast<nutexanimprog_s *>(ALIGN(buffer->addr, alignof(nutexanimprog_s)));
    }
    labtabcnt = 0;
    memset(labtab, 0, sizeof(labtab));
    NUFPAR *parser = NuFParOpen(file);
    if (parser == NULL)
        return NULL;
    NuFParPushCom(parser, nutexanimcomtab);
    NuTexAnimProgInit(program);
    parprog = program;
    while (NuFParGetLine(parser) != 0) {
        i32 length = NuFParGetWord(parser);
        if (length != 0 && NuFParInterpretWord(parser) == 0 && parser->word_buf[0] != '\0' &&
            parser->word_buf[length - 1] == ':') {
            parser->word_buf[length - 1] = '\0';
            i32 label = NuTexAnimLabelIndex(parser->word_buf, labtab, &labtabcnt);
            nta_labels[label] = parprog->instruction_count;
        }
    }
    if (buffer != NULL) {
        buffer->i16_ptr = program->instructions + program->instruction_count;
    }
    NuFParClose(parser);
    NuTexAnimProgAssembleEnd(program);
    program->next = sys_progs;
    if (sys_progs != NULL)
        sys_progs->previous = program;
    program->previous = NULL;
    sys_progs = program;
    return program;
}

NURAND texanim_rand;
extern "C" void NuSevereWarning(const char *, ...);
void NuTexAnimResetList(nutexanim_s *anim);

static bool TextureCondition(i32 condition, i32 texture, i32 value) {
    switch (condition) {
        case 0:
            return texture == value;
        case 1:
            return texture < value;
        case 2:
            return texture > value;
        case 3:
            return texture <= value;
        case 4:
            return texture >= value;
        case 5:
            return texture != value;
        default:
            return false;
    }
}

extern "C" void NuTexAnimAddList(nutexanim_s *anim) {
    if (anim == NULL)
        return;
    NuThreadCriticalSectionBegin(g_texAnimCriticalSection);
    nutexanimlist_s *node = ntal_free;
    if (node != NULL) {
        if (node->next == NULL)
            NuSevereWarning("Ran out of texture anim slots!");
        ntal_free = node->next;
        node->previous = NULL;
        node->first = anim;
        node->next = ntal_first;
        if (ntal_first != NULL)
            ntal_first->previous = node;
        ntal_first = node;
    }
    NuThreadCriticalSectionEnd(g_texAnimCriticalSection);
}

extern "C" void NuTexAnimCreate(void) {
}

extern "C" void NuTexAnimDestroy(void) {
}

extern "C" nutexanimenv_s *NuTexAnimEnvCreate(VARIPTR *buffer, numtl_s *material, u16 *ids, nutexanimprog_s *program) {
    nutexanimenv_s *env;
    if (buffer == NULL) {
        env = static_cast<nutexanimenv_s *>(NU_ALLOC(sizeof(*env), alignof(nutexanimenv_s), 1, "", 0));
    } else {
        env = reinterpret_cast<nutexanimenv_s *>(ALIGN(buffer->addr, alignof(nutexanimenv_s)));
        buffer->addr = reinterpret_cast<usize>(env + 1);
    }
    if (env != NULL) {
        env->program = program;
        env->material = material;
        NuTexAnimEnvReset(env);
        env->texture_ids = ids;
        if (buffer == NULL)
            env->flags |= 1;
        else
            env->flags &= ~1;
    }
    return env;
}

extern "C" void NuTexAnimEnvDestroy(void) {
}

extern "C" void NuTexAnimEnvProc(nutexanimenv_s *env) {
    nutexanimprog_s *program = env->program;
    if (program == NULL || (program->mask & nta_script_mask) == 0)
        return;
    numtl_s *material = env->material;
    u32 signals = nta_sig_off & program->off_mask;
    if (signals != 0) {
        for (i32 i = 0; i < 32; ++i) {
            if ((signals & (1u << i)) != 0) {
                env->wait_remaining = 0;
                env->instruction_index = program->off_signal[i];
                env->call_depth = 0;
                env->loop_depth = 0;
                break;
            }
        }
    }
    signals = nta_sig_on & program->on_mask;
    if (signals != 0) {
        for (i32 i = 0; i < 32; ++i) {
            if ((signals & (1u << i)) != 0) {
                env->wait_remaining = 0;
                env->instruction_index = program->on_signal[i];
                env->call_depth = 0;
                env->loop_depth = 0;
                break;
            }
        }
    }
    if (env->wait_remaining != 0) {
        env->wait_remaining -= nta_iframetime;
        if (env->wait_remaining > 0)
            return;
    }
    for (;;) {
        i16 *instruction = program->instructions + env->instruction_index;
        i32 next_instruction;
        switch (static_cast<u16>(instruction[0])) {
            // Each texture opcode commits the material and yields immediately.
            case 0: {
                env->texture_index = instruction[1];
                next_instruction = env->instruction_index + 2;
                u16 *texture_id = &env->texture_ids[env->texture_index];
                material->tex_id = *texture_id;
                material->shader_desc.diffuse_map_tex_id[0] = *texture_id & 0x7fff;
                env->instruction_index = next_instruction;
                env->wait_remaining += env->wait_base;
                if (env->wait_random != 0)
                    env->wait_remaining += NuRand(&texanim_rand) % env->wait_random;
                if (env->wait_remaining < 0)
                    env->wait_remaining = 0;
                return;
            }
            case 1: {
                env->texture_index = NuRand(&texanim_rand) % instruction[1];
                next_instruction = env->instruction_index + 2;
                u16 *texture_id = &env->texture_ids[env->texture_index];
                material->tex_id = *texture_id;
                material->shader_desc.diffuse_map_tex_id[0] = *texture_id & 0x7fff;
                env->instruction_index = next_instruction;
                env->wait_remaining += env->wait_base;
                if (env->wait_random != 0)
                    env->wait_remaining += NuRand(&texanim_rand) % env->wait_random;
                if (env->wait_remaining < 0)
                    env->wait_remaining = 0;
                return;
            }
            case 2: {
                i32 texture = env->texture_index + instruction[1];
                if (texture < instruction[2])
                    texture = instruction[2];
                if (texture > instruction[3])
                    texture = instruction[3];
                env->texture_index = texture;
                next_instruction = env->instruction_index + 4;
                u16 *texture_id = &env->texture_ids[env->texture_index];
                material->tex_id = *texture_id;
                material->shader_desc.diffuse_map_tex_id[0] = *texture_id & 0x7fff;
                env->instruction_index = next_instruction;
                env->wait_remaining += env->wait_base;
                if (env->wait_random != 0)
                    env->wait_remaining += NuRand(&texanim_rand) % env->wait_random;
                if (env->wait_remaining < 0)
                    env->wait_remaining = 0;
                return;
            }
            case 3: {
                i32 texture = NuRand(&texanim_rand) % (instruction[2] - instruction[1] + 1);
                texture += instruction[1] + env->texture_index;
                if (texture < instruction[3])
                    texture = instruction[3];
                if (texture > instruction[4])
                    texture = instruction[4];
                env->texture_index = texture;
                next_instruction = env->instruction_index + 5;
                u16 *texture_id = &env->texture_ids[env->texture_index];
                material->tex_id = *texture_id;
                material->shader_desc.diffuse_map_tex_id[0] = *texture_id & 0x7fff;
                env->instruction_index = next_instruction;
                env->wait_remaining += env->wait_base;
                if (env->wait_random != 0)
                    env->wait_remaining += NuRand(&texanim_rand) % env->wait_random;
                if (env->wait_remaining < 0)
                    env->wait_remaining = 0;
                return;
            }
            case 4:
                env->wait_remaining += instruction[1];
                if (instruction[2] != 0)
                    env->wait_remaining += NuRand(&texanim_rand) % instruction[2];
                env->instruction_index += 3;
                if (env->wait_remaining < 0)
                    env->wait_remaining = 0;
                return;
            case 7:
                env->wait_base = instruction[1];
                env->wait_random = instruction[2];
                env->instruction_index += 3;
                continue;
            case 9:
                env->instruction_index = instruction[1];
                continue;
            case 10:
                env->return_stack[env->call_depth++] = env->instruction_index + 2;
                env->instruction_index = instruction[1];
                continue;
            case 11:
                if (TextureCondition(instruction[1], env->texture_index, instruction[2]))
                    env->instruction_index = instruction[3];
                else
                    env->instruction_index += 4;
                continue;
            case 12:
                env->instruction_index = env->return_stack[--env->call_depth];
                continue;
            case 13:
                env->loop_counts[env->loop_depth] = instruction[1];
                if (instruction[2] != 0)
                    env->loop_counts[env->loop_depth] += NuRand(&texanim_rand) % instruction[2];
                env->instruction_index += 3;
                env->loop_starts[env->loop_depth++] = env->instruction_index;
                continue;
            case 14:
                if (env->loop_counts[env->loop_depth - 1] == 0) {
                    --env->loop_depth;
                    ++env->instruction_index;
                } else {
                    env->instruction_index = env->loop_starts[env->loop_depth - 1];
                    --env->loop_counts[env->loop_depth - 1];
                }
                continue;
            case 15:
                if (TextureCondition(instruction[1], env->texture_index, instruction[2]) ||
                    env->loop_counts[env->loop_depth - 1] == 0) {
                    --env->loop_depth;
                    env->instruction_index += 3;
                } else {
                    --env->loop_counts[env->loop_depth - 1];
                    env->instruction_index = env->loop_starts[env->loop_depth - 1];
                }
                continue;
            case 16:
                if (env->wait_remaining < 0)
                    env->wait_remaining = 0;
                return;
            case 17:
                for (nutexanimlist_s *list = ntal_first; list != NULL; list = list->next) {
                    for (nutexanim_s *anim = list->first; anim != NULL; anim = anim->next) {
                        nutexanimenv_s *other = anim->env;
                        if (other == NULL || other == env || other->program == NULL)
                            continue;
                        for (i32 i = 0; i < other->program->label_count; ++i) {
                            if (other->program->label_ids[i] == instruction[1]) {
                                other->instruction_index = other->program->label_offsets[i];
                                other->wait_remaining = 0;
                                other->call_depth = 0;
                                other->loop_depth = 0;
                                break;
                            }
                        }
                    }
                }
                env->instruction_index += 2;
                continue;
            default:
                continue;
        }
    }
}

extern "C" void NuTexAnimEnvReset(nutexanimenv_s *env) {
    if (env == NULL) {
        return;
    }

    env->instruction_index = 0;
    env->loop_depth = 0;
    env->call_depth = 0;
    env->wait_base = 0;
    env->wait_random = 0;
    env->wait_remaining = 0;
    env->texture_index = 0;
}

extern "C" void NuTexAnimFind(void) {
}

extern "C" void NuTexAnimProgAssembleEnd(nutexanimprog_s *program) {
    i16 index = 0;
    while (index < program->instruction_count) {
        switch (program->instructions[index]) {
            case 0:
            case 1:
            case 17:
                index += 2;
                break;
            case 3:
                index += 5;
                break;
            case 4:
            case 7:
            case 13:
            case 15:
                index += 3;
                break;
            case 9:
            case 10:
                program->instructions[index + 1] = nta_labels[program->instructions[index + 1]];
                index += 2;
                break;
            case 11:
                program->instructions[index + 3] = nta_labels[program->instructions[index + 3]];
                index += 4;
                break;
            case 2:
                index += 4;
                break;
            case 12:
            case 14:
            case 16:
                ++index;
                break;
        }
    }
}

extern "C" void NuTexAnimProgCreate(void) {
}

extern "C" void NuTexAnimProgDestroy(void) {
}

extern "C" nutexanimprog_s *NuTexAnimProgFind(char *name) {
    for (nutexanimprog_s *program = sys_progs; program != NULL; program = program->next) {
        if (NuStrICmp(name, program->name) == 0)
            return program;
    }
    return NULL;
}

extern "C" void NuTexAnimProgRead(void) {
}

extern "C" void NuTexAnimProgReadCFG(void) {
}

extern "C" nutexanimprog_s *NuTexAnimProgReadScript(char *path, VARIPTR *buffer) {
    NUFILE file = NuFileOpen(path, NUFILE_READ);
    nutexanimprog_s *program = NULL;
    if (file != 0) {
        VARIPTR end = {};
        program = NuTexAnimProgParseFile(file, buffer, end, 0);
        NuFileClose(file);
    }
    return program;
}

extern "C" void NuTexAnimProgRelease(void) {
    sys_progs = NULL;
}

extern "C" void NuTexAnimProgSysInit(void) {
    sys_progs = NULL;
    for (i32 i = 0; i < 63; ++i)
        ntalsysbuff[i].next = &ntalsysbuff[i + 1];
    ntalsysbuff[63].next = NULL;
    xdeflabtabcnt = 0;
    nta_sig_old = 0;
    nta_sig_off = 0;
    nta_sig_on = 0;
    ntal_free = ntalsysbuff;
    ntal_first = NULL;
    texanim_rand.value = 0;
}

extern "C" void NuTexAnimProgWrite(void) {
}

extern "C" void NuTexAnimRemoveList(void *anim) {
    NuThreadCriticalSectionBegin(g_texAnimCriticalSection);
    for (nutexanimlist_s *node = ntal_first; node != NULL; node = node->next) {
        if (node->first != anim)
            continue;
        if (node->next != NULL)
            node->next->previous = node->previous;
        if (node->previous != NULL)
            node->previous->next = node->next;
        else
            ntal_first = node->next;
        node->next = ntal_free;
        ntal_free = node;
        break;
    }
    NuThreadCriticalSectionEnd(g_texAnimCriticalSection);
}

extern "C" void NuTexAnimRestart(void) {
    NuThreadCriticalSectionBegin(g_texAnimCriticalSection);
    for (nutexanimlist_s *node = ntal_first; node != NULL; node = node->next)
        NuTexAnimResetList(node->first);
    NuThreadCriticalSectionEnd(g_texAnimCriticalSection);
}

extern "C" void NuTexAnimProcess(f32 frame_time) {
    nta_iframetime = static_cast<i32>(frame_time * 4096.0f);
    nta_script_mask = script_mask;
    NuThreadCriticalSectionBegin(g_texAnimCriticalSection);
    for (nutexanimlist_s *node = ntal_first; node != NULL; node = node->next)
        NuTexAnimProcessList(node->first);
    NuThreadCriticalSectionEnd(g_texAnimCriticalSection);
}
extern "C" void NuTexAnimProcessEx(f32 frame_time, u16 mask) {
    nta_iframetime = static_cast<i32>(frame_time * 4096.0f);
    nta_script_mask = mask;
    NuThreadCriticalSectionBegin(g_texAnimCriticalSection);
    for (nutexanimlist_s *node = ntal_first; node != NULL; node = node->next)
        NuTexAnimProcessList(node->first);
    NuThreadCriticalSectionEnd(g_texAnimCriticalSection);
}
extern "C" void NuTexAnimProcessList(nutexanim_s *anim) {
    for (; anim != NULL; anim = anim->next) {
        if (anim->env != NULL)
            NuTexAnimEnvProc(anim->env);
    }
}
