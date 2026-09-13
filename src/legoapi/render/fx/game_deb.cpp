#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "legoapi/render/fx/game_deb.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/glutils.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/android/nuptl_android.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"

#include <string.h>
#include <stdlib.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

NUMTL *CreateCopyMat(NUMTL *, i32, i32, i32, i32);

uv1deb *GenDebDummy(debkeydatatype_s *, debinftype *, float) {
    return NULL;
}

extern "C" {
    void AddFiniteShotDebrisEffect(i32 *, i32, NUVEC *, i32);
    void AddFiniteShotDebrisEffect2(i32 *, i32, NUVEC *, NUVEC *, NUVEC *, i32);
    void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);
    void AddVariableShotDebrisEffectMtx3(i32, NUVEC *, NUVEC *, i32, NUMTX *, NUMTX *);

    extern u32 debrisseed;
    extern f32 globaltime;
}

static dma_particle_s *DebrisParticleAt(debkeydatatype_s *key, i16 index, u8 particle_type) {
    const i32 particles_per_chunk = particle_type == 7 ? 12 : 32;
    return &key->particle_chunks[index / particles_per_chunk]->particles[index % particles_per_chunk];
}

uv1deb *GenDebIndex(debkeydatatype_s *key, debinftype *effect, float time) {
    i16 particle_index = key->field_18a;
    if (particle_index < key->particle_count) {
        ++key->field_18a;
    } else {
        particle_index = 0;
        key->field_18a = 1;
    }

    dma_particle_s *particle = DebrisParticleAt(key, particle_index, effect->particle_type);
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;

    NUVEC displacement;
    NuVecScale(&displacement, &effect->emitter_velocity, time - key->emission_time);

    NUVEC random_position;
    f32 random = NuRandFloatSeeded(&debrisseed);
    random_position.x = (random + random) * effect->field_058 - effect->field_058;
    random = NuRandFloatSeeded(&debrisseed);
    random_position.y = (random + random) * effect->field_05c - effect->field_05c;
    random = NuRandFloatSeeded(&debrisseed);
    random_position.z = (random + random) * effect->field_060 - effect->field_060;
    NuVecMtxTransformVU0(&random_position, &random_position, &key->emitter_orientation);
    NuVecAdd(&particle->position, &random_position, &displacement);

    random = NuRandFloatSeeded(&debrisseed);
    particle->momentum.x = (random + random) * effect->field_04c - effect->field_04c;
    random = NuRandFloatSeeded(&debrisseed);
    particle->momentum.y = (random + random) * effect->field_050 - effect->field_050 + effect->field_048;
    random = NuRandFloatSeeded(&debrisseed);
    particle->momentum.z = (random + random) * effect->field_054 - effect->field_054;
    NuVecMtxTransformVU0(&particle->momentum, &particle->momentum, &key->emitter_orientation);
    if (key->momentum_adjuster != NULL) {
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    }
    NuVecAdd(&particle->position, &particle->position, &key->emission_position);
    NuVecAdd(&particle->momentum, &particle->momentum, &key->momentum);

    for (i32 trail = 0; trail < static_cast<i8>(effect->trail_count); ++trail) {
        i16 trail_index = key->field_18a;
        if (trail_index < key->particle_count) {
            ++key->field_18a;
        } else {
            trail_index = 0;
            key->field_18a = 1;
        }
        dma_particle_s *trail_particle = DebrisParticleAt(key, trail_index, effect->particle_type);
        *trail_particle = *particle;
        trail_particle->start_time += static_cast<f32>(trail + 1) * effect->trail_time;
    }

    if (effect->native_data == NULL) {
        GenericDebinfoDmaTypeUpdate(effect);
    }
    return reinterpret_cast<uv1deb *>(particle);
}

uv1deb *GenDebIndexRadial(debkeydatatype_s *key, debinftype *effect, float time) {
    if (key->field_18a >= key->particle_count)
        key->field_18a = 0;
    if (effect->particle_type == 7)
        return NULL;
    dma_particle_s *particle = DebrisParticleAt(key, key->field_18a++, effect->particle_type);
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;
    f32 random = NuRandFloatSeeded(&debrisseed);
    i32 angle_y = static_cast<i32>((random + random) * effect->field_050 - effect->field_050 + effect->field_05c);
    random = NuRandFloatSeeded(&debrisseed);
    i32 angle_z = static_cast<i32>((random + random) * effect->field_054 - effect->field_054 + effect->field_060);
    f32 radius;
    if (effect->scale_in_time != 0.0f && effect->scale_in_time > globaltime - key->last_update_time)
        radius = (globaltime - key->last_update_time) / effect->scale_in_time * effect->field_058;
    else
        radius = effect->field_058;
    NUVEC vector = {0.0f, radius, 0.0f};
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    random = NuRandFloatSeeded(&debrisseed);
    vector.x = 0.0f;
    vector.y = (random + random) * effect->field_04c - effect->field_04c + effect->field_048;
    vector.z = 0.0f;
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;
    if (key->momentum_adjuster != NULL)
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    particle->position.x += key->emission_position.x;
    particle->position.y += key->emission_position.y;
    particle->position.z += key->emission_position.z;
    particle->momentum.x += key->momentum.x;
    particle->momentum.y += key->momentum.y;
    particle->momentum.z += key->momentum.z;
    for (i32 trail = 0; trail < static_cast<i8>(effect->trail_count); ++trail) {
        if (key->field_18a >= key->particle_count)
            key->field_18a = 0;
        dma_particle_s *copy = DebrisParticleAt(key, key->field_18a++, 0);
        *copy = *particle;
        copy->start_time += static_cast<f32>(trail + 1) * effect->trail_time;
    }
    if (effect->native_data == NULL)
        GenericDebinfoDmaTypeUpdate(effect);
    return reinterpret_cast<uv1deb *>(particle);
}

i32 SolveQuadratic(f32, f32, f32, f32 *, f32 *);
void DebrisGetControlStackLock();
void DebrisReleaseControlStackLock();
void AddChunkControlToStack(debris_chunk_control_s *, debris_chunk_control_s **);
extern "C" {
    extern i32 debrischunks, debrischunksglass, freechunkcontrolsptr;
    extern debris_chunk_control_s **freechunkcontrols;
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
}

uv1deb *GenDebIndexBounceY(debkeydatatype_s *key, debinftype *effect, float time) {
    if (key->field_18a >= key->particle_count)
        key->field_18a = 0;
    if (effect->particle_type == 7)
        return NULL;
    dma_particle_chunk_s *chunk = key->particle_chunks[key->field_18a / 32];
    dma_particle_s *particle = &chunk->particles[key->field_18a % 32];
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;

    NUVEC vector;
    f32 random = NuRandFloatSeeded(&debrisseed);
    vector.x = (random + random) * effect->field_058 - effect->field_058;
    random = NuRandFloatSeeded(&debrisseed);
    vector.y = (random + random) * effect->field_05c - effect->field_05c;
    random = NuRandFloatSeeded(&debrisseed);
    vector.z = (random + random) * effect->field_060 - effect->field_060;
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    random = NuRandFloatSeeded(&debrisseed);
    vector.x = (random + random) * effect->field_04c - effect->field_04c;
    random = NuRandFloatSeeded(&debrisseed);
    vector.y = (random + random) * effect->field_050 - effect->field_050 + effect->field_048;
    random = NuRandFloatSeeded(&debrisseed);
    vector.z = (random + random) * effect->field_054 - effect->field_054;
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;

    f32 first, second;
    if (SolveQuadratic(effect->field_0a0, particle->momentum.y, particle->position.y - key->collision_plane, &first,
                       &second)) {
        f32 collision_time = first > second ? first : second;
        if (collision_time > 0.0f && effect->particle_lifetime > collision_time) {
            DebrisGetControlStackLock();
            if (freechunkcontrolsptr < debrischunks + debrischunksglass) {
                debris_chunk_control_s *control = freechunkcontrols[freechunkcontrolsptr];
                control->particle_chunk = chunk;
                control->owner = NULL;
                control->active = 5;
                control->expiry_time = time + collision_time;
                control->effect_index = key->effect_index;
                control->rotation_y = 0;
                control->field_01e = 0;
                control->collision_plane = key->collision_plane;
                control->restitution = key->reflection_scale;
                control->collision_time = collision_time;
                control->particle_index = key->field_18a % 32;
                AddChunkControlToStack(control, &debris_chunk_control_stack[0]);
                ++freechunkcontrolsptr;
            }
            DebrisReleaseControlStackLock();
        }
    }
    if (key->momentum_adjuster != NULL)
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    ++key->field_18a;
    particle->position.x += key->emission_position.x;
    particle->position.y += key->emission_position.y;
    particle->position.z += key->emission_position.z;
    particle->momentum.x += key->momentum.x;
    particle->momentum.y += key->momentum.y;
    particle->momentum.z += key->momentum.z;
    if (effect->native_data == NULL)
        GenericDebinfoDmaTypeUpdate(effect);
    return reinterpret_cast<uv1deb *>(particle);
}

uv1deb *GenDebIndexBounceXZ(debkeydatatype_s *key, debinftype *effect, float time) {
    NUVEC normal = {1.0f, 0.0f, 0.0f};
    if (key->field_18a >= key->particle_count)
        key->field_18a = 0;
    if (effect->particle_type == 7)
        return NULL;
    dma_particle_chunk_s *chunk = key->particle_chunks[key->field_18a / 32];
    dma_particle_s *particle = &chunk->particles[key->field_18a % 32];
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;

    NUVEC vector;
    f32 random = NuRandFloatSeeded(&debrisseed);
    vector.x = (random + random) * effect->field_058 - effect->field_058;
    random = NuRandFloatSeeded(&debrisseed);
    vector.y = (random + random) * effect->field_05c - effect->field_05c;
    random = NuRandFloatSeeded(&debrisseed);
    vector.z = (random + random) * effect->field_060 - effect->field_060;
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    random = NuRandFloatSeeded(&debrisseed);
    vector.x = (random + random) * effect->field_04c - effect->field_04c;
    random = NuRandFloatSeeded(&debrisseed);
    vector.y = (random + random) * effect->field_050 - effect->field_050 + effect->field_048;
    random = NuRandFloatSeeded(&debrisseed);
    vector.z = (random + random) * effect->field_054 - effect->field_054;
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;

    NuVecRotateY(&normal, &normal, key->reflection_y);
    NUVEC plane_position;
    NuVecScale(&plane_position, &normal, key->collision_plane);
    NUVEC end_position = {particle->momentum.x * effect->particle_lifetime + particle->position.x, 0.0f,
                          effect->particle_lifetime * particle->momentum.z + particle->position.z};
    NUVEC start_delta, end_delta;
    NuVecSub(&start_delta, &plane_position, &particle->position);
    NuVecSub(&end_delta, &plane_position, &end_position);
    f32 start_distance = start_delta.x * normal.x + start_delta.z * normal.z;
    f32 end_distance = normal.x * end_delta.x + normal.z * end_delta.z;
    if ((start_distance < 0.0f && end_distance > 0.0f) || (start_distance > 0.0f && end_distance < 0.0f)) {
        f32 lifetime = effect->particle_lifetime;
        DebrisGetControlStackLock();
        if (freechunkcontrolsptr < debrischunks + debrischunksglass) {
            f32 collision_time = (0.0f - start_distance) / (end_distance - start_distance) * lifetime;
            debris_chunk_control_s *control = freechunkcontrols[freechunkcontrolsptr];
            control->particle_chunk = chunk;
            control->expiry_time = time + collision_time;
            control->owner = NULL;
            control->active = 6;
            control->effect_index = key->effect_index;
            control->rotation_y = key->reflection_y;
            control->field_01e = 0;
            control->collision_plane = key->collision_plane;
            control->restitution = key->reflection_scale;
            control->collision_time = collision_time;
            control->particle_index = key->field_18a % 32;
            AddChunkControlToStack(control, &debris_chunk_control_stack[0]);
            ++freechunkcontrolsptr;
        }
        DebrisReleaseControlStackLock();
    }
    if (key->momentum_adjuster != NULL)
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    ++key->field_18a;
    particle->position.x += key->emission_position.x;
    particle->position.y += key->emission_position.y;
    particle->position.z += key->emission_position.z;
    particle->momentum.x += key->momentum.x;
    particle->momentum.y += key->momentum.y;
    particle->momentum.z += key->momentum.z;
    if (effect->native_data == NULL)
        GenericDebinfoDmaTypeUpdate(effect);
    return reinterpret_cast<uv1deb *>(particle);
}

uv1deb *GenDebIndexSpheroid(debkeydatatype_s *key, debinftype *effect, float time) {
    i16 particle_index = key->field_18a;
    if (particle_index < key->particle_count) {
        ++key->field_18a;
    } else {
        particle_index = 0;
        key->field_18a = 1;
    }

    dma_particle_s *particle = DebrisParticleAt(key, particle_index, effect->particle_type);
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;

    NUVEC vector = {NuFsqrt(NuRandFloatSeeded(&debrisseed)), 0.0f, 0.0f};
    i32 angle_y = static_cast<i32>(NuRandFloatSeeded(&debrisseed) * 65536.0f);
    f32 random = NuRandFloatSeeded(&debrisseed);
    f32 sine = random + random - 1.0f;
    f32 absolute = NuFabs(sine);
    f32 root = NuFsqrt(1.0f - sine * sine);
    f32 small = root < absolute ? root : absolute;
    f32 region = (absolute - 0.70710677f) * 3.40282e+38f;
    region = region < 1.0f ? (region > -1.0f ? region : -1.0f) : 1.0f;
    f32 sign = sine * 3.40282e+38f;
    sign = sign < 1.0f ? (sign > -1.0f ? sign : -1.0f) : 1.0f;
    f32 quadrant = region * sign;
    f32 argument = small * quadrant;
    f32 squared = argument * argument;
    f32 cubed = argument * squared;
    f32 fourth = squared * squared;
    f32 fifth = squared * cubed;
    f32 radians = (quadrant + sign) * 0.785398f - argument;
    radians += (argument * -0.166667f) * squared;
    radians = (-0.075f * squared) * cubed + radians;
    radians += (-0.0446429f * cubed) * fourth;
    radians += (-0.0303819f * fourth) * fifth;
    i16 angle_z = static_cast<i16>(static_cast<i32>(radians * 10430.4f));
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NUMTX scale;
    NuMtxSetIdentity(&scale);
    scale.m00 = effect->field_058;
    scale.m11 = effect->field_05c;
    scale.m22 = effect->field_060;
    NuVecMtxTransformVU0(&vector, &vector, &scale);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    random = NuRandFloatSeeded(&debrisseed);
    vector.x = (random + random) * effect->field_04c - effect->field_04c;
    random = NuRandFloatSeeded(&debrisseed);
    vector.y = (random + random) * effect->field_050 - effect->field_050 + effect->field_048;
    random = NuRandFloatSeeded(&debrisseed);
    vector.z = (random + random) * effect->field_054 - effect->field_054;
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;
    if (key->momentum_adjuster != NULL) {
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    }
    NuVecAdd(&particle->position, &particle->position, &key->emission_position);
    NuVecAdd(&particle->momentum, &particle->momentum, &key->momentum);

    for (i32 trail = 0; trail < static_cast<i8>(effect->trail_count); ++trail) {
        i16 trail_index = key->field_18a;
        if (trail_index < key->particle_count) {
            ++key->field_18a;
        } else {
            trail_index = 0;
            key->field_18a = 1;
        }
        dma_particle_s *trail_particle = DebrisParticleAt(key, trail_index, effect->particle_type);
        *trail_particle = *particle;
        trail_particle->start_time += static_cast<f32>(trail + 1) * effect->trail_time;
    }

    if (effect->native_data == NULL) {
        GenericDebinfoDmaTypeUpdate(effect);
    }
    return reinterpret_cast<uv1deb *>(particle);
}

void GenDebMomAdjFromPos(debkeydatatype_s *, debinftype *, uv1deb *data) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    particle->momentum.x = particle->position.x * 2.0f;
    particle->momentum.z = particle->position.z * 2.0f;
}

uv1deb *GenDebIndexRadialStar(debkeydatatype_s *key, debinftype *effect, float time) {
    if (key->field_18a >= key->particle_count)
        key->field_18a = 0;
    if (effect->particle_type == 7)
        return NULL;
    dma_particle_s *particle = DebrisParticleAt(key, key->field_18a++, effect->particle_type);
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;
    f32 random = NuRandFloatSeeded(&debrisseed);
    i32 angle_y = static_cast<i32>((random + random) * effect->field_050 - effect->field_050 + effect->field_05c);
    random = NuRandFloatSeeded(&debrisseed);
    i32 angle_z = static_cast<i32>((random + random) * effect->field_054 - effect->field_054 + effect->field_060);
    f32 radius;
    if (effect->scale_in_time != 0.0f && effect->scale_in_time > globaltime - key->last_update_time)
        radius = (globaltime - key->last_update_time) / effect->scale_in_time * effect->field_058;
    else
        radius = effect->field_058;
    i32 segment_angle = 0x10000 / static_cast<i8>(effect->radial_segments);
    i32 phase = static_cast<i32>(static_cast<f32>((angle_y + 0x10000) % segment_angle) /
                                 static_cast<f32>(segment_angle) * 32768.0f);
    NUVEC vector = {
        0.0f,
        ((1.0f - effect->radial_floor) * (1.0f - NuTrigTable[(phase >> 1) & 0x7fff]) + effect->radial_floor) * radius,
        0.0f};
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    random = NuRandFloatSeeded(&debrisseed);
    vector.x = 0.0f;
    vector.y = (random + random) * effect->field_04c - effect->field_04c + effect->field_048;
    vector.z = 0.0f;
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;
    if (key->momentum_adjuster != NULL)
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    particle->position.x += key->emission_position.x;
    particle->position.y += key->emission_position.y;
    particle->position.z += key->emission_position.z;
    particle->momentum.x += key->momentum.x;
    particle->momentum.y += key->momentum.y;
    particle->momentum.z += key->momentum.z;
    for (i32 trail = 0; trail < static_cast<i8>(effect->trail_count); ++trail) {
        if (key->field_18a >= key->particle_count)
            key->field_18a = 0;
        dma_particle_s *copy = DebrisParticleAt(key, key->field_18a++, 0);
        *copy = *particle;
        copy->start_time += static_cast<f32>(trail + 1) * effect->trail_time;
    }
    if (effect->native_data == NULL)
        GenericDebinfoDmaTypeUpdate(effect);
    return reinterpret_cast<uv1deb *>(particle);
}

uv1deb *GenDebIndexRadialRotor(debkeydatatype_s *key, debinftype *effect, float time) {
    if (key->field_18a >= key->particle_count)
        key->field_18a = 0;
    if (effect->particle_type == 7)
        return NULL;
    dma_particle_s *particle = DebrisParticleAt(key, key->field_18a++, effect->particle_type);
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;
    NUVEC displacement;
    NuVecScale(&displacement, &effect->emitter_velocity, time - key->emission_time);
    NuVecAdd(&displacement, &displacement, reinterpret_cast<NUVEC *>(&effect->field_058));
    i32 angle_y = static_cast<i32>(static_cast<f32>(key->emitter_rotation_x) + displacement.y);
    i32 angle_z = static_cast<i32>(static_cast<f32>(key->emitter_rotation_y) + displacement.z);
    NUVEC vector = {0.0f, displacement.x, 0.0f};
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    f32 random = NuRandFloatSeeded(&debrisseed);
    vector.x = 0.0f;
    vector.y = (random + random) * effect->field_04c - effect->field_04c + effect->field_048;
    vector.z = 0.0f;
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;
    if (key->momentum_adjuster != NULL)
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    particle->position.x += key->emission_position.x;
    particle->position.y += key->emission_position.y;
    particle->position.z += key->emission_position.z;
    particle->momentum.x += key->momentum.x;
    particle->momentum.y += key->momentum.y;
    particle->momentum.z += key->momentum.z;
    for (i32 trail = 0; trail < static_cast<i8>(effect->trail_count); ++trail) {
        if (key->field_18a >= key->particle_count)
            key->field_18a = 0;
        dma_particle_s *copy = DebrisParticleAt(key, key->field_18a++, 0);
        *copy = *particle;
        copy->start_time += static_cast<f32>(trail + 1) * effect->trail_time;
    }
    if (effect->native_data == NULL)
        GenericDebinfoDmaTypeUpdate(effect);
    if (effect->field_050 != 0.0f)
        key->emitter_rotation_x += static_cast<i16>(static_cast<i32>(effect->field_050));
    else
        key->emitter_rotation_x = 0;
    if (effect->field_054 != 0.0f)
        key->emitter_rotation_y += static_cast<i16>(static_cast<i32>(effect->field_054));
    else
        key->emitter_rotation_y = 0;
    return reinterpret_cast<uv1deb *>(particle);
}

void GenDebMomAdjFromPosAll(debkeydatatype_s *, debinftype *, uv1deb *data) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    particle->momentum.x = particle->position.x * 5.4f;
    particle->momentum.y = particle->position.y * 5.4f;
    particle->momentum.z = particle->position.z * 5.4f;
}

void GenDebMomAdjFromPosRev(debkeydatatype_s *, debinftype *, uv1deb *data) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    particle->momentum.x = -particle->position.x;
    particle->momentum.z = -particle->position.z;
}

void GenDebMomAdjFromSplash(debkeydatatype_s *, debinftype *, uv1deb *data) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    particle->momentum.x = particle->position.x * 4.0f;
    particle->momentum.z = particle->position.z * 4.0f;
}

void GenDebMomAdjFromAshRock(debkeydatatype_s *, debinftype *, uv1deb *data) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    particle->momentum.x = particle->position.x * 16.0f;
    particle->momentum.z = particle->position.z * 16.0f;
}

uv1deb *GenDebIndexImprovedRadial(debkeydatatype_s *key, debinftype *effect, float time) {
    if (key->field_18a >= key->particle_count)
        key->field_18a = 0;
    if (effect->particle_type == 7)
        return NULL;
    dma_particle_s *particle = DebrisParticleAt(key, key->field_18a++, effect->particle_type);
    particle->start_time = time;
    key->emission_epoch = time;
    particle->inverse_lifetime = 64.0f / effect->particle_lifetime;
    NUVEC displacement;
    NuVecScale(&displacement, &effect->emitter_velocity, time - key->emission_time);
    NuVecAdd(&displacement, &displacement, reinterpret_cast<NUVEC *>(&effect->field_058));
    i32 random_angle = static_cast<i32>(NuRandFloatSeeded(&debrisseed) * 65536.0f - 32768.0f);
    f32 lower_sine = NU_SIN_LUT(static_cast<i32>(displacement.z - effect->field_054));
    f32 upper_sine = NU_SIN_LUT(static_cast<i32>(effect->field_054 + displacement.z));
    f32 random = NuRandFloatSeeded(&debrisseed);
    f32 sine = random * (upper_sine - lower_sine) + lower_sine;
    f32 absolute = NuFabs(sine);
    f32 root = NuFsqrt(1.0f - sine * sine);
    f32 small = root < absolute ? root : absolute;
    f32 region = (absolute - 0.70710677f) * 3.40282e+38f;
    region = region < 1.0f ? (region > -1.0f ? region : -1.0f) : 1.0f;
    f32 sign = sine * 3.40282e+38f;
    sign = sign < 1.0f ? (sign > -1.0f ? sign : -1.0f) : 1.0f;
    f32 quadrant = region * sign;
    f32 argument = small * quadrant;
    f32 squared = argument * argument;
    f32 cubed = argument * squared;
    f32 fourth = squared * squared;
    f32 fifth = squared * cubed;
    f32 radians = (quadrant + sign) * 0.785398f - argument;
    radians += (argument * -0.166667f) * squared;
    radians = (-0.075f * squared) * cubed + radians;
    radians += (-0.0446429f * cubed) * fourth;
    radians += (-0.0303819f * fourth) * fifth;
    i16 angle_z = static_cast<i16>(static_cast<i32>(radians * 10430.4f));
    i32 angle_y = static_cast<i32>(displacement.y) +
                  static_cast<i32>(static_cast<f32>(random_angle) * effect->field_050 * 0.000030517578125f);
    NUVEC vector = {displacement.x, 0.0f, 0.0f};
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->position = vector;
    random = NuRandFloatSeeded(&debrisseed);
    vector.x = (random + random) * effect->field_04c - effect->field_04c + effect->field_048;
    vector.y = 0.0f;
    vector.z = 0.0f;
    NuVecRotateZ(&vector, &vector, angle_z);
    NuVecRotateY(&vector, &vector, angle_y);
    NuVecMtxTransformVU0(&vector, &vector, &key->emitter_orientation);
    particle->momentum = vector;
    if (key->momentum_adjuster != NULL)
        key->momentum_adjuster(key, effect, reinterpret_cast<uv1deb *>(particle));
    particle->position.x += key->emission_position.x;
    particle->position.y += key->emission_position.y;
    particle->position.z += key->emission_position.z;
    particle->momentum.x += key->momentum.x;
    particle->momentum.y += key->momentum.y;
    particle->momentum.z += key->momentum.z;
    for (i32 trail = 0; trail < static_cast<i8>(effect->trail_count); ++trail) {
        if (key->field_18a >= key->particle_count)
            key->field_18a = 0;
        dma_particle_s *copy = DebrisParticleAt(key, key->field_18a++, 0);
        *copy = *particle;
        copy->start_time += static_cast<f32>(trail + 1) * effect->trail_time;
    }
    if (effect->native_data == NULL)
        GenericDebinfoDmaTypeUpdate(effect);
    return reinterpret_cast<uv1deb *>(particle);
}

void GenDebMomAdjFromPosRevTree(debkeydatatype_s *, debinftype *effect, uv1deb *data) {
    dma_particle_s *particle = reinterpret_cast<dma_particle_s *>(data);
    particle->momentum.x += particle->position.x * -0.6f;
    particle->momentum.z += particle->position.z * -0.6f;
    particle->position.y -=
        NuFsqrt(particle->position.x * particle->position.x + particle->position.z * particle->position.z) * 0.4f;
    f32 lifetime = effect->particle_lifetime;
    particle->inverse_lifetime = 64.0f / (static_cast<f32>(lrand48()) * lifetime / 1503238528.0f + lifetime);
}

extern "C" {

    // These are the original registry globals.  `effecttypes` is the
    // contiguous record arena and `debtab` is the separately allocated table
    // of pointers into it.  Slot zero is reserved by the loader.
    debinftype *effecttypes = NULL;
    debinftype **debtab = NULL;
    i32 EDPP_MAX_TYPES = 0;
    i32 EDPP_SCALE_TYPES = 20;
    i32 edpp_types_used = 0;
    i32 DEBPAGE_AREA = -1;
    i32 DEBPAGE_CHARACTER = -1;
    i32 DEBPAGE_GENERAL = -1;
    usize edpp_page_scene[8] = {};
    i32 edpp_page_on[8] = {};
    i32 edpp_page_used[8] = {};
    edpp_particle_s edpp_ptls[512] = {};
    i32 edpp_nearest;
    NUVEC edpp_cam_pos;
    i32 edpp_instances_used = 0;
    debkeydatatype_s *debkeydata = NULL;
    i16 *freedebkeys = NULL;
    i32 freedebkeyptr = 0;
    i32 maxdebkeys = 0;
    debkeydatatype_s *debris_keystack = NULL;
    f32 globaltime = 0.0f;
    f32 panelglobaltime = 0.0f;
    f32 renderglobaltime = 0.0f;
    f32 renderpanelglobaltime = 0.0f;
    f32 timeincrement = 0.0f;
    i32 globalframes = 0;
    i32 update_debris_enabled = 1;
    u32 debrisseed = 0x5c0999;
    i32 processdeb = 0;
    f32 glyntestha = 0.0f;
    DEBRISMOMENTUMADJUSTER gencodetab[7] = {
        NULL,
        GenDebMomAdjFromPos,
        GenDebMomAdjFromPosRev,
        GenDebMomAdjFromSplash,
        GenDebMomAdjFromAshRock,
        GenDebMomAdjFromPosRevTree,
        GenDebMomAdjFromPosAll,
    };
    DEBRISGENERATOR gensorttab[13] = {
        GenDebIndex,           GenDebDummy,        GenDebDummy,         GenDebDummy,
        GenDebDummy,           GenDebDummy,        GenDebIndexRadial,   GenDebIndexRadialRotor,
        GenDebIndexSpheroid,   GenDebIndexBounceY, GenDebIndexBounceXZ, GenDebIndexImprovedRadial,
        GenDebIndexRadialStar,
    };
    void *perm_debrissys = NULL;
    PartHeader **DmaDebTypes = NULL;
    i32 EDPP_MAX_DMADEBTYPES = 0x40;
    i32 freeDmaDebType = 0;
    i32 debris_setup_called = 0;
    usize debris_trash_space = 0;
    usize debris_trash_size = 0;
    u32 *spherecolldata = NULL;
    u32 *toruscolldata = NULL;
    debscale_s *debscale = NULL;
    i32 debrischunks = 0;
    i32 debrischunksglass = 0;
    i32 freedebchkptr = 0;
    i32 freedebchkptrg = 0;
    debris_chunk_control_s *debris_chunk_controls = NULL;
    debris_chunk_control_s **freechunkcontrols = NULL;
    i32 freechunkcontrolsptr = 0;
    dma_particle_chunk_s **freedebchunks = NULL;
    dma_particle_chunk_s **freedebchunksglass = NULL;
    particlechunkrendertype_s *ParticleChunkToRender = NULL;
    particlechunkrendertype_s *ParticleChunkRenderStack[5] = {};
    debris_chunk_control_s *debris_chunk_control_stack[2] = {};
    NUMTL *DebMat[10] = {};
    i32 Debris_Init;
    i32 numspherecolldata;
    i32 numtoruscolldata;
    NUMTX xzfacingmtx;
    debinftype nulleffecttype = {
        "null",                                       // name
        0,                                            // category
        0,                                            // page
        0,                                            // cutscene_only
        0,                                            // disabled
        100,                                          // max_particles
        60,                                           // frequency
        0,                                            // emission_period
        1.0f,                                         // emission_period_random
        0,                                            // emission_pause
        0,                                            // emission_pause_random
        0,                                            // start_offset_random
        0,                                            // generator_type
        0,                                            // momentum_adjustment_type
        0,                                            // particle_type
        0,                                            // status
        {0x0, 0x40, 0x1c, 0x47},                      // fields_030
        25.0f,                                        // clip_extent
        0,                                            // sound_range
        0,                                            // sound_range_override
        0.5f,                                         // field_044
        1.0f,                                         // field_048
        0,                                            // field_04c
        0,                                            // field_050
        0,                                            // field_054
        0,                                            // field_058
        0,                                            // field_05c
        0,                                            // field_060
        {},                                           // emitter_velocity
        {},                                           // fields_070
        0,                                            // field_0a0
        1.0f,                                         // particle_lifetime
        0,                                            // field_0a8
        0,                                            // field_0aa
        0,                                            // field_0ab
        0,                                            // field_0ac
        0,                                            // field_0b0
        0,                                            // field_0b4
        0,                                            // field_0b8
        0,                                            // field_0bc
        {{0.0f, 64, 64, 64, 0}, {1.0f, 0, 0, 0, 0}},  // colour_keys
        {{0.0f, 64.0f}, {1.0f, 0.0f}},                // alpha_keys
        0.125f,                                       // field_140
        0.125f,                                       // field_144
        0,                                            // field_148
        500.0f,                                       // field_14c
        {{0.0f, 500.0f}, {1.0f, 500.0f}},             // width_keys
        {{0.0f, 500.0f}, {1.0f, 500.0f}},             // height_keys
        {0x0, 0x0, 0xb4, 0xc3, 0x0, 0x0, 0xb4, 0x43}, // fields_1d0
        {{0.0f, 0.0f}, {1.0f, 0.0f}},                 // rotation_keys
        {
            // fields_218
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x3f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,
            0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x3f,
        },
        0,                                                              // texture_u0
        0,                                                              // texture_v0
        256.0f,                                                         // texture_u1
        256.0f,                                                         // texture_v1
        0,                                                              // native_data
        0,                                                              // last_render_time
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x3f}, // fields_2b0
        0,                                                              // process_spheres
        0,                                                              // time_group
        3,                                                              // field_2f2
        0,                                                              // use_explicit_clip_box
        4.0f,                                                           // thinning
        {
            // fields_2f8
            0x0,  0x0,  0x80, 0x3f, 0x0,  0x0,  0x80, 0x3f, 0x0, 0x0,  0x80, 0x3f, 0x0, 0x0,  0x80, 0x3f, 0xcd,
            0xcc, 0xcc, 0x3d, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x80, 0x3f, 0x0,  0x0,  0x80, 0x3f, 0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x80, 0x3f, 0x0,  0x0,
            0x80, 0x3f, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0,
            0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x80, 0x3f, 0x0,  0x0, 0x80, 0x3f,
        },
        {-1, -1, -1, -1, -1, -1, -1, -1},         // particle_keys
        {-1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0}, // sound_data
        0,                                        // trail_count
        0,                                        // radial_segments
        0,                                        // camera_facing
        0,                                        // field_413
        0,                                        // trail_time
        0,                                        // scale_in_time
        0.5f,                                     // radial_floor
        1.0f,                                     // scale
        0,                                        // unscaled_effect_index
    };
    void NuRegisterEndFrameCallBackFn(void (*callback)(void));
    void DebrisProcessTimeSlip(void);
    extern i32 debris_suspended;

    void DebrisTrashableSetup(VARIPTR *, VARIPTR *);

    void DebrisSetup2(VARIPTR *, VARIPTR, VARIPTR *, VARIPTR, char *, i32, i32, i32, i32);

    void DebrisSetup2(VARIPTR *buffer, VARIPTR buffer_end, VARIPTR *trash_buffer, VARIPTR trash_end, char *texture_name,
                      i32 chunk_count, i32 glass_chunk_count, i32 key_count, i32 effect_count) {
        if (debris_trash_space == 0) {
            debrischunks = chunk_count;
            debrischunksglass = glass_chunk_count;
        }
        debris_setup_called = 1;
        maxdebkeys = key_count;
        EDPP_MAX_TYPES = effect_count;
        Debris_Init = 1;
        NuRegisterEndFrameCallBackFn(DebrisProcessTimeSlip);

        const i32 total_chunks = debrischunks + debrischunksglass;
        buffer->addr = ALIGN(buffer->addr, 0x10);
        debris_chunk_controls = reinterpret_cast<debris_chunk_control_s *>(buffer->addr);
        buffer->addr += static_cast<usize>(total_chunks) * 2 * sizeof(debris_chunk_control_s);
        freechunkcontrols = reinterpret_cast<debris_chunk_control_s **>(buffer->addr);
        buffer->addr += static_cast<usize>(total_chunks) * 2 * sizeof(debris_chunk_control_s *);
        freedebchunks = reinterpret_cast<dma_particle_chunk_s **>(buffer->addr);
        buffer->addr += static_cast<usize>(debrischunks) * sizeof(dma_particle_chunk_s *);
        freedebchunksglass = reinterpret_cast<dma_particle_chunk_s **>(buffer->addr);
        buffer->addr += static_cast<usize>(debrischunksglass) * sizeof(dma_particle_chunk_s *);
        ParticleChunkToRender = reinterpret_cast<particlechunkrendertype_s *>(buffer->addr);
        buffer->addr += static_cast<usize>(total_chunks) * sizeof(particlechunkrendertype_s);

        buffer->addr = ALIGN(buffer->addr, 0x10);
        debkeydata = reinterpret_cast<debkeydatatype_s *>(buffer->addr);
        buffer->addr += static_cast<usize>(maxdebkeys) * sizeof(debkeydatatype_s);
        freedebkeys = reinterpret_cast<i16 *>(buffer->addr);
        buffer->addr += static_cast<usize>(maxdebkeys) * sizeof(i16);
        effecttypes = reinterpret_cast<debinftype *>(buffer->addr);
        buffer->addr += static_cast<usize>(EDPP_MAX_TYPES) * sizeof(debinftype);
        debtab = reinterpret_cast<debinftype **>(buffer->addr);
        buffer->addr += static_cast<usize>(EDPP_MAX_TYPES) * sizeof(debinftype *);
        DmaDebTypes = reinterpret_cast<PartHeader **>(buffer->addr);
        buffer->addr += static_cast<usize>(EDPP_MAX_DMADEBTYPES) * sizeof(PartHeader *);
        spherecolldata = reinterpret_cast<u32 *>(buffer->addr);
        buffer->addr += static_cast<usize>(maxdebkeys) * 2 * sizeof(u32);
        toruscolldata = reinterpret_cast<u32 *>(buffer->addr);
        buffer->addr += static_cast<usize>(maxdebkeys) * 2 * sizeof(u32);
        debscale = reinterpret_cast<debscale_s *>(buffer->addr);
        buffer->addr += static_cast<usize>(EDPP_SCALE_TYPES) * sizeof(debscale_s);

        memset(freedebkeys, 0, static_cast<usize>(maxdebkeys) * sizeof(i16));
        memset(debkeydata, 0, static_cast<usize>(maxdebkeys) * sizeof(debkeydatatype_s));
        memset(effecttypes, 0, static_cast<usize>(EDPP_MAX_TYPES) * sizeof(debinftype));
        memset(debtab, 0, static_cast<usize>(EDPP_MAX_TYPES) * sizeof(debinftype *));
        memset(debscale, 0, static_cast<usize>(EDPP_SCALE_TYPES) * sizeof(u32));

        for (i32 i = 0; i < maxdebkeys; ++i) {
            spherecolldata[i * 2] = static_cast<u32>(i);
            spherecolldata[i * 2 + 1] = 3;
            toruscolldata[i * 2] = static_cast<u32>(i);
            toruscolldata[i * 2 + 1] = 3;
        }

        effecttypes[0] = nulleffecttype;
        debtab[0] = &effecttypes[0];
        edpp_types_used = 1;
        numspherecolldata = maxdebkeys;
        numtoruscolldata = maxdebkeys;

        DebMat[0] = CreateAlphaBlendTexture(buffer, buffer_end, texture_name, 1, 2, 0x100, 0);
        const i16 texture_id = DebMat[0]->tex_id;
        DebMat[0]->attribs.z_mode = 1;
        DebMat[0]->attribs.alpha_test = 1;
        NuMtlUpdate(DebMat[0]);
        DebMat[1] = CreateCopyMat(DebMat[0], 0, 0, 3, 0);
        DebMat[2] = CreateCopyMat(DebMat[0], 1, 1, 1, 1);
        DebMat[3] = CreateCopyMat(DebMat[0], 1, 3, 1, 1);
        DebMat[4] = CreateCopyMat(DebMat[0], 0, 1, 1, 1);
        DebMat[5] = CreateCopyMat(DebMat[0], 0, 2, 1, 1);
        DebMat[6] = CreateCopyMat(DebMat[0], 0, 0, 0, 1);
        for (i32 i = 0; i <= 6; ++i) {
            DebMat[i]->shader_desc.vtx_desc.unknown_2_16 = 1;
            DebMat[i]->tex_id = texture_id;
        }

        DebMat[7] = NuMtlCreateEx3D(1, 2);
        NUMTL *glass = DebMat[7];
        glass->diffuse_color = {1.0f, 1.0f, 1.0f};
        glass->opacity = 0.999f;
        f32 glass_parameter = 0.25f;
        memcpy(glass->filler3, &glass_parameter, sizeof(glass_parameter));
        glass->tex_id = -1;
        glass->shader_desc.vtx_desc.unknown_2_16 = 1;
        glass->attribs.alpha_mode = 1;
        glass->attribs.filter_mode = 0;
        glass->attribs.unknown_1_1_2 = 1;
        glass->attribs.unknown_1_4_8 = 1;
        glass->attribs.cull_mode = 2;
        glass->attribs.z_mode = 1;
        glass->attribs.unknown_2_1_2 = 2;
        glass->attribs.unknown_2_4 = 1;
        glass->attribs.alpha_fail |= 2;
        glass->attribs.uv_mode = 0;
        glass->sort_pri = 128;
        glass->particle_type_tag = -105;
        NuMtlUpdate(glass);
        DebMat[8] = CreateCopyMat(DebMat[0], 0, 6, 0, 1);

        for (debinftype **entry = debtab; *entry != NULL; ++entry) {
            debinftype *effect = *entry;
            if (effect->field_0ab == 0)
                effect->field_0ab = 1;
            f32 frame_rate = static_cast<f32>((effect->field_0a8 * 60) / static_cast<i8>(effect->field_0ab));
            u32 packed;
            memcpy(&packed, &frame_rate, sizeof(packed));
            packed = (packed & 0xfffff800) + ((effect->field_0a8 * (static_cast<i8>(effect->field_0aa) - 1)) & 0x3ff);
            memcpy(&effect->field_0ac, &packed, sizeof(packed));
            effect->alpha_keys[0].value /= effect->particle_lifetime;
            effect->texture_u0 += 524288.0f;
            effect->texture_u1 += 524288.0f;
            effect->texture_v0 += 524288.0f;
            effect->texture_v1 += 524288.0f;
        }
        for (i32 i = 0; i < EDPP_MAX_TYPES; ++i)
            effecttypes[i].native_data = NULL;
        freeDmaDebType = 0;
        if (debris_suspended == 0) {
            if (trash_buffer != NULL)
                DebrisTrashableSetup(trash_buffer, &trash_end);
            else
                DebrisTrashableSetup(buffer, &buffer_end);
        }
        for (i32 i = 0; i < maxdebkeys; ++i)
            freedebkeys[i] = static_cast<i16>(i);
        for (i32 i = 0; i < maxdebkeys; ++i) {
            debkeydatatype_s *key = &debkeydata[i];
            key->field_184 = 0;
            key->field_18a = 0;
            key->emitter_rotation_x = 0;
            key->emitter_rotation_y = 0;
            key->field_1d4 = 0;
            key->effect_index = 0;
            key->allocation_index = -1;
        }

        NuMtxSetIdentity(&xzfacingmtx);
        NuMtxRotateX(&xzfacingmtx, -0x4000);
    }

    // DebrisSetup @0x34c7d0 is only the argument-shuffling wrapper around the
    // real setup routine.  Keep the call shape visible; DebrisSetup2 owns the
    // arena and all of the ancillary debris tables.
    void DebrisSetup(VARIPTR *p1, VARIPTR p2, char *p3, i32 p4, i32 p5, i32 p6) {
        DebrisSetup2(p1, p2, p1, p2, p3, p4, 0x20, p5, p6);
    }

    // LookupDebrisEffectPageIgnore @0x355d30. Search the requested page first,
    // then the two permanent pages, then every other active page.
    i32 LookupDebrisEffectPageIgnore(char *name, i32 page, i32 ignore) {
        if (name == NULL || debtab == NULL) {
            return -1;
        }

        if (static_cast<u32>(page) < 8 && edpp_page_used[page] != 0) {
            for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
                debinftype *effect = debtab[i];
                if (i != ignore && effect != NULL && effect->page == static_cast<u8>(page) &&
                    NuStrICmp(effect->name, name) == 0) {
                    return i;
                }
            }
        }

        for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
            debinftype *effect = debtab[i];
            if (i != ignore && effect != NULL && effect->page < 2 && edpp_page_used[effect->page] != 0 &&
                NuStrICmp(effect->name, name) == 0) {
                return i;
            }
        }

        for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
            debinftype *effect = debtab[i];
            if (i != ignore && effect != NULL && effect->page < 8 && edpp_page_used[effect->page] != 0 &&
                NuStrICmp(effect->name, name) == 0) {
                return i;
            }
        }
        return -1;
    }

    // LookupDebrisEffectPage @0x355ef0: tail call into the Ignore variant.
    i32 LookupDebrisEffectPage(char *name, char page) {
        return LookupDebrisEffectPageIgnore(name, page, 0);
    }

    i32 LookupDebrisEffectPageOnly(char *name, char page) {
        if (name == NULL || debtab == NULL) {
            return -1;
        }

        const u8 requested_page = static_cast<u8>(page);
        if (requested_page < 8 && edpp_page_used[requested_page] != 0) {
            for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
                debinftype *effect = debtab[i];
                if (effect != NULL && effect->page == requested_page && NuStrICmp(effect->name, name) == 0) {
                    return i;
                }
            }
        }

        for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
            debinftype *effect = debtab[i];
            if (effect != NULL && effect->page < 2 && edpp_page_used[effect->page] != 0 &&
                NuStrICmp(effect->name, name) == 0) {
                return i;
            }
        }
        return -1;
    }

    // InitGameDebris @0x3ca2d0. Carves a typed debris system and its entries
    // from the world's particle bump buffer. Pointer-sized fields and sizeof
    // keep the host layout valid without changing the original 32-bit layout.

    i32 LookupDebrisEffect(char *name) {
        return LookupDebrisEffectPage(name, 0);
    }

} // extern "C"
