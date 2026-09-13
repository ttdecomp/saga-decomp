#pragma once

#include "nu2api/nucore/fixed_width.h"

struct nuvec_s;
struct numtl_s;

void DrawRopeCurved(nuvec_s *start, nuvec_s *points, i32 point_count, i32 flags, numtl_s *material);
