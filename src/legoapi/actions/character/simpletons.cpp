#include "legoapi/world/world.h"
#include "legoapi/props/system/socksys.h"
#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" {
    extern i16 id_PENGUIN_GOON, id_PENGUIN_GOON_GUN;
    extern i16 id_JOKER_GOON, id_JOKER_GOON_GUN;
    extern i16 id_RIDDLER_GOON, id_RIDDLER_GOON_GUN;
}

i16 GetGenericGoon(i32 armed) {
    if (armed) {
        if (id_PENGUIN_GOON_GUN != -1 && APICharacterLoaded(id_PENGUIN_GOON_GUN))
            return id_PENGUIN_GOON_GUN;
        if (id_JOKER_GOON_GUN != -1 && APICharacterLoaded(id_JOKER_GOON_GUN))
            return id_JOKER_GOON_GUN;
        if (id_RIDDLER_GOON_GUN != -1 && APICharacterLoaded(id_RIDDLER_GOON_GUN))
            return id_RIDDLER_GOON_GUN;
    } else {
        if (id_PENGUIN_GOON != -1 && APICharacterLoaded(id_PENGUIN_GOON))
            return id_PENGUIN_GOON;
        if (id_JOKER_GOON != -1 && APICharacterLoaded(id_JOKER_GOON))
            return id_JOKER_GOON;
        if (id_RIDDLER_GOON != -1 && APICharacterLoaded(id_RIDDLER_GOON))
            return id_RIDDLER_GOON;
    }
    return -1;
}

static NARROWSOCKEXCEPTION *NarrowSockException;

static inline i32 NarrowSockPositionAllowed(GameObject_s *object, SOCKSYS *system, i32 level_index) {
    if (system == NULL || object->sock_position.location.sock == -1)
        return 0;
    if ((system->sock[object->sock_position.location.sock].flags & 0x800) == 0)
        return 0;
    if (NarrowSockException != NULL) {
        for (NARROWSOCKEXCEPTION *exception = NarrowSockException; exception->level_name != NULL; ++exception) {
            if (exception->level_index == -1 || exception->level_index != level_index ||
                exception->sock_index != object->sock_position.location.sock)
                continue;
            if (object->sock_position.distance >= exception->start_distance &&
                object->sock_position.distance <= exception->end_distance)
                return 0;
        }
    }
    return 1;
}

i32 ObjInNarrowSock(GameObject_s *object, SOCKSYS *system, i32 level_index) {
    if ((WORLD->current_level->flags & LEVEL_NARROW_SOCKS) == 0 && !VehicleArea)
        return 0;
    if (disable_narrow_socks || object->apiobj.field_0x27c == -1)
        return 0;
    if ((object->apiobj.flags_low & 0x80) != 0) {
        GameObject_s *other = NULL;
        if (object == Player[0])
            other = Player[1];
        else if (object == Player[1])
            other = Player[0];
        if (other != NULL && (other->apiobj.flags_low & 0x80) != 0 &&
            !NarrowSockPositionAllowed(other, system, level_index))
            return 0;
    }
    return NarrowSockPositionAllowed(object, system, level_index);
}

i32 objInNetWaitContext(GameObject_s *object, i32 context) {
    if (LEGOCONTEXT_NETWAIT != -1 && object != NULL && LEGOCONTEXT_NETWAIT == object->character_context) {
        return object->context_animation == context;
    }
    return false;
}

namespace {
    struct AtOnceAttacker {
        GameObject_s *object;
        f32 distance;
    };

    AtOnceAttacker AtOnce_attackingPlayer[8][17];
    i32 AtOnce_attackersPerRow = 32;
    f32 AtOnce_InitialRowDist = 0.75f;
    f32 AtOnce_RowDist = 0.75f;
    i32 AtOnce_maxAttackers = 1;
} // namespace

bool oneAtOnce_CanAttack(GameObject_s *object, GameObject_s *opponent) {
    if (object == NULL) {
        return false;
    }
    if ((object->field_0xf01 & 0x20) == 0) {
        return true;
    }
    return object->one_at_once_player != 0xff && Player[object->one_at_once_player] == opponent;
}

f32 oneAtOnce_GetHoldRange(GameObject_s *object) {
    APIOBJECT *opponent = object->ai.opponent_object;
    const i32 player_index = opponent->field_0x27c;
    if (static_cast<u32>(player_index) > 8) {
        return 0.75f;
    }

    AtOnceAttacker *attackers = AtOnce_attackingPlayer[player_index];
    i32 row = 0;
    i32 unassigned_in_row = 0;
    i32 index = 0;
    GameObject_s *candidate = attackers[index].object;
    while (candidate != NULL) {
        if (index > 16)
            return AtOnce_InitialRowDist;
        if (candidate == object) {
            return static_cast<f32>(row) * AtOnce_RowDist + AtOnce_InitialRowDist;
        }
        unassigned_in_row += candidate->one_at_once_player == 0xff;
        if (unassigned_in_row >= AtOnce_attackersPerRow) {
            ++row;
            unassigned_in_row = 0;
        }
        ++index;
        candidate = attackers[index].object;
    }
    return AtOnce_InitialRowDist;
}

void oneAtOnce_MaintainArray() {
    GameObject_s *previous_attackers[8][4] = {};
    i32 previous_count[8] = {};
    i32 attacker_count[8] = {};

    for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
        GameObject_s *object = &Obj[index];
        const u16 character_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER;
        if (object == NULL || (object->apiobj.field_0x1f8 & character_flags) != character_flags ||
            object->ai.opponent == NULL || (object->field_0xf01 & 0x20) == 0) {
            continue;
        }

        const u8 old_player = object->one_at_once_player;
        if (old_player != 0xff) {
            previous_attackers[old_player][previous_count[old_player]++] = object;
        }
        object->one_at_once_player = 0xff;

        APIOBJECT *opponent_api = static_cast<APIOBJECT *>(object->ai.opponent);
        GameObject_s *opponent = opponent_api != NULL ? opponent_api->objptr : NULL;
        i32 player_index = -1;
        for (i32 player = 0; player < 8; ++player) {
            if (Player[player] == opponent) {
                player_index = player;
                break;
            }
        }
        if (player_index < 0) {
            continue;
        }

        const i32 slot = attacker_count[player_index]++;
        AtOnce_attackingPlayer[player_index][slot].object = object;
        AtOnce_attackingPlayer[player_index][slot].distance =
            NuVecDistSqr(&opponent->apiobj.collision_position, &object->apiobj.collision_position, NULL);
    }

    for (i32 player = 0; player < 8; ++player) {
        const i32 count = attacker_count[player];
        AtOnce_attackingPlayer[player][count].object = NULL;
        AtOnce_attackingPlayer[player][count].distance = 1.0e9f;
    }

    for (i32 player = 0; player < 8; ++player) {
        const i32 count = attacker_count[player];
        for (i32 slot = 0; slot < count; ++slot) {
            for (i32 previous = 0; previous < previous_count[player]; ++previous) {
                if (AtOnce_attackingPlayer[player][slot].object == previous_attackers[player][previous]) {
                    AtOnce_attackingPlayer[player][slot].distance *= 0.75f;
                    break;
                }
            }
        }

        for (i32 remaining = count; remaining > 1; --remaining) {
            for (i32 slot = 1; slot < remaining; ++slot) {
                if (AtOnce_attackingPlayer[player][slot].distance < AtOnce_attackingPlayer[player][slot - 1].distance &&
                    AtOnce_attackingPlayer[player][slot].distance !=
                        AtOnce_attackingPlayer[player][slot - 1].distance) {
                    GameObject_s *object = AtOnce_attackingPlayer[player][slot - 1].object;
                    const f32 distance = AtOnce_attackingPlayer[player][slot - 1].distance;
                    AtOnce_attackingPlayer[player][slot - 1] = AtOnce_attackingPlayer[player][slot];
                    AtOnce_attackingPlayer[player][slot].object = object;
                    AtOnce_attackingPlayer[player][slot].distance = distance;
                }
            }
        }

        if (Player[player] != NULL) {
            const i32 attack_limit = AtOnce_maxAttackers < count ? AtOnce_maxAttackers : count;
            for (i32 slot = 0; slot < attack_limit; ++slot) {
                AtOnce_attackingPlayer[player][slot].object->one_at_once_player = static_cast<u8>(player);
            }
        }
    }
}

void oneAtOnce_SetDistPerRow(float distance) {
    AtOnce_RowDist = MAX(0.0f, distance);
}

void NarrowSockExceptions_Init(NARROWSOCKEXCEPTION *exceptions) {
    i32 level_index __attribute__((aligned(16)));
    if (exceptions == NULL)
        return;
    NarrowSockException = exceptions;
    for (; exceptions->level_name != NULL; ++exceptions) {
        Level_FindByName(exceptions->level_name, &level_index);
        exceptions->level_index = level_index;
    }
}

void oneAtOnce_SetNumAttackers(i32 attackers) {
    AtOnce_maxAttackers = MAX(0, MIN(attackers, 4));
}

void oneAtOnce_SetInitDistPerRow(float distance) {
    AtOnce_InitialRowDist = MAX(0.0f, distance);
}

void oneAtOnce_SetAttackersPerRow(i32 attackers) {
    AtOnce_attackersPerRow = MAX(0, attackers);
}
