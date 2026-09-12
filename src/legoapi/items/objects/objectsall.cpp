#include "nu2api/nu3d/nuspecial.h"
#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world_shared.h"
#include "legoapi/render/fx.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/characters/motion.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

i32 LEGOCONTEXT_BUCK = -1;
i16 LEGOACT_BUCK = -1;
f32 BUCK_RIDERJUMPCLEARANCE = 1.0f;
void (*BuckStartExtraFn)(GameObject_s *) = NULL;
void NewRumble(nupad_s *, f32, i32);
void StartJump(GameObject_s *, i32);
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);

void Buck_Start(GameObject_s *object, GameObject_s *rider) {
    if (LEGOCONTEXT_BUCK == -1 || LEGOACT_BUCK == -1 ||
        object->apiobj.character_model->model_data_b[LEGOACT_BUCK] == NULL)
        return;
    if (rider != NULL)
        NewRumble(rider->pad_gamepad->pad, 0.6f, 0);
    object->context_animation = LEGOACT_BUCK;
    object->character_context = LEGOCONTEXT_BUCK;
    object->context_animation_timer = AnimDuration(object->id, LEGOACT_BUCK, 0.0f, 0.0f, 1);
    if (BuckStartExtraFn != NULL)
        BuckStartExtraFn(object);
}

extern i16 id_SNOWMOB;
f32 GameShadow(GameObject_s *, NUVEC *, f32, i32);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);

i32 DoBuckStart(GameObject_s *object) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (world->current_level != HOTHESCAPEB_LDATA || object->id != id_SNOWMOB)
        return 0;
    i32 jumped = 0;
    NUVEC forward = {0.0f, 0.0f, 1.0f};
    for (i32 i = 0; i < 8; ++i) {
        GameObject_s *player = Player[i];
        if (player == NULL || (player->apiobj.field_0x1f8 & 0x1001) != 0x1001 || player->apiobj.field_0x287 != 0 ||
            player->field_0xcc0 != NULL)
            continue;
        NUVEC delta;
        NuVecSub(&delta, &player->apiobj.lower_position, reinterpret_cast<NUVEC *>(&object->joint_matrices[1].m30));
        if (delta.y > -0.01f && delta.y < 0.1f && delta.x * delta.x + delta.z * delta.z < 0.04000000283122063f) {
            NUVEC destination;
            NuVecRotateY(&destination, &forward, object->apiobj.field_0x276);
            NuVecAdd(&destination, &destination, &Player[i]->apiobj.collision_position);
            f32 height = GameShadow(NULL, &destination, 5.0f, -1);
            if (height != 2000000.0f)
                destination.y = height;
            StartBigJump(Player[i], &destination, 0, 1.0f, 1.0f, 1, 0);
            jumped = 1;
        }
    }
    GameAudio_PlaySfx(4, &object->apiobj.collision_position, 0, 0);
    return jumped;
}

pushblock_s *BlockInBlock(WORLDINFO_s *world, pushblock_s *block, i32 excluded, pushblock_s **support) {
    NUVEC *position = block->position;
    if (support)
        *support = NULL;
    for (i32 i = 0; i < world->push_block_count; ++i) {
        if (i == excluded)
            continue;
        pushblock_s *other = &world->push_blocks[i];
        if (other->flags_0cb & 2)
            continue;
        if (!NuSpecialGetVisibilityFn(&other->special))
            continue;
        f32 x = position->x - other->position->x, y = position->y - other->position->y,
            z = position->z - other->position->z;
        f32 ymax = other->bounds_max.y - block->bounds_min.y;
        bool overlapx = x > (other->bounds_min.x - block->bounds_max.x) + 0.01f &&
                        x < (other->bounds_max.x - block->bounds_min.x) - 0.01f;
        bool overlapy = y > other->bounds_min.y - block->bounds_max.y && y < ymax;
        bool overlapz = z > (other->bounds_min.z - block->bounds_max.z) + 0.01f &&
                        z < (other->bounds_max.z - block->bounds_min.z) - 0.01f;
        if (support && overlapz && overlapx && y - ymax > 0.0f) {
            if (!*support)
                *support = other;
            else if ((position->y - (*support)->position->y) - ((*support)->bounds_max.y - block->bounds_min.y) >
                     y - ymax) {
                other->supporting_block = *support;
                *support = other;
            }
        }
        if (overlapx && overlapy && overlapz)
            return other;
    }
    return NULL;
}

void Boulder_Kill(PART_s *, i32) {
}

void Boulder_Move(PART_s *, float) {
}

void Buck_MoveCode(GameObject_s *object, i32 start) {
    if (LEGOCONTEXT_BUCK != -1 && object->character_context == LEGOCONTEXT_BUCK) {
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer <= 0.0f)
            object->character_context = -1;
    } else if (object->character_context == -1 && start != 0) {
        Buck_Start(object, object->field_0xcc0);
    }
}

void FindNextBreak(unsigned char *, i32) {
}

void FindNearestBreak(unsigned char *, i32) {
}

void BuckStartExtra_LSW(GameObject_s *object) {
    DoBuckStart(object);
}

void Buck_StartRiderJump(GameObject_s *rider, GameObject_s *mount) {
    u16 angle = qrand();
    f32 speed = static_cast<f32>(qrand()) * 1.5259021893143654e-05f * 0.09f + 0.01f;
    rider->apiobj.velocity.x = NuTrigTable[angle >> 1] * speed;
    rider->apiobj.velocity.z = speed * NuTrigTable[((i32)angle + 0x4000) >> 1 & 0x7fff];
    StartJump(rider, 0);
    rider->tag_flags |= 1;
    rider->airborne_collision_target = mount;
    rider->apiobj.velocity.x = 0.0f;
    rider->apiobj.velocity.z = 0.0f;
    rider->pad_gamepad->buttons_held = 0;
    rider->pad_gamepad->buttons_pressed = 0;
    f32 clearance = mount->apiobj.upper_position.y;
    if ((mount->apiobj.character_data->game_character->flags_094[2] & 2) != 0) {
        clearance += BUCK_RIDERJUMPCLEARANCE;
    }
    clearance = clearance - rider->apiobj.lower_position.y + 0.1f;
    GAMECHARACTERDATA *character = rider->apiobj.character_data->game_character;
    if (clearance > character->jump_height) {
        rider->apiobj.velocity.y = NuFsqrt(-2.0f * character->gravity * clearance);
    }
}

void SetEffectVisibility(char *name, i32 visible) {
    i32 type = LookupDebrisEffectPageOnly(name, static_cast<char>(WorldInfo_CurrentlyActive()->page_pp));
    if (type != -1) {
        if (visible != 0)
            DebrisTypeStatusAlwaysOn(type);
        else
            DebrisTypeStatusAlwaysOff(type);
    }
}

i32 Conveyor_AdjustSpeed(NUVEC *velocity) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (world != NULL && world->field_0x5174 != 0) {
        velocity->x += world->current_level->conveyor_x_speed;
        velocity->z += world->current_level->conveyor_z_speed;
    }
    return 0;
}

void AddDevice(nufile_device_s *) {
}

extern NUGSCN *IconScene_FindById(i32 character_id);

// LevelObjects_InitForLevel @0x475630. Creates the runtime model table and
// resolves each registered model from the scene selected by its table kind.
// Found models are hidden in their source scene: their runtime handle is the
// template used by gizmos and other placed objects to draw them elsewhere.
void LevelObjects_InitForLevel(WORLDINFO_s *world) {
    usize aligned = ALIGN(world->giz_buffer.addr, 4);
    world->lev_objs = reinterpret_cast<LEVEL_OBJECT_RUNTIME *>(aligned);
    world->giz_buffer.addr = aligned + static_cast<usize>(LEVELOBJECTCOUNT) * sizeof(LEVEL_OBJECT_RUNTIME);
    memset(world->lev_objs, 0, static_cast<usize>(LEVELOBJECTCOUNT) * sizeof(LEVEL_OBJECT_RUNTIME));

    if (ObjTabList == NULL || LEVELOBJECTCOUNT <= 0) {
        return;
    }

    for (i32 object_index = 0; object_index < LEVELOBJECTCOUNT; ++object_index) {
        LEVELOBJECT &object_type = ObjTabList[object_index];
        LEVEL_OBJECT_RUNTIME &object = world->lev_objs[object_index];

        if (NuStrICmp(object_type.name, "power_up") == 0) {
            KNOBS = object_index;
        }

        NUGSCN *scene = NULL;
        switch (object_type.kind) {
            case LEVEL_OBJECT_SCENE_LEVEL:
                scene = world->current_gscn;
                break;

            case LEVEL_OBJECT_SCENE_AREA:
                scene = area_scene != NULL ? area_scene : world->current_gscn;
                break;

            case LEVEL_OBJECT_SCENE_CHARACTER_ICON:
                for (i32 character_id = 0; character_id < CHARCOUNT; ++character_id) {
                    scene = IconScene_FindById(character_id);
                    if (scene != NULL && NuSpecialFind(scene, &object.special, object_type.name, 1) != 0) {
                        goto object_resolved;
                    }
                }
                scene = big_icon_scene != NULL ? big_icon_scene : world->icons_gscn;
                break;

            case LEVEL_OBJECT_SCENE_SAVE_ICON:
                scene = saveicon_scene;
                break;

            case LEVEL_OBJECT_SCENE_VEHICLE:
                scene = vehicle_scene;
                break;

            case LEVEL_OBJECT_SCENE_BUTTON:
                scene = button_scene;
                break;

            case LEVEL_OBJECT_SCENE_THINGS:
            default:
                if (area_scene != NULL && world->area != NULL &&
                    (world->area->flags & AREAFLAG_OVERRIDE_THINGS_SCENE) != 0) {
                    if (NuSpecialFind(area_scene, &object.special, object_type.name, 1) != 0) {
                        goto object_resolved;
                    }
                }
                scene = things_scene;
                break;
        }

        if (scene != NULL) {
            NuSpecialFind(scene, &object.special, object_type.name, 1);
        }

    object_resolved:
        object.active = static_cast<u8>(NuSpecialExistsFn(&object.special));
        if (object.active != 0) {
            NuSpecialSetVisibility(&object.special, 0);
        }
    }
}

void EquivalentObjects_Configure(WORLDINFO_s *world, char *config) {
    (void)world;
    (void)config;
}
