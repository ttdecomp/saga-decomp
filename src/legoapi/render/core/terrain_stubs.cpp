#include "legoapi/world/world_shared.h"
#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/android/nutimebar_plain.h"
#include "nu2api/nu3d/android/nurain_android.h"
#include "gameapi/edtools/edstubs.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nucore/nustring.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

i16 TerrImpact;
i32 terrhitflags;
i32 TERRAINMASK_NONWEAPON = 0x20;
i32 TERRAINMASK_NONDROID;
NUVEC TerrImpactPos;
NUVEC TerrImpactNormal;
NUVEC ShadNorm;
NUVEC ShadRoofNorm;
i16 castroofnum;
i16 shadroofhit;
NUVEC EShadNorm;
NUVEC EShadRoofNorm;
i16 ecastroofnum;
i16 eshadroofhit;
f32 ShadRoofY;
f32 EShadRoofY;
i16 ecastnum;
i16 shadhit;
i16 eshadhit;
TERRAIN_SHAPE *ShadRoofPoly;
TERRAIN_SHAPE *EShadRoofPoly;
extern TERRAIN_SHAPE *EShadPoly;
extern void *ScaleTerrainT1;
extern TERRAIN_SHAPE *ScaleTerrain;
struct TerrainLastImpact_s {
    NUVEC position;
    f32 hit_type;
};
TerrainLastImpact_s TerrLastImpact;
TERRSET *CurTerr;
TERRPICKUPSET *PickupTerr;
PLATSKININFO *PlatSkinInfo;
PLATSKINMEMINFO *SkinMemInfo;
i32 PlatSkinMaxStore;
i32 TerrainUpadteCnt;
u8 *PlatSkinMem;
u8 *PlatSkinMemEnd;
i32 PlatSkinMax;
i32 PlatSkinMaxSize;
i32 PlatSkinCnt;
static i32 PlatSkinResetTotal = -1;
void SkinPlatformSize(i32, unsigned char *, PLATSKININFO *);
extern i32 PlatImpactId;
i32 ReadTerrainPickup(unsigned char *, i16 **, TERRPICKUPSET *);
// Runtime-selected groups appended after the fixed terrain allocation.
i32 curPickInst;
i32 WallSplinesOnly;
f32 TerrPlatScanDist = 10.0f;
i32 cntrots;
struct TERRAIN_PLATFORM_CALLBACK {
    void (*function)(void *);
    void *argument;
};
static i32 PlatCodeCallback;
static TERRAIN_PLATFORM_CALLBACK PlatCallback[8];
extern "C" i32 DeletePlatinst(i32 platform_index);
extern "C" PartHeader *CreateDmaPartEffectList(void *memory, i32 *size);
extern "C" dma_particle_chunk_s *CreateDmaParticleSet(void *memory, i32 *size);
extern "C" dma_particle_chunk_s *CreateDmaParticleSetGlass(void *memory, i32 *size);
void ScanTerrIDRemovePlat(i32 platform_index);

u8 TerrainHitInfo[4];
i32 plathitid;
extern i32 TerrPolyObj;
extern TERRAIN_SHAPE *TerrPoly;
extern u8 TerrWallInfo;
i32 PlatCrush;
i16 NuTerrPlatsOff;
i16 castnum;
TERRAIN_TRACK_SLOT *CurTrackInfo;
tertype **TerrOverRideScan;
TerrainQuery_s *TerI;
f32 wallover;
extern i32 terraincnt;
extern i32 curSphereter;
TERRAIN_SPHERE SphereData[16];
extern i32 platinrange;
extern TERRAIN_SHAPE *ShadPoly;
void *TerImpactData;
i32 *TerImpactDataCount;
i32 TerImpactDataMax;
static TERRAIN_AXIS_FREEDOM_SHAPE *TerrShape;
static i32 TerrShapeAdjCnt;

extern "C" {
    void *NuScratchAlloc32(i32 size);
    void NuScratchRelease(void);
}

TERRAIN_TRACK_SLOT *ScanTerrId(void *hit_flags);
void ScanTerrain(i32 scan_type, i32 terrain_mask, i32 scan_flags);
i32 PlatformChecks(i32 count, NUVEC *movement);
void DerotateMovementVector(void);
i32 HitTerrain(void);
void TerrainImpactNorm(void);
void RayImpact(NUVEC *);
void StorePlatImpact(void);
void NewTerrStoreAnyInfo(void);
i32 TerrainPlatformEmbedded(NUVEC *movement);
i32 TerrShapeSideStep(NUVEC *position, NUVEC *movement, u8 *hit_flags);
void TerrainImpact(NUVEC *position, NUVEC *movement, u8 *hit_flags);
void TerrFlush(void);
void NewScanRot(NUVEC *position, i32 terrain_mask);
f32 NewCast(NUVEC *position, f32 height_above, f32 height_below);

extern "C" {
    extern debkeydatatype_s *debkeydata;
    extern i32 maxdebkeys;
    extern debinftype **debtab;
    extern i32 EDPP_MAX_TYPES;
    extern i32 EDPP_SCALE_TYPES;
    extern i32 edpp_types_used;
    extern debinftype *effecttypes;
    extern debscale_s *debscale;
    extern f32 panelglobaltime;
    extern f32 globaltime;
    extern f32 renderpanelglobaltime;
    extern f32 renderglobaltime;
    extern f32 timeincrement;
    extern i32 globalframes;
    extern i32 update_debris_enabled;
    extern u32 debrisseed;
    extern PartHeader **DmaDebTypes;
    extern i32 EDPP_MAX_DMADEBTYPES;
    extern i32 freeDmaDebType;
    extern i32 debris_setup_called;
    extern usize debris_trash_space;
    extern usize debris_trash_size;
    extern i32 debrischunks;
    extern i32 debrischunksglass;
    extern i32 freedebchkptr;
    extern i32 freedebchkptrg;
    extern debris_chunk_control_s *debris_chunk_controls;
    extern debris_chunk_control_s **freechunkcontrols;
    extern i32 freechunkcontrolsptr;
    extern dma_particle_chunk_s **freedebchunks;
    extern dma_particle_chunk_s **freedebchunksglass;
    extern particlechunkrendertype_s *ParticleChunkToRender;
    extern particlechunkrendertype_s *ParticleChunkRenderStack[5];
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
    extern NUMTL *DebMat[10];
    extern i32 debris_suspended;
    extern f32 debris_thinning_level;
    extern i32 forced_debris_thinning;

    void NuRndrParticleGroup(uv1debdata *, PartHeader *, NUMTL *, f32, NUMTX *, i32, f32, f32, f32, f32);
    void NuRndrSetParticleRotation(NUMTX *);

    void DebrisReScale(i32, f32);
    void GenericDebinfoDmaTypeUpdate(debinftype *);
    void DebReAlloc(debkeydatatype_s *, i32);
    extern "C++" void DebrisProcessSpheres(uv1deb *, f32, debinftype *, debkeydatatype_s *, i32);
    extern "C++" {
        void DebrisCleanUpDmaDebTypeTables(void);
        void DebrisProcessAllocation(void);
        void DebrisProcessControlChunks(i32);
        void DebrisProcessGeneration(void);
        void DebrisProcessTriggers(void);
    }
}

// InitGameDebris @0x3ca2d0 (game_deb.cpp).
// edppLoadPage @0x36c630 (edtoolsall_plain.cpp) — deferred parts-page loader.
void DebrisFreeOldestDmaDebTypeTable() {
    i32 oldest = 0;
    f32 oldest_age = 0.0f;
    for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
        debinftype *effect = debtab[i];
        if (effect == NULL || effect->native_data == NULL)
            continue;
        f32 age = (effect->time_group == 4 ? panelglobaltime : globaltime) - effect->last_render_time;
        if (age > oldest_age) {
            oldest_age = age;
            oldest = i;
        }
    }
    if (oldest != 0) {
        DmaDebTypes[--freeDmaDebType] = debtab[oldest]->native_data;
        debtab[oldest]->native_data = NULL;
    }
}

extern "C" i32 edppLoadPage(char *path, i32 flag, usize scene);
// NuFileExists @nufile (nucore_plain.cpp).
extern "C" i32 NuFileExists(char *name);

void TerrainSideClamp(NUVEC *axis, NUVEC *position) {
    TERRAIN_AXIS_FREEDOM_SHAPE *shape = TerrShape;
    const f32 shape_x = shape->offset.x;
    const f32 axis_z = axis->z;
    const f32 axis_x = axis->x;
    const f32 shape_z = shape->offset.z;
    const f32 forward_z = shape_z * axis_z;
    f32 clamped_forward = shape_x * axis_x;
    f32 clamped_side = shape_x * axis_z;
    clamped_forward += forward_z;
    clamped_side -= shape_z * axis_x;

    const f32 radius = shape->radius;
    const f32 negative_radius = -radius;
    clamped_forward = MIN(radius, clamped_forward);
    clamped_forward = MAX(negative_radius, clamped_forward);
    const f32 side_radius = radius * 0.2f;
    const f32 negative_side_radius = negative_radius * 0.2f;
    clamped_side = MIN(side_radius, clamped_side);
    clamped_side = MAX(negative_side_radius, clamped_side);

    position->x -= (axis->x * clamped_forward + axis->z * clamped_side) - shape->offset.x;
    position->z -= (axis->z * clamped_forward - axis->x * clamped_side) - shape->offset.z;

    shape->offset.x = axis->x * clamped_forward + axis->z * clamped_side;
    shape->offset.z = axis->z * clamped_forward - axis->x * clamped_side;
}

i32 TerrShapeSideStep(NUVEC *, NUVEC *, u8 *) {
    enum TERRAIN_SIDE_STEP_RESULT {
        TERRAIN_SIDE_STEP_HANDLED = 0,
        TERRAIN_SIDE_STEP_USE_STANDARD_IMPACT = 1,
    };

    constexpr f32 kCylinderFloorThreshold = 0.5f;
    constexpr f32 kMaximumWallNormalY = 0.707f;
    constexpr f32 kMinimumWallNormalY = -0.8f;
    constexpr f32 kMovementPushScale = 0.15f;
    constexpr f32 kShapePushScale = 0.05f;

    i32 result = TERRAIN_SIDE_STEP_USE_STANDARD_IMPACT;
    TerrainQuery_s *query = TerI;
    const u8 hit_type = query->hit_type;
    if (hit_type == TERRAIN_HIT_TYPE_NONE) {
        return result;
    }

    i32 push_from_collision_normal;
    switch (hit_type) {
        case TERRAIN_HIT_TYPE_CYLINDER:
        case TERRAIN_HIT_TYPE_VERTEX:
            push_from_collision_normal = kCylinderFloorThreshold <= query->impact_normal.y;
            break;
        default:
            if (query->impact_normal.y >= kMaximumWallNormalY || query->impact_normal.y < kMinimumWallNormalY) {
                return TERRAIN_SIDE_STEP_USE_STANDARD_IMPACT;
            }
            push_from_collision_normal = false;
            break;
    }

    const f32 shape_angle = TerrShape->angle;
    NUVEC axis;
    axis.x = NuTrigTable[static_cast<i32>(shape_angle + 16384.0f) >> 1 & 0x7fff];
    axis.y = 0.0f;
    axis.z = -NuTrigTable[static_cast<i32>(shape_angle) >> 1 & 0x7fff];

    query->start_position.x = query->position.x;
    query->start_position.y = query->position.y;
    query->start_position.z = query->position.z;
    query->start_movement.x = query->movement.x;
    query->start_movement.y = query->movement.y;
    query->start_movement.z = query->movement.z;
    query->movement.y = 0.0f;

    if (push_from_collision_normal == 1) {
        const f32 push_x = query->movement_normal.x * TerrShape->radius;
        const f32 push_z = query->movement_normal.z * TerrShape->radius;
        query->movement.x = push_x * kMovementPushScale;
        query->movement.z = push_z * kMovementPushScale;
        TerrShape->offset.x += push_x * kShapePushScale;
        TerrShape->offset.z += push_z * kShapePushScale;
    } else {
        f32 axis_impact = query->impact_normal.x * axis.x + query->impact_normal.z * axis.z;
        if (axis_impact == 0.0f) {
            axis_impact = (TerrShape->offset.x * axis.x + TerrShape->offset.z * axis.z) / TerrShape->radius;
            if (axis_impact == 0.0f) {
                return TERRAIN_SIDE_STEP_USE_STANDARD_IMPACT;
            }
        }

        const f32 signed_distance = axis_impact < 0.0f ? -NuFsqrt(-axis_impact) : NuFsqrt(axis_impact);
        query = TerI;
        query->movement.x = axis.x * signed_distance * TerrShape->radius + TerrShape->offset.x;
        query->movement.z = axis.z * signed_distance * TerrShape->radius + TerrShape->offset.z;
    }

    u8 side_step_hit_flags[2];
    NUVEC impact_result;
    do {
        DerotateMovementVector();
        HitTerrain();
        TerrainImpactNorm();
        TerrainImpact(&impact_result, &impact_result, side_step_hit_flags);

        --TerrShapeAdjCnt;
        query = TerI;
    } while (TerrShapeAdjCnt > 0 && query->hit_type != TERRAIN_HIT_TYPE_NONE);

    if (query->hit_type == TERRAIN_HIT_TYPE_NONE) {
        query->position.x += query->movement.x;
        query->position.y += query->movement.y;
        query->position.z += query->movement.z;
    }

    TerrShape->offset.x += query->start_position.x - query->position.x;
    TerrShape->offset.z += query->start_position.z - query->position.z;
    query->hit_type = TERRAIN_HIT_TYPE_FACE;
    query->movement.x = query->start_movement.x;
    query->movement.y = query->start_movement.y;
    query->movement.z = query->start_movement.z;
    TerrainSideClamp(&axis, &query->position);

    result = TERRAIN_SIDE_STEP_HANDLED;
    return result;
}

extern "C" void NewTerrAxisFreedom(TERRAIN_AXIS_FREEDOM_SHAPE *shape, NUVEC *position) {
    TerrShape = shape;
    const f32 shape_angle = shape->angle;
    TerrShapeAdjCnt = 2;

    NUVEC axis;
    axis.x = NuTrigTable[static_cast<i32>(shape_angle + 16384.0f) >> 1 & 0x7fff];
    axis.y = 0.0f;
    axis.z = -NuTrigTable[static_cast<i32>(shape_angle) >> 1 & 0x7fff];
    TerrainSideClamp(&axis, position);
}

void TerrFlush() {
    TerrShapeAdjCnt = 0;
}

extern "C" void noterraininit(void) {
    terraincnt = 0;
    curSphereter = 0;
    platinrange = 0;
    ShadPoly = 0;
    TerI = static_cast<TerrainQuery_s *>(NuScratchAlloc32(sizeof(TerrainQuery_s)));
    NuScratchRelease();
    CurTerr = NULL;
    TerrFlush();
}
extern "C" void TerrainSetCur(void *terrain) {
    CurTerr = static_cast<TERRSET *>(terrain);
}
extern "C" void TerrSetPlatScanDist(f32 dist) {
    TerrPlatScanDist = dist;
}
extern "C" void TerrainPlatformOldUpdate(void) {
    if (CurTerr != NULL) {
        curPickInst = 0;
        for (i32 i = 0; i < CurTerr->removed_platform_count; ++i)
            DeletePlatinst(CurTerr->removed_platforms[i]);
        CurTerr->removed_platform_count = 0;
        curSphereter = 0;
        if (CurTerr->max_platforms <= 0)
            return;
        TERRAIN_PLATFORM *platform = CurTerr->platforms;
        TERRAIN_PLATFORM *end = platform + CurTerr->max_platforms;
        for (; platform != end; ++platform) {
            if (platform->scene_object != NULL)
                platform->previous_matrix = *static_cast<NUMTX *>(platform->scene_object);
        }
    }
}
extern "C" void TerrainPlatformNewUpdate(void) {
    if (CurTerr != NULL) {
        for (i32 i = 0; i < 64; ++i) {
            TERRAIN_TRACK_SLOT &slot = CurTerr->track_slots[i];
            if (slot.id != NULL) {
                if (slot.platform_contact_state > 0)
                    --slot.platform_contact_state;
                if (slot.wall_contact_state > 0)
                    --slot.wall_contact_state;
            }
        }
        for (i32 i = 0; i < 16; ++i) {
            if (static_cast<i16>(CurTerr->index_levels[i].entry_count) > 0)
                --CurTerr->index_levels[i].entry_count;
        }
    }
    for (i32 i = 0; i < PlatCodeCallback; ++i)
        PlatCallback[i].function(PlatCallback[i].argument);
    PlatCodeCallback = 0;
    cntrots = 0;
    if (CurTerr == NULL)
        return;

    const NUMTX &camera = global_camera.mtx;
    const f32 negative_distance = -TerrPlatScanDist;
    const f32 back_x = negative_distance * camera.m20 * 0.25f;
    const f32 back_z = negative_distance * camera.m22 * 0.25f;
    f32 min_x = camera.m00 * negative_distance * 0.25f + back_x;
    f32 min_z = camera.m02 * negative_distance * 0.25f + back_z;
    f32 max_x = camera.m00 * negative_distance * 0.35f + camera.m20 * TerrPlatScanDist;
    f32 max_z = camera.m02 * negative_distance * 0.35f + camera.m22 * TerrPlatScanDist;
    f32 x = min_x;
    f32 z = min_z;
    min_x = MIN(min_x, max_x);
    min_z = MIN(min_z, max_z);
    max_x = MAX(x, max_x);
    max_z = MAX(z, max_z);
    x = camera.m00 * TerrPlatScanDist * 0.35f + camera.m20 * TerrPlatScanDist;
    z = TerrPlatScanDist * camera.m02 * 0.35f + camera.m22 * TerrPlatScanDist;
    min_x = MIN(min_x, x);
    max_x = MAX(max_x, x);
    min_z = MIN(min_z, z);
    max_z = MAX(max_z, z);
    x = camera.m00 * TerrPlatScanDist * 0.25f + back_x;
    z = TerrPlatScanDist * camera.m02 * 0.25f + back_z;
    min_x = (MIN(min_x, x)) + (camera.m30 - 1.0f);
    max_x = (MAX(max_x, x)) + (camera.m30 + 1.0f);
    min_z = (MIN(min_z, z)) + (camera.m32 - 1.0f);
    max_z = (MAX(max_z, z)) + (camera.m32 + 1.0f);
    CurTerr->platform_scan_min.x = min_x;
    CurTerr->platform_scan_min.z = min_z;
    CurTerr->platform_scan_max.x = max_x;
    CurTerr->platform_scan_max.z = max_z;
    CurTerr->active_platform_count = 0;
    TERRAIN_CELL &cell = CurTerr->cells[TERRAIN_PLATFORM_CELL];
    i16 *group_index = CurTerr->group_indices + cell.first_group;
    i16 *end = group_index + cell.group_count;
    for (; group_index != end; ++group_index) {
        TERRAIN_GROUP &group = CurTerr->groups[*group_index];
        TERRAIN_PLATFORM &platform = CurTerr->platforms[group.scene_index];
        if (platform.scene_transform != NULL) {
            const u8 visibility = *static_cast<u8 *>(platform.scene_transform);
            if ((platform.flags & TERRAIN_PLATFORM_FLAG_DISPLAY_LIST_BACKED) != 0) {
                if ((visibility & 2) == 0)
                    continue;
            } else if ((visibility & 1) == 0)
                continue;
        }
        if (group.chunk_type == 0)
            continue;
        NUMTX *matrix = static_cast<NUMTX *>(platform.scene_object);
        if (matrix != NULL) {
            if (platform.bounce_frames != 0) {
                f32 velocity = platform.bounce_velocity - platform.bounce_damping * platform.bounce_velocity;
                if (platform.bounce_frames >= 125)
                    velocity = (platform.bounce_impulse - platform.bounce_damping * platform.bounce_velocity) +
                               platform.bounce_velocity;
                const f32 offset = platform.bounce_offset;
                --platform.bounce_frames;
                velocity += -offset * fabsf(offset) * platform.bounce_spring * 0.5f - platform.bounce_spring * offset;
                platform.bounce_velocity = velocity;
                platform.bounce_offset = velocity + offset;
                matrix->m31 += platform.bounce_offset;
            }
            group.origin = *NUMTX_GET_ROW_VEC(matrix, 3);
        }
        const u8 flags = platform.flags;
        if ((flags & TERRAIN_PLATFORM_FLAG_COLLIDED) != 0)
            platform.bounce_frames = 128;
        platform.field_0x44 = 0;
        platform.flags &= ~TERRAIN_PLATFORM_FLAG_COLLIDED;
        if ((flags & TERRAIN_PLATFORM_FLAG_ROTATING) == 0) {
            if (group.bounds_min.x + group.origin.x > max_x || group.origin.x + group.bounds_max.x < min_x ||
                group.bounds_min.z + group.origin.z > max_z || group.origin.z + group.bounds_max.z < min_z)
                continue;
        } else {
            if (group.origin.x - group.radius > max_x || group.origin.x + group.radius < min_x ||
                group.origin.z - group.radius > max_z || group.origin.z + group.radius < min_z)
                continue;
        }
        if (CurTerr->active_platform_count < 96)
            CurTerr->active_platform_groups[CurTerr->active_platform_count++] = *group_index;
    }
}
extern "C" i32 PARTLookupTypePageOnly(char *, i32);
extern "C" part_type_s part_types[128];
void *InitPartDebris(VARIPTR *buf, VARIPTR *, i32 capacity, i32 named_count, char **names, i32 page) {
    PARTDEBSYS_s *system = static_cast<PARTDEBSYS_s *>(BUFFER_ALLOC(buf, sizeof(PARTDEBSYS_s), 16));
    if (system == NULL)
        return system;
    system->entries = NULL;
    system->capacity = capacity;
    system->named_count = named_count;
    system->entries = static_cast<PARTDEBENTRY_s *>(BUFFER_ALLOC(buf, capacity * sizeof(PARTDEBENTRY_s), 16));
    if (system->entries == NULL)
        return NULL;
    memset(system->entries, -1, capacity * sizeof(PARTDEBENTRY_s));
    i32 i = 0;
    if (names != NULL) {
        for (; i < system->named_count; ++i) {
            NuStrCpy(system->entries[i].name, names[i]);
            system->entries[i].type_id = -1;
            system->entries[i].type_id = PARTLookupTypePageOnly(system->entries[i].name, page);
        }
    }
    for (i32 type = 0; type < 128 && i < system->capacity; ++type) {
        system->entries[i].type_id = -1;
        if (part_types[type].variant_count > 0) {
            NuStrCpy(system->entries[i].name, part_types[type].name);
            system->entries[i].type_id = PARTLookupTypePageOnly(system->entries[i].name, page);
            ++i;
        }
    }
    for (; i < system->capacity; ++i)
        system->entries[i].type_id = -1;
    return system;
}

// Particles_Load @0x4a2a50.
void Particles_Load(WORLDINFO *world, char **debris_name, i32 count, i32 flags) {
    char path[0x100];

    world->page_pp = -1;
    sprintf(path, "%s.ptl", world->config_file);
    if (NuFileExists(path) != 0) {
        world->page_pp = edppLoadPage(path, 1, reinterpret_cast<usize>(world->current_gscn));
    }

    world->debris_sys = (APIDEBRISSYS_s *)InitGameDebris(&world->giz_buffer, world->unknown_0108, count, flags,
                                                         debris_name, (char)world->page_pp);
}

extern "C" {

    void AITerrInit(void) {
    }

    void AITerrShadow(void) {
    }

    void AITerrShadowOnPlatform(void) {
    }

    i32 CheckForPlatInst(i32 instance) {
        if (CurTerr->max_platforms <= 0) {
            return 0;
        }
        TERRAIN_PLATFORM *platform = CurTerr->platforms;
        for (i32 i = 0; i < CurTerr->max_platforms; ++i, ++platform) {
            if (platform->scene_object != NULL && static_cast<i16>(platform->scene_object_index) == instance) {
                return 1;
            }
        }
        return 0;
    }

    i32 CreateScaledEffect(i32 effect_index, f32 requested_scale) {
        if (effect_index < 1 || EDPP_MAX_TYPES < effect_index || debtab[effect_index] == NULL) {
            return -1;
        }

        debinftype *effect = debtab[effect_index];
        if (effect->scale != 1.0f) {
            effect_index = effect->unscaled_effect_index;
            effect = debtab[effect_index];
            if (effect == NULL) {
                return -1;
            }
        }
        if (requested_scale == 1.0f && effect_index != 0) {
            return effect_index;
        }

        if (requested_scale < 0.01f) {
            requested_scale = 0.01f;
        }

        for (i32 i = 0; i < EDPP_SCALE_TYPES; ++i) {
            debscale_s &entry = debscale[i];
            if (entry.unscaled_effect_index == effect_index && entry.scale == requested_scale &&
                entry.scaled_effect_index != 0) {
                debinftype *scaled = debtab[entry.scaled_effect_index];
                if (scaled != NULL && scaled->unscaled_effect_index == effect_index) {
                    return entry.scaled_effect_index;
                }
                entry.scale = 0.0f;
            }
        }

        i32 closest_index = effect_index;
        f32 closest_distance = fabsf(requested_scale / effect->scale - 1.0f);
        for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
            debinftype *candidate = debtab[i];
            if (candidate != NULL && candidate->unscaled_effect_index == effect_index) {
                f32 distance = 1.0f;
                if (requested_scale != 0.0f && candidate->scale != 0.0f) {
                    distance = fabsf(requested_scale / candidate->scale - 1.0f);
                }
                if (distance < closest_distance) {
                    closest_distance = distance;
                    closest_index = i;
                }
            }
        }
        if (closest_index != 0 && closest_distance < 0.1f) {
            return closest_index;
        }

        i32 scaled_index = 1;
        while (scaled_index < EDPP_MAX_TYPES && debtab[scaled_index] != NULL) {
            ++scaled_index;
        }
        if (scaled_index == EDPP_MAX_TYPES) {
            return closest_index;
        }

        debinftype *scaled = &effecttypes[scaled_index];
        debtab[scaled_index] = scaled;
        *scaled = *effect;
        scaled->native_data = NULL;
        for (i32 i = 0; i < 8; ++i) {
            scaled->particle_keys[i] = -1;
        }
        DebrisReScale(scaled_index, requested_scale);
        scaled->scale = requested_scale;
        scaled->last_render_time = scaled->time_group == 4 ? panelglobaltime : globaltime;
        scaled->unscaled_effect_index = effect_index;

        if (strlen(effect->name) < 13) {
            sprintf(scaled->name, "%s%03d", effect->name, scaled_index);
        } else {
            char shortened[13];
            memcpy(shortened, effect->name, 12);
            shortened[12] = '\0';
            sprintf(scaled->name, "%s%03d", shortened, scaled_index);
        }
        ++edpp_types_used;

        for (i32 i = 0; i < EDPP_SCALE_TYPES; ++i) {
            if (debscale[i].scale == 0.0f) {
                debscale[i].unscaled_effect_index = effect_index;
                debscale[i].scaled_effect_index = scaled_index;
                debscale[i].scale = requested_scale;
                break;
            }
        }
        return scaled_index;
    }

    void CreateScaledPARTEffect(void) {
    }

    void CubeImpact(void) {
    }

    void DebFreeAllCreatedEffects(void) {
    }

    void DebFreeAllDMADebTablesInstantly(void) {
    }

    void DebFreeAllPanelEffects(void) {
    }

    void Debris(i32 update_panel_time) {
        if (update_debris_enabled == 0 || debris_suspended != 0) {
            return;
        }

        panelglobaltime += timeincrement;
        renderpanelglobaltime += timeincrement;
        DebrisProcessControlChunks(1);
        if (update_panel_time == 0) {
            globaltime += timeincrement;
            renderglobaltime += timeincrement;
            ++globalframes;
            DebrisCleanUpDmaDebTypeTables();
            DebrisProcessTriggers();
            DebrisProcessAllocation();
            DebrisProcessGeneration();
            DebrisProcessControlChunks(0);
        }
    }

    void DebrisAllCollisionCheckScaleYFlag(void) {
    }

    void DebrisCollisionCheck(void) {
    }

    void DebrisCollisionCheckFlag(void) {
    }

    void DebrisCollisionCheckScaleY(void) {
    }

    void DebrisCollisionCheckScaleYFlag(void) {
    }

} // extern "C"

i32 NuRndrFlickerBeginScene(void);
void NuRndrFlickerEnd(void);

void DebrisTimeSlip(i32 group) {
    if (group == 0) {
        globaltime = renderglobaltime;
        for (debris_chunk_control_s *control = debris_chunk_control_stack[0]; control != NULL; control = control->next)
            control->expiry_time -= 800.0f;
    } else if (group == 1) {
        panelglobaltime = renderpanelglobaltime;
        for (debris_chunk_control_s *control = debris_chunk_control_stack[1]; control != NULL; control = control->next)
            control->expiry_time -= 800.0f;
    }
    for (i32 i = 0; i < maxdebkeys; ++i) {
        debkeydatatype_s *key = &debkeydata[i];
        debinftype *effect = debtab[key->effect_index];
        if (effect->time_group == 4 ? group != 1 : group != 0)
            continue;
        key->last_update_time -= 800.0f;
        key->emission_epoch -= 800.0f;
        key->emission_time -= 800.0f;
        key->previous_emission_time -= 800.0f;
        key->field_1e4 -= 800.0f;
        for (i32 chunk = 0; chunk < 32; ++chunk) {
            if (key->particle_chunks[chunk] == NULL)
                continue;
            dma_particle_chunk_s *particles = key->particle_chunks[chunk];
            i32 count = effect->particle_type == 7 ? 12 : 32;
            for (i32 particle = 0; particle < count; ++particle)
                particles->particles[particle].start_time -= 800.0f;
        }
    }
    for (i32 i = 1; i < EDPP_MAX_TYPES; ++i) {
        debinftype *effect = debtab[i];
        if (effect == NULL)
            continue;
        if (effect->time_group == 4 ? group != 1 : group != 0)
            continue;
        effect->last_render_time -= 800.0f;
        if (effect->native_data != NULL)
            effect->native_data->last_render_time -= 800.0f;
    }
}

void DebrisDrawCalculateClipBoxes(debinftype *effect, debkeydatatype_s *key) {
    NUMTX matrix = key->effect_orientation;
    NuMtxTranslate(&matrix, &key->position);
    f32 emission_time = effect->emission_period_random + effect->emission_pause;
    f32 lifetime = effect->particle_lifetime;
    NUVEC extent = {
        (fabsf(effect->emitter_velocity.x) * emission_time + effect->field_04c * lifetime) + effect->field_058,
        (fabsf(effect->emitter_velocity.y) * emission_time +
         (fabsf(effect->field_048) + effect->field_050) * lifetime) +
            effect->field_05c,
        (fabsf(effect->emitter_velocity.z) * emission_time + effect->field_054 * lifetime) + effect->field_060};
    NuVecMtxRotate(&extent, &extent, &key->emitter_orientation);
    if (extent.x < 0.0f)
        extent.x = -extent.x;
    if (extent.y < 0.0f)
        extent.y = -extent.y;
    if (extent.z < 0.0f)
        extent.z = -extent.z;
    f32 padding = 0.0001f * effect->field_14c;
    extent.x = (extent.x + padding) + 0.2f;
    extent.y = (extent.y + padding) + 0.2f;
    extent.z = (extent.z + padding) + 0.2f;
    NUVEC minimum = {-extent.x, -extent.y, -extent.z};
    if (effect->field_0a0 > 0.0f)
        extent.y += (effect->field_0a0 * lifetime) * lifetime;
    else if (effect->field_0a0 < 0.0f)
        minimum.y = (effect->field_0a0 * lifetime) * lifetime - extent.y;
    f32 horizontal = extent.x > extent.z ? extent.x : extent.z;
    f32 radius;
    if (extent.y > -minimum.y)
        radius = horizontal > extent.y ? horizontal : extent.y;
    else if (horizontal > -minimum.y)
        radius = horizontal;
    else
        radius = extent.y > -minimum.y ? extent.y : -minimum.y;
    key->clip_max = extent;
    key->clip_matrix = matrix;
    key->clip_radius = radius;
    key->clip_min = minimum;
}

extern "C" {
    static i32 debris_rt;
    static i32 debris_initialised;
    static NUMTL *debris_copy_mtl;
    static f32 debrisu1;
    static f32 debrisv1;
    extern i32 PS2_REZ_W;
    extern i32 PS2_REZ_H;
    i32 NuPower2(i32 value);

    static i32 DebrisCutSceneMode;
    i32 DebrisSuspendDrawObjectSwitch = -1;
    i32 g_renderingDebris;
    extern NUMTX xzfacingmtx;
    f32 CameraEmitterDistance(NUVEC *position);

    void DebrisDraw(i32, i32 pass) {
        if (debris_suspended != 0)
            return;
        _NuTimeBarSlotBegin(0, 12, "deb");
        DebMat[7]->tex_id = static_cast<i16>(debris_rt);
        i32 drawn = 0;
        for (particlechunkrendertype_s *chunk = ParticleChunkRenderStack[pass]; chunk != NULL; chunk = chunk->next) {
            if (chunk->particle_chunk == NULL)
                continue;
            debinftype *effect = chunk->effect;
            debkeydatatype_s *key = chunk->key;
            if (DebrisCutSceneMode != 0 && effect->cutscene_only != 0)
                continue;
            if (DebrisSuspendDrawObjectSwitch != -1 && key != NULL &&
                key->trigger_second == DebrisSuspendDrawObjectSwitch)
                continue;
            if (effect->native_data == NULL) {
                if (freeDmaDebType == EDPP_MAX_DMADEBTYPES)
                    DebrisFreeOldestDmaDebTypeTable();
                GenericDebinfoDmaTypeUpdate(effect);
                if (effect->native_data == NULL)
                    continue;
            }
            NUMTX matrix;
            NUVEC position;
            if (key != NULL) {
                bool assigned = (effect->particle_keys[0] != -1 && &debkeydata[effect->particle_keys[0]] == key) ||
                                (effect->particle_keys[1] != -1 && &debkeydata[effect->particle_keys[1]] == key) ||
                                (effect->particle_keys[2] != -1 && &debkeydata[effect->particle_keys[2]] == key) ||
                                (effect->particle_keys[3] != -1 && &debkeydata[effect->particle_keys[3]] == key) ||
                                (effect->particle_keys[4] != -1 && &debkeydata[effect->particle_keys[4]] == key) ||
                                (effect->particle_keys[5] != -1 && &debkeydata[effect->particle_keys[5]] == key) ||
                                (effect->particle_keys[6] != -1 && &debkeydata[effect->particle_keys[6]] == key) ||
                                (effect->particle_keys[7] != -1 && &debkeydata[effect->particle_keys[7]] == key);
                bool visible = true;
                if (!assigned && effect->sound_range > 0.0f && effect->use_explicit_clip_box == 0 &&
                    key->cutoff_distance > effect->sound_range)
                    visible = false;
                if (key->field_2f7 == 0)
                    continue;
                if (visible && pass != 4 && effect->generator_type == 0 && effect->use_explicit_clip_box == 0 &&
                    !assigned) {
                    if (key->clip_radius == 0.0f || *edbits_editor_enabled != 0)
                        DebrisDrawCalculateClipBoxes(effect, key);
                    if (edbits_editmode == 0 && key->gscene != NULL && key->field_2f2 != -1)
                        visible = NuPortalClipTest(key->gscene, &key->position, key->clip_radius, key->field_2f2) != 0;
                    NUVEC minimum = key->clip_min;
                    NUVEC maximum = key->clip_max;
                    if (NuCameraClipTestExtents(&minimum, &maximum, &key->clip_matrix, 0.0f, 0) == 0)
                        continue;
                }
                if (!visible)
                    continue;
                matrix = key->effect_orientation;
                position = key->position;
            } else {
                if (effect->sound_range > 0.0f && effect->use_explicit_clip_box == 0 &&
                    CameraEmitterDistance(&chunk->position) > effect->sound_range)
                    continue;
                matrix = chunk->effect_orientation;
                position = chunk->position;
            }
            NuMtxTranslate(&matrix, &position);
            f32 render_time;
            if (pass == 4) {
                NuMtxMulVU0(&matrix, &matrix, NuCameraGetMtx());
                render_time = renderpanelglobaltime;
                effect->last_render_time = panelglobaltime;
            } else {
                render_time = renderglobaltime;
                effect->last_render_time = globaltime;
            }
            if (key != NULL && key->field_2fa != 0)
                NuRndrSetParticleRotation(&key->particle_orientation);
            else if (effect->camera_facing != 0)
                NuRndrSetParticleRotation(&xzfacingmtx);
            else
                NuRndrSetParticleRotation(NULL);
            NuMtxPreScaleX(&matrix, 1.0f);
            i32 mode;
            if (effect->particle_type == 7)
                mode = 4;
            else if (effect->use_explicit_clip_box != 0) {
                NuRndrSetDebBox(reinterpret_cast<NUVEC *>(effect->fields_2f8));
                mode = 6;
            } else
                mode = 0;
            NuRndrParticleGroup(reinterpret_cast<uv1debdata *>(chunk->particle_chunk), chunk->effect->native_data,
                                DebMat[static_cast<i8>(chunk->effect->particle_type)], render_time, &matrix, mode,
                                effect->field_140, effect->field_144, effect->clip_extent, effect->field_044);
            drawn = 1;
        }
        if (drawn != 0)
            g_renderingDebris = g_renderingDebris == 0;
        _NuTimeBarSlotEnd(0, 12);
    }

    void DebrisDrawGlassEx(i32 glass_type);

    void DebrisDrawGlass(void) {
        DebrisDrawGlassEx(0);
    }

    i32 DebrisGlassParticlesActive(void);
    i32 NuRndrBeginSceneEx(i32, i32, i32);
    void NuRndrEndScene(void);

    void DebrisDrawGlassEx(i32 flicker) {
        if (debris_initialised == 0)
            return;
        i32 reserved = NuTexReserve(NuTexGetReqSize(debris_rt, 0));
        DebrisGlassParticlesActive();
        if (flicker == 0)
            NuRndrBeginSceneEx(-1, -2, 1);
        else
            NuRndrFlickerBeginScene();
        if (DebrisGlassParticlesActive() != 0)
            DebrisDraw(0, 2);
        NuRainDraw(reserved);
        if (flicker != 0)
            NuRndrFlickerEnd();
        NuRndrEndScene();
        NuTexUnReserve();
    }

    void DebrisEmitterMomentum(i32 handle, f32 x, f32 y, f32 z) {
        if (handle != -1) {
            debkeydata[handle].emitter_momentum = NUVEC{x, y, z};
        }
    }

    void DebrisEmitterOrientation(i32 handle, i16 z, i16 y, i16 x) {
        if (handle == -1) {
            return;
        }
        debkeydatatype_s &key = debkeydata[handle];
        NUMTX *mtx = &key.emitter_orientation;
        NuMtxSetIdentity(mtx);
        NuMtxRotateZ(mtx, z);
        NuMtxRotateY(mtx, y);
        NuMtxRotateX(mtx, x);
        mtx->m30 = 0.0f;
        mtx->m31 = 0.0f;
        mtx->m32 = 0.0f;
        key.orientation_dirty = 0.0f;
    }

    void DebrisEmitterOrientationMtx(i32 handle, NUMTX *source) {
        if (handle != -1) {
            debkeydatatype_s &key = debkeydata[handle];
            NUMTX *mtx = &key.emitter_orientation;
            memcpy(mtx, source, sizeof(NUMTX));
            mtx->m30 = 0.0f;
            mtx->m31 = 0.0f;
            mtx->m32 = 0.0f;
            key.orientation_dirty = 0.0f;
        }
    }

    void DebrisEmitterPos(i32 handle, f32 x, f32 y, f32 z) {
        if (handle != -1) {
            debkeydatatype_s &key = debkeydata[handle];
            key.orientation_dirty = 0.0f;
            key.position.x = x;
            key.position.y = y;
            key.position.z = z;
        }
    }

    void DebrisFindAllOfType(void) {
    }

    void DebrisGetConeProperties(void) {
    }

    void DebrisGetDuration(void) {
    }

    void DebrisGetName(void) {
    }

    void DebrisGetParticleCount(void) {
    }

    void DebrisGetRingProperties(void) {
    }

    void DebrisGetSeed(void) {
    }

    void DebrisGlassClose(void) {
        if (debris_initialised != 0) {
            if (debris_copy_mtl != NULL) {
                NuMtlDestroy(debris_copy_mtl);
                debris_copy_mtl = NULL;
            }
            if (debris_rt != 0) {
                NuTexDestroy(debris_rt);
                debris_rt = 0;
            }
            debris_initialised = 0;
        }
    }

    void DebrisGlassInit(void) {
        if (debris_initialised != 0)
            return;
        if (debris_rt == 0) {
            NUTEX texture = {NUTEX_RTT24, PS2_REZ_W, PS2_REZ_H};
            debris_rt = NuTexCreate(&texture);
        }
        i32 texture_width = NuPower2(PS2_REZ_W);
        debrisu1 = (static_cast<f32>(PS2_REZ_W) - 1.0f) / static_cast<f32>(texture_width);
        i32 texture_height = NuPower2(PS2_REZ_H);
        debrisv1 = (static_cast<f32>(PS2_REZ_H) - 1.0f) / static_cast<f32>(texture_height);
        if (debris_copy_mtl == NULL) {
            NUMTL *material = NuMtlCreateEx(1, 14);
            material->diffuse_color = {1.0f, 1.0f, 1.0f};
            material->opacity = 0.999f;
            debris_copy_mtl = material;
            material->tex_id = -1;
            material->attribs.unknown_1_1_2 = 1;
            material->attribs.unknown_1_4_8 = 1;
            material->attribs.cull_mode = 2;
            material->attribs.z_mode = 3;
            material->attribs.alpha_mode = 0;
            material->attribs.filter_mode = 0;
            material->attribs.unknown_2_1_2 = 2;
            material->attribs.unknown_2_4 = 1;
            NuMtlUpdate(material);
        }
        debris_initialised = 1;
    }

    i32 DebrisGlassParticlesActive(void) {
        return freedebchkptrg > 0;
    }

    void DebrisOrientation(i32 handle, i16 z, i16 y) {
        if (handle != -1) {
            NuMtxSetIdentity(&debkeydata[handle].effect_orientation);
            NUMTX *mtx = &debkeydata[handle].effect_orientation;

            const f32 cos_z = NU_COS_LUT(z);
            const f32 sin_z = NU_SIN_LUT(z);
            const f32 z_m00 = mtx->m00;
            const f32 z_m10 = mtx->m10;
            const f32 z_m20 = mtx->m20;
            const f32 z_m30 = mtx->m30;
            mtx->m00 = z_m00 * cos_z - mtx->m01 * sin_z;
            mtx->m01 = z_m00 * sin_z + mtx->m01 * cos_z;
            mtx->m10 = z_m10 * cos_z - mtx->m11 * sin_z;
            mtx->m11 = z_m10 * sin_z + mtx->m11 * cos_z;
            mtx->m20 = z_m20 * cos_z - mtx->m21 * sin_z;
            mtx->m21 = z_m20 * sin_z + mtx->m21 * cos_z;
            mtx->m30 = z_m30 * cos_z - mtx->m31 * sin_z;
            mtx->m31 = z_m30 * sin_z + mtx->m31 * cos_z;

            const f32 cos_y = NU_COS_LUT(y);
            const f32 sin_y = NU_SIN_LUT(y);
            const f32 y_m00 = mtx->m00;
            const f32 y_m10 = mtx->m10;
            const f32 y_m20 = mtx->m20;
            const f32 y_m30 = mtx->m30;
            mtx->m00 = y_m00 * cos_y + mtx->m02 * sin_y;
            mtx->m02 = mtx->m02 * cos_y - y_m00 * sin_y;
            mtx->m10 = y_m10 * cos_y + mtx->m12 * sin_y;
            mtx->m12 = mtx->m12 * cos_y - y_m10 * sin_y;
            mtx->m20 = y_m20 * cos_y + mtx->m22 * sin_y;
            mtx->m22 = mtx->m22 * cos_y - y_m20 * sin_y;
            mtx->m30 = y_m30 * cos_y + mtx->m32 * sin_y;
            mtx->m32 = mtx->m32 * cos_y - y_m30 * sin_y;
            debkeydata[handle].orientation_dirty = 0.0f;
        }
    }

    void DebrisOrientationMtx(i32 handle, NUMTX *source) {
        if (handle != -1) {
            debkeydatatype_s &key = debkeydata[handle];
            NUMTX *mtx = &key.effect_orientation;
            memcpy(mtx, source, sizeof(NUMTX));
            mtx->m30 = 0.0f;
            mtx->m31 = 0.0f;
            mtx->m32 = 0.0f;
            key.orientation_dirty = 0.0f;
        }
    }

    void DebrisParticleMomentum(i32 handle, f32 x, f32 y, f32 z) {
        if (handle != -1) {
            debkeydata[handle].momentum = NUVEC{x, y, z};
        }
    }

    void DebrisPopulateInstance(i32 handle, f32 duration) {
        if (duration < 0.0f || handle == -1) {
            return;
        }

        debkeydatatype_s *key = &debkeydata[handle];
        debinftype *effect = debtab[key->effect_index];
        if (effect->frequency == 0) {
            return;
        }
        if (duration == 0.0f) {
            duration = effect->particle_lifetime;
        }

        const f32 now = effect->time_group == 4 ? panelglobaltime : globaltime;
        const f32 end_time = now + timeincrement;
        f32 emission_time = now - duration;
        if (key->previous_allocated_chunk_count == 0) {
            const f32 thinning =
                forced_debris_thinning == 0
                    ? (debris_thinning_level <= effect->thinning ? debris_thinning_level : effect->thinning)
                    : debris_thinning_level;
            DebReAlloc(key, static_cast<i32>(static_cast<f32>(effect->max_particles) / thinning));
        }

        key->field_184 = 1;
        if (key->allocated_chunk_count <= 0) {
            return;
        }
        const f32 thinning =
            forced_debris_thinning == 0
                ? (debris_thinning_level <= effect->thinning ? debris_thinning_level : effect->thinning)
                : debris_thinning_level;
        const f32 emission_interval = 1.0f / (static_cast<f32>(effect->frequency) / thinning);
        key->emission_epoch = emission_time;
        emission_time += emission_interval;
        for (i32 remaining = 999; emission_time < end_time && remaining != 0; --remaining) {
            uv1deb *particle = key->generator(key, effect, emission_time);
            if (effect->process_spheres != 0 && particle != NULL) {
                DebrisProcessSpheres(particle, emission_time, effect, key, 0);
            }
            emission_time = key->emission_epoch + emission_interval;
        }
    }

    void DebrisPosOrientationMtx(i32 handle, NUMTX *source) {
        if (handle != -1) {
            debkeydatatype_s &key = debkeydata[handle];
            NUMTX *mtx = &key.effect_orientation;
            memcpy(mtx, source, sizeof(NUMTX));
            key.position.x = source->m30;
            key.position.y = source->m31;
            key.position.z = source->m32;
            mtx->m30 = 0.0f;
            mtx->m31 = 0.0f;
            mtx->m32 = 0.0f;
            key.orientation_dirty = 0.0f;
        }
    }

    void DebrisPreCheckCollisions(void) {
    }

    void DebrisProcessTimeSlip(void) {
        if (globaltime - 1.0f > renderglobaltime)
            DebrisTimeSlip(0);
        else if (globaltime > 900.0f)
            renderglobaltime -= 800.0f;
        if (panelglobaltime - 1.0f > renderpanelglobaltime)
            DebrisTimeSlip(1);
        else if (panelglobaltime > 900.0f)
            renderpanelglobaltime -= 800.0f;
    }

    void DebrisQueryPriority(void) {
    }

    void DebrisReScale(i32 effect_index, f32 scale) {
        if (effect_index < 0 || EDPP_MAX_TYPES <= effect_index || debtab[effect_index] == NULL) {
            return;
        }

        debinftype *effect = debtab[effect_index];
        for (i32 i = 0; i < 8; ++i) {
            effect->width_keys[i].value *= scale;
            effect->height_keys[i].value *= scale;
        }
        effect->field_148 *= scale;
        effect->field_14c *= scale;
        effect->field_048 *= scale;
        effect->field_0a0 *= scale;

        const u8 generator_type = effect->generator_type;
        if ((generator_type & 0xf7) == 0 || generator_type == 9 || generator_type == 10) {
            effect->field_058 *= scale;
            effect->field_05c *= scale;
            effect->field_060 *= scale;
            effect->field_04c *= scale;
            effect->field_050 *= scale;
            effect->field_054 *= scale;
        } else if (generator_type == 6 || generator_type == 7 || generator_type == 11 || generator_type == 12) {
            effect->field_058 *= scale;
            effect->field_04c *= scale;
        }
        effect->field_0b4 *= scale;
        effect->field_0bc *= scale;
        GenericDebinfoDmaTypeUpdate(effect);
    }

    void DebrisReflectionOrientation(i32 handle, i16 x, i16 y, f32 plane, f32 scale) {
        if (handle != -1) {
            debkeydatatype_s &key = debkeydata[handle];
            key.collision_plane = plane;
            key.reflection_x = x;
            key.reflection_y = y;
            key.reflection_scale = scale;
        }
    }

    NUVEC *CutoffCameraVec = NULL;

    void DebrisRegisterCutoffCameraVec(void *position) {
        CutoffCameraVec = static_cast<NUVEC *>(position);
    }

    void DebrisReserveTrashableSpace(void) {
    }

    void DebrisResetTimers(void) {
        panelglobaltime = 0.0f;
        renderpanelglobaltime = 0.0f;
        globaltime = 0.0f;
        renderglobaltime = 0.0f;
        globalframes = 0;
    }

    void DebrisSetCutSceneMode(i32 enabled) {
        DebrisCutSceneMode = enabled;
    }

    void DebrisSetDetailLevels(i32 handle, i32 detail_levels) {
        debkeydata[handle].field_1da = static_cast<u8>(detail_levels);
    }

    void DebrisSetDrawFlag(i32 handle, i8 draw_flag) {
        if (handle != -1) {
            debkeydata[handle].field_2f7 = draw_flag;
        }
    }

    void DebrisSetFacing(i32 handle, u8 enabled, i16 x_angle, i16 y_angle) {
        if (handle == -1)
            return;
        debkeydata[handle].field_2fa = enabled;
        NuMtxSetIdentity(&debkeydata[handle].particle_orientation);
        NUMTX *matrix = &debkeydata[handle].particle_orientation;
        f32 sine = NU_SIN_LUT(x_angle);
        f32 cosine = NU_COS_LUT(x_angle);
        f32 y0 = matrix->m01;
        f32 z0 = matrix->m02;
        matrix->m01 = cosine * y0 - z0 * sine;
        matrix->m02 = sine * y0 + z0 * cosine;
        f32 y1 = matrix->m11;
        f32 z1 = matrix->m12;
        matrix->m11 = cosine * y1 - z1 * sine;
        matrix->m12 = sine * y1 + z1 * cosine;
        f32 y2 = matrix->m21;
        f32 z2 = matrix->m22;
        matrix->m21 = cosine * y2 - z2 * sine;
        matrix->m22 = sine * y2 + z2 * cosine;
        f32 y3 = matrix->m31;
        f32 z3 = matrix->m32;
        matrix->m31 = cosine * y3 - z3 * sine;
        matrix->m32 = sine * y3 + z3 * cosine;
        sine = NU_SIN_LUT(y_angle);
        cosine = NU_COS_LUT(y_angle);
        f32 x0 = matrix->m00;
        z0 = matrix->m02;
        matrix->m00 = x0 * cosine + z0 * sine;
        matrix->m02 = z0 * cosine - x0 * sine;
        f32 x1 = matrix->m10;
        z1 = matrix->m12;
        matrix->m10 = x1 * cosine + z1 * sine;
        matrix->m12 = z1 * cosine - x1 * sine;
        f32 x2 = matrix->m20;
        z2 = matrix->m22;
        matrix->m20 = x2 * cosine + z2 * sine;
        matrix->m22 = z2 * cosine - x2 * sine;
        f32 x3 = matrix->m30;
        z3 = matrix->m32;
        matrix->m30 = x3 * cosine + z3 * sine;
        matrix->m32 = z3 * cosine - x3 * sine;
    }

    void DebrisSetGroupID(void) {
    }

    void DebrisSetPriority(void) {
    }

    i32 debris_render_group;
    void DebrisSetRenderGroup(i32 group) {
        debris_render_group = group;
    }

    void DebrisSetRoomID(void) {
    }

    void DebrisSetSeed(i32 seed) {
        debrisseed = static_cast<u32>(seed);
    }

    void DebrisSetTrigger(i32 handle, i32 first, i32 second, i32 third) {
        if (handle != -1) {
            debkeydatatype_s &key = debkeydata[handle];
            key.trigger_third = third;
            key.trigger_first = first;
            key.trigger_second = second;
        }
    }

    void DebrisSetUserData(void) {
    }

    void DebrisShift(void) {
    }

    void DebrisStartOffsetEx(debkeydatatype_s *key, f32 offset) {
        if (key == NULL) {
            return;
        }
        const i16 effect_index = key->effect_index;
        debinftype *effect = debtab[effect_index];
        f32 now = effect->time_group == 4 ? panelglobaltime : globaltime;
        f32 start;
        f32 period;
        if (effect->emission_period_random == 0.0f && effect->emission_pause_random == 0.0f) {
            const f32 interval = effect->emission_period + effect->emission_pause;
            start = static_cast<f32>(static_cast<i32>(now / interval)) * interval;
            if (effect->generator_type == 7 && effect->emission_pause == 0.0f) {
                const f32 frames = offset * 60.0f;
                key->emitter_rotation_x = static_cast<i16>(static_cast<i32>(effect->field_050 * frames));
                key->emitter_rotation_y = static_cast<i16>(static_cast<i32>(effect->field_054 * frames));
            } else {
                start += offset;
            }
            start += interval;
            key->emission_time = start;
            while (now < start) {
                start -= interval;
            }
        } else {
            start = now;
            key->emission_time = start;
        }
        period = effect->emission_period;
        key->previous_emission_time = -10.0f;
        key->field_1e4 = NuRandFloatSeeded(&debrisseed) * effect->emission_period_random + start + period;
    }

    void DebrisStartOffset(i32 handle, f32 offset) {
        if (handle != -1) {
            DebrisStartOffsetEx(debkeydata + handle, offset);
        }
    }

    void DebrisStatusAlwaysOff(i32 *handle) {
        if (*handle != -1)
            debkeydata[*handle].field_2f4 = 0;
    }

    void DebrisStatusAlwaysOn(void) {
    }

    void DebrisStatusNormal(void) {
    }

    void DebrisTorusCollisionCheck(void) {
    }

    void DebrisTorusCollisionCheckFlag(void) {
    }

    void DebrisTorusCollisionCheckScaleY(void) {
    }

    void DebrisTorusCollisionCheckScaleYFlag(void) {
    }

    void DebrisTrashableSetup(VARIPTR *buffer, VARIPTR *) {
        if (debris_setup_called == 0) {
            return;
        }

        usize cursor = debris_trash_space;
        if (cursor == 0) {
            if (buffer == NULL) {
                return;
            }
            buffer->addr = ALIGN(buffer->addr, 0x10);
            cursor = buffer->addr;
        }

        const i32 chunk_count = debrischunks + debrischunksglass;
        memset(debris_chunk_controls, 0, static_cast<usize>(chunk_count) * 2 * sizeof(debris_chunk_control_s));
        memset(freechunkcontrols, 0, static_cast<usize>(chunk_count) * 2 * sizeof(debris_chunk_control_s *));
        memset(freedebchunks, 0, static_cast<usize>(debrischunks) * sizeof(dma_particle_chunk_s *));
        memset(freedebchunksglass, 0, static_cast<usize>(debrischunksglass) * sizeof(dma_particle_chunk_s *));
        memset(ParticleChunkToRender, 0, static_cast<usize>(chunk_count) * sizeof(particlechunkrendertype_s));

        for (i32 i = 0; i < debrischunks; ++i) {
            i32 size;
            freedebchunks[i] = CreateDmaParticleSet(reinterpret_cast<void *>(cursor), &size);
            cursor += static_cast<usize>(size);
        }
        for (i32 i = 0; i < debrischunksglass; ++i) {
            i32 size;
            freedebchunksglass[i] = CreateDmaParticleSetGlass(reinterpret_cast<void *>(cursor), &size);
            cursor += static_cast<usize>(size);
        }

        freedebchkptr = 0;
        freedebchkptrg = 0;
        for (i32 i = 0; i < EDPP_MAX_DMADEBTYPES; ++i) {
            i32 size;
            DmaDebTypes[i] = CreateDmaPartEffectList(reinterpret_cast<void *>(cursor), &size);
            cursor += static_cast<usize>(size);
        }

        if (debris_trash_space == 0) {
            debris_trash_space = buffer->addr;
            debris_trash_size = cursor - buffer->addr;
            buffer->addr = cursor;
        }

        debris_chunk_control_stack[0] = NULL;
        debris_chunk_control_stack[1] = NULL;
        for (i32 i = 0; i < chunk_count * 2; ++i) {
            freechunkcontrols[i] = &debris_chunk_controls[i];
        }
        freechunkcontrolsptr = 0;
        for (i32 i = 0; i < chunk_count; ++i) {
            ParticleChunkToRender[i].particle_chunk = NULL;
            ParticleChunkToRender[i].effect = NULL;
            ParticleChunkToRender[i].key = NULL;
            ParticleChunkToRender[i].previous = NULL;
            ParticleChunkToRender[i].next = NULL;
        }
        ParticleChunkRenderStack[0] = NULL;
        ParticleChunkRenderStack[1] = NULL;
        ParticleChunkRenderStack[2] = NULL;
        ParticleChunkRenderStack[3] = NULL;
        ParticleChunkRenderStack[4] = NULL;
    }

    void DebrisTypeStatusAlwaysOff(i32 type) {
        if (type != -1)
            debtab[type]->status = 0;
    }

    void DebrisTypeStatusAlwaysOn(i32 type) {
        if (type != -1)
            debtab[type]->status = 2;
    }

    void DebrisTypeStatusNormal(i32 type) {
        if (type != -1)
            debtab[type]->status = 1;
    }

    i32 DeletePlatinst(i32 index) {
        if (index < 0 || index >= CurTerr->max_platforms)
            return 0;
        TERRAIN_PLATFORM &platform = CurTerr->platforms[index];
        if (platform.scene_object == NULL)
            return 0;
        const i16 last_group = --CurTerr->group_count;
        --CurTerr->group_index_count;
        TERRAIN_GROUP &last = CurTerr->groups[last_group];
        CurTerr->platforms[last.scene_index].terrain_group_index = platform.terrain_group_index;
        CurTerr->groups[platform.terrain_group_index] = last;
        last.chunk_type = -1;
        platform.scene_object = NULL;
        TERRAIN_CELL &cell = CurTerr->cells[TERRAIN_PLATFORM_CELL];
        i16 *indices = CurTerr->group_indices + cell.first_group;
        for (i32 i = 0; i < cell.group_count; ++i) {
            if (indices[i] == last_group) {
                indices[i] = CurTerr->group_indices[CurTerr->group_index_count];
                break;
            }
        }
        --cell.group_count;
        ScanTerrIDRemovePlat(index);
        return 1;
    }

    void DrawHitTerrain(void) {
    }

    void DrawPlatform(void) {
    }

    i32 FindPlatInst(i32 instance) {
        if (instance != -1) {
            for (i32 i = 0; i < CurTerr->max_platforms; ++i) {
                TERRAIN_PLATFORM *platform = &CurTerr->platforms[i];
                if (platform->scene_object != NULL && static_cast<i16>(platform->scene_object_index) == instance)
                    return i;
            }
        }
        return -1;
    }

    void NewMSituTerrEx(void) {
    }

    i32 NewPlatInst(void *object, i32 instance) {
        if (CurTerr == NULL || CurTerr->group_index_count >= CurTerr->max_group_indices ||
            CurTerr->group_count >= CurTerr->max_groups || object == NULL || CurTerr->max_platforms <= 0)
            return -1;
        i32 index = 0;
        while (CurTerr->platforms[index].scene_object != NULL) {
            if (++index == CurTerr->max_platforms)
                return -1;
        }
        for (i32 source = 0; source < CurTerr->max_platforms; ++source) {
            TERRAIN_PLATFORM &original = CurTerr->platforms[source];
            if (original.scene_object == NULL || static_cast<i16>(original.scene_object_index) != instance)
                continue;
            const i16 group_index = CurTerr->group_count;
            TERRAIN_GROUP &group = CurTerr->groups[group_index];
            group = CurTerr->groups[original.terrain_group_index];
            group.scene_index = index;
            group.chunk_type = 1;
            TERRAIN_PLATFORM &platform = CurTerr->platforms[index];
            platform.scene_object = object;
            platform.terrain_group_index = group_index;
            platform.scene_object_index = instance;
            platform.scene_transform = NULL;
            platform.flags = (platform.flags & ~1) | (original.flags & 1);
            CurTerr->group_indices[CurTerr->group_index_count++] = group_index;
            platform.bounce_impulse = 0.0f;
            platform.bounce_offset = 0.0f;
            platform.bounce_velocity = 0.0f;
            platform.bounce_damping = 0.0f;
            platform.bounce_spring = 0.0f;
            ++CurTerr->group_count;
            ++CurTerr->cells[TERRAIN_PLATFORM_CELL].group_count;
            return index;
        }
        return -1;
    }

    void NewPlatInstMSitu(void) {
    }

    void AddPickupTerr(i32 type, NUVEC *position) {
        if (CurTerr == NULL)
            return;
        i16 source = PickupTerr->group_for_type[type];
        if (source == -1 || curPickInst >= 128)
            return;
        TERRAIN_GROUP *group = &CurTerr->groups[CurTerr->max_groups + curPickInst];
        *group = PickupTerr->groups[source];
        group->origin = *position;
        ++curPickInst;
    }

    i32 AddPickupTerrRot(i32 type, NUMTX *matrix, NUMTX *previous, i32 rotating) {
        TERRSET *terrain = CurTerr;
        if (terrain == NULL)
            return -1;
        if (rotating == 0 && previous == NULL) {
            AddPickupTerr(type, reinterpret_cast<NUVEC *>(&matrix->m30));
            return -1;
        }
        if (terrain->group_index_count >= terrain->max_group_indices || terrain->group_count >= terrain->max_groups ||
            matrix == NULL || (rotating != 0 && previous == NULL))
            return -1;
        i16 source = PickupTerr->group_for_type[type];
        if (source == -1 || terrain->removed_platform_count >= 32 || terrain->max_platforms <= 0)
            return -1;
        i32 index = 0;
        while (terrain->platforms[index].scene_object != NULL) {
            if (++index == terrain->max_platforms)
                return -1;
        }
        i16 group_index = terrain->group_count;
        TERRAIN_GROUP &group = terrain->groups[group_index];
        group = PickupTerr->groups[source];
        group.scene_index = index;
        group.chunk_type = 1;
        TERRAIN_PLATFORM &platform = terrain->platforms[index];
        platform.scene_object = matrix;
        if (rotating != 0) {
            platform.previous_matrix = *previous;
            platform.flags |= TERRAIN_PLATFORM_FLAG_ROTATING;
        } else {
            NUMTX *old = previous != NULL ? previous : matrix;
            platform.previous_matrix.m30 = old->m30;
            platform.previous_matrix.m31 = old->m31;
            platform.previous_matrix.m32 = old->m32;
            platform.flags &= ~TERRAIN_PLATFORM_FLAG_ROTATING;
        }
        group.origin.x = matrix->m30;
        group.origin.y = matrix->m31;
        group.origin.z = matrix->m32;
        platform.terrain_group_index = group_index;
        platform.scene_object_index = 0;
        platform.scene_transform = NULL;
        terrain->group_indices[terrain->group_index_count++] = group_index;
        platform.flags &= ~TERRAIN_PLATFORM_FLAG_COLLIDED;
        platform.bounce_impulse = 0.0f;
        platform.bounce_offset = 0.0f;
        platform.bounce_velocity = 0.0f;
        platform.bounce_damping = 0.0f;
        platform.bounce_spring = 0.0f;
        if (terrain->active_platform_count < 96)
            terrain->active_platform_groups[terrain->active_platform_count++] = terrain->group_count;
        ++terrain->cells[TERRAIN_PLATFORM_CELL].group_count;
        ++terrain->group_count;
        terrain->removed_platforms[terrain->removed_platform_count++] = index;
        return index;
    }

    i16 NewPlatPickupInst(void *object, i32 type) {
        TERRSET *terrain = CurTerr;
        if (terrain == NULL || terrain->group_index_count >= terrain->max_group_indices ||
            terrain->group_count >= terrain->max_groups || object == NULL)
            return -1;
        i16 source = PickupTerr->group_for_type[type];
        if (source == -1 || terrain->max_platforms <= 0)
            return -1;
        i32 index = 0;
        while (terrain->platforms[index].scene_object != NULL) {
            if (++index == terrain->max_platforms)
                return -1;
        }
        i16 group_index = terrain->group_count;
        TERRAIN_GROUP &group = terrain->groups[group_index];
        group = PickupTerr->groups[source];
        group.scene_index = index;
        group.chunk_type = 1;
        TERRAIN_PLATFORM &platform = terrain->platforms[index];
        platform.scene_object = object;
        platform.flags &= ~TERRAIN_PLATFORM_FLAG_ROTATING;
        NUMTX *matrix = static_cast<NUMTX *>(object);
        group.origin.x = matrix->m30;
        group.origin.y = matrix->m31;
        group.origin.z = matrix->m32;
        platform.terrain_group_index = group_index;
        platform.scene_object_index = 0;
        platform.scene_transform = NULL;
        terrain->group_indices[terrain->group_index_count++] = group_index;
        platform.flags &= ~TERRAIN_PLATFORM_FLAG_COLLIDED;
        platform.bounce_impulse = 0.0f;
        platform.bounce_offset = 0.0f;
        platform.bounce_velocity = 0.0f;
        platform.bounce_damping = 0.0f;
        platform.bounce_spring = 0.0f;
        ++terrain->group_count;
        ++terrain->cells[TERRAIN_PLATFORM_CELL].group_count;
        return index;
    }

    i32 NewRayCast(NUVEC *position, NUVEC *movement, f32 radius, i32 scan_flags) {
        plathitid = -1;
        TerrPolyObj = -1;
        TerrPoly = NULL;
        TerrWallInfo = 0;
        castnum = -1;
        if (CurTerr == NULL)
            return 0;
        TerI = static_cast<TerrainQuery_s *>(NuScratchAlloc32(0x948));
        TerrOverRideScan = NULL;
        TerrainQuery_s *query = TerI;
        query->object_scale = 1.0f;
        query->object_scale_sq = 1.0f;
        query->inverse_object_scale = 1.0f;
        query->inverse_object_scale_sq = 1.0f;
        query->collision_radius = radius;
        query->inverse_collision_radius = radius == 0.0f ? 0.0f : 1.0f / radius;
        query->collision_radius_sq = radius * radius;
        query->start_position.x = query->position.x = position->x;
        query->start_position.y = query->position.y = position->y;
        query->start_position.z = query->position.z = position->z;
        query->movement = *movement;
        query->start_movement = query->movement;
        query->object_index = -1;
        query->hit_flags = NULL;
        query->scan_result = 1;
        query->separation_epsilon = 0.01f;
        query->compare_epsilon = 0.00001f;
        ScanTerrain(1, 0, scan_flags != 0 ? 0x40 : 0);
        DerotateMovementVector();
        HitTerrain();
        if (TerI->hit_type != 0) {
            RayImpact(movement);
            TerrainImpactNorm();
            ShadNorm = TerI->movement_normal;
        }
        NuScratchRelease();
        return TerI->hit_type;
    }

    void NewRayCastEx(void) {
    }

    f32 NewRayCastGetEmbedDist(void) {
        return TerI->unclamped_hit_time;
    }

    void NewRayCastGetImpactNormal(NUVEC *normal) {
        if (TerI != NULL)
            *normal = TerI->movement_normal;
    }

    i32 NewRayCastGetImpactTerrainType(void) {
        return TerI->surface != NULL ? TerI->surface->material[0] : 0;
    }

    f32 NewRayCastGetTOFI(void) {
        return TerI->hit_time;
    }

    i32 NewRayCastHitWallSpline(void) {
        return TerI->shape_adjusted;
    }

    void NewRayCastMask(void) {
    }

    void NewRayCastPlatForm(void) {
    }

    void NewRayCastScaleY(void) {
    }

    i32 NewRayCastScaleYMask(NUVEC *, NUVEC *, f32, f32, i32, u32) {
        return 0;
    }

    void NewRayCastSet(void) {
    }

    void NewRayCastSetHandel(void) {
    }

    void NewRayCastSetMask(void) {
    }

    void NewScanHandel(void) {
    }

    f32 NewShadowEx(NUVEC *position, i32 handle, f32 height_above, f32 height_below, i32 terrain_mask);

    f32 NewShadow(NUVEC *position, f32 height_above, f32 height_below, i32 terrain_mask) {
        return NewShadowEx(position, 0, height_above, height_below, terrain_mask);
    }

    f32 NewShadowEx(NUVEC *position, i32, f32 height_above, f32 height_below, i32 terrain_mask) {
        TerrPolyObj = -1;
        castnum = -1;
        ecastnum = -1;
        EShadRoofPoly = NULL;
        EShadRoofY = 2000000.0f;
        eshadhit = 0;
        EShadPoly = NULL;
        ShadRoofPoly = NULL;
        shadhit = 0;
        ShadPoly = 0;
        EShadY = 2000000.0f;
        EShadNorm.y = 1.0f;
        ShadRoofY = 2000000.0f;
        ShadNorm.y = 1.0f;

        f32 shadow_height = 2000000.0f;
        if (CurTerr != NULL) {
            TerI = static_cast<TerrainQuery_s *>(NuScratchAlloc32(sizeof(TerrainQuery_s)));
            ScaleTerrain = static_cast<TERRAIN_SHAPE *>(ScaleTerrainT1);
            NUVEC scan_position = *position;
            NewScanRot(&scan_position, terrain_mask);
            NewCast(&scan_position, height_above, height_below);
            NuScratchRelease();
            shadow_height = scan_position.y;
            NuTerrPlatsOff = 0;
        }
        return shadow_height;
    }

    void NewShadowHandelEx(void) {
    }

    i32 NewShadowOnMSitu(void) {
        return castnum != -1 ? castnum : TerrPolyObj;
    }

    i32 NewShadowOnPlatform(void) {
        if (CurTerr == NULL || castnum == -1) {
            return -1;
        }

        TERRAIN_GROUP &group = CurTerr->groups[castnum];
        if (group.chunk_type != TERRAIN_CHUNK_GROUP_SECONDARY) {
            return -1;
        }
        return group.scene_index;
    }

    void NewTerrHitInfo(u8 *info) {
        info[0] = TerrainHitInfo[0];
        info[1] = TerrainHitInfo[1];
        info[2] = TerrainHitInfo[2];
        info[3] = TerrainHitInfo[3];
    }

    void NewTerrPlatformsOff(void) {
        NuTerrPlatsOff = 1;
    }

    void ShadowDir(NUVEC *direction) {
        if (ShadPoly != NULL) {
            direction->x = ShadPoly->vectors[1].x - ShadPoly->vectors[0].x;
            direction->y = ShadPoly->vectors[1].y - ShadPoly->vectors[0].y;
            direction->z = ShadPoly->vectors[1].z - ShadPoly->vectors[0].z;
        }
    }

    i32 ShadowRoofInfo(void) {
        return ShadRoofPoly != NULL ? ShadRoofPoly->material[0] : -1;
    }

    void NewTerrain(void) {
    }

    i32 NewTerrainOnAPlatform(void *id) {
        if (CurTerr != NULL) {
            CurTrackInfo = ScanTerrId(id);
            if (CurTrackInfo != NULL)
                return CurTrackInfo->platform_contact_state;
        }
        return 0;
    }

    void NewTerrainScaleY(void) {
    }

    void NewTerrainScaleYMask(NUVEC *position, NUVEC *movement, u8 *hit_flags, i32 object_index, f32 radius,
                              f32 collision_radius, f32 object_scale, i32 embedded_retry, i32 scan_flags,
                              i32 terrain_mask) {
        TerrainHitInfo[0] = 0;
        TerrainHitInfo[1] = 0;
        TerrainHitInfo[2] = 0;
        TerrainHitInfo[3] = 0;
        TerrImpact = 0;
        plathitid = -1;
        TerrPolyObj = -1;
        TerrPoly = 0;
        TerrWallInfo = 0;
        PlatCrush = 0;
        terrhitflags = 0;
        castnum = -1;

        if (CurTerr == NULL) {
            TerImpactData = 0;
            return;
        }

        TerrLastImpact.hit_type = 0.0f;
        CurTrackInfo = ScanTerrId(hit_flags);
        TerrOverRideScan = 0;

        TerI = static_cast<TerrainQuery_s *>(NuScratchAlloc32(0x948));
        TerrainQuery_s *query = TerI;
        query->object_scale = object_scale;
        query->object_scale_sq = object_scale * object_scale;
        if (object_scale == 0.0f) {
            query->inverse_object_scale = 0.0f;
            query->inverse_object_scale_sq = 0.0f;
        } else {
            query->inverse_object_scale = 1.0f / object_scale;
            query->inverse_object_scale_sq = query->inverse_object_scale * query->inverse_object_scale;
        }
        query->collision_radius = collision_radius;
        query->inverse_collision_radius = collision_radius == 0.0f ? 0.0f : 1.0f / collision_radius;
        query->collision_radius_sq = collision_radius * collision_radius;

        const f32 position_x = position->x;
        const f32 position_y = position->y + collision_radius * object_scale;
        const f32 position_z = position->z;
        query->position.x = position_x;
        query->start_position.x = position_x;
        query->position.y = position_y;
        query->start_position.y = position_y;
        query->position.z = position_z;
        query->start_position.z = position_z;

        const f32 movement_x = movement->x;
        const f32 movement_y = movement->y;
        const f32 movement_z = movement->z;
        query->movement.x = movement_x;
        query->start_movement.x = movement_x;
        query->movement.y = movement_y;
        query->start_movement.y = movement_y;
        query->movement.z = movement_z;
        query->start_movement.z = movement_z;
        query->object_index = static_cast<i16>(object_index);
        query->flags &= static_cast<u8>(~1u);
        query->hit_flags = hit_flags;
        query->radius = radius;
        query->scan_result = 0;
        query->separation_epsilon = 0.005f;
        query->compare_epsilon = 0.000005f;

        castnum = -1;
        ScanTerrain(1, terrain_mask, scan_flags != 0 ? 0x40 : 0);

        if (hit_flags[1] != 0 && radius > fabsf(movement->x) && radius > fabsf(movement->y) &&
            radius > fabsf(movement->z) && platinrange == 0) {
            NuScratchRelease();
            TerrFlush();
            ShadNorm.x = 0.0f;
            ShadNorm.y = 1.0f;
            ShadNorm.z = 0.0f;
            terrhitflags |= 2;
            TerImpactData = 0;
            return;
        }

        query->position.y *= query->inverse_object_scale;
        query->movement.y *= query->inverse_object_scale;
        hit_flags[0] = 0;
        hit_flags[1] = 0;

        i32 scan_count = PlatformChecks(6, movement);
        for (;;) {
            DerotateMovementVector();
            HitTerrain();
            TerrainImpactNorm();
            StorePlatImpact();

            query = TerI;
            u8 hit_type = query->hit_type;
            bool impact_already_resolved = false;
            if (hit_type > TERRAIN_HIT_TYPE_SECOND_NORMAL && query->terrain_group_index != -1 &&
                CurTerr->groups != NULL) {
                TERRAIN_GROUP *group = &CurTerr->groups[query->terrain_group_index];
                if (group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY) {
                    --scan_count;
                    NewTerrStoreAnyInfo();
                    NUVEC before = query->position;
                    i32 embedded = TerrainPlatformEmbedded(movement);
                    query = TerI;
                    impact_already_resolved =
                        embedded == 0 && (before.x != query->position.x || before.y != query->position.y ||
                                          before.z != query->position.z);
                    hit_type = query->hit_type;
                }
            }

            if (!impact_already_resolved) {
                if (query->hit_type != TERRAIN_HIT_TYPE_NONE && query->terrain_group_index >= 0 &&
                    CurTerr->groups != NULL) {
                    TERRAIN_GROUP *group = &CurTerr->groups[query->terrain_group_index];
                    ShadNorm.x = query->impact_normal.x;
                    ShadNorm.y = query->impact_normal.y;
                    ShadNorm.z = query->impact_normal.z;

                    f32 slope = 0.707f;
                    if (query->surface != NULL) {
                        slope = query->movement_normal.x * query->surface->normals[0].x +
                                query->movement_normal.y * query->surface->normals[0].y +
                                query->movement_normal.z * query->surface->normals[0].z;
                    }

                    const u8 hit_class = query->hit_type & TERRAIN_HIT_TYPE_CLASS_MASK;
                    f32 wall_limit;
                    if (group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY) {
                        wall_limit = hit_class > TERRAIN_HIT_TYPE_FACE && 0.95f > slope ? 0.98f : 0.707f;
                    } else {
                        wall_limit = query->shape_adjusted != 0 ? 1.1f : -1.1f;
                    }

                    if (wallover != 0.0f) {
                        wall_limit = wallover;
                    }
                    if (hit_class != TERRAIN_HIT_TYPE_FACE) {
                        wall_limit = 0.707f;
                    }
                    if (query->surface != NULL && (query->surface->normal_flags & TERRAIN_SURFACE_CLASS_MASK) ==
                                                      TERRAIN_SURFACE_CLASS_WALL_OVERRIDE) {
                        query->position.x += query->movement_normal.x * 0.001f;
                        query->position.z += query->movement_normal.z * 0.001f;
                        wall_limit = 1.1f;
                    }

                    if (query->impact_normal.y >= wall_limit && group->chunk_type == TERRAIN_CHUNK_GROUP_SECONDARY &&
                        CurTerr->platforms != NULL) {
                        CurTerr->platforms[group->scene_index].flags |= TERRAIN_PLATFORM_FLAG_COLLIDED;
                    }
                }

                if (TerrShapeAdjCnt == 0 || TerrShapeSideStep(position, movement, hit_flags) != 0) {
                    TerrainImpact(position, movement, hit_flags);
                }

                query = TerI;
                --scan_count;
                TerrLastImpact.position.x = query->position.x - query->movement_normal.x * query->collision_radius;
                TerrLastImpact.position.y =
                    (query->position.y - query->movement_normal.y * query->collision_radius) * query->object_scale;
                TerrLastImpact.position.z = query->position.z - query->movement_normal.z * query->collision_radius;
                TerrLastImpact.hit_type = static_cast<f32>(static_cast<u32>(query->hit_type));
                hit_type = query->hit_type;
            }

            if (hit_type == TERRAIN_HIT_TYPE_NONE) {
                if (scan_count > 3 && hit_flags[0] == 0 && hit_flags[1] == 0 && embedded_retry != 0) {
                    query->position.x = position->x;
                    query->position.y = position->y * query->inverse_object_scale + query->collision_radius + 0.003f;
                    query->position.z = position->z;
                    query->movement.x = 0.0f;
                    query->movement.y = -0.007f;
                    query->movement.z = 0.0f;
                    DerotateMovementVector();
                    HitTerrain();
                    query = TerI;
                    if (query->hit_type != TERRAIN_HIT_TYPE_NONE &&
                        (query->terrain_group_index < 0 || query->surface == NULL ||
                         (query->surface->normal_flags & TERRAIN_SURFACE_CLASS_MASK) !=
                             TERRAIN_SURFACE_CLASS_WALL_OVERRIDE)) {
                        TerrainImpactNorm();
                        ShadNorm.x = query->impact_normal.x;
                        ShadNorm.y = query->impact_normal.y;
                        ShadNorm.z = query->impact_normal.z;
                        query->start_movement.x = movement->x;
                        query->start_movement.y = movement->y;
                        query->start_movement.z = movement->z;
                        TerrainImpact(position, movement, hit_flags);
                        query = TerI;
                        position->x = query->position.x;
                        position->y =
                            query->position.y * query->object_scale - query->collision_radius * query->object_scale;
                        position->z = query->position.z;
                        movement->x = query->start_movement.x;
                        movement->y = query->start_movement.y;
                        movement->z = query->start_movement.z;
                    }
                }
                break;
            }

            TerrImpact = query->surface == NULL ? 2 : 1;
            TerrImpactPos.x = query->position.x - query->movement_normal.x * query->collision_radius;
            TerrImpactPos.y =
                (query->position.y - query->movement_normal.y * query->collision_radius) * query->object_scale;
            TerrImpactPos.z = query->position.z - query->movement_normal.z * query->collision_radius;
            TerrImpactNormal = query->impact_normal;

            if (scan_count > 0) {
                const f32 normal_mag_sq = query->movement_normal.x * query->movement_normal.x +
                                          query->movement_normal.y * query->movement_normal.y +
                                          query->movement_normal.z * query->movement_normal.z;
                if (normal_mag_sq <= 1.5f) {
                    continue;
                }
            }

            position->x = query->position.x;
            position->y = query->position.y * query->object_scale - query->collision_radius * query->object_scale;
            position->z = query->position.z;
            break;
        }

        NuScratchRelease();
        TerrFlush();
        TerImpactData = 0;
    }

    void PartTerrInit(void) {
    }

    void PlatInstBounce(i32 index, f32 impulse, f32 spring, f32 damping) {
        if (index >= 0 && index < CurTerr->max_platforms) {
            TERRAIN_PLATFORM &platform = CurTerr->platforms[index];
            if (platform.bounce_impulse == 0.0f) {
                platform.bounce_offset = 0.0f;
                platform.bounce_velocity = 0.0f;
            }
            platform.bounce_impulse = impulse;
            platform.bounce_spring = spring;
            platform.bounce_damping = damping;
        }
    }

    void PlatInstCenter(i32 index, NUVEC *position) {
        if (CurTerr != NULL && index >= 0 && index < CurTerr->max_platforms)
            *position = CurTerr->groups[CurTerr->platforms[index].terrain_group_index].origin;
    }

    i32 PlatInstGetHit(i32 index) {
        if (CurTerr != NULL && index >= 0 && index < CurTerr->max_platforms)
            return (CurTerr->platforms[index].flags >> 1) & 1;
        return 0;
    }

    void PlatInstRotate(i32 index, i32 rotate) {
        if (index >= 0 && index < CurTerr->max_platforms) {
            TERRAIN_PLATFORM &platform = CurTerr->platforms[index];
            platform.flags = (platform.flags & ~1) | (rotate & 1);
        }
    }

    i32 PlatInstSkinRegisterEx(NUMTX *matrix, void *skin_data, void *matrix_data, f32 scale, i32 instance, i32 flags,
                               TERRSET *source) {
        TERRSET *terrain = CurTerr;
        if (terrain == NULL || source == NULL)
            return -1;
        i32 group_index = terrain->group_count;
        if (group_index >= terrain->max_groups)
            return -3;
        if (skin_data == NULL || matrix == NULL)
            return -4;
        if (PlatSkinCnt >= PlatSkinMax)
            return -5;
        TERRAIN_PLATFORM *platform = source->platforms;
        i32 i;
        for (i = 0; i < source->max_platforms; ++i, ++platform) {
            if (platform->scene_object != NULL && static_cast<i16>(platform->scene_object_index) == instance)
                break;
        }
        if (i == source->max_platforms || source->max_platforms <= 0)
            return -6;
        TERRAIN_GROUP &group = terrain->groups[group_index];
        group = source->groups[platform->terrain_group_index];
        group.chunk_type = 0;
        PLATSKININFO *info = &PlatSkinInfo[PlatSkinCnt];
        info->matrix = matrix;
        info->skin_data = skin_data;
        info->matrix_data = matrix_data;
        info->scale = scale;
        info->flags = flags;
        info->mirrored = matrix->m02 * matrix->m20 - matrix->m00 * matrix->m22 > 0.0f;
        SkinPlatformSize(group_index, PlatSkinMem, info);
        info = &PlatSkinInfo[PlatSkinCnt];
        info->terrain_group = CurTerr->group_count;
        TERRAIN_GROUP &registered_group = CurTerr->groups[CurTerr->group_count];
        info->terrain_data = registered_group.data;
        registered_group.data = NULL;
        registered_group.scene_index = ~PlatSkinCnt;
        ++CurTerr->group_count;
        ++PlatSkinCnt;
        return CurTerr->group_count - 1;
    }

    i32 PlatInstSkinRegister(NUMTX *matrix, void *skin_data, void *matrix_data, f32 scale, i32 instance, i32 flags) {
        return PlatInstSkinRegisterEx(matrix, skin_data, matrix_data, scale, instance, flags, CurTerr);
    }

    void PlatOnOff(i32 index, i32 enabled) {
        if (CurTerr != NULL && index >= 0 && index < CurTerr->max_platforms) {
            TERRAIN_GROUP &group = CurTerr->groups[CurTerr->platforms[index].terrain_group_index];
            if (enabled != 0) {
                group.chunk_type = 1;
                return;
            }
            group.chunk_type = -1;
        }
    }

    void *PlatSkinEndReigster(i32 cache_slots) {
        TERRSET *terrain = CurTerr;
        i16 *group_cells = reinterpret_cast<i16 *>(reinterpret_cast<uintptr_t>(PlatSkinMemEnd) & ~uintptr_t(1));
        group_cells -= terrain->max_groups;
        f32 *group_min_x = reinterpret_cast<f32 *>(reinterpret_cast<uintptr_t>(group_cells) & ~uintptr_t(3));
        group_min_x -= terrain->max_groups;
        f32 *group_max_x = group_min_x - terrain->max_groups;
        f32 *group_min_z = group_max_x - terrain->max_groups;
        f32 *group_max_z = group_min_z - terrain->max_groups;
        TERRAIN_GROUP *groups = terrain->groups;
        f32 minimum_x = 200000000.0f, minimum_z = 200000000.0f;
        f32 maximum_x = -200000000.0f, maximum_z = -200000000.0f;
        for (i32 i = 0; i < terrain->group_count; ++i) {
            TERRAIN_GROUP &group = groups[i];
            if (group.chunk_type == 0) {
                minimum_x = MIN(group.bounds_min.x, minimum_x);
                minimum_z = MIN(group.bounds_min.z, minimum_z);
                maximum_x = MAX(group.bounds_max.x, maximum_x);
                maximum_z = MAX(group.bounds_max.z, maximum_z);
            } else if (group.chunk_type == 1) {
                if (minimum_x > group.bounds_min.x)
                    minimum_x = group.origin.x + group.bounds_min.x;
                if (minimum_z > group.bounds_min.z)
                    minimum_z = group.origin.z + group.bounds_min.z;
                if (maximum_x < group.bounds_max.x)
                    maximum_x = group.origin.x + group.bounds_max.x;
                if (maximum_z < group.bounds_max.z)
                    maximum_z = group.origin.z + group.bounds_max.z;
            }
        }
        terrain->group_index_count = 0;
        for (i32 i = 0; i < terrain->group_count; ++i) {
            TERRAIN_GROUP &group = groups[i];
            if (group.chunk_type != 0)
                continue;
            group_min_x[i] = MIN(group.bounds_min.x, 200000000.0f);
            group_min_z[i] = MIN(group.bounds_min.z, 200000000.0f);
            group_max_x[i] = MAX(group.bounds_max.x, -200000000.0f);
            group_max_z[i] = MAX(group.bounds_max.z, -200000000.0f);
            i32 x_distance = static_cast<i32>((group_min_x[i] + group_max_x[i]) * 0.5f - minimum_x);
            i32 x_cell = static_cast<i32>(static_cast<f32>(x_distance * 7) / (maximum_x - minimum_x));
            if (x_cell < 0)
                x_cell = 0;
            else if (x_cell > 6)
                x_cell = 6;
            i32 z_distance = static_cast<i32>((group_min_z[i] + group_max_z[i]) * 0.5f - minimum_z);
            i32 z_cell = static_cast<i32>(static_cast<f32>(z_distance * 7) / (maximum_z - minimum_z));
            if (z_cell < 0)
                z_cell = 0;
            else if (z_cell > 6)
                z_cell = 6;
            group_cells[i] = x_cell + z_cell * 7;
        }
        for (i32 i = 0; i < TERRAIN_CELL_RECORD_COUNT; ++i)
            terrain->cells[i].group_count = 0;
        terrain->used_cell_count = 0;
        for (i32 cell_index = 0; cell_index < TERRAIN_GRID_CELL_COUNT; ++cell_index) {
            TERRAIN_CELL &cell = terrain->cells[terrain->used_cell_count];
            cell.first_group = terrain->group_index_count;
            f32 min_x = 200000000.0f, min_z = 200000000.0f;
            f32 max_x = -200000000.0f, max_z = -200000000.0f;
            for (i32 i = 0; i < terrain->group_count; ++i) {
                if (group_cells[i] != cell_index || groups[i].chunk_type != 0)
                    continue;
                min_x = MIN(group_min_x[i], min_x);
                min_z = MIN(group_min_z[i], min_z);
                max_x = MAX(group_max_x[i], max_x);
                max_z = MAX(group_max_z[i], max_z);
                ++cell.group_count;
                terrain->group_indices[terrain->group_index_count++] = i;
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
        platform_cell.first_group = terrain->group_index_count;
        TERRAIN_GROUP *group = groups;
        for (i32 i = 0; i < terrain->group_count; ++i) {
            // The original advances this pointer only for a secondary group.
            if (group->chunk_type != 1)
                continue;
            terrain->group_indices[terrain->group_index_count++] = i;
            ++platform_cell.group_count;
            terrain->platforms[group->scene_index].terrain_group_index = i;
            ++group;
        }
        SkinMemInfo = reinterpret_cast<PLATSKINMEMINFO *>(PlatSkinMem + cache_slots * PlatSkinMaxSize);
        for (i32 i = 0; i < cache_slots; ++i) {
            SkinMemInfo[i].last_used = 0;
            SkinMemInfo[i].skin_index = -1;
        }
        PlatSkinMaxStore = cache_slots;
        return SkinMemInfo + cache_slots;
    }

    void PlatSkinMemReset(void) {
        if (PlatSkinResetTotal >= 0) {
            CurTerr->group_count = PlatSkinResetTotal;
            for (i32 i = 0; i < 16; ++i)
                CurTerr->index_levels[i].entry_count = 0;
        }
    }

    void PlatSkinMemRigister(void *start, void *end, i32 maximum) {
        PlatSkinMax = maximum;
        PlatSkinInfo = static_cast<PLATSKININFO *>(start);
        PlatSkinMem = reinterpret_cast<u8 *>(PlatSkinInfo + maximum);
        PlatSkinMemEnd = static_cast<u8 *>(end);
        PlatSkinMaxSize = 0;
        PlatSkinCnt = 0;
        PlatSkinResetTotal = CurTerr->group_count;
    }

    i32 PlatformCrush(void) {
        return PlatCrush;
    }

    void PlatformRemoveCallback(void (*function)(void *)) {
        for (i32 i = 0; i < PlatCodeCallback; ++i) {
            if (PlatCallback[i].function == function) {
                --PlatCodeCallback;
                PlatCallback[i].function = PlatCallback[PlatCodeCallback].function;
                PlatCallback[i].argument = PlatCallback[PlatCodeCallback].argument;
                --i;
            }
        }
    }

    void PlatformUpdateCallback(void (*function)(void *), void *argument) {
        if (PlatCodeCallback < 8) {
            PlatCallback[PlatCodeCallback].function = function;
            PlatCallback[PlatCodeCallback].argument = argument;
            ++PlatCodeCallback;
        }
    }

    void SortDebrisRenderStack(void) {
    }

    void TerrDrawImpactPol(void) {
    }

    const char *TerrErrorString(i32 error) {
        static const char *const errors[] = {"ERR_UNKNOWN", "ERR_NOTERR",      "ERR_MAXTERLIST", "ERR_MAXTERR",
                                             "ERR_INOUT",   "ERR_PLATSKINMAX", "ERR_NOINSTANCE"};
        i32 index = -error;
        if (index >= 7)
            index = 0;
        return errors[index];
    }

    void TerrainAddWallSpline(TERRAIN_SPATIAL_NODE *node, TERRSET *terrain) {
        reinterpret_cast<TERRAIN_SPATIAL_NODE **>(node)[-1] = terrain->spatial_nodes;
        terrain->spatial_nodes = node;
    }

    i32 TerrainFreeId(void *id) {
        if (CurTerr != NULL) {
            for (i32 i = 0; i < 64; ++i) {
                if (CurTerr->track_slots[i].id == id) {
                    CurTerr->track_slots[i].id = NULL;
                    return 1;
                }
            }
        }
        return 0;
    }

    TERRSET *TerrainGetCur(void) {
        return CurTerr;
    }

    TERRAIN_GROUP *TerrainGetModelByInst(i32 instance) {
        if (CurTerr != NULL && CurTerr->max_platforms > 0) {
            TERRAIN_PLATFORM *platform = CurTerr->platforms;
            for (i32 i = 0; i < CurTerr->max_platforms; ++i, ++platform) {
                if (platform->scene_object != NULL && static_cast<i16>(platform->scene_object_index) == instance)
                    return &CurTerr->groups[platform->terrain_group_index];
            }
        }
        return NULL;
    }

    void TerrainPlatGetMtx(i32 index, NUMTX **previous, NUMTX **current) {
        if (index >= 0) {
            *previous = &CurTerr->platforms[index].previous_matrix;
            *current = static_cast<NUMTX *>(CurTerr->platforms[index].scene_object);
        }
    }

    i32 TerrainPlatId(void) {
        return plathitid;
    }

    void TerrainPolyEdge(NUVEC *first, NUVEC *second) {
        if ((TerI->hit_type & 0xf) != TERRAIN_HIT_TYPE_CYLINDER)
            return;
        switch (TerI->hit_edge) {
            case 1: {
                NUVEC *origin = &CurTerr->groups[TerI->terrain_group_index].origin;
                TERRAIN_SHAPE *surface = TerI->surface;
                first->x = origin->x + surface->vectors[0].x;
                first->y = origin->y + surface->vectors[0].y;
                first->z = origin->z + surface->vectors[0].z;
                second->x = origin->x + surface->vectors[1].x;
                second->y = origin->y + surface->vectors[1].y;
                second->z = origin->z + surface->vectors[1].z;
                break;
            }
            case 2: {
                NUVEC *origin = &CurTerr->groups[TerI->terrain_group_index].origin;
                TERRAIN_SHAPE *surface = TerI->surface;
                first->x = origin->x + surface->vectors[1].x;
                first->y = origin->y + surface->vectors[1].y;
                first->z = origin->z + surface->vectors[1].z;
                second->x = origin->x + surface->vectors[2].x;
                second->y = origin->y + surface->vectors[2].y;
                second->z = origin->z + surface->vectors[2].z;
                break;
            }
            case 3: {
                NUVEC *origin = &CurTerr->groups[TerI->terrain_group_index].origin;
                TERRAIN_SHAPE *surface = TerI->surface;
                first->x = origin->x + surface->vectors[2].x;
                first->y = origin->y + surface->vectors[2].y;
                first->z = origin->z + surface->vectors[2].z;
                second->x = origin->x + surface->vectors[0].x;
                second->y = origin->y + surface->vectors[0].y;
                second->z = origin->z + surface->vectors[0].z;
                break;
            }
            case 4: {
                NUVEC *origin = &CurTerr->groups[TerI->terrain_group_index].origin;
                TERRAIN_SHAPE *surface = TerI->surface;
                first->x = origin->x + surface->vectors[1].x;
                first->y = origin->y + surface->vectors[1].y;
                first->z = origin->z + surface->vectors[1].z;
                second->x = origin->x + surface->vectors[3].x;
                second->y = origin->y + surface->vectors[3].y;
                second->z = origin->z + surface->vectors[3].z;
                break;
            }
            case 5: {
                NUVEC *origin = &CurTerr->groups[TerI->terrain_group_index].origin;
                TERRAIN_SHAPE *surface = TerI->surface;
                first->x = origin->x + surface->vectors[3].x;
                first->y = origin->y + surface->vectors[3].y;
                first->z = origin->z + surface->vectors[3].z;
                second->x = origin->x + surface->vectors[2].x;
                second->y = origin->y + surface->vectors[2].y;
                second->z = origin->z + surface->vectors[2].z;
                break;
            }
            default:
                break;
        }
    }

    void TerrainRemoveWallSpline(TERRAIN_SPATIAL_NODE *node, TERRSET *terrain) {
        if (terrain == NULL)
            return;
        TERRAIN_SPATIAL_NODE *previous = terrain->spatial_nodes;
        if (previous == NULL)
            return;
        if (previous == node) {
            terrain->spatial_nodes = reinterpret_cast<TERRAIN_SPATIAL_NODE **>(previous)[-1];
            return;
        }
        TERRAIN_SPATIAL_NODE *next = reinterpret_cast<TERRAIN_SPATIAL_NODE **>(previous)[-1];
        while (next != NULL && next != node) {
            previous = next;
            next = reinterpret_cast<TERRAIN_SPATIAL_NODE **>(previous)[-1];
        }
        if (next == node)
            reinterpret_cast<TERRAIN_SPATIAL_NODE **>(previous)[-1] =
                reinterpret_cast<TERRAIN_SPATIAL_NODE **>(node)[-1];
    }

    void TerrainScanWallSpline(TERRAIN_SPATIAL_NODE *node) {
        if (node->point_count <= 2) {
            node->points[0].y = 2147483648.0f;
            return;
        }
        i32 count = node->point_count <= 14 ? node->point_count + 1 : 16;
        f32 first_x = node->points[0].x;
        f32 first_z = node->points[0].z;
        // The original Android binary repeats this block without advancing
        // the spline or terminating when point_count is greater than two.
        for (;;) {
            f32 min_x = first_x;
            f32 max_x = first_x;
            f32 min_z = first_z;
            f32 max_z = first_z;
            for (i32 i = 1; i < count; ++i) {
                f32 x = node->points[i].x;
                f32 z = node->points[i].z;
                min_x = x < min_x ? x : min_x;
                max_x = x > max_x ? x : max_x;
                min_z = z < min_z ? z : min_z;
                max_z = z > max_z ? z : max_z;
            }
            node->points[0].y = min_x;
            node->points[1].y = max_x;
            node->points[2].y = min_z;
            node->points[3].y = max_z;
        }
    }

    void TerrainSetImpactData(void *impact_data, i32 *impact_count, i32 maximum_impacts) {
        TerImpactData = impact_data;
        TerImpactDataCount = impact_count;
        *impact_count = 0;
        TerImpactDataMax = maximum_impacts;
    }

    void UpdateDebrisRenderStackPriority(void) {
    }

    void terraininit(void) {
    }

    void terrainpickupinit(char *path, void **buffer) {
        TERRPICKUPSET *terrain = static_cast<TERRPICKUPSET *>(*buffer);
        *buffer = static_cast<u8 *>(*buffer) + sizeof(TERRPICKUPSET);
        terrain->groups = static_cast<TERRAIN_GROUP *>(*buffer);
        *buffer = static_cast<u8 *>(*buffer) + 32 * sizeof(TERRAIN_GROUP);
        terrain->shape_data = *buffer;
        *buffer = static_cast<u8 *>(*buffer) + 0x1c00;
        memset(terrain->groups, 0, 32 * sizeof(TERRAIN_GROUP));
        for (i32 i = 0; i < 32; ++i)
            terrain->groups[i].chunk_type = -1;
        for (i32 i = 0; i < 32; ++i) {
            TERRAIN_PLATFORM &platform = terrain->platforms[i];
            platform.scene_object = NULL;
            platform.bounce_impulse = 0.0f;
            platform.bounce_offset = 0.0f;
            platform.bounce_velocity = 0.0f;
            platform.bounce_damping = 0.0f;
            platform.bounce_spring = 0.0f;
        }
        memset(terrain->group_for_type, 0xff, sizeof(terrain->group_for_type));
        curPickInst = 0;
        terraincnt = 0;
        curSphereter = 0;
        platinrange = 0;
        ShadPoly = NULL;
        TerrPoly = NULL;
        TerrWallInfo = 0;
        PlatImpactId = -1;
        terrain->group_count =
            ReadTerrainPickup(reinterpret_cast<unsigned char *>(path), reinterpret_cast<i16 **>(buffer), terrain);
        for (i32 i = 0; i < 32; ++i) {
            TERRAIN_GROUP &group = terrain->groups[i];
            if (static_cast<u32>(group.chunk_type) > 1)
                continue;
            NUVEC minimum = {200000000.0f, 200000000.0f, 200000000.0f};
            NUVEC maximum = {-200000000.0f, -200000000.0f, -200000000.0f};
            f32 radius_squared = 0.0f;
            TERRAIN_SHAPE_BATCH *batch = static_cast<TERRAIN_SHAPE_BATCH *>(group.data);
            while (batch->marker >= 0) {
                TERRAIN_SHAPE *shape = reinterpret_cast<TERRAIN_SHAPE *>(batch + 1);
                for (i32 j = 0; j < batch->shape_count; ++j, ++shape) {
                    if (shape->material[0] == 0xff) {
                        shape->material[0] = shape->material[1];
                        shape->material[1] = 0;
                        shape->flags |= 0x80;
                    }
                    minimum.x = MIN(shape->min_x, minimum.x);
                    minimum.y = MIN(shape->min_y, minimum.y);
                    minimum.z = MIN(shape->min_z, minimum.z);
                    maximum.x = MAX(shape->max_x, maximum.x);
                    maximum.y = MAX(shape->max_y, maximum.y);
                    maximum.z = MAX(shape->max_z, maximum.z);
                    for (i32 k = 0; k < 4; ++k) {
                        NUVEC &v = shape->vectors[k];
                        f32 length_squared = (v.x * v.x + v.y * v.y) + v.z * v.z;
                        radius_squared = MAX(length_squared, radius_squared);
                    }
                }
                batch = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(reinterpret_cast<TERRAIN_SHAPE *>(batch + 1) +
                                                                batch->shape_count);
            }
            group.radius = NuFsqrt(radius_squared);
            group.chunk_type = 0;
            ++terraincnt;
            group.bounds_min = minimum;
            group.bounds_max = maximum;
        }
        for (i32 i = 0; i < 32; ++i) {
            terrain->groups[i].scene_index = i;
            terrain->platforms[i].scene_object = NULL;
        }
        PickupTerr = terrain;
    }

    void UpdatePlatinst(i32 index, void *object) {
        if (index >= 0 && index < CurTerr->max_platforms)
            CurTerr->platforms[index].scene_object = object;
    }

} // extern "C"
