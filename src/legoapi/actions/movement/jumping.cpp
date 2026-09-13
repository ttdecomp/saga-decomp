#include "decomp.h"
#include "globals.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nutrig.h"
#include "legoapi/world/world.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/items/objects/gameobjects.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "gameapi/ai/aisys/aisys.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;
void StartLunge(GameObject_s *, f32, f32);
i32 Slam_Start(GameObject_s *, f32);
void StartHold(GameObject_s *);
void ComboHitFrame(GameObject_s *, i32);
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);
void PlaySabreSfx(char *, GameObject_s *, NUVEC *, i32);
i32 DoubleJump_JediSlam = 0;
f32 SLAMJUMPSPEED = 3.0f;
bool (*IsWearingBackPackFn)(GameObject_s *) = NULL;
i32 (*Jump_PreventJumpFn)(GameObject_s *) = NULL;
i32 (*CanMagnetClimbFn)(GameObject_s *) = NULL;
i32 (*CanGlideFn)(GameObject_s *) = NULL;
i32 DoubleJump_AlwaysReachJump2Height = 0;
i32 (*Slam_GetDebrisFn)(GameObject_s *, i32) = NULL;

enum PLAYER_JUMP_RUNTIME_FLAGS : u8 {
    PLAYER_JUMP_RUNTIME_BUTTON_HELD = 0x10,
};

enum PLAYER_JUMP_INPUT_FLAGS : u8 {
    PLAYER_JUMP_INPUT_BUFFERED = 0x10,
};

enum PLAYER_JUMP_FLAGS : u8 {
    PLAYER_JUMP_FLAG_SPECIAL_TAKEOFF = 0x01,
};

enum PLAYER_JUMP_CONTEXT_FLAGS : u32 {
    PLAYER_JUMP_CONTEXT_ALLOW_START = 0x00001000,
};

enum PLAYER_JUMP_VARIANT_FLAGS : u8 {
    PLAYER_JUMP_VARIANT_BUTTON_RELEASED = 0x10,
    PLAYER_JUMP_VARIANT_SECOND_JUMP = 0x40,
    PLAYER_JUMP_VARIANT_FALLING = 0x80,
    PLAYER_JUMP_VARIANT_START_CLEAR = 0x90,
    PLAYER_JUMP_VARIANT_END_CLEAR = 0x50,
};

enum PLAYER_MOVEMENT_RUNTIME_FLAGS : u8 {
    PLAYER_MOVEMENT_RUNTIME_DISABLE_JUMP_CODE = 0x10,
};

enum PLAYER_JUMP_ANIMATION_FLAGS : u32 {
    PLAYER_JUMP_ANIMATION_ALLOW_DOUBLE_JUMP = 0x0008,
    PLAYER_JUMP_ANIMATION_USE_THIRD_JUMP = 0x0010,
};

static const f32 PLAYER_JUMP_MINIMUM_AIR_TIME = 0.1f;
static const f32 PLAYER_JUMP_REENTRY_DELAY = 0.2f;

void PlayJumpSfx(GameObject_s *object, i32 variant);
void PlayLandSfx(GameObject_s *object, i32 variant, i32 force);

void StartJump(GameObject_s *object, i32 movement_state);

static GAMECHARACTERDATA *Jump_GetCharacterData(GameObject_s *object) {
    if (object == NULL || object->apiobj.character_data == NULL) {
        return NULL;
    }
    return static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
}

static bool Jump_HasAction(const GameObject_s *object, PLAYER_JUMP_ACTION action) {
    return object != NULL && object->apiobj.character_model != NULL &&
           object->apiobj.character_model->model_data_b != NULL &&
           object->apiobj.character_model->model_data_b[action] != NULL;
}

void (*BigJump_EndOfLandFn)(GameObject_s *) = NULL;
void StartEndOfJump(GameObject_s *);
void FindSlamOrigin(GameObject_s *, NUVEC *, NUVEC *);
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void NewRumbleAllPlayers(f32, f32, i32, i32);
EXPLOSION *AddExplosion(NUVEC *, f32, f32, GameObject_s *, i32, i32);

static i32 BigJump_JumpAction_Default(GameObject_s *object) {
    if (object->field_0x7aa == 0) {
        if (LEGOACT_COMBOJUMP != -1 && object->apiobj.character_model->model_data_b[LEGOACT_COMBOJUMP] != NULL)
            return LEGOACT_COMBOJUMP;
        if (LEGOACT_JUMP2 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP2] != NULL)
            return LEGOACT_JUMP2;
    } else if (object->field_0x7aa == 4) {
        if (LEGOACT_FLIP != -1 && object->apiobj.character_model->model_data_b[LEGOACT_FLIP] != NULL)
            return LEGOACT_FLIP;
    }
    return LEGOACT_JUMP;
}
i32 (*BigJump_JumpActionFn)(GameObject_s *) = BigJump_JumpAction_Default;

static i32 BigJump_LandAction_Default(GameObject_s *object) {
    switch (object->field_0x7aa) {
        case 0:
            if (LEGOACT_COMBOLAND != -1 && object->apiobj.character_model->model_data_b[LEGOACT_COMBOLAND] != NULL)
                return LEGOACT_COMBOLAND;
            break;
        case 2:
        case 3:
            if (LEGOACT_LAND3 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_LAND3] != NULL)
                return LEGOACT_LAND3;
            break;
        case 4:
            if (LEGOACT_FLIPLAND != -1 && object->apiobj.character_model->model_data_b[LEGOACT_FLIPLAND] != NULL)
                return LEGOACT_FLIPLAND;
            return LEGOACT_LAND;
        case 1:
            return LEGOACT_LAND;
        default:
            return LEGOACT_LAND;
    }
    if (LEGOACT_LAND2 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_LAND2] != NULL)
        return LEGOACT_LAND2;
    return LEGOACT_LAND;
}
i32 (*BigJump_LandActionFn)(GameObject_s *) = BigJump_LandAction_Default;

void BigJumpCode(GameObject_s *object) {
    if (LEGOCONTEXT_BIGJUMP == -1 || object->character_context != LEGOCONTEXT_BIGJUMP)
        return;
    object->apiobj.respawn_timer = 0.0f;
    if (object->build_button_taps != 0 && object->field_0x1084 != 0) {
        object->character_context = -1;
        return;
    }
    if (object->field_0x7a3 == 2) {
        object->context_animation_timer += FRAMETIME;
        if (object->context_animation_timer >= object->airborne_action_duration) {
            object->character_context = -1;
            if (BigJump_EndOfLandFn == NULL)
                return;
            BigJump_EndOfLandFn(object);
        }
    } else if (object->field_0x7a3 == 1) {
        object->context_animation_timer += FRAMETIME;
        if (object->context_animation_timer >= object->airborne_action_duration) {
            if (LEGOACT_FALL != -1 && object->context_animation == LEGOACT_FALL) {
                object->character_context = -1;
                return;
            }
            if (object->field_0x7aa != 0 &&
                (object->pad_gamepad->input_magnitude > 0.0f || object->IsRunningTaskType(MechTouchTaskGoTo::HashId))) {
                object->character_context = -1;
                return;
            }
            WORLDINFO *world = WorldInfo_CurrentlyActive();
            if (LEGOACT_COMBOJUMP != -1 && object->context_animation == LEGOACT_COMBOJUMP) {
                object->apiobj.facing_angle += 0x8000;
                object->apiobj.movement_facing_angle += 0x8000;
                object->apiobj.field_0x276 += 0x8000;
                ResetAnimPacket(&object->apiobj.anim_packet, -1);
            }
            if ((object->context_variant_flags & 1) != 0 && object->apiobj.field_0x27d == 0) {
                f32 ground = GameShadow(NULL, &object->launch_origin, 5.0f, -1);
                if (ground != 2000000.0f && object->launch_origin.y - ground >= 0.1f) {
                    StartEndOfJump(object);
                    return;
                }
            }
            ResetAnimPacket(&object->apiobj.anim_packet, -1);
            object->field_0x7a3 = 2;
            object->context_animation = BigJump_LandActionFn(object);
            f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
            object->airborne_action_duration = duration == 0.0f ? 1.0f : duration;
            object->context_animation_timer = 0.0f;
            if ((object->field_0xefb & 8) != 0 || (object->field_0xefd & 0x20) != 0) {
                GameCam_Judder(GameCam, -0.3f, 0, &object->apiobj.collision_position);
                PlayLandSfx(object, 3, 0);
                NewRumbleAllPlayers(0.6f, 0.0f, 0, 0);
            } else if ((object->apiobj.flags_low & 0x80) != 0 &&
                       (TestForController() || SuperOptions.touch_controls == 0)) {
                GameCam_Judder(GameCam, -0.2f, 0, &object->apiobj.collision_position);
                PlayLandSfx(object, 3, 0);
            } else {
                PlayLandSfx(object, 0, 0);
            }
            object->external_force = object->apiobj.position;
            object->apiobj.velocity.x = object->apiobj.velocity.y = object->apiobj.velocity.z = 0.0f;
            if ((object->field_0xefd & 0x20) != 0) {
                NUVEC origin;
                FindSlamOrigin(object, &origin, NULL);
                AddExplosion(&origin, 2.0f, 0.4f, object, object->slam_debris_effect, 0x19);
            }
            AISysGetCharacterPathPos(world->ai_sys, &object->apiobj, &object->ai, 0xff, 1);
        } else {
            f32 fraction = object->context_animation_timer / object->airborne_action_duration;
            f32 horizontal = fraction;
            f32 arc = 0.0f;
            if (object->field_0x7aa != 0) {
                f32 rise = object->launch_origin.y - object->external_force.y;
                if (rise > 0.21f) {
                    horizontal = NU_SIN_LUT(static_cast<i32>(fraction * 16384.0f + 32768.0f + 16384.0f)) + 1.0f;
                    if (rise > 0.42f)
                        arc = rise / 3.0f + 0.0f;
                } else if (rise < -0.21f) {
                    horizontal = NU_SIN_LUT(static_cast<i32>(fraction * 16384.0f + 49152.0f + 16384.0f));
                    if (rise < -0.42f)
                        arc = 0.0f - rise / 3.0f;
                }
            }
            f32 sine = NU_SIN_LUT(static_cast<i32>(fraction * 32768.0f));
            object->apiobj.position.x =
                object->external_force.x + (object->launch_origin.x - object->external_force.x) * horizontal;
            object->apiobj.position.y =
                object->external_force.y + (object->launch_origin.y - object->external_force.y) * fraction + arc * sine;
            object->apiobj.position.z =
                object->external_force.z + (object->launch_origin.z - object->external_force.z) * horizontal;
            f32 height = sine * object->apiobj.character_data->game_character->second_jump_height;
            object->apiobj.position.y += (height + height) * object->big_jump_height;
            NuVecSub(&object->apiobj.velocity, &object->apiobj.position, &object->apiobj.start_position);
            NuVecScale(&object->apiobj.velocity, &object->apiobj.velocity, 1.0f / FRAMETIME);
            if ((object->jump_flags & 4) == 0) {
                f32 half_duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 0) * 0.5f;
                if (object->context_animation_timer >= half_duration) {
                    if (static_cast<u8>(object->field_0x7aa - 2) <= 1) {
                        if (LEGOACT_JUMP3 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP3] != NULL)
                            object->context_animation = LEGOACT_JUMP3;
                        else if (LEGOACT_JUMP2 != -1 &&
                                 object->apiobj.character_model->model_data_b[LEGOACT_JUMP2] != NULL)
                            object->context_animation = LEGOACT_JUMP2;
                    }
                    object->jump_flags |= 4;
                }
            }
        }
    }
    if (object->character_context != -1)
        object->ai.movement_event_flags |= 2;
}

i32 UseFallAnim(GameObject_s *object) {
    const CHARACTER_CONTEXT_INFO_s &context = CInfo[object->character_context];
    return (context.flags & CHARACTER_CONTEXT_INFO_FLAG_USE_FALL_ANIMATION) != 0 && LEGOACT_FALL != -1 &&
           object->apiobj.character_model->model_data_b[LEGOACT_FALL] != NULL;
}

extern i16 id_YODA;
void Player_ClearContext(GameObject_s *, i32);
void Player_ResetContexts(PLAYERPACKET_s *);

i32 StartBigJump(GameObject_s *object, NUVEC *destination, i32 mode, f32 height, f32 duration_scale, i32 animation,
                 i8 variant) {
    if (LEGOCONTEXT_BIGJUMP == -1 || (object->apiobj.character_data->model_flags & 0x200000) != 0)
        return 0;
    Player_ClearContext(object, 0);
    GameObject_s *carried = object->field_0xcc0;
    if (carried == NULL || carried->id != id_YODA || WORLD->current_level != DAGOBAHE_LDATA)
        carried = NULL;
    Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    object->field_0xcc0 = carried;
    object->build_button_taps = animation;
    object->field_0x7a3 = 1;
    object->jump_flags &= ~4;
    object->character_context = LEGOCONTEXT_BIGJUMP;
    object->field_0x7aa = variant;
    object->context_animation = BigJump_JumpActionFn(object);
    object->external_force = object->apiobj.position;
    object->launch_origin = *destination;
    object->airborne_action_duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 0);
    if (static_cast<u8>(variant - 2) <= 1)
        object->airborne_action_duration = object->apiobj.character_data->game_character->second_jump_duration;
    if (object->field_0x7aa != 0) {
        f32 distance = NuVecXZDist(&object->external_force, &object->launch_origin, NULL);
        f32 speed = object->apiobj.character_data->game_character->run_speed;
        f32 duration = distance == 0.0f || speed == 0.0f ? 0.0f : distance / speed;
        if (!(object->airborne_action_duration > duration))
            object->airborne_action_duration = duration;
        if (object->field_0x7aa == 2) {
            if (LEGOACT_JUMP2 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP2] != NULL)
                height *= 1.5f;
        } else if (object->field_0x7aa == 3) {
            if (LEGOACT_JUMP3 != -1 && object->apiobj.character_model->model_data_b[LEGOACT_JUMP3] != NULL)
                height *= 1.8f;
        }
    }
    if (object->airborne_action_duration == 0.0f) {
        object->airborne_action_duration = 1.0f;
        object->context_animation = LEGOACT_FALL;
    } else {
        ResetAnimPacket(&object->apiobj.anim_packet, -1);
    }
    object->airborne_action_duration *= duration_scale;
    object->context_animation_timer = 0.0f;
    object->launch_origin.y -= object->character_bottom * object->apiobj.field_0xa8;
    u16 angle = NuAtan2D(destination->x - object->external_force.x, destination->z - object->external_force.z);
    if (object->field_0x7aa == 4)
        angle -= 0x8000;
    object->apiobj.movement_facing_angle = angle;
    object->apiobj.facing_angle = angle;
    object->apiobj.field_0x276 = angle;
    object->field_0xefd = (object->field_0xefd & ~0x20) | ((mode & 1) << 5);
    PlayJumpSfx(object, 2);
    object->ai.movement_event_flags |= 2;
    object->ai.field_0x180 = NULL;
    object->context_variant_flags &= ~1;
    object->big_jump_height = height < 0.0f ? 0.0f : height;
    return 1;
}

i32 StartFallLand(GameObject_s *object, i32 action) {
    PlayLandSfx(object, 0, 0);
    if (LEGOCONTEXT_LAND_JUMP == -1) {
        object->movement_runtime_flags &= ~4;
        return 0;
    }
    void **animations = object->apiobj.character_model->model_data_b;
    if (action == -1 || animations[action] == NULL) {
        if (IsWearingBackPackFn != NULL && IsWearingBackPackFn(object) && LEGOACT_BACKPACKFALLLAND != -1 &&
            animations[LEGOACT_BACKPACKFALLLAND] != NULL) {
            object->context_animation = LEGOACT_BACKPACKFALLLAND;
        } else if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) && LEGOACT_EXTRA_LAND2 != -1 &&
                   animations[LEGOACT_EXTRA_LAND2] != NULL) {
            object->context_animation = LEGOACT_EXTRA_LAND2;
        } else if (LEGOACT_FALLLAND != -1 && animations[LEGOACT_FALLLAND] != NULL) {
            object->context_animation = LEGOACT_FALLLAND;
        } else if (LEGOACT_LAND2 != -1 && animations[LEGOACT_LAND2] != NULL) {
            object->context_animation = LEGOACT_LAND2;
        } else {
            object->context_animation = LEGOACT_LAND;
            if (animations[object->context_animation] == NULL) {
                object->movement_runtime_flags &= ~4;
                return 0;
            }
        }
    } else {
        object->context_animation = action;
        if (animations[object->context_animation] == NULL) {
            object->movement_runtime_flags &= ~4;
            return 0;
        }
    }
    object->character_context = LEGOCONTEXT_LAND_JUMP;
    ResetAnimPacket(&object->apiobj.anim_packet, -1);
    ResetMiniAnimPacket(&object->mini_animation, -1);
    object->fall_animation_timer = 0.0f;
    object->context_animation_timer = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 1);
    object->movement_runtime_flags =
        static_cast<u8>((object->movement_runtime_flags & ~0x08) | ((object->movement_runtime_flags & 0x04) << 1));
    return 1;
}

void StartEndOfJump(GameObject_s *object) {
    if (LEGOCONTEXT_JUMP != -1) {
        object->character_context = LEGOCONTEXT_JUMP;
        object->action_movement_state = 0;
        object->jump_sequence = 2;
        object->jump_flags &= ~PLAYER_JUMP_FLAG_SPECIAL_TAKEOFF;
        object->context_animation = LEGOACT_FALL;
        object->context_variant_flags =
            static_cast<i8>((static_cast<u8>(object->context_variant_flags) | PLAYER_JUMP_VARIANT_FALLING) &
                            ~PLAYER_JUMP_VARIANT_END_CLEAR);
    }
    object->airborne_collision_target = NULL;
}

void StartBallooning(GameObject_s *object, i32 movement_state) {
    object->field_0x7a3 = movement_state;
    object->character_context = 0x5d;
    object->context_animation = object->apiobj.character_model->model_data_b[0xb1] != NULL ? 0xb1 : 0x101;
    object->field_0x768 = 1000000000.0f;
}

void StartJetPackFall(GameObject_s *, i32) {
}

void MakeJumpReachHeight(GameObject_s *object, float height, i32 force) {
    const f32 remaining_height = height - (object->apiobj.position.y - object->jump_start_height);
    if (remaining_height > 0.0f) {
        const GAMECHARACTERDATA *game_character =
            static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
        const f32 vertical_speed = NuFsqrt(-2.0f * game_character->gravity * remaining_height);
        if (force != 0 || vertical_speed > object->apiobj.velocity.y) {
            object->apiobj.velocity.y = vertical_speed;
        }
    } else if (force != 0) {
        object->apiobj.velocity.y = 0.0f;
    }
}

void SetBallooningHeight(GameObject_s *object, float height) {
    if (object->character_context == 0x5d)
        object->field_0x768 = height;
}

void StartJump(GameObject_s *object, i32 movement_state) {
    GAMECHARACTERDATA *game_character = Jump_GetCharacterData(object);
    if (object == NULL || game_character == NULL) {
        return;
    }

    object->character_context = CHARACTER_CONTEXT_JUMP;
    object->action_movement_state = static_cast<u8>(movement_state);
    object->jump_sequence = 1;
    object->jump_start_height = object->apiobj.position.y;
    object->context_animation_timer = 0.0f;
    object->context_animation = PLAYER_JUMP_ACTION_JUMP;
    object->jump_flags =
        static_cast<u8>((object->jump_flags & ~PLAYER_JUMP_FLAG_SPECIAL_TAKEOFF) |
                        ((movement_state == 3 || movement_state == 4) ? PLAYER_JUMP_FLAG_SPECIAL_TAKEOFF : 0));
    object->context_variant_flags =
        static_cast<i8>(static_cast<u8>(object->context_variant_flags) & ~PLAYER_JUMP_VARIANT_START_CLEAR);
    object->airborne_action_timer = 0.0f;
    if (movement_state != 6 && movement_state != 7) {
        PlayJumpSfx(object, 0);
    }
    object->apiobj.field_0x27d = 0;
    object->field_0x105c = 0;
    object->field_0xe22 |= PLAYER_JUMP_RUNTIME_BUTTON_HELD;
    object->delayed_turn_timer = 0.0f;
    object->airborne_input_timer = 0.0f;
    object->airborne_collision_target = NULL;
    object->apiobj.velocity.y = game_character->jump_speed;
    ResetAnimPacket(&object->apiobj.anim_packet, -1);
    object->apiobj.pitch_angle = 0;
    object->apiobj.roll_angle = 0;
    object->field_0x1086 = 2;
}
