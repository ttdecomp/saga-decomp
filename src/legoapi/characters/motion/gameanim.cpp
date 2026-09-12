#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "globals.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/nu3d/nutexanm.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/gizmo/base/gizactions.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nucore/nugcutscene.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nucore/nuhgobj.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numusic/sfx.h"

#include <float.h>
#include <string.h>

i32 LEGOCONTEXT_SUPERCARRY = -1;

i32 SuperCarry_Carrying(GameObject_s *object) {
    if (LEGOCONTEXT_SUPERCARRY != -1 && object->character_context == LEGOCONTEXT_SUPERCARRY) {
        if (static_cast<u8>(object->field_0x7a3 - 2) < 2)
            return 1;
        if (object->field_0x7a3 == 6)
            return 1;
    }
    return 0;
}

float CalcValue1648(char *data, i32 quarter, i32 stride, float fraction, ani3_scalemin_s *scale_min) {
    u16 *next = reinterpret_cast<u16 *>(data + stride);
    u16 *keys = reinterpret_cast<u16 *>(data);

    switch (quarter) {
        case 0: {
            float tangent = static_cast<float>((keys[2] & 0xfff) - (keys[1] & 0xfff)) * fraction +
                            static_cast<float>(keys[1] & 0xfff);
            float value =
                static_cast<float>(static_cast<i32>(*next) - static_cast<i32>(keys[0])) * (tangent / 4095.0f) +
                static_cast<float>(static_cast<i32>(keys[0]));
            return value * scale_min->scale + scale_min->minimum;
        }
        case 1: {
            float tangent = static_cast<float>((keys[3] & 0xfff) - (keys[2] & 0xfff)) * fraction +
                            static_cast<float>(keys[2] & 0xfff);
            float value =
                static_cast<float>(static_cast<i32>(*next) - static_cast<i32>(keys[0])) * (tangent / 4095.0f) +
                static_cast<float>(static_cast<i32>(keys[0]));
            return value * scale_min->scale + scale_min->minimum;
        }
        case 2: {
            i32 start = keys[3] & 0xfff;
            i32 end = ((keys[2] & 0xf000) >> 8) | ((keys[3] & 0xf000) >> 4) | (keys[1] >> 12);
            float tangent = static_cast<float>(end - start) * fraction + static_cast<float>(start);
            float value =
                static_cast<float>(static_cast<i32>(*next) - static_cast<i32>(keys[0])) * (tangent / 4095.0f) +
                static_cast<float>(static_cast<i32>(keys[0]));
            return value * scale_min->scale + scale_min->minimum;
        }
        case 3: {
            i32 next_value = *next;
            i32 tangent = ((keys[2] & 0xf000) >> 8) | ((keys[3] & 0xf000) >> 4) | (keys[1] >> 12);
            float a =
                static_cast<float>(next_value - static_cast<i32>(keys[0])) * static_cast<float>(tangent) / 4095.0f +
                static_cast<float>(static_cast<i32>(keys[0]));
            float b = static_cast<float>(static_cast<i32>(reinterpret_cast<u16 *>(data)[stride]) - next_value) *
                          static_cast<float>(next[1] & 0xfff) / 4095.0f +
                      static_cast<float>(next_value);
            float value = (b - a) * fraction + a;
            return value * scale_min->scale + scale_min->minimum;
        }
    }

    return 0.0f;
}
void CalcValue1648Get2Values(char *data, i32 quarter, i32 stride, ani3_scalemin_s *scale_min, float *first,
                             float *second) {
    u16 *next = reinterpret_cast<u16 *>(data + stride);
    u16 *keys = reinterpret_cast<u16 *>(data);
    i32 start, delta, end, last;
    switch (quarter) {
        case 0: {
            start = keys[0];
            delta = static_cast<i32>(next[0]) - start;
            *first = (static_cast<f32>(keys[1] & 0xfff) * static_cast<f32>(delta) / 4095.0f + static_cast<f32>(start)) *
                         scale_min->scale +
                     scale_min->minimum;
            *second =
                (static_cast<f32>(keys[2] & 0xfff) * static_cast<f32>(delta) / 4095.0f + static_cast<f32>(start)) *
                    scale_min->scale +
                scale_min->minimum;
            break;
        }
        case 1: {
            start = keys[0];
            delta = static_cast<i32>(next[0]) - start;
            *first = (static_cast<f32>(keys[2] & 0xfff) * static_cast<f32>(delta) / 4095.0f + static_cast<f32>(start)) *
                         scale_min->scale +
                     scale_min->minimum;
            *second =
                (static_cast<f32>(keys[3] & 0xfff) * static_cast<f32>(delta) / 4095.0f + static_cast<f32>(start)) *
                    scale_min->scale +
                scale_min->minimum;
            break;
        }
        case 2: {
            start = keys[0];
            delta = static_cast<i32>(next[0]) - start;
            *first = (static_cast<f32>(keys[3] & 0xfff) * static_cast<f32>(delta) / 4095.0f + static_cast<f32>(start)) *
                         scale_min->scale +
                     scale_min->minimum;
            last = ((keys[2] & 0xf000) >> 8) | ((keys[3] & 0xf000) >> 4) | (keys[1] >> 12);
            *second = (static_cast<f32>(last) * static_cast<f32>(delta) / 4095.0f + static_cast<f32>(start)) *
                          scale_min->scale +
                      scale_min->minimum;
            break;
        }
        case 3: {
            last = ((keys[2] & 0xf000) >> 8) | ((keys[3] & 0xf000) >> 4) | (keys[1] >> 12);
            start = keys[0];
            end = next[0];
            *first = (static_cast<f32>(last) * static_cast<f32>(end - start) / 4095.0f + static_cast<f32>(start)) *
                         scale_min->scale +
                     scale_min->minimum;
            *second = (static_cast<f32>(static_cast<i32>(next[static_cast<u32>(stride) / sizeof(u16)]) - end) *
                           static_cast<f32>(next[1] & 0xfff) / 4095.0f +
                       static_cast<f32>(end)) *
                          scale_min->scale +
                      scale_min->minimum;
            break;
        }
    }
}
extern "C" void VuQuatSlerpFast(NUQUAT *out, NUQUAT *from, NUQUAT *to, f32 t);
void EvalAnim(nuhspecial_s *special, f32 frame, numtx_s *matrix, i32 include_instance_translation);
i32 UseFallAnim(GameObject_s *object);
i32 GetDefaultIdle(GameObject_s *object);
i32 SetProtocolDroidFallAnim(GameObject_s *object);
// TODO: Restore target-local linkage once the four remaining animation-mode
// callers are decompiled; their references naturally prevent inlining.
static void MoveAnim_Manage(GameObject_s *object, f32 movement_speed, i32 allow_tiptoe, i32 weapon_variant);
static void MoveAnim_Check(GameObject_s *object);
static void JumpAnimCode(GameObject_s *object);
static bool JumpAnim_HasAction(const GameObject_s *object, i16 action);
static GAMECHARACTERDATA *GetGameCharacterData(GameObject_s *object);
void UpdateCharacterIdle(GameObject_s *object);
void AutoWeaponOnOff(GameObject_s *object);
void AddFootSteps(GameObject_s *object);
f32 GizBuildItMul(GameObject_s *object);
extern "C" f32 AnimDuration(i32 character_id, i32 animation, f32 start_frame, f32 end_frame, i32 subtract_frame_time);
void RootFnEx(NUMTX *matrix, void *data, NUVEC *source_root, NUVEC *target_root, NUVEC *root_delta, f32 blend,
              i32 include_y);
extern "C" void PlaySfxByIdAndSetVolume(i32 sfx_id, NUVEC *position, f32 volume);
i32 MatrixReflection(NUMTX *matrix, i32 axis, f32 plane, f32 height, NUMTX *result);

extern i16 id_BODYGUARD;
extern i16 id_IMPERIALGUARD;
extern i16 id_YODA;
extern i16 id_YODAGHOST;
extern i16 id_GONKDROID;
extern "C" i32 GetAnimBlendMode(void);

enum CHARACTER_ANIMATION : i16 {
    CHARACTER_ANIMATION_WALK = 0,
    CHARACTER_ANIMATION_IDLE = 1,
    CHARACTER_ANIMATION_RUN = 3,
    CHARACTER_ANIMATION_TIPTOE = 4,
    CHARACTER_ANIMATION_FALL = 5,
    CHARACTER_ANIMATION_WEAPON_IDLE = 11,
    CHARACTER_ANIMATION_ALT_IDLE = 25,
    CHARACTER_ANIMATION_SABER_RUN = 23,
    CHARACTER_ANIMATION_ALT_WEAPON_IDLE = 39,
    CHARACTER_ANIMATION_FALL_VARIANT_40 = 40,
    CHARACTER_ANIMATION_SABER_TIPTOE = 63,
    CHARACTER_ANIMATION_SABER_WALK = 64,
    CHARACTER_ANIMATION_FALL_VARIANT_75 = 75,
    CHARACTER_ANIMATION_FALL_VARIANT_76 = 76,
    CHARACTER_ANIMATION_BACKWARDS = 80,
    CHARACTER_ANIMATION_EXTRA_TIPTOE = 113,
    CHARACTER_ANIMATION_EXTRA_WALK = 114,
    CHARACTER_ANIMATION_EXTRA_RUN = 115,
    CHARACTER_ANIMATION_EXTRA_FALL = 116,
    CHARACTER_ANIMATION_EXTRA_IDLE = 117,
    CHARACTER_ANIMATION_SUIT_TIPTOE = 198,
    CHARACTER_ANIMATION_SUIT_WALK = 199,
    CHARACTER_ANIMATION_SUIT_RUN = 200,
};

enum CHARACTER_ANIMATION_FLAGS : u32 {
    CHARACTER_ANIMATION_FLAG_SYNCHRONISED = 0x02,
    CHARACTER_ANIMATION_FLAG_ROOT_MOTION = 0x20,
    CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT = 0x80,
    CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION = 0x200,
};

static bool HasAnimation(const CHARACTERMODEL_s *model, i32 animation) {
    return model != NULL && animation >= 0 && model->model_data_b != NULL && model->model_data_b[animation] != NULL;
}

static void MoveAnim_Check(GameObject_s *object) {
    if (GetAnimBlendMode() == 1) {
        return;
    }

    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    i16 requested = packet.requested_animation;
    const i16 previous = packet.previous_animation;

    if (object->released_movement_animation != -1) {
        object->movement_animation_hold_timer = 0.1f;
        object->held_movement_animation = -1;
    } else {
        if (requested == previous) {
            object->movement_animation_hold_timer = 0.1f;
            object->held_movement_animation = -1;
            object->movement_animation_release_timer = 0.1f;
            object->released_movement_animation = -1;
            return;
        }

        const u32 requested_flags = ActionInfo[requested].flags;
        if (((requested_flags & 7) == 0 && requested != CHARACTER_ANIMATION_ALT_IDLE &&
             requested != CHARACTER_ANIMATION_IDLE) ||
            (ActionInfo[previous].flags & 7) == 0 || (requested_flags & 4) != 0) {
            object->movement_animation_hold_timer = 0.1f;
            object->held_movement_animation = -1;
            object->movement_animation_release_timer = 0.1f;
            object->released_movement_animation = -1;
            return;
        }

        if (object->held_movement_animation != -1) {
            object->movement_animation_hold_timer -= FRAMETIME;
            if (object->movement_animation_hold_timer > 0.0f) {
                packet.requested_animation = object->held_movement_animation;
                object->movement_animation_release_timer = 0.1f;
                object->released_movement_animation = -1;
                return;
            }
            object->held_movement_animation = -1;
        } else {
            object->movement_animation_hold_timer = 0.1f;
            if (packet.blending == 0) {
                bool retain_previous = false;
                if (previous == CHARACTER_ANIMATION_RUN) {
                    retain_previous = requested == CHARACTER_ANIMATION_WALK ||
                                      requested == CHARACTER_ANIMATION_TIPTOE || requested == CHARACTER_ANIMATION_IDLE;
                } else if (previous == CHARACTER_ANIMATION_SABER_RUN) {
                    const GAMECHARACTERDATA *game_character =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    const i16 alternate_idle =
                        game_character->field275_0x116 != 0
                            ? CHARACTER_ANIMATION_ALT_IDLE
                            : static_cast<i16>((object->apiobj.character_data->model_flags & 0x80) != 0 ? 118 : 25);
                    retain_previous = requested == CHARACTER_ANIMATION_SABER_TIPTOE ||
                                      requested == CHARACTER_ANIMATION_SABER_WALK || requested == alternate_idle;
                }

                if (retain_previous) {
                    packet.requested_animation = previous;
                    object->held_movement_animation = previous;
                    object->movement_animation_release_timer = 0.1f;
                    object->released_movement_animation = -1;
                    return;
                }
            }

            object->movement_animation_release_timer = 0.1f;
            object->released_movement_animation = -1;
            return;
        }
    }

    requested = packet.requested_animation;
    const u32 requested_flags = ActionInfo[requested].flags;
    if (requested == previous || (requested_flags & 7) == 0 ||
        (previous != CHARACTER_ANIMATION_ALT_IDLE && previous != CHARACTER_ANIMATION_IDLE) ||
        (requested_flags & 4) != 0) {
        object->movement_animation_release_timer = 0.1f;
        object->released_movement_animation = -1;
        return;
    }

    if (object->released_movement_animation != -1) {
        object->movement_animation_release_timer -= FRAMETIME;
        if (object->movement_animation_release_timer <= 0.0f) {
            object->movement_animation_release_timer = -1.0f;
        } else {
            packet.requested_animation = object->released_movement_animation;
        }
        return;
    }

    object->movement_animation_hold_timer = 0.1f;
    if (packet.blending == 0 && (requested_flags & 3) != 0) {
        packet.requested_animation = previous;
        object->released_movement_animation = previous;
    }
}

static void JumpAnimCode(GameObject_s *object) {
    if (object->context_variant_flags >= 0) {
        ANIMPACKET_s *packet = &object->apiobj.anim_packet;
        packet->requested_animation = object->context_animation;
        const u8 state = object->action_movement_state;
        if (object->context_animation != 0x49 && (state == 6 || state < 2 || state == 7 || state == 9)) {
            if (packet->blending == 0 && object->context_animation == packet->animation_index &&
                (packet->flags & 1) != 0) {
                object->airborne_input_timer += FRAMETIME;
                if (object->airborne_input_timer >= 0.1f &&
                    (object->nearby_floor_distance == 2000000.0f || object->nearby_floor_distance > 0.35f)) {
                    object->context_variant_flags |= 0x80;
                }
            } else {
                object->airborne_input_timer = 0.0f;
            }
        }
        return;
    }

    void **animations = object->apiobj.character_model->model_data_b;
    if (object->action_movement_state == PLAYER_JUMP_MOVEMENT_COMBAT_ROLL &&
        animations[PLAYER_JUMP_ACTION_COMBAT_ROLL_FALL] != NULL) {
        object->apiobj.anim_packet.requested_animation = PLAYER_JUMP_ACTION_COMBAT_ROLL_FALL;
        return;
    }
    if (animations[PLAYER_JUMP_ACTION_FALL] != NULL) {
        object->apiobj.anim_packet.requested_animation = PLAYER_JUMP_ACTION_FALL;
        return;
    }
    object->apiobj.anim_packet.requested_animation = object->context_animation;
}

static CHARACTERANIM_s *GetAnimationInfo(const CHARACTERMODEL_s *model, i32 animation) {
    if (model == NULL || animation < 0 || model->model_data_a == NULL) {
        return NULL;
    }
    return static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
}

static GAMECHARACTERDATA *GetGameCharacterData(GameObject_s *object) {
    if (object == NULL || object->apiobj.character_data == NULL) {
        return NULL;
    }
    return static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
}

static f32 UpdateAnimTimer(CHARACTERMODEL_s *model, ANIMPACKET_s *packet, i16 animation, f32 time, f32 frame_step,
                           f32 movement_speed, i32 report_events, char *reversed, i32 backwards,
                           f32 backwards_multiplier) {
    CHARACTERANIM_s *animation_info = GetAnimationInfo(model, animation);
    if (!HasAnimation(model, animation) || animation_info == NULL) {
        return time;
    }

    f32 rate = animation_info->playback_rate;
    if (animation_info->movement_speed > 0.0f) {
        rate *= movement_speed / animation_info->movement_speed;
        if (rate >= 0.0f) {
            if (animation_info->movement_rate_cap > 0.0f && rate > animation_info->movement_rate_cap) {
                rate = animation_info->movement_rate_cap;
            }
        } else if (animation_info->movement_rate_cap < 0.0f && rate < animation_info->movement_rate_cap) {
            rate = animation_info->movement_rate_cap;
        }
    }
    if (*reversed != 0) {
        frame_step = -(frame_step * backwards_multiplier);
    }

    const f32 delta = rate * (frame_step / 30.0f);
    time += delta;
    const f32 end_frame = NuAnimEndFrame(model->model_data_b[animation]);
    bool looped = false;

    // Original 0x3ce29f only enters reverse playback for an ordered negative delta.
    if (!(delta < 0.0f)) {
        if (time > end_frame) {
            if ((animation_info->flags & CHARACTER_ANIMATION_FLAG_SYNCHRONISED) == 0) {
                time = end_frame;
                if (report_events) {
                    packet->flags |= ANIMPACKET_FLAG_FINISHED;
                }
            } else {
                if (end_frame > 1.0f) {
                    while (time > end_frame) {
                        time -= end_frame - 1.0f;
                    }
                } else {
                    time = 1.0f;
                }
                if (report_events) {
                    packet->flags |= ANIMPACKET_FLAG_LOOPED;
                }
                looped = true;
            }
        }
    } else {
        if (report_events) {
            packet->flags |= ANIMPACKET_FLAG_PLAYING_REVERSED;
        }
        if (time < 1.0f) {
            if ((animation_info->flags & CHARACTER_ANIMATION_FLAG_SYNCHRONISED) == 0) {
                time = 1.0f;
                if (report_events) {
                    packet->flags |= ANIMPACKET_FLAG_FINISHED;
                }
            } else {
                if (end_frame > 1.0f) {
                    while (time < 1.0f) {
                        time += end_frame - 1.0f;
                    }
                } else {
                    time = 1.0f;
                }
                if (report_events) {
                    packet->flags |= ANIMPACKET_FLAG_LOOPED;
                }
                looped = true;
            }
        }
    }

    if (looped) {
        if (*reversed == 0) {
            if (backwards && (animation_info->flags & CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT) != 0) {
                *reversed = 1;
            }
        } else if (!backwards) {
            *reversed = 0;
        }
    }
    return time;
}

static void StartAnimation(CHARACTERMODEL_s *model, ANIMPACKET_s *packet, i16 animation, bool backwards) {
    packet->animation_index = animation;
    CHARACTERANIM_s *info = GetAnimationInfo(model, animation);
    const bool reverse =
        backwards && info != NULL && (info->flags & CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT) != 0;
    packet->current_reversed = reverse ? 1 : 0;
    packet->current_time = reverse ? NuAnimEndFrame(model->model_data_b[animation]) : 1.0f;
    packet->previous_time = packet->current_time;
    packet->blending = 0;
}

static const u8 KeyStructSizes[16] = {3, 4, 4, 3, 4, 3, 4, 8, 4, 8, 4, 0, 0, 0, 0, 0};
static const u8 CurveGroupMasks[3] = {2, 1, 8};

static inline f32 DecodeAni4V4Curve(const ani3_animheader_s *anim, u16 type, u32 quarter, f32 fraction, u8 *&keys,
                                    ani3_scalemin_s *&scale_min) {
    if (type != 6) {
        const u16 constant = reinterpret_cast<const u16 *>(anim->constants)[type - 16];
        return static_cast<f32>(constant) * anim->scale + anim->minimum;
    }

    const u32 first_word = *reinterpret_cast<const u32 *>(keys);
    const u32 next_word = *reinterpret_cast<const u32 *>(keys + anim->key_stride);
    const f32 first_value = static_cast<f32>(first_word & 0xff);
    const f32 next_value = static_cast<f32>(next_word & 0xff);
    const u32 tangents = first_word >> 8;
    const f32 tangent_scale = 0.01587302f;
    const f32 tangent0 = static_cast<f32>((tangents >> (quarter * 6)) & 0x3f) * tangent_scale;

    f32 packed_value;
    if (quarter == 3) {
        const f32 interpolated = (next_value - first_value) * tangent0 + first_value;
        const f32 tangent1 = static_cast<f32>((next_word >> 8) & 0x3f) * tangent_scale;
        const f32 after_value = static_cast<f32>(keys[anim->key_stride * 2]);
        const f32 next_interpolated = (after_value - next_value) * tangent1 + next_value;
        packed_value = (next_interpolated - interpolated) * fraction + interpolated;
    } else {
        const f32 tangent1 = static_cast<f32>((tangents >> (((quarter * 3 + 3) * 2) & 0x1f)) & 0x3f) * tangent_scale;
        packed_value = (next_value - first_value) * ((tangent1 - tangent0) * fraction + tangent0) + first_value;
    }

    const f32 value = packed_value * scale_min->scale + scale_min->minimum;
    keys += 4;
    ++scale_min;
    return value;
}

static inline f32 WrapAni4BlendRotation(f32 delta) {
    if (delta > 3.1415927f || delta < -3.1415927f) {
        i32 fixed_turn = static_cast<i32>(delta * 65536.0f / 6.2831855f) & 0xffff;
        if (fixed_turn >= 0x8000) {
            fixed_turn -= 0x10000;
        }
        delta = static_cast<f32>(fixed_turn) * 9.58738e-5f;
    }
    return delta;
}

static inline void SkipAni4V4Curve(u16 type, u8 *&keys, ani3_scalemin_s *&scale_min) {
    if (type < 16) {
        keys += KeyStructSizes[type];
        ++scale_min;
    }
}

static inline void GetAni4SamplePosition(const ani3_animheader_s *anim, f32 frame, u32 &quarter, f32 &fraction,
                                         i32 &key_offset) {
    if (anim->key_count == 1) {
        quarter = 0;
        fraction = 0.0f;
        key_offset = 0;
        return;
    }

    const f32 last_key = static_cast<f32>(anim->key_count - 1);
    f32 key = (frame - anim->first_frame) * last_key / static_cast<f32>(anim->frame_count - 1);
    if (key < 0.0f) {
        key = 0.0f;
    }
    if (last_key <= key) {
        key = last_key;
    }

    const i32 whole_key = static_cast<i32>(key);
    quarter = static_cast<u32>(whole_key) & 3;
    fraction = key - static_cast<f32>(whole_key);
    key_offset = (whole_key >> 2) * anim->key_stride;
}

static inline f32 DecodeAni4QuaternionScalar(const ani3_animheader_s *anim, u16 type, u32 quarter, f32 fraction,
                                             u8 *&keys, ani3_scalemin_s *&scale_min) {
    if (type == 7) {
        const f32 value = CalcValue1648(reinterpret_cast<char *>(keys), quarter, anim->key_stride, fraction, scale_min);
        keys += 8;
        ++scale_min;
        return value;
    }
    if (type == 9) {
        const u16 *samples = reinterpret_cast<const u16 *>(keys);
        const f32 first = static_cast<f32>(samples[quarter]);
        const f32 second = quarter == 3 ? static_cast<f32>(*reinterpret_cast<const u16 *>(keys + anim->key_stride))
                                        : static_cast<f32>(samples[quarter + 1]);
        const f32 value = ((second - first) * fraction + first) * scale_min->scale + scale_min->minimum;
        keys += 8;
        ++scale_min;
        return value;
    }
    return DecodeAni4V4Curve(anim, type, quarter, fraction, keys, scale_min);
}

static inline void DecodeAni4QuaternionPair(const ani3_animheader_s *anim, u16 type, u32 quarter, u8 *&keys,
                                            ani3_scalemin_s *&scale_min, f32 &first, f32 &second) {
    if (type == 7) {
        CalcValue1648Get2Values(reinterpret_cast<char *>(keys), quarter, anim->key_stride, scale_min, &first, &second);
        keys += 8;
        ++scale_min;
        return;
    }
    if (type == 9) {
        const u16 *samples = reinterpret_cast<const u16 *>(keys);
        first = static_cast<f32>(samples[quarter]) * scale_min->scale + scale_min->minimum;
        const u16 next = quarter == 3 ? *reinterpret_cast<const u16 *>(keys + anim->key_stride) : samples[quarter + 1];
        second = static_cast<f32>(next) * scale_min->scale + scale_min->minimum;
        keys += 8;
        ++scale_min;
        return;
    }
    if (type == 6) {
        u8 *pair_keys = keys;
        ani3_scalemin_s *pair_scale_min = scale_min;
        first = DecodeAni4V4Curve(anim, type, quarter, 0.0f, pair_keys, pair_scale_min);
        pair_keys = keys;
        pair_scale_min = scale_min;
        second = DecodeAni4V4Curve(anim, type, quarter, 1.0f, pair_keys, pair_scale_min);
        keys += 4;
        ++scale_min;
        return;
    }

    first = second =
        static_cast<f32>(reinterpret_cast<const u16 *>(anim->constants)[type - 16]) * anim->scale + anim->minimum;
}

static inline NUQUAT DecodeAni4Quaternion(const ani3_animheader_s *anim, i32 component_count, const u16 *types,
                                          u32 quarter, f32 fraction, u8 *&keys, ani3_scalemin_s *&scale_min) {
    NUQUAT first = {0.0f, 0.0f, 0.0f, 0.0f};
    NUQUAT second = {0.0f, 0.0f, 0.0f, 0.0f};
    f32 *first_values = &first.x;
    f32 *second_values = &second.x;
    for (i32 component = 0; component < component_count; ++component) {
        DecodeAni4QuaternionPair(anim, types[component], quarter, keys, scale_min, first_values[component],
                                 second_values[component]);
    }

    if (component_count == 3) {
        first.w = NuFsqrt(1.0f - first.x * first.x - first.y * first.y - first.z * first.z);
        second.w = NuFsqrt(1.0f - second.x * second.x - second.y * second.y - second.z * second.z);
    }

    NuQuatHarmonize(&first, &second);
    NUQUAT result;
    NuQuatLerp2(&result, &first, &second, fraction);
    NuQuatNormalise(&result, &result);
    return result;
}

static inline void SkipAni4QuaternionJoint(const ani3_animheader_s *anim, i32 quaternion_components, i32 joint,
                                           const u16 *types, u8 *&keys, ani3_scalemin_s *&scale_min) {
    const u8 flags = anim->node_flags[joint];
    if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
        for (i32 component = 0; component < 3; ++component) {
            SkipAni4V4Curve(types[component], keys, scale_min);
        }
    }
    if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
        for (i32 component = 0; component < quaternion_components; ++component) {
            SkipAni4V4Curve(types[3 + component], keys, scale_min);
        }
    }
    if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
        const i32 scale_offset = 3 + quaternion_components;
        for (i32 component = 0; component < 3; ++component) {
            SkipAni4V4Curve(types[scale_offset + component], keys, scale_min);
        }
    }
}

void Animate_POD(GameObject_s *) {
}

void Animate_ATAT(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_IDLE;
        if (static_cast<i16>(object->apiobj.field_0x1f8) < 0 &&
            object->apiobj.character_model->model_data_b[15] != NULL) {
            packet.requested_animation = 15;
        }
        if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
            object->pad_gamepad->input_magnitude > 0.0f) {
            packet.requested_animation = CHARACTER_ANIMATION_WALK;
        }
    }

    if (packet.requested_animation != CHARACTER_ANIMATION_IDLE) {
        return;
    }

    const i32 turn = RotDiff(object->previous_movement_angle, object->apiobj.field_0x276);
    if (turn > 0 && object->apiobj.character_model->model_data_b[79] != NULL) {
        packet.requested_animation = 79;
    } else if (turn < 0 && object->apiobj.character_model->model_data_b[38] != NULL) {
        packet.requested_animation = 38;
    }
}

void Animate_JEDI(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    bool check_movement_animation = false;

    if ((object->field_0xe23 & GAMEOBJECT_E23_FLAG_FORCE_WEAPON_IDLE) != 0) {
        if (((object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 || object->field_0xe32 == 1) &&
            object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_WEAPON_IDLE] != NULL) {
            packet.requested_animation = CHARACTER_ANIMATION_ALT_WEAPON_IDLE;
        } else {
            packet.requested_animation = CHARACTER_ANIMATION_WEAPON_IDLE;
        }
    } else if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;

        if (object->field_0x7a5 != CHARACTER_CONTEXT_DOOMED) {
            bool use_default_idle = object->apiobj.field_0x27d != 0;
            if (!use_default_idle) {
                const bool has_fall_animation =
                    object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL;
                // Original 0x16c978/0x16c9ce joins the character-data check
                // at 0x16c7bc even when the ground-contact timer has expired.
                if (object->ground_contact_grace_timer > 0.0f || !has_fall_animation ||
                    (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                     object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f)) {
                    const GAMECHARACTERDATA *game_character =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    use_default_idle = !(game_character->field_0x28 > 0.0f) || !has_fall_animation;
                }
            }
            if (use_default_idle) {
                packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
            }
        }

        if (object->field_0x7a5 == CHARACTER_CONTEXT_JUMP) {
            JumpAnimCode(object);
        } else if (UseFallAnim(object)) {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
        } else if (object->field_0x7a5 == CHARACTER_CONTEXT_FORCE_PUSH) {
            if ((object->action_flags & GAMEOBJECT_ACTION_FLAG_FORCE_PUSH_WEAPON_IDLE_MASK) != 0) {
                if (((object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 || object->field_0xe32 == 1) &&
                    object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_WEAPON_IDLE] != NULL) {
                    packet.requested_animation = CHARACTER_ANIMATION_ALT_WEAPON_IDLE;
                } else {
                    packet.requested_animation = CHARACTER_ANIMATION_WEAPON_IDLE;
                }
            } else {
                packet.requested_animation =
                    object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_WEAPON_IDLE] != NULL
                        ? CHARACTER_ANIMATION_ALT_WEAPON_IDLE
                        : CHARACTER_ANIMATION_WEAPON_IDLE;
            }
        } else if (object->field_0x7a5 == CHARACTER_CONTEXT_FORCE_DEFLECT ||
                   object->field_0x7a5 == CHARACTER_CONTEXT_FORCE_THROW ||
                   object->field_0x7a5 == CHARACTER_CONTEXT_FORCE) {
            if (((object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 || object->field_0xe32 == 1) &&
                object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_WEAPON_IDLE] != NULL) {
                packet.requested_animation = CHARACTER_ANIMATION_ALT_WEAPON_IDLE;
            } else {
                packet.requested_animation = CHARACTER_ANIMATION_WEAPON_IDLE;
            }
        } else if (packet.requested_animation != CHARACTER_ANIMATION_FALL) {
            GAMEPAD_s *pad = object->pad_gamepad;
            if ((pad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 && pad->input_magnitude > 0.0f) {
                if (object->id == id_YODA || object->id == id_YODAGHOST) {
                    packet.requested_animation = CHARACTER_ANIMATION_SABER_TIPTOE;
                } else {
                    const GAMECHARACTERDATA *game_character =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    const i32 allow_tiptoe =
                        (game_character->flags_090 & GAMECHARACTER_FLAG_DISABLE_TIPTOE) == 0 ? 1 : 0;
                    MoveAnim_Manage(object, pad->input_magnitude, allow_tiptoe, 1);
                }
            } else if (((object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 ||
                        object->field_0xe32 == 1) &&
                       object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_IDLE] != NULL) {
                packet.requested_animation = CHARACTER_ANIMATION_ALT_IDLE;
            }
        }

        if (UsingExtraActionsFn != NULL && UsingExtraActionsFn(object) != 0 &&
            packet.requested_animation <= CHARACTER_ANIMATION_FALL) {
            switch (packet.requested_animation) {
                case CHARACTER_ANIMATION_WALK:
                    packet.requested_animation = CHARACTER_ANIMATION_EXTRA_WALK;
                    break;
                case CHARACTER_ANIMATION_IDLE:
                    packet.requested_animation = CHARACTER_ANIMATION_EXTRA_IDLE;
                    break;
                case 2:
                    break;
                case CHARACTER_ANIMATION_RUN:
                    packet.requested_animation = CHARACTER_ANIMATION_EXTRA_RUN;
                    break;
                case CHARACTER_ANIMATION_TIPTOE:
                    packet.requested_animation = CHARACTER_ANIMATION_EXTRA_TIPTOE;
                    break;
                case CHARACTER_ANIMATION_FALL:
                    packet.requested_animation = CHARACTER_ANIMATION_EXTRA_FALL;
                    break;
            }
        }
        check_movement_animation = true;
    }

    if (check_movement_animation) {
        MoveAnim_Check(object);
    }
    UpdateCharacterIdle(object);

    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }

    if (object->id == id_IMPERIALGUARD || object->weapon_scale <= 0.0f) {
        return;
    }

    const char *loop_sfx;
    if (object->id == id_BODYGUARD) {
        loop_sfx = "Grv_GuardWeaponLp";
    } else if ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_JEDI_BADDIE) != 0) {
        loop_sfx = "SaberLoopB";
    } else {
        loop_sfx = "SaberLoopJ";
    }
    PlaySfxByIdAndSetVolume(GetSfxId(loop_sfx), &object->apiobj.collision_position, object->weapon_scale);
}

static void MoveAnim_Manage(GameObject_s *object, f32 movement_speed, i32 allow_tiptoe, i32 weapon_variant) {
    GAMECHARACTERDATA *game_character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    CHARACTERMODEL_s *model = object->apiobj.character_model;

    const f32 walk_run_threshold = (game_character->walk_speed + game_character->run_speed) * 0.5f;
    const bool use_weapon_locomotion =
        weapon_variant != 0 && (object->weapon_scale == 0.0f || object->weapon_scale_state == WEAPON_SCALE_EXTENDING);

    CHARACTER_ANIMATION animation;
    if (allow_tiptoe != 0 && movement_speed <= (game_character->tiptoe_speed + game_character->walk_speed) * 0.5f) {
        animation = use_weapon_locomotion && model->model_data_b[CHARACTER_ANIMATION_SABER_TIPTOE] != NULL
                        ? CHARACTER_ANIMATION_SABER_TIPTOE
                        : CHARACTER_ANIMATION_TIPTOE;
    } else if (movement_speed <= walk_run_threshold) {
        if (use_weapon_locomotion && model->model_data_b[CHARACTER_ANIMATION_SABER_WALK] != NULL) {
            animation = CHARACTER_ANIMATION_SABER_WALK;
        } else if (model->model_data_b[CHARACTER_ANIMATION_BACKWARDS] != NULL &&
                   (object->field_0xefd & GAMEOBJECT_MOVEMENT_FLAG_BACKWARDS) != 0) {
            animation = CHARACTER_ANIMATION_BACKWARDS;
        } else {
            animation = CHARACTER_ANIMATION_WALK;
        }
    } else {
        animation = use_weapon_locomotion && model->model_data_b[CHARACTER_ANIMATION_SABER_RUN] != NULL
                        ? CHARACTER_ANIMATION_SABER_RUN
                        : CHARACTER_ANIMATION_RUN;
    }
    object->apiobj.anim_packet.requested_animation = animation;

    const SUIT_s *suit = static_cast<const SUIT_s *>(object->suit);
    if (suit != NULL && (suit->store_flag & SUIT_STORE_FLAG_EXTRA_MOVEMENT_ANIMATIONS) != 0 &&
        (object->movement_context_state & 0x00ffff00) != 0x00054300) {
        if (animation == CHARACTER_ANIMATION_TIPTOE && model->model_data_b[CHARACTER_ANIMATION_SUIT_TIPTOE] != NULL) {
            animation = CHARACTER_ANIMATION_SUIT_TIPTOE;
        } else if (animation == CHARACTER_ANIMATION_WALK &&
                   model->model_data_b[CHARACTER_ANIMATION_SUIT_WALK] != NULL) {
            animation = CHARACTER_ANIMATION_SUIT_WALK;
        } else if (animation == CHARACTER_ANIMATION_RUN && model->model_data_b[CHARACTER_ANIMATION_SUIT_RUN] != NULL) {
            animation = CHARACTER_ANIMATION_SUIT_RUN;
        }
        object->apiobj.anim_packet.requested_animation = animation;
    }

    // The target applies this bounded fallback exactly three times. Keeping
    // the passes explicit preserves its finite walk/run alternation when a
    // model supplies none of the ordinary locomotion clips.
    if (model->model_data_b[animation] == NULL) {
        if (animation == CHARACTER_ANIMATION_TIPTOE) {
            animation = CHARACTER_ANIMATION_WALK;
        } else if (animation == CHARACTER_ANIMATION_WALK) {
            animation = CHARACTER_ANIMATION_RUN;
        } else if (animation == CHARACTER_ANIMATION_RUN) {
            animation = CHARACTER_ANIMATION_WALK;
        } else {
            return;
        }
        object->apiobj.anim_packet.requested_animation = animation;
    }
    if (model->model_data_b[animation] == NULL) {
        if (animation == CHARACTER_ANIMATION_TIPTOE) {
            animation = CHARACTER_ANIMATION_WALK;
        } else if (animation == CHARACTER_ANIMATION_WALK) {
            animation = CHARACTER_ANIMATION_RUN;
        } else if (animation == CHARACTER_ANIMATION_RUN) {
            animation = CHARACTER_ANIMATION_WALK;
        } else {
            return;
        }
        object->apiobj.anim_packet.requested_animation = animation;
    }
    if (model->model_data_b[animation] == NULL) {
        if (animation == CHARACTER_ANIMATION_TIPTOE) {
            animation = CHARACTER_ANIMATION_WALK;
        } else if (animation == CHARACTER_ANIMATION_WALK) {
            animation = CHARACTER_ANIMATION_RUN;
        } else if (animation == CHARACTER_ANIMATION_RUN) {
            animation = CHARACTER_ANIMATION_WALK;
        } else {
            return;
        }
        object->apiobj.anim_packet.requested_animation = animation;
    }
}

void AnimatePlayer(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    packet.previous_animation = packet.animation_index;
    object->mini_animation.previous_animation = object->mini_animation.animation_index;
    GAMEPAD_s *pad = object->pad_gamepad;
    CHARACTERMODEL_s *model = object->apiobj.character_model;

    if ((object->apiobj.field_0x1f4 & APIOBJECT_STATE_FLAG_IGNORE_DOORS) == 0) {
        object->apiobj.character_data->animate_fn(object);
    }

    const i16 override_from = object->ai.animation_override_from;
    if (override_from != -1 &&
        ((override_from == 0xe9 && object->character_context != 0x1c) || override_from == packet.requested_animation)) {
        packet.requested_animation = object->ai.animation_override_to;
    }

    if (model == NULL ||
        (object->apiobj.field_0x287 != 0 && (object->field_0x1018 == 0.0f || object->apiobj.field_0x287 == 1))) {
        return;
    }

    const f32 direction = (object->field_0xefd & GAMEOBJECT_MOVEMENT_FLAG_BACKWARDS) != 0 ? -1.0f : 1.0f;
    f32 movement_speed;
    if (object->apiobj.character_model->model_data_b[1] != NULL) {
        movement_speed = pad->animation_input_magnitude;
        if (movement_speed > 0.0f && object->id == id_GONKDROID && Cheat_IsOn(8) == 0)
            movement_speed = object->apiobj.character_data->game_character->walk_speed;
    } else if ((object->apiobj.character_data->game_character->flags_090 &
                GAMECHARACTER_FLAG_ANIMATION_SPEED_FROM_VELOCITY) != 0) {
        const f32 forward_speed = object->apiobj.velocity.x * object->facing_direction.x +
                                  object->apiobj.velocity.z * object->facing_direction.z;
        movement_speed = forward_speed < 0.0f ? 0.0f : forward_speed;
    } else {
        movement_speed = pad->input_magnitude;
    }

    // The original passes signed movement into UpdateAnimPacket.  Clips with
    // CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT (including the acrobatic
    // jump clips) start at their end frame and play backwards while the
    // character is travelling backwards.
    f32 time_multiplier = 1.0f;
    if (object->character_context == 0x2d && object->field_0x788 != NULL)
        time_multiplier = GizBuildItMul(object);
    UpdateAnimPacket(model, &packet, (FRAMETIME * 30.0f) * time_multiplier, movement_speed * direction,
                     time_multiplier * FRAMETIME,
                     object->apiobj.character_data->game_character->backwards_speed_multiplier);
    if ((packet.flags & ANIMPACKET_FLAG_ANIMATION_CHANGED) != 0 &&
        (packet.blending != 0 ? packet.blend_animation_b : packet.animation_index) == 0x5f) {
        f32 *time = packet.blending != 0 ? &packet.blend_target_time : &packet.current_time;
        const f32 duration = AnimDuration(object->id, 0x5f, 0.0f, 0.0f, 0);
        i32 count = static_cast<i32>(duration / 0.3f);
        if (NuFmod(duration, 3.0f) > 0.15f)
            ++count;
        const i32 phase = qrand() / (0xffff / count + 1);
        CHARACTERMODEL_s *current_model = object->apiobj.character_model;
        const f32 start =
            static_cast<f32>(phase) *
                (0.3f * static_cast<CHARACTERANIM_s *>(current_model->model_data_a[0x5f])->playback_rate) +
            1.0f;
        if (NuAnimEndFrame(current_model->model_data_b[0x5f]) > start)
            *time = start;
        object->field_0xe21 = (object->field_0xe21 & ~0x40) | ((phase & 1) << 6);
    }
    AutoWeaponOnOff(object);
    AddFootSteps(object);
}

void Animate_BEAST(GameObject_s *) {
}

void Animate_BARMAN(GameObject_s *object) {
    GAMEPAD_s *pad = object->pad_gamepad;
    object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_IDLE;
    if ((pad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 && pad->input_magnitude > 0.0f) {
        object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_WALK;
    }
    UpdateCharacterIdle(object);
}

void Animate_CANNON(GameObject_s *object) {
    object->apiobj.anim_packet.requested_animation =
        (CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0
            ? object->context_animation
            : CHARACTER_ANIMATION_IDLE;
}

void Animate_WALKER(GameObject_s *object) {
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        object->apiobj.anim_packet.requested_animation = object->context_animation;
        return;
    }

    object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_IDLE;
    if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
        object->pad_gamepad->input_magnitude > 0.0f) {
        object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_WALK;
    }
}

void Animate_WEIRDO(GameObject_s *) {
}

void Animate_CRITTER(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
        return;
    }

    if (object->character_context == 30) {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
        if (object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL) {
            return;
        }
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
    }

    if (object->character_context != CHARACTER_CONTEXT_DOOMED) {
        bool use_default_idle = object->apiobj.field_0x27d != 0;
        if (!use_default_idle) {
            if (object->ground_contact_grace_timer > 0.0f) {
                const GAMECHARACTERDATA *game_character = GetGameCharacterData(object);
                use_default_idle = game_character->field_0x28 <= 0.0f ||
                                   object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL;
            } else if (object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL) {
                use_default_idle = true;
            } else if (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                       object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f) {
                use_default_idle = true;
            }
        }
        if (use_default_idle) {
            packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
        }
    }

    if (UseFallAnim(object)) {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
    } else if (packet.requested_animation != CHARACTER_ANIMATION_FALL &&
               (object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
               object->pad_gamepad->input_magnitude > 0.0f) {
        const bool has_walk = object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_WALK] != NULL;
        const bool has_run = object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_RUN] != NULL;
        if (has_run && has_walk) {
            const GAMECHARACTERDATA *game_character = GetGameCharacterData(object);
            const f32 run_threshold = (game_character->walk_speed + game_character->run_speed) * 0.5f;
            packet.requested_animation = run_threshold < object->pad_gamepad->input_magnitude
                                             ? CHARACTER_ANIMATION_RUN
                                             : CHARACTER_ANIMATION_WALK;
        } else if (has_run) {
            packet.requested_animation = CHARACTER_ANIMATION_RUN;
        } else if (has_walk) {
            packet.requested_animation = CHARACTER_ANIMATION_WALK;
        }
    }
    MoveAnim_Check(object);
}

void Animate_DEFAULT(GameObject_s *object) {
    object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_IDLE;
}

void Animate_VEHICLE(GameObject_s *) {
}

void Animate_DROIDEKA(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
        if (object->character_context != CHARACTER_CONTEXT_DOOMED) {
            if (object->apiobj.field_0x27d != 0) {
                packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
            } else if (object->ground_contact_grace_timer > 0.0f) {
                const GAMECHARACTERDATA *game_character =
                    static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                if (game_character->field_0x28 <= 0.0f ||
                    object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL) {
                    packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
                }
            } else if (object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL ||
                       (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                        object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f)) {
                const GAMECHARACTERDATA *game_character =
                    static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                if (game_character->field_0x28 <= 0.0f ||
                    object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL) {
                    packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
                }
            }
        }

        if (UseFallAnim(object) || (object->character_context == -1 && object->apiobj.field_0x27d == 0)) {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
        } else if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
                   object->pad_gamepad->input_magnitude > 0.0f) {
            const GAMECHARACTERDATA *game_character = GetGameCharacterData(object);
            const f32 walk_threshold = (game_character->tiptoe_speed + game_character->walk_speed) * 0.5f;
            packet.requested_animation = object->pad_gamepad->input_magnitude > walk_threshold
                                             ? CHARACTER_ANIMATION_WALK
                                             : CHARACTER_ANIMATION_TIPTOE;
        }
        MoveAnim_Check(object);
    }

    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

void Animate_PROTOCOL(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
        goto final_animation;
    }
    if (object->context_target_position != NULL) {
        packet.requested_animation = static_cast<i16>(SetProtocolDroidFallAnim(object));
        goto final_animation;
    }
    packet.requested_animation = CHARACTER_ANIMATION_FALL;
    if (object->character_context == CHARACTER_CONTEXT_DOOMED)
        goto falling;
    if (object->apiobj.field_0x27d != 0)
        goto idle;
    if (object->ground_contact_grace_timer > 0.0f)
        goto check_fall;
    if (object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL)
        goto check_fall;
    if (!(object->fall_animation_timer < 0.2f) || object->nearby_floor_distance == 2000000.0f ||
        !(object->nearby_floor_distance < 0.25f) || !(object->apiobj.velocity.y < 0.0f))
        goto falling;
check_fall:
    if (!(GetGameCharacterData(object)->field_0x28 <= 0.0f) &&
        object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL)
        goto falling;
idle:
    packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
    if (packet.requested_animation == CHARACTER_ANIMATION_FALL)
        goto falling;
    switch (object->field_0xe38) {
        case 1:
            packet.requested_animation = 8;
            break;
        case 2:
            packet.requested_animation = 20;
            break;
        case 3:
            packet.requested_animation = 15;
            break;
        default:
            packet.requested_animation = 1;
            break;
    }
    if (UseFallAnim(object)) {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
    } else if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
               object->pad_gamepad->input_magnitude > 0.0f) {
        switch (object->field_0xe38) {
            case 1:
                packet.requested_animation = 29;
                break;
            case 2:
                packet.requested_animation = 23;
                break;
            case 3:
                packet.requested_animation = 3;
                break;
            default:
                packet.requested_animation = 0;
                break;
        }
    }
    goto final_animation;
falling:
    packet.requested_animation = static_cast<i16>(SetProtocolDroidFallAnim(object));
    if (UseFallAnim(object))
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
final_animation:
    if (packet.requested_animation == CHARACTER_ANIMATION_FALL)
        packet.requested_animation = static_cast<i16>(SetProtocolDroidFallAnim(object));
    UpdateCharacterIdle(object);
    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

// Original @0x4a9900.
void GameAnimSet_Draw(GAMEANIMSET_s &set) {
    for (GAMEANIMOBJ_s *object = set.objects; object != NULL; object = object->next) {
        if ((object->flags & 2) == 0 && NuSpecialGetVisibilityFn(&object->special) != 0) {
            NuSpecialDrawAt(&object->special, NuSpecialGetDrawMtx(&object->special));
        }
    }
}

i32 GameAnimSet_Play(GAMEANIMSET_s *set, float speed, i32 evaluate_state) {
    if (set == NULL) {
        return 1;
    }

    if (evaluate_state != 0) {
        GameAnimSet_EvaluateState(set);
    }
    set->flags = static_cast<GAMEANIMSET_FLAGS>(set->flags & ~GAMEANIMSET_FLAG_STOP_REQUESTED);

    if (speed >= 0.0f) {
        if (set->state == GAMEANIMSET_STATE_AT_END) {
            return 1;
        }
    } else if (speed < 0.0f) {
        if (set->state == GAMEANIMSET_STATE_AT_START) {
            return 1;
        }
    }

    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        nuinstanim_s *animation = object->instance_animation;
        if (animation == NULL) {
            continue;
        }

        animation->playing = 1;
        animation->waiting = 0;

        f32 direction;
        if (object->end_frame < object->start_frame) {
            direction = -1.0f;
        } else {
            direction = 1.0f;
        }
        f32 current_frame = animation->ltime * direction;
        if (object->start_frame * direction > current_frame) {
            animation->ltime = object->start_frame;
        } else if (current_frame > object->end_frame * direction) {
            animation->ltime = object->end_frame;
        }

        if (animation->fparam1 != 0.0f) {
            animation->tfactor = animation->fparam1 * speed * direction;
        } else {
            animation->tfactor = direction * speed;
        }
    }

    if ((set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) == 0) {
        GameAnimSet_AddToSystemList(set);
    }
    return 1;
}

i32 GameAnimSet_Stop(GAMEANIMSET_s *set) {
    if (set == NULL) {
        return 1;
    }

    GAMEANIMSET_STATE state = static_cast<GAMEANIMSET_STATE>(set->state & ~GAMEANIMSET_STATE_AT_END);
    if (state != GAMEANIMSET_STATE_ACTIVE_FORWARD) {
        return 1;
    }

    set->flags = static_cast<GAMEANIMSET_FLAGS>(set->flags | GAMEANIMSET_FLAG_STOP_REQUESTED);
    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if (object->instance_animation != NULL) {
            object->instance_animation->playing = 0;
        }
    }
    return 1;
}

void Animate_ASTROMECH(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) == 0) {
        if (object->context_target_position != NULL) {
            if (object->apiobj.character_model->model_data_b[43] != NULL) {
                packet.requested_animation = 43;
            } else {
                const i32 target_state =
                    *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(object->context_target_position) + 0x14);
                packet.requested_animation = target_state == 0 ? CHARACTER_ANIMATION_IDLE : CHARACTER_ANIMATION_FALL;
            }
        } else {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
            if (object->character_context != CHARACTER_CONTEXT_DOOMED) {
                if (object->apiobj.field_0x27d != 0) {
                    packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
                } else if (object->ground_contact_grace_timer > 0.0f) {
                    const GAMECHARACTERDATA *game_character =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    if (game_character->field_0x28 <= 0.0f ||
                        object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL) {
                        packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
                    }
                } else if (object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL ||
                           (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                            object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f)) {
                    const GAMECHARACTERDATA *game_character =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    if (game_character->field_0x28 <= 0.0f ||
                        object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL) {
                        packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
                    }
                }
            }

            if (object->character_context == 0) {
                packet.requested_animation = object->pad_gamepad->input_magnitude > 0.0f ? 37 : 36;
            } else if (UseFallAnim(object)) {
                packet.requested_animation = CHARACTER_ANIMATION_FALL;
            } else if (packet.requested_animation != CHARACTER_ANIMATION_FALL &&
                       (object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
                       object->pad_gamepad->input_magnitude > 0.0f) {
                packet.requested_animation = CHARACTER_ANIMATION_WALK;
            }
        }
    } else {
        packet.requested_animation = object->context_animation;
    }

    UpdateCharacterIdle(object);
    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

void Animate_CHARACTER(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    bool check_movement_animation = false;

    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else if (object->context_target_position != NULL) {
        if (object->apiobj.character_model->model_data_b[43] != NULL) {
            packet.requested_animation = 43;
        } else {
            const i32 target_state =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(object->context_target_position) + 0x14);
            packet.requested_animation = target_state == 0 ? CHARACTER_ANIMATION_IDLE : CHARACTER_ANIMATION_FALL;
        }
    } else if (object->character_context == CHARACTER_CONTEXT_FORCE) {
        packet.requested_animation = CHARACTER_ANIMATION_WEAPON_IDLE;
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;

        if (object->character_context != CHARACTER_CONTEXT_DOOMED) {
            bool use_default_idle = object->apiobj.field_0x27d != 0;
            if (!use_default_idle) {
                const bool has_fall_animation =
                    object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL;
                if (object->ground_contact_grace_timer > 0.0f) {
                    const GAMECHARACTERDATA *game_character =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    use_default_idle = game_character->field_0x28 <= 0.0f || !has_fall_animation;
                } else if (!has_fall_animation) {
                    use_default_idle = true;
                } else if (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                           object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f) {
                    use_default_idle = true;
                }
            }
            if (use_default_idle) {
                packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
            }
        }

        if (object->character_context == CHARACTER_CONTEXT_JUMP) {
            JumpAnimCode(object);
        } else if (UseFallAnim(object)) {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
        } else if (object->character_context == CHARACTER_CONTEXT_DOOMED) {
            // The doomed context retains the fall choice unless the model's
            // context handler supplied another action above.
        } else if (packet.requested_animation != CHARACTER_ANIMATION_FALL) {
            GAMEPAD_s *pad = object->pad_gamepad;
            if ((pad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 && pad->input_magnitude > 0.0f) {
                const GAMECHARACTERDATA *game_character =
                    static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                if ((object->apiobj.field_0x1f8 & 0x80) == 0 && game_character->field275_0x116 == 1) {
                    MoveAnim_Manage(object, pad->input_magnitude, 0, 1);
                } else {
                    const i32 allow_tiptoe =
                        (game_character->flags_090 & GAMECHARACTER_FLAG_DISABLE_TIPTOE) == 0 ? 1 : 0;
                    MoveAnim_Manage(object, pad->input_magnitude, allow_tiptoe, 1);
                }
            } else if (((object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 ||
                        object->field_0xe32 == 1) &&
                       object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_IDLE] != NULL) {
                packet.requested_animation = CHARACTER_ANIMATION_ALT_IDLE;
            }
        }
        check_movement_animation = true;
    }

    if (check_movement_animation) {
        MoveAnim_Check(object);
    }
    UpdateCharacterIdle(object);

    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

void Animate_GEONOSIAN(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
        if (object->character_context != CHARACTER_CONTEXT_DOOMED &&
            (object->apiobj.field_0x27d != 0 ||
             ((object->ground_contact_grace_timer > 0.0f ||
               object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL ||
               (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f)) &&
              !(static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->field_0x28 > 0.0f &&
                object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL)))) {
            packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
        }

        if (UseFallAnim(object)) {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
        } else if (object->character_context == -1 && object->field_0xe31 == 1) {
            packet.requested_animation = object->pad_gamepad->input_magnitude > 0.0f ? 37 : 15;
        } else if (packet.requested_animation != CHARACTER_ANIMATION_FALL &&
                   (object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
                   object->pad_gamepad->input_magnitude > 0.0f) {
            packet.requested_animation = CHARACTER_ANIMATION_WALK;
        }
        MoveAnim_Check(object);
    }

    UpdateCharacterIdle(object);
    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_40 || animation == CHARACTER_ANIMATION_FALL_VARIANT_75 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

i32 GameAnimSet_Reset(GAMEANIMSET_s *set) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            nuinstanim_s *animation = object->instance_animation;
            if (animation != NULL) {
                animation->playing = 0;
                animation->waiting = 0;
                animation->tfactor = 1.0f;
                animation->ltime = object->start_frame;
            }
            GameAnimSet_RemoveFromSystemList(set);
        }
    }
    return 1;
}

void Animate_HOVERDROID(GameObject_s *object) {
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        object->apiobj.anim_packet.requested_animation = object->context_animation;
    } else if (object->character_context == 0x1e &&
               object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL) {
        object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_FALL;
    } else {
        object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_IDLE;
    }
}

GAMEANIMSET_s *GameAnimSet_Create(variptr_u *buf, variptr_u *buf_end, GAMEANIMOBJPOOL_s *object_pool,
                                  GAMEANIMSYS_s *system) {
    GAMEANIMSET_s *set = NULL;
    if (object_pool != NULL) {
        set = static_cast<GAMEANIMSET_s *>(GameBufferAlloc(buf, buf_end, sizeof(GAMEANIMSET_s)));
        if (set != NULL) {
            set->object_pool = object_pool;
            set->system = system;
            i32 index = system->set_count;
            if (index < gameanimsysprogress.entry_size) {
                system->sets[index] = set;
                system->set_count = index + 1;
            }
        }
    }
    return set;
}

void Animate_BATTLEDROID(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else if (object->context_target_position != NULL) {
        if (object->apiobj.character_model->model_data_b[43] != NULL) {
            packet.requested_animation = 43;
        } else {
            packet.requested_animation = object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] == NULL
                                             ? CHARACTER_ANIMATION_IDLE
                                             : CHARACTER_ANIMATION_FALL;
        }
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
        if (object->character_context != CHARACTER_CONTEXT_DOOMED) {
            bool use_default_idle = object->apiobj.field_0x27d != 0;
            if (!use_default_idle) {
                const bool has_fall = object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL;
                if (object->ground_contact_grace_timer > 0.0f || !has_fall ||
                    (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                     object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f)) {
                    use_default_idle = GetGameCharacterData(object)->field_0x28 <= 0.0f || !has_fall;
                }
            }
            if (use_default_idle) {
                packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
            }
        }

        if (UseFallAnim(object)) {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
        } else if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
                   object->pad_gamepad->input_magnitude > 0.0f) {
            const GAMECHARACTERDATA *game_character = GetGameCharacterData(object);
            MoveAnim_Manage(object, object->pad_gamepad->input_magnitude,
                            (game_character->flags_090 & GAMECHARACTER_FLAG_DISABLE_TIPTOE) == 0 ? 1 : 0, 0);
        }
        MoveAnim_Check(object);
    }

    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

void Animate_SPEEDERBIKE(GameObject_s *object) {
    object->apiobj.anim_packet.requested_animation =
        (CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0
            ? object->context_animation
            : CHARACTER_ANIMATION_IDLE;
}

i32 GameAnimSet_Playing(GAMEANIMSET_s *set) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            if (object->instance_animation != NULL && object->instance_animation->playing == 0) {
                return 0;
            }
        }
    }
    return 1;
}

void GameAnimSet_EvalAnim(GAMEANIMSET_s *set) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            EvalAnim2(&object->special, object->instance_animation->ltime);
        }
    }
}

GAMEANIMOBJ_s *GameAnimSet_AddObject(GAMEANIMSET_s *set, nuhspecial_s *special, float start_frame, float end_frame,
                                     i32 append) {
    if (set == NULL || set->object_pool == NULL || set->object_pool->free_objects == NULL || special == NULL ||
        NuSpecialExistsFn(special) == 0) {
        return NULL;
    }

    GAMEANIMOBJPOOL_s *pool = set->object_pool;
    ++set->object_count;
    GAMEANIMOBJ_s *object = pool->free_objects;
    ++pool->active_count;
    pool->free_objects = object->next;

    if (append != 0) {
        object->next = NULL;
        if (set->objects == NULL) {
            set->objects = object;
        } else {
            GAMEANIMOBJ_s *tail = set->objects;
            while (tail->next != NULL) {
                tail = tail->next;
            }
            tail->next = object;
        }
    } else {
        object->next = set->objects;
        set->objects = object;
    }

    const i32 object_index = object - pool->objects;
    object->object_data = static_cast<u8 *>(pool->object_data) + pool->object_data_size * object_index;
    object->special = *special;
    object->instance_animation = NuSpecialGetInstAnim(&object->special);
    if (object->instance_animation == NULL) {
        return object;
    }

    object->animation = object->special.scene->instance_animation_data[object->instance_animation->anim_ix];
    const f32 last_frame = NuSpecialGetAnimEndFrame(&object->special);
    if (last_frame > 0.0f) {
        if (last_frame < end_frame) {
            object->end_frame = last_frame;
        } else {
            object->end_frame = end_frame;
            if (object->end_frame < 1.0f) {
                object->end_frame = 1.0f;
            }
        }
        if (start_frame > last_frame) {
            object->start_frame = last_frame;
        } else {
            object->start_frame = start_frame;
            if (object->start_frame < 1.0f) {
                object->start_frame = 1.0f;
            }
        }
        ++set->animated_object_count;
    } else {
        NuSpecialGetName(&object->special);
    }
    return object;
}

i32 GameAnimSet_JumpToEnd(GAMEANIMSET_s *set) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            if (object->instance_animation != NULL) {
                object->instance_animation->playing = 0;
                object->instance_animation->ltime = object->end_frame;
            }
        }
    }
    return 1;
}

void GameAnimSet_SetOffset(GAMEANIMSET_s *set, NUVEC *offset) {
    if (set == NULL) {
        return;
    }

    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if ((object->flags & 4) != 0) {
            continue;
        }
        NUMTX *source = NuSpecialGetMtx(&object->special);
        if (source == NULL) {
            continue;
        }

        NUMTX matrix = *source;
        matrix.m30 += offset->x;
        matrix.m31 += offset->y;
        matrix.m32 += offset->z;
        NuSpecialSetDrawMtx(&object->special, &matrix);
    }
}

f32 GameAnimSet_GetAnimPos(GAMEANIMOBJ_s *object) {
    if (object == NULL || object->instance_animation == NULL || object->animation == NULL) {
        return 0.0f;
    }

    if (object->start_frame == object->end_frame) {
        return 1.0f;
    }

    f32 position =
        (object->instance_animation->ltime - object->start_frame) / (object->end_frame - object->start_frame);
    if (position < 0.0f) {
        return 0.0f;
    }
    if (position > 1.0f) {
        position = 1.0f;
    }
    return position;
}

void GameAnimSet_SetAnimPos(GAMEANIMOBJ_s *object, float position) {
    if (object == NULL || object->instance_animation == NULL || object->animation == NULL) {
        return;
    }

    if (position < 0.0f) {
        position = 0.0f;
    }
    if (position > 1.0f) {
        position = 1.0f;
    }
    object->instance_animation->ltime = (object->end_frame - object->start_frame) * position + object->start_frame;
}

void GameAnimSet_SetTFactor(GAMEANIMSET_s *set, float factor) {
    if (set == NULL) {
        return;
    }

    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        nuinstanim_s *animation = object->instance_animation;
        if (animation != NULL) {
            f32 direction = object->end_frame < object->start_frame ? -1.0f : 1.0f;
            if (animation->fparam1 != 0.0f) {
                animation->tfactor = animation->fparam1 * factor * direction;
            } else {
                animation->tfactor = direction * factor;
            }
        }
    }
}

void Animate_REPUBLICGUNSHIP(GameObject_s *object) {
    object->apiobj.anim_packet.requested_animation = CHARACTER_ANIMATION_IDLE;
    if (object->character_context == 0x23) {
        object->apiobj.anim_packet.requested_animation = 0x2c;
    } else if (object->character_context == 0x24) {
        object->apiobj.anim_packet.requested_animation = 0x2d;
    }
    UpdateCharacterIdle(object);
}

i32 GameAnimSet_JumpToStart(GAMEANIMSET_s *set) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            if (object->instance_animation != NULL) {
                object->instance_animation->playing = 0;
                object->instance_animation->ltime = object->start_frame;
            }
        }
    }
    return 1;
}

void Animate_SUPERBATTLEDROID(GameObject_s *object) {
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    if ((CInfo[object->character_context].flags & CHARACTER_CONTEXT_INFO_FLAG_OWNS_ANIMATION) != 0) {
        packet.requested_animation = object->context_animation;
    } else if (object->context_target_position != NULL) {
        if (object->apiobj.character_model->model_data_b[43] != NULL) {
            packet.requested_animation = 43;
        } else {
            const i32 target_state =
                *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(object->context_target_position) + 0x14);
            packet.requested_animation = target_state == 0 ? CHARACTER_ANIMATION_IDLE : CHARACTER_ANIMATION_FALL;
        }
    } else {
        packet.requested_animation = CHARACTER_ANIMATION_FALL;
        if (object->character_context != CHARACTER_CONTEXT_DOOMED &&
            object->character_context != CHARACTER_CONTEXT_JUMP) {
            bool use_default_idle = object->apiobj.field_0x27d != 0;
            if (!use_default_idle) {
                const bool has_fall = object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_FALL] != NULL;
                if (object->ground_contact_grace_timer > 0.0f) {
                    const GAMECHARACTERDATA *game_character = GetGameCharacterData(object);
                    use_default_idle = game_character->field_0x28 <= 0.0f || !has_fall;
                } else if (!has_fall) {
                    use_default_idle = true;
                } else if (object->fall_animation_timer < 0.2f && object->nearby_floor_distance != 2000000.0f &&
                           object->nearby_floor_distance < 0.25f && object->apiobj.velocity.y < 0.0f) {
                    use_default_idle = true;
                }
            }
            if (use_default_idle) {
                packet.requested_animation = static_cast<i16>(GetDefaultIdle(object));
            }
        }

        if (UseFallAnim(object)) {
            packet.requested_animation = CHARACTER_ANIMATION_FALL;
        } else if (packet.requested_animation != CHARACTER_ANIMATION_FALL) {
            const GAMECHARACTERDATA *game_character = GetGameCharacterData(object);
            const bool weapon_out =
                (object->field_0xe22 & GAMEOBJECT_E22_FLAG_WEAPON_ANIMATION) != 0 || object->weapon_scale > 0.0f;
            if ((object->pad_gamepad->allocated_5a & GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT) == 0 &&
                object->pad_gamepad->input_magnitude > 0.0f) {
                const f32 run_threshold = (game_character->walk_speed + game_character->run_speed) * 0.5f;
                if (object->pad_gamepad->input_magnitude > run_threshold) {
                    packet.requested_animation = weapon_out ? CHARACTER_ANIMATION_SABER_RUN : CHARACTER_ANIMATION_RUN;
                } else {
                    packet.requested_animation = weapon_out ? CHARACTER_ANIMATION_SABER_WALK : CHARACTER_ANIMATION_WALK;
                }
            } else if (object->apiobj.character_model->model_data_b[CHARACTER_ANIMATION_ALT_IDLE] != NULL &&
                       object->weapon_scale >= 0.5f) {
                packet.requested_animation = CHARACTER_ANIMATION_ALT_IDLE;
            }
        }
        MoveAnim_Check(object);
    }

    UpdateCharacterIdle(object);
    const i16 animation = packet.requested_animation;
    if (animation == CHARACTER_ANIMATION_FALL ||
        ((object->apiobj.character_data->model_flags & CHARACTER_MODEL_FLAG_HIGH_JUMP) != 0 &&
         (animation == CHARACTER_ANIMATION_FALL_VARIANT_75 || animation == CHARACTER_ANIMATION_FALL_VARIANT_40 ||
          animation == CHARACTER_ANIMATION_FALL_VARIANT_76))) {
        object->fall_animation_timer += FRAMETIME;
    } else {
        object->fall_animation_timer = 0.0f;
    }
}

void GameAnimSet_RemoveObject(GAMEANIMSET_s *set, GAMEANIMOBJ_s *object) {
    if (object == NULL || set == NULL)
        return;
    if (set->objects == object) {
        set->objects = object->next;
    } else {
        GAMEANIMOBJ_s *previous = set->objects;
        while (previous != NULL && previous->next != object) {
            previous = previous->next;
        }
        if (previous != NULL)
            previous->next = object->next;
    }
    object->next = NULL;
    --set->object_count;
    GAMEANIMOBJPOOL_s *pool = set->object_pool;
    --pool->active_count;
    object->next = pool->free_objects;
    pool->free_objects = object;
}

void GameAnimSet_ScaleFParam1(GAMEANIMSET_s *set, float scale) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            if (object->instance_animation != NULL) {
                object->instance_animation->fparam1 *= scale;
            }
        }
    }
}

i32 GameAnimSet_SetRepeating(GAMEANIMSET_s *set, i32 repeating) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            if (object->instance_animation != NULL) {
                object->instance_animation->repeating = repeating & 1;
            }
        }
    }
    return 1;
}

void GameAnimSet_EvaluateState(GAMEANIMSET_s *set) {
    if ((set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) != 0) {
        return;
    }

    GAMEANIMOBJ_s *object = set->objects;
    i32 all_at_end = 1;
    i32 all_at_start = 1;

    while (object != NULL) {
        nuinstanim_s *animation = object->instance_animation;
        if (animation != NULL) {
            f32 direction = object->end_frame < object->start_frame ? -1.0f : 1.0f;
            f32 current_frame = animation->ltime * direction;
            if (object->end_frame * direction > current_frame) {
                all_at_end = 0;
            }
            if (current_frame > object->start_frame * direction) {
                all_at_start = 0;
            }
        }
        object = object->next;
    }

    set->state = GAMEANIMSET_STATE_AT_START;
    if (all_at_end != 0) {
        set->state = GAMEANIMSET_STATE_AT_END;
    } else if (all_at_start == 0) {
        set->state = GAMEANIMSET_STATE_BETWEEN_ENDPOINTS;
    }
}

i32 GameAnimSet_GetAveragePos(GAMEANIMSET_s *set, NUVEC *position, i32 frame_selection, i32 include_animated,
                              i32 include_static) {
    NUVEC sum = {0.0f, 0.0f, 0.0f};
    if (position == NULL || set == NULL || set->object_count == 0 || set->objects == NULL)
        return 0;
    i32 count = 0;
    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if ((object->flags & 1) != 0)
            continue;
        if (object->instance_animation != NULL) {
            if (include_animated == 0)
                continue;
            f32 frame;
            if (frame_selection == 0)
                frame = object->start_frame;
            else if (frame_selection == 1)
                frame = object->end_frame;
            else
                frame = object->instance_animation->ltime;
            NUMTX matrix;
            EvalAnim(&object->special, frame, &matrix, 1);
            NuVecAdd(&sum, &sum, NUMTX_GET_ROW_VEC(&matrix, 3));
            ++count;
        } else if (include_static != 0) {
            NuVecAdd(&sum, &sum, NuSpecialGetDrawPos(&object->special));
            ++count;
        }
    }
    if (count == 0)
        return 0;
    NuVecScale(position, &sum, 1.0f / static_cast<f32>(count));
    return 1;
}

GAMEANIMSET_VISIBILITY GameAnimSet_GetVisibility(GAMEANIMSET_s *set) {
    if (set == NULL) {
        return GAMEANIMSET_VISIBILITY_NONE;
    }

    i32 visible_count = 0;
    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if (NuSpecialGetVisibilityFn(&object->special) == 1) {
            ++visible_count;
        }
    }

    if (visible_count == set->object_count) {
        return GAMEANIMSET_VISIBILITY_ALL;
    }
    if (visible_count > 0) {
        return GAMEANIMSET_VISIBILITY_PARTIAL;
    }
    return GAMEANIMSET_VISIBILITY_NONE;
}

void GameAnimSet_JumpToAnimPos(GAMEANIMSET_s *set, float position) {
    if (set == NULL) {
        return;
    }

    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        nuinstanim_s *animation = object->instance_animation;
        if (animation == NULL || object->animation == NULL) {
            continue;
        }

        animation->playing = 0;
        f32 frame = (object->end_frame - object->start_frame) * position + object->start_frame;
        f32 direction = object->end_frame < object->start_frame ? -1.0f : 1.0f;
        f32 scaled_frame = frame * direction;
        f32 scaled_end = object->end_frame * direction;
        if (scaled_frame <= scaled_end) {
            animation->ltime = frame;
        } else {
            animation->ltime = object->end_frame;
            scaled_frame = scaled_end;
        }
        if (object->start_frame * direction > scaled_frame) {
            animation->ltime = object->start_frame;
        }
    }
}

void GameAnimSet_RemoveSpecial(GAMEANIMSET_s *set, nuhspecial_s *special) {
    if (special == NULL || set == NULL)
        return;
    GAMEANIMOBJ_s *object = set->objects;
    while (object != NULL && NuSpecialCompare(&object->special, special) == 0) {
        object = object->next;
    }
    if (object != NULL)
        GameAnimSet_RemoveObject(set, object);
}

void GameAnimSet_SetVisibility(GAMEANIMSET_s *set, i32 visibility) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            NuSpecialSetVisibility(&object->special, visibility);
        }
    }
}

void GameAnimSet_DrawReflection(GAMEANIMSET_s *set, i32 axis, float offset, numtx_s *matrix) {
    if (set == NULL || set->objects == NULL) {
        return;
    }
    if (matrix == NULL) {
        matrix = NuSpecialGetMtx(&set->objects->special);
    }
    f32 plane = offset + reinterpret_cast<f32 *>(matrix)[11 + axis];
    NuRndrStartReflectionRender(0);
    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if ((object->flags & 2) == 0 && NuSpecialGetVisibilityFn(&object->special) != 0) {
            NUMTX reflection __attribute__((aligned(16)));
            extern i32 MatrixReflection(NUMTX *, i32, f32, f32, NUMTX *);
            NUMTX *draw_matrix = NuSpecialGetDrawMtx(&object->special);
            if (MatrixReflection(draw_matrix, axis, plane, WORLD->current_level->unknown_0cc, &reflection) != 0) {
                NuSpecialDrawAt(&object->special, &reflection);
            }
        }
    }
    NuRndrEndReflectionRender();
}

GAMEANIMOBJ_s *GameAnimSet_AddObjectByName(GAMEANIMSET_s *set, nugscn_s *scene, char *name, float start_frame,
                                           float end_frame, i32 append, GIZMOSYS_s *gizmo_sys, char *prefix,
                                           char *suffix) {
    if (set == NULL) {
        return NULL;
    }

    nuhspecial_s special;
    if (Gizmo_FindNuSpecial(scene, &special, name, 1, gizmo_sys, prefix, suffix) == 0) {
        return NULL;
    }
    return GameAnimSet_AddObject(set, &special, start_frame, end_frame, append);
}

void GameAnimSet_AddToSystemList(GAMEANIMSET_s *set) {
    if (set != NULL && set->system != NULL && (set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) == 0) {
        NuLinkedListAppend(&set->system->active_sets, &set->links);
        set->flags = static_cast<GAMEANIMSET_FLAGS>(set->flags | GAMEANIMSET_FLAG_IN_SYSTEM_LIST);
    }
}

f32 GameAnimSet_AutoSetReflectY(GAMEANIMSET_s *set, nuvec_s *position, numtx_s *matrix) {
    if (set != NULL && set->objects != NULL) {
        if (matrix == NULL) {
            matrix = NuSpecialGetMtx(&set->objects->special);
        }
        f32 height = matrix->m31;
        f32 ground = GameShadow(NULL, position, 5.0f, -1);
        if (ground != 2000000.0f) {
            return ground - height;
        }
    }
    return 0.0f;
}

f32 GameAnimSet_GetCurrentFrame(GAMEANIMSET_s *set) {
    if (set != NULL) {
        for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
            if (object->instance_animation != NULL) {
                return object->instance_animation->ltime;
            }
        }
    }
    return 0.0f;
}

GAMEANIMOBJPOOL_s *GameAnimSet_CreateObjectPool(variptr_u *buf, variptr_u *buf_end, i32 object_data_size,
                                                i32 capacity) {
    GAMEANIMOBJPOOL_s *pool = NULL;
    if (capacity != 0) {
        pool = static_cast<GAMEANIMOBJPOOL_s *>(GameBufferAlloc(buf, buf_end, sizeof(GAMEANIMOBJPOOL_s)));
        if (pool != NULL) {
            pool->capacity = static_cast<u16>(capacity);
            pool->object_data_size = static_cast<u16>(object_data_size);
            pool->objects = static_cast<GAMEANIMOBJ_s *>(
                GameBufferAlloc(buf, buf_end, static_cast<u16>(capacity) * sizeof(GAMEANIMOBJ_s)));
            if (object_data_size != 0) {
                pool->object_data = GameBufferAlloc(
                    buf, buf_end, static_cast<u16>(pool->object_data_size) * static_cast<u16>(pool->capacity));
            }

            for (i32 i = 0; i < pool->capacity; ++i) {
                GAMEANIMOBJ_s *object = &pool->objects[i];
                object->next = pool->free_objects;
                pool->free_objects = object;
            }
        }
    }
    return pool;
}

i32 GameAnimSet_IsAnimationReset(GAMEANIMSET_s *set) {
    if (set == NULL || set->objects == NULL) {
        return 1;
    }

    i32 repeating_count = 0;
    i32 animated_count = 0;
    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        nuinstanim_s *animation = object->instance_animation;
        if (animation == NULL) {
            continue;
        }

        ++animated_count;
        if (animation->repeating != 0) {
            ++repeating_count;
            continue;
        }
        if (animation->playing != 0) {
            return 0;
        }

        const f32 direction = object->end_frame < object->start_frame ? -1.0f : 1.0f;
        if (animation->ltime * direction > object->start_frame * direction) {
            return 0;
        }
    }

    if (animated_count == 0) {
        return 1;
    }
    return animated_count != repeating_count;
}

void GameAnimSet_RemoveAllObjects(GAMEANIMSET_s *set) {
    if (set != NULL) {
        while (set->objects != NULL)
            GameAnimSet_RemoveObject(set, set->objects);
    }
}

i32 GameAnimSet_GetCentreAndRadius(GAMEANIMSET_s *set, NUVEC *centre, f32 *radius, i32 frame_selection,
                                   i32 include_animated, i32 include_static) {
    if (centre == NULL || set == NULL || set->object_count == 0 || set->objects == NULL) {
        return 0;
    }

    NUVEC minimum = {1.0e9f, 1.0e9f, 1.0e9f};
    NUVEC maximum = {-1.0e9f, -1.0e9f, -1.0e9f};
    bool found_object = false;

    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if ((object->flags & 1) != 0) {
            continue;
        }

        NUMTX matrix;
        if (object->instance_animation == NULL) {
            if (include_static == 0) {
                continue;
            }
            NUMTX *draw_matrix = NuSpecialGetDrawMtx(&object->special);
            if (draw_matrix == NULL) {
                continue;
            }
            matrix = *draw_matrix;
        } else {
            if (include_animated == 0) {
                continue;
            }
            f32 frame;
            if (frame_selection == 0) {
                frame = object->start_frame;
            } else if (frame_selection == 1) {
                frame = object->end_frame;
            } else {
                frame = object->instance_animation->ltime;
            }
            EvalAnim(&object->special, frame, &matrix, 1);
        }

        NUVEC object_centre;
        f32 object_radius;
        NuSpecialGetRadius(&object->special, &object_centre, &object_radius);
        object_radius *= 0.75f;
        NuVecMtxTransform(&object_centre, &object_centre, &matrix);

        const NUVEC object_minimum = {
            object_centre.x - object_radius,
            object_centre.y - object_radius,
            object_centre.z - object_radius,
        };
        const NUVEC object_maximum = {
            object_centre.x + object_radius,
            object_centre.y + object_radius,
            object_centre.z + object_radius,
        };
        if (object_minimum.x < minimum.x) {
            minimum.x = object_minimum.x;
        }
        if (object_minimum.y < minimum.y) {
            minimum.y = object_minimum.y;
        }
        if (object_minimum.z < minimum.z) {
            minimum.z = object_minimum.z;
        }
        if (object_maximum.x > maximum.x) {
            maximum.x = object_maximum.x;
        }
        if (object_maximum.y > maximum.y) {
            maximum.y = object_maximum.y;
        }
        if (object_maximum.z > maximum.z) {
            maximum.z = object_maximum.z;
        }
        found_object = true;
    }

    if (!found_object) {
        return 0;
    }

    centre->x = (minimum.x + maximum.x) * 0.5f;
    centre->y = (minimum.y + maximum.y) * 0.5f;
    centre->z = (minimum.z + maximum.z) * 0.5f;
    if (radius != NULL) {
        const f32 half_x = (maximum.x - minimum.x) * 0.5f;
        const f32 half_y = (maximum.y - minimum.y) * 0.5f;
        const f32 half_z = (maximum.z - minimum.z) * 0.5f;
        *radius = NuFsqrt(half_x * half_x + half_y * half_y + half_z * half_z);
    }
    return 1;
}

f32 GameAnimSet_GetCompletionRatio(GAMEANIMSET_s *set) {
    if (set == NULL) {
        return 0.0f;
    }
    for (GAMEANIMOBJ_s *object = set->objects; object != NULL; object = object->next) {
        if (object->instance_animation == NULL) {
            continue;
        }
        if (object->start_frame == object->end_frame) {
            return 1.0f;
        }
        f32 ratio =
            (object->instance_animation->ltime - object->start_frame) / (object->end_frame - object->start_frame);
        if (ratio > 1.0f) {
            ratio = 1.0f;
        }
        if (ratio < 0.0f) {
            ratio = 0.0f;
        }
        return ratio;
    }
    return 0.0f;
}

void GameAnimSet_RemoveFromSystemList(GAMEANIMSET_s *set) {
    if (set != NULL && set->system != NULL) {
        if ((set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) != 0) {
            NuLinkedListRemove(&set->system->active_sets, &set->links);
            set->flags = static_cast<GAMEANIMSET_FLAGS>(set->flags & ~GAMEANIMSET_FLAG_IN_SYSTEM_LIST);
        }
        GameAnimSet_EvaluateState(set);
    }
}

static __used__ i32 LoadAnim(char *, i32, variptr_u *, variptr_u) {
    return 0;
}
static __used__ i32 LoadAnimFromPAK(char *, i32, char *, i32) {
    return 0;
}
static __used__ void NormalizeAnimPath(char *) {
}

i32 ANI_SimpleAni3PlayerV4Joint_Blend_Quat3(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend,
                                            i32 joint_count, i32 first_joint, NUVEC *root_translation);
i32 ANI_SimpleAni3PlayerV4Joint_Blend_Quat3W(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend,
                                             i32 joint_count, i32 first_joint, NUVEC *root_translation);

extern "C" {

    i32 ANI_SimpleAni3PlayerV4Joint_EulerQuat(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                              i32 first_joint);
    void ANI_SimpleAni3PlayerV4Joint_Blend_EulerQuat(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer,
                                                     f32 blend, i32 joint_count, i32 first_joint,
                                                     NUVEC *root_translation);

    void ANI_Ani3ExtractAllNodeCurves(ani3_animheader_s *anim, float frame, float *values, i32 node, char *curve_mask) {
        u32 curve_count = anim->curve_count;
        u32 quarter;
        u32 stride = anim->key_stride;
        float fraction;
        i32 key_offset;

        if (ForcePlayEndFrame == 0 || anim->end_frame == 0) {
            if (anim->key_count == 1) {
                quarter = 0;
                fraction = 0.0f;
                key_offset = 0;
            } else {
                float last_key = static_cast<float>(anim->key_count - 1);
                float key = (frame - static_cast<float>(anim->first_frame)) * last_key /
                            static_cast<float>(anim->frame_count - 1);
                if (key < 0.0f) {
                    key = 0.0f;
                }
                if (last_key <= key) {
                    key = last_key;
                }
                i32 whole_key = static_cast<i32>(key);
                fraction = key - static_cast<float>(whole_key);
                quarter = static_cast<u32>(whole_key) & 3;
                key_offset = (whole_key >> 2) * stride;
            }
        } else {
            float key = static_cast<float>(anim->end_frame + anim->key_count - 4);
            i32 whole_key = static_cast<i32>(key);
            fraction = key - static_cast<float>(whole_key);
            quarter = static_cast<u32>(whole_key) & 3;
            key_offset = (whole_key >> 2) * stride;
        }

        u16 *types = anim->curve_types;
        u8 *force_zero = reinterpret_cast<u8 *>(types + anim->node_count * curve_count);
        ani3_scalemin_s *scale_min = anim->scale_min;
        i16 *constants = anim->constants;
        u8 *keys = anim->keys + key_offset;

        for (i32 n = 0; n < node; ++n) {
            for (u32 curve = 0; curve < curve_count; ++curve) {
                u16 type = *types++;
                if (type < 16) {
                    ++scale_min;
                    keys += KeyStructSizes[type];
                }
            }
            force_zero += curve_count;
        }

        i32 quarter_shift = static_cast<i32>(quarter) * 6;
        i32 next_quarter_shift = (static_cast<i32>(quarter) * 3 + 3) * 2;
        for (u32 curve = 0; curve < curve_count; ++curve, ++values) {
            u16 type = types[curve];
            bool evaluate = curve_mask == NULL || *curve_mask == static_cast<char>(curve);
            if (curve_mask != NULL && evaluate) {
                ++curve_mask;
            }
            if (!evaluate) {
                if (type == 7) {
                    keys += 8;
                    ++scale_min;
                } else if (type == 6) {
                    keys += 4;
                    ++scale_min;
                } else if (type == 8 || type == 10) {
                    keys += 4;
                }
                continue;
            }

            float key_fraction = (force_zero[curve] & 1) != 0 ? 0.0f : fraction;
            if (type == 7) {
                *values = CalcValue1648(reinterpret_cast<char *>(keys), quarter, stride, key_fraction, scale_min);
                keys += 8;
                ++scale_min;
            } else if (type == 8) {
                *values = static_cast<float>(constants[keys[quarter]]);
                keys += 4;
            } else if (type == 10) {
                u32 index = keys[quarter];
                const u32 packed = static_cast<u32>(static_cast<i32>(constants[index + 1])) |
                                   (static_cast<u32>(static_cast<i32>(constants[index])) << 16);
                memcpy(values, &packed, sizeof(packed));
                keys += 4;
            } else if (type == 6) {
                u32 first = *reinterpret_cast<u32 *>(keys);
                u32 next = *reinterpret_cast<u32 *>(keys + stride);
                float first_value = static_cast<float>(first & 0xff);
                float next_value = static_cast<float>(next & 0xff);
                u32 tangents = first >> 8;
                float tangent0 = static_cast<float>((tangents >> quarter_shift) & 0x3f) * 0.01587302f;
                float packed_value;
                if (quarter == 3) {
                    float interpolated = (next_value - first_value) * tangent0 + first_value;
                    float tangent1 = static_cast<float>((next >> 8) & 0x3f) * 0.01587302f;
                    float after = static_cast<float>(keys[stride * 2]);
                    packed_value =
                        (((after - next_value) * tangent1 + next_value) - interpolated) * key_fraction + interpolated;
                } else {
                    float tangent1 = static_cast<float>((tangents >> (next_quarter_shift & 0x1f)) & 0x3f) * 0.01587302f;
                    packed_value =
                        (next_value - first_value) * ((tangent1 - tangent0) * key_fraction + tangent0) + first_value;
                }
                *values = packed_value * scale_min->scale + scale_min->minimum;
                keys += 4;
                ++scale_min;
            } else {
                u16 constant = reinterpret_cast<u16 *>(constants)[anim->constant_index + type - 16];
                *values = static_cast<float>(constant) * anim->scale + anim->minimum;
            }
        }
    }

    void ANI_FixUpAddrs(ani3_animheader_s *anim, isize delta, i32) {
        if (anim->magic != 0x414e4934) {
            return;
        }
        while (true) {
            if (anim->constants != NULL) {
                anim->constants = reinterpret_cast<i16 *>(reinterpret_cast<usize>(anim->constants) + (usize)delta);
            }
            if (anim->scale_min != NULL) {
                anim->scale_min =
                    reinterpret_cast<ani3_scalemin_s *>(reinterpret_cast<usize>(anim->scale_min) + (usize)delta);
            }
            if (anim->keys != NULL) {
                anim->keys = reinterpret_cast<u8 *>(reinterpret_cast<usize>(anim->keys) + (usize)delta);
            }
            if (anim->curve_types != NULL) {
                anim->curve_types = reinterpret_cast<u16 *>(reinterpret_cast<usize>(anim->curve_types) + (usize)delta);
            }
            if (anim->node_flags != NULL) {
                anim->node_flags = reinterpret_cast<u8 *>(reinterpret_cast<usize>(anim->node_flags) + (usize)delta);
            }
            if (anim->field_38 != NULL) {
                anim->field_38 = reinterpret_cast<void *>(reinterpret_cast<usize>(anim->field_38) + (usize)delta);
            }
            u16 next = anim->next_block;
            if (next == 0) {
                break;
            }
            delta += next;
            anim = reinterpret_cast<ani3_animheader_s *>(reinterpret_cast<usize>(anim) + next);
        }
    }

    // Original @0x2c17d0. The non-quaternion ANI4 player uses a compact
    // four-samples-per-word curve stream. Only groups enabled by the node's
    // translation/rotation/scale flags occupy space in that stream.
    i32 ANI_SimpleAni3PlayerV4Joint(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                    i32 first_joint) {
        if ((anim->format_flags & ANI3_FORMAT_QUATERNION_ROTATION) != 0) {
            if ((anim->format_flags & ANI3_FORMAT_QUATERNION_STORES_W) != 0) {
                return ANI_SimpleAni3PlayerV4Joint_Quat3W(anim, frame, buffer, joint_count, first_joint);
            }
            return ANI_SimpleAni3PlayerV4Joint_Quat3(anim, frame, buffer, joint_count, first_joint);
        }
        if (ForceEulerToQuat != 0) {
            return ANI_SimpleAni3PlayerV4Joint_EulerQuat(anim, frame, buffer, joint_count, first_joint);
        }

        buffer->use_quaternions = 0;

        u32 quarter;
        f32 fraction;
        i32 key_offset;
        if (anim->key_count == 1) {
            quarter = 0;
            fraction = 0.0f;
            key_offset = 0;
        } else {
            const f32 last_key = static_cast<f32>(anim->key_count - 1);
            f32 key = (frame - anim->first_frame) * last_key / static_cast<f32>(anim->frame_count - 1);
            if (key < 0.0f) {
                key = 0.0f;
            }
            if (last_key <= key) {
                key = last_key;
            }

            const i32 whole_key = static_cast<i32>(key);
            fraction = key - static_cast<f32>(whole_key);
            quarter = static_cast<u32>(whole_key) & 3;
            key_offset = (whole_key >> 2) * anim->key_stride;
        }

        u8 *keys = anim->keys + key_offset;
        ani3_scalemin_s *scale_min = anim->scale_min;
        const u16 *curve_types = anim->curve_types;

        // Bring all three packed-data cursors to the requested first joint.
        for (i32 joint = 0; joint < first_joint; ++joint) {
            const u8 flags = anim->node_flags[joint];
            for (i32 group = 0; group < 3; ++group) {
                if ((flags & CurveGroupMasks[group]) == 0) {
                    continue;
                }
                for (i32 component = 0; component < 3; ++component) {
                    if (curve_types[group * 3 + component] < 16) {
                        keys += 4;
                        ++scale_min;
                    }
                }
            }
            curve_types += 9;
        }

        const i32 decode_count = joint_count <= anim->node_count ? joint_count : anim->node_count;
        const i32 end_joint = first_joint + decode_count;
        for (i32 joint_index = first_joint; joint_index < end_joint; ++joint_index) {
            const u8 flags = anim->node_flags[joint_index];
            buffer->joint_flags[joint_index] = flags;
            f32 *group_values = reinterpret_cast<f32 *>(&buffer->joints[joint_index]);

            for (i32 group = 0; group < 3; ++group) {
                if ((flags & CurveGroupMasks[group]) == 0) {
                    const f32 default_value = group == 2 ? 1.0f : 0.0f;
                    group_values[0] = default_value;
                    group_values[1] = default_value;
                    group_values[2] = default_value;
                } else {
                    group_values[0] = DecodeAni4V4Curve(anim, curve_types[0], quarter, fraction, keys, scale_min);
                    group_values[1] = DecodeAni4V4Curve(anim, curve_types[1], quarter, fraction, keys, scale_min);
                    group_values[2] = DecodeAni4V4Curve(anim, curve_types[2], quarter, fraction, keys, scale_min);
                }

                curve_types += 3;
                group_values += 4;
            }
        }
        return 0;
    }

    void ANI_SimpleAni3PlayerV4Joint_Blend(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend,
                                           i32 joint_count, i32 first_joint, NUVEC *root_translation) {
        if ((anim->format_flags & ANI3_FORMAT_QUATERNION_ROTATION) != 0) {
            if ((anim->format_flags & ANI3_FORMAT_QUATERNION_STORES_W) != 0) {
                ANI_SimpleAni3PlayerV4Joint_Blend_Quat3W(anim, frame, buffer, blend, joint_count, first_joint,
                                                         root_translation);
            } else {
                ANI_SimpleAni3PlayerV4Joint_Blend_Quat3(anim, frame, buffer, blend, joint_count, first_joint,
                                                        root_translation);
            }
            return;
        }
        if (buffer->use_quaternions != 0) {
            ANI_SimpleAni3PlayerV4Joint_Blend_EulerQuat(anim, frame, buffer, blend, joint_count, first_joint,
                                                        root_translation);
            return;
        }

        const f32 last_key = static_cast<f32>(anim->key_count - 1);
        f32 key = (frame - anim->first_frame) * last_key / static_cast<f32>(anim->frame_count - 1);
        if (key < 0.0f) {
            key = 0.0f;
        }
        if (last_key <= key) {
            key = last_key;
        }

        const i32 whole_key = static_cast<i32>(key);
        const u32 quarter = static_cast<u32>(whole_key) & 3;
        const f32 fraction = key - static_cast<f32>(whole_key);
        u8 *keys = anim->keys + (whole_key >> 2) * anim->key_stride;
        ani3_scalemin_s *scale_min = anim->scale_min;
        const u16 *curve_types = anim->curve_types;

        // Advance all packed stream cursors to the first requested joint.
        for (i32 joint = 0; joint < first_joint; ++joint) {
            const u8 flags = anim->node_flags[joint];
            if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
                SkipAni4V4Curve(curve_types[0], keys, scale_min);
                SkipAni4V4Curve(curve_types[1], keys, scale_min);
                SkipAni4V4Curve(curve_types[2], keys, scale_min);
            }
            if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
                SkipAni4V4Curve(curve_types[3], keys, scale_min);
                SkipAni4V4Curve(curve_types[4], keys, scale_min);
                SkipAni4V4Curve(curve_types[5], keys, scale_min);
            }
            if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
                SkipAni4V4Curve(curve_types[6], keys, scale_min);
                SkipAni4V4Curve(curve_types[7], keys, scale_min);
                SkipAni4V4Curve(curve_types[8], keys, scale_min);
            }
            curve_types += 9;
        }

        const i32 decode_count = joint_count <= anim->node_count ? joint_count : anim->node_count;
        const i32 end_joint = first_joint + decode_count;
        NUVEC *root = first_joint == 0 ? root_translation : NULL;
        const f32 inverse_blend = 1.0f - blend;

        for (i32 joint_index = first_joint; joint_index < end_joint; ++joint_index) {
            const u8 flags = anim->node_flags[joint_index];
            buffer->joint_flags[joint_index] |= flags;
            f32 *group_values = reinterpret_cast<f32 *>(&buffer->joints[joint_index]);

            for (i32 group = 0; group < 3; ++group) {
                if ((flags & CurveGroupMasks[group]) != 0) {
                    f32 decoded = DecodeAni4V4Curve(anim, curve_types[0], quarter, fraction, keys, scale_min);
                    f32 delta = decoded - group_values[0];
                    if (group == 1) {
                        delta = WrapAni4BlendRotation(delta);
                    }
                    if (root != NULL) {
                        root->x = decoded;
                    }
                    group_values[0] += delta * blend;

                    decoded = DecodeAni4V4Curve(anim, curve_types[1], quarter, fraction, keys, scale_min);
                    delta = decoded - group_values[1];
                    if (group == 1) {
                        delta = WrapAni4BlendRotation(delta);
                    }
                    if (root != NULL) {
                        root->y = decoded;
                    }
                    group_values[1] += delta * blend;

                    decoded = DecodeAni4V4Curve(anim, curve_types[2], quarter, fraction, keys, scale_min);
                    delta = decoded - group_values[2];
                    if (group == 1) {
                        delta = WrapAni4BlendRotation(delta);
                    }
                    if (root != NULL) {
                        root->z = -decoded;
                    }
                    group_values[2] += delta * blend;
                } else if (group == 2) {
                    group_values[0] = group_values[0] * inverse_blend + blend;
                    group_values[1] = group_values[1] * inverse_blend + blend;
                    group_values[2] = group_values[2] * inverse_blend + blend;
                } else {
                    group_values[0] *= inverse_blend;
                    group_values[1] *= inverse_blend;
                    group_values[2] *= inverse_blend;
                    if (root != NULL) {
                        root->x = 0.0f;
                        root->y = 0.0f;
                        root->z = 0.0f;
                    }
                }

                if (group == 0) {
                    root = NULL;
                }
                curve_types += 3;
                group_values += 4;
            }
        }
    }

    void ANI_SimpleAni3PlayerV4Joint_Blend_EulerQuat(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer,
                                                     f32 blend, i32 joint_count, i32 first_joint,
                                                     NUVEC *root_translation) {
        u32 quarter;
        f32 fraction;
        i32 key_offset;
        GetAni4SamplePosition(anim, frame, quarter, fraction, key_offset);

        u8 *keys = anim->keys + key_offset;
        ani3_scalemin_s *scale_min = anim->scale_min;
        const u16 *curve_types = anim->curve_types;
        for (i32 joint = 0; joint < first_joint; ++joint) {
            const u8 flags = anim->node_flags[joint];
            for (i32 group = 0; group < 3; ++group) {
                if ((flags & CurveGroupMasks[group]) != 0) {
                    for (i32 component = 0; component < 3; ++component) {
                        SkipAni4V4Curve(curve_types[group * 3 + component], keys, scale_min);
                    }
                }
            }
            curve_types += 9;
        }

        i32 end_joint = first_joint + joint_count;
        if (end_joint > anim->node_count) {
            end_joint = anim->node_count;
        }
        const f32 inverse_blend = 1.0f - blend;
        NUVEC *root = first_joint == 0 ? root_translation : NULL;
        for (i32 joint_index = first_joint; joint_index < end_joint; ++joint_index) {
            const u8 flags = anim->node_flags[joint_index];
            buffer->joint_flags[joint_index] |= flags;
            nuanimbuffjoint_s &joint = buffer->joints[joint_index];

            if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
                f32 sampled[3];
                f32 *translation = &joint.translation.x;
                sampled[0] = DecodeAni4V4Curve(anim, curve_types[0], quarter, fraction, keys, scale_min);
                translation[0] += (sampled[0] - translation[0]) * blend;
                sampled[1] = DecodeAni4V4Curve(anim, curve_types[1], quarter, fraction, keys, scale_min);
                translation[1] += (sampled[1] - translation[1]) * blend;
                sampled[2] = DecodeAni4V4Curve(anim, curve_types[2], quarter, fraction, keys, scale_min);
                translation[2] += (sampled[2] - translation[2]) * blend;
                if (root != NULL) {
                    root->x = sampled[0];
                    root->y = sampled[1];
                    root->z = -sampled[2];
                }
            } else {
                joint.translation.x *= inverse_blend;
                joint.translation.y *= inverse_blend;
                joint.translation.z *= inverse_blend;
                if (root != NULL) {
                    root->x = 0.0f;
                    root->y = 0.0f;
                    root->z = 0.0f;
                }
            }

            NUQUAT sampled_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
            if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
                f32 euler[3];
                for (i32 component = 0; component < 3; ++component) {
                    euler[component] =
                        DecodeAni4V4Curve(anim, curve_types[3 + component], quarter, fraction, keys, scale_min);
                }
                NuQuatFromEulerXYZ(&sampled_rotation, static_cast<NUANG>(euler[0] * 10430.378f),
                                   static_cast<NUANG>(euler[1] * 10430.378f),
                                   static_cast<NUANG>(euler[2] * 10430.378f));
            }
            NUQUAT *rotation = reinterpret_cast<NUQUAT *>(&joint.rotation);
            VuQuatSlerpFast(rotation, rotation, &sampled_rotation, blend);

            if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
                f32 *scale = &joint.scale.x;
                for (i32 component = 0; component < 3; ++component) {
                    const f32 sampled =
                        DecodeAni4V4Curve(anim, curve_types[6 + component], quarter, fraction, keys, scale_min);
                    scale[component] += (sampled - scale[component]) * blend;
                }
            } else {
                joint.scale.x = joint.scale.x * inverse_blend + blend;
                joint.scale.y = joint.scale.y * inverse_blend + blend;
                joint.scale.z = joint.scale.z * inverse_blend + blend;
            }

            root = NULL;
            curve_types += 9;
        }
    }

    i32 ANI_SimpleAni3PlayerV4Joint_EulerQuat(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                              i32 first_joint) {
        buffer->use_quaternions = 1;

        u32 quarter;
        f32 fraction;
        i32 key_offset;
        GetAni4SamplePosition(anim, frame, quarter, fraction, key_offset);

        u8 *keys = anim->keys + key_offset;
        ani3_scalemin_s *scale_min = anim->scale_min;
        const u16 *curve_types = anim->curve_types;
        for (i32 joint = 0; joint < first_joint; ++joint) {
            const u8 flags = anim->node_flags[joint];
            for (i32 group = 0; group < 3; ++group) {
                if ((flags & CurveGroupMasks[group]) != 0) {
                    for (i32 component = 0; component < 3; ++component) {
                        SkipAni4V4Curve(curve_types[group * 3 + component], keys, scale_min);
                    }
                }
            }
            curve_types += 9;
        }

        i32 end_joint = first_joint + joint_count;
        if (end_joint > anim->node_count) {
            end_joint = anim->node_count;
        }
        for (i32 joint_index = first_joint; joint_index < end_joint; ++joint_index) {
            const u8 flags = anim->node_flags[joint_index];
            buffer->joint_flags[joint_index] = flags;
            nuanimbuffjoint_s &joint = buffer->joints[joint_index];

            if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
                f32 *translation = &joint.translation.x;
                translation[0] = DecodeAni4V4Curve(anim, curve_types[0], quarter, fraction, keys, scale_min);
                translation[1] = DecodeAni4V4Curve(anim, curve_types[1], quarter, fraction, keys, scale_min);
                translation[2] = DecodeAni4V4Curve(anim, curve_types[2], quarter, fraction, keys, scale_min);
            } else {
                joint.translation = {0.0f, 0.0f, 0.0f};
            }

            NUQUAT *rotation = reinterpret_cast<NUQUAT *>(&joint.rotation);
            if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
                f32 euler[3];
                for (i32 component = 0; component < 3; ++component) {
                    euler[component] =
                        DecodeAni4V4Curve(anim, curve_types[3 + component], quarter, fraction, keys, scale_min);
                }
                NuQuatFromEulerXYZ(rotation, static_cast<NUANG>(euler[0] * 10430.378f),
                                   static_cast<NUANG>(euler[1] * 10430.378f),
                                   static_cast<NUANG>(euler[2] * 10430.378f));
            } else {
                *rotation = {0.0f, 0.0f, 0.0f, 1.0f};
            }

            if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
                f32 *scale = &joint.scale.x;
                for (i32 component = 0; component < 3; ++component) {
                    scale[component] =
                        DecodeAni4V4Curve(anim, curve_types[6 + component], quarter, fraction, keys, scale_min);
                }
            } else {
                joint.scale = {1.0f, 1.0f, 1.0f};
            }

            curve_types += 9;
        }
        return 0;
    }

    void AddAnimEffects(void) {
    }

    f32 animduration_blendouttime;

    f32 AnimDuration(i32 character_id, i32 animation, f32 start_frame, f32 end_frame, i32 subtract_frame_time) {
        if (apicharsys == NULL || character_id < 0 || character_id >= apicharsys->character_count || animation < 0 ||
            animation >= apicharsys->model_id_capacity) {
            return 0.0f;
        }

        const i16 model_index = apicharsys->playermodelids[character_id];
        if (model_index == -1) {
            return 0.0f;
        }

        CHARACTERMODEL_s *model = &apicharsys->models[model_index];
        if (model->model_data_b == NULL || model->model_data_b[animation] == NULL || model->model_data_a == NULL ||
            model->model_data_a[animation] == NULL) {
            return 0.0f;
        }

        f32 duration = NuAnimEndFrame(model->model_data_b[animation]);
        if (start_frame >= 1.0f && duration > start_frame) {
            if (end_frame >= 1.0f && duration > end_frame && end_frame > start_frame) {
                duration = end_frame - start_frame;
            } else {
                duration -= start_frame;
            }
        } else if (end_frame >= 1.0f && duration > end_frame && end_frame > start_frame) {
            duration = end_frame - 1.0f;
        } else {
            duration -= 1.0f;
        }

        CHARACTERANIM_s *animation_info = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
        duration *= (1.0f / (animation_info->playback_rate / 30.0f)) * (1.0f / 30.0f);
        animduration_blendouttime = animation_info->blend_out_time;
        if (subtract_frame_time != 0) {
            duration -= animduration_blendouttime;
        }
        return duration;
    }

    float AnimEndFrame(void *model_ptr, i32 animation) {
        CHARACTERMODEL_s *model = static_cast<CHARACTERMODEL_s *>(model_ptr);
        if (animation == -1 || model->model_data_b[animation] == NULL) {
            return 0.0f;
        }
        return NuAnimEndFrame(model->model_data_b[animation]);
    }

    f32 AnimListFrame(CHARACTERMODEL_s *model, i32 animation, i32 frame) {
        if (animation == -1 || model->model_data_b[animation] == NULL || frame < 0 || frame > 3) {
            return 0.0f;
        }
        CHARACTERANIM_s *info = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
        return info->event_frames[frame];
    }

    f32 *AnimListFrameArray(CHARACTERMODEL_s *model, i32 animation) {
        if (animation == -1 || model->model_data_b[animation] == NULL) {
            return NULL;
        }
        CHARACTERANIM_s *info = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
        return info->event_frames;
    }

    void AnimList_NoLoad(void) {
    }

    void AnimList_RequestAnimGroups(i32 group, ...) {
    }

    void AnimList_RequestAnimGroupForCreatures(i32 creature, ...) {
        va_list groups;
        va_start(groups, creature);
        i32 group = va_arg(groups, i32);
        while (group != -1) {
            AnimList_RequestAnimGroups(group, creature, -1);
            group = va_arg(groups, i32);
        }
        va_end(groups);
    }

    i32 AnimMiscFlags(CHARACTERMODEL_s *model, i32 animation) {
        if (!HasAnimation(model, animation)) {
            return 0;
        }
        return static_cast<CHARACTERANIM_s *>(model->model_data_a[animation])->misc_flags;
    }

    void AnimPacket_FullToMini(ANIMPACKET_s *packet, MINIANIMPACKET_s *mini_packet) {
        mini_packet->current_time = packet->current_time;
        mini_packet->previous_time = packet->previous_time;
        mini_packet->blend_elapsed = packet->blend_elapsed;
        mini_packet->blend_duration = packet->blend_duration;
        mini_packet->blend_source_time = packet->blend_source_time;
        mini_packet->blend_target_time = packet->blend_target_time;
        mini_packet->flags = packet->flags;
        mini_packet->blending = packet->blending;
        mini_packet->blend_animation_a = packet->blend_animation_a;
        mini_packet->blend_animation_b = packet->blend_animation_b;
        mini_packet->animation_index = packet->animation_index;
        mini_packet->previous_animation = packet->previous_animation;
        mini_packet->requested_animation_id = packet->requested_animation;
    }

    void AnimPacket_MiniToFull(MINIANIMPACKET_s *mini_packet, ANIMPACKET_s *packet) {
        packet->current_time = mini_packet->current_time;
        packet->previous_time = mini_packet->previous_time;
        packet->blend_elapsed = mini_packet->blend_elapsed;
        packet->blend_duration = mini_packet->blend_duration;
        packet->blend_source_time = mini_packet->blend_source_time;
        packet->blend_target_time = mini_packet->blend_target_time;
        packet->flags = mini_packet->flags;
        packet->blending = mini_packet->blending;
        packet->blend_animation_a = mini_packet->blend_animation_a;
        packet->blend_animation_b = mini_packet->blend_animation_b;
        packet->animation_index = mini_packet->animation_index;
        packet->previous_animation = mini_packet->previous_animation;
        packet->requested_animation = mini_packet->requested_animation_id;
        packet->blend_source_reversed = 0;
        packet->blend_target_reversed = 0;
        packet->current_reversed = 0;
        packet->overlay_animation = -1;
    }

    void AnimsAvailableToBothCharacters(void) {
    }

    void BlendRootFn(NUMTX *matrix, void *data, NUVEC *source_root, NUVEC *target_root, NUVEC *root_delta, f32 blend) {
        APIOBJECT *object = static_cast<APIOBJECT *>(data);
        CHARACTERANIM_s *source_animation = static_cast<CHARACTERANIM_s *>(
            object->character_model->model_data_a[object->anim_packet.blend_animation_a]);
        CHARACTERANIM_s *target_animation = static_cast<CHARACTERANIM_s *>(
            object->character_model->model_data_a[object->anim_packet.blend_animation_b]);

        NUVEC source_motion;
        NUVEC target_motion;
        NUVEC source_position;
        NUVEC target_position;

        if ((source_animation->flags & CHARACTER_ANIMATION_FLAG_ROOT_MOTION) != 0) {
            if (object->previous_animation_root_time > object->anim_packet.blend_source_time ||
                object->previous_animation_root_info != source_animation) {
                object->previous_animation_root = *source_root;
                object->previous_animation_root_info = source_animation;
            }

            source_motion.x = source_root->x - object->previous_animation_root.x;
            source_motion.y = (source_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) != 0
                                  ? source_root->y - object->previous_animation_root.y
                                  : 0.0f;
            source_motion.z = source_root->z - object->previous_animation_root.z;
            object->previous_animation_root = *source_root;

            source_position.x = 0.0f;
            source_position.y =
                (source_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) != 0 ? 0.0f : source_root->y;
            source_position.z = 0.0f;
            object->previous_animation_root_time = object->anim_packet.blend_source_time;
        } else {
            source_motion.x = 0.0f;
            source_motion.y = 0.0f;
            source_motion.z = 0.0f;
            source_position = *source_root;
            object->previous_animation_root_time = FLT_MAX;
        }

        NUVEC source_offset = source_animation->root_translation;

        if ((target_animation->flags & CHARACTER_ANIMATION_FLAG_ROOT_MOTION) != 0) {
            if (object->previous_blend_target_root_time > object->anim_packet.blend_target_time ||
                object->previous_blend_target_root_info != target_animation) {
                object->previous_blend_target_root = *target_root;
                object->previous_blend_target_root_info = target_animation;
            }

            target_motion.x = target_root->x - object->previous_blend_target_root.x;
            target_motion.y = (target_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) != 0
                                  ? target_root->y - object->previous_blend_target_root.y
                                  : 0.0f;
            target_motion.z = target_root->z - object->previous_blend_target_root.z;
            object->previous_blend_target_root = *target_root;

            target_position.x = 0.0f;
            target_position.y =
                (target_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) != 0 ? 0.0f : target_root->y;
            target_position.z = 0.0f;
            object->previous_blend_target_root_time = object->anim_packet.blend_target_time;
        } else {
            target_motion.x = 0.0f;
            target_motion.y = 0.0f;
            target_motion.z = 0.0f;
            target_position = *target_root;
            object->previous_blend_target_root_time = FLT_MAX;
        }

        NUVEC target_offset = target_animation->root_translation;
        NuVecLerp(NUMTX_GET_ROW_VEC(matrix, 3), &source_position, &target_position, blend);

        NUVEC blended_offset;
        NuVecLerp(&blended_offset, &source_offset, &target_offset, blend);
        root_delta->x += blended_offset.x;
        root_delta->y += blended_offset.y;
        root_delta->z += blended_offset.z;
        NuMtxTranslate(matrix, root_delta);

        NuVecMtxRotate(&source_motion, &source_motion, &object->field_0xb8);
        if ((source_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) == 0) {
            source_motion.y = 0.0f;
        }
        NuVecMtxRotate(&target_motion, &target_motion, &object->field_0xb8);
        if ((target_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) == 0) {
            target_motion.y = 0.0f;
        }
        NuVecLerp(&object->animation_root_delta, &source_motion, &target_motion, blend);

        const f32 root_motion_epsilon = 1.0e-11f;
        if (((source_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) != 0 ||
             (target_animation->flags & CHARACTER_ANIMATION_FLAG_VERTICAL_ROOT_MOTION) != 0) &&
            object->animation_root_delta.y == 0.0f) {
            object->animation_root_delta.y = root_motion_epsilon;
        } else if (object->animation_root_delta.x == 0.0f && object->animation_root_delta.y == 0.0f &&
                   object->animation_root_delta.z == 0.0f) {
            object->animation_root_delta.x = root_motion_epsilon;
        }
    }

    void BlendTimeBetweenAnims(void) {
    }

    i32 CurrentAnim(ANIMPACKET_s *packet) {
        return packet->blending == 0 ? packet->animation_index : packet->blend_animation_b;
    }

    void EvalModelAnim(void) {
    }

    i32 FindAnimIX(CHARACTERDATA *character, char *name) {
        if (character != NULL) {
            CHARACTERANIM_s *animation = character->animations;
            while (animation != NULL && animation->name != NULL) {
                if (NuStrICmp(name, animation->name) == 0) {
                    return animation->animation_id;
                }
                ++animation;
            }
        }
        return -1;
    }

    f32 GetAnimTimeRandom(CHARACTERMODEL_s *model, i32 animation) {
        if (!HasAnimation(model, animation)) {
            return 0.0f;
        }
        return 1.0f + NuRandFloat() * (NuAnimEndFrame(model->model_data_b[animation]) - 1.0f);
    }

    f32 GetInstAnimEndFrame(nugscn_s *scene, nuinstanim_s *instance_animation) {
        if (instance_animation == NULL) {
            return 0.0f;
        }

        void *animation = scene->instance_animation_data[instance_animation->anim_ix];
        if (animation != NULL) {
            return NuAnimEndFrameOld(animation);
        }
        if ((instance_animation->end_frame_lookup_bits & NUINSTANIM_END_FRAME_LOOKUP_MASK) == 0 ||
            scene->animation_end_frames == NULL) {
            return 0.0f;
        }

        return static_cast<f32>(scene->animation_end_frames[instance_animation->end_frame_lookup_index - 1].end_frame);
    }

    void ResetAnimPacket(ANIMPACKET_s *packet, i16 animation) {
        if (packet == NULL) {
            return;
        }
        packet->requested_animation = animation;
        packet->previous_animation = packet->requested_animation;
        packet->animation_index = packet->previous_animation;
        packet->previous_time = 1.0f;
        packet->blend_target_time = packet->previous_time;
        packet->current_time = packet->blend_target_time;
        packet->blending = 0;
        packet->flags = ANIMPACKET_FLAG_ANIMATION_CHANGED;
        packet->overlay_animation = -1;
        packet->current_reversed = 0;
        packet->blend_source_reversed = 0;
        packet->blend_target_reversed = 0;
    }

    void RootFn(NUMTX *matrix, void *data, NUVEC *source_root, NUVEC *target_root, NUVEC *root_delta, f32 blend) {
        RootFnEx(matrix, data, source_root, target_root, root_delta, blend, 0);
    }

    void RootFnY(NUMTX *matrix, void *data, NUVEC *source_root, NUVEC *target_root, NUVEC *root_delta, f32 blend) {
        RootFnEx(matrix, data, source_root, target_root, root_delta, blend, 1);
    }

    ACTIONINFO_s *APIActionInfo;
    EXTRAACTIONDATA_s *APIExtraActionData;

    void SetActionInfo(ACTIONINFO_s *action_info, EXTRAACTIONDATA_s *extra_action_data) {
        APIActionInfo = action_info;
        APIExtraActionData = extra_action_data;
    }

    void SetAnimTimeRandom(CHARACTERMODEL_s *model, ANIMPACKET_s *packet) {
        if (model == NULL || packet == NULL) {
            return;
        }

        void *animation = model->model_data_b[packet->field_0x3a];
        if (animation != NULL) {
            const f32 random = NuRandFloat();
            const f32 end_frame = NuAnimEndFrame(model->model_data_b[packet->field_0x3a]);
            packet->field_0x00 = 1.0f + random * (end_frame - 1.0f);
        }
    }

    NUJOINTPROCANIMFN JointProcAnimFn;

    void SetProceduralAnimationFn(void *function) {
        JointProcAnimFn = reinterpret_cast<NUJOINTPROCANIMFN>(function);
    }

    i32 StateAnimEvaluate(StateAnim *state, u8 *index, u8 *value, f32 frame) {
        u8 next = *index;
        if (next < state->count) {
            bool changed = false;
            do {
                if (frame < state->times[next]) {
                    if (changed) {
                        return 1;
                    }
                    break;
                }
                changed = true;
                *value = state->values[next];
                next = static_cast<u8>(*index + 1);
                *index = next;
            } while (next < state->count);
            if (next >= state->count) {
                return 1;
            }
        }

        i32 changed = 0;
        if (next == 0) {
            return 0;
        }
        do {
            if (state->times[next - 1] <= frame) {
                return changed;
            }
            next--;
            *index = next;
            *value = next == 0 ? state->values[0] : state->values[next - 1];
            changed = 1;
        } while (next != 0);
        return 1;
    }

    bool StateAnimEvaluate2(StateAnim *state, u8 *index, char *value, f32 frame) {
        i32 current = *index;
        i32 count = state->count;
        if (current >= count) {
            current = count - 1;
        }
        if (current < 0) {
            current = 0;
        }
        char old_value = state->values[current];
        if (frame < state->times[current]) {
            while (current != 0 && frame < state->times[current - 1]) {
                --current;
            }
        } else {
            while (current < count - 1 && state->times[current + 1] <= frame) {
                ++current;
            }
        }
        char new_value = state->values[current];
        *value = new_value;
        *index = static_cast<u8>(current);
        return old_value != new_value;
    }

    void NuSpecialSetInstAnimTime(nuhspecial_s *special, f32 frame) {
        NUGSCN *scene = special->scene;
        if (scene == NULL) {
            return;
        }
        nuinstanim_s *animation = NuSpecialGetInstAnim(special);
        if (animation == NULL) {
            return;
        }
        animation->ltime = frame;
        if ((animation->end_frame_lookup_bits & NUINSTANIM_END_FRAME_LOOKUP_MASK) != 0 &&
            scene->animation_end_frames != NULL) {
            StateAnim *state =
                reinterpret_cast<StateAnim *>(&scene->animation_end_frames[animation->end_frame_lookup_index - 1]);
            u8 index = static_cast<u8>(static_cast<u32>(animation->flags) >> NUINSTANIM_STATE_INDEX_SHIFT);
            char value;
            StateAnimEvaluate2(state, &index, &value, frame);
            animation->flags =
                static_cast<NUINSTANIM_FLAGS>((static_cast<u32>(animation->flags) & ~NUINSTANIM_STATE_INDEX_MASK) |
                                              (static_cast<u32>(index) << NUINSTANIM_STATE_INDEX_SHIFT));
        }
    }

    StateAnim *StateAnimFixPtrs(StateAnim *state, isize delta) {
        if (state == NULL) {
            return NULL;
        }
        state = reinterpret_cast<StateAnim *>(reinterpret_cast<usize>(state) + delta);
        if (state == NULL) {
            return NULL;
        }
        state->times =
            state->times != NULL ? reinterpret_cast<f32 *>(reinterpret_cast<usize>(state->times) + delta) : NULL;
        state->values =
            state->values != NULL ? reinterpret_cast<u8 *>(reinterpret_cast<usize>(state->values) + delta) : NULL;
        return state;
    }

    void UpdateAnimPacket(CHARACTERMODEL_s *model, ANIMPACKET_s *packet, f32 frame_step, f32 movement_speed,
                          f32 blend_step, f32 backwards_multiplier) {
        i32 backwards = 0;
        i32 interrupted_reversed = 0;
        i32 interrupted = 0;
        if (movement_speed < 0.0f) {
            backwards = 1;
            movement_speed = -movement_speed;
        }

        if (model == NULL || packet == NULL) {
            return;
        }

        const i32 paused = packet->flags & ANIMPACKET_FLAG_PAUSED;
        const i32 force_restart = packet->flags & ANIMPACKET_FLAG_FORCE_RESTART;
        packet->flags = 0;
        packet->previous_time = packet->blending == 0 ? packet->current_time : packet->blend_target_time;
        if (frame_step == 0.0f) {
            packet->flags |= ANIMPACKET_FLAG_ZERO_TIMESTEP;
            return;
        }

        if (packet->overlay_animation == -1) {
            CHARACTERANIM_s *current_info;
            if (packet->blending != 0) {
                const i16 requested = packet->requested_animation;
                CHARACTERANIM_s *requested_info = GetAnimationInfo(model, requested);
                if (requested != -1 && requested != packet->blend_animation_b && HasAnimation(model, requested) &&
                    requested_info != NULL && requested_info->blend_in_time == 0.0f) {
                    packet->animation_index = requested;
                    if (backwards != 0 &&
                        (requested_info->flags & CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT) != 0) {
                        packet->current_reversed = 1;
                        packet->current_time = NuAnimEndFrame(model->model_data_b[requested]);
                    } else {
                        packet->current_reversed = 0;
                        packet->current_time = 1.0f;
                    }
                    packet->blending = 0;
                    packet->previous_time = packet->current_time;
                    packet->flags |= ANIMPACKET_FLAG_ANIMATION_CHANGED;
                    goto update_timers;
                }

                packet->blend_elapsed += blend_step;
                if (packet->blend_elapsed >= packet->blend_duration) {
                    packet->blending = 0;
                    packet->animation_index = packet->blend_animation_b;
                    packet->current_time = packet->blend_target_time;
                    packet->current_reversed = packet->blend_target_reversed;
                    packet->flags |= ANIMPACKET_FLAG_BLEND_FINISHED;
                    goto update_timers;
                }

                if (GetAnimBlendMode() != 1 || requested == packet->blend_animation_b ||
                    !HasAnimation(model, requested)) {
                    goto update_timers;
                }

                if (packet->blend_elapsed < packet->blend_duration * 0.5f) {
                    packet->previous_animation = packet->blend_animation_a;
                    interrupted_reversed = static_cast<i8>(packet->blend_source_reversed);
                    interrupted = 1;
                } else {
                    packet->previous_animation = packet->blend_animation_b;
                    interrupted_reversed = static_cast<i8>(packet->blend_target_reversed);
                    packet->blend_source_time = packet->blend_target_time;
                    interrupted = 1;
                }
            }

            // Interrupted blends enter the transition directly (original
            // 0x3ce858/0x3ce88a), even when returning to their source animation.
            if (interrupted == 0 && packet->requested_animation == packet->previous_animation) {
                if (force_restart == 0 || packet->requested_animation != packet->animation_index ||
                    !HasAnimation(model, packet->animation_index)) {
                    packet->animation_index = packet->requested_animation;
                    packet->blending = 0;
                    goto update_timers;
                }
            }

            if (packet->previous_animation != -1 && packet->requested_animation != -1 &&
                HasAnimation(model, packet->previous_animation) && HasAnimation(model, packet->requested_animation)) {
                CHARACTERANIM_s *source_info = GetAnimationInfo(model, packet->previous_animation);
                CHARACTERANIM_s *target_info = GetAnimationInfo(model, packet->requested_animation);
                if (source_info != NULL && target_info != NULL && source_info->blend_out_time > blend_step &&
                    target_info->blend_in_time > blend_step) {
                    packet->blending = 1;
                    packet->blend_animation_a = packet->previous_animation;
                    packet->blend_source_reversed = static_cast<u8>(interrupted_reversed);
                    packet->blend_animation_b = packet->requested_animation;
                    if (interrupted == 0) {
                        packet->blend_source_time = packet->current_time;
                    }
                    packet->blend_source_reversed = packet->current_reversed;

                    const bool synchronised = (source_info->flags & CHARACTER_ANIMATION_FLAG_SYNCHRONISED) != 0 &&
                                              (target_info->flags & CHARACTER_ANIMATION_FLAG_SYNCHRONISED) != 0 &&
                                              source_info->playback_rate == target_info->playback_rate &&
                                              NuAnimEndFrame(model->model_data_b[packet->blend_animation_a]) ==
                                                  NuAnimEndFrame(model->model_data_b[packet->blend_animation_b]);
                    if (synchronised) {
                        packet->blend_target_time = packet->blend_source_time;
                        packet->blend_target_reversed =
                            backwards != 0 && (target_info->flags & CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT) != 0
                                ? 1
                                : 0;
                    } else if (backwards != 0 &&
                               (target_info->flags & CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT) != 0) {
                        packet->blend_target_reversed = 1;
                        packet->blend_target_time = NuAnimEndFrame(model->model_data_b[packet->blend_animation_b]);
                    } else {
                        packet->blend_target_reversed = 0;
                        packet->blend_target_time = 1.0f;
                    }
                    packet->blend_elapsed = 0.0f;
                    packet->blend_duration = target_info->blend_in_time;
                    if (packet->blend_duration > source_info->blend_out_time) {
                        packet->blend_duration = source_info->blend_out_time;
                    }
                    packet->flags |= ANIMPACKET_FLAG_ANIMATION_CHANGED;
                    goto update_timers;
                }
            }

            packet->animation_index = packet->requested_animation;
            current_info = GetAnimationInfo(model, packet->animation_index);
            if (backwards != 0 && HasAnimation(model, packet->animation_index) && current_info != NULL &&
                (current_info->flags & CHARACTER_ANIMATION_FLAG_REVERSE_WITH_MOVEMENT) != 0) {
                packet->current_reversed = 1;
                packet->current_time = NuAnimEndFrame(model->model_data_b[packet->animation_index]);
            } else {
                packet->current_reversed = 0;
                packet->current_time = 1.0f;
            }
            packet->blending = 0;
            packet->previous_time = packet->current_time;
            packet->flags |= ANIMPACKET_FLAG_ANIMATION_CHANGED;

        update_timers:
            if (packet->blending == 0) {
                if (!HasAnimation(model, packet->animation_index)) {
                    const i16 requested_animation = packet->requested_animation;
                    ResetAnimPacket(packet, -1);
                    packet->requested_animation = requested_animation;
                    return;
                }
                if (paused != 0) {
                    frame_step = 0.0f;
                }
                packet->current_time = UpdateAnimTimer(
                    model, packet, packet->animation_index, packet->current_time, frame_step, movement_speed, 1,
                    reinterpret_cast<char *>(&packet->current_reversed), backwards, backwards_multiplier);
            } else if (HasAnimation(model, packet->blend_animation_a) &&
                       HasAnimation(model, packet->blend_animation_b)) {
                packet->blend_source_time = UpdateAnimTimer(
                    model, packet, packet->blend_animation_a, packet->blend_source_time, frame_step, movement_speed, 0,
                    reinterpret_cast<char *>(&packet->blend_source_reversed), backwards, backwards_multiplier);
                packet->blend_target_time = UpdateAnimTimer(
                    model, packet, packet->blend_animation_b, packet->blend_target_time, frame_step, movement_speed, 1,
                    reinterpret_cast<char *>(&packet->blend_target_reversed), backwards, backwards_multiplier);
            }
        } else if (HasAnimation(model, packet->requested_animation) && HasAnimation(model, packet->overlay_animation)) {
            packet->blend_source_time = UpdateAnimTimer(
                model, packet, packet->requested_animation, packet->blend_source_time, frame_step, movement_speed, 0,
                reinterpret_cast<char *>(&packet->blend_source_reversed), backwards, backwards_multiplier);
            packet->blend_target_time = UpdateAnimTimer(
                model, packet, packet->overlay_animation, packet->blend_target_time, frame_step, movement_speed, 1,
                reinterpret_cast<char *>(&packet->blend_target_reversed), backwards, backwards_multiplier);
        }
    }

    void UpdateMiniAnimPacket(CHARACTERMODEL_s *model, MINIANIMPACKET_s *mini_packet, f32 frame_step,
                              f32 movement_speed, f32 blend_step) {
        ANIMPACKET_s packet;
        AnimPacket_MiniToFull(mini_packet, &packet);
        UpdateAnimPacket(model, &packet, frame_step, movement_speed, blend_step, 0.0f);
        AnimPacket_FullToMini(&packet, mini_packet);
    }

} // extern "C"

void SetAnimFrame(nuhspecial_s *special, float frame) {
    if (NuSpecialExistsFn(special) == 0)
        return;
    NUMTX matrix __attribute__((aligned(16)));
    NuMtxSetIdentity(&matrix);
    nuinstanim_s *instance_animation = NuSpecialGetInstAnim(special);
    if (instance_animation == NULL)
        return;
    nuanimdata_s *animation = special->scene->instance_animation_data[instance_animation->anim_ix];
    if (animation == NULL)
        return;
    // The animation header begins with its final frame; the remaining header is opaque here.
    f32 end_frame;
    memcpy(&end_frame, animation, sizeof(end_frame));
    if (frame == 1.0e9f)
        frame = end_frame;
    if (!(frame >= 1.0f && frame <= end_frame))
        return;
    NuAnimData2CalcMatrix(animation, 0, frame, &matrix);
    instance_animation->mtx = matrix;
    NUMTX *instance_matrix = NuSpecialGetInstanceMtx(special);
    instance_animation->mtx.m30 = instance_matrix->m30;
    instance_animation->mtx.m31 = instance_matrix->m31;
    instance_animation->mtx.m32 = instance_matrix->m32;
    instance_animation->ltime = frame;
}

struct DefaultIdleCharacterData {
    u8 pad[0x116];
    u8 use_standard_idle;
};

i32 GetDefaultIdle(GameObject_s *obj) {
    CHARACTERDATA *character = obj->apiobj.character_data;
    DefaultIdleCharacterData *game_character = static_cast<DefaultIdleCharacterData *>(character->field11_0x24);

    i32 animation = 25;
    i32 table_offset = 100;
    if (game_character->use_standard_idle == 0 && (character->model_flags & 0x80) != 0) {
        animation = 118;
        table_offset = 472;
    }
    if (obj->batarang != NULL && *(reinterpret_cast<u8 *>(obj->batarang) + 0x7d) != 0) {
        return 151;
    }

    u8 *animation_table = reinterpret_cast<u8 *>(obj->apiobj.character_model->model_data_b);
    void *entry = *reinterpret_cast<void **>(animation_table + table_offset);
    if (entry != NULL &&
        (*reinterpret_cast<i32 *>(animation_table + 4) == 0 || (obj->field_0xe22 & 1) != 0 || obj->field_0xe32 == 1)) {
        return animation;
    }
    return 1;
}

i32 GetAnimDirection(nuinstanim_s *animation) {
    if (animation == NULL || animation->tfactor == 0.0f) {
        return -1;
    }
    return animation->tfactor < 0.0f ? 1 : 0;
}

i32 FindTexAnimFromMtl(nugscn_s *scene, numtl_s *material) {
    nutexanim_s *animations = static_cast<nutexanim_s *>(scene->texture_anims);
    for (i32 animation_index = 0; animation_index < scene->num_texture_anims; ++animation_index) {
        if (animations[animation_index].material == material) {
            return animation_index + 1;
        }
    }
    return 0;
}

static char **TexAnimList;

void InitTexAnimScripts(char **names) {
    TexAnimList = names;
    if (names == NULL)
        return;
    while (*names != NULL) {
        permbuffer_ptr.addr = ALIGN(permbuffer_ptr.addr, 4);
        char path[72];
        NuStrCpy(path, "stuff\\ats\\");
        NuStrCat(path, *names++);
        NuStrCat(path, ".ats");
        NuTexAnimProgReadScript(path, &permbuffer_ptr);
    }
    permbuffer_ptr.addr = ALIGN(permbuffer_ptr.addr, 16);
}

i32 GizmoFileReadGameAnimSet(GAMEANIMSET_s *set, void *world_ptr,
                             void (*read_object_data)(GAMEANIMOBJ_s *, unsigned char), unsigned char version,
                             char *prefix, char *suffix) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    const unsigned char file_version = static_cast<unsigned char>(EdFileReadChar());
    const unsigned char object_count = static_cast<unsigned char>(EdFileReadChar());
    i32 success = 1;

    for (i32 object_index = 0; object_index < object_count; ++object_index) {
        char object_name[64];
        const i32 name_length = static_cast<signed char>(EdFileReadChar());
        if (name_length != 0) {
            EdFileRead(object_name, name_length);
        }

        const f32 start_frame = EdFileReadFloat();
        const f32 end_frame = EdFileReadFloat();
        u32 flags = 0;
        if (file_version > 1) {
            flags = EdFileReadInt();
        }

        if (name_length == 0) {
            continue;
        }

        if (set->object_pool == NULL || set->object_pool->free_objects == NULL) {
            success = 0;
        }

        GAMEANIMOBJ_s *object = GameAnimSet_AddObjectByName(set, world->current_gscn, object_name, start_frame,
                                                            end_frame, 0, world->gizmo_sys, prefix, suffix);
        GAMEANIMOBJ_s missing_object;
        if (object == NULL) {
            memset(&missing_object, 0, sizeof(missing_object));
            object = &missing_object;
        }

        object->flags = flags;
        if (read_object_data != NULL) {
            read_object_data(object, version);
        }
        if (file_version <= 2) {
            object->flags &= ~2u;
        }
    }

    return success;
}

static i32 PlayAni4Quaternion(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                              i32 first_joint, i32 quaternion_components) {
    buffer->use_quaternions = 1;

    u32 quarter;
    f32 fraction;
    i32 key_offset;
    GetAni4SamplePosition(anim, frame, quarter, fraction, key_offset);

    u8 *keys = anim->keys + key_offset;
    ani3_scalemin_s *scale_min = anim->scale_min;
    const u16 *curve_types = anim->curve_types;
    const i32 curves_per_joint = quaternion_components + 6;

    for (i32 joint = 0; joint < first_joint; ++joint) {
        SkipAni4QuaternionJoint(anim, quaternion_components, joint, curve_types, keys, scale_min);
        curve_types += curves_per_joint;
    }

    i32 end_joint = first_joint + joint_count;
    if (end_joint > anim->node_count) {
        end_joint = anim->node_count;
    }
    for (i32 joint_index = first_joint; joint_index < end_joint; ++joint_index) {
        const u8 flags = anim->node_flags[joint_index];
        buffer->joint_flags[joint_index] = flags;
        nuanimbuffjoint_s &joint = buffer->joints[joint_index];

        if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
            f32 *translation = &joint.translation.x;
            for (i32 component = 0; component < 3; ++component) {
                translation[component] =
                    DecodeAni4QuaternionScalar(anim, curve_types[component], quarter, fraction, keys, scale_min);
            }
        } else {
            joint.translation = {0.0f, 0.0f, 0.0f};
        }

        NUQUAT *rotation = reinterpret_cast<NUQUAT *>(&joint.rotation);
        if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
            *rotation =
                DecodeAni4Quaternion(anim, quaternion_components, curve_types + 3, quarter, fraction, keys, scale_min);
        } else {
            *rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        }

        const i32 scale_offset = quaternion_components + 3;
        if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
            f32 *scale = &joint.scale.x;
            for (i32 component = 0; component < 3; ++component) {
                scale[component] = DecodeAni4QuaternionScalar(anim, curve_types[scale_offset + component], quarter,
                                                              fraction, keys, scale_min);
            }
        } else {
            joint.scale = {1.0f, 1.0f, 1.0f};
        }

        curve_types += curves_per_joint;
    }
    return 0;
}

static void BlendAni4Quaternion(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend, i32 joint_count,
                                i32 first_joint, NUVEC *root_translation, i32 quaternion_components) {
    u32 quarter;
    f32 fraction;
    i32 key_offset;
    GetAni4SamplePosition(anim, frame, quarter, fraction, key_offset);

    u8 *keys = anim->keys + key_offset;
    ani3_scalemin_s *scale_min = anim->scale_min;
    const u16 *curve_types = anim->curve_types;
    const i32 curves_per_joint = quaternion_components + 6;
    const f32 inverse_blend = 1.0f - blend;

    for (i32 joint = 0; joint < first_joint; ++joint) {
        SkipAni4QuaternionJoint(anim, quaternion_components, joint, curve_types, keys, scale_min);
        curve_types += curves_per_joint;
    }

    i32 end_joint = first_joint + joint_count;
    if (end_joint > anim->node_count) {
        end_joint = anim->node_count;
    }
    NUVEC *root = first_joint == 0 ? root_translation : NULL;
    for (i32 joint_index = first_joint; joint_index < end_joint; ++joint_index) {
        const u8 flags = anim->node_flags[joint_index];
        buffer->joint_flags[joint_index] |= flags;
        nuanimbuffjoint_s &joint = buffer->joints[joint_index];

        if ((flags & NUANIMBUFF_JOINT_TRANSLATION) != 0) {
            f32 sampled[3];
            f32 *translation = &joint.translation.x;
            for (i32 component = 0; component < 3; ++component) {
                sampled[component] =
                    DecodeAni4QuaternionScalar(anim, curve_types[component], quarter, fraction, keys, scale_min);
                translation[component] += (sampled[component] - translation[component]) * blend;
            }
            if (root != NULL) {
                root->x = sampled[0];
                root->y = sampled[1];
                root->z = -sampled[2];
            }
        } else {
            joint.translation.x *= inverse_blend;
            joint.translation.y *= inverse_blend;
            joint.translation.z *= inverse_blend;
            if (root != NULL) {
                root->x = 0.0f;
                root->y = 0.0f;
                root->z = 0.0f;
            }
        }

        NUQUAT sampled_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        if ((flags & NUANIMBUFF_JOINT_ROTATION) != 0) {
            sampled_rotation =
                DecodeAni4Quaternion(anim, quaternion_components, curve_types + 3, quarter, fraction, keys, scale_min);
        }
        NUQUAT *rotation = reinterpret_cast<NUQUAT *>(&joint.rotation);
        VuQuatSlerpFast(rotation, rotation, &sampled_rotation, blend);

        const i32 scale_offset = quaternion_components + 3;
        if ((flags & NUANIMBUFF_JOINT_SCALE) != 0) {
            f32 *scale = &joint.scale.x;
            for (i32 component = 0; component < 3; ++component) {
                const f32 sampled = DecodeAni4QuaternionScalar(anim, curve_types[scale_offset + component], quarter,
                                                               fraction, keys, scale_min);
                scale[component] += (sampled - scale[component]) * blend;
            }
        } else {
            joint.scale.x = joint.scale.x * inverse_blend + blend;
            joint.scale.y = joint.scale.y * inverse_blend + blend;
            joint.scale.z = joint.scale.z * inverse_blend + blend;
        }

        root = NULL;
        curve_types += curves_per_joint;
    }
}

static inline f32 DecodeAni4Quat3Scalar(const ani3_animheader_s *anim, i32 key_stride, const u16 *constants, u16 type,
                                        u32 quarter, f32 fraction, u8 *&keys, ani3_scalemin_s *&scale_min) {
    if (type != 6) {
        const u32 constant = constants[type];
        return static_cast<f32>(constant) * anim->scale + anim->minimum;
    }

    const u32 first_word = *reinterpret_cast<const u32 *>(keys);
    const u32 next_word = *reinterpret_cast<const u32 *>(keys + key_stride);
    const f32 first_value = static_cast<f32>(first_word & 0xff);
    const f32 next_value = static_cast<f32>(next_word & 0xff);
    const u32 tangents = first_word >> 8;
    const f32 tangent_scale = 0.01587302f;
    const f32 tangent0 = static_cast<f32>((tangents >> (quarter * 6)) & 0x3f) * tangent_scale;

    f32 packed_value;
    if (quarter == 3) {
        const f32 interpolated = (next_value - first_value) * tangent0 + first_value;
        const f32 tangent1 = static_cast<f32>((next_word >> 8) & 0x3f) * tangent_scale;
        const f32 after_value = static_cast<f32>(keys[key_stride * 2]);
        const f32 next_interpolated = (after_value - next_value) * tangent1 + next_value;
        packed_value = (next_interpolated - interpolated) * fraction + interpolated;
    } else {
        const f32 tangent1 = static_cast<f32>((tangents >> (((quarter * 3 + 3) * 2) & 0x1f)) & 0x3f) * tangent_scale;
        packed_value = (next_value - first_value) * ((tangent1 - tangent0) * fraction + tangent0) + first_value;
    }

    const f32 value = packed_value * scale_min->scale + scale_min->minimum;
    keys += 4;
    ++scale_min;
    return value;
}

static inline void DecodeAni4Quat3Pair(const ani3_animheader_s *anim, i32 key_stride, const u16 *constants, u16 type,
                                       u32 quarter, u8 *&keys, ani3_scalemin_s *&scale_min, f32 &first, f32 &second) {
    if (type == 7) {
        CalcValue1648Get2Values(reinterpret_cast<char *>(keys), quarter, key_stride, scale_min, &first, &second);
        keys += 8;
        ++scale_min;
        return;
    }
    if (type == 9) {
        const u16 *samples = reinterpret_cast<const u16 *>(keys);
        first = static_cast<f32>(samples[quarter]) * scale_min->scale + scale_min->minimum;
        const u16 next = quarter == 3 ? *reinterpret_cast<const u16 *>(keys + key_stride) : samples[quarter + 1];
        second = static_cast<f32>(next) * scale_min->scale + scale_min->minimum;
        keys += 8;
        ++scale_min;
        return;
    }
    if (type == 6) {
        const u32 first_word = *reinterpret_cast<const u32 *>(keys);
        const u32 next_word = *reinterpret_cast<const u32 *>(keys + key_stride);
        const f32 start = static_cast<f32>(first_word & 0xff);
        const f32 next = static_cast<f32>(next_word & 0xff);
        const u32 tangents = first_word >> 8;
        const f32 tangent0 = static_cast<f32>((tangents >> (quarter * 6)) & 0x3f) * 0.01587302f;
        first = ((next - start) * tangent0 + start) * scale_min->scale + scale_min->minimum;
        if (quarter == 3) {
            const f32 tangent1 = static_cast<f32>((next_word >> 8) & 0x3f) * 0.01587302f;
            const f32 after = static_cast<f32>(keys[key_stride * 2]);
            second = ((after - next) * tangent1 + next) * scale_min->scale + scale_min->minimum;
        } else {
            const f32 tangent1 = static_cast<f32>((tangents >> (((quarter * 3 + 3) * 2) & 0x1f)) & 0x3f) * 0.01587302f;
            second = ((next - start) * tangent1 + start) * scale_min->scale + scale_min->minimum;
        }
        keys += 4;
        ++scale_min;
        return;
    }

    first = second = static_cast<f32>(static_cast<u32>(constants[type])) * anim->scale + anim->minimum;
}

i32 ANI_SimpleAni3PlayerV4Joint_Quat3(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                      i32 first_joint) {
    const u8 *node_flags = anim->node_flags;
    buffer->use_quaternions = 1;
    u32 quarter;
    f32 fraction;
    i32 key_offset;
    if (anim->key_count == 1) {
        quarter = 0;
        fraction = 0.0f;
        key_offset = 0;
    } else {
        const i32 last = anim->key_count - 1;
        f32 key = (frame - anim->first_frame) * static_cast<f32>(last) / static_cast<f32>(anim->frame_count - 1);
        if (key < 0.0f)
            key = 0.0f;
        i32 whole;
        if (static_cast<f32>(last) <= key) {
            whole = last;
            fraction = 0.0f;
        } else {
            whole = static_cast<i32>(key);
            fraction = key - static_cast<f32>(whole);
        }
        quarter = static_cast<u32>(whole) & 3;
        key_offset = (whole >> 2) * anim->key_stride;
    }
    u8 *keys = anim->keys + key_offset;
    i32 key_stride = anim->key_stride;
    const u16 *constants = reinterpret_cast<const u16 *>(anim->constants) - 16;
    ani3_scalemin_s *scale_min = anim->scale_min;
    const u16 *curve_types = anim->curve_types;
    i32 count = joint_count < anim->node_count ? joint_count : anim->node_count;
    const u8 *first_flags = node_flags + first_joint;
    for (const u8 *skip_flags = node_flags; skip_flags < first_flags; ++skip_flags) {
        if (*skip_flags & 2) {
            SkipAni4V4Curve(curve_types[0], keys, scale_min);
            SkipAni4V4Curve(curve_types[1], keys, scale_min);
            SkipAni4V4Curve(curve_types[2], keys, scale_min);
        }
        if (*skip_flags & 1) {
            SkipAni4V4Curve(curve_types[3], keys, scale_min);
            SkipAni4V4Curve(curve_types[4], keys, scale_min);
            SkipAni4V4Curve(curve_types[5], keys, scale_min);
        }
        if (*skip_flags & 8) {
            SkipAni4V4Curve(curve_types[6], keys, scale_min);
            SkipAni4V4Curve(curve_types[7], keys, scale_min);
            SkipAni4V4Curve(curve_types[8], keys, scale_min);
        }
        curve_types += 9;
    }
    node_flags += first_joint;
    const u8 *end_flags = node_flags + count;
    u8 *joint_flags = buffer->joint_flags + first_joint;
    nuanimbuffjoint_s *joint = buffer->joints + first_joint;
    for (; node_flags < end_flags; ++node_flags, ++joint) {
        const u8 flags = *node_flags;
        *joint_flags++ = flags;
        f32 *output = &joint->translation.x;
        for (i32 group = 0; group < 3; ++group, curve_types += 3, output += 4) {
            if (!(flags & CurveGroupMasks[group])) {
                if (group == 0) {
                    output[0] = output[1] = output[2] = 0.0f;
                } else if (group == 1) {
                    output[0] = output[1] = output[2] = 0.0f;
                    output[3] = 1.0f;
                } else {
                    output[0] = output[1] = output[2] = 1.0f;
                }
            } else if (group != 1) {
                output[0] = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[0], quarter, fraction, keys,
                                                  scale_min);
                output[1] = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[1], quarter, fraction, keys,
                                                  scale_min);
                output[2] = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[2], quarter, fraction, keys,
                                                  scale_min);
            } else {
                NUQUAT first, second;
                DecodeAni4Quat3Pair(anim, key_stride, constants, curve_types[0], quarter, keys, scale_min, first.x,
                                    second.x);
                DecodeAni4Quat3Pair(anim, key_stride, constants, curve_types[1], quarter, keys, scale_min, first.y,
                                    second.y);
                DecodeAni4Quat3Pair(anim, key_stride, constants, curve_types[2], quarter, keys, scale_min, first.z,
                                    second.z);
                first.w = NuFsqrt(1.0f - (first.x * first.x + first.y * first.y + first.z * first.z));
                second.w = NuFsqrt(1.0f - (second.x * second.x + second.y * second.y + second.z * second.z));
                if (first.x * second.x + first.y * second.y + first.z * second.z + first.w * second.w < 0.0f) {
                    second.x = -second.x;
                    second.y = -second.y;
                    second.z = -second.z;
                    second.w = -second.w;
                }
                NUQUAT result;
                result.x = first.x * (1.0f - fraction) + second.x * fraction;
                result.y = first.y * (1.0f - fraction) + second.y * fraction;
                result.z = first.z * (1.0f - fraction) + second.z * fraction;
                result.w = first.w * (1.0f - fraction) + second.w * fraction;
                f32 length =
                    NuFsqrt(result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z);
                f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                output[0] = result.x * inverse_length;
                output[1] = result.y * inverse_length;
                output[2] = result.z * inverse_length;
                output[3] = result.w * inverse_length;
            }
        }
    }
    return 0;
}

i32 ANI_SimpleAni3PlayerV4Joint_Quat3W(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, i32 joint_count,
                                       i32 first_joint) {

    const u8 *node_flags = anim->node_flags;
    buffer->use_quaternions = 1;
    u32 quarter;
    f32 fraction;
    i32 key_offset;
    if (anim->key_count == 1) {
        quarter = 0;
        fraction = 0.0f;
        key_offset = 0;
    } else {
        const i32 last = anim->key_count - 1;
        f32 key = (frame - anim->first_frame) * static_cast<f32>(last) / static_cast<f32>(anim->frame_count - 1);
        if (key < 0.0f)
            key = 0.0f;
        i32 whole;
        if (static_cast<f32>(last) <= key) {
            whole = last;
            fraction = 0.0f;
        } else {
            whole = static_cast<i32>(key);
            fraction = key - static_cast<f32>(whole);
        }
        quarter = static_cast<u32>(whole) & 3;
        key_offset = (whole / 4) * anim->key_stride;
    }
    u8 *keys = anim->keys + key_offset;
    i32 key_stride = anim->key_stride;
    const u16 *constants = reinterpret_cast<const u16 *>(anim->constants) - 16;
    ani3_scalemin_s *scale_min = anim->scale_min;
    const u16 *curve_types = anim->curve_types;
    // The original clamps the count before applying first_joint.
    i32 count = joint_count < anim->node_count ? joint_count : anim->node_count;
    const u8 *first_flags = node_flags + first_joint;
    for (const u8 *skip_flags = node_flags; skip_flags < first_flags; ++skip_flags) {
        for (i32 group = 0; group < 3; ++group) {
            i32 components = group == 1 ? 4 : 3;
            if (*skip_flags & CurveGroupMasks[group]) {
                for (i32 component = 0; component < components; ++component) {
                    SkipAni4V4Curve(*curve_types++, keys, scale_min);
                }
            } else
                curve_types += components;
        }
    }
    node_flags += first_joint;
    const u8 *end_flags = node_flags + count;
    u8 *joint_flags = buffer->joint_flags + first_joint;
    nuanimbuffjoint_s *joint = buffer->joints + first_joint;
    for (; node_flags < end_flags; ++node_flags, ++joint) {
        const u8 flags = *node_flags;
        *joint_flags++ = flags;
        f32 *output = &joint->translation.x;
        for (i32 group = 0; group < 3; ++group, output += 4) {
            if (!(flags & CurveGroupMasks[group])) {
                if (group == 0) {
                    output[0] = output[1] = output[2] = 0.0f;
                } else if (group == 1) {
                    output[0] = output[1] = output[2] = 0.0f;
                    output[3] = 1.0f;
                } else {
                    output[0] = output[1] = output[2] = 1.0f;
                }
                curve_types += group == 1 ? 4 : 3;
            } else if (group != 1) {
                output[0] = DecodeAni4Quat3Scalar(anim, key_stride, constants, *curve_types++, quarter, fraction, keys,
                                                  scale_min);
                output[1] = DecodeAni4Quat3Scalar(anim, key_stride, constants, *curve_types++, quarter, fraction, keys,
                                                  scale_min);
                output[2] = DecodeAni4Quat3Scalar(anim, key_stride, constants, *curve_types++, quarter, fraction, keys,
                                                  scale_min);
            } else {
                // This format stores w explicitly and interpolates the stored signs.
                NUQUAT first, second;
                DecodeAni4Quat3Pair(anim, key_stride, constants, *curve_types++, quarter, keys, scale_min, first.x,
                                    second.x);
                DecodeAni4Quat3Pair(anim, key_stride, constants, *curve_types++, quarter, keys, scale_min, first.y,
                                    second.y);
                DecodeAni4Quat3Pair(anim, key_stride, constants, *curve_types++, quarter, keys, scale_min, first.z,
                                    second.z);
                DecodeAni4Quat3Pair(anim, key_stride, constants, *curve_types++, quarter, keys, scale_min, first.w,
                                    second.w);
                NUQUAT result;
                result.x = first.x * (1.0f - fraction) + second.x * fraction;
                result.y = first.y * (1.0f - fraction) + second.y * fraction;
                result.z = first.z * (1.0f - fraction) + second.z * fraction;
                result.w = first.w * (1.0f - fraction) + second.w * fraction;
                f32 length =
                    NuFsqrt(result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z);
                f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                NUQUAT normalized = {result.x * inverse_length, result.y * inverse_length, result.z * inverse_length,
                                     result.w * inverse_length};
                *reinterpret_cast<NUQUAT *>(output) = normalized;
            }
        }
    }
    return 0;
}

static inline void DecodeAni4BlendQuat3Pair(const ani3_animheader_s *anim, i32 key_stride, const u16 *constants,
                                            u16 type, u32 quarter, u8 *&keys, ani3_scalemin_s *&scale_min, f32 &first,
                                            f32 &second) {
    if (type == 6) {
        const u32 first_word = *reinterpret_cast<const u32 *>(keys);
        const u32 next_word = *reinterpret_cast<const u32 *>(keys + key_stride);
        const f32 start = static_cast<f32>(first_word & 0xff);
        const f32 next = static_cast<f32>(next_word & 0xff);
        const u32 tangents = first_word >> 8;
        const f32 tangent0 = static_cast<f32>((tangents >> (quarter * 6)) & 0x3f) * 0.01587302f;
        first = ((next - start) * tangent0 + start) * scale_min->scale + scale_min->minimum;
        if (quarter == 3) {
            const f32 tangent1 = static_cast<f32>((next_word >> 8) & 0x3f) * 0.01587302f;
            const f32 after = static_cast<f32>(keys[key_stride * 2]);
            second = ((after - next) * tangent1 + next) * scale_min->scale + scale_min->minimum;
        } else {
            const f32 tangent1 = static_cast<f32>((tangents >> (((quarter * 3 + 3) * 2) & 0x1f)) & 0x3f) * 0.01587302f;
            second = ((next - start) * tangent1 + start) * scale_min->scale + scale_min->minimum;
        }
        keys += 4;
        ++scale_min;
        return;
    }

    if (type == 7) {
        CalcValue1648Get2Values(reinterpret_cast<char *>(keys), quarter, key_stride, scale_min, &first, &second);
        keys += 8;
        ++scale_min;
        return;
    }
    first = second = static_cast<f32>(static_cast<u32>(constants[type])) * anim->scale + anim->minimum;
}

i32 ANI_SimpleAni3PlayerV4Joint_Blend_Quat3(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend,
                                            i32 joint_count, i32 first_joint, NUVEC *root_translation) {
    const u8 *node_flags = anim->node_flags;
    const f32 inverse_blend = 1.0f - blend;
    f32 key =
        (frame - anim->first_frame) * static_cast<f32>(anim->key_count - 1) / static_cast<f32>(anim->frame_count - 1);
    if (key < 0.0f)
        key = 0.0f;
    if (static_cast<f32>(anim->key_count) <= key)
        key = static_cast<f32>(anim->key_count - 1);
    const i32 whole = static_cast<i32>(key);
    const u32 quarter = static_cast<u32>(whole) & 3;
    const f32 fraction = key - static_cast<f32>(whole);
    const i32 key_stride = anim->key_stride;
    u8 *keys = anim->keys + (whole / 4) * key_stride;
    ani3_scalemin_s *scale_min = anim->scale_min;
    const u16 *constants = reinterpret_cast<const u16 *>(anim->constants) - 16;
    const u16 *curve_types = anim->curve_types;
    const i32 count = joint_count < anim->node_count ? joint_count : anim->node_count;
    if (first_joint != 0)
        root_translation = NULL;
    const u8 *first_flags = node_flags + first_joint;
    for (const u8 *skip_flags = node_flags; skip_flags < first_flags; ++skip_flags) {
        if (*skip_flags & 2) {
            SkipAni4V4Curve(curve_types[0], keys, scale_min);
            SkipAni4V4Curve(curve_types[1], keys, scale_min);
            SkipAni4V4Curve(curve_types[2], keys, scale_min);
        }
        if (*skip_flags & 1) {
            SkipAni4V4Curve(curve_types[3], keys, scale_min);
            SkipAni4V4Curve(curve_types[4], keys, scale_min);
            SkipAni4V4Curve(curve_types[5], keys, scale_min);
        }
        if (*skip_flags & 8) {
            SkipAni4V4Curve(curve_types[6], keys, scale_min);
            SkipAni4V4Curve(curve_types[7], keys, scale_min);
            SkipAni4V4Curve(curve_types[8], keys, scale_min);
        }
        curve_types += 9;
    }
    node_flags += first_joint;
    const u8 *end_flags = node_flags + count;
    u8 *joint_flags = buffer->joint_flags + first_joint;
    f32 *output = &buffer->joints[first_joint].translation.x;
    for (; node_flags < end_flags; ++node_flags) {
        const u8 flags = *node_flags;
        *joint_flags++ |= flags;
        for (i32 group = 0; group < 3; ++group, curve_types += 3, output += 4) {
            if (!(flags & CurveGroupMasks[group])) {
                if (group == 0) {
                    output[0] *= inverse_blend;
                    output[1] *= inverse_blend;
                    output[2] *= inverse_blend;
                    if (root_translation) {
                        root_translation->x = root_translation->y = root_translation->z = 0.0f;
                        root_translation = NULL;
                    }
                } else if (group == 1) {
                    const f32 blended_w = output[3] * inverse_blend + blend;
                    output[0] *= inverse_blend;
                    output[1] *= inverse_blend;
                    output[3] = blended_w;
                    output[2] *= inverse_blend;
                    f32 length = NuFsqrt(output[3] * output[3] + output[0] * output[0] + output[1] * output[1] +
                                         output[2] * output[2]);
                    f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                    output[3] *= inverse_length;
                    output[0] *= inverse_length;
                    output[1] *= inverse_length;
                    output[2] *= inverse_length;
                } else {
                    output[0] = output[0] * inverse_blend + blend;
                    output[1] = output[1] * inverse_blend + blend;
                    output[2] = output[2] * inverse_blend + blend;
                }
            } else if (group != 1) {
                f32 sampled = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[0], quarter, fraction,
                                                    keys, scale_min);
                f32 delta = sampled - output[0];
                if (root_translation)
                    root_translation->x = sampled;
                output[0] += delta * blend;
                sampled = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[1], quarter, fraction, keys,
                                                scale_min);
                delta = sampled - output[1];
                if (root_translation)
                    root_translation->y = sampled;
                output[1] += delta * blend;
                sampled = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[2], quarter, fraction, keys,
                                                scale_min);
                delta = sampled - output[2];
                if (root_translation)
                    root_translation->z = -sampled;
                output[2] += delta * blend;
                root_translation = NULL;
            } else {
                NUQUAT first, second, result;
                DecodeAni4BlendQuat3Pair(anim, key_stride, constants, curve_types[0], quarter, keys, scale_min, first.x,
                                         second.x);
                DecodeAni4BlendQuat3Pair(anim, key_stride, constants, curve_types[1], quarter, keys, scale_min, first.y,
                                         second.y);
                DecodeAni4BlendQuat3Pair(anim, key_stride, constants, curve_types[2], quarter, keys, scale_min, first.z,
                                         second.z);
                first.w = NuFsqrt(1.0f - (first.x * first.x + first.y * first.y + first.z * first.z));
                second.w = NuFsqrt(1.0f - (second.x * second.x + second.y * second.y + second.z * second.z));
                NuQuatHarmonize(&first, &second);
                result.x = first.x * (1.0f - fraction) + second.x * fraction;
                result.y = first.y * (1.0f - fraction) + second.y * fraction;
                result.z = first.z * (1.0f - fraction) + second.z * fraction;
                result.w = first.w * (1.0f - fraction) + second.w * fraction;
                f32 length =
                    NuFsqrt(result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z);
                f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                result.w *= inverse_length;
                result.x *= inverse_length;
                result.y *= inverse_length;
                result.z *= inverse_length;
                VuQuatSlerpFast(reinterpret_cast<NUQUAT *>(output), reinterpret_cast<NUQUAT *>(output), &result, blend);
            }
        }
    }
    return 0;
}

static inline void DecodeAni4BlendQuat3WPair(const ani3_animheader_s *anim, i32 key_stride, const u16 *constants,
                                             u16 type, u32 quarter, u8 *&keys, ani3_scalemin_s *&scale_min, f32 &first,
                                             f32 &second) {
    if (type == 6) {
        const u32 first_word = *reinterpret_cast<const u32 *>(keys);
        const u32 next_word = *reinterpret_cast<const u32 *>(keys + key_stride);
        const f32 start = static_cast<f32>(first_word & 0xff);
        const f32 next = static_cast<f32>(next_word & 0xff);
        const u32 tangents = first_word >> 8;
        const f32 tangent0 = static_cast<f32>((tangents >> (quarter * 6)) & 0x3f) * 0.01587302f;
        first = ((next - start) * tangent0 + start) * scale_min->scale + scale_min->minimum;
        if (quarter == 3) {
            const f32 tangent1 = static_cast<f32>((next_word >> 8) & 0x3f) * 0.01587302f;
            const f32 after = static_cast<f32>(keys[key_stride * 2]);
            second = ((after - next) * tangent1 + next) * scale_min->scale + scale_min->minimum;
        } else {
            const f32 tangent1 = static_cast<f32>((tangents >> (((quarter * 3 + 3) * 2) & 0x1f)) & 0x3f) * 0.01587302f;
            second = ((next - start) * tangent1 + start) * scale_min->scale + scale_min->minimum;
        }
        keys += 4;
        ++scale_min;
        return;
    }

    if (type == 7) {
        CalcValue1648Get2Values(reinterpret_cast<char *>(keys), quarter, key_stride, scale_min, &first, &second);
        keys += 8;
        ++scale_min;
        return;
    }
    first = second = static_cast<f32>(static_cast<u32>(constants[type])) * anim->scale + anim->minimum;
}
i32 ANI_SimpleAni3PlayerV4Joint_Blend_Quat3W(ani3_animheader_s *anim, f32 frame, nuanimbuff_s *buffer, f32 blend,
                                             i32 joint_count, i32 first_joint, NUVEC *root_translation) {
    const u8 *node_flags = anim->node_flags;
    const f32 inverse_blend = 1.0f - blend;
    f32 key =
        (frame - anim->first_frame) * static_cast<f32>(anim->key_count - 1) / static_cast<f32>(anim->frame_count - 1);
    if (key < 0.0f)
        key = 0.0f;
    // Preserve the fractional interval after the last integer key.
    if (static_cast<f32>(anim->key_count) <= key)
        key = static_cast<f32>(anim->key_count - 1);
    const i32 whole = static_cast<i32>(key);
    const u32 quarter = static_cast<u32>(whole) & 3;
    const f32 fraction = key - static_cast<f32>(whole);
    const i32 key_stride = anim->key_stride;
    u8 *keys = anim->keys + (whole / 4) * key_stride;
    ani3_scalemin_s *scale_min = anim->scale_min;
    const u16 *constants = reinterpret_cast<const u16 *>(anim->constants) - 16;
    const u16 *curve_types = anim->curve_types;
    const i32 count = joint_count < anim->node_count ? joint_count : anim->node_count;
    if (first_joint != 0)
        root_translation = NULL;
    const u8 *first_flags = node_flags + first_joint;
    for (const u8 *skip_flags = node_flags; skip_flags < first_flags; ++skip_flags) {
        for (i32 group = 0; group < 3; ++group) {
            i32 components = group == 1 ? 4 : 3;
            if (*skip_flags & CurveGroupMasks[group]) {
                for (i32 component = 0; component < components; ++component) {
                    SkipAni4V4Curve(*curve_types++, keys, scale_min);
                }
            } else
                curve_types += components;
        }
    }
    node_flags += first_joint;
    const u8 *end_flags = node_flags + count;
    u8 *joint_flags = buffer->joint_flags + first_joint;
    nuanimbuffjoint_s *joint = buffer->joints + first_joint;
    for (; node_flags < end_flags; ++node_flags, ++joint) {
        const u8 flags = *node_flags;
        *joint_flags++ |= flags;
        f32 *output = &joint->translation.x;
        for (i32 group = 0; group < 3; ++group, output += 4) {
            if (!(flags & CurveGroupMasks[group])) {
                if (group == 0) {
                    output[0] *= inverse_blend;
                    output[1] *= inverse_blend;
                    output[2] *= inverse_blend;
                    if (root_translation) {
                        root_translation->x = root_translation->y = root_translation->z = 0.0f;
                        root_translation = NULL;
                    }
                } else if (group == 1) {
                    output[3] = output[3] * inverse_blend + blend;
                    output[0] *= inverse_blend;
                    output[1] *= inverse_blend;
                    output[2] *= inverse_blend;
                    f32 length = NuFsqrt(output[3] * output[3] + output[0] * output[0] + output[1] * output[1] +
                                         output[2] * output[2]);
                    f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                    output[3] *= inverse_length;
                    output[0] *= inverse_length;
                    output[1] *= inverse_length;
                    output[2] *= inverse_length;
                } else {
                    output[0] = output[0] * inverse_blend + blend;
                    output[1] = output[1] * inverse_blend + blend;
                    output[2] = output[2] * inverse_blend + blend;
                }
                curve_types += group == 1 ? 4 : 3;
            } else if (group != 1) {
                f32 sampled = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[0], quarter, fraction,
                                                    keys, scale_min);
                f32 delta_x = sampled - output[0];
                if (root_translation)
                    root_translation->x = sampled;
                output[0] += delta_x * blend;
                sampled = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[1], quarter, fraction, keys,
                                                scale_min);
                f32 delta_y = sampled - output[1];
                if (root_translation)
                    root_translation->y = sampled;
                output[1] += delta_y * blend;
                sampled = DecodeAni4Quat3Scalar(anim, key_stride, constants, curve_types[2], quarter, fraction, keys,
                                                scale_min);
                f32 delta_z = sampled - output[2];
                if (root_translation)
                    root_translation->z = -sampled;
                output[2] += delta_z * blend;
                root_translation = NULL;
                curve_types += 3;
            } else {
                NUQUAT first, second, result;
                DecodeAni4BlendQuat3WPair(anim, key_stride, constants, curve_types[0], quarter, keys, scale_min,
                                          first.x, second.x);
                DecodeAni4BlendQuat3WPair(anim, key_stride, constants, curve_types[1], quarter, keys, scale_min,
                                          first.y, second.y);
                DecodeAni4BlendQuat3WPair(anim, key_stride, constants, curve_types[2], quarter, keys, scale_min,
                                          first.z, second.z);
                DecodeAni4BlendQuat3WPair(anim, key_stride, constants, curve_types[3], quarter, keys, scale_min,
                                          first.w, second.w);
                curve_types += 4;
                result.x = first.x * (1.0f - fraction) + second.x * fraction;
                result.y = first.y * (1.0f - fraction) + second.y * fraction;
                result.z = first.z * (1.0f - fraction) + second.z * fraction;
                result.w = first.w * (1.0f - fraction) + second.w * fraction;
                f32 length =
                    NuFsqrt(result.w * result.w + result.x * result.x + result.y * result.y + result.z * result.z);
                f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                result.w *= inverse_length;
                result.x *= inverse_length;
                result.y *= inverse_length;
                result.z *= inverse_length;
                // Original W blending writes this local result and discards it.
                VuQuatSlerpFast(&result, reinterpret_cast<NUQUAT *>(output), &result, blend);
            }
        }
    }
    return 0;
}

void EvalAnim(nuhspecial_s *special, float frame, numtx_s *matrix, i32 include_instance_translation) {
    if (matrix == NULL || special == NULL) {
        return;
    }

    nuinstanim_s *instance_animation = NuSpecialGetInstAnim(special);
    if (instance_animation == NULL) {
        if (include_instance_translation != 0) {
            NUMTX *instance_matrix = NuSpecialGetInstanceMtx(special);
            if (instance_matrix != NULL) {
                memcpy(matrix, instance_matrix, sizeof(NUMTX));
            }
        }
        return;
    }

    NUGSCN *scene = special->scene;
    nuanimdata_s *animation = scene->instance_animation_data[instance_animation->anim_ix];
    if (animation == NULL) {
        return;
    }

    NuAnimData2CalcMatrix(animation, 0, frame, matrix);
    if (include_instance_translation != 0) {
        NUMTX *instance_matrix = NuSpecialGetInstanceMtx(special);
        if (instance_matrix != NULL) {
            NUVEC *translation = NUMTX_GET_ROW_VEC(matrix, 3);
            const NUVEC *instance_translation = NUMTX_GET_ROW_VEC(instance_matrix, 3);
            translation->x += instance_translation->x;
            translation->y += instance_translation->y;
            translation->z += instance_translation->z;
        }
    }
}

void EvalAnim2(nuhspecial_s *special, float frame) {
    nuinstanim_s *instance_animation = NuSpecialGetInstAnim(special);
    if (instance_animation == NULL) {
        return;
    }

    NUGSCN *scene = special->scene;
    nuanimdata_s *animation = scene->instance_animation_data[instance_animation->anim_ix];
    if (animation == NULL || frame == instance_animation->prev_eval_time) {
        return;
    }

    NUMTX *instance_matrix = NuSpecialGetInstanceMtx(special);
    NuAnimData2CalcMatrix(animation, 0, frame, &instance_animation->mtx);

    instance_animation->prev_eval_time = frame;
    const i32 instance_index = instance_animation - scene->instance_animations;
    NUVEC &animation_translation = *NUMTX_GET_ROW_VEC(&instance_animation->mtx, 3);
    NUVEC &instance_translation = *NUMTX_GET_ROW_VEC(instance_matrix, 3);
    NUMTX *evaluated_matrix = &scene->instance_animation_matrices[instance_index];
    memcpy(evaluated_matrix, &instance_animation->mtx, sizeof(NUMTX));

    animation_translation.x += instance_translation.x;
    animation_translation.y += instance_translation.y;
    animation_translation.z += instance_translation.z;
    NuSpecialUpdate(special);
}

void GameAnimSys_AllocateLevelProgressData(variptr_u *buf, variptr_u *buf_end, i32 capacity, i32 level_count) {
    if (buf_end == NULL || buf == NULL)
        return;
    gameanimsysprogress.count = level_count;
    gameanimsysprogress.entry_size = capacity;
    gameanimsysprogress.entries = static_cast<u8 **>(GameBufferAlloc(buf, buf_end, level_count * sizeof(u8 *)));
    if (gameanimsysprogress.entries != NULL) {
        for (i32 i = 0; i < level_count; ++i)
            gameanimsysprogress.entries[i] = static_cast<u8 *>(GameBufferAlloc(buf, buf_end, capacity));
    }
}

u8 *GameAnimSys_GetProgressData(i32 index) {
    if (index < 0 || index >= gameanimsysprogress.count)
        return NULL;
    return gameanimsysprogress.entries[index];
}

void GameAnimSys_StoreProgress(GAMEANIMSYS_s *system, i32 index) {
    if (system == NULL || system->sets == NULL || index < 0 || index >= gameanimsysprogress.count)
        return;
    u8 *progress = gameanimsysprogress.entries[index];
    for (i32 i = 0; i < gameanimsysprogress.entry_size && system->sets[i] != NULL; ++i)
        progress[i] = system->sets[i]->state;
}

void GameAnimSys_ReStoreProgress(GAMEANIMSYS_s *system, i32 index) {
    if (system == NULL || system->sets == NULL || index < 0 || index >= gameanimsysprogress.count)
        return;
    u8 *progress = gameanimsysprogress.entries[index];
    for (i32 i = 0; i < gameanimsysprogress.entry_size && system->sets[i] != NULL; ++i) {
        GAMEANIMSET_s *set = system->sets[i];
        set->state = static_cast<GAMEANIMSET_STATE>(static_cast<i8>(progress[i]));
        if (set->state == GAMEANIMSET_STATE_ACTIVE_FORWARD || set->state == GAMEANIMSET_STATE_ACTIVE_BACKWARD) {
            if ((set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) == 0)
                GameAnimSet_AddToSystemList(set);
        } else if ((set->flags & GAMEANIMSET_FLAG_IN_SYSTEM_LIST) != 0)
            GameAnimSet_RemoveFromSystemList(set);
    }
}

GAMEANIMSYS_s *GameAnimSys_Create(variptr_u *buf, variptr_u *buf_end) {
    GAMEANIMSYS_s *system = static_cast<GAMEANIMSYS_s *>(GameBufferAlloc(buf, buf_end, sizeof(GAMEANIMSYS_s)));
    if (system != NULL && gameanimsysprogress.entry_size != 0) {
        system->sets = static_cast<GAMEANIMSET_s **>(
            GameBufferAlloc(buf, buf_end, gameanimsysprogress.entry_size * sizeof(GAMEANIMSET_s *)));
    }
    return system;
}
