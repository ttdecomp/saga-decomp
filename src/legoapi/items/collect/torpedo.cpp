#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nutex.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/world.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "globals.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" void AddVariableShotDebrisEffectTimed1(i32 effect, NUVEC *position, i32 count, f32 time, i16 z_rotation,
                                                  i16 y_rotation, NUMTX *orientation);
void AddPickups(i32, i32, i32, i32, NUVEC *, NUVEC *, f32, i32, f32, f32, GameObject_s *, i32, i32, bool);
i32 ObjHitObj(GameObject_s *attacker, GameObject_s *target, i32 damage, u16 flags, i32 param, i32 context);
void NewBuzzFrames(nupad_s *pad, i32 frames, i32 mode);
extern AREADATA_s *PODSPRINT_ADATA;

i32 PODSPRINTDEB = 117;

static TORPEDOPACKET TorpedoPackets[16];

void DropTorpedoPickups(TORPEDOPACKET_s *packet, i32 count) {
    if (count == 0 || packet == NULL)
        return;
    if (count > packet->count)
        count = packet->count;
    if (count < 0)
        return;
    for (i32 index = 1; index <= count; ++index) {
        AddPickups(0, 0, 1, 0, &packet->pickup_positions[packet->count - index], &v010, 5.0f, -1, 1.0f, 2000000.0f,
                   NULL, 1, 0, true);
    }
    packet->count -= count;
}

extern i16 id_YWING, id_MINIYWING, id_TIEBOMBER, id_MINITIEBOMBER, id_MINISTARDESTROYER;

i32 getMaxTorpedos(GameObject_s *object) {
    if (object != NULL && (object->id == id_YWING || object->id == id_MINIYWING || object->id == id_TIEBOMBER ||
                           object->id == id_MINITIEBOMBER || object->id == id_MINISTARDESTROYER))
        return 5;
    return 3;
}

void PodCollisionCode(GameObject_s *object) {
    static f32 magdif;

    if (GamePlayTimer.time_elapsed < 1.0f) {
        return;
    }
    if (object->apiobj.field_0x27c == -1) {
        return;
    }
    if (static_cast<i8>(object->apiobj.flags_low) >= 0) {
        return;
    }
    if (object->field_0x1084 == 0) {
        return;
    }
    if (object->contact_normal.y > 0.574f || object->contact_normal.y < -0.574f) {
        return;
    }

    if (PODSPRINT_ADATA != NULL) {
        if (WORLD->area == PODSPRINT_ADATA) {
            AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[PODSPRINTDEB].effect,
                                              &object->contact_position, 50, FRAMETIME, 0, 0, NULL);
            if (qrand() <= 0x7fff) {
                NewBuzzFrames(object->pad_gamepad->pad, 1, 0);
            }
            return;
        }
    }

    magdif = (1.0f / object->pre_terrain_speed) * object->post_terrain_speed;
    if (1.0f > magdif) {
        if (0.999f > magdif) {
            ObjHitObj(NULL, object, 1, 0, 0, 1);
        }
        PodLoseSpeed(object, 0, 0);
    }
}

TORPEDOPACKET *GetTorpedoPacket(void) {
    for (i32 i = 0; i < 16; ++i) {
        if ((TorpedoPackets[i].field_0x1 & 1) == 0) {
            TorpedoPackets[i].field_0x1 |= 1;
            return &TorpedoPackets[i];
        }
    }
    return NULL;
}

void FreeTorpedoPacket(TORPEDOPACKET_s **packet) {
    if (packet == NULL || *packet == NULL) {
        return;
    }
    for (i32 i = 0; i < 16; ++i) {
        if (*packet == &TorpedoPackets[i] && (TorpedoPackets[i].field_0x1 & 1) != 0) {
            memset(*packet, 0, sizeof(TORPEDOPACKET));
            *packet = NULL;
            return;
        }
    }
}

extern i16 id_ATAT;
void GetShootOrigin_LSW(GameObject_s *object, nuvec_s *position) {
    *position = object->apiobj.collision_position;
    if (object->id == id_ATAT) {
        u16 angle = object->apiobj.field_0x276;
        f32 scale = object->apiobj.field_0x1dc;
        position->x += (NU_SIN_LUT(angle) * scale) * 1.5f;
        position->z += (scale * NU_COS_LUT(angle)) * 1.5f;
    }
}

void InitTorpedoPackets() {
    memset(TorpedoPackets, 0, sizeof(TorpedoPackets));
}

extern i16 id_ATAT, id_CLONEWALKER, id_ATST, id_ATST_LOWRES;
i32 GetShootDirection_LSW(GameObject_s *object, nuvec_s *direction) {
    NUVEC temporary;
    if (direction == NULL)
        direction = &temporary;
    if (object->field_0x1086 == 4) {
        NUMTX matrix = object->apiobj.field_0xb8;
        NuMtxPreRotateY(&matrix, 0x8000);
        NuVecMtxRotate(direction, &v001, &matrix);
        return object->apiobj.facing_angle;
    }
    characterdata_s *model = object->apiobj.character_data;
    GAMECHARACTERDATA *data = model->game_character;
    if (data->weapon_shoot_joints[0] != -1 &&
        ((object->id == id_ATAT && (object->apiobj.flags_low & 0x80) == 0) || object->id == id_CLONEWALKER ||
         object->id == id_ATST || object->id == id_ATST_LOWRES)) {
        direction->x = direction->y = 0.0f;
        direction->z = object->id == id_ATAT ? 1.0f : -1.0f;
        NuVecMtxRotate(direction, direction, &object->joint_matrices[data->weapon_shoot_joints[0]]);
        return NuAtan2D(direction->x, direction->z);
    }
    i32 angle;
    if ((model->model_flags & 0x2000) != 0 || (data->flags_090 & 0x80) != 0) {
        angle = object->apiobj.facing_angle;
        if (object->character_context == 0x2a &&
            1.0f - object->context_animation_timer / object->airborne_action_duration >= 0.25f)
            angle -= 0x8000;
    } else
        angle = object->apiobj.movement_facing_angle;
    direction->x = NuTrigTable[static_cast<u16>(angle) >> 1];
    direction->y = 0.0f;
    direction->z = NuTrigTable[((static_cast<u16>(angle) + 0x4000) >> 1) & 0x7fff];
    return angle;
}
