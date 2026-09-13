#ifndef LEGOAPI_TYPES_H
#define LEGOAPI_TYPES_H
#pragma once
#include "gameapi/ai/aisys/aimessage_types.h"
#include "gamelib/util/gamelib_util_types.h"

#include "nu2api/nu3d/ShaderManagerOpenGL.h"
#include "decomp.h"
#include "nu2api/nucore/fixed_width.h"
#include "nu2api/nucore/numemory.h"
#include "decomp.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/nucore/nuvuvec.hpp"
#include "nu2api/nu3d/nuhspecial.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuvec4.h"

#include "legoapi/items/base/apiobject.h"
#include "MechInputTouch/MechInputTouch_types.h"
#include "legoapi/characters/core/CharacterObjectInterface.h"
#include "legoapi/characters/motion/animlist.h"
#include "legoapi/items/base/animpacket.h"
#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "legoapi/gizmo/base/GizBuildItObjectInterface.h"
#include "legoapi/gizmo/base/GizForceObjectInterface.h"
#include "legoapi/gizmo/base/GizLeverObjectInterface.h"
#include "legoapi/gizmo/base/GizObstacleObjectInterface.h"
#include "legoapi/gizmo/base/GizPanelObjectInterface.h"
#include "legoapi/gizmo/base/GizTurretObjectInterface.h"
#include "legoapi/gizmo/base/HatMachineObjectInterface.h"
#include "legoapi/gizmo/base/TeleportObjectInterface.h"
#include "legoapi/render/core/SwipeDecalRenderer.h"

struct ADDGAMEMSG;
struct ADDPART_s;
struct AIANTINODE_s;
struct AIAREA_s;
struct AIGROUP_s;
struct AILOCATOR_s;
struct AIPACKET_s;
struct AIPATHCNXCONTROLLER_s;
struct AIPATHCNXCONTROLSYS_s;
struct AIPATHCNXHELPERSYS_s;
struct AIPATHCNXHELPER_s;
struct AIPATHCNX_s;
struct AIPATHINFO_s;
struct AIPATHNODE_s;
struct AIPATH_s;
struct AISCRIPTPROCESS_s;
struct AISCRIPT_s;
struct AISYS_s;
struct AITRIGGERSETSYS_s;
struct AITRIGGERSET_s;
struct ANIMLIST_s;
struct ANIMPACKET_s;
struct ANIMREDIRECT;
struct APICHARACTERMODELLIST_s;
struct APIOBJECT_s;
struct AREADATA_s;
struct AREASAVE_s;
struct ATTRACTO_s;
struct BATARANG_s;
struct BOLTSYS;
struct BOLT_s;
enum BUILDIT_FIND_ENUM : i32;
struct BaseEditor;
struct BaseThing;
struct CABLE_s;
struct CHARACTERDATA_s;
struct CHARACTERMODEL_s;
struct CHARCATEGORY;
struct CHARFIXUP;
struct CHARPIVOT;
struct CHARPLATFORMSYS_s;
struct CHARVARIANT;
struct CHEAT;
struct CLIMBOBJECTSYS_s;
struct COLLECTION_s;
struct CUSTOMISER;
struct CUSTOMISESAVE_s;
struct CUSTOMPIECE;
struct CUTINFO;
struct CUTSCENESYS;
struct CUTSYS;
struct CantPickupBombTimerAddon;
struct CharacterObjectInterface;
struct ClassEditor;
struct ClassItem;
struct ClassObject;
struct ClassObjectList;
struct ClickToPressStartGestureTracker;
struct CursorTool;
struct DATAPTR;
struct DETONATOR_s;
struct DOOR_s;
struct EDCREATURE_s;
struct EPISODEDATA;
struct EXPLOSION;
struct EXTRAMODEL;
struct EdClass;
struct EdControl;
struct EdFileInputStream;
struct EdFileOutputStream;
struct EdInputContext;
struct EdRef;
struct EdStream;
struct EdTool;
struct FADEINFO_s;
struct FADETYPE;
struct FADER_s;
struct FLOWBOX_s;
struct FS_FILEENTRYHDR;
struct Fade;
struct FadeBase;
struct FadeStill;
struct FadeStillWipe;
struct FadeSystem;
struct FadeWipe;
struct GAMEANIMOBJPOOL_s;
struct GAMEANIMOBJ_s;
struct GAMEANIMSET_s;
struct GAMEANIMSYS_s;
struct GAMEANTINODEDATA_s;
struct GAMEANTINODESYS_s;
struct GAMEANTINODE_s;
struct GAMEAUDIO;
struct GAMECAMERA_s;
struct GAMECHARACTERDATA_s;
struct GAMEMESSAGE_s;
struct GAMEPAD_s;
struct GIZACTIONDEFN_s;
struct GIZAIMESSAGESYS_s;
struct GIZAIMESSAGE_s;
struct AIMESSAGESYS_s;
struct AIMESSAGE_s;
struct GIZBOMBGENSYS_s;
struct GIZBUILDIT_s;
struct GIZFLOWPROGRESS_s;
struct GIZFLOW_s;
struct GIZFORCESYS_s;
struct GIZFORCE_s;
struct GIZFORCEGROUP_s;
struct GIZMOBLOWUPTYPE_s;
struct GIZMOBLOWUP_s;
struct GIZMOPICKUPRUNTIMESYS_s;
struct GIZMOPICKUPSYS_s;
struct GIZMOPICKUP_s;
struct GIZMOSYS_s;
struct GIZMO_s;
struct GIZOBSTACLESYS_s;
struct GIZOBSTACLE_s;
struct GIZPANEL_s;
struct GIZSPECIAL_s;
struct GIZSPECIALSYS_s;
struct GIZSPINNER_s;
struct GIZTURRETSYS_s;
struct GIZTURRET_s;
struct GRABBER_s;
struct GRAPPLE_s;
struct GameObject_s;
struct GameThingManager;
struct GizBlowupObjectInterface;
struct GizBuildItObjectInterface;
struct GizForceObjectInterface;
struct GizLeverObjectInterface;
struct GizObstacleObjectInterface;
struct GizPanelObjectInterface;
struct GizTurretObjectInterface;
struct HATMACHINE_s;
struct HINT_s;
struct HINTSYS_s;
struct HINTUIBUTTON_s;
struct HOTHBATTLE_MELEE_s;
struct HashRedirect;
struct HashedKey;
struct HatMachineObjectInterface;
struct HudRadarPulse;
struct LANGUAGEDATA;
struct LEVELDATADISPLAY;
struct LEVELDATA_s;
struct LEVELOBJECT;
struct LEVELSCRIPTPROCESS_s;
struct LEVELSPLINE;
struct LEVEL_PROGRESS_s;
struct LEVER_s;
struct LevelEditor;
struct LoadedUniqueShaderRecord;
struct MENU_s;
struct MISSIONSAVE_s;
struct MISSIONSYS_s;
struct MechAddon;
struct MechInputTouchMainController;
struct MechInputTouchVirtualConsoleController;
struct MechObjectInterface;
struct MechTouchUIElement;
struct MemoryBuffer;
struct MemoryManager;
struct MoveToMarker;
struct NARROWSOCKEXCEPTION;
struct NUGCUTCHAR_s;
struct NUGCUTSCENE_s;
struct NetAddress;
struct NetMessage;
struct NetPeer;
struct NuBloomParameters;
struct NuShaderObject;
struct NuShaderUsageMask_s;
struct NuVec2;
struct OPTIONSSAVE_s;
struct OccluderSet;
struct OcclusionManager;
struct PARTDEBSYS_s;
struct PART_s;
struct PLATSKININFO;
struct PLAYERITEMTYPE_s;
struct PLAYERITEM_s;
struct PLAYERPACKET_s;
struct PLUGSYS_s;
struct PLUG_s;
struct PULSE_s;
struct PULSESYS_s;
struct PartHeader;
struct PartObjectInterface;
struct Placeable;
struct PlaceableHelper;
struct PlaceableInterface;
struct PlaceableNameControl;
struct PropertyMenu;
struct PropertyMenuList;
struct PropertyTool;
struct REGISTERSTATUSPACKET_s;
struct RGBA;
struct RUMBLEPACKET;
struct SCENEPROGRESS_s;
enum SHADERSEMANTIC_enum : i32;
struct SHARD_s;
struct SOCKPOSITION_s;
struct SOCKSYS;
struct SPLINEPOS_s;
struct STATUSPACKET_s;
struct STATUSPACKET_LSW_s;
struct STATUS_STAGE_s;
struct SUIT_s;
struct SUPERCOUNTER;
struct SUPERCOUNTERPICKUP;
struct SceneInstance;
struct SceneObject;
struct SceneObjectHelper;
struct ShaderManagerOpenGL;
struct ShaderMtlDescFilter;
struct ShaderObjectKey;
struct SoundTable;
struct SpecialObject;
struct SwipeDecalRenderer;
struct TECHNO_s;
struct TELEPORT_s;
struct TERRPICKUPSET;
struct TERRSET;
struct TERRAIN_SURFACE_s;
struct TEXTCRAWL_s;
struct TEXTENTRY;
struct TMClient;
struct TORPEDOPACKET_s;
struct TRAFFICANIMSYS_s;
struct TTNetwork;
struct TUBE_s;
struct TeleportObjectInterface;
struct ThingLevelData;
struct ThingManager;
struct ThingProcessData;
struct ThingRemoveData;
struct ThingRenderData;
struct ThingResetData;
struct TouchHolder;
struct VirtualControlButton;
struct VirtualControlButtonMover;
struct VirtualControlDPad;
struct VirtualControlDPad_LockButton;
struct VirtualStackAllocator;
struct VuMtx;
struct VuVec;
struct WORLDINFO_s;
struct __sFILE;
struct _vum_s;
struct _vuv_s;
struct bgprocinfo_s;
struct bitrate_manager_state;
struct codebook;
struct debinftype;
struct debkeydatatype_s;
struct debris_chunk_control_s;
struct nunativedebrisdata_s;
struct drft_lookup;
struct edcam_s;
struct eduiitem_s;
struct eduimenu_s;
struct envelope_lookup;
struct flightspline_s;
struct instNUGCUTLOOKAT_s;
struct instNUGCUTSCENE_s;
struct mdct_lookup;
struct minisnowtrooper_s;
struct minitrooperteam_s;
struct nuanimbuff_s;
struct nuanimdata_s;
struct nucolour3_s;
struct nudisplaylistitem_s;
struct nudisplayscene_s;
struct nufile_device_s;
struct nufpar_s;
struct nufpcomjmp_s;
struct nuglobalrndrstate_s;
struct nugraph_s;
struct nugscn_s;
struct nugspline_s;
struct nuinstanim_s;
struct numtl_s;
struct numtx_s;
struct nunativedebrisdata_s;
struct nuoctreenode_s;
struct nupad_s;
struct nushadermtldesc_s;
struct nusound_filename_info_s;
struct nutex_s;
struct nutexmanager_s;
struct nuvec_s;
struct particlechunkrendertype_s;
struct pushblock_s;
struct ripple_node_s;
struct ripple_set_s;
struct rtldata_s;
struct rtlset;
struct shopitem_s;
struct specialsfx_s;
struct speedup_s;
struct starfighter_s;
struct terrsitu_s;
struct tertype;
struct uv1deb;
struct uv1debdata;
struct vucharidx_s;

struct ADDGAMEMSG {
    char *text;                         // 0x00
    nuvec_s *position;                  // 0x04
    nuvec_s *target_position;           // 0x08
    f32 scale;                          // 0x0c
    f32 target_scale;                   // 0x10
    u8 red;                             // 0x14
    u8 green;                           // 0x15
    u8 blue;                            // 0x16
    u8 alpha;                           // 0x17
    u32 flags;                          // 0x18
    f32 duration;                       // 0x1c
    f32 field_0x20;                     // 0x20
    u16 field_0x24;                     // 0x24
    i16 icon;                           // 0x26
    nuvec_s *extra_position;            // 0x28
    u32 score;                          // 0x2c
    f32 field_0x30;                     // 0x30
    f32 field_0x34;                     // 0x34
    f32 field_0x38;                     // 0x38
    f32 field_0x3c;                     // 0x3c
    void (*update_fn)(GAMEMESSAGE_s *); // 0x40
    void *field_0x44;                   // 0x44
    void (*end_fn)(GAMEMESSAGE_s *);    // 0x48
    i8 player_index;                    // 0x4c
    u8 field_0x4d;
    u8 field_0x4e;
    u8 field_0x4f;
};
DECOMP_ASSERT(sizeof(ADDGAMEMSG) == 0x50, "ADDGAMEMSG size");
typedef ADDGAMEMSG ADDGAMEMSG_ALIGNED16 __attribute__((aligned(16)));
struct PARTLIGHTSOURCE_s {
    u8 reserved_00[0x78];
    NUVEC values[7];
};

struct ADDPART_s {
    NUMTX *matrix;           // 0x00
    NUVEC *position;         // 0x04, used when no matrix is supplied
    NUVEC *velocity;         // 0x08
    GameObject_s *owner;     // 0x0c
    GameObject_s *recipient; // 0x10
    f32 field_14, field_18;
    f32 gravity; // 0x1c
    f32 field_20;
    nuhspecial_s *special; // 0x24
    i32 field_28;
    u32 flags; // 0x2c
    void (*field_30)(PART_s *);
    void (*move_fn)(PART_s *, f32); // 0x34
    void (*update_fn)(PART_s *);    // 0x38
    void (*field_3c)(PART_s *);
    void (*field_40)(PART_s *);
    void (*field_44)(PART_s *, i32);
    void (*field_48)(PART_s *);
    void (*stop_fn)(PART_s *);    // 0x4c
    void (*replace_fn)(PART_s *); // 0x50
    i32 (*draw_fn)(PART_s *);     // 0x54
    i32 field_58;
    i32 *debris_key; // 0x5c
    i32 field_60, field_64;
    f32 field_68, field_6c;
    i32 field_70, field_74, field_78;
    f32 field_7c;
    i32 *lighting_template; // 0x80, receives rtlDynamicAllocTemplate's result
    i32 field_84;
    f32 field_88;
    f32 time_step; // 0x8c
    i32 field_90;
    PARTLIGHTSOURCE_s *lighting; // 0x94
    f32 field_98, field_9c, field_a0;
    f32 field_a4;
    u32 field_a8[6];
    f32 field_c0;
    u8 field_c4;
    u8 field_c5[3];
};
DECOMP_ASSERT(sizeof(ADDPART_s) == 0xc8, "ADDPART_s ABI");
typedef ADDPART_s ADDPART_ALIGNED16 __attribute__((aligned(16)));
DECOMP_ASSERT(offsetof(ADDPART_s, matrix) == 0x00, "ADDPART matrix offset");
DECOMP_ASSERT(offsetof(ADDPART_s, velocity) == 0x08, "ADDPART velocity offset");
DECOMP_ASSERT(offsetof(ADDPART_s, field_14) == 0x14, "ADDPART field_14 offset");
DECOMP_ASSERT(offsetof(ADDPART_s, field_18) == 0x18, "ADDPART field_18 offset");
DECOMP_ASSERT(offsetof(ADDPART_s, gravity) == 0x1c, "ADDPART gravity offset");
DECOMP_ASSERT(offsetof(ADDPART_s, flags) == 0x2c, "ADDPART flags offset");
DECOMP_ASSERT(offsetof(ADDPART_s, field_40) == 0x40, "ADDPART collision callback offset");
DECOMP_ASSERT(offsetof(ADDPART_s, special) == 0x24, "ADDPART special offset");
DECOMP_ASSERT(offsetof(ADDPART_s, move_fn) == 0x34, "ADDPART movement callback offset");
DECOMP_ASSERT(offsetof(ADDPART_s, stop_fn) == 0x4c, "ADDPART stop callback offset");
DECOMP_ASSERT(offsetof(ADDPART_s, draw_fn) == 0x54, "ADDPART draw callback offset");
DECOMP_ASSERT(offsetof(ADDPART_s, time_step) == 0x8c, "ADDPART time step offset");
struct AIANTINODE_s;
struct AIAREA_s;
struct AIGROUP_s;
struct AILOCATOR_s;
struct AIPACKET_s;
struct AIPATHCNXCONTROLLER_s {
    NULISTLNK links;         // 0x00
    AIPATHCNX_s *connection; // 0x08
    union {
        void *target;          // 0x0c, gizmo/cutscene/fake-animation target
        nuhspecial_s special;  // 0x0c, target_type == 0
        i32 fake_animation_id; // 0x0c, target_type == 5
    };
    i32 gizmo_output;  // 0x18
    u32 on_flags;      // 0x1c
    u32 off_flags;     // 0x20
    u32 on_frames[32]; // 0x24, one bit per frame (maximum 1024)
    u8 target_type;    // 0xa4
    u8 path_index;     // 0xa5
    u8 flags;          // 0xa6
    u8 padding_0xa7;
};
DECOMP_ASSERT(sizeof(AIPATHCNXCONTROLLER_s) == 0xa8, "AIPATHCNXCONTROLLER_s size");
DECOMP_ASSERT(offsetof(AIPATHCNXCONTROLLER_s, connection) == 0x08, "AI path controller connection offset");
DECOMP_ASSERT(offsetof(AIPATHCNXCONTROLLER_s, on_frames) == 0x24, "AI path controller frame mask offset");
DECOMP_ASSERT(offsetof(AIPATHCNXCONTROLLER_s, flags) == 0xa6, "AI path controller flags offset");
struct AIPATHCNXCONTROLSYS_s {
    i32 controller_count;
    AIPATHCNXCONTROLLER_s *controllers;
    NULISTHDR available_controllers;
    NULISTHDR active_controllers;
};
DECOMP_ASSERT(sizeof(AIPATHCNXCONTROLSYS_s) == 0x18, "AIPATHCNXCONTROLSYS_s size");
struct AIPATHCNXHELPER_s {
    AIPATHCNX_s *connection;
    void *target;
    u8 direction;
    u8 type;
    u8 padding_0x0a[2];
    f32 jump_off_dy;
};
DECOMP_ASSERT(sizeof(AIPATHCNXHELPER_s) == 0x10, "AIPATHCNXHELPER_s size");
struct AIPATHCNXHELPERSYS_s {
    i16 field_0x00;
    i16 helper_count;
    AIPATHCNXHELPER_s *helpers;
};
DECOMP_ASSERT(sizeof(AIPATHCNXHELPERSYS_s) == 0x8, "AIPATHCNXHELPERSYS_s size");
struct AIPATHINFO_s;
struct AIPATHNODE_s;
struct AISCRIPTPROCESS_s;
struct AISCRIPT_s;
struct AITRIGGERSET_TARGET {
    u8 reserved_00[0x10];
    NUVEC position;
    u8 reserved_1c[4];
    AIPATHINFO path;
    u8 reserved_38[4];
};
DECOMP_ASSERT(sizeof(AITRIGGERSET_TARGET) == 0x3c, "AITRIGGERSET target size");
struct AITRIGGERSET_s {
    union {
        u8 pad_0x000[0x204];
        struct {
            union {
                u32 reserved_000;
                FLOWBOX_s *flowbox;
            };
            union {
                GIZMO_s *triggers[8];
                GIZMO_s *gizmos[8];
            };
            union {
                AITRIGGERSET_TARGET targets[8];
                AILOCATOR locators[8];
            };
        };
    };
    i8 trigger_indices[8];
    union {
        u8 pad_0x20c[8];
        struct {
            u8 trigger_count;
            u8 field_20d;
            u16 field_0x20e;
            u8 flags;
            u8 reserved_211[3];
        };
    };
};
DECOMP_ASSERT(sizeof(AITRIGGERSET_s) == 0x214, "AITRIGGERSET_s size");
DECOMP_ASSERT(offsetof(AITRIGGERSET_s, flags) == 0x210, "AITRIGGERSET flags offset");
struct AITRIGGERSETSYS_s {
    AITRIGGERSET_s sets[32];
    i8 field_0x4280[64]; // Trigger-set index assigned to each Obj[] slot, or -1.
    i8 field_0x42c0[64]; // Trigger index within that set, or -1.
    i32 field_0x4300;
};
DECOMP_ASSERT(sizeof(AITRIGGERSETSYS_s) == 0x4304, "AITRIGGERSETSYS_s size");
struct ANIMREDIRECT {
    char *name;
    i16 animation_id;
    u8 pad_06[2];
};
DECOMP_ASSERT(sizeof(ANIMREDIRECT) == 0x8, "ANIMREDIRECT ABI");
struct AREADATA_s;
struct AREASAVE_s {
    u8 complete;
    u8 area_complete;
    u8 story_buildup_complete;
    u8 freeplay_buildup_complete;
    u8 minikit_complete;
    union {
        u8 field_0x5[3];
        struct {
            u8 minikit_count, red_brick_collected, reserved_0x7;
        };
    };
    f32 challenge_trial_time;
};
DECOMP_ASSERT(sizeof(AREASAVE_s) == 0xc, "AREASAVE_s size");
DECOMP_ASSERT(offsetof(AREASAVE_s, area_complete) == 0x1, "AREASAVE area completion offset");
DECOMP_ASSERT(offsetof(AREASAVE_s, challenge_trial_time) == 0x8, "AREASAVE challenge time offset");
struct ATTRACTO_s;
struct BATARANG_s {
    u8 pad_0x00[0x7c];
    u8 active; // 0x7c
    u8 field_0x7d;
    u8 pad_0x7e[0x84 - 0x7e];
    i32 target_id; // 0x84
    u8 pad_0x88[0xac - 0x88];
    u16 cooldown; // 0xac
    u8 pad_0xae[0xb4 - 0xae];
};
DECOMP_ASSERT(sizeof(BATARANG_s) == 0xb4, "BATARANG_s size");
struct BOLTTYPE_s;
struct BOLTSYS {
    BOLTTYPE_s *types;
    i32 count;
    void (*stop_targeting)(GameObject_s *, NUVEC *);
    void (*debris)(BOLT_s *, NUVEC *, i32, NUVEC *, i32);
    void (*shoot_origin)(GameObject_s *, NUVEC *);
    i32 (*shoot_direction)(GameObject_s *, NUVEC *);
    i32 (*hit_part)(BOLT_s *, PART_s *);
    i32 (*alternate_fire)(GameObject_s *, i32);
};
DECOMP_ASSERT(sizeof(BOLTSYS) == 0x20, "BOLTSYS ABI");
DECOMP_ASSERT(offsetof(BOLTSYS, stop_targeting) == 8, "BOLTSYS targeting callback offset");
extern BOLTSYS *BoltSys;
struct BOLT_s {
    BOLTTYPE_s *type;
    GameObject_s *owner;      // 0x04
    NUMTX orientation;        // 0x08, decoded shot matrix
    NUMTX effect_orientation; // 0x48, quarter-turn X composed with shot rotation
    NUVEC position;           // 0x88
    NUVEC previous_position;  // 0x94
    NUVEC velocity;           // 0xa0
    NUVEC field_0xac;
    NUVEC ray_end; // 0xb8, unclipped ray endpoint
    f32 scale;     // 0xc4
    f32 time;      // 0xc8
    f32 speed;
    f32 lifetime;         // 0xd0
    f32 radius;           // 0xd4
    f32 collision_radius; // 0xd8
    f32 ray_time;         // 0xdc
    f32 ray_radius;       // 0xe0
    f32 field_0xe4;
    f32 field_0xe8;
    f32 acceleration_y; // 0xec
    u32 flags;          // 0xf0
    u16 surface_x_rotation;
    u16 surface_z_rotation;
    u16 hit_flags;       // 0xf8, passed to ObjHitObj
    i16 hit_platform;    // 0xfa
    i16 hit_debris;      // 0xfc
    i16 hit_part_debris; // 0xfe
    u8 active;           // 0x100
    u8 type_id;          // 0x101
    u8 field_0x102;
    u8 index; // 0x103
    i8 field_0x104;
    i8 field_0x105;
    u8 pad_0x106[2];
    NUVEC bounds_min;            // 0x108, position minus collision radius
    NUVEC bounds_max;            // 0x114, position plus collision radius
    NUVEC hit_normal;            // 0x120, platform deflection normal
    NUVEC dogfight_hit_position; // 0x12c
};
DECOMP_ASSERT(sizeof(BOLT_s) == 0x138, "BOLT_s ABI");
DECOMP_ASSERT(offsetof(BOLT_s, owner) == 4, "BOLT owner offset");
DECOMP_ASSERT(offsetof(BOLT_s, orientation) == 0x08, "BOLT orientation offset");
DECOMP_ASSERT(offsetof(BOLT_s, effect_orientation) == 0x48, "BOLT effect orientation offset");
DECOMP_ASSERT(offsetof(BOLT_s, previous_position) == 0x94, "BOLT previous position offset");
DECOMP_ASSERT(offsetof(BOLT_s, bounds_min) == 0x108, "BOLT lower bounds offset");
DECOMP_ASSERT(offsetof(BOLT_s, bounds_max) == 0x114, "BOLT upper bounds offset");
DECOMP_ASSERT(offsetof(BOLT_s, position) == 0x88, "BOLT position offset");
DECOMP_ASSERT(offsetof(BOLT_s, velocity) == 0xa0, "BOLT velocity offset");
DECOMP_ASSERT(offsetof(BOLT_s, time) == 0xc8, "BOLT time offset");
DECOMP_ASSERT(offsetof(BOLT_s, lifetime) == 0xd0, "BOLT lifetime offset");
DECOMP_ASSERT(offsetof(BOLT_s, field_0xac) == 0xac, "BOLT deflection position offset");
DECOMP_ASSERT(offsetof(BOLT_s, type_id) == 0x101, "BOLT type offset");
DECOMP_ASSERT(offsetof(BOLT_s, speed) == 0xcc, "BOLT speed offset");
DECOMP_ASSERT(offsetof(BOLT_s, hit_flags) == 0xf8, "BOLT hit flags offset");
DECOMP_ASSERT(offsetof(BOLT_s, scale) == 0xc4, "BOLT scale offset");
DECOMP_ASSERT(offsetof(BOLT_s, radius) == 0xd4, "BOLT radius offset");
DECOMP_ASSERT(offsetof(BOLT_s, collision_radius) == 0xd8, "BOLT collision radius offset");
DECOMP_ASSERT(offsetof(BOLT_s, acceleration_y) == 0xec, "BOLT acceleration offset");
DECOMP_ASSERT(offsetof(BOLT_s, index) == 0x103, "BOLT index offset");
DECOMP_ASSERT(offsetof(BOLT_s, hit_normal) == 0x120, "BOLT hit normal offset");
DECOMP_ASSERT(offsetof(BOLT_s, ray_end) == 0xb8, "BOLT ray endpoint offset");
enum BUILDIT_FIND_ENUM : i32 {
    BUILDIT_FIND_USABLE = 0,
    BUILDIT_FIND_AVAILABLE = 1,
    BUILDIT_FIND_ANY = 2,
};
struct CABLE_s {
    GameObject_s *source;
    GameObject_s *target;
    NUVEC points[16];
    NUVEC velocities[16];
    f32 segment_lengths[15];
    f32 max_length;
    f32 total_length;
    u8 unknown_1cc[8];
    f32 pull_time;
    u8 wrap_indices[15];
    u8 point_count;
    u8 wrap_count;
    u8 flags_1e9;
    u8 unknown_1ea[2];
};
DECOMP_ASSERT(offsetof(CABLE_s, segment_lengths) == 0x188, "CABLE_s segment lengths offset");
DECOMP_ASSERT(offsetof(CABLE_s, total_length) == 0x1c8, "CABLE_s total length offset");
DECOMP_ASSERT(offsetof(CABLE_s, pull_time) == 0x1d4, "CABLE_s pull time offset");
DECOMP_ASSERT(offsetof(CABLE_s, wrap_indices) == 0x1d8, "CABLE_s wrap indices offset");
DECOMP_ASSERT(offsetof(CABLE_s, wrap_count) == 0x1e8, "CABLE_s wrap count offset");
DECOMP_ASSERT(offsetof(CABLE_s, points) == 0x08, "CABLE_s points offset");
DECOMP_ASSERT(offsetof(CABLE_s, velocities) == 0xc8, "CABLE_s velocities offset");
DECOMP_ASSERT(offsetof(CABLE_s, point_count) == 0x1e7, "CABLE_s point count offset");
DECOMP_ASSERT(offsetof(CABLE_s, target) == 0x04, "CABLE_s target offset");
DECOMP_ASSERT(offsetof(CABLE_s, max_length) == 0x1c4, "CABLE_s max length offset");
DECOMP_ASSERT(offsetof(CABLE_s, flags_1e9) == 0x1e9, "CABLE_s tied flags offset");
DECOMP_ASSERT(sizeof(CABLE_s) == 0x1ec, "CABLE_s ABI");
struct CHARACTERDATA_s {};
struct CHARCATEGORY {
    union {
        char *name;
        u8 field0_0x0[4];
    };
    union {
        u32 model_flags;
        u32 field1_0x4;
    };
    union {
        u32 game_flags;
        i32 field2_0x8;
    };
};
DECOMP_ASSERT(sizeof(CHARCATEGORY) == 0x0c, "CHARCATEGORY ABI");
DECOMP_ASSERT(offsetof(CHARCATEGORY, model_flags) == 4, "CHARCATEGORY model flags offset");
DECOMP_ASSERT(offsetof(CHARCATEGORY, game_flags) == 8, "CHARCATEGORY game flags offset");
struct CHARFIXUP;
struct CHARPIVOT {
    char *name;
    i16 animation;
    i16 point_of_interest;
    f32 start_frame;
    f32 end_frame;
    i16 character_id;
    u16 padding;
};
DECOMP_ASSERT(sizeof(CHARPIVOT) == 0x14, "CHARPIVOT ABI");
DECOMP_ASSERT(offsetof(CHARPIVOT, animation) == 4, "CHARPIVOT animation offset");
DECOMP_ASSERT(offsetof(CHARPIVOT, point_of_interest) == 6, "CHARPIVOT point offset");
DECOMP_ASSERT(offsetof(CHARPIVOT, start_frame) == 8, "CHARPIVOT start frame offset");
DECOMP_ASSERT(offsetof(CHARPIVOT, end_frame) == 12, "CHARPIVOT end frame offset");
DECOMP_ASSERT(offsetof(CHARPIVOT, character_id) == 16, "CHARPIVOT character ID offset");
struct CHARPLATFORM_s {
    nuhspecial_s special;
    i16 object_id;
    i16 platform_id;
    GameObject_s *object;
};
DECOMP_ASSERT(sizeof(CHARPLATFORM_s) == 0x14, "CHARPLATFORM_s size");
struct CHARPLATFORMSYS_s {
    i32 field_0x00;
    i32 platform_count;
    CHARPLATFORM_s platforms[1];
};
DECOMP_ASSERT(offsetof(CHARPLATFORMSYS_s, platforms) == 0x8, "CHARPLATFORMSYS platforms offset");
struct CHARVARIANT {};
struct CHEAT;
struct CLIMBOBJECT_s {
    NUVEC normal;
    f32 impact_x;
    f32 impact_z;
    AIPATHCNX_s *connection;
    AIPATH_s *path;
    u8 flags;
    u8 field_0x1d[3];
};
struct CLIMBOBJECTSYS_s {
    CLIMBOBJECT_s *objects;
    u16 count;
    u16 capacity;
};
DECOMP_ASSERT(sizeof(CLIMBOBJECT_s) == 0x20, "CLIMBOBJECT_s size");
DECOMP_ASSERT(sizeof(CLIMBOBJECTSYS_s) == 8, "CLIMBOBJECTSYS_s size");
struct CUSTOMPIECECATEGORY {
    u32 field_0;
    u8 uses_special;
};
struct CUSTOMISER {
    union {
        u8 pad_0x00[0x6c];
        struct {
            CUSTOMPIECECATEGORY *categories[9];
            CUSTOMPIECE *piece_sets[9];
            i32 piece_counts[9];
        };
        struct {
            u8 pad_custom_pieces[0x38];
            struct CUSTOMPIECE *pieces;
            u8 pad_custom_after_pieces[0x30];
        };
    };
    i16 character_ids[2];
    char display_names[2][0x80];
    i32 (*piece_available)(CUSTOMPIECE *);
    CUSTOMISESAVE_s *save;             // 0x174
    ANIMPACKET_s animation_packets[2]; // 0x178
    i32 model_texture_ids[18];         // 0x208
    u8 pad_0x250[0xa6c - 0x250];
    u8 animation_active[2];  // 0xa6c
    u8 animation_state[2];   // 0xa6e
    i32 animation_values[2]; // 0xa70
    u8 pad_0xa78[0xc28 - 0xa78];
    i16 default_pieces[2][10]; // 0xc28; nine saved pieces plus one unused entry per character
};
DECOMP_ASSERT(offsetof(CUSTOMISER, piece_sets) == 0x24, "CUSTOMISER piece arrays");
DECOMP_ASSERT(offsetof(CUSTOMISER, piece_counts) == 0x48, "CUSTOMISER piece counts");
DECOMP_ASSERT(offsetof(CUSTOMISER, piece_available) == 0x170, "CUSTOMISER piece availability callback");
DECOMP_ASSERT(sizeof(CUSTOMISER) == 0xc50, "CUSTOMISER recovered prefix size");
DECOMP_ASSERT(offsetof(CUSTOMISER, save) == 0x174, "CUSTOMISER save offset");
DECOMP_ASSERT(offsetof(CUSTOMISER, default_pieces) == 0xc28, "CUSTOMISER default pieces offset");
DECOMP_ASSERT(offsetof(CUSTOMISER, character_ids) == 0x6c, "CUSTOMISER character IDs offset");
DECOMP_ASSERT(offsetof(CUSTOMISER, animation_packets) == 0x178, "CUSTOMISER animation packets offset");
DECOMP_ASSERT(offsetof(CUSTOMISER, model_texture_ids) == 0x208, "CUSTOMISER model texture IDs offset");
DECOMP_ASSERT(offsetof(CUSTOMISER, animation_active) == 0xa6c, "CUSTOMISER animation active offset");
struct __attribute__((packed)) CUSTOMISESAVE_s {
    i16 pieces[9];           // 0x00
    u8 field_0x12[2];        // 0x12
    char primary_name[0x20]; // 0x14
    union {
        u8 primary_name_unlocked;
        u8 primary_use_saved_name;
    }; // 0x34
    u8 field_0x35[3];        // 0x35
    i16 secondary_pieces[9]; // 0x38
    union {
        u8 field_0x4a[2];
        u8 secondary_piece_flags[2];
    }; // 0x4a
    char secondary_name[0x20]; // 0x4c
    union {
        u8 secondary_name_unlocked;
        u8 secondary_use_saved_name;
    }; // 0x6c
    u8 field_0x6d[2]; // 0x6d
};
DECOMP_ASSERT(sizeof(CUSTOMISESAVE_s) == 0x6f, "CUSTOMISESAVE_s size");
DECOMP_ASSERT(offsetof(CUSTOMISESAVE_s, secondary_pieces) == 0x38, "CUSTOMISESAVE secondary pieces offset");
DECOMP_ASSERT(offsetof(CUSTOMISESAVE_s, primary_name) == 0x14, "CUSTOMISESAVE primary name offset");
DECOMP_ASSERT(offsetof(CUSTOMISESAVE_s, primary_name_unlocked) == 0x34, "CUSTOMISESAVE primary flag offset");
DECOMP_ASSERT(offsetof(CUSTOMISESAVE_s, secondary_name) == 0x4c, "CUSTOMISESAVE secondary name offset");
DECOMP_ASSERT(offsetof(CUSTOMISESAVE_s, secondary_name_unlocked) == 0x6c, "CUSTOMISESAVE secondary flag offset");
struct CUSTOMPIECE {
    u8 unknown_00[4];
    i16 character_id;
    i16 icon_character_id;
    u8 unknown_08[0xa];
    union {
        struct {
            u8 layer_flags;
            u8 availability_flags_high;
        };
        u16 availability_flags;
    }; // 0x12
    u8 unknown_14[0x14];
};
DECOMP_ASSERT(sizeof(CUSTOMPIECE) == 0x28, "CUSTOMPIECE size");
DECOMP_ASSERT(offsetof(CUSTOMPIECE, layer_flags) == 0x12, "CUSTOMPIECE layer flags offset");
DECOMP_ASSERT(offsetof(CUSTOMISESAVE_s, secondary_pieces) == 0x38, "CUSTOMISESAVE secondary pieces offset");
struct CUTSCENESFX {
    i16 id;
    u8 flags;
    u8 pad_03;
    f32 frame;
    NUVEC position;
};
DECOMP_ASSERT(sizeof(CUTSCENESFX) == 0x14, "CUTSCENESFX size");

enum CUTSCENEPLAYEROBJ_FLAGS {
    CLIP_OBJECT_SHOW = 1,
    CLIP_OBJECT_HIDE = 2,
    CLIP_OBJECT_ANIM_END = 4,
};
struct CUTSCENEPLAYEROBJ {
    nuhspecial_s special;
    u8 flags;
    u8 padding[3];
};
DECOMP_ASSERT(sizeof(CUTSCENEPLAYEROBJ) == 0x10, "CUTSCENEPLAYEROBJ size");

struct CUTSCENETEXANIM {
    f32 frame;
    i32 index;
};
DECOMP_ASSERT(sizeof(CUTSCENETEXANIM) == 8, "CUTSCENETEXANIM size");

struct CUTSCENEFADEFOG {
    f32 frame;
    f32 near_distance;
    f32 far_distance;
    f32 value;
};
DECOMP_ASSERT(sizeof(CUTSCENEFADEFOG) == 0x10, "CUTSCENEFADEFOG size");

struct CUTSCENESUBTITLE {
    i16 text_id;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    u8 alignment;
    u8 pad_07;
    f32 start_frame;
    f32 end_frame;
    f32 x;
    f32 y;
    f32 x_scale;
    f32 y_scale;
    f32 max_width;
    f32 fade_time;
};
DECOMP_ASSERT(sizeof(CUTSCENESUBTITLE) == 0x28, "CUTSCENESUBTITLE size");

struct CUTINFO {
    void *scene;
    void *instance;
    char name[0x40];
    CUTSCENEPLAYEROBJ *state_entries;
    u8 state_count;
    u8 pad_4d[3];
    u32 flags;
    f32 previous_frame;
    f32 field_58;
    f32 frames_per_second;
    f32 burnout_threshold;
    f32 burnout_intensity;
    f32 burnout_flare;
    f32 camera_near_clip; // 0x6c, zero keeps the level display setting
    CUTSCENESFX sfx[6];   // 0x70
    u16 camera_far_clip;  // 0xe8, zero keeps the level display setting
    i16 legacy_music_index;
    i16 skip_level; // 0xec, optional level selected when a stopped cutscene is skipped
    union {
        u8 linked_audio;
        u8 linked_music_mode;
    };
    u8 blob_shadow_alpha;
    u8 blob_shadow_fade_near;
    u8 blob_shadow_fade_far;
    u8 reflection_range;
    i8 debris_render_group;
    char door_name[0x10];
    char next_cutscene[0x40];
    CUTSCENETEXANIM texture_animations[4];
    CUTSCENEFADEFOG fade_fog[2];
    CUTSCENESUBTITLE *subtitle_data;
    u16 subtitle_count;
    union {
        u8 end_flags;
        u8 playback_flags;
    };
    u8 pad_18b;
    union {
        i32 music_handle;
        i32 music_track_handle;
    };
    f32 low_end_distance;
    f32 field_194;
};
DECOMP_ASSERT(offsetof(CUTINFO, burnout_threshold) == 0x60, "CUTINFO burnout threshold offset");
DECOMP_ASSERT(offsetof(CUTINFO, texture_animations) == 0x144, "CUTINFO texture animations offset");
DECOMP_ASSERT(offsetof(CUTINFO, fade_fog) == 0x164, "CUTINFO fade-fog offset");
DECOMP_ASSERT(offsetof(CUTINFO, subtitle_data) == 0x184, "CUTINFO subtitle data offset");
DECOMP_ASSERT(offsetof(CUTINFO, camera_near_clip) == 0x6c, "CUTINFO near-clip offset");
DECOMP_ASSERT(offsetof(CUTINFO, sfx) == 0x70, "CUTINFO SFX offset");
DECOMP_ASSERT(offsetof(CUTINFO, camera_far_clip) == 0xe8, "CUTINFO far-clip offset");
DECOMP_ASSERT(offsetof(CUTINFO, legacy_music_index) == 0xea, "CUTINFO legacy music-index offset");
DECOMP_ASSERT(offsetof(CUTINFO, skip_level) == 0xec, "CUTINFO skip-level offset");
DECOMP_ASSERT(offsetof(CUTINFO, debris_render_group) == 0xf3, "CUTINFO debris render-group offset");
DECOMP_ASSERT(offsetof(CUTINFO, next_cutscene) == 0x104, "CUTINFO chained-cutscene offset");
DECOMP_ASSERT(offsetof(CUTINFO, end_flags) == 0x18a, "CUTINFO end-flags offset");
DECOMP_ASSERT(offsetof(CUTINFO, music_handle) == 0x18c, "CUTINFO music-handle offset");
DECOMP_ASSERT(sizeof(CUTINFO) == 0x198, "CUTINFO size");
DECOMP_ASSERT(offsetof(CUTINFO, music_track_handle) == 0x18c, "CUTINFO music offset");
DECOMP_ASSERT(offsetof(CUTINFO, texture_animations) + offsetof(CUTSCENETEXANIM, index) == 0x148,
              "CUTINFO texture signals offset");
struct CUTSCENESYS {
    i16 blaster_object_0;
    i16 blaster_object_1;
    i16 field_04;
    i16 field_06;
};
struct CUTSYS {
    CUTINFO **cuts;
    i32 count;
    u32 *character_bits;
};
struct ClassItem {};
struct DETONATOR_s {
    u8 field_0x00[0xc];
    NUVEC field_0x0c;
    NUVEC field_0x18;
    GameObject_s *object;
    u8 active;
    u8 field_0x29[7];
    f32 timer;
    void *field_0x34;
};
DECOMP_ASSERT(sizeof(DETONATOR_s) == 0x38, "DETONATOR_s size");
struct EDCREATURE_s {};
struct EPISODEDATA;
struct EXPLOSION {
    i32 field_0x00;
    i32 field_0x04;
    GameObject_s *object;
    NUVEC position;
    f32 field_0x18;
    f32 field_0x1c;
    f32 field_0x20;
    i32 field_0x24;
    u16 field_0x28;
    u16 field_0x2a;
    u16 field_0x2c;
    u16 field_0x2e;
    u16 field_0x30;
    u8 field_0x32;
    u8 field_0x33;
};
DECOMP_ASSERT(sizeof(EXPLOSION) == 0x34, "EXPLOSION size");
struct EXTRAMODEL {
    i16 *model_list;
    void *field_04;
};
DECOMP_ASSERT(sizeof(EXTRAMODEL) == 8, "EXTRAMODEL size");
struct EdClass;
struct EdControl;
struct EdFileInputStream;
struct EdFileOutputStream;
struct EdInputContext;
struct EdRef;
struct EdStream;
struct EdTool {
    u8 reserved_0x00[4];
    EdTool *next;
    EdTool *previous;
};
enum FADETYPE_VALUE {
    FADE_TYPE_NONE = -1,
    FADE_TYPE_SCREEN = 0,
    FADE_TYPE_WIPE = 1,
    FADE_TYPE_STILL_WIPE = 2,
    FADE_TYPE_STILL = 3,
};
struct FADEINFO_s {
    // FadeSystem passes its own storage to the individual fade objects as a
    // FADEINFO_s*.  The original object is 0x2c bytes; keep these fields
    // named instead of accessing the storage through byte offsets.
    u32 direction;
    f32 fade;
    f32 rate;
    i32 busy;
    i32 stage;
    FadeBase *fades[4];
    FADETYPE_VALUE pending_type;
    i32 field_28;
};
DECOMP_ASSERT(offsetof(FADEINFO_s, fade) == 4, "FADEINFO fade offset");
struct FADETYPE {
    FADETYPE_VALUE type;
};
DECOMP_ASSERT(sizeof(FADETYPE) == 4, "FADETYPE ABI");
struct FLOWBOXGIZMOREF_s {
    GIZMO_s *gizmo;
    char *name;
};
DECOMP_ASSERT(sizeof(FLOWBOXGIZMOREF_s) == 8, "Flow-box gizmo reference ABI");
struct FLOWBOXGIZMODATA_s {
    i32 gizmo_count;
    union {
        i32 capacity;
        u8 pad_0x04[4];
    };
    FLOWBOXGIZMOREF_s **gizmos;
};
DECOMP_ASSERT(sizeof(FLOWBOXGIZMODATA_s) == 0xc, "FLOWBOXGIZMODATA_s ABI");
struct FLOWBOXACTION_s {
    FLOWBOXACTION_s *next;
    union {
        char **parameters;
        char **arguments;
    };
    union {
        i32 parameter_count;
        i32 argument_count;
    };
    GIZACTIONDEFN_s *definition;
};
typedef FLOWBOXACTION_s FLOWBOXACTIONDATA_s;
DECOMP_ASSERT(sizeof(FLOWBOXACTION_s) == 0x10, "Flow-box action node ABI");
DECOMP_ASSERT(offsetof(FLOWBOXACTION_s, parameters) == 0x04, "Flow-box action parameters offset");
DECOMP_ASSERT(offsetof(FLOWBOXACTION_s, parameter_count) == 0x08, "Flow-box action parameter count offset");
DECOMP_ASSERT(offsetof(FLOWBOXACTION_s, definition) == 0x0c, "Flow-box action definition offset");
struct FLOWBOX_s {
    u8 parent_count;
    u8 loop_parent_count;
    u8 child_count;
    u8 type;
    union {
        u32 runtime_id;
        struct {
            u8 ai_trigger_group;
            u8 runtime_id_upper[3];
        };
    };
    u8 last_process_frame;
    u8 loop_checksum;
    union {
        u16 state_flags;
        struct {
            u8 state_flags_low;
            u8 state_flags_high;
        };
    };
    union {
        FLOWBOXGIZMODATA_s *data;
        FLOWBOXACTION_s *actions;
        u8 *condition_data;
    };
    FLOWBOX_s **parents;
    FLOWBOX_s **children;
    u8 *output_indices;
    char *name;
};
DECOMP_ASSERT(sizeof(FLOWBOX_s) == 0x20, "FLOWBOX_s ABI");
DECOMP_ASSERT(offsetof(FLOWBOX_s, state_flags_low) == 0xa, "FLOWBOX completion flags offset");
DECOMP_ASSERT(offsetof(FLOWBOX_s, ai_trigger_group) == 4, "FLOWBOX AI trigger group offset");
struct FS_FILEENTRYHDR {};
struct FadeBase {
    virtual ~FadeBase() = default;
    virtual void Init(FADEINFO_s *) = 0;
    virtual void InitFade() = 0;
    virtual void UpdateFade() = 0;
    virtual void DrawFade() = 0;
    virtual FADETYPE_VALUE GetFadeType() const = 0;
    FADEINFO_s *info;
};
struct GAMEANIMOBJ_s {
    GAMEANIMOBJ_s *next;
    nuhspecial_s special;             // 0x04
    nuinstanim_s *instance_animation; // 0x10
    nuanimdata_s *animation;          // 0x14
    f32 start_frame;                  // 0x18
    f32 end_frame;                    // 0x1c
    f32 current_frame;                // 0x20
    u32 flags;                        // 0x24
    void *object_data;                // 0x28
};
DECOMP_ASSERT(sizeof(GAMEANIMOBJ_s) == 0x2c, "GAMEANIMOBJ_s ABI");

struct GAMEANIMOBJPOOL_s {
    u16 object_data_size; // 0x00
    u16 active_count;     // 0x02
    u16 capacity;         // 0x04
    u16 field_0x06;
    GAMEANIMOBJ_s *objects;      // 0x08
    GAMEANIMOBJ_s *free_objects; // 0x0c
    void *object_data;           // 0x10
};
DECOMP_ASSERT(sizeof(GAMEANIMOBJPOOL_s) == 0x14, "GAMEANIMOBJPOOL_s ABI");

enum GAMEANIMSET_FLAGS : u8 {
    GAMEANIMSET_FLAG_NO_VISIBILITY_TEST = 0x01,
    GAMEANIMSET_FLAG_STOP_REQUESTED = 0x02,
    GAMEANIMSET_FLAG_IN_SYSTEM_LIST = 0x04,
};

enum GAMEANIMSET_STATE : i32 {
    GAMEANIMSET_STATE_AT_START = 0,
    GAMEANIMSET_STATE_ACTIVE_FORWARD = 1,
    GAMEANIMSET_STATE_AT_END = 2,
    GAMEANIMSET_STATE_ACTIVE_BACKWARD = 3,
    GAMEANIMSET_STATE_BETWEEN_ENDPOINTS = 4,
};

enum GAMEANIMSET_VISIBILITY : i32 {
    GAMEANIMSET_VISIBILITY_NONE = 0,
    GAMEANIMSET_VISIBILITY_PARTIAL = 1,
    GAMEANIMSET_VISIBILITY_ALL = 2,
};

struct GAMEANIMSET_s {
    NULISTLNK links;          // 0x00
    u8 object_count;          // 0x08
    u8 animated_object_count; // 0x09
    u8 flags;                 // 0x0a, GAMEANIMSET_FLAGS
    u8 field_0x0b;
    GAMEANIMSET_STATE state;        // 0x0c
    GAMEANIMOBJPOOL_s *object_pool; // 0x10
    GAMEANIMSYS_s *system;          // 0x14
    GAMEANIMOBJ_s *objects;         // 0x18
};
DECOMP_ASSERT(sizeof(GAMEANIMSET_s) == 0x1c, "GAMEANIMSET_s ABI");

struct GAMEANIMSYS_s {
    NULISTHDR active_sets; // 0x00
    i32 set_count;         // 0x08
    GAMEANIMSET_s **sets;  // 0x0c
};
DECOMP_ASSERT(sizeof(GAMEANIMSYS_s) == 0x10, "GAMEANIMSYS_s ABI");
struct GAMEANTINODEDATA_s {
    NUVEC position;
    f32 radius;
    f32 min_y;
    f32 max_y;
    f32 extent_x;
    f32 extent_z;
    i16 flags;
    u8 use_largest_extent;
    u8 mode;
};
DECOMP_ASSERT(sizeof(GAMEANTINODEDATA_s) == 0x24, "GAMEANTINODEDATA_s ABI");
struct GAMEANTINODE_s {
    NUVEC position;
    f32 radius;
    f32 min_y;
    f32 max_y;
    f32 extent_x;
    f32 extent_z;
    f32 remaining_time;
    u32 user_data[2];
    u16 angle;
    u8 shape;
    u8 grid_min_x;
    u8 grid_min_z;
    u8 grid_max_x;
    u8 grid_max_z;
    u8 reserved_33;
    GAMEANTINODE_s *next;
};
struct GAMEANTINODESYS_s {
    GAMEANTINODE_s *nodes;
    GAMEANTINODE_s *active;
    GAMEANTINODE_s *free;
    WORLDINFO_s *world;
    i32 capacity;
    i32 count;
};
DECOMP_ASSERT(sizeof(GAMEANTINODE_s) == 0x38, "GAMEANTINODE_s ABI");
DECOMP_ASSERT(offsetof(GAMEANTINODE_s, user_data) == 0x24, "Game antinode user data offset");
DECOMP_ASSERT(offsetof(GAMEANTINODE_s, angle) == 0x2c, "Game antinode angle offset");
DECOMP_ASSERT(offsetof(GAMEANTINODE_s, next) == 0x34, "Game antinode link offset");
DECOMP_ASSERT(sizeof(GAMEANTINODESYS_s) == 0x18, "GAMEANTINODESYS_s ABI");
DECOMP_ASSERT(offsetof(GAMEANTINODESYS_s, world) == 0xc, "Game antinode world offset");
struct GAMEAUDIO {
    i32 (*override_footstep_fn)(GameObject_s *, i32);
    i32 (*check_reverb_fn)(void);
    const char *sfx_names[0x55];
    i16 sfx_ids[0x55];
};

DECOMP_ASSERT(offsetof(GAMEAUDIO, sfx_names) == 0x8, "GAMEAUDIO sfx_names offset");
DECOMP_ASSERT(offsetof(GAMEAUDIO, sfx_ids) == 0x15c, "GAMEAUDIO sfx_ids offset");
// Camera state (GameCam). The original global points directly at a static
// 0x230-byte block; matrix and mode offsets are verified against
// MoveGameCamera and InitGameBeforeConfig.
struct GAMECAMERA_s {
    SOCKPOSITION sock_position;       // 0x000; camera begins with its rail position state
    NUMTX mtx;                        // 0x038
    NUMTX target_mtx;                 // 0x078
    NUMTX render_mtx;                 // 0x0b8
    NUVEC shaken_right;               // 0x0f8
    NUVEC shaken_up;                  // 0x104
    NUVEC dir;                        // 0x110
    NUVEC pos;                        // 0x11c
    NUVEC target;                     // 0x128
    NUVEC blend_start_position;       // 0x134
    NUVEC desired_position;           // 0x140
    NUVEC blend_end_position;         // 0x14c
    NUVEC blend_destination_position; // 0x158; captured on the first blend frame
    NUVEC blend_start_target;         // 0x164
    u8 pad_170[0x17c - 0x170];
    NUVEC blend_end_target;         // 0x17c
    NUVEC blend_destination_target; // 0x188; captured on the first blend frame
    NUVEC shake_offset;             // 0x194; filtered camera shake applied to render_mtx
    NUVEC shake_direction;          // 0x1a0; current random shake direction
    NUVEC shake_target;             // 0x1ac; next random shake direction
    f32 field_0x1b8;
    f32 blend_time;          // 0x1bc
    f32 blend_duration;      // 0x1c0
    f32 blend_curve;         // 0x1c4
    f32 judder_time;         // 0x1c8; remaining impact-judder time
    f32 judder_duration;     // 0x1cc; initial impact-judder time
    f32 shake_amplitude;     // 0x1d0; filtered ambient-shake amplitude
    f32 shake_target_amount; // 0x1d4; amplitude used while shake_time is active
    f32 shake_time;          // 0x1d8; remaining forced-shake time
    f32 shake_speed;         // 0x1dc; forced-shake direction/filter speed
    f32 position_seek;
    f32 angle_seek;
    f32 field_0x1e8;
    f32 field_0x1ec;
    f32 field_0x1f0;
    f32 field_0x1f4;
    NUANG pitch; // 0x1f8
    union {
        NUANG yaw; // 0x1fc
        struct {
            u16 input_yaw; // camera-relative controller angle
            u16 yaw_padding;
        };
    };
    NUANG roll; // 0x200
    f32 field_0x204;
    f32 field_0x208;
    f32 field_0x20c;
    f32 field_0x210;
    f32 field_0x214;
    f32 field_0x218;
    u16 blend_start_pitch;   // 0x21c
    u16 desired_pitch;       // 0x21e
    u16 blend_start_yaw;     // 0x220
    u16 desired_yaw;         // 0x222
    u16 blend_start_roll;    // 0x224
    u16 desired_roll;        // 0x226
    u8 judder_reverse;       // 0x228
    u8 judder_axis;          // 0x229; 0 pitch, 1 yaw, otherwise roll
    u8 reset_blend;          // 0x22a
    u8 blend_mode;           // 0x22b
    i8 mode;                 // 0x22c
    i8 previous_mode;        // 0x22d
    i8 previous_camera_mode; // 0x22e
    u8 pad_22f;
};
DECOMP_ASSERT(sizeof(GAMECAMERA_s) == 0x230, "GAMECAMERA_s ABI");
DECOMP_ASSERT(offsetof(GAMECAMERA_s, shake_offset) == 0x194, "GAMECAMERA shake offset");
DECOMP_ASSERT(offsetof(GAMECAMERA_s, judder_time) == 0x1c8, "GAMECAMERA judder offset");
DECOMP_ASSERT(offsetof(GAMECAMERA_s, shake_amplitude) == 0x1d0, "GAMECAMERA shake amplitude offset");
DECOMP_ASSERT(offsetof(GAMECAMERA_s, judder_reverse) == 0x228, "GAMECAMERA judder flags offset");

// Camera-space containment plane: one point on the plane followed by its
// inward-facing normal.  The original PlayPlane global contains six of these.
struct PLAYPLANE_s {
    NUVEC point;
    NUVEC normal;
};
DECOMP_ASSERT(sizeof(PLAYPLANE_s) == 0x18, "PLAYPLANE_s ABI");

struct GAMECUTSCENES_s {
    CUTINFO *podrace_pod_explode;
    CUTINFO *podrace_out_of_time;
    CUTINFO *bonus_gunship_cavalry_explode;
    CUTINFO *droid_factory_conveyor;
    CUTINFO *podrace_avalanche;
    CUTINFO *dogfight_die;
    CUTINFO *podrace_sebulba;
    CUTINFO *cutscene; // 0x1c, Pod Sprint avalanche
    CUTINFO *podsprint_out_of_time;
    CUTINFO *podsprint_sebulba;
};

DECOMP_ASSERT(sizeof(GAMECUTSCENES_s) == 0x28, "GAMECUTSCENES_s size");
DECOMP_ASSERT(offsetof(GAMECUTSCENES_s, cutscene) == 0x1c, "GAMECUTSCENES_s active cutscene offset");

struct GAMEMESSAGE_s {
    char pad_0x00[0x88];
    NUVEC target_position;
    char pad_0x94[0xe6 - 0x94];
    u16 icon;   // 0xe6
    u32 color1; // 0xe8
    u32 color2; // 0xec
    u32 color3; // 0xf0
    char pad_0xf4[0xf7 - 0xf4];
    u8 alpha;  // 0xf7
    u8 active; // 0xf8
    u8 pad_0xf9[4];
    i8 player_index;
    u8 target_type; // 0xfe
    u8 pad_0xff[0x10c - 0xff];
    void (*draw_callback)(GAMEMESSAGE_s *, NUVEC *, f32); // 0x10c
    u8 pad_0x110[4];
};
DECOMP_ASSERT(sizeof(GAMEMESSAGE_s) == 0x114, "GAMEMESSAGE_s size");
static_assert(offsetof(GAMEMESSAGE_s, target_position) == 0x88, "game message target position offset");
static_assert(offsetof(GAMEMESSAGE_s, player_index) == 0xfd, "game message player index offset");
// Rumble state packet embedded in GAMEPAD_s (20 bytes; floats driven by
// NuSound3UpdateRumble / UpdateRumble).
struct RUMBLEPACKET {
    u32 field_0x00;
    f32 rumble_amount; // 0x04
    u32 field_0x08;
    f32 rumble_time; // 0x0c
    union {
        u32 field_0x10;
        struct {
            u8 active;
            u8 pad_0x11[3];
        };
    };
};
struct GAMEPAD_s {
    nupad_s *pad; // 0x00  the bound input pad (fields 0x00..0x1f)
    union {
        u32 buttons_held;
        u32 unknown_04;
    };
    union {
        u32 buttons_pressed;
        u32 buttons_down_08;
    };
    union {
        u32 buttons_released;
        u32 unknown_0c;
    };
    union {
        u32 left_directions;
        u32 unknown_10;
    };
    union {
        u32 previous_left_directions;
        u32 unknown_14;
    };
    union {
        u32 right_directions;
        u32 unknown_18;
    };
    union {
        u32 previous_right_directions;
        u32 unknown_1c;
    };
    u32 unknown_20;
    union {
        struct {
            u8 input_state;
            u8 input_mode;
            u16 input_angle;
        };
        u32 unknown_24;
    };
    union {
        f32 input_magnitude;
        f32 unknown_28;
    };
    f32 input_direction_z; // 0x2c
    f32 input_direction_x; // 0x30
    union {
        f32 peak_input_magnitude;
        f32 animation_input_magnitude; // 0x34, speed consumed by AnimatePlayer
    };
    f32 previous_input_magnitude;
    f32 waggle_magnitude;
    RUMBLEPACKET rumble_packet; // 0x40
    void *operator_data;        // 0x54
    u16 previous_input_angle;
    u8 allocated_5a; // allocation and input-state flags
    char pad_5b[0x60 - 0x5b];
};
enum GAMEPAD_RUNTIME_FLAGS : u8 {
    GAMEPAD_RUNTIME_ALLOCATED = 0x01,
    GAMEPAD_RUNTIME_SUPPRESS_MOVEMENT = 0x02,
    GAMEPAD_RUNTIME_WAGGLED = 0x08,
    GAMEPAD_RUNTIME_MAGNITUDE_CHANGED = 0x40,
};
DECOMP_ASSERT(sizeof(GAMEPAD_s) == 0x60, "GAMEPAD_s ABI");
DECOMP_ASSERT(offsetof(GAMEPAD_s, input_mode) == 0x25, "GAMEPAD input mode offset");
DECOMP_ASSERT(offsetof(GAMEPAD_s, input_direction_z) == 0x2c, "GAMEPAD first direction component offset");
DECOMP_ASSERT(offsetof(GAMEPAD_s, input_direction_x) == 0x30, "GAMEPAD second direction component offset");
DECOMP_ASSERT(offsetof(GAMEPAD_s, input_angle) == 0x26, "GAMEPAD input angle offset");
DECOMP_ASSERT(offsetof(GAMEPAD_s, input_magnitude) == 0x28, "GAMEPAD input magnitude offset");
typedef void GIZACTIONFN(GIZFLOW_s *, FLOWBOX_s *, char **, i32);

struct GIZACTIONDEFN_s {
    const char *name;
    GIZACTIONFN *action_fn;
};
DECOMP_ASSERT(sizeof(GIZACTIONDEFN_s) == 8, "GIZACTIONDEFN_s ABI");

struct FLOWACTION_s {
    FLOWACTION_s *next;
    char **arguments;
    i32 argument_count;
    GIZACTIONDEFN_s *definition;
};
DECOMP_ASSERT(sizeof(FLOWACTION_s) == 0x10, "flow action ABI");

DECOMP_ASSERT(sizeof(FLOWBOXACTIONDATA_s) == 0x10, "FLOWBOXACTIONDATA size");
DECOMP_ASSERT(offsetof(FLOWBOXACTIONDATA_s, arguments) == 4, "FLOWBOXACTIONDATA arguments offset");
DECOMP_ASSERT(offsetof(FLOWBOXACTIONDATA_s, argument_count) == 8, "FLOWBOXACTIONDATA argument count offset");
DECOMP_ASSERT(offsetof(FLOWBOXACTIONDATA_s, definition) == 0xc, "FLOWBOXACTIONDATA definition offset");
DECOMP_ASSERT(offsetof(FLOWBOX_s, actions) == 0xc, "FLOWBOX actions offset");

// The AI message system: a fixed pool of 0x38-byte messages; the free list
// and the active list live in the header (ResetGizAIMessageSys fills the
// free list from the pool, CheckGizAIMessage moves nodes free -> active).
struct GIZAIMESSAGESYS_s {
    i32 count;                // 0x00 number of pooled messages
    GIZAIMESSAGE_s *messages; // 0x04 pool base (each 0x38 bytes)
    NULISTHDR free_list;      // 0x08
    NULISTHDR active_list;    // 0x10
};

enum GIZAIMESSAGE_FLAGS {
    GIZAIMESSAGE_FLAG_ADD_GIZMO = 0x01,
    GIZAIMESSAGE_FLAG_GIZMO_ADDED = 0x02,
};

struct GIZAIMESSAGE_s {
    NULISTLNK links;     // 0x00
    char name[0x20];     // 0x08
    float value;         // 0x28
    i8 output_values[8]; // 0x2c
    i8 output_count;     // 0x34
    undefined field_0x35;
    byte flags; // 0x36, GIZAIMESSAGE_FLAGS
    undefined field_0x37;
};
DECOMP_ASSERT(sizeof(GIZAIMESSAGE_s) == 0x38, "GIZAIMESSAGE_s ABI");
DECOMP_ASSERT(offsetof(GIZAIMESSAGE_s, output_values) == 0x2c, "GIZAIMESSAGE_s output values offset");
DECOMP_ASSERT(offsetof(GIZAIMESSAGE_s, output_count) == 0x34, "GIZAIMESSAGE_s output count offset");
DECOMP_ASSERT(offsetof(GIZAIMESSAGE_s, flags) == 0x36, "GIZAIMESSAGE_s flags offset");
// Defined by gizmos/traps/gizbombgen.h.
struct GIZFLOWPROGRESS_s {
    i32 valid;
    union {
        u32 active[16];
        u32 state_bit_0[16];
    };
    union {
        u32 triggered[16];
        u32 state_bit_8[16];
        u32 reversing[16];
    };
    union {
        u32 completed[16];
        u32 state_bit_1[16];
        u32 finished[16];
    };
    union {
        u32 latched[16];
        u32 state_bit_5[16];
        u32 waiting_for_children[16];
    };
    union {
        u32 output_state[16];
        u32 state_bit_10[16];
        u32 loop_pending[16];
    };
};
DECOMP_ASSERT(sizeof(GIZFLOWPROGRESS_s) == 0x144, "GIZFLOWPROGRESS ABI");
DECOMP_ASSERT(offsetof(GIZFLOWPROGRESS_s, triggered) == 0x44, "GIZFLOWPROGRESS triggered offset");
DECOMP_ASSERT(offsetof(GIZFLOWPROGRESS_s, completed) == 0x84, "GIZFLOWPROGRESS completed offset");
DECOMP_ASSERT(offsetof(GIZFLOWPROGRESS_s, latched) == 0xc4, "GIZFLOWPROGRESS latched offset");
DECOMP_ASSERT(offsetof(GIZFLOWPROGRESS_s, output_state) == 0x104, "GIZFLOWPROGRESS output offset");
struct GIZFLOW_s {
    GIZMOSYS_s *gizmo_sys;
    i32 flowbox_count;
    FLOWBOX_s *flowboxes;
    u8 field_0x0c;
    u8 pointers_need_reset;
    u8 field_0x0e[2];
};
DECOMP_ASSERT(sizeof(GIZFLOW_s) == 0x10, "GIZFLOW_s ABI");
DECOMP_ASSERT(offsetof(GIZFLOW_s, pointers_need_reset) == 0xd, "GIZFLOW reset flag offset");
struct GIZFORCESYS_s;
struct GIZMOBLOWUPTYPE_s {
    nuhspecial_s alternate_specials[4]; // 0x00
    union {
        nuhspecial_s animated_special; // primary model/animation shared by instances
        nuhspecial_s special;
    }; // 0x30
    nuhspecial_s decal_special;  // 0x3c, drawn for blown-up instances
    nuhspecial_s shadow_special; // 0x48, projected beneath active instances
    nuhspecial_s burst_special;  // 0x54, drawn for instances with the burst output set
    u8 field_0x60[0x78 - 0x60];
    f32 animation_time_scale; // 0x78
    union {
        u32 type_flags; // 0x7c
        struct {
            u8 type_flags_low;
            u8 animation_flags; // 0x7d, GIZMOBLOWUPTYPE_ANIMATION_FLAGS
            u8 type_flags_high[2];
        };
    };
    u32 effect_flags; // 0x80
    f32 field_0x84;
    f32 animation_start_frame; // 0x88
    f32 animation_end_frame;   // 0x8c
    f32 animation_base_frame;  // 0x90
    f32 field_0x94;
    f32 field_0x98;
    f32 field_0x9c;
    i16 particle_types[9];             // 0xa0
    u16 instance_count;                // 0xb2, number of consecutive instances using this type
    GAMEANTINODEDATA_s anti_node_data; // 0xb4
    char name[0x20];
    u8 field_0xf8;
    u8 field_0xf9;
    u8 field_0xfa;
    u8 field_0xfb;
    u8 animation_runtime_flags; // 0xfc, cleared by the late-update pass
    u8 field_0xfd[3];
};
DECOMP_ASSERT(sizeof(GIZMOBLOWUPTYPE_s) == 0x100, "GIZMOBLOWUPTYPE_s ABI");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, animated_special) == 0x30, "GIZMOBLOWUPTYPE_s animated special offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, decal_special) == 0x3c, "GIZMOBLOWUPTYPE_s decal special offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, shadow_special) == 0x48, "GIZMOBLOWUPTYPE_s shadow special offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, burst_special) == 0x54, "GIZMOBLOWUPTYPE_s burst special offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, animation_flags) == 0x7d, "GIZMOBLOWUPTYPE_s animation flags offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, animation_base_frame) == 0x90,
              "GIZMOBLOWUPTYPE_s animation base frame offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, instance_count) == 0xb2, "GIZMOBLOWUPTYPE_s instance count offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, name) == 0xd8, "GIZMOBLOWUPTYPE_s name offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUPTYPE_s, animation_runtime_flags) == 0xfc,
              "GIZMOBLOWUPTYPE_s animation runtime flags offset");
enum GIZMOPICKUP_TYPE_FLAGS : u8 {
    GIZMOPICKUP_TYPE_DRAW_TUMBLING = 0x01,
    GIZMOPICKUP_TYPE_DRAW_Y_ROTATION = 0x02,
    GIZMOPICKUP_TYPE_MINIKIT_DETECTOR = 0x04,
    GIZMOPICKUP_TYPE_RED_BRICK_DETECTOR = 0x08,
    GIZMOPICKUP_TYPE_COLLISION_FILTER = 0x10,
    GIZMOPICKUP_TYPE_CHALLENGE_MODE_FILTER = 0x20,
    GIZMOPICKUP_TYPE_FLAG_40 = 0x40,
};

enum GIZMOPICKUP_CONFIG_FLAGS : u8 {
    GIZMOPICKUP_CONFIG_REQUIRES_ACTIVATION = 0x01,
    GIZMOPICKUP_CONFIG_REGISTER_GIZMO = 0x02,
    GIZMOPICKUP_CONFIG_DISABLE_SHADOW_MAP = 0x08,
};

enum GIZMOPICKUP_STATE_FLAGS : u8 {
    GIZMOPICKUP_STATE_ACTIVE = 0x01,
    GIZMOPICKUP_STATE_ENABLED = 0x02,
    GIZMOPICKUP_STATE_VISIBLE = 0x04,
    GIZMOPICKUP_STATE_COLLECTED = 0x08,
    GIZMOPICKUP_STATE_DRAWN = 0x10,
    GIZMOPICKUP_STATE_DRAW_VISIBLE = 0x20,
    GIZMOPICKUP_STATE_ALTERNATE_TYPE = 0x40,
    GIZMOPICKUP_STATE_ACTIVATED = 0x80,
};

struct GIZMOPICKUP_s {
    char name[8];     // 0x00
    NUVEC position;   // 0x08
    char type_code;   // 0x14, e.g. 's', 'g', 'm'
    u8 config_flags;  // 0x15, GIZMOPICKUP_CONFIG_FLAGS
    u8 model_variant; // 0x16
    union {
        u8 state_flags; // GIZMOPICKUP_STATE_FLAGS
        u8 runtime_flags;
        struct {
            u8 state_active : 1;
            u8 state_enabled : 1;
            u8 state_visible : 1;
            u8 other_state_flags : 5;
        };
        struct {
            u8 activation_state_padding : 7;
            u8 state_activated : 1;
        };
    }; // 0x17
    union {
        u8 activation_group;
        u8 type_id;
    }; // 0x18
    u8 field_0x19[3];
    f32 floor_height;      // 0x1c
    u16 shadow_x_rotation; // 0x20
    u16 shadow_z_rotation; // 0x22
    union {
        i8 room_index;
        u8 collected;
    }; // 0x24
    u8 type_index;              // 0x25, resolved from type_code during reset
    u16 draw_rotation;          // 0x26
    f32 remaining_visible_time; // 0x28
};
DECOMP_ASSERT(sizeof(GIZMOPICKUP_s) == 0x2c, "GIZMOPICKUP_s ABI");
DECOMP_ASSERT(offsetof(GIZMOPICKUP_s, position) == 0x08, "GIZMOPICKUP position offset");
DECOMP_ASSERT(offsetof(GIZMOPICKUP_s, state_flags) == 0x17, "GIZMOPICKUP state flags offset");
DECOMP_ASSERT(offsetof(GIZMOPICKUP_s, floor_height) == 0x1c, "GIZMOPICKUP floor height offset");
DECOMP_ASSERT(offsetof(GIZMOPICKUP_s, type_index) == 0x25, "GIZMOPICKUP type index offset");

typedef void (*GIZMOPICKUPUPDATEFN)(WORLDINFO_s *, GIZMOPICKUP_s *);
typedef void (*GIZMOPICKUPCOLLECTFN)(WORLDINFO_s *, GIZMOPICKUP_s *, i32, GameObject_s *, i32);

struct GIZMO_PICKUP_TYPE {
    char *name;                // 0x00
    char *activation_sfx_name; // 0x04
    char *collection_sfx_name; // 0x08
    char type_code;            // 0x0c
    u8 random_model_count;     // 0x0d
    u8 flags;                  // 0x0e, GIZMOPICKUP_TYPE_FLAGS
    u8 field_0x0f;
    u16 first_model_id; // 0x10
    u16 score;
    f32 shadow_radius_x;             // 0x14
    f32 shadow_radius_z;             // 0x18
    f32 draw_distance;               // 0x1c
    f32 bob_height;                  // 0x20
    GIZMOPICKUPUPDATEFN update_fn;   // 0x24
    GIZMOPICKUPCOLLECTFN collect_fn; // 0x28
    i16 debris_id;                   // 0x2c
    i16 overlay_model_id;            // 0x2e
    f32 shadow_extent_x;             // 0x30, derived during reset
    f32 shadow_extent_z;             // 0x34, derived during reset
};
DECOMP_ASSERT(sizeof(GIZMO_PICKUP_TYPE) == 0x38, "GIZMO_PICKUP_TYPE ABI");
DECOMP_ASSERT(offsetof(GIZMO_PICKUP_TYPE, type_code) == 0x0c, "GIZMO_PICKUP_TYPE code offset");
DECOMP_ASSERT(offsetof(GIZMO_PICKUP_TYPE, score) == 0x12, "GIZMO_PICKUP_TYPE score offset");
DECOMP_ASSERT(offsetof(GIZMO_PICKUP_TYPE, update_fn) == 0x24, "GIZMO_PICKUP_TYPE update offset");

struct GIZMOPICKUPSYS_s {
    GIZMO_PICKUP_TYPE *types;
    u8 *random_coin_types;
    u16 type_count;
    i8 minikit_type;
    union {
        i8 gizmo_type_id;
        i8 field_0x0b;
    };
    i8 alternate_type;
    i8 field_0x0d;
    i16 field_0x0e;
};
DECOMP_ASSERT(sizeof(GIZMOPICKUPSYS_s) == 0x10, "GIZMOPICKUP type system ABI");

struct GIZMOPICKUPRUNTIMESYS_s {
    GIZMOPICKUP_s *pickups;
    GIZMOPICKUP_s *temporary_pickups;
    i32 pickup_count;
    i32 field_0x0c;
    i32 total_score;
    f32 draw_distance;
    f32 pickup_scale;
};
DECOMP_ASSERT(sizeof(GIZMOPICKUPRUNTIMESYS_s) == 0x1c, "GIZMOPICKUP runtime system ABI");

extern GIZMO_PICKUP_TYPE GizmoPickupType[10];
extern GIZMOPICKUPSYS_s GizmoPickupSys_Game;
i32 GetRandomCoinType();

struct SPECIALMINIKIT_s {
    nuhspecial_s special;
    void *inst_anim;
    void *anim_data;
    f32 end_frame;
    char special_name[16];
    char pickup_name[8];
    GIZMO_s *special_gizmo;
    GIZMO_s *pickup_gizmo;
    u8 flags;
};
DECOMP_ASSERT(sizeof(SPECIALMINIKIT_s) == 0x3c, "SPECIALMINIKIT_s size");
struct SPECIALMINIKITSYS_s {
    SPECIALMINIKIT_s *items;
    i32 count;
};
DECOMP_ASSERT(sizeof(SPECIALMINIKITSYS_s) == 0x8, "SPECIALMINIKITSYS_s size");

struct FADER_s {
    nuhspecial_s special;
    u8 pad_0x0c[0xc];
};
DECOMP_ASSERT(sizeof(FADER_s) == 0x18, "FADER_s size");

struct PULSE_s {
    nuhspecial_s special;
    f32 on_time;
    f32 off_time;
    f32 timer;
    f32 start_wait;
    u16 active;
    u16 disabled;
    char gizmo_name[16];
    GIZMO_s *gizmo;
};
DECOMP_ASSERT(sizeof(PULSE_s) == 0x34, "PULSE_s size");

struct GIZMOSYS_s;
struct GIZMO_s;
struct GIZOBSTACLESYS_s;
struct GIZSPECIAL_s;
enum GIZSPINNER_FLAGS : u8 {
    GIZSPINNER_FLAG_VALID = 0x01,
    GIZSPINNER_FLAG_HIDE_ARM = 0x02,
    GIZSPINNER_FLAG_HIDE_BASE = 0x04,
};

enum GIZSPINNER_STATE_FLAGS : u32 {
    GIZSPINNER_STATE_RESET = 0x0010,
    GIZSPINNER_STATE_SHADOW_PLATFORM = 0x0800,
};

struct GIZSPINNER_s {
    NUMTX matrix;                    // 0x000
    char name[0x10];                 // 0x040
    nuhspecial_s special;            // 0x050
    NUVEC position;                  // 0x05c
    GAMEANIMSET_s *anim_set;         // 0x068
    GAMEANIMOBJ_s *primary_anim_obj; // 0x06c
    union {
        u8 field_0x070[8];
        struct {
            f32 field_70;
            u32 field_74;
        };
    };
    f32 animation_speed; // 0x078
    u16 rotation;        // 0x07c
    u16 target_rotation;
    u16 previous_rotation;
    u16 initial_rotation; // 0x082
    u32 state_flags;      // 0x084, GIZSPINNER_STATE_FLAGS
    u8 field_0x088;
    i8 room_index; // 0x089
    u8 state;
    u8 type;
    u16 field_0x08c;
    u8 field_0x08e[2];
    f32 field_0x090;
    f32 field_0x094;
    f32 field_0x098;
    f32 field_0x09c;
    f32 ground_height; // 0x0a0
    u8 output_count;   // 0x0a4
    u8 field_0x0a5[7];
    u8 flags; // 0x0ac, GIZSPINNER_FLAGS
    u8 field_0x0ad[0x2d8 - 0x0ad];
    f32 field_0x2d8;
    f32 animation_points[10]; // 0x2dc
};
DECOMP_ASSERT(sizeof(GIZSPINNER_s) == 0x304, "GIZSPINNER_s ABI");
DECOMP_ASSERT(offsetof(GIZSPINNER_s, name) == 0x40, "GIZSPINNER name offset");
DECOMP_ASSERT(offsetof(GIZSPINNER_s, position) == 0x5c, "GIZSPINNER position offset");
DECOMP_ASSERT(offsetof(GIZSPINNER_s, field_70) == 0x70, "GIZSPINNER trigger progress offset");
DECOMP_ASSERT(offsetof(GIZSPINNER_s, anim_set) == 0x68, "GIZSPINNER anim-set offset");
DECOMP_ASSERT(offsetof(GIZSPINNER_s, flags) == 0xac, "GIZSPINNER flags offset");
DECOMP_ASSERT(offsetof(GIZSPINNER_s, animation_points) == 0x2dc, "GIZSPINNER animation-points offset");
struct GIZTURRETSYS_s;
struct GRABBER_s {
    union {
        NUMTX matrix; // 0x00, source scene-special matrix
        struct {
            u8 reserved_000[0x30];
            NUVEC position; // 0x30, translation from the source matrix
            u8 reserved_03c[0x40 - 0x3c];
        };
    };
    NUMTX grab_matrix; // 0x40
    u8 reserved_080[0x440 - 0x80];
    NUMTX carried_matrix; // 0x440
    union {
        NUVEC initial_position; // 0x480
        struct {
            f32 field_0x480;
            f32 field_0x484;
            f32 field_0x488;
        };
    };
    union {
        NUVEC grab_position;
        struct {
            f32 field_0x48c;
            f32 field_0x490;
            f32 field_0x494;
        };
    };
    NUVEC velocity;
    NUVEC target_velocity;
    CHARACTERMODEL_s *character_model; // 0x4b0, APICharacterLoaded result
    ANIMPACKET_s animation;
    nuhspecial_s special;        // 0x4fc, source scene special
    nuhspecial_s shadow_special; // 0x508
    GameObject_s *victim;        // 0x514
    GIZOBSTACLE_s *action_switch;
    GIZOBSTACLE_s *direction_switches[4];
    GIZMOBLOWUP_s *carried_blowup; // 0x52c
    f32 scale;                     // 0x530
    f32 speed;                     // 0x534
    f32 radius;                    // 0x538
    f32 stuck_timer;
    f32 shadow_height;
    f32 carried_height_offset; // 0x544
    u16 shadow_z_angle;
    u16 shadow_x_angle;
    f32 floor_height;
    f32 animation_timer;
    f32 platform_contact_timer; // 0x554
    u8 state;
    u8 flags_559;
    u16 yaw;
    u16 target_yaw;
    i16 character_id; // 0x55e
    i16 platform_id;  // 0x560
    u8 move_xy;       // 0x562
    u8 invert_x;      // 0x563
    u8 reserved_564[0x568 - 0x564];
};
DECOMP_ASSERT(sizeof(GRABBER_s) == 0x568, "GRABBER allocation size");
DECOMP_ASSERT(offsetof(GRABBER_s, matrix) == 0x00, "GRABBER source matrix offset");
DECOMP_ASSERT(offsetof(GRABBER_s, position) == 0x30, "GRABBER source position offset");
DECOMP_ASSERT(offsetof(GRABBER_s, grab_matrix) == 0x40, "GRABBER grab matrix offset");
DECOMP_ASSERT(offsetof(GRABBER_s, initial_position) == 0x480, "GRABBER initial position offset");
DECOMP_ASSERT(offsetof(GRABBER_s, character_model) == 0x4b0, "GRABBER character model offset");
DECOMP_ASSERT(offsetof(GRABBER_s, special) == 0x4fc, "GRABBER special offset");
DECOMP_ASSERT(offsetof(GRABBER_s, shadow_special) == 0x508, "GRABBER shadow special offset");
DECOMP_ASSERT(offsetof(GRABBER_s, flags_559) == 0x559, "GRABBER model flags offset");
DECOMP_ASSERT(offsetof(GRABBER_s, platform_contact_timer) == 0x554, "GRABBER platform contact timer offset");
DECOMP_ASSERT(offsetof(GRABBER_s, platform_id) == 0x560, "GRABBER platform ID offset");
DECOMP_ASSERT(offsetof(GRABBER_s, field_0x484) == 0x484, "GRABBER saved field offset");
DECOMP_ASSERT(offsetof(GRABBER_s, field_0x48c) == 0x48c, "GRABBER saved field offset");
DECOMP_ASSERT(offsetof(GRABBER_s, field_0x494) == 0x494, "GRABBER saved field offset");
DECOMP_ASSERT(offsetof(GRABBER_s, victim) == 0x514, "GRABBER victim offset");
DECOMP_ASSERT(offsetof(GRABBER_s, animation) == 0x4b4, "GRABBER animation offset");
DECOMP_ASSERT(offsetof(GRABBER_s, carried_blowup) == 0x52c, "GRABBER carried blowup offset");
struct GRAPPLE_s;
enum HINT_FLAGS {
    HINT_COMPLETE_ON_DISPLAY = 0x01,
    HINT_SHOP_PURCHASED = 0x02,
    HINT_DIRECT_DISPLAY = 0x08,
};
struct HINT_s {
    i16 control_mode_ids[2]; // 0x00
    u8 flags;                // 0x04
    u8 pad_0x05[3];
    i32 shop_price;       // 0x08: consumed by InitShop
    f32 display_duration; // 0x0c; zero has no timed cancellation
    f32 repeat_delay;     // 0x10
    union {
        i32 (*availability_fn)(HINT_s *);
        u8 pad_0x14[4];
    };
    void (*on_display)(HINT_s *); // 0x18
    u8 completion_flags[4];       // 0x1c
    f32 field_0x20;
};
DECOMP_ASSERT(sizeof(HINT_s) == 0x24, "HINT_s size");
DECOMP_ASSERT(offsetof(HINT_s, shop_price) == 0x08, "HINT_s shop price offset");
DECOMP_ASSERT(offsetof(HINT_s, on_display) == 0x18, "HINT_s display callback offset");
struct HINTSYS_s {
    void (*update_fn)(HINT_s *, i32); // 0x00
    HINT_s *hints;                    // 0x04
    HINT_s *active_hint;              // 0x08
    u8 pad_0x0c[0x0d - 0x0c];
    u8 state; // 0x0d
    u8 pad_0x0e[0x10 - 0x0e];
    u32 *save_bits;     // 0x10
    i32 save_bit_count; // 0x14
    union {
        i32 current_hint;
        f32 display_elapsed; // 0x18
    };
    union {
        i32 field_0x1c;
        f32 alpha;
    };
};
DECOMP_ASSERT(sizeof(HINTSYS_s) == 0x20, "HINTSYS_s size");
struct HINTUIBUTTON_s {
    u8 pad_0x00[0x40];
    f32 *field_0x40;
    f32 field_0x44;
    f32 field_0x48;
    f32 field_0x4c;
    f32 field_0x50;
    f32 field_0x54;
    u8 pad_0x58[0x78 - 0x58];
    void *field_0x78;
    HINT_s *field_0x7c;
    u8 pad_0x80[0x84 - 0x80];
    f32 *field_0x84;
    f32 field_0x88;
    f32 field_0x8c;
    f32 field_0x90;
    f32 field_0x94;
    f32 field_0x98;
    u8 pad_0x9c[0xa0 - 0x9c];
    u8 field_0xa0;
    f32 field_0xa4;
};
DECOMP_ASSERT(sizeof(HINTUIBUTTON_s) == 0xa8, "HINTUIBUTTON_s size");
DECOMP_ASSERT(offsetof(HINTUIBUTTON_s, field_0x7c) == 0x7c, "HINTUIBUTTON_s pending hint offset");
struct HOTHBATTLE_MELEE_WAVE_s {
    i32 field_0x0;
    i16 character_id;
    u8 reserved_06[2];
    GameObject_s *creatures[4];
    u8 field_0x18;
    u8 field_0x19;
    u8 reserved_1a;
    char name[0xd];
};
DECOMP_ASSERT(sizeof(HOTHBATTLE_MELEE_WAVE_s) == 0x28, "HOTHBATTLE_MELEE_WAVE_s ABI");

struct HOTHBATTLE_MELEE_s {
    u8 field_0x0;
    u8 field_0x1;
    u8 field_0x2;
    u8 field_0x3;
    i8 field_0x4;
    u8 field_0x5[3];
    HOTHBATTLE_MELEE_WAVE_s waves[4];
    u8 reserved_0xa8[0x1c];
    u8 creature_count;
    u8 reserved_0xc5[3];
};
DECOMP_ASSERT(sizeof(HOTHBATTLE_MELEE_s) == 0xc8, "HOTHBATTLE_MELEE_s ABI");
DECOMP_ASSERT(offsetof(HOTHBATTLE_MELEE_s, waves[0].creatures) == 0x10, "HOTHBATTLE melee creature offset");
DECOMP_ASSERT(offsetof(HOTHBATTLE_MELEE_s, creature_count) == 0xc4, "HOTHBATTLE melee count offset");
struct HashRedirect;
struct LANGUAGEDATA {
    i32 language;   // 0x00
    i32 unknown_04; // 0x04 (Game_LanguageList entries: {1,0},{2,0},{4,0},{5,0},{3,0},{8,0},{-1,0})
};
struct LEVELDATADISPLAY;
struct LEVELDATA_s;
struct LEVELOBJECT;
struct LEVELSCRIPTPROCESS_s;
struct LEVELSPLINE {
    struct nugscn_s **scene;
    const char *name;
    u16 min_points;
    u16 max_points;
    i16 level;
    i16 area;
};

DECOMP_ASSERT(sizeof(LEVELSPLINE) == 0x10, "LEVELSPLINE ABI");

struct LoadedUniqueShaderRecord;
struct MENU_s;
struct MISSIONSAVE_s;
struct MemoryBuffer;
struct NARROWSOCKEXCEPTION {
    char *level_name;
    i16 level_index;
    i16 sock_index;
    f32 start_distance;
    f32 end_distance;
};
DECOMP_ASSERT(sizeof(NARROWSOCKEXCEPTION) == 0x10, "Narrow socket exception ABI");
struct NUGCUTCHAR_s;
struct NUGCUTSCENE_s;
struct NetAddress;
struct NetMessage;
struct NetPeer;
struct OPTIONSSAVE_s;
struct PARTDEBENTRY_s {
    i32 type_id;
    char name[16];
};
struct PARTDEBSYS_s {
    i32 named_count;
    i32 capacity;
    PARTDEBENTRY_s *entries;
};
struct PLATSKININFO {
    NUMTX *matrix;
    void *skin_data;
    void *matrix_data;
    f32 scale;
    void *terrain_data;
    i16 terrain_group;
    union {
        u8 reserved_16[2];
        i16 cache_slot;
    };
    i16 flags;
    i16 mirrored;
};
DECOMP_ASSERT(sizeof(PLATSKININFO) == 0x1c, "PLATSKININFO ABI");
DECOMP_ASSERT(offsetof(PLATSKININFO, terrain_data) == 0x10, "PLATSKININFO terrain data offset");
DECOMP_ASSERT(offsetof(PLATSKININFO, mirrored) == 0x1a, "PLATSKININFO mirrored offset");
struct PLATSKINMEMINFO {
    i32 last_used;
    i16 skin_index;
    u8 reserved_06[2];
};
DECOMP_ASSERT(sizeof(PLATSKINMEMINFO) == 8, "PLATSKINMEMINFO ABI");
struct PLAYERITEMTYPE_s {};
struct PLAYERITEM_s {
    u8 *type;
    u8 ammunition;
    u8 reserved[3];
};
DECOMP_ASSERT(sizeof(PLAYERITEM_s) == 8, "PLAYERITEM size");

// Per-player runtime state embedded at GameObject_s + 0x6b4.  The packet is
// reset independently of the rest of the object by ResetPlayerPacket and is
// also shared by the local-player context helpers.
struct PLAYERPACKET_s {
    CHARACTER_SHADOW_s character_shadows[5]; // 0x000
    u8 pad_064[0x0e8 - 0x064];
    u16 context_animation; // 0x0e8
    u8 pad_0ea[0x0ee - 0x0ea];
    u8 context_mode; // 0x0ee
    u8 pad_0ef[0x0f1 - 0x0ef];
    i8 build_context; // 0x0f1, -1 when no gameplay context owns the player
    u8 pad_0f2[0x0f4 - 0x0f2];
    u8 action_movement_state;   // 0x0f4
    u8 action_movement_variant; // 0x0f5
    u8 pad_0f6[0x0fc - 0x0f6];
    f32 context_animation_time; // 0x0fc
    i8 context_target;          // 0x100
    u8 context_state_flags;     // 0x101
    u8 pad_102[0x5d4 - 0x102];
    NUVEC reset_up_direction; // 0x5d4
    GAMEPAD_s *gamepad;       // 0x5e0
    u8 pad_5e4[0x604 - 0x5e4];
    void *field_0x604;
    u8 pad_608[0x60c - 0x608];
    GameObject_s *linked_object; // 0x60c
    u8 pad_610[0x614 - 0x610];
    u32 field_0x614;
    u32 field_0x618;
    u32 field_0x61c;
    u32 field_0x620;
    u32 field_0x624;
    u32 field_0x628;
    u32 field_0x62c;
    u32 field_0x630;
    u8 pad_634[0x648 - 0x634];
    u32 field_0x648;
    u8 pad_64c[0x654 - 0x64c];
    union {
        i32 force_glow_mode;
        void *force_glow_object;
    };
    union {
        i32 force_glow_state;
        void *force_glow_candidate;
    };
    u8 pad_65c[0x660 - 0x65c];
    u32 input_state; // 0x660
    u32 field_0x664;
    u32 field_0x668;
    u8 pad_66c[0x670 - 0x66c];
    u32 field_0x670;
    u32 field_0x674;
    u32 field_0x678;
    f32 context_blend;    // 0x67c
    f32 context_distance; // 0x680
    u8 pad_684[0x688 - 0x684];
    u32 field_0x688;
    f32 delayed_turn_timer; // 0x68c
    u32 field_0x690;
    u32 field_0x694;
    u8 pad_698[0x69c - 0x698];
    u32 field_0x69c;
    u32 field_0x6a0;
    u32 field_0x6a4;
    u8 pad_6a8[0x6c8 - 0x6a8];
    u32 field_0x6c8;
    f32 force_glow_x;         // 0x6cc
    f32 force_glow_y;         // 0x6d0
    f32 force_glow_intensity; // 0x6d4
    u8 pad_6d8[0x6dc - 0x6d8];
    f32 force_glow_z; // 0x6dc
    u32 field_0x6e0;
    u8 pad_6e4[0x6e8 - 0x6e4];
    u32 field_0x6e8;
    f32 ground_height; // 0x6ec, 2000000.0f when invalid
    u8 pad_6f0[0x6f8 - 0x6f0];
    u32 field_0x6f8;
    u32 field_0x6fc;
    u8 pad_700[0x710 - 0x700];
    u32 field_0x710;
    u8 pad_714[0x718 - 0x714];
    u32 field_0x718;
    u32 field_0x71c;
    u32 field_0x720;
    u32 field_0x724;
    u32 field_0x728;
    u8 pad_72c[0x730 - 0x72c];
    u32 field_0x730;
    u32 field_0x734;
    u8 pad_738[0x73c - 0x738];
    u32 field_0x73c;
    u8 pad_740[0x75c - 0x740];
    i16 movement_angle_0;     // 0x75c
    i16 movement_angle_1;     // 0x75e
    i16 movement_angle_2;     // 0x760
    i16 movement_angle_3;     // 0x762
    i16 movement_lean_angle;  // 0x764
    i16 secondary_lean_angle; // 0x766
    i16 tertiary_lean_angle;  // 0x768
    i16 force_glow_index;     // 0x76a
    u8 movement_flags;        // 0x76c, GAMEOBJECT_E20_FLAGS
    u8 secondary_flags;       // 0x76d
    u8 animation_flags;       // 0x76e, GAMEOBJECT_E22_FLAGS
    u8 pad_76f;
    u8 render_flags; // 0x770
    u8 field_0x771;
    u8 pad_772;
    u8 random_state[4]; // 0x773
    u8 pad_777[0x77d - 0x777];
    u8 field_0x77d;
    u8 field_0x77e;
    u8 pad_77f[0x78c - 0x77f];
    u8 random_context;
    u8 pad_78d;
    i8 surface_type; // 0x78e, -1 when no surface is selected
    u8 pad_78f[0x794 - 0x78f];
    u32 field_0x794;
};
DECOMP_ASSERT(sizeof(PLAYERPACKET_s) == 0x798, "PLAYERPACKET_s ABI");
DECOMP_ASSERT(offsetof(PLAYERPACKET_s, gamepad) == 0x5e0, "PLAYERPACKET gamepad offset");
DECOMP_ASSERT(offsetof(PLAYERPACKET_s, force_glow_mode) == 0x654, "PLAYERPACKET force glow mode offset");
DECOMP_ASSERT(offsetof(PLAYERPACKET_s, force_glow_x) == 0x6cc, "PLAYERPACKET force glow position offset");
DECOMP_ASSERT(offsetof(PLAYERPACKET_s, movement_lean_angle) == 0x764, "PLAYERPACKET movement lean offset");
DECOMP_ASSERT(offsetof(PLAYERPACKET_s, animation_flags) == 0x76e, "PLAYERPACKET animation flags offset");
struct PLUG_s;
struct PULSESYS_s {
    PULSE_s *pulses;
    u16 pulse_count;
    i16 sfx_turn_on;
    i16 sfx_turn_off;
    i16 sfx_on_loop;
    i16 sfx_off_loop;
    i16 sfx_hit_player;
    f32 collide_radius;
    f32 hit_direction_line;
    f32 hit_direction_radius_origin;
    i16 debris_hit_player;
    u8 radial_hit_direction;
    u8 pad_0x1f;
};
DECOMP_ASSERT(sizeof(PULSESYS_s) == 0x20, "PULSESYS_s size");
DECOMP_ASSERT(offsetof(PULSESYS_s, pulse_count) == 0x4, "PULSESYS pulse count offset");
DECOMP_ASSERT(offsetof(PULSESYS_s, collide_radius) == 0x10, "PULSESYS collision radius offset");
DECOMP_ASSERT(offsetof(PULSESYS_s, radial_hit_direction) == 0x1e, "PULSESYS hit direction mode offset");
struct PartHeader;
struct PropertyMenuList {};
struct REGISTERSTATUSPACKET_s {
    STATUSPACKET_LSW_s *lsw_packet;
    i32 (*init_callback)(WORLDINFO_s *, STATUSPACKET_s *);
    i32 (*finish_callback)(WORLDINFO_s *, STATUSPACKET_s *, i32);
    void (*reset_callback)(STATUSPACKET_s *);
    void (*draw_background_callback)(STATUSPACKET_s *);
    f32 stage_delay;
};
DECOMP_ASSERT(sizeof(REGISTERSTATUSPACKET_s) == 0x18, "REGISTERSTATUSPACKET_s size");
struct RGBA {
    union {
        u32 value;
        struct {
            u8 r, g, b, a;
        };
    };
};
DECOMP_ASSERT(sizeof(RGBA) == 4, "RGBA value ABI");
struct SCENESPECIALPROGRESS_s {
    u32 frame : 11;
    u32 visible : 1;
    u32 playing : 1;
    u32 waiting : 1;
    u32 repeating : 1;
    u32 reserved : 17;
    f32 tfactor;
};
struct SCENEPROGRESS_s {
    SCENESPECIALPROGRESS_s specials[1280];
};
DECOMP_ASSERT(sizeof(SCENESPECIALPROGRESS_s) == 8, "scene special progress size");
DECOMP_ASSERT(offsetof(SCENESPECIALPROGRESS_s, tfactor) == 4, "scene special progress time factor");
DECOMP_ASSERT(sizeof(SCENEPROGRESS_s) == 0x2800, "scene progress size");
enum SHADERSEMANTIC_enum : i32 {};
struct SHARD_s;
struct SOCKPOSITION_s;

enum STATUS_FLAGS {
    STATUS_FLAG_DRAW_BACKDROP = 0x04,
};
// Status / achievements screen packet (332 bytes; fields used by NuMain:
// model ids at 0x9c/0x9e, per-player bytes at 0xa4/0xa5, flags at 0xb1/0xb2).
struct STATUSPACKET_s {
    STATUSPACKET_LSW_s *lsw_packet;                               // 0x00
    i32 (*init_callback)(WORLDINFO_s *, STATUSPACKET_s *);        // 0x04
    i32 (*finish_callback)(WORLDINFO_s *, STATUSPACKET_s *, i32); // 0x08
    void (*reset_callback)(STATUSPACKET_s *);                     // 0x0c
    void (*draw_background_callback)(STATUSPACKET_s *);           // 0x10
    AREADATA_s *area;                                             // 0x14
    EPISODEDATA *episode;                                         // 0x18
    u32 *score;                                                   // 0x1c
    struct MISSIONDATA_s *mission;                                // 0x20
    f32 player0_rumble_amount;                                    // 0x24
    f32 player0_rumble_time;                                      // 0x28
    f32 player0_rumble_duration;                                  // 0x2c
    f32 player0_buzz_amount;                                      // 0x30
    u8 player0_rumble_priority;                                   // 0x34
    undefined field_0x35[0x38 - 0x35];
    f32 player1_rumble_amount;   // 0x38
    f32 player1_rumble_time;     // 0x3c
    f32 player1_rumble_duration; // 0x40
    f32 player1_buzz_amount;     // 0x44
    u8 player1_rumble_priority;  // 0x48
    u8 field_0x49[3];
    f32 area_time;          // 0x4c
    f32 collected_score;    // 0x50
    f32 true_hero_target;   // 0x54
    f32 true_hero_percent;  // 0x58
    f32 previous_best_time; // 0x5c
    f32 new_best_time;      // 0x60
    f32 elapsed_time;       // 0x64
    f32 field_0x68;
    u32 previous_best_score; // 0x6c
    u32 new_best_score;      // 0x70
    u32 original_score;      // 0x74
    u32 reward_score;        // 0x78
    u32 time_reward_score;   // 0x7c
    u32 final_reward_score;  // 0x80
    u32 coins_remaining[2];  // 0x84
    u32 coins_collected[2];  // 0x8c
    i32 new_minikits;        // 0x94
    u32 field_0x98;
    u16 player0_model; // 0x9c
    u16 player1_model; // 0x9e
    i16 area_id;       // 0xa0
    i8 episode_id;     // 0xa2
    u8 field_0xa3;
    u8 player0_active;    // 0xa4
    u8 player1_active;    // 0xa5
    u8 challenge_state;   // 0xa6
    u8 mission_state;     // 0xa7
    f32 superstory_time;  // 0xa8
    u32 superstory_score; // 0xac
    u8 field_0xb0;
    u8 mode_flags;            // 0xb1
    u8 status_flags;          // 0xb2
    u8 minikit_count;         // 0xb3
    u8 minikit_max;           // 0xb4
    u8 stage_count;           // 0xb5
    i8 current_gold_brick;    // 0xb6
    u8 save_state;            // 0xb7
    u8 prompt_choice;         // 0xb8
    u8 newly_completed;       // 0xb9
    u8 previous_completion;   // 0xba
    u8 previous_gold_bricks;  // 0xbb
    u8 displayed_gold_bricks; // 0xbc
    u8 field_0xbd;
    i8 field_0xbe;
    u8 field_0xbf[7];
    i8 stage_types[0xee - 0xc6]; // 0xc6
    u8 gold_brick_enabled[40];   // 0xee
    i16 next_area;               // 0x116
    i8 chapter;                  // 0x118
    u8 field_0x119[3];
    struct STATUS_STAGE_s *previous_stage_2; // 0x11c
    struct STATUS_STAGE_s *previous_stage;   // 0x120
    struct STATUS_STAGE_s *stage;            // 0x124
    struct STATUS_STAGE_s *next_stage;       // 0x128
    undefined field_0x12c[0x14c - 0x12c];
};
DECOMP_ASSERT(sizeof(STATUSPACKET_s) == 0x14c, "STATUSPACKET_s size");
struct STATUSPACKET_LSW_s {
    i32 field_0x00;
};
struct STATUS_STAGE_s {
    void (*draw_callback)(STATUS_STAGE_s *, STATUSPACKET_s *, i32);   // 0x00
    void (*update_callback)(STATUS_STAGE_s *, STATUSPACKET_s *, f32); // 0x04
    void (*skip_callback)(STATUS_STAGE_s *, STATUSPACKET_s *);        // 0x08
    i32 type;                                                         // 0x0c
    u16 field_0x10;
    u8 field_0x12;
    u8 pad_0x13;
    i32 field_0x14;
    f32 field_0x18;
    f32 field_0x1c;
};
DECOMP_ASSERT(sizeof(STATUS_STAGE_s) == 0x20, "STATUS_STAGE_s size");
enum SUIT_STORE_FLAGS : u8 {
    SUIT_STORE_FLAG_SHADOW_MODE = 0x01,
    SUIT_STORE_FLAG_EXTRA_MOVEMENT_ANIMATIONS = 0x40,
};
struct SUIT_s {
    char *base_character_name;
    char *suit_character_name;
    i16 *text_id;
    union {
        u16 flags;
        struct {
            u8 store_flag;
            u8 initially_available;
        };
        struct {
            u16 : 1;
            u16 can_glide : 1;
            u16 : 4;
            u16 can_magnet_climb : 1;
            u16 : 9;
        };
    };
    u8 group;
    char letter;
    u32 character_flags;
    i16 character_id;
    u8 index;
    u8 field_0x17;
};
DECOMP_ASSERT(sizeof(SUIT_s) == 0x18, "SUIT_s size");
DECOMP_ASSERT(offsetof(SUIT_s, store_flag) == 0x0c, "SUIT_s store flag offset");
DECOMP_ASSERT(offsetof(SUIT_s, group) == 0xe, "SUIT_s group offset");
struct SUPERCOUNTER {
    u8 pad_0x00[0x1e2];
    u16 reset_value; // 0x1e2
    u8 pad_0x1e4[0x1e7 - 0x1e4];
    u8 processed_flags; // 0x1e7
};
DECOMP_ASSERT(sizeof(SUPERCOUNTER) == 0x1e8, "SUPERCOUNTER size");
struct SUPERCOUNTERPICKUP {};
struct ShaderObjectKey;
struct SoundTable {
    u16 bits[100];
    const char **names;
};
DECOMP_ASSERT(sizeof(SoundTable) == 0xcc, "SoundTable size");
struct TECHNO_s;
struct TERRPICKUPSET;

// Terrain files contain ordinary collision groups followed by auxiliary
// chunks.  Values 0 and 1 both describe groups; 2 links a spatial-index block
// into TERRSET, while 3 identifies the crash-data block.
enum TERRAIN_CHUNK_TYPE {
    TERRAIN_CHUNK_GROUP_PRIMARY = 0,
    TERRAIN_CHUNK_GROUP_SECONDARY = 1,
    TERRAIN_CHUNK_SPATIAL_INDEX = 2,
    TERRAIN_CHUNK_CRASH_DATA = 3,
};

struct TERRAIN_SHAPE_BATCH {
    i16 marker;
    i16 shape_count;
    f32 min_x;
    f32 max_x;
    f32 min_z;
    f32 max_z;
};
DECOMP_ASSERT(sizeof(TERRAIN_SHAPE_BATCH) == 0x14, "TERRAIN_SHAPE_BATCH ABI");

enum TERRAIN_SHAPE_FLAGS : u8 {
    TERRAIN_SHAPE_FLAG_MATERIAL_HIGH_BIT = 0x40,
    TERRAIN_SHAPE_FLAG_MATERIAL_ALIAS = 0x80,
};

enum TERRAIN_SHAPE_NORMAL_FLAGS : u8 {
    TERRAIN_SHAPE_NORMAL_FLAG_FIRST_STEEP = 0x01,
    TERRAIN_SHAPE_NORMAL_FLAG_SECOND_STEEP = 0x02,
};

enum TERRAIN_SURFACE_CLASS : u8 {
    TERRAIN_SURFACE_CLASS_MASK = 0x7c,
    TERRAIN_SURFACE_CLASS_WALL_OVERRIDE = 0x04,
};

enum TERRAIN_HIT_TYPE : u8 {
    TERRAIN_HIT_TYPE_NONE = 0x00,
    TERRAIN_HIT_TYPE_FACE = 0x01,
    TERRAIN_HIT_TYPE_CYLINDER = 0x02,
    TERRAIN_HIT_TYPE_VERTEX = 0x03,
    TERRAIN_HIT_TYPE_SPHERE = 0x04,
};

enum TERRAIN_HIT_TYPE_ENCODING : u8 {
    TERRAIN_HIT_TYPE_CLASS_MASK = 0x0f,
    TERRAIN_HIT_TYPE_SECOND_NORMAL = 0x10,
};

enum TERRAIN_QUERY_FLAGS : u8 {
    TERRAIN_QUERY_FLAG_PREVIOUS_NORMAL = 0x01,
};

enum TERRAIN_HIT_EDGE : u8 {
    TERRAIN_HIT_EDGE_NONE = 0,
    TERRAIN_HIT_EDGE_0_1 = 1,
    TERRAIN_HIT_EDGE_1_2 = 2,
    TERRAIN_HIT_EDGE_2_0 = 3,
    TERRAIN_HIT_EDGE_1_3 = 4,
    TERRAIN_HIT_EDGE_3_2 = 5,
};

struct tertype {
    f32 min_x;
    f32 max_x;
    f32 min_y;
    f32 max_y;
    f32 min_z;
    f32 max_z;
    NUVEC vectors[4];
    NUVEC normals[2];
    u8 material[2];
    u8 flags;
    u8 normal_flags;
};
typedef tertype TERRAIN_SHAPE;
DECOMP_ASSERT(sizeof(tertype) == 0x64, "tertype ABI");
DECOMP_ASSERT(offsetof(tertype, vectors) == 0x18, "tertype vectors offset");
DECOMP_ASSERT(offsetof(tertype, normals) == 0x48, "tertype normals offset");
DECOMP_ASSERT(offsetof(tertype, material) == 0x60, "tertype material offset");

struct TERRAIN_SURFACE_s {
    f32 movement_scale;
    u32 flags;
    u32 field_0x08;
};
DECOMP_ASSERT(sizeof(TERRAIN_SURFACE_s) == 0xc, "TERRAIN_SURFACE_s size");

struct TERRAIN_SPHERE {
    NUVEC position;
    f32 radius;
};
DECOMP_ASSERT(sizeof(TERRAIN_SPHERE) == 0x10, "TERRAIN_SPHERE ABI");

// Constraint used by NewTerrAxisFreedom while resolving a terrain side step.
// The offset is updated as contacts are resolved; angle and radius describe
// the horizontal axis and the extent allowed around it.
struct TERRAIN_AXIS_FREEDOM_SHAPE {
    NUVEC offset;
    f32 angle;
    f32 radius;
};
DECOMP_ASSERT(sizeof(TERRAIN_AXIS_FREEDOM_SHAPE) == 0x14, "TERRAIN_AXIS_FREEDOM_SHAPE ABI");
DECOMP_ASSERT(offsetof(TERRAIN_AXIS_FREEDOM_SHAPE, angle) == 0x0c, "TERRAIN_AXIS_FREEDOM_SHAPE angle offset");
DECOMP_ASSERT(offsetof(TERRAIN_AXIS_FREEDOM_SHAPE, radius) == 0x10, "TERRAIN_AXIS_FREEDOM_SHAPE radius offset");

// Scratch record shared by the terrain broad phase and collision response.
// ScanTerrain serializes groups of shape pointers into scan_list_storage.
struct TerrainQuery_s {
    u8 flags;
    u8 pad_0x001[3];
    NUVEC start_position;
    NUVEC start_movement;
    NUVEC position;
    NUVEC movement;
    u8 scan_result;
    u8 shape_adjusted;
    i16 object_index;
    f32 radius;
    f32 horizontal_movement_length;
    u8 *hit_flags;
    f32 movement_pitch;
    f32 movement_yaw;
    f32 movement_length;
    f32 collision_radius;
    f32 collision_radius_sq;
    f32 inverse_collision_radius;
    f32 object_scale;
    f32 object_scale_sq;
    f32 inverse_object_scale;
    f32 inverse_object_scale_sq;
    i16 terrain_group_index;
    u8 pad_0x06e[2];
    NUVEC local_start;
    NUVEC local_end;
    u8 hit_type;
    TERRAIN_HIT_EDGE hit_edge;
    i16 scan_group_index;
    void *scan_list;
    TERRAIN_SHAPE *surface;
    tertype *working_surface;
    f32 hit_time;
    f32 unclamped_hit_time;
    f32 separation_epsilon;
    f32 compare_epsilon;
    NUVEC movement_normal;
    NUVEC impact_normal;
    NUVEC platform_position;
    NUVEC platform_normal;
    NUVEC previous_movement_normal;
    u8 pad_0x0e4[0x18];
    NUVEC transformed_vertices[4];
    u8 pad_0x12c[0x1c];
    u8 scan_list_storage[0x800];
};
DECOMP_ASSERT(offsetof(TerrainQuery_s, position) == 0x1c, "TerrainQuery_s position offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, movement) == 0x28, "TerrainQuery_s movement offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, horizontal_movement_length) == 0x3c,
              "TerrainQuery_s horizontal movement length offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, movement_pitch) == 0x44, "TerrainQuery_s movement pitch offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, movement_yaw) == 0x48, "TerrainQuery_s movement yaw offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, movement_length) == 0x4c, "TerrainQuery_s movement length offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, terrain_group_index) == 0x6c, "TerrainQuery_s terrain group index offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, local_start) == 0x70, "TerrainQuery_s local start offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, local_end) == 0x7c, "TerrainQuery_s local end offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, hit_edge) == 0x89, "TerrainQuery_s hit edge offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, scan_group_index) == 0x8a, "TerrainQuery_s scan group offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, scan_list) == 0x8c, "TerrainQuery_s scan list offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, working_surface) == 0x94, "TerrainQuery_s working surface offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, hit_time) == 0x98, "TerrainQuery_s hit time offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, unclamped_hit_time) == 0x9c, "TerrainQuery_s unclamped hit time offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, platform_position) == 0xc0, "TerrainQuery_s platform position offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, platform_normal) == 0xcc, "TerrainQuery_s platform normal offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, previous_movement_normal) == 0xd8,
              "TerrainQuery_s previous movement normal offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, transformed_vertices) == 0xfc, "TerrainQuery_s transformed vertices offset");
DECOMP_ASSERT(offsetof(TerrainQuery_s, scan_list_storage) == 0x148, "TerrainQuery_s scan storage offset");
DECOMP_ASSERT(sizeof(TerrainQuery_s) == 0x948, "TerrainQuery_s ABI");

struct TERRAIN_GROUP {
    NUVEC origin;     // 0x00
    void *data;       // 0x0c, points into the loaded .ter image
    NUVEC bounds_min; // 0x10, world-space except for secondary groups
    NUVEC bounds_max; // 0x1c, world-space except for secondary groups
    union {
        u32 platform_flags; // 0x28
        u32 field_0x28;
    };
    i32 chunk_type; // 0x2c, TERRAIN_CHUNK_GROUP_PRIMARY/SECONDARY
    union {
        i16 scene_index; // 0x30 on input; platform index after TerrainInitEx
        i16 field_0x30;
    };
    u16 field_0x32;
    f32 radius; // 0x34, largest shape-vector magnitude in the group
};
DECOMP_ASSERT(sizeof(TERRAIN_GROUP) == 0x38, "TERRAIN_GROUP ABI");
DECOMP_ASSERT(offsetof(TERRAIN_GROUP, bounds_min) == 0x10, "TERRAIN_GROUP bounds_min offset");
DECOMP_ASSERT(offsetof(TERRAIN_GROUP, platform_flags) == 0x28, "TERRAIN_GROUP platform_flags offset");
DECOMP_ASSERT(offsetof(TERRAIN_GROUP, scene_index) == 0x30, "TERRAIN_GROUP scene_index offset");
DECOMP_ASSERT(offsetof(TERRAIN_GROUP, radius) == 0x34, "TERRAIN_GROUP radius offset");

enum TERRAIN_PLATFORM_FLAGS : u8 {
    TERRAIN_PLATFORM_FLAG_ROTATING = 0x01,
    TERRAIN_PLATFORM_FLAG_COLLIDED = 0x02,
    TERRAIN_PLATFORM_FLAG_DISPLAY_LIST_BACKED = 0x04,
};

struct TERRAIN_PLATFORM {
    NUMTX previous_matrix;
    void *scene_object; // 0x40
    i32 field_0x44;
    i16 terrain_group_index; // 0x48
    u16 scene_object_index;  // 0x4a
    u8 flags;                // 0x4c, TERRAIN_PLATFORM_FLAGS
    u8 pad_0x4d[3];
    i16 bounce_frames;
    u8 pad_0x52[2];
    f32 bounce_impulse;
    f32 bounce_offset;
    f32 bounce_velocity;
    f32 bounce_spring;
    f32 bounce_damping;
    void *scene_transform; // 0x68
};
DECOMP_ASSERT(sizeof(TERRAIN_PLATFORM) == 0x6c, "TERRAIN_PLATFORM ABI");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, previous_matrix) == 0, "TERRAIN_PLATFORM previous matrix offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, bounce_frames) == 0x50, "TERRAIN_PLATFORM bounce frames offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, bounce_impulse) == 0x54, "TERRAIN_PLATFORM bounce impulse offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, bounce_damping) == 0x64, "TERRAIN_PLATFORM bounce damping offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, scene_object) == 0x40, "TERRAIN_PLATFORM scene_object offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, terrain_group_index) == 0x48, "TERRAIN_PLATFORM terrain_group_index offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, flags) == 0x4c, "TERRAIN_PLATFORM flags offset");
DECOMP_ASSERT(offsetof(TERRAIN_PLATFORM, scene_transform) == 0x68, "TERRAIN_PLATFORM scene_transform offset");

struct TERRAIN_SPATIAL_NODE;
struct TERRPICKUPSET {
    TERRAIN_GROUP *groups;
    void *shape_data;
    i32 group_count;
    i16 group_for_type[128];
    TERRAIN_PLATFORM platforms[32];
    TERRAIN_SPATIAL_NODE *spatial_nodes;
    u8 reserved_e90[0xa490 - 0xe90];
};
DECOMP_ASSERT(sizeof(TERRPICKUPSET) == 0xa490, "TERRPICKUPSET ABI");
DECOMP_ASSERT(offsetof(TERRPICKUPSET, group_for_type) == 0x0c, "Pickup terrain type table offset");
DECOMP_ASSERT(offsetof(TERRPICKUPSET, platforms) == 0x10c, "Pickup terrain platforms offset");
DECOMP_ASSERT(offsetof(TERRPICKUPSET, spatial_nodes) == 0xe8c, "Pickup terrain spatial list offset");

struct TERRAIN_SPATIAL_NODE {
    u16 point_count;
    u16 field_0x02;
    NUVEC points[1]; // variable-length array stored in the terrain file
};
DECOMP_ASSERT(offsetof(TERRAIN_SPATIAL_NODE, points) == 4, "TERRAIN_SPATIAL_NODE points offset");

struct TERRAIN_WALL_POINT {
    NUVEC position;
    u8 material[4];
};
DECOMP_ASSERT(sizeof(TERRAIN_WALL_POINT) == 0x10, "TERRAIN_WALL_POINT ABI");

enum TERRAIN_TRACK_FLAGS : u16 {
    TERRAIN_TRACK_FLAG_NONE = 0x00,
    TERRAIN_TRACK_FLAG_CONNECTED = 0x01,
};

enum TERRAIN_TRACK_CONTACT_STATE : i16 {
    TERRAIN_TRACK_CONTACT_NONE = 0,
    TERRAIN_TRACK_CONTACT_ACTIVE = 4,
};

struct TERRAIN_TRACK_SLOT {
    void *id;
    i16 platform_index;
    u16 flags;                  // TERRAIN_TRACK_FLAGS
    i16 platform_contact_state; // TERRAIN_TRACK_CONTACT_STATE
    i16 wall_contact_state;     // TERRAIN_TRACK_CONTACT_STATE
    NUVEC impact_normal;
};
DECOMP_ASSERT(sizeof(TERRAIN_TRACK_SLOT) == 0x18, "TERRAIN_TRACK_SLOT ABI");
DECOMP_ASSERT(offsetof(TERRAIN_TRACK_SLOT, platform_index) == 0x04, "TERRAIN_TRACK_SLOT platform index offset");
DECOMP_ASSERT(offsetof(TERRAIN_TRACK_SLOT, flags) == 0x06, "TERRAIN_TRACK_SLOT flags offset");
DECOMP_ASSERT(offsetof(TERRAIN_TRACK_SLOT, platform_contact_state) == 0x08,
              "TERRAIN_TRACK_SLOT platform contact state offset");
DECOMP_ASSERT(offsetof(TERRAIN_TRACK_SLOT, wall_contact_state) == 0x0a, "TERRAIN_TRACK_SLOT wall contact state offset");
DECOMP_ASSERT(offsetof(TERRAIN_TRACK_SLOT, impact_normal) == 0x0c, "TERRAIN_TRACK_SLOT impact normal offset");

struct TERRAIN_CELL {
    i16 first_group;
    u16 group_count;
    f32 min_x;
    f32 min_z;
    f32 max_x;
    f32 max_z;
};
DECOMP_ASSERT(sizeof(TERRAIN_CELL) == 0x14, "TERRAIN_CELL ABI");

struct TERRAIN_INDEX_LEVEL {
    union {
        u16 entry_count;
        i16 cache_age;
    };
    u8 pad_0x002[2];
    f32 center_x;
    f32 center_z;
    union {
        // The packed list alternates a pair of i16s (count, group index)
        // with count terrain-shape pointers, and ends with a zero header.
        u8 scan_list[0x800];
        struct {
            void *entries;
            u8 pad_0x010[0x7fc];
        };
    };
};
DECOMP_ASSERT(sizeof(TERRAIN_INDEX_LEVEL) == 0x80c, "TERRAIN_INDEX_LEVEL ABI");
DECOMP_ASSERT(offsetof(TERRAIN_INDEX_LEVEL, center_x) == 4, "terrain cache X offset");
DECOMP_ASSERT(offsetof(TERRAIN_INDEX_LEVEL, center_z) == 8, "terrain cache Z offset");
DECOMP_ASSERT(offsetof(TERRAIN_INDEX_LEVEL, scan_list) == 0xc, "terrain cache list offset");

enum {
    TERRAIN_GRID_WIDTH = 7,
    TERRAIN_GRID_CELL_COUNT = TERRAIN_GRID_WIDTH * TERRAIN_GRID_WIDTH,
    TERRAIN_CELL_RECORD_COUNT = 257,
    TERRAIN_PLATFORM_CELL = 256,
    TERRAIN_INDEX_LEVEL_COUNT = 17,
    TERRAIN_TRACK_SLOT_COUNT = 64,
};

// Fixed header allocated by TerrainInitEx.  The arrays immediately following
// it are sized from the level configuration at runtime.
struct TERRSET {
    TERRAIN_GROUP *groups; // 0x000
    u8 pad_0x004[0x60];
    void *field_0x064;
    TERRAIN_PLATFORM *platforms;                                 // 0x068
    i16 active_platform_groups[96];                              // 0x06c
    i32 active_platform_count;                                   // 0x12c
    NUVEC platform_scan_min;                                     // 0x130
    NUVEC platform_scan_max;                                     // 0x13c
    TERRAIN_SPATIAL_NODE *spatial_nodes;                         // 0x148
    i16 group_count;                                             // 0x14c
    u16 file_version;                                            // 0x14e
    i32 group_index_count;                                       // 0x150
    i32 removed_platform_count;                                  // 0x154
    i16 removed_platforms[32];                                   // 0x158
    TERRAIN_TRACK_SLOT track_slots[TERRAIN_TRACK_SLOT_COUNT];    // 0x198
    TERRAIN_CELL cells[TERRAIN_CELL_RECORD_COUNT];               // 0x798
    i32 minimum_height;                                          // 0x1bac
    i32 used_cell_count;                                         // 0x1bb0
    i16 *group_indices;                                          // 0x1bb4
    TERRAIN_INDEX_LEVEL index_levels[TERRAIN_INDEX_LEVEL_COUNT]; // 0x1bb8
    i32 max_group_indices;                                       // 0xa484
    i32 max_groups;                                              // 0xa488
    i32 max_platforms;                                           // 0xa48c
};
DECOMP_ASSERT(sizeof(TERRSET) == 0xa490, "TERRSET ABI");
DECOMP_ASSERT(offsetof(TERRSET, platforms) == 0x68, "TERRSET platforms offset");
DECOMP_ASSERT(offsetof(TERRSET, active_platform_groups) == 0x6c, "TERRSET active platform groups offset");
DECOMP_ASSERT(offsetof(TERRSET, active_platform_count) == 0x12c, "TERRSET active platform count offset");
DECOMP_ASSERT(offsetof(TERRSET, platform_scan_min) == 0x130, "TERRSET platform scan minimum offset");
DECOMP_ASSERT(offsetof(TERRSET, removed_platform_count) == 0x154, "TERRSET removed platform count offset");
DECOMP_ASSERT(offsetof(TERRSET, spatial_nodes) == 0x148, "TERRSET spatial_nodes offset");
DECOMP_ASSERT(offsetof(TERRSET, group_count) == 0x14c, "TERRSET group_count offset");
DECOMP_ASSERT(offsetof(TERRSET, file_version) == 0x14e, "TERRSET file_version offset");
DECOMP_ASSERT(offsetof(TERRSET, track_slots) == 0x198, "TERRSET track_slots offset");
DECOMP_ASSERT(offsetof(TERRSET, cells) == 0x798, "TERRSET cells offset");
DECOMP_ASSERT(offsetof(TERRSET, minimum_height) == 0x1bac, "TERRSET minimum_height offset");
DECOMP_ASSERT(offsetof(TERRSET, group_indices) == 0x1bb4, "TERRSET group_indices offset");
DECOMP_ASSERT(offsetof(TERRSET, index_levels) == 0x1bb8, "TERRSET index_levels offset");
DECOMP_ASSERT(offsetof(TERRSET, max_group_indices) == 0xa484, "TERRSET max_group_indices offset");
struct TEXTCRAWL_s {
    i16 *heading_text;
    i16 *vehicle_bonus_text;
    i16 *character_bonus_text;
    i32 paragraph_count;
};
DECOMP_ASSERT(sizeof(TEXTCRAWL_s) == 0x10, "TEXTCRAWL_s size");
struct TEXTENTRY {
    i16 *text_id;
    i16 value;
    i16 pad;
};
struct TRAFFICANIM_s {
    nuhspecial_s special;    // 0x000
    void *animation;         // 0x00c
    f32 end_frame;           // 0x010
    f32 tfactor;             // 0x014
    f32 frame_interval;      // 0x018
    f32 random_interval;     // 0x01c
    f32 room_frame_interval; // 0x020
    f32 y_offset;            // 0x024
    i8 vehicle_indices[16];  // 0x028
    u8 rooms[256];           // 0x038
    i8 vehicle_count;        // 0x138
    u8 disabled;             // 0x139
    u8 reserved_13a[2];
    f32 next_spawn_time; // 0x13c
};
DECOMP_ASSERT(sizeof(TRAFFICANIM_s) == 0x140, "traffic animation size");

struct TRAFFICANIMINSTANCE_s {
    NULISTLNK link;           // 0x00
    TRAFFICANIM_s *animation; // 0x08
    f32 frame;                // 0x0c
    i8 vehicle_index;         // 0x10
    u8 reserved_11[3];
};
DECOMP_ASSERT(sizeof(TRAFFICANIMINSTANCE_s) == 0x14, "traffic animation instance size");

struct TRAFFICANIMSYS_s {
    TRAFFICANIM_s animations[64];         // 0x0000
    nuhspecial_s vehicles[16];            // 0x5000
    TRAFFICANIMINSTANCE_s instances[500]; // 0x50c0
    NULISTHDR free_instances;             // 0x77d0
    NULISTHDR active_instances;           // 0x77d8
    i8 animation_count;                   // 0x77e0
    i8 vehicle_count;                     // 0x77e1
    u8 reserved_77e2[2];
};
DECOMP_ASSERT(sizeof(TRAFFICANIMSYS_s) == 0x77e4, "traffic animation system size");
struct TUBE_s;
struct ThingLevelData {};
struct ThingRemoveData {};
struct VirtualStackAllocator;
struct VuMtx;
struct _vum_s {};
struct _vuv_s;
struct bgprocinfo_s;
struct bitrate_manager_state {};
struct codebook {};
struct debris_colour_key_s {
    f32 time;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
};
struct debris_float_key_s {
    f32 time;
    f32 value;
};
struct debinftype {
    char name[16];               // 0x000
    u8 category;                 // 0x010
    u8 page;                     // 0x011
    u8 cutscene_only;            // 0x012
    u8 disabled;                 // 0x013
    i16 max_particles;           // 0x014
    i16 frequency;               // 0x016
    f32 emission_period;         // 0x018
    f32 emission_period_random;  // 0x01c
    f32 emission_pause;          // 0x020
    f32 emission_pause_random;   // 0x024
    f32 start_offset_random;     // 0x028
    u8 generator_type;           // 0x02c
    u8 momentum_adjustment_type; // 0x02d
    u8 particle_type;            // 0x02e
    u8 status;                   // 0x02f
    u8 fields_030[8];            // 0x030
    f32 clip_extent;             // 0x038
    f32 sound_range;
    f32 sound_range_override;
    f32 field_044;
    f32 field_048;
    f32 field_04c;
    f32 field_050;
    f32 field_054;
    f32 field_058;
    f32 field_05c;
    f32 field_060;
    NUVEC emitter_velocity; // 0x064
    u8 fields_070[0x30];
    f32 field_0a0;
    f32 particle_lifetime; // 0x0a4
    i16 field_0a8;
    u8 field_0aa;
    u8 field_0ab;
    f32 field_0ac;
    f32 field_0b0;
    f32 field_0b4;
    f32 field_0b8;
    f32 field_0bc;
    debris_colour_key_s colour_keys[8]; // 0x0c0
    debris_float_key_s alpha_keys[8];   // 0x100
    f32 field_140;
    f32 field_144;
    f32 field_148;
    f32 field_14c;
    debris_float_key_s width_keys[8];  // 0x150
    debris_float_key_s height_keys[8]; // 0x190
    u8 fields_1d0[8];
    debris_float_key_s rotation_keys[8]; // 0x1d8
    u8 fields_218[0x80];
    f32 texture_u0;          // 0x298
    f32 texture_v0;          // 0x29c
    f32 texture_u1;          // 0x2a0
    f32 texture_v1;          // 0x2a4
    PartHeader *native_data; // 0x2a8 (target)
    f32 last_render_time;    // 0x2ac (target)
    u8 fields_2b0[0x40];     // 0x2b0 (target)
    u8 process_spheres;      // 0x2f0 (target)
    i8 time_group;           // 0x2f1
    u8 field_2f2;
    u8 use_explicit_clip_box; // 0x2f3
    f32 thinning;             // 0x2f4
    union {
        u8 fields_2f8[0xd8];
        NUVEC repeat_box; // 0x2f8
    };
    i16 particle_keys[8]; // 0x3d0
    i32 sound_data[12];   // 0x3e0
    i8 trail_count;       // 0x410
    u8 radial_segments;   // 0x411
    u8 camera_facing;     // 0x412
    u8 field_413;
    f32 trail_time;            // 0x414
    f32 scale_in_time;         // 0x418
    f32 radial_floor;          // 0x41c
    f32 scale;                 // 0x420
    i32 unscaled_effect_index; // 0x424
};
struct debscale_s {
    i32 unscaled_effect_index;
    i32 scaled_effect_index;
    f32 scale;
};
DECOMP_ASSERT(sizeof(debinftype) == 0x428, "debinftype size");

typedef uv1deb *(*DEBRISGENERATOR)(debkeydatatype_s *, debinftype *, f32);
typedef void (*DEBRISMOMENTUMADJUSTER)(debkeydatatype_s *, debinftype *, uv1deb *);

struct debris_process_sphere_s {
    NUVEC position;
    NUVEC momentum;
    f32 time;
};
DECOMP_ASSERT(sizeof(debris_process_sphere_s) == 0x1c, "debris process sphere size");
DECOMP_ASSERT(offsetof(debris_process_sphere_s, time) == 0x18, "debris sphere time offset");

// One interpolated particle frame in the original debris effect table.
// The table is embedded in PartHeader at 0x4c and contains 64 entries.
struct debris_particle_frame_s {
    NUVEC position;       // 0x00
    NUVEC texture_offset; // 0x0c
    NUVEC extent;         // 0x18
    u32 colour;           // 0x24
};
DECOMP_ASSERT(sizeof(debris_particle_frame_s) == 0x28, "debris particle frame size");

struct PartHeader {
    u8 fields_000[0x10];
    f32 gravity;          // 0x10
    f32 last_render_time; // 0x14
    u8 fields_018[0x10];
    f32 texture_u0; // 0x28
    f32 texture_v0; // 0x2c
    f32 texture_u1; // 0x30
    f32 texture_v1; // 0x34
    u8 fields_038[0x14];
    debris_particle_frame_s frames[64]; // 0x4c
};
DECOMP_ASSERT(offsetof(PartHeader, gravity) == 0x10, "PartHeader gravity offset");
DECOMP_ASSERT(offsetof(PartHeader, last_render_time) == 0x14, "PartHeader render time offset");
DECOMP_ASSERT(offsetof(PartHeader, texture_u0) == 0x28, "PartHeader texture coordinate offset");
DECOMP_ASSERT(offsetof(PartHeader, frames) == 0x4c, "PartHeader frame table offset");
DECOMP_ASSERT(sizeof(PartHeader) == 0xa4c, "PartHeader size");

struct dma_particle_s {
    NUVEC position;
    f32 start_time;
    NUVEC momentum;
    f32 inverse_lifetime;
};

struct dma_particle_chunk_s {
    u8 command;
    u8 fields_001[3];
    dma_particle_chunk_s *next;
    u8 fields_008[0x0c];
    u32 field_014;
    u32 field_018;
    u32 field_01c;
    dma_particle_s particles[32];
    u32 end_command;
};
DECOMP_ASSERT(sizeof(dma_particle_s) == 0x20, "dma_particle_s size");
DECOMP_ASSERT(offsetof(dma_particle_s, position) == 0x00, "dma particle position offset");
DECOMP_ASSERT(offsetof(dma_particle_s, start_time) == 0x0c, "dma particle start time offset");
DECOMP_ASSERT(offsetof(dma_particle_s, momentum) == 0x10, "dma particle momentum offset");
DECOMP_ASSERT(offsetof(dma_particle_s, inverse_lifetime) == 0x1c, "dma particle lifetime offset");
DECOMP_ASSERT(offsetof(dma_particle_chunk_s, next) == 0x04, "dma particle next offset");
DECOMP_ASSERT(offsetof(dma_particle_chunk_s, field_014) == 0x14, "dma particle header offset");
DECOMP_ASSERT(offsetof(dma_particle_chunk_s, particles) == 0x20, "dma particle data offset");
DECOMP_ASSERT(offsetof(dma_particle_chunk_s, end_command) == 0x420, "dma particle terminator offset");
DECOMP_ASSERT(sizeof(dma_particle_chunk_s) == 0x424, "dma_particle_chunk_s size");

struct debkeydatatype_s {
    union {
        u8 fields_000[0x40];
        NUMTX clip_matrix;
    };
    NUMTX effect_orientation;                  // 0x040
    NUMTX emitter_orientation;                 // 0x080
    NUMTX particle_orientation;                // 0x0c0
    dma_particle_chunk_s *particle_chunks[32]; // 0x100 (target)
    i16 allocated_chunk_count;                 // 0x180 (target)
    i16 effect_index;                          // 0x182
    u8 field_184;
    u8 allocation_locked;       // 0x185
    i16 particle_count;         // 0x186
    i16 controlled_chunk_count; // 0x188
    i16 field_18a;
    i16 previous_allocated_chunk_count; // 0x18c
    i16 previous_particle_count;        // 0x18e
    NUVEC position;                     // 0x190
    NUVEC emission_position;            // 0x19c
    NUVEC momentum;                     // 0x1a8
    NUVEC emitter_momentum;
    DEBRISGENERATOR generator;                // 0x1c0 (target)
    DEBRISMOMENTUMADJUSTER momentum_adjuster; // 0x1c4 (target)
    debkeydatatype_s *previous;               // 0x1c8 (target)
    debkeydatatype_s *next;                   // 0x1cc (target)
    i16 emitter_rotation_x;                   // 0x1d0 (target)
    i16 emitter_rotation_y;                   // 0x1d2
    i32 field_1d4;
    i16 field_1d8;
    u8 field_1da;
    u8 field_1db;
    f32 previous_emission_time; // 0x1dc
    f32 emission_time;          // 0x1e0
    f32 field_1e4;
    debris_process_sphere_s process_spheres[8]; // 0x1e8
    i16 field_2c8;
    i16 sphere_skip_count;
    f32 sphere_next_time;
    i32 trigger_first;
    i32 trigger_second;
    union {
        i32 trigger_third;
        f32 switch_variable;
    };
    i16 reflection_x;
    i16 reflection_y;
    f32 collision_plane;
    f32 reflection_scale;
    i16 collision_timers[4];
    union {
        i16 render_group;
        u8 fields_2f0[2];
    };
    i16 field_2f2;
    u8 field_2f4;
    u8 process_collision_sound;
    u8 field_2f6;
    u8 field_2f7;
    u8 timed_flags;
    u8 field_2f9;
    u8 field_2fa;
    u8 field_2fb;
    nugscn_s *gscene; // 0x2fc (target)
    union {
        f32 orientation_dirty; // zero invalidates the cached clip bounds
        f32 clip_radius;       // 0x300
    };
    NUVEC clip_min; // 0x304
    NUVEC clip_max; // 0x310
    f32 cutoff_distance;
    f32 last_update_time;
    f32 emission_epoch;
    i16 render_priority;
    i16 allocation_index;
    i32 field_32c;
};
DECOMP_ASSERT(sizeof(debkeydatatype_s) == 0x330, "debkeydatatype_s size");
DECOMP_ASSERT(offsetof(debkeydatatype_s, process_spheres) == 0x1e8, "debris sphere array offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, sphere_skip_count) == 0x2ca, "debris sphere skip offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, sphere_next_time) == 0x2cc, "debris sphere deadline offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, collision_plane) == 0x2e0, "debris collision plane offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, reflection_scale) == 0x2e4, "debris restitution offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, clip_matrix) == 0, "debris clip matrix offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, clip_radius) == 0x300, "debris clip radius offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, clip_min) == 0x304, "debris clip minimum offset");
DECOMP_ASSERT(offsetof(debkeydatatype_s, clip_max) == 0x310, "debris clip maximum offset");
struct debris_chunk_control_s {
    dma_particle_chunk_s *particle_chunk;
    f32 expiry_time;
    i32 active;
    debkeydatatype_s *owner;
    debris_chunk_control_s *next;
    i32 particle_index;
    f32 collision_time;
    i16 effect_index;
    i16 field_01e;
    i16 rotation_y;
    i16 field_022;
    f32 collision_plane;
    f32 restitution;
};
DECOMP_ASSERT(sizeof(debris_chunk_control_s) == 0x2c, "debris_chunk_control_s size");
struct drft_lookup {};
enum EDCAM_FREEDOM : u8 {
    EDCAM_FREEDOM_POSITION_X = 1 << 0,
    EDCAM_FREEDOM_POSITION_Y = 1 << 1,
    EDCAM_FREEDOM_POSITION_Z = 1 << 2,
    EDCAM_FREEDOM_PITCH = 1 << 3,
    EDCAM_FREEDOM_YAW = 1 << 4,
    EDCAM_FREEDOM_DISTANCE = 1 << 5,
    EDCAM_FREEDOM_INVERT_PAD_PITCH = 1 << 6,
};

struct edcam_s {
    NUVEC position;           // 0x00
    i32 pitch;                // 0x0c
    i32 yaw;                  // 0x10
    f32 distance;             // 0x14
    NUVEC offset;             // 0x18
    NUVEC snap_origin;        // 0x24
    NUVEC snap_step;          // 0x30
    i32 pitch_snap_step;      // 0x3c
    i32 yaw_snap_step;        // 0x40
    NUVEC snapped_position;   // 0x44
    i32 snapped_pitch;        // 0x50
    i32 snapped_yaw;          // 0x54
    NUVEC position_speed;     // 0x58
    i32 pad_pitch_speed;      // 0x64
    i32 pad_yaw_speed;        // 0x68
    f32 distance_speed;       // 0x6c
    f32 minimum_distance;     // 0x70
    f32 mouse_pitch_speed;    // 0x74
    f32 mouse_yaw_speed;      // 0x78
    f32 mouse_move_speed;     // 0x7c
    f32 auto_move_base;       // 0x80
    f32 auto_move_dist_scale; // 0x84
    f32 auto_zoom_base;       // 0x88
    f32 auto_zoom_dist_scale; // 0x8c
    union {
        u8 freedoms; // 0x90, EDCAM_FREEDOM bits
        struct {
            u8 allow_position_x : 1;
            u8 allow_position_y : 1;
            u8 allow_position_z : 1;
            u8 allow_pitch : 1;
            u8 allow_yaw : 1;
            u8 allow_distance : 1;
            u8 invert_pad_pitch : 1;
            u8 reserved_freedom : 1;
        };
    };
    u8 padding_91[3];
};
DECOMP_ASSERT(sizeof(edcam_s) == 0x94, "edcam_s size");
struct eduiitem_s;
struct eduimenu_s;
struct envelope_lookup {};
struct flightspline_s {};
struct instNUGCUTLOOKAT_s {};
struct instNUGCUTSCENE_s;
struct mdct_lookup {};
struct minisnowtrooper_s {};
struct minitrooperteam_s {};
struct nuanimbuff_s;
struct nucolour3_s;
struct nudisplaylistitem_s;
struct nudisplayscene_s;
struct nufile_device_s;
struct nufpar_s;
struct nufpcomjmp_s;
struct nuglobalrndrstate_s;
struct nugraph_s {
    i8 interpolation;
    i8 point_count;
    u8 pad_02[2];
    f32 x[8];
    f32 y[8];
    f32 x_scale;
    f32 y_scale;
    f32 x_extent;
    f32 y_extent;
    f32 x_offset;
    f32 y_offset;
};
DECOMP_ASSERT(sizeof(nugraph_s) == 0x5c, "nugraph_s ABI");
struct nugscn_s;
struct nugspline_s;
enum NUINSTANIM_FLAGS : u32 {
    NUINSTANIM_FLAG_PLAYING = 1 << 0,
    NUINSTANIM_FLAG_BACKWARDS = 1 << 1,
    NUINSTANIM_FLAG_WAITING = 1 << 2,
    NUINSTANIM_FLAG_REPEATING = 1 << 3,
    NUINSTANIM_FLAG_OSCILLATE = 1 << 4,
};

enum NUINSTANIM_PACKED_FIELDS : u16 {
    NUINSTANIM_END_FRAME_LOOKUP_MASK = 0xffc0,
    NUINSTANIM_END_FRAME_LOOKUP_SHIFT = 6,
};

enum NUINSTANIM_STATE_FIELDS : u32 {
    NUINSTANIM_STATE_INDEX_MASK = 0x003fc000,
    NUINSTANIM_STATE_INDEX_SHIFT = 14,
};

struct nuinstanim_s {
    NUMTX mtx;     // 0x00
    f32 tfactor;   // 0x40
    f32 tfirst;    // 0x44
    f32 tinterval; // 0x48
    f32 ltime;     // 0x4c
    union {
        NUINSTANIM_FLAGS flags; // 0x50
        struct {
            u16 playing : 1;
            u16 backwards : 1;
            u16 waiting : 1;
            u16 repeating : 1;
            u16 oscillate : 1;
            u16 unused_playback_bits : 11;
            u16 unused_end_frame_bits : 6;
            u16 end_frame_lookup_index : 10;
        };
        struct {
            u16 playback_flag_bits;
            u16 end_frame_lookup_bits;
        };
    };
    f32 fparam1;        // 0x54
    f32 prev_eval_time; // 0x58
    u16 anim_ix;        // 0x5c
    u16 instance_ix;    // 0x5e
};
DECOMP_ASSERT(sizeof(nuinstanim_s) == 0x60, "nuinstanim_s ABI");
DECOMP_ASSERT(offsetof(nuinstanim_s, end_frame_lookup_bits) == 0x52, "nuinstanim_s end-frame lookup bits offset");
struct numtl_s;
struct numtx_s;
struct nunativedebrisdata_s {
    u8 vertex_buffer_index; // 0x00
    u8 fields_001[3];
    i32 use_system_memory_vb; // 0x04
    i32 first_vertex;         // 0x08
    i32 vertex_count;         // 0x0c
    numtl_s *material;        // 0x10
};
struct debris_vertex_s {
    NUVEC position;
    u32 colour;
    f32 u;
    f32 v;
};
DECOMP_ASSERT(sizeof(debris_vertex_s) == 0x18, "debris vertex size");
DECOMP_ASSERT(offsetof(nunativedebrisdata_s, use_system_memory_vb) == 0x04, "native debris system-memory flag offset");
DECOMP_ASSERT(offsetof(nunativedebrisdata_s, first_vertex) == 0x08, "native debris first vertex offset");
DECOMP_ASSERT(offsetof(nunativedebrisdata_s, vertex_count) == 0x0c, "native debris vertex count offset");
DECOMP_ASSERT(offsetof(nunativedebrisdata_s, material) == 0x10, "native debris material offset");
DECOMP_ASSERT(sizeof(nunativedebrisdata_s) == 0x14, "native debris packet size");
struct nuoctreenode_s {};
struct nupad_s;
struct nushadermtldesc_s;
struct nusound_filename_info_s;
struct nutex_s;
struct nutexmanager_s;
struct particlechunkrendertype_s {
    dma_particle_chunk_s *particle_chunk;
    debinftype *effect;
    debkeydatatype_s *key;
    NUMTX effect_orientation;
    NUVEC position;
    particlechunkrendertype_s *previous;
    particlechunkrendertype_s *next;
    i16 render_priority;
    u8 fields_062[2];
};
DECOMP_ASSERT(sizeof(particlechunkrendertype_s) == 0x64, "particlechunkrendertype_s size");
struct edpp_particle_s {
    NUVEC position;
    i32 effect_index;
    i32 instance_id;
    u8 pad_0x14[0x34 - 0x14];
    i32 switch_type;
    i32 switch_id;
    f32 switch_variable;
    u8 pad_0x40[0x4c - 0x40];
    i16 render_group;
    u8 pad_0x4e[0x51 - 0x4e];
    i8 page;
    i8 detail_levels;
    u8 pad_0x53[0x58 - 0x53];
};
DECOMP_ASSERT(sizeof(edpp_particle_s) == 0x58, "edpp_particle_s ABI");
struct pushblock_s {
    union {
        f32 ground_offset; // 0x00
        f32 vertical_penetration;
    };
    union {
        f32 ground_height; // 0x04
        f32 support_height;
    };
    f32 snap_timer;                  // 0x08
    f32 settled_height;              // 0x0c
    i8 terrain_info[4];              // 0x10
    i8 extra_terrain_info[4];        // 0x14
    i32 previous_extra_terrain_info; // 0x18
    union {
        pushblock_s *supporting_block; // 0x1c
        pushblock_s *block_below;
    };
    pushblock_s *previous_supporting_block; // 0x20
    nuhspecial_s special;                   // 0x24
    GameObject_s *pushing_object;           // 0x30
    i32 platform_id;                        // 0x34
    u8 fields_038[8];
    f32 snap_distance;                     // 0x40
    char name[0x10];                       // 0x44
    NUVEC *snap_positions;                 // 0x54
    NUVEC *position;                       // 0x58
    nuhspecial_s end_position_specials[2]; // 0x5c
    NUVEC end_position_origins[2];         // 0x74
    NUVEC velocity;                        // 0x8c
    NUVEC target_velocity;                 // 0x98
    NUVEC snap_origin;                     // 0xa4
    NUVEC bounds_min;                      // 0xb0
    NUVEC bounds_max;                      // 0xbc
    union {
        struct {
            u8 runtime_flags_0c8;
            u8 runtime_flags_0c9;
            union {
                u8 flags_0ca;
                struct {
                    u8 fields_0ca_0_3 : 4;
                    u8 config_0ca_4 : 1;
                    u8 config_0ca_5 : 1;
                    u8 config_0ca_6 : 1;
                    u8 config_0ca_7 : 1;
                };
            };
            union {
                u8 flags_0cb;
                struct {
                    u8 fields_0cb_0_1 : 2;
                    u8 config_0cb_2 : 1;
                    u8 config_0cb_3 : 1;
                    u8 field_0cb_4 : 1;
                    u8 config_0cb_5 : 1;
                    u8 config_0cb_6 : 1;
                    u8 field_0cb_7 : 1;
                };
            };
        };
        u8 state_flags[4];
        u32 packed_state_flags;
    };
    union {
        u16 completion_flags; // 0xcc
        u16 field_0xcc;
    };
    union {
        struct {
            u8 end_position_count;
            u8 output_count;
        };
        u16 field_0xce;
    };
};
DECOMP_ASSERT(sizeof(pushblock_s) == 0xd0, "pushblock_s size");
DECOMP_ASSERT(offsetof(pushblock_s, special) == 0x24, "pushblock_s special offset");
DECOMP_ASSERT(offsetof(pushblock_s, name) == 0x44, "pushblock_s name offset");
DECOMP_ASSERT(offsetof(pushblock_s, snap_positions) == 0x54, "pushblock_s snap positions offset");
DECOMP_ASSERT(offsetof(pushblock_s, position) == 0x58, "pushblock_s position offset");
DECOMP_ASSERT(offsetof(pushblock_s, end_position_specials) == 0x5c, "pushblock_s end positions offset");
DECOMP_ASSERT(offsetof(pushblock_s, velocity) == 0x8c, "pushblock_s velocity offset");
DECOMP_ASSERT(offsetof(pushblock_s, completion_flags) == 0xcc, "pushblock_s completion flags offset");
struct RIPPLEEFFECT_s {
    RGBA start_color;
    RGBA end_color;
    f32 lifetime;
    f32 initial_size;
    f32 end_size;
    union {
        u8 reserved_14[0x10];
        char texture_name[16];
    };
    numtl_s *material;
};
DECOMP_ASSERT(sizeof(RIPPLEEFFECT_s) == 0x28, "Ripple effect ABI");
DECOMP_ASSERT(offsetof(RIPPLEEFFECT_s, material) == 0x24, "Ripple effect material offset");

struct ripple_node_s {
    union {
        u8 pad_0x00[0x78];
        struct {
            NUMTX matrix;
            NUVEC velocity;
            struct numtl_s *material;
            f32 size;
            f32 initial_size;
            f32 growth;
            RGBA color;
            RGBA start_color;
            RGBA end_color;
            f32 age;
            f32 lifetime;
            f32 delay;
            u16 flags;
            u16 reserved_76;
        };
    };
    ripple_node_s *next;
    ripple_node_s *previous;
};
DECOMP_ASSERT(sizeof(ripple_node_s) == 0x80, "ripple_node_s ABI");
DECOMP_ASSERT(offsetof(ripple_node_s, material) == 0x4c, "Ripple material offset");
DECOMP_ASSERT(offsetof(ripple_node_s, color) == 0x5c, "Ripple colour offset");
DECOMP_ASSERT(offsetof(ripple_node_s, age) == 0x68, "Ripple age offset");
DECOMP_ASSERT(offsetof(ripple_node_s, next) == 0x78, "Ripple next link offset");
struct ripple_set_s {
    union {
        u32 reset_state;
        struct {
            u16 count;
            union {
                u16 free_count;
                u16 active_count;
            };
        };
    };
    ripple_node_s *nodes;
    union {
        ripple_node_s *current;
        ripple_node_s *free_head;
    };
    union {
        void *field_0x0c;
        ripple_node_s *newest;
    };
    union {
        void *field_0x10;
        ripple_node_s *oldest;
    };
};
DECOMP_ASSERT(sizeof(ripple_set_s) == 0x14, "ripple_set_s ABI");
struct rtlset;
struct shopitem_s {
    char name[0x40];
    char special_name[0x20];
    i16 item_id;
    u8 type;
    u8 unlocked;
    i32 price;
    nuhspecial_s special;
};
DECOMP_ASSERT(sizeof(shopitem_s) == 0x74, "shopitem_s size");
DECOMP_ASSERT(offsetof(shopitem_s, item_id) == 0x60, "shopitem_s item-id offset");
DECOMP_ASSERT(offsetof(shopitem_s, special) == 0x68, "shopitem_s special offset");
struct specialsfx_s;
struct speedup_s {
    f32 distance;
    f32 speed;
};
struct starfighter_s {};
struct terrsitu_s {};
struct uv1deb {};
struct uv1debdata;
struct BaseEditor {
    u8 reserved_0x00[4];
    BaseEditor *next;
    BaseEditor *previous;
    i32 field_0x0c;

    void Initialise(variptr_u &, variptr_u &, i32);
    void ReadBuffer(void **, void *, i32);
    void WriteBeginBlock(i32, i32);
    void WriteEndBlock(i32);
    void WriteMetaData(i32, i32, i32, i32);
};
#include "legoapi/items/objects/basething.h"
struct CantPickupBombTimerAddon : MechAddon {
    CantPickupBombTimerAddon(MechObjectInterface &, float);
    bool OnProcess(MechAddon::ProcessStage, float) override;
    ~CantPickupBombTimerAddon() override;
    static void operator delete(void *allocation) {
        NU_FREE(allocation);
    }
    static HashedKey s_hashId;
    f32 remaining_time;
};
DECOMP_ASSERT(sizeof(CantPickupBombTimerAddon) == 0x1c, "CantPickupBombTimerAddon ABI");
DECOMP_ASSERT(offsetof(CantPickupBombTimerAddon, remaining_time) == 0x18, "Bomb pickup timer offset");

struct ClassEditor {
    u8 reserved_0x00[0x10];
    EdTool *first_tool;
    EdTool *last_tool;
    i32 tool_count;
    u8 reserved_0x1c[0x50 - 0x1c];
    VuVec snap_ray;
    u8 reserved_0x60[4];
    i32 snap_mode;

    ClassEditor();
    void AddMenuItems(eduimenu_s *);
    void ClearLevel(i32);
    void CreateObject();
    void CreateObject(ClassObject &);
    void CreateObject(EdClass *);
    void CreateObject(i32);
    void DestroySelectedObjects();
    void DestroySelectedObjectsNow();
    void DrawObjectSphere(ClassObject &, i32);
    void Editable(void *, EdClass *, i32);
    void Enter();
    void Exit();
    void FindNearestObject(VuVec &, ClassObject &, ClassObject &, i32);
    void FindNearestObject(VuVec &, ClassObject &, i32);
    void FindNearestObject(VuVec &, VuVec &, ClassObject &, ClassObject &, i32);
    void FindNearestObject(VuVec &, VuVec &, ClassObject &, i32);
    void Flush();
    void FocusSelected();
    void Initialise(variptr_u &, variptr_u &, i32);
    void InitialiseObject(ClassObject &);
    void IsSelectedClass(EdClass *);
    void IsSelectedObject(ClassObject &);
    void IsSelectedObject(void *, EdRef *);
    void IsUniqueName(char *);
    void MakeUniqueName(char const *, char *, i32);
    void PostLoadInitialisation(MemoryBuffer *, MemoryBuffer *);
    void PostSaveInitialisation();
    void PreLoadInitialisation(MemoryBuffer *, MemoryBuffer *);
    void PreSaveInitialisation();
    void Process(EdInputContext &);
    void ReadBlock(DATAPTR *);
    void RegisterTool(EdTool &);
    void Render();
    void SelectLED(i32);
    void SelectObject(ClassObject &, i32);
    void Serialise(EdStream &);
    void SetMode(i32);
    void SetViewMenuHilight(eduimenu_s *);
    void SnapPoint(VuVec &);
    void UpdateClassFilter(EdInputContext &);
    void UpdateLists(MemoryBuffer *, MemoryBuffer *);
    void UpdateSelectedObjects(EdInputContext &);
    void UpdateSnapRay(VuVec &);
    void ViewSelected();
    void WriteBlock(i32);
    void cbDestroyMenu(eduimenu_s *, eduimenu_s *);
    void cbDestroyObject(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassDeleteObject(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassExportMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassFileMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassImportMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassModeMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassNewMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassNewObject(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassRemoveDuplicates(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSelectClassMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSelectObject(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSelectObjectMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSetMode(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSetPinned(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSetSnap(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSetView(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassSnapMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassToolsMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdClassViewMenu(eduimenu_s *, eduiitem_s *, u32);
    void cbEdCopySelectedObject(EdInputContext &);
    void cbEdCreateClassNewObject(i32);
    void cbEdFilterLED(eduimenu_s *, eduiitem_s *, u32);
    void cbEdLevelDeselectAll(eduimenu_s *, eduiitem_s *, u32);
    void cbEdLevelSelectAll(eduimenu_s *, eduiitem_s *, u32);
    void cbEdPadSetManipulatorMode(eduimenu_s *, eduiitem_s *, u32);
    void cbFileSelected(eduimenu_s *, eduiitem_s *, u32);
};
struct ClassObject {
    EdClass *ed_class;
    void *object;
    EdRef *reference;

    void GetName(char *, i32);
    void Set(char *);
};
struct ClassObjectListEntry {
    ClassObjectListEntry *next;
    ClassObjectListEntry *previous;
    EdClass *ed_class;
    void *object;
    EdRef *reference;
};
struct ClassObjectList {
    ClassObjectListEntry *first;
    ClassObjectListEntry *last;
    i32 count;

    void GetAveragePosition(VuVec &);
    void GetAveragePosition(VuVec &, float &);
    bool IsInList(ClassObject);
    bool IsInList(EdClass *);
    bool IsInList(void *, EdRef *);
};
struct CursorTool {
    void Initialise(variptr_u &, variptr_u &, i32);
    void Process(EdInputContext &);
    void Render();
};
struct Fade : FadeBase {
    FADETYPE_VALUE GetFadeType() const override;
    void DrawFade();
    void Init(FADEINFO_s *);
    void InitFade();
    void UpdateFade();
};
struct FadeStill : FadeBase {
    FADETYPE_VALUE GetFadeType() const override;
    void DrawFade();
    void Init(FADEINFO_s *);
    void InitFade();
    void UpdateFade();
};
struct FadeStillWipe : FadeBase {
    FADETYPE_VALUE GetFadeType() const override;
    void DrawFade();
    void Init(FADEINFO_s *);
    void InitFade();
    void UpdateFade();
};
struct FadeSystem : FADEINFO_s {
    i32 AddFade(FadeBase *);
    void Draw();
    void Init();
    i32 SetFade(FADETYPE const &, u32);
    void SetStage(char);
    void Update();
};
struct FadeWipe : FadeBase {
    FADETYPE_VALUE GetFadeType() const override;
    void DrawFade();
    void Init(FADEINFO_s *);
    void InitFade();
    void UpdateFade();
};

enum GIZBUILDIT_STATE_FLAGS {
    GIZBUILDIT_STATE_ROTATING_WOBBLE = 0x0001,
    GIZBUILDIT_STATE_TURN_OFF_WHEN_COMPLETE = 0x0002,
    GIZBUILDIT_STATE_AUTO_BUILD_STATIC_OBJECTS = 0x0040,
    GIZBUILDIT_STATE_DISABLE_WOBBLE_WITHOUT_SHADOWS = 0x0080,
    GIZBUILDIT_STATE_DRAW_REFLECTION = 0x0100,
    GIZBUILDIT_STATE_REISSUE_REWARD_ON_ACTIVATE = 0x0200,
    GIZBUILDIT_STATE_DISABLE_SHADOW_RENDERING = 0x0400,
};

enum GIZBUILDIT_AVAILABILITY_FLAGS {
    GIZBUILDIT_AVAILABILITY_ACTIVE = 0x01,
    GIZBUILDIT_AVAILABILITY_VISIBLE = 0x02,
    GIZBUILDIT_AVAILABILITY_LINKED = 0x03,
    GIZBUILDIT_AVAILABILITY_INTERACTING = 0x04,
    GIZBUILDIT_AVAILABILITY_HAS_BLOWUP_TYPE = 0x80,
};

enum GIZBUILDIT_RUNTIME_FLAGS {
    GIZBUILDIT_RUNTIME_REWARD_RELEASED = 0x02,
};

enum GIZBUILDIT_BUILD_STATE {
    GIZBUILDIT_BUILD_IDLE = 0,
    GIZBUILDIT_BUILD_FINISHING = 1,
    GIZBUILDIT_BUILD_COMPLETE = 2,
};

struct part_emit_s {
    NUVEC position;
    i32 effect_id; // 0x0c
    char name[16];
    f32 time_20;
    f32 time_24;
    f32 time_28;
    i16 rotation_2c, rotation_2e, rotation_30;
    i8 page;
    u8 field_33;
    i32 shots_remaining;
    f32 time_38;
    u8 field_3c, field_3d, field_3e, field_3f;
    f32 camera_distance;
    i16 field_44, field_46;
    i32 instance_id; // 0x48
    i32 trailing_state_words[8];
};
DECOMP_ASSERT(sizeof(part_emit_s) == 0x6c, "part_emit_s size");

struct part_typedesc_s {
    char name[16];
    char object_names[8][16];
    i16 effect_ids[8];
    i8 effect_pages[8];
    i8 scene_indices[8];
    i8 variant_count;
    u8 variant_mode;
    i8 page;
    u8 field_b3;
    f32 particle_scale;
    f32 effect_scale;
    f32 lifetime;
    f32 lifetime_random;
    f32 speed;
    f32 gravity;
    f32 emission_rate;
    f32 bounce;
    NUVEC position_random;
    NUVEC velocity_random;
    f32 emission_period;
    f32 emission_period_random;
    f32 emission_pause;
    f32 emission_pause_random;
    i32 rotation[3];
    i32 rotation_random[3];
    u32 flags;
    i32 trail_effects[2];
    f32 trail_rates[2];
    i32 attached_effect;
    i32 impact_effect;
    i32 impact_part;
    char impact_part_name[16];
    i32 kill_effect;
    i32 sounds[4];
    u8 sound_modes[4];
    f32 maximum_distance;
    f32 field_160, field_164, field_168;
    f32 last_used_time;
    f32 scale;
    u32 field_174;
};
typedef part_typedesc_s part_type_s;
DECOMP_ASSERT(sizeof(part_type_s) == 0x178, "part_type_s size");

struct GIZBUILDITANIMDATA_s {
    NUMTX start_mtx;
    NUMTX end_mtx;
    NUMTX draw_mtx;
    f32 wobble_time;
    u8 wobble_axis;
    u8 padding_0xc5;
    u8 was_drawn;
    u8 padding_0xc7;
};
DECOMP_ASSERT(sizeof(GIZBUILDITANIMDATA_s) == 0xc8, "BuildIt animation-data ABI");
DECOMP_ASSERT(offsetof(GIZBUILDITANIMDATA_s, start_mtx) == 0x00, "BuildIt start matrix offset");
DECOMP_ASSERT(offsetof(GIZBUILDITANIMDATA_s, end_mtx) == 0x40, "BuildIt end matrix offset");
DECOMP_ASSERT(offsetof(GIZBUILDITANIMDATA_s, draw_mtx) == 0x80, "BuildIt draw matrix offset");
DECOMP_ASSERT(offsetof(GIZBUILDITANIMDATA_s, wobble_time) == 0xc0, "BuildIt wobble timer offset");

struct GIZBUILDIT_s {
    char name[0x10];
    GAMEANIMSET_s *anim_set;                    // 0x10
    GAMEANIMOBJ_s **anim_objects;               // 0x14
    GIZBUILDIT_s *linked_buildit;               // 0x18
    MechObjectInterface *mech_object_interface; // 0x1c
    NUVEC file_position;                        // 0x20
    NUVEC start_position;                       // 0x2c, centre of the unbuilt pieces
    NUVEC position;                             // 0x38, centre of the completed build
    f32 bounds_radius;                          // 0x44
    f32 step_timer;                             // 0x48
    f32 step_duration;                          // 0x4c
    f32 interaction_radius;                     // 0x50
    f32 field_0x54;
    f32 radius_scale; // 0x58
    i16 field_0x5c;
    union {
        i16 field_0x5e;
        u16 completion_score;
    };
    i16 field_0x60;
    i16 field_0x62;
    NUVEC effect_position; // 0x64
    f32 activation_radius; // 0x70
    u8 anim_object_count;  // 0x74
    u8 build_state;        // 0x75
    u8 builders_active;
    u8 built_object_count; // 0x77
    u8 field_0x78;
    u8 progress;     // 0x79
    u16 state_flags; // 0x7a
    u16 field_0x7c;
    i16 room_index;
    i16 blowup_type;       // 0x80
    u8 availability_flags; // 0x82
    u8 field_0x83;

    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(GIZBUILDIT_s) == 0x84, "GIZBUILDIT_s ABI");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, completion_score) == 0x5e, "GIZBUILDIT completion score offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, mech_object_interface) == 0x1c, "GIZBUILDIT interface offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, radius_scale) == 0x58, "GIZBUILDIT radius scale offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, start_position) == 0x2c, "GIZBUILDIT start position offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, position) == 0x38, "GIZBUILDIT position offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, bounds_radius) == 0x44, "GIZBUILDIT bounds radius offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, step_timer) == 0x48, "GIZBUILDIT timer offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, interaction_radius) == 0x50, "GIZBUILDIT radius offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, builders_active) == 0x76, "GIZBUILDIT active builder count offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, room_index) == 0x7e, "GIZBUILDIT room index offset");
DECOMP_ASSERT(offsetof(GIZBUILDIT_s, availability_flags) == 0x82, "GIZBUILDIT flags offset");

struct GIZBUILDITSYS_s {
    GIZBUILDIT_s *buildits;
    u16 count;                 // 0x04
    u16 capacity;              // 0x06
    u16 max_animation_objects; // 0x08
    u16 field_0x0a;
    GAMEANIMOBJPOOL_s *anim_pool; // 0x0c
    GAMEANIMOBJ_s **anim_objects; // 0x10, shared pointer table partitioned per build-it
};
DECOMP_ASSERT(sizeof(GIZBUILDITSYS_s) == 0x14, "GIZBUILDITSYS_s ABI");

enum GIZFORCE_CONFIG_FLAGS : u32 {
    GIZFORCE_CONFIG_WAIT_FOR_FORCE_RANGE = 0x00000002,
    GIZFORCE_CONFIG_DRAW_REFLECTION = 0x00000004,
    GIZFORCE_CONFIG_RESET_STATE_ON_ACTIVATE = 0x00000008,
    GIZFORCE_CONFIG_JEDI_BADDIE_ONLY = 0x00000010,
    GIZFORCE_CONFIG_BLOWUP_AT_ANIM_OBJECTS = 0x00000020,
    GIZFORCE_CONFIG_ALONG_SOCKET = 0x00000040,
    GIZFORCE_CONFIG_HIT_TEST_TYPE_0 = 0x00000100,
    GIZFORCE_CONFIG_HIT_TEST_TYPE_1 = 0x00000200,
    GIZFORCE_CONFIG_HIT_TEST_MASK = 0x00000300,
    GIZFORCE_CONFIG_TARGET_ANIMATION_OBJECTS = 0x00000400,
    GIZFORCE_CONFIG_LEGACY_ENABLED = 0x00000800,
    GIZFORCE_CONFIG_CAST_SHADOW = 0x00001000,
};

enum GIZFORCE_RUNTIME_FLAGS : u8 {
    GIZFORCE_RUNTIME_HAS_PLATFORM = 0x01,
    GIZFORCE_RUNTIME_REWARD_RELEASED = 0x02,
    GIZFORCE_RUNTIME_PENDING_BLOWUP_TYPE = 0x04,
    GIZFORCE_RUNTIME_ALONG_SOCKET_HIDDEN = 0x08,
    GIZFORCE_RUNTIME_OFFSET_APPLIED = 0x10,
    GIZFORCE_RUNTIME_FORCE_RANGE_COMPLETE = 0x20,
    GIZFORCE_RUNTIME_COMPLETION_RELEASED = 0x40,
    GIZFORCE_RUNTIME_PENDING_COMPLETION = 0x80,
};

enum GIZFORCE_PROGRESS_FLAGS : u8 {
    GIZFORCE_PROGRESS_ENABLED = 0x01,
    GIZFORCE_PROGRESS_VISIBLE = 0x02,
    GIZFORCE_PROGRESS_REVERSE_ACTIVE = 0x10,
    GIZFORCE_PROGRESS_GROUP_MEMBER = 0x20,
    GIZFORCE_PROGRESS_ANIMATION_REVERSED = 0x40,
    GIZFORCE_PROGRESS_DRAW_ACTIVE = 0x80,
};

enum GIZFORCE_STATE_FLAGS : u8 {
    GIZFORCE_STATE_DESTROYED_OR_THROWN = 0x01,
    GIZFORCE_STATE_DEBRIS_ACTIVE = 0x02,
    GIZFORCE_STATE_BEING_USED = 0x20,
};

enum GIZFORCE_GROUP_FLAGS : u8 {
    GIZFORCE_GROUP_ACTIVE = 0x01,
    GIZFORCE_GROUP_STACK_COMPLETE = 0x02,
    GIZFORCE_GROUP_STACK_COMPLETE_IN_ORDER = 0x04,
};

struct GIZFORCEANIMDATA_s {
    u16 flags;
    i16 platform_id;
    union {
        u32 field_0x04;
        struct {
            u32 force_glow_active : 1;
            u32 : 31;
        };
    };
};
DECOMP_ASSERT(sizeof(GIZFORCEANIMDATA_s) == 8, "GIZFORCE animation data ABI");

struct GIZFORCE_s {
    char name[0x10];
    NUVEC file_position;        // 0x10
    NUVEC position;             // 0x1c, position exposed through GIZMOFNS
    GAMEANIMSET_s *anim_set;    // 0x28
    nuhspecial_s along_socket;  // 0x2c
    GameObject_s *using_object; // 0x38, cleared after every Force update
    union {
        f32 field_0x3c;
        u32 field_0x3c_bits;
    };
    GIZFORCEGROUP_s *group; // 0x40
    f32 force_strength;     // 0x44
    f32 field_0x48;
    f32 force_range; // 0x4c
    f32 field_0x50;
    f32 interaction_radius; // 0x54
    f32 animation_speed;    // 0x58
    f32 start_frame;        // 0x5c
    f32 end_frame;          // 0x60
    f32 effect_scale;       // 0x64
    f32 radius;             // 0x68
    f32 strength_0x6c;      // 0x6c
    f32 vertical_range;     // 0x70
    f32 horizontal_range;   // 0x74
    u32 config_flags;       // 0x78, GIZFORCE_CONFIG_FLAGS
    u32 field_0x7c;
    u8 group_index;     // 0x80
    u8 collision_mask;  // 0x81
    u8 activation_mode; // 0x82
    u8 field_0x83;
    i16 room_id;      // 0x84
    i16 start_sfx_id; // 0x86
    i16 loop_sfx_id;  // 0x88
    i16 stop_sfx_id;  // 0x8a
    i16 blowup_type;  // 0x8c
    union {
        i16 debris_type;
        u16 pickup_count;     // 0x8e
        u16 completion_score; // 0x8e
    };
    union {
        i16 hit_points;
        u16 pickup_direction_x;
        u16 pickup_rotation_x; // 0x90
    };
    union {
        i16 score;
        u16 pickup_direction_y;
        u16 pickup_rotation_y; // 0x92
    };
    union {
        NUVEC effect_position;
        NUVEC pickup_offset; // 0x94
    };
    union {
        f32 activation_radius;
        f32 pickup_scatter_height; // 0xa0
    };
    union {
        u32 field_0xa4;
        MechObjectInterface *mech_object_interface;
    };
    union {
        struct {
            u8 progress_flags; // 0xa8, GIZFORCE_PROGRESS_FLAGS
            u8 runtime_flags;  // 0xa9, GIZFORCE_RUNTIME_FLAGS
            union {
                u8 field_0xaa;
                u8 state_flags; // GIZFORCE_STATE_FLAGS
            };
            u8 field_0xab;
        };
        u32 packed_state;
    };
    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(GIZFORCE_s) == 0xac, "GIZFORCE_s ABI");
DECOMP_ASSERT(offsetof(GIZFORCE_s, mech_object_interface) == 0xa4, "Force object interface offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, completion_score) == 0x8e, "GIZFORCE completion score offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, anim_set) == 0x28, "GIZFORCE animation set offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, config_flags) == 0x78, "GIZFORCE config flags offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, pickup_count) == 0x8e, "GIZFORCE pickup count offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, pickup_rotation_x) == 0x90, "GIZFORCE pickup X rotation offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, pickup_rotation_y) == 0x92, "GIZFORCE pickup Y rotation offset");
DECOMP_ASSERT(offsetof(GIZFORCE_s, progress_flags) == 0xa8, "GIZFORCE progress flags offset");

struct GIZFORCEGROUP_s {
    GIZFORCE_s *forces[8];
    f32 combined_height;
    u8 field_0x24;
    u8 count;
    u8 configured_count;
    u8 field_0x27;
};
DECOMP_ASSERT(sizeof(GIZFORCEGROUP_s) == 0x28, "GIZFORCE group ABI");

struct GIZFORCESYS_s {
    GIZFORCE_s *forces;           // 0x00
    GIZFORCE_s **visible_forces;  // 0x04, rebuilt by GizForces_Update
    GIZMO_s **hit_test_gizmos;    // 0x08, rebuilt by GizForces_Update
    u16 capacity;                 // 0x0c
    u16 count;                    // 0x0e
    u16 visible_force_count;      // 0x10
    u16 hit_test_gizmo_count;     // 0x12
    GAMEANIMOBJPOOL_s *anim_pool; // 0x14
    GIZFORCEGROUP_s groups[8];    // 0x18
};
DECOMP_ASSERT(sizeof(GIZFORCESYS_s) == 0x158, "GIZFORCESYS_s ABI");
DECOMP_ASSERT(offsetof(GIZFORCESYS_s, visible_force_count) == 0x10, "GIZFORCESYS visible count offset");
struct GIZMOBLOWUP_s {
    union {
        NUMTX transform; // 0x00, instance transform refreshed by the early-update pass
        struct {
            undefined field0_0x0[0x30];
            NUVEC position; // 0x30, translation exposed through GIZMOFNS
            undefined field_0x3c[4];
        };
    };
    union {
        undefined field_0x40[4];
        MechObjectInterface *mech_object_interface;
    };
    NUVEC screen_position; // 0x44, projected by the draw pass
    union {
        char field_0x50[0x50]; // compatibility view used by level-specific links
        struct {
            NUVEC mid_position; // 0x50, center used by effects and collision
            NUVEC bounds_min;   // 0x5c
            NUVEC bounds_max;   // 0x68
            f32 field_0x74;
            f32 field_0x78;
            f32 field_0x7c;
            f32 field_0x80;
            f32 field_0x84;
            f32 field_0x88;
            f32 field_0x8c;
            f32 field_0x90;
            f32 field_0x94;
            f32 flicker_timer; // 0x98
            union {
                u32 status_flags; // 0x9c, aggregate tested by the draw pass
                struct {
                    u8 output_flags;     // 0x9c, GIZMOBLOWUP_OUTPUT_FLAGS
                    u8 visibility_flags; // 0x9d, GIZMOBLOWUP_VISIBILITY_FLAGS
                    u8 state_flags;      // 0x9e, GIZMOBLOWUP_STATE_FLAGS
                    u8 field_0x9f;       // 0x9f, secondary output/runtime flags
                };
            };
        };
    };
    union {
        u32 draw_flags; // 0xa0, GIZMOBLOWUP_DRAW_FLAGS
        u32 field_0xa0; // compatibility name for level-specific setup code
    };
    u32 secondary_flags;     // 0xa4
    u32 field_0xa8;          // 0xa8
    GIZMOBLOWUPTYPE_s *type; // 0xac, shared type/animation data
    f32 target_scale;        // 0xb0, scales the transform-target marker
    f32 field_0xb4;
    f32 field_0xb8;
    f32 activation_delay; // 0xbc
    f32 field_0xc0;
    f32 animation_time; // 0xc4, explicitly selected animation frame
    f32 field_0xc8;
    f32 field_0xcc;
    f32 animation_offset; // 0xd0, offset from the type's base frame
    f32 respawn_timer;    // 0xd4
    f32 field_0xd8;
    f32 reflection_height;     // 0xdc
    GAMEANTINODE_s *anti_node; // 0xe0, registered while the blowup is visible
    i16 field_0xe4;
    i16 field_0xe6;
    i16 field_0xe8;
    i16 field_0xea;
    i16 field_0xec;
    i16 field_0xee;
    u16 field_0xf0;
    u16 field_0xf2;
    u16 field_0xf4;
    i16 field_0xf6;
    i16 field_0xf8;
    char name[0x10]; // 0xfa
    i16 platform_id; // 0x10a, terrain platform toggled with visibility
    i16 field_0x10c;
    undefined field_0x10e[6];
    u8 field_0x114;
    u8 field_0x115;
    u8 saved_state_0;   // 0x116
    u8 initial_state_0; // 0x117
    u8 saved_state_1;   // 0x118
    u8 initial_state_1; // 0x119
    undefined field_0x11a[2];
    nuhspecial_s *override_special; // 0x11c, optional per-instance special
    void *field_0x120;              // 0x120
    u8 field_0x124;                 // 0x124
    undefined field_0x125[3];
    float field_0x128; // 0x128
    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(GIZMOBLOWUP_s) == 0x12c, "GIZMOBLOWUP_s ABI");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, position) == 0x30, "GIZMOBLOWUP position offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, output_flags) == 0x9c, "GIZMOBLOWUP output flags offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, status_flags) == 0x9c, "GIZMOBLOWUP aggregate status offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, draw_flags) == 0xa0, "GIZMOBLOWUP draw flags offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, type) == 0xac, "GIZMOBLOWUP type offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, target_scale) == 0xb0, "GIZMOBLOWUP target scale offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, reflection_height) == 0xdc, "GIZMOBLOWUP reflection height offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, field_0xa8) == 0xa8, "GIZMOBLOWUP base score offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, field_0x115) == 0x115, "GIZMOBLOWUP score multiplier offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, name) == 0xfa, "GIZMOBLOWUP name offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, platform_id) == 0x10a, "GIZMOBLOWUP platform id offset");
DECOMP_ASSERT(offsetof(GIZMOBLOWUP_s, override_special) == 0x11c, "GIZMOBLOWUP override special offset");
struct GIZOBSTACLE_s {
    char name[0x10];
    union {
        NUVEC position; // 0x10, position exposed through GIZMOFNS
        struct {
            f32 position_x;
            f32 position_y;
            f32 field_0x18;
        };
    };
    union {
        NUVEC secondary_position; // 0x1c
        struct {
            f32 field_0x1c;
            f32 field_0x20;
            f32 field_0x24;
        };
    };
    NUVEC evaluated_position;        // 0x28, refreshed from the animation objects
    GAMEANIMSET_s *anim_set;         // 0x34
    GameObject_s *triggering_object; // 0x38, closest object satisfying the trigger
    union {
        f32 auto_return_delay; // 0x3c
        f32 field_0x3c;        // compatibility alias for level scripts
    };
    f32 auto_return_timer; // 0x40
    f32 trigger_radius;    // 0x44
    f32 animation_speed;   // 0x48, reset to 1.0
    f32 field_0x4c;
    f32 field_0x50;
    f32 reflection_alpha; // 0x54
    f32 field_0x58;
    NUVEC trigger_box_half_extents; // 0x5c
    u32 config_flags;               // 0x68
    u32 field_0x6c;
    NUVEC pickup_offset; // 0x70
    f32 pickup_scatter_height;
    i16 pickup_direction_x;
    i16 pickup_direction_y;
    i16 trigger_box_yaw;
    i16 room_id;     // 0x86
    i16 blowup_type; // 0x88, name-table id until PostLoad
    union {
        i16 pickup_count;
        u16 completion_score;
    };
    i16 start_sfx_id; // 0x8c
    i16 stop_sfx_id;  // 0x8e
    u8 state;         // 0x90
    u8 mode;          // 0x91
    u8 trigger_mode;  // 0x92
    u8 field_0x93;
    MechObjectInterface *mech_object_interface; // 0x94
    union {
        u8 progress_flags; // 0x98, persisted by GizObstacles progress data
        struct {
            u8 progress_enabled : 1;
            u8 progress_visible : 1;
            u8 progress_external_control : 1;
            u8 progress_push_control : 1;
            u8 progress_reserved : 4;
        };
    };
    u8 control_flags; // 0x99, GIZOBSTACLE_CONTROL_FLAGS
    u8 field_0x9a[2];
    i32 proximity_output; // 0x9c
    u8 runtime_flags;     // 0xa0, GIZOBSTACLE_RUNTIME_FLAGS
    u8 field_a1_0xa1;
    u8 field_0xa2[2];
    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, mech_object_interface) == 0x94, "Obstacle interface offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, evaluated_position) == 0x28, "Obstacle evaluated position offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, field_0x58) == 0x58, "Obstacle evaluated radius offset");

enum GIZOBSTACLE_CONFIG_FLAGS : u32 {
    GIZOBSTACLE_CONFIG_INVERT_PROXIMITY = 0x0001,
    GIZOBSTACLE_CONFIG_USE_ANIM_AVERAGE_POSITION = 0x0002,
    GIZOBSTACLE_CONFIG_EXCLUDE_NON_PLAYER = 0x0004,
    GIZOBSTACLE_CONFIG_REQUIRE_CHARACTER_DATA_FLAG_20 = 0x0008,
    GIZOBSTACLE_CONFIG_CHECK_SUPPORTING_PLATFORM = 0x0010,
    GIZOBSTACLE_CONFIG_REQUIRE_ACTIVE_PLAYER = 0x0020,
    GIZOBSTACLE_CONFIG_DRAW_REFLECTION = 0x0040,
    GIZOBSTACLE_CONFIG_RESET_AFTER_COMPLETION = 0x0080,
    GIZOBSTACLE_CONFIG_BLOWUP_AT_ANIM_OBJECTS = 0x0200,
    GIZOBSTACLE_CONFIG_REQUIRE_LINKED_OBJECT = 0x0400,
    GIZOBSTACLE_CONFIG_ALWAYS_RUN_PROXIMITY = 0x0800,
    GIZOBSTACLE_CONFIG_ADD_TO_ACTIVE_LIST = 0x1000,
    GIZOBSTACLE_CONFIG_REQUIRE_ALL_PLAYERS = 0x2000,
    GIZOBSTACLE_CONFIG_REQUIRE_CHARACTER_DATA_FLAG_04 = 0x4000,
};

enum GIZOBSTACLE_PROGRESS_FLAGS : u8 {
    GIZOBSTACLE_PROGRESS_FLAG_ENABLED = 0x01,
    GIZOBSTACLE_PROGRESS_FLAG_VISIBLE = 0x02,
    GIZOBSTACLE_PROGRESS_FLAG_EXTERNAL_CONTROL = 0x04,
    GIZOBSTACLE_PROGRESS_FLAG_PUSH_CONTROL = 0x08,
};

enum GIZOBSTACLE_CONTROL_FLAGS : u8 {
    GIZOBSTACLE_CONTROL_FLAG_REVERSE_ACTIVE = 0x01,
};

enum GIZOBSTACLE_RUNTIME_FLAGS : u8 {
    // Pickups for this completion have already been emitted.
    GIZOBSTACLE_RUNTIME_FLAG_PICKUPS_SPAWNED = 0x01,
    // This is set while blowup_type still contains a load-time name-table id.
    // PostLoad resolves the final type and clears the bit.
    GIZOBSTACLE_RUNTIME_FLAG_PENDING_BLOWUP_TYPE = 0x02,
    // The cantina barman obstacle sets this while its controlling AI set is
    // alive.  The blocked bit is mutually exclusive there.
    GIZOBSTACLE_RUNTIME_FLAG_AI_ACTIVE = 0x04,
    // Prevents interaction/entry. Hub doors set this while locked.
    GIZOBSTACLE_RUNTIME_FLAG_BLOCKED = 0x08,
    // At least one animated scene object belongs to a terrain platform.
    GIZOBSTACLE_RUNTIME_FLAG_HAS_PLATFORM = 0x10,
    // At least one animation object's serialized flags contain bit 1.
    GIZOBSTACLE_RUNTIME_FLAG_ANIM_OBJECT_BIT_1 = 0x20,
    // Keeps proximity-driven animation active until the end state is reached.
    GIZOBSTACLE_RUNTIME_FLAG_TRIGGER_LATCHED = 0x40,
    // The obstacle has completed its destruction path and is no longer drawn.
    GIZOBSTACLE_RUNTIME_FLAG_DESTROYED = 0x80,
};

DECOMP_ASSERT(sizeof(GIZOBSTACLE_s) == 0xa4, "GIZOBSTACLE_s ABI");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, completion_score) == 0x8a, "GIZOBSTACLE completion score offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, anim_set) == 0x34, "GIZOBSTACLE anim set offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, config_flags) == 0x68, "GIZOBSTACLE config flags offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, progress_flags) == 0x98, "GIZOBSTACLE progress flags offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, proximity_output) == 0x9c, "GIZOBSTACLE proximity output offset");
DECOMP_ASSERT(offsetof(GIZOBSTACLE_s, runtime_flags) == 0xa0, "GIZOBSTACLE runtime flags offset");

struct GIZOBSTACLESYS_s {
    GIZOBSTACLE_s *obstacles; // 0x00
    GIZMO_s **active_gizmos;  // 0x04, rebuilt by GizObstacles_Update
    u16 count;                // 0x08
    u16 capacity;             // 0x0a
    u16 active_gizmo_count;   // 0x0c
    u16 field_0x0e;
    GAMEANIMOBJPOOL_s *anim_pool; // 0x10
};
DECOMP_ASSERT(sizeof(GIZOBSTACLESYS_s) == 0x14, "GIZOBSTACLESYS_s ABI");

// Node in the obstacle linked list (field_0x34 → field_0x18 chain).
struct GIZOBSTACLENODE_s {
    void *next;    // 0x00
    void *special; // 0x04
    char pad_0x08[0x18 - 0x08];
    void *field_0x18; // 0x18
};

// Compatibility view of a force group used by older level-specific code.
struct GIZFORCEOBJ_s {
    char pad_0x00[0x24];
    u32 flags; // 0x24
};

// Path connection flags (LevPathCnx entries).
struct PATHCNXDATA_s {
    u32 flags0; // 0x00
    u32 flags4; // 0x04
};

// Cutscene scene data (byte flag at 0x89).
struct CUTSCENEDATA_s {
    char pad_0x00[0x88];
    u8 field_0x88; // 0x88  flags byte
    u8 flags;      // 0x89
};

// Player sub-object (through APIOBJECT::field_0x54).
struct PLAYERSUBOBJ_s {
    char pad_0x00[0x24];
    void *field_0x24; // 0x24
};

// Second-level player sub-object (float at 0x1c).
struct PLAYERSUBOBJ2_s {
    char pad_0x00[0x1c];
    float value; // 0x1c
};

// Character entry inside apicharsys character-data array (stride 0x54).
struct PODCHARENTRY_s {
    char pad_0x00[0xc];
    void *model;                // 0x0c
    char pad_0x10[0x54 - 0x10]; // pad to the 0x54-byte stride
};

// Model data referenced by PODCHARENTRY_s::model.
struct PODMODELDATA_s {
    char pad_0x00[0x4];
    float *value; // 0x04 pointer to the pod model's speed value
};
enum GIZPANEL_FLAGS : u8 {
    GIZPANEL_FLAG_STATE = 1 << 1,
    GIZPANEL_FLAG_VISIBLE = 1 << 2,
    GIZPANEL_FLAG_TRACK_PLAYER = 1 << 3,
    GIZPANEL_FLAG_PLAYER_NEAR = 1 << 4,
    GIZPANEL_FLAG_HIDE_BASE = 1 << 5,
    GIZPANEL_FLAG_HIDE_TARGET = 1 << 6,
    GIZPANEL_FLAG_BADDIE = 1 << 7,
};

enum GIZPANEL_DRAW_FLAGS : u8 {
    GIZPANEL_DRAW_FLAG_ALT_MODEL = 1 << 0,
    GIZPANEL_DRAW_FLAG_NO_SHADOW_RESET = 1 << 1,
    GIZPANEL_DRAW_FLAG_LEGACY = 1 << 2,
};

struct GIZPANEL_s {
    NUMTX matrix;    // 0x00
    char name[0x10]; // 0x40
    NUVEC position;  // 0x50
    u16 y_rotation;  // 0x5c
    u8 field_0x5e[3];
    u8 model_variant; // 0x61
    u8 field_0x62[2];
    f32 flash_timer; // 0x64
    union {
        GIZPANEL_FLAGS flags; // 0x68
        struct {
            u8 reserved_flag : 1;
            u8 state : 1;
            u8 visible : 1;
            u8 track_player : 1;
            u8 player_near : 1;
            u8 hide_base : 1;
            u8 hide_target : 1;
            u8 baddie : 1;
        };
    };
    GIZPANEL_DRAW_FLAGS draw_flags; // 0x69
    u8 field_0x6a[2];
    NUVEC floor_position;                       // 0x6c
    NUVEC target_offset;                        // 0x78
    u16 target_x_rotation;                      // 0x84
    u16 target_y_rotation;                      // 0x86
    u16 arm_x_rotation;                         // 0x88
    u16 target_pitch;                           // 0x8a
    u16 target_roll;                            // 0x8c
    i16 platform_id;                            // 0x8e
    f32 activation_time;                        // 0x90
    f32 target_scale;                           // 0x94
    MechObjectInterface *mech_object_interface; // 0x98

    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(GIZPANEL_s) == 0x9c, "GIZPANEL_s ABI");
DECOMP_ASSERT(offsetof(GIZPANEL_s, name) == 0x40, "GIZPANEL name offset");
DECOMP_ASSERT(offsetof(GIZPANEL_s, position) == 0x50, "GIZPANEL position offset");
DECOMP_ASSERT(offsetof(GIZPANEL_s, flags) == 0x68, "GIZPANEL flags offset");
DECOMP_ASSERT(offsetof(GIZPANEL_s, floor_position) == 0x6c, "GIZPANEL floor-position offset");
DECOMP_ASSERT(offsetof(GIZPANEL_s, platform_id) == 0x8e, "GIZPANEL platform-id offset");
DECOMP_ASSERT(offsetof(GIZPANEL_s, activation_time) == 0x90, "GIZPANEL activation-time offset");

struct GIZPANELSYS_s {
    i32 count;
    GIZPANEL_s *panels;
};
DECOMP_ASSERT(sizeof(GIZPANELSYS_s) == 8, "GIZPANELSYS_s ABI");
enum GIZSPECIAL_FLAGS : u8 {
    GIZSPECIAL_FLAG_REVERSED = 0x01,
    GIZSPECIAL_FLAG_ACTIVE = 0x02,
    GIZSPECIAL_FLAG_ANIMATION_MASK = GIZSPECIAL_FLAG_REVERSED | GIZSPECIAL_FLAG_ACTIVE,
};

struct GIZSPECIAL_s {
    char name[0x20];
    GAMEANIMSET_s *anim_set; // 0x20
    union {
        GIZSPECIAL_FLAGS flags; // 0x24
        struct {
            u8 is_reversed : 1;
            u8 is_active : 1;
            u8 field_0x24_2 : 6;
        };
    };
    u8 field_0x25[3];
};
DECOMP_ASSERT(sizeof(GIZSPECIAL_s) == 0x28, "GIZSPECIAL_s ABI");
DECOMP_ASSERT(offsetof(GIZSPECIAL_s, anim_set) == 0x20, "GIZSPECIAL anim-set offset");

struct GIZSPECIALSYS_s {
    GIZSPECIAL_s *specials; // 0x00
    i32 count;              // 0x04
    u8 field_0x08[4];
    GAMEANIMOBJPOOL_s *anim_pool; // 0x0c
};
DECOMP_ASSERT(sizeof(GIZSPECIALSYS_s) == 0x10, "GIZSPECIALSYS_s ABI");

struct GIZTURRET_s {
    u8 field_0x00[8];
    char name[0x10];
    GAMEANIMSET_s *anim_set;           // 0x18
    GAMEANIMOBJ_s *primary_anim_obj;   // 0x1c
    GAMEANIMOBJ_s *secondary_anim_obj; // 0x20
    NUVEC position;                    // 0x24, position exposed through GIZMOFNS
    NUVEC field_0x30;
    NUVEC field_0x3c;
    NUVEC field_0x48;
    NUANG pitch; // 0x54
    i32 field_0x58;
    i32 field_0x5c;
    NUANG yaw; // 0x60
    i32 field_0x64;
    i32 field_0x68;
    i32 field_0x6c;
    i32 field_0x70;
    NUVEC field_0x74[4];
    NUMTX field_0xa4;
    void *field_0xe4;
    GameObject_s *controller; // 0xe8
    f32 field_0xec;
    f32 field_0xf0;
    f32 reflection_alpha; // 0xf4
    f32 fire_cooldown;    // 0xf8
    f32 fire_interval;    // 0xfc
    f32 pitch_turn_speed; // 0x100
    f32 yaw_turn_speed;   // 0x104
    union {
        u32 behavior_flags; // 0x108
        struct {
            u8 behavior_flags_low;
            u8 animation_flags; // 0x109
            u16 behavior_flags_high;
        };
    };
    union {
        i16 field_0x10c;
        u16 completion_score;
    };
    i16 room_id; // 0x10e
    i16 field_0x110;
    i16 field_0x112;
    NUVEC field_0x114;
    f32 field_0x120;
    i16 blowup_type; // 0x124 (name-table id until post-load fixup)
    i16 field_0x126;
    i16 base_y_rotation; // 0x128
    i16 field_0x12a;
    u8 field_0x12c;
    i8 bolt_type_id; // 0x12d
    u8 field_0x12e;
    u8 field_0x12f;
    u8 field_0x130;
    u8 field_0x131;
    u8 field_0x132[2];
    i16 field_0x134;
    u8 field_0x136[2];
    i16 field_0x138;
    u8 flags;                                   // 0x13a
    u8 runtime_flags;                           // 0x13b
    MechObjectInterface *mech_object_interface; // 0x13c
    f32 field_0x140;
    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(GIZTURRET_s) == 0x144, "GIZTURRET_s ABI");
DECOMP_ASSERT(offsetof(GIZTURRET_s, controller) == 0xe8, "GIZTURRET controller offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, completion_score) == 0x10c, "GIZTURRET completion score offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, position) == 0x24, "GIZTURRET position offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, pitch) == 0x54, "GIZTURRET pitch offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, yaw) == 0x60, "GIZTURRET yaw offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, base_y_rotation) == 0x128, "GIZTURRET base-y-rotation offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, bolt_type_id) == 0x12d, "GIZTURRET bolt-type offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, reflection_alpha) == 0xf4, "GIZTURRET reflection-alpha offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, fire_cooldown) == 0xf8, "GIZTURRET fire-cooldown offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, behavior_flags) == 0x108, "GIZTURRET behavior-flags offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, animation_flags) == 0x109, "GIZTURRET animation-flags offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, room_id) == 0x10e, "GIZTURRET room offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, blowup_type) == 0x124, "GIZTURRET blowup-type offset");
DECOMP_ASSERT(offsetof(GIZTURRET_s, flags) == 0x13a, "GIZTURRET flags offset");

struct GIZTURRETSYS_s {
    GIZTURRET_s *turrets;         // 0x00
    GAMEANIMOBJPOOL_s *anim_pool; // 0x04
    u16 count;                    // 0x08
    u16 capacity;                 // 0x0a
    u8 field_0x0c[4];
};
DECOMP_ASSERT(sizeof(GIZTURRETSYS_s) == 0x10, "GIZTURRETSYS_s ABI");
struct GameThingManager; // defined after ThingManager (derives from it)

enum HATMACHINE_FLAGS : u8 {
    HATMACHINE_FLAG_ANIMATING = 0x01,
    HATMACHINE_FLAG_FINISHED = 0x02,
    HATMACHINE_FLAG_VISIBLE = 0x04,
    HATMACHINE_FLAG_ENABLED = 0x08,
    HATMACHINE_FLAG_HIDE_MACHINE = 0x20,
};

struct HATMACHINE_s {
    union {
        NUMTX transform;
        NUMTX matrix;
    }; // 0x00
    char name[0x10]; // 0x40
    NUVEC position;  // 0x50
    union {
        u16 y_rotation;
        u16 yaw;
    }; // 0x5c
    union {
        u8 configured_hat_count;
        u8 configured_hat;
    }; // 0x5e
    union {
        u8 displayed_hat;
        u8 current_hat;
    }; // 0x5f
    i8 animation_state; // 0x60
    union {
        u8 machine_special_index;
        char model_letter;
    }; // 0x61
    union {
        i8 target_special_index;
        u8 model_special_index;
    }; // 0x62
    union {
        HATMACHINE_FLAGS flags;
        struct {
            u8 state_bit0 : 1;
            u8 state_bit1 : 1;
            u8 progress_state1 : 1;
            u8 progress_state0 : 1;
            u8 reserved_state_bits : 4;
        };
    }; // 0x63
    union {
        NUVEC ground_offset;
        NUVEC player_position;
    }; // 0x64
    NUVEC target_offset; // 0x70
    union {
        u16 ground_x_rotation;
        u16 terrain_pitch;
    }; // 0x7c
    union {
        u16 ground_z_rotation;
        u16 terrain_roll;
    }; // 0x7e
    i16 platform_id; // 0x80
    u16 reserved_82;
    union {
        f32 hat_delay;
        f32 hat_refresh_timer;
    }; // 0x84
    union {
        f32 state_duration;
        f32 animation_duration;
    }; // 0x88
    f32 state_elapsed; // 0x8c
    union {
        f32 target_scale;
        f32 scale;
    }; // 0x90
    f32 flash_timer; // 0x94
    union {
        f32 animation_time;
        f32 render_animation_time;
    }; // 0x98
    union {
        f32 idle_bounce_timer;
        f32 blink_timer;
    }; // 0x9c
    u8 reserved_a0[4];
    MechObjectInterface *mech_object_interface; // 0xa4

    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(HATMACHINE_s) == 0xa8, "HATMACHINE_s ABI");
DECOMP_ASSERT(offsetof(HATMACHINE_s, name) == 0x40, "HATMACHINE name offset");
DECOMP_ASSERT(offsetof(HATMACHINE_s, position) == 0x50, "HATMACHINE position offset");
DECOMP_ASSERT(offsetof(HATMACHINE_s, player_position) == 0x64, "HATMACHINE movement target offset");
DECOMP_ASSERT(offsetof(HATMACHINE_s, flags) == 0x63, "HATMACHINE flags offset");
DECOMP_ASSERT(offsetof(HATMACHINE_s, hat_delay) == 0x84, "HATMACHINE timer offset");

struct HATMACHINESYS_s {
    i32 count; // 0x00
    i32 reserved_04;
    i32 reserved_08;
    HATMACHINE_s *machines; // 0x0c
};
DECOMP_ASSERT(sizeof(HATMACHINESYS_s) == 0x10, "HATMACHINESYS_s ABI");
struct HudRadarPulseStage {
    i32 angle;
    f32 radius;
    f32 speed;
    f32 delay;
    u8 delay_finished;
    u8 finished;
    u8 reserved_12[2];
};
DECOMP_ASSERT(sizeof(HudRadarPulseStage) == 0x14, "HudRadarPulseStage ABI");

struct HudRadarPulse {
    HudRadarPulse(VuVec const &);
    u8 IsFinished();
    void Process(float);
    void Render();

    HudRadarPulseStage pulses[3];
    VuVec position;
    u8 active;
    u8 paused;
    u8 reserved_4e[2];
};
DECOMP_ASSERT(offsetof(HudRadarPulse, pulses) == 0x00, "HudRadarPulse::pulses ABI");
DECOMP_ASSERT(offsetof(HudRadarPulse, position) == 0x3c, "HudRadarPulse::position ABI");
DECOMP_ASSERT(offsetof(HudRadarPulse, active) == 0x4c, "HudRadarPulse::active ABI");
DECOMP_ASSERT(offsetof(HudRadarPulse, paused) == 0x4d, "HudRadarPulse::paused ABI");
DECOMP_ASSERT(sizeof(HudRadarPulse) == 0x50, "HudRadarPulse ABI");
enum LEVER_FLAGS : u16 {
    LEVER_FLAG_INTERACTING = 0x0001,
    LEVER_FLAG_BEING_PULLED = 0x0002,
    LEVER_FLAG_GOODIE = 0x0004,
    LEVER_FLAG_BADDIE = 0x0008,
    LEVER_FLAG_VISIBLE = 0x0010,
    LEVER_FLAG_AUTO_RESET = 0x0020,
    LEVER_FLAG_RETURNING = 0x0040,
    LEVER_FLAG_ENABLED = 0x0080,
    LEVER_FLAG_HIDE_BASE = 0x0100,
    LEVER_FLAG_HIDE_TARGET_INDICATOR = 0x0200,
    LEVER_FLAG_INTERACTION_FINISHED = 0x0800,
    LEVER_FLAG_PULLED_DOWN = 0x1000,
    LEVER_FLAG_LOW_STATE_MASK = 0x004f,
    LEVER_FLAG_RETURN_RESET_MASK = LEVER_FLAG_LOW_STATE_MASK | LEVER_FLAG_PULLED_DOWN,
    LEVER_FLAG_IDLE_STATE_MASK = LEVER_FLAG_INTERACTING | LEVER_FLAG_BEING_PULLED | LEVER_FLAG_ENABLED,
};

struct LEVER_s {
    NUMTX transform;                  // 0x00
    MechObjectInterface *mech_object; // 0x40
    NUVEC floor_position;             // 0x44
    NUVEC target_offset;              // 0x50
    char name[0x10];                  // 0x5c
    NUVEC position;                   // 0x6c
    f32 idle_animation_timer;         // 0x78
    f32 pull_progress;                // 0x7c
    f32 auto_reset_timer;             // 0x80
    f32 flash_timer;                  // 0x84
    f32 auto_reset_delay;             // 0x88
    f32 target_indicator_scale;       // 0x8c
    u16 y_rotation;                   // 0x90
    i16 model_special_index;          // 0x92
    i16 platform_id;                  // 0x94
    u16 animation_frame;              // 0x96
    u16 target_x_rotation;            // 0x98
    u16 target_z_rotation;            // 0x9a
    union {
        u16 flags; // 0x9c, LEVER_FLAGS
        struct {
            u8 interacting : 1;
            u8 being_pulled : 1;
            u8 goodie : 1;
            u8 baddie : 1;
            u8 visible : 1;
            u8 auto_reset : 1;
            u8 returning : 1;
            u8 enabled : 1;
            u8 flags_high;
        };
    };
    char model_variant; // 0x9e
    u8 field_0x9f[9];

    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(LEVER_s) == 0xa8, "LEVER_s ABI");
DECOMP_ASSERT(offsetof(LEVER_s, mech_object) == 0x40, "Lever interface pointer offset");
DECOMP_ASSERT(offsetof(LEVER_s, target_offset) == 0x50, "LEVER target offset");
DECOMP_ASSERT(offsetof(LEVER_s, floor_position) == 0x44, "LEVER movement target offset");
DECOMP_ASSERT(offsetof(LEVER_s, name) == 0x5c, "LEVER name offset");
DECOMP_ASSERT(offsetof(LEVER_s, position) == 0x6c, "LEVER position offset");
DECOMP_ASSERT(offsetof(LEVER_s, flags) == 0x9c, "LEVER flags offset");
struct LevelEditorScene {
    u8 reserved_0x00[0xa0];
    nugscn_s *scene;
    i32 field_0xa4;
};
DECOMP_ASSERT(sizeof(LevelEditorScene) == 0xa8, "LevelEditorScene ABI");

struct LevelEditor {
    u8 pad_0x000[0x2a0];
    i32 reset_pending;
    LevelEditorScene scenes[10]; // 0x2a4
    BaseEditor *first_editor;
    BaseEditor *last_editor;
    i32 editor_count;

    void AddInfoText(char *);
    void AddScene(char *, nugscn_s *, i32);
    void AddText(char *);
    void BeginMultiLoad(variptr_u *, variptr_u *);
    void ClearLevel(i32);
    void CloseMenu();
    void CreateEditorList(eduimenu_s *, eduiitem_s *);
    void CreateMenu();
    void Display(ThingRenderData *);
    void DrawInfoText(char **, i32, i32, i32, i32, i32, i32, i32);
    void EndMultiLoad(variptr_u *, variptr_u *);
    void Enter();
    void Exit();
    void FindSceneId(char *);
    void Flush();
    LevelEditorScene *GetEdScene(i32);
    void GetScene(char *);
    nugscn_s *GetScene(i32);
    void Initalise(variptr_u &, variptr_u &, i32);
    void IsActiveScene(nugscn_s *);
    void IsEditable(i32);
    LevelEditor();
    void Load(char *, variptr_u *, variptr_u *, i32);
    void LoadState(variptr_u *, variptr_u *, variptr_u *, variptr_u *, variptr_u *, variptr_u *);
    void ProcessEvenWhenPaused(ThingProcessData *);
    void ReadStream(EdFileInputStream &);
    void RegisterEditor(BaseEditor &);
    void Reset();
    void Save();
    void SaveState(i32, variptr_u *, variptr_u *);
    void SaveState(variptr_u *, variptr_u *);
    void SetNextMenu(eduimenu_s *);
    void SetPadText(i32, char *);
    void SetSaveFilename(char *);
    void WriteStream(EdFileOutputStream &);
};
DECOMP_ASSERT(offsetof(LevelEditor, reset_pending) == 0x2a0, "LevelEditor reset_pending offset");
DECOMP_ASSERT(offsetof(LevelEditor, scenes) == 0x2a4, "LevelEditor scenes offset");
struct MemoryManager {
    usize cursor;
    usize end;
    usize *cursor_cell;
    usize *end_cell;
    usize high_water;
    usize allocated;
    usize remaining;
    void *free_lists[139];

    void *AllocPool(u32, i32);
    void FreePool(void *, u32);
};
// One mine entry (0x1c bytes, MINESYS_s::mines[64] starting at 0x0c).
struct MINEENTRY_s {
    u32 active;       // 0x00
    NUVEC pos;        // 0x04
    float grow;       // 0x10 grows by FRAMETIME each update
    float grow_alpha; // 0x14 clamped copy of grow (max 1.0); used as the draw scale
    u16 rotx;         // 0x18 draw pre-rotation about X
    u16 roty;         // 0x1a draw pre-rotation about Y
};

// Mine system (minesys global, 0x748 bytes). Layout from the original
// pod-race code: PodRaceInit fills the header + radius/areas/debris ids,
// UpdatePodRaceMines iterates mines[] and uses the timers.
struct MINESYS_s {
    nuhspecial_s mine_special; // 0x00
    MINEENTRY_s mines[64];     // 0x0c .. 0x70c
    float mine_radius;         // 0x70c NuSpecialGetOriginRadius(mine special)
    float spawn_timer;         // 0x710 seeded from camera zoom / 1000000000.0f sentinel
    void *nomine_areas[10];    // 0x714 AISysFindArea("nomine_N")
    i16 nomine_count;          // 0x73c
    i16 mine_debris;           // 0x73e FindGameDebris("MINE_POP")
    i32 mine_part;             // 0x740 PARTLookupType("POD_MINE_PART")
    float update_timer;        // 0x744
};

struct MoveToMarker {
    void BlowUp();
    void FadeOut();
    MoveToMarker(MechObjectInterface &);
    void Process(float);
    void Render();
};
struct OccluderRecord {
    NUVEC4 vertices[4];
    NUVEC4 transformed[4];
    f32 min_x, max_x, min_y, max_y, min_depth;
    f32 depth;
};
DECOMP_ASSERT(sizeof(OccluderRecord) == 0x98, "occluder record size");
DECOMP_ASSERT(offsetof(OccluderRecord, depth) == 0x94, "occluder depth offset");
struct OccluderSet {
    OccluderRecord *occluders;
    u32 *indices;
    u32 capacity;
    u32 count;
    bool queries_prepared;
    u8 unknown_11[0x0f];
    NUMTX projection_matrix;
    NUMTX query_matrix;
    static numtl_s *ms_pZOnlyMtl3D;
    static numtl_s *ms_pZOnlyMtl2D;
    static numtl_s *ms_pAlphaMtl2D;
    void AddOccluder(nuvec_s const *, nuvec_s const *, nuvec_s const *, nuvec_s const *);
    void Clear();
    void Init(u32, variptr_u *, variptr_u);
    bool IsOccludedOBB(nuvec_s const *, nuvec_s const *, numtx_s const *);
    bool IsOccludedSphere(nuvec_s const *, float);
    OccluderSet();
    void OnCameraSet();
    void PrepareForQueries(numtx_s const *, numtx_s const *);
    void RenderOccluders(bool) const;
    static i32 SortByDepth(void const *, void const *);
    ~OccluderSet();
};
struct OcclusionManager {
    bool initialized;
    bool enabled;
    u8 unknown_02[0x0e];
    OccluderSet sets[2];
    OccluderSet *building_set;
    OccluderSet *current_set;
    f32 unknown_158;
    f32 unknown_15c;
    u32 unknown_160;
    u32 unknown_164;
    u8 unknown_168[8];
    void AddOccluder(nuvec_s const *, float);
    void AddOccluder(nuvec_s const *, nuvec_s const *, numtx_s const *);
    void AddOccluder(nuvec_s const *, nuvec_s const *, nuvec_s const *, nuvec_s const *);
    void BeginFrame();
    void EndFrame();
    void Init(u32, variptr_u *, variptr_u);
    bool IsOccludedOBB(nuvec_s const *, nuvec_s const *, numtx_s const *);
    bool IsOccludedSphere(nuvec_s const *, float);
    OcclusionManager();
    void OnCameraSet();
    void RenderStats() const;
    void RenderZPass() const;
    void SetEnabled(bool);
    ~OcclusionManager();
};
DECOMP_ASSERT(sizeof(OccluderSet) == 0xa0, "occluder set size");
DECOMP_ASSERT(sizeof(OcclusionManager) == 0x170, "occlusion manager size");
DECOMP_ASSERT(offsetof(OcclusionManager, current_set) == 0x154, "current occluder set offset");
extern OcclusionManager g_OcclusionManager;
struct PART_s {
    union {
        NUMTX transform;
        struct {
            u8 pad_000[0x30];
            NUVEC position;
            u32 field_03c;
        };
    };
    NUMTX previous_transform;
    NUVEC velocity;
    NUVEC bounds_min;
    NUVEC bounds_max;
    NUVEC impact_position;
    NUVEC impact_normal;
    u8 pad_0bc[0xc8 - 0xbc];
    NUVEC initial_position;
    GameObject_s *owner;
    GameObject_s *recipient; // 0xd8
    f32 target_radius;
    f32 radius;
    f32 field_0e4;
    f32 gravity;
    f32 field_0ec, field_0f0, elapsed, time_step, scale_time, field_100, field_104;
    u32 flags;
    u32 field_10c[6];
    i32 field_124[5]; // ordered angular velocities, then two random spin rates
    u16 rotation_x, rotation_y;
    u16 field_13c;
    union {
        i16 pickup_type;
        i16 type_id;
    }; // 0x13e
    u16 field_140;
    u8 rotation_axis_1, rotation_axis_2;
    u8 active;
    u8 render_flags;
    u8 reflection_flags;
    u8 pad_147;
    nuhspecial_s special;
    nuhspecial_s *source_special;
    NUVEC lighting[7];
    void (*field_1ac)(PART_s *);
    void (*move_callback)(PART_s *, f32);
    void (*update_callback)(PART_s *);
    void (*field_1b8)(PART_s *);
    void (*field_1bc)(PART_s *);
    void (*field_1c0)(PART_s *, i32);
    void (*field_1c4)(PART_s *);
    void (*stop_callback)(PART_s *);
    void (*replace_callback)(PART_s *);
    i32 (*draw_callback)(PART_s *);
    i32 debris_handle;
    i32 *debris_key;
    i32 *lighting_template;
    i32 field_1e0, field_1e4;
    f32 field_1e8, field_1ec;
    i32 field_1f0;
    i32 field_1f4;
    i32 field_1f8;
    f32 field_1fc;
    u32 field_200;
    u16 field_204;
    i8 force_player_mask;
    i8 field_207;
    u8 field_208, field_209, field_20a;
    i8 field_20b;
    union {
        u32 field_20c;
        f32 crate_spawn_delay;
    };
    union {
        f32 field_210;
        GIZMOBLOWUP_s *grabber_blowup; // 0x210, grabber release callback data
    };
    f32 field_214;
    union {
        u32 force_flags;
        GIZMOBLOWUP_s *carried_blowup; // 0x218, thrown-object callback data
    };
    union {
        u32 field_21c;
        f32 reflection_height;
    };
    union {
        u32 field_220;
        PartObjectInterface *mech_object_interface;
    };
    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(PART_s) == 0x224, "PART size");
DECOMP_ASSERT(offsetof(PART_s, grabber_blowup) == 0x210, "PART grabber callback data offset");
DECOMP_ASSERT(offsetof(PART_s, field_1c0) == 0x1c0, "PART kill callback offset");
DECOMP_ASSERT(offsetof(PART_s, debris_key) == 0x1d8, "PART debris key offset");
DECOMP_ASSERT(offsetof(PART_s, lighting_template) == 0x1dc, "PART dynamic handle offset");
DECOMP_ASSERT(offsetof(PART_s, field_1f4) == 0x1f4, "PART death effect offset");
DECOMP_ASSERT(offsetof(PART_s, field_1fc) == 0x1fc, "PART death effect scale offset");
DECOMP_ASSERT(offsetof(PART_s, position) == 0x30, "PART position offset");
DECOMP_ASSERT(offsetof(PART_s, velocity) == 0x80, "PART velocity offset");
DECOMP_ASSERT(offsetof(PART_s, owner) == 0xd4, "PART owner offset");
DECOMP_ASSERT(offsetof(PART_s, flags) == 0x108, "PART flags offset");
DECOMP_ASSERT(offsetof(PART_s, active) == 0x144, "PART active offset");
DECOMP_ASSERT(offsetof(PART_s, radius) == 0xe0, "PART radius offset");
DECOMP_ASSERT(offsetof(PART_s, special) == 0x148, "PART special offset");
DECOMP_ASSERT(offsetof(PART_s, gravity) == 0xe8, "PART gravity offset");
DECOMP_ASSERT(offsetof(PART_s, move_callback) == 0x1b0, "PART move callback offset");
DECOMP_ASSERT(offsetof(PART_s, force_player_mask) == 0x206, "PART Force player mask offset");
DECOMP_ASSERT(offsetof(PART_s, force_flags) == 0x218, "PART Force flags offset");
DECOMP_ASSERT(offsetof(PART_s, carried_blowup) == 0x218, "PART carried blowup offset");
struct PartObjectInterface : MechObjectInterface {
    PART_s &part;

    void GetPos(VuVec &, i32) const override;
    f32 GetRadius() const override;
    const char *GetTargetName() const override;
    i32 GetObjectType() const override {
        return 12;
    }
    void *GetTgtVoidPtr() override {
        return &part;
    }
    PART_s *GetPart() override {
        return &part;
    }
    PartObjectInterface(PART_s &);
    ~PartObjectInterface() override;
};
DECOMP_ASSERT(sizeof(PartObjectInterface) == 0xc, "Part interface ABI");
struct Placeable {
    void Reset();
    void GetCurrentPosition() const;
    void GetInitialPosition() const;
    void SetCurrentPosition(VuVec const *);
    void SetInitialPosition(VuVec const *);
};
struct PlaceableHelper {
    i32 object_type_count;

    void Find(char *);
    void Find(char *, Placeable **, i32);
    void FindObject(char *);
    void GetNextObject(void *);
    void GetNextObject(void *, i32 (*)(void *));
    void Initialise();
    void IsEditorObject(ClassObject &);
    PlaceableHelper();
    void RegisterObjectType(char *, PlaceableInterface *);
};
struct PlaceableInterface {
    void DebugOutputObjects();
    void Find(char *);
    void Find(char *, Placeable **, i32);
};
struct PlaceableNameControl {
    void AddMenuItem(eduimenu_s *, EdRef *, void *);
    PlaceableNameControl();
    void Process(EdInputContext &);
    void Render();
    void cbButton(eduimenu_s *, eduiitem_s *, u32);
    void cbChanged(eduimenu_s *, eduiitem_s *, u32);
    void cbSelectObject(eduimenu_s *, eduiitem_s *, u32);
};
// Pod race net packet (podrace_netpacket).
struct PODRACENETPACKET_s {
    char pad_0x00[0xc];
    float countdown; // 0x0c
};

// One AI spline slot inside the pod sprint state. Type name from the original
// local symbol _ZL22PodSprint_InitAISplineP11WORLDINFO_sP20PODSPRINT_AISPLINE_sPc.
struct PODSPRINT_AISPLINE_s {
    struct nugspline_s *spline; // 0x00
    i16 vals[6];                // 0x04
};

// Pod sprint state (podsprint, 0x94 bytes).
struct PODSPRINT_s {
    struct nugspline_s *finish_line; // 0x00
    struct nugspline_s *halfway;     // 0x04
    PODSPRINT_AISPLINE_s ai[6];      // 0x08
    GIZAIMESSAGE_s *lap_msg;         // 0x68
    GIZAIMESSAGE_s *max_speed_msg;   // 0x6c
    GIZAIMESSAGE_s *min_speed_msg;   // 0x70
    GIZAIMESSAGE_s *speed_step_msg;  // 0x74
    void *field_0x78;                // 0x78 pacemaker/boulders object (dword write in the original)
    void *boulders;                  // 0x7c  AIArea from AISysFindArea("Boulders")
    float field_0x80;                // 0x80 pacemaker alpha
    float speed;                     // 0x84 sebulba speed (integer part drives the lap lights)
    float field_0x88;                // 0x88 second speed channel (sent as net->speed2)
    char pad_0x8c[0x8e - 0x8c];
    u8 ai_state; // 0x8e
    u8 ai_index; // 0x8f signed; index into PODSPRINT_AISPLINE_s::vals
    char pad_0x90[0x92 - 0x90];
    u8 flags; // 0x92
    char pad_0x93[0x94 - 0x93];
};

// Pod-sprint network packet (podsprint_netpacket).
struct PODSPRINTNETPACKET_s {
    i16 ai_state; // 0x00
    char pad_0x02[0x6 - 0x2];
    i16 speed;  // 0x06
    i16 speed2; // 0x08
};

struct PropertyMenu {
    PropertyMenu *next;
    PropertyMenu *previous;
    eduimenu_s *menu;
    EdControl *control;
    ClassObject objects[8];
    i32 object_count;

    void AddObject(ClassObject &);
    void ClearObjecs();
    bool ContainsObject(ClassObject &);
    bool ContainsObject(void *);
    void Destroy();
    void SelectAttr(i32);
};
struct PropertyTool {
    u8 reserved_0x00[0xc];
    PropertyMenu *active_menu;

    void AddPropertyMenuItems(eduimenu_s *, EdClass *, void *, eduiitem_s *);
    void AutoLocateMenu(PropertyMenu *);
    void BringToFront(PropertyMenu *);
    void CreatePropertyMenu(ClassObject &);
    void FindItemMenu(PropertyMenu *, ClassItem *);
    void GetActiveMenu(PropertyMenu *);
    void GetClassName(EdRef *, char *);
    void GetNextActiveMenu();
    void GetNextDefaultActiveMenu(eduimenu_s *);
    void GetTypeName(EdRef *, char *);
    bool HasActiveMenu();
    void Initialise(variptr_u &, variptr_u &, i32);
    void Process(EdInputContext &);
    void ProcessControls(EdInputContext &);
    void ProcessMenu(EdInputContext &);
    PropertyTool();
    void RefreshMenuControls(PropertyMenu *);
    void Render();
    void RenderMenu(PropertyMenu *);
    void RetrievePropertyMenu(ClassObject *, PropertyMenuList *);
    void SelectAttr(i32);
    void SetDefaultActiveMenu(PropertyMenu *);
    void SetMenuControl(eduimenu_s *, EdControl *);
    void ToggleActiveMenu();
    void ediGetMenuStartMetrics();
    void ediMenuRetrieveMetrics(eduimenu_s *);
    void ediMenuStoreMetrics(eduimenu_s *);
};
// Retake-G network packet (retakeg_netpacket).
struct RETAKEGNETPACKET_s {
    i16 guard_a; // 0x00
    i16 guard_b; // 0x02
};

// Pod-race sniper entry (PodRace_snipers[5], 0x40 bytes each). The first two
// vectors overlay consecutive spline points; the position is copied from the
// sniper's spline via a 0x18-byte copy.
struct SNIPER_s {
    NUVEC prev;       // 0x00 first spline point (unused)
    NUVEC pos;        // 0x0c actual sniper position
    float fire_timer; // 0x18
    float state;      // 0x1c
}; // 0x20 bytes: the original strides this array by 0x20

struct SceneInstance {
    u8 reserved_0x00[0x34];
    NUMTX initial_transform;
    NUMTX current_transform;
    i32 visibility;

    VuVec const *GetCurrentPosition() const;
    VuMtx const *GetCurrentTransform() const;
    VuVec const *GetInitialPosition() const;
    VuMtx const *GetInitialTransform() const;
    i32 GetVisibility() const;
    void Render(VuMtx const *) const;
    SceneInstance();
    void SetCurrentPosition(VuVec const *);
    void SetCurrentTransform(VuMtx const *);
    void SetInitialPosition(VuVec const *);
    void SetInitialTransform(VuMtx const *);
    void SetVisibility(i32);
};
struct SceneObject {
    void Clone(i32) const;
    SceneObject();
};
struct SceneObjectHelper {
    u8 reserved_0x00[0x6c];
    i32 scene_object_count;
    u8 reserved_0x70[0x30];
    i32 owned_object_count;

    void AddMenuItems(eduimenu_s *);
    void ClearLevel(i32);
    void CreateObject(void *, i32, i32);
    void DestroyObject(void *, i32);
    void Flush();
    void GetNextObject(void *);
    i32 GetNumObjects();
    void Initialise();
    void PostLoadInitialisation(MemoryBuffer *, MemoryBuffer *);
    void PreLoadInitialisation(MemoryBuffer *, MemoryBuffer *);
    void SetSceneFilter(char *);
    void SubRender();
    void UpdateLists(MemoryBuffer *, MemoryBuffer *);
    void cbEdSceneObjectShowHiddenSolid(eduimenu_s *, eduiitem_s *, u32);
    void cbEdSceneObjectShowHiddenWire(eduimenu_s *, eduiitem_s *, u32);
    void cbEdSceneObjectShowOwnedObjects(eduimenu_s *, eduiitem_s *, u32);
};

struct SpecialObject {
    void Exists() const;
    void GetCollision() const;
    void GetCurrentPosition() const;
    void GetCurrentTransform() const;
    void GetInitialPosition() const;
    void GetInitialTransform() const;
    void GetMtl(i32) const;
    void GetName() const;
    void GetNumMtls() const;
    void GetRadius() const;
    void GetVisibility() const;
    void Render(VuMtx const *) const;
    void SetCollision(i32);
    void SetCurrentPosition(VuVec const *);
    void SetCurrentTransform(VuMtx const *);
    void SetInitialPosition(VuVec const *);
    void SetInitialTransform(VuMtx const *);
    void SetVisibility(i32);
    SpecialObject();
};
struct TELEPORT_s {
    char name[0x40];
    struct nugspline_s *path;
    f32 duration;
    f32 range_squared;
    u16 flags;
    u8 enabled;
    u8 active;
    nuhspecial_s blocking_special;
    nuhspecial_s flap1_special;
    nuhspecial_s flap2_special;
    u16 field_74;
    u16 field_76;
    u16 field_78;
    u16 field_7a;
    NUMTX flap1_matrix;
    NUMTX flap2_matrix;
    TeleportObjectInterface *mech_object_interface;
    void ClearMechObjectInterface();
    MechObjectInterface *GetMechObjectInterface();
};
DECOMP_ASSERT(sizeof(TELEPORT_s) == 0x100, "TELEPORT_s size");
struct TMClient {
    struct TM_MOUSE_AXIS {};
    void AllocHandle();
    void Connect();
    void FClose(i32);
    void FOpen(char const *, char const *);
    void FRead(void *, u32, u32, i32);
    void FSeek(i32, i64, i32);
    void FTell(i32);
    void FWrite(void const *, u32, u32, i32);
    void FlushKeyBuffer();
    void GetKey(i32 *);
    void GetMouseAxis(TMClient::TM_MOUSE_AXIS);
    void GetMouseButtons();
    void SendTTY(char const *, i32);
    void TestKey(i32);
};
struct TTNetwork {
  private:
    u8 reserved_04[0x2168];
    NetAddress my_address;
    NetAddress my_host_address;
    i32 has_my_host_address;

  public:
    void Broadcast(NetMessage, unsigned char);
    void ClearMyHostAddress();
    void Display(ThingRenderData *);
    const NetAddress &GetMyAddress() const;
    const NetAddress *GetMyHostAddress() const;
    void Initialise();
    void ProcessEvenWhenPaused(ThingProcessData *);
    void ReliableBroadcast(NetMessage, unsigned char);
    void ReliableSend(NetMessage, unsigned char, NetPeer &, char const *, u32);
    void Resume();
    void Send(NetMessage, unsigned char, NetPeer &);
    void SetMyHostAddress(NetAddress const &);
    void Shutdown();
    bool Suspend();
    TTNetwork();
    void Update();
    virtual ~TTNetwork();
};
// All ThingManager methods are virtual in the original: its vtable order is
// D2, D0, AddThing, AddThingAfterThis, RemoveTemporaryThings,
// RemoveDependanciesThings, ResetThings, EnterLevelThings, ExitLevelThings,
// ProcessThings (vptr+0x24), RenderThings (vptr+0x28), DisplayThings
// (vptr+0x2c), EffectsThings (vptr+0x30). NuMain and PanelRender dispatch
// through these slots, so the declarations must carry the same order.
struct ThingManager {
    virtual ~ThingManager();
    virtual void AddThing(BaseThing *);
    virtual void AddThingAfterThis(BaseThing *);
    virtual void RemoveTemporaryThings();
    virtual void RemoveDependanciesThings(ThingRemoveData *);
    virtual void ResetThings(ThingResetData *);
    virtual void EnterLevelThings(ThingLevelData *);
    virtual void ExitLevelThings(ThingLevelData *);
    virtual void ProcessThings(ThingProcessData *);
    virtual void RenderThings(ThingRenderData *);
    virtual void DisplayThings(ThingRenderData *);
    virtual void EffectsThings(ThingRenderData *);
    void EnableActions(i32, i32, i32);
    ThingManager(i32);
    void cbEdTimingSelect(eduimenu_s *, eduiitem_s *, u32);
    void cbEdTrackCancel(eduimenu_s *, eduimenu_s *);
    void edTimingEnter();
    void edTimingInit();
    void edTimingProc(float, nupad_s *);
    void edTimingRender();

    // data (object is 0x24 bytes; the ctor carves `things` from theMemoryManager)
    BaseThing **things; // 0x04
    i32 max_things;     // 0x08
    i32 count;          // 0x0c
    u32 field_0x10;     // 0x10 high-water cursor (written by the ctor / AllocPool)
    i32 field_0x14;     // 0x14 AddThingAfterThis reservation, folded in by the next AddThing
    i32 timebar;        // 0x18 NuTimeBarCreateSet index
    u32 field_0x1c;
    i32 ed_timing_state; // 0x20 editor timing selection state
};
DECOMP_ASSERT(sizeof(ThingManager) == 0x24, "ThingManager ABI");
DECOMP_ASSERT(offsetof(ThingManager, ed_timing_state) == 0x20, "ThingManager timing state offset");
// GameThingManager shares the base vtable entries (only the dtors differ) and
// registers itself in theGameThings (ctor @0x4e8b00 / D1 dtor @0x4e8a80).
struct GameThingManager : ThingManager {
    void AddLevelOnlyThings();
    void AddOnceOnlyThings();
    GameThingManager(i32);
    virtual ~GameThingManager();
};
struct VirtualControlButton {
    void Process(float);
    void Render();
    VirtualControlButton(NuVec2 const &, float, MechInputTouchMainController::eButtonTypes);
};
struct VirtualControlButtonMover {
    void Process(float);
    VirtualControlButtonMover(MechInputTouchVirtualConsoleController &);
};
struct VirtualControlDPad {
    void Process(float);
    void Render();
    VirtualControlDPad(NuVec2 const &, float, MechInputTouchVirtualConsoleController &);
    virtual ~VirtualControlDPad();
};
struct VirtualControlDPad_LockButton {
    void Process(float);
    void Render();
    VirtualControlDPad_LockButton(VirtualControlDPad &);
};

#endif // LEGOAPI_TYPES_H
