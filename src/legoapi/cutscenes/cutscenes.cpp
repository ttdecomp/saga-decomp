#include "legoapi/legoapi_types.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "globals.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/props/doors/door.h"
#include "legoapi/world/level.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/render/light/shadow.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nucore/nugcutscene.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numusic/numusic.h"
#include "nu2api/nusound/nusound.h"

#include <string.h>
#include "MechInputTouch/MechInputTouch_types.h"
#include "legoapi/world/level.h"
#include <stdio.h>
#include "legoapi/world/mission.h"
#include "legoapi/world/levels/episode.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"

extern i32 ACTIVECUTCOUNT;
extern "C" i32 CUTDRAWWORLD;
extern "C" i32 Paused;
extern i32 CUTCAM;
extern i32 CutSceneWaiting;
extern i32 CUTCAMONLY;
extern i32 cut_waiting_for_new_level;
extern i32 waiting_for_level;
extern i32 newlevel_resumecutaudio;
extern CUTSYS *CS_cutsys;
extern FadeSystem FadeSys;
CUTSCENESYS *CutSceneSys;

static void CutScene_DrawCharacter(instNUGCUTSCENE_s *, NUGCUTSCENE_s *, instNUGCUTCHAR_s *, NUGCUTCHAR_s *, f32, i32);
static void CutScene_EvalCharacter(instNUGCUTSCENE_s *, NUGCUTSCENE_s *, instNUGCUTCHAR_s *, NUGCUTCHAR_s *, f32);
static void CutScene_FindCharacters(NUGCUTSCENE_s *);
static void CutScene_ResetCharacters(instNUGCUTSCENE_s *);
static void CutScene_RigidPostRender(NUGCUTRIGID_s *, instNUGCUTRIGID_s *, NUMTX *);
static void CutScene_CreateCharacterInstance(NUGCUTCHAR_s *, instNUGCUTCHAR_s *, variptr_u *);

extern "C" {
    void instNuGCutSceneEnd(instNUGCUTSCENE_s *instance);
    i32 instNuGCutSceneIsFinished(instNUGCUTSCENE_s *instance);
    void instNuGCutScenePause(instNUGCUTSCENE_s *, u8);
    void instNuGCutSceneReset(instNUGCUTSCENE_s *);
    void instNuGCutSceneStart(instNUGCUTSCENE_s *);
    void instNuGCutSceneStop(instNUGCUTSCENE_s *);
    void instNuGCutSceneDestroy(instNUGCUTSCENE_s *);
    void NuGCutSceneSysRender(i32);
    void NuGCutSceneSysUpdate(i32, i32, f32);
    extern NUGCUTLOCATORFNENTRY_s cutscene_locatorfns[];
}

void SetLevelLights(void *, f32);
void NewLevelFromMenu(LEVELDATA_s *, i32, i32, i32);
void FindAndSetLights(NUVEC *, f32, void *);
void SetZeroLights(void);
void Panel_Clear(void);
void GameFog_Reset(void);
void NeedScreenGrab(i32);
void EnableShadowMapRendering(i32);
void ResetShadowMapRendering(void);
i32 MatrixReflection(NUMTX *, i32, f32, f32, NUMTX *);
f32 FindReflectionNoPlatforms(NUVEC *);
extern "C" i32 NewShadowOnPlatform(void);
void FindAnglesZX(NUVEC *, u16 *, u16 *);
void CharScene_Draw(WORLDINFO_s *, i32, NUMTX *, NUMTX *);
void DrawObjectOnCharacter(WORLDINFO_s *, GameObject_s *, i32, nuhspecial_s *, i32, i32, NUMTX *, i32, u32, NUMTX *,
                           NUVEC *, f32, f32);
i32 qrand(void);
void NewRumbleAllPlayers(f32, f32, i32, i32);
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
extern "C" i32 ShadowInfo(void);
extern "C" TERRAIN_SURFACE_s TerSurface[32];
extern "C" void APITransparentCharDraw(nuhgobj_s *, NUMTX *, i32, i16 *, NUMTX *, void **, i32);
extern "C" void instNuGCutLocatorUpdate(instNUGCUTSCENE_s *, NUGCUTLOCATORSYS_s *, instNUGCUTLOCATOR_s *,
                                        NUGCUTLOCATOR_s *, f32, NUMTX *, i32);
CUTSCENEPLAYERCLIP *CutScenePlayer_Active(void);
void CutScenePlayer_SetObjects(CUTINFO *);
void AddPartDebris(PARTDEBSYS_s *, i32, nuvec_s *);
extern "C" void DebrisSetRenderGroup(i32);
extern AREADATA_s *BONUS_GUNSHIP_ADATA;
extern AREADATA_s *GUNSHIP_ADATA;
extern AREADATA_s *BATTLEOVERCORUSCANT_ADATA;

static NUVEC PodRaceADiePos = {350.0f, 0.25f, -15.0f};
static NUVEC PodRaceBDiePos = {138.0f, 0.15f, 178.0f};
static NUVEC PodRaceCDiePos = {140.0f, -7.2f, 52.0f};
static NUVEC GunshipADiePos = {10.0f, -1.0f, -20.0f};
static NUVEC GunshipBDiePos = {0.0f, -1.0f, -150.0f};
static NUVEC PodAvalanchePeakPos = {-110.0f, -10.0f, -455.0f};
static u8 CutBlobShadowAlpha;
static f32 CutBlobShadowFadeNear;
static f32 CutBlobShadowFadeFar;
static f32 CutReflectRange;
static f32 CutReflectRange2;

extern "C" {
    extern i16 id_ANAKINSPODGREEN;
    extern i16 id_ANAKINSNEWPOD;
    extern i16 id_ANAKINSNEWPODGREEN;
    extern i16 id_REPUBLICGUNSHIP_GREEN;
    extern i16 id_NEW_REPUBLIC_GUNSHIP_GREEN;
    extern i16 id_JEDISTARFIGHTERYELLOWEP3;
}

i32 CutInstEndCount;
static instNUGCUTSCENE_s *CutInstEnd[4];
static i32 CutInstEndStop;
static i32 cutaudiopaused;
static CUTINFO *g_lastCutInfo;
static f32 g_lastCutsceneTime;
static f32 g_accumCutsceneTime;
static f32 CutFrame;
static i32 stream_cut_issued;
i32 NewCutInfoCount;
static CUTINFO *NewCutInfo[8];
u8 cutskip_dontplaylevelintro;
i32 CUTNOFOG;
extern i32 CUTSKIPLOCK;
f32 LevelNameMul;
f32 LevelNameTime;

extern i32 drawcharactermodel_nobsa;
extern "C" i32 NewMode;
void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);

extern "C" {
    void PauseGameAudio(void);
    void PauseGameCut(void);
    void SetLinkedCutSceneMusic(void *context, i32 state);
    void PlaySfxById(i32 sfx_id, nuvec_s *position);
    extern instNUGCUTSCENE_s *cutscene_load_instance;
    void instNuGCutSceneServiceLoad(void);
    void NuGCutSceneSysPostBackgroundLoad(void);
}

static void bgAckStreamCutScene(bgprocinfo_s *) {
    stream_cut_issued = 0;
}

static void bgLoadStreamCutScene(bgprocinfo_s *) {
    if (cutscene_load_instance != NULL) {
        instNuGCutSceneServiceLoad();
    }
}

static i32 CutScene_Start(WORLDINFO_s *world, CUTINFO *cut, i32) {
    instNUGCUTSCENE_s *instance = static_cast<instNUGCUTSCENE_s *>(cut->instance);
    if (cut->music_handle != -1) {
        g_lastCutsceneTime = 0.0f;
        g_accumCutsceneTime = 0.0f;
        g_lastCutInfo = NULL;
    }
    instNuGCutSceneReset(instance);
    if (CutScene_StartFn != NULL) {
        CutScene_StartFn(cut);
    }
    if (CutScenePlayer_Active() != 0) {
        CutScenePlayer_SetObjects(cut);
    }
    instance->rate = cut->frames_per_second * FRAMETIME;
    instNuGCutSceneStart(instance);
    if ((cut->flags & 0x200) != 0) {
        instance->flags_88 |= 8;
    } else {
        instance->flags_88 &= ~8U;
    }
    cut->field_58 = 0.0f;

    if (cut->music_handle != -1) {
        music_man.SelectTrackByHandle(TRACK_CLASS_CUTSCENE, cut->music_handle);
        i32 status = music_man.PlayTrack(TRACK_CLASS_CUTSCENE, 0);
        instance->rate = 0.0f;
        if ((cut->flags & 1) == 0 || status != 1) {
            cutaudiopaused = 0;
        } else {
            CutSceneWaiting = 1;
            PauseGameAudio();
            cutaudiopaused = 1;
        }
    } else {
        PauseGameAudio();
        SetLinkedCutSceneMusic(instance, cut->linked_audio == 0 ? MUSIC_PLAYBACK_DUAL_STREAM
                                                                : MUSIC_PLAYBACK_DUAL_STREAM_PENDING);
        instance->rate = cut->frames_per_second * FRAMETIME;
        if ((cut->flags & 1) != 0) {
            CutSceneWaiting = 0;
            cutaudiopaused = 0;
        }
    }

    if ((cut->flags & 1) == 0) {
        return 1;
    }
    Panel_Clear();
    CutFrame = 0;
    for (i32 i = 0; i < world->cutscene_sys->count; ++i) {
        CUTINFO *other = world->cutscene_sys->cuts[i];
        if (other == NULL || other->instance == NULL) {
            continue;
        }
        instNUGCUTSCENE_s *other_instance = static_cast<instNUGCUTSCENE_s *>(other->instance);
        if ((other_instance->flags_88 & 2) != 0) {
            other_instance->rate = 0.0f;
            if (other == cut || (other->flags & 1) == 0) {
                instNuGCutScenePause(other_instance, 1);
            } else {
                instNuGCutSceneStop(other_instance);
            }
        }
    }
    CUTSTOPGAME = 1;
    CUTNOFOG = cut->flags & 4;
    CUTDRAWWORLD = cut->flags & 2;
    CutStopInfo = cut;
    DebrisSetRenderGroup(cut->debris_render_group);
    LevelNameTime = 0.0f;
    LevelNameMul = 0.0f;
    ACTIVECUTCOUNT = 1;
    CutBorderScale = 1.0f;
    cut_waiting_for_new_level = 0;
    GameFog_Reset();
    for (CUTSCENETEXANIM &animation : cut->texture_animations) {
        if (animation.index != -1) {
            texanimbits &= ~(1U << (animation.index & 0x1f));
        }
    }
    return 1;
}

void CutScenes_End() {
    if (CutScenePlayer_Active() != 0 && NewLData != NULL && NewLData == HUB_LDATA) {
        return;
    }
    i32 i = 0;
    if (CutInstEndCount > 0) {
        do {
            instNuGCutSceneEnd(CutInstEnd[i]);
            if (i == CutInstEndStop) {
                CUTSTOPGAME = 0;
                CutStopInfo = NULL;
                GameCam_Reset(GameCam);
            }
            ++i;
        } while (CutInstEndCount > i);
    }
}

void CutScenes_Draw(WORLDINFO_s *world) {
    if (world->cutscene_sys != NULL && ACTIVECUTCOUNT > 0) {
        SetLevelLights(world->rtl_set, 1.0f);

        CUTINFO *cut = static_cast<CUTINFO *>(CutStopInfo);
        if (cut == NULL) {
            CutBlobShadowAlpha = static_cast<u8>(world->current_level->blob_shadow_alpha);
            CutBlobShadowFadeNear = static_cast<f32>(static_cast<u8>(world->current_level->blob_shadow_fade_near));
            CutBlobShadowFadeFar = static_cast<f32>(static_cast<u8>(world->current_level->blob_shadow_fade_far));
            CutReflectRange = static_cast<f32>(static_cast<u8>(world->current_level->reflection_range));
        } else {
            CutBlobShadowAlpha = cut->blob_shadow_alpha;
            CutBlobShadowFadeNear = static_cast<f32>(cut->blob_shadow_fade_near);
            CutBlobShadowFadeFar = static_cast<f32>(cut->blob_shadow_fade_far);
            CutReflectRange = static_cast<f32>(cut->reflection_range);
        }
        CutReflectRange2 = CutReflectRange * CutReflectRange;
        NuGCutSceneSysRender(Paused);

        if (CUTDRAWWORLD != 0 && world->current_gscn != NULL) {
            SetLevelLights(world->rtl_set, 1.0f);
            NuGScnRndr3(world->current_gscn);
            if (world->gizmo_sys != NULL && cut != NULL && (cut->flags & 0x8000) != 0) {
                GizmoSysDraw(world->gizmo_sys, world, FRAMETIME);
            }
        }
    }
}

void CutScenes_Stop(CUTSYS *system) {
    if (system == NULL || system->count <= 0) {
        return;
    }
    for (i32 i = 0; i < system->count; ++i) {
        CUTINFO *cut = system->cuts[i];
        if (cut->instance != NULL) {
            instNuGCutSceneStop(static_cast<instNUGCUTSCENE_s *>(cut->instance));
        }
    }
}

void CutScenes_Reset(WORLDINFO_s *world) {
    CUTSYS *system = world->cutscene_sys;
    if (system != NULL) {
        for (i32 i = 0; i < system->count; ++i) {
            CUTINFO *cut = system->cuts[i];
            instNUGCUTSCENE_s *instance = static_cast<instNUGCUTSCENE_s *>(cut->instance);
            if (instance == NULL)
                continue;
            instNUGCUTCHARSYS_s *runtime = instance->character_instance;
            NUGCUTCHARSYS_s *characters = static_cast<NUGCUTSCENE_s *>(cut->scene)->character_system;
            if (runtime == NULL || characters == NULL)
                continue;
            for (i32 j = 0; j < characters->character_count; ++j) {
                instNUGCUTCHAR_s *character = &runtime->characters[j];
                if (character->character != reinterpret_cast<void *>(1))
                    continue;
                NUGCUTCHAR_s *definition = &characters->characters[j];
                GameObject_s *object = NULL;
                if ((character->field_14 & 2) == 0) {
                    for (i32 p = 0; p < 8; ++p) {
                        GameObject_s *player = Player[p];
                        if (player != NULL && (player->apiobj.flags_low & 1) != 0 &&
                            NuStrICmp(player->apiobj.character_data->file, definition->model_file) == 0) {
                            object = player;
                            break;
                        }
                    }
                }
                character->character = object;
                definition->character = object;
            }
        }
    }
    cutaudiopaused = 0;
    CutStopInfo = NULL;
    CutSceneWaiting = 0;
    CutBorderScale = 0.0f;
    if (world->cutscene_sys != NULL && reset_restart != 0 &&
        ((world->current_level->flags & 0xe0) != 0 || world->level_progress == NULL ||
         (world->level_progress->flags & 2) == 0)) {
        CUTINFO *cut = NULL;
        CUTSCENEPLAYERCLIP *clip = CutScenePlayer_Active();
        if (clip != NULL && world->cutscene_sys->count > 0) {
            if (clip->name[0] != '\0')
                cut = CutScene_Find(world->cutscene_sys, clip->name);
            if (cut == NULL)
                cut = world->cutscene_sys->cuts[0];
        }
        if (cut == NULL && cutskip_dontplaylevelintro == 0) {
            for (i32 i = 0; i < world->cutscene_sys->count; ++i) {
                if ((world->cutscene_sys->cuts[i]->flags & 0x1000) != 0) {
                    cut = world->cutscene_sys->cuts[i];
                    break;
                }
            }
        }
        if (cut != NULL) {
            MechSystems::Get()->PauseButton().skip_prompt_timer = 0.0f;
            if (cut->instance != NULL) {
                bool start = true;
                LEVEL_PROGRESS_s *progress = world->level_progress;
                if (world->cutscene_sys != NULL) {
                    i32 index = -1;
                    for (i32 i = 0; i < world->cutscene_sys->count; ++i) {
                        if (world->cutscene_sys->cuts[i] == cut)
                            index = i;
                    }
                    if (progress != NULL) {
                        u32 mask = 1u << (index & 31);
                        if (static_cast<i16>(index) != -1 && (cut->playback_flags & 1) != 0 &&
                            (progress->played_cutscene_mask & mask) != 0)
                            start = false;
                        else
                            progress->played_cutscene_mask |= mask;
                    }
                }
                if (start && CutScene_Start(world, cut, 0) != 0)
                    CutBorderScale = 1.0f;
            }
        }
    }
    NewCutInfoCount = 0;
    cutskip_dontplaylevelintro = 0;
    cut_waiting_for_new_level = 0;
}

void CutScenes_Start(WORLDINFO_s *world) {
    for (i32 i = 0; i < NewCutInfoCount; ++i) {
        CUTINFO *cut = NewCutInfo[i];
        MechSystems::Get()->PauseButton().skip_prompt_timer = 0.0f;
        if (cut == NULL || cut->instance == NULL) {
            continue;
        }
        LEVEL_PROGRESS_s *level_progress = world->level_progress;
        i32 cutscene_index = -1;
        if (world->cutscene_sys != NULL) {
            for (i32 j = 0; j < world->cutscene_sys->count; ++j) {
                if (world->cutscene_sys->cuts[j] == cut) {
                    cutscene_index = j;
                }
            }
            if (level_progress != NULL) {
                const u32 bit = 1U << (cutscene_index & 0x1f);
                if (static_cast<i16>(cutscene_index) != -1 && (cut->end_flags & 1) != 0 &&
                    (level_progress->played_cutscene_mask & bit) != 0) {
                    continue;
                }
                level_progress->played_cutscene_mask |= bit;
            }
        }
        CutScene_Start(world, cut, cutscene_index);
    }
    NewCutInfoCount = 0;
}

void CutScenes_Update(WORLDINFO_s *world, i32 paused) {
    i32 active_before[32] = {};
    if (CutScenePlayer_Active() != 0 && CutStopInfo != NULL) {
        CutScenePlayer_SetObjects(static_cast<CUTINFO *>(CutStopInfo));
    }

    CutInstEnd[0] = NULL;
    CutInstEndStop = -1;
    CUTSTOPGAME = 0;
    CutInstEndCount = 0;
    CUTCAMONLY = 0;
    CutStopInfo = NULL;
    CUTCAM = 0;
    CUTDRAWWORLD = 0;
    CUTNOFOG = 0;
    CUTSKIPLOCK = 0;

    CUTSYS *system = world->cutscene_sys;
    if (system == NULL) {
        return;
    }

    i32 stop_index = -1;
    for (i32 i = 0; i < system->count; ++i) {
        CUTINFO *cut = system->cuts[i];
        if (cut == NULL || cut->instance == NULL) {
            continue;
        }
        instNUGCUTSCENE_s *instance = reinterpret_cast<instNUGCUTSCENE_s *>(cut->instance);
        if ((instance->flags_88 & 2) == 0) {
            continue;
        }
        if ((cut->flags & 1) != 0) {
            CUTSTOPGAME = 1;
            CUTDRAWWORLD = cut->flags & 2;
            CUTNOFOG = cut->flags & 4;
            CutStopInfo = cut;
            stop_index = i;
            DebrisSetRenderGroup(cut->debris_render_group);
            ++CutFrame;
            LevelNameTime = 0.0f;
            LevelNameMul = 0.0f;
            break;
        } else if ((cut->flags & 0x20) != 0) {
            CUTCAMONLY = 1;
        }
    }
    if (stop_index == -1) {
        DebrisSetRenderGroup(1);
    }

    ACTIVECUTCOUNT = 0;
    for (i32 i = 0; i < system->count; ++i) {
        CUTINFO *cut = system->cuts[i];
        if (cut == NULL || cut->instance == NULL) {
            active_before[i] = 0;
            continue;
        }
        instNUGCUTSCENE_s *instance = static_cast<instNUGCUTSCENE_s *>(cut->instance);
        if ((instance->flags_88 & 2) == 0) {
            active_before[i] = 0;
            continue;
        }

        active_before[i] = 1;
        cut->previous_frame = instance->current_frame;
        cut->field_58 += FRAMETIME;
        if (cutaudiopaused != 0 && cut->field_58 >= 3.0f) {
            cutaudiopaused = 0;
        }

        if (i == stop_index || stop_index == -1) {
            bool waiting_for_audio = false;
            if (NOSOUND == 0 && NOMUSIC == 0 && i == stop_index && cut->music_handle != -1 && instance->rate == 0.0f &&
                cutaudiopaused != 0 && music_man.GetStatus(TRACK_CLASS_CUTSCENE, NULL) != 4) {
                instance->rate = 0.0f;
                CutSceneWaiting = 1;
                waiting_for_audio = true;
            }

            if (!waiting_for_audio) {
                i32 music_status = music_man.GetStatus(TRACK_CLASS_CUTSCENE, NULL);
                if (instance->current_frame == 1.0f && cut->music_handle != -1 && music_status != 4) {
                    music_man.PlayTrack(TRACK_CLASS_CUTSCENE, 0);
                } else if (music_status == 4) {
                    if (g_lastCutInfo != cut) {
                        g_accumCutsceneTime += g_lastCutsceneTime;
                        g_lastCutsceneTime =
                            static_cast<NUGCUTSCENE_s *>(cut->scene)->duration / cut->frames_per_second;
                        g_lastCutInfo = cut;
                    }
                    f32 audio_frame = (music_man.GetPlaybackTime(TRACK_CLASS_CUTSCENE) - g_accumCutsceneTime) *
                                      cut->frames_per_second;
                    if (audio_frame < 0.0f) {
                        audio_frame = 0.0f;
                    }
                    f32 rate = audio_frame - (instance->current_frame - 1.0f);
                    instance->rate = rate < 0.0f ? 0.0f : rate;
                } else {
                    instance->rate = cut->frames_per_second * FRAMETIME;
                }

                if (i == stop_index && CutSceneWaiting != 0) {
                    CutSceneWaiting = 0;
                    cutaudiopaused = 0;
                }
            }

            instNuGCutScenePause(instance, 0);
            if (CutScene_PreUpdateFn != NULL) {
                CutScene_PreUpdateFn(cut);
            }
            for (CUTSCENETEXANIM &animation : cut->texture_animations) {
                if (animation.index != -1) {
                    const u32 bit = 1U << (animation.index & 0x1f);
                    if ((texanimbits & bit) == 0 && animation.frame <= instance->current_frame) {
                        texanimbits |= bit;
                    }
                }
            }
        } else {
            instance->rate = 0.0f;
            instNuGCutScenePause(instance, 1);
        }
        ++ACTIVECUTCOUNT;
    }

    if (g_isLowEndDevice != 0 && ACTIVECUTCOUNT > 0) {
        drawcharactermodel_nobsa = 0;
    }
    NuGCutSceneSysUpdate(paused, 0, 1.0f);
    if (CutScene_PostUpdateFn != NULL) {
        CutScene_PostUpdateFn();
    }

    if (paused == 0) {
        for (i32 i = 0; i < system->count; ++i) {
            if (active_before[i] == 0) {
                continue;
            }
            CUTINFO *cut = system->cuts[i];
            instNUGCUTSCENE_s *instance = static_cast<instNUGCUTSCENE_s *>(cut->instance);
            if (instance->rate <= 0.0f) {
                continue;
            }
            for (CUTSCENESFX &sfx : cut->sfx) {
                if (sfx.id != -1 && cut->previous_frame <= sfx.frame && sfx.frame != cut->previous_frame &&
                    sfx.frame <= instance->current_frame) {
                    PlaySfxById(sfx.id, (sfx.flags & 1) != 0 ? &sfx.position : NULL);
                }
            }

            if ((i != stop_index || stop_index == -1) && instNuGCutSceneIsFinished(instance) != 0) {
                if (CutScene_StoppedFn != NULL) {
                    CutScene_StoppedFn(cut);
                }
                if (CutInstEndCount < 4) {
                    CutInstEnd[CutInstEndCount++] = instance;
                    instance->flags_88 |= 2;
                }
            }
        }
    }
    if (stop_index == -1) {
        return;
    }

    CUTINFO *stop_cut = system->cuts[stop_index];
    instNUGCUTSCENE_s *stop_instance = static_cast<instNUGCUTSCENE_s *>(stop_cut->instance);
    GameFog_Reset();
    if (instNuGCutSceneIsFinished(stop_instance) == 0) {
        return;
    }
    if (NewLData == NULL || NewLData == WORLD->current_level || NewLData != HUB_LDATA) {
        FADETYPE fade_type;
        fade_type.type = FADE_TYPE_STILL_WIPE;
        FadeSys.SetFade(fade_type, 0);
    }
    if (CutScene_StoppedFn != NULL) {
        CutScene_StoppedFn(stop_cut);
    }
    for (CUTSCENETEXANIM &animation : stop_cut->texture_animations) {
        if (animation.index != -1) {
            texanimbits &= ~(1U << (animation.index & 0x1f));
        }
    }

    if (stop_cut->skip_level == -1) {
        CUTINFO *next = NewCutScene(NULL, system, stop_cut->next_cutscene, 0);
        if (CutScenePlayer_Active() != 0 && next == NULL) {
            NewLevelFromMenu(HUB_LDATA, -1, -1, 1);
            hub_from_cutsceneplayer = 1;
        } else if ((stop_cut->flags & 0x400) != 0) {
            NewMode = 1;
        } else if (stop_cut->door_name[0] != '\0') {
            DOOR_s *door = Door_FindByName(world, stop_cut->door_name);
            if (door != NULL) {
                Door_GoThrough(world, door, 1);
            }
        } else if ((stop_cut->flags & 0x100) != 0) {
            FADETYPE fade_type;
            fade_type.type = FADE_TYPE_STILL_WIPE;
            FadeSys.SetFade(fade_type, 0);
            NeedScreenGrab(1);
            FadeSys.SetStage(1);
            GameAudio_PlaySfx(0x2d, NULL, 0, 0);
        }
    } else {
        NewLData = &LDataList[stop_cut->skip_level];
        if (NewLData == HUB_LDATA && CutScenePlayer_Active() == 0) {
            if (WORLD->area != NULL && (WORLD->area->flags & 2) != 0) {
                NewLData = SuperStory == 0 ? CREDITS_LDATA : STATUS_LDATA;
            }
        } else if (CutScenePlayer_Active() != 0 &&
                   ((NewLData->flags & (LEVEL_OUTRO | LEVEL_MIDTRO | LEVEL_INTRO)) == 0 &&
                    (stop_cut->end_flags & 2) == 0)) {
            NewLevelFromMenu(HUB_LDATA, -1, -1, 1);
            hub_from_cutsceneplayer = 1;
        }
    }

    if (waiting_for_level != -1) {
        cut_waiting_for_new_level = 1;
        PauseGameCut();
        music_man.PauseTrack(TRACK_CLASS_CUTSCENE);
        newlevel_resumecutaudio = 1;
    }
    CUTNOFOG = 0;

    if (CutInstEndCount < 4) {
        CutInstEndStop = CutInstEndCount;
        CutInstEnd[CutInstEndCount++] = stop_instance;
        stop_instance->flags_88 |= 2;
    }
}

void *CutScene_FindInst(CUTSYS *system, char *name) {
    if (system != NULL) {
        for (i32 i = 0; i < system->count; ++i) {
            if (NuStrICmp(system->cuts[i]->name, name) == 0) {
                return system->cuts[i]->instance;
            }
        }
    }
    return NULL;
}

void CutScenes_Destroy(CUTSYS *system) {
    if (system == NULL || system->count <= 0) {
        return;
    }
    for (i32 i = 0; i < system->count; ++i) {
        CUTINFO *cut = system->cuts[i];
        if (cut->instance != NULL) {
            instNuGCutSceneDestroy(static_cast<instNUGCUTSCENE_s *>(cut->instance));
            cut->instance = NULL;
        }
    }
    for (i32 i = 0; i < system->count; ++i) {
        CUTINFO *cut = system->cuts[i];
        if (cut->scene != NULL) {
            NuGCutSceneDestroy(static_cast<NUGCUTSCENE_s *>(cut->scene));
            cut->scene = NULL;
        }
    }
}

i32 CutScene_HasPlayed(CUTINFO *cut) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    CUTSYS *cutscene_system = world->cutscene_sys;
    if (cut == NULL || cutscene_system == NULL || cutscene_system->count <= 0) {
        return 0;
    }

    i32 cutscene_index = -1;
    for (i32 index = 0; index < cutscene_system->count; ++index) {
        if (cutscene_system->cuts[index] == cut) {
            cutscene_index = index;
        }
    }
    i32 has_played = 0;
    if (cutscene_index != -1) {
        world = WorldInfo_CurrentlyActive();
        has_played = (world->level_progress->played_cutscene_mask & (1u << (cutscene_index & 0x1f))) != 0;
    }
    return has_played;
}

void CutScene_SnapToEnd(CUTINFO *cut) {
    if (cut != NULL) {
        instNuGCutSceneEnd(static_cast<instNUGCUTSCENE_s *>(cut->instance));
    }
}

void CutScene_StartAudio() {
}

i32 CutScene_IsSkippable(CUTINFO *cut) {
    return FadeSys.fade == 0.0f && cut != NULL && (CutStopInfo != cut || CutSceneWaiting == 0 || cutaudiopaused == 0);
}

i32 CutScene_StartFn_LSW(CUTINFO *cut) {
    LOG_INFO("cutscene start requested cut=%p world=%p", (void *)cut, (void *)WORLD);
    if (PODRACE_ADATA != NULL && PODRACE_ADATA == WORLD->area) {
        if (cut != game_cutscenes.podrace_pod_explode && cut != game_cutscenes.podrace_out_of_time) {
            return 0;
        }
        NUMTX matrix;
        NUVEC *position;
        NUANG rotation;
        if (WORLD->current_level == PODRACEA_LDATA) {
            position = &PodRaceADiePos;
            rotation = 0;
        } else {
            rotation = 0x4000;
            position = WORLD->current_level == PODRACEB_LDATA ? &PodRaceBDiePos : &PodRaceCDiePos;
        }

        NuMtxSetRotationY(&matrix, rotation);
        NuMtxTranslate(&matrix, position);
        instNuGCutSceneSetMtx(static_cast<instNUGCUTSCENE_s *>(cut->instance), &matrix);
        return 1;
    }

    if (BONUS_GUNSHIP_ADATA == NULL || BONUS_GUNSHIP_ADATA != WORLD->area ||
        cut != game_cutscenes.bonus_gunship_cavalry_explode) {
        return 0;
    }

    NUMTX matrix;
    NUVEC *position = WORLD->current_level == BONUS_GUNSHIPA_LDATA ? &GunshipADiePos : &GunshipBDiePos;
    NuMtxSetRotationY(&matrix, 0);
    NuMtxTranslate(&matrix, position);
    instNuGCutSceneSetMtx(static_cast<instNUGCUTSCENE_s *>(cut->instance), &matrix);
    return 1;
}

void CutScenes_InitSystem(CUTSCENESYS *system) {
    CutSceneSys = system;
    NuGCutSceneSysInit(cutscene_locatorfns);
    NuSetCutSceneCharacterRenderFn(CutScene_DrawCharacter);
    NuSetCutSceneFindCharactersFn(CutScene_FindCharacters);
    NuSetCutSceneCharacterCreateDataFn(CutScene_CreateCharacterInstance);
    NuSetCutSceneResetCharactersFn(CutScene_ResetCharacters);
    NuSetCutSceneCharacterEvalFn(CutScene_EvalCharacter);
    NuSetCutSceneRigidPostRenderFn(CutScene_RigidPostRender);
}

extern i32 dco_id;
extern i32 dco_reflectaxis;
extern i32 dco_wearinghat;
extern f32 dco_reflectcoord;
extern GAMECHARACTERDATA_s *dco_gcdata;
extern CHARACTERMODEL_s *dco_cmodel;

static void CutScene_DrawCharacter(instNUGCUTSCENE_s *cutscene_instance, NUGCUTSCENE_s *cutscene,
                                   instNUGCUTCHAR_s *instance, NUGCUTCHAR_s *character, f32 frame, i32 paused) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    CUTSYS *cutscene_system = world->cutscene_sys;
    if (cutscene_system == NULL) {
        return;
    }

    CUTINFO *cut = NULL;
    for (i32 i = 0; i < cutscene_system->count; ++i) {
        CUTINFO *candidate = cutscene_system->cuts[i];
        if (candidate->instance == cutscene_instance) {
            cut = candidate;
            break;
        }
    }

    GameObject_s *scene_object = NULL;
    CHARACTERMODEL_s *model = NULL;
    if (cut != NULL && CutScene_ReplaceCharacterModelFn != NULL) {
        i32 replacement_id = CutScene_ReplaceCharacterModelFn(cut, character);
        if (replacement_id != -1 && apicharsys->playermodelids[replacement_id] != -1) {
            model = &apicharsys->models[apicharsys->playermodelids[replacement_id]];
        }
    }
    if (model == NULL) {
        if ((character->flags & 2) == 0) {
            scene_object = static_cast<GameObject_s *>(instance->character_model);
            if (scene_object != NULL) {
                model = scene_object->apiobj.character_model;
            }
        } else {
            model = static_cast<CHARACTERMODEL_s *>(instance->character_model);
        }
    }
    if (model == NULL || model->hierarchy == NULL) {
        return;
    }

    const i32 character_id = model->model_id;
    CHARACTERDATA *character_data = &apicharsys->char_data[character_id];
    GAMECHARACTERDATA *game_data = static_cast<GAMECHARACTERDATA *>(character_data->field11_0x24);
    if (game_data == NULL) {
        return;
    }

    NUMTX world_matrix;
    i32 visible;
    u32 animation_index;
    f32 animation_rate;
    f32 blend_time;
    f32 animation_start_frame;
    i32 layer_mask = -1;
    NuGCutCharAnimProcess(character, frame, &world_matrix, &visible, &animation_index, &animation_rate, &blend_time,
                          &animation_start_frame, &layer_mask);
    bool use_low_detail = false;
    if (cut != NULL) {
        use_low_detail = (cut->flags & 0x10000) != 0 && g_isLowEndDevice != 0;
        if (cut->low_end_distance > 0.0f && g_isLowEndDevice != 0 &&
            NuVecDistSqr(reinterpret_cast<NUVEC *>(&world_matrix.m30), reinterpret_cast<NUVEC *>(&pNuCam->mtx.m30),
                         NULL) > cut->low_end_distance * cut->low_end_distance) {
            return;
        }
    }
    if (paused != 0) {
        animation_rate = 0.0f;
    }
    if (layer_mask == -1) {
        if (use_low_detail) {
            layer_mask = static_cast<i32>(game_data->layer_mask_low);
        } else if (reinterpret_cast<u8 *>(CutSceneSys)[7] == 0) {
            layer_mask = static_cast<i32>(game_data->layer_mask_special);
        } else {
            layer_mask = static_cast<i32>(game_data->layer_mask);
        }
    }
    if (static_cast<i8>(cutscene_instance->flags_88) < 0) {
        NuMtxMul(&world_matrix, &world_matrix, &cutscene_instance->matrix);
    }
    if (scene_object != NULL) {
        scene_object->apiobj.field_0xb8 = world_matrix;
        scene_object->apiobj.position = *reinterpret_cast<NUVEC *>(&world_matrix.m30);
    }
    if (visible == 0) {
        return;
    }

    if (NuIOS_IsLowEndDevice() != 0 && RETAKEINTRO2_LDATA != NULL && RETAKEINTRO2_LDATA == world->current_level) {
        static f32 lastFrameTime;
        static i32 gungansRenderedThisFrame;
        layer_mask = static_cast<i32>(game_data->layer_mask_medium);
        if (frame != lastFrameTime) {
            gungansRenderedThisFrame = 0;
            lastFrameTime = frame;
        }
        if (model->model_id == id_GUNGAN && gungansRenderedThisFrame++ > 7) {
            return;
        }
    }

    i16 render_indices[32];
    const i32 render_count = game_data->make_layer_list(model, render_indices, static_cast<u32>(layer_mask));
    if (render_count < 1) {
        return;
    }

    if (instance->field_15 != animation_index) {
        const u8 requested_animation = static_cast<u8>(animation_index);
        if (!(blend_time > 0.0f) || instance->field_15 == 0xff) {
            instance->field_16 = requested_animation;
            instance->animation_frame_a = !(animation_start_frame > 1.0f) ? 1.0f : animation_start_frame;
        } else {
            if ((instance->field_14 & 1) == 0) {
                if (instance->field_15 == 0) {
                    instance->animation_frame_a = frame;
                    instance->field_16 = requested_animation;
                } else {
                    instance->field_16 = instance->field_15;
                    animation_index = instance->field_15;
                }
            } else {
                instance->field_16 = instance->field_17;
                instance->animation_frame_a = instance->animation_frame_b;
                animation_index = instance->field_17;
            }
            if (!(animation_start_frame > 1.0f)) {
                animation_start_frame = 1.0f;
            }
            instance->field_17 = requested_animation;
            instance->animation_frame_b = animation_start_frame;
            if (requested_animation != static_cast<u8>(animation_index)) {
                instance->blend_progress = 0;
                instance->field_14 |= 1;
            }
        }
        instance->field_15 = requested_animation;
    }

    bool blending = (instance->field_14 & 1) != 0;
    u32 animation_a_index = instance->field_16;
    if (blending) {
        if (blend_time <= 0.0f) {
            instance->blend_progress = 0;
            animation_a_index = instance->field_17;
            instance->field_14 ^= 1;
            instance->field_16 = instance->field_17;
            instance->animation_frame_a = instance->animation_frame_b;
            blending = false;
        } else {
            instance->blend_progress += (1.0f / blend_time) * (FRAMETIME * 60.0f);
            if (instance->blend_progress >= 1.0f) {
                instance->blend_progress = 0;
                animation_a_index = instance->field_17;
                instance->field_14 ^= 1;
                instance->field_16 = instance->field_17;
                instance->animation_frame_a = instance->animation_frame_b;
                blending = false;
            }
        }
    }

    ani3_animheader_s *animation_a;
    nuanimdata2_s *dwa_animation_a;
    if (animation_a_index == 0) {
        instance->animation_frame_a = frame;
        animation_a = reinterpret_cast<ani3_animheader_s *>(character->face_animation);
        dwa_animation_a = character->extra_animation;
    } else {
        animation_a = static_cast<ani3_animheader_s *>(model->model_data_b[animation_a_index - 1]);
        dwa_animation_a = static_cast<nuanimdata2_s *>(model->model_data_c[animation_a_index - 1]);
    }

    NUMTX joint_matrices[256];
    void **dwa = NULL;
    if (!blending) {
        if (animation_a == NULL) {
            NuHGobjEval(model->hierarchy, 0, NULL, joint_matrices);
        } else {
            NuHGobjEvalAnim2(model->hierarchy, animation_a, instance->animation_frame_a, 0, NULL, joint_matrices);
        }
        if (dwa_animation_a != NULL) {
            dwa = NuHGobjEvalDwa2(render_count, render_indices, dwa_animation_a, instance->animation_frame_a);
        }
    } else {
        ani3_animheader_s *animation_b;
        nuanimdata2_s *dwa_animation_b;
        if (instance->field_17 == 0) {
            instance->animation_frame_b = frame;
            animation_b = reinterpret_cast<ani3_animheader_s *>(character->face_animation);
            dwa_animation_b = character->extra_animation;
        } else {
            animation_b = static_cast<ani3_animheader_s *>(model->model_data_b[instance->field_17 - 1]);
            dwa_animation_b = static_cast<nuanimdata2_s *>(model->model_data_c[instance->field_17 - 1]);
        }
        if (animation_a != NULL && animation_b != NULL) {
            NuHGobjEvalAnimBlend2(model->hierarchy, animation_a, instance->animation_frame_a, animation_b,
                                  instance->animation_frame_b, instance->blend_progress, 0, NULL, joint_matrices);
        } else if (animation_b != NULL) {
            NuHGobjEvalAnim2(model->hierarchy, animation_b, instance->animation_frame_b, 0, NULL, joint_matrices);
        } else if (animation_a != NULL) {
            NuHGobjEvalAnim2(model->hierarchy, animation_a, instance->animation_frame_a, 0, NULL, joint_matrices);
        } else {
            NuHGobjEval(model->hierarchy, 0, NULL, joint_matrices);
        }

        if (dwa_animation_a != NULL && dwa_animation_b != NULL) {
            dwa = NuHGobjEvalDwaBlend2(render_count, render_indices, dwa_animation_a, instance->animation_frame_a,
                                       dwa_animation_b, instance->animation_frame_b, instance->blend_progress);
        }

        if (animation_b != NULL && instance->field_17 != 0xff && instance->field_17 != 0 &&
            (cut == NULL || cut != CutStopInfo || CutSceneWaiting == 0)) {
            const f32 end_frame = AnimEndFrame(model, static_cast<u8>(instance->field_17) - 1);
            instance->animation_frame_b += FRAMETIME * 60.0f * animation_rate;
            if (instance->animation_frame_b > end_frame) {
                CHARACTERANIM_s *animation_info =
                    static_cast<CHARACTERANIM_s *>(model->model_data_a[static_cast<u8>(instance->field_17) - 1]);
                if ((animation_info->flags & 2) == 0) {
                    instance->animation_frame_b = end_frame;
                } else {
                    instance->animation_frame_b = instance->animation_frame_b - end_frame + 1.0f;
                }
            }
        }
    }

    NUVEC locator_positions[16];
    NUMTX locator_matrices[16];
    StoreLocatorCoordinates(model, &world_matrix, joint_matrices, locator_positions, locator_matrices);

    if (animation_a != NULL && animation_a_index != 0 && animation_a_index != 0xff &&
        (cut == NULL || cut != CutStopInfo || CutSceneWaiting == 0)) {
        const f32 end_frame = AnimEndFrame(model, animation_a_index - 1);
        instance->animation_frame_a += FRAMETIME * 60.0f * animation_rate;
        if (instance->animation_frame_a > end_frame) {
            CHARACTERANIM_s *animation_info =
                static_cast<CHARACTERANIM_s *>(model->model_data_a[animation_a_index - 1]);
            if ((animation_info->flags & 2) == 0) {
                instance->animation_frame_a = end_frame;
            } else {
                instance->animation_frame_a = instance->animation_frame_a - end_frame + 1.0f;
            }
        }
    }

    EnableShadowMapRendering(0);
    if (Cheats_CheckFlags(1) == 0) {
        FindAndSetLights(reinterpret_cast<NUVEC *>(&world_matrix.m30), 1.0f, world->rtl_set);
    } else {
        SetZeroLights();
    }

    u8 render_character = 1;
    if ((character->flags & 0x20) == 0 && (character_data->model_flags & 0x10000) != 0) {
        render_character = static_cast<u8>(((character->flags >> 4) ^ 1) & 1);
    }
    model->hierarchy->suppress_shadow_surface_points = render_character;

    if ((game_data->flags_090 & 0x8000) != 0) {
        APITransparentCharDraw(model->hierarchy, &world_matrix, render_count, render_indices, joint_matrices, dwa,
                               character->flags & 8);
    }
    NuHGobjRndrMtxDwa(model->hierarchy, &world_matrix, render_count, render_indices, joint_matrices, dwa,
                      character->flags & 8);

    NUMTX *attachment_matrix = NULL;
    if ((character_data->flags & 1) != 0) {
        attachment_matrix = &world_matrix;
        i32 locator = game_data->thingy_locator;
        if (locator != -1 && model->points_of_interest[locator] != NULL) {
            attachment_matrix = &locator_matrices[locator];
        }
        CharScene_Draw(world, character_id, attachment_matrix, NULL);
    }
    ResetShadowMapRendering();

    f32 ground_height = 2000000.0f;
    i32 platform = -1;
    i32 surface = -1;
    bool shadow_sampled = false;
    if ((character->flags & 0x10) != 0 && CutBlobShadowAlpha != 0 && (character_data->model_flags & 0x10000) == 0 &&
        g_isLowEndDevice == 0) {
        u8 shadow_alpha = CutBlobShadowAlpha;
        if (game_data->field_0xf6 != 0xff) {
            shadow_alpha = game_data->field_0xf6;
        }
        if (shadow_alpha != 0) {
            f32 shadow_radius = game_data->field_0x8c;
            if (game_data->shadow_locators == 0) {
                if (!(shadow_radius < 99.0f)) {
                    shadow_radius = character_data->collision_radius * character_data->model_scale * 2.0f;
                }
                if (shadow_radius > 0.0f) {
                    ground_height = GameShadow(NULL, reinterpret_cast<NUVEC *>(&world_matrix.m30), 5.0f, -1);
                    if (ground_height != 2000000.0f) {
                        platform = NewShadowOnPlatform();
                        surface = ShadowInfo();
                        if (ShadNorm.y > 0.0f) {
                            NUVEC shadow_position = *reinterpret_cast<NUVEC *>(&world_matrix.m30);
                            shadow_position.y = ground_height + 0.005f;
                            f32 alpha = BlobShadowFade(&shadow_position, CutBlobShadowFadeNear, CutBlobShadowFadeFar,
                                                       ShadNorm.y);
                            if (alpha > 0.0f) {
                                u16 x_rotation;
                                u16 z_rotation;
                                FindAnglesZX(&ShadNorm, &x_rotation, &z_rotation);
                                NuRndrAddShadow(&shadow_position, shadow_radius,
                                                static_cast<i32>(static_cast<f32>(shadow_alpha) * alpha * ShadNorm.y),
                                                x_rotation, 0, z_rotation);
                            }
                        }
                    }
                    shadow_sampled = true;
                }
            } else {
                if (!(shadow_radius < 99.0f)) {
                    shadow_radius = character_data->collision_radius * character_data->model_scale;
                }
                if (shadow_radius > 0.0f) {
                    f32 alpha = BlobShadowFade(reinterpret_cast<NUVEC *>(&world_matrix.m30), CutBlobShadowFadeNear,
                                               CutBlobShadowFadeFar, 1.0f);
                    if (alpha > 0.0f) {
                        for (i32 i = 0; i < 16; ++i) {
                            if ((game_data->shadow_locators & (1 << i)) != 0 && model->points_of_interest[i] != NULL) {
                                NUVEC shadow_position = locator_positions[i];
                                f32 locator_ground = GameShadow(NULL, &shadow_position, 5.0f, -1);
                                if (locator_ground != 2000000.0f && ShadNorm.y > 0.0f) {
                                    shadow_position.y = locator_ground + 0.005f;
                                    u16 x_rotation;
                                    u16 z_rotation;
                                    FindAnglesZX(&ShadNorm, &x_rotation, &z_rotation);
                                    NuRndrAddShadow(
                                        &shadow_position, shadow_radius,
                                        static_cast<i32>(alpha * static_cast<f32>(shadow_alpha) * ShadNorm.y),
                                        x_rotation, 0, z_rotation);
                                }
                            }
                        }
                    }
                }
                ground_height = 2000000.0f;
                platform = -1;
                shadow_sampled = false;
            }
        }
    }

    f32 reflection_height = 2000000.0f;
    if ((character->flags & 0x20) != 0 && (game_data->flags_090 & 0x8000) == 0 &&
        NuVecDistSqr(reinterpret_cast<NUVEC *>(&world_matrix.m30), reinterpret_cast<NUVEC *>(&GameCam->render_mtx.m30),
                     NULL) < CutReflectRange2 &&
        Reflections_On != 0) {
        if (!shadow_sampled) {
            ground_height = GameShadow(NULL, reinterpret_cast<NUVEC *>(&world_matrix.m30), 5.0f, -1);
            if (ground_height != 2000000.0f) {
                platform = NewShadowOnPlatform();
                surface = ShadowInfo();
            }
        }
        if (surface >= 0 && surface < 32 && (TerSurface[surface].flags & 2) != 0 && ground_height != 2000000.0f) {
            reflection_height = ground_height;
        } else if (platform != -1) {
            reflection_height = FindReflectionNoPlatforms(reinterpret_cast<NUVEC *>(&world_matrix.m30));
        }
        if (reflection_height != 2000000.0f) {
            NUMTX reflection_matrix;
            if (MatrixReflection(&world_matrix, 2, reflection_height, WORLD->current_level->unknown_0cc,
                                 &reflection_matrix) != 0) {
                NuRndrStartReflectionRender(0);
                NuHGobjRndrMtxDwa(model->hierarchy, &reflection_matrix, render_count, render_indices, joint_matrices,
                                  dwa, character->flags & 8);
                if (attachment_matrix != NULL) {
                    NUMTX attachment_reflection;
                    if (MatrixReflection(attachment_matrix, 2, reflection_height, WORLD->current_level->unknown_0cc,
                                         &attachment_reflection) != 0) {
                        CharScene_Draw(world, character_id, NULL, &attachment_reflection);
                    }
                }
                NuRndrEndReflectionRender();
            }
        }
    }

    if (CutSceneSys->field_04 != -1 && reinterpret_cast<i8 *>(CutSceneSys)[6] != -1 &&
        Cheat_IsOn(reinterpret_cast<i8 *>(CutSceneSys)[6]) != 0) {
        dco_id = character_id;
        dco_gcdata = game_data;
        dco_cmodel = model;
        dco_wearinghat = 0;
        dco_reflectaxis = reflection_height != 2000000.0f ? 2 : 0;
        dco_reflectcoord = reflection_height;
        DrawObjectOnCharacter(NULL, NULL, CutSceneSys->field_04, NULL, game_data->head_locator, -1, locator_matrices,
                              reflection_height != 2000000.0f, static_cast<u32>(layer_mask), NULL, NULL, 1.0f, 1.0f);
    }

    if (character->locator_index != 0xff) {
        NUGCUTLOCATORSYS_s *locator_system = cutscene->locator_system;
        for (i32 i = 0; i < character->locator_count; ++i) {
            i32 locator_index = character->locator_index + i;
            NUGCUTLOCATOR_s *locator = &locator_system->locators[locator_index];
            NUMTX *parent_matrix = &world_matrix;
            if (locator->field_5a != 0xff) {
                NUMTX locator_parent;
                NuMtxMulVU0(&locator_parent, &joint_matrices[locator->field_5a], &world_matrix);
                instNuGCutLocatorUpdate(cutscene_instance, locator_system,
                                        &cutscene_instance->locator_instance->locators[locator_index], locator, frame,
                                        &locator_parent, paused);
            } else {
                instNuGCutLocatorUpdate(cutscene_instance, locator_system,
                                        &cutscene_instance->locator_instance->locators[locator_index], locator, frame,
                                        parent_matrix, paused);
            }
        }
    }
}

static void CutScene_EvalCharacter(instNUGCUTSCENE_s *cutscene_instance, NUGCUTSCENE_s *, instNUGCUTCHAR_s *instance,
                                   NUGCUTCHAR_s *character, f32 frame) {
    NUMTX matrix;
    i32 visible;
    u32 animation_index;
    f32 animation_rate;
    f32 blend_time;
    f32 animation_start_frame;
    i32 layer_mask = -1;
    NuGCutCharAnimProcess(character, frame, &matrix, &visible, &animation_index, &animation_rate, &blend_time,
                          &animation_start_frame, &layer_mask);
    if (static_cast<i8>(cutscene_instance->flags_88) < 0) {
        NuMtxMul(&matrix, &matrix, &cutscene_instance->matrix);
    }
    if ((character->flags & 2) == 0 && instance->character_model != NULL) {
        u8 *object = static_cast<u8 *>(instance->character_model);
        memcpy(object + 0xb8, &matrix, sizeof(matrix));
        memcpy(object + 0x5c, &matrix.m30, sizeof(NUVEC));
    }
}

static void CutScene_FindCharacters(NUGCUTSCENE_s *cutscene) {
    NUGCUTCHARSYS_s *system = cutscene->character_system;
    for (i32 i = 0; i < system->character_count; ++i) {
        NUGCUTCHAR_s *character = &system->characters[i];
        character->flags |= 2;

        i32 model_index;
        for (model_index = 0; model_index < apicharsys->loaded_model_count; ++model_index) {
            i32 character_id = apicharsys->models[model_index].model_id;
            if (NuStrICmp(character->name, CDataList[character_id].file) == 0) {
                character->character_model = &apicharsys->models[model_index];
                if (character_id != -1) {
                    CS_cutsys->character_bits[character_id / 32] |= 1U << (character_id & 0x1f);
                }
                break;
            }
        }
        if (model_index == apicharsys->loaded_model_count) {
            character->character_model = NULL;
        }

        if (character->has_locator != 0 && reinterpret_cast<isize>(character->locator) <= 0xfe) {
            character->locator_index = static_cast<u8>(reinterpret_cast<usize>(character->locator));
            character->locator = &cutscene->locator_system->locators[character->locator_index];
        } else {
            character->locator_index = 0xff;
        }
    }
}

static void CutScene_ResetCharacters(instNUGCUTSCENE_s *instance) {
    NUGCUTSCENE_s *cutscene = instance->cutscene;
    instNUGCUTCHARSYS_s *character_instance = instance->character_instance;
    NUGCUTCHARSYS_s *character_system = cutscene->character_system;

    i32 i = 0;
    while (i < character_system->character_count) {
        instNUGCUTCHAR_s *character = &character_instance->characters[i];
        character->field_04 = 0;
        character->field_14 = 0;
        character->field_15 = 0xff;
        ++i;
    }
}

static void CutScene_RigidPostRender(NUGCUTRIGID_s *rigid, instNUGCUTRIGID_s *instance, NUMTX *matrix) {
    if (Reflections_On == 0 || (rigid->flags & 0x10) == 0) {
        return;
    }

    NUVEC *position = reinterpret_cast<NUVEC *>(&matrix->m30);
    if (NuVecDistSqr(position, reinterpret_cast<NUVEC *>(&GameCam->render_mtx.m30), NULL) >= CutReflectRange2) {
        return;
    }

    f32 ground = GameShadow(NULL, position, 5.0f, -1);
    if (ground == 2000000.0f) {
        return;
    }
    i32 surface = ShadowInfo();
    if (static_cast<u32>(surface) > 0x20 || (TerSurface[surface].flags & 2) == 0) {
        return;
    }

    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    NUMTX reflection = *matrix;
    reflection.m01 = -reflection.m01;
    reflection.m11 = -reflection.m11;
    reflection.m21 = -reflection.m21;
    reflection.m31 = ground - (reflection.m31 - ground);

    nuhspecial_s *special = reinterpret_cast<nuhspecial_s *>(instance);
    i32 object_index = LevelObject_FindIndexFromName_RefOnly(NuSpecialGetName(special));
    if (object_index != -1) {
        i32 reflection_index = LevelObject_GetReflection(object_index);
        if (reflection_index != -1) {
            nuhspecial_s *reflection_special = &world->lev_objs[reflection_index].special;
            if (reflection_special != NULL) {
                special = reflection_special;
            }
        }
    }

    NuRndrStartReflectionRender(0);
    NuSpecialDrawAt(special, &reflection);
    NuRndrEndReflectionRender();
}

static void CutScene_CreateCharacterInstance(NUGCUTCHAR_s *character, instNUGCUTCHAR_s *instance, variptr_u *) {
    if ((character->flags & 2) != 0) {
        instance->character_model = character->character_model;
    }
}

void CutScene_DrawSubtitles() {
    CUTINFO *cut = static_cast<CUTINFO *>(CutStopInfo);
    if (cut == NULL || cut->subtitle_data == NULL || cut->subtitle_count == 0) {
        return;
    }

    CUTSCENESUBTITLE *subtitle = cut->subtitle_data;
    for (i32 i = 0; i < cut->subtitle_count; ++i, ++subtitle) {
        f32 frame = cut->field_58;
        if (frame >= subtitle->start_frame && subtitle->end_frame >= frame) {
            f32 alpha = 1.0f;
            if (subtitle->fade_time > 0.0f) {
                f32 fade_end = subtitle->start_frame + subtitle->fade_time;
                if (fade_end > frame) {
                    alpha = (frame - subtitle->start_frame) / (fade_end - subtitle->start_frame);
                } else {
                    f32 fade_start = subtitle->end_frame - subtitle->fade_time;
                    if (frame > fade_start) {
                        alpha = 1.0f - (frame - fade_start) / (subtitle->end_frame - fade_start);
                    }
                }
            }

            SmartTextEx(TTab[subtitle->text_id], subtitle->x, subtitle->y, 1.0f, subtitle->x_scale, subtitle->y_scale,
                        1.0f, subtitle->alignment, subtitle->red, subtitle->green, subtitle->blue, subtitle->max_width,
                        1, NULL, 0, static_cast<i32>(static_cast<f32>(static_cast<i32>(subtitle->alpha)) * alpha));
        }
    }
}

void CutScene_StoppedFn_LSW(CUTINFO *cut) {
    LOG_INFO("cutscene stopped cut=%p world=%p", (void *)cut, (void *)WORLD);
    if (cut != game_cutscenes.cutscene) {
        return;
    }

    for (i32 i = 50; i <= 51; ++i) {
        nuhspecial_s *special = &LevHSpecial[i];
        if (NuSpecialExistsFn(special) != 0) {
            AddPartDebris(WORLD->part_debris_sys, 0x10, NuSpecialGetDrawPos(special));
            NuSpecialSetVisibility(special, 0);
        }
    }
}

void CutScenes_BGLoadManager() {
    if (cutscene_load_instance != NULL && stream_cut_issued == 0) {
        stream_cut_issued = 1;
        bgPostRequest(bgLoadStreamCutScene, bgAckStreamCutScene, NULL, 0);
    }
    NuGCutSceneSysPostBackgroundLoad();
}

void CutScenes_ConfigureList(char *, variptr_u *, variptr_u) {
}

void CutScene_PreUpdateFn_LSW(CUTINFO *cut) {
    if (cut != game_cutscenes.podrace_avalanche && cut != game_cutscenes.cutscene) {
        return;
    }

    instNUGCUTSCENE_s *instance = static_cast<instNUGCUTSCENE_s *>(cut->instance);
    f32 intensity = 111.0f - instance->current_frame;
    if (-110.0f > intensity) {
        intensity = 110.0f;
    }
    intensity = 1.0f - __builtin_fabsf(intensity) / 110.0f;

    f32 distance = NuVecDist(&GameCam->pos, &PodAvalanchePeakPos, NULL);
    if (distance < 750.0f) {
        intensity *= 1.0f - distance / 750.0f;
    }

    NewRumbleAllPlayers(static_cast<f32>(qrand()) * (1.0f / 65535.0f) * intensity, 0.0f, 0, 0);
    GameCam_NewShake(GameCam, intensity + intensity, 0.1f, 1.25f);
}

void CutScene_PostUpdateFn_LSW() {
}

i32 CutScene_PlayingOrRequested(CUTINFO *cut) {
    if (cut == NULL) {
        if (CutStopInfo != NULL || NewCutInfoCount != 0)
            return 1;
        return 0;
    }
    if (cut == CutStopInfo)
        return 1;
    for (i32 i = 0; i < NewCutInfoCount; ++i) {
        if (NewCutInfo[i] == cut)
            return 1;
    }
    return 0;
}

i32 CutScene_ReplaceCharacterModelFn_LSW(CUTINFO *cut, NUGCUTCHAR_s *character) {
    if (character == NULL || cut == NULL) {
        return -1;
    }

    if (PODRACE_ADATA != NULL && PODRACE_ADATA == WORLD->area) {
        if (cut != game_cutscenes.podrace_pod_explode && cut != game_cutscenes.podrace_out_of_time &&
            cut != game_cutscenes.podrace_sebulba) {
            return -1;
        }
        if (id_ANAKINSPOD == -1 || NuStrICmp(apicharsys->char_data[id_ANAKINSPOD].file, character->name) != 0) {
            return -1;
        }
        GameObject_s *vehicle = CutDeadVehiclePlayer != NULL ? CutDeadVehiclePlayer : player;
        if (vehicle != NULL && vehicle->id == id_ANAKINSPODGREEN) {
            return vehicle->id;
        }
        return -1;
    }

    if (WORLD->current_level == PODSPRINTA_LDATA) {
        if (cut != game_cutscenes.podsprint_out_of_time && cut != game_cutscenes.podsprint_sebulba) {
            return -1;
        }
        if (id_ANAKINSNEWPOD == -1 || NuStrICmp(apicharsys->char_data[id_ANAKINSNEWPOD].file, character->name) != 0) {
            return -1;
        }
        GameObject_s *vehicle = CutDeadVehiclePlayer != NULL ? CutDeadVehiclePlayer : player;
        if (vehicle != NULL && vehicle->id == id_ANAKINSNEWPODGREEN) {
            return vehicle->id;
        }
        return id_ANAKINSNEWPOD;
    }

    if (BONUS_GUNSHIP_ADATA != NULL && BONUS_GUNSHIP_ADATA == WORLD->area) {
        if (cut == game_cutscenes.bonus_gunship_cavalry_explode && player != NULL &&
            player->id == id_REPUBLICGUNSHIP_GREEN) {
            return player->id;
        }
        return -1;
    }

    if (GUNSHIP_ADATA != NULL && GUNSHIP_ADATA == WORLD->area) {
        if (cut != game_cutscenes.bonus_gunship_cavalry_explode) {
            return -1;
        }
        if (player != NULL && player->id == id_NEW_REPUBLIC_GUNSHIP_GREEN) {
            return player->id;
        }
        return id_NEW_REPUBLIC_GUNSHIP;
    }

    if (BATTLEOVERCORUSCANT_ADATA != NULL && BATTLEOVERCORUSCANT_ADATA == WORLD->area &&
        cut == game_cutscenes.dogfight_die && player != NULL && player->id == id_JEDISTARFIGHTERYELLOWEP3) {
        return player->id;
    }
    return -1;
}

void ResetScene(nugscn_s *scene, SCENEPROGRESS_s *progress) {
    if (progress == NULL || scene == NULL)
        return;
    i32 count = NuSpecialGetNumSpecials(scene);
    if (count == 0)
        return;
    nuhspecial_s special;
    for (i32 i = 0; i < count; ++i) {
        if (i != 0)
            NuSpecialGetNext(&special);
        else
            NuSpecialGetFirst(scene, &special, 1);
        NuSpecialSetVisibility(&special, progress->specials[i].visible);
        NUMTX *base = NuSpecialGetMtx(&special);
        NuSpecialSetDrawMtx(&special, base);
        NuSpecialUpdate(&special);
        if (scene->instance_animation_data == NULL)
            continue;
        nuinstanim_s *animation = NuSpecialGetInstAnim(&special);
        if (animation == NULL)
            continue;
        nuanimdata_s *data = scene->instance_animation_data[animation->anim_ix];
        if (data == NULL)
            continue;
        animation->ltime = static_cast<f32>(static_cast<u32>(progress->specials[i].frame));
        animation->playing = progress->specials[i].playing;
        animation->waiting = progress->specials[i].waiting;
        animation->repeating = progress->specials[i].repeating;
        animation->tfactor = progress->specials[i].tfactor;
        animation->prev_eval_time = animation->ltime;
        NuAnimData2CalcMatrix(data, 0, animation->ltime, &animation->mtx);
        scene->instance_animation_matrices[animation - scene->instance_animations] = animation->mtx;
        NUVEC *translation = NUMTX_GET_ROW_VEC(&animation->mtx, 3);
        NUVEC *base_translation = NUMTX_GET_ROW_VEC(base, 3);
        translation->x += base_translation->x;
        translation->y += base_translation->y;
        translation->z += base_translation->z;
    }
}

CUTINFO *NewCutScene(CUTINFO *cut, CUTSYS *system, char *name, i32) {
    if (cut == NULL) {
        if (name == NULL || system == NULL)
            return NULL;
        cut = CutScene_Find(system, name);
    }
    if (cut != NULL) {
        if (NewCutInfoCount < 8) {
            NewCutInfo[NewCutInfoCount++] = cut;
            return cut;
        }
    }
    return NULL;
}

void NextStatusStage(STATUSPACKET_s *);
void FinishStatusPacket(i32);
f32 StatusIconsOnOff(f32);
extern f32 icon_y;

void Exit_LSW_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    if (stage->field_0x14 >= 0) {
        stage->field_0x18 += elapsed;
    }
    if (stage->field_0x18 >= stage->field_0x1c) {
        NextStatusStage(packet);
        if (packet->current_gold_brick < packet->stage_count - 1) {
            stage->field_0x1c = 0.1f;
        } else {
            FinishStatusPacket(packet->prompt_choice);
        }
    }
}

void Fade_LSW_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    if (stage->field_0x14 == 0) {
        stage->field_0x18 = 0.0f;
        stage->field_0x1c = 0.6f;
        stage->field_0x14 = 1;
    } else if (stage->field_0x14 == 1) {
        stage->field_0x18 += elapsed;
        if (stage->field_0x18 >= stage->field_0x1c) {
            FinishStatusPacket(packet->prompt_choice);
        }
        f32 ratio = 0.0f;
        if (stage->field_0x1c != 0.0f && stage->field_0x18 != 0.0f) {
            ratio = stage->field_0x18 / stage->field_0x1c;
        }
        icon_y = -StatusIconsOnOff(ratio);
    }
}

void RelocateCutScene(NUGCUTSCENE_s *, variptr_u *) {
}

i32 STATUS_R = 255;
i32 STATUS_G = 191;
i32 STATUS_B;
extern f32 iconalphaoverride;
extern i16 tTRUEJEDI, tSTORY, tFREEPLAY;
extern i16 tLEVELCOMPLETE, tFREEPLAYUNLOCKED, tSUPERSTORYCOMPLETE, tBONUSUNLOCKED;
extern i16 tEPISODEVUNLOCKED, tEPISODEVIUNLOCKED, tCHALLENGECOMPLETE, tOUTOFTIME;
extern i16 tYOUDIDNTCOLLECTALL10CANISTERS, tMISSIONCOMPLETE, tSTORYCLIPSUNLOCKED;
extern i16 tEPISODEICOMPLETE, tEPISODEIICOMPLETE, tEPISODEIIICOMPLETE;
extern i16 tEPISODEIVCOMPLETE, tEPISODEVCOMPLETE, tEPISODEVICOMPLETE;
f32 getFinishedStatusAlpha(STATUSPACKET_s *);
bool FreePlayUnlocked();
void IncreaseScore(u32 *, u64, i32);
void NewStatusRumbleBuzz(i32, f32, f32, i32);
void Text_MakeScore(u32, char *);
void AddFancyMessage(char *, f32, f32, f32, f32, i32, i32);
void *AddGameMessage(char *, nuvec_s *, f32, nuvec_s *, f32, u8, u8, u8, u32, f32);
extern "C" void PlaySfx(char *, nuvec_s *);
extern "C" void Text3D(char *, f32, f32, f32, f32, f32, f32, u32, u8, u8, u8);
i32 DrawPanel3DObjectNoAlpha(f32, f32, f32, f32, f32, f32, u16, u16, u16, nuhspecial_s *, i32);

void GoldBrick_LSW_Draw(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, i32 active) {
    if (active == 0)
        return;
    if (stage->field_0x14 > 0) {
        const f32 time = stage->field_0x18;
        if (time >= 1.0f && time < stage->field_0x1c - 1.0f) {
            f32 blend = 1.0f;
            i32 count = packet->previous_gold_bricks;
            if (time >= 2.0f) {
                const f32 end = stage->field_0x1c - 2.0f;
                if (time >= end)
                    blend = NuTrigTable[(static_cast<i32>((1.0f - (time - end)) * 16384.0f) >> 1) & 0x7fff];
                count = Game.field_0x7c26[0];
                if (time < (Game.field_0x7c26[0] - packet->previous_gold_bricks - 1) * 2.5f + 2.0f) {
                    count = (packet->previous_gold_bricks + 1) + (time - 2.0f) / 2.5f;
                }
            } else
                blend = NuTrigTable[(static_cast<i32>((time - 1.0f) * 16384.0f) >> 1) & 0x7fff];
            char text[268];
            sprintf(text, "%i/%i", count, GOLDBRICKPOINTS);
            SmartTextEx(text, 0.0f, blend * -0.99999994f + 1.3f, 1.0f, 1.5f, 1.5f, 1.5f, 0, 255, 255, 255, 1.7f, 1,
                        NULL, 0, static_cast<i32>(blend * 128.0f));
            const u32 angle = NuFmod(GlobalTimer.time_elapsed, 4.0f) * 0.25f * 65536.0f;
            DrawPanel3DObjectNoAlpha(0.0f, blend * 0.99999994f - 1.3f, 1.0f, 1.0f, 1.0f, 1.0f,
                                     static_cast<i32>(NuTrigTable[angle & 0x7fff] * 1820.0f), angle, 0,
                                     reinterpret_cast<nuhspecial_s *>(&WORLD->lev_objs[0xd3]), 2);
            if (Game.field_0x7c26[0] > 60)
                CoinTotal_Draw(*packet->score, (STATSPOSY - STATSPOS2Y) * blend + STATSPOS2Y, CoinTotalScale, 1, 1.0f,
                               255, 191, 0);
        }
    }
    if ((stage->field_0x14 == 0 || stage->field_0x18 < 1.0f) && packet->mission_state != 0 &&
        packet->previous_gold_bricks < Game.field_0x7c26[0])
        iconalphaoverride = 0.0f;
}

void GoldBrick_LSW_Skip(STATUS_STAGE_s *, STATUSPACKET_s *packet) {
    if (Game.field_0x7c26[0] < 61)
        NextStatusStage(packet);
}

void GoldBrick_LSW_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    const i32 count = Game.field_0x7c26[0] - packet->previous_gold_bricks;
    if (stage->field_0x14 == 0) {
        stage->field_0x14 = 1;
        stage->field_0x18 = 0.0f;
        stage->field_0x1c = (count - 1) * 2.5f + 2.0f + 3.0f + 1.0f + 1.0f;
    } else if (stage->field_0x14 == 1) {
        const f32 previous = stage->field_0x18;
        stage->field_0x18 += elapsed;
        if (stage->field_0x18 >= stage->field_0x1c)
            NextStatusStage(packet);
        else
            for (i32 i = 0; i < count; ++i) {
                const f32 at = i * 2.5f + 2.0f;
                if (previous < at && stage->field_0x18 >= at) {
                    PlaySfx(const_cast<char *>("TrueJedi_100pc"), NULL);
                    NewStatusRumbleBuzz(-1, 0.6f, 0.0f, 0);
                    const i32 points = packet->previous_gold_bricks + i + 1;
                    char text[252];
                    sprintf(text, "%i/%i", points, GOLDBRICKPOINTS);
                    AddFancyMessage(text, 0.0f, 0.3f, 0.75f, 1.25f, 1, 1);
                    nuvec_s position = {0.0f, 0.0375f, 1.0f};
                    const i16 id = reinterpret_cast<i16 *>(packet->field_0x12c)[i];
                    char *label = TTab[id];
                    if (id == tTRUEJEDI) {
                        NuStrCpy(text, label);
                        if (BOTHTRUEJEDIGOLDBRICKS != 0) {
                            NuStrCat(text, " ");
                            NuStrCat(text, TTab[(packet->field_0xb0 & 0x40) != 0 ? tFREEPLAY : tSTORY]);
                        }
                        label = text;
                    }
                    AddGameMessage(label, &position, 0.7f, &position, 0.7f, 255, 0, 127, 0x4020, 2.0f);
                    if (points > 60) {
                        IncreaseScore(packet->score, 100000, 0);
                        CoinTotalScale = 1.5f;
                        PlaySfx(const_cast<char *>("Shop_BuyCheat"), NULL);
                        Text_MakeScore(100000, text);
                        AddFancyMessage(text, 0.0f, -STATSPOSY, 0.5f, 1.25f, 0, 0);
                    }
                }
            }
    }
}

void LevelComplete_LSW_Draw(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, i32 active) {
    if (active == 0) {
        if (stage->field_0x12 != 0) {
            const f32 alpha = getFinishedStatusAlpha(packet);
            if ((stage->type == 26 && packet->mission_state == 2) ||
                (stage->type == 23 && packet->challenge_state == 2)) {
                CoinTotal_Draw(*packet->score, STATSPOSY, CoinTotalScale, 1, alpha, 255, 191, 0);
            }
        }
        return;
    }
    char text[252];
    if (stage->field_0x14 > 0) {
        f32 alpha = 1.0f;
        if (stage->field_0x18 < 0.5f)
            alpha = stage->field_0x18 + stage->field_0x18;
        else if (stage->field_0x18 >= 3.5f)
            alpha = 1.0f - ((stage->field_0x18 - 3.5f) + (stage->field_0x18 - 3.5f));
        if (alpha > 0.0f) {
            const i32 opacity = alpha * 128.0f;
            i16 first = -1, second = -1;
            f32 y = 0.1f, scale = 0.7f;
            i32 red = STATUS_R, green = STATUS_G, blue = STATUS_B;
            switch (stage->type) {
                case 14:
                    strcpy(text, TTab[EDataList[packet->episode_id].text_id]);
                    SmartTextEx(text, 0.0f, 0.1f, 1.0f, 0.525f, 0.525f, 0.525f, 0, red, green, blue, 1.7f, 1, NULL, 0,
                                opacity);
                    second = tSUPERSTORYCOMPLETE;
                    break;
                case 13:
                    first = tLEVELCOMPLETE;
                    if ((PODRACE_ADATA != NULL && packet->area == PODRACE_ADATA) ||
                        (BONUS_GUNSHIP_ADATA != NULL && packet->area == BONUS_GUNSHIP_ADATA) || !FreePlayUnlocked())
                        y = 0.0f;
                    else
                        second = tFREEPLAYUNLOCKED;
                    break;
                case 8:
                    first = tEPISODEVUNLOCKED;
                    second = tEPISODEVIUNLOCKED;
                    break;
                case 28:
                    first = tBONUSUNLOCKED;
                    y = 0.0f;
                    break;
                case 23:
                    y = 0.0f;
                    if (packet->challenge_state == 2) {
                        first = tCHALLENGECOMPLETE;
                        red = green = blue = 255;
                    } else
                        first = packet->challenge_state == 3 ? tOUTOFTIME : tYOUDIDNTCOLLECTALL10CANISTERS;
                    break;
                case 26:
                    y = 0.0f;
                    if (packet->mission_state == 2) {
                        first = tMISSIONCOMPLETE;
                        red = green = blue = 255;
                    } else
                        first = tOUTOFTIME;
                    break;
                case 29:
                    first = tEPISODEICOMPLETE;
                    second = tSTORYCLIPSUNLOCKED;
                    break;
                case 30:
                    first = tEPISODEIICOMPLETE;
                    second = tSTORYCLIPSUNLOCKED;
                    break;
                case 31:
                    first = tEPISODEIIICOMPLETE;
                    second = tSTORYCLIPSUNLOCKED;
                    break;
                case 32:
                    first = tEPISODEIVCOMPLETE;
                    second = tSTORYCLIPSUNLOCKED;
                    break;
                case 33:
                    first = tEPISODEVCOMPLETE;
                    second = tSTORYCLIPSUNLOCKED;
                    break;
                default:
                    first = tEPISODEVICOMPLETE;
                    second = tSTORYCLIPSUNLOCKED;
                    break;
            }
            if (first != -1)
                SmartTextEx(TTab[first], 0.0f, y, 1.0f, scale, scale, scale, 0, red, green, blue, 1.7f, 1, NULL, 0,
                            opacity);
            if (second != -1)
                SmartTextEx(TTab[second], 0.0f, -0.1f, 1.0f, 0.7f, 0.7f, 0.7f, 0, STATUS_R, STATUS_G, STATUS_B, 1.7f, 1,
                            NULL, 0, opacity);
        }
    }
    if (stage->type != 23 && stage->type != 26)
        return;
    if (stage->field_0x14 == 0 || stage->field_0x18 < stage->field_0x1c - 0.5f)
        iconalphaoverride = 0.0f;
    else
        iconalphaoverride = (stage->field_0x18 - (stage->field_0x1c - 0.5f)) * 2.0f;
    if (stage->type == 26) {
        if (packet->mission_state != 2)
            return;
        if (packet->previous_gold_bricks < Game.field_0x7c26[0])
            iconalphaoverride = 0.0f;
    } else if (packet->challenge_state != 2)
        return;
    f32 progress, slide;
    if (stage->field_0x18 < 0.75f) {
        progress = 0.0f;
        slide = stage->field_0x18 / 0.7f;
    } else if (stage->field_0x18 >= stage->field_0x1c - 0.75f) {
        progress = 1.0f;
        slide = 1.0f - (stage->field_0x18 - (stage->field_0x1c - 0.75f)) / 0.75f;
    } else {
        progress = (stage->field_0x18 - 0.75f) / (stage->field_0x1c - 0.75f - 0.75f);
        slide = 1.0f;
    }
    const f32 coin_slide = stage->field_0x18 < 0.75f ? slide : 1.0f;
    u32 score = packet->original_score;
    IncreaseScore(&score, static_cast<i64>(static_cast<i32>(static_cast<f32>(packet->reward_score - score) * progress)),
                  0);
    CoinTotal_Draw(score,
                   (STATSPOSY - STATSPOS2Y) * NuTrigTable[(static_cast<i32>(coin_slide * 16384.0f) >> 1) & 0x7fff] +
                       STATSPOS2Y,
                   CoinTotalScale, 1, 1.0f, 255, 191, 0);
    f32 remaining = (stage->type == 26 ? static_cast<u16>(packet->mission->time) : packet->area->challenge_trial_time) -
                    packet->elapsed_time;
    if (remaining < 0.0f)
        remaining = 0.0f;
    Text_MakeTime((1.0f - progress) * remaining, 0, 1, 1, text);
    Text3D(text, 0.0f,
           -((STATSPOSY - STATSPOS2Y) * NuTrigTable[(static_cast<i32>(slide * 16384.0f) >> 1) & 0x7fff] + STATSPOS2Y),
           1.0f, 0.6f, 0.6f, 0.6f, 0, 255, 191, 0);
}

void LevelComplete_LSW_Skip(STATUS_STAGE_s *stage, STATUSPACKET_s *packet) {
    if ((stage->type == 26 && packet->mission_state == 2) || (stage->type == 23 && packet->challenge_state == 2)) {
        *packet->score = packet->reward_score;
    }
    NextStatusStage(packet);
}

void SetDrawGoldBrick(STATUSPACKET_s *, i32);
void NewStatusRumbleBuzz(i32, f32, f32, i32);
extern "C" void PlaySfx(char *, nuvec_s *);

void LevelComplete_LSW_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    if (stage->field_0x14 == 0) {
        stage->field_0x18 = 0.0f;
        stage->field_0x1c = 4.0f;
        stage->field_0x14 = 1;
        return;
    }
    if (stage->field_0x14 != 1) {
        return;
    }
    SetDrawGoldBrick(packet, packet->current_gold_brick);
    const f32 previous = stage->field_0x18;
    stage->field_0x18 += elapsed;
    if (stage->field_0x18 >= stage->field_0x1c) {
        if ((stage->type == 26 && packet->mission_state == 2) || (stage->type == 23 && packet->challenge_state == 2)) {
            *packet->score = packet->reward_score;
        }
        NextStatusStage(packet);
        return;
    }
    if (previous < 0.5f && stage->field_0x18 >= 0.5f) {
        if ((stage->type == 23 && packet->challenge_state == 3) || (stage->type == 26 && packet->mission_state == 3)) {
            GameAudio_PlaySfx(0x32, NULL, 0, 0);
        } else {
            PlaySfx(const_cast<char *>("StatusAward"), NULL);
        }
        NewStatusRumbleBuzz(-1, 0.6f, 0.0f, 0);
        return;
    }
    if ((stage->type != 26 || packet->mission_state != 2) && (stage->type != 23 || packet->challenge_state != 2)) {
        return;
    }
    const f32 reward_end = stage->field_0x1c - 0.75f;
    if (previous < reward_end && stage->field_0x18 >= reward_end) {
        CoinTotalScale = 1.5f;
        NewStatusRumbleBuzz(-1, 0.0f, 0.1f, 0);
        PlaySfx(const_cast<char *>("Shop_BuyCheat"), NULL);
    } else if (stage->field_0x18 >= 0.75f && stage->field_0x18 < reward_end) {
        PlaySfx(const_cast<char *>("PickupCoin"), NULL);
    }
}

static __used__ void Titles_Draw(WORLDINFO_s *) {
}

static __used__ void Titles_Init(WORLDINFO_s *) {
}

static __used__ void Titles_Update(WORLDINFO_s *) {
}
