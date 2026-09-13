#include "legoapi/legoapi_types.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/characters/motion.h"
#include "nu2api/numath/nutrig.h"
#include <string.h>

struct SPECIALMOVE_s {
    i16 attacker_animation;
    i16 attacker_id;
    i8 attacker_action_type;
    i8 victim_action_type;
    i16 victim_id;
    i16 victim_animation;
    u16 flags;
    f32 distance;
};
DECOMP_ASSERT(sizeof(SPECIALMOVE_s) == 0x10, "SPECIALMOVE size");
DECOMP_ASSERT(offsetof(SPECIALMOVE_s, victim_animation) == 8, "SPECIALMOVE victim animation offset");
DECOMP_ASSERT(offsetof(SPECIALMOVE_s, flags) == 10, "SPECIALMOVE flags offset");
static SPECIALMOVE_s *SpecialMove;
static i32 SpecialMoveCount;

i32 StartBackFlip(GameObject_s *object);

i32 SpecialMove_Check(GameObject_s *attacker, GameObject_s *victim) {
    if (SpecialMove != NULL && ((attacker->apiobj.flags_low & 0x80) == 0 || attacker->field_0xda8 <= 0.0f) &&
        LEGOCONTEXT_SPECIALMOVE_ATTACKER != -1 && LEGOCONTEXT_SPECIALMOVE_VICTIM != -1) {
        for (i32 i = 0; i < SpecialMoveCount; ++i) {
            SPECIALMOVE_s *move = &SpecialMove[i];
            GAMECHARACTERDATA *a = static_cast<GAMECHARACTERDATA *>(attacker->apiobj.character_data->field11_0x24);
            GAMECHARACTERDATA *v = static_cast<GAMECHARACTERDATA *>(victim->apiobj.character_data->field11_0x24);
            if ((move->attacker_id == attacker->id ||
                 (move->attacker_action_type != -1 && move->attacker_action_type == a->field275_0x116)) &&
                attacker->apiobj.character_model->model_data_b[move->attacker_animation] != NULL &&
                (move->victim_id == victim->id ||
                 (move->victim_action_type != -1 && move->victim_action_type == v->field275_0x116)) &&
                victim->apiobj.character_model->model_data_b[move->victim_animation] != NULL &&
                ((victim->apiobj.flags_low & 0x80) == 0 ||
                 (victim->spawn_protection_timer <= 0.0f && (victim->field_0xefe & 0x40) == 0)))
                return i;
        }
    }
    return -1;
}

void SpecialMove_Cancel(GameObject_s *) {
}

u32 SpecialMove_GetFlags(i32 index, u32 mask) {
    if (index == -1) {
        return 0;
    }
    if (mask != 0) {
        return SpecialMove[index].flags & mask;
    }
    return SpecialMove[index].flags;
}

void SpecialMove_VictimCode(GameObject_s *) {
}

void SpecialMoves_Configure(char *, variptr_u *, variptr_u *) {
}

void SpecialMove_ReleaseVictim(GameObject_s *) {
}

void SpecialMove_GetVictimAction(i32) {
}

// Original 0x497ee0, 43 bytes.
f32 SpecialMove_GetDistanceApart(i32 index) {
    return index == -1 ? 0.0f : SpecialMove[index].distance;
}

void SpecialMove_GetAttackerAction(i32) {
}

// Original 0x498410, 198 bytes.
void SpecialMove_Attacker_SetTargetMom(GameObject_s *object) {
    f32 distance = SpecialMove_GetDistanceApart(object->field_0x7a7);
    u16 angle = object->apiobj.movement_facing_angle;
    f32 z = object->force_target->apiobj.position.z - NU_COS_LUT(angle) * distance;
    f32 x = object->force_target->apiobj.position.x - NU_SIN_LUT(angle) * distance;
    object->target_velocity.z = (z - object->apiobj.position.z) * 10.0f;
    object->apiobj.velocity.z = object->target_velocity.z;
    object->target_velocity.x = (x - object->apiobj.position.x) * 10.0f;
    object->apiobj.velocity.x = object->target_velocity.x;
}

void BackFlipCode(GameObject_s *object) {
    if (LEGOCONTEXT_BACKFLIP == -1 || object->character_context != LEGOCONTEXT_BACKFLIP) {
        return;
    }

    if ((object->field_0xe22 & 0x10) != 0 && (object->pad_gamepad->buttons_held & GAMEPAD_JUMP) == 0) {
        object->field_0xe22 &= static_cast<u8>(~0x10u);
    }

    const i32 animation = object->context_animation;
    if (object->apiobj.character_model->model_data_b[animation] != NULL &&
        CurrentAnim(&object->apiobj.anim_packet) != animation) {
        return;
    }

    object->context_animation_timer -= FRAMETIME;
    if (object->context_animation_timer > 0.0f) {
        return;
    }

    object->character_context = -1;
    if ((object->pad_gamepad->buttons_held & GAMEPAD_JUMP) != 0 && (object->field_0xe22 & 0x10) == 0) {
        StartBackFlip(object);
    } else {
        object->apiobj.velocity.x = 0.0f;
        object->apiobj.velocity.z = 0.0f;
    }
}
