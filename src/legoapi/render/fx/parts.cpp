#include "legoapi/audio/sfx.h"
#include "legoapi/render/fx/parts.h"
#include "legoapi/legoapi_types.h"
#include "globals.h"
#include "gamelib/util/gamelib_util_types.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/render/fx.h"
#include "legoapi/render/core/rtl.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/gizmos/traps/giztorpmachine.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "legoapi/world/area.h"
#include "legoapi/characters/core/players.h"
#include "nu2api/nu3d/nuportal.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/core/input/gamepads.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nurndrstat.h"
#include "nu2api/nucore/nustring.h"

#include <float.h>
#include <math.h>
#include <string.h>

ADDPART_s Default_ADDPART = {NULL,
                             0,
                             NULL,
                             0,
                             0,
                             0.1f,
                             0.1f,
                             -5.0f,
                             0.75f,
                             NULL,
                             -1,
                             0,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             -1,
                             NULL,
                             -1,
                             -1,
                             60.0f,
                             60.0f,
                             -1,
                             -1,
                             -1,
                             1.0f,
                             0,
                             -1,
                             2000000.0f,
                             1.0f / 60.0f,
                             -1,
                             0,
                             1.0f,
                             1.0f,
                             1.0f,
                             0.0f,
                             {0, 0, 0, 0, 0, 0},
                             1.0f,
                             0,
                             {0, 0, 0}};

struct rtlset;
extern rtlset *PartRTL;
f32 PARTSCALEUPTIME = 0.5f;
void NewPartOrderedRotation(PART_s *);
void PartTimeSlip();
void PartCleanupTypes();
void UpdatePartEmits(f32);
static i32 part_raycasts_enabled = 1;
static NUVEC PartNorm;
extern u8 object_switches[0x80];

extern "C" {
    void DebFreeInstantly(i32 *handle);
    // Shared suspend flag consulted by all timed debris emitters.
    i32 debris_suspended = 0;

    extern debkeydatatype_s *debkeydata;
    extern debinftype **debtab;
    extern i32 EDPP_MAX_TYPES;
    extern DEBRISGENERATOR gensorttab[13];
    extern DEBRISMOMENTUMADJUSTER gencodetab[7];
    extern f32 globaltime;
    extern f32 partglobaltime;
    extern f32 panelglobaltime;
    extern f32 timeincrement;
    f32 CameraEmitterDistance(NUVEC *);
    void SetSfxBit_On(i32);
    void PlaySfxByIdEx(i32, NUVEC *, f32, f32);
    void PlaySfxById(i32, NUVEC *);
    void DebrisEmitterMomentum(i32, f32, f32, f32);
    void DebrisParticleMomentum(i32, f32, f32, f32);
    void AddFiniteShotDebrisEffect2(i32 *, i32, NUVEC *, NUVEC *, NUVEC *, i32);
    void AddVariableShotDebrisEffectMtx3(i32, NUVEC *, NUVEC *, i32, NUMTX *, NUMTX *);
    void AddVariableShotDebrisEffectMtx4(i32, NUVEC *, NUVEC *, i32, NUMTX *, NUMTX *, i16, u8);
    extern i32 debris_suspended;
    extern f32 debris_thinning_level;
    extern i32 forced_debris_thinning;
    extern i32 freedebkeyptr;
    extern i32 maxdebkeys;
    extern i32 debrischunks;
    extern i32 debrischunksglass;
    extern i32 freedebchkptr;
    extern i32 freedebchkptrg;
    extern dma_particle_chunk_s **freedebchunks;
    extern dma_particle_chunk_s **freedebchunksglass;
    extern particlechunkrendertype_s *ParticleChunkToRender;
    extern particlechunkrendertype_s *ParticleChunkRenderStack[5];
    extern debris_chunk_control_s **freechunkcontrols;
    extern i32 freechunkcontrolsptr;
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
    extern edpp_particle_s edpp_ptls[512];
    extern i32 edpp_page_used[8];
    extern i32 edpp_page_on[8];
    extern i32 edpp_instances_used;
    extern u32 partseed;

    void NuPartEnableRayCasts(i32 enabled) {
        part_raycasts_enabled = enabled;
    }

    u32 NuPartGetSeed(void) {
        return partseed;
    }

    void NuPartSetSeed(i32 seed) {
        partseed = static_cast<u32>(seed);
    }

    extern part_type_s part_types[128];
    extern part_emit_s part_emits[512];
    extern i32 part_emits_used;
    void CheckPartCount();
    i32 AddPARTEffect(i32, NUVEC *);
    void PARTStartOffset(i32, f32);

    void ResetParts(void);

    i32 DebAlloc(void);
    void DebrisStartOffset(i32, f32);
    void DebrisEmitterPos(i32, f32, f32, f32);
    void DebrisEmitterOrientation(i32, i16, i16, i16);
    void DebrisOrientation(i32, i16, i16);
    void DebrisOrientationMtx(i32, NUMTX *);
    void DebrisReflectionOrientation(i32, i16, i16, f32, f32);
    void DebrisSetTrigger(i32, i16, i16, i16);
    void DebrisEmitterOrientationMtx(i32, NUMTX *);
    i32 CreateScaledEffect(i32, f32);
    i32 NuCameraClipTestExtentsAxisAligned(NUVEC *, NUVEC *, f32);
    void NuVecAddScale(NUVEC *, NUVEC *, NUVEC *, f32);
    void LinkDmaParticalSets(dma_particle_chunk_s **, i32);
    void AddVariableShotDebrisEffectTimed3(i32, NUVEC *, NUVEC *, i32, f32, NUMTX *, NUMTX *);
    void AddVariableShotDebrisEffectTimed5(i32, NUVEC *, NUVEC *, NUVEC *, i32, f32, NUMTX *, NUMTX *, i16, u8);
}

void AddDebrisEffectToStack(debkeydatatype_s *);
void AddChunkToRenderStack(particlechunkrendertype_s *, particlechunkrendertype_s **);
void AddChunkControlToStack(debris_chunk_control_s *, debris_chunk_control_s **);
void RemoveChunkControlFromStack(debris_chunk_control_s *, debris_chunk_control_s **);
void DebrisProcessSpheres(uv1deb *, f32, debinftype *, debkeydatatype_s *, i32);
void DebrisGetControlStackLock();
void DebrisReleaseControlStackLock();
void FindAnglesZX(NUVEC *, u16 *, u16 *);
extern f32 AreaPickupGravity;
extern i16 temp_xrot;
extern i16 temp_zrot;

// Forward declarations for local (static) part/gizmo helper stubs.
struct CUSTOMPIECEANIM;
struct spacelevel_s;
struct quickboltinfo;

void PART_s::ClearMechObjectInterface() {
    delete mech_object_interface;
}

MechObjectInterface *PART_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL) {
        new PartObjectInterface(*this);
    }
    return mech_object_interface;
}

// Local (static) part (PART_s), pickup (GIZMOPICKUP_s), gizmo flow
// (GIZFLOW_s/FLOWBOX_s), power-up and space/starfighter helpers. Stubbed as
// local `t` symbols matching res/libTTapp.so.

extern GameObject_s *Obj;
extern i32 HIGHGAMEOBJECT, VehicleArea;
extern WORLDINFO_s *WORLD;
extern GAMECAMERA_s *GameCam;
extern f32 ForceThrowSpeed, ForceThrowGravity;
extern "C" void KillPart(PART_s *, i32);
i32 SphereSphereOverlapScaleY(NUVEC *, f32, f32, NUVEC *, f32, f32);
void DeflectPart(PART_s *, GameObject_s *, f32, f32, i32, i32);
i32 getMaxTorpedos(GameObject_s *);
void SetCoinType(i32 model, GIZMOPICKUP_s *pickup) {
    if (static_cast<u32>(model - 0xb7) <= 3) {
        pickup->type_index = 0;
        pickup->model_variant = model - 0xb7;
    } else if (static_cast<u32>(model - 0xbf) <= 3) {
        pickup->type_index = 1;
        pickup->model_variant = model - 0xbf;
    } else if (static_cast<u32>(model - 0xc7) <= 3) {
        pickup->type_index = 2;
        pickup->model_variant = model - 0xc7;
    } else {
        pickup->type_index = 3;
        pickup->model_variant = model - 0xd5;
    }
    u8 count = GizmoPickupType[pickup->type_index].random_model_count;
    pickup->model_variant = count ? pickup->model_variant % count : 0;
}
void GizmoPickup_CollectCoin(WORLDINFO_s *, NUVEC *, i32, i32, GameObject_s *, i32);
void CollectPowerUp(GameObject_s *, NUVEC *, u16, i32);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
void NewBuzz(nupad_s *, f32, i32);
void CollectHitPoint(GameObject_s *object, NUVEC *position, i32) {
    if (static_cast<i8>(object->current_hp) > 0 && static_cast<i8>(object->current_hp) < object->hitpoints) {
        object->current_hp += TouchHacks::TouchControlsActive ? 2 : 1;
        if (static_cast<i8>(object->current_hp) > object->hitpoints)
            object->current_hp = object->hitpoints;
    }
    if (object->field_0xe38 < 4) {
        object->field_0xe38 += TouchHacks::TouchControlsActive ? 2 : 1;
        if (object->field_0xe38 == 5)
            object->field_0xe38 = 4;
    }
    if (object->apiobj.character_data->model_flags & 0x2000) {
        NUVEC momentum;
        NuVecScale(&momentum, &object->apiobj.velocity, 0.5f);
        AddGameDebrisMom(WORLD->debris_sys, 0x10, position, 15, &momentum);
    } else {
        AddGameDebris(WORLD->debris_sys, 0x0f, position);
    }
    GameAudio_PlaySfx(0x27, position, 0, 0);
    NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
}
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void ReleaseBuildIt(GameObject_s *, i32);
void ReleasePush(GameObject_s *);
void ObjHitShield(GameObject_s *, GameObject_s *, i32, BOLT_s *);
i32 CannotKill(GameObject_s *);
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
void NewRumble(nupad_s *, f32, i32);

static void PartCollide(PART_s *part, i32 three_dimensional) {
    const NUVEC minimum = {part->position.x - part->field_0e4, part->position.y - part->field_0e4,
                           part->position.z - part->field_0e4};
    const NUVEC maximum = {part->position.x + part->field_0e4, part->position.y + part->field_0e4,
                           part->position.z + part->field_0e4};
    GameObject_s *object = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        APIOBJECT_s *api = &object->apiobj;
        if ((api->field_0x1f8 & 0x1001) != 0x1001 || api->field_0x287 != 0)
            continue;
        i8 context = static_cast<i8>(object->character_context);
        if ((CInfo[context].flags & 0x8000) != 0 || (object->field_0xe20 & 0x20) != 0)
            continue;
        if (part->force_player_mask != 0) {
            if ((part->flags & 0x8000) == 0 && part->owner == object)
                continue;
        } else if (part->owner == object) {
            if (0.5f > part->scale_time)
                continue;
        } else if (api->field_0x27c != -1 && 0.25f > part->scale_time)
            continue;
        if (context == 0x39 || context == 0x3b || context == 0x3c)
            continue;
        if ((part->flags & 0x4000) != 0 && part->field_207 == api->field_0x289)
            continue;
        if ((part->flags & 4) != 0 && (api->flags_low & 0x80) == 0)
            continue;
        if (minimum.x > api->collision_max.x || api->collision_min.x > maximum.x || minimum.z > api->collision_max.z ||
            api->collision_min.z > maximum.z)
            continue;
        if (three_dimensional != 0 &&
            !((api->character_data->model_flags & 0x2000) != 0 && (part->flags & 0x40) != 0)) {
            if (minimum.y > api->collision_max.y || api->collision_min.y > maximum.y)
                continue;
        }
        if ((part->flags & 0x40) != 0) {
            if ((part->active & 2) == 0) {
                if (part->pickup_type == 0xcb) {
                    if (!(part->scale_time >= 0.5f))
                        continue;
                    if (VehicleArea == 0 && (static_cast<i8>(object->current_hp) <= 0 ||
                                             static_cast<i8>(object->current_hp) >= object->hitpoints))
                        continue;
                } else if (!(part->scale_time >= 0.1f))
                    continue;
            }
            if ((api->field_0x1f4 & 0x40000) != 0) {
                KillPart(part, 2);
                continue;
            }
            if (part->pickup_type == 0xcb) {
                CollectHitPoint(object, &part->position, 1);
                KillPart(part, 2);
            } else if (part->pickup_type == 0xd0) {
                CollectPowerUp(object, &part->position, part->rotation_y, 1);
                KillPart(part, 2);
            } else if (part->force_player_mask == 3) {
                if ((api->character_data->model_flags & 0x2000) == 0 || object->torpedo == NULL)
                    continue;
                if (object->torpedo->count < getMaxTorpedos(object)) {
                    TORPEDOPACKET_s *packet = object->torpedo;
                    packet->pickup_positions[packet->count] = part->position;
                    packet->pickup_data[packet->count] = part->field_10c[0];
                    packet->pickup_flags[packet->count] = part->field_10c[1];
                    ++packet->count;
                    object->torpedo->field_08 = 0.0f;
                    object->torpedo->field_03 = 0;
                    KillPart(part, 2);
                }
            } else {
                GIZMOPICKUP_s pickup;
                SetCoinType(part->pickup_type, &pickup);
                GizmoPickup_CollectCoin(WORLD, &part->position, pickup.type_index, pickup.model_variant, object, 1);
                KillPart(part, 2);
            }
            continue;
        }
        GAMECHARACTERDATA_s *character = static_cast<GAMECHARACTERDATA_s *>(api->character_data->field11_0x24);
        if ((character->flags_090 & 0x8000) != 0)
            continue;
        if (!SphereSphereOverlapScaleY(&part->position, part->field_0e4, part->field_0e4, &api->collision_position,
                                       api->collision_radius, api->field_0x1e0))
            continue;
        if (!((part->active & 2) != 0 && (part->flags & 0x1000008) == 0x1000008))
            KillPart(part, 1);
        if ((part->flags & 8) != 0 && !((part->flags & 0x1000000) != 0 && (part->active & 2) != 0) &&
            ((api->flags_low & 0x80) != 0 || (part->flags & 0x800000) == 0)) {
            u32 flags = CInfo[static_cast<i8>(object->character_context)].flags;
            if ((flags & 0x4000000) != 0 || ((flags & 0x8000000) != 0 && (object->jump_flags & 2) != 0)) {
                i32 effect = -1;
                if (object->id == id_BOB)
                    effect = (object->field_0xefd & 2) != 0 ? 2 : 3;
                else if (object->blade_index != -1)
                    effect = BladeTab[object->blade_index].hit_effect;
                DeflectPart(part, object, ForceThrowSpeed, ForceThrowGravity, 0, effect);
            } else {
                GameCam_Judder(GameCam, 0.2f, 0, NULL);
                ReleaseBuildIt(object, 0);
                ReleasePush(object);
                if (!(object->field_0xd24 < 1.0f))
                    ObjHitShield(part->owner, object, object->field_0xe37, NULL);
                else if (!CannotKill(object)) {
                    ObjHitObj((part->flags & 0x10000) != 0 ? NULL : part->owner, object, part->field_204,
                              static_cast<u16>(part->force_flags), 0, 1);
                } else if (part->owner != NULL) {
                    NewRumble(part->owner->pad_gamepad->pad, 0.5f, 0);
                    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
                }
            }
        }
        if (api->field_0x287 != 0 || (api->flags_low & 2) != 0)
            continue;
        if ((part->active & 1) == 0) {
            f32 speed = static_cast<GAMECHARACTERDATA_s *>(api->character_data->field11_0x24)->run_speed;
            if (api->scaled_radius > 1.0f)
                speed /= api->scaled_radius;
            NUVEC direction;
            NuVecNorm(&direction, &part->velocity);
            api->velocity.x += direction.x * speed;
            api->velocity.z += direction.z * speed;
        }
        if ((part->flags & 0x2000000) != 0 && ((part->flags & 0x4000000) == 0 || (part->active & 2) != 0)) {
            u16 angle = NuAtan2D(api->position.x - part->position.x, api->position.z - part->position.z);
            f32 speed = static_cast<GAMECHARACTERDATA_s *>(api->character_data->field11_0x24)->run_speed;
            api->field_0x1fc = NU_SIN_LUT(angle) * speed;
            api->field_0x204 = NU_COS_LUT(angle) * speed;
        }
    }
}

static __used__ void TiePart_Kill(PART_s *, i32) {
}

static __used__ void TiePart_Move(PART_s *, f32) {
}

static __used__ void TiePart_Impact(PART_s *) {
}

static __used__ void TiePart_KillExplode(PART_s *, i32) {
}

static __used__ void TieSpinZPart_Move(PART_s *, f32) {
}

extern f32 coinimpactwait;
extern i32 GAMEDEMO;
extern "C" void PlaySfxAndSetVolume(char *, NUVEC *, f32);

static __used__ void PartImpact_Coin(PART_s *part) {
    if (coinimpactwait <= 0.0f) {
        f32 speed = NuVecMag(&part->velocity);
        if (speed > 0.0f) {
            i32 angle;
            if (speed > 1.5f)
                angle = 0x4000;
            else
                angle = static_cast<i32>((1.5f - speed) / 1.5f * 16384.0f + 16384.0f);
            f32 volume = NU_SIN_LUT(angle);
            if (GAMEDEMO != 0) {
                PlaySfxAndSetVolume("PickupCoin", &part->position, volume);
                coinimpactwait = 0.1f;
            } else if (static_cast<u16>(part->pickup_type - 0xb7) <= 3) {
                PlaySfxAndSetVolume("CoinDropS", &part->position, volume);
            } else {
                PlaySfxAndSetVolume("CoinDropG", &part->position, volume);
            }
        }
    }
}

static __used__ void PartStolen_Coin(PART_s *part) {
    if (netclient != 0 || (part->render_flags & 2) == 0)
        return;
    GameObject_s *recipient = Player[0];
    i32 first_active = recipient != NULL && static_cast<i8>(recipient->apiobj.flags_low) < 0;
    if (Player[1] != NULL && static_cast<i8>(Player[1]->apiobj.flags_low) < 0) {
        recipient = Player[1];
        if (first_active)
            recipient = Player[qrand() / 0x8000];
    } else if (!first_active) {
        return;
    }
    if (recipient != NULL) {
        GIZMOPICKUP_s pickup;
        SetCoinType(part->pickup_type, &pickup);
        GizmoPickup_CollectCoin(WORLD, &part->position, pickup.type_index, pickup.model_variant, recipient, 1);
    }
}

extern "C" void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);
static __used__ void PartExtra_BlueCoin(PART_s *part) {
    if ((part->render_flags & 2) == 0)
        return;
    i32 effect = WORLD->debris_sys->entries[56].effect;
    if (effect == -1)
        return;
    f32 rate = 5.0f;
    if (WORLD->area != NULL && (WORLD->area->flags & 0x104) == 4)
        rate = 2.5f;
    i32 count = ParticlesPerSecond(rate, FRAMETIME);
    if (count > 0)
        AddVariableShotDebrisEffect(effect, &part->position, count, 0, 0);
}

static __used__ void PartExtra_PurpleCoin(PART_s *part) {
    if ((part->render_flags & 2) == 0)
        return;
    i32 effect = WORLD->debris_sys->entries[57].effect;
    if (effect == -1)
        return;
    f32 rate = 5.0f;
    if (WORLD->area != NULL && (WORLD->area->flags & 0x104) == 4)
        rate = 2.5f;
    i32 count = ParticlesPerSecond(rate, FRAMETIME);
    if (count > 0)
        AddVariableShotDebrisEffect(effect, &part->position, count, 0, 0);
}

static __used__ void PowerUp_DrawPart(PART_s *) {
}

static __used__ void PowerUp_ImpactPart(PART_s *) {
}

static __used__ void PowerUp_UpdatePart(PART_s *) {
}

static __used__ void PowerUp_EndMsg(GAMEMESSAGE_s *) {
}

static __used__ void PowerUp_UpdateMsg(GAMEMESSAGE_s *) {
}

static __used__ i32 SpeederPart_Draw(PART_s *) {
    return true;
}

static __used__ void SpeederPart_Kill(PART_s *, i32) {
}

static __used__ void SpeederPart_Update(PART_s *) {
}

extern WORLDINFO_s *WORLD;
extern AREADATA_s *PODRACE_ADATA;
extern AREADATA_s *BONUS_GUNSHIP_ADATA;
extern AREADATA_s *GUNSHIP_ADATA;

static __used__ i32 PartDraw_VehicleHeart(PART_s *part) {
    f32 scale;
    if (PODRACE_ADATA != NULL && WORLD->area == PODRACE_ADATA)
        scale = 4.0f;
    else if (BONUS_GUNSHIP_ADATA != NULL && WORLD->area == BONUS_GUNSHIP_ADATA)
        scale = 7.5f;
    else if (GUNSHIP_ADATA != NULL && WORLD->area == GUNSHIP_ADATA)
        scale = 7.5f;
    else
        scale = 5.0f;
    NUVEC scaling;
    scaling.x = (1.0f - part->field_100) * scale;
    if (scaling.x > 0.0f) {
        scaling.y = scaling.z = scaling.x;
        NUMTX_ALIGNED16 matrix = part->transform;
        NuMtxPreScale(&matrix, &scaling);
        if (NuSpecialDrawAt(&part->special, &matrix) != 0)
            part->render_flags |= 6;
    }
    return 0;
}

static __used__ i32 PartKill_DrawCreature(PART_s *) {
    return false;
}

static __used__ void PartMove_VehicleHeart(PART_s *, f32) {
}

static __used__ void PartMove_VehiclePickup(PART_s *, f32) {
}

static __used__ void PartKill_EjectedCreature(PART_s *, i32) {
}

static __used__ void UpdateAnimTimer(CHARACTERMODEL_s *, ANIMPACKET_s *, i16, f32, f32, f32, i32, char *, i32, f32) {
}

static __used__ void UpdateCustomPieceAnim(CUSTOMPIECEANIM *, u16, u16) {
}

extern "C" {

    void AddDebrisEffect(i32 *handle, i32 effect_index, f32 x, f32 y, f32 z) {
        if (handle == NULL || effect_index < 0 || EDPP_MAX_TYPES <= effect_index || debtab == NULL ||
            debtab[effect_index] == NULL) {
            return;
        }
        debinftype *effect = debtab[effect_index];
        if (effect->disabled != 0) {
            return;
        }

        bool newly_allocated = false;
        i32 key_index = *handle;
        if (key_index == -1) {
            key_index = DebAlloc();
            *handle = key_index;
            if (key_index == -1) {
                return;
            }
            newly_allocated = true;
        }

        debkeydatatype_s &key = debkeydata[key_index];
        const f32 now = effect->time_group == 4 ? panelglobaltime : globaltime;
        key.field_184 = 0;
        key.effect_index = static_cast<i16>(effect_index);
        DebrisStartOffset(key_index, effect->emission_period);
        key.generator = gensorttab[static_cast<i8>(effect->generator_type)];
        key.momentum_adjuster = gencodetab[static_cast<i8>(effect->momentum_adjustment_type)];
        key.field_1d4 = 0;
        key.sphere_next_time = 0.0f;
        key.emitter_rotation_x = 0;
        key.emitter_rotation_y = 0;
        key.field_2c8 = 0;
        for (i32 i = 0; i < effect->process_spheres; ++i) {
            key.process_spheres[i].time = -1.0f;
        }
        memset(&key.emission_position, 0, sizeof(key.emission_position));
        memset(&key.momentum, 0, sizeof(key.momentum));
        memset(&key.emitter_momentum, 0, sizeof(key.emitter_momentum));
        key.orientation_dirty = 0.0f;
        key.cutoff_distance = 1000000.0f;
        key.field_2f2 = -1;
        key.field_1d8 = 0;
        key.field_1da = 7;
        key.gscene = NULL;
        key.process_collision_sound = 0;
        key.last_update_time = now;
        key.field_2f9 = 1;
        key.field_32c = 0;
        key.field_2fa = 0;
        key.emission_epoch = key.field_1e4 < now ? key.field_1e4 : now;

        switch (effect->particle_type) {
            case 3:
                key.render_priority = static_cast<i16>(40000);
                break;
            case 7:
                key.render_priority = static_cast<i16>(20000);
                break;
            case 2:
                key.render_priority = static_cast<i16>(50000);
                break;
            default:
                key.render_priority = static_cast<i16>(30000);
                break;
        }
        for (i32 i = 0; i != 4; ++i) {
            key.collision_timers[i] = 9999;
            const i32 sound_id = effect->sound_data[i * 3];
            if (sound_id != -1) {
                const i32 mode = effect->sound_data[i * 3 + 1];
                if (mode == 3 || mode == 4) {
                    key.collision_timers[i] = 1;
                }
                key.process_collision_sound = 1;
            }
        }
        DebrisEmitterPos(key_index, x, y, z);
        DebrisEmitterOrientation(key_index, 0, 0, 0);
        DebrisOrientation(key_index, 0, 0);
        DebrisReflectionOrientation(key_index, 0, 0, 0, 0.9f);
        DebrisSetTrigger(key_index, 0, -1, 0);
        if (newly_allocated) {
            AddDebrisEffectToStack(debkeydata + key_index);
        }
    }

    void AddFiniteShotDebrisEffect(i32 *handle, i32 effect, NUVEC *position, i32 count) {
        AddFiniteShotDebrisEffect2(handle, effect, position, NULL, NULL, count);
    }

    void AddFiniteShotDebrisEffect2(i32 *handle, i32 effect, NUVEC *position, NUVEC *emitter_momentum,
                                    NUVEC *particle_momentum, i32 count) {
        if (debris_suspended != 0)
            return;
        AddDebrisEffect(handle, effect, position->x, position->y, position->z);
        if (*handle == -1)
            return;
        debkeydatatype_s &key = debkeydata[*handle];
        key.field_1d4 = count;
        const f32 now = debtab[effect]->time_group == 4 ? panelglobaltime : globaltime;
        key.field_184 = 1;
        key.field_1d8 = 0;
        const f32 offset = key.emission_time - now;
        key.emission_time -= offset;
        key.field_1e4 -= offset;
        key.emission_epoch = key.last_update_time;
        key.cutoff_distance = CameraEmitterDistance(position);
        if (key.process_collision_sound != 0) {
            f32 range = debtab[effect]->sound_range_override;
            if (range == 0.0f)
                range = debtab[effect]->sound_range;
            if (range == 0.0f)
                range = debtab[effect]->clip_extent;
            f32 volume = 0.0f;
            if (key.cutoff_distance < range) {
                for (i32 i = 0; i < 4; ++i) {
                    const i32 sound = debtab[effect]->sound_data[i * 3];
                    if (sound != -1)
                        SetSfxBit_On(sound);
                }
                volume = (range - key.cutoff_distance) / range;
            }
            if (key.cutoff_distance < range) {
                for (i32 i = 0; i < 4; ++i) {
                    const i32 sound = debtab[effect]->sound_data[i * 3];
                    if (sound != -1 && debtab[effect]->sound_data[i * 3 + 1] == 1) {
                        PlaySfxByIdEx(sound, position, volume, 1.0f);
                    }
                }
            }
        }
        if (emitter_momentum != NULL) {
            DebrisEmitterMomentum(*handle, emitter_momentum->x, emitter_momentum->y, emitter_momentum->z);
        }
        if (particle_momentum != NULL) {
            DebrisParticleMomentum(*handle, particle_momentum->x, particle_momentum->y, particle_momentum->z);
        }
    }

    void AddFiniteShotDebrisEffectUserData(void) {
    }

    i32 AddFiniteShotPART(i32 effect, NUVEC *position, i32 count) {
        i32 index = -1;
        if (effect != -1) {
            part_type_s *type = &part_types[effect];
            if (type->effect_ids[0] != -1) {
                index = AddPARTEffect(effect, position);
                if (index != -1) {
                    part_emit_s *emitter = &part_emits[index];
                    emitter->shots_remaining = count;
                    emitter->time_24 = partglobaltime;
                    f32 end = partglobaltime + type->emission_period;
                    emitter->time_28 = end + NuRandFloatSeeded(&partseed) * type->emission_period_random;
                    emitter->time_20 = partglobaltime - 1.0f / type->emission_rate;
                }
            }
        }
        return index;
    }

    void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);

    void AddMSituExtraTerrRot(void) {
    }

    i32 AddPARTEffect(i32 effect, NUVEC *position) {
        CheckPartCount();
        if (part_emits_used == 40 || part_types[effect].effect_ids[0] == -1)
            return -1;
        for (i32 i = 0; i < 40; ++i) {
            if (part_emits[i].effect_id != -1)
                continue;
            part_emit_s *emitter = &part_emits[i];
            emitter->position = *position;
            emitter->effect_id = effect;
            part_type_s *type = &part_types[effect];
            NuStrNCpy(emitter->name, type->name, 16);
            PARTStartOffset(i, 0.0f);
            emitter->rotation_30 = 0;
            emitter->camera_distance = 9999.9f;
            emitter->page = type->page;
            emitter->rotation_2e = 0;
            emitter->rotation_2c = 0;
            emitter->shots_remaining = 0;
            emitter->field_33 = 0;
            emitter->field_3d = 1;
            emitter->field_44 = 0;
            emitter->field_46 = 0;
            ++part_emits_used;
            return i;
        }
        return -1;
    }

    void KillPart(PART_s *, i32);
    void NewPartRotation(PART_s *);
    void DebrisPopulateInstance(i32, f32);
    i32 rtlDynamicAllocTemplate(rtlset *, i32);

    PART_s *AddPart(ADDPART_s *params) {
        NUVEC forward = {0.0f, 0.0f, -1.0f};
        if (params->special != NULL) {
            if (!NuSpecialExistsFn(params->special))
                return NULL;
        } else if (params->field_28 != -1 && params->field_28 != 9999) {
            return NULL;
        }
        PART_s *part = &Part[i_part];
        i32 scanned = 0;
        while ((part->active & 1) != 0 && (part->flags & 0x200) != 0 && scanned < MAXPARTS) {
            if (++i_part == MAXPARTS)
                i_part = 0;
            part = &Part[i_part];
            ++scanned;
        }
        if (scanned == MAXPARTS)
            return NULL;
        if ((part->active & 1) != 0) {
            if (part->replace_callback != NULL)
                part->replace_callback(part);
            KillPart(part, 0);
        }
        if (params->matrix != NULL) {
            part->transform = *params->matrix;
            NUMTX_ALIGNED16 matrix = *params->matrix;
            NuVecMtxRotate(&forward, &forward, &matrix);
            part->rotation_y = NuAtan2D(forward.x, forward.z);
            NuVecRotateY(&forward, &forward, -part->rotation_y);
            part->rotation_x = -NuAtan2D(forward.y, forward.z);
        } else {
            if (params->position == NULL)
                return NULL;
            NuMtxSetTranslation(&part->transform, params->position);
        }
        part->initial_position = part->position;
        part->owner = params->owner;
        part->recipient = params->recipient;
        part->velocity = *params->velocity;
        part->force_player_mask = -1;
        part->field_20b = -1;
        part->field_100 = part->field_104 = params->field_a4;
        part->active = (part->active & ~0x60) | 0x21;
        part->time_step = params->time_step;
        part->scale_time = 0.0f;
        part->target_radius = params->field_14;
        part->radius = (params->time_step / PARTSCALEUPTIME) * params->field_14;
        part->field_0e4 = params->field_18;
        part->field_214 = params->field_c0;
        part->gravity = params->gravity;
        part->field_0ec = params->field_20;
        part->active &= ~2;
        part->pickup_type = params->field_28;
        part->field_0f0 = params->field_88;
        part->flags = params->flags;
        part->field_20a = params->field_c4;
        part->render_flags &= ~0x40;
        if (params->lighting != NULL) {
            for (i32 i = 0; i < 7; ++i)
                part->lighting[i] = params->lighting->values[i];
            part->active |= 8;
        } else {
            part->active &= ~8;
            if (params->field_98 != 1.0f || params->field_9c != 1.0f || params->field_a0 != 1.0f) {
                part->lighting[6].x = params->field_98;
                part->lighting[6].y = params->field_9c;
                part->lighting[6].z = params->field_a0;
                part->render_flags |= 0x20;
            } else {
                part->render_flags &= ~0x20;
            }
        }
        part->source_special = params->special;
        if (params->special != NULL)
            part->special = *params->special;
        else
            memset(&part->special, 0, sizeof(part->special));
        part->active &= ~0x14;
        part->field_1ac = params->field_30;
        part->move_callback = params->move_fn;
        part->update_callback = params->update_fn;
        part->field_1b8 = params->field_3c;
        part->field_1bc = params->field_40;
        part->field_1c0 = params->field_44;
        part->field_1c4 = params->field_48;
        part->stop_callback = params->stop_fn;
        part->replace_callback = params->replace_fn;
        part->draw_callback = params->draw_fn;
        part->bounds_min.x = part->position.x - part->target_radius;
        part->bounds_min.y = part->position.y - part->target_radius;
        part->bounds_min.z = part->position.z - part->target_radius;
        part->bounds_max.x = part->position.x + part->target_radius;
        part->bounds_max.y = part->position.y + part->target_radius;
        part->bounds_max.z = part->position.z + part->target_radius;
        part->elapsed = 0.0f;
        for (i32 i = 0; i < 6; ++i)
            part->field_10c[i] = params->field_a8[i];
        for (i32 i = 0; i < 5; ++i)
            part->field_124[i] = 0.0f;
        if ((part->flags & 0x80) != 0)
            NewPartRotation(part);
        if ((part->flags & 0x100000) != 0)
            NewPartOrderedRotation(part);
        if (params->field_58 != -1) {
            part->debris_handle = -1;
            part->debris_key = &part->debris_handle;
            i32 effect = params->field_58;
            if (params->field_7c != 1.0f)
                effect = CreateScaledEffect(effect, params->field_7c);
            AddDebrisEffect(part->debris_key, effect, part->position.x, part->position.y, part->position.z);
            DebrisPopulateInstance(*part->debris_key, 0.0f);
        } else {
            part->debris_key = params->debris_key;
        }
        part->field_1e0 = params->field_60;
        part->field_1e4 = params->field_64;
        part->field_1e8 = params->field_68;
        part->field_1ec = params->field_6c;
        part->field_1f0 = params->field_70;
        part->field_1f8 = params->field_74;
        part->field_1f4 = params->field_78;
        part->field_1fc = params->field_7c;
        part->previous_transform = part->transform;
        part->field_208 = 1;
        part->force_flags = 0;
        part->field_21c = 0;
        part->render_flags |= 0x18;
        part->field_210 = 0.0f;
        part->field_207 = params->field_90;
        part->field_204 = 1;
        if ((part->flags & 0x2000) != 0 && params->lighting_template != NULL && PartRTL != NULL) {
            part->lighting_template = params->lighting_template;
            *params->lighting_template = rtlDynamicAllocTemplate(PartRTL, params->field_84);
        }
        if (++i_part == MAXPARTS)
            i_part = 0;
        return part;
    }

    void AddRotatedDebrisEffect(void) {
    }

    void AddScaledFiniteShotDebrisEffect(i32 *key, i32 effect, NUVEC *position, NUVEC *orientation, NUVEC *momentum,
                                         i32 count, f32 scale) {
        i32 scaled = CreateScaledEffect(effect, scale);
        if (scaled != -1) {
            AddFiniteShotDebrisEffect2(key, scaled, position, orientation, momentum, count);
        }
    }

    void AddScaledFiniteShotPART(void) {
    }

    void AddScaledVariableShotDebrisEffect(void) {
    }

    void AddVariableShotDebrisEffectTimed1(i32, NUVEC *, i32, f32, i16, i16, NUMTX *);

    i32 AddScaledVariableShotDebrisEffect1(i32 effect, NUVEC *position, i32 count, f32 time, i16 z_rotation,
                                           i16 y_rotation, NUMTX *orientation, f32 scale) {
        i32 scaled = CreateScaledEffect(effect, scale);
        if (scaled != -1) {
            AddVariableShotDebrisEffectTimed1(scaled, position, count, time, z_rotation, y_rotation, orientation);
        }
        return scaled;
    }

    i32 AddScaledVariableShotDebrisEffect2(i32 effect_index, NUVEC *position, i32 count, f32 time,
                                           NUMTX *emitter_orientation, NUMTX *particle_orientation, f32 scale) {
        const i32 scaled_effect = CreateScaledEffect(effect_index, scale);
        if (scaled_effect != -1) {
            AddVariableShotDebrisEffectTimed3(scaled_effect, position, &nuvec_zero, count, time, emitter_orientation,
                                              particle_orientation);
        }
        return scaled_effect;
    }

    void AddScaledVariableShotDebrisEffect3(void) {
    }

    void AddScaledVariableShotDebrisEffect4(void) {
    }

    void AddScaledVariableShotDebrisEffect5(void) {
    }

    void AddScaledVariableShotPARTEffect(void) {
    }

    void AddVariableShotDebrisEffectMtx(i32, NUVEC *, i32, i16, i16, NUMTX *);

    void AddVariableShotDebrisEffect(i32 effect, NUVEC *position, i32 count, i16 z_rotation, i16 y_rotation) {
        AddVariableShotDebrisEffectMtx(effect, position, count, z_rotation, y_rotation, NULL);
    }

    void AddVariableShotDebrisEffectMtx(i32 effect, NUVEC *position, i32 count, i16 z_rotation, i16 y_rotation,
                                        NUMTX *particle_orientation) {
        NUMTX orientation;
        NuMtxSetIdentity(&orientation);
        NuMtxRotateZ(&orientation, z_rotation);
        NuMtxRotateY(&orientation, y_rotation);
        AddVariableShotDebrisEffectMtx3(effect, position, &nuvec_zero, count, &orientation, particle_orientation);
    }

    void AddVariableShotDebrisEffectMtx3(i32 effect, NUVEC *position, NUVEC *momentum, i32 count,
                                         NUMTX *emitter_orientation, NUMTX *particle_orientation) {
        if (effect < 0 || debtab[effect] == NULL)
            return;
        i16 priority = 20000;
        switch (debtab[effect]->particle_type) {
            case 2:
                priority = static_cast<i16>(40000);
                break;
            case 3:
                priority = 30000;
                break;
            case 7:
                priority = 10000;
                break;
        }
        AddVariableShotDebrisEffectMtx4(effect, position, momentum, count, emitter_orientation, particle_orientation,
                                        priority, 0);
    }

    void AddVariableShotDebrisEffectMtx4(i32 effect, NUVEC *position, NUVEC *momentum, i32 count,
                                         NUMTX *emitter_orientation, NUMTX *particle_orientation, i16 priority,
                                         u8 flags) {
        AddVariableShotDebrisEffectTimed5(effect, position, momentum, NULL, count * 30, timeincrement,
                                          emitter_orientation, particle_orientation, priority, flags);
    }

    void AddVariableShotDebrisEffectTimed3(i32, NUVEC *, NUVEC *, i32, f32, NUMTX *, NUMTX *);

    void AddVariableShotDebrisEffectTimed1(i32 effect, NUVEC *position, i32 count, f32 time, i16 z_rotation,
                                           i16 y_rotation, NUMTX *particle_orientation) {
        NUMTX orientation;
        NuMtxSetIdentity(&orientation);
        NuMtxRotateZ(&orientation, z_rotation);
        NuMtxRotateY(&orientation, y_rotation);
        AddVariableShotDebrisEffectTimed3(effect, position, &nuvec_zero, count, time, &orientation,
                                          particle_orientation);
    }

    void AddVariableShotDebrisEffectTimed3(i32 effect_index, NUVEC *position, NUVEC *momentum, i32 count, f32 time,
                                           NUMTX *emitter_orientation, NUMTX *particle_orientation) {
        if (effect_index < 0 || debtab[effect_index] == NULL) {
            return;
        }

        i16 render_priority = 20000;
        switch (debtab[effect_index]->particle_type) {
            case 2:
                render_priority = 40000;
                break;
            case 3:
                render_priority = 30000;
                break;
            case 4:
                render_priority = 20000;
                break;
            case 5:
                render_priority = 20000;
                break;
            case 6:
                render_priority = 20000;
                break;
            case 7:
                render_priority = 10000;
                break;
        }
        AddVariableShotDebrisEffectTimed5(effect_index, position, momentum, NULL, count, time, emitter_orientation,
                                          particle_orientation, render_priority, 0);
    }

    void AddVariableShotDebrisEffectTimed5(i32 effect_index, NUVEC *position, NUVEC *momentum, NUVEC *position_delta,
                                           i32 count, f32 duration, NUMTX *emitter_orientation,
                                           NUMTX *particle_orientation, i16 render_priority, u8 timed_flags) {
        if (debris_suspended != 0 || effect_index < 1 || EDPP_MAX_TYPES <= effect_index || debtab == NULL ||
            debtab[effect_index] == NULL || count < 1) {
            return;
        }

        debinftype *effect = debtab[effect_index];
        if (effect->disabled != 0) {
            return;
        }

        if (effect->time_group != 4) {
            NUVEC extent = {1.0f, 1.0f, 1.0f};
            if (NuCameraClipTestExtentsAxisAligned(position, &extent, effect->clip_extent) == 0) {
                return;
            }
        }

        f32 emission_interval = 0.0f;
        bool no_interval = true;
        const f32 thinning =
            forced_debris_thinning == 0
                ? (debris_thinning_level <= effect->thinning ? debris_thinning_level : effect->thinning)
                : debris_thinning_level;
        if (thinning != 0.0f && count != 0) {
            emission_interval = thinning / static_cast<f32>(count);
            no_interval = emission_interval == 0.0f;
        }

        if (emitter_orientation == NULL) {
            emitter_orientation = &numtx_identity;
        }
        if (particle_orientation == NULL) {
            particle_orientation = &numtx_identity;
        }

        const bool panel_time = effect->time_group == 4;
        const f32 now = panel_time ? panelglobaltime : globaltime;
        const f32 end_time = now + duration;
        const f32 elapsed_intervals = no_interval ? 0.0f : floorf(now / emission_interval);
        f32 emission_time = emission_interval + elapsed_intervals * emission_interval;
        if (end_time < emission_time) {
            return;
        }

        i32 emission_count = 0;
        f32 next_emission_time = emission_time;
        do {
            next_emission_time += emission_interval;
            ++emission_count;
        } while (next_emission_time <= end_time && emission_count != 99);

        const i32 particle_count = emission_count * (static_cast<i32>(effect->trail_count) + 1);
        const i32 particles_per_chunk = effect->particle_type == 7 ? 12 : 32;
        const i32 maximum_particles = effect->particle_type == 7 ? 0x180 : 0x400;

        i32 particle_key_slot = -1;
        debkeydatatype_s *key = NULL;
        for (i32 slot = 0; slot != 8; ++slot) {
            const i16 key_index = effect->particle_keys[slot];
            if (key_index == -1) {
                continue;
            }
            debkeydatatype_s *candidate = &debkeydata[key_index];
            if (candidate->particle_count + particle_count <= maximum_particles) {
                particle_key_slot = slot;
                key = candidate;
                break;
            }
        }

        if (key == NULL) {
            if (freedebkeyptr >= maxdebkeys) {
                return;
            }
            for (i32 slot = 0; slot != 8; ++slot) {
                if (effect->particle_keys[slot] == -1) {
                    const i32 key_index = DebAlloc();
                    if (key_index == -1) {
                        return;
                    }
                    particle_key_slot = slot;
                    effect->particle_keys[slot] = static_cast<i16>(key_index);
                    key = &debkeydata[key_index];
                    key->effect_index = static_cast<i16>(effect_index);
                    key->field_1d4 = 0;
                    key->generator = gensorttab[effect->generator_type];
                    key->momentum_adjuster = gencodetab[effect->momentum_adjustment_type];
                    key->effect_orientation = *particle_orientation;
                    key->effect_orientation.m30 = 0.0f;
                    key->effect_orientation.m31 = 0.0f;
                    key->effect_orientation.m32 = 0.0f;
                    DebrisEmitterPos(key_index, 0.0f, 0.0f, 0.0f);
                    key->timed_flags = timed_flags;
                    key->render_priority = render_priority;
                    break;
                }
            }
            if (key == NULL) {
                return;
            }
        }

        const i32 required_particles = key->particle_count + particle_count;
        const i32 required_chunks = (required_particles + particles_per_chunk - 1) / particles_per_chunk;
        i32 allocated_chunks = key->allocated_chunk_count;
        if (required_chunks > allocated_chunks) {
            const i32 new_chunk_count = required_chunks - allocated_chunks;
            i32 &free_chunk_count = effect->particle_type == 7 ? freedebchkptrg : freedebchkptr;
            const i32 available_chunk_count = effect->particle_type == 7 ? debrischunksglass : debrischunks;
            dma_particle_chunk_s **free_chunks = effect->particle_type == 7 ? freedebchunksglass : freedebchunks;
            if (available_chunk_count <= free_chunk_count + new_chunk_count || required_chunks > 32) {
                return;
            }

            for (i32 i = 0; i != new_chunk_count; ++i) {
                dma_particle_chunk_s *chunk = free_chunks[free_chunk_count + i];
                key->particle_chunks[allocated_chunks + i] = chunk;
                for (i32 particle = 0; particle != particles_per_chunk; ++particle) {
                    chunk->particles[particle].start_time = 0.0f;
                    chunk->particles[particle].inverse_lifetime = 32768.0f;
                }
            }
            free_chunk_count += new_chunk_count;
            key->allocated_chunk_count = static_cast<i16>(required_chunks);
            LinkDmaParticalSets(key->particle_chunks, required_chunks);

            // The renderer needs one entry per contiguous DMA chain, created
            // when the key receives its first chunk; subsequent growth merely
            // relinks that same chain.
            if (required_chunks == new_chunk_count) {
                const i32 total_chunk_count = debrischunks + debrischunksglass;
                particlechunkrendertype_s *render_chunk = NULL;
                for (i32 i = 0; i < total_chunk_count; ++i) {
                    if (ParticleChunkToRender[i].particle_chunk == NULL) {
                        render_chunk = &ParticleChunkToRender[i];
                        break;
                    }
                }
                if (render_chunk != NULL) {
                    render_chunk->particle_chunk = key->particle_chunks[0];
                    render_chunk->effect = effect;
                    render_chunk->key = key;
                    render_chunk->render_priority = render_priority;
                    AddChunkToRenderStack(render_chunk, &ParticleChunkRenderStack[effect->time_group]);
                }
            }
            allocated_chunks = required_chunks;
        }

        key->particle_count = static_cast<i16>(required_particles);
        if (momentum == NULL) {
            key->momentum = nuvec_zero;
        } else {
            key->momentum = *momentum;
        }
        DebrisEmitterOrientationMtx(effect->particle_keys[particle_key_slot], emitter_orientation);
        key->emission_epoch = elapsed_intervals * emission_interval;

        for (i32 i = 0; i != 100 && emission_time <= end_time; ++i) {
            if (position_delta == NULL) {
                key->emission_position = *position;
            } else {
                NuVecAddScale(&key->emission_position, position, position_delta, emission_time - end_time);
            }
            key->emission_time = emission_time;
            uv1deb *particle = key->generator(key, effect, emission_time);
            if (effect->process_spheres != 0 && particle != NULL && i == 0) {
                DebrisProcessSpheres(particle, emission_time, effect, key, 1);
            }
            emission_time = key->emission_epoch + emission_interval;
        }

        DebrisGetControlStackLock();
        while (key->controlled_chunk_count < key->allocated_chunk_count &&
               freechunkcontrolsptr < (debrischunks + debrischunksglass) * 2) {
            const i32 chunk_index = key->controlled_chunk_count;
            debris_chunk_control_s *control = freechunkcontrols[freechunkcontrolsptr++];
            control->particle_chunk = key->particle_chunks[chunk_index];
            control->active = 1;
            control->owner = key;
            control->expiry_time =
                now + effect->particle_lifetime + static_cast<f32>(effect->trail_count) * effect->trail_time;
            AddChunkControlToStack(control, &debris_chunk_control_stack[panel_time ? 1 : 0]);
            ++key->controlled_chunk_count;
        }

        if (key->controlled_chunk_count == key->allocated_chunk_count && key->controlled_chunk_count != 0) {
            debris_chunk_control_s **stack = &debris_chunk_control_stack[panel_time ? 1 : 0];
            const dma_particle_chunk_s *last_chunk = key->particle_chunks[key->controlled_chunk_count - 1];
            for (debris_chunk_control_s *control = *stack; control != NULL; control = control->next) {
                if (control->particle_chunk == last_chunk) {
                    RemoveChunkControlFromStack(control, stack);
                    control->expiry_time =
                        now + effect->particle_lifetime + static_cast<f32>(effect->trail_count) * effect->trail_time;
                    AddChunkControlToStack(control, stack);
                    break;
                }
            }
        }
        DebrisReleaseControlStackLock();

        key->previous_particle_count = key->particle_count;
        key->previous_allocated_chunk_count = key->allocated_chunk_count;
    }

    void AddVariableShotPARTEffect(void) {
    }

    i32 NewRayCast(NUVEC *, NUVEC *, f32, i32);
    void NewRayCastGetImpactNormal(NUVEC *);
    i32 NewRayCastGetImpactTerrainType();
    f32 NewRayCastGetTOFI();
    f32 NewRayCastGetEmbedDist();
    i32 NewRayCastHitWallSpline();
    i32 TerrainPlatId();

    i32 PartRayCast(NUVEC *position, NUVEC *movement, f32 radius, i32 flags) {
        i32 hit = NewRayCast(position, movement, radius, flags);
        NewRayCastGetImpactNormal(&PartNorm);
        return hit;
    }

    i32 PartPlatId() {
        return TerrainPlatId();
    }

    void CastPart(PART_s *part, f32 time) {
        NUVEC movement;
        NUVEC cast_movement;
        part->active &= ~4;
        part->elapsed = time;
        movement.x = time * part->velocity.x;
        movement.y = time * part->velocity.y + time * time * (0.5f * part->gravity);
        movement.z = time * part->velocity.z;
        if (part->velocity.y > 0.0f && movement.y <= 0.0f) {
            time = -part->velocity.y / part->gravity;
            part->elapsed = time;
            movement.x = time * part->velocity.x;
            movement.y = time * part->velocity.y + time * time * (0.5f * part->gravity);
            movement.z = time * part->velocity.z;
        }
        f32 distance = NuVecMagSqr(&movement);
        if (distance < 0.001f || distance > 10000.0f) {
            part->field_209 = -1;
            return;
        }
        i32 hit;
        if ((part->active & 0x60) == 0x20) {
            f32 radius = (part->time_step + time) / PARTSCALEUPTIME * part->target_radius;
            if (radius > part->target_radius)
                radius = part->target_radius;
            cast_movement = movement;
            hit = PartRayCast(&part->position, &cast_movement, radius, 0);
            if (hit == 0) {
                part->field_209 = -1;
                return;
            }
            part->field_209 = NewRayCastGetImpactTerrainType();
            if ((NewRayCastGetTOFI() > 0.0f || PartNorm.y > 0.5f) &&
                ((part->flags & 0x10000000) == 0 || NewRayCastHitWallSpline() != 4))
                goto impact;
            if (hit > 0x10) {
                part->position.x -= PartNorm.x * NewRayCastGetEmbedDist() * 1.05f;
                part->position.y -= PartNorm.y * NewRayCastGetEmbedDist() * 1.05f;
                part->position.z -= PartNorm.z * NewRayCastGetEmbedDist() * 1.05f;
            }
        }
        part->active &= ~0x60;
        cast_movement = movement;
        hit = PartRayCast(&part->position, &cast_movement, part->radius, 0);
        if (hit == 0) {
            part->field_209 = -1;
            return;
        }
        part->field_209 = NewRayCastGetImpactTerrainType();
        if ((NewRayCastGetTOFI() > 0.0f || PartNorm.y > 0.5f) &&
            ((part->flags & 0x10000000) == 0 || NewRayCastHitWallSpline() != 4))
            goto impact;
        if (hit > 0x10) {
            part->position.x -= PartNorm.x * NewRayCastGetEmbedDist() * 1.05f;
            part->position.y -= PartNorm.y * NewRayCastGetEmbedDist() * 1.05f;
            part->position.z -= PartNorm.z * NewRayCastGetEmbedDist() * 1.05f;
        }
        {
            f32 radius = (part->time_step - time) / PARTSCALEUPTIME * part->target_radius;
            if (radius <= 0.0f)
                return;
            part->active = (part->active & ~0x60) | 0x40;
            cast_movement = movement;
            if (PartRayCast(&part->position, &cast_movement, radius, 0) == 0) {
                part->field_209 = -1;
                return;
            }
            part->field_209 = NewRayCastGetImpactTerrainType();
            if ((part->flags & 0x10000000) != 0 && NewRayCastHitWallSpline() == 4)
                return;
        }
    impact:
        part->active |= 4;
        part->field_200 = PartPlatId();
        part->active = (part->active & ~0x10) | (part->field_200 != static_cast<u32>(-1) ? 0x10 : 0);
        part->elapsed *= NewRayCastGetTOFI();
        part->impact_normal = PartNorm;
        part->impact_position.x = part->position.x + cast_movement.x;
        part->impact_position.y = part->position.y + cast_movement.y;
        part->impact_position.z = part->position.z + cast_movement.z;
    }

    void CheckPartCount(void) {
    }

    void DrawParts(i32 keep_offscreen) {
        PART_s *part = Part;
        for (i32 i = 0; i < MAXPARTS; ++i, ++part) {
            if ((part->active & 1) == 0 || (part->flags & 0x1000) != 0)
                continue;
            if (part->draw_callback != NULL && part->draw_callback(part) == 0)
                continue;
            if ((part->active & 8) != 0) {
                NuRndrLightingStateCurrent.direction[0] = part->lighting[3];
                NuRndrLightingStateCurrent.direction[1] = part->lighting[4];
                NuRndrLightingStateCurrent.direction[2] = part->lighting[5];
                NuRndrLightingStateCurrent.intensity[0] = *reinterpret_cast<NUCOLOUR3 *>(&part->lighting[0]);
                NuRndrLightingStateCurrent.intensity[1] = *reinterpret_cast<NUCOLOUR3 *>(&part->lighting[1]);
                NuRndrLightingStateCurrent.intensity[2] = *reinterpret_cast<NUCOLOUR3 *>(&part->lighting[2]);
                NuRndrSetDirectionalLightsPS(&part->lighting[3], reinterpret_cast<NUCOLOUR3 *>(&part->lighting[0]),
                                             &part->lighting[4], reinterpret_cast<NUCOLOUR3 *>(&part->lighting[1]),
                                             &part->lighting[5], reinterpret_cast<NUCOLOUR3 *>(&part->lighting[2]));
                NuRndrLightingStateCurrent.ambient = *reinterpret_cast<NUCOLOUR3 *>(&part->lighting[6]);
                NuRndrSetAmbientLightPS(reinterpret_cast<NUCOLOUR3 *>(&part->lighting[6]));
            } else if ((part->render_flags & 0x20) != 0) {
                NuRndrLightingStateCurrent.ambient = *reinterpret_cast<NUCOLOUR3 *>(&part->lighting[6]);
                NuRndrSetAmbientLightPS(reinterpret_cast<NUCOLOUR3 *>(&part->lighting[6]));
            }
            part->render_flags &= ~2;
            NUMTX_ALIGNED16 scaled;
            NUMTX *matrix = &part->transform;
            if (part->field_214 != 1.0f) {
                scaled = *matrix;
                NUVEC scale = {part->field_214, part->field_214, part->field_214};
                NuMtxPreScale(&scaled, &scale);
                matrix = &scaled;
            }
            i32 drawn = 0;
            if (part->source_special != NULL && NuSpecialDrawAt(&part->special, matrix) != 0) {
                part->render_flags |= 6;
                drawn = 1;
            }
            if ((drawn | keep_offscreen) == 0 && ((part->flags & 0x10) == 0 || (part->render_flags & 1) != 0)) {
                KillPart(part, 0);
            }
        }
    }

    void FindPart(void) {
    }

    i32 GetMaxPartTypes(void) {
        return 0x80;
    }

    void GetPartCount(void) {
    }

    void GetPartName(void) {
    }

    part_type_s part_types[128];
    extern part_emit_s part_emits[512];
    extern i32 part_page_on[8];
    extern i32 part_page_used[8];
    i32 part_types_used;
    i32 part_emits_used;
    NUGSCN *part_scene[32];
    i32 part_scene_pageid[32];
    i32 part_platimpactcnt;
    i16 part_platimpactlist[4];
    void ResetParts(void);

    void InitParts(i32 count, VARIPTR *buffer, VARIPTR) {
        MAXPARTS = count;
        buffer->addr = (buffer->addr + 15) & ~static_cast<usize>(15);
        Part = reinterpret_cast<PART_s *>(buffer->void_ptr);
        buffer->addr += MAXPARTS * sizeof(PART_s);
        ResetParts();
        memset(part_types, 0, sizeof(part_types));
        for (i32 i = 0; i < 128; ++i) {
            for (i32 j = 0; j < 8; ++j) {
                part_types[i].effect_ids[j] = -1;
                part_types[i].effect_pages[j] = -1;
            }
        }
        part_types_used = 0;
        memset(part_emits, 0, 40 * sizeof(part_emit_s));
        for (i32 i = 0; i < 40; ++i) {
            part_emits[i].effect_id = -1;
        }
        part_emits_used = 0;
        memset(part_page_used, 0, sizeof(part_page_used));
        memset(part_page_on, 0, sizeof(part_page_on));
        memset(part_scene, 0, sizeof(part_scene));
        memset(part_scene_pageid, -1, sizeof(part_scene_pageid));
        part_platimpactcnt = 0;
    }

    void KillAllParts(void) {
        if (Part != NULL) {
            PART_s *part = Part;
            for (i32 i = 0; i < MAXPARTS; ++i, ++part) {
                if ((part->active & 1) != 0) {
                    KillPart(part, 0);
                }
            }
        }
    }

    void DebFreeInstantly(i32 *);
    void rtlDynamicFree(i32);

    void KillPart(PART_s *part, i32 reason) {
        if ((part->active & 1) == 0)
            return;
        part->active &= ~1;
        if ((part->flags & 0x20000) != 0 && part->debris_key != NULL) {
            DebFreeInstantly(part->debris_key);
        }
        if (part->lighting_template != NULL) {
            rtlDynamicFree(*part->lighting_template);
            *part->lighting_template = -1;
        }
        if (part->field_1c0 != NULL)
            part->field_1c0(part, reason);
        if (reason != 6 && part->field_1f4 != -1) {
            i32 key = -1;
            AddScaledFiniteShotDebrisEffect(&key, part->field_1f4, &part->position, 0, 0, 1, part->field_1fc);
        }
    }

    void KillPartsByScene(NUGSCN *scene) {
        PART_s *part = Part;
        for (i32 index = 0; index < MAXPARTS; ++index, ++part) {
            if ((part->active & 1) != 0 && part->source_special != NULL && part->special.scene == scene) {
                part->active &= ~1;
                memset(&part->special, 0, sizeof(part->special));
                part->source_special = NULL;
            }
        }
    }

    void NewPartRotation(PART_s *part) {
        part->rotation_axis_1 = (NuRandIntSeeded(&partseed) % 65535u) / 21845u + 1;
        part->field_124[3] = static_cast<i32>((NuRandFloatSeeded(&partseed) * 2.0f - 1.0f) * 65536.0f);
        part->rotation_axis_2 = (NuRandIntSeeded(&partseed) % 65535u) / 21845u + 1;
        while (part->rotation_axis_2 == part->rotation_axis_1) {
            part->rotation_axis_2 = (NuRandIntSeeded(&partseed) % 65535u) / 21845u + 1;
        }
        part->field_124[4] = static_cast<i32>((NuRandFloatSeeded(&partseed) * 2.0f - 1.0f) * 65536.0f);
    }

    void PARTEmitterOrientation(void) {
    }

    void PARTEmitterPos(void) {
    }

    void PARTGetTotalOffTime(void) {
    }

    void PARTGetTotalOnTime(void) {
    }

    i32 PARTLookupType(char *name) {
        if (name != NULL && name[0] != 0) {
            for (i32 i = 0; i < 128; ++i) {
                if (NuStrNICmp(name, part_types[i].name, 16) == 0)
                    return i;
            }
        }
        return -1;
    }

    i32 edpart_load_particle_page = -1;

    void edpartSetParticlePage(i32 page) {
        edpart_load_particle_page = page;
    }

    void edpartStartPage(i8 page) {
        for (i32 i = 0; i < 40; ++i) {
            if (part_emits[i].effect_id != -1 && part_emits[i].page == page) {
                part_emits[i].camera_distance = 9999.9f;
            }
        }
        part_page_on[page] = 1;
    }

    void edpartStopPage(i8 page) {
        part_page_on[page] = 0;
    }

    i32 PARTLookupTypePageOnly(char *name, i32 page) {
        if (name == NULL || name[0] == 0)
            return -1;
        if (static_cast<u32>(page - 1) <= 6) {
            for (i32 i = 0; i < 128; ++i) {
                if (part_types[i].page == page && NuStrNICmp(name, part_types[i].name, 16) == 0)
                    return i;
            }
        }
        for (i32 i = 0; i < 128; ++i) {
            if (part_types[i].page == 0 && NuStrNCmp(name, part_types[i].name, 16) == 0)
                return i;
        }
        return -1;
    }

    void PARTStartOffset(i32 index, f32 offset) {
        if (index == -1)
            return;
        part_emit_s *emitter = &part_emits[index];
        i32 effect = emitter->effect_id;
        if (effect == -1)
            return;
        part_type_s *type = &part_types[effect];
        if (type->emission_period_random == 0.0f && type->emission_pause_random == 0.0f) {
            f32 period = type->emission_period + type->emission_pause;
            emitter->time_24 = static_cast<i32>(partglobaltime / period) * period + offset;
            emitter->time_28 = emitter->time_24 + type->emission_period;
            while (partglobaltime > emitter->time_24 && partglobaltime > emitter->time_28) {
                if (emitter->time_24 >= emitter->time_28)
                    emitter->time_28 = emitter->time_24 + type->emission_period;
                else
                    emitter->time_24 = emitter->time_28 + type->emission_pause;
            }
        } else {
            emitter->time_24 = partglobaltime;
            f32 end = partglobaltime + type->emission_period;
            emitter->time_28 = end + NuRandFloatSeeded(&partseed) * type->emission_period_random;
        }
        f32 interval = 1.0f / type->emission_rate;
        f32 last_emission;
        if (partglobaltime > emitter->time_28)
            last_emission = emitter->time_24 - interval;
        else
            last_emission = static_cast<i32>(partglobaltime / interval) * interval;
        emitter->time_20 = last_emission;
        emitter->time_38 = partglobaltime;
    }

    void ParticleReset(void) {
        for (i32 i = 0; i < 512; ++i) {
            edpp_ptls[i].instance_id = -1;
        }
        memset(edpp_page_used, 0, sizeof(edpp_page_used));
        memset(edpp_page_on, 0, sizeof(edpp_page_on));
        edpp_instances_used = 0;
    }

    void ReassignPickupInst(void) {
    }

    void RemovePARTEffect(i32 index) {
        if (index != -1) {
            CheckPartCount();
            if (part_emits[index].effect_id != -1) {
                part_emits[index].effect_id = -1;
                --part_emits_used;
            }
            CheckPartCount();
        }
    }

    void ResetParts(void) {
        if (Part != NULL) {
            memset(Part, 0, static_cast<usize>(MAXPARTS) * sizeof(PART_s));
        }
        i_part = 0;
    }

    void FullReflect(NUVEC *, NUVEC *, NUVEC *);
    void DebrisStatusAlwaysOff(i32 *);
    i32 rtlDynamicSetPos(i32, NUVEC *);

    void UpdateParts(f32 time) {
        i32 key;
        NUVEC direction;
        NUMTX_ALIGNED16 impact_orientation;
        NUMTX_ALIGNED16 movement_orientation;
        NUMTX_ALIGNED16 emitter_orientation;
        partglobaltime += time;
        if (partglobaltime > 900.0f)
            PartTimeSlip();
        PartCleanupTypes();
        part_platimpactcnt = 0;
        UpdatePartEmits(time);
        PART_s *parts = Part;
        for (i32 i = 0; i < MAXPARTS; ++i) {
            PART_s *part = &parts[i];
            if ((part->active & 1) == 0)
                continue;
            part->previous_transform = part->transform;
            part->scale_time += time;
            if ((part->active & 2) == 0) {
                f32 movement_time = time;
                if ((part->flags & 0x400) == 0) {
                    if (time >= part->elapsed) {
                        part->active = (part->active & ~0x60) | (part->target_radius > part->radius ? 0x20 : 0);
                        if ((part->active & 4) != 0) {
                            if ((part->flags & 0x80000) != 0 && (part->active & 0x10) != 0 && part_platimpactcnt < 4) {
                                part_platimpactlist[part_platimpactcnt++] = part->field_200;
                            }
                            if ((part->flags & 0x100) != 0) {
                                part->active |= 2;
                                if (part->stop_callback != NULL)
                                    part->stop_callback(part);
                                if ((part->flags & 1) != 0 || (part->active & 0x10) != 0)
                                    KillPart(part, 0);
                            } else {
                                movement_time -= part->elapsed;
                                part->position = part->impact_position;
                                FullReflect(&part->impact_normal, &part->velocity, &part->velocity);
                                if ((part->flags & 0x80) != 0)
                                    NewPartRotation(part);
                                if ((part->flags & 0x100000) != 0)
                                    NewPartOrderedRotation(part);
                                part->velocity.x *= part->field_0ec;
                                part->velocity.z *= part->field_0ec;
                                if (part->impact_normal.y > 0.5f) {
                                    part->velocity.y *= part->field_0ec;
                                    if (part->velocity.x * part->velocity.x + part->velocity.y * part->velocity.y +
                                            part->velocity.z * part->velocity.z <
                                        0.1f) {
                                        part->active |= 2;
                                        if (part->stop_callback != NULL)
                                            part->stop_callback(part);
                                        if ((part->flags & 1) != 0 || (part->active & 0x10) != 0 ||
                                            (part->render_flags & 1) != 0)
                                            KillPart(part, 0);
                                    } else if ((part->render_flags & 1) != 0) {
                                        KillPart(part, 0);
                                    }
                                }
                            }
                            if (part->field_1f0 != -1) {
                                key = -1;
                                NuMtxSetIdentity(&impact_orientation);
                                NuMtxAlignY(&impact_orientation, &part->impact_normal);
                                AddScaledFiniteShotDebrisEffect(&key, part->field_1f0, &part->impact_position, NULL,
                                                                NULL, 1, part->field_1fc);
                                DebrisOrientationMtx(key, &impact_orientation);
                            }
                            if (part->field_1f8 != -1)
                                AddFiniteShotPART(part->field_1f8, &part->impact_position, 1);
                            if (part->field_1b8 != NULL)
                                part->field_1b8(part);
                        }
                        if (part_raycasts_enabled != 0)
                            CastPart(part, 0.1f);
                        if ((part->active & 4) != 0 && movement_time >= part->elapsed)
                            movement_time = 0.0f;
                    }
                    part->elapsed -= movement_time;
                }
                if (movement_time > 0.0f) {
                    if (part->move_callback != NULL) {
                        part->move_callback(part, movement_time);
                    } else {
                        part->position.x += part->velocity.x * movement_time;
                        part->position.y += part->velocity.y * movement_time;
                        part->position.z += part->velocity.z * movement_time;
                        part->velocity.y += part->gravity * movement_time;
                    }
                }
                if ((part->render_flags & 1) == 0 && part->field_0f0 != 2000000.0f &&
                    part->position.y < part->field_0f0)
                    part->render_flags |= 1;
                part->bounds_min.x = part->position.x - part->target_radius;
                part->bounds_max.x = part->position.x + part->target_radius;
                part->bounds_min.y = part->position.y - part->target_radius;
                part->bounds_max.y = part->position.y + part->target_radius;
                part->bounds_min.z = part->position.z - part->target_radius;
                part->bounds_max.z = part->position.z + part->target_radius;
                if ((part->active & 0x60) == 0x20) {
                    part->time_step += time;
                    part->radius = part->time_step / PARTSCALEUPTIME * part->target_radius;
                    if (part->radius > part->target_radius) {
                        part->radius = part->target_radius;
                        part->active &= ~0x60;
                    }
                } else if ((part->active & 0x60) == 0x40) {
                    part->time_step -= time;
                    if (part->time_step <= 0.0f)
                        KillPart(part, 0);
                    else
                        part->radius = part->time_step / PARTSCALEUPTIME * part->target_radius;
                }
                if ((part->flags & 0x80) != 0) {
                    i32 angle = static_cast<i32>(part->field_124[3] * time);
                    switch (part->rotation_axis_1) {
                        case 1:
                            NuMtxPreRotateX(&part->transform, angle);
                            break;
                        case 2:
                            NuMtxPreRotateY(&part->transform, angle);
                            break;
                        case 3:
                            NuMtxPreRotateZ(&part->transform, angle);
                            break;
                    }
                    angle = static_cast<i32>(part->field_124[4] * time);
                    switch (part->rotation_axis_2) {
                        case 1:
                            NuMtxPreRotateX(&part->transform, angle);
                            break;
                        case 2:
                            NuMtxPreRotateY(&part->transform, angle);
                            break;
                        case 3:
                            NuMtxPreRotateZ(&part->transform, angle);
                            break;
                    }
                }
                if ((part->flags & 0x100000) != 0) {
                    if (part->field_124[0] != 0)
                        NuMtxPreRotateX(&part->transform, static_cast<i32>(part->field_124[0] * time));
                    if (part->field_124[1] != 0)
                        NuMtxPreRotateY(&part->transform, static_cast<i32>(part->field_124[1] * time));
                    if (part->field_124[2] != 0)
                        NuMtxPreRotateZ(&part->transform, static_cast<i32>(part->field_124[2] * time));
                }
                if ((part->flags & 0x800) != 0) {
                    direction.x = -part->velocity.x;
                    direction.y = -part->velocity.y;
                    direction.z = -part->velocity.z;
                    NuMtxSetIdentity(&movement_orientation);
                    NuMtxLookAtZ(&movement_orientation, &direction);
                    memmove(&part->transform, &movement_orientation, 0x30);
                }
            }
            if ((part->active & 1) != 0) {
                if ((part->flags & 2) != 0 && part->field_1bc != NULL)
                    part->field_1bc(part);
                if ((part->active & 1) != 0) {
                    if (part->field_1c4 != NULL)
                        part->field_1c4(part);
                    if (part->field_100 > 0.0f) {
                        part->field_100 -= time;
                        if (part->field_100 <= 0.0f)
                            KillPart(part, 0);
                    }
                }
            }
            if ((part->flags & 0x20000) != 0 && part->debris_key != NULL) {
                NuMtxSetIdentity(&emitter_orientation);
                direction = part->velocity;
                direction.x = -direction.x;
                direction.y = -direction.y;
                direction.z = -direction.z;
                NuMtxLookAtY(&emitter_orientation, &direction);
                DebrisOrientationMtx(*part->debris_key, &emitter_orientation);
                DebrisEmitterPos(*part->debris_key, part->position.x, part->position.y, part->position.z);
                if (((part->flags & 0x200000) != 0 && (part->active & 2) != 0) || (part->flags & 0x1000) != 0)
                    DebrisStatusAlwaysOff(part->debris_key);
            }
            if (time != 0.0f) {
                if ((part->flags & 0x40000) != 0 && part->field_1e0 != -1 &&
                    !((part->flags & 0x200000) != 0 && (part->active & 2) != 0) && (part->flags & 0x1000) == 0)
                    AddScaledVariableShotDebrisEffect1(part->field_1e0, &part->position,
                                                       static_cast<i32>(part->field_1e8), time, 0, 0, NULL,
                                                       part->field_1fc);
                if ((part->flags & 0x40000) != 0 && part->field_1e4 != -1 &&
                    !((part->flags & 0x200000) != 0 && (part->active & 2) != 0) && (part->flags & 0x1000) == 0)
                    AddScaledVariableShotDebrisEffect1(part->field_1e4, &part->position,
                                                       static_cast<i32>(part->field_1ec), time, 0, 0, NULL,
                                                       part->field_1fc);
            }
            if (part->lighting_template != NULL) {
                if ((part->flags & 0x1000) != 0) {
                    direction.x = direction.y = direction.z = 10000.0f;
                    rtlDynamicSetPos(*part->lighting_template, &direction);
                } else {
                    rtlDynamicSetPos(*part->lighting_template, &part->position);
                }
            }
            if (part->field_1ac != NULL)
                part->field_1ac(part);
            part->field_208 = 0;
            if ((part->active & 1) == 0) {
                KillPart(part, 0);
                memset(part, 0, sizeof(PART_s));
            }
        }
    }

} // extern "C"

extern u16 TargetDeg_Near, TargetDeg_Mid, TargetDeg_Far;
extern f32 TargetDist_Near2, TargetDist_Mid2;
BOLTTYPE_s *BoltType_FindByID(i32, WORLDINFO_s *);
PART_s *TargetPart(GameObject_s *object, nuvec_s *position, nuvec_s *direction, f32 radius, f32 range_squared,
                   i32 directional, i32 bolt_id) {
    NUVEC aim = *direction;
    BOLTTYPE_s *bolt = BoltType_FindByID(bolt_id, WORLD);
    PART_s *part = Part;
    f32 min_x = position->x - radius, max_x = position->x + radius;
    f32 min_z = position->z - radius, max_z = position->z + radius;
    if (directional != 0) {
        aim = *direction;
        if ((bolt->field_60 & 0x20000) != 0) {
            aim.y = 0.0f;
            NuVecNorm(&aim, &aim);
        }
    }
    PART_s *best = NULL;
    PART_s *previous = NULL;
    f32 nearest_distance = range_squared;
    for (i32 i = 0; i < MAXPARTS; ++i, ++part) {
        if ((part->active & 1) == 0 || ((part->flags & 0x8000000) == 0 && (part->flags & 0xa) != 0xa))
            continue;
        if (part->position.x < min_x || part->position.x > max_x || part->position.z < min_z ||
            part->position.z > max_z)
            continue;
        NUVEC delta;
        f32 distance = NuVecDistSqr(&part->position, position, &delta);
        if (!(range_squared > distance))
            continue;
        if (directional == 0)
            NuVecRotateY(&aim, &v001, NuAtan2D(part->position.x - position->x, part->position.z - position->z));
        if ((bolt->field_60 & 0x20000) != 0)
            delta.y = 0.0f;
        NuVecNorm(&delta, &delta);
        f32 dot = NuVecDot(&delta, &aim);
        u16 angle;
        if (TargetDist_Near2 > distance && directional != 0)
            angle = TargetDeg_Near;
        else if (TargetDist_Mid2 > distance)
            angle = TargetDeg_Mid;
        else
            angle = TargetDeg_Far;
        if (!(dot > NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff]) || !(nearest_distance > distance))
            continue;
        if (object->attack_part_target == part)
            previous = part;
        else {
            nearest_distance = distance;
            best = part;
        }
    }
    return best != NULL ? best : previous;
}

extern "C" i32 NuGScnNumSpecials(nugscn_s *);
i32 edpartLookupObjectInScene(char *name, nugscn_s *scene) {
    if (name[0] == 0)
        return -1;
    if (NuStrNCmp(name, "NULL instance", 16) == 0)
        return 9999;
    if (scene != NULL) {
        nuhspecial_s special;
        const i32 count = NuGScnNumSpecials(scene);
        for (i32 i = 0; i < count; ++i) {
            NuGScnGetSpecial(&special, scene, i);
            if (NuSpecialExistsFn(&special) && NuStrNCmp(name, NuSpecialGetName(&special), 16) == 0)
                return i;
        }
    }
    return -1;
}
i32 edpartLookupDebrisEffect(char *name) {
    if (edpart_load_particle_page != -1)
        return LookupDebrisEffectPageOnly(name, static_cast<i8>(edpart_load_particle_page));
    return LookupDebrisEffect(name);
}
extern "C" i32 GetSfxIdN(char *, i32);

void edpartLoadSingleType(part_typedesc_s *type, i32 version, i32 page) {
    char name[16];
    EdFileRead(type->name, 16);
    if (version > 5) {
        for (i32 i = 0; i < 8; ++i)
            type->effect_pages[i] = EdFileReadChar();
    } else {
        type->effect_pages[0] = version > 1 ? EdFileReadChar() : 0;
        for (i32 i = 1; i < 8; ++i)
            type->effect_pages[i] = -1;
    }
    const i32 variants = version > 5 ? 8 : 1;
    for (i32 i = 0; i < variants; ++i) {
        EdFileRead(type->object_names[i], 16);
        type->effect_ids[i] = -1;
        if (type->effect_pages[i] == 0 || type->effect_pages[i] == 1) {
            const i32 scene_page = type->effect_pages[i] == 0 ? page : 0;
            for (i32 scene = 0; scene < 32; ++scene) {
                if (part_scene_pageid[scene] == scene_page) {
                    type->effect_ids[i] = edpartLookupObjectInScene(type->object_names[i], part_scene[scene]);
                }
                if (type->effect_ids[i] != -1) {
                    type->scene_indices[i] = scene;
                    break;
                }
            }
        }
        if (type->effect_ids[i] == -1 && type->object_names[i][0] != 0)
            type->effect_ids[i] = 9998;
    }
    type->variant_count = 0;
    for (i32 i = 0; i < 8; ++i) {
        if (type->effect_ids[i] != 9999 && type->effect_ids[i] != -1)
            ++type->variant_count;
    }
    type->variant_mode = version > 5 ? EdFileReadChar() : 0;
    type->particle_scale = version > 15 ? EdFileReadFloat() : 1.0f;
    type->effect_scale = version > 15 ? EdFileReadFloat() : 1.0f;
    type->lifetime = EdFileReadFloat();
    type->lifetime_random = version > 13 ? EdFileReadFloat() : 0.0f;
    type->speed = EdFileReadFloat();
    type->gravity = EdFileReadFloat();
    type->emission_rate = EdFileReadFloat();
    type->bounce = version > 7 ? EdFileReadFloat() : 0.75f;
    type->position_random.x = EdFileReadFloat();
    type->position_random.y = EdFileReadFloat();
    type->position_random.z = EdFileReadFloat();
    type->velocity_random.x = EdFileReadFloat();
    type->velocity_random.y = EdFileReadFloat();
    type->velocity_random.z = EdFileReadFloat();
    type->emission_period = EdFileReadFloat();
    type->emission_period_random = EdFileReadFloat();
    type->emission_pause = EdFileReadFloat();
    type->emission_pause_random = EdFileReadFloat();
    if (version > 12) {
        for (i32 i = 0; i < 3; ++i)
            type->rotation[i] = EdFileReadInt();
        for (i32 i = 0; i < 3; ++i)
            type->rotation_random[i] = EdFileReadInt();
    } else {
        for (i32 i = 0; i < 3; ++i)
            type->rotation[i] = EdFileReadShort();
        for (i32 i = 0; i < 3; ++i)
            type->rotation_random[i] = EdFileReadShort();
    }
    type->flags = EdFileReadUnsignedInt();
    if (type->effect_ids[0] == 9999)
        type->flags |= 0x10;
    if (version > 4) {
        EdFileRead(name, 16);
        type->trail_effects[0] = edpartLookupDebrisEffect(name);
        if (version > 8) {
            EdFileRead(name, 16);
            type->trail_effects[1] = edpartLookupDebrisEffect(name);
        } else
            type->trail_effects[1] = -1;
        EdFileRead(name, 16);
        type->attached_effect = edpartLookupDebrisEffect(name);
        type->trail_rates[0] = EdFileReadFloat();
        type->trail_rates[1] = version > 8 ? EdFileReadFloat() : 60.0f;
    } else {
        type->trail_effects[0] = -1;
        type->trail_effects[1] = -1;
        type->attached_effect = -1;
        if (version > 2) {
            const char mode = EdFileReadChar();
            EdFileRead(name, 16);
            if (mode == 0)
                type->trail_effects[0] = edpartLookupDebrisEffect(name);
            else if (mode == 1)
                type->attached_effect = edpartLookupDebrisEffect(name);
            type->trail_rates[0] = EdFileReadFloat();
        } else
            type->trail_rates[0] = 60.0f;
        type->trail_rates[1] = 60.0f;
    }
    if (version > 3) {
        EdFileRead(name, 16);
        type->kill_effect = edpartLookupDebrisEffect(name);
        EdFileRead(name, 16);
        type->impact_effect = edpartLookupDebrisEffect(name);
    } else {
        type->impact_effect = -1;
        type->kill_effect = -1;
    }
    if (version > 11) {
        EdFileRead(type->impact_part_name, 16);
        type->impact_part = PARTLookupType(type->impact_part_name);
    } else {
        type->impact_part_name[0] = 0;
        type->impact_part = -1;
    }
    if (version > 6) {
        for (i32 i = 0; i < 4; ++i) {
            EdFileRead(name, 16);
            type->sounds[i] = GetSfxIdN(name, 16);
        }
        for (i32 i = 0; i < 4; ++i)
            type->sound_modes[i] = EdFileReadChar();
    } else {
        for (i32 i = 0; i < 4; ++i)
            type->sounds[i] = -1;
        for (i32 i = 0; i < 4; ++i)
            type->sound_modes[i] = 0;
    }
    type->maximum_distance = version > 9 ? EdFileReadFloat() : 25.0f;
    type->field_160 = version > 14 ? EdFileReadFloat() : 1.0f;
    type->field_164 = version > 14 ? EdFileReadFloat() : 1.0f;
    type->field_168 = version > 14 ? EdFileReadFloat() : 1.0f;
    type->scale = 1.0f;
    type->field_174 = 0;
}

extern "C" {
    i32 edpart_nearest;
    NUVEC edpart_cam_pos;
    part_emit_s *edpart_nearest_emit;
    part_typedesc_s *edpart_nearest_type;
}

void edpartDetermineNearest(f32 distance) {
    NUVEC delta;
    if (edpart_nearest != -1 && part_emits[edpart_nearest].effect_id != -1) {
        NuVecSub(&delta, &edpart_cam_pos, &part_emits[edpart_nearest].position);
        if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z == 0.0f)
            return;
    }
    edpart_nearest = -1;
    for (i32 i = 0; i < 40; ++i) {
        if (part_emits[i].effect_id != -1) {
            NuVecSub(&delta, &edpart_cam_pos, &part_emits[i].position);
            const f32 squared_distance = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
            if (distance < 0.0f || squared_distance < distance) {
                distance = squared_distance;
                edpart_nearest = i;
            }
        }
    }
    edpart_nearest_emit = NULL;
    edpart_nearest_type = NULL;
    if (edpart_nearest != -1) {
        edpart_nearest_emit = &part_emits[edpart_nearest];
        if (edpart_nearest_emit->effect_id != -1)
            edpart_nearest_type = &part_types[edpart_nearest_emit->effect_id];
    }
}
extern "C" {
    i32 edbits_part_general_page;
    i32 edbits_part_level_page = 1;

    i32 edpartLoadPageEx(char *path, i8 mode, nugscn_s **scenes, i32 scene_count) {
        i32 page = 0;
        if (mode != 0) {
            for (page = 1; page < 8; ++page) {
                if (part_page_used[page] == 0)
                    break;
            }
            if (page == 8)
                return -1;
        }
        i32 scene_slot = 0;
        for (i32 i = 0; i < scene_count; ++i) {
            if (scenes[i] != NULL) {
                while (part_scene[scene_slot] != NULL && scene_slot < 32)
                    ++scene_slot;
                if (scene_slot < 32) {
                    part_scene[scene_slot] = scenes[i];
                    part_scene_pageid[scene_slot] = page;
                }
            }
        }
        EdFileSetMedia(1);
        if (EdFileOpen(path, NUFILE_READ) == 0)
            return -1;
        EdFileSetReadWrongEndianess(1);
        const i32 version = EdFileReadInt();
        if (static_cast<u32>(version - 1) > 15) {
            EdFileClose();
            return -1;
        }
        i32 type_count = EdFileReadInt();
        i32 skipped_types = 0;
        if (type_count + part_types_used > 128) {
            skipped_types = type_count + part_types_used - 128;
            type_count -= skipped_types;
        }
        i32 type_slot = 0;
        for (i32 i = 0; i < type_count; ++i) {
            while (part_types[type_slot].name[0] != 0)
                ++type_slot;
            part_typedesc_s *type = &part_types[type_slot];
            edpartLoadSingleType(type, version, page);
            if (type->name[0] != 0) {
                type->page = page;
                type->field_b3 = mode;
                ++part_types_used;
            }
        }
        part_typedesc_s discarded_type;
        for (i32 i = 0; i < skipped_types; ++i)
            edpartLoadSingleType(&discarded_type, version, page);
        i32 emitter_count = EdFileReadInt();
        if (emitter_count + part_emits_used > 40)
            emitter_count = 40 - part_emits_used;
        CheckPartCount();
        i32 emitter_slot = 0;
        for (i32 i = 0; i < emitter_count; ++i) {
            while (part_emits[emitter_slot].effect_id != -1)
                ++emitter_slot;
            part_emit_s *emitter = &part_emits[emitter_slot];
            emitter->position.x = EdFileReadFloat();
            emitter->position.y = EdFileReadFloat();
            emitter->position.z = EdFileReadFloat();
            EdFileRead(emitter->name, 16);
            emitter->effect_id = PARTLookupTypePageOnly(emitter->name, page);
            emitter->rotation_30 = EdFileReadShort();
            emitter->rotation_2e = EdFileReadShort();
            emitter->rotation_2c = EdFileReadShort();
            if (version > 10) {
                emitter->field_44 = EdFileReadShort();
                emitter->field_46 = EdFileReadShort();
            } else {
                emitter->field_44 = 0;
                emitter->field_46 = 0;
            }
            emitter->page = page;
            emitter->field_3c = 0;
            part_typedesc_s *type = &part_types[emitter->effect_id];
            for (i32 sound = 0; sound < 4; ++sound) {
                if (type->sounds[sound] != -1 && type->sound_modes[sound] != 0) {
                    emitter->field_3c = 1;
                    break;
                }
            }
            if (emitter->effect_id != -1) {
                ++part_emits_used;
                PARTStartOffset(emitter_slot, 0.0f);
                emitter->field_3d = 1;
            }
        }
        CheckPartCount();
        EdFileSetReadWrongEndianess(0);
        EdFileClose();
        edpartDetermineNearest(1.0f);
        part_page_used[page] = 1;
        if (page == 0)
            part_page_on[0] = 1;
        if (mode == 0)
            edbits_part_general_page = page;
        else if (mode == 1)
            edbits_part_level_page = page;
        return page;
    }

    i32 edpartLoadPage(char *path, i32 mode, void *scene) {
        return edpartLoadPageEx(path, static_cast<i8>(mode), reinterpret_cast<nugscn_s **>(&scene), 1);
    }
}

void PartTimeSlip() {
    for (i32 i = 0; i < 40; ++i) {
        if (part_emits[i].effect_id != -1) {
            part_emits[i].time_20 -= 800.0f;
            part_emits[i].time_24 -= 800.0f;
            part_emits[i].time_28 -= 800.0f;
        }
    }
    for (i32 i = 0; i < 128; ++i) {
        if (part_types[i].effect_ids[0] != -1) {
            part_types[i].last_used_time -= 800.0f;
        }
    }
    partglobaltime -= 800.0f;
}

extern i32 PDEBCOUNT;
extern void *PDebNameList;

void InitPartTable(char **names) {
    PDEBCOUNT = 0;
    PDebNameList = names;
    if (names != NULL) {
        while (names[PDEBCOUNT] != NULL) {
            ++PDEBCOUNT;
        }
    }
}

i32 SetPartTarget(GameObject_s *object, PART_s *target) {
    object->attack_target_position = target->position;
    object->attack_target_velocity = target->velocity;
    object->field_0xe21 |= 8;
    object->attack_part_target = VehicleArea != 0 ? NULL : target;
    return 1;
}

WORLDINFO_s *WorldInfo_CurrentlyActive();
extern f32 FRAMETIME;
void PartCollide_3D(PART_s *);
void PartCollide_2D(PART_s *);
void PartUpdate_Heart(PART_s *);
void PartStop_Flickerer(PART_s *);
i32 PartDraw_Flickerer(PART_s *);
i32 PartDraw_Torp(PART_s *);

void AddTorpedoAsPart(nuvec_s *position, nuvec_s *velocity, float scale, float lifetime) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    ADDPART_ALIGNED16 params = Default_ADDPART;
    params.position = position;
    params.velocity = velocity;
    f32 torpedo_scale = 0.1f * WORLD->giz_torp_machine_sys->scale;
    params.field_14 = torpedo_scale * GizmoPickupType[8].shadow_radius_x;
    params.field_18 = torpedo_scale * GizmoPickupType[8].shadow_extent_x;
    params.gravity = AreaPickupGravity;
    if (params.gravity == 0.0f)
        params.field_a4 = 15.0f;
    params.special = &world->lev_objs[0x79].special;
    params.field_28 = 0x79;
    params.flags = 0x256;
    params.move_fn = params.gravity == 0.0f ? PartMove_VehiclePickup : NULL;
    params.field_40 = PartCollide_3D;
    if (FreePlay != 0 && WORLD->current_level == ANAKINSFLIGHTB_LDATA)
        params.field_40 = PartCollide_2D;
    params.field_48 = NULL;
    params.stop_fn = PartStop_Flickerer;
    params.draw_fn = PartDraw_Torp;
    params.field_88 = lifetime;
    params.time_step = FRAMETIME;
    params.field_c0 = scale;
    PART_s *part = AddPart(&params);
    if (part != NULL) {
        part->rotation_y = qrand();
        part->force_player_mask = 3;
        part->field_214 = 0.1f * WORLD->giz_torp_machine_sys->scale * NU_SIN_LUT(0x4000);
    }
}

void AddHeartAsPart(GameObject_s *recipient, nuvec_s *position, nuvec_s *velocity, float scale, float lifetime) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (recipient == NULL)
        return;
    ADDPART_ALIGNED16 params = Default_ADDPART;
    params.position = position;
    params.velocity = velocity;
    params.field_28 = 0xcb;
    params.flags = 0x256;
    params.field_18 = params.field_14 = scale * GizmoPickupType[5].shadow_radius_x;
    params.gravity = AreaPickupGravity;
    params.special = &world->lev_objs[0xcb].special;
    params.field_88 = lifetime;
    params.field_40 = PartCollide_3D;
    params.field_48 = PartUpdate_Heart;
    if ((world->current_level->flags & LEVEL_PICKUPS_TO_PANEL) != 0) {
        params.field_a4 = 1.0f;
        params.move_fn = PartMove_VehicleHeart;
        params.flags |= 0x400;
        params.recipient = recipient;
        params.draw_fn = PartDraw_VehicleHeart;
    } else {
        params.stop_fn = PartStop_Flickerer;
        params.draw_fn = PartDraw_Flickerer;
        if (params.gravity == 0.0f)
            params.move_fn = PartMove_VehiclePickup;
    }
    params.time_step = FRAMETIME;
    params.field_c0 = scale;
    PART_s *part = AddPart(&params);
    if (part != NULL)
        part->rotation_y = qrand();
}

i32 FindPartDebris(PARTDEBSYS_s *system, char *name) {
    if (system != NULL) {
        for (i32 i = system->named_count; i < system->capacity; ++i) {
            if (NuStrICmp(name, system->entries[i].name) == 0)
                return system->entries[i].type_id;
        }
    }
    return -1;
}

void MakePartVector(NUVEC *velocity, NUVEC *direction, f32 scale) {
    if (direction == NULL) {
        f32 random = static_cast<f32>(qrand()) * (1.0f / 65535.0f);
        velocity->x = random + random - 1.0f;
        velocity->y = AreaPickupGravity == 0.0f ? 0.0f : static_cast<f32>(qrand()) * (1.0f / 65535.0f) + 1.0f;
        random = static_cast<f32>(qrand()) * (1.0f / 65535.0f);
        velocity->z = random + random - 1.0f;
        NuVecScale(velocity, velocity, AreaPickupScale);
    } else {
        FindAnglesZX(direction, NULL, NULL);
        *velocity = v010;
        const i32 z_random = qrand();
        const i32 x_random = qrand();
        NuVecRotateZ(velocity, velocity, (z_random / 6 + static_cast<u16>(temp_zrot) - 0x1555) & 0xffff);
        NuVecRotateX(velocity, velocity, (x_random / 6 + static_cast<u16>(temp_xrot) - 0x1555) & 0xffff);
        NuVecScale(velocity, velocity, scale);
    }
}

void PartCollide_2D(PART_s *part) {
    PartCollide(part, 0);
}

void PartCollide_3D(PART_s *part) {
    PartCollide(part, 1);
}

void SetKillPartMom(nuvec_s *momentum) {
    momentum->x = 0.0f;
    momentum->y = 1.0f;
    momentum->z = 0.0f;
    NuVecRotateZ(momentum, momentum, qrand());
    NuVecRotateX(momentum, momentum, qrand());
}

WORLDINFO_s *WorldInfo_CurrentlyActive();
extern f32 FRAMETIME;
void PartStop_Coin(PART_s *part) {
    if (netclient == 0) {
        WORLDINFO_s *world = WORLD;
        GIZMOPICKUPRUNTIMESYS_s *system = world->gizmo_pickup_sys;
        GIZMOPICKUP_s *pickup = &system->temporary_pickups[system->field_0x0c];
        if (++system->field_0x0c > 63)
            system->field_0x0c = 0;
        if (pickup->state_flags & 1)
            GizmoPickup_CollectCoin(world, &pickup->position, pickup->type_index, pickup->model_variant, NULL, 1);
        SetCoinType(part->pickup_type, pickup);
        pickup->state_flags = (pickup->state_flags & 0xe0) | 0x17;
        pickup->position = part->position;
        pickup->draw_rotation = 0;
        pickup->room_index =
            WORLD->current_gscn != NULL ? NuPortalWhichRoom(WORLD->current_gscn, &pickup->position) : -1;
        pickup->floor_height = 2000000.0f;
        pickup->remaining_visible_time = 5.0f;
        pickup->shadow_x_rotation = 0;
        pickup->shadow_z_rotation = 0;
        pickup->state_flags = (pickup->state_flags | 0x20) & ~0x40;
    }
    part->active &= ~1;
}

void AddCoinsAsParts(i32 type_id, nuvec_s *position, nuvec_s *velocity, float lifetime, float scale) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    GIZMO_PICKUP_TYPE *type = &GizmoPickupType[type_id];
    i32 model = static_cast<i16>(type->first_model_id);
    if (type->random_model_count != 0)
        model += qrand() / (65535 / type->random_model_count + 1);
    LEVEL_OBJECT_RUNTIME_s *object = &world->lev_objs[model];
    if (object->active == 0)
        return;
    ADDPART_ALIGNED16 params = Default_ADDPART;
    params.special = &object->special;
    params.field_28 = model;
    params.position = position;
    params.velocity = velocity;
    params.field_14 = params.field_18 = scale * type->shadow_radius_x;
    params.flags = 0x56;
    params.gravity = AreaPickupGravity;
    if (static_cast<u32>(model - 0xd5) <= 3) {
        params.flags |= 0x200;
    } else if (static_cast<u32>(model - 0xc7) <= 3) {
        params.flags |= 0x200;
        params.field_30 = PartExtra_BlueCoin;
    } else {
        params.replace_fn = PartStolen_Coin;
    }
    params.field_88 = lifetime;
    params.field_40 = PartCollide_3D;
    params.stop_fn = PartStop_Coin;
    params.field_3c = PartImpact_Coin;
    params.time_step = FRAMETIME;
    params.field_c0 = scale;
    PART_s *part = AddPart(&params);
    if (part != NULL) {
        part->field_100 = 30.0f;
        if (params.gravity == 0.0f) {
            part->move_callback = PartMove_VehiclePickup;
            part->field_104 = static_cast<f32>(qrand()) * (1.0f / 65535.0f) + 4.0f;
        }
    }
}

void UpdatePartEmits(f32 time) {
    i32 switch_changes[32][2];
    i32 last_switch_change = -1;
    for (i32 i = 0; i < 40; ++i) {
        part_emit_s *emitter = &part_emits[i];
        if (part_page_on[emitter->page] == 0 || emitter->effect_id == -1)
            continue;
        part_type_s *type = &part_types[emitter->effect_id];
        if (type->effect_ids[0] == -1)
            continue;
        f32 interval = 1.0f / type->emission_rate;
        if (emitter->field_44 == 1 && emitter->field_3d != 2 && emitter->field_46 != -1) {
            i32 state = object_switches[emitter->field_46];
            switch (state) {
                case 0:
                    emitter->field_3d = 0;
                    break;
                case 1:
                    emitter->field_3d = 1;
                    break;
                case 2:
                case 3: {
                    last_switch_change = (last_switch_change + 1) & 31;
                    emitter->field_3d = 2;
                    switch_changes[last_switch_change][0] = emitter->field_46;
                    switch_changes[last_switch_change][1] = state == 2 ? 4 : 0;
                    PARTStartOffset(i, 0.0f);
                    f32 shift = emitter->time_24 - partglobaltime;
                    emitter->time_20 = partglobaltime - interval;
                    emitter->time_24 -= shift;
                    emitter->time_28 -= shift;
                    break;
                }
                case 4:
                    last_switch_change = (last_switch_change + 1) & 31;
                    switch_changes[last_switch_change][0] = emitter->field_46;
                    switch_changes[last_switch_change][1] = 0;
                    break;
            }
        }
        f32 previous_distance = emitter->camera_distance;
        emitter->camera_distance = CameraEmitterDistance(&emitter->position);
        if (emitter->field_3c != 0) {
            for (i32 sound = 0; sound < 4; ++sound) {
                if (type->sounds[sound] != -1) {
                    SetSfxBit_On(type->sounds[sound]);
                    if (type->sound_modes[sound] == 4)
                        PlaySfxById(type->sounds[sound], &emitter->position);
                }
            }
        }
        if ((type->flags & 0x400000) != 0) {
            if (type->maximum_distance > emitter->camera_distance && previous_distance >= type->maximum_distance) {
                emitter->time_24 = partglobaltime;
                emitter->time_28 = partglobaltime - 1.0f;
                emitter->time_20 = partglobaltime - interval + 0.001f;
            } else if (emitter->time_24 > emitter->time_28) {
                emitter->time_28 += time;
                emitter->time_24 += time;
                emitter->time_20 += time;
            }
        }
        f32 emission_time = emitter->time_20 + interval;
        i32 remaining = 101;
        while (partglobaltime + time > emission_time && --remaining != 0) {
            while (emission_time >= emitter->time_28 && emission_time > emitter->time_24 &&
                   partglobaltime + time > emission_time) {
                if (emitter->time_24 >= emitter->time_28) {
                    f32 end = emitter->time_24 + type->emission_period;
                    emitter->time_28 = end + NuRandFloatSeeded(&partseed) * type->emission_period_random;
                    if (emitter->field_3c != 0 && emitter->field_3d != 0) {
                        for (i32 sound = 0; sound < 4; ++sound) {
                            if (type->sounds[sound] != -1 && type->sound_modes[sound] == 1)
                                PlaySfxById(type->sounds[sound], &emitter->position);
                        }
                    }
                    if (emitter->shots_remaining != 0 && --emitter->shots_remaining == 0) {
                        RemovePARTEffect(i);
                        emission_time = partglobaltime + 99999.9f;
                    }
                } else {
                    f32 pause = type->emission_pause;
                    emitter->time_24 =
                        pause + NuRandFloatSeeded(&partseed) * type->emission_pause_random + emitter->time_28;
                    emission_time = emitter->time_24 + 0.001f;
                    emitter->time_20 = emission_time - interval + 0.001f;
                    if (emitter->field_3d == 2)
                        emitter->field_3d = 0;
                    if (emitter->field_3c != 0 && emitter->field_3d != 0) {
                        for (i32 sound = 0; sound < 4; ++sound) {
                            if (type->sounds[sound] != -1 && type->sound_modes[sound] == 2)
                                PlaySfxById(type->sounds[sound], &emitter->position);
                        }
                    }
                }
            }
            if (partglobaltime + time <= emission_time)
                continue;
            if (emitter->field_3d != 0 &&
                (type->maximum_distance == 0.0f || type->maximum_distance > emitter->camera_distance)) {
                ADDPART_s params = Default_ADDPART;
                params.field_c0 = type->particle_scale;
                NUVEC position;
                f32 random = NuRandFloatSeeded(&partseed);
                position.x = (random + random) * type->position_random.x - type->position_random.x;
                random = NuRandFloatSeeded(&partseed);
                position.y = (random + random) * type->position_random.y - type->position_random.y;
                random = NuRandFloatSeeded(&partseed);
                position.z = (random + random) * type->position_random.z - type->position_random.z;
                NuVecRotateZ(&position, &position, emitter->rotation_2c);
                NuVecRotateY(&position, &position, emitter->rotation_2e);
                NuVecRotateX(&position, &position, emitter->rotation_30);
                NuVecAdd(&position, &position, &emitter->position);
                params.position = &position;
                NUVEC velocity = {0.0f, type->speed, 0.0f};
                random = NuRandFloatSeeded(&partseed);
                velocity.x += (random + random) * type->velocity_random.x - type->velocity_random.x;
                random = NuRandFloatSeeded(&partseed);
                velocity.y += (random + random) * type->velocity_random.y - type->velocity_random.y;
                random = NuRandFloatSeeded(&partseed);
                velocity.z += (random + random) * type->velocity_random.z - type->velocity_random.z;
                NuVecRotateZ(&velocity, &velocity, emitter->rotation_2c);
                NuVecRotateY(&velocity, &velocity, emitter->rotation_2e);
                NuVecRotateX(&velocity, &velocity, emitter->rotation_30);
                params.velocity = &velocity;
                params.gravity = type->gravity;
                params.field_20 = type->bounce;
                i32 variant = 0;
                if (type->variant_count != 0) {
                    if (type->variant_mode == 0)
                        variant = static_cast<i32>(NuRandFloatSeeded(&partseed) * 65535.0f) % type->variant_count;
                    else if (type->variant_mode == 1) {
                        i8 next = static_cast<i8>(emitter->field_33 + 1);
                        if (type->variant_count > next)
                            variant = next;
                        emitter->field_33 = variant;
                    }
                }
                params.field_28 = type->effect_ids[variant];
                nuhspecial_s special;
                if (static_cast<u16>(params.field_28 - 9998) <= 1)
                    params.special = NULL;
                else if (params.field_28 != -1) {
                    params.special = &special;
                    NuGScnGetSpecial(&special, part_scene[type->scene_indices[variant]], params.field_28);
                }
                f32 lifetime = type->lifetime;
                params.field_a4 = lifetime + NuRandFloatSeeded(&partseed) * type->lifetime_random;
                NUMTX_ALIGNED16 matrix;
                NuMtxSetIdentity(&matrix);
                f32 rotation_x = static_cast<f32>(type->rotation[0]);
                f32 random_x = NuRandFloatSeeded(&partseed);
                f32 range_x = static_cast<f32>(type->rotation_random[0]);
                f32 rotation_y = static_cast<f32>(type->rotation[1]);
                f32 random_y = NuRandFloatSeeded(&partseed);
                f32 range_y = static_cast<f32>(type->rotation_random[1]);
                f32 rotation_z = static_cast<f32>(type->rotation[2]);
                f32 random_z = NuRandFloatSeeded(&partseed);
                f32 range_z = static_cast<f32>(type->rotation_random[2]);
                for (i32 axis = 0; axis < 3; ++axis) {
                    params.field_a8[axis] = type->rotation[axis];
                    params.field_a8[axis + 3] = type->rotation_random[axis];
                }
                NuMtxRotateX(&matrix, static_cast<i16>(
                                          static_cast<i32>((random_x + random_x) * range_x + rotation_x - range_x)));
                NuMtxRotateY(&matrix, static_cast<i16>(
                                          static_cast<i32>((random_y + random_y) * range_y + rotation_y - range_y)));
                NuMtxRotateZ(&matrix, static_cast<i16>(
                                          static_cast<i32>((random_z + random_z) * range_z + rotation_z - range_z)));
                matrix.m30 = position.x;
                matrix.m31 = position.y;
                matrix.m32 = position.z;
                params.matrix = &matrix;
                params.flags = type->flags & ~0x60000;
                if (type->trail_effects[0] != -1) {
                    params.field_60 = type->trail_effects[0];
                    params.flags |= 0x40000;
                }
                if (type->trail_effects[1] != -1) {
                    params.field_64 = type->trail_effects[1];
                    params.flags |= 0x40000;
                }
                if (type->attached_effect != -1) {
                    params.field_58 = type->attached_effect;
                    params.flags |= 0x20000;
                }
                params.field_68 = type->trail_rates[0];
                params.field_6c = type->trail_rates[1];
                params.field_70 = type->impact_effect;
                if (type->impact_part == -1 && type->impact_part_name[0] != 0)
                    type->impact_part = PARTLookupType(type->impact_part_name);
                params.field_74 = type->impact_part;
                params.field_7c = type->effect_scale;
                params.field_98 = type->field_160;
                params.field_9c = type->field_164;
                params.field_a0 = type->field_168;
                params.field_78 = type->kill_effect;
                AddPart(&params);
                if (emitter->field_3c != 0) {
                    for (i32 sound = 0; sound < 4; ++sound) {
                        if (type->sounds[sound] != -1 && type->sound_modes[sound] == 3)
                            PlaySfxById(type->sounds[sound], &emitter->position);
                    }
                }
                type->last_used_time = emission_time;
            }
            emitter->time_20 = emission_time;
            emission_time += interval;
        }
    }
    for (i32 i = 0; i <= last_switch_change; ++i)
        object_switches[switch_changes[i][0]] = switch_changes[i][1];
}

i32 LineIntersectSphere(NUVEC *, NUVEC *, NUVEC *, f32, f32 *);

PART_s *FindIncomingPart(void *owner, NUVEC *position, f32 radius, u32 flags, f32 range) {
    PART_s *nearest = NULL;
    f32 nearest_distance = range > 0.0f ? range : 100.0f;
    for (i32 i = 0; i < MAXPARTS; ++i) {
        PART_s *part = &Part[i];
        if ((part->active & 1) == 0 || part->owner == owner || (part->flags & flags) != flags)
            continue;
        if (range <= 0.0f) {
            f32 speed = NuVecMag(&part->velocity);
            if (speed != 0.0f) {
                NUVEC direction;
                NuVecScale(&direction, &part->velocity, 1.0f / speed);
                f32 distance = NuVecDistSqr(&part->position, position, NULL);
                if (distance < speed * 0.5f * speed * 0.5f &&
                    LineIntersectSphere(&part->position, &direction, position, radius * radius, NULL)) {
                    nearest_distance = distance;
                    nearest = part;
                }
            }
        } else {
            f32 distance = NuVecDistSqr(&part->position, position, NULL);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest = part;
            }
        }
    }
    return nearest;
}

void InstantKillParts(GameObject_s *, i32, float) {
}

void edpartDestroy(i32 index) {
    CheckPartCount();
    if (part_emits[index].effect_id != -1) {
        part_emits[index].effect_id = -1;
        --part_emits_used;
    }
    CheckPartCount();
}

void PartCleanupTypes() {
    static i32 frame = 0;
    static i32 index = 0;
    if (++frame > 5) {
        frame = 0;
        if (part_types[index].effect_ids[0] != -1 && partglobaltime > part_types[index].last_used_time + 5.0f &&
            part_types[index].scale != 1.0f) {
            for (i32 i = 0; i < 40; ++i) {
                if (part_emits[i].effect_id == index)
                    edpartDestroy(i);
            }
            part_types[index].name[0] = 0;
            part_types[index].effect_ids[0] = -1;
            --part_types_used;
        }
        if (++index >= 128)
            index = 0;
    }
}

extern f32 brickimpactwait;
extern "C" i32 GetSfxId(const char *);
extern "C" void PlaySfxByIdAndSetVolume(i32, NUVEC *, f32);

void PartImpact_Brick(PART_s *part) {
    if (brickimpactwait <= 0.0f) {
        f32 speed = NuVecMag(&part->velocity);
        if (speed > 0.0f) {
            if (speed > 1.0f)
                speed = 1.0f;
            f32 volume = NU_SIN_LUT(static_cast<i32>((1.0f - speed) * 16384.0f + 16384.0f));
            PlaySfxByIdAndSetVolume(GetSfxId("LegoSingle"), &part->position, volume);
            brickimpactwait = 0.1f;
        }
    }
}

void PartUpdate_Heart(PART_s *part) {
    NUVEC position = part->position;
    part->rotation_y += 16384.0f * FRAMETIME;
    NuMtxSetRotationY(&part->transform, part->rotation_y);
    NuMtxTranslate(&part->transform, &position);
}

void Asteroid_PartKill(PART_s *, i32) {
}

void PartStop_Flickerer(PART_s *part) {
    part->field_100 = 5.0f;
    if ((part->field_20a & 1) != 0)
        part->field_100 = 2.5f;
}

i32 PartDraw_Flickerer(PART_s *part) {
    if (part->field_100 > 0.0f) {
        if (PickupFlickerFrame % PickUpFlickerFrames < PickUpFlickerTest)
            return 1;
        f32 threshold = (part->field_20a & 1) != 0 ? 1.0f : 2.0f;
        return part->field_100 >= threshold;
    }
    return 1;
}

void AddPartDebris(PARTDEBSYS_s *, i32, NUVEC *);
void NewRumbleAllPlayers(f32, f32, i32, i32);
void GameCam_NewShake(GAMECAMERA_s *, f32, f32, f32);

void PartKill_ForceThrow(PART_s *part, i32) {
    LEVELDATA *level = WorldInfo_CurrentlyActive()->current_level;
    i32 debris = -1;
    i32 part_debris = -1;
    if (level == CLOUDCITYTRAPB_LDATA) {
        debris = 0x75;
        part_debris = 8;
    } else if (level == MAULA_LDATA) {
        debris = 0x2d;
        part_debris = 9;
    } else if (level == MAULF_LDATA) {
        i32 type = 11;
        if (NuSpecialExistsFn(&LevHSpecial[0]) && NuSpecialCompare(&LevHSpecial[0], &part->special))
            type = 11;
        else if (NuSpecialExistsFn(&LevHSpecial[1]) && NuSpecialCompare(&LevHSpecial[1], &part->special))
            type = 12;
        else if (NuSpecialExistsFn(&LevHSpecial[2]) && NuSpecialCompare(&LevHSpecial[2], &part->special))
            type = 13;
        part_debris = type;
    } else if (level == DOOKUC_LDATA) {
        debris = 0x34;
        part_debris = 10;
    }
    if (debris != -1)
        AddGameDebris(WORLD->debris_sys, debris, &part->position);
    if (part_debris != -1)
        AddPartDebris(WORLD->part_debris_sys, part_debris, &part->position);
    NewRumbleAllPlayers(0.7f, 0.0f, 0, 0);
    GameCam_Judder(GameCam, qrand() > 0x7fff ? -0.4f : 0.4f, 2, NULL);
    GameCam_NewShake(GameCam, 0.6f, 0.6f, 1.0f);
    PlaySfx("Explode1", &part->position);
}

void PartImpact_Basketball(PART_s *) {
}

void PartUpdate_Basketball(PART_s *) {
}

PART_s *Part_FindFromHSpecial(nuhspecial_s *special) {
    if (special != NULL) {
        for (i32 i = 0; i < MAXPARTS; ++i) {
            if ((Part[i].active & 1) != 0 && NuSpecialCompare(&Part[i].special, special))
                return &Part[i];
        }
    }
    return NULL;
}

void NewPartOrderedRotation(PART_s *part) {
    f32 range = static_cast<i32>(part->field_10c[3]);
    i32 base = part->field_10c[0];
    part->field_124[0] = base + static_cast<i32>((NuRandFloatSeeded(&partseed) * 2.0f - 1.0f) * range);
    range = static_cast<i32>(part->field_10c[4]);
    base = part->field_10c[1];
    part->field_124[1] = base + static_cast<i32>((NuRandFloatSeeded(&partseed) * 2.0f - 1.0f) * range);
    range = static_cast<i32>(part->field_10c[5]);
    base = part->field_10c[2];
    part->field_124[2] = base + static_cast<i32>((NuRandFloatSeeded(&partseed) * 2.0f - 1.0f) * range);
}

void KillParts(GameObject_s *, i32, i32, i32, float, i32, u16 *) {
}
