#include "legoapi/gizmo/base/gizmo.h"
#include "decomp.h"
#include "globals.h"
#include "MechInputTouch/MechInputTouch_types.h"

#include "legoapi/gizmo/base/GizBlowupObjectInterface.h"
#include "legoapi/gizmo/base/GizBuildItObjectInterface.h"
#include "legoapi/gizmo/base/GizForceObjectInterface.h"
#include "legoapi/gizmo/base/GizLeverObjectInterface.h"
#include "legoapi/gizmo/base/GizObstacleObjectInterface.h"
#include "legoapi/gizmo/base/GizPanelObjectInterface.h"
#include "legoapi/gizmo/base/GizTurretObjectInterface.h"
#include "legoapi/gizmo/base/HatMachineObjectInterface.h"
#include "legoapi/gizmo/base/TeleportObjectInterface.h"
#include "legoapi/gizmos/trigger/ai.h"
#include "legoapi/gizmos/door/door.h"
#include "legoapi/gizmos/fx/edgizshadowmachine.h"
#include "legoapi/gizmos/trigger/gizaimessage.h"
#include "legoapi/gizmos/traps/gizbombgen.h"
#include "legoapi/gizmos/object/gizbuildits.h"
#include "legoapi/gizmos/traps/gizforce.h"
#include "legoapi/gizmos/fx/gizmopickups.h"
#include "legoapi/gizmos/object/gizobstacles.h"
#include "legoapi/gizmos/object/gizpanel.h"
#include "legoapi/gizmos/transport/gizportal.h"
#include "legoapi/gizmos/trigger/gizrandom.h"
#include "legoapi/gizmos/trigger/gizspecial.h"
#include "legoapi/gizmos/trigger/giztimer.h"
#include "legoapi/gizmos/traps/giztorpmachine.h"
#include "legoapi/gizmos/traps/gizturrets.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "legoapi/gizmos/object/hatmachine.h"
#include "legoapi/gizmos/object/lever.h"
#include "legoapi/gizmos/trigger/minicut.h"
#include "legoapi/gizmos/object/newblowup.h"
#include "legoapi/gizmos/door/plugs.h"
#include "legoapi/gizmos/door/push.h"
#include "legoapi/gizmos/door/spinner.h"
#include "legoapi/gizmos/object/technos.h"
#include "legoapi/gizmos/transport/teleport.h"
#include "legoapi/gizmos/transport/tubes.h"
#include "legoapi/gizmos/door/zipups.h"
#include "gameapi/edtools/edfile.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"

#include <stdio.h>
#include <string.h>
struct FLOWBOX_s;

i32 GizObstacle_CheckExcludeFlagsFn_LSW(GIZOBSTACLE_s *, GameObject_s *);

static i32 DefaultGizmo_GetOutput(GIZMO *, i32, i32) {
    return 0;
}

static char *DefaultGizmo_GetOutputName(GIZMO *, i32) {
    return "default";
}

static i32 DefaultGizmo_GetNumOutputs(GIZMO *) {
    return 1;
}

ADDGIZMOTYPE Default_ADDGIZMOTYPE = {
    NULL,                       // name
    NULL,                       // prefix
    -1,                         // id
    0,                          // unknown1
    0,                          // unknown2
    NULL,                       // get_max_gizmos_fn
    NULL,                       // add_gizmos_fn
    NULL,                       // early_update_fn
    NULL,                       // late_update_fn
    NULL,                       // draw_fn
    NULL,                       // panel_draw_fn
    NULL,                       // unknown_fn
    NULL,                       // get_gizmo_name_fn
    DefaultGizmo_GetOutput,     // get_output_fn
    DefaultGizmo_GetOutputName, // get_output_name_fn
    DefaultGizmo_GetNumOutputs, // get_num_outputs_fn
    NULL,                       // activate_fn
    NULL,                       // activate_rev_fn
    NULL,                       // set_visibility_fn
    NULL,                       // get_visibility_fn
    NULL,                       // get_pos_fn
    NULL,                       // using_special_fn
    NULL,                       // bolt_hit_plat_fn
    NULL,                       // get_best_bolt_target_fn
    NULL,                       // bolt_hit_fn
    NULL,                       // allocate_progress_data_fn
    NULL,                       // clear_progress_fn
    NULL,                       // store_progress_fn
    NULL,                       // reset_fn
    NULL,                       // reserve_buffer_space_fn
    NULL,                       // load_fn
    NULL,                       // post_load_fn
    NULL,                       // add_level_sfx_fn
};

// this technically has the wrong linkage, it does not have a C++ mangled name in the original binary... maybe should be
// in a different file?
static REGISTERGIZMOTYPEFN GizmoTypesLSW[] = {GizObstacles_RegisterGizmo,
                                              GizBuildIts_RegisterGizmo,
                                              GizForce_RegisterGizmo,
                                              NewBlowup_RegisterGizmo,
                                              GizmoPickups_RegisterGizmo,
                                              Levers_RegisterGizmo,
                                              Spinner_RegisterGizmo,
                                              MiniCut_RegisterGizmo,
                                              Tubes_RegisterGizmo,
                                              ZipUps_RegisterGizmo,
                                              GizTurrets_RegisterGizmo,
                                              GizBombGen_RegisterGizmo,
                                              AI_RegisterGizmo,
                                              GizSpecial_RegisterGizmo,
                                              GizAIMessage_RegisterGizmo,
                                              GizTimer_RegisterGizmo,
                                              GizRandom_RegisterGizmo,
                                              GizPanel_RegisterGizmo,
                                              HatMachine_RegisterGizmo,
                                              Push_RegisterGizmo,
                                              Door_RegisterGizmo,
                                              Teleport_RegisterGizmo,
                                              GizTorpMachine_RegisterGizmo,
                                              EdGizShadowMachine_RegisterGizmo,
                                              Portal_RegisterGizmo,
                                              Grapples_RegisterGizmo,
                                              Plugs_RegisterGizmo,
                                              Technos_RegisterGizmo,
                                              NULL};

#define GIZMO_TYPES_LSW_COUNT ((sizeof(GizmoTypesLSW) / sizeof(REGISTERGIZMOTYPEFN)) - 1)

GIZMOTYPES *gizmotypes;

VARIPTR *GizmoBufferAlloc(VARIPTR *buffer, VARIPTR *buffer_end, i32 size) {
    VARIPTR *ptr = NULL;

    if (buffer_end != NULL && buffer != NULL) {
        if (buffer_end->addr > buffer->addr + size) {
            ptr = (VARIPTR *)ALIGN(buffer->addr, 16);
            buffer->addr = (usize)ptr + size;
            memset(ptr, 0, size);
        }
    }

    return ptr;
}

void RegisterGizmoTypes(VARIPTR *buffer, VARIPTR *buffer_end, REGISTERGIZMOTYPEFN *register_gizmo_type_fns,
                        i32 unknown) {
    ADDGIZMOTYPE *addgizmo;
    GIZMOTYPES *types;
    GIZMOTYPE *gizmo;
    void *pvVar4;
    VARIPTR *pvVar3;
    i32 i;

    if (gizmotypes != NULL || register_gizmo_type_fns == NULL || *register_gizmo_type_fns == NULL) {
        return;
    }

    i32 ntypes = 0;
    for (; register_gizmo_type_fns[ntypes] != NULL; ntypes++) {
    }

    types = (GIZMOTYPES *)GizmoBufferAlloc(buffer, buffer_end, 0xc);
    gizmotypes = types;
    if (types == NULL) {
        return;
    }

    types->unknown = unknown;
    types->types = (GIZMOTYPE *)GizmoBufferAlloc(buffer, buffer_end, ntypes * sizeof(GIZMOTYPE));
    gizmo = gizmotypes->types;

    if (gizmo == NULL) {
        return;
    }

    gizmotypes->count = ntypes;

    for (i32 n = 0; n < gizmotypes->count; n++, gizmo++) {
        addgizmo = register_gizmo_type_fns[n](n);

        if (*addgizmo->prefix != '\0' && n != 0) {
            i = 0;
            do {
                while (gizmotypes->types[i].prefix[0] == '\0') {
                    i = i + 1;
                    if (i == n)
                        goto copy_data;
                }
                NuStrICmp(addgizmo->prefix, gizmotypes->types[i].prefix);
                i = i + 1;
            } while (i != n);
        }

    copy_data:
        NuStrCpy(gizmo->name, addgizmo->name);
        NuStrNCpy(gizmo->prefix, addgizmo->prefix, sizeof(gizmo->prefix));
        gizmo->fns.unknown1 = addgizmo->fns.unknown1;
        gizmo->fns.get_max_gizmos_fn = addgizmo->fns.get_max_gizmos_fn;
        gizmo->fns.add_gizmos_fn = addgizmo->fns.add_gizmos_fn;
        gizmo->fns.early_update_fn = addgizmo->fns.early_update_fn;
        gizmo->fns.late_update_fn = addgizmo->fns.late_update_fn;
        gizmo->fns.draw_fn = addgizmo->fns.draw_fn;
        gizmo->fns.panel_draw_fn = addgizmo->fns.panel_draw_fn;
        gizmo->fns.get_gizmo_name_fn = addgizmo->fns.get_gizmo_name_fn;
        gizmo->fns.get_output_fn = addgizmo->fns.get_output_fn;
        gizmo->fns.get_output_name_fn = addgizmo->fns.get_output_name_fn;
        gizmo->fns.get_num_outputs_fn = addgizmo->fns.get_num_outputs_fn;
        gizmo->fns.activate_fn = addgizmo->fns.activate_fn;
        gizmo->fns.activate_rev_fn = addgizmo->fns.activate_rev_fn;
        gizmo->fns.set_visibility_fn = addgizmo->fns.set_visibility_fn;
        gizmo->fns.get_visibility_fn = addgizmo->fns.get_visibility_fn;
        gizmo->fns.get_pos_fn = addgizmo->fns.get_pos_fn;
        gizmo->fns.using_special_fn = addgizmo->fns.using_special_fn;
        gizmo->fns.bolt_hit_plat_fn = addgizmo->fns.bolt_hit_plat_fn;
        gizmo->fns.get_best_bolt_target_fn = addgizmo->fns.get_best_bolt_target_fn;
        gizmo->fns.bolt_hit_fn = addgizmo->fns.bolt_hit_fn;
        gizmo->fns.allocate_progress_data_fn = addgizmo->fns.allocate_progress_data_fn;
        gizmo->fns.clear_progress_fn = addgizmo->fns.clear_progress_fn;
        gizmo->fns.store_progress_fn = addgizmo->fns.store_progress_fn;
        gizmo->fns.reset_fn = addgizmo->fns.reset_fn;
        gizmo->fns.reserve_buffer_space_fn = addgizmo->fns.reserve_buffer_space_fn;
        gizmo->fns.load_fn = addgizmo->fns.load_fn;
        gizmo->fns.post_load_fn = addgizmo->fns.post_load_fn;
        gizmo->fns.add_level_sfx_fn = addgizmo->fns.add_level_sfx_fn;

        pvVar4 = (void *)addgizmo->fns.allocate_progress_data_fn;

        if (pvVar4 != NULL && gizmotypes->unknown != 0) {
            pvVar3 = GizmoBufferAlloc(buffer, buffer_end, gizmotypes->unknown << 2);
            gizmo->buffer = pvVar3;
            if (types != NULL && gizmotypes->unknown > 0) {
                i = 0;
                while (1) {
                    void *pvVar5 = gizmo->fns.allocate_progress_data_fn(buffer, buffer_end);
                    pvVar3[i].void_ptr = pvVar5;
                    i = i + 1;
                    if (gizmotypes->unknown <= i)
                        break;
                    pvVar3 = gizmo->buffer;
                }
            }
        }
    }
}

void RegisterGizmoTypes_LSW(VARIPTR *buffer, VARIPTR *buffer_end) {
    REGISTERGIZMOTYPEFN gizmo_types[GIZMO_TYPES_LSW_COUNT + 1];
    memcpy(gizmo_types, GizmoTypesLSW, sizeof(GizmoTypesLSW));
    RegisterGizmoTypes(buffer, buffer_end, gizmo_types, 12);
}

GIZMO *AddGizmo(GIZMOSYS *gizmo_sys, i32 type_id, char *name, void *object) {
    if (gizmotypes == NULL || gizmo_sys == NULL) {
        return NULL;
    }

    if (name != NULL) {
        type_id = GizmoGetTypeIDByName(gizmo_sys, name);
    }

    if (type_id > -1 && gizmotypes->count > type_id) {
        GIZMOSET *set = &gizmo_sys->sets[type_id];
        if (set->count < set->max_count) {
            GIZMO *gizmo = &set->gizmos[set->count];
            if (gizmo != NULL) {
                if (object != NULL) {
                    gizmo->object = object;
                }

                gizmo->unknown = 0;
                gizmo->type_id = (u8)type_id;
                set->count++;

                return gizmo;
            }
        }
    }

    return NULL;
}

i32 GizmoGetTypeIDByName(GIZMOSYS_s *gizmo_sys, char *name) {
    if (gizmotypes == NULL || name == NULL || gizmo_sys == NULL || gizmotypes->count <= 0) {
        return -1;
    }

    for (i32 i = 0; i < gizmotypes->count; i++) {
        if (NuStrCmp(gizmotypes->types[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

void GizmoSetVisibility(GIZMOSYS *gizmo_sys, GIZMO *gizmo, i32 visibility, i32 unknown) {
    if (gizmotypes == NULL || gizmo == NULL || gizmo_sys == NULL || gizmo->type_id >= gizmotypes->count ||
        gizmotypes->types[gizmo->type_id].fns.set_visibility_fn == NULL) {
        return;
    }

    if (visibility == 0 && gizmotypes->types[gizmo->type_id].fns.activate_fn != NULL) {
        gizmotypes->types[gizmo->type_id].fns.activate_fn(gizmo, 0);
    }

    gizmotypes->types[gizmo->type_id].fns.set_visibility_fn(gizmo, visibility);
}

i32 GizmoGetVisibility(GIZMOSYS *gizmo_sys, GIZMO *gizmo) {

    if (gizmotypes == NULL || gizmo == NULL || gizmo_sys == NULL || gizmo->type_id >= gizmotypes->count ||
        gizmotypes->types[gizmo->type_id].fns.get_visibility_fn == NULL) {
        return 0;
    }

    return gizmotypes->types[gizmo->type_id].fns.get_visibility_fn(gizmo);
}

void GizmoActivate(GIZMOSYS *gizmo_sys, GIZMO *gizmo, i32 unknown1, i32 unknown2) {
    if (gizmotypes == NULL || gizmo == NULL || gizmo_sys == NULL || gizmo->type_id >= gizmotypes->count) {
        return;
    }

    GIZMOTYPE *type = &gizmotypes->types[gizmo->type_id];
    if (type->fns.activate_fn == NULL) {
        return;
    }

    if (unknown1 != 0 && type->fns.set_visibility_fn != NULL) {
        type->fns.set_visibility_fn(gizmo, 1);
    }

    type->fns.activate_fn(gizmo, unknown1);
    LOG_INFO("gizmo activate type=%s gizmo=%p active=%d", type->name, (void *)gizmo, unknown1);
}

char *GizmoGetOutputName(GIZMOSYS *gizmo_sys, GIZMO *gizmo, i32 output_index) {
    if (gizmotypes == NULL || gizmo == NULL || gizmo_sys == NULL) {
        return NULL;
    }

    if (gizmo->type_id >= gizmotypes->count || gizmotypes->types[gizmo->type_id].fns.get_output_name_fn == NULL) {
        return "default";
    }

    return gizmotypes->types[gizmo->type_id].fns.get_output_name_fn(gizmo, output_index);
}

i32 GizmoGetOutput(GIZMOSYS *gizmo_sys, GIZMO *gizmo, i32 unknown1, i32 unknown2) {
    if (gizmotypes == NULL || gizmo == NULL || gizmo_sys == NULL) {
        return 0;
    }

    if (gizmo->type_id >= gizmotypes->count || gizmotypes->types[gizmo->type_id].fns.get_output_fn == NULL) {
        return 0;
    }

    return gizmotypes->types[gizmo->type_id].fns.get_output_fn(gizmo, unknown1, unknown2);
}

void GizmoSysEarlyUpdate(GIZMOSYS *gizmo_sys, void *world_info, float delta_time) {
    if (gizmotypes == NULL || gizmo_sys == NULL) {
        return;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 i = 0; i < gizmotypes->count; i++, set++, type++) {
        if (type->fns.early_update_fn != NULL) {
            type->fns.early_update_fn(world_info, set->unknown, delta_time);
        }
    }
}

void GizmoSysLateUpdate(GIZMOSYS *gizmo_sys, void *world_info, float delta_time) {
    if (gizmotypes == NULL || gizmo_sys == NULL) {
        return;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 i = 0; i < gizmotypes->count; i++, set++, type++) {
        if (type->fns.late_update_fn != NULL) {
            type->fns.late_update_fn(world_info, set->unknown, delta_time);
        }
    }
}

void GizmoSysDraw(GIZMOSYS *gizmo_sys, void *world_info, float delta_time) {
    if (gizmotypes == NULL || gizmo_sys == NULL) {
        return;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 i = 0; i < gizmotypes->count; i++, set++, type++) {
        if (type->fns.draw_fn != NULL) {
            type->fns.draw_fn(world_info, set->unknown, delta_time);
        }
    }
}

void GizmoSysPanelDraw(GIZMOSYS *gizmo_sys, void *world_info, float delta_time) {
    if (gizmotypes == NULL || gizmo_sys == NULL) {
        return;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 i = 0; i < gizmotypes->count; i++, set++, type++) {
        if (type->fns.panel_draw_fn != NULL) {
            type->fns.panel_draw_fn(world_info, set->unknown, delta_time);
        }
    }
}

i32 GizmoSys_BoltHitPlat(GIZMOSYS *gizmo_sys, void *world_info, BOLT *bolt, unsigned char *unknown) {
    if (gizmotypes == NULL || bolt == NULL || gizmo_sys == NULL) {
        return 0;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 i = 0; i < gizmotypes->count; i++, set++, type++) {
        if (type->fns.bolt_hit_plat_fn != NULL) {
            i32 result = type->fns.bolt_hit_plat_fn(world_info, set->unknown, bolt, unknown);
            if (result != 0) {
                return 1;
            }
        }
    }

    return 0;
}

i32 ResetGizmoType(GIZMOSYS *gizmo_sys, i32 type_id, char *name) {
    if (name != NULL && type_id == -1) {
        type_id = GizmoGetTypeIDByName(gizmo_sys, name);
    }

    if (type_id == -1) {
        return 0;
    }

    gizmo_sys->sets[type_id].count = 0;
    SAGA_HOST_SAFE_MEMSET(gizmo_sys->sets[type_id].gizmos, 0, gizmo_sys->sets[type_id].max_count * sizeof(GIZMO));

    return 1;
}

void GizmoSysClearLevelProgress(void *unknown, i32 type_id) {
    GIZMOTYPE *type = gizmotypes->types;
    if (gizmotypes == NULL || gizmotypes->count <= 0) {
        return;
    }

    if (type_id < 0) {
        for (i32 i = 0; i < gizmotypes->count; i++, type++) {
            if (type->fns.clear_progress_fn != NULL) {
                type->fns.clear_progress_fn(unknown, NULL);
            }
        }
    } else {
        for (i32 i = 0; i < gizmotypes->count; i++, type++) {
            if (type->fns.clear_progress_fn != NULL) {
                void *progress = NULL;
                if (type->buffer != NULL && type_id < gizmotypes->unknown) {
                    progress = type->buffer[type_id].void_ptr;
                }

                type->fns.clear_progress_fn(unknown, progress);
            }
        }
    }
}

extern "C" {
    extern f32 hackFlashTimer;
    extern GAMEANIMSET_s *hackFlashingGameAnimSet;
    nuhspecial_s *hackFlashingSpecial;
}

void GizForceObjectInterface::GetPos(VuVec &result, i32) const {
    if (selected_object != NULL) {
        NUVEC *position = NuSpecialGetDrawPos(&selected_object->special);
        result = VuVec(position->x, position->y, position->z, 1.0f);
    } else {
        result = VuVec(force.position.x, force.position.y, force.position.z, 1.0f);
    }
}

f32 GizForceObjectInterface::GetRadius() const {
    if (selected_object != NULL)
        return NuSpecialGetOriginRadius(&selected_object->special);
    return force.radius;
}

const char *GizForceObjectInterface::GetTargetName() const {
    return force.name;
}

void *GizForceObjectInterface::GetTgtVoidPtr() {
    if (selected_object != NULL)
        return selected_object;
    return &force;
}

GizForceObjectInterface::GizForceObjectInterface(GIZFORCE_s &value) : force(value) {
    force.mech_object_interface = this;
}

void GizForceObjectInterface::TargetedFlash() {
    if (!(force.field_0xaa & 0x40)) {
        hackFlashTimer = 1.0f;
        if (selected_object != NULL) {
            hackFlashingSpecial = &selected_object->special;
            hackFlashingGameAnimSet = NULL;
        } else {
            hackFlashingSpecial = NULL;
            hackFlashingGameAnimSet = force.anim_set;
        }
    }
}

GizForceObjectInterface::~GizForceObjectInterface() {
    force.mech_object_interface = NULL;
}

void MechObjectInterface::GetFloorTargetPos(VuVec &position, i32 mode) const {
    GetPos(position, mode);
}

void MechTempPosInterface::GetFloorTargetPos(VuVec &result, i32 mode) const {
    GetPos(result, mode);
}

MechTempPosInterface::MechTempPosInterface(VuVec const &value) {
    position.x = value.x;
    position.y = value.y;
    position.z = value.z;
    position.w = value.w;
    radius = 0.2f;
}

MechTempPosInterface::MechTempPosInterface(nuvec_s const &value) {
    position.xyz = value;
    radius = 0.2f;
}

void GizLeverObjectInterface::GetPos(VuVec &position, i32) const {
    position = VuVec(lever.position.x, lever.position.y, lever.position.z, 1.0f);
}

f32 GizLeverObjectInterface::GetRadius() const {
    return 0.1f;
}

const char *GizLeverObjectInterface::GetTargetName() const {
    return lever.name;
}

GizLeverObjectInterface::GizLeverObjectInterface(LEVER_s &value) : lever(value) {
    lever.mech_object = this;
}

void GizLeverObjectInterface::TargetedFlash() {
    lever.flash_timer = 1.0f;
}

GizLeverObjectInterface::~GizLeverObjectInterface() {
    lever.mech_object = NULL;
}

void GizPanelObjectInterface::GetFloorTargetPos(VuVec &position, i32) const {
    position = VuVec(panel.position.x, panel.position.y, panel.position.z, 1.0f);
}

void GizPanelObjectInterface::GetPos(VuVec &position, i32) const {
    GizPanel_GetAbsTargetPos(&panel, &position.xyz, 1);
}

f32 GizPanelObjectInterface::GetRadius() const {
    return 0.1f;
}

const char *GizPanelObjectInterface::GetTargetName() const {
    return panel.name;
}

GizPanelObjectInterface::GizPanelObjectInterface(GIZPANEL_s &value) : panel(value) {
    panel.mech_object_interface = this;
}

void GizPanelObjectInterface::TargetedFlash() {
    panel.flash_timer = 1.0f;
}

GizPanelObjectInterface::~GizPanelObjectInterface() {
    panel.mech_object_interface = NULL;
}

void TeleportObjectInterface::GetPos(VuVec &position, i32 direction) const {
    NUVEC *point;
    if (direction != -1) {
        point = teleport.path->pts;
    } else if (index > 0 && (teleport.flags & 1) == 0) {
        point = &teleport.path->pts[teleport.path->length - 1];
    } else {
        point = teleport.path->pts;
    }
    position = VuVec(point->x, point->y, point->z, 1.0f);
}

f32 TeleportObjectInterface::GetRadius() const {
    return 0.1f;
}

const char *TeleportObjectInterface::GetTargetName() const {
    return teleport.name;
}

void TeleportObjectInterface::TargetedFlash() {
}

TeleportObjectInterface::TeleportObjectInterface(TELEPORT_s &value, i32 teleport_index)
    : teleport(value), index(teleport_index) {
    value.mech_object_interface = this;
}

TeleportObjectInterface::~TeleportObjectInterface() {
    teleport.mech_object_interface = NULL;
}

// Original 0x447180..0x447fb0: this interface belongs to the shared gizmo
// interface family; PART_s owns only the lazy allocation/deletion entry points.
void PartObjectInterface::GetPos(VuVec &position, i32) const {
    position = VuVec(part.position.x, part.position.y, part.position.z, 1.0f);
}

f32 PartObjectInterface::GetRadius() const {
    return part.radius;
}

const char *PartObjectInterface::GetTargetName() const {
    return "Part";
}

PartObjectInterface::PartObjectInterface(PART_s &value) : part(value) {
    part.mech_object_interface = this;
}

PartObjectInterface::~PartObjectInterface() {
    part.mech_object_interface = NULL;
}

void GizBlowupObjectInterface::GetPos(VuVec &position, i32) const {
    if (blowup->field_0x120 != NULL) {
        NUVEC *origin = static_cast<NUVEC *>(blowup->field_0x120);
        position = VuVec(origin->x, origin->y, origin->z, 1.0f);
    } else {
        position.x = blowup->position.x;
        position.y = blowup->position.y;
        position.z = blowup->position.z;
    }
}

f32 GizBlowupObjectInterface::GetRadius() const {
    return blowup->target_scale;
}

const char *GizBlowupObjectInterface::GetTargetName() const {
    return blowup->name;
}

GizBlowupObjectInterface::GizBlowupObjectInterface(GIZMOBLOWUP_s &object) : blowup(&object) {
    object.mech_object_interface = this;
}

bool GizBlowupObjectInterface::IsDead() {
    return (blowup->state_flags & 0x80) == 0;
}

void GizBlowupObjectInterface::TargetedFlash() {
    blowup->flicker_timer = 1.0f;
}

GizBlowupObjectInterface::~GizBlowupObjectInterface() {
    blowup->mech_object_interface = NULL;
}

void GizTurretObjectInterface::GetPos(VuVec &position, i32) const {
    position = VuVec(turret.field_0x3c.x, turret.field_0x3c.y, turret.field_0x3c.z, 1.0f);
}

f32 GizTurretObjectInterface::GetRadius() const {
    return 0.1f;
}

const char *GizTurretObjectInterface::GetTargetName() const {
    return turret.name;
}

GizTurretObjectInterface::GizTurretObjectInterface(GIZTURRET_s &value) : turret(value) {
    turret.mech_object_interface = this;
}

void GizTurretObjectInterface::TargetedFlash() {
    hackFlashTimer = 1.0f;
    hackFlashingGameAnimSet = turret.anim_set;
}

GizTurretObjectInterface::~GizTurretObjectInterface() {
    turret.mech_object_interface = NULL;
}

void CalcAveragePosAndRad(GIZBUILDIT_s &, VuVec &, f32 &, bool);
f32 hackFlashTimer;
GAMEANIMSET_s *hackFlashingGameAnimSet;

void GizBuildItObjectInterface::GetPos(VuVec &position, i32) const {
    f32 radius;
    CalcAveragePosAndRad(*buildit, position, radius, true);
}

f32 GizBuildItObjectInterface::GetRadius() const {
    f32 radius;
    VuVec position;
    CalcAveragePosAndRad(*buildit, position, radius, true);
    return radius;
}

const char *GizBuildItObjectInterface::GetTargetName() const {
    return buildit->name;
}

GizBuildItObjectInterface::GizBuildItObjectInterface(GIZBUILDIT_s &object) : buildit(&object) {
    object.mech_object_interface = this;
}

void GizBuildItObjectInterface::TargetedFlash() {
    hackFlashTimer = 1.0f;
    hackFlashingGameAnimSet = buildit->anim_set;
}

GizBuildItObjectInterface::~GizBuildItObjectInterface() {
    buildit->mech_object_interface = NULL;
}

void HatMachineObjectInterface::GetPos(VuVec &position, i32) const {
    position = VuVec(machine.position.x, machine.position.y, machine.position.z, 1.0f);
}

f32 HatMachineObjectInterface::GetRadius() const {
    return 0.1f;
}

const char *HatMachineObjectInterface::GetTargetName() const {
    return machine.name;
}

HatMachineObjectInterface::HatMachineObjectInterface(HATMACHINE_s &value) : machine(value) {
    machine.mech_object_interface = this;
}

void HatMachineObjectInterface::TargetedFlash() {
    machine.flash_timer = 1.0f;
}

HatMachineObjectInterface::~HatMachineObjectInterface() {
    machine.mech_object_interface = NULL;
}

void GizObstacleObjectInterface::GetPos(VuVec &result, i32) const {
    result = VuVec(obstacle.evaluated_position.x, obstacle.evaluated_position.y, obstacle.evaluated_position.z, 1.0f);
}

f32 GizObstacleObjectInterface::GetRadius() const {
    return obstacle.field_0x58;
}

const char *GizObstacleObjectInterface::GetTargetName() const {
    return obstacle.name;
}

GizObstacleObjectInterface::GizObstacleObjectInterface(GIZOBSTACLE_s &value) : obstacle(value) {
    obstacle.mech_object_interface = this;
}

bool GizObstacleObjectInterface::IsDead() {
    return !(obstacle.progress_flags & 1);
}

void GizObstacleObjectInterface::TargetedFlash() {
    hackFlashTimer = 1.0f;
    hackFlashingGameAnimSet = obstacle.anim_set;
}

GizObstacleObjectInterface::~GizObstacleObjectInterface() {
    obstacle.mech_object_interface = NULL;
}

static __used__ void CheckIfParentsFinished(GIZFLOW_s *system, FLOWBOX_s *box) {
    for (i32 i = 0; i < box->parent_count; ++i) {
        FLOWBOX_s *parent = box->parents[i];
        if ((parent->state_flags_low & 0x20) == 0)
            continue;

        i32 child_index;
        for (child_index = 0; child_index < parent->child_count; ++child_index) {
            FLOWBOX_s *child = parent->children[child_index];
            if ((child->state_flags_low & 0xc0) != 0) {
                if ((child->state_flags & 0x102) != 2)
                    break;
            } else if ((child->state_flags_low & 1) != 0 && child != box) {
                break;
            }
        }
        if (child_index != parent->child_count)
            continue;

        if ((parent->state_flags_low & 8) != 0) {
            FLOWBOXGIZMODATA_s *data = parent->data;
            for (i32 j = 0; j < data->gizmo_count; ++j)
                GizmoSetVisibility(system->gizmo_sys, data->gizmos[j]->gizmo, 0, 1);
        } else if ((parent->state_flags_low & 0x10) != 0) {
            FLOWBOXGIZMODATA_s *data = parent->data;
            for (i32 j = 0; j < data->gizmo_count; ++j)
                GizmoActivate(system->gizmo_sys, data->gizmos[j]->gizmo, 0, 1);
        }
        if ((parent->state_flags_low & 0x40) != 0) {
            parent->state_flags_high &= ~1;
            CheckIfParentsFinished(system, parent);
        }
        parent->state_flags_low &= ~0x20;
    }
}

static void ProcessFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);

static void ResetForLoopEx(GIZFLOW_s *flow, FLOWBOX_s *root, FLOWBOX_s *box, i32 checksum) {
    if (box != root && box->loop_checksum != checksum) {
        box->loop_checksum = checksum;
        if (box->type == 0 && (box->state_flags_high & 0x10) == 0 && box->data != NULL) {
            FLOWBOXGIZMODATA_s *data = box->data;
            for (i32 i = 0; i < data->gizmo_count; ++i) {
                GizmoActivate(flow->gizmo_sys, data->gizmos[i]->gizmo, 0, 1);
            }
        }
        for (i32 i = 0; i < box->child_count; ++i) {
            ResetForLoopEx(flow, root, box->children[i], checksum);
        }
    }
}

static __used__ void ResetGizmoFlowBox(GIZFLOW_s *giz_flow, FLOWBOX_s *flow_box) {
    FLOWBOXGIZMODATA_s &data = *flow_box->data;
    if ((flow_box->state_flags & 0x1000) != 0 || data.gizmo_count <= 0) {
        return;
    }
    for (i32 i = 0; i < data.gizmo_count; ++i) {
        GizmoActivate(giz_flow->gizmo_sys, data.gizmos[i]->gizmo, 0, 1);
    }
}

static i32 ProcessGizmoFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);
static i32 ProcessActionFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);
static i32 ProcessConditionFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);
static i32 CheckOutputGizmoFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);
static i32 CheckOutputActionFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);
static i32 CheckOutputConditionFlowBox(GIZFLOW_s *, FLOWBOX_s *, u8);
static u8 getNextLoopChecksum();
void PerformActionFlowBox(GIZFLOW_s *, FLOWBOX_s *);
void GizmoActivateReverse(GIZMOSYS_s *, GIZMO_s *, i32, i32, i32);

struct FLOWBOXTYPE {
    void (*reset)(GIZFLOW_s *, FLOWBOX_s *);
    i32 (*process)(GIZFLOW_s *, FLOWBOX_s *, u8);
    i32 (*check_output)(GIZFLOW_s *, FLOWBOX_s *, u8);
};
static FLOWBOXTYPE flowboxtypes[] = {
    {ResetGizmoFlowBox, ProcessGizmoFlowBox, CheckOutputGizmoFlowBox},
    {NULL, ProcessConditionFlowBox, CheckOutputConditionFlowBox},
    {NULL, ProcessActionFlowBox, CheckOutputActionFlowBox},
};
DECOMP_ASSERT(sizeof(flowboxtypes) == 0x24, "Flow-box callback table ABI");

static __used__ void ProcessFlowBox(GIZFLOW_s *system, FLOWBOX_s *box, u8 frame) {
    if (box->state_flags_high & 1) {
        i32 i;
        for (i = 0; i < box->parent_count; ++i) {
            FLOWBOX_s *parent = box->parents[i];
            if (!flowboxtypes[parent->type].check_output(system, parent, box->output_indices[0]))
                break;
        }
        FLOWBOXGIZMODATA_s *data = box->data;
        if (i == box->parent_count) {
            for (i32 j = 0; j < data->gizmo_count; ++j)
                GizmoActivateReverse(system->gizmo_sys, data->gizmos[j]->gizmo, 0, box->state_flags_low >> 7, 1);
        } else {
            for (i32 j = 0; j < data->gizmo_count; ++j)
                GizmoActivateReverse(system->gizmo_sys, data->gizmos[j]->gizmo, 1, box->state_flags_low >> 7, 1);
        }
    }
    if ((box->state_flags_low & 1) == 0)
        return;
    if (!flowboxtypes[box->type].process(system, box, frame))
        return;

    if ((box->state_flags_high & 0xc) == 0xc)
        box->state_flags_high &= ~4;
    box->state_flags_high |= 8;
    box->state_flags_low = (box->state_flags_low & ~1) | 2;
    if (box->type == 0) {
        if (box->state_flags_low & 0x18) {
            if (box->child_count != 0) {
                box->state_flags_low |= 0x20;
            } else {
                FLOWBOXGIZMODATA_s *data = box->data;
                if (box->state_flags_low & 8) {
                    for (i32 i = 0; i < data->gizmo_count; ++i)
                        GizmoSetVisibility(system->gizmo_sys, data->gizmos[i]->gizmo, 0, 1);
                } else if (box->state_flags_low & 0x10) {
                    for (i32 i = 0; i < data->gizmo_count; ++i)
                        GizmoActivate(system->gizmo_sys, data->gizmos[i]->gizmo, 0, 1);
                }
                box->state_flags_low &= ~0x20;
            }
        }
        if (box->state_flags_low & 0xc0) {
            box->state_flags_low |= 0x20;
            box->state_flags_high |= 1;
            FLOWBOX_s **parents = box->parents;
            i32 count = box->parent_count;
            for (i32 i = 0; i < count; ++i) {
                if (parents[i]->type == 1)
                    parents[i]->state_flags_high |= 2;
                if (parents[i]->type == 0 && (parents[i]->state_flags_low & 0x40)) {
                    parents[i]->state_flags_high |= 1;
                    parents[i]->state_flags_low |= 0x20;
                }
            }
        }
    }
    if (box->type != 0)
        CheckIfParentsFinished(system, box);
    for (i32 i = 0; i < box->child_count; ++i) {
        FLOWBOX_s **child = &box->children[i];
        (*child)->state_flags_low |= 1;
        if ((*child)->last_process_frame != frame || box->type == 1) {
            (*child)->last_process_frame = frame;
            ProcessFlowBox(system, *child, frame);
        }
    }
}

void ProcessGizFlow(GIZFLOW_s *system, float) {
    if (system == NULL)
        return;
    ++system->field_0x0c;
    FLOWBOX_s *box = system->flowboxes;
    for (i32 i = 0; i < system->flowbox_count; ++i, ++box) {
        if (box->last_process_frame != system->field_0x0c && (box->state_flags & 0x101))
            ProcessFlowBox(system, box, system->field_0x0c);
        box->last_process_frame = system->field_0x0c;
    }
}

static i32 ProcessGizmoFlowBox(GIZFLOW_s *system, FLOWBOX_s *box, u8) {
    FLOWBOX_s **parents = box->parents;
    u8 *outputs = box->output_indices;
    for (i32 i = 0; i < box->parent_count; ++i) {
        FLOWBOX_s *parent = parents[i];
        if (!flowboxtypes[parent->type].check_output(system, parent, outputs[i]))
            return 0;
    }
    CheckIfParentsFinished(system, box);
    if ((box->state_flags_high & 0x10) != 0 || ((box->state_flags_high & 0x40) == 0 && FreePlay == 0) ||
        ((box->state_flags_high & 0x20) == 0 && FreePlay != 0))
        return 1;
    FLOWBOXGIZMODATA_s *data = box->data;
    for (i32 i = 0; i < data->gizmo_count; ++i)
        GizmoActivate(system->gizmo_sys, data->gizmos[i]->gizmo, 1, 1);
    return 1;
}

static i32 ProcessActionFlowBox(GIZFLOW_s *system, FLOWBOX_s *box, u8) {
    FLOWBOX_s **parents = box->parents;
    u8 *outputs = box->output_indices;
    for (i32 i = 0; i < box->parent_count; ++i) {
        FLOWBOX_s *parent = parents[i];
        if (!flowboxtypes[parent->type].check_output(system, parent, outputs[i]))
            return 0;
    }
    PerformActionFlowBox(system, box);
    return 1;
}

static i32 ProcessConditionFlowBox(GIZFLOW_s *flow, FLOWBOX_s *box, u8) {
    u8 *condition = reinterpret_cast<u8 *>(box->data);
    if (condition == NULL) {
        FLOWBOX_s **parents = box->parents;
        u8 *outputs = box->output_indices;
        for (i32 i = 0; i < box->parent_count; ++i) {
            FLOWBOX_s *parent = parents[i];
            if (flowboxtypes[parent->type].check_output(flow, parent, outputs[i]) == 0) {
                return 0;
            }
        }
        return 1;
    }
    switch (condition[0]) {
        case 0: {
            FLOWBOX_s **parents = box->parents;
            u8 *outputs = box->output_indices;
            for (i32 i = 0; i < box->parent_count; ++i) {
                FLOWBOX_s *parent = parents[i];
                if (flowboxtypes[parent->type].check_output(flow, parent, outputs[i]) == 0) {
                    return 0;
                }
            }
            break;
        }
        case 2: {
            FLOWBOX_s **parents = box->parents;
            u8 *outputs = box->output_indices;
            for (i32 i = 0; i < box->parent_count; ++i) {
                FLOWBOX_s *parent = parents[i];
                if (flowboxtypes[parent->type].check_output(flow, parent, outputs[i]) != 0) {
                    return 0;
                }
            }
            break;
        }
        case 1:
        case 3:
        case 5: {
            const i32 required = condition[0] == 1 ? 1 : condition[1];
            FLOWBOX_s **parents = box->parents;
            u8 *outputs = box->output_indices;
            i32 count = 0;
            for (i32 i = 0; i < box->parent_count; ++i) {
                FLOWBOX_s *parent = parents[i];
                count += flowboxtypes[parent->type].check_output(flow, parent, outputs[i]) != 0;
            }
            if (condition[0] == 5) {
                return count == required;
            }
            if (count < required) {
                return 0;
            }
            break;
        }
        case 4: {
            i32 required = box->parent_count;
            if ((box->state_flags_high & 4) != 0) {
                required -= box->loop_parent_count;
            }
            if (box->parent_count != 0) {
                FLOWBOX_s **parents = box->parents;
                u8 *outputs = box->output_indices;
                i32 count = 0;
                for (i32 i = 0; i < box->parent_count; ++i) {
                    FLOWBOX_s *parent = parents[i];
                    count += flowboxtypes[parent->type].check_output(flow, parent, outputs[i]) != 0;
                }
                if (count < required) {
                    return 0;
                }
            }
            break;
        }
    }
    if (condition[0] == 4) {
        for (i32 i = 0; i < box->child_count; ++i) {
            const u8 checksum = getNextLoopChecksum();
            ResetForLoopEx(flow, box, box->children[i], checksum);
        }
    }
    return 1;
}

static i32 CheckOutputGizmoFlowBox(GIZFLOW_s *system, FLOWBOX_s *box, u8 output) {
    if (((box->state_flags_high & 0x40) == 0 && FreePlay == 0) ||
        ((box->state_flags_high & 0x20) == 0 && FreePlay != 0))
        return 1;
    FLOWBOXGIZMODATA_s *data = box->data;
    i32 result = 1;
    for (i32 i = 0; i < data->gizmo_count; ++i) {
        if (!GizmoGetOutput(system->gizmo_sys, data->gizmos[i]->gizmo, output, (box->state_flags_high >> 4) & 1))
            result = 0;
    }
    return result;
}

static i32 CheckOutputActionFlowBox(GIZFLOW_s *, FLOWBOX_s *box, u8) {
    return (box->state_flags_low >> 1) & 1;
}

static i32 CheckOutputConditionFlowBox(GIZFLOW_s *system, FLOWBOX_s *box, u8) {
    if ((box->state_flags_high & 2) == 0)
        return (box->state_flags_low >> 1) & 1;
    u8 saved = box->state_flags_high & 4;
    box->state_flags_high |= 4;
    i32 result = flowboxtypes[box->type].process(system, box, 0) != 0;
    box->state_flags_high = (box->state_flags_high & ~4) | saved;
    return result;
}

static i32 Loop_CountLoopingInputsEx(FLOWBOX_s *loop, FLOWBOX_s *box, i32 count, u8 checksum) {
    if (loop == box) {
        --box->loop_checksum;
        return count + 1;
    }
    for (i32 i = 0; i < box->child_count; ++i) {
        if (box->children[i]->loop_checksum != checksum) {
            box->children[i]->loop_checksum = checksum;
            count = Loop_CountLoopingInputsEx(loop, box->children[i], count, checksum);
        }
    }
    return count;
}

static u8 getNextLoopChecksum() {
    static u8 loop_checksum;
    return ++loop_checksum;
}

void DynamicAddGizmoToFlow(GIZFLOW_s *system, GIZMO_s *gizmo) {
    if (gizmo == NULL || system == NULL || gizmotypes == NULL || gizmo->type_id >= gizmotypes->count ||
        gizmotypes->types[gizmo->type_id].fns.get_gizmo_name_fn == NULL)
        return;
    FLOWBOX_s *box = system->flowboxes;
    for (i32 i = 0; i < system->flowbox_count; ++i, ++box) {
        FLOWBOXGIZMODATA_s *data = box->data;
        if (data == NULL)
            continue;
        for (i32 j = 0; j < data->gizmo_count; ++j) {
            if (data->gizmos[j]->gizmo != NULL)
                continue;
            char *name = gizmotypes->types[gizmo->type_id].fns.get_gizmo_name_fn(gizmo);
            if (NuStrICmp(data->gizmos[j]->name, name) != 0)
                continue;
            data->gizmos[j]->gizmo = gizmo;
            if ((box->state_flags_low & 1) == 0) {
                if (flowboxtypes[box->type].reset != NULL)
                    flowboxtypes[box->type].reset(system, box);
                if ((box->state_flags_low & 5) == 4 && box->type == 0)
                    GizmoSetVisibility(system->gizmo_sys, data->gizmos[j]->gizmo, 0, 1);
            }
        }
    }
}

void ResetGizFlowPointers(GIZFLOW_s *system) {
    if (system == NULL || system->flowbox_count == 0)
        return;
    FLOWBOX_s *box = system->flowboxes;
    for (i32 i = 0; i < system->flowbox_count; ++i, ++box) {
        if (box->type != 0)
            continue;
        FLOWBOXGIZMODATA_s *data = box->data;
        for (i32 j = 0; j < data->gizmo_count; ++j) {
            FLOWBOXGIZMOREF_s *ref = data->gizmos[j];
            if (ref->gizmo != NULL) {
                ref->gizmo = GizmoFindByName(system->gizmo_sys, ref->gizmo->type_id, ref->name);
            } else {
                ref->gizmo = GizmoFindByName(system->gizmo_sys, -1, ref->name);
                if (data->gizmos[j]->gizmo != NULL && (box->state_flags_low & 1) == 0) {
                    if (flowboxtypes[box->type].reset != NULL)
                        flowboxtypes[box->type].reset(system, box);
                    if ((box->state_flags_low & 5) == 4 && box->type == 0)
                        GizmoSetVisibility(system->gizmo_sys, data->gizmos[j]->gizmo, 0, 1);
                }
            }
        }
    }
}

void ResetGizFlow(GIZFLOW_s *system, GIZFLOWPROGRESS_s *progress) {
    if (system == NULL)
        return;
    system->field_0x0c = 0;
    FLOWBOX_s *box = system->flowboxes;
    ResetGizFlowPointers(system);
    for (i32 i = 0; i < system->flowbox_count; ++i, ++box) {
        box->last_process_frame = 0;
        box->state_flags_low &= ~0x20;
        if (progress != NULL && progress->valid != 0) {
            i32 word = i >> 5;
            u32 bit = 1u << (i & 31);
            box->state_flags_low = (box->state_flags_low & ~1) | ((progress->active[word] & bit) != 0);
            box->state_flags_low = (box->state_flags_low & ~2) | (((progress->completed[word] & bit) != 0) << 1);
            box->state_flags_low |= ((progress->latched[word] & bit) != 0) << 5;
            box->state_flags_high = (box->state_flags_high & ~4) | (((progress->output_state[word] & bit) != 0) << 2);
            if (box->type == 1 && box->condition_data[0] == 4) {
                u8 checksum = getNextLoopChecksum();
                i32 count = 0;
                for (i32 child = 0; child < box->child_count; ++child) {
                    count = Loop_CountLoopingInputsEx(box, box->children[child], count, checksum);
                }
                box->loop_parent_count = count;
            }
            continue;
        }
        box->state_flags_low &= ~2;
        box->state_flags_high &= ~1;
        if (box->type == 1 && box->condition_data[0] == 4) {
            u8 checksum = getNextLoopChecksum();
            i32 count = 0;
            for (i32 child = 0; child < box->child_count; ++child) {
                count = Loop_CountLoopingInputsEx(box, box->children[child], count, checksum);
            }
            box->loop_parent_count = count;
            box->state_flags_high |= 4;
            if (box->parent_count == box->loop_parent_count) {
                box->state_flags_high |= 8;
                box->state_flags_low |= 1;
            } else
                box->state_flags_low &= ~1;
        } else {
            box->state_flags_low = (box->state_flags_low & ~1) | (box->parent_count == 0);
        }

        if (flowboxtypes[box->type].reset != NULL && (box->state_flags_high & 0x10) == 0)
            flowboxtypes[box->type].reset(system, box);
        if (box->type == 0) {
            if (((box->state_flags_high & 0x40) == 0 && FreePlay == 0) ||
                ((box->state_flags_high & 0x20) == 0 && FreePlay != 0)) {
                FLOWBOXGIZMODATA_s *data = box->data;
                for (i32 gizmo = 0; gizmo < data->gizmo_count; ++gizmo)
                    GizmoSetVisibility(system->gizmo_sys, data->gizmos[gizmo]->gizmo, 0, 1);
            } else if ((box->state_flags_low & 5) == 4) {
                FLOWBOXGIZMODATA_s *data = box->data;
                for (i32 gizmo = 0; gizmo < data->gizmo_count; ++gizmo)
                    GizmoSetVisibility(system->gizmo_sys, data->gizmos[gizmo]->gizmo, 0, 1);
            }
        }
    }
}

// Original: 43 bytes.
i32 TwistLevel(LEVELDATA_s *level) {
    return DOGFIGHTA_LDATA != NULL && level == DOGFIGHTA_LDATA;
}

NUVEC *GizmoGetPos(GIZMOSYS_s *, GIZMO_s *gizmo) {
    if (gizmotypes != NULL && gizmo != NULL) {
        GIZMOGETPOSFN get_pos = gizmotypes->types[gizmo->type_id].fns.get_pos_fn;
        if (get_pos != NULL) {
            return get_pos(gizmo);
        }
    }

    return NULL;
}

i32 GizmoGetGuid(GIZMOSYS_s *, GIZMO_s *) {
    return -1;
}

char *GizmoGetName(GIZMO *gizmo) {
    GIZMOTYPES *types = gizmotypes;
    if (types == NULL || gizmo == NULL || gizmo->type_id >= types->count) {
        return NULL;
    }

    GIZMOGETGIZMONAMEFN get_name = types->types[gizmo->type_id].fns.get_gizmo_name_fn;
    if (get_name == NULL) {
        return NULL;
    }
    return get_name(gizmo);
}

void GizmoSysReset(GIZMOSYS *gizmo_sys, void *world, i32 progress_index) {
    if (gizmotypes != NULL && gizmo_sys != NULL) {
        GIZMOTYPE *type = gizmotypes->types;
        GIZMOSET *set = gizmo_sys->sets;

        for (i32 type_index = 0; type_index < gizmotypes->count; ++type_index, ++type, ++set) {
            if (type->fns.reset_fn != NULL) {
                if (progress_index >= 0) {
                    if (type->buffer != NULL) {
                        type->fns.reset_fn(world, set->unknown, type->buffer[progress_index].void_ptr);
                    } else {
                        type->fns.reset_fn(world, set->unknown, NULL);
                    }
                } else {
                    type->fns.reset_fn(world, set->unknown, NULL);
                }
            }
        }
    }
}

GIZMO *GizmoFindByData(GIZMOSYS *gizmo_sys, i32 type_id, void *data) {
    if (gizmotypes == NULL || data == NULL || gizmo_sys == NULL) {
        return NULL;
    }

    if (type_id >= 0 && type_id <= gizmotypes->count) {
        GIZMOTYPE &type = gizmotypes->types[type_id];
        GIZMOSET &set = gizmo_sys->sets[type_id];
        if (type.fns.get_gizmo_name_fn == NULL) {
            return NULL;
        }

        GIZMO *gizmo = set.gizmos;
        for (i32 i = 0; i < set.count; ++i, ++gizmo) {
            if (gizmo->object == data) {
                return gizmo;
            }
        }
        return NULL;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 type_index = 0; type_index < gizmotypes->count; ++type_index, ++type, ++set) {
        if (type->fns.get_gizmo_name_fn != NULL) {
            GIZMO *gizmo = set->gizmos;
            for (i32 i = 0; i < set->count; ++i, ++gizmo) {
                if (gizmo->object == data) {
                    return gizmo;
                }
            }
        }
    }
    return NULL;
}

GIZMO *GizmoFindByName(GIZMOSYS *gizmo_sys, i32 type_id, char *name) {
    if (gizmotypes == NULL || name == NULL || gizmo_sys == NULL) {
        return NULL;
    }

    if (type_id >= 0 && type_id <= gizmotypes->count) {
        GIZMOTYPE &type = gizmotypes->types[type_id];
        GIZMOSET &set = gizmo_sys->sets[type_id];
        if (type.fns.get_gizmo_name_fn == NULL) {
            return NULL;
        }

        GIZMO *gizmo = set.gizmos;
        for (i32 i = 0; i < set.count; ++i, ++gizmo) {
            if (NuStrICmp(type.fns.get_gizmo_name_fn(gizmo), name) == 0) {
                return gizmo;
            }
        }
        return NULL;
    }

    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = gizmo_sys->sets;
    for (i32 type_index = 0; type_index < gizmotypes->count; ++type_index, ++type, ++set) {
        if (type->fns.get_gizmo_name_fn == NULL) {
            continue;
        }

        GIZMO *gizmo = set->gizmos;
        for (i32 gizmo_index = 0; gizmo_index < set->count; ++gizmo_index, ++gizmo) {
            if (NuStrICmp(type->fns.get_gizmo_name_fn(gizmo), name) == 0) {
                return gizmo;
            }
        }
    }

    return NULL;
}

void GizmoSysSetGame() {
    GizObstacle_CheckExcludeFlagsFn = GizObstacle_CheckExcludeFlagsFn_LSW;
}

i32 GizmoSys_BoltHit(GIZMOSYS_s *, void *, BOLT_s *, nuvec_s *, nuvec_s *, nuvec_s *, float, unsigned char *) {
    return 0;
}

void ResetPaintPuzzle(WORLDINFO_s *) {
}

i32 GizmoFileReadName(char *name) {
    i32 name_length = EdFileReadChar();
    if (name_length == 0) {
        return 0;
    }

    EdFileRead(name, name_length);
    return 1;
}

i32 GizmoIsNameUnique(GIZMOSYS *gizmo_sys, char *name) {
    if (name == NULL || gizmo_sys == NULL) {
        return 1;
    }

    GIZMOSET *set = gizmo_sys->sets;
    GIZMOTYPE *type = gizmotypes->types;
    for (i32 type_index = 0; type_index < gizmotypes->count; ++type_index, ++type, ++set) {
        if (type->fns.get_gizmo_name_fn == NULL) {
            continue;
        }

        GIZMO *gizmo = set->gizmos;
        for (i32 gizmo_index = 0; gizmo_index < set->count; ++gizmo_index, ++gizmo) {
            if (NuStrICmp(name, type->fns.get_gizmo_name_fn(gizmo)) == 0) {
                return 0;
            }
        }
    }

    return 1;
}

void GizmoSysWriteInfo(GIZMOSYS_s *, char *, nugscn_s *) {
}

void UpdatePaintPuzzle(WORLDINFO_s *) {
}

void GizmoGetNumOutputs(GIZMOSYS_s *, GIZMO_s *) {
}

i32 GizmoGetUniqueName(GIZMOSYS *gizmo_sys, char *prefix, char *name, char *result, i32 result_size) {
    if (name == NULL || gizmo_sys == NULL || result == NULL) {
        return 0;
    }

    for (i32 type_index = 0; type_index < gizmotypes->count; ++type_index) {
        if (NuStrICmp(prefix, gizmotypes->types[type_index].prefix) != 0 &&
            GizmoNameUsesPrefix(name, gizmotypes->types[type_index].prefix)) {
            return 0;
        }
    }

    char suffix[16];
    char shortened_name[64];
    i32 name_length = NuStrLen(name);

    for (i32 suffix_number = 1; suffix_number != 999; ++suffix_number) {
        sprintf(suffix, "%d", suffix_number);
        i32 suffix_length = NuStrLen(suffix);

        if (name_length + suffix_length >= result_size) {
            name_length = result_size - suffix_length;
            if (name_length <= 0) {
                return 0;
            }

            NuStrNCpy(shortened_name, name, name_length);
            name = shortened_name;
        }

        sprintf(result, "%s%s", name, suffix);
        if (GizmoIsNameUnique(gizmo_sys, result)) {
            return 1;
        }
    }

    return 0;
}

i32 GizmoNameUsesPrefix(char *name, char *prefix) {
    if (prefix == NULL || name == NULL) {
        return 0;
    }

    while (*name != '\0') {
        if (NuToUpper(*name) != NuToUpper(*prefix)) {
            return 0;
        }
        ++prefix;
        ++name;
    }

    return 1;
}

void GizmoActivateReverse(GIZMOSYS_s *system, GIZMO_s *gizmo, i32 reverse, i32 visibility, i32) {
    const i32 command = visibility == 0 ? 2 : 6;
    const i32 query = visibility == 0 ? 3 : 7;
    if (gizmo == NULL || gizmotypes == NULL || gizmotypes->types[gizmo->type_id].fns.activate_rev_fn == NULL) {
        return;
    }
    if (gizmotypes->types[gizmo->type_id].fns.activate_rev_fn(gizmo, reverse, query) == 0) {
        return;
    }
    gizmotypes->types[gizmo->type_id].fns.activate_rev_fn(gizmo, reverse, command);
    LOG_INFO("gizmo reverse activation type=%s gizmo=%p reverse=%d visibility=%d",
             gizmotypes->types[gizmo->type_id].name, (void *)gizmo, reverse, visibility);
    if (visibility != 0) {
        GizmoSetVisibility(system, gizmo, reverse == 0, 1);
    }
}

u16 ObjHitObj_Flags(GameObject_s *object);
i32 GizmoSys_SetBestBoltTarget(GIZMOSYS_s *system, void *, GameObject_s *object, nuvec_s *position, nuvec_s *direction,
                               f32 radius, f32 range_squared, i32 directional, i32 planar, i32 bolt_id) {
    if (gizmotypes == NULL || object == NULL || system == NULL)
        return 0;
    if ((ObjHitObj_Flags(object) & 0x800) == 0)
        return 0;
    GIZMOTYPE *type = gizmotypes->types;
    GIZMOSET *set = system->sets;
    void *best = NULL;
    void *previous = NULL;
    f32 best_distance = 1.0e9f;
    NUVEC best_position, best_velocity, previous_position, previous_velocity;
    for (i32 i = 0; i < gizmotypes->count; ++i, ++type, ++set) {
        if (type->fns.get_best_bolt_target_fn == NULL)
            continue;
        f32 distance;
        NUVEC target_position, target_velocity;
        void *target =
            type->fns.get_best_bolt_target_fn(set, &distance, &target_position, &target_velocity, object, position,
                                              direction, radius, range_squared, directional, planar, bolt_id);
        if (target == NULL)
            continue;
        if (target == object->attack_gizmo_target) {
            previous = target;
            previous_position = target_position;
            previous_velocity = target_velocity;
        } else if (best_distance > distance) {
            best = target;
            best_position = target_position;
            best_velocity = target_velocity;
            best_distance = distance;
        }
    }
    if (best != NULL) {
        object->attack_target_position = best_position;
        object->attack_target_velocity = best_velocity;
        object->field_0xe21 |= 8;
        object->attack_gizmo_target = VehicleArea != 0 ? NULL : best;
        return 1;
    }
    if (previous != NULL) {
        object->attack_target_position = previous_position;
        object->attack_target_velocity = previous_velocity;
        object->field_0xe21 |= 8;
        object->attack_gizmo_target = VehicleArea != 0 ? NULL : previous;
        return 1;
    }
    return 0;
}
