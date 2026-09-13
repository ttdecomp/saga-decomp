#include <SDL3/SDL.h>

#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "decomp.h"
#include "globals.h"
#include "gameapi/ai/aisys/aisys.h"
#include "gameapi/gui/apimenu.h"
#include "gameframework/saveload.h"
#include "host/harness/window.hpp"
#include "host/platform/free_camera.hpp"
#include "host/platform/graphics.hpp"
#include "host/platform/input.hpp"
#include "host/platform/runtime.hpp"
#include "legoapi/characters/core/players.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/NuRenderDevice.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/nuscreen.hpp"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nuplatform/nuplatform.h"

#if defined(__EMSCRIPTEN__)
// These movement fields must retain the original 32-bit layout in WASM.
static_assert(sizeof(GameObject_s) == 0x10e4, "WASM GameObject movement ABI");
static_assert(offsetof(GAMEPAD_s, input_mode) == 0x25, "WASM input mode offset");
static_assert(offsetof(GAMEPAD_s, input_direction_z) == 0x2c, "WASM first direction component offset");
static_assert(offsetof(GAMEPAD_s, input_direction_x) == 0x30, "WASM second direction component offset");
static_assert(offsetof(APIOBJECT, velocity) == 0x68, "WASM velocity offset");
static_assert(offsetof(APIOBJECT, collision_position) == 0x80, "WASM collision position offset");
static_assert(offsetof(APIOBJECT, collision_contact_mask) == 0x1ec, "WASM collision mask offset");
static_assert(offsetof(APIOBJECT, collision_exclusion_mask) == 0x298, "WASM exclusion mask offset");
static_assert(offsetof(APIOBJECT, collision_mask_high) == 0x29c, "WASM exclusion mask high offset");
static_assert(alignof(APIOBJECT) == 4, "WASM API object alignment");
static_assert(offsetof(GameObject_s, character_context) == 0x7a5, "WASM movement context offset");
static_assert(offsetof(GameObject_s, pad_gamepad) == 0xc94, "WASM movement controller offset");
static_assert(offsetof(GameObject_s, delayed_turn_timer) == 0xd40, "WASM turn timer offset");
static_assert(offsetof(GameObject_s, target_velocity) == 0xf24, "WASM target velocity offset");
#endif

extern "C" i32 NuMain(i32 argc, char **argv);
extern i32 GetMenuID();
extern i32 memcard_loadneeded;
extern i32 memcard_loadstarted;
extern i32 memcard_loadfailed;
extern i32 memcard_loadcorrupt;
extern i32 waiting_for_level;
extern i32 waiting_for_character;
extern i32 waiting_for_new_level;
extern i32 abort_load;
extern i32 reset_load;
extern i32 CharacterDataLoad;
extern i32 NewMode;
extern i32 Paused;
extern FadeSystem FadeSys;
extern GAMEPAD_s GamePad[64];

namespace {
    char host_capture_directory[128] = ".work/capture";

    struct HostSpecialHandleLayout {
        NUGSCN *scene;
        void *special;
        void *display_special;
    };

    struct HostDisplaySpecialLayout {
        NUMTX mtx;
        NUMTX draw_mtx;
        NUVEC min;
        f32 min_w;
        NUVEC max;
        f32 max_w;
        u8 pad_a0[0x10];
        NUCLIPOBJECT *clip_objects;
        char *name;
        u32 flags;
        f32 *clip_range;
        i32 instance_ix;
        NUMTX *draw_mtx_ptr;
        i16 wind_speed;
        i16 wind_scale;
        u32 pad_cc;
    };

    constexpr i32 host_window_width = 1280;
    constexpr i32 host_window_height = 720;
    constexpr i32 host_poll_interval_ms = 16;
    constexpr i32 host_tail_frames = 30;
    // Leave enough time for the original asynchronous load result and its
    // one-second menu result delay to complete after the final scripted tap.
    constexpr Uint64 host_scripted_title_input_ms = 18000;
    constexpr Uint64 host_scripted_menu_settle_ms = 500;
    constexpr Uint64 host_scripted_play_move_ms = 2000;
    constexpr Uint64 host_scripted_play_settle_ms = 1000;
    constexpr Uint64 host_scripted_play_jump_ready_timeout_ms = 2000;
    constexpr Uint64 host_scripted_play_jump_ascent_ms = 200;
    constexpr Uint64 host_scripted_play_second_jump_ms = 64;
    constexpr Uint64 host_scripted_play_jump_timeout_ms = 2500;
    constexpr Uint64 host_scripted_action_entry_timeout_ms = 2000;
    constexpr Uint64 host_scripted_action_release_timeout_ms = 5000;
    constexpr Uint64 host_scripted_pause_settle_ms = 500;
    constexpr Uint64 host_camera_orbit_duration_ms = 10000;
    constexpr f32 host_full_camera_rotation = 65536.0f;

    enum class HostScriptedInputStage {
        title,
        new_or_load,
        select_controls,
        load_wait,
        save_slot,
        overwrite_confirm,
        cantina_wait,
        play_move,
        play_settle,
        play_jump_ascent,
        play_jump_second,
        play_jump_land,
        action_wait_entry,
        action_wait_release,
        pause_wait,
        pause_settle,
        pause_move_wait,
        pause_return_wait,
        resume_wait,
        complete,
    };

    struct HostScriptedPlaySnapshot {
        NUVEC player_position{};
        NUVEC camera_position{};
        NUVEC camera_desired_position{};
        NUVEC camera_socket_position{};
        NUVEC camera_target{};
        NUVEC player_velocity{};
        u16 camera_pitch = 0;
        u16 camera_yaw = 0;
        u16 camera_desired_pitch = 0;
        u16 camera_desired_yaw = 0;
        f32 camera_position_seek = 0.0f;
        f32 camera_angle_seek = 0.0f;
        f32 camera_look_pitch = 0.0f;
        f32 camera_look_yaw = 0.0f;
        i8 camera_mode = -1;
        i8 camera_previous_mode = -1;
        i8 camera_socket = -1;
        i16 camera_socket_segment = -1;
        f32 camera_socket_ratio = 0.0f;
        u16 player_yrot = 0;
        u16 player_facing_angle = 0;
        u16 player_input_angle = 0;
        u32 player_flags = 0;
        u32 player_motion_flags = 0;
        u32 buttons_held = 0;
        u8 pad_runtime_flags = 0;
        i8 player_context = -1;
        i16 context_animation = -1;
        i16 queued_context_animation = -1;
        f32 input_magnitude = 0.0f;
        f32 animation_input_magnitude = 0.0f;
        f32 movement_speed = 0.0f;
        f32 frame_time = 0.0f;
        f32 animation_time = 0.0f;
        f32 animation_blend_elapsed = 0.0f;
        f32 animation_blend_duration = 0.0f;
        i16 animation_current = -1;
        i16 animation_requested = -1;
        i16 animation_previous = -1;
        i16 animation_blend_source = -1;
        i16 animation_blend_target = -1;
        u8 animation_flags = 0;
        u8 animation_blending = 0;
        u8 animation_format_flags = 0;
        u32 character_model_flags = 0;
        u32 game_character_flags = 0;
        i8 character_movement_type = -1;
        CHARACTERUPDATEFN move_function = nullptr;
        CHARACTERUPDATEFN animate_function = nullptr;
        bool has_walk_animation = false;
        bool has_idle_animation = false;
        bool has_run_animation = false;
        i32 object_index = -1;
    };

    enum class HostScriptedMenuAction {
        waiting,
        navigating,
        confirmed,
    };

    enum class HostJediAction : i16 {
        combo_1_1 = 46,
        combo_2_1 = 53,
    };

    enum class HostJumpAction : i16 {
        land = 7,
        second_jump = 9,
        land_after_fall = 10,
        third_jump = 14,
    };

    static bool host_is_jedi_combo_action(i16 action) {
        return action == static_cast<i16>(HostJediAction::combo_1_1) ||
               action == static_cast<i16>(HostJediAction::combo_2_1);
    }

    static bool host_is_jump_landing_action(i16 action) {
        return action == static_cast<i16>(HostJumpAction::land) ||
               action == static_cast<i16>(HostJumpAction::land_after_fall);
    }

    static bool host_finite_vec(const NUVEC &vec) {
        return std::isfinite(vec.x) && std::isfinite(vec.y) && std::isfinite(vec.z);
    }

    static bool host_player_matrix_ready(const GameObject_s *object) {
        const NUMTX &matrix = object->apiobj.field_0xb8;
        const f32 basis_squared = matrix.m00 * matrix.m00 + matrix.m01 * matrix.m01 + matrix.m02 * matrix.m02 +
                                  matrix.m10 * matrix.m10 + matrix.m11 * matrix.m11 + matrix.m12 * matrix.m12 +
                                  matrix.m20 * matrix.m20 + matrix.m21 * matrix.m21 + matrix.m22 * matrix.m22;
        const f32 dx = matrix.m30 - object->apiobj.position.x;
        const f32 dy = matrix.m31 - object->apiobj.position.y;
        const f32 dz = matrix.m32 - object->apiobj.position.z;
        return std::isfinite(basis_squared) && basis_squared > 0.0001f && std::isfinite(matrix.m33) &&
               std::fabs(matrix.m33 - 1.0f) < 0.001f && dx * dx + dy * dy + dz * dz < 0.0001f;
    }

    static bool host_scripted_play_ready() {
        // current_level can point at the hub while the asynchronous world and
        // character loads are still finishing.  Require the active hub area,
        // its gameplay socket system, the normal idle load sentinels, and the
        // real Player 0 controller assignment before injecting any input.
        if (WORLD == nullptr || WORLD->loaded == 0 || WORLD->current_level != HUB_LDATA || WORLD->area != HUB_ADATA ||
            WORLD->sock_sys == nullptr || Player[0] == nullptr) {
            return false;
        }

        const u32 player_flags = Player[0]->apiobj.field_0x1f8;
        return NewLData == nullptr && NewMode == 0 && player == Player[0] && Player[1] == nullptr &&
               (player_flags & 0x1001) == 0x1001 && static_cast<i8>(player_flags) < 0 &&
               Player[0]->pad_gamepad == &GamePad[0] && host_player_matrix_ready(Player[0]) &&
               host_finite_vec(Player[0]->apiobj.position) && std::isfinite(global_camera.mtx.m30) &&
               std::isfinite(global_camera.mtx.m31) && std::isfinite(global_camera.mtx.m32) &&
               waiting_for_level == -1 && waiting_for_character == -1 && waiting_for_new_level == 0 && abort_load == 0;
    }

    static bool host_player_can_start_jump(const GameObject_s *object) {
        if (object == nullptr || object->character_context != CHARACTER_CONTEXT_NONE) {
            return false;
        }

        // JumpCode accepts a grounded character, or the short grace period
        // established by PreResetCode after the last grounded frame.
        return object->apiobj.field_0x27d != 0 || object->ground_contact_grace_timer > 0.0f;
    }

    static HostScriptedPlaySnapshot host_scripted_play_snapshot() {
        HostScriptedPlaySnapshot snapshot;
        snapshot.player_position = Player[0]->apiobj.position;
        snapshot.player_velocity = Player[0]->apiobj.velocity;
        snapshot.player_yrot = Player[0]->yrot;
        snapshot.player_facing_angle = Player[0]->apiobj.facing_angle;
        snapshot.player_input_angle = Player[0]->current_input_angle;
        snapshot.player_flags = Player[0]->apiobj.field_0x1f8;
        snapshot.player_motion_flags = Player[0]->apiobj.field_0x1f4;
        snapshot.buttons_held = GamePad[0].buttons_held;
        snapshot.pad_runtime_flags = GamePad[0].allocated_5a;
        snapshot.player_context = Player[0]->character_context;
        snapshot.context_animation = Player[0]->context_animation;
        snapshot.queued_context_animation = Player[0]->queued_context_animation;
        snapshot.input_magnitude = GamePad[0].input_magnitude;
        snapshot.animation_input_magnitude = GamePad[0].animation_input_magnitude;
        snapshot.frame_time = FRAMETIME;
        const ANIMPACKET_s &animation = Player[0]->apiobj.anim_packet;
        snapshot.animation_time = animation.current_time;
        snapshot.animation_blend_elapsed = animation.blend_elapsed;
        snapshot.animation_blend_duration = animation.blend_duration;
        snapshot.animation_current = animation.animation_index;
        snapshot.animation_requested = animation.requested_animation;
        snapshot.animation_previous = animation.previous_animation;
        snapshot.animation_blend_source = animation.blend_animation_a;
        snapshot.animation_blend_target = animation.blend_animation_b;
        snapshot.animation_flags = animation.flags;
        snapshot.animation_blending = animation.blending;
        if (Player[0]->apiobj.character_data != nullptr) {
            CHARACTERDATA *character = Player[0]->apiobj.character_data;
            snapshot.character_model_flags = character->model_flags;
            snapshot.move_function = character->move_fn;
            snapshot.animate_function = character->animate_fn;
            if (character->field11_0x24 != nullptr) {
                GAMECHARACTERDATA *game_character = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
                snapshot.game_character_flags = game_character->flags_090;
                snapshot.character_movement_type = static_cast<i8>(game_character->field275_0x116);
            }
        }
        if (Player[0]->apiobj.character_model != nullptr &&
            Player[0]->apiobj.character_model->model_data_b != nullptr) {
            void **animations = Player[0]->apiobj.character_model->model_data_b;
            snapshot.has_walk_animation = animations[0] != nullptr;
            snapshot.has_idle_animation = animations[1] != nullptr;
            snapshot.has_run_animation = animations[3] != nullptr;
            if (animation.animation_index >= 0 && animations[animation.animation_index] != nullptr) {
                const ani3_animheader_s *header =
                    static_cast<ani3_animheader_s *>(animations[animation.animation_index]);
                snapshot.animation_format_flags = header->format_flags;
            }
        }
        if (Obj != nullptr && Player[0] >= Obj && Player[0] < Obj + HIGHGAMEOBJECT) {
            snapshot.object_index = static_cast<i32>(Player[0] - Obj);
        }
        if (Player[0]->apiobj.character_data != nullptr && Player[0]->apiobj.character_data->field11_0x24 != nullptr) {
            snapshot.movement_speed =
                static_cast<GAMECHARACTERDATA *>(Player[0]->apiobj.character_data->field11_0x24)->movement_speed;
        }
        snapshot.camera_position = {global_camera.mtx.m30, global_camera.mtx.m31, global_camera.mtx.m32};
        if (GameCam != nullptr) {
            snapshot.camera_target = GameCam->target;
            snapshot.camera_desired_position = GameCam->desired_position;
            snapshot.camera_socket_position = GameCam->sock_position.camera_position;
            snapshot.camera_pitch = static_cast<u16>(GameCam->pitch);
            snapshot.camera_yaw = static_cast<u16>(GameCam->yaw);
            snapshot.camera_desired_pitch = GameCam->desired_pitch;
            snapshot.camera_desired_yaw = GameCam->desired_yaw;
            snapshot.camera_position_seek = GameCam->position_seek;
            snapshot.camera_angle_seek = GameCam->angle_seek;
            snapshot.camera_look_pitch = GameCam->field_0x214;
            snapshot.camera_look_yaw = GameCam->field_0x218;
            snapshot.camera_mode = GameCam->mode;
            snapshot.camera_previous_mode = GameCam->previous_mode;
            snapshot.camera_socket = GameCam->sock_position.location.sock;
            snapshot.camera_socket_segment = GameCam->sock_position.location.segment;
            snapshot.camera_socket_ratio = GameCam->sock_position.ratio;
        }
        return snapshot;
    }

    static void host_log_camera_trace(const char *stage, const HostScriptedPlaySnapshot &snapshot) {
        LOG_INFO("scripted camera %s: rendered=(%.3f,%.3f,%.3f) desired=(%.3f,%.3f,%.3f) "
                 "socket-position=(%.3f,%.3f,%.3f) socket=%d segment=%d ratio=%.4f look=(%.1f,%.1f)",
                 stage, snapshot.camera_position.x, snapshot.camera_position.y, snapshot.camera_position.z,
                 snapshot.camera_desired_position.x, snapshot.camera_desired_position.y,
                 snapshot.camera_desired_position.z, snapshot.camera_socket_position.x,
                 snapshot.camera_socket_position.y, snapshot.camera_socket_position.z, snapshot.camera_socket,
                 snapshot.camera_socket_segment, snapshot.camera_socket_ratio, snapshot.camera_look_pitch,
                 snapshot.camera_look_yaw);
    }

    static void host_log_ai_script(const AISCRIPT *script) {
        if (script == nullptr) {
            return;
        }

        LOG_INFO("scripted AI script: name=%s base=%s", script->name != nullptr ? script->name : "-",
                 script->derived_from != nullptr ? script->derived_from : "-");
        for (const NULISTLNK *state_node = script->states.head; state_node != nullptr; state_node = state_node->next) {
            const AISTATE *state = reinterpret_cast<const AISTATE *>(state_node);
            LOG_INFO("scripted AI state: script=%s state=%s", script->name != nullptr ? script->name : "-",
                     state->name != nullptr ? state->name : "-");
            for (const NULISTLNK *action_node = state->actions.head; action_node != nullptr;
                 action_node = action_node->next) {
                const AIACTION *action = reinterpret_cast<const AIACTION *>(action_node);
                LOG_INFO("scripted AI action: script=%s state=%s action=%s params=%d",
                         script->name != nullptr ? script->name : "-", state->name != nullptr ? state->name : "-",
                         action->def != nullptr && action->def->name != nullptr ? action->def->name : "-",
                         action->param_count);
                for (i32 param = 0; param < action->param_count; ++param) {
                    LOG_INFO("scripted AI action param: script=%s state=%s action=%s index=%d value=%s",
                             script->name != nullptr ? script->name : "-", state->name != nullptr ? state->name : "-",
                             action->def != nullptr && action->def->name != nullptr ? action->def->name : "-", param,
                             action->params != nullptr && action->params[param] != nullptr ? action->params[param]
                                                                                           : "-");
                }
            }
            for (const NULISTLNK *condition_node = state->conditions.head; condition_node != nullptr;
                 condition_node = condition_node->next) {
                const AICONDITION *condition = reinterpret_cast<const AICONDITION *>(condition_node);
                LOG_INFO("scripted AI condition: script=%s state=%s condition=%s arg=%s next=%s",
                         script->name != nullptr ? script->name : "-", state->name != nullptr ? state->name : "-",
                         condition->def != nullptr && condition->def->name != nullptr ? condition->def->name : "-",
                         condition->arg != nullptr ? condition->arg : "-",
                         condition->next_state_name != nullptr ? condition->next_state_name : "-");
            }
        }
    }

    static void host_log_animation_trace(const char *stage, const HostScriptedPlaySnapshot &snapshot) {
        LOG_INFO("scripted animation %s: current=%d requested=%d previous=%d time=%.3f flags=0x%x format=0x%x "
                 "blend=(active=%u,source=%d,target=%d,elapsed=%.3f,duration=%.3f) "
                 "movement=(context=%d,pad-flags=0x%x,input=%.3f,animation-input=%.3f,facing=%u,input-angle=%u) "
                 "character=(model-flags=0x%x,game-flags=0x%x,movement-type=%d,move=%p,animate=%p) "
                 "available=(walk=%d,idle=%d,run=%d)",
                 stage, snapshot.animation_current, snapshot.animation_requested, snapshot.animation_previous,
                 snapshot.animation_time, snapshot.animation_flags, snapshot.animation_format_flags,
                 snapshot.animation_blending, snapshot.animation_blend_source, snapshot.animation_blend_target,
                 snapshot.animation_blend_elapsed, snapshot.animation_blend_duration, snapshot.player_context,
                 snapshot.pad_runtime_flags, snapshot.input_magnitude, snapshot.animation_input_magnitude,
                 snapshot.player_facing_angle, snapshot.player_input_angle, snapshot.character_model_flags,
                 snapshot.game_character_flags, snapshot.character_movement_type,
                 reinterpret_cast<void *>(snapshot.move_function), reinterpret_cast<void *>(snapshot.animate_function),
                 snapshot.has_walk_animation ? 1 : 0, snapshot.has_idle_animation ? 1 : 0,
                 snapshot.has_run_animation ? 1 : 0);
    }

    static void host_log_portal_trace(const char *stage) {
        NUGSCN *scene = WORLD != nullptr ? WORLD->current_gscn : nullptr;
        if (scene == nullptr) {
            LOG_INFO("portal visibility %s: no current scene", stage);
            return;
        }
        i32 visible = 0;
        const void *visibility_marker = scene->portal_visibility_marker;
        const i32 instance_count = scene->display_list != nullptr ? scene->display_list->nclip_objects : 0;
        for (i32 i = 0; i < instance_count; ++i) {
            visible += (PortalVisiFlags[i >> 3] >> (i & 7)) & 1;
        }
        LOG_INFO("portal visibility %s: rooms=%d portals=%u camera-room=%d frusta=%d clips=%d visible=%d "
                 "data=%p spheres=%p boxes=%p visibility-tag=0x%x bits=%p display=%p",
                 stage, scene->num_rooms, scene->max_portals, scene->camera_room, scene->num_portal_frusta,
                 instance_count, visible, reinterpret_cast<void *>(scene->portal_instance_count), scene->portal_spheres,
                 scene->portal_boxes, static_cast<u32>(reinterpret_cast<usize>(visibility_marker)),
                 scene->instance_visibility_flags, scene->display_list);
    }

    static void host_log_display_list_trace() {
        NUGSCN *scene = WORLD != nullptr ? WORLD->current_gscn : nullptr;
        NUDLDLISTSCENE *display = scene != nullptr ? scene->display_list : nullptr;
        if (display == nullptr || display->items == nullptr) {
            LOG_INFO("display-list trace: no current display list");
            return;
        }

        i32 item_counts[256] = {};
        for (i32 i = 0; i < display->nitems; ++i) {
            ++item_counts[display->items[i].type];
        }
        LOG_INFO("display-list trace: name=%s instances=%d items=%d materials=%u specials=%d sort-priorities=%d",
                 display->name != nullptr ? display->name : "-", scene->num_instances, display->nitems, display->nmtls,
                 display->nspecials, display->nsort_pris);
        for (i32 type = 0; type < 256; ++type) {
            if (item_counts[type] != 0) {
                LOG_INFO("display-list type 0x%02x: count=%d", type, item_counts[type]);
            }
        }
    }

    static void host_log_pickup_trace(const char *stage) {
        if (WORLD == nullptr || WORLD->gizmo_pickup_sys == nullptr) {
            LOG_INFO("pickup trace %s: no pickup system", stage);
            return;
        }

        GIZMOPICKUPRUNTIMESYS_s *system = WORLD->gizmo_pickup_sys;
        i32 active = 0;
        i32 visible = 0;
        i32 draw_visible = 0;
        i32 collected = 0;
        i32 drawn = 0;
        i32 room_visible = 0;
        i32 model_active = 0;
        i32 types[256] = {};
        i32 nearest_index = -1;
        f32 nearest_distance_squared = 0.0f;
        const GameObject_s *player = Player[0];
        for (i32 index = 0; index < system->pickup_count; ++index) {
            const GIZMOPICKUP_s &pickup = system->pickups[index];
            active += (pickup.state_flags & GIZMOPICKUP_STATE_ACTIVE) != 0;
            visible += (pickup.state_flags & GIZMOPICKUP_STATE_VISIBLE) != 0;
            draw_visible += (pickup.state_flags & GIZMOPICKUP_STATE_DRAW_VISIBLE) != 0;
            collected += (pickup.state_flags & GIZMOPICKUP_STATE_COLLECTED) != 0;
            drawn += (pickup.state_flags & GIZMOPICKUP_STATE_DRAWN) != 0;
            ++types[static_cast<u8>(pickup.type_code)];

            const bool pickup_room_visible = pickup.room_index < 0 || WORLD->rooms_visible_ptr == nullptr ||
                                             WORLD->rooms_visible_ptr[pickup.room_index] != 0;
            room_visible += pickup_room_visible;
            if (pickup.type_index < GizmoPickupSys_Game.type_count) {
                const GIZMO_PICKUP_TYPE &type = GizmoPickupSys_Game.types[pickup.type_index];
                const i32 model_index = type.first_model_id + pickup.model_variant;
                model_active += WORLD->lev_objs != nullptr && WORLD->lev_objs[model_index].active != 0;
            }
            if (player != nullptr && (pickup.state_flags & (GIZMOPICKUP_STATE_ACTIVE | GIZMOPICKUP_STATE_ENABLED |
                                                            GIZMOPICKUP_STATE_COLLECTED)) ==
                                         (GIZMOPICKUP_STATE_ACTIVE | GIZMOPICKUP_STATE_ENABLED)) {
                const f32 dx = pickup.position.x - player->apiobj.position.x;
                const f32 dy = pickup.position.y - player->apiobj.position.y;
                const f32 dz = pickup.position.z - player->apiobj.position.z;
                const f32 distance_squared = dx * dx + dy * dy + dz * dz;
                if (nearest_index == -1 || distance_squared < nearest_distance_squared) {
                    nearest_index = index;
                    nearest_distance_squared = distance_squared;
                }
            }
        }
        LOG_INFO("pickup trace %s: total=%d active=%d visible=%d draw-visible=%d collected=%d drawn=%d "
                 "room-visible=%d model-active=%d draw-distance=%.3f scale=%.3f",
                 stage, system->pickup_count, active, visible, draw_visible, collected, drawn, room_visible,
                 model_active, system->draw_distance, system->pickup_scale);
        for (i32 type = 0; type < 256; ++type) {
            if (types[type] != 0) {
                LOG_INFO("pickup type '%c': count=%d", type, types[type]);
            }
        }
        if (nearest_index != -1) {
            const GIZMOPICKUP_s &pickup = system->pickups[nearest_index];
            LOG_INFO("pickup nearest %s: index=%d type='%c' position=(%.3f,%.3f,%.3f) "
                     "player=(%.3f,%.3f,%.3f) distance-squared=%.3f flags=0x%x config=0x%x",
                     stage, nearest_index, pickup.type_code, pickup.position.x, pickup.position.y, pickup.position.z,
                     player->apiobj.position.x, player->apiobj.position.y, player->apiobj.position.z,
                     nearest_distance_squared, pickup.state_flags, pickup.config_flags);
        }
    }

    static HostScriptedMenuAction host_scripted_menu_select(i32 row, i32 column, Uint64 elapsed_ticks,
                                                            Uint64 &last_action_ticks) {
        if (elapsed_ticks < last_action_ticks + host_scripted_menu_settle_ms) {
            return HostScriptedMenuAction::waiting;
        }

        const MENU &menu = GameMenu[GameMenuLevel];
        u32 button = GAMEPAD_JUMP;
        HostScriptedMenuAction result = HostScriptedMenuAction::confirmed;
        if (menu.selected_row < row) {
            button = GAMEPAD_DDOWN;
            result = HostScriptedMenuAction::navigating;
        } else if (menu.selected_row > row) {
            button = GAMEPAD_DUP;
            result = HostScriptedMenuAction::navigating;
        } else if (menu.selected_column < column) {
            button = GAMEPAD_DRIGHT;
            result = HostScriptedMenuAction::navigating;
        } else if (menu.selected_column > column) {
            button = GAMEPAD_DLEFT;
            result = HostScriptedMenuAction::navigating;
        }

        LOG_INFO("scripted gamepad menu=%d cursor=(%d,%d) target=(%d,%d) button=0x%x", GetMenuID(),
                 menu.selected_column, menu.selected_row, column, row, button);
        HostInputTap(0, button);
        last_action_ticks = elapsed_ticks;
        return result;
    }

    static bool host_write_ppm(const char *path, const u8 *rgba, i32 width, i32 height) {
        FILE *file = fopen(path, "wb");
        if (file == nullptr) {
            LOG_ERR("failed to open %s for writing: %s", path, strerror(errno));
            return false;
        }
        fprintf(file, "P6\n%d %d\n255\n", width, height);

        for (i32 y = height - 1; y >= 0; y--) {
            for (i32 x = 0; x < width; x++) {
                const usize src = (static_cast<usize>(y) * static_cast<usize>(width) + static_cast<usize>(x)) * 4;
                fwrite(&rgba[src], 1, 3, file);
            }
        }
        fclose(file);

        return true;
    }

    static u64 host_pixel_hash(const u8 *pixels, usize pixel_count) {
        u64 hash = 1469598103934665603ULL;
        // The capture files contain RGB. Ignore framebuffer alpha as well:
        // blend-state changes can alter it without changing the visible image.
        for (usize i = 0; i < pixel_count; i++) {
            const u8 *pixel = pixels + i * 4;
            for (usize channel = 0; channel < 3; channel++) {
                hash ^= pixel[channel];
                hash *= 1099511628211ULL;
            }
        }
        return hash;
    }

    static bool host_frame_has_visible_pixels(const u8 *pixels, usize pixel_count) {
        for (usize i = 0; i < pixel_count; ++i) {
            const u8 *pixel = pixels + i * 4;
            if (pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0) {
                return true;
            }
        }
        return false;
    }

    static bool host_read_frame(std::vector<u8> &pixels, i32 &width, i32 &height, u64 &hash) {
        pixels.resize(static_cast<usize>(host_window_width) * host_window_height * 4);
        const i32 packed = HostReadbackPixels(host_window_width, host_window_height, pixels.data());
        if (packed <= 0) {
            return false;
        }

        width = packed / 1000;
        height = packed % 1000;
        hash = host_pixel_hash(pixels.data(), static_cast<usize>(width) * height);
        return true;
    }

    static bool host_capture_frame(i32 frame, const std::vector<u8> &pixels, i32 width, i32 height) {
        char filename[192];
        snprintf(filename, sizeof(filename), "%s/window_%04d.ppm", host_capture_directory, frame);

        return host_write_ppm(filename, pixels.data(), width, height);
    }

    static void host_sdl_init(bool offscreen, bool mute, bool msaa) {
        const char *video_driver = HostPlatformVideoDriver();
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, video_driver);
        if (mute) {
            SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        }

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            LOG_ERR("SDL_Init(VIDEO) failed: %s", SDL_GetError());
            return;
        }
        SDL_InitSubSystem(SDL_INIT_AUDIO);

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
        // The 32-bit NVIDIA package does not provide the vendor's external EGL
        // platform modules. SDL's GLX path can still create a hardware GLES2
        // context, which the Linux host render-device adapter adopts below.
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, msaa ? 1 : 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa ? 4 : 0);
#endif

        SDL_WindowFlags window_flags =
            offscreen ? static_cast<SDL_WindowFlags>(SDL_WINDOW_HIDDEN | SDL_WINDOW_NOT_FOCUSABLE) : 0;
#if defined(__linux__) && !defined(__EMSCRIPTEN__)
        window_flags = static_cast<SDL_WindowFlags>(window_flags | SDL_WINDOW_OPENGL);
#endif
        SDL_Window *window = SDL_CreateWindow("saga", host_window_width, host_window_height, window_flags);
        if (window == nullptr) {
            LOG_ERR("SDL_CreateWindow failed: %s", SDL_GetError());
            return;
        }

        if (strcmp(SDL_GetCurrentVideoDriver(), video_driver) != 0) {
            LOG_ERR("unexpected video driver: %s", SDL_GetCurrentVideoDriver());
            return;
        }

#if defined(__linux__) && !defined(__EMSCRIPTEN__)
        SDL_GLContext bootstrap_context = SDL_GL_CreateContext(window);
        if (bootstrap_context == nullptr) {
            LOG_ERR("SDL_GL_CreateContext failed: %s", SDL_GetError());
            return;
        }
        HostSetSDLGraphics(window, bootstrap_context);
#endif
        g_renderDevice.OnWindowCreated(HostPlatformNativeWindow(window));
    }

    static SDL_Thread *host_numain_thread = nullptr;
    static std::atomic<i32> host_numain_result{0};
    static std::atomic<bool> host_numain_done{false};

    static void host_resume_game_complete() {
        // ResumeGame clears the menu stack in the middle of the current game
        // tick. Keep host-side menu queries disabled until DrawMenu validates
        // another active menu.
        MenuValidated = 0;
    }

    static i32 host_menu_id() {
        // GetMenuID is an original game helper and assumes the menu system has
        // already been initialized. The host thread starts polling before
        // NuMain reaches APIInitMenu, so do that host-only lifetime check here.
        if (GameMenuLevel < 0 || GameMenuLevel >= 10) {
            return -1;
        }
        return GetMenuID();
    }

    static int SDLCALL host_numain_thread_main(void *arg) {
        (void)arg;
        char *argv[] = {const_cast<char *>("saga"), nullptr};
        const i32 result = NuMain(1, argv);
        host_numain_result.store(result, std::memory_order_relaxed);
        host_numain_done.store(true, std::memory_order_release);
        return result;
    }

} // namespace

i32 host_run_window(const HostWindowOptions &options) {
    HostSetReadbackEnabled(options.capture);
    HostSetFpsOverlayEnabled(options.show_fps);
    HostSetMsaaEnabled(options.msaa);
    HostFreeCameraConfigure(options.camera_free);
    NuPortalEnabled(options.portals ? 1 : 0);
    host_sdl_init(options.offscreen, options.mute, options.msaa);
    const char *documents_path = options.documents_path != nullptr ? options.documents_path : "res/";
    char scripted_documents_path[256];
    if (options.script_input) {
        SDL_CreateDirectory(".work/host-documents-scripted");
        snprintf(scripted_documents_path, sizeof(scripted_documents_path), ".work/host-documents-scripted/%llu/",
                 static_cast<unsigned long long>(SDL_GetTicksNS()));
        documents_path = scripted_documents_path;
    }
    if (!SDL_CreateDirectory(documents_path)) {
        LOG_ERR("failed to create host documents directory %s: %s", documents_path, SDL_GetError());
    }
    HostSetDocumentsPath(documents_path);

    void *buffer = malloc(0x1000000);
    VARIPTR ptr = VARIPTR{.void_ptr = buffer};
    NUDATHDR *dat = NuDatOpen("res/main.1060.com.wb.lego.tcs.obb", &ptr, 0);
    if (dat == nullptr) {
        LOG_ERR("failed to open res/main.1060.com.wb.lego.tcs.obb");
        free(buffer);
        return 1;
    }
    NuDatSet(dat);

    if (options.capture) {
        SDL_CreateDirectory(".work/capture");
        snprintf(host_capture_directory, sizeof(host_capture_directory), ".work/capture/run_%llu",
                 static_cast<unsigned long long>(SDL_GetTicksNS()));
        if (!SDL_CreateDirectory(host_capture_directory)) {
            LOG_ERR("failed to create capture directory %s: %s", host_capture_directory, SDL_GetError());
        } else {
            LOG_INFO("capturing frames under %s", host_capture_directory);
        }
    }

    // AndroidMain performs the process-side lifecycle after the activity has
    // supplied its surface: platform, screen, render device, then NuMain.
    NuPlatform::Create();
    NuScreen::Create();
    g_renderDevice.Initialize();

    // Mesa advertises S3TC, but this bundled OBB contains Android ETC1 assets.
    // The host upload boundary decodes ETC1 when the driver cannot upload it.
    NuPlatform::Get()->SetCurrentPlatform(ANDROID_ETC1_PLATFORM);
    HostInputReset();
    ResumeGame_ExtraCodeFn = host_resume_game_complete;

    host_numain_result.store(0, std::memory_order_relaxed);
    host_numain_done.store(false, std::memory_order_relaxed);
    host_numain_thread = SDL_CreateThread(host_numain_thread_main, "numain", nullptr);
    if (host_numain_thread == nullptr) {
        LOG_ERR("failed to create NuMain thread: %s", SDL_GetError());
        free(buffer);
        return 1;
    }

    const Uint64 start_ticks = SDL_GetTicks();
    i32 frame_count = 0;
    u64 previous_hash = 0;
    u64 captured_hash = 0;
    Uint64 last_capture_ticks = 0;
    Uint64 last_change_ticks = 0;
    Uint64 next_readback_ticks = 0;
    bool have_hash = false;
    bool saw_visible_frame = false;
    bool image_changing = false;
    HostScriptedInputStage scripted_stage = HostScriptedInputStage::title;
    Uint64 scripted_stage_ticks = 0;
    Uint64 scripted_jump_ready_wait_ticks = 0;
    i32 scripted_last_menu = -2;
    Uint64 scripted_menu_since = 0;
    HostScriptedPlaySnapshot scripted_play_before{};
    HostScriptedPlaySnapshot scripted_play_during{};
    HostScriptedPlaySnapshot scripted_play_after{};
    HostScriptedPlaySnapshot scripted_play_jump_start{};
    HostScriptedPlaySnapshot scripted_play_jump_ascent{};
    HostScriptedPlaySnapshot scripted_play_jump_landed{};
    HostScriptedPlaySnapshot scripted_action_active{};
    HostScriptedPlaySnapshot scripted_action_released{};
    bool scripted_play_started = false;
    bool scripted_play_finished = false;
    bool scripted_play_movement_observed = false;
    bool scripted_play_jump_observed = false;
    bool scripted_action_observed = false;
    bool scripted_action_release_observed = false;
    i16 scripted_action = -1;
    i16 scripted_pause_initial_row = 0;
    bool scripted_play_input_held = false;
    bool camera_orbit_started = false;
    bool camera_orbit_finished = false;
    Uint64 camera_orbit_start_ticks = 0;
    f32 camera_orbit_base_yaw = 0.0f;
    u32 escape_held_button = 0;
    std::vector<u8> pixels;
    i32 capture_width = 0;
    i32 capture_height = 0;

    bool quit_requested = false;
    Uint64 movement_trace_ticks = 0;
    while (!quit_requested) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (options.offscreen) {
                continue;
            }
            if (event.type == SDL_EVENT_QUIT) {
                quit_requested = true;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    if (escape_held_button == 0) {
                        const i32 menu_id = host_menu_id();
                        const bool opens_pause = Paused == 0 && menu_id == -1;
                        const bool resumes_pause = Paused != 0 && menu_id == LEGO_MENU_PAUSE_MAIN;
                        escape_held_button = opens_pause || resumes_pause ? GAMEPAD_START : GAMEPAD_TAG;
                    }
                }
            } else if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_ESCAPE) {
                    escape_held_button = 0;
                }
            } else if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                escape_held_button = 0;
            }
            HostPlatformHandleInputEvent(event, host_window_width, host_window_height);
        }
        if (quit_requested) {
            break;
        }

        u32 keyboard_buttons = escape_held_button;
        if (!options.offscreen) {
            const bool *keyboard = SDL_GetKeyboardState(nullptr);
            keyboard_buttons |= HostPlatformKeyboardButtons(keyboard);
        }
        HostInputSetKeyboardHeld(0, keyboard_buttons);

        if (options.camera_free) {
            const bool *keyboard = SDL_GetKeyboardState(nullptr);
            u32 controls = 0;
            controls |= keyboard[SDL_SCANCODE_KP_4] ? HOST_FREE_CAMERA_NUMPAD_4 : 0;
            controls |= keyboard[SDL_SCANCODE_KP_5] ? HOST_FREE_CAMERA_NUMPAD_5 : 0;
            controls |= keyboard[SDL_SCANCODE_KP_6] ? HOST_FREE_CAMERA_NUMPAD_6 : 0;
            controls |= keyboard[SDL_SCANCODE_KP_8] ? HOST_FREE_CAMERA_NUMPAD_8 : 0;
            controls |= keyboard[SDL_SCANCODE_LSHIFT] || keyboard[SDL_SCANCODE_RSHIFT] ? HOST_FREE_CAMERA_SHIFT : 0;
            HostFreeCameraSetControls(controls);
            HostFreeCameraSetReady(host_scripted_play_ready());
        }

        if (host_numain_done.load(std::memory_order_acquire)) {
            for (i32 i = 0; i < host_tail_frames; i++) {
                SDL_Delay(host_poll_interval_ms);
                frame_count++;
            }
            break;
        }

        frame_count++;

        const Uint64 elapsed_ticks = SDL_GetTicks() - start_ticks;
        if (options.trace_movement && elapsed_ticks - movement_trace_ticks >= 500) {
            movement_trace_ticks = elapsed_ticks;
            const GameObject_s *player = Player[0];
            if (player != nullptr && (player->apiobj.field_0x1f8 & 0x1001) == 0x1001 && host_menu_id() == -1) {
                const GAMEPAD_s *pad = player->pad_gamepad;
                const APIOBJECT &api = player->apiobj;
                LOG_INFO("movement trace: t=%llu dt=%.4f context=%d action=%d anim=%d "
                         "input=%.3f keys=0x%x buttons=0x%x pad-flags=0x%x "
                         "pos=(%.3f,%.3f,%.3f) velocity=(%.3f,%.3f,%.3f) "
                         "target=(%.3f,%.3f,%.3f) external=(%.3f,%.3f) "
                         "ground=%u surface=%d facing=%u input-angle=%u flags=(%x,%x)",
                         static_cast<unsigned long long>(elapsed_ticks), FRAMETIME, player->character_context,
                         player->action_movement_state, player->context_animation,
                         pad != nullptr ? pad->input_magnitude : 0.0f, keyboard_buttons,
                         pad != nullptr ? pad->buttons_held : 0, pad != nullptr ? pad->allocated_5a : 0, api.position.x,
                         api.position.y, api.position.z, api.velocity.x, api.velocity.y, api.velocity.z,
                         player->target_velocity.x, player->target_velocity.y, player->target_velocity.z,
                         api.movement_direction.x, api.movement_direction.z, api.field_0x27d,
                         static_cast<i8>(api.field_0x281), api.facing_angle, player->current_input_angle,
                         player->field_0xe20, player->field_0xefd);
            }
        }
        if (options.script_input) {
            const i32 menu_id = host_menu_id();
            if (menu_id != scripted_last_menu) {
                scripted_last_menu = menu_id;
                scripted_menu_since = elapsed_ticks;
            }

            if (scripted_stage == HostScriptedInputStage::title && menu_id == 0 &&
                elapsed_ticks >= host_scripted_title_input_ms && GameTimer.time_elapsed >= 4.0f) {
                // This deliberately sends one edge. A connected controller's
                // first press must reach the title menu rather than being
                // consumed by host device discovery/remapping.
                HostInputTap(0, GAMEPAD_START | GAMEPAD_JUMP);
                scripted_stage = HostScriptedInputStage::new_or_load;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::new_or_load && menu_id == 1 &&
                       elapsed_ticks - scripted_menu_since >= host_scripted_menu_settle_ms) {
                // The original menu defaults to Load Game when any save is
                // present. This route deliberately exercises New Game.
                const HostScriptedMenuAction action =
                    host_scripted_menu_select(options.script_load ? 1 : 0, 0, elapsed_ticks, scripted_stage_ticks);
                if (action == HostScriptedMenuAction::confirmed && options.script_load) {
                    // Load Game may lead to the original No Data screen
                    // (menu 1005); dismiss it once it has settled.
                    scripted_stage = HostScriptedInputStage::load_wait;
                } else if (action == HostScriptedMenuAction::confirmed) {
                    // New Game now follows the original Select Controls
                    // screen (menu 33). Its default entry is Classic, so
                    // confirm it after the normal settle interval before
                    // looking for the save-slot menu.
                    scripted_stage = HostScriptedInputStage::select_controls;
                }
            } else if (scripted_stage == HostScriptedInputStage::select_controls && menu_id == 33 &&
                       elapsed_ticks - scripted_menu_since >= host_scripted_menu_settle_ms) {
                if (host_scripted_menu_select(0, 0, elapsed_ticks, scripted_stage_ticks) ==
                    HostScriptedMenuAction::confirmed) {
                    scripted_stage = HostScriptedInputStage::save_slot;
                }
            } else if (scripted_stage == HostScriptedInputStage::select_controls && menu_id == 1000) {
                // A connected keyboard-backed gamepad follows the original
                // controller path directly to save slots. Touch-only mode
                // visits menu 33 first; support both without forcing either.
                scripted_stage = HostScriptedInputStage::save_slot;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::load_wait && menu_id == 1012 &&
                       elapsed_ticks - scripted_menu_since >= host_scripted_menu_settle_ms) {
                i32 slot = 0;
                while (slot < SAVESLOTS && saveload_slotused[slot] == 0) {
                    ++slot;
                }
                if (slot < SAVESLOTS) {
                    if (host_scripted_menu_select(0, slot, elapsed_ticks, scripted_stage_ticks) ==
                        HostScriptedMenuAction::confirmed) {
                        scripted_stage = HostScriptedInputStage::complete;
                        scripted_stage_ticks = elapsed_ticks;
                    }
                }
            } else if (scripted_stage == HostScriptedInputStage::load_wait && menu_id == 1005 &&
                       elapsed_ticks - scripted_menu_since >= host_scripted_menu_settle_ms) {
                HostInputTap(0, GAMEPAD_JUMP);
                scripted_stage = HostScriptedInputStage::complete;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::save_slot && menu_id == 1000 &&
                       elapsed_ticks - scripted_menu_since >= host_scripted_menu_settle_ms) {
                i32 slot = 0;
                while (slot < SAVESLOTS && saveload_slotused[slot] != 0) {
                    ++slot;
                }
                const bool overwrite = slot == SAVESLOTS;
                if (overwrite) {
                    slot = 0;
                }
                if (host_scripted_menu_select(0, slot, elapsed_ticks, scripted_stage_ticks) ==
                    HostScriptedMenuAction::confirmed) {
                    scripted_stage = overwrite ? HostScriptedInputStage::overwrite_confirm
                                               : (options.script_play ? HostScriptedInputStage::cantina_wait
                                                                      : HostScriptedInputStage::complete);
                }
            } else if (scripted_stage == HostScriptedInputStage::overwrite_confirm && menu_id == 1008 &&
                       elapsed_ticks - scripted_menu_since >= host_scripted_menu_settle_ms) {
                if (host_scripted_menu_select(0, 0, elapsed_ticks, scripted_stage_ticks) ==
                    HostScriptedMenuAction::confirmed) {
                    scripted_stage =
                        options.script_play ? HostScriptedInputStage::cantina_wait : HostScriptedInputStage::complete;
                }
            } else if (scripted_stage == HostScriptedInputStage::cantina_wait && host_scripted_play_ready() &&
                       (options.script_action ? Player[0]->apiobj.field_0x27d != 0 &&
                                                    Player[0]->character_context == CHARACTER_CONTEXT_NONE
                                              : host_player_can_start_jump(Player[0]))) {
                scripted_play_before = host_scripted_play_snapshot();
                host_log_camera_trace("before", scripted_play_before);
                host_log_animation_trace("before", scripted_play_before);
                host_log_portal_trace("before");
                host_log_display_list_trace();
                host_log_pickup_trace("before");
                scripted_play_started = true;
                const CHARACTERMODEL_s *player_model = Player[0]->apiobj.character_model;
                LOG_INFO("scripted play: cantina ready level=%s idx=%d area=%d player=%p pad=%p pad0=%p "
                         "flags=(state=0x%x,motion=0x%x,contact=0x%x,terrain=0x%x) "
                         "floor=(shadow=%.3f,bottom=%.3f,radius=%.3f) position=(%.3f,%.3f,%.3f) "
                         "velocity=(%.3f,%.3f,%.3f) yrot=%u "
                         "input=(held=0x%x,magnitude=%.3f,speed=%.3f) objects=(base=%p,high=%d,index=%d) "
                         "render=(model=%p,hierarchy=%p,draw=%u,mode=%u,scale=%.3f) "
                         "frametime=%.6f camera=(%.3f,%.3f,%.3f)->(%.3f,%.3f,%.3f) "
                         "angles=(%u,%u)->(%u,%u) seek=(%.3f,%.3f) mode=%d/%d",
                         WORLD->current_level->name, WORLD->current_level->idx, WORLD->area->index, Player[0],
                         Player[0]->pad_gamepad, &GamePad[0], scripted_play_before.player_flags,
                         scripted_play_before.player_motion_flags, Player[0]->apiobj.field_0x27d,
                         Player[0]->field_0x105c, Player[0]->apiobj.field_0x218, Player[0]->apiobj.collision_min.y,
                         Player[0]->apiobj.collision_radius, scripted_play_before.player_position.x,
                         scripted_play_before.player_position.y, scripted_play_before.player_position.z,
                         scripted_play_before.player_velocity.x, scripted_play_before.player_velocity.y,
                         scripted_play_before.player_velocity.z, scripted_play_before.player_yrot,
                         scripted_play_before.buttons_held, scripted_play_before.input_magnitude,
                         scripted_play_before.movement_speed, Obj, HIGHGAMEOBJECT, scripted_play_before.object_index,
                         player_model, player_model != nullptr ? player_model->hierarchy : nullptr,
                         Player[0]->apiobj.model_draw_result, Player[0]->field_0x1086, Player[0]->apiobj.field_0xa8,
                         scripted_play_before.frame_time, scripted_play_before.camera_position.x,
                         scripted_play_before.camera_position.y, scripted_play_before.camera_position.z,
                         scripted_play_before.camera_target.x, scripted_play_before.camera_target.y,
                         scripted_play_before.camera_target.z, scripted_play_before.camera_pitch,
                         scripted_play_before.camera_yaw, scripted_play_before.camera_desired_pitch,
                         scripted_play_before.camera_desired_yaw, scripted_play_before.camera_position_seek,
                         scripted_play_before.camera_angle_seek, scripted_play_before.camera_mode,
                         scripted_play_before.camera_previous_mode);
                if (options.script_action) {
                    LOG_INFO("scripted action: tapping GAMEPAD_ACTION (0x%x)", GAMEPAD_ACTION);
                    HostInputTap(0, GAMEPAD_ACTION);
                    scripted_stage = HostScriptedInputStage::action_wait_entry;
                } else {
                    scripted_play_jump_start = scripted_play_before;
                    HostInputTap(0, GAMEPAD_JUMP);
                    scripted_stage = HostScriptedInputStage::play_jump_ascent;
                }
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::cantina_wait && !options.script_action &&
                       host_scripted_play_ready()) {
                if (scripted_jump_ready_wait_ticks == 0) {
                    scripted_jump_ready_wait_ticks = elapsed_ticks;
                } else if (elapsed_ticks >= scripted_jump_ready_wait_ticks + host_scripted_play_jump_ready_timeout_ms) {
                    LOG_ERR("scripted play: player did not become jump-eligible within %llu ms; "
                            "position-y=%.3f velocity-y=%.3f contact=0x%x terrain=0x%x "
                            "contact-grace=%.3f context=%d",
                            static_cast<unsigned long long>(host_scripted_play_jump_ready_timeout_ms),
                            Player[0]->apiobj.position.y, Player[0]->apiobj.velocity.y, Player[0]->apiobj.field_0x27d,
                            Player[0]->field_0x105c, Player[0]->ground_contact_grace_timer,
                            Player[0]->character_context);
                    scripted_play_finished = true;
                    scripted_stage = HostScriptedInputStage::complete;
                    scripted_stage_ticks = elapsed_ticks;
                }
            } else if (scripted_stage == HostScriptedInputStage::action_wait_entry && Player[0] != nullptr) {
                const ANIMPACKET_s &animation = Player[0]->apiobj.anim_packet;
                const i16 selected_action = Player[0]->context_animation;
                const bool animation_active =
                    animation.animation_index == selected_action || animation.requested_animation == selected_action;
                if (Player[0]->character_context == CHARACTER_CONTEXT_COMBO &&
                    host_is_jedi_combo_action(selected_action) &&
                    Player[0]->queued_context_animation == selected_action && animation_active) {
                    scripted_action = selected_action;
                    scripted_action_active = host_scripted_play_snapshot();
                    scripted_action_observed = true;
                    LOG_INFO("scripted action: active context=%d action=%d queued=%d animation=%d/%d flags=0x%x",
                             scripted_action_active.player_context, scripted_action_active.context_animation,
                             scripted_action_active.queued_context_animation, scripted_action_active.animation_current,
                             scripted_action_active.animation_requested, scripted_action_active.animation_flags);
                    const GAMECHARACTERDATA *weapon_data =
                        static_cast<const GAMECHARACTERDATA *>(Player[0]->apiobj.character_data->field11_0x24);
                    LOG_INFO("scripted weapon: scale=%.3f model=%d joints=(%d,%d,%d,%d) color=%u flags=0x%x "
                             "models-active=(hilt=%u,green=%u,blue=%u)",
                             Player[0]->weapon_scale, weapon_data->weapon_model, weapon_data->weapon_joints[0],
                             weapon_data->weapon_joints[1], weapon_data->weapon_joints[2],
                             weapon_data->weapon_joints[3], weapon_data->field_0x117, weapon_data->field_0x94,
                             WORLD->lev_objs[0x11].active, WORLD->lev_objs[0x67].active, WORLD->lev_objs[0x69].active);
                    scripted_stage = HostScriptedInputStage::action_wait_release;
                    scripted_stage_ticks = elapsed_ticks;
                } else if (elapsed_ticks >= scripted_stage_ticks + host_scripted_action_entry_timeout_ms) {
                    LOG_ERR("scripted action: combo context/action did not become active within %llu ms; "
                            "context=%d selected=%d queued=%d animation=%d/%d contact=0x%x terrain=0x%x "
                            "buttons=(held=0x%x,pressed=0x%x) flags=(api=0x%x,e20=0x%x,e22=0x%x)",
                            static_cast<unsigned long long>(host_scripted_action_entry_timeout_ms),
                            Player[0]->character_context, Player[0]->context_animation,
                            Player[0]->queued_context_animation, animation.animation_index,
                            animation.requested_animation, Player[0]->apiobj.field_0x27d, Player[0]->field_0x105c,
                            Player[0]->pad_gamepad->buttons_held, Player[0]->pad_gamepad->buttons_pressed,
                            Player[0]->apiobj.flags_low, Player[0]->field_0xe20, Player[0]->field_0xe22);
                    scripted_play_finished = true;
                    scripted_stage = HostScriptedInputStage::complete;
                    scripted_stage_ticks = elapsed_ticks;
                }
            } else if (scripted_stage == HostScriptedInputStage::action_wait_release && Player[0] != nullptr) {
                if (Player[0]->character_context == CHARACTER_CONTEXT_NONE &&
                    (Player[0]->field_0xe20 & GAMEOBJECT_E20_FLAG_COMBO_MOVEMENT) == 0) {
                    scripted_action_released = host_scripted_play_snapshot();
                    scripted_action_release_observed = scripted_action_observed &&
                                                       host_is_jedi_combo_action(scripted_action) &&
                                                       host_finite_vec(scripted_action_released.player_position);
                    LOG_INFO("scripted action: released action=%d context=%d animation=%d/%d "
                             "combo-movement=%d observed=%d",
                             scripted_action, scripted_action_released.player_context,
                             scripted_action_released.animation_current, scripted_action_released.animation_requested,
                             (Player[0]->field_0xe20 & GAMEOBJECT_E20_FLAG_COMBO_MOVEMENT) != 0,
                             scripted_action_release_observed ? 1 : 0);
                    scripted_play_finished = true;
                    scripted_stage = HostScriptedInputStage::complete;
                    scripted_stage_ticks = elapsed_ticks;
                } else if (elapsed_ticks >= scripted_stage_ticks + host_scripted_action_release_timeout_ms) {
                    LOG_ERR("scripted action: combo did not release within %llu ms; context=%d action=%d "
                            "animation=%d/%d flags=0x%x",
                            static_cast<unsigned long long>(host_scripted_action_release_timeout_ms),
                            Player[0]->character_context, scripted_action,
                            Player[0]->apiobj.anim_packet.animation_index,
                            Player[0]->apiobj.anim_packet.requested_animation, Player[0]->apiobj.anim_packet.flags);
                    scripted_play_finished = true;
                    scripted_stage = HostScriptedInputStage::complete;
                    scripted_stage_ticks = elapsed_ticks;
                }
            } else if (scripted_stage == HostScriptedInputStage::play_move &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_play_move_ms) {
                scripted_play_during = host_scripted_play_snapshot();
                host_log_camera_trace("during", scripted_play_during);
                host_log_animation_trace("during", scripted_play_during);
                LOG_INFO("scripted play: during DUP flags=(state=0x%x,motion=0x%x) "
                         "position=(%.3f,%.3f,%.3f) velocity=(%.3f,%.3f,%.3f) "
                         "input=(held=0x%x,magnitude=%.3f,speed=%.3f) object-index=%d frametime=%.6f "
                         "camera=(%.3f,%.3f,%.3f)->(%.3f,%.3f,%.3f) angles=(%u,%u)->(%u,%u) "
                         "seek=(%.3f,%.3f) mode=%d/%d",
                         scripted_play_during.player_flags, scripted_play_during.player_motion_flags,
                         scripted_play_during.player_position.x, scripted_play_during.player_position.y,
                         scripted_play_during.player_position.z, scripted_play_during.player_velocity.x,
                         scripted_play_during.player_velocity.y, scripted_play_during.player_velocity.z,
                         scripted_play_during.buttons_held, scripted_play_during.input_magnitude,
                         scripted_play_during.movement_speed, scripted_play_during.object_index,
                         scripted_play_during.frame_time, scripted_play_during.camera_position.x,
                         scripted_play_during.camera_position.y, scripted_play_during.camera_position.z,
                         scripted_play_during.camera_target.x, scripted_play_during.camera_target.y,
                         scripted_play_during.camera_target.z, scripted_play_during.camera_pitch,
                         scripted_play_during.camera_yaw, scripted_play_during.camera_desired_pitch,
                         scripted_play_during.camera_desired_yaw, scripted_play_during.camera_position_seek,
                         scripted_play_during.camera_angle_seek, scripted_play_during.camera_mode,
                         scripted_play_during.camera_previous_mode);
                HostInputSetHeld(0, 0);
                scripted_play_input_held = false;
                scripted_stage = HostScriptedInputStage::play_settle;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::play_settle &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_play_settle_ms) {
                scripted_play_after = host_scripted_play_snapshot();
                host_log_camera_trace("after", scripted_play_after);
                host_log_animation_trace("after", scripted_play_after);
                const NUVEC player_delta = {
                    scripted_play_after.player_position.x - scripted_play_before.player_position.x,
                    scripted_play_after.player_position.y - scripted_play_before.player_position.y,
                    scripted_play_after.player_position.z - scripted_play_before.player_position.z,
                };
                const NUVEC camera_delta = {
                    scripted_play_after.camera_position.x - scripted_play_before.camera_position.x,
                    scripted_play_after.camera_position.y - scripted_play_before.camera_position.y,
                    scripted_play_after.camera_position.z - scripted_play_before.camera_position.z,
                };
                const f32 player_delta_squared =
                    player_delta.x * player_delta.x + player_delta.y * player_delta.y + player_delta.z * player_delta.z;
                scripted_play_movement_observed = player_delta_squared > 0.0001f;
                LOG_INFO("scripted play: held DUP for %llu ms; "
                         "after position=(%.3f,%.3f,%.3f) velocity=(%.3f,%.3f,%.3f) yrot=%u "
                         "input=(held=0x%x,magnitude=%.3f,speed=%.3f) camera=(%.3f,%.3f,%.3f); "
                         "deltas player=(%.3f,%.3f,%.3f) yrot=%d camera=(%.3f,%.3f,%.3f); "
                         "camera-target=(%.3f,%.3f,%.3f) angles=(%u,%u)->(%u,%u) seek=(%.3f,%.3f) "
                         "mode=%d/%d movement_observed=%d",
                         static_cast<unsigned long long>(host_scripted_play_move_ms),
                         scripted_play_after.player_position.x, scripted_play_after.player_position.y,
                         scripted_play_after.player_position.z, scripted_play_after.player_velocity.x,
                         scripted_play_after.player_velocity.y, scripted_play_after.player_velocity.z,
                         scripted_play_after.player_yrot, scripted_play_after.buttons_held,
                         scripted_play_after.input_magnitude, scripted_play_after.movement_speed,
                         scripted_play_after.camera_position.x, scripted_play_after.camera_position.y,
                         scripted_play_after.camera_position.z, player_delta.x, player_delta.y, player_delta.z,
                         static_cast<i32>(scripted_play_after.player_yrot) -
                             static_cast<i32>(scripted_play_before.player_yrot),
                         camera_delta.x, camera_delta.y, camera_delta.z, scripted_play_after.camera_target.x,
                         scripted_play_after.camera_target.y, scripted_play_after.camera_target.z,
                         scripted_play_after.camera_pitch, scripted_play_after.camera_yaw,
                         scripted_play_after.camera_desired_pitch, scripted_play_after.camera_desired_yaw,
                         scripted_play_after.camera_position_seek, scripted_play_after.camera_angle_seek,
                         scripted_play_after.camera_mode, scripted_play_after.camera_previous_mode,
                         scripted_play_movement_observed ? 1 : 0);
                if (options.script_pause) {
                    HostInputTap(0, GAMEPAD_START);
                    scripted_stage = HostScriptedInputStage::pause_wait;
                } else {
                    scripted_play_finished = true;
                    scripted_stage = HostScriptedInputStage::complete;
                }
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::play_jump_ascent &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_play_jump_ascent_ms) {
                scripted_play_jump_ascent = host_scripted_play_snapshot();
                host_log_animation_trace("jump-ascent", scripted_play_jump_ascent);
                const f32 ascent =
                    scripted_play_jump_ascent.player_position.y - scripted_play_jump_start.player_position.y;
                const bool jump_animation = scripted_play_jump_ascent.animation_current == 6 ||
                                            scripted_play_jump_ascent.animation_requested == 6;
                scripted_play_jump_observed = ascent > 0.02f && scripted_play_jump_ascent.player_velocity.y > 0.0f &&
                                              scripted_play_jump_ascent.player_context == 0 && jump_animation;
                LOG_INFO("scripted play: jump ascent position-y=%.3f delta-y=%.3f velocity-y=%.3f "
                         "context=%d animation=%d/%d observed=%d",
                         scripted_play_jump_ascent.player_position.y, ascent,
                         scripted_play_jump_ascent.player_velocity.y, scripted_play_jump_ascent.player_context,
                         scripted_play_jump_ascent.animation_current, scripted_play_jump_ascent.animation_requested,
                         scripted_play_jump_observed ? 1 : 0);
                HostInputTap(0, GAMEPAD_JUMP);
                scripted_stage = HostScriptedInputStage::play_jump_second;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::play_jump_second &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_play_second_jump_ms) {
                const HostScriptedPlaySnapshot second_jump = host_scripted_play_snapshot();
                const bool second_jump_animation =
                    second_jump.animation_current == static_cast<i16>(HostJumpAction::second_jump) ||
                    second_jump.animation_requested == static_cast<i16>(HostJumpAction::second_jump) ||
                    second_jump.animation_current == static_cast<i16>(HostJumpAction::third_jump) ||
                    second_jump.animation_requested == static_cast<i16>(HostJumpAction::third_jump);
                const bool second_jump_observed = Player[0]->jump_sequence == 2 &&
                                                  second_jump.player_context == CHARACTER_CONTEXT_JUMP &&
                                                  second_jump.player_velocity.y > 0.0f && second_jump_animation;
                scripted_play_jump_observed = scripted_play_jump_observed && second_jump_observed;
                LOG_INFO("scripted play: second jump position-y=%.3f velocity-y=%.3f sequence=%u "
                         "context=%d animation=%d/%d observed=%d",
                         second_jump.player_position.y, second_jump.player_velocity.y, Player[0]->jump_sequence,
                         second_jump.player_context, second_jump.animation_current, second_jump.animation_requested,
                         second_jump_observed ? 1 : 0);
                scripted_stage = HostScriptedInputStage::play_jump_land;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::play_jump_land &&
                       Player[0]->character_context == CHARACTER_CONTEXT_NONE && Player[0]->apiobj.field_0x27d != 0) {
                scripted_play_jump_landed = host_scripted_play_snapshot();
                host_log_animation_trace("jump-landed", scripted_play_jump_landed);
                const f32 landing_height_delta =
                    scripted_play_jump_landed.player_position.y - scripted_play_jump_start.player_position.y;
                const bool land_animation = host_is_jump_landing_action(scripted_play_jump_landed.animation_current) ||
                                            host_is_jump_landing_action(scripted_play_jump_landed.animation_previous) ||
                                            host_is_jump_landing_action(scripted_play_jump_landed.animation_requested);
                scripted_play_jump_observed = scripted_play_jump_observed &&
                                              std::isfinite(scripted_play_jump_landed.player_position.y) &&
                                              scripted_play_jump_landed.player_velocity.y <= 0.0f &&
                                              scripted_play_jump_landed.player_velocity.y > -0.5f &&
                                              std::fabs(landing_height_delta) < 0.6f && land_animation;
                LOG_INFO("scripted play: jump landed position-y=%.3f delta-y=%.3f velocity-y=%.3f "
                         "context=%d animation=%d/%d observed=%d",
                         scripted_play_jump_landed.player_position.y, landing_height_delta,
                         scripted_play_jump_landed.player_velocity.y, scripted_play_jump_landed.player_context,
                         scripted_play_jump_landed.animation_current, scripted_play_jump_landed.animation_requested,
                         scripted_play_jump_observed ? 1 : 0);
                // From the fixed cantina spawn/camera, forward crosses the nearest
                // world pickup. This exercises the normal collision and HUD path.
                HostInputSetHeld(0, GAMEPAD_DUP);
                scripted_play_input_held = true;
                scripted_stage = HostScriptedInputStage::play_move;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::play_jump_land &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_play_jump_timeout_ms) {
                const HostScriptedPlaySnapshot timeout = host_scripted_play_snapshot();
                LOG_ERR("scripted play: jump did not land within %llu ms; "
                        "position-y=%.3f delta-y=%.3f velocity-y=%.3f contact=0x%x context=%d "
                        "animation=%d/%d time=%.3f flags=0x%x",
                        static_cast<unsigned long long>(host_scripted_play_jump_timeout_ms), timeout.player_position.y,
                        timeout.player_position.y - scripted_play_jump_start.player_position.y,
                        timeout.player_velocity.y, Player[0]->apiobj.field_0x27d, timeout.player_context,
                        timeout.animation_current, timeout.animation_requested, timeout.animation_time,
                        timeout.animation_flags);
                scripted_play_jump_observed = false;
                scripted_play_finished = true;
                scripted_stage = HostScriptedInputStage::complete;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::pause_wait && Paused != 0 &&
                       host_menu_id() == LEGO_MENU_PAUSE_MAIN) {
                LOG_INFO("scripted play: pause menu opened id=%d pad=%d", host_menu_id(), pause_i_pad);
                scripted_pause_initial_row = GameMenu[GameMenuLevel].selected_row;
                scripted_stage = HostScriptedInputStage::pause_settle;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::pause_settle &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_pause_settle_ms) {
                HostInputTap(0, GAMEPAD_DDOWN);
                scripted_stage = HostScriptedInputStage::pause_move_wait;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::pause_move_wait &&
                       GameMenu[GameMenuLevel].selected_row != scripted_pause_initial_row) {
                LOG_INFO("scripted play: pause menu moved row=%d->%d", scripted_pause_initial_row,
                         GameMenu[GameMenuLevel].selected_row);
                HostInputTap(0, GAMEPAD_DUP);
                scripted_stage = HostScriptedInputStage::pause_return_wait;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::pause_return_wait &&
                       GameMenu[GameMenuLevel].selected_row == scripted_pause_initial_row &&
                       elapsed_ticks >= scripted_stage_ticks + host_scripted_pause_settle_ms) {
                HostInputTap(0, GAMEPAD_START);
                scripted_stage = HostScriptedInputStage::resume_wait;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::resume_wait && Paused == 0 && host_menu_id() == -1) {
                LOG_INFO("scripted play: resumed from pause menu");
                scripted_play_finished = true;
                scripted_stage = HostScriptedInputStage::complete;
                scripted_stage_ticks = elapsed_ticks;
            } else if (scripted_stage == HostScriptedInputStage::complete && !options.camera_free &&
                       (!options.camera_orbit || camera_orbit_finished) &&
                       elapsed_ticks >= scripted_stage_ticks + options.script_tail_ms) {
                break;
            }
        }

        if (options.camera_orbit && !camera_orbit_finished && GameCam != nullptr &&
            (camera_orbit_started || host_scripted_play_ready())) {
            if (!camera_orbit_started) {
                camera_orbit_started = true;
                camera_orbit_start_ticks = elapsed_ticks;
                camera_orbit_base_yaw = GameCam->field_0x218;
                LOG_INFO("host camera orbit: starting 360-degree yaw over %llu ms",
                         static_cast<unsigned long long>(host_camera_orbit_duration_ms));
            }

            const Uint64 orbit_elapsed = elapsed_ticks - camera_orbit_start_ticks;
            const Uint64 clamped_elapsed =
                orbit_elapsed < host_camera_orbit_duration_ms ? orbit_elapsed : host_camera_orbit_duration_ms;
            const f32 progress = static_cast<f32>(clamped_elapsed) / static_cast<f32>(host_camera_orbit_duration_ms);
            GameCam->field_0x218 = camera_orbit_base_yaw - progress * host_full_camera_rotation;

            if (orbit_elapsed >= host_camera_orbit_duration_ms) {
                GameCam->field_0x218 = camera_orbit_base_yaw;
                camera_orbit_finished = true;
                LOG_INFO("host camera orbit: completed");
                host_log_portal_trace("after orbit");
            }
        }

        SDL_Delay(host_poll_interval_ms);

        if (SDL_GetTicks() - start_ticks > options.timeout_ms && options.timeout_ms > 0) {
            LOG_ERR("window utility: timeout after %llu ms", static_cast<unsigned long long>(options.timeout_ms));
            break;
        }

        if (!options.capture) {
            continue;
        }

        const Uint64 readback_ticks = SDL_GetTicks();
        if (readback_ticks < next_readback_ticks) {
            continue;
        }
        // glReadPixels synchronizes with the render thread. Sampling it every
        // host-loop iteration throttles the game itself, so inspect at 10 Hz
        // and retain the existing 500 ms capture cadence while images move.
        next_readback_ticks = readback_ticks + 100;
        HostRequestReadback();

        u64 current_hash = 0;
        if (!host_read_frame(pixels, capture_width, capture_height, current_hash)) {
            continue;
        }
        saw_visible_frame |= host_frame_has_visible_pixels(pixels.data(), static_cast<usize>(capture_width) *
                                                                              static_cast<usize>(capture_height));
        const Uint64 now = SDL_GetTicks();
        if (!have_hash) {
            host_capture_frame(frame_count, pixels, capture_width, capture_height);
            captured_hash = current_hash;
            last_capture_ticks = now;
            previous_hash = current_hash;
            have_hash = true;
        } else if (current_hash != previous_hash) {
            last_change_ticks = now;
            if (!image_changing || now - last_capture_ticks >= 500) {
                host_capture_frame(frame_count, pixels, capture_width, capture_height);
                captured_hash = current_hash;
                last_capture_ticks = now;
            }
            previous_hash = current_hash;
            image_changing = true;
        } else if (image_changing && now - last_change_ticks >= 500) {
            if (current_hash != captured_hash) {
                host_capture_frame(frame_count, pixels, capture_width, capture_height);
                captured_hash = current_hash;
                last_capture_ticks = now;
            }
            image_changing = false;
        }
    }

    if (scripted_play_input_held) {
        HostInputSetHeld(0, 0);
    }
    HostInputSetKeyboardHeld(0, 0);
    HostFreeCameraSetControls(0);

    if (options.capture) {
        u64 final_hash = 0;
        if (host_read_frame(pixels, capture_width, capture_height, final_hash)) {
            saw_visible_frame |= host_frame_has_visible_pixels(pixels.data(), static_cast<usize>(capture_width) *
                                                                                  static_cast<usize>(capture_height));
            if (!have_hash || final_hash != captured_hash) {
                host_capture_frame(frame_count, pixels, capture_width, capture_height);
            }
            have_hash = true;
        }
        if (!have_hash || !saw_visible_frame) {
            LOG_ERR("capture verification failed: framebuffer was %s",
                    have_hash ? "black for the entire run" : "never available");
        }
    }

    const bool numain_finished = host_numain_done.load(std::memory_order_acquire);
    if (host_numain_thread != nullptr && numain_finished) {
        i32 thread_result = 0;
        SDL_WaitThread(host_numain_thread, &thread_result);
        host_numain_result.store(thread_result, std::memory_order_relaxed);
        host_numain_thread = nullptr;
    } else if (host_numain_thread != nullptr) {
        SDL_DetachThread(host_numain_thread);
        host_numain_thread = nullptr;
    }

    if (options.script_input) {
        LOG_INFO("scripted input stopped on menu id %d, save slot %d, status %d, load=(%d,%d,%d,%d), occurred=%d, "
                 "new-level=%p idx=%d name=%s current-level=%p idx=%d name=%s fade=(%.3f,%d,%d), "
                 "waits=(level=%d,character=%d,new=%d,abort=%d,reset=%d), players=(%d:%p,%d:%p), char-load=%d",
                 host_menu_id(), saveload_slotid, saveload_status, memcard_loadneeded, memcard_loadstarted,
                 memcard_loadfailed, memcard_loadcorrupt, MenuLoadOccurred, NewLData,
                 NewLData != nullptr ? NewLData->idx : -1, NewLData != nullptr ? NewLData->name : "-",
                 WORLD != nullptr ? WORLD->current_level : nullptr,
                 WORLD != nullptr && WORLD->current_level != nullptr ? WORLD->current_level->idx : -1,
                 WORLD != nullptr && WORLD->current_level != nullptr ? WORLD->current_level->name : "-", FadeSys.fade,
                 FadeSys.pending_type, FadeSys.busy, waiting_for_level, waiting_for_character, waiting_for_new_level,
                 abort_load, reset_load, PlayerID[0], Player[0], PlayerID[1], Player[1], CharacterDataLoad);

        i32 active_objects = 0;
        i32 model_objects = 0;
        i32 drawn_objects = 0;
        for (i32 index = 0; Obj != nullptr && index < HIGHGAMEOBJECT; ++index) {
            const GameObject_s &object = Obj[index];
            if ((object.apiobj.field_0x1f8 & 1) == 0) {
                continue;
            }
            ++active_objects;
            if (object.apiobj.character_model != nullptr && object.apiobj.character_model->hierarchy != nullptr) {
                ++model_objects;
            }
            if (object.apiobj.model_draw_result != 0) {
                ++drawn_objects;
            }
        }
        LOG_INFO("scripted objects: high=%d active=%d models=%d drawn=%d", HIGHGAMEOBJECT, active_objects,
                 model_objects, drawn_objects);
        host_log_pickup_trace("after");
        std::vector<const AISCRIPT *> logged_ai_scripts;
        for (i32 index = 0; Obj != nullptr && index < HIGHGAMEOBJECT; ++index) {
            const GameObject_s &object = Obj[index];
            if ((object.apiobj.field_0x1f8 & 1) == 0) {
                continue;
            }
            const CHARACTERDATA *character = object.apiobj.character_data;
            const CHARACTERMODEL_s *model = object.apiobj.character_model;
            void **animations = model != nullptr ? model->model_data_b : nullptr;
            const PLAYERCHARACTERCONFIG_s *config = character != nullptr ? character->player_config : nullptr;
            LOG_INFO(
                "scripted object[%d]: id=%d file=%s pos=(%.3f,%.3f,%.3f) matrix=(%.3f,%.3f,%.3f) "
                "collision-y=(%.3f..%.3f) floor=%.3f bounds=(%.3f..%.3f) scale=%.3f "
                "mode=%u origin=%u context=%d target=%p layer=0x%x anim=%d/%d idle=(%d,%d,%.3f/%.3f) "
                "available=(0:%d,1:%d,25:%d,118:%d) flags=(0x%x,0x%x) origin-joints=(%d,%d)",
                index, object.id, character != nullptr && character->file != nullptr ? character->file : "-",
                object.apiobj.position.x, object.apiobj.position.y, object.apiobj.position.z,
                object.apiobj.field_0xb8.m30, object.apiobj.field_0xb8.m31, object.apiobj.field_0xb8.m32,
                object.apiobj.collision_min.y, object.apiobj.collision_max.y, object.apiobj.field_0x218,
                object.character_bottom, object.character_top, object.apiobj.field_0xa8, object.field_0x1086,
                object.use_model_origin, object.character_context, object.context_target_position, object.field_0x1054,
                object.apiobj.anim_packet.animation_index, object.apiobj.anim_packet.requested_animation,
                object.idle_animation, object.previous_idle_animation, object.idle_animation_time,
                object.idle_animation_limit, animations != nullptr && animations[0] != nullptr,
                animations != nullptr && animations[1] != nullptr, animations != nullptr && animations[25] != nullptr,
                animations != nullptr && animations[118] != nullptr, object.apiobj.field_0x1f8,
                object.apiobj.field_0x1f4, config != nullptr ? config->model_origin_joint : -2,
                config != nullptr ? config->collision_origin_joint : -2);

            if ((object.apiobj.field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) != 0) {
                const AISCRIPTPROCESS *processor = reinterpret_cast<const AISCRIPTPROCESS *>(&object.ai);
                const AIACTION *action = reinterpret_cast<const AIACTION *>(processor->action_node);
                LOG_INFO(
                    "scripted object[%d] AI: script=%s state=%s action=%s first=%u disabled=%u "
                    "movement=(flags=0x%x,runtime=0x%x,event=0x%x,speed=%u,target=%p) "
                    "destination=(%.3f,%.3f,%.3f) waypoint=(%.3f,%.3f,%.3f) pad=(%.3f,%u)",
                    index,
                    processor->script != nullptr && processor->script->name != nullptr ? processor->script->name : "-",
                    processor->state != nullptr && processor->state->name != nullptr ? processor->state->name : "-",
                    action != nullptr && action->def != nullptr && action->def->name != nullptr ? action->def->name
                                                                                                : "-",
                    processor->is_first_time_action, processor->is_disabled, object.ai.movement_flags,
                    object.ai.runtime_flags, object.ai.movement_event_flags, object.ai.goal_speed_mode,
                    object.ai.movement_target, object.ai.movement_destination.x, object.ai.movement_destination.y,
                    object.ai.movement_destination.z, object.ai.movement_position.x, object.ai.movement_position.y,
                    object.ai.movement_position.z,
                    object.pad_gamepad != nullptr ? object.pad_gamepad->input_magnitude : -1.0f,
                    object.pad_gamepad != nullptr ? object.pad_gamepad->input_angle : 0);

                if (object.id == id_CANTINABAND && processor->script != nullptr) {
                    bool already_logged = false;
                    for (const AISCRIPT *script : logged_ai_scripts) {
                        if (script == processor->script) {
                            already_logged = true;
                            break;
                        }
                    }
                    if (!already_logged) {
                        logged_ai_scripts.push_back(processor->script);
                        host_log_ai_script(processor->script);
                    }
                }
            }
        }
        if (Player[0] != nullptr) {
            const CHARACTERMODEL_s *model = Player[0]->apiobj.character_model;
            i16 render_indices[32] = {};
            i32 render_count = 0;
            i32 clip_state = -1;
            if (model != nullptr && model->hierarchy != nullptr) {
                MAKELAYERLISTFN make_layer_list = GCDataList[model->model_id].make_layer_list;
                if (make_layer_list != nullptr) {
                    render_count =
                        make_layer_list(Player[0]->apiobj.character_model, render_indices, Player[0]->field_0x1054);
                }
                clip_state = NuCameraClipTestExtents(&model->hierarchy->bounds_min, &model->hierarchy->bounds_max,
                                                     &Player[0]->apiobj.field_0xb8, character_farclip, 0);
            }
            LOG_INFO("scripted player render: object=%p id=%d model=%p hierarchy=%p draw=%u "
                     "flags=(state=0x%x,motion=0x%x) transform-mode=%u scale=%.3f matrix-m33=%.3f "
                     "hierarchy=(renders=%d,joints=%d) layer=(count=%d,first=%d) clip=%d far=%.3f",
                     Player[0], Player[0]->id, model, model != nullptr ? model->hierarchy : nullptr,
                     Player[0]->apiobj.model_draw_result, Player[0]->apiobj.field_0x1f8, Player[0]->apiobj.field_0x1f4,
                     Player[0]->field_0x1086, Player[0]->apiobj.field_0xa8, Player[0]->apiobj.field_0xb8.m33,
                     model != nullptr && model->hierarchy != nullptr ? model->hierarchy->render_count : -1,
                     model != nullptr && model->hierarchy != nullptr ? model->hierarchy->joint_count : -1, render_count,
                     render_count > 0 ? render_indices[0] : -1, clip_state, character_farclip);
            if (model != nullptr && model->hierarchy != nullptr && render_count > 0) {
                nuhgobj_s *hierarchy = model->hierarchy;
                nuhgobjrender_s &part = hierarchy->render_parts[render_indices[0]];
                i32 rigid_count = 0;
                i32 alternate_rigid_count = 0;
                void *first_rigid = nullptr;
                void *first_alternate_rigid = nullptr;
                for (i32 joint = 0; part.rigid_specials != nullptr && joint < hierarchy->joint_count; ++joint) {
                    rigid_count += part.rigid_specials[joint] != nullptr;
                    if (first_rigid == nullptr) {
                        first_rigid = part.rigid_specials[joint];
                    }
                }
                for (i32 joint = 0; part.alternate_rigid_specials != nullptr && joint < hierarchy->joint_count;
                     ++joint) {
                    alternate_rigid_count += part.alternate_rigid_specials[joint] != nullptr;
                    if (first_alternate_rigid == nullptr) {
                        first_alternate_rigid = part.alternate_rigid_specials[joint];
                    }
                }
                HostSpecialHandleLayout *handle = static_cast<HostSpecialHandleLayout *>(
                    part.smooth_skin_special != nullptr
                        ? part.smooth_skin_special
                        : (part.alternate_smooth_skin_special != nullptr
                               ? part.alternate_smooth_skin_special
                               : (first_rigid != nullptr ? first_rigid : first_alternate_rigid)));
                HostDisplaySpecialLayout *special =
                    handle != nullptr ? static_cast<HostDisplaySpecialLayout *>(handle->display_special) : nullptr;
                NUDLDLISTSCENE *scene = handle != nullptr && handle->scene != nullptr
                                            ? static_cast<NUDLDLISTSCENE *>(handle->scene->display_list)
                                            : nullptr;
                NUCLIPOBJECT *clip_object = special != nullptr ? special->clip_objects : nullptr;
                LOG_INFO("scripted player special: part=%d name=%s rigid=(array=%p,count=%d) "
                         "smooth=%p alternate=(rigid=%p,count=%d,smooth=%p) handle=(scene=%p,legacy=%p,display=%p) "
                         "display=(name=%s,flags=0x%x,instance=%d,clip=%p,range=%p,bounds=(%.3f,%.3f,%.3f)-"
                         "(%.3f,%.3f,%.3f)) scene=(display=%p,items=%d,clip=%d,mtls=%u,specials=%d,flags=0x%x) "
                         "clip-object=(materials=%d,ids=%p,indices=%p)",
                         render_indices[0], part.name != nullptr ? part.name : "-", part.rigid_specials, rigid_count,
                         part.smooth_skin_special, part.alternate_rigid_specials, alternate_rigid_count,
                         part.alternate_smooth_skin_special, handle != nullptr ? handle->scene : nullptr,
                         handle != nullptr ? handle->special : nullptr,
                         handle != nullptr ? handle->display_special : nullptr,
                         special != nullptr && special->name != nullptr ? special->name : "-",
                         special != nullptr ? special->flags : 0, special != nullptr ? special->instance_ix : -1,
                         special != nullptr ? special->clip_objects : nullptr,
                         special != nullptr ? special->clip_range : nullptr, special != nullptr ? special->min.x : 0.0f,
                         special != nullptr ? special->min.y : 0.0f, special != nullptr ? special->min.z : 0.0f,
                         special != nullptr ? special->max.x : 0.0f, special != nullptr ? special->max.y : 0.0f,
                         special != nullptr ? special->max.z : 0.0f, scene, scene != nullptr ? scene->nitems : -1,
                         scene != nullptr ? scene->nclip_objects : -1, scene != nullptr ? scene->nmtls : 0,
                         scene != nullptr ? scene->nspecials : -1, scene != nullptr ? scene->flags : 0,
                         clip_object != nullptr ? clip_object->nmaterials : -1,
                         clip_object != nullptr ? clip_object->material_ids : nullptr,
                         clip_object != nullptr ? clip_object->indices : nullptr);
            }
        }
    }
    if (options.script_play && !scripted_play_finished) {
        LOG_INFO("scripted play: incomplete started=%d ready_now=%d stage=%d movement_observed=unavailable",
                 scripted_play_started ? 1 : 0, host_scripted_play_ready() ? 1 : 0, static_cast<i32>(scripted_stage));
        if (Player[0] != nullptr) {
            const NUMTX &matrix = Player[0]->apiobj.field_0xb8;
            const f32 basis_squared = matrix.m00 * matrix.m00 + matrix.m01 * matrix.m01 + matrix.m02 * matrix.m02 +
                                      matrix.m10 * matrix.m10 + matrix.m11 * matrix.m11 + matrix.m12 * matrix.m12 +
                                      matrix.m20 * matrix.m20 + matrix.m21 * matrix.m21 + matrix.m22 * matrix.m22;
            LOG_INFO("scripted play: readiness world=(%p,loaded=%d,level=%p,hub=%p,area=%p,hub-area=%p,sock=%p) "
                     "load=(new=%p,mode=%d,level-wait=%d,char-wait=%d,new-wait=%d,abort=%d) "
                     "player=(global=%p,p0=%p,p1=%p,flags=0x%x,pad=%p,pad0=%p) "
                     "matrix=(basis2=%.3f,m33=%.3f,pos=%.3f,%.3f,%.3f,translation=%.3f,%.3f,%.3f) "
                     "camera=(%.3f,%.3f,%.3f)",
                     WORLD, WORLD != nullptr ? WORLD->loaded : 0, WORLD != nullptr ? WORLD->current_level : nullptr,
                     HUB_LDATA, WORLD != nullptr ? WORLD->area : nullptr, HUB_ADATA,
                     WORLD != nullptr ? WORLD->sock_sys : nullptr, NewLData, NewMode, waiting_for_level,
                     waiting_for_character, waiting_for_new_level, abort_load, player, Player[0], Player[1],
                     Player[0]->apiobj.field_0x1f8, Player[0]->pad_gamepad, &GamePad[0], basis_squared, matrix.m33,
                     Player[0]->apiobj.position.x, Player[0]->apiobj.position.y, Player[0]->apiobj.position.z,
                     matrix.m30, matrix.m31, matrix.m32, global_camera.mtx.m30, global_camera.mtx.m31,
                     global_camera.mtx.m32);
        }
    }
    const bool scripted_play_passed =
        !options.script_play ||
        (scripted_play_finished &&
         (options.script_action ? scripted_action_observed && scripted_action_release_observed
                                : scripted_play_movement_observed && scripted_play_jump_observed));
    const bool capture_passed = !options.capture || (have_hash && saw_visible_frame);
    LOG_INFO("presented %d frame_count", frame_count);
    if (numain_finished) {
        const i32 result = host_numain_result.load(std::memory_order_relaxed);
        free(buffer);
        return result != 0 ? result : (scripted_play_passed && capture_passed ? 0 : 1);
    }
    return scripted_play_passed && capture_passed ? 0 : 1;
}
