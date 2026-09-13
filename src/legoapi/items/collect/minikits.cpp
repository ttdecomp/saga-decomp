#include "decomp.h"
#include "globals.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/render/core/render.h"
#include "nu2api/numath/nutrig.h"
#include <math.h>
#include <stdio.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void MiniKits_Init(variptr_u *, variptr_u *) {
}

void CollectMinikit(nuvec_s *, char *, i32) {
}

i32 AllMiniKitsDone(AREASAVE_s *save) {
    if (save == NULL) {
        return 1;
    }

    for (i32 i = 0; i < AREACOUNT; ++i, ++save) {
        if ((ADataList[i].flags & AREAFLAG_MINIKIT) != 0 && save->minikit_complete == SAVE_INCOMPLETE) {
            return 0;
        }
    }
    return 1;
}

void MiniKitDetector(nuvec_s *) {
}

void CharMiniKit_Draw(i32, numtx_s *, i32, float, float) {
}

extern i32 currentminikit, newminikitcount;
f32 slideseek;
f32 pop_timer;
f32 jibberlen = 0.1f;
f32 slidetime = 0.25f;
void NextStatusStage(STATUSPACKET_s *);
void SetDrawGoldBrick(STATUSPACKET_s *, i32);
void IncreaseScore(u32 *, u64, i32);
void NewStatusRumbleBuzz(i32, f32, f32, i32);
extern "C" void PlaySfx(char *, nuvec_s *);
i32 FindGameMsgsWithID(i32, i32, i32, GAMEMESSAGE_s *);
void AddStatusMiniKitParts();
void DrawStatusMiniKit(f32, f32, f32, f32, f32, i32, STATUSPACKET_s *, f32);
void DrawMiniKitCount(f32, f32, i32, i32);
f32 getFinishedStatusAlpha(STATUSPACKET_s *);
extern f32 STATUS_TITLE_Y;
extern i16 tMINIKIT;

void MiniKit_LSW_Draw(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, i32 current) {
    char text[60];
    if (current == 0) {
        if (stage->field_0x12 == 0)
            return;
        const f32 alpha = getFinishedStatusAlpha(packet);
        i32 angle = 0x2000;
        if (GameTimer.time_elapsed_mod_seconds <= 0.25f) {
            angle = (static_cast<i32>(GameTimer.time_elapsed_mod_seconds * 32768.0f + 16384.0f) >> 1) & 0x7fff;
        }
        if (Game.area_save[packet->area->index].field_0x5[0] == 0) {
            const f32 size = ((1.0f - fabsf(NuTrigTable[angle])) + 1.0f) * 1.2f;
            Text3DEx("?", -0.6f, -0.6f, 1.1f, size, size, size, 0, 255, 255, 255, static_cast<i32>(alpha * 128.0f));
        } else if (alpha > 0.0f) {
            DrawStatusMiniKit(-0.6f, -0.5f, 1.1f,
                              NuTrigTable[(static_cast<i32>(alpha * 16384.0f) >> 1) & 0x7fff] * 0.15f, 1.0f,
                              Game.area_save[packet->area->index].field_0x5[0], packet, 0.0f);
        }
        if (packet->minikit_max == Game.area_save[packet->area->index].field_0x5[0]) {
            Text3DEx("$", -0.6f, -0.7f, 1.0f, 0.8f, 0.8f, 0.8f, 0, 255, 0, 127, static_cast<i32>(alpha * 128.0f));
        } else {
            sprintf(text, "%i/%i", Game.area_save[packet->area->index].field_0x5[0], packet->minikit_max);
            Text3DEx(text, -0.6f, -0.8f, 1.0f, 0.5f, 0.5f, 0.5f, 0, 255, 0, 127, static_cast<i32>(alpha * 128.0f));
        }
        return;
    }
    f32 title_alpha;
    switch (stage->field_0x14) {
        case 0:
            title_alpha = 0.0f;
            break;
        case 1: {
            title_alpha = stage->field_0x18;
            i32 angle = 0x6000;
            if (title_alpha < 1.0f)
                angle = (static_cast<i32>(title_alpha * 32768.0f + 16384.0f) >> 1) & 0x7fff;
            const f32 blend = 1.0f - (NuTrigTable[angle] + 1.0f) * 0.5f;
            if (title_alpha <= stage->field_0x1c)
                DrawStatusMiniKit(0.0f, blend * -1.4f + 1.4f, 1.1f, 0.333f, 0.0f, currentminikit, packet, 0.0f);
            else
                DrawStatusMiniKit(0.0f, 0.0f, 1.1f, 0.333f, 0.0f, currentminikit, packet, 0.0f);
            const i32 count = packet->new_minikits < 1 ? Game.area_save[packet->area->index].field_0x5[0]
                              : currentminikit < 0     ? 0
                                                       : currentminikit;
            DrawMiniKitCount(blend, 1.0f, count, packet->minikit_max);
            break;
        }
        case 2: {
            const f32 duration = stage->field_0x1c - 0.25f;
            f32 size;
            if (stage->field_0x18 < duration) {
                i32 angle = 0;
                if (duration != 0.0f && stage->field_0x18 != 0.0f)
                    angle = (static_cast<i32>((stage->field_0x18 / duration) * 16384.0f + 49152.0f + 16384.0f) >> 1) &
                            0x7fff;
                size = NuTrigTable[angle] * 0.333f;
            } else
                size = 0.333f;
            DrawStatusMiniKit(0.0f, 0.0f, 1.1f, 0.333f, size, currentminikit + 1, packet, 0.0f);
            const i32 count = packet->new_minikits < 1 ? Game.area_save[packet->area->index].field_0x5[0]
                              : currentminikit < 0     ? 0
                                                       : currentminikit;
            DrawMiniKitCount(1.0f, 1.0f, count, packet->minikit_max);
            title_alpha = 1.0f;
            break;
        }
        case 3: {
            f32 blend = 0.0f;
            if (stage->field_0x1c - jibberlen < stage->field_0x18)
                blend = (stage->field_0x18 - (stage->field_0x1c - jibberlen)) / jibberlen;
            DrawStatusMiniKit(0.0f, 0.0f, 1.1f, 0.333f, 0.333f, currentminikit + 1, packet, blend);
            i32 count = packet->new_minikits < 1 ? Game.area_save[packet->area->index].field_0x5[0] : currentminikit;
            if (blend >= 0.5f)
                ++count;
            if (count < 0)
                count = 0;
            const f32 size =
                (1.0f - fabsf(NuTrigTable[(static_cast<i32>(blend * 32768.0f + 16384.0f) >> 1) & 0x7fff])) * 0.25f +
                1.0f;
            DrawMiniKitCount(1.0f, size, count, packet->minikit_max);
            title_alpha = 1.0f;
            break;
        }
        case 4: {
            title_alpha = 1.0f - stage->field_0x18;
            f32 progress = 0.0f;
            if (stage->field_0x1c != 0.0f && stage->field_0x18 != 0.0f)
                progress = stage->field_0x18 / stage->field_0x1c;
            const f32 blend =
                1.0f - (NuTrigTable[(static_cast<i32>(progress * 32768.0f + 16384.0f) >> 1) & 0x7fff] + 1.0f) * 0.5f;
            DrawStatusMiniKit(blend * -0.6f + 0.0f, blend * -0.5f + 0.0f, 1.1f, blend * -0.183f + 0.333f, 1.0f,
                              Game.area_save[packet->area->index].field_0x5[0], packet, 0.0f);
            const f32 off = stage->field_0x18 < 1.0f ? 1.0f - stage->field_0x18 : 0.0f;
            DrawMiniKitCount(1.0f - (NuTrigTable[(static_cast<i32>(off * 32768.0f + 16384.0f) >> 1) & 0x7fff] + 1.0f) *
                                        0.5f,
                             1.0f, Game.area_save[packet->area->index].field_0x5[0], packet->minikit_max);
            break;
        }
        case 6: {
            title_alpha = 1.0f - stage->field_0x18;
            i32 angle = 0x2000;
            if (stage->field_0x18 < 1.0f)
                angle = (static_cast<i32>(title_alpha * 32768.0f + 16384.0f) >> 1) & 0x7fff;
            if (stage->field_0x18 < stage->field_0x1c) {
                DrawMiniKitCount(1.0f - (NuTrigTable[angle] + 1.0f) * 0.5f, 1.0f,
                                 Game.area_save[packet->area->index].field_0x5[0], packet->minikit_max);
                DrawStatusMiniKit(0.0f, 0.0f, 1.1f, 0.333f, 1.0f, Game.area_save[packet->area->index].field_0x5[0],
                                  packet, 0.0f);
            }
            break;
        }
        default:
            title_alpha = 1.0f;
            break;
    }
    title_alpha = title_alpha < 0.0f ? 0.0f : title_alpha > 1.0f ? 1.0f : title_alpha;
    Text3DEx(TTab[tMINIKIT], 0.0f, STATUS_TITLE_Y, 1.0f, 0.5f, 0.5f, 0.5f, 0, 255, 255, 255,
             static_cast<i32>(title_alpha * 128.0f));
}

void MiniKit_LSW_Skip(STATUS_STAGE_s *stage, STATUSPACKET_s *packet) {
    currentminikit += newminikitcount;
    if ((packet->field_0xb0 & 0x10) != 0 && (stage->field_0x14 != 6 || stage->field_0x18 < stage->field_0x1c))
        IncreaseScore(packet->score, 50000, 0);
    NextStatusStage(packet);
}

i32 UpdateNewMiniKits(STATUSPACKET_s *, STATUS_STAGE_s *stage) {
    if (stage->field_0x18 > stage->field_0x1c) {
        stage->field_0x18 = 0.0f;
        ++currentminikit;
        pop_timer = 0.5f;
        slideseek = 1.0f;
        return 1;
    }
    if ((stage->field_0x1c - jibberlen) - slidetime < stage->field_0x18 &&
        stage->field_0x18 <= stage->field_0x1c - jibberlen) {
        slideseek = ((stage->field_0x1c - stage->field_0x18) - jibberlen) / slidetime;
    }
    return 0;
}

void CollectAllMiniKits(AREASAVE_s *save) {
    if (save == NULL)
        return;
    for (i32 i = 0; i < AREACOUNT; ++i) {
        if ((ADataList[i].flags & AREAFLAG_MINIKIT) != 0 && save[i].complete != SAVE_INCOMPLETE) {
            save[i].minikit_count = 10;
            save[i].minikit_complete = SAVE_COMPLETE;
        }
    }
}

void MiniKit_LSW_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    switch (stage->field_0x14) {
        case 0:
            newminikitcount = packet->new_minikits;
            currentminikit = Game.area_save[packet->area->index].field_0x5[0];
            if (newminikitcount > 0)
                currentminikit -= newminikitcount;
            stage->field_0x18 = 0.0f;
            stage->field_0x1c = 1.0f;
            stage->field_0x14 = 1;
            break;
        case 1:
            stage->field_0x18 += elapsed;
            if (stage->field_0x18 >= stage->field_0x1c) {
                stage->field_0x18 = 0.0f;
                if (newminikitcount == 0) {
                    PlaySfx(const_cast<char *>("TrueJedi_NOT"), NULL);
                    stage->field_0x1c = 1.0f;
                    stage->field_0x14 = 4;
                } else {
                    stage->field_0x14 = 2;
                    stage->field_0x1c = 0.5f;
                    slideseek = 1.0f;
                }
            }
            break;
        case 2:
            stage->field_0x18 += elapsed;
            if (stage->field_0x18 >= stage->field_0x1c) {
                stage->field_0x18 = 0.0f;
                stage->field_0x1c = 0.4f;
                stage->field_0x14 = 3;
                PlaySfx(const_cast<char *>("Jp_Ana_Jump"), NULL);
            }
            break;
        case 3:
            if (newminikitcount < 1) {
                stage->field_0x18 = 0.0f;
                stage->field_0x1c = 3.0f;
                stage->field_0x14 = 4;
            } else {
                const f32 previous = stage->field_0x18;
                stage->field_0x18 += elapsed;
                if (previous < stage->field_0x1c - jibberlen && stage->field_0x18 >= stage->field_0x1c - jibberlen) {
                    NewStatusRumbleBuzz(-1, 0.0f, 0.1f, 0);
                    PlaySfx(const_cast<char *>("MK-Panel"), NULL);
                }
                if (UpdateNewMiniKits(packet, stage) == 1) {
                    stage->field_0x18 = 0.0f;
                    if (currentminikit < packet->minikit_count) {
                        stage->field_0x1c = 0.5f;
                        stage->field_0x14 = 2;
                        return;
                    }
                    stage->field_0x1c = 1.0f;
                    stage->field_0x14 = 4;
                } else if (stage->field_0x14 != 4)
                    return;
            }
            if ((packet->field_0xb0 & 0x10) == 0)
                PlaySfx(const_cast<char *>("TrueJedi_NOT"), NULL);
            else {
                stage->field_0x14 = 6;
                stage->field_0x1c = 1.0f;
            }
            break;
        case 4:
            stage->field_0x18 += elapsed;
            if (stage->field_0x18 >= stage->field_0x1c)
                NextStatusStage(packet);
            break;
        case 6: {
            SetDrawGoldBrick(packet, packet->current_gold_brick);
            const f32 previous = stage->field_0x18;
            stage->field_0x18 += elapsed;
            if (stage->field_0x18 < stage->field_0x1c) {
                if (static_cast<u32>(GameTimer.update_count) % 6 < 3 && (GameTimer.update_count - 1U) % 6 > 2)
                    NewStatusRumbleBuzz(-1, 0.0f, 0.0f, 2);
            } else if (previous < stage->field_0x1c) {
                AddStatusMiniKitParts();
                NewStatusRumbleBuzz(-1, 1.0f, 0.1f, 0);
                PlaySfx(const_cast<char *>("Explode1"), NULL);
            } else if (FindGameMsgsWithID(1, 0, -1, NULL) == 0) {
                PlaySfx(const_cast<char *>("Shop_BuyCheat"), NULL);
                NextStatusStage(packet);
            }
            break;
        }
    }
}

void MiniKit_GameMsg_End(GAMEMESSAGE_s *) {
}

void ResetMinikitCounter() {
    minikitCounter_C = 0;
    minikitCounter_A = 0;
}

extern i16 id_SLAVE1, tALLMINIKITSBUILT;
extern i32 STATUS_R, STATUS_G, STATUS_B;

void AllMiniKits_LSW_Draw(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, i32 current) {
    char text[60];
    if (current == 0) {
        if (stage->field_0x12 == 0)
            return;
        const f32 alpha = getFinishedStatusAlpha(packet);
        const i32 opacity = static_cast<i32>(alpha * 128.0f);
        i32 angle = 0x2000;
        if (GameTimer.time_elapsed_mod_seconds <= 0.25f)
            angle = (static_cast<i32>(GameTimer.time_elapsed_mod_seconds * 32768.0f + 16384.0f) >> 1) & 0x7fff;
        if (Game.area_save[packet->area->index].field_0x5[0] == 0) {
            const f32 size = ((1.0f - fabsf(NuTrigTable[angle])) + 1.0f) * 1.2f;
            Text3DEx("?", -0.6f, -0.6f, 1.1f, size, size, size, 0, 255, 255, 255, opacity);
        } else {
            DrawStatusMiniKit(-0.6f, -0.5f, 1.1f,
                              NuTrigTable[(static_cast<i32>(alpha * 16384.0f) >> 1) & 0x7fff] * 0.15f, 1.0f,
                              Game.area_save[packet->area->index].field_0x5[0], packet, 0.0f);
        }
        if (Game.area_save[packet->area->index].field_0x5[0] == packet->minikit_max) {
            Text3DEx("$", -0.6f, -0.7f, 1.0f, 0.8f, 0.8f, 0.8f, 0, 255, 0, 127, opacity);
        } else {
            sprintf(text, "%i/%i", Game.area_save[packet->area->index].field_0x5[0], packet->minikit_max);
            Text3DEx(text, -0.6f, -0.8f, 1.0f, 0.5f, 0.5f, 0.5f, 0, 255, 0, 127, opacity);
        }
        return;
    }
    if (stage->field_0x14 < 1)
        return;
    const f32 time = stage->field_0x18;
    f32 title_alpha;
    f32 icon_alpha = 0.0f;
    f32 y = 0.225f;
    if (time < 0.5f)
        title_alpha = time + time;
    else if (time < 3.5f)
        title_alpha = 1.0f;
    else if (time < 4.0f) {
        icon_alpha = (time - 3.5f) + (time - 3.5f);
        title_alpha = 1.0f - icon_alpha;
        y = (1.0f - NuTrigTable[(static_cast<i32>(icon_alpha * 16384.0f) >> 1) & 0x7fff]) * -0.5f + 0.225f;
    } else if (time < 6.0f) {
        title_alpha = 0.0f;
        icon_alpha = 1.0f;
        y = (1.0f - NuTrigTable[0x2000]) * -0.5f + 0.225f;
    } else if (time < 6.5f) {
        title_alpha = 0.0f;
        icon_alpha = 1.0f - ((time - 6.0f) + (time - 6.0f));
        if (icon_alpha <= 0.0f)
            return;
    } else
        title_alpha = 0.0f;
    if (icon_alpha > 0.0f) {
        DrawCharIcon(id_SLAVE1, 0.0f, y, 0.0f, 0.4f, 0xa7, icon_alpha, icon_alpha, 1, NULL);
        SmartTextEx(TTab[CDataList[id_SLAVE1].name_id], 0.0f, -0.1f, 1.0f, 0.6f, 0.6f, 0.6f, 0, STATUS_R, STATUS_G,
                    STATUS_B, 1.7f, 1, NULL, 0, static_cast<i32>(icon_alpha * 128.0f));
    }
    if (title_alpha > 0.0f) {
        Text3DEx(TTab[tALLMINIKITSBUILT], 0.0f, 0.225f, 1.0f, 0.7f, 0.7f, 0.7f, 0, STATUS_R, STATUS_G, STATUS_B,
                 static_cast<i32>(title_alpha * 128.0f));
    }
}

void AllMiniKits_LSW_Skip(STATUS_STAGE_s *, STATUSPACKET_s *packet) {
    NextStatusStage(packet);
}

void SpecialMiniKits_Draw(WORLDINFO_s *) {
}

void AddStatusMiniKitParts() {
}

void AllMiniKits_LSW_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    if (stage->field_0x14 == 0) {
        stage->field_0x18 = 0.0f;
        stage->field_0x1c = 6.5f;
        stage->field_0x14 = 1;
    } else if (stage->field_0x14 == 1) {
        const f32 previous = stage->field_0x18;
        stage->field_0x18 += elapsed;
        if (stage->field_0x18 >= stage->field_0x1c)
            NextStatusStage(packet);
        else if (previous < 0.5f && stage->field_0x18 >= 0.5f) {
            PlaySfx(const_cast<char *>("StatusAward"), NULL);
            NewStatusRumbleBuzz(-1, 0.6f, 0.0f, 0);
        } else if (previous < 4.0f && stage->field_0x18 >= 4.0f)
            PlaySfx(const_cast<char *>("Char_Icon_App"), NULL);
    }
}

void CharacterMiniKits_Dump(WORLDINFO_s *) {
}

void MiniKit_GameMsg_Update(GAMEMESSAGE_s *) {
}

void SetEffectVisibility(char *, i32);

void EffectOffProgress_Reset(LEVEL_PROGRESS_s *progress) {
    if (progress == NULL)
        return;
    for (i32 i = 0; i < 12; ++i) {
        if (progress->disabled_effect_names[i][0] != '\0')
            SetEffectVisibility(progress->disabled_effect_names[i], 0);
    }
}

void IncrementMinikitCounter(GameObject_s *) {
}

i32 EffectOffProgress_Update(LEVEL_PROGRESS_s *progress, char *name, i32 visible) {
    if (name == NULL || progress == NULL || NuStrLen(name) > 15)
        return 0;
    for (i32 i = 0; i < 12; ++i) {
        if (NuStrICmp(progress->disabled_effect_names[i], name) == 0) {
            if (visible != 0) {
                progress->disabled_effect_names[i][0] = '\0';
                return 2;
            }
            return 3;
        }
    }
    if (visible != 0)
        return 0;
    for (i32 i = 0; i < 12; ++i) {
        if (progress->disabled_effect_names[i][0] == '\0') {
            NuStrCpy(progress->disabled_effect_names[i], name);
            break;
        }
    }
    return 1;
}

void SpecialMiniKits_Configure(WORLDINFO_s *world, char *config) {
    (void)world;
    (void)config;
}
