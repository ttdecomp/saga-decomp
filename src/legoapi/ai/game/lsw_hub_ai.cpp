#include "decomp.h"
#include "gameapi/ai/aisys/aisys.h"
#include "globals.h"
#include "legoapi/ai/game/lsw_hub_ai.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/gizmo/base/GizObstacleObjectInterface.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/world/world.h"
#include "legoapi/world/world_shared.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nurand.h"

#include <stdio.h>
#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

struct HUBAI_s {
    AIAREA *areas[10];
    AIPATH *paths[10];
    AILOCATOR *area_locators[10][4];
    i16 area;
    i16 unknown_0x0f2;
    i16 table_count;
    i16 counter_count;
    u8 unknown_0x0f8[4];
    void *unknown_0x0fc;
    void *unknown_0x100;
    GIZAIMESSAGE_s *area_message;
    GIZAIMESSAGE_s *serving_customer_message;
    AILOCATOR *counter_a[4];
    AILOCATOR *counter_b[4];
    AILOCATOR *table_locators[8];
    AILOCATOR *serve_player;
};

DECOMP_ASSERT(sizeof(HUBAI_s) == 0x150, "HUBAI_s size");

HUBAI_s hub_ai = {};

char *hub_areas[10] = {"MAINROOM", "EPISODE1", "EPISODE2", "EPISODE3", "EPISODE4",
                       "EPISODE5", "EPISODE6", "JUNKYARD", "BONUS",    "BOUNTY"};

extern GIZOBSTACLE_s *LevGizObst[8];

extern i32 CheckPosAIArea(AIAREA_s *area, nuvec_s *position, f32 tolerance);
extern void FreeTorpedoPacket(TORPEDOPACKET_s **packet);
extern void RemoveGameObject(GameObject_s *object, i32 mode);

extern "C" {
    i32 AISysSetLevelPath(AISYS_s *system, char *path_name);
    void AISysCharacterSetPath(AIPACKET_s *packet, AIPATH_s *path);
    void AISysGetCharacterPathPos(AISYS_s *system, APIOBJECT_s *object, AIPACKET_s *packet, i32 checks, i32 ground);
}

enum HUB_AI_CONSTANTS {
    HUB_AREA_COUNT = 10,
    HUB_PLAYER_COUNT = 8,
    HUB_TABLE_COUNT = 8,
    HUB_CHARACTER_CAPACITY = 340,
    HUB_BARMAN_CREATURE_SET = 2,
};

f32 Condition_InHubArea(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *area_argument) {
    AI_HUB_AREA_ARGUMENT argument = {};
    argument.pointer = area_argument;
    return argument.value == hub_ai.area ? 1.0f : 0.0f;
}

static inline void Hub_MarkCharacterUnavailable(i16 *characters, i16 character) {
    if (character != -1) {
        characters[character] = -1;
    }
}

static void Hub_MakeListCharactersAvailable(i16 *characters) {
    for (i32 character = 0; character < CHARCOUNT; ++character) {
        characters[character] = apicharsys->playermodelids[character];
    }

    for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index) {
        GameObject_s *object = &Obj[object_index];
        if ((object->apiobj.field_0x1f8 & APIOBJECT_FLAG_IN_USE) != 0) {
            characters[object->id] = -1;
        }
    }

    Hub_MarkCharacterUnavailable(characters, id_BARMAN);
    Hub_MarkCharacterUnavailable(characters, id_TRAININGREMOTE);
    Hub_MarkCharacterUnavailable(characters, id_JABBA);
    Hub_MarkCharacterUnavailable(characters, id_WOMPRAT);
    Hub_MarkCharacterUnavailable(characters, id_WEIRDO1);
    Hub_MarkCharacterUnavailable(characters, id_WEIRDO2);
    Hub_MarkCharacterUnavailable(characters, id_CANTINABAND);
    Hub_MarkCharacterUnavailable(characters, id_DROIDEKA);
    Hub_MarkCharacterUnavailable(characters, id_BOB);
    Hub_MarkCharacterUnavailable(characters, id_WHIP);

    for (i32 pack = 0; pack < 11; ++pack) {
        i16 *character = StorePack[pack].id;
        if (character != NULL && *character != -1 && Store_IsPackUnlocked(pack) == 0) {
            characters[*character] = -1;
        }
    }
}

static void Hub_GoneThroughDoor(WORLDINFO_s *world) {
    GameObject_s *first_player = Player[0];
    if (first_player == NULL) {
        return;
    }

    const i32 player_index = static_cast<i8>(first_player->apiobj.field_0x27c);
    i32 new_area = -1;
    for (i32 area = 0; area < HUB_AREA_COUNT; ++area) {
        if (CheckPosAIArea(hub_ai.areas[area], PlayerStart[player_index].pos, 0.0f) != 0) {
            new_area = area;
            break;
        }
    }

    if (new_area == -1 || new_area == hub_ai.area) {
        return;
    }

    hub_ai.area = static_cast<i16>(new_area);
    AIPATH *area_path = hub_ai.paths[new_area];
    if (area_path != NULL && AISysSetLevelPath(world->ai_sys, area_path->name) != 0) {
        for (i32 player_index = 0; player_index < HUB_PLAYER_COUNT; ++player_index) {
            GameObject_s *player = Player[player_index];
            if (player == NULL || (player->apiobj.field_0x1f8 & APIOBJECT_FLAG_IN_USE) == 0) {
                continue;
            }

            AISysCharacterSetPath(&player->ai, world->ai_sys->path_sys->active_path);
            AISysGetCharacterPathPos(world->ai_sys, &player->apiobj, &player->ai, 0xff,
                                     static_cast<i8>(player->apiobj.field_0x27d));
        }
    }

    i16 available_characters[HUB_CHARACTER_CAPACITY];
    Hub_MakeListCharactersAvailable(available_characters);

    for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index) {
        GameObject_s *object = &Obj[object_index];
        if ((object->apiobj.field_0x1f8 & APIOBJECT_FLAG_IN_USE) == 0 || object->apiobj.field_0x27c != -1 ||
            object->ai.field_0x134 != 0xff) {
            continue;
        }

        object->field_0xeb4 = 0;
        FreeTorpedoPacket(&object->torpedo);
        RemoveGameObject(object, 1);
    }

    hub_ai.unknown_0x0f2 = 0;
    hub_ai.unknown_0x0fc = NULL;
    hub_ai.unknown_0x100 = NULL;
    SetGizAIMessage(gizaimessagesys, "ServingCustomer", 0.0f, hub_ai.serving_customer_message);
    if (hub_ai.area_message != NULL) {
        hub_ai.area_message->value = static_cast<f32>(hub_ai.area);
    }

    hub_ai.table_count = 0;
    char name[32];
    for (i32 table = 0; table < HUB_TABLE_COUNT; ++table) {
        if (hub_ai.area == 0) {
            sprintf(name, "Table_%d", table);
        } else {
            sprintf(name, "Table%d_%d", hub_ai.area, table);
        }

        hub_ai.table_locators[table] = AIPathFindLocator(world->ai_sys, name);
        if (hub_ai.table_locators[table] == NULL) {
            return;
        }
        ++hub_ai.table_count;
    }
}

void LSW_Hub_InitAI(WORLDINFO_s *world) {
    memset(&hub_ai, 0, sizeof(hub_ai));

    char name[32];
    for (i32 area = 0; area < 10; ++area) {
        hub_ai.areas[area] = AISysFindArea(world->ai_sys, hub_areas[area]);
        hub_ai.paths[area] = AISysFindPath(world->ai_sys, hub_areas[area]);

        for (i32 locator = 0; locator < 4; ++locator) {
            sprintf(name, "%s_%d", hub_areas[area], locator + 1);
            hub_ai.area_locators[area][locator] = AIPathFindLocator(world->ai_sys, name);
        }
    }

    hub_ai.area = -1;
    hub_ai.serving_customer_message = SetGizAIMessage(gizaimessagesys, "ServingCustomer", 0.0f, NULL);
    hub_ai.area_message = SetGizAIMessage(gizaimessagesys, "Area", 0.0f, NULL);
    hub_ai.counter_count = 0;

    for (i32 counter = 0; counter < 4; ++counter) {
        sprintf(name, "CounterA_%d", counter);
        hub_ai.counter_a[counter] = AIPathFindLocator(world->ai_sys, name);
        sprintf(name, "CounterB_%d", counter);
        hub_ai.counter_b[counter] = AIPathFindLocator(world->ai_sys, name);
        if (hub_ai.counter_a[counter] != NULL && hub_ai.counter_b[counter] != NULL) {
            hub_ai.counter_count++;
        }
    }

    hub_ai.serve_player = AIPathFindLocator(world->ai_sys, "ServePlayer");
    hub_ai.table_count = 0;
    for (i32 i = 0; i < 8; ++i) {
        hub_ai.table_locators[i] = NULL;
    }
}

void LSW_Hub_ResetAI(WORLDINFO_s *world) {
    hub_ai.area = -1;
    LevGizObst[0] = GizObstacle_FindByName(world->giz_obstacle_sys, "BandDoor_Open");
}

void LSW_Hub_UpdateAI(WORLDINFO_s *world) {
    if (hub_character_ready != -1) {
        SetLevelSfxBits(world);
        hub_character_ready = -1;
    }

    if (netclient != 0) {
        return;
    }

    if (hub_ai.area == -1) {
        Hub_GoneThroughDoor(world);
        if (netclient != 0) {
            return;
        }
    }

    GIZOBSTACLE_s *barman_obstacle = LevGizObst[0];
    if (barman_obstacle != NULL) {
        if (aicreature_sets_alive[HUB_BARMAN_CREATURE_SET] != 0) {
            barman_obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE;
            barman_obstacle->runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_BLOCKED);
        } else {
            barman_obstacle->runtime_flags &= static_cast<u8>(~GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE);
            barman_obstacle->runtime_flags |= GIZOBSTACLE_RUNTIME_FLAG_BLOCKED;
        }
    }
}

i32 Hub_CurrentArea() {
    if (WORLD->current_level == HUB_LDATA) {
        return hub_ai.area;
    }
    return -1;
}

i32 Hub_Outside() {
    return Hub_CurrentArea() == 7;
}

i32 Hub_GetRandomCharType() {
    i16 available_characters[HUB_CHARACTER_CAPACITY];
    i32 candidates[HUB_CHARACTER_CAPACITY];
    Hub_MakeListCharactersAvailable(available_characters);

    i32 candidate_count = 0;
    for (i32 character = 0; character < CHARCOUNT; ++character) {
        if (available_characters[character] != -1) {
            candidates[candidate_count++] = character;
        }
    }

    if (candidate_count == 0) {
        if (id_MOSEISLEYCITIZEN != -1 && apicharsys->playermodelids[id_MOSEISLEYCITIZEN] != -1) {
            candidates[candidate_count++] = id_MOSEISLEYCITIZEN;
        }
        if (id_CANTINAALIEN != -1 && apicharsys->playermodelids[id_CANTINAALIEN] != -1) {
            candidates[candidate_count++] = id_CANTINAALIEN;
        }
    }

    if (candidate_count == 0) {
        return -1;
    }
    return candidates[NuRandInt() % candidate_count];
}
