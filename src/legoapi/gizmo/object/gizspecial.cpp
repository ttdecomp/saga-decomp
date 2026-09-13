#include "legoapi/gizmos/trigger/gizspecial.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nu3d/nuspecial.h"

static void GizmoAppendSpecialError(GIZMOSYS *gizmo_sys, const char *text) {
    if (text == NULL || gizmo_sys->error_log == NULL) {
        return;
    }

    if (NuStrLen(gizmo_sys->error_log) + NuStrLen(text) <= gizmoerrorlogsize) {
        NuStrNCat(gizmo_sys->error_log, text, gizmoerrorlogsize);
    } else {
        gizmo_sys->flags |= GIZMOSYS_FLAG_ERROR_LOG_OVERFLOW;
    }
}

i32 Gizmo_FindNuSpecial(nugscn_s *scene, nuhspecial_s *special, char *name, i32 flags, GIZMOSYS *gizmo_sys,
                        char *prefix, char *suffix) {
    if (NuSpecialFind(scene, special, name, flags) != 0) {
        return 1;
    }

    if (gizmo_sys == NULL || gizmo_sys->error_log == NULL || (gizmo_sys->flags & GIZMOSYS_FLAG_LOADING) == 0) {
        return 0;
    }

    GizmoAppendSpecialError(gizmo_sys, name);
    if (prefix != NULL) {
        GizmoAppendSpecialError(gizmo_sys, "\t\t");
        GizmoAppendSpecialError(gizmo_sys, prefix);
    }
    if (suffix != NULL) {
        GizmoAppendSpecialError(gizmo_sys, "\t\t");
        GizmoAppendSpecialError(gizmo_sys, suffix);
    }
    GizmoAppendSpecialError(gizmo_sys, "\n");
    return 0;
}

void ReleaseAllTakeOvers() {
}

i32 GizmoGetGizmosUsingSpecial(GIZMOSYS *gizmo_sys, void *world, GIZMO **result, i32 result_capacity, char *name) {
    if (gizmo_sys == NULL || gizmotypes == NULL || result == NULL || gizmotypes->count <= 0 || result_capacity <= 0) {
        return 0;
    }

    i32 type_index = 0;
    i32 result_count = 0;
    for (; type_index < gizmotypes->count && result_count < result_capacity; ++type_index) {
        GIZMOUSINGSPECIALFN using_special = gizmotypes->types[type_index].fns.using_special_fn;
        if (using_special != NULL) {
            const i32 added = using_special(result + result_count, world, result_capacity - result_count, name);
            if (added == -1) {
                return result_capacity;
            }
            result_count += added;
            if (result_count > result_capacity) {
                return result_capacity;
            }
        }
    }
    return result_count;
}
