#pragma once

#include "legoapi/gizmo/base/gizmo.h"

extern i32 spinner_gizmotype_id;

#ifdef __cplusplus
struct GIZSPINNER_s;

typedef struct SPINNER_s {
    char unknown_00[0x40];
    char name[0x40];
    char unknown_080[0x284];
} SPINNER;

ADDGIZMOTYPE *Spinner_RegisterGizmo(i32 type_id);
GIZSPINNER_s *GizSpinner_FindBySpecialName(void *world, char *name);
i32 GizSpinner_GetState(GIZSPINNER_s *spinner);
i32 GizSpinner_Update(GIZSPINNER_s *spinner);
void GizSpinner_GetSpinnerPos(GIZSPINNER_s *spinner, struct nuvec_s *position);

extern "C" {
#endif

#ifdef __cplusplus
}
#endif
