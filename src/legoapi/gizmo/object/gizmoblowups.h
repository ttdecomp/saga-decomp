#pragma once

#include "nu2api/nucore/fixed_width.h"

struct WORLDINFO_s;
struct nuvec_s;

void GizmoBlowUpTypeBlowUp(WORLDINFO_s *world, i32 type, nuvec_s *position);
i32 GizmoBlowupGetNameTableId(char *name);
i32 GizmoBlowupGetTypeFromNameTableId(WORLDINFO_s *world, i32 name_table_id);
