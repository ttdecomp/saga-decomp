#include "legoapi/gizmos/fx/gizmopickups.h"
#include "nu2api/nucore/nustring.h"

#include "batman.h"
#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "globals.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/misc/utilities.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/mission.h"
#include "legoapi/world/world.h"

f32 COINMAGNETSCALE = 3.0f;
f32 COINMSGTIME = 1.0f;
f32 (*GizmoPickups_Collide2DFn)(GameObject_s *) = NULL;
static GIZMOPICKUPSYS_s *GizmoPickupSys = &GizmoPickupSys_Game;

static GIZMOPICKUP_s *GizmoPickups_CollideList(GameObject_s *, GIZMOPICKUP_s *, i32);

void GizmoPickup_InBox(WORLDINFO_s *, i32, nuvec_s *, nuvec_s *) {
}

GIZMOPICKUP_s *GizmoPickups_Collide(WORLDINFO_s *world, GameObject_s *object, i32) {
    if (world == NULL || (object->apiobj.field_0x1f4 & 0x00040000) != 0) {
        return NULL;
    }

    GIZMOPICKUP_s *pickup = GizmoPickups_CollideList(object, world->gizmo_pickup_sys->temporary_pickups, 64);
    if (pickup != NULL) {
        pickup->state_flags =
            static_cast<u8>((pickup->state_flags | GIZMOPICKUP_STATE_COLLECTED) & ~GIZMOPICKUP_STATE_ACTIVE);
    } else {
        pickup =
            GizmoPickups_CollideList(object, world->gizmo_pickup_sys->pickups, world->gizmo_pickup_sys->pickup_count);
        if (pickup == NULL) {
            return NULL;
        }
        pickup->state_flags |= GIZMOPICKUP_STATE_COLLECTED;
    }

    i32 type_index = pickup->type_index;
    if ((pickup->state_flags & GIZMOPICKUP_STATE_ALTERNATE_TYPE) != 0 && GizmoPickupSys->alternate_type != -1) {
        type_index = GizmoPickupSys->alternate_type;
    }
    GIZMO_PICKUP_TYPE *type = &GizmoPickupSys->types[type_index];
    if (type->collection_sfx_name != NULL) {
        PlaySfx(type->collection_sfx_name, &pickup->position);
    }
    if (type->collect_fn != NULL) {
        type->collect_fn(world, pickup, type_index, object, 0);
    }
    if ((type->flags & GIZMOPICKUP_TYPE_FLAG_40) != 0) {
        ++AreaGlobals.values.field_0x18;
    }
    return pickup;
}

void GizmoPickup_FindNearest(WORLDINFO_s *, nuvec_s *, float *) {
}

void GizmoPickup_NumberOfType(WORLDINFO_s *, i32, char) {
}

void GizmoPickup_TurnOnPickup(GIZMOPICKUP_s *pickup) {
    if (pickup != NULL) {
        pickup->state_flags |= GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE | GIZMOPICKUP_STATE_ACTIVATED;
    }
}

GIZMOPICKUP_s *GizmoPickup_FindByName(WORLDINFO_s *world, char *name) {
    if (name != NULL && world != NULL) {
        GIZMOPICKUP_s *pickup = world->gizmo_pickup_sys->pickups;
        if (pickup != NULL) {
            for (i32 index = 0; index < world->gizmo_pickup_sys->pickup_count; ++index, ++pickup) {
                if (NuStrICmp(pickup->name, name) == 0)
                    return pickup;
            }
        }
    }
    return NULL;
}

i32 GizmoPickup_BeenTurnedOn(GIZMOPICKUP_s *pickup) {
    return pickup != NULL ? pickup->state_activated : 0;
}

u32 GizmoPickups_TotalScore(void *world) {
    GIZMOPICKUPRUNTIMESYS_s *system = static_cast<WORLDINFO_s *>(world)->gizmo_pickup_sys;
    GIZMOPICKUP_s *pickup = system->pickups;
    u32 score = 0;
    if (pickup != NULL) {
        for (i32 i = 0; i < system->pickup_count; ++i, ++pickup)
            score += GizmoPickupSys->types[pickup->type_index].score;
    }
    return score;
}
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>

extern "C" {
    i32 NuPortalWhichRoom(NUGSCN *scene, NUVEC *position);
    void NewTerrPlatformsOff(void);
}

extern FadeSystem FadeSys;
extern i32 BonusArea;
extern i32 TimingBarSet;

f32 GameShadow(GameObject_s *object, NUVEC *position, f32 probe_height, i32 terrain_mask);
void FindAnglesZX(NUVEC *normal, u16 *x_rotation, u16 *z_rotation);
void SetAreaPickupGravity(i32 area, i32 level);
void SuperCounter_ActivateGizmoPickup(GIZMO_s *gizmo, GIZMOPICKUP_s *pickup);
void SuperCounters_ResetProcessed(WORLDINFO_s *world);
void MiniKitDetector(NUVEC *position);

namespace {

    enum : i32 {
        GIZMOPICKUP_PROGRESS_CAPACITY = 512,
        GIZMOPICKUP_PROGRESS_WORDS = GIZMOPICKUP_PROGRESS_CAPACITY / 32,
        GIZMOPICKUP_TEMPORARY_CAPACITY = 64,
    };

    struct GIZMOPICKUPPROGRESS_s {
        u32 collected[GIZMOPICKUP_PROGRESS_WORDS];
        u32 enabled[GIZMOPICKUP_PROGRESS_WORDS];
        u32 visible[GIZMOPICKUP_PROGRESS_WORDS];
        u32 activated[GIZMOPICKUP_PROGRESS_WORDS];
    };
    DECOMP_ASSERT(sizeof(GIZMOPICKUPPROGRESS_s) == 0x100, "GIZMOPICKUP progress ABI");

    GIZMO_PICKUP_TYPE *GetPickupType(const GIZMOPICKUP_s &pickup) {
        i32 type_index = pickup.type_index;
        if ((pickup.state_flags & GIZMOPICKUP_STATE_ALTERNATE_TYPE) != 0 && GizmoPickupSys->alternate_type >= 0) {
            type_index = GizmoPickupSys->alternate_type;
        }
        if (type_index < 0 || type_index >= GizmoPickupSys->type_count) {
            type_index = 0;
        }
        return &GizmoPickupSys->types[type_index];
    }

    i32 FindPickupTypeIndex(char type_code) {
        for (i32 index = 0; index < GizmoPickupSys->type_count; ++index) {
            if (GizmoPickupSys->types[index].type_code == type_code) {
                return index;
            }
        }
        return 0;
    }

    f32 GetAreaPickupScale(const WORLDINFO *world) {
        if (world == NULL || world->area == NULL) {
            return 1.0f;
        }
        if ((world->area->flags & AREAFLAG_NOPICKUPGRAVITY) != 0) {
            return 6.0f;
        }
        if ((world->area->flags & AREAFLAG_VEHICLE_AREA) != 0) {
            return BonusArea == 0 ? 5.0f : 3.0f;
        }
        return 1.0f;
    }

    void ClearPickupProgress(GIZMOPICKUPPROGRESS_s *progress) {
        if (progress == NULL) {
            return;
        }
        memset(progress->visible, 0xff, sizeof(progress->visible));
        memset(progress->enabled, 0xff, sizeof(progress->enabled));
        memset(progress->collected, 0, sizeof(progress->collected));
        memset(progress->activated, 0, sizeof(progress->activated));
    }

    void DrawPickupList(WORLDINFO *world, GIZMOPICKUP_s *pickups, i32 count) {
        if (world == NULL || pickups == NULL || world->lev_objs == NULL || GameCam == NULL) {
            return;
        }

        const f32 maximum_distance = world->gizmo_pickup_sys->draw_distance;
        const f32 maximum_distance_squared = maximum_distance * maximum_distance;
        for (i32 index = 0; index < count; ++index) {
            GIZMOPICKUP_s &pickup = pickups[index];
            pickup.state_flags &= static_cast<u8>(~GIZMOPICKUP_STATE_DRAWN);
            if ((pickup.state_flags & GIZMOPICKUP_STATE_ACTIVE) == 0 ||
                (pickup.state_flags & (GIZMOPICKUP_STATE_VISIBLE | GIZMOPICKUP_STATE_COLLECTED)) !=
                    GIZMOPICKUP_STATE_VISIBLE ||
                (pickup.state_flags & GIZMOPICKUP_STATE_DRAW_VISIBLE) == 0) {
                continue;
            }
            if (pickup.room_index >= 0 && world->rooms_visible_ptr != NULL &&
                world->rooms_visible_ptr[pickup.room_index] == 0) {
                continue;
            }

            const f32 camera_x = pickup.position.x - GameCam->pos.x;
            const f32 camera_z = pickup.position.z - GameCam->pos.z;
            if (camera_x * camera_x + camera_z * camera_z > maximum_distance_squared) {
                continue;
            }

            GIZMO_PICKUP_TYPE *type = GetPickupType(pickup);
            if (type->field_0x0f != 0 && pickups != WorldInfo_CurrentlyActive()->gizmo_pickup_sys->temporary_pickups) {
                continue;
            }
            const i32 model_index = type->first_model_id + pickup.model_variant;
            LEVEL_OBJECT_RUNTIME_s &model = world->lev_objs[model_index];
            if (model.active == 0) {
                continue;
            }

            NUMTX matrix;
            if ((type->flags & GIZMOPICKUP_TYPE_DRAW_TUMBLING) != 0) {
                const u16 y_rotation = pickup.draw_rotation;
                const i32 x_rotation = static_cast<i32>(NU_SIN_LUT(y_rotation) * 1820.0f);
                const f32 cos_x = NU_COS_LUT(x_rotation);
                const f32 sin_x = NU_SIN_LUT(x_rotation);
                const f32 cos_y = NU_COS_LUT(y_rotation);
                const f32 sin_y = NU_SIN_LUT(y_rotation);

                matrix.m00 = cos_y;
                matrix.m01 = 0.0f;
                matrix.m02 = -sin_y;
                matrix.m03 = 0.0f;
                matrix.m10 = sin_x * sin_y;
                matrix.m11 = cos_x;
                matrix.m12 = sin_x * cos_y;
                matrix.m13 = 0.0f;
                matrix.m20 = cos_x * sin_y;
                matrix.m21 = -sin_x;
                matrix.m22 = cos_x * cos_y;
                matrix.m23 = 0.0f;
                matrix.m30 = 0.0f;
                matrix.m31 = 0.0f;
                matrix.m32 = 0.0f;
                matrix.m33 = 1.0f;
                NuMtxTranslate(&matrix, &pickup.position);
            } else if ((type->flags & GIZMOPICKUP_TYPE_DRAW_Y_ROTATION) != 0) {
                NuMtxSetRotationY(&matrix, pickup.draw_rotation);
                NuMtxTranslate(&matrix, &pickup.position);
            } else {
                NuMtxSetTranslation(&matrix, &pickup.position);
            }
            if (AreaPickupScale != 1.0f) {
                NuMtxPreScaleU(&matrix, AreaPickupScale);
            }

            if ((pickup.config_flags & GIZMOPICKUP_CONFIG_DISABLE_SHADOW_MAP) != 0) {
                ResetShadowMapRendering();
            }
            const i32 drawn = NuSpecialDrawAt(&model.special, &matrix);
            pickup.state_flags = static_cast<u8>((pickup.state_flags & ~GIZMOPICKUP_STATE_DRAWN) |
                                                 (drawn != 0 ? GIZMOPICKUP_STATE_DRAWN : 0));

            if (drawn != 0 && type->overlay_model_id != -1) {
                LEVEL_OBJECT_RUNTIME_s &overlay = world->lev_objs[type->overlay_model_id];
                if (overlay.active != 0) {
                    ResetShadowMapRendering();
                    NuSpecialDrawAt(&overlay.special, &matrix);
                    EnableShadowMapRendering(0);
                }
            }
            if ((pickup.config_flags & GIZMOPICKUP_CONFIG_DISABLE_SHADOW_MAP) != 0) {
                EnableShadowMapRendering(0);
            }

            if (VehicleArea == 0 && pickup.floor_height != 2000000.0f && maximum_distance_squared > 0.0f) {
                const f32 distance_ratio = (camera_x * camera_x + camera_z * camera_z) / maximum_distance_squared;
                NUVEC shadow_position = pickup.position;
                shadow_position.y = pickup.floor_height + 0.005f;
                const i32 opacity =
                    static_cast<i32>((1.0f - distance_ratio) *
                                     static_cast<f32>(static_cast<u8>(world->current_level->blob_shadow_alpha)));
                NuRndrAddShadow(&shadow_position, type->shadow_radius_x, opacity, pickup.shadow_x_rotation, 0,
                                pickup.shadow_z_rotation);
            }
        }
    }

    void UpdatePickupList(WORLDINFO *world, GIZMOPICKUP_s *pickups, i32 count, bool play_nearby_sfx) {
        if (world == NULL || pickups == NULL) {
            return;
        }

        const bool detector_phase =
            play_nearby_sfx && FadeSys.fade == 0.0f && NuFmod(GameTimer.time_elapsed_mod_seconds, 0.2f) < 0.1f;
        const bool minikit_detector = detector_phase && Cheats_CheckFlags(0x200) != 0;
        const bool red_brick_detector = detector_phase && Cheats_CheckFlags(0x40000) != 0;
        NUVEC *nearby_pickup = NULL;
        f32 nearest_distance_squared = 1.0f;

        SuperCounters_ResetProcessed(world);
        for (i32 index = 0; index < count; ++index) {
            GIZMOPICKUP_s &pickup = pickups[index];
            if ((pickup.state_flags & GIZMOPICKUP_STATE_ACTIVE) == 0) {
                continue;
            }

            pickup.draw_rotation = static_cast<u16>(pickup.draw_rotation + 8192.0f * FRAMETIME);
            GIZMO_PICKUP_TYPE *type = GetPickupType(pickup);

            if ((pickup.state_flags & GIZMOPICKUP_STATE_COLLECTED) == 0) {
                if ((minikit_detector && (type->flags & GIZMOPICKUP_TYPE_MINIKIT_DETECTOR) != 0) ||
                    (red_brick_detector && (type->flags & GIZMOPICKUP_TYPE_RED_BRICK_DETECTOR) != 0)) {
                    NUVEC detector_position = pickup.position;
                    detector_position.y += 0.5f;
                    MiniKitDetector(&detector_position);
                }

                if (play_nearby_sfx && GameCam != NULL) {
                    const f32 distance_squared = NuVecDistSqr(&pickup.position, &GameCam->pos, NULL);
                    if (distance_squared < nearest_distance_squared) {
                        nearest_distance_squared = distance_squared;
                        nearby_pickup = &pickup.position;
                    }
                }
            }

            if (pickup.remaining_visible_time > 0.0f) {
                if (pickup.remaining_visible_time <= 0.5f && PickUpFlickerFrames > 0 &&
                    PickUpFlickerTest <= PickupFlickerFrame % PickUpFlickerFrames) {
                    pickup.state_flags &= static_cast<u8>(~GIZMOPICKUP_STATE_DRAW_VISIBLE);
                }
                if (MiniCutCam != 2) {
                    pickup.remaining_visible_time -= FRAMETIME;
                    if (pickup.remaining_visible_time < 0.0f) {
                        pickup.state_flags &= static_cast<u8>(~GIZMOPICKUP_STATE_ACTIVE);
                    }
                }
            }

            if ((pickup.state_flags &
                 (GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE | GIZMOPICKUP_STATE_COLLECTED)) ==
                    (GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE) &&
                type->field_0x0f == 0 && type->update_fn != NULL) {
                type->update_fn(world, &pickup);
            }
        }

        if (nearby_pickup != NULL) {
            GameAudio_PlaySfx(0x25, nearby_pickup, 0, 0);
        }
    }

} // namespace

i32 gizmopickup_typeid = -1;
f32 AreaPickupScale;
i32 PickUpFlickerTest = 3;
i32 PickUpFlickerFrames = 6;
i32 PickupFlickerFrame;

static i32 GizmoPickups_GetMaxGizmos(void *pickup) {
    WORLDINFO *world = static_cast<WORLDINFO *>(pickup);
    return world != NULL ? world->current_level->max_pickups : 0;
}

static void GizmoPickups_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *, void *data) {
    GIZMOPICKUPRUNTIMESYS_s *pickup_sys = static_cast<GIZMOPICKUPRUNTIMESYS_s *>(data);
    if (pickup_sys == NULL || pickup_sys->pickups == NULL) {
        return;
    }
    for (i32 index = 0; index < pickup_sys->pickup_count; ++index) {
        GIZMOPICKUP_s &pickup = pickup_sys->pickups[index];
        if ((pickup.config_flags & GIZMOPICKUP_CONFIG_REGISTER_GIZMO) != 0) {
            AddGizmo(gizmo_sys, type_id, NULL, &pickup);
        }
    }
}

static void GizmoPickups_Update(void *world_ptr, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL || world->gizmo_pickup_sys == NULL) {
        return;
    }

    GIZMOPICKUPRUNTIMESYS_s *pickup_sys = world->gizmo_pickup_sys;
    if (pickup_sys->pickups != NULL && Missions_PickupsOff(MissionSys) == 0) {
        UpdatePickupList(world, pickup_sys->pickups, pickup_sys->pickup_count, true);
    }
    if (pickup_sys->temporary_pickups != NULL && Missions_PickupsOff(MissionSys) == 0) {
        UpdatePickupList(world, pickup_sys->temporary_pickups, GIZMOPICKUP_TEMPORARY_CAPACITY, false);
    }
}

static void GizmoPickups_Draw(void *world_ptr, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL || world->gizmo_pickup_sys == NULL) {
        return;
    }

    EnableShadowMapRendering(0);
    if (TimingBarSet == 5) {
        TBOPENFN(const_cast<char *>("Coins"), 5);
    }

    GIZMOPICKUPRUNTIMESYS_s *pickup_sys = world->gizmo_pickup_sys;
    if (pickup_sys->pickups != NULL && Missions_PickupsOff(MissionSys) == 0) {
        DrawPickupList(world, pickup_sys->pickups, pickup_sys->pickup_count);
    }
    if (pickup_sys->temporary_pickups != NULL && Missions_PickupsOff(MissionSys) == 0) {
        DrawPickupList(world, pickup_sys->temporary_pickups, GIZMOPICKUP_TEMPORARY_CAPACITY);
    }

    if (TimingBarSet == 5) {
        TBCLOSEFN(const_cast<char *>("Coins"), 5);
    }
    ResetShadowMapRendering();
}

static char *GizmoPickup_GetGizmoName(GIZMO *gizmo) {
    GIZMOPICKUP_s *pickup = gizmo != NULL ? static_cast<GIZMOPICKUP_s *>(gizmo->object) : NULL;
    return pickup != NULL ? pickup->name : NULL;
}

static i32 GizmoPickup_GetOutput(GIZMO *gizmo, i32, i32) {
    GIZMOPICKUP_s *pickup = static_cast<GIZMOPICKUP_s *>(gizmo->object);
    return (pickup->state_flags & GIZMOPICKUP_STATE_COLLECTED) != 0;
}

static char *GizmoPickup_GetOutputName(GIZMO *, i32) {
    return const_cast<char *>("Collected");
}

static i32 GizmoPickup_GetNumOutputs(GIZMO *) {
    return 1;
}

static void GizmoPickup_Activate(GIZMO *gizmo, i32 activate) {
    if (gizmo == NULL || gizmo->object == NULL) {
        return;
    }

    GIZMOPICKUP_s *pickup = static_cast<GIZMOPICKUP_s *>(gizmo->object);
    if (activate == 0) {
        pickup->state_flags &= static_cast<u8>(~GIZMOPICKUP_STATE_ENABLED);
    } else {
        pickup->state_flags |= GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_ACTIVATED;
        SuperCounter_ActivateGizmoPickup(gizmo, pickup);

        GIZMO_PICKUP_TYPE *type = GetPickupType(*pickup);
        const bool challenge_type = (type->flags & GIZMOPICKUP_TYPE_CHALLENGE_MODE_FILTER) != 0;
        if ((ChallengeMode != 0) == challenge_type && Mission_Active(NULL) == NULL) {
            if (type->activation_sfx_name != NULL) {
                PlaySfx(type->activation_sfx_name, &pickup->position);
            }
            if (type->debris_id != -1) {
                WORLDINFO *world = WorldInfo_CurrentlyActive();
                if (world != NULL) {
                    AddGameDebris(world->debris_sys, type->debris_id, &pickup->position);
                }
            }
        }
    }

    if (pickup->activation_group == 0) {
        return;
    }
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    if (world == NULL || world->gizmo_pickup_sys == NULL || world->gizmo_pickup_sys->temporary_pickups == NULL) {
        return;
    }
    const u8 enabled_and_visible = activate != 0 ? GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE : 0;
    for (i32 index = 0; index < GIZMOPICKUP_TEMPORARY_CAPACITY; ++index) {
        GIZMOPICKUP_s &group_pickup = world->gizmo_pickup_sys->temporary_pickups[index];
        if (group_pickup.activation_group == pickup->activation_group) {
            group_pickup.state_flags =
                static_cast<u8>((group_pickup.state_flags & ~(GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE)) |
                                enabled_and_visible);
        }
    }
}

static void GizmoPickup_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo == NULL) {
        return;
    }
    GIZMOPICKUP_s *pickup = static_cast<GIZMOPICKUP_s *>(gizmo->object);
    pickup->state_visible = visible != 0;
}

static NUVEC *GizmoPickup_GetPos(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZMOPICKUP_s *pickup = static_cast<GIZMOPICKUP_s *>(gizmo->object);
        return &pickup->position;
    }
    return NULL;
}

static GIZMOPICKUP_s *GizmoPickups_CollideList(GameObject_s *object, GIZMOPICKUP_s *pickups, i32 count) {
    if (pickups == NULL) {
        return NULL;
    }

    i32 collide_2d = 0;
    if ((object->apiobj.character_data->model_flags & 0x2000) != 0) {
        collide_2d = VehicleArea != 0;
    }

    f32 collide_scale = 0.0f;
    if (GizmoPickups_Collide2DFn != NULL) {
        collide_scale = GizmoPickups_Collide2DFn(object);
        if (collide_scale != 0.0f) {
            collide_2d = 1;
        }
    }

    const i32 coin_magnet = Cheats_CheckFlags(0x8000) != 0 || object->field_0xdec > 0.0f;
    f32 scaled_pickup = coin_magnet ? COINMAGNETSCALE * AreaPickupScale : AreaPickupScale;
    const f32 normal_pickup = AreaPickupScale;
    if (collide_scale != 0.0f) {
        scaled_pickup *= collide_scale;
    }
    if (collide_2d && VehicleArea != 0) {
        scaled_pickup += scaled_pickup;
    }

    for (i32 index = 0; index < count; ++index) {
        GIZMOPICKUP_s *pickup = &pickups[index];
        GIZMO_PICKUP_TYPE *type = &GizmoPickupSys->types[pickup->type_index];
        u8 flags = pickup->state_flags;
        if ((type->flags & GIZMOPICKUP_TYPE_COLLISION_FILTER) != 0 ||
            (flags & (GIZMOPICKUP_STATE_ACTIVE | GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_COLLECTED)) !=
                (GIZMOPICKUP_STATE_ACTIVE | GIZMOPICKUP_STATE_ENABLED) ||
            ((flags & GIZMOPICKUP_STATE_DRAWN) == 0 && (pickup->config_flags & 4) == 0)) {
            continue;
        }

        if ((flags & GIZMOPICKUP_STATE_ALTERNATE_TYPE) != 0 && GizmoPickupSys->alternate_type != -1) {
            type = &GizmoPickupSys->types[GizmoPickupSys->alternate_type];
        }
        if (type->field_0x0f != 0 && pickups != WorldInfo_CurrentlyActive()->gizmo_pickup_sys->temporary_pickups) {
            continue;
        }
        if ((pickup->config_flags & 4) != 0) {
            return pickup;
        }

        f32 scale = type->score == 0 ? normal_pickup : scaled_pickup;
        f32 radius_x = type->shadow_extent_x * scale;
        f32 radius_y = scale * (coin_magnet ? type->shadow_extent_z : type->shadow_radius_z);
        if (object->apiobj.collision_min.x > pickup->position.x + radius_x ||
            pickup->position.x - radius_x > object->apiobj.collision_max.x ||
            object->apiobj.collision_min.z > pickup->position.z + radius_x ||
            pickup->position.z - radius_x > object->apiobj.collision_max.z) {
            continue;
        }

        if (collide_2d) {
            f32 dx = pickup->position.x - object->apiobj.collision_position.x;
            f32 dz = pickup->position.z - object->apiobj.collision_position.z;
            f32 radius = radius_x + object->apiobj.field_0x1dc;
            if (dx * dx + dz * dz < radius * radius) {
                return pickup;
            }
        } else if (object->apiobj.collision_min.y <= pickup->position.y + radius_y &&
                   pickup->position.y - radius_y <= object->apiobj.collision_max.y) {
            if (object->field_0xcc0 == NULL) {
                if (SphereSphereOverlapScaleY(&pickup->position, radius_x, radius_y,
                                              &object->apiobj.collision_position, object->apiobj.collision_radius,
                                              object->apiobj.field_0x1e0)) {
                    return pickup;
                }
            } else {
                f32 dx = pickup->position.x - object->apiobj.collision_position.x;
                f32 dz = pickup->position.z - object->apiobj.collision_position.z;
                f32 radius = radius_x + object->apiobj.field_0x1dc;
                if (dx * dx + dz * dz < radius * radius) {
                    return pickup;
                }
            }
        }
    }
    return NULL;
}


static void *GizmoPickups_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(GIZMOPICKUPPROGRESS_s));
}

static void GizmoPickups_ClearProgress(void *, void *progress_ptr) {
    ClearPickupProgress(static_cast<GIZMOPICKUPPROGRESS_s *>(progress_ptr));
}

static void GizmoPickups_StoreProgress(void *world_ptr, void *, void *progress_ptr) {
    GIZMOPICKUPPROGRESS_s *progress = static_cast<GIZMOPICKUPPROGRESS_s *>(progress_ptr);
    if (progress != NULL) {
        memset(progress->collected, 0, sizeof(progress->collected));
        memset(progress->enabled, 0xff, sizeof(progress->enabled));
        memset(progress->visible, 0xff, sizeof(progress->visible));
        memset(progress->activated, 0, sizeof(progress->activated));
    }
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (progress == NULL || world == NULL || world->gizmo_pickup_sys == NULL ||
        world->gizmo_pickup_sys->pickups == NULL) {
        return;
    }

    const i32 count = world->gizmo_pickup_sys->pickup_count < GIZMOPICKUP_PROGRESS_CAPACITY
                          ? world->gizmo_pickup_sys->pickup_count
                          : GIZMOPICKUP_PROGRESS_CAPACITY;
    for (i32 index = 0; index < count; ++index) {
        const GIZMOPICKUP_s &pickup = world->gizmo_pickup_sys->pickups[index];
        const i32 word = index >> 5;
        const u32 bit = 1u << (index & 31);
        if ((pickup.state_flags & GIZMOPICKUP_STATE_VISIBLE) == 0) {
            progress->visible[word] &= ~bit;
        }
        if ((pickup.state_flags & GIZMOPICKUP_STATE_ENABLED) == 0) {
            progress->enabled[word] &= ~bit;
        }
        if ((pickup.state_flags & GIZMOPICKUP_STATE_COLLECTED) != 0) {
            progress->collected[word] |= bit;
        }
    }
}

static void GizmoPickups_Reset(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZMOPICKUPPROGRESS_s *progress = static_cast<GIZMOPICKUPPROGRESS_s *>(progress_ptr);

    for (i32 index = 0; index < GizmoPickupSys->type_count; ++index) {
        GIZMO_PICKUP_TYPE &type = GizmoPickupSys->types[index];
        type.shadow_extent_x = type.shadow_radius_x * type.draw_distance;
        type.shadow_extent_z = type.shadow_radius_z * type.draw_distance;
    }
    if (world == NULL || world->gizmo_pickup_sys == NULL) {
        return;
    }

    GIZMOPICKUPRUNTIMESYS_s *pickup_sys = world->gizmo_pickup_sys;
    for (i32 index = 0; index < pickup_sys->pickup_count; ++index) {
        GIZMOPICKUP_s &pickup = pickup_sys->pickups[index];
        pickup.type_index = static_cast<u8>(FindPickupTypeIndex(pickup.type_code));

        NewTerrPlatformsOff();
        pickup.floor_height = GameShadow(NULL, &pickup.position, 5.0f, -1);
        if (pickup.floor_height != -1.0f) {
            if (pickup.floor_height < pickup.position.y) {
                FindAnglesZX(&ShadNorm, &pickup.shadow_x_rotation, &pickup.shadow_z_rotation);
            } else {
                pickup.floor_height = 2000000.0f;
            }
        }

        pickup.state_flags = GIZMOPICKUP_STATE_ACTIVE | GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE |
                             GIZMOPICKUP_STATE_DRAW_VISIBLE;
        GIZMO_PICKUP_TYPE &type = GizmoPickupSys->types[pickup.type_index];
        pickup.model_variant = 0;
        if (type.random_model_count != 0) {
            pickup.model_variant = static_cast<u8>(static_cast<u16>(qrand()) / (0xffff / type.random_model_count + 1));
        }
        pickup.room_index = world->current_gscn != NULL
                                ? static_cast<i8>(NuPortalWhichRoom(world->current_gscn, &pickup.position))
                                : -1;
        pickup.draw_rotation = static_cast<u16>(qrand());
        pickup.remaining_visible_time = 0.0f;

        if (progress != NULL && index < GIZMOPICKUP_PROGRESS_CAPACITY) {
            const i32 word = index >> 5;
            const u32 bit = 1u << (index & 31);
            pickup.state_flags =
                static_cast<u8>((pickup.state_flags & ~(GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE |
                                                        GIZMOPICKUP_STATE_COLLECTED | GIZMOPICKUP_STATE_ACTIVATED)) |
                                ((progress->enabled[word] & bit) != 0 ? GIZMOPICKUP_STATE_ENABLED : 0) |
                                ((progress->visible[word] & bit) != 0 ? GIZMOPICKUP_STATE_VISIBLE : 0) |
                                ((progress->collected[word] & bit) != 0 ? GIZMOPICKUP_STATE_COLLECTED : 0) |
                                ((progress->activated[word] & bit) != 0 ? GIZMOPICKUP_STATE_ACTIVATED : 0));
        }
        if ((pickup.config_flags & GIZMOPICKUP_CONFIG_REQUIRES_ACTIVATION) != 0 &&
            (pickup.state_flags & GIZMOPICKUP_STATE_ACTIVATED) == 0) {
            pickup.state_flags &= static_cast<u8>(~(GIZMOPICKUP_STATE_ENABLED | GIZMOPICKUP_STATE_VISIBLE));
        }
    }

    if (pickup_sys->temporary_pickups != NULL) {
        memset(pickup_sys->temporary_pickups, 0, sizeof(GIZMOPICKUP_s) * GIZMOPICKUP_TEMPORARY_CAPACITY);
    }
}

static void *GizmoPickups_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    GIZMOPICKUPRUNTIMESYS_s *pickup_sys = reinterpret_cast<GIZMOPICKUPRUNTIMESYS_s *>(world->giz_buffer.addr);
    world->gizmo_pickup_sys = pickup_sys;
    world->giz_buffer.addr += sizeof(*pickup_sys);
    memset(pickup_sys, 0, sizeof(*pickup_sys));

    pickup_sys->draw_distance = 10.0f;
    AreaPickupScale = 1.0f;
    const i32 area_index = world->level_sub_id;
    if (area_index >= 0 && area_index < AREACOUNT) {
        const u16 flags = ADataList[area_index].flags;
        if ((flags & AREAFLAG_NOPICKUPGRAVITY) != 0) {
            AreaPickupScale = 6.0f;
        } else if ((flags & AREAFLAG_VEHICLE_AREA) != 0) {
            AreaPickupScale = BonusArea == 0 ? 5.0f : 3.0f;
        }
    }
    pickup_sys->pickup_scale = AreaPickupScale;
    SetAreaPickupGravity(world->level_sub_id, world->level_idx);

    usize buffer_address = world->giz_buffer.addr;
    if (world->current_level->max_pickups != 0) {
        buffer_address = ALIGN(buffer_address, 4);
        world->giz_buffer.addr = buffer_address;
        pickup_sys->pickups = reinterpret_cast<GIZMOPICKUP_s *>(buffer_address);
        buffer_address += world->current_level->max_pickups * sizeof(GIZMOPICKUP_s);
    }
    buffer_address = ALIGN(buffer_address, 4);
    world->giz_buffer.addr = buffer_address;
    pickup_sys->temporary_pickups = reinterpret_cast<GIZMOPICKUP_s *>(buffer_address);
    world->giz_buffer.addr += GIZMOPICKUP_TEMPORARY_CAPACITY * sizeof(GIZMOPICKUP_s);
    return pickup_sys;
}

static i32 GizmoPickups_Load(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZMOPICKUPRUNTIMESYS_s *pickup_sys = static_cast<GIZMOPICKUPRUNTIMESYS_s *>(data);
    if (world == NULL || pickup_sys == NULL || pickup_sys->pickup_count != 0) {
        return 0;
    }

    const i32 version = EdFileReadInt();
    pickup_sys->pickup_count = EdFileReadInt();
    if (version >= 3) {
        pickup_sys->total_score = EdFileReadInt();
    }
    if (version >= 5) {
        pickup_sys->draw_distance = EdFileReadFloat();
        pickup_sys->pickup_scale = EdFileReadFloat();
        if (version == 5) {
            pickup_sys->pickup_scale = GetAreaPickupScale(world);
        }
        AreaPickupScale = pickup_sys->pickup_scale;
    } else {
        pickup_sys->pickup_scale = 1.0f;
        AreaPickupScale = 1.0f;
    }

    if (pickup_sys->draw_distance < 10.0f) {
        pickup_sys->draw_distance = 10.0f;
    }
    if (version == 6 && world->area != NULL && (world->area->flags & AREAFLAG_VEHICLE_AREA) != 0 &&
        pickup_sys->draw_distance < 100.0f) {
        pickup_sys->draw_distance = 100.0f;
    }
    SetAreaPickupGravity(world->level_sub_id, world->level_idx);

    for (i32 index = 0; index < pickup_sys->pickup_count; ++index) {
        GIZMOPICKUP_s &pickup = pickup_sys->pickups[index];
        EdFileRead(pickup.name, sizeof(pickup.name));
        EdFileReadNuVec(&pickup.position);
        pickup.type_code = static_cast<char>(EdFileReadChar());
        if (version >= 2) {
            pickup.config_flags = static_cast<u8>(EdFileReadChar());
        }
        if (version >= 4) {
            pickup.activation_group = static_cast<u8>(EdFileReadChar());
        }
    }
    return 1;
}

void GizmoPickups_PostLoad(void *, void *) {
}

void GizmoPickups_InitSys(GIZMOPICKUPSYS_s *pickup_sys) {
    GizmoPickupSys = pickup_sys;
}

ADDGIZMOTYPE *GizmoPickups_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizmoPickup";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0x100;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizmoPickups_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizmoPickup_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizmoPickups_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = GizmoPickups_Update;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = GizmoPickups_Draw;
    addtype.fns.get_gizmo_name_fn = GizmoPickup_GetGizmoName;
    addtype.fns.get_output_fn = GizmoPickup_GetOutput;
    addtype.fns.get_output_name_fn = GizmoPickup_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizmoPickup_GetNumOutputs;
    addtype.fns.activate_fn = GizmoPickup_Activate;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = GizmoPickup_SetVisibility;
    addtype.fns.allocate_progress_data_fn = GizmoPickups_AllocateProgressData;
    addtype.fns.clear_progress_fn = GizmoPickups_ClearProgress;
    addtype.fns.store_progress_fn = GizmoPickups_StoreProgress;
    addtype.fns.reset_fn = GizmoPickups_Reset;
    addtype.fns.reserve_buffer_space_fn = GizmoPickups_ReserveBufferSpace;
    addtype.fns.load_fn = GizmoPickups_Load;
    addtype.fns.post_load_fn = GizmoPickups_PostLoad;
    addtype.fns.add_level_sfx_fn = NULL;
    gizmopickup_typeid = type_id;

    return &addtype;
}

void SpecialMiniKits_Reset(WORLDINFO_s *world) {
    WORLDINFO_s *world_info = world;
    SPECIALMINIKITSYS_s *system = world_info->special_minikits;
    if (system == NULL || GizmoPickupSys->gizmo_type_id == -1) {
        return;
    }

    i32 count = system->count;
    SPECIALMINIKIT_s *item = system->items;
    if (count <= 0) {
        return;
    }

    i32 index = 0;
    for (;;) {
        item->pickup_gizmo = GizmoFindByName(world_info->gizmo_sys, gizmopickup_typeid, item->pickup_name);
        if ((item->flags & 0x20) != 0) {
            item->special_gizmo = GizmoFindByName(world_info->gizmo_sys, -1, item->special_name);
        }
        ++index;
        ++item;
        if (world_info->special_minikits->count <= index) {
            break;
        }
    }
}
