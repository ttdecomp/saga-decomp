#include "gameapi/edtools/edgra.h"
#include "gameapi/edtools/edui.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/numouse.h"
#include "nu2api/nucore/nukeyboard.h"
#include "gameapi/edtools/gameapi_edtools_types.h"
#include "gameapi/edtools/edfile.h"
#include "gameapi/edtools/edcam.h"
#include "gameapi/edtools/edstubs.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nutime.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/nucore/nuvideo.h"

#include <string.h>
#include <math.h>

float edanimPlayerAnimDistance(i32 parameter_index);
extern u8 object_switches[0x80];
extern "C" i32 NuRndrDoingScreenGrab;
static i32 edbits_cubecount;
static i32 edgra_clumpthin = 1;
static i32 edgra_elementthin = 1;
static VARIPTR gra_ptr;
static VARIPTR gra_end;
static i32 editor_return;
static i32 bShowCursor = 1;
static i32 edui_font;
static f32 edui_font_scale_x = 0.9f;
static f32 edui_font_scale_y = 0.9f;
static u32 edui_cursor_colour = 0xff000000;
extern "C" {
    typedef void (*EDBITSPLAYSOUNDCALLBACK)(NUVEC *, i32);
    typedef i32 (*EDBITSREQUESTSOUNDCALLBACK)(char *);

    i32 bCameraEnabled = 1;
    EDBITSPLAYSOUNDCALLBACK edbitsPlaySound;
    EDBITSREQUESTSOUNDCALLBACK edbitsRequestSound;
    eduimenu_s *edui_messagemenu;
    edgra_clump_s *GrassClumps;
    i32 EDGRA_MAX_CLUMPS;
    i32 EDGRA_MAX_UNITS_PER_INDIVIDUAL_CLUMP;
    edgra_individual_s *IndGrassClumps;
    void *edgra_free_vecbuffer;
    i32 edgra_clumps_used;
    i32 *IndGrassClumpsUsed;
    i32 edgra_ind_clumps_used;
    void edgraInitAllClumps(void);
}

void edgraClumpReseed(i32 index) {
    edgra_clump_s *clump = &GrassClumps[index];
    clump->seed = NuRand(NULL);
    edgraInitAllClumps();
}

edgra_individual_s *GetIndGrassClump(i32 clump, i32 element) {
    return &IndGrassClumps[clump * EDGRA_MAX_UNITS_PER_INDIVIDUAL_CLUMP + element];
}

void edgraClumpDestroy(i32 index) {
    edgra_clump_s *clump = &GrassClumps[index];
    clump->element_count = 0;
    edgra_free_vecbuffer = clump->vector_buffer;
    clump->vector_buffer = NULL;
    --edgra_clumps_used;
    if (clump->kind == 3) {
        IndGrassClumpsUsed[clump->individual_index] = 0;
        --edgra_ind_clumps_used;
    }
    edgraInitAllClumps();
}
void edbitsDoSingleDump(i32 face);
void edbriBridgeUpdate(i32 bridge, NUGSCN *scene);

struct edbridge_s {
    i32 instance_id;
    NUVEC position;
    f32 length;
    f32 field_14;
    i16 rotation_z;
    i16 rotation_y;
    u8 connection_index;
    i8 field_1d;
    i8 field_1e;
    u8 field_1f;
    i32 special_20;
    i32 special_24;
    f32 field_28, field_2c, field_30, field_34, field_38, field_3c;
    u8 red, green, blue, field_43;
};
DECOMP_ASSERT(sizeof(edbridge_s) == 0x44, "edbridge_s size");

static NUVEC *ed_loc;

extern "C" {
    extern debinftype *effecttypes;
    extern debinftype **debtab;
    extern i32 EDPP_MAX_TYPES;
    extern i32 edpp_types_used;
    extern usize edpp_page_scene[8];
    extern i32 edpp_page_used[8];
    extern i32 edpp_page_on[8];
    extern PartHeader **DmaDebTypes;
    extern i32 freeDmaDebType;
    i32 LookupDebrisEffect(char *name);
    i32 edbitsLookupSoundFX(char *name);
    void edbitsSoundPlay(NUVEC *position, i32 sound);
    void edanimSoundDestroy(i32 parameter_index, i32 sound_index);
    void AddVariableShotDebrisEffectTimed3(i32, NUVEC *, NUVEC *, i32, f32, NUMTX *, NUMTX *);
    extern part_type_s part_types[128];
    extern i32 part_types_used;
    extern i32 part_emits_used;
    extern NUGSCN *part_scene[32];
    extern i32 part_scene_pageid[32];
    i32 FindPlatInst(i32);
    void PlatInstBounce(i32, f32, f32, f32);
    void CheckPartCount(void);
    void KillPartsByScene(NUGSCN *);
    extern i32 DEBPAGE_GENERAL;
    extern i32 DEBPAGE_CHARACTER;
    extern i32 DEBPAGE_AREA;
    part_emit_s part_emits[512];
    i32 part_page_on[8];
    i32 part_page_used[8];
    i32 edpart_instances_used;
    edanim_param_s AnimParams[64];
    i32 edbits_anim_page;
    i32 edanim_particle_mode;
    i32 edanim_sound_mode;
    i32 edanim_next_param;
    i32 edanim_params_used;
    i32 edanim_page_on[8];
    i32 edanim_page_used[8];
    NUGSCN *edanim_page_scene[8];
    i32 edanim_nearest;
    i32 edanim_nearest_param_id;
    i32 edanim_nearest_particle;
    i32 edanim_nearest_sound;
    i32 edanim_particle_type;
    i32 edanim_sound_type;
    NUGSCN *edbits_base_scene;
    edbridge_s edBridges[64];
    i32 edbri_bridges_used;
    i32 edbri_page_on[8];
    NUGSCN *edbri_page_scene[8];
    i32 edbri_page_used[8];
    i32 edbri_rotz, edbri_roty, edbri_pageid;
    i32 edbri_plank_instance_type, edbri_post_instance_type;
    f32 edbri_length = 1.0f;
    f32 edbri_width = 0.5f;
    i32 edbri_planks = 11;
    i32 edbri_post_interval = 5;
}

void FileLoadSingleEffectType(debinftype *, i32, char);
extern "C" void NuBridgeInit(void);
extern "C" void NuBridgeRemove(i32 bridge);
extern "C" i32 NuBridgeCreate(NUGSCN *, nuhspecial_s *, nuhspecial_s *, NUVEC *, NUVEC *, f32, i32, f32, f32, f32, f32,
                              i32, f32, f32, i32, u32);

void edbriBridgeUpdate(i32 index, NUGSCN *scene) {
    if (edBridges[index].instance_id != -1) {
        NuBridgeRemove(edBridges[index].instance_id);
        edBridges[index].instance_id = -1;
    }
    NUVEC end;
    NUVEC delta;
    delta.x = edBridges[index].length;
    delta.y = 0.0f;
    delta.z = 0.0f;
    NuVecRotateZ(&delta, &delta, edBridges[index].rotation_z);
    NuVecRotateY(&delta, &delta, edBridges[index].rotation_y);
    NuVecAdd(&end, &edBridges[index].position, &delta);
    nuhspecial_s first = {};
    u32 colour = edBridges[index].red + ((edBridges[index].green << 8) + (edBridges[index].blue << 16) + 0x80000000u);
    if (edBridges[index].special_20 != -1)
        NuGScnGetSpecial(&first, edbits_base_scene, edBridges[index].special_20);
    nuhspecial_s second = {};
    if (edBridges[index].special_24 != -1)
        NuGScnGetSpecial(&second, edbits_base_scene, edBridges[index].special_24);
    edBridges[index].instance_id =
        NuBridgeCreate(scene, &first, &second, &edBridges[index].position, &end, edBridges[index].field_3c,
                       edBridges[index].rotation_y, edBridges[index].field_28, edBridges[index].field_2c,
                       edBridges[index].field_30, edBridges[index].field_34, edBridges[index].field_1d,
                       edBridges[index].field_14, edBridges[index].field_38, edBridges[index].field_1e, colour);
}

void edbriBridgePlace(i32 index, NUVEC *position) {
    edbridge_s *bridge = &edBridges[index];
    bridge->position = *position;
    bridge->rotation_z = edbri_rotz;
    bridge->rotation_y = edbri_roty;
    bridge->length = edbri_length;
    bridge->field_14 = edbri_width;
    bridge->field_1d = edbri_planks;
    bridge->field_1e = edbri_post_interval;
    bridge->special_20 = edbri_plank_instance_type;
    bridge->special_24 = edbri_post_instance_type;
    edbriBridgeUpdate(index, edbits_base_scene);
}

i32 edbriBridgeCreate(NUVEC *position) {
    if (edbri_bridges_used == 64)
        return -1;
    i32 index = 0;
    while (edBridges[index].connection_index != 0xff)
        ++index;
    edbridge_s *bridge = &edBridges[index];
    bridge->connection_index = edbri_pageid;
    ++edbri_bridges_used;
    bridge->field_28 = 0.18f;
    bridge->field_2c = 0.1f;
    bridge->field_30 = -0.01f;
    bridge->field_34 = 4.0f;
    bridge->field_38 = 0.5f;
    bridge->field_3c = 0.5f;
    bridge->red = 0x50;
    bridge->green = 0x50;
    bridge->blue = 0x30;
    bridge->field_43 = 0x80;
    edbri_page_used[edbri_pageid] = 1;
    edbri_page_scene[edbri_pageid] = edbits_base_scene;
    edbriBridgePlace(index, position);
    return index;
}

void edbriBridgeDestroy(i32 index) {
    edbridge_s *bridge = &edBridges[index];
    NuBridgeRemove(bridge->instance_id);
    bridge->connection_index = 0xff;
    bridge->instance_id = -1;
    --edbri_bridges_used;
}

void edanimDetermineNearestAnim(f32);
void edppDetermineNearest(float);
void edppPtlDestroy(i32);
extern "C" {
    extern edpp_particle_s edpp_ptls[512];
    extern i32 edpp_nearest;
    extern debkeydatatype_s *debkeydata;
    extern i32 maxdebkeys;
    void DebFreeOrphansInstantly(debinftype *);
    i32 LookupDebrisEffectPageIgnore(char *, i32, i32);
    void DebFreeInstantly(i32 *);
}

extern "C" void do_Pad_Standard_camera(edcam_s *camera, f32 delta_time, nupad_s *pad);
extern "C" void do_maya_mouse_camera(edcam_s *camera);
extern "C" void do_mouse_flymode_camera(edcam_s *camera, f32 delta_time);
extern "C" i32 NuKeyboard(i32 key);

static edcam_s gp_cam = {
    {0.0f, 0.0f, 0.0f},
    0,
    0,
    -2.0f,
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
    1,
    1,
    {0.0f, 0.0f, 0.0f},
    0,
    0,
    {0.0015625f, 0.0015625f, 0.0015625f},
    2,
    2,
    0.0078125f,
    0.5f,
    0.2f,
    0.2f,
    4.0f,
    0.15f,
    0.2f,
    0.01f,
    0.1f,
    EDCAM_FREEDOM_POSITION_X | EDCAM_FREEDOM_POSITION_Y | EDCAM_FREEDOM_POSITION_Z | EDCAM_FREEDOM_PITCH |
        EDCAM_FREEDOM_YAW | EDCAM_FREEDOM_DISTANCE,
    {0, 0, 0},
};

void edcamSetContoller(i32 invert_pitch) {
    gp_cam.invert_pad_pitch = invert_pitch;
}

static NUCAMERA *edmaincam = NULL;
static NUCAMERA *edinternalcam = NULL;
static eduimenu_s *active_menu;
static eduimenu_s *default_active_menu;
static eduimenu_s *processing_menu;
static f32 edui_cursor_x = 50.0f;
static f32 edui_cursor_y = 50.0f;
static f32 edui_cursor_dx;
static f32 edui_cursor_dy;
static i32 edui_cursor_locked;
static u32 edui_cursor_buttons;
static u32 edui_cursor_buttons_old;
static u32 edui_cursor_buttons_db;

static i32 numInteracts;
static eduimenu_s *ed_main_menu;
static eduimenu_s *ed_cfg_menu;
static i32 ed_init;
static ed_module_s *ed_list;
static i32 ed_module_active;
static ed_module_s *ed_curr;
static edui_interact_s eduiInteracts[64];
static edui_interact_s *eduiInteractLocked;

extern "C" {
    i32 PadFlyMode = 0;
    NUMTX *ed_remap_mtx = NULL;
    i32 edmain_cursor_enabled = 0;

    eduimenu_s *edGetMainMenu(void) {
        return ed_main_menu;
    }
    void edGraDisableTerrainSwap(void) {
    }
    void edGraEnableTerrainSwap(void) {
    }
    void edGraInitTerrainSwapProtection(void) {
    }
    i32 edanimLoadPage(char *path, NUGSCN *scene) {
        i32 page;
        if (edanim_page_used[0] == 0)
            page = 0;
        else if (edanim_page_used[1] == 0)
            page = 1;
        else if (edanim_page_used[2] == 0)
            page = 2;
        else if (edanim_page_used[3] == 0)
            page = 3;
        else if (edanim_page_used[4] == 0)
            page = 4;
        else if (edanim_page_used[5] == 0)
            page = 5;
        else if (edanim_page_used[6] == 0)
            page = 6;
        else if (edanim_page_used[7] == 0)
            page = 7;
        else
            return -1;
        EdFileSetMedia(1);
        if (EdFileOpen(path, NUFILE_READ) == 0)
            return -1;
        EdFileSetReadWrongEndianess(1);
        i32 version = EdFileReadInt();
        if (version > 6) {
            EdFileSetReadWrongEndianess(0);
            EdFileClose();
            return -1;
        }
        i32 count = EdFileReadInt();
        if (count + edanim_params_used > 64)
            count = 64 - edanim_params_used;
        i32 slot = 0;
        for (i32 index = 0; index < count; ++index) {
            while (AnimParams[slot].instance_id != -1 && slot < 64)
                ++slot;
            if (slot >= 64)
                continue;
            char name[20];
            EdFileRead(name, 20);
            edanim_param_s *param = &AnimParams[slot];
            param->instance_id = edanimLookupSpecial(name, scene);
            param->effect_count = EdFileReadInt();
            param->sound_count = version > 1 ? EdFileReadInt() : 0;
            param->field_00c = EdFileReadInt();
            param->field_010 = EdFileReadInt();
            param->field_014 = EdFileReadFloat();
            param->field_018 = EdFileReadFloat();
            if (param->effect_count > 8)
                param->effect_count = 8;
            for (i32 effect = 0; effect < param->effect_count; ++effect) {
                EdFileRead(param->effect_names[effect], 16);
                param->effect_ids[effect] = -1;
                if (version > 4) {
                    param->effect_intervals[effect] =
                        version == 5 ? static_cast<i32>(EdFileReadFloat()) : EdFileReadInt();
                } else {
                    i32 interval = EdFileReadInt();
                    param->effect_intervals[effect] = interval > 0 ? interval * 60 : interval == 0 ? 0 : -60 / interval;
                }
                param->effect_flags[effect] = EdFileReadInt();
                EdFileReadNuVec(reinterpret_cast<NUVEC *>(param->effect_positions[effect]));
                if (version > 2) {
                    param->effect_angles[effect] = EdFileReadShort();
                    param->effect_angle_ranges[effect] = EdFileReadShort();
                } else {
                    param->effect_angles[effect] = 0;
                    param->effect_angle_ranges[effect] = 0;
                }
            }
            param->field_17c = 0.99f;
            if (version > 1) {
                if (param->sound_count > 8)
                    param->sound_count = 8;
                for (i32 sound = 0; sound < param->sound_count; ++sound) {
                    EdFileRead(param->sound_names[sound], 16);
                    param->sound_ids[sound] = -1;
                    param->sound_flags[sound] = EdFileReadInt();
                    param->sound_values[sound] = EdFileReadFloat();
                    EdFileReadNuVec(reinterpret_cast<NUVEC *>(param->sound_positions[sound]));
                }
            }
            nuhspecial_s special;
            NuGScnGetSpecial(&special, scene, param->instance_id);
            param->platform_id = FindPlatInst(NuSpecialGetInstanceix(&special));
            if (version > 3) {
                param->bounce_impulse = EdFileReadFloat();
                param->bounce_spring = EdFileReadFloat();
                param->bounce_damping = EdFileReadFloat();
            } else {
                param->bounce_impulse = 0.0f;
                param->bounce_spring = 0.0f;
                param->bounce_damping = 0.0f;
            }
            if (param->platform_id != -1)
                PlatInstBounce(param->platform_id, param->bounce_impulse, param->bounce_spring, param->bounce_damping);
            param->page = static_cast<i8>(page);
            ++edanim_params_used;
        }
        EdFileSetReadWrongEndianess(0);
        EdFileClose();
        edanim_page_used[page] = 1;
        edanim_page_scene[page] = scene;
        edbits_anim_page = page;
        edanim_next_param = count;
        edanim_particle_mode = 0;
        edanim_nearest = -1;
        edanim_nearest_param_id = -1;
        edanimDetermineNearestAnim(1.0f);
        return page;
    }

    void edanimClearPage(i32 page) {
        if (edanim_page_on[page] != 0)
            edanimStopPage(page);
        for (i32 index = 0; index < 64; ++index) {
            if (AnimParams[index].page == page) {
                AnimParams[index].instance_id = -1;
                --edanim_params_used;
            }
        }
        edanim_page_used[page] = 0;
        edanim_page_scene[page] = NULL;
    }
    i32 edanimLookupSpecial(char *name, NUGSCN *scene) {
        if (scene != NULL) {
            nuhspecial_s special;
            for (i32 index = 0; index < NuGScnNumSpecials(scene); ++index) {
                NuGScnGetSpecial(&special, scene, index);
                if (NuStrNICmp(NuSpecialGetName(&special), name, 19) == 0)
                    return index;
            }
        }
        return -1;
    }
    void edanimParamReset(void) {
        for (i32 i = 0; i < 64; ++i) {
            AnimParams[i].instance_id = -1;
        }
        edanim_next_param = 0;
        edanim_params_used = 0;
        memset(edanim_page_used, 0, sizeof(edanim_page_used));
        memset(edanim_page_on, 0, sizeof(edanim_page_on));
    }
    void edanimParticleDestroy(i32 parameter_index, i32 particle_index) {
        edanim_param_s *parameters = AnimParams;
        for (i32 index = particle_index; index < parameters[parameter_index].effect_count - 1; ++index) {
            parameters[parameter_index].effect_ids[index] = parameters[parameter_index].effect_ids[index + 1];
            parameters[parameter_index].effect_intervals[index] =
                parameters[parameter_index].effect_intervals[index + 1];
            parameters[parameter_index].effect_flags[index] = parameters[parameter_index].effect_flags[index + 1];
            memcpy(parameters[parameter_index].effect_positions[index],
                   parameters[parameter_index].effect_positions[index + 1], sizeof(NUVEC));
            strcpy(parameters[parameter_index].effect_names[index],
                   parameters[parameter_index].effect_names[index + 1]);
        }
        --parameters[parameter_index].effect_count;
    }
    void edanimRegisterCubeDumpInfo(void) {
    }
    void edanimStartPage(i32 page) {
        if (edanim_page_used[page] != 0 && edanim_page_scene[page] != NULL && edanim_page_on[page] == 0)
            edanim_page_on[page] = 1;
    }
    void edanimStopPage(i32 page) {
        edanim_page_on[page] = 0;
    }
    void edanimUpdateObjects(float elapsed) {
        static i32 localframecount;
        float seconds;
        if (NuVideoGetMode() == 3)
            seconds = elapsed / 50.0f;
        else
            seconds = elapsed / 60.0f;
        for (i32 index = 0; index < 64; ++index) {
            edanim_param_s *param = &AnimParams[index];
            if (param->instance_id == -1 || edanim_page_on[param->page] == 0)
                continue;
            nuhspecial_s special;
            nuinstanim_s *animation = NULL;
            if (edanim_page_scene[param->page] != NULL) {
                NuGScnGetSpecial(&special, edanim_page_scene[param->page], param->instance_id);
                animation = NuSpecialGetInstAnim(&special);
                NuSpecialGetInstanceix(&special);
            }
            if (animation != NULL) {
                switch (param->field_00c) {
                    case 1:
                        animation->playing = 0;
                        if (param->field_010 != -1 && object_switches[param->field_010] != 0)
                            animation->playing = 1;
                        break;
                    case 2:
                        animation->repeating = 0;
                        if (param->field_010 != -1 && object_switches[param->field_010] != 0 && !animation->playing) {
                            animation->ltime = 1.0f;
                            animation->playing = 1;
                            animation->backwards = 0;
                            animation->waiting = 0;
                        }
                        break;
                    case 3:
                        animation->repeating = 1;
                        if (param->field_010 != -1 && object_switches[param->field_010] != 0)
                            animation->playing = 1;
                        break;
                    case 4:
                        animation->playing = param->field_014 > edanimPlayerAnimDistance(index);
                        break;
                    case 5:
                        animation->repeating = 0;
                        if (param->field_014 > edanimPlayerAnimDistance(index) && !animation->playing) {
                            animation->playing = 1;
                            animation->backwards = 0;
                            animation->waiting = 0;
                            animation->ltime = 1.0f;
                        }
                        break;
                    case 6:
                        animation->repeating = 1;
                        if (param->field_014 > edanimPlayerAnimDistance(index))
                            animation->playing = 1;
                        break;
                    case 10:
                        animation->ltime = 1.0f;
                        animation->playing = 0;
                        break;
                    case 11:
                        animation->playing = 1;
                        break;
                    case 12:
                        animation->playing = 1;
                        animation->repeating = 1;
                        break;
                }
                if (index == edanim_nearest_param_id && (edanim_particle_mode != 0 || edanim_sound_mode != 0))
                    animation->ltime = 1.0f;
            }
            if (NuSpecialGetVisibilityFn(&special) != NULL) {
                i32 particle_index = 0;
                while (particle_index < param->effect_count) {
                    i32 effect = param->effect_ids[particle_index];
                    if (effect != -1 && debtab[effect] != NULL) {
                        if (param->effect_flags[particle_index] == 0 || (animation != NULL && animation->playing)) {
                            NUMTX matrix;
                            NuMtxInvRSS(&matrix, NuSpecialGetMtx(&special));
                            NuMtxMul(&matrix, &matrix, NuSpecialGetDrawMtx(&special));
                            NUVEC position = *reinterpret_cast<NUVEC *>(param->effect_positions[particle_index]);
                            NuVecMtxTransform(&position, &position, &matrix);
                            NUMTX orientation = numtx_identity;
                            NuMtxRotateZ(&orientation, param->effect_angles[particle_index]);
                            NuMtxRotateY(&orientation, param->effect_angle_ranges[particle_index]);
                            NuMtxMul(&matrix, &orientation, &matrix);
                            AddVariableShotDebrisEffectTimed3(param->effect_ids[particle_index], &position, &nuvec_zero,
                                                              param->effect_intervals[particle_index], seconds, &matrix,
                                                              &numtx_identity);
                        }
                    } else if (param->effect_names[particle_index][0] != 0) {
                        param->effect_ids[particle_index] = LookupDebrisEffect(param->effect_names[particle_index]);
                        if (param->effect_ids[particle_index] == -1) {
                            edanimParticleDestroy(index, particle_index);
                            continue;
                        }
                    }
                    ++particle_index;
                }
                for (i32 sound_index = 0; sound_index < param->sound_count; ++sound_index) {
                    i32 sound = param->sound_ids[sound_index];
                    if (sound == -1) {
                        if (param->sound_names[sound_index][0] != 0) {
                            param->sound_ids[sound_index] = edbitsLookupSoundFX(param->sound_names[sound_index]);
                            if (param->sound_ids[sound_index] == -1) {
                                edanimSoundDestroy(index, sound_index);
                                --sound_index;
                            }
                        }
                        continue;
                    }
                    i32 play = -1;
                    if (param->sound_flags[sound_index] == 1) {
                        i32 interval = static_cast<i32>(param->sound_values[sound_index]);
                        if (static_cast<i32>(static_cast<float>(localframecount) + elapsed) / interval >
                            localframecount / interval)
                            play = sound;
                    } else if (animation != NULL) {
                        float frame = param->sound_values[sound_index];
                        if (animation->ltime >= frame && frame > param->field_17c)
                            play = sound;
                        if (animation->oscillate && frame >= animation->ltime && fabsf(param->field_17c) > frame)
                            play = sound;
                    }
                    if (play != -1) {
                        NUMTX matrix;
                        NuMtxInvRSS(&matrix, NuSpecialGetMtx(&special));
                        NuMtxMul(&matrix, &matrix, NuSpecialGetDrawMtx(&special));
                        // The original uses the completed particle-loop index here (0x35d332).
                        NUVEC position = *reinterpret_cast<NUVEC *>(param->sound_positions[particle_index]);
                        NuVecMtxTransform(&position, &position, &matrix);
                        edbitsSoundPlay(&position, play);
                    }
                }
            }
            if (animation != NULL)
                param->field_17c = animation->oscillate ? -animation->ltime : animation->ltime;
        }
        localframecount += static_cast<i32>(elapsed);
    }
    void edbitsDrawCube(f32, f32, f32, f32, f32, f32, i32, i32, i32, i32, i32, i32, numtl_s *);
    void edbitsDrawBBox(NUVEC *minimum, NUVEC *maximum, i32 colour, numtl_s *material) {
        edbitsDrawCube((minimum->x + maximum->x) * 0.5f, (minimum->y + maximum->y) * 0.5f,
                       (minimum->z + maximum->z) * 0.5f, (maximum->x - minimum->x) * 0.5f,
                       (maximum->y - minimum->y) * 0.5f, (maximum->z - minimum->z) * 0.5f, 0, 0, 0, 0, 0, colour,
                       material);
    }
    void edbitsDrawOvalTilted(NUVEC *, f32, f32, i32, i32, i32, i32);
    void edbitsDrawOvalXY(NUVEC *, f32, f32, i32, i32);
    void edbitsDrawCircleTilted(NUVEC *centre, f32 radius, i32 colour, i32 unused, i32 rotation_z, i32 rotation_y) {
        edbitsDrawOvalTilted(centre, radius, radius, colour, unused, rotation_z, rotation_y);
    }
    void edbitsDrawCircleXY(NUVEC *centre, f32 radius, i32 colour, i32 unused) {
        edbitsDrawOvalXY(centre, radius, radius, colour, unused);
    }
    void edbitsDrawOvalXY(NUVEC *centre, f32 radius_x, f32 radius_z, i32 colour, i32 unused) {
        edbitsDrawOvalTilted(centre, radius_x, radius_z, colour, unused, 0, 0);
    }
    void edbitsDrawSphere(NUVEC *centre, f32 radius, i32 colour, i32 unused) {
        edbitsDrawCircleXY(centre, radius, colour, unused);
        edbitsDrawCircleTilted(centre, radius, colour, unused, 0x2000, 0);
        edbitsDrawCircleTilted(centre, radius, colour, unused, 0x4000, 0);
        edbitsDrawCircleTilted(centre, radius, colour, unused, 0x6000, 0);
        edbitsDrawCircleTilted(centre, radius, colour, unused, 0x4000, 0x2000);
        edbitsDrawCircleTilted(centre, radius, colour, unused, 0x4000, 0x4000);
        edbitsDrawCircleTilted(centre, radius, colour, unused, 0x4000, 0x6000);
    }
    char *edbitsGetSoundName(i32) {
        return NULL;
    }
    i32 edbitsLookupInstance(char *name, NUGSCN *scene) {
        if (scene) {
            nuhspecial_s special;
            for (i32 index = 0; index < NuGScnNumSpecials(scene); ++index) {
                NuGScnGetSpecial(&special, scene, index);
                if (NuStrNICmp(NuSpecialGetName(&special), name, 19) == 0) {
                    return index;
                }
            }
        }
        return -1;
    }
    struct EDBITS_GAME_SOUND {
        char name[16];
        i32 id;
    };
    i32 edbits_numsounds;
    EDBITS_GAME_SOUND edbitsGameSound[320];

    i32 edbitsLookupSound(char *name) {
        for (i32 index = 0; index < edbits_numsounds; ++index) {
            if (NuStrNICmp(edbitsGameSound[index].name, name, 15) == 0) {
                return index;
            }
        }
        return -1;
    }
    i32 edbitsLookupSoundFX(char *) {
        return -1;
    }
    NUCAMERA *cubemapcam;
    i32 edbitsProcessCubemapDump(void) {
        if (edbits_cubecount == 0)
            return 0;
        if (cubemapcam == NULL)
            cubemapcam = NuCameraCreate();
        cubemapcam->fov = 1.5708f;
        cubemapcam->aspect = 1.0f;
        switch (edbits_cubecount) {
            case 60:
                edmainExtCamera(cubemapcam);
                edcamSetDist(0.0f);
                edcamSetAng(0, 0x4000);
                edcamSet();
                break;
            case 55:
                edbitsDoSingleDump(0);
                break;
            case 50:
                edcamSetAng(0, 0xc000);
                edcamSet();
                break;
            case 45:
                edbitsDoSingleDump(1);
                break;
            case 40:
                edcamSetAng(-0x4000, 0);
                edcamSet();
                break;
            case 35:
                edbitsDoSingleDump(2);
                break;
            case 30:
                edcamSetAng(0x4000, 0);
                edcamSet();
                break;
            case 25:
                edbitsDoSingleDump(3);
                break;
            case 20:
                edcamSetAng(0, 0);
                edcamSet();
                break;
            case 15:
                edbitsDoSingleDump(4);
                break;
            case 10:
                edcamSetAng(0, 0x8000);
                edcamSet();
                break;
            case 5:
                edbitsDoSingleDump(5);
                break;
            case 1:
                edmainExtCamera(NULL);
                NuRndrDoingScreenGrab = 0;
                break;
        }
        return --edbits_cubecount;
    }
    char edbits_datapath[256];
    char edbits_level_filename[256];
    u8 edbits_what_game;
    NUGSCN *edbits_things_scene;

    void edbitsRegisterDataPath(char *path) {
        if (path) {
            NuStrCpy(edbits_datapath, path);
        } else {
            edbits_datapath[0] = '\0';
        }
    }
    i32 edbits_editmode;
    static i32 edbits_local_editor_enabled;
    i32 *edbits_editor_enabled = &edbits_local_editor_enabled;

    void edbitsRegisterEditMode(i32 mode) {
        edbits_editmode = mode;
    }
    void edbitsRegisterEditorEnabledFlag(i32 *enabled) {
        edbits_editor_enabled = enabled;
    }
    void edbitsRegisterLevel(char *filename, i32 game) {
        if (filename) {
            NuStrCpy(edbits_level_filename, filename);
        } else {
            edbits_level_filename[0] = '\0';
        }
        edbits_what_game = game;
    }
    void edbitsRegisterPlaySound(EDBITSPLAYSOUNDCALLBACK callback) {
        edbitsPlaySound = callback;
    }
    void edbitsRegisterRequestSound(EDBITSREQUESTSOUNDCALLBACK callback) {
        edbitsRequestSound = callback;
    }
    char edbits_general_save_directory[256];
    char edbits_general_save_name[256];
    char edbits_general_save_extension[256];
    char edbits_level_save_directory[256];
    char edbits_level_save_name[256];
    char edbits_level_save_extension[256];

    void edbitsRegisterSaveFormat(char *general_save_directory, char *general_save_name, char *general_save_extension,
                                  char *level_save_directory, char *level_save_name, char *level_save_extension) {
        if (general_save_directory) {
            NuStrCpy(edbits_general_save_directory, general_save_directory);
        } else {
            edbits_general_save_directory[0] = '\0';
        }
        if (general_save_name) {
            NuStrCpy(edbits_general_save_name, general_save_name);
        } else {
            edbits_general_save_name[0] = '\0';
        }
        if (general_save_extension) {
            NuStrCpy(edbits_general_save_extension, general_save_extension);
        } else {
            edbits_general_save_extension[0] = '\0';
        }
        if (level_save_directory) {
            NuStrCpy(edbits_level_save_directory, level_save_directory);
        } else {
            edbits_level_save_directory[0] = '\0';
        }
        if (level_save_name) {
            NuStrCpy(edbits_level_save_name, level_save_name);
        } else {
            edbits_level_save_name[0] = '\0';
        }
        if (level_save_extension) {
            NuStrCpy(edbits_level_save_extension, level_save_extension);
        } else {
            edbits_level_save_extension[0] = '\0';
        }
    }
    void edbitsRegisterSoundEffect(char *name, i32 id) {
        i32 index = edbits_numsounds;
        EDBITS_GAME_SOUND *sound = &edbitsGameSound[index];
        strncpy(sound->name, name, 16);
        sound->name[15] = '\0';
        sound->id = id;
        edbits_numsounds = index + 1;
    }
    void edbitsRegisterThingsScene(NUGSCN *scene) {
        edbits_things_scene = scene;
    }
    void edbitsRegisterBaseTerrain(void *terrain) {
        extern void *edbits_base_terrain;
        edbits_base_terrain = terrain;
    }
    i32 edbitsSfxVol = 100;
    void edbitsSetSoundFxVolume(i32 volume) {
        edbitsSfxVol = volume;
    }
    i32 edbitsStartCubemapDump(void) {
        if (edbits_cubecount != 0)
            return 0;
        edbits_cubecount = 65;
        NuRndrDoingScreenGrab = 1;
        return 1;
    }
    void edbitsVector2YZRot(i16 *rotation_y, i16 *rotation_z, f32 x, f32 y, f32 z) {
        *rotation_y = -NuAtan2DA(z, x);
        *rotation_z = NuAtan2DA(y, NuFsqrt(x * x + z * z));
    }
    void edbriBridgesReset(void) {
        NuBridgeInit();
        for (i32 i = 0; i < 64; ++i) {
            edBridges[i].instance_id = -1;
            edBridges[i].connection_index = 0xff;
        }
        memset(edbri_page_used, 0, sizeof(edbri_page_used));
        memset(edbri_page_scene, 0, sizeof(edbri_page_scene));
        memset(edbri_page_on, 0, sizeof(edbri_page_on));
        edbri_bridges_used = 0;
    }
    void edbriStopPage(i8 page);
    void NuBridgeRemove(i32 bridge);
    void edbriClearPage(i8 page) {
        edbriStopPage(page);
        for (i32 i = 0; i < 64; ++i) {
            if (edBridges[i].connection_index == static_cast<u8>(page)) {
                edBridges[i].connection_index = 0xff;
                --edbri_bridges_used;
            }
        }
        edbri_page_used[page] = 0;
        edbri_page_scene[page] = 0;
    }
    void edbriStartPage(i32 page_number) {
        i8 page = static_cast<i8>(page_number);
        if (edbri_page_used[page] && edbri_page_scene[page]) {
            for (i32 i = 0; i < 64; ++i) {
                if (edBridges[i].connection_index == static_cast<u8>(page)) {
                    edbriBridgeUpdate(i, edbri_page_scene[page]);
                }
            }
        }
    }
    void edbriStopPage(i8 page) {
        if (edbri_page_used[page]) {
            for (i32 i = 0; i < 64; ++i) {
                if (edBridges[i].connection_index == static_cast<u8>(page) && edBridges[i].instance_id != -1) {
                    NuBridgeRemove(edBridges[i].instance_id);
                    edBridges[i].instance_id = -1;
                }
            }
        }
    }
    f32 edcamGetDist(void) {
        return gp_cam.distance;
    }
    edcam_s *edcamGetEdCam(void) {
        return &gp_cam;
    }
    void edcamGetOffset(NUVEC *offset) {
        *offset = gp_cam.offset;
    }
    void edcamGetPosAng(NUVEC *position, i32 *pitch, i32 *yaw) {
        if (position != NULL) {
            *position = gp_cam.position;
        }
        if (pitch != NULL) {
            *pitch = gp_cam.pitch;
        }
        if (yaw != NULL) {
            *yaw = gp_cam.yaw;
        }
    }
    void edcamGetPosAngSnap(NUVEC *position, i32 *pitch, i32 *yaw) {
        if (position != NULL) {
            *position = gp_cam.snapped_position;
        }
        if (pitch != NULL) {
            *pitch = gp_cam.snapped_pitch;
        }
        if (yaw != NULL) {
            *yaw = gp_cam.snapped_yaw;
        }
    }
    NUVEC *edcamGetPosPointer(void) {
        return &gp_cam.position;
    }
    void edcamMove(nupad_s *pad) {
        edcamMoveEx(pad, NuTimeGetFrameTime());
    }
    void edcamMoveEx(nupad_s *pad, f32 delta_time) {
        if (edmainGetCursorEnabled() != 0) {
            if (PadFlyMode == 0 || NuKeyboard(0x38) != 0) {
                do_maya_mouse_camera(&gp_cam);
            } else {
                do_mouse_flymode_camera(&gp_cam, delta_time);
            }
        }
        if (pad != NULL) {
            if (PadFlyMode == 0) {
                do_Pad_Standard_camera(&gp_cam, delta_time, pad);
            } else {
                do_Pad_flymode_camera(&gp_cam, delta_time, pad);
            }
        }
    }
    void edcamMtx(NUMTX *matrix) {
        NUVEC distance = {0.0f, 0.0f, gp_cam.distance};
        NuMtxSetTranslation(matrix, &distance);
        NuMtxRotateX(matrix, gp_cam.pitch);
        NuMtxRotateY(matrix, gp_cam.yaw);
        NuMtxTranslate(matrix, &gp_cam.position);
        NuMtxTranslate(matrix, &gp_cam.offset);
        if (ed_remap_mtx != NULL) {
            NuMtxMul(matrix, matrix, ed_remap_mtx);
            ed_remap_mtx = NULL;
        }
    }
    void edcamSet(void) {
        NUMTX matrix;
        edcamMtx(&matrix);
        edmainSetCamera(&matrix);
    }
    void edcamSetAdjustFreedom(bool position_x, bool position_y, bool position_z, bool pitch, bool yaw, bool distance) {
        gp_cam.allow_position_x = position_x;
        gp_cam.allow_position_y = position_y;
        gp_cam.allow_position_z = position_z;
        gp_cam.allow_pitch = pitch;
        gp_cam.allow_yaw = yaw;
        gp_cam.allow_distance = distance;
    }
    void edcamSetAng(i32 pitch, i32 yaw) {
        gp_cam.pitch = pitch;
        gp_cam.yaw = yaw;
    }
    void edcamSetAutoSpeed(f32 move_base, f32 move_distance_scale, f32 zoom_base, f32 zoom_distance_scale) {
        gp_cam.auto_move_base = move_base;
        gp_cam.auto_move_dist_scale = move_distance_scale;
        gp_cam.auto_zoom_base = zoom_base;
        gp_cam.auto_zoom_dist_scale = zoom_distance_scale;
    }
    void edcamSetDist(f32 distance) {
        gp_cam.distance = distance;
    }
    void edcamSetMouseSensitivity(f32 pitch, f32 yaw, f32 movement) {
        gp_cam.mouse_pitch_speed = pitch;
        gp_cam.mouse_yaw_speed = yaw;
        gp_cam.mouse_move_speed = movement;
    }
    void edcamSetOffset(NUVEC *offset) {
        gp_cam.offset = *offset;
    }
    void edcamSetPos(NUVEC *position) {
        gp_cam.position = *position;
        gp_cam.offset = {0.0f, 0.0f, 0.0f};
    }
    void edcamSetPosAng(NUVEC *position, i32 pitch, i32 yaw) {
        gp_cam.position = *position;
        gp_cam.offset = {0.0f, 0.0f, 0.0f};
        gp_cam.pitch = pitch;
        gp_cam.yaw = yaw;
    }
    void edcamSetSpeed(f32 position_x, f32 position_y, f32 position_z, f32 distance) {
        gp_cam.position_speed = {position_x, position_y, position_z};
        gp_cam.distance_speed = distance;
    }
    void edcamSetSpeedPos(f32 position_x, f32 position_y, f32 position_z) {
        gp_cam.position_speed = {position_x, position_y, position_z};
    }
    i32 edgraBufferUsage(void) {
        i32 count = 0;
        for (i32 i = 0; i < EDGRA_MAX_CLUMPS; ++i)
            count += GrassClumps[i].element_count;
        return count * 0x4c + 0x10;
    }
    extern i32 edgra_page_used[8];
    extern NUGSCN *edgra_page_scene[8];
    extern i32 edgra_page_vectors_valid[8];
    extern i32 edgra_page_calculate_done[8];
    NUMTX *edgra_page_matrix_stack[8];
    void edgraClearPage(i8 page) {
        edgraStopPage(page);
        for (i32 i = 0; i < EDGRA_MAX_CLUMPS; ++i) {
            edgra_clump_s *clump = &GrassClumps[i];
            if (clump->element_count && clump->page == static_cast<u8>(page)) {
                if (clump->kind == 3) {
                    IndGrassClumpsUsed[clump->individual_index] = 0;
                    --edgra_ind_clumps_used;
                }
                clump->element_count = 0;
                --edgra_clumps_used;
            }
        }
        edgra_page_used[page] = 0;
        edgra_page_scene[page] = NULL;
        edgra_page_matrix_stack[page] = NULL;
        edgra_page_vectors_valid[page] = 0;
        edgra_page_calculate_done[page] = 0;
    }
    i32 EDGRA_MAX_INDIVIDUAL_CLUMPS;
    extern i32 edgra_page_on[8];
    void edgraClumpsReset(void) {
        for (i32 i = 0; i < EDGRA_MAX_CLUMPS; ++i)
            GrassClumps[i].element_count = 0;
        memset(edgra_page_used, 0, sizeof(edgra_page_used));
        memset(edgra_page_scene, 0, sizeof(edgra_page_scene));
        memset(edgra_page_on, 0, sizeof(edgra_page_on));
        edgra_clumps_used = 0;
        for (i32 i = 0; i < EDGRA_MAX_INDIVIDUAL_CLUMPS; ++i)
            IndGrassClumpsUsed[i] = 0;
        edgra_ind_clumps_used = 0;
    }
    void edgraSetMemoryBuffer(VARIPTR start, VARIPTR end) {
        gra_ptr = start;
        gra_end = end;
    }
    void edgraSetThinning(i32 clumps, i32 elements) {
        edgra_clumpthin = clumps;
        edgra_elementthin = elements;
        if (edgra_clumpthin <= 0)
            edgra_clumpthin = 1;
        if (edgra_elementthin <= 0)
            edgra_elementthin = 1;
    }
    void NuWindSetup(VARIPTR *, VARIPTR, i32, i32);
    void NuFadeObjSetup(VARIPTR *, VARIPTR, i32, i32);
    void edgraSetup(VARIPTR *buffer, VARIPTR end, i32 clumps, i32 individual_clumps, i32 units_per_clump) {
        EDGRA_MAX_CLUMPS = clumps;
        EDGRA_MAX_INDIVIDUAL_CLUMPS = individual_clumps;
        EDGRA_MAX_UNITS_PER_INDIVIDUAL_CLUMP = units_per_clump;
        if (units_per_clump == 0)
            EDGRA_MAX_INDIVIDUAL_CLUMPS = 0;
        buffer->addr = (buffer->addr + 15) & ~static_cast<usize>(15);
        GrassClumps = static_cast<edgra_clump_s *>(buffer->void_ptr);
        buffer->char_ptr += EDGRA_MAX_CLUMPS * sizeof(edgra_clump_s);
        IndGrassClumps = static_cast<edgra_individual_s *>(buffer->void_ptr);
        buffer->char_ptr +=
            EDGRA_MAX_INDIVIDUAL_CLUMPS * EDGRA_MAX_UNITS_PER_INDIVIDUAL_CLUMP * sizeof(edgra_individual_s);
        IndGrassClumpsUsed = static_cast<i32 *>(buffer->void_ptr);
        buffer->char_ptr += EDGRA_MAX_INDIVIDUAL_CLUMPS * sizeof(i32);
        NuWindSetup(buffer, end, 0x3000, clumps);
        NuFadeObjSetup(buffer, end, 0x3000, clumps);
    }
    i32 edmainActivate(ed_module_s *module, i32 notify) {
        if (module) {
            for (ed_module_s *entry = ed_list; entry; entry = entry->next) {
                if (entry == module) {
                    ed_curr = module;
                    ed_module_active = 1;
                    if (module->activate && notify)
                        module->activate();
                    return 1;
                }
            }
        } else if (ed_curr) {
            if (ed_curr->deactivate && notify)
                ed_curr->deactivate();
            ed_curr = NULL;
            ed_module_active = 0;
        }
        return 0;
    }
    void edmainClose(void) {
        if (ed_init) {
            NuCameraDestroy(edinternalcam);
            while (ed_list) {
                if (ed_list->close)
                    ed_list->close();
                ed_list = ed_list->next;
            }
            if (ed_main_menu) {
                eduiMenuDestroy(ed_main_menu);
                ed_main_menu = NULL;
            }
            if (ed_cfg_menu) {
                eduiMenuDestroy(ed_cfg_menu);
                ed_cfg_menu = NULL;
            }
            ed_list = NULL;
            ed_curr = NULL;
            ed_init = 0;
        }
    }
    ed_module_s *edmainCurrent(void) {
        return ed_module_active ? ed_curr : NULL;
    }
    void edmainExtCamera(NUCAMERA *camera) {
        edmaincam = camera != NULL ? camera : edinternalcam;
    }
    NUCAMERA *edmainGetCamera(void) {
        return edmaincam;
    }
    i32 edmainGetCursorEnabled(void) {
        return edmain_cursor_enabled;
    }
    // Shared editor font; edmainInit receives and stores the font handle.
    void *ed_fnt;

    void edmainInit(void) {
    }
    void edmainInitEx(void) {
    }
    void edmainProcess(void) {
    }
    NUVEC *edmainQueryLocVec(void) {
        return ed_loc;
    }
    i32 edmainRegister(ed_module_s *module) {
        if (ed_init)
            edmainClose();
        if (ed_list)
            ed_list->previous = module;
        module->next = ed_list;
        ed_list = module;
        module->previous = NULL;
        return 1;
    }
    void edmainRegisterLocVec(NUVEC *position) {
        ed_loc = position;
    }
    void edmainRender(void) {
    }
    void edmainSetCamera(NUMTX *matrix) {
        edmaincam->mtx = *matrix;
        NuCameraSet(edmaincam);
    }
    void edmainSetCursorEnabled(i32 enabled) {
        edmain_cursor_enabled = enabled;
    }
    void edmainSetMainMenuScale(void) {
    }
    void edmainSetReturn(i32 result) {
        editor_return = result;
    }
    void edpartClearPage(i8 page) {
        NuThreadDisableThreadSwap();
        CheckPartCount();
        if (part_page_on[page] != 0)
            edpartStopPage(page);
        for (i32 index = 0; index < 128; ++index) {
            if (part_types[index].page == page && part_types[index].name[0] != 0) {
                part_types[index].name[0] = 0;
                part_types[index].effect_ids[0] = -1;
                --part_types_used;
            }
        }
        for (i32 index = 0; index < 40; ++index) {
            if (part_emits[index].page == page && part_emits[index].effect_id != -1) {
                part_emits[index].effect_id = -1;
                --part_emits_used;
            }
        }
        CheckPartCount();
        for (i32 index = 0; index < 32; ++index) {
            if (part_scene_pageid[index] == page) {
                KillPartsByScene(part_scene[index]);
                part_scene_pageid[index] = -1;
                part_scene[index] = NULL;
            }
        }
        NuThreadEnableThreadSwap();
        part_page_used[page] = 0;
    }
    void edpartDestroyAllParticles(void) {
    }
    void edpartParticleReset(void) {
        part_emit_s *emit = part_emits;
        part_emit_s *const emit_end = part_emits + 512;
        do {
            emit->instance_id = -1;
        } while (++emit != emit_end);
        memset(part_page_used, 0, sizeof(part_page_used));
        memset(part_page_on, 0, sizeof(part_page_on));
        edpart_instances_used = 0;
    }
    void edpartRegisterPointerToGameCharLocation(NUVEC *position) {
        edmainRegisterLocVec(position);
    }
    void edppClearPage(i8 page) {
        edpp_page_on[page] = 0;
        edpp_page_used[page] = 0;
        for (i32 index = 0; index < 512; ++index) {
            if (edpp_ptls[index].page == page)
                edppPtlDestroy(index);
        }
        for (i32 index = 1; index < EDPP_MAX_TYPES; ++index) {
            if (debtab[index] == NULL || debtab[index]->page != static_cast<u8>(page))
                continue;
            debtab[index]->disabled = 1;
            for (i32 key = 0; key < maxdebkeys; ++key) {
                if (debkeydata[key].effect_index == index) {
                    i32 handle = key;
                    DebFreeInstantly(&handle);
                }
            }
            DebFreeOrphansInstantly(debtab[index]);
            if (debtab[index]->native_data != NULL) {
                DmaDebTypes[--freeDmaDebType] = debtab[index]->native_data;
                debtab[index]->native_data = NULL;
            }
            debtab[index] = NULL;
            --edpp_types_used;
        }
    }
    void edppDeleteEffect(i32 index) {
        if (edpp_ptls[edpp_nearest].effect_index == index)
            edpp_nearest = -1;
        DebFreeOrphansInstantly(debtab[index]);
        i32 replacement = LookupDebrisEffectPageIgnore(debtab[index]->name, 1, index);
        if (replacement != -1) {
            for (i32 i = 0; i < 512; ++i) {
                if (edpp_ptls[i].effect_index == index) {
                    i32 handle = edpp_ptls[i].instance_id;
                    if (handle != 99999 && handle != -1)
                        debkeydata[handle].effect_index = replacement;
                    edpp_ptls[i].effect_index = replacement;
                }
            }
            for (i32 i = 0; i < maxdebkeys; ++i)
                if (debkeydata[i].effect_index == index)
                    debkeydata[i].effect_index = replacement;
        } else {
            for (i32 i = 0; i < 512; ++i)
                if (edpp_ptls[i].effect_index == index)
                    edppPtlDestroy(i);
            for (i32 i = 0; i < maxdebkeys; ++i) {
                if (debkeydata[i].effect_index == index) {
                    i32 handle = i;
                    DebFreeInstantly(&handle);
                }
            }
        }
        debtab[index] = NULL;
        --edpp_types_used;
        edppDetermineNearest(1.0f);
    }
    void edppDestroyAllEffects(void) {
    }
    void edppDestroyAllParticles(void) {
    }
    void edppDrawSpheres(void) {
    }
    void edppDrawTorus(void) {
    }
    void edppFindAllSounds(void) {
    }
    // Parts-page loader (edppLoadPage @0x36c630).  The normal general (0) and
    // character (5) pages only contain effect-type records; instance records
    // are read by the page-1/0 branches in the original and are deliberately
    // not entered here.
    i32 edppLoadPage(char *path, i32 flag, usize scene) {
        (void)scene;
        u8 category;
        i32 page_index;
        if (flag == 0) {
            category = 0;
            page_index = 0;
        } else if (flag == 5) {
            category = 5;
            page_index = 1;
        } else {
            // The remaining page kinds have their own instance-record paths;
            // they are outside the general/character pages recovered here.
            return -1;
        }

        EdFileSetMedia(1);
        if (EdFileOpen(path, NUFILE_READ) == 0) {
            return -1;
        }
        EdFileSetReadWrongEndianess(1);

        i32 version = EdFileReadInt();
        if (version < 5 || version > 41) {
            EdFileSetReadWrongEndianess(0);
            EdFileClose();
            return -1;
        }

        edpp_page_used[page_index] = 1;
        edpp_page_scene[page_index] = scene;

        i32 requested = EdFileReadInt();
        i32 available = EDPP_MAX_TYPES - edpp_types_used;
        if (requested > available) {
            requested = available;
        }
        if (requested < 0) {
            requested = 0;
        }

        for (i32 n = 0; n < requested; n++) {
            i32 index = 1;
            while (index < EDPP_MAX_TYPES && debtab[index] != NULL) {
                index++;
            }
            if (index >= EDPP_MAX_TYPES) {
                break;
            }

            debinftype *effect = &effecttypes[index];
            FileLoadSingleEffectType(effect, version, static_cast<char>(category));
            effect->native_data = NULL;
            effect->last_render_time = 0.0f;
            effect->page = static_cast<u8>(page_index);
            debtab[index] = effect;
            edpp_types_used++;
        }

        EdFileSetReadWrongEndianess(0);
        EdFileClose();
        edppDetermineNearest(1.0f);

        if (flag == 0) {
            DEBPAGE_GENERAL = page_index;
        } else if (flag == 5) {
            DEBPAGE_CHARACTER = page_index;
        }
        return page_index;
    }
    void edppRegisterPointerToGameCharLocation(NUVEC *position) {
        edmainRegisterLocVec(position);
    }
    void edppRestartAllEffectsInLevel(void) {
    }
    void edppSetSaveName(void) {
    }
    void edppStopPage(i32) {
    }
    void edqrand(void) {
    }
    void edrtlCalculateBurnout(void) {
    }
    void edrtlCalculateBurnoutEx(void) {
    }
    void edrtlDrawLight(void) {
    }
    void edrtlDrawLightEx(void) {
    }
    void edrtlGetFogSet(void) {
    }
    void eduiAddPropTextPickEnt(void) {
    }
    void eduiAddTextPickEnt(void) {
    }
    void eduiAddTextPickEntEx(void) {
    }
    i32 eduiClearActiveMenu(void) {
        eduiSetActiveMenu(NULL);
        return 0;
    }
    i32 eduiCheckForPadMenuCancel(eduimenu_s *menu, nupad_s *pad) {
        i32 result = 0;
        if (pad && (pad->digital_buttons_pressed & 0x10)) {
            eduimenu_s *parent = menu->parent;
            if (!eduiGetUsingMenuFocus() && menu->parent)
                eduiMenuDetach(menu);
            if (menu->callback)
                menu->callback(menu, parent);
            result = 1;
        }
        return result;
    }
    void eduiCreate3LineMessageMenu(void) {
    }
    void eduiCreateMessageMenu(void) {
    }
    i32 eduiCursorOverMenu(eduimenu_s *menu) {
        return edui_cursor_x >= menu->x && edui_cursor_y >= menu->y && edui_cursor_x < menu->x + menu->width &&
               edui_cursor_y < menu->y + menu->height;
    }
    void cbInteractMenuScrollTo(eduimenu_s *menu, char *text) {
        if (!text || !menu || !text[0])
            return;
        i32 comparison = menu->first && menu->first->text ? NuStrICmp(menu->first->text, text) : -1;
        menu->field_0c = menu->first;
        menu->selected = menu->first;
        while (menu->selected->next && comparison < 0) {
            menu->selected = menu->selected->next;
            comparison = menu->selected->text ? NuStrICmp(menu->selected->text, text) : -1;
        }
        menu->field_10 = menu->selected;
    }
    void cbInteractMenuKeySelect(eduimenu_s *menu) {
        char text[16];
        u32 modifiers;
        i32 key = NuKeyGet(&modifiers);
        if (key >= 0) {
            text[0] = NuKeyToAscii(key, modifiers & 1);
            text[1] = 0;
            cbInteractMenuScrollTo(menu, text);
        }
    }
    void eduiFlushInteracts(void) {
        numInteracts = 0;
    }
    eduimenu_s *eduiGetActiveMenu(void) {
        return active_menu;
    }
    eduimenu_s *eduiGetActiveMenuParent(void) {
        return eduiGetTopLevelParent(active_menu);
    }
    void eduiGetAnalougePadValue(void) {
    }
    i32 eduiGetCameraEnabled(void) {
        return bCameraEnabled;
    }
    void eduiGetCursorCoords(f32 *x, f32 *y) {
        *x = edui_cursor_x / 640.0f;
        *y = edui_cursor_y / 224.0f;
    }
    void eduiGetCursorDelta(f32 *x, f32 *y) {
        *x = edui_cursor_dx / 640.0f;
        *y = edui_cursor_dy / 224.0f;
    }
    eduimenu_s *eduiGetTopLevelParent(eduimenu_s *menu) {
        if (menu)
            while (menu->parent)
                menu = menu->parent;
        return menu;
    }
    i32 bUsingMenuFocus;
    i32 eduiGetUsingMenuFocus(void) {
        return bUsingMenuFocus;
    }
    void eduiGradPickRead(void) {
    }
    void eduiGradStageAdd(void) {
    }
    void eduiGradStageAddRGB(void) {
    }
    void eduiGradStageDelete(void) {
    }
    void eduiGradStageSetHSV(void) {
    }
    void eduiGradStageSetRGB(void) {
    }
    void eduiIitemExpanderSetDepth(void) {
    }
    void eduiInit(void) {
    }
    void eduiInitMaterials(void) {
    }
    void eduiItemCheckCreate(void) {
    }
    void eduiItemColourPickCreate(void) {
    }
    void eduiItemColourPickSetHSV(void) {
    }
    void eduiItemColourPickSetRGB(void) {
    }
    void eduiItemColourSliderCreate(void) {
    }
    void eduiItemDataGradPickCreate(void) {
    }
    void eduiItemExpanderAddChild(void) {
    }
    void eduiItemExpanderCreate(void) {
    }
    void eduiItemFilePickCreate(void) {
    }
    void eduiItemFilePickSetFmt(void) {
    }
    void eduiItemFilterAddItem(void) {
    }
    void eduiItemFilterCreate(void) {
    }
    void eduiItemFilterRemoveItem(void) {
    }
    void eduiItemGradPickCreate(void) {
    }
    void eduiItemGraphAddOnionSkin(void) {
    }
    void eduiItemGraphCreate(void) {
    }
    void eduiItemGraphSetCursor(void) {
    }
    void eduiItemGraphSetLabels(void) {
    }
    void eduiItemGreyGradPickCreate(void) {
    }
    void eduiItemGreyPickCreate(void) {
    }
    void eduiItemNumberCreate(void) {
    }
    void eduiItemPropCreate(void) {
    }
    void eduiItemPropCreateEx(void) {
    }
    i32 eduiItemPropSetText(edui_prop_s *item, char *text) {
        if (item->property_text && NuStrLen(item->property_text) < NuStrLen(text)) {
            NU_FREE(item->property_text);
            item->property_text = NULL;
        }
        if (!item->property_text) {
            u32 size = NuStrLen(text) + 1;
            item->property_text = static_cast<char *>(NU_ALLOC(size, 4, 1, "", 0));
        }
        if (item->property_text) {
            NuStrCpy(item->property_text, text);
            return 1;
        }
        return 0;
    }
    void eduiItemRender(void) {
    }
    void eduiItemSelCreate(void) {
    }
    void eduiItemSelWithClipColourCreate(void) {
    }
    void eduiItemSeparatorCreate(void) {
    }
    i32 eduiItemSetText(eduiitem_s *item, char *text) {
        if (item->text && NuStrLen(item->text) < NuStrLen(text)) {
            NU_FREE(item->text);
            item->text = NULL;
        }
        if (!item->text) {
            u32 size = NuStrLen(text) + 1;
            item->text = static_cast<char *>(NU_ALLOC(size, 4, 1, "", 0));
        }
        if (item->text) {
            NuStrCpy(item->text, text);
            return 1;
        }
        return 0;
    }
    void eduiItemSliderCreate(void) {
    }
    void eduiItemSliderCreateInt(void) {
    }
    void eduiItemSliderSetFmt(edui_slider_s *item, char *format) {
        if (!item->format) {
            u32 size = NuStrLen(format) + 1;
            item->format = static_cast<char *>(NU_ALLOC(size, 4, 1, "", 0));
        } else if (NuStrLen(item->format) < NuStrLen(format)) {
            NU_FREE(item->format);
            u32 size = NuStrLen(format) + 1;
            item->format = static_cast<char *>(NU_ALLOC(size, 4, 1, "", 0));
        }
        NuStrCpy(item->format, format);
    }
    void eduiItemSliderSetGranularity(edui_slider_s *item, f32 granularity) {
        item->granularity = granularity;
    }
    void eduiItemSliderSetVal(edui_slider_s *item, f32 value) {
        eduiItemSliderSetValEx(item, value, 1, 1);
    }
    void eduiItemSliderSetValEx(edui_slider_s *item, f32 value, i32 reset_timer, i32 notify) {
        item->normalized_value = (value - item->minimum) / item->range;
        item->value = value;
        if (reset_timer)
            item->change_timer = 60;
        if (notify && item->changed)
            item->changed(NULL, item, 0);
    }
    void eduiItemTextPickCreate(void) {
    }
    void eduiItemTextPickSetFmt(void) {
    }
    void eduiItemTextSelectorCreate(void) {
    }
    void eduiItemTexturePickCreate(void) {
    }
    void eduiItemToggleCreate(void) {
    }
    eduiitem_s *edui_last_item;
    eduiitem_s *eduiMenuAddItem(eduimenu_s *menu, eduiitem_s *item) {
        if (menu->last)
            menu->last->next = item;
        else
            menu->first = item;
        item->previous = menu->last;
        item->next = NULL;
        menu->last = item;
        menu->field_0c = menu->first;
        menu->field_10 = 0;
        edui_last_item = item;
        return item;
    }
    void eduiMenuAddItemAfter(eduimenu_s *menu, eduiitem_s *item, eduiitem_s *after) {
        if (after) {
            item->next = after->next;
            item->previous = after;
            after->next = item;
            if (item->next)
                item->next->previous = item;
            else
                menu->last = item;
            menu->field_0c = menu->first;
            menu->field_10 = 0;
            edui_last_item = item;
        } else
            eduiMenuAddItemFirst(menu, item);
    }
    void eduiMenuAddItemBefore(eduimenu_s *menu, eduiitem_s *item, eduiitem_s *before) {
        if (before) {
            item->previous = before->previous;
            item->next = before;
            before->previous = item;
            // The original does not update the preceding item's next link.
            if (!item->previous)
                menu->first = item;
            menu->field_0c = menu->first;
            menu->field_10 = 0;
            edui_last_item = item;
        } else
            eduiMenuAddItemLast(menu, item);
    }
    void eduiMenuAddItemFirst(eduimenu_s *menu, eduiitem_s *item) {
        if (menu->first)
            menu->first->previous = item;
        else
            menu->last = item;
        item->next = menu->first;
        item->previous = NULL;
        menu->first = item;
        menu->field_0c = item;
        menu->field_10 = 0;
        edui_last_item = item;
    }
    void eduiMenuAddItemLast(eduimenu_s *menu, eduiitem_s *item) {
        if (menu->last)
            menu->last->next = item;
        else
            menu->first = item;
        item->previous = menu->last;
        item->next = NULL;
        menu->last = item;
        menu->field_0c = item;
        menu->field_10 = 0;
        edui_last_item = item;
    }
    i32 eduiMenuAttach(eduimenu_s *menu, eduimenu_s *child) {
        if (child->parent)
            return 0;
        while (menu->child)
            menu = menu->child;
        child->parent = menu;
        menu->child = child;
        eduiSetActiveMenu(child);
        return 1;
    }
    eduimenu_s *eduiMenuCreate(i32 x, i32 y, i32 width, i32 height, void *font, EdUiMenuCallback callback,
                               char *title) {
        u32 bytes = sizeof(eduimenu_s) + 1;
        if (title)
            bytes += NuStrLen(title);
        eduimenu_s *menu = static_cast<eduimenu_s *>(NU_ALLOC(bytes, 4, 1, "", 0));
        if (menu) {
            memset(menu, 0, bytes);
            menu->field_24 = -1;
            menu->x = x;
            menu->y = y;
            menu->width = width;
            menu->height = height;
            menu->field_28 = -1;
            menu->font = font;
            menu->callback = callback;
            menu->flags = 0;
            if (title) {
                menu->title = reinterpret_cast<char *>(menu + 1);
                NuStrCpy(menu->title, title);
            }
            active_menu = menu;
        }
        edui_last_item = NULL;
        return menu;
    }
    void eduiMenuDestroy(eduimenu_s *menu) {
        if (menu) {
            if (eduiInteractLocked && eduiInteractLocked->menu == menu)
                eduiInteractLocked = NULL;
            if (processing_menu == menu)
                processing_menu = NULL;
            if (menu->parent) {
                if (active_menu == menu)
                    active_menu = menu->parent;
                menu->parent->child = NULL;
            }
            if (default_active_menu == menu)
                default_active_menu = NULL;
            if (active_menu == menu)
                active_menu = NULL;
            if (menu->child)
                menu->child->parent = NULL;
            eduiMenuDestroyItems(menu);
            NU_FREE(menu);
        }
    }
    i32 eduiMenuDetach(eduimenu_s *menu) {
        if (menu->parent) {
            if (menu->child)
                eduiMenuDetach(menu->child);
            if (eduiGetActiveMenu() == menu)
                active_menu = menu->parent;
            menu->parent->child = NULL;
            menu->parent = NULL;
            return 1;
        }
        eduiSetActiveMenu(NULL);
        return 0;
    }
    void eduiMenuEnsureSelection(eduimenu_s *menu) {
        if (!menu->selected)
            menu->selected = menu->first;
    }
    void eduiMenuFitOnScreen(void) {
    }
    void eduiMenuFitWidth(void) {
    }
    void eduiMenuHighlight(eduimenu_s *menu, eduiitem_s *item) {
        if (item->selection_group) {
            for (eduiitem_s *entry = menu->first; entry; entry = entry->next) {
                if (entry == item) {
                    if (entry->type == EDUI_ITEM_TOGGLE)
                        entry->highlighted = 1 - entry->highlighted;
                    else
                        entry->highlighted = 1;
                } else if (entry->selection_group == item->selection_group) {
                    entry->highlighted = 0;
                }
            }
        }
    }
    i32 eduiMenuIsActive(eduimenu_s *menu) {
        if (!eduiGetUsingMenuFocus())
            return 1;
        if (eduiGetUsingMenuFocus() && active_menu == menu)
            return 1;
        return 0;
    }
    i32 eduiMenuItemMoveDown(eduimenu_s *menu, eduiitem_s *item) {
        if (menu->last == item)
            return 0;
        eduiitem_s *next = item->next;
        eduiitem_s *previous = item->previous;
        item->previous = next;
        item->next = next->next;
        if (item->next)
            item->next->previous = item;
        else
            menu->last = item;
        if (previous)
            previous->next = next;
        else
            menu->first = next;
        next->next = item;
        next->previous = previous;
        menu->field_0c = menu->first;
        menu->field_10 = 0;
        return 1;
    }
    i32 eduiMenuItemMoveUp(eduimenu_s *menu, eduiitem_s *item) {
        if (menu->first == item)
            return 0;
        eduiitem_s *previous = item->previous;
        eduiitem_s *next = item->next;
        item->next = previous;
        item->previous = previous->previous;
        if (item->previous)
            item->previous->next = item;
        else
            menu->first = item;
        if (next)
            next->previous = previous;
        else
            menu->last = item->next;
        previous->previous = item;
        previous->next = next;
        menu->field_0c = menu->first;
        menu->field_10 = 0;
        return 1;
    }
    i32 eduiMenuProcess(eduimenu_s *menu, f32 delta_time, nupad_s *pad) {
        processing_menu = menu;
        if (menu->child) {
            i32 result = eduiMenuProcess(menu->child, delta_time, pad);
            if (processing_menu) {
                if (result)
                    eduiSetActiveMenu(menu->child ? menu->child : menu);
                else
                    result = eduiProcessInteracts(menu, pad);
            }
            return result;
        }
        return eduiMenuProcessAux(menu, delta_time, pad);
    }
    i32 eduiMenuProcessAux(eduimenu_s *menu, f32 delta_time, nupad_s *pad) {
        if (eduiMenuIsActive(menu)) {
            eduiMenuEnsureSelection(menu);
            i32 result = eduiMenuProcessSelectedItem(menu, delta_time, pad);
            if (eduiMenuIsActive(menu) && processing_menu == menu) {
                eduiMenuEnsureSelection(menu);
                if (eduiMenuIsActive(menu) && processing_menu == menu)
                    eduiMenuProcessInput(menu, delta_time, pad, result);
            }
        }
        return eduiProcessInteracts(menu, pad);
    }
    void eduiMenuProcessInput(eduimenu_s *menu, f32 delta_time, nupad_s *pad, i32 item_result) {
    }
    i32 eduiMenuProcessSelectedItem(eduimenu_s *menu, f32 delta_time, nupad_s *pad) {
        if (menu && menu->selected && !(menu->selected->flags & EDUI_ITEM_DISABLED) && menu->selected->process)
            return menu->selected->process(menu, menu->selected, delta_time, pad);
        return 0;
    }
    void eduiMenuRemoveItem(eduimenu_s *menu, eduiitem_s *item) {
        if (menu->first == item)
            menu->first = item->next;
        if (menu->last == item)
            menu->last = item->previous;
        if (menu->selected == item)
            menu->selected = item->next ? item->next : item->previous;
        menu->field_0c = menu->first;
        menu->field_10 = 0;
        if (item->next)
            item->next->previous = item->previous;
        if (item->previous)
            item->previous->next = item->next;
        item->next = NULL;
        item->previous = NULL;
    }
    void eduiMenuRender(void) {
    }
    void eduiMenuSelectFirstEntry(eduimenu_s *menu) {
        menu->selected = NULL;
    }
    void eduiMenuSetAttr(void) {
    }
    void eduiMenuSetDisabled(void) {
    }
    void eduiMenuSetTransparency(void) {
    }
    void eduiMenuSortItemsByTxt(void) {
    }
    void eduiProcessCursor(f32 delta_time, nupad_s *pad) {
        eduiProcessCursorDefault(delta_time, pad);
    }
    void eduiProcessCursorDefault(f32 delta_time, nupad_s *pad) {
        if (pad && eduiUsedAlgPad(pad) && !(pad->digital_buttons & EDUI_CURSOR_PRIMARY)) {
            eduiSetCursorCoords(0.5f, 0.5f);
        } else {
            edui_cursor_dx = -NuMouseReadXVel() * 0.1f;
            edui_cursor_dy = -NuMouseReadYVel() * 0.1f;
        }
        edui_cursor_locked = NuKeyboard(0x38) != 0;
        if (!edui_cursor_locked) {
            edui_cursor_x += edui_cursor_dx;
            edui_cursor_y += edui_cursor_dy;
        }
        if (edui_cursor_x < 0.0f)
            edui_cursor_x = 0.0f;
        if (edui_cursor_x > 640.0f)
            edui_cursor_x = 640.0f;
        if (edui_cursor_y < 0.0f)
            edui_cursor_y = 0.0f;
        if (edui_cursor_y > 224.0f)
            edui_cursor_y = 224.0f;
        edui_cursor_buttons_old = edui_cursor_buttons;
        edui_cursor_buttons = 0;
        edui_cursor_buttons |= NuMouseReadButtons() == 1 ? EDUI_CURSOR_PRIMARY : 0;
        edui_cursor_buttons |= NuMouseReadButtons() == 2 ? EDUI_CURSOR_SECONDARY : 0;
        edui_cursor_buttons_db = edui_cursor_buttons & ~edui_cursor_buttons_old;
    }
    i32 eduiProcessInteracts(eduimenu_s *menu, nupad_s *pad) {
        i32 result = 0;
        if (edmainGetCursorEnabled()) {
            if (eduiInteractLocked) {
                if ((!eduiInteractLocked->menu || !eduiInteractLocked->item) && eduiInteractLocked->field_20)
                    eduiInteractLocked = NULL;
            }
            if (eduiInteractLocked) {
                result = 1;
                if (menu == eduiInteractLocked->menu || menu->parent == eduiInteractLocked->menu) {
                    eduiInteractLocked = eduiInteractLocked->callback(eduiInteractLocked) ? eduiInteractLocked : NULL;
                    result = eduiInteractLocked != NULL;
                }
            } else {
                edui_interact_s *interact = eduiInteracts + numInteracts - 1;
                for (i32 i = 0; i < numInteracts; ++i, --interact) {
                    if (interact->menu == menu && (interact->buttons & edui_cursor_buttons_db) &&
                        edui_cursor_x >= interact->x && edui_cursor_y >= interact->y &&
                        edui_cursor_x < interact->x + interact->width &&
                        edui_cursor_y < interact->y + interact->height) {
                        if (menu && interact->item && interact->item->type != 0x12)
                            menu->selected = interact->item;
                        if (interact->field_20 || (interact->buttons & edui_cursor_buttons)) {
                            if (!interact->field_20 && eduiGetActiveMenu() != menu)
                                eduiSetActiveMenu(menu);
                            if (interact->callback && interact->callback(interact))
                                eduiInteractLocked = interact;
                        }
                        result = 1;
                        break;
                    }
                }
            }
        }
        return result;
    }
    void eduiRenderCursor(void) {
    }
    void eduiRenderInteracts(void) {
    }
    void eduiSetActiveMenu(eduimenu_s *menu) {
        if (eduiGetUsingMenuFocus()) {
            if (menu) {
                while (menu->child)
                    menu = menu->child;
                active_menu = menu;
            } else {
                menu = default_active_menu;
                if (menu)
                    while (menu->child)
                        menu = menu->child;
                active_menu = menu;
            }
        }
    }
    void eduiSetCameraEnabled(i32 enabled) {
        bCameraEnabled = enabled;
    }
    void eduiSetCursorColour(u32 colour) {
        edui_cursor_colour = colour;
    }
    void eduiSetCursorCoords(f32 x, f32 y) {
        edui_cursor_x = x * 640.0f;
        edui_cursor_y = y * 224.0f;
    }
    void eduiSetDefaultActiveMenu(eduimenu_s *menu) {
        default_active_menu = menu;
    }
    void eduiSetFont(i32 font) {
        edui_font = font;
    }
    void eduiSetFontScale(f32 x, f32 y) {
        edui_font_scale_x = x;
        edui_font_scale_y = y;
    }
    void eduiSetGlobalSliderAccel(void) {
    }
    void eduiSetRenderPlane(void) {
    }
    void eduiSetUsingMenuFocus(i32 enabled) {
        bUsingMenuFocus = enabled;
    }
    void eduiShowCursor(i32 show) {
        bShowCursor = show;
    }
    i32 eduiUsedAlgPad(nupad_s *pad) {
        if (pad) {
            if (pad->analog_left_x < 64 || pad->analog_left_x > 192 || pad->analog_left_y < 64 ||
                pad->analog_left_y > 192)
                return EDUI_ANALOG_PAD_LEFT;
            if (pad->analog_right_x < 64 || pad->analog_right_x > 192 || pad->analog_right_y < 64 ||
                pad->analog_right_y > 192)
                return EDUI_ANALOG_PAD_RIGHT;
        }
        return EDUI_ANALOG_PAD_NONE;
    }
    void eduicbCancelMessageMenu(void) {
        eduiMenuDestroy(edui_messagemenu);
        edui_messagemenu = NULL;
    }
    i32 eduicbInteractSlider(edui_interact_s *interact) {
        edui_slider_s *slider = static_cast<edui_slider_s *>(interact->item);
        f32 value = (edui_cursor_x - interact->x) / interact->width;
        value = CLAMP(value, 0.0f, 1.0f);
        eduiItemSliderSetVal(slider, value * slider->range);
        return (edui_cursor_buttons & EDUI_CURSOR_PRIMARY) != 0;
    }
    void eduicbItemDestroy(eduimenu_s *menu, eduiitem_s *item) {
        edui_interact_s *interact = eduiInteracts;
        for (i32 i = 0; i < numInteracts; ++i, ++interact) {
            if (menu == interact->menu && item == interact->item) {
                interact->menu = NULL;
                interact->item = NULL;
            }
        }
        if (item->text) {
            NU_FREE(item->text);
            item->text = NULL;
        }
        NU_FREE(item);
        menu->field_0c = menu->first;
    }
    void eduicbItemDestroyProp(eduimenu_s *menu, eduiitem_s *item) {
        edui_prop_s *prop = static_cast<edui_prop_s *>(item);
        if (prop->text) {
            NU_FREE(prop->text);
            prop->text = NULL;
        }
        if (prop->property_text) {
            NU_FREE(prop->property_text);
            prop->property_text = NULL;
        }
        NU_FREE(prop);
        menu->field_0c = menu->first;
    }
}
