#include "gameapi/ai/aisys/aisys.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nulgtlaser.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/audio/sfx.h"
#include "nu2api/numath/nurand.h"
#include "legoapi/core/input/qrand.h"
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
i32 Player_HasInvincibility(GameObject_s *);
void GameAudio_PlaySfxById(i32, NUVEC *, i32, i32);
extern "C" i32 GetSfxId(const char *);
extern i32 testlaser_type;
extern f32 testlaser_sizew, testlaser_sizel, testlaser_sizewab, testlaser_endw;
#include "nu2api/nucore/nustring.h"
#include "gameapi/ai/aisys/aisys.h"
#include <stdio.h>
#include "legoapi/world/level.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "decomp.h"
#include "globals.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"

extern TERRAIN_SURFACE_s TerSurface[32];

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" i32 FindPlatInst(i32 instance_ix);

// Episode 6 level handlers, in the game's Episode_VI progression:
// jabbas palace / sarlacc pit / speeder chase / endor battle / death star 2
// battle / emperor fight, plus the senate bonus.

// ===========================================================================
// Jabba's Palace (JabbasPalace_A / B / D / E)
// ===========================================================================

void JabbasPalaceA_Init(WORLDINFO_s *) {
}

void JabbasPalaceB_Init(WORLDINFO_s *) {
}

void JabbasPalaceE_Init(WORLDINFO_s *) {
}

void JabbasPalaceA_Reset(WORLDINFO_s *) {
    TerSurface[14].flags = 0xa040;
    TerSurface[9].flags = 0x2040;
}

void JabbasPalaceB_Reset(WORLDINFO_s *) {
    TerSurface[14].flags = 0xa040;
    TerSurface[9].flags = 0x2040;
}

void JabbasPalaceD_Reset(WORLDINFO_s *world) {
    TerSurface[14].flags = 0xa040;
    TerSurface[9].flags = 0x2040;
    if (world->current_level == JABBASPALACED_LDATA && world->push_block_count > 0) {
        pushblock_s *push_block = world->push_blocks;
        NUDISPLAYSPECIAL_s *display_special = push_block->special.display_special;
        if (display_special != NULL) {
            NUGSCN *scene = push_block->special.scene;
            if (scene != NULL && scene->display_list != NULL)
                scene->display_list->visibility_flags[display_special->instance_ix] |= 8;
        }
    }
}

void JabbasPalaceE_Reset(WORLDINFO_s *world) {
    LevGameObject[0] = GetNamedGameObject(world->ai_sys, "AI_rancor");
    LevArea[0] = AISysFindArea(world->ai_sys, "Alcove_1");
    LevArea[1] = AISysFindArea(world->ai_sys, "Alcove_2");
    LevArea[2] = AISysFindArea(world->ai_sys, "Back_Room");
    LevAIMessage[0] = CheckGizAIMessage(gizaimessagesys, "ShowHearts", NULL);
    TerSurface[14].flags = 0xa040;
    TerSurface[9].flags = 0x2040;
}

void JabbasPalaceE_Panel(WORLDINFO_s *) {
}

void JabbasPalaceA_Update(WORLDINFO_s *) {
}

void JabbasPalaceE_Update(WORLDINFO_s *) {
}

// ===========================================================================
// Sarlacc Pit (SarlaccPit_A / B / C)
// ===========================================================================

void SarlaccPitA_Draw(WORLDINFO_s *) {
}

void SarlaccPitA_Reset(WORLDINFO_s *world) {
    LevSafePlatID[0] = -1;
    if (NuSpecialFind(world->current_gscn, &LevHSpecial[0], "float_skiff_2", 1) == 0) {
        return;
    }
    if (world->terrain == NULL) {
        return;
    }
    LevSafePlatID[0] = FindPlatInst(NuSpecialGetInstanceix(&LevHSpecial[0]));
}

void SarlaccPitB_Init(WORLDINFO_s *) {
}

void SarlaccPitB_Reset(WORLDINFO_s *) {
}

void SarlaccPitB_Update(WORLDINFO_s *) {
}

void SarlaccPitB_SpecialUpdate(WORLDINFO_s *) {
}

void SarlaccPitC_Init(WORLDINFO_s *) {
}

void SarlaccPitC_Reset(WORLDINFO_s *) {
}

void SarlaccPitC_Update(WORLDINFO_s *) {
}

bool SarlaccPitDiscoActive(WORLDINFO_s *) {
    // Disco-state behavior remains unreconstructed.
    return false;
}

// ===========================================================================
// Endor battle (EndorBattle_A / C)
// ===========================================================================

void EndorBattleA_Init(WORLDINFO_s *) {
}

void EndorBattleC_Init(WORLDINFO_s *) {
}

void EndorBattleA_Update(WORLDINFO_s *) {
}

// ===========================================================================
// Death Star 2 battle
// ===========================================================================

void DeathStar2BattleD_Init(WORLDINFO_s *) {
}

void DeathStar2BattleD_Update(WORLDINFO_s *) {
}

void DeathStar2BattleD_InZapRange(GameObject_s *) {
}

void DeathStar2BattleA_AlwaysUpdate(WORLDINFO_s *) {
}

// ===========================================================================
// Emperor fight (EmperorFight_A)
// ===========================================================================

struct EmperorFightAPacket {
    i32 state;
    u8 enabled;
    u8 zap;
    u8 player_floor_mask;
    u8 reserved;
};
DECOMP_ASSERT(sizeof(EmperorFightAPacket) == 8, "Emperor fight packet ABI");
EmperorFightAPacket *emperorfighta_netpacket;
f32 eFloor_timer;
i32 floor_route_on;
static u64 routemask_efloor_on;
static u64 routemask_efloor_off;
static nuhspecial_s *hspecial_efloor;
extern u8 LevFlag[16];

void EmperorFightA_Init(WORLDINFO_s *world) {
    emperorfighta_netpacket = static_cast<EmperorFightAPacket *>(SetLevelHack(8));
    emperorfighta_netpacket->enabled = 1;
    NuSpecialFind(world->current_gscn, &LevHSpecial[3], "lift_rlight_bon1", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[4], "lift_rlight_bon2", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[5], "lift_rlight_bon3", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[6], "lift_rlight_bon4", 1);
    LevFlag[1] = 0;
    LevFlag[2] = 0;
    LevFlag[3] = 0;
    LevFlag[4] = 0;
    NuSpecialFind(world->current_gscn, &LevHSpecial[7], "lift_rarr_on1", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[8], "lift_rarr_on2", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[9], "lift_rarr_on3", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[10], "lift_rarr_on4", 1);
    LevFlag[5] = 0;
    LevFlag[6] = 0;
    LevFlag[7] = 0;
    LevFlag[8] = 0;
    NuSpecialFind(world->current_gscn, &LevHSpecial[30], "dot_on_01", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[31], "dot_on_02", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[32], "dot_on_03", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[33], "dot_on_04", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[34], "dot_on_05", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[35], "dot_on_06", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[36], "dot_on_07", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[37], "dot_on_08", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[38], "dot_on_09", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[39], "dot_on_10", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[40], "dot_on_11", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[41], "dot_on_12", 1);
    reinterpret_cast<u8 *>(LevSfxFlag)[0] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[1] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[2] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[3] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[4] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[5] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[6] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[7] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[8] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[9] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[10] = 0;
    reinterpret_cast<u8 *>(LevSfxFlag)[11] = 0;
    GIZOBSTACLE_s *obstacle = GizObstacle_FindByName(world->giz_obstacle_sys, "Obstacle25");
    if (obstacle != NULL)
        obstacle->field_a1_0xa1 |= 1;
    {
        GIZMOBLOWUP_s *blowup = GizmoBlowUp_FindByName(world, "Deton_011");
        if (blowup != NULL)
            blowup->draw_flags |= 2;
    }
    {
        GIZMOBLOWUP_s *blowup = GizmoBlowUp_FindByName(world, "Deton_031");
        if (blowup != NULL)
            blowup->draw_flags |= 2;
    }
    {
        GIZMOBLOWUP_s *blowup = GizmoBlowUp_FindByName(world, "thermo_light1");
        if (blowup != NULL)
            blowup->draw_flags |= 2;
    }
    char name[10];
    {
        sprintf(name, "force%d", 23);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 24);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 25);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 26);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 27);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 28);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 29);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 30);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 31);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
    {
        sprintf(name, "force%d", 32);
        GIZFORCE_s *force = GizForce_FindByName(world->giz_force_sys, name);
        if (force != NULL)
            force->force_strength = 30.0f;
    }
}

extern i32 obstacle_gizmotype_id, force_gizmotype_id;
void EmperorFightA_Reset(WORLDINFO_s *world) {
    LevAIMessage[0] = CheckGizAIMessage(gizaimessagesys, "ShowHearts", NULL);
    LevAIMessage[1] = CheckGizAIMessage(gizaimessagesys, "ElectricFloor", NULL);
    GIZMO *gizmo = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, "obstacle21");
    if (gizmo != NULL && gizmo->object != NULL)
        LevGizObst[0] = static_cast<GIZOBSTACLE_s *>(gizmo->object);
    LevGizmo[0] = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "force15");
    LevGizmo[1] = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "force16");
    NuSpecialFind(world->current_gscn, &LevHSpecial[0], "lift_force_lnull1", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[1], "lift_force_rnull1", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[2], "lift_main1", 1);
    LevArea[0] = AISysFindArea(world->ai_sys, "ElectricFloor");
    routemask_efloor_on = 0;
    routemask_efloor_off = 0;
    eFloor_timer = 0.0f;
    emperorfighta_netpacket->state = 0;
    for (i32 i = 0; i < world->ai_sys->path_sys->active_path->route_count; ++i) {
        if (NuStrICmp(world->ai_sys->path_sys->active_path->routes[i].name, "electric_on") == 0)
            routemask_efloor_on = static_cast<u64>(1) << (i & 63);
        else if (NuStrICmp(world->ai_sys->path_sys->active_path->routes[i].name, "electric_off") == 0)
            routemask_efloor_off = static_cast<u64>(1) << (i & 63);
        if (routemask_efloor_on != 0 && routemask_efloor_off != 0)
            break;
    }
    hspecial_efloor = &LevHSpecial[67];
    for (i32 i = 0; i < 20; ++i) {
        char name[32];
        if (i < 9)
            sprintf(name, "elecpad_0%d_on", i + 1);
        else
            sprintf(name, "elecpad_%d_on", i + 1);
        NuSpecialFind(world->current_gscn, &hspecial_efloor[i], name, 1);
    }
    gizmo = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, "eFloor_AlwaysOn");
    if (gizmo != NULL && gizmo->object != NULL)
        LevGizObst[1] = static_cast<GIZOBSTACLE_s *>(gizmo->object);
    gizmo = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, "eFloor_AlwaysOf");
    if (gizmo != NULL && gizmo->object != NULL)
        LevGizObst[2] = static_cast<GIZOBSTACLE_s *>(gizmo->object);
    gizmo = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, "eFloor_OnOff");
    if (gizmo != NULL && gizmo->object != NULL)
        LevGizObst[3] = static_cast<GIZOBSTACLE_s *>(gizmo->object);
}

static inline u64 EmperorFloorAreaMask(WORLDINFO_s *world) {
    i32 index = static_cast<i32>(LevArea[0] - world->ai_sys->areas);
    return static_cast<u64>(static_cast<i64>(static_cast<i32>(1u << (index & 31))));
}
void EmperorFightA_Update(WORLDINFO_s *world) {
    if (LevGameObject[0] == NULL)
        LevGameObject[0] = FindGameObject(id_THEEMPEROR, 1, 1, 0, 0);
    if (FreePlay)
        KillBossCompleteLevel(id_THEEMPEROR, 0, 0.0f);
    else
        KillBossPlayCutScene(id_THEEMPEROR, 0, 0.0f, "emperorfight_outro");
    i32 client = netclient;
    EmperorFightAPacket *packet = emperorfighta_netpacket;
    if (!client) {
        packet->player_floor_mask = 0;
        {
            GameObject_s *player = Player[0];
            if (player != NULL && !player->apiobj.field_0x287 && player->field_0x101c <= 0.0f && LevArea[0] != NULL &&
                (player->apiobj.ai_area_mask & EmperorFloorAreaMask(world)))
                packet->player_floor_mask |= 1 << 0;
        }
        {
            GameObject_s *player = Player[1];
            if (player != NULL && !player->apiobj.field_0x287 && player->field_0x101c <= 0.0f && LevArea[0] != NULL &&
                (player->apiobj.ai_area_mask & EmperorFloorAreaMask(world)))
                packet->player_floor_mask |= 1 << 1;
        }
        f32 state = LevAIMessage[1]->value;
        if (state == 1.0f) {
            if (packet->player_floor_mask) {
                if (packet->zap) {
                    eFloor_timer += FRAMETIME;
                    if ((packet->state == 1 || packet->state == 3) && eFloor_timer > 2.0f) {
                        packet->state = 2;
                        eFloor_timer = 0.0f;
                    } else if (packet->state == 2 && eFloor_timer > 3.0f) {
                        packet->state = 3;
                        eFloor_timer = 0.0f;
                    }
                } else {
                    packet->state = 1;
                    packet->zap = 1;
                    eFloor_timer = 0.0f;
                }
            }
        } else if (state == 2.0f) {
            packet->zap = 0;
            packet->state = 4;
        }
    }
    for (i32 i = 0; i < 2; ++i)
        if (Player[i] != NULL && Player[i]->character_context == 0x42)
            Player[i]->character_context = -1;
    if (client && packet->zap && packet->player_floor_mask && LevGameObject[0] != NULL) {
        AILOCATOR *locator = AIPathFindLocator(world->ai_sys, "Zap");
        NUVEC hands[2], end, delta;
        ForceLightning_Origin(LevGameObject[0], &hands[0], &hands[1]);
        end = locator->position;
        end.x += 0.2f - NuRandFloat() * 0.4f;
        end.z += 0.2f - NuRandFloat() * 0.4f;
        for (i32 i = 0; i < 2; ++i) {
            if (hands[i].y != 1000000000.0f) {
                f32 distance = NuVecDist(&end, &hands[i], &delta);
                NuLgtLaser(lightning_type, lightning_sizew[0], lightning_sizel[0], lightning_sizewab[0], &hands[i],
                           &delta, lightning_col[0], lightning_endw[0], distance);
                PlaySfx("ForceLightningLp", &locator->position);
            }
        }
        LevGameObject[0]->ai.movement_look_target = &locator->position;
        packet = emperorfighta_netpacket;
    }
    bool floor_active = false;
    if (hspecial_efloor != NULL && packet->player_floor_mask) {
        for (i32 floor = 0; floor < 20; ++floor) {
            nuinstanim_s *animation = NuSpecialGetInstAnim(&hspecial_efloor[floor]);
            if (animation == NULL || !(animation->ltime > 1.0f))
                continue;
            NUVEC *floor_position = NuSpecialGetDrawPos(&hspecial_efloor[floor]);
            floor_active = true;
            for (i32 i = 0; i < 2; ++i) {
                GameObject_s *player = Player[i];
                if (player->apiobj.field_0x287 || !(player->field_0x101c <= 0.0f) ||
                    !(emperorfighta_netpacket->player_floor_mask & (1 << i)))
                    continue;
                if ((static_cast<i8>(player->apiobj.object_flags) < 0) && Player_HasInvincibility(player))
                    continue;
                player = Player[i];
                if (!(player->apiobj.position.y > floor_position->y))
                    continue;
                f32 dx = player->apiobj.position.x - floor_position->x;
                if (!(dx < 0.35f && dx > -0.35f))
                    continue;
                f32 dz = player->apiobj.position.z - floor_position->z;
                if (!(dz < 0.35f && dz > -0.35f))
                    continue;
                player->character_context = 0x42;
                if (!(player->apiobj.field_0x1f4 & 0x40000))
                    ObjHitObj(NULL, player, 1, 0, 0, 1);
                player = Player[i];
                if (dx > 0.0f)
                    player->apiobj.velocity.x += 0.2f;
                else
                    player->apiobj.velocity.x -= 0.2f;
                if (dz > 0.0f)
                    player->apiobj.velocity.z += 0.2f;
                else
                    player->apiobj.velocity.z -= 0.2f;
                NUVEC floor_end, player_end, delta;
                floor_end.x = player->apiobj.collision_position.x;
                f32 random = NuRandFloat();
                floor_end.x += 0.2f - (random + random) * 0.2f;
                if (floor_end.x > floor_position->x + 0.35f)
                    floor_end.x = floor_position->x + 0.35f;
                else if (floor_position->x - 0.35f > floor_end.x)
                    floor_end.x = floor_position->x - 0.35f;
                floor_end.y = floor_position->y;
                floor_end.z = Player[i]->apiobj.collision_position.z;
                random = NuRandFloat();
                floor_end.z = (0.2f - (random + random) * 0.2f) + floor_end.z;
                if (floor_end.z > floor_position->z + 0.35f)
                    floor_end.z = floor_position->z + 0.35f;
                else if (floor_position->z - 0.35f > floor_end.z)
                    floor_end.z = floor_position->z - 0.35f;
                player = Player[i];
                player_end.x = player->apiobj.collision_position.x;
                f32 height = player->apiobj.upper_position.y - player->apiobj.lower_position.y;
                f32 base = player->apiobj.lower_position.y + 0.25f * height;
                player_end.y = base + (height * 0.5f) * (static_cast<f32>(qrand()) * 0.000015259021893143654f);
                player_end.z = Player[i]->apiobj.collision_position.z;
                f32 distance = NuVecDist(&player_end, &floor_end, &delta);
                NuLgtLaser(testlaser_type, testlaser_sizew, testlaser_sizel, testlaser_sizewab, &floor_end, &delta,
                           0xff808040, testlaser_endw, distance);
                NUVEC *sound_position = &Player[i]->apiobj.collision_position;
                i32 sound = GetSfxId("ForceLightningLp");
                GameAudio_PlaySfxById(sound, sound_position, 0, 1);
                break;
            }
        }
        if (floor_active) {
            for (i32 i = 0; i < 2; ++i) {
                GameObject_s *player = Player[i];
                if (!player->apiobj.field_0x287 && player->field_0x101c <= 0.0f && LevArea[0] != NULL &&
                    (player->apiobj.ai_area_mask & EmperorFloorAreaMask(world)) &&
                    player->apiobj.collision_position.x < 1.5f) {
                    player->apiobj.last_safe_position.x = 1.5f;
                    player->field_0xf00 |= 4;
                    player->apiobj.respawn_position.x = 1.5f;
                }
            }
        }
        packet = emperorfighta_netpacket;
    }
    if (!netclient && routemask_efloor_on && routemask_efloor_off) {
        bool update_routes = false;
        if (packet->enabled || (floor_active && !floor_route_on)) {
            floor_route_on = 1;
            packet->enabled = 0;
            update_routes = true;
        } else if (!floor_active && floor_route_on) {
            floor_route_on = 0;
            update_routes = true;
        }
        if (update_routes) {
            AIPATH *path = world->ai_sys->path_sys->active_path;
            for (i32 i = 0; i < path->connection_count; ++i) {
                AIPATHCNX *connection = &path->connections[i];
                if (connection->route_mask & static_cast<u32>(routemask_efloor_on)) {
                    if (floor_route_on) {
                        connection->traversal_flags[0] &= 0x7fffffff;
                        connection->traversal_flags[1] &= 0x7fffffff;
                    } else {
                        connection->traversal_flags[0] |= 0x80000000;
                        connection->traversal_flags[1] |= 0x80000000;
                    }
                } else if (connection->route_mask & static_cast<u32>(routemask_efloor_off)) {
                    if (floor_route_on) {
                        connection->traversal_flags[0] |= 0x80000000;
                        connection->traversal_flags[1] |= 0x80000000;
                    } else {
                        connection->traversal_flags[0] &= 0x7fffffff;
                        connection->traversal_flags[1] &= 0x7fffffff;
                    }
                }
            }
            for (i32 i = 0; i < 2; ++i)
                if (Player[i] != NULL && (Player[i]->apiobj.object_flags & 0x1001) == 0x1001)
                    AISysGetCharacterPathPos(WORLD->ai_sys, &Player[i]->apiobj, &Player[i]->ai, 0xff, 1);
            if (LevGameObject[0] != NULL)
                AISysGetCharacterPathPos(WORLD->ai_sys, &LevGameObject[0]->apiobj, &LevGameObject[0]->ai, 0xff, 1);
            packet = emperorfighta_netpacket;
        }
    }
    switch (packet->state) {
        case 0:
        case 4:
            GizObstacle_PlayBackwards(LevGizObst[1]);
            GizObstacle_PlayBackwards(LevGizObst[2]);
            GizObstacle_PlayBackwards(LevGizObst[3]);
            break;
        case 1:
            GizObstacle_PlayForwards(LevGizObst[1]);
            GizObstacle_PlayForwards(LevGizObst[2]);
            GizObstacle_PlayForwards(LevGizObst[3]);
            break;
        case 2:
            GizObstacle_PlayForwards(LevGizObst[1]);
            GizObstacle_PlayBackwards(LevGizObst[2]);
            GizObstacle_PlayBackwards(LevGizObst[3]);
            break;
        case 3:
            GizObstacle_PlayForwards(LevGizObst[1]);
            GizObstacle_PlayBackwards(LevGizObst[2]);
            GizObstacle_PlayForwards(LevGizObst[3]);
            break;
    }
    if (!LevFlag[4]) {
        {
            if (!LevFlag[1] && NuSpecialGetVisibilityFn(&LevHSpecial[1 + 2])) {
                PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[1 + 2]));
                LevFlag[1] = 1;
            }
        }
        {
            if (!LevFlag[2] && NuSpecialGetVisibilityFn(&LevHSpecial[2 + 2])) {
                PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[2 + 2]));
                LevFlag[2] = 1;
            }
        }
        {
            if (!LevFlag[3] && NuSpecialGetVisibilityFn(&LevHSpecial[3 + 2])) {
                PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[3 + 2]));
                LevFlag[3] = 1;
            }
        }
        {
            if (!LevFlag[4] && NuSpecialGetVisibilityFn(&LevHSpecial[4 + 2])) {
                PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[4 + 2]));
                LevFlag[4] = 1;
            }
        }
    }
    u8 *sound_flags = reinterpret_cast<u8 *>(LevSfxFlag);
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 0]) && !sound_flags[0]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[0]));
            sound_flags[0] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 0]))
            sound_flags[0] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 1]) && !sound_flags[1]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[1]));
            sound_flags[1] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 1]))
            sound_flags[1] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 2]) && !sound_flags[2]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[2]));
            sound_flags[2] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 2]))
            sound_flags[2] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 3]) && !sound_flags[3]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[3]));
            sound_flags[3] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 3]))
            sound_flags[3] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 4]) && !sound_flags[4]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[4]));
            sound_flags[4] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 4]))
            sound_flags[4] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 5]) && !sound_flags[5]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[5]));
            sound_flags[5] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 5]))
            sound_flags[5] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 6]) && !sound_flags[6]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[6]));
            sound_flags[6] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 6]))
            sound_flags[6] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 7]) && !sound_flags[7]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[7]));
            sound_flags[7] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 7]))
            sound_flags[7] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 8]) && !sound_flags[8]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[8]));
            sound_flags[8] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 8]))
            sound_flags[8] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 9]) && !sound_flags[9]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[9]));
            sound_flags[9] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 9]))
            sound_flags[9] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 10]) && !sound_flags[10]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[10]));
            sound_flags[10] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 10]))
            sound_flags[10] = 0;
    }
    {
        if (NuSpecialGetVisibilityFn(&LevHSpecial[30 + 11]) && !sound_flags[11]) {
            PlaySfx("env_block_light_on", NuSpecialGetDrawPos(&LevHSpecial[11]));
            sound_flags[11] = 1;
        } else if (!NuSpecialGetVisibilityFn(&LevHSpecial[30 + 11]))
            sound_flags[11] = 0;
    }
}

void EmperorFightA_Panel(WORLDINFO_s *) {
}

// ===========================================================================
// Fire / slow-down helpers (Death Star 2 fire)
// ===========================================================================

void DeathStar2BattleFire_Draw(WORLDINFO_s *) {
}

void DeathStar2BattleFire_Init(WORLDINFO_s *) {
}

void DeathStar2BattleFire_Update(WORLDINFO_s *) {
}

static f32 slowDownTimer[2];
f32 DeathStar2BattleFire_GetSlowDownMul(GameObject_s *object) {
    if (object != NULL) {
        i32 index = object->apiobj.field_0x27c;
        if ((u8)index < 2) {
            if (slowDownTimer[index] < 0.0f)
                slowDownTimer[index] = 0.0f;
            else
                return (3.0f - slowDownTimer[index]) / 3.0f;
        }
    }
    return 1.0f;
}
void DeathStar2BattleFire_SetSlowDownMul(GameObject_s *object) {
    if (WORLD->current_level != DEATHSTAR2BATTLEE_LDATA && WORLD->current_level != DEATHSTAR2BATTLEF_LDATA &&
        WORLD->current_level != DEATHSTAR2BATTLEG_LDATA)
        return;
    if (object != NULL) {
        i32 index = object->apiobj.field_0x27c;
        if ((u8)index < 2) {
            object->apiobj.velocity = v000;
            slowDownTimer[index] = 3.0f;
        }
    }
}
void DeathStar2BattleFire_UpdateSlowDownMul(f32 dt) {
    for (i32 index = 0; index < 2; ++index)
        if (slowDownTimer[index] > 0.0f)
            slowDownTimer[index] -= dt;
}
