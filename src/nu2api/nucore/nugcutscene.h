#pragma once

#include "decomp.h"
#include "nu2api/nucore/nuanim3.h"
#include "nu2api/numath/numtx.h"

struct nugscn_s;
typedef struct nugscn_s NUGSCN;
struct NUGCUTRIGID_s;
struct instNUGCUTSCENE_s;

struct NUGCUTLOCATOR_s {
    NUMTX base_matrix;
    NUVEC pivot;
    f32 locator_scale;
    nuanimdata2_s *animation;
    u32 field_54;
    u8 flags;
    u8 type_index;
    u8 field_5a;
    u8 field_5b;
    u8 pad_5c[8];
};

struct NUGCUTLOCATORTYPE_s {
    char *name;
    u8 flags;
    u8 field_05;
    u16 function_index;
    u32 field_08;
};

struct NUGCUTLOCATORSYS_s {
    NUGCUTLOCATOR_s *locators;
    NUGCUTLOCATORTYPE_s *types;
    u8 locator_count;
    u8 type_count;
    u8 pad_0a[2];
};

struct instNUGCUTLOCATOR_s {
    u32 field_00;
    i32 effect_handle;
};

typedef void (*NUGCUTLOCATORFN)(instNUGCUTSCENE_s *, NUGCUTLOCATORSYS_s *, instNUGCUTLOCATOR_s *, NUGCUTLOCATOR_s *,
                                f32, NUMTX *, i32);

struct NUGCUTLOCATORFNENTRY_s {
    const char *name;
    i32 field_04;
    i32 field_08;
    i32 field_0c;
    NUGCUTLOCATORFN function;
};

struct instNUGCUTLOCATORSYS_s {
    instNUGCUTLOCATOR_s *locators;
};

struct StateAnim {
    u16 count;
    u16 unused;
    f32 *times;
    u8 *values;
};

struct NUGCUTRIGIDSYS_s {
    NUGCUTRIGID_s *rigids;
    u16 count;
    u16 unused;
};

struct NUGCUTCAMERA_s {
    NUMTX base_matrix;
    u8 flags;
    u8 field_41;
    u8 animation_node;
    u8 field_43;
    u8 pad_44[0x50 - 0x44];
};

struct NUGCUTCAMERASYS_s {
    u32 camera_count;
    NUGCUTCAMERA_s *cameras;
    nuanimdata2_s *animation;
    StateAnim *state_animation;
    u8 field_10;
    i8 target_camera_map[0x0b];
    nuanimdata2_s *focus_animation;
    StateAnim *focus_state_animation;
};

struct NUGCUTRIGID_s {
    NUMTX base_matrix;
    char *name;
    NUGSCN *scene;
    void *special_object;
    nuanimdata2_s *animation;
    StateAnim *state_animation;
    usize locator;
    u8 flags;
    u8 locator_count;
    u8 locator_index;
    u8 field_5b;
};

struct NUGCUTCHAR_s {
    NUMTX base_matrix;
    union {
        char *name;
        char *model_file;
    };
    nuanimdata2_s *animation;
    nuanimdata2_s *face_animation;
    nuanimdata2_s *extra_animation;
    union {
        void *character_model;
        void *character;
    };
    NUGCUTLOCATOR_s *locator;
    f32 animation_rate;
    u8 flags;
    u8 animation_index;
    union {
        u8 locator_count;
        u8 has_locator;
    };
    u8 field_5f;
    u8 locator_index;
    u8 blend_time;
    u8 field_62;
    i8 animation_start_frame;
};

struct NUGCUTCHARSYS_s {
    NUGCUTCHAR_s *characters;
    u16 character_count;
    u8 reserved_06[0x14 - 0x06];
};

struct instNUGCUTCHAR_s {
    union {
        void *character_model;
        void *character;
    };
    union {
        i32 field_04;
        f32 blend_progress;
    };
    u32 field_08;
    f32 animation_frame_a;
    f32 animation_frame_b;
    u8 field_14;
    u8 field_15;
    u8 field_16;
    u8 field_17;
};

struct instNUGCUTCHARSYS_s {
    instNUGCUTCHAR_s *characters;
};

struct NUGCUTCHARANIM_s {
    u32 field_00;
    nuanimdata2_s *animation;
    u8 pad_08[8];
};

struct NUGCUTTRIGGEREVENT_s {
    i32 field_00;
    void *field_04;
    StateAnim *state_animation;
};

struct NUGCUTTRIGGERSYS_s {
    i32 event_count;
    NUGCUTTRIGGEREVENT_s *events;
};

struct instNUGCUTTRIGGERSYS_s {
    void *owner;
    u32 *event_states;
};

struct NUGCUTSCENE_s {
    i32 version;
    isize string_delta;
    f32 duration;
    char *strings;
    NUGCUTCAMERASYS_s *camera_system;
    NUGCUTRIGIDSYS_s *rigid_system;
    NUGCUTCHARSYS_s *character_system;
    NUGCUTLOCATORSYS_s *locator_system;
    void *bounds;
    NUGCUTTRIGGERSYS_s *trigger_system;
    isize relocation_delta;
    char *filename;
    NUGCUTSCENE_s *stream_buffer_0;
    NUGCUTSCENE_s *stream_buffer_1;
    u32 flags;
    i32 stream_buffer_size;
    union {
        u32 field_40;
        struct {
            u8 last_stream;
            u8 pad_41;
            u16 total_stream_frames;
        };
    };
    NUGSCN *scene;
    void *extra_scene;
    u32 field_4c;
    NUGCUTCHARANIM_s *character_animations;
    u16 *focus_camera_indices;
    i32 loaded_size;
};

struct instNUGCUTRIGID_s {
    NUGSCN *scene;
    void *special;
    void *display_special;
    u8 state_index;
    u8 visible;
    u8 pad_0e[2];
};

struct instNUGCUTRIGIDSYS_s {
    instNUGCUTRIGID_s *rigids;
};

struct instNUGCUTCAMTGT_s {
    NUVEC *target;
    f32 start_frame;
    f32 duration;
    i8 target_index;
    u8 pad_0d[3];
};

struct instNUGCUTCAMSTATE_s {
    u8 flags;
    u8 event_index;
    u8 pad_02[2];
};

struct instNUGCUTSCENECAMERA_s {
    instNUGCUTCAMTGT_s *targets;
    instNUGCUTCAMSTATE_s *camera_states;
    u8 state_index;
    i8 camera_index;
    u8 next_target_index;
    u8 target_capacity;
    u8 target_count;
    u8 focus_state_index;
    i8 focus_index;
    u8 field_0f;
};

struct instNUGCUTSCENE_s {
    instNUGCUTSCENE_s *next;
    instNUGCUTSCENE_s *previous;
    char name[0x10];
    NUMTX matrix;
    NUGCUTSCENE_s *cutscene;
    NUGCUTSCENE_s *cutscene_copy;
    NUVEC transformed_bounds_center;
    NUVEC bounds_min;
    NUVEC bounds_max;
    f32 max_camera_distance_squared;
    union {
        u8 flags_88;
        struct {
            u8 flags_88_low : 2;
            u8 paused : 1;
            u8 flags_88_high : 5;
        };
    };
    u8 flags_89;
    u8 flags_8a;
    u8 flags_8b;
    u8 flags_8c;
    u8 flags_8d;
    u8 pad_8e[2];
    f32 current_frame;
    f32 render_frame;
    f32 rate;
    instNUGCUTSCENECAMERA_s *camera_instance;
    instNUGCUTRIGIDSYS_s *rigid_instance;
    instNUGCUTCHARSYS_s *character_instance;
    instNUGCUTLOCATORSYS_s *locator_instance;
    instNUGCUTTRIGGERSYS_s *trigger_instance;
    instNUGCUTSCENE_s *chained_instance;
    void (*end_callback)(instNUGCUTSCENE_s *);
    u32 field_b8;
    NUGCUTSCENE_s *stream_buffer_0;
    NUGCUTSCENE_s *stream_buffer_1;
    void *field_c4;
    void *field_c8;
    u8 pad_cc[0xd8 - 0xcc];
    i16 skip_countdown;
    u8 pad_da[0xdf - 0xda];
    u8 stream_index;
    f32 accumulated_stream_duration;
    f32 elapsed;
    f32 alpha;
    void *pending_stream_buffer;
    i32 allocation_size;
    instNUGCUTSCENE_s *queued_stream_instance;
};

DECOMP_ASSERT(offsetof(NUGCUTSCENE_s, stream_buffer_size) == 0x3c, "NUGCUTSCENE stream-buffer size offset");
DECOMP_ASSERT(offsetof(instNUGCUTSCENE_s, stream_buffer_0) == 0xbc, "cutscene instance stream-buffer 0 offset");
DECOMP_ASSERT(offsetof(instNUGCUTSCENE_s, stream_buffer_1) == 0xc0, "cutscene instance stream-buffer 1 offset");
DECOMP_ASSERT(offsetof(instNUGCUTSCENE_s, stream_index) == 0xdf, "cutscene instance stream index offset");
DECOMP_ASSERT(offsetof(instNUGCUTSCENE_s, pending_stream_buffer) == 0xec,
              "cutscene instance pending stream-buffer offset");
DECOMP_ASSERT(sizeof(instNUGCUTSCENE_s) == 0xf8, "cutscene instance size");

typedef void (*NUGCUTSCENECHARACTERCREATEDATAFN)(NUGCUTCHAR_s *, instNUGCUTCHAR_s *, variptr_u *);
typedef NUGSCN *(*NUGCUTSCENEGETHGOBJFN)(instNUGCUTSCENE_s *, i32);
typedef void (*NUGCUTSCENECHARACTERDESTROYDATAFN)(NUGCUTCHAR_s *, instNUGCUTCHAR_s *);
typedef void (*NUGCUTSCENECHARACTEREVALFN)(instNUGCUTSCENE_s *, NUGCUTSCENE_s *, instNUGCUTCHAR_s *, NUGCUTCHAR_s *,
                                           f32);
typedef void (*NUGCUTSCENECHARACTERRELEASEFN)(instNUGCUTCHAR_s *, NUGCUTCHAR_s *);
typedef void (*NUGCUTSCENECHARACTERPROCESSFN)(instNUGCUTSCENE_s *, NUGCUTSCENE_s *, instNUGCUTCHAR_s *, NUGCUTCHAR_s *,
                                              f32, i32);
typedef void (*NUGCUTSCENECHARACTERRENDERFN)(instNUGCUTSCENE_s *, NUGCUTSCENE_s *, instNUGCUTCHAR_s *, NUGCUTCHAR_s *,
                                             f32, i32);
typedef void (*NUGCUTSCENEFINDCHARACTERSFN)(NUGCUTSCENE_s *);
typedef void (*NUGCUTSCENERESETCHARACTERSFN)(instNUGCUTSCENE_s *);
typedef void (*NUGCUTSCENERIGIDPOSTRENDERFN)(NUGCUTRIGID_s *, instNUGCUTRIGID_s *, NUMTX *);
typedef void (*NUGCUTSCENERIGIDCOLLISIONCHECKFN)(NUGCUTRIGID_s *, NUMTX *);
extern "C" NUGCUTSCENERIGIDCOLLISIONCHECKFN NuCutSceneRigidCollisionCheck;
extern "C" void NuSetCutSceneRigidCollisionCheckFn(NUGCUTSCENERIGIDCOLLISIONCHECKFN callback);
typedef void (*NUGCUTSCENEREQUESTSFXFN)(instNUGCUTSCENE_s *);
typedef i32 (*NUGCUTSCENESFXFIXUPFN)(usize);
typedef void (*NUGCUTSCENESFXUPDATEFN)(NUGCUTLOCATORSYS_s *, instNUGCUTLOCATOR_s *, NUGCUTLOCATOR_s *, f32, NUMTX *,
                                       i32);

extern "C" NUGCUTSCENECHARACTERCREATEDATAFN NuCutSceneCharacterCreateData;
extern "C" NUGCUTSCENEGETHGOBJFN NuCutSceneGetHGObj;
extern "C" NUGCUTSCENECHARACTERDESTROYDATAFN NuCutSceneCharacterDestroyData;
extern "C" NUGCUTSCENECHARACTEREVALFN NuCutSceneCharacterEval;
extern "C" NUGCUTSCENECHARACTERRELEASEFN NuCutSceneCharacterRelease;
extern "C" NUGCUTSCENECHARACTERPROCESSFN NuCutSceneCharacterProcess;
extern "C" NUGCUTSCENECHARACTERRENDERFN NuCutSceneCharacterRender;
extern "C" NUGCUTSCENEFINDCHARACTERSFN NuCutSceneFindCharacters;
extern "C" NUGCUTSCENERESETCHARACTERSFN NuCutSceneResetCharactersFn;
extern "C" void (*NuCutSceneDestroyCharacters)(NUGCUTSCENE_s *);
extern "C" void NuSetCutSceneDestroyCharactersFn(void (*callback)(NUGCUTSCENE_s *));
extern "C" NUGCUTSCENERIGIDPOSTRENDERFN NuCutSceneRigidPostRender;
extern "C" NUGCUTSCENEREQUESTSFXFN NuCutSceneRequestSFX;
extern "C" NUGCUTSCENESFXFIXUPFN NuCutSceneSFXFixUp;
extern "C" NUGCUTSCENESFXUPDATEFN NuCutSceneSFXUpdate;
extern "C" void instNuGCutSceneSetMtx(instNUGCUTSCENE_s *instance, NUMTX *matrix);

extern "C" void NuSetCutSceneCharacterCreateDataFn(NUGCUTSCENECHARACTERCREATEDATAFN function);
extern "C" void NuSetGetHGObjFromIndxFn(NUGCUTSCENEGETHGOBJFN function);
extern "C" void NuSetCutSceneCharacterDestroyDataFn(NUGCUTSCENECHARACTERDESTROYDATAFN function);
extern "C" void NuSetCutSceneCharacterEvalFn(NUGCUTSCENECHARACTEREVALFN function);
extern "C" void NuSetCutSceneCharacterReleaseFn(NUGCUTSCENECHARACTERRELEASEFN function);
extern "C" void NuSetCutSceneCharacterProcessFn(NUGCUTSCENECHARACTERPROCESSFN function);
extern "C" void NuSetCutSceneCharacterRenderFn(NUGCUTSCENECHARACTERRENDERFN function);
extern "C" void NuSetCutSceneFindCharactersFn(NUGCUTSCENEFINDCHARACTERSFN function);
extern "C" void NuSetCutSceneResetCharactersFn(NUGCUTSCENERESETCHARACTERSFN function);
extern "C" void NuSetCutSceneRigidPostRenderFn(NUGCUTSCENERIGIDPOSTRENDERFN function);
extern "C" void NuSetCutSceneRequestSFXFn(NUGCUTSCENEREQUESTSFXFN function);
extern "C" void NuSetCutSceneSFXFixUpFn(NUGCUTSCENESFXFIXUPFN function);
extern "C" void NuSetCutSceneSFXUpdateFn(NUGCUTSCENESFXUPDATEFN function);
void NuGCutSceneRemapFocusIdToLocaterNum(NUGCUTSCENE_s *cutscene, VARIPTR *buffer);
extern "C" void instNuGCutSceneCreateCamTgtArray(instNUGCUTSCENE_s *instance, i32 count, VARIPTR *buffer);
extern "C" i32 instNuGCutSceneAddCamTgt(instNUGCUTSCENE_s *instance, NUVEC *target, f32 start_frame, f32 duration,
                                        i8 target_index);
extern "C" void NuGCutCharAnimProcess(NUGCUTCHAR_s *character, f32 frame, NUMTX *matrix, i32 *visible,
                                      u32 *animation_index, f32 *animation_rate, f32 *blend_time,
                                      f32 *animation_start_frame, i32 *layer_mask);
void NuGCutCharAnimProcess_3(NUGCUTCHAR_s *character, f32 frame, NUMTX *matrix, i32 *visible, u32 *animation_index,
                             f32 *animation_rate, f32 *blend_time, f32 *animation_start_frame, i32 *layer_mask);

DECOMP_ASSERT(sizeof(StateAnim) == 0x0c, "StateAnim must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTLOCATOR_s) == 0x64, "NUGCUTLOCATOR_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTLOCATORTYPE_s) == 0x0c, "NUGCUTLOCATORTYPE_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTLOCATORSYS_s) == 0x0c, "NUGCUTLOCATORSYS_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(instNUGCUTLOCATOR_s) == 0x08, "instNUGCUTLOCATOR_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTLOCATORFNENTRY_s) == 0x14, "NUGCUTLOCATORFNENTRY_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTRIGID_s) == 0x5c, "NUGCUTRIGID_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTCHAR_s) == 0x64, "NUGCUTCHAR_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTCAMERA_s) == 0x50, "NUGCUTCAMERA_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTCAMERASYS_s) == 0x24, "NUGCUTCAMERASYS_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTCHARSYS_s) == 0x14, "NUGCUTCHARSYS_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(instNUGCUTCHAR_s) == 0x18, "instNUGCUTCHAR_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(NUGCUTCHARANIM_s) == 0x10, "NUGCUTCHARANIM_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(instNUGCUTSCENE_s) == 0xf8, "instNUGCUTSCENE_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(instNUGCUTCAMTGT_s) == 0x10, "instNUGCUTCAMTGT_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(instNUGCUTCAMSTATE_s) == 0x04, "instNUGCUTCAMSTATE_s must match the original x86 layout");
DECOMP_ASSERT(sizeof(instNUGCUTSCENECAMERA_s) == 0x10, "instNUGCUTSCENECAMERA_s must match the original x86 layout");
