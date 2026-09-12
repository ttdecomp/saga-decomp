#include "decomp.h"
#include "globals.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "legoapi/render/fx/parts.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void AddPartDebris(PARTDEBSYS_s *, i32, nuvec_s *);
EXPLOSION *AddExplosion(nuvec_s *, f32, f32, GameObject_s *, i32, i32);
void NewRumbleAllPlayers(f32, f32, i32, i32);
void GameCam_NewShake(GAMECAMERA_s *, f32, f32, f32);
void GameCam_Judder(GAMECAMERA_s *, f32, i32, nuvec_s *);
void AlertSurroundingCreatures(GameObject_s *, NUVEC *);
i32 qrand();
EXPLOSION *Detonate(nuvec_s *position, u16 flags);
i32 MatrixReflection(NUMTX *matrix, i32 axis, f32 plane, f32 height, NUMTX *result);

void ThermalDetonator_Throw(GameObject_s *) {
}

i32 PartDraw_ThermalDetonator(PART_s *part) {
    const i32 draw = PartDraw_Flickerer(part);
    NUMTX matrix = part->transform;

    NUMTX reflection;
    i32 reflected = 0;
    if ((part->reflection_flags & 2) != 0 && part->reflection_height != 0.0f) {
        reflected =
            MatrixReflection(&matrix, 2, part->reflection_height, WORLD->current_level->unknown_0cc, &reflection);
    }

    LEVEL_OBJECT_RUNTIME *level_special = NULL;
    if (WORLD->lev_objs[0xea].active != 0 && WORLD->lev_objs[0xeb].active != 0) {
        const i32 index = draw == 1 ? 0xea : 0xeb;
        level_special = &WORLD->lev_objs[index];
        NuSpecialDrawAt(&level_special->special, &matrix);
    }

    if (reflected != 0) {
        NuRndrStartReflectionRender(1);
        if (part->source_special != NULL) {
            NuSpecialDrawAt(&part->special, &reflection);
        }
        if (level_special != NULL) {
            NuSpecialDrawAt(&level_special->special, &reflection);
        }
        NuRndrEndReflectionRender();
    }
    return 1;
}

void PartKill_ThermalDetonator(PART_s *part, i32) {
    if (part == NULL) {
        return;
    }

    if (part->owner != NULL) {
        AlertSurroundingCreatures(part->owner, &part->position);
    }

    u16 flags = 0x200;
    if (part->owner != NULL && static_cast<i8>(part->owner->apiobj.flags_low) < 0 && Cheat_IsOn(0x17) != 0) {
        flags = 0x1200;
    }

    EXPLOSION *explosion = Detonate(&part->position, flags);
    if (explosion != NULL && part->owner != NULL && static_cast<u8>(part->owner->apiobj.field_0x27c) <= 1) {
        explosion->field_0x33 = static_cast<u8>(part->owner->apiobj.field_0x27c);
    }
}

i32 ThermalDetonator_MoveCode(GameObject_s *) {
    return 0;
}

void ThermalDetonator_ThrowMom(GameObject_s *, nuvec_s *) {
}

void PartImpact_ThermalDetonator(PART_s *) {
}

void PartUpdate_ThermalDetonator(PART_s *) {
}

EXPLOSION *Detonate(nuvec_s *position, u16 flags) {
    AddGameDebris(WORLD->debris_sys, 0x49, position);
    AddGameDebris(WORLD->debris_sys, 0x4a, position);
    AddGameDebris(WORLD->debris_sys, 0x4b, position);
    AddPartDebris(WORLD->part_debris_sys, 2, position);
    NewRumbleAllPlayers(1.0f, 0.1f, 0, 0);
    PlaySfx((char *)"exp_thermalDet", position);
    f32 amount_a;
    f32 amount_b;
    if ((flags & 0x1000) != 0) {
        f32 amount = qrand() < 0x8000 ? -2.0f : 2.0f;
        GameCam_Judder(GameCam, amount, 2, position);
        GameCam_NewShake(GameCam, 2.0f, 1.0f, 1.0f);
        PlaySfx((char *)"exp_thermalDet", position);
        amount_a = 1.3125f;
        amount_b = 0.875f;
    } else {
        GameCam_NewShake(GameCam, 1.0f, 1.0f, 1.0f);
        amount_a = 0.75f;
        amount_b = 0.5f;
    }
    return AddExplosion(position, amount_a, amount_b, NULL, -1, (flags & 0xffff) | 0x67);
}
