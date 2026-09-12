#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/nu3d/nutex.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

static TRAFFICANIM_s reference_trafficanim;
static TRAFFICANIM_s *parse_trafficanim;
static TRAFFICANIMSYS_s *parse_trafficanimsys;
static WORLDINFO_s *parse_worldinfo;
i32 traffic_portalling;

static void Traffic_tfactor(NUFPAR *parser) {
    reference_trafficanim.tfactor = NuFParGetFloat(parser);
}

static void Traffic_rand_interval(NUFPAR *parser) {
    reference_trafficanim.random_interval = NuFParGetFloat(parser);
}

static void Traffic_frame_interval(NUFPAR *parser) {
    reference_trafficanim.frame_interval = NuFParGetFloat(parser);
}

static void TrafficAnim_yoffset(NUFPAR *parser) {
    if (parse_trafficanim != NULL) {
        parse_trafficanim->y_offset = NuFParGetFloat(parser);
    }
}

static void Traffic_vehicle(NUFPAR *parser) {
    if (NuFParGetWord(parser) == 0) {
        return;
    }
    if (parse_trafficanimsys == NULL) {
        return;
    }
    if (reference_trafficanim.vehicle_count > 15) {
        return;
    }

    char *name = parser->word_buf;
    if (name == NULL) {
        return;
    }
    if (parse_worldinfo == NULL) {
        return;
    }

    i32 vehicle_index;
    for (vehicle_index = 0; vehicle_index < parse_trafficanimsys->vehicle_count; ++vehicle_index) {
        if (NuStrICmp(name, NuSpecialGetName(&parse_trafficanimsys->vehicles[vehicle_index])) == 0) {
            break;
        }
    }

    if (vehicle_index == parse_trafficanimsys->vehicle_count) {
        if (parse_trafficanimsys->vehicle_count > 15) {
            return;
        }
        if (NuSpecialFind(parse_worldinfo->current_gscn,
                          &parse_trafficanimsys->vehicles[parse_trafficanimsys->vehicle_count], name, 1) == 0) {
            return;
        }
        vehicle_index = parse_trafficanimsys->vehicle_count++;
    }

    reference_trafficanim.vehicle_indices[reference_trafficanim.vehicle_count++] = vehicle_index;
}

static NUFPCOMJMP TrafficAnim_ConfigKeywords[] = {
    {const_cast<char *>("yoffset"), TrafficAnim_yoffset},
    {NULL, NULL},
};

static void Traffic_animobj(NUFPAR *parser) {
    if (NuFParGetWord(parser) == 0) {
        return;
    }

    nuhspecial_s special;
    if (NuSpecialFind(parse_worldinfo->current_gscn, &special, parser->word_buf, 1) == 0) {
        return;
    }

    nuinstanim_s *instance_animation = NuSpecialGetInstAnim(&special);
    if (instance_animation == NULL || parse_trafficanimsys->animation_count > 63) {
        return;
    }

    TRAFFICANIM_s *animation = &parse_trafficanimsys->animations[parse_trafficanimsys->animation_count++];
    parse_trafficanim = animation;
    *animation = reference_trafficanim;
    animation->special = special;
    animation->tfactor = instance_animation->tfactor;
    animation->animation = special.scene->instance_animation_data[instance_animation->anim_ix];

    NuFParPushCom(parser, TrafficAnim_ConfigKeywords);
    while (NuFParGetWord(parser) != 0) {
        NuFParInterpretWord(parser);
    }
    NuFParPopCom(parser);
}

static NUFPCOMJMP Traffic_ConfigKeywords[] = {
    {const_cast<char *>("animobj"), Traffic_animobj},
    {const_cast<char *>("frame_interval"), Traffic_frame_interval},
    {const_cast<char *>("rand_interval"), Traffic_rand_interval},
    {const_cast<char *>("tfactor"), Traffic_tfactor},
    {const_cast<char *>("vehicle"), Traffic_vehicle},
    {NULL, NULL},
};

void TrafficAnimSys_Draw(TRAFFICANIMSYS_s *system) {
    if (g_lowEndLevelBehaviour != 0 || system == NULL) {
        return;
    }

    if (traffic_portalling != 0) {
        u32 visible_rooms = 0;
        if (WORLD->current_gscn->num_rooms > 0) {
            i32 room = 0;
            while (room < WORLD->current_gscn->num_rooms && room < 32) {
                if (WORLD->rooms_visible_ptr[room] != 0) {
                    visible_rooms |= 1U << room;
                }
                ++room;
            }
        }

        TRAFFICANIMINSTANCE_s *instance =
            reinterpret_cast<TRAFFICANIMINSTANCE_s *>(NuLinkedListGetHead(&system->active_instances));
        while (instance != NULL) {
            TRAFFICANIM_s *animation = instance->animation;
            i32 frame_index = static_cast<i32>((instance->frame - 1.0f) / animation->room_frame_interval);
            if (frame_index <= 255) {
                u32 animation_rooms = 0xffffffff;
                u8 room = animation->rooms[frame_index];
                if (room != 0xff) {
                    animation_rooms = 1U << room;
                }
                room = animation->rooms[frame_index + 1];
                if (room != 0xff) {
                    animation_rooms |= 1U << room;
                }

                if (animation_rooms == 0xffffffff || (visible_rooms & animation_rooms) != 0) {
                    NUMTX matrix;
                    EvalAnim(&animation->special, instance->frame, &matrix, 1);
                    matrix.m31 += animation->y_offset;
                    NuSpecialDrawAt(&system->vehicles[instance->vehicle_index], &matrix);
                }
            }

            instance = reinterpret_cast<TRAFFICANIMINSTANCE_s *>(
                NuLinkedListGetNext(&system->active_instances, &instance->link));
        }
    } else {
        TRAFFICANIMINSTANCE_s *instance =
            reinterpret_cast<TRAFFICANIMINSTANCE_s *>(NuLinkedListGetHead(&system->active_instances));
        while (instance != NULL) {
            TRAFFICANIM_s *animation = instance->animation;
            if (animation->disabled == 0) {
                NUMTX matrix;
                EvalAnim(&animation->special, instance->frame, &matrix, 1);
                matrix.m31 += animation->y_offset;
                NuSpecialDrawAt(&system->vehicles[instance->vehicle_index], &matrix);
            }
            instance = reinterpret_cast<TRAFFICANIMINSTANCE_s *>(
                NuLinkedListGetNext(&system->active_instances, &instance->link));
        }
    }
}

void TrafficAnimSys_Reset(TRAFFICANIMSYS_s *system) {
    if (system == NULL) {
        return;
    }

    system->free_instances.head = NULL;
    system->free_instances.tail = NULL;
    system->active_instances.head = NULL;
    system->active_instances.tail = NULL;
    memset(system->instances, 0, sizeof(system->instances));

    for (i32 i = 0; i < 500; ++i) {
        NuLinkedListAppend(&system->free_instances, &system->instances[i].link);
    }

    if (g_lowEndLevelBehaviour != 0) {
        return;
    }

    TRAFFICANIM_s *animation = system->animations;
    for (i32 animation_index = 0; animation_index < system->animation_count; ++animation_index, ++animation) {
        f32 frame = 0.0f;
        if (animation->end_frame > 0.0f && animation->vehicle_count != 0) {
            while (frame < animation->end_frame && animation->vehicle_count != 0) {
                TRAFFICANIMINSTANCE_s *instance =
                    reinterpret_cast<TRAFFICANIMINSTANCE_s *>(NuLinkedListGetHead(&system->free_instances));
                if (instance == NULL) {
                    break;
                }

                NuLinkedListRemove(&system->free_instances, &instance->link);
                NuLinkedListAppend(&system->active_instances, &instance->link);
                instance->frame = frame;
                i32 random = qrand();
                instance->animation = animation;
                instance->vehicle_index = random / (0xffff / static_cast<i32>(animation->vehicle_count) + 1);
                frame += animation->frame_interval - animation->random_interval +
                         animation->random_interval * 2.0f * NuRandFloat();
            }
        }

        animation->next_spawn_time =
            animation->frame_interval - animation->random_interval + animation->random_interval * 2.0f * NuRandFloat();
    }
}

void TrafficAnimSys_Update(TRAFFICANIMSYS_s *system) {
    if (g_lowEndLevelBehaviour != 0 || system == NULL) {
        return;
    }

    TRAFFICANIMINSTANCE_s *instance =
        reinterpret_cast<TRAFFICANIMINSTANCE_s *>(NuLinkedListGetHead(&system->active_instances));
    while (instance != NULL) {
        TRAFFICANIMINSTANCE_s *next =
            reinterpret_cast<TRAFFICANIMINSTANCE_s *>(NuLinkedListGetNext(&system->active_instances, &instance->link));
        TRAFFICANIM_s *animation = instance->animation;
        instance->frame += animation->tfactor * FRAMETIME * 60.0f;
        if (instance->frame > animation->end_frame) {
            NuLinkedListRemove(&system->active_instances, &instance->link);
            instance->link.next = NULL;
            NuLinkedListAppend(&system->free_instances, &instance->link);
        }
        instance = next;
    }

    TRAFFICANIM_s *animation = system->animations;
    for (i32 animation_index = 0; animation_index < system->animation_count; ++animation_index, ++animation) {
        animation->next_spawn_time -= animation->tfactor * FRAMETIME * 60.0f;
        if (animation->next_spawn_time <= 0.0f) {
            if (animation->vehicle_count != 0) {
                instance = reinterpret_cast<TRAFFICANIMINSTANCE_s *>(NuLinkedListGetHead(&system->free_instances));
                if (instance != NULL) {
                    NuLinkedListRemove(&system->free_instances, &instance->link);
                    NuLinkedListAppend(&system->active_instances, &instance->link);
                    instance->frame = 0.0f;
                    i32 random = qrand();
                    instance->animation = animation;
                    instance->vehicle_index = random / (0xffff / static_cast<i32>(animation->vehicle_count) + 1);
                }
            }

            animation->next_spawn_time = animation->frame_interval - animation->random_interval +
                                         animation->random_interval * 2.0f * NuRandFloat();
        }
    }
}

void TrafficAnimSys_Configure(WORLDINFO_s *world, char *config) {
    world->trafficanim_sys = NULL;
    if (world->current_gscn == NULL) {
        return;
    }

    NUFPAR *parser = NuFParCreateMem(const_cast<char *>("Traffic"), config, 0xffff);
    if (parser == NULL) {
        return;
    }

    VARIPTR old_buffer = world->giz_buffer;
    VARIPTR old_buffer_end = world->unknown_0108;
    world->trafficanim_sys = static_cast<TRAFFICANIMSYS_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, sizeof(TRAFFICANIMSYS_s)));
    if (world->trafficanim_sys == NULL) {
        return;
    }

    parse_trafficanimsys = world->trafficanim_sys;
    memset(&reference_trafficanim, 0, sizeof(reference_trafficanim));
    parse_worldinfo = world;
    NuFParPushCom(parser, Traffic_ConfigKeywords);

    i32 parsing_traffic = 0;
    while (NuFParGetLine(parser) != 0) {
        if (NuFParGetWord(parser) == 0) {
            continue;
        }
        if (parsing_traffic != 0) {
            if (NuStrICmp(parser->word_buf, "traffic_end") == 0) {
                parsing_traffic = 0;
            } else {
                NuFParInterpretWord(parser);
                parsing_traffic = 1;
            }
        } else if (NuStrICmp(parser->word_buf, "traffic_start") == 0) {
            parsing_traffic = 1;
            memset(&reference_trafficanim, 0, sizeof(reference_trafficanim));
            reference_trafficanim.frame_interval = 15.0f;
            reference_trafficanim.random_interval = 5.0f;
            reference_trafficanim.tfactor = 1.0f;
        }
    }

    NuFParPopCom(parser);
    NuFParDestroy(parser);

    TRAFFICANIMSYS_s *system = world->trafficanim_sys;
    if (system->animation_count == 0) {
        world->giz_buffer = old_buffer;
        world->unknown_0108 = old_buffer_end;
        world->trafficanim_sys = NULL;
    } else {
        for (i32 animation_index = 0; animation_index < system->animation_count; ++animation_index) {
            TRAFFICANIM_s *animation = &system->animations[animation_index];
            if (animation->animation == NULL) {
                continue;
            }
            animation->end_frame = NuAnimEndFrameOld(animation->animation);
            animation->room_frame_interval = (animation->end_frame - 1.0f) * (1.0f / 256.0f);
            for (i32 frame_index = 0; frame_index < 256; ++frame_index) {
                NUMTX matrix;
                EvalAnim(&animation->special, frame_index * animation->room_frame_interval + 1.0f, &matrix, 1);
                matrix.m31 += animation->y_offset;
                animation->rooms[frame_index] = 0xff;
                i16 room =
                    static_cast<i16>(NuPortalWhichRoom(world->current_gscn, reinterpret_cast<NUVEC *>(&matrix.m30)));
                if (room >= 0 && room <= 63) {
                    animation->rooms[frame_index] = static_cast<u8>(room);
                }
            }
        }
    }

    parse_trafficanimsys = NULL;
    parse_worldinfo = NULL;
}
