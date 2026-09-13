#pragma once

#include "legoapi/gizmo/base/gizmo.h"

extern i32 gizaimessage_gizmotype_id;

ADDGIZMOTYPE *GizAIMessage_RegisterGizmo(i32 type_id);
void ResetGizAIMessageSys(GIZAIMESSAGESYS_s *sys);
GIZAIMESSAGESYS_s *CreateGizAIMessageSys(VARIPTR *buf, VARIPTR *buf_end, i32 size);
void ClearGizAIMessageSys(GIZAIMESSAGESYS_s *sys);
GIZAIMESSAGE_s *CheckGizAIMessage(GIZAIMESSAGESYS_s *sys, char const *name, GIZAIMESSAGE_s *out);
GIZAIMESSAGE_s *SetGizAIMessage(GIZAIMESSAGESYS_s *sys, char const *name, float value, GIZAIMESSAGE_s *out);
float GetGizAIMessage(GIZAIMESSAGESYS_s *sys, char const *name, GIZAIMESSAGE_s *out);
GIZAIMESSAGE_s *QueryGizAIMessage(GIZAIMESSAGESYS_s *sys, GIZAIMESSAGE_s *msg);
char *GizAIMessage_GetName(GIZAIMESSAGE_s *msg);
