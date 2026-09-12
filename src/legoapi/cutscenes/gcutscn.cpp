#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nugcutscene.h"

#include "globals.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/levels/episode.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/world/world.h"

#include <string.h>
#include <stdio.h>
#include "nu2api/nufile/nufpar.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/render/core/render.h"
#include "gameapi/gui/apimenu.h"

i32 (*CutScenePlayer_AcceptFn)(CUTSCENEPLAYERCLIP *) = NULL;
extern i32 NextArea_FreePlay;
extern i16 tCHAPTER;
extern f32 ICONSIZE;
void NewLevelFromMenu(LEVELDATA_s *, i32, i32, i32);

CUTSCENEPLAYER_s *CutScenePlayer = NULL;

void CutScenePlayer_Reset() {
    if (CutScenePlayer != NULL) {
        CutScenePlayer->active = 0;
    }
}

void CutScenePlayer_Start(i32 clip_id, i32 door) {
    if (CutScenePlayer != NULL && clip_id >= 0 && clip_id < CutScenePlayer->clip_count) {
        NewLevelFromMenu(&LDataList[CutScenePlayer->clips[clip_id].level_id], -1, -1, 1);
        CutScenePlayer->active = &CutScenePlayer->clips[clip_id];
        NextArea_FreePlay = 0;
        FreePlay = 0;
        CutScenePlayer->return_door = door;
    }
}

CUTSCENEPLAYERCLIP *CutScenePlayer_Active() {
    return CutScenePlayer != NULL ? CutScenePlayer->active : 0;
}

void CutScenePlayer_GetText(i32 id, char *title, char *subtitle, i32 mode) {
    if (CutScenePlayer == NULL) {
        if (title != NULL)
            NuStrCpy(title, "?");
        if (subtitle != NULL)
            NuStrCpy(subtitle, "?");
        return;
    }
    CUTSCENEPLAYERCLIP *clip = &CutScenePlayer->clips[id];
    if (title != NULL) {
        if (mode <= 1) {
            sprintf(title, CutScenePlayer->clip_text != NULL ? TTab[*CutScenePlayer->clip_text] : "Clip %i", id + 1);
        } else
            title[0] = 0;
        if (mode != 0) {
            char *type_text = NULL;
            switch (clip->type) {
                case CLIP_INTRO:
                    if (CutScenePlayer->intro_text)
                        type_text = TTab[*CutScenePlayer->intro_text];
                    break;
                case CLIP_MIDTRO:
                    if (CutScenePlayer->midtro_text)
                        type_text = TTab[*CutScenePlayer->midtro_text];
                    break;
                case CLIP_OUTRO:
                    if (CutScenePlayer->outro_text)
                        type_text = TTab[*CutScenePlayer->outro_text];
                    break;
                case CLIP_ENDING:
                    if (CutScenePlayer->ending_text)
                        type_text = TTab[*CutScenePlayer->ending_text];
                    break;
            }
            if (mode <= 1)
                NuStrCat(title, " - ");
            else {
                title[0] = 0;
                i32 area = LDataList[clip->level_id].area_index;
                if (mode != 2 && area != -1 && !(ADataList[area].flags & 2) &&
                    static_cast<i8>(ADataList[area].area_index) != -1) {
                    sprintf(title, "%s %i", TTab[tCHAPTER], static_cast<i8>(ADataList[area].area_index) + 1);
                    NuStrCat(title, " - ");
                }
            }
            NuStrCat(title, type_text);
        }
    }
    if (subtitle != NULL) {
        LEVELDATA_s *level = &LDataList[clip->level_id];
        if (level->area_index != -1) {
            AREADATA_s *area = &ADataList[level->area_index];
            if (!(area->flags & 2)) {
                NuStrCpy(subtitle, TTab[area->name_id]);
                return;
            }
            if (level->episode_index != -1) {
                NuStrCpy(subtitle, TTab[EDataList[level->episode_index].text_id]);
                return;
            }
        }
        NuStrCpy(subtitle, "?");
    }
}

i32 CutScenePlayer_CanStart(i32 clip_id) {
    if (CutScenePlayer != NULL && clip_id >= 0 && clip_id < CutScenePlayer->clip_count) {
        const i32 area_id = static_cast<i8>(LDataList[CutScenePlayer->clips[clip_id].level_id].area_index);
        if (area_id != -1) {
            AREADATA_s *area = &ADataList[area_id];
            if ((area->flags & 2) == 0) {
                if (Game_AreaSave != NULL && Game_AreaSave[area_id].area_complete != 0)
                    return 1;
            } else {
                const i32 episode = static_cast<i8>(area->episode_index);
                if (episode != -1 && Episode_IsComplete(&EDataList[episode], NULL))
                    return 2;
            }
        }
    }
    return 0;
}

void CutScenePlayer_DrawGrid(COLLECTION_s *collection, i16 *ids, float x, float y, i32 selected, float alpha) {
    if (FadeSys.fade > 0.0f || !WORLD->lev_objs[167].active)
        return;
    if (alpha > 1.0f)
        alpha = 1.0f;
    const i32 rows = collection->count_y / collection->count_x;
    const f32 scale = ICONSIZE * 1.2f;
    NUVEC minimum, maximum;
    NuSpecialGetBounds(&WORLD->lev_objs[167].special, &minimum, &maximum);
    const f32 dx = (maximum.x - minimum.x) * scale / PANEL3DMULX;
    const f32 dy = -(maximum.y - minimum.y) * scale / PANEL3DMULY;
    y -= rows * dy * 0.5f;
    const f32 first_x = x - (collection->count_x - 1) * dx * 0.5f;
    MENU *menu = &GameMenu[GameMenuLevel];
    i32 previous_area = -1, colour = -1;
    static const u8 colours[8][3] = {{255, 63, 0},  {255, 127, 0},  {255, 223, 0},  {0, 255, 63},
                                     {0, 127, 255}, {191, 31, 255}, {255, 31, 191}, {255, 255, 255}};
    for (i32 row = 0; row <= rows; ++row, y += dy) {
        x = first_x;
        for (i32 column = 0; column < collection->count_x; ++column, x += dx) {
            const i32 i = row * collection->count_x + column;
            if (i >= collection->count_y)
                continue;
            collection->list[i].grid_x = x;
            collection->list[i].grid_y = y;
            if (alpha <= 0.0f)
                continue;
            i32 object = i == selected ? (menu_flash ? 168 : 166) : 167;
            f32 opacity = CutScenePlayer_CanStart(ids[i]) ? alpha : alpha * 0.375f;
            DrawPanel3DObject(x, y, 1.0f, scale, scale, scale, 0, 0, 0, &WORLD->lev_objs[object].special, 0, opacity);
            menu->item_x[i] = x;
            menu->item_y[i] = y;
            menu->item_width[i] = scale * 0.5f;
            menu->item_height[i] = 0.0f;
            char text[32];
            sprintf(text, "%i", i + 1);
            i32 area = LDataList[CutScenePlayer->clips[ids[i]].level_id].area_index;
            if (area != previous_area) {
                previous_area = area;
                if (++colour > 7)
                    colour = 7;
            }
            const f32 text_scale = scale * 3.5f;
            Text3DEx(text, x, y, 1.0f, text_scale * 0.925f, text_scale, text_scale, 0, colours[colour][0],
                     colours[colour][1], colours[colour][2], static_cast<u8>(opacity * 255.0f));
        }
    }
}

void CutScenePlayer_Configure(char *file, VARIPTR *buffer, VARIPTR *, i16 *clip_text, i16 *intro, i16 *midtro,
                              i16 *outro, i16 *ending) {
    NUFPAR *fp = NuFParCreate(file);
    if (fp == NULL)
        return;
    CUTSCENEPLAYER_s player;
    buffer->addr = (buffer->addr + 3) & ~3U;
    player.clips = reinterpret_cast<CUTSCENEPLAYERCLIP *>(buffer->addr);
    player.active = NULL;
    player.clip_count = 0;
    player.clip_text = clip_text;
    player.intro_text = intro;
    player.midtro_text = midtro;
    player.outro_text = outro;
    player.ending_text = ending;
    while (NuFParGetLine(fp)) {
        if (!NuFParGetWord(fp) || NuStrICmp(fp->word_buf, "clip"))
            continue;
        CUTSCENEPLAYERCLIP *clip = &player.clips[player.clip_count];
        clip->level_id = -1;
        clip->type = CLIP_INTRO;
        clip->name[0] = 0;
        clip->guest_episode = -1;
        while (NuFParGetWord(fp)) {
            if (NuStrICmp(fp->word_buf, "in_level") == 0) {
                i32 level;
                if (NuFParGetWord(fp) && Level_FindByName(fp->word_buf, &level))
                    clip->level_id = level;
            } else if (NuStrICmp(fp->word_buf, "name") == 0) {
                if (NuFParGetWord(fp) && NuStrLen(fp->word_buf) <= 63)
                    NuStrCpy(clip->name, fp->word_buf);
            } else if (NuStrICmp(fp->word_buf, "intro") == 0)
                clip->type = CLIP_INTRO;
            else if (NuStrICmp(fp->word_buf, "midtro") == 0)
                clip->type = CLIP_MIDTRO;
            else if (NuStrICmp(fp->word_buf, "outro") == 0)
                clip->type = CLIP_OUTRO;
            else if (NuStrICmp(fp->word_buf, "ending") == 0)
                clip->type = CLIP_ENDING;
            else if (NuStrICmp(fp->word_buf, "guest_in_episode") == 0 && NuFParGetWord(fp))
                clip->guest_episode = NuAToI(fp->word_buf);
        }
        if (clip->level_id != -1 && (CutScenePlayer_AcceptFn == NULL || CutScenePlayer_AcceptFn(clip)))
            ++player.clip_count;
    }
    NuFParDestroy(fp);
    if (player.clip_count != 0) {
        buffer->void_ptr = &player.clips[player.clip_count];
        memmove(buffer->void_ptr, &player, sizeof(player));
        CutScenePlayer = reinterpret_cast<CUTSCENEPLAYER_s *>(buffer->void_ptr);
        buffer->addr += sizeof(player);
    }
}

void CutScenePlayer_SetObjects(CUTINFO *cut) {
    if (CutScenePlayer_Active() != NULL && cut->state_entries != NULL) {
        for (i32 i = 0; i < cut->state_count; ++i) {
            CUTSCENEPLAYEROBJ *entry = &cut->state_entries[i];
            const u8 flags = entry->flags;
            if (flags & CLIP_OBJECT_SHOW)
                NuSpecialSetVisibility(&entry->special, 1);
            else if (flags & CLIP_OBJECT_HIDE)
                NuSpecialSetVisibility(&entry->special, 0);
            else if (flags & CLIP_OBJECT_ANIM_END) {
                f32 end = NuSpecialGetAnimEndFrame(&entry->special);
                nuinstanim_s *anim = NuSpecialGetInstAnim(&cut->state_entries[i].special);
                if (anim != NULL) {
                    anim->ltime = end;
                    anim->playing = 1;
                }
            }
        }
    }
}

i32 CutScenePlayer_CountEpisodeClips(i32 episode, i32 include_guests, i16 *ids) {
    i32 count = 0;
    if (CutScenePlayer != NULL) {
        for (i32 i = 0; i < CutScenePlayer->clip_count; ++i) {
            CUTSCENEPLAYERCLIP *clip = &CutScenePlayer->clips[i];
            if (LDataList[clip->level_id].episode_index == episode ||
                (include_guests != 0 && clip->guest_episode == episode)) {
                if (ids != NULL)
                    ids[count] = i;
                ++count;
            }
        }
    }
    return count;
}

void FindGameCutScenes() {
    memset(&game_cutscenes, 0, sizeof(game_cutscenes));

    game_cutscenes.podrace_pod_explode = CutScene_Find(WORLD->cutscene_sys, "ep1_podrace_podexplode");
    game_cutscenes.podrace_out_of_time = CutScene_Find(WORLD->cutscene_sys, "ep1_podrace_outoftime");
    game_cutscenes.bonus_gunship_cavalry_explode =
        CutScene_Find(WORLD->cutscene_sys, "ep2_bonus_gunshipcavalry_explode");
    game_cutscenes.droid_factory_conveyor = CutScene_Find(WORLD->cutscene_sys, "ep2_droidfactory_conveyor");
    game_cutscenes.podrace_avalanche = CutScene_Find(WORLD->cutscene_sys, "ep1_podrace_avalanche");
    game_cutscenes.dogfight_die = CutScene_Find(WORLD->cutscene_sys, "ep3_dogfight_die");
    game_cutscenes.podrace_sebulba = CutScene_Find(WORLD->cutscene_sys, "ep1_podrace_sebulba");
    game_cutscenes.cutscene = CutScene_Find(WORLD->cutscene_sys, "ep1_podsprint_avalanche");
    game_cutscenes.podsprint_out_of_time = CutScene_Find(WORLD->cutscene_sys, "ep1_podsprint_outoftime");
    game_cutscenes.podsprint_sebulba = CutScene_Find(WORLD->cutscene_sys, "ep1_podsprint_sebulba");
}

void FindSceneStateObj(nugscn_s *, SCENEPROGRESS_s *, nuhspecial_s *) {
}

void instGetLookAtLocatorInfo(instNUGCUTSCENE_s *, instNUGCUTLOOKAT_s *) {
}

void instNuGCutGetNextRigidInfo(instNUGCUTSCENE_s *, float, i32, numtx_s *, nuhspecial_s *) {
}

i32 instNuGCutSceneSwapBuffers(instNUGCUTSCENE_s *instance, i32 force) {
    if (static_cast<i8>(instance->flags_8c) >= 0 || force != 0) {
        if (instance->pending_stream_buffer == NULL) {
            u8 flags = instance->flags_8b;
            if ((flags & 0x10) != 0) {
                flags &= ~0x10U;
                instance->flags_8b = flags;
                instance->pending_stream_buffer = instance->stream_buffer_1;
            } else {
                flags |= 0x10;
                instance->flags_8b = flags;
                instance->pending_stream_buffer = instance->stream_buffer_0;
            }
            return 1;
        }
    }
    return 0;
}

void instNuGCutSceneResetCamLock(instNUGCUTSCENE_s *instance) {
    if (instance != NULL && instance->camera_instance != NULL && instance->camera_instance->camera_index >= 0) {
        CutSceneCameraCTRL = 0;
    }
}

void instNuGCutSceneEndButNotSystems(instNUGCUTSCENE_s *instance) {
    instance->flags_88 &= ~2U;
    instance->current_frame = instance->cutscene->duration;
    instance->flags_89 |= 0x10;
    instance->render_frame = instance->cutscene->duration;
    instance->flags_8c &= ~0x40U;
    instNuGCutSceneResetCamLock(instance);
}

void instNuGCutContainsInstancedRigids(instNUGCUTSCENE_s *) {
}
