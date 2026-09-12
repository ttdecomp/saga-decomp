#include "legoapi/world/world_shared.h"

#include <stdio.h>

#include "gameapi/edtools/edstubs.h"
#include "gameapi/gui/apimenu.h"
#include "globals.h"
#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/gizmos/object/technos.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/level.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"

#include <limits.h>
#include <math.h>
#include <string.h>

struct tertype;
struct terrsitu_s;
struct PLATSKININFO;
struct GAMECAMERA_s;
struct pushblock_s;
extern "C" void PlatOnOff(i32 index, i32 enabled);
extern "C" void TerrainSetImpactData(void *impact_data, i32 *impact_count, i32 maximum_impacts);
extern "C" void TerrainPlatGetMtx(i32 index, NUMTX **previous, NUMTX **current);
extern "C" void TerrainSetPlatConnectTol(f32 tolerance);
void Surface_Deflect(NUVEC *normal, NUVEC *movement, NUVEC *result, i32 mode);
extern i16 id_ATST;
extern i16 id_SPEEDERBIKE;
void GetSurfaceInfo(GameObject_s *object, i32 update_surface, f32 shadow_height);
f32 FindReflectionNoPlatforms(NUVEC *position);
extern i32 TimingBarSet;
void TBOPENFN(char *, i32);
void TBCLOSEFN(char *, i32);
i32 NOTERRAINSTOP;
extern GameObject_s *CarWashHack;
GameObject_s *CharPlatform_FindObjFromPlatID(CHARPLATFORMSYS_s *system, i32 platform_id);
void StartFlatten(GameObject_s *source, GameObject_s *target);
void AddWaterSplash(GameObject_s *object, NUVEC *position);
i32 LoseCoins(GameObject_s *object, i32 cause);
i32 CannotKill(GameObject_s *object);
void AddPickups(i32, i32, i32, i32, NUVEC *, NUVEC *, f32, i32, f32, f32, GameObject_s *, i32, i32, bool);
extern i32 gone_through_door_to_new_level;
extern AREADATA_s *PODSPRINT_ADATA;
extern AREADATA_s *PODRACE_ADATA;
void NewRumble(nupad_s *pad, f32 strength, i32 mode);
void NewBuzz(nupad_s *pad, f32 duration, i32 mode);
void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 delay, i32 mode);
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
i32 NoLayerKill(GameObject_s *object);
void ClearLastSafeTakeOver(GameObject_s *object);
void Player_ClearContext(GameObject_s *object, i32 mode);
void PlayDieSfx(GameObject_s *object);
i32 StartBigJump(GameObject_s *, NUVEC *, i32, f32, f32, i32, i8);
extern i16 id_SNAKE, id_TRACTOR;
extern AREADATA_s *DAGOBAH_ADATA;
extern f32 MiscTime;
void TakeOverCode(GameObject_s *object, i32 tag_pressed);
extern "C" TERRAIN_SURFACE_s TerSurface[32];
void ApplyGravity(GameObject_s *object, f32 *gravity, f32 hover_height, f32 seek_rate, f32 *ground_height);
void InstantKillParts(GameObject_s *object, i32 mode, f32 delay);
i32 IntersectWater(GameObject_s *object);
extern "C" void NewTerrPlatformsOff();
i32 testshadowfix = 1;

namespace {
    struct TERRAIN_SCENE_OBJECT {
        u8 pad_0x00[0x44];
        u8 transform[4];
        void *object;
        u8 pad_0x4c[4];
    };
    DECOMP_ASSERT(sizeof(TERRAIN_SCENE_OBJECT) == 0x50, "TERRAIN_SCENE_OBJECT ABI");

    struct TERRAIN_DISPLAY_OBJECT {
        u8 pad_0x00[0x40];
        u8 embedded_object[0x78];
        u8 transform[8];
        i32 scene_object_index;
        void *object;
        u8 pad_0xc8[8];
    };
    DECOMP_ASSERT(sizeof(TERRAIN_DISPLAY_OBJECT) == 0xd0, "TERRAIN_DISPLAY_OBJECT ABI");
    DECOMP_ASSERT(offsetof(TERRAIN_DISPLAY_OBJECT, transform) == 0xb8, "TERRAIN_DISPLAY_OBJECT transform offset");
    DECOMP_ASSERT(offsetof(TERRAIN_DISPLAY_OBJECT, scene_object_index) == 0xc0,
                  "TERRAIN_DISPLAY_OBJECT scene_object_index offset");

    struct TERRAIN_DISPLAY_SCENE {
        u8 pad_0x00[0x6c];
        i32 object_count;
        TERRAIN_DISPLAY_OBJECT *objects;
    };
    DECOMP_ASSERT(offsetof(TERRAIN_DISPLAY_SCENE, object_count) == 0x6c, "TERRAIN_DISPLAY_SCENE object_count offset");

    struct TERRAIN_GSCENE {
        u8 pad_0x00[0x1c];
        i32 object_count;
        TERRAIN_SCENE_OBJECT *objects;
        u8 pad_0x24[0x34];
        void *object_map_present;
        u16 *object_map;
        u8 pad_0x60[0xb0];
        TERRAIN_DISPLAY_SCENE *display_scene;
    };
    DECOMP_ASSERT(offsetof(TERRAIN_GSCENE, object_count) == 0x1c, "TERRAIN_GSCENE object_count offset");
    DECOMP_ASSERT(offsetof(TERRAIN_GSCENE, object_map_present) == 0x58, "TERRAIN_GSCENE object_map_present offset");
    DECOMP_ASSERT(offsetof(TERRAIN_GSCENE, display_scene) == 0x110, "TERRAIN_GSCENE display_scene offset");

} // namespace

void *ScaleTerrainT1;
static void *ScaleTerrainT2;
TERRAIN_SHAPE *ScaleTerrain;
static void *TempScanStack;
static void *TempStackPtr;
static i32 TerrPlatDis = -1;
static TERRAIN_WALL_POINT *WallSplList;
static i32 WallSplCount;
i32 terraincnt;
i32 curSphereter;
i32 platinrange;
TERRAIN_SHAPE *ShadPoly;
extern "C" i32 ShadowIntensityInfo(void) {
    return ShadPoly ? (i32)ShadPoly->normal_flags - 8 : -1;
}
TERRAIN_SHAPE *TerrPoly;
extern "C" void NuRndrLine3dDbg(f32, f32, f32, f32, f32, f32, i32);

extern "C" void SphereDrawEx(NUVEC *centre, f32 radius, f32 vertical_scale, i32 colour) {
    f32 radius_squared = radius * radius;
    for (i32 band = 0; band < 8; ++band) {
        f32 lower_radius = radius * NU_SIN_LUT((band < 4 ? band : 8 - band) * 4096);
        f32 upper_radius = radius * NU_SIN_LUT((band < 3 ? band + 1 : 7 - band) * 4096);
        f32 lower_y = NuFsqrt(radius_squared - lower_radius * lower_radius) * vertical_scale;
        f32 upper_y = NuFsqrt(radius_squared - upper_radius * upper_radius) * vertical_scale;
        if (band > 4)
            lower_y = -lower_y;
        if (band > 3)
            upper_y = -upper_y;
        lower_y += radius * vertical_scale + centre->y;
        upper_y += radius * vertical_scale + centre->y;
        for (i32 angle = 0; angle < 65536; angle += 2048) {
            NuRndrLine3dDbg(centre->x + lower_radius * NU_COS_LUT(angle), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle), centre->x + upper_radius * NU_COS_LUT(angle),
                            upper_y, centre->z + upper_radius * NU_SIN_LUT(angle), colour);
            NuRndrLine3dDbg(centre->x + lower_radius * NU_COS_LUT(angle), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle),
                            centre->x + lower_radius * NU_COS_LUT(angle + 2048), lower_y,
                            centre->z + lower_radius * NU_SIN_LUT(angle + 2048), colour);
        }
    }
}

extern "C" void SphereDraw(NUVEC *centre, f32 radius, f32 vertical_scale) {
    SphereDrawEx(centre, radius, vertical_scale, 0x00400000);
}
u8 TerrWallInfo;
u8 TerrWallTab[4];
NUVEC TerrWallNorm;
i32 PlatImpactId;
static TERRAIN_SHAPE PlatImpactTer;
static NUVEC PlatImpactNorm;
i32 TerrPolyObj;
static TERRAIN_SHAPE TerrPolyInfo;

extern TERRSET *CurTerr;
extern TerrainQuery_s *TerI;
extern i32 WallSplinesOnly;
extern "C" i32 IgnoreWallSplines;
extern i32 curPickInst;
extern i16 castnum;
extern f32 wallover;
extern TERRAIN_TRACK_SLOT *CurTrackInfo;
extern void *TerImpactData;
extern i32 *TerImpactDataCount;
extern i32 TerImpactDataMax;
extern tertype **TerrOverRideScan;

void TerrainSkinAllocate(terrsitu_s *terrain_group);
void ScanWallSplineTerrain(i32 scan_type, i32 terrain_mask, i32 scan_flags);
void NewTerrStoreAnyInfo();
void TerrainMoveImpactData();
void RotateVec(NUVEC *source, NUVEC *destination);
TERRAIN_TRACK_SLOT *AllocTerrId();
void PlatformConnect(char *track_id, NUVEC *position_delta, NUVEC *movement_delta, i32 platform_index);
void FullDeflectSmallY(NUVEC *normal, NUVEC *movement, NUVEC *result);
extern "C" void FullDeflect(NUVEC *normal, NUVEC *movement, NUVEC *result);
void DerotateMovementVector();
i32 HitTerrain();
void TerrainImpactNorm();
void Tag_Check(GameObject_s *object);
void BigJumpCode(GameObject_s *object);
f32 GameShadow(GameObject_s *object, NUVEC *position, f32 probe_height, i32 terrain_mask);
void GetSurfaceInfo(GameObject_s *object, i32 update_surface, f32 shadow_height);
extern "C" i32 NewShadowOnPlatform();
extern NUVEC ShadNorm;

extern i32 terrhitflags;
extern i32 TERRAINMASK_NONWEAPON;
extern i32 TERRAINMASK_NONDROID;
extern i32 LEGO_AIPATHCNX_BLOCKAGE;
extern i32 LEGO_AIPATHCNX_FULLTERRAIN;
extern i32 LEGO_AIPATHCNX_WALLSHUFFLE;
NUVEC TerCrossProduct(NUVEC *a, NUVEC *b);

void StorePlatImpact() {
    TerrainQuery_s *query = TerI;
    const u8 hit_type = query->hit_type;
    if (hit_type == TERRAIN_HIT_TYPE_NONE || query->terrain_group_index == -1) {
        return;
    }

    TERRAIN_GROUP *group = &CurTerr->groups[query->terrain_group_index];
    if (group->chunk_type != TERRAIN_CHUNK_GROUP_SECONDARY) {
        return;
    }

    castnum = query->terrain_group_index;
    NUVEC impact_normal = query->movement_normal;

    const u8 maximum_supported_hit_type = TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_SPHERE;
    if (hit_type > maximum_supported_hit_type) {
        return;
    }

    const i32 hit_type_flag = 1 << hit_type;
    const i32 rotated_class_mask =
        (1 << TERRAIN_HIT_TYPE_CYLINDER) | (1 << TERRAIN_HIT_TYPE_VERTEX) | (1 << TERRAIN_HIT_TYPE_SPHERE);
    const i32 rotated_hit_mask = rotated_class_mask | (rotated_class_mask << TERRAIN_HIT_TYPE_SECOND_NORMAL);
    const i32 face_class_mask = 1 << TERRAIN_HIT_TYPE_FACE;
    const i32 direct_hit_mask = face_class_mask | (face_class_mask << TERRAIN_HIT_TYPE_SECOND_NORMAL);
    if ((hit_type_flag & rotated_hit_mask) != 0) {
        RotateVec(&impact_normal, &impact_normal);
        query = TerI;
        if ((query->hit_type & TERRAIN_HIT_TYPE_SECOND_NORMAL) == 0) {
            impact_normal.x *= query->inverse_collision_radius;
            impact_normal.y *= query->inverse_collision_radius;
            impact_normal.z *= query->inverse_collision_radius;
        }
    } else if ((hit_type_flag & direct_hit_mask) == 0) {
        return;
    }

    const f32 unit_scale = 1.0f;
    if (query->object_scale != unit_scale) {
        const f32 normal_length = NuFsqrt(impact_normal.x * impact_normal.x +
                                          impact_normal.y * impact_normal.y * query->inverse_object_scale_sq +
                                          impact_normal.z * impact_normal.z);
        f32 inverse_normal_length = 0.0f;
        if (normal_length != 0.0f) {
            inverse_normal_length = unit_scale / normal_length;
        }

        query = TerI;
        impact_normal.x *= inverse_normal_length;
        impact_normal.y *= query->inverse_object_scale * inverse_normal_length;
        impact_normal.z *= inverse_normal_length;
    }

    query = TerI;
    PlatImpactId = CurTerr->groups[query->terrain_group_index].scene_index;
    PlatImpactNorm = impact_normal;
    PlatImpactTer = *query->surface;
}

extern "C" void PlatImpactInfo(NUVEC *normal, i32 *material0, i32 *material1) {
    if (PlatImpactId != -1) {
        *normal = PlatImpactNorm;
        *material0 = PlatImpactTer.material[0];
        *material1 = PlatImpactTer.material[1];
    }
    PlatImpactId = -1;
}

namespace {

    struct TerrainScanBounds {
        f32 min_x;
        f32 min_y;
        f32 min_z;
        f32 max_x;
        f32 max_y;
        f32 max_z;
    };

    struct TerrainScanWriter {
        u8 *group_header;
        TERRAIN_SHAPE **cursor;
        u8 *limit;
        i32 group_shape_count;
        i32 scaled_shape_count;
    };

    static bool TerrainBoundsOverlap(const TerrainScanBounds &bounds, const NUVEC &minimum, const NUVEC &maximum) {
        return bounds.max_x >= minimum.x && bounds.max_y >= minimum.y && bounds.max_z >= minimum.z &&
               maximum.x >= bounds.min_x && maximum.y > bounds.min_y && maximum.z > bounds.min_z;
    }

    static bool TerrainShapeOverlaps(const TerrainScanBounds &bounds, const TERRAIN_SHAPE &shape) {
        return bounds.max_x >= shape.min_x && shape.max_x > bounds.min_x && bounds.max_y >= shape.min_y &&
               shape.max_y > bounds.min_y && bounds.max_z >= shape.min_z && shape.max_z > bounds.min_z;
    }

    static void TerrainFinishScanGroup(TerrainScanWriter *writer, i32 group_index) {
        if (writer->group_shape_count == 0) {
            return;
        }

        i16 *header = reinterpret_cast<i16 *>(writer->group_header);
        header[0] = static_cast<i16>(writer->group_shape_count);
        header[1] = static_cast<i16>(group_index);
        writer->group_header = reinterpret_cast<u8 *>(writer->cursor);
        // The target's pointers are four bytes, so its packed header occupies
        // one pointer slot.  Keeping that relationship on wider hosts lets the
        // scan consumers use the same `entries + 1` traversal without reading
        // the first pointer halfway through the header.
        writer->cursor = reinterpret_cast<TERRAIN_SHAPE **>(writer->group_header + sizeof(TERRAIN_SHAPE *));
        writer->group_shape_count = 0;
    }

    static TERRAIN_SHAPE *TerrainScaleShape(TERRAIN_SHAPE *candidate, const TERRAIN_GROUP &group,
                                            TerrainScanWriter *writer) {
        if (TerI->object_scale != 1.0f) {
            TERRAIN_SHAPE *source = candidate;
            candidate = &ScaleTerrain[writer->scaled_shape_count++];
            candidate->material[0] = source->material[0];
            candidate->material[1] = source->material[1];
            candidate->flags = source->flags;
            candidate->normal_flags = source->normal_flags;

            for (i32 vector_index = 0; vector_index < 3; ++vector_index) {
                candidate->vectors[vector_index].x = source->vectors[vector_index].x;
                candidate->vectors[vector_index].y =
                    (source->vectors[vector_index].y + group.origin.y) * TerI->inverse_object_scale - group.origin.y;
                candidate->vectors[vector_index].z = source->vectors[vector_index].z;
            }

            if (source->normals[1].y < 65535.0f) {
                candidate->vectors[3].x = source->vectors[3].x;
                candidate->vectors[3].y =
                    (source->vectors[3].y + group.origin.y) * TerI->inverse_object_scale - group.origin.y;
                candidate->vectors[3].z = source->vectors[3].z;

                const f32 length = NuFsqrt(source->normals[1].x * source->normals[1].x +
                                           source->normals[1].y * source->normals[1].y * TerI->object_scale_sq +
                                           source->normals[1].z * source->normals[1].z);
                const f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
                candidate->normals[1].x = source->normals[1].x * inverse_length;
                candidate->normals[1].y = source->normals[1].y * TerI->object_scale * inverse_length;
                candidate->normals[1].z = source->normals[1].z * inverse_length;
            } else {
                candidate->normals[1].y = 65536.0f;
            }

            const f32 length = NuFsqrt(source->normals[0].x * source->normals[0].x +
                                       source->normals[0].y * source->normals[0].y * TerI->object_scale_sq +
                                       source->normals[0].z * source->normals[0].z);
            const f32 inverse_length = length == 0.0f ? 0.0f : 1.0f / length;
            candidate->normals[0].x = source->normals[0].x * inverse_length;
            candidate->normals[0].y = source->normals[0].y * TerI->object_scale * inverse_length;
            candidate->normals[0].z = source->normals[0].z * inverse_length;
        }
        return candidate;
    }

    static void TerrainScanPlatformGroup(TerrainScanWriter *writer, const TerrainScanBounds &bounds, i32 group_index,
                                         i32 terrain_mask, i32 scan_flags, f32 movement_scale) {
        TERRAIN_GROUP &group = CurTerr->groups[group_index];
        TERRAIN_PLATFORM &platform = CurTerr->platforms[group.scene_index];
        if (group.chunk_type == -1)
            return;
        if (platform.scene_transform != NULL) {
            const u8 visible_mask = (platform.flags & TERRAIN_PLATFORM_FLAG_DISPLAY_LIST_BACKED) != 0 ? 2 : 1;
            if ((*static_cast<u8 *>(platform.scene_transform) & visible_mask) == 0)
                return;
        }
        TerrainScanBounds local = bounds;
        NUMTX *matrix = static_cast<NUMTX *>(platform.scene_object);
        if (matrix != NULL) {
            const f32 dx = (matrix->m30 - platform.previous_matrix.m30) * movement_scale;
            const f32 dy = (matrix->m31 - platform.previous_matrix.m31) * movement_scale;
            const f32 dz = (matrix->m32 - platform.previous_matrix.m32) * movement_scale;
            if (dx > 0.0f)
                local.max_x += dx;
            else
                local.min_x += dx;
            if (dy > 0.0f)
                local.max_y += dy;
            else
                local.min_y += dy;
            if (dz > 0.0f)
                local.max_z += dz;
            else
                local.min_z += dz;
        }
        local.min_x -= group.origin.x;
        local.max_x -= group.origin.x;
        local.min_y -= group.origin.y;
        local.max_y -= group.origin.y;
        local.min_z -= group.origin.z;
        local.max_z -= group.origin.z;
        const bool rotating = (platform.flags & TERRAIN_PLATFORM_FLAG_ROTATING) != 0;
        if (!rotating && !TerrainBoundsOverlap(local, group.bounds_min, group.bounds_max))
            return;
        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
        while (batch->marker >= 0) {
            TERRAIN_SHAPE *shapes = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
            if (rotating || (local.max_x >= batch->min_x && batch->max_x > local.min_x && local.max_z >= batch->min_z &&
                             batch->max_z > local.min_z)) {
                for (i32 i = 0; i < batch->shape_count; ++i) {
                    TERRAIN_SHAPE *shape = &shapes[i];
                    if (!rotating && !TerrainShapeOverlaps(local, *shape))
                        continue;
                    platinrange = 1;
                    if (reinterpret_cast<u8 *>(writer->cursor) >= writer->limit ||
                        (shape->material[1] != 0 && (shape->material[1] & terrain_mask) == 0) ||
                        (shape->flags & scan_flags) == 0x40)
                        continue;
                    if (!rotating) {
                        shape = TerrainScaleShape(shape, group, writer);
                    } else {
                        NUVEC4 vertices[4];
                        const bool quad = shape->normals[1].y < 65535.0f;
                        for (i32 v = 0; v < 3; ++v) {
                            vertices[v].x = shape->vectors[v].x;
                            vertices[v].y = shape->vectors[v].y;
                            vertices[v].z = shape->vectors[v].z;
                            vertices[v].w = 0.0f;
                        }
                        NuVec4MtxTransformVU0x3(vertices, vertices, matrix);
                        if (quad) {
                            vertices[3].x = shape->vectors[3].x;
                            vertices[3].y = shape->vectors[3].y;
                            vertices[3].z = shape->vectors[3].z;
                            vertices[3].w = 0.0f;
                            NuVec4MtxTransformVU0(&vertices[3], &vertices[3], matrix);
                        } else
                            vertices[3] = vertices[2];
                        bool above_min_x = false, below_max_x = false;
                        bool above_min_z = false, below_max_z = false;
                        for (i32 v = 0; v < 4; ++v) {
                            above_min_x |= vertices[v].x > local.min_x;
                            below_max_x |= vertices[v].x < local.max_x;
                            above_min_z |= vertices[v].z > local.min_z;
                            below_max_z |= vertices[v].z < local.max_z;
                        }
                        if (!above_min_x || !below_max_x || !above_min_z || !below_max_z)
                            continue;
                        TERRAIN_SHAPE *transformed = &ScaleTerrain[writer->scaled_shape_count++];
                        transformed->material[0] = shape->material[0];
                        transformed->material[1] = shape->material[1];
                        transformed->flags = shape->flags;
                        transformed->normal_flags = shape->normal_flags;
                        for (i32 v = 0; v < (quad ? 4 : 3); ++v) {
                            transformed->vectors[v].x = vertices[v].x;
                            transformed->vectors[v].y =
                                (vertices[v].y + group.origin.y) * TerI->inverse_object_scale - group.origin.y;
                            transformed->vectors[v].z = vertices[v].z;
                        }
                        if (!quad)
                            transformed->normals[1].y = 65536.0f;
                        for (i32 n = quad ? 1 : 0; n >= 0; --n) {
                            NUVEC a, b;
                            NUVEC *v = transformed->vectors;
                            NuVecSub(&a, &v[n ? 1 : 2], &v[n ? 3 : 0]);
                            NuVecSub(&b, &v[n ? 2 : 1], &v[n ? 3 : 0]);
                            transformed->normals[n] = TerCrossProduct(&a, &b);
                            NUVEC &normal = transformed->normals[n];
                            const f32 length = NuFsqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                            const f32 inverse = length == 0.0f ? 0.0f : 1.0f / length;
                            normal.x *= inverse;
                            normal.y *= inverse;
                            normal.z *= inverse;
                        }
                        shape = transformed;
                    }
                    *writer->cursor++ = shape;
                    ++writer->group_shape_count;
                }
            }
            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(shapes + batch->shape_count);
        }
        TerrainFinishScanGroup(writer, group_index);
    }

    static TerrainScanBounds TerrainGetScanBounds(const TerrainQuery_s &query) {
        const f32 padding = 0.05f;
        TerrainScanBounds bounds;

        if (query.scan_result != 1) {
            const f32 move_length = NuFsqrt(query.movement.x * query.movement.x + query.movement.y * query.movement.y +
                                            query.movement.z * query.movement.z);
            const f32 reach = move_length + query.collision_radius + 0.1f;
            const f32 vertical_reach = reach * query.object_scale;
            bounds.min_x = query.position.x - padding - reach;
            bounds.max_x = query.position.x + padding + reach;
            bounds.min_y = query.position.y - padding - vertical_reach;
            bounds.max_y = query.position.y + padding + vertical_reach;
            bounds.min_z = query.position.z - padding - reach;
            bounds.max_z = query.position.z + padding + reach;
        } else {
            const f32 reach = 0.02f + query.collision_radius;
            if (query.movement.x > 0.0f) {
                bounds.min_x = query.position.x - reach;
                bounds.max_x = query.position.x + query.movement.x + reach;
            } else {
                bounds.min_x = query.position.x + query.movement.x - reach;
                bounds.max_x = query.position.x + reach;
            }
            if (query.movement.y > 0.0f) {
                bounds.min_y = query.position.y - reach;
                bounds.max_y = query.position.y + query.movement.y + reach;
            } else {
                bounds.min_y = query.position.y + query.movement.y - reach;
                bounds.max_y = query.position.y + reach;
            }
            if (query.movement.z > 0.0f) {
                bounds.min_z = query.position.z - reach;
                bounds.max_z = query.position.z + query.movement.z + reach;
            } else {
                bounds.min_z = query.position.z + query.movement.z - reach;
                bounds.max_z = query.position.z + reach;
            }
        }
        return bounds;
    }

    static void TerrainCollectWallSplines(TerrainScanBounds bounds, i32 terrain_mask) {
        bounds.min_x -= 0.02f;
        bounds.min_z -= 0.02f;
        bounds.max_x += 0.02f;
        bounds.max_z += 0.02f;
        WallSplCount = 0;
        for (TERRAIN_SPATIAL_NODE *node = CurTerr->spatial_nodes; node != NULL;
             node = *reinterpret_cast<TERRAIN_SPATIAL_NODE **>(reinterpret_cast<u8 *>(node) - sizeof(void *))) {
            const u8 mask = node->field_0x02 >> 8;
            if (mask != 0 && (mask & terrain_mask) == 0)
                continue;
            for (i32 first = 0; first < node->point_count; first += 16) {
                NUVEC *points = node->points + first;
                i32 end = node->point_count;
                if (points[0].y != 2147483648.0f) {
                    if (points[1].y < bounds.min_x || points[0].y > bounds.max_x || points[3].y < bounds.min_z ||
                        points[2].y > bounds.max_z)
                        continue;
                    end = MIN(end, first + 16);
                }
                for (i32 i = first; i < end; ++i) {
                    NUVEC &a = node->points[i];
                    NUVEC &b = node->points[i + 1];
                    if ((a.x < bounds.min_x || b.x > bounds.max_x) && (b.x < bounds.min_x || a.x > bounds.max_x))
                        continue;
                    if ((a.z < bounds.min_z || b.z > bounds.max_z) && (b.z < bounds.min_z || a.z > bounds.max_z))
                        continue;
                    if (WallSplCount < 64) {
                        WallSplList[WallSplCount].position = a;
                        WallSplList[WallSplCount + 1].position = b;
                        for (i32 p = 0; p < 2; ++p) {
                            u8 *material = WallSplList[WallSplCount + p].material;
                            material[0] = static_cast<u8>(node->field_0x02);
                            material[1] = mask;
                            material[2] = 0;
                            material[3] = 0;
                        }
                        WallSplCount += 2;
                    }
                }
            }
        }
        TerrOverRideScan = NULL;
    }

} // namespace

i16 debug_index;

i32 ReadTerrain(unsigned char *base_path, i32 first_group, i16 **buffer, TERRSET *terrain);
void NuVecCheckForSNANs(NUVEC *vector);
void TerrFlush();

// Debris and terrain globals — accessed from DebrisSetThinningLevel etc.
f32 debris_thinning_level;
i32 forced_debris_thinning;
i32 debris_detail_level;
// char *debris_name[147] @0x61e860 (.data): the static debris effect names,
// seeded into every world's debris system by InitGameDebris.
char *debris_name[147] = {
    "BULLET_SPARK",   "SABER_RED",     "SABER_GREEN",   "SABER_BLUE",    "SABER_PURPLE",   "EXPLD_1A",
    "JS_THRUST",      "S1_THRUST",     "COIN_BLUE",     "COIN_GOLD",     "COIN_SILVER",    "R2D2THRUSTER",
    "R2Q5THRUSTER",   "SPLASH",        "RIPPLE",        "HEART",         "HEART_BIG",      "EXPLOSION",
    "MINI_01",        "MINI_02",       "MINI_03",       "TER_SPARK",     "TER_SPARK2",     "EXPOL_02",
    "SPEEDERDUST",    "POD_DUST",      "CAVE_DUST",     "JANGOTHRUSTER", "EXPLO_DROID",    "EXPLO_NAB",
    "NAB_SMOKE",      "NAB_BOOST",     "MIKESMOKE",     "EXPLO_05",      "EXPLO_06",       "EXPLO_07",
    "EXPLO_11",       "MIKESTALL",     "TRAINING_01",   "TAG_BLUE",      "TAG_GREEN",      "ZIP_TARGET",
    "DOOKU_BALL",     "WALKER_01",     "WALKER_02",     "ENG_BLOW",      "DogImpact",      "DogEngine",
    "DogSmoke",       "DogTrail",      "YODA_HOVER_01", "K_Bolt",        "DUK_P2",         "PodHaze",
    "PodDust10",      "rock_pop1",     "BlueTrail",     "PurpleTrail",   "LavaDie",        "ComboRed",
    "ComboGreen",     "ComboBlue",     "ComboPurple",   "REPAIR",        "SABER_RED1",     "SABER_GREEN1",
    "SABER_BLUE1",    "SABER_PURPLE1", "XWING_1",       "MOUSE_POP",     "PHOTON",         "PHOTON_EXPLO",
    "PHOTON_EXPLO_S", "EXPLO_ORAN_2A", "EXPLO_ORAN_2B", "EXPLO_ORAN_2C", "BUILD_IT1",      "BUILD_IT2",
    "BUILD_IT3",      "BUILD_IT4",     "BUILD_IT5",     "BUILD_IT6",     "DRAG_POP_1",     "DRAG_POP_2",
    "DRAG_POP_4",     "GenoGun",       "SNOW_SPEEDER1", "LEVER_SPARK",   "ZAPPER_1",       "V_BOLT_RED",
    "V_BOLT_GREEN",   "CHAR_SMOKE1",   "CHAR_SMOKE2",   "POWER_P_1",     "POWER_P_2",      "POWER_P_3",
    "TIE_HIT1",       "BRICK_01",      "BRICK_02",      "BRICK_03",      "BUMP_01",        "BUMP_02",
    "LAND_S_DUST1",   "LAND_S_DUST2",  "LAND_S_THRUST", "LAND_S_HAZE",   "EXPLO_06",       "EXPLO_06b",
    "BUMP_03",        "FORCE_BLUE",    "FORCE_RED",     "SCAN3",         "REPULSOR",       "WEE_POP",
    "EXIT_FIRE_1",    "EXIT_FIRE_2",   "EWOK_POP1",     "EXPLO_02",      "ATAT_POP_1",     "ATAT_POP_2",
    "ATAT_POP_3",     "FIRE_1",        "RAIN_1",        "CHAR_BUBBLE",   "CHAR_GHOST",     "DEATH_1",
    "DEATH_2",        "BOULDER_2",     "DISH_1",        "DISH_2",        "SNOW_POP_1",     "SNOW_POP_2",
    "TREE_POP_1",     "TREE_POP_4",    "SPEEDER_HIT",   "FALCONGLOW",    "FALCONTHRUSTER", "SPEED_SPARK",
    "PUNCH_1",        "PUNCH_2",       "GUNSHIP_01",    "GUNSHIP_02",    "SandBlast",      "BULLET1",
    "BULLET2",        "BULLET_HIT",    "POD_SPARK",
};
i32 Grass_Available = 1;
i32 PDEBCOUNT = 0;
void *PDebNameList = NULL;

extern "C" void DebrisSetThinningLevel(f32 level) {
    debris_thinning_level = level < 1.0f ? 1.0f : level;
}
extern "C" void DebrisSetForcedThinning(i32 forced) {
    forced_debris_thinning = forced;
}
extern "C" void DebrisSetDetailLevel(i32 level) {
    debris_detail_level = level;
}

// Constructs the fixed terrain header and its variable-sized group, index and
// platform arrays in the caller's arena. The four bounds arrays and the cell
// assignment array are temporary storage carved backwards from buf_end.
extern "C" void *TerrainInitEx(i32 level_num, void *buf, void *buf_end, i32 options, char *path, void *gscn,
                               i32 scene_id, u32 max_group_indices, u32 max_groups, u32 max_platforms) {
    (void)level_num;
    (void)options;
    (void)scene_id;

    VARIPTR *cursor = static_cast<VARIPTR *>(buf);
    const VARIPTR saved_cursor = *cursor;
    TERRAIN_GSCENE *scene = static_cast<TERRAIN_GSCENE *>(gscn);

    usize scratch = reinterpret_cast<usize>(buf_end) & ~static_cast<usize>(1);
    scratch -= max_groups * sizeof(i16);
    i16 *group_cells = reinterpret_cast<i16 *>(scratch);

    scratch &= ~static_cast<usize>(3);
    scratch -= max_groups * sizeof(f32);
    f32 *group_min_x = reinterpret_cast<f32 *>(scratch);
    scratch -= max_groups * sizeof(f32);
    f32 *group_max_x = reinterpret_cast<f32 *>(scratch);
    scratch -= max_groups * sizeof(f32);
    f32 *group_min_z = reinterpret_cast<f32 *>(scratch);
    scratch -= max_groups * sizeof(f32);
    f32 *group_max_z = reinterpret_cast<f32 *>(scratch);

    if (ScaleTerrainT1 == NULL) {
        ScaleTerrainT1 = NU_ALLOC(0xc800, 4, NuMemoryManager::MEM_ALLOC_SET_TO_ZERO, "", NUMEMORY_CATEGORY_NONE);
    }
    if (ScaleTerrainT2 == NULL) {
        ScaleTerrainT2 = NU_ALLOC(0xc800, 4, NuMemoryManager::MEM_ALLOC_SET_TO_ZERO, "", NUMEMORY_CATEGORY_NONE);
    }
    if (TempScanStack == NULL) {
        TempScanStack = NU_ALLOC(0x2000, 4, NuMemoryManager::MEM_ALLOC_SET_TO_ZERO, "", NUMEMORY_CATEGORY_NONE);
    }
    if (WallSplList == NULL) {
        WallSplList = static_cast<TERRAIN_WALL_POINT *>(
            NU_ALLOC(0x600, 4, NuMemoryManager::MEM_ALLOC_SET_TO_ZERO, "", NUMEMORY_CATEGORY_NONE));
    }

    TERRSET *terrain = reinterpret_cast<TERRSET *>(cursor->u8_ptr);
    cursor->u8_ptr += sizeof(TERRSET);

    terrain->groups = reinterpret_cast<TERRAIN_GROUP *>(cursor->u8_ptr);
    cursor->u8_ptr += (max_groups + 0x80) * sizeof(TERRAIN_GROUP);
    terrain->group_indices = reinterpret_cast<i16 *>(cursor->u8_ptr);
    cursor->u8_ptr += max_group_indices * sizeof(i16);
    terrain->platforms = reinterpret_cast<TERRAIN_PLATFORM *>(cursor->u8_ptr);
    cursor->u8_ptr += max_platforms * sizeof(TERRAIN_PLATFORM);

    terrain->max_group_indices = max_group_indices;
    terrain->max_groups = max_groups;
    terrain->max_platforms = max_platforms;

    for (i32 level = 0; level < TERRAIN_INDEX_LEVEL_COUNT; ++level) {
        terrain->index_levels[level].entry_count = 0;
        terrain->index_levels[level].entries = NULL;
    }

    memset(terrain->groups, 0, max_groups * sizeof(TERRAIN_GROUP));
    for (u32 group_index = 0; group_index < max_groups; ++group_index) {
        terrain->groups[group_index].chunk_type = -1;
    }

    for (i32 cell = 0; cell < TERRAIN_CELL_RECORD_COUNT; ++cell) {
        terrain->cells[cell].group_count = 0;
    }

    for (u32 platform_index = 0; platform_index < max_platforms; ++platform_index) {
        TERRAIN_PLATFORM &platform = terrain->platforms[platform_index];
        platform.flags &= ~TERRAIN_PLATFORM_FLAG_DISPLAY_LIST_BACKED;
        platform.scene_object = NULL;
        platform.field_0x44 = 0;
        platform.bounce_impulse = 0;
        platform.bounce_offset = 0;
        platform.bounce_velocity = 0;
        platform.bounce_spring = 0;
        platform.bounce_damping = 0;
    }

    for (i32 slot = 0; slot < TERRAIN_TRACK_SLOT_COUNT; ++slot) {
        terrain->track_slots[slot].id = NULL;
        terrain->track_slots[slot].wall_contact_state = TERRAIN_TRACK_CONTACT_NONE;
    }

    terrain->field_0x064 = NULL;
    terrain->active_platform_count = 0;
    terrain->removed_platform_count = 0;

    terraincnt = 0;
    curSphereter = 0;
    platinrange = 0;
    ShadPoly = 0;
    TerrPoly = 0;
    TerrWallInfo = 0;
    PlatImpactId = -1;

    const i32 group_count =
        ReadTerrain(reinterpret_cast<unsigned char *>(path), 0, reinterpret_cast<i16 **>(cursor), terrain);
    terrain->group_count = static_cast<i16>(group_count);
    if (group_count < 0) {
        *cursor = saved_cursor;
        return NULL;
    }

    static const f32 kBoundsMinimum = -200000000.0f;
    static const f32 kBoundsMaximum = 200000000.0f;
    static const f32 kSteepNormalY = 0.707f;

    terrain->minimum_height = INT_MAX;

    for (i32 group_index = 0; group_index < group_count; ++group_index) {
        TERRAIN_GROUP &group = terrain->groups[group_index];
        if (static_cast<u32>(group.chunk_type) > TERRAIN_CHUNK_GROUP_SECONDARY) {
            continue;
        }

        f32 min_x = kBoundsMaximum;
        f32 min_y = kBoundsMaximum;
        f32 min_z = kBoundsMaximum;
        f32 max_x = kBoundsMinimum;
        f32 max_y = kBoundsMinimum;
        f32 max_z = kBoundsMinimum;
        f32 radius_squared = 0.0f;

        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
        while (batch->marker >= 0) {
            TERRAIN_SHAPE *shape = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
            for (i32 shape_index = 0; shape_index < batch->shape_count; ++shape_index, ++shape) {
                NuVecCheckForSNANs(&shape->normals[0]);
                NuVecCheckForSNANs(&shape->normals[1]);

                if (shape->material[1] > 0xef) {
                    const u8 legacy_material = shape->material[1];
                    shape->material[1] = 0;
                    shape->normal_flags |= static_cast<u8>(legacy_material * 4 + 0x44);
                }

                if (shape->material[0] == 0xff) {
                    shape->flags |= TERRAIN_SHAPE_FLAG_MATERIAL_ALIAS;
                    shape->material[0] = shape->material[1];
                    shape->material[1] = 0;
                } else if (static_cast<i8>(shape->material[0]) < 0) {
                    shape->material[0] &= 0x7f;
                    shape->flags |= TERRAIN_SHAPE_FLAG_MATERIAL_HIGH_BIT;
                }

                if (terrain->file_version == 0) {
                    shape->normal_flags &= static_cast<u8>(
                        ~(TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP | TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP));
                    if (kSteepNormalY >= shape->normals[0].y) {
                        shape->normal_flags |= TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP;
                    }
                    if (kSteepNormalY >= shape->normals[1].y) {
                        shape->normal_flags |= TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP;
                    }
                }

                if (shape->min_x < min_x)
                    min_x = shape->min_x;
                if (shape->max_x > max_x)
                    max_x = shape->max_x;
                if (shape->min_y < min_y)
                    min_y = shape->min_y;
                if (shape->max_y > max_y)
                    max_y = shape->max_y;
                if (shape->min_z < min_z)
                    min_z = shape->min_z;
                if (shape->max_z > max_z)
                    max_z = shape->max_z;

                f32 current_minimum_height = static_cast<f32>(terrain->minimum_height);
                for (i32 vector_index = 0; vector_index < 4; ++vector_index) {
                    const NUVEC &vector = shape->vectors[vector_index];
                    const f32 world_y = group.origin.y + vector.y;
                    if (current_minimum_height > world_y) {
                        terrain->minimum_height = static_cast<i32>(world_y);
                        current_minimum_height = static_cast<f32>(terrain->minimum_height);
                    }

                    const f32 length_squared = vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
                    if (length_squared > radius_squared) {
                        radius_squared = length_squared;
                    }
                }
            }

            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(reinterpret_cast<TERRAIN_SHAPE *>(batch + 1) +
                                                            batch->shape_count);
        }

        group.radius = NuFsqrt(radius_squared);
        if (group.chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY) {
            group.bounds_min.x = min_x;
            group.bounds_min.y = min_y;
            group.bounds_min.z = min_z;
            group.bounds_max.x = max_x;
            group.bounds_max.y = max_y;
            group.bounds_max.z = max_z;
        } else {
            group.bounds_min.x = group.origin.x + min_x;
            group.bounds_min.y = group.origin.y + min_y;
            group.bounds_min.z = group.origin.z + min_z;
            group.bounds_max.x = group.origin.x + max_x;
            group.bounds_max.y = group.origin.y + max_y;
            group.bounds_max.z = group.origin.z + max_z;
        }
        ++terraincnt;
    }

    f32 terrain_min_x = kBoundsMaximum;
    f32 terrain_min_z = kBoundsMaximum;
    f32 terrain_max_x = kBoundsMinimum;
    f32 terrain_max_z = kBoundsMinimum;

    for (i32 group_index = 0; group_index < group_count; ++group_index) {
        TERRAIN_GROUP &group = terrain->groups[group_index];
        if (group.chunk_type != TERRAIN_CHUNK_GROUP_PRIMARY) {
            continue;
        }

        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
        while (batch->marker >= 0) {
            const f32 min_x = group.origin.x + batch->min_x;
            const f32 max_x = group.origin.x + batch->max_x;
            const f32 min_z = group.origin.z + batch->min_z;
            const f32 max_z = group.origin.z + batch->max_z;
            if (min_x < terrain_min_x)
                terrain_min_x = min_x;
            if (max_x > terrain_max_x)
                terrain_max_x = max_x;
            if (min_z < terrain_min_z)
                terrain_min_z = min_z;
            if (max_z > terrain_max_z)
                terrain_max_z = max_z;

            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(reinterpret_cast<TERRAIN_SHAPE *>(batch + 1) +
                                                            batch->shape_count);
        }
    }

    terrain->group_index_count = 0;
    for (i32 group_index = 0; group_index < group_count; ++group_index) {
        TERRAIN_GROUP &group = terrain->groups[group_index];
        if (group.chunk_type != TERRAIN_CHUNK_GROUP_PRIMARY) {
            continue;
        }

        f32 min_x = kBoundsMaximum;
        f32 min_z = kBoundsMaximum;
        f32 max_x = kBoundsMinimum;
        f32 max_z = kBoundsMinimum;
        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
        while (batch->marker >= 0) {
            const f32 batch_min_x = group.origin.x + batch->min_x;
            const f32 batch_max_x = group.origin.x + batch->max_x;
            const f32 batch_min_z = group.origin.z + batch->min_z;
            const f32 batch_max_z = group.origin.z + batch->max_z;
            if (batch_min_x < min_x)
                min_x = batch_min_x;
            if (batch_max_x > max_x)
                max_x = batch_max_x;
            if (batch_min_z < min_z)
                min_z = batch_min_z;
            if (batch_max_z > max_z)
                max_z = batch_max_z;

            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(reinterpret_cast<TERRAIN_SHAPE *>(batch + 1) +
                                                            batch->shape_count);
        }

        group_min_x[group_index] = min_x;
        group_min_z[group_index] = min_z;
        group_max_x[group_index] = max_x;
        group_max_z[group_index] = max_z;

        const i32 x_distance = static_cast<i32>((min_x + max_x) * 0.5f - terrain_min_x);
        i32 x_cell =
            static_cast<i32>(static_cast<f32>(x_distance * TERRAIN_GRID_WIDTH) / (terrain_max_x - terrain_min_x));
        if (x_cell < 0)
            x_cell = 0;
        else if (x_cell >= TERRAIN_GRID_WIDTH)
            x_cell = TERRAIN_GRID_WIDTH - 1;

        const i32 z_distance = static_cast<i32>((min_z + max_z) * 0.5f - terrain_min_z);
        i32 z_cell =
            static_cast<i32>(static_cast<f32>(z_distance * TERRAIN_GRID_WIDTH) / (terrain_max_z - terrain_min_z));
        if (z_cell < 0)
            z_cell = 0;
        else if (z_cell >= TERRAIN_GRID_WIDTH)
            z_cell = TERRAIN_GRID_WIDTH - 1;

        group_cells[group_index] = static_cast<i16>(z_cell * TERRAIN_GRID_WIDTH + x_cell);
    }

    terrain->used_cell_count = 0;
    terrain->group_index_count = 0;
    for (i32 cell_index = 0; cell_index < TERRAIN_GRID_CELL_COUNT; ++cell_index) {
        TERRAIN_CELL &cell = terrain->cells[terrain->used_cell_count];
        cell.first_group = static_cast<u16>(terrain->group_index_count);

        f32 min_x = kBoundsMaximum;
        f32 min_z = kBoundsMaximum;
        f32 max_x = kBoundsMinimum;
        f32 max_z = kBoundsMinimum;
        for (i32 group_index = 0; group_index < group_count; ++group_index) {
            TERRAIN_GROUP &group = terrain->groups[group_index];
            if (group_cells[group_index] != cell_index || group.chunk_type != TERRAIN_CHUNK_GROUP_PRIMARY) {
                continue;
            }

            if (group_min_x[group_index] < min_x)
                min_x = group_min_x[group_index];
            if (group_min_z[group_index] < min_z)
                min_z = group_min_z[group_index];
            if (group_max_x[group_index] > max_x)
                max_x = group_max_x[group_index];
            if (group_max_z[group_index] > max_z)
                max_z = group_max_z[group_index];

            ++cell.group_count;
            terrain->group_indices[terrain->group_index_count] = static_cast<i16>(group_index);
            ++terrain->group_index_count;
        }

        if (cell.group_count != 0) {
            cell.min_x = min_x;
            cell.min_z = min_z;
            cell.max_x = max_x;
            cell.max_z = max_z;
            ++terrain->used_cell_count;
        }
    }

    TERRAIN_CELL &platform_cell = terrain->cells[TERRAIN_PLATFORM_CELL];
    platform_cell.first_group = static_cast<u16>(terrain->group_index_count);

    i32 platform_count = 0;
    for (i32 group_index = 0; group_index < group_count; ++group_index) {
        TERRAIN_GROUP &group = terrain->groups[group_index];
        if (group.chunk_type != TERRAIN_CHUNK_GROUP_SECONDARY) {
            continue;
        }
        if (platform_count >= terrain->max_platforms) {
            group.chunk_type = -1;
            continue;
        }

        u16 scene_object_index = group.scene_index;
        debug_index = static_cast<i16>(scene_object_index);
        if (scene == NULL) {
            group.chunk_type = -1;
            group.scene_index = static_cast<u16>(platform_count);
            continue;
        }

        if (scene->object_map_present != NULL) {
            scene_object_index = scene->object_map[scene_object_index];
            debug_index = static_cast<i16>(scene_object_index);
        }

        group.scene_index = static_cast<u16>(platform_count);
        TERRAIN_DISPLAY_SCENE *display_scene = scene->display_scene;
        if (display_scene == NULL && scene_object_index >= scene->object_count) {
            group.chunk_type = -1;
            continue;
        }

        terrain->group_indices[terrain->group_index_count] = static_cast<i16>(group_index);
        ++terrain->group_index_count;
        ++platform_cell.group_count;

        TERRAIN_PLATFORM &platform = terrain->platforms[static_cast<i16>(group.scene_index)];
        platform.flags = static_cast<u8>((platform.flags & 0xfa) | (group.platform_flags & 1));
        platform.scene_object_index = scene_object_index;
        platform.terrain_group_index = static_cast<i16>(group_index);

        if (display_scene != NULL) {
            TERRAIN_DISPLAY_OBJECT *display_object = NULL;
            for (i32 object_index = 0; object_index < display_scene->object_count; ++object_index) {
                TERRAIN_DISPLAY_OBJECT &candidate = display_scene->objects[object_index];
                if (candidate.scene_object_index == scene_object_index) {
                    display_object = &candidate;
                    break;
                }
            }

            if (display_object == NULL) {
                group.chunk_type = -1;
                continue;
            }

            if (display_object->object == NULL || display_object->object == reinterpret_cast<void *>(-1)) {
                platform.scene_object = display_object->embedded_object;
            } else {
                platform.scene_object = display_object->object;
            }
            platform.scene_transform = display_object->transform;
            platform.flags |= TERRAIN_PLATFORM_FLAG_DISPLAY_LIST_BACKED;
        } else {
            TERRAIN_SCENE_OBJECT &scene_object = scene->objects[scene_object_index];
            platform.scene_object = scene_object.object != NULL ? scene_object.object : &scene_object;
            platform.scene_transform = scene_object.transform;
        }

        ++platform_count;
    }

    TerrFlush();
    return terrain;
}
void LoadTerrainFile(WORLDINFO *world) {
    char path[256];
    world->terrain = NULL;
    if ((world->current_level->flags & LEVEL_TERRAIN) != 0) {
        NuStrCpy(path, world->config_file);
        LEVELDATA *level = world->current_level;
        if (level == (LEVELDATA *)PLATFORM_LDATA) {
            NuStrCpy(path, "levels\\episode_v\\cloudcityescape\\cloudcityescape_b\\cloudcityescape_b");
            level = world->current_level;
        }
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        void *terrain = TerrainInitEx(world->level_idx, &world->giz_buffer, world->unknown_0108.void_ptr, 0, path,
                                      world->current_gscn, 0, (u32)(u16)level->max_ter_groups,
                                      (u32)(u16)level->max_ter_groups, (u32)(u16)level->max_ter_platforms);
        world->terrain = terrain;
    }
}
void LoadGrassFile(WORLDINFO *world) {
    char path[268];
    world->page_grass = -1;
    Grass_Available = 1;
    char *config = world->config_file;

    if (g_isLowEndDevice != 0) {
        char *found = NuStrIStr(config, "Gungan");
        if (found == NULL)
            found = NuStrIStr(config, "SpeederChase");
        if (found == NULL)
            found = NuStrIStr(config, "EndorBattle");
        if (found == NULL)
            found = NuStrIStr(config, "Retake");

        if (found != NULL) {
            Grass_Available = 0;
        } else {
            Grass_Available = 1;
        }
    }

    if (g_isMidRangeDevice != 0 && NuStrIStr(config, "GunGan_A") != NULL) {
        Grass_Available = 0;
        return;
    }

    char *found = NuStrIStr(config, "SpeederChase");
    if (found != NULL) {
        Grass_Available = 0;
    }

    if (Grass_Available != 0) {
        sprintf(path, "%s.gra", config);
        if (NuFileExists(path)) {
            i32 page = edgraLoadPage(path, world->current_gscn, *(i32 *)&world->terrain, &world->giz_buffer,
                                     &world->unknown_0108);
            world->page_grass = page;
        }
    }
}
void LoadBridgeFile(WORLDINFO *world) {
    char path[256];
    world->page_bridge = -1;
    sprintf(path, "%s.bri", world->config_file);
    if (NuFileExists(path)) {
        i32 page = edbriLoadPage(path, world->current_gscn);
        world->page_bridge = page;
    }
}
void LoadPartFile(WORLDINFO *world) {
    char path[256];
    world->page_part = -1;
    edpartSetParticlePage(world->page_pp);

    if ((world->current_level->flags & (LEVEL_OUTRO | LEVEL_MIDTRO | LEVEL_INTRO)) == 0) {
        sprintf(path, "%s.par", world->config_file);
        i32 page = -1;
        if (NuFileExists(path)) {
            page = edpartLoadPage(path, 1, world->current_gscn);
            world->page_part = page;
        }
        world->part_debris_sys = static_cast<PARTDEBSYS_s *>(
            InitPartDebris(&world->giz_buffer, &world->unknown_0108, 0x40, PDEBCOUNT, (char **)PDebNameList, page));
    }
}
TERRAIN_TRACK_SLOT *ScanTerrId(void *id) {
    TERRSET *terrain = CurTerr;
    if (terrain == NULL) {
        return NULL;
    }

    for (i32 slot_index = 0; slot_index < TERRAIN_TRACK_SLOT_COUNT; ++slot_index) {
        if (terrain->track_slots[slot_index].id == id) {
            return &terrain->track_slots[slot_index];
        }
    }

    return NULL;
}
i32 UnderWater(GameObject_s *object) {
    if ((object->apiobj.field_0x27f & static_cast<u8>(~8u)) == 1) {
        return object->apiobj.water_height >= object->apiobj.pos_y;
    }
    return 0;
}
void ScanTerrain(i32 scan_type, i32 terrain_mask, i32 scan_flags) {
    if (WallSplinesOnly != 0) {
        ScanWallSplineTerrain(scan_type, terrain_mask, scan_flags);
        return;
    }

    ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT1);
    platinrange = 0;
    TerI->scan_group_index = -1;

    TerrainScanWriter writer;
    writer.group_header = TerI->scan_list_storage;
    writer.cursor = reinterpret_cast<TERRAIN_SHAPE **>(writer.group_header + sizeof(TERRAIN_SHAPE *));
    writer.limit = TerI->scan_list_storage + sizeof(TerI->scan_list_storage) - 12;
    writer.group_shape_count = 0;
    writer.scaled_shape_count = 0;

    const TerrainScanBounds bounds = TerrainGetScanBounds(*TerI);

    for (i32 cell_index = 0; cell_index < CurTerr->used_cell_count; ++cell_index) {
        const TERRAIN_CELL &cell = CurTerr->cells[cell_index];
        if (bounds.max_x < cell.min_x || bounds.max_z < cell.min_z || cell.max_x < bounds.min_x ||
            cell.max_z < bounds.min_z) {
            continue;
        }

        const i16 *group_indices = CurTerr->group_indices + cell.first_group;
        for (i32 cell_group = 0; cell_group < cell.group_count; ++cell_group) {
            const i32 group_index = group_indices[cell_group];
            TERRAIN_GROUP &group = CurTerr->groups[group_index];
            if (!TerrainBoundsOverlap(bounds, group.bounds_min, group.bounds_max) || group.chunk_type == -1) {
                continue;
            }

            if (static_cast<i16>(group.scene_index) < 0) {
                TerrainSkinAllocate(reinterpret_cast<terrsitu_s *>(&group));
            }

            TerrainScanBounds group_bounds = bounds;
            group_bounds.min_x -= group.origin.x;
            group_bounds.max_x -= group.origin.x;
            group_bounds.min_y -= group.origin.y;
            group_bounds.max_y -= group.origin.y;
            group_bounds.min_z -= group.origin.z;
            group_bounds.max_z -= group.origin.z;

            TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
            while (batch->marker >= 0) {
                TERRAIN_SHAPE *shapes = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
                if (group_bounds.max_x >= batch->min_x && batch->max_x > group_bounds.min_x &&
                    group_bounds.max_z >= batch->min_z && batch->max_z > group_bounds.min_z) {
                    for (i32 shape_index = 0; shape_index < batch->shape_count; ++shape_index) {
                        TERRAIN_SHAPE *candidate = &shapes[shape_index];
                        if (!TerrainShapeOverlaps(group_bounds, *candidate) ||
                            reinterpret_cast<u8 *>(writer.cursor) >= writer.limit) {
                            continue;
                        }
                        if (candidate->material[1] != 0 && (candidate->material[1] & terrain_mask) == 0) {
                            continue;
                        }
                        if ((candidate->flags & scan_flags) == 0x40) {
                            continue;
                        }

                        candidate = TerrainScaleShape(candidate, group, &writer);
                        *writer.cursor++ = candidate;
                        ++writer.group_shape_count;
                    }
                }
                batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(shapes + batch->shape_count);
            }
            TerrainFinishScanGroup(&writer, group_index);
        }
    }

    if (scan_type != 0) {
        TerrainScanBounds platform_bounds = bounds;
        platform_bounds.min_x -= 0.05f;
        platform_bounds.min_y -= 0.05f;
        platform_bounds.min_z -= 0.05f;
        platform_bounds.max_x += 0.05f;
        platform_bounds.max_y += 0.05f;
        platform_bounds.max_z += 0.05f;
        const i16 *groups = CurTerr->active_platform_groups;
        i32 count = CurTerr->active_platform_count;
        if (platform_bounds.min_x <= CurTerr->platform_scan_min.x ||
            platform_bounds.max_x >= CurTerr->platform_scan_max.x ||
            platform_bounds.min_z <= CurTerr->platform_scan_min.z ||
            platform_bounds.max_z >= CurTerr->platform_scan_max.z) {
            const TERRAIN_CELL &cell = CurTerr->cells[TERRAIN_PLATFORM_CELL];
            groups = CurTerr->group_indices + cell.first_group;
            count = cell.group_count;
        }
        for (i32 i = 0; i < count; ++i) {
            const TERRAIN_GROUP &group = CurTerr->groups[groups[i]];
            if (group.origin.x - group.radius > platform_bounds.max_x ||
                group.origin.x + group.radius < platform_bounds.min_x ||
                group.origin.z - group.radius > platform_bounds.max_z ||
                group.origin.z + group.radius < platform_bounds.min_z)
                continue;
            TerrainScanPlatformGroup(&writer, platform_bounds, groups[i], terrain_mask, scan_flags, 1.0f);
        }
    }

    for (i32 pick_index = 0; pick_index < curPickInst; ++pick_index) {
        const i32 group_index = CurTerr->max_groups + pick_index;
        TERRAIN_GROUP &group = CurTerr->groups[group_index];
        TerrainScanBounds local_bounds = bounds;
        local_bounds.min_x -= group.origin.x;
        local_bounds.max_x -= group.origin.x;
        local_bounds.min_y -= group.origin.y;
        local_bounds.max_y -= group.origin.y;
        local_bounds.min_z -= group.origin.z;
        local_bounds.max_z -= group.origin.z;
        if (!TerrainBoundsOverlap(local_bounds, group.bounds_min, group.bounds_max) || group.chunk_type == -1) {
            continue;
        }

        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
        while (batch->marker >= 0) {
            TERRAIN_SHAPE *shapes = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
            if (local_bounds.max_x >= batch->min_x && batch->max_x > local_bounds.min_x &&
                local_bounds.max_z >= batch->min_z && batch->max_z > local_bounds.min_z) {
                for (i32 shape_index = 0; shape_index < batch->shape_count; ++shape_index) {
                    TERRAIN_SHAPE *candidate = &shapes[shape_index];
                    if (!TerrainShapeOverlaps(local_bounds, *candidate) ||
                        reinterpret_cast<u8 *>(writer.cursor) >= writer.limit) {
                        continue;
                    }
                    if (candidate->material[1] != 0 && (candidate->material[1] & terrain_mask) == 0) {
                        continue;
                    }
                    if ((candidate->flags & scan_flags) == 0x40) {
                        continue;
                    }

                    candidate = TerrainScaleShape(candidate, group, &writer);
                    *writer.cursor++ = candidate;
                    ++writer.group_shape_count;
                }
            }
            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(shapes + batch->shape_count);
        }
        TerrainFinishScanGroup(&writer, group_index);
    }

    TerI->scan_list = TerI->scan_list_storage;
    i16 *terminator = reinterpret_cast<i16 *>(writer.group_header);
    terminator[0] = 0;
    terminator[1] = 0;
    // Original 0x37ff7b consumes this one-scan suppression flag.
    WallSplCount = 0;
    if (IgnoreWallSplines != 0) {
        IgnoreWallSplines = 0;
    } else {
        TerrainCollectWallSplines(bounds, terrain_mask);
    }
}
NUVEC TerrainSkin(PLATSKININFO *info, nuvec_s *position, float weight, i32 mode) {
    NUVEC4 point __attribute__((aligned(16)));
    NUMTX matrix __attribute__((aligned(16)));
    if (mode == 1) {
        point.x = position->x;
        point.y = position->y;
        point.z = position->z;
        point.w = 1.0f;
        NuVec4MtxTransformVU0(&point, &point, info->matrix);
    } else {
        if (weight < 0.0f)
            weight = position->z / info->scale;
        if (weight < 0.0f)
            weight = 0.0f;
        else
            weight = MIN(1.0f, weight);
        point.x = position->x;
        point.y = position->y;
        point.w = 1.0f;
        if (info->matrix_data != NULL) {
            weight = (f32)(i32)(weight * 8.0f + 0.5f) * 0.125f;
            point.z = position->z - info->scale * weight;
            i32 index = (i32)(weight * 8.0f);
            NUVEC4 *rows = static_cast<NUVEC4 *>(info->matrix_data);
            NUVEC4 *out = reinterpret_cast<NUVEC4 *>(&matrix);
            out[0] = rows[index];
            out[1] = rows[index + 11];
            out[2] = rows[index + 22];
            out[3] = rows[index + 33];
        } else {
            struct SKIN_BLEND_DATA {
                NUMTX end_matrix;
                NUVEC scale_amplitude;
                f32 reserved_4c;
                NUVEC translation_amplitude;
            };
            SKIN_BLEND_DATA *blend = static_cast<SKIN_BLEND_DATA *>(info->skin_data);
            weight = (f32)(i32)(weight * 10.0f + 0.5f) / 10.0f;
            point.z = position->z - info->scale * weight;
            f32 inverse_weight = 1.0f - weight;
            f32 wave = NU_SIN_LUT((i32)(weight * 32768.0f));
            f32 *out = reinterpret_cast<f32 *>(&matrix);
            f32 *start = reinterpret_cast<f32 *>(info->matrix);
            f32 *end = reinterpret_cast<f32 *>(&blend->end_matrix);
            f32 *amplitude = reinterpret_cast<f32 *>(&blend->scale_amplitude);
            for (i32 row = 0; row < 3; ++row) {
                f32 scale = amplitude[row] * wave + 1.0f;
                for (i32 column = 0; column < 4; ++column) {
                    i32 index = row * 4 + column;
                    out[index] = (start[index] * inverse_weight + end[index] * weight) * scale;
                }
            }
            matrix.m30 = start[12] * inverse_weight + end[12] * weight + blend->translation_amplitude.x * wave;
            matrix.m31 = start[13] * inverse_weight + end[13] * weight + blend->translation_amplitude.y * wave;
            matrix.m32 = start[14] * inverse_weight + end[14] * weight + blend->translation_amplitude.z * wave;
            matrix.m33 = start[15] * inverse_weight + end[15] * weight;
        }
        NuVec4MtxTransformVU0(&point, &point, &matrix);
    }
    NUVEC result;
    result.x = point.x;
    result.y = point.y;
    result.z = point.z;
    return result;
}
void RotateTerrain(tertype *) {
}

namespace {

    struct TERRAIN_IMPACT_RECORD {
        NUVEC position;
        NUVEC normal;
        u8 material[2];
        u8 flags;
        u8 normal_flags;
    };
    DECOMP_ASSERT(sizeof(TERRAIN_IMPACT_RECORD) == 0x1c, "TERRAIN_IMPACT_RECORD ABI");

    enum TERRAIN_IMPACT_RESULT_FLAGS {
        TERRAIN_IMPACT_RESULT_WALL = 0x01,
        TERRAIN_IMPACT_RESULT_GROUND = 0x02,
        TERRAIN_IMPACT_RESULT_STATIC_WALL = 0x10,
    };

} // namespace

void TerrainImpact(NUVEC *position, NUVEC *movement, u8 *hit_flags) {
    TerrainMoveImpactData();

    TerrainQuery_s *query = TerI;
    u8 hit_type = query->hit_type;
    f32 walkable_normal_y = 0.0f;

    if (hit_type == TERRAIN_HIT_TYPE_NONE) {
        hit_flags[0] = 0;
        position->x = query->position.x + query->movement.x;
        position->y = (query->position.y + query->movement.y) * query->object_scale -
                      query->object_scale * query->collision_radius;
        position->z = query->position.z + query->movement.z;
        query->flags &= static_cast<u8>(~TERRAIN_QUERY_FLAG_PREVIOUS_NORMAL);
    } else {
        TERRAIN_SHAPE *surface = query->surface;
        f32 surface_alignment = 0.707f;
        if (surface != NULL) {
            surface_alignment = query->movement_normal.x * surface->normals[0].x +
                                query->movement_normal.y * surface->normals[0].y +
                                query->movement_normal.z * surface->normals[0].z;
        }

        TERRAIN_GROUP *group = NULL;
        if (query->terrain_group_index != -1) {
            group = &CurTerr->groups[query->terrain_group_index];
        }
        const bool platform_group = group != NULL && group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY;

        if (platform_group) {
            const u8 base_hit_type = hit_type & TERRAIN_HIT_TYPE_CLASS_MASK;
            if (base_hit_type > TERRAIN_HIT_TYPE_FACE && surface_alignment < 0.95f) {
                walkable_normal_y = 0.98f;
            } else {
                walkable_normal_y = 0.707f;
            }
        } else {
            walkable_normal_y = query->shape_adjusted != 0 ? 1.1f : -1.1f;
        }

        // A non-negative group index on a hit always comes with a surface.
        const bool wall_override_surface =
            query->terrain_group_index >= 0 &&
            (surface->normal_flags & TERRAIN_SURFACE_CLASS_MASK) == TERRAIN_SURFACE_CLASS_WALL_OVERRIDE;
        if (wall_override_surface) {
            // Original 0x382168 uses the scaled movement normal (+0xa8).
            query->position.x += query->movement_normal.x * 0.0015f;
            query->position.z += query->movement_normal.z * 0.0015f;
            walkable_normal_y = 1.1f;
        }
        if (wallover != 0.0f) {
            walkable_normal_y = wallover;
        }

        switch (hit_type) {
            case TERRAIN_HIT_TYPE_CYLINDER:
            case TERRAIN_HIT_TYPE_VERTEX:
            case TERRAIN_HIT_TYPE_SPHERE:
                if (!wall_override_surface)
                    walkable_normal_y = 0.707f;
                // Fall through to the shared first-normal response.
            case TERRAIN_HIT_TYPE_FACE: {

                query->hit_time -= query->separation_epsilon;
                if (query->hit_time < 0.0f) {
                    query->hit_time = 0.0f;
                }

                const f32 advance = query->hit_time;
                const NUVEC travelled = {
                    query->movement.x * advance,
                    query->movement.y * advance,
                    query->movement.z * advance,
                };
                query->position.x += travelled.x;
                query->position.y += travelled.y;
                query->position.z += travelled.z;
                query->movement.x -= travelled.x;
                query->movement.y -= travelled.y;
                query->movement.z -= travelled.z;

                if (walkable_normal_y > query->impact_normal.y) {
                    hit_flags[0] = 0;
                    terrhitflags |= TERRAIN_IMPACT_RESULT_WALL;
                    if (!platform_group) {
                        terrhitflags |= TERRAIN_IMPACT_RESULT_STATIC_WALL;
                    }

                    query->position.x += query->movement_normal.x * 0.0006f;
                    query->position.z += query->movement_normal.z * 0.0006f;
                    query->movement.x += query->movement_normal.x * 0.0006f;
                    query->movement.z += query->movement_normal.z * 0.0006f;
                    movement->x += query->movement_normal.x * 0.0009f;
                    movement->z += query->movement_normal.z * 0.0009f;

                    FullDeflect(&query->movement_normal, &query->movement, &query->movement);
                    FullDeflectSmallY(&query->impact_normal, movement, movement);

                    query = TerI;
                    if (CurTrackInfo == NULL) {
                        CurTrackInfo = AllocTerrId();
                        CurTrackInfo->flags = TERRAIN_TRACK_FLAG_NONE;
                        CurTrackInfo->id = query->hit_flags;
                        CurTrackInfo->platform_index = 0;
                        CurTrackInfo->platform_contact_state = TERRAIN_TRACK_CONTACT_NONE;
                    }
                    CurTrackInfo->wall_contact_state = TERRAIN_TRACK_CONTACT_ACTIVE;
                    CurTrackInfo->impact_normal = query->impact_normal;
                    TerrWallNorm = query->impact_normal;
                } else {
                    FullDeflect(&query->movement_normal, &query->movement, &query->movement);
                    FullDeflect(&query->impact_normal, movement, movement);

                    terrhitflags |= TERRAIN_IMPACT_RESULT_GROUND;
                    hit_flags[0] = 1;
                    hit_flags[1] = 1;
                    query->position.y += query->movement_normal.y * 0.0003f;

                    query = TerI;
                    if (query->terrain_group_index != -1) {
                        group = &CurTerr->groups[query->terrain_group_index];
                        if (group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY) {
                            CurTerr->platforms[group->scene_index].flags |= TERRAIN_PLATFORM_FLAG_COLLIDED;
                        }
                    }
                }

                query = TerI;
                query->flags &= static_cast<u8>(~TERRAIN_QUERY_FLAG_PREVIOUS_NORMAL);

                break;
            }
            case TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_CYLINDER:
            case TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_VERTEX:
            case TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_SPHERE:
                walkable_normal_y = 0.707f;
                // Fall through to the shared second-normal response.
            case TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_FACE: {

                NUVEC response_normal = query->movement_normal;
                if ((query->flags & TERRAIN_QUERY_FLAG_PREVIOUS_NORMAL) != 0) {
                    const f32 previous_alignment = response_normal.x * query->previous_movement_normal.x +
                                                   response_normal.y * query->previous_movement_normal.y +
                                                   response_normal.z * query->previous_movement_normal.z;
                    if (previous_alignment < 0.0f) {
                        NUVEC corrected_normal;
                        FullDeflect(&response_normal, &query->previous_movement_normal, &corrected_normal);
                        const f32 correction_alignment = corrected_normal.x * response_normal.x +
                                                         corrected_normal.y * response_normal.y +
                                                         corrected_normal.z * response_normal.z;
                        if (correction_alignment > 0.25f) {
                            response_normal.x = corrected_normal.x / correction_alignment;
                            response_normal.y = corrected_normal.y / correction_alignment;
                            response_normal.z = corrected_normal.z / correction_alignment;
                        }
                    }
                }

                query = TerI;
                const f32 contact_push = 0.0035f - query->unclamped_hit_time * 1.05f;
                if (walkable_normal_y > query->impact_normal.y) {
                    const f32 movement_push = 0.0035f - query->unclamped_hit_time * 0.35f;
                    query->position.x += response_normal.x * contact_push;
                    query->position.y += response_normal.y * contact_push;
                    query->position.z += response_normal.z * contact_push;
                    query->movement.x += response_normal.x * movement_push;
                    query->movement.y += response_normal.y * movement_push;
                    query->movement.z += response_normal.z * movement_push;
                    hit_flags[0] = 1;

                    FullDeflect(&response_normal, &query->movement, &query->movement);
                    FullDeflectSmallY(&query->impact_normal, movement, movement);

                    query = TerI;
                    if (CurTrackInfo == NULL) {
                        CurTrackInfo = AllocTerrId();
                        CurTrackInfo->flags = TERRAIN_TRACK_FLAG_NONE;
                        CurTrackInfo->id = query->hit_flags;
                        CurTrackInfo->platform_index = 0;
                        CurTrackInfo->platform_contact_state = TERRAIN_TRACK_CONTACT_NONE;
                    }
                    CurTrackInfo->wall_contact_state = TERRAIN_TRACK_CONTACT_ACTIVE;
                    CurTrackInfo->impact_normal = query->impact_normal;
                } else {
                    query->position.x += response_normal.x * contact_push;
                    query->position.y += response_normal.y * contact_push;
                    query->position.y += response_normal.y * 0.01f;
                    query->position.z += response_normal.z * contact_push;
                    query->movement.y += response_normal.y * 0.01f;

                    FullDeflect(&response_normal, &query->movement, &query->movement);
                    FullDeflect(&query->impact_normal, movement, movement);
                }

                query = TerI;
                query->flags |= TERRAIN_QUERY_FLAG_PREVIOUS_NORMAL;
                query->previous_movement_normal = query->movement_normal;

                break;
            }
        }
    }

    query = TerI;
    hit_type = query->hit_type;
    if (hit_type >= TERRAIN_HIT_TYPE_FACE && hit_type <= TERRAIN_HIT_TYPE_CLASS_MASK &&
        query->terrain_group_index != -1) {
        TERRAIN_GROUP *group = &CurTerr->groups[query->terrain_group_index];
        if (group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY && query->impact_normal.y >= walkable_normal_y) {
            PlatformConnect(reinterpret_cast<char *>(query->hit_flags), &query->movement, movement, group->scene_index);
        }
    }

    if (TerImpactData == NULL || *TerImpactDataCount >= TerImpactDataMax) {
        return;
    }

    query = TerI;
    if (query->hit_type == TERRAIN_HIT_TYPE_NONE) {
        return;
    }

    TERRAIN_IMPACT_RECORD *records = static_cast<TERRAIN_IMPACT_RECORD *>(TerImpactData);
    TERRAIN_IMPACT_RECORD &record = records[*TerImpactDataCount];
    record.position.x = query->position.x - query->movement_normal.x * query->collision_radius;
    record.position.y = (query->position.y - query->movement_normal.y * query->collision_radius) * query->object_scale;
    record.position.z = query->position.z - query->movement_normal.z * query->collision_radius;
    record.normal = query->movement_normal;

    if (query->surface != NULL) {
        record.material[0] = query->surface->material[0];
        record.material[1] = query->surface->material[1];
        record.flags = query->surface->flags;
        record.normal_flags = query->surface->normal_flags;
    } else if (TerrWallInfo != 0) {
        record.material[0] = TerrWallTab[0];
        record.material[1] = TerrWallTab[1];
        record.flags = 0;
        record.normal_flags = 0;
    }
    ++*TerImpactDataCount;
}
static i32 TerrainKillPlayer(GameObject_s *object, i32 surface, NUVEC *normal) {
    const u32 flags = TerSurface[surface].flags;
    if ((flags & 1) == 0 &&
        ((flags & 0x4000) == 0 || (object->apiobj.character_data->model_flags & 0x10) != 0 ||
         (object->apiobj.character_data->game_character->flags_090 & 0x8000) != 0) &&
        ((flags & 0x8000) == 0 || (object->apiobj.field_0x27c != -1 && !(normal->y > 0.0f)))) {
        return 0;
    }
    InstantKillParts(object, 1, 0.0f);
    if ((object->apiobj.field_0x1f8 & 0x80) != 0 && BonusWinner == -1) {
        const i32 coins = LoseCoins(object, 1);
        AddPickups(coins, 0, 0, 0, &object->apiobj.collision_position, NULL, 2.0f, -1, 1.0f, 2000000.0f, object, 1, 0,
                   false);
    }
    KillPlayer(object, 2, 1, NULL);
    return 1;
}

void TerrainPlayer(GameObject_s *object) {
    GAMEPAD_s *const entry_pad = object->pad_gamepad;
    const u8 entry_underwater = object->apiobj.is_underwater;
    const u8 entry_intersects_water = object->apiobj.intersects_water;
    const i32 entry_menu_id = GetMenuID();
    const i16 entry_platform = object->apiobj.supporting_platform_id;
    TERRAIN_IMPACT_RECORD impact_records[8];
    i32 impact_count = -1;
    // Original 0x102720 skips integration and callbacks for inactive objects.
    if (object->apiobj.field_0x287 == 0 && object->field_0xcc0 != NULL && object->character_context == 0x3b) {
        TakeOverCode(object, object->pad_gamepad->buttons_pressed & GAMEPAD_TAG);
    } else if (object->apiobj.field_0x287 == 0) {

        APIOBJECT &api = object->apiobj;
        i32 terrain_mask = TERRAINMASK_NONWEAPON;
        if ((api.character_data->model_flags & 0x10) == 0 &&
            (api.character_data->game_character->flags_090 & 0x8000) == 0) {
            terrain_mask |= TERRAINMASK_NONDROID;
        }
        if ((api.field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) != 0 && object->id != id_DRAGBOMB &&
            (WORLD->area != HOTHBATTLE_ADATA || api.character_data->game_character->field_0x28 != 0.0f)) {
            terrain_mask |= 0x40;
        }
        object->field_0xe20 &= static_cast<u8>(~8u);
        const f32 entry_vertical_velocity = api.velocity.y;
        const AIPATHINFO &path_info = object->ai.path_info;
        const AIPATHCNX *path_connection = path_info.connection;
        bool special_path_endpoints = false;
        if (path_info.path != NULL && path_connection != NULL) {
            const AIPATHNODE *nodes = path_info.path->nodes;
            special_path_endpoints = ((nodes[path_connection->node_indices[path_info.direction]].runtime_flags |
                                       nodes[path_connection->node_indices[path_info.direction == 0]].runtime_flags) &
                                      0x82) != 0;
        }
        bool shadow_grounding = false;
        i32 retain_floor = false;
        if (object == CarWashHack) {
            shadow_grounding = true;
        } else if ((api.character_data->game_character->flags_090 & 0x80) != 0) {
            api.velocity.x = 0.0f;
            api.velocity.z = 0.0f;
            shadow_grounding = true;
        } else if ((api.field_0x1f8 & 0x40) == 0 && (CInfo[object->character_context].flags & 0x8000) == 0) {
            const bool ai_controlled = (api.field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) != 0;
            const bool ordinary_ai_path =
                ai_controlled && object->movement_spline == NULL && object->character_context != 0x1c &&
                object->context_target_position == NULL && !special_path_endpoints &&
                (path_info.flags & (AIPATHINFO_FLAG_ON_PATH | AIPATHINFO_FLAG_NARROW_PATH)) ==
                    AIPATHINFO_FLAG_ON_PATH &&
                Technos_FindControllingTechno(object) == NULL && (api.field_0x1fa & 4) == 0 &&
                (path_connection == NULL ||
                 ((path_connection->original_traversal_flags[0] & static_cast<u32>(LEGO_AIPATHCNX_BLOCKAGE)) == 0 &&
                  (path_connection->traversal_flags[0] & static_cast<u32>(LEGO_AIPATHCNX_WALLSHUFFLE)) == 0 &&
                  (path_connection->traversal_flags[0] & static_cast<u32>(LEGO_AIPATHCNX_FULLTERRAIN)) == 0)) &&
                (object->character_context != 0x5a || object->field_0x7a3 != 0);
            if (ordinary_ai_path) {
                shadow_grounding = true;
                retain_floor = (api.field_0x1f8 & 0x10) != 0;
            } else {
                retain_floor = !ai_controlled && (api.field_0x1f8 & 0x10) != 0;
                if ((object->field_0xf00 & 0x40) != 0) {
                    object->field_0xf00 &= static_cast<u8>(~0x40u);
                    shadow_grounding = true;
                } else {
                    shadow_grounding = (api.field_0x1f8 & 8) != 0;
                }
            }
        }

        // TerrainPlayer selects either the inexpensive shadow-grounding path used
        // by ordinary AI or the swept capsule resolver required by players and
        // special traversal connections, then refreshes floor/contact state before
        // dispatching the character movement callback.
        const f32 lower_bound = object->character_bottom * api.field_0xa8;
        // Original 0x102856..0x10316f consumes the previous edge-stop request
        // and probes the next horizontal position before integrating movement.
        const bool player_edge_probe = (api.field_0x1f8 & 0x80) != 0 && object->spawn_protection_timer > 1.25f;
        const bool requested_edge_stop = static_cast<i8>(object->edge_stop_requests) > 0;
        object->field_0xf04 &= static_cast<u8>(~0x40u);
        object->edge_stop_requests = 0;
        if (VehicleArea == 0 && api.field_0x287 == 0 &&
            (api.field_0x27d != 0 || requested_edge_stop ||
             (player_edge_probe && object->character_context == -1 && object->field_0xe31 != 1))) {
            const bool walkable_floor = api.field_0x218 != 2000000.0f && object->surface_normal.y > NuTrigTable[0x238e];
            const bool probe_edge = walkable_floor ? (player_edge_probe || requested_edge_stop ||
                                                      (CInfo[object->character_context].parameter & 1) != 0)
                                                   : requested_edge_stop;
            if (probe_edge && api.movement_direction.x == 0.0f && api.movement_direction.z == 0.0f &&
                (api.velocity.x != 0.0f || api.velocity.z != 0.0f)) {
                NUVEC next_position;
                next_position.x = api.position.x + api.velocity.x * FRAMETIME;
                next_position.y = api.position.y;
                next_position.z = api.position.z + api.velocity.z * FRAMETIME;
                const f32 next_floor = GameShadow(object, &next_position, 5.0f, terrain_mask | 0x1f);
                if (next_floor != 2000000.0f && ShadNorm.y > NuTrigTable[0x238e] &&
                    (player_edge_probe ? api.collision_min.y : api.field_0x218) - next_floor > 0.2f) {
                    object->field_0xf04 |= 0x40;
                    api.velocity.z = 0.0f;
                    api.velocity.x = 0.0f;
                }
            }
        }
        f32 movement_threshold = 0.0f;
        if (!(object->pad_gamepad->input_magnitude > 0.0f) && object->character_context != 0x33) {
            const i32 surface = static_cast<i8>(api.field_0x281);
            const bool slippery =
                api.field_0x27d != 0 && surface >= 0 && surface <= 31 && 1.0f > TerSurface[surface].movement_scale;
            if (!slippery && AnimSpeed(api.character_model, CurrentAnim(&api.anim_packet)) == 0.0f &&
                (api.character_data->model_flags & 0x800) == 0) {
                movement_threshold = api.character_data->game_character->field_0x0c * api.field_0xa8;
            }
        }

        const f32 speed_squared =
            (api.velocity.x * api.velocity.x + api.velocity.y * api.velocity.y) + api.velocity.z * api.velocity.z;
        const bool skip_motion =
            (object->field_0xefc & 0x20) == 0 && (object->field_0xf02 & 0x40) != 0 && !special_path_endpoints &&
            object->ai.field_0x180 == NULL && (api.field_0x1f8 & 0x80) == 0 && (object->field_0xe23 & 0x10) == 0 &&
            object->field_0xe31 == 0 && object->character_context != 0 && object->character_context != 0x1c &&
            api.field_0x281 != 8 && (static_cast<u32>(GameTimer.update_count) > 1 || (api.field_0x1f4 & 5) != 0) &&
            (api.field_0x27d & 3) != 0 && api.supporting_platform_id == -1 &&
            movement_threshold * movement_threshold >= speed_squared && object->character_context != 0x0f &&
            object->character_context != 0x0b && object->character_context != 0x1e;
        api.field_0x1f8 = (api.field_0x1f8 & ~4u) | (skip_motion ? 4u : 0u);
        object->field_0x1084 = 0;
        const bool direct_integration = (api.field_0x1f8 & 0x20) != 0 || (object->field_0xe20 & 0x20) != 0 ||
                                        object->movement_spline != NULL || object->move_override != NULL ||
                                        (object->character_context == 0x0f && object->field_0x7a3 <= 1) ||
                                        (object->character_context == 0x2c && object->field_0x7a3 == 0) ||
                                        (object->character_context == 0x47 && object->action_movement_state == 1);
        if (direct_integration) {
            // Original 0x102940/0x102bf8 selects integration without a terrain
            // query for these motion owners and action states.
            api.respawn_timer = 0.0f;
            object->field_0xe20 |= 2;
            if (TimingBarSet == 2)
                TBOPENFN("Ter", 2);
            api.field_0x27d = 0;
            api.supporting_platform_id = -1;
            object->field_0x6b0 = 0;
            api.position.x += api.velocity.x * FRAMETIME;
            api.position.y += api.velocity.y * FRAMETIME;
            api.position.z += api.velocity.z * FRAMETIME;
            if (TimingBarSet == 2)
                TBCLOSEFN("Ter", 2);
        } else if (shadow_grounding || retain_floor || (WORLD->current_level->flags & 0x10) != 0) {
            // Ordinary path-following AI uses the target's inexpensive shadow
            // grounding path.  Full swept collision is reserved for path
            // connections whose traversal flags require special collision.
            api.respawn_timer = 0.0f;
            if (!skip_motion) {
                if (TimingBarSet == 2)
                    TBOPENFN("Ter", 2);
                object->field_0xe20 |= 2;
                api.supporting_platform_id = -1;
                api.position.x += api.velocity.x * FRAMETIME;
                api.position.y += api.velocity.y * FRAMETIME;
                api.position.z += api.velocity.z * FRAMETIME;

                if (shadow_grounding) {
                    if (object == CarWashHack) {
                        NewTerrPlatformsOff();
                    }
                    if (testshadowfix != 0) {
                        NUVEC shadow_position = api.collision_position;
                        shadow_position.y = api.initial_position.y > api.collision_position.y
                                                ? api.initial_position.y
                                                : api.collision_position.y;
                        api.field_0x218 = GameShadow(object, &shadow_position, 5.0f, terrain_mask | 0x1f);
                    } else {
                        api.field_0x218 = GameShadow(object, &api.collision_position, 5.0f, terrain_mask | 0x1f);
                    }
                    api.supporting_platform_id = static_cast<i16>(NewShadowOnPlatform());
                    GetSurfaceInfo(object, api.field_0x218 != 2000000.0f ? 1 : 0, api.field_0x218);
                    if (api.field_0x218 == 2000000.0f) {
                        api.field_0x281 = 0;
                        object->field_0xe41 = 0;
                        object->surface_normal = NUVEC{0.0f, 1.0f, 0.0f};
                        api.water_height = 2000000.0f;
                        api.field_0x220 = 2000000.0f;
                        api.field_0x27f = static_cast<u8>(-1);
                        api.field_0x280 = static_cast<u8>(-1);
                        object->field_0x1078 = -1;
                    }
                    api.is_underwater = static_cast<u8>(UnderWater(object));
                    api.intersects_water = static_cast<u8>(IntersectWater(object));
                    if (api.supporting_platform_id == -1 && api.field_0x27d != 0 && object->field_0x1078 != -1) {
                        api.supporting_platform_id = object->field_0x1078;
                    }
                }
                const f32 grounded_lower_bound = object->character_bottom * api.field_0xa8;
                if (api.field_0x218 != 2000000.0f && api.position.y + grounded_lower_bound < api.field_0x218) {
                    api.velocity.y = 0.0f;
                    api.position.y = api.field_0x218 - grounded_lower_bound;
                }

                api.field_0x27d = 0;
                if (GameObjectNearFloor(object, 1.0f, NULL) != 0) {
                    api.field_0x27d |= APIOBJECT_TERRAIN_CONTACT_NEAR_FLOOR;
                }
                if (api.collision_min.y <= api.field_0x218) {
                    api.field_0x27d |= APIOBJECT_TERRAIN_CONTACT_FLOOR;
                }
                if ((api.field_0x27d & APIOBJECT_TERRAIN_CONTACT_FLOOR) != 0) {
                    object->field_0x1084 = 1;
                    object->contact_position.x = api.position.x;
                    object->contact_position.y = api.field_0x218;
                    object->contact_position.z = api.position.z;
                    object->field_0x6b0 = api.field_0x281;
                    object->contact_normal = shadow_grounding ? object->surface_normal : NUVEC{0.0f, 1.0f, 0.0f};
                    object->terrain_impact_speed = -api.velocity.y;
                } else {
                    object->field_0x6b0 = 0;
                }
                if (TimingBarSet == 2)
                    TBCLOSEFN("Ter", 2);
            }
        } else {
            if (skip_motion) {
                api.velocity.x = 0.0f;
                api.velocity.y = 0.0f;
                api.velocity.z = 0.0f;
            } else {
                NUVEC incoming_velocity = api.velocity;
                NUVEC collision_position = api.position;
                collision_position.y += lower_bound;

                NUVEC movement;
                NuVecScale(&movement, &api.velocity, FRAMETIME);

                // Original 0x1048e9 clears the complete contact metadata word.
                object->field_0x6b0 = 0;
                memset(object->pad_6b1, 0, sizeof(object->pad_6b1));
                if (TimingBarSet == 2)
                    TBOPENFN("Ter", 2);
                // Original 0x1048fc..0x10497c and 0x10545f..0x105544.
                if (object->use_model_origin != 0 && api.field_0x27d != 0 &&
                    (movement.x != 0.0f || movement.z != 0.0f) &&
                    !(WORLD->current_level == ENDORBATTLEC_LDATA && object->id == id_ATST)) {
                    i32 deflection_mode = 0;
                    if (VehicleArea != 0 && (api.character_data->model_flags & 0x2000) != 0 &&
                        api.character_data->game_character->field_0x28 != 0.0f) {
                        deflection_mode = 2;
                    } else if (object->character_context == -1 && object->pad_gamepad->input_magnitude > 0.0f) {
                        deflection_mode = 1;
                    }
                    if (deflection_mode != 0) {
                        NUVEC deflected;
                        Surface_Deflect(&object->surface_normal, &movement, &deflected, deflection_mode);
                        if (deflected.y > movement.y) {
                            movement = deflected;
                            if (deflection_mode == 1) {
                                object->field_0xe20 |= 8;
                            }
                        }
                    }
                }
                TerrainSetPlatConnectTol((api.character_data->model_flags & 0x800) != 0 ? 0.0f : 0.01f);
                if ((object->ai.field_0x1e4 & 0x80) != 0) {
                    IgnoreWallSplines = 1;
                }
                TerrainSetImpactData(impact_records, &impact_count, 8);
                f32 movement_scale = 1.0f;
                if ((object->field_0xefd & 4) != 0) {
                    GAMECHARACTERDATA *character = api.character_data->game_character;
                    if (object->id == id_SPEEDERBIKE) {
                        if (object->ai.goal_speed_mode == 1) {
                            movement_scale = 15.0f / character->walk_speed;
                        } else if (object->ai.goal_speed_mode == 2) {
                            movement_scale = 15.0f / character->tiptoe_speed;
                        } else {
                            movement_scale = 15.0f / character->run_speed;
                        }
                    } else {
                        movement_scale = 2.5f / character->run_speed;
                    }
                    movement.x *= movement_scale;
                    movement.z *= movement_scale;
                }
                const i32 object_index = Obj != NULL ? static_cast<i32>(object - Obj) : -1;

                // Original 0x104a8f..0x104bdb excludes owned and selected character
                // platforms for this query, then re-enables the same list.
                i32 excluded_platforms[32];
                i32 excluded_count = 0;
                if (object->field_0x107a != -1) {
                    excluded_platforms[excluded_count++] = object->field_0x107a;
                }
                if (object->field_0x107c != -1) {
                    excluded_platforms[excluded_count++] = object->field_0x107c;
                }
                if (object->character_context == 0) {
                    GameObject_s *target = object->airborne_collision_target;
                    if (target != NULL && target->field_0x107c != -1) {
                        excluded_platforms[excluded_count++] = target->field_0x107c;
                    }
                } else if (object->character_context == 0x58 && object->field_0x7a3 == 1) {
                    GIZMOBLOWUP_s *blowup = static_cast<GIZMOBLOWUP_s *>(object->field_0x788);
                    if (blowup->platform_id != -1) {
                        excluded_platforms[excluded_count++] = blowup->platform_id;
                    }
                }
                CHARPLATFORMSYS_s *platforms = WORLD->char_platform_sys;
                if (platforms != NULL && VehicleArea == 0) {
                    const bool player = (api.field_0x1f8 & 0x80) != 0;
                    if (!player || object->character_context == 0x3d || object->field_0xcc0 != NULL ||
                        object->character_context == 0x3b ||
                        (api.character_data->game_character->flags_090 & 0x8040) != 0) {
                        for (i32 i = 0; i < platforms->platform_count; ++i) {
                            GameObject_s *platform_object = platforms->platforms[i].object;
                            if (platform_object != NULL && ((object->field_0xf02 & 8) == 0 || player)) {
                                excluded_platforms[excluded_count++] = platform_object->field_0x107c;
                            }
                        }
                    }
                }
                for (i32 i = 0; i < excluded_count; ++i) {
                    PlatOnOff(excluded_platforms[i], 0);
                }

                NewTerrainScaleYMask(&collision_position, &movement, reinterpret_cast<u8 *>(&object->field_0x105c),
                                     object_index,
                                     entry_pad->input_magnitude > 0.0f ? 0.0f : movement_threshold * FRAMETIME,
                                     api.collision_radius, object->collision_y_scale, 0, 0, terrain_mask);
                IgnoreWallSplines = 0;
                for (i32 i = 0; i < excluded_count; ++i) {
                    PlatOnOff(excluded_platforms[i], 1);
                }
                if ((object->field_0xefd & 4) != 0) {
                    movement.x /= movement_scale;
                    movement.z /= movement_scale;
                }
                extern i32 TERRAINCALLS;
                ++TERRAINCALLS;
                if (TimingBarSet == 2)
                    TBCLOSEFN("Ter", 2);
                api.supporting_platform_id = static_cast<i16>(NewShadowOnPlatform());

                object->field_0x1084 = static_cast<u8>(TerrImpact);
                if (object->field_0x1084 != 0 && impact_count > 0) {
                    // Original copies the last complete 0x1c-byte impact record,
                    // including material and flags, before deriving impact speed.
                    memcpy(&object->contact_position, &impact_records[impact_count - 1], sizeof(TERRAIN_IMPACT_RECORD));
                    object->terrain_impact_speed = -NuVecDot(&object->contact_normal, &incoming_velocity);
                } else {
                    object->field_0x1084 = 0;
                }
                NuVecScale(&api.velocity, &movement, 1.0f / FRAMETIME);
                api.position.x = collision_position.x;
                api.position.z = collision_position.z;
                api.position.y = collision_position.y - object->character_bottom * api.field_0xa8;
                // Original 0x104eb0..0x10501a carries all three headings with the
                // rotation of the supporting platform.
                if (api.field_0x287 == 0 && api.supporting_platform_id != -1) {
                    NUMTX *previous = NULL;
                    NUMTX *current = NULL;
                    TerrainPlatGetMtx(api.supporting_platform_id, &previous, &current);
                    if (previous != NULL && current != NULL) {
                        NUVEC forward = {0.0f, 0.0f, 1.0f};
                        NUVEC previous_forward, current_forward;
                        NuVecMtxRotate(&previous_forward, &forward, previous);
                        const NUANG previous_heading = NuAtan2D(previous_forward.x, previous_forward.z);
                        NuVecMtxRotate(&current_forward, &forward, current);
                        const NUANG current_heading = NuAtan2D(current_forward.x, current_forward.z);
                        const NUANG rotation = NuAngSub(current_heading, previous_heading);
                        if (rotation != 0) {
                            api.field_0x276 = NuAngAdd(rotation, api.field_0x276);
                            api.facing_angle = NuAngAdd(rotation, api.facing_angle);
                            api.movement_facing_angle = NuAngAdd(rotation, api.movement_facing_angle);
                        }
                    }
                }
                if (api.field_0x287 == 0) {
                    bool wall_impact = false;
                    if (object->field_0x1084 != 0 && (api.field_0x1f8 & 0x84) == 0 && object->character_context != 0 &&
                        object->character_context != 0x43 && object->character_context != 0x45 &&
                        (object->field_0xf02 & 1) == 0) {
                        for (i32 i = 0; i < impact_count; ++i) {
                            if (impact_records[i].normal.y < 0.707f) {
                                wall_impact = true;
                                break;
                            }
                        }
                    }
                    if (wall_impact) {
                        api.respawn_timer += FRAMETIME;
                    } else {
                        api.respawn_timer -= FRAMETIME;
                        if (api.respawn_timer < 0.0f) {
                            api.respawn_timer = 0.0f;
                        }
                    }
                }
            }
            const f32 floor_height = (api.field_0x1f8 & 4) != 0
                                         ? api.field_0x218
                                         : GameShadow(object, &api.position, 5.0f, terrain_mask | 0x1f);
            bool stopped_without_terrain = false;
            if (floor_height != 2000000.0f) {
                object->field_0xe20 |= 2;
                const f32 bottom_offset = api.field_0xa8 * object->character_bottom;
                const f32 top = object->character_top * api.field_0xa8 + api.position.y;
                const f32 bottom = api.position.y + bottom_offset;
                const f32 height = top - bottom;
                if (!skip_motion && floor_height > 0.1f * height + bottom &&
                    height + 0.01f >= (floor_height + height) - top) {
                    api.position.y = floor_height - bottom_offset;
                }
            } else {
                object->field_0xe20 &= static_cast<u8>(~2u);
                if (NOTERRAINSTOP != 0) {
                    api.velocity.x = 0.0f;
                    api.velocity.z = 0.0f;
                    api.position.x = api.start_position.x;
                    api.position.z = api.start_position.z;
                    stopped_without_terrain = true;
                }
            }
            if (!stopped_without_terrain && (api.field_0x1f8 & 4) == 0) {
                api.field_0x218 = floor_height;
                GetSurfaceInfo(object, floor_height != 2000000.0f ? 1 : 0, floor_height);
                if (api.supporting_platform_id == -1 && api.field_0x27d != 0 && object->field_0x1078 != -1) {
                    api.supporting_platform_id = object->field_0x1078;
                }
                if ((api.character_data->game_character->flags_090 & 0x8000) == 0 &&
                    object->field_0x1020 == 2000000.0f && object->field_0x1078 != -1) {
                    const f32 reflection_height = FindReflectionNoPlatforms(&api.position);
                    if (reflection_height != 2000000.0f) {
                        object->field_0x1020 = reflection_height;
                        object->field_0x1087 = 2;
                    }
                }
            }
            // Contact reconstruction joins the near-floor test at 0x102d26.
            api.field_0x27d = object->field_0x105c != 0 ? APIOBJECT_TERRAIN_CONTACT_FLOOR : 0;
            if (GameObjectNearFloor(object, 1.0f, NULL) != 0) {
                api.field_0x27d |= APIOBJECT_TERRAIN_CONTACT_NEAR_FLOOR;
            }
            if (api.field_0x27d != 0) {
                object->field_0xeff |= 2;
            }
        }

        // Original 0x1029e6/0x103cb4 transfers vertical momentum only when
        // leaving or landing on the object's recorded character platform.
        const bool left_platform = api.field_0x27d == 0 && entry_platform != -1 && api.supporting_platform_id == -1 &&
                                   object->field_0x1078 == entry_platform;
        const bool landed_platform = api.field_0x27d != 0 && entry_platform == -1 && api.supporting_platform_id != -1 &&
                                     api.supporting_platform_id == object->field_0x1078 &&
                                     entry_vertical_velocity < 0.0f;
        if (left_platform || landed_platform) {
            const i32 platform_id = left_platform ? entry_platform : api.supporting_platform_id;
            GRABBER_s *grabber = WORLD->grabber;
            if (grabber != NULL && grabber->platform_id == platform_id) {
                if (grabber->platform_contact_timer <= 0.0f) {
                    grabber->platform_contact_timer = 0.25f;
                }
            } else {
                GameObject_s *platform_owner = CharPlatform_FindObjFromPlatID(WORLD->char_platform_sys, platform_id);
                if (platform_owner != NULL &&
                    platform_owner->apiobj.character_data->game_character->field_0x28 > 0.0f) {
                    if (left_platform) {
                        platform_owner->apiobj.velocity.y -= entry_vertical_velocity * 0.5f;
                    } else {
                        platform_owner->apiobj.velocity.y =
                            entry_vertical_velocity * 0.5f + platform_owner->apiobj.velocity.y;
                    }
                }
            }
        }

        // Original 0x102a01..0x102a9a handles a moving character platform
        // above the player before dispatching the normal movement callback.
        if (VehicleArea == 0 && (api.field_0x1f8 & 0x80) != 0 && api.supporting_platform_id != -1) {
            GameObject_s *platform_owner =
                CharPlatform_FindObjFromPlatID(WORLD->char_platform_sys, api.supporting_platform_id);
            if (platform_owner != NULL) {
                const GAMECHARACTERDATA *data = platform_owner->apiobj.character_data->game_character;
                if ((data->flags_090 & 0x1000) != 0 && platform_owner->apiobj.position.y > api.position.y &&
                    platform_owner->apiobj.horizontal_velocity_magnitude > 0.5f * data->run_speed) {
                    StartFlatten(platform_owner, object);
                }
            }
        }

        if (api.field_0x287 != 0) {
            if (api.field_0x27d != 0) {
                api.velocity.y = 0.0f;
                api.field_0x1f8 |= 4;
            }
            if ((api.field_0x1f8 & 4) == 0) {
                ApplyGravity(object, NULL, 0.0f, 0.0f, NULL);
            }
        } else {
            // Original 0x1032a0 checks swept contacts or the standing surface.
            const i32 check_terrain_hazards =
                object->character_context != 0x2b && (api.field_0x1f8 & 4) == 0 && gone_through_door_to_new_level == 0;
            // Original 0x1038b0 completes the doomed state on contact, timeout,
            // or a collision during the falling variant.
            if (object->character_context == 0x2b &&
                (api.field_0x27d != 0 || api.model_draw_result == 0 || object->turn_braking >= 5.0f ||
                 (object->field_0xe36 == 3 && object->field_0x1084 != 0))) {
                if (api.model_draw_result != 0) {
                    InstantKillParts(object, 0, 0.0f);
                }
                KillPlayer(object, 3, 1, NULL);
                if (WORLD->current_level == CRUISERD_LDATA && object->field_0xe36 == 3) {
                    MiscTime = 0.1f;
                }
            }
            if (check_terrain_hazards && CannotKill(object) == 0 && api.field_0x287 == 0 &&
                (VehicleArea == 0 || (api.field_0x1f8 & 0x80) != 0) && (object->field_0xefe & 4) != 0 &&
                (object->field_0xefa & 4) == 0 && object->character_context != 0x5d) {
                if (object->field_0x1084 != 0) {
                    const i32 count = impact_count == -1 ? 1 : impact_count;
                    for (i32 i = 0; i < count; ++i) {
                        const i32 surface =
                            static_cast<i8>(impact_count == -1 ? object->field_0x6b0 : impact_records[i].material[0]);
                        NUVEC *normal = impact_count == -1 ? &object->contact_normal : &impact_records[i].normal;
                        if (static_cast<u32>(surface) > 31) {
                            continue;
                        }
                        if (WORLD->current_level == BONUS_GUNSHIPA_LDATA ||
                            (PODRACE_ADATA != NULL && WORLD->area == PODRACE_ADATA && GameTimer.time_elapsed >= 1.0f)) {
                            NewRumble(object->pad_gamepad->pad, (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) * 0.5f,
                                      0);
                            if (api.model_draw_result == 0 && object->character_context != 0x23) {
                                if ((api.field_0x1f8 & 0x80) != 0) {
                                    LoseCoins(object, 2);
                                }
                                KillPlayer(object, 2, 1, NULL);
                                break;
                            }
                        } else if (((api.character_data->model_flags & 0x2000) != 0 ||
                                    (api.character_data->game_character->flags_090 & 0x04000000) != 0) &&
                                   api.field_0x27f == 6) {
                            break;
                        }
                        if ((WORLD->area == NULL || WORLD->area != PODSPRINT_ADATA) &&
                            TerrainKillPlayer(object, surface, normal) != 0) {
                            if (WORLD->current_level == ASTEROIDCHASEC_LDATA) {
                                GameCam_Blend(GameCam, 1.0f, 0.0f, 1);
                            }
                            break;
                        }
                        if ((TerSurface[surface].flags & 0x80) != 0 && WORLD->current_level != BONUS_GUNSHIPA_LDATA) {
                            NewRumble(object->pad_gamepad->pad, 0.6f, 0);
                            NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                            if (object->field_0xd24 != 1.0f && CannotKill(object) == 0) {
                                ObjHitObj(NULL, object, 1, 0, 0, 1);
                            }
                            break;
                        }
                    }
                } else if (api.field_0x27d != 0 && (WORLD->area == NULL || WORLD->area != PODSPRINT_ADATA)) {
                    const i32 surface = static_cast<i8>(api.field_0x281);
                    if (static_cast<u32>(surface) <= 31) {
                        TerrainKillPlayer(object, surface, &object->surface_normal);
                    }
                }
            }
            if (check_terrain_hazards && CannotKill(object) == 0 && (object->field_0xefa & 4) == 0 &&
                api.field_0x287 == 0 && api.water_height != 2000000.0f) {
                const i32 layer = static_cast<i8>(api.field_0x27f);
                const bool tractor_swamp =
                    layer == 9 && object->id == id_TRACTOR && WORLD->area != NULL && WORLD->area == DAGOBAH_ADATA;
                if ((layer == 3 || tractor_swamp || (TerLayer[layer].flags & 0x20) != 0 ||
                     ((api.character_data->model_flags & 0x2000) == 0 &&
                      (api.character_data->game_character->flags_090 & 0x04000000) == 0)) &&
                    NoLayerKill(object) == 0 && object->character_context != 0x5d) {
                    const u32 layer_flags = TerLayer[static_cast<i8>(api.field_0x27f)].flags;
                    const bool exempt_context = (api.field_0x1f8 & 0x80) == 0 && (object->character_context == 0x46 ||
                                                                                  object->character_context == 0x47);
                    const bool rescue_below =
                        ((layer_flags & 1) != 0 || tractor_swamp) && api.water_height > api.position.y &&
                        !(VehicleArea != 0 && BonusArea != 0 && api.character_data->game_character->field_0x28 > 0.0f);
                    const bool rescue_above = (layer_flags & 0x20) != 0 && api.collision_max.y > api.water_height;
                    if ((rescue_below || rescue_above) && !exempt_context) {
                        if (object->doomed_escape_locator != NULL && (api.field_0x1f8 & 0x80) == 0) {
                            if ((object->field_0xefd & 8) != 0 || TouchHacks::AiPlayerTakeDamageOnKillRescue(*object)) {
                                ObjHitObj(NULL, object, 1, 0, 0, 1);
                            }
                            if (api.field_0x287 == 0) {
                                StartBigJump(object, &object->doomed_escape_locator->position, 0, 1.0f, 1.0f, 0, 0);
                            }
                        } else {
                            ClearLastSafeTakeOver(object);
                            Player_ClearContext(object, 1);
                            object->context_animation = 5;
                            if (object->character_context != -1 &&
                                api.character_model->model_data_b[api.anim_packet.requested_animation] != NULL) {
                                object->context_animation = api.anim_packet.requested_animation;
                            }
                            object->character_context = 0x2b;
                            object->field_0xe31 = 0;
                            if ((TerLayer[static_cast<i8>(api.field_0x27f)].flags & 0x20) != 0) {
                                object->field_0xe36 = 3;
                            } else if (api.field_0x27f == 6) {
                                api.velocity.y = -0.25f;
                                object->field_0xe36 = 2;
                                NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                            } else if (WORLD->current_level == VADERC_LDATA) {
                                api.velocity.y = -0.5f;
                                object->field_0xe36 = 4;
                                NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
                            } else {
                                object->field_0xe36 = 1;
                            }
                            object->turn_braking = 0.0f;
                            PlayDieSfx(object);
                            object->current_hp = 0;
                            if ((api.field_0x1f8 & 0x80) != 0 && object->coinpacket != NULL &&
                                object->coinpacket->coins != 0 && BonusWinner == -1) {
                                const i32 coins = LoseCoins(object, 2);
                                if (coins > 0) {
                                    f32 height = api.water_height;
                                    if (height != 2000000.0f &&
                                        (TerLayer[static_cast<i8>(api.field_0x27f)].flags & 1) == 0) {
                                        height = 2000000.0f;
                                    }
                                    AddPickups(coins, 0, 0, 0, &api.collision_position, NULL, 2.0f,
                                               static_cast<i8>(api.field_0x27c), 1.0f, height, NULL, 1, 0, false);
                                }
                                object->coinpacket->scale = 1.5f;
                            }
                        }
                    } else if ((api.character_data->model_flags & 0x40) == 0 && object->id != id_SNAKE &&
                               api.field_0x27f == 9 && api.water_height > api.collision_max.y) {
                        if (BonusWinner == -1) {
                            const i32 coins = LoseCoins(object, 2);
                            if (coins > 0) {
                                AddPickups(coins, 0, 0, 0, &api.upper_position, NULL, 2.0f,
                                           static_cast<i8>(api.field_0x27c), 1.0f, 2000000.0f, NULL, 1, 0, false);
                            }
                        }
                        InstantKillParts(object, 1, 2.0f);
                        KillPlayer(object, 3, 1, NULL);
                    }
                }
            }
            // Original 0x1036a0 refreshes these even if the hazard handler killed
            // the object. The bypass and level-transition routes retain old state.
            if (check_terrain_hazards) {
                api.is_underwater = static_cast<u8>(UnderWater(object));
                api.intersects_water = static_cast<u8>(IntersectWater(object));
            }
            // Original 0x102ddb emits the splash only on a dry-to-water transition.
            if (api.intersects_water != 0 && (entry_underwater | entry_intersects_water) == 0) {
                bool splash = object->character_context == 0x1f || object->character_context == 0 ||
                              (CInfo[object->character_context].flags & 4) != 0 || object->character_context == 0x0e ||
                              object->character_context == 0x0d;
                if (!splash) {
                    const i32 animation = CurrentAnim(&api.anim_packet);
                    splash = animation == 0x28 || animation == 5 || static_cast<u32>(animation - 0x4b) <= 1;
                }
                if (splash) {
                    AddWaterSplash(object, &api.collision_position);
                }
            }
            if (api.model_draw_result != 0 && object->character_context != 0x40 && object->character_context != 0x2c) {
                extern void GameObjectRotation(GameObject_s *, i32);
                GameObjectRotation(object, 2);
            }
            // The jump callback returns to the common saved-button restoration
            // at 0x102ed4, including when it changes the object's gamepad.
            const u32 held_buttons = object->pad_gamepad->buttons_held;
            const u32 pressed_buttons = object->pad_gamepad->buttons_pressed;
            if ((api.field_0x1f8 & 0x180) == 0x80 && (entry_menu_id != -1 || MiniCutCam == 2)) {
                object->pad_gamepad->buttons_held = 0;
                object->pad_gamepad->buttons_pressed = 0;
            }
            Tag_Check(object);
            PreResetCode(object);
            if (api.character_data != NULL && api.character_data->move_fn != NULL) {
                api.character_data->move_fn(object);
            }
            PostResetCode(object);
            if ((api.character_data->model_flags & 0x00200000) == 0) {
                BigJumpCode(object);
            }
            object->pad_gamepad->buttons_held = held_buttons;
            object->pad_gamepad->buttons_pressed = pressed_buttons;
        }

        if ((object->field_0xefa & 4) == 0 && CurTerr != NULL && api.field_0x287 == 0 &&
            (WORLD->current_level->flags & 0xe0) == 0 &&
            static_cast<f32>(CurTerr->minimum_height) - 5.0f > api.position.y) {
            if (api.model_draw_result != 0) {
                InstantKillParts(object, 0, 0.0f);
            }
            KillPlayer(object, 3, 1, NULL);
        }
    }

    // Common epilogue at 0x102b60.
    entry_pad->allocated_5a &= 0xe9;
    object->field_0xd14 = 0;
    entry_pad->operator_data = NULL;
    NuVecRotateYValZ(&object->facing_direction, 1.0f, object->apiobj.field_0x276);
}
static inline void MakePlayCorner(GAMECAMERA_s *camera, f32 screen_x, f32 screen_y, NUVEC *near_corner,
                                  NUVEC *far_corner) {
    NUVEC ray = {screen_x, screen_y, 1.0f};
    NuVecMtxRotate(&ray, &ray, &camera->target_mtx);
    NuVecAdd(near_corner, &ray, &camera->pos);
    NuVecNorm(&ray, &ray);
    far_corner->x = near_corner->x + ray.x * 5.0f;
    far_corner->y = near_corner->y + ray.y * 5.0f;
    far_corner->z = near_corner->z + ray.z * 5.0f;
}

static inline void SetPlayPlane(PLAYPLANE_s *plane, NUVEC *a, NUVEC *b, NUVEC *c, NUVEC *d, NUVEC *edge_a0,
                                NUVEC *edge_a1, NUVEC *edge_b0, NUVEC *edge_b1) {
    plane->point.x = (a->x + b->x + c->x + d->x) * 0.25f;
    plane->point.y = (a->y + b->y + c->y + d->y) * 0.25f;
    plane->point.z = (a->z + b->z + c->z + d->z) * 0.25f;
    NUVEC first_edge;
    NUVEC second_edge;
    NuVecSub(&first_edge, edge_a0, edge_a1);
    NuVecSub(&second_edge, edge_b0, edge_b1);
    NuVecCross(&plane->normal, &first_edge, &second_edge);
    NuVecNorm(&plane->normal, &plane->normal);
}

void MakePlayPlanes(GAMECAMERA_s *camera) {
    NUVEC near_corner[4];
    NUVEC far_corner[4];
    MakePlayCorner(camera, -0.85f * PANEL3DMULX, 0.85f * PANEL3DMULY, &near_corner[0], &far_corner[0]);
    MakePlayCorner(camera, 0.85f * PANEL3DMULX, 0.85f * PANEL3DMULY, &near_corner[1], &far_corner[1]);
    MakePlayCorner(camera, -0.85f * PANEL3DMULX, -0.85f * PANEL3DMULY, &near_corner[2], &far_corner[2]);
    MakePlayCorner(camera, 0.85f * PANEL3DMULX, -0.85f * PANEL3DMULY, &near_corner[3], &far_corner[3]);

    // Plane order is near, left, right, top, bottom, far.
    SetPlayPlane(&PlayPlane[1], &near_corner[0], &far_corner[0], &far_corner[2], &near_corner[2], &far_corner[0],
                 &near_corner[0], &far_corner[2], &near_corner[0]);
    SetPlayPlane(&PlayPlane[2], &far_corner[1], &near_corner[1], &near_corner[3], &far_corner[3], &near_corner[1],
                 &far_corner[1], &near_corner[3], &far_corner[1]);
    SetPlayPlane(&PlayPlane[3], &near_corner[0], &near_corner[1], &far_corner[1], &far_corner[0], &near_corner[1],
                 &near_corner[0], &far_corner[1], &near_corner[0]);
    SetPlayPlane(&PlayPlane[4], &near_corner[2], &far_corner[2], &far_corner[3], &near_corner[3], &far_corner[2],
                 &near_corner[2], &far_corner[3], &near_corner[2]);
    SetPlayPlane(&PlayPlane[0], &near_corner[0], &near_corner[2], &near_corner[3], &near_corner[1], &near_corner[2],
                 &near_corner[0], &near_corner[3], &near_corner[0]);
    SetPlayPlane(&PlayPlane[5], &far_corner[0], &far_corner[1], &far_corner[3], &far_corner[2], &far_corner[1],
                 &far_corner[0], &far_corner[3], &far_corner[0]);
}
void TerrDrawPlatCol(tertype *, i16, i32) {
}
void TerrShowCamTerr() {
}
NUVEC TerrainStaticMtx(PLATSKININFO *info, nuvec_s *position, i32) {
    NUVEC4 point __attribute__((aligned(16)));
    point.x = position->x;
    point.y = position->y;
    point.z = position->z;
    point.w = 1.0f;
    NuVec4MtxTransformVU0(&point, &point, static_cast<NUMTX *>(info->matrix_data));
    NUVEC result;
    result.x = point.x;
    result.y = point.y;
    result.z = point.z;
    return result;
}
void ScanTerrainHandel(i32, i16 *) {
}
extern "C" void NewShapeInit(NUVEC *offset) {
    offset->x = 0.0f;
    offset->y = 0.0f;
    offset->z = 0.0f;
}

void TerrainImpactNorm() {
    TerrainMoveImpactData();

    const u8 maximum_supported_hit_type = TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_SPHERE;
    if (TerI->hit_type > maximum_supported_hit_type) {
        return;
    }

    const i32 hit_type_flag = 1 << TerI->hit_type;
    const i32 sphere_class_mask = 1 << TERRAIN_HIT_TYPE_SPHERE;
    const i32 rotate_and_mark_mask = sphere_class_mask | (sphere_class_mask << TERRAIN_HIT_TYPE_SECOND_NORMAL);
    const i32 rotated_surface_mask = (1 << TERRAIN_HIT_TYPE_CYLINDER) | (1 << TERRAIN_HIT_TYPE_VERTEX);
    const i32 rotate_mask = rotated_surface_mask | (rotated_surface_mask << TERRAIN_HIT_TYPE_SECOND_NORMAL);
    const i32 face_class_mask = 1 << TERRAIN_HIT_TYPE_FACE;
    const i32 direct_normal_mask = face_class_mask | (face_class_mask << TERRAIN_HIT_TYPE_SECOND_NORMAL);

    if ((hit_type_flag & rotate_and_mark_mask) != 0) {
        terrhitflags |= 4;
    }

    const bool rotated_hit = (hit_type_flag & (rotate_and_mark_mask | rotate_mask)) != 0;
    if (rotated_hit) {
        RotateVec(&TerI->movement_normal, &TerI->movement_normal);
    } else if ((hit_type_flag & direct_normal_mask) == 0) {
        return;
    }

    TerrainQuery_s *query = TerI;
    // Curved hits are produced in collision-height-scaled space and need to
    // be transformed back. Face normals already come from the terrain in
    // object space; the target's direct-face branch deliberately bypasses
    // this scaling before copying the normal below.
    if (rotated_hit && (query->hit_type & TERRAIN_HIT_TYPE_SECOND_NORMAL) == 0) {
        query->movement_normal.x *= query->inverse_collision_radius;
        query->movement_normal.y *= query->inverse_collision_radius;
        query->movement_normal.z *= query->inverse_collision_radius;
    }

    if (query->object_scale == 1.0f) {
        query->impact_normal = query->movement_normal;
        return;
    }

    const f32 normal_length =
        NuFsqrt(query->movement_normal.x * query->movement_normal.x +
                query->movement_normal.y * query->movement_normal.y * query->inverse_object_scale_sq +
                query->movement_normal.z * query->movement_normal.z);
    f32 inverse_normal_length = 0.0f;
    if (normal_length != 0.0f) {
        inverse_normal_length = 1.0f / normal_length;
    }

    query = TerI;
    query->impact_normal.x = query->movement_normal.x * inverse_normal_length;
    query->impact_normal.y = query->movement_normal.y * query->inverse_object_scale * inverse_normal_length;
    query->impact_normal.z = query->movement_normal.z * inverse_normal_length;
}
void ScanTerrainPlatform(i32 group_index, i32 terrain_mask) {
    ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT1);
    platinrange = 0;
    TerI->scan_group_index = -1;
    TerrainScanWriter writer;
    writer.group_header = TerI->scan_list_storage;
    writer.cursor = reinterpret_cast<TERRAIN_SHAPE **>(writer.group_header + sizeof(TERRAIN_SHAPE *));
    writer.limit = TerI->scan_list_storage + sizeof(TerI->scan_list_storage) - 12;
    writer.group_shape_count = 0;
    writer.scaled_shape_count = 0;
    TerrainScanBounds bounds = TerrainGetScanBounds(*TerI);
    if (TerI->scan_result != 1) {
        const f32 reach = TerI->collision_radius_sq + 0.02f + TerI->movement.x * TerI->movement.x +
                          TerI->movement.y * TerI->movement.y + TerI->movement.z * TerI->movement.z;
        bounds.min_x = TerI->position.x - reach;
        bounds.max_x = TerI->position.x + reach;
        bounds.min_y = TerI->position.y - TerI->object_scale * reach;
        bounds.max_y = TerI->position.y + TerI->object_scale * reach;
        bounds.min_z = TerI->position.z - reach;
        bounds.max_z = TerI->position.z + reach;
    }
    bounds.min_x -= 0.05f;
    bounds.min_y -= 0.05f;
    bounds.min_z -= 0.05f;
    bounds.max_x += 0.05f;
    bounds.max_y += 0.05f;
    bounds.max_z += 0.05f;
    TerI->scan_list = TerI->scan_list_storage;
    TerrainScanPlatformGroup(&writer, bounds, group_index, terrain_mask, 0, 1.5f);
    i16 *terminator = reinterpret_cast<i16 *>(writer.group_header);
    terminator[0] = 0;
    terminator[1] = 0;
}
i32 TerrainBlockOnBlock(WORLDINFO_s *world, pushblock_s *block, nuvec_s *points, float *heights) {
    if (!heights || !points)
        return 0;
    heights[0] = heights[1] = heights[2] = heights[3] = -10000.0f;
    for (i32 i = 0; i < world->push_block_count; ++i) {
        pushblock_s *other = &world->push_blocks[i];
        if (other == block)
            continue;
        f32 xmin = other->position->x - fabsf(other->bounds_min.x),
            xmax = other->position->x + fabsf(other->bounds_max.x);
        f32 zmin = other->position->z - fabsf(other->bounds_min.z),
            zmax = other->position->z + fabsf(other->bounds_max.z);
        f32 height = fabsf(other->bounds_max.y) + other->position->y;
        for (i32 j = 0; j < 4; ++j)
            if (points[j].x > xmin && points[j].x < xmax && points[j].z > zmin && points[j].z < zmax)
                heights[j] = height;
    }
    return 1;
}

extern PLATSKININFO *PlatSkinInfo;
extern PLATSKINMEMINFO *SkinMemInfo;
extern i32 PlatSkinCnt;
extern i32 PlatSkinMaxStore;
extern i32 PlatSkinMaxSize;
extern i32 TerrainUpadteCnt;
extern u8 *PlatSkinMem;
void SkinPlatform(terrsitu_s *, unsigned char *, PLATSKININFO *);

void TerrainSkinAllocate(terrsitu_s *terrain_group) {
    TERRAIN_GROUP *group = reinterpret_cast<TERRAIN_GROUP *>(terrain_group);
    i32 skin_index = ~static_cast<i32>(group->scene_index);
    if (skin_index >= PlatSkinCnt)
        return;
    ++TerrainUpadteCnt;
    PLATSKININFO *info = &PlatSkinInfo[skin_index];
    if (group->data != NULL) {
        SkinMemInfo[info->cache_slot].last_used = TerrainUpadteCnt;
        return;
    }
    i32 slot = 0;
    if (SkinMemInfo[0].skin_index != -1) {
        i32 oldest = SkinMemInfo[0].last_used;
        for (i32 i = 1; i < PlatSkinMaxStore; ++i) {
            if (SkinMemInfo[i].skin_index == -1) {
                slot = i;
                break;
            }
            if (SkinMemInfo[i].last_used < oldest) {
                oldest = SkinMemInfo[i].last_used;
                slot = i;
            }
        }
    }
    PLATSKINMEMINFO *cache = &SkinMemInfo[slot];
    if (cache->skin_index >= 0) {
        CurTerr->groups[PlatSkinInfo[cache->skin_index].terrain_group].data = NULL;
        for (i32 i = 0; i < 16; ++i)
            CurTerr->index_levels[i].entry_count = 0;
    }
    group->data = info->terrain_data;
    info->cache_slot = slot;
    cache->skin_index = skin_index;
    SkinPlatform(terrain_group, PlatSkinMem + slot * PlatSkinMaxSize, info);
    SkinMemInfo[slot].last_used = TerrainUpadteCnt;
}
void ScanTerrIDRemovePlat(i32 platform_index) {
    TERRAIN_TRACK_SLOT *slot = CurTerr->track_slots;
    i32 remaining = TERRAIN_TRACK_SLOT_COUNT;
    do {
        if (slot->platform_index == platform_index) {
            slot->id = NULL;
        }
        ++slot;
        --remaining;
    } while (remaining != 0);
}
void ScanWallSplineTerrain(i32, i32 terrain_mask, i32) {
    WallSplinesOnly = 0;
    ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT1);
    platinrange = 0;
    TerI->scan_group_index = -1;
    i16 *terminator = reinterpret_cast<i16 *>(TerI->scan_list_storage);
    terminator[0] = 0;
    terminator[1] = 0;
    TerrainCollectWallSplines(TerrainGetScanBounds(*TerI), terrain_mask);
}

i32 HitWallSpline() {
    i32 hit = 0;
    if (WallSplCount == 0)
        return hit;
    for (i32 i = 0; i < WallSplCount; i += 2) {
        const NUVEC &a = WallSplList[i].position;
        const NUVEC &b = WallSplList[i + 1].position;
        TerrainQuery_s *query = TerI;
        if (fabsf(query->position.x - a.x) >= 64.0f || fabsf(query->position.z - a.z) >= 64.0f)
            continue;
        NUVEC normal = {b.z - a.z, 0.0f, a.x - b.x};
        NuVecNorm(&normal, &normal);
        const f32 radius = query->collision_radius;
        const f32 px = query->position.x;
        const f32 pz = query->position.z;
        const f32 mx = query->movement.x;
        const f32 mz = query->movement.z;
        const f32 end_distance = ((mx + px) - a.x) * normal.x + ((mz + pz) - a.z) * normal.z - radius;
        const f32 start_distance = (px - a.x) * normal.x + (pz - a.z) * normal.z - radius;
        if ((end_distance < 0.0f || start_distance < 0.0f) && start_distance > -radius) {
            f32 time, x, z;
            if (start_distance < 0.0f) {
                time = start_distance;
                x = (px - normal.x * start_distance) - normal.x * radius;
                z = (pz - normal.z * start_distance) - normal.z * radius;
            } else {
                time = start_distance / (start_distance - end_distance);
                x = (mx * time + px) - normal.x * radius;
                z = (mz * time + pz) - normal.z * radius;
            }
            const f32 dx = b.x - a.x;
            const f32 dz = b.z - a.z;
            if ((x - a.x) * dx + dz * (z - a.z) >= -0.00005f && -dx * (x - b.x) - (z - b.z) * dz >= -0.00005f &&
                time < query->hit_time) {
                query->shape_adjusted = 4;
                query->terrain_group_index = -1;
                query->hit_type = start_distance > 0.0f ? 1 : 0x11;
                query->surface = NULL;
                query->movement_normal = normal;
                query->hit_time = time;
                TerrWallInfo = 1;
                TerrWallTab[0] = WallSplList[i].material[0];
                TerrWallTab[1] = WallSplList[i].material[1];
                hit = 1;
            }
        }
        const f32 dx = a.x - px;
        const f32 dz = a.z - pz;
        const f32 reach = radius + 0.005f + query->horizontal_movement_length;
        const f32 distance_sq = dx * dx + dz * dz;
        if (fabsf(dx) >= 64.0f || fabsf(dz) >= 64.0f || distance_sq >= reach * reach)
            continue;
        NUVEC direction = {mx, 0.0f, mz};
        NuVecNorm(&direction, &direction);
        const f32 along = direction.x * dx + direction.z * dz;
        const f32 padded_radius = radius + 0.0005f;
        bool penetrating = false;
        f32 time = 0.0f;
        if (along < 0.0f) {
            if (distance_sq > padded_radius * padded_radius)
                continue;
            penetrating = true;
        } else {
            const f32 side_x = a.x - (direction.x * along + px);
            const f32 side_z = a.z - (direction.z * along + pz);
            const f32 side_x_sq = side_x * side_x;
            const f32 side_z_sq = side_z * side_z;
            if (side_x_sq + side_z_sq <= padded_radius * padded_radius) {
                const f32 distance = along - NuFsqrt((radius * radius - side_x_sq) - side_z_sq);
                const f32 movement_sq = mx * mx + mz * mz;
                if (distance < -0.0005f || movement_sq < distance * distance) {
                    if (distance_sq > padded_radius * padded_radius)
                        continue;
                    penetrating = true;
                } else {
                    const f32 length = NuFsqrt(movement_sq);
                    if (length != 0.0f && distance != 0.0f)
                        time = MAX(distance / length, 0.0f);
                    if (time >= query->hit_time)
                        continue;
                    normal.x = direction.x * distance + px - a.x;
                    normal.z = distance * direction.z + pz - a.z;
                    NuVecNorm(&normal, &normal);
                }
            } else {
                if (distance_sq > padded_radius * padded_radius)
                    continue;
                penetrating = true;
            }
        }
        if (penetrating) {
            normal.x = px - a.x;
            normal.z = pz - a.z;
            NuVecNorm(&normal, &normal);
            time = (NuFsqrt(distance_sq) - radius) - 0.0005f;
        }
        query->hit_type = penetrating ? 0x11 : 1;
        query->hit_time = time;
        query->movement_normal = normal;
        query->shape_adjusted = 4;
        query->surface = NULL;
        query->terrain_group_index = -1;
        TerrWallInfo = 1;
        TerrWallTab[0] = WallSplList[i].material[0];
        TerrWallTab[1] = WallSplList[i].material[1];
        hit = 1;
    }
    TerI->unclamped_hit_time = TerI->hit_time;
    if (TerI->hit_time < 0.0f)
        TerI->hit_time = 0.0f;
    return hit;
}
i32 TerrainImpactPlatform(unsigned char *hit_flags) {
    TerrainMoveImpactData();

    TerrainQuery_s *query = TerI;
    if (query->hit_type == TERRAIN_HIT_TYPE_NONE) {
        hit_flags[0] = 0;
        query->position.x += query->movement.x;
        query->position.y += query->movement.y;
        query->position.z += query->movement.z;
        return 0;
    }

    const u8 maximum_platform_hit_type = TERRAIN_HIT_TYPE_SPHERE;
    if (query->hit_type > maximum_platform_hit_type) {
        return 1;
    }

    query->hit_time -= query->separation_epsilon;
    if (query->hit_time < 0.0f) {
        query->hit_time = 0.0f;
    }

    query->position.x += query->movement.x * query->hit_time;
    query->position.y += query->movement.y * query->hit_time;
    query->position.z += query->movement.z * query->hit_time;

    const f32 minimum_walkable_normal_y = 0.707f;
    if (query->impact_normal.y < minimum_walkable_normal_y) {
        hit_flags[0] = 0;
        return 0;
    }

    hit_flags[0] = 1;
    hit_flags[1] = 1;
    query->position.y += query->movement_normal.y * 0.004f;
    return 0;
}
void TerrainMoveImpactData() {
    const i32 stored_class_mask = (1 << TERRAIN_HIT_TYPE_FACE) | (1 << TERRAIN_HIT_TYPE_CYLINDER) |
                                  (1 << TERRAIN_HIT_TYPE_VERTEX) | (1 << TERRAIN_HIT_TYPE_SPHERE);
    const i32 stored_hit_type_mask = stored_class_mask | (stored_class_mask << TERRAIN_HIT_TYPE_SECOND_NORMAL);
    const u8 maximum_stored_hit_type = TERRAIN_HIT_TYPE_SECOND_NORMAL | TERRAIN_HIT_TYPE_SPHERE;

    if (TerI->hit_type > maximum_stored_hit_type) {
        return;
    }

    i32 hit_type_flag = 1 << TerI->hit_type;
    hit_type_flag &= stored_hit_type_mask;
    if (hit_type_flag == 0 || TerI->terrain_group_index == -1) {
        return;
    }

    NewTerrStoreAnyInfo();
    TerrPoly = &TerrPolyInfo;
    TerrPolyObj = TerI->terrain_group_index;
}
extern "C" void *NuScratchAlloc32(i32 size);
extern "C" void NuScratchRelease();

static NUMTX tertempmat;
static NUVEC4 tertempvec4;

i32 TerrainPlatformMoveCheck(nuvec_s *, nuvec_s *, i32, i32, i32);

void FullDeflectSize(nuvec_s *normal, nuvec_s *movement, nuvec_s *result) {
    f32 deflection = (-movement->y * normal->y - movement->x * normal->x) - movement->z * normal->z;
    f32 original_length = NuFsqrt(movement->x * movement->x + movement->y * movement->y + movement->z * movement->z);
    NUVEC output = {normal->x * deflection + movement->x, normal->y * deflection + movement->y,
                    normal->z * deflection + movement->z};
    *result = output;
    f32 deflected_length = NuFsqrt(output.x * output.x + output.y * output.y + output.z * output.z);
    if (deflected_length != 0.0f) {
        f32 scale = (original_length * 0.75f) / deflected_length + 0.25f;
        result->x *= scale;
        result->y *= scale;
        result->z *= scale;
    }
}

void FullReflectTest(nuvec_s *normal, nuvec_s *movement, nuvec_s *result) {
    f32 deflection = (-movement->y * normal->y - movement->x * normal->x) - movement->z * normal->z;
    f32 original_length = NuFsqrt(movement->x * movement->x + movement->y * movement->y + movement->z * movement->z);
    NUVEC output = {(normal->x * deflection) * 2.0f + movement->x, (normal->y * deflection) * 2.0f + movement->y,
                    (normal->z * deflection) * 2.0f + movement->z};
    *result = output;
    f32 deflected_length = NuFsqrt(output.x * output.x + output.y * output.y + output.z * output.z);
    if (deflected_length != 0.0f) {
        f32 scale = original_length / deflected_length;
        result->x *= scale;
        result->y *= scale;
        result->z *= scale;
    }
}

i32 FullDeflectTest(nuvec_s *normal, nuvec_s *movement, nuvec_s *result) {
    f32 deflection = ((-movement->y * normal->y - movement->x * normal->x) - movement->z * normal->z) + 0.0003f;
    NUVEC output = {normal->x * deflection + movement->x, normal->y * deflection + movement->y,
                    normal->z * deflection + movement->z};
    *result = output;
    return deflection > 0.0f;
}

static f32 TerConTol = 0.1f;

extern "C" void TerrainSetPlatConnectTol(f32 tolerance) {
    TerConTol = tolerance;
}

i32 PlatformChecks(i32 count, nuvec_s *movement) {
    TerrainQuery_s *old_query = TerI;
    if (old_query->scan_list == NULL || *static_cast<i16 *>(old_query->scan_list) == 0 || CurTrackInfo == NULL ||
        CurTrackInfo->platform_contact_state == 0 || TerConTol == 0.0f)
        return count;
    TerrainQuery_s *query = static_cast<TerrainQuery_s *>(NuScratchAlloc32(sizeof(TerrainQuery_s)));
    TerI = query;
    i16 *input = static_cast<i16 *>(old_query->scan_list);
    i16 *output = reinterpret_cast<i16 *>(query->scan_list_storage);
    while (input[0] > 0) {
        i32 shape_count = input[0];
        *output++ = *input++;
        *output++ = *input++;
        for (i32 i = 0; i < shape_count; ++i) {
            *output++ = *input++;
            *output++ = *input++;
        }
    }
    output[0] = 0;
    output[1] = 0;
    query->collision_radius = old_query->collision_radius;
    query->object_scale = old_query->object_scale;
    query->inverse_object_scale_sq = old_query->inverse_object_scale_sq;
    query->inverse_collision_radius = old_query->inverse_collision_radius;
    query->separation_epsilon = old_query->separation_epsilon;
    query->inverse_object_scale = old_query->inverse_object_scale;
    query->compare_epsilon = old_query->compare_epsilon;
    query->collision_radius_sq = old_query->collision_radius_sq;
    query->object_scale_sq = old_query->object_scale_sq;
    query->platform_position = old_query->position;
    if (CurTrackInfo != NULL && (CurTrackInfo->flags & TERRAIN_TRACK_FLAG_CONNECTED)) {
        TERRAIN_PLATFORM *platform = &CurTerr->platforms[CurTrackInfo->platform_index];
        NUMTX *matrix = static_cast<NUMTX *>(platform->scene_object);
        NUMTX *previous = &platform->previous_matrix;
        if (platform->flags & 1) {
            NuMtxInvRSS(&tertempmat, previous);
            tertempvec4.x = old_query->position.x - previous->m30;
            tertempvec4.y = (old_query->position.y - query->collision_radius) * query->object_scale - previous->m31;
            tertempvec4.z = old_query->position.z - previous->m32;
            tertempvec4.w = 1.0f;
            NuVec4MtxTransformVU0(&tertempvec4, &tertempvec4, &tertempmat);
            NuVec4MtxTransformVU0(&tertempvec4, &tertempvec4, matrix);
            query->platform_position.x = tertempvec4.x;
            query->platform_position.y = tertempvec4.y * query->inverse_object_scale + query->collision_radius;
            query->platform_position.z = tertempvec4.z;
        } else {
            query->platform_position.x += matrix->m30 - previous->m30;
            query->platform_position.y += (matrix->m31 - previous->m31) * old_query->inverse_object_scale;
            query->platform_position.z += matrix->m32 - previous->m32;
        }
    }
    query->position = query->platform_position;
    query->platform_normal = query->platform_position;
    query->movement.x = 0.0f;
    query->movement.y = TerConTol * -2.0f;
    query->movement.z = 0.0f;
    query->position.y += TerConTol;
    DerotateMovementVector();
    f32 saved_epsilon = TerI->separation_epsilon;
    TerI->separation_epsilon = 0.0f;
    HitTerrain();
    StorePlatImpact();
    TerI->separation_epsilon = saved_epsilon;
    TerrainImpactNorm();
    u8 flags[2];
    TerrainImpactPlatform(flags);
    if (TerI->hit_type != 0)
        ShadNorm = TerI->impact_normal;
    TerI->platform_position.y = TerI->position.y - TerI->collision_radius;
    TerI = old_query;
    if (query->hit_type == 0 ||
        fabsf((query->platform_normal.y - query->platform_position.y) - old_query->collision_radius) >
            TerConTol * 0.5f ||
        query->movement_normal.y < 0.707f) {
        NuScratchRelease();
        return count;
    }
    TERRAIN_GROUP *group = &CurTerr->groups[query->terrain_group_index];
    i32 platform_index = group->scene_index;
    if (movement->y > TerConTol * 0.5f) {
        TERRAIN_PLATFORM *platform = &CurTerr->platforms[platform_index];
        NUMTX *matrix = static_cast<NUMTX *>(platform->scene_object);
        if (movement->y > (TerConTol * 0.5f + matrix->m31) - platform->previous_matrix.m31) {
            NuScratchRelease();
            return count;
        }
    }
    if (query->movement_normal.y >= 0.707f && query->terrain_group_index >= 0 && group->chunk_type == 1)
        CurTerr->platforms[platform_index].flags |= 2;
    old_query->platform_position.x = query->platform_position.x;
    old_query->platform_position.y = query->platform_position.y + old_query->collision_radius;
    if (!(CurTerr->platforms[CurTrackInfo->platform_index].flags & 1))
        old_query->platform_position.y += 0.0025f;
    old_query->platform_position.z = query->platform_position.z;
    NUVEC direction = {old_query->platform_position.x - old_query->position.x,
                       old_query->platform_position.y - old_query->position.y,
                       old_query->platform_position.z - old_query->position.z};
    f32 distance = NuFsqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    direction.x = distance == 0.0f || direction.x == 0.0f ? 0.0f : direction.x / distance;
    direction.y = distance == 0.0f || direction.y == 0.0f ? 0.0f : direction.y / distance;
    direction.z = distance == 0.0f || direction.z == 0.0f ? 0.0f : direction.z / distance;
    NUVEC previous_position = TerI->position;
    i32 clear = TerrainPlatformMoveCheck(&TerI->platform_position, &TerI->platform_normal, platform_index, 1, 0);
    bool opposing_contact = false;
    if (TerI->hit_type != 0 && TerI->terrain_group_index >= 0) {
        TERRAIN_GROUP *hit_group = &CurTerr->groups[TerI->terrain_group_index];
        if (hit_group->chunk_type == 1)
            CurTrackInfo->platform_index = hit_group->scene_index;
        f32 dot = direction.x * TerI->platform_normal.x + direction.y * TerI->platform_normal.y +
                  direction.z * TerI->platform_normal.z;
        opposing_contact = dot < -0.7f;
    }
    --count;
    if (opposing_contact || clear != 0) {
        TerrPoly = query->surface;
        TerI->position = TerI->platform_position;
        if (movement->y < 0.0f) {
            FullDeflect(&ShadNorm, &TerI->movement, &TerI->movement);
            FullDeflect(&ShadNorm, movement, movement);
        } else {
            TerI->movement.y *= 0.75f;
        }
        TerI->hit_flags[1] = 1;
        PlatformConnect(reinterpret_cast<char *>(old_query->hit_flags), &old_query->movement, movement,
                        CurTerr->groups[query->terrain_group_index].scene_index);
    } else if (count > 0) {
        f32 travelled = (TerI->position.x - previous_position.x) * direction.x +
                        (TerI->position.y - previous_position.y) * direction.y +
                        (TerI->position.z - previous_position.z) * direction.z;
        if (distance > travelled && FullDeflectTest(&TerI->platform_normal, &direction, &TerI->platform_position)) {
            f32 dot = direction.x * TerI->platform_position.x + direction.y * TerI->platform_position.y +
                      direction.z * TerI->platform_position.z;
            if (dot > 0.0f) {
                f32 fraction = (distance - travelled) / dot;
                if (fraction < 0.8f) {
                    TerI->platform_position.x = TerI->platform_position.x * fraction + TerI->position.x;
                    TerI->platform_position.y = TerI->platform_position.y * fraction + TerI->position.y;
                    TerI->platform_position.z = TerI->platform_position.z * fraction + TerI->position.z;
                }
            }
        }
    }
    NuScratchRelease();
    TerI = old_query;
    return count;
}

i32 TerrainPlatformEmbedded(nuvec_s *movement_delta) {
    TerrainQuery_s *old_query = TerI;
    i32 platform_index = CurTerr->groups[old_query->terrain_group_index].scene_index;
    TERRAIN_PLATFORM *platform = &CurTerr->platforms[platform_index];
    NUMTX *matrix = static_cast<NUMTX *>(platform->scene_object);
    if (matrix == NULL)
        return 1;
    NUMTX *previous = &platform->previous_matrix;
    if (previous->m30 == matrix->m30 && previous->m31 == matrix->m31 && previous->m32 == matrix->m32) {
        if (!(platform->flags & 1))
            return 1;
        if (previous->m00 == matrix->m00 && previous->m01 == matrix->m01 && previous->m02 == matrix->m02 &&
            previous->m10 == matrix->m10 && previous->m11 == matrix->m11 && previous->m12 == matrix->m12 &&
            previous->m20 == matrix->m20 && previous->m21 == matrix->m21 && previous->m22 == matrix->m22)
            return 1;
    }
    i32 saved_sphere = curSphereter;
    curSphereter = 0;
    ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT2);
    TerrainQuery_s *query = static_cast<TerrainQuery_s *>(NuScratchAlloc32(sizeof(TerrainQuery_s)));
    TerI = query;
    void *scratch = NuScratchAlloc32(0xd0);
    NUVEC4 *vertices = reinterpret_cast<NUVEC4 *>((reinterpret_cast<uintptr_t>(scratch) + 31) & ~uintptr_t(15));
    query->collision_radius = old_query->collision_radius;
    query->object_index = old_query->object_index;
    query->inverse_object_scale_sq = old_query->inverse_object_scale_sq;
    query->hit_flags = old_query->hit_flags;
    query->inverse_collision_radius = old_query->inverse_collision_radius;
    query->scan_result = old_query->scan_result;
    query->separation_epsilon = old_query->separation_epsilon;
    query->compare_epsilon = old_query->compare_epsilon;
    query->collision_radius_sq = old_query->collision_radius_sq;
    query->object_scale_sq = old_query->object_scale_sq;
    query->inverse_object_scale = old_query->inverse_object_scale;
    query->radius = old_query->radius;
    query->object_scale = old_query->object_scale;
    query->movement.x = (previous->m30 - matrix->m30) * 1.73f;
    query->movement.y = ((previous->m31 - matrix->m31) * 1.73f) * query->inverse_object_scale;
    query->movement.z = (previous->m32 - matrix->m32) * 1.73f;
    if (platform->flags & 1) {
        NuMtxInvRSS(&tertempmat, previous);
        tertempvec4.x = old_query->position.x - previous->m30;
        tertempvec4.y = (old_query->position.y - query->collision_radius) * query->object_scale - previous->m31;
        tertempvec4.z = old_query->position.z - previous->m32;
        tertempvec4.w = 1.0f;
        NuVec4MtxTransformVU0(&tertempvec4, &tertempvec4, &tertempmat);
        NuVec4MtxTransformVU0(&tertempvec4, &tertempvec4, matrix);
        query->movement.x = (old_query->position.x - tertempvec4.x) * 1.3f;
        query->movement.y =
            (old_query->position.y - (tertempvec4.y * query->inverse_object_scale + query->collision_radius)) * 1.3f;
        query->movement.z = (old_query->position.z - tertempvec4.z) * 1.3f;
    }
    query->movement.x -= 0.01f * old_query->movement_normal.x;
    query->movement.y -= 0.01f * old_query->movement_normal.y;
    query->movement.z -= 0.01f * old_query->movement_normal.z;
    query->position.x = old_query->position.x - query->movement.x * 0.91f;
    query->position.y = old_query->position.y - query->movement.y * 0.91f;
    query->position.z = old_query->position.z - query->movement.z * 0.91f;

    TERRAIN_GROUP *group = &CurTerr->groups[old_query->terrain_group_index];
    TERRAIN_SHAPE **cursor = reinterpret_cast<TERRAIN_SHAPE **>(query->scan_list_storage + 4);
    i32 count = 0;
    i32 transformed_count = 0;
    TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group->data);
    while (batch->marker >= 0) {
        TERRAIN_SHAPE *shapes = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
        for (i32 i = 0; i < batch->shape_count; ++i) {
            TERRAIN_SHAPE *source = &shapes[i];
            if (source->material[1] != 0)
                continue;
            TERRAIN_SHAPE *surface = source;
            if ((platform->flags & 1) || query->object_scale != 1.0f) {
                surface = &ScaleTerrain[transformed_count++];
                memcpy(surface->material, source->material, 4);
                bool quad = source->normals[1].y < 65535.0f;
                if (platform->flags & 1) {
                    for (i32 v = 0; v < 3; ++v) {
                        vertices[v].x = source->vectors[v].x;
                        vertices[v].y = source->vectors[v].y;
                        vertices[v].z = source->vectors[v].z;
                        vertices[v].w = 0.0f;
                    }
                    NuVec4MtxTransformVU0x3(vertices, vertices, matrix);
                    if (quad) {
                        vertices[3].x = source->vectors[3].x;
                        vertices[3].y = source->vectors[3].y;
                        vertices[3].z = source->vectors[3].z;
                        vertices[3].w = 0.0f;
                        NuVec4MtxTransformVU0(&vertices[3], &vertices[3], matrix);
                    } else {
                        vertices[3] = vertices[2];
                    }
                }
                for (i32 v = 0; v < (quad ? 4 : 3); ++v) {
                    if (platform->flags & 1) {
                        surface->vectors[v].x = vertices[v].x;
                        surface->vectors[v].y = vertices[v].y;
                        surface->vectors[v].z = vertices[v].z;
                    } else {
                        surface->vectors[v] = source->vectors[v];
                    }
                    if (query->object_scale != 1.0f)
                        surface->vectors[v].y =
                            (surface->vectors[v].y + group->origin.y) * query->inverse_object_scale - group->origin.y;
                }
                if (!quad)
                    surface->normals[1].y = 65536.0f;
                for (i32 n = quad ? 1 : 0; n >= 0; --n) {
                    if (platform->flags & 1) {
                        i32 origin = n == 0 ? 0 : 3;
                        i32 first = n == 0 ? 2 : 1;
                        i32 second = n == 0 ? 1 : 2;
                        NUVEC a = {surface->vectors[first].x - surface->vectors[origin].x,
                                   surface->vectors[first].y - surface->vectors[origin].y,
                                   surface->vectors[first].z - surface->vectors[origin].z};
                        NUVEC b = {surface->vectors[second].x - surface->vectors[origin].x,
                                   surface->vectors[second].y - surface->vectors[origin].y,
                                   surface->vectors[second].z - surface->vectors[origin].z};
                        surface->normals[n] = TerCrossProduct(&a, &b);
                        NUVEC *normal = &surface->normals[n];
                        f32 length = NuFsqrt(normal->x * normal->x + normal->y * normal->y + normal->z * normal->z);
                        f32 inverse = length == 0.0f ? 0.0f : 1.0f / length;
                        normal->x *= inverse;
                        normal->y *= inverse;
                        normal->z *= inverse;
                    } else {
                        NUVEC *normal = &source->normals[n];
                        f32 length = NuFsqrt(normal->x * normal->x + (normal->y * normal->y) * query->object_scale_sq +
                                             normal->z * normal->z);
                        f32 inverse = length == 0.0f ? 0.0f : 1.0f / length;
                        surface->normals[n].x = normal->x * inverse;
                        surface->normals[n].y = (normal->y * query->object_scale) * inverse;
                        surface->normals[n].z = normal->z * inverse;
                    }
                }
            }
            *cursor++ = surface;
            ++count;
        }
        batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(shapes + batch->shape_count);
    }
    i16 *header = reinterpret_cast<i16 *>(query->scan_list_storage);
    if (count != 0) {
        header[0] = count;
        header[1] = old_query->terrain_group_index;
        header = reinterpret_cast<i16 *>(cursor);
    }
    header[0] = 0;
    header[1] = 0;
    u8 flags[2] = {0, 0};
    DerotateMovementVector();
    HitTerrain();
    TerrainImpactNorm();
    if (TerrainImpactPlatform(flags) != 0) {
        NuScratchRelease();
        NuScratchRelease();
        ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT1);
        TerI = old_query;
        curSphereter = saved_sphere;
        return 1;
    }
    query->start_position = query->position;
    TerI->movement.x = 0.0f;
    TerI->movement.y = -0.05f;
    TerI->movement.z = 0.0f;
    TerI->position.y = query->start_position.y + 0.025f;
    DerotateMovementVector();
    TerrOverRideScan = reinterpret_cast<tertype **>(old_query->scan_list_storage);
    HitTerrain();
    TerI->position.y = query->start_position.y;
    flags[0] = 0;
    if (TerI->hit_type >= 1 && TerI->hit_type <= 15) {
        query->start_position.y = ((TerI->movement.y * TerI->hit_time + query->start_position.y) + 0.001f) + 0.025f;
        TerI = old_query;
        if (!(query->start_position.y >= 2000000.0f)) {
            f32 delta = query->start_position.y - query->position.y;
            if (!(delta < -0.008f) &&
                (delta < 0.01f || query->movement.x * query->movement.x + query->movement.z * query->movement.z >
                                      (delta - 0.0025f) * (delta - 0.0025f))) {
                query->position.y += delta;
                flags[0] = 1;
            }
        } else {
            query->start_position.y = query->position.y;
        }
    } else {
        TerI = old_query;
        query->start_position.y = query->position.y;
    }
    if (!TerrainPlatformMoveCheck(&query->position, NULL, platform_index, 1, 1))
        flags[0] = 0;
    TerI->position = query->position;
    if (flags[0] != 0 && CurTerr->groups[castnum].chunk_type == 1) {
        TerI->hit_flags[1] = 1;
        PlatformConnect(reinterpret_cast<char *>(old_query->hit_flags), &old_query->movement, movement_delta,
                        CurTerr->groups[castnum].scene_index);
    } else if (CurTrackInfo != NULL) {
        CurTrackInfo->flags &= ~TERRAIN_TRACK_FLAG_CONNECTED;
    }
    NuScratchRelease();
    NuScratchRelease();
    ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT1);
    curSphereter = saved_sphere;
    TerI = old_query;
    return 0;
}
i32 TerrainPlatformMoveCheck(nuvec_s *position, nuvec_s *normal, i32 platform_index, i32 resolve_impact,
                             i32 keep_embedded_disabled) {
    i32 group_index = CurTerr->platforms[platform_index].terrain_group_index;
    u8 *entry = static_cast<u8 *>(TerI->scan_list);
    while (*reinterpret_cast<i16 *>(entry) > 0) {
        i16 *header = reinterpret_cast<i16 *>(entry);
        if (header[1] == group_index)
            header[0] = -header[0];
        i32 count = header[0];
        if (count < 0)
            count = -count;
        entry += 4 + count * 4;
    }
    if (position->y > TerI->position.y && TerI->position.y + 0.01f > position->y)
        TerI->position.y = position->y;
    NUVEC saved_movement = TerI->movement;
    TERRAIN_SHAPE *saved_surface = TerI->surface;
    i16 saved_group = TerI->terrain_group_index;
    i16 saved_scan_group = TerI->scan_group_index;
    f32 saved_time = TerI->hit_time;
    f32 saved_unclamped_time = TerI->unclamped_hit_time;
    NUVEC saved_movement_normal = TerI->movement_normal;
    NUVEC saved_impact_normal = TerI->impact_normal;
    u8 saved_hit_type = TerI->hit_type;
    TerI->movement.x = position->x - TerI->position.x;
    TerI->movement.y = position->y - TerI->position.y;
    TerI->movement.z = position->z - TerI->position.z;
    DerotateMovementVector();
    HitTerrain();
    if (resolve_impact != 0 && TerI->hit_type != 0) {
        TerrainImpactNorm();
        TerrainImpactPlatform(TerI->hit_flags);
        if (normal != NULL)
            *normal = TerI->movement_normal;
        *position = TerI->position;
    }
    u8 hit_type = TerI->hit_type;
    TerI->movement = saved_movement;
    TerI->surface = saved_surface;
    TerI->terrain_group_index = saved_group;
    TerI->scan_group_index = saved_scan_group;
    TerI->hit_time = saved_time;
    TerI->unclamped_hit_time = saved_unclamped_time;
    TerI->movement_normal = saved_movement_normal;
    TerI->impact_normal = saved_impact_normal;
    if (hit_type <= 15 || keep_embedded_disabled == 0) {
        entry = static_cast<u8 *>(TerI->scan_list);
        while (*reinterpret_cast<i16 *>(entry) != 0) {
            i16 *header = reinterpret_cast<i16 *>(entry);
            i32 count = header[0];
            if (count < 0)
                count = -count;
            if (header[1] == group_index)
                header[0] = count;
            entry += 4 + count * 4;
        }
    }
    TerI->hit_type = saved_hit_type;
    return hit_type == 0;
}
// The retained debug renderer reads the older 100-byte terrain record.
struct TERRAIN_DEBUG_RECORD {
    u8 unknown_00[0x18];
    NUVEC vertices[4];
    NUVEC normals[2];
    u8 unknown_60[4];
};
DECOMP_ASSERT(sizeof(TERRAIN_DEBUG_RECORD) == 100, "Terrain debug record ABI");

void TerrDrawSitu(tertype *terrain, terrsitu_s *situation) {
    TERRAIN_DEBUG_RECORD *record = reinterpret_cast<TERRAIN_DEBUG_RECORD *>(terrain);
    NUVEC *origin = reinterpret_cast<NUVEC *>(situation);
    if (record->normals[1].y > 65535.0f) {
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            NuRndrLine3dDbg(
                origin->x + record->vertices[0].x, origin->y + record->vertices[0].y, origin->z + record->vertices[0].z,
                origin->x + record->vertices[1].x + (record->vertices[2].x - record->vertices[1].x) * t,
                origin->y + record->vertices[1].y + (record->vertices[2].y - record->vertices[1].y) * t,
                origin->z + record->vertices[1].z + (record->vertices[2].z - record->vertices[1].z) * t, 0x003f7f80);
        }
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            NuRndrLine3dDbg(
                origin->x + record->vertices[1].x, origin->y + record->vertices[1].y, origin->z + record->vertices[1].z,
                origin->x + record->vertices[0].x + (record->vertices[2].x - record->vertices[0].x) * t,
                origin->y + record->vertices[0].y + (record->vertices[2].y - record->vertices[0].y) * t,
                origin->z + record->vertices[0].z + (record->vertices[2].z - record->vertices[0].z) * t, 0x003f7f80);
        }
    } else {
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            NuRndrLine3dDbg(origin->x + record->vertices[0].x + (record->vertices[1].x - record->vertices[0].x) * t,
                            origin->y + record->vertices[0].y + (record->vertices[1].y - record->vertices[0].y) * t,
                            origin->z + record->vertices[0].z + (record->vertices[1].z - record->vertices[0].z) * t,
                            origin->x + record->vertices[2].x + (record->vertices[3].x - record->vertices[2].x) * t,
                            origin->y + record->vertices[2].y + (record->vertices[3].y - record->vertices[2].y) * t,
                            origin->z + record->vertices[2].z + (record->vertices[3].z - record->vertices[2].z) * t,
                            0xffff7f80);
        }
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            NuRndrLine3dDbg(origin->x + record->vertices[0].x + (record->vertices[2].x - record->vertices[0].x) * t,
                            origin->y + record->vertices[0].y + (record->vertices[2].y - record->vertices[0].y) * t,
                            origin->z + record->vertices[0].z + (record->vertices[2].z - record->vertices[0].z) * t,
                            origin->x + record->vertices[1].x + (record->vertices[3].x - record->vertices[1].x) * t,
                            origin->y + record->vertices[1].y + (record->vertices[3].y - record->vertices[1].y) * t,
                            origin->z + record->vertices[1].z + (record->vertices[3].z - record->vertices[1].z) * t,
                            0xffff7f80);
        }
    }
}

void TerrDrawPlat(tertype *terrain, i16 index) {
    TERRAIN_DEBUG_RECORD *record = reinterpret_cast<TERRAIN_DEBUG_RECORD *>(terrain);
    TERRAIN_PLATFORM *platform = &CurTerr->platforms[CurTerr->groups[index].scene_index];
    if (platform->scene_transform != NULL) {
        if (platform->flags & TERRAIN_PLATFORM_FLAG_DISPLAY_LIST_BACKED) {
            if (!(*static_cast<u8 *>(platform->scene_transform) & 2))
                return;
        } else if (!(*static_cast<u8 *>(platform->scene_transform) & 1))
            return;
    }
    NUVEC4 points[5];
    points[3].x = record->vertices[3].x;
    points[3].y = record->vertices[3].y;
    points[3].z = record->vertices[3].z;
    points[3].w = 0.0f;
    for (i32 i = 0; i < 3; ++i) {
        points[i].x = record->vertices[i].x;
        points[i].y = record->vertices[i].y;
        points[i].z = record->vertices[i].z;
        points[i].w = 0.0f;
    }
    NUVEC4 &normal = points[4];
    normal.x = record->normals[0].x * 0.4f;
    normal.y = record->normals[0].y * 0.4f;
    normal.z = record->normals[0].z * 0.4f;
    normal.w = 0.0f;
    i32 colour = static_cast<i8>(record->unknown_60[2]) < 0 ? 0x0000ffff : 0x00ff0000;
    if (platform->flags & TERRAIN_PLATFORM_FLAG_ROTATING) {
        NuVec4MtxTransformVU0(&points[0], &points[0], static_cast<NUMTX *>(platform->scene_object));
        NuVec4MtxTransformVU0(
            &points[1], &points[1],
            static_cast<NUMTX *>(CurTerr->platforms[CurTerr->groups[index].scene_index].scene_object));
        NuVec4MtxTransformVU0(
            &points[2], &points[2],
            static_cast<NUMTX *>(CurTerr->platforms[CurTerr->groups[index].scene_index].scene_object));
        NuVec4MtxTransformVU0(
            &points[3], &points[3],
            static_cast<NUMTX *>(CurTerr->platforms[CurTerr->groups[index].scene_index].scene_object));
        NuVec4MtxTransformVU0(
            &normal, &normal,
            static_cast<NUMTX *>(CurTerr->platforms[CurTerr->groups[index].scene_index].scene_object));
    }
    NUVEC *origin;
    if (record->normals[1].y > 65535.0f) {
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            origin = &CurTerr->groups[index].origin;
            NuRndrLine3dDbg(origin->x + points[0].x, origin->y + points[0].y, origin->z + points[0].z,
                            origin->x + points[1].x + (points[2].x - points[1].x) * t,
                            origin->y + points[1].y + (points[2].y - points[1].y) * t,
                            origin->z + points[1].z + (points[2].z - points[1].z) * t, colour);
        }
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            origin = &CurTerr->groups[index].origin;
            NuRndrLine3dDbg(origin->x + points[1].x, origin->y + points[1].y, origin->z + points[1].z,
                            origin->x + points[0].x + (points[2].x - points[0].x) * t,
                            origin->y + points[0].y + (points[2].y - points[0].y) * t,
                            origin->z + points[0].z + (points[2].z - points[0].z) * t, colour);
        }
    } else {
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            origin = &CurTerr->groups[index].origin;
            NuRndrLine3dDbg(origin->x + points[0].x + (points[1].x - points[0].x) * t,
                            origin->y + points[0].y + (points[1].y - points[0].y) * t,
                            origin->z + points[0].z + (points[1].z - points[0].z) * t,
                            origin->x + points[2].x + (points[3].x - points[2].x) * t,
                            origin->y + points[2].y + (points[3].y - points[2].y) * t,
                            origin->z + points[2].z + (points[3].z - points[2].z) * t, colour);
        }
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            origin = &CurTerr->groups[index].origin;
            NuRndrLine3dDbg(origin->x + points[0].x + (points[2].x - points[0].x) * t,
                            origin->y + points[0].y + (points[2].y - points[0].y) * t,
                            origin->z + points[0].z + (points[2].z - points[0].z) * t,
                            origin->x + points[1].x + (points[3].x - points[1].x) * t,
                            origin->y + points[1].y + (points[3].y - points[1].y) * t,
                            origin->z + points[1].z + (points[3].z - points[1].z) * t, colour);
        }
    }
    origin = &CurTerr->groups[index].origin;
    f32 z = origin->z + points[0].z;
    f32 y = origin->y + points[0].y;
    f32 x = origin->x + points[0].x;
    colour = static_cast<i8>(index & 0xf0) + static_cast<i8>(index * 16) * 256;
    NuRndrLine3dDbg(x, y, z, x + normal.x, y + normal.y, z + normal.z, colour);
}

void TerrDraw(tertype *terrain, i16 index) {
    TERRAIN_DEBUG_RECORD *record = reinterpret_cast<TERRAIN_DEBUG_RECORD *>(terrain);
    i32 colour = (index & 0x80) + (static_cast<u8>(index * 64) << 8);
    if (!(record->normals[1].y > 65535.0f)) {
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            NUVEC *origin = &CurTerr->groups[index].origin;
            NuRndrLine3dDbg(origin->x + record->vertices[0].x + (record->vertices[1].x - record->vertices[0].x) * t,
                            origin->y + record->vertices[0].y + (record->vertices[1].y - record->vertices[0].y) * t,
                            origin->z + record->vertices[0].z + (record->vertices[1].z - record->vertices[0].z) * t,
                            origin->x + record->vertices[2].x + (record->vertices[3].x - record->vertices[2].x) * t,
                            origin->y + record->vertices[2].y + (record->vertices[3].y - record->vertices[2].y) * t,
                            origin->z + record->vertices[2].z + (record->vertices[3].z - record->vertices[2].z) * t,
                            colour);
        }
        for (i32 i = 0; i < 2; ++i) {
            f32 t = static_cast<f32>(i);
            NUVEC *origin = &CurTerr->groups[index].origin;
            NuRndrLine3dDbg(origin->x + record->vertices[0].x + (record->vertices[2].x - record->vertices[0].x) * t,
                            origin->y + record->vertices[0].y + (record->vertices[2].y - record->vertices[0].y) * t,
                            origin->z + record->vertices[0].z + (record->vertices[2].z - record->vertices[0].z) * t,
                            origin->x + record->vertices[1].x + (record->vertices[3].x - record->vertices[1].x) * t,
                            origin->y + record->vertices[1].y + (record->vertices[3].y - record->vertices[1].y) * t,
                            origin->z + record->vertices[1].z + (record->vertices[3].z - record->vertices[1].z) * t,
                            colour);
        }
    }
    NUVEC *origin = &CurTerr->groups[index].origin;
    f32 z = origin->z + record->vertices[0].z;
    f32 y = origin->y + record->vertices[0].y;
    f32 x = origin->x + record->vertices[0].x;
    NuRndrLine3dDbg(x, y, z, x + record->normals[0].x * 0.4f, y + record->normals[0].y * 0.4f,
                    z + record->normals[0].z * 0.4f, colour);
    if (record->normals[1].y < 65536.0f) {
        origin = &CurTerr->groups[index].origin;
        z = origin->z + record->vertices[3].z;
        y = origin->y + record->vertices[3].y;
        x = origin->x + record->vertices[3].x;
        NuRndrLine3dDbg(x, y, z, x + record->normals[1].x * 0.4f, y + record->normals[1].y * 0.4f,
                        z + record->normals[1].z * 0.4f, colour);
    }
}

void DrawMSitu(i32 index) {
    TERRAIN_GROUP *group = &CurTerr->groups[index];
    if ((u32)group->chunk_type <= TERRAIN_CHUNK_GROUP_SECONDARY && group->data != NULL) {
        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group->data);
        while (batch->marker >= 0) {
            u8 *entry = reinterpret_cast<u8 *>(batch + 1);
            i32 count = batch->shape_count;
            for (i32 remaining = count; remaining > 0; --remaining) {
                TerrDraw(reinterpret_cast<tertype *>(entry), static_cast<i16>(index));
                // This debug walker uses the original 100-byte record stride.
                entry += 100;
            }
            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(entry);
        }
    }
}

extern TERRSET *CurTerr;
extern TerrainQuery_s *TerI;
extern NUVEC ShadNorm;
extern TERRAIN_SHAPE *ShadPoly;
extern i16 castnum;
extern NUVEC ShadRoofNorm;
extern f32 ShadRoofY;
extern TERRAIN_SHAPE *ShadRoofPoly;
extern i16 castroofnum;
extern i16 shadroofhit;
extern i16 shadhit;
extern NUVEC EShadNorm;
extern NUVEC EShadRoofNorm;
extern f32 EShadY;
extern f32 EShadRoofY;
extern TERRAIN_SHAPE *EShadPoly;
extern TERRAIN_SHAPE *EShadRoofPoly;
extern i16 ecastnum;
extern i16 ecastroofnum;
extern i16 eshadhit;
extern i16 eshadroofhit;
i16 InsideLineF(f32, f32, f32, f32, f32, f32);

namespace {

    const f32 NO_TERRAIN_HEIGHT = 2000000.0f;

    struct ShadowSurfaceCandidate {
        f32 height;
        TERRAIN_SHAPE *surface;
        NUVEC normal;
        i16 terrain_group;
    };

    static void ConsiderShadowTriangle(const NUVEC &local_position, const TERRAIN_GROUP &group, TERRAIN_SHAPE *surface,
                                       const NUVEC &origin, const NUVEC &edge_a, const NUVEC &edge_b,
                                       const NUVEC &normal, f32 winding_y, f32 query_height, i16 terrain_group,
                                       ShadowSurfaceCandidate *floor_candidate,
                                       ShadowSurfaceCandidate *above_candidate) {
        const f32 point_x = local_position.x, point_z = local_position.z;
        if (winding_y > 0.0f) {
            if (InsideLineF(point_x, point_z, edge_a.x, edge_a.z, origin.x, origin.z) == 0 ||
                InsideLineF(point_x, point_z, edge_b.x, edge_b.z, edge_a.x, edge_a.z) == 0 ||
                InsideLineF(point_x, point_z, origin.x, origin.z, edge_b.x, edge_b.z) == 0)
                return;
        } else if (winding_y < 0.0f) {
            if (InsideLineF(point_x, point_z, origin.x, origin.z, edge_a.x, edge_a.z) == 0 ||
                InsideLineF(point_x, point_z, edge_a.x, edge_a.z, edge_b.x, edge_b.z) == 0 ||
                InsideLineF(point_x, point_z, edge_b.x, edge_b.z, origin.x, origin.z) == 0)
                return;
        } else
            return;

        const f32 plane_height =
            (origin.y + group.origin.y) +
            ((local_position.x - origin.x) * normal.x + (local_position.z - origin.z) * normal.z) / -normal.y;
        if (plane_height <= query_height) {
            if (winding_y > 0.0f && plane_height > floor_candidate->height) {
                floor_candidate->height = plane_height;
                floor_candidate->surface = surface;
                floor_candidate->normal = normal;
                floor_candidate->terrain_group = terrain_group;
            }
        } else if (plane_height < above_candidate->height ||
                   (plane_height == above_candidate->height && normal.y < above_candidate->normal.y)) {
            // The target keeps the nearest ordinary surface above the cast
            // origin as well as the conventional floor below it.  NewCast's
            // final range check decides whether this surface is reachable.
            above_candidate->height = plane_height;
            above_candidate->surface = surface;
            above_candidate->normal = normal;
            above_candidate->terrain_group = terrain_group;
        }
    }

} // namespace

f32 NewCast(nuvec_s *position, f32 height_above, f32 roof_range) {
    ShadowSurfaceCandidate floor_candidate = {
        -NO_TERRAIN_HEIGHT,
        NULL,
        {0.0f, 1.0f, 0.0f},
        -1,
    };
    ShadowSurfaceCandidate above_candidate = {
        NO_TERRAIN_HEIGHT,
        NULL,
        {0.0f, 1.0f, 0.0f},
        -1,
    };
    ShadowSurfaceCandidate extended_floor = floor_candidate;
    ShadowSurfaceCandidate extended_above = above_candidate;
    castnum = -1;
    ecastnum = -1;

    u8 *scan = TerI->scan_list_storage;
    i16 shape_count = *reinterpret_cast<i16 *>(scan);
    while (shape_count > 0) {
        const i16 group_index = *reinterpret_cast<i16 *>(scan + sizeof(i16));
        TERRAIN_GROUP &group = CurTerr->groups[group_index];
        TERRAIN_SHAPE **surfaces = reinterpret_cast<TERRAIN_SHAPE **>(scan) + 1;
        const NUVEC local_position = {
            position->x - group.origin.x,
            position->y - group.origin.y,
            position->z - group.origin.z,
        };

        for (i32 shape_index = 0; shape_index < shape_count; ++shape_index) {
            TERRAIN_SHAPE *surface = surfaces[shape_index];
            bool extended = surface->material[1] != 0;
            ShadowSurfaceCandidate *floor = extended ? &extended_floor : &floor_candidate;
            ShadowSurfaceCandidate *above = extended ? &extended_above : &above_candidate;
            f32 winding_y = surface->normals[0].y;
            if (!(winding_y > 0.0f || winding_y < 0.0f))
                continue;
            bool second_triangle = false;
            if (!(surface->normals[1].y > 65535.0f)) {
                const NUVEC &a = surface->vectors[winding_y > 0.0f ? 2 : 1];
                const NUVEC &b = surface->vectors[winding_y > 0.0f ? 1 : 2];
                second_triangle = InsideLineF(local_position.x, local_position.z, a.x, a.z, b.x, b.z) == 0;
            }
            if (second_triangle) {
                ConsiderShadowTriangle(local_position, group, surface, surface->vectors[3], surface->vectors[2],
                                       surface->vectors[1], surface->normals[1], winding_y, position->y, group_index,
                                       floor, above);
            } else {
                ConsiderShadowTriangle(local_position, group, surface, surface->vectors[0], surface->vectors[1],
                                       surface->vectors[2], surface->normals[0], winding_y, position->y, group_index,
                                       floor, above);
            }
        }

        scan = reinterpret_cast<u8 *>(surfaces + shape_count);
        shape_count = *reinterpret_cast<i16 *>(scan);
    }

    castnum = floor_candidate.terrain_group;
    ecastnum = extended_floor.terrain_group;
    bool ordinary_roof = above_candidate.height < NO_TERRAIN_HEIGHT && above_candidate.normal.y < 0.0f &&
                         position->y + roof_range > above_candidate.height;
    bool extended_blocked = ordinary_roof && extended_above.height > above_candidate.height;
    EShadRoofY = NO_TERRAIN_HEIGHT;
    EShadRoofPoly = NULL;
    if (extended_above.height < NO_TERRAIN_HEIGHT && extended_above.normal.y < 0.0f &&
        position->y + roof_range > extended_above.height && !extended_blocked) {
        EShadRoofY = extended_above.height;
        EShadRoofPoly = extended_above.surface;
        EShadRoofNorm = extended_above.normal;
        eshadroofhit = 1;
        ecastroofnum = extended_above.terrain_group;
    }
    const ShadowSurfaceCandidate *extended_result = NULL;
    if (extended_above.height < NO_TERRAIN_HEIGHT && extended_above.normal.y > 0.0f &&
        position->y + roof_range > extended_above.height) {
        if (!extended_blocked) {
            extended_result = &extended_above;
            eshadhit = 1;
            ecastnum = extended_above.terrain_group;
        }
    } else if (extended_floor.height > -NO_TERRAIN_HEIGHT &&
               !(floor_candidate.height > -NO_TERRAIN_HEIGHT && floor_candidate.height > extended_floor.height)) {
        extended_result = &extended_floor;
        eshadhit = 2;
    }
    if (extended_result != NULL) {
        EShadY = extended_result->height;
        EShadPoly = extended_result->surface;
        EShadNorm = extended_result->normal;
    } else {
        EShadY = NO_TERRAIN_HEIGHT;
        EShadPoly = NULL;
        EShadNorm.y = 1.0f;
        eshadhit = 3;
    }
    ShadRoofY = NO_TERRAIN_HEIGHT;
    ShadRoofPoly = NULL;
    if (ordinary_roof) {
        ShadRoofY = above_candidate.height;
        ShadRoofPoly = above_candidate.surface;
        ShadRoofNorm = above_candidate.normal;
        shadroofhit = 1;
        castroofnum = above_candidate.terrain_group;
    }
    const ShadowSurfaceCandidate *selected_candidate = NULL;
    if (above_candidate.surface != NULL && above_candidate.normal.y > 0.0f &&
        above_candidate.height < position->y + height_above) {
        selected_candidate = &above_candidate;
        shadhit = 1;
    } else if (floor_candidate.surface != NULL) {
        selected_candidate = &floor_candidate;
        shadhit = 2;
    }

    if (selected_candidate == NULL) {
        position->y = NO_TERRAIN_HEIGHT;
        ShadPoly = NULL;
        ShadNorm.y = 1.0f;
        shadhit = 3;
        castnum = -1;
        return 0.0f;
    }

    position->y = selected_candidate->height;
    ShadNorm = selected_candidate->normal;
    ShadPoly = selected_candidate->surface;
    castnum = selected_candidate->terrain_group;
    return 0.0f;
}

extern "C" void TerrainWallAng(f32 slope) {
    wallover = slope;
}

extern "C" void TerrainWallSideSlide(NUVEC *movement, void *id, f32 speed, f32 upward_scale, NUANG angle) {
    if (CurTerr == NULL)
        return;
    CurTrackInfo = ScanTerrId(id);
    if (CurTrackInfo == NULL || CurTrackInfo->wall_contact_state == 0)
        return;
    if (speed != 0.0f) {
        NUVEC slide = {0.0f, 0.0f, speed};
        NuVecRotateY(&slide, &slide, angle);
        FullDeflect(&CurTrackInfo->impact_normal, &slide, &slide);
        slide.x += (speed * CurTrackInfo->impact_normal.x) * 0.4f;
        slide.z += (speed * CurTrackInfo->impact_normal.z) * 0.4f;
        movement->z += slide.z;
        movement->x += slide.x;
    }
    if (movement->y > 0.0f)
        movement->y *= upward_scale;
}

i32 HitTerrPoly(tertype *surface, i32 group_index);

extern "C" i32 TerrainTrackBack(NUVEC *position, NUVEC *direction, f32 radius, f32 backoff, NUVEC *result) {
    if (CurTerr == NULL || TerrPolyObj == -1)
        return 0;
    TerI = static_cast<TerrainQuery_s *>(NuScratchAlloc32(sizeof(TerrainQuery_s)));
    TerI->object_scale = 1.0f;
    TerI->object_scale_sq = 1.0f;
    TerI->inverse_object_scale = 1.0f;
    TerI->inverse_object_scale_sq = 1.0f;
    TerI->collision_radius = radius;
    TerI->inverse_collision_radius = radius == 0.0f ? 0.0f : 1.0f / radius;
    TerI->collision_radius_sq = radius * radius;
    TerI->position.x = direction->x * (radius + 0.5f) + position->x;
    TerI->position.y = direction->y * (radius + 0.5f) + position->y;
    TerI->position.z = direction->z * (radius + 0.5f) + position->z;
    TerI->movement.x = -direction->x * (radius + 0.8f);
    TerI->movement.y = -direction->y * (radius + 0.8f);
    TerI->movement.z = -direction->z * (radius + 0.8f);
    TerI->object_index = -1;
    TerI->hit_flags = NULL;
    DerotateMovementVector();
    HitTerrPoly(TerrPoly, TerrPolyObj);
    switch (TerI->hit_type) {
        case 0:
        case 17:
        case 18:
        case 19:
        case 20:
            *result = TerI->position;
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            TerI->hit_time -= backoff;
            if (TerI->hit_time < 0.0f)
                TerI->hit_time = 0.0f;
            result->x = TerI->movement.x * TerI->hit_time + TerI->position.x;
            result->y = TerI->movement.y * TerI->hit_time + TerI->position.y;
            result->z = TerI->movement.z * TerI->hit_time + TerI->position.z;
            break;
    }
    NuScratchRelease();
    return TerI->hit_type;
}

extern "C" i32 TerrainInfo() {
    if (TerrPoly != NULL)
        return TerrPoly->material[0];
    if (TerrWallInfo != 0)
        return TerrWallTab[0];
    return -1;
}

extern "C" i32 TerrainInfoExtra() {
    if (TerrPoly != NULL)
        return TerrPoly->material[1];
    if (TerrWallInfo != 0)
        return TerrWallTab[1];
    return -1;
}

extern "C" i32 TerrainIntensityInfo() {
    if (TerrPoly != NULL)
        return static_cast<i32>(TerrPoly->normal_flags) - 8;
    if (TerrWallInfo != 0)
        return static_cast<i32>(TerrWallTab[3]) - 8;
    return -1;
}

extern "C" void TerrTempMemory(void **buffer) {
    u8 *cursor = static_cast<u8 *>(*buffer);
    if (ScaleTerrainT1 == NULL)
        ScaleTerrainT1 = cursor;
    cursor += 0xc800;
    *buffer = cursor;
    if (ScaleTerrainT2 == NULL)
        ScaleTerrainT2 = cursor;
    cursor += 0xc800;
    *buffer = cursor;
    if (TempScanStack == NULL)
        TempScanStack = cursor;
    cursor += 0x2000;
    *buffer = cursor;
    if (WallSplList == NULL)
        WallSplList = reinterpret_cast<TERRAIN_WALL_POINT *>(cursor);
    cursor += 0x600;
    *buffer = cursor;
}

// Original 0x378fb0: the optimized rotating scan shares the terrain engine state.
extern TERRSET *CurTerr;
extern TerrainQuery_s *TerI;
extern i16 NuTerrPlatsOff;
extern TERRAIN_SHAPE *ScaleTerrain;
extern "C" void *NuScratchAlloc32(i32);
extern "C" void NuScratchRelease();
NUVEC TerCrossProduct(NUVEC *, NUVEC *);

void TerrainSkinAllocate(terrsitu_s *terrain_group);

namespace {

    // Target NewScanRot 0x378fb0 uses 0.1f on both horizontal axes when
    // selecting the terrain shapes that can contain the cast point.
    const f32 SHADOW_SCAN_HALF_EXTENT = 0.1f;

    struct ShadowScanWriter {
        u8 *group_header;
        TERRAIN_SHAPE **cursor;
        u8 *limit;
        i32 shape_count;
    };

    static bool ShadowBoundsOverlap(f32 min_x, f32 min_z, f32 max_x, f32 max_z, const NUVEC &minimum,
                                    const NUVEC &maximum) {
        return max_x >= minimum.x && maximum.x >= min_x && max_z >= minimum.z && maximum.z > min_z;
    }

    static void ShadowFinishGroup(ShadowScanWriter *writer, i32 group_index) {
        if (writer->shape_count == 0) {
            return;
        }

        i16 *header = reinterpret_cast<i16 *>(writer->group_header);
        header[0] = static_cast<i16>(writer->shape_count);
        header[1] = static_cast<i16>(group_index);
        writer->group_header = reinterpret_cast<u8 *>(writer->cursor);
        writer->cursor = reinterpret_cast<TERRAIN_SHAPE **>(writer->group_header + sizeof(TERRAIN_SHAPE *));
        writer->shape_count = 0;
    }

    static void ShadowScanGroup(i32 group_index, f32 world_min_x, f32 world_min_z, f32 world_max_x, f32 world_max_z,
                                i32 terrain_mask, bool bounds_are_local, ShadowScanWriter *writer) {
        TERRAIN_GROUP &group = CurTerr->groups[group_index];
        if (group.chunk_type == -1) {
            return;
        }

        f32 local_min_x = world_min_x - group.origin.x;
        f32 local_max_x = world_max_x - group.origin.x;
        f32 local_min_z = world_min_z - group.origin.z;
        f32 local_max_z = world_max_z - group.origin.z;
        if (bounds_are_local) {
            if (!ShadowBoundsOverlap(local_min_x, local_min_z, local_max_x, local_max_z, group.bounds_min,
                                     group.bounds_max)) {
                return;
            }
        } else if (!ShadowBoundsOverlap(world_min_x, world_min_z, world_max_x, world_max_z, group.bounds_min,
                                        group.bounds_max)) {
            return;
        }

        if (group.scene_index < 0) {
            TerrainSkinAllocate(reinterpret_cast<terrsitu_s *>(&group));
        }

        TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
        while (batch->marker >= 0) {
            TERRAIN_SHAPE *shapes = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
            if (local_max_x >= batch->min_x && batch->max_x > local_min_x && local_max_z >= batch->min_z &&
                batch->max_z > local_min_z) {
                for (i32 shape_index = 0; shape_index < batch->shape_count; ++shape_index) {
                    TERRAIN_SHAPE *shape = &shapes[shape_index];
                    if (local_max_x < shape->min_x || shape->max_x <= local_min_x || local_max_z < shape->min_z ||
                        shape->max_z <= local_min_z) {
                        continue;
                    }
                    if (shape->material[1] != 0 && (shape->material[1] & terrain_mask) == 0) {
                        continue;
                    }
                    if (reinterpret_cast<u8 *>(writer->cursor + 1) > writer->limit) {
                        continue;
                    }
                    *writer->cursor++ = shape;
                    ++writer->shape_count;
                }
            }
            batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(shapes + batch->shape_count);
        }
        ShadowFinishGroup(writer, group_index);
    }

} // namespace

extern "C" void NewRaySetDisablePalt(i32 disabled) {
    TerrPlatDis = disabled;
}

extern "C" void NewScanInit(void) {
    TempStackPtr = TempScanStack;
    TerrPlatDis = -1;
}

void NewScanRot(nuvec_s *position, i32 terrain_mask) {
    void *scratch = NuScratchAlloc32(0xd0);
    i32 cache_index = 0;
    i32 oldest_age = CurTerr->index_levels[0].cache_age;
    bool cache_hit = false;
    for (i32 i = 0; i < 16; ++i) {
        TERRAIN_INDEX_LEVEL &cache = CurTerr->index_levels[i];
        i32 age = cache.cache_age;
        if (age > 0) {
            f32 dx = (position->x + 1.0f) - cache.center_x;
            f32 dz = (position->z + 1.0f) - cache.center_z;
            if (dx > 0.0f && dx < 2.0f && dz > 0.0f && dz < 2.0f) {
                cache_index = i;
                cache_hit = true;
                break;
            }
        }
        if (age < oldest_age) {
            oldest_age = age;
            cache_index = i;
        }
    }
    TERRAIN_INDEX_LEVEL &cache = CurTerr->index_levels[cache_index];
    bool fill_cache = !cache_hit && cache.cache_age <= 1;
    ShadowScanWriter writer;
    writer.group_header = fill_cache ? cache.scan_list : TerI->scan_list_storage;
    writer.cursor = reinterpret_cast<TERRAIN_SHAPE **>(writer.group_header + sizeof(TERRAIN_SHAPE *));
    // Target stops at TerI + 0x93c, leaving the final header slot available.
    writer.limit = writer.group_header + 0x7f4;
    writer.shape_count = 0;

    const f32 extent = fill_cache ? 1.0f : SHADOW_SCAN_HALF_EXTENT;
    f32 min_x = position->x - extent;
    f32 max_x = position->x + extent;
    f32 min_z = position->z - extent;
    f32 max_z = position->z + extent;

    if (!cache_hit) {
        for (i32 cell_index = 0; cell_index < CurTerr->used_cell_count; ++cell_index) {
            const TERRAIN_CELL &cell = CurTerr->cells[cell_index];
            if (max_x < cell.min_x || cell.max_x < min_x || max_z < cell.min_z || cell.max_z < min_z) {
                continue;
            }
            const i16 *group_indices = CurTerr->group_indices + cell.first_group;
            for (i32 cell_group = 0; cell_group < static_cast<i16>(cell.group_count); ++cell_group) {
                const i32 group_index = group_indices[cell_group];
                ShadowScanGroup(group_index, min_x, min_z, max_x, max_z, terrain_mask, false, &writer);
            }
        }
        // Skin allocation can invalidate the cache while scanning groups.
        fill_cache = cache.cache_age <= 1;
    }
    if (fill_cache) {
        reinterpret_cast<i16 *>(writer.group_header)[0] = 0;
        reinterpret_cast<i16 *>(writer.group_header)[1] = 0;
        cache.center_x = position->x;
        cache.center_z = position->z;
    }
    if (fill_cache || cache_hit) {
        cache.cache_age = 8;
        writer.group_header = TerI->scan_list_storage;
        writer.cursor = reinterpret_cast<TERRAIN_SHAPE **>(writer.group_header + 4);
        writer.limit = reinterpret_cast<u8 *>(TerI) + 0x93c;
        min_x = position->x - SHADOW_SCAN_HALF_EXTENT;
        max_x = position->x + SHADOW_SCAN_HALF_EXTENT;
        min_z = position->z - SHADOW_SCAN_HALF_EXTENT;
        max_z = position->z + SHADOW_SCAN_HALF_EXTENT;
        u8 *entry = cache.scan_list;
        for (;;) {
            i16 count = reinterpret_cast<i16 *>(entry)[0];
            if (count <= 0)
                break;
            i16 group_index = reinterpret_cast<i16 *>(entry)[1];
            TERRAIN_SHAPE **shapes = reinterpret_cast<TERRAIN_SHAPE **>(entry + 4);
            entry = reinterpret_cast<u8 *>(shapes + count);
            TERRAIN_GROUP &group = CurTerr->groups[group_index];
            if (!ShadowBoundsOverlap(min_x, min_z, max_x, max_z, group.bounds_min, group.bounds_max) ||
                group.chunk_type == -1)
                continue;
            f32 local_min_x = min_x - group.origin.x, local_max_x = max_x - group.origin.x;
            f32 local_min_z = min_z - group.origin.z, local_max_z = max_z - group.origin.z;
            for (i32 i = 0; i < count; ++i) {
                TERRAIN_SHAPE *shape = shapes[i];
                if (local_max_x < shape->min_x || shape->max_x <= local_min_x || local_max_z < shape->min_z ||
                    shape->max_z <= local_min_z)
                    continue;
                if (shape->material[1] != 0 && (shape->material[1] & terrain_mask) == 0)
                    continue;
                if (reinterpret_cast<u8 *>(writer.cursor) >= writer.limit)
                    continue;
                *writer.cursor++ = shape;
                ++writer.shape_count;
            }
            ShadowFinishGroup(&writer, group_index);
        }
    }

    if (NuTerrPlatsOff == 0) {
        min_x -= 0.05f;
        min_z -= 0.05f;
        max_x += 0.05f;
        max_z += 0.05f;
        i16 *platform_groups = CurTerr->active_platform_groups;
        i32 platform_count = CurTerr->active_platform_count;
        if (!(min_x > CurTerr->platform_scan_min.x && max_x < CurTerr->platform_scan_max.x &&
              min_z > CurTerr->platform_scan_min.z && max_z < CurTerr->platform_scan_max.z)) {
            TERRAIN_CELL &cell = CurTerr->cells[TERRAIN_PLATFORM_CELL];
            platform_groups = CurTerr->group_indices + cell.first_group;
            platform_count = static_cast<i16>(cell.group_count);
        }
        NUVEC4 *vertices = reinterpret_cast<NUVEC4 *>((reinterpret_cast<uintptr_t>(scratch) + 31) & ~uintptr_t(15));
        i32 transformed_count = 0;
        for (i32 p = 0; p < platform_count; ++p) {
            i32 group_index = platform_groups[p];
            TERRAIN_GROUP &group = CurTerr->groups[group_index];
            if (group.origin.x - group.radius > max_x || min_x > group.origin.x + group.radius ||
                group.origin.z - group.radius > max_z || min_z > group.origin.z + group.radius)
                continue;
            TERRAIN_PLATFORM &platform = CurTerr->platforms[group.scene_index];
            f32 local_min_x = min_x - group.origin.x, local_max_x = max_x - group.origin.x;
            f32 local_min_z = min_z - group.origin.z, local_max_z = max_z - group.origin.z;
            NUMTX *matrix = static_cast<NUMTX *>(platform.scene_object);
            if (matrix != NULL) {
                if (matrix->m30 > platform.previous_matrix.m30)
                    local_max_x = (matrix->m30 - platform.previous_matrix.m30) * 1.5f + max_x - group.origin.x;
                else
                    local_min_x = (matrix->m30 - platform.previous_matrix.m30) * 1.5f + min_x - group.origin.x;
                if (matrix->m32 > platform.previous_matrix.m32)
                    local_max_z = (matrix->m32 - platform.previous_matrix.m32) * 1.5f + max_z - group.origin.z;
                else
                    local_min_z = (matrix->m32 - platform.previous_matrix.m32) * 1.5f + min_z - group.origin.z;
            }
            bool rotating = (platform.flags & 1) != 0;
            if (rotating) {
                if (group.chunk_type == -1)
                    continue;
                f32 dx = (max_x + min_x) * 0.5f - group.origin.x;
                f32 dz = (min_z + max_z) * 0.5f - group.origin.z;
                f32 radius = group.radius + 0.00001f;
                if (!(radius * radius > dx * dx + dz * dz))
                    continue;
            } else if (!ShadowBoundsOverlap(local_min_x, local_min_z, local_max_x, local_max_z, group.bounds_min,
                                            group.bounds_max) ||
                       group.chunk_type == -1) {
                continue;
            }
            if (platform.scene_transform != NULL &&
                (*static_cast<u8 *>(platform.scene_transform) & ((platform.flags & 4) != 0 ? 2 : 1)) == 0)
                continue;
            TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
            while (batch->marker >= 0) {
                TERRAIN_SHAPE *shapes = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
                bool batch_overlaps = rotating || (local_max_x >= batch->min_x && batch->max_x > local_min_x &&
                                                   local_max_z >= batch->min_z && batch->max_z > local_min_z);
                for (i32 i = 0; batch_overlaps && i < batch->shape_count; ++i) {
                    TERRAIN_SHAPE *shape = &shapes[i];
                    if (shape->material[1] != 0 && (shape->material[1] & terrain_mask) == 0)
                        continue;
                    if (reinterpret_cast<u8 *>(writer.cursor) >= writer.limit)
                        continue;
                    if (!rotating) {
                        if (local_max_x < shape->min_x || shape->max_x <= local_min_x || local_max_z < shape->min_z ||
                            shape->max_z <= local_min_z)
                            continue;
                    } else {
                        for (i32 v = 0; v < 3; ++v) {
                            vertices[v] = {shape->vectors[v].x, shape->vectors[v].y, shape->vectors[v].z, 0.0f};
                        }
                        NuVec4MtxTransformVU0x3(vertices, vertices, matrix);
                        bool quad = shape->normals[1].y < 65535.0f;
                        if (quad) {
                            vertices[3] = {shape->vectors[3].x, shape->vectors[3].y, shape->vectors[3].z, 0.0f};
                            NuVec4MtxTransformVU0(&vertices[3], &vertices[3], matrix);
                        } else
                            vertices[3] = vertices[2];
                        bool above_x = false, below_x = false, above_z = false, below_z = false;
                        for (i32 v = 0; v < 4; ++v) {
                            above_x |= vertices[v].x > local_min_x;
                            below_x |= local_max_x > vertices[v].x;
                            above_z |= vertices[v].z > local_min_z;
                            below_z |= local_max_z > vertices[v].z;
                        }
                        if (!above_x || !below_x || !above_z || !below_z)
                            continue;
                        TERRAIN_SHAPE *transformed = &ScaleTerrain[transformed_count];
                        *reinterpret_cast<u32 *>(transformed->material) = *reinterpret_cast<u32 *>(shape->material);
                        for (i32 v = 0; v < (quad ? 4 : 3); ++v)
                            transformed->vectors[v] = {vertices[v].x, vertices[v].y, vertices[v].z};
                        if (!quad)
                            transformed->normals[1].y = 65536.0f;
                        for (i32 n = quad ? 1 : 0; n >= 0; --n) {
                            i32 origin = n ? 3 : 0, first = n ? 1 : 2, second = n ? 2 : 1;
                            NUVEC a = {vertices[first].x - vertices[origin].x, vertices[first].y - vertices[origin].y,
                                       vertices[first].z - vertices[origin].z};
                            NUVEC b = {vertices[second].x - vertices[origin].x, vertices[second].y - vertices[origin].y,
                                       vertices[second].z - vertices[origin].z};
                            NUVEC &normal = transformed->normals[n];
                            normal = TerCrossProduct(&a, &b);
                            f32 length = NuFsqrt((normal.x * normal.x + normal.y * normal.y) + normal.z * normal.z);
                            f32 inverse = length == 0.0f ? 0.0f : 1.0f / length;
                            normal.x *= inverse;
                            normal.y *= inverse;
                            normal.z *= inverse;
                        }
                        shape = transformed;
                        ++transformed_count;
                    }
                    *writer.cursor++ = shape;
                    ++writer.shape_count;
                }
                batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(shapes + batch->shape_count);
            }
            ShadowFinishGroup(&writer, group_index);
        }
    }
    NuScratchRelease();
    i16 *terminator = reinterpret_cast<i16 *>(writer.group_header);
    terminator[0] = 0;
    terminator[1] = 0;
    TerI->scan_list = TerI->scan_list_storage;
}
