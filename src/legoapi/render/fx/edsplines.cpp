#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/motion.h"
#include "nu2api/numath/nufloat.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx/spline_position.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspline.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

static f32 curx;
static f32 cury;
static f32 nextx;
static f32 dx;
i32 size;

void setnextpoint(float x, float y) {
    nextx = x;
    dx = (x - curx) / y;
    size = static_cast<i32>(y);
}

void BezierLinePos(VuVec &, VuVec &, VuVec &, VuVec &, VuVec &, float) {
}

void BezierLineEval(VuVec &, VuVec &, VuVec &, VuVec &, VuVec &, float) {
}

void CalcSplinePoint(flightspline_s *, _vuv_s *, float) {
}

void BezierLineLength(VuVec &, VuVec &, VuVec &, VuVec &) {
}

void BezierLineLength(VuVec &, VuVec &, VuVec &, VuVec &, float) {
}


static void SplinePointAngles(NUGSPLINE *spline, i32 index, i32 looping, u16 *pitch, u16 *angle) {
    NUVEC *current = &spline->pts[index];
    i32 previous_index = index - 1;
    NUVEC *previous = NULL;
    if (looping && previous_index < 0)
        previous = &spline->pts[spline->length - 1];
    else if (previous_index >= 0)
        previous = &spline->pts[previous_index];
    NUVEC direction = {0.0f, 0.0f, 0.0f};
    if (previous != NULL) {
        direction.x += current->x - previous->x;
        direction.y += current->y - previous->y;
        direction.z += current->z - previous->z;
    }
    NUVEC *next = NULL;
    if (index + 1 < spline->length)
        next = &spline->pts[index + 1];
    else if (looping)
        next = spline->pts;
    if (next != NULL) {
        direction.x += next->x - current->x;
        direction.y += next->y - current->y;
        direction.z += next->z - current->z;
    }
    if (pitch != NULL)
        *pitch = NuAtan2D(direction.y, NuFsqrt(direction.x * direction.x + direction.z * direction.z));
    if (angle != NULL)
        *angle = NuAtan2D(direction.x, direction.z);
}

void PointAlongSpline(NUGSPLINE *spline, f32 along, NUVEC *position, u16 *angle, u16 *pitch, i32 looping) {
    if (angle != NULL)
        *angle = 0;
    if (pitch != NULL)
        *pitch = 0;
    if (spline == NULL)
        return;
    if (along > 1.0f)
        along = 1.0f;
    else if (along < 0.0f)
        along = 0.0f;
    u32 extent = (u32)(looping ? spline->length : spline->length - 1) << 16;
    u32 fixed = (i32)((f32)extent * along);
    i32 index = fixed >> 16;
    NUVEC *current = &spline->pts[index];
    *position = *current;
    if (pitch != NULL || angle != NULL)
        SplinePointAngles(spline, index, looping, pitch, angle);
    index++;
    if (index >= spline->length) {
        if (!looping)
            return;
        index = 0;
    }
    fixed &= 0xffff;
    if (fixed == 0)
        return;
    f32 fraction = fixed * (1.0f / 65536.0f);
    NUVEC *next = &spline->pts[index];
    position->x += (next->x - current->x) * fraction;
    position->y += (next->y - current->y) * fraction;
    position->z += (next->z - current->z) * fraction;
    if (angle != NULL || pitch != NULL) {
        u16 next_angle, next_pitch;
        SplinePointAngles(spline, index, looping, pitch != NULL ? &next_pitch : NULL,
                          angle != NULL ? &next_angle : NULL);
        if (angle != NULL)
            *angle += (i32)(RotDiff(*angle, next_angle) * fraction);
        if (pitch != NULL)
            *pitch += (i32)(RotDiff(*pitch, next_pitch) * fraction);
    }
}

i32 getnextdatapoint(float *value, i32 *delta) {
    const f32 next_y = cury + 1.0f;
    const f32 old_x = curx;
    *value = old_x;
    const i32 old_value = static_cast<i32>(old_x);
    cury = next_y;
    --size;
    const f32 next_x = old_x + dx;
    curx = next_x;
    *delta = static_cast<i32>(next_x) - old_value;
    if (size != 0) {
        return 0;
    }
    curx = nextx;
    return -1;
}

void FlightSpline_Init(WORLDINFO_s *, flightspline_s *, i32) {
}

i32 LineIntersectXY(NUVEC *, NUVEC *, NUVEC *, NUVEC *, NUVEC *, NUVEC *);

i32 OutSideSplineArea(nuvec_s *position, nugspline_s *spline, nuvec_s *edge_end, nuvec_s *edge_start, i32 inside) {
    if (spline == NULL || position == NULL)
        return 0;
    NUVEC ray_start = {position->x, -position->z, position->y};
    NUVEC ray_end = {position->x, 100000.0f, position->y};
    NUVEC end = {spline->pts[0].x, -spline->pts[0].z, spline->pts[0].y};
    i32 intersections = 0;
    for (i32 i = 1; i < spline->length; ++i) {
        NUVEC start = end;
        end.x = spline->pts[i].x;
        end.y = -spline->pts[i].z;
        end.z = spline->pts[i].y;
        intersections += LineIntersectXY(&ray_start, &ray_end, &start, &end, NULL, NULL);
    }
    if ((intersections & 1) != 0 ? inside == 0 : inside != 0)
        return 0;
    i32 closest_index = 0;
    NUVEC closest = spline->pts[0];
    f32 best = 1000000000.0f;
    for (i32 i = 0; i < spline->length - 1; ++i) {
        f32 dx = spline->pts[i].x - position->x;
        f32 dz = spline->pts[i].z - position->z;
        f32 distance = dx * dx + dz * dz;
        if (distance < best) {
            closest_index = i;
            closest = spline->pts[i];
            best = distance;
        }
    }
    i32 previous = closest_index == 0 ? spline->length - 2 : closest_index - 1;
    i32 next = closest_index == spline->length - 2 ? 0 : closest_index + 1;
    NUVEC before = spline->pts[previous];
    NUVEC after = spline->pts[next];
    f32 ax = after.x - position->x;
    f32 az = after.z - position->z;
    f32 bx = before.x - position->x;
    f32 bz = before.z - position->z;
    if (bx * bx + bz * bz > ax * ax + az * az) {
        if (edge_end != NULL)
            *edge_end = after;
        if (edge_start != NULL)
            *edge_start = closest;
    } else {
        if (edge_end != NULL)
            *edge_end = closest;
        if (edge_start != NULL)
            *edge_start = before;
    }
    return 1;
}

void InitSplinePosition(SPLINEPOS_s *position, nugspline_s *spline, float distance, i32 looping) {
    if (position == NULL) {
        return;
    }

    SPLINEPOS_s *runtime = position;
    memset(runtime, 0, sizeof(*runtime));
    if (spline == NULL || spline->length < 2) {
        return;
    }

    runtime->spline = spline;
    runtime->looping = static_cast<u8>(looping);
    const NUVEC *first = reinterpret_cast<const NUVEC *>(reinterpret_cast<const u8 *>(spline->pts));
    const NUVEC *second = reinterpret_cast<const NUVEC *>(reinterpret_cast<const u8 *>(spline->pts) + spline->pt_size);
    runtime->segment_length = NuVecDist(const_cast<NUVEC *>(second), const_cast<NUVEC *>(first), NULL);
    if (distance > 0.0f) {
        MoveSplinePosition(position, distance);
    } else {
        runtime->position = *first;
        runtime->along = 0.0f;
    }
}

void GetNearestSplinePos(NUVEC *origin, SPLINEPOS_s *result, NUGSPLINE *spline, i32 looping, i16 first_point,
                         i16 last_point) {
    if (result == NULL)
        return;
    memset(result, 0, sizeof(*result));
    if (spline == NULL || origin == NULL || spline->length <= 1)
        return;
    result->spline = spline;
    result->looping = (i8)looping;
    i32 point_count = spline->length;
    i32 logical_count = result->looping != 0 ? point_count + 1 : point_count;
    i32 index = first_point;
    if (index < 0)
        index = 0;
    else if (index >= logical_count)
        return;
    i32 end = point_count;
    if (last_point >= 0 && last_point < end)
        end = last_point;
    NUVEC *point = (NUVEC *)((u8 *)spline->pts + index * (i16)spline->pt_size);
    f32 nearest = 1000000000.0f;
    NUVEC offset;
    do {
        f32 distance = NuVecDistSqr(origin, point, &offset);
        if (distance < nearest) {
            nearest = distance;
            result->segment = index;
        }
        point = (NUVEC *)((u8 *)point + (i16)result->spline->pt_size);
        index++;
    } while (index < end);
    spline = result->spline;
    i32 stride = (i16)spline->pt_size;
    NUVEC *current = (NUVEC *)((u8 *)spline->pts + result->segment * stride);
    NUVEC *next = (NUVEC *)((u8 *)spline->pts + ((result->segment + 1) % spline->length) * stride);
    result->segment_distance = 0.0f;
    result->segment_length = NuVecDist(next, current, &offset);
    result->position = *current;
    result->along = (result->segment_distance / result->segment_length + result->segment) / (logical_count - 1);
}

void CalcSplinePointFromDist(flightspline_s *, _vuv_s *, float) {
}

static LEVELSPLINE *LevSplList;
static i32 LEVELSPLINECOUNT;
static i32 levspl_i_start = -1;
static i32 levspl_i_startcam = -1;

void LevelSplines_InitForGame(LEVELSPLINE *splines) {
    LevSplList = splines;
    LEVELSPLINECOUNT = 0;
    levspl_i_start = -1;
    levspl_i_startcam = -1;

    if (splines == NULL) {
        return;
    }

    for (LEVELSPLINE *spline = splines; spline->name != NULL; ++spline) {
        if (levspl_i_start == -1 && NuStrICmp(spline->name, "start") == 0) {
            levspl_i_start = LEVELSPLINECOUNT;
        }
        if (levspl_i_startcam == -1 && NuStrICmp(spline->name, "start_cam") == 0) {
            levspl_i_startcam = LEVELSPLINECOUNT;
        }
        ++LEVELSPLINECOUNT;
    }
}

void EvaluateSplineXZIntersection(nugspline_s *, i32, SPLINEPOS_s *, nugspline_s *, i32, SPLINEPOS_s *) {
}

void setpoint(float x) {
    curx = x;
    cury = 0.0f;
}

static __used__ f32 SplineLength(nugspline_s *, i32) {
    return 0.0f;
}

void LevelSplines_InitForLevel(WORLDINFO_s *world) {
    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    world->portal_places = reinterpret_cast<PORTALPOS **>(world->giz_buffer.void_ptr);
    world->giz_buffer.addr += LEVELSPLINECOUNT * sizeof(*world->portal_places);
    memset(world->portal_places, 0, LEVELSPLINECOUNT * sizeof(*world->portal_places));

    if (LevSplList == NULL) {
        return;
    }

    for (i32 i = 0; i < LEVELSPLINECOUNT; ++i) {
        LEVELSPLINE *entry = &LevSplList[i];
        if ((entry->area != -1 && world->level_sub_id != entry->area) ||
            (entry->level != -1 && world->level_idx != entry->level)) {
            continue;
        }

        NUGSCN *scene = entry->scene != NULL ? *entry->scene : world->current_gscn;
        if (scene == NULL) {
            continue;
        }

        NUGSPLINE *spline = NuSplineFind(scene, const_cast<char *>(entry->name));
        world->portal_places[i] = reinterpret_cast<PORTALPOS *>(spline);
        if (spline == NULL) {
            continue;
        }

        const i32 point_count = spline->length;
        if ((entry->min_points != 0 && point_count < entry->min_points) ||
            (entry->max_points != 0 && entry->min_points <= entry->max_points && point_count > entry->max_points)) {
            world->portal_places[i] = NULL;
        }
    }
}
