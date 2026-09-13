#include <string.h>
#include "decomp.h"
#include "legoapi/world/level.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/props/doors/door.h"
#include "legoapi/gizmo/base/GizObstacleObjectInterface.h"
#include "legoapi/gizmo/base/GizForceObjectInterface.h"
#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "gameapi/ai/aisys/aisys.h"
#include "nu2api/nu3d/nuspecial.h"

extern i32 LevFlag[4];

extern "C" {
    void *AIPAthFindPathCnx(AISYS_s *, i32, void *, void *, void *);
}
#include "legoapi/render/core/render.h"
#include "nu2api/nu3d/nutex.h"

static GIZAIMESSAGE_s *KashyyykA_msg_TotalWookies;
static GIZAIMESSAGE_s *KashyyykA_msg_WookiesToRescue;
static GameObject_s *Grievous_obj; // current Grievous boss object
struct VADERAOBJECT_s {
    u8 reserved_00[0x28];
    i32 field_28;
};

struct VADERA_s {
    GIZAIMESSAGE_s *in_control_room_message;
    GIZAIMESSAGE_s *ceiling_collapse_message;
    VADERAOBJECT_s *object;
    f32 timer;
    u16 count;
    i16 subtitle;
    GIZFORCE_s *forces[4];
    u8 reserved_24[0x28 - 0x24];
    i32 collapse_started;
    u8 reset_flag;
    u8 reserved_2d[0x03];
};

static VADERA_s vader_a;
static GIZAIMESSAGE_s *vader_b_complete_msg; // Vader B "complete" message handle
static u8 vader_b_playersDead;               // Vader B player-death flag

static void VaderA_StartCollapseStage(WORLDINFO_s *world);

struct CRUISERCNETPACKET_s {
    i16 free_palpatine;
    i16 dooku_fight;
};

struct CRUISERC_s {
    GameObject_s *count_dooku;
    GIZAIMESSAGE_s *dooku_fight;
    GIZAIMESSAGE_s *free_palpatine;
    GameObject_s *palpatine;
    AILOCATOR_s *palpatine_locator;
};

extern "C" {
    CRUISERCNETPACKET_s *crusiserc_netpacket = NULL;
}
static CRUISERC_s cruiser_c;

// Episode 3 level handlers, in the game's Episode_III progression:
// dogfight / cruiser / grievous / kashyyyk / temple / vader / a-new-hope.

// ===========================================================================
// Dogfight (Dogfight_A)
// ===========================================================================

void SpaceResetAudioPoint();
void ProcessCurrentSpeed(WORLDINFO_s *, speedup_s *);
extern AREADATA *DOGFIGHT_ADATA;

speedup_s DogFightSpeedList[] = {
    {58.0f, 0.5f},  {72.0f, 1.0f},  {174.0f, 0.5f}, {183.0f, 1.0f}, {207.0f, 0.5f},
    {220.0f, 1.0f}, {313.0f, 0.5f}, {335.0f, 1.0f}, {0.0f, 0.0f},
};

void ChrisDogFightAInit(WORLDINFO_s *) {
}

void ChrisDogFightAReset(WORLDINFO_s *) {
}

void ChrisDogFightAUpdate(WORLDINFO_s *world) {
    SpaceResetAudioPoint();
    ProcessCurrentSpeed(world, DogFightSpeedList);

    if (AreaGlobals.values.field_0x00 == 0 && *((u8 *)LevFlag) == 0 && DOGFIGHT_ADATA != NULL &&
        Game.area_save[DOGFIGHT_ADATA->index].area_complete == 0 && GamePlayTimer.time_elapsed >= 3.0f) {
        *((u8 *)LevFlag) = 1;
    }
}

void ChrisDogFightADraw(WORLDINFO_s *) {
}

void ChrisDogFightAPanel(WORLDINFO_s *) {
}

// ===========================================================================
// Cruiser (Cruiser_A / Cruiser_C / Cruiser_D)
// ===========================================================================

void CruiserAInit(WORLDINFO_s *world) {
    *((u8 *)LevFlag) = 0;
    NuSpecialFind(world->current_gscn, &LevHSpecial[0], "starfighter1", 1);
    NuSpecialFind(world->current_gscn, &LevHSpecial[1], "starfighter2", 1);
    if (FreePlay)
        *((u8 *)LevFlag) = 2;
}

void CruiserAUpdate(WORLDINFO_s *) {
    if (*((u8 *)LevFlag) == 1) {
        void *sp;

        *((u8 *)LevFlag) = 2;
        sp = (void *)LevHSpecial;
        if (NuSpecialExistsFn(sp) != 0)
            NuSpecialSetVisibility(sp, 1);
        sp = (char *)sp + 0xc;
        if (NuSpecialExistsFn(sp) != 0)
            NuSpecialSetVisibility(sp, 1);
    }
}

void CruiserCReset(WORLDINFO_s *) {
    crusiserc_netpacket = static_cast<CRUISERCNETPACKET_s *>(SetLevelHack(4));
    cruiser_c.count_dooku = NULL;
    cruiser_c.dooku_fight = NULL;
    cruiser_c.free_palpatine = NULL;
    cruiser_c.palpatine = NULL;
    cruiser_c.palpatine_locator = NULL;
}

void CruiserCUpdate(WORLDINFO_s *) {
}

void CruiserCPanel(WORLDINFO_s *) {
}

void CruiserDInit(WORLDINFO_s *) {
}

void CruiserDReset(WORLDINFO_s *) {
}

void CruiserDUpdate(WORLDINFO_s *) {
}

// ===========================================================================
// Grievous (Grievous_A)
// ===========================================================================

void GrievousA_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "grievous_1")) != NULL) {
        nuvec_s pos = {5.42f, 2.76f, 1.79f};
        NuSpecialSetDrawPos(&b->type->animated_special, &pos);
        UpdateMidPos(b);
    }
    if ((b = GizmoBlowUp_FindByName(world, "grievous_2")) != NULL)
        b->field_0x124 = 1;
    if ((b = GizmoBlowUp_FindByName(world, "grievous_3")) != NULL)
        b->field_0x124 = 1;
}

void GrievousA_Reset(WORLDINFO_s *) {
    if (netclient != 0)
        return;
    Grievous_obj = (GameObject_s *)FindGameObject((i32)(i16)id_GRIEVOUS, 1, 1, 0, 0);
    if (Grievous_obj != NULL)
        DrawBossHitPoints(Grievous_obj);
}

void GrievousA_Update(WORLDINFO_s *world) {
    if (netclient != 0)
        return;

    if (Grievous_obj == NULL)
        return;

    if (Grievous_obj->current_hp > 0)
        return;

    if (FreePlay == 0)
        KillBossPlayCutScene((i32)(i16)id_GRIEVOUS, 0, 0.0f, "ep3_GeneralGrievous_Outro");
    else
        KillBossCompleteLevel((i32)(i16)id_GRIEVOUS, 0, 0.0f);
}

// ===========================================================================
// Kashyyyk (Kashyyyk_A / Kashyyyk_B / Kashyyyk_C / Kashyyyk_D)
// ===========================================================================

void KashyyykA_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "bridge_1_switc1")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
        b->field_0xa0 &= ~2;
    }
    if ((b = GizmoBlowUp_FindByName(world, "bridge_1_switc2")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
        b->field_0xa0 &= ~2;
    }
    if ((b = GizmoBlowUp_FindByName(world, "bridge_2_switc1")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
        b->field_0xa0 &= ~2;
    }
    if ((b = GizmoBlowUp_FindByName(world, "bridge_2_switc2")) != NULL) {
        b->field_0x128 = 0.3f;
        b->field_0x124 = 1;
        b->field_0xa0 &= ~2;
    }
}

void KashyyykB_Init(WORLDINFO_s *) {
}

void KashyyykC_Init(WORLDINFO_s *world) {
    GIZFORCE_s *f = GizForces_FindForce(world, "kashyyyk_boss");
    if (f != NULL) {
        if (f->force_strength == 3.0f)
            f->force_strength = 20.0f;
        f->strength_0x6c = 1.0f;
    }
}

void KashyyykD_Init(WORLDINFO_s *) {
}

void KashyyykA_Panel(WORLDINFO_s *) {
}

void KashyyykA_Reset(WORLDINFO_s *) {
    KashyyykA_msg_TotalWookies = CheckGizAIMessage(gizaimessagesys, "TotalWookies", NULL);
    KashyyykA_msg_WookiesToRescue = CheckGizAIMessage(gizaimessagesys, "WookiesToRescue", NULL);
}

void KashyyykB_Reset(WORLDINFO_s *) {
}

void KashyyykD_Reset(WORLDINFO_s *) {
}

i32 AnakinGreenSabre(GameObject_s *obj) {
    i32 result = 0;
    if (FreePlay == 0 && obj->id == id_ANAKINPADAWAN && WORLD->area != NULL) {
        if (WORLD->area == JEDI_ADATA || WORLD->area == DOOKU_ADATA)
            result = 1;
    }
    return result;
}

void KashyyykA_Update(WORLDINFO_s *) {
}

void KashyyykB_Update(WORLDINFO_s *) {
}

void KashyyykC_Update(WORLDINFO_s *) {
}

void KashyyykD_Update(WORLDINFO_s *) {
}

// ===========================================================================
// Temple (Temple_A / Temple_C)
// ===========================================================================

void TempleA_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *b;
    if ((b = GizmoBlowUp_FindByName(world, "temple_statue")) != NULL)
        b->field_0xa0 |= 2;
    if ((b = GizmoBlowUp_FindByName(world, "temple_pillar")) != NULL)
        b->field_0xa0 |= 2;
}

void TempleC_Init(WORLDINFO_s *) {
}

void TempleC_AlwaysUpdate(WORLDINFO_s *) {
}

// ===========================================================================
// Vader (Vader_A / Vader_B / Vader_C)
// ===========================================================================

void VaderA_Init(WORLDINFO_s *) {
}

void VaderB_Init(WORLDINFO_s *) {
}

i32 Vader_ObiWanKilledAnakin;
void *vaderc_netpacket;
extern "C" i32 FindPlatInst(i32);

void VaderC_Init(WORLDINFO_s *world) {
    char *names[10] = {"rock1", "rock2", "rock3", "rock4", "rock5", "rock6", "rock7", "rock8", "rock10", "rock11"};
    memset(&vader_c, 0, sizeof(vader_c));
    vader_c.final_fight_message = CheckGizAIMessage(gizaimessagesys, "FinalFight", NULL);
    vader_c.big_jump_locator = AIPathFindLocator(world->ai_sys, "Bigjump_0");
    for (i32 i = 0; i < 10; ++i) {
        if (NuSpecialFind(world->current_gscn, &vader_c.rocks[i], names[i], 1))
            vader_c.platform_ids[i] = FindPlatInst(NuSpecialGetInstanceix(&vader_c.rocks[i]));
        else
            vader_c.platform_ids[i] = -1;
    }
    Vader_ObiWanKilledAnakin = 0;
    vaderc_netpacket = SetLevelHack(1);
    LevGizObst[0] = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle19");
}

void VaderA_Reset(WORLDINFO_s *world) {
    DrawTimer(0, 0, 1);

    if (netclient == 0 && vader_a.count != 0) {
        VaderA_StartCollapseStage(world);
    }

    if (vader_a.object != NULL) {
        vader_a.object->field_28 = 0;
    }

    vader_a.collapse_started = 1;
    vader_a.reset_flag = 0;
}

void VaderB_Reset(WORLDINFO_s *) {
    vader_b_complete_msg = SetGizAIMessage(gizaimessagesys, "VaderBComplete", 0.0f, NULL);
    vader_b_playersDead = 0;
}

void VaderC_Reset(WORLDINFO_s *) {
    vader_c.field_0x94 = 0;
    vader_c.field_0x95 = 0;
}

void VaderA_Update(WORLDINFO_s *) {
}

void VaderB_Update(WORLDINFO_s *) {
}

void VaderC_Update(WORLDINFO_s *) {
}

void VaderA_DrawPanel(WORLDINFO_s *) {
    if (vader_a.count <= 2) {
        if (vader_a.count != 0) {
            DrawTimer((i32)vader_a.timer + 1, vader_a.subtitle, 0);
            vader_a.subtitle = 0;
        }
    }
}

void VaderB_DrawPanel(WORLDINFO_s *) {
}

void VaderC_DrawPanel(WORLDINFO_s *) {
}

void VaderA_GoneThroughDoor(WORLDINFO_s *, DOOR_s *door) {
    if (netclient == 0 && door != NULL)
        door->active = 1;
}

static __used__ void VaderA_StartCollapseStage(WORLDINFO_s *world) {
    i32 path_index;
    nuhspecial_s specials[100];
    u32 *path_connection = (u32 *)AIPAthFindPathCnx(world->ai_sys, (i32)(usize)world->ai_sys->path_sys->active_path,
                                                    (void *)"Block1_a", (void *)"Block1_b", &path_index);

    if (path_connection != NULL) {
        path_connection[path_index] |= 0x80000000;
    }

    vader_a.forces[0] = GizForces_FindForce(world, "force4");
    vader_a.forces[1] = GizForces_FindForce(world, "InControlRoom");
    vader_a.forces[2] = GizForces_FindForce(world, "ceiling_collapse");
    vader_a.forces[3] = GizForces_FindForce(world, "wobble");
    vader_a.count = 1;
    vader_a.timer = 30.0f;
    vader_a.in_control_room_message =
        SetGizAIMessage(gizaimessagesys, "InControlRoom", 1.0f, vader_a.in_control_room_message);
    vader_a.ceiling_collapse_message =
        SetGizAIMessage(gizaimessagesys, "ceiling_collapse", 1.0f, vader_a.ceiling_collapse_message);

    i32 special_count = NuSpecialFindMulti(world->current_gscn, specials, "wobble", 100, 0);
    for (i32 i = 0; i < special_count; ++i) {
        NuSpecialSetVisibility(&specials[i], 0);
    }
}

// ===========================================================================
// A New Hope (ANewHope_A)
// ===========================================================================

void ANewHopeA_Init(WORLDINFO_s *world) {
    GIZOBSTACLE_s *g;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle6")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle7")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle8")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle9")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle10")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle11")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle12")) != NULL)
        g->field_a1_0xa1 |= 1;
    if ((g = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle13")) != NULL)
        g->field_a1_0xa1 |= 1;
}
