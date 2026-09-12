#include "legoapi/world/level.h"

#include <stdio.h>
#include <string.h>

#include "globals.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/characters/motion.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/common.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numusic/numusic.h"
#include "legoapi/world/levels/levels.h"

extern "C" char ConfigBuffer[0x10000];

void LevelScriptReStoreProgress(WORLDINFO_s *world, LEVELSCRIPTPROCESS_s *process) {
    if (world->level_progress != NULL && process != NULL && NuStrLen(process->name) != 0) {
        for (i32 index = 0; index < 32; ++index) {
            if (NuStrLen(world->level_progress->scripts[index].name) == 0)
                break;
            if (NuStrICmp(world->level_progress->scripts[index].name, process->name) == 0) {
                for (i32 parameter = 0; parameter < 4; ++parameter)
                    process->processor.params[parameter] = world->level_progress->scripts[index].params[parameter];
                LOG_INFO("level-script progress restored name=%s params=[%g, %g, %g, %g]", process->name,
                         process->processor.params[0], process->processor.params[1], process->processor.params[2],
                         process->processor.params[3]);
                break;
            }
        }
    }
}

// Defined in legoapi/gameobjects.cpp
void GameAudio_PlaySfxAndSetVolume(i32, nuvec_s *, float);

// These are extern (U) in the original level.cpp.o — defined here as stubs
// until the original defining file is decompiled.
LEVELDATA *levelconfig_ldata = NULL;

extern i16 GetMusicIndex(char *, nusound_filename_info_s *, i32);

extern "C" {
    struct LEVELDATA_s *PODRACELEVELS[11]; // Arrival1-4, Intro, B, C, A, Outro1, Outro2, Status
}

static void Credits_Init_Game(WORLDINFO *) {
}
static void Credits_Update_Game(WORLDINFO *) {
}
static void Credits_Draw_Game(WORLDINFO *) {
}

extern void NewGame(void);
extern void Door_Reset(void);
extern void BackDrop_ResetColours(void);
extern i16 id_DEFAULTCHARACTER[2];
extern i32 GetMenuID(void);

static NUVEC titlesstartpos;
static i32 Pictures_NumLevels;

static void Pictures_FixUp(NUGSCN **scene) {
    if (*scene != NULL) {
        Pictures_NumLevels = 0;
        for (i32 episode = 0; episode < EPISODECOUNT; episode++) {
            char name[72];
            sprintf(name, "EP_%i", episode + 1);
            NuSpecialFind(*scene, &LevHSpecial[10 + episode], name, 1);

            for (i32 chapter = 0; chapter < 8; chapter++) {
                sprintf(name, "EP_%i_CH_%i", episode + 1, chapter + 1);
                NuSpecialFind(*scene, &LevHSpecial[20 + Pictures_NumLevels], name, 1);
                Pictures_NumLevels++;
            }
        }

        static const char *bonus_names[] = {
            "pod_race", "anakin_flight", "gunship", "new_hope", "lego_city", "new_town",
        };
        for (i32 i = 0; i < 6; i++) {
            NuSpecialFind(*scene, &LevHSpecial[20 + Pictures_NumLevels], const_cast<char *>(bonus_names[i]), 1);
            Pictures_NumLevels++;
        }
    }
    LevTime[0] = 0.0f;
    LevTime[1] = 0.0f;
}

static void Titles_Init(WORLDINFO *world) {
    char title_name[72];

    NewGame();
    switch (Text_Language) {
        case 2:
            NuStrCpy(title_name, "titles_french");
            break;
        case 3:
            NuStrCpy(title_name, "titles_spanish");
            break;
        case 4:
            NuStrCpy(title_name, "titles_german");
            break;
        case 5:
            NuStrCpy(title_name, "titles_italian");
            break;
        case 8:
            NuStrCpy(title_name, "titles_danish");
            break;
        default:
            NuStrCpy(title_name, Text_Language == 0x12 ? "titles_us" : "titles_uk");
            break;
    }

    NuSpecialFind(world->current_gscn, &LevHSpecial[0], title_name, 1);
    LevAlpha = 1.0f;
    nuhspecial_s *title_special = &LevHSpecial[0];
    if (NuSpecialExistsFn(title_special) != 0) {
        NUMTX *draw_mtx = NuSpecialGetDrawMtx(title_special);
        LevMtx = *draw_mtx;
        NuSpecialSetVisibility(title_special, 0);
        titlesstartpos.x = LevMtx.m30;
        titlesstartpos.y = 0.0f;
        titlesstartpos.z = LevMtx.m32 - 0.5f;
    }

    TitlesAlpha = 1.0f;
    if (GAMEDEMO == 0) {
        PlayerID[0] = id_DEFAULTCHARACTER[0];
        PlayerID[1] = id_DEFAULTCHARACTER[1];
    } else {
        GAMEDEMO = 1;
    }

    Door_Reset();
    newgamealpha = 1.0f;
    if (GAMEDEMO == 0) {
        PlayerProgress[0].active = 0;
        PlayerProgress[1].active = 0;
        newgamefade = 0;
        newgame_menudrawoff = 0;
    }
    BackDrop_ResetColours();
    Pictures_FixUp(&world->scene);
}

static void Titles_Update(WORLDINFO *) {
    if (NuSpecialExistsFn(&LevHSpecial[0]) != 0) {
        i32 menu_id = GetMenuID();
        f32 target =
            (MenuLoadOccurred == 0 && MenuSaveOccurred == 0 && (static_cast<u32>(menu_id + 1) < 2 || menu_id == 1))
                ? 1.0f
                : 0.0f;
        LevAlpha = SeekLinearF(LevAlpha, target, FRAMETIME + FRAMETIME);
    }

    TitlesAlpha = SeekLinearF(TitlesAlpha, 1.0f, FRAMETIME + FRAMETIME);
    LevTime[1] = 0.0f;
    if (GetMenuID() == 0 && GameTimer.time_elapsed >= 4.0f) {
        LevTime[1] = 1.0f;
    }
    LevTime[0] = SeekLinearF(LevTime[0], LevTime[1], FRAMETIME + FRAMETIME);
}

static void Titles_Draw(WORLDINFO *) {
    NUMTX draw_mtx;
    if (NuSpecialExistsFn(&LevHSpecial[0]) != 0 && LevAlpha > 0.0f) {
        draw_mtx = LevMtx;
        if (GameTimer.time_elapsed < 4.0f) {
            f32 t = NuTrigTable[(i32)(GameTimer.time_elapsed * 0.25f * 16384.0f) >> 1 & 0x7fff];
            draw_mtx.m30 = titlesstartpos.x + (LevMtx.m30 - titlesstartpos.x) * t;
            draw_mtx.m31 = titlesstartpos.y + (LevMtx.m31 - titlesstartpos.y) * t;
            draw_mtx.m32 = titlesstartpos.z + (LevMtx.m32 - titlesstartpos.z) * t;
        }

        NUVEC scale = {0.825f, 0.825f, 0.825f};
        NuMtxPreScale(&draw_mtx, &scale);
        f32 alpha = 1.0f - NuTrigTable[(i32)(LevAlpha * 16384.0f + 16384.0f) >> 1 & 0x7fff];
        NuSpecialDrawAtAlpha(&LevHSpecial[0], &draw_mtx, newgamealpha * TitlesAlpha * alpha);
    }

    if (LevTime[0] <= 0.0f || newgamealpha <= 0.0f) {
        return;
    }

    f32 time = GameTimer.time_elapsed;
    if (time > 30.0f) {
        time = NuFmod(time - 4.0f, 26.0f) + 4.0f;
    }
    draw_mtx = LevMtx;

    f32 alpha;
    if (time > 4.0f) {
        if (time < 5.0f)
            alpha = time - 4.0f;
        else if (time < 8.166666f)
            alpha = 1.0f;
        else if (time < 9.166666f)
            alpha = 1.0f - (time - 8.166666f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[10]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[10], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 8.166666f) {
        if (time < 9.166666f)
            alpha = time - 8.166666f;
        else if (time < 12.333332f)
            alpha = 1.0f;
        else if (time < 13.333332f)
            alpha = 1.0f - (time - 12.333332f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[11]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[11], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 12.333333f) {
        if (time < 13.333333f)
            alpha = time - 12.333333f;
        else if (time < 16.5f)
            alpha = 1.0f;
        else if (time < 17.5f)
            alpha = 1.0f - (time - 16.5f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[12]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[12], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 16.5f) {
        if (time < 17.5f)
            alpha = time - 16.5f;
        else if (time < 20.666666f)
            alpha = 1.0f;
        else if (time < 21.666666f)
            alpha = 1.0f - (time - 20.666666f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[13]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[13], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 20.666666f) {
        if (time < 21.666666f)
            alpha = time - 20.666666f;
        else if (time < 24.833332f)
            alpha = 1.0f;
        else if (time < 25.833332f)
            alpha = 1.0f - (time - 24.833332f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[14]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[14], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 24.833332f) {
        if (time < 25.833332f)
            alpha = time - 24.833332f;
        else if (time < 28.999998f)
            alpha = 1.0f;
        else if (time < 29.999998f)
            alpha = 1.0f - (time - 28.999998f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[15]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[15], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 29.0f) {
        if (time < 30.0f)
            alpha = time - 29.0f;
        else if (time < 33.166668f)
            alpha = 1.0f;
        else if (time < 34.166668f)
            alpha = 1.0f - (time - 33.166668f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[16]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[16], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
    if (time > 33.166664f) {
        if (time < 34.166664f)
            alpha = time - 33.166664f;
        else if (time < 37.333332f)
            alpha = 1.0f;
        else if (time < 38.333332f)
            alpha = 1.0f - (time - 37.333332f);
        else
            alpha = 0.0f;
        if (alpha > 0.0f && NuSpecialExistsFn(&LevHSpecial[17]) != 0) {
            draw_mtx.m31 = -0.15f;
            NuSpecialDrawAtAlpha(&LevHSpecial[17], &draw_mtx, newgamealpha * 0.5f * alpha * LevTime[0]);
        }
    }
}

i32 Text_StripComments(char *text, char *dest, i32 param) {
    char *start = text;
    char *end = text + NuStrLen(text);
    char *out = dest;
    while (text <= end) {
        char c = *text;
        if (c == '#' || c == ';' || (c == '/' && text[1] == '/')) {
            if (c == '/') {
                text++;
            }
            while (c != '\r' && c != '\n' && c != '\0') {
                c = *++text;
            }
            *out++ = c;
            text++;
        } else {
            if (param != 0 && (c == ',' || c == '=')) {
                c = ' ';
            }
            *out++ = c;
            text++;
        }
    }
    *out = '\0';
    return (i32)(out - dest);
}

LEVELDATA *LDataList = NULL;
LEVELDATA *NEWGAME_LDATA = NULL;
LEVELDATA *LOADGAME_LDATA = NULL;

static i32 MAXLDATA = 0x64;

i32 LEVELCOUNT;

LEVELDATA *Levels_ConfigureList(char *file, VARIPTR *buf, VARIPTR *buf_end, i32 max_level_count, i32 *level_count_out,
                                LEVELSETDEFAULTSFN *set_defaults_fn) {
    NUFPAR *parser;
    i32 in_level_config;
    i32 n;
    LEVELDATA *level_data;
    LEVELDATA *cur_level;
    i32 i;
    i32 j;

    parser = NuFParCreate(file);

    MAXLDATA = max_level_count;
    n = 0;

    level_data = (LEVELDATA *)ALIGN(buf->addr, 0x4);
    buf->void_ptr = level_data;

    cur_level = level_data;

    in_level_config = 0;

    while (NuFParGetLine(parser) != 0) {
        NuFParGetWord(parser);
        if (parser->word_buf[0] == '\0') {
            continue;
        }

        if (in_level_config) {
            if (NuStrICmp(parser->word_buf, "level_end") == 0) {
                in_level_config = 0;

                if (cur_level->dir[0] == '\0' || cur_level->name[0] == '\0' || (cur_level->flags & LEVEL_TEST) != 0) {
                    continue;
                }

                if ((cur_level->flags & LEVEL_NEWGAME) != 0) {
                    NEWGAME_LDATA = cur_level;
                }

                if ((cur_level->flags & LEVEL_LOADGAME) != 0) {
                    LOADGAME_LDATA = cur_level;
                }

                n++;
                in_level_config = 0;
                cur_level++;

                continue;
            }

            if (NuStrICmp(parser->word_buf, "dir") == 0) {
                if (NuFParGetWord(parser) != 0 && NuStrLen(parser->word_buf) < 0x40) {
                    NuStrCpy(cur_level->dir, parser->word_buf);
                }
            } else if (NuStrICmp(parser->word_buf, "file") == 0) {
                if (NuFParGetWord(parser) != 0 && NuStrLen(parser->word_buf) < 0x20) {
                    NuStrCpy(cur_level->name, parser->word_buf);
                }
            }

            if (NuStrICmp(parser->word_buf, "test_level") == 0) {
                cur_level->flags |= LEVEL_TEST;
            } else if (NuStrICmp(parser->word_buf, "intro_level") == 0) {
                cur_level->flags |= LEVEL_INTRO;
            } else if (NuStrICmp(parser->word_buf, "midtro_level") == 0 ||
                       NuStrICmp(parser->word_buf, "cutscene_level") == 0) {
                cur_level->flags |= LEVEL_MIDTRO;
            } else if (NuStrICmp(parser->word_buf, "outro_level") == 0) {
                cur_level->flags |= LEVEL_OUTRO;
            } else if (NuStrICmp(parser->word_buf, "status_level") == 0) {
                cur_level->flags &= ~LEVEL_GAMEPLAY;
                cur_level->flags &= ~LEVEL_TERRAIN;
                cur_level->flags |= LEVEL_STATUS;
            } else if (NuStrICmp(parser->word_buf, "newgame_level") == 0) {
                if (NEWGAME_LDATA == NULL) {
                    cur_level->flags |= LEVEL_NEWGAME;
                }
            } else if (NuStrICmp(parser->word_buf, "loadgame_level") == 0) {
                if (LOADGAME_LDATA == NULL) {
                    cur_level->flags |= LEVEL_LOADGAME;
                }
            }

            continue;
        }

        if (NuStrICmp(parser->word_buf, "level_start") == 0 && n < MAXLDATA) {
            cur_level->dir[0] = '\0';
            cur_level->name[0] = '\0';

            cur_level->unknown_060 = -1;

            cur_level->idx = n;

            cur_level->flags = LEVEL_GAMEPLAY | LEVEL_UNKNOWN_FLAG_4 | LEVEL_TERRAIN;

            cur_level->load_fn = NULL;
            cur_level->init_fn = NULL;
            cur_level->reset_fn = NULL;
            cur_level->update_fn = NULL;
            cur_level->always_update_fn = NULL;
            cur_level->draw_fn = NULL;
            cur_level->draw_status_fn = NULL;

            cur_level->data_display.unknown_14 = 20000;
            cur_level->data_display.unknown_00 = 0.1f;
            cur_level->data_display.unknown_04 = 0.15f;
            cur_level->data_display.far_clip = 20000.0f;
            cur_level->data_display.fog_start = 20100.0f;
            cur_level->data_display.particle_thin = g_isLowEndDevice ? 4.0f : 1.0f;
            cur_level->data_display.bg_red_bottom = 0;
            cur_level->data_display.bg_red_top = 0;
            cur_level->data_display.bg_green_bottom = 0;
            cur_level->data_display.bg_green_top = 0;
            cur_level->data_display.bg_blue_bottom = 0;
            cur_level->data_display.bg_blue_top = 0;

            cur_level->music_index = -1;

            cur_level->unknown_11c = 0.0f;
            cur_level->unknown_120 = 1.0f;

            cur_level->unknown_0a2 = -1;
            cur_level->max_ter_platforms = 0x80;
            cur_level->max_ter_groups = 0x100;
            cur_level->unknown_0a8 = -1;
            cur_level->unknown_0aa = -1;

            cur_level->mipmap_mode = 0x03;
            cur_level->blob_shadow_alpha = 0x7f;
            cur_level->unknown_0ae = -1;
            cur_level->area_index = -1;

            cur_level->unknown_0b8 = 0x50;

            cur_level->cam_tilt = 0.0f;

            cur_level->hover_height = 0.0f;

            cur_level->unknown_0b9 = 0x50;
            cur_level->unknown_0ba = 0x50;
            cur_level->unknown_0bb = 0x32;
            cur_level->unknown_0bc = 0x00;
            cur_level->unknown_0bd = 0x00;
            cur_level->unknown_0be = 0x00;
            cur_level->unknown_0bf = 0x00;

            cur_level->unknown_0c0 = 0.5f;
            cur_level->cam_pullback_dist = 0.0f;
            cur_level->cam_lateral_dist = 0.0f;
            cur_level->unknown_0cc = 2e+06f;

            cur_level->area_level_index = -1;
            cur_level->blob_shadow_fade_near = 5;
            cur_level->blob_shadow_fade_far = 10;
            cur_level->cam_pos_seek = 5;
            cur_level->cam_angle_seek = 5;
            cur_level->camera_judder_distance = 10;
            cur_level->unknown_0da = 5;
            cur_level->unknown_0db = 5;

            cur_level->conveyor_x_speed = 0.0f;
            cur_level->conveyor_z_speed = 0.0f;

            for (i = 0; i < 2; i++) {
                for (j = 0; j < 3; j++) {
                    cur_level->music_tracks[j][i] = -1;
                }
            }

            in_level_config = 1;

            if (set_defaults_fn != NULL) {
                (*set_defaults_fn)(cur_level);
            }
        }
    }

    NuFParDestroy(parser);

    if (n == 0) {
        return NULL;
    }

    buf->void_ptr = cur_level;

    if (level_count_out != NULL) {
        *level_count_out = n;
    }

    return level_data;
}

void Level_SetDefaults(LEVELDATA *level) {
    level->max_antinodes = 256;
    level->max_gizmo_blowups = 64;
    level->max_gizmo_blowup_types = 64;
    level->max_pickups = 256;
    level->max_obstacle_objs = 128;
    level->max_buildit_objs = 128;
    level->max_force_objs = 128;
    level->max_bombgen_objs = 16;

    level->max_tightropes = 10;
    level->max_giz_timers = 8;
    level->max_signals = 10;
    level->max_levers = 10;
    level->max_technos = 10;
    level->max_zipups = 10;
    level->max_grapples = 10;
    level->max_obstacles = 32;
    level->max_buildits = 16;
    level->max_shards = 64;
    level->max_spinners = 4;
    level->max_minicuts = 8;
    level->max_minicut_parts = 2;
    level->max_giz_specials = 32;
    level->max_attractos = 5;
    level->max_climb_objs = 10;
    level->max_guidelines = 32;
    level->max_ledges = 96;
    level->max_security_doors = 5;
    level->max_tubes = 5;
    level->max_giz_panels = 8;
    level->max_hat_machines = 2;
    level->max_force = 32;
    level->max_push_blocks = 16;
    level->max_push_block_end_pos = 8;
    level->max_doors = 16;
    level->max_teleports = 4;
    level->max_giz_randoms = 8;
    level->max_torp_machines = 2;
    level->max_spinner_anim_objs = 40;

    level->wind_size = 512.0f;
    level->wind_speed = 3.0f;

    level->max_turrets = 32;
    level->max_bombgens = 8;
    level->max_bridges = 5;
    level->max_plugs = 10;
}

LEVELDATA *Level_FindByName(char *name, i32 *idx_out) {
    for (i32 i = 0; i < LEVELCOUNT; i++) {
        if (NuStrICmp(LDataList[i].name, name) == 0) {
            if (idx_out != NULL) {
                *idx_out = i;
            }

            return &LDataList[i];
        }
    }

    if (idx_out != NULL) {
        *idx_out = -1;
    }

    return NULL;
}

void Level_Draw(WORLDINFO *world) {
    if (world->current_level->draw_fn != NULL) {
        world->current_level->draw_fn(world);
    }
}

LEVELFIXUP LevFixUp;

void Levels_FixUp(LEVELFIXUP *fixup) {
    if (fixup != NULL) {
        for (; fixup->name != NULL; fixup++) {
            if (fixup->level != NULL) {
                LEVELDATA *level = Level_FindByName(fixup->name, NULL);
                *fixup->level = level;
                if (level != NULL) {
                    if (fixup->load_fn != NULL) {
                        level->load_fn = fixup->load_fn;
                    }
                    if (fixup->init_fn != NULL) {
                        level->init_fn = fixup->init_fn;
                    }
                    if (fixup->reset_fn != NULL) {
                        level->reset_fn = fixup->reset_fn;
                    }
                    if (fixup->update_fn != NULL) {
                        level->update_fn = fixup->update_fn;
                    }
                    if (fixup->always_update_fn != NULL) {
                        level->always_update_fn = fixup->always_update_fn;
                    }
                    if (fixup->draw_fn != NULL) {
                        level->draw_fn = fixup->draw_fn;
                    }
                    if (fixup->draw_status_fn != NULL) {
                        level->draw_status_fn = fixup->draw_status_fn;
                    }
                }
            }
        }
    }
}

void Level_RegisterGameConfigKeywords(nufpcomjmp_s *beforeLoadKeywords, nufpcomjmp_s *afterLoadKeywords) {
    Level_ConfigBeforeLoad_GameKeywords = beforeLoadKeywords;
    Level_ConfigAfterLoad_GameKeywords = afterLoadKeywords;
}

LEVELDATA *ANEWHOPELEVELS[4];
void Credits_Load(WORLDINFO_s *, variptr_u *, variptr_u *);

void FixUpLevels(LEVELFIXUP *fixup) {
    Levels_FixUp(fixup);

    LEVELDATA *level = Level_FindByName("titles", NULL);
    TITLES_LDATA = level;
    if (level != NULL) {
        level->init_fn = Titles_Init;
        level->update_fn = Titles_Update;
        level->draw_fn = Titles_Draw;
        level->flags &= ~(LEVEL_GAMEPLAY | LEVEL_TERRAIN);
        level->music_index = (i16)GetMusicIndex("titles", MusicInfo, -1);

        TITLES_LDATA->music_tracks[0][0] = TITLES_LDATA->music_tracks[0][1] =
            music_man.GetTrackHandle(TRACK_CLASS_QUIET, "titles");
        TITLES_LDATA->music_tracks[1][0] = TITLES_LDATA->music_tracks[1][1] =
            music_man.GetTrackHandle(TRACK_CLASS_ACTION, "titles");
        TITLES_LDATA->music_tracks[2][0] = TITLES_LDATA->music_tracks[2][1] =
            music_man.GetTrackHandle(TRACK_CLASS_NOMUSIC, "titles");
    }

    level = Level_FindByName("status", NULL);
    STATUS_LDATA = level;
    if (level != NULL) {
        level->update_fn = UpdateStatusScreen;
        level->draw_status_fn = DrawStatusScreen;
        level->flags = (level->flags & ~(LEVEL_GAMEPLAY | LEVEL_TERRAIN)) | LEVEL_STATUS;
    }

    {
        LEVELDATA *level = Level_FindByName("credits", NULL);
        CREDITS_LDATA = level;
        if (level != NULL) {
            level->load_fn = Credits_Load;
            level->init_fn = Credits_Init_Game;
            level->update_fn = Credits_Update_Game;
            level->draw_fn = Credits_Draw_Game;
            level->draw_status_fn = Credits_DrawPanel;
            level->flags = 0;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("map", NULL);
        HUB_LDATA = level;
        if (level != NULL) {
            level->load_fn = Hub_Load;
            level->init_fn = Hub_Init;
            level->reset_fn = Hub_Reset;
            level->update_fn = Hub_Update;
            level->draw_fn = Hub_Draw3D;
            level->draw_status_fn = Hub_DrawPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("speederchase_a", NULL);
        SPEEDERCHASEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = SpeederChaseA_Init;
            level->reset_fn = SpeederChaseA_Reset;
            level->update_fn = SpeederChaseA_Update;
            level->draw_status_fn = SpeederChaseA_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("negotiations_a", NULL);
        NEGOTIATIONSA_LDATA = level;
        if (level != NULL) {
            level->init_fn = NegotiationsA_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("negotiations_b", NULL);
        NEGOTIATIONSB_LDATA = level;
        if (level != NULL) {
            level->init_fn = NegotiationsB_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("negotiations_c", NULL);
        NEGOTIATIONSC_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("gungan_a", NULL);
        GUNGAN_A_LDATA = level;
        if (level != NULL) {
            level->init_fn = GunganA_Init;
            level->update_fn = GunganA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("gungan_b", NULL);
        GUNGAN_B_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("rescue_a", NULL);
        RESCUEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = RescueA_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("rescue_b", NULL);
        RESCUEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = RescueB_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("rescue_c", NULL);
        RESCUEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = RescueC_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("rescue_e", NULL);
        RESCUEE_LDATA = level;
        if (level != NULL) {
            level->init_fn = RescueE_Init;
        }
    }

    // PodRace_Arrival/Intro/Outro array — original fills PODRACELEVELS[11]
    // order matches .rodata 0x565c20: Arrival1-4, Intro, B, C, A, Outro1, Outro2, Status
    {
        LEVELDATA *level = Level_FindByName("PodRace_Arrival1", NULL);
        PODRACELEVELS[0] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Arrival2", NULL);
        PODRACELEVELS[1] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Arrival3", NULL);
        PODRACELEVELS[2] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Arrival4", NULL);
        PODRACELEVELS[3] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Intro", NULL);
        PODRACELEVELS[4] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_B", NULL);
        PODRACELEVELS[5] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_C", NULL);
        PODRACELEVELS[6] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_A", NULL);
        PODRACELEVELS[7] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Outro1", NULL);
        PODRACELEVELS[8] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Outro2", NULL);
        PODRACELEVELS[9] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Status", NULL);
        PODRACELEVELS[10] = level;
    }

    {
        LEVELDATA *level = Level_FindByName("ANewHope_Intro", NULL);
        ANEWHOPELEVELS[0] = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("ANewHope_A", NULL);
        ANEWHOPELEVELS[1] = level;
        if (level != NULL) {
            level->init_fn = ANewHopeA_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("ANewHope_B", NULL);
        ANEWHOPELEVELS[2] = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("ANewHope_Status", NULL);
        ANEWHOPELEVELS[3] = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("podrace_b", NULL);
        PODRACEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = PodRaceBInit;
            level->reset_fn = PodRaceBReset;
            level->update_fn = PodRaceBUpdate;
            level->always_update_fn = PodRaceAlwasyUpdate;
            level->draw_status_fn = PodRacePanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("podrace_a", NULL);
        PODRACEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = PodRaceAInit;
            level->reset_fn = PodRaceAReset;
            level->update_fn = PodRaceAUpdate;
            level->always_update_fn = PodRaceA_AlwaysUpdate;
            level->draw_fn = PodRaceADraw;
            level->draw_status_fn = PodRacePanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("podrace_c", NULL);
        PODRACEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = PodRaceCInit;
            level->reset_fn = PodRaceCReset;
            level->update_fn = PodRaceCUpdate;
            level->always_update_fn = PodRaceAlwasyUpdate;
            level->draw_status_fn = PodRacePanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("PodRace_Outro1", NULL);
        PODRACEOUTRO1_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("podrace_status", NULL);
        PODRACESTATUS_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("PodSprint_A", NULL);
        PODSPRINTA_LDATA = level;
        if (level != NULL) {
            level->init_fn = PodSprintA_Init;
            level->reset_fn = PodSprintA_Reset;
            level->update_fn = PodSprintA_Update;
            level->draw_status_fn = PodSprintA_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("AnakinsFlight_B", NULL);
        ANAKINSFLIGHTB_LDATA = level;
        if (level != NULL) {
            level->init_fn = AnakinsFlightB_Init;
            level->update_fn = AnakinsFlightB_Update;
            level->draw_fn = AnakinsFlightB_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_intro1", NULL);
        RETAKEINTRO1_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_intro2", NULL);
        RETAKEINTRO2_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_intro3", NULL);
        RETAKEINTRO3_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_b", NULL);
        RETAKEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_d", NULL);
        RETAKED_LDATA = level;
        if (level != NULL) {
            level->init_fn = RetakeD_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_e", NULL);
        RETAKEE_LDATA = level;
        if (level != NULL) {
            level->init_fn = RetakeE_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("retake_g", NULL);
        RETAKEG_LDATA = level;
        if (level != NULL) {
            level->init_fn = RetakeG_Init;
            level->reset_fn = RetakeG_Reset;
            level->update_fn = RetakeG_Update;
            level->draw_status_fn = RetakeG_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Maul_A", NULL);
        MAULA_LDATA = level;
        if (level != NULL) {
            level->init_fn = MaulA_Init;
            level->reset_fn = MaulA_Reset;
            level->update_fn = MaulA_Update;
            level->draw_status_fn = MaulA_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Maul_B", NULL);
        MAULB_LDATA = level;
        if (level != NULL) {
            level->init_fn = MaulB_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Maul_D", NULL);
        MAULD_LDATA = level;
        if (level != NULL) {
            level->init_fn = MaulD_Init;
            level->update_fn = MaulD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Maul_E", NULL);
        MAULE_LDATA = level;
        if (level != NULL) {
            level->init_fn = MaulE_Init;
            level->update_fn = MaulE_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Maul_F", NULL);
        MAULF_LDATA = level;
        if (level != NULL) {
            level->init_fn = MaulF_Init;
            level->reset_fn = MaulF_Reset;
            level->update_fn = MaulF_Update;
            level->draw_status_fn = MaulF_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Jedi_B", NULL);
        JEDI_B_LDATA = level;
        if (level != NULL) {
            level->init_fn = JediB_Init;
            level->reset_fn = JediB_Reset;
            level->update_fn = JediB_Update;
            level->draw_status_fn = JediB_DrawPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Jedi_Outro", NULL);
        JEDI_OUTRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("gunship_a", NULL);
        GUNSHIPA_LDATA = level;
        if (level != NULL) {
            level->init_fn = GunshipA_Init;
            level->update_fn = GunshipA_Update;
            level->draw_fn = GunshipA_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("gunship_b", NULL);
        GUNSHIPB_LDATA = level;
        if (level != NULL) {
            level->reset_fn = GunshipB_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("bonus_gunship_a", NULL);
        BONUS_GUNSHIPA_LDATA = level;
        if (level != NULL) {
            level->reset_fn = BonusGunshipA_Reset;
            level->update_fn = BonusGunshipA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("bonus_gunship_b", NULL);
        BONUS_GUNSHIPB_LDATA = level;
        if (level != NULL) {
            level->init_fn = BonusGunshipB_Init;
            level->reset_fn = BonusGunshipB_Reset;
            level->update_fn = BonusGunshipB_Update;
            level->draw_status_fn = BonusGunshipB_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("pursuit_a", NULL);
        BOUNTYHUNTERPURSUITA_LDATA = level;
        if (level != NULL) {
            level->init_fn = BountyHunterPursuitA_Init;
            level->reset_fn = BountyHunterPursuitA_Reset;
            level->update_fn = BountyHunterPursuitA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("pursuit_b", NULL);
        BOUNTYHUNTERPURSUITB_LDATA = level;
        if (level != NULL) {
            level->init_fn = BountyHunterPursuitB_Init;
            level->reset_fn = BountyHunterPursuitB_Reset;
            level->update_fn = BountyHunterPursuitB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("pursuit_c", NULL);
        BOUNTYHUNTERPURSUITC_LDATA = level;
        if (level != NULL) {
            level->init_fn = BountyHunterPursuitC_Init;
            level->reset_fn = BountyHunterPursuitC_Reset;
            level->update_fn = BountyHunterPursuitC_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("pursuit_d", NULL);
        BOUNTYHUNTERPURSUITD_LDATA = level;
        if (level != NULL) {
            level->init_fn = BountyHunterPursuitD_Init;
            level->reset_fn = BountyHunterPursuitD_Reset;
            level->update_fn = BountyHunterPursuitD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("pursuit_e", NULL);
        BOUNTYHUNTERPURSUITE_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dogfight_a", NULL);
        DOGFIGHTA_LDATA = level;
        if (level != NULL) {
            level->init_fn = ChrisDogFightAInit;
            level->reset_fn = ChrisDogFightAReset;
            level->update_fn = ChrisDogFightAUpdate;
            level->draw_fn = ChrisDogFightADraw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Factory_B", NULL);
        FACTORYB_LDATA = level;
        if (level != NULL) {
            level->init_fn = FactoryB_Init;
            level->reset_fn = FactoryB_Reset;
            level->update_fn = FactoryB_Update;
            level->draw_fn = FactoryB_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Factory_D", NULL);
        FACTORYD_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Factory_F", NULL);
        FACTORYF_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Factory_G", NULL);
        FACTORYG_LDATA = level;
        if (level != NULL) {
            level->init_fn = FactoryG_Init;
            level->update_fn = FactoryG_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Dooku_C", NULL);
        DOOKUC_LDATA = level;
        if (level != NULL) {
            level->init_fn = DookuC_Init;
            level->reset_fn = DookuC_Reset;
            level->update_fn = DookuC_Update;
            level->draw_status_fn = DookuC_DrawPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Dooku_Outro", NULL);
        DOOKUOUTRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_A", NULL);
        KAMINOA_LDATA = level;
        if (level != NULL) {
            level->always_update_fn = KaminoA_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_C", NULL);
        KAMINOC_LDATA = level;
        if (level != NULL) {
            level->reset_fn = KaminoC_Reset;
            level->init_fn = KaminoC_Init;
            level->update_fn = KaminoC_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_D", NULL);
        KAMINOD_LDATA = level;
        if (level != NULL) {
            level->init_fn = KaminoD_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_Outro1", NULL);
        KAMINOOUTRO_LDATA = level;
        if (level != NULL) {
            level->init_fn = KaminoOutro_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_A", NULL);
        KAMINOA_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_C", NULL);
        KAMINOC_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_E", NULL);
        KAMINOE_LDATA = level;
        if (level != NULL) {
            level->reset_fn = KaminoE_Reset;
            level->init_fn = KaminoE_Init;
            level->always_update_fn = KaminoE_AlwaysUpdate;
            level->update_fn = KaminoE_Update;
            level->draw_fn = KaminoE_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kamino_F", NULL);
        KAMINOF_LDATA = level;
        if (level != NULL) {
            level->init_fn = KaminoF_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("NB_Kamino_a", NULL);
        NB_KAMINOALDATA_LDATA = level;
        if (level != NULL) {
            level->init_fn = NbKaminoA_Init;
            level->always_update_fn = KaminoA_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Dogfight_A", NULL);
        DOGFIGHTA_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Cruiser_A", NULL);
        CRUISERA_LDATA = level;
        if (level != NULL) {
            level->init_fn = CruiserAInit;
            level->update_fn = CruiserAUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Cruiser_B", NULL);
        CRUISERB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Cruiser_C", NULL);
        CRUISERC_LDATA = level;
        if (level != NULL) {
            level->reset_fn = CruiserCReset;
            level->update_fn = CruiserCUpdate;
            level->draw_status_fn = CruiserCPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Cruiser_D", NULL);
        CRUISERD_LDATA = level;
        if (level != NULL) {
            level->init_fn = CruiserDInit;
            level->reset_fn = CruiserDReset;
            level->update_fn = CruiserDUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Cruiser_E", NULL);
        CRUISERE_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("cruiser_g", NULL);
        CRUISERG_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Grievous_A", NULL);
        GRIEVOUSA_LDATA = level;
        if (level != NULL) {
            level->init_fn = GrievousA_Init;
            level->reset_fn = GrievousA_Reset;
            level->update_fn = GrievousA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("temple_A", NULL);
        TEMPLEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = TempleA_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("temple_b", NULL);
        TEMPLEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("temple_c", NULL);
        TEMPLEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = TempleC_Init;
            level->always_update_fn = TempleC_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("temple_status", NULL);
        TEMPLESTATUS_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kashyyyk_A", NULL);
        KASHYYYKA_LDATA = level;
        if (level != NULL) {
            level->init_fn = KashyyykA_Init;
            level->reset_fn = KashyyykA_Reset;
            level->update_fn = KashyyykA_Update;
            level->draw_status_fn = KashyyykA_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kashyyyk_B", NULL);
        KASHYYYKB_LDATA = level;
        if (level != NULL) {
            level->init_fn = KashyyykB_Init;
            level->reset_fn = KashyyykB_Reset;
            level->update_fn = KashyyykB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kashyyyk_C", NULL);
        KASHYYYKC_LDATA = level;
        if (level != NULL) {
            level->init_fn = KashyyykC_Init;
            level->update_fn = KashyyykC_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Kashyyyk_D", NULL);
        KASHYYYKD_LDATA = level;
        if (level != NULL) {
            level->init_fn = KashyyykD_Init;
            level->reset_fn = KashyyykD_Reset;
            level->update_fn = KashyyykD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("vader_a", NULL);
        VADERA_LDATA = level;
        if (level != NULL) {
            level->init_fn = VaderA_Init;
            level->reset_fn = VaderA_Reset;
            level->update_fn = VaderA_Update;
            level->draw_status_fn = VaderA_DrawPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("vader_b", NULL);
        VADERB_LDATA = level;
        if (level != NULL) {
            level->init_fn = VaderB_Init;
            level->reset_fn = VaderB_Reset;
            level->update_fn = VaderB_Update;
            level->draw_status_fn = VaderB_DrawPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("vader_c", NULL);
        VADERC_LDATA = level;
        if (level != NULL) {
            level->init_fn = VaderC_Init;
            level->reset_fn = VaderC_Reset;
            level->update_fn = VaderC_Update;
            level->draw_status_fn = VaderC_DrawPanel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("BlockadeRunner_B", NULL);
        BLOCKADERUNNERB_LDATA = level;
        if (level != NULL) {
            level->init_fn = BlockadeRunnerB_Init;
            level->update_fn = BlockadeRunnerB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("BlockadeRunner_C", NULL);
        BLOCKADERUNNERC_LDATA = level;
        if (level != NULL) {
            level->init_fn = BlockadeRunnerC_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("BlockadeRunner_D", NULL);
        BLOCKADERUNNERD_LDATA = level;
        if (level != NULL) {
            level->reset_fn = BlockadeRunnerD_Reset;
            level->update_fn = BlockadeRunnerD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("tatooine_b", NULL);
        TATOOINEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = TatooineB_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("tatooine_c", NULL);
        TATOOINEC_LDATA = level;
        level->init_fn = TatooineC_Init;
    }

    {
        LEVELDATA *level = Level_FindByName("moseisley_a", NULL);
        MOSEISLEYA_LDATA = level;
        if (level != NULL) {
            level->init_fn = MosEisleyA_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("moseisley_b", NULL);
        MOSEISLEYB_LDATA = level;
        if (level != NULL) {
            level->init_fn = MosEisleyB_Init;
            level->update_fn = MosEisleyB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("moseisley_c", NULL);
        MOSEISLEYC_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("moseisley_d", NULL);
        MOSEISLEYD_LDATA = level;
        if (level != NULL) {
            level->init_fn = MosEisleyD_Init;
            level->always_update_fn = MosEisleyD_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("moseisley_e", NULL);
        MOSEISLEYE_LDATA = level;
        if (level != NULL) {
            level->init_fn = MosEisleyE_Init;
            level->reset_fn = MosEisleyE_Reset;
            level->update_fn = MosEisleyE_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("tatooine_a", NULL);
        TATOOINEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = TatooineA_Init;
            level->update_fn = TatooineA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("tatooine_b", NULL);
        TATOOINEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("tatooine_d", NULL);
        TATOOINED_LDATA = level;
        if (level != NULL) {
            level->init_fn = TatooineD_Init;
            level->update_fn = TatooineD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("tatooine_e", NULL);
        TATOOINEE_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarrescue_a", NULL);
        DEATHSTARRESCUEA_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarrescue_b", NULL);
        DEATHSTARRESCUEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStarRescueB_Init;
            level->update_fn = DeathStarRescueB_Update;
            level->always_update_fn = DeathStarRescueB_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarrescue_c", NULL);
        DEATHSTARRESCUEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStarRescueC_Init;
            level->always_update_fn = DeathStarRescueC_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarrescue_d", NULL);
        DEATHSTARRESCUED_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarrescue_e", NULL);
        DEATHSTARRESCUEE_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarescape_a", NULL);
        DEATHSTARESCAPEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStarEscapeA_Init;
            level->update_fn = DeathStarEscapeA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarescape_b", NULL);
        DEATHSTARESCAPEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStarEscapeB_Init;
            level->update_fn = DeathStarEscapeB_Update;
            level->always_update_fn = DeathStarEscapeB_AlwaysUpdate;
            level->draw_fn = DeathStarEscapeB_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarescape_c", NULL);
        DEATHSTARESCAPEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStarEscapeC_Init;
            level->update_fn = DeathStarEscapeC_Update;
            level->reset_fn = DeathStarEscapeC_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarescape_d", NULL);
        DEATHSTARESCAPED_LDATA = level;
        if (level != NULL) {
            level->update_fn = DeathStarEscapeD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarbattle_a", NULL);
        DEATHSTARBATTLEA_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarbattle_b", NULL);
        DEATHSTARBATTLEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarbattle_c", NULL);
        DEATHSTARBATTLEC_LDATA = level;
        if (level != NULL) {
            level->always_update_fn = DeathStarBattleC_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("deathstarbattle_d", NULL);
        DEATHSTARBATTLED_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStarBattleDInit;
            level->update_fn = DeathStarBattleDUpdate;
            level->draw_fn = DeathStarBattleDDraw;
            level->reset_fn = DeathStarBattleDReset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothbattle_a", NULL);
        HOTHBATTLEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothBattleA_Init;
            level->reset_fn = HothBattleA_Reset;
            level->update_fn = HothBattleA_Update;
            level->draw_fn = HothBattleA_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothbattle_b", NULL);
        HOTHBATTLEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothbattle_c", NULL);
        HOTHBATTLEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothBattleC_Init;
            level->reset_fn = HothBattleC_Reset;
            level->update_fn = HothBattleC_Update;
            level->draw_fn = HothBattleC_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothbattle_d", NULL);
        HOTHBATTLED_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothbattle_e", NULL);
        HOTHBATTLEE_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothBattleE_Init;
            level->update_fn = HothBattleE_Update;
            level->draw_fn = HothBattleE_Draw;
            level->draw_status_fn = HothBattleE_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("HothBattle_Outro", NULL);
        HOTHBATTLEOUTRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothescape_a", NULL);
        HOTHESCAPEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothEscapeA_Init;
            level->update_fn = HothEscapeA_Update;
            level->reset_fn = HothEscapeA_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothescape_b", NULL);
        HOTHESCAPEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothEscapeB_Init;
            level->update_fn = HothEscapeB_Update;
            level->reset_fn = HothEscapeB_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothescape_c", NULL);
        HOTHESCAPEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothEscapeC_Init;
            level->update_fn = HothEscapeC_Update;
            level->always_update_fn = HothEscapeC_AlwaysUpdate;
            level->reset_fn = HothEscapeC_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("hothescape_d", NULL);
        HOTHESCAPED_LDATA = level;
        if (level != NULL) {
            level->init_fn = HothEscapeD_Init;
            level->update_fn = HothEscapeD_Update;
            level->reset_fn = HothEscapeD_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("AsteroidChase_A", NULL);
        ASTEROIDCHASEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = AsteroidChaseA_Init;
            level->reset_fn = AsteroidChaseA_Reset;
            level->update_fn = AsteroidChaseA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("AsteroidChase_B", NULL);
        ASTEROIDCHASEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = AsteroidChaseB_Init;
            level->reset_fn = AsteroidChaseB_Reset;
            level->update_fn = AsteroidChaseB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("AsteroidChase_C", NULL);
        ASTEROIDCHASEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = AsteroidChaseC_Init;
            level->reset_fn = AsteroidChaseC_Reset;
            level->update_fn = AsteroidChaseC_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("AsteroidChase_D", NULL);
        ASTEROIDCHASED_LDATA = level;
        if (level != NULL) {
            level->init_fn = AsteroidChaseD_Init;
            level->update_fn = AsteroidChaseD_Update;
            level->draw_fn = AsteroidChaseD_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("AsteroidChase_Midtro", NULL);
        ASTEROIDCHASEMITRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dagobah_a", NULL);
        DAGOBAHA_LDATA = level;
        if (level != NULL) {
            level->init_fn = DagobahA_Init;
            level->update_fn = DagobahA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dagobah_d", NULL);
        DAGOBAHD_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dagobah_e", NULL);
        DAGOBAHE_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dagobah_e", NULL);
        DAGOBAHE_LDATA = level;
        if (level != NULL) {
            level->init_fn = DagobahE_Init;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dagobah_b", NULL);
        DAGOBAHB_LDATA = level;
        if (level != NULL) {
            level->init_fn = DagobahB_Init;
            level->reset_fn = DagobahB_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("dagobah_c", NULL);
        DAGOBAHC_LDATA = level;
        if (level != NULL) {
            level->init_fn = DagobahC_Init;
            level->draw_status_fn = DagobahC_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("CloudCityTrap_A", NULL);
        CLOUDCITYTRAPA_LDATA = level;
        if (level != NULL) {
            level->init_fn = CloudCityTrapA_Init;
            level->update_fn = CloudCityTrapA_Update;
            level->reset_fn = CloudCityTrapA_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("CloudCityTrap_B", NULL);
        CLOUDCITYTRAPB_LDATA = level;
        if (level != NULL) {
            level->init_fn = CloudCityTrapB_Init;
            level->update_fn = CloudCityTrapB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("CloudCityTrap_C", NULL);
        CLOUDCITYTRAPC_LDATA = level;
        if (level != NULL) {
            level->reset_fn = CloudCityTrapC_Reset;
            level->update_fn = CloudCityTrapC_Update;
            level->draw_status_fn = CloudCityTrapC_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("CloudCityTrap_Outro", NULL);
        CLOUDCITYTRAPOUTRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("cloudcityescape_a", NULL);
        CLOUDCITYESCAPEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = CloudCityEscapeA_Init;
            level->reset_fn = CloudCityEscapeA_Reset;
            level->update_fn = CloudCityEscapeA_Update;
            level->draw_status_fn = CloudCityEscapeA_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("cloudcityescape_b", NULL);
        CLOUDCITYESCAPEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("cloudcityescape_c", NULL);
        CLOUDCITYESCAPEC_LDATA = level;
        if (level != NULL) {
            level->init_fn = CloudCityEscapeC_Init;
            level->update_fn = CloudCityEscapeC_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("jabbaspalace_a", NULL);
        JABBASPALACEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = JabbasPalaceA_Init;
            level->reset_fn = JabbasPalaceA_Reset;
            level->update_fn = JabbasPalaceA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("jabbaspalace_b", NULL);
        JABBASPALACEB_LDATA = level;
        if (level != NULL) {
            level->init_fn = JabbasPalaceB_Init;
            level->reset_fn = JabbasPalaceB_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("jabbaspalace_d", NULL);
        JABBASPALACED_LDATA = level;
        if (level != NULL) {
            level->reset_fn = JabbasPalaceD_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("jabbaspalace_e", NULL);
        JABBASPALACEE_LDATA = level;
        if (level != NULL) {
            level->init_fn = JabbasPalaceE_Init;
            level->reset_fn = JabbasPalaceE_Reset;
            level->update_fn = JabbasPalaceE_Update;
            level->draw_status_fn = JabbasPalaceE_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("jabbaspalace_Outro", NULL);
        JABBASPALACE_OUTRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("endorbattle_a", NULL);
        ENDORBATTLEA_LDATA = level;
        if (level != NULL) {
            level->init_fn = EndorBattleA_Init;
            level->update_fn = EndorBattleA_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("endorbattle_b", NULL);
        ENDORBATTLEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("endorbattle_c", NULL);
        ENDORBATTLEC_LDATA = level;
        level->init_fn = EndorBattleC_Init;
    }

    {
        LEVELDATA *level = Level_FindByName("endorbattle_d", NULL);
        ENDORBATTLED_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_A", NULL);
        DEATHSTAR2BATTLEA_LDATA = level;
        if (level != NULL) {
            level->always_update_fn = DeathStar2BattleA_AlwaysUpdate;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_B", NULL);
        DEATHSTAR2BATTLEB_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_D", NULL);
        DEATHSTAR2BATTLED_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStar2BattleD_Init;
            level->update_fn = DeathStar2BattleD_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_E", NULL);
        DEATHSTAR2BATTLEE_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStar2BattleFire_Init;
            level->update_fn = DeathStar2BattleFire_Update;
            level->draw_fn = DeathStar2BattleFire_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_F", NULL);
        DEATHSTAR2BATTLEF_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStar2BattleFire_Init;
            level->update_fn = DeathStar2BattleFire_Update;
            level->draw_fn = DeathStar2BattleFire_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_G", NULL);
        DEATHSTAR2BATTLEG_LDATA = level;
        if (level != NULL) {
            level->init_fn = DeathStar2BattleFire_Init;
            level->update_fn = DeathStar2BattleFire_Update;
            level->draw_fn = DeathStar2BattleFire_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("DeathStar2Battle_Midtro", NULL);
        DEATHSTAR2BATTLEMIDTRO_LDATA = level;
        if (level != NULL) {
        }
    }

    {
        LEVELDATA *level = Level_FindByName("EmperorFight_A", NULL);
        EMPERORFIGHTA_LDATA = level;
        if (level != NULL) {
            level->init_fn = EmperorFightA_Init;
            level->reset_fn = EmperorFightA_Reset;
            level->update_fn = EmperorFightA_Update;
            level->draw_status_fn = EmperorFightA_Panel;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("SarlaccPit_A", NULL);
        SARLACCPITA_LDATA = level;
        if (level != NULL) {
            level->reset_fn = SarlaccPitA_Reset;
            level->draw_fn = SarlaccPitA_Draw;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("SarlaccPit_B", NULL);
        SARLACCPITB_LDATA = level;
        if (level != NULL) {
            level->init_fn = SarlaccPitB_Init;
            level->reset_fn = SarlaccPitB_Reset;
            level->update_fn = SarlaccPitB_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("SarlaccPit_C", NULL);
        SARLACCPITC_LDATA = level;
        if (level != NULL) {
            level->init_fn = SarlaccPitC_Init;
            level->reset_fn = SarlaccPitC_Reset;
            level->update_fn = SarlaccPitC_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("lego_city", NULL);
        LEGOCITY_LDATA = level;
        if (level != NULL) {
            level->init_fn = LegoCity_Init;
            level->reset_fn = LegoCity_Reset;
            level->update_fn = LegoCity_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("Senate_a", NULL);
        SENATEA_LDATA = level;
        level->init_fn = SenateA_Init;
    }

    {
        LEVELDATA *level = Level_FindByName("new_town", NULL);
        NEWTOWN_LDATA = level;
        if (level != NULL) {
            level->init_fn = NewTown_Init;
            level->reset_fn = NewTown_Reset;
            level->update_fn = NewTown_Update;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("platform", NULL);
        PLATFORM_LDATA = level;
        if (level != NULL) {
            level->init_fn = Platform_Init;
            level->reset_fn = Platform_Reset;
        }
    }

    {
        LEVELDATA *level = Level_FindByName("E1CharacterBonus_A", NULL);
        E1CHARACTERBONUSA_LDATA = level;
        level->init_fn = E1CharacterBonus_Init;
    }

    {
        LEVELDATA *level = Level_FindByName("E2VehicleBonus_A", NULL);
        E2VEHICLEBONUSA_LDATA = level;
        if (level != NULL) {
        }
    }

    LEVELDATA *entry = LDataList;
    for (i32 i = 0; i < LEVELCOUNT; ++i, ++entry) {
        if ((entry->flags & LEVEL_STATUS) != 0 || entry == STATUS_LDATA) {
            entry->update_fn = UpdateStatusScreen;
            entry->draw_status_fn = DrawStatusScreen;
        }
    }
}

void Level_Update(WORLDINFO *world) {
    void (*updateFn)(WORLDINFO *) = world->current_level->update_fn;
    if (updateFn != NULL) {
        updateFn(world);
    }

    if (DoubleScoreTime > 0.0f) {
        if ((i32)(GameTimer.time_elapsed + GameTimer.time_elapsed) !=
            (i32)(GameTimer.last_time_elapsed + GameTimer.last_time_elapsed)) {
            GameAudio_PlaySfxAndSetVolume(0x35, NULL, DoubleScoreTime);
        }
    }

    if (AreaGlobals.values.field_0x00 == 0) {
        if (Cheats_CheckFlags(0x2000) != 0) {
            AreaGlobals.values.field_0x00 = 1;
        }
    }
}

i32 LevelObject_GetReflection(i32 objId) {
    if (objId != -1 && LevObjRef_FirstObj <= objId && objId <= LevObjRef_LastObj) {
        objId = (objId - LevObjRef_FirstObj) + LevObjRef_FirstRefObj;
    }
    return objId;
}

char *LevelObject_FindNameFromIndex(i32 index) {
    char *name = (char *)"";
    if (ObjTabList != NULL && index >= 0 && index < LEVELOBJECTCOUNT) {
        name = ObjTabList[index].name;
    }
    return name;
}

i32 LevelObject_FindIndexFromName(char *name) {
    if (ObjTabList != NULL && LEVELOBJECTCOUNT > 0) {
        for (i32 i = 0; i < LEVELOBJECTCOUNT; i++) {
            if (NuStrICmp(ObjTabList[i].name, name) == 0) {
                return i;
            }
        }
    }
    return -1;
}

i32 LevelObject_FindIndexFromName_RefOnly(char *name) {
    if (ObjTabList != NULL && LevObjRef_FirstObj != -1 && LevObjRef_FirstObj <= LevObjRef_LastObj) {
        for (i32 i = LevObjRef_FirstObj; i <= LevObjRef_LastObj; ++i) {
            if (NuStrICmp(ObjTabList[i].name, name) == 0) {
                return i;
            }
        }
    }
    return -1;
}

i32 LevelObject_AddExtra(char *name, i32 kind) {
    if (LEVELOBJECTCOUNT < LEVELOBJECTMAX && ExtraLevelObject_NameTable != NULL) {
        i32 nameLen = NuStrLen(name);
        char *nameDest = ExtraLevelObject_NameTable + ExtraLevelObject_NameTableIndex;
        LEVELOBJECT *obj = &ObjTabList[LEVELOBJECTCOUNT];
        if (nameLen + 1 + ExtraLevelObject_NameTableIndex < ExtraLevelObject_NameTableSize) {
            obj->kind = (u8)kind;
            obj->name = nameDest;
            LEVELOBJECTCOUNT++;
            EXTRALEVELOBJECTCOUNT++;
            NuStrCpy(nameDest, name);
            ExtraLevelObject_NameTableIndex += nameLen + 1;
            return 1;
        }
    }
    return 0;
}

void GameAnimSys_ClearProgress(i32 idx) {
    if (idx < 0) {
        return;
    }
    if (idx >= gameanimsysprogress.count) {
        return;
    }
    u8 *data = gameanimsysprogress.entries[idx];
    if (gameanimsysprogress.entry_size <= 0) {
        return;
    }
    for (i32 i = 0; i < gameanimsysprogress.entry_size; i++) {
        data[i] = 0;
    }
}

void ClearLevelProgress(i32 index, WORLDINFO *world) {
    if (index >= 0) {
        i32 offset = index * sizeof(LEVEL_PROGRESS_s);
        memset((u8 *)LevelProgressData + offset, 0, sizeof(LEVEL_PROGRESS_s));
        u8 *entry = (u8 *)LevelProgressData + offset;
        *(u32 *)(entry + 0x281c) = 0;
        *(u32 *)(entry + 0x2810) = 0x49f42400;
        if (world != NULL) {
            *reinterpret_cast<LEVEL_PROGRESS_DATA_s *>(entry) = world->progress_data;
        }
    }
    GizmoSysClearLevelProgress(NULL, index);
    GameAnimSys_ClearProgress(index);
}

void SetLevelExBlowupFlags(u32 flags) {
    EXBLOWUPFLAGS = flags;
}

u32 GetLevelExBlowupFlags(void) {
    return EXBLOWUPFLAGS;
}

void GoToNewLevel(i32 levelIdx) {
    NewLData = &LDataList[levelIdx];
    if (waiting_for_level != -1) {
        waiting_for_new_level = 1;
    }
}

void Level_LoadConfigFile(WORLDINFO *world) {
    char name[140];

    ConfigBuffer[0] = '\0';
    sprintf(name, "%s.txt", world->config_file);

    world->giz_buffer.void_ptr = (void *)ALIGN((usize)world->giz_buffer.void_ptr, 4);
    i32 bytesRead = NuFileLoadBuffer(name, world->giz_buffer.void_ptr, 0x10000);
    world->config_count = bytesRead;
    if (bytesRead > 0) {
        ((char *)world->giz_buffer.void_ptr)[bytesRead] = '\0';
        bytesRead = Text_StripComments((char *)world->giz_buffer.void_ptr, ConfigBuffer, 1);
        world->config_count = bytesRead;
    }
}
