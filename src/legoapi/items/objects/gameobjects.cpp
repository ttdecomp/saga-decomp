#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "legoapi/audio/audio.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "decomp.h"
#include "gameapi/ai/aisys/aisys.h"
#include "gameapi/edtools/edfile.h"
#include "gameapi/gui/apimenu.h"
#include "globals.h"
#include "gamelib/util/gamelib_util_types.h"
#include "legoapi/world/mission.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/core/config/cheat.h"
#include "legoapi/render/fx.h"
#include "legoapi/render/fx/parts.h"
#include "legoapi/render/fx/spline_position.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/numath/nuvec.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/world/level.h"
#include "legoapi/world/mission.h"
#include "legoapi/gizmos/object/technos.h"
#include "legoapi/world/world.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/motion/gameanim.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/characters/core/CharacterObjectInterface.h"
#include "legoapi/characters/core/charconfig.h"
#include "legoapi/menus/core/gamehint.h"
#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/gizmos/trigger/gizspecial.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmos/door/spinner.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/props/doors/door.h"
#include "legoapi/props/system/socksys.h"
#include "legoapi/world/area.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/core/input/timer.h"
#include "legoapi/gizmo/base/gizactions.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numusic/sfx.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nu3d/nuportal.h"
#include "nu2api/nu3d/nucamera.h"
#include "legoapi/audio/sfx.h"
#include "nu2api/nuandroid/ios_graphics.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nulgtlaser.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nugcutscene.h"

#include <stdio.h>
#include <string.h>

void SetObjAsHeadTarget(GameObject_s *, GameObject_s *, i8, f32, f32, f32);
void SetBallooningHeight(GameObject_s *, f32);
void GameObjectSetCanUse(GameObject_s *, void *, u8, u8, f32);
i32 Suit_GetIndex(SUIT_s *);
void GameObjectOrigin(GameObject_s *);
BOLTTYPE_s *BoltType_FindByID(i32, WORLDINFO_s *);
void Bolt_Shoot(GameObject_s *, i32, i32);
void Torpedo_Shoot(GameObject_s *);
void GameAudio_PlaySfxById(i32, NUVEC *, i32, i32);
void PartUpdate_Basketball(PART_s *);
void PartImpact_Basketball(PART_s *);
void AddSlamDebris(GameObject_s *);
void Batarang_Release(GameObject_s *, i32);
void SuperCarry_Throw(GameObject_s *, i32);
void ThermalDetonator_Throw(GameObject_s *);
void BobaRocket_Kill(PART_s *, i32);
void BobaRocket_Move(PART_s *, f32);
void BobaRocket_Deflect(PART_s *);
void PartCollide_3D(PART_s *);
extern f32 rocket_speed;
extern f32 sabrerubwait;
NUVEC *GetZapOrigin(GameObject_s *);
void PowerUp_Particles(WORLDINFO_s *, NUVEC *);
void PlaySabreSfx(char *, GameObject_s *, NUVEC *, i32);
i32 testlaser_type;
i32 lightning_type;
f32 testlaser_endw = 0.1f;
f32 testlaser_sizewab = 0.01f;
f32 testlaser_sizel = 0.1f;
f32 testlaser_sizew = 0.02f;
extern i16 id_YODA, id_YODAGHOST, id_GAMORREANGUARD, id_JANGOFETT;
extern i32 LEGO_AIPATHCNX_WALLSHUFFLE;
extern i32 VehicleArea;
extern i32 TERRAINMASK_NONWEAPON, TERRAINMASK_NONDROID;
f32 DEFENDTIME = 4.0f;
extern f32 jump_stuck_time;
i32 teleport_all_freeplay_modes = 1;
i32 drop_in_teleport = 3;

static f32 Condition_IAmAPartyCharacter(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && packet->owner->apiobj.field_0x27c != -1) {
        return 1.0f;
    }
    return 0.0f;
}

// NuCore profiling timebars (nucore_plain.cpp): NuTimeBarCreateSet returns a
// deferred-subsystem stub handle; the slot functions are no-op stubs.
extern "C" {
    void *NuTimeBarCreateSet(i32);
    void _NuTimeBarSlotBegin(void *, i32, char const *);
    u32 _NuTimeBarSlotEnd(void *, i32);
    void AddToAIGroup(AIGROUP_s *group, APIOBJECT_s *object);
    extern NUVEC plr_lastpos;
    extern i16 id_BAT;
    extern i16 id_SNAKE;
    extern i16 id_GRIEVOUS;
    extern i16 id_BODYGUARD;
    extern i16 id_IMPERIALGUARD;
    extern i16 id_JAWA;
    extern i16 id_UGNAUGHT;
    extern i16 id_ATAT;
    extern i16 id_GONKDROID;
    extern i16 id_BASKETCANNON;
    extern i16 id_R2Q5;
    extern i16 id_LANDSPEEDER, id_MILLENNIUMFALCON;
    extern i32 FalconDebKey[2];
    void AddVariableShotDebrisEffectTimed1(i32, NUVEC *, i32, f32, i16, i16, NUMTX *);
    void AddDebrisEffect(i32 *, i32, f32, f32, f32);
    void DebrisPosOrientationMtx(i32, NUMTX *);
    void DebFreeInstantly(i32 *);
    void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);
    f32 NewRayCastGetTOFI();
}

// Written by ThingManager's ctor (original global @0x124f2e0, .bss).
extern void *theThingManager;
extern void ReleaseTakeOver(GameObject_s *object, i32 immediate);
extern void oneAtOnce_MaintainArray();

void legoSetMusicVolume(float);
void MovePlayer(GameObject_s *object);
void AnimatePlayer(GameObject_s *object);
void TerrainPlayer(GameObject_s *object);
void KeepOnScreen(GameObject_s *object);
void SetPlayer();
void InitPlayerAI(GameObject_s *object);
void ResetPlayerMoves(GameObject_s *object);
void SnapCreaturePos(GameObject_s *object, NUVEC *position, i32 angle, AIPATHINFO_s *path_info, i32 set_on_surface);
void MovePlayerSpline(GameObject_s *object);
void GetTopBot(GameObject_s *object);
void ResetRumble(RUMBLEPACKET *packet);
void ResetLights(NUVEC *position, rtldata_s *data, void *set);
void LightGameObject(GameObject_s *object, void *set);
void InitSurfaceInfo(GameObject_s *object);
i32 TightRope_SnapTo(GameObject_s *object, NUVEC *position);
void Player_ClearContext(GameObject_s *object, i32 mode);
void Player_ResetContexts(PLAYERPACKET_s *packet);
i32 SetObjOnSurface(GameObject_s *object, i32 mode);
void PortalGameObject(GameObject_s *object, i32 enable, i32 immediate, i16 portal, nugscn_s *scene);
i32 Arcade_GetMode(u32 *mode);
void StarWars_GameAISysInit();
void GameAISysSetGame();
void ClearAICreatures();
void CollideGameObjects(WORLDINFO_s *world);
void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode);
i32 TagCode(GameObject_s *source, GameObject_s *target, i32 takeover, i32 blend, i32 mode);
extern i32 do_player_tag;
extern f32 player_tag_timer;
extern GameObject_s *player_tag_from;
extern GameObject_s *player_tag_to;
APIOBJECT *GameAPIOBJECTFromObjID(u8 object_id);
i32 EquivalentObject_Find(WORLDINFO_s *world, nuhspecial_s *special);
void AIPathCnxControlSysReset(AIPATHCNXCONTROLSYS_s *system);
void AIPathCnxHelperSysReset(WORLDINFO_s *world, AIPATHCNXHELPERSYS_s *system);
void InitAICreatures(AISYS_s *system);
void ResetAICreatures(AISYS_s *system);
void LevelScriptReStoreProgress(WORLDINFO_s *world, LEVELSCRIPTPROCESS_s *process);
void GizmoSysAddGizmos(GIZMOSYS_s *gizmo_sys, GIZFLOW_s *giz_flow, void *world);
void UpdateCoinPacket(COINPACKET_s *packet, i32 active, i32 player_index);
void ResetCoinPacket(COINPACKET_s *packet);
GIZMOPICKUP_s *GizmoPickups_Collide(WORLDINFO_s *world, GameObject_s *object, i32 arg);

f32 Condition_InHubArea(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *);
void *Condition_InHubAreaInit(AISYS_s *, char *, AISCRIPT_s *);

extern i32 LEGO_AIPATHCNX_BLOCKAGE;

extern "C" i32 AISysSetLevelPath(AISYS_s *system, char *path_name);

extern "C" void NuLightFogX(f32 start, f32 end, u32 colour, f32 unused_start, f32 unused_end, i32 high_quality,
                            f32 density);

GAMEFOG_STATE GameFog = {};

static i32 GameFogSnap;
static i32 GameFogSet;
static f32 GameFogDuration;
static f32 GameFogTime;

enum AI_ACTION_SPEED_MODE : u8 {
    AI_ACTION_SPEED_RUN = 0,
    AI_ACTION_SPEED_WALK = 1,
    AI_ACTION_SPEED_TIPTOE = 2,
};

enum SCRIPT_ERROR_LEVEL : u32 {
    SCRIPT_ERROR_LEVEL_NONE = 0,
    SCRIPT_ERROR_LEVEL_WARNING = 1,
    SCRIPT_ERROR_LEVEL_STRICT = 2,
    SCRIPT_ERROR_LEVEL_COUNT = 3,
};

enum AI_DEFAULTS : u8 {
    AI_DEFAULT_ACTIVATE_DIFFICULTY = 1,
};

enum AI_OBJECT_ROUTE_STATE : u8 {
    AI_OBJECT_ROUTE_STATE_SCRIPT_VISIBLE = 3,
};

enum CHARACTER_AI_MODEL_FLAGS : u32 {
    CHARACTER_AI_MODEL_FLAG_SNAP_ON_BIG_JUMP = 0x00200000,
    CHARACTER_AI_MODEL_FLAG_DISABLE_RESPAWN = 0x00400000,
};

enum AI_GAME_OBJECT_TYPE : u8 {
    AI_GAME_OBJECT_TYPE_VEHICLE = 0x2b,
};

enum GAME_OBJECT_AI_UPDATE_FLAGS : u8 {
    GAME_OBJECT_AI_UPDATE_PROCESS = 0x08,
    GAME_OBJECT_AI_UPDATE_FORCED = 0x10,
    GAME_OBJECT_AI_UPDATE_SPECIAL_STATE = 0x20,
};

// Enabled in the target data image.  Ordinary background characters are
// staggered across frames; special movement states opt back into full-rate
// processing through the forced-update path.
i32 timebase_updates = 1;

static i32 GameObjectAIUpdateInterval(WORLDINFO_s *world, GameObject_s *object) {
    // The target updates characters outside the visible portal set every ten
    // frames.  This avoids doing a full script, controller and terrain pass
    // for every off-screen Cantina inhabitant on the same frame.
    const i16 room = object->room_id;
    if (world->rooms_visible_ptr[room] == 0 || object->apiobj.model_draw_result == 0) {
        return 10;
    }
    if (object->apiobj.field_0x27d == 0)
        return 1;

    const GAMECHARACTERDATA *character = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    const f32 distance = object->ai_update_distance;
    i32 interval = character->ai_update_interval_0;
    if (interval == 0)
        return 1;
    if (!(distance > character->ai_update_distance_0)) {
        interval = character->ai_update_interval_1;
        if (interval == 0)
            return 1;
        if (!(distance > character->ai_update_distance_1)) {
            interval = character->ai_update_interval_2;
            if (interval == 0)
                return 1;
            if (!(distance > character->ai_update_distance_2)) {
                interval = character->ai_update_interval_3;
                if (interval == 0 || !(distance > character->ai_update_distance_3)) {
                    return 1;
                }
            }
        }
    }

    // Zero requests an immediate update at every tier. Nonzero cadence
    // values are selected only beyond that tier's distance threshold.
    return interval < 7 ? interval : 7;
}

static const f32 AI_RESPAWN_DELAY = 2.0f;

extern TERRSET *CurTerr;
extern "C" i32 FindPlatInst(i32 instance);

static f32 Condition_OnForcePlatform(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    if (force != NULL && packet != NULL && packet->owner != NULL) {
        GameObject_s *object = packet->owner->apiobj.objptr;
        if ((object->apiobj.field_0x27d != 0 || object->apiobj.field_0x27e != 0) &&
            object->apiobj.supporting_platform_id != -1) {
            i16 platform = object->apiobj.supporting_platform_id;
            NUMTX *transform = static_cast<NUMTX *>(CurTerr->platforms[platform].scene_object);
            if (object->apiobj.position.y >= transform->m31) {
                for (GAMEANIMOBJ_s *animation = force->anim_set->objects; animation != NULL;
                     animation = animation->next) {
                    GIZFORCEANIMDATA_s *data = static_cast<GIZFORCEANIMDATA_s *>(animation->object_data);
                    if (platform == data->platform_id)
                        return 1.0f;
                }
            }
        }
    }
    return 0.0f;
}

static f32 Condition_PlayerOnForcePlatform(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    if (force != NULL && player != NULL) {
        GameObject_s *object = player;
        if ((object->apiobj.field_0x27d != 0 || object->apiobj.field_0x27e != 0) &&
            object->apiobj.supporting_platform_id != -1) {
            i16 platform = object->apiobj.supporting_platform_id;
            NUMTX *transform = static_cast<NUMTX *>(CurTerr->platforms[platform].scene_object);
            if (object->apiobj.position.y >= transform->m31) {
                for (GAMEANIMOBJ_s *animation = force->anim_set->objects; animation != NULL;
                     animation = animation->next) {
                    GIZFORCEANIMDATA_s *data = static_cast<GIZFORCEANIMDATA_s *>(animation->object_data);
                    if (platform == data->platform_id)
                        return 1.0f;
                }
            }
        }
    }
    return 0.0f;
}

static f32 Condition_EitherPlayerOnForcePlatform(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    if (force != NULL && player != NULL) {
        i32 players = 0;
        if ((player->apiobj.field_0x27d != 0 || player->apiobj.field_0x27e != 0) &&
            player->apiobj.supporting_platform_id != -1) {
            NUMTX *transform =
                static_cast<NUMTX *>(CurTerr->platforms[player->apiobj.supporting_platform_id].scene_object);
            if (player->apiobj.position.y >= transform->m31)
                players |= 1;
        }
        if (player2 != NULL && (player2->apiobj.field_0x27d != 0 || player2->apiobj.field_0x27e != 0) &&
            player2->apiobj.supporting_platform_id != -1) {
            NUMTX *transform =
                static_cast<NUMTX *>(CurTerr->platforms[player2->apiobj.supporting_platform_id].scene_object);
            if (player2->apiobj.position.y >= transform->m31)
                players |= 2;
        }
        if (players != 0) {
            for (GAMEANIMOBJ_s *animation = force->anim_set->objects; animation != NULL; animation = animation->next) {
                GIZFORCEANIMDATA_s *data = static_cast<GIZFORCEANIMDATA_s *>(animation->object_data);
                if ((players & 1) && player->apiobj.supporting_platform_id == data->platform_id)
                    return 1.0f;
                if ((players & 2) && player2->apiobj.supporting_platform_id == data->platform_id)
                    return 1.0f;
            }
        }
    }
    return 0.0f;
}

static void *Condition_OnForcePlatformInit(AISYS_s *, char *name, AISCRIPT_s *) {
    GIZMO_s *gizmo = GizmoFindByName(WORLD->gizmo_sys, force_gizmotype_id, name);
    if (gizmo != NULL) {
        GIZFORCE_s *force = static_cast<GIZFORCE_s *>(gizmo->object);
        if (force != NULL && (force->runtime_flags & 1) != 0)
            return force;
    }
    return NULL;
}

static void *Condition_ForcePushingInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && GetNamedAPIObjectFn != NULL ? GetNamedAPIObjectFn(system, name) : NULL;
}

static f32 Condition_ForcePushing(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GameObject_s *object = static_cast<GameObject_s *>(argument);
    if (object == NULL && packet != NULL) {
        if (packet->owner == NULL)
            return 0.0f;
        object = packet->owner->apiobj.objptr;
        if (object == NULL)
            return 0.0f;
    }
    f32 result = 0.0f;
    if (object != NULL) {
        result = object->character_context == 0x1b ? 1.0f : 0.0f;
    }
    return result;
}

static f32 Condition_EitherPlayerUsingForce(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    return GizForce_GameObjUsingForce(player, force) != 0 || GizForce_GameObjUsingForce(player2, force) != 0 ? 1.0f
                                                                                                             : 0.0f;
}

static f32 Condition_PlayerUsingForce(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return GizForce_GameObjUsingForce(player, static_cast<GIZFORCE_s *>(argument)) != 0 ? 1.0f : 0.0f;
}

static f32 Condition_UsingForce(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    return GizForce_GameObjUsingForce(packet->owner->apiobj.objptr, static_cast<GIZFORCE_s *>(argument)) != 0 ? 1.0f
                                                                                                              : 0.0f;
}

static void *Condition_UsingForceInit(AISYS_s *, char *name, AISCRIPT_s *) {
    GIZMO_s *gizmo = GizmoFindByName(WORLD->gizmo_sys, force_gizmotype_id, name);
    return gizmo != NULL ? gizmo->object : NULL;
}

static f32 Condition_ForceBeingUsed(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    if (force == NULL) {
        return 0.0f;
    }
    if ((force->field_0xaa & 0x20) == 0 && force->field_0x3c_bits == 0) {
        return 0.0f;
    }
    return 1.0f;
}

static void *Condition_ForceInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return GizmoFindByName(WORLD->gizmo_sys, force_gizmotype_id, name);
}

static f32 Condition_ForceAtStart(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && GizmoGetOutput(WORLD->gizmo_sys, static_cast<GIZMO_s *>(argument), 1, 1) == 0 ? 1.0f
                                                                                                             : 0.0f;
}

static f32 Condition_ForceAtEnd(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && GizmoGetOutput(WORLD->gizmo_sys, static_cast<GIZMO_s *>(argument), 0, 1) != 0 ? 1.0f
                                                                                                             : 0.0f;
}

// The original executable returns zero unconditionally for this condition.
static f32 Condition_NumForceObjects(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return 0.0f;
}

static void *Condition_NumForceObjectsInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    i32 flags = 0;
    if (name != NULL && system != NULL) {
        flags = NuStrIStr(name, "throwable") != NULL;
        if (NuStrIStr(name, "inrange") != NULL)
            flags |= 2;
    }
    return reinterpret_cast<void *>(static_cast<intptr_t>(flags));
}

static f32 Condition_ForceStackComplete(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    return force != NULL && force->group != NULL && (force->group->field_0x24 & 2) ? 1.0f : 0.0f;
}

static f32 Condition_ForceStackCompleteInOrder(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    return force != NULL && force->group != NULL && (force->group->field_0x24 & 4) ? 1.0f : 0.0f;
}

static f32 Condition_ForceComplete(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    return force != NULL && GizForce_Complete(force) != 0 ? 1.0f : 0.0f;
}

static f32 Condition_ForceFinished(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZFORCE_s *force = static_cast<GIZFORCE_s *>(argument);
    return force != NULL && GizForce_AnimComplete(force) != 0 ? 1.0f : 0.0f;
}

static void *Condition_ForceCompleteInit(AISYS_s *, char *name, AISCRIPT_s *) {
    GIZMO *gizmo = GizmoFindByName(WORLD->gizmo_sys, force_gizmotype_id, name);
    return gizmo != NULL ? gizmo->object : NULL;
}

static f32 Condition_ObstacleOpenedByPlayer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(argument);
    if (obstacle != NULL) {
        return player != NULL && obstacle->triggering_object == player ? 1.0f : 0.0f;
    }
    return 0.0f;
}

static f32 Condition_ObstacleOpenedByEitherPlayer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *,
                                                  void *argument) {
    GIZOBSTACLE_s *obstacle = static_cast<GIZOBSTACLE_s *>(argument);
    return obstacle != NULL && ((player != NULL && obstacle->triggering_object == player) ||
                                (player2 != NULL && obstacle->triggering_object == player2))
               ? 1.0f
               : 0.0f;
}

static void *Condition_ObstacleOpenedByPlayerInit(AISYS_s *, char *name, AISCRIPT_s *) {
    GIZMO_s *gizmo = GizmoFindByName(WORLD->gizmo_sys, obstacle_gizmotype_id, name);
    return gizmo != NULL ? gizmo->object : NULL;
}

static f32 Condition_ObstacleLockedOpen(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return (static_cast<GIZOBSTACLE_s *>(argument)->runtime_flags & 4) != 0 ? 1.0f : 0.0f;
}

static f32 Condition_ObstacleLockedShut(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return (static_cast<GIZOBSTACLE_s *>(argument)->runtime_flags & 8) != 0 ? 1.0f : 0.0f;
}

static void *Condition_GizSpecialInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return GizmoFindByName(WORLD->gizmo_sys, gizspecial_gizmotype_id, name);
}

static void *Condition_ObstacleInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return GizmoFindByName(WORLD->gizmo_sys, obstacle_gizmotype_id, name);
}

static f32 Condition_ObstacleAtStart(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && GizmoGetOutput(WORLD->gizmo_sys, static_cast<GIZMO_s *>(argument), 1, 1) == 0 ? 1.0f
                                                                                                             : 0.0f;
}

static f32 Condition_ObstacleAtEnd(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && GizmoGetOutput(WORLD->gizmo_sys, static_cast<GIZMO_s *>(argument), 0, 1) != 0 ? 1.0f
                                                                                                             : 0.0f;
}

static void *Condition_OnSpeederBikeInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && system != NULL ? GetNamedGameObject(system, name) : NULL;
}

static f32 Condition_OnSpeederBike(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GameObject_s *object = static_cast<GameObject_s *>(argument);
    if (object == NULL) {
        if (packet->owner != NULL) {
            object = packet->owner->apiobj.objptr;
        }
    }
    if (object != NULL) {
        return object->field_0xcc0 != NULL && object->character_context == 0x3b &&
                       object->field_0xcc0->id == id_SPEEDERBIKE
                   ? 1.0f
                   : 0.0f;
    }
    return 0.0f;
}

static f32 Condition_Player2Active(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return player2 != NULL ? 1.0f : 0.0f;
}

static void *Condition_TakenOverInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && system != NULL ? GetNamedGameObject(system, name) : NULL;
}

static f32 Condition_TakenOver(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *name, void *argument) {
    GameObject_s *object = static_cast<GameObject_s *>(argument);
    if (object == NULL) {
        if (name != NULL) {
            if (NuStrICmp(name, "Opponent") == 0) {
                APIOBJECT_s *opponent = packet->owner->apiobj.objptr->ai.opponent_object;
                if (opponent != NULL)
                    object = opponent->objptr;
            } else if (NuStrICmp(name, "TakeoverTarget") == 0) {
                object = packet->owner->apiobj.objptr;
                if (object != NULL)
                    object = object->takeover_target;
            }
        } else if (packet != NULL && packet->owner != NULL) {
            object = packet->owner->apiobj.objptr;
        }
    }
    return object != NULL && object->field_0xcc0 != NULL && object->character_context != 0x3b ? 1.0f : 0.0f;
}

static void *Condition_LastLevelInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    if (name != NULL && system != NULL && WORLD->area != NULL) {
        for (i32 index = 0; index < LEVELCOUNT; ++index) {
            if (NuStrICmp(name, LDataList[index].name) == 0) {
                return reinterpret_cast<void *>(static_cast<isize>(index));
            }
        }
    }
    return reinterpret_cast<void *>(static_cast<isize>(-1));
}

static f32 Condition_LastLevel(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    if (LastLData != NULL) {
        return LastLData->idx == static_cast<i32>(reinterpret_cast<isize>(argument)) ? 1.0f : 0.0f;
    }
    return 0.0f;
}

static void *Condition_IsVisibleInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return name;
}

static f32 Condition_IsVisible(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    nuhspecial_s special = {};
    NuSpecialFind(WORLD->current_gscn, &special, static_cast<char *>(argument), 1);
    f32 result = 0.0f;
    if (NuSpecialExistsFn(&special) != 0) {
        result = NuSpecialGetVisibilityFn(&special);
    }
    return result;
}

// The reference executable exposes this condition as an unconditional zero.
static f32 Condition_Indy(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return 0.0f;
}

// The Android reference executable reports false for the PSP platform.
static f32 Condition_PSP(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return 0.0f;
}

// The reference executable exposes this condition as an unconditional zero.
static f32 Condition_CheatProgress(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return 0.0f;
}

static f32 Condition_ChallengeMode(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return ChallengeMode != 0 ? 1.0f : 0.0f;
}

static f32 Condition_Freeplay(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return static_cast<f32>(FreePlay);
}

static f32 Condition_MissionMode(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return Mission_Active(NULL) != NULL ? 1.0f : 0.0f;
}

static f32 Condition_MissionWon(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return MissionSys != NULL && MissionSys->field8_0x1d == 2 ? 1.0f : 0.0f;
}

static f32 Condition_NumInSetAlive(AISYS_s *, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *, void *argument) {
    i32 set = reinterpret_cast<intptr_t>(argument);
    if (set == -1)
        set = process->unknown_b0;
    f32 result = 0.0f;
    if (set != 0)
        result = static_cast<u32>(aicreature_sets_alive[set - 1]);
    return result;
}

static f32 Condition_IsSetAlive(AISYS_s *, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *, void *argument) {
    i32 set = reinterpret_cast<intptr_t>(argument);
    if (set == -1)
        set = process->unknown_b0;
    return set != 0 && aicreature_sets_alive[set - 1] != 0 ? 1.0f : 0.0f;
}

static void *Condition_IsSetAliveInit(AISYS_s *, char *arg, AISCRIPT_s *) {
    if (arg == NULL)
        return NULL;
    if (NuStrICmp(arg, "myset") == 0)
        return reinterpret_cast<void *>(static_cast<intptr_t>(-1));
    i32 set = NuAToI(arg);
    if (set < 1 || set > 16)
        set = 0;
    return reinterpret_cast<void *>(static_cast<intptr_t>(set));
}

extern "C" i32 instNuGCutSceneIsFinished(instNUGCUTSCENE_s *cutscene);

static f32 Condition_SockXDistanceToPlayer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    f32 result = 0.0f;
    NUVEC player_offset, object_offset;
    if (packet == NULL)
        return 0.0f;
    if (packet->owner != NULL && player != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        if (player->field_0x661 != 0xff && player->field_0x661 == object->field_0x661) {
            NuVecSub(&player_offset, &player->apiobj.position, &player->sock_position.midpoint);
            NuVecRotateY(&player_offset, &player_offset, -player->sock_position.midpoint_rotation.y);
            NuVecSub(&object_offset, &object->apiobj.position, &object->sock_position.midpoint);
            NuVecRotateY(&object_offset, &object_offset, -object->sock_position.midpoint_rotation.y);
            result = player_offset.x - object_offset.x;
        }
    }
    return result;
}

static f32 Condition_PlayerDeflectingPart(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return player != NULL && player->force_part != NULL ? 1.0f : 0.0f;
}

static f32 Condition_CollidingWithOpponent(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->opponent_object != NULL && packet->owner != NULL &&
        (packet->owner->apiobj.colliding_objects_mask & packet->opponent_object->collision_identity_mask) != 0) {
        return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_OpponentPathPosRange(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && packet->opponent_object != NULL &&
        packet->opponent_object->ai != NULL) {
        NUVEC difference;
        return NuVecDist(&packet->owner->apiobj.position, &packet->opponent_object->ai->last_path_position,
                         &difference);
    }
    return 1.0e9f;
}

static f32 Condition_OpponentToPlayerRange(AISYS_s *system, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && packet->opponent_object != NULL && system != NULL &&
        system->player_1 != NULL) {
        NUVEC difference;
        return NuVecDist(&system->player_1->position, &packet->opponent_object->position, &difference);
    }
    return 1.0e9f;
}

static f32 Condition_SockDistanceToPlayer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    f32 result = 0.0f;
    if (packet != NULL && packet->owner != NULL && player != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        if (player->field_0x661 != 0xff && player->field_0x661 == object->field_0x661) {
            f32 distance = MidDistanceFromSockStart(WORLD->sock_sys, &player->sock_position);
            result = distance - MidDistanceFromSockStart(WORLD->sock_sys, &object->sock_position);
        }
    }
    return result;
}

static f32 Condition_SockDistanceToOpponent(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    f32 result = 0.0f;
    if (packet != NULL && packet->owner != NULL && packet->opponent_object != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        GameObject *opponent = packet->opponent_object->objptr;
        if (opponent->field_0x661 != 0xff && opponent->field_0x661 == object->field_0x661) {
            f32 distance = MidDistanceFromSockStart(WORLD->sock_sys, &opponent->sock_position);
            result = distance - MidDistanceFromSockStart(WORLD->sock_sys, &object->sock_position);
        }
    }
    return result;
}

static f32 Condition_FurthestPlayerDistanceAlongSock(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    f32 distance = 0.0f;
    if (player != NULL) {
        if (player2 != NULL)
            distance = player2->sock_position.distance > player->sock_position.distance
                           ? player2->sock_position.distance
                           : player->sock_position.distance;
        else
            distance = player->sock_position.distance;
    }
    return distance;
}

static f32 Condition_PlayerDistanceAlongSock(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return player != NULL ? player->sock_position.distance : 0.0f;
}

static f32 Condition_PlayerInSock(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && WORLD->sock_sys != NULL &&
                   argument == &WORLD->sock_sys->sock[static_cast<i8>(player->field_0x661)]
               ? 1.0f
               : 0.0f;
}

static void *Condition_PlayerInSockInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return FindSock(WORLD->sock_sys, name);
}

extern "C" f32 NuAnimEndFrameOld(void *animation);

extern i32 Hub_GetRandomCharType();
extern u8 hub_custodians_finished_loading;
extern "C" {
    i32 nbaddies_can_see_players;
}

static f32 Condition_NumBaddiesThatCanSeePlayers(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return (f32)nbaddies_can_see_players;
}

static f32 Condition_OnSameObjectAsPlayer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && player != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        if ((object->apiobj.packed_contact_state & 0xffff00) != 0 &&
            (player->apiobj.packed_contact_state & 0xffff00) != 0 && object->apiobj.supporting_platform_id != -1 &&
            object->apiobj.supporting_platform_id == player->apiobj.supporting_platform_id)
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_RandomMapCharsAvailable(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    if (hub_custodians_finished_loading == 0)
        return 0.0f;
    i16 character = Hub_GetRandomCharType();
    return character != -1 ? 1.0f : 0.0f;
}

static f32 Condition_EitherPlayerWearingHelmet(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    // The shipped condition only checks the active player despite its name.
    return player->field_0x108e == 5 ? 1.0f : 0.0f;
}

static f32 Condition_AreaContainsPartyMember(AISYS_s *system, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *,
                                             void *argument) {
    if (system != NULL) {
        AIAREA *area = static_cast<AIAREA *>(argument);
        if (area == NULL)
            area = process->unknown_a0;
        if (area != NULL && (area->runtime_flags & 8) != 0)
            return 1.0f;
    }
    return 0.0f;
}

static void *Condition_AreaContainsPartyMemberInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL ? AISysFindArea(system, name) : NULL;
}

static f32 Condition_CharacterTypeExists(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    if (argument != NULL) {
        for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
            if (Obj[index].id == (intptr_t)argument)
                return 1.0f;
        }
    }
    return 0.0f;
}

static void *Condition_CharacterTypeExistsInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    if (name != NULL && system != NULL) {
        for (i32 index = 0; index < CHARCOUNT; ++index) {
            if (NuStrICmp(CDataList[index].file, name) == 0)
                return (void *)(intptr_t)index;
        }
    }
    return (void *)(intptr_t)-1;
}

static f32 Condition_EitherPlayerInMyTriggerArea(AISYS_s *system, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *,
                                                 void *) {
    if (system != NULL && process->unknown_a0 != NULL) {
        AIAREA *area = process->unknown_a0;
        if (system->player_1 != NULL && area->system != NULL) {
            i64 mask = 1 << (area - area->system->areas);
            u64 membership = ((u64)system->player_1->ai_area_mask_high << 32) | system->player_1->ai_area_mask_low;
            if ((membership & mask) != 0)
                return 1.0f;
        }
        if (system->player_2 != NULL && area->system != NULL) {
            i64 mask = 1 << (area - area->system->areas);
            u64 membership = ((u64)system->player_2->ai_area_mask_high << 32) | system->player_2->ai_area_mask_low;
            if ((membership & mask) != 0)
                return 1.0f;
        }
    }
    return 0.0f;
}

static f32 Condition_OnDynamicGrapple(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GameObject *object = static_cast<GameObject *>(argument);
    if (object == NULL) {
        if (packet == NULL || packet->owner == NULL)
            return 0.0f;
        object = packet->owner->apiobj.objptr;
    }
    if (object != NULL && object->character_context == 0x46) {
        GRAPPLE *grapple = static_cast<GRAPPLE *>(object->field_0x788);
        if (grapple != NULL && grapple->has_terrain_platform != 0)
            return 1.0f;
    }
    return 0.0f;
}

static void *Condition_OnDynamicGrappleInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && system != NULL ? GetNamedGameObject(system, name) : NULL;
}

static f32 Condition_AngleAboutMyLocatorToPlayer(AISYS_s *, AISCRIPTPROCESS_s *process, AIPACKET_s *packet, char *,
                                                 void *argument) {
    f32 result = 0.0f;
    if (argument != NULL && process->unknown_a4 != NULL && packet != NULL && packet->owner != NULL) {
        GameObject *object = packet->owner;
        GameObject *target = NULL;
        if ((intptr_t)argument == -1)
            target = player;
        else if ((intptr_t)argument == 1)
            target = Player[1];
        if (target != NULL) {
            i32 object_angle = NuAtan2D(object->apiobj.position.x - process->unknown_a4->position.x,
                                        object->apiobj.position.z - process->unknown_a4->position.z);
            i32 player_angle = NuAtan2D(target->apiobj.position.x - process->unknown_a4->position.x,
                                        target->apiobj.position.z - process->unknown_a4->position.z);
            result = (f32)NuAngSub(player_angle, object_angle);
        }
    }
    return result;
}

static void *Condition_AngleAboutMyLocatorToPlayerInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return (void *)(intptr_t)(name != NULL && NuStrICmp(name, "player1") == 0 ? 1 : -1);
}

static f32 Condition_EitherPlayerUsingHatMachine(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *name, void *) {
    if (player != NULL && player->character_context == 0x61) {
        HATMACHINE_s *machine = static_cast<HATMACHINE_s *>(player->field_0x788);
        if (machine != NULL && (name == NULL || NuStrICmp(machine->name, name) == 0))
            return 1.0f;
    }
    if (player2 != NULL && player2->character_context == 0x61) {
        HATMACHINE_s *machine = static_cast<HATMACHINE_s *>(player2->field_0x788);
        if (machine != NULL && (name == NULL || NuStrICmp(machine->name, name) == 0))
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_EitherPlayerPullingLever(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *name, void *) {
    if (player != NULL && player->character_context == 0x4a) {
        LEVER_s *lever = static_cast<LEVER_s *>(player->field_0x788);
        if (lever != NULL && (name == NULL || NuStrICmp(lever->name, name) == 0))
            return 1.0f;
    }
    if (player2 != NULL && player2->character_context == 0x4a) {
        LEVER_s *lever = static_cast<LEVER_s *>(player2->field_0x788);
        if (lever != NULL && (name == NULL || NuStrICmp(lever->name, name) == 0))
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_EitherPlayerUsingPanel(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *name, void *) {
    if (player != NULL && player->character_context == 0x0b) {
        GIZPANEL_s *panel = static_cast<GIZPANEL_s *>(player->field_0x788);
        if (panel != NULL && (name == NULL || NuStrICmp(panel->name, name) == 0))
            return 1.0f;
    }
    if (player2 != NULL && player2->character_context == 0x0b) {
        GIZPANEL_s *panel = static_cast<GIZPANEL_s *>(player2->field_0x788);
        if (panel != NULL && (name == NULL || NuStrICmp(panel->name, name) == 0))
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_AreaContainsBaddies(AISYS_s *system, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *,
                                         void *argument) {
    if (system != NULL) {
        AIAREA *area = static_cast<AIAREA *>(argument);
        if (area == NULL)
            area = process->unknown_a0;
        if (area != NULL && (area->runtime_flags & 4) != 0)
            return 1.0f;
    }
    return 0.0f;
}

static void *Condition_AreaContainsBaddiesInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL ? AISysFindArea(system, name) : NULL;
}

static f32 Condition_AreaContainsGoodies(AISYS_s *system, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *,
                                         void *argument) {
    if (system != NULL) {
        AIAREA *area = static_cast<AIAREA *>(argument);
        if (area == NULL)
            area = process->unknown_a0;
        if (area != NULL && (area->runtime_flags & 2) != 0)
            return 1.0f;
    }
    return 0.0f;
}

static void *Condition_AreaContainsGoodiesInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL ? AISysFindArea(system, name) : NULL;
}

static f32 Condition_PickupBeenTurnedOn(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL ? (f32)GizmoPickup_BeenTurnedOn(static_cast<GIZMOPICKUP_s *>(argument)) : 0.0f;
}

static void *Condition_PickupBeenTurnedOnInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return name != NULL ? GizmoPickup_FindByName(WORLD, name) : NULL;
}

static f32 Condition_AnimationFinished(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    nuinstanim_s *animation = static_cast<nuinstanim_s *>(argument);
    if (animation != NULL && !animation->playing) {
        nuanimdata_s *data = WORLD->current_gscn->instance_animation_data[animation->anim_ix];
        if (data != NULL && animation->ltime >= NuAnimEndFrameOld(data))
            return 1.0f;
    }
    return 0.0f;
}

static void *Condition_AnimationFinishedInit(AISYS_s *, char *name, AISCRIPT_s *) {
    nuhspecial_s special;
    NuSpecialFind(WORLD->current_gscn, &special, name, 1);
    return NuSpecialExistsFn(&special) != 0 ? NuSpecialGetInstAnim(&special) : NULL;
}

static f32 Condition_RigidAnimFrame(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    nuinstanim_s *animation = static_cast<nuinstanim_s *>(argument);
    return animation != NULL ? animation->ltime : 1.0f;
}

static void *Condition_RigidAnimFrameInit(AISYS_s *, char *name, AISCRIPT_s *) {
    nuhspecial_s special;
    NuSpecialFind(WORLD->current_gscn, &special, name, 1);
    return NuSpecialExistsFn(&special) != 0 ? NuSpecialGetInstAnim(&special) : NULL;
}

static f32 Condition_FinishedSpline(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && packet->owner->apiobj.objptr != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        return object->movement_spline != NULL && object->movement_spline_finished == 0 ? 0.0f : 1.0f;
    }
    return -1.0f;
}

static f32 Condition_CutSceneFinished(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    CUTINFO *cut = static_cast<CUTINFO *>(argument);
    return cut != NULL && cut->instance != NULL &&
                   instNuGCutSceneIsFinished(static_cast<instNUGCUTSCENE_s *>(cut->instance)) != 0
               ? 1.0f
               : 0.0f;
}

static void *Condition_CutSceneFinishedInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return CutScene_Find(WORLD->cutscene_sys, name);
}

static f32 Condition_CutSceneStarted(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    CUTINFO *cut = static_cast<CUTINFO *>(argument);
    return cut != NULL && cut->instance != NULL && (static_cast<instNUGCUTSCENE_s *>(cut->instance)->flags_88 & 2) != 0
               ? 1.0f
               : 0.0f;
}

static void *Condition_CutSceneStartedInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return CutScene_Find(WORLD->cutscene_sys, name);
}

static f32 Condition_CutSceneExists(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL ? 1.0f : 0.0f;
}

static void *Condition_CutSceneExistsInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return CutScene_Find(WORLD->cutscene_sys, name);
}

static f32 Condition_CutScenePlaying(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    // The reference target returns zero unconditionally for this condition.
    return 0.0f;
}

static void *Condition_CutScenePlayingInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return CutScene_Find(WORLD->cutscene_sys, name);
}

static f32 Condition_Message(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GIZAIMESSAGE_s *message = static_cast<GIZAIMESSAGE_s *>(argument);
    return message != NULL ? message->value : 0.0f;
}

static void *Condition_MessageInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    if (name != NULL && system != NULL && gizaimessagesys != NULL)
        return CheckGizAIMessage(gizaimessagesys, name, NULL);
    return NULL;
}

static f32 Condition_ScriptParam(AISYS_s *, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *, void *argument) {
    i32 index = reinterpret_cast<intptr_t>(argument);
    return index >= 0 ? process->params[index] : 0.0f;
}

static void *Condition_ScriptParamInit(AISYS_s *, char *name, AISCRIPT_s *script) {
    if (name != NULL) {
        for (i32 index = 0; index < 4; ++index) {
            if (NuStrICmp(script->params[index].name, name) == 0)
                return reinterpret_cast<void *>(static_cast<intptr_t>(index));
        }
        return reinterpret_cast<void *>(static_cast<intptr_t>(NuAToI(name)));
    }
    return reinterpret_cast<void *>(static_cast<intptr_t>(-1));
}

static f32 Condition_Blocking(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && packet->owner->apiobj.objptr != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        u32 flags = CInfo[object->character_context].flags;
        if ((flags & 0x04000000) != 0 || ((flags & 0x08000000) != 0 && (object->jump_flags & 2) != 0))
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_RaceLap(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return static_cast<f32>(Lap);
}

static f32 Condition_MusicOn(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return SuperOptions.music_enabled != 0 ? 1.0f : 0.0f;
}

static f32 Condition_GotOpponentLOS(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL && packet->owner->apiobj.objptr != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        APIOBJECT *opponent = object->character_context == 0x1b ? reinterpret_cast<APIOBJECT *>(object->force_target)
                                                                : packet->opponent_object;
        if (opponent != NULL &&
            ((WORLD->api_object_sys->line_of_sight[packet->owner->apiobj.field_0x289] >> opponent->field_0x289) & 1) !=
                0)
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_GotLocatorInSet(AISYS_s *system, AISCRIPTPROCESS_s *process, AIPACKET_s *, char *,
                                     void *argument) {
    AILOCATORSET *set = static_cast<AILOCATORSET *>(argument);
    if (set != NULL && process->unknown_a4 != NULL) {
        u8 locator = static_cast<u8>(process->unknown_a4 - system->locators);
        for (i32 index = 0; index < set->locator_count; ++index) {
            if (set->locator_entries[index] == locator)
                return 1.0f;
        }
    }
    return 0.0f;
}

static void *Condition_GotLocatorInSetInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return AIPathFindLocatorSet(system, name);
}

i32 CanFightLikeAJedi(GameObject_s *object) {
    return CharCategory_IsCategory(object, 0) != 0 || object->id == id_GRIEVOUS || object->id == id_BODYGUARD ||
           object->id == id_IMPERIALGUARD;
}

static f32 Condition_CanFightLikeAJedi(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    return packet != NULL && packet->owner != NULL && packet->owner->apiobj.objptr != NULL &&
                   CanFightLikeAJedi(packet->owner) != 0
               ? 1.0f
               : 0.0f;
}

static f32 Condition_EitherPlayerPushingSpinner(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    if (argument != NULL) {
        if (player != NULL && player->character_context == 0x28 && player->field_0x788 == argument)
            return 1.0f;
        if (player2 != NULL && player2->character_context == 0x28 && player2->field_0x788 == argument)
            return 1.0f;
    }
    return 0.0f;
}

static void *Condition_EitherPlayerPushingSpinnerInit(AISYS_s *, char *name, AISCRIPT_s *) {
    return GizSpinner_FindBySpecialName(WORLD, name);
}

static f32 Condition_CharacterRange(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GameObject *object = static_cast<GameObject *>(argument);
    if (packet != NULL && packet->owner != NULL && object != NULL) {
        NUVEC difference;
        return NuVecDist(&object->apiobj.position, &packet->owner->apiobj.position, &difference);
    }
    return 1000000000.0f;
}

static void *Condition_CharacterRangeInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && system != NULL ? GetNamedGameObject(system, name) : NULL;
}

static f32 Condition_MaulShouldRunAway(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet->owner != NULL) {
        i32 angle = NuAtan2D(packet->owner->apiobj.collision_position.x - 5.5f,
                             packet->owner->apiobj.collision_position.z - 3.65f);
        for (i32 index = 0; index < 2; ++index) {
            GameObject *object = Player[index];
            if (object != NULL && (object->apiobj.field_0x1f8 & 0x1001) == 0x1001) {
                f32 x = object->apiobj.collision_position.x - 5.5f;
                f32 z = object->apiobj.collision_position.z - 3.65f;
                if (x * x + z * z < 25.0f && NuAngSub(NuAtan2D(x, z), angle) <= 0xe37)
                    return 1.0f;
            }
        }
    }
    return 0.0f;
}

static f32 Condition_BeenTakenOver(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GameObject *object = static_cast<GameObject *>(argument);
    if (object == NULL && packet->owner != NULL)
        object = packet->owner->apiobj.objptr;
    return object != NULL && object->field_0xcc0 != NULL && object->character_context == 0x3b ? 1.0f : 0.0f;
}

static void *Condition_BeenTakenOverInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && system != NULL ? GetNamedGameObject(system, name) : NULL;
}

static f32 Condition_LastAttackerRange(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        GameObject *attacker = object->last_attacker;
        if (attacker != NULL) {
            NUVEC difference;
            return NuVecDist(&object->apiobj.position, &attacker->apiobj.position, &difference);
        }
    }
    return 1000000000.0f;
}

static f32 Condition_LastAttackerIsActivePlayer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    f32 result = 0.0f;
    if (packet != NULL && packet->owner != NULL) {
        GameObject *attacker = packet->owner->apiobj.objptr->last_attacker;
        if (attacker != NULL && (attacker->apiobj.flags_low & 0x80) != 0)
            result = 1.0f;
    }
    return result;
}

static f32 Condition_CannotReachDestination(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    return packet != NULL && (packet->runtime_flags & 0x40) != 0 ? 1.0f : 0.0f;
}

static f32 Condition_BeenSpawned(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    return packet != NULL && packet->owner != NULL && packet->owner->apiobj.field_0x27c == -1 &&
                   packet->field_0x134 == 0xff
               ? 1.0f
               : 0.0f;
}

static f32 Condition_ShouldAttackOpponent(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL) {
        GameObject *object = packet->owner->apiobj.objptr;
        if (object != NULL && object->ai.opponent_object != NULL) {
            GameObject *opponent = object->ai.opponent_object->objptr;
            if (opponent != NULL && (object->ai.field_0x1e5 & 8) != 0 && object->ai.opponent_metric < 1.0f) {
                if (opponent->id == id_BAT || opponent->id == id_SNAKE)
                    return 1.0f;
            }
        }
    }
    return 0.0f;
}

static f32 Condition_RespawnLocatorIs(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    return argument != NULL && packet != NULL && packet->respawn_locator == argument ? 1.0f : 0.0f;
}

static void *Condition_RespawnLocatorIsInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL ? AIPathFindLocator(system, name) : NULL;
}

static f32 Condition_BoltsDontGetDeflectedBack(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    return packet != NULL && packet->owner != NULL && (packet->owner->apiobj.objptr->field_0xefc & 8) != 0 ? 1.0f
                                                                                                           : 0.0f;
}

static f32 Condition_HelpWithTriggers(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    AITRIGGERSETSYS_s *system = WORLD->ai_trigger_set_sys;
    if (system == NULL)
        return 0.0f;
    if (packet != NULL && packet->owner != NULL) {
        u8 index = packet->owner->apiobj.field_0x289;
        if (system->field_0x42c0[index] != -1 && system->sets[system->field_0x4280[index]].field_0x20e != 0)
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_DropBackInTimer(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return drop_back_in_timer;
}

static f32 Condition_InMiniCut(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *arg, void *) {
    return MiniCutCam != 0 || (arg != NULL && ObstacleCamSpl != NULL) ? 1.0f : 0.0f;
}

static f32 Condition_BigJumpComplete(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    if (packet != NULL && packet->owner != NULL) {
        return packet->owner->apiobj.objptr->character_context != 0x1f ? 1.0f : 0.0f;
    }
    return 1.0f;
}

static f32 Condition_AIOverrideControl(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    APIOBJECT *object = static_cast<APIOBJECT *>(argument);
    if (object == NULL) {
        if (packet != NULL)
            object = reinterpret_cast<APIOBJECT *>(packet->owner);
    }
    return object != NULL && (object->flags_high & 1) != 0 ? 1.0f : 0.0f;
}

static void *Condition_AIOverrideControlInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    if (name != NULL && GetNamedAPIObjectFn != NULL)
        return GetNamedAPIObjectFn(system, name);
    return NULL;
}

static f32 Condition_IsOnScreen(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    APIOBJECT *object = static_cast<APIOBJECT *>(argument);
    if (object == NULL) {
        if (packet != NULL)
            object = reinterpret_cast<APIOBJECT *>(packet->owner);
    }
    return object != NULL && object->model_draw_result != 0 ? 1.0f : 0.0f;
}

static void *Condition_IsOnScreenInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    if (name != NULL && GetNamedAPIObjectFn != NULL)
        return GetNamedAPIObjectFn(system, name);
    return NULL;
}

static f32 Condition_IAmPlayer2(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *) {
    f32 result = 0.0f;
    if (packet != NULL && packet->owner != NULL) {
        if (player == Player[0]) {
            result = packet->owner->apiobj.objptr == Player[1] ? 1.0f : 0.0f;
        } else if (player == Player[1]) {
            result = packet->owner->apiobj.objptr == Player[0] ? 1.0f : 0.0f;
        }
    }
    return result;
}

static f32 Condition_PlayerOnObject(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    i32 platform = reinterpret_cast<intptr_t>(argument);
    f32 result = 0.0f;
    if (player != NULL && platform != -1 && (player->apiobj.field_0x27d != 0 || player->apiobj.field_0x27e != 0)) {
        if (player->apiobj.supporting_platform_id == platform) {
            NUMTX *transform = static_cast<NUMTX *>(CurTerr->platforms[platform].scene_object);
            result = player->apiobj.position.y >= transform->m31 ? 1.0f : 0.0f;
        }
    }
    return result;
}

static void *Condition_OnObjectInit(AISYS_s *, char *name, AISCRIPT_s *) {
    nuhspecial_s special;
    i32 platform = -1;
    if (CurTerr != NULL) {
        if (NuSpecialFind(WORLD->current_gscn, &special, name, 1) != 0) {
            platform = FindPlatInst(NuSpecialGetInstanceix(&special));
        }
    }
    return reinterpret_cast<void *>(static_cast<intptr_t>(platform));
}

static f32 Condition_PlayerOnGround(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return player != NULL && (player->apiobj.field_0x27d != 0 || player->apiobj.field_0x27e != 0) ? 1.0f : 0.0f;
}

static f32 Condition_UnderPlayerControl(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    GameObject_s *object = static_cast<GameObject_s *>(argument);
    f32 result = 0.0f;
    if (object != NULL) {
        if ((object->apiobj.flags_low & 0x80) != 0)
            result = 1.0f;
    }
    return result;
}

static void *Condition_UnderPlayerControlInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    if (name != NULL && system != NULL)
        return GetNamedGameObject(system, name);
    return NULL;
}

static f32 Condition_PlayerTakenOver(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return player != NULL && player->field_0xcc0 != NULL && player->character_context != 0x3b ? 1.0f : 0.0f;
}

static f32 Condition_EitherPlayerTakenOver(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    if (player != NULL && player->field_0xcc0 != NULL && player->character_context != 0x3b)
        return 1.0f;
    if (player2 != NULL && player2->field_0xcc0 != NULL && player2->character_context != 0x3b)
        return 1.0f;
    return 0.0f;
}

static f32 Condition_PartyContainsDroids(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    for (i32 index = 0; index < 8; ++index) {
        GameObject_s *object = Player[index];
        if (object != NULL && (object->apiobj.field_0x1f8 & 0x1001) == 0x1001 && (object->field_0xeff & 1) == 0 &&
            (object->apiobj.character_data->model_flags & 0x10) != 0)
            return 1.0f;
    }
    return 0.0f;
}

static f32 Condition_OpponentContext(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    f32 result = 0.0f;
    if (packet != NULL && packet->opponent_object != NULL) {
        if (packet->opponent_object->objptr->character_context == reinterpret_cast<intptr_t>(argument))
            result = 1.0f;
    }
    return result;
}

static void *Condition_InContextInit(AISYS_s *, char *name, AISCRIPT_s *) {
    i32 context;
    if (NuStrICmp(name, "DEACTIVATED") == 0)
        context = 0x17;
    else if (NuStrICmp(name, "FORCEDBACK") == 0)
        context = 0x22;
    else if (NuStrICmp(name, "GRAB") == 0)
        context = 0x38;
    else if (NuStrICmp(name, "EAT") == 0)
        context = 0x3f;
    else if (NuStrICmp(name, "FORCEPUSHED") == 0)
        context = 0x1c;
    else if (NuStrICmp(name, "FORCEPUSH") == 0)
        context = 0x1b;
    else if (NuStrICmp(name, "GETIN") == 0)
        context = 0x3c;
    else if (NuStrICmp(name, "BALLOONING") == 0)
        context = 0x5d;
    else if (NuStrICmp(name, "STUNNED") == 0)
        context = 0x5a;
    else if (NuStrICmp(name, "FLOAT") == 0)
        context = 0x4b;
    else if (NuStrICmp(name, "GRAPPLE") == 0)
        context = 0x46;
    else
        context = 0x64;
    return reinterpret_cast<void *>(static_cast<isize>(context));
}

static f32 Condition_InContext(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    f32 result = 0.0f;
    if (packet != NULL && packet->owner != NULL) {
        GameObject_s *object = packet->owner->apiobj.objptr;
        if (object != NULL && object->character_context == static_cast<i32>(reinterpret_cast<isize>(argument))) {
            result = 1.0f;
        }
    }
    return result;
}

static void *Condition_HitPointsInit(AISYS_s *system, char *name, AISCRIPT_s *) {
    return name != NULL && system != NULL ? GetNamedGameObject(system, name) : NULL;
}

static f32 Condition_HitPoints(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *packet, char *, void *argument) {
    GameObject_s *object = static_cast<GameObject_s *>(argument);
    if (object == NULL) {
        if (packet != NULL && packet->owner != NULL) {
            object = packet->owner->apiobj.objptr;
        }
    }
    f32 result = 0.0f;
    if (object != NULL) {
        result = static_cast<i8>(object->current_hp);
    }
    return result;
}

static f32 Condition_CurrentHintId(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *) {
    return Hint_CurrentId();
}

static void *Condition_HintAvailableInit(AISYS_s *, char *argument, AISCRIPT_s *) {
    return argument != NULL ? reinterpret_cast<void *>(static_cast<isize>(NuAToI(argument))) : NULL;
}

static f32 Condition_HintAvailable(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && Hint_isAvailable(static_cast<i32>(reinterpret_cast<isize>(argument))) != 0 ? 1.0f : 0.0f;
}

static f32 Condition_HintComplete(AISYS_s *, AISCRIPTPROCESS_s *, AIPACKET_s *, char *, void *argument) {
    return argument != NULL && Hint_isComplete(static_cast<i32>(reinterpret_cast<isize>(argument))) != 0 ? 1.0f : 0.0f;
}

static f32 Condition_EmptyTakeOver(AISYS_s *system, AISCRIPTPROCESS_s *, AIPACKET_s *, char *name, void *) {
    if (name == NULL || system == NULL) {
        return 0.0f;
    }
    i32 character = -1;
    for (i32 index = 0; index < CHARCOUNT && character == -1; ++index) {
        if (NuStrICmp(CDataList[index].file, name) == 0) {
            character = index;
        }
    }
    GameObject_s *object = Obj;
    for (i32 index = 0; index < HIGHGAMEOBJECT; ++index, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) == 0x1001 && (object->apiobj.field_0x1f4 & 0x400) != 0 &&
            object->id == character) {
            if (object->takeover_target == NULL || object->field_0xcc0 == NULL ||
                object->field_0xcc0->character_context != 0x3b) {
                return 1.0f;
            }
        }
    }
    return 0.0f;
}

extern "C" {
    AICONDITIONDEF lego_aiconditiondefs[] = {
        {"GlynTest", NULL, NULL},
        {"Debug", NULL, NULL},
        {"Active", NULL, NULL},
        {"GotGun", NULL, NULL},
        {"PrefersBrawling", NULL, NULL},
        {"IsAlive", NULL, NULL},
        {"IsOnScreen", Condition_IsOnScreen, Condition_IsOnScreenInit},
        {"OffScreenTimer", NULL, NULL},
        {"OnObject", NULL, Condition_OnObjectInit},
        {"OnSameObjectAsPlayer", Condition_OnSameObjectAsPlayer, NULL},
        {"PlayerOnObject", Condition_PlayerOnObject, Condition_OnObjectInit},
        {"EitherPlayerOnObject", NULL, Condition_OnObjectInit},
        {"EitherPlayerLocatorRangeXZ", NULL, NULL},
        {"OnGround", NULL, NULL},
        {"BeenAlerted", NULL, NULL},
        {"PlayerOnGround", Condition_PlayerOnGround, NULL},
        {"SpawnCount", NULL, NULL},
        {"BehindCamera", NULL, NULL},
        {"LocatorOnScreen", NULL, NULL},
        {"Blocking", Condition_Blocking, NULL},
        {"BeenHit", NULL, NULL},
        {"HoverPhase", NULL, NULL},
        {"HitPoints", Condition_HitPoints, Condition_HitPointsInit},
        {"OnDynamicGrapple", Condition_OnDynamicGrapple, Condition_OnDynamicGrappleInit},
        {"XPos", NULL, NULL},
        {"YPos", NULL, NULL},
        {"ZPos", NULL, NULL},
        {"CollidingWithOpponent", Condition_CollidingWithOpponent, NULL},
        {"Colliding", NULL, NULL},
        {"ObstacleAtStart", Condition_ObstacleAtStart, Condition_ObstacleInit},
        {"ObstacleAtEnd", Condition_ObstacleAtEnd, Condition_ObstacleInit},
        {"SpecialAtStart", Condition_ObstacleAtStart, Condition_GizSpecialInit},
        {"SpecialAtEnd", Condition_ObstacleAtEnd, Condition_GizSpecialInit},
        {"ObstacleLockedOpen", Condition_ObstacleLockedOpen, Condition_ObstacleOpenedByPlayerInit},
        {"ObstacleLockedShut", Condition_ObstacleLockedShut, Condition_ObstacleOpenedByPlayerInit},
        {"ForceAtStart", Condition_ForceAtStart, Condition_ForceInit},
        {"ForceAtEnd", Condition_ForceAtEnd, Condition_ForceInit},
        {"ObstacleOpenedByPlayer", Condition_ObstacleOpenedByPlayer, Condition_ObstacleOpenedByPlayerInit},
        {"ObstacleOpenedByEitherPlayer", Condition_ObstacleOpenedByEitherPlayer, Condition_ObstacleOpenedByPlayerInit},
        {"AnimationFinished", Condition_AnimationFinished, Condition_AnimationFinishedInit},
        {"EitherPlayerPullingLever", Condition_EitherPlayerPullingLever, NULL},
        {"EitherPlayerUsingHatMachine", Condition_EitherPlayerUsingHatMachine, NULL},
        {"EitherPlayerUsingPanel", Condition_EitherPlayerUsingPanel, NULL},
        {"EitherPlayerWearingHelmet", Condition_EitherPlayerWearingHelmet, NULL},
        {"PartyUnderCover", NULL, NULL},
        {"NumBaddiesThatCanSeePlayers", Condition_NumBaddiesThatCanSeePlayers, NULL},
        {"PlayerUsingForce", Condition_PlayerUsingForce, Condition_UsingForceInit},
        {"EitherPlayerUsingForce", Condition_EitherPlayerUsingForce, Condition_UsingForceInit},
        {"UsingForce", Condition_UsingForce, Condition_UsingForceInit},
        {"OnForcePlatform", Condition_OnForcePlatform, Condition_OnForcePlatformInit},
        {"PlayerOnForcePlatform", Condition_PlayerOnForcePlatform, Condition_OnForcePlatformInit},
        {"EitherPlayerOnForcePlatform", Condition_EitherPlayerOnForcePlatform, Condition_OnForcePlatformInit},
        {"ForceBeingUsed", Condition_ForceBeingUsed, Condition_UsingForceInit},
        {"ForcePushing", Condition_ForcePushing, Condition_ForcePushingInit},
        {"TurretAlive", NULL, NULL},
        {"PlayerDeflectingPart", Condition_PlayerDeflectingPart, NULL},
        {"ForceComplete", Condition_ForceComplete, Condition_ForceCompleteInit},
        {"ForceFinished", Condition_ForceFinished, Condition_ForceCompleteInit},
        {"ForceStackComplete", Condition_ForceStackComplete, Condition_ForceCompleteInit},
        {"ForceStackCompleteInOrder", Condition_ForceStackCompleteInOrder, Condition_ForceCompleteInit},
        {"BuildItComplete", NULL, NULL},
        {"BlowupBlownup", NULL, NULL},
        {"IAmA", NULL, NULL},
        {"OpponentIsA", NULL, NULL},
        {"OpponentIsAThreat", NULL, NULL},
        {"CanFightLikeAJedi", Condition_CanFightLikeAJedi, NULL},
        {"IAmAGoody", NULL, NULL},
        {"IAmABaddy", NULL, NULL},
        {"IAmANeutral", NULL, NULL},
        {"IAmAGoodyBaddy", NULL, NULL},
        {"IAmAPartyCharacter", Condition_IAmAPartyCharacter, NULL},
        {"CategoryIs", NULL, NULL},
        {"PlayerCategoryIs", NULL, NULL},
        {"EitherPlayerIs", NULL, NULL},
        {"Player1Is", NULL, NULL},
        {"Player2Is", NULL, NULL},
        {"IsSetAlive", Condition_IsSetAlive, Condition_IsSetAliveInit},
        {"NumInSetAlive", Condition_NumInSetAlive, Condition_IsSetAliveInit},
        {"Context", NULL, NULL},
        {"InContext", Condition_InContext, Condition_InContextInit},
        {"OpponentContext", Condition_OpponentContext, Condition_InContextInit},
        {"Player2Active", Condition_Player2Active, NULL},
        {"NumBaddies", NULL, NULL},
        {"NumForceObjects", Condition_NumForceObjects, Condition_NumForceObjectsInit},
        {"BeenToLevel", NULL, NULL},
        {"LastLevel", Condition_LastLevel, Condition_LastLevelInit},
        {"Message", Condition_Message, Condition_MessageInit},
        {"ScriptParam", Condition_ScriptParam, Condition_ScriptParamInit},
        {"CutSceneStarted", Condition_CutSceneStarted, Condition_CutSceneStartedInit},
        {"CutSceneFinished", Condition_CutSceneFinished, Condition_CutSceneFinishedInit},
        {"CutSceneExists", Condition_CutSceneExists, Condition_CutSceneExistsInit},
        {"PlayerInSock", Condition_PlayerInSock, Condition_PlayerInSockInit},
        {"CutScenePlaying", Condition_CutScenePlaying, Condition_CutScenePlayingInit},
        {"RigidAnimFrame", Condition_RigidAnimFrame, Condition_RigidAnimFrameInit},
        {"SockDistanceToPlayer", Condition_SockDistanceToPlayer, NULL},
        {"SockDistanceToOpponent", Condition_SockDistanceToOpponent, NULL},
        {"SockXDistanceToPlayer", Condition_SockXDistanceToPlayer, NULL},
        {"PlayerDistanceAlongSock", Condition_PlayerDistanceAlongSock, NULL},
        {"FurthestPlayerDistanceAlongSock", Condition_FurthestPlayerDistanceAlongSock, NULL},
        {"FinishedSpline", Condition_FinishedSpline, NULL},
        {"CurrentHintId", Condition_CurrentHintId, NULL},
        {"HintAvailable", Condition_HintAvailable, Condition_HintAvailableInit},
        {"HintComplete", Condition_HintComplete, Condition_HintAvailableInit},
        {"Freeplay", Condition_Freeplay, NULL},
        {"Indy", Condition_Indy, NULL},
        {"MissionMode", Condition_MissionMode, NULL},
        {"MissionWon", Condition_MissionWon, NULL},
        {"ChallengeMode", Condition_ChallengeMode, NULL},
        {"PSP", Condition_PSP, NULL},
        {"AIOverrideControl", Condition_AIOverrideControl, Condition_AIOverrideControlInit},
        {"BoltsDontGetDeflectedBack", Condition_BoltsDontGetDeflectedBack, NULL},
        {"CheatProgress", Condition_CheatProgress, NULL},
        {"BigJumpComplete", Condition_BigJumpComplete, NULL},
        {"RespawnLocatorIs", Condition_RespawnLocatorIs, Condition_RespawnLocatorIsInit},
        {"InMiniCut", Condition_InMiniCut, NULL},
        {"MaulShouldRunAway", Condition_MaulShouldRunAway, NULL},
        {"DropBackInTimer", Condition_DropBackInTimer, NULL},
        {"HelpWithTriggers", Condition_HelpWithTriggers, NULL},
        {"EitherPlayerPushingSpinner", Condition_EitherPlayerPushingSpinner, Condition_EitherPlayerPushingSpinnerInit},
        {"CharacterRange", Condition_CharacterRange, Condition_CharacterRangeInit},
        {"BeenSpawned", Condition_BeenSpawned, NULL},
        {"LastAttackerRange", Condition_LastAttackerRange, NULL},
        {"LastAttackerIsActivePlayer", Condition_LastAttackerIsActivePlayer, NULL},
        {"PartyContainsDroids", Condition_PartyContainsDroids, NULL},
        {"CannotReachDestination", Condition_CannotReachDestination, NULL},
        {"TakenOver", Condition_TakenOver, Condition_TakenOverInit},
        {"PlayerTakenOver", Condition_PlayerTakenOver, NULL},
        {"EitherPlayerTakenOver", Condition_EitherPlayerTakenOver, NULL},
        {"BeenTakenOver", Condition_BeenTakenOver, Condition_BeenTakenOverInit},
        {"OnSpeederBike", Condition_OnSpeederBike, Condition_OnSpeederBikeInit},
        {"UnderPlayerControl", Condition_UnderPlayerControl, Condition_UnderPlayerControlInit},
        {"CharacterExists", NULL, NULL},
        {"CharacterTypeExists", Condition_CharacterTypeExists, Condition_CharacterTypeExistsInit},
        {"GotLocatorInSet", Condition_GotLocatorInSet, Condition_GotLocatorInSetInit},
        {"GotOpponentLOS", Condition_GotOpponentLOS, NULL},
        {"EmptyTakeOver", Condition_EmptyTakeOver, NULL},
        {"HasTakeOverTarget", NULL, NULL},
        {"TakeOverRange", NULL, NULL},
        {"TakeOverTargetInTriggerArea", NULL, NULL},
        {"EitherPlayerInMyTriggerArea", Condition_EitherPlayerInMyTriggerArea, NULL},
        {"AreaContainsBaddies", Condition_AreaContainsBaddies, Condition_AreaContainsBaddiesInit},
        {"AreaContainsGoodies", Condition_AreaContainsGoodies, Condition_AreaContainsGoodiesInit},
        {"AreaContainsPartyMember", Condition_AreaContainsPartyMember, Condition_AreaContainsPartyMemberInit},
        {"GotVictim", NULL, NULL},
        {"IsVisible", Condition_IsVisible, Condition_IsVisibleInit},
        {"MySet", NULL, NULL},
        {"ScreenWipe", NULL, NULL},
        {"IAmPlayer2", Condition_IAmPlayer2, NULL},
        {"HeadTurnRestricted", NULL, NULL},
        {"ShopActive", NULL, NULL},
        {"Side", NULL, NULL},
        {"NearestPartyRange", NULL, NULL},
        {"NearestPartyXZRange", NULL, NULL},
        {"OpponentToPlayerRange", Condition_OpponentToPlayerRange, NULL},
        {"OpponentPathPosRange", Condition_OpponentPathPosRange, NULL},
        {"GizmoOutput0", NULL, NULL},
        {"GizmoOutput1", NULL, NULL},
        {"GizmoOutput2", NULL, NULL},
        {"GizmoOutput3", NULL, NULL},
        {"GizmoVisibility", NULL, NULL},
        {"AngleAboutMyLocatorToPlayer", Condition_AngleAboutMyLocatorToPlayer,
         Condition_AngleAboutMyLocatorToPlayerInit},
        {"AnimSpeedMul", NULL, NULL},
        {"PickupBeenTurnedOn", Condition_PickupBeenTurnedOn, Condition_PickupBeenTurnedOnInit},
        {"FlowBoxComplete", NULL, NULL},
        {"CanHearRadio", NULL, NULL},
        {"BeingTowed", NULL, NULL},
        {"RaceLap", Condition_RaceLap, NULL},
        {"MusicOn", Condition_MusicOn, NULL},
        {"CharacterLoaded", NULL, NULL},
        {"AreaComplete", NULL, NULL},
        {"ShouldAttackOpponent", Condition_ShouldAttackOpponent, NULL},
        {"InSwamp", NULL, NULL},
        {"InSameTriggerAreaAsNearestPlayer", NULL, NULL},
        {"NetworkGameOnGoing", NULL, NULL},
        {"InHubArea", &Condition_InHubArea, &Condition_InHubAreaInit},
        {"IsLowEndDevice", NULL, NULL},
        {"RandomMapCharsAvailable", Condition_RandomMapCharsAvailable, NULL},
        {NULL, NULL, NULL},
    };

    static_assert(sizeof(lego_aiconditiondefs) / sizeof(lego_aiconditiondefs[0]) == 178,
                  "complete game AI condition registry");

    f32 default_path_heighttol = 0.2f;
    u8 default_activate_difficulty = AI_DEFAULT_ACTIVATE_DIFFICULTY;
    u8 default_min_n_respawns;
    u8 default_max_n_respawns;
    f32 default_min_t_respawn;
    f32 default_max_t_respawn;

    CHARACTERNAMEFN *LevelCharacterNameFn;
    CHARACTERNAMEFN *SpecialRouteCharacterNameFn;
    CHARACTERGLOBALIDFN *LevelCharacterGlobalIDFn;
    GLOBALCHARACTERNAMEFN *GlobalCharacterNameFn;
    CHARACTERHGOBJFN *GlobalCharacterHGobjFn;
    CHARACTERRENDERFN *GlobalCharacterRenderFn;
    CHARACTERGOALSPEEDFN *GetCharacterGoalSpeedFn;
    CHARACTERTYPEIDFN *LevelCharacterTypeIDFn;

    AICHARACTERTYPEID *GlobalCharacterTypeIDFn;
    AISPECIALROUTECHARACTERTYPEID *SpecialRouteCharacterTypeIDFn;
    AICHARACTERDISTANCE *GetViewRangeFn;
    AICHARACTERDISTANCE *GetHearDistanceFn;
    AICHARACTERDISTANCE *GetMaxViewHeightFn;
    AICHARACTERDISTANCE *GetMinViewHeightFn;
    GAMEAILOAD *GameAILoadFn;
    AIACTIONPARSESPEED *AIActionParseSpeedFn;
    AIBIGJUMPTODESTINATION *AIBigJumpToDestinationFn;
    AIRESPAWNONPATH *AIRespawnOnPathFn;
    AICLEARCREATURES *ClearAICreaturesFn;
    APIOBJECTFROMOBJID *APIOBJECTFromObjIDFn;
    AIFINDALTERNATIVESPECIALOBJECT *FindAlternativeSpecialObjectFn;
    AIGETNAMEDAPIOBJECT *GetNamedAPIObjectFn;
    AIGETCREATUREORIGIN *GetAICreatureOriginFn;
}

static SCRIPT_ERROR_LEVEL ScriptErrorLevel;

// The special-route list reserves the first ten ids for suit characters.  The
// remaining ids enumerate the current story list while omitting variants that
// are represented by those dedicated routes.
static const char *skip_chars[] = {"Batman", "Robin", "Glide_Pack", NULL};

static char *LevelCharacterName(u8 character_index) {
    if (character_index == 0xff || CurrentStoryCList == NULL) {
        return NULL;
    }

    const i16 character_type = CurrentStoryCList[character_index].model_id;
    if (character_type == -1) {
        return NULL;
    }
    return apicharsys->char_data[character_type].file;
}

static i32 LevelCharacterGlobalID(u8 character_index) {
    if (character_index == 0xff || CurrentStoryCList == NULL) {
        return -1;
    }
    return CurrentStoryCList[character_index].model_id;
}

static char *GlobalCharacterName(i32 character_type) {
    if (character_type == -1 || character_type >= apicharsys->character_count) {
        return NULL;
    }
    return apicharsys->char_data[character_type].file;
}

static void *GlobalCharacterHGobj(i32 character_type) {
    if (character_type == -1) {
        return NULL;
    }

    const i16 model_index = apicharsys->playermodelids[character_type];
    if (model_index == -1) {
        return NULL;
    }
    return apicharsys->models[model_index].hierarchy;
}

static f32 GetViewRange(i32 character_type) {
    if (character_type == -1) {
        return 0.0f;
    }
    return static_cast<GAMECHARACTERDATA *>(apicharsys->char_data[character_type].field11_0x24)->viewdistance;
}

static f32 GetHearDistance(i32 character_type) {
    if (character_type == -1) {
        return 0.0f;
    }
    return static_cast<GAMECHARACTERDATA *>(apicharsys->char_data[character_type].field11_0x24)->heardistance;
}

static f32 GetMaxViewHeight(i32 character_type) {
    if (character_type == -1) {
        return 0.0f;
    }
    return static_cast<GAMECHARACTERDATA *>(apicharsys->char_data[character_type].field11_0x24)->maxviewheight;
}

static f32 GetMinViewHeight(i32 character_type) {
    if (character_type == -1) {
        return 0.0f;
    }
    return static_cast<GAMECHARACTERDATA *>(apicharsys->char_data[character_type].field11_0x24)->minviewheight;
}

static NUVEC *GetAICreatureOrigin(AISYS *system, AIPACKET *packet) {
    if (packet == NULL || system == NULL || packet->field_0x134 == 0xff)
        return NULL;
    if ((packet->navigation_flags & 8) == 0) {
        AICREATURE *creature = &system->creatures[packet->field_0x134];
        NUVEC offset;
        offset.x = ((packet->group_column + 1) >> 1) * creature->x_spacing;
        if ((packet->group_column & 1) != 0)
            offset.x = -offset.x;
        offset.y = 0.0f;
        offset.z = -creature->z_spacing * (f32)(u32)packet->group_row;
        NuVecRotateY(&offset, &offset, creature->y_rot);
        NuVecAdd(&packet->creature_origin, &offset, &creature->pos);
        packet->navigation_flags |= 8;
    }
    return &packet->creature_origin;
}

static APIOBJECT *GetNamedAPIObject(AISYS *system, char *name) {
    if (system != NULL && Obj != NULL) {
        for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
            GameObject_s *object = &Obj[index];
            if ((object->apiobj.field_0x1f8 & APIOBJECT_FLAG_IN_USE) == 0) {
                continue;
            }

            char *object_name = NULL;
            if ((object->apiobj.field_0x1f4 & 0x400) != 0 && object->ai.field_0x134 != 0xff &&
                system->creatures != NULL) {
                object_name = system->creatures[object->ai.field_0x134].name;
            } else if (object->apiobj.character_data != NULL) {
                object_name = object->apiobj.character_data->file;
            }
            if (object_name != NULL && NuStrICmp(object_name, name) == 0) {
                return &object->apiobj;
            }
        }
    }

    if (NuStrICmp(name, "player") == 0) {
        return player != NULL ? &player->apiobj : NULL;
    }
    if (NuStrICmp(name, "player_2") == 0) {
        GameObject_s *second = Player[0] == player ? Player[1] : Player[0];
        return second != NULL ? &second->apiobj : NULL;
    }
    for (i32 index = 0; index < 8; ++index) {
        if (Player[index] == NULL) {
            continue;
        }
        char player_name[72];
        sprintf(player_name, "Player%d", index);
        if (NuStrICmp(player_name, name) == 0) {
            return &Player[index]->apiobj;
        }
    }
    return NULL;
}

static i32 GlobalCharacterTypeID(char *name) {
    for (i32 character_type = 0; character_type < apicharsys->character_count; ++character_type) {
        if (NuStrICmp(name, apicharsys->char_data[character_type].file) == 0) {
            return character_type;
        }
    }
    return -1;
}

static i32 GameFindAlternativeSpecialObject(AISYS *, nuhspecial_s *special) {
    return EquivalentObject_Find(WorldInfo_CurrentlyActive(), special);
}

static void GameAILoad(AISYS *, i32, NUGSCN *, VARIPTR *, VARIPTR *) {
}

static void GlobalCharacterRender(NUVEC *, i16, i32, i32, EDCREATURE_s *) {
}

static f32 GetCharacterGoalSpeed(APIOBJECT *object) {
    if (object == NULL || object->ai == NULL) {
        return 0.0f;
    }

    switch (object->ai->goal_speed_mode) {
        case AI_ACTION_SPEED_RUN:
            return static_cast<GAMECHARACTERDATA *>(object->character_data->field11_0x24)->movement_speed * FRAMETIME;
        case AI_ACTION_SPEED_WALK:
            return static_cast<GAMECHARACTERDATA *>(object->character_data->field11_0x24)->field_0x18 * FRAMETIME;
        case AI_ACTION_SPEED_TIPTOE:
            return static_cast<GAMECHARACTERDATA *>(object->character_data->field11_0x24)->field_0x14 * FRAMETIME;
        default:
            return 0.0f;
    }
}

static i32 GameAIActionParseSpeed(char *name, u8 *speed) {
    if (NuStrICmp(name, "RUN") == 0) {
        *speed = AI_ACTION_SPEED_RUN;
        return 1;
    }
    if (NuStrICmp(name, "WALK") == 0) {
        *speed = AI_ACTION_SPEED_WALK;
        return 1;
    }
    if (NuStrICmp(name, "TIPTOE") == 0) {
        *speed = AI_ACTION_SPEED_TIPTOE;
        return 1;
    }
    return 0;
}

static char *SpecialRouteCharacterName(u8 route_id) {
    if (route_id == 0xff) {
        return NULL;
    }
    if (route_id < 10) {
        return Suit[route_id].suit_character_name;
    }

    i32 skipped_count = 0;
    for (i32 route_index = 0; route_index < 64;) {
        const i32 list_index = route_index + skipped_count;
        const i16 character_type = CurrentStoryCList[list_index].model_id;
        if (character_type == -1 || list_index > 63) {
            return NULL;
        }

        char *name = apicharsys->char_data[character_type].file;
        bool skip = false;
        for (const char **skip_name = &skip_chars[1]; *skip_name != NULL; ++skip_name) {
            if (NuStrICmp(name, *skip_name) == 0) {
                skip = true;
                break;
            }
        }
        if (skip) {
            ++skipped_count;
            continue;
        }
        if (route_index + 10 == route_id) {
            return name;
        }
        ++route_index;
    }
    return NULL;
}

static i32 LevelCharacterTypeID(char *name) {
    if (NuStrICmp(name, "Everyone") == 0) {
        return 0x40;
    }
    if (CurrentStoryCList == NULL) {
        return -1;
    }

    i32 character_index = 0;
    i16 character_type = CurrentStoryCList[character_index].model_id;
    if (character_type == -1) {
        return -1;
    }

    while (true) {
        if (NuStrICmp(name, apicharsys->char_data[character_type].file) == 0) {
            return character_index;
        }

        ++character_index;
        character_type = CurrentStoryCList[character_index].model_id;
        if (character_type == -1) {
            return -1;
        }
        if (character_index == 64) {
            return -1;
        }
    }
}

static u32 AIBigJumpToDestination(APIOBJECT *object, NUVEC *destination) {
    if (destination == NULL || object == NULL || object->objptr == NULL || object->field_0x287 != 0 ||
        object->objptr->field_0x7a5 == AI_GAME_OBJECT_TYPE_VEHICLE) {
        return 1;
    }

    GameObject_s *game_object = object->objptr;
    if (game_object->field_0xcc0 != NULL) {
        SnapCreaturePos(game_object, destination, 0, NULL, 0);
    } else if ((object->character_data->model_flags & CHARACTER_AI_MODEL_FLAG_SNAP_ON_BIG_JUMP) != 0) {
        object->start_position = *destination;
        object->position = *destination;
        object->velocity = v000;
        ResetPlayerMoves(game_object);
        object->respawn_timer = 0.0f;
    } else {
        StartBigJump(game_object, destination, 0, 0.5f, 1.0f, 0, 0);
    }

    return 1;
}

static u32 AIRespawnOnPath(APIOBJECT *object) {
    if (object->field_0x287 != 0 || (object->ai->path_info.flags & AI_RESPAWN_FLAG_DISABLED) != 0 ||
        (object->flags_high & APIOBJECT_HIGH_FLAG_RESPAWN_ENABLED) == 0) {
        return 0;
    }

    const u32 model_flags = object->character_data->model_flags;
    if ((model_flags & CHARACTER_AI_MODEL_FLAG_DISABLE_RESPAWN) != 0) {
        return 0;
    }

    GameObject_s *game_object = object->objptr;
    if (game_object->field_0x7a5 == AI_GAME_OBJECT_TYPE_VEHICLE || game_object->field_0xcc0 != NULL) {
        return 0;
    }

    if (object->respawn_timer > AI_RESPAWN_DELAY) {
        if ((model_flags & CHARACTER_AI_MODEL_FLAG_SNAP_ON_BIG_JUMP) != 0) {
            object->start_position = object->respawn_position;
            object->position = object->respawn_position;
            object->velocity = v000;
            ResetPlayerMoves(game_object);
            object->respawn_timer = 0.0f;
        } else {
            StartBigJump(game_object, &object->respawn_position, 0, 0.5f, 1.0f, 0, 0);
        }
    }

    return 0;
}

static i32 SpecialRouteCharacterTypeID(char *name) {
    if (NuStrICmp(name, "Everyone") == 0) {
        return 0x40;
    }

    for (i32 suit_index = 0; suit_index < 10; ++suit_index) {
        if (NuStrICmp(name, Suit[suit_index].suit_character_name) == 0) {
            return suit_index;
        }
    }

    if (CurrentStoryCList == NULL) {
        return -1;
    }
    i32 skipped_count = 0;
    for (i32 route_index = 0; route_index < 64;) {
        const i32 list_index = route_index + skipped_count;
        const i16 character_type = CurrentStoryCList[list_index].model_id;
        if (character_type == -1 || list_index > 63) {
            return -1;
        }

        char *character_name = apicharsys->char_data[character_type].file;
        bool skip = false;
        for (const char **skip_name = &skip_chars[1]; *skip_name != NULL; ++skip_name) {
            if (NuStrICmp(character_name, *skip_name) == 0) {
                skip = true;
                break;
            }
        }
        if (skip) {
            ++skipped_count;
            continue;
        }
        if (NuStrICmp(name, character_name) == 0) {
            return route_index + 10;
        }
        ++route_index;
    }
    return -1;
}

extern "C" {
    f32 NewShadowEx(NUVEC *position, i32 handle, f32 height_above, f32 height_below, i32 terrain_mask);
    void PlatOnOff(i32 platform_id, i32 enabled);
}
extern i32 TimingBarSet;
extern i32 SHADOWCALLS;
extern u32 LAYER_HOVERIGNORE;

f32 GameShadow(GameObject_s *object, nuvec_s *position, f32 probe_height, i32 terrain_mask) {
    i32 disabled_platforms[16];
    i32 disabled_platform_count = 0;

    if (object != NULL) {
        WORLDINFO_s *world = WorldInfo_CurrentlyActive();
        if (object->field_0x107c != -1) {
            disabled_platforms[disabled_platform_count++] = object->field_0x107c;
        }

        if (world != NULL && world->char_platform_sys != NULL && VehicleArea == 0 &&
            static_cast<i8>(object->apiobj.field_0x1f8) >= 0) {
            CHARPLATFORMSYS_s *system = world->char_platform_sys;
            for (i32 i = 0; i < system->platform_count; ++i) {
                if (system->platforms[i].object != NULL) {
                    disabled_platforms[disabled_platform_count++] = system->platforms[i].object->field_0x107c;
                }
            }
        }

        for (i32 i = 0; i < disabled_platform_count; ++i) {
            PlatOnOff(disabled_platforms[i], 0);
        }

        if (LAYER_HOVERIGNORE != 0xffffffff &&
            static_cast<GAMECHARACTERDATA_s *>(object->apiobj.character_data->field11_0x24)->field_0x28 == 0.0f) {
            terrain_mask &= ~LAYER_HOVERIGNORE;
        }
    }

    if (TimingBarSet == 2) {
        TBOPENFN("Ter", 2);
    }
    const f32 shadow_height = NewShadowEx(position, 0, probe_height, probe_height, terrain_mask);
    ++SHADOWCALLS;
    if (TimingBarSet == 2) {
        TBCLOSEFN("Ter", 2);
    }

    if (object != NULL && object->field_0x107c != -1) {
        PlatOnOff(object->field_0x107c, 1);
    }
    for (i32 i = 0; i < disabled_platform_count; ++i) {
        PlatOnOff(disabled_platforms[i], 1);
    }
    return shadow_height;
}

extern f32 MainRenderTime;
extern f32 MainRenderTargetTime;
extern f32 backdrop_top_r;
extern f32 backdrop_top_g;
extern f32 backdrop_top_b;
extern f32 backdrop_bot_r;
extern f32 backdrop_bot_g;
extern f32 backdrop_bot_b;
extern void (*BackDrop_AlphaFn)(f32 *alpha);
extern void BackDrop_UpdateColours(i32 instant);
extern i32 Paused;
extern f32 PauseMenus_X;
extern i32 PauseMenus_Align;
struct CUTSCENEPLAYERCLIP;
extern CUTSCENEPLAYERCLIP *CutScenePlayer_Active();

void UpdateGameMessages();
extern i32 DoubleScore;
extern FadeSystem FadeSys;
i32 InDoubleScoreZone(GameObject_s *object);

void GameTiming(WORLDINFO_s *, float *game_time) {
    if (Paused == 0) {
        if (game_time != NULL) {
            *game_time += FRAMETIME;
        }
        UpdateTimer(&GameTimer);
        UpdateTimer(&LevelTimer);
        UpdateTimer(&AreaTimer);
        if (CUTSTOPGAME == 0) {
            UpdateGameMessages();
            f32 target = 0.0f;
            if (DoubleScore != 0 && GetMenuID() == -1)
                target = 1.0f;
            DoubleScoreTime = SeekLinearF(DoubleScoreTime, target, FRAMETIME);
        } else {
            DoubleScoreTime = 0.0f;
        }
    } else {
        DoubleScoreTime = 0.0f;
    }

    UpdateTimer(&GlobalTimer);
    menu_flash = NuFmod(GlobalTimer.time_elapsed_mod_seconds, 0.2f) < 0.1f;

    f32 pulse_time = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f);
    game_pulse = NuTrigTable[(i32)(pulse_time * 2.0f * 65536.0f) >> 1 & 0x7fff];
    pulse_time = NuFmod(GlobalTimer.time_elapsed_mod_seconds, 0.5f);
    global_pulse = NuTrigTable[(i32)(pulse_time * 2.0f * 65536.0f) >> 1 & 0x7fff];

    MainRenderTime = SeekLinearF(MainRenderTime, MainRenderTargetTime, FRAMETIME);
    qrand();
}

void GameFog_Set() {
    if (NuIOS_IsLowEndDevice()) {
        NuLightFogX(GameFog.low_quality_start, GameFog.low_quality_end, GameFog.colour, 0.0f, 0.0f, 0, 0.0f);
        return;
    }

    NuLightFogX(GameFog.high_quality_start, GameFog.high_quality_end, GameFog.colour, 0.0f, 0.0f, 1,
                GameFog.high_quality_density);
}

extern "C" i32 NewRayCastScaleYMask(NUVEC *, NUVEC *, f32, f32, i32, u32);
extern i32 RAYCASTCALLS;

i32 GameRayCast(NUVEC *position, NUVEC *displacement, f32 radius, i32 mask) {
    i32 hit = NewRayCastScaleYMask(position, displacement, radius, 1.0f, 0, mask);
    ++RAYCASTCALLS;
    return hit;
}

f32 draw_attention_distance = 2.0f;
extern f32 party_follow_offsets[8];
extern i32 active_neutral_count;
extern "C" i32 party_under_cover;
extern i32 party_cant_be_under_cover;
extern AREADATA *VADER_ADATA;
extern GameObject_s *alert_obj;
void AITriggerSetSysProcess(AITRIGGERSETSYS_s *system);
i32 Hub_InMenu();
i32 test_new_interaction;
i32 debug_ignore_players;
i32 WALKSTEALTH;
i32 active_goody_count;
extern i32 active_baddy_count;
f32 timetogetlos;
void GameCreatureOpponentSelection(AISYS_s *, i32, APIOBJECT **, i32, APIOBJECT **, i32, APIOBJECT **, u64, f32);
void GizTurrets_OpponentSelection(GIZTURRETSYS_s *, i32, APIOBJECT **, i32, APIOBJECT **);

static void TestWalkAround(APIOBJECT *object, APIOBJECT *other, NUVEC *difference, f32 radius) {
    if (difference->x * difference->x + difference->z * difference->z < radius * radius) {
        difference->x = object->position.x - other->position.x;
        difference->z = object->position.z - other->position.z;
        i32 angle = NuAtan2D(difference->x, difference->z);
        difference->x = object->ai->movement_destination.x - other->position.x;
        difference->z = object->ai->movement_destination.z - other->position.z;
        i32 turn = NuAngSub(NuAtan2D(difference->x, difference->z), angle);
        i32 limit = static_cast<i32>((ai_moveradius / radius) * 10430.378f);
        if (turn > limit)
            turn = limit;
        else if (turn < -limit)
            turn = -limit;
        angle = NuAngAdd(angle, turn);
        difference->x = 0.0f;
        difference->y = 0.0f;
        difference->z = radius;
        NuVecRotateY(difference, difference, angle);
        object->ai->movement_position.x = other->position.x + difference->x;
        object->ai->movement_position.y = object->ai->movement_destination.y;
        object->ai->movement_position.z = other->position.z + difference->z;
    }
}

void GameAIProcess() {
    if (TimingBarSet == 4)
        TBOPENFN("(Sys)", 4);
    ai_fighting = 0;
    AISysProcess(WORLD->ai_sys, reinterpret_cast<APIOBJECT *>(player), reinterpret_cast<APIOBJECT *>(player2));

    for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
        GameObject_s *object = &Obj[index];
        if ((object->apiobj.flags_high & 0x10) == 0 || object->apiobj.field_0x287 != 0) {
            continue;
        }
        object->ai.capabilities &= ~0x00e00000u;
        if ((object->apiobj.flags_low & 0x80) != 0 && TouchHacks::TouchControlsActive) {
            if ((object->ai.capabilities & 1) != 0)
                object->ai.capabilities |= 0x00200000;
            if ((object->ai.capabilities & 2) != 0)
                object->ai.capabilities |= 0x00400000;
            if ((object->ai.capabilities & 0x20) != 0)
                object->ai.capabilities |= 0x00800000;
        }
        object->field_0xefd &= 0x7f;
        object->field_0xf01 &= 0x3f;
        object->field_0xf02 &= 0xfc;
        object->field_0x107e = 0;
        if (object->use_action_frames != 0) {
            --object->use_action_frames;
        } else if (object->use_action_parameter > 0.0f) {
            object->use_action_parameter -= FRAMETIME;
        } else {
            object->can_use_object = NULL;
            object->use_action = 0;
        }
        if (object->field_0x1094 != 0)
            --object->field_0x1094;
        object->apiobj.visibility_range_extension =
            (object->ai.field_0x1e5 & 0x40) != 0 ? draw_attention_distance : 0.0f;
        if (MiniCutCam != 0 && (object->apiobj.flags_high & 1) != 0) {
            object->pad_gamepad->buttons_held &= GAMEPAD_START;
            object->pad_gamepad->buttons_pressed &= GAMEPAD_START;
        }
        if (object->alert_target != NULL) {
            object->alert_target_timer -= FRAMETIME;
            if (object->alert_target_timer <= 0.0f) {
                object->alert_target = NULL;
                object->alert_target_timer = 0.0f;
            }
        }
    }

    party_under_cover = 0;
    active_neutral_count = 0;
    i32 all_under_cover = 1;
    for (i32 index = 0; index < 8; ++index) {
        GameObject_s *object = Player[index];
        if (object != NULL && (object->apiobj.object_flags & 0x1001) == 0x1001 &&
            (object->apiobj.field_0x287 == 0 || object->field_0x101c > 0.0f) && (object->field_0xeff & 1) == 0 &&
            (VADER_ADATA == NULL || WORLD->area != VADER_ADATA) && party_cant_be_under_cover == 0) {
            if (object->field_0x108e == 5 ||
                ((FreePlay != 0 || WORLD->current_level == HUB_LDATA) &&
                 (object->apiobj.character_data->model_flags & 0x204) != 0 && object->field_0xcc0 == NULL)) {
                if (alert_obj == NULL || alert_obj->apiobj.field_0x27c == -1 || (alert_obj->field_0xeff & 1) != 0)
                    party_under_cover = 1;
            }
            if ((object->apiobj.character_data->model_flags & 0x80000) == 0)
                all_under_cover = 0;
        }
        party_follow_offsets[index] = 0.5f;
    }
    if ((VADER_ADATA == NULL || WORLD->area != VADER_ADATA) && all_under_cover != 0 && party_cant_be_under_cover == 0) {
        party_under_cover = 1;
    }
    AITriggerSetSysProcess(WORLD->ai_trigger_set_sys);

    APIOBJECT *neutral_objects[64];
    APIOBJECT *interactive_objects[64];
    APIOBJECT *goodies[64];
    APIOBJECT *baddies[64];
    i32 neutral_count = 0;
    i32 interactive_count = 0;
    i32 goody_count = 0;
    i32 baddy_count = 0;
    f32 follow_offset = 0.0f;
    for (i32 index = 0; index < 8; ++index) {
        GameObject_s *object = Player[index];
        if (index < 2 && Player[1] == player)
            object = Player[1 - index];
        if (object != NULL && (object->apiobj.object_flags & 0x1001) == 0x1001 &&
            (object->apiobj.field_0x287 == 0 || object->field_0x101c > 0.0f) && object->character_context != 0x17 &&
            object->character_context != 0x5a) {
            if (party_under_cover != 0 && (object->ai.field_0x1e5 & 0x40) != 0)
                party_under_cover = 0;
            if (!(object->camera_screen_position.x > -0.85f && object->camera_screen_position.x < 0.85f &&
                  object->camera_screen_position.y > -0.85f && object->camera_screen_position.y < 0.85f) &&
                (object->tag_flags & 2) == 0 && drop_back_in_timer > 0.0f) {
                object->ai.nearest_opponent = NULL;
                object->ai.pending_nearest_opponent = NULL;
                object->ai.pending_nearest_metric = 1.0e9f;
                object->ai.opponent = NULL;
                object->ai.pending_opponent = NULL;
                object->ai.pending_opponent_metric = 1.0e9f;
                object->ai.field_0x1e5 &= ~0x10;
            } else if ((object->apiobj.field_0x1f4 & 1) != 0) {
                baddies[baddy_count++] = &object->apiobj;
            } else if ((object->apiobj.field_0x1f4 & 4) != 0) {
                neutral_objects[neutral_count++] = &object->apiobj;
            } else {
                goodies[goody_count++] = &object->apiobj;
            }
        }
        object = Player[index];
        if (object != NULL && (object->apiobj.object_flags & 0x1001) == 0x1001 &&
            (object->apiobj.field_0x287 == 0 || object->field_0x101c > 0.0f) && (object->field_0xeff & 1) == 0 &&
            (object->apiobj.flags_low & 0x80) == 0) {
            party_follow_offsets[object->apiobj.field_0x27c] = follow_offset;
            follow_offset += 0.2f;
        }
    }

    u64 awareness = 0;
    NUVEC wall_direction;
    for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
        GameObject_s *object = &Obj[index];
        i32 ground_checks;
        i32 process_ai;
        const u16 character_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER;
        object->active_trigger_set = NULL;
        if ((object->apiobj.field_0x1f8 & character_flags) != character_flags || object->apiobj.field_0x287 != 0 ||
            object->character_context == 0x3b) {
            goto interaction;
        }

        ground_checks = object->apiobj.field_0x27d != 0;
        if (ground_checks == 0) {
            ground_checks = (object->apiobj.character_data->model_flags >> 13) & 1;
        }

        process_ai = (object->field_0xf00 & GAME_OBJECT_AI_UPDATE_PROCESS) != 0;
        if ((object->apiobj.field_0x1f4 & 0x400) != 0) {
            if ((object->apiobj.field_0x1f4 & 0x10000) != 0) {
                baddies[baddy_count++] = &object->apiobj;
                goodies[goody_count++] = &object->apiobj;
            } else if ((object->apiobj.field_0x1f4 & 1) != 0) {
                baddies[baddy_count++] = &object->apiobj;
            } else if ((object->apiobj.field_0x1f4 & 4) != 0) {
                neutral_objects[neutral_count++] = &object->apiobj;
            } else {
                goodies[goody_count++] = &object->apiobj;
            }
        }
        if ((object->apiobj.field_0x1f4 & 0x40000) != 0) {
            if ((object->apiobj.flags_high & 0x10) != 0 && object->apiobj.field_0x287 == 0 &&
                object->context_target_position == NULL &&
                (object->apiobj.character_data->game_character->flags_090 & 0x8000) == 0) {
                interactive_objects[interactive_count++] = &object->apiobj;
                object->apiobj.resolved_collision_priority = 0;
                object->apiobj.collision_priority = HIGHGAMEOBJECT - object->apiobj.field_0x289;
                object->ai.field_0x1e5 |= 0x20;
            } else if (object->field_0x101c > 0.0f) {
                object->ai.field_0x1e5 |= 0x20;
            } else {
                object->ai.nearest_opponent = NULL;
                object->ai.opponent = NULL;
                object->ai.field_0x1e5 &= ~0x20;
            }
            if (object->opponent != NULL && (static_cast<APIOBJECT *>(object->opponent)->flags_low & 1) == 0)
                object->opponent = NULL;
            if (object->ai.opponent_object != NULL && ((object->ai.opponent_object->flags_low & 1) == 0 ||
                                                       (object->ai.opponent_object->ai->field_0x1e5 & 0x20) == 0)) {
                object->ai.opponent_metric = 1.0e9f;
                object->ai.opponent = NULL;
            }
            if (process_ai != 0)
                AISysUpdateCharacterPathPos(WORLD->ai_sys, &object->apiobj, &object->ai, ground_checks,
                                            object->ai_elapsed_time);
            continue;
        }
        if (object->pad_gamepad->pad == NULL) {
            object->pad_gamepad->buttons_held = 0;
            object->pad_gamepad->buttons_pressed = 0;
            object->pad_gamepad->buttons_released = 0;
            object->pad_gamepad->left_directions = 0;
            object->pad_gamepad->previous_left_directions = 0;
            object->pad_gamepad->right_directions = 0;
            object->pad_gamepad->previous_right_directions = 0;
            object->pad_gamepad->unknown_20 = 0;
        }
        awareness |= object->apiobj.ai_awareness_mask;
        if (process_ai != 0) {
            object->script_fire_target = NULL;
            object->field_0xef9 = (object->field_0xef9 & ~4) | ((object->field_0xef9 << 1) & 4);
            object->field_0xef8 &= ~0x20;
            if ((object->field_0xef9 & 0x80) != 0) {
                object->field_0xef9 &= ~0x80;
                object->field_0xe64 = object->field_0xe58;
            }
            object->field_0xefa &= ~3;
        }
        AISysProcessCharacter(WORLD->ai_sys, &object->apiobj, &object->ai, ground_checks, object->ai_elapsed_time, 0,
                              process_ai);
        if ((object->apiobj.flags_low & 0x80) == 0) {
            if (object->character_context == 0x5d)
                SetBallooningHeight(object, object->ai.movement_destination.y);
            if (WORLD->current_level == JEDI_B_LDATA && object->id == id_JANGOFETT && (object->field_0xefb & 8) != 0 &&
                object->hover_height_override != 1.0e9f && object->field_0xe31 == 0)
                object->field_0xe31 = 1;
            if (object->apiobj.field_0x27c != -1 && object->field_0xe31 != 0 &&
                (object->ai.capabilities & LEGO_AIPATHCNX_R2D2GLIDE) == 0)
                object->field_0xe31 = 0;
            if (object->character_context == 0x4b && object->suit != NULL &&
                (static_cast<SUIT_s *>(object->suit)->flags & 0x10) != 0)
                object->pad_gamepad->buttons_held |= GAMEPAD_SPECIAL;
            AIPATHCNX *connection = object->ai.path_info.connection;
            if (connection != NULL && (connection->traversal_flags[object->ai.path_info.direction] &
                                       object->ai.capabilities & LEGO_AIPATHCNX_WALLSHUFFLE) != 0) {
                object->field_0xf02 |= 1;
                AIPATHNODE *nodes = object->ai.path_info.path->nodes;
                i32 direction = object->ai.path_info.direction;
                u8 other_node = connection->node_indices[direction == 0];
                u8 node = connection->node_indices[direction];
                u16 angle;
                if (object->character_context == 0x45) {
                    angle = object->takeover_start_angle;
                    wall_direction = v001;
                    NuVecRotateY(&wall_direction, &wall_direction, angle);
                } else {
                    NUVEC lateral;
                    NuVecSub(&lateral, &nodes[other_node].position, &nodes[node].position);
                    lateral.y = 0.0f;
                    NuVecNorm(&lateral, &lateral);
                    NuVecRotateY(&lateral, &lateral, 0x4000);
                    NUVEC ray = lateral;
                    f32 first_distance = 1.0e9f;
                    if (GameRayCast(&nodes[node].position, &ray, 0.0f,
                                    TERRAINMASK_NONWEAPON | TERRAINMASK_NONDROID | 0x5f) != 0)
                        first_distance = NuVecMag(&ray);
                    ray.x = -lateral.x;
                    ray.y = 0.0f;
                    ray.z = -lateral.z;
                    f32 second_distance = 1.0e9f;
                    if (GameRayCast(&nodes[node].position, &ray, 0.0f,
                                    TERRAINMASK_NONWEAPON | TERRAINMASK_NONDROID | 0x5f) != 0)
                        second_distance = NuVecMag(&ray);
                    if (first_distance < 1.0e9f || second_distance < 1.0e9f) {
                        if (second_distance > first_distance) {
                            wall_direction = lateral;
                        } else {
                            wall_direction.x = -lateral.x;
                            wall_direction.y = 0.0f;
                            wall_direction.z = -lateral.z;
                        }
                    }
                    angle = NuAtan2D(wall_direction.x, wall_direction.z);
                }
                NUVEC movement;
                movement.x = object->ai.movement_position.x - object->apiobj.position.x;
                movement.y = 0.0f;
                movement.z = object->ai.movement_position.z - object->apiobj.position.z;
                f32 distance = NuVecMag(&movement);
                if (distance > ai_moveradius)
                    NuVecScale(&movement, &movement, (ai_moveradius / distance) * 2.0f);
                NuVecRotateY(&movement, &movement, -static_cast<i32>(angle));
                movement.z = 0.0f;
                NuVecRotateY(&movement, &movement, angle);
                NuVecScale(&wall_direction, &wall_direction, ai_moveradius);
                NuVecAdd(&movement, &movement, &wall_direction);
                object->ai.movement_position.x = object->apiobj.position.x + movement.x;
                object->ai.movement_position.y = object->apiobj.position.y;
                object->ai.movement_position.z = object->apiobj.position.z + movement.z;
            } else if (object->ai.path_connection_state == 0 && (object->ai.path_info.flags & 1) != 0 &&
                       object->apiobj.movement_stuck_time > jump_stuck_time) {
                if (object->apiobj.supporting_platform_id != -1 && connection != NULL &&
                    (connection->original_traversal_flags[0] & LEGO_AIPATHCNX_BLOCKAGE) != 0) {
                    if ((object->apiobj.character_data->model_flags & 0x88) != 0) {
                        object->pad_gamepad->buttons_pressed |= GAMEPAD_ACTION;
                        object->apiobj.movement_stuck_time = 0.0f;
                        GameObjectSetCanUse(object, NULL, 5, 0, 1.0f);
                    } else if (FreePlay != 0) {
                        object->input_toggle_hold_time -= FRAMETIME;
                        if (object->input_toggle_hold_time <= 0.0f) {
                            Player_ToggleCharacter(object, 1, 0);
                            if (object->id == id_DROIDEKA)
                                Player_ToggleCharacter(object, 1, 0);
                            object->input_toggle_hold_time = object->apiobj.model_draw_result != 0 ? 0.25f : 0.0f;
                        }
                    }
                } else if ((object->ai.capabilities & (LEGO_AIPATHCNX_R2D2GLIDE | LEGO_AIPATHCNX_JUMP)) != 0 &&
                           object->field_0xe31 == 0) {
                    object->pad_gamepad->buttons_pressed |= GAMEPAD_JUMP;
                    object->apiobj.movement_stuck_time = 0.0f;
                } else {
                    object->ai.field_0x1e6 |= AIPACKET_RUNTIME_USING_PATH_WAYPOINT;
                }
            }
        }
        if (FreePlay != 0 && (object->apiobj.field_0x1f4 & 0x400) == 0) {
            if ((object->apiobj.flags_low & 0x80) == 0 && object->character_context != 0x0b) {
                if (drop_in_teleport == 1 && VehicleArea == 0 && (Arcade != 0 || teleport_all_freeplay_modes != 0) &&
                    object->apiobj.model_draw_result == 0 && (object->tag_flags & 2) == 0 &&
                    drop_back_in_timer > 0.0f) {
                    NUVEC position = player->apiobj.last_safe_position;
                    if (position.x == player->apiobj.position.x && position.y == player->apiobj.position.y &&
                        position.z == player->apiobj.position.z)
                        position.z += 0.01f;
                    object->apiobj.start_position = position;
                    object->apiobj.position = object->apiobj.start_position;
                    object->apiobj.velocity = v000;
                    object->apiobj.field_0x276 = player->apiobj.field_0x276;
                    ResetPlayerMoves(object);
                    object->apiobj.movement_stuck_time = 0.0f;
                    InitSurfaceInfo(object);
                    SetObjOnSurface(object, 1);
                    GameObjectOrigin(object);
                    AddGameDebris(WORLD->debris_sys, 0x5c, &object->apiobj.collision_position);
                    goto ai_combat;
                }
                if ((object->ai.field_0x1e6 & AIPACKET_RUNTIME_ROUTE_SELECTED) != 0) {
                    object->ai.current_route = 0xff;
                    object->route_character_id = -1;
                    object->field_0xf14 = 0;
                } else if ((object->ai.field_0x1e6 & AIPACKET_RUNTIME_USING_PATH_WAYPOINT) != 0 &&
                           ((object->ai.field_0x1e6 & AIPACKET_RUNTIME_PATH_BLOCKED) != 0 ||
                            (object->ai.path_info.connection != NULL &&
                             object->ai.path_info.connection->route_mask != 0))) {
                    if ((object->field_0xefc & 0x40) != 0) {
                        if (object->field_0xf14 != object->ai.frame_state) {
                            object->route_character_id = -1;
                            object->route_suit_index = -1;
                            object->field_0xefc &= ~0x40;
                            object->field_0xf14 = 0;
                        }
                    } else if ((object->ai.frame_state & LEGO_AIPATHCNX_DONTTOGGLE) == 0) {
                        if (object->route_character_id == -1 || object->field_0xf14 != object->ai.frame_state ||
                            object->route_character_id == id_DROIDEKA) {
                            object->route_character_id = object->id;
                            object->route_suit_index =
                                object->suit != NULL ? Suit_GetIndex(static_cast<SUIT_s *>(object->suit)) : -1;
                            object->input_toggle_hold_time = 0.0f;
                            object->field_0xf14 = object->ai.frame_state;
                            object->route_start_index = object->ai.next_route;
                            object->route_search_index = object->ai.next_route;
                        }
                        object->input_toggle_hold_time -= FRAMETIME;
                        if (object->input_toggle_hold_time <= 0.0f) {
                            Player_ToggleCharacter(object, 1, 0);
                            if (object->id == id_DROIDEKA)
                                Player_ToggleCharacter(object, 1, 0);
                            object->input_toggle_hold_time = object->apiobj.model_draw_result != 0 ? 0.25f : 0.0f;
                            if (object->id == object->route_character_id &&
                                Suit_GetIndex(static_cast<SUIT_s *>(object->suit)) == object->route_suit_index) {
                                Player_ToggleCharacter(object, -1, 0);
                                if (object->id == id_DROIDEKA)
                                    Player_ToggleCharacter(object, -1, 0);
                                object->route_character_id = object->id;
                                object->route_suit_index = Suit_GetIndex(static_cast<SUIT_s *>(object->suit));
                                object->ai.next_route = object->route_search_index;
                                AISysFindRoute(&object->ai);
                                object->route_search_index = object->ai.next_route;
                                if (object->route_search_index == object->route_start_index)
                                    object->field_0xefc |= 0x40;
                            }
                        }
                    }
                } else {
                    object->route_character_id = -1;
                    object->route_suit_index = -1;
                    object->field_0xefc &= ~0x40;
                    object->field_0xf14 = 0;
                    object->route_search_index = 0;
                }
            }
        } else if ((object->ai.field_0x1e6 & AIPACKET_RUNTIME_USING_PATH_WAYPOINT) != 0) {
            AISysFindRoute(&object->ai);
        }
    ai_combat:
        if ((object->apiobj.field_0x1f8 & 0x180) == 0x80) {
            object->field_0xefc &= ~4;
            object->apiobj.flags_high =
                (object->apiobj.flags_high & ~2) | ((object->apiobj.character_data->model_flags >> 27) & 2);
        } else {
            object->apiobj.flags_high &= ~2;
            bool defend = false;
            if ((object->field_0xef8 & 0x80) != 0 && object->ai.opponent_object != NULL) {
                BOLTTYPE_s *bolt = BoltType_FindByID(0, WORLD);
                if (bolt->field_10 * bolt->field_14 > object->ai.opponent_metric) {
                    NUVEC direction, forward;
                    NuVecSub(&direction, &object->ai.opponent_object->collision_position,
                             &object->apiobj.collision_position);
                    f32 scale = object->ai.opponent_metric == 0.0f ? 0.0f : 1.0f / object->ai.opponent_metric;
                    NuVecScale(&direction, &direction, scale);
                    NuVecRotateY(&forward, &v001, object->apiobj.movement_facing_angle);
                    f32 dot = NuVecDot(&direction, &forward);
                    u16 angle = object->ai.opponent_metric < 0.5f   ? 0x3aaa
                                : object->ai.opponent_metric < 1.0f ? 0x3000
                                                                    : 0x2555;
                    if (dot > NuTrigTable[angle]) {
                        object->pad_gamepad->buttons_pressed |= GAMEPAD_ACTION;
                        object->script_fire_target = *object->ai.action_target_ref;
                    }
                }
                defend = true;
            }
            if (FreePlay != 0 && (object->apiobj.field_0x1f4 & 0x400) == 0 && object->id == id_DROIDEKA)
                object->pad_gamepad->buttons_pressed |= GAMEPAD_TOGGLERIGHT;
            if ((object->field_0xef8 & 2) != 0 &&
                ((object->apiobj.field_0x1f4 & 0x400) != 0 || party_under_cover == 0 ||
                 (object->field_0xeff & 1) != 0) &&
                (CanFightLikeAJedi(object) != 0 || object->id == id_GAMORREANGUARD)) {
                if ((object->field_0xef8 & 1) != 0 || object->ai.opponent_object != NULL ||
                    (object->field_0xe22 & 8) != 0)
                    object->field_0xed8 = DEFENDTIME;
                if (object->field_0xed8 > 0.0f) {
                    defend = true;
                    if ((object->field_0xe22 & 9) == 9)
                        object->pad_gamepad->buttons_pressed |= GAMEPAD_ACTION;
                }
            }
            if ((object->field_0xefb & 0x20) != 0) {
                defend = true;
                if ((object->field_0xe22 & 1) != 0) {
                    object->pad_gamepad->buttons_pressed |= GAMEPAD_ACTION;
                    object->field_0xefb &= ~0x20;
                }
            }
            if ((object->field_0xef8 & 8) != 0 &&
                (defend || (object->field_0xef8 & 0x30) != 0 ||
                 ((object->id == id_YODA || object->id == id_YODAGHOST) && (object->apiobj.flags_low & 0x80) == 0 &&
                  object->apiobj.field_0x27c != -1))) {
                if ((object->field_0xe22 & 1) == 0 && object->character_context != 7 && object->field_0xe32 == 0)
                    object->pad_gamepad->buttons_pressed |= GAMEPAD_ACTION;
            } else if ((object->pad_gamepad->buttons_pressed & GAMEPAD_ACTION) == 0 && (object->field_0xe22 & 1) != 0 &&
                       object->character_context != 6 && object->field_0xe32 == 0) {
                object->pad_gamepad->buttons_pressed |= GAMEPAD_SPECIAL;
            }
        }
        if ((object->apiobj.character_data->game_character->flags_094[3] & 0x10) != 0 &&
            (object->apiobj.flags_low & 0x80) == 0) {
            f32 threshold = object->apiobj.character_data->game_character->walk_speed - 0.01f;
            if (object->field_0xe31 == 1) {
                if (threshold >= object->pad_gamepad->input_magnitude) {
                    object->ai_jump_timer += FRAMETIME;
                    if (object->ai_jump_timer > 0.1f) {
                        object->pad_gamepad->buttons_pressed |= GAMEPAD_JUMP;
                        object->ai_jump_timer = 0.0f;
                    }
                } else {
                    object->ai_jump_timer = 0.0f;
                }
            } else if (object->pad_gamepad->input_magnitude > threshold) {
                object->ai_jump_timer += FRAMETIME;
                if (object->ai_jump_timer > 1.0f) {
                    object->pad_gamepad->buttons_pressed |= GAMEPAD_JUMP;
                    object->ai_jump_timer = 0.0f;
                }
            } else {
                object->ai_jump_timer = 0.0f;
            }
        }
        if (object->ai.action_target_ref != NULL)
            SetObjAsHeadTarget(object, *object->ai.action_target_ref, 2, 1.0f, 0.0f, 0.0f);
    interaction:
        if ((object->apiobj.flags_high & 0x10) != 0 && object->apiobj.field_0x287 == 0 &&
            object->context_target_position == NULL &&
            (object->apiobj.character_data->game_character->flags_090 & 0x8000) == 0) {
            interactive_objects[interactive_count++] = &object->apiobj;
            object->apiobj.resolved_collision_priority = 0;
            object->apiobj.collision_priority = HIGHGAMEOBJECT - object->apiobj.field_0x289;
            if ((object->apiobj.flags_low & 2) != 0 || (object->field_0xeff & 0x80) != 0) {
                object->apiobj.collision_priority = 0x8000;
            } else if ((object->apiobj.flags_low & 0x80) == 0) {
                if ((object->ai.field_0x1e7 & 0x80) != 0)
                    object->apiobj.collision_priority |= 0x4000;
                if ((object->ai.field_0x1e7 & 0x40) != 0)
                    object->apiobj.collision_priority |= 0x2000;
                if ((object->ai.navigation_flags & 1) != 0)
                    object->apiobj.collision_priority |= 0x1000;
                if ((object->pad_gamepad->allocated_5a & 2) != 0)
                    object->apiobj.collision_priority |= 0x800;
                if ((object->ai.path_info.flags & 1) == 0 || object->ai.path_info.connection == NULL)
                    object->apiobj.collision_priority |= 0x400;
                if (object->ai.movement_parameter == 0.0f) {
                    f32 x = object->ai.fallback_destination.x - object->apiobj.position.x;
                    f32 z = object->ai.fallback_destination.z - object->apiobj.position.z;
                    f32 radius = object->ai.mover_height + 1.0f;
                    if (x * x + z * z < radius * radius)
                        object->apiobj.collision_priority |= 0x200;
                }
            }
            object->ai.field_0x1e5 |= 0x20;
            if (object->opponent != NULL && (static_cast<APIOBJECT *>(object->opponent)->flags_low & 1) == 0)
                object->opponent = NULL;
            if (object->ai.opponent_object != NULL && ((object->ai.opponent_object->flags_low & 1) == 0 ||
                                                       (object->ai.opponent_object->ai->field_0x1e5 & 0x20) == 0)) {
                object->ai.opponent = NULL;
                object->ai.opponent_metric = 1.0e9f;
            }
            if ((object->field_0xeff & 8) != 0 || debug_ignore_players != 0 || Hub_InMenu() != 0) {
                object->ai_opponent_exclusion_mask |= (u64)1 << player->apiobj.field_0x289;
                if (player2 != NULL)
                    object->ai_opponent_exclusion_mask |= (u64)1 << player2->apiobj.field_0x289;
            }
            if ((object->apiobj.flags_low & 0x80) != 0 && object->apiobj.field_0x27d != 0 &&
                object->pad_gamepad->input_magnitude >
                    (WALKSTEALTH != 0 ? object->apiobj.character_data->game_character->walk_speed
                                      : object->apiobj.character_data->game_character->tiptoe_speed))
                object->field_0xef9 |= 8;
        } else if (object->field_0x101c > 0.0f) {
            object->ai.field_0x1e5 |= 0x20;
        } else {
            object->ai.nearest_opponent = NULL;
            object->ai.opponent = NULL;
            object->ai.field_0x1e5 &= ~0x20;
        }
    }
    if (TimingBarSet == 4)
        TBCLOSEFN("(Sys)", 4);
    active_goody_count = goody_count;
    active_baddy_count = baddy_count;
    active_neutral_count = neutral_count;
    if (TimingBarSet == 4)
        TBOPENFN("(LOS)", 4);
    for (i32 index = 0; index < neutral_count; ++index)
        baddies[baddy_count + index] = neutral_objects[index];
    i32 los_count = baddy_count + neutral_count;
    timetogetlos = (f32)(los_count * los_count) * FRAMETIME * 0.5f;
    APIObjectLOSChecks(WORLD->api_object_sys, 2, goody_count, goodies, los_count, baddies,
                       (f32) static_cast<u8>(WORLD->current_level->unknown_0da));
    if (TimingBarSet == 4)
        TBCLOSEFN("(LOS)", 4);
    if (TimingBarSet == 4)
        TBOPENFN("(Avoid)", 4);
    if (test_new_interaction == 0) {
        LEGO_AISysCreatureInteraction2D(WORLD->ai_sys, interactive_count, interactive_objects, NULL, FRAMETIME);
    } else {
        for (i32 index = 0; index < interactive_count; ++index) {
            f32 timer = interactive_objects[index]->ai->antinode_timer - FRAMETIME;
            if (timer < 0.0f)
                timer = 0.0f;
            interactive_objects[index]->ai->antinode_timer = timer;
        }
        for (i32 first_index = 0; first_index < interactive_count - 1; ++first_index) {
            APIOBJECT *first = interactive_objects[first_index];
            for (i32 second_index = first_index + 1; second_index < interactive_count; ++second_index) {
                APIOBJECT *second = interactive_objects[second_index];
                f32 radius = first->ai->mover_height + second->ai->mover_height;
                NUVEC difference;
                difference.x = first->ai->movement_position.x - second->ai->movement_position.x;
                if (difference.x > radius || difference.x < -radius)
                    continue;
                difference.z = first->ai->movement_position.z - second->ai->movement_position.z;
                if (difference.z > radius || difference.z < -radius)
                    continue;
                if (first->collision_min.y > second->collision_max.y ||
                    second->collision_min.y > first->collision_max.y || first->collision_link == second ||
                    second->collision_link == first)
                    continue;
                bool fixed_first;
                bool fixed_second;
                if (first->collision_priority > second->collision_priority) {
                    fixed_first = true;
                    fixed_second = (second->flags_low & 0x80) != 0;
                } else if (first->collision_priority < second->collision_priority) {
                    fixed_first = (first->flags_low & 0x80) != 0;
                    fixed_second = true;
                } else {
                    continue;
                }
                if (fixed_first && fixed_second)
                    continue;
                f32 distance = NuFsqrt(difference.x * difference.x + difference.z * difference.z);
                if (distance < radius) {
                    difference.y = 0.0f;
                    if (fixed_first) {
                        if (first->collision_priority > second->resolved_collision_priority) {
                            second->resolved_collision_priority = first->collision_priority;
                            difference.x = -difference.x;
                            difference.z = -difference.z;
                            TestWalkAround(second, first, &difference, radius);
                        }
                    } else if (fixed_second) {
                        if (second->collision_priority > first->resolved_collision_priority) {
                            first->resolved_collision_priority = second->collision_priority;
                            TestWalkAround(first, second, &difference, radius);
                        }
                    } else {
                        first->resolved_collision_priority = second->collision_priority;
                        difference.x = -difference.x;
                        difference.z = -difference.z;
                        TestWalkAround(second, first, &difference, radius);
                    }
                    AIPACKET *first_ai = first->ai;
                    AIPACKET *second_ai = second->ai;
                    f32 first_timer = first_ai->antinode_timer;
                    f32 second_timer = second_ai->antinode_timer;
                    if (first_timer > antinode_time && second_timer > antinode_time) {
                        if (first_timer > second_timer) {
                            second_ai->antinode_timer = first_timer;
                            second_ai->antinode_clockwise = first_ai->antinode_clockwise;
                        } else {
                            first_ai->antinode_timer = second_timer;
                            first_ai->antinode_clockwise = second_ai->antinode_clockwise;
                        }
                    } else if (second_timer > antinode_time) {
                        first_ai->antinode_timer = second_timer;
                        first_ai->antinode_clockwise = second_ai->antinode_clockwise;
                    } else if (first_timer > 0.0f) {
                        second_ai->antinode_timer = first_timer;
                        second_ai->antinode_clockwise = first_ai->antinode_clockwise;
                    }
                    if (((first->field_0x1f4 ^ second->field_0x1f4) & 1) != 0) {
                        first->ai->field_0x1e5 |= 0x40;
                        second->ai->field_0x1e5 |= 0x40;
                    }
                }
            }
        }
        AISysCreatureAntinodeInteraction(WORLD->ai_sys, interactive_count, interactive_objects, NULL);
    }
    if (TimingBarSet == 4)
        TBCLOSEFN("(Avoid)", 4);
    if (party_under_cover != 0) {
        for (i32 index = 0; index < 8; ++index) {
            if (Player[index] != NULL)
                Player[index]->ai.field_0x1e5 &= ~0x40;
        }
    }
    if (TimingBarSet == 4)
        TBOPENFN("(Opponent)", 4);
    GameCreatureOpponentSelection(WORLD->ai_sys, interactive_count, interactive_objects, goody_count, goodies,
                                  baddy_count, baddies, awareness, FRAMETIME);
    if (TimingBarSet == 4)
        TBCLOSEFN("(Opponent)", 4);
    if (TimingBarSet == 4)
        TBOPENFN("(Turret)", 4);
    GizTurrets_OpponentSelection(WORLD->giz_turret_sys, goody_count, goodies, baddy_count, baddies);
    if (TimingBarSet == 4)
        TBCLOSEFN("(Turret)", 4);
    oneAtOnce_MaintainArray();
}

extern "C" {
    void InitFn_LevelCharacterTypeID(CHARACTERTYPEIDFN *function) {
        LevelCharacterTypeIDFn = function;
        SpecialRouteCharacterTypeIDFn = function;
    }

    void InitFn_SpecialRouteCharacterTypeID(CHARACTERTYPEIDFN *function) {
        SpecialRouteCharacterTypeIDFn = function;
    }

    void InitFn_LevelCharacterName(CHARACTERNAMEFN *function) {
        LevelCharacterNameFn = function;
        SpecialRouteCharacterNameFn = function;
    }

    void InitFn_SpecialRouteCharacterName(CHARACTERNAMEFN *function) {
        SpecialRouteCharacterNameFn = function;
    }

    void InitFn_LevelCharacterGlobalID(CHARACTERGLOBALIDFN *function) {
        LevelCharacterGlobalIDFn = function;
    }

    void InitFn_GlobalCharacterTypeID(CHARACTERTYPEIDFN *function) {
        GlobalCharacterTypeIDFn = function;
    }

    void InitFn_GlobalCharacterName(GLOBALCHARACTERNAMEFN *function) {
        GlobalCharacterNameFn = function;
    }

    void InitFn_GlobalCharacterRender(CHARACTERRENDERFN *function) {
        GlobalCharacterRenderFn = function;
    }

    void InitFn_GlobalCharacterHGobj(CHARACTERHGOBJFN *function) {
        GlobalCharacterHGobjFn = function;
    }

    void InitFn_ClearAICreatures(AICLEARCREATURES *function) {
        ClearAICreaturesFn = function;
    }

    void InitFn_GetCharacterGoalSpeedFn(CHARACTERGOALSPEEDFN *function) {
        GetCharacterGoalSpeedFn = function;
    }

    void InitFn_GetViewRange(CHARACTERDISTANCEFN *function) {
        GetViewRangeFn = function;
    }

    void InitFn_GetHearDistance(CHARACTERDISTANCEFN *function) {
        GetHearDistanceFn = function;
    }

    void InitFn_GlobalGetMaxViewHeight(CHARACTERDISTANCEFN *function) {
        GetMaxViewHeightFn = function;
    }

    void InitFn_GlobalGetMinViewHeight(CHARACTERDISTANCEFN *function) {
        GetMinViewHeightFn = function;
    }

    void InitFn_GameAILoad(GAMEAILOAD *function) {
        GameAILoadFn = function;
    }

    void InitFn_AIActionParseSpeed(AIACTIONPARSESPEED *function) {
        AIActionParseSpeedFn = function;
    }

    void InitFn_AIRespawnOnPath(AIRESPAWNONPATH *function) {
        AIRespawnOnPathFn = function;
    }

    void InitFn_AIBigJumpToDestination(AIBIGJUMPTODESTINATION *function) {
        AIBigJumpToDestinationFn = function;
    }

    void InitFn_FindAlternativeSpecialObjectFn(AIFINDALTERNATIVESPECIALOBJECT *function) {
        FindAlternativeSpecialObjectFn = function;
    }

    void InitFn_APIOBJECTFromObjIDFn(APIOBJECTFROMOBJID *function) {
        APIOBJECTFromObjIDFn = function;
    }

    void InitFn_GetNamedAPIObject(AIGETNAMEDAPIOBJECT *function) {
        GetNamedAPIObjectFn = function;
    }

    void InitFn_GetAICreatureOrigin(AIGETCREATUREORIGIN *function) {
        GetAICreatureOriginFn = function;
    }

    void SetScriptErrorLevel(SCRIPT_ERROR_LEVEL level) {
        if (level < SCRIPT_ERROR_LEVEL_COUNT) {
            ScriptErrorLevel = level;
        }
    }
}

extern f32 OFFSCREEN_CATCHUP_TIME;
extern f32 drop_back_in_timer;

i32 TryToTeleportToNextNode(GameObject_s *object, AIPATHNODE_s *node, i32 tightrope) {
    if ((object->apiobj.field_0x1f4 & 0x400) != 0 || !object->ai.path_info.on_path) {
        return 0;
    }
    if (!(object->field_0xf1c >= OFFSCREEN_CATCHUP_TIME) &&
        (object->apiobj.model_draw_result != 0 || (object->tag_context_flags & 2) != 0 ||
         !(drop_back_in_timer > 0.0f))) {
        return 0;
    }
    NUVEC position = node->position;
    position.y += object->apiobj.field_0x1e0;
    if (NuCameraClipTestSphere(&position, object->apiobj.field_0x1e0, &numtx_identity) == 0) {
        return 0;
    }
    if (tightrope != 0) {
        if (TightRope_SnapTo(object, &node->position) == 0) {
            return 0;
        }
        object->field_0x109c = 0;
        object->field_0x1092 = 0;
        object->field_0x1093 = 0;
        object->ai.path_connection_state = 0;
        return 1;
    }
    object->apiobj.initial_position = node->position;
    object->apiobj.collision_position = object->apiobj.initial_position;
    object->apiobj.start_position = object->apiobj.initial_position;
    object->ai.owner->apiobj.position = object->apiobj.initial_position;
    plr_lastpos = object->apiobj.initial_position;
    object->apiobj.velocity = v000;
    InitSurfaceInfo(object);
    SetObjOnSurface(object, 0);
    object->field_0x1092 = 0;
    object->field_0x1093 = 0;
    object->field_0x109c = 0;
    object->ai.path_connection_state = 0;
    if (object->field_0xcc0 == NULL) {
        Player_ClearContext(object, 0);
        Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(object->player_packet));
    }
    return 1;
}

void SetSpecialMove(GameObject_s *object, AIPATHNODE_s *target, AIPATHNODE_s *node, char move) {
    object->ai.field_0x180 = target;
    object->field_0x1092 = move;
    object->field_0x1093 = 0;
    object->special_move_node = node;
    object->special_move_progress = 0.0f;
}

void ClearSpecialMove(GameObject_s *object) {
    object->special_move_progress = 1.0f;
    object->ai.field_0x180 = NULL;
    object->field_0x1092 = 0;
    object->field_0x1093 = 0;
    object->special_move_node = NULL;
    object->field_0x1080 = 0.0f;
    object->field_0x107f = 0;
}

void GameAISysInit() {
    RegisterAIScriptActions(lego_aiactiondefs);
    RegisterAIScriptConditions(lego_aiconditiondefs);
    InitFn_LevelCharacterTypeID(LevelCharacterTypeID);
    InitFn_SpecialRouteCharacterTypeID(SpecialRouteCharacterTypeID);
    InitFn_LevelCharacterName(LevelCharacterName);
    InitFn_SpecialRouteCharacterName(SpecialRouteCharacterName);
    InitFn_LevelCharacterGlobalID(LevelCharacterGlobalID);
    InitFn_GlobalCharacterTypeID(GlobalCharacterTypeID);
    InitFn_GlobalCharacterName(GlobalCharacterName);
    InitFn_GlobalCharacterRender(GlobalCharacterRender);
    InitFn_GlobalCharacterHGobj(GlobalCharacterHGobj);
    InitFn_ClearAICreatures(ClearAICreatures);
    InitFn_GetCharacterGoalSpeedFn(GetCharacterGoalSpeed);
    InitFn_GetViewRange(GetViewRange);
    InitFn_GetHearDistance(GetHearDistance);
    InitFn_GlobalGetMaxViewHeight(GetMaxViewHeight);
    InitFn_GlobalGetMinViewHeight(GetMinViewHeight);
    InitFn_GameAILoad(GameAILoad);
    InitFn_AIActionParseSpeed(GameAIActionParseSpeed);
    InitFn_AIRespawnOnPath(AIRespawnOnPath);
    InitFn_AIBigJumpToDestination(AIBigJumpToDestination);
    InitFn_FindAlternativeSpecialObjectFn(GameFindAlternativeSpecialObject);
    InitFn_APIOBJECTFromObjIDFn(GameAPIOBJECTFromObjID);
    InitFn_GetNamedAPIObject(GetNamedAPIObject);
    InitFn_GetAICreatureOrigin(GetAICreatureOrigin);

    default_path_heighttol = 0.2f;
    default_activate_difficulty = AI_DEFAULT_ACTIVATE_DIFFICULTY;
    default_min_n_respawns = 0;
    default_max_n_respawns = 0;
    default_min_t_respawn = 0.0f;
    default_max_t_respawn = 0.0f;

    SetScriptErrorLevel(SCRIPT_ERROR_LEVEL_WARNING);
    GameAISysSetGame();
}

void GameFog_Reset() {
    GameFogSnap = 1;
    GameFogDuration = 0.0f;
    GameFogSet = 0;
    GameFogTime = 0.0f;
}

void Game_KillPart(PART_s *, i32) {
}

void GameAISysReset(AISYS_s *system) {
    if (system == NULL) {
        return;
    }

    AISysSetLevelPath(system, NULL);

    WORLD->processor_count = 0;
    for (i32 script_index = 0; script_index < 32; ++script_index) {
        char script_name[16];
        if (script_index != 0) {
            sprintf(script_name, "Level%d", script_index);
        } else {
            sprintf(script_name, "Level");
        }

        if (AIScriptFind(WORLD->ai_sys, script_name, 0, 1, 0) == NULL) {
            continue;
        }

        AIScriptProcessorInit(system, NULL, &WORLD->processors[WORLD->processor_count].processor, NULL, script_name,
                              NULL, 0, NULL, NULL);
        NuStrCpy(WORLD->processors[WORLD->processor_count].name, script_name);
        LevelScriptReStoreProgress(WORLD, &WORLD->processors[WORLD->processor_count]);
        ++WORLD->processor_count;
    }

    AIPATHSYS *path_system = system->path_sys;
    if (path_system != NULL) {
        for (i32 path_index = 0; path_index < path_system->path_count; ++path_index) {
            AIPATH *path = path_system->paths[path_index];
            AIPATHCNX *connection = path->connections;
            for (i32 connection_index = 0; connection_index < path->connection_count;
                 ++connection_index, ++connection) {
                connection->node_a = connection->previous_node_a;
                connection->node_b = connection->previous_node_b;

                if (LEGO_AIPATHCNX_BLOCKAGE != 0) {
                    connection->node_a &= ~LEGO_AIPATHCNX_BLOCKAGE;
                    connection->node_b &= ~LEGO_AIPATHCNX_BLOCKAGE;
                }
            }
        }
    }

    AIPathCnxControlSysReset(WORLD->ai_path_cnx_control_sys);
    AIPathCnxHelperSysReset(WORLD, WORLD->ai_path_cnx_helper_sys);
    InitAICreatures(system);
    ResetAICreatures(system);

    if (WORLD->processor_count != 0) {
        GizmoSysAddGizmos(WORLD->gizmo_sys, WORLD->giz_flow, WORLD);
    }
}

extern "C" i32 party_under_cover;
extern i32 party_cant_be_under_cover;
extern GameObject_s *alert_obj;
extern f32 alert_timer;
extern AREADATA *VADER_ADATA;

void MakeBaddiesForgetAboutParty(i32 check_hostility) {
    GameObject_s *baddies[64];
    GameObject_s *party[64];
    i32 party_count = 0;
    i32 baddie_count = 0;
    GameObject_s *objects = Obj;
    i32 count = HIGHGAMEOBJECT;
    GameObject_s *object = objects;
    for (i32 i = 0; i < count; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0)
            continue;
        if ((object->apiobj.field_0x1f4 & 1) != 0)
            party[party_count++] = object;
        else if ((object->apiobj.field_0x1f4 & 4) == 0)
            baddies[baddie_count++] = object;
    }
    if (check_hostility != 0) {
        for (i32 i = 0; i < baddie_count; ++i) {
            u64 mask = u64(1) << baddies[i]->apiobj.field_0x289;
            for (i32 j = 0; j < party_count; ++j) {
                APIOBJECT *object = &party[j]->apiobj;
                u64 hostile = u64(object->field387_0x2a0) | (u64(object->field388_0x2a4) << 32);
                if ((hostile & mask) != 0) {
                    u32 *defaults = WORLD->api_object_sys->hostility_masks[object->field_0x289];
                    if (((u64(defaults[0]) | (u64(defaults[1]) << 32)) & mask) != 0)
                        return;
                }
            }
        }
    }
    for (i32 i = 0; i < baddie_count; ++i) {
        APIOBJECT *baddie = &baddies[i]->apiobj;
        u64 mask = ~(u64(1) << baddie->field_0x289);
        for (i32 j = 0; j < party_count; ++j) {
            APIOBJECT *member = &party[j]->apiobj;
            member->field387_0x2a0 &= static_cast<u32>(mask);
            member->field388_0x2a4 &= static_cast<u32>(mask >> 32);
            u64 other_mask = ~(u64(1) << member->field_0x289);
            baddie->field387_0x2a0 &= static_cast<u32>(other_mask);
            baddie->field388_0x2a4 &= static_cast<u32>(other_mask >> 32);
        }
    }
    object = objects;
    for (i32 i = 0; i < count; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0)
            continue;
        object->ai.field_0x1e5 &= ~0x58;
        object->ai.opponent = NULL;
        object->ai.opponent_metric = 1000000000.0f;
        object->ai.nearest_opponent = NULL;
        object->ai.field_0xdc = 0;
        object->ai.nearest_opponent_metric = 1000000000.0f;
        object->ai.target_metric_e0 = 1000000000.0f;
        object->ai.field_0xec = 0;
        object->ai.target_metric_f0 = 1000000000.0f;
        object->apiobj.field387_0x2a0 = 0;
        object->apiobj.field388_0x2a4 = 0;
        object->field_0xebc = 0;
        object->field_0xec0 = 0;
        object->field_0xecc = 0;
    }
    party_under_cover = 0;
    alert_obj = NULL;
    alert_timer = 0.0f;
    if (VADER_ADATA != NULL && WORLD->area == VADER_ADATA)
        return;
    if (party_cant_be_under_cover != 0)
        return;
    i32 all_under_cover = 1;
    for (i32 i = 0; i < 8; ++i) {
        GameObject_s *player = Player[i];
        if (player == NULL || (player->apiobj.field_0x1f8 & 0x1001) != 0x1001 || player->apiobj.field_0x287 != 0 ||
            (player->field_0xeff & 1) != 0)
            continue;
        if (player->field_0x108e == 5 ||
            ((FreePlay != 0 || WORLD->current_level == HUB_LDATA) &&
             (player->apiobj.character_data->model_flags & 0x204) != 0 && player->field_0xcc0 == NULL)) {
            party_under_cover = 1;
        }
        if ((player->apiobj.character_data->model_flags & 0x80000) == 0)
            all_under_cover = 0;
    }
    if (all_under_cover != 0)
        party_under_cover = 1;
}

void GameAttackInit() {
}

extern "C" void MenuRegisterSoundFX(i32 move, i32 select, i32 back, i32 no_entry);
i32 GameAudio_GetSfxId(i32 sfx);
void GameAudio_PlaySfxById(i32 sfx_id, nuvec_s *position, i32 flags, i32 volume);

void GameFog_Update(WORLDINFO_s *) {
}

void *GameBufferAlloc(variptr_u *buf, variptr_u *buf_end, i32 size) {
    // Carves `size` bytes out of the permanent buffer (original at
    // 0x4890a0); returns the previous cursor.
    void *ptr = (void *)(usize)buf->addr;
    buf->addr += size;
    return ptr;
}

extern i16 tUNKNOWN;
char *GameObj_GetName(i32 model, GameObject_s *object, char *buffer) {
    if (object != NULL) {
        if (object->field_0xcc0 != NULL && object->field_0xcc0->apiobj.character_data->name_id != -1)
            model = object->field_0xcc0->id;
        else
            model = object->id;
    } else if (model == -1) {
        return TTab[tUNKNOWN];
    }
    if (buffer != NULL) {
        i32 index = -1;
        if (model == id_WEIRDO1) {
            if (Game.customizer.primary_use_saved_name)
                index = 0;
        } else if (model == id_WEIRDO2) {
            if (Game.customizer.secondary_use_saved_name)
                index = 1;
        }
        if (index == -1)
            return TTab[CDataList[model].name_id];
        NuStrCpy(buffer,
                 reinterpret_cast<char *>(&Game.customizer) + offsetof(CUSTOMISESAVE_s, primary_name) + index * 0x38);
        for (i32 i = 14; i >= 0; --i) {
            if (buffer[i] != ' ')
                return buffer;
            buffer[i] = '\0';
        }
        NuStrCpy(buffer, "?");
        return buffer;
    }
    return TTab[CDataList[model].name_id];
}

void Game_AutoSaving() {
}

void GameAISysSetGame() {
    AIPathCnxHelperSysInitFn = NULL;
    StarWars_GameAISysInit();
}

void GameAudio_AddSfx(i32 sfx, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx) {
    if (sfx_count == NULL || sfx_ids == NULL || *sfx_count >= max_sfx) {
        return;
    }

    i32 sfx_id = GameAudio_GetSfxId(sfx);
    if (sfx_id == -1) {
        return;
    }
    for (i32 i = 0; i < *sfx_count; ++i) {
        if (sfx_ids[i] == sfx_id) {
            return;
        }
    }
    sfx_ids[*sfx_count] = sfx_id;
    ++*sfx_count;
}

void GameObjectOrigin(GameObject_s *object) {
    APIOBJECT &api = object->apiobj;
    api.field_0x1f4 |= 0x100u;

    f32 predicted_vertical_displacement;
    f32 origin_x;
    f32 origin_z;
    if (object->use_model_origin != 0) {
        PLAYERCHARACTERCONFIG_s *config = api.character_data->player_config;
        const i32 model_origin_joint = config->model_origin_joint;
        CHARACTERMODEL_s *model = api.character_model;
        if (model_origin_joint != -1 && model != NULL && model->points_of_interest[model_origin_joint] != NULL) {
            f32 predicted_x;
            f32 predicted_z;
            if ((object->field_0xe24 & GAMEOBJECT_E24_FLAG_JOINT_MATRICES_UPDATED) == 0) {
                api.collision_position = {0.0f, -object->character_bottom, 0.0f};
                NuVecMtxRotate(&api.collision_position, &api.collision_position, &api.field_0xb8);
                NuVecAdd(&api.collision_position, &api.collision_position, &api.position);
                const f32 frame_time = FRAMETIME;
                predicted_x = api.previous_velocity.x * frame_time;
                predicted_vertical_displacement = api.previous_velocity.y * frame_time;
                predicted_z = api.previous_velocity.z * frame_time;
            } else {
                const f32 frame_time = FRAMETIME;
                predicted_x = api.previous_velocity.x * frame_time;
                predicted_vertical_displacement = api.previous_velocity.y * frame_time;
                predicted_z = api.previous_velocity.z * frame_time;
                const NUVEC &joint_position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[model_origin_joint], 3);
                api.collision_position.x = joint_position.x + predicted_x;
                api.collision_position.y = joint_position.y + predicted_vertical_displacement;
                api.collision_position.y += (object->character_bottom + object->character_top) * api.field_0xa8 * 0.5f;
                api.collision_position.z = joint_position.z + predicted_z;
            }
            api.collision_position.x += predicted_x;
            api.collision_position.y += predicted_vertical_displacement;
            api.collision_position.z += predicted_z;
            origin_x = api.position.x + predicted_x;
            origin_z = api.position.z + predicted_z;
            goto update_bounds;
        }

        const i32 collision_origin_joint = config->collision_origin_joint;
        if (object->field_0xd24 == 1.0f && collision_origin_joint != -1 &&
            model->points_of_interest[collision_origin_joint] != NULL && api.field_0x288 != 0) {
            api.collision_position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[collision_origin_joint], 3);
            const f32 frame_time = FRAMETIME;
            origin_x = api.position.x + api.previous_velocity.x * frame_time;
            origin_z = api.position.z + api.previous_velocity.z * frame_time;
            predicted_vertical_displacement = api.previous_velocity.y * frame_time;
            goto update_bounds;
        }
    }

    api.field_0x1f4 &= ~0x100u;
    predicted_vertical_displacement = api.previous_velocity.y * FRAMETIME;
    api.collision_position.x = api.position.x + api.previous_velocity.x * FRAMETIME;
    api.collision_position.y = api.position.y + predicted_vertical_displacement;
    api.collision_position.y += (object->character_bottom + object->character_top) * api.field_0xa8 * 0.5f;
    api.collision_position.z = api.position.z + api.previous_velocity.z * FRAMETIME;
    origin_x = api.collision_position.x;
    origin_z = api.collision_position.z;

update_bounds:
    const f32 radius = api.field_0x1dc;
    const f32 half_height = api.field_0x1e0;
    api.collision_min.x = api.collision_position.x - radius;
    api.collision_min.y = api.collision_position.y - half_height;
    api.collision_min.z = api.collision_position.z - radius;
    api.collision_max.x = api.collision_position.x + radius;
    api.collision_max.y = api.collision_position.y + half_height;
    api.collision_max.z = api.collision_position.z + radius;

    api.upper_position.x = origin_x;
    api.upper_position.y = api.collision_max.y;
    api.upper_position.z = origin_z;
    api.lower_position.x = origin_x;
    api.lower_position.y = api.collision_min.y;
    api.lower_position.z = origin_z;
    api.collision_origin.x = origin_x;
    api.collision_origin.y = api.collision_min.y + predicted_vertical_displacement;
    api.collision_origin.z = origin_z;

    object->ai.terrain_origin = api.collision_origin;
    const u32 context_flags = CInfo[object->character_context].flags;
    if ((context_flags & CHARACTER_CONTEXT_INFO_FLAG_TERRAIN_ORIGIN_AT_TOP) != 0 &&
        (object->context_variant_flags & 0x08) != 0) {
        object->ai.terrain_origin.y += api.collision_max.y - api.collision_min.y;
    } else if ((context_flags & CHARACTER_CONTEXT_INFO_FLAG_TERRAIN_ORIGIN_AT_POSITION) != 0) {
        object->ai.terrain_origin = api.position;
    } else {
        object->ai.terrain_origin.y = api.collision_origin.y - object->terrain_origin_floor_offset;
        if (object->ai.terrain_origin.y < api.field_0x218 &&
            api.field_0x218 < api.collision_origin.y + api.scaled_height) {
            object->ai.terrain_origin.y = api.field_0x218;
        }
    }
}

i32 Game_IgnoreInput() {
    extern i32 newgamecam;
    return newgamecam != 0;
}

extern i16 id_GONKDROID;
u32 GameAI_TotalScore() {
    GameObject_s *object = Obj;
    u32 total = 0;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        if ((object->apiobj.flags_low & 1) == 0 || (object->apiobj.field_0x1f4 & 0x400) == 0 ||
            object->ai.field_0x134 == 0xff)
            continue;
        GAMECHARACTERDATA_s *config = static_cast<GAMECHARACTERDATA_s *>(object->apiobj.character_data->field11_0x24);
        if ((config->flags_090 & 0x40) == 0 && object->id != id_GONKDROID)
            total += static_cast<u16>(config->field_0xee);
    }
    return total;
}

void GameDrawMenuEntry(MENU_s *menu, char *text) {
    if (Paused != 0) {
        dme_align = PauseMenus_Align;
        menu->draw_x = PauseMenus_X;
    }
    DrawMenuEntryEx(menu, text, static_cast<u8>(MenuA));
}

void GameAnimSys_Update(GAMEANIMSYS_s *system) {
    if (system == NULL) {
        return;
    }

    GAMEANIMSET_s *set = reinterpret_cast<GAMEANIMSET_s *>(NuLinkedListGetHead(&system->active_sets));
    while (set != NULL) {
        GAMEANIMSET_s *next_set =
            reinterpret_cast<GAMEANIMSET_s *>(NuLinkedListGetNext(&system->active_sets, &set->links));

        const u8 was_no_visibility_test = set->flags & GAMEANIMSET_FLAG_NO_VISIBILITY_TEST;
        set->flags &= ~GAMEANIMSET_FLAG_NO_VISIBILITY_TEST;

        if ((set->flags & GAMEANIMSET_FLAG_STOP_REQUESTED) != 0) {
            set->flags &= ~(GAMEANIMSET_FLAG_NO_VISIBILITY_TEST | GAMEANIMSET_FLAG_STOP_REQUESTED);
            GameAnimSet_RemoveFromSystemList(set);
            set = next_set;
            continue;
        }

        set->state = GAMEANIMSET_STATE_AT_START;
        i32 all_playing_forward = 1;
        i32 all_at_start = 1;
        i32 all_at_end = 1;
        i32 any_special_no_visibility_test = 0;

        GAMEANIMOBJ_s *object = set->objects;
        while (object != NULL) {
            if (NuSpecialGetNoVisiTestFn(&object->special) != 0) {
                any_special_no_visibility_test = 1;
            }

            if (object->instance_animation != NULL) {
                const f32 direction = object->start_frame > object->end_frame ? -1.0f : 1.0f;
                const f32 previous_frame = object->instance_animation->ltime;

                if (previous_frame * direction >= object->end_frame * direction) {
                    if (object->instance_animation->tfactor * direction < 0.0f) {
                        object->instance_animation->ltime = object->end_frame;
                    } else if (object->instance_animation->repeating != 0) {
                        object->instance_animation->ltime = previous_frame - object->end_frame + object->start_frame;
                    } else {
                        object->instance_animation->ltime = object->end_frame;
                        object->instance_animation->playing = 0;
                    }
                } else if (previous_frame * direction <= object->start_frame * direction) {
                    if (object->instance_animation->tfactor * direction > 0.0f) {
                        object->instance_animation->ltime = object->start_frame;
                    } else if (object->instance_animation->repeating != 0) {
                        object->instance_animation->ltime = object->end_frame - (object->start_frame - previous_frame);
                    } else {
                        object->instance_animation->ltime = object->start_frame;
                        object->instance_animation->playing = 0;
                    }
                }

                if (previous_frame != object->instance_animation->ltime) {
                    EvalAnim2(&object->special, object->instance_animation->ltime);
                }

                if (object->instance_animation->playing != 0) {
                    set->flags |= GAMEANIMSET_FLAG_NO_VISIBILITY_TEST;
                    if (object->instance_animation->tfactor * direction < 0.0f) {
                        all_playing_forward = 0;
                    }
                }

                const f32 directed_frame = object->instance_animation->ltime * direction;
                if (directed_frame > object->start_frame * direction) {
                    all_at_start = 0;
                }
                if (directed_frame < object->end_frame * direction) {
                    all_at_end = 0;
                }
            }
            object = object->next;
        }

        if ((set->flags & GAMEANIMSET_FLAG_NO_VISIBILITY_TEST) != any_special_no_visibility_test) {
            object = set->objects;
            while (object != NULL) {
                NuSpecialSetNoVisiTest(&object->special, set->flags & GAMEANIMSET_FLAG_NO_VISIBILITY_TEST);
                object = object->next;
            }
        }

        if ((set->flags & GAMEANIMSET_FLAG_NO_VISIBILITY_TEST) != 0) {
            set->state =
                all_playing_forward != 0 ? GAMEANIMSET_STATE_ACTIVE_FORWARD : GAMEANIMSET_STATE_ACTIVE_BACKWARD;
        } else {
            if (all_at_end != 0) {
                set->state = GAMEANIMSET_STATE_AT_END;
            } else if (all_at_start == 0) {
                set->state = GAMEANIMSET_STATE_BETWEEN_ENDPOINTS;
            }
            if (was_no_visibility_test != 0) {
                set->flags |= GAMEANIMSET_FLAG_STOP_REQUESTED;
            }
        }

        if ((set->flags & (GAMEANIMSET_FLAG_NO_VISIBILITY_TEST | GAMEANIMSET_FLAG_STOP_REQUESTED)) == 0) {
            GameAnimSet_RemoveFromSystemList(set);
        }
        set = next_set;
    }
}

CABLE_s cables[8];
CABLE_s *GameObjIsCableTied(GameObject_s *object) {
    if (cables[0].target != object)
        return NULL;

    if ((cables[0].flags_1e9 & 1) != 0)
        return &cables[0];
    if ((cables[1].flags_1e9 & 1) != 0)
        return &cables[1];
    if ((cables[2].flags_1e9 & 1) != 0)
        return &cables[2];
    if ((cables[3].flags_1e9 & 1) != 0)
        return &cables[3];
    if ((cables[4].flags_1e9 & 1) != 0)
        return &cables[4];
    if ((cables[5].flags_1e9 & 1) != 0)
        return &cables[5];
    if ((cables[6].flags_1e9 & 1) != 0)
        return &cables[6];
    if ((cables[7].flags_1e9 & 1) != 0)
        return &cables[7];
    return NULL;
}

void FindAnglesZX(NUVEC *, u16 *, u16 *);
f32 GetVehicleHoverHeight(GameObject_s *, f32 *);
u16 SeekRot(u16, u16, f32);
i32 RotDiff(u16, u16);
extern i16 id_BUGGY, id_RADIOCAR, id_POLICECAR, id_MOWER, id_ATST;

// Original 0x133040, 5,608 bytes. Suspension state shares the action packet.
void GameObjectRotation(GameObject_s *object, i32 mode) {
    APIOBJECT &api = object->apiobj;
    GAMECHARACTERDATA_s *game = api.character_data->game_character;
    u32 flags = game->flags_090;
    if ((flags & 0x200000) != 0) {
        static NUVEC buggy[4] = {
            {-0.1175f, 0, 0.1575f}, {0.1175f, 0, 0.1575f}, {0.1175f, 0, -0.1575f}, {-0.1175f, 0, -0.1575f}};
        static NUVEC radio[4] = {{-0.12f, 0, 0.08f}, {0.12f, 0, 0.08f}, {0.12f, 0, -0.08f}, {-0.12f, 0, -0.08f}};
        static NUVEC police[4] = {{-0.19f, 0, 0.31f}, {0.19f, 0, 0.31f}, {0.19f, 0, -0.31f}, {-0.19f, 0, -0.31f}};
        static NUVEC mower[4] = {
            {-0.1175f, 0, 0.155f}, {0.1175f, 0, 0.155f}, {0.1175f, 0, -0.155f}, {-0.1175f, 0, -0.155f}};
        static NUVEC standard[4] = {{-0.2f, 0, 0.2f}, {0.2f, 0, 0.2f}, {0.2f, 0, -0.2f}, {-0.2f, 0, -0.2f}};
        NUVEC *offsets = object->id == id_BUGGY       ? buggy
                         : object->id == id_RADIOCAR  ? radio
                         : object->id == id_POLICECAR ? police
                         : object->id == id_MOWER     ? mower
                                                      : standard;
        const f32 dy = api.position.y - api.start_position.y;
        const f32 vertical_speed = dy / FRAMETIME;
        NUVEC points[4];
        for (i32 i = 0; i < 4; ++i) {
            NuVecRotateY(&points[i], &offsets[i], api.field_0x276);
            NuVecAdd(&points[i], &points[i], &api.lower_position);
            const f32 floor = GameShadow(NULL, &points[i], 5.0f, -1);
            if (floor == 2000000.0f) {
                object->suspension[i].height = api.lower_position.y;
                object->suspension[i].velocity = dy / FRAMETIME;
            } else {
                object->suspension[i].velocity += -5.0f * FRAMETIME;
                const f32 height = object->suspension[i].height + dy + object->suspension[i].velocity * FRAMETIME;
                if (floor > height) {
                    object->suspension[i].height = floor;
                    object->suspension[i].velocity = vertical_speed;
                } else {
                    object->suspension[i].height = height;
                }
                const f32 lower = api.lower_position.y - (api.field_0x1dc + api.field_0x1dc);
                if (lower > object->suspension[i].height) {
                    object->suspension[i].height = lower;
                    object->suspension[i].velocity = vertical_speed;
                } else {
                    const f32 upper = api.lower_position.y + 3.0f * api.field_0x1dc;
                    if (object->suspension[i].height > upper) {
                        object->suspension[i].height = upper;
                        object->suspension[i].velocity = vertical_speed;
                    }
                }
            }
        }
        NUVEC normal = v000;
        for (i32 i = 0; i < 4; ++i) {
            NUVEC a = points[i], b = points[(i + 1) & 3], c = points[(i + 2) & 3];
            a.y = object->suspension[i].height;
            b.y = object->suspension[(i + 1) & 3].height;
            c.y = object->suspension[(i + 2) & 3].height;
            NUVEC ab, ac;
            NuVecSub(&ab, &b, &a);
            NuVecSub(&ac, &c, &a);
            NuVecCross(&ab, &ab, &ac);
            NuVecAdd(&normal, &normal, &ab);
        }
        NuVecScale(&normal, &normal, 0.25f);
        if (normal.y > 0.0f)
            NuVecNorm(&object->suspension_normal, &normal);
        game = api.character_data->game_character;
        flags = game->flags_090;
    }
    if ((api.field_0x1f8 & 4) == 0) {
        if ((flags & 0x200000) != 0) {
            FindAnglesZX(&object->suspension_normal, &object->field_0x1062, &object->field_0x1064);
        } else if (object->surface_normal.x != 0.0f || object->surface_normal.y != 1.0f ||
                   object->surface_normal.z != 0.0f) {
            FindAnglesZX(&object->surface_normal, &object->field_0x1062, &object->field_0x1064);
        } else {
            object->field_0x1062 = object->field_0x1064 = 0;
        }
        game = api.character_data->game_character;
        flags = game->flags_090;
    }
    u16 x = object->field_0x1062, z = object->field_0x1064;
    temp_xrot = x;
    temp_zrot = z;
    const bool sliding = (flags & 0x200) != 0 && object->character_context == 0x33;
    const bool magnet = !sliding && (flags & 0x10000000) != 0 && object->character_context == 0x44;
    const bool grapple = !sliding && object->character_context == 0x46 && object->field_0x7a3 == 0;
    i32 alignment = 0;
    if (object->field_0x1086 == 2 &&
        ((api.character_data->model_flags & 0x100) != 0 || (flags & 0x200000) != 0 || sliding)) {
        const f32 hover =
            (api.character_data->model_flags & 0x2000) != 0 ? GetVehicleHoverHeight(object, NULL) : game->field_0x28;
        if (hover > 0.0f) {
            alignment = 2;
            if (api.field_0x218 == 2000000.0f) {
                x = z = 0;
            } else {
                const f32 height = api.collision_min.y - api.field_0x218;
                f32 fraction;
                if (hover > height)
                    fraction = 1.0f;
                else if (2.5f * hover > height)
                    fraction = 1.0f - (height - hover) / (2.5f * hover - hover);
                else
                    fraction = 0.0f;
                x = static_cast<i32>(RotDiff(0, static_cast<u16>(temp_xrot)) * fraction);
                z = static_cast<i32>(RotDiff(0, static_cast<u16>(temp_zrot)) * fraction);
            }
        } else {
            alignment = 1;
            if (api.field_0x27d == 0 && (api.character_data->game_character->flags_090 & 0x200000) == 0 && !sliding) {
                alignment = 0;
                if ((api.character_data->model_flags & 0x100) != 0 &&
                    api.character_data->game_character->field_0x28 == 0.0f &&
                    (api.character_data->game_character->flags_090 & 0x100) != 0 && api.field_0x218 != 2000000.0f) {
                    alignment = api.field_0x1e0 > api.collision_min.y - api.field_0x218;
                }
            }
        }
        if (object->id == id_ATST && VehicleArea == 0) {
            x = static_cast<i32>(RotDiff(0, static_cast<u16>(temp_xrot)) * 0.5f);
            z = static_cast<i32>(RotDiff(0, static_cast<u16>(temp_zrot)) * 0.5f);
        }
    }
    const bool reflection = (api.character_data->model_flags & 0x2000) != 0 &&
                            api.character_data->game_character->field_0x28 > 0.0f &&
                            (api.field_0x27f == 0x10 || api.field_0x27f == 7) && api.field_0x27c == -1;
    if (mode == 1) {
        object->field_0x105e = x;
        object->field_0x1060 = z;
        if (alignment == 0) {
            api.pitch_angle = api.roll_angle = 0;
        } else if (magnet) {
            if (static_cast<u16>(object->context_animation - 5) <= 1)
                api.pitch_angle = api.roll_angle = 0;
            else {
                api.pitch_angle = object->magnet_surface_angle;
                api.roll_angle = object->grapple_swing_phase;
            }
        } else if (grapple) {
            NUVEC direction;
            NuVecSub(&direction, &static_cast<GRAPPLE_s *>(object->field_0x788)->hook_position,
                     &api.collision_position);
            FindAnglesZX(&direction, &api.pitch_angle, &api.roll_angle);
        } else if (reflection) {
            api.pitch_angle = object->field_0x1068;
            api.roll_angle = object->field_0x106a;
        } else if (alignment == 2 || object->surface_normal.y > NuTrigTable[0x3000]) {
            api.pitch_angle = x;
            api.roll_angle = z;
        } else {
            api.pitch_angle = api.roll_angle = 0;
        }
    } else if (mode == 2) {
        if (object->field_0x105e != object->field_0x1062)
            object->field_0x105e = SeekRot(object->field_0x105e, x, 8.0f);
        if (object->field_0x1060 != object->field_0x1064)
            object->field_0x1060 = SeekRot(object->field_0x1060, z, 8.0f);
        if (object->field_0x1086 != 2)
            return;
        if (magnet) {
            if (static_cast<u16>(object->context_animation - 5) <= 1) {
                api.pitch_angle = api.roll_angle = x = z = 0;
            } else {
                x = object->magnet_surface_angle;
                z = object->grapple_swing_phase;
            }
        } else if (grapple) {
            NUVEC direction;
            NuVecSub(&direction, &static_cast<GRAPPLE_s *>(object->field_0x788)->hook_position,
                     &api.collision_position);
            FindAnglesZX(&direction, &x, &z);
        } else if (reflection) {
            x = object->field_0x1068;
            z = object->field_0x106a;
        } else if (alignment != 2 && (alignment == 0 || !(object->surface_normal.y > NuTrigTable[0x3000]))) {
            x = z = 0;
        }
        const f32 speed = object->id == id_ATST && VehicleArea == 0 ? 3.0f : 8.0f;
        if (api.pitch_angle != x)
            api.pitch_angle = SeekRot(api.pitch_angle, x, speed);
        if (api.roll_angle != z)
            api.roll_angle = SeekRot(api.roll_angle, z, speed);
    }
}

// Original: reads the user's music volume from the options save as the product
// of two 0..10 sliders scaled to 0..1 (option bytes at 0x4 and 0x5).
f32 GameGetMusicVolume(OPTIONSSAVE_s *options) {
    return ((f32)(u8)options->field5_0x5 / 10.0f) * ((f32)(u8)options->field4_0x4 / 10.0f);
}

// Original: applies GameGetMusicVolume, zeroing it while the title logos are
// up (SuperOptions.music_enabled == 0 on the titles level); the title menu restores
// the user's volume via GameGetMusicVolume once the menu phase starts.
f32 GameSetMusicVolume(OPTIONSSAVE_s *options) {
    f32 volume = GameGetMusicVolume(options);
    if (SuperOptions.music_enabled == 0 && WORLD->current_level == TITLES_LDATA) {
        volume = 0.0f;
    }
    legoSetMusicVolume(volume);
    return volume;
}

void GameAISysStartFrame(AISYS_s *system) {
    if (system == NULL || netclient != 0) {
        return;
    }

    if (system->path_sys != NULL && system->path_sys->path_count != 0) {
        for (i32 index = 0; index < system->path_sys->path_count; ++index) {
            memset(&system->path_sys->paths[index]->updated_node_bits[0], 0, 0x20);
            memmove(system->path_sys->paths[index]->previous_inside_node_bits,
                    system->path_sys->paths[index]->inside_node_bits, 0x20);
            memset(system->path_sys->paths[index]->inside_node_bits, 0,
                   sizeof(system->path_sys->paths[index]->inside_node_bits));
        }

        // Moving specials can carry path endpoints. Update both ends of each
        // connection; AIPathNodeUpdatePos uses updated_node_bits to ensure a
        // shared endpoint is transformed only once this frame.
        for (i32 path_index = 0; path_index < system->path_sys->path_count; ++path_index) {
            AIPATH *path = system->path_sys->paths[path_index];
            for (i32 connection_index = 0; connection_index < path->connection_count; ++connection_index) {
                AIPATHCNX *connection = &path->connections[connection_index];
                AIPATHNODE *node = &path->nodes[connection->direction_a];
                if (node->has_special != 0) {
                    AIPathNodeUpdatePos(system, path, node);
                }
                node = &path->nodes[connection->direction_b];
                if (node->has_special != 0) {
                    AIPathNodeUpdatePos(system, path, node);
                }
            }
        }
    }

    // Spread the object/area overlap work across frames. Area runtime flags
    // summarize the kinds of live characters inside it, while every object
    // retains a 64-area occupancy mask for script queries.
    if (system->next_area_check < system->area_count) {
        AIAREA *area = &system->areas[system->next_area_check];
        const i32 area_index = static_cast<i32>(area - WORLD->ai_sys->areas);
        const u64 area_bit = 1ULL << area_index;
        area->runtime_flags &= static_cast<u8>(
            ~(AIAREA_RUNTIME_PLAYER_PRESENT | AIAREA_RUNTIME_OBJECT_STATE_CLEAR | AIAREA_RUNTIME_OBJECT_STATE_SET));

        GameObject_s *object = Obj;
        for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index, ++object) {
            if ((object->apiobj.field_0x1f8 & (APIOBJECT_FLAG_CHARACTER | APIOBJECT_FLAG_IN_USE)) !=
                (APIOBJECT_FLAG_CHARACTER | APIOBJECT_FLAG_IN_USE)) {
                continue;
            }
            if (object->apiobj.field_0x287 != 0 && !(object->field_0x101c > 0.0f) &&
                static_cast<i8>(object->apiobj.flags_low) >= 0) {
                continue;
            }

            NUVEC local_position;
            NuVecSub(&local_position, &object->apiobj.position, &area->position);
            NuVecRotateY(&local_position, &local_position, -area->rotation);
            const bool is_inside = local_position.x >= -area->half_width && local_position.y >= -0.1f &&
                                   local_position.z >= -area->half_depth && local_position.x <= area->half_width &&
                                   local_position.y <= area->height && local_position.z <= area->half_depth;
            if (!is_inside) {
                object->apiobj.ai_area_mask_low &= ~static_cast<u32>(area_bit);
                object->apiobj.ai_area_mask_high &= ~static_cast<u32>(area_bit >> 32);
                continue;
            }

            object->apiobj.ai_area_mask_low |= static_cast<u32>(area_bit);
            object->apiobj.ai_area_mask_high |= static_cast<u32>(area_bit >> 32);
            if ((object->apiobj.flags_low & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0) {
                area->runtime_flags |= AIAREA_RUNTIME_PLAYER_PRESENT;
            }
            if (object->apiobj.field_0x27c != -1) {
                area->runtime_flags |= AIAREA_RUNTIME_CHARACTER_SLOT_SEEN;
            }
            if ((object->apiobj.field_0x1f4 & 1) != 0) {
                area->runtime_flags |= AIAREA_RUNTIME_OBJECT_STATE_SET;
            } else if ((object->apiobj.field_0x1f4 & 4) == 0) {
                area->runtime_flags |= AIAREA_RUNTIME_OBJECT_STATE_CLEAR;
            }
        }

        ++system->next_area_check;
        if (system->next_area_check >= system->area_count) {
            system->next_area_check = 0;
        }
    }

    // Level scripts are ordinary AI processors without an object or packet.
    // Disabled processors retain their state but do not advance this frame.
    for (i32 index = 0; index < WORLD->processor_count; ++index) {
        if (!WORLD->processors[index].processor.is_disabled) {
            AIScriptProcess(system, NULL, NULL, &WORLD->processors[index].processor, FRAMETIME);
        }
    }
}

void GameDisplaySettings(LEVELDATADISPLAY *display, i32 *background_colours) {
    CUTINFO *cut = static_cast<CUTINFO *>(CutStopInfo);
    if (cut != NULL && cut->camera_near_clip != 0.0f) {
        pNuCam->near_clip = cut->camera_near_clip;
    } else {
        pNuCam->near_clip = display->unknown_04;
    }

    u16 far_clip;
    if (cut != NULL) {
        far_clip = cut->camera_far_clip;
        if (far_clip == 0) {
            far_clip = static_cast<u16>(display->unknown_14);
        }
    } else {
        far_clip = static_cast<u16>(display->unknown_14);
    }
    pNuCam->far_clip = static_cast<f32>(static_cast<u32>(far_clip));

    LEVELDATA *level = WORLD->current_level;
    const bool use_backdrop = level == TITLES_LDATA || (level->flags & LEVEL_STATUS) != 0 || level == STATUS_LDATA ||
                              level == CREDITS_LDATA || MainRenderTime < 0.0f;
    if (!use_backdrop) {
        background_colours[0] =
            0x80000000u | static_cast<u8>(display->bg_red_top) |
            (static_cast<u8>(display->bg_green_top) << 8 | static_cast<u8>(display->bg_blue_top) << 16);
        background_colours[1] =
            0x80000000u | static_cast<u8>(display->bg_red_bottom) |
            (static_cast<u8>(display->bg_green_bottom) << 8 | static_cast<u8>(display->bg_blue_bottom) << 16);
        return;
    }

    BackDrop_UpdateColours(0);

    i32 top_g = static_cast<i32>(backdrop_top_g);
    i32 top_r = static_cast<i32>(backdrop_top_r);
    i32 top_b = static_cast<i32>(backdrop_top_b);
    i32 bottom_r = static_cast<i32>(backdrop_bot_r);
    i32 bottom_g = static_cast<i32>(backdrop_bot_g);
    i32 bottom_b = static_cast<i32>(backdrop_bot_b);

    f32 alpha = 1.0f;
    if (BackDrop_AlphaFn != NULL) {
        BackDrop_AlphaFn(&alpha);
        if (alpha != 0.0f) {
            top_r = static_cast<i32>(static_cast<f32>(top_r) * alpha);
            top_g = static_cast<i32>(static_cast<f32>(top_g) * alpha);
            top_b = static_cast<i32>(static_cast<f32>(top_b) * alpha);
            bottom_r = static_cast<i32>(static_cast<f32>(bottom_r) * alpha);
            bottom_g = static_cast<i32>(static_cast<f32>(bottom_g) * alpha);
            bottom_b = static_cast<i32>(static_cast<f32>(bottom_b) * alpha);
        }
    }

    background_colours[0] = 0x80000000u | (top_r & 0xff) | (top_g & 0xff) << 8 | (top_b & 0xff) << 16;
    background_colours[1] = 0x80000000u | (bottom_r & 0xff) | (bottom_g & 0xff) << 8 | (bottom_b & 0xff) << 16;
}

u8 grapple_attach_frames = 5;
void GameObjectSetCanUse(GameObject_s *object, void *target, unsigned char action, unsigned char, float parameter) {
    object->can_use_object = target;
    object->use_action_parameter = parameter;
    object->use_action = action;
    object->use_action_frames = grapple_attach_frames;
}

void GameObjOwnsAnyCables(GameObject_s *) {
}

void GameObjectDimensionsExtra_LSW(GameObject_s *object);

void GameObjectDimensions(GameObject_s *object) {
    APIOBJECT &api = object->apiobj;
    PLAYERCHARACTERCONFIG_s *config = api.character_data->player_config;
    const i32 collision_origin_joint = config->collision_origin_joint;
    if (object->use_model_origin != 0 && object->field_0xd24 == 1.0f && collision_origin_joint != -1 &&
        api.character_model->points_of_interest[collision_origin_joint] != NULL && api.field_0x288 != 0) {
        const f32 radius = object->field_0x1004 * config->collision_origin_radius;
        api.field_0x1dc = radius;
        api.field_0x1e0 = radius;
    } else {
        api.field_0x1dc = api.collision_radius;
        api.field_0x1e0 = api.collision_height;
    }
    GameObjectDimensionsExtra_LSW(object);
}

void GameAntiNodeData_Init(GAMEANTINODEDATA_s *data, nuhspecial_s *) {
    if (data != NULL) {
        memset(data, 0, sizeof(*data));
    }
}

void GameAntiNodeData_Read(GAMEANTINODEDATA_s *data) {
    if (EdFileReadChar() == 0) {
        return;
    }
    EdFileReadNuVec(&data->position);
    data->radius = EdFileReadFloat();
    data->min_y = EdFileReadFloat();
    data->max_y = EdFileReadFloat();
    data->extent_x = EdFileReadFloat();
    data->extent_z = EdFileReadFloat();
    data->flags = EdFileReadShort();
    data->use_largest_extent = static_cast<u8>(EdFileReadChar());
    data->mode = static_cast<u8>(EdFileReadChar());
}

extern "C" {
    extern NUVEC nusound_special_positions[5];
    void PlaySfxById(i32 sfx_id, nuvec_s *position);
}

void GameAudio_PlaySfxById(i32 sfx_id, nuvec_s *position, i32 flags, i32) {
    if (flags == 0) {
        PlaySfxById(sfx_id, position);
        return;
    }
    if ((flags & ~2) == 1) {
        nusound_special_positions[1] = *position;
        PlaySfxById(sfx_id, &nusound_special_positions[1]);
    }
    flags -= 2;
    if (static_cast<u32>(flags) <= 1) {
        nusound_special_positions[2] = *position;
        PlaySfxById(sfx_id, &nusound_special_positions[2]);
    }
}

void Game_GotAllGoldBricks() {
}

APIOBJECT *GameAPIOBJECTFromObjID(u8 object_id) {
    if (object_id >= HIGHGAMEOBJECT) {
        return NULL;
    }

    GameObject_s *object = &Obj[object_id];
    if ((object->apiobj.flags_low & APIOBJECT_FLAG_IN_USE) == 0) {
        return NULL;
    }

    if ((object->apiobj.flags_high & APIOBJECT_HIGH_FLAG_PLAYER_CHARACTER) == 0 &&
        object->ai.reset_mode != AI_OBJECT_ROUTE_STATE_SCRIPT_VISIBLE) {
        return NULL;
    }
    if (object->apiobj.field_0x287 != 0 && !(object->field_0x101c > 0.0f)) {
        return NULL;
    }
    return &object->apiobj;
}

i32 GameDrawCharacterModel(CHARACTERMODEL_s *model, ANIMPACKET_s *animation, NUMTX *matrix, NUMTX *secondary_matrix,
                           NUMTX *reflection_matrix, NUMTX *auxiliary_matrix, GameObject_s *object, u32 flags) {
    if (model == NULL) {
        return 0;
    }

    drawcharactermodel_keepmergeaction = game_keepmergeaction;
    MakeLayerList = GCDataList[model->model_id].make_layer_list;

    CHARACTERDATA *character_data =
        object != NULL ? object->apiobj.character_data : &apicharsys->char_data[model->model_id];

    // The original reserves a fixed 256-matrix evaluation array in this
    // wrapper before calling APIDrawCharacterModel.
    NUMTX output_matrices[256];
    return APIDrawCharacterModel(model, character_data, animation, matrix, secondary_matrix, reflection_matrix, 0,
                                 auxiliary_matrix, object, flags, NULL, 0, WORLD, FRAMETIME, output_matrices, 0, NULL);
}

u32 AdjustLayerBits(u32, GameObject_s *);
extern i16 id_ANAKINJEDISCARRED;

void GameObjectToCameraCode(GameObject_s *object) {
    if (GameTimer.update_count == 0) {
        object->shadow_opacity = 0.0f;
        return;
    }
    f32 distance = object->ai_update_distance;
    LEVELDATA *level = WORLD->current_level;
    i32 far_distance = static_cast<u8>(level->blob_shadow_fade_far);
    if (distance < static_cast<f32>(far_distance)) {
        u32 near_distance = static_cast<u8>(level->blob_shadow_fade_near);
        if (distance > static_cast<f32>(near_distance))
            object->shadow_opacity = 1.0f - (distance - static_cast<f32>(near_distance)) /
                                                static_cast<f32>(static_cast<i32>(far_distance - near_distance));
        else
            object->shadow_opacity = 1.0f;
    } else {
        object->shadow_opacity = 0.0f;
    }
    GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    if ((data->flags_094[0] & 0x10) != 0)
        return;
    u32 mask;
    if (object->apiobj.field_0x287 != 0) {
        GAMECHARACTERDATA *model_data = static_cast<GAMECHARACTERDATA *>(
            apicharsys->char_data[object->apiobj.character_model->model_id].field11_0x24);
        mask = model_data->layer_mask_dead;
        if (mask != 0)
            goto apply_layers;
    }
    if (object->id == id_ANAKINJEDISCARRED)
        mask = data->layer_mask_special;
    else if (g_lowEndLevelBehaviour == 0 || (object->apiobj.flags_low & 0x80) != 0) {
        if (distance > data->field_0xb8)
            mask = data->layer_mask_low;
        else if (distance > data->field_0xb4)
            mask = data->layer_mask_medium;
        else
            mask = data->layer_mask;
    } else {
        if (data->field_0xb4 > distance)
            mask = data->layer_mask_medium;
        else
            mask = data->layer_mask_low;
    }
apply_layers:
    object->field_0x1054 = mask | object->extra_layer_mask;
    object->field_0x1054 = AdjustLayerBits(object->field_0x1054, object);
}

void GameRegisterGizActions() {
    RegisterGizActions(game_gizactiondefs);
}

i32 GameAudio_GetPlrSfxBits(void *object_ptr) {
    APIOBJECT *object = static_cast<APIOBJECT *>(object_ptr);
    i32 sfx_bits = 0;
    if (object != NULL && static_cast<i8>(object->flags_low) < 0) {
        sfx_bits = 1 << object->field_0x27c;
    }
    return sfx_bits;
}

void GameBlowUpBlownUpFn_LSW(GIZMOBLOWUP_s *) {
}

void GameLoadCharacterModels(APICHARACTERMODELLIST_s *list, i32 append, VARIPTR *buf, VARIPTR *buf_end, i32 area_models,
                             i32 area) {
    if (area_models != 0 && CutScenePlayer_Active() != 0 && area != -1 && &ADataList[area] != HUB_ADATA) {
        area_models = 0;
    }

    APILoadCharacterModels(list, append, buf, *buf_end, area_models);
}

i32 Game_100PercentComplete() {
    if (Game_CompletionSave == NULL) {
        return 0;
    }
    return reinterpret_cast<STATUSCOLLECT_s *>(Game_CompletionSave)->flags & 1;
}

void Game_WorldInfo_InitMenu(WORLDINFO_s *world, i32 *menu_id, i32 *) {
    if (world->current_level == TITLES_LDATA) {
        *menu_id = 0;
    } else if (world->current_level == CREDITS_LDATA) {
        *menu_id = 30;
    }
}

void GameObjectToCameraDistances() {
    const NUVEC camera_position = GameCam->pos;
    GameObject_s *object = Obj;
    for (i32 object_index = 0; object_index < HIGHGAMEOBJECT; ++object_index, ++object) {
        const u16 required_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER;
        if ((object->apiobj.field_0x1f8 & required_flags) != required_flags) {
            continue;
        }

        const f32 dx = camera_position.x - object->apiobj.position.x;
        const f32 dy = camera_position.y - object->apiobj.position.y;
        const f32 dz = camera_position.z - object->apiobj.position.z;
        object->ai_update_distance = NuFsqrt(dx * dx + dy * dy + dz * dz);
    }
}

extern GameObject_s *alert_obj;
extern NUVEC alert_pos;
extern f32 alert_timer;
extern "C" {
    i32 party_under_cover;
}
i32 ZapTarget(GameObject_s *object);

void GameCreatureOpponentSelection(AISYS_s *system, i32 count, APIOBJECT_s **objects, i32 goody_count,
                                   APIOBJECT_s **goodies, i32 baddy_count, APIOBJECT_s **baddies, u64 awareness,
                                   float) {
    NUVEC difference = {0.0f, 0.0f, 0.0f};
    if (system == NULL)
        return;
    if (goody_count == 0)
        return;
    f32 distance = 0.0f;
    i32 complete = baddy_count == 0;
    if (alert_obj != NULL && (alert_obj->apiobj.flags_high & 0x10) != 0 && (alert_obj->apiobj.field_0x1f4 & 5) == 0 &&
        (static_cast<GAMECHARACTERDATA *>(alert_obj->apiobj.character_data->field11_0x24)->flags_090 & 0x8000) == 0) {
        const u64 alert_mask = (u64)1 << alert_obj->apiobj.field_0x289;
        for (i32 i = 0; i < baddy_count; ++i) {
            APIOBJECT_s *object = baddies[i];
            if ((object->ai_awareness_mask & alert_mask) != 0) {
                object->objptr->alert_target_timer = 5.0f;
                object->objptr->alert_target = &alert_obj->apiobj;
            } else if (WORLD->rooms_visible_ptr[object->objptr->room_id] != 0) {
                difference.x = alert_pos.x - object->collision_position.x;
                if (object->heardistance > difference.x) {
                    difference.z = alert_pos.z - object->collision_position.z;
                    if (object->heardistance > difference.z) {
                        difference.y = alert_pos.y - object->collision_position.y;
                        if (object->maxviewheight > difference.y && difference.y > object->minviewheight) {
                            distance = difference.x * difference.x + difference.z * difference.z;
                            if (!(object->heardistance * object->heardistance > distance))
                                continue;
                            const f32 dx = alert_obj->apiobj.collision_position.x - object->collision_position.x;
                            const f32 dz = alert_obj->apiobj.collision_position.z - object->collision_position.z;
                            if ((WORLD->api_object_sys->line_of_sight[object->field_0x289] & alert_mask) != 0 ||
                                dx * dx + dz * dz > object->viewdistance * object->viewdistance) {
                                object->objptr->alert_target = &alert_obj->apiobj;
                                object->ai_awareness_mask |= alert_mask;
                                awareness |= object->ai_awareness_mask;
                                object->objptr->alert_target_timer = 5.0f;
                            }
                        }
                    }
                }
            }
        }
        alert_timer -= FRAMETIME;
        if (!(alert_timer > 0.0f))
            alert_obj = NULL;
    }
    if (system->goody_idx >= goody_count)
        system->goody_idx = 0;
    i32 budget = 32;
    // Pending choices survive budget exhaustion. Publish only after a pass
    // finishes without changing the baddies' assignments.
    while (!complete && budget > 0) {
        complete = 0;
        while (system->goody_idx < goody_count && budget > 0) {
            APIOBJECT_s *goody = goodies[system->goody_idx];
            f32 best_distance = 1.0e9f;
            APIOBJECT_s *best_baddy = NULL;
            for (i32 i = 0; i < baddy_count; ++i) {
                APIOBJECT_s *baddy = baddies[i];
                if (baddy == goody || baddy->ai->pending_opponent == goody)
                    continue;
                i32 have_distance = 0;
                if (baddy->objptr->field_0xefc & 2) {
                    goody->ai_awareness_mask &= ~((u64)1 << baddy->field_0x289);
                } else if ((goody->ai_awareness_mask >> baddy->field_0x289) & 1) {
                    distance = NuVecDist(&baddy->position, &goody->position, &difference);
                    have_distance = 1;
                    if (distance >= goody->viewdistance + baddy->visibility_range_extension)
                        goody->ai_awareness_mask &= ~((u64)1 << baddy->field_0x289);
                } else if ((WORLD->api_object_sys->line_of_sight[goody->field_0x289] >> baddy->field_0x289) & 1) {
                    distance = NuVecDist(&baddy->position, &goody->position, &difference);
                    have_distance = 1;
                    if ((!party_under_cover || ((goody->field_0x1f4 & 0x400) && (baddy->field_0x1f4 & 0x400))) &&
                        (((awareness >> baddy->field_0x289) & 1) ||
                         (((WORLD->api_object_sys->line_of_sight[goody->field_0x289] >> baddy->field_0x289) & 1) &&
                          ((goody->flags_high & 8) ||
                           difference.x * goody->objptr->facing_direction.x +
                                   difference.z * goody->objptr->facing_direction.z >=
                               0.0f ||
                           (goody->heardistance > distance && (baddy->objptr->field_0xef9 & 8)))))) {
                        goody->ai_awareness_mask |= (u64)1 << baddy->field_0x289;
                        awareness |= goody->ai_awareness_mask;
                    }
                }
                if (goody->objptr->field_0xefc & 2) {
                    baddy->ai_awareness_mask &= ~((u64)1 << goody->field_0x289);
                } else if (baddy->objptr->alert_target == goody) {
                    baddy->ai_awareness_mask |= (u64)1 << goody->field_0x289;
                    awareness |= baddy->ai_awareness_mask;
                } else if ((baddy->ai_awareness_mask >> goody->field_0x289) & 1) {
                    if (!have_distance)
                        distance = NuVecDist(&baddy->position, &goody->position, &difference);
                    have_distance = 1;
                    if (distance >= baddy->viewdistance + goody->visibility_range_extension)
                        baddy->ai_awareness_mask &= ~((u64)1 << goody->field_0x289);
                } else if ((WORLD->api_object_sys->line_of_sight[baddy->field_0x289] >> goody->field_0x289) & 1) {
                    if (!have_distance)
                        distance = NuVecDist(&baddy->position, &goody->position, &difference);
                    have_distance = 1;
                    if ((!party_under_cover || ((baddy->field_0x1f4 & 0x400) && (goody->field_0x1f4 & 0x400))) &&
                        (((awareness >> goody->field_0x289) & 1) ||
                         (((WORLD->api_object_sys->line_of_sight[baddy->field_0x289] >> goody->field_0x289) & 1) &&
                          ((baddy->flags_high & 8) ||
                           -difference.z * baddy->objptr->facing_direction.z -
                                   difference.x * baddy->objptr->facing_direction.x >=
                               0.0f ||
                           (baddy->heardistance > distance && (goody->objptr->field_0xef9 & 8)))))) {
                        baddy->ai_awareness_mask |= (u64)1 << goody->field_0x289;
                        awareness |= baddy->ai_awareness_mask;
                    }
                }
                if (!((goody->ai_awareness_mask >> baddy->field_0x289) & 1) &&
                    !((baddy->ai_awareness_mask >> goody->field_0x289) & 1))
                    continue;
                if (!have_distance)
                    distance = NuVecDist(&baddy->position, &goody->position, &difference);
                GameObject_s *baddy_object = baddy->objptr;
                i16 baddy_id = baddy_object->id;
                if (baddy_id == id_JAWA || baddy_id == id_UGNAUGHT) {
                    if (!ZapTarget(goody->objptr))
                        baddy->objptr->ai_opponent_exclusion_mask |= (u64)1 << goody->field_0x289;
                    baddy_object = baddy->objptr;
                    baddy_id = baddy_object->id;
                } else if (goody->character_data->model_flags & 0x80000) {
                    baddy_object->ai_opponent_exclusion_mask |= (u64)1 << goody->field_0x289;
                } else if ((baddy_object->field_0xefb & 1) && goody->objptr != player && goody->objptr != player2) {
                    baddy_object->ai_opponent_exclusion_mask |= (u64)1 << goody->field_0x289;
                } else if (goody->objptr->id == id_DRAGBOMB && baddy_id != id_ATAT) {
                    baddy_object->ai_opponent_exclusion_mask |= (u64)1 << goody->field_0x289;
                }
                GameObject_s *goody_object = goody->objptr;
                if (!((baddy_object->ai_opponent_exclusion_mask >> goody->field_0x289) & 1)) {
                    AIPACKET *packet = baddy->ai;
                    if (packet->pending_nearest_metric > distance) {
                        packet->pending_nearest_metric = distance;
                        packet->pending_nearest_opponent = goody;
                    }
                    f32 metric = distance;
                    if (packet->opponent_object == goody)
                        metric -= 0.5f;
                    if ((baddy->ai_awareness_mask >> goody->field_0x289) & 1) {
                        if (packet->pending_opponent == NULL) {
                            if (!(packet->runtime_flags & 2) ||
                                ((WORLD->api_object_sys->line_of_sight[baddy->field_0x289] >> goody->field_0x289) &
                                 1) ||
                                baddy_object->alert_target == goody) {
                                if (best_distance > metric) {
                                    best_distance = distance;
                                    best_baddy = baddy;
                                }
                            }
                        } else if ((goody->flags_low & 0x80) && (baddy_object->field_0xefb & 0x40)) {
                            if (!(packet->pending_opponent->flags_low & 0x80) ||
                                packet->pending_opponent_metric > metric) {
                                best_distance = distance;
                                best_baddy = baddy;
                            }
                        }
                    }
                }
                if (baddy_id == id_JAWA || baddy_id == id_UGNAUGHT || (baddy->character_data->model_flags & 0x80000))
                    goody_object->ai_opponent_exclusion_mask |= (u64)1 << baddy->field_0x289;
                if (!((goody_object->ai_opponent_exclusion_mask >> baddy->field_0x289) & 1)) {
                    AIPACKET *packet = goody->ai;
                    if (packet->pending_nearest_metric > distance) {
                        packet->pending_nearest_metric = distance;
                        packet->pending_nearest_opponent = baddy;
                    }
                    f32 metric = distance;
                    if (packet->opponent_object == baddy)
                        metric -= 0.5f;
                    if (((goody->ai_awareness_mask >> baddy->field_0x289) & 1) &&
                        (!(packet->runtime_flags & 2) ||
                         ((WORLD->api_object_sys->line_of_sight[goody->field_0x289] >> baddy->field_0x289) & 1))) {
                        if (packet->pending_opponent_metric > metric ||
                            (packet->pending_opponent != NULL && packet->pending_opponent->objptr != NULL &&
                             packet->pending_opponent->objptr->character_context == 0x5a &&
                             baddy_object->character_context != 0x5a)) {
                            packet->pending_opponent_metric = distance;
                            packet->pending_opponent = baddy;
                        }
                    }
                }
            }
            if (baddy_count > 0) {
                budget -= baddy_count;
                if (best_baddy != NULL && best_baddy->ai->pending_opponent != goody) {
                    best_baddy->ai->pending_opponent = goody;
                    best_baddy->ai->pending_opponent_metric = best_distance;
                    system->unknown_flag_2 = 1;
                    if (!((goody->objptr->ai_opponent_exclusion_mask >> best_baddy->field_0x289) & 1) &&
                        ((goody->ai_awareness_mask >> best_baddy->field_0x289) & 1)) {
                        AIPACKET *packet = goody->ai;
                        if (!(packet->field_0x1e5 & 0x10) || packet->pending_opponent_metric > best_distance) {
                            packet->pending_opponent = best_baddy;
                            packet->pending_opponent_metric = best_distance;
                            packet->field_0x1e5 |= 0x10;
                        }
                    }
                }
            }
            ++system->goody_idx;
            if (system->goody_idx >= goody_count) {
                system->goody_idx = 0;
                if (system->unknown_flag_2) {
                    system->unknown_flag_2 = 0;
                    complete = 0;
                } else {
                    complete = 1;
                }
            }
        }
    }
    if (!complete)
        return;
    system->goody_idx = 0;
    nbaddies_can_see_players = 0;
    // Losing LOS alone does not erase the seen history; losing awareness does.
    for (i32 i = 0; i < goody_count; ++i) {
        APIOBJECT_s *goody = goodies[i];
        for (i32 j = 0; j < baddy_count; ++j) {
            APIOBJECT_s *baddy = baddies[j];
            if ((goody->ai_awareness_mask >> baddy->field_0x289) & 1) {
                if ((WORLD->api_object_sys->line_of_sight[goody->field_0x289] >> baddy->field_0x289) & 1)
                    goody->objptr->ai_seen_mask |= (u64)1 << baddy->field_0x289;
            } else if ((goody->objptr->ai_seen_mask >> baddy->field_0x289) & 1) {
                goody->objptr->ai_seen_mask &= ~((u64)1 << baddy->field_0x289);
            }
            if ((baddy->ai_awareness_mask >> goody->field_0x289) & 1) {
                if ((WORLD->api_object_sys->line_of_sight[baddy->field_0x289] >> goody->field_0x289) & 1)
                    baddy->objptr->ai_seen_mask |= (u64)1 << goody->field_0x289;
            } else if ((baddy->objptr->ai_seen_mask >> goody->field_0x289) & 1) {
                baddy->objptr->ai_seen_mask &= ~((u64)1 << goody->field_0x289);
            }
        }
    }
    for (i32 i = 0; i < count; ++i) {
        APIOBJECT_s *object = objects[i];
        AIPACKET *packet = object->ai;
        packet->opponent_metric = packet->pending_opponent_metric;
        packet->opponent_object = packet->pending_opponent;
        packet->field_0x1e5 = (packet->field_0x1e5 & ~8) | ((packet->field_0x1e5 >> 1) & 8);
        packet = object->ai;
        packet->nearest_opponent_metric = packet->pending_nearest_metric;
        packet->nearest_opponent_object = packet->pending_nearest_opponent;
        packet->pending_opponent_metric = 1.0e9f;
        packet->pending_nearest_metric = 1.0e9f;
        packet->field_0x1e5 &= ~0x10;
        packet->pending_nearest_opponent = NULL;
        packet->pending_opponent = NULL;
        if ((awareness >> object->field_0x289) & 1)
            object->ai->field_0x1e5 |= 0x40;
        else
            object->ai->field_0x1e5 &= ~0x40;
        object->objptr->field_0xef9 &= ~8;
        GameObject_s *game_object = object->objptr;
        game_object->ai_opponent_exclusion_mask = 0;
        if ((player != NULL && ((game_object->ai_seen_mask >> player->apiobj.field_0x289) & 1)) ||
            (player2 != NULL && ((game_object->ai_seen_mask >> player2->apiobj.field_0x289) & 1)))
            ++nbaddies_can_see_players;
        if (game_object->opponent != NULL) {
            packet = object->ai;
            packet->opponent_object = static_cast<APIOBJECT_s *>(game_object->opponent);
            packet->opponent_metric = NuVecDist(&packet->opponent_object->position, &object->position, &difference);
            packet = object->ai;
            if (packet->opponent_object->ai->opponent_object == object)
                packet->field_0x1e5 |= 8;
            else
                packet->field_0x1e5 &= ~8;
        }
    }
}

void GameObjectDimensionsExtra_LSW(GameObject_s *) {
}

i32 AnakinGreenSabre(GameObject_s *object);
extern "C" i16 id_THEEMPEROR, id_IMPERIALGUARD, id_BODYGUARD;
void NewRumble(nupad_s *, f32, i32);
i32 CannotKill(GameObject_s *object);
u16 ObjHitObj_Flags(GameObject_s *object);
i32 ObjHitObj(GameObject_s *, GameObject_s *, i32, u16, i32, i32);
void AddStreakPoints(NUVEC *, f32, u32, void **, i32, void *);
i32 SphereSphereOverlapScaleY(NUVEC *, f32, f32, NUVEC *, f32, f32);
GIZMOBLOWUP_s *GizmoBlowUp_Hit(GameObject_s *, NUVEC *, i32, f32, NUVEC *, NUVEC *, BOLT_s *, u32, u8 *);
i32 GizmoBlowupBlowup(GIZMOBLOWUP_s *, i32, i32, i32, GameObject_s *, i32);
void AlertSurroundingCreatures(GameObject_s *, NUVEC *);
extern "C" i32 AddGameDebrisRot(APIDEBRISSYS_s *, i32, NUVEC *, i32, i16, i16);

static void LightSabreStreakCode(GameObject_s *object, i32 blade, i32 effect) {
    if (object->weapon_scale < 1.0f)
        return;
    const i8 context = object->character_context;
    if ((context == 0 && (object->action_movement_state == 4 || object->action_movement_state == 2)) || context == 4 ||
        context == 16 || (context == 13 && object->id != id_THEEMPEROR) || context == 14 ||
        (context == 31 && object->field_0x7a3 == 1) ||
        ((object->apiobj.flags_low & 0x80) != 0 && Cheat_PowerUpActive(object->apiobj.field_0x27c))) {
        object->sabre_flags |= 2;
    }
    if (object->id == id_IMPERIALGUARD)
        object->sabre_flags &= ~3;
    if (object->sabre_flags == 0 || object->apiobj.field_0x288 == 0 || (object->field_0xe23 & 8) == 0)
        return;
    GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    const i32 joint_a = data->streak_joints[blade][0];
    const i32 joint_b = data->streak_joints[blade][1];
    if (joint_a == -1 || object->apiobj.character_model->points_of_interest[joint_a] == NULL || joint_b == -1 ||
        object->apiobj.character_model->points_of_interest[joint_b] == NULL)
        return;
    object->blade_states[blade] = -1;
    NUVEC points[3];
    points[0] = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint_a], 3);
    points[1] = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint_b], 3);
    if ((object->sabre_flags & 2) != 0) {
        i32 colour;
        if (object->apiobj.field_0x27c != -1 && Cheat_IsOn(25))
            colour = 0;
        else if (object->apiobj.field_0x27c != -1 && Player_HasPurpleForce(object))
            colour = 3;
        else if (object->id == id_GRIEVOUS && (blade == 3 || blade == 0))
            colour = 2;
        else if (AnakinGreenSabre(object) || (object->id == id_BOB && (object->field_0xefd & 2) != 0))
            colour = 1;
        else
            colour = static_cast<i8>(GCDataList[object->id].field_0x117);
        object->blade_states[blade] = colour;
        if (object->apiobj.model_draw_result != 0) {
            const u8 *rgb = BladeTab[colour].colour;
            const u32 packed = 0xff000000u | rgb[0] | (rgb[1] << 8) | (rgb[2] << 16);
            AddStreakPoints(points, 0.25f, packed, &object->sabre_streaks[blade][0], 0, object);
            if (object->field_0x1087 != 0 && object->field_0x1020 != 2000000.0f) {
                f32 plane = object->field_0x1020;
                if (WORLD->current_level->unknown_0cc != 2000000.0f)
                    plane = WORLD->current_level->unknown_0cc;
                NUVEC reflected[2] = {points[0], points[1]};
                reflected[0].y = plane - (reflected[0].y - plane);
                reflected[1].y = plane - (reflected[1].y - plane);
                AddStreakPoints(reflected, 0.25f, packed, &object->sabre_streaks[blade][1], 1, object);
            }
        }
    }
    if ((object->sabre_flags & 7) == 0)
        return;
    if (object->apiobj.field_0x27c != -1 && Cheat_IsOn(25))
        effect = 1;
    else if (object->apiobj.field_0x27c != -1 && Player_HasPurpleForce(object))
        effect = 4;
    else if (object->id == id_GRIEVOUS)
        effect = (blade == 3 || blade == 0) ? 3 : 2;
    if (data->field275_0x116 == 12 && context == 5 && object->combo_stage == 2 && object->context_animation == 52 &&
        object->apiobj.character_model->points_of_interest[4] != NULL) {
        points[0] = *NUMTX_GET_ROW_VEC(&object->joint_matrices[4], 3);
        NuVecAdd(&points[1], &points[0], &object->apiobj.collision_position);
        NuVecScale(&points[1], &points[1], 0.5f);
    }
    NUVEC difference;
    f32 length = NuVecDist(&points[0], &points[1], &difference);
    points[2].x = difference.x * 0.5f + points[1].x;
    points[2].y = difference.y * 0.5f + points[1].y;
    points[2].z = difference.z * 0.5f + points[1].z;
    object->sabre_collision_radius = length * 0.25f;
    f32 extent = length * 1.5f;
    NUVEC minimum = {points[2].x - extent, points[2].y - extent, points[2].z - extent};
    NUVEC maximum = {points[2].x + extent, points[2].y + extent, points[2].z + extent};
    if ((object->sabre_flags & 4) != 0) {
        NUVEC direction;
        NuVecRotateY(&direction, &v001, static_cast<u16>(object->apiobj.facing_angle + (context == 16 ? 0x8000 : 0)));
        NuVecScale(&difference, &direction, 0.2f);
        NuVecAdd(&points[0], &object->apiobj.collision_position, &difference);
        NuVecScale(&difference, &direction, 0.5f);
        NuVecAdd(&points[1], &object->apiobj.collision_position, &difference);
        NuVecScale(&difference, &direction, 0.35f);
        NuVecAdd(&points[2], &object->apiobj.collision_position, &difference);
        if (context == 13) {
            points[0].y = object->apiobj.collision_min.y +
                          (object->apiobj.collision_max.y - object->apiobj.collision_min.y) * 0.333f;
            points[1].y = points[2].y = points[0].y;
        }
        length = NuVecDist(&points[0], &points[1], &difference);
        points[2].x = difference.x * 0.5f + points[1].x;
        points[2].y = difference.y * 0.5f + points[1].y;
        points[2].z = difference.z * 0.5f + points[1].z;
        object->sabre_collision_radius = length * 0.25f;
        extent = length * 1.5f;
        minimum = NUVEC{points[2].x - extent, points[2].y - extent, points[2].z - extent};
        maximum = NUVEC{points[2].x + extent, points[2].y + extent, points[2].z + extent};
    }
    if ((object->sabre_flags & 5) == 0)
        return;
    GameObject_s *nearest = NULL;
    i32 nearest_point = 0;
    f32 nearest_distance = 1000000.0f;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *target = &Obj[i];
        if (target == object || (target->apiobj.field_0x1f8 & 0x1001) != 0x1001 || target->use_model_origin <= 1 ||
            target->apiobj.field_0x287 != 0 || (target->character_context & 0xfd) == 57 ||
            target->character_context == 60 || (CInfo[target->character_context].flags & 0x8000) != 0)
            continue;
        GAMECHARACTERDATA *target_data = static_cast<GAMECHARACTERDATA *>(target->apiobj.character_data->field11_0x24);
        if ((target_data->flags_090 & 0x8000) != 0 || target->apiobj.collision_min.x > maximum.x ||
            target->apiobj.collision_max.x < minimum.x || target->apiobj.collision_min.z > maximum.z ||
            target->apiobj.collision_max.z < minimum.z || target->apiobj.collision_min.y > maximum.y ||
            target->apiobj.collision_max.y < minimum.y)
            continue;
        for (i32 point = 2; point >= 0; --point) {
            if (!SphereSphereOverlapScaleY(&target->apiobj.collision_position, target->apiobj.field_0x1dc,
                                           target->apiobj.field_0x1e0, &points[point], object->sabre_collision_radius,
                                           object->sabre_collision_radius))
                continue;
            if ((object->sabre_flags & 1) != 0) {
                AddGameDebrisRot(WORLD->debris_sys, effect, &points[point], ParticlesPerSecond(10.0f, FRAMETIME), 0, 0);
            }
            const f32 distance =
                NuVecDistSqr(&object->apiobj.collision_position, &target->apiobj.collision_position, NULL);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                nearest_point = point;
                nearest = target;
            }
        }
    }
    if (nearest != NULL && (object->sabre_flags & 4) != 0) {
        AddGameDebris(WORLD->debris_sys, effect, &points[nearest_point]);
        if (!CannotKill(nearest)) {
            i32 damage = object->sabre_damage;
            if (damage != 0 && (nearest->apiobj.flags_low & 0x80) != 0) {
                damage = (object->apiobj.flags_low & 0x80) != 0 && Player_HasDoubleWeaponDamage(object) ? 2 : 1;
            }
            ObjHitObj(object, nearest, damage, ObjHitObj_Flags(object) | 0x100, 0, 1);
        } else {
            NewRumble(object->pad_gamepad->pad, 0.75f, 0);
            NewRumble(nearest->pad_gamepad->pad, 0.75f, 0);
        }
        return;
    }
    if ((object->sabre_flags & 4) != 0 && (object->apiobj.flags_low & 0x80) != 0) {
        if (GizmoBlowUp_Hit(object, points, 3, object->sabre_collision_radius, &minimum, &maximum, NULL, 0, NULL)) {
            AddGameDebris(WORLD->debris_sys, effect, &points[0]);
            AddGameDebris(WORLD->debris_sys, effect, &points[1]);
            AddGameDebris(WORLD->debris_sys, effect, &points[2]);
            NewRumble(object->pad_gamepad->pad, 0.75f, 0);
        }
    }
}

void GameObjectStuffAfterAnimation() {
    GameObject_s *object = Obj;
    i32 count = HIGHGAMEOBJECT;
    for (i32 i = 0; i < count; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
            object->apiobj.field_0x288 == 0)
            continue;
        const bool drawn = object->apiobj.model_draw_result != 0;
        if (drawn || (object->field_0x1050 & 4) != 0) {
            if (object->script_fire_target != NULL && (object->script_fire_target->apiobj.field_0x1f8 & 1) == 0)
                object->script_fire_target = NULL;
            if (static_cast<i8>(object->quick_shoot_bolt_id) != -1) {
                if (object->id == id_BASKETCANNON) {
                    GAMECHARACTERDATA *data =
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                    GameAudio_PlaySfxById(data->sfx_shoot, &object->apiobj.collision_position, 0, 0);
                    NUMTX matrix;
                    i32 joint = data->weapon_shoot_joints[0];
                    if (drawn && joint != -1 && object->apiobj.character_model->points_of_interest[joint] != NULL)
                        matrix = object->joint_matrices[joint];
                    else {
                        matrix = object->apiobj.field_0xb8;
                        *NUMTX_GET_ROW_VEC(&matrix, 3) = object->apiobj.collision_position;
                    }
                    NUVEC velocity = {0.0f, 2.0f * (static_cast<f32>(qrand()) * (1.0f / 65535.0f)),
                                      -(3.0f + 2.0f * (static_cast<f32>(qrand()) * (1.0f / 65535.0f)))};
                    NuVecMtxRotate(&velocity, &velocity, &matrix);
                    ADDPART_s part = Default_ADDPART;
                    part.matrix = &matrix;
                    part.velocity = &velocity;
                    part.field_28 = 90;
                    part.gravity = -10.0f;
                    part.flags = 0x10;
                    part.field_14 = 0.05f;
                    part.field_18 = 0.05f;
                    part.special = &WORLD->lev_objs[90].special;
                    part.field_48 = PartUpdate_Basketball;
                    part.field_3c = PartImpact_Basketball;
                    part.stop_fn = PartStop_Flickerer;
                    part.draw_fn = PartDraw_Flickerer;
                    part.time_step = FRAMETIME;
                    AddPart(&part);
                } else {
                    Bolt_Shoot(object, static_cast<i8>(object->quick_shoot_bolt_id), object->quick_shoot_flags);
                    if (object->quick_shoot_bolt_id == 29)
                        object->context_flags |= 0x40;
                }
            }
            if ((object->field_0xe23 & 4) != 0)
                Torpedo_Shoot(object);
        }
        if (drawn && ((object->apiobj.character_data->model_flags & 8) != 0 || object->id == id_BODYGUARD ||
                      object->id == id_IMPERIALGUARD)) {
            GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
            const i32 effect = object->blade_index == -1 ? -1 : BladeTab[object->blade_index].hit_effect;
            if ((data->field275_0x116 == 3 || object->id == id_GRIEVOUS || object->id == id_COUNTDOOKU) &&
                object->character_context == 0 && object->action_movement_state == 3)
                object->sabre_flags |= 2;
            if (object->id == id_BODYGUARD && (object->character_context == 12 || object->character_context == 24) &&
                static_cast<u16>(object->context_animation - 26) <= 1)
                object->sabre_flags |= 2;
            u8 streak = object->sabre_flags & 2;
            const u8 damage = object->sabre_flags & 4;
            if (object->id == id_GRIEVOUS) {
                bool reset_streak = (object->character_context == 12 || object->character_context == 24) &&
                                    object->context_animation == 26;
                if (reset_streak) {
                    object->sabre_flags |= 2;
                    streak = 2;
                }
                i32 first = -1, second = -1;
                if (damage != 0) {
                    if (object->character_context == 13) {
                        first = 2;
                        second = 0;
                    } else if (object->character_context == 16) {
                        first = 3;
                        second = 2;
                    } else if (object->character_context == 5 &&
                               static_cast<u16>(object->context_animation - 46) <= 13) {
                        static const i32 first_blade[14] = {-1, -1, -1, 2, 2, 2, 2, 1, 3, 3, -1, -1, -1, -1};
                        static const i32 second_blade[14] = {0, 3, 3, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2};
                        first = first_blade[object->context_animation - 46];
                        second = second_blade[object->context_animation - 46];
                    }
                }
                u8 flags = streak | 1;
                for (i32 blade = 0; blade < 4; ++blade) {
                    if (blade == 2 && reset_streak)
                        flags = 1;
                    object->sabre_flags = flags;
                    if (damage != 0 && (first == blade || second == blade))
                        object->sabre_flags |= damage;
                    if (object->character_context == 16) {
                        if (blade < 2)
                            object->sabre_flags &= ~2;
                        else
                            object->sabre_flags |= 2;
                    }
                    LightSabreStreakCode(object, blade, effect);
                }
            } else if (object->id == id_DARTHMAUL) {
                LightSabreStreakCode(object, 0, effect);
                object->sabre_flags = streak | 1;
                LightSabreStreakCode(object, 1, effect);
            } else if (object->id == id_BODYGUARD) {
                u8 first = object->sabre_flags;
                u8 second = first;
                if (object->character_context == 5 && damage != 0) {
                    if (object->combo_stage == 2) {
                        first &= ~4;
                        second |= 4;
                    } else {
                        first |= 4;
                        second &= ~4;
                    }
                } else if (object->character_context == 0 && object->action_movement_state == 4) {
                    first |= 2;
                    second = first;
                }
                object->sabre_flags = first;
                LightSabreStreakCode(object, 0, effect);
                object->sabre_flags = second;
                LightSabreStreakCode(object, 1, effect);
            } else
                LightSabreStreakCode(object, 0, effect);
        }
        const i32 debris_joint = static_cast<i8>(object->pad_e3c[1]);
        if (debris_joint != -1)
            AddGameDebris(WORLD->debris_sys, (object->movement_runtime_flags & 1) != 0 ? 139 : 138,
                          NUMTX_GET_ROW_VEC(&object->joint_matrices[debris_joint], 3));
        if (object->character_context == 38) {
            CHARACTERANIM_s *animation =
                static_cast<CHARACTERANIM_s *>(object->apiobj.character_model->model_data_a[object->context_animation]);
            if (animation != NULL && static_cast<i8>(animation->locator) != -1 &&
                ((object->context_flags & 0x40) == 0 || debris_joint != -1)) {
                NUVEC points[2];
                points[0] = points[1] =
                    *NUMTX_GET_ROW_VEC(&object->joint_matrices[static_cast<i8>(animation->locator)], 3);
                points[0].y += 0.075f;
                points[1].y -= 0.075f;
                AddStreakPoints(points, 0.25f, 0xffffffff, &object->sabre_streaks[0][0], 0, object);
            }
        }
        if (object->slam_debris_effect != -1)
            AddSlamDebris(object);
        if (object->pad_e3c[0] != 0)
            Batarang_Release(object, object->pad_e3c[0]);
        if (static_cast<i8>(object->field_0xe22) < 0)
            SuperCarry_Throw(object, 0);
        if ((object->field_0xe20 & 0x10) != 0) {
            NUMTX matrix;
            i32 joint =
                drawn ? static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->rocket_locator
                      : -1;
            if (drawn && joint != -1 && object->apiobj.character_model->points_of_interest[joint] != NULL)
                matrix = object->joint_matrices[joint];
            else {
                NuMtxSetRotationX(&matrix, 0x1555);
                NuMtxRotateY(&matrix, object->apiobj.field_0x276 + 0x8000);
                *NUMTX_GET_ROW_VEC(&matrix, 3) = object->apiobj.collision_position;
            }
            NUVEC velocity = {0.0f, 0.0f, -1.0f};
            NuVecMtxRotate(&velocity, &velocity, &matrix);
            ADDPART_s part = Default_ADDPART;
            part.matrix = &matrix;
            part.velocity = &velocity;
            part.field_14 = 0.1f;
            part.field_18 = 0.1f;
            part.special = &WORLD->lev_objs[232].special;
            part.field_28 = 232;
            const bool player = (object->apiobj.field_0x1f8 & 0x80) != 0;
            part.flags = player ? 0xc11b : 0xc31b;
            part.owner = object;
            part.field_90 = static_cast<u8>(object->apiobj.field_0x289);
            part.field_44 = BobaRocket_Kill;
            part.move_fn = BobaRocket_Move;
            part.update_fn = BobaRocket_Deflect;
            part.field_40 = PartCollide_3D;
            part.time_step = FRAMETIME;
            part.gravity = 0.0f;
            part.field_a4 = player ? 4.0f + 2.0f * (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) : 10.0f;
            NUVEC direction;
            BoltSys->shoot_direction(object, &direction);
            f32 range = part.field_a4 * rocket_speed;
            part.recipient = TargetGameObject(object, &object->apiobj.collision_position, &direction, range,
                                              range * range, 0, 1, 0, -1);
            PART_s *created = AddPart(&part);
            if (created != NULL)
                created->force_flags = ObjHitObj_Flags(object);
        }
        if ((object->movement_runtime_flags & 0x40) != 0)
            ThermalDetonator_Throw(object);
        if ((object->field_0xe23 & 2) != 0 && object->field_0x780 != NULL) {
            NUVEC *origin = GetZapOrigin(object);
            NUVEC delta;
            f32 distance =
                NuVecDist(&static_cast<GameObject_s *>(object->field_0x780)->apiobj.collision_position, origin, &delta);
            NuLgtLaser(testlaser_type, testlaser_sizew, testlaser_sizel, testlaser_sizewab, origin, &delta,
                       object->id == id_R2Q5 ? 0xff202080 : 0xff808040, testlaser_endw, distance);
        }
        if (drawn && (object->apiobj.character_data->model_flags & 8) != 0 && object->weapon_scale == 1.0f) {
            for (i32 blade = 0; blade < 4; ++blade) {
                if (object->apiobj.field_0x288 == 0 || (object->field_0xe23 & 8) == 0)
                    continue;
                GAMECHARACTERDATA *blade_data =
                    static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                i32 joint_a = blade_data->streak_joints[blade][0];
                i32 joint_b = blade_data->streak_joints[blade][1];
                if (joint_a == -1 || object->apiobj.character_model->points_of_interest[joint_a] == NULL ||
                    joint_b == -1 || object->apiobj.character_model->points_of_interest[joint_b] == NULL)
                    continue;
                NUVEC points[2];
                points[0] = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint_a], 3);
                points[1] = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint_b], 3);
                i32 colour = object->blade_states[blade];
                if (colour != -1 && object->character_context == 5 && (object->context_flags & 0x1c) == 0 &&
                    (static_cast<u8>(object->combo_branch) & 0xfb) == 2) {
                    i32 index = object->combo_branch == 6 ? BladeTab[colour].hit_effect : BladeTab[colour].effect_6;
                    i32 effect = WORLD->debris_sys->entries[index].effect;
                    if (effect != -1) {
                        i32 count = ParticlesPerSecond(100.0f, FRAMETIME);
                        if (count > 0) {
                            f32 fraction = static_cast<f32>(qrand()) * (1.0f / 65535.0f);
                            NUVEC position = {points[0].x + (points[1].x - points[0].x) * fraction,
                                              points[0].y + (points[1].y - points[0].y) * fraction,
                                              points[0].z + (points[1].z - points[0].z) * fraction};
                            AddVariableShotDebrisEffect(effect, &position, count, 0, 0);
                        }
                    }
                }
                if ((object->apiobj.field_0x1f8 & 0x80) == 0)
                    continue;
                bool hit = false;
                for (i32 side = 0; side < 2; ++side) {
                    NUVEC delta;
                    NuVecSub(&delta, &points[1 - side], &points[side]);
                    if (GameRayCast(&points[side], &delta, 0.0f, TERRAINMASK_NONWEAPON | 0x1f) != 0) {
                        hit = true;
                        f32 fraction = NewRayCastGetTOFI();
                        NUVEC position = {points[side].x + (points[1 - side].x - points[side].x) * fraction,
                                          points[side].y + (points[1 - side].y - points[side].y) * fraction,
                                          points[side].z + (points[1 - side].z - points[side].z) * fraction};
                        i32 index;
                        if (object->id == id_GRIEVOUS && static_cast<u32>(blade - 1) <= 1 &&
                            (object->apiobj.field_0x27c == -1 ||
                             (Cheat_IsOn(25) == 0 &&
                              (object->apiobj.field_0x27c == -1 || !Player_HasPurpleForce(object)))))
                            index = 65;
                        else
                            index = object->blade_index == -1 ? -1 : BladeTab[object->blade_index].effect_8;
                        if (index != -1) {
                            i32 effect = WORLD->debris_sys->entries[index].effect;
                            if (effect != -1) {
                                i32 count = ParticlesPerSecond(50.0f, FRAMETIME);
                                if (count > 0)
                                    AddVariableShotDebrisEffect(effect, &position, count, 0, 0);
                            }
                        }
                    }
                }
                if (hit && sabrerubwait <= 0.0f && object->apiobj.anim_packet.blending == 0 &&
                    object->character_context != 5 && object->character_context != 16 &&
                    object->character_context != 13 && object->character_context != 14 &&
                    (CInfo[object->character_context].flags & 0x4000000) == 0 &&
                    ((CInfo[object->character_context].flags & 0x8000000) == 0 || (object->jump_flags & 2) == 0)) {
                    PlaySabreSfx(const_cast<char *>("SaberSparks"), object, &points[blade], 1);
                    sabrerubwait = 0.3f + 0.3f * (static_cast<f32>(qrand()) * (1.0f / 65535.0f));
                    if (GetMenuID() == -1)
                        NewRumble(object->pad_gamepad->pad,
                                  0.2f + 0.2f * (static_cast<f32>(qrand()) * (1.0f / 65535.0f)), 0);
                }
            }
        }
        if (drawn && object->id == id_LANDSPEEDER) {
            f32 ratio = object->pad_gamepad->input_magnitude /
                        static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->movement_speed;
            i32 effect = WORLD->debris_sys->entries[105].effect;
            if (effect != -1)
                AddVariableShotDebrisEffectTimed1(effect, &object->apiobj.lower_position, 60, FRAMETIME, 0, 0, NULL);
            effect = WORLD->debris_sys->entries[104].effect;
            if (effect != -1) {
                for (i32 joint = 2; joint <= 4; ++joint) {
                    if (object->apiobj.character_model->points_of_interest[joint] != NULL) {
                        i32 count = static_cast<i32>(60.0f * ratio);
                        if (count > 0)
                            AddVariableShotDebrisEffectTimed1(effect,
                                                              NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3),
                                                              count, FRAMETIME, 0, 0, NULL);
                    }
                }
            }
            effect = WORLD->debris_sys->entries[103].effect;
            if (effect != -1 && object->apiobj.field_0x218 != 2000000.0f) {
                for (i32 joint = 2; joint <= 4; ++joint) {
                    if (object->apiobj.character_model->points_of_interest[joint] != NULL) {
                        NUVEC position = *NUMTX_GET_ROW_VEC(&object->joint_matrices[joint], 3);
                        f32 height = position.y - object->apiobj.field_0x218;
                        if (height > 0.0f && height <= 0.2f) {
                            f32 amount = height < 0.1f ? 25.0f : (1.0f - (height - 0.1f) / 0.1f) * 25.0f;
                            if (static_cast<i32>(amount * ratio) > 0) {
                                position.y = object->apiobj.field_0x218;
                                AddVariableShotDebrisEffectTimed1(effect, &position, 60, FRAMETIME, 0, 0, NULL);
                            }
                        }
                    }
                }
            }
        }
        if (VehicleArea != 0 && static_cast<u8>(object->apiobj.field_0x27c) <= 1) {
            i32 *key = &FalconDebKey[static_cast<u8>(object->apiobj.field_0x27c)];
            if (object->id == id_MILLENNIUMFALCON && (object->apiobj.field_0x1f8 & 0x80) != 0 &&
                (object->field_0xe24 & 8) != 0 && object->apiobj.character_model->points_of_interest[2] != NULL &&
                object->apiobj.field_0xa8 == 1.0f) {
                if (*key == -1)
                    AddDebrisEffect(key, WORLD->debris_sys->entries[136].effect, 0.0f, 0.0f, 0.0f);
                else {
                    NUMTX matrix = object->joint_matrices[2];
                    NuMtxPreRotateX(&matrix, 0x4000);
                    DebrisPosOrientationMtx(*key, &matrix);
                }
            } else
                DebFreeInstantly(key);
        }
        if (object->field_0xdec > 0.0f && object->apiobj.field_0x287 == 0) {
            f32 radius = object->apiobj.field_0x1dc * (0.5f + 0.5f * (static_cast<f32>(qrand()) * (1.0f / 65535.0f)));
            u16 angle = qrand();
            NUVEC position = {object->apiobj.position.x + radius * NuTrigTable[angle >> 1],
                              object->apiobj.position.y + object->field_0xffc * object->apiobj.field_0xa8 + 0.05f,
                              object->apiobj.position.z + radius * NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff]};
            PowerUp_Particles(WORLD, &position);
        }
        count = HIGHGAMEOBJECT;
    }
}

void GameMsg_DrawAdjustNewPos_CoinToTotal(GAMEMESSAGE_s *message) {
    message->target_position.x = cointotal_x[message->player_index];
}

i32 Game_Exit(i32) {
    return 0;
}

LEVELDATA_s *CanSaveAndExit(WORLDINFO_s *world) {
    extern i32 GAMEDEMO;
    extern i32 SuperStory;
    extern i32 ChallengeMode;
    extern i32 Arcade;

    if (GAMEDEMO == 0 && SuperStory == 0 && world->area != NULL && world->area != HUB_ADATA &&
        (world->area->flags & 0x146) == 0 && Mission_Active(NULL) == NULL && ChallengeMode == 0 && Arcade == 0 &&
        CutScenePlayer_Active() == NULL && Game_AreaSave != NULL &&
        Game_AreaSave[world->level_sub_id].area_complete != 0 && AreaGlobals.values.field_0x18 > 0) {
        return Area_FindStatusLevel(world->area, NULL);
    }

    return NULL;
}

void GameObject_s::ClearAddons() {
    delete addons;
    addons = NULL;
}

void GameObject_s::ClearMechObjectInterface() {
    delete mech_object_interface;
}

MechAddonCollection *GameObject_s::GetAddons(bool create) {
    if (addons == NULL && create) {
        MechObjectInterface *object = GetMechObjectInterface();
        addons = new MechAddonCollection(*object);
    }
    return addons;
}

MechObjectInterface *GameObject_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL)
        new CharacterObjectInterface(*this);
    return mech_object_interface;
}

bool GameObject_s::IsRunningTaskType(HashedKey const &type) {
    MechTouchTask *task = touch_task;
    while (task != NULL) {
        MechTouchTask *next = task->next;
        if (task->GetHashId().value == type.value)
            return true;
        task = next;
    }
    return false;
}

void GameObject_s::KillTasks() {
    MechTouchTask *task = touch_task;
    while (task != NULL) {
        MechTouchTask *next = task->next;
        task->OnStop();
        delete task;
        task = next;
    }
    touch_task = NULL;
}

// ThingManager::AddThing @0x424c10. Appends at count; the pending
// AddThingAfterThis reservation (field_0x14) is folded into the index and
// cleared here.
void ThingManager::AddThing(BaseThing *thing) {
    i32 index = this->count;
    if (thing != NULL) {
        if (index < this->max_things) {
            this->things[index] = thing;
            index = index + 1;
        }
    }
    index = index + this->field_0x14;
    this->field_0x14 = 0;
    this->count = index;
}

// ThingManager::AddThingAfterThis @0x424c40. Reserves the slot after the
// current tail: bumps field_0x14 and stores the thing at count+field_0x14;
// the next AddThing folds the reservation into count.
void ThingManager::AddThingAfterThis(BaseThing *thing) {
    if (thing != NULL) {
        i32 index = this->field_0x14 + 1;
        this->field_0x14 = index;
        index = index + this->count;
        if (index < this->max_things) {
            this->things[index] = thing;
        }
    }
}

// ThingManager::DisplayThings @0x4252c0. Single pass over Display,
// bracketed with timebar slot 3 ("Dis"). PanelRender uses this pass for the
// display-layer things that render on top of the gameplay panel.
void ThingManager::DisplayThings(ThingRenderData *data) {
    static const char *name = "Dis"; // timebar slot name @0x5734db

    if (this->count <= 0) {
        return;
    }
    i32 i = 0;
    do {
        BaseThing *thing = this->things[i];
        if (thing == NULL || (thing->flags & THING_FLAG_SKIP_DISPLAY)) {
        } else {
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotBegin(this->timebar, 3, name);
                thing = this->things[i];
            }
            thing->Display(data);
            thing = this->things[i];
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotEnd(this->timebar, 3);
            }
        }
        ++i;
    } while (i < this->count);
}

void ThingManager::EffectsThings(ThingRenderData *) {
}

// ThingManager::EnableActions @0x425930. Finds the first thing whose 0x4 id
// matches and sets (invert==0) or clears (invert!=0) the given flags bits.
void ThingManager::EnableActions(i32 id, i32 flags, i32 invert) {
    i32 count = this->count;
    if (count <= 0) {
        return;
    }
    for (i32 i = 0; i < count; i++) {
        BaseThing *thing = this->things[i];
        if (thing == NULL) {
            continue;
        }
        if (thing->field_0x4 == (u32)id) {
            if (invert == 0) {
                thing->flags |= (u32)flags;
            } else {
                thing->flags &= ~(u32)flags;
            }
            return;
        }
    }
}

void ThingManager::EnterLevelThings(ThingLevelData *) {
}

void ThingManager::ExitLevelThings(ThingLevelData *) {
}

// ThingManager::ProcessThings @0x425460. Pass 1 always runs
// ProcessEvenWhenPaused first; then, per ThingProcessData.paused, either
// Process or ProcessOnlyWhenPaused. Each pass has its own opt-out flag. The count
// is re-read every iteration because thing Process calls may add things.
// Profiling: things with a non-NULL profiling handle are bracketed with
// NuTimeBarSlotBegin/End (stubbed no-ops on this build).
void ThingManager::ProcessThings(ThingProcessData *data) {
    static const char *name = "PROC"; // timebar slot name @0x5734e3

    if (this->count <= 0) {
        return;
    }
    for (i32 i = 0; i < this->count; i++) {
        BaseThing *thing = this->things[i];
        if (thing == NULL || (thing->flags & THING_FLAG_SKIP_PROCESS_EVEN_WHEN_PAUSED)) {
            continue;
        }
        if (thing->profiling_0xc != NULL) {
            _NuTimeBarSlotBegin(this->timebar, 0, name);
        }
        thing->ProcessEvenWhenPaused(data);
        thing = this->things[i];
        if (thing->profiling_0xc != NULL) {
            _NuTimeBarSlotEnd(this->timebar, 0);
        }
    }
    if (data->paused != 0) {
        if (this->count <= 0) {
            return;
        }
        for (i32 i = 0; i < this->count; i++) {
            BaseThing *thing = this->things[i];
            if (thing == NULL || (thing->flags & THING_FLAG_SKIP_PROCESS_ONLY_WHEN_PAUSED)) {
                continue;
            }
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotBegin(this->timebar, 0, name);
            }
            thing->ProcessOnlyWhenPaused(data);
            thing = this->things[i];
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotEnd(this->timebar, 0);
            }
        }
    } else {
        if (this->count <= 0) {
            return;
        }
        for (i32 i = 0; i < this->count; i++) {
            BaseThing *thing = this->things[i];
            if (thing == NULL || (thing->flags & THING_FLAG_SKIP_PROCESS)) {
                continue;
            }
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotBegin(this->timebar, 0, name);
            }
            thing->Process(data);
            thing = this->things[i];
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotEnd(this->timebar, 0);
            }
        }
    }
}

void ThingManager::RemoveDependanciesThings(ThingRemoveData *) {
}

void ThingManager::RemoveTemporaryThings() {
}

// ThingManager::RenderThings @0x425390. Single pass over Render,
// bracketed with timebar slot 1 ("Rnd").
void ThingManager::RenderThings(ThingRenderData *data) {
    static const char *name = "Rnd"; // timebar slot name @0x5734df

    if (this->count <= 0) {
        return;
    }
    i32 i = 0;
    do {
        BaseThing *thing = this->things[i];
        if (thing == NULL || (thing->flags & THING_FLAG_SKIP_RENDER)) {
        } else {
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotBegin(this->timebar, 1, name);
                thing = this->things[i];
            }
            thing->Render(data);
            thing = this->things[i];
            if (thing->profiling_0xc != NULL) {
                _NuTimeBarSlotEnd(this->timebar, 1);
            }
        }
        ++i;
    } while (i < this->count);
}

void ThingManager::ResetThings(ThingResetData *data) {
    const char *name = "Res";

    i32 i = 0;
    if (this->count > 0) {
        do {
            BaseThing *thing = this->things[i];
            if (thing != NULL && (thing->flags & 8) == 0) {
                if (thing->profiling_0xc != NULL) {
                    _NuTimeBarSlotBegin(this->timebar, 4, name);
                }
                thing = this->things[i];
                thing->Reset(data);
                thing = this->things[i];
                if (thing->profiling_0xc != NULL) {
                    _NuTimeBarSlotEnd(this->timebar, 4);
                }
            }
            ++i;
        } while (i < this->count);
    }
}

// ThingManager::ThingManager @0x425870. Stores the manager in theThingManager
// and carves the thing-pointer array from the
// theMemoryManager linear pool. On allocation failure the array is NULL — the
// manager then simply never accepts things (AddThing's count < max check).
ThingManager::ThingManager(i32 max_things) {
    const usize need = static_cast<usize>(max_things) * sizeof(*things);

    BaseThing **array = NULL;
    if (*theMemoryManager.end_cell - *theMemoryManager.cursor_cell > need) {
        const usize aligned = ALIGN(*theMemoryManager.cursor_cell, 0x10);
        *theMemoryManager.cursor_cell = aligned + need;
        array = reinterpret_cast<BaseThing **>(aligned);
        memset(array, 0, need);
        theMemoryManager.allocated += need;
        theMemoryManager.remaining -= need;
        theMemoryManager.high_water = *theMemoryManager.cursor_cell;
    }
    this->things = array;
    this->max_things = max_things;
    // Profiling sets are a deferred subsystem; the handle is only ever passed
    // to the NuTimeBarSlotBegin/End stubs, so NULL behaves like the original
    // with profiling disabled.
    this->timebar = NuTimeBarCreateSet(0);
    theThingManager = this;
}

ThingManager::~ThingManager() {
}

void ThingManager::cbEdTimingSelect(eduimenu_s *, eduiitem_s *, u32) {
}

void ThingManager::cbEdTrackCancel(eduimenu_s *, eduimenu_s *) {
}

void ThingManager::edTimingEnter() {
}

void ThingManager::edTimingInit() {
    static_cast<ThingManager *>(theThingManager)->ed_timing_state = 0;
}

void ThingManager::edTimingProc(float, nupad_s *) {
}

void ThingManager::edTimingRender() {
}

void SpecialObject::Exists() const {
}

void SpecialObject::GetCollision() const {
}

void SpecialObject::GetCurrentPosition() const {
}

void SpecialObject::GetCurrentTransform() const {
}

void SpecialObject::GetInitialPosition() const {
}

void SpecialObject::GetInitialTransform() const {
}

void SpecialObject::GetMtl(i32) const {
}

void SpecialObject::GetName() const {
}

void SpecialObject::GetNumMtls() const {
}

void SpecialObject::GetRadius() const {
}

void SpecialObject::GetVisibility() const {
}

void SpecialObject::Render(VuMtx const *) const {
}

void SpecialObject::SetCollision(i32) {
}

void SpecialObject::SetCurrentPosition(VuVec const *) {
}

void SpecialObject::SetCurrentTransform(VuMtx const *) {
}

void SpecialObject::SetInitialPosition(VuVec const *) {
}

void SpecialObject::SetInitialTransform(VuMtx const *) {
}

void SpecialObject::SetVisibility(i32) {
}

SpecialObject::SpecialObject() {
}

void GameThingManager::AddLevelOnlyThings() {
}

// GameThingManager::AddOnceOnlyThings @0x4e8bb0: registers the MechSystems
// singleton as the manager's once-only thing (via the virtual AddThing slot).
void GameThingManager::AddOnceOnlyThings() {
    this->AddThing(MechSystems::Get());
}

// GameThingManager::GameThingManager @0x4e8b00: stores the object in
// theGameThings (the vptr switch to the derived vtable is compiler-generated).
GameThingManager::GameThingManager(i32 max_things) : ThingManager(max_things) {
    theGameThings = this;
}

// GameThingManager D1 dtor @0x4e8a80 clears the global before destruction.
GameThingManager::~GameThingManager() {
    theGameThings = NULL;
}

HashedKey CantPickupBombTimerAddon::s_hashId;

CantPickupBombTimerAddon::CantPickupBombTimerAddon(MechObjectInterface &object, float duration)
    : MechAddon(object, s_hashId.value), remaining_time(duration) {
    GameObject_s *character = object.GetCharacterObject();
    if (character != NULL) {
        character->apiobj.velocity.x += character->apiobj.velocity.x;
        character->apiobj.velocity.z += character->apiobj.velocity.z;
    }
}

bool CantPickupBombTimerAddon::OnProcess(MechAddon::ProcessStage, float elapsed) {
    remaining_time -= elapsed;
    return remaining_time > 0.0f;
}

CantPickupBombTimerAddon::~CantPickupBombTimerAddon() {
}

// BaseThing::BaseThing @0x425840 zeroes the data fields after the vptr.
BaseThing::BaseThing() {
    this->field_0x4 = 0;
    this->flags = 0;
    this->profiling_0xc = NULL;
}

// BaseThing defaults @0x424bf0 (dtor) and 0x425990..0x425a20 (interface
// defaults); RemoveDependancies returns 1, the rest are no-ops. GetName holds
// a 0 slot in the original base vtable (pure) — see basething.h.
BaseThing::~BaseThing() {
}

i32 BaseThing::RemoveDependancies(ThingRemoveData *) {
    return 1;
}

void BaseThing::EnterLevel(ThingLevelData *) {
}

void BaseThing::ExitLevel(ThingLevelData *) {
}

void BaseThing::Reset(ThingResetData *) {
}

void BaseThing::Process(ThingProcessData *) {
}

void BaseThing::ProcessEvenWhenPaused(ThingProcessData *) {
}

void BaseThing::ProcessOnlyWhenPaused(ThingProcessData *) {
}

void BaseThing::Render(ThingRenderData *) {
}

void BaseThing::Display(ThingRenderData *) {
}

void BaseThing::Effects(ThingRenderData *) {
}

static __used__ void LEGO_100PercentFn() {
}
static __used__ void LEGO_AllGoldBricksFn() {
}

i32 NoLayerKill(GameObject_s *object) {
    GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
    if ((data->flags_094[2] & 0x80) != 0)
        return object->apiobj.field_0x27f == 6;
    return 0;
}

void GetUsageMask(NuShaderUsageMask_s *) {
}

void GetTakeOverPos(GameObject_s *, NUVEC *);
void ReleaseTakeOver(GameObject_s *, i32);
void TakeOverGameObject2(GameObject_s *, GameObject_s *, i32);
void TakeOver2GetIn(GameObject_s *, GameObject_s *);
void Move_BEAST(GameObject_s *);
void Hint_SetComplete(i32);
void PlayLandSfx(GameObject_s *, i32, i32);
void NewBuzz(nupad_s *, f32, i32);
i32 RotDiff(u16, u16);
f32 SpeederChaseATATInOutMul(NUVEC *, NUVEC *);
i32 players_cannot_exit_speeder;
extern i16 id_ATAT, id_SPEEDERBIKE;
extern "C" i16 id_GRABCONTROL, id_GRABR2CONTROL;

void TakeOverCode(GameObject_s *object, i32 tag_pressed) {
    APIOBJECT &api = object->apiobj;
    if (object->character_context == 0x3c) {
        GameObject_s *target = object->takeover_entry_target;
        if (target == NULL) {
            object->character_context = -1;
            return;
        }
        object->context_animation_timer += FRAMETIME;
        if (object->context_animation_timer >= object->airborne_action_duration) {
            object->character_context = -1;
            if ((api.field_0x1f8 & 0x80) != 0) {
                if (target->id == id_GRABCONTROL || target->id == id_GRABR2CONTROL) {
                    Hint_SetComplete(0x28a);
                } else if (target->apiobj.character_data->move_fn == Move_BEAST) {
                    Hint_SetComplete(0x263);
                } else if (object->id == id_ATAT) {
                    Hint_SetComplete(0x28d);
                } else {
                    Hint_SetComplete(0x25d);
                }
            }
            if (WORLD->grabber != NULL && (object->takeover_entry_target->id == id_GRABCONTROL ||
                                           object->takeover_entry_target->id == id_GRABR2CONTROL)) {
                PlaySfx("env_crane_in", &api.collision_position);
            } else {
                PlayLandSfx(object, 0, 0);
            }
            NewBuzz(object->pad_gamepad->pad, 0.1f, 0);
            TakeOverGameObject2(object, object->takeover_entry_target, 1);
            return;
        }
        const f32 fraction = object->context_animation_timer / object->airborne_action_duration;
        NUVEC destination;
        GetTakeOverPos(target, &destination);
        api.position.x = (destination.x - object->external_force.x) * fraction + object->external_force.x;
        api.position.y = (destination.y - object->external_force.y) * fraction + object->external_force.y;
        api.position.z = (destination.z - object->external_force.z) * fraction + object->external_force.z;
        target = object->takeover_entry_target;
        f32 height = target->apiobj.field_0x1e0;
        if (object->context_animation == 0xc9 || object->context_animation == 6) {
            const f32 jump_height = api.character_data->game_character->field_0x34;
            height = jump_height > height ? jump_height : height;
        }
        if (target->id == id_ATAT && WORLD->current_level == SPEEDERCHASEA_LDATA) {
            height *= SpeederChaseATATInOutMul(&destination, &object->external_force) * 0.75f;
            target = object->takeover_entry_target;
        }
        const f32 phase = fraction * 32768.0f;
        api.position.y += height * NU_SIN_LUT(static_cast<i32>(phase));
        const u16 start_angle = object->takeover_start_angle;
        const i32 difference = RotDiff(start_angle, target->apiobj.field_0x276);
        const f32 blend = 1.0f - (NU_SIN_LUT(static_cast<i32>(phase + 16384.0f)) + 1.0f) * 0.5f;
        const u16 angle =
            static_cast<u16>(static_cast<i32>(static_cast<f32>(start_angle) + static_cast<f32>(difference) * blend));
        api.field_0x276 = angle;
        api.movement_facing_angle = angle;
        api.facing_angle = angle;
        return;
    }
    object->tag_flags &= static_cast<u8>(~1u);
    GameObject_s *target = object->field_0xcc0;
    if (target != NULL) {
        if (object->character_context == 0x3b) {
            if (!(players_cannot_exit_speeder != 0 && target->id == id_SPEEDERBIKE)) {
                u32 buttons = target->pad_gamepad->buttons_pressed;
                if ((object->field_0xf00 & 2) != 0)
                    buttons &= ~GAMEPAD_JUMP;
                if (target->character_context != 0x2a && (buttons & (GAMEPAD_JUMP | GAMEPAD_TAG)) != 0) {
                    ReleaseTakeOver(object, 1);
                    if ((api.field_0x1f8 & 0x80) != 0)
                        Hint_SetComplete(0x25e);
                    return;
                }
            }
            GetTakeOverPos(target, &api.position);
            target = object->field_0xcc0;
            api.pitch_angle = 0;
            api.roll_angle = 0;
            api.field_0x276 = target->apiobj.facing_angle;
            api.facing_angle = target->apiobj.facing_angle;
            api.movement_facing_angle = target->apiobj.facing_angle;
            GameObjectOrigin(object);
            object->takeover_timer = 1.0f;
        } else if (api.character_data->game_character->field275_0x116 == 12) {
            i16 animation = 0xc3;
            if (CurrentAnim(&api.anim_packet) == 3 || CurrentAnim(&api.anim_packet) == 0x17)
                animation = 0xc4;
            target = object->field_0xcc0;
            if (target->apiobj.character_model->model_data_b[animation] != NULL)
                target->context_animation = animation;
        }
        return;
    }
    if (object->takeover_timer >= 0.0f) {
        object->takeover_timer -= FRAMETIME;
        return;
    }
    if (MiniCutCam != 0 || (api.field_0x1f8 & 0x80) == 0 || api.field_0x287 != 0 || !(object->takeover_timer <= 0.0f) ||
        (object->character_context != -1 && (object->character_context != 0 || object->action_movement_state == 3 ||
                                             object->action_movement_state == 4)) ||
        api.field_0x27d == 0) {
        return;
    }
    f32 nearest_distance = 9.0f;
    GameObject_s *nearest = NULL;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *candidate = &Obj[i];
        const APIOBJECT &other = candidate->apiobj;
        if ((other.field_0x1f8 & 0x1001) != 0x1001 || other.field_0x287 != 0 || other.field_0x27c != -1 ||
            (CInfo[candidate->character_context].flags & 0x8000) != 0 || candidate->field_0xcc0 != NULL ||
            (other.character_data->game_character->flags_090 & 0x40) == 0 ||
            ((other.character_data->model_flags & 0x40000000) != 0 &&
             (candidate->character_context == 0x3e || candidate->character_context == 0x17)) ||
            (candidate->tag_flags & 2) != 0)
            continue;
        if (candidate->id == id_ATAT && WORLD->current_level == SPEEDERCHASEA_LDATA) {
            if (api.field_0x281 == 0x13)
                nearest = candidate;
            continue;
        }
        if (api.collision_min.y > other.collision_max.y || other.collision_min.y > api.collision_max.y)
            continue;
        const f32 dx = other.collision_position.x - api.collision_position.x;
        const f32 dz = other.collision_position.z - api.collision_position.z;
        const f32 range = (other.field_0x1dc + other.field_0x1dc) + api.field_0x1dc;
        const f32 distance = dx * dx + dz * dz;
        if (object->facing_direction.x * dx + object->facing_direction.z * dz > 0.0f && range * range > distance &&
            nearest_distance > distance) {
            nearest_distance = distance;
            nearest = candidate;
        }
    }
    if (nearest != NULL) {
        if (tag_pressed != 0 || (nearest->id == id_ATAT && WORLD->current_level == SPEEDERCHASEA_LDATA)) {
            TakeOver2GetIn(object, nearest);
        } else {
            object->tag_flags |= 1;
            nearest->field_0xe23 |= 0x80;
        }
    }
}

void InitExtraList() {
    for (i32 i = 0; i < 44; ++i) {
        NuStrCpy(ExtraItems[i].special_name, Cheat[i].extra_name);
        NuSpecialFind(WORLD->current_gscn, &ExtraItems[i].special, ExtraItems[i].special_name, 1);
        ExtraItems[i].type = 2;
        ExtraItems[i].unlocked = 0;
        ExtraItems[i].item_id = i;

        u32 unlocked = reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(&Game) + 0x7bf0)[i >> 5];
        if (((static_cast<u64>(unlocked) >> (i & 0x1f)) & 1) != 0) {
            ExtraItems[i].unlocked = 1;
            reinterpret_cast<u32 *>(reinterpret_cast<u8 *>(&Game) + 0x7c00)[i >> 5] |= 1u << (i & 0x1f);
        }
        ExtraItems[i].price = Cheat[i].extra_price;
    }

    SHOPEXTRACOUNT = 44;
    for (i32 i = 0; i < SHOPEXTRACOUNT; ++i) {
        char name[32];
        NuStrCpy(name, ExtraItems[i].special_name);
        NuStrCat(name, "b");
        NuSpecialFind(WORLD->current_gscn, &extrasils[i], name, 1);
    }
}

GameObject_s *FindGameObject(i32 character_id, u32 required_flags, i32 alive_only, i32 vehicle_only,
                             i32 non_level_only) {
    for (i32 index = 0; index < HIGHGAMEOBJECT; ++index) {
        GameObject_s *object = &Obj[index];
        if ((object->apiobj.field_0x1f8 & APIOBJECT_FLAG_IN_USE) == 0) {
            continue;
        }
        if (vehicle_only != 0 && (object->apiobj.field_0x1f8 & 0x1000) == 0) {
            continue;
        }
        if (required_flags != 0 && (object->apiobj.field_0x1f4 & required_flags) != required_flags) {
            continue;
        }
        if (character_id != -1 && object->id != character_id) {
            continue;
        }
        if (alive_only != 0 && object->apiobj.field_0x287 != 0) {
            continue;
        }
        if (non_level_only != 0 && object->field_0x107c != -1) {
            continue;
        }
        return object;
    }
    return NULL;
}

void KillGameObject(GameObject_s *object, i32 reason, i32) {
    if (object == NULL || (object->apiobj.field_0x1f8 & APIOBJECT_FLAG_IN_USE) == 0) {
        return;
    }

    const i32 requested_reason = reason;
    if (reason == 5) {
        reason = 4;
    }

    object->KillTasks();
    object->current_hp = 0;
    object->apiobj.velocity.x = 0.0f;
    object->apiobj.velocity.z = 0.0f;

    // The shipped function converts the ordinary scripted kill (reason 4)
    // into the terminal death state 2 after its effects have been emitted.
    const bool terminal_kill = reason == 4;
    if (terminal_kill) {
        reason = 2;
    }
    object->apiobj.field_0x287 = static_cast<u8>(reason == 3 ? 2 : reason);
    if (object->apiobj.field_0x287 == 2) {
        object->movement_lean_angle = 0;
        object->field_0x1018 = 0.0f;
    } else {
        object->apiobj.field_0x287 = 1;
        object->apiobj.start_position = object->apiobj.position;
        object->field_0x1018 = 0.5f;
    }

    object->ai.opponent = NULL;
    object->ai.nearest_opponent = NULL;
    object->ai.dont_avoid_character = NULL;
    object->last_attacker = NULL;
    object->force_target = NULL;
    object->airborne_collision_target = NULL;
    object->field_0xecc = 0;
    object->field_0xed0 = 0;

    if (!terminal_kill) {
        AISCRIPTPROCESS *processor = reinterpret_cast<AISCRIPTPROCESS *>(&object->ai);
        if (AIScriptSetBaseScriptStateByName(processor, const_cast<char *>("BeenKilled")) != 0 && WORLD != NULL &&
            WORLD->ai_sys != NULL) {
            AIScriptProcess(WORLD->ai_sys, &object->apiobj, &object->ai, processor, FRAMETIME);
        }
    }

    if (terminal_kill && object->apiobj.field_0x27c == -1) {
        const u8 respawn_flags = object->field_0xefa >> 4;
        if (requested_reason == 5 || (respawn_flags & 1) == 0) {
            object->ai.reset_mode = 4;
        } else if ((respawn_flags & 2) != 0) {
            object->field_0x101c = 1.0f;
        } else {
            object->ai.reset_mode = 1;
            object->ai_spawn_delay = 1.0f;
        }
    }
}

void ConstantRumble(GameObject_s *object, f32 strength, f32 phase);

void PowerUp_Update(GameObject_s *object) {
    if (object->field_0xdec > 0.0f) {
        object->field_0xdec -= FRAMETIME;
        if (object->field_0xdec <= 0.0f) {
            GameAudio_PlaySfx(0x52, NULL, 0, 0);
        } else {
            GameAudio_PlaySfx(0x51, NULL, 0, 0);
            ConstantRumble(object, (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) * 0.5f, 0.0f);
        }
    }
}

void GetTakeOverPos(GameObject_s *object, nuvec_s *position) {
    i32 locator = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24)->ride_locator;
    if (locator != -1 && object->apiobj.character_model->points_of_interest[locator] != NULL) {
        NUMTX *matrix = &object->joint_matrices[locator];
        *position = *NUMTX_GET_ROW_VEC(matrix, 3);
    } else {
        position->x = object->apiobj.position.x;
        position->y = object->apiobj.field_0x194;
        position->z = object->apiobj.position.z;
    }
}

extern i16 id_ATAT;
extern i16 id_ATST;
extern i16 id_ATST_LOWRES;
extern u8 PlayerRGB[2][3];
extern i16 id_BUGGY, id_GYROCOPTER, id_BANTHA, id_BOMARRMONK, id_DEWBACK;
extern i16 id_LANDSPEEDER, id_FLASHSPEEDER, id_TAUNTAUN, id_SPEEDERBIKE;
extern i16 id_HEAVYREPEATINGCANNON, id_BIGGUN, id_TROOPERCANNON, id_STAP2, id_CLONEWALKER;
extern "C" f32 AnimDuration(i32, i32, f32, f32, i32);
f32 SpeederChaseATATInOutMul(NUVEC *, NUVEC *);
void PlayJumpSfx(GameObject_s *, i32);
void ReleaseForce(GameObject_s *, i32);
void ReleasePush(GameObject_s *);
void Player_ResetContexts(PLAYERPACKET_s *);
void SetWeaponIn(GameObject_s *);
void GameCam_Blend(GAMECAMERA_s *, f32, f32, i32);
void GameCam_Judder(GAMECAMERA_s *, f32, i32, NUVEC *);
void NewBuzz(nupad_s *, f32, i32);
void Hint_SetComplete(i32);

static void TakeOver_SetAction(GameObject_s *rider, GameObject_s *vehicle) {
    i32 action;
    if (vehicle->id == id_BUGGY)
        action = 0x70;
    else if (vehicle->id == id_GYROCOPTER)
        action = 0x83;
    else if (static_cast<GAMECHARACTERDATA *>(vehicle->apiobj.character_data->field11_0x24)->field275_0x116 == 12) {
        action = CurrentAnim(&vehicle->apiobj.anim_packet) == 3 || CurrentAnim(&vehicle->apiobj.anim_packet) == 0x17
                     ? 0xc4
                     : 0xc3;
    } else if (vehicle->id == id_BANTHA || vehicle->id == id_BOMARRMONK)
        action = 0x70;
    else if (vehicle->id == id_DEWBACK)
        action = 0x83;
    else if (vehicle->id == id_LANDSPEEDER || vehicle->id == id_FLASHSPEEDER)
        action = 0x84;
    else if (vehicle->id == id_TAUNTAUN)
        action = 0x86;
    else if (vehicle->id == id_SPEEDERBIKE)
        action = 0xc0;
    else if (vehicle->id == id_HEAVYREPEATINGCANNON || vehicle->id == id_BIGGUN)
        action = 0xc1;
    else if (vehicle->id == id_TROOPERCANNON) {
        action = rider->apiobj.character_model->model_data_b[0xc2] != NULL ? 0xc2 : 0xc1;
    } else if (vehicle->id == id_STAP2)
        action = 0xc1;
    else if (vehicle->id == id_CLONEWALKER)
        action = 0xc0;
    else {
        rider->context_animation = 0x6c;
        return;
    }
    rider->context_animation = rider->apiobj.character_model->model_data_b[action] != NULL ? action : 0x6c;
}

void ResetForceGlow(PLAYERPACKET_s *);
extern i32 CUTSKIPLOCK;
void GizForce_ResetLOS(GameObject_s *);
void AICreatureResumeScript(GameObject_s *);
void NewBuzzFrames(nupad_s *, i32, i32);

GameObject_s *player_tag_to;
GameObject_s *player_tag_from;
f32 player_tag_timer;
i32 do_player_tag;
static i32 Tag_Mode = 2;
void (*Tag_DrawIconFn)(GameObject_s *);

void Tag_SetMode(i32 mode) {
    Tag_Mode = mode;
}

extern i16 id_LUKESKYWALKERDAGOBAH;
i32 TakeOverYodaSeekDistanceHack(GameObject_s *, GameObject_s *, NUVEC *);
// GCC otherwise rewrites this once-called helper; the original has a separate local body.
static __attribute__((noinline)) GameObject_s *Tag_FindGameObject_TRANSFER(GameObject_s *object) {
    f32 nearest_distance = object->character_context == 0x17 ? 1.44f : 0.48999998f;
    const i32 count = Tag_Mode == 3 ? HIGHGAMEOBJECT : 8;
    GameObject_s *nearest = NULL;
    for (i32 index = 0; index < count; ++index) {
        GameObject_s *candidate = Tag_Mode == 3 ? &Obj[index] : Player[index];
        if (candidate == NULL || (candidate->apiobj.field_0x1f8 & 0x1001) != 0x1001 || candidate == object ||
            candidate->apiobj.field_0x287 != 0 || (candidate->tag_context_flags & 2) != 0) {
            continue;
        }
        const i8 context = candidate->character_context;
        if (context == 0x17 || context == 0x3d || (CInfo[context].flags & 0x8000) != 0 ||
            ((candidate->field_0xf00 & 2) != 0 && object->id != id_LUKESKYWALKERDAGOBAH)) {
            continue;
        }
        NUVEC direction;
        const f32 distance = NuVecDistSqr(&object->apiobj.position, &candidate->apiobj.position, &direction);
        if (distance < nearest_distance) {
            NuVecRotateY(&direction, &direction, -static_cast<u32>(object->apiobj.field_0x276));
            if (direction.z < 0.0f) {
                nearest_distance = distance;
                nearest = candidate;
            }
        }
    }
    return nearest;
}

struct TAGSCRIPTSTATE {
    AISCRIPTPROCESS processor;
    void *field_c8;
};
DECOMP_ASSERT(sizeof(TAGSCRIPTSTATE) == 0xcc, "Tag script state ABI");
DECOMP_ASSERT(offsetof(GameObject_s, tag_cooldown) == 0x7b0, "Tag cooldown ABI");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xef0) == 0xef0, "Tag reset state ABI");
DECOMP_ASSERT(offsetof(GameObject_s, takeover_target) == 0xeb0, "AI takeover target ABI");
DECOMP_ASSERT(offsetof(GameObject_s, field_0xeb4) == 0xeb4, "Object callback ABI");
DECOMP_ASSERT(offsetof(GameObject_s, current_hp) == 0x108b, "Tag signed health ABI");

i32 TagCode(GameObject_s *source, GameObject_s *destination, i32 takeover, i32 blend_camera, i32) {
    if (CUTSKIPLOCK != 0)
        return 0;
    GAMEPAD_s *source_pad = source->pad_gamepad;
    GAMEPAD_s *destination_pad = destination->pad_gamepad;
    f32 source_da8 = source->field_0xda8;
    i8 source_hp = source->current_hp;
    i8 destination_hp = destination->current_hp;
    u8 source_player = source->apiobj.player_controlled;
    u8 destination_player = destination->apiobj.player_controlled;
    TAGSCRIPTSTATE source_script, destination_script;
    u8 source_set, destination_set;
    i32 source_ai = 0, destination_ai = 0;
    if (takeover != 0) {
        memcpy(&source_script, &source->ai, sizeof(source_script));
        source_set = source->ai.creature_set;
        memcpy(&destination_script, &destination->ai, sizeof(destination_script));
        source_ai = source->apiobj.script_enabled;
        destination_ai = destination->apiobj.script_enabled;
        destination_set = destination->ai.creature_set;
    }
    ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(source->player_packet));
    ResetForceGlow(reinterpret_cast<PLAYERPACKET_s *>(destination->player_packet));
    destination->pad_gamepad = source_pad;
    source->pad_gamepad = destination_pad;
    source->apiobj.player_controlled = destination_player;
    destination->apiobj.player_controlled = source_player;
    source->field_0xda8 = destination->field_0xda8;
    destination->field_0xda8 = source_da8;
    COINPACKET *coins = source->coinpacket;
    source->coinpacket = destination->coinpacket;
    destination->coinpacket = coins;
    u8 player_index = source->apiobj.field_0x27c;
    source->apiobj.field_0x27c = destination->apiobj.field_0x27c;
    destination->apiobj.field_0x27c = player_index;
    f32 dec = source->field_0xdec;
    source->field_0xdec = destination->field_0xdec;
    destination->field_0xdec = dec;
    GizForceLOSState_s *los = source->gizforce_los_info;
    source->gizforce_los_info = destination->gizforce_los_info;
    destination->gizforce_los_info = los;
    source->KillTasks();
    destination->KillTasks();
    GizForce_ResetLOS(source);
    GizForce_ResetLOS(destination);
    u32 mask = takeover == 0 ? 0x46f8 : 0x146fd;
    u32 source_state = source->apiobj.field_0x1f4;
    source->apiobj.field_0x1f4 &= ~mask;
    source->apiobj.field_0x1f4 |= destination->apiobj.field_0x1f4 & mask;
    destination->apiobj.field_0x1f4 = (destination->apiobj.field_0x1f4 & ~mask) | (source_state & mask);
    f32 protection = source->spawn_protection_timer;
    source->spawn_protection_timer = destination->spawn_protection_timer;
    destination->spawn_protection_timer = protection;
    void (*eb4)(GameObject_s *) = source->field_0xeb4;
    source->field_0xeb4 = destination->field_0xeb4;
    destination->field_0xeb4 = eb4;
    AICreatureResumeScript(source);
    if (takeover != 0) {
        if (static_cast<i8>(source->apiobj.field_0x27c) != -1)
            Player[static_cast<i8>(source->apiobj.field_0x27c)] = source;
        if (static_cast<i8>(destination->apiobj.field_0x27c) != -1)
            Player[static_cast<i8>(destination->apiobj.field_0x27c)] = destination;
    } else {
        source->hud_icon_timer = 0.0f;
        source->field_0xef0 = 0;
        source->pause_context_state = 0;
        source->input_toggle_hold_time = TOGGLEHOLDTIME;
        source->field_0xefc |= 0x80;
        source->field_0xefe &= ~0x18;
        destination->hud_icon_timer = 2.0f;
        destination->input_toggle_hold_time = TOGGLEHOLDTIME;
        destination->field_0xefc |= 0x80;
        NewBuzzFrames(destination->pad_gamepad->pad, 2, 0);
        if (static_cast<i8>(source->apiobj.field_0x27c) != -1) {
            Player[static_cast<i8>(source->apiobj.field_0x27c)] = source;
            if (source->field_0xcc0 != NULL)
                source->hitpoints =
                    static_cast<GAMECHARACTERDATA *>(source->apiobj.character_data->field11_0x24)->hitpoints;
            else if (WORLD->current_level != VADERC_LDATA)
                source->hitpoints = DEFAULT_PLAYERHITPOINTS;
        } else
            source->hitpoints =
                static_cast<GAMECHARACTERDATA *>(source->apiobj.character_data->field11_0x24)->hitpoints;
        if (static_cast<i8>(destination->apiobj.field_0x27c) != -1) {
            Player[static_cast<i8>(destination->apiobj.field_0x27c)] = destination;
            if (destination->field_0xcc0 != NULL)
                destination->hitpoints =
                    static_cast<GAMECHARACTERDATA *>(destination->apiobj.character_data->field11_0x24)->hitpoints;
            else if (WORLD->current_level != VADERC_LDATA)
                destination->hitpoints = DEFAULT_PLAYERHITPOINTS;
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
        } else
            destination->hitpoints =
                static_cast<GAMECHARACTERDATA *>(destination->apiobj.character_data->field11_0x24)->hitpoints;
        if (source_hp > destination->hitpoints)
            source_hp = destination->hitpoints;
        destination->current_hp = source_hp;
        if (destination->current_hp == 0 && destination->hitpoints != 0)
            destination->current_hp = 1;
        if (WORLD->current_level == VADERC_LDATA || static_cast<i8>(source->apiobj.flags_low) < 0)
            source->current_hp = destination_hp;
        else if ((source->apiobj.character_data->model_flags & 0x20) != 0)
            source->current_hp = source->field_0xe38;
        else
            source->current_hp = source->hitpoints;
    }
    destination->field_0xefe &= ~0x18;
    destination->field_0xef0 = 0;
    destination->pause_context_state = 0;
    destination->tag_cooldown = 1.0f;
    if (source == player_tag_to && destination != player_tag_from) {
        player_tag_to = NULL;
        player_tag_timer = 0.0f;
        player_tag_from = NULL;
        do_player_tag = 0;
    }
    if (takeover != 0) {
        AISCRIPTPROCESS *processor = reinterpret_cast<AISCRIPTPROCESS *>(&destination->ai);
        if (AIScriptSetBaseScriptStateByName(processor, const_cast<char *>("JustBeenTakenOver")) != 0)
            AIScriptProcess(WORLD->ai_sys, &destination->apiobj, &destination->ai, processor, FRAMETIME);
        destination_script.processor.unknown_b0 = destination_set;
        memcpy(&source->ai, &destination_script, sizeof(destination_script));
        source_script.processor.unknown_b0 = source_set;
        memcpy(&destination->ai, &source_script, sizeof(source_script));
        source->ai.creature_set = source_set;
        destination->ai.creature_set = destination_set;
        AICreatureResumeScript(source);
        source->apiobj.script_enabled = destination_ai;
        destination->apiobj.script_enabled = source_ai;
        ReleaseForce(source, 0);
        ReleaseBuildIt(source, 0);
        ReleasePush(source);
        Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(source->player_packet));
        if ((source->field_0xef8 & 0x10) == 0)
            SetWeaponIn(source);
        source->field_0xcc0 = destination;
        source->takeover_target = NULL;
        destination->field_0xcc0 = source;
        source->takeover_source = destination;
        destination->takeover_target = NULL;
        source->character_context = 0x3b;
        TakeOver_SetAction(source, source->field_0xcc0);
        source->external_force = source->apiobj.position;
        source->takeover_timer = 1.0f;
        if (blend_camera != 0)
            GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
        if (static_cast<GAMECHARACTERDATA *>(destination->apiobj.character_data->field11_0x24)->field_0x28 > 0.0f)
            destination->apiobj.velocity.y -=
                0.5f * static_cast<GAMECHARACTERDATA *>(source->apiobj.character_data->field11_0x24)->jump_speed;
    }
    return 1;
}

i32 TakeOverYoda(GameObject_s *rider, GameObject_s *vehicle, i32 blend_camera, i32) {
    ReleaseForce(rider, 0);
    ReleaseBuildIt(rider, 0);
    ReleasePush(rider);
    Player_ResetContexts(reinterpret_cast<PLAYERPACKET_s *>(rider->player_packet));
    if ((rider->field_0xef8 & 0x10) == 0)
        SetWeaponIn(rider);
    rider->field_0xcc0 = vehicle;
    vehicle->field_0xcc0 = rider;
    rider->takeover_target = NULL;
    rider->takeover_source = vehicle;
    vehicle->takeover_target = NULL;
    rider->character_context = 0x3b;
    TakeOver_SetAction(rider, rider->field_0xcc0);
    rider->external_force = rider->apiobj.position;
    rider->takeover_timer = 1.0f;
    if (blend_camera != 0)
        GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
    NewBuzz(vehicle->pad_gamepad->pad, 0.1f, 0);
    GameCam_Judder(GameCam, 0.1f, 0, NULL);
    Hint_SetComplete(0x28c);
    return 1;
}

void TakeOver2GetIn(GameObject_s *rider, GameObject_s *vehicle) {
    GameObject_s *object = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) == 0x1001 && object->character_context == 0x3c &&
            object->field_0x780 == vehicle)
            return;
    }
    if (rider == NULL || (rider->apiobj.flags_low & 1) == 0 || vehicle == NULL ||
        (vehicle->apiobj.flags_low & 1) == 0 || rider->field_0xcc0 != NULL || vehicle->field_0xcc0 != NULL)
        return;
    rider->character_context = 0x3c;
    if (rider->apiobj.character_model->model_data_b[6] != NULL) {
        ResetAnimPacket(&rider->apiobj.anim_packet, -1);
        rider->context_animation = 6;
        rider->airborne_action_duration = AnimDuration(rider->id, 6, 0.0f, 0.0f, 1);
    } else {
        rider->context_animation = 5;
        rider->airborne_action_duration = 0.5f;
    }
    if (vehicle->id == id_ATAT && WORLD->current_level == SPEEDERCHASEA_LDATA) {
        NUVEC position;
        GetTakeOverPos(vehicle, &position);
        rider->airborne_action_duration *= 2.0f * SpeederChaseATATInOutMul(&rider->apiobj.position, &position);
    }
    rider->context_animation_timer = 0.0f;
    rider->takeover_target = NULL;
    rider->external_force = rider->apiobj.position;
    rider->takeover_start_angle = rider->apiobj.field_0x276;
    rider->field_0x780 = vehicle;
    vehicle->takeover_target = NULL;
    PlayJumpSfx(rider, 0);
}

extern i32 VehicleArea;
extern i32 menu_i_pack;
extern ADDGAMEMSG AddGameMsg_Default;
extern char *ASCII_DOWN;
void CollideGameObjects(WORLDINFO_s *world);
i32 GetMenuID();
i32 FindGameMsgsWithID(i32, i32, i32, GAMEMESSAGE_s *);
GAMEMESSAGE_s *AddGameMsg(ADDGAMEMSG *);
void Hint_CancelCurrent();
i32 InCollectList_Index(i32, COLLECTID *, i32);
i32 NuIOS_AreInAppPurchasesAvailable();
i32 NuIOS_CanMakeInAppPurchases();
void GameCam_HitRoll();
void GameCam_Reset(GAMECAMERA_s *);
void RememberPlayerIDs(i32, i32, i32);
void Tag_NewTransfer(GameObject_s *, GameObject_s *);

static void DrawPackButton(GAMEMESSAGE_s *, nuvec_s *, float) {
}

void Tag_Check(GameObject_s *object) {
    DECOMP_ASSERT(offsetof(GameObject_s, tag_player_index) == 0x7b4, "Tag player index ABI");
    DECOMP_ASSERT(offsetof(GameObject_s, tag_flags) == 0x7b5, "Tag flags ABI");
    DECOMP_ASSERT(offsetof(GameObject_s, pending_tag_target) == 0x7b8, "Pending tag target ABI");
    GameObject_s *target = NULL;
    i32 result;
    if ((object->tag_flags & 1) != 0 || object->character_context == 0x51) {
        object->field_0xefe &= ~0x18;
        object->hud_icon_timer = 0.0f;
        object->field_0xef0 = 0.0f;
        object->pause_context_state = 0.0f;
        if (Arcade == 0)
            return;
    } else if (Arcade != 0) {
        object->field_0xef0 = 0.0f;
        object->pause_context_state = 0.0f;
        object->field_0xefe &= ~0x18;
    }
    if (object->apiobj.field_0x27c == -1)
        return;
    if ((object->tag_flags & 4) != 0) {
        object->tag_cooldown -= FRAMETIME;
        if (object->tag_cooldown > 0.0f && object->pending_tag_target != NULL) {
            target = object->pending_tag_target;
            if ((target->apiobj.field_0x1f8 & 0x1001) != 0x1001) {
                object->pending_tag_target = NULL;
            } else {
                result = (object->field_0xf00 & 2) != 0 ? TakeOverYoda(object, target, 0, 1)
                                                        : TagCode(object, target, (object->tag_flags >> 3) & 1, 0, 1);
                if (result == 1)
                    goto tagged;
                if (result != 0)
                    goto draw_icon;
            }
        }
        object->tag_flags &= ~4;
        goto draw_icon;
    }
    object->tag_player_index = -1;
    if (!object->apiobj.player_controlled || object->apiobj.field_0x287 != 0 ||
        (CInfo[object->character_context].flags & 0x800) != 0 || object->apiobj.script_enabled ||
        !(object->tag_cooldown <= 0.0f))
        goto draw_icon;
    if (VehicleArea != 0) {
        if ((object->pad_gamepad->buttons_pressed & GAMEPAD_TAG) != 0)
            object->hud_icon_timer = 2.0f;
        goto draw_icon;
    }
    if (Arcade == 0 && Tag_Mode == 3 && GetMenuID() == -1) {
        if (FindGameMsgsWithID(-1, 0, -1, NULL) != 0) {
            Hint_CancelCurrent();
        } else {
            i32 pack_index = -1;
            f32 nearest = 0.7f * 0.7f * 2.0f;
            NUVEC difference;
            for (i32 i = 0; i < 11; ++i) {
                if (!Store_IsPackUnlocked(i) && StorePack[i].id != NULL) {
                    f32 distance =
                        NuVecDistSqr(&object->apiobj.position, &StorePack[i].custodian_position, &difference);
                    if (distance < nearest) {
                        nearest = distance;
                        pack_index = i;
                    }
                }
            }
            if (pack_index != -1) {
                STOREPACK *pack = &StorePack[pack_index];
                NUVEC position = pack->custodian_position;
                position.y += CDataList[*pack->id].field16_0x38 + 0.075f;
                ADDGAMEMSG message = AddGameMsg_Default;
                message.text = TTab[pack->message_text_index];
                message.position = &position;
                message.red = 255;
                message.green = 191;
                message.blue = 0;
                message.flags = 0x1083;
                message.field_0x44 = reinterpret_cast<void *>(DrawPackButton);
                message.field_0x4e = pack_index;
                message.scale = 0.4f;
                AddGameMsg(&message);
                Hint_CancelCurrent();
            }
        }
    }
    if ((object->pad_gamepad->buttons_pressed & GAMEPAD_TAG) == 0)
        goto draw_icon;
    if (CUTSKIPLOCK != 0) {
        object->hud_icon_timer = 2.0f;
        object->input_toggle_hold_time = TOGGLEHOLDTIME;
        goto no_target;
    }
    if (Arcade != 0)
        goto no_target;
    if (Tag_Mode == 1 || Tag_Mode == 3) {
        if (object->character_context == 0x17 || object->character_context == -1 ||
            (CInfo[object->character_context].flags & 4) != 0 ||
            (CInfo[object->character_context].parameter & 0x10) != 0) {
            target = Tag_FindGameObject_TRANSFER(object);
        }
        if (Tag_Mode == 1) {
            if (target != NULL && target->apiobj.field_0x27c != -1)
                object->tag_player_index = target->apiobj.field_0x27c;
            goto select_player;
        }
        if (target == NULL)
            goto no_target;
        if (InCollectList_Index(target->id, NULL, 0) != -1) {
            i32 pack = Store_FindPack(target->id, NULL);
            if (pack != -1 && static_cast<i32>(target->apiobj.field_0x1f4) < 0 && !Store_IsPackUnlocked(pack)) {
                Hint_CancelCurrent();
                if (NuIOS_AreInAppPurchasesAvailable() && NuIOS_CanMakeInAppPurchases()) {
                    GameAudio_PlaySfx(0x30, NULL, 0, 0);
                    menu_i_pack = pack;
                    NewMenu(0x14, -1, -1);
                } else {
                    GameAudio_PlaySfx(0x32, NULL, 0, 0);
                    GameCam_HitRoll();
                }
                goto draw_icon;
            }
        } else if (InCollectList_Index(target->id, NULL, 0) == -1) {
            GameAudio_PlaySfx(0x32, &object->apiobj.collision_position, 0, 0);
            NewRumble(object->pad_gamepad->pad, 0.5f, 0);
            AISCRIPTPROCESS *processor = reinterpret_cast<AISCRIPTPROCESS *>(&target->ai);
            if (AIScriptSetBaseScriptStateByName(processor, const_cast<char *>("MapRunAway")) != 0)
                AIScriptProcess(WORLD->ai_sys, &target->apiobj, &target->ai, processor, FRAMETIME);
            goto no_target;
        }
        goto attempt_tag;
    } else {
        i32 index = object->apiobj.field_0x289;
        for (i32 remaining = HIGHGAMEOBJECT - 1; remaining > 0; --remaining) {
            if (++index == HIGHGAMEOBJECT)
                index = 0;
            GameObject_s *candidate = &Obj[index];
            if (candidate == object || candidate == NULL || (candidate->apiobj.field_0x1f8 & 0x1001) != 0x1001 ||
                candidate->apiobj.field_0x287 != 0 || candidate->apiobj.field_0x27c == -1 ||
                (candidate->tag_flags & 2) != 0 || (candidate->field_0xf00 & 2) != 0 ||
                (CInfo[candidate->character_context].flags & 0x800) != 0)
                continue;
            object->tag_player_index = candidate->apiobj.field_0x27c;
            break;
        }
    }
select_player:
    if (object->tag_player_index < 0)
        goto no_target;
    target = Player[object->tag_player_index];
    if (target == NULL)
        goto no_target;
attempt_tag:
    if ((target->field_0xf00 & 2) != 0) {
        TakeOver2GetIn(target, object);
        goto draw_icon;
    }
    if (object->apiobj.player_controlled && target->apiobj.player_controlled) {
        if (player_tag_timer > 0.0f && object == player_tag_to && target == player_tag_from)
            do_player_tag = 1;
        else {
            player_tag_to = target;
            player_tag_from = object;
            player_tag_timer = 0.5f;
        }
        goto draw_icon;
    }
    result = TagCode(object, target, 0, 0, 1);
    if (result == 2) {
        object->pending_tag_target = target;
        object->tag_flags = (object->tag_flags | 4) & ~8;
        object->tag_cooldown = 1.0f;
        goto draw_icon;
    }
    if (result != 1)
        goto draw_icon;
tagged:
    object->tag_flags &= ~4;
    GameAudio_PlaySfx(0x21, &object->apiobj.collision_position, 0, 0);
    if (target->apiobj.model_draw_result != 0)
        GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
    else
        GameCam_Reset(GameCam);
    RememberPlayerIDs(0, Player[0] != NULL ? Player[0]->id : -1, Player[1] != NULL ? Player[1]->id : -1);
    if ((Tag_Mode & ~2) == 1)
        Tag_NewTransfer(object, target);
    if (static_cast<u8>(object->apiobj.field_0x27c) < 2 && static_cast<u8>(target->apiobj.field_0x27c) < 2)
        ResetTimer(&JoinInTimer, 0.0f);
    goto draw_icon;
no_target:
    if (object->field_0xcc0 == NULL)
        object->hud_icon_timer = 2.0f;
draw_icon:
    if (Tag_DrawIconFn != NULL)
        Tag_DrawIconFn(object);
}

void PowerUp_AddPart(nuvec_s *, nuvec_s *, float, float) {
}

void ScaleGameObject(GameObject_s *object) {
    const f32 scale = object->apiobj.field_0xa8;
    CHARACTERDATA *character = object->apiobj.character_data;
    object->apiobj.scaled_radius = character->field13_0x2c * scale;
    object->apiobj.collision_radius = object->field_0x1008 * scale;
    object->apiobj.collision_height = object->apiobj.collision_radius * object->collision_y_scale;
    object->apiobj.scaled_height = (character->field16_0x38 - character->field15_0x34) * object->field_0x1004;
}

void DestroySnakeBody(GameObject_s *obj);

void RemoveGameObject(GameObject_s *obj, i32) {
    if (obj == NULL) {
        return;
    }

    obj->KillTasks();
    obj->ClearAddons();
    obj->ClearMechObjectInterface();

    const u32 low_mask = ~obj->apiobj.field_0x1e4;
    const u32 high_mask = ~obj->apiobj.field_0x1e8;
    const u8 index = obj->apiobj.field_0x289;
    const u32 index_low_mask = index < 32 ? ~(1u << index) : ~0u;
    const u32 index_high_mask = index < 32 ? ~0u : ~(1u << (index - 32));
    for (i32 i = 0; i < HIGHGAMEOBJECT; i++) {
        Obj[i].apiobj.field_0x1ec &= low_mask;
        Obj[i].apiobj.field_0x1f0 &= high_mask;
        Obj[i].apiobj.field387_0x2a0 &= index_low_mask;
        Obj[i].apiobj.field388_0x2a4 &= index_high_mask;
        Obj[i].field_0xebc &= index_low_mask;
        Obj[i].field_0xec0 &= index_high_mask;
        Obj[i].field_0xec4 &= index_low_mask;
        Obj[i].field_0xec8 &= index_high_mask;
    }

    if (obj->pad_gamepad != NULL) {
        obj->pad_gamepad->allocated_5a &= ~1u;
    }
    DestroySnakeBody(obj);
    APIObjectDestroy(WORLD->api_object_sys, &obj->apiobj);

    HIGHGAMEOBJECT = 0;
    for (i32 i = 0; i < 64; i++) {
        if ((Obj[i].apiobj.field_0x1f8 & 1) != 0) {
            HIGHGAMEOBJECT = i + 1;
        }
    }
    for (i32 i = 0; i < 8; i++) {
        if (Player[i] == obj) {
            Player[i] = NULL;
        }
    }
}

extern u16 TargetDeg_Near, TargetDeg_Mid, TargetDeg_Far;
extern f32 TargetDist_Near2, TargetDist_Mid2;
BOLTTYPE_s *BoltType_FindByID(i32, WORLDINFO_s *);
GameObject_s *TargetGameObject(GameObject_s *object, nuvec_s *position, nuvec_s *direction, f32 radius,
                               f32 range_squared, u32 model_mask, i32 directional, i32 require_drawn, i32 bolt_id) {
    NUVEC aim = *direction;
    BOLTTYPE_s *bolt = BoltType_FindByID(bolt_id, WORLD);
    u16 near_angle = TargetDeg_Near, mid_angle = TargetDeg_Mid, far_angle = TargetDeg_Far;
    f32 near_distance = TargetDist_Near2, mid_distance = TargetDist_Mid2;
    if ((object->apiobj.flags_low & 0x80) != 0 &&
        (WORLD->current_level == DEATHSTAR2BATTLEA_LDATA || WORLD->current_level == ASTEROIDCHASED_LDATA)) {
        near_angle = 0x2aaa;
        mid_angle = 0x1c71;
        far_angle = 0xe38;
        near_distance = 6.25f;
        mid_distance = 100.0f;
    }
    f32 min_x = position->x - radius, max_x = position->x + radius;
    f32 min_z = position->z - radius, max_z = position->z + radius;
    if (directional != 0) {
        aim = *direction;
        if (bolt != NULL && (bolt->field_60 & 0x20000) != 0) {
            aim.y = 0.0f;
            NuVecNorm(&aim, &aim);
        }
    }
    i32 arcade = Arcade_GetMode(NULL) == 99;
    GameObject_s *target = Obj;
    GameObject_s *best = NULL, *previous = NULL;
    f32 nearest_distance = range_squared;
    i32 count = HIGHGAMEOBJECT;
    for (i32 i = 0; i < count; ++i, ++target) {
        if (target == object || (target->apiobj.flags_high & 0x10) == 0 || target->apiobj.field_0x287 != 0 ||
            (require_drawn != 0 && target->apiobj.model_draw_result == 0) || (target->field_0xf00 & 1) != 0)
            continue;
        i8 context = target->character_context;
        if ((static_cast<u8>(context) & 0xfd) == 0x39 || context == 0x3c || (CInfo[context].flags & 0x8000) != 0)
            continue;
        if (model_mask != 0) {
            if ((target->apiobj.character_data->model_flags & model_mask) == 0)
                continue;
        } else if (arcade == 0 && ((target->apiobj.field_0x1f4 ^ object->apiobj.field_0x1f4) & 0x10001) == 0)
            continue;
        if (static_cast<u8>(context - 0x5f) <= 1)
            continue;
        if (arcade == 0 && (object->apiobj.flags_low & 0x80) != 0 && (target->field_0xf02 & 0x80) == 0) {
            u32 *mask = WORLD->api_object_sys->hostility_masks[object->apiobj.field_0x289];
            u64 hostility = static_cast<u64>(mask[0]) | (static_cast<u64>(mask[1]) << 32);
            if (((hostility >> (target->apiobj.field_0x289 & 63)) & 1) == 0)
                continue;
        }
        if (target->apiobj.collision_position.x < min_x || target->apiobj.collision_position.x > max_x ||
            target->apiobj.collision_position.z < min_z || target->apiobj.collision_position.z > max_z)
            continue;
        if (WORLD->current_level == DEATHSTARRESCUEA_LDATA && target->apiobj.field_0x27c == -1 &&
            target->apiobj.position.y < -1.5f && GameCam->sock_position.location.sock == 2)
            continue;
        NUVEC delta;
        f32 distance = NuVecDistSqr(&target->apiobj.collision_position, position, &delta);
        if (nearest_distance > distance) {
            if (directional == 0)
                NuVecRotateY(&aim, &v001,
                             NuAtan2D(target->apiobj.collision_position.x - position->x,
                                      target->apiobj.collision_position.z - position->z));
            if (bolt != NULL && (bolt->field_60 & 0x20000) != 0)
                delta.y = 0.0f;
            NuVecNorm(&delta, &delta);
            f32 dot = NuVecDot(&delta, &aim);
            u16 angle = near_distance > distance && directional != 0
                            ? near_angle
                            : (mid_distance > distance ? mid_angle : far_angle);
            if (dot > NuTrigTable[((angle + 0x4000) >> 1) & 0x7fff]) {
                if (object->collision_target == target)
                    previous = target;
                else {
                    nearest_distance = distance;
                    best = target;
                }
            }
        }
        count = HIGHGAMEOBJECT;
    }
    return best != NULL ? best : previous;
}

extern i32 nethost;
extern i16 id_PKDROID;
void ResetAICreature(GameObject_s *, AISYS_s *);
void SpawnCreatureFromCrate(GameObject_s *, f32, f32);
void SetToLastSafePos(GameObject_s *);
GameObject_s *GetOtherActivePlayer(GameObject_s *);
void FreeTorpedoPacket(TORPEDOPACKET_s **);
void TakeOverGameObject(GameObject_s *, GameObject_s *, i32, i32);

void ManageGameObjects() {
    GameObject_s *object = Obj;
    memset(aicreature_sets_alive, 0, sizeof(aicreature_sets_alive));
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *candidate = &object[i];
        if ((candidate->apiobj.field_0x1f8 & 0x1000) != 0 && candidate->apiobj.field_0x287 == 0 &&
            candidate->ai.creature_set != 0)
            ++aicreature_sets_alive[candidate->ai.creature_set - 1];
    }
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 1) == 0)
            continue;
        if ((object->apiobj.field_0x1f4 & 0x40000) != 0) {
            if (nethost != 0 && (object->apiobj.field_0x1f8 & 0x1000) == 0 && object->ai.field_0x134 == 0xff &&
                (object->ai.reset_mode & ~2) != 1 && (object->apiobj.field_0x1f4 & 0x4000) != 0)
                AIScriptProcess(WORLD->ai_sys, &object->apiobj, &object->ai, &object->ai.script_process, FRAMETIME);
            continue;
        }
        if ((object->apiobj.field_0x1f8 & 0x1000) != 0) {
            if (object->apiobj.field_0x287 == 0)
                continue;
            if (object->field_0xeb4 != NULL) {
                object->field_0xeb4(object);
                object->field_0xeb4 = NULL;
            }
            if (object->field_0x101c > 0.0f) {
                object->field_0x101c -= FRAMETIME;
                if (object->field_0x101c <= 0.0f) {
                    object->field_0x101c = 0.0f;
                    GameObject_s *other = NULL;
                    if (WORLD->current_level == PODSPRINTA_LDATA)
                        other = GetOtherActivePlayer(object);
                    if (other != NULL) {
                        object->apiobj.start_position = other->apiobj.position;
                        object->apiobj.position = other->apiobj.position;
                        object->field_0x10c8 = other->apiobj.position.x;
                        object->field_0x10cc = other->apiobj.position.y;
                        object->field_0x10d0 = other->apiobj.position.z;
                        GameObjectOrigin(object);
                        object->apiobj.velocity = other->apiobj.velocity;
                        object->apiobj.movement_facing_angle = other->apiobj.field_0x276;
                        object->apiobj.facing_angle = other->apiobj.field_0x276;
                        object->apiobj.field_0x276 = other->apiobj.field_0x276;
                    } else {
                        SetToLastSafePos(object);
                        GameObjectOrigin(object);
                        object->apiobj.velocity.x = object->apiobj.velocity.z = 0.0f;
                        object->apiobj.velocity.y = -0.1f;
                        object->apiobj.facing_angle = object->apiobj.movement_facing_angle;
                        object->apiobj.field_0x276 = object->apiobj.movement_facing_angle;
                    }
                    u16 saved_flags = object->apiobj.field_0x1f8 & 0x2000;
                    ResetPlayerMoves(object);
                    object->apiobj.field_0x287 = 0;
                    object->apiobj.field_0x1f8 = (object->apiobj.field_0x1f8 & ~0x2000) | saved_flags;
                    object->current_hp = object->hitpoints;
                    object->field_0xe37 = object->apiobj.character_data->game_character->field_0xf5;
                    object->field_0xe38 = 4;
                    object->field_0xefc |= 0x80;
                    object->field_0x7a5 = 0xff;
                    if (object->apiobj.field_0x27c != -1)
                        object->spawn_protection_timer = 2.5f;
                    AISysGetCharacterPathPos(WORLD->ai_sys, &object->apiobj, &object->ai, 0xff, 1);
                    GameObject_s *vehicle = object->takeover_source;
                    if (vehicle != NULL && (vehicle->field_0xcc0 == NULL || vehicle->field_0xcc0 == object) &&
                        (vehicle->apiobj.field_0x1f8 & 0x1000) != 0 && vehicle->field_0x7a5 != 0x2b &&
                        vehicle->apiobj.field_0x287 == 0) {
                        GetTakeOverPos(vehicle, &object->apiobj.position);
                        TakeOverGameObject(object, object->takeover_source, 0, 1);
                    }
                    if ((object->apiobj.field_0x1f8 & 0x80) != 0 && WORLD->current_level == SPEEDERCHASEA_LDATA &&
                        disable_narrow_socks == 0)
                        GameCam_Blend(GameCam, 0.5f, 0.0f, 0);
                } else if (WORLD->current_level == PODSPRINTA_LDATA) {
                    GameObject_s *other = GetOtherActivePlayer(object);
                    if (other != NULL)
                        object->apiobj.position = other->apiobj.position;
                }
                continue;
            }
            object->timer_1014 += FRAMETIME;
            if (!(object->timer_1014 >= object->field_0x1018))
                continue;
            object->timer_1014 = object->field_0x1018;
            u64 contact_mask = ~object->apiobj.collision_identity_mask;
            u64 object_mask = ~(static_cast<u64>(1) << object->apiobj.field_0x289);
            for (i32 j = 0; j < HIGHGAMEOBJECT; ++j) {
                Obj[j].apiobj.collision_contact_mask &= contact_mask;
                Obj[j].apiobj.ai_awareness_mask &= object_mask;
                Obj[j].ai_seen_mask &= object_mask;
                Obj[j].ai_opponent_exclusion_mask &= object_mask;
            }
            if ((object->apiobj.field_0x1f4 & 0x4000) != 0) {
                if ((object->apiobj.field_0x1f4 & 0x400) != 0) {
                    AICREATURE *creature = &WORLD->ai_sys->creatures[object->ai.field_0x134];
                    if ((object->field_0xefa & 0x10) != 0) {
                        if ((object->field_0xefa & 0x20) == 0) {
                            object->field_0x101c =
                                object->id == id_STAP2 && WORLD->current_level == NEGOTIATIONSC_LDATA ? 5.0f : 1.0f;
                            continue;
                        }
                    } else {
                        u32 limit = object->ai_respawn_count + 1;
                        if (creature->max_respawn_count != -1 && object->ai.respawn_locator == NULL)
                            limit = creature->min_respawn_count +
                                    (Game.difficulty - 1) *
                                        (creature->max_respawn_count - creature->min_respawn_count) / 9 +
                                    1;
                        if (limit <= object->ai_respawn_count) {
                            object->apiobj.field_0x1f8 &= ~0x1000;
                            object->ai.reset_mode = 4;
                            continue;
                        }
                    }
                    object->apiobj.field_0x1f8 &= ~0x1000;
                    object->ai.reset_mode = 1;
                    f32 blend = 1.0f - (static_cast<f32>(static_cast<u32>(Game.difficulty)) - 1.0f) / 9.0f;
                    object->ai_spawn_delay =
                        creature->max_respawn_time * blend + creature->min_respawn_time * (1.0f - blend);
                } else {
                    object->field_0x101c = 2.0f;
                }
                continue;
            }
            if ((object->field_0xefa & 0x10) != 0) {
                object->field_0x101c = 1.0f;
                continue;
            }
            goto remove_object;
        }
        if (object->ai.field_0x134 != 0xff) {
            AISYS *system = WORLD->ai_sys;
            AICREATURE *creature = &system->creatures[object->ai.field_0x134];
            if (object->ai.reset_mode == 0) {
                switch (creature->activate_type) {
                    case 0:
                        if (Game.difficulty >= creature->activation_difficulty)
                            ResetAICreature(object, system);
                        else
                            object->ai.reset_mode = 4;
                        break;
                    case 1:
                        if (creature->activate_area == NULL) {
                            creature->activate_type = 0;
                        } else {
                            i32 area = creature->activate_area - system->areas;
                            i64 mask = static_cast<i32>(1u << (area & 31));
                            if (player != NULL && (player->apiobj.ai_area_mask & mask) != 0) {
                                if (Game.difficulty < creature->activation_difficulty) {
                                    object->ai.reset_mode = 4;
                                } else if (creature->count > 1 && creature->start_stagger > 0.0f &&
                                           object->ai.group_member_index != 0) {
                                    object->ai.reset_mode = 1;
                                    object->ai_spawn_delay =
                                        static_cast<f32>(static_cast<u32>(object->ai.group_member_index)) *
                                        creature->start_stagger;
                                } else {
                                    ResetAICreature(object, system);
                                }
                            }
                        }
                        break;
                    case 2:
                        if (Game.difficulty >= creature->activation_difficulty)
                            AIScriptProcess(system, &object->apiobj, &object->ai, &object->ai.script_process,
                                            FRAMETIME);
                        break;
                }
            } else if (object->ai.reset_mode == 1) {
                AIGROUP *group = object->ai.group;
                if (group != NULL && !group->can_respawn)
                    continue;
                if (creature->activate_type == 1) {
                    i32 area = creature->activate_area - system->areas;
                    i64 mask = static_cast<i32>(1u << (area & 31));
                    if (player == NULL || (player->apiobj.ai_area_mask & mask) == 0 ||
                        Game.difficulty < creature->activation_difficulty) {
                        if (creature->count > 1 && creature->start_stagger > 0.0f)
                            object->ai_spawn_delay = static_cast<f32>(static_cast<u32>(object->ai.group_member_index)) *
                                                     creature->start_stagger;
                        continue;
                    }
                }
                object->ai_spawn_delay -= FRAMETIME;
                if (!(object->ai_spawn_delay <= 0.0f))
                    continue;
                if (group != NULL) {
                    AILOCATOR *locator = object->ai.locator;
                    AILOCATOR *respawn_locator = object->ai.respawn_locator;
                    for (i32 j = 0; j < group->member_count; ++j) {
                        APIOBJECT *member = group->members[j];
                        if (member != NULL) {
                            GameObject_s *target = member->objptr;
                            target->ai_spawn_delay = 0.0f;
                            target->ai.locator = locator;
                            target->ai.respawn_locator = respawn_locator;
                            ResetAICreature(target, WORLD->ai_sys);
                            group = object->ai.group;
                            if (group->leader == NULL)
                                group->leader = group->members[j];
                        }
                        group = object->ai.group;
                    }
                    group->is_in_formation = 0;
                    object->ai.group->is_reversed = 0;
                } else {
                    object->ai_spawn_delay = 0.0f;
                    if (object->id == id_PKDROID) {
                        ResetAICreature(object, system);
                        SpawnCreatureFromCrate(object, 3.0f, 0.0f);
                    } else {
                        ResetAICreature(object, WORLD->ai_sys);
                    }
                }
            }
            continue;
        }
        if (object->ai.reset_mode == 1) {
            object->ai_spawn_delay -= FRAMETIME;
            if (object->ai_spawn_delay <= 0.0f) {
                object->ai.reset_mode = 2;
                object->ai_spawn_delay = 0.0f;
                object->apiobj.field_0x1f8 |= 0x1000;
            }
            continue;
        }
        if (object->ai.reset_mode == 3)
            continue;
        if ((object->apiobj.field_0x1f4 & 0x4000) != 0) {
            AIScriptProcess(WORLD->ai_sys, &object->apiobj, &object->ai, &object->ai.script_process, FRAMETIME);
            continue;
        }
    remove_object:
        AIGROUP *group = object->ai.group;
        if (group != NULL) {
            if (group->rows[0].is_alive == 0 && group->rows[1].is_alive == 0 && group->rows[2].is_alive == 0 &&
                group->rows[3].is_alive == 0) {
                DestroyAIGroup(group);
            } else {
                for (i32 j = 0; j < group->member_count; ++j) {
                    if (group->members[j] == &object->apiobj)
                        group->members[j] = NULL;
                }
            }
        }
        FreeTorpedoPacket(&object->torpedo);
        RemoveGameObject(object, 1);
    }
}

f32 PowerUp_GetPanelY(i32) {
    return 0.0f;
}

void PowerUp_Particles(WORLDINFO_s *, nuvec_s *) {
}

extern i32 adaptivedifficulty[3];
extern i8 (*adtab)[4];
f32 Hub_PadSpeed[2];
u16 Hub_PadAngle[2];
GameObject_s *CarWashHack;
f32 AIFireIntervalMul = 1.0f;
f32 GhostLightMul = 1.0f;
f32 GhostLightTargetMul = 1.0f;
f32 CurrentSpeedOverride;
f32 lightning_sizew[2] = {0.02f, 0.02f};
f32 lightning_sizel[2] = {0.1f, 0.1f};
f32 lightning_sizewab[2] = {0.01f, 0.01f};
f32 lightning_endw[2] = {0.1f, 0.1f};
u32 lightning_col[2] = {0xff808040, 0xff808040};
f32 CurrentSpeed = 10.0f;
f32 BaseCurrentSpeed = 10.0f;
extern AREADATA *PODSPRINT_ADATA;
extern i16 id_ANAKINSNEWPOD, id_ANAKINSNEWPODGREEN, id_SEBULBASPOD;
extern f32 tieonsfxwait, tieoffsfxwait;
void MovePlayer_NETWORK(GameObject_s *);
void SetFlicker(GameObject_s *, f32);
void UpdateRumble(RUMBLEPACKET *);
void Player_ToggleCharacter(GameObject_s *, i32, i32);
void AveragePlayerCurrentSpeedMul();
void RegenerateHearts(GameObject_s *);
void UpdateLastSafePosition(GameObject_s *);
void AddSurfaceRipples(GameObject_s *);
void AddSurfaceDebris(GameObject_s *);

void GameAntinode_Update(GAMEANTINODESYS_s *system);

void UpdateGameObjects(WORLDINFO_s *world) {
    Hub_PadSpeed[1] = 0.0f;
    Hub_PadSpeed[0] = 0.0f;
    SetPlayer();
    AIFireIntervalMul = 1.0f;
    CarWashHack = 0;
    if (adtab[adaptivedifficulty[0]][3] == 1)
        AIFireIntervalMul = 0.5f;
    else if (adtab[adaptivedifficulty[0]][3] == -1)
        AIFireIntervalMul = 2.0f;
    GhostLightMul = SeekLinearF(GhostLightMul, (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) * 0.5f + 0.75f,
                                FRAMETIME + FRAMETIME);
    if (GhostLightMul == GhostLightTargetMul) {
        GhostLightTargetMul = (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) * 0.5f + 0.75f;
    }

    // AI updates are scheduled before the object/player movement passes. The
    // elapsed value is accumulated until an object becomes eligible for its
    // next script and path update.
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *object = &Obj[i];
        const u16 character_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER;
        if ((object->apiobj.field_0x1f8 & character_flags) != character_flags) {
            continue;
        }

        MechAddonCollection *addons = object->GetAddons(false);
        if (addons != NULL)
            addons->Process(MechAddon::PROCESS_STAGE_0, FRAMETIME);
        if ((object->field_0xf00 & GAME_OBJECT_AI_UPDATE_PROCESS) != 0) {
            object->ai_elapsed_time = 0.0f;
        }
        object->ai_elapsed_time += FRAMETIME;

        const bool force_update =
            timebase_updates == 0 || (object->apiobj.field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) == 0 ||
            (object->field_0xefb & 8) != 0 || (object->field_0xf00 & GAME_OBJECT_AI_UPDATE_SPECIAL_STATE) != 0 ||
            (CInfo[object->character_context].flags & 0x200) != 0 || object->apiobj.supporting_platform_id != -1 ||
            (object->field_0xcc0 != NULL && object->character_context == CHARACTER_CONTEXT_LINKED_OBJECT) ||
            Technos_FindControllingTechno(object) != NULL ||
            (object->active_trigger_set != NULL && (object->active_trigger_set->flags & 2) != 0) ||
            (object->ai.group != NULL && object->ai.group->is_in_formation);
        const i32 interval = force_update ? 1 : GameObjectAIUpdateInterval(world, object);

        if (interval <= 1) {
            object->field_0xf00 |= GAME_OBJECT_AI_UPDATE_FORCED | GAME_OBJECT_AI_UPDATE_PROCESS;
        } else {
            object->field_0xf00 &= ~GAME_OBJECT_AI_UPDATE_FORCED;
            const u32 update_phase =
                static_cast<u32>(object->apiobj.field_0x289) + static_cast<u32>(GameTimer.update_count);
            if (update_phase % static_cast<u32>(interval) == 0) {
                object->field_0xf00 |= GAME_OBJECT_AI_UPDATE_PROCESS;
            } else {
                object->field_0xf00 &= ~GAME_OBJECT_AI_UPDATE_PROCESS;
            }
        }

        // AI positions between scheduled updates are render extrapolations.
        // The target restores the last terrain-resolved position here before
        // GameAIProcess and the movement/terrain passes (0x140c2..0x140f0),
        // so an extrapolated floor offset never becomes the next collision
        // query's starting point.
        const u32 authoritative_position_flags =
            APIOBJECT_MOTION_FLAG_AI_CONTROLLED | APIOBJECT_STATE_FLAG_IGNORE_DOORS;
        if ((object->field_0xf00 & GAME_OBJECT_AI_UPDATE_PROCESS) != 0 &&
            (object->apiobj.field_0x1f4 & authoritative_position_flags) == APIOBJECT_MOTION_FLAG_AI_CONTROLLED) {
            object->apiobj.position.x = object->field_0x10c8;
            object->apiobj.position.y = object->field_0x10cc;
            object->apiobj.position.z = object->field_0x10d0;
        }
    }

    GameAntinode_Update(world->game_antinode_sys);
    if (TimingBarSet == 4)
        TBOPENFN("AIProc", 4);
    GameAIProcess();
    if (TimingBarSet == 4)
        TBCLOSEFN("AIProc", 4);

    // AI movement precedes the separate player movement pass.
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *object = &Obj[i];
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 ||
            (object->apiobj.field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) == 0)
            continue;
        object->apiobj.flags_high = (object->apiobj.flags_high & ~0x40) | ((object->field_0x7a5 == 0) << 6);
        object->field_0x1024 -= FRAMETIME;
        if (object->field_0x1024 < -1.0f)
            SetFlicker(object, 0.0f);
        if (object->timer_d28 > 0.0f)
            object->timer_d28 -= FRAMETIME;
        if (object->timer_d50 > 0.0f)
            object->timer_d50 -= FRAMETIME;
        if ((object->apiobj.field_0x1f4 & 0x40000) != 0) {
            MovePlayer_NETWORK(object);
        } else if ((object->field_0xf00 & GAME_OBJECT_AI_UPDATE_PROCESS) != 0) {
            f32 frame_time = FRAMETIME;
            FRAMETIME = object->ai_elapsed_time;
            object->field_0xe20 &= ~0x20;
            if (object->spawn_protection_timer > 0.0f)
                object->spawn_protection_timer -= FRAMETIME;
            object->field_0xdec = 0.0f;
            if (TimingBarSet == 4)
                TBOPENFN("Move", 4);
            if (static_cast<i32>(object->apiobj.field_0x1f4) < 0) {
                object->apiobj.field_0x276 = object->field_0x106e;
                object->apiobj.movement_facing_angle = object->field_0x106e;
                object->apiobj.position.x = object->field_0xee8;
                object->apiobj.facing_angle = object->field_0x106e;
                object->apiobj.position.z = object->field_0xeec;
            }
            if (object->move_override != NULL)
                object->move_override(object);
            else if (object->movement_spline != NULL)
                MovePlayerSpline(object);
            else
                MovePlayer(object);
            if (TimingBarSet == 4)
                TBCLOSEFN("Move", 4);
            if ((object->field_0xefa & 0x40) != 0 && (object->apiobj.flags_low & 4) == 0) {
                object->room_id = -1;
                if ((object->field_0xefa & 0x80) != 0 && world->current_gscn != NULL)
                    object->room_id = NuPortalWhichRoom(world->current_gscn, &object->apiobj.collision_position);
            }
            FRAMETIME = frame_time;
        }
        GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
        if (data->field275_0x116 == 10 &&
            ((object->apiobj.model_draw_result != 0) != ((object->field_0xf01 & 1) != 0))) {
            f32 *wait = object->apiobj.model_draw_result != 0 ? &tieonsfxwait : &tieoffsfxwait;
            if (*wait <= 0.0f) {
                PlaySfx("veh_tie_by", &object->apiobj.collision_position);
                *wait = (static_cast<f32>(qrand()) * (1.0f / 65535.0f)) * 0.5f + 0.5f;
            }
        }
    }

    // Player movement has its own timers, power-up and controller processing.
    for (i32 i = 0; i < 8; ++i) {
        GameObject_s *object = Player[i];
        const u16 player_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_PLAYER_CHARACTER;
        if (object == NULL || (object->apiobj.field_0x1f8 & player_flags) != player_flags) {
            continue;
        }
        if (VehicleArea != 0 && static_cast<u8>(object->apiobj.field_0x27c) <= 1 &&
            (object->apiobj.flags_low & 0x80) == 0)
            object->field_0xe20 |= 0x20;
        else
            object->field_0xe20 &= ~0x20;
        if (object->timer_d5c > 0.0f)
            object->timer_d5c -= FRAMETIME;
        if (object->tag_cooldown > 0.0f)
            object->tag_cooldown -= FRAMETIME;
        if (object->spawn_protection_timer > 0.0f)
            object->spawn_protection_timer -= FRAMETIME;
        PowerUp_Update(object);
        if ((object->apiobj.flags_low & 0x80) != 0 && (Cheats_CheckFlags(0x1000) != 0 || object->field_0xdec > 0.0f))
            RegenerateHearts(object);
        object->field_0xf03 &= ~0x10;
        f32 flicker_decay = 1.0f;
        if (object->field_0x1024 > 0.0f && (object->apiobj.flags_low & 0x80) != 0) {
            if (adtab[adaptivedifficulty[0]][2] == 1)
                flicker_decay = 2.0f;
            else if (adtab[adaptivedifficulty[0]][2] == -1)
                flicker_decay = 0.5f;
        }
        object->field_0x1024 -= flicker_decay * FRAMETIME;
        if (object->field_0x1024 < -1.0f)
            SetFlicker(object, 0.0f);
        if (object->timer_d28 > 0.0f)
            object->timer_d28 -= FRAMETIME;
        if (object->timer_d50 > 0.0f)
            object->timer_d50 -= FRAMETIME;
        object->apiobj.flags_high = (object->apiobj.flags_high & ~0x40) | ((object->field_0x7a5 == 0) << 6);
        if ((object->apiobj.field_0x1f4 & 0x40000) != 0) {
            MovePlayer_NETWORK(object);
        } else {
            if ((object->apiobj.flags_low & 0x80) != 0) {
                if (object->pad_gamepad->pad != NULL) {
                    UpdateRumble(&object->pad_gamepad->rumble_packet);
                    Player_ToggleCharacter(object, 0, 1);
                }
            } else if (FreePlay != 0) {
                Player_ToggleCharacter(object, 0, 0);
            }
            if (TimingBarSet == 3)
                TBOPENFN("Move", 3);
            if (object->move_override != NULL)
                object->move_override(object);
            else if (object->movement_spline != NULL)
                MovePlayerSpline(object);
            else
                MovePlayer(object);
            if (TimingBarSet == 3)
                TBCLOSEFN("Move", 3);
        }
        if ((object->field_0xefa & 0x40) != 0 && (object->apiobj.flags_low & 4) == 0) {
            object->room_id = -1;
            if ((object->field_0xefa & 0x80) != 0 && world->current_gscn != NULL)
                object->room_id = NuPortalWhichRoom(world->current_gscn, &object->apiobj.collision_position);
        }
    }
    CurrentSpeedOverride = -1000.0f;
    AveragePlayerCurrentSpeedMul();

    // AI-controlled characters use their accumulated AI interval for a full
    // terrain update when their script was processed. Between those updates,
    // the original advances the last resolved velocity and still animates the
    // object every frame.
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *object = &Obj[i];
        const u16 character_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER;
        if ((object->apiobj.field_0x1f8 & character_flags) != character_flags ||
            (object->apiobj.field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) == 0) {
            object->field_0xef8 &= ~1;
            if (object->timer_ed8 > 0.0f)
                object->timer_ed8 -= FRAMETIME;
            continue;
        }
        if (TimingBarSet == 2)
            TBOPENFN("TA", 2);
        if ((object->field_0xf00 & GAME_OBJECT_AI_UPDATE_PROCESS) != 0) {
            const f32 frame_time = FRAMETIME;
            FRAMETIME = object->ai_elapsed_time;
            TerrainPlayer(object);

            object->field_0x10c8 = object->apiobj.position.x;
            object->field_0x10cc = object->apiobj.position.y;
            object->field_0x10d0 = object->apiobj.position.z;

            const f32 vertical_displacement = object->apiobj.position.y - object->apiobj.start_position.y;
            if (vertical_displacement == 0.0f || object->ai_elapsed_time == 0.0f) {
                object->vertical_velocity = 0.0f;
            } else {
                object->vertical_velocity = vertical_displacement / object->ai_elapsed_time;
            }
            FRAMETIME = frame_time;
        } else {
            PreResetCode(object);
            PostResetCode(object);
            if ((object->apiobj.field_0x1f4 & APIOBJECT_STATE_FLAG_IGNORE_DOORS) != 0) {
                GameObjectOrigin(object);
            } else {
                object->apiobj.position.x += object->apiobj.velocity.x * FRAMETIME;
                object->apiobj.position.y += object->vertical_velocity * FRAMETIME;
                object->apiobj.position.z += object->apiobj.velocity.z * FRAMETIME;
            }
        }

        AnimatePlayer(object);
        object->context_target_position = NULL;
        if (TimingBarSet == 2)
            TBCLOSEFN("TA", 2);
        if (world->sock_sys != NULL && (object->field_0xef9 & 0x40) != 0) {
            ComplexSockPosition(world->sock_sys, &object->apiobj.position, static_cast<i8>(object->field_0x661),
                                object->sock_segment, &object->sock_position);
            ComplexSockAngles(&object->sock_angles);
        } else {
            object->sock_segment = -1;
            object->field_0x661 = 0xff;
        }
        if (TimingBarSet == 4)
            TBOPENFN("Misc", 4);
        if (WORLD->current_level != SPEEDERCHASEA_LDATA || disable_narrow_socks != 0 || object->id != id_SPEEDERBIKE ||
            object->apiobj.field_0x287 == 0)
            UpdateLastSafePosition(object);
        f32 scale = 1.0f;
        if (object->apiobj.field_0x287 == 1)
            scale -= object->timer_1014 / object->field_0x1018;
        object->field_0x1004 = scale;
        if (object->field_0x1038 != 1000000000.0f)
            object->field_0x1004 = object->field_0x1038;
        object->apiobj.field_0xa8 = object->field_0x1004 * object->apiobj.character_data->model_scale;
        ScaleGameObject(object);
        GameObjectDimensions(object);
        AddSurfaceRipples(object);
        if (object->use_model_origin != 0xff) {
            ++object->use_model_origin;
        }
        NuCameraTransformScreenClip(&object->camera_screen_position, &object->apiobj.collision_position, 1, NULL);
        GameObjectToCameraCode(object);
        if (TimingBarSet == 4)
            TBCLOSEFN("Misc", 4);
        object->field_0xef8 &= ~1;
        if (object->timer_ed8 > 0.0f)
            object->timer_ed8 -= FRAMETIME;
    }

    CurrentSpeed = 0.0f;
    for (i32 i = 0; i < 8; ++i) {
        GameObject_s *object = Player[i];
        const u16 player_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_PLAYER_CHARACTER;
        if (object == NULL || (object->apiobj.field_0x1f8 & player_flags) != player_flags) {
            continue;
        }

        if (VehicleArea != 0 || (object->apiobj.character_data->model_flags & 0x2000) != 0) {
            object->reset_velocity = object->apiobj.velocity;
            object->pre_terrain_speed = NuFsqrt(object->reset_velocity.x * object->reset_velocity.x +
                                                object->reset_velocity.y * object->reset_velocity.y +
                                                object->reset_velocity.z * object->reset_velocity.z);
        }
        TerrainPlayer(object);
        object->field_0x10c8 = object->apiobj.position.x;
        object->field_0x10cc = object->apiobj.position.y;
        object->field_0x10d0 = object->apiobj.position.z;
        const f32 vertical_displacement = object->apiobj.position.y - object->apiobj.start_position.y;
        object->vertical_velocity = vertical_displacement == 0.0f || object->ai_elapsed_time == 0.0f
                                        ? 0.0f
                                        : vertical_displacement / object->ai_elapsed_time;
        AnimatePlayer(object);
        object->context_target_position = NULL;
        if (VehicleArea != 0 || (object->apiobj.character_data->model_flags & 0x2000) != 0) {
            object->post_terrain_speed = NuFsqrt(object->apiobj.velocity.x * object->apiobj.velocity.x +
                                                 object->apiobj.velocity.y * object->apiobj.velocity.y +
                                                 object->apiobj.velocity.z * object->apiobj.velocity.z);
        }
        if (object->oldpos != NULL)
            *object->oldpos = object->sock_position;
        if (world->sock_sys != NULL &&
            ((object->apiobj.flags_low & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0 || VehicleArea != 0)) {
            ComplexSockPosition(world->sock_sys, &object->apiobj.position, static_cast<i8>(object->field_0x661),
                                object->sock_segment, &object->sock_position);
            ComplexSockAngles(&object->sock_angles);
        } else {
            object->sock_segment = -1;
            object->field_0x661 = 0xff;
            if (object->oldpos != NULL) {
                object->oldpos->location.segment = -1;
                object->oldpos->location.sock = -1;
            }
        }
        if (object == Player[0]) {
            if (object->field_0x661 != 0xff)
                CurrentSpeed = world->sock_sys->sock[static_cast<i8>(object->field_0x661)].current_speed;
            BaseCurrentSpeed = CurrentSpeed;
            if (CurrentSpeedOverride != -1000.0f)
                CurrentSpeed = CurrentSpeedOverride;
        }
        UpdateLastSafePosition(object);
        plr_lastpos = object->field_0x661 == 0xff ? object->apiobj.position : object->apiobj.start_position;
        if (object->field_0x661 != 0xff && object->oldpos != NULL && object->oldpos->location.sock == -1) {
            *object->oldpos = object->sock_position;
        }
        KeepOnScreen(object);
        Doors_Check(world, object);
        f32 scale = 1.0f;
        if (object->apiobj.field_0x287 == 1)
            scale -= object->timer_1014 / object->field_0x1018;
        object->field_0x1004 = scale;
        if (FreePlay != 0 && WORLD->area != NULL && WORLD->area == PODSPRINT_ADATA && object->id != id_ANAKINSNEWPOD &&
            object->id != id_ANAKINSNEWPODGREEN && object->id != id_SEBULBASPOD) {
            object->field_0x1004 = scale * 0.55f;
        }
        if (object->field_0x1038 != 1000000000.0f)
            object->field_0x1004 = object->field_0x1038;
        object->apiobj.field_0xa8 = object->field_0x1004 * object->apiobj.character_data->model_scale;
        ScaleGameObject(object);
        GameObjectDimensions(object);
        if ((object->apiobj.flags_low & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0 && FadeSys.fade == 0.0f &&
            InDoubleScoreZone(object) != 0) {
            DoubleScore |= static_cast<i32>(1u << (static_cast<u8>(object->apiobj.field_0x27c) & 31));
        }
        UpdateCoinPacket(object->coinpacket, static_cast<u8>(object->apiobj.flags_low) >> 7,
                         static_cast<i8>(object->apiobj.field_0x27c));
        if ((object->apiobj.flags_low & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0 && object->apiobj.field_0x287 == 0 &&
            object->field_0x7a5 != 0x2b && !(object->field_0x7a5 == 0x0f && object->field_0x7a3 == 1)) {
            GizmoPickups_Collide(world, object, 1);
        } else {
            ResetCoinPacket(object->coinpacket);
        }
        AddSurfaceRipples(object);
        if ((object->apiobj.character_data->model_flags & 0x2000) != 0 && object->apiobj.field_0x287 == 0 &&
            object->apiobj.model_draw_result != 0 && (object->field_0xe20 & 0x20) == 0) {
            AddSurfaceDebris(object);
        }
        if (object->use_model_origin != 0xff) {
            ++object->use_model_origin;
        }
        NuCameraTransformScreenClip(&object->camera_screen_position, &object->apiobj.collision_position, 1, NULL);
        GameObjectToCameraCode(object);
    }

    i32 lighting_start = 0;
    const i32 lighting_phase = static_cast<i8>(MainFrameCounters.third_frame);
    if (lighting_phase != -1)
        lighting_start = (lighting_phase * HIGHGAMEOBJECT) / 3;
    GameObject_s *dagobah_luke = FindGameObject(id_LUKESKYWALKERDAGOBAH, 0, 1, 1, 0);
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
        GameObject_s *object = &Obj[i];
        const u16 character_flags = APIOBJECT_FLAG_IN_USE | APIOBJECT_FLAG_CHARACTER;
        if (world->rooms_visible_ptr[object->room_id] != 0 &&
            (object->apiobj.field_0x1f8 & character_flags) == character_flags) {
            NUVEC offset;
            i32 yoda_proximity = TakeOverYodaSeekDistanceHack(object, dagobah_luke, &offset);
            f32 target = (object->field_0xe23 & 0x80) != 0 || yoda_proximity != 0 ? 1.0f : 0.0f;
            object->interaction_arrow_blend =
                SeekLinearF(object->interaction_arrow_blend, target, FRAMETIME + FRAMETIME);
            if (object->apiobj.model_draw_result == 0)
                continue;
            if (i >= lighting_start)
                LightGameObject(object, world->rtl_set);
            if (object->interaction_arrow_blend > 0.0f) {
                GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
                i32 locator = data->ride_locator;
                NUVEC position;
                if (locator != -1 && object->apiobj.character_model->points_of_interest[locator] != NULL) {
                    position.x = object->joint_matrices[locator].m30;
                    position.y = object->joint_matrices[locator].m31;
                    position.z = object->joint_matrices[locator].m32;
                } else {
                    position.x = object->apiobj.field_0x190;
                    position.y = object->apiobj.field_0x194;
                    position.z = object->apiobj.field_0x198;
                }
                f32 height = object->apiobj.field_0x1e0 * 0.5f;
                position.y += 0.21f > height ? 0.21f : height;
                i32 phase = static_cast<u16>(object->apiobj.field_0x289 << 13);
                f32 amplitude = 0.05f * object->apiobj.field_0x1dc;
                i32 angle = static_cast<u16>(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 1.5f) / 1.5f * 65536.0f));
                position.x += amplitude * NuTrigTable[((angle + phase) >> 1) & 0x7fff];
                angle = static_cast<u16>(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 1.174f) / 1.174f * 65536.0f));
                position.y += (0.75f * amplitude) * NuTrigTable[((angle + phase) >> 1) & 0x7fff];
                angle = static_cast<u16>(static_cast<i32>(NuFmod(GameTimer.time_elapsed, 1.259f) / 1.259f * 65536.0f));
                position.z += amplitude * NuTrigTable[((angle + phase) >> 1) & 0x7fff];
                char text[64];
                NuStrCpy(text, ASCII_DOWN);
                ADDGAMEMSG message = AddGameMsg_Default;
                message.position = &position;
                message.text = text;
                message.scale = 1.0f;
                f32 pulse = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f);
                angle = static_cast<u16>(static_cast<i32>((pulse + pulse) * 65536.0f));
                message.alpha = static_cast<u8>(
                    static_cast<i32>((24.0f * NuTrigTable[angle >> 1] + 104.0f) * object->interaction_arrow_blend));
                message.flags = 0x1083;
                message.field_0x4f = 4;
                AddGameMsg(&message);
            }
        }
    }

    CollideGameObjects(world);
    if (do_player_tag != 0) {
        i32 result = 0;
        if (player_tag_to != NULL && player_tag_from != NULL && player_tag_to != player_tag_from &&
            (player_tag_to->apiobj.field_0x1f8 & 0x1001) == 0x1001 && player_tag_to->apiobj.field_0x287 == 0 &&
            (player_tag_to->tag_flags & 2) == 0 && (player_tag_from->apiobj.field_0x1f8 & 0x1001) == 0x1001 &&
            player_tag_from->apiobj.field_0x287 == 0 && (player_tag_from->tag_flags & 2) == 0) {
            result = TagCode(player_tag_to, player_tag_from, 0, 0, 1);
            if (result == 1) {
                GameAudio_PlaySfx(0x22, &player_tag_to->apiobj.collision_position, 0, 0);
                GameCam_Blend(GameCam, 0.5f, 0.0f, 1);
                RememberPlayerIDs(0, Player[0] != NULL ? Player[0]->id : -1, Player[1] != NULL ? Player[1]->id : -1);
                player_tag_to->hud_icon_timer = 2.0f;
                player_tag_from->hud_icon_timer = 2.0f;
            }
        }
        if (result != 2) {
            player_tag_to = NULL;
            player_tag_timer = 0.0f;
            player_tag_from = NULL;
            do_player_tag = 0;
        }
    } else if (player_tag_timer > 0.0f) {
        player_tag_timer -= FRAMETIME;
        if (player_tag_timer < 0.0f || player_tag_to == NULL || player_tag_from == NULL ||
            (player_tag_to->apiobj.field_0x1f8 & 0x1001) != 0x1001 || player_tag_to->apiobj.field_0x287 != 0 ||
            (player_tag_to->tag_flags & 2) != 0 || (player_tag_from->apiobj.field_0x1f8 & 0x1001) != 0x1001 ||
            player_tag_from->apiobj.field_0x287 != 0 || (player_tag_from->tag_flags & 2) != 0) {
            player_tag_to = NULL;
            player_tag_timer = 0.0f;
            player_tag_from = NULL;
            do_player_tag = 0;
        }
    }
    i32 player_indicator[2] = {0, 0};
    if ((VehicleArea != 0 || (WORLD->current_level == SPEEDERCHASEA_LDATA && disable_narrow_socks == 0)) &&
        Player[0] != NULL && (Player[0]->apiobj.flags_low & 0x80) != 0 && Player[1] != NULL &&
        (Player[1]->apiobj.flags_low & 0x80) != 0) {
        player_indicator[0] = 1;
        player_indicator[1] = 1;
    } else {
        for (i32 i = 0; i < 2; ++i) {
            GameObject_s *object = Player[i];
            if (object != NULL && (object->apiobj.flags_low & 0x80) != 0 && (object->field_0xe24 & 8) != 0 &&
                object->field_0xcc0 != NULL &&
                (object->id == id_ATST || object->id == id_ATST_LOWRES || object->id == id_ATAT)) {
                player_indicator[i] = 2;
            }
        }
    }
    for (i32 i = 0; i < 2; ++i) {
        GameObject_s *object = Player[i];
        if (object == NULL || player_indicator[i] == 0 || object->apiobj.field_0x287 != 0 ||
            (object->hud_icon_timer > 0.0f && VehicleArea == 0))
            continue;
        NUVEC position;
        if (player_indicator[i] == 2) {
            GAMECHARACTERDATA *data = static_cast<GAMECHARACTERDATA *>(object->apiobj.character_data->field11_0x24);
            i32 locator = data->ride_locator;
            if (locator != -1 && object->apiobj.character_model->points_of_interest[locator] != NULL) {
                position.x = object->joint_matrices[locator].m30;
                position.y = object->joint_matrices[locator].m31;
                position.z = object->joint_matrices[locator].m32;
            } else {
                position = object->apiobj.upper_position;
            }
            position.y += 0.5f * object->apiobj.field_0x1e0;
        } else if (world->current_level == DOGFIGHTA_LDATA) {
            position.x = 0.0f;
            position.y = object->field_0x1000 * object->apiobj.field_0xa8;
            position.z = 0.0f;
            NuVecMtxRotate(&position, &position, &object->vehicle_orientation);
            NuVecAdd(&position, &position, &object->apiobj.position);
        } else {
            position = object->apiobj.collision_position;
            position.y += 1.5f * object->apiobj.field_0x1e0;
        }
        ADDGAMEMSG message = AddGameMsg_Default;
        message.text = ASCII_DOWN;
        message.position = &position;
        message.scale = 0.666f;
        message.red = PlayerRGB[i][0];
        message.green = PlayerRGB[i][1];
        message.blue = PlayerRGB[i][2];
        f32 pulse = NuFmod(GameTimer.time_elapsed_mod_seconds, 0.5f);
        u16 angle = static_cast<u16>(static_cast<i32>((pulse + pulse) * 65536.0f));
        message.alpha = static_cast<u8>(static_cast<i32>(48.0f * NuTrigTable[angle >> 1] + 80.0f));
        message.flags = 0x83;
        message.field_0x4f = 4;
        AddGameMsg(&message);
    }
    if (MissionSys != NULL && Mission_Active(MissionSys) != NULL && MissionSys->mission != NULL) {
        CheckMissionEnd(MissionSys);
    }
}

GameObject_s *AddDynamicCreature(i32 model, nuvec_s *position, i32 angle, char *script_name, AIPATHINFO_s *path_info,
                                 AIGROUP_s *group, i32 set_on_surface, nugspline_s *spline, nuvec_s *spline_offset,
                                 i32 spline_mode, i32 creature_set) {
    const bool has_no_spline = spline == NULL;
    if (position == NULL && spline == NULL) {
        return NULL;
    }

    if (NOAICREATURES != 0 && model != id_DRAGBOMB && (GCDataList[model].flags_090 & 0x40) == 0) {
        return NULL;
    }

    u32 arcade_mode;
    Arcade_GetMode(&arcade_mode);
    if ((arcade_mode & 0x10) != 0) {
        return NULL;
    }

    if (static_cast<u32>(model) >= 0x154 || apicharsys->playermodelids[model] == -1) {
        return NULL;
    }

    GameObject_s *object = AddCreature(model, 0);
    if (object == NULL) {
        return NULL;
    }

    object->apiobj.field_0x1f4 |= APIOBJECT_MOTION_FLAG_AI_CONTROLLED;
    const u32 model_flags = apicharsys->char_data[model].model_flags;
    if ((model_flags & 0x200) != 0) {
        object->apiobj.field_0x1f4 |= 0x404;
    } else if ((model_flags & 0x400) != 0) {
        object->apiobj.field_0x1f4 |= 0x401;
    }
    object->field_0x1050 |= (model_flags & 0x1000) != 0 ? 5 : 1;

    GAMECHARACTERDATA &game_character = *(GAMECHARACTERDATA *)apicharsys->char_data[model].field11_0x24;
    object->apiobj.viewdistance = game_character.viewdistance;
    object->apiobj.heardistance = game_character.heardistance;
    object->apiobj.maxviewheight = game_character.maxviewheight;
    object->apiobj.minviewheight = game_character.minviewheight;
    object->field_0xef9 &= static_cast<u8>(~8u);
    object->ai.field_0xe0 = 0x4e6e6b28;
    object->ai.field_0xf0 = 0x4e6e6b28;
    object->field_0xef8 &= static_cast<u8>(~1u);
    object->ai.field_0x1e5 &= static_cast<u8>(~0x50u);
    object->apiobj.field387_0x2a0 = 0;
    object->apiobj.field388_0x2a4 = 0;
    object->field_0xebc = 0;
    object->field_0xec0 = 0;
    object->field_0xecc = 0;
    object->field_0xed0 = 0;
    object->field_0xed8 = 0;
    object->ai.nearest_opponent = NULL;
    object->ai.field_0xdc = 0;
    object->ai.field_0xec = 0;
    object->ai.opponent = NULL;
    object->ai.antinode_timer = 0.0f;
    InitPlayerAI(object);

    if (spline == NULL) {
        object->apiobj.position = *position;
        object->apiobj.facing_angle = static_cast<u16>(angle);
        object->apiobj.movement_facing_angle = static_cast<u16>(angle);
        object->apiobj.field_0x276 = static_cast<u16>(angle);
        if (group != NULL) {
            AddToAIGroup(group, &object->apiobj);
            NUVEC offset;
            offset.x = ((object->ai.group_column + 1) / 2) * group->x_spacing;
            if (object->ai.group_column & 1)
                offset.x = -offset.x;
            offset.y = 0.0f;
            offset.z = -group->z_spacing * object->ai.group_member;
            NuVecRotateY(&offset, &offset, angle);
            NuVecAdd(&object->apiobj.position, &object->apiobj.position, &offset);
        }
    } else {
        SPLINEPOS_s *spline_position = reinterpret_cast<SPLINEPOS_s *>(&object->movement_spline);
        InitSplinePosition(spline_position, spline, 0.0f, spline_mode);
        SPLINEPOSITION_RUNTIME_s *runtime = reinterpret_cast<SPLINEPOSITION_RUNTIME_s *>(spline_position);
        NUVEC spline_point;
        u16 yaw = 0;
        u16 pitch = 0;
        PointAlongSpline(runtime->spline, runtime->normalized_position, &spline_point, &yaw, &pitch, runtime->looping);
        object->apiobj.facing_angle = yaw;
        object->apiobj.movement_facing_angle = yaw;
        object->apiobj.field_0x276 = yaw;
        object->apiobj.pitch_angle = static_cast<u16>(-pitch);

        if (spline_offset != NULL) {
            NUVEC *stored_offset = &object->movement_spline_offset;
            *stored_offset = *spline_offset;
            if (spline_offset->x != 0.0f || spline_offset->y != 0.0f || spline_offset->z != 0.0f) {
                NUVEC rotated;
                NuVecRotateX(&rotated, spline_offset, static_cast<u16>(-pitch));
                NuVecRotateY(&rotated, &rotated, yaw);
                NuVecAdd(&spline_point, &spline_point, &rotated);
            }
        }
        object->apiobj.position = spline_point;
    }

    ResetPlayerMoves(object);
    object->apiobj.pos_x = object->apiobj.position.x;
    object->apiobj.pos_y = object->apiobj.position.y;
    object->apiobj.pos_z = object->apiobj.position.z;
    object->apiobj.start_position = object->apiobj.position;
    object->apiobj.initial_position = object->apiobj.position;
    plr_lastpos = object->apiobj.position;
    object->apiobj.velocity = v000;

    GetTopBot(object);
    GameObjectDimensions(object);
    ResetRumble(&object->pad_gamepad->rumble_packet);
    ResetLights(&object->apiobj.position, &object->light_data, WORLD->rtl_set);

    object->field_0x661 = 0xff;
    object->sock_segment = -1;
    if (WORLD->sock_sys != NULL) {
        extern i32 complexsockposition_forcesock;
        if (Door_NextSock != -1 && WORLD->sock_sys->sock[Door_NextSock].valid != 0)
            complexsockposition_forcesock = Door_NextSock;
        ComplexSockPosition(WORLD->sock_sys, &object->apiobj.position, -1, -1, &object->sock_position);
        ComplexSockAngles(&object->sock_angles);
    }
    NuVecRotateYValZ(&object->facing_direction, 1.0f, object->apiobj.field_0x276);
    object->apiobj.model_draw_result = 1;
    object->use_model_origin = 0;
    object->apiobj.field_0x288 = 0;
    object->field_0xefe &= ~4u;

    if (has_no_spline) {
        InitSurfaceInfo(object);
        if (set_on_surface != 0) {
            SetObjOnSurface(object, 0);
        }
    }
    PortalGameObject(object, 1, 1, -1, WORLD->current_gscn);
    if (reset_restart != 0) {
        GAMECHARACTERDATA *config = (GAMECHARACTERDATA *)object->apiobj.character_data->field11_0x24;
        object->hitpoints = config->hitpoints;
        object->current_hp = config->hitpoints;
        object->field_0xe37 = config->field_0xf5;
        object->field_0xe38 = 4;
    }

    object->apiobj.field_0x1f4 &= ~0x100u;
    object->apiobj.field_0x1f8 &= static_cast<u16>(~4u);
    object->field_0x1004 = 1.0f;
    object->apiobj.field_0x287 = 0;
    object->field_0x7a5 = 0xff;
    object->apiobj.field_0x285 = 0;
    memset(&object->ai.path_info, 0, sizeof(object->ai.path_info));
    object->ai.field_0x124 = -1;
    object->ai.field_0x138 = 0xff;
    object->ai.field_0x139 = 0;
    if (has_no_spline) {
        if (path_info != NULL) {
            AISysCharacterSetPath(&object->ai, path_info->path);
            AISysCharacterSetPathCnx(&object->ai, &object->apiobj.position, path_info->connection,
                                     path_info->direction);
        }
        if (object->ai.path_info.connection == NULL) {
            AISysGetCharacterPathPos(WORLD->ai_sys, &object->apiobj, &object->ai, 0xff,
                                     static_cast<i8>(object->apiobj.field_0x27d));
        }
    }
    AIScriptProcessorInit(WORLD->ai_sys, &object->ai, &object->ai.script_process, NULL, script_name, NULL, 1, NULL,
                          NULL);
    object->apiobj.field_0x214 = 2000000.0f;
    if (object->ai.group != NULL) {
        AIGROUP *active_group = object->ai.group;
        u32 member = object->ai.group_member_index;
        active_group->member_is_alive |= (member & 32) ? 0 : (1u << (member & 31));
        AIROW *row = &active_group->rows[object->ai.group_row];
        u32 column = object->ai.group_member_index - object->ai.group_row * active_group->count_across;
        row->is_alive |= (column & 32) ? 0 : (1u << (column & 31));
        if (object->ai.group_column == 0) {
            row->pos = object->apiobj.position;
            row->path_info = object->ai.path_info;
            row->y_rot = object->apiobj.field_0x276;
            row->next_connection = NULL;
            row->is_turning = 0;
        }
    }
    PreResetCode(object);
    PostResetCode(object);
    GameObjectOrigin(object);
    object->apiobj.previous_position[0] = object->apiobj.position.x;
    object->apiobj.previous_position[1] = object->apiobj.position.y;
    object->apiobj.previous_position[2] = object->apiobj.position.z;
    object->field_0x10c8 = object->apiobj.position.x;
    object->field_0x10cc = object->apiobj.position.y;
    object->field_0x10d0 = object->apiobj.position.z;
    if (static_cast<u32>(creature_set - 1) < 16) {
        object->ai.creature_set = static_cast<u8>(creature_set);
        ++aicreature_sets_alive[creature_set - 1];
    }
    return object;
}

GameObject_s *GetNamedGameObject(AISYS_s *system, char *name) {
    if (GetNamedAPIObjectFn != NULL) {
        APIOBJECT *object = GetNamedAPIObjectFn(system, name);
        if (object != NULL) {
            return object->objptr;
        }
    }
    return NULL;
}

void TakeOverGameObject2(GameObject_s *, GameObject_s *, i32);

void TakeOverGameObject(GameObject_s *rider, GameObject_s *vehicle, i32 blend_camera, i32 immediate) {
    if (immediate != 0)
        TakeOverGameObject2(rider, vehicle, blend_camera);
    else
        TakeOver2GetIn(rider, vehicle);
}

void RegisterTakeOverObject(GameObject_s *object);

void TakeOverGameObject2(GameObject_s *rider, GameObject_s *vehicle, i32 blend_camera) {
    if (rider == NULL || (rider->apiobj.field_0x1f8 & 1) == 0 || vehicle == NULL ||
        (vehicle->apiobj.field_0x1f8 & 1) == 0 || rider->field_0xcc0 != NULL || vehicle->field_0xcc0 != NULL) {
        return;
    }
    i32 result;
    if ((rider->field_0xf00 & 2) != 0) {
        result = TakeOverYoda(rider, vehicle, blend_camera, 1);
    } else {
        if (WORLD->current_level == SPEEDERCHASEA_LDATA && disable_narrow_socks == 0 &&
            (rider->apiobj.field_0x1f8 & 0x80) != 0) {
            blend_camera = 1;
        }
        RegisterTakeOverObject(vehicle);
        result = TagCode(rider, vehicle, 1, blend_camera, 1);
    }
    if (result == 2) {
        rider->pending_tag_target = vehicle;
        rider->tag_cooldown = 1.0f;
        rider->tag_flags |= 0x0c;
    }
}

void DeactivateGameObject(GameObject_s *object) {
    if (object == NULL) {
        return;
    }

    if (object->field_0xcc0 != NULL) {
        if ((object->apiobj.field_0x1f4 & 0x4000) != 0) {
            ReleaseTakeOver(object, 1);
        } else {
            KillGameObject(object->field_0xcc0, 4, 0);
            object->apiobj.character = 0;
        }
    }
    object->apiobj.character = 0;

    if (object->ai.field_0x134 != 0xff &&
        AIScriptSetBaseScriptStateByName(reinterpret_cast<AISCRIPTPROCESS *>(&object->ai),
                                         const_cast<char *>("InActive")) != 0) {
        object->ai.reset_mode = 0;
        WORLD->ai_sys->creatures[object->ai.field_0x134].activate_type = 2;
    } else {
        object->ai.reset_mode = 4;
    }
}

i32 EquivalentObject_Find(WORLDINFO_s *, nuhspecial_s *) {
    return 0;
}

GameObject_s *FindNearestGameObject(NUVEC *position, GameObject_s *exclude, u32 required_flags, f32 radius,
                                    f32 extra_radius, i32 animation, i32 character_id, i32 player_index,
                                    f32 *distance_squared, i32 horizontal_only, i32 (*filter)(GameObject_s *),
                                    bool first_match) {
    GameObject_s *nearest = NULL;
    f32 nearest_distance = 100000000.0f;
    f32 radius_squared = radius * radius;
    GameObject_s *object = Obj;
    for (i32 i = 0; i < HIGHGAMEOBJECT; ++i, ++object) {
        if ((object->apiobj.field_0x1f8 & 0x1001) != 0x1001 || object->apiobj.field_0x287 != 0 ||
            (object->field_0xe20 & 0x20) || (CInfo[object->character_context].flags & 0x8000))
            continue;
        if (character_id != -1 && object->id != character_id)
            continue;
        if (player_index >= 0) {
            if (player_index <= 1) {
                if (object->apiobj.field_0x27c != player_index)
                    continue;
            } else if (player_index == 99) {
                if (object->apiobj.field_0x27c == -1)
                    continue;
            } else if (player_index == 100) {
                if (object->apiobj.field_0x27c != -1)
                    continue;
            }
        }
        if (exclude && object == exclude)
            continue;
        if (required_flags && (object->apiobj.character_data->model_flags & required_flags) != required_flags)
            continue;
        if (animation != -1 && object->apiobj.character_model->model_data_b[animation] == NULL)
            continue;
        if (filter && !filter(object))
            continue;
        f32 x = object->apiobj.collision_position.x - position->x;
        f32 y = object->apiobj.collision_position.y - position->y;
        f32 z = object->apiobj.collision_position.z - position->z;
        f32 distance = horizontal_only ? x * x + z * z : x * x + y * y + z * z;
        if (exclude) {
            if (radius_squared <= 0.0f || distance < radius_squared) {
                f32 combined_radius = exclude->apiobj.collision_radius + object->apiobj.collision_radius + extra_radius;
                if (distance < combined_radius * combined_radius && distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest = object;
                    if (first_match)
                        break;
                }
            }
        } else if ((radius_squared <= 0.0f || distance < radius_squared) && distance < nearest_distance) {
            nearest_distance = distance;
            nearest = object;
            if (first_match)
                break;
        }
    }
    if (nearest && distance_squared)
        *distance_squared = nearest_distance;
    return nearest;
}

extern "C" {
    extern debris_chunk_control_s *debris_chunk_control_stack[2];
    extern debris_chunk_control_s **freechunkcontrols;
    extern i32 freechunkcontrolsptr;
}
void RemoveChunkControlFromStack(debris_chunk_control_s *, debris_chunk_control_s **);
void RemoveAnyChunkControls(i32 *chunk) {
    for (i32 i = 0; i < 2; ++i) {
        for (debris_chunk_control_s *control = debris_chunk_control_stack[i]; control != NULL;) {
            debris_chunk_control_s *next = control->next;
            if (control->particle_chunk == reinterpret_cast<dma_particle_chunk_s *>(chunk)) {
                RemoveChunkControlFromStack(control, &debris_chunk_control_stack[i]);
                freechunkcontrols[--freechunkcontrolsptr] = control;
                control->particle_chunk = NULL;
            }
            control = next;
        }
    }
}

void RemoveChunkFromRenderStack(particlechunkrendertype_s *chunk, particlechunkrendertype_s **stack) {
    if (*stack == chunk) {
        particlechunkrendertype_s *next = chunk->next;
        *stack = next;
        if (next != NULL) {
            next->previous = NULL;
        }
    } else if (chunk->previous != NULL) {
        chunk->previous->next = chunk->next;
        if (chunk->next != NULL) {
            chunk->next->previous = chunk->previous;
        }
    }
    chunk->previous = NULL;
    chunk->next = NULL;
}

void RemoveChunkControlFromStack(debris_chunk_control_s *control, debris_chunk_control_s **stack) {
    debris_chunk_control_s **link = stack;
    while (*link != NULL) {
        if (*link == control) {
            *link = control->next;
            break;
        }
        link = &(*link)->next;
    }
    control->next = NULL;
}

extern "C" debkeydatatype_s *debris_keystack;

void RemoveDebrisEffectFromStack(debkeydatatype_s *key) {
    if (key->next == NULL) {
        debris_keystack = key->previous;
        if (debris_keystack != NULL) {
            debris_keystack->next = NULL;
        }
    } else {
        key->next->previous = key->previous;
        if (key->previous != NULL) {
            key->previous->next = key->next;
        }
    }
    key->next = NULL;
    key->previous = NULL;
}

extern "C" {

    i32 InModelList(APICHARACTERMODELLIST_s *list, i32 id, i32 *out_index) {
        if (list != NULL) {
            i32 i = 0;
            for (; list->model_id != -1; list++, i++) {
                if (list->model_id == id) {
                    if (out_index != NULL)
                        *out_index = i;
                    return 1;
                }
            }
        }
        if (out_index != NULL)
            *out_index = -1;
        return 0;
    }

} // extern "C"
