#include <string.h>
#include "decomp.h"
#include "nu2api/nu3d/nuspecial.h"
extern "C" float FRAMETIME;
#include "nu2api/numath/nutrig.h"
extern "C" {
    extern i16 id_GRABMAGNET;
    struct nuvec_s;
    void PlaySfx(const char *, nuvec_s *);
}
#include "legoapi/legoapi_types.h"
#include "legoapi/world/level.h"
#include "legoapi/characters/motion.h"
#include "nu2api/numath/numtx.h"

// Original 0x22aae0, 264 bytes.
NUVEC *Grabber_GetGrabPos(GRABBER_s *grabber, numtx_s *matrix) {
    if (grabber->character_model != NULL && (grabber->flags_559 & 1) != 0 &&
        grabber->character_model->points_of_interest[0] != NULL) {
        if (matrix != NULL)
            *matrix = grabber->grab_matrix;
        return reinterpret_cast<NUVEC *>(&grabber->grab_matrix.m30);
    }
    if (matrix != NULL)
        *matrix = numtx_identity;
    return &grabber->grab_position;
}

void Grabber_StoreProgress(WORLDINFO_s *world, LEVEL_PROGRESS_s *progress) {
    if (progress != NULL && world != NULL) {
        GRABBER_s *grabber = world->grabber;
        if (grabber != NULL) {
            progress->grabber_field_0x48c = grabber->field_0x48c;
            progress->grabber_field_0x484 = grabber->field_0x484;
            progress->grabber_field_0x494 = grabber->field_0x494;
        } else {
            progress->grabber_field_0x484 = 2000000.0f;
        }
    }
}

// Static grabber victim-pos helper. Moved from gizmisc_stubs.cpp.

static __used__ void Grabber_SetVictimPos(GRABBER_s *grabber) {
    if (grabber->victim != NULL) {
        NUVEC position = *Grabber_GetGrabPos(grabber, NULL);
        GameObject_s *victim = grabber->victim;
        if (grabber->character_id == id_GRABMAGNET) {
            f32 fraction = victim->context_animation_timer / victim->airborne_action_duration;
            if (fraction > 1.0f)
                fraction = 1.0f;
            NUVEC start = victim->external_force;
            victim->apiobj.position.x = start.x + (position.x - start.x) * fraction;
            victim->apiobj.position.y = start.y + (position.y - start.y) * fraction;
            victim->apiobj.position.z = start.z + (position.z - start.z) * fraction;
            victim->apiobj.position.x -= NU_SIN_LUT(victim->apiobj.field_0x276) * victim->apiobj.field_0x1e0 * fraction;
            victim->apiobj.position.z -=
                NU_SIN_LUT(victim->apiobj.field_0x276 + 0x4000) * victim->apiobj.field_0x1e0 * fraction;
            if (victim->field_0x7a3 == 0 && fraction == 1.0f) {
                PlaySfx("imp_C3PO_magnet", &position);
                grabber->victim->field_0x7a3 = 1;
                victim = grabber->victim;
            }
        } else if (victim->context_animation == 5) {
            victim->apiobj.position.x =
                position.x - NU_SIN_LUT(victim->apiobj.field_0x276) * victim->apiobj.field_0x1e0;
            victim->apiobj.position.y = position.y;
            victim->apiobj.position.z =
                position.z - NU_SIN_LUT(victim->apiobj.field_0x276 + 0x4000) * victim->apiobj.field_0x1e0;
        } else {
            victim->apiobj.position = position;
        }
        victim->field_0xeff &= ~2;
        grabber->victim->saved_position = grabber->victim->apiobj.position;
    }
}

extern ADDPART_s Default_ADDPART;
extern "C" PART_s *AddPart(ADDPART_s *);
void PartKill_Grabber(PART_s *, i32);
static __used__ void Grabber_Drop(GRABBER_s *grabber, NUVEC *previous_position) {
    grabber->flags_559 |= 2;
    if (grabber->victim != NULL) {
        if (grabber->character_id == id_GRABMAGNET && grabber->victim->field_0x7a3 != 0)
            PlaySfx("imp_C3PO_magnet_drop", Grabber_GetGrabPos(grabber, NULL));
        Grabber_SetVictimPos(grabber);
        GameObject_s *victim = grabber->victim;
        victim->apiobj.velocity.x = ((grabber->grab_position.x - previous_position->x) / FRAMETIME) * 0.5f;
        if (grabber->character_id == id_GRABMAGNET)
            victim->apiobj.velocity.y = -1.0f;
        victim->apiobj.velocity.z = ((grabber->grab_position.z - previous_position->z) / FRAMETIME) * 0.5f;
        victim->field_0x7a5 = 0xff;
        grabber->victim = NULL;
    } else if (grabber->carried_blowup != NULL) {
        ADDPART_s params = Default_ADDPART;
        NUMTX matrix = grabber->carried_matrix;
        NUVEC velocity;
        NUVEC position = *Grabber_GetGrabPos(grabber, NULL);
        matrix.m30 = position.x;
        matrix.m31 = position.y + grabber->carried_height_offset;
        matrix.m32 = position.z;
        velocity.x = ((grabber->grab_position.x - previous_position->x) / FRAMETIME) * 0.5f;
        velocity.y = -0.1f;
        velocity.z = ((grabber->grab_position.z - previous_position->z) / FRAMETIME) * 0.5f;
        params.matrix = &matrix;
        params.velocity = &velocity;
        NUVEC center;
        NuSpecialGetRadius(&grabber->carried_blowup->type->special, &center, &params.field_14);
        params.flags = 0x311;
        params.field_18 = params.field_14;
        params.special = &grabber->carried_blowup->type->special;
        params.gravity = -6.0f;
        params.time_step = FRAMETIME;
        params.field_44 = PartKill_Grabber;
        params.field_a4 = 3.0f;
        PART_s *part = AddPart(&params);
        if (part != NULL)
            part->grabber_blowup = grabber->carried_blowup;
        grabber->carried_blowup = NULL;
    }
}

#include "gameapi/ai/aisys/aisys.h"
#include "legoapi/characters/motion/gameanim.h"
extern "C" {
    extern i16 id_GRABCONTROL, id_GRABR2CONTROL, id_GRABMACHINE;
    extern f32 GameTimer;
    extern TERRAIN_SURFACE_s TerSurface[32];
    void NewTerrPlatformsOff();
    i32 ShadowInfo();
    void PlaySfxAndSetPitch(const char *, NUVEC *, f32);
    f32 AnimDuration(i32, i32, i32, i32, i32);
    f32 AnimListFrame(CHARACTERMODEL_s *, i32, i32);
    AIANTINODE_s *AIAntinodeCreateSingleFrame(NUVEC *, f32);
}
extern GRABBER_s *Grab_grabber;
extern WORLDINFO_s *WORLD;
extern LEVELDATA *DEATHSTARESCAPEB_LDATA, *JABBASPALACEB_LDATA, *CLOUDCITYTRAPA_LDATA;
extern NUVEC ShadNorm;
extern GameObject_s *Obj;
extern i32 HIGHGAMEOBJECT;
GameObject_s *FindGameObject(i32, u32, i32, i32, i32);
GameObject_s *FindNearestGameObject(NUVEC *, GameObject_s *, u32, f32, f32, i32, i32, i32, f32 *, i32,
                                    i32 (*)(GameObject_s *), bool);
f32 SeekValF(f32, f32, f32);
u16 SeekRot(u16, u16, f32);
u16 GamePad_InputAngle(GameObject_s *, GAMEPAD_s *);
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
void FindAnglesZX(NUVEC *, u16 *, u16 *);
i32 Cheat_PowerUpActive(i32);
i32 qrand();
void ConstantRumble(GameObject_s *, f32, f32);
void NewRumbleAllPlayers(f32, f32, i32, i32);
void Hint_SetComplete(i32);
i32 GameAnimSet_IsAnimationReset(GAMEANIMSET_s *);
GIZMOPICKUP_s *GizmoPickup_InBox(WORLDINFO_s *, i32, NUVEC *, NUVEC *);
void Pup_CollectCoin(WORLDINFO_s *, GIZMOPICKUP_s *, i32, GameObject_s *, i32);
void CollectMinikit(NUVEC *, char *, i32);
GIZMOBLOWUP_s *FindNearestGizmoBlowUp(WORLDINFO_s *, NUVEC *, f32);
i32 GizmoBlowupBlowup(GIZMOBLOWUP_s *, i32, i32, i32, GameObject_s *, i32);
static __used__ i32 IsGrabbable(GameObject_s *object) {
    return (object->apiobj.character_data->game_character->flags_090 & GAMECHARACTER_FLAG_GRAB_DISABLED) == 0;
}
void Grabber_Update(WORLDINFO_s *world) {
    GRABBER_s *g = world->grabber;
    if (g == NULL)
        return;
    if (g->victim != NULL &&
        ((g->victim->apiobj.object_flags & 0x1001) != 0x1001 || g->victim->apiobj.field_0x287 != 0))
        g->victim = NULL;
    if (g->platform_contact_timer > 0.0f)
        g->platform_contact_timer -= FRAMETIME;
    g->target_velocity.z = g->target_velocity.y = g->target_velocity.x = 0.0f;
    NUVEC next = g->grab_position, previous = g->grab_position;
    GameObject_s *control = FindGameObject(id_GRABCONTROL, 0, 0, 1, 0);
    GAMEPAD_s *pad = NULL;
    if (control == NULL) {
        if (id_GRABR2CONTROL != -1)
            FindGameObject(id_GRABR2CONTROL, 0, 0, 1, 0);
    } else
        pad = control->pad_gamepad;
    if (control != NULL && control->field_0xcc0 != NULL && (control->apiobj.object_flags & 0x80)) {
        if (pad->input_magnitude > 0.0f) {
            if (Grab_grabber->move_xy) {
                g->target_velocity.y = pad->input_direction_x * g->speed;
                g->target_velocity.x = 0.0f;
                g->target_velocity.z =
                    Grab_grabber->invert_x ? -(g->speed * pad->input_direction_z) : g->speed * pad->input_direction_z;
            } else {
                u16 angle = GamePad_InputAngle(control, pad);
                g->target_velocity.x = NU_SIN_LUT(angle) * g->speed;
                if (Grab_grabber->invert_x)
                    g->target_velocity.x = -g->target_velocity.x;
                g->target_velocity.z = g->speed * NU_SIN_LUT(angle + 0x4000);
            }
        }
    } else {
        bool minus = g->direction_switches[0] && (g->direction_switches[0]->progress_flags & 2) &&
                     g->direction_switches[0]->anim_set->state == 2;
        bool plus = g->direction_switches[1] && (g->direction_switches[1]->progress_flags & 2) &&
                    g->direction_switches[1]->anim_set->state == 2;
        if (minus && !plus)
            g->target_velocity.x = -g->speed;
        else if (plus && !minus)
            g->target_velocity.x = g->speed;
        plus = g->direction_switches[2] && (g->direction_switches[2]->progress_flags & 2) &&
               g->direction_switches[2]->anim_set->state == 2;
        minus = g->direction_switches[3] && (g->direction_switches[3]->progress_flags & 2) &&
                g->direction_switches[3]->anim_set->state == 2;
        if (plus && !minus)
            g->target_velocity.z = g->speed;
        else if (minus && !plus)
            g->target_velocity.z = -g->speed;
        if (!NuSpecialGetVisibilityFn(&g->special))
            g->target_velocity.z = g->target_velocity.x = 0.0f;
    }
    g->velocity.x = SeekValF(g->velocity.x, g->target_velocity.x, 5.0f);
    g->velocity.y = SeekValF(g->velocity.y, g->target_velocity.y, 5.0f);
    g->velocity.z = SeekValF(g->velocity.z, g->target_velocity.z, 5.0f);
    if ((g->flags_559 & 8) && (g->target_velocity.x != 0.0f || g->target_velocity.z != 0.0f))
        g->target_yaw = NuAtan2D(g->target_velocity.x, g->target_velocity.z);
    next.x += FRAMETIME * g->velocity.x;
    next.y += g->velocity.y * FRAMETIME;
    next.z += FRAMETIME * g->velocity.z;
    if (world->current_level == DEATHSTARESCAPEB_LDATA) {
        if (next.z >= 11.84f) {
            next.z = 11.84f;
            g->target_velocity.z = g->velocity.z = 0.0f;
        } else if (next.z < 10.125f) {
            next.z = 10.125f;
            g->target_velocity.z = g->velocity.z = 0.0f;
        }
        if (next.y < -4.85f) {
            next.y = -4.85f;
            g->target_velocity.y = g->velocity.y = 0.0f;
        } else if (next.y > -3.9f) {
            next.y = -3.9f;
            g->target_velocity.y = g->velocity.y = 0.0f;
        }
    } else if (world->current_level == JABBASPALACEB_LDATA) {
        f32 pitch = (NuVecMag(&g->velocity) / g->speed) * 0.25f + 0.5f;
        if (pitch > 1.0f)
            pitch = 1.0f;
        PlaySfxAndSetPitch("env_hover_box_lp", &next, pitch);
        next.y = g->initial_position.y + 0.0125f * NU_SIN_LUT((u16)(i32)((NuFmod(GameTimer, 2.0f) * 0.5f) * 65536.0f));
        g->target_velocity.y = g->velocity.y = 0.0f;
        if (g->platform_contact_timer > 0.0f)
            next.y -=
                ((1.0f - NU_SIN_LUT((i32)((g->platform_contact_timer * 4.0f) * 65536.0f + 16384.0f))) * 0.5f) * 0.02f;
    }
    i32 moved = 0;
    if (next.x != g->grab_position.x || next.y != g->grab_position.y || next.z != g->grab_position.z) {
        if (world->current_level != JABBASPALACEB_LDATA &&
            (g->target_velocity.x != 0.0f || g->target_velocity.y != 0.0f || g->target_velocity.z != 0.0f))
            PlaySfx(world->current_level == CLOUDCITYTRAPA_LDATA ? "CarbonFreezeCraneLp" : "env_crane_mvt_lp",
                    &g->grab_position);
        NUVEC query = next;
        query.y += 0.25f;
        if (WORLD->current_level != CLOUDCITYTRAPA_LDATA)
            NewTerrPlatformsOff();
        f32 height = GameShadow(NULL, &query, 5.0f, -1);
        i32 surface = -1;
        if (height != 2000000.0f)
            surface = ShadowInfo();
        if (surface >= 0 && surface <= 31 && (TerSurface[surface].flags & 0x200)) {
            g->shadow_height = height;
            FindAnglesZX(&ShadNorm, &g->shadow_z_angle, &g->shadow_x_angle);
            g->floor_height = (TerSurface[surface].flags & 2) ? height : 2000000.0f;
            g->grab_position = next;
            if ((g->target_velocity.x != 0.0f || g->target_velocity.y != 0.0f || g->target_velocity.z != 0.0f) &&
                !Cheat_PowerUpActive(-1)) {
                i32 random = qrand();
                f32 mag = NuVecMag(&g->velocity);
                ConstantRumble(NULL, ((f32)random * 0.000015259021893143654f * 0.4f) * (mag / g->speed), 0.15f);
            }
            moved = 1;
        } else {
            g->target_velocity = v000;
            g->velocity = v000;
            if (world->current_level == JABBASPALACEB_LDATA)
                g->grab_position.y = next.y;
        }
    }
    NUMTX matrix;
    if (g->flags_559 & 8)
        g->yaw = SeekRot(g->yaw, g->target_yaw, 3.0f);
    matrix = g->matrix;
    if (g->flags_559 & 8)
        NuMtxPreRotateY(&matrix, g->yaw);
    if (g->scale != 1.0f) {
        NUVEC scale = {g->scale, g->scale, g->scale};
        NuMtxPreScale(&matrix, &scale);
    }
    matrix.m30 = g->grab_position.x;
    matrix.m31 = g->grab_position.y;
    matrix.m32 = g->grab_position.z;
    NuSpecialSetDrawMtx(&g->special, &matrix);
    NuSpecialUpdate(&g->special);
    switch (g->state) {
        case 0:
            if (pad != NULL) {
                if (!(pad->buttons_pressed & (GAMEPAD_SPECIAL | GAMEPAD_ACTION)))
                    break;
            } else {
                if ((g->flags_559 & 4) && g->action_switch && (g->action_switch->progress_flags & 2) &&
                    GameAnimSet_IsAnimationReset(g->action_switch->anim_set))
                    g->flags_559 &= ~4;
                if (g->flags_559 & 4)
                    break;
                if (!g->action_switch || !(g->action_switch->progress_flags & 2) ||
                    (g->action_switch->anim_set->flags & 1))
                    break;
            }
            if (g->character_model && g->character_model->model_data_b[102]) {
                g->state = 1;
                g->animation_timer = AnimDuration(g->character_model->model_id, 102, 0, 0, 1);
                g->flags_559 &= ~2;
                if (!control)
                    g->flags_559 |= 4;
                NewRumbleAllPlayers(0, 0, 2, 0);
            } else if (g->character_id == id_GRABMAGNET) {
                g->victim = FindNearestGameObject(Grabber_GetGrabPos(g, NULL), NULL, 16, 0.5f, 0, -1, -1, -1, NULL, 1,
                                                  IsGrabbable, false);
                if (g->victim) {
                    GameObject_s *v = g->victim;
                    v->character_context = 47;
                    v->context_animation = v->apiobj.character_model->model_data_b[104] ? 104 : 5;
                    NewRumbleAllPlayers(0, 0.1f, 0, 0);
                    v->context_animation_timer = 0;
                    v->airborne_action_duration = 0.75f;
                    v->field_0x7a3 = 0;
                    v->external_force = v->apiobj.start_position;
                    Hint_SetComplete(651);
                    g->stuck_timer = 0;
                    g->state = 2;
                    if (!control)
                        g->flags_559 |= 4;
                }
            }
            PlaySfx("env_grabber_pickup", Grabber_GetGrabPos(g, NULL));
            break;
        case 1: {
            f32 *frame = AnimPlaying(&g->animation, 102, 1, 0);
            if (!frame)
                break;
            g->animation_timer -= FRAMETIME;
            if (g->animation_timer <= 0) {
                if (g->victim || g->carried_blowup) {
                    g->stuck_timer = 0;
                    g->state = 2;
                } else
                    g->state = 0;
                break;
            }
            if (g->flags_559 & 2)
                break;
            f32 trigger = AnimListFrame(g->character_model, 102, 0);
            if (!(trigger > 0) || !(*frame >= trigger))
                break;
            g->flags_559 |= 2;
            NUVEC *pos = Grabber_GetGrabPos(g, NULL);
            NUVEC low = {pos->x - 0.5f, pos->y - 0.5f, pos->z - 0.5f};
            NUVEC high = {pos->x + 0.5f, pos->y + 0.5f, pos->z + 0.5f};
            GIZMOPICKUP_s *pickup = GizmoPickup_InBox(world, 4, &low, &high);
            if (pickup) {
                pickup->state_flags = (pickup->state_flags & ~6) | 8;
                if (pickup->state_flags & 64)
                    Pup_CollectCoin(world, pickup, 2, control, 1);
                else
                    CollectMinikit(&pickup->position, "m_pup1", 1);
                NewRumbleAllPlayers(0, 0.1f, 0, 0);
                Hint_SetComplete(651);
                PlaySfx("env_grabber_down", pos);
            } else {
                g->victim = FindNearestGameObject(pos, NULL, 0, 0.5f, 0, -1, -1, -1, NULL, 0, IsGrabbable, false);
                if (g->victim) {
                    GameObject_s *v = g->victim;
                    v->character_context = 47;
                    v->context_animation = v->apiobj.character_model->model_data_b[104] ? 104 : 5;
                    NewRumbleAllPlayers(0, 0.1f, 0, 0);
                    v->context_animation_timer = 0;
                    v->airborne_action_duration = 0.75f;
                    v->field_0x7a3 = 0;
                    v->external_force = v->apiobj.start_position;
                    Hint_SetComplete(651);
                    PlaySfx("env_grabber_down", pos);
                } else {
                    g->carried_blowup = FindNearestGizmoBlowUp(world, pos, 0.25f);
                    if (g->carried_blowup) {
                        g->carried_matrix = g->carried_blowup->transform;
                        GizmoBlowupBlowup(g->carried_blowup, 0, -1, -1, NULL, 1);
                        g->carried_height_offset = g->carried_blowup->transform.m31 - g->carried_blowup->mid_position.y;
                        NewRumbleAllPlayers(0, 0.1f, 0, 0);
                        Hint_SetComplete(651);
                        PlaySfx("env_grabber_down", pos);
                    }
                }
            }
            PlaySfx("env_grabber_up", pos);
            break;
        }
        case 2:
            if (!moved && g->victim && (g->victim->apiobj.object_flags & 0x80)) {
                g->stuck_timer += FRAMETIME;
                if (g->stuck_timer >= 3.0f)
                    goto opening;
            } else
                g->stuck_timer = 0;
            if (!control) {
                if ((g->flags_559 & 4) && g->action_switch && (g->action_switch->progress_flags & 2) &&
                    GameAnimSet_IsAnimationReset(g->action_switch->anim_set))
                    g->flags_559 &= ~4;
            }
            if (!g->victim && !g->carried_blowup) {
                g->state = 0;
                break;
            }
            if (control) {
                if (!(pad->buttons_pressed & (GAMEPAD_SPECIAL | GAMEPAD_ACTION)))
                    break;
            } else if ((g->flags_559 & 4) || !g->action_switch || !(g->action_switch->progress_flags & 2) ||
                       (g->action_switch->anim_set->flags & 1))
                break;
        opening:
            if (g->character_model && g->character_model->model_data_b[103]) {
                g->state = 3;
                g->animation_timer = AnimDuration(g->character_model->model_id, 103, 0, 0, 1);
                g->flags_559 &= ~2;
                if (!control)
                    g->flags_559 |= 4;
                NewRumbleAllPlayers(0, 0, 2, 0);
            } else {
                Grabber_Drop(g, &previous);
                if (!control)
                    g->flags_559 |= 4;
            }
            PlaySfx("env_grabber_down", Grabber_GetGrabPos(g, NULL));
            break;
        case 3: {
            f32 *frame = AnimPlaying(&g->animation, 103, 1, 0);
            if (!frame)
                break;
            g->animation_timer -= FRAMETIME;
            if (g->animation_timer <= 0) {
                g->state = 0;
                if (!(g->flags_559 & 2))
                    Grabber_Drop(g, &previous);
                break;
            }
            if (g->flags_559 & 2)
                break;
            f32 trigger = AnimListFrame(g->character_model, 103, 0);
            if (trigger > 0 && *frame >= trigger)
                Grabber_Drop(g, &previous);
            break;
        }
    }
    if (g->character_model) {
        g->animation.previous_animation = g->animation.animation_index;
        g->animation.requested_animation = g->state == 1 ? 102 : g->state == 2 ? 15 : g->state == 3 ? 103 : 1;
        UpdateAnimPacket(g->character_model, &g->animation, FRAMETIME * 30.0f, g->speed, FRAMETIME, 0);
    }
    if (g->victim) {
        g->victim->context_animation_timer += FRAMETIME;
        Grabber_SetVictimPos(g);
        g->victim->apiobj.velocity.z = g->victim->apiobj.velocity.x = 0;
        g->victim->apiobj.velocity.y = -0.1f;
        g->victim->apiobj.field_0x27d = 0;
    }
    if (g->character_id == id_GRABMACHINE) {
        i32 mask = 0;
        bool found = false;
        for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
            GameObject_s *object = &Obj[i];
            if ((object->apiobj.object_flags & 0x1001) != 0x1001)
                continue;
            if (object->apiobj.character_data->game_character->flags_090 & 0x8000)
                found = true;
            else {
                u8 index = object->apiobj.field_0x289;
                mask |= (index & 32) ? 0 : (1u << (index & 31));
            }
        }
        if (found) {
            AIANTINODE_s *node = AIAntinodeCreateSingleFrame(&g->grab_position, 0.25f);
            if (node) {
                node->min_y = node->position.y - 4.0f;
                node->user_data[0] = (u32)mask;
                node->user_data[1] = (u32)(mask >> 31);
            }
        }
    }
}

#include "legoapi/gizmo/base/gizmo.h"
extern i32 obstacle_gizmotype_id;
extern "C" i32 FindPlatInst(i32);
void Grabber_Reset(WORLDINFO_s *world) {
    GRABBER_s *g = world->grabber;
    if (g == NULL)
        return;
    GIZMO *handle;
    handle = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, (char *)"pad_drop");
    Grab_grabber->action_switch = handle ? (GIZOBSTACLE_s *)handle->object : NULL;
    if (Grab_grabber->action_switch && !Grab_grabber->action_switch->anim_set)
        Grab_grabber->action_switch = NULL;
    handle = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, (char *)"pad_n");
    Grab_grabber->direction_switches[0] = handle ? (GIZOBSTACLE_s *)handle->object : NULL;
    if (Grab_grabber->direction_switches[0] && !Grab_grabber->direction_switches[0]->anim_set)
        Grab_grabber->direction_switches[0] = NULL;
    handle = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, (char *)"pad_s");
    Grab_grabber->direction_switches[1] = handle ? (GIZOBSTACLE_s *)handle->object : NULL;
    if (Grab_grabber->direction_switches[1] && !Grab_grabber->direction_switches[1]->anim_set)
        Grab_grabber->direction_switches[1] = NULL;
    handle = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, (char *)"pad_e");
    Grab_grabber->direction_switches[2] = handle ? (GIZOBSTACLE_s *)handle->object : NULL;
    if (Grab_grabber->direction_switches[2] && !Grab_grabber->direction_switches[2]->anim_set)
        Grab_grabber->direction_switches[2] = NULL;
    handle = GizmoFindByName(world->gizmo_sys, obstacle_gizmotype_id, (char *)"pad_w");
    Grab_grabber->direction_switches[3] = handle ? (GIZOBSTACLE_s *)handle->object : NULL;
    if (Grab_grabber->direction_switches[3] && !Grab_grabber->direction_switches[3]->anim_set)
        Grab_grabber->direction_switches[3] = NULL;
    g->grab_position.y = g->initial_position.y;
    LEVEL_PROGRESS_s *progress = world->level_progress;
    if (progress && progress->grabber_field_0x484 != 2000000.0f) {
        g->grab_position.x = progress->grabber_field_0x48c;
        g->grab_position.z = progress->grabber_field_0x494;
    } else {
        g->grab_position.x = g->initial_position.x;
        g->grab_position.z = g->initial_position.z;
    }
    g->shadow_height = 2000000.0f;
    g->floor_height = 2000000.0f;
    ResetAnimPacket(&g->animation, -1);
    g->state = 0;
    if (g->victim) {
        Grabber_SetVictimPos(g);
        g->victim->field_0x7a5 = 0xff;
    }
    g->victim = NULL;
    g->carried_blowup = NULL;
    NUVEC query = g->grab_position;
    query.y += 0.25f;
    if (WORLD->current_level != CLOUDCITYTRAPA_LDATA)
        NewTerrPlatformsOff();
    f32 height = GameShadow(NULL, &query, 5.0f, -1);
    if (height != 2000000.0f) {
        i32 surface = ShadowInfo();
        if ((u32)surface <= 31 && (TerSurface[surface].flags & 0x200)) {
            g->shadow_height = height;
            FindAnglesZX(&ShadNorm, &g->shadow_z_angle, &g->shadow_x_angle);
            g->floor_height = (TerSurface[surface].flags & 2) ? height : 2000000.0f;
        }
    }
    g->flags_559 &= 0xfa;
    g->target_yaw = 0;
    g->yaw = 0;
    g->velocity.x = 0;
    g->target_velocity.z = g->target_velocity.y = g->target_velocity.x = 0;
    g->velocity.y = g->velocity.z = 0;
    g->platform_contact_timer = 0;
    if (world->terrain)
        g->platform_id = FindPlatInst(NuSpecialGetInstanceix(&g->special));
}

#include "legoapi/render/fx.h"
extern GAMECAMERA_s *GameCam;
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void GameCam_NewShake(GAMECAMERA_s *, f32, f32, f32);
EXPLOSION *AddExplosion(NUVEC *, f32, f32, GameObject_s *, i32, i32);
void PartKill_Grabber(PART_s *part, i32) {
    GIZMOBLOWUP_s *blowup = part->grabber_blowup;
    NUVEC *position = &part->position;
    PlaySfx("Explode2", position);
    GameCam_Judder(GameCam, 0.5f, 0, position);
    if (blowup->type->particle_types[4] != -1)
        AddGameDebris(WORLD->debris_sys, blowup->type->particle_types[4], position);
    if (blowup->type->particle_types[5] != -1)
        AddGameDebris(WORLD->debris_sys, blowup->type->particle_types[5], position);
    if (blowup->type->particle_types[6] != -1)
        AddGameDebris(WORLD->debris_sys, blowup->type->particle_types[6], position);
    if (blowup->type->particle_types[0] != -1)
        AddFiniteShotPART(blowup->type->particle_types[0], position, 1);
    if (blowup->type->particle_types[1] != -1)
        AddFiniteShotPART(blowup->type->particle_types[1], position, 1);
    AddExplosion(position, 0.6f, 0.3f, NULL, -1, 0x27);
    GameCam_NewShake(GameCam, 0.5f, 0.5f, 1.0f);
    NewRumbleAllPlayers(0.7f, 0.0f, 0, 0);
}

#include "legoapi/characters/core/character.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nu3d/nurndr.h"
extern "C" {
    void rtlResetEx(rtldata_s *, i32);
    void rtlApplySetScale(void *, rtldata_s *, NUVEC *, NUMTX *, i32, f32);
    void rtlSetLights(rtldata_s *);
}
extern i32 CHARSHADOWS_ON, Paused, Reflections_On;
extern LEVELDATA *BLOCKADERUNNERC_LDATA;
extern MAKELAYERLISTFN MakeLayerList;
void EnableShadowMapRendering(i32);
void ResetShadowMapRendering();
i32 MatrixReflection(NUMTX *, i32, f32, f32, NUMTX *);
i32 MatrixReflectionVU0_AXISY(NUMTX *, f32, f32, NUMTX *);
void Grabber_Draw(WORLDINFO_s *world) {
    GRABBER_s *g = world->grabber;
    if (!g)
        return;
    NUVEC *grab_position = NULL;
    if (g->character_model) {
        grab_position = Grabber_GetGrabPos(g, NULL);
        rtldata_s lights;
        rtlResetEx(&lights, 1);
        rtlApplySetScale(WORLD->rtl_set, &lights, grab_position, NULL, -1, 1.0f);
        rtlSetLights(&lights);
    } else if (!NuSpecialGetVisibilityFn(&g->special))
        return;
    NUMTX matrix;
    memcpy(&matrix, NuSpecialGetDrawMtx(&g->special), sizeof(matrix));
    NUMTX reflection;
    if (world->current_level == BLOCKADERUNNERC_LDATA) {
        f32 distance = NuVecXZDist(&GameCam->pos, (NUVEC *)&matrix.m30, NULL);
        if (distance < 2.0f) {
            f32 fraction = 1.0f - distance * 0.5f;
            f32 wave = NU_SIN_LUT((i32)(fraction * 32768.0f + 16384.0f));
            matrix.m31 += (1.0f - (wave + 1.0f) * 0.5f) * 1.25f;
        }
    }
    if (g->shadow_height != 2000000.0f) {
        if ((g->flags_559 & 0x10) && CHARSHADOWS_ON) {
            NUVEC position;
            position.x = g->grab_position.x;
            position.y = g->shadow_height + 0.005f;
            if (WORLD->current_level == CLOUDCITYTRAPA_LDATA)
                position.y += 0.005f;
            position.z = g->grab_position.z;
            if (NuSpecialExistsFn(&g->shadow_special)) {
                f32 sx = NU_SIN_LUT(g->shadow_x_angle), cx = NU_SIN_LUT(g->shadow_x_angle + 0x4000);
                f32 sz = NU_SIN_LUT(g->shadow_z_angle), cz = NU_SIN_LUT(g->shadow_z_angle + 0x4000);
                f32 zero_s = sz * 0.0f, zero_c = 0.0f * cz;
                reflection.m00 = cx;
                reflection.m01 = sx * cz - zero_s;
                reflection.m02 = sx * sz + zero_c;
                reflection.m03 = 0;
                reflection.m10 = -sx;
                reflection.m11 = cx * cz - zero_s;
                reflection.m12 = cx * sz + zero_c;
                reflection.m13 = 0;
                reflection.m20 = 0;
                reflection.m21 = zero_c - sz;
                reflection.m22 = cz + zero_s;
                reflection.m23 = 0;
                reflection.m30 = 0;
                reflection.m31 = zero_c - zero_s;
                reflection.m32 = zero_c + zero_s;
                reflection.m33 = 1;
                NuMtxTranslate(&reflection, &position);
                NuSpecialDrawAt(&g->shadow_special, &reflection);
            } else
                NuRndrAddShadow(&position, g->radius, 128, g->shadow_z_angle, 0, g->shadow_x_angle);
        }
        if (g->floor_height != 2000000.0f)
            MatrixReflectionVU0_AXISY(&matrix, g->floor_height, WORLD->current_level->unknown_0cc, &reflection);
    }
    if (g->character_model) {
        g->flags_559 &= ~1;
        MakeLayerList = GCDataList[g->character_model->model_id].make_layer_list;
        if (g->flags_559 & 0x10) {
            g->character_model->hierarchy->suppress_shadow_surface_points = 0;
            EnableShadowMapRendering(0);
        } else {
            g->character_model->hierarchy->suppress_shadow_surface_points = 1;
            ResetShadowMapRendering();
        }
        NUMTX joints[256];
        if (APIDrawCharacterModel(g->character_model, &CDataList[g->character_model->model_id], &g->animation, &matrix,
                                  NULL, ((u8)Reflections_On && g->floor_height != 2000000.0f) ? &reflection : NULL,
                                  NULL, &g->grab_matrix, NULL, 0xffff, NULL, 0, (WORLDINFO_s *)(usize)Paused, FRAMETIME,
                                  joints, 0, WORLD->debris_sys))
            g->flags_559 |= 1;
        if (g->flags_559 & 0x10)
            ResetShadowMapRendering();
        g->character_model->hierarchy->suppress_shadow_surface_points = 0;
    } else if ((u8)Reflections_On && g->floor_height != 2000000.0f)
        NuSpecialDrawAt(&g->special, &reflection);
    if (g->carried_blowup && grab_position) {
        memcpy(&matrix, &g->carried_matrix, sizeof(matrix));
        matrix.m30 = grab_position->x;
        matrix.m31 = grab_position->y;
        matrix.m32 = grab_position->z;
        matrix.m31 += g->carried_height_offset;
        NuSpecialDrawAt(&g->carried_blowup->type->special, &matrix);
        if ((u8)Reflections_On && g->floor_height != 2000000.0f &&
            MatrixReflection(&matrix, 2, g->floor_height, WORLD->current_level->unknown_0cc, &reflection))
            NuSpecialDrawAt(&g->carried_blowup->type->special, &reflection);
    }
}
