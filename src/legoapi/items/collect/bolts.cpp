#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/render/fx.h"
#include "globals.h"
#include "legoapi/world/area.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/items/collect/bolts.h"
#include "decomp.h"
#include "nu2api/nucore/nustring.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nu3d/nuspecial.h"
#include "gameapi/edtools/edfile.h"
#include "gamelib/util/gamelib_util_types.h"
#include "nu2api/numusic/sfx.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include <string.h>

extern BOLT_s Bolt[32];
extern i32 i_bolt;
extern f32 BOLT_OVERRIDE_PLAYERBOLTSPEED;
extern f32 BOLT_OVERRIDE_PLAYERBOLTDURATION;
struct spacelevel_s;
struct quickboltinfo;

static void Bolt_Debris_Default(BOLT_s *, NUVEC *, i32, NUVEC *, i32);
static void Bolt_GetShootOrigin_Default(GameObject_s *, NUVEC *);
static i32 Bolt_GetShootDirection_Default(GameObject_s *, NUVEC *);
static void UpdateBolt_Geonosian(BOLT_s *);
static void EndBolt_EwokTorpedo(BOLT_s *);
void Torpedo_InitBolt(BOLT_s *);
void Torpedo_UpdateBolt(BOLT_s *);
void Torpedo_EndBolt(BOLT_s *);
void Torpedo_InitRicochet(BOLT_s *, NUVEC *);
f32 Torpedo_Scale(BOLT_s *);

#include "legoapi/items/collect/bolttypes_lsw.inc"

BOLTTYPE_s GlobalBoltType_Default = {"null", 4.0f, 2.0f, 0.0f, 0.125f, 1.0f, 0.1f, -1,   -1,   -1,   0, -1, 0,  1,
                                     255,    0,    0,    NULL, NULL,   NULL, NULL, NULL, NULL, NULL, 0, -1, -1, {}};
static BOLTSYS BoltSys_Default = {
    &GlobalBoltType_Default,        1,    NULL, Bolt_Debris_Default, Bolt_GetShootOrigin_Default,
    Bolt_GetShootDirection_Default, NULL, NULL};
BOLTSYS *BoltSys = &BoltSys_Default;

BOLT_s *Bolt_Alloc() {
    i32 index = i_bolt;
    i32 attempts = 0;
    while (Bolt[index].active != 0 && (Bolt[index].flags & 8) != 0 && attempts < 32) {
        ++index;
        if (index == 32)
            index = 0;
        ++attempts;
    }
    i_bolt = (index + 1) % 32;
    return &Bolt[index];
}

extern i32 addbolt_newsfx;
extern NUVEC addbolt_newpos;
f32 BOLT_SHOOTFLASHTIME = 0.1f;
f32 Bolt_ObjTargetPosYAdjust(GameObject_s *);
void FindAnglesXY(NUVEC *, u16 *, u16 *);
void CalculateInterceptVector(NUVEC *, NUVEC *, NUVEC *, f32, NUVEC *, NUVEC *);
void GameAudio_PlaySfxById(i32, NUVEC *, i32, i32);
i16 LEGOACT_SHOOTBACK = -1;
i16 LEGOACT_SHOOTLEFT = -1;
i16 LEGOACT_SHOOTRIGHT = -1;

void Bolt_Shoot(GameObject_s *object, i32 type_id, i32 fire_flags) {
    BOLTTYPE_s *type = BoltType_FindByID(type_id, WORLD);
    u32 flags = type->field_60;
    f32 speed = type->field_10;
    f32 duration = type->field_14;
    if (static_cast<i8>(object->apiobj.flags_low) < 0) {
        flags |= 0x20;
        if (object->apiobj.field_0x27c == 0)
            flags |= 1;
        else if (object->apiobj.field_0x27c == 1)
            flags |= 2;
    }
    if ((object->field_0xefc & 8) != 0)
        flags |= 0x40;
    NUVEC aimed_position = object->attack_target_position;
    NUVEC origin, direction;
    BoltSys->shoot_origin(object, &origin);
    u16 heading = BoltSys->shoot_direction(object, &direction);
    if (object->character_context != -1 && object->context_animation != -1) {
        if (object->context_animation == LEGOACT_SHOOTRIGHT)
            heading += 0x4000;
        else if (object->context_animation == LEGOACT_SHOOTLEFT)
            heading -= 0x4000;
        else if (object->context_animation == LEGOACT_SHOOTBACK)
            heading += 0x8000;
    }
    NUVEC adjusted_position, target_velocity;
    f32 target_distance = 0.0f;
    if ((object->field_0xe21 & 8) != 0) {
        if (static_cast<i8>(object->field_0xef9) >= 0 && object->script_fire_target == NULL &&
            object->apiobj.field_0x27c != -1 &&
            (object->attack_target_velocity.x != 0.0f || object->attack_target_velocity.y != 0.0f ||
             object->attack_target_velocity.z != 0.0f)) {
            target_distance = duration * speed;
            NuVecSub(&adjusted_position, &aimed_position, &origin);
            NuVecNorm(&adjusted_position, &adjusted_position);
            NuVecScale(&adjusted_position, &adjusted_position, target_distance);
            NuVecAdd(&aimed_position, &origin, &adjusted_position);
        } else {
            target_distance = NuVecDist(&origin, &aimed_position, NULL);
        }
    }
    NUVEC *target_position = NULL;
    NUVEC *intercept_position = NULL;
    NUVEC *velocity = NULL;
    if (static_cast<i8>(object->field_0xef9) < 0) {
        target_position = intercept_position = &object->field_0xe58;
        if ((object->field_0xefa & 3) == 2) {
            NuVecSub(&target_velocity, &object->field_0xe58, &object->field_0xe64);
            NuVecScale(&target_velocity, &target_velocity, 1.0f / FRAMETIME);
            velocity = &target_velocity;
        }
    } else if (object->script_fire_target != NULL) {
        GameObject_s *target = object->script_fire_target;
        velocity = &target->apiobj.velocity;
        target_position = intercept_position = &target->apiobj.collision_position;
        if ((object->apiobj.character_data->model_flags & 0x2000) == 0 && (object->apiobj.field_0x1f4 & 1) != 0) {
            adjusted_position = target->apiobj.collision_position;
            adjusted_position.y += Bolt_ObjTargetPosYAdjust(target);
            target_position = &adjusted_position;
        }
    } else if ((object->field_0xe21 & 8) != 0) {
        target_position = intercept_position = &aimed_position;
    }
    i16 sounds[8];
    NUVEC sound_positions[8];
    i32 sound_count = 0;
    i32 bolts[5], bolt_count = 0;
    i32 used_locator = 0;
    for (i32 index = 0; index < 5; ++index) {
        if ((index == 1 && fire_flags == 1) || (index == 0 && fire_flags == 2))
            continue;
        if (BoltSys->alternate_fire != NULL && BoltSys->alternate_fire(object, index) == 0)
            continue;
        NUVEC *shot_position;
        if (index == 4) {
            if (used_locator != 0)
                break;
            shot_position = &origin;
        } else {
            if (object->apiobj.model_draw_result == 0)
                continue;
            GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
            i32 locator = data->weapon_shoot_joints[index];
            if (locator == -1 || object->apiobj.character_model->points_of_interest[locator] == NULL) {
                locator = data->weapon_joints[index];
                if (locator == -1 || object->apiobj.character_model->points_of_interest[locator] == NULL)
                    continue;
            }
            used_locator = 1;
            shot_position = reinterpret_cast<NUVEC *>(&object->joint_matrices[locator].m30);
        }
        NUMTX matrix;
        if ((flags & 0x600000) != 0) {
            i32 pitch = (flags & 0x200000) != 0 ? 0xf8e4 : 0xf1c8;
            if (static_cast<i32>(flags) < 0 && static_cast<i8>(object->apiobj.flags_low) >= 0 &&
                object->script_fire_target != NULL) {
                f32 distance = NuVecXZDist(&object->script_fire_target->apiobj.collision_position,
                                           &object->apiobj.collision_position, NULL);
                if (distance > 2.0f)
                    distance = 2.0f;
                f32 degrees = (flags & 0x200000) != 0 ? -10.0f : -20.0f;
                pitch = static_cast<u16>(static_cast<i32>(((degrees * distance) * 0.5f * 65536.0f) / 360.0f));
            }
            if (target_position != NULL)
                heading = NuAtan2D(target_position->x - shot_position->x, target_position->z - shot_position->z);
            NUANGVEC angles = {pitch, heading, 0};
            NuMtxSetRotationXYVU0(&matrix, &angles);
        } else if (target_position != NULL) {
            NUVEC delta;
            if (velocity != NULL && (object->field_0xefa & 2) != 0)
                CalculateInterceptVector(shot_position, intercept_position, velocity, speed, &delta, NULL);
            else
                NuVecSub(&delta, target_position, shot_position);
            FindAnglesXY(&delta, NULL, NULL);
            NUANGVEC angles = {static_cast<u16>(temp_xrot), static_cast<u16>(temp_yrot), 0};
            NuMtxSetRotationXYVU0(&matrix, &angles);
        } else if (object->field_0x1086 == 4) {
            FindAnglesXY(&direction, NULL, NULL);
            NUANGVEC angles = {static_cast<u16>(temp_xrot), static_cast<u16>(temp_yrot), 0};
            NuMtxSetRotationXYVU0(&matrix, &angles);
        } else {
            NuMtxSetRotationY(&matrix, heading);
        }
        addbolt_nosfx = 1;
        BOLT_s *bolt = Bolt_Add(object, shot_position, &matrix, type_id, 0);
        if (bolt != NULL)
            bolts[bolt_count++] = bolt->index;
        if (addbolt_newsfx != -1 && sound_count < 8) {
            sounds[sound_count] = addbolt_newsfx;
            sound_positions[sound_count++] = addbolt_newpos;
        }
        if ((type->field_60 & 0x20000000) != 0)
            object->timer_d50 = BOLT_SHOOTFLASHTIME;
    }
    object->field_0xe22 |= 4;
    if (sound_count != 0) {
        i32 index = sound_count == 1 ? 0 : qrand() / (0xffff / sound_count + 1);
        GameAudio_PlaySfxById(sounds[index], &sound_positions[index], 0, 0);
    }
    if ((object->field_0xe21 & 8) != 0 && bolt_count > 1) {
        f32 duration_scale = type->field_14;
        for (i32 index = 0; index < bolt_count; ++index) {
            BOLT_s *bolt = &Bolt[bolts[index]];
            f32 factor = 0.0f;
            if (target_distance != 0.0f) {
                f32 range = bolt->speed * bolt->lifetime;
                if (range != 0.0f)
                    factor = target_distance / range;
            }
            f32 lifetime = factor * duration_scale;
            if (bolt->lifetime > lifetime)
                bolt->lifetime = lifetime;
        }
    }
}

i32 MatrixReflectionVU0_AXISY(NUMTX *, f32, f32, NUMTX *);
void Bolt_End(BOLT_s *, i32);
extern i32 Paused;

void Bolts_Draw(WORLDINFO_s *world) {
    NUVEC scale = {0.0f, 0.0f, 0.0f};
    for (BOLT_s *bolt = Bolt; bolt != Bolt + 32; ++bolt) {
        if (!bolt->active)
            continue;
        BOLTTYPE_s *type = bolt->type;
        f32 size = type->scale_callback != NULL ? type->scale_callback(bolt) : bolt->scale;
        NUMTX matrix = bolt->effect_orientation;
        if (bolt->time < type->field_24) {
            scale.y = (type->field_60 & 0x2000000) != 0 ? size : (bolt->time / type->field_24) * size;
            scale.x = scale.z = (bolt->flags & 0x200) != 0 ? scale.y : size;
            NuMtxPreScale(&matrix, &scale);
        } else if (bolt->scale != 1.0f) {
            scale.x = scale.y = scale.z = size;
            NuMtxPreScale(&matrix, &scale);
        }
        NuMtxTranslate(&matrix, &bolt->position);
        i32 first = 0, second = 0;
        if (NuSpecialExistsFn(type->pad_68))
            first = NuSpecialDrawAt(type->pad_68, &matrix);
        if (NuSpecialExistsFn(type->pad_68 + 12))
            second = NuSpecialDrawAt(type->pad_68 + 12, &matrix);
        if (bolt->field_0xe8 != 2000000.0f) {
            NUMTX reflection;
            if (MatrixReflectionVU0_AXISY(&matrix, bolt->field_0xe8, world->current_level->unknown_0cc, &reflection)) {
                if (NuSpecialExistsFn(type->pad_68 + 24))
                    NuSpecialDrawAt(type->pad_68 + 24, &reflection);
                if (NuSpecialExistsFn(type->pad_68 + 36))
                    NuSpecialDrawAt(type->pad_68 + 36, &reflection);
            }
        }
        if (bolt->field_0xe4 != 2000000.0f) {
            scale.x = scale.z = scale.y;
            NuMtxSetScale(&matrix, &scale);
            NuMtxRotateZ(&matrix, bolt->surface_z_rotation);
            NuMtxRotateX(&matrix, bolt->surface_x_rotation);
            matrix.m30 = bolt->position.x;
            matrix.m31 = bolt->field_0xe4 + 0.005f;
            matrix.m32 = bolt->position.z;
            NuSpecialDrawAt(type->pad_68 + 48, &matrix);
        }
        if (first != 0 || second != 0)
            bolt->field_0x102 = 1;
        else if (!Paused && bolt->field_0x102 != 0 && (bolt->flags & 0x20) == 0)
            Bolt_End(bolt, 0);
    }
}

void Bolts_Reset() {
    memset(Bolt, 0, sizeof(Bolt));
    i_bolt = 0;
    BOLT_OVERRIDE_PLAYERBOLTSPEED = 0.0f;
    BOLT_OVERRIDE_PLAYERBOLTDURATION = 0.0f;
}

void BoltSys_Init(BOLTSYS *system) {
    for (i32 i = 0; i < system->count; ++i) {
        BOLTTYPE_s *type = &system->types[i];
        type->shoot_sfx_id = -1;
        if (type->shoot_sfx != NULL) {
            type->shoot_sfx_id = GetSfxId(type->shoot_sfx);
            type = &system->types[i];
        }
        type->hit_sfx_id = -1;
        if (type->hit_sfx != NULL)
            type->hit_sfx_id = GetSfxId(type->hit_sfx);
    }
    BoltSys = system;
    if (system->debris == NULL)
        system->debris = Bolt_Debris_Default;
    if (system->shoot_origin == NULL)
        system->shoot_origin = Bolt_GetShootOrigin_Default;
    if (system->shoot_direction == NULL)
        system->shoot_direction = Bolt_GetShootDirection_Default;
}

void Bolt_Reflect(nuvec_s *normal, nuvec_s *incoming, nuvec_s *out) {
    f32 dot = normal->x * incoming->x + normal->y * incoming->y + normal->z * incoming->z;
    f32 twice_dot = dot + dot;
    f32 x = incoming->x - normal->x * twice_dot;
    f32 y = incoming->y - normal->y * twice_dot;
    f32 z = incoming->z - normal->z * twice_dot;
    out->x = x;
    out->y = y;
    out->z = z;
}

extern "C" {
    void KillPart(PART_s *, i32);
}
i32 SphereSphereOverlapScaleY(NUVEC *, f32, f32, NUVEC *, f32, f32);
void GameAudio_PlaySfxById(i32, NUVEC *, i32, i32);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);

void Bolt_PlayHitSfx(BOLT_s *bolt) {
    if (bolt->type->hit_sfx_id != -1)
        GameAudio_PlaySfxById(bolt->type->hit_sfx_id, &bolt->position, 0, 0);
    else
        GameAudio_PlaySfx(0x28, &bolt->position, 0, 0);
}
void Bolt_End(BOLT_s *, i32);
void NewRumble(nupad_s *, f32, i32);

PART_s *Bolt_HitParts(BOLT_s *bolt, NUVEC *points, NUVEC *minimum, NUVEC *maximum, f32 radius, i32 reason) {
    PART_s *part = Part;
    for (i32 i = 0; i < MAXPARTS; ++i, ++part) {
        if ((part->active & 1) == 0)
            continue;
        if ((part->flags & 0x08000000) == 0 && (part->flags & 0xa) != 0xa)
            continue;
        if (minimum->x > part->bounds_max.x || part->bounds_min.x > maximum->x || minimum->z > part->bounds_max.z ||
            part->bounds_min.z > maximum->z || minimum->y > part->bounds_max.y || part->bounds_min.y > maximum->y)
            continue;
        i32 hit;
        if ((bolt->flags & 0x200) == 0 &&
            SphereSphereOverlapScaleY(&part->position, part->field_0e4, part->field_0e4, &points[2], radius, radius))
            hit = 2;
        else if (SphereSphereOverlapScaleY(&part->position, part->field_0e4, part->field_0e4, &points[1], radius,
                                           radius))
            hit = 1;
        else if ((bolt->flags & 0x200) == 0 && SphereSphereOverlapScaleY(&part->position, part->field_0e4,
                                                                         part->field_0e4, &points[0], radius, radius))
            hit = 0;
        else
            continue;
        BoltSys->debris(bolt, points, hit, &part->velocity, 0);
        Bolt_PlayHitSfx(bolt);
        if (bolt->owner != NULL && (bolt->owner->apiobj.flags_low & 1) != 0)
            NewRumble(bolt->owner->pad_gamepad->pad, 0.7f, 0);
        Bolt_End(bolt, 1);
        if (BoltSys->hit_part != NULL && BoltSys->hit_part(bolt, part) == 1)
            return part;
        KillPart(part, reason);
        return part;
    }
    return NULL;
}

void BoltTypes_Reset(WORLDINFO_s *world) {
    memset(world->bolt_types, 0, sizeof(world->bolt_types));
}

void AddPartDebris(PARTDEBSYS_s *, i32, NUVEC *);
extern "C" void PlaySfx(char *, NUVEC *);
i32 Player_HasDoubleBoltDamage_FromBolt(BOLT_s *);
void GameCam_NewShake(GAMECAMERA_s *, f32, f32, f32);
void GameCam_HitJudder();
void NewRumbleAllPlayers(f32, f32, i32, i32);
EXPLOSION *AddExplosion(NUVEC *, f32, f32, GameObject_s *, i32, i32);

void Bolt_Debris_LSW(BOLT_s *bolt, nuvec_s *points, i32 point, nuvec_s *, i32 no_explosion) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    i32 effect;
    if (world->current_level == DOGFIGHTA_LDATA) {
        if (bolt->field_0x104 != -1)
            points = &bolt->dogfight_hit_position;
        effect = 0x2e;
    } else {
        effect = bolt->hit_debris;
    }
    if (((bolt->flags & 0x40000200) == 0 && point == -1) || point == 0) {
        AddGameDebris(world->debris_sys, effect, &points[0]);
        if (!(bolt->flags & 0x10000000) && Player_HasDoubleBoltDamage_FromBolt(bolt))
            AddGameDebris(world->debris_sys, effect, &points[0]);
    }
    if (point == -1 || point == 1) {
        AddGameDebris(world->debris_sys, effect, &points[1]);
        if (!(bolt->flags & 0x10000000) && Player_HasDoubleBoltDamage_FromBolt(bolt))
            AddGameDebris(world->debris_sys, effect, &points[1]);
    }
    if (((bolt->flags & 0x40000200) == 0 && point == -1) || point == 2) {
        AddGameDebris(world->debris_sys, effect, &points[2]);
        if (!(bolt->flags & 0x10000000) && Player_HasDoubleBoltDamage_FromBolt(bolt))
            AddGameDebris(world->debris_sys, effect, &points[2]);
    }
    if (bolt->hit_part_debris != -1)
        AddPartDebris(world->part_debris_sys, bolt->hit_part_debris, &bolt->position);
    if (bolt->type_id == 0x1d) {
        AddGameDebris(world->debris_sys, 0x7f, &bolt->position);
        AddPartDebris(world->part_debris_sys, 6, &bolt->position);
        GameCam_NewShake(GameCam, 0.75f, 0.75f, 1.0f);
        NewRumbleAllPlayers(0.5f, 0.0f, 0, 0);
    }
    if (no_explosion == 0 && bolt->owner != NULL && static_cast<i8>(bolt->owner->apiobj.flags_low) < 0 &&
        Cheat_IsOn(0x12) && (world->area == NULL || !(world->area->flags & 1))) {
        AddGameDebris(WORLD->debris_sys, 0x49, &bolt->position);
        NewRumble(bolt->owner->pad_gamepad->pad, 0.3f, 0);
        PlaySfx("exp_thermalDet", points);
        GameCam_HitJudder();
        EXPLOSION *explosion = AddExplosion(&bolt->position, 0.5f * AreaPickupScale, 0.25f, NULL, -1, 0xa067);
        if (explosion != NULL && Arcade != 0 && bolt->owner != NULL &&
            (bolt->owner == Player[0] || bolt->owner == Player[1]) &&
            (bolt->owner->apiobj.field_0x1f8 & 0x1001) == 0x1001 &&
            static_cast<u8>(bolt->owner->apiobj.field_0x27c) <= 1) {
            explosion->field_0x24 |= 0x10000;
            explosion->object = bolt->owner;
        }
    }
}

i32 Bolt_HitPartMode(BOLT_s *bolt) {
    GameObject_s *owner = bolt->owner;
    if (owner == NULL)
        return 3;
    i8 player = owner->apiobj.field_0x27c;
    if (player == 0)
        return 4;
    return 3 + 2 * (player == 1);
}

i32 Bolt_HitPart_LSW(BOLT_s *, PART_s *part) {
    if (part->force_player_mask == 1) {
        if (part->scale_time < 2.0f)
            return 1;
    } else if (part->force_player_mask == 2) {
        if (part->scale_time < 1.0f)
            return 1;
    }
    return 0;
}

BOLTTYPE_s *BoltType_FindByID(i32 id, WORLDINFO_s *world) {
    if (id >= 0 && id < BoltSys->count) {
        return &BoltSys->types[id];
    }
    if (world != NULL && id >= BoltSys->count && id <= BoltSys->count + 7) {
        return &world->bolt_types[id - BoltSys->count];
    }
    return NULL;
}

void Bolt_HitGameObjectRC(NetMessage &);
i32 Bolt_HitGameObject(BOLT_s *bolt, GameObject_s *object, NUVEC *points, NUVEC *, NUVEC *, f32 radius, u8 *) {
    NUVEC center = object->apiobj.collision_position;
    i32 hit = (bolt->flags & 0x200) != 0 ? 1 : 2;
    for (;;) {
        NUVEC point = points[hit];
        if ((bolt->flags & 0x8000000) != 0 && bolt->time >= bolt->type->field_24) {
            center.y = 0.0f;
            point.y = 0.0f;
        }
        if (SphereSphereOverlapScaleY(&center, object->apiobj.field_0x1dc, object->apiobj.field_0x1e0, &point, radius,
                                      radius))
            break;
        if (hit == 0 || (hit == 1 && (bolt->flags & 0x200) != 0))
            return 0;
        --hit;
    }
    i16 bolt_index = static_cast<i16>(bolt - Bolt);
    i16 object_index = static_cast<i16>(object - Obj);
    NetMessage message = {1, NULL, 0x20, 0x20};
    for (i32 i = 0; i < 512; ++i) {
        if (NetMessage::sm_poolMessageData[i].references == 0) {
            message.data = &NetMessage::sm_poolMessageData[i];
            message.data->references = 1;
            break;
        }
    }
    if (message.data != NULL) {
        u8 *destination = message.data->bytes + message.write_offset;
        memcpy(destination, &bolt_index, 2);
        if (message.swap_endianness)
            EdFileSwapEndianess16(destination);
        message.write_offset += 2;
        destination = message.data->bytes + message.write_offset;
        memcpy(destination, &object_index, 2);
        if (message.swap_endianness)
            EdFileSwapEndianess16(destination);
        message.write_offset += 2;
        for (i32 i = 0; i < 3; ++i) {
            destination = message.data->bytes + message.write_offset;
            memmove(destination, &points[i], 12);
            if (message.swap_endianness) {
                EdFileSwapEndianess32(destination);
                EdFileSwapEndianess32(destination + 4);
                EdFileSwapEndianess32(destination + 8);
            }
            message.write_offset += 12;
        }
        destination = message.data->bytes + message.write_offset;
        memcpy(destination, &hit, 4);
        if (message.swap_endianness)
            EdFileSwapEndianess32(destination);
        message.write_offset += 4;
        destination = message.data->bytes + message.write_offset;
        memmove(destination, &netclient, 4);
        if (message.swap_endianness)
            EdFileSwapEndianess32(destination);
        message.write_offset += 4;
    }
    Bolt_HitGameObjectRC(message);
    if (message.data != NULL) {
        if (message.data->references > 1)
            --message.data->references;
        else
            message.data->references = 0;
    }
    return 1;
}

void KaminoE_CheckPlatHit(BOLT_s *);

i32 Bolt_HitPlatFn_LSW(BOLT_s *bolt) {
    if (WORLD->current_level == KAMINOE_LDATA)
        KaminoE_CheckPlatHit(bolt);
    return 0;
}

i32 Bolt_HitGameObjects(BOLT_s *bolt, NUVEC *points, NUVEC *minimum, NUVEC *maximum, f32 radius, u8 *hit_flags) {
    GameObject_s *object = Obj;
    i32 count = HIGHGAMEOBJECT;
    for (i32 i = 0; i < count; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0)
            continue;
        GameObject_s *owner = bolt->owner;
        if (object == owner || (object->field_0xe20 & 0x20) != 0)
            continue;
        if (owner != NULL && owner->field_0xcc0 != NULL && object == owner->field_0xcc0)
            continue;
        u32 flags = bolt->flags;
        if ((flags & 0x80) != 0 && object->apiobj.field_0x27c == -1 &&
            (object->field_0xcc0 == NULL || object->field_0xcc0->apiobj.field_0x27c == -1))
            continue;
        if ((CInfo[object->character_context].flags & 0x8080) != 0 || (object->movement_runtime_flags & 0x80) != 0)
            continue;
        if (VehicleArea != 0 && BonusArea == 0 && owner != NULL && owner->apiobj.field_0x27c != -1 &&
            object->apiobj.field_0x27c != -1)
            continue;
        if ((object->apiobj.character_data->game_character->flags_090 & 0x8000) != 0)
            continue;
        if (minimum->x > object->apiobj.collision_max.x || object->apiobj.collision_min.x > maximum->x ||
            minimum->z > object->apiobj.collision_max.z || object->apiobj.collision_min.z > maximum->z)
            continue;
        if ((flags & 0x8000000) == 0 &&
            (minimum->y > object->apiobj.collision_max.y || object->apiobj.collision_min.y > maximum->y))
            continue;
        if (Bolt_HitGameObject(bolt, object, points, minimum, maximum, radius, hit_flags) != 0)
            return 1;
        count = HIGHGAMEOBJECT;
    }
    return 0;
}

void Bolt_HitCustomFn_LSW(BOLT_s *, nuvec_s *) {
}

i32 LEGOCONTEXT_BLOCK = -1;
i16 LEGOACT_DEACTIVATED = -1;
i32 addbolt_nosfx;
BOLT_s *objhitobj_bolt;
extern i32 LEGOCONTEXT_HOLD, i_temp_xrot;
extern f32 DEACTIVATEDTIME;
i32 Player_HasDeflectBolts(GameObject_s *);
i32 CannotKill(GameObject_s *);
i32 NewBlockAction(GameObject_s *);
void FindAnglesXY(NUVEC *, u16 *, u16 *);
void CalculateInterceptVector(NUVEC *, NUVEC *, NUVEC *, f32, NUVEC *, NUVEC *);
void ObjHitShield(GameObject_s *, GameObject_s *, i32, BOLT_s *);
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
void NewBuzz(nupad_s *, f32, i32);
void GameCam_HitJudder();
void Bolt_HitGameObjectRC(NetMessage &message) {
    u16 bolt_index, object_index;
    NUVEC points[3];
    i32 hit, client;
    if (message.data != NULL) {
        memmove(&bolt_index, message.data->bytes + message.read_offset, 2);
        if (message.swap_endianness)
            EdFileSwapEndianess16(&bolt_index);
        message.read_offset += 2;
    }
    if (message.data != NULL) {
        memmove(&object_index, message.data->bytes + message.read_offset, 2);
        if (message.swap_endianness)
            EdFileSwapEndianess16(&object_index);
        message.read_offset += 2;
    }
    for (i32 i = 0; i < 3; ++i) {
        if (message.data != NULL) {
            memmove(&points[i], message.data->bytes + message.read_offset, 12);
            if (message.swap_endianness) {
                EdFileSwapEndianess32(&points[i].x);
                EdFileSwapEndianess32(&points[i].y);
                EdFileSwapEndianess32(&points[i].z);
            }
            message.read_offset += 12;
        }
    }
    if (message.data != NULL) {
        memmove(&hit, message.data->bytes + message.read_offset, 4);
        if (message.swap_endianness)
            EdFileSwapEndianess32(&hit);
        message.read_offset += 4;
    }
    if (message.data != NULL) {
        memmove(&client, message.data->bytes + message.read_offset, 4);
        if (message.swap_endianness)
            EdFileSwapEndianess32(&client);
        message.read_offset += 4;
    }
    GameObject_s *object = &Obj[object_index];
    BOLT_s *bolt = &Bolt[bolt_index];
    BOLTTYPE_s *type = bolt->type;
    if (BoltSys->stop_targeting != NULL && bolt->owner != NULL && (bolt->owner->apiobj.flags_low & 0x80) != 0)
        BoltSys->stop_targeting(bolt->owner, &bolt->position);
    i32 deflect = 0;
    if ((object->apiobj.flags_low & 0x80) != 0)
        deflect = Player_HasDeflectBolts(object) != 0;
    i32 cheat_deflect = 0;
    if (Cheats_CheckFlags(0x100000) != 0)
        cheat_deflect = (object->apiobj.flags_low & 0x80) != 0;
    if (!deflect)
        deflect = TouchHacks::ShouldDeflectBolt(*object, *bolt);
    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
    if ((bolt->flags & 0x100) == 0 || deflect) {
        u32 context_flags = CInfo[object->character_context].flags;
        if ((context_flags & 0x4000000) != 0 || ((context_flags & 0x8000000) != 0 && (object->jump_flags & 2) != 0) ||
            deflect || (context_flags & 0x800000) != 0 ||
            ((context_flags & 0x1000000) != 0 && (object->jump_flags & 1) != 0) || CannotKill(object)) {
            BoltSys->debris(bolt, points, hit, &object->apiobj.velocity, 1);
            if (LEGOCONTEXT_HOLD != -1 && object->character_context == LEGOCONTEXT_HOLD)
                NewBlockAction(object);
            GameAudio_PlaySfx(42, &points[hit], 0, 0);
            addbolt_nosfx = 1;
            if ((LEGOCONTEXT_BLOCK == -1 || object->character_context != LEGOCONTEXT_BLOCK) && !deflect &&
                !(bolt->time >= 0.1f))
                goto finish;
            NUVEC direction;
            if (bolt->owner != NULL && (bolt->owner->apiobj.flags_low & 1) != 0)
                CalculateInterceptVector(&points[hit], &bolt->owner->apiobj.collision_position,
                                         &bolt->owner->apiobj.velocity, bolt->speed, &direction, NULL);
            else
                NuVecSub(&direction, &bolt->previous_position, &points[hit]);
            FindAnglesXY(&direction, NULL, NULL);
            if (((bolt->flags & 0x40) == 0 || deflect) && (object->apiobj.flags_low & 0x80) != 0 &&
                ((LEGOCONTEXT_BLOCK != -1 && object->character_context == LEGOCONTEXT_BLOCK) || cheat_deflect) &&
                (object->apiobj.character_data->model_flags & 8) != 0) {
                if (bolt->owner == NULL || (bolt->owner->apiobj.flags_low & 1) == 0 ||
                    ((bolt->owner->apiobj.field_0x1f8 & 0x1001) == 0x1001 && bolt->owner->apiobj.field_0x287 == 0))
                    goto reflected;
            }
            if (!TouchHacks::TouchControlsActive) {
                temp_yrot += static_cast<i32>((qrand() * (1.0f / 65535.0f)) * 16384.0f - 8192.0f);
                i_temp_xrot += static_cast<i32>((qrand() * (1.0f / 65535.0f)) * 16384.0f - 5461.0f);
                if (i_temp_xrot < -8192)
                    i_temp_xrot = -8192;
                else if (i_temp_xrot > 8192)
                    i_temp_xrot = 8192;
                temp_xrot = i_temp_xrot;
            }
        reflected:
            NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
            NUMTX matrix;
            NUANGVEC angles = {static_cast<u16>(temp_xrot), static_cast<u16>(temp_yrot), 0};
            NuMtxSetRotationXYVU0(&matrix, &angles);
            BOLT_s *reflected_bolt = Bolt_Add(object, &points[hit], &matrix, bolt->type_id, 0);
            if (reflected_bolt != NULL)
                reflected_bolt->flags |= 0x10000000;
            goto finish;
        }
    }
    {
        auto *data = object->apiobj.character_data;
        auto *character = data->game_character;
        bool shield =
            object->field_0xd24 == 1.0f || ((character->flags_094[3] & 4) != 0 && VehicleArea != 0 && BonusArea != 0) ||
            (object->current_hp == 0 && (data->model_flags & 0x2000) != 0) ||
            ((object->apiobj.flags_low & 0x80) != 0 && (character->flags_090 & 0x40) != 0 &&
             (bolt->owner == NULL || (bolt->owner->apiobj.character_data->game_character->flags_090 & 0x40) == 0)) ||
            ((data->model_flags & 0x20000000) != 0 && object->field_0xcc0 == NULL);
        if (shield) {
            if ((bolt->flags & 0x4000000) != 0) {
                BoltSys->debris(bolt, points, hit, &object->apiobj.velocity, 0);
                ObjHitShield(bolt->owner, object, type->field_3c, bolt);
            } else {
                BoltSys->debris(bolt, points, hit, &object->apiobj.velocity, 1);
                ObjHitShield(bolt->owner, object, 0, bolt);
                GameAudio_PlaySfx(41, &bolt->position, 0, 0);
                NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                if ((bolt->flags & 0x100000) == 0) {
                    temp_yrot = NuAtan2D(bolt->position.x - object->apiobj.collision_position.x,
                                         bolt->position.z - object->apiobj.collision_position.z);
                    temp_yrot += static_cast<i32>((qrand() * (1.0f / 65535.0f)) * 16384.0f - 5461.0f);
                    temp_xrot = static_cast<i32>(-1820.0f - (qrand() * (1.0f / 65535.0f)) * 6371.0f);
                    NUMTX matrix;
                    NUANGVEC angles = {static_cast<u16>(temp_xrot), static_cast<u16>(temp_yrot), 0};
                    NuMtxSetRotationXYVU0(&matrix, &angles);
                    addbolt_nosfx = 1;
                    BOLT_s *reflected_bolt = Bolt_Add(object, &points[hit], &matrix, bolt->type_id, 0);
                    if (reflected_bolt != NULL)
                        reflected_bolt->flags |= 0x10000000;
                }
            }
        } else {
            BoltSys->debris(bolt, points, hit, &object->apiobj.velocity, 0);
            i32 damage = 0;
            if ((bolt->flags & 0x800000) == 0) {
                damage = BoltType_FindByID(bolt->type_id, WORLD)->field_3c;
                if (Player_HasDoubleBoltDamage_FromBolt(bolt))
                    damage += damage;
            }
            Bolt_PlayHitSfx(bolt);
            GameObject_s *owner = bolt->owner;
            if (owner != NULL && (owner->apiobj.flags_low & 1) != 0 && (owner->apiobj.flags_low & 0x80) == 0 &&
                (owner->apiobj.field_0x1f4 & 1) != 0 &&
                ((owner->apiobj.field_0x1f4 ^ object->apiobj.field_0x1f4) & 1) == 0)
                goto owner_feedback;
            objhitobj_bolt = bolt;
            ObjHitObj(owner, object, damage, bolt->hit_flags, 0, 0);
            if ((bolt->flags & 0x800000) != 0 && (CInfo[object->character_context].flags & 0x10008000) == 0 &&
                (object->apiobj.character_data->model_flags & 0x10) != 0 && LEGOACT_DEACTIVATED != -1 &&
                object->apiobj.character_model->model_data_b[LEGOACT_DEACTIVATED] != NULL) {
                DeactivatePlayer(object, DEACTIVATEDTIME, NULL);
                if (bolt->owner != NULL)
                    NewBuzz(bolt->owner->pad_gamepad->pad, 0.1f, 0);
                goto finish;
            }
        owner_feedback:
            if (bolt->owner != NULL && bolt->owner->pad_gamepad != NULL)
                NewRumble(bolt->owner->pad_gamepad->pad, 0.7f, 0);
        }
    }
finish:
    if (object->apiobj.field_0x287 == 0 && (object->apiobj.flags_low & 2) == 0) {
        f32 amount = object->apiobj.character_data->game_character->field_0x18;
        if (object->apiobj.scaled_radius > 1.0f)
            amount /= object->apiobj.scaled_radius;
        object->apiobj.velocity.x += bolt->field_0xac.x * amount;
        object->apiobj.velocity.z += bolt->field_0xac.z * amount;
    }
    if ((object->apiobj.flags_low & 0x80) != 0)
        GameCam_HitJudder();
    Bolt_End(bolt, 1);
}

void Bolt_AddDeflectedBolt(BOLT_s *, nuvec_s *, nuvec_s *, unsigned char *) {
}

extern "C" TERRAIN_SURFACE_s TerSurface[32];
i32 GizmoBlowupBlowup(GIZMOBLOWUP_s *, i32, i32, i32, GameObject_s *, i32);
static __used__ i32 Bolt_HitPlat(BOLT_s *bolt, u8 *hit_flags, WORLDINFO_s *) {
    u32 exclude = GetLevelExBlowupFlags();
    Bolt_PlayHitSfx(bolt);
    if ((bolt->hit_flags & 0x800) != 0 && GizmoSys_BoltHitPlat(WORLD->gizmo_sys, WORLD, bolt, hit_flags))
        return 1;
    GIZMOBLOWUP_s *blowup = GizmoBlowUp_FindFromPlatID(WORLD, bolt->hit_platform);
    if (blowup != NULL && (blowup->draw_flags & 0x8000) != 0) {
        GameObject_s *owner = bolt->owner;
        if (owner != NULL && (blowup->draw_flags & 0x80000) != 0) {
            if ((exclude & 1) != 0 && owner->apiobj.character_data->game_character->field_0x28 > 0.0f)
                return 0;
            if (owner->field_0xcc0 == NULL)
                return 0;
        }
        if (GizmoBlowupBlowup(blowup, 1, 3, bolt->type->field_3c, NULL, 1) != 0 && bolt->owner != NULL)
            NewRumble(bolt->owner->pad_gamepad->pad, 0.4f, 0);
        return 1;
    }
    if (bolt->field_0x104 != -1 && ((TerSurface[bolt->field_0x104].flags & 0x1800) != 0 || blowup != NULL)) {
        GameAudio_PlaySfx(41, &bolt->position, 0, 0);
        Bolt_AddDeflectedBolt(bolt, &bolt->field_0xac, &bolt->hit_normal, hit_flags);
        if (bolt->owner != NULL && (bolt->owner->apiobj.flags_low & 0x80) != 0) {
            NewRumble(bolt->owner->pad_gamepad->pad, 0.4f, 0);
            GameCam_HitJudder();
        }
    }
    return 0;
}

extern i16 id_XWING, id_MINIXWING, id_MINITIEINTERCEPTOR, id_MINIATAT, id_MINIROYALSTARSHIP, id_MINIIMPERIALSHUTTLE;
extern i16 id_MILLENNIUMFALCON, id_MINIMILLENNIUMFALCON, id_ATST, id_JEDISTARFIGHTERREDEP3, id_JEDISTARFIGHTERYELLOWEP3;
i32 Bolt_AlternateFire_LSW(GameObject_s *object, i32 index) {
    i16 id = object->id;
    if (id == id_XWING || id == id_MINIXWING || id == id_MINITIEINTERCEPTOR || id == id_MINIATAT ||
        id == id_MINIROYALSTARSHIP || id == id_MINIIMPERIALSHUTTLE) {
        if (object->bolt_fire_phase == 0) {
            if (index == 0 || index == 2)
                return 0;
        } else if (object->bolt_fire_phase == 1) {
            if (index == 1 || index == 3)
                return 0;
        }
    } else if (id == id_MILLENNIUMFALCON || id == id_MINIMILLENNIUMFALCON || id == id_ATST ||
               id == id_JEDISTARFIGHTERREDEP3 || id == id_JEDISTARFIGHTERYELLOWEP3) {
        if (object->bolt_fire_phase != index)
            return 0;
    }
    return 1;
}

f32 Bolt_ObjTargetPosYAdjust(GameObject_s *object) {
    f32 height = object->apiobj.collision_height;
    i32 random = qrand();
    return static_cast<f32>(random) * ((height + height) / 65535.0f) - height;
}

extern "C" i16 id_4LOM;
extern "C" i16 id_ANAKINSSPEEDER;
extern "C" i16 id_ANAKINSSPEEDER_GREEN;
extern "C" i16 id_ATAT;
extern "C" i16 id_ATST_LOWRES;
extern "C" i16 id_BIGGUN;
extern "C" i16 id_BOBAFETT;
extern "C" i16 id_CATAPULT;
extern "C" i16 id_CLONEWALKER;
extern "C" i16 id_EWOK;
extern "C" i16 id_FLASHSPEEDER;
extern "C" i16 id_KAMINOANDROID;
extern "C" i16 id_MINIDROIDEKA;
extern "C" i16 id_MINISITHINFILTRATOR;
extern "C" i16 id_MINISOLARSAILOR;
extern "C" i16 id_MINISTARDESTROYER;
extern "C" i16 id_MINITIEADVANCED;
extern "C" i16 id_MINITIEBOMBER;
extern "C" i16 id_MINITIEFIGHTER;
extern "C" i16 id_NABOOSTARFIGHTERLIME;
extern "C" i16 id_NEW_REPUBLIC_GUNSHIP_GREEN;
extern "C" i16 id_PROBEDROID;
extern "C" i16 id_REPUBLICGUNSHIP;
extern "C" i16 id_REPUBLICGUNSHIP_GREEN;
extern "C" i16 id_SENTRYDROID;
extern "C" i16 id_SLAVE1;
extern "C" i16 id_SPEEDERBIKESNOW;
extern "C" i16 id_STAP2;
extern "C" i16 id_TIEBOMBER;
extern "C" i16 id_TIEFIGHTER;
extern "C" i16 id_TIEFIGHTERDARTH;
extern "C" i16 id_TIEINTERCEPTOR;
extern "C" i16 id_WICKET;
extern "C" i16 id_ZAMSSPEEDER;
void Move_CANNON(GameObject_s *);
extern AREADATA *DOGFIGHT_ADATA;
i32 BoltType_FindIDByCreature(GameObject_s *object, i32 fallback) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (object == NULL)
        return fallback;
    i32 id = object->id;
    if (id == id_MILLENNIUMFALCON) {
        if (world->area != VEHICLES_ADATA && object->bolt_fire_phase != 0)
            return 9;
        return 7 + (world->current_level == ASTEROIDCHASED_LDATA);
    }
    if (id == id_NABOOSTARFIGHTER || id == id_NABOOSTARFIGHTERLIME)
        return 34;
    if (id == id_ANAKINSSPEEDER || id == id_ANAKINSSPEEDER_GREEN || id == id_ZAMSSPEEDER)
        return 11;
    if (id == id_FLASHSPEEDER)
        return 20;
    if (id == id_XWING || id == id_SLAVE1)
        return 6;
    if (id == id_SPEEDERBIKE || id == id_SPEEDERBIKESNOW)
        return 10;
    if (id == id_PROBEDROID || id == id_SNOWSPEEDER)
        return 11;
    if (id == id_TIEFIGHTER || id == id_TIEINTERCEPTOR || id == id_TIEFIGHTERDARTH || id == id_TIEBOMBER ||
        id == id_IMPERIALSHUTTLE) {
        if (world == NULL)
            return 12;
        if (world->current_level == DEATHSTARBATTLED_LDATA)
            return 13;
        return 12 + 2 * (world->current_level == DOGFIGHTA_LDATA);
    }
    if (id == id_MINITIEFIGHTER || id == id_MINITIEINTERCEPTOR || id == id_MINITIEADVANCED || id == id_MINITIEBOMBER ||
        id == id_MINIIMPERIALSHUTTLE || id == id_MINISITHINFILTRATOR || id == id_MINISOLARSAILOR || id == id_MINIATAT ||
        id == id_MINISTARDESTROYER)
        return 23;
    if (id == id_MINIMILLENNIUMFALCON)
        return object->bolt_fire_phase == 0 ? 21 : 22;
    if (id == id_JEDISTARFIGHTERYELLOWEP3 || id == id_JEDISTARFIGHTERREDEP3) {
        if (WORLD->area != NULL && WORLD->area == DOGFIGHT_ADATA)
            return WORLD->lev_objs[0x12d].active == 0 ? 33 : 32;
        return 33;
    }
    if (id == id_CATAPULT)
        return 29;
    if (object->apiobj.character_data->move_fn == Move_CANNON)
        return 20;
    if (GCDataList[id].uses_weapon_action == 8 || GCDataList[id].weapon_model == 22)
        return 2;
    if (id == id_BIGGUN)
        return 20;
    if (id == id_BOBAFETT || id == id_4LOM)
        return 26;
    if (id == id_TRAININGREMOTE)
        return 3;
    if (id == id_ATAT)
        return 24;
    if (id == id_ATST || id == id_CLONEWALKER)
        return 4;
    if (id == id_ATST_LOWRES)
        return 25;
    if (id == id_DROIDEKA || id == id_MINIDROIDEKA)
        return 17;
    if (id == id_STAP || id == id_STAP2)
        return 0;
    if (id == id_EWOK)
        return Cheat_IsOn(0x29) != 0 ? 30 : 27;
    if (id == id_WICKET)
        return Cheat_IsOn(0x29) != 0 ? 31 : 28;
    if (id == id_REPUBLICGUNSHIP || id == id_REPUBLICGUNSHIP_GREEN) {
        if (WORLD->current_level == BONUS_GUNSHIPA_LDATA)
            return 35;
        if (WORLD->current_level == BONUS_GUNSHIPB_LDATA)
            return 36;
        return fallback;
    }
    if (id == id_NEW_REPUBLIC_GUNSHIP || id == id_NEW_REPUBLIC_GUNSHIP_GREEN) {
        if (WORLD->current_level == GUNSHIPA_LDATA)
            return 37;
        if (WORLD->current_level == GUNSHIPB_LDATA)
            return 38;
        return fallback;
    }
    if (id == id_SENTRYDROID || id == id_KAMINOANDROID)
        return 0;
    return fallback;
}

void Bolt_Init(void *, NetMessage &);
BOLT_s *Bolt_Add(GameObject_s *object, nuvec_s *position, numtx_s *matrix, i32 type_id, i32 flags) {
    BOLTTYPE_s *type = BoltType_FindByID(type_id, WORLD);
    if (type == NULL || NuSpecialExistsFn(type->pad_68) == 0)
        return NULL;
    BOLT_s *bolt = Bolt_Alloc();
    if (bolt == NULL)
        return NULL;
    i16 owner = object != NULL ? static_cast<i16>(object - Obj) : -1;
    NetMessage message = {1, NULL, 0x20, 0x20};
    for (i32 i = 0; i < 512; ++i) {
        if (NetMessage::sm_poolMessageData[i].references == 0) {
            message.data = &NetMessage::sm_poolMessageData[i];
            message.data->references = 1;
            break;
        }
    }
    if (message.data != NULL) {
        memcpy(message.data->bytes + message.write_offset, &owner, sizeof(owner));
        EdFileSwapEndianess16(message.data->bytes + message.write_offset);
        message.write_offset += 2;
        memmove(message.data->bytes + message.write_offset, position, sizeof(*position));
        for (i32 i = 0; i < 3; ++i)
            EdFileSwapEndianess32(message.data->bytes + message.write_offset + i * 4);
        message.write_offset += 12;
        memmove(message.data->bytes + message.write_offset, matrix, sizeof(*matrix));
        for (i32 i = 0; i < 16; ++i)
            EdFileSwapEndianess32(message.data->bytes + message.write_offset + i * 4);
        message.write_offset += 64;
        memcpy(message.data->bytes + message.write_offset, &type_id, sizeof(type_id));
        EdFileSwapEndianess32(message.data->bytes + message.write_offset);
        message.write_offset += 4;
        memcpy(message.data->bytes + message.write_offset, &flags, sizeof(flags));
        EdFileSwapEndianess32(message.data->bytes + message.write_offset);
        message.write_offset += 4;
    }
    Bolt_Init(bolt, message);
    if (message.data != NULL) {
        if (message.data->references > 1)
            --message.data->references;
        else
            message.data->references = 0;
    }
    return bolt;
}

void Bolt_End(BOLT_s *bolt, i32 run_callback) {
    bolt->active = 0;
    if (run_callback && bolt->type != NULL && bolt->type->end_callback != NULL)
        bolt->type->end_callback(bolt);
}

BOLT_s *Bolt_Find(i32 type_id, NUVEC *position, GameObject_s *owner) {
    BOLT_s *nearest = NULL;
    f32 nearest_distance = 0.49f;
    for (i32 index = 0; index < 32; ++index) {
        BOLT_s *bolt = &Bolt[index];
        if (bolt->active == 0)
            continue;
        if (type_id != -1 && bolt->type_id != type_id)
            continue;
        if (owner != NULL && bolt->owner != owner)
            continue;
        if (position == NULL)
            return bolt;

        const f32 distance = NuVecDistSqr(&bolt->position, position, NULL);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest = bolt;
        }
    }
    return nearest;
}

void Bolt_Free(BOLT_s *bolt) {
    bolt->active = 0;
}

static bool Bolt_RayCast(BOLT_s *, NUVEC *, NUVEC *, f32);
i32 GizmoSys_BoltHit(GIZMOSYS_s *, void *, BOLT_s *, NUVEC *, NUVEC *, NUVEC *, f32, u8 *);
GIZMOBLOWUP_s *GizmoBlowUp_Hit(GameObject_s *, NUVEC *, i32, f32, NUVEC *, NUVEC *, BOLT_s *, u32, u8 *);
i32 ObjHitObj_Flags(GameObject_s *);
extern "C" f32 NewRayCastGetTOFI();
i32 addbolt_noobjmom, addbolt_newsfx;
NUVEC addbolt_newpos;
i32 (*BoltInitSfxFn)(GameObject_s *);
void Bolt_Init(void *storage, NetMessage &message) {
    BOLT_s *bolt = static_cast<BOLT_s *>(storage);
    i16 owner_index;
    NUVEC position;
    NUMTX orientation;
    i32 type_id, hit_flags;
    if (message.data != NULL) {
        memmove(&owner_index, message.data->bytes + message.read_offset, 2);
        if (message.swap_endianness)
            EdFileSwapEndianess16(&owner_index);
        message.read_offset += 2;
    }
    if (message.data != NULL) {
        memmove(&position, message.data->bytes + message.read_offset, 12);
        if (message.swap_endianness) {
            EdFileSwapEndianess32(&position.x);
            EdFileSwapEndianess32(&position.y);
            EdFileSwapEndianess32(&position.z);
        }
        message.read_offset += 12;
    }
    if (message.data != NULL) {
        memmove(&orientation, message.data->bytes + message.read_offset, 64);
        if (message.swap_endianness)
            for (i32 i = 0; i < 16; ++i)
                EdFileSwapEndianess32(reinterpret_cast<u8 *>(&orientation) + i * 4);
        message.read_offset += 64;
    }
    if (message.data != NULL) {
        memmove(&type_id, message.data->bytes + message.read_offset, 4);
        if (message.swap_endianness)
            EdFileSwapEndianess32(&type_id);
        message.read_offset += 4;
    }
    if (message.data != NULL) {
        memmove(&hit_flags, message.data->bytes + message.read_offset, 4);
        if (message.swap_endianness)
            EdFileSwapEndianess32(&hit_flags);
        message.read_offset += 4;
    }
    GameObject_s *owner = owner_index < 0 ? NULL : &Obj[owner_index];
    BOLTTYPE_s *type = BoltType_FindByID(type_id, WORLD);
    NUVEC momentum = nuvec_zero;
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    i32 no_sfx = addbolt_nosfx;
    addbolt_nosfx = 0;
    i32 no_momentum = addbolt_noobjmom;
    addbolt_noobjmom = 0;
    addbolt_newsfx = -1;
    if (type == NULL || !NuSpecialExistsFn(type->pad_68))
        return;
    if (bolt->active != 0) {
        NUVEC old_points[3];
        old_points[1] = bolt->position;
        if ((bolt->flags & 0x200) == 0) {
            NUVEC offset = {0.0f, 0.0f, bolt->collision_radius + bolt->collision_radius};
            NuVecMtxRotate(&offset, &offset, &bolt->orientation);
            NuVecSub(&old_points[0], &bolt->position, &offset);
            NuVecAdd(&old_points[2], &bolt->position, &offset);
        }
        BoltSys->debris(bolt, old_points, -1, NULL, 0);
    }
    bolt->type = type;
    bolt->type_id = type_id;
    if (BOLT_OVERRIDE_PLAYERBOLTSPEED != 0.0f && BOLT_OVERRIDE_PLAYERBOLTDURATION != 0.0f) {
        bolt->speed = BOLT_OVERRIDE_PLAYERBOLTSPEED;
        bolt->lifetime = BOLT_OVERRIDE_PLAYERBOLTDURATION;
    } else {
        bolt->speed = type->field_10;
        bolt->lifetime = type->field_14;
    }
    bolt->acceleration_y = type->field_18;
    bolt->radius = type->field_1c;
    bolt->scale = type->field_20;
    bolt->hit_debris = static_cast<i16>(static_cast<u32>(type->field_2c) >> 16);
    bolt->hit_part_debris = type->field_38;
    bolt->index = bolt - Bolt;
    if (hit_flags == 0)
        hit_flags = ObjHitObj_Flags(owner);
    bolt->hit_flags = hit_flags | 0x80;
    u32 flags = type->field_60;
    if (owner != NULL) {
        if ((owner->apiobj.flags_low & 0x80) != 0) {
            if (owner->apiobj.field_0x27c == 0)
                flags |= 0x29;
            else
                flags |= owner->apiobj.field_0x27c == 1 ? 0x2a : 0x28;
        }
        bool add_speed = InitBolt_AddMomentumType != NULL && InitBolt_AddMomentumType(bolt, owner, &momentum) != 0;
        if (!add_speed) {
            u32 model_flags = owner->apiobj.character_data->model_flags;
            add_speed = (model_flags & 0x4000000) != 0 ||
                        ((owner->apiobj.flags_low & 0x80) != 0 && (model_flags & 0x2000) != 0);
        }
        if (add_speed && !no_momentum) {
            f32 speed = owner->facing_direction.x * owner->apiobj.velocity.x +
                        owner->facing_direction.z * owner->apiobj.velocity.z;
            if (speed < 0.0f)
                speed = 0.0f;
            bolt->speed += speed;
        }
        if (owner->field_0xcc0 != NULL)
            flags |= 0x40000;
        if (BoltInitSfxFn != NULL)
            addbolt_newsfx = BoltInitSfxFn(owner);
        if (addbolt_newsfx == -1)
            addbolt_newsfx = owner->apiobj.character_data->game_character->sfx_shoot;
    }
    if (addbolt_newsfx == -1)
        addbolt_newsfx = type->shoot_sfx_id;
    if (!no_sfx && addbolt_newsfx != -1)
        GameAudio_PlaySfxById(addbolt_newsfx, &position, 0, 0);
    addbolt_newpos = position;
    bolt->time = 0.0f;
    bolt->active = 1;
    bolt->position = position;
    bolt->previous_position = position;
    bolt->field_0x102 = 0;
    bolt->hit_platform = -1;
    bolt->owner = owner;
    bolt->flags = flags;
    bolt->field_0xe4 = 2000000.0f;
    bolt->field_0xe8 = 2000000.0f;
    bolt->field_0x105 = -1;
    if (owner != NULL) {
        if (owner->apiobj.field_0x218 != 2000000.0f) {
            bolt->field_0xe4 = owner->apiobj.field_0x218;
            bolt->surface_x_rotation = owner->field_0x105e;
            bolt->surface_z_rotation = owner->field_0x1060;
        }
        if (owner->field_0x1020 != 2000000.0f && owner->field_0x1087 == 2)
            bolt->field_0xe8 = owner->field_0x1020;
    }
    f32 collision_radius = bolt->radius;
    if ((flags & 0x200) == 0)
        collision_radius *= 1.0f / 3.0f;
    bolt->radius *= bolt->scale;
    bolt->collision_radius = collision_radius * bolt->scale;
    bolt->velocity.x = 0.0f;
    bolt->velocity.y = 0.0f;
    bolt->velocity.z = bolt->speed;
    NuMtxSetRotationX(&bolt->effect_orientation, 0x4000);
    bolt->orientation = orientation;
    NuVecMtxRotate(&bolt->velocity, &bolt->velocity, &bolt->orientation);
    NuVecAdd(&bolt->velocity, &bolt->velocity, &momentum);
    NuVecNorm(&bolt->field_0xac, &bolt->velocity);
    if ((bolt->flags & 0x1000) != 0) {
        u16 x, y;
        FindAnglesXY(&bolt->field_0xac, &x, &y);
        NuMtxSetRotationX(&bolt->effect_orientation, x + 0x4000);
        NuMtxRotateY(&bolt->effect_orientation, y);
        NuMtxSetRotationX(&bolt->orientation, x);
        NuMtxRotateY(&bolt->orientation, y);
    } else
        NuMtxMulR(&bolt->effect_orientation, &bolt->effect_orientation, &bolt->orientation);
    NUVEC points[3];
    points[1] = bolt->position;
    if ((bolt->flags & 0x200) == 0) {
        NUVEC offset = {0.0f, 0.0f, bolt->collision_radius + bolt->collision_radius};
        NuVecMtxRotate(&offset, &offset, &bolt->orientation);
        NuVecSub(&points[0], &bolt->position, &offset);
        NuVecAdd(&points[2], &bolt->position, &offset);
    }
    bolt->bounds_min = {bolt->position.x - bolt->radius, bolt->position.y - bolt->radius,
                        bolt->position.z - bolt->radius};
    bolt->bounds_max = {bolt->position.x + bolt->radius, bolt->position.y + bolt->radius,
                        bolt->position.z + bolt->radius};
    f32 initial_speed = bolt->speed, initial_lifetime = bolt->lifetime;
    NUVEC movement;
    NuVecScale(&movement, &bolt->field_0xac, 0.2f * initial_speed);
    if (bolt->acceleration_y != 0.0f)
        movement.y = 0.2f * bolt->velocity.y + (bolt->acceleration_y * 0.5f) * 0.04000000283122063f;
    f32 back_distance = 0.01f;
    if (owner != NULL) {
        back_distance = owner->apiobj.collision_radius;
        if ((owner->apiobj.character_data->model_flags & 0x2000) == 0 &&
            (owner->apiobj.character_data->game_character->flags_090 & 0x80) == 0)
            back_distance *= 3.0f;
    }
    NUVEC back, start;
    NuVecScale(&back, &bolt->field_0xac, back_distance);
    NuVecSub(&start, &bolt->position, &back);
    NuVecAdd(&movement, &movement, &back);
    bolt->ray_radius = bolt->collision_radius;
    bolt->ray_time = 0.2f;
    if ((bolt->flags & 4) == 0 && Bolt_RayCast(bolt, &start, &movement, bolt->ray_radius)) {
        f32 offset_time = (back_distance / (initial_speed * initial_lifetime)) * bolt->lifetime;
        bolt->lifetime = bolt->ray_time + offset_time;
        bolt->lifetime = NewRayCastGetTOFI() * bolt->lifetime - offset_time;
        if (bolt->lifetime <= 0.0f) {
            NuVecAdd(&start, &start, &movement);
            AddGameDebris(world->debris_sys, bolt->hit_debris, &start);
            AddGameDebris(world->debris_sys, bolt->hit_debris, &bolt->position);
            Bolt_End(bolt, 1);
            if ((bolt->flags & 0x10000) == 0 && !Bolt_HitGameObjects(bolt, points, &bolt->bounds_min, &bolt->bounds_max,
                                                                     bolt->collision_radius, NULL)) {
                bool hit = false;
                if ((bolt->flags & 0x13) != 0 ||
                    (bolt->owner != NULL && ((bolt->owner->field_0xefb & 0x10) != 0 || bolt->owner->use_action == 5))) {
                    hit = bolt->hit_platform != -1 && Bolt_HitPlat(bolt, NULL, world);
                    if (!hit)
                        hit = GizmoSys_BoltHit(world->gizmo_sys, world, bolt, points, &bolt->bounds_min,
                                               &bolt->bounds_max, bolt->collision_radius, NULL) != 0;
                    if (!hit) {
                        bool single = (bolt->flags & 0x200) != 0;
                        if (GizmoBlowUp_Hit(bolt->owner, single ? &points[1] : points, single ? 1 : 3,
                                            bolt->collision_radius, &bolt->bounds_min, &bolt->bounds_max, bolt, 1,
                                            NULL)) {
                            BoltSys->debris(bolt, points, -1, NULL, 0);
                            if (bolt->owner != NULL)
                                NewRumble(bolt->owner->pad_gamepad->pad, 0.6f, 0);
                            Bolt_End(bolt, 1);
                            Bolt_PlayHitSfx(bolt);
                            hit = true;
                        } else
                            hit = Bolt_HitParts(bolt, points, &bolt->bounds_min, &bolt->bounds_max,
                                                bolt->collision_radius, Bolt_HitPartMode(bolt)) != NULL;
                    }
                }
                if (!hit)
                    Bolt_PlayHitSfx(bolt);
                if (BoltSys->stop_targeting != NULL && bolt->owner != NULL &&
                    (bolt->owner->apiobj.flags_low & 0x80) != 0)
                    BoltSys->stop_targeting(bolt->owner, &bolt->position);
            }
        }
    }
    if (type->init_callback != NULL)
        type->init_callback(bolt);
}

extern "C" void AddVariableShotDebrisEffectTimed1(i32, NUVEC *, i32, f32, i16, i16, NUMTX *);
static __used__ void UpdateBolt_Geonosian(BOLT_s *bolt) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    i32 effect = world->debris_sys->entries[85].effect;
    if (effect != -1)
        AddVariableShotDebrisEffectTimed1(effect, &bolt->position, 60, FRAMETIME, 0, 0, NULL);
}

i32 GameRayCast(NUVEC *, NUVEC *, f32, i32);
extern "C" {
    void PlatOnOff(i32, i32);
    i32 TerrainPlatId();
    i32 NewRayCastGetImpactTerrainType();
    i32 IgnoreWallSplines;
}
static __used__ bool Bolt_RayCast(BOLT_s *bolt, NUVEC *start, NUVEC *movement, f32 radius) {
    NUVEC end;
    NuVecAdd(&end, start, movement);
    i32 owner_platform = -1;
    if (bolt->owner != NULL && bolt->owner->field_0x107c != -1) {
        owner_platform = bolt->owner->field_0x107c;
        PlatOnOff(owner_platform, 0);
    }
    if (LevBoltIgnorePlatIds[0] != -1)
        PlatOnOff(LevBoltIgnorePlatIds[0], 0);
    if (LevBoltIgnorePlatIds[1] != -1)
        PlatOnOff(LevBoltIgnorePlatIds[1], 0);
    bool hit = false;
    if (GameRayCast(start, movement, radius, 0) != 0) {
        bolt->hit_platform = TerrainPlatId();
        bolt->field_0x104 = NewRayCastGetImpactTerrainType();
        NuVecAdd(&bolt->dogfight_hit_position, start, movement);
        bolt->hit_normal = ShadNorm;
        hit = true;
    } else {
        bolt->hit_platform = -1;
        bolt->field_0x104 = -1;
    }
    if (owner_platform != -1)
        PlatOnOff(owner_platform, 1);
    if (LevBoltIgnorePlatIds[0] != -1)
        PlatOnOff(LevBoltIgnorePlatIds[0], 1);
    if (LevBoltIgnorePlatIds[1] != -1)
        PlatOnOff(LevBoltIgnorePlatIds[1], 1);
    bolt->ray_end = end;
    IgnoreWallSplines = 0;
    return hit;
}

void AddPartDebris(PARTDEBSYS_s *, i32, NUVEC *);
static __used__ void Bolt_Debris_Default(BOLT_s *bolt, nuvec_s *points, int point, nuvec_s *, int) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (((bolt->flags & 0x40000200) == 0 && point == -1) || point == 0)
        AddGameDebris(world->debris_sys, bolt->hit_debris, &points[0]);
    if (point == -1 || point == 1)
        AddGameDebris(world->debris_sys, bolt->hit_debris, &points[1]);
    if (((bolt->flags & 0x40000200) == 0 && point == -1) || point == 2)
        AddGameDebris(world->debris_sys, bolt->hit_debris, &points[2]);
    if (bolt->hit_part_debris != -1)
        AddPartDebris(world->part_debris_sys, bolt->hit_part_debris, &bolt->position);
}

static __used__ void Bolt_GetShootOrigin_Default(GameObject_s *object, nuvec_s *position) {
    *position = object->apiobj.collision_position;
}

static __used__ i32 Bolt_GetShootDirection_Default(GameObject_s *object, nuvec_s *direction) {
    u16 angle = object->apiobj.movement_facing_angle;
    if (direction != NULL) {
        direction->x = NuTrigTable[angle >> 1];
        direction->y = 0.0f;
        direction->z = NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff];
    }
    return angle;
}

static __used__ unsigned int Batarang_GetTargetPos(BATARANG_s *, int, nuvec_s *) {
    return {};
}

static __used__ void CollideBoltStarFighter(BOLT_s *, starfighter_s *, _vuv_s *, _vuv_s *) {
}

EXPLOSION *Detonate(NUVEC *, u16);
static __used__ void EndBolt_EwokTorpedo(BOLT_s *bolt) {
    Detonate(&bolt->position, 0);
}

static __used__ void ProcessSpaceLevel(spacelevel_s *) {
}

static __used__ void ProcessStarFighter(starfighter_s *, quickboltinfo *) {
}

static __used__ void StarFighterAlign(starfighter_s *, _vuv_s *, f32, i32) {
}

static __used__ void TrooperTeamSetStateCode(minitrooperteam_s *) {
}

static __used__ unsigned int BoltInitSfx_LSW(GameObject_s *) {
    return {};
}

void BoltTypes_Init(WORLDINFO_s *world) {
    (void)world;
}

void BoltTypes_Configure(WORLDINFO_s *world, char *config) {
    (void)world;
    (void)config;
}

extern "C" {

    void HitParts(void) {
    }

} // extern "C"

extern "C" {
    void NewTerrHitInfo(u8 *);
    void NewRayCastGetImpactNormal(NUVEC *);
    i32 NewShadowOnPlatform();
    i32 ShadowInfo();
    void AddVariableShotDebrisEffectTimed5(i32, NUVEC *, NUVEC *, NUVEC *, i32, f32, NUMTX *, NUMTX *, i16, u8);
}
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
f32 FindReflectionNoPlatforms(NUVEC *);
void FindAnglesZX(NUVEC *, u16 *, u16 *);

void Bolts_Update(WORLDINFO_s *world) {
    NUVEC normal = v010;
    u8 processed[32] = {};
    for (BOLT_s *bolt = Bolt; bolt != Bolt + 32; ++bolt) {
        if (!bolt->active || processed[bolt - Bolt])
            continue;
        if (bolt->owner != NULL && (bolt->owner->apiobj.field_0x1f8 & 0x1001) != 0x1001)
            bolt->owner = NULL;
        BOLTTYPE_s *type = bolt->type;
        if ((bolt->flags & 4) == 0) {
            bolt->ray_time -= FRAMETIME;
            if (bolt->ray_time <= 0.0f) {
                NUVEC movement, end;
                NuVecScale(&movement, &bolt->field_0xac, 0.2f * bolt->speed);
                bolt->ray_time = 0.2f;
                if (bolt->acceleration_y != 0.0f)
                    movement.y = 0.2f * bolt->velocity.y + (bolt->acceleration_y * 0.5f) * 0.04f;
                NuVecAdd(&end, &bolt->position, &movement);
                NuVecSub(&movement, &end, &bolt->ray_end);
                if (Bolt_RayCast(bolt, &bolt->ray_end, &movement, bolt->ray_radius)) {
                    u8 info[4];
                    NewRayCastGetImpactNormal(&normal);
                    NewTerrHitInfo(info);
                    f32 time = bolt->time;
                    f32 ray_time = bolt->ray_time;
                    bolt->lifetime = time + ray_time * NewRayCastGetTOFI();
                    if (bolt->type->ricochet_callback != NULL)
                        bolt->type->ricochet_callback(bolt, &normal);
                }
                bolt->field_0xe8 = 2000000.0f;
                f32 height = GameShadow(NULL, &bolt->position, 5.0f, -1);
                if (height != 2000000.0f) {
                    if (NuSpecialExistsFn(type->pad_68 + 0x30)) {
                        bolt->field_0xe4 = height;
                        FindAnglesZX(&ShadNorm, &bolt->surface_x_rotation, &bolt->surface_z_rotation);
                    }
                    u32 surface = ShadowInfo();
                    if (surface <= 31 && (TerSurface[surface].flags & 2) != 0)
                        bolt->field_0xe8 = height;
                    else if (static_cast<f32>(NewShadowOnPlatform()) != -1.0f)
                        bolt->field_0xe8 = FindReflectionNoPlatforms(&bolt->position);
                }
            }
        }
        bolt->time += FRAMETIME;
        if (type->update_callback != NULL)
            type->update_callback(bolt);
        if (bolt->acceleration_y != 0.0f) {
            bolt->velocity.y += FRAMETIME * bolt->acceleration_y;
            bolt->speed = NuVecMag(&bolt->velocity);
            NuVecNorm(&bolt->field_0xac, &bolt->velocity);
            if ((bolt->flags & 0x1000) != 0) {
                u16 x, y;
                FindAnglesXY(&bolt->field_0xac, &x, &y);
                NuMtxSetRotationX(&bolt->effect_orientation, x + 0x4000);
                NuMtxRotateY(&bolt->effect_orientation, y);
                NuMtxSetRotationX(&bolt->orientation, x);
                NuMtxRotateY(&bolt->orientation, y);
            }
        }
        bolt->position.x += bolt->velocity.x * FRAMETIME;
        bolt->position.y += bolt->velocity.y * FRAMETIME;
        bolt->position.z += bolt->velocity.z * FRAMETIME;
        NUVEC points[3];
        points[1] = bolt->position;
        if ((bolt->flags & 0x200) == 0) {
            NUVEC offset = {0.0f, 0.0f, bolt->collision_radius + bolt->collision_radius};
            NuVecMtxRotate(&offset, &offset, &bolt->orientation);
            NuVecSub(&points[0], &bolt->position, &offset);
            NuVecAdd(&points[2], &bolt->position, &offset);
        }
        bolt->bounds_min.x = bolt->position.x - bolt->radius;
        bolt->bounds_min.y = bolt->position.y - bolt->radius;
        bolt->bounds_min.z = bolt->position.z - bolt->radius;
        bolt->bounds_max.x = bolt->position.x + bolt->radius;
        bolt->bounds_max.y = bolt->position.y + bolt->radius;
        bolt->bounds_max.z = bolt->position.z + bolt->radius;
        bool expired = bolt->time >= bolt->lifetime;
        bool surface_hit = false;
        if (expired) {
            if (bolt->field_0x104 != -1) {
                NUVEC reflection;
                Bolt_Reflect(&bolt->hit_normal, &bolt->velocity, &reflection);
                NuVecScale(&reflection, &reflection, 0.2f);
                i32 platform_reflection = bolt->hit_platform != -1 && bolt->field_0x104 != -1 &&
                                          (TerSurface[bolt->field_0x104].flags & 0x1000) != 0;
                BoltSys->debris(bolt, points, -1, &reflection, platform_reflection);
                surface_hit = true;
            } else
                BoltSys->debris(bolt, points, -1, NULL, 0);
            Bolt_End(bolt, 1);
        } else {
            if (Bolt_HitCustomFn != NULL)
                Bolt_HitCustomFn(bolt, points);
            for (i32 i = 0; i < 2; ++i) {
                i16 effect = static_cast<i16>(static_cast<u32>(type->field_30) >> (i * 16));
                u16 count = static_cast<u16>(static_cast<u32>(type->field_34) >> (i * 16));
                if (effect != -1 && count != 0)
                    AddVariableShotDebrisEffectTimed5(world->debris_sys->entries[effect].effect, &bolt->position, NULL,
                                                      &bolt->velocity, count, FRAMETIME, NULL, NULL, 20000, 0);
            }
        }
        if ((bolt->flags & 0x10000) != 0)
            continue;
        u8 *hits = expired ? NULL : processed;
        bool character_hit = false;
        if (expired)
            character_hit = Bolt_HitGameObjects(bolt, points, &bolt->bounds_min, &bolt->bounds_max,
                                                bolt->collision_radius, NULL) != 0;
        else {
            GameObject_s *object = Obj;
            i32 count = HIGHGAMEOBJECT;
            u32 flags = bolt->flags;
            for (i32 i = 0; i < count; ++i, ++object) {
                if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0)
                    continue;
                GameObject_s *owner = bolt->owner;
                if (object == owner || (object->field_0xe20 & 0x20) != 0)
                    continue;
                if (owner != NULL && owner->field_0xcc0 != NULL && object == owner->field_0xcc0)
                    continue;
                if ((flags & 0x80) != 0 && object->apiobj.field_0x27c == -1 &&
                    (object->field_0xcc0 == NULL || object->field_0xcc0->apiobj.field_0x27c == -1))
                    continue;
                if ((CInfo[object->character_context].flags & 0x8080) != 0 ||
                    (object->movement_runtime_flags & 0x80) != 0)
                    continue;
                if (VehicleArea != 0 && BonusArea == 0 && owner != NULL && owner->apiobj.field_0x27c != -1 &&
                    object->apiobj.field_0x27c != -1)
                    continue;
                if ((object->apiobj.character_data->game_character->flags_090 & 0x8000) != 0)
                    continue;
                if (bolt->bounds_min.x > object->apiobj.collision_max.x ||
                    object->apiobj.collision_min.x > bolt->bounds_max.x ||
                    bolt->bounds_min.z > object->apiobj.collision_max.z ||
                    object->apiobj.collision_min.z > bolt->bounds_max.z)
                    continue;
                if ((flags & 0x8000000) == 0 && (bolt->bounds_min.y > object->apiobj.collision_max.y ||
                                                 object->apiobj.collision_min.y > bolt->bounds_max.y))
                    continue;
                if (Bolt_HitGameObject(bolt, object, points, &bolt->bounds_min, &bolt->bounds_max,
                                       bolt->collision_radius, processed)) {
                    character_hit = true;
                    break;
                }
                flags = bolt->flags;
                count = HIGHGAMEOBJECT;
            }
        }
        if (character_hit)
            continue;
        bool interact = (bolt->flags & 0x13) != 0 ||
                        (bolt->owner != NULL &&
                         ((bolt->owner->field_0xefb & 0x10) != 0 || static_cast<u8>(bolt->owner->use_action) == 5));
        bool handled = false;
        if (interact) {
            if (expired && bolt->hit_platform != -1) {
                if (Bolt_HitPlatFn == NULL || !Bolt_HitPlatFn(bolt))
                    Bolt_HitPlat(bolt, processed, world);
                handled = true;
            } else if (GizmoSys_BoltHit(world->gizmo_sys, world, bolt, points, &bolt->bounds_min, &bolt->bounds_max,
                                        bolt->collision_radius, hits))
                handled = true;
            else if (GizmoBlowUp_Hit(bolt->owner, (bolt->flags & 0x200) ? &points[1] : points,
                                     (bolt->flags & 0x200) ? 1 : 3, bolt->collision_radius, &bolt->bounds_min,
                                     &bolt->bounds_max, bolt, 1, hits)) {
                BoltSys->debris(bolt, points, -1, NULL, 0);
                if (bolt->owner != NULL)
                    NewRumble(bolt->owner->pad_gamepad->pad, 0.6f, 0);
                Bolt_End(bolt, 1);
                Bolt_PlayHitSfx(bolt);
                handled = true;
            } else if (Bolt_HitParts(bolt, points, &bolt->bounds_min, &bolt->bounds_max, bolt->collision_radius,
                                     Bolt_HitPartMode(bolt)))
                handled = true;
        }
        if (!expired)
            continue;
        if (!handled) {
            if (!interact && surface_hit && bolt->hit_platform != -1 && bolt->field_0x104 != -1 &&
                (TerSurface[bolt->field_0x104].flags & 0x1000) != 0) {
                GameAudio_PlaySfx(41, &bolt->position, 0, 0);
                Bolt_AddDeflectedBolt(bolt, &bolt->field_0xac, &bolt->hit_normal, processed);
            }
            if (surface_hit && bolt->field_0x104 >= 0 && bolt->field_0x104 <= 31 &&
                (TerSurface[bolt->field_0x104].flags & 0x800) != 0 && (bolt->flags & 0x8000) == 0) {
                GameAudio_PlaySfx(41, &bolt->position, 0, 0);
                Bolt_AddDeflectedBolt(bolt, &bolt->velocity, &bolt->hit_normal, processed);
            } else if ((bolt->flags & 0x80000) == 0)
                Bolt_PlayHitSfx(bolt);
        }
        if (BoltSys->stop_targeting != NULL && bolt->owner != NULL && (bolt->owner->apiobj.flags_low & 0x80) != 0)
            BoltSys->stop_targeting(bolt->owner, &bolt->position);
    }
}

i8 BoltType_FindIDByName(char *name, WORLDINFO *world) {
    if (NuStrLen(name) == 0)
        return -1;
    for (i32 i = 7; i >= 0; --i) {
        if (NuStrICmp(name, world->bolt_types[i].name) == 0)
            return BoltSys->count + i;
    }
    // Local records were checked above. The original's count + 7 here reads
    // eight records beyond GlobalBoltType; search only the allocated globals.
    for (i32 i = BoltSys->count - 1; i >= 0; --i) {
        if (NuStrICmp(name, BoltSys->types[i].name) == 0)
            return i;
    }
    return -1;
}
