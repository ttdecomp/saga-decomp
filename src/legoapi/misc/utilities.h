#pragma once

#include "nu2api/nucore/fixed_width.h"

struct nuvec_s;
struct numtx_s;

void MakeThrowVector(nuvec_s *result, nuvec_s *origin, nuvec_s *target, nuvec_s *target_velocity, f32 speed,
                     f32 gravity);
i32 MatrixReflection(numtx_s *matrix, i32 axis, f32 plane, f32 height, numtx_s *result);
i32 MatrixReflectionVU0_AXISY(numtx_s *matrix, f32 plane, f32 scale, numtx_s *result);
void FindAnglesXY(nuvec_s *direction, u16 *x_rotation, u16 *y_rotation);
void FindAnglesZX(nuvec_s *normal, u16 *x_rotation, u16 *z_rotation);
void CalculateInterceptVector(nuvec_s *origin, nuvec_s *target, nuvec_s *velocity, f32 speed,
                              nuvec_s *direction, nuvec_s *intercept_velocity);
i32 SphereSphereOverlapScaleY(nuvec_s *position_a, f32 radius_a, f32 y_radius_a, nuvec_s *position_b,
                              f32 radius_b, f32 y_radius_b);
