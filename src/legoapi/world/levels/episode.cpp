#include "legoapi/world/levels/episode.h"

#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/render/core/render.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/mission.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/numath/nutrig.h"

EPISODEDATA *EDataList = NULL;

extern TerrainQuery_s *TerI;
extern u8 TerrainHitInfo[4];
extern f32 text3d_width;

void Text_MakeScore(u32 score, char *text);
void DrawSuperStoryTime(f32 x, f32 timer, f32 target, i32 flags, i32 draw_icon);

u32 Episode_FindAreaFromFlags(EPISODEDATA *ep, u32 flags, u32 want) {
    for (i32 i = 0; i < (i32)ep->area_count; i++) {
        AREADATA *a = &ADataList[ep->area_ids[i]];
        if ((a->flags & flags) == want) {
            return (u8)a->index;
        }
    }
    return 0xffffffff;
}
EPISODEDATA *Episodes_ConfigureList(char *file, VARIPTR *bufferStart, VARIPTR *bufferEnd, i32 maxCount,
                                    i32 *countDest) {
    NUFPAR *fp = NuFParCreate(file);
    if (fp == NULL) {
        if (countDest != NULL) {
            *countDest = 0;
        }
        return NULL;
    }

    i32 count = 0;
    bool bVar3 = false;
    EPISODEDATA *episodePtr = (EPISODEDATA *)ALIGN((usize)bufferStart->void_ptr, 4);
    bufferStart->void_ptr = (void *)episodePtr;
    EPISODEDATA *episode = episodePtr;

    while (NuFParGetLine(fp)) {
    get_word:
        NuFParGetWord(fp);
        char *a = fp->word_buf;
        if (*a == '\0') {
            continue;
        }

        if (!bVar3) {
            if (NuStrICmp(a, "episode_start") == 0 && count < maxCount) {
                episode->name_id = -1;
                episode->text_id = -1;
                episode->area_count = 0;
                episode->index = (u8)count;
                bVar3 = true;
            }
            continue;
        }

        if (NuStrICmp(a, "episode_end") == 0) {
            bVar3 = false;
            if (episode->area_count != 0) {
                count++;
                episode++;
                if (NuFParGetLine(fp) == 0) {
                    break;
                }
                goto get_word;
            }
            continue;
        }

        if (NuStrICmp(a, "area") == 0) {
            if (episode->area_count <= 9 && NuFParGetWord(fp) != 0) {
                i32 areaIndex;
                AREADATA *area = Area_FindByName(fp->word_buf, &areaIndex);
                bVar3 = true;
                if (areaIndex != -1) {
                    u32 areaCount = episode->area_count;
                    bool found = false;
                    if (areaCount != 0) {
                        if (episode->area_ids[0] == areaIndex) {
                            found = true;
                        } else {
                            for (i32 k = 1; k < (i32)areaCount; k++) {
                                if (episode->area_ids[k] == areaIndex) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (!found) {
                        for (i32 j = 0; j < count; j++) {
                            EPISODEDATA *prev = &episodePtr[j];
                            for (i32 byteOff = 0; byteOff < (i32)prev->area_count * 2; byteOff += 2) {
                                if (*(i16 *)((u8 *)prev->area_ids + byteOff) == areaIndex) {
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                break;
                            }
                        }
                    }
                    if (!found) {
                        episode->area_ids[areaCount] = (i16)areaIndex;
                        episode->area_count = (u8)(areaCount + 1);
                        if ((area->flags & (AREAFLAG_ENDING_AREA | AREAFLAG_BONUS_AREA)) == 0) {
                            episode->regular_areas += 1;
                        }
                    }
                }
            } else {
                bVar3 = true;
            }
            continue;
        }

        if (NuStrICmp(a, "name_id") == 0) {
            episode->name_id = (i16)NuFParGetInt(fp);
            bVar3 = true;
            continue;
        }

        if (NuStrICmp(a, "text_id") == 0) {
            bVar3 = true;
            episode->text_id = (i16)NuFParGetInt(fp);
            continue;
        }

        bVar3 = true;
        continue;
    }

    NuFParDestroy(fp);
    if (count != 0) {
        bufferStart->void_ptr = (void *)episode;
        if (countDest != NULL) {
            *countDest = count;
        }
        return episodePtr;
    }
    return NULL;
}

i32 Episode_ContainsArea(i32 areaId, i32 *areaIndex) {
    for (i32 i = 0; i < EPISODECOUNT; i++) {
        EPISODEDATA *episode = &EDataList[i];

        for (i32 j = 0; j < episode->area_count; j++) {
            i16 id = episode->area_ids[j];
            if (id == areaId) {
                if (areaIndex != NULL) {
                    *areaIndex = j;
                }

                return i;
            }
        }
    }

    if (areaIndex != NULL) {
        *areaIndex = -1;
    }

    return -1;
}

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

// Episode-global logic and helpers shared across episodes: completion/
// episode bookkeeping, super-story, boss/cutscene-adjacent helpers. The
// per-episode level *handlers* live in the matching episodeI..VI.cpp files.

// ===========================================================================
// Episode bookkeeping
// ===========================================================================

void BossKilled(i32) {
}

void CountOpenEpisodes() {
}

i32 Episode_IsComplete(EPISODEDATA *episode, i32 *completed_area_count) {
    if (Game_AreaSave == NULL) {
        return 0;
    }

    i32 complete = 0;
    for (i32 i = 0; i < episode->regular_areas; i++) {
        if (Game_AreaSave[episode->area_ids[i]].area_complete != 0) {
            complete++;
        }
    }

    if (completed_area_count != NULL) {
        *completed_area_count = complete;
    }
    return complete == episode->regular_areas;
}

i32 Episodes_Completed() {
    i32 completed = 0;
    for (i32 i = 0; i < EPISODECOUNT; ++i) {
        if (Episode_IsComplete(&EDataList[i], NULL) != 0) {
            ++completed;
        }
    }
    return completed;
}

void Episodes_CompleteAllSuperStories() {
}

void Episode_FindFromArea(i32) {
}

i32 EpCompleteTotal, EpCompleteCount;
i32 EpMiniKitTotal, EpMiniKitCount;
i32 EpCharKitTotal, EpCharKitCount;
i32 EpBuildUpTotal, EpBuildUpCount;
i32 EpStoryBuildUpTotal, EpStoryBuildUpCount;
i32 EpFreePlayBuildUpTotal, EpFreePlayBuildUpCount;
i32 EpRedBrickTotal, EpRedBrickCount;
i32 EpGoldBrickTotal, EpGoldBrickCount;

i32 Episode_CountOpenAreas(i32 episode_index, i32 area_index, AREASAVE_s *saves) {
    EpCompleteTotal = EpCompleteCount = 0;
    EpMiniKitTotal = EpMiniKitCount = 0;
    EpCharKitTotal = EpCharKitCount = 0;
    EpBuildUpTotal = EpBuildUpCount = 0;
    EpStoryBuildUpTotal = EpStoryBuildUpCount = 0;
    EpFreePlayBuildUpTotal = EpFreePlayBuildUpCount = 0;
    EpRedBrickTotal = EpRedBrickCount = 0;
    EpGoldBrickTotal = EpGoldBrickCount = 0;
    if (episode_index == -1)
        return 0;
    if (GOLDBRICKFORSUPERSTORY != 0 && area_index == -1) {
        EpGoldBrickTotal = 1;
        if (Game_EpisodeSave != NULL && (Game_EpisodeSave[episode_index].flags & 0xff) != 0)
            EpGoldBrickCount = 1;
    }
    i32 open = 0;
    for (i32 i = 0; i < EDataList[episode_index].area_count; ++i) {
        const i32 id = EDataList[episode_index].area_ids[i];
        if (id != area_index && area_index != -1)
            continue;
        AREADATA *area = &ADataList[id];
        if (area == HUB_ADATA || (area->flags & 0x22) != 0)
            continue;
        if ((area->flags & 0x100) != 0) {
            if (GOLDBRICKFORSUPERBONUS != 0) {
                ++EpGoldBrickTotal;
                if (saves[i].area_complete != 0)
                    ++EpGoldBrickCount;
            }
            continue;
        }
        if ((area->flags & 4) != 0)
            continue;
        AREASAVE_s *save = &saves[id];
        ++EpCompleteTotal;
        EpMiniKitTotal += 10;
        if (save->complete != 0)
            ++open;
        if (save->area_complete != 0) {
            ++EpCompleteCount;
            ++EpGoldBrickCount;
        }
        ++EpGoldBrickTotal;
        if ((area->flags & 0x10) == 0)
            continue;
        if (save->minikit_complete != 0)
            ++EpGoldBrickCount;
        EpMiniKitCount += save->field_0x5[0];
        if (BOTHTRUEJEDIGOLDBRICKS == 0) {
            ++EpBuildUpTotal;
            EpGoldBrickTotal += 2;
            if (save->story_buildup_complete != 0 || save->freeplay_buildup_complete != 0) {
                ++EpGoldBrickCount;
                ++EpBuildUpCount;
            }
        } else {
            if (save->story_buildup_complete != 0) {
                ++EpBuildUpCount;
                ++EpGoldBrickCount;
                ++EpStoryBuildUpCount;
            }
            EpBuildUpTotal += 2;
            EpGoldBrickTotal += 3;
            if (save->freeplay_buildup_complete != 0) {
                ++EpBuildUpCount;
                ++EpGoldBrickCount;
                ++EpFreePlayBuildUpCount;
            }
        }
        ++EpRedBrickTotal;
        if (save->field_0x5[1] != 0)
            ++EpRedBrickCount;
        if (Store_IsPackUnlocked(8)) {
            ++EpCharKitTotal;
            if (GOLDBRICKFORCHALLENGE != 0)
                ++EpGoldBrickTotal;
            if (save->field_0x5[2] != 0) {
                ++EpCharKitCount;
                if (GOLDBRICKFORCHALLENGE != 0)
                    ++EpGoldBrickCount;
            }
        }
    }
    return open;
}

void InitSuperStory(i32) {
}

i32 InStory() {
    if (FreePlay != 0 || ChallengeMode != 0 || Mission_Active(NULL) != NULL || Arcade != 0) {
        return 0;
    }
    return 1;
}

// ===========================================================================
// HUD / score helpers
// ===========================================================================

void CoinTotal_Draw(i32 total, f32 y, f32 scale, i32 remember_positions, f32 icon_phase, i32 red, i32 green, i32 blue) {
    char text[32];
    Text_MakeScore(static_cast<u32>(total), text);

    const f32 score_scale = scale * COINTOTAL_SCORESIZE;
    const i32 alpha = static_cast<u8>(icon_phase * 128.0f);
    Text3DEx(text, 0.0f, y + PANEL_COINADJUSTDY, 1.0f, score_scale, score_scale, score_scale, 0, static_cast<u8>(red),
             static_cast<u8>(green), static_cast<u8>(blue), alpha);

    const i32 phase = static_cast<i32>(icon_phase * 16384.0f);
    const f32 icon_scale = scale * COINTOTAL_COINSIZE * NuTrigTable[(phase >> 1) & 0x7fff];

    LEVEL_OBJECT_RUNTIME *left = &WORLD->lev_objs[cointotal_i_obj[0]];
    if (left->active != 0) {
        const f32 x = text3d_width * -0.5f - scale * COINTOTAL_COINDX;
        if (remember_positions != 0) {
            cointotal_x[0] = x;
        }
        DrawPanel3DObject(x, y, 1.0f, icon_scale, icon_scale, icon_scale, 0, 0, 0, &left->special, 0, 1.0f);
    }

    LEVEL_OBJECT_RUNTIME *right = &WORLD->lev_objs[cointotal_i_obj[1]];
    if (right->active != 0) {
        const f32 x = text3d_width * 0.5f + scale * COINTOTAL_COINDX;
        if (remember_positions != 0) {
            cointotal_x[1] = x;
        }
        DrawPanel3DObject(x, y, 1.0f, icon_scale, icon_scale, icon_scale, 0, 0, 0, &right->special, 0, 1.0f);
    }
}

void DoubleScoreAlpha() {
}

// ===========================================================================
// Shared gameplay helpers
// ===========================================================================

void TrooperShoot(WORLDINFO_s *, minitrooperteam_s *, minisnowtrooper_s *, u16 *, i32) {
}

void NewTerrStoreAnyInfo() {
    TerrainQuery_s *query = TerI;
    TERRAIN_SHAPE *surface = query->surface;
    if (surface == NULL || query->terrain_group_index == -1) {
        return;
    }

    if (surface->material[0] != 0) {
        TerrainHitInfo[0] = surface->material[0];
    }
    if (surface->material[1] != 0) {
        TerrainHitInfo[1] = surface->material[1];
    }
    if (surface->flags != 0) {
        TerrainHitInfo[2] = surface->flags;
    }
    if (surface->normal_flags != 0) {
        TerrainHitInfo[3] = surface->normal_flags;
    }
}

void SetBobaRocketTarget(MechObjectInterface *) {
}

void FireBountyHunterRocket(GameObject_s *) {
}

void ResetTrooperCannons(WORLDINFO_s *, i32) {
}

void UpdateTrooperCannons(WORLDINFO_s *) {
}

void UpdateMiniSnowTroopers(WORLDINFO_s *) {
}

static __used__ void seed_chase(f32 *, i32, abi_long) {
}
