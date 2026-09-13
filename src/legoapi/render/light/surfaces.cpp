#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/glutils.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/numath/nutrig.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

NUMTL *SolidMtl;
NUMTL *SolidMtl3D;

extern "C" {
    u32 LAYER_HOVERIGNORE = 0xffffffff;
    u16 SURFACEBITS_DUST;
    u16 SURFACEBITS_NODUST;
    TERRAIN_SURFACE_s TerSurface[32] = {
        {1.0f, 0, 0xffff},     {1.0f, 65, 0xffff},   {1.0f, 2, 0xffff},      {1.0f, 8, 0xffff},   {1.0f, 65536, 0xffff},
        {1.0f, 66560, 0xffff}, {1.0f, 0, 0xffff},    {1.0f, 2048, 0xffff},   {1.0f, 0, 0xffff},   {1.0f, 8192, 0xffff},
        {1.0f, 10, 0xffff},    {1.0f, 67, 0xffff},   {1.0f, 16, 0xffff},     {1.0f, 16, 0xffff},  {1.0f, 8192, 0xffff},
        {1.0f, 8192, 0xffff},  {0.1f, 1088, 0xffff}, {0.2f, 131074, 0xffff}, {1.0f, 81, 0xffff},  {1.0f, 512, 0xffff},
        {1.0f, 8192, 0xffff},  {1.0f, 514, 0xffff},  {1.0f, 64, 0xffff},     {1.0f, 192, 0xffff}, {1.0f, 8192, 0xffff},
        {1.0f, 4160, 0xffff},  {1.0f, 0, 0xffff},    {1.0f, 5184, 0xffff},   {1.0f, 256, 0xffff}, {1.0f, 320, 0xffff},
        {1.0f, 64, 0xffff},    {1.0f, 64, 0xffff},
    };
}

extern TERRSET *CurTerr;
extern TerrainQuery_s *TerI;
extern NUVEC ShadNorm;

f32 GameShadow(GameObject_s *object, NUVEC *position, f32 probe_height, i32 terrain_mask);
f32 FindReflectionNoPlatforms(NUVEC *position);
i32 UnderWater(GameObject_s *object);
void FindAnglesZX(NUVEC *normal, u16 *x_rotation, u16 *z_rotation);
extern "C" i32 NewShadowOnPlatform();
extern "C" i32 ShadowInfo();

void (*SurfaceInfo_ExtraReflectFn)(GameObject_s *object);

TERRAIN_TRACK_SLOT *AllocTerrId() {
    TERRSET *terrain = CurTerr;
    if (terrain == NULL) {
        return NULL;
    }

    for (i32 slot_index = 0; slot_index < TERRAIN_TRACK_SLOT_COUNT; ++slot_index) {
        if (terrain->track_slots[slot_index].id == NULL) {
            return &terrain->track_slots[slot_index];
        }
    }

    return NULL;
}

NUMTL *CreateCopyMat(NUMTL *source, i32 enable_uv_mode, i32 alpha_mode, i32 depth_mode, i32 filter_mode) {
    if (source == NULL) {
        return NULL;
    }

    NUMTL *material = NuMtlCreate3D(1);
    material->diffuse_color.r = 1.0f;
    material->diffuse_color.g = 1.0f;
    material->diffuse_color.b = 1.0f;
    material->attribs.cull_mode = 2;
    material->attribs.z_mode = static_cast<u32>(depth_mode) & 3;
    material->attribs.filter_mode = static_cast<u32>(filter_mode) & 3;
    material->attribs.alpha_mode = static_cast<u32>(alpha_mode) & 0xf;
    material->opacity = 0.999f;
    material->attribs.uv_mode = enable_uv_mode != 0;
    material->attribs.unknown_1_1_2 = 1;
    material->attribs.unknown_1_4_8 = 1;
    material->tex_id = source->tex_id;
    material->sort_pri = source->sort_pri;
    material->attribs.alpha_fail = 1;
    NuMtlUpdate(material);
    return material;
}

void SurfaceMaskOn(u32 *surface_mask) {
    *surface_mask = 0x20;
}

void GetSurfaceInfo(GameObject_s *object, i32 update_surface, f32 shadow_height) {
    APIOBJECT &api = object->apiobj;
    if (shadow_height == 2000000.0f) {
        object->field_0x1020 = 2000000.0f;
        object->field_0x1087 = 0;
        object->field_0x1078 = -1;
    } else {
        i32 surface = ShadowInfo();
        if (surface < 0 || surface > 31) {
            surface = 0;
        }
        object->field_0x1087 = 2;
        object->field_0x1020 = shadow_height;
        object->field_0x1078 = static_cast<i16>(NewShadowOnPlatform());

        if (update_surface != 0) {
            object->field_0xe41 = static_cast<u8>(surface);
            api.field_0x281 = static_cast<u8>(surface);
            object->surface_normal = ShadNorm;
        }
    }

    if (update_surface != 0 && shadow_height == 2000000.0f) {
        object->field_0xe41 = 0;
        api.field_0x281 = 0;
        object->surface_normal = v010;
    }

    // The extended shadow casts populate water/roof information in the target.
    // Until that cast path is reconstructed, preserve its no-hit state rather
    // than manufacturing a surface from the ordinary floor result.
    api.water_height = 2000000.0f;
    api.field_0x220 = 2000000.0f;
    api.field_0x27f = 0xff;
    api.field_0x280 = 0xff;

    if (SurfaceInfo_ExtraReflectFn != NULL) {
        SurfaceInfo_ExtraReflectFn(object);
    }
}

i32 IntersectWater(GameObject_s *object) {
    if ((object->apiobj.field_0x27f & static_cast<u8>(~8u)) != 1) {
        return 0;
    }

    const f32 water_height = object->apiobj.water_height;
    if (water_height > object->apiobj.collision_max.y) {
        return 0;
    }
    return object->apiobj.collision_min.y <= water_height;
}

void SurfaceMaskOff(u32 *surface_mask) {
    *surface_mask = 0;
}

void Surfaces_Reset() {
    LAYER_HOVERIGNORE = 0x10;
    SURFACEBITS_DUST = 0;

    i32 i = 0;
    u32 no_dust = 0;
    const u32 initial_surface_bit = 1;
    u32 dust = 0;
    while (i < 32) {
        const u32 surface_bit = initial_surface_bit << i;
        if ((TerSurface[i].flags & 4) != 0) {
            dust |= surface_bit;
        } else {
            no_dust |= surface_bit;
        }
        ++i;
    }
    SURFACEBITS_DUST = dust;
    SURFACEBITS_NODUST = no_dust;
}

void DeRotateTerrain(tertype *surface) {
    TerrainQuery_s *query = TerI;

    const f32 sin_pitch = NuTrigTable[(static_cast<i32>(-query->movement_pitch) >> 1) & 0x7fff];
    const f32 cos_pitch = NuTrigTable[(static_cast<i32>(16384.0f - query->movement_pitch) >> 1) & 0x7fff];
    const f32 sin_yaw = NuTrigTable[(static_cast<i32>(-query->movement_yaw) >> 1) & 0x7fff];
    const f32 cos_yaw = NuTrigTable[(static_cast<i32>(16384.0f - query->movement_yaw) >> 1) & 0x7fff];
    const f32 start_z = query->local_start.z;
    const f32 start_x = query->local_start.x;
    const f32 start_y = query->local_start.y;

    for (i32 vertex_index = 0; vertex_index < 3; ++vertex_index) {
        const NUVEC &vertex = surface->vectors[vertex_index];
        NUVEC &transformed = query->transformed_vertices[vertex_index];
        const f32 relative_z = vertex.z - start_z;
        const f32 relative_x = vertex.x - start_x;
        const f32 rotated_x = relative_z * sin_yaw + relative_x * cos_yaw;
        const f32 rotated_z = relative_z * cos_yaw - relative_x * sin_yaw;

        transformed.x = rotated_x;
        transformed.y = (vertex.y - start_y) * cos_pitch - rotated_z * sin_pitch;
        transformed.z = (vertex.y - start_y) * sin_pitch + rotated_z * cos_pitch;
    }

    if (65536.0f > surface->normals[1].y) {
        const NUVEC &vertex = surface->vectors[3];
        NUVEC &transformed = query->transformed_vertices[3];
        const f32 relative_z = vertex.z - start_z;
        const f32 relative_x = vertex.x - start_x;
        const f32 rotated_x = relative_z * sin_yaw + relative_x * cos_yaw;
        const f32 rotated_z = relative_z * cos_yaw - relative_x * sin_yaw;

        transformed.x = rotated_x;
        transformed.y = (vertex.y - start_y) * cos_pitch - rotated_z * sin_pitch;
        transformed.z = (vertex.y - start_y) * sin_pitch + rotated_z * cos_pitch;
    }
}

void InitSurfaceInfo(GameObject_s *object) {
    APIOBJECT &api = object->apiobj;
    const f32 shadow_height = GameShadow(object, &api.position, 5.0f, -1);
    api.field_0x218 = shadow_height;

    if (shadow_height == 2000000.0f) {
        GetSurfaceInfo(object, 0, 2000000.0f);
        object->field_0xd6c = 0.0f;
        api.field_0x281 = 0;
        object->field_0xe41 = 0;
        object->surface_normal = v010;
    } else {
        GetSurfaceInfo(object, 1, shadow_height);
        object->field_0xd6c = (TerSurface[static_cast<i8>(api.field_0x281)].flags & 8) != 0 ? 1.0f : 0.0f;
        FindAnglesZX(&object->surface_normal, NULL, NULL);
        object->field_0x1062 = static_cast<u16>(temp_xrot);
        object->field_0x105e = static_cast<u16>(temp_xrot);
        object->field_0x1064 = static_cast<u16>(temp_zrot);
        object->field_0x1060 = static_cast<u16>(temp_zrot);
    }

    api.is_underwater = static_cast<u8>(UnderWater(object));
    api.intersects_water = static_cast<u8>(IntersectWater(object));
    api.water_height = 2000000.0f;
    api.field_0x220 = 2000000.0f;
    api.field_0x27f = 0xff;
    api.field_0x280 = 0xff;
    GetSurfaceInfo(object, 1, api.field_0x218);

    const GAMECHARACTERDATA_s *character = static_cast<GAMECHARACTERDATA_s *>(api.character_data->field11_0x24);
    if ((character->flags_090 & 0x8000) == 0 && object->field_0x1020 == 2000000.0f && object->field_0x1078 != -1) {
        const f32 reflection_height = FindReflectionNoPlatforms(&api.position);
        if (reflection_height != 2000000.0f) {
            object->field_0x1020 = reflection_height;
            object->field_0x1087 = 2;
        }
    }

    if (SurfaceInfo_ExtraReflectFn != NULL) {
        SurfaceInfo_ExtraReflectFn(object);
    }
}

enum SurfaceDeflectMode : i32 {
    SURFACE_DEFLECT_SLIDE = 0,
    SURFACE_DEFLECT_PUSH_OUT_SLOW = 1,
    SURFACE_DEFLECT_PUSH_OUT_FAST = 2,
};

void Surface_Deflect(nuvec_s *normal, nuvec_s *movement, nuvec_s *result, i32 mode) {
    const f32 movement_x = movement->x;
    const f32 movement_y = movement->y;
    const f32 normal_x = normal->x;
    const f32 normal_y = normal->y;
    f32 correction = -movement_y * normal_y - movement_x * normal_x;
    const f32 movement_z = movement->z;
    const f32 normal_z = normal->z;
    correction -= movement_z * normal_z;
    if (mode == SURFACE_DEFLECT_PUSH_OUT_FAST) {
        correction += FRAMETIME * 0.015f;
    } else if (mode == SURFACE_DEFLECT_PUSH_OUT_SLOW) {
        correction -= FRAMETIME * 0.006f;
    }

    result->x = movement_x + normal_x * correction;
    result->y = movement_y + normal_y * correction;
    result->z = movement_z + normal_z * correction;
}

void CreateUsefulMaterials() {
    SolidMtl = NuMtlCreate(1);
    SolidMtl->attribs.z_mode = 0;
    SolidMtl->attribs.alpha_mode = 0;
    SolidMtl->attribs.unknown_2_4 = 1;
    NuMtlUpdate(SolidMtl);

    SolidMtl3D = NuMtlCreate3D(1);
    SolidMtl3D->attribs.z_mode = 0;
    SolidMtl3D->attribs.alpha_mode = 0;
    SolidMtl3D->attribs.unknown_2_4 = 1;
    NuMtlUpdate(SolidMtl3D);
}
