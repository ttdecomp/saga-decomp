#include "legoapi/props/system/socksys.h"
f32 CalculateDistanceToSpecificSideOrEnd(i32, NUVEC *, SOCKPOSITION *, i32, SOCKSYS *);
void TurnOffAllSocksExcept(SOCKSYS *, i32);
void RestoreLastSocksTurnoff(SOCKSYS *);
struct SOCKCAMERARESULT {
    SOCKPOSITION socket;
    NUVEC position, target;
    f32 blend, position_seek, angle_seek, shake, separation;
};
DECOMP_ASSERT(sizeof(SOCKCAMERARESULT) == 0x64, "Socket camera result ABI");
#include "legoapi/items/base/apiobject.h"

#include <stdio.h>
#include <string.h>

#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
struct GameObject_s;

extern "C" void PerspectMidPoint(NUVEC *result, NUVEC *first, NUVEC *second, NUVEC *camera_position);
extern "C" f32 NewShadow(NUVEC *position, f32 height_above, f32 height_below, i32 terrain_mask);
extern "C" NUFPCOMJMPCTX SockSys_ConfigKeywords[];

extern "C" {
    NUGSCN *sockpar_scene;
    SOCK *sockpar_sock;
}

static VARIPTR *sockpar_buffer_ptr;
static VARIPTR *sockpar_buffer_end;

// Spline name prefixes for a socket's rails; the socket index is appended
// zero-padded, e.g. "sock_cam_03". Looked up against the scene splines in
// SockSysFindInScene.
static const char *tSockCAM = "sock_cam_";
static const char *tSockA = "sock_a_";
static const char *tSockB = "sock_b_";
static const char *tSockC = "sock_c_";
static const char *tSockD = "sock_d_";
static const char *tSockMID = "sock_mid_";
static const char *tSockLEFT = "sock_left_";
static const char *tSockRIGHT = "sock_right_";
static const char *tSockLOOK = "sock_look_";
static const char *tSockLATERAL = "sock_lateral_";
static const char *tSockTRACKIN = "sock_trackin_";
static const char *tSockLIMIT = "sock_limit_";

// Grows `min`/`max` to include `pnt`.
static void AdjustMinMaxBox(NUVEC *pnt, NUVEC *min, NUVEC *max) {
    if (pnt->x < min->x) {
        min->x = pnt->x;
    }
    if (max->x < pnt->x) {
        max->x = pnt->x;
    }
    if (pnt->y < min->y) {
        min->y = pnt->y;
    }
    if (max->y < pnt->y) {
        max->y = pnt->y;
    }
    if (pnt->z < min->z) {
        min->z = pnt->z;
    }
    if (max->z < pnt->z) {
        max->z = pnt->z;
    }
}

extern "C" {

    void SockSysFindInScene(SOCKSYS *sock_sys, NUGSCN *gscn) {
        i32 i;
        i32 j;
        SOCK *sock;
        i32 len;
        float extent_z;
        char name[256];

        if ((gscn != NULL) && (sock_sys != NULL)) {
            sock = sock_sys->sock;
            for (i = 0; i < 0x40; i++, sock++) {
                if (sock->cam != NULL) {
                    continue;
                }
                sock->unknown_7c = 1.0f;
                sock->unknown_80 = 1.0f;
                sock->unknown_84 = 1.0f;
                sock->look_ratio_xz = 1.0f;
                sock->look_ratio_y = 1.0f;
                sock->camera_position_seek = 5.0f;
                sock->camera_angle_seek = 5.0f;
                sprintf(sock->name, "%.2i", i);
                sock->overlap_blend_ratio = 0.5f;
                sprintf(name, "%s%.2i", tSockCAM, i);
                sock->cam = NuSplineFind(gscn, name);
                if (sock->cam == NULL) {
                    continue;
                }
                len = (i32)(i16)sock->cam->length;
                if (1 < len) {
                    sock->length = len - 1;
                    sprintf(name, "%s%.2i", tSockA, i);
                    sock->a = NuSplineFind(gscn, name);
                    if (sock->a == NULL) {
                        continue;
                    }
                    if ((i32)(i16)sock->a->length != len) {
                        continue;
                    }
                    sprintf(name, "%s%.2i", tSockB, i);
                    sock->b = NuSplineFind(gscn, name);
                    if (sock->b == NULL) {
                        continue;
                    }
                    if ((i32)(i16)sock->b->length != len) {
                        continue;
                    }
                    sprintf(name, "%s%.2i", tSockC, i);
                    sock->c = NuSplineFind(gscn, name);
                    if (sock->c == NULL) {
                        sock->flags |= 1;
                    } else if ((i32)(i16)sock->c->length != len) {
                        sock->flags |= 1;
                    }
                    if ((sock->flags & 1) == 0) {
                        sprintf(name, "%s%.2i", tSockD, i);
                        sock->d = NuSplineFind(gscn, name);
                        if (sock->d == NULL) {
                            sock->flags |= 1;
                        } else if ((i32)(i16)sock->d->length != len) {
                            sock->flags |= 1;
                        }
                    }
                    sprintf(name, "%s%.2i", tSockMID, i);
                    sock->mid = NuSplineFind(gscn, name);
                    if ((sock->mid != NULL) && ((i32)(i16)sock->mid->length != len)) {
                        sock->mid = NULL;
                    }
                    sprintf(name, "%s%.2i", tSockLEFT, i);
                    sock->left = NuSplineFind(gscn, name);
                    if ((sock->left != NULL) && ((i32)(i16)sock->left->length != len)) {
                        sock->left = NULL;
                    }
                    sprintf(name, "%s%.2i", tSockRIGHT, i);
                    sock->right = NuSplineFind(gscn, name);
                    if ((sock->right != NULL) && ((i32)(i16)sock->right->length != len)) {
                        sock->right = NULL;
                    }
                    sprintf(name, "%s%.2i", tSockLOOK, i);
                    sock->look = NuSplineFind(gscn, name);
                    if ((sock->look != NULL) && ((i32)(i16)sock->look->length != len)) {
                        sock->look = NULL;
                    }
                    sprintf(name, "%s%.2i", tSockLATERAL, i);
                    sock->lateral = NuSplineFind(gscn, name);
                    if ((sock->lateral != NULL) && ((i32)(i16)sock->lateral->length != len)) {
                        sock->lateral = NULL;
                    }
                    sprintf(name, "%s%.2i", tSockTRACKIN, i);
                    sock->trackin = NuSplineFind(gscn, name);
                    if ((sock->trackin != NULL) && ((i32)(i16)sock->trackin->length != len)) {
                        sock->trackin = NULL;
                    }
                    sprintf(name, "%s%.2i", tSockLIMIT, i);
                    sock->limit = NuSplineFind(gscn, name);
                    sock->max = sock->a->pts[0];
                    sock->min = sock->max;
                    for (j = 0; j < len; j++) {
                        AdjustMinMaxBox(&sock->a->pts[j], &sock->min, &sock->max);
                        AdjustMinMaxBox(&sock->b->pts[j], &sock->min, &sock->max);
                        if ((sock->flags & 1) == 0) {
                            AdjustMinMaxBox(&sock->c->pts[j], &sock->min, &sock->max);
                            AdjustMinMaxBox(&sock->d->pts[j], &sock->min, &sock->max);
                        }
                    }
                    sock->center.x = (sock->min.x + sock->max.x) * 0.5f;
                    sock->center.y = (sock->min.y + sock->max.y) * 0.5f;
                    sock->center.z = (sock->min.z + sock->max.z) * 0.5f;
                    sock->extent = sock->max.x - sock->center.x;
                    extent_z = sock->max.z - sock->center.z;
                    if (extent_z < sock->extent) {
                        sock->extent = extent_z;
                    }
                    sock->valid = 1;
                    sock_sys->count++;
                }
            }
        }
    }

    SOCKSYS *SockSysInit(VARIPTR *buf, VARIPTR buf_end, NUGSCN *gscn) {
        buf->addr = ALIGN(buf->addr, 4);
        if (buf->addr + 0x4f08 >= buf_end.addr) {
            return NULL;
        }

        SOCKSYS *sys = (SOCKSYS *)buf->void_ptr;
        buf->void_ptr = (char *)buf->void_ptr + 8;
        sys->sock = (SOCK *)buf->void_ptr;
        buf->void_ptr = (char *)buf->void_ptr + 0x4f00;
        sys->count = 0;
        memset(sys->sock, 0, 0x4f00);
        SockSysFindInScene(sys, gscn);
        return sys;
    }
}

static __used__ void SockDataError() {
}

static i32 RotDiff(u16 from, u16 to) {
    i32 difference = (i32)to - (i32)from;
    if (difference > 0x8000) {
        difference -= 0x10000;
    } else if (difference < -0x8000) {
        difference += 0x10000;
    }
    return difference;
}

static void SockEdgeAnglesXY(NUVEC *from, NUVEC *to, u16 *x, u16 *y) {
    f32 dx = to->x - from->x;
    f32 dz = to->z - from->z;
    *x = (u16)-NuAtan2D(to->y - from->y, NuFsqrt(dx * dx + dz * dz));
    *y = (u16)NuAtan2D(dx, dz);
}

static void SockMidpointAt(const SOCK *sock, i32 point, NUVEC *midpoint) {
    if (sock->mid != NULL) {
        *midpoint = sock->mid->pts[point];
        return;
    }

    NuVecAdd(midpoint, &sock->a->pts[point], &sock->b->pts[point]);
    if ((sock->flags & 1) == 0) {
        NuVecAdd(midpoint, midpoint, &sock->c->pts[point]);
        NuVecAdd(midpoint, midpoint, &sock->d->pts[point]);
        NuVecScale(midpoint, midpoint, 0.25f);
    } else {
        NuVecScale(midpoint, midpoint, 0.5f);
    }
}

static void SockRailAngles(SOCK *sock, NUGSPLINE *spline, SOCKROT *rotations) {
    i32 point_count = sock->cam->length;
    for (i32 point = 0; point < point_count; ++point) {
        i32 previous = point - 1;
        if (previous < 0) {
            previous = point_count - 1;
        }
        i32 next = point + 1;
        if (next >= point_count) {
            next = 0;
        }

        NUVEC previous_pos;
        NUVEC position;
        NUVEC next_pos;
        if (spline != NULL) {
            previous_pos = spline->pts[previous];
            position = spline->pts[point];
            next_pos = spline->pts[next];
        } else {
            SockMidpointAt(sock, previous, &previous_pos);
            SockMidpointAt(sock, point, &position);
            SockMidpointAt(sock, next, &next_pos);
        }

        u16 x0 = 0;
        u16 y0 = 0;
        u16 x1 = 0;
        u16 y1 = 0;
        bool average = true;
        if (point == 0 && sock->unknown_33 == 0) {
            SockEdgeAnglesXY(&position, &next_pos, &x0, &y0);
            average = false;
        } else if (point == point_count - 1 && sock->unknown_33 == 0) {
            SockEdgeAnglesXY(&previous_pos, &position, &x0, &y0);
            average = false;
        } else {
            SockEdgeAnglesXY(&previous_pos, &position, &x0, &y0);
            SockEdgeAnglesXY(&position, &next_pos, &x1, &y1);
        }

        rotations[point].x = x0;
        rotations[point].y = y0;
        if (average) {
            rotations[point].x = (u16)(x0 + RotDiff(x0, x1) / 2);
            rotations[point].y = (u16)(y0 + RotDiff(y0, y1) / 2);
        }
    }
}

static f32 SplineLength(NUGSPLINE *spline, i32 closed) {
    if (spline == NULL || spline->length < 2) {
        return 0.0f;
    }
    i32 segment_count = closed == 0 ? spline->length - 1 : spline->length;
    f32 length = 0.0f;
    for (i32 segment = 0; segment < segment_count; ++segment) {
        i32 next = segment == spline->length - 1 ? 0 : segment + 1;
        length += NuVecDist(&spline->pts[segment], &spline->pts[next], NULL);
    }
    return length;
}

static void SetSockBitValue(SOCK *sock, i32 index) {
    sock->overlap_exclusion_mask[index >> 5] |= 1U << (index & 0x1f);
}

static bool SockBitIsSet(const SOCK *sock, i32 index) {
    return (sock->overlap_exclusion_mask[index >> 5] & (1U << (index & 0x1f))) != 0;
}

static bool BoundsOverlap(const NUVEC &min_a, const NUVEC &max_a, const NUVEC &min_b, const NUVEC &max_b,
                          bool ignore_y) {
    return min_b.x <= max_a.x && min_a.x <= max_b.x && min_b.z <= max_a.z && min_a.z <= max_b.z &&
           (ignore_y || (min_b.y <= max_a.y && min_a.y <= max_b.y));
}

SOCKPOSITION TempSPosList[4];
SOCKPOSITION *TempBestSPos;
i32 TempSPosCount;
NUVEC temp_pos;
SOCKPOSITION TempSPos;
u32 TempSLoc[2];
i32 complexsockposition_forcesock = -1;

static NUVEC temp_sockmidpos;
static NUVEC temp_sockcampos;

static bool OnOrOutsidePlane(NUVEC *point, NUVEC *plane_point, NUVEC *normal) {
    return normal->x * (point->x - plane_point->x) + normal->y * (point->y - plane_point->y) +
               normal->z * (point->z - plane_point->z) >=
           0.0f;
}

static bool InsideLineXZ(f32 x, f32 z, f32 x0, f32 z0, f32 x1, f32 z1) {
    return (x - x0) * (z1 - z0) + (x0 - x1) * (z - z0) >= 0.0f;
}

static f32 DistanceToPlane(NUVEC *point, NUVEC *plane_point, NUVEC *normal) {
    return normal->x * (point->x - plane_point->x) + normal->y * (point->y - plane_point->y) +
           normal->z * (point->z - plane_point->z);
}

static f32 RatioBetweenPlanes(NUVEC *point, NUVEC *point_a, NUVEC *normal_a, NUVEC *point_b, NUVEC *normal_b) {
    f32 distance_a = DistanceToPlane(point, point_a, normal_a);
    f32 distance_b = DistanceToPlane(point, point_b, normal_b);
    return distance_a / (distance_a + distance_b);
}

static f32 SockDistanceToLineXZ(NUVEC *point, NUVEC *line_a, NUVEC *line_b) {
    f32 dx = line_b->x - line_a->x;
    f32 dz = line_b->z - line_a->z;
    f32 length = NuFsqrt(dx * dx + dz * dz);
    if (length == 0.0f) {
        return 0.0f;
    }
    f32 cross = (point->x - line_a->x) * dz - (point->z - line_a->z) * dx;
    return cross < 0.0f ? -cross / length : cross / length;
}

static f32 RatioBetweenEdgesXZ(NUVEC *point, NUVEC *edge_a0, NUVEC *edge_a1, NUVEC *edge_b0, NUVEC *edge_b1) {
    f32 distance_a = SockDistanceToLineXZ(point, edge_a0, edge_a1);
    f32 distance_b = SockDistanceToLineXZ(point, edge_b0, edge_b1);
    return distance_a / (distance_a + distance_b);
}

static void SockSysPointAlongMID(SOCK *sock, SOCKPOSITION *position, NUVEC *result) {
    NUVEC *from;
    NUVEC *to;
    if (sock->mid == NULL) {
        from = &sock->segments[position->location.segment].midpoint;
        to = &sock->segments[position->next_segment].midpoint;
    } else {
        from = &sock->mid->pts[position->location.segment];
        to = &sock->mid->pts[position->next_segment];
    }
    if (result == NULL) {
        result = &temp_sockmidpos;
    }
    result->x = from->x + (to->x - from->x) * position->ratio;
    result->y = from->y + (to->y - from->y) * position->ratio;
    result->z = from->z + (to->z - from->z) * position->ratio;
}

static void FillSockPosition(SOCKSYS *sock_sys, SOCKPOSITION *position) {
    if (sock_sys == NULL) {
        return;
    }
    SOCK *sock = &sock_sys->sock[position->location.sock];
    i32 segment = position->location.segment;
    i32 next = position->next_segment;
    SockSysPointAlongSpline(&temp_sockcampos, sock->cam, segment, next, position->ratio);

    SOCKROT *cam_from = &sock->cam_rotations[segment];
    SOCKROT *cam_to = &sock->cam_rotations[next];
    position->camera_rotation.x = (u16)(cam_from->x + (f32)RotDiff(cam_from->x, cam_to->x) * position->ratio);
    position->camera_rotation.y = (u16)(cam_from->y + (f32)RotDiff(cam_from->y, cam_to->y) * position->ratio);

    SOCKROT *mid_from = &sock->mid_rotations[segment];
    SOCKROT *mid_to = &sock->mid_rotations[next];
    position->midpoint_rotation.x = (u16)(mid_from->x + (f32)RotDiff(mid_from->x, mid_to->x) * position->ratio);
    position->midpoint_rotation.y = (u16)(mid_from->y + (f32)RotDiff(mid_from->y, mid_to->y) * position->ratio);

    position->distance = (f32)segment + position->ratio;
    i32 divisor = sock->length + (sock->unknown_33 != 0 ? 1 : 0);
    position->normalized_distance = position->distance / (f32)divisor;
}

static f32 BestSockPosition(SOCKSYS *sock_sys, NUVEC *point, SOCKPOSITION *result, i32 sock_index, i32 prior_segment) {
    SOCK *sock = &sock_sys->sock[sock_index];
    result->location.sock = -1;
    if (sock->valid == 0) {
        return 0.0f;
    }

    bool within_bounds =
        sock->min.x <= point->x && point->x <= sock->max.x && sock->min.z <= point->z && point->z <= sock->max.z;
    if ((sock->flags & 1) == 0) {
        within_bounds = within_bounds && sock->min.y <= point->y && point->y <= sock->max.y;
    }
    if (!within_bounds) {
        return 0.0f;
    }

    i32 segment_count = sock->length + (sock->unknown_33 != 0 ? 1 : 0);
    i32 initial_segment = prior_segment == -1 ? segment_count / 2 : prior_segment;
    i32 ascending_segment = initial_segment + 1;
    i32 descending_segment = initial_segment - 1;
    i32 direction = 0;
    i32 exhausted_directions = 0;

    for (;;) {
        i32 segment;
        if (direction == 0) {
            segment = initial_segment;
        } else if (direction == 1) {
            segment = ascending_segment++;
        } else {
            segment = descending_segment--;
        }

        if (segment < 0 || segment >= segment_count) {
            exhausted_directions |= direction;
        } else {
            i32 next = segment + 1;
            if (next == segment_count && sock->unknown_33 != 0) {
                next = 0;
            }

            NUVEC *a0 = &sock->a->pts[segment];
            NUVEC *b0 = &sock->b->pts[segment];
            NUVEC *a1 = &sock->a->pts[next];
            NUVEC *b1 = &sock->b->pts[next];
            SOCKSEGMENT *segment_data = &sock->segments[segment];
            bool inside = false;

            if ((sock->flags & 1) == 0) {
                if (BoundsOverlap(segment_data->min, segment_data->max, *point, *point, false)) {
                    NUVEC *c1 = &sock->c->pts[next];
                    NUVEC *d1 = &sock->d->pts[next];
                    inside = OnOrOutsidePlane(point, a0, &segment_data->planes[0]) &&
                             OnOrOutsidePlane(point, a1, &segment_data->planes[1]) &&
                             OnOrOutsidePlane(point, b1, &segment_data->planes[2]) &&
                             OnOrOutsidePlane(point, c1, &segment_data->planes[3]) &&
                             OnOrOutsidePlane(point, d1, &segment_data->planes[4]) &&
                             OnOrOutsidePlane(point, b1, &segment_data->planes[5]);
                }
            } else if (segment_data->min.x <= point->x && point->x <= segment_data->max.x &&
                       segment_data->min.z <= point->z && point->z <= segment_data->max.z) {
                inside = InsideLineXZ(point->x, point->z, b0->x, b0->z, a0->x, a0->z) &&
                         InsideLineXZ(point->x, point->z, a0->x, a0->z, a1->x, a1->z) &&
                         InsideLineXZ(point->x, point->z, a1->x, a1->z, b1->x, b1->z) &&
                         InsideLineXZ(point->x, point->z, b1->x, b1->z, b0->x, b0->z);
            }

            if (inside) {
                result->location.sock = (i8)sock_index;
                result->location.segment = (i16)segment;
                if ((sock->flags & 1) == 0) {
                    result->ratio = RatioBetweenPlanes(point, a0, &segment_data->planes[0], &sock->c->pts[next],
                                                       &segment_data->planes[5]);
                } else {
                    result->ratio = RatioBetweenEdgesXZ(point, b0, a0, b1, a1);
                }
                result->next_segment = (i16)next;
                SockSysPointAlongMID(sock, result, &temp_sockmidpos);
                f32 distance_squared = NuVecDistSqr(point, &temp_sockmidpos, NULL);
                FillSockPosition(sock_sys, result);
                TempSLoc[0] = *(u32 *)&result->location;
                TempSLoc[1] = *(u32 *)&result->ratio;
                return distance_squared;
            }
        }

        if (exhausted_directions == 3) {
            return 0.0f;
        }
        direction = direction == 1 ? 2 : 1;
    }
}

extern "C" {

    void ComplexSockPosition(SOCKSYS *sock_sys, NUVEC *position, i32 prior_sock, i32 prior_segment,
                             SOCKPOSITION *result) {
        if (sock_sys == NULL) {
            result->location.sock = -1;
            result->location.segment = -1;
            result->flags = 0;
            return;
        }

        i32 original_prior_sock = prior_sock;
        i32 forced_sock = complexsockposition_forcesock;
        complexsockposition_forcesock = -1;
        memset(TempSPosList, 0, sizeof(TempSPosList));
        temp_pos = *position;

        i32 sockets_to_check = 0x40;
        if (forced_sock != -1) {
            prior_sock = forced_sock;
            prior_segment = -1;
            sockets_to_check = 1;
        } else if (prior_sock == -1) {
            prior_sock = 0;
            prior_segment = -1;
        }

        bool fallback_search = false;
        for (;;) {
            i32 candidate_count = 0;
            u32 candidate_mask = 0;
            bool excluded_socket_seen = false;
            SOCKPOSITION *best = NULL;
            f32 best_distance = 1000000.0f;

            for (i32 checked = 0; checked < sockets_to_check; ++checked) {
                SOCK *sock = &sock_sys->sock[prior_sock];
                if ((sock->flags & 0x100) == 0) {
                    bool excluded = forced_sock == -1 && !fallback_search && original_prior_sock != -1 &&
                                    prior_sock != original_prior_sock &&
                                    SockBitIsSet(&sock_sys->sock[original_prior_sock], prior_sock);
                    if (excluded) {
                        excluded_socket_seen = true;
                    } else {
                        f32 distance = BestSockPosition(sock_sys, position, &TempSPosList[candidate_count], prior_sock,
                                                        prior_segment);
                        SOCKPOSITION *candidate = &TempSPosList[candidate_count];
                        if (candidate->location.sock != -1) {
                            candidate->midpoint = temp_sockmidpos;
                            candidate->camera_position = temp_sockcampos;
                            if (best == NULL || distance < best_distance) {
                                best = candidate;
                                best_distance = distance;
                            }
                            candidate_mask |= 1U << (prior_sock & 0x1f);
                            ++candidate_count;
                            if (candidate_count == 4) {
                                break;
                            }
                        }
                    }
                }
                prior_sock = (prior_sock + 1) % 0x40;
                prior_segment = -1;
            }

            if (best != NULL) {
                *result = *best;
                result->candidate_mask = candidate_mask;
                result->flags = fallback_search ? 1 : 0;
                result->candidate_count = (i8)candidate_count;
                TempBestSPos = best;
                TempSPosCount = candidate_count;
                memcpy(TempSLoc, result, sizeof(TempSLoc));
                return;
            }
            if (!excluded_socket_seen) {
                result->location.sock = -1;
                result->location.segment = -1;
                result->flags = 0;
                result->candidate_count = 0;
                TempBestSPos = NULL;
                TempSPosCount = 0;
                memcpy(TempSLoc, result, sizeof(TempSLoc));
                return;
            }

            fallback_search = true;
            prior_sock = 0;
            prior_segment = -1;
            forced_sock = -1;
            sockets_to_check = 0x40;
        }
    }

    void ComplexSockAngles(SOCKROT *angles) {
        SOCKPOSITION *positions = TempSPosList;
        i32 count = TempSPosCount;
        if (count > 0) {
            angles->x = positions[0].midpoint_rotation.x;
            angles->y = positions[0].midpoint_rotation.y;
            if (count > 1) {
                NUVEC midpoint = positions[0].midpoint;
                for (i32 i = 1; i < count; ++i) {
                    NUVEC from = midpoint;
                    NUVEC to = positions[i].midpoint;
                    NUVEC direction;
                    NUVEC offset;
                    NuVecSub(&direction, &to, &from);
                    f32 distance =
                        NuFsqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
                    f32 ratio = NuFdiv(1.0f, distance);
                    direction.x *= ratio;
                    direction.y *= ratio;
                    direction.z *= ratio;
                    NuVecSub(&offset, &temp_pos, &from);
                    f32 projection = offset.x * direction.x + offset.y * direction.y + offset.z * direction.z;
                    if (projection > 0.0f) {
                        if (projection >= 1.0f) {
                            angles->x = positions[i].midpoint_rotation.x;
                            angles->y = positions[i].midpoint_rotation.y;
                            midpoint = to;
                        } else {
                            ratio = projection / distance;
                            angles->x += static_cast<u16>(static_cast<i32>(
                                static_cast<f32>(RotDiff(angles->x, positions[i].midpoint_rotation.x)) * ratio));
                            angles->y += static_cast<u16>(static_cast<i32>(
                                static_cast<f32>(RotDiff(angles->y, positions[i].midpoint_rotation.y)) * ratio));
                            midpoint.x += (to.x - midpoint.x) * ratio;
                            midpoint.y += (to.y - midpoint.y) * ratio;
                            midpoint.z += (to.z - midpoint.z) * ratio;
                        }
                    }
                }
            }
        }
    }

    f32 EnforceSockYLimits(f32 y, SOCKPOSITION *position, SOCKSYS *sock_sys) {
        if (sock_sys != NULL) {
            if (y < position->camera_position.y) {
                y = position->camera_position.y;
            } else {
                SOCK *sock = &sock_sys->sock[position->location.sock];
                if (sock->mid != NULL && position->midpoint.y < y) {
                    y = position->midpoint.y;
                }
            }
        }
        return y;
    }

    SOCK *FindSock(SOCKSYS *system, char *name) {
        if (system != NULL) {
            SOCK *sock = system->sock;
            for (i32 index = 0; index < 64; ++index, ++sock) {
                if (sock->valid != 0 && NuStrICmp(name, sock->name) == 0)
                    return sock;
            }
        }
        return NULL;
    }

    f32 MidDistanceFromSockStart(SOCKSYS *system, SOCKPOSITION *position) {
        if (system != NULL && position->location.sock != -1) {
            SOCKSEGMENT *segment = &system->sock[position->location.sock].segments[position->location.segment];
            return segment->distance_from_start + segment->length * position->ratio;
        }
        return 0.0f;
    }

    void MoveSockPosition(SOCKSYS *system, SOCKPOSITION *source, f32 distance, SOCKPOSITION *result) {
        *result = *source;
        if (system == NULL || result->location.sock == -1 || distance == 0.0f)
            return;
        i32 direction;
        if (distance > 0.0f)
            direction = 1;
        else {
            direction = 2;
            distance = -distance;
        }
        i32 length;
        NUVEC *from;
        NUVEC *to;
        SOCK *sock = &system->sock[result->location.sock];
        NUVEC *point = &result->midpoint;
        if (sock->unknown_33 != 0)
            length = sock->length + 1;
        else
            length = sock->length;
        for (;;) {
            result->next_segment = static_cast<i16>(result->location.segment + 1);
            if (result->next_segment == length && sock->unknown_33 != 0)
                result->next_segment = 0;
            if (sock->mid != NULL) {
                from = &sock->mid->pts[result->location.segment];
                to = &sock->mid->pts[result->next_segment];
            } else {
                from = &sock->segments[result->location.segment].midpoint;
                to = &sock->segments[result->location.segment].next_midpoint;
            }
            NUVEC *target = direction == 1 ? to : from;
            f32 remaining = NuVecDist(point, target, NULL);
            if (distance > remaining) {
                *point = *target;
                result->ratio = 0.0f;
                distance -= remaining;
                if (direction == 1) {
                    if (sock->unknown_33 != 0) {
                        ++result->location.segment;
                        if (result->location.segment == length)
                            result->location.segment = 0;
                    } else {
                        if (result->location.segment >= length)
                            break;
                        ++result->location.segment;
                    }
                } else {
                    if (sock->unknown_33 != 0) {
                        --result->location.segment;
                        if (result->location.segment == -1)
                            result->location.segment = sock->length;
                    } else {
                        if (result->location.segment <= 0)
                            break;
                        --result->location.segment;
                    }
                }
                continue;
            }
            if (remaining > 0.0f && distance > 0.0f) {
                f32 ratio = distance / remaining;
                point->x += (target->x - point->x) * ratio;
                point->y += (target->y - point->y) * ratio;
                point->z += (target->z - point->z) * ratio;
                if (sock->mid != NULL) {
                    from = &sock->mid->pts[result->location.segment];
                    to = &sock->mid->pts[result->next_segment];
                } else {
                    from = &sock->segments[result->location.segment].midpoint;
                    to = &sock->segments[result->location.segment].next_midpoint;
                }
                result->ratio = NuVecDist(from, point, NULL) / NuVecDist(from, to, NULL);
            } else {
                result->ratio = 0.0f;
            }
            break;
        }
        FillSockPosition(system, result);
        result->camera_position = temp_sockcampos;
    }

    void SetSockBit(SOCK *sock, i32 index) {
        SetSockBitValue(sock, index);
    }

    void SetSockPostion(SOCKSYS *system, SOCKPOSITION *position, i32 index, i32 segment, f32 ratio) {
        position->location.sock = -1;
        if (system == NULL)
            return;
        if (index < 0 || index > 63)
            return;
        SOCK *sock = &system->sock[index];
        i32 length;
        if (sock->unknown_33 != 0) {
            length = sock->length + 1;
            if (segment > length)
                segment = 0;
        } else {
            length = sock->length;
            if (segment > length)
                segment = length;
        }
        if (segment < 0)
            segment = 0;
        if (ratio < 0.0f)
            ratio = 0.0f;
        else if (ratio > 1.0f)
            ratio = 1.0f;
        position->location.sock = static_cast<i8>(index);
        position->location.segment = static_cast<i16>(segment);
        position->ratio = ratio;
        position->next_segment = static_cast<i16>(segment + 1);
        if (position->next_segment == length && sock->unknown_33 != 0)
            position->next_segment = 0;
        position->candidate_count = 1;
        position->flags = 0;
        position->candidate_mask = 1u << (index & 31);
        FillSockPosition(system, position);
        position->camera_position = temp_sockcampos;
        SockSysPointAlongMID(sock, position, &position->midpoint);
    }

    bool SockBitSet(SOCK *sock, i32 index) {
        return SockBitIsSet(sock, index);
    }

    void SockOff(SOCKSYS *, i32) {
    }

    void SockOn(SOCKSYS *, i32) {
    }

    void SockRotationMatrix(SOCKSYS *system, SOCKPOSITION *position, NUMTX *out, i32 stride, i32 mode) {
        NuMtxSetIdentity(out);
        if (position->location.sock == -1)
            return;
        i32 first = (mode == 4 || mode == 5) ? 0 : 1;
        i32 last = (mode == 4 || mode == 5) ? 3 : 2;
        SOCK *sock = &system->sock[position->location.sock];
        i32 segment = position->location.segment;
        i32 samples[4];
        samples[1] = segment - segment % stride;
        samples[2] = samples[1] + stride;
        samples[3] = samples[2] + stride;
        samples[0] = samples[1] - stride;
        if (samples[0] < 0)
            samples[0] = 0;
        if (samples[2] > sock->length)
            samples[2] = sock->length;
        if (samples[3] > sock->length)
            samples[3] = sock->length;
        NUMTX matrices[4];
        NUQUAT quats[4], result;
        f32 widths[4], heights[4];
        NUVEC right, up, forward;
        for (i32 i = first; i <= last; ++i) {
            NuMtxSetIdentity(&matrices[i]);
            NUVEC *a = &sock->a->pts[samples[i]];
            NUVEC *b = &sock->b->pts[samples[i]];
            NUVEC *c = &sock->c->pts[samples[i]];
            NUVEC *d = &sock->d->pts[samples[i]];
            right.x = (b->x - a->x) + (c->x - d->x);
            right.y = (b->y - a->y) + (c->y - d->y);
            right.z = (b->z - a->z) + (c->z - d->z);
            up.x = (b->x - c->x) + (a->x - d->x);
            up.y = (b->y - c->y) + (a->y - d->y);
            up.z = (b->z - c->z) + (a->z - d->z);
            widths[i] = NuVecMag(&right);
            heights[i] = NuVecMag(&up);
            NuVecNorm(&right, &right);
            NuVecNorm(&up, &up);
            NuVecCross(&forward, &right, &up);
            NuVecNorm(&forward, &forward);
            NuVecCross(&up, &forward, &right);
            NuVecNorm(&up, &up);
            matrices[i].m00 = right.x;
            matrices[i].m01 = right.y;
            matrices[i].m02 = right.z;
            matrices[i].m10 = up.x;
            matrices[i].m11 = up.y;
            matrices[i].m12 = up.z;
            matrices[i].m20 = forward.x;
            matrices[i].m21 = forward.y;
            matrices[i].m22 = forward.z;
            NuMtxToQuat(&matrices[i], &quats[i]);
        }
        NUVEC *a = &sock->a->pts[0], *b = &sock->b->pts[0];
        NUVEC *c = &sock->c->pts[0], *d = &sock->d->pts[0];
        right.x = (b->x - a->x) + (c->x - d->x);
        right.y = (b->y - a->y) + (c->y - d->y);
        right.z = (b->z - a->z) + (c->z - d->z);
        up.x = (b->x - c->x) + (a->x - d->x);
        up.y = (b->y - c->y) + (a->y - d->y);
        up.z = (b->z - c->z) + (a->z - d->z);
        f32 base_width = NuVecMag(&right);
        f32 base_height = NuVecMag(&up);
        f32 ratio = position->ratio;
        ratio += (f32)(segment % stride);
        ratio /= (f32)stride;
        if (sock->flags & 0x200) {
            f32 width = (1.0f - ratio) * (widths[1] / base_width) + (widths[2] / base_width) * ratio;
            if (1.0f >= width)
                sock->camera_local_x_ratio = sock->camera_lateral_ratio * width;
            else
                sock->camera_local_x_ratio =
                    sock->camera_lateral_ratio + (1.0f - 1.0f / width) * (1.0f - sock->camera_lateral_ratio);
            f32 height = (1.0f - ratio) * (heights[1] / base_height) + (heights[2] / base_height) * ratio;
            if (1.0f >= height)
                sock->camera_vertical_ratio = sock->camera_lateral_ratio * height;
            else
                sock->camera_vertical_ratio =
                    sock->camera_lateral_ratio + (1.0f - 1.0f / height) * (1.0f - sock->camera_lateral_ratio);
            if (width > 1.0f)
                sock->look_ratio_xz = sock->unknown_80 / width;
            if (height > 1.0f)
                sock->look_ratio_y = sock->unknown_84 / height;
        } else {
            sock->camera_vertical_ratio = sock->camera_lateral_ratio;
            sock->camera_local_x_ratio = sock->camera_vertical_ratio;
        }
        if (mode == 1) {
            out->m00 = matrices[1].m00 + (matrices[2].m00 - matrices[1].m00) * ratio;
            out->m01 = matrices[1].m01 + (matrices[2].m01 - matrices[1].m01) * ratio;
            out->m02 = matrices[1].m02 + (matrices[2].m02 - matrices[1].m02) * ratio;
            out->m10 = matrices[1].m10 + (matrices[2].m10 - matrices[1].m10) * ratio;
            out->m11 = matrices[1].m11 + (matrices[2].m11 - matrices[1].m11) * ratio;
            out->m12 = matrices[1].m12 + (matrices[2].m12 - matrices[1].m12) * ratio;
            out->m20 = matrices[1].m20 + (matrices[2].m20 - matrices[1].m20) * ratio;
            out->m21 = matrices[1].m21 + (matrices[2].m21 - matrices[1].m21) * ratio;
            out->m22 = matrices[1].m22 + (matrices[2].m22 - matrices[1].m22) * ratio;
            NuVecNorm((NUVEC *)&out->m00, (NUVEC *)&out->m00);
            NuVecNorm((NUVEC *)&out->m10, (NUVEC *)&out->m10);
            NuVecNorm((NUVEC *)&out->m20, (NUVEC *)&out->m20);
        } else if (mode == 2) {
            NuQuatLerp(&result, &quats[1], &quats[2], ratio);
            NuQuatToMtx(&result, out);
        } else if (mode == 3) {
            NuQuatSlerp(&result, &quats[1], &quats[2], ratio);
            NuQuatToMtx(&result, out);
        } else if (mode == 4) {
            NuQuatCubicInt(&result, &quats[0], &quats[1], &quats[2], &quats[3], ratio);
            NuQuatToMtx(&result, out);
        } else if (mode == 5) {
            NuQuatHermiteInt(&result, &quats[0], &quats[1], &quats[2], &quats[3], ratio);
            NuQuatToMtx(&result, out);
        } else {
            *out = matrices[1];
        }
    }

    i32 SockSegmentsAhead(SOCK *sock, i32 segment, i32 *from_segment, i32 *to_segment) {
        if (sock->look_ahead_segments == 0) {
            return 0;
        }

        *from_segment = segment;
        i32 segments_ahead = sock->look_ahead_segments;
        if (sock->unknown_33 != 0) {
            while (segments_ahead-- > 0) {
                ++*from_segment;
                if (*from_segment == sock->length + 1) {
                    *from_segment = 0;
                }
            }
            *to_segment = *from_segment + 1;
            if (*to_segment == sock->length + 1) {
                *to_segment = 0;
            }
            return 1;
        }

        i32 advanced = 0;
        while (segments_ahead > 0 && *from_segment < sock->length - 1) {
            ++*from_segment;
            --segments_ahead;
            *to_segment = *from_segment + 1;
            advanced = 1;
        }
        return advanced;
    }

    i32 SockSysCamera(SOCKSYS *sock_sys, NUVEC *fallback_camera_position, i32 socket_changed,
                      NUVEC *player_camera_positions, NUVEC *player_positions, i32 player_count,
                      SOCKPOSITION *camera_socket_position, NUVEC *camera_position, NUVEC *camera_target,
                      f32 *overlap_blend, f32 *position_seek, f32 *angle_seek, f32 *camera_shake,
                      f32 *separation_scale) {
        NUVEC average_camera_position, average_player_position, accumulated_target;
        NUVEC scratch = {0.0f, 0.0f, 0.0f};
        NUVEC candidate_camera, look_position, lateral_position, edge_position, local_right, local_x, local_y;
        if (sock_sys == NULL) {
            return 0;
        }

        average_camera_position.x = average_camera_position.y = average_camera_position.z = 0.0f;
        average_player_position.x = average_player_position.y = average_player_position.z = 0.0f;
        for (i32 i = 0; i < player_count; ++i) {
            NuVecAdd(&average_camera_position, &average_camera_position, &player_camera_positions[i]);
            NuVecAdd(&average_player_position, &average_player_position, &player_positions[i]);
        }
        f32 working_scale = 1.0f / (f32)player_count;
        NuVecScale(&average_camera_position, &average_camera_position, working_scale);
        NuVecScale(&average_player_position, &average_player_position, working_scale);

        if (player_count == 2 && socket_changed == 0) {
            PerspectMidPoint(camera_target, &player_camera_positions[0], &player_camera_positions[1],
                             fallback_camera_position);
        } else {
            *camera_target = average_camera_position;
        }

        ComplexSockPosition(sock_sys, &average_camera_position, camera_socket_position->location.sock,
                            camera_socket_position->location.segment, camera_socket_position);
        if (camera_socket_position->location.sock == -1) {
            *camera_position = *fallback_camera_position;
            return 0;
        }

        camera_position->x = camera_position->y = camera_position->z = 0.0f;
        accumulated_target.x = accumulated_target.y = accumulated_target.z = 0.0f;
        if (overlap_blend != NULL) {
            *overlap_blend = 0.0f;
        }
        if (position_seek != NULL) {
            *position_seek = 0.0f;
        }
        if (angle_seek != NULL) {
            *angle_seek = 0.0f;
        }
        if (camera_shake != NULL) {
            *camera_shake = 0.0f;
        }
        if (separation_scale != NULL) {
            *separation_scale = 0.0f;
        }

        i32 contributing_sockets = 0;
        f32 single_player_pullback = 0.0f;
        f32 two_player_pullback = 0.0f;
        f32 camera_height_above_ground = 0.0f;
        for (i32 i = 0; i < TempSPosCount; ++i) {
            SOCKPOSITION *candidate = &TempSPosList[i];
            SOCK *sock = &sock_sys->sock[candidate->location.sock];
            bool include =
                socket_changed == 0 || candidate->location.sock == camera_socket_position->location.sock ||
                !SockBitIsSet(&sock_sys->sock[camera_socket_position->location.sock], candidate->location.sock);
            if (!include) {
                continue;
            }

            if ((sock->flags & SOCK_FLAG_PROJECT_CAMERA_FROM_PLAYER) != 0) {

                local_x = {1.0f, 0.0f, 0.0f};
                local_y = {0.0f, 1.0f, 0.0f};
                NuVecSub(&scratch, &average_camera_position, &candidate->midpoint);
                NuVecRotateX(&local_x, &local_x, candidate->camera_rotation.x);
                NuVecRotateY(&local_x, &local_x, candidate->camera_rotation.y);
                NuVecRotateX(&local_y, &local_y, candidate->camera_rotation.x);
                NuVecRotateY(&local_y, &local_y, candidate->camera_rotation.y);
                f32 projection_x = NuVecDot(&scratch, &local_x);
                f32 projection_y = NuVecDot(&scratch, &local_y);
                projection_x *= sock->camera_local_x_ratio;
                projection_y *= sock->camera_vertical_ratio;
                NuVecScale(&local_x, &local_x, projection_x);
                NuVecScale(&local_y, &local_y, projection_y);
                NuVecAdd(&candidate_camera, &local_x, &local_y);
                NuVecAdd(&candidate_camera, &candidate_camera, &candidate->camera_position);
            } else {
                candidate_camera = candidate->camera_position;
            }

            if (sock->camera_rail_offset != 0.0f) {
                scratch = {0.0f, 0.0f, 1.0f};
                NuVecRotateX(&scratch, &scratch, candidate->camera_rotation.x);
                NuVecRotateY(&scratch, &scratch, candidate->camera_rotation.y);
                working_scale = sock->camera_rail_offset;
                candidate_camera.x += scratch.x * working_scale;
                candidate_camera.y += scratch.y * working_scale;
                candidate_camera.z += scratch.z * working_scale;
            }

            f32 lateral_ratio;
            if (sock->lateral == NULL) {
                lateral_ratio = sock->camera_lateral_ratio;
            } else {

                local_right = {1.0f, 0.0f, 0.0f};
                SockSysPointAlongSpline(&lateral_position, sock->lateral, candidate->location.segment,
                                        candidate->next_segment, candidate->ratio);
                NuVecRotateY(&local_right, &local_right, candidate->midpoint_rotation.y);
                const f32 lateral_projection = local_right.x * (lateral_position.x - candidate->midpoint.x) +
                                               local_right.z * (lateral_position.z - candidate->midpoint.z);

                SockSysPointAlongSpline(&edge_position, sock->a, candidate->location.segment, candidate->next_segment,
                                        candidate->ratio);
                const f32 edge_a_x = edge_position.x - candidate->midpoint.x;
                const f32 edge_a_z = edge_position.z - candidate->midpoint.z;
                f32 half_width = NuFsqrt(edge_a_x * edge_a_x + edge_a_z * edge_a_z);
                SockSysPointAlongSpline(&edge_position, sock->b, candidate->location.segment, candidate->next_segment,
                                        candidate->ratio);
                const f32 edge_b_x = edge_position.x - candidate->midpoint.x;
                const f32 edge_b_z = edge_position.z - candidate->midpoint.z;
                half_width = (half_width + NuFsqrt(edge_b_x * edge_b_x + edge_b_z * edge_b_z)) * 0.5f;
                lateral_ratio = half_width > 0.0f ? lateral_projection / half_width * working_scale : 0.0f;
            }

            if (lateral_ratio != 0.0f && (sock->flags & SOCK_FLAG_PROJECT_CAMERA_FROM_PLAYER) == 0) {
                f32 lateral_x = (average_camera_position.x - candidate->midpoint.x) * lateral_ratio;
                f32 lateral_z = (average_camera_position.z - candidate->midpoint.z) * lateral_ratio;
                if (sock->left != NULL || sock->right != NULL) {
                    const f32 lateral_distance = NuFsqrt(lateral_x * lateral_x + lateral_z * lateral_z);
                    local_right = {1.0f, 0.0f, 0.0f};
                    NuVecRotateY(&local_right, &local_right, candidate->midpoint_rotation.y);
                    const f32 side = local_right.x * lateral_x + local_right.z * lateral_z;
                    NUGSPLINE *limit = side < 0.0f ? sock->left : sock->right;
                    if (limit != NULL) {

                        SockSysPointAlongSpline(&lateral_position, limit, candidate->location.segment,
                                                candidate->next_segment, candidate->ratio);
                        const f32 limit_x = lateral_position.x - candidate->camera_position.x;
                        const f32 limit_z = lateral_position.z - candidate->camera_position.z;
                        const f32 limit_distance = NuFsqrt(limit_x * limit_x + limit_z * limit_z);
                        if (limit_distance < lateral_distance) {
                            working_scale = limit_distance / lateral_distance;
                            lateral_x *= working_scale;
                            lateral_z *= working_scale;
                        }
                    }
                }
                candidate_camera.x += lateral_x;
                candidate_camera.z += lateral_z;
            }

            if (sock->camera_vertical_ratio != 0.0f && (sock->flags & SOCK_FLAG_PROJECT_CAMERA_FROM_PLAYER) == 0) {
                candidate_camera.y += sock->camera_vertical_ratio * (average_camera_position.y - candidate->midpoint.y);
            }

            const bool has_arena_blend = sock->camera_arena_blend.x > 0.0f || sock->camera_arena_blend.y > 0.0f ||
                                         sock->camera_arena_blend.z > 0.0f;
            const bool has_arena_offset = sock->camera_arena_offset.x != 0.0f || sock->camera_arena_offset.y != 0.0f ||
                                          sock->camera_arena_offset.z != 0.0f;
            if (has_arena_blend && has_arena_offset) {

                NuVecAdd(&scratch, &average_player_position, &sock->camera_arena_offset);
                if (sock->camera_arena_blend.x < 1.0f)
                    candidate_camera.x += (scratch.x - candidate_camera.x) * sock->camera_arena_blend.x;
                else
                    candidate_camera.x = scratch.x;
                if (sock->camera_arena_blend.y < 1.0f)
                    candidate_camera.y += (scratch.y - candidate_camera.y) * sock->camera_arena_blend.y;
                else
                    candidate_camera.y = scratch.y;
                if (sock->camera_arena_blend.z < 1.0f)
                    candidate_camera.z += (scratch.z - candidate_camera.z) * sock->camera_arena_blend.z;
                else
                    candidate_camera.z = scratch.z;
                if ((sock->flags & SOCK_FLAG_CLAMP_TARGET_Y) != 0) {
                    candidate_camera.y = EnforceSockYLimits(candidate_camera.y, candidate, sock_sys);
                }
            } else if (sock->camera_arena_offset.y != 0.0f) {
                candidate_camera.y = average_camera_position.y + sock->camera_arena_offset.y;
                if ((sock->flags & SOCK_FLAG_CLAMP_TARGET_Y) != 0) {
                    candidate_camera.y = EnforceSockYLimits(candidate_camera.y, candidate, sock_sys);
                }
            }

            if (sock->camera_distance_to_target == 0.0f) {
                if (sock->camera_pullback_ratio != 0.0f) {
                    candidate_camera.x += (camera_target->x - candidate_camera.x) * sock->camera_pullback_ratio;
                    candidate_camera.y += (camera_target->y - candidate_camera.y) * sock->camera_pullback_ratio;
                    candidate_camera.z += (camera_target->z - candidate_camera.z) * sock->camera_pullback_ratio;
                }
            } else {

                if ((sock->flags & SOCK_FLAG_CAMERA_DISTANCE_XZ) != 0) {
                    scratch = {camera_target->x - candidate_camera.x, 0.0f, camera_target->z - candidate_camera.z};
                } else {
                    NuVecSub(&scratch, camera_target, &candidate_camera);
                }
                NuVecNorm(&scratch, &scratch);
                candidate_camera.x = camera_target->x - scratch.x * sock->camera_distance_to_target;
                candidate_camera.z = camera_target->z - scratch.z * sock->camera_distance_to_target;
                if ((sock->flags & SOCK_FLAG_CAMERA_DISTANCE_XZ) == 0) {
                    candidate_camera.y = camera_target->y - scratch.y * sock->camera_distance_to_target;
                }
            }

            if (sock->look_ratio_xz == 1.0f && sock->look_ratio_y == 1.0f && sock->look_ahead_segments == 0) {
                NuVecAdd(&accumulated_target, &accumulated_target, camera_target);
            } else {

                if (sock->look != NULL) {
                    SockSysPointAlongSpline(&look_position, sock->look, candidate->location.segment,
                                            candidate->next_segment, candidate->ratio);
                } else {
                    look_position = {0.0f, 0.0f, NuVecDist(&candidate_camera, camera_target, NULL)};
                    i32 look_from;
                    i32 look_to;
                    if (SockSegmentsAhead(sock, candidate->location.segment, &look_from, &look_to) == 0) {
                        NuVecRotateX(&look_position, &look_position, candidate->camera_rotation.x);
                        NuVecRotateY(&look_position, &look_position, candidate->camera_rotation.y);
                    } else {
                        SOCKROT *from_rotation = &sock->cam_rotations[look_from];
                        SOCKROT *to_rotation = &sock->cam_rotations[look_to];
                        const u16 pitch = static_cast<u16>(from_rotation->x +
                                                           static_cast<f32>(RotDiff(from_rotation->x, to_rotation->x)) *
                                                               candidate->ratio);
                        const u16 yaw = static_cast<u16>(from_rotation->y +
                                                         static_cast<f32>(RotDiff(from_rotation->y, to_rotation->y)) *
                                                             candidate->ratio);
                        NuVecRotateX(&look_position, &look_position, pitch);
                        NuVecRotateY(&look_position, &look_position, yaw - 0x1555);
                    }
                    NuVecAdd(&look_position, &look_position, &candidate_camera);
                }
                accumulated_target.x += look_position.x + (camera_target->x - look_position.x) * sock->look_ratio_xz;
                accumulated_target.y += look_position.y + (camera_target->y - look_position.y) * sock->look_ratio_y;
                accumulated_target.z += look_position.z + (camera_target->z - look_position.z) * sock->look_ratio_xz;
            }

            NuVecAdd(camera_position, camera_position, &candidate_camera);
            if (overlap_blend != NULL) {
                *overlap_blend += sock->overlap_blend_ratio;
            }
            if (position_seek != NULL) {
                *position_seek += sock->camera_position_seek;
            }
            if (angle_seek != NULL) {
                *angle_seek += sock->camera_angle_seek;
            }
            if (camera_shake != NULL) {
                *camera_shake += sock->camera_shake;
            }
            single_player_pullback += sock->single_player_pullback;
            two_player_pullback += sock->two_player_pullback;
            camera_height_above_ground += sock->camera_height_above_ground;
            ++contributing_sockets;
        }

        if (contributing_sockets > 0) {
            f32 inverse_socket_count = 1.0f / (f32)contributing_sockets;
            NuVecScale(camera_position, camera_position, inverse_socket_count);
            NuVecScale(camera_target, &accumulated_target, inverse_socket_count);
            if (overlap_blend != NULL) {
                *overlap_blend *= inverse_socket_count;
            }
            if (position_seek != NULL) {
                *position_seek *= inverse_socket_count;
            }
            if (angle_seek != NULL) {
                *angle_seek *= inverse_socket_count;
            }
            if (camera_shake != NULL) {
                *camera_shake *= inverse_socket_count;
            }
            single_player_pullback *= inverse_socket_count;
            two_player_pullback *= inverse_socket_count;
            if (separation_scale != NULL) {
                *separation_scale = two_player_pullback * inverse_socket_count;
            }
            camera_height_above_ground *= inverse_socket_count;
        }

        if (camera_height_above_ground > 0.0f) {
            const f32 ground_y = NewShadow(camera_position, 0.0f, 5.0f, -1);
            if (ground_y != 2000000.0f && camera_position->y < ground_y + camera_height_above_ground) {
                camera_position->y = ground_y + camera_height_above_ground;
            }
        }

        if (player_count == 1 && single_player_pullback != 0.0f) {

            f32 distance = NuVecDist(camera_target, camera_position, &scratch);
            if (distance > 1.0f) {
                NuVecNorm(&scratch, &scratch);
                if (distance - single_player_pullback < 1.0f) {
                    single_player_pullback = distance - 1.0f;
                }
                NuVecAddScale(camera_position, camera_position, &scratch, single_player_pullback);
            }
        } else if (player_count == 2 && two_player_pullback != 0.0f) {
            const SOCK *active_socket = &sock_sys->sock[camera_socket_position->location.sock];
            const bool planar = (active_socket->flags & SOCK_FLAG_TWO_PLAYER_PLANAR_PULLBACK) != 0;
            const f32 player_separation =
                (active_socket->flags & SOCK_FLAG_TWO_PLAYER_VERTICAL_SEPARATION) != 0
                    ? NuFabs(player_camera_positions[0].y - player_camera_positions[1].y)
                    : (planar ? NuVecXZDist(&player_camera_positions[0], &player_camera_positions[1], NULL)
                              : NuVecDist(&player_camera_positions[0], &player_camera_positions[1], NULL));

            const f32 camera_distance = planar ? NuVecXZDist(camera_target, camera_position, &scratch)
                                               : NuVecDist(camera_target, camera_position, &scratch);
            if (camera_distance > 0.0f) {
                NuVecNorm(&scratch, &scratch);
                f32 offset = -two_player_pullback * player_separation;
                if (camera_distance - offset < 1.0f) {
                    offset = camera_distance - 1.0f;
                }
                if (planar) {
                    camera_position->x += scratch.x * offset;
                    camera_position->z += scratch.z * offset;
                } else {
                    NuVecAddScale(camera_position, camera_position, &scratch, offset);
                }
            }
        }
        return 1;
    }

    i32 SockSysCameraWithOverlapBlend(SOCKSYS *sock_sys, NUVEC *fallback_camera_position, i32 socket_changed,
                                      NUVEC *player_camera_positions, NUVEC *player_positions, i32 player_count,
                                      SOCKPOSITION *camera_socket_position, NUVEC *camera_position,
                                      NUVEC *camera_target, f32 *overlap_blend, f32 *position_seek, f32 *angle_seek,
                                      f32 *camera_shake, f32 *separation_scale) {
        static i32 LastSingleSockWeWereIn = -1;
        f32 blend;
        SOCKCAMERARESULT *from, *to;
        i32 i, j;
        ComplexSockPosition(sock_sys, player_camera_positions, camera_socket_position->location.sock,
                            camera_socket_position->location.segment, camera_socket_position);
        if (TempSPosCount > 1) {
            i32 first_index = TempSPosList[0].location.sock;
            i32 second_index = TempSPosList[1].location.sock;
            SOCK *first_sock = &sock_sys->sock[first_index];
            SOCK *second_sock = &sock_sys->sock[second_index];
            i32 reverse;
            f32 distances[2];
            SOCKCAMERARESULT first, second;
            first.socket = second.socket = *camera_socket_position;
            for (i = 0; i < static_cast<i32>(first_sock->blend_count); ++i) {
                if (first_sock->blend_entries[i].value == TempSPosList[1].location.sock)
                    distances[0] =
                        CalculateDistanceToSpecificSideOrEnd(static_cast<u8>(first_sock->blend_entries[i].edge),
                                                             player_camera_positions, TempSPosList, 0, sock_sys);
            }
            for (j = 0; j < static_cast<i32>(second_sock->blend_count); ++j) {
                if (second_sock->blend_entries[j].value == TempSPosList[0].location.sock)
                    distances[1] =
                        CalculateDistanceToSpecificSideOrEnd(static_cast<u8>(second_sock->blend_entries[j].edge),
                                                             player_camera_positions, &TempSPosList[1], 0, sock_sys);
            }
            if (i != 0 && j != 0) {
                TurnOffAllSocksExcept(sock_sys, first_index);
                SockSysCamera(sock_sys, fallback_camera_position, socket_changed, player_camera_positions,
                              player_positions, player_count, &first.socket, &first.position, &first.target,
                              &first.blend, &first.position_seek, &first.angle_seek, &first.shake, &first.separation);
                RestoreLastSocksTurnoff(sock_sys);
                TurnOffAllSocksExcept(sock_sys, second_index);
                SockSysCamera(sock_sys, fallback_camera_position, socket_changed, player_camera_positions,
                              player_positions, player_count, &second.socket, &second.position, &second.target,
                              &second.blend, &second.position_seek, &second.angle_seek, &second.shake,
                              &second.separation);
                RestoreLastSocksTurnoff(sock_sys);
                reverse = LastSingleSockWeWereIn != first_index;
                blend = distances[reverse] / (distances[0] + distances[1]);
                if (blend < 0.5f)
                    blend = (blend * blend) * 2.0f;
                else {
                    blend = 1.0f - blend;
                    blend = 1.0f - (blend * blend) * 2.0f;
                }
                if (reverse == 0) {
                    from = &second;
                    to = &first;
                } else {
                    from = &first;
                    to = &second;
                }
                camera_position->x = from->position.x + (to->position.x - from->position.x) * blend;
                camera_position->y = from->position.y + (to->position.y - from->position.y) * blend;
                camera_position->z = from->position.z + (to->position.z - from->position.z) * blend;
                camera_target->x = from->target.x + (to->target.x - from->target.x) * blend;
                camera_target->y = from->target.y + (to->target.y - from->target.y) * blend;
                camera_target->z = from->target.z + (to->target.z - from->target.z) * blend;
                *overlap_blend = from->blend + (to->blend - from->blend) * blend;
                *position_seek = from->position_seek + (to->position_seek - from->position_seek) * blend;
                *angle_seek = from->angle_seek + (to->angle_seek - from->angle_seek) * blend;
                *camera_shake = from->shake + (to->shake - from->shake) * blend;
                if (separation_scale)
                    *separation_scale = from->separation + (to->separation - from->separation) * blend;
                return 1;
            }
        }
        LastSingleSockWeWereIn = TempSPosCount > 0 ? TempSPosList[0].location.sock : -1;
        return SockSysCamera(sock_sys, fallback_camera_position, socket_changed, player_camera_positions,
                             player_positions, player_count, camera_socket_position, camera_position, camera_target,
                             overlap_blend, position_seek, angle_seek, camera_shake, separation_scale);
    }

    struct SOCKPAR_CONTEXT {
        u32 reserved[2];
        SOCKSYS **sock_sys;
        SOCK *sock;
    };

    void SockSysConfigureNuFPar(NUFPAR *parser, SOCKPAR_CONTEXT *context) {
        if (context == NULL || context->sock_sys == NULL || *context->sock_sys == NULL || parser == NULL) {
            return;
        }

        sockpar_buffer_ptr = NULL;
        sockpar_buffer_end = NULL;
        sockpar_scene = NULL;
        const i32 sock_index = NuFParGetInt(parser);
        if (sock_index < 0 || sock_index >= 0x40) {
            return;
        }

        context->sock = &(*context->sock_sys)->sock[sock_index];
        sockpar_sock = context->sock;
        memset(&sockpar_sock->unknown_110, 0, sizeof(sockpar_sock->unknown_110) + sizeof(sockpar_sock->unknown_114));

        NuFParPushComCTX(parser, SockSys_ConfigKeywords);
        do {
            if (NuStrICmp(parser->word_buf, "sock_end") == 0) {
                NuFParPopCom(parser);
                return;
            }
            if (NuFParGetWord(parser) != 0) {
                NuFParInterpretWordCTX(parser, context);
            }
        } while (NuFParGetLine(parser) != 0);
    }

    void SockSysPointAlongSpline(NUVEC *result, NUGSPLINE *spline, i32 segment, i32 next_segment, f32 ratio) {
        NUVEC *from = &spline->pts[segment];
        NUVEC *to = &spline->pts[next_segment];
        result->x = from->x + (to->x - from->x) * ratio;
        result->y = from->y + (to->y - from->y) * ratio;
        result->z = from->z + (to->z - from->z) * ratio;
    }

    void SockSysSetObjectVisibility(void) {
    }

    void SockSysTrackInSplineInfo(void) {
    }

    void SockSys_Configure(SOCKSYS *sock_sys, char *config, i32, VARIPTR *buf, VARIPTR *buf_end, NUGSCN *gscn) {
        if (sock_sys == NULL || config == NULL || gscn == NULL) {
            return;
        }

        NUFPAR *parser = NuFParCreateMem(const_cast<char *>("socks"), config, 0xffff);
        if (parser == NULL) {
            return;
        }

        sockpar_scene = gscn;
        sockpar_buffer_ptr = buf;
        sockpar_buffer_end = buf_end;
        NuFParPushComCTX(parser, SockSys_ConfigKeywords);

        i32 inside_sock = 0;
        while (NuFParGetLine(parser) != 0) {
            NuFParGetWord(parser);
            if (parser->word_buf[0] == '\0') {
                continue;
            }

            if (inside_sock) {
                if (NuStrICmp(parser->word_buf, "sock_end") == 0) {
                    inside_sock = 0;
                } else {
                    NuFParInterpretWordCTX(parser, NULL);
                }
            } else if (NuStrICmp(parser->word_buf, "sock_start") == 0) {
                const i32 sock_index = NuFParGetInt(parser);
                if (sock_index >= 0 && sock_index < 0x40) {
                    inside_sock = 1;
                    sockpar_sock = &sock_sys->sock[sock_index];
                }
            }
        }
        NuFParDestroy(parser);
    }

    void SockSys_GenerateData(SOCKSYS *sock_sys, VARIPTR *buf, VARIPTR buf_end) {
        if (sock_sys == NULL) {
            return;
        }

        for (i32 sock_index = 0; sock_index < 0x40; ++sock_index) {
            SOCK *sock = &sock_sys->sock[sock_index];
            if (sock->valid == 0) {
                continue;
            }

            i32 point_count = sock->cam->length;
            usize rotations_size = (usize)point_count * sizeof(SOCKROT);
            if (buf->addr + rotations_size >= buf_end.addr) {
                SockDataError();
                return;
            }
            sock->cam_rotations = (SOCKROT *)buf->void_ptr;
            SockRailAngles(sock, sock->cam, sock->cam_rotations);
            buf->addr += rotations_size;

            if (buf->addr + rotations_size >= buf_end.addr) {
                SockDataError();
                return;
            }
            sock->mid_rotations = (SOCKROT *)buf->void_ptr;
            SockRailAngles(sock, sock->mid, sock->mid_rotations);
            buf->addr += rotations_size;

            usize segments_size = (usize)point_count * sizeof(SOCKSEGMENT);
            if (buf->addr + segments_size >= buf_end.addr) {
                SockDataError();
                return;
            }
            sock->segments = (SOCKSEGMENT *)buf->void_ptr;
            f32 distance_from_start = 0.0f;
            for (i32 segment_index = 0; segment_index < point_count; ++segment_index) {
                i32 next = (segment_index + 1) % point_count;
                SOCKSEGMENT *segment = &sock->segments[segment_index];

                SockMidpointAt(sock, segment_index, &segment->midpoint);
                SockMidpointAt(sock, next, &segment->next_midpoint);

                segment->min = sock->a->pts[segment_index];
                segment->max = segment->min;
                AdjustMinMaxBox(&sock->b->pts[segment_index], &segment->min, &segment->max);
                if ((sock->flags & 1) == 0) {
                    AdjustMinMaxBox(&sock->c->pts[segment_index], &segment->min, &segment->max);
                    AdjustMinMaxBox(&sock->d->pts[segment_index], &segment->min, &segment->max);
                }
                AdjustMinMaxBox(&sock->a->pts[next], &segment->min, &segment->max);
                AdjustMinMaxBox(&sock->b->pts[next], &segment->min, &segment->max);
                if ((sock->flags & 1) == 0) {
                    AdjustMinMaxBox(&sock->c->pts[next], &segment->min, &segment->max);
                    AdjustMinMaxBox(&sock->d->pts[next], &segment->min, &segment->max);

                    NuVecSurfaceNormal(&segment->planes[0], &sock->a->pts[segment_index], &sock->b->pts[segment_index],
                                       &sock->d->pts[segment_index]);
                    NuVecSurfaceNormal(&segment->planes[1], &sock->a->pts[next], &sock->b->pts[next],
                                       &sock->b->pts[segment_index]);
                    NuVecSurfaceNormal(&segment->planes[2], &sock->b->pts[next], &sock->c->pts[next],
                                       &sock->c->pts[segment_index]);
                    NuVecSurfaceNormal(&segment->planes[3], &sock->c->pts[next], &sock->d->pts[next],
                                       &sock->d->pts[segment_index]);
                    NuVecSurfaceNormal(&segment->planes[4], &sock->d->pts[next], &sock->a->pts[next],
                                       &sock->a->pts[segment_index]);
                    NuVecSurfaceNormal(&segment->planes[5], &sock->b->pts[next], &sock->a->pts[next],
                                       &sock->c->pts[next]);
                }

                segment->length = NuVecDist(&segment->midpoint, &segment->next_midpoint, NULL);
                segment->distance_from_start = distance_from_start;
                distance_from_start += segment->length;
            }
            sock->unknown_98 = sock->mid == NULL ? 0.0f : SplineLength(sock->mid, sock->unknown_33);
            if (sock->mid == NULL) {
                i32 segment_count = sock->length + (sock->unknown_33 != 0 ? 1 : 0);
                for (i32 segment = 0; segment < segment_count; ++segment) {
                    sock->unknown_98 += sock->segments[segment].length;
                }
            }
            buf->addr += segments_size;
        }

        buf->addr = ALIGN(buf->addr, 16);
        for (i32 sock_index = 0; sock_index < 0x40; ++sock_index) {
            SOCK *sock = &sock_sys->sock[sock_index];
            if (sock->valid == 0) {
                continue;
            }
            for (i32 other_index = 0; other_index < 0x40; ++other_index) {
                SOCK *other = &sock_sys->sock[other_index];
                if (sock_index == other_index || other->valid == 0) {
                    continue;
                }
                bool ignore_y = (sock->flags & 1) != 0 || (other->flags & 1) != 0;
                if (!BoundsOverlap(sock->min, sock->max, other->min, other->max, ignore_y)) {
                    SetSockBitValue(sock, other_index);
                    continue;
                }

                i32 segment_count = sock->length + (sock->unknown_33 != 0 ? 1 : 0);
                i32 other_segment_count = other->length + (other->unknown_33 != 0 ? 1 : 0);
                bool overlap = false;
                for (i32 segment = 0; segment < segment_count && !overlap; ++segment) {
                    for (i32 other_segment = 0; other_segment < other_segment_count; ++other_segment) {
                        if (BoundsOverlap(sock->segments[segment].min, sock->segments[segment].max,
                                          other->segments[other_segment].min, other->segments[other_segment].max,
                                          ignore_y)) {
                            overlap = true;
                            break;
                        }
                    }
                }
                if (!overlap) {
                    SetSockBitValue(sock, other_index);
                }
            }
        }
    }

} // extern "C"

i32 GetSockEdgeEnum(char *name) {
    if (NuStrICmp(name, const_cast<char *>("SIDE")) == 0) {
        return 0;
    }
    if (NuStrICmp(name, const_cast<char *>("END")) == 0) {
        return 1;
    }
    return -1;
}

f32 DistanceToLineXZ(NUVEC *, NUVEC *, NUVEC *);
f32 CalculateDistanceToNearestSide(NUVEC *position, SOCKPOSITION *resolved, i32 unused, SOCKSYS *system) {
    SOCK *sock;
    f32 distance;
    NUVEC *a0, *a1, *b0, *b1;
    f32 distance_a, distance_b;
    NUVEC *d0, *d1;
    f32 distance_d;
    NUVEC a, b, d, flat;
    flat = *position;
    sock = &system->sock[resolved->location.sock];
    distance = 0.0f;
    flat.y = 0.0f;
    if (sock->flags & 1) {
        a0 = &sock->a->pts[resolved->next_segment - 1];
        a1 = &sock->a->pts[resolved->next_segment];
        a.x = a0->x + (a1->x - a0->x) * resolved->ratio;
        a.y = 0.0f;
        a.z = a0->z + (a1->z - a0->z) * resolved->ratio;
        b0 = &sock->b->pts[resolved->next_segment - 1];
        b1 = &sock->b->pts[resolved->next_segment];
        b.x = b0->x + (b1->x - b0->x) * resolved->ratio;
        b.y = 0.0f;
        b.z = b0->z + (b1->z - b0->z) * resolved->ratio;
        distance_a = NuVecDistSqr(&flat, &a, NULL);
        distance_b = NuVecDistSqr(&flat, &b, NULL);
        distance = distance_a < distance_b ? distance_a : distance_b;
    } else {
        a0 = &sock->a->pts[resolved->next_segment - 1];
        a1 = &sock->a->pts[resolved->next_segment];
        a.x = a0->x + (a1->x - a0->x) * resolved->ratio;
        a.y = 0.0f;
        a.z = a0->z + (a1->z - a0->z) * resolved->ratio;
        d0 = &sock->d->pts[resolved->next_segment - 1];
        d1 = &sock->d->pts[resolved->next_segment];
        d.x = d0->x + (d1->x - d0->x) * resolved->ratio;
        d.y = 0.0f;
        d.z = d0->z + (d1->z - d0->z) * resolved->ratio;
        distance_a = NuVecDistSqr(&flat, &a, NULL);
        distance_d = NuVecDistSqr(&flat, &d, NULL);
        distance = distance_a < distance_d ? distance_a : distance_d;
    }
    return distance > 0.0f ? NuFsqrt(distance) : 0.0f;
}
f32 CalculateDistanceToNearestEnd(NUVEC *position, SOCKPOSITION *resolved, i32 unused, SOCKSYS *system) {
    NUVEC flat = *position;
    SOCK *sock = &system->sock[resolved->location.sock];
    f32 distance = 0.0f;
    NUVEC *first_a, *first_b, *last_a, *last_b;
    f32 start_distance, magnitude, along, ratio, end_distance, end_magnitude, end_along;
    NUVEC a, b, c, d, edge0, projected0, edge1, projected1, scaled0, scaled1, temporary, end_temporary;
    flat.y = 0.0f;
    first_a = sock->a->pts;
    first_b = (sock->flags & 1) ? sock->b->pts : sock->d->pts;
    a = *first_a;
    b = *first_b;
    a.y = b.y = 0.0f;
    last_a = &sock->a->pts[sock->a->length - 1];
    last_b = (sock->flags & 1) ? &sock->b->pts[sock->b->length - 1] : &sock->d->pts[sock->d->length - 1];
    c = *last_a;
    d = *last_b;
    c.y = d.y = 0.0f;
    start_distance = DistanceToLineXZ(position, first_a, first_b);
    NuVecSub(&temporary, &flat, &a);
    magnitude = NuFabs(NuVecMagVU0(&temporary));
    along = NuFsqrt(magnitude * magnitude - start_distance * start_distance);
    NuVecSub(&edge0, &b, &a);
    edge0.y = 0.0f;
    ratio = along / NuVecMagVU0(&edge0);
    scaled0.x = edge0.x * ratio;
    scaled0.y = edge0.y * ratio;
    scaled0.z = edge0.z * ratio;
    NuVecAdd(&projected0, &a, &scaled0);
    end_distance = DistanceToLineXZ(position, last_a, last_b);
    NuVecSub(&end_temporary, &flat, &c);
    end_magnitude = NuFabs(NuVecMagVU0(&end_temporary));
    end_along = NuFsqrt(end_magnitude * end_magnitude - end_distance * end_distance);
    NuVecSub(&edge1, &d, &c);
    edge1.y = 0.0f;
    ratio = end_along / NuVecMagVU0(&edge1);
    scaled1.x = edge1.x * ratio;
    scaled1.y = edge1.y * ratio;
    scaled1.z = edge1.z * ratio;
    NuVecAdd(&projected1, &c, &scaled1);
    distance = start_distance < end_distance ? start_distance : end_distance;
    return distance;
}
f32 CalculateDistanceToSpecificSideOrEnd(i32 side, NUVEC *position, SOCKPOSITION *resolved, i32 unused,
                                         SOCKSYS *system) {
    switch (side) {
        case 0:
            return CalculateDistanceToNearestSide(position, resolved, unused, system);
        case 1:
            return CalculateDistanceToNearestEnd(position, resolved, unused, system);
        default:
            return 0.0f;
    }
}

static u32 sock_turnoff_mask[2];
void TurnOffAllSocksExcept(SOCKSYS *system, i32 exception) {
    for (i32 word = 0; word < 2; ++word)
        sock_turnoff_mask[word] = 0;
    for (i32 index = 0; index < 64; ++index) {
        if (!(system->sock[index].flags & 0x100))
            sock_turnoff_mask[index / 32] |= 1 << (index & 31);
        system->sock[index].flags |= 0x100;
    }
    system->sock[exception].flags &= ~0x100;
}
void RestoreLastSocksTurnoff(SOCKSYS *system) {
    for (i32 index = 0; index < 64; ++index) {
        if (sock_turnoff_mask[index / 32] & (1 << (index & 31)))
            system->sock[index].flags &= ~0x100;
    }
}
