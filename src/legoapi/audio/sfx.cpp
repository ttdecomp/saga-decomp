#include "legoapi/world/world_shared.h"
#include "legoapi/audio/audio.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/characters/core/charconfig.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/world/area.h"
#include "legoapi/world/mission.h"
#include "legoapi/props/system/socksys.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numusic/numusic.h"
#include "nu2api/numusic/sfx.h"
#include "nu2api/nusound/nusound.h"
#include "nu2api/nufile/nufile.h"
#include "decomp.h"

#include <stdio.h>
#include <string.h>

enum RepeatSfxState : u8 {
    REPEAT_SFX_INACTIVE = 0,
    REPEAT_SFX_INITIAL_DELAY = 1,
    REPEAT_SFX_PLAY = 2,
    REPEAT_SFX_INTERVAL = 3,
};

struct RepeatSfx {
    i16 sfx_id;
    RepeatSfxState state;
    i8 plays_remaining;
    f32 timer;
    f32 interval;
    nuvec_s *position;
};

DECOMP_ASSERT(sizeof(RepeatSfx) == 0x10, "RepeatSfx size");

static i32 repsfxcount;
static RepeatSfx repsfxtab[32];
static i32 ticktock;
static f32 MusicVolume = 1.0f;
static f32 CutVolume = 0.8f;

extern "C" {
    u16 GlobalSfxBits[100];
    u16 SfxBits[100];
    SoundTable CurrentSFXTAB;
    void (*ExtraDieSfxFn)(GameObject_s *);
    void (*ExtraHurtSfxFn)(GameObject_s *);
    i32 CruiserD_LiftChase;
}

i32 GroupBuffer_GetSample(i32 group_id, i32 sequential);
i32 GroupBuffer_GetNumInGroup(i32 group_id);
i32 GroupBuffer_GetSampleByIndex(i32 group_id, i32 sample_index);
void PlayAMusic(i32 stream, i32 track, i32 volume, i32 one_shot);

extern "C" void NuGCutSetCutAudioStream(i32 stream);

bool HandleGroupLimit(i32 group_id) {
    i32 voice_count = 0;
    NuSoundVoice *oldest_voice = NULL;
    f32 oldest_position = -1.0f;

    i32 sample_count = GroupBuffer_GetNumInGroup(group_id);
    for (i32 i = 0; i < sample_count; i++) {
        i32 sfx_id = GroupBuffer_GetSampleByIndex(group_id, i);
        i32 sample_index = g_soundInfo[sfx_id].index;
        voice_count += NuSound3CountVoices(sample_index);

        f32 playback_position = 0.0f;
        NuSoundVoice *voice = NuSound3FindOldestVoice(sample_index, &playback_position);
        if (voice != NULL && oldest_position < playback_position) {
            oldest_position = playback_position;
            oldest_voice = voice;
        }
    }

    if (voice_count < g_NuSoundMaxVoicesPerSample) {
        return true;
    }

    i32 first_sfx = GroupBuffer_GetSampleByIndex(group_id, 0);
    if (g_soundInfo[first_sfx].field29_0x40 == 1) {
        NuSound3StopVoice(oldest_voice);
        return true;
    }
    return false;
}

extern "C" void PlaySfxByIdEx(i32 sfx_id, nuvec_s *position, f32 volume, f32 pitch);
extern "C" void PlaySfxById(i32 sfx_id, nuvec_s *position);
void GameAudio_PlaySfxById(i32 sfx_id, nuvec_s *position, i32 flags, i32 volume);
void GameAudio_PlaySfx(i32 sfx, nuvec_s *position, i32 flags, i32 volume);
i32 GameAudio_GetPlrSfxBits(void *object);
void GameAudio_AddSfx(i32 sfx, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx);
void SetSfxBit_OnEx(i32);
void SetSfxBit_OffEx(i32);
void SetSfxBitTab_OnEx(SoundTable *, i32);
void SetSfxBitTab_OffEx(SoundTable *, i32);
i32 SfxBitEx(i32);
i32 SfxBitTabEx(const SoundTable *, i32);
void AddLevelSfxFromName(char *sfx_name, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx_count);
void AddLevelSfxGizmoSys(GIZMOSYS_s *gizmo_sys, void *world_info, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx_count);
void SetSpecialSfxBits(i32 *sfx_ids, i32 *sfx_count, WORLDINFO_s *world);
int SpecialSfxLoad(char *path, WORLDINFO_s *world);
void SetupBlowupSfx(WORLDINFO_s *world, specialsfx_s *special_sfx);
void Pulses_AddSfx(PULSESYS_s *system, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx_count);
void Move_BEAST(GameObject_s *object);
void PlayFootStepSfx(GameObject_s *object);
i32 qrand(void);
void GameCam_NewShake(GAMECAMERA_s *camera, f32 amount, f32 duration, f32 speed);
void GameCam_Judder(GAMECAMERA_s *camera, f32 amount, i32 axis, nuvec_s *source);
void NewRumbleAllPlayers(f32 intensity, f32 duration, i32 flags, i32 player_index);
extern "C" TERRAIN_SURFACE_s TerSurface[32];
i32 Players_AveragePos(nuvec_s *position, SOCKPOSITION_s *socket_position);
i32 Hub_Outside(void);
i32 KaminoInside(void);
i32 KaminoDiscoOn(void);
bool DeathStarShieldDown(void);
bool SarlaccPitDiscoActive(WORLDINFO_s *world);

extern AREADATA *BONUS_GUNSHIP_ADATA;
extern AREADATA *DAGOBAH_ADATA;
extern AREADATA *GUNSHIP_ADATA;
extern AREADATA *HOTHESCAPE_ADATA;
extern AREADATA *JABBASPALACE_ADATA;
extern AREADATA *NEWTOWN_ADATA;
extern AREADATA *PODRACE_ADATA;
extern AREADATA *PODSPRINT_ADATA;
extern AREADATA *SPEEDERCHASE_ADATA;
extern i16 id_CHEWBACCA;
extern i16 id_EWOK;
extern i16 id_GAMORREANGUARD;
extern i16 id_WICKET;
extern f32 chattersfxwait;
extern i32 DoubleScore;

static GAMEAUDIO GameAudio_Default;
static GAMEAUDIO *GameAudio = &GameAudio_Default;
extern "C" void MenuRegisterSoundFX(i32, i32, i32, i32);
i32 GameAudio_GetSfxId(i32);

extern "C" {
    void SetSfxBit_On(i32 sound);
    void SetSoundBitsById(const i32 *sound_ids, void (*set_bit)(i32));
    void SfxBitsStore(SoundTable *table);
    void MusicPreSeek(i32 track);
    void NuSound3FlushLoops(void);
    i32 NuSound3SetReverb(i32 mode);
}

i32 ActionFromQuiet(i32 idx) {
    static i16 ActionPairTab[14] = {-1};
    if (idx != -1) {
        i16 *pair = ActionPairTab;
        while (*pair != -1) {
            if (*pair == idx) {
                return pair[1];
            }
            pair += 14;
        }
    }
    return -1;
}
i32 AmbientFromQuiet(i32 idx) {
    static i16 AmbientPairTab[2] = {-1};
    if (idx != -1) {
        i16 *pair = AmbientPairTab;
        while (*pair != -1) {
            if (*pair == idx) {
                return pair[1];
            }
            pair += 2;
        }
    }
    return -1;
}

extern "C" void ResetSounds(void) {
    memcpy(SfxBits, GlobalSfxBits, sizeof(SfxBits));
}

extern "C" void PrepareSounds(const u16 *sounds) {
    for (i32 i = 0; i < 100; ++i) {
        SfxBits[i] |= sounds[i];
    }
}

extern "C" void MaskSounds(const u16 *mask) {
    for (i32 i = 0; i < 100; ++i) {
        SfxBits[i] &= mask[i];
    }
}

void SetLevelSfxBits(WORLDINFO *world) {
    i32 sfx_ids[1024];
    i32 sfx_count = 0;

#define ADD_SFX(name) AddLevelSfxFromName(name, sfx_ids, &sfx_count, 0x400)
#define ADD_GAME_SFX(id) GameAudio_AddSfx(id, sfx_ids, &sfx_count, 0x400)

    for (i32 i = 0; i < world->level_sfx_count; ++i) {
        sfx_ids[sfx_count++] = world->level_sfx[i].id;
    }

    if (world->spinner_count > 0) {
        ADD_SFX("fly_paddle_rotate_lp");
        ADD_SFX("fly_paddle_stuck");
    }
    if (world->teleport_count > 0) {
        ADD_SFX("env_door_flap");
    }

    ADD_GAME_SFX(0x50);
    ADD_GAME_SFX(0x51);
    ADD_GAME_SFX(0x52);
    ADD_SFX("Grv_GuardWeaponLp");

    if (world->grabber != NULL) {
        if (world->current_level == CLOUDCITYTRAPA_LDATA) {
            ADD_SFX("CarbonFreezeCraneLp");
            ADD_SFX("CarbonFreezeCrane");
        } else if (world->current_level == JABBASPALACEB_LDATA) {
            ADD_SFX("env_hover_box_lp");
        } else {
            ADD_SFX("env_crane_mvt_lp");
        }
        ADD_SFX("env_crane_in");
        ADD_SFX("env_grabber_down");
        ADD_SFX("env_grabber_pickup");
        ADD_SFX("env_grabber_up");
        ADD_SFX("Explode2");
        if (world->current_level == CLOUDCITYESCAPEC_LDATA) {
            ADD_SFX("imp_C3PO_magnet");
            ADD_SFX("imp_C3PO_magnet_drop");
        }
    }

    if (world->cutscene_sys != NULL) {
        for (i32 i = 0; i < world->cutscene_sys->count; ++i) {
            CUTINFO *cut = world->cutscene_sys->cuts[i];
            for (CUTSCENESFX &sfx : cut->sfx) {
                if (sfx.id != -1) {
                    sfx_ids[sfx_count++] = sfx.id;
                }
            }
        }
    }

    Pulses_AddSfx(world->pulses_sys, sfx_ids, &sfx_count, 0x400);
    if (world->hat_machine_sys != NULL && world->hat_machine_sys->count > 0) {
        ADD_SFX("SwLever");
        ADD_SFX("HatOn");
    }
    if (world->nlevers > 0) {
        ADD_SFX("SwLever");
    }

    if (world->area == NULL || (world->area->flags & 1) == 0) {
        ADD_SFX("Char_Slide_Lp");
        if (world->push_block_count > 0) {
            ADD_SFX("Block_Shove");
            ADD_SFX("Block_Push_Lp");
        }
        ADD_SFX("exp_thermalDet");
        ADD_SFX("imp_thermalDet_attach");
        if (world->area != NULL && world->area == SPEEDERCHASE_ADATA) {
            ADD_SFX("XWing_LoopDeLoop");
        }
    } else {
        ADD_SFX("XWing_Torpedo");
        ADD_SFX("imp_proton_torp");
        ADD_SFX("XWing_LoopDeLoop");
    }
    ADD_SFX("Explode1");

    LEVELDATA *level = world->current_level;
    if (level != NULL) {
        if (level->unknown_0a2 != -1) {
            sfx_ids[sfx_count++] = level->unknown_0a2;
        }
        if (level == STATUS_LDATA || (level->flags & LEVEL_STATUS) != 0) {
            ADD_SFX("StatusAward");
            ADD_SFX("Status_GoldBarDec");
            ADD_SFX("TrueJedi_100pc");
            ADD_SFX("TrueJedi_NOT");
            ADD_SFX("MK-Panel");
            ADD_SFX("Char_Icon_App");
            ADD_SFX("Char_Icon_Slide");
            ADD_SFX("LegoClicks");
            ADD_SFX("Shop_BuyCheat");
            ADD_SFX("Explode1");
            ADD_SFX("Jp_Ana_Jump");
            ADD_SFX("PickupCoin");
        } else if (world->area != NULL && (world->area->flags & 4) != 0) {
            ADD_SFX("Victory");
            ADD_SFX("exp_debris");
        } else if (level == CREDITS_LDATA) {
            ADD_SFX("StatusAward");
        }
    } else if (world->area != NULL && (world->area->flags & 4) != 0) {
        ADD_SFX("Victory");
        ADD_SFX("exp_debris");
    }

    if (world->area != NULL) {
        if (world->area->super_counter_count != 0) {
            ADD_GAME_SFX(0x53);
        }
        if (world->area == PODRACE_ADATA || world->area == PODSPRINT_ADATA) {
            ADD_SFX("PodX_TuskenBlast");
            ADD_SFX("Pod_TuskHit");
            ADD_SFX("PodX_EngSebulba_Lp");
            ADD_SFX("PodX_EngAnakin_Lp");
            ADD_SFX("PodX_EngGeneric1_Lp");
            ADD_SFX("PodX_EngGeneric2_Lp");
            ADD_SFX("PodX_EngGeneric3_Lp");
            ADD_SFX("PodX_EngStartup1");
            ADD_SFX("PodX_EngStartup2");
            ADD_SFX("Pod_Race_Go");
            ADD_SFX("Pod_Race_Light");
            ADD_SFX("PodX_EngGeneric_Lp");
            ADD_SFX("PodX_Collide");
            ADD_SFX("PodX_Crash");
            ADD_SFX("PodX_Booster");
            ADD_SFX("CountdownTimerTick");
            ADD_SFX("CountdownTimerTock");
            ADD_SFX("PodX_PurpCrysHit");
        } else if (world->area == GUNSHIP_ADATA || world->area == BONUS_GUNSHIP_ADATA) {
            ADD_SFX("GC_GunshipBlasterFire");
            ADD_SFX("GC_GunshipEngineLp");
            ADD_SFX("GC_GunshipDeath");
            ADD_SFX("GC_LaserBeam");
            ADD_SFX("Explode1");
        } else if (world->area == HUB_ADATA) {
            ADD_SFX("Shop_BuyCheat");
            ADD_SFX("Shop_NotEnufMuny");
            ADD_SFX("ui_ZoomIn");
            ADD_SFX("ui_ZoomOut");
            ADD_SFX("ui_hover_lp");
        } else if (world->area == JABBASPALACE_ADATA) {
            ADD_SFX("swdisco");
        } else if (world->area == NEWTOWN_ADATA) {
            ADD_SFX("BBounce");
        }
    }

    level = world->current_level;
    if (level != NULL) {
        if (level == NEGOTIATIONSA_LDATA) {
            ADD_SFX("NegA_CoinMDoorUp");
            ADD_SFX("NegA_CoinMDoorDn");
        } else if (level == NEGOTIATIONSB_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("NegC_Crate");
        } else if (level == NEGOTIATIONSC_LDATA) {
            ADD_SFX("NegC_MagnetCoil");
            ADD_SFX("NegC_MagnetCoilH");
            ADD_SFX("NegC_MagnetCoilS");
            ADD_SFX("FField");
            ADD_SFX("NegC_Crate");
        } else if (level == GUNGAN_A_LDATA) {
            ADD_SFX("GunA_TreeFall");
            ADD_SFX("FS_CreaRun");
            ADD_SFX("IkopiGrowl");
            ADD_SFX("KaaduBark");
            ADD_SFX("KaaduGrowl");
        } else if (level == RESCUEC_LDATA) {
            ADD_SFX("ResX_MarbColLp");
            ADD_SFX("ResX_PoleLift_Rise");
            ADD_SFX("ResX_PoleLift_Lower");
        } else if (level == RETAKEB_LDATA) {
            ADD_SFX("ResX_MarbColLp");
        } else if (level == RETAKED_LDATA) {
            ADD_SFX("ResX_PortCulLp");
            ADD_SFX("ResX_PortCulEnd");
            ADD_SFX("Ep1_5_OutdoorPlatform");
        } else if (level == RETAKEE_LDATA) {
            ADD_SFX("Ep1_5_OutdoorPlatform");
            ADD_SFX("ResX_PortCulLp");
            ADD_SFX("ResX_PortCulEnd");
        } else if (level == MAULA_LDATA) {
            ADD_SFX("Ep1_7_BDroidPlatf");
            ADD_SFX("Ep1_7_SFPowerUp");
            ADD_SFX("Ep1_7_SFHoverLp");
            ADD_SFX("Ep1_7_SFTakeOff");
        } else if (level == MAULD_LDATA) {
            ADD_SFX("Ep1_7_FloatPlatRise");
            ADD_SFX("NegC_Crate");
        } else if (level == KAMINOE_LDATA) {
            ADD_SFX("Kam_Slave1BlasterFire");
            ADD_SFX("Slave1_EngineLp");
        } else if (level == KAMINOC_LDATA) {
            ADD_SFX("Kam_DiscoFloorPanelOn");
            ADD_SFX("Kam_DiscoFloorPanelDone");
            ADD_SFX("Kam_SparkLp");
            ADD_SFX("Kam_SparkLp");
            ADD_SFX("Kam_ForceFieldLp");
            ADD_SFX("Kam_ForceFieldOff");
        } else if (level == FACTORYB_LDATA) {
            ADD_SFX("FacB_BeltLp");
            ADD_SFX("FacB_ConvStop");
            ADD_SFX("Fac_StampHydros");
            ADD_SFX("Fac_StampImpacts");
            ADD_SFX("Fac_BonusCylUp");
            ADD_SFX("Fac_BonusBeep");
        } else if (level == FACTORYD_LDATA) {
            ADD_SFX("Fac_BucketGearUD");
            ADD_SFX("Fac_BucketAwayFB");
            ADD_SFX("Fac_BucketLp");
            ADD_SFX("Fac_BucketArriveLock");
            ADD_SFX("Fac_BucketOpens");
        } else if (level == FACTORYF_LDATA) {
            ADD_SFX("Fac_LaserGateLoop");
        } else if (level == FACTORYG_LDATA) {
            ADD_SFX("Fac_ObiRestraintLoop");
        } else if (level == GUNSHIPB_LDATA) {
            ADD_SFX("CountdownTimerTick");
            ADD_SFX("CountdownTimerTock");
        } else if (level == DOOKUC_LDATA) {
            ADD_SFX("Dooku_LightningLp");
        } else if (level == DOGFIGHTA_LDATA) {
            ADD_SFX("Ep3_1_StarDestEngLp");
            ADD_SFX("Ep3_1_ProtoXWingMissile");
            ADD_SFX("Ep3_1_ExplosionM");
            ADD_SFX("Ep3_1_ExplosionXXL");
            ADD_SFX("Dog_TurretFire");
            ADD_SFX("Dog_CloneARC170Gun");
            ADD_SFX("Dog_JediStFighterGun");
            ADD_SFX("Dog_JediStFighterGun2");
            ADD_SFX("Dog_JediStFighterGun3");
            ADD_SFX("Dog_JediStFighterEngLp");
            ADD_SFX("Dog_CloneARC170EngLp");
            ADD_SFX("Dog_DroidFighterBlast");
            ADD_SFX("Dog_TriFighterGuns");
            ADD_SFX("Dog_TriFighterGuns2");
            ADD_SFX("Dog_DroidFighterEngLp");
            ADD_SFX("Dog_TriFighterEngLp");
            ADD_SFX("Dog_DroidFighterHit");
            ADD_SFX("Dog_TriFighterHit");
            ADD_SFX("Dog_HugeBeamGunFire");
            ADD_SFX("Dog_HugeBeamGunLp");
            ADD_SFX("Dog_SepShieldPwrDown");
            ADD_SFX("Dog_ShipBreakExplo");
            ADD_SFX("Dog_ShipDoorOpen");
            ADD_SFX("Dog_StDestTurretSpins");
            ADD_SFX("Dog_TowerBreakExplo");
        } else if (level == CRUISERD_LDATA) {
            ADD_SFX("Cru_HugeWallMoveLp");
        } else if (level == CRUISERG_LDATA) {
            ADD_SFX("NegA_CoinMDoorUp");
            ADD_SFX("NegA_CoinMDoorDn");
        } else if (level == KASHYYYKD_LDATA) {
            ADD_SFX("Kas_BoulderLoop");
            ADD_SFX("Kas_BoulderExplo");
        } else if (level == VADERA_LDATA) {
            ADD_SFX("CountdownTimerTick");
            ADD_SFX("CountdownTimerTock");
        } else if (level == TATOOINEA_LDATA) {
            ADD_SFX("env_shower_lp");
        } else if (level == TATOOINEB_LDATA) {
            ADD_SFX("greenlighton");
            ADD_SFX("env_suckerspit_3PO");
            ADD_SFX("env_suckerspit_gonk");
            ADD_SFX("conveyorlp");
        } else if (level == MOSEISLEYA_LDATA) {
            ADD_SFX("Landspeeder_EngineLp");
            ADD_SFX("env_jacuzzi_lp");
        } else if (level == MOSEISLEYC_LDATA) {
            ADD_SFX("Droid_BeamLp");
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
            ADD_SFX("SwPPad");
        } else if (level == DEATHSTARRESCUEB_LDATA) {
            ADD_SFX("env_tractorbeam_off");
            ADD_SFX("env_tractorbeam_lp");
        } else if (level == DEATHSTARRESCUEC_LDATA) {
            ADD_SFX("ffield");
        } else if (level == DEATHSTARRESCUEE_LDATA) {
            ADD_SFX("Turret_PitchLp");
            ADD_SFX("Turret_YawLp");
            ADD_SFX("Turbo_Fire");
        } else if (level == DEATHSTARESCAPEA_LDATA) {
            ADD_SFX("Dianoga_Groan");
            ADD_SFX("Dianoga_Roar");
        } else if (level == DEATHSTARESCAPED_LDATA) {
            ADD_SFX("Dianoga_Groan");
            ADD_SFX("Dianoga_Roar");
        } else if (level == DEATHSTARESCAPEB_LDATA) {
            ADD_SFX("SqueakWash");
        } else if (level == DEATHSTARESCAPEC_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == DEATHSTARBATTLEA_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == DEATHSTARBATTLEB_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == DEATHSTARBATTLEC_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == DEATHSTARBATTLED_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == HOTHESCAPEA_LDATA) {
            ADD_SFX("ThermalDet_Beep");
            ADD_SFX("env_padLight_on");
        } else if (level == HOTHESCAPEC_LDATA) {
            ADD_SFX("env_padLight_on");
        } else if (level == ASTEROIDCHASED_LDATA) {
            ADD_SFX("exp_asteroid");
        } else if (level == CLOUDCITYTRAPA_LDATA) {
            ADD_SFX("Env_Steam_Lp");
        } else if (level == CLOUDCITYESCAPEA_LDATA) {
            ADD_SFX("env_steam_lp");
            ADD_SFX("Env_ctrl_desk_on");
        } else if (level == CLOUDCITYESCAPEC_LDATA) {
            ADD_SFX("env_steam_lp");
        } else if (level == DEATHSTAR2BATTLEB_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == DEATHSTAR2BATTLED_LDATA) {
            ADD_SFX("ForceLightningLp");
        } else if (level == JABBASPALACEA_LDATA) {
            ADD_SFX("SwDisco");
            ADD_SFX("Leia_Blaster");
        } else if (level == JABBASPALACEB_LDATA) {
            ADD_SFX("SwDisco");
            ADD_SFX("Leia_Blaster");
        } else if (level == JABBASPALACED_LDATA) {
            ADD_SFX("SwDisco");
            ADD_SFX("Leia_Blaster");
        } else if (level == JABBASPALACEE_LDATA) {
            ADD_SFX("exp_minecart");
        } else if (level == SARLACCPITB_LDATA) {
            ADD_SFX("env_curtain_lp");
            ADD_SFX("Kam_DiscoFloorPanelOn");
            ADD_SFX("Kam_DiscoFloorPanelDone");
        } else if (level == SARLACCPITC_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
            ADD_SFX("imp_c3po_magnet_drop");
            ADD_SFX("env_magnet_on");
        } else if (level == SPEEDERCHASEA_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == ENDORBATTLEA_LDATA) {
            ADD_SFX("env_lantern_lp");
        } else if (level == ENDORBATTLEB_LDATA) {
            ADD_SFX("waterfall");
            ADD_SFX("drd_r2_mvt_water_lp");
            ADD_SFX("env_lantern_lp");
        } else if (level == ENDORBATTLED_LDATA) {
            ADD_SFX("FField");
            ADD_SFX("FFieldOff");
        } else if (level == EMPERORFIGHTA_LDATA) {
            ADD_SFX("env_block_light_on");
            ADD_SFX("env_padlight_on");
        }
    }

    if (VehicleArea != 0) {
        ADD_SFX("TieDoorsOpen");
    }
    if (Mission_Active(NULL) != NULL) {
        ADD_SFX("Victory");
    }
    if (Arcade != 0) {
        ADD_SFX("env_padLight_on");
    }
    if (world->area == HOTHESCAPE_ADATA || world->area == JABBASPALACE_ADATA) {
        ADD_SFX("fs_ice");
    } else if (world->area == DAGOBAH_ADATA) {
        ADD_SFX("fs_swamp");
    }

    level = world->current_level;
    if (apicharsys->loaded_model_count > 0 && level != NULL && (level->flags & 2) != 0 && VehicleArea == 0) {
        ADD_GAME_SFX(0x14);
        ADD_GAME_SFX(0x15);
    }

    for (i32 character_id = 0; character_id < apicharsys->character_count; ++character_id) {
        i16 model_id = apicharsys->playermodelids[character_id];
        if (model_id == -1 || (apicharsys->models[model_id].flags & 1) == 0) {
            continue;
        }

        CHARACTERDATA *character = &CDataList[character_id];
        GAMECHARACTERDATA *game_character = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);

        if (game_character->sfx_die != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_die;
        } else if ((character->model_flags & 0x44002010) == 0) {
            ADD_GAME_SFX((static_cast<i32>(game_character->flags_090 << 14) >> 31) + 0x1c);
        }
        if (game_character->sfx_hurt != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_hurt;
        } else if ((character->model_flags & 0x44002010) == 0) {
            ADD_GAME_SFX((static_cast<i32>(game_character->flags_090 << 14) >> 31) + 0x17);
        }
        if (game_character->sfx_grunt != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_grunt;
        } else if ((character->model_flags & 0x44002010) == 0) {
            ADD_GAME_SFX((static_cast<i32>(game_character->flags_090 << 14) >> 31) + 0x15);
        }

        if (game_character->sfx_engine != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_engine;
        }
        if (game_character->sfx_shoot != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_shoot;
        }
        if (game_character->sfx_footstep != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_footstep;
        }
        if (game_character->sfx_chatter != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_chatter;
        }
        if (game_character->sfx_sabre != -1) {
            sfx_ids[sfx_count++] = game_character->sfx_sabre;
        }
        for (i32 i = 0; i < 6 && game_character->sfx_misc[i] != -1; ++i) {
            sfx_ids[sfx_count++] = game_character->sfx_misc[i];
        }

        if ((character->model_flags & 0x40) != 0) {
            ADD_SFX("drd_r2_scope_up");
            ADD_SFX("drd_r2_scope_down");
            ADD_SFX("drd_r2_mvt_water_lp");
        }
        if ((game_character->flags_090 & 0x400) != 0) {
            ADD_SFX("TowCable_Fire");
            ADD_SFX("TowCable_Latch");
            ADD_SFX("TowCable_Detach");
            ADD_SFX("TowCable_Snap");
        }
        if ((character->model_flags & 0x2000) != 0) {
            ADD_SFX("XWing_Torpedo");
            ADD_SFX("env_tractorbeam_lp");
            if ((character->model_flags & 0x04000000) == 0) {
                ADD_SFX("Explode1");
            }
        }
        if (character_id == id_CHEWBACCA) {
            ADD_SFX("C3_Hurt");
            ADD_SFX("C3_Death");
        }
        if (character->move_fn == Move_BEAST) {
            ADD_SFX("Lego_Poo");
            ADD_SFX("Lego_PLOP");
            ADD_SFX("FliesLp");
        }
        if (game_character->uses_weapon_action == 10) {
            ADD_SFX("veh_tie_by");
            ADD_SFX("Tie_Spins");
        }
        if (character_id == id_EWOK || character_id == id_WICKET) {
            ADD_SFX("wpn_bomb_drop");
            ADD_SFX("exp_bomb");
        }
        if ((game_character->flags_090 & 4) != 0) {
            ADD_SFX("ForceLightningLp");
        } else if ((game_character->flags_090 & 2) != 0) {
            ADD_SFX("ForceChokeCrunch");
        } else if ((character->model_flags & 8) != 0) {
            ADD_SFX("ForceMindTrick");
        }
        if (game_character->uses_weapon_action == 12 && (character->model_flags & 8) != 0 && id_GAMORREANGUARD != -1 &&
            apicharsys->playermodelids[id_GAMORREANGUARD] != -1) {
            ADD_SFX("ForceChokeCrunch");
        }
        if ((game_character->flags_094[3] & 0x20) != 0) {
            ADD_GAME_SFX(0x4c);
            ADD_GAME_SFX(0x4d);
            ADD_GAME_SFX(0x4e);
        }

        CHARACTER_EFFECT_s *effect = character->effects;
        if (effect != NULL) {
            while (effect->character_id != -1) {
                if (effect->sound_id != -1) {
                    sfx_ids[sfx_count++] = effect->sound_id;
                }
                ++effect;
            }
        }
        if ((character->model_flags & 0x20) != 0) {
            ADD_SFX("TC14_VLA");
            ADD_SFX("TC14_VLN");
        }
        if ((character->model_flags & 0x40) != 0) {
            ADD_SFX("R2D2_VLA");
        }
    }

    level = world->current_level;
    bool double_score_sfx = Arcade != 0 || (level != NULL && (level->flags & 0x800) != 0);
    if (!double_score_sfx) {
        for (i32 portal = 0x13; portal <= 0x17; ++portal) {
            if (world->portal_places[portal] != NULL) {
                double_score_sfx = true;
                break;
            }
        }
    }
    if (double_score_sfx) {
        ADD_SFX("ui_DoubleScoreEntry");
        ADD_SFX("ui_DoubleScoreText");
    }

    AddLevelSfxGizmoSys(world->gizmo_sys, world, sfx_ids, &sfx_count, 0x400);
    SetSpecialSfxBits(sfx_ids, &sfx_count, world);
    sfx_ids[sfx_count] = -1;
    if (sfx_count > 0) {
        SetSoundBitsById(sfx_ids, SetSfxBit_On);
    }
    SfxBitsStore(&CurrentSFXTAB);

#undef ADD_GAME_SFX
#undef ADD_SFX
}
void ResetLevSfx(WORLDINFO *world) {
    for (i32 i = 0; i < 0x40; i++) {
        world->level_sfx[i].id = -1;
    }
    world->level_sfx_count = 0;
}

bool InitSpecialSfx(WORLDINFO *world) {
    bool result = false;
    if (world != NULL) {
        world->special_sfx_count = 0;
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        world->special_sfx = reinterpret_cast<specialsfx_s *>(world->giz_buffer.void_ptr);
        world->giz_buffer.addr += 0xf00;
        memset(world->special_sfx, 0, 0xf00);

        world->special_sfx_event_count = 0;
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 0x10);
        world->special_sfx_events = reinterpret_cast<SPECIALSFXEVENT_s *>(world->giz_buffer.void_ptr);
        world->giz_buffer.addr += 0xc00;
        memset(world->special_sfx_events, 0, 0xc00);
        result = true;
    }
    return result;
}
void LoadSpecialSfxFile(WORLDINFO *world) {
    char path[268];
    sprintf(path, "%s.sfx", world->config_file);
    if (NuFileExists(path) != 0 && SpecialSfxLoad(path, world) != 0) {
        for (i32 i = 0; i < world->special_sfx_count; i++) {
            if ((world->special_sfx[i].flags & 0xf) == 1) {
                SetupBlowupSfx(world, &world->special_sfx[i]);
            }
        }
    }
}

extern "C" {

    f32 sfx_wait;

    i32 GetSfxIdN(char *name, i32 length) {
        for (i32 index = 0; index < 1600; ++index) {
            if (NuStrNICmp(name, g_soundInfo[index].sfx_name, length) == 0) {
                return index;
            }
        }
        return -1;
    }

    char *GetSfxName(i32 sfx_id) {
        return sfx_id < 0 ? NULL : const_cast<char *>(g_soundInfo[sfx_id].sfx_name);
    }

    i32 IsSfxLooping(i32 sfx_id) {
        return sfx_id == -1 ? -1 : g_soundInfo[sfx_id].loop;
    }

    void PauseGameAudio(void) {
        if (NOSOUND == 0) {
            NuSound3StopSFX();
            NuSound3SetSFXPitch(0);
        }
    }

    void PauseGameMusic(void) {
        if (NOMUSIC == 0 && NOSOUND == 0 && NUSOUND_STREAM_3 != -1 && static_cast<u16>(Music.state - 11) > 2) {
            NuSound3CancelCheckStereo();
            NuSound3PauseStereoStream(Music.primary_stream);
            if (static_cast<u16>(Music.state - 5) < 6) {
                NuSound3PauseStereoStream(1 - Music.primary_stream);
            }
        }
        Music.restore_requested = false;
    }

    void PauseGameSfx(void) {
        NuSound3StopSFX();
        NuSound3FlushLoops();
        NuSound3SetSFXPitch(0);
    }

    void PlayAltGameMusic(i32 track) {
        Music.update_delay = 1;
        if (Music.requested_track == track && !Music.pause_requested) {
            i32 stream = 1 - Music.primary_stream;
            NuSound3ResumeStereoStream(stream);
            NuSound3SetStereoStreamVolume(stream, static_cast<i32>(g_music[track].index * MusicVolume));
            Music.resume_track = -1;
        } else {
            PlayAMusic(1 - Music.primary_stream, track, static_cast<i32>(g_music[track].index * MusicVolume), 0);
        }
    }

    i32 PlayCutMusic(i32 track, i32 state, void *context) {
        if (NOSOUND != 0 || NOMUSIC != 0 || track < 0 || track >= SFX_MUSIC_COUNT) {
            return 0;
        }

        Music.queued_track = static_cast<i16>(track);
        Music.transition_frames = 0;

        const i32 stream = 1 - Music.primary_stream;
        reinterpret_cast<u8 *>(&Music)[0x12 + stream] = 6;
        NuSound3CancelCheckStereo();
        NuSound3PauseStereoStream(Music.primary_stream);

        const i32 volume = static_cast<i32>(static_cast<f32>(g_music[track].index) * CutVolume);
        NuSound3SetStereoStreamVolume(Music.primary_stream, volume);

        i32 started = 0;
        if (Music.requested_track != track || Music.pause_requested ||
            NuSound3GetStereoStreamStatus(stream) == NUSOUND_STEREO_STREAM_FINISHED) {
            PlayAMusic(stream, track, volume, 0);
            started = 1;
        } else {
            NuSound3ResumeStereoStream(stream);
            NuSound3SetStereoStreamVolume(stream, volume);
        }

        NuGCutSetCutAudioStream(stream);
        Music.requested_track = -1;
        Music.pause_requested = false;
        Music.transition = 1.0f;
        if (state == MUSIC_PLAYBACK_DUAL_STREAM_PENDING) {
            Music.state = MUSIC_PLAYBACK_DUAL_STREAM_PENDING;
        } else if (state == 12) {
            Music.state = static_cast<MusicPlaybackState>(12);
        } else {
            Music.state =
                static_cast<u16>(Music.state) < 1 ? static_cast<MusicPlaybackState>(12) : MUSIC_PLAYBACK_DUAL_STREAM;
        }
        Music.track_data = context;
        return started;
    }

    void PlayMusic(i32 track, i32 mode) {
        if (NOSOUND != 0 || NOMUSIC != 0) {
            return;
        }

        i16 previous_state = Music.state;
        i16 previous_stream = Music.primary_stream;
        i16 previous_track = Music.current_track;

        if (track < 0 || track >= SFX_MUSIC_COUNT) {
            Music.state = static_cast<MusicPlaybackState>((mode == 3) * 2 + 7);
            Music.primary_stream = 1 - Music.primary_stream;
            Music.current_track = static_cast<i16>(track);
            Music.queued_track = previous_track;
            Music.restore_requested = false;
            Music.transition = 0.0f;
            return;
        }

        Music.transition_frames = 0;
        Music.queued_track = Music.current_track;
        Music.current_track = static_cast<i16>(track);

        i32 requested;
        i32 stream;
        if (Music.state == MUSIC_PLAYBACK_STOPPED) {
            requested = Music.requested_track;
            stream = Music.primary_stream;
            reinterpret_cast<u8 *>(&Music)[0x12 + stream] = 0;
            if (requested == track) {
                Music.primary_stream = 1 - previous_stream;
                stream = Music.primary_stream;
            }
        } else if (mode != 1 && NUSOUND_STREAM_3 != -1) {
            requested = Music.requested_track;
            Music.primary_stream = 1 - Music.primary_stream;
            stream = Music.primary_stream;
            reinterpret_cast<u8 *>(&Music)[0x12 + stream] = 0;
            if (requested == track && !Music.pause_requested) {
                Music.state = static_cast<MusicPlaybackState>((mode == 3) * 2 + 7);
                NuSound3ResumeStereoStream(stream);
                NuSound3SetStereoStreamVolume(stream, 0);
            } else {
                Music.state = static_cast<MusicPlaybackState>((mode == 3) * 2 + 8);
                PlayAMusic(stream, track, 0, 0);
            }
            Music.requested_track = -1;
            Music.pause_requested = false;
            Music.restore_requested = false;
            Music.transition = 0.0f;
            return;
        } else {
            requested = Music.requested_track;
            stream = Music.primary_stream;
            reinterpret_cast<u8 *>(&Music)[0x12 + stream] = 0;
            if (requested == track && previous_state != MUSIC_PLAYBACK_DUAL_STREAM) {
                Music.primary_stream = 1 - previous_stream;
                stream = Music.primary_stream;
            }
        }

        NuSound3StopStereoStream(1 - stream);
        if (Music.requested_track == track && !Music.pause_requested &&
            NuSound3GetStereoStreamStatus(Music.primary_stream) != NUSOUND_STEREO_STREAM_FINISHED) {
            NuSound3ResumeStereoStream(Music.primary_stream);
            NuSound3SetStereoStreamVolume(Music.primary_stream, static_cast<i32>(g_music[track].index * MusicVolume));
        } else {
            PlayAMusic(Music.primary_stream, track, static_cast<i32>(g_music[track].index * MusicVolume), 0);
        }
        Music.transition = 1.0f;
        Music.restore_requested = false;
        Music.pause_requested = false;
        Music.requested_track = -1;
        Music.state = MUSIC_PLAYBACK_ACTIVE;
    }

    void PlaySfx(char *name, struct nuvec_s *position) {
        i32 sfx_id = GetSfxId(name);
        if (sfx_id != -1) {
            PlaySfxById(sfx_id, position);
        }
    }

    void PlaySfxAndSetPitch(char *name, nuvec_s *position, f32 pitch) {
        i32 sfx_id = GetSfxId(name);
        if (sfx_id != -1) {
            PlaySfxByIdEx(sfx_id, position, 1.0f, pitch);
        }
    }

    void PlaySfxAndSetVolume(char *name, nuvec_s *position, f32 volume) {
        i32 id = GetSfxId(name);
        if (id != -1) {
            PlaySfxByIdEx(id, position, volume, 1.0f);
        }
    }

    void PlaySfxAndSetVolumeAndPitch(char *name, nuvec_s *position, f32 volume, f32 pitch) {
        i32 sfx_id = GetSfxId(name);
        if (sfx_id != -1) {
            PlaySfxByIdEx(sfx_id, position, volume, pitch);
        }
    }

    void PlaySfxById(i32 sfx_id, nuvec_s *position) {
        PlaySfxByIdEx(sfx_id, position, 1.0f, 1.0f);
    }

    void PlaySfxByIdAndSetPitch(i32 sfx_id, nuvec_s *position, f32 pitch) {
        PlaySfxByIdEx(sfx_id, position, 1.0f, pitch);
    }

    void PlaySfxByIdAndSetVolume(i32 sfx_id, nuvec_s *position, f32 volume) {
        PlaySfxByIdEx(sfx_id, position, volume, 1.0f);
    }

    void PlaySfxByIdAndSetVolumeAndPitch(i32 sfx_id, nuvec_s *position, f32 volume, f32 pitch) {
        PlaySfxByIdEx(sfx_id, position, volume, pitch);
    }

    void PlaySfxByIdEx(i32 sfx_id, nuvec_s *position, f32 volume, f32 pitch) {
        static u32 seed;
        static nuvec_s pos;

        if (sfx_id == -1) {
            return;
        }

        NUSOUNDINFO *sound = &g_soundInfo[sfx_id];
        if (sound->disabled != 0 || sound->comment != 0) {
            return;
        }

        i8 priority = sound->priority;
        if (sound->group != -1) {
            sfx_id = GroupBuffer_GetSample(sound->group, sound->seq);
            if (sfx_id == -1) {
                return;
            }
            sound = &g_soundInfo[sfx_id];
        }

        f32 pan = sound->pan;
        i32 sample_index = sound->index;
        bool loop = sound->loop != 0;
        f32 buzz_timer = sound->buzz_timer;
        i32 rumble_strength = sound->rumble_strength;
        f32 rumble_sustain = sound->rumble_sustain;
        f32 rumble_release = sound->rumble_release;

        f32 falloff_near = sound->falloff_near;
        f32 falloff_far = sound->falloff_far;
        f32 saved_fade_start = 0.0f;
        f32 saved_fade_end = 0.0f;
        bool custom_falloff = falloff_near != 0.0f || falloff_far != 0.0f;
        if (custom_falloff) {
            saved_fade_start = nusound_fade_start;
            saved_fade_end = nusound_fade_end;
            nusound_fade_start = falloff_near * saved_fade_start * 0.5f;
            nusound_fade_end = falloff_far * saved_fade_end / 15.0f;
        } else {
            falloff_near = 2.0f;
            falloff_far = 15.0f;
        }

        const NUMTX *listener = reinterpret_cast<const NUMTX *>(NuSound3GetListener());
        if (listener == NULL) {
            return;
        }

        if (position != NULL) {
            f32 dx = position->x - listener->m30;
            f32 dy = position->y - listener->m31;
            f32 dz = position->z - listener->m32;
            f32 max_distance_squared = nusound_fade_end * nusound_fade_end;
            if (dx * dx + dy * dy + dz * dz > max_distance_squared) {
                if (custom_falloff) {
                    nusound_fade_start = saved_fade_start;
                    nusound_fade_end = saved_fade_end;
                }
                return;
            }
        }

        if (sound->group != -1 && !HandleGroupLimit(sound->group)) {
            return;
        }

        f32 volume_scale;
        if (volume == 1.0f) {
            volume_scale = static_cast<f32>(sound->volume);
        } else {
            if (volume > 1.0f) {
                volume = 1.0f;
            }
            volume_scale = static_cast<f32>(sound->volume) * volume;
        }

        if (sound->nofade == 0) {
            volume_scale *= AUDIOFADELEVEL;
            volume_scale *= numusicGetDuckVolume();
        }
        i32 voice_volume = static_cast<i32>(volume_scale * MASTERVOLUME);

        if (sound->pitch_rnd != 0.0f) {
            f32 pitch_variation = NuRandFloatSeeded(&seed) * sound->pitch_rnd;
            if ((NuRandIntSeeded(&seed) & 1) == 0) {
                pitch_variation *= 0.5f;
                pitch *= 1.0f - pitch_variation;
            } else {
                pitch *= 1.0f + pitch_variation;
            }
        }

        if (sound->volume_rnd != 0.0f) {
            voice_volume = static_cast<i32>(static_cast<f32>(voice_volume) *
                                            (1.0f + NuRandFloatSeeded(&seed) * sound->volume_rnd));
        }

        if (static_cast<u32>(sample_index) <= 1599) {
            if (position != NULL) {
                if (loop) {
                    NuSound3Play3dLoopSfx(position, sample_index, falloff_near, falloff_far, voice_volume, voice_volume,
                                          pitch);
                } else if (priority == 0) {
                    NuSound3Play3d(position, sample_index, falloff_near, falloff_far, voice_volume, voice_volume, pitch,
                                   buzz_timer, rumble_strength, rumble_sustain, rumble_release);
                } else {
                    NuSound3Play3dPri(position, sample_index, falloff_near, falloff_far, voice_volume, voice_volume,
                                      pitch, buzz_timer, rumble_strength, rumble_sustain, rumble_release, priority);
                }
            } else {
                i32 volume_left = voice_volume;
                i32 volume_right = voice_volume;
                if (pan < 0.0f) {
                    volume_right = static_cast<i32>(static_cast<f32>(voice_volume) * (1.0f + pan));
                } else if (pan > 0.0f) {
                    volume_left = static_cast<i32>(static_cast<f32>(voice_volume) * (1.0f - pan));
                }

                if (loop) {
                    pos.x = listener->m30;
                    pos.y = listener->m31;
                    pos.z = listener->m32;
                    nuvec_s *forward = reinterpret_cast<nuvec_s *>(const_cast<f32 *>(&listener->m20));
                    NuVecAdd(&pos, &pos, forward);
                    NuSound3Play3dLoopSfx(&pos, sample_index, falloff_near, falloff_far, volume_left, volume_right,
                                          pitch);
                } else if (priority == 0) {
                    NuSound3Play(sample_index, volume_left, volume_right, pitch, buzz_timer, rumble_strength,
                                 rumble_sustain, rumble_release);
                } else {
                    NuSound3PlayPri(sample_index, volume_left, volume_right, pitch, buzz_timer, rumble_strength,
                                    rumble_sustain, rumble_release, priority);
                }
            }
        }

        if (custom_falloff) {
            nusound_fade_start = saved_fade_start;
            nusound_fade_end = saved_fade_end;
        }
    }

    i32 PlayingCutMusic(void) {
        const i32 stream = 1 - Music.primary_stream;
        u8 &delay = reinterpret_cast<u8 *>(&Music)[0x12 + stream];
        if (delay != 0) {
            --delay;
            return 0;
        }
        if (NOSOUND != 0 || NOMUSIC != 0) {
            return 0;
        }
        return NuSound3GetStereoStreamStatus(stream) != NUSOUND_STEREO_STREAM_FINISHED;
    }

    void PrepareAllSounds(void) {
        memset(SfxBits, 0xff, sizeof(SfxBits));
    }

    void RegisterSounds(SoundTable *table) {
        memset(table->bits, 0, sizeof(table->bits));
        if (table->names == NULL) {
            return;
        }
        for (const char **name = table->names; *name != NULL; ++name) {
            i32 id = GetSfxId(*name);
            table->bits[id >> 4] |= static_cast<u16>(1 << (id & 0xf));
        }
    }

    void ResetPreSeek(void) {
        if (Music.state == MUSIC_PLAYBACK_DUAL_STREAM) {
            Music.current_track = -1;
        } else {
            Music.queued_track = -1;
            Music.requested_track = -1;
            Music.pause_requested = false;
        }
        Music.seek_offset = 0.0f;
    }

    void RestoreGameMusic(void) {
        if ((NOMUSIC == 0 || NOSOUND == 0) && NUSOUND_STREAM_3 != -1) {
            if (Music.transition_frames < 25) {
                Music.restore_requested = true;
                return;
            }
            if (static_cast<u16>(Music.state - 11) > 2) {
                NuSound3ResumeStereoStream(Music.primary_stream);
                if (static_cast<u16>(Music.state - 5) < 6) {
                    NuSound3ResumeStereoStream(1 - Music.primary_stream);
                }
            }
        }
    }

    void ResumeGameAudio(void) {
    }

    void SOUND_SFXRequest_Table(void) {
    }

    void SetAudioFadeLevel(f32 level) {
        AUDIOFADELEVEL = level;
    }

    void SetCutVolume(f32 volume) {
        CutVolume = volume;
    }

    void SetLinkedCutSceneMusic(void *context, i32 state) {
        Music.track_data = context;
        if (static_cast<u16>(Music.state - MUSIC_PLAYBACK_DUAL_STREAM) <= 2) {
            Music.state = state == MUSIC_PLAYBACK_DUAL_STREAM_PENDING ? MUSIC_PLAYBACK_DUAL_STREAM_PENDING
                                                                      : MUSIC_PLAYBACK_DUAL_STREAM;
        }
    }

    void SetMusicVolume(f32 volume) {
        if (NOSOUND != 0 || NOMUSIC != 0) {
            return;
        }
        MusicVolume = volume;
        if (Music.state == MUSIC_PLAYBACK_ACTIVE) {
            NuSound3SetStereoStreamVolume(
                Music.primary_stream, static_cast<i32>(static_cast<f32>(g_music[Music.current_track].index) * volume));
        }
    }

    void SetPreSeekStartPoint(f32 start_point) {
        Music.seek_offset = start_point;
    }

    void SetSfxBitTab_Off(SoundTable *table, i32 sound) {
        if (sound >= 0) {
            SetSfxBitTab_OffEx(table, g_soundInfo[sound].index);
        }
    }

    void SetSfxBitTab_On(SoundTable *table, i32 sound) {
        if (sound >= 0) {
            SetSfxBitTab_OnEx(table, g_soundInfo[sound].index);
        }
    }

    void SetSfxBit_Off(i32 sound) {
        if (sound >= 0) {
            SetSfxBit_OffEx(g_soundInfo[sound].index);
        }
    }

    void SetSfxBit_On(i32 sound) {
        if (sound >= 0)
            SetSfxBit_OnEx(g_soundInfo[sound].index);
    }

    i32 SfxBit(i32 sound) {
        if (sound < 0) {
            return 0;
        }
        return SfxBitEx(g_soundInfo[sound].index);
    }

    void SfxBitMaskTable(u16 *bits, const u16 *mask) {
        for (i32 i = 0; i < 100; i++) {
            bits[i] &= mask[i];
        }
    }

    i32 SfxBitTab(const SoundTable *table, i32 sound) {
        if (sound < 0) {
            return 0;
        }
        return SfxBitTabEx(table, g_soundInfo[sound].index);
    }

    void SfxBitsRestore(SoundTable *table) {
        memmove(SfxBits, table->bits, sizeof(SfxBits));
    }

    void SfxBitsSetAll(u16 *bits) {
        memset(bits, 0xff, sizeof(SfxBits));
    }

    void SfxBitsStore(SoundTable *table) {
        memmove(table->bits, SfxBits, sizeof(SfxBits));
    }

    void StopAltGameMusic(void) {
        NuSound3CancelCheckStereo();
        NuSound3PauseStereoStream(1 - Music.primary_stream);
        Music.update_delay = 0;
        Music.queued_track = -1;
        Music.requested_track = -1;
        if (Music.resume_track >= 0) {
            MusicPreSeek(Music.resume_track);
        }
    }

    void SwapMusic(i32 mode) {
        if (NOSOUND != 0 || NOMUSIC != 0) {
            return;
        }

        i16 previous_track = Music.current_track;
        i16 queued_track = Music.queued_track;
        Music.primary_stream = 1 - Music.primary_stream;
        Music.current_track = queued_track;
        Music.queued_track = previous_track;
        NuSound3ResumeStereoStream(Music.primary_stream);

        if (mode == 1) {
            NuSound3SetStereoStreamVolume(Music.primary_stream,
                                          static_cast<i32>(g_music[queued_track].index * MusicVolume));
            NuSound3PauseStereoStream(1 - Music.primary_stream);
            NuSound3SetStereoStreamVolume(1 - Music.primary_stream, 0);
            Music.state = MUSIC_PLAYBACK_ACTIVE;
            Music.transition = 1.0f;
        } else {
            f32 transition = 0.0f;
            if (static_cast<u16>(Music.state - 5) < 2) {
                transition = 1.0f - Music.transition;
            }
            Music.transition = transition;
            NuSound3SetStereoStreamVolume(Music.primary_stream,
                                          static_cast<i32>(g_music[queued_track].index * transition));
            Music.state = static_cast<MusicPlaybackState>((mode == 3) + 5);
        }
    }

} // extern "C"

void PlayDieSfx(GameObject_s *object) {
    CHARACTERDATA *character = object->apiobj.character_data;
    GAMECHARACTERDATA_s *config = character->game_character;
    i32 sfx = config->sfx_die;
    if (sfx == -1) {
        const u32 flags = character->model_flags;
        if ((config->flags_090 & 0x800) != 0 || (flags & 0x2000) != 0) {
            sfx = GameAudio->sfx_ids[0x18];
        } else if ((flags & 0x04000000) != 0) {
            sfx = GameAudio->sfx_ids[0x19];
        } else if ((flags & 0x10) != 0) {
            sfx = GameAudio->sfx_ids[0x1a];
        } else if ((flags & 0x40000000) == 0) {
            sfx = GameAudio->sfx_ids[(object->field_0xf01 & 8) != 0 ? 0x1b : 0x1c];
        }
        if (sfx == -1) {
            goto extra;
        }
    }
    GameAudio_PlaySfxById(sfx, &object->apiobj.collision_position, 0, 0);
extra:
    if (ExtraDieSfxFn != NULL) {
        ExtraDieSfxFn(object);
    }
}

void PlayHurtSfx(GameObject_s *object) {
    CHARACTERDATA *character = object->apiobj.character_data;
    i32 sfx = character->game_character->sfx_hurt;
    if (sfx == -1) {
        if ((character->model_flags & 0x44002010) != 0) {
            goto extra;
        }
        sfx = GameAudio->sfx_ids[(object->field_0xf01 & 8) != 0 ? 0x16 : 0x17];
        if (sfx == -1) {
            goto extra;
        }
    }
    GameAudio_PlaySfxById(sfx, &object->apiobj.collision_position, 0, 0);
extra:
    if (ExtraHurtSfxFn != NULL) {
        ExtraHurtSfxFn(object);
    }
}

void PlayJumpSfx(GameObject_s *object, i32 type) {
    i32 sfx;
    const u32 flags = object->apiobj.character_data->model_flags;
    if ((flags & 0x40) != 0) {
        sfx = GameAudio->sfx_ids[1];
    } else if ((flags & 8) == 0) {
        sfx = GameAudio->sfx_ids[0];
    } else {
        static const u8 jump_sfx[5] = {2, 3, 6, 4, 5};
        if (static_cast<u32>(type) >= 5) {
            return;
        }
        sfx = GameAudio->sfx_ids[jump_sfx[type]];
    }
    if (sfx != -1) {
        if (static_cast<i8>(object->apiobj.flags_high) >= 0 && (object->field_0xefb & 8) == 0) {
            PlaySfxByIdAndSetVolume(sfx, &object->apiobj.lower_position, 0.5f);
        } else {
            GameAudio_PlaySfxById(sfx, &object->apiobj.lower_position, 0, 1);
        }
    }
}

void PlayLandSfx(GameObject_s *object, i32 type, i32) {
    i32 sfx;
    if (type == 1) {
        if ((object->apiobj.character_data->model_flags & 8) == 0) {
            return;
        }
        sfx = GameAudio->sfx_ids[0xb];
    } else if (type >= 2 && type <= 4) {
        sfx = GameAudio->sfx_ids[type + 0xa];
    } else {
        if (object->apiobj.is_underwater != 0) {
            return;
        }
        const bool alternate = (WorldInfo_CurrentlyActive()->current_level->flags & 0x1000) != 0;
        if ((object->apiobj.character_data->model_flags & 0x10) != 0) {
            sfx = GameAudio->sfx_ids[alternate ? 10 : 9];
        } else {
            if (type != 0) {
                return;
            }
            sfx = GameAudio->sfx_ids[alternate ? 8 : 7];
        }
    }
    if (sfx != -1) {
        if (static_cast<i8>(object->apiobj.flags_high) < 0 || (object->field_0xefb & 8) != 0) {
            GameAudio_PlaySfxById(sfx, &object->apiobj.lower_position, 0, 1);
        } else {
            PlaySfxByIdAndSetVolume(sfx, &object->apiobj.lower_position, 0.5f);
        }
    }
}

i32 SfxBitTabEx(SoundTable const *table, i32 sound) {
    if (static_cast<u32>(sound) >= 1600) {
        return -1;
    }
    return (table->bits[sound >> 4] & (1 << (sound & 15))) != 0;
}

void TickTockSfx() {
    if (ticktock == 0) {
        GameAudio_PlaySfx(0x1e, NULL, 0, 0);
        ticktock = 1;
    } else {
        GameAudio_PlaySfx(0x1d, NULL, 0, 0);
        ticktock = 0;
    }
}

void AddFootSteps(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if (packet.blending != 0 || packet.animation_index == -1) {
        return;
    }
    CHARACTERMODEL_s *model = object->apiobj.character_model;
    const i32 animation = packet.animation_index;
    if (model->model_data_b[animation] == NULL) {
        return;
    }
    CHARACTERANIM_s *config = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
    if (config == NULL || (config->flags & CHARACTER_ANIMATION_FLAG_FOOTSTEPS) == 0 ||
        packet.current_time == packet.previous_time) {
        return;
    }

    bool crossed = false;
    for (i32 i = 0; i < 4; ++i) {
        const f32 frame = config->event_frames[i];
        if (frame < 1.0f) {
            continue;
        }
        if ((packet.flags & ANIMPACKET_FLAG_LOOPED) != 0) {
            crossed = frame > packet.previous_time || frame <= packet.current_time;
        } else if ((packet.flags & ANIMPACKET_FLAG_PLAYING_REVERSED) != 0) {
            crossed = frame <= packet.previous_time && frame > packet.current_time;
        } else {
            crossed = frame > packet.previous_time && frame <= packet.current_time;
        }
        if (crossed) {
            break;
        }
    }
    if (crossed) {
        PlayFootStepSfx(object);
    }
}

void PlayGruntSfx(GameObject_s *object) {
    CHARACTERDATA *character = object->apiobj.character_data;
    i32 sfx = character->game_character->sfx_grunt;
    if (sfx == -1) {
        if ((character->model_flags & 0x44002010) != 0) {
            return;
        }
        sfx = GameAudio->sfx_ids[(object->field_0xf01 & 8) != 0 ? 0x14 : 0x15];
        if (sfx == -1) {
            return;
        }
    }
    GameAudio_PlaySfxById(sfx, &object->apiobj.collision_position, 0, 0);
}

void PlaySabreSfx(char *name, GameObject_s *object, nuvec_s *position, i32) {
    const u8 player = static_cast<u8>(object->apiobj.field_0x27c);
    if (player == 0xff && WORLD->rooms_visible_ptr[object->room_id] == 0) {
        return;
    }
    i32 player_bits = 0;
    if (static_cast<i8>(object->apiobj.flags_high) < 0) {
        player_bits = 1 << (player & 0x1f);
    }
    if (name == NULL) {
        const i32 sfx = object->apiobj.character_data->game_character->sfx_sabre;
        if (sfx == -1) {
            GameAudio_PlaySfx(0x40, &object->apiobj.collision_position, 0, 1);
        } else {
            GameAudio_PlaySfxById(sfx, &object->apiobj.collision_position, player_bits, 1);
        }
    } else {
        if (position == NULL) {
            position = &object->apiobj.collision_position;
        }
        GameAudio_PlaySfxById(GetSfxId(name), position, 0, 1);
    }
}

void LevChatterSfx(char *name, nuvec_s *position) {
    if (chattersfxwait <= 0.0f && qrand() < 0x400) {
        PlaySfx(name, position);
        const i32 random = qrand();
        chattersfxwait = static_cast<f32>(random) * (1.0f / 65535.0f) * 2.0f + 3.0f;
    }
}

void PlayRepeatSfx(char *name, i32 sfx_id, f32 initial_delay, char play_count, f32 interval, nuvec_s *position) {
    if (play_count == 1 && initial_delay == 0.0f) {
        if (sfx_id != -1) {
            GameAudio_PlaySfxById(sfx_id, position, 0, 0);
        } else {
            PlaySfx(name, position);
        }
        return;
    }

    if (initial_delay > 0.0f) {
        repsfxtab[repsfxcount].state = REPEAT_SFX_INITIAL_DELAY;
    } else {
        repsfxtab[repsfxcount].state = REPEAT_SFX_PLAY;
    }

    if (sfx_id == -1)
        sfx_id = GetSfxId(name);

    repsfxtab[repsfxcount].sfx_id = static_cast<i16>(sfx_id);
    RepeatSfx &repeat = repsfxtab[repsfxcount];
    repsfxcount = (repsfxcount + 1) & 31;
    repeat.timer = initial_delay;
    repeat.plays_remaining = play_count;
    repeat.interval = interval;
    repeat.position = position;
}

void ResetRepeatSfx() {
    repsfxcount = 0;
    memset(repsfxtab, 0, sizeof(repsfxtab));
    repsfxtab[0].sfx_id = -1;
}

void SetSfxBit_OnEx(i32 sound) {
    if (static_cast<u32>(sound) < 1600) {
        SfxBits[sound >> 4] |= 1 << (sound & 15);
    }
}

void UpdateLevelSfx(WORLDINFO_s *world, i32 paused) {
    SfxBitsRestore(&CurrentSFXTAB);

    if (paused != 0) {
        goto disable_reverb_and_return;
    }
    if (GameAudio->check_reverb_fn != NULL && GameAudio->check_reverb_fn() != 0) {
        goto enable_reverb;
    }
    NuSound3SetReverb(0);

update_ambient:
    if (CUTSTOPGAME == 0 && world->current_level->unknown_0a2 != -1) {
        GameAudio_PlaySfxById(world->current_level->unknown_0a2, NULL, 0, 0);
    }
    return;

disable_reverb_and_return:
    NuSound3SetReverb(0);
    return;

enable_reverb:
    NuSound3SetReverb(1);
    goto update_ambient;
}

void PlayFootStepSfx(GameObject_s *object) {
    i32 on_platform = 1;
    const i8 surface = static_cast<i8>(object->apiobj.field_0x281);
    if (object->field_0x1078 == -1 && object->apiobj.supporting_platform_id == -1 &&
        (surface == -1 || (TerSurface[surface].flags & 2) == 0)) {
        on_platform = 0;
    }

    i32 sfx = object->apiobj.character_data->game_character->sfx_footstep;
    if (sfx == -1 && (GameAudio->override_footstep_fn == NULL ||
                      (sfx = GameAudio->override_footstep_fn(object, on_platform)) == -1)) {
        if (object->apiobj.is_underwater == 0 && object->apiobj.intersects_water == 0) {
            const bool alternate = (WorldInfo_CurrentlyActive()->current_level->flags & 0x1000) != 0;
            if ((object->apiobj.character_data->model_flags & 0x10) == 0) {
                sfx = GameAudio->sfx_ids[alternate ? 0x10 : 0xf];
            } else {
                sfx = GameAudio->sfx_ids[alternate ? 0x12 : 0x11];
            }
        } else {
            sfx = GameAudio->sfx_ids[0x13];
        }
    }

    if (sfx != -1) {
        if (static_cast<i8>(object->apiobj.flags_high) < 0 || (object->field_0xefb & 8) != 0) {
            GameAudio_PlaySfxById(sfx, &object->apiobj.lower_position, 0, 0);
        } else {
            PlaySfxByIdAndSetVolume(sfx, &object->apiobj.lower_position, 0.5f);
        }
    }

    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if (packet.blending == 0 && packet.animation_index != -1) {
        CHARACTERANIM_s *config =
            static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[packet.animation_index]);
        if (config != NULL) {
            if ((config->flags & 0x20000) != 0) {
                GameCam_Judder(GameCam, -0.25f, 0, &object->apiobj.collision_position);
                NewRumbleAllPlayers(0.6f, 0.0f, 0, 0);
            } else if ((config->flags & 0x40000) != 0 && VehicleArea == 0) {
                GameCam_NewShake(GameCam, 0.5f, 0.5f, 1.0f);
                NewRumbleAllPlayers(0.5f, 0.0f, 0, 0);
            }
        }
    }
}

void SetSfxBit_OffEx(i32 sound) {
    if (static_cast<u32>(sound) < 1600) {
        SfxBits[sound >> 4] &= ~(1 << (sound & 15));
    }
}

void UpdateRepeatSfx() {
    for (RepeatSfx &repeat : repsfxtab) {
        switch (repeat.state) {
            case REPEAT_SFX_INITIAL_DELAY:
                if (repeat.timer > 0.0f) {
                    repeat.timer -= FRAMETIME;
                } else {
                    repeat.state = REPEAT_SFX_PLAY;
                }
                break;

            case REPEAT_SFX_PLAY:
                GameAudio_PlaySfxById(repeat.sfx_id, repeat.position, 0, 0);
                --repeat.plays_remaining;
                if (repeat.plays_remaining > 0) {
                    repeat.timer = repeat.interval;
                    repeat.state = REPEAT_SFX_INTERVAL;
                } else {
                    memset(&repeat, 0, sizeof(repeat));
                }
                break;

            case REPEAT_SFX_INTERVAL:
                if (repeat.timer > 0.0f) {
                    repeat.timer -= FRAMETIME;
                } else {
                    repeat.state = REPEAT_SFX_PLAY;
                }
                break;

            default:
                break;
        }
    }
}

void AddLevelSfxFromId(i32 sfx_id, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx) {
    if (sfx_count == NULL || sfx_ids == NULL || *sfx_count >= max_sfx || sfx_id == -1) {
        return;
    }
    for (i32 index = 0; index < *sfx_count; ++index) {
        if (sfx_ids[index] == sfx_id) {
            return;
        }
    }
    sfx_ids[*sfx_count] = sfx_id;
    ++*sfx_count;
}

void SetSfxBitTab_OnEx(SoundTable *table, i32 sound) {
    if (static_cast<u32>(sound) < 1600) {
        table->bits[sound >> 4] |= 1 << (sound & 15);
    }
}

void SetSfxBitTab_OffEx(SoundTable *table, i32 sound) {
    if (static_cast<u32>(sound) < 1600) {
        table->bits[sound >> 4] &= ~(1 << (sound & 15));
    }
}

void SfxCheckMusicOnOff(OPTIONSSAVE_s *) {
}

void AddLevelSfxFromName(char *sfx_name, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx_count) {
    if (sfx_count == NULL || sfx_ids == NULL || *sfx_count >= max_sfx_count) {
        return;
    }

    i32 sfx_id = GetSfxId(sfx_name);
    if (sfx_id == -1) {
        return;
    }

    for (i32 i = 0; i < *sfx_count; i++) {
        if (sfx_ids[i] == sfx_id) {
            return;
        }
    }

    sfx_ids[*sfx_count] = sfx_id;
    ++*sfx_count;
}

void AddLevelSfxGizmoSys(GIZMOSYS_s *gizmo_sys, void *world_info, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx_count) {
    if (gizmotypes == NULL || gizmo_sys == NULL) {
        return;
    }

    GIZMOSET *set = gizmo_sys->sets;
    GIZMOTYPE *type = gizmotypes->types;
    i32 i = 0;
    while (i < gizmotypes->count) {
        if (type->fns.add_level_sfx_fn != NULL) {
            type->fns.add_level_sfx_fn(world_info, set->unknown, sfx_ids, sfx_count, max_sfx_count);
        }
        ++i;
        ++set;
        ++type;
    }
}

void BlockSfx(GameObject_s *object) {
    GameObject_s *target = object->force_target;
    if (target != NULL) {
        const u32 context_flags = CInfo[target->character_context].flags;
        if ((context_flags & 0x4000000) != 0 || ((context_flags & 0x8000000) != 0 && (target->jump_flags & 2) != 0)) {
            const i32 sfx_bits = GameAudio_GetPlrSfxBits(target);
            GameAudio_PlaySfx(0x3f, &object->apiobj.collision_position, sfx_bits, 0);
        }
    }
}

i32 SfxBitEx(i32 sound) {
    if (static_cast<u32>(sound) >= 1600) {
        return -1;
    }
    return (SfxBits[sound >> 4] & (1 << (sound & 15))) != 0;
}

void AddLevSfx(WORLDINFO_s *world, nuvec_s *position, char *name, i32 sfx) {
    if (sfx == -1 && name != NULL) {
        sfx = GetSfxId(name);
    }
    if (sfx == -1) {
        return;
    }

    i32 index = 0;
    while (index < world->level_sfx_count && world->level_sfx[index].id != sfx) {
        ++index;
    }
    if (index == world->level_sfx_count) {
        if (index >= 64) {
            return;
        }
        world->level_sfx[index].position = position != NULL ? *position : nuvec_zero;
        world->level_sfx[index].id = static_cast<i16>(sfx);
        world->level_sfx[index].references = 1;
        world->level_sfx_count = index + 1;
    } else if (world->level_sfx[index].references != -1) {
        ++world->level_sfx[index].references;
    }
}

void GameAudio_Init(GAMEAUDIO *audio) {
    GameAudio = audio;
    for (i32 i = 0; i < 0x55; ++i) {
        audio->sfx_ids[i] = static_cast<i16>(GetSfxId(audio->sfx_names[i]));
    }

    MenuRegisterSoundFX(GameAudio_GetSfxId(0x2f), GameAudio_GetSfxId(0x30), GameAudio_GetSfxId(0x31),
                        GameAudio_GetSfxId(0x32));
}

void GameAudio_Reset() {
    memset(&GameAudio_Default, 0, sizeof(GameAudio_Default));
    GameAudio = &GameAudio_Default;
    memset(GameAudio_Default.sfx_names, 0, sizeof(GameAudio_Default.sfx_names));
    for (i32 i = 0; i < 0x55; ++i) {
        GameAudio_Default.sfx_ids[i] = -1;
    }
}

void GameAudio_PlaySfx(i32 sfx, nuvec_s *position, i32 flags, i32 volume) {
    if ((u32)sfx < 0x55) {
        GameAudio_PlaySfxById(GameAudio->sfx_ids[sfx], position, flags, volume);
    }
}

i32 GameAudio_GetSfxId(i32 sfx) {
    if (static_cast<u32>(sfx) <= 0x54) {
        return GameAudio->sfx_ids[sfx];
    }
    return -1;
}

void GameAudio_PlaySfxAndSetVolume(i32 sfx, nuvec_s *position, f32 volume) {
    if (static_cast<u32>(sfx) < 0x55) {
        PlaySfxByIdAndSetVolume(GameAudio->sfx_ids[sfx], position, volume);
    }
}

extern "C" void MusicSeekOffset(i32 track, f32 seek_offset) {
    const i16 transition_frames = Music.transition_frames;
    const i16 primary_stream = Music.primary_stream;
    if (seek_offset < 0.0f) {
        seek_offset = Music.seek_offset;
    }
    if (track < 0 || track >= SFX_MUSIC_COUNT) {
        return;
    }

    if (static_cast<u16>(Music.state - MUSIC_PLAYBACK_DUAL_STREAM) < 3) {
        if (Music.current_track == track && !Music.pause_requested) {
            return;
        }
        const i32 stream = Music.primary_stream;
        Music.requested_track = -1;
        Music.state = MUSIC_PLAYBACK_DUAL_STREAM;
        Music.current_track = static_cast<i16>(track);
        Music.queued_track = static_cast<i16>(track);
        reinterpret_cast<u8 *>(&Music)[stream + 0x12] = 0;
        if (transition_frames < 64) {
            Music.pause_requested = true;
        } else {
            Music.pause_requested = false;
            const i32 volume = static_cast<i32>(static_cast<f32>(g_music[track].index) * MusicVolume);
            if (NOSOUND == 0 && NOMUSIC == 0) {
                NuSound3StopStereoStream(stream);
                NuSound3PlayStereoV(NUSOUNDPLAYTOK_STEREOSTREAM, stream, NUSOUNDPLAYTOK_SAMPLE, track,
                                    NUSOUNDPLAYTOK_VOL, volume, NUSOUNDPLAYTOK_STARTOFFSET,
                                    static_cast<f64>(seek_offset), NUSOUNDPLAYTOK_ONESHOT, NUSOUNDPLAYTOK_END);
                Music.secondary_stream = primary_stream;
            }
            Music.transition_frames = 0;
            Music.seek_offset = 0.0f;
        }
        return;
    }

    const i32 stream = Music.primary_stream;
    Music.pause_requested = false;
    reinterpret_cast<u8 *>(&Music)[stream + 0x12] = 0;
    Music.state = MUSIC_PLAYBACK_ACTIVE;
    Music.transition_frames = 0;
    Music.requested_track = static_cast<i16>(track);
    Music.current_track = static_cast<i16>(track);
    Music.queued_track = static_cast<i16>(track);
    const i32 volume = static_cast<i32>(static_cast<f32>(g_music[track].index) * MusicVolume);
    if (NOSOUND == 0 && NOMUSIC == 0) {
        NuSound3StopStereoStream(stream);
        NuSound3PlayStereoV(NUSOUNDPLAYTOK_STEREOSTREAM, stream, NUSOUNDPLAYTOK_SAMPLE, track, NUSOUNDPLAYTOK_VOL,
                            volume, NUSOUNDPLAYTOK_STARTOFFSET, static_cast<f64>(seek_offset), NUSOUNDPLAYTOK_ONESHOT,
                            NUSOUNDPLAYTOK_END);
        Music.transition_frames = 0;
        Music.secondary_stream = primary_stream;
    }
}

struct SoundTrackData {
    u8 reserved_00[0x88];
    u8 flags_88;
};

extern "C" void SoundUpdate(float frame_time) {
    if (Music.update_delay != 0 || frame_time == 0.0f || static_cast<u16>(Music.state - 4) >= 10) {
        return;
    }

    switch (Music.state) {
        case MUSIC_PLAYBACK_ACTIVE:
            goto active_music;
        case 5:
        case 6:
        case 7:
        case 9:
            goto transition_music;
        case 8:
        case 10:
            if (NuSound3GetStereoStreamStatus(Music.primary_stream) != NUSOUND_STEREO_STREAM_FINISHED) {
                Music.state = static_cast<MusicPlaybackState>(7);
            }
            return;
        case MUSIC_PLAYBACK_DUAL_STREAM:
        case 12:
        case MUSIC_PLAYBACK_DUAL_STREAM_PENDING:
            goto linked_music;
        default:
            return;
    }

transition_music:
    if (Music.state == 6 || Music.state == 9) {
        frame_time += frame_time;
    } else {
        frame_time *= 0.2f;
    }
    Music.transition += frame_time;
    if (Music.transition >= 1.0f) {
        Music.transition = 1.0f;
        const i32 other_stream = 1 - Music.primary_stream;
        if (static_cast<u16>(Music.state - 5) <= 1) {
            NuSound3PauseStereoStream(other_stream);
        } else {
            NuSound3StopStereoStream(other_stream);
        }
        Music.state = MUSIC_PLAYBACK_ACTIVE;
        Music.transition_frames = 0;
    }

    if (Music.current_track != -1) {
        NuSound3SetStereoStreamVolume(
            Music.primary_stream,
            static_cast<i32>(static_cast<f32>(g_music[Music.current_track].index) * Music.transition * MusicVolume));
    }
    if (Music.queued_track != -1) {
        NuSound3SetStereoStreamVolume(1 - Music.primary_stream,
                                      static_cast<i32>(static_cast<f32>(g_music[Music.queued_track].index) *
                                                       (1.0f - Music.transition) * MusicVolume));
    }
    return;

active_music:
    if (NuSound3GetStereoStreamStatus(Music.secondary_stream) == NUSOUND_STEREO_STREAM_FINISHED) {
        return;
    }
    if (Music.transition_frames <= 0x7f) {
        ++Music.transition_frames;
        if (Music.transition_frames <= 0x40) {
            return;
        }
    }
    if (Music.pause_requested) {
        MusicPreSeek(Music.requested_track);
    }
    if (Music.transition_frames <= 0x18) {
        return;
    }
    if (Music.restore_requested) {
        RestoreGameMusic();
        Music.restore_requested = false;
    }
    return;

linked_music:
    SoundTrackData *track_data = static_cast<SoundTrackData *>(Music.track_data);
    if (track_data != NULL && (track_data->flags_88 & 2) != 0) {
        Music.resume_frames = 0;
    } else {
        const i32 other_stream = 1 - Music.primary_stream;
        if (NuSound3GetStereoStreamStatus(other_stream) == NUSOUND_STEREO_STREAM_FINISHED) {
            Music.resume_frames = 0;
        } else {
            if (Music.resume_frames <= 0x3f) {
                ++Music.resume_frames;
                if (Music.resume_frames <= 8) {
                    goto update_secondary_stream;
                }
            }
            if (Music.current_track != -1 && Music.state != MUSIC_PLAYBACK_DUAL_STREAM_PENDING &&
                NuSound3GetStereoStreamStatus(Music.primary_stream) != NUSOUND_STEREO_STREAM_FINISHED &&
                !Music.pause_requested) {
                if (Music.state == 12) {
                    Music.state = MUSIC_PLAYBACK_STOPPED;
                    NuSound3StopStereoStream(other_stream);
                    NuSound3StopStereoStream(Music.primary_stream);
                } else {
                    Music.state = MUSIC_PLAYBACK_ACTIVE;
                    NuSound3StopStereoStream(other_stream);
                    NuSound3ResumeStereoStream(Music.primary_stream);
                }
                Music.transition = 1.0f;
                NuSound3SetStereoStreamVolume(
                    Music.primary_stream,
                    static_cast<i32>(static_cast<f32>(g_music[Music.current_track].index) * MusicVolume));
                Music.transition_frames = 0;
                Music.requested_track = -1;
            }
        }
    }

update_secondary_stream:
    if (NuSound3GetStereoStreamStatus(Music.secondary_stream) == NUSOUND_STEREO_STREAM_FINISHED) {
        return;
    }
    if (Music.transition_frames <= 0x7f) {
        ++Music.transition_frames;
        if (Music.transition_frames <= 0x40) {
            return;
        }
    }
    if (Music.pause_requested) {
        MusicPreSeek(Music.requested_track);
    }
}
