#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmo/base/GizForceObjectInterface.h"
#include "legoapi/world/world.h"
#include "legoapi/legoapi_types.h"

u32 GizForce_TotalScore(void *world) {
    GIZFORCESYS_s *system = static_cast<WORLDINFO_s *>(world)->giz_force_sys;
    u32 total = 0;
    if (system != NULL) {
        GIZFORCE_s *item = system->forces;
        if (item != NULL) {
            for (i32 i = 0; i < system->count; ++i, ++item)
                total += item->completion_score;
        }
    }
    return total;
}

#include "batman.h"
#include "decomp.h"
#include "globals.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/actions/combat/shovesys.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/playeritems.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/gizmo/object/gizmoblowups.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/misc/supportall.h"
#include "legoapi/misc/utilities.h"
#include "legoapi/render/core/terrain.h"
#include "legoapi/render/fx.h"
#include "legoapi/render/fx/parts.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numusic/sfx.h"
#include "nu2api/nufile/nufpar.h"

#include <string.h>

i32 force_gizmotype_id = -1;

// Older Force data stores sound names in the level configuration instead of the gizmo file.
static i32 GizForceSFX_load_version = 99999;
static GIZFORCE_s *GizForceSFX_force;
static WORLDINFO_s *GizForceSFX_worldinfo;

static void GizForceSFX_forcename(nufpar_s *parser) {
    GizForceSFX_force = NULL;
    if (!NuFParGetWord(parser))
        return;
    char *name = parser->word_buf;
    GIZFORCESYS_s *system = GizForceSFX_worldinfo->giz_force_sys;
    GIZFORCE_s *force = NULL;
    if (system != NULL) {
        force = system->forces;
        for (i32 i = 0; i < system->count; ++i, ++force) {
            if (force->anim_set != NULL) {
                for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
                    char *special_name = NuSpecialGetName(&object->special);
                    if (special_name != NULL && NuStrICmp(name, special_name) == 0) {
                        goto found_force;
                    }
                }
            }
        }
    }
    force = NULL;
found_force:
    GizForceSFX_force = force;
}

static void GizForceSFX_processsfx(nufpar_s *parser) {
    if (GizForceSFX_force != NULL && NuFParGetWord(parser)) {
        GIZFORCE_s *force = GizForceSFX_force;
        if (force->start_sfx_id == -1)
            force->start_sfx_id = GetSfxId(parser->word_buf);
    }
}

static void GizForceSFX_completesfx(nufpar_s *parser) {
    if (GizForceSFX_force != NULL && NuFParGetWord(parser)) {
        GIZFORCE_s *force = GizForceSFX_force;
        if (force->loop_sfx_id == -1)
            force->loop_sfx_id = GetSfxId(parser->word_buf);
    }
}

static void GizForceSFX_returnsfx(nufpar_s *parser) {
    if (GizForceSFX_force != NULL && NuFParGetWord(parser)) {
        GIZFORCE_s *force = GizForceSFX_force;
        if (force->stop_sfx_id == -1)
            force->stop_sfx_id = GetSfxId(parser->word_buf);
    }
}

static NUFPCOMJMP GizForceSFX_ConfigKeywords[] = {
    {"forcename", GizForceSFX_forcename},
    {"processsfx", GizForceSFX_processsfx},
    {"completesfx", GizForceSFX_completesfx},
    {"returnsfx", GizForceSFX_returnsfx},
    {NULL, NULL},
};

void GizForceSFX_Configure(WORLDINFO_s *world, char *config) {
    if (GizForceSFX_load_version > 15 || world == NULL || world->giz_force_sys == NULL ||
        world->giz_force_sys->count == 0)
        return;
    GizForceSFX_worldinfo = world;
    GizForceSFX_force = NULL;
    nufpar_s *parser = NuFParCreateMem("ForceSFX", config, 0xffff);
    if (parser == NULL)
        return;
    NuFParPushCom(parser, GizForceSFX_ConfigKeywords);
    i32 inside = 0;
    while (NuFParGetLine(parser)) {
        while (NuFParGetWord(parser)) {
            if (inside) {
                inside = 0;
                if (NuStrICmp(parser->word_buf, "forcesfx_end") != 0) {
                    inside = 1;
                    NuFParInterpretWord(parser);
                }
            } else {
                inside = NuStrICmp(parser->word_buf, "forcesfx_start") == 0;
            }
        }
    }
    NuFParPopCom(parser);
    NuFParDestroy(parser);
}

void GIZFORCE_s::ClearMechObjectInterface() {
    if (mech_object_interface != NULL)
        delete mech_object_interface;
}

MechObjectInterface *GIZFORCE_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL)
        new GizForceObjectInterface(*this);
    return mech_object_interface;
}

namespace {

    enum : i32 {
        GIZFORCE_PROGRESS_CAPACITY = 128,
        GIZFORCE_PROGRESS_WORDS = GIZFORCE_PROGRESS_CAPACITY / 32,
    };

    struct GIZFORCEPROGRESS_s {
        u32 progress_flag_0[GIZFORCE_PROGRESS_WORDS];
        u32 progress_flag_1[GIZFORCE_PROGRESS_WORDS];
        u32 runtime_flag_1[GIZFORCE_PROGRESS_WORDS];
        u32 runtime_flag_3[GIZFORCE_PROGRESS_WORDS];
        u32 runtime_flag_6[GIZFORCE_PROGRESS_WORDS];
        u32 runtime_flag_7[GIZFORCE_PROGRESS_WORDS];
        u32 field_aa_flag_0[GIZFORCE_PROGRESS_WORDS];
        i8 group_members[8][8];
    };

    DECOMP_ASSERT(sizeof(GIZFORCEPROGRESS_s) == 0xb0, "GIZFORCE progress ABI");

    void ClearForceProgress(GIZFORCEPROGRESS_s *progress) {
        if (progress == NULL) {
            return;
        }
        for (i32 word = 0; word < GIZFORCE_PROGRESS_WORDS; ++word) {
            progress->progress_flag_0[word] = 0xffffffff;
            progress->progress_flag_1[word] = 0xffffffff;
            progress->runtime_flag_1[word] = 0;
            progress->runtime_flag_3[word] = 0;
            progress->runtime_flag_6[word] = 0;
            progress->runtime_flag_7[word] = 0;
            progress->field_aa_flag_0[word] = 0;
        }
        for (i32 group = 0; group < 8; ++group) {
            for (i32 member = 0; member < 8; ++member) {
                progress->group_members[group][member] = -1;
            }
        }
    }

    void edgizforce_ReadAnimSetData(GAMEANIMOBJ_s *object, unsigned char version) {
        if (object == NULL) {
            return;
        }

        GIZFORCEANIMDATA_s fallback = {};
        GIZFORCEANIMDATA_s *object_data = static_cast<GIZFORCEANIMDATA_s *>(object->object_data);
        if (object_data == NULL) {
            object_data = &fallback;
        }
        if (version > 8) {
            object_data->flags = static_cast<u16>(EdFileReadShort());
        }
    }

} // namespace

static i32 GizForces_GetMaxGizmos(void *force) {
    WORLDINFO *world = static_cast<WORLDINFO *>(force);
    if (world == NULL) {
        return 0;
    }
    return world->current_level->max_force;
}

static void GizForces_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *, void *data) {
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    if (force_sys != NULL) {
        if (force_sys->count != 0) {
            for (i32 i = 0; i < force_sys->count; ++i) {
                if (NuStrLen(force_sys->forces[i].name) != 0) {
                    AddGizmo(gizmo_sys, type_id, NULL, &force_sys->forces[i]);
                }
            }
        }
    }
}

static f32 GizForce_GetAnimatedHeight(GIZFORCE_s *force) {
    if (force == NULL || force->anim_set == NULL) {
        return 0.0f;
    }

    f32 minimum_y = 1.0e9f;
    f32 maximum_y = -1.0e9f;
    for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
        NUVEC minimum;
        NUVEC maximum;
        NuSpecialGetBounds(&object->special, &minimum, &maximum);
        if (minimum.y < minimum_y) {
            minimum_y = minimum.y;
        }
        if (maximum.y > maximum_y) {
            maximum_y = maximum.y;
        }
    }
    return maximum_y - minimum_y;
}

static void GizForce_AddToGroup(GIZFORCE_s *force) {
    GIZFORCEGROUP_s *group = force->group;
    if (group == NULL || (force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) != 0 || group->count >= 8) {
        return;
    }
    if (group->count != 0) {
        group->combined_height += GizForce_GetAnimatedHeight(group->forces[group->count - 1]);
    }
    group->forces[group->count++] = force;
    force->progress_flags |= GIZFORCE_PROGRESS_GROUP_MEMBER;
}

static void GizForce_RemoveFromGroup(GIZFORCE_s *force) {
    GIZFORCEGROUP_s *group = force->group;
    if (group == NULL || (force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) == 0) {
        return;
    }
    if (group->count != 0 && group->forces[group->count - 1] == force) {
        group->forces[group->count] = NULL;
        --group->count;
        if (group->count != 0) {
            group->combined_height -= GizForce_GetAnimatedHeight(group->forces[group->count - 1]);
        }
        force->progress_flags &= static_cast<u8>(~GIZFORCE_PROGRESS_GROUP_MEMBER);
        GameAnimSet_SetOffset(force->anim_set, &v000);
    }
}

static void GizForces_Update(void *world_ptr, void *data, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    if (force_sys == NULL || world == NULL || world->gizmo_sys == NULL || world->gizmo_sys->sets == NULL) {
        return;
    }
    for (i32 i = 0; i < 8; ++i) {
        force_sys->groups[i].field_0x24 &= static_cast<u8>(~GIZFORCE_GROUP_ACTIVE);
    }
    force_sys->visible_force_count = 0;
    force_sys->hit_test_gizmo_count = 0;
    GIZMOSET *set = &world->gizmo_sys->sets[force_gizmotype_id];
    GIZMO *gizmo = set->gizmos;
    for (i32 index = 0; index < set->count; ++index, ++gizmo) {
        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
        GameObject_s *user = force->using_object;
        if (user != NULL && force->group != NULL && (force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) == 0 &&
            (force->group->field_0x24 & GIZFORCE_GROUP_ACTIVE) != 0 && user->character_context == 8) {
            user->character_context = -1;
            user->gizforce_target = NULL;
            user = NULL;
        }
        const bool being_used = user != NULL;
        force->field_0xaa = static_cast<u8>((force->field_0xaa & ~GIZFORCE_STATE_BEING_USED) |
                                            (being_used ? GIZFORCE_STATE_BEING_USED : 0));
        force->using_object = NULL;
        force->progress_flags &= static_cast<u8>(~GIZFORCE_PROGRESS_DRAW_ACTIVE);
        const bool was_moving = (force->anim_set->flags & 2) != 0;
        if ((force->runtime_flags & GIZFORCE_RUNTIME_OFFSET_APPLIED) != 0 && force->field_0x50 == 0.0f) {
            NUVEC offset = {0.0f, 0.0f, 0.0f};
            if ((force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) != 0) {
                offset.y = GameAnimSet_GetCompletionRatio(force->anim_set) * force->group->combined_height;
            }
            GameAnimSet_SetOffset(force->anim_set, &offset);
            force->runtime_flags &= static_cast<u8>(~GIZFORCE_RUNTIME_OFFSET_APPLIED);
        }
        if ((force->progress_flags & (GIZFORCE_PROGRESS_VISIBLE | GIZFORCE_PROGRESS_ENABLED)) !=
                (GIZFORCE_PROGRESS_VISIBLE | GIZFORCE_PROGRESS_ENABLED) ||
            force->anim_set == NULL || (force->progress_flags & GIZFORCE_PROGRESS_REVERSE_ACTIVE) != 0 ||
            (force->field_0xaa & GIZFORCE_STATE_DESTROYED_OR_THROWN) != 0) {
            continue;
        }
        if (NuCameraClipTestSphere(&force->position, force->radius, &numtx_identity) == 0) {
            force_sys->visible_forces[force_sys->visible_force_count++] = force;
            force->progress_flags |= GIZFORCE_PROGRESS_DRAW_ACTIVE;
        }

        bool playing_forwards = false;
        bool started_shaking = false;
        f32 target_speed = 1.0f;
        if (user == NULL && force->effect_scale > 1.0f &&
            (force->config_flags & GIZFORCE_CONFIG_WAIT_FOR_FORCE_RANGE) == 0 && force->group == NULL &&
            GameAnimSet_GetCurrentFrame(force->anim_set) > force->effect_scale && !GizForce_AnimComplete(force)) {
            goto seek_and_play;
        }
        if (being_used && (Cheats_CheckFlags(0x400000) != 0 || user->field_0xdec > 0.0f)) {
            target_speed = 3.0f;
        seek_and_play:
            force->animation_speed = SeekValF(force->animation_speed, target_speed, 5.0f);
            playing_forwards = true;
        } else {
            force->animation_speed = SeekValF(force->animation_speed, 1.0f, 5.0f);
            playing_forwards = being_used || (force->runtime_flags & GIZFORCE_RUNTIME_PENDING_COMPLETION) != 0;
        }
        if (playing_forwards) {
            force->runtime_flags &= static_cast<u8>(~GIZFORCE_RUNTIME_COMPLETION_RELEASED);
            GizForce_PlayForwards(force);
            if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) != 0) {
                force->field_0x48 = 0.0f;
                force->field_0x50 = 0.0f;
            } else {
                force->field_0x48 = force->force_strength;
                if (force->force_range > 0.0f && (force->runtime_flags & GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE) == 0 &&
                    force->anim_set->state == GAMEANIMSET_STATE_AT_END) {
                    started_shaking = force->field_0x50 == 0.0f;
                    force->field_0x50 += FRAMETIME;
                    if (force->field_0x50 >= force->force_range) {
                        force->field_0x50 = 0.0f;
                        force->runtime_flags |= GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE;
                    }
                } else {
                    force->field_0x50 = 0.0f;
                }
                if ((force->config_flags & 0x40) != 0 && (force->runtime_flags & 8) == 0 &&
                    GizForce_AnimComplete(force) &&
                    (force->force_range == 0.0f ||
                     (force->runtime_flags & GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE) != 0)) {
                    GameAnimSet_SetVisibility(force->anim_set, 0);
                    force->runtime_flags |= 8;
                }
            }
        } else {
            force->field_0x50 = 0.0f;
            if (!GizForce_AnimComplete(force)) {
                GizForce_PlayBackwards(force);
            } else if ((force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) == 0 &&
                       ((force->config_flags & GIZFORCE_CONFIG_WAIT_FOR_FORCE_RANGE) != 0 ||
                        force->field_0x48 > 0.0f)) {
                force->field_0x48 -= FRAMETIME;
                if (force->field_0x48 <= 0.0f) {
                    force->field_0x48 = 0.0f;
                    if ((force->config_flags & 0x40) != 0 && (force->runtime_flags & 8) != 0) {
                        GameAnimSet_SetVisibility(force->anim_set, 1);
                        force->runtime_flags &= static_cast<u8>(~8);
                    }
                    GizForce_PlayBackwards(force);
                    force->runtime_flags &= static_cast<u8>(~GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE);
                }
            }
        }
        if (force->field_0x50 > 0.0f) {
            NUVEC offset;
            offset.x = qrand() * (1.0f / 65535.0f) * 0.05f - 0.025f;
            offset.y = (force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) != 0
                           ? GameAnimSet_GetCompletionRatio(force->anim_set) * force->group->combined_height
                           : 0.0f;
            offset.y += qrand() * (1.0f / 65535.0f) * 0.05f - 0.025f;
            offset.z = qrand() * (1.0f / 65535.0f) * 0.05f - 0.025f;
            GameAnimSet_SetOffset(force->anim_set, &offset);
            force->runtime_flags |= GIZFORCE_RUNTIME_OFFSET_APPLIED;
            if (force->start_sfx_id == -1) {
                PlaySfx("LegShakeL", &force->position);
            } else if (started_shaking || IsSfxLooping(force->start_sfx_id)) {
                GameAudio_PlaySfxById(force->start_sfx_id, &force->position, 0, 0);
            }
        }

        if ((force->anim_set->flags & 5) != 0 || was_moving) {
            if (force->group != NULL &&
                ((force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) != 0 || force->group->count < 8)) {
                GizForce_AddToGroup(force);
                force->group->field_0x24 |= GIZFORCE_GROUP_ACTIVE;
                NUVEC offset = {0.0f, GameAnimSet_GetCompletionRatio(force->anim_set) * force->group->combined_height,
                                0.0f};
                GameAnimSet_SetOffset(force->anim_set, &offset);
            }
            force->radius = 1.0f;
            force->position = force->file_position;
            GameAnimSet_GetCentreAndRadius(force->anim_set, &force->position, &force->radius, 2, 1, 1);
            if (was_moving) {
                if (GizForce_AnimComplete(force)) {
                    if (force->loop_sfx_id != -1)
                        GameAudio_PlaySfxById(force->loop_sfx_id, &force->position, 0, 0);
                } else if (force->anim_set->state != 4 && force->stop_sfx_id != -1) {
                    GameAudio_PlaySfxById(force->stop_sfx_id, &force->position, 0, 0);
                }
            } else if ((force->anim_set->flags & 5) != 0 && force->field_0x50 == 0.0f && playing_forwards) {
                if (force->start_sfx_id == -1) {
                    PlaySfx("LegoForm", &force->position);
                } else if (force->anim_set->state == GAMEANIMSET_STATE_AT_START || IsSfxLooping(force->start_sfx_id)) {
                    GameAudio_PlaySfxById(force->start_sfx_id, &force->position, 0, 0);
                }
            }
            if ((force->config_flags & 0x2000) == 0) {
                for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
                    GIZFORCEANIMDATA_s *object_data = static_cast<GIZFORCEANIMDATA_s *>(object->object_data);
                    if ((object_data->flags & 2) == 0 && object_data->platform_id != -1) {
                        AddShoveObject(&object->special, object_data->platform_id);
                    }
                }
            }
        } else if (force->anim_set->state == GAMEANIMSET_STATE_AT_START &&
                   (force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) != 0) {
            GizForce_RemoveFromGroup(force);
            force->progress_flags &= static_cast<u8>(~GIZFORCE_PROGRESS_ANIMATION_REVERSED);
        }

        if (force->anim_set->state == GAMEANIMSET_STATE_AT_END) {
            if ((force->progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) != 0) {
                if (GizForce_Complete(force) && user == NULL) {
                    force->progress_flags |= GIZFORCE_PROGRESS_ANIMATION_REVERSED;
                }
            } else if (!(force->force_range > 0.0f &&
                         (force->runtime_flags & GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE) == 0) &&
                       !((force->config_flags & 0x40) != 0 && (force->runtime_flags & 8) == 0) &&
                       (force->anim_set->animated_object_count != 0 || force->force_range > 0.0f ||
                        (force->config_flags & 0x40) != 0) &&
                       (force->runtime_flags & GIZFORCE_RUNTIME_COMPLETION_RELEASED) == 0) {
                if (force->blowup_type != -1) {
                    if ((force->config_flags & 0x20) != 0) {
                        for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
                            NUVEC *position = NuSpecialGetDrawPos(&object->special);
                            if (position != NULL)
                                GizmoBlowUpTypeBlowUp(world, force->blowup_type, position);
                        }
                    } else {
                        GizmoBlowUpTypeBlowUp(world, force->blowup_type, &force->position);
                    }
                    GameAnimSet_SetVisibility(force->anim_set, 0);
                    force->field_0xaa |= GIZFORCE_STATE_DESTROYED_OR_THROWN;
                }
                if (force->pickup_count != 0 && (force->runtime_flags & 2) == 0) {
                    i32 hearts = ReleaseHearts();
                    NUVEC position;
                    NUVEC direction;
                    NuVecAdd(&position, &force->position, &force->effect_position);
                    NuVecRotateX(&direction, &v010, force->pickup_rotation_x);
                    NuVecRotateY(&direction, &direction, force->pickup_rotation_y);
                    AddPickups(force->pickup_count, hearts, 0, 0, &position, &direction, 2.0f, -1,
                               force->activation_radius, 2000000.0f, NULL, 1, 0, true);
                    force->runtime_flags |= 2;
                }
                force->runtime_flags |= GIZFORCE_RUNTIME_COMPLETION_RELEASED;
            }
        }
        GIZFORCEGROUP_s *group = force->group;
        if (group != NULL) {
            if (force->anim_set->state == GAMEANIMSET_STATE_AT_END) {
                if (was_moving && group->count == group->configured_count) {
                    group->field_0x24 |= GIZFORCE_GROUP_STACK_COMPLETE | GIZFORCE_GROUP_STACK_COMPLETE_IN_ORDER;
                    for (i32 i = 0; i < group->configured_count; ++i) {
                        i32 order_mask = static_cast<i8>(group->forces[i]->collision_mask);
                        if (order_mask != 0 && (order_mask & (1 << i)) == 0) {
                            group->field_0x24 &= static_cast<u8>(~GIZFORCE_GROUP_STACK_COMPLETE_IN_ORDER);
                            break;
                        }
                    }
                }
            } else {
                group->field_0x24 &=
                    static_cast<u8>(~(GIZFORCE_GROUP_STACK_COMPLETE | GIZFORCE_GROUP_STACK_COMPLETE_IN_ORDER));
            }
        }
        if ((force->runtime_flags & GIZFORCE_RUNTIME_PENDING_COMPLETION) != 0 && GizForce_Complete(force)) {
            force->runtime_flags &= static_cast<u8>(~GIZFORCE_RUNTIME_PENDING_COMPLETION);
        }
        if ((force->field_0xaa & GIZFORCE_STATE_DESTROYED_OR_THROWN) == 0 &&
            (force->config_flags & GIZFORCE_CONFIG_HIT_TEST_MASK) != 0) {
            force_sys->hit_test_gizmos[force_sys->hit_test_gizmo_count++] = gizmo;
        }
        if ((force->progress_flags & GIZFORCE_PROGRESS_DRAW_ACTIVE) != 0 && !GizForce_Complete(force)) {
            if ((force->config_flags & 0x400) != 0) {
                for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
                    GIZFORCEANIMDATA_s *object_data = static_cast<GIZFORCEANIMDATA_s *>(object->object_data);
                    if (object_data->force_glow_active || !NuSpecialExistsFn(&object->special))
                        continue;
                    NUVEC centre;
                    f32 radius = 0.0f;
                    NuSpecialGetRadius(&object->special, &centre, &radius);
                    radius = 1.2f * force->horizontal_range * radius;
                    NuVecMtxTransformVU0(&centre, &centre, NuSpecialGetDrawMtx(&object->special));
                    NUVEC offset = {0.0f, 0.0f, 0.0f};
                    if (radius != 0.0f)
                        offset.y = qrand() / (65535.0f / (0.5f * radius) + 1.0f);
                    NuVecRotateZ(&offset, &offset, qrand());
                    NuVecRotateY(&offset, &offset, qrand() & 0xffff);
                    NuVecAdd(&centre, &centre, &offset);
                    if ((force->field_0xaa & 2) == 0) {
                        if ((force->config_flags & 0x10) != 0) {
                            AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[110].effect, &centre,
                                                              static_cast<i32>(90.0f * radius), FRAMETIME, 0, 0, NULL);
                        } else {
                            AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[109].effect, &centre,
                                                              static_cast<i32>(90.0f * radius), FRAMETIME, 0, 0, NULL);
                        }
                    }
                }
            } else if ((force->field_0xaa & 2) == 0) {
                NUVEC centre;
                f32 radius = 0.0f;
                GameAnimSet_GetCentreAndRadius(force->anim_set, &centre, &radius, 2, 1, 1);
                radius = 1.2f * force->horizontal_range * radius;
                NUVEC offset = {0.0f, 0.0f, 0.0f};
                if (radius != 0.0f)
                    offset.y = qrand() / (65535.0f / (0.5f * radius) + 1.0f);
                NuVecRotateZ(&offset, &offset, qrand());
                NuVecRotateY(&offset, &offset, qrand() & 0xffff);
                NuVecAdd(&centre, &centre, &offset);
                if ((force->config_flags & 0x10) != 0) {
                    AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[110].effect, &centre,
                                                      static_cast<i32>(90.0f * radius), FRAMETIME, 0, 0, NULL);
                } else {
                    AddVariableShotDebrisEffectTimed1(WORLD->debris_sys->entries[109].effect, &centre,
                                                      static_cast<i32>(90.0f * radius), FRAMETIME, 0, 0, NULL);
                }
            }
        }
        force->field_0xaa &= static_cast<u8>(~2);
        if ((force->config_flags & 0x400) != 0) {
            for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
                static_cast<GIZFORCEANIMDATA_s *>(object->object_data)->force_glow_active = 0;
            }
        }
    }
}

static void GizForces_Draw(void *world_ptr, void *data, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    if (force_sys == NULL) {
        return;
    }

    GIZFORCE_s *force = force_sys->forces;
    for (i32 index = 0; index < force_sys->count; ++index, ++force) {
        if ((force->progress_flags & GIZFORCE_PROGRESS_VISIBLE) == 0 || static_cast<i8>(force->progress_flags) >= 0 ||
            (force->room_id >= 0 && world->rooms_visible_ptr[force->room_id] == 0)) {
            continue;
        }

        ResetShadowMapRendering();
        if ((force->config_flags & GIZFORCE_CONFIG_DRAW_REFLECTION) != 0) {
            GameAnimSet_DrawReflection(force->anim_set, 2, force->vertical_range, NULL);
        }
        if ((force->config_flags & GIZFORCE_CONFIG_CAST_SHADOW) == 0) {
            EnableShadowMapRendering(0);
        }
        if ((force->config_flags & GIZFORCE_CONFIG_ALONG_SOCKET) == 0) {
            continue;
        }
        if ((force->runtime_flags & GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN) == 0) {
            continue;
        }

        NUMTX *draw_matrix = NuSpecialGetDrawMtx(&force->anim_set->objects->special);
        NuSpecialDrawAt(&force->along_socket, draw_matrix);
        if ((force->config_flags & GIZFORCE_CONFIG_DRAW_REFLECTION) != 0) {
            NUMTX reflection_matrix;
            if (MatrixReflection(draw_matrix, 2, draw_matrix->m31 + force->vertical_range,
                                 WORLD->current_level->unknown_0cc, &reflection_matrix) != 0) {
                NuRndrStartReflectionRender(0);
                NuSpecialDrawAt(&force->along_socket, &reflection_matrix);
                NuRndrEndReflectionRender();
            }
        }
    }
    ResetShadowMapRendering();
}

static char *GizmoForce_GetGizmoName(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
        if (force != NULL) {
            return force->name;
        }
    }
    return NULL;
}

static i32 GizmoForce_GetOutput(GIZMO *gizmo, i32 output_index, i32 ignore_activation_state) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
    if (((force->progress_flags & GIZFORCE_PROGRESS_VISIBLE) == 0 && force->blowup_type == -1) ||
        (force->progress_flags & GIZFORCE_PROGRESS_ENABLED) == 0) {
        if (ignore_activation_state == 0) {
            return 0;
        }
    }

    switch (output_index) {
        case 0:
            return force->anim_set != NULL && force->anim_set->state == GAMEANIMSET_STATE_AT_END;
        case 1:
            return force->anim_set != NULL && force->anim_set->state != GAMEANIMSET_STATE_AT_START;
        case 2:
            return force->anim_set != NULL && force->anim_set->state == GAMEANIMSET_STATE_AT_START;
        case 3:
            return force->group != NULL && (force->group->field_0x24 & GIZFORCE_GROUP_STACK_COMPLETE) != 0;
        case 4:
            return force->group != NULL && (force->group->field_0x24 & GIZFORCE_GROUP_STACK_COMPLETE_IN_ORDER) != 0;
        case 5:
            return (force->field_0xaa & GIZFORCE_STATE_DESTROYED_OR_THROWN) != 0;
        case 6:
            return GizForce_Complete(force);
        case 7:
            return (force->field_0xaa & GIZFORCE_STATE_BEING_USED) != 0;
        default:
            return 0;
    }
}

static char *GizmoForce_GetOutputName(GIZMO *, i32 output_index) {
    switch (output_index) {
        case 0:
            return const_cast<char *>("AtEnd");
        case 1:
            return const_cast<char *>("NotAtStart");
        case 2:
            return const_cast<char *>("AtStart");
        case 3:
            return const_cast<char *>("StackComplete");
        case 4:
            return const_cast<char *>("StackCompleteInOrder");
        case 5:
            return const_cast<char *>("Destroyed/Thrown");
        case 6:
            return const_cast<char *>("Complete");
        case 7:
            return const_cast<char *>("BeingUsed");
        default:
            return NULL;
    }
}

static i32 GizmoForce_GetNumOutputs(GIZMO *) {
    return 8;
}

static void GizmoForce_Activate(GIZMO *gizmo, i32 activate) {
    if (gizmo == NULL) {
        return;
    }
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
    if (activate == 0) {
        force->progress_flags &= static_cast<u8>(~GIZFORCE_PROGRESS_ENABLED);
        return;
    }

    u8 progress_flags = force->progress_flags;
    if ((progress_flags & GIZFORCE_PROGRESS_ENABLED) == 0) {
        GameAnimSet_JumpToStart(force->anim_set);
        progress_flags = force->progress_flags;
    }
    force->field_0xaa &= static_cast<u8>(~GIZFORCE_STATE_DESTROYED_OR_THROWN);
    force->progress_flags =
        static_cast<u8>((progress_flags & ~(GIZFORCE_PROGRESS_REVERSE_ACTIVE | GIZFORCE_PROGRESS_ANIMATION_REVERSED)) |
                        GIZFORCE_PROGRESS_ENABLED);
    force->field_0x48 = 0.0f;
    force->field_0x50 = 0.0f;
    u8 runtime_flags = force->runtime_flags;
    if ((force->config_flags & GIZFORCE_CONFIG_RESET_STATE_ON_ACTIVATE) != 0) {
        runtime_flags &= static_cast<u8>(~GIZFORCE_RUNTIME_REWARD_RELEASED);
    }
    force->runtime_flags = static_cast<u8>(runtime_flags & 0x87);
    GameAnimSet_EvaluateState(force->anim_set);
    force->radius = 1.0f;
    force->position = force->file_position;
    GameAnimSet_GetCentreAndRadius(force->anim_set, &force->position, &force->radius, 2, 1, 1);
}

static i32 GizmoForce_ActivateRev(GIZMO *gizmo, i32 activate, i32 flags) {
    if (gizmo == NULL || gizmo->object == NULL) {
        return 0;
    }
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
    if ((flags & 1) != 0) {
        const i32 reverse_active = (force->progress_flags & GIZFORCE_PROGRESS_REVERSE_ACTIVE) != 0 ? 1 : 0;
        return activate != reverse_active;
    }
    if (activate == 0) {
        GizForce_PlayForwards(force);
        force->progress_flags =
            static_cast<u8>((force->progress_flags & ~GIZFORCE_PROGRESS_REVERSE_ACTIVE) | GIZFORCE_PROGRESS_ENABLED);
        return 1;
    }
    GizForce_PlayBackwards(force);
    force->progress_flags =
        static_cast<u8>((force->progress_flags & ~GIZFORCE_PROGRESS_ENABLED) | GIZFORCE_PROGRESS_REVERSE_ACTIVE);
    return 1;
}

static void GizmoForce_SetVisibility(GIZMO *gizmo, i32 visibility) {
    if (gizmo != NULL) {
        GizForce_SetVisibility(static_cast<GIZFORCE_s *>(gizmo->object), visibility);
    }
}

static NUVEC *GizmoForce_GetPos(GIZMO *gizmo) {
    if (gizmo != NULL) {
        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
        if (force != NULL) {
            return &force->position;
        }
    }
    return NULL;
}

static i32 GizForces_BoltHitPlat(void *, void *, BOLT *, unsigned char *) {
    UNIMPLEMENTED();
    return {};
}

static i32 *GizForces_GetBestBoltTarget(GIZMOSET *, float *, NUVEC *, NUVEC *, void *, NUVEC *, NUVEC *, float, float,
                                        i32, i32, i32) {
    UNIMPLEMENTED();
    return {};
}

static i32 GizForces_BoltHit(void *, void *, void *, NUVEC *, i32, float, NUVEC *, NUVEC *, BOLT *, u32,
                             unsigned char *) {
    UNIMPLEMENTED();
    return {};
}

static void *GizForces_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(GIZFORCEPROGRESS_s));
}

static void GizForces_ClearProgress(void *, void *progress_ptr) {
    ClearForceProgress(static_cast<GIZFORCEPROGRESS_s *>(progress_ptr));
}

static void GizForces_StoreProgress(void *, void *data, void *progress_ptr) {
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    GIZFORCEPROGRESS_s *progress = static_cast<GIZFORCEPROGRESS_s *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    ClearForceProgress(progress);
    u32 count = force_sys->count;
    if (count > GIZFORCE_PROGRESS_CAPACITY) {
        count = GIZFORCE_PROGRESS_CAPACITY;
    }
    GIZFORCE_s *force = force_sys->forces;
    for (u32 index = 0; index < count; ++index, ++force) {
        const u32 word = index >> 5;
        const u32 bit = 1u << (index & 31);
        if ((force->progress_flags & GIZFORCE_PROGRESS_VISIBLE) == 0) {
            progress->progress_flag_1[word] &= ~bit;
        }
        if ((force->progress_flags & GIZFORCE_PROGRESS_ENABLED) == 0) {
            progress->progress_flag_0[word] &= ~bit;
        }
        if ((force->runtime_flags & GIZFORCE_RUNTIME_REWARD_RELEASED) != 0) {
            progress->runtime_flag_1[word] |= bit;
        }
        if ((force->runtime_flags & GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN) != 0) {
            progress->runtime_flag_3[word] |= bit;
        }
        if ((force->runtime_flags & GIZFORCE_RUNTIME_COMPLETION_RELEASED) != 0) {
            progress->runtime_flag_6[word] |= bit;
        }
        if ((force->runtime_flags & GIZFORCE_RUNTIME_PENDING_COMPLETION) != 0) {
            progress->runtime_flag_7[word] |= bit;
        }
        if ((force->field_0xaa & GIZFORCE_STATE_DESTROYED_OR_THROWN) != 0) {
            progress->field_aa_flag_0[word] |= bit;
        }
    }

    for (i32 group_index = 0; group_index < 8; ++group_index) {
        GIZFORCEGROUP_s &group = force_sys->groups[group_index];
        for (i32 member = 0; member < 8; ++member) {
            i8 force_index = -1;
            if (member < group.count && group.forces[member] != NULL) {
                force_index = static_cast<i8>(group.forces[member] - force_sys->forces);
            }
            progress->group_members[group_index][member] = force_index;
        }
    }
}

static void GizForces_Reset(void *world_ptr, void *data, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    GIZFORCEPROGRESS_s *progress = static_cast<GIZFORCEPROGRESS_s *>(progress_ptr);
    memset(force_sys->groups, 0, sizeof(force_sys->groups));
    GIZFORCE_s *force = force_sys->forces;
    for (i32 index = 0; index < force_sys->count; ++index, ++force) {
        force->field_0x3c = 0.0f;
        force->progress_flags |= GIZFORCE_PROGRESS_ENABLED | GIZFORCE_PROGRESS_VISIBLE;
        force->progress_flags &= 0x9f;
        force->runtime_flags &= GIZFORCE_RUNTIME_PENDING_BLOWUP_TYPE;
        force->field_0xaa &= 0xe2;
        force->animation_speed = 1.0f;
        force->field_0x48 = 0.0f;
        force->field_0x50 = 0.0f;

        if (force->anim_set != NULL) {
            for (GAMEANIMOBJ_s *object = force->anim_set->objects; object != NULL; object = object->next) {
                GIZFORCEANIMDATA_s *object_data = static_cast<GIZFORCEANIMDATA_s *>(object->object_data);
                if (object_data == NULL) {
                    continue;
                }
                object_data->platform_id = -1;
                if (world->terrain != NULL && (object_data->flags & 1) == 0 &&
                    NuSpecialExistsFn(&object->special) != 0) {
                    object_data->platform_id = FindPlatInst(NuSpecialGetInstanceix(&object->special));
                    if (object_data->platform_id != -1) {
                        force->runtime_flags |= GIZFORCE_RUNTIME_HAS_PLATFORM;
                    }
                }
            }

            force->room_id = world->current_gscn != NULL
                                 ? static_cast<i16>(NuPortalWhichRoom(world->current_gscn, &force->file_position))
                                 : -1;
            GameAnimSet_SetOffset(force->anim_set, &v000);
            force->radius = 1.0f;
            force->position = force->file_position;
            GameAnimSet_GetCentreAndRadius(force->anim_set, &force->position, &force->radius, 2, 1, 1);
            GameAnimSet_EvaluateState(force->anim_set);
        }

        if (progress != NULL && index < GIZFORCE_PROGRESS_CAPACITY) {
            const u32 word = static_cast<u32>(index) >> 5;
            const u32 bit = 1u << (index & 31);
            force->progress_flags =
                static_cast<u8>((force->progress_flags & ~GIZFORCE_PROGRESS_VISIBLE) |
                                ((progress->progress_flag_1[word] & bit) != 0 ? GIZFORCE_PROGRESS_VISIBLE : 0));
            force->progress_flags =
                static_cast<u8>((force->progress_flags & ~GIZFORCE_PROGRESS_ENABLED) |
                                ((progress->progress_flag_0[word] & bit) != 0 ? GIZFORCE_PROGRESS_ENABLED : 0));
            force->runtime_flags =
                static_cast<u8>((force->runtime_flags & ~GIZFORCE_RUNTIME_REWARD_RELEASED) |
                                ((progress->runtime_flag_1[word] & bit) != 0 ? GIZFORCE_RUNTIME_REWARD_RELEASED : 0));
            force->runtime_flags = static_cast<u8>(
                (force->runtime_flags & ~GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN) |
                ((progress->runtime_flag_3[word] & bit) != 0 ? GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN : 0));
            force->runtime_flags = static_cast<u8>(
                (force->runtime_flags & ~GIZFORCE_RUNTIME_COMPLETION_RELEASED) |
                ((progress->runtime_flag_6[word] & bit) != 0 ? GIZFORCE_RUNTIME_COMPLETION_RELEASED : 0));
            force->runtime_flags = static_cast<u8>(
                (force->runtime_flags & ~GIZFORCE_RUNTIME_PENDING_COMPLETION) |
                ((progress->runtime_flag_7[word] & bit) != 0 ? GIZFORCE_RUNTIME_PENDING_COMPLETION : 0));
            force->field_0xaa = static_cast<u8>(
                (force->field_0xaa & ~GIZFORCE_STATE_DESTROYED_OR_THROWN) |
                ((progress->field_aa_flag_0[word] & bit) != 0 ? GIZFORCE_STATE_DESTROYED_OR_THROWN : 0));
        }

        if (force->group_index < 8) {
            force->group = &force_sys->groups[force->group_index];
            ++force->group->configured_count;
        } else {
            force->group = NULL;
        }
    }

    if (progress == NULL) {
        return;
    }
    for (i32 group_index = 0; group_index < 8; ++group_index) {
        for (i32 member = 0; member < 8; ++member) {
            const u8 force_index = static_cast<u8>(progress->group_members[group_index][member]);
            if (force_index >= force_sys->count) {
                continue;
            }

            GIZFORCE_s &force = force_sys->forces[force_index];
            GIZFORCEGROUP_s *group = force.group;
            if ((force.progress_flags & GIZFORCE_PROGRESS_GROUP_MEMBER) == 0) {
                if (group == NULL || group->count >= 8) {
                    continue;
                }
                if (group->count != 0) {
                    GIZFORCE_s *previous = group->forces[group->count - 1];
                    f32 minimum_y = 1.0e9f;
                    f32 maximum_y = -1.0e9f;
                    if (previous != NULL && previous->anim_set != NULL) {
                        for (GAMEANIMOBJ_s *object = previous->anim_set->objects; object != NULL;
                             object = object->next) {
                            NUVEC minimum;
                            NUVEC maximum;
                            NuSpecialGetBounds(&object->special, &minimum, &maximum);
                            if (minimum.y < minimum_y) {
                                minimum_y = minimum.y;
                            }
                            if (maximum.y > maximum_y) {
                                maximum_y = maximum.y;
                            }
                        }
                    }
                    group->combined_height += maximum_y - minimum_y;
                }
                group->forces[group->count++] = &force;
                force.progress_flags |= GIZFORCE_PROGRESS_GROUP_MEMBER;
            }

            NUVEC offset = {0.0f, GameAnimSet_GetCompletionRatio(force.anim_set) * group->combined_height, 0.0f};
            GameAnimSet_SetOffset(force.anim_set, &offset);
            force.radius = 1.0f;
            force.position = force.file_position;
            GameAnimSet_GetCentreAndRadius(force.anim_set, &force.position, &force.radius, 2, 1, 1);
        }
    }
}

static void *GizForces_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZFORCESYS_s *force_sys =
        static_cast<GIZFORCESYS_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, sizeof(GIZFORCESYS_s)));

    force_sys->capacity = world->current_level->max_force;
    force_sys->forces = static_cast<GIZFORCE_s *>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, force_sys->capacity * sizeof(GIZFORCE_s)));
    force_sys->hit_test_gizmos = static_cast<GIZMO **>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, force_sys->capacity * sizeof(GIZMO *)));
    force_sys->visible_forces = static_cast<GIZFORCE_s **>(
        GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, force_sys->capacity * sizeof(GIZFORCE_s *)));
    force_sys->anim_pool =
        GameAnimSet_CreateObjectPool(&world->giz_buffer, &world->unknown_0108, 8, world->current_level->max_force_objs);

    for (i32 i = 0; i < force_sys->capacity; ++i) {
        force_sys->forces[i].anim_set =
            GameAnimSet_Create(&world->giz_buffer, &world->unknown_0108, force_sys->anim_pool, world->game_anim_sys);
    }
    world->giz_force_sys = force_sys;
    return force_sys;
}

static i32 GizForces_Load(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    const u8 version = static_cast<u8>(EdFileReadChar());
    GizForceSFX_load_version = version;
    force_sys->count = static_cast<u16>(EdFileReadShort());

    GIZFORCE_s *force_ptr = force_sys->forces;
    for (i32 index = 0; index < force_sys->count; ++index, ++force_ptr) {
        GIZFORCE_s &force = *force_ptr;
        force.progress_flags = 0;
        force.runtime_flags = 0;
        force.field_0xaa = 0;
        force.collision_mask = 0;
        force.along_socket = {};

        EdFileRead(force.name, sizeof(force.name));
        EdFileReadNuVec(&force.file_position);

        if (version == 1) {
            NUVEC unused_position;
            EdFileReadNuVec(&unused_position);
            force.force_strength = EdFileReadFloat();
            force.interaction_radius = EdFileReadFloat();
            EdFileReadNuVec(&unused_position);
            EdFileReadShort();
            force.config_flags = static_cast<u32>(EdFileReadInt()) | GIZFORCE_CONFIG_LEGACY_ENABLED;
            force.group_index = static_cast<u8>(EdFileReadChar());
            force.activation_mode = static_cast<u8>(EdFileReadChar());
            EdFileReadChar();
        } else {
            force.force_strength = EdFileReadFloat();
            if (version < 8) {
                force.interaction_radius = EdFileReadFloat();
                force.config_flags = static_cast<u32>(EdFileReadInt()) | GIZFORCE_CONFIG_LEGACY_ENABLED;
                force.group_index = static_cast<u8>(EdFileReadChar());
            } else {
                force.force_range = EdFileReadFloat();
                force.interaction_radius = EdFileReadFloat();
                force.config_flags = static_cast<u32>(EdFileReadInt());
                if (version < 13) {
                    force.config_flags |= GIZFORCE_CONFIG_LEGACY_ENABLED;
                }
                force.group_index = static_cast<u8>(EdFileReadChar());
                if (version > 10) {
                    force.collision_mask = static_cast<u8>(EdFileReadChar());
                    if (version == 11 && force.collision_mask != 0) {
                        force.collision_mask = static_cast<u8>(1u << (force.collision_mask - 1));
                    }
                }
            }
            force.activation_mode = static_cast<u8>(EdFileReadChar());
        }

        GizmoFileReadGameAnimSet(force.anim_set, world, edgizforce_ReadAnimSetData, version,
                                 const_cast<char *>("GizForce"), force.name);
        force.start_frame = EdFileReadFloat();
        force.end_frame = EdFileReadFloat();

        force.horizontal_range = 1.0f;
        force.blowup_type = -1;
        if (version > 5) {
            force.effect_scale = EdFileReadFloat();
            if (version == 6) {
                force.vertical_range = EdFileReadFloat();
            } else {
                force.horizontal_range = EdFileReadFloat();
                force.vertical_range = EdFileReadFloat();
            }
        } else if (version > 2) {
            force.vertical_range = EdFileReadFloat();
        }

        if (version == 4) {
            force.blowup_type = EdFileReadShort();
            force.pickup_count = EdFileReadShort();
            force.pickup_rotation_x = EdFileReadShort();
            force.pickup_rotation_y = EdFileReadShort();
            EdFileReadNuVec(&force.effect_position);
        } else if (version > 4) {
            char blowup_name[32] = {};
            const i32 name_length = static_cast<i8>(EdFileReadChar());
            if (name_length != 0) {
                EdFileRead(blowup_name, name_length);
                force.blowup_type = static_cast<i16>(GizmoBlowupGetNameTableId(blowup_name));
                if (force.blowup_type != -1) {
                    force.runtime_flags |= GIZFORCE_RUNTIME_PENDING_BLOWUP_TYPE;
                }
            }
            force.pickup_count = EdFileReadShort();
            force.pickup_rotation_x = EdFileReadShort();
            force.pickup_rotation_y = EdFileReadShort();
            EdFileReadNuVec(&force.effect_position);
        }

        if (version > 9) {
            force.activation_radius = EdFileReadFloat();
        } else {
            force.activation_radius =
                world->area != NULL && (world->area->flags & AREAFLAG_VEHICLE_AREA) != 0 ? 12.0f : 1.75f;
        }

        force.start_sfx_id = -1;
        force.loop_sfx_id = -1;
        force.stop_sfx_id = -1;
        if (version > 13) {
            char sfx_name[64];
            if (GizmoFileReadName(sfx_name) != 0) {
                force.start_sfx_id = static_cast<i16>(GetSfxId(sfx_name));
            }
            if (GizmoFileReadName(sfx_name) != 0) {
                force.loop_sfx_id = static_cast<i16>(GetSfxId(sfx_name));
            }
            if (GizmoFileReadName(sfx_name) != 0) {
                force.stop_sfx_id = static_cast<i16>(GetSfxId(sfx_name));
            }
        }

        if ((force.config_flags & GIZFORCE_CONFIG_ALONG_SOCKET) != 0) {
            for (GAMEANIMOBJ_s *object = force.anim_set->objects; object != NULL; object = object->next) {
                char *special_name = NuSpecialGetName(&object->special);
                if (special_name == NULL) {
                    continue;
                }

                char along_socket_name[64] = {};
                i32 name_pos = 0;
                while (name_pos < 63) {
                    along_socket_name[name_pos] = special_name[name_pos];
                    if (special_name[name_pos] == '_' || special_name[name_pos + 1] == '\0') {
                        break;
                    }
                    ++name_pos;
                }
                NuStrCat(along_socket_name, const_cast<char *>("AlongSock"));
                if (NuSpecialFind(object->special.scene, &force.along_socket, along_socket_name, 1) != 0) {
                    break;
                }
            }
        }
    }
    return 1;
}

static void GizForces_PostLoad(void *world_ptr, void *data) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    if (force_sys == NULL) {
        return;
    }

    for (i32 index = 0; index < force_sys->count; ++index) {
        GIZFORCE_s &force = force_sys->forces[index];
        if ((force.runtime_flags & GIZFORCE_RUNTIME_PENDING_BLOWUP_TYPE) != 0) {
            force.blowup_type = static_cast<i16>(GizmoBlowupGetTypeFromNameTableId(world, force.blowup_type));
            force.runtime_flags &= ~GIZFORCE_RUNTIME_PENDING_BLOWUP_TYPE;
        }
    }
}

static void GizForces_AddLevelSfx(void *, void *data, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx) {
    GIZFORCESYS_s *force_sys = static_cast<GIZFORCESYS_s *>(data);
    if (force_sys == NULL) {
        return;
    }
    GIZFORCE_s *force = force_sys->forces;
    for (i32 index = 0; index < force_sys->count; ++index, ++force) {
        if (force->start_sfx_id != -1) {
            AddLevelSfxFromId(force->start_sfx_id, sfx_ids, sfx_count, max_sfx);
        }
        if (force->loop_sfx_id != -1) {
            AddLevelSfxFromId(force->loop_sfx_id, sfx_ids, sfx_count, max_sfx);
        }
        if (force->stop_sfx_id != -1) {
            AddLevelSfxFromId(force->stop_sfx_id, sfx_ids, sfx_count, max_sfx);
        }
    }
}

ADDGIZMOTYPE *GizForce_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "GizForce";
    addtype.prefix = "";
    addtype.fns.unknown1 = 0xb0;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizForces_GetMaxGizmos;
    addtype.fns.get_pos_fn = GizmoForce_GetPos;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizForces_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = GizForces_BoltHitPlat;
    addtype.fns.get_best_bolt_target_fn = GizForces_GetBestBoltTarget;
    addtype.fns.late_update_fn = GizForces_Update;
    addtype.fns.bolt_hit_fn = GizForces_BoltHit;
    addtype.fns.draw_fn = GizForces_Draw;
    addtype.fns.get_gizmo_name_fn = GizmoForce_GetGizmoName;
    addtype.fns.get_output_fn = GizmoForce_GetOutput;
    addtype.fns.get_output_name_fn = GizmoForce_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizmoForce_GetNumOutputs;
    addtype.fns.activate_fn = GizmoForce_Activate;
    addtype.fns.activate_rev_fn = GizmoForce_ActivateRev;
    addtype.fns.set_visibility_fn = GizmoForce_SetVisibility;
    addtype.fns.allocate_progress_data_fn = GizForces_AllocateProgressData;
    addtype.fns.clear_progress_fn = GizForces_ClearProgress;
    addtype.fns.store_progress_fn = GizForces_StoreProgress;
    addtype.fns.reset_fn = GizForces_Reset;
    addtype.fns.reserve_buffer_space_fn = GizForces_ReserveBufferSpace;
    addtype.fns.load_fn = GizForces_Load;
    addtype.fns.post_load_fn = GizForces_PostLoad;
    addtype.fns.add_level_sfx_fn = GizForces_AddLevelSfx;
    force_gizmotype_id = type_id;

    return &addtype;
}

namespace {
    enum : u8 {
        GIZFORCE_CHARACTER_CONTEXT = 8,
    };
}

PART_s *GizForce_Throw(GameObject_s *object, GIZFORCE_s *force, float speed, float gravity, i32) {
    static NUVEC darth_centre = {19.05f, 4.0f, -105.23f};
    nuhspecial_s *special = &force->anim_set->objects->special;
    NUMTX local_matrix;
    NUMTX *matrix;
    if (WORLD->current_level == MAULF_LDATA) {
        NuMtxSetTranslation(&local_matrix, &darth_centre);
        matrix = &local_matrix;
    } else
        matrix = NuSpecialGetMtx(special);
    if (object == NULL || object->force_throw_target == NULL)
        return NULL;
    NUVEC velocity;
    MakeThrowVector(&velocity, (NUVEC *)&matrix->m30, &object->force_throw_target->apiobj.collision_position, &v000,
                    speed, gravity);
    ADDPART_s params = Default_ADDPART;
    params.matrix = matrix;
    params.velocity = &velocity;
    params.field_90 = object->apiobj.field_0x289;
    params.field_40 = PartCollide_3D;
    params.field_44 = PartKill_ForceThrow;
    params.special = special;
    params.gravity = gravity;
    params.owner = object;
    params.flags = 0xc39b;
    params.time_step = FRAMETIME;
    params.field_18 = 0.15f;
    params.field_14 = 0.15f;
    PART_s *part = AddPart(&params);
    NewRumble(object->pad_gamepad->pad, 0.5f, 0);
    PlaySfx("JForcePush", &object->apiobj.collision_position);
    GameAnimSet_SetVisibility(force->anim_set, 0);
    force->field_0xaa |= 1;
    return part;
}

i32 GizForce_Complete(GIZFORCE_s *force) {
    if ((force->config_flags & GIZFORCE_CONFIG_WAIT_FOR_FORCE_RANGE) != 0 ||
        (force->force_range > 0.0f && (force->runtime_flags & GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE) == 0)) {
        return 0;
    }
    if ((force->config_flags & GIZFORCE_CONFIG_ALONG_SOCKET) != 0 &&
        (force->runtime_flags & GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN) == 0) {
        return 0;
    }
    return GizForce_AnimComplete(force);
}

void GizForce_ResetLOS(GameObject_s *object) {
    if (object->gizforce_los_info != NULL) {
        memset(object->gizforce_los_info, 0, sizeof(*object->gizforce_los_info));
    }
}

GIZFORCE_s *GizForce_FindByName(GIZFORCESYS_s *force_sys, char *name) {
    GIZFORCE_s *force = NULL;
    if (name == NULL || force_sys == NULL) {
        return force;
    }
    force = force_sys->forces;
    for (i32 index = 0; index < force_sys->count; ++index, ++force) {
        if (NuStrICmp(force->name, name) == 0) {
            return force;
        }
    }
    return force;
}

i32 GizForce_UpdateHint(HINT_s *) {
    for (i32 i = 0; i < 2; ++i) {
        GameObject_s *object = Player[i];
        if (object != NULL && (object->apiobj.flags_low & 0x80) != 0 && object->field_0xd80 > 0.0f &&
            object->force_glow_object != NULL && object->force_glow_kind == 0)
            return 1;
    }
    return 0;
}

GIZFORCE_s *GizForces_FindForce(WORLDINFO_s *world, char *name) {
    GIZMO *gizmo = GizmoFindByName(world->gizmo_sys, force_gizmotype_id, name);
    return gizmo != NULL ? static_cast<GIZFORCE_s *>(gizmo->object) : NULL;
}

i32 GizForce_AnimComplete(GIZFORCE_s *force) {
    if (force != NULL && force->anim_set != NULL) {
        if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) == 0) {
            if (force->anim_set->state == GAMEANIMSET_STATE_AT_END) {
                return 1;
            }
        } else if (force->anim_set->state == GAMEANIMSET_STATE_AT_START) {
            return 1;
        }
        return 0;
    }
    return 1;
}

void GizForce_PlayForwards(GIZFORCE_s *force) {
    if (force == NULL) {
        return;
    }
    if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) == 0) {
        GameAnimSet_SetRepeating(force->anim_set, 0);
        f32 speed = force->animation_speed;
        if (speed < 0.0f) {
            GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
            return;
        }
        GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
        return;
    }
    GameAnimSet_SetRepeating(force->anim_set, 0);
    f32 speed = -force->animation_speed;
    if (force->animation_speed >= 0.0f) {
        GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
        return;
    }
    GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
}

i32 GizForce_StoodOnForce(GIZFORCE_s *force, GameObject_s *object) {
    i32 result = 0;
    GAMEANIMOBJ_s *anim_object;
    if ((force->runtime_flags & GIZFORCE_RUNTIME_HAS_PLATFORM) != 0 && object->field_0x1078 != -1 &&
        (anim_object = force->anim_set->objects) != NULL) {
        while (object->field_0x1078 != static_cast<GIZFORCEANIMDATA_s *>(anim_object->object_data)->platform_id) {
            anim_object = anim_object->next;
            if (anim_object == NULL) {
                return 0;
            }
        }
        result = 1;
    }
    return result;
}

void GizForce_PlayBackwards(GIZFORCE_s *force) {
    if (force == NULL) {
        return;
    }
    if ((force->progress_flags & GIZFORCE_PROGRESS_ANIMATION_REVERSED) == 0) {
        GameAnimSet_SetRepeating(force->anim_set, 0);
        f32 speed = -force->animation_speed;
        if (force->animation_speed < 0.0f) {
            GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
            return;
        }
        GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
        return;
    }
    GameAnimSet_SetRepeating(force->anim_set, 0);
    f32 speed = force->animation_speed;
    if (speed >= 0.0f) {
        GameAnimSet_Play(force->anim_set, speed * force->start_frame, 0);
        return;
    }
    GameAnimSet_Play(force->anim_set, speed * force->end_frame, 0);
}

void GizForce_SetVisibility(GIZFORCE_s *force, i32 visibility) {
    if (force != NULL) {
        GameAnimSet_SetVisibility(force->anim_set, visibility);
        force->progress_flags =
            static_cast<u8>((force->progress_flags & ~GIZFORCE_PROGRESS_VISIBLE) | ((visibility != 0) << 1));
    }
}

u16 GizForces_AngleToForce(nuvec_s *position, GIZFORCE_s *force) {
    return NuAtan2D(force->position.x - position->x, force->position.z - position->z);
}

i32 GizForce_GameObjUsingForce(GameObject_s *object, GIZFORCE_s *force) {
    return force != NULL && object != NULL && object->field_0x7a5 == GIZFORCE_CHARACTER_CONTEXT &&
           object->gizforce_target == force;
}

i32 GizForce_FindBestForceTarget(GIZFORCESYS_s *force_sys, GameObject_s *object) {
    struct ForceTarget {
        GIZFORCE_s *force;
        GAMEANIMOBJ_s *animation;
        f32 distance;
        i32 index;
    };
    static ForceTarget possible_forcetargets[384];
    if (object == NULL || force_sys == NULL) {
        return 0;
    }
    i32 super_force = SuperWeirdo(object) || ((object->apiobj.flags_low & 0x80) != 0 && Cheat_IsOn(25));
    object->gizforce_target = NULL;
    object->gizforce_target_object = NULL;
    const f32 facing_x = NU_SIN_LUT(object->apiobj.movement_facing_angle);
    const f32 facing_z = NU_COS_LUT(object->apiobj.movement_facing_angle);
    i32 count = 0;
    for (i32 index = 0; index < force_sys->visible_force_count; ++index) {
        GIZFORCE_s *force = force_sys->visible_forces[index];
        if (force->using_object != NULL || force->field_0x3c_bits != 0) {
            continue;
        }
        if ((force->config_flags & 0x10) != 0 && (object->apiobj.character_data->model_flags & 4) == 0 && !super_force)
            continue;
        if (force->group == NULL) {
            if (GizForce_Complete(force) != 0) {
                continue;
            }
        } else if (force->group->count != 0) {
            GIZFORCE_s *last = force->group->forces[force->group->count - 1];
            if (last != force) {
                if ((force->group->field_0x24 & 1) != 0 || force->anim_set->state == GAMEANIMSET_STATE_AT_END)
                    continue;
                if (last != NULL && ((last->anim_set->flags & 7) != 0 || last->using_object != NULL))
                    continue;
            }
        }
        if (GizForce_StoodOnForce(force, object) != 0) {
            continue;
        }

        NUVEC_ALIGNED16 delta;
        f32 distance = NuVecDistSqr(&force->position, &object->apiobj.collision_position, &delta);
        if (distance > force->interaction_radius * force->interaction_radius)
            continue;
        if ((force->config_flags & GIZFORCE_CONFIG_TARGET_ANIMATION_OBJECTS) != 0) {
            for (GAMEANIMOBJ_s *anim_object = force->anim_set->objects; anim_object != NULL;
                 anim_object = anim_object->next) {
                if ((anim_object->flags & 1) != 0 ||
                    (object->force_glow_previous != NULL && object->force_glow_previous != anim_object)) {
                    continue;
                }
                distance = NuVecDistSqr(NuSpecialGetDrawPos(&anim_object->special), &object->apiobj.collision_position,
                                        &delta);
                if (delta.x * facing_x + delta.z * facing_z < 0.0f)
                    continue;
                possible_forcetargets[count].force = force;
                possible_forcetargets[count].animation = anim_object;
                possible_forcetargets[count].distance = distance;
                possible_forcetargets[count].index = anim_object - force_sys->anim_pool->objects;
                ++count;
            }
            continue;
        }
        if (delta.x * facing_x + delta.z * facing_z < 0.0f ||
            (object->force_glow_previous != NULL && object->force_glow_previous != force))
            continue;
        possible_forcetargets[count].force = force;
        possible_forcetargets[count].animation = NULL;
        possible_forcetargets[count].distance = distance;
        possible_forcetargets[count].index =
            force->anim_set->objects != NULL ? force->anim_set->objects - force_sys->anim_pool->objects : -1;
        ++count;
    }
    if (count == 0)
        return 0;
    GizForceLOSState_s *los = object->gizforce_los_info;
    if (los != NULL) {
        ForceTarget *oldest = NULL;
        f32 oldest_time = 1.0e9f;
        for (i32 i = 0; i < count; ++i) {
            f32 time = static_cast<f32>(los->words[possible_forcetargets[i].index + 12]);
            if (time < oldest_time) {
                oldest_time = time;
                oldest = &possible_forcetargets[i];
            }
        }
        if (oldest != NULL) {
            i32 index = oldest->index;
            los->words[index + 12] = LevelTimer.update_count;
            if ((oldest->force->config_flags & 0x800) == 0) {
                los->words[index >> 5] |= 1u << (index & 31);
            } else {
                los->words[index >> 5] &= ~(1u << (index & 31));
                NUVEC direction;
                i32 clear = 0;
                if (oldest->animation != NULL) {
                    NuVecSub(&direction, NuSpecialGetDrawPos(&oldest->animation->special),
                             &object->apiobj.collision_position);
                    clear = GameRayCast(&object->apiobj.collision_position, &direction, 0.0f, 0) == 0;
                    if (!clear && TerrainPlatId() != -1)
                        clear = static_cast<GIZFORCEANIMDATA_s *>(oldest->animation->object_data)->platform_id ==
                                TerrainPlatId();
                } else if ((oldest->force->field_0xaa & 0x80) == 0) {
                    NuVecSub(&direction, &oldest->force->position, &object->apiobj.collision_position);
                    clear = GameRayCast(&object->apiobj.collision_position, &direction, 0.0f, 0) == 0;
                    if (!clear && (oldest->force->runtime_flags & 1) != 0 && TerrainPlatId() != -1) {
                        for (GAMEANIMOBJ_s *animation = oldest->force->anim_set->objects; animation != NULL;
                             animation = animation->next) {
                            if (static_cast<GIZFORCEANIMDATA_s *>(animation->object_data)->platform_id ==
                                TerrainPlatId()) {
                                clear = 1;
                                break;
                            }
                        }
                    }
                }
                if (clear)
                    object->gizforce_los_info->words[index >> 5] |= 1u << (index & 31);
            }
        }
    }
    ForceTarget *best = NULL;
    f32 best_distance = 1.0e9f;
    los = object->gizforce_los_info;
    for (i32 i = 0; i < count; ++i) {
        ForceTarget *target = &possible_forcetargets[i];
        if (los != NULL && (target->force == NULL || (target->force->field_0xaa & 0x80) == 0) && target->index != -1 &&
            (los->words[target->index >> 5] & (1u << (target->index & 31))) == 0)
            continue;
        f32 distance = target->distance;
        if (object->force_glow_previous != NULL &&
            (object->force_glow_previous == target->force || object->force_glow_previous == target->animation))
            distance = -1.0f;
        if (distance < best_distance) {
            best_distance = distance;
            best = target;
        }
    }
    if (best != NULL) {
        object->gizforce_target = best->force;
        object->gizforce_target_object = best->animation;
    }
    return 0;
}
