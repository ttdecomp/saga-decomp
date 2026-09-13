#include "decomp.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "globals.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "legoapi/characters/core/players.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/numath/nufloat.h"

#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/world_shared.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/nutrig.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/audio/audio.h"
#include <stdio.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern TerrainQuery_s *TerI;
extern TERRSET *CurTerr;
extern tertype **TerrOverRideScan;
extern i32 PlatCrush;
extern i32 curSphereter;
extern i32 plathitid;
extern TERRAIN_SPHERE SphereData[16];

i32 HitWallSpline();
void DeRotatePoint(NUVEC *point);
void DeRotateTerrain(tertype *surface);
void RotateVec(NUVEC *source, NUVEC *destination);
i16 InsidePolLines(f32 point_x, f32 point_y, f32 point_z, f32 edge_a_x, f32 edge_a_y, f32 edge_a_z, f32 edge_b_x,
                   f32 edge_b_y, f32 edge_b_z, NUVEC *normal);
i32 CheckCylinder(i32 first_vertex, i32 second_vertex, i32 *vertex_mask, i32 remaining_vertex_mask);
i32 CheckSphere(i32 vertex_index);
i32 CheckSphereTer(NUVEC *position, f32 radius);
i32 HitPoly(f32 primary_start, f32 primary_end, f32 secondary_start, f32 secondary_end, tertype *surface);
i32 ForcePushed_SuperPush_Occurring(GameObject_s *first, GameObject_s *second);
void StartFlatten(GameObject_s *source, GameObject_s *target);

i32 objhitobj_nohurtsfx;
i32 objhitobj_noattackerrumble;
i32 objhitobj_throwkillpartsup;
extern i32 objhitobj_noimpactsfx;
extern i16 *objhitobj_killparts_yrot;
extern BOLT_s *objhitobj_bolt;
extern i32 ObstacleCamHoldUntilPlayersMove, LEGOCONTEXT_LAND_SLAM, LEGOCONTEXT_LAND_LUNGE, LEGOCONTEXT_COMBO,
    LEGOCONTEXT_HOLD;
extern i32 disable_narrow_socks, players_cannot_exit_speeder, BuildUpDone;
extern f32 BuildUpScale, DrawBuildUpTime, builduptime;
void AlertSurroundingCreatures(GameObject_s *, NUVEC *);
i32 Hub_InMenu();
i32 RotDiff(u16, u16);
void NewRumble(nupad_s *, f32, i32);
void NewBlockAction(GameObject_s *);
void PlayerTakeHit(GameObject_s *, GameObject_s *);
void ReleaseForce(GameObject_s *, i32);
void LoseHelmet(GameObject_s *, i32, i32);
void TakeHitRumble(GameObject_s *, f32);
void KillRumble(GameObject_s *);
void PlayHurtSfx(GameObject_s *);
void SnakeBeenHit(GameObject_s *);
void PopBalloon(GameObject_s *);
i32 Player_HasFastBuild(GameObject_s *);
i32 Player_HasInvincibility(GameObject_s *);
i32 ObjIsTargetSpeeder(GameObject_s *);
i32 LoseCoins(GameObject_s *, i32);
i32 ReleaseHearts();
void AddPickups(i32, i32, i32, i32, NUVEC *, NUVEC *, f32, i32, f32, f32, GameObject_s *, i32, i32, bool);
void DropTorpedoPickups(TORPEDOPACKET_s *, i32);
void KillParts(GameObject_s *, i32, i32, i32, f32, i32, u16 *);
void KillGameObject(GameObject_s *, i32, i32);
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void Arcade_Kill(i32, i32);
i32 qrand();
GAMEPAD_s *ViewCamGetGamePad();
i32 Cheat_IsOn(i32);
void SetFlicker(GameObject_s *, f32);
void Player_ClearContext(GameObject_s *, i32);
extern "C" void AddGameDebris(APIDEBRISSYS_s *, i32, NUVEC *);
extern "C" i32 AnimMiscFlags(CHARACTERMODEL_s *, i32);
extern "C" void NuSpecialSetVisibility(void *, i32);
static const u8 objhit_damage_joints[3] = {6, 8, 7};

extern i16 id_ANAKINJEDI;
extern i16 id_ATAT;
extern i16 id_BODYGUARD;
extern i16 id_DRAGBOMB;
extern i16 id_GAMORREANGUARD;
extern i16 id_IMPERIALGUARD;
extern i16 id_OBIWANKENOBIEP3;
extern i16 id_ROYALGUARD;
extern i16 id_SNAKE;
extern i16 id_SPEEDERBIKE;
extern i16 id_WOOKIEE;
extern AREADATA_s *HOTHBATTLE_ADATA;
extern AREADATA_s *PODRACE_ADATA;
extern AREADATA_s *PODSPRINT_ADATA;
extern AREADATA_s *SPEEDERCHASE_ADATA;
extern LEVELDATA_s *CRUISERC_LDATA;
extern LEVELDATA_s *HUB_LDATA;
extern LEVELDATA_s *SPEEDERCHASEA_LDATA;
extern LEVELDATA_s *VADERC_LDATA;

i32 CannotKill(GameObject_s *object) {
    GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    return (CInfo[object->character_context].flags & 0x2000000) != 0 ||
           ((data->field_0x94 & 0x800) != 0 && object->apiobj.field_0x27c == -1);
}

static i32 Collide2Objects(APIOBJECT *first, APIOBJECT *second) {
    const f32 first_vertical_velocity = first->velocity.y;
    const f32 second_vertical_velocity = second->velocity.y;
    GameObject_s *first_object = first->objptr;
    GameObject_s *second_object = second->objptr;
    i32 collision_result = ForcePushed_SuperPush_Occurring(first_object, second_object);
    if (collision_result == 0) {
        if (WORLD->current_level == DOGFIGHTA_LDATA) {
            collision_result = APIObjectCollision(first, second);
        } else {
            collision_result = APIObjectCollision2D(first, second);
        }

        if (collision_result == 2 && VehicleArea == 0) {
            GAMECHARACTERDATA *first_data =
                static_cast<GAMECHARACTERDATA *>(first_object->apiobj.character_data->field11_0x24);
            GAMECHARACTERDATA *second_data =
                static_cast<GAMECHARACTERDATA *>(second_object->apiobj.character_data->field11_0x24);
            if ((first_data->flags_090 & 0x1000) != 0 &&
                first->horizontal_velocity_magnitude > first_data->movement_speed * 0.5f) {
                StartFlatten(first_object, second_object);
            } else if ((second_data->flags_090 & 0x1000) != 0 &&
                       second->horizontal_velocity_magnitude > second_data->movement_speed * 0.5f) {
                StartFlatten(second_object, first_object);
            } else if (second_vertical_velocity < 0.0f && (first_data->flags_090 & 0x40) != 0 &&
                       first_object->field_0x107c == -1) {
                first->velocity.y += second_vertical_velocity * 0.5f;
            } else if (first_vertical_velocity < 0.0f && (second_data->flags_090 & 0x40) != 0 &&
                       second_object->field_0x107c == -1) {
                second->velocity.y += first_vertical_velocity * 0.5f;
            }
        }

        f32 maximum_velocity = first->character_data->game_character->movement_speed * 1.25f;
        if (maximum_velocity < 1.0f) {
            maximum_velocity = 1.0f;
        }
        f32 velocity_squared = first->velocity.x * first->velocity.x + first->velocity.z * first->velocity.z;
        if (velocity_squared > maximum_velocity * maximum_velocity) {
            const f32 scale = maximum_velocity / NuFsqrt(velocity_squared);
            first->velocity.x *= scale;
            first->velocity.z *= scale;
        }

        maximum_velocity = second->character_data->game_character->movement_speed * 1.25f;
        if (maximum_velocity < 1.0f) {
            maximum_velocity = 1.0f;
        }
        velocity_squared = second->velocity.x * second->velocity.x + second->velocity.z * second->velocity.z;
        if (velocity_squared > maximum_velocity * maximum_velocity) {
            const f32 scale = maximum_velocity / NuFsqrt(velocity_squared);
            second->velocity.x *= scale;
            second->velocity.z *= scale;
        }
    } else {
        collision_result = (second_object->apiobj.field_0x1e8 & first_object->apiobj.field_0x1f0) != 0 ||
                                   (second_object->apiobj.field_0x1e4 & first_object->apiobj.field_0x1ec) != 0 ||
                                   (first_object->apiobj.field_0x1e8 & second_object->apiobj.field_0x1f0) != 0 ||
                                   (first_object->apiobj.field_0x1e4 & second_object->apiobj.field_0x1ec) != 0
                               ? 1
                               : 2;
        first->field_0x1ec |= second->field_0x1e4;
        first->field_0x1f0 |= second->field_0x1e8;
        second->field_0x1ec |= first->field_0x1e4;
        second->field_0x1f0 |= first->field_0x1e8;
    }
    return collision_result;
}

i32 HitTerrain() {
    u8 *scan = reinterpret_cast<u8 *>(TerrOverRideScan);
    if (scan == NULL) {
        scan = TerI->scan_list_storage;
    }

    TerI->hit_time = 999.9f;
    const f32 radius = TerI->collision_radius;
    TerI->shape_adjusted = 0;
    TerI->hit_type = TERRAIN_HIT_TYPE_NONE;
    TerI->terrain_group_index = -1;

    HitWallSpline();

    TerI->horizontal_movement_length =
        NuFsqrt(TerI->movement.x * TerI->movement.x + TerI->movement.z * TerI->movement.z);

    i32 collision_found = 0;
    f32 primary_start_distance = 0.0f;
    f32 secondary_start_distance = 0.0f;
    f32 secondary_end_distance = 0.0f;
    const f32 negative_radius = -radius;

    i16 shape_count = *reinterpret_cast<i16 *>(scan);
    for (;;) {
        while (shape_count > 0) {
            // A scan group packs its count and terrain-group index into the first
            // pointer-sized slot, followed by the selected shape pointers.
            TERRAIN_GROUP *group = &CurTerr->groups[*reinterpret_cast<i16 *>(scan + sizeof(i16))];
            tertype **scan_entries = reinterpret_cast<tertype **>(scan);
            tertype **surfaces = scan_entries + 1;

            TerrainQuery_s *query = TerI;
            query->local_start.x = query->position.x - group->origin.x;
            query->local_start.y = query->position.y - group->origin.y;
            query->local_start.z = query->position.z - group->origin.z;
            query->local_end.y = query->position.y + query->movement.y - group->origin.y;
            query->local_end.x = query->position.x + query->movement.x - group->origin.x;
            query->local_end.z = query->position.z + query->movement.z - group->origin.z;

            const i32 last_shape_index = shape_count - 1;
            i32 shape_index = 0;
            for (;;) {
                tertype *surface = scan_entries[shape_index + 1];
                const NUVEC &primary_point = surface->vectors[0];
                const NUVEC &primary_normal = surface->normals[0];

                f32 primary_end_distance = (query->local_end.x - primary_point.x) * primary_normal.x +
                                           (query->local_end.y - primary_point.y) * primary_normal.y +
                                           (query->local_end.z - primary_point.z) * primary_normal.z - radius -
                                           query->compare_epsilon;

                i32 crosses_surface = 0;
                if (0.0f > primary_end_distance) {
                    primary_start_distance = (query->local_start.x - primary_point.x) * primary_normal.x +
                                             (query->local_start.y - primary_point.y) * primary_normal.y +
                                             (query->local_start.z - primary_point.z) * primary_normal.z - radius;
                    crosses_surface = primary_start_distance > negative_radius;
                }

                const NUVEC &secondary_point = surface->vectors[3];
                const NUVEC &secondary_normal = surface->normals[1];
                if (65536.0f > secondary_normal.y) {
                    secondary_end_distance = (query->local_end.x - secondary_point.x) * secondary_normal.x +
                                             (query->local_end.y - secondary_point.y) * secondary_normal.y +
                                             (query->local_end.z - secondary_point.z) * secondary_normal.z - radius -
                                             query->compare_epsilon;
                    if (0.0f > secondary_end_distance) {
                        secondary_start_distance = (query->local_start.x - secondary_point.x) * secondary_normal.x +
                                                   (query->local_start.y - secondary_point.y) * secondary_normal.y +
                                                   (query->local_start.z - secondary_point.z) * secondary_normal.z -
                                                   radius;
                        if (secondary_start_distance > negative_radius) {
                            crosses_surface = 1;
                        }
                    }
                }

                if (crosses_surface != 0 && HitPoly(primary_start_distance, primary_end_distance,
                                                    secondary_start_distance, secondary_end_distance, surface) != 0) {
                    query = TerI;
                    query->terrain_group_index = *reinterpret_cast<u16 *>(scan + sizeof(i16));
                    collision_found = 1;
                    tertype *hit_surface = query->surface;
                    if ((query->hit_type & TERRAIN_HIT_TYPE_SECOND_NORMAL) != 0 &&
                        (hit_surface->flags & TERRAIN_SHAPE_FLAG_MATERIAL_ALIAS) != 0) {
                        PlatCrush = hit_surface->material[0] + 1;
                    }
                }
                query = TerI;
                if (shape_index == last_shape_index) {
                    break;
                }
                ++shape_index;
            }

            const i16 next_shape_count = *reinterpret_cast<i16 *>(surfaces + shape_index + 1);
            scan = reinterpret_cast<u8 *>(surfaces + shape_count);
            shape_count = next_shape_count;
        }

        if (shape_count == 0) {
            break;
        }
        do {
            scan += (1 - shape_count) * sizeof(tertype *);
            shape_count = *reinterpret_cast<i16 *>(scan);
        } while (shape_count < 0);
    }

    NUVEC sphere_position;
    for (i32 sphere_index = 0; sphere_index < curSphereter; ++sphere_index) {
        const TERRAIN_SPHERE &sphere = SphereData[sphere_index];
        const f32 scaled_collision_radius = TerI->collision_radius * TerI->object_scale;
        sphere_position = sphere.position;
        sphere_position.y *= TerI->inverse_object_scale;
        sphere_position.y -= scaled_collision_radius;
        DeRotatePoint(&sphere_position);
        collision_found |= CheckSphereTer(&sphere_position, sphere.radius);
    }

    TerrainQuery_s *query = TerI;
    if (query->hit_type != TERRAIN_HIT_TYPE_NONE && query->terrain_group_index != -1) {
        TERRAIN_GROUP *group = &CurTerr->groups[query->terrain_group_index];
        if (group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY) {
            plathitid = group->scene_index;
        }
    }

    TerrOverRideScan = NULL;
    return collision_found;
}

void KillRumble(GameObject_s *) {
}

i32 CheckSphere(i32 vertex_index) {
    f32 radius = TerI->collision_radius;
    if (TerI->transformed_vertices[vertex_index].z < -radius ||
        TerI->transformed_vertices[vertex_index].z > TerI->movement_length + radius) {
        return 0;
    }

    f32 radial_distance_sq = TerI->transformed_vertices[vertex_index].x * TerI->transformed_vertices[vertex_index].x +
                             TerI->transformed_vertices[vertex_index].y * TerI->transformed_vertices[vertex_index].y;
    if (radial_distance_sq > TerI->collision_radius_sq) {
        return 0;
    }

    f32 radial_extent = NuFsqrt(TerI->collision_radius_sq - radial_distance_sq);
    f32 hit_distance = TerI->transformed_vertices[vertex_index].z - radial_extent;
    if (0.0f <= hit_distance && TerI->movement_length >= hit_distance) {
        f32 hit_time = hit_distance / TerI->movement_length;
        if (TerI->hit_time <= hit_time) {
            return 0;
        }

        TerI->hit_time = hit_time;
        TerI->hit_type = TERRAIN_HIT_TYPE_VERTEX;
        TerI->movement_normal.x = -TerI->transformed_vertices[vertex_index].x;
        TerI->movement_normal.y = -TerI->transformed_vertices[vertex_index].y;
        TerI->movement_normal.z = -radial_extent;
        return 1;
    }

    f32 distance_sq =
        radial_distance_sq + TerI->transformed_vertices[vertex_index].z * TerI->transformed_vertices[vertex_index].z;
    if (TerI->collision_radius_sq <= distance_sq) {
        return 0;
    }

    f32 distance = NuFsqrt(distance_sq);
    f32 overlap_time = distance - TerI->collision_radius;
    if (TerI->hit_time <= overlap_time) {
        return 0;
    }

    tertype *surface = TerI->working_surface;
    f32 primary_distance = (TerI->local_start.x - surface->vectors[vertex_index].x) * surface->normals[0].x +
                           (TerI->local_start.y - surface->vectors[vertex_index].y) * surface->normals[0].y +
                           (TerI->local_start.z - surface->vectors[vertex_index].z) * surface->normals[0].z;
    if (0.0f >= primary_distance) {
        return 0;
    }

    TerI->hit_time = overlap_time;
    f32 inverse_distance = 0.0f;
    if (distance != 0.0f) {
        inverse_distance = 1.0f / distance;
    }
    TerI->hit_type = TERRAIN_HIT_TYPE_VERTEX | TERRAIN_HIT_TYPE_SECOND_NORMAL;
    TerI->movement_normal.x = -TerI->transformed_vertices[vertex_index].x * inverse_distance;
    TerI->movement_normal.y = -TerI->transformed_vertices[vertex_index].y * inverse_distance;
    TerI->movement_normal.z = -TerI->transformed_vertices[vertex_index].z * inverse_distance;
    return 1;
}

void FloatRumble(GameObject_s *) {
}

i32 HitTerrPoly(tertype *surface, i32 group_index) {
    f32 radius = TerI->collision_radius;
    TerI->hit_type = 0;
    TerI->hit_time = 999.9f;
    TerI->horizontal_movement_length =
        NuFsqrt(TerI->movement.x * TerI->movement.x + TerI->movement.z * TerI->movement.z);
    NUVEC *origin = &CurTerr->groups[group_index].origin;
    TerI->local_start.x = TerI->position.x - origin->x;
    TerI->local_start.y = TerI->position.y - origin->y;
    TerI->local_start.z = TerI->position.z - origin->z;
    TerI->local_end.x = (TerI->position.x + TerI->movement.x) - origin->x;
    TerI->local_end.y = (TerI->position.y + TerI->movement.y) - origin->y;
    TerI->local_end.z = (TerI->position.z + TerI->movement.z) - origin->z;
    f32 primary_end = (((TerI->local_end.x - surface->vectors[0].x) * surface->normals[0].x +
                        (TerI->local_end.y - surface->vectors[0].y) * surface->normals[0].y) +
                       (TerI->local_end.z - surface->vectors[0].z) * surface->normals[0].z) -
                      radius - TerI->compare_epsilon;
    f32 primary_start = 0.0f;
    bool test = false;
    if (primary_end < 0.0f) {
        primary_start = ((TerI->local_start.x - surface->vectors[0].x) * surface->normals[0].x +
                         (TerI->local_start.y - surface->vectors[0].y) * surface->normals[0].y) +
                        (TerI->local_start.z - surface->vectors[0].z) * surface->normals[0].z - radius;
        test = primary_start > -radius;
    }
    f32 secondary_start = 0.0f;
    f32 secondary_end = 0.0f;
    if (surface->normals[1].y < 65536.0f) {
        secondary_end = (((TerI->local_end.x - surface->vectors[3].x) * surface->normals[1].x +
                          (TerI->local_end.y - surface->vectors[3].y) * surface->normals[1].y) +
                         (TerI->local_end.z - surface->vectors[3].z) * surface->normals[1].z) -
                        radius - TerI->compare_epsilon;
        if (secondary_end < 0.0f) {
            secondary_start = ((TerI->local_start.x - surface->vectors[3].x) * surface->normals[1].x +
                               (TerI->local_start.y - surface->vectors[3].y) * surface->normals[1].y) +
                              (TerI->local_start.z - surface->vectors[3].z) * surface->normals[1].z - radius;
            if (secondary_start > -radius)
                test = true;
        }
    }
    if (test && HitPoly(primary_start, primary_end, secondary_start, secondary_end, surface)) {
        TerI->terrain_group_index = group_index;
        return 1;
    }
    return 0;
}

i16 InsideLineF(f32 point_u, f32 point_v, f32 line_start_u, f32 line_start_v, f32 line_end_u, f32 line_end_v) {
    const f32 side =
        (point_u - line_start_u) * (line_end_v - line_start_v) + (point_v - line_start_v) * (line_start_u - line_end_u);
    if (side < 0.0f) {
        return 0;
    }
    return 1;
}

i16 InsideLineXZ(f32 point_u, f32 point_v, f32 line_start_u, f32 line_start_v, f32 line_end_u, f32 line_end_v) {
    const f32 side =
        (point_u - line_start_u) * (line_end_v - line_start_v) + (point_v - line_start_v) * (line_start_u - line_end_u);
    return side >= 0.0f;
}

void ObjHitShield(GameObject_s *, GameObject_s *, i32, BOLT_s *) {
}

i32 CheckCylinder(i32 first_vertex, i32 second_vertex, i32 *vertex_mask, i32 remaining_vertex_mask) {
    TerrainQuery_s *query = TerI;

    // Reject an edge when both endpoints lie outside the swept sphere's
    // expanded bounds. Once an edge is rejected this way, its endpoint bits
    // can be removed from the later vertex tests as well.
    if ((query->transformed_vertices[first_vertex].x > query->collision_radius &&
         query->transformed_vertices[second_vertex].x > query->collision_radius) ||
        (query->transformed_vertices[first_vertex].x < -query->collision_radius &&
         query->transformed_vertices[second_vertex].x < -query->collision_radius) ||
        (query->transformed_vertices[first_vertex].y > query->collision_radius &&
         query->transformed_vertices[second_vertex].y > query->collision_radius) ||
        (query->transformed_vertices[first_vertex].y < -query->collision_radius &&
         query->transformed_vertices[second_vertex].y < -query->collision_radius) ||
        (query->transformed_vertices[first_vertex].z < -query->collision_radius &&
         query->transformed_vertices[second_vertex].z < -query->collision_radius) ||
        (query->transformed_vertices[first_vertex].z > query->movement_length + query->collision_radius &&
         query->transformed_vertices[second_vertex].z > query->movement_length + query->collision_radius)) {
        *vertex_mask &= remaining_vertex_mask;
        return 0;
    }

    NUVEC edge_direction = {
        query->transformed_vertices[second_vertex].x - query->transformed_vertices[first_vertex].x,
        query->transformed_vertices[second_vertex].y - query->transformed_vertices[first_vertex].y,
        query->transformed_vertices[second_vertex].z - query->transformed_vertices[first_vertex].z,
    };
    const f32 horizontal_length_sq = edge_direction.x * edge_direction.x + edge_direction.y * edge_direction.y;

    // An almost vertical edge has no stable radial cylinder in the XY plane.
    // It can only overlap the sphere at the start of the sweep when it crosses
    // z = 0.
    if (horizontal_length_sq < 1.0e-12f) {
        if ((query->transformed_vertices[first_vertex].z < 0.0f &&
             query->transformed_vertices[second_vertex].z < 0.0f) ||
            (query->transformed_vertices[first_vertex].z > 0.0f &&
             query->transformed_vertices[second_vertex].z > 0.0f)) {
            return 0;
        }

        const f32 distance_sq =
            query->transformed_vertices[second_vertex].x * query->transformed_vertices[second_vertex].x +
            query->transformed_vertices[second_vertex].y * query->transformed_vertices[second_vertex].y;
        if (distance_sq > query->collision_radius_sq) {
            return 0;
        }

        const f32 distance = NuFsqrt(distance_sq);
        const f32 overlap_time = distance - query->collision_radius;
        if (query->hit_time <= overlap_time) {
            return 0;
        }

        NUVEC local_normal = {
            -query->transformed_vertices[second_vertex].x,
            -query->transformed_vertices[second_vertex].y,
            0.0f,
        };
        RotateVec(&local_normal, &local_normal);
        const NUVEC &surface_normal = query->working_surface->normals[0];
        if (local_normal.x * surface_normal.x + local_normal.y * surface_normal.y + local_normal.z * surface_normal.z <=
            0.0f) {
            return 0;
        }

        query = TerI;
        query->hit_time = overlap_time;
        f32 inverse_distance = 0.0f;
        if (distance != 0.0f) {
            inverse_distance = 1.0f / distance;
        }
        query->hit_type = TERRAIN_HIT_TYPE_CYLINDER | TERRAIN_HIT_TYPE_SECOND_NORMAL;
        query->movement_normal.x = -query->transformed_vertices[second_vertex].x * inverse_distance;
        query->movement_normal.y = -query->transformed_vertices[second_vertex].y * inverse_distance;
        query->movement_normal.z = 0.0f;
        return 1;
    }

    // Squared radial distance from the movement axis to the infinite line
    // containing the edge. This avoids a square root for the common miss.
    const f32 line_cross = query->transformed_vertices[first_vertex].x * edge_direction.y -
                           query->transformed_vertices[first_vertex].y * edge_direction.x;
    if (line_cross * line_cross > query->collision_radius_sq * horizontal_length_sq) {
        *vertex_mask &= remaining_vertex_mask;
        return 0;
    }

    f32 line_distance_sq = 0.0f;
    if (horizontal_length_sq != 0.0f && line_cross != 0.0f) {
        line_distance_sq = line_cross * (line_cross / horizontal_length_sq);
    }

    const f32 edge_length = NuFsqrt(horizontal_length_sq + edge_direction.z * edge_direction.z);
    f32 inverse_edge_length = 0.0f;
    if (edge_length != 0.0f) {
        inverse_edge_length = 1.0f / edge_length;
    }
    edge_direction.x *= inverse_edge_length;
    edge_direction.y *= inverse_edge_length;
    edge_direction.z *= inverse_edge_length;

    NUVEC line_cross_vector = {
        -query->transformed_vertices[first_vertex].x,
        -query->transformed_vertices[first_vertex].y,
        -query->transformed_vertices[first_vertex].z,
    };
    NuVecCross(&line_cross_vector, &line_cross_vector, &edge_direction);

    NUVEC horizontal_perpendicular = {-edge_direction.y, edge_direction.x, 0.0f};
    const f32 horizontal_direction_sq = horizontal_perpendicular.x * horizontal_perpendicular.x +
                                        horizontal_perpendicular.y * horizontal_perpendicular.y;
    const f32 height_numerator =
        line_cross_vector.x * horizontal_perpendicular.x + line_cross_vector.y * horizontal_perpendicular.y;
    f32 negative_closest_height = 0.0f;
    if (horizontal_direction_sq != 0.0f && height_numerator != 0.0f) {
        negative_closest_height = height_numerator / horizontal_direction_sq;
    }
    const f32 closest_height = -negative_closest_height;

    // This normal lies in the plane formed by the edge and the movement axis.
    // Its z component tells how far along the sweep the cylinder surface is
    // reached.
    NUVEC cylinder_side = {
        edge_direction.x * edge_direction.z,
        -edge_direction.y * edge_direction.z,
        -(edge_direction.x * edge_direction.x + edge_direction.y * edge_direction.y),
    };
    const f32 cylinder_side_length = NuFsqrt(cylinder_side.x * cylinder_side.x + cylinder_side.y * cylinder_side.y +
                                             cylinder_side.z * cylinder_side.z);
    if (cylinder_side_length != 0.0f && cylinder_side.z != 0.0f) {
        cylinder_side.z /= cylinder_side_length;
    } else {
        cylinder_side.z = 0.0f;
    }

    const f32 radial_extent = NuFsqrt(query->collision_radius_sq - line_distance_sq);
    f32 height_offset = 0.0f;
    if (radial_extent != 0.0f && cylinder_side.z != 0.0f) {
        height_offset = radial_extent / cylinder_side.z;
    }

    f32 hit_distance;
    if (height_offset >= 0.0f) {
        hit_distance = closest_height - height_offset;
    } else {
        hit_distance = height_offset - negative_closest_height;
    }
    if (hit_distance >= 0.0f && hit_distance <= query->movement_length) {
        const f32 edge_parameter = (hit_distance - query->transformed_vertices[first_vertex].z) * edge_direction.z -
                                   query->transformed_vertices[first_vertex].x * edge_direction.x -
                                   query->transformed_vertices[first_vertex].y * edge_direction.y;
        if (edge_parameter > 0.0f && edge_parameter <= edge_length) {
            f32 hit_time = 0.0f;
            if (hit_distance != 0.0f && query->movement_length != 0.0f) {
                hit_time = hit_distance / query->movement_length;
            }
            if (query->hit_time <= hit_time) {
                *vertex_mask &= remaining_vertex_mask;
                return 0;
            }

            query->hit_type = TERRAIN_HIT_TYPE_CYLINDER;
            query->hit_time = hit_time;
            query->movement_normal.x =
                -(query->transformed_vertices[first_vertex].x + edge_direction.x * edge_parameter);
            query->movement_normal.y =
                -(query->transformed_vertices[first_vertex].y + edge_direction.y * edge_parameter);
            query->movement_normal.z =
                hit_distance - query->transformed_vertices[first_vertex].z - edge_direction.z * edge_parameter;
            *vertex_mask &= remaining_vertex_mask;
            return 1;
        }
    }

    // No forward-time hit was found. Test whether the sphere already overlaps
    // the finite edge at the start of the sweep.
    if ((query->transformed_vertices[first_vertex].z < -query->collision_radius &&
         query->transformed_vertices[second_vertex].z < -query->collision_radius) ||
        (query->transformed_vertices[first_vertex].z > query->collision_radius &&
         query->transformed_vertices[second_vertex].z > query->collision_radius)) {
        return 0;
    }

    const f32 closest_parameter = -query->transformed_vertices[first_vertex].x * edge_direction.x -
                                  query->transformed_vertices[first_vertex].y * edge_direction.y -
                                  query->transformed_vertices[first_vertex].z * edge_direction.z;
    const f32 edge_projection =
        (query->transformed_vertices[second_vertex].x - query->transformed_vertices[first_vertex].x) *
            edge_direction.x +
        (query->transformed_vertices[second_vertex].y - query->transformed_vertices[first_vertex].y) *
            edge_direction.y +
        (query->transformed_vertices[second_vertex].z - query->transformed_vertices[first_vertex].z) * edge_direction.z;
    if (closest_parameter < 0.0f || closest_parameter > edge_projection) {
        return 0;
    }

    // The normalized direction is no longer needed after the closest
    // parameter has been bounded, so reuse it for the closest point itself.
    edge_direction = {
        query->transformed_vertices[first_vertex].x + edge_direction.x * closest_parameter,
        query->transformed_vertices[first_vertex].y + edge_direction.y * closest_parameter,
        query->transformed_vertices[first_vertex].z + edge_direction.z * closest_parameter,
    };
    const f32 distance_sq =
        edge_direction.x * edge_direction.x + edge_direction.y * edge_direction.y + edge_direction.z * edge_direction.z;
    if (distance_sq >= query->collision_radius_sq) {
        return 0;
    }

    const f32 distance = NuFsqrt(distance_sq);
    const f32 overlap_time = distance - query->collision_radius;
    if (query->hit_time <= overlap_time) {
        return 0;
    }

    RotateVec(&edge_direction, &horizontal_perpendicular);
    const NUVEC &surface_normal = query->working_surface->normals[0];
    if (horizontal_perpendicular.x * surface_normal.x + horizontal_perpendicular.y * surface_normal.y +
            horizontal_perpendicular.z * surface_normal.z <
        0.0f) {
        query = TerI;
        query->hit_time = overlap_time;
        f32 inverse_distance = 0.0f;
        if (distance != 0.0f) {
            inverse_distance = 1.0f / distance;
        }
        query->hit_type = TERRAIN_HIT_TYPE_CYLINDER | TERRAIN_HIT_TYPE_SECOND_NORMAL;
        query->movement_normal.x = -edge_direction.x * inverse_distance;
        query->movement_normal.y = -edge_direction.y * inverse_distance;
        query->movement_normal.z = -edge_direction.z * inverse_distance;
        return 1;
    }
    return 0;
}

i32 CheckSphereTer(NUVEC *position, f32 radius) {
    const f32 terrain_radius = TerI->collision_radius;
    const f32 combined_radius = radius + terrain_radius;

    if (position->z < -combined_radius || position->z > TerI->movement_length + combined_radius) {
        return 0;
    }

    const f32 radial_distance_sq = position->x * position->x + position->y * position->y;
    const f32 combined_radius_sq = combined_radius * combined_radius;
    if (radial_distance_sq > combined_radius_sq) {
        return 0;
    }

    const f32 radial_extent = NuFsqrt(combined_radius_sq - radial_distance_sq);
    const f32 hit_distance = position->z - radial_extent;
    if (hit_distance >= 0.0f && hit_distance <= TerI->movement_length) {
        const f32 hit_time = hit_distance / TerI->movement_length;
        if (hit_time >= TerI->hit_time) {
            return 0;
        }

        TerI->hit_time = hit_time;
        TerI->hit_type = TERRAIN_HIT_TYPE_SPHERE;
        TerI->movement_normal.x = -position->x;
        TerI->movement_normal.y = -position->y;
        TerI->movement_normal.z = -radial_extent;
        return 1;
    }

    const f32 distance_sq = radial_distance_sq + position->z * position->z;
    if (distance_sq >= combined_radius_sq) {
        return 0;
    }

    const f32 distance = NuFsqrt(distance_sq);
    f32 inverse_distance = 0.0f;
    if (distance != 0.0f) {
        inverse_distance = 1.0f / distance;
    }

    TerI->hit_time = 0.0f;
    TerI->hit_type = TERRAIN_HIT_TYPE_SPHERE | TERRAIN_HIT_TYPE_SECOND_NORMAL;
    TerI->movement_normal.x = -position->x * inverse_distance;
    TerI->movement_normal.y = -position->y * inverse_distance;
    TerI->movement_normal.z = -position->z * inverse_distance;
    return 1;
}

i16 InsidePolLines(f32 point_x, f32 point_y, f32 point_z, f32 edge_a_x, f32 edge_a_y, f32 edge_a_z, f32 edge_b_x,
                   f32 edge_b_y, f32 edge_b_z, NUVEC *normal) {
    // Project onto the plane perpendicular to the dominant normal component.
    // The call order preserves the polygon winding in that projection.
    if (NuFabs(normal->y) >= NuFabs(normal->x) && NuFabs(normal->y) >= NuFabs(normal->z)) {
        if (0.0f > normal->y) {
            if (InsideLineF(point_x, point_z, 0.0f, 0.0f, edge_a_x, edge_a_z) == 0 ||
                InsideLineF(point_x, point_z, edge_b_x, edge_b_z, 0.0f, 0.0f) == 0) {
                return 0;
            }
            return InsideLineF(point_x, point_z, edge_a_x, edge_a_z, edge_b_x, edge_b_z) != 0;
        }

        if (InsideLineF(point_x, point_z, edge_a_x, edge_a_z, 0.0f, 0.0f) == 0 ||
            InsideLineF(point_x, point_z, 0.0f, 0.0f, edge_b_x, edge_b_z) == 0) {
            return 0;
        }
        return InsideLineF(point_x, point_z, edge_b_x, edge_b_z, edge_a_x, edge_a_z) != 0;
    }

    if (NuFabs(normal->y) > NuFabs(normal->x) || NuFabs(normal->z) > NuFabs(normal->x)) {
        if (0.0f > normal->z) {
            if (InsideLineF(point_y, point_x, 0.0f, 0.0f, edge_a_y, edge_a_x) == 0 ||
                InsideLineF(point_y, point_x, edge_b_y, edge_b_x, 0.0f, 0.0f) == 0) {
                return 0;
            }
            return InsideLineF(point_y, point_x, edge_a_y, edge_a_x, edge_b_y, edge_b_x) != 0;
        }

        if (InsideLineF(point_y, point_x, edge_a_y, edge_a_x, 0.0f, 0.0f) == 0 ||
            InsideLineF(point_y, point_x, 0.0f, 0.0f, edge_b_y, edge_b_x) == 0) {
            return 0;
        }
        return InsideLineF(point_y, point_x, edge_b_y, edge_b_x, edge_a_y, edge_a_x) != 0;
    }

    if (0.0f > normal->x) {
        if (InsideLineF(point_y, point_z, edge_a_y, edge_a_z, 0.0f, 0.0f) == 0 ||
            InsideLineF(point_y, point_z, 0.0f, 0.0f, edge_b_y, edge_b_z) == 0) {
            return 0;
        }
        return InsideLineF(point_y, point_z, edge_b_y, edge_b_z, edge_a_y, edge_a_z) != 0;
    }

    if (InsideLineF(point_y, point_z, 0.0f, 0.0f, edge_a_y, edge_a_z) == 0 ||
        InsideLineF(point_y, point_z, edge_b_y, edge_b_z, 0.0f, 0.0f) == 0) {
        return 0;
    }
    return InsideLineF(point_y, point_z, edge_a_y, edge_a_z, edge_b_y, edge_b_z) != 0;
}

u16 ObjHitObj_Flags(GameObject_s *object) {
    if (object == NULL)
        return 0;
    const bool player = (object->apiobj.flags_low & 0x80) != 0;
    const u16 ordinary = player ? 0x80c : 0x00a;
    const u16 special = player ? 0x824 : 0x022;
    const u16 scripted = player ? 0x804 : 0x002;
    if ((object->apiobj.field_0x1f4 & 0x10001) != 0)
        return scripted | 0x10;
    return (object->apiobj.field_0x1f4 & 4) != 0 ? special : ordinary;
}

void CollideGameObjects(WORLDINFO_s *world) {
    if (world->level_sub_id != -1 && (world->area->flags & 0x80) != 0) {
        return;
    }

    u32 narrow_speeder_mask = 0;
    u32 flattened_mask = 0;
    u32 player_collision_mask = 0;
    u32 no_vertical_movement_mask = 0;
    u32 vertical_movement_mask = 0;
    u32 player_slot_mask = 0;

    i32 object_count = HIGHGAMEOBJECT;
    GameObject_s *objects = Obj;
    GameObject_s *object = objects;
    for (i32 index = 0; index < object_count; ++index, ++object) {
        const i32 flags = object->apiobj.flags_low;
        if ((flags & APIOBJECT_FLAG_IN_USE) == 0) {
            continue;
        }

        const u32 bit = static_cast<u32>(static_cast<u64>(1) << object->apiobj.field_0x289);
        if (object->apiobj.field_0x27c != -1) {
            player_slot_mask |= bit;
        }
        GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
        if (!(data->field_0x28 > 0.0f)) {
            no_vertical_movement_mask |= bit;
        } else {
            vertical_movement_mask |= bit;
        }
        if (((flags & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0 || (object->field_0xf02 & 8) != 0) &&
            (data->flags_090 & 0x40) == 0) {
            player_collision_mask |= bit;
        }
        if (object->character_context == 0x3d && (flags & APIOBJECT_FLAG_PLAYER_ACTIVE) == 0) {
            flattened_mask |= bit;
        }
        if (WORLD->current_level == SPEEDERCHASEA_LDATA && disable_narrow_socks == 0 && object->id == id_SPEEDERBIKE &&
            object->apiobj.field_0x27c != -1) {
            narrow_speeder_mask |= bit;
        }
    }

    APIOBJECT *collision_objects[64];
    NUVEC collision_minimums[64];
    NUVEC collision_maximums[64];
    i32 collision_count = 0;
    object = objects;
    for (i32 index = 0; index < object_count; ++index, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
            object->apiobj.model_draw_result == 0 || object->use_model_origin <= 1) {
            continue;
        }

        const i8 context = object->character_context;
        if ((CInfo[context].flags & 0x8000) != 0 || context == 0x3b || context == 0x3c || context == 0x0f ||
            context == 0x39) {
            continue;
        }
        if (context == 0x26 && object->field_0x7a7 != -1 && (object->context_variant_flags & 2) != 0) {
            continue;
        }
        if (context == 0x1f && object->field_0x7a3 == 1) {
            continue;
        }

        GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
        if ((data->flags_090 & 0x8000) != 0 || (object->action_flags & 0x20) != 0 ||
            (context == 0x47 && object->field_0x788 != NULL &&
             (*reinterpret_cast<u8 *>(static_cast<u8 *>(object->field_0x788) + 0x68) & 1) != 0)) {
            continue;
        }

        collision_objects[collision_count] = &object->apiobj;
        collision_minimums[collision_count] = object->apiobj.collision_min;
        collision_maximums[collision_count] = object->apiobj.collision_max;
        ++collision_count;

        object->apiobj.collision_mask_low = 0;
        object->apiobj.collision_mask_high = 0;
        if ((data->flags_090 & 0x1000) != 0) {
            object->apiobj.collision_mask_low = flattened_mask;
        }

        if (object->field_0x107c != -1) {
            object->apiobj.collision_mask_low |= player_collision_mask;
            for (i32 player_index = 0; player_index < 8; ++player_index) {
                GameObject_s *player = Player[player_index];
                if (player == NULL || (player->apiobj.field_0x1f8 & 0x1001) != 0x1001 || player == object ||
                    !(player->field_0xdc4 > 0.0f) || player->apiobj.supporting_platform_id != object->field_0x107c ||
                    (object->field_0x107c == player->field_0x1078 &&
                     (player->apiobj.field_0x27d != 0 || GameObjectNearFloor(player, 3.0f, NULL) != 0))) {
                    continue;
                }
                NUVEC direction = {object->apiobj.collision_position.x - player->apiobj.collision_position.x, 0.0f,
                                   object->apiobj.collision_position.z - player->apiobj.collision_position.z};
                NuVecNorm(&direction, &direction);
                object->apiobj.movement_direction.x = direction.x;
                object->apiobj.movement_direction.z = direction.z;
            }
        }

        if (object->character_context == CHARACTER_CONTEXT_JUMP && object->airborne_collision_target != NULL) {
            const u64 bit = static_cast<u64>(1) << object->airborne_collision_target->apiobj.field_0x289;
            object->apiobj.collision_exclusion_mask |= bit;
        }
        if (WORLD->current_level == SPEEDERCHASEA_LDATA && disable_narrow_socks == 0 && object->id == id_SPEEDERBIKE &&
            object->apiobj.field_0x27c != -1) {
            object->apiobj.collision_mask_low |= narrow_speeder_mask;
        }

        if (VehicleArea != 0 && WORLD->current_level != BOUNTYHUNTERPURSUITA_LDATA &&
            WORLD->current_level != DOGFIGHTA_LDATA) {
            data = object->apiobj.character_data->game_character;
            if (object->apiobj.field_0x27c == -1 && data->field_0x28 != 0.0f) {
                object->apiobj.collision_mask_low |=
                    (vertical_movement_mask & player_slot_mask) | no_vertical_movement_mask;
            } else if (data->field_0x28 == 0.0f) {
                object->apiobj.collision_mask_low |= (~player_slot_mask) & vertical_movement_mask;
            } else {
                object->apiobj.collision_mask_low |= vertical_movement_mask;
            }
        }
    }

    APIObjectCollisions(collision_count, collision_objects, collision_minimums, collision_maximums, Collide2Objects);
}

void CalculateRayBoxIntersection(VuVec const &, VuVec const &, VuVec const &, VuVec const &, float, float &) {
}

f32 CalcCapsuleIntersectDistance(VuVec const &start, VuVec const &direction, f32 maximum_distance, VuVec const &centre,
                                 f32 radius) {
    const f32 delta_x = centre.x - start.x;
    const f32 delta_y = centre.y - start.y;
    const f32 delta_z = centre.z - start.z;
    f32 distance = direction.x * delta_x + direction.y * delta_y + direction.z * delta_z;

    if (distance >= 0.0f && distance < maximum_distance) {
        const f32 distance_from_axis_squared =
            delta_x * delta_x + delta_y * delta_y + delta_z * delta_z - distance * distance;
        if (radius * radius >= distance_from_axis_squared) {
            return distance;
        }
    }
    return 1.0e9f;
}

i32 HitPoly(f32 primary_start, f32 primary_end, f32 secondary_start, f32 secondary_end, tertype *surface) {
    TerrainQuery_s *query = TerI;
    const f32 radius = query->collision_radius;
    const f32 no_secondary_normal = 65536.0f;

    i32 collision_found = 0;
    f32 candidate_time = 0.0f;
    NUVEC contact_point;

    const NUVEC &primary_origin = surface->vectors[0];
    NUVEC &primary_normal = surface->normals[0];
    contact_point.x = query->local_start.x - radius * primary_normal.x - primary_origin.x;
    contact_point.y = query->local_start.y - radius * primary_normal.y - primary_origin.y;
    contact_point.z = query->local_start.z - radius * primary_normal.z - primary_origin.z;

    i32 test_primary_face = 0;
    if (primary_start > 0.0f && primary_end < 0.0f) {
        candidate_time = primary_start / (primary_start - primary_end);
        contact_point.x += query->movement.x * candidate_time;
        contact_point.y += query->movement.y * candidate_time;
        contact_point.z += query->movement.z * candidate_time;
        test_primary_face = 1;
    } else if (primary_start <= 0.0f && primary_end <= 0.0f && primary_start >= -radius && primary_end >= -radius) {
        candidate_time = primary_start;
        const f32 projection_distance = -primary_start;
        contact_point.x += primary_normal.x * projection_distance;
        contact_point.y += primary_normal.y * projection_distance;
        contact_point.z += primary_normal.z * projection_distance;
        test_primary_face = 1;
    }

    if (test_primary_face != 0) {
        const NUVEC edge_0_1 = {
            surface->vectors[1].x - primary_origin.x,
            surface->vectors[1].y - primary_origin.y,
            surface->vectors[1].z - primary_origin.z,
        };
        const NUVEC edge_0_2 = {
            surface->vectors[2].x - primary_origin.x,
            surface->vectors[2].y - primary_origin.y,
            surface->vectors[2].z - primary_origin.z,
        };
        if (InsidePolLines(contact_point.x, contact_point.y, contact_point.z, edge_0_1.x, edge_0_1.y, edge_0_1.z,
                           edge_0_2.x, edge_0_2.y, edge_0_2.z, &primary_normal) != 0 &&
            candidate_time <= query->hit_time) {
            query->hit_time = candidate_time;
            query->hit_type =
                primary_start > 0.0f ? TERRAIN_HIT_TYPE_FACE : TERRAIN_HIT_TYPE_FACE | TERRAIN_HIT_TYPE_SECOND_NORMAL;
            query->movement_normal = primary_normal;
            query->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP) != 0;
            collision_found = 1;
        }
    }

    NUVEC &secondary_normal = surface->normals[1];
    if (secondary_normal.y < no_secondary_normal) {
        const NUVEC &secondary_origin = surface->vectors[3];
        contact_point.x = query->local_start.x - radius * secondary_normal.x - secondary_origin.x;
        contact_point.y = query->local_start.y - radius * secondary_normal.y - secondary_origin.y;
        contact_point.z = query->local_start.z - radius * secondary_normal.z - secondary_origin.z;

        i32 test_secondary_face = 0;
        if (secondary_start > 0.0f && secondary_end < 0.0f) {
            candidate_time = secondary_start / (secondary_start - secondary_end);
            contact_point.x += query->movement.x * candidate_time;
            contact_point.y += query->movement.y * candidate_time;
            contact_point.z += query->movement.z * candidate_time;
            test_secondary_face = 1;
        } else if (secondary_start <= 0.0f && secondary_end <= 0.0f && secondary_start >= -radius &&
                   secondary_end >= -radius) {
            candidate_time = secondary_start;
            const f32 projection_distance = -secondary_start;
            contact_point.x += secondary_normal.x * projection_distance;
            contact_point.y += secondary_normal.y * projection_distance;
            contact_point.z += secondary_normal.z * projection_distance;
            test_secondary_face = 1;
        }

        if (test_secondary_face != 0) {
            const NUVEC edge_3_2 = {
                surface->vectors[2].x - secondary_origin.x,
                surface->vectors[2].y - secondary_origin.y,
                surface->vectors[2].z - secondary_origin.z,
            };
            const NUVEC edge_3_1 = {
                surface->vectors[1].x - secondary_origin.x,
                surface->vectors[1].y - secondary_origin.y,
                surface->vectors[1].z - secondary_origin.z,
            };
            if (InsidePolLines(contact_point.x, contact_point.y, contact_point.z, edge_3_2.x, edge_3_2.y, edge_3_2.z,
                               edge_3_1.x, edge_3_1.y, edge_3_1.z, &secondary_normal) != 0 &&
                candidate_time < query->hit_time) {
                query->hit_time = candidate_time;
                query->hit_type = secondary_start > 0.0f ? TERRAIN_HIT_TYPE_FACE
                                                         : TERRAIN_HIT_TYPE_FACE | TERRAIN_HIT_TYPE_SECOND_NORMAL;
                query->movement_normal = secondary_normal;
                query->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP) != 0;
                collision_found = 1;
            }
        }
    }

    if (radius != 0.0f) {
        query->working_surface = surface;
        i32 vertex_mask = 0xf;
        DeRotateTerrain(surface);

        if (primary_start >= -radius) {
            if (CheckCylinder(0, 1, &vertex_mask, 0xc) != 0) {
                query = TerI;
                query->hit_edge = TERRAIN_HIT_EDGE_0_1;
                query->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP) != 0;
                collision_found = 1;
            }
            if (CheckCylinder(2, 0, &vertex_mask, 0xa) != 0) {
                query = TerI;
                query->hit_edge = TERRAIN_HIT_EDGE_2_0;
                query->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP) != 0;
                collision_found = 1;
            }
        }

        if (CheckCylinder(1, 2, &vertex_mask, 0x9) != 0) {
            query = TerI;
            query->hit_edge = TERRAIN_HIT_EDGE_1_2;
            query->shape_adjusted = (surface->normal_flags & (TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP |
                                                              TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP)) != 0;
            collision_found = 1;
        }

        if (secondary_normal.y < no_secondary_normal && secondary_start >= -radius) {
            if (CheckCylinder(1, 3, &vertex_mask, 0x5) != 0) {
                query = TerI;
                query->hit_edge = TERRAIN_HIT_EDGE_1_3;
                query->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP) != 0;
                collision_found = 1;
            }
            if (CheckCylinder(3, 2, &vertex_mask, 0x3) != 0) {
                query = TerI;
                query->hit_edge = TERRAIN_HIT_EDGE_3_2;
                query->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP) != 0;
                collision_found = 1;
            }
            if ((vertex_mask & (1 << 3)) != 0 && CheckSphere(3) != 0) {
                TerI->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP) != 0;
                collision_found = 1;
            }
        }

        if (primary_start >= -radius) {
            if ((vertex_mask & (1 << 0)) != 0 && CheckSphere(0) != 0) {
                TerI->shape_adjusted = (surface->normal_flags & TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP) != 0;
                collision_found = 1;
            }
            if ((vertex_mask & (1 << 1)) != 0 && CheckSphere(1) != 0) {
                TerI->shape_adjusted = (surface->normal_flags & (TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP |
                                                                 TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP)) != 0;
                collision_found = 1;
            }
        }

        if ((vertex_mask & (1 << 2)) != 0 && CheckSphere(2) != 0) {
            TerI->shape_adjusted = (surface->normal_flags & (TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP |
                                                             TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP)) != 0;
            collision_found = 1;
        }
    }

    query = TerI;
    if (collision_found != 0) {
        query->surface = surface;
    }
    query->unclamped_hit_time = query->hit_time;
    if (query->hit_time < 0.0f) {
        query->hit_time = 0.0f;
    }
    return collision_found;
}

i32 CheckCol(nutex_s *, i32, i32, i32, i32) {
    return true;
}

void HitRumble(GameObject_s *) {
}

i32 ObjHitObj(GameObject_s *attacker, GameObject_s *target, i32 damage, u16 flags, i32 probe, i32) {
    if (target == NULL || (target->apiobj.field_0x1f8 & 0x1001) != 0x1001 || target->apiobj.field_0x287 != 0 ||
        target->field_0x101c > 0.0f)
        return 0;
    i32 no_hurt = objhitobj_nohurtsfx;
    i32 no_impact = objhitobj_noimpactsfx;
    i32 no_rumble = objhitobj_noattackerrumble;
    i32 throw_up = objhitobj_throwkillpartsup;
    i16 *parts_angle = objhitobj_killparts_yrot;
    i32 random = qrand();
    objhitobj_nohurtsfx = 0;
    objhitobj_noimpactsfx = 0;
    objhitobj_noattackerrumble = 0;
    objhitobj_throwkillpartsup = 0;
    objhitobj_killparts_yrot = NULL;
    BOLT_s *bolt = objhitobj_bolt;
    objhitobj_bolt = NULL;
    if (Hub_InMenu() && (target->apiobj.flags_low & 0x80))
        return 0;
    if (target->apiobj.character_data->game_character->flags_090 & 0x8000)
        return 0;
    if (target->character_context == 95 || target->character_context == 96)
        return 0;
    if (target->character_context == 90 && (!target->field_0x7a3 || (attacker && !(attacker->apiobj.flags_low & 0x80))))
        return 0;
    if (!(damage == -1 && (flags & 0x200))) {
        if (target->character_context == 0 &&
            (target->action_movement_state == 3 || target->action_movement_state == 4))
            return 0;
        if (target->character_context == 13 || target->character_context == 14)
            return 0;
    }
    if (CInfo[target->character_context].parameter & 4)
        return 0;
    if (attacker && !(attacker->apiobj.flags_low & 1))
        attacker = NULL;
    if (target->apiobj.field_0x27c != -1)
        ObstacleCamHoldUntilPlayersMove = 0;
    i32 hit = damage;
    bool instant = damage == -1;
    if (target->character_context == 76 || target->character_context == 81) {
        hit = 0;
        instant = false;
    }
    if (attacker && (attacker->apiobj.flags_low & 0x80))
        AlertSurroundingCreatures(attacker, &target->apiobj.collision_position);
    if (hit > 0 || instant)
        target->field_0xef8 |= 1;
    if (!(flags & 0x100)) {
        damage = 0;
    } else {
        if ((CInfo[target->character_context].flags & 0x4000000) ||
            (target->character_context != -1 && (target->character_context == LEGOCONTEXT_LAND_SLAM ||
                                                 target->character_context == LEGOCONTEXT_LAND_LUNGE))) {
            i32 angle = RotDiff(attacker->apiobj.movement_facing_angle, target->apiobj.movement_facing_angle);
            if (angle < 0)
                angle = -angle;
            if (angle > 0x4000 && !(LEGOCONTEXT_COMBO != -1 && attacker->character_context == LEGOCONTEXT_COMBO &&
                                    attacker->combo_branch == 6)) {
                if (target->apiobj.flags_low & 0x80)
                    NewRumble(target->pad_gamepad->pad, 0.75f, 0);
                if (LEGOCONTEXT_HOLD != -1 && target->character_context == LEGOCONTEXT_HOLD) {
                    NewBlockAction(target);
                    if ((target->id == id_IMPERIALGUARD || target->id == id_GAMORREANGUARD) &&
                        (target->character_context == 24 || target->character_context == 12))
                        GameAudio_PlaySfx(74, &target->apiobj.collision_position, 0, 0);
                }
                return 0;
            }
        }
    }
    if (target->field_0xd24 >= 1.0f) {
        if ((flags & 0x100) &&
            (instant || (LEGOCONTEXT_COMBO != -1 && attacker->character_context == LEGOCONTEXT_COMBO &&
                         attacker->combo_branch == 6)))
            damage = target->field_0xe37;
        ObjHitShield(attacker, target, damage, bolt);
        return 0;
    }
    if (flags == 0)
        flags = ObjHitObj_Flags(attacker);
    if ((target->field_0xefb & 8) || WORLD->current_level == VADERC_LDATA) {
        if (static_cast<u32>(hit) >= 2)
            hit = 1;
    } else if (attacker && (flags & 0x100) && attacker->character_context == 5 && attacker->combo_branch == 6 &&
               target->apiobj.field_0x27c == -1) {
        hit = -1;
    }
    if ((target->field_0xefa & 8) && (flags & 0x80)) {
        hit = 0;
    } else {
        if (WORLD->current_level == SPEEDERCHASEA_LDATA) {
            if (!disable_narrow_socks && (flags & 4) && (target->apiobj.flags_low & 0x80)) {
                hit = 0;
                goto attributed_hit;
            }
            if (target->id == id_SPEEDERBIKE && !(target->apiobj.flags_low & 0x80)) {
                if (attacker && !(attacker->apiobj.character_data->model_flags & 0x2000))
                    hit = 0;
                else if (target->ai.creature_set != 2)
                    hit = 0;
                goto attributed_hit;
            }
        }
        if (WORLD->area == HOTHBATTLE_ADATA &&
            ((target->id == id_ATAT && !(target->apiobj.flags_low & 0x80) &&
              (!attacker || !(attacker->apiobj.flags_low & 0x80))) ||
             (attacker && attacker->id == id_ATAT && !(target->apiobj.flags_low & 0x80) &&
              !(attacker->apiobj.flags_low & 0x80)))) {
            if (hit == -1 && target->id == id_ATAT) {
                if (target->character_context != 23)
                    hit = 0;
            } else
                hit = attacker && attacker->id == id_ATAT && target->id == id_DRAGBOMB ? -1 : 0;
            goto attributed_hit;
        }
        if (flags & 1)
            goto attributed_hit;
        if ((flags & 2) && !(target->apiobj.flags_low & 0x80) && WORLD->current_level != HUB_LDATA) {
            hit = 0;
            goto attributed_hit;
        }
        if ((flags & 4) && (target->apiobj.field_0x1f4 & 0x10405) == 0x400 && WORLD->current_level != HUB_LDATA) {
            hit = 0;
            goto attributed_hit;
        }
        if ((flags & 12) == 8 && (target->apiobj.flags_low & 0x80) && WORLD->current_level != HUB_LDATA &&
            WORLD->current_level != VADERC_LDATA) {
            hit = 0;
            goto attributed_hit;
        }
        if ((MiniCutCam && (target->apiobj.flags_low & 0x80)) || target->pad_gamepad == ViewCamGetGamePad() ||
            ((target->apiobj.character_data->model_flags & 0x20000000) && target->field_0xcc0 == NULL))
            hit = 0;
    }
attributed_hit:
    if (attacker) {
        target->last_attacker = attacker;
        if (attacker->apiobj.flags_low & 0x80) {
            if ((target->apiobj.character_data->game_character->flags_090 & 0x40) && !VehicleArea &&
                (WORLD->current_level != SPEEDERCHASEA_LDATA || disable_narrow_socks))
                hit = -1;
            if (WORLD->current_level == HUB_LDATA &&
                AIScriptSetBaseScriptStateByName(&target->ai.script_process, "TakenHitFromPlayer")) {
                AIScriptProcess(WORLD->ai_sys, &target->apiobj, &target->ai, &target->ai.script_process, FRAMETIME);
                goto impact;
            }
        }
    }
    if (AIScriptSetBaseScriptStateByName(&target->ai.script_process, "TakenHit"))
        AIScriptProcess(WORLD->ai_sys, &target->apiobj, &target->ai, &target->ai.script_process, FRAMETIME);
impact:
    if (!no_impact) {
        if (attacker && attacker->id == id_BODYGUARD)
            PlaySfx("Grv_GuardImpact", &target->apiobj.collision_position);
        else if (attacker && attacker->id == id_IMPERIALGUARD)
            PlaySfx("wpn_bib_stab", &target->apiobj.collision_position);
        else if (bolt) {
            if (!(bolt->type_id >= 27 && bolt->type_id <= 29) && WORLD->area &&
                (WORLD->area == PODRACE_ADATA || WORLD->area == PODSPRINT_ADATA)) {
                PlaySfx("Pod_TuskHit", &bolt->position);
                no_hurt = 1;
            }
        } else if (VehicleArea || (WORLD->current_level == SPEEDERCHASEA_LDATA && !disable_narrow_socks)) {
            i32 bits =
                attacker && static_cast<u8>(attacker->apiobj.field_0x27c) <= 1 ? 1 << attacker->apiobj.field_0x27c : 0;
            GameAudio_PlaySfx(40, &target->apiobj.collision_position, bits, 0);
        } else if (flags & 0x40) {
            if (attacker && attacker->character_context == 38 &&
                (AnimMiscFlags(attacker->apiobj.character_model, attacker->context_animation) & 4))
                PlaySfx("WhipHit", &target->apiobj.collision_position);
            else {
                i32 bits = attacker && static_cast<u8>(attacker->apiobj.field_0x27c) <= 1
                               ? 1 << attacker->apiobj.field_0x27c
                               : 0;
                GameAudio_PlaySfx(74, &target->apiobj.collision_position, bits, 0);
            }
        } else if (attacker) {
            if (attacker->apiobj.character_data->model_flags & 8)
                GameAudio_PlaySfx(65, &target->apiobj.collision_position, GameAudio_GetPlrSfxBits(attacker), 0);
            else
                GameAudio_PlaySfx(74, &target->apiobj.collision_position, 0, 0);
        }
    }
    i32 result;
    i32 health;
    i32 coins;
    i32 hearts;
    u16 computed_angle;
    if (target->spawn_protection_timer > 0.0f || (target->field_0xefe & 0x40)) {
        result = 0;
        if (target->field_0xefd & 0x10)
            PlayerTakeHit(target, attacker);
        goto finish;
    }
    if (target->character_context == 21 && (target->field_0xefb & 8)) {
        target->spawn_protection_timer = 2.5f;
        result = 0;
        goto finish;
    }
    if ((target->apiobj.character_data->model_flags & 0x10) && !target->current_hp && (flags & 0x40) &&
        static_cast<f32>(random) * 1.5259021893143654e-05f > 0.75f) {
        DeactivatePlayer(target, 5.0f, NULL);
        result = 0;
        goto finish;
    }
    if (hit != -1 && !(target->flicker_time <= 0.0f) && (!attacker || !(attacker->apiobj.flags_low & 0x80)))
        return 0;
    if (!(hit > 0 && (target->apiobj.flags_low & 0x80))) {
        if (target->character_context == 45 && !Player_HasFastBuild(target))
            GizBuildIt_SetToStart(static_cast<GIZBUILDIT_s *>(target->field_0x788), 1, 1);
        Player_ClearContext(target, 0);
        ReleaseForce(target, 1);
    }
    if (target->field_0x108e && !TouchHacks::TouchControlsActive)
        LoseHelmet(target, 0, 0);
    if (hit != -1) {
        if (!target->current_hp || ((target->apiobj.flags_low & 0x80) && Player_HasInvincibility(target))) {
            if (target->id == id_ROYALGUARD || target->id == id_WOOKIEE)
                SetFlicker(target, 0.4f);
            PlayerTakeHit(target, attacker);
            result = 0;
            if (target->apiobj.flags_low & 0x80)
                TakeHitRumble(target, 0.666f);
            goto hurt;
        }
        health = target->current_hp - hit;
        if (health > 0) {
            if ((target->apiobj.character_data->model_flags & 0x40000000) && target->character_context == 23)
                goto refill;
            goto surviving_hit;
        }
    } else
        health = 0;
    no_impact = players_cannot_exit_speeder && target->id == id_SPEEDERBIKE && target->field_0xcc0 &&
                target->apiobj.field_0x27c != -1;
    if ((target->apiobj.character_data->model_flags & 0x20000000) && !ObjIsTargetSpeeder(target) && !no_impact) {
        coins = 0;
        if ((target->apiobj.flags_low & 0x80) && target->coinpacket && target->coinpacket->coins && BonusWinner == -1)
            coins = LoseCoins(target, 1);
        AddPickups(coins, 0, 0, 0, &target->apiobj.collision_position, NULL, 2.0f,
                   attacker ? attacker->apiobj.field_0x27c : -1, 1.0f, 2000000.0f, attacker, 1, 0, false);
        DeactivatePlayer(target, 1000000000.0f, NULL);
        target->current_hp = target->hitpoints;
        SetFlicker(target, 0.4f);
        result = 0;
        goto hurt;
    }
    if (target->apiobj.character_data->model_flags & 0x40000000)
        goto refill;
    if ((target->field_0xefb & 8) && (!FreePlay || WORLD->current_level != CRUISERC_LDATA))
        goto surviving_hit;
    if (probe) {
        result = 2;
        goto finish;
    }
    if (target->character_context == 93) {
        PopBalloon(target);
        target->current_hp = 1;
        result = 0;
        goto hurt;
    }
    target->current_hp = 0;
    coins = 0;
    hearts = 0;
    if (target->apiobj.field_0x27c == -1) {
        if (!(target->apiobj.flags_low & 0x80)) {
            coins = BonusArea ? static_cast<u16>(target->apiobj.character_data->game_character->field_0xee)
                              : (Cheat_IsOn(16) ? 350 : 0);
            hearts = ReleaseHearts();
        }
    } else if (target->apiobj.flags_low & 0x80) {
        if (target->coinpacket && target->coinpacket->coins && BonusWinner == -1)
            coins = LoseCoins(target, 1);
        if (!BuildUpDone)
            BuildUpScale = 1.5f;
        DrawBuildUpTime = 1.0f;
        builduptime = 1.0f;
    }
    if (hearts || coins > 0)
        AddPickups(coins, hearts, 0, 0, &target->apiobj.collision_position, NULL, 2.0f, -1, 1.0f, 2000000.0f, attacker,
                   !BonusArea && coins < 2500, BonusArea ? 1 : 0, false);
    if (target->torpedo && WORLD->area != SPEEDERCHASE_ADATA && (!WORLD->area || !(WORLD->area->flags & 4)))
        DropTorpedoPickups(target->torpedo, target->torpedo->count);
    if (!parts_angle) {
        if (!attacker)
            parts_angle = NULL;
        else if (flags & 0x240) {
            computed_angle = NuAtan2D(target->apiobj.position.x - attacker->apiobj.position.x,
                                      target->apiobj.position.z - attacker->apiobj.position.z);
            parts_angle = reinterpret_cast<i16 *>(&computed_angle);
        }
    }
    if (WORLD->current_level == VADERC_LDATA && !netclient) {
        GIZAIMESSAGE_s *message = CheckGizAIMessage(gizaimessagesys, "FinalFight", NULL);
        if (message && message->value == 1.0f && (!attacker || (attacker->apiobj.flags_low & 0x80))) {
            grab_screen_image = 1;
            if (FreePlay)
                CompleteLevel(WORLD);
            else {
                char name[16];
                nuhspecial_s special;
                for (i32 i = 1; i != 12; ++i) {
                    sprintf(name, "rock%d", i);
                    if (NuSpecialFind(WORLD->current_gscn, &special, name, 1))
                        NuSpecialSetVisibility(&special, 0);
                }
                NewCutScene(NULL, WORLD->cutscene_sys,
                            attacker && attacker->id == id_OBIWANKENOBIEP3 && target->id == id_ANAKINJEDI
                                ? const_cast<char *>("ep3_darthvader_outro2")
                                : const_cast<char *>("ep3_darthvader_outro1"),
                            1);
            }
            return 0;
        }
    }
    if (no_impact && target->field_0xcc0) {
        KillParts(target->field_0xcc0, -1, target->field_0xcc0->id == id_BODYGUARD ? 4 : -1, 1, 0.0f, 0,
                  reinterpret_cast<u16 *>(parts_angle));
        KillGameObject(target->field_0xcc0, 2, 0);
    }
    KillParts(target, -1, target->id == id_BODYGUARD ? 4 : -1, 1, throw_up ? 1.0f : 0.0f, 0,
              reinterpret_cast<u16 *>(parts_angle));
    KillGameObject(target, 2, 0);
    if (target->apiobj.flags_low & 0x80)
        GameCam_Judder(GameCam, 0.2f, 0, NULL);
    result = 2;
    goto finish;
surviving_hit:
    if (target->apiobj.flags_low & 0x80)
        TakeHitRumble(target, 0.666f);
    target->current_hp = health;
    if (hit != 0) {
        if (AIScriptSetBaseScriptStateByName(&target->ai.script_process, "LostHitPoints"))
            AIScriptProcess(WORLD->ai_sys, &target->apiobj, &target->ai, &target->ai.script_process, FRAMETIME);
        SetFlicker(target, 0.4f);
        if (target->id == id_SNAKE)
            SnakeBeenHit(target);
        PlayerTakeHit(target, attacker);
        if (target->apiobj.field_0x27c != -1 && hit > 0) {
            u8 value = target->field_0xe38 - hit;
            target->field_0xe38 = value ? value : 1;
            if ((target->apiobj.character_data->model_flags & 0x20) && target->apiobj.field_0x288) {
                i32 index = target->field_0xe38 - 1;
                if (index > 2)
                    index = 2;
                index = objhit_damage_joints[2 - index];
                if (target->apiobj.character_model->points_of_interest[index])
                    AddGameDebris(WORLD->debris_sys, 113,
                                  reinterpret_cast<NUVEC *>(&target->joint_matrices[index].m30));
            }
        }
    } else
        PlayerTakeHit(target, attacker);
    result = 1;
    if (target->id == id_BODYGUARD && target->current_hp == 1)
        KillParts(target, 4, -1, 1, 0.0f, 0, NULL);
    goto hurt;
hurt:
    if (!no_hurt && target->character_context != 23)
        PlayHurtSfx(target);
    goto finish;
finish:
    if (attacker && (attacker->apiobj.flags_low & 0x80)) {
        if (!no_rumble) {
            if (result == 2)
                KillRumble(attacker);
            else
                HitRumble(attacker);
        }
        if (!(flags & 0x4000) && result == 2 && static_cast<u8>(attacker->apiobj.field_0x27c) <= 1 && Arcade)
            Arcade_Kill(attacker->apiobj.field_0x27c, target->apiobj.field_0x27c);
    }
    return result;
refill:
    coins = 0;
    if ((target->apiobj.flags_low & 0x80) && target->coinpacket && target->coinpacket->coins && BonusWinner == -1)
        coins = LoseCoins(target, 1);
    AddPickups(coins, 0, 0, 0, &target->apiobj.collision_position, NULL, 2.0f,
               attacker ? attacker->apiobj.field_0x27c : -1, 1.0f, 2000000.0f, attacker, 1, 0, false);
    if (target->character_context != 23)
        SetFlicker(target, 0.4f);
    Player_ClearContext(target, 1);
    target->current_hp = target->hitpoints;
    result = 0;
    goto hurt;
}

void TerrainMoveImpactData();

void RayImpact(NUVEC *movement) {
    TerrainMoveImpactData();
    switch (TerI->hit_type) {
        case 1:
        case 2:
        case 3:
        case 4: {
            TerI->hit_time -= TerI->separation_epsilon;
            if (TerI->hit_time < 0.0f)
                TerI->hit_time = 0.0f;
            f32 time = TerI->hit_time;
            movement->x = TerI->movement.x * time;
            movement->y = TerI->movement.y * time;
            movement->z = TerI->movement.z * time;
            break;
        }
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
            movement->x = 0.0f;
            movement->y = 0.0f;
            movement->z = 0.0f;
            break;
    }
}
