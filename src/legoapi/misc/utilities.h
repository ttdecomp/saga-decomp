#pragma once

#include "nu2api/nucore/fixed_width.h"

struct nuvec_s;
struct numtx_s;

void MakeThrowVector(nuvec_s *result, nuvec_s *origin, nuvec_s *target, nuvec_s *target_velocity, f32 speed,
                     f32 gravity);
i32 MatrixReflection(numtx_s *matrix, i32 axis, f32 plane, f32 height, numtx_s *result);
