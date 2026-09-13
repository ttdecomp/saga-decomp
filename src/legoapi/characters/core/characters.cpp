#include "nu2api/nu3d/nuportal.h"
#include "decomp.h"
#include "gameapi/ai/aisys/aisys.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world_shared.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nutex.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/charconfig.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/characters/motion.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/render/fx.h"
#include "legoapi/gizmos/object/lever.h"
#include "legoapi/gizmos/door/zipups.h"
#include "legoapi/gizmos/object/technos.h"
#include "legoapi/world/level.h"
#include "legoapi/world/areas.h"
#include "legoapi/world/world.h"
#include "legoapi/world/mission.h"
#include "gameapi/ai/aisys/aisys.h"
#include "gameapi/edtools/edstubs.h"
#include "nu2api/nucore/bgproc.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/nustring.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "legoapi/menus/core/text.h"
#include "nu2api/numath/nutrig.h"

static CHARSCENE_s *CharScene_Area;

// LoadPerm1 is one of the few game-level entry points which wires together
// otherwise C-linkage engine subsystems.  Keep these declarations local: the
// individual subsystem TUs intentionally expose their original plain names.
extern "C" {
    i32 rtlInitDynamic(VARIPTR *, VARIPTR, i32);
    void DebrisSetup(VARIPTR *, VARIPTR, char *, i32, i32, i32);
    void DebrisRegisterCutoffCameraVec(void *);
    void edgraSetup(VARIPTR *, VARIPTR, i32, i32, i32);
    void InitParts(i32, VARIPTR *, VARIPTR);
    void ParticleReset(void);
    i32 NuFileExists(char *);
    i32 NuStrCpy(char *, const char *);
    i32 edppLoadPage(char *, i32, usize);
    void NuRndrShadowInit(u8 *);
    void NuTexAnimProgSysInit(void);
    void terrainpickupinit(char *, void **);

    extern i32 Grass_Available;
    extern i32 DEBPAGE_GENERAL;
    extern i32 DEBPAGE_CHARACTER;
    extern void *perm_debrissys;
}

struct AISYS_s;
void AIScriptLoadAll(char *path, VARIPTR *buf, VARIPTR *buf_end, AISYS_s *sys);
void InitTexAnimScripts(char **names);
void BackDrop_Init(char *path, VARIPTR *buf, VARIPTR *buf_end);

i32 PARTPAGE_GENERAL = -1;

// The original table at 0x006281a0.  These are the material animation
// scripts made available before the permanent things scene is loaded.
char *TexAnimList_LSW[32] = {
    (char *)"arrow",          (char *)"blink_01",      (char *)"blink_02",    (char *)"blink_03",
    (char *)"coin",           (char *)"coin_pause",    (char *)"control",     (char *)"ds_esc_intro_1",
    (char *)"ds_esc_intro_2", (char *)"ds_1",          (char *)"helpmeobi",   (char *)"lightening1",
    (char *)"plasma",         (char *)"screen",        (char *)"Dso_screen",  (char *)"anakinbows",
    (char *)"anakinkilling",  (char *)"anakinspod",    (char *)"boost1",      (char *)"boost2",
    (char *)"boost3",         (char *)"boost4",        (char *)"gasganospod", (char *)"helpobi",
    (char *)"hologram1",      (char *)"hologramnoise", (char *)"play8",       (char *)"quidie",
    (char *)"sebulbaspod",    (char *)"sidious",       (char *)"crowd",       NULL,
};

void InitStreaks(VARIPTR *, VARIPTR, char *);
void InitRipples(ripple_set_s **, VARIPTR *, VARIPTR *, i32);
void CreateFadeMaterials();
void CreateUsefulMaterials();
extern ripple_set_s *ripples;
extern NUMTL *ShadowMat;

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

void CharConfig_ConfigureAll(i32 permanent, nufpcomjmp_s *game_keywords);
void ExtraCharacterFixUpAfterConfig();
extern i32 CHARPAK;
extern i32 apiloadcharactermodels_nopakfile;
extern "C" i32 apiloadcharactermodels_append;

extern i32 GetMenuID(void);
extern i32 InCollectList_Index(i32 id, COLLECTID *list, i32 count);
extern i32 Collection_Got(i32 id);
extern void IconScenes_Load(APICHARACTERMODELLIST_s *list, i32 permanent, VARIPTR *buf, VARIPTR *buf_end);
extern NUGSCN *IconScene_FindById(i32 character_id);
extern void Customiser_SaveModelTextureIDs(CUSTOMISER *customiser, CHARACTERMODEL_s *model);
extern CUSTOMISER *CharacterCustomiser;
extern VARIPTR characterbuffer_ptr;
extern VARIPTR characterbuffer_end;
extern i32 waiting_for_character;
extern f32 WaitingForCharacterTime;

extern "C" {
    i32 hub_character_ready = -1;
}

APICHARACTERMODELLIST_s PermModelList[] = {{-1, 0}};

enum LegoObjectId : i16 {
    LEGO_OBJECT_FLOOR_TARGET = 0x55,
};

void LSW_SetIndy(i32) {
    LedgeTerrain_On = 0;
    Grapples_Available = 0;
    SuperCarry_Bash = 0;
    SuperCarry_Jump = 0;
    PUNCHGAP = 0.3f;
    PUNCHCHARGAP = 0.3f;
    HINTS_ON = 1;

    TechnoSys.interaction_time = 0.2f;
    TechnoSys.idle_offset = v000;
    TechnoSys.active_offset = v000;
    TechnoSys.active_effect_id = 0x2c;
    TechnoSys.success_effect_id = -1;
    TechnoSys.failure_effect_id = -1;
    TechnoSys.complete_offset = v000;
    TechnoSys.floor_target_object_id = LEGO_OBJECT_FLOOR_TARGET;
    TechnoSys.activation_effect_id = 0x2d;
    TechnoSys.completion_effect_id = 0x2e;

    LeverSys.floor_target_object_id = LEGO_OBJECT_FLOOR_TARGET;
    GameAudio_SetActionMusicTimes(1.0f, 6.0f);
}

void HairMovement(GameObject_s *) {
}

void HeadMovement(GameObject_s *object) {
    NUVEC direction;
    if (CInfo[object->character_context].flags & 0x200000) {
        return;
    }
    if (!(object->apiobj.character_data->game_character->field_0x94 & 0x40000000) &&
        !(object->apiobj.character_data->model_flags & 0x20000)) {
        CHARACTERDATA *character = object->apiobj.character_data;

        if (object->field_0x1089 < 3 && character->game_character->head_joint != -1) {
            object->field_0xefe &= ~1;
            i32 allowed;
            i16 animation;
            if (!object->apiobj.anim_packet.blending) {
                animation = object->apiobj.anim_packet.animation_index;
                allowed = 1;
                if (animation < 0)
                    goto gate_done_ordinary;
            } else {
                animation = object->apiobj.anim_packet.blend_animation_b;
                if (animation >= 0 && animation < apicharsys->model_id_capacity &&
                    object->apiobj.character_model->model_data_b[animation] != NULL) {
                    if (static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[animation])->flags &
                        0x40) {
                        allowed = 0;
                        goto gate_done_ordinary;
                    }
                }
                animation = object->apiobj.anim_packet.blend_animation_a;
                allowed = 1;
                if (animation < 0)
                    goto gate_done_ordinary;
            }

            if (animation < apicharsys->model_id_capacity &&
                object->apiobj.character_model->model_data_b[animation] != NULL) {
                allowed =
                    (static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[animation])->flags &
                     0x40) == 0;
            }
        gate_done_ordinary:;

            f32 first = 0.0f;
            f32 second = 0.0f;
            if (object->head_target != NULL && object->head_target_timer >= object->head_target_delay &&
                character->game_character->head_locator != -1 && allowed) {
                object->field_0xefe |= 1;

                NuVecInvMtxTransform(&direction, &object->head_target_position,
                                     &object->joint_matrices[character->game_character->head_locator]);
                if (direction.z < 0.0f) {
                    f32 angle = NuAtan2(-direction.y, NuFsqrt(direction.x * direction.x + direction.z * direction.z));
                    first = object->joint_modifiers[object->field_0x1089].rotation.y - angle;
                    f32 limit = character->game_character->field_0x54;
                    if (first >= limit)
                        first = limit;
                    else if (first <= -limit)
                        first = -limit;
                    angle = NuAtan2(-direction.x, -direction.z);
                    second = angle + object->joint_modifiers[object->field_0x1089].rotation.z;
                    limit = character->game_character->field_0x50;
                    if (second >= limit)
                        second = limit;
                    else if (second <= -limit)
                        second = -limit;
                    else
                        object->field_0xefe &= ~1;
                }
            }
            object->joint_modifiers[object->field_0x1089].joint_index = character->game_character->head_joint;
            object->joint_modifiers[object->field_0x1089].flags = static_cast<NUJOINTANIM_FLAGS>(0);
            f32 blend = character->game_character->field_0x58 * FRAMETIME;
            f32 previous;
            if (blend > 1.0f) {
                blend = 1.0f;
                previous = 0.0f;
            } else {
                previous = 1.0f - blend;
            }
            object->joint_modifiers[object->field_0x1089].rotation.x = 0.0f;
            object->joint_modifiers[object->field_0x1089].rotation.y =
                first * blend + object->joint_modifiers[object->field_0x1089].rotation.y * previous;
            object->joint_modifiers[object->field_0x1089].rotation.z =
                second * blend + object->joint_modifiers[object->field_0x1089].rotation.z * previous;
            i32 limit = static_cast<i32>(character->game_character->field_0x54 * 10430.3779296875f);
            object->joint_modifiers[object->field_0x1089].rotation_limit_start[1] = limit;
            object->joint_modifiers[object->field_0x1089].rotation_limit_end[1] = -limit;
            limit = static_cast<i32>(character->game_character->field_0x50 * 10430.3779296875f);
            object->joint_modifiers[object->field_0x1089].rotation_limit_start[2] = limit;
            object->joint_modifiers[object->field_0x1089].rotation_limit_end[2] = -limit;
            if (object->joint_modifiers[object->field_0x1089].rotation.y > 0.0017453293548896909f ||
                object->joint_modifiers[object->field_0x1089].rotation.y < -0.0017453293548896909f ||
                object->joint_modifiers[object->field_0x1089].rotation.z > 0.0017453293548896909f ||
                object->joint_modifiers[object->field_0x1089].rotation.z < -0.0017453293548896909f) {
                object->joint_modifiers[object->field_0x1089].flags = NUJOINTANIM_ROTATION;
                if (allowed) {
                    if (object->joint_modifiers[object->field_0x1089].rotation.y != 0.0f)
                        object->joint_modifiers[object->field_0x1089].flags =
                            static_cast<NUJOINTANIM_FLAGS>(NUJOINTANIM_ROTATION | NUJOINTANIM_LIMIT_ROTATION_Y);
                    if (object->joint_modifiers[object->field_0x1089].rotation.z != 0.0f)
                        object->joint_modifiers[object->field_0x1089].flags = static_cast<NUJOINTANIM_FLAGS>(
                            object->joint_modifiers[object->field_0x1089].flags | NUJOINTANIM_LIMIT_ROTATION_Z);
                }
            }
            if (object->head_target != NULL) {
                object->head_target_timer -= FRAMETIME;
                if (object->head_target_timer < 0.0f) {
                    object->head_target_timer = 0.0f;
                    object->head_target_delay = 0.0f;
                    object->head_target = NULL;
                    object->head_target_priority = 0;
                }
            }
            object->field_0x1089 = object->field_0x1089 + 1;
        }
    } else if (!(object->apiobj.character_data->game_character->field_0x94 & 0x40000000)) {
        CHARACTERDATA *character = object->apiobj.character_data;

        if (object->field_0x1089 > 2 || character->game_character->head_joint == -1) {
            return;
        }

        i32 allowed;
        i16 animation;
        if (!object->apiobj.anim_packet.blending) {
            animation = object->apiobj.anim_packet.animation_index;
            allowed = 1;
            if (animation < 0)
                goto gate_done_old;
        } else {
            animation = object->apiobj.anim_packet.blend_animation_b;
            if (animation >= 0 && animation < apicharsys->model_id_capacity &&
                object->apiobj.character_model->model_data_b[animation] != NULL) {
                if (static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[animation])->flags &
                    0x40) {
                    allowed = 0;
                    goto gate_done_old;
                }
            }
            animation = object->apiobj.anim_packet.blend_animation_a;
            allowed = 1;
            if (animation < 0)
                goto gate_done_old;
        }

        if (animation < apicharsys->model_id_capacity &&
            object->apiobj.character_model->model_data_b[animation] != NULL) {
            allowed = (static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[animation])->flags &
                       0x40) == 0;
        }
    gate_done_old:;

        f32 first = 0.0f;
        f32 second = 0.0f;
        if (object->head_target != NULL && object->head_target_timer >= object->head_target_delay &&
            character->game_character->head_locator != -1 && allowed) {

            NuVecInvMtxTransform(&direction, &object->head_target_position,
                                 &object->joint_matrices[character->game_character->head_locator]);
            if (direction.z < 0.0f) {
                f32 angle = NuAtan2(-direction.x, -direction.z);
                first = object->joint_modifiers[object->field_0x1089].rotation.x - angle;
                f32 limit = character->game_character->field_0x50;
                if (first >= limit)
                    first = limit;
                else if (first <= -limit)
                    first = -limit;
                angle = NuAtan2(-direction.y, -direction.z);
                second = object->joint_modifiers[object->field_0x1089].rotation.y - angle;
                limit = character->game_character->field_0x54;
                if (second >= limit)
                    second = limit;
                else if (second <= -limit)
                    second = -limit;
            }
        }
        object->joint_modifiers[object->field_0x1089].joint_index = character->game_character->head_joint;
        object->joint_modifiers[object->field_0x1089].flags = static_cast<NUJOINTANIM_FLAGS>(0);
        f32 blend = character->game_character->field_0x58 * FRAMETIME;
        f32 previous;
        if (blend > 1.0f) {
            blend = 1.0f;
            previous = 0.0f;
        } else {
            previous = 1.0f - blend;
        }
        object->joint_modifiers[object->field_0x1089].rotation.z = 0.0f;
        object->joint_modifiers[object->field_0x1089].rotation.x =
            first * blend + object->joint_modifiers[object->field_0x1089].rotation.x * previous;
        object->joint_modifiers[object->field_0x1089].rotation.y =
            second * blend + object->joint_modifiers[object->field_0x1089].rotation.y * previous;
        i32 limit = static_cast<i32>(character->game_character->field_0x50 * 10430.3779296875f);
        object->joint_modifiers[object->field_0x1089].rotation_limit_start[0] = limit;
        object->joint_modifiers[object->field_0x1089].rotation_limit_end[0] = -limit;
        limit = static_cast<i32>(character->game_character->field_0x54 * 10430.3779296875f);
        object->joint_modifiers[object->field_0x1089].rotation_limit_start[1] = limit;
        object->joint_modifiers[object->field_0x1089].rotation_limit_end[1] = -limit;
        if (object->joint_modifiers[object->field_0x1089].rotation.x > 0.0017453293548896909f ||
            object->joint_modifiers[object->field_0x1089].rotation.x < -0.0017453293548896909f ||
            object->joint_modifiers[object->field_0x1089].rotation.y > 0.0017453293548896909f ||
            object->joint_modifiers[object->field_0x1089].rotation.y < -0.0017453293548896909f) {
            object->joint_modifiers[object->field_0x1089].flags = NUJOINTANIM_ROTATION;
            if (allowed) {
                if (object->joint_modifiers[object->field_0x1089].rotation.x != 0.0f)
                    object->joint_modifiers[object->field_0x1089].flags =
                        static_cast<NUJOINTANIM_FLAGS>(NUJOINTANIM_ROTATION | NUJOINTANIM_LIMIT_ROTATION_X);
                if (object->joint_modifiers[object->field_0x1089].rotation.y != 0.0f)
                    object->joint_modifiers[object->field_0x1089].flags = static_cast<NUJOINTANIM_FLAGS>(
                        object->joint_modifiers[object->field_0x1089].flags | NUJOINTANIM_LIMIT_ROTATION_Y);
            }
        }
        if (object->head_target != NULL) {
            object->head_target_timer -= FRAMETIME;
            if (object->head_target_timer < 0.0f) {
                object->head_target_timer = 0.0f;
                object->head_target_delay = 0.0f;
                object->head_target = NULL;
                object->head_target_priority = 0;
            }
        }
        object->field_0x1089 = object->field_0x1089 + 1;
    } else {
        if (object->field_0x1089 < 3 && object->apiobj.character_data->game_character->head_joint != -1) {
            object->field_0xefe &= ~1;
            i32 allowed;
            i16 animation;
            if (!object->apiobj.anim_packet.blending) {
                animation = object->apiobj.anim_packet.animation_index;
                allowed = 1;
                if (animation < 0)
                    goto gate_done_alternate;
            } else {
                animation = object->apiobj.anim_packet.blend_animation_b;
                if (animation >= 0 && animation < apicharsys->model_id_capacity &&
                    object->apiobj.character_model->model_data_b[animation] != NULL) {
                    if (static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[animation])->flags &
                        0x40) {
                        allowed = 0;
                        goto gate_done_alternate;
                    }
                }
                animation = object->apiobj.anim_packet.blend_animation_a;
                allowed = 1;
                if (animation < 0)
                    goto gate_done_alternate;
            }

            if (animation < apicharsys->model_id_capacity &&
                object->apiobj.character_model->model_data_b[animation] != NULL) {
                allowed =
                    (static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[animation])->flags &
                     0x40) == 0;
            }
        gate_done_alternate:;

            f32 first = 0.0f;
            f32 second = 0.0f;
            if (object->head_target != NULL && object->head_target_timer >= object->head_target_delay &&
                object->apiobj.character_data->game_character->head_locator != -1 && allowed) {
                object->field_0xefe |= 1;

                NuVecInvMtxTransform(
                    &direction, &object->head_target_position,
                    &object->joint_matrices[object->apiobj.character_data->game_character->head_locator]);
                if (direction.z < 0.0f) {
                    f32 angle = NuAtan2(-direction.y, NuFsqrt(direction.x * direction.x + direction.z * direction.z));
                    first = angle - object->joint_modifiers[object->field_0x1089].rotation.y;
                    f32 limit = object->apiobj.character_data->game_character->field_0x54;
                    if (first >= limit)
                        first = limit;
                    else if (first <= -limit)
                        first = -limit;
                    angle = NuAtan2(-direction.x, -direction.z);
                    second = -angle - object->joint_modifiers[object->field_0x1089].rotation.z;
                    limit = object->apiobj.character_data->game_character->field_0x50;
                    if (second >= limit)
                        second = limit;
                    else if (second <= -limit)
                        second = -limit;
                    else
                        object->field_0xefe &= ~1;
                }
            }
            CHARACTERDATA *character = object->apiobj.character_data;
            object->joint_modifiers[object->field_0x1089].joint_index = character->game_character->head_joint;
            object->joint_modifiers[object->field_0x1089].flags = static_cast<NUJOINTANIM_FLAGS>(0);
            f32 blend = object->apiobj.character_data->game_character->field_0x58 * FRAMETIME;
            f32 previous;
            if (blend > 1.0f) {
                blend = 1.0f;
                previous = 0.0f;
            } else {
                previous = 1.0f - blend;
            }
            object->joint_modifiers[object->field_0x1089].rotation.x = 0.0f;
            object->joint_modifiers[object->field_0x1089].rotation.y =
                first * blend + object->joint_modifiers[object->field_0x1089].rotation.y * previous;
            object->joint_modifiers[object->field_0x1089].rotation.z =
                second * blend + object->joint_modifiers[object->field_0x1089].rotation.z * previous;
            i32 limit = static_cast<i32>(object->apiobj.character_data->game_character->field_0x54 * 10430.3779296875f);
            object->joint_modifiers[object->field_0x1089].rotation_limit_start[1] = limit;
            object->joint_modifiers[object->field_0x1089].rotation_limit_end[1] = -limit;
            limit = static_cast<i32>(object->apiobj.character_data->game_character->field_0x50 * 10430.3779296875f);
            object->joint_modifiers[object->field_0x1089].rotation_limit_start[2] = limit;
            object->joint_modifiers[object->field_0x1089].rotation_limit_end[2] = -limit;
            if (object->joint_modifiers[object->field_0x1089].rotation.y > 0.0017453293548896909f ||
                object->joint_modifiers[object->field_0x1089].rotation.y < -0.0017453293548896909f ||
                object->joint_modifiers[object->field_0x1089].rotation.z > 0.0017453293548896909f ||
                object->joint_modifiers[object->field_0x1089].rotation.z < -0.0017453293548896909f) {
                object->joint_modifiers[object->field_0x1089].flags = NUJOINTANIM_ROTATION;
                if (allowed) {
                    if (object->joint_modifiers[object->field_0x1089].rotation.y != 0.0f)
                        object->joint_modifiers[object->field_0x1089].flags =
                            static_cast<NUJOINTANIM_FLAGS>(NUJOINTANIM_ROTATION | NUJOINTANIM_LIMIT_ROTATION_Y);
                    if (object->joint_modifiers[object->field_0x1089].rotation.z != 0.0f)
                        object->joint_modifiers[object->field_0x1089].flags = static_cast<NUJOINTANIM_FLAGS>(
                            object->joint_modifiers[object->field_0x1089].flags | NUJOINTANIM_LIMIT_ROTATION_Z);
                }
            }
            if (object->head_target != NULL) {
                object->head_target_timer -= FRAMETIME;
                if (object->head_target_timer < 0.0f) {
                    object->head_target_timer = 0.0f;
                    object->head_target_delay = 0.0f;
                    object->head_target = NULL;
                    object->head_target_priority = 0;
                }
            }
            object->field_0x1089 = object->field_0x1089 + 1;
        }
    }
}

void fullcodename(i32) {
}

nuhspecial_s *CharScene_FindHSpecial(WORLDINFO_s *world, i32 character_id);

void CharScene_Draw(WORLDINFO_s *world, i32 character_id, numtx_s *matrix, numtx_s *reflection_matrix) {
    nuhspecial_s *special = CharScene_FindHSpecial(world, character_id);
    if (special != NULL) {
        if (matrix != NULL) {
            NuSpecialDrawAt(special, matrix);
        }
        if (reflection_matrix != NULL) {
            NuSpecialDrawAt(special, reflection_matrix);
        }
    }
}

i32 CharIDFromName(char *name) {
    for (i32 i = 0; i < CHARCOUNT; i++) {
        if (NuStrICmp(CDataList[i].file, name) == 0) {
            return i;
        }
    }

    return -1;
}

CHARACTERDATA *CDataFromName(char *name) {
    for (i32 i = 0; i < CHARCOUNT; i++) {
        if (NuStrICmp(CDataList[i].file, name) == 0) {
            return &CDataList[i];
        }
    }

    return nullptr;
}

i32 RedirectAnim(char *path, ANIMREDIRECT *redirects, ANIMLIST_s *animation_list, char *directory) {
    CHARACTERANIM_s *animation = reinterpret_cast<CHARACTERANIM_s *>(animation_list);
    for (ANIMREDIRECT *redirect = redirects; redirect->name != NULL; ++redirect) {
        if (redirect->animation_id == animation->animation_id && NuStrICmp(redirect->name, animation->name) == 0) {
            NuStrCpy(path, directory);
            NuStrCat(path, animation->name);
            animation->flags &= ~CHARACTER_ANIMATION_FLAG_BSA;
            return 1;
        }
    }
    return 0;
}

void CharScenes_Init(variptr_u *buf, variptr_u *) {
    buf->addr = ALIGN(buf->addr, 4);
    CharScene_Area = reinterpret_cast<CHARSCENE_s *>(buf->void_ptr);
    buf->addr += static_cast<usize>(CHARCOUNT) * sizeof(*CharScene_Area);
    memset(CharScene_Area, 0, static_cast<usize>(CHARCOUNT) * sizeof(*CharScene_Area));
}

void CharScenes_LevelLoad(WORLDINFO *world) {
    if (CHARCOUNT <= 0) {
        return;
    }

    for (i32 i = 0; i < CHARCOUNT; i++) {
        CHARSCENE_s *entry = &world->minikit.character_scenes[i];
        entry->scene = NULL;

        // Check if we should load this character scene
        if ((CharScene_Area == NULL || CharScene_Area[i].scene == NULL) && (CDataList[i].flags & 1) != 0 &&
            world->cutscene_sys != NULL) {
            // Check if this character is in a cutscene
            u32 *cutscene_flags = *(u32 **)((char *)world->cutscene_sys + 8);
            u32 flag = (cutscene_flags[i >> 5] >> (i & 0x1f)) & 1;
            if (flag != 0) {
                // Load the character scene
                char path[136];
                VARIPTR buf_end = world->unknown_0108;
                sprintf(path, "chars\\%s\\%s.gsc", CDataList[i].dir, CDataList[i].file);
                NUGSCN *scene = NuGScnRead(&world->giz_buffer, buf_end, path);
                entry->scene = scene;
                if (scene != NULL) {
                    NuSpecialFind(scene, &entry->special_scene, CDataList[i].file, 1);
                }
            }
        }
    }
}

extern i16 tUNKNOWN;
void Move_CHARACTER(GameObject_s *);
void Animate_CHARACTER(GameObject_s *);

void FixUpCharacters(CHARFIXUP *fixup) {
    for (i32 i = 0; i < CHARCOUNT; ++i) {
        GCDataList[i] = GCDATA_DEFAULT;

        CHARACTERDATA &character = CDataList[i];
        if (character.field0_0x0 == -1) {
            character.field0_0x0 = tUNKNOWN;
        }
        if (character.move_fn == NULL) {
            character.move_fn = Move_CHARACTER;
        }
        if (character.animate_fn == NULL) {
            character.animate_fn = Animate_CHARACTER;
        }
    }

    if (fixup != NULL) {
        while (fixup->name != NULL) {
            if (fixup->id != NULL) {
                *fixup->id = (i16)CharIDFromName(fixup->name);
            }
            ++fixup;
        }
    }
}

void PostAnimate_FETT(GameObject_s *) {
}

void ResetAICreature(GameObject_s *, AISYS_s *);
void LightGameObject(GameObject_s *, void *);
void InitSurfaceInfo(GameObject_s *);
i32 SetObjOnSurface(GameObject_s *, i32);
extern NUVEC plr_lastpos;

GameObject_s *ActivateCharacter(char *name, nuvec_s *position, i32 angle) {
    if (Mission_Active(NULL) != NULL || name == NULL)
        return NULL;
    GameObject_s *object = GetNamedGameObject(WORLD->ai_sys, name);
    if (object == NULL || (object->apiobj.field_0x1f8 & 0x1000) != 0)
        return NULL;
    if (FreePlay != 0 && (object->apiobj.field_0x1f4 & 0x400) == 0)
        return NULL;
    if (object->ai.field_0x134 != 0xff) {
        ResetAICreature(object, WORLD->ai_sys);
    } else {
        object->apiobj.field_0x1f8 |= 0x1000;
        AIScriptSetBaseScriptStateByName(reinterpret_cast<AISCRIPTPROCESS *>(&object->ai), const_cast<char *>("Base"));
        if (position != NULL) {
            object->apiobj.position = *position;
            object->apiobj.field_0x276 = object->apiobj.facing_angle = object->apiobj.movement_facing_angle = angle;
            AISysGetCharacterPathPos(WORLD->ai_sys, &object->apiobj, &object->ai, 0xff, 1);
            object->apiobj.initial_position = object->apiobj.position;
            object->apiobj.collision_position = object->apiobj.position;
            plr_lastpos = object->apiobj.position;
            object->apiobj.start_position = object->apiobj.position;
            object->apiobj.respawn_position = object->apiobj.position;
            object->apiobj.last_safe_position = object->apiobj.position;
            object->saved_position = object->apiobj.position;
            object->reset_velocity = v000;
            object->apiobj.velocity = v000;
            InitSurfaceInfo(object);
            SetObjOnSurface(object, 0);
        }
    }
    LightGameObject(object, WORLD->rtl_set);
    return object;
}

void FinishWeirdoNames(i32) {
}

extern i32 addcreature_override_id_check;

extern f32 default_mover_extra;
extern void SetGameObjectCharacterData(GameObject_s *obj);
extern void GetTopBot(GameObject_s *obj);
extern void GameObjectDimensions(GameObject_s *obj);
extern void GameObjectOrigin(GameObject_s *obj);
extern i32 GetDefaultIdle(GameObject_s *obj);
extern void ResetCharacterIdle(GameObject_s *obj, i32 mode, i32 idle);
extern void *Suit_GetDefault(i32 id);
extern void ResetLights(NUVEC *position, rtldata_s *data, void *set);
extern "C" void ResetAnimPacket(void *packet, i32 enabled);
extern void ResetPlayerPacket(PLAYERPACKET_s *packet, CHARACTERDATA_s *data);

static u32 LayerBit(u8 layer) {
    return 1u << layer;
}

static void SetLayers_BOB(GameObject_s *object) {
    i32 head_layer[6] = {1, 2, 3, 21, 22, 23};
    i32 body_layer[8] = {5, 6, 7, 8, 9, 10, 16, 17};
    i32 legs_layer[3] = {11, 12, 15};
    i32 arms_layer[2] = {13, 14};
    i32 hands_layer[2] = {19, 20};
    object->random_layer_variant = qrand() <= 0x7fff;
    qrand();
    object->field_0x1054 = 1;
    object->field_0x1054 |= 1u << head_layer[qrand() / 0x2aab];
    object->field_0x1054 |= 1u << body_layer[qrand() / 0x2000];
    object->field_0x1054 |= 1u << legs_layer[qrand() / 0x5556];
    object->field_0x1054 |= 1u << arms_layer[qrand() / 0x8000];
    object->field_0x1054 |= 1u << hands_layer[qrand() / 0x8000];
}

static void SetLayers_MOSEISLEYCITIZEN(GameObject_s *object) {
    static const u8 hat_layers[5] = {0, 0, 0, 7, 14};
    static const u8 head_layers[5] = {20, 20, 8, 8, 4};
    static const u8 body_layers[3] = {3, 9, 19};
    static const u8 arm_layers[3] = {1, 12, 15};
    static const u8 hand_layers[3] = {2, 13, 16};
    static const u8 waist_layers[3] = {6, 10, 17};
    static const u8 leg_layers[3] = {5, 11, 18};

    object->field_0x1054 = 0;
    object->field_0x1054 |= LayerBit(hat_layers[qrand() / 0x3334]);
    object->field_0x1054 |= LayerBit(head_layers[qrand() / 0x3334]);
    object->field_0x1054 |= LayerBit(body_layers[qrand() / 0x5556]);
    object->field_0x1054 |= LayerBit(arm_layers[qrand() / 0x5556]);
    object->field_0x1054 |= LayerBit(hand_layers[qrand() / 0x5556]);
    object->field_0x1054 |= LayerBit(waist_layers[qrand() / 0x5556]);
    object->field_0x1054 |= LayerBit(leg_layers[qrand() / 0x5556]);
}

i32 InitCreature(GameObject_s *obj, i32 id, i32 param) {
    addcreature_override_id_check = 0;
    if (id < 0 || id > 0x153 || apicharsys->playermodelids[id] == -1) {
        return 0;
    }

    CHARACTERDATA *character_data = &apicharsys->char_data[id];
    GAMECHARACTERDATA *game_character_data = static_cast<GAMECHARACTERDATA *>(character_data->field11_0x24);
    obj->apiobj.character_data = character_data;
    obj->field_0x1054 = game_character_data->layer_mask;
    obj->apiobj.field_0x1f4 = 0;
    obj->field_0x107c = -1;
    memset(obj->player_packet, 0, 0x798);
    obj->apiobj.field_0x27c = -1;

    NUVEC *start_position = Player_StartPos(obj);
    if (param == 0) {
        obj->apiobj.field_0x1f4 |= 2;
    } else {
        obj->apiobj.field_0x1f4 |= 0x4002;
    }
    if ((WORLD->current_level->flags & 0x40000) == 0) {
        obj->apiobj.field_0x1f4 |= 0x40;
    }

    obj->pad_gamepad = GamePad_Allocate();
    obj->pad_gamepad->unknown_24 |= 0x100;
    obj->hitpoints = game_character_data->hitpoints;
    obj->current_hp = game_character_data->hitpoints;
    ResetPlayerPacket(reinterpret_cast<PLAYERPACKET_s *>(obj->player_packet),
                      reinterpret_cast<CHARACTERDATA_s *>(character_data));

    obj->apiobj.field_0x1fc = v000.x;
    obj->apiobj.field_0x200 = v000.y;
    obj->apiobj.field_0x204 = v000.z;
    obj->field_0xe38 = 4;
    obj->field_0xe37 = game_character_data->field_0xf5;
    obj->id = static_cast<i16>(id);
    obj->apiobj.character_model = &apicharsys->models[apicharsys->playermodelids[id]];
    obj->suit = Suit_GetDefault(id);
    obj->ai.field_0x134 = 0xff;
    SetGameObjectCharacterData(obj);

    obj->apiobj.start_position = *start_position;
    obj->apiobj.initial_position = *start_position;
    obj->apiobj.position = *start_position;
    obj->apiobj.pos_x = start_position->x;
    obj->apiobj.pos_y = start_position->y;
    obj->apiobj.pos_z = start_position->z;
    obj->field_0x1018 = 0.5f;

    GetTopBot(obj);
    GameObjectDimensions(obj);
    obj->apiobj.anim_packet.animation_index = 1;

    i32 reset_animation = 1;
    if (obj->apiobj.character_model != NULL) {
        void **animation_table = *reinterpret_cast<void ***>(reinterpret_cast<u8 *>(obj->apiobj.character_model) + 0xc);
        if (animation_table != NULL && animation_table[1] == NULL) {
            reset_animation = 0;
            for (i32 i = 0; i < 0xe9; i++) {
                if (animation_table[i] != NULL) {
                    obj->apiobj.anim_packet.animation_index = static_cast<u16>(i);
                    reset_animation = i;
                    break;
                }
            }
        }
    }
    ResetAnimPacket(&obj->apiobj.anim_packet, reset_animation);
    ResetCharacterIdle(obj, 2, GetDefaultIdle(obj));
    ResetLights(&obj->apiobj.position, &obj->light_data, WORLD->rtl_set);

    obj->ai.mover_height = obj->apiobj.field_0x1dc + default_mover_extra;
    GameObjectOrigin(obj);
    obj->apiobj.previous_position[0] = obj->apiobj.position.x;
    obj->apiobj.previous_position[1] = obj->apiobj.position.y;
    obj->apiobj.previous_position[2] = obj->apiobj.position.z;
    obj->field_0x10c8 = obj->apiobj.position.x;
    obj->field_0x10cc = obj->apiobj.position.y;
    obj->field_0x10d0 = obj->apiobj.position.z;

    obj->field_0xf01 = static_cast<u8>((obj->field_0xf01 & ~8u) | (((game_character_data->flags_090 >> 17) & 1) << 3));
    if (id == id_MOSEISLEYCITIZEN) {
        SetLayers_MOSEISLEYCITIZEN(obj);
    } else if (id == id_CANTINAALIEN) {
        static const u8 head_layers[3] = {0, 3, 6};
        static const u8 body_layers[3] = {1, 4, 7};
        static const u8 leg_layers[3] = {2, 5, 8};
        obj->field_0x1054 = LayerBit(head_layers[qrand() / 0x5556]) | LayerBit(body_layers[qrand() / 0x5556]) |
                            LayerBit(leg_layers[qrand() / 0x5556]);
    } else if (id == id_CLOUDCITYCITIZEN) {
        static const u8 head_layers[3] = {1, 5, 7};
        static const u8 body_layers[3] = {2, 3, 6};
        static const u8 leg_layers[3] = {0, 4, 8};
        obj->field_0x1054 = LayerBit(head_layers[qrand() / 0x5556]) | LayerBit(body_layers[qrand() / 0x5556]) |
                            LayerBit(leg_layers[qrand() / 0x5556]);
        obj->field_0xf01 = static_cast<u8>((obj->field_0xf01 & ~8u) | (((obj->field_0x1054 >> 7) & 1) << 3));
    } else if (id == id_BOB) {
        SetLayers_BOB(obj);
    } else if (id == id_GEONOSIAN) {
        obj->random_layer_variant = qrand() <= 0x7fff;
    }

    if ((game_character_data->flags_090 & 0x8000u) != 0) {
        obj->apiobj.field_0x1f4 |= 0x20000;
    } else {
        obj->apiobj.field_0x1f4 &= ~0x20000u;
    }
    return 1;
}

void Shards_HandleLostObj(WORLDINFO_s *, GameObject_s *);
void LoseHelmet(GameObject_s *, i32, i32);
void DestroySnakeBody(GameObject_s *);
void InitPlayerAI(GameObject_s *);
void Player_ClearContext(GameObject_s *, i32);
void ResetPlayerMoves(GameObject_s *);
void Player_ResetContexts(PLAYERPACKET_s *);
void InitSurfaceInfo(GameObject_s *);
i32 SetObjOnSurface(GameObject_s *, i32);
void GizForce_ResetLOS(GameObject_s *);
i32 StartSlide(GameObject_s *, i32);

i32 NewPlayerCharacter(GameObject_s *object, i32 id, i32 old_id, i32) {
    if (id == old_id || id < 0 || id >= CHARCOUNT || apicharsys->playermodelids[id] == -1)
        return 0;
    Shards_HandleLostObj(WORLD, object);
    if (VehicleArea == 0)
        AddGameDebris(WORLD->debris_sys, 0x5c, &object->apiobj.collision_position);
    i8 context = object->character_context;
    i16 lean = object->movement_lean_angle;
    u32 lean_enabled = object->apiobj.character_data->model_flags & 0x2000;
    LoseHelmet(object, 0, 1);
    DestroySnakeBody(object);
    object->id = id;
    NUVEC velocity = object->apiobj.velocity;
    u8 health = object->current_hp;
    u8 hitpoints = object->hitpoints;
    u8 saved_flags = (object->apiobj.flags_high >> 5) & 1;
    object->apiobj.character_model = &apicharsys->models[apicharsys->playermodelids[id]];
    object->apiobj.character_data = &apicharsys->char_data[id];
    object->takeover_source = NULL;
    object->suit = Suit_GetDefault(id);
    u32 saved_f14 = object->field_0xf14;
    u32 saved_f20 = object->field_0xf20;
    u8 saved_efe = (object->field_0xefe >> 5) & 1;
    u8 route = object->ai.current_route;
    u8 next_route = object->ai.next_route;
    AIPATHINFO path = object->ai.path_info;
    u32 frame_state = object->ai.frame_state;
    InitPlayerAI(object);
    object->field_0xefe = (object->field_0xefe & ~0x20) | (saved_efe << 5);
    object->field_0xf20 = saved_f20;
    object->ai.next_route = next_route;
    object->ai.path_info = path;
    object->field_0xf14 = saved_f14;
    object->field_0x1004 = 1.0f;
    object->ai.current_route = route;
    object->ai.frame_state = frame_state;
    GAMECHARACTERDATA *data = object->apiobj.character_data->game_character;
    object->field_0x1054 = data->layer_mask;
    object->field_0xe37 = data->field_0xf5;
    object->field_0xe38 = 4;
    Player_ClearContext(object, 1);
    ResetPlayerMoves(object);
    SetGameObjectCharacterData(object);
    ResetAnimPacket(&object->apiobj.anim_packet, -1);
    Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    GetTopBot(object);
    GameObjectDimensions(object);
    object->hud_icon_timer = 2.0f;
    object->input_toggle_hold_time = TOGGLEHOLDTIME;
    object->field_0xefc |= 0x80;
    InitSurfaceInfo(object);
    i32 on_surface = SetObjOnSurface(object, 1);
    GizForce_ResetLOS(object);
    object->field_0xefe &= ~8;
    object->current_hp = health;
    object->hitpoints = hitpoints;
    object->field_0xef0 = 0;
    object->pause_context_state = 0;
    object->apiobj.flags_high = (object->apiobj.flags_high & ~0x20) | (saved_flags << 5);
    ResetMiniAnimPacket(&object->mini_animation, -1);
    object->weapon_scale = (object->field_0xe22 & 1) != 0 ? 1.0f : 0.0f;
    object->field_0xe32 = 0;
    if (on_surface)
        object->ground_contact_grace_timer = 0.2f;
    else
        object->apiobj.velocity.y = velocity.y;
    if (context == 0x33 && StartSlide(object, 0)) {
        object->apiobj.velocity.x = velocity.x;
        object->apiobj.velocity.z = velocity.z;
    }
    if (object->apiobj.velocity.y == 0.0f)
        object->apiobj.velocity.y = -0.1f;
    if (lean_enabled && (object->apiobj.character_data->model_flags & 0x2000) != 0)
        object->movement_lean_angle = lean;
    data = apicharsys->char_data[object->id].game_character;
    object->apiobj.viewdistance = data->viewdistance;
    object->apiobj.heardistance = data->heardistance;
    object->apiobj.maxviewheight = data->maxviewheight;
    object->apiobj.minviewheight = data->minviewheight;
    data = object->apiobj.character_data->game_character;
    object->field_0xf01 = (object->field_0xf01 & ~8) | (((data->flags_090 >> 17) & 1) << 3);
    if (object->id == id_MOSEISLEYCITIZEN) {
        SetLayers_MOSEISLEYCITIZEN(object);
    } else if (object->id == id_CANTINAALIEN) {
        static const u8 head_layer[3] = {0, 3, 6};
        static const u8 body_layer[3] = {1, 4, 7};
        static const u8 legs_layer[3] = {2, 5, 8};
        object->field_0x1054 = 0;
        object->field_0x1054 |= LayerBit(head_layer[qrand() / 0x5556]);
        object->field_0x1054 |= LayerBit(body_layer[qrand() / 0x5556]);
        object->field_0x1054 |= LayerBit(legs_layer[qrand() / 0x5556]);
    } else if (object->id == id_CLOUDCITYCITIZEN) {
        static const u8 head_layer[3] = {1, 5, 7};
        static const u8 body_layer[3] = {2, 3, 6};
        static const u8 legs_layer[3] = {0, 4, 8};
        object->field_0x1054 = 0;
        object->field_0x1054 |= LayerBit(head_layer[qrand() / 0x5556]);
        object->field_0x1054 |= LayerBit(body_layer[qrand() / 0x5556]);
        object->field_0x1054 |= LayerBit(legs_layer[qrand() / 0x5556]);
        object->field_0xf01 = (object->field_0xf01 & ~8) | (((object->field_0x1054 >> 7) & 1) << 3);
    } else if (object->id == id_BOB) {
        SetLayers_BOB(object);
    } else if (object->id == id_GEONOSIAN) {
        object->random_layer_variant = qrand() <= 0x7fff;
    }
    if ((object->apiobj.character_data->game_character->flags_090 & 0x8000) != 0)
        object->apiobj.field_0x1f4 |= 0x20000;
    else
        object->apiobj.field_0x1f4 &= ~0x20000;
    return 1;
}

extern "C" f32 AnimDuration(i32 character_id, i32 animation, f32 start_frame, f32 end_frame, i32 subtract_frame_time);
i32 GetDefaultIdle(GameObject_s *object);

static i32 IdleRepetitionCount(u8 minimum, u8 maximum) {
    if (maximum <= minimum) {
        return minimum;
    }

    const i32 bucket_size = 0xffff / (maximum - minimum) + 1;
    return minimum + qrand() / bucket_size;
}

void ResetCharacterIdle(GameObject_s *object, i32 mode, i32 animation) {
    // Animation 151 belongs to the held-batarang override and deliberately
    // leaves the ordinary idle scheduler untouched.
    if (animation == 151) {
        return;
    }

    object->idle_animation = static_cast<i16>(animation);
    object->idle_animation_time = 0.0f;

    CHARACTERMODEL_s *model = object->apiobj.character_model;
    CHARACTERANIM_s *animation_info = NULL;
    if (model != NULL && model->model_data_b != NULL && model->model_data_b[animation] != NULL &&
        model->model_data_a != NULL) {
        animation_info = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
    }

    const u8 minimum = animation_info != NULL ? static_cast<u8>(animation_info->minimum_repetitions) : 0;
    if (minimum == 0) {
        object->idle_animation_limit = static_cast<f32>(qrand()) * (1.0f / 65535.0f) * 7.0f + 8.0f;
    } else {
        const u8 maximum = static_cast<u8>(animation_info->maximum_repetitions);
        const i32 repetitions = IdleRepetitionCount(minimum, maximum);
        object->idle_animation_limit = AnimDuration(object->id, animation, 0.0f, 0.0f, 0) * repetitions - FRAMETIME;
    }

    if (mode > 0) {
        object->idle_total_time = 0.0f;
        if (mode != 1) {
            object->previous_idle_animation = -1;
        }
    }
}

static void NewCharacterIdle(GameObject_s *object, i32 default_idle) {
    CHARACTERDATA *character = object->apiobj.character_data;
    GAMECHARACTERDATA *game_character = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
    const i32 alternate_idle = game_character->field275_0x116 == 0 && (character->model_flags & 0x80) != 0 ? 118 : 25;

    i32 candidates[233];
    i32 candidate_count = 0;
    CHARACTERMODEL_s *model = object->apiobj.character_model;
    const bool select_alternate_group = alternate_idle == default_idle;
    for (i32 animation = 0; animation < 233; ++animation) {
        if (model->model_data_b[animation] == NULL) {
            continue;
        }
        CHARACTERANIM_s *info = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
        if ((info->flags & 0x10) == 0) {
            continue;
        }
        const bool alternate_group = (info->flags & 0x800) != 0;
        if (alternate_group == select_alternate_group) {
            candidates[candidate_count++] = animation;
        }
    }

    if (candidate_count == 0) {
        ResetCharacterIdle(object, 0, default_idle);
        return;
    }

    i32 animation;
    if (candidate_count == 1) {
        animation = candidates[0];
    } else {
        do {
            const i32 bucket_size = 0xffff / candidate_count + 1;
            animation = candidates[qrand() / bucket_size];
            object->idle_animation = static_cast<i16>(animation);
        } while (animation == object->previous_idle_animation);
    }
    object->idle_animation = static_cast<i16>(animation);

    CHARACTERANIM_s *info = static_cast<CHARACTERANIM_s *>(model->model_data_a[animation]);
    i32 repetitions = static_cast<u8>(info->minimum_repetitions);
    const u8 maximum = static_cast<u8>(info->maximum_repetitions);
    object->previous_idle_animation = static_cast<i16>(animation);
    if (repetitions > 1 && (info->flags & 2) == 0) {
        repetitions = 1;
    }
    if (repetitions == 0) {
        repetitions = 1;
    } else if (maximum > repetitions) {
        repetitions = IdleRepetitionCount(static_cast<u8>(repetitions), maximum);
    }

    object->idle_animation_time = 0.0f;
    object->idle_animation_limit = AnimDuration(object->id, animation, 0.0f, 0.0f, 0) * repetitions - FRAMETIME;
}

void UpdateCharacterIdle(GameObject_s *object) {
    if (object->apiobj.character_model == NULL) {
        return;
    }

    CHARACTERDATA *character = object->apiobj.character_data;
    GAMECHARACTERDATA *game_character = static_cast<GAMECHARACTERDATA *>(character->field11_0x24);
    const i16 alternate_idle =
        static_cast<i16>(game_character->field275_0x116 == 0 && (character->model_flags & 0x80) != 0 ? 118 : 25);
    ANIMPACKET_s &packet = object->apiobj.anim_packet;
    const i16 requested = packet.requested_animation;

    if (requested != 1 && requested != alternate_idle) {
        const i32 default_idle = GetDefaultIdle(object);
        if (default_idle != -1) {
            ResetCharacterIdle(object, 1, default_idle);
            return;
        }
    }

    if (requested == 1) {
        if (packet.previous_animation == alternate_idle) {
            ResetCharacterIdle(object, 1, 1);
        } else {
            object->idle_total_time += FRAMETIME;
            object->idle_animation_time += FRAMETIME;
            if (object->idle_animation_time >= object->idle_animation_limit) {
                if (object->idle_animation == 1) {
                    NewCharacterIdle(object, 1);
                } else {
                    ResetCharacterIdle(object, 1, 1);
                }
            }
        }
        packet.requested_animation = object->idle_animation;
        return;
    }

    if (requested == alternate_idle) {
        if (packet.previous_animation == 1) {
            ResetCharacterIdle(object, 1, alternate_idle);
        } else {
            object->idle_total_time += FRAMETIME;
            object->idle_animation_time += FRAMETIME;
            if (object->idle_animation_time >= object->idle_animation_limit) {
                if (object->idle_animation == alternate_idle) {
                    NewCharacterIdle(object, alternate_idle);
                } else {
                    ResetCharacterIdle(object, 1, alternate_idle);
                }
            }
        }
        packet.requested_animation = object->idle_animation;
        return;
    }

    const i32 default_idle = GetDefaultIdle(object);
    if (default_idle != -1) {
        ResetCharacterIdle(object, 1, default_idle);
    }
}

void UpdateCharacterIDs() {
    const u16 count = ShopCollection.count_y;
    if (count) {
        COLLECTID *collect = ShopCollection.list;
        shopitem_s *item = CharItems;
        for (i32 i = 0; i < count; ++i, ++item, ++collect) {
            item->item_id = collect->id;
            item->price = collect->field3_0x4;
        }
    }
}

void CharScenes_AreaDump() {
    if (CharScene_Area == NULL) {
        return;
    }
    for (i32 i = 0; i < CHARCOUNT; ++i) {
        CHARSCENE_s &entry = CharScene_Area[i];
        if (entry.scene != NULL) {
            NuGScnRemove(entry.scene);
        }
        entry.scene = NULL;
    }
}

void CharScenes_AreaLoad(APICHARACTERMODELLIST_s *list, variptr_u *buf, variptr_u buf_end) {
    if (CharScene_Area == NULL || apicharsys->loaded_model_count == 0) {
        return;
    }

    for (i32 i = 0; i < apicharsys->loaded_model_count; ++i) {
        const i32 model_id = apicharsys->models[i].model_id;
        CHARSCENE_s &entry = CharScene_Area[model_id];
        if (entry.scene != NULL || (CDataList[model_id].flags & 1) == 0) {
            continue;
        }

        i32 list_index;
        if (InModelList(list, model_id, &list_index) == 0 || list[list_index].count == 0) {
            continue;
        }

        char path[128];
        sprintf(path, "chars\\%s\\%s.gsc", CDataList[model_id].dir, CDataList[model_id].file);
        entry.scene = NuGScnRead(buf, buf_end, path);
        if (entry.scene != NULL) {
            NuSpecialFind(entry.scene, &entry.special_scene, CDataList[model_id].file, 1);
        }
    }
}

void DeactivateGameObject(GameObject_s *);

void DeactivateCharacter(char *name) {
    if (Mission_Active(NULL) != NULL || name == NULL)
        return;
    GameObject_s *object = GetNamedGameObject(WORLD->ai_sys, name);
    if (object != NULL && (FreePlay == 0 || (object->apiobj.field_0x1f4 & 0x400) != 0)) {
        DeactivateGameObject(object);
    }
}

void LoadSingleCharacter(bgprocinfo_s *) {
    APICHARACTERMODELLIST_s list[2] = {
        {static_cast<i16>(waiting_for_character), 1},
        {-1, 0},
    };

    apiloadcharactermodels_append = 1;
    apiloadcharactermodels_nopakfile = CHARPAK == 0;
    APILoadCharacterModels(list, 0, &characterbuffer_ptr, characterbuffer_end, 1);
    IconScenes_Load(list, 0, &characterbuffer_ptr, &characterbuffer_end);

    NUGSCN *icon_scene = IconScene_FindById(list[0].model_id);
    if (icon_scene != NULL && LEVELOBJECTCOUNT > 0) {
        for (i32 i = 0; i < LEVELOBJECTCOUNT; ++i) {
            if (ObjTab[i].kind != 3) {
                continue;
            }

            LEVEL_OBJECT_RUNTIME &object = WORLD->lev_objs[i];
            if (object.active != 0) {
                continue;
            }

            if (NuSpecialFind(icon_scene, &object.special, ObjTab[i].name, 1) != 0) {
                object.active = 1;
            }
        }
    }

    Customiser_SaveModelTextureIDs(CharacterCustomiser, APICharacterLoaded(list[0].model_id));
    hub_character_ready = waiting_for_character;
    waiting_for_character = -1;
    g_loadingCharacterInHub = 0;
}

void UpdateCharacterLoad() {
    const i32 menu_id = GetMenuID();
    if (menu_id == 13 || (menu_id >= 15 && menu_id <= 19)) {
        return;
    }

    if (LOADEROFF != 0 || BGLOAD == 0 || waiting_for_character != -1 || hub_character_ready != -1 ||
        bgGetProcActive() != NULL || global_dlist_manager.ndisplay_lists > 0xfd) {
        return;
    }

    const i32 required_buffer =
        (static_cast<i32>((static_cast<f32>(CHARACTERBUFFERSIZE) / 7077888.0f) * 1048576.0f) + 0x3ff) & ~0x3ff;
    if (required_buffer > static_cast<i32>(characterbuffer_end.addr - characterbuffer_ptr.addr)) {
        return;
    }

    i32 candidates[0x154] __attribute__((aligned(16)));
    i32 candidate_count = 0;
    i32 candidate = 0;
    i16 *fixed_candidate = NULL;
    i32 fixed_value = -1;

    if (id_BARMAN != -1 && APICharacterLoaded(id_BARMAN) == NULL) {
        fixed_candidate = &id_BARMAN;
        goto queue_fixed_character;
    }
    if (id_CANTINABAND != -1 && APICharacterLoaded(id_CANTINABAND) == NULL) {
        fixed_candidate = &id_CANTINABAND;
        goto queue_fixed_character;
    }
    if (id_WEIRDO1 != -1 && APICharacterLoaded(id_WEIRDO1) == NULL) {
        fixed_candidate = &id_WEIRDO1;
        goto queue_fixed_character;
    }
    if (id_WEIRDO2 != -1 && APICharacterLoaded(id_WEIRDO2) == NULL) {
        fixed_candidate = &id_WEIRDO2;
        goto queue_fixed_character;
    }
    if (id_JABBA != -1 && APICharacterLoaded(id_JABBA) == NULL) {
        fixed_candidate = &id_JABBA;
        goto queue_fixed_character;
    }
    if (id_MOSEISLEYCITIZEN != -1 && APICharacterLoaded(id_MOSEISLEYCITIZEN) == NULL) {
        fixed_candidate = &id_MOSEISLEYCITIZEN;
        goto queue_fixed_character;
    }
    if (id_CANTINAALIEN != -1 && APICharacterLoaded(id_CANTINAALIEN) == NULL) {
        fixed_candidate = &id_CANTINAALIEN;
        goto queue_fixed_character;
    }
    if (id_WOMPRAT != -1 && APICharacterLoaded(id_WOMPRAT) == NULL) {
        fixed_candidate = &id_WOMPRAT;
        goto queue_fixed_character;
    }

    for (i32 pack = 0; pack < 11; ++pack) {
        if (g_lowEndLevelBehaviour != 0 && Hub_LowEnd_IconsInsteadOfModels != 0) {
            continue;
        }
        if (Store_IsPackUnlocked(pack) != 0 || StorePack[pack].id == NULL) {
            continue;
        }

        const i32 id = *StorePack[pack].id;
        if (id != -1 && APICharacterLoaded(id) == NULL) {
            fixed_value = id;
            goto queue_fixed_value;
        }
    }

    for (i16 id = 0; id < CHARCOUNT; ++id) {
        if (id == id_DROIDEKA || (CDataList[id].model_flags & 0x04002000) != 0 || APICharacterLoaded(id) != NULL) {
            continue;
        }
        if (static_cast<i32>(CDataList[id].field5_0x14) + apicharsys->loaded_animation_count >
            apicharsys->animation_capacity) {
            continue;
        }
        if (InCollectList_Index(id, NULL, 0) == -1 || Collection_Got(id) == 0) {
            continue;
        }
        candidates[candidate_count++] = id;
    }

    if (candidate_count == 0) {
        return;
    }

    if (candidate_count != 1) {
        candidate = qrand() / ((0xffff / candidate_count) + 1);
    }
    goto queue_character;

queue_fixed_character:
    candidates[0] = *fixed_candidate;
    candidate = 0;
    goto queue_character;

queue_fixed_value:
    candidates[0] = fixed_value;
    candidate = 0;
queue_character:
    waiting_for_character = candidates[candidate];
    WaitingForCharacterTime = 0.0f;
    bgPostRequest(LoadSingleCharacter, NULL, NULL, 0);
}

void CharScenes_LevelDump(WORLDINFO_s *world) {
    CHARSCENE_s *scenes = world->minikit.character_scenes;
    if (scenes != NULL && CHARCOUNT > 0) {
        i32 i = 0;
        do {
            CHARSCENE_s *entry = &scenes[i];
            if (entry->scene != NULL) {
                NuGScnRemove(entry->scene);
                scenes = world->minikit.character_scenes;
                entry = &scenes[i];
            }
            ++i;
            entry->scene = NULL;
        } while (CHARCOUNT > i);
    }
}

void CollectAllCharacters(i32) {
}

extern VARIPTR characterbuffer_base;
extern i32 CHARACTERBUFFERSIZE;
extern i32 Area;
extern i32 last_area;

void ResetCharacterBuffer(i32 force_reset) {
    if (force_reset == 0 && Area != -1 && Area == last_area) {
        return;
    }

    characterbuffer_ptr = characterbuffer_base;
    memset(characterbuffer_base.void_ptr, 0, CHARACTERBUFFERSIZE);
    apicharsys->loaded_model_count = apicharsys->permanent_model_count;
    apicharsys->animation_load_attempts = apicharsys->area_animation_count;
    apicharsys->loaded_animation_count = apicharsys->area_animation_count;
}

void NextStatusStage(STATUSPACKET_s *);
i32 AddToCollection(i32);
void NewStatusRumbleBuzz(i32, f32, f32, i32);
void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);
extern "C" void PlaySfx(char *, nuvec_s *);
void Collection_Draw(COLLECTION_s *, f32, f32, f32, APICHARACTERMODELLIST_s *, f32, i32);
void Collection_GetPos(COLLECTION_s *, i32, f32 *, f32 *);
void DrawCharIcon(i32, f32, f32, f32, f32, i32, f32, f32, i32, nuhspecial_s *);
extern f32 ICONSIZE;
extern i32 STATUS_R, STATUS_G, STATUS_B;
f32 COLLECTION_Y_STATUS = -0.3f;
static i32 playedappearsfx;
static i32 playedmovesfx;

void CollectCharcters_Draw(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, i32 active) {
    COLLECTION_s *collection = (packet->field_0xb0 & 0x80) != 0 ? &VehicleCollection : &CharacterCollection;
    if (active == 0)
        return;
    if (stage->field_0x14 == 0) {
        const f32 offscreen = (1.0f - fabsf(COLLECTION_Y_STATUS)) + 1.0f;
        const f32 ratio =
            stage->field_0x1c != 0.0f && stage->field_0x18 != 0.0f ? stage->field_0x18 / stage->field_0x1c : 0.0f;
        Collection_Draw(collection, 0.0f,
                        NuTrigTable[(static_cast<i32>(ratio * 16384.0f) >> 1) & 0x7fff] *
                                (COLLECTION_Y_STATUS + offscreen) -
                            offscreen,
                        collection->field_10, NULL, 1.0f, 0);
        return;
    }
    const i32 id = StatusCollectList.ids[stage->field_0x14 - 1];
    if (id == -1)
        return;
    Collection_Draw(collection, 0.0f, COLLECTION_Y_STATUS, collection->field_10, NULL, 1.0f, 0);
    f32 time = stage->field_0x18;
    f32 x = 0.0f, y = 0.7f, size = 0.4f, icon_alpha = 1.0f, text_alpha;
    if (time < 0.5f) {
        if (time > 0.25f && playedappearsfx == 0) {
            PlaySfx(const_cast<char *>("Char_Icon_App"), NULL);
            playedappearsfx = 1;
            time = stage->field_0x18;
        }
        text_alpha = NuTrigTable[(static_cast<i32>((time + time) * 16384.0f) >> 1) & 0x7fff];
        icon_alpha = time + time;
    } else if (time < 2.0f) {
        text_alpha = 1.0f;
        playedappearsfx = 0;
        playedmovesfx = 0;
    } else {
        text_alpha = 1.0f - NuTrigTable[(static_cast<i32>((time - 2.0f) * 16384.0f) >> 1) & 0x7fff];
        if (playedmovesfx == 0) {
            PlaySfx(const_cast<char *>("Char_Icon_Slide"), NULL);
            playedmovesfx = 1;
        }
        const f32 blend =
            1.0f - (NuTrigTable[(static_cast<i32>((time - 2.0f) * 32768.0f + 16384.0f) >> 1) & 0x7fff] + 1.0f) * 0.5f;
        if (InCollectList_Index(id, collection->list, collection->count_y) != -1) {
            f32 target_x, target_y;
            Collection_GetPos(collection, id, &target_x, &target_y);
            f32 target_size = collection->field_10 * ICONSIZE;
            if (Game_OptionsSave != NULL && Game_OptionsSave->field11_0xb != 0)
                target_size *= 0.875f;
            x = target_x * blend + 0.0f;
            y = (target_y - 0.7f) * blend + 0.7f;
            size = (target_size - 0.4f) * blend + 0.4f;
        }
    }
    DrawCharIcon(id, x, y, 0.0f, size, 0xa7, icon_alpha, icon_alpha, 1, NULL);
    SmartTextEx(TTab[CDataList[id].name_id], 0.0f, 0.35f, 1.0f, 0.6f, 0.6f, 0.6f, 0, STATUS_R, STATUS_G, STATUS_B, 1.7f,
                1, NULL, 0, static_cast<i32>(text_alpha * 128.0f));
}

void CollectCharcters_Skip(STATUS_STAGE_s *stage, STATUSPACKET_s *packet) {
    if (stage->field_0x14 == 0)
        stage->field_0x14 = 1;
    while (StatusCollectList.ids[stage->field_0x14 - 1] != -1) {
        AddToCollection(StatusCollectList.ids[stage->field_0x14 - 1]);
        ++stage->field_0x14;
    }
    NextStatusStage(packet);
    NextStatusStage(packet);
}

void E1CharacterBonus_Init(WORLDINFO_s *) {
}

AILOCATOR_s *LocalGetRandomLocator(AILOCATOR_s **locators, i32 count, f32 clip_radius, NUVEC *position,
                                   f32 max_distance, i32 outside_camera, f32 max_delta_y, f32 min_delta_y) {
    i32 candidates[64] __attribute__((aligned(16)));
    if (count > 64) {
        count = 64;
    }
    i32 candidate_count = 0;
    for (i32 i = 0; i < count; ++i) {
        if (locators[i] == NULL) {
            continue;
        }
        bool valid = false;
        if (outside_camera != 0) {
            if (NuCameraClipTestSphere(&locators[i]->position, 0.0f, &numtx_identity) == 0) {
                valid = true;
            }
        } else {
            valid = true;
            if (clip_radius > 0.0f) {
                if (NuCameraClipTestSphere(&locators[i]->position, clip_radius, &numtx_identity) == 0) {
                    const i16 room = static_cast<i16>(NuPortalWhichRoom(WORLD->current_gscn, &locators[i]->position));
                    valid = false;
                    if (room >= 0 && WORLD->rooms_visible_ptr[room] == 0) {
                        valid = true;
                    }
                }
            }
        }
        if (valid != 0) {
            if (max_distance == 1000000000.0f) {
                candidates[candidate_count++] = i;
            } else {
                NUVEC delta __attribute__((aligned(16)));
                const f32 distance = NuVecDistSqr(position, &locators[i]->position, &delta);
                if (min_delta_y != 1000000000.0f && min_delta_y > delta.y) {
                    continue;
                }
                if (max_delta_y != 1000000000.0f && delta.y > max_delta_y) {
                    continue;
                }
                if (max_distance * max_distance > distance) {
                    candidates[candidate_count++] = i;
                }
            }
        }
    }
    if (candidate_count == 0) {
        return NULL;
    }
    const i32 selected = qrand() / (65535 / candidate_count + 1);
    AILOCATOR_s *result = locators[candidates[selected]];
    locators[candidates[selected]] = NULL;
    return result;
}

void PostAnimate_ASTROMECH(GameObject_s *) {
}

nuhspecial_s *CharScene_FindHSpecial(WORLDINFO_s *world, i32 character_id) {
    CHARSCENE_s *scene;
    if (CharScene_Area != NULL && CharScene_Area[character_id].scene != NULL) {
        scene = &CharScene_Area[character_id];
    } else {
        scene = &world->minikit.character_scenes[character_id];
        if (scene->scene == NULL) {
            scene = NULL;
        }
    }
    if (scene != NULL && NuSpecialExistsFn(&scene->special_scene) == 0) {
        scene = NULL;
    }
    return scene == NULL ? NULL : &scene->special_scene;
}

AILOCATOR_s *LocalGetNearestLocator(AILOCATOR_s **locators, i32 count, f32 clip_radius, NUVEC *position,
                                    f32 max_distance, i32 outside_camera, f32 max_delta_y, f32 min_delta_y) {
    f32 nearest_distance = max_distance == 1000000000.0f ? max_distance : max_distance * max_distance;
    i32 candidates[64];
    i32 candidate_count = 0;
    if (count > 64) {
        count = 64;
    }
    for (i32 i = 0; i < count; ++i) {
        if (locators[i] == NULL) {
            continue;
        }
        if (outside_camera != 0) {
            if (NuCameraClipTestSphere(&locators[i]->position, 0.0f, &numtx_identity) != 0) {
                continue;
            }
        } else if (clip_radius > 0.0f &&
                   NuCameraClipTestSphere(&locators[i]->position, clip_radius, &numtx_identity) == 0) {
            continue;
        }
        candidates[candidate_count++] = i;
    }
    i32 nearest = -1;
    for (i32 i = 0; i < candidate_count; ++i) {
        NUVEC delta;
        const f32 distance = NuVecDistSqr(position, &locators[candidates[i]]->position, &delta);
        if (min_delta_y != 1000000000.0f && min_delta_y > delta.y) {
            continue;
        }
        if (max_delta_y != 1000000000.0f && delta.y > max_delta_y) {
            continue;
        }
        if (nearest_distance > distance) {
            nearest_distance = distance;
            nearest = i;
        }
    }
    if (nearest == -1) {
        return NULL;
    }
    AILOCATOR_s *result = locators[candidates[nearest]];
    locators[candidates[nearest]] = NULL;
    return result;
}

void AddToCompletionPoints(u32);

i32 newCharactersCollected(STATUSPACKET_s *) {
    i32 count = 0;
    for (i16 *player_id = Area_PlayerIDList; *player_id != -1 && count < 7; ++player_id) {
        const i32 id = *player_id;
        i32 previous = 0;
        while (previous < count && StatusCollectList.ids[previous] != id) {
            ++previous;
        }
        if (previous != count) {
            continue;
        }
        if (Game_CharacterSave != NULL && (Game_CharacterSave[id] & 1) != 0) {
            continue;
        }
        const i32 index = InCollectList_Index(id, MasterCollection.list, MasterCollection.count_y);
        if (index != -1 && MasterCollection.list[index].can_buy == 0) {
            StatusCollectList.ids[count++] = *player_id;
            if (MasterCollection.list[index].field5_0x9 != 0) {
                AddToCompletionPoints(POINTS_PER_CHARACTER);
            }
        }
    }
    StatusCollectList.ids[count] = -1;
    return count;
}

void CollectCharcters_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    if (stage->field_0x14 == 0) {
        stage->field_0x1c = 1.0f;
        stage->field_0x18 += elapsed;
        if (stage->field_0x18 >= 1.0f) {
            stage->field_0x14 = 1;
            if (StatusCollectList.ids[0] == -1) {
                GameAudio_PlaySfx(0x32, NULL, 0, 0);
                NewStatusRumbleBuzz(-1, 0.6f, 0.0f, 0);
                NextStatusStage(packet);
            } else {
                stage->field_0x18 = 0.0f;
                stage->field_0x1c = 3.0f;
            }
        }
    } else {
        stage->field_0x18 += elapsed;
        if (stage->field_0x18 >= stage->field_0x1c) {
            AddToCollection(StatusCollectList.ids[stage->field_0x14 - 1]);
            PlaySfx(const_cast<char *>("LegoClicks"), NULL);
            ++stage->field_0x14;
            stage->field_0x18 = 0.0f;
            stage->field_0x1c = 3.0f;
            if (StatusCollectList.ids[stage->field_0x14 - 1] == -1)
                NextStatusStage(packet);
        }
    }
}

void RegisterGizmoTypes_Indy(variptr_u *, variptr_u *) {
}

i32 SetProtocolDroidFallAnim(GameObject_s *object) {
    switch (object->field_0xe38) {
        case 1:
            return 76;
        case 2:
            return 75;
        case 3:
            return 40;
        default:
            return 5;
    }
}

void CollectCharactersOff_Draw(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, i32 active) {
    COLLECTION_s *collection = (packet->field_0xb0 & 0x80) != 0 ? &VehicleCollection : &CharacterCollection;
    if (active == 0)
        return;
    f32 alpha = 1.0f;
    if (stage->field_0x14 > 0 && stage->field_0x1c != 0.0f && stage->field_0x18 != 0.0f)
        alpha = 1.0f - stage->field_0x18 / stage->field_0x1c;
    Collection_Draw(collection, 0.0f, COLLECTION_Y_STATUS, collection->field_10, NULL, alpha, 0);
}

void CollectCharactersOff_Skip(STATUS_STAGE_s *, STATUSPACKET_s *) {
}

void ScaleGameObject(GameObject_s *obj);

void SetGameObjectCharacterData(GameObject_s *obj) {
    CHARACTERDATA *data = obj->apiobj.character_data;
    obj->apiobj.field_0xa8 = data->field17_0x3c;
    obj->field_0x1008 = data->field14_0x30;
    obj->field_0xffc = data->field15_0x34;
    obj->field_0x1000 = data->field16_0x38;
    ScaleGameObject(obj);
}

void CollectCharactersOff_Update(STATUS_STAGE_s *stage, STATUSPACKET_s *packet, float elapsed) {
    if (stage->field_0x14 == 0) {
        stage->field_0x18 = 0.0f;
        stage->field_0x1c = 1.0f;
        stage->field_0x14 = 1;
    } else if (stage->field_0x14 == 1) {
        stage->field_0x18 += elapsed;
        if (stage->field_0x18 >= stage->field_0x1c)
            NextStatusStage(packet);
    }
}

extern i32 dagobah_training;

i32 TakeOverYodaSeekDistanceHack(GameObject_s *object, GameObject_s *luke, nuvec_s *offset) {
    if ((object->field_0xf00 & 2) == 0 || dagobah_training == 0 || object->field_0xcc0 != NULL || luke == NULL ||
        (luke->apiobj.flags_low & 0x80) == 0)
        return 0;
    if (!(NuVecDistSqr(&object->apiobj.position, &luke->apiobj.position, offset) < 0.7f * 0.7f))
        return 0;
    NuVecRotateY(offset, offset, -static_cast<i32>(luke->apiobj.field_0x276));
    return offset->z > 0.0f;
}

void SetProtocolDroidInterfaceAction(GameObject_s *object) {
    if (object->field_0xe38 == 3)
        object->context_animation = 0x45;
    else if (object->field_0xe38 == 2)
        object->context_animation = 0x46;
    else if (object->field_0xe38 == 1)
        object->context_animation = 0x47;
}

void SetProtocolDroidDeactivatedAction(GameObject_s *object) {
    if (object->field_0xe38 == 3)
        object->context_animation = 0x42;
    else if (object->field_0xe38 == 2)
        object->context_animation = 0x43;
    else if (object->field_0xe38 == 1)
        object->context_animation = 0x44;
}

void LoadPerm1() {
    char buf[0x100];

    rtlInitDynamic(&permbuffer_ptr, superbuffer_end, 0x40);
    permbuffer_ptr.addr = (permbuffer_ptr.addr + 3) & ~3u;
    DebrisSetup(&permbuffer_ptr, permbuffer_end, (char *)"stuff\\particle", 0x100, 0x200, 0x200);
    DebrisRegisterCutoffCameraVec(reinterpret_cast<NUVEC *>(&global_camera.mtx.m30));
    if (Grass_Available != 0) {
        edgraSetup(&permbuffer_ptr, permbuffer_end, 0x200, 0x20, 0x40);
    }
    InitParts(0x40, &permbuffer_ptr, permbuffer_end);
    ParticleReset();

    NuStrCpy(buf, (char *)"stuff\\general.ptl");
    if (NuFileExists(buf) != 0) {
        DEBPAGE_GENERAL = edppLoadPage(buf, 0, 0);
    }
    NuStrCpy(buf, (char *)"stuff\\char.ptl");
    if (NuFileExists(buf) != 0) {
        DEBPAGE_CHARACTER = edppLoadPage(buf, 5, 0);
    }
    perm_debrissys = InitGameDebris(&permbuffer_ptr, permbuffer_end, 0x190, 0x93, debris_name, 0);

    permbuffer_ptr.addr = (permbuffer_ptr.addr + 3) & ~3u;
    InitStreaks(&permbuffer_ptr, permbuffer_end, (char *)"stuff\\streak.pnt");
    permbuffer_ptr.addr = (permbuffer_ptr.addr + 3) & ~3u;
    InitRopeMtl((char *)"rope", &permbuffer_ptr, &permbuffer_end);
    InitRipples(&ripples, &permbuffer_ptr, &permbuffer_end, 0x40);
    permbuffer_ptr.addr = (permbuffer_ptr.addr + 0xff) & ~0xffu;

    ShadowMat = NuMtlCreate3D(1);
    ShadowMat->diffuse_color.r = 1.0f;
    ShadowMat->diffuse_color.g = 1.0f;
    ShadowMat->diffuse_color.b = 1.0f;
    ShadowMat->sort_pri = 0xff;
    ShadowMat->opacity = 0.999f;
    u8 *attributes = reinterpret_cast<u8 *>(&ShadowMat->attribs);
    attributes[1] = (attributes[1] & 0x0f) | 0x60;
    attributes[0] = (attributes[0] & 0xf0) | 1;
    attributes[2] = (attributes[2] & 0x8c) | 0x12;
    ShadowMat->tex_id = static_cast<i16>(NuTexRead((char *)"stuff\\gradient", &permbuffer_ptr, &permbuffer_end));
    NuMtlUpdate(ShadowMat);

    u8 shadow_random[0x800];
    for (i32 i = 0; i < 0x800; ++i) {
        shadow_random[i] = static_cast<u8>(qrand() >> 8);
    }
    NuRndrShadowInit(shadow_random);
    CreateFadeMaterials();
    CreateUsefulMaterials();

    edgraClumpsReset();
    edanimParamReset();
    NuTexAnimProgSysInit();
    InitTexAnimScripts(TexAnimList_LSW);

    permbuffer_ptr.addr = (permbuffer_ptr.addr + 3) & ~3u;
    things_scene = NuGScnRead(&permbuffer_ptr, permbuffer_end, (char *)"stuff\\things.gsc");
    NUGSCN *terrain_scene = NULL;
    if (things_scene != NULL) {
        edbitsRegisterThingsScene(things_scene);
        terrain_scene = things_scene;
        if (things_scene->display_list != NULL) {
            things_scene->display_list->flags |= NU_DISPLAYSCENE_FLAG_NEEDS_BUILD;
        }
    }

    things_scene_terrain = TerrainInitEx(-1, &permbuffer_ptr, permbuffer_end.void_ptr, 0, (char *)"stuff\\things",
                                         terrain_scene, 0, 0x14, 0x14, 0x14);
    terrainpickupinit((char *)"stuff\\things", &things_scene_terrain);
    BackDrop_Init((char *)"stuff\\starfield.gsc", &permbuffer_ptr, &permbuffer_end);

    NuMtlSetCurrentRenderPlane(0xf);
    edpartSetParticlePage(DEBPAGE_GENERAL);
    NuStrCpy(buf, (char *)"stuff\\general.par");
    if (NuFileExists(buf) != 0) {
        PARTPAGE_GENERAL = edpartLoadPage(buf, 0, things_scene);
    }
    NuMtlSetCurrentRenderPlane(0);
    AIScriptLoadAll((char *)"scripts", &permbuffer_ptr, &permbuffer_end, NULL);
}

void LoadPerm2() {
    extern i16 tALL;
    extern i16 tJEDI;
    extern i16 tBLASTER;
    extern i16 tBOUNTYHUNTERCHARACTERS;

    CharConfig_ConfigureAll(1, ConfigChar_GameKeywords);
    ExtraCharacterFixUpAfterConfig();
    apiloadcharactermodels_nopakfile = CHARPAK == 0;
    APILoadCharacterModels(PermModelList, 1, &permbuffer_ptr, permbuffer_end, 1);

    Collection_CreateMaster(const_cast<char *>("All"), NULL, &MasterCollection, 15, COLLECTION_DEFAULTSCALE);
    Collection_CreateCustom(const_cast<char *>("Characters"), &tALL, &CharacterCollection, 0, 0x04002000, 0, 0, 16,
                            &permbuffer_ptr, &permbuffer_end, 0, 0.725f);
    Collection_CreateCustom(const_cast<char *>("Vehicles"), NULL, &VehicleCollection, 0x2000, 0x04000000, 0, 0, 7,
                            &permbuffer_ptr, &permbuffer_end, 0, 1.0f);
    Collection_CreateCustom(const_cast<char *>("Minikits"), NULL, &MiniKitCollection, 0x04000000, 0, 0, 0, 12,
                            &permbuffer_ptr, &permbuffer_end, 0, COLLECTION_DEFAULTSCALE);
    Collection_CreateCustom(const_cast<char *>("Shop"), NULL, &ShopCollection, 0, 0, 0, 1, 10, &permbuffer_ptr,
                            &permbuffer_end, 0, COLLECTION_DEFAULTSCALE);
    Collection_CreateCustom(const_cast<char *>("Jedi"), &tJEDI, &JediCollection, 8, 0, 0, 0, 7, &permbuffer_ptr,
                            &permbuffer_end, 0, COLLECTION_DEFAULTSCALE);
    Collection_CreateCustom(const_cast<char *>("BlasterZipUps"), &tBLASTER, &BlasterCollection, 0x100080, 0x01000000, 0,
                            0, 11, &permbuffer_ptr, &permbuffer_end, 0, COLLECTION_DEFAULTSCALE);
    Collection_CreateCustom(const_cast<char *>("BountyHunters"), &tBOUNTYHUNTERCHARACTERS, &BountyHunterCollection,
                            0x01000000, 0, 0, 0, 4, &permbuffer_ptr, &permbuffer_end, 0, COLLECTION_DEFAULTSCALE);
    Areas_ConfigureResidents(&permbuffer_ptr, &permbuffer_end);
}

void MapToGrid(nuvec_s *, nuvec_s *, i32 *, i32 *, nuvec_s *, nutexmanager_s *) {
}
