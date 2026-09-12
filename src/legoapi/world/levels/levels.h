#pragma once

// Episode level-handler module: netpacket / pod-race shared game state plus
// the per-episode level fix-up handler prototypes. These functions are defined
// in the individual episode*.cpp / hub.cpp files and wired into the LEVELDATA
// function-pointer tables by level.cpp.

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/common.h"

struct WORLDINFO_s;

// --- PodRace / PodSprint / boss netpacket shared globals (defined in
// globals.cpp; used by the episode level files) ---
//
// These are level-loading scratch that must keep matching the original
// .data/.bss placement, so their definitions live in globals.cpp and are only
// declared here for the episode files that consume them.

struct GIZAIMESSAGESYS_s;
struct AREADATA_s;
struct MINESYS_s;   // full type in legoapi/legoapi_types.h
struct PODSPRINT_s; // full type in legoapi/legoapi_types.h
struct PODRACENETPACKET_s;
struct PODSPRINTNETPACKET_s;
struct RETAKEGNETPACKET_s;
struct SNIPER_s;
struct FadeSystem;
struct GIZFORCE_s;
struct GAMECAMERA_s;
struct GameObject_s;
struct nuhspecial_s;
struct GAMECUTSCENES_s; // full type in legoapi/legoapi_types.h

extern GIZAIMESSAGESYS_s *gizaimessagesys;
extern i16 trooper_boltid;
extern i8 trooper_side[3];
extern nuhspecial_s *hothtroopers;
extern i32 TimingBarSet;
extern struct AREADATA_s *PODRACE_ADATA;
extern struct AREADATA_s *JEDI_ADATA;
extern struct AREADATA_s *DOOKU_ADATA;
extern i16 id_ANAKINPADAWAN;
extern u32 client_mines[];
extern MINESYS_s minesys; // held by value in the original (0x748 bytes)
extern i32 nethost;
extern i32 mine_count;
extern float gungan_a_time_Normal;
extern float gungan_a_time_LowEnd;
extern i32 active_neutral_count;
extern i32 active_baddy_count;
extern FadeSystem FadeSys;
#ifdef __cplusplus
extern "C" i32 Paused;
#else
extern i32 Paused;
#endif
extern i32 MiniCutCam;
extern PODRACENETPACKET_s *podrace_netpacket;
extern PODSPRINTNETPACKET_s *podsprint_netpacket;
extern GAMECAMERA_s *GameCam;
extern i32 pause_rndr_on;
extern u8 object_switches[0x80];
extern i32 gunship_player_dead;
extern GIZFORCE_s *force_array[4];
extern GameObject_s *ObiWan;
extern i32 bonus_gunship_store_progress_flag;
extern float MiscTime;
extern void *kaminoe_netpacket;
extern void *factoryb_netpacket;
struct BONUSGUNSHIP_NETPACKET_s {
    u8 state; // 0x00 (LevFlag.progress sync)
    u8 sub;   // 0x01 (LevFlag.substate sync)
    u8 pad[2];
    float time; // 0x04
};
extern struct BONUSGUNSHIP_NETPACKET_s *bonusgunshipb_netpacket;
extern i32 LevForce;
extern u8 dookuC_nodesNeedUpdating;
// Vader C boss level state.
struct vader_c_s {
    GIZAIMESSAGE_s *final_fight_message;
    AILOCATOR_s *big_jump_locator;
    nuhspecial_s rocks[10];
    i16 platform_ids[10];
    u8 field_0x94; // 0x94
    u8 field_0x95; // 0x95
    char pad_0x96[2];
};
extern struct vader_c_s vader_c;
struct CUTINFO;
extern CUTINFO *factoryb_cut;
extern void *factoryb_conveyor_stopped_msg;
extern float podanimendframe;
extern PODSPRINT_s podsprint; // held by value in the original (0x94 bytes)
extern GameObject_s **game_objects;
extern float pacemaker_alpha_table[];
extern i32 clients_mines_bitfield[2];
extern i32 pod_mines_bitfield[2];
extern GAMECUTSCENES_s game_cutscenes; // held by value in the original (0x28 bytes)

// Unmangled globals from the original binary (C linkage).
extern "C" {
    extern RETAKEGNETPACKET_s *retakeg_netpacket; // pointer from SetLevelHack(4)
    extern i32 podrace_section;
    extern i32 max_nsnipers;
    extern i32 PodRace_nsnipers;
    extern SNIPER_s PodRace_snipers[5];
    extern float PodRace_sniper_fire_time;
    extern float PodRace_sniper_start_fire_radius;
    extern float PodRace_sniper_fire_radius;
    extern float PodRace_sniper_fire_range_time;
    extern float pod_roll[2];
    extern float pod_roll_target[2];
    extern float pod_animtime[2];
}
extern i16 temp_yrot;
extern i16 temp_xrot;
extern f32 avg_currentspeed_mul;
extern GameObject_s *player2;
extern GameObject_s *player;

// --- Per-episode level fix-up handler prototypes

struct WORLDINFO_s;

void UpdateStatusScreen(struct WORLDINFO_s *);
void DrawStatusScreen(struct WORLDINFO_s *);
void Hub_Draw3D(struct WORLDINFO_s *);
void Hub_Update(struct WORLDINFO_s *);
void JediB_Init(struct WORLDINFO_s *);
void MaulA_Init(struct WORLDINFO_s *);
void MaulB_Init(struct WORLDINFO_s *);
void MaulD_Init(struct WORLDINFO_s *);
void MaulE_Init(struct WORLDINFO_s *);
void MaulF_Init(struct WORLDINFO_s *);
void DookuC_Init(struct WORLDINFO_s *);
void JediB_Reset(struct WORLDINFO_s *);
void MaulA_Panel(struct WORLDINFO_s *);
void MaulA_Reset(struct WORLDINFO_s *);
void MaulF_Panel(struct WORLDINFO_s *);
void MaulF_Reset(struct WORLDINFO_s *);
void VaderA_Init(struct WORLDINFO_s *);
void VaderB_Init(struct WORLDINFO_s *);
void VaderC_Init(struct WORLDINFO_s *);
void CruiserAInit(struct WORLDINFO_s *);
void CruiserDInit(struct WORLDINFO_s *);
void DookuC_Reset(struct WORLDINFO_s *);
void GunganA_Init(struct WORLDINFO_s *);
void JediB_Update(struct WORLDINFO_s *);
void KaminoC_Init(struct WORLDINFO_s *);
void KaminoD_Init(struct WORLDINFO_s *);
void KaminoE_Draw(struct WORLDINFO_s *);
void KaminoE_Init(struct WORLDINFO_s *);
void KaminoF_Init(struct WORLDINFO_s *);
void MaulA_Update(struct WORLDINFO_s *);
void MaulD_Update(struct WORLDINFO_s *);
void MaulE_Update(struct WORLDINFO_s *);
void MaulF_Update(struct WORLDINFO_s *);
void NewTown_Init(struct WORLDINFO_s *);
void PodRaceADraw(struct WORLDINFO_s *);
void PodRaceAInit(struct WORLDINFO_s *);
void PodRaceBInit(struct WORLDINFO_s *);
void PodRaceCInit(struct WORLDINFO_s *);
void PodRacePanel(struct WORLDINFO_s *);
void RescueA_Init(struct WORLDINFO_s *);
void RescueB_Init(struct WORLDINFO_s *);
void RescueC_Init(struct WORLDINFO_s *);
void RescueE_Init(struct WORLDINFO_s *);
void RetakeD_Init(struct WORLDINFO_s *);
void RetakeE_Init(struct WORLDINFO_s *);
void RetakeG_Init(struct WORLDINFO_s *);
void SenateA_Init(struct WORLDINFO_s *);
void TempleA_Init(struct WORLDINFO_s *);
void TempleC_Init(struct WORLDINFO_s *);
void VaderA_Reset(struct WORLDINFO_s *);
void VaderB_Reset(struct WORLDINFO_s *);
void VaderC_Reset(struct WORLDINFO_s *);
void CruiserCPanel(struct WORLDINFO_s *);
void CruiserCReset(struct WORLDINFO_s *);
void CruiserDReset(struct WORLDINFO_s *);
void DagobahA_Init(struct WORLDINFO_s *);
void DagobahB_Init(struct WORLDINFO_s *);
void DagobahC_Init(struct WORLDINFO_s *);
void DagobahE_Init(struct WORLDINFO_s *);
void DookuC_Update(struct WORLDINFO_s *);
void FactoryB_Draw(struct WORLDINFO_s *);
void FactoryB_Init(struct WORLDINFO_s *);
void FactoryG_Init(struct WORLDINFO_s *);
void GunshipA_Draw(struct WORLDINFO_s *);
void GunshipA_Init(struct WORLDINFO_s *);
void Hub_DrawPanel(struct WORLDINFO_s *);
void KaminoC_Reset(struct WORLDINFO_s *);
void KaminoE_Reset(struct WORLDINFO_s *);
void LegoCity_Init(struct WORLDINFO_s *);
void NewTown_Reset(struct WORLDINFO_s *);
void Platform_Init(struct WORLDINFO_s *);
void PodRaceAReset(struct WORLDINFO_s *);
void PodRaceBReset(struct WORLDINFO_s *);
void PodRaceCReset(struct WORLDINFO_s *);
void RetakeG_Panel(struct WORLDINFO_s *);
void RetakeG_Reset(struct WORLDINFO_s *);
void VaderA_Update(struct WORLDINFO_s *);
void VaderB_Update(struct WORLDINFO_s *);
void VaderC_Update(struct WORLDINFO_s *);
void ANewHopeA_Init(struct WORLDINFO_s *);
void CruiserAUpdate(struct WORLDINFO_s *);
void CruiserCUpdate(struct WORLDINFO_s *);
void CruiserDUpdate(struct WORLDINFO_s *);
void DagobahB_Reset(struct WORLDINFO_s *);
void DagobahC_Panel(struct WORLDINFO_s *);
void FactoryB_Reset(struct WORLDINFO_s *);
void GrievousA_Init(struct WORLDINFO_s *);
void GunganA_Update(struct WORLDINFO_s *);
void GunshipB_Reset(struct WORLDINFO_s *);
void KaminoC_Update(struct WORLDINFO_s *);
void KaminoE_Update(struct WORLDINFO_s *);
void KashyyykA_Init(struct WORLDINFO_s *);
void KashyyykB_Init(struct WORLDINFO_s *);
void KashyyykC_Init(struct WORLDINFO_s *);
void KashyyykD_Init(struct WORLDINFO_s *);
void LegoCity_Reset(struct WORLDINFO_s *);
void NbKaminoA_Init(struct WORLDINFO_s *);
void NewTown_Update(struct WORLDINFO_s *);
void Platform_Reset(struct WORLDINFO_s *);
void PodRaceAUpdate(struct WORLDINFO_s *);
void PodRaceBUpdate(struct WORLDINFO_s *);
void PodRaceCUpdate(struct WORLDINFO_s *);
void RetakeG_Update(struct WORLDINFO_s *);
void TatooineA_Init(struct WORLDINFO_s *);
void TatooineB_Init(struct WORLDINFO_s *);
void TatooineC_Init(struct WORLDINFO_s *);
void TatooineD_Init(struct WORLDINFO_s *);
void DagobahA_Update(struct WORLDINFO_s *);
void FactoryB_Update(struct WORLDINFO_s *);
void FactoryG_Update(struct WORLDINFO_s *);
void GrievousA_Reset(struct WORLDINFO_s *);
void GunshipA_Update(struct WORLDINFO_s *);
void JediB_DrawPanel(struct WORLDINFO_s *);
void KashyyykA_Panel(struct WORLDINFO_s *);
void KashyyykA_Reset(struct WORLDINFO_s *);
void KashyyykB_Reset(struct WORLDINFO_s *);
void KashyyykD_Reset(struct WORLDINFO_s *);
void LegoCity_Update(struct WORLDINFO_s *);
void MosEisleyA_Init(struct WORLDINFO_s *);
void MosEisleyB_Init(struct WORLDINFO_s *);
void MosEisleyD_Init(struct WORLDINFO_s *);
void MosEisleyE_Init(struct WORLDINFO_s *);
void PodSprintA_Init(struct WORLDINFO_s *);
void DookuC_DrawPanel(struct WORLDINFO_s *);
void GrievousA_Update(struct WORLDINFO_s *);
void HothBattleA_Draw(struct WORLDINFO_s *);
void HothBattleA_Init(struct WORLDINFO_s *);
void HothBattleC_Draw(struct WORLDINFO_s *);
void HothBattleC_Init(struct WORLDINFO_s *);
void HothBattleE_Draw(struct WORLDINFO_s *);
void HothBattleE_Init(struct WORLDINFO_s *);
void HothEscapeA_Init(struct WORLDINFO_s *);
void HothEscapeB_Init(struct WORLDINFO_s *);
void HothEscapeC_Init(struct WORLDINFO_s *);
void HothEscapeD_Init(struct WORLDINFO_s *);
void KaminoOutro_Init(struct WORLDINFO_s *);
void KashyyykA_Update(struct WORLDINFO_s *);
void KashyyykB_Update(struct WORLDINFO_s *);
void KashyyykC_Update(struct WORLDINFO_s *);
void KashyyykD_Update(struct WORLDINFO_s *);
void MosEisleyE_Reset(struct WORLDINFO_s *);
void PodSprintA_Panel(struct WORLDINFO_s *);
void PodSprintA_Reset(struct WORLDINFO_s *);
void SarlaccPitA_Draw(struct WORLDINFO_s *);
void SarlaccPitB_Init(struct WORLDINFO_s *);
void SarlaccPitC_Init(struct WORLDINFO_s *);
void TatooineA_Update(struct WORLDINFO_s *);
void TatooineD_Update(struct WORLDINFO_s *);
void VaderA_DrawPanel(struct WORLDINFO_s *);
void VaderB_DrawPanel(struct WORLDINFO_s *);
void VaderC_DrawPanel(struct WORLDINFO_s *);
void Credits_DrawPanel(struct WORLDINFO_s *);
void EndorBattleA_Init(struct WORLDINFO_s *);
void EndorBattleC_Init(struct WORLDINFO_s *);
void HothBattleA_Reset(struct WORLDINFO_s *);
void HothBattleC_Reset(struct WORLDINFO_s *);
void HothBattleE_Panel(struct WORLDINFO_s *);
void HothEscapeA_Reset(struct WORLDINFO_s *);
void HothEscapeB_Reset(struct WORLDINFO_s *);
void HothEscapeC_Reset(struct WORLDINFO_s *);
void HothEscapeD_Reset(struct WORLDINFO_s *);
void MosEisleyB_Update(struct WORLDINFO_s *);
void MosEisleyE_Update(struct WORLDINFO_s *);
void PodSprintA_Update(struct WORLDINFO_s *);
void SarlaccPitA_Reset(struct WORLDINFO_s *);
void SarlaccPitB_Reset(struct WORLDINFO_s *);
void SarlaccPitC_Reset(struct WORLDINFO_s *);
void BonusGunshipB_Init(struct WORLDINFO_s *);
void ChrisDogFightADraw(struct WORLDINFO_s *);
void ChrisDogFightAInit(struct WORLDINFO_s *);
void EmperorFightA_Init(struct WORLDINFO_s *);
void HothBattleA_Update(struct WORLDINFO_s *);
void HothBattleC_Update(struct WORLDINFO_s *);
void HothBattleE_Update(struct WORLDINFO_s *);
void HothEscapeA_Update(struct WORLDINFO_s *);
void HothEscapeB_Update(struct WORLDINFO_s *);
void HothEscapeC_Update(struct WORLDINFO_s *);
void HothEscapeD_Update(struct WORLDINFO_s *);
void JabbasPalaceA_Init(struct WORLDINFO_s *);
void JabbasPalaceB_Init(struct WORLDINFO_s *);
void JabbasPalaceE_Init(struct WORLDINFO_s *);
void NegotiationsA_Init(struct WORLDINFO_s *);
void NegotiationsB_Init(struct WORLDINFO_s *);
void SarlaccPitB_Update(struct WORLDINFO_s *);
void SarlaccPitC_Update(struct WORLDINFO_s *);
void SpeederChaseA_Init(struct WORLDINFO_s *);
void AnakinsFlightB_Draw(struct WORLDINFO_s *);
void AnakinsFlightB_Init(struct WORLDINFO_s *);
void AsteroidChaseA_Init(struct WORLDINFO_s *);
void AsteroidChaseB_Init(struct WORLDINFO_s *);
void AsteroidChaseC_Init(struct WORLDINFO_s *);
void AsteroidChaseD_Init(struct WORLDINFO_s *);
void BonusGunshipA_Reset(struct WORLDINFO_s *);
void BonusGunshipB_Panel(struct WORLDINFO_s *);
void BonusGunshipB_Reset(struct WORLDINFO_s *);
void ChrisDogFightAReset(struct WORLDINFO_s *);
void CloudCityTrapA_Init(struct WORLDINFO_s *);
void CloudCityTrapB_Init(struct WORLDINFO_s *);
void EmperorFightA_Panel(struct WORLDINFO_s *);
void EmperorFightA_Reset(struct WORLDINFO_s *);
void EndorBattleA_Update(struct WORLDINFO_s *);
void JabbasPalaceA_Reset(struct WORLDINFO_s *);
void JabbasPalaceB_Reset(struct WORLDINFO_s *);
void JabbasPalaceD_Reset(struct WORLDINFO_s *);
void JabbasPalaceE_Panel(struct WORLDINFO_s *);
void JabbasPalaceE_Reset(struct WORLDINFO_s *);
void PodRaceAlwasyUpdate(struct WORLDINFO_s *);
void SpeederChaseA_Panel(struct WORLDINFO_s *);
void SpeederChaseA_Reset(struct WORLDINFO_s *);
void AsteroidChaseA_Reset(struct WORLDINFO_s *);
void AsteroidChaseB_Reset(struct WORLDINFO_s *);
void AsteroidChaseC_Reset(struct WORLDINFO_s *);
void AsteroidChaseD_Panel(struct WORLDINFO_s *);
void BlockadeRunnerB_Init(struct WORLDINFO_s *);
void BlockadeRunnerC_Init(struct WORLDINFO_s *);
void BonusGunshipA_Update(struct WORLDINFO_s *);
void BonusGunshipB_Update(struct WORLDINFO_s *);
void ChrisDogFightAUpdate(struct WORLDINFO_s *);
void CloudCityTrapA_Reset(struct WORLDINFO_s *);
void CloudCityTrapC_Panel(struct WORLDINFO_s *);
void CloudCityTrapC_Reset(struct WORLDINFO_s *);
void DeathStarBattleDDraw(struct WORLDINFO_s *);
void DeathStarBattleDInit(struct WORLDINFO_s *);
void EmperorFightA_Update(struct WORLDINFO_s *);
void JabbasPalaceA_Update(struct WORLDINFO_s *);
void JabbasPalaceE_Update(struct WORLDINFO_s *);
void KaminoA_AlwaysUpdate(struct WORLDINFO_s *);
void KaminoE_AlwaysUpdate(struct WORLDINFO_s *);
void SpeederChaseA_Update(struct WORLDINFO_s *);
void TempleC_AlwaysUpdate(struct WORLDINFO_s *);
void AnakinsFlightB_Update(struct WORLDINFO_s *);
void AsteroidChaseA_Update(struct WORLDINFO_s *);
void AsteroidChaseB_Update(struct WORLDINFO_s *);
void AsteroidChaseC_Update(struct WORLDINFO_s *);
void AsteroidChaseD_Update(struct WORLDINFO_s *);
void BlockadeRunnerD_Reset(struct WORLDINFO_s *);
void CloudCityEscapeA_Init(struct WORLDINFO_s *);
void CloudCityEscapeC_Init(struct WORLDINFO_s *);
void CloudCityTrapA_Update(struct WORLDINFO_s *);
void CloudCityTrapB_Update(struct WORLDINFO_s *);
void CloudCityTrapC_Update(struct WORLDINFO_s *);
void DeathStarBattleDReset(struct WORLDINFO_s *);
void DeathStarEscapeA_Init(struct WORLDINFO_s *);
void DeathStarEscapeB_Draw(struct WORLDINFO_s *);
void DeathStarEscapeB_Init(struct WORLDINFO_s *);
void DeathStarEscapeC_Init(struct WORLDINFO_s *);
void DeathStarRescueB_Init(struct WORLDINFO_s *);
void DeathStarRescueC_Init(struct WORLDINFO_s *);
void E1CharacterBonus_Init(struct WORLDINFO_s *);
void PodRaceA_AlwaysUpdate(struct WORLDINFO_s *);
void BlockadeRunnerB_Update(struct WORLDINFO_s *);
void BlockadeRunnerD_Update(struct WORLDINFO_s *);
void CloudCityEscapeA_Panel(struct WORLDINFO_s *);
void CloudCityEscapeA_Reset(struct WORLDINFO_s *);
void DeathStar2BattleD_Init(struct WORLDINFO_s *);
void DeathStarBattleDUpdate(struct WORLDINFO_s *);
void DeathStarEscapeC_Reset(struct WORLDINFO_s *);
void CloudCityEscapeA_Update(struct WORLDINFO_s *);
void CloudCityEscapeC_Update(struct WORLDINFO_s *);
void DeathStarEscapeA_Update(struct WORLDINFO_s *);
void DeathStarEscapeB_Update(struct WORLDINFO_s *);
void DeathStarEscapeC_Update(struct WORLDINFO_s *);
void DeathStarEscapeD_Update(struct WORLDINFO_s *);
void DeathStarRescueB_Update(struct WORLDINFO_s *);
void MosEisleyD_AlwaysUpdate(struct WORLDINFO_s *);
void DeathStar2BattleD_Update(struct WORLDINFO_s *);
void HothEscapeC_AlwaysUpdate(struct WORLDINFO_s *);
void BountyHunterPursuitA_Init(struct WORLDINFO_s *);
void BountyHunterPursuitB_Init(struct WORLDINFO_s *);
void BountyHunterPursuitC_Init(struct WORLDINFO_s *);
void BountyHunterPursuitD_Init(struct WORLDINFO_s *);
void DeathStar2BattleFire_Draw(struct WORLDINFO_s *);
void DeathStar2BattleFire_Init(struct WORLDINFO_s *);
void BountyHunterPursuitA_Reset(struct WORLDINFO_s *);
void BountyHunterPursuitB_Reset(struct WORLDINFO_s *);
void BountyHunterPursuitC_Reset(struct WORLDINFO_s *);
void BountyHunterPursuitD_Reset(struct WORLDINFO_s *);
void BountyHunterPursuitA_Update(struct WORLDINFO_s *);
void BountyHunterPursuitB_Update(struct WORLDINFO_s *);
void BountyHunterPursuitC_Update(struct WORLDINFO_s *);
void BountyHunterPursuitD_Update(struct WORLDINFO_s *);
void DeathStar2BattleFire_Update(struct WORLDINFO_s *);
void DeathStarBattleC_AlwaysUpdate(struct WORLDINFO_s *);
void DeathStarEscapeB_AlwaysUpdate(struct WORLDINFO_s *);
void DeathStarRescueB_AlwaysUpdate(struct WORLDINFO_s *);
void DeathStarRescueC_AlwaysUpdate(struct WORLDINFO_s *);
void DeathStar2BattleA_AlwaysUpdate(struct WORLDINFO_s *);
void Hub_Init(struct WORLDINFO_s *);
void Hub_Load(struct WORLDINFO_s *, VARIPTR *, VARIPTR *);
void Hub_Reset(struct WORLDINFO_s *);
void PodRaceInit(struct WORLDINFO_s *);
void HothBattleB_Init(struct WORLDINFO_s *);
void AsteroidChaseB_Draw(struct WORLDINFO_s *);
void Hub_DrawMiniKits(struct WORLDINFO_s *);
void Hub_InitMiniKits(struct WORLDINFO_s *);
void Hub_LockUnlockDoors(struct WORLDINFO_s *);
void Hub_UpdateMiniKits(struct WORLDINFO_s *);
