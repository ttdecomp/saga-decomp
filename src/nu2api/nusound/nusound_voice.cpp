// NuSoundVoice — decompiled from libTTapp.so (nu2api.2013/nusound/nusound.cpp).
// Engine-side voice: play state machine, volume/pitch, the eight output
// gains fed from the positional mix, and the per-frame Update that pushes the
// mix down into the platform voice (NuVoiceAndroid).

#include "nu2api_nusound_types.h"

#include "decomp.h"

#include "nu2api/nucore/nuthread.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nusound/nusound_bus.hpp"
#include "nu2api/nusound/nusound_source.hpp"

#include <string.h>

pthread_mutex_t NuSoundVoice::sStateCriticalSection = PTHREAD_MUTEX_INITIALIZER;

namespace {
    struct VoiceListenerLink {
        NuSoundListener *listener;
        VoiceListenerLink *previous;
        VoiceListenerLink *next;
    };

    static inline void DetachVoiceListener(VoiceListenerLink *link) {
        NuSoundListener *listener = link->listener;
        if (listener == NULL) {
            return;
        }

        VoiceListenerLink *previous = link->previous;
        if (previous == link) {
            listener->field_0x8 = NULL;
        } else {
            VoiceListenerLink *next = link->next;
            previous->next = next;
            next->previous = previous;
            if (listener->field_0x8 == link) {
                listener->field_0x8 = previous;
            }
        }

        link->listener = NULL;
        link->previous = NULL;
        link->next = NULL;
    }

    static inline void AttachVoiceListener(VoiceListenerLink *link, NuSoundListener *listener) {
        DetachVoiceListener(link);

        VoiceListenerLink *head = static_cast<VoiceListenerLink *>(listener->field_0x8);
        if (head == NULL) {
            listener->field_0x8 = link;
            link->previous = link;
            link->next = link;
        } else {
            VoiceListenerLink *previous = head->previous;
            link->previous = previous;
            link->next = head;
            head->previous = link;
            previous->next = link;
        }
        link->listener = listener;
    }
} // namespace

// ---------------------------------------------------------------------------
// construction / destruction
// ---------------------------------------------------------------------------

NuSoundVoice::NuSoundVoice(NuSoundSource *sound_source, bool loop) {
    this->field_0x24 = NULL;
    this->field_0x28 = NULL;
    this->field57_0x80 = NULL;
    this->field58_0x84 = NULL;
    this->field59_0x88 = NULL;
    this->field60_0x8c = NULL;
    this->field61_0x90 = NULL;
    this->field62_0x94 = NULL;
    this->queued_buffers = 0;

    // The source is locked for the lifetime of the voice and keeps the stream
    // open until the voice releases it.
    sound_source->IsStreamOpen();
    sound_source->Lock();
    sound_source->VoiceReference();
    this->sound_source = sound_source;

    memset(this->mix_gains, 0, sizeof(this->mix_gains));

    this->field63_0x98 = 0.0f;
    this->field64_0x9c = 0.0f;
    this->field65_0xa0 = 1.0f; // falloff attenuation
    this->field66_0xa4 = 0.0f;
    this->field67_0xa8 = 1.0f; // final mix scalar
    this->field68_0xac = 1.0f; // pitch scale
    this->pitch = 1.0f;
    this->volume = 1.0f;
    this->falloff_a = 1.0f;
    this->falloff_b = 6.0f;
    this->field69_0xb0 = 20.0f;
    this->field70_0xb4 = 180.0f;
    this->field71_0xb8 = 70.0f;
    this->field72_0xbc = 0.0f;
    this->field73_0xc0 = 0.0f;
    this->field74_0xc4 = 1.0f;
    this->falloff_type = 0;
    this->field113_0x10c = 1.0f; // LFE gain
    this->start_offset = 0.0f;
    this->output_devices = 1;
    this->controller_bits = 0;
    this->output_bus = NuSoundSystem::sMasterBus;
    this->downmixer_type = 0;
    this->routing_table = NuSoundSystem::sDefaultRoutingTable;
    this->surround_mode = 2; // 2D omni
    this->listeners = NULL;
    this->field130_0x144 = 1.0f;
    this->field131_0x148 = -1;
    this->custom_surround_mix = NULL;

    this->flags2 = (u8)(this->flags2 & 0xf6 | loop << 3);
    this->flags2 &= 0xf9;
    this->flags = (u8)(this->flags & 0xf0 | 0x10); // mix update on the first Update

    this->SetState(PLAYSTATE_STOPPED); // libTTapp.so ctor tail (0x3275b9)
}

NuSoundVoice::~NuSoundVoice() {
    // Handles are owned by their callers.  The target invalidates them and
    // then empties the intrusive list without destroying the handle objects.
    for (NuSoundHandle *handle = this->handles.Front(); handle != this->handles.End();
         handle = handle->intrusive_next) {
        handle->SetVoice(NULL);
    }
    while (this->handles.length != 0) {
        this->handles.Remove(this->handles.Front());
    }

    this->sound_source->VoiceRelease();
    this->sound_source->Unlock();

    // The target destructor unlinks both embedded listener memberships before
    // the voice storage is returned to the scratch allocator.
    DetachVoiceListener(reinterpret_cast<VoiceListenerLink *>(&this->field60_0x8c));
    DetachVoiceListener(reinterpret_cast<VoiceListenerLink *>(&this->field57_0x80));
}

// ---------------------------------------------------------------------------
// play state
// ---------------------------------------------------------------------------

NuSoundVoice::PlayState NuSoundVoice::GetState() const {
    NuSoundVoice::PlayState state;

    pthread_mutex_lock(&sStateCriticalSection);
    state = this->state;
    pthread_mutex_unlock(&sStateCriticalSection);
    return state;
}

void NuSoundVoice::SetState(PlayState state) {
    pthread_mutex_lock(&sStateCriticalSection);
    this->state = state;
    pthread_mutex_unlock(&sStateCriticalSection);
}

bool NuSoundVoice::GetAutoDelete() const {
    return (this->flags2 & 1) != 0;
}

void NuSoundVoice::SetAutoDelete(bool auto_delete) {
    this->flags2 = (u8)(this->flags2 & 0xfe | auto_delete);
}

void NuSoundVoice::SetMixUpdate(bool mix_update) {
    this->flags = (u8)(this->flags & 0xef | mix_update << 4);
}

void NuSoundVoice::SetVolume(f32 volume) {
    // The original rejects out-of-range values instead of clamping.
    if (0.0f <= volume && volume <= 1.0f) {
        this->volume = volume;
    }
}

void NuSoundVoice::SetPitch(f32 pitch) {
    if (0.0f <= pitch) {
        this->pitch = pitch;
    }
}

// ---------------------------------------------------------------------------
// play control
// ---------------------------------------------------------------------------

void NuSoundVoice::Play() {
    NuSoundVoice::PlayState state = this->GetState();
    if (state == PLAYSTATE_PLAYING) {
        return;
    }
    state = this->GetState();
    if (state == PLAYSTATE_PAUSED) {
        this->Resume();
        return;
    }

    if (this->queued_buffers == 0) {
        // Ask the source for the initial buffers; the streamer (or the sample
        // itself) hands them back through SubmitBuffer.
        u32 num_buffers = this->sound_source->GetNumInitialBuffers();
        for (u32 i = 0; i < num_buffers; i++) {
            if ((this->flags2 & 8) == 0 && (this->flags2 & 2) != 0) {
                break;
            }
            NuSoundWeakPtr<NuSoundBufferCallback> callback;
            callback.Set(this);
            this->sound_source->RequestBuffer((this->flags2 >> 3) & 1, callback);
        }
    }

    this->Update(0.0f);         // prime the mix and the hardware state
    this->StartHardwareVoice(); // flags the device start (applied in UpdateHardwareVoice)
    this->SetState(PLAYSTATE_PLAYING);
}

void NuSoundVoice::Pause() {
    if (this->GetState() == PLAYSTATE_PLAYING) {
        this->PauseHardwareVoice();
        this->SetState(PLAYSTATE_PAUSED);
        this->flags = (u8)(this->flags & 0xf0 | (this->flags + 1) & 0xf);
    }
}

void NuSoundVoice::Resume() {
    u8 flags = this->flags;
    if ((flags & 0xf) != 0) {
        flags = (u8)(flags & 0xf0 | (flags & 0xf) + 0xf & 0xf);
        this->flags = flags;
    }
    if ((flags & 0xf) != 0) {
        return;
    }

    if (this->GetState() == PLAYSTATE_PAUSED) {
        this->Update(0.0f);
        this->ResumeHardwareVoice();
        this->SetState(PLAYSTATE_PLAYING);
    }

    this->flags &= 0xf0;
}

void NuSoundVoice::Stop(bool with_effects) {
    if (this->GetState() == PLAYSTATE_STOPPED) {
        return;
    }

    if (with_effects) {
        if (this->BeginStopEffects()) {
            // Stop effects were started; the voice stops once they finished.
            if (!this->CheckStopEffects()) {
                return;
            }
        }
    }

    this->StopHardwareVoice();
    this->SetState(PLAYSTATE_STOPPED);
}

// ---------------------------------------------------------------------------
// per-frame processing
// ---------------------------------------------------------------------------

void NuSoundVoice::Update(f32 frametime) {
    if ((this->flags2 & 4) != 0 && this->CheckStopEffects()) {
        this->Stop(true);
    }

    this->UpdateEffects(frametime, NuSoundEffect::EffectProcessStage::ZERO);

    if ((this->flags & 0x10) != 0 || this->GetState() == PLAYSTATE_STOPPED) {
        this->UpdateMix(frametime);
    }

    this->UpdateEffects(frametime, NuSoundEffect::EffectProcessStage::ONE);

    this->ApplyHardwareVoiceMix();
    this->UpdateHardwareVoice(frametime);
}

void NuSoundVoice::UpdateMix(f32 frametime) {
    f32 bus_gains[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    this->CalculatePositionalMix();

    if (this->output_bus != NULL) {
        this->output_bus->ApplyFinalMix(bus_gains);
    }

    f32 attenuation = this->CalculateEffectAttenuation();
    f32 pitch_scale = this->CalculateEffectPitchScale();
    f32 volume = this->volume;

    for (u32 i = 0; i < 8; i++) {
        this->mix_gains[i] *= bus_gains[i] * attenuation * volume;
    }

    this->field67_0xa8 = bus_gains[2] * attenuation * this->field67_0xa8;
    this->field68_0xac = pitch_scale;
}

static inline VuVec CopySoundPosition(const VuVec &position) {
    VuVec result;
    result.x = position.x;
    result.y = position.y;
    result.z = position.z;
    result.w = position.w;
    return result;
}

void NuSoundVoice::CalculatePositionalMix() {
    memset(this->mix_gains, 0, sizeof(this->mix_gains));

    this->field67_0xa8 = 0.0f;
    this->field65_0xa0 = 1.0f;
    this->field63_0x98 = 0.0f;
    this->field64_0x9c = 0.0f;
    this->field66_0xa4 = this->field69_0xb0;

    VoiceListenerLink *real_link = reinterpret_cast<VoiceListenerLink *>(&this->field57_0x80);
    VoiceListenerLink *focus_link = reinterpret_cast<VoiceListenerLink *>(&this->field60_0x8c);
    DetachVoiceListener(real_link);
    DetachVoiceListener(focus_link);

    if (this->surround_mode == 0) {
        NuSoundListener *real_listener =
            NuSoundSystem::GetNearestRealListener(*this->listeners, CopySoundPosition(this->position));
        f32 focus_distance = 0.0f;
        NuSoundListener *focus_listener =
            NuSoundSystem::GetNearestFocusListener(*this->listeners, CopySoundPosition(this->position), focus_distance);

        if (real_listener == NULL || focus_listener->GetSensitivity() <= 0.0f || this->falloff_b <= focus_distance) {
            return;
        }

        f32 attenuation = this->CalculateFalloffAttenuation(focus_distance);
        if (attenuation <= 0.0f) {
            return;
        }

        f32 real_distance = real_listener->GetHeadDistance(CopySoundPosition(this->position));
        NuSoundListener *second_real_listener = NULL;
        f32 second_real_distance = 0.0f;
        NuSoundListener *listener = static_cast<NuSoundListener *>(this->listeners->begin->field_0x4);
        while (listener != this->listeners->end) {
            if (listener != real_listener && listener->IsEnabled()) {
                f32 distance = listener->GetHeadDistance(CopySoundPosition(this->position));
                if (distance - real_distance < 6.0f) {
                    second_real_listener = listener;
                    second_real_distance = distance;
                    break;
                }
            }
            listener = static_cast<NuSoundListener *>(listener->field_0x4);
        }

        f32 field_angle = 0.0f;
        f32 first_mix[8] = {};
        f32 second_mix[8] = {};
        if (real_listener->Get2DScreenPosition() != NULL) {
            NUMTX identity;
            NuMtxSetIdentity(&identity);
            VuVec screen_position(real_listener->Get2DScreenPosition()->x, 0.0f,
                                  1.0f - real_listener->Get2DScreenPosition()->y, 1.0f);
            NUMTX copied_identity = identity;
            f32 outer_angle = this->field69_0xb0 + this->field71_0xb8;
            if (!(outer_angle < 360.0f)) {
                outer_angle = 360.0f;
            }
            this->CalculatePositionalCoefficients(this->mix_gains, screen_position,
                                                  *reinterpret_cast<VuMtx *>(&copied_identity), this->field69_0xb0,
                                                  outer_angle);
            second_real_listener = NULL;
        } else {
            field_angle = this->CalculateFieldAngle(real_distance);
            f32 outer_angle = field_angle + this->field71_0xb8;
            if (!(outer_angle < 360.0f)) {
                outer_angle = 360.0f;
            }
            this->CalculatePositionalCoefficients(first_mix, CopySoundPosition(this->position),
                                                  *real_listener->GetHeadMatrix(), field_angle, outer_angle);

            if (second_real_listener == NULL) {
                memmove(this->mix_gains, first_mix, sizeof(first_mix));
            } else {
                f32 second_field_angle = this->CalculateFieldAngle(second_real_distance);
                f32 second_outer_angle = field_angle + this->field71_0xb8;
                if (!(second_outer_angle < 360.0f)) {
                    second_outer_angle = 360.0f;
                }
                this->CalculatePositionalCoefficients(second_mix, CopySoundPosition(this->position),
                                                      *second_real_listener->GetHeadMatrix(), second_field_angle,
                                                      second_outer_angle);
                for (u32 i = 0; i < 8; ++i) {
                    this->mix_gains[i] = first_mix[i] > second_mix[i] ? first_mix[i] : second_mix[i];
                }
            }
        }

        for (u32 i = 0; i < 8; ++i) {
            this->mix_gains[i] *= attenuation * focus_listener->GetSensitivity();
        }
        if (NuSoundSystem::GetOutputChannelConfig() > 5) {
            this->mix_gains[3] = attenuation * this->field113_0x10c * focus_listener->GetSensitivity();
        }
        this->field67_0xa8 = attenuation * focus_listener->GetSensitivity();

        AttachVoiceListener(real_link, real_listener);
        if (focus_listener != NULL) {
            AttachVoiceListener(focus_link, focus_listener);
        }

        this->field65_0xa0 = attenuation;
        this->field63_0x98 = real_distance;
        this->field64_0x9c = focus_distance;
        this->field66_0xa4 = field_angle;
        return;
    }

    if (this->surround_mode == 1) {
        f32 focus_distance = 0.0f;
        NuSoundListener *focus_listener =
            NuSoundSystem::GetNearestFocusListener(*this->listeners, CopySoundPosition(this->position), focus_distance);
        if (focus_listener == NULL || this->falloff_b <= focus_distance) {
            return;
        }

        NuSoundListener *real_listener =
            NuSoundSystem::GetNearestRealListener(*this->listeners, CopySoundPosition(this->position));
        if (real_listener == NULL) {
            return;
        }

        const NUMTX *head_matrix = reinterpret_cast<const NUMTX *>(real_listener->GetHeadMatrix());
        VuVec relative_direction(head_matrix->m30 - this->direction.x, head_matrix->m31 - this->direction.y,
                                 head_matrix->m32 - this->direction.z, 0.0f);
        f32 coefficients[8] = {};
        f32 outer_angle = this->field69_0xb0 + this->field71_0xb8;
        if (!(outer_angle < 360.0f)) {
            outer_angle = 360.0f;
        }
        this->CalculatePositionalCoefficients(coefficients, relative_direction, *real_listener->GetHeadMatrix(),
                                              this->field69_0xb0, outer_angle);

        f32 attenuation = this->CalculateFalloffAttenuation(focus_distance);
        for (u32 i = 0; i < 8; ++i) {
            this->mix_gains[i] = coefficients[i] * attenuation * focus_listener->GetSensitivity();
        }
        if (NuSoundSystem::GetOutputChannelConfig() > 5) {
            this->mix_gains[3] = attenuation * this->field113_0x10c * focus_listener->GetSensitivity();
        }
        this->field67_0xa8 = attenuation * focus_listener->GetSensitivity();

        AttachVoiceListener(real_link, real_listener);
        AttachVoiceListener(focus_link, focus_listener);
        this->field65_0xa0 = attenuation;
        this->field64_0x9c = focus_distance;
        return;
    }

    if (this->surround_mode == 2) {
        this->mix_gains[0] = 1.0f;
        this->mix_gains[1] = 1.0f;
        this->mix_gains[2] = 1.0f;
        this->mix_gains[4] = 1.0f;
        this->mix_gains[5] = 1.0f;
        this->mix_gains[6] = 1.0f;
        this->mix_gains[7] = 1.0f;
        this->field67_0xa8 = 1.0f;
        this->mix_gains[3] = this->field113_0x10c;
        return;
    }

    if (this->surround_mode == 3) {
        f32 focus_distance = 0.0f;
        NuSoundListener *focus_listener =
            NuSoundSystem::GetNearestFocusListener(*this->listeners, CopySoundPosition(this->position), focus_distance);
        if (focus_listener == NULL || this->falloff_b <= focus_distance) {
            return;
        }

        f32 attenuation = this->CalculateFalloffAttenuation(focus_distance);
        this->field67_0xa8 = attenuation;
        for (u32 i = 0; i < 8; ++i) {
            this->mix_gains[i] = attenuation * focus_listener->GetSensitivity();
        }
        this->mix_gains[3] = attenuation * this->field113_0x10c * focus_listener->GetSensitivity();
        this->field67_0xa8 = attenuation * focus_listener->GetSensitivity();

        AttachVoiceListener(focus_link, focus_listener);
        this->field65_0xa0 = attenuation;
        this->field64_0x9c = focus_distance;
        return;
    }

    if (this->surround_mode == 4) {
        memmove(this->mix_gains, this->custom_surround_mix, sizeof(this->mix_gains));
    }
}

bool NuSoundVoice::AreStopEffectsRunning() const {
    if ((this->flags2 & 4) != 0) {
        for (NuListNodeBase *node = this->effects.Head(); node != this->effects.Tail(); node = node->GetNext()) {
            NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
            if (effect->stop_effect == 1 && effect->state == 1) {
                return true;
            }
        }
    }
    return false;
}

bool NuSoundVoice::CheckStopEffects() {
    if ((this->flags2 & 4) != 0) {
        for (NuListNodeBase *node = this->effects.Head(); node != this->effects.Tail(); node = node->GetNext()) {
            NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
            u32 effect_type = *reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(effect) + 0xc);
            u32 effect_state = *reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(effect) + 0x10);
            if (effect_type == 1 && effect_state == 1) {
                return false;
            }
        }
    }
    return true;
}

void NuSoundVoice::UpdateEffects(f32 frametime, NuSoundEffect::EffectProcessStage stage) {
    NuListNodeBase *node = this->effects.Head();
    while (node != this->effects.Tail()) {
        NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
        node = node->GetNext();

        if (effect->process_stage == stage && effect->enabled) {
            effect->ProcessVoice(this, frametime);
        }

        if (effect->state == 2 && !effect->keep_attached) {
            this->RemoveEffect(effect);
        }
    }
}

void NuSoundVoice::CheckStarvedBuffers() {
}

void NuSoundVoice::UpdateHardwareVoice(f32) {
}

// ---------------------------------------------------------------------------
// remaining original surface (off the title music path; kept as stubs)
// ---------------------------------------------------------------------------

bool NuSoundVoice::AddEffect(NuSoundEffect *effect) {
    if (effects.Length() != 0) {
        NuListNodeBase *node = effects.Head();
        NuListNodeBase *last = effects.Tail()->GetPrev();
        if (static_cast<NuListNode<NuSoundEffect *> *>(node)->value == effect) {
            return false;
        }
        while (node != last) {
            node = node->GetNext();
            if (static_cast<NuListNode<NuSoundEffect *> *>(node)->value == effect) {
                return false;
            }
        }
    }
    bool attached = effect->AttachVoice(this);
    if (!attached) {
        return false;
    }
    NuSoundMemory::PushNuListNode(effects, effect);
    return attached;
}

bool NuSoundVoice::BeginStopEffects() {
    if ((this->flags2 & 4) == 0) {
        for (NuListNodeBase *node = this->effects.Head(); node != this->effects.Tail(); node = node->GetNext()) {
            NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
            if (effect->stop_effect == 1) {
                effect->Enable();
                this->flags2 |= 4;
            }
        }
    }
    return (this->flags2 & 4) != 0;
}

f32 NuSoundVoice::CalculateEffectAttenuation() {
    f32 attenuation = 1.0f;
    for (NuListNodeBase *node = this->effects.Head(); node != this->effects.Tail(); node = node->GetNext()) {
        NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
        attenuation *= effect->output_mix;
    }
    return attenuation;
}

f32 NuSoundVoice::CalculateEffectPitchScale() {
    f32 scale = 1.0f;
    for (NuListNodeBase *node = this->effects.Head(); node != this->effects.Tail(); node = node->GetNext()) {
        NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
        scale *= effect->pitch_mix;
    }
    return scale;
}

f32 NuSoundVoice::CalculateFalloffAttenuation(f32 distance) {
    if (distance > this->falloff_a) {
        if (this->falloff_type == 0) {
            return (this->falloff_b - distance) / (this->falloff_b - this->falloff_a);
        }
        if (this->falloff_type == 1) {
            f32 ratio = (this->falloff_b - distance) / (this->falloff_b - this->falloff_a);
            f32 scale = (1.0f - ratio) * 10.0f + 1.0f;
            return 1.0f / (scale * scale);
        }
    }
    return 1.0f;
}

f32 NuSoundVoice::CalculateFieldAngle(f32 distance) {
    f32 result = this->field69_0xb0;
    if (this->field73_0xc0 > distance) {
        if (this->field72_0xbc > distance) {
            return this->field70_0xb4;
        }
        result += ((this->field73_0xc0 - distance) / (this->field73_0xc0 - this->field72_0xbc)) *
                  (this->field70_0xb4 - result);
    }
    return result;
}

// Literal speaker sites preserve the original per-channel branch ordering.
#define NUSOUND_SPEAKER_COEFFICIENT(Index, Angle)                                                                      \
    {                                                                                                                  \
        if (gains[(Index)] == 0.0f && (Angle) > outer_start && (Angle) < outer_end) {                                  \
            if ((Angle) > inner_start && (Angle) < inner_end) {                                                        \
                gains[(Index)] = 1.0f;                                                                                 \
            } else {                                                                                                   \
                f32 outer = (Angle) - bearing >= 0.0f ? outer_end : outer_start;                                       \
                f32 inner = (Angle) - bearing >= 0.0f ? inner_end : inner_start;                                       \
                f32 value = NuFabs((outer - (Angle)) / (outer - inner));                                               \
                gains[(Index)] = value < 1.0f ? value : 1.0f;                                                          \
            }                                                                                                          \
        }                                                                                                              \
    }

void NuSoundVoice::CalculatePositionalCoefficients(f32 *gains, VuVec const &position, VuMtx const &mtx,
                                                   f32 speaker_field_angle_min, f32 speaker_field_angle_max) {
    NUVEC listener_space;
    NuVecInvMtxTransform(&listener_space, reinterpret_cast<NUVEC *>(const_cast<VuVec *>(&position)),
                         reinterpret_cast<NUMTX *>(const_cast<VuMtx *>(&mtx)));

    f32 bearing = NuFmod(NuATan2f(listener_space.x, listener_space.z) * 180.0f / 3.1415927f, 360.0f);
    speaker_field_angle_max *= 0.5f;
    f32 outer_start = NuFmod(bearing - speaker_field_angle_max, 360.0f);
    f32 outer_end = NuFmod(bearing + speaker_field_angle_max, 360.0f);
    speaker_field_angle_min *= 0.5f;
    f32 inner_start = NuFmod(bearing - speaker_field_angle_min, 360.0f);
    f32 inner_end = NuFmod(bearing + speaker_field_angle_min, 360.0f);

    NUSOUND_SPEAKER_COEFFICIENT(2, -360.0f);
    NUSOUND_SPEAKER_COEFFICIENT(1, -330.0f);
    NUSOUND_SPEAKER_COEFFICIENT(5, -270.0f);
    NUSOUND_SPEAKER_COEFFICIENT(7, -210.0f);
    NUSOUND_SPEAKER_COEFFICIENT(6, -150.0f);
    NUSOUND_SPEAKER_COEFFICIENT(4, -90.0f);
    NUSOUND_SPEAKER_COEFFICIENT(0, -30.0f);

    NUSOUND_SPEAKER_COEFFICIENT(2, 0.0f);
    NUSOUND_SPEAKER_COEFFICIENT(1, 30.0f);
    NUSOUND_SPEAKER_COEFFICIENT(5, 90.0f);
    NUSOUND_SPEAKER_COEFFICIENT(7, 150.0f);
    NUSOUND_SPEAKER_COEFFICIENT(6, 210.0f);
    NUSOUND_SPEAKER_COEFFICIENT(4, 270.0f);
    NUSOUND_SPEAKER_COEFFICIENT(0, 330.0f);
    NUSOUND_SPEAKER_COEFFICIENT(2, 360.0f);
}

#undef NUSOUND_SPEAKER_COEFFICIENT

u8 NuSoundVoice::GetControllerBits() const {
    return controller_bits;
}

const VuVec *NuSoundVoice::GetDirection() const {
    return &direction;
}

NuSoundSystem::DownmixType NuSoundVoice::GetDownmixerType() const {
    return static_cast<NuSoundSystem::DownmixType>(downmixer_type);
}

NuSoundEffect *NuSoundVoice::GetEffect(NuSoundEffect::EffectType type) {
    NuListNodeBase *node = effects.Head();
    NuListNodeBase *end = effects.Tail();
    for (; node != end; node = node->GetNext()) {
        NuSoundEffect *effect = static_cast<NuListNode<NuSoundEffect *> *>(node)->value;
        if (effect->type == type) {
            return effect;
        }
    }
    return NULL;
}

NuSoundSystem::FalloffType NuSoundVoice::GetFalloffType() const {
    return static_cast<NuSoundSystem::FalloffType>(falloff_type);
}

f32 NuSoundVoice::GetFar() const {
    return falloff_b;
}

f32 NuSoundVoice::GetLowFrequencyMix() const {
    return field113_0x10c;
}

f32 NuSoundVoice::GetNear() const {
    return falloff_a;
}

i32 NuSoundVoice::GetNumEffects() const {
    return effects.Length();
}

NuSoundBus *NuSoundVoice::GetOutputBus() const {
    return output_bus;
}

f32 NuSoundVoice::GetPenetration() const {
    return field74_0xc4;
}

f32 NuSoundVoice::GetPitch() const {
    return pitch;
}

f32 NuSoundVoice::GetPlaybackPositionSeconds() {
    u64 position = GetPlaybackPositionSamples();
    u32 sample_rate = sound_source->GetStreamDesc()->GetSampleRate();
    return static_cast<f32>(position) / static_cast<f32>(sample_rate);
}

const VuVec *NuSoundVoice::GetPosition() const {
    return &position;
}

f32 NuSoundVoice::GetReverbWetMix() const {
    return field130_0x144;
}

NuSoundRoutingTable *NuSoundVoice::GetRoutingTable() const {
    return routing_table;
}

f32 NuSoundVoice::GetSpeakerBleedAngle() const {
    return field71_0xb8;
}

f32 NuSoundVoice::GetSpeakerBleedFar() const {
    return field73_0xc0;
}

f32 NuSoundVoice::GetSpeakerBleedNear() const {
    return field72_0xbc;
}

f32 NuSoundVoice::GetSpeakerFieldAngleMax() const {
    return field70_0xb4;
}

f32 NuSoundVoice::GetSpeakerFieldAngleMin() const {
    return field69_0xb0;
}

f32 NuSoundVoice::GetStartOffset() const {
    return start_offset;
}

NuSoundSystem::SurroundMode NuSoundVoice::GetSurroundMode() const {
    return static_cast<NuSoundSystem::SurroundMode>(surround_mode);
}

const VuVec *NuSoundVoice::GetVelocity() const {
    return &velocity;
}

f32 NuSoundVoice::GetVolume() const {
    return volume;
}

bool NuSoundVoice::IsLooping() const {
    return (flags2 & 8) != 0;
}

void NuSoundVoice::RegisterHandle(NuSoundHandle *handle) {
    if (handle != NULL) {
        handle->InvalidateVoice();
        handle->SetVoice(this);
        handles.PushBack(handle);
    }
}

void NuSoundVoice::RemoveEffect(NuSoundEffect *effect) {
    if (!effects.Contains(effect)) {
        return;
    }
    effect->DetachVoice(this);
    if (effects.Length() != 0) {
        effects.RemoveValue(effect);
    }
    NuSoundSystem::sAllocdMemory[static_cast<i32>(NuSoundSystem::MemoryDiscipline::SCRATCH)] -=
        sizeof(NuListNode<NuSoundEffect *>);
}

void NuSoundVoice::SetControllerBits(i32 bits) {
    this->controller_bits = (u8)bits;
}

void NuSoundVoice::SetCustomSurroundMix(f32 *mix) {
    this->custom_surround_mix = mix;
}

void NuSoundVoice::SetDirection(VuVec *value) {
    if (value != NULL) {
        this->direction.x = value->x;
        this->direction.y = value->y;
        this->direction.z = value->z;
        this->direction.w = value->w;
        NuVecNorm(reinterpret_cast<NUVEC *>(&this->direction), reinterpret_cast<NUVEC *>(&this->direction));
    }
}

void NuSoundVoice::SetDownmixerType(NuSoundSystem::DownmixType type) {
    this->downmixer_type = (u32)type;
}

void NuSoundVoice::SetFalloff(f32 near_distance, f32 far_distance, NuSoundSystem::FalloffType type) {
    if (near_distance >= 0.0f && far_distance > near_distance) {
        this->falloff_a = near_distance;
        this->falloff_b = far_distance;
        this->falloff_type = (u32)type;
    }
}

void NuSoundVoice::SetLowFrequencyMix(f32 mix) {
    this->field113_0x10c = mix;
}

void NuSoundVoice::SetOutputBus(NuSoundBus *bus) {
    this->output_bus = bus != NULL ? bus : NuSoundSystem::sMasterBus;
}

void NuSoundVoice::SetOutputDevices(i32 devices) {
    this->output_devices = (u32)devices;
}

void NuSoundVoice::SetPenetration(f32 penetration) {
    this->field74_0xc4 = penetration;
}

void NuSoundVoice::SetPosition(VuVec *value) {
    if (value != NULL) {
        memcpy(&this->position, value, sizeof(this->position));
    }
}

void NuSoundVoice::SetReverbWetMix(f32 mix) {
    this->field130_0x144 = mix;
}

void NuSoundVoice::SetRoutingTable(NuSoundRoutingTable *table) {
    this->routing_table = table;
}

void NuSoundVoice::SetSpeakerBleedAngle(f32 angle) {
    this->field71_0xb8 = angle;
}

void NuSoundVoice::SetSpeakerBleedFar(f32 distance) {
    this->field73_0xc0 = distance;
}

void NuSoundVoice::SetSpeakerBleedNear(f32 distance) {
    this->field72_0xbc = distance;
}

void NuSoundVoice::SetSpeakerFieldAngle(f32 minimum, f32 maximum) {
    this->field69_0xb0 = minimum;
    this->field70_0xb4 = maximum;
}

void NuSoundVoice::SetStartOffset(f32 offset) {
    this->start_offset = offset;
}

void NuSoundVoice::SetListeners(NuEList<NuSoundListener, DefaultElist> const *value) {
    this->listeners = value;
}

void NuSoundVoice::SetSurroundMode(NuSoundSystem::SurroundMode mode) {
    this->surround_mode = (u32)mode;
}

void NuSoundVoice::SetVelocity(VuVec const &value) {
    this->velocity = value;
}

void NuSoundVoice::UnregisterHandle(NuSoundHandle *handle) {
    handle->SetVoice(NULL);
    handles.Remove(handle);
}
