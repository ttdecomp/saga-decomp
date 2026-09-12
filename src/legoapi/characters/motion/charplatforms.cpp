#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/objects/gameobjects.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/numath/nufloat.h"
#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" i16 FindPlatInst(i32);
extern "C" void NewTerrPlatformsOff(void);
extern "C" i32 ShadowInfo(void);
extern TERRSET *CurTerr;
extern TERRAIN_TRACK_SLOT *CurTrackInfo;
extern i16 castnum;
extern "C" TERRAIN_SURFACE_s TerSurface[32];
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
extern i32 PlatSkinMaxSize;

TERRAIN_TRACK_SLOT *AllocTerrId();

i32 SkinFlipTab[8] = {0, 1, 2, 3, 0, 2, 1, 3};
NUVEC TerrainSkin(PLATSKININFO *, NUVEC *, f32, i32);
NUVEC TerCrossProduct(NUVEC *, NUVEC *);
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
extern "C" void NewTerrPlatformsOff();
extern "C" i32 ShadowInfo();
extern "C" TERRAIN_SURFACE_s TerSurface[32];

void SkinPlatform(terrsitu_s *terrain_group, unsigned char *buffer, PLATSKININFO *info) {
    TERRAIN_GROUP *group = reinterpret_cast<TERRAIN_GROUP *>(terrain_group);
    if (CurTerr == NULL || static_cast<u32>(group->chunk_type) > 1)
        return;
    TERRAIN_SHAPE_BATCH *input = static_cast<TERRAIN_SHAPE_BATCH *>(group->data);
    TERRAIN_SHAPE_BATCH *output = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(buffer);
    while (input->marker >= 0) {
        output->marker = input->marker;
        output->shape_count = input->shape_count;
        TERRAIN_SHAPE *source = reinterpret_cast<TERRAIN_SHAPE *>(input + 1);
        TERRAIN_SHAPE *destination = reinterpret_cast<TERRAIN_SHAPE *>(output + 1);
        f32 min_x = 123456792.0f, min_z = 123456792.0f;
        f32 max_x = -123456792.0f, max_z = -123456792.0f;
        for (i32 i = 0; i < input->shape_count; ++i, ++source, ++destination) {
            i32 last_vertex = source->normals[1].y > 65535.0f ? 2 : 3;
            memcpy(destination, source, sizeof(TERRAIN_SHAPE));
            NUVEC minimum = {123456792.0f, 123456792.0f, 123456792.0f};
            NUVEC maximum = {-123456792.0f, -123456792.0f, -123456792.0f};
            for (i32 vertex = last_vertex; vertex >= 0; --vertex) {
                i32 source_vertex = SkinFlipTab[vertex + info->mirrored * 4];
                NUVEC &v = destination->vectors[vertex];
                v = TerrainSkin(info, &source->vectors[source_vertex], -1.0f, info->flags);
                v.x -= info->matrix->m30;
                v.y -= info->matrix->m31;
                v.z -= info->matrix->m32;
                min_x = MIN(v.x, min_x);
                max_x = MAX(v.x, max_x);
                min_z = MIN(v.z, min_z);
                max_z = MAX(v.z, max_z);
                minimum.x = MIN(v.x, minimum.x);
                minimum.y = MIN(v.y, minimum.y);
                minimum.z = MIN(v.z, minimum.z);
                maximum.x = MAX(v.x, maximum.x);
                maximum.y = MAX(v.y, maximum.y);
                maximum.z = MAX(v.z, maximum.z);
            }
            destination->min_x = minimum.x - 0.05f;
            destination->min_y = minimum.y - 0.05f;
            destination->min_z = minimum.z - 0.05f;
            destination->max_x = maximum.x + 0.05f;
            destination->max_y = maximum.y + 0.05f;
            destination->max_z = maximum.z + 0.05f;
            for (i32 normal = source->normals[1].y < 65535.0f ? 1 : 0; normal >= 0; --normal) {
                i32 origin = normal != 0 ? 3 : 0;
                i32 first = normal != 0 ? 1 : 2;
                i32 second = normal != 0 ? 2 : 1;
                NUVEC a, b;
                a.x = destination->vectors[first].x - destination->vectors[origin].x;
                a.y = destination->vectors[first].y - destination->vectors[origin].y;
                a.z = destination->vectors[first].z - destination->vectors[origin].z;
                b.x = destination->vectors[second].x - destination->vectors[origin].x;
                b.y = destination->vectors[second].y - destination->vectors[origin].y;
                b.z = destination->vectors[second].z - destination->vectors[origin].z;
                NUVEC &n = destination->normals[normal];
                n = TerCrossProduct(&a, &b);
                f32 length = NuFsqrt((n.x * n.x + n.y * n.y) + n.z * n.z);
                f32 inverse = length == 0.0f ? 0.0f : 1.0f / length;
                n.x *= inverse;
                n.y *= inverse;
                n.z *= inverse;
            }
        }
        if (input->shape_count > 0) {
            min_x -= 0.05f;
            min_z -= 0.05f;
            max_x += 0.05f;
            max_z += 0.05f;
        }
        output->min_x = min_x;
        output->max_x = max_x;
        output->min_z = min_z;
        output->max_z = max_z;
        input = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(source);
        output = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(destination);
    }
    output->marker = -1;
    output->shape_count = -1;
    group->data = buffer;
}

void Platform_Init(WORLDINFO_s *world) {
    NuSpecialFind(world->current_gscn, &LevHSpecial[0], const_cast<char *>("slave1_level"), 0);
}

void Platform_Reset(WORLDINFO_s *) {
    NuSpecialSetVisibility(&LevHSpecial[0], 0);
}

void PlatformConnect(char *track_id, nuvec_s *position_delta, nuvec_s *movement_delta, i32 platform_index) {
    if (CurTrackInfo == NULL) {
        CurTrackInfo = AllocTerrId();
        if (CurTrackInfo != NULL) {
            CurTrackInfo->flags |= TERRAIN_TRACK_FLAG_CONNECTED;
            CurTrackInfo->platform_index = static_cast<i16>(platform_index);
            CurTrackInfo->id = track_id;
            CurTrackInfo->platform_contact_state = TERRAIN_TRACK_CONTACT_ACTIVE;
            position_delta->y = 0.0f;
            movement_delta->y = 0.0f;
        }
    } else {
        CurTrackInfo->flags |= TERRAIN_TRACK_FLAG_CONNECTED;
        CurTrackInfo->platform_index = static_cast<i16>(platform_index);
        CurTrackInfo->platform_contact_state = TERRAIN_TRACK_CONTACT_ACTIVE;
    }

    castnum = CurTerr->platforms[platform_index].terrain_group_index;
}

void SkinPlatformSize(i32 group_index, unsigned char *buffer, PLATSKININFO *info) {
    if (CurTerr == NULL || group_index < 0)
        return;
    TERRAIN_GROUP *group = &CurTerr->groups[group_index];
    NUVEC minimum = {123456792.0f, 123456792.0f, 123456792.0f};
    NUVEC maximum = {-123456792.0f, -123456792.0f, -123456792.0f};
    f32 radius_squared = 0.0f;
    i32 bytes = 0;
    if (static_cast<u32>(group->chunk_type) <= 1) {
        TERRAIN_SHAPE_BATCH *input = static_cast<TERRAIN_SHAPE_BATCH *>(group->data);
        TERRAIN_SHAPE_BATCH *output = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(buffer);
        while (input->marker >= 0) {
            output->marker = input->marker;
            output->shape_count = input->shape_count;
            TERRAIN_SHAPE *source = reinterpret_cast<TERRAIN_SHAPE *>(input + 1);
            TERRAIN_SHAPE *destination = reinterpret_cast<TERRAIN_SHAPE *>(output + 1);
            for (i32 i = 0; i < input->shape_count; ++i, ++source, ++destination) {
                i32 last_vertex = source->normals[1].y > 65535.0f ? 2 : 3;
                memcpy(destination, source, sizeof(TERRAIN_SHAPE));
                for (i32 vertex = last_vertex; vertex >= 0; --vertex) {
                    NUVEC &v = destination->vectors[vertex];
                    v = TerrainSkin(info, &source->vectors[SkinFlipTab[vertex + info->mirrored * 4]], -1.0f,
                                    info->flags);
                    v.x -= info->matrix->m30;
                    v.y -= info->matrix->m31;
                    v.z -= info->matrix->m32;
                    minimum.x = MIN(v.x, minimum.x);
                    minimum.y = MIN(v.y, minimum.y);
                    minimum.z = MIN(v.z, minimum.z);
                    maximum.x = MAX(v.x, maximum.x);
                    maximum.y = MAX(v.y, maximum.y);
                    maximum.z = MAX(v.z, maximum.z);
                    radius_squared = MAX((v.x * v.x + v.y * v.y) + v.z * v.z, radius_squared);
                }
                for (i32 normal = source->normals[1].y < 65535.0f ? 1 : 0; normal >= 0; --normal) {
                    i32 origin = normal != 0 ? 3 : 0;
                    i32 first = normal != 0 ? 1 : 2;
                    i32 second = normal != 0 ? 2 : 1;
                    NUVEC a, b;
                    a.x = destination->vectors[first].x - destination->vectors[origin].x;
                    a.y = destination->vectors[first].y - destination->vectors[origin].y;
                    a.z = destination->vectors[first].z - destination->vectors[origin].z;
                    b.x = destination->vectors[second].x - destination->vectors[origin].x;
                    b.y = destination->vectors[second].y - destination->vectors[origin].y;
                    b.z = destination->vectors[second].z - destination->vectors[origin].z;
                    NUVEC &n = destination->normals[normal];
                    n = TerCrossProduct(&a, &b);
                    f32 squared = (n.x * n.x + n.y * n.y) + n.z * n.z;
                    f32 inverse = squared == 0.0f ? 0.0f : 1.0f / NuFsqrt(squared);
                    n.x *= inverse;
                    n.y *= inverse;
                    n.z *= inverse;
                }
            }
            input = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(source);
            output = reinterpret_cast<TERRAIN_SHAPE_BATCH *>(destination);
        }
        output->marker = -1;
        output->shape_count = -1;
        bytes = reinterpret_cast<unsigned char *>(input) + 4 - static_cast<unsigned char *>(group->data);
    }
    group->origin.x = info->matrix->m30;
    group->origin.y = info->matrix->m31;
    group->origin.z = info->matrix->m32;
    group->bounds_min.x = (minimum.x - 0.1f) + info->matrix->m30;
    group->bounds_min.y = (minimum.y - 0.1f) + info->matrix->m31;
    group->bounds_min.z = (minimum.z - 0.1f) + info->matrix->m32;
    group->bounds_max.x = (maximum.x + 0.1f) + info->matrix->m30;
    group->bounds_max.y = (maximum.y + 0.1f) + info->matrix->m31;
    group->bounds_max.z = (maximum.z + 0.1f) + info->matrix->m32;
    group->radius = NuFsqrt(radius_squared);
    if (bytes > PlatSkinMaxSize)
        PlatSkinMaxSize = bytes;
}

void CharPlatforms_Reset(CHARPLATFORMSYS_s *system) {
    if (system == NULL) {
        return;
    }

    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        if ((Obj[i].apiobj.field_0x1f8 & 1) != 0) {
            Obj[i].field_0x107c = -1;
        }
    }

    for (i32 i = 0; i < system->platform_count; ++i) {
        CHARPLATFORM_s *platform = &system->platforms[i];
        NuSpecialSetVisibility(&platform->special, 0);
        platform->platform_id = FindPlatInst(NuSpecialGetInstanceix(&platform->special));
        platform->object = NULL;
        if (platform->platform_id != -1) {
            GameObject_s *object = FindGameObject(platform->object_id, 0, 1, 0, 1);
            if (object != NULL) {
                object->field_0x107c = platform->platform_id;
                platform->object = object;
            }
        }
    }
}

void CharPlatforms_Configure(WORLDINFO_s *world, char *config) {
    world->char_platform_sys = NULL;
    if (world->current_gscn == NULL) {
        return;
    }

    NUFPAR *parser = NuFParCreateMem(const_cast<char *>("CharPlatforms"), config, 0xffff);
    if (parser == NULL) {
        return;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    CHARPLATFORMSYS_s *system = reinterpret_cast<CHARPLATFORMSYS_s *>(world->giz_buffer.void_ptr);
    world->char_platform_sys = system;
    system->field_0x00 = static_cast<i32>(reinterpret_cast<usize>(world->current_gscn));
    system->platform_count = 0;

    while (NuFParGetLine(parser) != 0) {
        if (NuFParGetWord(parser) == 0) {
            break;
        }
        if (NuStrICmp(parser->word_buf, const_cast<char *>("char_platform")) != 0 || NuFParGetWord(parser) == 0) {
            continue;
        }

        CHARPLATFORM_s *platform = &system->platforms[system->platform_count];
        platform->object_id = CharIDFromName(parser->word_buf);
        if (platform->object_id == -1 || NuFParGetWord(parser) == 0) {
            continue;
        }
        if (NuSpecialFind(world->current_gscn, &platform->special, parser->word_buf, 1) == 0) {
            continue;
        }

        platform->platform_id = -1;
        platform->object = NULL;
        ++system->platform_count;
    }

    NuFParDestroy(parser);
    if (system->platform_count > 0) {
        world->giz_buffer.addr = ALIGN(reinterpret_cast<usize>(&system->platforms[system->platform_count]), 4);
    } else {
        world->char_platform_sys = NULL;
    }
}

void CharPlatforms_Update(CHARPLATFORMSYS_s *system) {
    if (system == NULL)
        return;
    CHARPLATFORM_s *platform = system->platforms;
    for (i32 i = 0; i < system->platform_count; ++i, ++platform) {
        if (platform->object_id == -1)
            continue;
        i32 visible = 0;
        GameObject_s *object = Obj;
        for (i32 j = 0; j < HIGHGAMEOBJECT; ++j, ++object) {
            if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
                object->field_0x107c != platform->platform_id)
                continue;
            nuhspecial_s *special = &system->platforms[i].special;
            NUMTX *matrix = NuSpecialGetDrawMtx(special);
            if (matrix != NULL) {
                *matrix = object->apiobj.field_0xb8;
                if (GameTimer.time_elapsed * 2.0f < 1.0f) {
                    matrix->m31 -= 1.0f - GameTimer.time_elapsed * 2.0f;
                }
                NuSpecialUpdate(special);
            }
            visible = 1;
            break;
        }
        NuSpecialSetVisibility(&system->platforms[i].special, visible);
    }
}

f32 FindReflectionNoPlatforms(nuvec_s *position) {
    NewTerrPlatformsOff();
    f32 height = GameShadow(NULL, position, 5.0f, -1);
    if (height != 2000000.0f) {
        u32 surface = static_cast<u32>(ShadowInfo());
        if (surface < 32 && (TerSurface[surface].flags & 2) != 0) {
            return height;
        }
    }
    return 2000000.0f;
}

GameObject_s *CharPlatform_FindObjFromPlatID(CHARPLATFORMSYS_s *system, i32 platform_id) {
    if (system != NULL) {
        CHARPLATFORM_s *platform = system->platforms;
        for (i32 i = 0; i < system->platform_count; ++i, ++platform) {
            if (platform->object != NULL && platform->platform_id == platform_id)
                return platform->object;
        }
    }
    return NULL;
}
