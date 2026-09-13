#include "gamelib_util_types.h"

#include "globals.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nucore/nuvuvec.hpp"
#include "legoapi/render/light/shadow.h"

f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
i32 SuperWeirdo(GameObject_s *);
i32 GizForce_StoodOnForce(GIZFORCE_s *, GameObject_s *);

NUCOLOUR3 flashCol = {2.0f, 2.0f, 2.0f};
bool TouchHacks::TouchControlsActive;
extern i32 BonusArea;
extern "C" i16 id_GRABCONTROL, id_WICKET, id_EWOK;
extern "C" i16 id_ATST, id_ATST_LOWRES;
extern "C" i16 id_WATTO, id_GONKDROID;

bool TouchHacks::AiPlayerTakeDamageOnKillRescue(GameObject_s &) {
    return TouchControlsActive;
}

void TouchHacks::CalculateJumpVelToHitPoint(GameObject_s &, VuVec const &) {
}

void TouchHacks::CalculateJumpVelToHitPointDblJump(GameObject_s &, VuVec const &) {
}

void TouchHacks::CalculateXZVelForArcToHitPoint(VuVec const &, VuVec const &, float, float) {
}

i32 TouchHacks::CanBlowupBeBlownUp(GIZMOBLOWUP_s &blowup, i32 hit_type) {
    if (hit_type != 1) {
        return 1;
    }
    return (blowup.draw_flags >> 7) & 1;
}

void TouchHacks::CanForceTargetObj(GameObject_s &, GameObject_s &) {
}

bool TouchHacks::CanJump(GameObject_s &object) {
    return (object.apiobj.field_0x27d != 0 || object.ground_contact_grace_timer > 0.0f) &&
           object.apiobj.character_model != NULL && ObjLandReady(&object) &&
           (object.apiobj.character_model->model_data_b[6] != NULL ||
            (object.apiobj.character_data->model_flags & 0x40) != 0 || object.id == id_WATTO ||
            (object.id == id_GONKDROID && Cheat_IsOn(8)));
}

void TouchHacks::CanJumpToPoint(GameObject_s &, AIPATHNODE_s const &) {
}

void TouchHacks::CanJumpToPoint(GameObject_s &, VuVec const &) {
}

bool TouchHacks::CanLunge(GameObject_s &object) {
    CHARACTERDATA *character = object.apiobj.character_data;
    return (character->game_character->field275_0x116 != 0 || (character->model_flags & 8) != 0) &&
           LEGOACT_LUNGE != -1 && object.apiobj.character_model->model_data_b[LEGOACT_LUNGE] != NULL;
}

bool TouchHacks::CanPoo(GameObject_s &object) {
    return (object.apiobj.character_data->game_character->flags_094[3] & 0x80) != 0 && object.character_context == -1 &&
           object.apiobj.field_0x27d != 0 && (object.apiobj.flags_low & 0x80) != 0 &&
           (Cheat[1].enabled != 0 || Cheat[9].enabled != 0);
}

bool TouchHacks::CanShoot(GameObject_s &object) {
    CHARACTERDATA *character = object.apiobj.character_data;
    return (character->model_flags & 0x10000000) != 0 && (character->game_character->flags_094[0] & 8) == 0;
}

bool TouchHacks::CanSlam(GameObject_s &object) {
    return LEGOACT_SLAM != -1 && object.apiobj.character_model->model_data_b[LEGOACT_SLAM] != NULL;
}

void TouchHacks::CanTagTo(GameObject_s &, GameObject_s &) {
}

void Move_CHARACTER(GameObject_s *);
void Move_WEIRDO(GameObject_s *);
void Move_JEDI(GameObject_s *);
void Move_DROIDGENERIC(GameObject_s *);
void Move_JAWA(GameObject_s *);
void Move_GEONOSIAN(GameObject_s *);
extern i16 id_SKELETON, id_GRIEVOUS, id_BODYGUARD, id_ATAT, id_STAP, id_STAP2;

bool TouchHacks::CanTagVehicle(GameObject_s &object, GameObject_s &vehicle) {
    CHARACTERDATA *character = object.apiobj.character_data;
    if (character->move_fn != Move_CHARACTER && character->move_fn != Move_WEIRDO && character->move_fn != Move_JEDI &&
        character->move_fn != Move_DROIDGENERIC && character->move_fn != Move_JAWA &&
        character->move_fn != Move_GEONOSIAN) {
        return false;
    }
    if ((vehicle.apiobj.character_data->model_flags & 0x40000000) != 0 &&
        (vehicle.character_context == 0x17 || vehicle.character_context == 0x3e)) {
        return false;
    }
    if (vehicle.field_0xcc0 != NULL || (character->model_flags & 0x10) != 0 || object.id == id_SKELETON ||
        object.id == id_GRIEVOUS || object.id == id_BODYGUARD) {
        return false;
    }
    if (vehicle.id != id_ATST && vehicle.id != id_ATST_LOWRES && vehicle.id != id_ATAT && vehicle.id != id_STAP &&
        vehicle.id != id_STAP2 &&
        fabsf(vehicle.apiobj.position.y - object.apiobj.position.y) > object.apiobj.scaled_height) {
        return false;
    }
    const f32 x = object.apiobj.position.x - vehicle.apiobj.position.x;
    const f32 y = object.apiobj.position.y - vehicle.apiobj.position.y;
    const f32 z = object.apiobj.position.z - vehicle.apiobj.position.z;
    return !(x * x + y * y + z * z > 4.0f);
}

void TouchHacks::CanThrowBountyBomb(GameObject_s &) {
}

void Move_DEFAULT(GameObject_s *);

bool TouchHacks::CanToggleTo(GameObject_s &object, i32 id) {
    if (object.id == id)
        return false;
    if ((CInfo[object.character_context].flags & 0x100) != 0)
        return false;
    if (object.apiobj.field_0x27f <= 16 && (TerLayer[static_cast<i8>(object.apiobj.field_0x27f)].flags & 1) != 0 &&
        GCDataList[id].field_0x28 <= 0.0f)
        return false;
    if (object.apiobj.field_0x218 != 2000000.0f && object.apiobj.field_0x220 != 2000000.0f &&
        object.apiobj.character_data->move_fn != Move_DEFAULT &&
        CDataList[id].bounds_max_y - CDataList[id].bounds_min_y >=
            object.apiobj.field_0x220 - object.apiobj.field_0x218)
        return false;
    return true;
}

bool TouchHacks::CanUseBuildIt(GameObject_s &object) {
    return LEGOACT_BUILD != -1 && object.apiobj.character_model != NULL &&
           object.apiobj.character_model->model_data_b[LEGOACT_BUILD] != NULL &&
           !AnimPlaying(&object.apiobj.anim_packet, LEGOACT_BUILD, 1, 1) && object.apiobj.field_0x27d != 0 &&
           ObjLandReady(&object) != 0;
}

bool TouchHacks::CanUseGizForce(GameObject_s &object) {
    return object.apiobj.character_data != NULL && (object.apiobj.character_data->model_flags & 8) != 0;
}

bool TouchHacks::CanUseGizForce(GameObject_s &object, GIZFORCE_s &force) {
    if (force.using_object != NULL || force.field_0x3c_bits != 0 ||
        (force.state_flags & GIZFORCE_STATE_DESTROYED_OR_THROWN) != 0) {
        return false;
    }

    i32 can_use_restricted_force;
    if (SuperWeirdo(&object) == 0) {
        if (static_cast<i8>(object.apiobj.flags_low) < 0 && Cheat_IsOn(25) != 0) {
            can_use_restricted_force = 1;
        } else {
            can_use_restricted_force = 0;
        }
    } else {
        can_use_restricted_force = 1;
    }
    if ((force.config_flags & GIZFORCE_CONFIG_JEDI_BADDIE_ONLY) != 0 &&
        (object.apiobj.character_data->model_flags & 4) == 0 && can_use_restricted_force == 0) {
        return false;
    }

    if (force.group == NULL) {
        if (GizForce_Complete(&force) != 0) {
            return false;
        }
    } else if (force.group->count != 0) {
        GIZFORCE_s *last = force.group->forces[force.group->count - 1];
        if (last != &force) {
            if ((force.group->field_0x24 & GIZFORCE_GROUP_ACTIVE) != 0 ||
                force.anim_set->state == GAMEANIMSET_STATE_AT_END) {
                return false;
            }
            if (last != NULL && ((last->anim_set->flags & 7) != 0 || last->using_object != NULL)) {
                return false;
            }
        }
    }

    return GizForce_StoodOnForce(&force, &object) == 0;
}

bool TouchHacks::CanUseHatMachine(GameObject_s &object) {
    return object.apiobj.character_model->model_data_b[93] != NULL && object.apiobj.field_0x27d != 0 &&
           ObjLandReady(&object) != 0;
}

bool TouchHacks::CanUseLever(GameObject_s &object) {
    return object.apiobj.character_model->model_data_b[93] != NULL && object.apiobj.field_0x27d != 0;
}

bool TouchHacks::CanUseTeleport(GameObject_s &object) {
    return object.apiobj.character_data != NULL &&
           ((object.apiobj.character_data->model_flags & 0x40000) != 0 || SuperWeirdo(&object));
}

bool TouchHacks::CanUseVehicleSmartBomb(GameObject_s &object) {
    return Cheat_IsOn(20) && (object.apiobj.flags_low & 0x80) != 0 &&
           (object.apiobj.character_data->model_flags & 0x2000) != 0 && InCollectList_Index(object.id, NULL, 0) != -1;
}

bool TouchHacks::CanUseZipup(GameObject_s &object) {
    extern i32 ObjLandReady(GameObject_s *);
    extern i32 SuperWeirdo(GameObject_s *);
    extern i32 Cheat_IsOn(i32);
    if (object.apiobj.character_data == NULL || !ObjLandReady(&object))
        return false;
    if ((object.apiobj.character_data->model_flags & 0x100000) != 0 || SuperWeirdo(&object))
        return true;
    return (object.apiobj.character_data->model_flags & 8) != 0 &&
           (object.apiobj.character_data->game_character->flags_094[1] & 0x80) == 0 && Cheat_IsOn(13) != 0;
}

bool TouchHacks::CheckForAboutToRunIntoKillTerrain(GameObject_s &object, float time) {
    if (WORLD->current_level != SPEEDERCHASEA_LDATA) {
        const f32 dx = object.apiobj.velocity.x * time;
        const f32 dz = time * object.apiobj.velocity.z;
        VuVec position(object.apiobj.position.x, object.apiobj.position.y, object.apiobj.position.z, 1.0f);
        position.x = dx + position.x;
        position.z = dz + position.z;
        position.y += 0.3f;
        if (GameShadow(&object, reinterpret_cast<NUVEC *>(&position), 5.0f, -1) == 2000000.0f)
            return false;
        u32 layer = EShadowInfo();
        if (layer > 16 || (TerLayer[layer].flags & 1) == 0)
            return false;
        VuVec direction(object.apiobj.velocity.x, 0.0f, object.apiobj.velocity.z, 1.0f);
        NuVecNorm(reinterpret_cast<NUVEC *>(&direction), reinterpret_cast<NUVEC *>(&direction));
        const f32 radius = object.apiobj.collision_radius * 0.8f;
        position.x += direction.x * radius;
        position.z += radius * direction.z;
        if (GameShadow(&object, reinterpret_cast<NUVEC *>(&position), 5.0f, -1) == 0.0f)
            return true;
        layer = EShadowInfo();
        return layer > 16 || (TerLayer[layer].flags & 1) != 0;
    }
    return false;
}

void TouchHacks::CheckForAboutToRunOffAnEdge(GameObject_s &, float) {
}

void TouchHacks::CheckJumpForLandingSpot(GameObject_s &, float) {
}

void TouchHacks::CleanupAllMechObjectInterfaces(WORLDINFO_s *) {
}

void TouchHacks::FindBombTarget(GameObject_s &) {
}

nucolour3_s *TouchHacks::GetFlashColour() {
    return &flashCol;
}

f32 TouchHacks::GetIncomingPartRange() {
    return 16.0f;
}

i32 TouchHacks::GetLoseStudsDieValue() {
    return BonusArea != 0 ? 10000 : 1000;
}

i32 TouchHacks::GetLoseStudsFallValue() {
    return 0;
}

bool TouchHacks::InParty(GameObject_s &object) {
    for (i32 index = 0; index < 8; ++index) {
        if (Player[index] == &object) {
            return true;
        }
    }
    return false;
}

void TouchHacks::PlaySmartBombBuildupEffects(GameObject_s &, float, float) {
}

bool TouchHacks::ShouldAutoGrabDragBomb(GameObject_s &object) {
    if (object.id == id_ATST || object.id == id_ATST_LOWRES)
        return false;
    return TouchControlsActive;
}

bool TouchHacks::ShouldBlock(GameObject_s &object) {
    if (TouchControlsActive && object.incoming_melee != NULL && object.apiobj.field_0x27c == -1) {
        return qrand() > 14999;
    }
    return true;
}

CABLE_s *GameObjIsCableTied(GameObject_s *);
i32 TouchHacks::ShouldDeflectBolt(GameObject_s &object, BOLT_s &bolt) {
    if (!TouchControlsActive || VehicleArea == 0)
        return 0;
    if (object.id != id_ATST && object.id != id_ATST_LOWRES)
        return 0;
    if (bolt.owner == NULL || (bolt.owner->apiobj.flags_low & 0x80) == 0)
        return 0;
    CABLE_s *cable = GameObjIsCableTied(&object);
    if (cable == NULL)
        return 0;
    return cable->source == bolt.owner;
}

bool TouchHacks::ShouldFlash(float timer) {
    return timer > 0.0f && NuFmod(timer, 0.3f) < 0.15f;
}

bool TouchHacks::ShouldKeepWeaponOut(GameObject_s &object) {
    return TouchControlsActive && object.id != id_GRABCONTROL && (object.apiobj.flags_low & 0x80) != 0 &&
           object.ai.opponent != NULL && object.character_context == -1;
}

bool TouchHacks::ShouldPutWeaponAway(GameObject_s &object) {
    return TouchControlsActive && object.id != id_GRABCONTROL && object.id != id_WICKET && object.id != id_EWOK &&
           (object.apiobj.flags_low & 0x80) != 0 && object.ai.opponent == NULL && object.weapon_out_timer > 5.0f &&
           object.character_context == -1 && object.field_0xe31 != 1;
}

bool TouchHacks::SolveRoot(float a, float b, float c, float &root1, float &root2) {
    if (a == 0.0f) {
        return false;
    }

    const f32 discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    const f32 square_root = NuFsqrt(discriminant);
    const f32 denominator = a + a;
    root1 = (-b - square_root) / denominator;
    root2 = (square_root - b) / denominator;
    return true;
}

void TouchHacks::TriggerVehicleSmartBomb(GameObject_s &) {
}
