#include <stdio.h>
#include <string.h>
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nufloat.h"

#include "decomp.h"
#include "legoapi/world/level.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "legoapi/gizmo/base/GizForceObjectInterface.h"
#include "legoapi/gizmos/object/newblowup.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/menus/core/gamemessage.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/render/core/render.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuhspecial.h"
#include "nu2api/numath/nurand.h"
#include "gameapi/ai/aisys/aipath.h"
#include "gameapi/ai/aisys/aisys.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/numath/nuang.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/world/mission.h"
#include "legoapi/world/world.h"
#include "legogame/game.h"
#include "nu2api/numath/nutrig.h"

// This level's view of the shared 16-byte LevFlag scratch. byte0 holds the
// bonus-gunship milestone state; byte1 a secondary state.
enum GUNSHIP_STATE_e {
    GUNSHIP_INACTIVE = 0, // never entered
    GUNSHIP_ACTIVE = 1,   // player aboard / stage running
    GUNSHIP_WON = 2,      // stage finished
};
struct GUNSHIP_LEVFLAG_s {
    u8 progress; // 0x00 -> GUNSHIP_STATE_e
    u8 exit;     // 0x01
    u8 pad[14];  // 0x02
};
static_assert(sizeof(struct GUNSHIP_LEVFLAG_s) == 16, "LevFlag must be 16 bytes");
extern struct GUNSHIP_LEVFLAG_s LevFlag;

// --- Cross-file entry points / id globals ---
//
// AIPAthFindPathCnx remains local here because it is called with a different
// argument arity than in episodeI (both are byte-matched as-is), so it cannot
// live in a shared header.

extern "C" {
    void *AIPAthFindPathCnx(AISYS_s *, i32, void *, void *, void *); // legoapi/ai pathfinding
}

// Defined in legoapi/characters/motion/camera.cpp and legoapi/ai/game/creature.cpp;
// neither has a header yet.
i32 OnOrInsidePlane(nuvec_s *point, nuvec_s *plane_point, nuvec_s *plane_normal, nuvec_s *corrected_point,
                    f32 normal_offset, f32 *distance_out);
void RemoveGameObject(GameObject_s *object, i32 immediate);
void ClearAICreatures();
// Defined in gameapi/edtools/edtoolsall.cpp, which has no header yet.
nugspline_s *edSpline_SplineFind(nugscn_s *scene, char *name);

struct JEDIBNETPACKET_s;

// Jedi_B tuning and spawn tables (unmangled globals in the original).
extern "C" {
    // Facing angle applied to each restrained hero when it is teleported.
    i32 jedib_yrot[3] = {0x6000, 0xa000, 0xe000};
    nuvec_s jedib_offsets[3] = {{0.1f, -0.1f, -0.1f}, {-0.1f, -0.1f, -0.1f}, {-0.1f, -0.1f, 0.1f}};
    // Radius around the arena centre inside which a player still draws baddies.
    f32 jedib_safe_r = 9.0f;
    i16 *JediB_playerids[3] = {&id_PADMECLAWED, &id_ANAKINPADAWAN, &id_OBIWANKENOBIJEDIMASTER};
    i32 jedib_n_active;
    // Grid spacing of the initial spawn sweep, in world units.
    f32 jedib_create_step_LowEnd = 3.0f;
    f32 jedib_create_step_Normal = 2.0f;
    // Distance from a goody at which its surrounding baddies are placed.
    f32 jedib_proximity = 0.5f;
    // Random jitter applied to each grid point.
    f32 jedib_offset = 0.5f;
    // Only grid points between these radii from the arena centre are used.
    f32 jedib_inner = 3.0f;
    f32 jedib_outer_r = 8.0f;
    i32 jedib_max_baddies_per_goody = 3;
    i32 jedib_min_baddies_per_goody = 1;
    // Seed the arena's own random stream starts from, so a level lays out the same way each time.
    u32 jedib_seed = 0x11;
    JEDIBNETPACKET_s *jedib_netpacket;
}

// --- File-local statics (original _ZL... symbols; not renamed) ---------------

// Kamino disco-room state (original _ZL11kaminodisco).
struct KaminoDiscoState {
    u8 pad_0x000[0x3d4];
    u8 initialized;
    u8 mode;
    i8 first_character;
    i8 second_character;
    i32 counter;
    u8 pending;
    u8 pad_0x3dd[0xb];
};
DECOMP_ASSERT(sizeof(KaminoDiscoState) == 0x3e8, "KaminoDiscoState size");
static KaminoDiscoState kaminodisco;
static GIZAIMESSAGE_s *dooku_c; // _ZL7dooku_c
struct dooku_state_s {
    i32 hit_message;
    nuhspecial_s node;
};
static dooku_state_s dooku_state;

// kamino_e level state block and hud scene object.
struct kamino_e_state_s {
    char pad_0x00[0x28];
    f32 field_0x28; // 0x28
};
static struct kamino_e_state_s *kamino_e_state;
static void *kamino_e_special;    // kamino_e named scene object
static void *pursuit_state[0x20]; // bounty-hunter pursuit state
static i16 gunship_bolts[2];      // gun-ship bolt type ids
static u8 gunship_flags[0xa];     // gun-ship weapon-select flags
static void *gunship_weapons[4];  // gun-ship gizmo weapons

struct ZAMARROW_s {
    GameObject_s *object;
    f32 timer;
};

struct PURSUIT_TRAFFIC_ENTRY_s {
    nuhspecial_s special;
    i32 platform_id;
    f32 animation_frame;
    u8 pad_0x14[0x139 - 0x14];
    u8 active;
    i8 direction;
    u8 pad_0x13b[0x140 - 0x13b];
};

struct PURSUIT_TRAFFIC_s {
    PURSUIT_TRAFFIC_ENTRY_s entries[95];
    u8 pad_0x76c0[0x77e0 - 0x76c0];
    i8 count;
    u8 pad_0x77e1[2];
    i8 side;
};

struct PURSUIT_ARROW_COLOURS_s {
    u8 pad_0x0000[0x1340];
    u32 colour[3];
    u8 pad_0x134c[2];
    u8 enabled;
};

ZAMARROW_s zamarrow;
i32 pursuit_c_hack = 1;
f32 traffic_test_z = -360.0f;

// Episode 2 level handlers, in the game's Episode_II progression:
// pursuit (coruscant bounty-hunter) / kamino / factory (geonosis droid
// factory) / jedi / gunship / bonus gunship / dooku, then the NewTown bonus.
//
// The bounty-hunter pursuit and bonus-gunship functions came from a separate
// pursuit.cpp translation unit in the original binary (_GLOBAL__sub_I_pursuit.cpp);
// JediB currently lives here per the Episode II level grouping.

// ===========================================================================
// Coruscant — bounty-hunter pursuit (Zam Wesell)
// ===========================================================================

void BountyHunterPursuitA_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "Jango")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "b1")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "b2")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "b3")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "b4")) != NULL)
        b->field_0x9f |= 0x20;
}

void BountyHunterPursuitB_Init(WORLDINFO_s *) {
}

void BountyHunterPursuitC_Init(WORLDINFO_s *) {
}

void BountyHunterPursuitD_Init(WORLDINFO_s *) {
}

void BountyHunterPursuitA_Reset(WORLDINFO_s *world) {
    pursuit_state[0] = 0;
    pursuit_state[1] = 0;
    pursuit_state[0] = GetNamedGameObject(world->ai_sys, "pursuit_a");
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "za1")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "za2")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "za3")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "za4")) != NULL)
        b->field_0x9f |= 0x20;
    if ((b = GizmoBlowUp_FindByName(world, "za5")) != NULL)
        b->field_0x9f |= 0x20;
}

void BountyHunterPursuitB_Reset(WORLDINFO_s *world) {
    LevGizmo[0] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitb_1");
    LevGizmo[1] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitb_2");
    LevGizmo[2] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitb_3");
    LevGizmo[3] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitb_4");
    LevGizmo[4] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitb_5");
    LevGizmo[5] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitb_6");
    pursuit_state[0] = GetNamedGameObject(world->ai_sys, "pursuitb_exit");
}

static void UpdateZamArrow(WORLDINFO_s *world) {
    NUVEC position = zamarrow.object->apiobj.upper_position;
    position.y += 1.0f;
    GAMEMESSAGE_s *message = static_cast<GAMEMESSAGE_s *>(
        AddGameMessage(const_cast<char *>(" "), &position, 0.05f, NULL, 0.0f, 255, 63, 63, 0x10083, 0.0f));
    if (message == NULL) {
        return;
    }

    message->icon = 0x134;
    message->alpha = static_cast<u8>(NU_SIN_LUT(static_cast<i32>(zamarrow.timer * 16384.0f)) * 128.0f);
    PURSUIT_ARROW_COLOURS_s *colours = reinterpret_cast<PURSUIT_ARROW_COLOURS_s *>(world->lev_objs);
    if (colours->enabled != 0) {
        message->color1 = colours->colour[0];
        message->color2 = colours->colour[1];
        message->color3 = colours->colour[2];
    }
}

void BountyHunterPursuitC_Reset(WORLDINFO_s *world) {
    zamarrow = {};
    zamarrow.object = GetNamedGameObject(world->ai_sys, const_cast<char *>("ai_zam"));

    PURSUIT_TRAFFIC_s *traffic = reinterpret_cast<PURSUIT_TRAFFIC_s *>(world->trafficanim_sys);
    if (traffic == NULL) {
        return;
    }

    traffic->side = 0;
    PURSUIT_TRAFFIC_ENTRY_s *entry = traffic->entries;
    for (i32 i = 0; i < traffic->count; ++i, ++entry) {
        if (pursuit_c_hack == 0) {
            entry->direction = 0;
            continue;
        }

        NUMTX first;
        NUMTX last;
        EvalAnim(&entry->special, 1.0f, &first, 1);
        EvalAnim(&entry->special, entry->animation_frame, &last, 1);
        if (traffic_test_z > first.m32) {
            entry->direction = traffic_test_z > last.m32 ? -1 : 0;
        } else {
            entry->direction = last.m32 > traffic_test_z ? 1 : 0;
        }
    }
}

void BountyHunterPursuitD_Reset(WORLDINFO_s *world) {
    pursuit_state[0] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_1");
    pursuit_state[1] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_2");
    pursuit_state[2] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_3");
    pursuit_state[3] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_4");
    pursuit_state[4] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_5");
    pursuit_state[5] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_6");
    pursuit_state[6] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_7");
    pursuit_state[7] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_8");
    pursuit_state[8] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_9");
    pursuit_state[9] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_10");
    pursuit_state[10] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_11");
    pursuit_state[11] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "pursuitd_12");
    pursuit_state[12] = GetNamedGameObject(world->ai_sys, "pursuitd_last");
}

void BountyHunterPursuitA_Update(WORLDINFO_s *world) {
    if (zamarrow.object != NULL && (zamarrow.object->apiobj.field_0x1f8 & 0x1000) != 0 &&
        zamarrow.object->apiobj.field_0x287 == 0) {
        if (FadeSys.fade == 0.0f && pause_rndr_on == 0) {
            zamarrow.timer = MIN(1.0f, zamarrow.timer + FRAMETIME + FRAMETIME);
            if (0.1f > NuFmod(GameTimer.time_elapsed_mod_seconds, 0.2f)) {
                UpdateZamArrow(world);
            }
        } else {
            zamarrow.timer = 0.0f;
        }
    }
}

void BountyHunterPursuitB_Update(WORLDINFO_s *world) {
    if (LevAIMessage[0] == NULL || LevAIMessage[0]->value != 1.0f || zamarrow.object == NULL ||
        (zamarrow.object->apiobj.field_0x1f8 & 0x1000) == 0 || zamarrow.object->apiobj.field_0x287 != 0) {
        return;
    }

    if (FadeSys.fade == 0.0f && pause_rndr_on == 0) {
        zamarrow.timer = MIN(1.0f, zamarrow.timer + FRAMETIME + FRAMETIME);
        if (0.1f > NuFmod(GameTimer.time_elapsed_mod_seconds, 0.2f)) {
            UpdateZamArrow(world);
        }
    } else {
        zamarrow.timer = 0.0f;
    }
}

void BountyHunterPursuitC_Update(WORLDINFO_s *world) {
    if (zamarrow.object != NULL && (zamarrow.object->apiobj.field_0x1f8 & 0x1000) != 0 &&
        zamarrow.object->apiobj.field_0x287 == 0) {
        if (FadeSys.fade == 0.0f && pause_rndr_on == 0) {
            zamarrow.timer = MIN(1.0f, zamarrow.timer + FRAMETIME + FRAMETIME);
            if (0.1f > NuFmod(GameTimer.time_elapsed_mod_seconds, 0.2f)) {
                UpdateZamArrow(world);
            }
        } else {
            zamarrow.timer = 0.0f;
        }
    }

    PURSUIT_TRAFFIC_s *traffic = reinterpret_cast<PURSUIT_TRAFFIC_s *>(world->trafficanim_sys);
    if (traffic == NULL || player == NULL) {
        return;
    }

    if (traffic_test_z <= player->apiobj.position.z) {
        if (traffic->side == 1) {
            return;
        }
        traffic->side = 1;
    } else {
        if (traffic->side == -1) {
            return;
        }
        traffic->side = -1;
    }

    PURSUIT_TRAFFIC_ENTRY_s *entry = traffic->entries;
    for (i32 i = 0; i < traffic->count; ++i, ++entry) {
        if (traffic_test_z > player->apiobj.position.z) {
            entry->active = entry->direction == 1;
        } else {
            entry->active = entry->direction == -1;
        }
    }
}

void BountyHunterPursuitD_Update(WORLDINFO_s *) {
}

// ===========================================================================
// Kamino
// ===========================================================================

i32 KaminoInside() {
    if (WORLD->area != NULL && WORLD->area == KAMINO_ADATA) {
        if (WORLD->current_level == KAMINOA_LDATA) {
            if (CUTSTOPGAME == 0) {
                if (GameCam->sock_position.location.sock == 5)
                    return 1;
                if (GameCam->sock_position.location.sock == 6)
                    return 1;
            } else {
                return 1;
            }
        } else if (WORLD->current_level == KAMINOE_LDATA) {
            if (CUTSTOPGAME == 0 && GameCam->sock_position.location.sock != 0x1e)
                return 1;
        }
    }
    return 0;
}

i32 KaminoDiscoOn() {
    return kaminodisco.mode == 2;
}

i32 KaminoInDiscoRoom() {
    i32 r = 0;
    if (WORLD->current_level == KAMINOC_LDATA)
        r = (GameCam->sock_position.location.sock == 0x15);
    return r;
}

void KaminoA_AlwaysUpdate(WORLDINFO_s *) {
    bool v = 0;
    if (CUTSTOPGAME == 0) {
        u8 b = GameCam->sock_position.location.sock;
        if (b != 5)
            v = (b != 6);
    }
    object_switches[1] = v;
}

void KaminoC_Init(WORLDINFO_s *) {
    memset(&kaminodisco, 0, sizeof(kaminodisco));
}

void KaminoC_Reset(WORLDINFO_s *) {
}

void KaminoC_Update(WORLDINFO_s *) {
}

void KaminoD_Init(WORLDINFO_s *world) {
    for (i32 i = 1; i < 13; i++) {
        char buf[0x10];
        sprintf(buf, "DOT%i", i);
        GIZOBSTACLE_s *g = GizObstacle_FindByName(world->giz_obstacle_sys, buf);
        if (g->field_0x3c != 0.0f) {
            break;
        }

        g->field_0x3c = 13.5f;
    }

    GIZMOBLOWUP_s *target = GizmoBlowUp_FindByName(world, "target_a11");
    if (target != NULL) {
        target->field_0x124 = 1;
    }
}

void KaminoE_Init(WORLDINFO_s *world) {
    kaminoe_netpacket = SetLevelHack(0x14);
    GIZMO_s *g = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "Force");
    if (g != NULL)
        LevForce = *(i32 *)g;
}

void KaminoE_Reset(WORLDINFO_s *) {
}

void KaminoE_Update(WORLDINFO_s *) {
}

void KaminoE_AlwaysUpdate(WORLDINFO_s *) {
    bool v = 1;
    if (CUTSTOPGAME == 0)
        v = (GameCam->sock_position.location.sock == 0x1e);
    object_switches[1] = v;
}

void KaminoE_Draw(WORLDINFO_s *world) {
    if (netclient == 0) {
        if (kamino_e_state != NULL && kamino_e_state->field_0x28 > 0.0f) {
            GameObject_s *obj = (GameObject_s *)FindGameObject((i32)(i16)id_JANGOFETT, 1, 1, 1, 0);
            if (obj != NULL && kamino_e_state != NULL && obj->apiobj.anim_packet.time_secondary == 1.0f)
                DrawBossHitPoints(obj);
        }
    }
    NuSpecialSetDrawMtx(&kamino_e_special, NuSpecialGetDrawMtx(&kamino_e_special));
    NuSpecialSetVisibility(&kamino_e_special, 1);
}

void KaminoE_CheckPlatHit(BOLT_s *) {
}

void KaminoF_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "f1")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
    }
    if ((b = GizmoBlowUp_FindByName(world, "f2")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
    }
    if ((b = GizmoBlowUp_FindByName(world, "f3")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
    }
}

void KaminoOutro_Init(WORLDINFO_s *) {
    bool v = 0;
    if (CUTSTOPGAME == 0) {
        u8 b = GameCam->sock_position.location.sock;
        if (b != 5)
            v = (b != 6);
    }
    object_switches[1] = v;
}

void NbKaminoA_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "nb1")) != NULL) {
        b->field_0x128 = 1.0f;
        b->field_0x124 = 1;
    }
    if ((b = GizmoBlowUp_FindByName(world, "nb2")) != NULL) {
        b->field_0x128 = 1.0f;
        b->field_0x124 = 1;
    }
}

// ===========================================================================
// Geonosis — droid factory (Factory_B / Factory_G)
// ===========================================================================

void FactoryB_Init(WORLDINFO_s *world) {
    factoryb_netpacket = SetLevelHack(0x4);
    InitPaintPuzzle(world);
    LevGizObst[0] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv1");
    LevGizObst[1] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv2");
    LevGizObst[2] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv3");
    LevGizObst[3] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv4");
    LevGizObst[4] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv5");
    LevGizObst[5] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv6");
    LevGizObst[6] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv7");
    LevGizObst[7] = GizObstacle_FindByName(world->giz_obstacle_sys, "conv8");
    for (i32 i = 0; i < 8; i++) {
        if (LevGizObst[i] != NULL) {
            LevGizObst[i]->mode = 3;
            LevGizObst[i]->state = 0;
        }
    }
}

void FactoryB_Reset(WORLDINFO_s *world) {
    ResetPaintPuzzle(world);
    factoryb_cut = NewCutScene(NULL, world->cutscene_sys, "fb_cut", 0);
    if (factoryb_cut != NULL) {
        CUTSCENEDATA_s *scene = (CUTSCENEDATA_s *)factoryb_cut->scene;
        if (scene != NULL) {
            scene->field_0x88 |= 2;
            scene->field_0x88 |= 8;
        }
    }
    factoryb_conveyor_stopped_msg = CheckGizAIMessage(gizaimessagesys, "conv_stopped", NULL);
}

void FactoryB_Update(WORLDINFO_s *) {
}

void FactoryB_Draw(WORLDINFO_s *) {
    DrawPaintLights();
}

void FactoryG_Init(WORLDINFO_s *world) {
    if (netclient != 0)
        return;
    GIZMO *g = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "force_g1");
    if (g != NULL)
        force_array[0] = (GIZFORCE_s *)g->object;
    g = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "force_g2");
    if (g != NULL)
        force_array[1] = (GIZFORCE_s *)g->object;
    g = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "force_g3");
    if (g != NULL)
        force_array[2] = (GIZFORCE_s *)g->object;
    g = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, "force_g4");
    if (g != NULL)
        force_array[3] = (GIZFORCE_s *)g->object;
}

void FactoryG_Update(WORLDINFO_s *world) {
    if (netclient != 0)
        return;
    i32 complete = 0;
    if (GizForce_Complete(force_array[0]))
        complete++;
    if (GizForce_Complete(force_array[1]))
        complete++;
    if (GizForce_Complete(force_array[2]))
        complete++;
    if (GizForce_Complete(force_array[3]))
        complete++;
    if (ObiWan == NULL) {
        ObiWan = (GameObject_s *)FindGameObject((i32)(i16)id_OBIWANKENOBIJEDIMASTER, 0x400, 0, 1, 0);
        return;
    }
    if (complete == 4) {
        if (FreePlay == 0)
            NewCutScene(NULL, world->cutscene_sys, "factory_escape", 1);
    } else {
        ObiWan->apiobj.position = {79.2f, 0.75f, -10.5f};
    }
}

// ===========================================================================
// Jedi (Jedi_B)
// ===========================================================================

// One generated combatant slot. The record outlives its GameObject_s: a baddie
// that drifts outside the play planes is removed and its transform written back
// to `position` / `angle`, so the same slot can spawn it again once it is back
// in range.
struct JEDIB_CREATURE_s {
    nuvec_s position;          // 0x00, respawn position
    i32 angle;                 // 0x0c
    i32 id;                    // 0x10, character id, -1 picks one at spawn time
    GameObject_s *object;      // 0x14
    JEDIB_CREATURE_s *partner; // 0x18, opponent handed over when this slot has none
    AILOCATOR locator;         // 0x1c
    i32 field_0x58;            // 0x58
    // 0x5c. The bits are written individually (the original stores them as
    // bitfields); `flags` with JEDIB_CREATURE_FLAGS masks is for combined tests.
    union {
        u8 flags;
        struct {
            u8 spawned_behind : 1;
            u8 random_type : 1;
            u8 two_row_hp : 1;
            u8 in_wave : 1;
            u8 released : 1;
        };
    };
    u8 pad_0x5d[3];
};
DECOMP_ASSERT(sizeof(JEDIB_CREATURE_s) == 0x60, "JEDIB_CREATURE_s ABI");

enum JEDIB_CREATURE_FLAGS : u8 {
    JEDIB_CREATURE_SPAWNED_BEHIND = 0x01,
    JEDIB_CREATURE_RANDOM_TYPE = 0x02,
    JEDIB_CREATURE_TWO_ROW_HP = 0x04,
    JEDIB_CREATURE_IN_WAVE = 0x08,
    JEDIB_CREATURE_RELEASED = 0x10,
};

// A phase spawn list, terminated by a null id.
struct JEDIB_PHASE_s {
    i16 *id;
    char *script;
};

// Jedi_B arena state (original _ZL6jedi_b).
struct JEDIB_s {
    JEDIB_CREATURE_s baddies[256];       // 0x0000
    JEDIB_CREATURE_s goodies[8];         // 0x6000
    i16 baddie_count;                    // 0x6300
    i16 goody_count;                     // 0x6302
    i16 phase;                           // 0x6304
    i16 state;                           // 0x6306
    f32 timer;                           // 0x6308
    u32 seed;                            // 0x630c
    nuhspecial_s hero_spawns[3][4];      // 0x6310
    GameObject_s *heroes[3];             // 0x63a0
    GIZAIMESSAGE_s *msg_phase;           // 0x63ac
    GIZAIMESSAGE_s *msg_phase_complete;  // 0x63b0
    GIZAIMESSAGE_s *msg_objectives_left; // 0x63b4
    GIZAIMESSAGE_s *msg_restrain[3];     // 0x63b8
    i16 wave_ids[6];                     // 0x63c4
    char wave_spawned[6];                // 0x63d0
    u8 pad_0x63d6[2];
    GameObject_s *boss; // 0x63d8
    f32 wave_timer;     // 0x63dc
    // One bit per player that generated baddies must not target, OR'd into each
    // spawned baddie's own exclusion mask.
    union __attribute__((packed, aligned(4))) {
        u64 exclusion_mask; // 0x63e0
        struct {
            u32 exclusion_mask_low;
            u32 exclusion_mask_high;
        };
    };
    u8 unique_spawned; // 0x63e8, the one-off Luminara / Shaak Ti slots
    u8 pad_0x63e9[3];
};
DECOMP_ASSERT(sizeof(JEDIB_s) == 0x63ec, "JEDIB_s ABI");

static JEDIB_s jedi_b;

// Panel state the host mirrors to clients (jedib_netpacket, SetLevelHack slot of 0x20 bytes).
struct JEDIBNETPACKET_s {
    i16 state; // 0x00
    i16 phase; // 0x02
    i16 count; // 0x04
    i16 pad_0x06;
    i16 ids[8];      // 0x08
    char spawned[8]; // 0x18
};
DECOMP_ASSERT(sizeof(JEDIBNETPACKET_s) == 0x20, "JEDIBNETPACKET_s ABI");

static JEDIB_PHASE_s jedi_b_phase1[8] = {
    {&id_DROIDEKA, "phase_droids"},
    {&id_DROIDEKA, "phase_droids"},
    {&id_BATTLEDROIDSECURITY, "phase_droids"},
    {&id_BATTLEDROIDSECURITY, "phase_droids"},
    {&id_BATTLEDROIDSECURITY, "phase_droids"},
    {&id_BATTLEDROIDSECURITY, "phase_droids"},
    {&id_BATTLEDROIDSECURITY, "phase_droids"},
    {NULL, NULL},
};

static JEDIB_PHASE_s jedi_b_phase2[8] = {
    {&id_DROIDEKA, "phase_droids"},         {&id_DROIDEKA, "phase_droids"},
    {&id_SUPERBATTLEDROID, "phase_droids"}, {&id_SUPERBATTLEDROID, "phase_droids"},
    {&id_SUPERBATTLEDROID, "phase_droids"}, {&id_SUPERBATTLEDROID, "phase_droids"},
    {&id_SUPERBATTLEDROID, "phase_droids"}, {NULL, NULL},
};

static JEDIB_PHASE_s jedi_b_phase3[8] = {
    {&id_SUPERBATTLEDROID, "phase_droids"}, {&id_SUPERBATTLEDROID, "phase_droids"},
    {&id_DROIDEKA, "phase_droids"},         {&id_DROIDEKA, "phase_droids"},
    {&id_DROIDEKA, "phase_droids"},         {&id_DROIDEKA, "phase_droids"},
    {&id_DROIDEKA, "phase_droids"},         {NULL, NULL},
};

// Pushes `position` out of any antinode it falls inside, then resolves it to a
// path point and fills in `locator` for it. Returns 0 when the point is off the
// arena or off the path network.
static i32 JediBInitLocator(WORLDINFO_s *world, nuvec_s *position, i32 angle, AILOCATOR *locator, f32 radius) {
    if (netclient != 0) {
        return 0;
    }
    if (world->ai_sys != NULL) {
        i32 count = world->ai_sys->antinode_count;
        AIANTINODE *node = world->ai_sys->antinodes;
        for (i32 index = 0; index < count; index++, node++) {
            if (node->radius != 0.0f) {
                f32 dx = position->x - node->position.x;
                f32 dz = position->z - node->position.z;
                f32 dist_sqr = dx * dx + dz * dz;
                f32 r = node->radius + 1.0f;
                if (dist_sqr < r * r) {
                    f32 scale = r / NuFsqrt(dist_sqr);
                    position->x = node->position.x + dx * scale;
                    position->z = node->position.z + dz * scale;
                    break;
                }
            }
        }
    }
    memset(locator, 0, sizeof(AILOCATOR));
    if (!(position->x * position->x + position->z * position->z < radius * radius)) {
        return 0;
    }
    f32 floor_y = GameShadow(NULL, position, 5.0f, 0);
    if (floor_y != 2000000.0f) {
        position->y = floor_y;
    }
    AISysGetPathPos(world->ai_sys, position, &locator->path_info, NULL, 0xff);
    if ((locator->path_info.flags & AIPATHINFO_FLAG_ON_PATH) == 0) {
        return 0;
    }
    locator->position = *position;
    locator->direction = angle;
    return 1;
}

// Killed-object callback installed on every generated Jedi_B combatant
// (original _ZL19JediBKilledCallbackP12GameObject_s).
static void JediBKilledCallback(GameObject_s *object) {
    i32 index;
    if (object == NULL) {
        return;
    }
    for (index = 0; index < jedi_b.goody_count; index++) {
        if (jedi_b.goodies[index].object == object) {
            jedi_b.goodies[index].object = NULL;
            i32 alive = 0;
            for (i32 other = 0; other < jedi_b.goody_count; other++) {
                if (jedi_b.goodies[other].object != NULL) {
                    alive++;
                }
            }
            if (alive == 0) {
                jedi_b.goody_count = 0;
            }
            return;
        }
    }
    for (index = 0; index < jedi_b.baddie_count; index++) {
        if (jedi_b.baddies[index].object == object) {
            // Low-end levels only run the first four waves.
            i32 wave;
            for (wave = 0; wave < 6; wave++) {
                if (g_lowEndLevelBehaviour != 0 && wave >= 4) {
                    break;
                }
                if (jedi_b.wave_ids[wave] == jedi_b.baddies[index].id && jedi_b.wave_spawned[wave] == 0) {
                    jedi_b.wave_spawned[wave] = 1;
                    jedi_b.wave_timer = 0.0f;
                    break;
                }
            }
            jedi_b.baddies[index].object = NULL;
            jedi_b.baddies[index].spawned_behind = 1;
            jedi_b.baddies[index].in_wave = 0;
            jedi_b.baddies[index].released = 0;
            JEDIB_CREATURE_s *baddie = &jedi_b.baddies[index];
            if (baddie->random_type != 0) {
                if (FreePlay == 0 && (jedi_b.unique_spawned & 1) == 0) {
                    baddie->id = id_LUMINARA;
                    jedi_b.unique_spawned |= 1;
                } else if (FreePlay == 0 && (jedi_b.unique_spawned & 2) == 0) {
                    baddie->id = id_SHAAKTI;
                    jedi_b.unique_spawned |= 2;
                } else if (NuRandFloat() < 0.5f) {
                    baddie->id = id_BATTLEDROIDGEONOSIAN;
                } else {
                    baddie->id = id_GEONOSIAN;
                }
            } else {
                baddie->id = id_BOB;
            }
            baddie->two_row_hp = 0;
            baddie->in_wave = 0;
            baddie->released = 0;
            baddie->field_0x58 = 0;
            return;
        }
    }
}

void JediB_Init(WORLDINFO_s *world) {
    if (Mission_Active(MissionSys) != NULL) {
        return;
    }
    memset(&jedi_b, 0, sizeof(jedi_b));
    jedi_b.seed = jedib_seed;
    jedib_netpacket = (JEDIBNETPACKET_s *)SetLevelHack(sizeof(JEDIBNETPACKET_s));
    if (netclient == 0) {
        f32 step = jedib_create_step_Normal;
        if (g_lowEndLevelBehaviour != 0) {
            step = jedib_create_step_LowEnd;
        }
        for (f32 x = -jedib_outer_r; x <= jedib_outer_r; x += step) {
            for (f32 z = -jedib_outer_r; z <= jedib_outer_r; z += step) {
                nuvec_s position;
                AILOCATOR locator;
                position.x = jedib_offset - jedib_offset * 2.0f * NuRandFloatSeeded(&jedi_b.seed) + x;
                position.y = 0.0f;
                position.z = jedib_offset - jedib_offset * 2.0f * NuRandFloatSeeded(&jedi_b.seed) + z;
                f32 dist_sqr = position.x * position.x + position.z * position.z;
                if (dist_sqr < jedib_outer_r * jedib_outer_r && dist_sqr > jedib_inner * jedib_inner) {
                    f32 floor_y = GameShadow(NULL, &position, 5.0f, 0);
                    if (floor_y != 2000000.0f) {
                        position.y = floor_y;
                    }
                    i32 angle = NuRandIntSeeded(&jedi_b.seed);
                    if (JediBInitLocator(world, &position, angle, &locator, jedib_outer_r) != 0 &&
                        jedi_b.baddie_count <= 255) {
                        JEDIB_CREATURE_s *goody = &jedi_b.baddies[jedi_b.baddie_count++];
                        goody->position = position;
                        goody->locator = locator;
                        goody->random_type = 0;

                        // Ring the goody with baddies at jedib_proximity, evenly spaced from a
                        // random starting angle.
                        nuvec_s offset;
                        offset.x = 0.0f;
                        offset.y = 0.0f;
                        offset.z = jedib_proximity;
                        angle = NuRandIntSeeded(&jedi_b.seed);
                        NuVecRotateY(&offset, &offset, NuRandIntSeeded(&jedi_b.seed));
                        i32 count = NuRandIntSeeded(&jedi_b.seed) %
                                        (jedib_max_baddies_per_goody - jedib_min_baddies_per_goody) +
                                    jedib_min_baddies_per_goody;
                        for (i32 index = 0; index < count && jedi_b.baddie_count <= 255; index++) {
                            if (index != 0) {
                                angle = NuAngAdd(angle, 0x10000 / count);
                            }
                            NuVecRotateY(&position, &offset, angle);
                            NuVecAdd(&position, &position, &goody->position);
                            if (JediBInitLocator(world, &position, angle, &locator, jedib_outer_r) != 0) {
                                JEDIB_CREATURE_s *baddie = &jedi_b.baddies[jedi_b.baddie_count++];
                                baddie->random_type = 1;
                                baddie->position = position;
                                baddie->locator = locator;
                                baddie->partner = goody;
                                baddie->angle = angle;
                                goody->partner = baddie;
                            }
                        }
                    }
                }
            }
        }
    }
    nugspline_s *spline = edSpline_SplineFind(world->current_gscn, "teleport_01");
    if (spline != NULL) {
        spline->pts[0].y -= 0.35f;
        spline->pts[1].y = spline->pts[0].y;
    }
    LevBlowUp[0] = GizmoBlowUp_FindByName(world, "Thermo_011");
}

void JediB_Reset(WORLDINFO_s *world) {
    if (Mission_Active(MissionSys) != NULL) {
        return;
    }
    if (netclient != 0) {
        return;
    }
    i32 index;
    for (index = 0; index < jedi_b.baddie_count; index++) {
        jedi_b.baddies[index].object = NULL;
        jedi_b.baddies[index].spawned_behind = 0;
        jedi_b.baddies[index].in_wave = 0;
        jedi_b.baddies[index].released = 0;
        jedi_b.baddies[index].id = -1;
    }
    for (index = 0; index < jedi_b.goody_count; index++) {
        jedi_b.goodies[index].object = NULL;
        jedi_b.goodies[index].spawned_behind = 0;
    }
    ClearAICreatures();
    char name[32];
    for (i32 hero = 0; hero < 3; hero++) {
        sprintf(name, "pillar%d_01a", hero + 1);
        NuSpecialFind(world->current_gscn, &jedi_b.hero_spawns[hero][0], name, 1);
        sprintf(name, "pillar%d_01b", hero + 1);
        NuSpecialFind(world->current_gscn, &jedi_b.hero_spawns[hero][1], name, 1);
        sprintf(name, "pillar%d_01c", hero + 1);
        NuSpecialFind(world->current_gscn, &jedi_b.hero_spawns[hero][2], name, 1);
        sprintf(name, "pillar%d_01ba", hero + 1);
        NuSpecialFind(world->current_gscn, &jedi_b.hero_spawns[hero][3], name, 1);
        jedi_b.heroes[hero] = NULL;
        if (FreePlay == 0) {
            for (i32 slot = 0; slot < 8; slot++) {
                if (Player[slot] != NULL && Player[slot]->id == *JediB_playerids[hero]) {
                    jedi_b.heroes[hero] = Player[slot];
                    break;
                }
            }
        }
    }
    jedi_b.msg_phase = SetGizAIMessage(gizaimessagesys, "Phase", 0.0f, NULL);
    jedi_b.msg_phase_complete = SetGizAIMessage(gizaimessagesys, "PhaseComplete", 0.0f, NULL);
    jedi_b.msg_objectives_left = SetGizAIMessage(gizaimessagesys, "ObjectivesLeft", 0.0f, NULL);
    jedi_b.msg_restrain[0] = SetGizAIMessage(gizaimessagesys, "RestrainPadme", 0.0f, NULL);
    jedi_b.msg_restrain[1] = SetGizAIMessage(gizaimessagesys, "RestrainAnakin", 0.0f, NULL);
    jedi_b.msg_restrain[2] = SetGizAIMessage(gizaimessagesys, "RestrainObiWan", 0.0f, NULL);
    jedi_b.boss = NULL;
    jedi_b.unique_spawned &= ~3;
}

void JediB_Update(WORLDINFO_s *world) {
    if (Mission_Active(MissionSys) != NULL) {
        return;
    }
    if (netclient != 0) {
        return;
    }

    for (i32 hero = 0; hero < 3; hero++) {
        if (FreePlay != 0 && jedi_b.heroes[hero] == NULL) {
            for (i32 spawn = 0; spawn < 4; spawn++) {
                if (NuSpecialGetVisibilityFn(&jedi_b.hero_spawns[hero][spawn]) != 0) {
                    jedi_b.heroes[hero] = AddDynamicCreature(*JediB_playerids[hero],
                                                             NuSpecialGetDrawPos(&jedi_b.hero_spawns[hero][spawn]), 0,
                                                             "Party", NULL, NULL, 1, NULL, NULL, 0, 0);
                }
            }
        }
        if (jedi_b.heroes[hero] == NULL || jedi_b.msg_restrain[hero]->value != 1.0f) {
            continue;
        }
        for (i32 spawn = 0; spawn < 4; spawn++) {
            if (NuSpecialGetVisibilityFn(&jedi_b.hero_spawns[hero][spawn]) == 0) {
                continue;
            }
            jedi_b.heroes[hero]->apiobj.position = *NuSpecialGetDrawPos(&jedi_b.hero_spawns[hero][spawn]);
            NuVecAdd(&jedi_b.heroes[hero]->apiobj.position, &jedi_b.heroes[hero]->apiobj.position,
                     &jedib_offsets[hero]);
            jedi_b.heroes[hero]->apiobj.velocity.x = 0.0f;
            jedi_b.heroes[hero]->saved_position = jedi_b.heroes[hero]->apiobj.position;
            jedi_b.heroes[hero]->apiobj.velocity.y = 0.0f;
            jedi_b.heroes[hero]->apiobj.field_0x276 = static_cast<u16>(jedib_yrot[hero]);
            jedi_b.heroes[hero]->apiobj.velocity.z = 0.0f;
            jedi_b.heroes[hero]->apiobj.respawn_timer = 0.0f;
        }
    }

    jedi_b.exclusion_mask = 0;
    // Not `timer >= 4.0f`: that form compares the other way round and is not
    // equivalent for NaN, and the original tests `4.0f > timer`.
    if (!(jedi_b.timer < 4.0f)) {
        if (jedi_b.phase != 7) {
            if (player->apiobj.pos_x * player->apiobj.pos_x + player->apiobj.pos_z * player->apiobj.pos_z >
                jedib_safe_r * jedib_safe_r) {
                jedi_b.exclusion_mask = 1ULL << player->apiobj.field_0x289;
            }
            if (player2 != NULL &&
                player2->apiobj.pos_x * player2->apiobj.pos_x + player2->apiobj.pos_z * player2->apiobj.pos_z >
                    jedib_safe_r * jedib_safe_r) {
                jedi_b.exclusion_mask |= 1ULL << player2->apiobj.field_0x289;
            }
        } else if (jedi_b.boss != NULL) {
            GameObject_s *target = player2 != NULL && jedi_b.boss->ai.opponent == player2 ? player2 : player;
            jedi_b.exclusion_mask = 1ULL << target->apiobj.field_0x289;
        }
    } else {
        if (player != NULL) {
            jedi_b.exclusion_mask = 1ULL << player->apiobj.field_0x289;
        }
        if (player2 != NULL) {
            jedi_b.exclusion_mask |= 1ULL << player2->apiobj.field_0x289;
        }
    }

    GameObject_s *boss = jedi_b.boss;
    for (i32 slot = 0; slot < 8; slot++) {
        if (Player[slot] == NULL) {
            continue;
        }
        Player[slot]->ai_opponent_exclusion_mask = 0;
        if (jedi_b.phase == 7 && boss != NULL && static_cast<i8>(Player[slot]->apiobj.flags_low) >= 0) {
            Player[slot]->ai_opponent_exclusion_mask = 1ULL << boss->apiobj.field_0x289;
        }
    }

    switch (jedi_b.state) {
        case 1:
            jedi_b.timer += FRAMETIME;
            switch (jedi_b.phase) {
                case 1:
                case 2:
                case 3:
                    jedi_b.msg_objectives_left->value = static_cast<f32>(jedi_b.goody_count);
                    if (jedi_b.msg_phase_complete->value == 1.0f) {
                        jedi_b.timer = 0.0f;
                        jedi_b.state = 2;
                    }
                    break;
                case 4:
                case 5:
                case 6: {
                    // The low-end wave is two slots shorter.
                    i32 wave_done = 0;
                    if (g_lowEndLevelBehaviour != 0) {
                        if (jedi_b.wave_spawned[0] != 0 && jedi_b.wave_spawned[1] != 0 && jedi_b.wave_spawned[2] != 0 &&
                            jedi_b.wave_spawned[3] != 0) {
                            wave_done = 1;
                        }
                    } else {
                        if (jedi_b.wave_spawned[0] != 0 && jedi_b.wave_spawned[1] != 0 && jedi_b.wave_spawned[2] != 0 &&
                            jedi_b.wave_spawned[3] != 0 && jedi_b.wave_spawned[4] != 0 && jedi_b.wave_spawned[5] != 0) {
                            wave_done = 1;
                        }
                    }
                    jedi_b.wave_timer += FRAMETIME;
                    if (jedi_b.wave_timer > 5.0f) {
                        i32 release = -1;
                        for (i32 i = 0; i < jedi_b.baddie_count; i++) {
                            if ((jedi_b.baddies[i].flags & JEDIB_CREATURE_RELEASED) != 0) {
                                release = -1;
                                break;
                            }
                            if ((jedi_b.baddies[i].flags & JEDIB_CREATURE_IN_WAVE) != 0) {
                                release = i;
                            }
                        }
                        if (release != -1) {
                            jedi_b.baddies[release].flags |= JEDIB_CREATURE_RELEASED;
                        }
                        jedi_b.wave_timer = 0.0f;
                    }
                    if (wave_done != 0) {
                        jedi_b.timer = 0.0f;
                        jedi_b.state = 2;
                    }
                    break;
                }
                case 7:
                    if (boss == NULL) {
                        boss = FindGameObject(id_JANGOFETT, 1, 1, 0, 0);
                        jedi_b.boss = boss;
                        if (boss == NULL) {
                            break;
                        }
                    }
                    if (boss->apiobj.field_0x287 != 0 || boss->current_hp == 0) {
                        jedi_b.timer = 0.0f;
                        jedi_b.state = 2;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 2:
            if (jedi_b.timer < 0.1f) {
                jedi_b.timer += FRAMETIME;
                break;
            }
            if (jedi_b.phase > 6) {
                if (FreePlay == 0) {
                    NewLData = JEDI_OUTRO_LDATA;
                }
                CompleteLevel(world);
            }
            jedi_b.timer = 0.0f;
            jedi_b.state = 0;
            break;

        case 0: {
            JEDIB_PHASE_s *phase_lists[3] = {jedi_b_phase1, jedi_b_phase2, jedi_b_phase3};
            if (netclient != 0) {
                break;
            }
            if (jedi_b.phase > 2 && jedi_b.timer < 3.0f) {
                jedi_b.timer += FRAMETIME;
                break;
            }
            jedi_b.phase = static_cast<i16>(jedi_b.phase + 1);
            if (jedi_b.phase == 6) {
                jedi_b.phase = 7;
            }
            switch (jedi_b.phase) {
                case 4:
                case 5:
                case 6: {
                    memset(jedi_b.wave_spawned, 0, sizeof(jedi_b.wave_spawned));
                    for (i32 i = 0; i < jedi_b.baddie_count; i++) {
                        jedi_b.baddies[i].flags &= static_cast<u8>(~(JEDIB_CREATURE_IN_WAVE | JEDIB_CREATURE_RELEASED));
                    }
                    i32 chosen = 0;
                    while (chosen < (g_lowEndLevelBehaviour != 0 ? 4 : 6)) {
                        i32 index = NuRand(NULL) % jedi_b.baddie_count;
                        i32 id;
                        if (jedi_b.phase == 4) {
                            id = id_BATTLEDROIDSECURITY;
                        } else if (jedi_b.phase == 5) {
                            id = (chosen & 1) != 0 ? id_BATTLEDROIDSECURITY : id_SUPERBATTLEDROID;
                        } else {
                            id = (chosen & 1) != 0 ? id_DROIDEKA : id_SUPERBATTLEDROID;
                        }
                        for (;;) {
                            if ((jedi_b.baddies[index].flags & JEDIB_CREATURE_IN_WAVE) == 0 &&
                                (jedi_b.baddies[index].flags & JEDIB_CREATURE_RANDOM_TYPE) != 0 &&
                                jedi_b.baddies[index].id != id) {
                                break;
                            }
                            index++;
                            if (index >= jedi_b.baddie_count) {
                                index = 0;
                            }
                        }
                        if (jedi_b.baddies[index].object != NULL) {
                            RemoveGameObject(jedi_b.baddies[index].object, 1);
                            jedi_b.baddies[index].object = NULL;
                            jedi_b.baddies[index].flags &= static_cast<u8>(~JEDIB_CREATURE_SPAWNED_BEHIND);
                        }
                        jedi_b.baddies[index].id = id;
                        jedi_b.wave_ids[chosen] = static_cast<i16>(id);
                        jedi_b.baddies[index].flags |= JEDIB_CREATURE_IN_WAVE;
                        chosen++;
                    }
                    jedi_b.wave_timer = 0.0f;
                    break;
                }
                case 1:
                case 2:
                case 3:
                    for (JEDIB_PHASE_s *entry = phase_lists[jedi_b.phase - 1];
                         entry != NULL && entry->id != NULL && jedi_b.goody_count <= 7; entry++) {
                        if (g_lowEndLevelBehaviour != 0 && jedi_b.goody_count == 5) {
                            break;
                        }
                        char name[0x10];
                        sprintf(name, "phase%d_%d", jedi_b.phase, jedi_b.goody_count + 1);
                        AILOCATOR *locator = AIPathFindLocator(world->ai_sys, name);
                        if (locator == NULL) {
                            continue;
                        }
                        JEDIB_CREATURE_s *goody = &jedi_b.goodies[jedi_b.goody_count];
                        goody->locator = *locator;
                        goody->object =
                            AddDynamicCreature(*entry->id, &locator->position, locator->direction, entry->script,
                                               &locator->path_info, NULL, 1, NULL, NULL, 0, 0);
                        if (jedi_b.goodies[jedi_b.goody_count].object == NULL) {
                            continue;
                        }
                        GameObject_s *object = jedi_b.goodies[jedi_b.goody_count].object;
                        jedi_b.goodies[jedi_b.goody_count].id = *entry->id;
                        object->field_0xefb |= 1;
                        object->ai.locator = locator;
                        object->field_0xeb4 = JediBKilledCallback;
                        jedi_b.goody_count = static_cast<i16>(jedi_b.goody_count + 1);
                    }
                    break;
                default:
                    break;
            }

            if (jedi_b.msg_phase == NULL) {
                jedi_b.msg_phase = CheckGizAIMessage(gizaimessagesys, "Phase", NULL);
            }
            if (jedi_b.msg_phase_complete == NULL) {
                jedi_b.msg_phase_complete = CheckGizAIMessage(gizaimessagesys, "PhaseComplete", NULL);
            }
            if (jedi_b.msg_objectives_left == NULL) {
                jedi_b.msg_objectives_left = CheckGizAIMessage(gizaimessagesys, "ObjectivesLeft", NULL);
            }
            if (jedi_b.msg_restrain[0] == NULL) {
                jedi_b.msg_restrain[0] = CheckGizAIMessage(gizaimessagesys, "RestrainPadme", NULL);
            }
            if (jedi_b.msg_restrain[1] == NULL) {
                jedi_b.msg_restrain[1] = CheckGizAIMessage(gizaimessagesys, "RestrainAnakin", NULL);
            }
            if (jedi_b.msg_restrain[2] == NULL) {
                jedi_b.msg_restrain[2] = CheckGizAIMessage(gizaimessagesys, "RestrainObiWan", NULL);
            }
            jedi_b.msg_phase->value = static_cast<f32>(jedi_b.phase);
            jedi_b.msg_objectives_left->value = static_cast<f32>(jedi_b.goody_count);
            jedi_b.msg_phase_complete->value = 0.0f;
            jedi_b.timer = 0.0f;
            jedi_b.state = 1;
            break;
        }

        default:
            break;
    }

    jedib_n_active = 0;
    for (i32 i = 0; i < jedi_b.baddie_count; i++) {
        JEDIB_CREATURE_s *baddie = &jedi_b.baddies[i];
        GameObject_s *object = baddie->object;
        if (object != NULL) {
            if ((baddie->flags & JEDIB_CREATURE_RELEASED) != 0) {
                object->ai.locator = NULL;
                if ((player->apiobj.character_data->model_flags & 0x80000) == 0 &&
                    (player->apiobj.objptr->id != id_JARJAR || static_cast<i8>(player->apiobj.flags_low) < 0) &&
                    (player->apiobj.character_data->game_character->uses_weapon_action != 4 ||
                     object->apiobj.character_data->game_character->uses_weapon_action != 4)) {
                    object->ai.opponent = player;
                } else {
                    if (object->ai.opponent == player) {
                        object->ai.opponent = NULL;
                    }
                    if (player2 != NULL) {
                        if ((player2->apiobj.character_data->model_flags & 0x80000) == 0 &&
                            (player->apiobj.objptr->id != id_JARJAR || static_cast<i8>(player->apiobj.flags_low) < 0) &&
                            (player->apiobj.character_data->game_character->uses_weapon_action != 4 ||
                             object->apiobj.character_data->game_character->uses_weapon_action != 4)) {
                            object->ai.opponent = player2;
                        } else if (object->ai.opponent == player2) {
                            object->ai.opponent = NULL;
                        }
                    }
                }
            } else {
                // Evaluated into a flag before the locator checks so the
                // locator test is the fall-through path, as in the original.
                i32 object_in_play = OnOrInsidePlane(&object->apiobj.position, &PlayPlane[1].point,
                                                     &PlayPlane[1].normal, NULL, 2.0f, NULL) != 0 ||
                                     OnOrInsidePlane(&object->apiobj.position, &PlayPlane[2].point,
                                                     &PlayPlane[2].normal, NULL, 2.0f, NULL) != 0;
                if (object_in_play && (OnOrInsidePlane(&baddie->locator.position, &PlayPlane[1].point,
                                                       &PlayPlane[1].normal, NULL, 2.0f, NULL) != 0 ||
                                       OnOrInsidePlane(&baddie->locator.position, &PlayPlane[2].point,
                                                       &PlayPlane[2].normal, NULL, 2.0f, NULL) != 0)) {
                    baddie->position = baddie->object->apiobj.position;
                    baddie->angle = baddie->object->apiobj.field_0x276;
                    RemoveGameObject(baddie->object, 1);
                    baddie->object = NULL;
                    continue;
                } else {
                    object = baddie->object;
                    if (object == NULL) {
                        continue;
                    }
                    if (object->apiobj.ai->opponent == NULL && baddie->partner != NULL &&
                        baddie->partner->object != NULL) {
                        object->apiobj.ai->opponent = baddie->partner->object;
                    }
                }
            }
        } else if ((baddie->flags & JEDIB_CREATURE_RELEASED) == 0 &&
                   (OnOrInsidePlane(&baddie->position, &PlayPlane[1].point, &PlayPlane[1].normal, NULL, 1.5f, NULL) !=
                        0 ||
                    OnOrInsidePlane(&baddie->position, &PlayPlane[2].point, &PlayPlane[2].normal, NULL, 1.5f, NULL) !=
                        0)) {
            object = baddie->object;
        } else {
            // OnOrInsidePlane takes the slot position by non-const pointer, so
            // the copy below cannot be hoisted above these calls without
            // changing the emitted code.
            const i32 spawn_behind =
                (baddie->flags & JEDIB_CREATURE_SPAWNED_BEHIND) != 0 &&
                OnOrInsidePlane(&baddie->position, &PlayPlane[1].point, &PlayPlane[1].normal, NULL, 0.5f, NULL) == 0 &&
                OnOrInsidePlane(&baddie->position, &PlayPlane[2].point, &PlayPlane[2].normal, NULL, 0.5f, NULL) == 0;
            nuvec_s spawn_position = baddie->position;
            if (spawn_behind != 0) {
                NuVecRotateY(&spawn_position, &spawn_position, 0x8000);
            }
            if (baddie->id == -1) {
                if ((baddie->flags & JEDIB_CREATURE_RANDOM_TYPE) != 0) {
                    if (FreePlay == 0 && (jedi_b.unique_spawned & 1) == 0) {
                        baddie->id = id_LUMINARA;
                        jedi_b.unique_spawned |= 1;
                    } else if (FreePlay == 0 && (jedi_b.unique_spawned & 2) == 0) {
                        baddie->id = id_SHAAKTI;
                        jedi_b.unique_spawned |= 2;
                    } else if (NuRandFloat() < 0.5f) {
                        baddie->id = id_BATTLEDROIDGEONOSIAN;
                    } else {
                        baddie->id = id_GEONOSIAN;
                    }
                } else {
                    baddie->id = id_BOB;
                }
                baddie->flags &=
                    static_cast<u8>(~(JEDIB_CREATURE_TWO_ROW_HP | JEDIB_CREATURE_IN_WAVE | JEDIB_CREATURE_RELEASED));
                baddie->field_0x58 = 0;
            }
            object = AddDynamicCreature(baddie->id, &spawn_position, baddie->angle, "gen_bdroids",
                                        &baddie->locator.path_info, NULL, 1, NULL, NULL, 0, 0);
            baddie->object = object;
            if (object == NULL) {
                continue;
            }
            object->ai.locator = &baddie->locator;
            object->field_0xeb4 = JediBKilledCallback;
            if (baddie->id == id_BOB) {
                if (baddie->field_0x58 != 0) {
                    object->field_0x1054 = baddie->field_0x58;
                    if ((baddie->flags & JEDIB_CREATURE_TWO_ROW_HP) != 0) {
                        object->field_0xefd |= 2;
                    } else {
                        object->field_0xefd &= static_cast<u8>(~2);
                    }
                    object = baddie->object;
                } else {
                    baddie->field_0x58 = object->field_0x1054;
                    if ((object->field_0xefd & 2) != 0) {
                        baddie->flags |= JEDIB_CREATURE_TWO_ROW_HP;
                    } else {
                        baddie->flags &= static_cast<u8>(~JEDIB_CREATURE_TWO_ROW_HP);
                    }
                }
            }
        }

        if (object == NULL) {
            continue;
        }
        jedib_n_active++;
        if ((baddie->flags & JEDIB_CREATURE_RELEASED) == 0) {
            object->ai_opponent_exclusion_mask |= jedi_b.exclusion_mask;
        }
    }
}

void JediB_DrawPanel(WORLDINFO_s *) {
    if (Mission_Active(MissionSys) != NULL) {
        return;
    }
    i16 ids[8];
    char spawned[8] = {0};
    if (netclient == 0) {
        if (nethost != 0) {
            jedib_netpacket->state = jedi_b.state;
            jedib_netpacket->phase = jedi_b.phase;
        }
        if (jedi_b.state != 1) {
            return;
        }
        switch (jedi_b.phase) {
            case 1:
            case 2:
            case 3: {
                i32 count;
                if (jedi_b.goody_count != 0) {
                    count = jedi_b.goody_count;
                    if (count > 8) {
                        count = 8;
                    }
                    for (i32 index = 0; index < count; index++) {
                        ids[index] = static_cast<i16>(jedi_b.goodies[index].id);
                        if (jedi_b.goodies[index].object == NULL) {
                            spawned[index] = 1;
                        }
                    }
                } else {
                    if (jedi_b.phase == 1) {
                        ids[0] = id_PADMECLAWED;
                    } else if (jedi_b.phase == 2) {
                        ids[0] = id_ANAKINPADAWAN;
                    } else {
                        ids[0] = id_OBIWANKENOBIJEDIMASTER;
                    }
                    count = 1;
                }
                DrawMeleeTargets(ids, spawned, NULL, count);
                if (nethost != 0) {
                    memmove(jedib_netpacket->ids, ids, count * 2);
                    memmove(jedib_netpacket->spawned, spawned, count);
                    jedib_netpacket->count = static_cast<i16>(count);
                }
                break;
            }
            case 4:
            case 5:
            case 6:
                DrawMeleeTargets(jedi_b.wave_ids, jedi_b.wave_spawned, NULL, g_lowEndLevelBehaviour != 0 ? 4 : 6);
                if (nethost != 0) {
                    memmove(jedib_netpacket->ids, jedi_b.wave_ids, sizeof(jedi_b.wave_ids));
                    memmove(jedib_netpacket->spawned, jedi_b.wave_spawned, sizeof(jedi_b.wave_spawned));
                }
                break;
            case 7:
                if (jedi_b.boss == NULL) {
                    jedi_b.boss = (GameObject_s *)FindGameObject((i32)(i16)id_JANGOFETT, 1, 1, 0, 0);
                    if (jedi_b.boss == NULL) {
                        return;
                    }
                }
                DrawBossHitPoints(jedi_b.boss);
                break;
        }
    } else {
        if (jedib_netpacket->state != 1) {
            return;
        }
        switch (jedib_netpacket->phase) {
            case 1:
            case 2:
            case 3:
                DrawMeleeTargets(jedib_netpacket->ids, jedib_netpacket->spawned, NULL, jedib_netpacket->count);
                break;
            case 4:
            case 5:
            case 6:
                DrawMeleeTargets(jedib_netpacket->ids, jedib_netpacket->spawned, NULL,
                                 g_lowEndLevelBehaviour != 0 ? 4 : 6);
                break;
            case 7:
                if (jedi_b.boss != NULL) {
                    DrawBossHitPoints(jedi_b.boss);
                }
                break;
        }
    }
}

// ===========================================================================
// Gunship (Gunship_A / Gunship_B)
// ===========================================================================

void GunshipA_Init(WORLDINFO_s *world) {
    gunship_bolts[1] = (i16)BoltType_FindIDByName("gunbolt2", world);
    gunship_bolts[0] = (i16)BoltType_FindIDByName("gunbolt1", world);
    gunship_flags[0] = 0;
    gunship_flags[1] = 0;
    gunship_flags[2] = 0;
    gunship_flags[3] = 0;
    gunship_flags[4] = 0;
    gunship_flags[5] = 1;
    gunship_flags[6] = 1;
    gunship_flags[7] = 1;
    gunship_flags[8] = 1;
    gunship_flags[9] = 1;
    InitMiniSnowTroopers(world, 0xa, 0x20, 0);
    gunship_weapons[0] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gunw1");
    gunship_weapons[1] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gunw2");
    gunship_weapons[2] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gunw3");
    gunship_weapons[3] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gunw4");
}

void GunshipA_Update(WORLDINFO_s *world) {
    UpdateMiniSnowTroopers(world);
}

void GunshipA_Draw(WORLDINFO_s *world) {
    if (TimingBarSet == 5) {
        TBOPENFN("gun_timing", 5);
        DrawMiniSnowTroopers(world);
    } else {
        DrawMiniSnowTroopers(world);
        if (TimingBarSet == 5)
            TBCLOSEFN("gun_timing", 5);
    }
}

void GunshipB_Reset(WORLDINFO_s *world) {
    LevGizmo[0] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun1");
    LevGizmo[1] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun2");
    LevGizmo[2] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun3");
    LevGizmo[3] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun4");
    LevGizmo[4] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun5");
    LevGizmo[5] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun6");
    LevGizmo[6] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun7");
    LevGizmo[7] = GizmoFindByName(world->gizmo_sys, blowup_gizmotype_id, "gun8");
}

i32 GunshipInLevel(LEVELDATA_s *level) {
    if (BONUS_GUNSHIPA_LDATA == NULL)
        return 0;
    return BONUS_GUNSHIPA_LDATA == level;
}

f32 gunshipb_seekmomseek = 5.0f;
f32 gunshipb_seekmom = 5.0f;
f32 gunshipb_seekrange = 4.0f;
f32 SeekValF(f32 current, f32 target, f32 rate);

// Original: 1,436 bytes.
void GunShip_DragBombSeekBlowUp(GameObject_s *object) {
    if (object->character_context != 0x34)
        return;
    f32 nearest_distance = gunshipb_seekrange * gunshipb_seekrange;
    GIZMOBLOWUP_s *nearest = NULL;
    NUVEC offset, nearest_offset, direction;
    for (i32 i = 0; i < 8; ++i) {
        if (LevGizmo[i] == NULL)
            continue;
        GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(LevGizmo[i]->object);
        if (blowup == NULL || (blowup->status_flags & 0x800001) != 0x800000)
            continue;
        f32 distance = NuVecDistSqr(&blowup->mid_position, &object->apiobj.collision_position, &offset);
        if (nearest_distance > distance) {
            nearest_distance = distance;
            nearest_offset = offset;
            nearest = blowup;
        }
    }
    if (nearest == NULL)
        return;
    NuVecNorm(&direction, &nearest_offset);
    f32 momentum = (1.0f - NuFsqrt(nearest_distance) / gunshipb_seekrange) * gunshipb_seekmom;
    direction.x *= momentum;
    direction.z *= momentum;
    object->apiobj.velocity.x = SeekValF(object->apiobj.velocity.x, direction.x, gunshipb_seekmomseek);
    object->apiobj.velocity.z = SeekValF(object->apiobj.velocity.z, direction.z, gunshipb_seekmomseek);
}

// ===========================================================================
// Bonus gunship (Bonus_Gunship_A / Bonus_Gunship_B)
// ===========================================================================

void BonusGunshipA_Reset(WORLDINFO_s *) {
    gunship_player_dead = 0;
    if (LevFlag.progress == GUNSHIP_ACTIVE)
        LevFlag.progress = GUNSHIP_WON;
    LevFlag.exit = 0;
    bonus_gunship_store_progress_flag = 0;
}

void BonusGunshipA_Update(WORLDINFO_s *world) {
    if (LevFlag.progress == GUNSHIP_INACTIVE) {
        bool found = false;
        if (Player[0] != NULL && Player[0]->field_0x661 == 0 && Player[0]->field_0x68c > 0.001f) {
            found = true;
        } else if (Player[1] != NULL && Player[1]->field_0x661 == 0 && Player[1]->field_0x68c > 0.001f) {
            found = true;
        }
        if (found) {
            if (netclient != 0) {
                LevFlag.progress = GUNSHIP_ACTIVE;
            } else {
                Doors_SetLastDoor((DOOR_s *)Door_FindByName(world, "bonus_door"));
                bonus_gunship_store_progress_flag = 1;
                StoreLevelProgress(world);
                bonus_gunship_store_progress_flag = 0;
                LevFlag.progress = GUNSHIP_ACTIVE;
            }
        }
    }
    if (gunship_player_dead == 0) {
        if ((Player[0] != NULL && Player[0]->apiobj.field_0x287 != 0) ||
            (Player[1] != NULL && Player[1]->apiobj.field_0x287 != 0)) {
            gunship_player_dead = 1;
            ResetLevel(world, "bonus", 1);
        }
    }
}

void BonusGunshipB_Init(WORLDINFO_s *world) {
    bonusgunshipb_netpacket = (struct BONUSGUNSHIP_NETPACKET_s *)SetLevelHack(0xc);
    LevGizObst[0] = GizObstacle_FindByName(world->giz_obstacle_sys, "obs");
}

void BonusGunshipB_Reset(WORLDINFO_s *) {
    LevFlag.progress = GUNSHIP_INACTIVE;
    LevFlag.exit = 0;
    MiscTime = 0;
    gunship_player_dead = 0;
    bonus_gunship_store_progress_flag = 0;
}

void BonusGunshipB_Update(WORLDINFO_s *world) {
    if (netclient != 0) {
        LevFlag.progress = bonusgunshipb_netpacket->state;
        LevFlag.exit = bonusgunshipb_netpacket->sub;
        MiscTime = bonusgunshipb_netpacket->time;
    } else {
        if (gunship_player_dead == 0 && ((Player[0] != NULL && Player[0]->apiobj.field_0x287 != 0) ||
                                         (Player[1] != NULL && Player[1]->apiobj.field_0x287 != 0))) {
            gunship_player_dead = 1;
            ResetLevel(world, "bonus_gunship", 1);
        }
        bonusgunshipb_netpacket->state = LevFlag.progress;
        bonusgunshipb_netpacket->sub = LevFlag.exit;
        bonusgunshipb_netpacket->time = MiscTime;
    }
    if (LevFlag.progress == 0) {
        if (LevDeaths > 0) {
            float x = (float)LevDeaths * LevDeaths + 1.0f;
            if (GameTimer.time_elapsed >= x)
                LevFlag.progress = GUNSHIP_ACTIVE;
        }
    } else if (LevFlag.progress == GUNSHIP_ACTIVE) {
        if (MiscTime > 5.0f)
            MiscTime = 5.0f;
    }
}

void BonusGunshipB_Panel(WORLDINFO_s *) {
    if (LevFlag.progress == GUNSHIP_ACTIVE) {
        if (MiscTime > 60.0f)
            DrawTimer((i32)MiscTime + 1, 0, 0);
    }
}

// ===========================================================================
// Dooku (Dooku_C)
// ===========================================================================

void DookuC_Init(WORLDINFO_s *world) {
    LevGizForce[0] = GizForce_FindByName(world->giz_force_sys, "dooku");
    LevGizForce[1] = GizForce_FindByName(world->giz_force_sys, "dooku1");
    LevGizForce[2] = GizForce_FindByName(world->giz_force_sys, "dooku2");
    void *path1 = AIPathFindNode(world->ai_sys, NULL, "path1");
    LevAIPathNode[0] = path1;
    void *path2 = AIPathFindNode(world->ai_sys, NULL, "path2");
    LevAIPathNode[1] = path2;
    void *path3 = AIPathFindNode(world->ai_sys, NULL, "path3");
    LevAIPathNode[2] = path3;
    void *path4 = AIPathFindNode(world->ai_sys, NULL, "path4");
    LevAIPathNode[3] = path4;
    char buf[0x40];
    LevPathCnx[0] = AIPAthFindPathCnx(world->ai_sys, 0, path1, path2, buf);
    LevPathCnx[1] = AIPAthFindPathCnx(world->ai_sys, 0, path2, path3, buf);
    LevPathCnx[2] = AIPAthFindPathCnx(world->ai_sys, 0, path3, path4, buf);
    LevPathCnx[3] = AIPAthFindPathCnx(world->ai_sys, 0, path4, (void *)"conn", buf);
    dookuC_nodesNeedUpdating = 1;
}

void DookuC_Reset(WORLDINFO_s *world) {
    dooku_c = 0;
    dooku_state = {};
    if (netclient == 0) {
        dooku_c = SetGizAIMessage(gizaimessagesys, "dooku_total", 0.0f, NULL);
        dooku_state.hit_message = (i32)(usize)CheckGizAIMessage(gizaimessagesys, "dooku_hits", NULL);
    }
    NuSpecialFind(world->current_gscn, &dooku_state.node, "dooku_node", 1);
}

void DookuC_Update(WORLDINFO_s *world) {
    if (netclient == 0) {
        if (FreePlay != 0) {
            KillBossCompleteLevel((i32)(i16)id_COUNTDOOKU, 0, 0.0f);
        } else if (DOOKUOUTRO_LDATA != NULL) {
            KillBossNewLevel((i32)(i16)id_COUNTDOOKU, 0, 0.0f, DOOKUOUTRO_LDATA->idx);
        }
    }
    DrawForceBackEffect(&dooku_state.node);
}

void DookuC_DrawPanel(WORLDINFO_s *) {
    if (netclient != 0)
        return;
    GameObject_s *obj = (GameObject_s *)FindGameObject((i32)(i16)id_COUNTDOOKU, 1, 1, 1, 0);
    if (obj != NULL && dooku_c != 0 && obj->apiobj.anim_packet.time_secondary == 1.0f)
        DrawBossHitPoints(obj);
}
