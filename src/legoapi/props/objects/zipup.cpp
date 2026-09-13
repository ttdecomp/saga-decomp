#include "legoapi/gizmos/door/zipups.h"

#include "MechInputTouch/MechInputTouch_types.h"
#include "batman.h"
#include "decomp.h"
#include "gameapi/edtools/edfile.h"
#include "gamelib/util/gamelib_util_types.h"
#include "globals.h"
#include "legoapi/audio/sfx.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/misc/utilities.h"
#include "legoapi/render/core/terrain.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "legoapi/world/world_shared.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"

struct ZIPUPPROGRESS {
    u32 active_mask;
    u32 visible_mask;
};

i32 zipup_gizmotype_id = -1;
static char zipup_outputName[] = "Active";
static NUVEC ZipUpHookOffset = {0.0f, -0.1143f, -0.0859f};

static i32 ZipUps_GetMaxGizmos(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL) {
        return 0;
    }
    return world->current_level->max_zipups;
}

static void ZipUps_AddGizmos(GIZMOSYS *gizmo_sys, i32 type_id, void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    for (i32 index = 0; index < world->zipup_count; ++index) {
        ZIPUP *zipup = &world->zipups[index];
        if (NuStrLen(zipup->name) != 0) {
            AddGizmo(gizmo_sys, type_id, NULL, zipup);
        }
    }
}

static void ZipUps_Update(void *world_ptr, void *, float) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world == NULL) {
        return;
    }

    CanDrawZipUpSwirls = 0;
    if (world->zipups == NULL) {
        return;
    }

    CanDrawZipUpSwirls = AvailableToPlayer(1u << 20, -1, 0, 0);
    ZIPUP *zipup = world->zipups;
    for (i32 index = 0; index < world->zipup_count; ++index, ++zipup) {
        GameObject_s *occupant = zipup->occupant;
        if (occupant != NULL && occupant->build_context == -1) {
            occupant->context_flags &= ~(1 << 5);
            occupant->field_0x788 = NULL;
            zipup->runtime_flags &= ~ZIPUP_RUNTIME_FLAG_OCCUPIED;
            zipup->occupant = NULL;
        }
    }
}

static void ZipUps_Draw(void *world_ptr, void *, float) {
    enum ZIPUP_SPECIALS {
        ZIPUP_SPECIAL_HOOK = 83,
        ZIPUP_SPECIAL_GROUND_SWIRL = 85,
        ZIPUP_SPECIAL_GROUND_MARKER = 86,
        ZIPUP_SPECIAL_ENDPOINT_SWIRL = 229,
    };
    enum ZIPUP_DRAW_MATRICES {
        ZIPUP_MATRIX_HOOK,
        ZIPUP_MATRIX_LOWER_MARKER,
        ZIPUP_MATRIX_UPPER_MARKER,
        ZIPUP_MATRIX_LOWER_SWIRL,
        ZIPUP_MATRIX_UPPER_SWIRL,
        ZIPUP_MATRIX_COUNT,
    };

    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world != NULL && world->zipups != NULL) {
        const u16 spin_angle = static_cast<u16>(NuFmod(GameTimer.time_elapsed, 5.0f) / 5.0f * 65536.0f);
        const f32 pulse_phase = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f) * 2.0f * 65536.0f;
        const f32 pulse_alpha = NuTrigTable[(static_cast<i32>(pulse_phase) >> 1) & 0x7fff] * 0.2f + 0.8f;

        NUVEC endpoint_position;
        NUVEC offset;
        NUVEC ground_position;
        NUMTX matrices[ZIPUP_MATRIX_COUNT];

        ZIPUP *zipup = world->zipups;
        for (i32 index = 0; index < world->zipup_count; ++index, ++zipup) {
            if ((zipup->flags & (ZIPUP_FLAG_ACTIVE | ZIPUP_FLAG_VISIBLE)) != (ZIPUP_FLAG_ACTIVE | ZIPUP_FLAG_VISIBLE)) {
                continue;
            }

            if ((zipup->flags & ZIPUP_FLAG_CONFIG_5) != 0) {
                if (world->lev_objs[ZIPUP_SPECIAL_HOOK].active != 0) {
                    offset.x = 0.0f;
                    offset.y = 0.84f;
                    offset.z = 0.9229f;

                    NuVecRotateY(&offset, &offset, zipup->facing_angle);
                    NuVecAdd(&endpoint_position, &zipup->lower_position, &offset);
                    NuMtxSetTranslation(&matrices[ZIPUP_MATRIX_HOOK], &endpoint_position);
                    NuSpecialDrawAt(&world->lev_objs[ZIPUP_SPECIAL_HOOK].special, &matrices[ZIPUP_MATRIX_HOOK]);

                    if ((zipup->flags & ZIPUP_FLAG_CONFIG_3) != 0) {
                        NuVecRotateY(&offset, &offset, 0x8000);
                        NuVecAdd(&endpoint_position, &zipup->upper_position, &offset);
                        NuMtxSetTranslation(&matrices[ZIPUP_MATRIX_HOOK], &endpoint_position);
                        NuSpecialDrawAt(&world->lev_objs[ZIPUP_SPECIAL_HOOK].special, &matrices[ZIPUP_MATRIX_HOOK]);
                    }
                }

                if (world->lev_objs[ZIPUP_SPECIAL_GROUND_MARKER].active != 0) {
                    NuMtxSetRotationY(&matrices[ZIPUP_MATRIX_LOWER_MARKER], spin_angle);
                    NuMtxRotateZ(&matrices[ZIPUP_MATRIX_LOWER_MARKER], zipup->lower_ground_z_rotation);
                    NuMtxRotateX(&matrices[ZIPUP_MATRIX_LOWER_MARKER], zipup->lower_ground_x_rotation);
                    ground_position.x = zipup->lower_position.x;
                    ground_position.y = zipup->lower_ground_height;
                    ground_position.z = zipup->lower_position.z;
                    NuMtxTranslate(&matrices[ZIPUP_MATRIX_LOWER_MARKER], &ground_position);
                    NuSpecialDrawAtAlpha(&world->lev_objs[ZIPUP_SPECIAL_GROUND_MARKER].special,
                                         &matrices[ZIPUP_MATRIX_LOWER_MARKER], pulse_alpha);
                }

                if ((zipup->flags & ZIPUP_FLAG_CONFIG_3) != 0 &&
                    world->lev_objs[ZIPUP_SPECIAL_GROUND_MARKER].active != 0) {
                    NuMtxSetRotationY(&matrices[ZIPUP_MATRIX_UPPER_MARKER], spin_angle);
                    NuMtxRotateZ(&matrices[ZIPUP_MATRIX_UPPER_MARKER], zipup->upper_ground_z_rotation);
                    NuMtxRotateX(&matrices[ZIPUP_MATRIX_UPPER_MARKER], zipup->upper_ground_x_rotation);
                    ground_position.x = zipup->upper_position.x;
                    ground_position.y = zipup->upper_ground_height;
                    ground_position.z = zipup->upper_position.z;
                    NuMtxTranslate(&matrices[ZIPUP_MATRIX_UPPER_MARKER], &ground_position);
                    NuSpecialDrawAtAlpha(&world->lev_objs[ZIPUP_SPECIAL_GROUND_MARKER].special,
                                         &matrices[ZIPUP_MATRIX_UPPER_MARKER], pulse_alpha);
                }
                continue;
            }

            if ((zipup->flags & ZIPUP_FLAG_CONFIG_4) != 0 && world->lev_objs[ZIPUP_SPECIAL_HOOK].active != 0) {
                EnableShadowMapRendering(0);
                NuMtxSetRotationX(&matrices[ZIPUP_MATRIX_HOOK], zipup->hook_x_rotation);
                NuMtxRotateY(&matrices[ZIPUP_MATRIX_HOOK], zipup->hook_y_rotation);
                NuMtxTranslate(&matrices[ZIPUP_MATRIX_HOOK], &zipup->hook_origin);
                NuSpecialDrawAt(&world->lev_objs[ZIPUP_SPECIAL_HOOK].special, &matrices[ZIPUP_MATRIX_HOOK]);
                ResetShadowMapRendering();
            }

            if (CanDrawZipUpSwirls == 0 || (zipup->flags & ZIPUP_FLAG_CONFIG_1) == 0 ||
                world->lev_objs[ZIPUP_SPECIAL_ENDPOINT_SWIRL].active == 0) {
                continue;
            }

            NuMtxSetTranslation(&matrices[ZIPUP_MATRIX_HOOK], &zipup->lower_position);
            NuSpecialDrawAtAlpha(&world->lev_objs[ZIPUP_SPECIAL_ENDPOINT_SWIRL].special, &matrices[ZIPUP_MATRIX_HOOK],
                                 1.0f);

            if ((zipup->flags & ZIPUP_FLAG_CONFIG_3) != 0) {
                NuMtxSetTranslation(&matrices[ZIPUP_MATRIX_HOOK], &zipup->upper_position);
                NuSpecialDrawAtAlpha(&world->lev_objs[ZIPUP_SPECIAL_ENDPOINT_SWIRL].special,
                                     &matrices[ZIPUP_MATRIX_HOOK], 1.0f);
            }

            if ((zipup->flags & ZIPUP_FLAG_CONFIG_2) == 0) {
                continue;
            }

            if (world->lev_objs[ZIPUP_SPECIAL_GROUND_SWIRL].active != 0) {
                NuMtxSetRotationY(&matrices[ZIPUP_MATRIX_LOWER_SWIRL], spin_angle);
                NuMtxRotateZ(&matrices[ZIPUP_MATRIX_LOWER_SWIRL], zipup->lower_ground_z_rotation);
                NuMtxRotateX(&matrices[ZIPUP_MATRIX_LOWER_SWIRL], zipup->lower_ground_x_rotation);
                ground_position.x = zipup->lower_position.x;
                ground_position.y = zipup->lower_ground_height;
                ground_position.z = zipup->lower_position.z;
                NuMtxTranslate(&matrices[ZIPUP_MATRIX_LOWER_SWIRL], &ground_position);
                NuSpecialDrawAtAlpha(&world->lev_objs[ZIPUP_SPECIAL_GROUND_SWIRL].special,
                                     &matrices[ZIPUP_MATRIX_LOWER_SWIRL], pulse_alpha);
            }

            if ((zipup->flags & ZIPUP_FLAG_CONFIG_3) != 0 && world->lev_objs[ZIPUP_SPECIAL_GROUND_SWIRL].active != 0) {
                NuMtxSetRotationY(&matrices[ZIPUP_MATRIX_UPPER_SWIRL], spin_angle);
                NuMtxRotateZ(&matrices[ZIPUP_MATRIX_UPPER_SWIRL], zipup->upper_ground_z_rotation);
                NuMtxRotateX(&matrices[ZIPUP_MATRIX_UPPER_SWIRL], zipup->upper_ground_x_rotation);
                ground_position.x = zipup->upper_position.x;
                ground_position.y = zipup->upper_ground_height;
                ground_position.z = zipup->upper_position.z;
                NuMtxTranslate(&matrices[ZIPUP_MATRIX_UPPER_SWIRL], &ground_position);
                NuSpecialDrawAtAlpha(&world->lev_objs[ZIPUP_SPECIAL_GROUND_SWIRL].special,
                                     &matrices[ZIPUP_MATRIX_UPPER_SWIRL], pulse_alpha);
            }
        }
    }
}

static char *ZipUp_GetGizmoName(GIZMO *gizmo) {
    return gizmo != NULL ? static_cast<ZIPUP *>(gizmo->object)->name : NULL;
}

static i32 ZipUp_GetOutput(GIZMO *gizmo, i32, i32) {
    return gizmo != NULL && (static_cast<ZIPUP *>(gizmo->object)->flags & ZIPUP_FLAG_ACTIVE) != 0;
}

static char *ZipUp_GetOutputName(GIZMO *, i32) {
    return zipup_outputName;
}

static i32 ZipUp_GetNumOutputs(GIZMO *) {
    return 1;
}

static void ZipUp_Activate(GIZMO *gizmo, i32 active) {
    ZIPUP *zipup = static_cast<ZIPUP *>(gizmo->object);
    i32 active_flag = 0;
    if (active != 0) {
        zipup->direction = 0;
        active_flag = 1;
    }
    zipup->active = active_flag;
}

static i32 ZipUp_ActivateRev(GIZMO *gizmo, i32 active, i32 query) {
    if (gizmo == NULL || gizmo->object == NULL) {
        return 0;
    }

    ZIPUP *zipup = static_cast<ZIPUP *>(gizmo->object);
    if ((query & 1) != 0) {
        return zipup->direction != active;
    }
    if (active != 0) {
        zipup->direction = 1;
        zipup->flags &= ~ZIPUP_FLAG_ACTIVE;
    } else {
        zipup->direction = 0;
        zipup->flags |= ZIPUP_FLAG_ACTIVE;
    }
    return 1;
}

static void ZipUp_SetVisibility(GIZMO *gizmo, i32 visible) {
    if (gizmo != NULL) {
        ZIPUP *zipup = static_cast<ZIPUP *>(gizmo->object);
        zipup->visible = visible != 0;
    }
}

static void *ZipUps_AllocateProgressData(VARIPTR *buffer, VARIPTR *buffer_end) {
    return GizmoBufferAlloc(buffer, buffer_end, sizeof(ZIPUPPROGRESS));
}

static void ZipUps_ClearProgress(void *, void *progress_data) {
    ZIPUPPROGRESS *progress = (ZIPUPPROGRESS *)progress_data;
    if (progress == NULL) {
        return;
    }

    progress->active_mask = ~0u;
    progress->visible_mask = ~0u;
}

static void ZipUps_StoreProgress(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    ZIPUPPROGRESS *progress = static_cast<ZIPUPPROGRESS *>(progress_ptr);
    if (progress == NULL) {
        return;
    }

    progress->active_mask = ~0u;
    progress->visible_mask = ~0u;
    if (world == NULL || world->zipups == NULL) {
        return;
    }

    for (i32 index = 0; index < world->zipup_count && index < 32; ++index) {
        const u32 mask = 1u << index;
        if ((world->zipups[index].flags & ZIPUP_FLAG_VISIBLE) == 0) {
            progress->visible_mask &= ~mask;
        }
        if ((world->zipups[index].flags & ZIPUP_FLAG_ACTIVE) == 0) {
            progress->active_mask &= ~mask;
        }
    }
}

void ZipUps_Reset(void *world_ptr, void *, void *progress_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    ZIPUPPROGRESS *progress = static_cast<ZIPUPPROGRESS *>(progress_ptr);
    if (world == NULL) {
        return;
    }

    ZIPUP *zipup = world->zipups;
    if (zipup != NULL) {
        for (i32 index = 0; index < WORLD->zipup_count; ++index, ++zipup) {
            NuVecRotateX(&zipup->hook_position, &ZipUpHookOffset, zipup->hook_x_rotation);
            NuVecRotateY(&zipup->hook_position, &zipup->hook_position, zipup->hook_y_rotation);
            NuVecAdd(&zipup->hook_position, &zipup->hook_origin, &zipup->hook_position);

            NewTerrPlatformsOff();
            f32 ground_height = GameShadow(NULL, &zipup->lower_position, 5.0f, -1);
            if (ground_height != -78.0f) {
                zipup->lower_ground_height = ground_height + 0.005f;
                FindAnglesZX(&ShadNorm, &zipup->lower_ground_x_rotation, &zipup->lower_ground_z_rotation);
            } else {
                zipup->lower_ground_height = 2000000.0f;
            }

            NewTerrPlatformsOff();
            ground_height = GameShadow(NULL, &zipup->upper_position, 5.0f, -1);
            if (ground_height != -78.0f) {
                zipup->upper_ground_height = ground_height + 0.005f;
                FindAnglesZX(&ShadNorm, &zipup->upper_ground_x_rotation, &zipup->upper_ground_z_rotation);
            } else {
                zipup->upper_ground_height = 2000000.0f;
            }

            zipup->facing_angle = static_cast<u16>(NuAtan2D(zipup->upper_position.x - zipup->lower_position.x,
                                                            zipup->upper_position.z - zipup->lower_position.z));
            zipup->runtime_flags &= ~ZIPUP_RUNTIME_FLAG_OCCUPIED;
            zipup->occupant = NULL;
            zipup->flags |= ZIPUP_FLAG_ACTIVE | ZIPUP_FLAG_VISIBLE;

            if (progress != NULL && index <= 31) {
                const u32 bit = 1u << index;
                const u8 visible = (progress->visible_mask & bit) != 0;
                zipup->flags = static_cast<u8>((zipup->flags & ~ZIPUP_FLAG_VISIBLE) | (visible << 7));
                const u8 active = (progress->active_mask & bit) != 0;
                zipup->flags = static_cast<u8>((zipup->flags & ~ZIPUP_FLAG_ACTIVE) | (active << 6));
            }
        }
    }

    if (world->current_level != NULL && world->current_level == CLOUDCITYESCAPEC_LDATA && world->zipup_count > 0) {
        world->zipups[0].lower_ground_height += 0.005f;
    }
}

static void *ZipUps_ReserveBufferSpace(void *world_ptr) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    void *reserved_space = NULL;
    world->zipups = NULL;
    world->zipup_count = 0;
    if (world->current_level->max_zipups != 0) {
        world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
        world->zipups = reinterpret_cast<ZIPUP *>(world->giz_buffer.addr);
        world->giz_buffer.addr += world->current_level->max_zipups * sizeof(ZIPUP);
        reserved_space = world->zipups;
    }
    return reserved_space;
}

static i32 ZipUps_Load(void *world_ptr, void *) {
    WORLDINFO *world = static_cast<WORLDINFO *>(world_ptr);
    if (world->zipup_count != 0) {
        return 0;
    }

    const i32 version = EdFileReadInt();
    world->zipup_count = EdFileReadInt();
    for (i32 index = 0; index < world->zipup_count; ++index) {
        EdFileRead(world->zipups[index].name, sizeof(world->zipups[index].name));
        EdFileReadNuVec(&world->zipups[index].lower_position);
        EdFileReadNuVec(&world->zipups[index].hook_origin);
        EdFileReadNuVec(&world->zipups[index].upper_position);
        world->zipups[index].hook_x_rotation = EdFileReadUnsignedShort();
        world->zipups[index].hook_y_rotation = EdFileReadUnsignedShort();

        const u8 config_0 = EdFileReadUnsignedChar() != 0;
        world->zipups[index].flags = static_cast<u8>((world->zipups[index].flags & ~ZIPUP_FLAG_CONFIG_0) | config_0);
        const u8 config_1 = EdFileReadUnsignedChar() != 0;
        world->zipups[index].flags =
            static_cast<u8>((world->zipups[index].flags & ~ZIPUP_FLAG_CONFIG_1) | (config_1 << 1));
        const u8 config_3 = EdFileReadUnsignedChar() != 0;
        world->zipups[index].flags =
            static_cast<u8>((world->zipups[index].flags & ~ZIPUP_FLAG_CONFIG_3) | (config_3 << 3));

        if (version <= 1) {
            world->zipups[index].flags |= ZIPUP_FLAG_CONFIG_4;
            world->zipups[index].flags &= ~ZIPUP_FLAG_CONFIG_5;
            world->zipups[index].flags |= ZIPUP_FLAG_CONFIG_2;
        } else {
            const u8 config_4 = EdFileReadUnsignedChar() != 0;
            world->zipups[index].flags =
                static_cast<u8>((world->zipups[index].flags & ~ZIPUP_FLAG_CONFIG_4) | (config_4 << 4));
            if (version == 2) {
                world->zipups[index].flags &= ~ZIPUP_FLAG_CONFIG_5;
                world->zipups[index].flags |= ZIPUP_FLAG_CONFIG_2;
            } else {
                const u8 config_5 = EdFileReadUnsignedChar() != 0;
                world->zipups[index].flags =
                    static_cast<u8>((world->zipups[index].flags & ~ZIPUP_FLAG_CONFIG_5) | (config_5 << 5));
                if (version == 3) {
                    world->zipups[index].flags |= ZIPUP_FLAG_CONFIG_2;
                } else {
                    const u8 config_2 = EdFileReadUnsignedChar() != 0;
                    world->zipups[index].flags =
                        static_cast<u8>((world->zipups[index].flags & ~ZIPUP_FLAG_CONFIG_2) | (config_2 << 2));
                }
            }
        }
    }
    return 1;
}

ADDGIZMOTYPE *ZipUps_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "ZipUp";
    addtype.prefix = "";
    addtype.fns.unknown1 = 8;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = ZipUps_GetMaxGizmos;
    addtype.fns.get_pos_fn = NULL;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = ZipUps_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = ZipUps_Update;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = ZipUps_Draw;
    addtype.fns.get_gizmo_name_fn = ZipUp_GetGizmoName;
    addtype.fns.get_output_fn = ZipUp_GetOutput;
    addtype.fns.get_output_name_fn = ZipUp_GetOutputName;
    addtype.fns.get_num_outputs_fn = ZipUp_GetNumOutputs;
    addtype.fns.activate_fn = ZipUp_Activate;
    addtype.fns.activate_rev_fn = ZipUp_ActivateRev;
    addtype.fns.set_visibility_fn = ZipUp_SetVisibility;
    addtype.fns.allocate_progress_data_fn = ZipUps_AllocateProgressData;
    addtype.fns.clear_progress_fn = ZipUps_ClearProgress;
    addtype.fns.store_progress_fn = ZipUps_StoreProgress;
    addtype.fns.reset_fn = ZipUps_Reset;
    addtype.fns.reserve_buffer_space_fn = ZipUps_ReserveBufferSpace;
    addtype.fns.load_fn = ZipUps_Load;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;
    zipup_gizmotype_id = type_id;

    return &addtype;
}

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

i16 LEGOACT_WHIP_SWING_START = -1;
i16 LEGOACT_WHIP_SWING_SWING = -1;
i16 LEGOACT_WHIP_SWING_JUMP = -1;
i32 ObjLandReady(GameObject_s *);
i32 SuperWeirdo(GameObject_s *);
void SetHeadTarget(GameObject_s *, NUVEC *, i8, f32, f32, f32);
void StartJump(GameObject_s *, i32);
i32 StartFallLand(GameObject_s *, i32);
void SetWeaponIn(GameObject_s *);
void FastWeaponOut(GameObject_s *, i32);
void Hint_SetComplete(i32);
i32 GameAudio_GetPlrSfxBits(void *);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
void PlayJumpSfx(GameObject_s *, i32);
i32 RotDiff(u16, u16);
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);

void ZipUp_MoveCode(GameObject_s *object, i32 special_pressed) {
    APIOBJECT &api = object->apiobj;
    if (object->character_context == 0x47) {
        if (object->action_movement_state == 1) {
            void *info = api.character_model->model_data_b[object->context_animation];
            f32 *playing = NULL;
            if (info != NULL)
                playing = AnimPlaying(&api.anim_packet, object->context_animation, 1, 0);
            if (info == NULL || playing != NULL) {
                object->context_animation_timer += FRAMETIME;
                if (object->context_animation_timer >= object->airborne_action_duration)
                    object->context_animation_timer = object->airborne_action_duration;
            }
            api.velocity = v000;
            NUVEC delta;
            if (object->field_0x7a3 == 0) {
                f32 fraction;
                bool animation_fraction = false;
                if (playing != NULL && *playing > 0.0f) {
                    f32 frame = AnimListFrame(api.character_model, object->context_animation, 0);
                    if (frame > 1.0f && AnimEndFrame(api.character_model, object->context_animation) > frame) {
                        f32 progress = (*playing - 1.0f) / (frame - 1.0f);
                        fraction = progress < 1.0f ? progress : 1.0f;
                        animation_fraction = true;
                    }
                }
                if (!animation_fraction)
                    fraction = object->context_animation_timer / object->airborne_action_duration;
                NuVecSub(&delta, &object->zipup_start_position, &object->zipup_entry_position);
                NuVecScale(&delta, &delta, fraction);
                NuVecAdd(&api.position, &object->zipup_entry_position, &delta);
                if (object->context_animation_timer >= object->airborne_action_duration) {
                    object->field_0x7a3 = 1;
                    object->context_animation_timer = 0.0f;
                    object->context_animation = LEGOACT_WHIP_SWING_SWING;
                    f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 0);
                    object->airborne_action_duration = duration <= 0.0f ? 1.0f : duration;
                    GameAudio_PlaySfx(0x4d, &api.collision_position, GameAudio_GetPlrSfxBits(object), 0);
                }
            } else if (object->field_0x7a3 == 1) {
                f32 fraction = object->context_animation_timer / object->airborne_action_duration;
                NuVecSub(&delta, &object->zipup_swing_position, &object->zipup_start_position);
                NuVecScale(&delta, &delta, fraction);
                NuVecAdd(&api.position, &object->zipup_start_position, &delta);
                if (object->context_animation_timer >= object->airborne_action_duration) {
                    object->field_0x7a3 = 2;
                    object->context_animation_timer = 0.0f;
                    object->context_animation = LEGOACT_WHIP_SWING_JUMP;
                    f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 0);
                    object->airborne_action_duration = duration <= 0.0f ? 1.0f : duration;
                    PlayJumpSfx(object, 0);
                }
            } else if (object->field_0x7a3 == 2) {
                f32 fraction = object->context_animation_timer / object->airborne_action_duration;
                NuVecSub(&delta, &object->zipup_landing_position, &object->zipup_swing_position);
                NuVecScale(&delta, &delta, fraction);
                NuVecAdd(&api.position, &object->zipup_swing_position, &delta);
                if (object->context_animation_timer >= object->airborne_action_duration) {
                    api.velocity.x = api.velocity.z = 0.0f;
                    api.velocity.y = -2.0f;
                    object->character_context = -1;
                    StartFallLand(object, LEGOACT_LAND);
                }
            }
            return;
        }
        ZIPUP *zipup = static_cast<ZIPUP *>(object->field_0x788);
        NUVEC *hook = &zipup->hook_origin;
        NUVEC *destination = object->field_0x7a3 == 0 ? &zipup->upper_position : &zipup->lower_position;
        if ((zipup->flags & 1) == 0) {
            if (NuVecDistSqr(&api.position, hook, NULL) < 0.01f || object->context_animation_timer >= 5.0f) {
                StartJump(object, 6);
                PlaySfx("GrapDetach", &api.collision_position);
                if ((api.flags_low & 0x80) != 0)
                    Hint_SetComplete(0x262);
                u16 angle = NuAtan2D(destination->x - api.position.x, destination->z - api.position.z);
                api.field_0x276 = api.facing_angle = api.movement_facing_angle = angle;
                f32 speed =
                    NuVecXZDist(&api.position, destination, NULL) / api.character_data->game_character->jump_duration;
                object->airborne_action_timer = speed;
                api.velocity.x = NU_SIN_LUT(angle) * speed;
                api.velocity.z = NU_COS_LUT(angle) * speed;
                object->context_flags &= ~0x20;
                static_cast<ZIPUP *>(object->field_0x788)->runtime_flags &= ~1;
                object->field_0x788 = NULL;
                return;
            }
            zipup = static_cast<ZIPUP *>(object->field_0x788);
        }
        if ((zipup->flags & 0xc0) != 0xc0 || (zipup->runtime_flags & 1) == 0 || (object->context_flags & 0x20) == 0) {
            object->character_context = -1;
            object->context_flags &= ~0x20;
            zipup->runtime_flags &= ~1;
            object->field_0x788 = NULL;
            return;
        }
        if ((zipup->flags & 1) == 0) {
            object->context_animation_timer += FRAMETIME;
            PlaySfx("GrapWindLp", &api.collision_position);
            return;
        }
        if (object->context_animation_timer >= 1.5f) {
            object->context_animation_timer = 0.0f;
            object->character_context = -1;
            zipup->occupant = NULL;
            object->context_flags &= ~0x20;
            static_cast<ZIPUP *>(object->field_0x788)->runtime_flags &= ~1;
            object->fall_animation_timer = 0.2f;
            api.velocity.x = object->target_velocity.x = v000.x;
            api.velocity.y = object->target_velocity.y = -1.0f;
            api.velocity.z = object->target_velocity.z = v000.z;
            object->field_0x788 = NULL;
            object->magnet_surface_angle = 0;
            PlaySfx("GrapDetach", &api.collision_position);
            if ((api.flags_low & 0x80) != 0)
                Hint_SetComplete(0x262);
            return;
        }
        zipup->runtime_flags |= 2;
        u16 start_angle = NuAtan2D(zipup->rider_start_offset.x, zipup->rider_start_offset.z);
        NUVEC end_offset;
        NuVecSub(&end_offset, destination, hook);
        end_offset.y += 0.5f;
        u16 end_angle = NuAtan2D(end_offset.x, end_offset.z);
        i32 difference = 0x8000 - static_cast<u16>(RotDiff(start_angle, end_angle));
        if (difference < 0)
            difference = -difference;
        i32 half = static_cast<i32>(difference * 0.5f);
        u16 axis = start_angle > 0x8000 ? start_angle + half : start_angle - half;
        NUVEC offset = zipup->rider_start_offset;
        NuVecRotateY(&offset, &offset, static_cast<u16>(-axis));
        f32 phase = object->context_animation_timer / 1.5f * 32768.0f;
        i32 pitch = static_cast<i32>((1.0f - (NU_SIN_LUT(static_cast<i32>(16384.0f + phase)) + 1.0f) * 0.5f) *
                                     zipup->pitch_adjustment);
        NuVecRotateX(&zipup->rider_target_position, &offset, pitch);
        NuVecRotateY(&zipup->rider_target_position, &zipup->rider_target_position, axis);
        zipup->rider_target_position.y *= 1.0f - (1.0f - NuTrigTable[0x1000]) * NU_SIN_LUT(static_cast<i32>(phase));
        NuVecAdd(&zipup->rider_target_position, hook, &zipup->rider_target_position);
        f32 old_time = object->context_animation_timer;
        object->context_animation_timer += FRAMETIME;
        if (old_time < 0.55f && object->context_animation_timer >= 0.55f)
            PlaySfx("GrapSwing", &api.collision_position);
        return;
    }
    if (!ObjLandReady(object) && !objInNetWaitContext(object, 0x47))
        return;
    if ((api.flags_low & 0x80) != 0) {
        if ((api.character_data->model_flags & 0x100000) == 0 && !SuperWeirdo(object)) {
            if ((api.character_data->model_flags & 8) == 0 ||
                (api.character_data->game_character->flags_094[1] & 0x80) != 0 || !Cheat_IsOn(13))
                return;
        }
    } else if (object->use_action != 4 || (api.character_data->model_flags & 0x100000) == 0) {
        return;
    }
    i32 endpoint;
    ZIPUP *zipup = ZipUp_FindNearest(WORLD, &api.lower_position, api.collision_radius, NULL, &endpoint, object, false);
    if (objInNetWaitContext(object, 0x47)) {
        object->context_animation_timer -= FRAMETIME;
        if (object->context_animation_timer <= 0.0f)
            object->character_context = -1;
    }
    if (zipup == NULL)
        return;
    if ((special_pressed == 0 && !objInNetWaitContext(object, 0x47)) ||
        ((zipup->flags & 1) != 0 && (zipup->runtime_flags & 1) != 0)) {
        SetHeadTarget(object, &zipup->hook_origin, 4, 2.0f, 1.0f, 2.0f);
        return;
    }
    if ((api.character_data->model_flags & 0x100000) == 0) {
        NUVEC *destination = endpoint == 0 ? &zipup->upper_position : &zipup->lower_position;
        if (StartBigJump(object, destination, 0, 0.5f, 1.0f, 0, 0))
            object->context_variant_flags |= 1;
        return;
    }
    if ((api.flags_low & 0x80) != 0)
        Hint_SetComplete(0x262);
    object->field_0x788 = zipup;
    object->character_context = 0x47;
    zipup->occupant = object;
    object->context_flags |= 0x20;
    static_cast<ZIPUP *>(object->field_0x788)->runtime_flags |= 1;
    if ((api.character_data->game_character->flags_094[3] & 0x20) != 0) {
        object->action_movement_state = 1;
        SetWeaponIn(object);
        GameAudio_PlaySfx(0x4c, &api.collision_position, GameAudio_GetPlrSfxBits(object), 0);
        object->field_0x7a3 = 0;
        object->context_animation_timer = 0.0f;
        object->context_animation = LEGOACT_WHIP_SWING_START;
        f32 duration = AnimDuration(object->id, object->context_animation, 0.0f, 0.0f, 0);
        object->airborne_action_duration = duration <= 0.0f ? 0.5f : duration;
        object->zipup_entry_position = api.position;
        ZIPUP *current = static_cast<ZIPUP *>(object->field_0x788);
        object->zipup_start_position = endpoint == 0 ? current->lower_position : current->upper_position;
        object->zipup_landing_position = endpoint == 0 ? current->upper_position : current->lower_position;
        object->zipup_landing_position.y = endpoint == 0 ? zipup->upper_ground_height : zipup->lower_ground_height;
        NUVEC offset;
        NuVecSub(&offset, &object->zipup_landing_position, &object->zipup_start_position);
        offset.y = 0.0f;
        NuVecNorm(&offset, &offset);
        NuVecScale(&offset, &offset, 1.923615932464599609375f);
        NuVecAdd(&object->zipup_swing_position, &object->zipup_start_position, &offset);
        object->zipup_swing_position.y += 0.25150299072265625f;
        api.movement_facing_angle = NuAtan2D(object->zipup_landing_position.x - object->zipup_start_position.x,
                                             object->zipup_landing_position.z - object->zipup_start_position.z);
        api.field_0x27d = 0;
        return;
    }
    zipup = static_cast<ZIPUP *>(object->field_0x788);
    object->action_movement_state = 0;
    object->context_animation = 0x2a;
    NUVEC *hook = &zipup->hook_origin;
    NUVEC *start = endpoint == 0 ? &zipup->lower_position : &zipup->upper_position;
    NUVEC *destination = endpoint == 0 ? &zipup->upper_position : &zipup->lower_position;
    object->field_0x7a3 = endpoint == 0 ? 0 : 1;
    NUVEC *heading = start->x == hook->x && start->z == hook->z ? destination : hook;
    api.movement_facing_angle = NuAtan2D(heading->x - start->x, heading->z - start->z);
    api.velocity.y = 0.0f;
    if (api.character_data->game_character->field275_0x116 == 2)
        FastWeaponOut(object, 1);
    object->context_animation_timer = 0.0f;
    PlaySfx("GrapAttach", &api.upper_position);
    FastWeaponOut(object, 0);
    object->field_0xe31 = 0;
    f32 dx = hook->x - start->x, dz = hook->z - start->z;
    i32 angle = NuAtan2D(hook->y - start->y, NuFsqrt(dx * dx + dz * dz));
    i32 pitch = 0x4000 - (angle < 0 ? -angle : angle);
    object->magnet_surface_angle = angle < 0 ? -pitch : pitch;
    zipup = static_cast<ZIPUP *>(object->field_0x788);
    if ((zipup->flags & 1) != 0) {
        NuVecSub(&zipup->rider_start_offset, start, hook);
        NUVEC end_offset;
        NuVecSub(&end_offset, destination, hook);
        end_offset.y += 0.5f;
        f32 rider_height = 0.5f * api.scaled_height;
        zipup->rider_start_offset.x *= 0.9f;
        zipup->rider_start_offset.y *= 0.9f;
        zipup->rider_start_offset.y = rider_height + zipup->rider_start_offset.y;
        zipup->rider_start_offset.z *= 0.9f;
        i32 yaw = -static_cast<u16>(NuAtan2D(zipup->rider_start_offset.x, zipup->rider_start_offset.z));
        NUVEC start_offset = zipup->rider_start_offset;
        NuVecRotateY(&start_offset, &start_offset, yaw);
        NuVecRotateY(&end_offset, &end_offset, yaw);
        NuVecNorm(&start_offset, &start_offset);
        NuVecNorm(&end_offset, &end_offset);
        static_cast<ZIPUP *>(object->field_0x788)->pitch_adjustment =
            NuACos(start_offset.y * end_offset.y + start_offset.z * end_offset.z);
        zipup = static_cast<ZIPUP *>(object->field_0x788);
        zipup->rider_target_position = zipup->rider_start_offset;
        NuVecAdd(&zipup->rider_target_position, hook, &zipup->rider_target_position);
    }
}


static void ZipUp_GetStartPoint(GameObject_s *object, NUVEC *position) {
    GAMECHARACTERDATA *character = object->apiobj.character_data->game_character;
    i32 joint = character->grapple_locators[0];
    if (joint != -1 && object->apiobj.character_model->points_of_interest[joint] != NULL &&
        object->apiobj.field_0x288 != 0) {
        *position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3);
        return;
    }
    joint = character->weapon_shoot_joints[0];
    if (joint != -1 && object->apiobj.character_model->points_of_interest[joint] != NULL &&
        object->apiobj.field_0x288 != 0) {
        *position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3);
        return;
    }
    joint = character->weapon_joints[0];
    if (joint != -1 && object->apiobj.character_model->points_of_interest[joint] != NULL &&
        object->apiobj.field_0x288 != 0) {
        *position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3);
        return;
    }
    position->x = object->apiobj.collision_position.x;
    position->y = object->apiobj.collision_position.y + object->apiobj.collision_height;
    position->z = object->apiobj.collision_position.z;
}

numtl_s *ropemtl;

void InitRopeMtl(char *name, variptr_u *buffer, variptr_u *buffer_end) {
    char path[64] = "stuff\\";
    if (name == NULL) {
        ropemtl = NULL;
        return;
    }
    ropemtl = NuMtlCreate3D(1);
    ropemtl->opacity = 1.0f;
    ropemtl->attribs.unknown_2_1_2 = 0;
    ropemtl->attribs.unknown_1_1_2 = 0;
    ropemtl->attribs.unknown_1_4_8 = 0;
    ropemtl->attribs.z_mode = 0;
    ropemtl->attribs.alpha_mode = 0;
    ropemtl->attribs.filter_mode = 1;
    NuStrCat(path, name);
    buffer->addr = ALIGN(buffer->addr, 16);
    ropemtl->tex_id = NuTexRead(path, buffer, reinterpret_cast<VARIPTR *>(buffer_end->addr));
    if (ropemtl->tex_id == 0) {
        NuMtlDestroy(ropemtl);
        ropemtl = NULL;
    } else {
        NuMtlUpdate(ropemtl);
    }
}

void DrawRopeSingle(nuvec_s *start, nuvec_s *end, float amount, numtl_s *material, float time, float grow_time,
                    float spacing, float scale) {
    static f32 ROPELEN;
    static u32 ropedif = 0xff505050;
    ROPELEN = 0.04f;
    ropedif = Cheat_IsOn(3) ? 0xff103f10 : 0xff505050;
    if (material == NULL)
        material = ropemtl;
    if (end == NULL || start == NULL)
        return;
    amount = NuFmax(0.0f, NuFmin(1.0f, amount));
    NUVEC direction = {end->x - start->x, end->y - start->y, end->z - start->z};
    f32 length = NuVecMag(&direction) * amount;
    f32 repeats = length / ROPELEN;
    NURND_VERTEX3D vertices[10] = {
        {{-0.01f, 0.0f, 0.01f}, {-0.7071068286895752f, 0.0f, 0.7071068286895752f}, ropedif, 0.0f, 0.0f},
        {{-0.01f, length, 0.01f}, {-0.7071068286895752f, 0.0f, 0.7071068286895752f}, ropedif, repeats, 0.0f},
        {{0.01f, 0.0f, 0.01f}, {0.7071068286895752f, 0.0f, 0.7071068286895752f}, ropedif, 0.0f, 1.0f},
        {{0.01f, length, 0.01f}, {0.7071068286895752f, 0.0f, 0.7071068286895752f}, ropedif, repeats, 1.0f},
        {{0.01f, 0.0f, -0.01f}, {0.7071068286895752f, 0.0f, -0.7071068286895752f}, ropedif, 0.0f, 2.0f},
        {{0.01f, length, -0.01f}, {0.7071068286895752f, 0.0f, -0.7071068286895752f}, ropedif, repeats, 2.0f},
        {{-0.01f, 0.0f, -0.01f}, {-0.7071068286895752f, 0.0f, -0.7071068286895752f}, ropedif, 0.0f, 3.0f},
        {{-0.01f, length, -0.01f}, {-0.7071068286895752f, 0.0f, -0.7071068286895752f}, ropedif, repeats, 3.0f},
        {{-0.01f, 0.0f, 0.01f}, {-0.7071068286895752f, 0.0f, 0.7071068286895752f}, ropedif, 0.0f, 4.0f},
        {{-0.01f, length, 0.01f}, {-0.7071068286895752f, 0.0f, 0.7071068286895752f}, ropedif, repeats, 4.0f}};
    u16 x_rotation, z_rotation;
    FindAnglesZX(&direction, &x_rotation, &z_rotation);
    NUMTX matrix;
    NuMtxSetRotationZ(&matrix, z_rotation);
    NuMtxRotateX(&matrix, x_rotation);
    NuMtxTranslate(&matrix, start);
    NuRndrTriStrip3dClip(vertices, 10, &matrix, material);
    if (Cheat_IsOn(3) == 0 || VehicleArea != 0)
        return;
    NUVEC position = v000;
    NUVEC size = v000;
    position.y = 0.0f;
    f32 step = spacing * ROPELEN;
    u16 rotation = 0;
    while (position.y < length) {
        f32 magnitude = scale;
        if (grow_time >= time) {
            i32 angle = (i32)((1.0f / grow_time * time) * 16384.0f + 32768.0f + 16384.0f);
            magnitude = (1.0f + NuTrigTable[(angle >> 1) & 0x7fff]) * scale;
        }
        size.x = size.y = size.z = magnitude;
        rotation = (u16)(rotation + 0x5555);
        NuMtxSetScale(&matrix, &size);
        NuMtxTranslate(&matrix, &position);
        NuMtxRotateY(&matrix, rotation);
        NuMtxRotateZ(&matrix, z_rotation);
        NuMtxRotateX(&matrix, x_rotation);
        NuMtxTranslate(&matrix, start);
        NuSpecialDrawAt(&WORLD->lev_objs[0x124].special, &matrix);
        NuSpecialDrawAt(&WORLD->lev_objs[0x125].special, &matrix);
        NuSpecialDrawAt(&WORLD->lev_objs[0x126].special, &matrix);
        position.y += step;
    }
}

void ZipUps_DrawLines() {
    GameObject_s *object = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        NURND_VERTEX3D start, end;
        if (object->character_context == 0x47) {
            if (object->action_movement_state != 0)
                continue;
            ZipUp_GetStartPoint(object, &start.position);
            f32 time = object->context_animation_timer;
            start.colour = 0xffffffff;
            end.position = static_cast<ZIPUP *>(object->field_0x788)->hook_position;
            if (time < 0.2f) {
                f32 fraction = time / 0.2f;
                end.position.x = (end.position.x - start.position.x) * fraction + start.position.x;
                end.position.y = (end.position.y - start.position.y) * fraction + start.position.y;
                end.position.z = (end.position.z - start.position.z) * fraction + start.position.z;
            }
            end.colour = 0xff808080;
            DrawRopeSingle(&start.position, &end.position, 1.0f, ropemtl, time, 0.2f, 3.5f, 1.0f);
        } else if (object->character_context == 0x35) {
            end.position = object->external_force;
            start.colour = 0xffffffff;
            end.colour = 0xff808080;
            ZipUp_GetStartPoint(object, &start.position);
            DrawRopeSingle(&start.position, &end.position, 1.0f, ropemtl, object->context_animation_timer, 0.2f, 3.5f,
                           1.0f);
        }
    }
}

ZIPUP *ZipUp_FindNearest(WORLDINFO_s *world, nuvec_s *position, float radius, float *distance, i32 *endpoint,
                         GameObject_s *object, bool touch) {
    if (world == NULL || object == NULL || object->apiobj.character_data == NULL ||
        object->apiobj.character_data->game_character == NULL)
        return NULL;
    if (touch && !TouchHacks::CanUseZipup(*object))
        return NULL;
    if (world->zipups == NULL)
        return NULL;
    float nearest_distance = (radius + 0.25f) * (radius + 0.25f);
    ZIPUP *nearest = NULL;
    ZIPUP *zipup = world->zipups;
    for (i32 i = 0; i < world->zipup_count; ++i, ++zipup) {
        if (zipup != NULL) {
            if ((zipup->flags & 0xc0) != 0xc0)
                continue;
            if ((zipup->flags & 0x20) != 0) {
                if ((object->apiobj.character_data->game_character->flags_094[3] & 0x20) == 0)
                    continue;
            } else {
                if ((object->apiobj.character_data->game_character->flags_094[3] & 0x20) != 0)
                    continue;
                if ((zipup->flags & 2) == 0 && object->apiobj.field_0x27c != -1)
                    continue;
            }
        }
        float candidate = NuVecDistSqr(position, &zipup->lower_position, NULL);
        if (candidate < nearest_distance) {
            nearest = zipup;
            nearest_distance = candidate;
            if (endpoint != NULL)
                *endpoint = 0;
        }
        if ((zipup->flags & 8) != 0) {
            candidate = NuVecDistSqr(position, &zipup->upper_position, NULL);
            if (candidate < nearest_distance) {
                if (endpoint != NULL)
                    *endpoint = 2;
                nearest = zipup;
                nearest_distance = candidate;
            }
        }
    }
    if (distance != NULL)
        *distance = nearest_distance;
    return nearest;
}

i32 ZipUps_UpdateHint(HINT_s *hint) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (world == NULL || world->zipups == NULL)
        return 0;
    for (i32 i = 0; i < world->zipup_count; ++i) {
        ZIPUP *zipup = &world->zipups[i];
        if (hint->completion_flags[MechInputTouchSystem::s_baseControlMode] == 0 && CanDrawZipUpSwirls != 0 &&
            (zipup->flags & ZIPUP_FLAG_ACTIVE) != 0 && ActivePlayerInRange(&zipup->lower_position, 2.0f, NULL))
            return 1;
    }
    return 0;
}
