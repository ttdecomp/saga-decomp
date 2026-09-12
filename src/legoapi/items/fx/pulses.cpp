#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numusic/sfx.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void AddLevelSfxFromId(i32 sfx_id, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx);
void GameAudio_PlaySfxById(i32 sfx_id, NUVEC *position, i32 flags, i32 volume);
i32 ObjHitObj(GameObject_s *attacker, GameObject_s *target, i32 damage, u16 flags, i32 unknown, i32 context);
void TakeHitRumble(GameObject_s *object, f32 strength);
void KillRumble(GameObject_s *object);
extern "C" void AddFiniteShotDebrisEffect(i32 *handle, i32 effect, NUVEC *position, i32 count);

void Pulses_Reset(PULSESYS_s *pulse_sys) {
    PULSESYS_s *current_pulse_sys = pulse_sys;

    if (current_pulse_sys == NULL) {
        return;
    }
    for (i32 i = 0; i < current_pulse_sys->pulse_count; i++) {
        NuSpecialSetVisibility(&current_pulse_sys->pulses[i].special, 0);

        PULSE_s *pulse = &current_pulse_sys->pulses[i];
        pulse->disabled = 0;
        pulse->active = 0;
        pulse->timer = pulse->start_wait;
        if (pulse->gizmo_name[0] != '\0') {
            pulse->gizmo = GizmoFindByName(WorldInfo_CurrentlyActive()->gizmo_sys, -1, pulse->gizmo_name);
        }
    }
}

void Pulses_AddSfx(PULSESYS_s *pulse_sys, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx_count) {
    if (pulse_sys == NULL) {
        return;
    }

    if (pulse_sys->sfx_on_loop != -1) {
        AddLevelSfxFromId(pulse_sys->sfx_on_loop, sfx_ids, sfx_count, max_sfx_count);
    }
    if (pulse_sys->sfx_off_loop != -1) {
        AddLevelSfxFromId(pulse_sys->sfx_off_loop, sfx_ids, sfx_count, max_sfx_count);
    }
    if (pulse_sys->sfx_turn_on != -1) {
        AddLevelSfxFromId(pulse_sys->sfx_turn_on, sfx_ids, sfx_count, max_sfx_count);
    }
    if (pulse_sys->sfx_turn_off != -1) {
        AddLevelSfxFromId(pulse_sys->sfx_turn_off, sfx_ids, sfx_count, max_sfx_count);
    }
    if (pulse_sys->sfx_hit_player != -1) {
        AddLevelSfxFromId(pulse_sys->sfx_hit_player, sfx_ids, sfx_count, max_sfx_count);
    }
}

void Pulses_Update(PULSESYS_s *pulse_sys) {
    if (pulse_sys == NULL) {
        return;
    }

    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (pulse_sys->pulse_count == 0) {
        return;
    }

    NUVEC hit_direction = {pulse_sys->hit_direction_line, 0.0f, pulse_sys->hit_direction_radius_origin};
    NuVecRotateY(&hit_direction, &hit_direction, 0x4000);

    for (i32 i = 0; i < pulse_sys->pulse_count; ++i) {
        PULSE_s *pulse = &pulse_sys->pulses[i];
        if (pulse->gizmo != NULL && GizmoGetVisibility(world->gizmo_sys, pulse->gizmo) == 0) {
            pulse->disabled = 1;
            NuSpecialSetVisibility(&pulse->special, 0);
        }
        if (pulse->disabled != 0) {
            continue;
        }

        NUVEC *pulse_position = NuSpecialGetDrawPos(&pulse->special);
        pulse->timer -= FRAMETIME;
        if (pulse->timer <= 0.0f && netclient == 0) {
            if (pulse->active != 0) {
                pulse->active = 0;
                pulse->timer = pulse->off_time;
                GameAudio_PlaySfxById(pulse_sys->sfx_turn_off, pulse_position, 0, 0);
            } else {
                pulse->active = 1;
                pulse->timer = pulse->on_time;
                GameAudio_PlaySfxById(pulse_sys->sfx_turn_on, pulse_position, 0, 0);
            }

            NuSpecialSetVisibility(&pulse->special, pulse->active);
            if (pulse->active != 0) {
                GameAudio_PlaySfxById(pulse_sys->sfx_on_loop, pulse_position, 0, 0);
            } else {
                GameAudio_PlaySfxById(pulse_sys->sfx_off_loop, pulse_position, 0, 0);
            }
        }

        if (pulse->active == 0) {
            continue;
        }

        for (i32 player_index = 0; player_index < 2; ++player_index) {
            GameObject_s *player = Player[player_index];
            if (player == NULL || static_cast<i8>(player->apiobj.flags_low) >= 0 || player->apiobj.field_0x287 != 0 ||
                (LEGOCONTEXT_DOOMED != -1 && player->character_context == LEGOCONTEXT_DOOMED) ||
                player->flicker_timer > 0.0f || player->spawn_protection_timer > 0.0f ||
                (player->field_0xefe & 0x40) != 0) {
                continue;
            }

            NUVEC direction = hit_direction;
            if (pulse_sys->radial_hit_direction != 0) {
                direction.x = pulse_position->x - hit_direction.x;
                direction.y = 0.0f;
                direction.z = pulse_position->z - hit_direction.z;
                NuVecNorm(&direction, &direction);
            }

            NUVEC offset;
            NuVecSub(&offset, &player->apiobj.collision_position, pulse_position);
            f32 distance = direction.x * offset.x + direction.z * offset.z;
            if (pulse_sys->radial_hit_direction == 1) {
                if (distance < 0.0f) {
                    continue;
                }
                NuVecRotateY(&offset, &offset, 0x4000);
                distance = direction.x * offset.x + direction.z * offset.z;
            }

            if (NuFabs(distance) >= pulse_sys->collide_radius) {
                continue;
            }
            if (pulse_sys->radial_hit_direction == 0 &&
                offset.x * pulse_sys->hit_direction_line + offset.z * pulse_sys->hit_direction_radius_origin < 0.0f) {
                continue;
            }

            GameAudio_PlaySfxById(pulse_sys->sfx_hit_player, &player->apiobj.collision_position, 0, 0);
            if (ObjHitObj(NULL, player, 1, 0, 0, 1) == 2) {
                KillRumble(player);
                continue;
            }

            TakeHitRumble(player, 0.7f);
            player->apiobj.velocity.x = 0.0f;
            player->apiobj.velocity.z = 0.0f;
            player->flicker_flags = (player->flicker_flags & ~7) | (distance >= 0.0f ? 4 : 3);

            i32 debris_handle = -1;
            AddFiniteShotDebrisEffect(&debris_handle, world->debris_sys->entries[pulse_sys->debris_hit_player].effect,
                                      &player->apiobj.collision_position, 1);
        }
    }
}

void Pulses_Configure(WORLDINFO_s *world, char *config) {
    world->pulses_sys = NULL;
    if (world->current_gscn == NULL) {
        return;
    }

    NUFPAR *parser = NuFParCreateMem("pulses", config, 0xffff);
    if (parser == NULL) {
        return;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);

    PULSESYS_s pulse_sys = {};
    PULSE_s *pulse = reinterpret_cast<PULSE_s *>(world->giz_buffer.addr);
    pulse_sys.pulses = pulse;
    pulse_sys.sfx_turn_on = -1;
    pulse_sys.sfx_turn_off = -1;
    pulse_sys.sfx_on_loop = -1;
    pulse_sys.sfx_off_loop = -1;
    pulse_sys.sfx_hit_player = -1;
    pulse_sys.collide_radius = 1.0f;
    pulse_sys.debris_hit_player = -1;
    pulse_sys.radial_hit_direction = 1;

    while (NuFParGetLine(parser) != 0) {
        if (NuFParGetWord(parser) == 0 || NuStrICmp(parser->word_buf, "pulses_start") != 0) {
            continue;
        }

        while (NuFParGetLine(parser) != 0) {
            memset(pulse, 0, sizeof(*pulse));
            if (NuFParGetWord(parser) == 0) {
                continue;
            }

            if (NuStrICmp(parser->word_buf, "pulses_end") == 0) {
                break;
            }

            if (NuStrICmp(parser->word_buf, "sfx_onloop") == 0) {
                if (NuFParGetWord(parser) != 0) {
                    pulse_sys.sfx_on_loop = static_cast<i16>(GetSfxId(parser->word_buf));
                }
            } else if (NuStrICmp(parser->word_buf, "sfx_offloop") == 0) {
                if (NuFParGetWord(parser) != 0) {
                    pulse_sys.sfx_off_loop = static_cast<i16>(GetSfxId(parser->word_buf));
                }
            } else if (NuStrICmp(parser->word_buf, "sfx_turnon") == 0) {
                if (NuFParGetWord(parser) != 0) {
                    pulse_sys.sfx_turn_on = static_cast<i16>(GetSfxId(parser->word_buf));
                }
            } else if (NuStrICmp(parser->word_buf, "sfx_turnoff") == 0) {
                if (NuFParGetWord(parser) != 0) {
                    pulse_sys.sfx_turn_off = static_cast<i16>(GetSfxId(parser->word_buf));
                }
            } else if (NuStrICmp(parser->word_buf, "sfx_hitplayer") == 0) {
                if (NuFParGetWord(parser) != 0) {
                    pulse_sys.sfx_hit_player = static_cast<i16>(GetSfxId(parser->word_buf));
                }
            } else if (NuStrICmp(parser->word_buf, "collide_radius") == 0) {
                pulse_sys.collide_radius = NuFParGetFloat(parser);
            } else if (NuStrICmp(parser->word_buf, "hit_direction_line") == 0) {
                pulse_sys.hit_direction_line = NuFParGetFloat(parser);
                pulse_sys.hit_direction_radius_origin = NuFParGetFloat(parser);
                pulse_sys.radial_hit_direction = 0;
            } else if (NuStrICmp(parser->word_buf, "hit_direction_radius_origin") == 0) {
                pulse_sys.hit_direction_line = NuFParGetFloat(parser);
                pulse_sys.hit_direction_radius_origin = NuFParGetFloat(parser);
                pulse_sys.radial_hit_direction = 1;
            } else if (NuStrICmp(parser->word_buf, "debris_hitplayer") == 0) {
                if (NuFParGetWord(parser) != 0) {
                    pulse_sys.debris_hit_player = static_cast<i16>(FindGameDebris(world->debris_sys, parser->word_buf));
                }
            } else if (NuStrICmp(parser->word_buf, "pulse") == 0 && NuFParGetWord(parser) != 0 &&
                       NuSpecialFind(world->current_gscn, &pulse->special, parser->word_buf, 1) != 0) {
                while (NuFParGetWord(parser) != 0) {
                    if (NuStrICmp(parser->word_buf, "gizmo") == 0) {
                        if (NuFParGetWord(parser) != 0 && NuStrLen(parser->word_buf) < sizeof(pulse->gizmo_name)) {
                            NuStrCpy(pulse->gizmo_name, parser->word_buf);
                        }
                    } else if (NuStrICmp(parser->word_buf, "on_time") == 0) {
                        pulse->on_time = NuFParGetFloat(parser);
                    } else if (NuStrICmp(parser->word_buf, "off_time") == 0) {
                        pulse->off_time = NuFParGetFloat(parser);
                    } else if (NuStrICmp(parser->word_buf, "start_wait") == 0) {
                        pulse->start_wait = NuFParGetFloat(parser);
                    }
                }

                if (NuSpecialExistsFn(&pulse->special) != 0) {
                    if (pulse->on_time < 0.1f) {
                        pulse->on_time = 0.1f;
                    }
                    if (pulse->off_time < 0.1f) {
                        pulse->off_time = 0.1f;
                    }
                    if (pulse->start_wait < 0.0f) {
                        pulse->start_wait = 0.0f;
                    }
                    if (pulse_sys.collide_radius < 0.1f) {
                        pulse_sys.collide_radius = 0.1f;
                    }
                    ++pulse_sys.pulse_count;
                    ++pulse;
                }
            }
        }
        break;
    }

    NuFParDestroy(parser);
    if (pulse_sys.pulse_count == 0) {
        return;
    }

    world->giz_buffer.addr = reinterpret_cast<usize>(pulse);
    world->pulses_sys = reinterpret_cast<PULSESYS_s *>(world->giz_buffer.addr);
    *world->pulses_sys = pulse_sys;
    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr + sizeof(PULSESYS_s), 4);
}
