#include "legoapi/legoapi_types.h"
#include "globals.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/core/input/qrand.h"

#include "gameapi/ai/aisys/aisys.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/world/world.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/levels/levels.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "gameapi/gui/apimenu.h"

struct TAGTRANSFER_s {
    GameObject_s *source;
    NUVEC position[3];
    f32 height[3];
    f32 time;
};
DECOMP_ASSERT(sizeof(TAGTRANSFER_s) == 0x38, "tag transfer size");
static TAGTRANSFER_s Tag_Transfer[2];

static const f32 Tag_TransferResetTimer = 0.5f;

extern void (*Tag_DrawIconFn)(GameObject_s *);
i32 (*Tag_NoHiddenIconFn)(GameObject_s *) = NULL;
char *LEGOASCII_UP = NULL;
extern u8 PlayerRGB[2][3];
extern ADDGAMEMSG AddGameMsg_Default;
GAMEMESSAGE_s *AddGameMsg(ADDGAMEMSG *message);
extern i32 do_player_tag;
extern f32 player_tag_timer;
extern GameObject_s *player_tag_from;
extern GameObject_s *player_tag_to;

void ResetForceGlow(PLAYERPACKET_s *packet);
void AICreatureResumeScript(GameObject_s *object);
void GizForce_ResetLOS(GameObject_s *object);
void NewBuzzFrames(nupad_s *pad, i32 frames, i32 flags);
void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode);
void GameAudio_PlaySfx(i32 sfx, NUVEC *position, i32 flags, i32 volume);
void TakeOver2GetIn(GameObject_s *source, GameObject_s *target);
void TakeOverYoda(GameObject_s *source, GameObject_s *target, i32 mode, i32 blend);
extern i32 CUTSKIPLOCK;
extern i16 id_LUKESKYWALKERDAGOBAH;
extern "C" i32 menu_i_pack;
void NewRumble(nupad_s *, f32, i32);
void Hint_CancelCurrent();
void GameCam_HitRoll();
i32 NuIOS_AreInAppPurchasesAvailable();
i32 NuIOS_CanMakeInAppPurchases();

// The original keeps this search out of line and passes the object in EAX.

i32 TagCode(GameObject_s *, GameObject_s *, i32, i32, i32);

i32 TagCharacter(GameObject_s *source, GameObject_s *target, i32 mode) {
    const auto available = [source](GameObject_s *candidate) {
        return candidate != NULL && (candidate->apiobj.field_0x1f8 & 0x1001) == 0x1001 && candidate != source &&
               candidate->apiobj.field_0x287 == 0 && static_cast<i8>(candidate->apiobj.flags_low) >= 0 &&
               (candidate->tag_context_flags & 2) == 0;
    };
    if (!available(target)) {
        if (available(Player[0]))
            target = Player[0];
        else if (available(Player[1]))
            target = Player[1];
        else if (available(Player[2]))
            target = Player[2];
        else if (available(Player[3]))
            target = Player[3];
        else if (available(Player[4]))
            target = Player[4];
        else if (available(Player[5]))
            target = Player[5];
        else if (available(Player[6]))
            target = Player[6];
        else if (available(Player[7]))
            target = Player[7];
        else
            return 0;
    }
    const u8 target_active = target->apiobj.flags_low >> 7;
    const u8 source_active = source->apiobj.flags_low >> 7;
    const i32 result = TagCode(source, target, 0, 0, mode);
    if (result == 2) {
        const u8 flags = (source->tag_context_flags | 4) & ~8;
        source->tag_target = target;
        source->tag_context_timer = 3.0f;
        source->tag_context_flags = flags;
        return 0;
    }
    if (result == 0)
        return 0;
    target->apiobj.flags_low = (target->apiobj.flags_low & 0x7f) | (source_active << 7);
    source->apiobj.flags_low = (source->apiobj.flags_low & 0x7f) | (target_active << 7);
    return 1;
}

extern i16 id_TC14;
extern i16 id_GRABCONTROL;
extern i16 id_GRABR2CONTROL;
void Move_BEAST(GameObject_s *);
HINT_s *Hint_FindHint(i32);
i32 Hint_isComplete(HINT_s *);
void Hint_SetComplete(HINT_s *);

i32 Tag_UpdateHint(HINT_s *hint) {
    if (WORLD->area != NULL && WORLD->area == HUB_ADATA)
        return 0;
    u8 conditions = static_cast<u8>(LSW_HintConditions);
    i32 tc14 = (conditions >> 1) & 1;
    auto check_tc14 = [&](GameObject_s *object) {
        if (object != NULL && (object->apiobj.field_0x1f8 & 0x1080) == 0x1080 && object->id == id_TC14)
            tc14 = 1;
    };
    check_tc14(Player[0]);
    check_tc14(Player[1]);
    check_tc14(Player[2]);
    check_tc14(Player[3]);
    check_tc14(Player[4]);
    check_tc14(Player[5]);
    check_tc14(Player[6]);
    check_tc14(Player[7]);
    reinterpret_cast<u8 *>(&LSW_HintConditions)[0] = (conditions & ~2) | (tc14 << 1);
    switch (hint->control_mode_ids[0]) {
        case 600:
            if (FreePlay == 0)
                return 0;
            if (player != NULL && static_cast<i8>(player->apiobj.flags_low) < 0 && player->field_0xcc0 != NULL)
                return 0;
            return player2 == NULL || static_cast<i8>(player2->apiobj.flags_low) >= 0 || player2->field_0xcc0 == NULL;
        case 602: {
            if (Tag_DoneFirst != 0) {
                Hint_SetComplete(hint);
                return 0;
            }
            if (VehicleArea != 0 || WORLD->current_level == HUB_LDATA)
                return 0;
            GameObject_s *first = player;
            GameObject_s *second = player2;
            if (first != NULL && static_cast<i8>(first->apiobj.flags_low) < 0 && first->field_0xcc0 != NULL)
                return 0;
            if (second != NULL && static_cast<i8>(second->apiobj.flags_low) < 0 && second->field_0xcc0 != NULL)
                return 0;
            i32 count = 0;
            i32 active = 0;
            auto count_player = [&](GameObject_s *object) {
                if (object != NULL) {
                    ++count;
                    active += (object->apiobj.field_0x1f8 & 0x1080) == 0x1080;
                }
            };
            count_player(Player[0]);
            count_player(Player[1]);
            count_player(Player[2]);
            count_player(Player[3]);
            count_player(Player[4]);
            count_player(Player[5]);
            count_player(Player[6]);
            count_player(Player[7]);
            if (active == 2 && count == 2)
                return 0;
            return (first != NULL && first->field_0xcc0 == NULL) || (second != NULL && second->field_0xcc0 == NULL);
        }
        case 603: {
            if ((LSW_HintConditions & 4) == 0) {
                if (Tag_DoneFirst > 1)
                    Tag_DoneFirst = 1;
                return 0;
            }
            HINT_s *previous = Hint_FindHint(602);
            if (Tag_DoneFirst == 2) {
                Hint_SetComplete(hint);
                return 0;
            }
            if (Tag_DoneFirst != 1 || (previous != NULL && Hint_isComplete(previous) == 0))
                return 0;
            if (VehicleArea != 0 || WORLD->current_level == HUB_LDATA)
                return 0;
            return (player != NULL && player->field_0xcc0 == NULL) || (player2 != NULL && player2->field_0xcc0 == NULL);
        }
        case 605:
        case 611:
        case 650:
            for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
                GameObject_s *object = &Obj[i];
                if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
                    static_cast<i8>(object->field_0xe23) >= 0)
                    continue;
                const bool grab = object->id == id_GRABCONTROL || object->id == id_GRABR2CONTROL;
                if (hint->control_mode_ids[0] == 650) {
                    if (grab)
                        return 1;
                } else if (!grab) {
                    const bool beast = object->apiobj.character_data->move_fn == Move_BEAST;
                    if (beast == (hint->control_mode_ids[0] == 611))
                        return 1;
                }
            }
            return 0;
        case 606:
            for (i32 i = 0; i < 2; ++i) {
                GameObject_s *object = Player[i];
                if (object != NULL && static_cast<i8>(object->apiobj.flags_low) < 0 && object->field_0xcc0 != NULL &&
                    object->id != id_LUKESKYWALKERDAGOBAH)
                    return WORLD->current_level != SPEEDERCHASEA_LDATA || disable_narrow_socks != 0;
            }
            return 0;
        case 647:
            return WORLD->current_level == SPEEDERCHASEA_LDATA &&
                   ((Player[0] != NULL && Player[0]->id == id_SPEEDERBIKE) ||
                    (Player[1] != NULL && Player[1]->id == id_SPEEDERBIKE));
        case 649:
            if (VehicleArea == 0)
                return 0;
            for (i32 i = 0; i < 2; ++i) {
                if (Player[i] != NULL && Player[i]->torpedo != NULL && Player[i]->torpedo->count != 0 &&
                    Player[i]->torpedo->target != 0)
                    return 1;
            }
            return 0;
        case 651:
            for (i32 i = 0; i < 2; ++i) {
                if (Player[i] != NULL && static_cast<i8>(Player[i]->apiobj.flags_low) < 0 &&
                    (Player[i]->id == id_GRABCONTROL || Player[i]->id == id_GRABR2CONTROL))
                    return 1;
            }
            return 0;
        case 654:
            if (VehicleArea == 0)
                return 0;
            for (i32 i = 0; i < 2; ++i) {
                if (Player[i] != NULL && static_cast<i8>(Player[i]->apiobj.flags_low) < 0 &&
                    Player[i]->apiobj.field_0x287 == 0 && (Player[i]->field_0xe24 & 0x10) != 0)
                    return 1;
            }
            return 0;
        default:
            return 0;
    }
}

void Tag_NewTransfer(GameObject_s *source, GameObject_s *target) {
    const i8 player_index = target->apiobj.field_0x27c;
    if (static_cast<u8>(player_index) < 2) {
        Tag_Transfer[player_index].time = 0.0f;
        Tag_Transfer[player_index].source = source;
        Tag_Transfer[player_index].height[0] = static_cast<f32>(qrand()) * 1.5259022e-05f * 0.4f + 0.4f;
        TAGTRANSFER_s &first = Tag_Transfer[target->apiobj.field_0x27c];
        first.position[0].x = source->apiobj.collision_position.x;
        first.position[0].z = source->apiobj.collision_position.z;
        first.position[0].y = first.height[0] * (source->apiobj.collision_max.y - source->apiobj.collision_min.y) +
                              source->apiobj.collision_min.y;

        const i8 second_index = target->apiobj.field_0x27c;
        Tag_Transfer[second_index].height[1] = static_cast<f32>(qrand()) * 1.5259022e-05f * 0.4f + 0.4f;
        TAGTRANSFER_s &second = Tag_Transfer[target->apiobj.field_0x27c];
        second.position[1].x = source->apiobj.collision_position.x;
        second.position[1].z = source->apiobj.collision_position.z;
        second.position[1].y = second.height[1] * (source->apiobj.collision_max.y - source->apiobj.collision_min.y) +
                               source->apiobj.collision_min.y;

        const i8 third_index = target->apiobj.field_0x27c;
        Tag_Transfer[third_index].height[2] = static_cast<f32>(qrand()) * 1.5259022e-05f * 0.4f + 0.4f;
        TAGTRANSFER_s &third = Tag_Transfer[target->apiobj.field_0x27c];
        third.position[2].x = source->apiobj.collision_position.x;
        third.position[2].z = source->apiobj.collision_position.z;
        third.position[2].y = third.height[2] * (source->apiobj.collision_max.y - source->apiobj.collision_min.y) +
                              source->apiobj.collision_min.y;
    }
    if (static_cast<i8>(target->apiobj.flags_low) < 0) {
        if (Tag_DoneFirst == 0) {
            Tag_DoneFirst = 1;
        } else if (Tag_DoneFirst == 1) {
            Tag_DoneFirst = 2;
        }
        Tag_DoneAny = 1;
    }
}

void Tag_DrawIcon_LSW(GameObject_s *object) {
    if (VehicleArea != 0 || FadeSys.fade != 0.0f || static_cast<i8>(object->apiobj.flags_low) >= 0 ||
        (WORLD->current_level->flags & LEVEL_HIDE_ICONS) != 0) {
        return;
    }
    if (Tag_NoHiddenIconFn != NULL && Tag_NoHiddenIconFn(object) != 0) {
        return;
    }
    if ((object->field_0xefe & 0x10) == 0) {
        f32 timer = object->hud_icon_timer;
        if (!(timer > 0.0f) || (timer < 2.0f && NuFmod(timer, 0.4f) < 0.2f)) {
            return;
        }
    }
    ADDGAMEMSG message = AddGameMsg_Default;
    message.field_0x4f = 1;
    message.scale = 3.0f;
    NUVEC position = object->apiobj.position;
    position.y += object->field_0xffc * object->apiobj.field_0xa8;
    message.text = LEGOASCII_UP;
    message.position = &position;
    message.red = PlayerRGB[0][0];
    message.green = PlayerRGB[0][1];
    message.blue = PlayerRGB[0][2];
    f32 phase = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f);
    message.flags = 0x87;
    message.alpha = static_cast<i32>(48.0f * NU_SIN_LUT(static_cast<u16>((phase + phase) * 65536.0f)) + 80.0f);
    AddGameMsg(&message);
}

void Tag_ResetTransfers() {
    Tag_Transfer[0].time = Tag_TransferResetTimer;
    Tag_Transfer[1].time = Tag_TransferResetTimer;
}

void Tag_DrawIcon_Batman(GameObject_s *) {
}

extern "C" void AddVariableShotDebrisEffectTimed1(i32, NUVEC *, i32, f32, i16, i16, NUMTX *);

void Tag_UpdateTransfers(i32 effect0, i32 effect1, i32 count) {
    NUVEC_ALIGNED16 position;
    if (Player[0] == NULL) {
        Tag_Transfer[0].time = Tag_TransferResetTimer;
        goto second_player;
    }
    if (!(Tag_Transfer[0].time < Tag_TransferResetTimer))
        goto second_player;
    Tag_Transfer[0].time += FRAMETIME;
    if (Tag_Transfer[0].time > Tag_TransferResetTimer) {
        Tag_Transfer[0].time = Tag_TransferResetTimer;
        goto second_player;
    }
    {
        f32 phase = Tag_Transfer[0].time + Tag_Transfer[0].time;
        {
            GameObject_s *source = Tag_Transfer[0].source;
            GameObject_s *target = Player[0];
            const f32 height = Tag_Transfer[0].height[0];
            NUVEC from = source->apiobj.collision_position;
            NUVEC to = target->apiobj.collision_position;
            from.y = (source->apiobj.collision_max.y - source->apiobj.collision_min.y) * height +
                     source->apiobj.collision_min.y;
            to.y = (target->apiobj.collision_max.y - target->apiobj.collision_min.y) * height +
                   target->apiobj.collision_min.y;
            position.x = (to.x - from.x) * phase + from.x;
            position.y = (to.y - from.y) * phase + from.y;
            position.z = (to.z - from.z) * phase + from.z;
            AddVariableShotDebrisEffectTimed1(effect0, &position, count, FRAMETIME, 0, 0, NULL);
            Tag_Transfer[0].position[0] = position;
        }
        {
            phase = NU_SIN_LUT(static_cast<i32>(phase * 16384.0f));
            GameObject_s *source = Tag_Transfer[0].source;
            GameObject_s *target = Player[0];
            const f32 height = Tag_Transfer[0].height[1];
            NUVEC from = source->apiobj.collision_position;
            NUVEC to = target->apiobj.collision_position;
            from.y = (source->apiobj.collision_max.y - source->apiobj.collision_min.y) * height +
                     source->apiobj.collision_min.y;
            to.y = (target->apiobj.collision_max.y - target->apiobj.collision_min.y) * height +
                   target->apiobj.collision_min.y;
            position.x = (to.x - from.x) * phase + from.x;
            position.y = (to.y - from.y) * phase + from.y;
            position.z = (to.z - from.z) * phase + from.z;
            position.y += 0.005f * NU_SIN_LUT(static_cast<i32>(phase * 65536.0f));
            AddVariableShotDebrisEffectTimed1(effect0, &position, count, FRAMETIME, 0, 0, NULL);
            Tag_Transfer[0].position[1] = position;
        }
        {
            phase = 1.0f - NU_SIN_LUT(static_cast<i32>(phase * 16384.0f + 16384.0f));
            GameObject_s *source = Tag_Transfer[0].source;
            GameObject_s *target = Player[0];
            const f32 height = Tag_Transfer[0].height[2];
            NUVEC from = source->apiobj.collision_position;
            NUVEC to = target->apiobj.collision_position;
            from.y = (source->apiobj.collision_max.y - source->apiobj.collision_min.y) * height +
                     source->apiobj.collision_min.y;
            to.y = (target->apiobj.collision_max.y - target->apiobj.collision_min.y) * height +
                   target->apiobj.collision_min.y;
            position.x = (to.x - from.x) * phase + from.x;
            position.y = (to.y - from.y) * phase + from.y;
            position.z = (to.z - from.z) * phase + from.z;
            position.y += 0.01f * NU_SIN_LUT(static_cast<i32>(phase * 32768.0f + 16384.0f));
            AddVariableShotDebrisEffectTimed1(effect0, &position, count, FRAMETIME, 0, 0, NULL);
            Tag_Transfer[0].position[2] = position;
        }
    }
second_player:
    if (Player[1] == NULL) {
        Tag_Transfer[1].time = Tag_TransferResetTimer;
        return;
    }
    if (!(Tag_Transfer[1].time < Tag_TransferResetTimer))
        return;
    Tag_Transfer[1].time += FRAMETIME;
    if (Tag_Transfer[1].time > Tag_TransferResetTimer) {
        Tag_Transfer[1].time = Tag_TransferResetTimer;
        return;
    }
    {
        f32 phase = Tag_Transfer[1].time + Tag_Transfer[1].time;
        {
            GameObject_s *source = Tag_Transfer[1].source;
            GameObject_s *target = Player[1];
            const f32 height = Tag_Transfer[1].height[0];
            NUVEC from = source->apiobj.collision_position;
            NUVEC to = target->apiobj.collision_position;
            from.y = (source->apiobj.collision_max.y - source->apiobj.collision_min.y) * height +
                     source->apiobj.collision_min.y;
            to.y = (target->apiobj.collision_max.y - target->apiobj.collision_min.y) * height +
                   target->apiobj.collision_min.y;
            position.x = (to.x - from.x) * phase + from.x;
            position.y = (to.y - from.y) * phase + from.y;
            position.z = (to.z - from.z) * phase + from.z;
            AddVariableShotDebrisEffectTimed1(effect1, &position, count, FRAMETIME, 0, 0, NULL);
            Tag_Transfer[1].position[0] = position;
        }
        {
            phase = NU_SIN_LUT(static_cast<i32>(phase * 16384.0f));
            GameObject_s *source = Tag_Transfer[1].source;
            GameObject_s *target = Player[1];
            const f32 height = Tag_Transfer[1].height[1];
            NUVEC from = source->apiobj.collision_position;
            NUVEC to = target->apiobj.collision_position;
            from.y = (source->apiobj.collision_max.y - source->apiobj.collision_min.y) * height +
                     source->apiobj.collision_min.y;
            to.y = (target->apiobj.collision_max.y - target->apiobj.collision_min.y) * height +
                   target->apiobj.collision_min.y;
            position.x = (to.x - from.x) * phase + from.x;
            position.y = (to.y - from.y) * phase + from.y;
            position.z = (to.z - from.z) * phase + from.z;
            position.y += 0.005f * NU_SIN_LUT(static_cast<i32>(phase * 65536.0f));
            AddVariableShotDebrisEffectTimed1(effect1, &position, count, FRAMETIME, 0, 0, NULL);
            Tag_Transfer[1].position[1] = position;
        }
        {
            phase = 1.0f - NU_SIN_LUT(static_cast<i32>(phase * 16384.0f + 16384.0f));
            GameObject_s *source = Tag_Transfer[1].source;
            GameObject_s *target = Player[1];
            const f32 height = Tag_Transfer[1].height[2];
            NUVEC from = source->apiobj.collision_position;
            NUVEC to = target->apiobj.collision_position;
            from.y = (source->apiobj.collision_max.y - source->apiobj.collision_min.y) * height +
                     source->apiobj.collision_min.y;
            to.y = (target->apiobj.collision_max.y - target->apiobj.collision_min.y) * height +
                   target->apiobj.collision_min.y;
            position.x = (to.x - from.x) * phase + from.x;
            position.y = (to.y - from.y) * phase + from.y;
            position.z = (to.z - from.z) * phase + from.z;
            position.y += 0.01f * NU_SIN_LUT(static_cast<i32>(phase * 32768.0f + 16384.0f));
            AddVariableShotDebrisEffectTimed1(effect1, &position, count, FRAMETIME, 0, 0, NULL);
            Tag_Transfer[1].position[2] = position;
        }
    }
}
