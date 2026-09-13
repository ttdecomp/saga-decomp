#include "legoapi/world/world_shared.h"
#include "decomp.h"
#include "globals.h"
#include "gameapi/edtools/edfile.h"
#include "legoapi/gizmo/base/gizmo.h"

#include <stdio.h>
#include <string.h>
struct FLOWBOX_s;
void ResetGizFlowPointers(GIZFLOW_s *giz_flow);
void GizmoActivateReverse(GIZMOSYS_s *, GIZMO_s *, i32, i32, i32);

i32 gizmoerrorlogsize = 0x800;

GIZMOSYS *CreateGizmoSys(void *world, VARIPTR *buf, VARIPTR *buf_end) {
    GIZMOSYS *gizmo_sys = NULL;

    if (gizmotypes != NULL) {
        gizmo_sys = reinterpret_cast<GIZMOSYS *>(GizmoBufferAlloc(buf, buf_end, sizeof(GIZMOSYS)));
        if (gizmo_sys != NULL) {
            gizmo_sys->sets = reinterpret_cast<GIZMOSET *>(
                GizmoBufferAlloc(buf, buf_end, gizmotypes->count * static_cast<i32>(sizeof(GIZMOSET))));

            if (gizmo_sys->sets != NULL) {
                GIZMOSET *set = gizmo_sys->sets;
                GIZMOTYPE *type = gizmotypes->types;
                for (i32 type_index = 0; type_index < gizmotypes->count; ++type_index, ++type, ++set) {
                    set->type = type;

                    if (type->fns.get_max_gizmos_fn != NULL) {
                        set->max_count = type->fns.get_max_gizmos_fn(world);
                    }
                    if (set->max_count != 0) {
                        set->gizmos = reinterpret_cast<GIZMO *>(
                            GizmoBufferAlloc(buf, buf_end, set->max_count * static_cast<i32>(sizeof(GIZMO))));
                    }
                    if (type->fns.reserve_buffer_space_fn != NULL) {
                        set->unknown = type->fns.reserve_buffer_space_fn(world);
                    }
                }
            }

            if (gizmoerrorlogsize != 0) {
                gizmo_sys->error_log = reinterpret_cast<char *>(GizmoBufferAlloc(buf, buf_end, gizmoerrorlogsize));
            }
        }
    }

    return gizmo_sys;
}
void LoadGizmoSys(GIZMOSYS_s *gizmo_sys, void *world, char *config_file) {
    if (gizmo_sys != NULL) {
        gizmo_sys->flags |= GIZMOSYS_FLAG_LOADING;
        if (gizmo_sys->error_log != NULL) {
            memset(gizmo_sys->error_log, 0, gizmoerrorlogsize);
        }
        gizmo_sys->flags &= ~3;

        if (gizmotypes->count != 0) {
            char gizmo_name[32];
            char path[256];
            sprintf(path, "%s.giz", config_file);

            EdFileSetMedia(1);
            if (EdFileOpen(path, NUFILE_READ) != 0) {
                EdFileReadInt();
                i32 name_length = EdFileReadInt();
                while (name_length != 0) {
                    memset(gizmo_name, 0, sizeof(gizmo_name));
                    EdFileRead(gizmo_name, name_length);

                    i32 data_length = EdFileReadInt();
                    i32 type_id = GizmoGetTypeIDByName(gizmo_sys, gizmo_name);
                    if (data_length > 0 && type_id >= 0 && type_id < gizmotypes->count &&
                        gizmotypes->types[type_id].fns.load_fn != NULL &&
                        gizmotypes->types[type_id].fns.load_fn(world, gizmo_sys->sets[type_id].unknown) != 0) {
                        name_length = EdFileReadInt();
                        continue;
                    }

                    while (data_length != 0) {
                        EdFileReadChar();
                        --data_length;
                    }
                    name_length = EdFileReadInt();
                }
                EdFileClose();
            }

            GIZMOTYPE *type = gizmotypes->types;
            GIZMOSET *set = gizmo_sys->sets;
            for (i32 type_id = 0; type_id < gizmotypes->count; ++type_id, ++type, ++set) {
                if (type->fns.post_load_fn != NULL) {
                    type->fns.post_load_fn(world, set->unknown);
                }
            }
        }

        gizmo_sys->flags &= ~GIZMOSYS_FLAG_LOADING;
    }
}
void Hub_LoadAndFixUpMiniKits(WORLDINFO *world, VARIPTR *buf, VARIPTR *buf_end) {
    (void)world;
    (void)buf;
    (void)buf_end;
}
void MiniKit_Load(MINIKIT *minikit, i32 id, VARIPTR *buf, VARIPTR *buf_end, void *param) {
    (void)minikit;
    (void)id;
    (void)buf;
    (void)buf_end;
    (void)param;
}
void MiniKit_InitPieces(MINIKIT *minikit, i32 count, VARIPTR *buf, VARIPTR *buf_end) {
    (void)minikit;
    (void)count;
    (void)buf;
    (void)buf_end;
}
void CharacterMiniKits_Load(COLLECTION_s *collection, WORLDINFO *world, VARIPTR *buf, VARIPTR *buf_end) {
    (void)collection;
    (void)world;
    (void)buf;
    (void)buf_end;
}
void GizmoSysAddGizmos(GIZMOSYS_s *gizmo_sys, GIZFLOW_s *giz_flow, void *world) {
    if (gizmotypes != NULL && gizmo_sys != NULL) {
        GIZMOTYPE *type = gizmotypes->types;
        GIZMOSET *set = gizmo_sys->sets;
        for (i32 type_id = 0; type_id < gizmotypes->count; ++type_id, ++type, ++set) {
            ResetGizmoType(gizmo_sys, type_id, NULL);
            if (type->fns.add_gizmos_fn != NULL) {
                type->fns.add_gizmos_fn(gizmo_sys, type_id, world, set->unknown);
            }
        }
    }

    if (giz_flow != NULL && giz_flow->pointers_need_reset != 0) {
        ResetGizFlowPointers(giz_flow);
    }
}
