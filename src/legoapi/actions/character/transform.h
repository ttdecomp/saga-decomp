#pragma once

#include "nu2api/nucore/fixed_width.h"

struct GIZMOBLOWUP_s;
struct nuvec_s;

void Transform_DrawTarget(nuvec_s *position, float radius, float alpha);
i32 Transform_TargettedByObj(void *object);
void GizmoBlowup_TransformDraw_Game(GIZMOBLOWUP_s *blowup);
