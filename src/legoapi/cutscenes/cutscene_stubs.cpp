#include "legoapi/world/world_shared.h"
#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nugcutscene.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numusic/numusic.h"
#include "nu2api/nusound/nusound.h"

#include <cfloat>
#include <string.h>

extern "C" i32 NuGCutLocatorIsVisble(NUGCUTLOCATOR_s *, f32, nuanimtime_s *, f32 *, f32 *);
extern "C" i32 NuGCutLocatorCalcMtx(NUGCUTLOCATOR_s *, f32, NUMTX *, nuanimtime_s *);
void instNuGCutSceneEndButNotSystems(instNUGCUTSCENE_s *instance);
void instNuGCutSceneResetCamLock(instNUGCUTSCENE_s *);

extern "C" {
    extern debinftype **debtab;
    extern NUGCUTLOCATORFNENTRY_s *locatorfns;
    extern f32 timeincrement;
    extern i32 processdeb;
    extern f32 glyntestha;

    i32 (*TriggerLocatorVfxFn)(u16, f32 *) = NULL;
    void (*ReleaseLocatorVfxFn)(i32) = NULL;
    extern void (*NuCutSceneCharacterRelease)(instNUGCUTCHAR_s *, NUGCUTCHAR_s *);
    void (*UpdateLocatorVfxFn)(i32, f32 *) = NULL;
    void (*NuCutSceneSFXUpdate)(NUGCUTLOCATORSYS_s *, instNUGCUTLOCATOR_s *, NUGCUTLOCATOR_s *, f32, NUMTX *,
                                i32) = NULL;

    void NuAnimData2CalcTime(nuanimdata2_s *, f32, nuanimtime_s *);
    void NuMtxPreTranslate(NUMTX *, NUVEC *);
    void NuMtxMul(NUMTX *, NUMTX *, NUMTX *);

    i32 AddScaledVariableShotDebrisEffect2(i32, NUVEC *, i32, f32, NUMTX *, NUMTX *, f32);
    void AddDebrisEffect(i32 *, i32, f32, f32, f32);
    void DebrisPopulateInstance(i32, f32);
    void DebrisEmitterPos(i32, f32, f32, f32);
    void DebrisOrientationMtx(i32, NUMTX *);
    void DebrisPosOrientationMtx(i32, NUMTX *);
    void AddScaledFiniteShotDebrisEffect(i32 *, i32, NUVEC *, NUVEC *, NUVEC *, i32, f32);
    void DebrisEmitterOrientationMtx(i32, NUMTX *);
    void DebFreeInstantly(i32 *);
}

extern "C" {

    i32 CheckStreamFileID(void) {
        return 0;
    }

    void ClearLinkedCutSceneMusic(void *context) {
        if (context == NULL || Music.track_data == context) {
            Music.track_data = NULL;
        }
    }

    void DisplayCutSceneMemory(void) {
    }

    void PauseGameCut(void) {
        if (NOMUSIC != 0 || NOSOUND != 0 || NUSOUND_STREAM_3 == -1) {
            return;
        }

        MusicPlayback *music = &Music;
        if (music->state != MUSIC_PLAYBACK_DUAL_STREAM && music->state != MUSIC_PLAYBACK_DUAL_STREAM_PENDING) {
            return;
        }
        NuSound3CancelCheckStereo();
        NuSound3PauseStereoStream(1 - music->primary_stream);
    }

    void RestoreGameCut(void) {
        MusicPlayback *music = &Music;

        if (NOMUSIC != 0 && NOSOUND != 0) {
            return;
        }
        if (NUSOUND_STREAM_3 == -1) {
            return;
        }
        if (music->state != MUSIC_PLAYBACK_DUAL_STREAM && music->state != MUSIC_PLAYBACK_DUAL_STREAM_PENDING) {
            return;
        }
        NuSound3ResumeStereoStream(1 - music->primary_stream);
    }

    u8 ForceScenePlayBack;

    void SetForceScenePlayBack(i32 enabled) {
        ForceScenePlayBack = static_cast<u8>(enabled);
    }

    void instCutSceneTimeElapsed(void) {
    }

    i32 instNuGCutSceneAddCamTgt(instNUGCUTSCENE_s *instance, NUVEC *target, f32 start_frame, f32 duration,
                                 i8 target_index) {
        instNUGCUTSCENECAMERA_s *camera = instance->camera_instance;
        if (camera == NULL || camera->target_count >= camera->target_capacity) {
            return 0;
        }
        instNUGCUTCAMTGT_s *entry = &camera->targets[camera->target_count++];
        entry->target = target;
        entry->start_frame = start_frame;
        entry->duration = duration;
        entry->target_index = target_index;
        return 1;
    }

    void instNuGCutSceneAddCleanUpItem(void) {
    }

    void instNuGCutSceneCalculateAverageCentre(void) {
    }

    void instNuGCutSceneChain(instNUGCUTSCENE_s *instance, instNUGCUTSCENE_s *next) {
        instance->chained_instance = next;
    }

    void instNuGCutSceneCharGetStartMtx(void) {
    }

    void instNuGCutSceneCleanUp(void) {
    }

    void instNuGCutSceneCreateCamTgtArray(instNUGCUTSCENE_s *instance, i32 count, VARIPTR *buf) {
        if (count == 0 || instance->camera_instance == NULL) {
            return;
        }
        instNUGCUTSCENECAMERA_s *camera = instance->camera_instance;
        camera->target_capacity = static_cast<u8>(count);
        buf->addr = ALIGN(buf->addr, 0x10);
        camera->targets = reinterpret_cast<instNUGCUTCAMTGT_s *>(buf->void_ptr);
        buf->void_ptr = camera->targets + count;
        memset(camera->targets, 0, count * sizeof(instNUGCUTCAMTGT_s));
    }

    void instNuGCutSceneDisable(instNUGCUTSCENE_s *instance) {
        instance->flags_89 |= 0x8;
    }

    void instNuGCutSceneEnable(instNUGCUTSCENE_s *instance) {
        instance->flags_89 &= ~0x8;
    }

    void instNuGCutSceneFind(void) {
    }

    i32 instNuGCutSceneIsFinished(instNUGCUTSCENE_s *instance) {
        return (instance->flags_89 & 0x10) != 0 ? -1 : 0;
    }

    void instNuGCutSceneJumpToEnd(void) {
    }

    void instNuGCutSceneJumpToLastFrame(void) {
    }

    void instNuGCutScenePlay(void) {
    }

    void instNuGCutSceneResetCleanUp(void) {
    }

    void instNuGCutSceneRotateY(void) {
    }

    void instNuGCutSceneSetEndCallback(instNUGCUTSCENE_s *instance, void (*callback)(instNUGCUTSCENE_s *)) {
        instance->end_callback = callback;
    }

    void instNuGCutSceneSetMtx(instNUGCUTSCENE_s *instance, NUMTX *matrix) {
        instance->flags_88 |= 0x80;
        instance->matrix = *matrix;

        NUMTX *instance_matrix = &instance->matrix;
        NUVEC *bounds = static_cast<NUVEC *>(instance->cutscene->bounds);
        if (bounds != NULL) {
            instance->transformed_bounds_center.x = (bounds[1].x + bounds[0].x) * 0.5f;
            instance->transformed_bounds_center.y = (bounds[1].y + bounds[0].y) * 0.5f;
            instance->transformed_bounds_center.z = (bounds[1].z + bounds[0].z) * 0.5f;
        } else {
            instance->transformed_bounds_center.x = 0.0f;
            instance->transformed_bounds_center.y = 0.0f;
            instance->transformed_bounds_center.z = 0.0f;
        }
        NuVecMtxTransform(&instance->transformed_bounds_center, &instance->transformed_bounds_center, instance_matrix);
    }

    void instNuGCutSceneSetPos(void) {
    }

    void instNuGCutSceneSetRepeat(void) {
    }

    void instNuGCutSceneStop(instNUGCUTSCENE_s *instance) {
        instance->flags_88 &= ~6;
        instance->flags_89 &= ~0x10;
        if ((instance->flags_8a & 4) != 0) {
            instNUGCUTCHARSYS_s *runtime = instance->character_instance;
            if (runtime != NULL) {
                NUGCUTCHARSYS_s *system = instance->cutscene->character_system;
                for (i32 i = 0; i < system->character_count; ++i) {
                    instNUGCUTCHAR_s *character = &runtime->characters[i];
                    if (character->character != NULL && NuCutSceneCharacterRelease != NULL)
                        NuCutSceneCharacterRelease(character, &system->characters[i]);
                }
            }
        } else {
            instNUGCUTCHARSYS_s *runtime = instance->character_instance;
            if (runtime != NULL) {
                NUGCUTCHARSYS_s *system = instance->cutscene->character_system;
                for (i32 i = 0; i < system->character_count; ++i) {
                    instNUGCUTCHAR_s *character = &runtime->characters[i];
                    if (character->character != NULL && NuCutSceneCharacterRelease != NULL)
                        NuCutSceneCharacterRelease(character, &system->characters[i]);
                }
            }
        }
        instNuGCutSceneResetCamLock(instance);
    }

    f32 instNuGCutSceneTimeLeft(instNUGCUTSCENE_s *instance) {
        if (instance == NULL || instance->cutscene == NULL) {
            return 0.0f;
        }

        NUGCUTSCENE_s *cutscene = instance->cutscene;
        f32 frames_left;
        if (cutscene->version > 9) {
            frames_left = static_cast<f32>(static_cast<i32>(cutscene->total_stream_frames)) - instance->current_frame;
        } else {
            if (cutscene->version > 1 && (cutscene->flags & 1) != 0 && cutscene->last_stream != 0) {
                return FLT_MAX;
            }
            frames_left = cutscene->duration - 1.0f - instance->current_frame;
        }

        f32 time_left = 0.0f;
        if (frames_left > 0.0f) {
            if (instance->rate == 0.0f) {
                return FLT_MAX;
            }
            time_left = NuFdiv(frames_left, instance->rate);
        }
        return time_left;
    }

    f32 instNuGCutSceneTotalTime(instNUGCUTSCENE_s *instance) {
        if (instance == NULL || instance->cutscene == NULL || instance->cutscene->version <= 9) {
            return 0.0f;
        }

        f32 total_frames = instance->cutscene_copy->total_stream_frames;
        if (instance->rate != 0.0f) {
            return NuFdiv(total_frames, instance->rate);
        }
        return 0.0f;
    }

    void instNuGCutSceneTranslate(void) {
    }

    void instNuGCutSceneWaitAtEnd(void) {
    }

    void instNuGCutSoundStream(void) {
    }

    void instNuGCutLocatorUpdate(instNUGCUTSCENE_s *instance, NUGCUTLOCATORSYS_s *system,
                                 instNUGCUTLOCATOR_s *inst_locator, NUGCUTLOCATOR_s *locator, f32 frame,
                                 NUMTX *parent_mtx, i32 paused) {
        NUGCUTLOCATORTYPE_s *type = &system->types[locator->type_index];
        i32 effect_index = static_cast<i32>(static_cast<u16>(type->function_index));
        debinftype *effect =
            debtab != NULL && effect_index >= 0 && effect_index != 0xffff ? debtab[effect_index] : NULL;

        if ((type->flags & 1) != 0) {
            if (paused != 0 || effect == NULL) {
                return;
            }
            nuanimtime_s time;
            f32 scale = 0.0f;
            f32 rate = 0.0f;
            if (locator->animation != NULL) {
                NuAnimData2CalcTime(locator->animation, frame, &time);
            }
            i32 visible = NuGCutLocatorIsVisble(locator, frame, &time, &scale, &rate);
            if (visible != 0 && scale < 0.0f && (locator->flags & 0x10) == 0) {
                scale = static_cast<f32>(effect->frequency) * -scale;
            }
            if (visible != 0 && scale > 0.1f) {
                if ((locator->flags & 0x10) != 0 && inst_locator->effect_handle != 0) {
                    return;
                }
                NUMTX matrix;
                NuGCutLocatorCalcMtx(locator, frame, &matrix, &time);
                if ((locator->flags & 4) != 0) {
                    NuMtxPreTranslate(&matrix, &locator->pivot);
                }
                if (parent_mtx != NULL) {
                    NuMtxMul(&matrix, &matrix, parent_mtx);
                }
                if ((locator->flags & 0x10) == 0) {
                    if ((locator->flags & 0x20) == 0) {
                        AddScaledVariableShotDebrisEffect2(effect_index, reinterpret_cast<NUVEC *>(&matrix.m30),
                                                           static_cast<i32>(scale), timeincrement, &matrix, 0, rate);
                    } else {
                        if (inst_locator->effect_handle == -1) {
                            AddDebrisEffect(&inst_locator->effect_handle, effect_index, matrix.m30, matrix.m31,
                                            matrix.m32);
                            DebrisPopulateInstance(inst_locator->effect_handle, 0.0f);
                        }
                        if (inst_locator->effect_handle >= 0) {
                            if (processdeb == 1) {
                                DebrisEmitterPos(inst_locator->effect_handle, matrix.m30, matrix.m31, matrix.m32);
                            } else if (processdeb == 2) {
                                DebrisOrientationMtx(inst_locator->effect_handle, &matrix);
                            } else {
                                DebrisPosOrientationMtx(inst_locator->effect_handle, &matrix);
                            }
                        }
                        if (glyntestha > 0.0f) {
                            NuRndrAxisArrowsMtx(&matrix, glyntestha, 0);
                        }
                    }
                } else {
                    i32 finite_handle = -1;
                    AddScaledFiniteShotDebrisEffect(&finite_handle, effect_index,
                                                    reinterpret_cast<NUVEC *>(&matrix.m30), 0, 0, 1, rate);
                    DebrisEmitterOrientationMtx(finite_handle, &matrix);
                    inst_locator->effect_handle = 1;
                }
                return;
            }
            if ((locator->flags & 0x20) == 0) {
                inst_locator->effect_handle = 0;
            } else if (inst_locator->effect_handle >= 0) {
                DebFreeInstantly(&inst_locator->effect_handle);
            }
            inst_locator->field_00 = 0;
            return;
        }

        if ((type->flags & 2) != 0) {
            if (effect_index >= 0 && locatorfns != NULL) {
                NUGCUTLOCATORFN fn = locatorfns[effect_index].function;
                if (fn != NULL) {
                    fn(instance, system, inst_locator, locator, frame, parent_mtx, paused);
                    return;
                }
            }
        } else if ((type->flags & 0x10) != 0 && paused == 0) {
            if (TriggerLocatorVfxFn == NULL || ReleaseLocatorVfxFn == NULL || UpdateLocatorVfxFn == NULL) {
                return;
            }
            f32 opacity = 0.0f;
            nuanimtime_s time;
            if (locator->animation != NULL) {
                NuAnimData2CalcTime(locator->animation, frame, &time);
            }
            if (NuGCutLocatorIsVisble(locator, frame, &time, &opacity, NULL) == 0 || opacity <= 0.1f) {
                if (inst_locator->effect_handle >= 0) {
                    ReleaseLocatorVfxFn(inst_locator->effect_handle);
                }
                inst_locator->effect_handle = -1;
            } else {
                NUMTX matrix;
                NuGCutLocatorCalcMtx(locator, frame, &matrix, &time);
                if ((locator->flags & 4) != 0) {
                    NuMtxPreTranslate(&matrix, &locator->pivot);
                }
                if (parent_mtx != NULL) {
                    NuMtxMul(&matrix, &matrix, parent_mtx);
                }
                f32 values[16];
                memcpy(values, &matrix, sizeof(values));
                if (inst_locator->effect_handle < 0) {
                    inst_locator->effect_handle = TriggerLocatorVfxFn(static_cast<u16>(effect_index), values);
                } else {
                    UpdateLocatorVfxFn(inst_locator->effect_handle, values);
                }
            }
        } else if ((type->flags & 4) != 0 && NuCutSceneSFXUpdate != NULL) {
            NuCutSceneSFXUpdate(system, inst_locator, locator, frame, parent_mtx, paused);
        }
    }

} // extern "C"
