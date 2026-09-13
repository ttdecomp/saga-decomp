#include "legoapi/gizmo/base/gizmessage.h"

#include <stdio.h>
#include <string.h>

#include "decomp.h"
#include "globals.h"
#include "legoapi/ai/core/ai_sys_stubs.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nulist.h"
#include "nu2api/nucore/nustring.h"

i32 gizaimessage_gizmotype_id = -1;
static char gizaimessage_prefix[] = "msg_";

static i32 GizAIMessage_GetMaxGizmos(void *) {
    return 0x40;
}

static i32 GizAIMessage_GetOutput(GIZMO *gizmo, i32 output_index, i32) {
    if ((gizmo == NULL) || (gizmo->object == NULL) || ((u32)output_index > 7)) {
        return 0;
    }

    GIZAIMESSAGE_s *message = (GIZAIMESSAGE_s *)gizmo->object;
    return message->value == (f32)message->output_values[output_index];
}

static i32 GizAIMessage_GetNumOutputs(GIZMO *gizmo) {
    if ((gizmo == NULL) || (gizmo->object == NULL)) {
        return 0;
    }
    return ((GIZAIMESSAGE_s *)gizmo->object)->output_count;
}

static char *GizAIMessage_GetOutputName(GIZMO *gizmo, i32 output_index) {
    static char returnstr[4];

    if ((gizmo == NULL) || (gizmo->object == NULL) || ((u32)output_index > 7)) {
        return NULL;
    }

    GIZAIMESSAGE_s *message = (GIZAIMESSAGE_s *)gizmo->object;
    sprintf(returnstr, "%i", (i32)message->output_values[output_index]);
    return returnstr;
}

static void GizAIMessage_AddGizmos(GIZMOSYS *gizmo_sys, i32, void *, void *) {
    if (gizaimessagesys == NULL) {
        return;
    }

    GIZAIMESSAGE_s *message = (GIZAIMESSAGE_s *)NuLinkedListGetHead(&gizaimessagesys->active_list);
    while (message != NULL) {
        if ((message->flags & GIZAIMESSAGE_FLAG_ADD_GIZMO) != 0) {
            AddGizmo(gizmo_sys, gizaimessage_gizmotype_id, NULL, message);
            message->flags |= GIZAIMESSAGE_FLAG_GIZMO_ADDED;
        }
        message = (GIZAIMESSAGE_s *)NuLinkedListGetNext(&gizaimessagesys->active_list, &message->links);
    }
}

void ResetGizAIMessageSys(GIZAIMESSAGESYS_s *sys) {
    if (sys == NULL) {
        return;
    }
    sys->free_list.head = NULL;
    sys->free_list.tail = NULL;
    sys->active_list.head = NULL;
    sys->active_list.tail = NULL;
    memset(sys->messages, 0, (usize)sys->count * sizeof(GIZAIMESSAGE_s));
    for (i32 i = 0; i < sys->count; i++) {
        NuLinkedListAppend(&sys->free_list, &sys->messages[i].links);
    }
}

GIZAIMESSAGESYS_s *CreateGizAIMessageSys(VARIPTR *buf, VARIPTR *buf_end, i32 size) {
    GIZAIMESSAGESYS_s *sys = (GIZAIMESSAGESYS_s *)AISysBufferAlloc(buf, buf_end, 0x18);
    if (sys != NULL) {
        memset(sys, 0, 0x18);
        sys->messages = (GIZAIMESSAGE_s *)AISysBufferAlloc(buf, buf_end, (u32)size * 0x38);
        if (sys->messages != NULL) {
            sys->count = size;
            ResetGizAIMessageSys(sys);
        }
    }
    return sys;
}

void ClearGizAIMessageSys(GIZAIMESSAGESYS_s *sys) {
    if (sys == NULL) {
        return;
    }
    for (NULISTLNK *node = NuLinkedListGetHead(&sys->active_list); node != NULL;
         node = NuLinkedListGetNext(&sys->active_list, node)) {
        ((GIZAIMESSAGE_s *)node)->value = 0.0f;
    }
}

GIZAIMESSAGE_s *CheckGizAIMessage(GIZAIMESSAGESYS_s *sys, char const *name, GIZAIMESSAGE_s *out) {
    if (sys == NULL) {
        return NULL;
    }
    if (out != NULL) {
        return out;
    }
    if (name == NULL) {
        return NULL;
    }

    char local[0x20];
    if (NuStrIStr((char *)name, gizaimessage_prefix) != NULL) {
        strcpy(local, name);
    } else {
        if (NuStrLen(name) + NuStrLen(gizaimessage_prefix) > 0x1e) {
            return NULL;
        }
        sprintf(local, "%s%s", gizaimessage_prefix, name);
    }

    for (NULISTLNK *node = NuLinkedListGetHead(&sys->active_list); node != NULL;
         node = NuLinkedListGetNext(&sys->active_list, node)) {
        if (NuStrNICmp(local, ((const GIZAIMESSAGE_s *)node)->name, 0x20) == 0) {
            return (GIZAIMESSAGE_s *)node;
        }
    }

    NULISTLNK *node = NuLinkedListGetHead(&sys->free_list);
    if (node == NULL) {
        return NULL;
    }
    NuLinkedListRemove(&sys->free_list, node);
    NuLinkedListAppend(&sys->active_list, node);
    NuStrNCpy(((GIZAIMESSAGE_s *)node)->name, local, 0x20);
    return (GIZAIMESSAGE_s *)node;
}

GIZAIMESSAGE_s *SetGizAIMessage(GIZAIMESSAGESYS_s *sys, char const *name, float value, GIZAIMESSAGE_s *out) {
    GIZAIMESSAGE_s *msg = CheckGizAIMessage(sys, name, out);
    if (msg != NULL) {
        msg->value = value;
    }
    return msg;
}

float GetGizAIMessage(GIZAIMESSAGESYS_s *sys, char const *name, GIZAIMESSAGE_s *out) {
    GIZAIMESSAGE_s *msg = CheckGizAIMessage(sys, name, out);
    return (msg != NULL) ? msg->value : 0.0f;
}

GIZAIMESSAGE_s *QueryGizAIMessage(GIZAIMESSAGESYS_s *sys, GIZAIMESSAGE_s *msg) {
    if (msg != NULL) {
        return (GIZAIMESSAGE_s *)NuLinkedListGetNext(&sys->active_list, &msg->links);
    }
    return (GIZAIMESSAGE_s *)NuLinkedListGetHead(&sys->active_list);
}

char *GizAIMessage_GetName(GIZAIMESSAGE_s *msg) {
    return (msg != NULL) ? msg->name : NULL;
}

static char *GizAIMessage_GetGizmoName(GIZMO *gizmo) {
    if (gizmo == NULL) {
        return NULL;
    }
    return GizAIMessage_GetName((GIZAIMESSAGE_s *)gizmo->object);
}

ADDGIZMOTYPE *GizAIMessage_RegisterGizmo(i32 type_id) {
    static ADDGIZMOTYPE addtype;

    addtype = Default_ADDGIZMOTYPE;
    addtype.name = "Message";
    addtype.prefix = gizaimessage_prefix;
    addtype.fns.unknown1 = 0;
    addtype.fns.early_update_fn = NULL;
    addtype.fns.panel_draw_fn = NULL;
    addtype.fns.get_visibility_fn = NULL;
    addtype.fns.get_max_gizmos_fn = GizAIMessage_GetMaxGizmos;
    addtype.fns.get_pos_fn = NULL;
    addtype.fns.using_special_fn = NULL;
    addtype.fns.add_gizmos_fn = GizAIMessage_AddGizmos;
    addtype.fns.bolt_hit_plat_fn = NULL;
    addtype.fns.get_best_bolt_target_fn = NULL;
    addtype.fns.late_update_fn = NULL;
    addtype.fns.bolt_hit_fn = NULL;
    addtype.fns.draw_fn = NULL;
    addtype.fns.get_gizmo_name_fn = GizAIMessage_GetGizmoName;
    addtype.fns.get_output_fn = GizAIMessage_GetOutput;
    addtype.fns.get_output_name_fn = GizAIMessage_GetOutputName;
    addtype.fns.get_num_outputs_fn = GizAIMessage_GetNumOutputs;
    addtype.fns.activate_fn = NULL;
    addtype.fns.activate_rev_fn = NULL;
    addtype.fns.set_visibility_fn = NULL;
    addtype.fns.allocate_progress_data_fn = NULL;
    addtype.fns.clear_progress_fn = NULL;
    addtype.fns.store_progress_fn = NULL;
    addtype.fns.reset_fn = NULL;
    addtype.fns.reserve_buffer_space_fn = NULL;
    addtype.fns.load_fn = NULL;
    addtype.fns.post_load_fn = NULL;
    addtype.fns.add_level_sfx_fn = NULL;
    gizaimessage_gizmotype_id = type_id;

    return &addtype;
}
