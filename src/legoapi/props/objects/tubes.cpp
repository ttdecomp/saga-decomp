#include "decomp.h"
#include "globals.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/motion.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" void AddVariableShotDebrisEffectTimed1(i32 effect, NUVEC *position, i32 count, f32 time, i16 z_rotation,
                                                  i16 y_rotation, NUMTX *orientation);
i32 ObjHitObj(GameObject_s *attacker, GameObject_s *target, i32 damage, u16 flags, i32 parameter, i32 context);
void NewRumble(nupad_s *pad, f32 strength, i32 mode);
void NewBuzz(nupad_s *pad, f32 duration, i32 mode);
void GetRotationAngles(NUVEC *direction, u16 *z_rotation, u16 *y_rotation);
void DisorientateCode(GameObject_s *object, NUVEC *limit, f32 range);

void TractorBeamCode(GameObject_s *object) {
    if (object == NULL || WORLD->area == NULL || (WORLD->area->flags & 1) == 0)
        return;
    if (static_cast<i8>(object->apiobj.flags_low) >= 0)
        return;
    if (Cheat[29].enabled == 0 && !(object->field_0xdec > 0.0f))
        return;
    if ((object->apiobj.character_data->model_flags & 0x2000) == 0)
        return;

    const f32 range = object->apiobj.field_0x1dc * 10.0f;
    const f32 range_squared = range * range;
    for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
        GameObject_s *candidate = &Obj[index];
        if (candidate == NULL || (candidate->apiobj.field_0x1f8 & 0x1001) != 0x1001 ||
            (candidate->apiobj.character_data->model_flags & 4) == 0 || (candidate->field_0xf04 & 4) != 0 ||
            (candidate->field_0xefb & 8) != 0 || candidate->apiobj.field_0x27c != -1 ||
            candidate->apiobj.field_0x27d != 0) {
            continue;
        }

        NUVEC direction;
        const f32 distance =
            NuVecDistSqr(&candidate->apiobj.collision_position, &object->apiobj.collision_position, &direction);
        const f32 collision_range = object->apiobj.field_0x1dc + candidate->apiobj.field_0x1dc;
        if (distance <= collision_range * collision_range) {
            ObjHitObj(NULL, candidate, -1, 0, 0, 1);
            NewRumble(object->pad_gamepad->pad, 0.5f, 0);
            NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
            continue;
        }
        if (!(range_squared > distance))
            continue;

        u16 z_rotation;
        u16 y_rotation;
        GetRotationAngles(&direction, &z_rotation, &y_rotation);
        AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[128].effect, &object->apiobj.position, 90,
                                          FRAMETIME, z_rotation, y_rotation, NULL);

        const f32 seek_rate = (1.0f + NU_SIN_LUT((1.0f - distance / range_squared) * 16384.0f + 49152.0f)) * 25.0f;
        NuVecNeg(&direction, &direction);
        NuVecAdd(&direction, &direction, &object->apiobj.velocity);
        SeekVec(&candidate->apiobj.velocity, &candidate->apiobj.velocity, &direction, seek_rate);

        NUVEC limit = {1.0e9f, 1.0e9f, 1.0e9f};
        DisorientateCode(candidate, &limit, range);
    }

    PlaySfx(const_cast<char *>("env_tractorbeam_lp"), &object->apiobj.collision_position);
}

extern i32 LEGOCONTEXT_TUBE;
