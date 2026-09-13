#include <stdio.h>
#include <string.h>

#include "decomp.h"
#include "legoapi/world/level.h"
#include "globals.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include "legoapi/characters/motion.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/menus/core/gamehint.h"
#include "legoapi/menus/core/gamemessage.h"
#include "legoapi/misc.h"
#include "legoapi/render/fx.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/levels/levels.h"
#include "legoapi/render/core/render.h"
#include "legoapi/world/world_shared.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nutrig.h"

// This level's view of the shared 16-byte LevFlag scratch. byte0 holds the
// pod-race progress state, byte1 the mushroom-collapse state.
struct LEVFLAGBYTES_s {
    u8 podrace_state;  // 0x00
    u8 mushroom_state; // 0x01
    u8 pad[14];        // 0x02
};
static_assert(sizeof(struct LEVFLAGBYTES_s) == 16, "LevFlag must be 16 bytes");
extern struct LEVFLAGBYTES_s LevFlag;

// Episode 1 level handlers: negotiations / gungan / rescue / podrace /
// podsprint / retake / maul / anakins-flight. This is the only implemented
// level file; episodes II-VI are still stubs.
//
// Functions appear in the original translation unit's address order (see
// res/libTTapp.so 0x1fa8e0..0x203d50). PodLevel and SetPodMergeAnims live at
// the end because the original binary defines them in other units.

// --- Cross-file entry points (C linkage) -----------------------------------
//
// Declared here rather than in a shared header because episodeI.cpp is their
// only consumer within the levels module.

extern "C" {
    void *AIPAthFindPathCnx(AISYS_s *, i32, char *, void *, void *); // original name keeps the typo
}

// --- Cross-file entry points (C++ linkage) ---------------------------------

void ResetPodStuff(void); // below
// PodSprintA_Update boulder-seek state.
extern "C" {
    float boulder_offset_y = 0.0f;
    float boulder_offset_y_seek = 15.0f;
}
void UpdatePodRaceLapDisplay(float); // defined below

// --- File-local layout types -----------------------------------------------

// One lap entry inside the pod race state (0x98-byte stride).
struct PODRACE_LAPENTRY_s {
    char pad_0x00[0x80];
    u32 *data; // 0x80
    char pad_0x84[0x94 - 0x84];
    void *next; // 0x94
};

// Per-level PodRace state block held at WORLDINFO.podrace (0x5120), 0xaf24
// bytes total (size of the memset in PodRaceInit).
struct PODRACE_s {
    char pad_0x0000[0xa580];
    PODRACE_LAPENTRY_s lap_entries[0x10]; // 0xa580 .. 0xaf00 (zeroed by PodRaceReset)
    float lap_countdown;                  // 0xaf00
    float mushroom_timer;                 // 0xaf04
    float lap_display;                    // 0xaf08
    float prev_lap_display;               // 0xaf0c
    char pad_0xaf10[0xaf20 - 0xaf10];
    u8 flags; // 0xaf20 bit1/bit0 cleared by PodRaceReset
    char pad_0xaf21[0xaf24 - 0xaf21];
};

// Pacemaker display data stored at LevObjs[0] for the pacemaker object.
struct PACEMAKERDATA_s {
    char pad_0x00[0x1340];
    u32 color1; // 0x1340
    u32 color2; // 0x1344
    u32 color3; // 0x1348
    char pad_0x134c[0x134e - 0x134c];
    u8 enabled; // 0x134e
};

// AI locator view used as gungan spawn groups (AIPathFindLocator results:
// position at 0x10, rotation at 0x1c). Overlays AILOCATOR_s.
struct GUNGAN_GROUP_s {
    char pad_0x00[0x10];
    NUVEC pos;             // 0x10
    i32 rot;               // 0x1c
    AIPATHINFO_s pathinfo; // 0x20 embedded route cursor
};

// WORLDINFO::lev_objs uses the engine's 16-byte special-entry layout.
struct WORLDLEVOBJ_s {
    void *special; // 0x00
    char pad_0x04[0xa];
    u8 enabled; // 0x0e
    char pad_0x0f[1];
};

// --- File-local state --------------------------------------------------------
//
// Names are the original statics (_ZL... symbols in res/libTTapp.so) and must
// not be renamed. Initial values match the original .data image.

static PODRACE_s *PodRace;            // _ZL7PodRace
static GameObject_s *pod_pacemaker;   // _ZL13pod_pacemaker
static float pod_pacemaker_alpha;     // _ZL19pod_pacemaker_alpha
static float podlapalpha;             // _ZL11podlapalpha
static float podhurryalpha;           // _ZL13podhurryalpha
static float podstartracealpha;       // _ZL17podstartracealpha
static i32 podhurry_i;                // _ZL10podhurry_i
static NUVEC pod_old_pos[2] __used__; // _ZL11pod_old_pos (0x18 bytes of .bss)

// Mushroom-collapse cutscene state (initial values from the .data image;
// mushroom_time_* are also mutated by Action_MushroomCollapse).
static CUTINFO *mushroom0_cut;                             // _ZL13mushroom0_cut
static i32 mushroom_collapse;                              // _ZL17mushroom_collapse
static i32 mushroom_nattempts_per_increment __used__ = 1;  // _ZL32mushroom_nattempts_per_increment
static i32 mushroom_n_attempts;                            // _ZL19mushroom_n_attempts
static float mushroom_countdown = 15.0f;                   // _ZL18mushroom_countdown
static float mushroom_time_available = 15.0f;              // _ZL23mushroom_time_available
static float mushroom_time_increment __used__ = 1.0f;      // _ZL23mushroom_time_increment
static float mushroom_max_time_available __used__ = 15.0f; // _ZL27mushroom_max_time_available
static float mushroom0_along __used__ = 65.0f;             // _ZL15mushroom0_along
static float mushroom2_along __used__ = 85.0f;             // _ZL15mushroom2_along

// Retake-G guard counters (net-shared through retakeg_netpacket).
static GIZAIMESSAGE_s *RetakeG_TotalGuards_msg;    // _ZL23RetakeG_TotalGuards_msg
static GIZAIMESSAGE_s *RetakeG_GuardsToRescue_msg; // _ZL26RetakeG_GuardsToRescue_msg

// Maul boss levels.
static GIZAIMESSAGE_s *MaulA_ai_message;   // _ZL16MaulA_ai_message
static GIZAIMESSAGE_s *MaulA_hits_message; // _ZL18MaulA_hits_message
static GameObject_s *Maul_obj;             // _ZL8Maul_obj

// Gungan plains wildlife spawner (0x114 bytes, layout verified against
// _ZL8gungan_a).
static struct {
    undefined pad_0x00[2];
    i16 count;         // 0x02 number of origin/target locator pairs
    void *origins[32]; // 0x04 AIPathFindLocator("origin_N")
    void *targets[32]; // 0x84 AIPathFindLocator("target_N")
    float spawn_timer; // 0x104
    i16 model_index;   // 0x108
    i16 pad_0x10a;
    i16 models[4]; // 0x10c kaadu, gungan, falumpaset, gungan
} gungan_a;

// ===========================================================================
// Sniper turrets (shared by PodRace C and PodSprint A)
// ===========================================================================

static __used__ void PodRaceSnipersUpdate(void) {
    i32 bolttype = (WORLD->current_level == PODSPRINTA_LDATA) ? 0x29 : 0x28;
    i32 n = PodRace_nsnipers;
    if (n <= 0 || max_nsnipers <= 0)
        return;
    for (i32 i = 0; i < n && i < max_nsnipers; i++) {
        SNIPER_s *s = &PodRace_snipers[i];
        GameObject_s *target = NULL;
        GameObject_s *p0 = Player[0];
        if (p0 != NULL && (p0->apiobj.field_0x1f8 & 0x1001) == 0x1001) {
            target = p0;
        } else {
            GameObject_s *p1 = Player[1];
            if (p1 != NULL && (p1->apiobj.field_0x1f8 & 0x1001) == 0x1001)
                target = p1;
        }
        if (target != NULL) {
            float dx = target->apiobj.pos_x - s->pos.x;
            float dy = target->apiobj.pos_z - s->pos.z;
            float dist2 = dx * dx + dy * dy;
            if (dist2 > PodRace_sniper_start_fire_radius * PodRace_sniper_start_fire_radius) {
                if (dist2 <= PodRace_sniper_fire_radius * PodRace_sniper_fire_radius) {
                    s->state = PodRace_sniper_fire_range_time;
                } else {
                    s->fire_timer += FRAMETIME;
                    if (s->fire_timer >= PodRace_sniper_fire_time) {
                        s->fire_timer = 0.0f;
                        if (s->state > 0.0f) {
                            // fire
                            float height = target->apiobj.pos_y - s->pos.y;
                            temp_yrot = NuAtan2D(dx, -dy);
                            temp_xrot = NuAtan2D(-height, dx);
                            NUMTX mtx;
                            NuMtxSetRotationX(&mtx, (u16)temp_xrot);
                            NuMtxRotateY(&mtx, (u16)temp_yrot);
                            Bolt_Add(NULL, &s->pos, &mtx, bolttype, 0);
                        }
                    }
                }
            }
        } else {
            s->fire_timer += FRAMETIME;
        }
    }
}

static __used__ void PodRaceSnipersReset(void) {
    max_nsnipers = (g_lowEndLevelBehaviour != 0) ? 2 : 5;
    PodRace_nsnipers = 0;
    if (Lap != 0) {
        char buf[0x1c];
        sprintf(buf, "Sniper%d", Lap);
        NUGSPLINE *spline = NuSplineFind(WORLD->current_gscn, buf);
        if (spline != NULL && spline->length > 0 && PodRace_nsnipers <= 9) {
            i32 npts = 0;
            do {
                SNIPER_s *dst = &PodRace_snipers[PodRace_nsnipers];
                memcpy(dst, &spline->pts[npts], 0x18);
                dst->fire_timer = NuRandFloat() * PodRace_sniper_fire_time + PodRace_sniper_fire_time * 0.5f;
                dst->state = 0.0f;
                PodRace_nsnipers++;
                npts += 2;
            } while (spline->length > npts && PodRace_nsnipers <= 9);
        }
    }
}

// Stub for the original _ZL17CreatePodRaceMineP7nuvec_s (defined in this unit
// at 0x1faed0). The tracked comparison copy currently lives in
// legoapi/ai/game/misc_a_game.cpp; this one stays unused so the compiler drops
// it exactly like today's build does.
static void *CreatePodRaceMine(nuvec_s *pos) {
    (void)pos;
    return NULL;
}

// Original: _ZL22PodSprint_InitAISplineP11WORLDINFO_sP20PODSPRINT_AISPLINE_sPc
// (inlined into its callers there; kept as a helper here).
static void PodSprint_InitAISpline(WORLDINFO_s *world, PODSPRINT_AISPLINE_s *ai, char *name) {
    NUGSPLINE *spl = ai->spline;
    i32 count = 0;
    for (i32 esi = 0; esi < (i32)(i16)spl->length - 1; esi++) {
        if (count > 5)
            break;
        NUVEC *pts = spl->pts;
        NUVEC *pts2 = ((count & 1) ? podsprint.finish_line : podsprint.halfway)->pts;
        if (XZLinesIntersect(&pts[esi], &pts[esi + 1], &pts2[0], &pts2[1], NULL, NULL)) {
            ai->vals[count] = (u16)esi;
            count++;
        }
    }
    if (ai->vals[5] == 0)
        ai->vals[5] = spl->length;
}

// Original: _ZL22UpdatePacemakerDisplayP11WORLDINFO_s.isra.7.part.8 — callers
// pass WORLD::lev_objs directly (that is what the .isra clone consumed).
static void UpdatePacemakerDisplay(void *lev_objs) {
    GameObject_s *pacemaker = pod_pacemaker;
    float v[3];
    v[0] = pacemaker->apiobj.field_0x190;
    v[1] = 0.75f + pacemaker->apiobj.field_0x194;
    v[2] = pacemaker->apiobj.field_0x198;
    GAMEMESSAGE_s *msg =
        (GAMEMESSAGE_s *)AddGameMessage(" ", (nuvec_s *)v, 0.08f, NULL, 0.0f, 0xff, 0x3f, 0x3f, 0x10083, 0);
    if (msg != NULL) {
        i32 idx = ((i32)(16384.0f * pod_pacemaker_alpha) >> 1) & 0x7fff;
        msg->icon = 0x134;
        msg->alpha = (u8)(i32)(128.0f * pacemaker_alpha_table[idx]);
        PACEMAKERDATA_s *data = *(PACEMAKERDATA_s **)lev_objs;
        if (data->enabled != 0) {
            msg->color1 = data->color1;
            msg->color2 = data->color2;
            msg->color3 = data->color3;
        }
    }
}

// Mine update — mirrors _ZL18UpdatePodRaceMinesv. Host: mines behind the
// camera despawn, mines touched by a vehicle explode (players die instead).
// Client: mines flagged in client_mines by the host explode on contact.
static __used__ void UpdatePodRaceMines(void) {
    GameObject_s *minesarr[64];
    i32 minecount = 0;

    // Collect active vehicles from the shared object pool (Obj, stride
    // 0x10e4); the pool end is Obj + 0x43900 (= 64 objects).
    for (GameObject_s *obj = (GameObject_s *)Obj; obj != (GameObject_s *)((u8 *)Obj + 0x43900); obj++) {
        if (obj != NULL && (obj->apiobj.field_0x1f8 & 0x1001) == 0x1001 && obj != pod_pacemaker)
            minesarr[minecount++] = obj;
    }

    MINESYS_s *mines = &minesys;

    // Host path runs inline first in the original; the client mirror sits at
    // the end of the function behind this early-out.
    if (netclient == 0) {
        GAMECAMERA_s *cam = GameCam;
        for (MINEENTRY_s *entry = &mines->mines[0]; entry != &mines->mines[64]; entry++) {
            if (entry->active == 0)
                continue;

            NUVEC delta;
            NuVecSub(&delta, &entry->pos, &cam->pos);
            float along = delta.x * cam->dir.x + delta.y * cam->dir.y + delta.z * cam->dir.z;
            if (along < 0.0f) {
                // Behind the camera: drop the mine again.
                i32 slot = (i32)(entry - &mines->mines[0]);
                u32 mask = ~(1u << (slot & 0x1f));
                pod_mines_bitfield[0] &= mask;
                pod_mines_bitfield[1] &= (i32)mask >> 31;
                memset(entry, 0, sizeof(*entry));
                mine_count--;
                continue;
            }

            entry->grow += FRAMETIME;
            if (entry->grow > 1.0f)
                entry->grow_alpha = 1.0f;
            else
                entry->grow_alpha = entry->grow;

            float r = entry->grow_alpha * mines->mine_radius;
            if (minecount == 0)
                continue;

            for (i32 i = 0; i < minecount; i++) {
                GameObject_s *obj = minesarr[i];
                if (obj == NULL)
                    continue;
                float dx = obj->apiobj.pos_x - entry->pos.x;
                float dz = obj->apiobj.pos_z - entry->pos.z;
                float rr = *(float *)((u8 *)obj + 0x1dc) + r;
                if (rr * rr <= dx * dx + dz * dz)
                    continue;

                if ((u8)obj->apiobj.field_0x27c == 0xff) {
                    // Player character: kill instead of exploding the pod.
                    KillPlayer(obj, 2, 1, NULL);
                    minesarr[i] = NULL;
                    continue;
                }

                if (mines->mine_debris != -1)
                    AddGameDebris(WORLD->debris_sys, mines->mine_debris, &entry->pos);
                if (mines->mine_part != -1)
                    AddFiniteShotPART(mines->mine_part, &entry->pos, 1);
                GameCam_HitJudder();
                GameCam_NewShake(NULL, 0.75f, 1.0f, 1.0f);
                PlaySfx("Explode1", (NUVEC *)((u8 *)obj + 0x80));
                PodLoseSpeed(obj, 1, 1);

                i32 slot = (i32)(entry - &mines->mines[0]);
                u32 mask = ~(1u << (slot & 0x1f));
                pod_mines_bitfield[0] &= mask;
                pod_mines_bitfield[1] &= (i32)mask >> 31;
                memset(entry, 0, sizeof(*entry));
                mine_count--;
                if (obj->apiobj.field_0x287 != 0)
                    minesarr[i] = NULL;
            }
        }
        return;
    }

    {
        float radius = mines->mine_radius;
        u32 *client = client_mines;
        for (u32 idx = 0; idx < 0x40; idx++) {
            u32 mask = 1u << (idx & 0x1f);
            // Words 0xc0/0xc1: host mine-present flags; 0xc2/0xc3: exploded ack.
            if (((client[(idx >> 5) + 0xc0] | client[(idx >> 5) + 0xc2]) & mask) == 0)
                continue;
            NUVEC *mine_pos = (NUVEC *)&client[idx * 3];
            for (i32 i = 0; i < minecount; i++) {
                GameObject_s *obj = minesarr[i];
                if (obj == NULL)
                    continue;
                float dx = obj->apiobj.pos_x - mine_pos->x;
                float dz = obj->apiobj.pos_z - mine_pos->z;
                float rr = *(float *)((u8 *)obj + 0x1dc) + radius;
                if (rr * rr <= dx * dx + dz * dz)
                    continue;
                if (mines->mine_debris != -1)
                    AddGameDebris(WORLD->debris_sys, mines->mine_debris, mine_pos);
                if (mines->mine_part != -1)
                    AddFiniteShotPART(mines->mine_part, mine_pos, 1);
                GameCam_HitJudder();
                GameCam_NewShake(NULL, 0.75f, 1.0f, 1.0f);
                PlaySfx("Explode1", mine_pos);
                client[(idx >> 5) + 0xc2] |= mask;
                break;
            }
        }
    }
}

// ===========================================================================
// Negotiations / Gungan plains / Rescue (coruscant)
// ===========================================================================

void NegotiationsA_Init(WORLDINFO_s *world) {
    GIZFORCE_s *f = GizForce_FindByName(world->giz_force_sys, "Force3");
    if (f != NULL)
        f->strength_0x6c = 0.4f;
    f = GizForce_FindByName(world->giz_force_sys, "Force2");
    if (f != NULL)
        f->strength_0x6c = 0.4f;
    f = GizForce_FindByName(world->giz_force_sys, "Force72");
    if (f != NULL)
        f->strength_0x6c = 0.75f;
}

void NegotiationsB_Init(WORLDINFO_s *world) {
    GIZFORCE_s *f = GizForce_FindByName(world->giz_force_sys, "Force10");
    if (f != NULL)
        f->strength_0x6c = 0.5f;
}

void GunganA_Init(WORLDINFO_s *world) {
    if (netclient == 0) {
        memset(&gungan_a, 0, sizeof(gungan_a));
        gungan_a.models[0] = id_KAADU;
        gungan_a.models[1] = id_GUNGAN;
        gungan_a.models[2] = id_FALUMPASET;
        gungan_a.models[3] = id_GUNGAN;
        for (i32 i = 0; i < 32; i++) {
            char buf[16];
            sprintf(buf, "origin_%d", i);
            gungan_a.origins[i] = AIPathFindLocator(world->ai_sys, buf);
            sprintf(buf, "target_%d", i);
            gungan_a.targets[i] = AIPathFindLocator(world->ai_sys, buf);
            if (gungan_a.origins[i] == NULL || gungan_a.targets[i] == NULL)
                break;
            gungan_a.count++;
        }
    }
    GIZMOBLOWUP_s *b = GizmoBlowUp_FindByName(world, "Trunk2_exp11");
    if (b != NULL) {
        UpdateMidPos(b);
        b->field_0x124 = 1;
        b->field_0x128 = b->target_scale;
        GIZMOBLOWUP_s *b2 = GizmoBlowUp_FindByName(world, "leaves_exp11");
        if (b2 != NULL) {
            b2->field_0x124 = 1;
            b2->field_0x120 = (void *)&b->field_0x50;
            b2->field_0x128 = b->target_scale;
        }
        GIZMOBLOWUP_s *b3 = GizmoBlowUp_FindByName(world, "branch3_exp11");
        if (b3 != NULL) {
            b3->field_0x124 = 1;
            b3->field_0x120 = (void *)&b->field_0x50;
            b3->field_0x128 = b->target_scale;
        }
    }
}

void GunganA_Update(WORLDINFO_s *world) {
    (void)world;
    if (netclient != 0)
        return;
    gungan_a.spawn_timer += FRAMETIME;
    float maxtime = gungan_a_time_Normal;
    if (g_lowEndLevelBehaviour != 0)
        maxtime = gungan_a_time_LowEnd;
    if (gungan_a.count != 0 && gungan_a.spawn_timer > maxtime) {
        i32 r = NuRand(NULL) % gungan_a.count;
        // Normal limits: four neutrals and fourteen enemies; low-end: three/six.
        i32 nh = (g_lowEndLevelBehaviour == 0) ? 14 : 6;
        i32 nb = (g_lowEndLevelBehaviour == 0) ? 4 : 3;
        GameObject_s *obj;
        if (nb > active_neutral_count) {
            i32 nv = (gungan_a.model_index + 1) % 4;
            gungan_a.model_index = (i16)nv;
            i32 model = gungan_a.models[nv];
            GUNGAN_GROUP_s *grp = (GUNGAN_GROUP_s *)gungan_a.origins[r];
            obj = AddDynamicCreature(model, &grp->pos, grp->rot, "Wildlife", &grp->pathinfo, NULL, 0, NULL, NULL, 0, 0);
        } else if (nh > active_baddy_count) {
            i32 pick = NuRand(NULL);
            if ((pick & 1) != 0) {
                GUNGAN_GROUP_s *grp = (GUNGAN_GROUP_s *)gungan_a.origins[r];
                obj =
                    AddDynamicCreature(id_STAP, &grp->pos, grp->rot, "STAP", &grp->pathinfo, NULL, 0, NULL, NULL, 0, 0);
            } else {
                GUNGAN_GROUP_s *grp = (GUNGAN_GROUP_s *)gungan_a.origins[r];
                obj = AddDynamicCreature(id_BATTLEDROID, &grp->pos, grp->rot, "Battledroid", &grp->pathinfo, NULL, 0,
                                         NULL, NULL, 0, 0);
            }
        } else {
            goto reset_spawn_timer;
        }
        if (obj != NULL) {
            const bool normal = g_lowEndLevelBehaviour == 0;
            obj->ai.field_0x364 = gungan_a.targets[r];
            if (normal)
                obj->field_0xf04 &= 0x7f;
        }
    reset_spawn_timer:
        gungan_a.spawn_timer = 0.25f - NuFloatRand(NULL) * 0.5f;
    }
}

void RescueA_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *g = GizmoBlowUp_FindByName(world, "deton_021");
    if (g != NULL)
        g->field_0xa0 |= 2;
    g = GizmoBlowUp_FindByName(world, "deton_011");
    if (g != NULL)
        g->field_0xa0 |= 2;
    g = GizmoBlowUp_FindByName(world, "pod_071");
    if (g != NULL)
        g->field_0xa0 |= 2;
    g = GizmoBlowUp_FindByName(world, "pod_081");
    if (g != NULL)
        g->field_0xa0 |= 2;
}

void RescueB_Init(WORLDINFO_s *) {
}

void RescueC_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *g;
    if ((g = GizmoBlowUp_FindByName(world, "pod31")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "pod11")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "pod51")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "pod21")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "target_a31")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "target_a51")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "target_a41")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "target_a61")) != NULL)
        g->field_0x124 = 1;
    if ((g = GizmoBlowUp_FindByName(world, "flower_tub31")) != NULL)
        g->field_0xa0 |= 2;
    if ((g = GizmoBlowUp_FindByName(world, "flower_tub1")) != NULL)
        g->field_0xa0 |= 2;
    char buf[0x1c];
    for (i32 i = 1; i <= 9; i++) {
        sprintf(buf, "window_balcony%d", i);
        if ((g = GizmoBlowUp_FindByName(world, buf)) != NULL) {
            g->field_0x124 = 1;
            g->field_0xa0 |= 2;
        }
    }
    for (i32 i = 0xa; i <= 0xc; i++) {
        sprintf(buf, "window_balcon%d", i);
        if ((g = GizmoBlowUp_FindByName(world, buf)) != NULL) {
            g->field_0x124 = 1;
            g->field_0xa0 |= 2;
        }
    }
}

void RescueE_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *g = GizmoBlowUp_FindByName(world, "roof_light61");
    if (g != NULL)
        g->field_0xa0 |= 2;
}

// ===========================================================================
// Pod race core (A/B/C laps)
// ===========================================================================

void ResetPodStuff() {
    i16 val = apicharsys->playermodelids[id_ANAKINSPOD];
    float *fp = NULL;
    if (val != -1) {
        APICHARACTERMODEL *entry = &apicharsys->models[val];
        fp = ((PODMODELDATA_s *)entry->model_data_b)->value;
    }
    if (fp != NULL && *fp > 0.0f) {
        pod_animtime[0] = 1.0f;
        pod_roll_target[0] = 0.0f;
        pod_roll[0] = 0.0f;
        pod_roll_target[1] = 0.0f;
        pod_animtime[1] = 1.0f;
        pod_roll[1] = 0.0f;
        pod_animtime[0] = (float)qrand() * 1.5259021893143654e-05f * (*fp - 1.0f) + pod_animtime[0];
        pod_animtime[1] = (float)qrand() * 1.5259021893143654e-05f * (*fp - 1.0f) + pod_animtime[1];
    } else {
        pod_roll_target[0] = 0.0f;
        pod_roll_target[1] = 0.0f;
        pod_roll[0] = 0.0f;
        pod_roll[1] = 0.0f;
        pod_animtime[0] = 1.0f;
        pod_animtime[1] = 1.0f;
    }
}

void PodRace_IncreaseLap() {
    Lap++;
}

void PodRaceUpdate(WORLDINFO_s *, float) {
    if (netclient != 0)
        return;
    PODRACE_s *podrace = PodRace;
    if (podrace->lap_countdown <= 0.0f)
        return;
    avg_currentspeed_mul = 0;
    podrace->lap_display = podrace->prev_lap_display;
    if (Player[0] != NULL && (Player[0]->apiobj.field_0x1f8 & 0x80))
        return;
    if (Player[1] != NULL && !(Player[1]->apiobj.field_0x1f8 & 0x80))
        Player[1]->field_0xc34 = 0;
    i32 n = (i32)podrace->lap_countdown;
    i32 o = (i32)podrace->lap_display;
    if (n != o)
        PlaySfx("Pod_Race_Light", NULL);
    for (i32 i = 0; i < 0x10; i++) {
        PODRACE_LAPENTRY_s *entry = &podrace->lap_entries[i];
        if (entry->next == NULL)
            continue;
        u32 *p = entry->data;
        if (p != NULL) {
            // Body optimized out in the original binary.
        }
    }
}

void PodRacePanel(WORLDINFO_s *world) {
    if (netclient != 0) {
        podlapalpha = podrace_netpacket->countdown;
        if (podlapalpha > 0.0f) {
            WORLDLEVOBJ_s *entry = &((WORLDLEVOBJ_s *)world->lev_objs)[Lap + 0x135];
            if (entry->enabled != 0)
                return;
            Text3DEx(NULL, 0, 1.0f, 0.16f, 0.16f, 0.16f, (u16)0, (u16)0, (u16)0, 0, 0x3f, 0);
        } else {
            if (podhurryalpha > 0.0f && PodRace != NULL && PodRace->lap_display > 0.0f) {
                char buf[0x20];
                sprintf(buf, "%i", (i32)PodRace->lap_display + 1);
                if (NuFmod(PodRace->lap_display, 1.0f) < 0.7f)
                    Text3DEx(buf, 0, 0.4f, 1.0f, 0.75f, 0.75f, 0.75f, 0, 0xff, 0x3f, 0,
                             (u8)(i32)(128.0f * podhurryalpha));
            }
            if (podstartracealpha > 0.0f && PodRace != NULL && PodRace->lap_countdown > 0.0f) {
                char buf[0x20];
                sprintf(buf, "%i", (i32)PodRace->lap_countdown + 1);
                if (NuFmod(PodRace->lap_countdown, 1.0f) < 0.7f)
                    Text3DEx(buf, 0, 0.4f, 1.0f, 0.75f, 0.75f, 0.75f, 0, 0, 0, 0,
                             (u8)(i32)(128.0f * podstartracealpha));
            }
        }
    } else {
        podrace_netpacket->countdown = podlapalpha;
        if (podhurryalpha > 0.0f && PodRace != NULL && PodRace->lap_display > 0.0f) {
            char buf[0x20];
            sprintf(buf, "%i", (i32)PodRace->lap_display + 1);
            if (NuFmod(PodRace->lap_display, 1.0f) < 0.7f)
                Text3DEx(buf, 0, 0.4f, 1.0f, 0.75f, 0.75f, 0.75f, 0, 0xff, 0x3f, 0, (u8)(i32)(128.0f * podhurryalpha));
        }
        if (podstartracealpha > 0.0f && PodRace != NULL && PodRace->lap_countdown > 0.0f) {
            char buf[0x20];
            sprintf(buf, "%i", (i32)PodRace->lap_countdown + 1);
            if (NuFmod(PodRace->lap_countdown, 1.0f) < 0.7f)
                Text3DEx(buf, 0, 0.4f, 1.0f, 0.75f, 0.75f, 0.75f, 0, 0, 0, 0, (u8)(i32)(128.0f * podstartracealpha));
        }
    }
}

void UpdatePodRaceLapDisplay(float arg) {
    if (FadeSys.fade == 0.0f && MiniCutCam == 0 && CUTSTOPGAME == 0) {
        if (Paused != 0) {
            podlapalpha = SeekLinearF(podlapalpha, 1.0f, arg + arg);
            return;
        } else {
            float t = 0.0f;
            if (WORLD->current_level == PODRACEB_LDATA && GameTimer.time_elapsed >= 1.0f &&
                GameTimer.time_elapsed > 6.0f)
                t = 1.0f;
            podlapalpha = SeekLinearF(podlapalpha, t, arg + arg);
        }
    } else {
        podlapalpha = 0.0f;
        podhurryalpha = 0.0f;
        podstartracealpha = 0.0f;
        if (Paused != 0)
            return;
    }
    if (PodRace != NULL && PodRace->lap_display < 10.0f) {
        i32 oldhurry = podhurry_i;
        podhurry_i = (i32)PodRace->lap_display;
        if (podhurryalpha < 1.0f) {
            float x = arg * 2.0f + podhurryalpha;
            podhurryalpha = x < 1.0f ? x : 1.0f;
        }
        if (podhurry_i > 0 && oldhurry != podhurry_i) {
            TickTockSfx();
            if (Paused != 0)
                return;
        }
    }
    if (PodRace != NULL && PodRace->lap_countdown > 0.0f && podstartracealpha < 1.0f) {
        float x = arg * 2.0f + podstartracealpha;
        podstartracealpha = 1.0f >= x ? x : 1.0f;
    }
}

void PodRaceAlwasyUpdate(WORLDINFO_s *world) { // original spelling
    UpdatePodRaceLapDisplay(FRAMETIME);
}

i32 PodRace_InStartCountdown(WORLDINFO_s *world) {
    if (world->area != NULL && world->area == PODRACE_ADATA && PodRace != NULL &&
        PodRace->lap_countdown > 4.158760129802644e+21f)
        return 1;
    return 0;
}

void PodRaceAUpdate(WORLDINFO_s *world) {
    if (pod_pacemaker != 0) {
        if (FadeSys.fade == 0.0f || pause_rndr_on != 0)
            pod_pacemaker_alpha = 0.0f;
    }
    PodRaceUpdate(world, FRAMETIME);
    if (netclient != 0 || Lap > 3 || MiniCutCam != 0) {
        UpdatePodRaceMines();
        return;
    }
    MINESYS_s *mines = &minesys;
    float t = mines->update_timer + FRAMETIME;
    mines->update_timer = t;
    if (t > 1.0f) {
        GAMECAMERA_s *cam = GameCam;
        if (mines->spawn_timer == 1000000000.0f) {
            float v = cam->sock_position.distance;
            mines->spawn_timer = v;
            if (v < 100.0f) {
                nuvec_s vec = {(0.5f - NuRandFloat()) * 60.0f, 0.0f, 100.0f};
                NuVecRotateY(&vec, &vec, player->yrot);
                nuvec_s p;
                NUVEC player_pos = {player->apiobj.pos_x, player->apiobj.pos_y, player->apiobj.pos_z};
                NuVecAdd(&p, &vec, &player_pos);
                if (CreatePodRaceMine(&p) != NULL)
                    mines->spawn_timer = cam->sock_position.distance;
            }
        }
    }
}

void PodRaceA_AlwaysUpdate(WORLDINFO_s *world) {
    PodRaceAlwasyUpdate(world);
    if (Lap == 3) {
        if (InStory() != 0) {
            if (PODRACEOUTRO1_LDATA != NULL) {
                other_level_override = PODRACEOUTRO1_LDATA->idx;
                return;
            }
        }
        if (PODRACESTATUS_LDATA != NULL)
            other_level_override = PODRACESTATUS_LDATA->idx;
    }
}

void PodRaceADraw(WORLDINFO_s *world) {
    if (netclient != 0) {
        NUMTX mtx;
        for (i32 i = 0; i < 0x40; i++) {
            i32 bit = 1 << (i & 0x1f);
            if (((client_mines[0x300 / 4] & bit) | (client_mines[0x304 / 4] & (bit >> 31))) != 0) {
                NuMtxSetIdentity(&mtx);
                NuMtxTranslate(&mtx, (NUVEC *)&client_mines[i * 3]);
                NuSpecialDrawAt(&minesys, &mtx);
            }
        }
    } else if (NuSpecialExistsFn(&minesys) != 0) {
        NUMTX mtx;
        MINESYS_s *mines = &minesys;
        for (MINEENTRY_s *entry = mines->mines; entry != &mines->mines[64]; entry++) {
            if (entry->active != 0) {
                float s = entry->grow_alpha;
                NUVEC scale = {s, s, s};
                NuMtxSetScale(&mtx, &scale);
                NuMtxTranslate(&mtx, &entry->pos);
                NuMtxPreRotateX(&mtx, entry->rotx);
                NuMtxPreRotateY(&mtx, entry->roty);
                NuSpecialDrawAt(&minesys, &mtx);
            }
        }
    }
}

void PodRaceBUpdate(WORLDINFO_s *world) {
    if (Lap == 2) {
        GAMECAMERA_s *gamcam = GameCam;
        CUTINFO *cut = (CUTINFO *)mushroom0_cut;
        if (PodRace->mushroom_timer > 1.0f && mushroom_collapse != 0) {
            switch (LevFlag.mushroom_state) {
                case 0:
                    if (gamcam->sock_position.distance > mushroom0_along) {
                        NewCutScene(NULL, world->cutscene_sys, "ep1_podrace_mushroom0", 1);
                        LevFlag.mushroom_state = 1;
                    }
                    break;
                case 1:
                    if (cut != NULL && (((CUTSCENEDATA_s *)cut->scene)->flags & 0x10)) {
                        NewCutScene(NULL, world->cutscene_sys, "EP1_PODRACE_MUSHROOM1", 1);
                        mushroom_countdown = mushroom_time_available;
                        LevFlag.mushroom_state = 2;
                    }
                    break;
                case 2:
                    mushroom_countdown -= FRAMETIME;
                    if (mushroom_countdown > 0.0f) {
                        if (gamcam->sock_position.distance > mushroom2_along)
                            LevFlag.mushroom_state = 3;
                    } else {
                        NewCutScene(NULL, world->cutscene_sys, "EP1_PODRACE_MUSHROOM2", 1);
                        mushroom_n_attempts++;
                        if (mushroom_nattempts_per_increment > 0 &&
                            mushroom_n_attempts % mushroom_nattempts_per_increment == 0) {
                            mushroom_time_available =
                                mushroom_time_available + mushroom_time_increment < mushroom_max_time_available
                                    ? mushroom_time_available + mushroom_time_increment
                                    : mushroom_max_time_available;
                        }
                        LevFlag.mushroom_state = 4;
                    }
                    break;
                case 3:
                    if (NuSpecialExistsFn(LevHSpecial) != 0 &&
                        NuSpecialClipTestExtents(LevHSpecial, NuSpecialGetDrawMtx(LevHSpecial)) == 0)
                        LevFlag.mushroom_state = 4;
                    break;
            }
        }
    }
    if (pod_pacemaker != 0) {
        if (FadeSys.fade != 0.0f && pause_rndr_on == 0) {
            float t = GameTimer.time_elapsed_mod_seconds;
            pod_pacemaker_alpha =
                pod_pacemaker_alpha + FRAMETIME * 2.0f < 1.0f ? pod_pacemaker_alpha + FRAMETIME * 2.0f : 1.0f;
            if (NuFmod(t, 0.2f) > 0.1f)
                UpdatePacemakerDisplay(world->lev_objs);
        } else {
            pod_pacemaker_alpha = 0.0f;
        }
    }
    UpdatePodRaceLapDisplay(FRAMETIME);
    PodRaceUpdate(world, FRAMETIME);
    if (Lap == 1) {
        if (LevFlag.podrace_state == 0) {
            if (GameTimer.time_elapsed > 10.0f) {
                Hint_SetComplete(0x27e);
                LevFlag.podrace_state = 1;
            }
        }
    }
}

void PodRaceCUpdate(WORLDINFO_s *world) {
    if (pod_pacemaker != 0) {
        if (FadeSys.fade != 0.0f && pause_rndr_on == 0) {
            float t = pod_pacemaker_alpha + FRAMETIME * 2.0f;
            pod_pacemaker_alpha = t < 1.0f ? t : 1.0f;
            if (NuFmod(GameTimer.time_elapsed_mod_seconds, 0.2f) > 0.1f)
                UpdatePacemakerDisplay(world->lev_objs);
        } else {
            pod_pacemaker_alpha = 0.0f;
        }
    }
    UpdatePodRaceLapDisplay(FRAMETIME);
    PodRaceUpdate(world, FRAMETIME);
    PodRaceSnipersUpdate();
    switch (LevFlag.podrace_state) {
        case 0: {
            CUTINFO *cs = CutScene_Find(world->cutscene_sys, "Ep1_Podrace_TuskenRaiders");
            if (cs != NULL && (((CUTSCENEDATA_s *)((CUTINFO *)cs)->scene)->flags & 0x10))
                LevFlag.podrace_state = 1;
            break;
        }
        case 1: {
            i32 none = 1;
            nuhspecial_s *slots = LevHSpecial;
            for (i32 i = 1; i <= 9; i++) {
                if (NuSpecialExistsFn(&slots[i]) != 0) {
                    NuSpecialSetVisibility(&slots[i], 1);
                    none = 0;
                }
            }
            if (none)
                LevFlag.podrace_state = 2;
            break;
        }
        default:
            break;
    }
}

void PodRaceReset() {
    PODRACE_s *podrace = PodRace;
    podrace->mushroom_timer = 0.0f;
    memset(podrace->lap_entries, 0, sizeof(podrace->lap_entries));
    podrace->flags &= 0xfc;
    podhurry_i = -1;
    PodKeyReset();
}

void PodRaceCReset(WORLDINFO_s *world) {
    PodRaceReset();
    switch (Lap) {
        case 1:
            podrace_section = 2;
            break;
        case 2:
            podrace_section = 5;
            break;
        case 3:
            podrace_section = 8;
            break;
        default:
            break;
    }
    pod_pacemaker = 0;
    PodRaceSnipersReset();
    CUTINFO *cs = CutScene_Find(world->cutscene_sys, "ep1_podrace_avalanche");
    if (cs != NULL) {
        if (Lap == 2)
            NewCutScene(cs, world->cutscene_sys, 0, 1);
        else if (Lap == 3)
            CutScene_SnapToEnd(cs);
    }
}

void PodRaceBReset(WORLDINFO_s *world) {
    PodRaceReset();
    switch (Lap) {
        case 1:
            podrace_section = 1;
            break;
        case 2:
            podrace_section = 4;
            break;
        case 3:
            podrace_section = 7;
            break;
    }
    pod_pacemaker = 0;
    LevFlag.podrace_state = 0;
    LevFlag.mushroom_state = 0;
    mushroom0_cut = CutScene_Find(world->cutscene_sys, "EP1_PODRACE_MUSHROOM0");
    NuSpecialFind(world->current_gscn, &LevHSpecial[0], "collapsing_mush", 1);
}

void PodRaceAReset(WORLDINFO_s *world) {
    PodRaceReset();
    switch (Lap) {
        case 1:
            podrace_section = 3;
            break;
        case 2:
            podrace_section = 6;
            break;
        case 3:
            podrace_section = 9;
            break;
        default:
            break;
    }
    pod_pacemaker = 0;
    MINESYS_s *mines = &minesys;
    mines->spawn_timer = 1000000000.0f;
    clients_mines_bitfield[0] = 0;
    clients_mines_bitfield[1] = 0;
    mines->update_timer = 0.0f;
    pod_mines_bitfield[0] = 0;
    pod_mines_bitfield[1] = 0;
    memset(mines->mines, 0, sizeof(mines->mines));
    memset(client_mines, 0, 0xc5 * 4);
    mine_count = 0;
    if (Lap == 3 && nethost == 0 && netclient == 0)
        NewCutScene(NULL, world->cutscene_sys, "ep1_podrace_sebulba", 1);
}

void PodRaceInit(WORLDINFO_s *world) {
    podrace_netpacket = (PODRACENETPACKET_s *)(usize)SetLevelHack(0x14);
    PODRACE_s *podrace = (PODRACE_s *)world->podrace;
    if (podrace == NULL) {
        ChrisAllocLevelStuff(world);
        podrace = (PODRACE_s *)world->podrace;
    }
    PodRace = podrace;
    memset(podrace, 0, sizeof(*podrace)); // 0xaf24 bytes in the original
    if (netclient != 0) {
        memset(&minesys, 0, 0x1d2 * 4);
        memset(client_mines, 0, 0xc5 * 4);
        MINESYS_s *mines = &minesys;
        if (NuSpecialFind(vehicle_scene, &mines->mine_special, "mine", 1) != 0) {
            mines->mine_radius = NuSpecialGetOriginRadius(&mines->mine_special);
            char b2[0x20];
            {
                sprintf(b2, "nomine_%d", 0);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 1);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 2);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 3);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 4);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 5);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 6);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 7);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 8);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
                sprintf(b2, "nomine_%d", 9);
                mines->nomine_areas[mines->nomine_count] = AISysFindArea(WORLD->ai_sys, b2);
                mines->nomine_count++;
            }
            mines->mine_debris = FindGameDebris(WORLD->debris_sys, "MINE_POP");
            mines->mine_part = PARTLookupType("POD_MINE_PART");
        }
    } else {
        FlightSpline_Init(world, (flightspline_s *)podrace, 0x20);
    }
    PodKeyReset();
    ResetPodStuff();
    i16 id = apicharsys->playermodelids[id_ANAKINSPOD];
    if (id != -1) {
        APICHARACTERMODEL *entry = &apicharsys->models[id];
        if (entry->model_data_b != NULL && ((PODMODELDATA_s *)entry->model_data_b)->value != NULL)
            podanimendframe = AnimEndFrame(entry, 1);
    }
}

void PodRaceCInit(WORLDINFO_s *world) {
    PodRaceInit(world);
    LevFlag.podrace_state = 0;
    char buf[0x20];
    nuhspecial_s *slots = LevHSpecial;
    sprintf(buf, "boost0%i", 1);
    NuSpecialFind(world->current_gscn, &slots[0], buf, 1);
    sprintf(buf, "boost0%i", 2);
    NuSpecialFind(world->current_gscn, &slots[1], buf, 1);
    sprintf(buf, "boost0%i", 3);
    NuSpecialFind(world->current_gscn, &slots[2], buf, 1);
    sprintf(buf, "boost0%i", 4);
    NuSpecialFind(world->current_gscn, &slots[3], buf, 1);
    sprintf(buf, "boost0%i", 5);
    NuSpecialFind(world->current_gscn, &slots[4], buf, 1);
    sprintf(buf, "boost0%i", 6);
    NuSpecialFind(world->current_gscn, &slots[5], buf, 1);
    sprintf(buf, "boost0%i", 7);
    NuSpecialFind(world->current_gscn, &slots[6], buf, 1);
    sprintf(buf, "boost0%i", 8);
    NuSpecialFind(world->current_gscn, &slots[7], buf, 1);
    sprintf(buf, "boost0%i", 9);
    NuSpecialFind(world->current_gscn, &slots[8], buf, 1);
    sprintf(buf, "boost0%i", 10);
    NuSpecialFind(world->current_gscn, &slots[9], buf, 1);

    // Pod-race boost sprite activation.
    if (FreePlay != 0 || (*(u8 *)((char *)LevelProgressData + (i16)world->current_level->idx * 0x2e24 + 0x2800) & 1)) {
        for (i32 i = 0; i < 10; i++) {
            if (NuSpecialExistsFn(&slots[i]) != 0)
                NuSpecialSetVisibility(&slots[i], 1);
        }
        LevFlag.podrace_state = 2;
    } else {
        for (i32 i = 0; i < 17; i++) {
            if (NuSpecialExistsFn(&slots[i]) != 0)
                NuSpecialSetVisibility(&slots[i], 0);
        }
    }
}

void PodRaceBInit(WORLDINFO_s *world) {
    PodRaceInit(world);
    mushroom_collapse = 0;
    mushroom_nattempts_per_increment = 1;
    mushroom_n_attempts = 0;
    if (Lap <= 1)
        PodRace->lap_countdown = 3.0f;
}

void PodRaceAInit(WORLDINFO_s *world) {
    PodRaceInit(world);
}

// ===========================================================================
// Pod sprint (Sebulba events)
// ===========================================================================

float PodSprint_InStartCountdown(WORLDINFO_s *world) {
    if (world->current_level != PODSPRINTA_LDATA)
        return 0.0f;
    return podsprint.speed;
}

void PodSprintA_Init(WORLDINFO_s *world) {
    PODSPRINT_s *ps = &podsprint;
    memset(ps, 0, sizeof(*ps));
    podsprint_netpacket = (PODSPRINTNETPACKET_s *)(usize)SetLevelHack(0xa);

    ps->finish_line = NuSplineFind(world->current_gscn, "finish_line");
    if (ps->finish_line != NULL && (i16)ps->finish_line->length <= 1)
        ps->finish_line = NULL;
    ps->halfway = NuSplineFind(world->current_gscn, "halfway");
    if (ps->halfway != NULL && (i16)ps->halfway->length <= 1)
        ps->halfway = NULL;

    ps->lap_msg = CheckGizAIMessage(gizaimessagesys, "Lap", NULL);
    ps->lap_msg->value = 0.0f;
    ps->max_speed_msg = CheckGizAIMessage(gizaimessagesys, "sebulba_max_speed", NULL);
    ps->min_speed_msg = CheckGizAIMessage(gizaimessagesys, "sebulba_min_speed", NULL);
    ps->speed_step_msg = CheckGizAIMessage(gizaimessagesys, "sebulba_speed_step", NULL);

    ps->ai[0].spline = NuSplineFind(world->current_gscn, "ai_sebulba");
    if (ps->ai[0].spline != NULL && ps->finish_line != NULL && ps->halfway != NULL)
        PodSprint_InitAISpline(world, &ps->ai[0], "ai_sebulba");

    ps->ai[1].spline = NuSplineFind(world->current_gscn, "ai_general");
    if (ps->ai[1].spline != NULL && ps->finish_line != NULL && ps->halfway != NULL)
        PodSprint_InitAISpline(world, &ps->ai[1], "ai_general");

    nuhspecial_s *bigrocks = LevHSpecial;
    NuSpecialFind(world->current_gscn, &bigrocks[50], "bigrock_five", 1);
    NuSpecialFind(world->current_gscn, &bigrocks[51], "bigrock_eight", 1);
}

void PodSprintA_Reset(WORLDINFO_s *world) {
    PODSPRINT_s *ps = &podsprint;
    u8 b = ps->flags;
    ps->field_0x78 = 0;
    ps->flags = b & 0xef;
    pod_old_pos[0].x = -1.0f;
    pod_old_pos[0].y = -1.0f;
    pod_old_pos[0].z = -1.0f;
    pod_old_pos[1].x = -1.0f;
    pod_old_pos[1].y = -1.0f;
    pod_old_pos[1].z = -1.0f;
    ps->field_0x88 = 0;
    if ((b & 0xc) != 0 || (netclient != 0 && podsprint_netpacket->ai_state > 2)) {
        ps->ai_state = 3;
        ps->flags &= 0xf2;
        ps->ai_index = 4;
        VehicleAreaRememberSpeed = 1.0f;
        GameObject_s *p = player;
        if (p != NULL && (p->apiobj.field_0x1f8 & 0x1000)) {
            p->field_0xdc8 = 1.0f;
            p->apiobj.velocity.x = 0.0f;
            p->apiobj.velocity.y = 0.0f;
            p->apiobj.velocity.z = ((PLAYERSUBOBJ2_s *)((PLAYERSUBOBJ_s *)p->apiobj.character_data)->field_0x24)->value;
            NuVecRotateY(&p->apiobj.velocity, &p->apiobj.velocity, p->apiobj.field_0x276);
        } else if (player2 != NULL && (player2->apiobj.field_0x1f8 & 0x1000)) {
            GameObject_s *p2 = player2;
            p2->field_0xdc8 = 1.0f;
            p2->apiobj.velocity.x = 0.0f;
            p2->apiobj.velocity.y = 0.0f;
            p2->apiobj.velocity.z =
                ((PLAYERSUBOBJ2_s *)((PLAYERSUBOBJ_s *)p2->apiobj.character_data)->field_0x24)->value;
            NuVecRotateY(&p2->apiobj.velocity, &p2->apiobj.velocity, p2->apiobj.field_0x276);
        }
        void *cs = game_cutscenes.cutscene;
        if (cs != NULL) {
            CutScene_SnapToEnd((CUTINFO *)cs);
            CutScene_StoppedFn_LSW((CUTINFO *)cs);
        }
    } else {
        ps->ai_state = 1;
        ps->ai_index = 0;
        ps->flags &= 0xfe;
        ps->speed = 3.0f;
        podstartracealpha = 0.0f;
    }
    PodRaceSnipersReset();
    ps->boulders = AISysFindArea(world->ai_sys, "Boulders");
}

void PodSprintA_Update(WORLDINFO_s *world) {
    PODSPRINT_s *ps = &podsprint;
    PODSPRINTNETPACKET_s *net = podsprint_netpacket;
    VehicleAreaRememberSpeed = 1.0f;
    if (nethost != 0) {
        net->ai_state = (i16)(i8)ps->ai_state;
        net->speed = (i16)ps->speed;
        net->speed2 = (i16)ps->field_0x88;
        if (netclient != 0)
            goto speed_section;
    } else if (netclient != 0) {
        ps->speed = (float)(i16)net->speed;
        ps->ai_state = (u8)net->ai_state;
        ps->field_0x88 = (float)(i16)net->speed2;
        goto speed_section;
    }
    // netclient == 0: react to cutscene / free-play flag.
    if (CutScene_PlayingOrRequested(NULL))
        return;
    if ((ps->flags & 0xc) != 0) {
        if (!(ps->flags & 0x10)) {
            ps->flags |= 0x10;
            ResetLevel(world, NULL, 1);
        }
        return;
    }
speed_section:
    if (ps->speed > 0.0f) {
        float v = ps->speed;
        if (FadeSys.fade == 0.0f)
            ps->speed -= FRAMETIME;
        if (Player[0] != NULL) {
            Player[0]->field_0xdc8 = 0.0f;
            Player[0]->apiobj.velocity.z = 0.0f;
            Player[0]->apiobj.velocity.x = 0.0f;
            Player[0]->field_0xee0 = ps->speed <= 0.0f ? 1000000000.0f : 0.0f;
        }
        if (Player[1] != NULL) {
            Player[1]->field_0xdc8 = 0.0f;
            Player[1]->apiobj.velocity.z = 0.0f;
            Player[1]->apiobj.velocity.x = 0.0f;
            Player[1]->field_0xee0 = ps->speed <= 0.0f ? 1000000000.0f : 0.0f;
            if (v < 2.9f && (i32)v != (i32)ps->speed)
                PlaySfx("Pod_Race_Light", NULL);
        }
    } else {
        // ps->speed <= 0: pacemaker display.
        if (ps->field_0x78 == NULL)
            ps->field_0x78 = GetNamedGameObject(world->ai_sys, "SebulbasPod");
        if (ps->field_0x78 != NULL && (*(u16 *)((u8 *)ps->field_0x78 + 0x1f9) & 0x10) &&
            *(u8 *)((u8 *)ps->field_0x78 + 0x287) == 0) {
            if (FadeSys.fade == 0.0f && pause_rndr_on == 0) {
                float t = ps->field_0x80 + FRAMETIME * 2.0f;
                ps->field_0x80 = t < 1.0f ? t : 1.0f;
                if (NuFmod(GameTimer.time_elapsed_mod_seconds, 0.2f) < 0.1f) {
                    GAMEMESSAGE_s *msg = (GAMEMESSAGE_s *)AddGameMessage(
                        " ", (NUVEC *)((u8 *)ps->field_0x78 + 0x190), 0.08f, NULL, 0.0f, 0xff, 0x3f, 0x3f, 0x10083, 0);
                    if (msg != NULL) {
                        msg->icon = 0x134;
                        i32 idx = ((i32)(16384.0f * ps->field_0x80) >> 1) & 0x7fff;
                        msg->alpha = (u8)(i32)(128.0f * NuTrigTable[idx]);
                        PACEMAKERDATA_s *pd = *(PACEMAKERDATA_s **)world->lev_objs;
                        if (pd->enabled) {
                            msg->color1 = pd->color1;
                            msg->color2 = pd->color2;
                            msg->color3 = pd->color3;
                        }
                    }
                }
            } else {
                ps->field_0x80 = 0.0f;
            }
        }
        if (Player[0] != NULL) {
            PodRaceSnipersUpdate();
        } else if (ps->finish_line != NULL && ps->halfway != NULL) {
            ps->field_0x88 += FRAMETIME;
            if (ps->flags & 1) {
                if (Player[0] != NULL && pod_old_pos[0].x != -1.0f && pod_old_pos[0].y != -1.0f &&
                    pod_old_pos[0].z != -1.0f) {
                    if (XZLinesIntersect(&ps->finish_line->pts[0], &ps->finish_line->pts[1],
                                         (NUVEC *)((u8 *)Player[0] + 0x5c), &pod_old_pos[0], NULL, NULL)) {
                        ps->ai_index++;
                        ps->ai_state++;
                        ps->flags &= ~1u;
                        ps->field_0x88 = 0.0f;
                        if (ps->ai_state == 3) {
                            ps->flags |= 8u;
                            StoreLevelProgress(world);
                            ResetLevel(world, "ep1_podsprint_sebulba", 1);
                        } else if (ps->ai_state > 3) {
                            if (FreePlay == 0) {
                                if (NewCutScene(NULL, WORLD->cutscene_sys, "Ep1_Podrace_Outro1", 1) == 0 &&
                                    NewCutScene(NULL, WORLD->cutscene_sys, "Ep1_Podsprint_Outro1", 1) == 0) {
                                    CompleteLevel(world);
                                }
                            } else {
                                CompleteLevel(world);
                            }
                        }
                    }
                }
                if (Player[1] != NULL && pod_old_pos[1].x != -1.0f && pod_old_pos[1].y != -1.0f &&
                    pod_old_pos[1].z != -1.0f) {
                    if (XZLinesIntersect(&ps->finish_line->pts[0], &ps->finish_line->pts[1],
                                         (NUVEC *)((u8 *)Player[1] + 0x5c), &pod_old_pos[1], NULL, NULL)) {
                        ps->ai_index++;
                        ps->ai_state++;
                        ps->flags &= ~1u;
                        ps->field_0x88 = 0.0f;
                        if (ps->ai_state == 3) {
                            ps->flags |= 8u;
                            StoreLevelProgress(world);
                            ResetLevel(world, "ep1_podsprint_sebulba", 1);
                        } else if (ps->ai_state > 3) {
                            if (FreePlay == 0) {
                                if (NewCutScene(NULL, WORLD->cutscene_sys, "Ep1_Podrace_Outro1", 1) == 0 &&
                                    NewCutScene(NULL, WORLD->cutscene_sys, "Ep1_Podsprint_Outro1", 1) == 0) {
                                    CompleteLevel(world);
                                }
                            } else {
                                CompleteLevel(world);
                            }
                        }
                    }
                }
            } else {
                if (Player[0] != NULL) {
                    if (XZLinesIntersect(&ps->halfway->pts[0], &ps->halfway->pts[1], (NUVEC *)((u8 *)Player[0] + 0x5c),
                                         &pod_old_pos[0], NULL, NULL)) {
                        ps->flags |= 1u;
                        ps->ai_index++;
                    }
                }
                if (Player[1] != NULL) {
                    if (XZLinesIntersect(&ps->halfway->pts[0], &ps->halfway->pts[1], (NUVEC *)((u8 *)Player[1] + 0x5c),
                                         &pod_old_pos[1], NULL, NULL)) {
                        ps->flags |= 1u;
                        ps->ai_index++;
                    }
                }
            }
            if (Player[0] != NULL) {
                pod_old_pos[0].x = *(float *)((u8 *)Player[0] + 0x5c);
                pod_old_pos[0].y = *(float *)((u8 *)Player[0] + 0x60);
                pod_old_pos[0].z = *(float *)((u8 *)Player[0] + 0x64);
            }
            if (Player[1] != NULL) {
                pod_old_pos[1].x = *(float *)((u8 *)Player[1] + 0x5c);
                pod_old_pos[1].y = *(float *)((u8 *)Player[1] + 0x60);
                pod_old_pos[1].z = *(float *)((u8 *)Player[1] + 0x64);
            }
            ps->lap_msg->value = (float)(i8)ps->ai_state;
            PodRaceSnipersUpdate();
        }
    }
    // Object pool loop: ease each active pod vehicle's boulder offset.
    i32 count = HIGHGAMEOBJECT;
    for (GameObject_s *obj = (GameObject_s *)Obj; count > 0; count--, obj = (GameObject_s *)((u8 *)obj + 0x10e4)) {
        if ((obj->apiobj.field_0x1f8 & 0x1001) == 0x1001 && (u8)obj->apiobj.field_0x27c == 0xff) {
            float seek_src = 0.0f;
            if (ps->boulders != NULL && WORLD->ai_sys != NULL && ps->ai_state > 1) {
                i32 slot = (u8)((size_t)ps->boulders * 0xeeeeeeef);
                u32 bit = 1u << (slot & 0x1f);
                if (((*(u32 *)((u8 *)obj + 0x2ac) & bit) | (*(u32 *)((u8 *)obj + 0x2a8) & bit)) != 0)
                    seek_src = boulder_offset_y;
            }
            *(float *)((u8 *)obj + 0xe94) = SeekValF(*(float *)((u8 *)obj + 0xe94), seek_src, boulder_offset_y_seek);
        }
    }
}

void PodSprintA_Panel(WORLDINFO_s *world) {
    PODSPRINT_s *ps = &podsprint;
    if (FadeSys.fade == 0.0f && MiniCutCam == 0 && CUTSTOPGAME == 0) {
        if (ps->speed > 0.0f) {
            if (Paused == 0 && podstartracealpha < 1.0f)
                podstartracealpha =
                    podstartracealpha + FRAMETIME * 2.0f < 1.0f ? podstartracealpha + FRAMETIME * 2.0f : 1.0f;
            podlapalpha = 0.0f;
        } else {
            float seek_arg = 1.0f;
            if (Paused == 0 && ps->field_0x88 <= 6.0f)
                seek_arg = 0.0f;
            podlapalpha = SeekLinearF(podlapalpha, seek_arg, FRAMETIME * 2.0f);
        }
    } else {
        podlapalpha = 0.0f;
        podhurryalpha = 0.0f;
        podstartracealpha = 0.0f;
    }
    if (Paused == 0 && podstartracealpha != 0.0f) {
        float v = ps->speed;
        if (v > 0.0f) {
            char buf[0x20];
            i32 n = (i32)v + 1;
            if (n > 3)
                n = 3;
            sprintf(buf, "%i", n);
            float m = NuFmod(v, 1.0f);
            if (m < 0.7f)
                m = 1.0f;
            else
                m = (m - 0.7f) / -0.1f + 1.0f;
            Text3DEx(buf, 0, 0.425f, 1.0f, m * 0.75f, m * 0.75f, m * 0.75f, 0, 0xff, 0, 0,
                     (u8)(i32)(128.0f * podstartracealpha));
        }
    }
    if (podlapalpha > 0.0f) {
        i32 idx = 0x135 + (i8)ps->ai_state;
        WORLDLEVOBJ_s *entry = &((WORLDLEVOBJ_s *)world->lev_objs)[idx];
        if (entry->enabled != 0) {
            float c = podlapalpha;
            float f = FadeSys.fade;
            DrawPanel3DObject(0.0f, f, 1.0f, 0.16f, 0.16f, 0.16f, (u16)0, (u16)0, (u16)0, (nuhspecial_s *)entry, 0, c);
        }
    }
}

void PodSprint_GetIAlongVals(nugspline_s *spline, i16 *out1, i16 *out2) {
    if (spline == NULL)
        return;
    PODSPRINT_s *ps = &podsprint;
    i32 idx = -1;
    if (spline == ps->ai[0].spline)
        idx = 0;
    else if (spline == ps->ai[1].spline)
        idx = 1;
    else if (spline == ps->ai[2].spline)
        idx = 2;
    else if (spline == ps->ai[3].spline)
        idx = 3;
    else if (spline == ps->ai[4].spline)
        idx = 4;
    else if (spline == ps->ai[5].spline)
        idx = 5;
    else
        return;
    i32 b = (i8)ps->ai_index;
    if (b != 0) {
        i32 a = b, esi;
        if (b <= 4)
            esi = b - 1;
        else {
            esi = 4;
            a = 5;
        }
        *out1 = ps->ai[idx].vals[esi];
        *out2 = ps->ai[idx].vals[a];
    } else {
        *out1 = 0;
        *out2 = ps->ai[idx].vals[0];
    }
}

// ===========================================================================
// Retake the palace (D/E/G)
// ===========================================================================

void RetakeD_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *g;
    g = GizmoBlowUp_FindByName(world, "lattice_a11");
    if (g != NULL) {
        g->field_0x128 = 0.5f;
        g->field_0x124 = 1;
    }
    g = GizmoBlowUp_FindByName(world, "lattice_b11");
    if (g != NULL) {
        g->field_0x128 = 0.5f;
        g->field_0x124 = 1;
    }
    g = GizmoBlowUp_FindByName(world, "lattice_c11");
    if (g != NULL) {
        g->field_0x128 = 0.5f;
        g->field_0x124 = 1;
    }
    g = GizmoBlowUp_FindByName(world, "lattice_d11");
    if (g != NULL) {
        g->field_0x128 = 0.5f;
        g->field_0x124 = 1;
    }
    g = GizmoBlowUp_FindByName(world, "lattice_e11");
    if (g != NULL) {
        g->field_0x128 = 0.5f;
        g->field_0x124 = 1;
    }
    g = GizmoBlowUp_FindByName(world, "lattice_f11");
    if (g != NULL) {
        g->field_0x128 = 0.5f;
        g->field_0x124 = 1;
    }
}

void RetakeE_Init(WORLDINFO_s *world) {
    GIZMOBLOWUP_s *g = GizmoBlowUp_FindByName(world, "box_deton_011");
    if (g != NULL)
        g->field_0xa0 |= 2;

    // The four obstacles are deliberately not uniform in the original:
    // obstacle3 shifts specials -0.75 on z, obstacle12 +0.75, and 11/13 reuse
    // the pos pointer left over from the previous loop iteration.
    GIZOBSTACLE_s *obs;
    GIZOBSTACLENODE_s *n;
    struct nuvec_s *pos;

    obs = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle3");
    if (obs != NULL) {
        n = (GIZOBSTACLENODE_s *)((GIZOBSTACLENODE_s *)obs->anim_set)->field_0x18;
        while (n != NULL) {
            pos = NuSpecialGetPos(n->special);
            pos->z -= 0.75f;
            GizObstacle_EvalAveragePosAndRadius(obs, 2);
            obs->field_0x18 = pos->z;
            obs->field_0x24 = pos->z;
            obs->field_0x3c = 15.0f;
            n = (GIZOBSTACLENODE_s *)n->next;
        }
    }

    obs = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle12");
    if (obs != NULL) {
        n = (GIZOBSTACLENODE_s *)((GIZOBSTACLENODE_s *)obs->anim_set)->field_0x18;
        while (n != NULL) {
            pos = NuSpecialGetPos(n->special);
            pos->z += 0.75f;
            GizObstacle_EvalAveragePosAndRadius(obs, 2);
            obs->field_0x18 = pos->z;
            obs->field_0x1c = pos->x;
            obs->field_0x20 = pos->y;
            obs->field_0x24 = pos->z;
            obs->field_0x3c = 15.0f;
            n = (GIZOBSTACLENODE_s *)n->next;
        }
    }

    obs = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle11");
    if (obs != NULL) {
        n = (GIZOBSTACLENODE_s *)((GIZOBSTACLENODE_s *)obs->anim_set)->field_0x18;
        while (n != NULL) {
            obs->field_0x1c = pos->x; // stale pos on purpose (matches original)
            obs->field_0x20 = pos->y;
            obs->field_0x24 = pos->z;
            obs->field_0x3c = 15.0f;
            n = (GIZOBSTACLENODE_s *)n->next;
        }
    }

    obs = GizObstacle_FindByName(world->giz_obstacle_sys, "obstacle13");
    if (obs != NULL) {
        n = (GIZOBSTACLENODE_s *)((GIZOBSTACLENODE_s *)obs->anim_set)->field_0x18;
        while (n != NULL) {
            obs->field_0x1c = pos->x; // stale pos on purpose (matches original)
            obs->field_0x20 = pos->y;
            obs->field_0x24 = pos->z;
            obs->field_0x3c = 15.0f;
            n = (GIZOBSTACLENODE_s *)n->next;
        }
    }
}

void RetakeG_Init(WORLDINFO_s *world) {
    char buf[0x10];
    retakeg_netpacket = (RETAKEGNETPACKET_s *)SetLevelHack(4);
    RetakeG_TotalGuards_msg = CheckGizAIMessage(gizaimessagesys, "TotalGuards", NULL);
    RetakeG_GuardsToRescue_msg = CheckGizAIMessage(gizaimessagesys, "GuardsToRescue", NULL);
    LevGizForce[0] = GizForce_FindByName(world->giz_force_sys, "force6");
    LevPathCnx[0] = AIPAthFindPathCnx(world->ai_sys, 0, "stack1_b", NULL, buf);
    LevPathCnx[1] = AIPAthFindPathCnx(world->ai_sys, 0, "stack1_a", NULL, buf);
    LevPathCnx[2] = AIPAthFindPathCnx(world->ai_sys, 0, "stack1_c", NULL, buf);
    LevPathCnx[3] = AIPAthFindPathCnx(world->ai_sys, 0, "stack1_d", NULL, buf);
    LevGizForce[1] = GizForce_FindByName(world->giz_force_sys, "force3");
    LevPathCnx[4] = AIPAthFindPathCnx(world->ai_sys, 0, "stack2_b", NULL, buf);
    LevPathCnx[5] = AIPAthFindPathCnx(world->ai_sys, 0, "stack2_a", NULL, buf);
    LevPathCnx[6] = AIPAthFindPathCnx(world->ai_sys, 0, "stack2_c", NULL, buf);
    LevPathCnx[7] = AIPAthFindPathCnx(world->ai_sys, 0, "stack2_d", NULL, buf);
    GIZFORCE_s *f = GizForce_FindByName(world->giz_force_sys, "Force18");
    if (f != NULL)
        f->strength_0x6c = 0.85f;
    f = GizForce_FindByName(world->giz_force_sys, "Force19");
    if (f != NULL)
        f->strength_0x6c = 0.85f;
    f = GizForce_FindByName(world->giz_force_sys, "Force20");
    if (f != NULL)
        f->strength_0x6c = 0.85f;
}

void RetakeG_Reset(WORLDINFO_s *) {
}

void RetakeG_Update(WORLDINFO_s *world) {
    (void)world;
    if (netclient != 0) {
        RetakeG_GuardsToRescue_msg->value = (float)retakeg_netpacket->guard_b;
        RetakeG_TotalGuards_msg->value = (float)retakeg_netpacket->guard_a;
    } else {
        retakeg_netpacket->guard_b = (i16)RetakeG_GuardsToRescue_msg->value;
        retakeg_netpacket->guard_a = (i16)RetakeG_TotalGuards_msg->value;
    }
    GIZFORCE_s *g0 = LevGizForce[0];
    GIZFORCE_s *g1 = LevGizForce[1];
    if (g0 != NULL && g0->group != NULL && g1 != NULL && g1->group != NULL) {
        for (i32 i = 0; i < 6; i++) {
            GIZFORCE_s *obj = (i < 3) ? g0 : g1;
            GIZFORCEOBJ_s *obj40 = (GIZFORCEOBJ_s *)obj->group;
            PATHCNXDATA_s *cnx = (PATHCNXDATA_s *)LevPathCnx[i];
            if (cnx != NULL) {
                if (obj40->flags & 4) {
                    cnx->flags0 &= 0x7fffffff;
                    cnx->flags4 &= 0x7fffffff;
                } else {
                    cnx->flags0 |= 0x80000000;
                    cnx->flags4 |= 0x80000000;
                }
            }
        }
    }
}

void RetakeG_Panel(WORLDINFO_s *world) {
    (void)world;
    char buf[0x10];
    i16 countbuf[6];
    for (i32 i = 0; i < 6; i++) {
        buf[i] = 1;
        countbuf[i] = id_ROYALGUARD;
    }
    if (RetakeG_TotalGuards_msg != NULL && RetakeG_GuardsToRescue_msg != NULL &&
        RetakeG_TotalGuards_msg->value > 0.0f && RetakeG_GuardsToRescue_msg->value > 0.0f) {
        i32 n = (i32)RetakeG_GuardsToRescue_msg->value;
        if (n > 6)
            n = 6;
        if (n > 0)
            memset(buf, 0, n);
    }
    i32 m = (i32)(RetakeG_TotalGuards_msg ? RetakeG_TotalGuards_msg->value : 0.0f);
    if (m > 6)
        m = 6;
    DrawMeleeTargets(countbuf, buf, NULL, m);
}

// ===========================================================================
// Maul boss (A/B/D/E/F)
// ===========================================================================

void MaulA_Init(WORLDINFO_s *world) {
    MaulA_ai_message = CheckGizAIMessage(gizaimessagesys, "MaulOnTheRun", NULL);
    MaulA_hits_message = CheckGizAIMessage(gizaimessagesys, "Hits", NULL);
    nuhspecial_s *slots = LevHSpecial;
    NuSpecialFind(world->current_gscn, &slots[4], "engine_1c", 1);
    NuSpecialFind(world->current_gscn, &slots[5], "engine_2c", 1);
    NuSpecialFind(world->current_gscn, &slots[6], "engine_1d", 1);
    NuSpecialFind(world->current_gscn, &slots[7], "engine_2d", 1);
}

void MaulA_Reset(WORLDINFO_s *world) {
    nuhspecial_s *slots = LevHSpecial;
    NuSpecialSetVisibility(&slots[4], 0);
    NuSpecialSetVisibility(&slots[5], 0);
    NuSpecialSetVisibility(&slots[6], 0);
    NuSpecialSetVisibility(&slots[7], 0);
    Maul_obj = FindGameObject(id_DARTHMAUL, 1, 1, 0, 0);
}

void MaulA_Update(WORLDINFO_s *) {
}

void MaulA_Panel(WORLDINFO_s *world) {
    if (netclient == 0) {
        if (Maul_obj != NULL && MaulA_ai_message != NULL && MaulA_ai_message->value == 0.0f &&
            MaulA_hits_message != NULL) {
            GameObject_s *maul = Maul_obj;
            u8 hp = (u8)(i32)MaulA_hits_message->value;
            maul->hitpoints = 3;
            maul->current_hp = hp;
            DrawBossHitPoints(maul);
        } else {
            DrawBossHitPoints(NULL);
        }
    }
}

void MaulB_Init(WORLDINFO_s *world) {
    GIZOBSTACLE_s *o = GizObstacle_FindByName(world->giz_obstacle_sys, "Obstacle19");
    if (o != NULL)
        o->field_a1_0xa1 |= 1;
}

void MaulD_Init(WORLDINFO_s *) {
}

void MaulD_Update(WORLDINFO_s *) {
}

void MaulE_Init(WORLDINFO_s *) {
}

void MaulE_Update(WORLDINFO_s *) {
}

void MaulF_Init(WORLDINFO_s *world) {
    MaulA_ai_message = CheckGizAIMessage(gizaimessagesys, "ShowHearts", NULL);
    nuhspecial_s *slots = LevHSpecial;
    NuSpecialFind(world->current_gscn, &slots[0], "throw_object1", 1);
    NuSpecialFind(world->current_gscn, &slots[1], "throw_object2", 1);
    NuSpecialFind(world->current_gscn, &slots[2], "throw_object3", 1);
}

void MaulF_Reset(WORLDINFO_s *world) {
    Maul_obj = FindGameObject(id_DARTHMAUL, 1, 1, 0, 0);
}

void MaulF_Update(WORLDINFO_s *) {
}

void MaulF_Panel(WORLDINFO_s *world) {
    if (netclient == 0) {
        if (Maul_obj != NULL && MaulA_ai_message != NULL && MaulA_ai_message->value == 1.0f) {
            DrawBossHitPoints(Maul_obj);
        } else {
            DrawBossHitPoints(NULL);
        }
    }
}

// ===========================================================================
// Anakin's flight (B)
// ===========================================================================

void AnakinsFlightB_Init(WORLDINFO_s *world) {
    trooper_boltid = BoltType_FindIDByName("trooper_red", world);
    trooper_side[0] = 0;
    trooper_side[1] = 0;
    trooper_side[2] = 0;
    nuhspecial_s *slots = LevHSpecial;
    i32 count = NuSpecialFind(world->current_gscn, &slots[0], "minifig_1_1", 1);
    count += NuSpecialFind(world->current_gscn, &slots[1], "minifig_1_2", 1);
    count += NuSpecialFind(world->current_gscn, &slots[2], "minifig_1_3", 1);
    if (count == 3)
        hothtroopers = (nuhspecial_s *)LevHSpecial;
}

void AnakinsFlightB_Update(WORLDINFO_s *) {
}

void AnakinsFlightB_Draw(WORLDINFO_s *world) {
    if (TimingBarSet == 5) {
        TBOPENFN("mini", 5);
        if (TimingBarSet == 5) {
            TBCLOSEFN("mini", 5);
        }
    }
}

// ===========================================================================
// Helpers defined in other original translation units but kept here until
// they get their own home (their originals live outside the ep1 TU).
// ===========================================================================

// Original TU near 0x153a40.
i32 PodLevel(AREADATA_s *area) {
    return PODRACE_ADATA != NULL && PODRACE_ADATA == area;
}

// Original TU near 0x175830.
void SetPodMergeAnims(ANIMPACKET_s *packet, i32 index) {
    ANIMPACKET_s *a = packet;
    i16 frame = (pod_roll[index] < 0.0f) ? 0x26 : 0x4f;
    a->field_0x3a = 1;
    a->frame = frame;
    float m = pod_animtime[index];
    a->time = m;
    a->time_secondary = m;
    a->field_0x44 = fabsf(pod_roll[index]);
}
