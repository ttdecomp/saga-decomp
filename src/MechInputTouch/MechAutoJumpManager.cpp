#include "MechInputTouch_types.h"

#include "gameapi/ai/aisys/aisys.h"
#include "gamelib/util/gamelib_util_types.h"
#include "globals.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/characters/motion.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"

i32 testStreakAlpha = 32;
f32 testColourSecF = 256.0f;
f32 testStreakTime = 0.5f;
f32 testStreakCrossSize2 = 0.15f;
f32 testStreakRotateSpeed = 65536.0f;
f32 generateNewStreakTime = 1.5f;
f32 testStreakMinDist = 1.0f;
f32 testStreakSpeed = 1.0f;
f32 testStreakCrossSize = 0.1f;
f32 testStreakClipTestRadius = 0.2f;
extern i32 show_autojump_hint;
extern i32 id_HINT_LSW_AUTOJUMP;
extern i32 id_HINT_LSW_AUTOJUMP_FAIL;
void AddStreakPoints(NUVEC *, f32, u32, void **, i32, void *);

namespace {
    struct AutoJumpStreakLink {
        NULISTLNK link;
        MechAutoJumpConnection *connection;
        AIPATH_s *path;
        AIPATHCNX_s *path_connection;
        i32 direction;
        void *handle;
        void *reserved;
        NUVEC start;
        NUVEC end;
        u32 colour;
        f32 elapsed;
    };
    DECOMP_ASSERT(sizeof(AutoJumpStreakLink) == 0x40, "AutoJumpStreakLink size");
} // namespace

MechAutoJumpConnection *MechAutoJumpManager::AddAutoJumpConnection(AIPATH_s *path, AIPATHCNX_s *connection,
                                                                   i32 direction, bool use_path_direction, i32 colour,
                                                                   bool allow_streak) {
    if (connection != NULL && path != NULL) {
        MechAutoJumpConnection *jump =
            reinterpret_cast<MechAutoJumpConnection *>(NuLinkedListGetHead(&jump_connections));
        while (jump != NULL) {
            if (jump->path == path && jump->connection == connection && jump->direction == direction) {
                break;
            }
            jump = reinterpret_cast<MechAutoJumpConnection *>(
                NuLinkedListGetNext(&jump_connections, reinterpret_cast<NULISTLNK *>(jump)));
        }

        if (jump == NULL) {
            jump = new MechAutoJumpConnection;
            jump->path = NULL;
            jump->connection = NULL;
            jump->active = false;
            jump->use_path_direction = false;
            jump->is_using = false;
            jump->link.prev = NULL;
            jump->link.next = NULL;
            jump->base_colour_components.x = colour & 0xff;
            jump->base_colour_components.y = (colour >> 8) & 0xff;
            jump->streak_handle = NULL;
            jump->cooldown = 0.0f;
            jump->streak_alpha = 0.0f;
            jump->streak_reserved = 0.0f;
            jump->base_colour = colour;
            jump->base_colour_components.z = (colour >> 16) & 0xff;
            NuLinkedListInsert(&jump_connections, reinterpret_cast<NULISTLNK *>(jump));
        }

        jump->path = path;
        jump->direction = direction;
        jump->use_path_direction = use_path_direction;
        jump->connection = connection;
        jump->active = true;
        jump->colour = colour;
        jump->allow_streak = allow_streak;
        return jump;
    }
    return NULL;
}

void MechAutoJumpManager::DeleteJumpConnection(MechAutoJumpConnection *connection) {
    if (connection != NULL) {
        AutoJumpStreakLink *streak = reinterpret_cast<AutoJumpStreakLink *>(NuLinkedListGetHead(&streaks));
        while (streak != NULL) {
            if (streak->connection == connection) {
                streak->connection = NULL;
            }
            streak = reinterpret_cast<AutoJumpStreakLink *>(
                NuLinkedListGetNext(&streaks, reinterpret_cast<NULISTLNK *>(streak)));
        }

        NuLinkedListRemove(&jump_connections, reinterpret_cast<NULISTLNK *>(connection));
        delete connection;
    }
}

void MechAutoJumpManager::DeleteJumpConnectionsAndStreaks() {
    AutoJumpStreakLink *streak;
    while ((streak = reinterpret_cast<AutoJumpStreakLink *>(NuLinkedListGetHead(&streaks))) != NULL) {
        NuLinkedListRemove(&streaks, reinterpret_cast<NULISTLNK *>(streak));
        delete streak;
    }

    MechAutoJumpConnection *connection =
        reinterpret_cast<MechAutoJumpConnection *>(NuLinkedListGetHead(&jump_connections));
    while (connection != NULL) {
        DeleteJumpConnection(connection);
        connection = reinterpret_cast<MechAutoJumpConnection *>(NuLinkedListGetHead(&jump_connections));
    }
}

void MechAutoJumpManager::Init() {
}

MechAutoJumpManager::MechAutoJumpManager(AISYS_s *ai_system) {
    if (ai_system != nullptr) {
        streak_time = 0.0f;
        ai_sys = ai_system;
        streaks.head = nullptr;
        streaks.tail = nullptr;
        jump_connections.head = nullptr;
        jump_connections.tail = nullptr;
    }
}

void MechAutoJumpManager::PreProcessJumpConnections() {
    MechAutoJumpConnection *jump = reinterpret_cast<MechAutoJumpConnection *>(NuLinkedListGetHead(&jump_connections));
    while (jump != NULL) {
        jump->active = false;
        jump->use_path_direction = false;
        jump->allow_streak = false;
        if (jump->cooldown > 0.0f) {
            jump->cooldown -= FRAMETIME;
            if (jump->cooldown < 0.0f) {
                jump->cooldown = 0.0f;
            }
        }

        if (jump->is_using != 0) {
            if (player == NULL || ((player->touch_task == NULL || player->touch_task->IsBigJumpTask() == 0) &&
                                   static_cast<i8>(player->character_context) != LEGOCONTEXT_BIGJUMP)) {
                jump->is_using = false;
            } else if ((jump->connection->traversal_flags[jump->direction == 0] & mechAutoJumpFlags) != 0) {
                WORLDINFO *world = WorldInfo_CurrentlyActive();
                if (world != NULL && world->mech_auto_jump_manager != NULL) {
                    MechAutoJumpConnection *active = world->mech_auto_jump_manager->AddAutoJumpConnection(
                        jump->path, jump->connection, jump->direction == 0, false,
                        (static_cast<u32>(static_cast<u8>(testStreakAlpha)) << 24) | 0x808080, false);
                    if (active != NULL) {
                        active->cooldown = 2.0f;
                    }
                }
            }
        }
        jump = reinterpret_cast<MechAutoJumpConnection *>(
            NuLinkedListGetNext(&jump_connections, reinterpret_cast<NULISTLNK *>(jump)));
    }
}

void MechAutoJumpManager::Process() {
    streak_time += FRAMETIME;
    bool create_streak = false;
    if (streak_time >= generateNewStreakTime) {
        streak_time -= generateNewStreakTime;
        create_streak = true;
    }

    PreProcessJumpConnections();

    AIPATHSYS *paths = ai_sys != NULL ? ai_sys->path_sys : NULL;
    if (paths == NULL) {
        ProcessJumpConnections();
        return;
    }

    if (TouchHacks::TouchControlsActive) {
        AIPATH *path = paths->active_path;
        if (path != NULL) {
            show_autojump_hint = 0;
            for (i32 index = 0; index < path->connection_count; ++index) {
                AIPATHCNX *connection = &path->connections[index];
                for (i32 direction = 1; direction >= 0; --direction) {
                    i32 far_index = direction == 0 ? 1 : 0;
                    u32 flags = connection->traversal_flags[far_index];
                    if ((flags & (static_cast<u32>(LEGO_AIPATHCNX_DONT_JUMP_NOW) | 0x98000000u)) != 0 ||
                        (flags & static_cast<u32>(mechAutoJumpFlags)) == 0) {
                        continue;
                    }
                    AIPATHNODE *far_node = &path->nodes[connection->node_indices[far_index]];
                    if (NuCameraClipTestSphere(&far_node->position, testStreakClipTestRadius, &numtx_identity) != 0) {
                        continue;
                    }

                    u32 checks = DoSomeChecks(*player, *path, *connection, direction);
                    if (checks == 0) {
                        continue;
                    }
                    u32 colour = static_cast<u32>(static_cast<u8>(testStreakAlpha)) << 24;
                    bool allow_streak = (checks & 4) != 0;
                    if (allow_streak) {
                        colour |= 0xff00;
                        if (show_autojump_hint == 0) {
                            show_autojump_hint = id_HINT_LSW_AUTOJUMP;
                        }
                    } else {
                        colour |= 0x808080;
                        show_autojump_hint = id_HINT_LSW_AUTOJUMP_FAIL;
                    }
                    if ((checks & 2) == 0) {
                        continue;
                    }
                    bool use_path_direction = (checks & 1) != 0;
                    MechAutoJumpConnection *jump =
                        AddAutoJumpConnection(path, connection, direction, use_path_direction, colour, allow_streak);
                    if (use_path_direction && create_streak && jump != NULL && jump->cooldown <= 0.0f) {
                        AutoJumpStreakLink *new_streak = new AutoJumpStreakLink;
                        new_streak->link.prev = NULL;
                        new_streak->link.next = NULL;
                        new_streak->connection = jump;
                        new_streak->path = path;
                        new_streak->path_connection = connection;
                        new_streak->direction = direction;
                        new_streak->handle = NULL;
                        new_streak->reserved = NULL;
                        new_streak->colour = colour;
                        new_streak->elapsed = 0.0f;
                        NuLinkedListInsert(&streaks, reinterpret_cast<NULISTLNK *>(new_streak));
                    }
                }
            }
        }
    }

    AutoJumpStreakLink *streak = reinterpret_cast<AutoJumpStreakLink *>(NuLinkedListGetHead(&streaks));
    while (streak != NULL) {
        AutoJumpStreakLink *next = reinterpret_cast<AutoJumpStreakLink *>(
            NuLinkedListGetNext(&streaks, reinterpret_cast<NULISTLNK *>(streak)));
        streak->elapsed += FRAMETIME;
        AIPATHNODE *start_node = &streak->path->nodes[streak->path_connection->node_indices[streak->direction]];
        AIPATHNODE *end_node = &streak->path->nodes[streak->path_connection->node_indices[streak->direction == 0]];
        f32 distance = NuVecXZDist(&end_node->position, &start_node->position, NULL);
        if (distance < testStreakMinDist) {
            distance = testStreakMinDist;
        }
        f32 progress = streak->elapsed * testStreakSpeed;
        if (progress > distance) {
            NuLinkedListRemove(&streaks, reinterpret_cast<NULISTLNK *>(streak));
            delete streak;
        } else {
            f32 t = distance != 0.0f ? progress / distance : 0.0f;
            NUVEC centre;
            centre.x = start_node->position.x + (end_node->position.x - start_node->position.x) * t;
            centre.y = start_node->position.y + (end_node->position.y - start_node->position.y) * t;
            centre.z = start_node->position.z + (end_node->position.z - start_node->position.z) * t;
            centre.y += NuTrigTable[(static_cast<i32>(t * 32768.0f) >> 1) & 0x7fff];
            i32 facing = NuAtan2D(end_node->position.x - centre.x, end_node->position.z - centre.z);
            NUVEC cross = {testStreakCrossSize, 0.0f, 0.0f};
            NuVecRotateZ(&cross, &cross,
                         static_cast<u16>(static_cast<i32>(LevelTimer.time_elapsed * testStreakRotateSpeed)));
            NuVecRotateY(&cross, &cross, static_cast<u16>(facing));
            NuVecAdd(&streak->start, &centre, &cross);
            NuVecSub(&streak->end, &centre, &cross);
            if (streak->connection != NULL) {
                streak->colour = streak->connection->base_colour;
            }
        }
        streak = next;
    }

    ProcessJumpConnections();
}

void MechAutoJumpManager::ProcessJumpConnections() {
    MechAutoJumpConnection *jump = reinterpret_cast<MechAutoJumpConnection *>(NuLinkedListGetHead(&jump_connections));
    while (jump != NULL) {
        MechAutoJumpConnection *next = reinterpret_cast<MechAutoJumpConnection *>(
            NuLinkedListGetNext(&jump_connections, reinterpret_cast<NULISTLNK *>(jump)));
        if (jump->active == 0 && jump->is_using == 0 && jump->cooldown <= 0.005f) {
            DeleteJumpConnection(jump);
        } else if (jump->colour != jump->base_colour) {
            jump->base_colour_components.x = SeekLinearF(
                jump->base_colour_components.x, static_cast<f32>(jump->colour & 0xff), FRAMETIME * testColourSecF);
            jump->base_colour_components.y =
                SeekLinearF(jump->base_colour_components.y, static_cast<f32>((jump->colour >> 8) & 0xff),
                            FRAMETIME * testColourSecF);
            jump->base_colour_components.z =
                SeekLinearF(jump->base_colour_components.z, static_cast<f32>((jump->colour >> 16) & 0xff),
                            FRAMETIME * testColourSecF);
            jump->streak_alpha = SeekLinearF(jump->streak_alpha, static_cast<f32>(static_cast<i32>(jump->colour >> 24)),
                                             FRAMETIME * testColourSecF);
            jump->base_colour = (static_cast<u32>(static_cast<i32>(jump->base_colour_components.x)) & 0xff) |
                                ((static_cast<u32>(static_cast<i32>(jump->base_colour_components.y)) << 8) & 0xffff) |
                                ((static_cast<u32>(static_cast<i32>(jump->base_colour_components.z)) & 0xff) << 16) |
                                (static_cast<u32>(static_cast<i32>(jump->streak_alpha)) << 24);
        }
        jump = next;
    }
}

void MechAutoJumpManager::Render() {
    if (!TouchHacks::TouchControlsActive) {
        return;
    }

    AutoJumpStreakLink *streak = reinterpret_cast<AutoJumpStreakLink *>(NuLinkedListGetHead(&streaks));
    while (streak != NULL) {
        AutoJumpStreakLink *next = reinterpret_cast<AutoJumpStreakLink *>(
            NuLinkedListGetNext(&streaks, reinterpret_cast<NULISTLNK *>(streak)));
        AddStreakPoints(&streak->start, testStreakTime, streak->colour, &streak->handle, 0, player);
        streak = next;
    }

    MechAutoJumpConnection *jump = reinterpret_cast<MechAutoJumpConnection *>(NuLinkedListGetHead(&jump_connections));
    while (jump != NULL) {
        if (jump->cooldown <= 0.0f) {
            AIPATHNODE *node = &jump->path->nodes[jump->connection->node_indices[jump->direction]];
            NUVEC cross = {testStreakCrossSize2, 0.0f, 0.0f};
            NuVecRotateY(&cross, &cross,
                         static_cast<u16>(static_cast<i32>(LevelTimer.time_elapsed * testStreakRotateSpeed)));
            NUVEC first;
            NUVEC second;
            NuVecAdd(&first, &node->position, &cross);
            NuVecSub(&second, &node->position, &cross);
            AddStreakPoints(&first, testStreakTime, jump->base_colour, &jump->streak_handle, 0, player);
        }
        jump = reinterpret_cast<MechAutoJumpConnection *>(
            NuLinkedListGetNext(&jump_connections, reinterpret_cast<NULISTLNK *>(jump)));
    }
}

MechAutoJumpManager::~MechAutoJumpManager() {
    DeleteJumpConnectionsAndStreaks();
}
