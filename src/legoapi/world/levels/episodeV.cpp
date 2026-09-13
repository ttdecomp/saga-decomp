#include "decomp.h"
#include "globals.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/gizmos/object/newblowup.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "legoapi/gizmos/traps/gizbombgen.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/core/render.h"
#include "legoapi/render/fx/parts.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/world/world.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"

#include <string.h>

extern TERRAIN_SURFACE_s TerSurface[32];
extern i32 dagobah_training;
AILOCATOR_s *locator;
GameObject_s *gameobj;
extern u8 troopercannons_beenReset;
extern "C" i32 FindPlatInst(i32 instance_ix);
void Asteroid_PartKill(PART_s *, i32);
void GizmoBlowupUpdateMatrix(GIZMOBLOWUP_s *);
void PartCollide_3D(PART_s *);

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" {
    GIZBOMBGEN *HothBattleC_BombGenerator = NULL;
    HOTHBATTLE_MELEE_s melee;
}

void DagobahA_Init(WORLDINFO_s *) {
}

void DagobahB_Init(WORLDINFO_s *) {
    dagobah_training = 0;
}

void DagobahC_Init(WORLDINFO_s *) {
}

void DagobahE_Init(WORLDINFO_s *) {
}

void DagobahB_Reset(WORLDINFO_s *world) {
    LevSafePlatID[1] = -1;
    LevSafePlatID[0] = -1;

    if (NuSpecialFind(world->current_gscn, &LevHSpecial[0], "pad_2_base_2", 1) != 0) {
        if (world->terrain != NULL) {
            LevSafePlatID[0] = FindPlatInst(NuSpecialGetInstanceix(&LevHSpecial[0]));
        }
    }

    if (NuSpecialFind(world->current_gscn, &LevHSpecial[1], "pad_4_base_2", 1) != 0) {
        if (world->terrain != NULL) {
            LevSafePlatID[1] = FindPlatInst(NuSpecialGetInstanceix(&LevHSpecial[1]));
        }
    }
}

void DagobahC_Panel(WORLDINFO_s *) {
}

void KillParts_ATAT(ADDPART_s *, i32, i32, GameObject_s *) {
}

f32 rocket_speed = 1.2f;

void BobaRocket_Kill(PART_s *, i32) {
}

void BobaRocket_Move(PART_s *, float) {
}

void DagobahA_Update(WORLDINFO_s *) {
}

void HothBattleA_Draw(WORLDINFO_s *world) {
    if (TimingBarSet == 5) {
        TBOPENFN("mini", 5);
    }
    DrawMiniSnowTroopers(world);
    if (TimingBarSet == 5) {
        TBCLOSEFN("mini", 5);
    }
}

void HothBattleA_Init(WORLDINFO_s *) {
}

void HothBattleB_Init(WORLDINFO_s *) {
}

void HothBattleC_Draw(WORLDINFO_s *world) {
    if (TimingBarSet == 5) {
        TBOPENFN("mini", 5);
    }
    DrawMiniSnowTroopers(world);
    if (TimingBarSet == 5) {
        TBCLOSEFN("mini", 5);
    }
}

void HothBattleC_Init(WORLDINFO_s *) {
}

void HothBattleE_Draw(WORLDINFO_s *world) {
    if (NuIOS_IsLowEndDevice()) {
        return;
    }
    if (TimingBarSet == 5) {
        TBOPENFN("mini", 5);
    }
    DrawMiniSnowTroopers(world);
    if (TimingBarSet == 5) {
        TBCLOSEFN("mini", 5);
    }
}

void HothBattleE_Init(WORLDINFO_s *) {
}

void HothEscapeA_Init(WORLDINFO_s *) {
}

void HothEscapeB_Init(WORLDINFO_s *) {
}

void HothEscapeC_Init(WORLDINFO_s *) {
}

void HothEscapeD_Init(WORLDINFO_s *) {
}

void HothBattleA_Reset(WORLDINFO_s *world) {
    GIZMO *gizmo = LevGizmo[0];
    if (gizmo == NULL || gizmo->object == NULL) {
        return;
    }

    GIZMOPICKUP_s *pickup = static_cast<GIZMOPICKUP_s *>(gizmo->object);
    if (minikitCounter_A == 10 && (pickup->state_flags & 8) == 0) {
        GizmoActivate(world->gizmo_sys, gizmo, 1, 1);
        return;
    }
    GizmoSetVisibility(world->gizmo_sys, gizmo, 0, 1);
}

void HothBattleC_Reset(WORLDINFO_s *world) {
    GIZMO *bomb_generator = GizmoFindByName(world->gizmo_sys, bombgen_gizmotype_id, "bomb_generator1");
    if (bomb_generator != NULL && bomb_generator->object != NULL) {
        HothBattleC_BombGenerator = static_cast<GIZBOMBGEN *>(bomb_generator->object);
    }

    GIZMO *gizmo = LevGizmo[1];
    if (gizmo == NULL || gizmo->object == NULL) {
        return;
    }

    GIZMOPICKUP_s *pickup = static_cast<GIZMOPICKUP_s *>(gizmo->object);
    if (minikitCounter_C == 10 && (pickup->state_flags & 8) == 0) {
        GizmoActivate(world->gizmo_sys, gizmo, 1, 1);
        return;
    }
    GizmoSetVisibility(world->gizmo_sys, gizmo, 0, 1);
}

void HothBattleE_Panel(WORLDINFO_s *) {
}

void HothEscapeA_Reset(WORLDINFO_s *) {
}

void HothEscapeB_Reset(WORLDINFO_s *world) {
    locator = AIPathFindLocator(world->ai_sys, "snow_mob");
    gameobj = GetNamedGameObject(world->ai_sys, "snowmob_1");
    TerSurface[9].movement_scale = TerSurface[17].movement_scale;
    TerSurface[9].flags = TerSurface[17].flags & ~2u;
}

void HothEscapeC_Reset(WORLDINFO_s *) {
}

void HothEscapeD_Reset(WORLDINFO_s *) {
}

void BobaRocket_Deflect(PART_s *) {
}

void HothBattleA_Update(WORLDINFO_s *) {
}

void HothBattleC_Update(WORLDINFO_s *) {
}

void HothBattleE_Update(WORLDINFO_s *) {
}

void HothEscapeA_Update(WORLDINFO_s *) {
}

void HothEscapeB_Update(WORLDINFO_s *) {
}

void HothEscapeC_Update(WORLDINFO_s *) {
}

void HothEscapeD_Update(WORLDINFO_s *) {
}

void InitTrooperCannons(WORLDINFO_s *) {
}

void CloudCityTrapA_Init(WORLDINFO_s *) {
}

void CloudCityTrapB_Init(WORLDINFO_s *) {
}

void CloudCityTrapA_Reset(WORLDINFO_s *) {
    if (netclient == 0)
        troopercannons_beenReset = 0;
}

void CloudCityTrapC_Panel(WORLDINFO_s *) {
}

void CloudCityTrapC_Reset(WORLDINFO_s *) {
}

void InitMiniSnowTroopers(WORLDINFO_s *, i32, i32, i32) {
}

void CloudCityEscapeA_Init(WORLDINFO_s *) {
}

void CloudCityEscapeC_Init(WORLDINFO_s *) {
}

void CloudCityTrapA_Update(WORLDINFO_s *) {
}

void CloudCityTrapB_Update(WORLDINFO_s *) {
}

void CloudCityTrapC_Update(WORLDINFO_s *) {
}

void HothBattle_Melee_init(HOTHBATTLE_MELEE_s *melee) {
    if (melee != NULL) {
        melee->waves[0].field_0x0 = 0;
        melee->field_0x0 = 0;
        melee->field_0x1 = 0;
        melee->field_0x2 = 1;
        melee->field_0x4 = -1;
    }
}

void CloudCityEscapeA_Panel(WORLDINFO_s *) {
}

void CloudCityEscapeA_Reset(WORLDINFO_s *world) {
    LevAIMessage[0] = CheckGizAIMessage(gizaimessagesys, "BobaFightStarted", NULL);
    LevAIMessage[1] = CheckGizAIMessage(gizaimessagesys, "Built_C3PO", NULL);
    LevGizmo[0] = GizmoFindByName(world->gizmo_sys, gizbuildit_gizmotype_id, "buildit2");
}

void HothBattleE_UpdateWave() {
}

void CloudCityEscapeA_Update(WORLDINFO_s *) {
}

void CloudCityEscapeC_Update(WORLDINFO_s *) {
}

void HothBattle_StartNewWave() {
}

void HothEscapeC_AlwaysUpdate(WORLDINFO_s *) {
}

i32 isHothBattleWaveCreature(GameObject_s *object) {
    for (i32 wave = 0; wave < 4; ++wave) {
        for (i32 creature = 0; creature < 4; ++creature) {
            if (melee.waves[wave].creatures[creature] == object)
                return 1;
        }
    }
    return 0;
}

void HothBattle_ManageBackgroundCreatures() {
}

// ===========================================================================
// Asteroid chase (AsteroidChase_A / B / C / D)
// ===========================================================================

struct ASTEROID_s {
    nuhspecial_s special;
    GIZMOBLOWUP_s *blowup;
    i16 rotation_speed_x;
    i16 rotation_speed_y;
    i16 rotation_speed_z;
    u8 activated;
    u8 reserved_17;
};
DECOMP_ASSERT(sizeof(ASTEROID_s) == 0x18, "ASTEROID_s size");

i32 nasteroids;
ASTEROID_s asteroids[128];

static void Asteroid_AddParts(GIZMOBLOWUP_s *blowup) {
    i32 special_indices[4] = {0, -1, -1, -1};
    const i32 part_count = qrand() / 0x4000 + 1;
    for (i32 index = 1; index < part_count; ++index) {
        special_indices[index] = qrand() / (0xffff / 3 + 1) + 1;
    }

    for (i32 index = 0; index < part_count; ++index) {
        nuhspecial_s *special = &LevHSpecial[special_indices[index]];
        if (NuSpecialExistsFn(special) == 0) {
            continue;
        }

        NUANGVEC rotation = {qrand(), qrand(), qrand()};
        NUMTX_ALIGNED16 matrix;
        NuMtxSetRotateXYZVU0(&matrix, &rotation);
        NuMtxTranslate(&matrix, &blowup->position);

        NUVEC velocity = {0.0f, 0.0f, static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 3.0f + 2.0f};
        NuVecRotateY(&velocity, &velocity, qrand());

        ADDPART_ALIGNED16 params = Default_ADDPART;
        params.matrix = &matrix;
        params.velocity = &velocity;
        NUVEC centre;
        NuSpecialGetRadius(special, &centre, &params.field_14);
        params.field_18 = params.field_14;
        params.gravity = 0.0f;
        params.special = special;
        params.flags = index == 0 ? 0x800019b : 0x8000193;
        params.field_40 = PartCollide_3D;
        params.field_44 = Asteroid_PartKill;
        params.time_step = FRAMETIME;
        params.field_a4 = static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 3.0f + 7.0f;

        PART_s *part = AddPart(&params);
        if (part != NULL) {
            part->force_player_mask = index == 0 ? 1 : 2;
        }
    }
}

static __used__ void Asteroids_Update() {
    ASTEROID_s *asteroid = asteroids;
    for (i32 index = 0; index < nasteroids; ++index, ++asteroid) {
        GIZMOBLOWUP_s *blowup = asteroid->blowup;
        if (blowup != NULL) {
            if ((static_cast<u16>(blowup->status_flags) & 0x4001) == 0x4000) {
                asteroid->activated = 0;
                blowup->field_0xf0 += static_cast<i16>(static_cast<f32>(asteroid->rotation_speed_x) * FRAMETIME);
                blowup->field_0xf2 += static_cast<i16>(static_cast<f32>(asteroid->rotation_speed_y) * FRAMETIME);
                blowup->state_flags |= 1;
                blowup->field_0xf4 += static_cast<i16>(static_cast<f32>(asteroid->rotation_speed_z) * FRAMETIME);
                GizmoBlowupUpdateMatrix(blowup);
                continue;
            }
        } else if (NuSpecialGetVisibilityFn(&asteroid->special) != 0) {
            asteroid->activated = 0;
            NUMTX *matrix = NuSpecialGetDrawMtx(&asteroid->special);
            if (matrix != NULL) {
                NuMtxPreRotateX(matrix, static_cast<i32>(static_cast<f32>(asteroid->rotation_speed_x) * FRAMETIME));
                NuMtxPreRotateY(matrix, static_cast<i32>(static_cast<f32>(asteroid->rotation_speed_y) * FRAMETIME));
                NuSpecialUpdate(&asteroid->special);
                continue;
            }
        }

        if (asteroid->activated == 0) {
            asteroid->activated = 1;
            if (blowup != NULL) {
                blowup->field_0xa8 = 0;
                Asteroid_AddParts(blowup);
            }
        }
    }
}

static void Asteroids_Reset(WORLDINFO_s *world) {
    static const i32 maxrotspd[3] = {0x1555, 0x38e, 0x16c};
    nuhspecial_s specials[128];

    memset(asteroids, 0, sizeof(asteroids));
    nasteroids = 0;

    i32 special_count = NuSpecialFindMulti(world->current_gscn, specials, "asteroid", 128, 0);
    if (special_count == 0)
        return;

    if (special_count > 0) {
        for (i32 special_index = 0; special_index < special_count; ++special_index) {
            for (i32 type_index = 0; type_index < world->gizmo_blowup_type_count; ++type_index) {
                NuSpecialCompare(&world->gizmo_blowup_types[type_index].special, &specials[special_index]);
            }

            if (NuSpecialGetVisibilityFn(&specials[special_index]) != 0) {
                ASTEROID_s *asteroid = &asteroids[nasteroids];
                asteroid->special = specials[special_index];

                char *name = NuSpecialGetName(&asteroid->special);
                i32 asteroid_type;
                if (name == NULL || NuStrIStr(name, "asteroid_a") != NULL || NuStrIStr(name, "asteroid_pop") != NULL) {
                    asteroid_type = 0;
                } else if (NuStrIStr(name, "asteroid_b") != NULL) {
                    asteroid_type = 1;
                } else if (NuStrIStr(name, "asteroid_c") != NULL) {
                    asteroid_type = 2;
                } else {
                    asteroid_type = 0;
                }

                i32 max_speed = maxrotspd[asteroid_type];
                asteroid->rotation_speed_x = static_cast<i16>(qrand() / ~(0xffff / (max_speed * 2)) + max_speed);
                asteroid->rotation_speed_y = static_cast<i16>(qrand() / ~(0xffff / (max_speed * 2)) + max_speed);
                asteroid->rotation_speed_z = static_cast<i16>(qrand() / ~(0xffff / (max_speed * 2)) + max_speed);
                ++nasteroids;
            }
        }
    }

    for (i32 blowup_index = 0; blowup_index < world->gizmo_blowup_count; ++blowup_index) {
        ASTEROID_s *asteroid = &asteroids[nasteroids];
        GIZMOBLOWUP_s *blowup = &world->gizmo_blowups[blowup_index];
        char *name = blowup->name;
        i32 asteroid_type;
        if (name == NULL || NuStrIStr(name, "asteroid_a") != NULL || NuStrIStr(name, "asteroid_pop") != NULL) {
            asteroid_type = 0;
        } else if (NuStrIStr(name, "asteroid_mid") != NULL) {
            asteroid_type = 1;
        } else {
            continue;
        }

        asteroid->blowup = blowup;
        i32 max_speed = maxrotspd[asteroid_type];
        asteroid->rotation_speed_x = static_cast<i16>(qrand() / ~(0xffff / (max_speed * 2)) + max_speed);
        asteroid->rotation_speed_y = static_cast<i16>(qrand() / ~(0xffff / (max_speed * 2)) + max_speed);
        asteroid->rotation_speed_z = static_cast<i16>(qrand() / ~(0xffff / (max_speed * 2)) + max_speed);
        ++nasteroids;
    }
}

void AsteroidChaseA_Init(WORLDINFO_s *) {
}

void AsteroidChaseB_Init(WORLDINFO_s *) {
}

void AsteroidChaseB_Draw(WORLDINFO_s *) {
}

void AsteroidChaseC_Init(WORLDINFO_s *) {
}

void AsteroidChaseD_Init(WORLDINFO_s *) {
}

void AsteroidChaseA_Reset(WORLDINFO_s *world) {
    Asteroids_Reset(world);
}

void AsteroidChaseB_Reset(WORLDINFO_s *world) {
    Asteroids_Reset(world);
}

void AsteroidChaseC_Reset(WORLDINFO_s *world) {
    Asteroids_Reset(world);
}

void AsteroidChaseD_Panel(WORLDINFO_s *) {
}

void AsteroidChaseA_Update(WORLDINFO_s *) {
    Asteroids_Update();
}

void AsteroidChaseB_Update(WORLDINFO_s *) {
}

void AsteroidChaseC_Update(WORLDINFO_s *) {
}

void AsteroidChaseD_Update(WORLDINFO_s *) {
}
