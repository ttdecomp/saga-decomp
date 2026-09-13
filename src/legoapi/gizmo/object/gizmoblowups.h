#pragma once

#include "nu2api/nucore/fixed_width.h"

struct WORLDINFO_s;
struct GIZMOBLOWUP_s;
struct GameObject_s;
struct nuvec_s;
struct nuinstanim_s;

extern i32 (*BlowupExFunc)(GIZMOBLOWUP_s *, i32);
extern u32 EXBLOWUPFLAGS;
extern i32 (*GizmoBlowUp_NoTargetFn)(WORLDINFO_s *, GameObject_s *);
extern void (*GizmoBlowUp_SfxFn)(GIZMOBLOWUP_s *, nuvec_s *);
extern void (*GizmoBlowup_TransformDrawFn)(GIZMOBLOWUP_s *);
extern void (*GameBlowUpBlownUpFn)(GIZMOBLOWUP_s *);

void SetLevelExBlowupFlags(u32 flags);
u32 GetLevelExBlowupFlags(void);

void GizmoBlowUpTypeBlowUp(WORLDINFO_s *world, i32 type, nuvec_s *position);
void GizmoBlowupResetNameTable(void);
void UpdateMidPos(GIZMOBLOWUP_s *blowup);
void PlayAnim(nuinstanim_s *animation, float *playback, float speed, i32 backwards);
i32 GizmoBlowupGetNameTableId(char *name);
i32 GizmoBlowupGetTypeFromNameTableId(WORLDINFO_s *world, i32 name_table_id);
