#include "editor/edpath.h"
#include "legoapi/world/world_shared.h"
#include "decomp.h"
#include "gameapi/ai/aisys/aisys.h"
#include "globals.h"
#include "gameapi/edtools/edfile.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufile.h"
#include "nu2api/nufile/nufilepak.h"
#include "nu2api/numath/nuang.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/nuplane.h"

#include <stdio.h>
#include <float.h>
#include <string.h>

static const u32 iAISysPathColoursG[5] = {0xffff0000, 0xff00ff00, 0xff00ffff, 0xffff00ff, 0xffffff00};

extern "C" f32 (*GetCharacterGoalSpeedFn)(APIOBJECT *);

static __used__ i32 RowMoveWander(AIGROUP *group, AIROW *row, AIROW *previous, APIOBJECT *leader) {
    AIPATHCNX *connection = row->path_info.connection;
    if (connection == NULL)
        return 0;
    i32 node_index = connection->node_indices[row->path_info.direction == 0];
    if (row->next_connection == NULL) {
        if (previous != NULL) {
            row->next_connection = previous->path_info.connection == connection ? previous->next_connection
                                                                                : previous->path_info.connection;
            if (row->next_connection == NULL)
                return 0;
        } else {
            AIPATHNODE *node = &row->path_info.path->nodes[node_index];
            i32 first = NuRandInt() % node->connection_count;
            for (i32 i = 0; i < node->connection_count; ++i) {
                row->next_connection = node->connections[(first + i) % node->connection_count];
                AIPATHCNX *next = row->next_connection;
                if (next != connection) {
                    u8 route = leader->ai->field_0x138;
                    if (route == 0xff || ((static_cast<u64>(next->route_mask) >> route) & 1) != 0 ||
                        ((static_cast<u64>(node->route_boundary_mask) >> route) & 1) != 0) {
                        i32 direction = node_index != next->node_indices[0];
                        AIPATHNODE *other = &row->path_info.path->nodes[next->node_indices[direction ^ 1]];
                        if (other->radius > group->radius && next->traversal_flags[direction] == 0)
                            break;
                    }
                }
                row->next_connection = NULL;
            }
            if (row->next_connection == NULL)
                row->next_connection = connection;
        }
        AIPATHCNX *next = row->next_connection;
        if (next->traversal_flags[node_index != next->node_indices[0]] == 0) {
            i32 current_angle = connection->rotation;
            if (row->path_info.direction != 0)
                current_angle = NuAngAdd(0x8000, current_angle);
            i32 next_angle;
            if (connection->node_indices[row->path_info.direction == 0] == next->node_indices[0]) {
                row->next_direction = 0;
                next_angle = next->rotation;
            } else {
                row->next_direction = 1;
                next_angle = NuAngAdd(0x8000, next->rotation);
            }
            row->is_clockwise = NuAngSub(next_angle, current_angle) < 0;
        }
    }
    NUVEC offset = {row->is_clockwise ? -group->radius : group->radius, 0.0f, 0.0f};
    NUVEC rotated, centre;
    NuVecRotateY(&rotated, &offset, row->y_rot);
    NuVecAdd(&centre, &rotated, &row->pos);
    if (!row->is_turning) {
        if (row->path_info.connection != row->next_connection) {
            AIPATHNODE *nodes = row->path_info.path->nodes;
            f32 distance =
                NuInfiniteLineToPointDistSqr(&nodes[row->next_connection->node_indices[0]].position,
                                             &nodes[row->next_connection->node_indices[1]].position, &centre);
            if (group->radius * group->radius >= distance)
                row->is_turning = 1;
        } else {
            AIPATHNODE *node =
                &row->path_info.path->nodes[row->path_info.connection->node_indices[row->path_info.direction == 0]];
            f32 distance = NuVecXZDistSqr(&row->pos, &node->position, &rotated);
            if (node->radius_squared > distance) {
                for (i32 i = 0; i < group->row_count; ++i) {
                    group->rows[i].next_connection = NULL;
                    group->rows[i].y_rot = NuAngAdd(group->rows[i].y_rot, 0x8000);
                    group->rows[i].path_info.direction = group->rows[i].path_info.direction == 0;
                }
                group->leader = NULL;
                group->is_reversed = !group->is_reversed;
                if (group->is_reversed) {
                    for (i32 i = group->member_count - 1; i != -1; --i) {
                        APIOBJECT *object = group->members[i];
                        if (object != NULL && (object->field_0x1f8 & 0x1001) == 0x1001 && object->field_0x287 == 0) {
                            group->leader = object;
                            break;
                        }
                    }
                } else {
                    for (i32 i = 0; i < group->member_count; ++i) {
                        APIOBJECT *object = group->members[i];
                        if (object->objptr != NULL && (object->field_0x1f8 & 0x1001) == 0x1001 &&
                            object->field_0x287 == 0) {
                            group->leader = object;
                            break;
                        }
                    }
                }
                return 1;
            }
        }
    }
    if (row->is_turning) {
        i32 angle = row->next_connection->rotation;
        if (row->next_direction != 0)
            angle = NuAngAdd(angle, 0x8000);
        i32 difference = NuAngSub(angle, row->y_rot);
        i32 step = group->rotation_speed;
        if (difference <= step) {
            step = -step;
            if (difference >= step) {
                row->is_turning = 0;
                row->path_info.connection = row->next_connection;
                row->next_connection = NULL;
                row->path_info.direction = row->next_direction;
                step = difference;
            }
        }
        row->y_rot = NuAngAdd(step, row->y_rot);
        NuVecRotateY(&rotated, &offset, row->y_rot);
        NuVecSub(&row->pos, &centre, &rotated);
        return 0;
    }
    if (!group->is_row_turning) {
        i32 angle = row->path_info.connection->rotation;
        if (row->path_info.direction != 0)
            angle = NuAngAdd(angle, 0x8000);
        i32 difference = NuAngSub(angle, row->y_rot);
        i32 step = group->rotation_speed;
        if (difference <= step) {
            step = -step;
            if (difference >= step)
                step = difference;
        }
        row->y_rot = NuAngAdd(step, row->y_rot);
        rotated.x = rotated.y = rotated.z = 0.0f;
        if (GetCharacterGoalSpeedFn != NULL)
            rotated.z = GetCharacterGoalSpeedFn(leader);
        NuVecRotateY(&rotated, &rotated, row->y_rot);
        NuVecAdd(&row->pos, &row->pos, &rotated);
        row->pos.y = leader->position.y;
    }
    return 0;
}

static __used__ i32 RowMoveTowards(AIGROUP *group, AIROW *row, AIROW *previous, APIOBJECT *leader) {
    AIPATHCNX *connection = row->path_info.connection;
    if (connection == NULL)
        return 0;
    i32 node_index = connection->node_indices[row->path_info.direction == 0];
    i32 direction_node = node_index;
    if (row->next_connection == NULL) {
        if (previous != NULL) {
            row->next_connection = previous->path_info.connection == connection ? previous->next_connection
                                                                                : previous->path_info.connection;
        } else if (group->leader != NULL) {
            AIPATHINFO *destination = &group->leader->ai->script_process.path_info;
            if (destination->path == row->path_info.path) {
                i32 destination_node = destination->connection->node_indices[destination->dist > 0.5f];
                direction_node = destination_node;
                u8 connection_index = destination->path->route_matrix[node_index][destination_node];
                if (connection_index != 0xff) {
                    row->next_connection = destination->path->nodes[node_index].connections[connection_index];
                    if (row->next_connection->traversal_flags[node_index != row->next_connection->node_indices[0]] != 0)
                        row->next_connection = NULL;
                }
            }
        }
        AIPATHCNX *next = row->next_connection;
        if (next != NULL && next->traversal_flags[direction_node != next->node_indices[0]] == 0) {
            i32 current_angle = connection->rotation;
            if (row->path_info.direction != 0)
                current_angle = NuAngAdd(0x8000, current_angle);
            i32 next_angle;
            if (connection->node_indices[row->path_info.direction == 0] == next->node_indices[0]) {
                row->next_direction = 0;
                next_angle = next->rotation;
            } else {
                row->next_direction = 1;
                next_angle = NuAngAdd(0x8000, next->rotation);
            }
            row->is_clockwise = NuAngSub(next_angle, current_angle) < 0;
        }
    }
    if (row->next_connection == NULL) {
        if (row->path_info.direction != 0) {
            if (row->path_info.dist < 0.0f)
                return 1;
        } else if (row->path_info.dist > 1.0f) {
            return 1;
        }
        NUVEC movement;
        i32 angle = row->path_info.connection->rotation;
        if (row->path_info.direction != 0)
            angle = NuAngAdd(angle, 0x8000);
        i32 difference = NuAngSub(angle, row->y_rot);
        i32 step = group->rotation_speed;
        if (difference <= step) {
            step = -step;
            if (difference >= step)
                step = difference;
        }
        row->y_rot = NuAngAdd(step, row->y_rot);
        movement.x = movement.y = movement.z = 0.0f;
        if (GetCharacterGoalSpeedFn != NULL)
            movement.z = GetCharacterGoalSpeedFn(leader);
        NuVecRotateY(&movement, &movement, row->y_rot);
        NuVecAdd(&row->pos, &row->pos, &movement);
        row->pos.y = leader->position.y;
        return 0;
    }
    NUVEC offset = {row->is_clockwise ? -group->radius : group->radius, 0.0f, 0.0f};
    NUVEC rotated, centre;
    NuVecRotateY(&rotated, &offset, row->y_rot);
    NuVecAdd(&centre, &rotated, &row->pos);
    if (!row->is_turning) {
        if (row->path_info.connection != row->next_connection) {
            AIPATHNODE *nodes = row->path_info.path->nodes;
            f32 distance =
                NuInfiniteLineToPointDistSqr(&nodes[row->next_connection->node_indices[0]].position,
                                             &nodes[row->next_connection->node_indices[1]].position, &centre);
            if (group->radius * group->radius >= distance)
                row->is_turning = 1;
        } else {
            AIPATHNODE *node =
                &row->path_info.path->nodes[row->path_info.connection->node_indices[row->path_info.direction == 0]];
            f32 distance = NuVecXZDistSqr(&row->pos, &node->position, &rotated);
            if (node->radius_squared > distance) {
                for (i32 i = 0; i < group->row_count; ++i) {
                    group->rows[i].next_connection = NULL;
                    group->rows[i].y_rot = NuAngAdd(group->rows[i].y_rot, 0x8000);
                    group->rows[i].path_info.direction = group->rows[i].path_info.direction == 0;
                }
                group->leader = NULL;
                group->is_reversed = !group->is_reversed;
                if (group->is_reversed) {
                    for (i32 i = group->member_count - 1; i != -1; --i) {
                        APIOBJECT *object = group->members[i];
                        if (object != NULL && (object->field_0x1f8 & 0x1001) == 0x1001 && object->field_0x287 == 0) {
                            group->leader = object;
                            break;
                        }
                    }
                } else {
                    for (i32 i = 0; i < group->member_count; ++i) {
                        APIOBJECT *object = group->members[i];
                        if (object->objptr != NULL && (object->field_0x1f8 & 0x1001) == 0x1001 &&
                            object->field_0x287 == 0) {
                            group->leader = object;
                            break;
                        }
                    }
                }
                return 1;
            }
        }
    }
    if (row->is_turning) {
        i32 angle = row->next_connection->rotation;
        if (row->next_direction != 0)
            angle = NuAngAdd(angle, 0x8000);
        i32 difference = NuAngSub(angle, row->y_rot);
        i32 step = group->rotation_speed;
        if (difference <= step) {
            step = -step;
            if (difference >= step) {
                row->is_turning = 0;
                row->path_info.connection = row->next_connection;
                row->next_connection = NULL;
                row->path_info.direction = row->next_direction;
                step = difference;
            }
        }
        row->y_rot = NuAngAdd(step, row->y_rot);
        NuVecRotateY(&rotated, &offset, row->y_rot);
        NuVecSub(&row->pos, &centre, &rotated);
        return 0;
    }
    if (!group->is_row_turning) {
        i32 angle = row->path_info.connection->rotation;
        if (row->path_info.direction != 0)
            angle = NuAngAdd(angle, 0x8000);
        i32 difference = NuAngSub(angle, row->y_rot);
        i32 step = group->rotation_speed;
        if (difference <= step) {
            step = -step;
            if (difference >= step)
                step = difference;
        }
        row->y_rot = NuAngAdd(step, row->y_rot);
        rotated.x = rotated.y = rotated.z = 0.0f;
        if (GetCharacterGoalSpeedFn != NULL)
            rotated.z = GetCharacterGoalSpeedFn(leader);
        NuVecRotateY(&rotated, &rotated, row->y_rot);
        NuVecAdd(&row->pos, &row->pos, &rotated);
        row->pos.y = leader->position.y;
    }
    return 0;
}

static void FormationMove(AIGROUP *group, i32 (*move)(AIGROUP *, AIROW *, AIROW *, APIOBJECT *)) {
    i32 turning = 0;
    if (move != NULL && group->is_in_formation) {
        AIROW *previous = NULL;
        if (group->is_reversed) {
            for (i32 i = group->row_count - 1; i != -1; --i) {
                AIROW *row = &group->rows[i];
                if (row->is_alive) {
                    if (move(group, row, previous, group->leader) != 0)
                        break;
                    if (row->is_turning)
                        turning = 1;
                    previous = row;
                }
            }
        } else {
            for (i32 i = 0; i < group->row_count; ++i) {
                AIROW *row = &group->rows[i];
                if (row->is_alive) {
                    if (move(group, row, previous, group->leader) != 0)
                        break;
                    if (row->is_turning)
                        turning = 1;
                    previous = row;
                }
            }
        }
    }
    group->is_row_turning = turning;
}

static void AISysCheckAntinode_Rectangle(APIOBJECT *object, AIANTINODE *antinode, NUVEC *difference, f32 radius) {
    difference->x = object->ai->movement_position.x - antinode->position.x;
    if (difference->x > radius || difference->x < -radius) {
        return;
    }
    difference->z = object->ai->movement_position.z - antinode->position.z;
    if (difference->z > radius || difference->z < -radius || object->collision_min.y > antinode->max_y ||
        !(antinode->min_y <= object->collision_max.y)) {
        return;
    }
    NuVecRotateY(difference, difference, -antinode->flags);
    if (fabsf(difference->x) < antinode->base_radius + object->ai->mover_height &&
        fabsf(difference->z) < antinode->base_height + object->ai->mover_height) {
        NUVEC local_object;
        NUVEC local_destination;
        difference->x = object->position.x - antinode->position.x;
        difference->y = 0.0f;
        difference->z = object->position.z - antinode->position.z;
        NuVecRotateY(&local_object, difference, -antinode->flags);
        i32 object_angle = NuAtan2D(local_object.x, local_object.z);
        difference->x = object->ai->movement_destination.x - antinode->position.x;
        difference->y = 0.0f;
        difference->z = object->ai->movement_destination.z - antinode->position.z;
        NuVecRotateY(&local_destination, difference, -antinode->flags);
        i32 turn = NuAngSub(NuAtan2D(local_destination.x, local_destination.z), object_angle);
        i32 turn_offset = 0;
        if (object->ai->antinode_timer > 0.0f) {
            if (((object->ai->path_info.flags & 3) == 2 || object->respawn_timer > 1.0f) &&
                object->ai->antinode_timer < antinode_time) {
                object->ai->antinode_clockwise = !object->ai->antinode_clockwise;
                object->ai->antinode_timer = antinode_time + antinode_reverse_time;
            }
            if (object->ai->antinode_clockwise) {
                if (turn < 0) {
                    turn_offset = 0x10000;
                }
            } else if (turn > 0) {
                turn_offset = -0x10000;
            }
        } else if (turn > 0) {
            object->ai->antinode_clockwise = 1;
        } else {
            object->ai->antinode_clockwise = 0;
        }
        if ((antinode->game_flags & 2) == 0 && object->ai->antinode_timer < antinode_time) {
            object->ai->antinode_timer = antinode_time;
        }
        i32 corner_angle = NuAtan2D(antinode->base_radius, antinode->base_height);
        i32 angle = NuAngAdd(object_angle, 0);
        f32 tangent_x;
        f32 tangent_z;
        if (angle > 0x8000 - corner_angle) {
            object->ai->movement_position.x = (antinode->base_radius + object->ai->mover_height) * NU_SIN_LUT(angle);
            object->ai->movement_position.z = -(object->ai->mover_height + antinode->base_height);
            tangent_x = -1.0f;
            tangent_z = 0.0f;
        } else if (angle > corner_angle) {
            object->ai->movement_position.x = antinode->base_radius + object->ai->mover_height;
            object->ai->movement_position.z = (object->ai->mover_height + antinode->base_height) * NU_COS_LUT(angle);
            tangent_x = 0.0f;
            tangent_z = -1.0f;
        } else if (angle > -corner_angle) {
            object->ai->movement_position.x = (antinode->base_radius + object->ai->mover_height) * NU_SIN_LUT(angle);
            object->ai->movement_position.z = object->ai->mover_height + antinode->base_height;
            tangent_x = 1.0f;
            tangent_z = 0.0f;
        } else if (angle >= corner_angle - 0x7fff) {
            object->ai->movement_position.x = -(antinode->base_radius + object->ai->mover_height);
            object->ai->movement_position.z = (object->ai->mover_height + antinode->base_height) * NU_COS_LUT(angle);
            tangent_x = 0.0f;
            tangent_z = 1.0f;
        } else {
            object->ai->movement_position.x = (antinode->base_radius + object->ai->mover_height) * NU_SIN_LUT(angle);
            object->ai->movement_position.z = -(object->ai->mover_height + antinode->base_height);
            tangent_x = -1.0f;
            tangent_z = 0.0f;
        }
        if (turn + turn_offset < 0) {
            tangent_x = -tangent_x;
            tangent_z = -tangent_z;
        }
        i32 absolute_turn = turn < 0 ? -turn : turn;
        if (absolute_turn < 1820) {
            f32 scale = static_cast<f32>(absolute_turn) / 1820.0f;
            tangent_x *= scale;
            tangent_z *= scale;
        }
        object->ai->movement_position.y = 0.0f;
        object->ai->movement_position.x += tangent_x;
        object->ai->movement_position.z += tangent_z;
        NuVecRotateY(&object->ai->movement_position, &object->ai->movement_position, antinode->flags);
        NuVecAdd(&object->ai->movement_position, &object->ai->movement_position, &antinode->position);
        object->field_0x1fa |= 4;
    }
}

static void AISysCheckAntinode_Ellipse(APIOBJECT *object, AIANTINODE *antinode, NUVEC *difference, f32 radius) {
    f32 distance_squared = difference->x * difference->x + difference->z * difference->z;
    if (distance_squared < radius * radius) {
        f32 aspect = antinode->base_height / antinode->base_radius;
        NUVEC local_position;
        NUVEC boundary;
        difference->y = 0.0f;
        NuVecRotateY(&local_position, difference, -antinode->flags);
        i32 angle = NuAtan2D(aspect * local_position.x, local_position.z);
        boundary.x = NU_SIN_LUT(angle) * antinode->base_radius;
        boundary.y = 0.0f;
        boundary.z = NU_COS_LUT(angle) * antinode->base_height;
        NuVecNorm(difference, &boundary);
        NuVecScale(difference, difference, object->ai->mover_height);
        NuVecAdd(&boundary, &boundary, difference);
        if (distance_squared < boundary.x * boundary.x + boundary.z * boundary.z) {
            NUVEC local_object;
            NUVEC object_boundary;
            NUVEC local_destination;
            difference->x = object->position.x - antinode->position.x;
            difference->y = 0.0f;
            difference->z = object->position.z - antinode->position.z;
            NuVecRotateY(&local_object, difference, -antinode->flags);
            i32 object_angle = NuAtan2D(aspect * local_object.x, local_object.z);
            object_boundary.x = NU_SIN_LUT(object_angle) * antinode->base_radius;
            object_boundary.y = 0.0f;
            object_boundary.z = NU_COS_LUT(object_angle) * antinode->base_height;
            NuVecNorm(difference, &object_boundary);
            NuVecScale(difference, difference, object->ai->mover_height);
            NuVecAdd(&object_boundary, &object_boundary, difference);
            difference->x = object->ai->movement_destination.x - antinode->position.x;
            difference->y = 0.0f;
            difference->z = object->ai->movement_destination.z - antinode->position.z;
            NuVecRotateY(&local_destination, difference, -antinode->flags);
            i32 turn = NuAngSub(NuAtan2D(aspect * local_destination.x, local_destination.z), object_angle);
            if (object->ai->antinode_timer > 0.0f) {
                if (((object->ai->path_info.flags & 3) == 2 || object->respawn_timer > 1.0f) &&
                    object->ai->antinode_timer < antinode_time) {
                    object->ai->antinode_clockwise = !object->ai->antinode_clockwise;
                    object->ai->antinode_timer = antinode_time + antinode_reverse_time;
                }
                if (object->ai->antinode_clockwise) {
                    if (turn < 0) {
                        turn += 0x10000;
                    }
                } else if (turn > 0) {
                    turn -= 0x10000;
                }
            } else if (turn > 0) {
                object->ai->antinode_clockwise = 1;
            } else {
                object->ai->antinode_clockwise = 0;
            }
            if ((antinode->game_flags & 2) == 0 && object->ai->antinode_timer < antinode_time) {
                object->ai->antinode_timer = antinode_time;
            }
            f32 tangent_x = NU_COS_LUT(object_angle) * antinode->base_radius;
            f32 tangent_z = -NU_SIN_LUT(object_angle) * antinode->base_height;
            if (turn < 0) {
                tangent_x = -tangent_x;
                tangent_z = -tangent_z;
            }
            i32 absolute_turn = turn < 0 ? -turn : turn;
            if (absolute_turn < 1820) {
                f32 scale = static_cast<f32>(absolute_turn) / 1820.0f;
                tangent_x *= scale;
                tangent_z *= scale;
            }
            object->ai->movement_position.y = 0.0f;
            object->ai->movement_position.x = object_boundary.x + tangent_x;
            object->ai->movement_position.z = object_boundary.z + tangent_z;
            NuVecRotateY(&object->ai->movement_position, &object->ai->movement_position, antinode->flags);
            NuVecAdd(&object->ai->movement_position, &object->ai->movement_position, &antinode->position);
            object->field_0x1fa |= 4;
        }
    }
}

static void AISysCheckAntinode_Circle(APIOBJECT *object, AIANTINODE *antinode, NUVEC *difference, f32 radius) {
    if (difference->x * difference->x + difference->z * difference->z < radius * radius) {
        difference->x = object->position.x - antinode->position.x;
        difference->z = object->position.z - antinode->position.z;
        i32 angle = NuAtan2D(difference->x, difference->z);
        difference->x = object->ai->movement_destination.x - antinode->position.x;
        difference->z = object->ai->movement_destination.z - antinode->position.z;
        i32 turn = NuAngSub(NuAtan2D(difference->x, difference->z), angle);
        if (object->ai->antinode_timer > 0.0f) {
            if (((object->ai->path_info.flags & 3) == 2 || object->respawn_timer > 1.0f) &&
                object->ai->antinode_timer < antinode_time) {
                object->ai->antinode_clockwise = !object->ai->antinode_clockwise;
                object->ai->antinode_timer = antinode_time + antinode_reverse_time;
            }
            if (object->ai->antinode_clockwise) {
                if (turn < 0) {
                    turn += 0x10000;
                }
            } else if (turn > 0) {
                turn -= 0x10000;
            }
        } else {
            if (turn > 0) {
                object->ai->antinode_clockwise = 1;
            } else {
                object->ai->antinode_clockwise = 0;
            }
        }
        if ((antinode->game_flags & 2) == 0 && object->ai->antinode_timer < antinode_time) {
            object->ai->antinode_timer = antinode_time;
        }
        i32 max_turn = static_cast<i32>((ai_moveradius / radius) * 10430.3779296875f);
        if (turn > max_turn) {
            turn = max_turn;
        } else if (turn < -max_turn) {
            turn = -max_turn;
        }
        angle = NuAngAdd(angle, turn);
        difference->x = 0.0f;
        difference->y = 0.0f;
        difference->z = radius;
        NuVecRotateY(difference, difference, angle);
        object->ai->movement_position.x = difference->x + antinode->position.x;
        object->ai->movement_position.y = object->ai->movement_destination.y;
        object->ai->movement_position.z = difference->z + antinode->position.z;
        object->field_0x1fa |= 4;
    }
}

extern "C" {
    void (*checkantinodefns[3])(APIOBJECT *, AIANTINODE *, NUVEC *, f32) = {
        AISysCheckAntinode_Circle, AISysCheckAntinode_Ellipse, AISysCheckAntinode_Rectangle};
    char *AiLevelPathName = "";
    AISCRIPTPROCESS *pSetStateDebugee;
    SCRIPTPROCESSFIRSTTIMEACTION *ScriptProcessFirstTimeActionFn;
    extern f32 default_path_heighttol;
    extern void (*checkantinodefns[3])(APIOBJECT_s *, AIANTINODE_s *, NUVEC *, f32);
}

void AIPathCalcExtents(AIPATH *path);
i32 AIPathCheckExtents(AIPATH *path, NUVEC *position);
void AIMoveToDestination(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);
void AIRetreatFromDestination(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);
void AICircle(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);
void AIWander(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);
void AIMoveToDestinationAvoidingCamera(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);
void AIMoveDirectlyToDestination(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks);

typedef i32 MIDSPECIALMOVE(AISYS *system, AIPACKET *packet, APIOBJECT *object);
extern MIDSPECIALMOVE *MidSpecialMoveFn;

extern "C" void *AISysBufferAlloc(VARIPTR *cursor, VARIPTR *buf_end, u32 size);
extern "C" i32 AISysSetLevelPath(AISYS *system, char *path_name);
extern "C" void AISysFindRoute(AIPACKET *packet);
extern "C" void AISysCharacterSetPath(AIPACKET *packet, AIPATH *path);
extern "C" void AISysCharacterSetPathCnx(AIPACKET *packet, NUVEC *position, AIPATHCNX *connection, i32 direction);

extern "C" void AIPathNodeUpdatePos(AISYS *system, AIPATH *path, AIPATHNODE *node);

static void *AISysLoadAlloc(AISYS *system, u32 size) {
    return AISysBufferAlloc(&system->storage_cursor, &system->storage_end, size);
}

static char *AISysLoadString(AISYS *system, i32 length) {
    if (length == 0) {
        return NULL;
    }

    char *text = static_cast<char *>(AISysLoadAlloc(system, length + 1));
    EdFileRead(text, length);
    return text;
}

static void AISysLoadPathRoutes(AISYS *system, AIPATH *path) {
    if (path->node_count != 0) {
        path->route_matrix = static_cast<u8 **>(AISysLoadAlloc(system, path->node_count * sizeof(u8 *)));
        for (i32 i = 0; i < path->node_count; ++i) {
            path->route_matrix[i] = static_cast<u8 *>(AISysLoadAlloc(system, path->node_count));
            EdFileRead(path->route_matrix[i], path->node_count);
        }
    }

    path->route_count = static_cast<u8>(EdFileReadChar());
    if (path->route_count != 0) {
        path->routes = static_cast<AIPATHROUTE *>(AISysLoadAlloc(system, path->route_count * sizeof(AIPATHROUTE)));
        for (i32 i = 0; i < path->route_count; ++i) {
            AIPATHROUTE *route = &path->routes[i];
            i32 name_length = EdFileReadChar();
            route->name = AISysLoadString(system, name_length);
            if (name_length != 0) {
                route->route_count = static_cast<u8>(EdFileReadChar());
                route->exit_node_count = static_cast<u8>(EdFileReadChar());
                EdFileReadChar();
                EdFileReadChar();

                if (path->node_count != 0 && route->route_count != 0) {
                    route->node_routes = static_cast<u8 *>(AISysLoadAlloc(system, path->node_count));
                    EdFileRead(route->node_routes, path->node_count);
                    route->node_directions = static_cast<u8 *>(AISysLoadAlloc(system, route->route_count));
                    EdFileRead(route->node_directions, route->route_count);
                    route->route_nodes = static_cast<u8 **>(AISysLoadAlloc(system, route->route_count * sizeof(u8 *)));
                    for (i32 route_index = 0; route_index < route->route_count; ++route_index) {
                        route->route_nodes[route_index] = static_cast<u8 *>(AISysLoadAlloc(system, route->route_count));
                        EdFileRead(route->route_nodes[route_index], route->route_count);
                    }
                    if (route->exit_node_count != 0) {
                        route->exit_nodes = static_cast<u8 *>(AISysLoadAlloc(system, route->exit_node_count));
                        EdFileRead(route->exit_nodes, route->exit_node_count);
                    }
                }
            }

            i32 character_count = EdFileReadChar();
            for (i32 character_index = 0; character_index < character_count; ++character_index) {
                char character_name[256];
                i32 character_name_length = EdFileReadChar();
                EdFileRead(character_name, character_name_length);
                if (SpecialRouteCharacterTypeIDFn != NULL) {
                    u8 type = SpecialRouteCharacterTypeIDFn(character_name);
                    if (type < 64) {
                        route->character_masks[0] |= static_cast<u64>(1) << type;
                        route->character_masks[1] |= static_cast<u64>(1) << type;
                    } else {
                        route->character_mask[0] = 0xffffffff;
                        route->character_mask[1] = 0xffffffff;
                        route->character_mask[3] |= 0x80000000;
                    }
                }
            }
        }
    }

    path->special_route_count = static_cast<u8>(EdFileReadChar());
    if (path->special_route_count != 0) {
        path->special_routes =
            static_cast<AIPATHNODELINK *>(AISysLoadAlloc(system, path->special_route_count * sizeof(AIPATHNODELINK)));
        for (i32 i = 0; i < path->special_route_count; ++i) {
            path->special_routes[i].node_index = EdFileReadUnsignedChar();
            path->special_routes[i].special_route_index = EdFileReadShort();
        }
    }
}

static AIPATHSYS *AISysLoadPaths(AISYS *system, i32 version, NUGSCN *scene) {
    i32 path_count = EdFileReadInt();
    if (path_count == 0) {
        return NULL;
    }

    AIPATHSYS *path_system = static_cast<AIPATHSYS *>(AISysLoadAlloc(system, sizeof(AIPATHSYS)));
    if (path_system == NULL) {
        return NULL;
    }
    path_system->path_count = static_cast<u8>(path_count);
    path_system->paths = static_cast<AIPATH **>(AISysLoadAlloc(system, path_system->path_count * sizeof(AIPATH *)));

    for (i32 path_index = 0; path_index < path_system->path_count; ++path_index) {
        AIPATH *path = static_cast<AIPATH *>(AISysLoadAlloc(system, sizeof(AIPATH)));
        path_system->paths[path_index] = path;
        EdFileRead(path->name, sizeof(path->name));
        path->node_count = static_cast<u8>(EdFileReadChar());
        path->flags = static_cast<u8>(EdFileReadChar());
        path->index = static_cast<u8>(path_index);
        path->connection_count = EdFileReadShort();

        if (path->connection_count != 0) {
            path->connections =
                static_cast<AIPATHCNX *>(AISysLoadAlloc(system, path->connection_count * sizeof(AIPATHCNX)));
            for (i32 connection_index = 0; connection_index < path->connection_count; ++connection_index) {
                AIPATHCNX *connection = &path->connections[connection_index];
                connection->direction_a = static_cast<u8>(EdFileReadChar());
                connection->direction_b = static_cast<u8>(EdFileReadChar());
                connection->node_a = EdFileReadInt();
                connection->node_b = EdFileReadInt();
                connection->previous_node_a = connection->node_a;
                connection->previous_node_b = connection->node_b;
                connection->flags = EdFileReadShort();
                connection->game_flags = EdFileReadShort();
                connection->width = EdFileReadFloat();
                connection->cost = EdFileReadFloat();
                if (connection->cost == 0.0f)
                    connection->cost = 0.00001f;
            }
        }

        if (path->node_count != 0) {
            path->nodes = static_cast<AIPATHNODE *>(AISysLoadAlloc(system, path->node_count * sizeof(AIPATHNODE)));
            for (i32 node_index = 0; node_index < path->node_count; ++node_index) {
                AIPATHNODE *node = &path->nodes[node_index];
                node->name = AISysLoadString(system, EdFileReadInt());
                EdFileReadNuVec(&node->position);
                node->radius = EdFileReadFloat();
                node->radius_squared = node->radius * node->radius;
                node->min_height = EdFileReadFloat();
                node->max_height = EdFileReadFloat();
                node->min_height_offset = node->min_height - node->position.y;
                node->max_height_offset = node->max_height - node->position.y;
                node->connection_count = static_cast<u8>(EdFileReadChar());
                node->flags = static_cast<u8>(EdFileReadChar());
                EdFileReadChar();
                node->runtime_flags = static_cast<u8>(EdFileReadChar()) & ~6u;
                node->path_flags = EdFileReadShort();
                node->distance_cache_nodes[0] = 0xff;
                node->distance_cache_nodes[1] = 0xff;
                node->special_route_index = static_cast<u8>(EdFileReadChar());

                char special_name[256];
                i32 special_name_length = EdFileReadChar();
                if (special_name_length != 0) {
                    EdFileRead(special_name, special_name_length);
                    node->has_special =
                        static_cast<u8>(NuSpecialFind(scene, &node->special_handle, special_name, 1) != 0);
                    EdFileReadNuVec(&node->special_position);
                }

                if (path->connections != NULL && node->connection_count != 0) {
                    node->connections =
                        static_cast<AIPATHCNX **>(AISysLoadAlloc(system, node->connection_count * sizeof(AIPATHCNX *)));
                    for (i32 i = 0; i < node->connection_count; ++i) {
                        i16 connection_index = EdFileReadShort();
                        node->connections[i] = &path->connections[connection_index];
                    }
                    if ((node->connection_count & 1) != 0) {
                        EdFileReadShort();
                    }
                }

                node->route_membership_mask = EdFileReadShort();
                node->route_boundary_mask = EdFileReadShort();
            }
            AIPathCalcExtents(path);
        }

        AISysLoadPathRoutes(system, path);
    }

    path_system->special_route_count = EdFileReadShort();
    if (path_system->special_route_count != 0) {
        path_system->special_routes = static_cast<AIPATHSPECIALROUTE *>(
            AISysLoadAlloc(system, path_system->special_route_count * sizeof(AIPATHSPECIALROUTE)));
        for (i32 i = 0; i < path_system->special_route_count; ++i) {
            AIPATHSPECIALROUTE *route = &path_system->special_routes[i];
            route->path_count = static_cast<u8>(EdFileReadChar());
            route->paths = static_cast<AIPATH **>(AISysLoadAlloc(system, route->path_count * sizeof(AIPATH *)));
            for (i32 path_index = 0; path_index < route->path_count; ++path_index) {
                route->paths[path_index] = path_system->paths[EdFileReadChar()];
            }
        }
    }
    return path_system;
}

static AIAREA *AISysLoadFindArea(AISYS *system, char *name) {
    for (i32 index = 0; index < system->area_count; ++index) {
        if (NuStrICmp(system->areas[index].name, name) == 0) {
            return &system->areas[index];
        }
    }
    return NULL;
}

static i16 AISysPathIntersectionAngle(f32 value) {
    f32 absolute = NuFabs(value);
    f32 root = NuFsqrt(1.0f - value * value);
    f32 small = root < absolute ? root : absolute;
    f32 side = (absolute - 0.70710677f) * 3.40282e+38f;
    side = side < 1.0f ? (side > -1.0f ? side : -1.0f) : 1.0f;
    f32 sign = value * 3.40282e+38f;
    sign = sign < 1.0f ? (sign > -1.0f ? sign : -1.0f) : 1.0f;
    f32 product = side * sign;
    f32 x = small * product;
    f32 x2 = x * x;
    f32 x3 = x * x2;
    f32 x4 = x2 * x2;
    f32 x5 = x2 * x3;
    return static_cast<i16>(static_cast<i32>(((sign + product) * 0.785398f - x + (x * -0.166667f) * x2 +
                                              (-0.075f * x2) * x3 + (-0.0446429f * x3) * x4 + (x4 * -0.0303819f) * x5) *
                                             10430.4f));
}

extern "C" i32 WithinConnection(AISYS *system, NUVEC *position, AIPATH *path, AIPATHCNX *connection, i32 checks,
                                AIPATHCNX *previous_connection, i32 route, i32 ground, AIPATHINFO *path_info,
                                f32 radius, i32 update_once) {
    (void)checks;
    (void)ground;
    if (update_once != 0) {
        if (connection->last_search_checksum == path->search_checksum)
            return 0;
        connection->last_search_checksum = path->search_checksum;
    }
    if (route != 0xff && previous_connection != NULL &&
        ((static_cast<u64>(connection->route_mask) >> route) & 1) == 0) {
        AIPATHNODE *shared;
        if (previous_connection->node_indices[0] == connection->node_indices[0])
            shared = &path->nodes[connection->node_indices[0]];
        else if (previous_connection->node_indices[0] == connection->node_indices[1])
            shared = &path->nodes[connection->node_indices[1]];
        else if (previous_connection->node_indices[1] == connection->node_indices[0])
            shared = &path->nodes[connection->node_indices[0]];
        else if (previous_connection->node_indices[1] == connection->node_indices[1])
            shared = &path->nodes[connection->node_indices[1]];
        else
            return 0;
        if (shared == NULL || ((static_cast<u64>(shared->route_boundary_mask) >> route) & 1) == 0)
            return 0;
    }

    AIPATHNODE *first = &path->nodes[connection->node_indices[0]];
    AIPATHNODE *second = &path->nodes[connection->node_indices[1]];
    f32 first_radius = first->radius;
    f32 second_radius = second->radius;
    u8 narrow = 0;
    if (first_radius > radius + 0.05f)
        first_radius -= radius;
    else
        narrow = 1;
    if (second_radius > radius + 0.05f)
        second_radius -= radius;
    else
        narrow = 1;

    if (first->has_special != 0 &&
        (path->updated_node_bits[connection->node_indices[0] >> 3] & (1 << (connection->node_indices[0] & 7))) == 0)
        AIPathNodeUpdatePos(system, path, first);
    if (second->has_special != 0 &&
        (path->updated_node_bits[connection->node_indices[1] >> 3] & (1 << (connection->node_indices[1] & 7))) == 0)
        AIPathNodeUpdatePos(system, path, second);

    NUVEC delta;
    delta.x = position->x - first->position.x;
    delta.z = position->z - first->position.z;
    NUVEC local;
    NuVecRotateY(&local, &delta, -connection->rotation);
    if (NuFabs(second_radius - first_radius) > connection->horizontal_distance) {
        if (second_radius > first_radius) {
            first = second;
            first_radius = second_radius;
        }
        if (first->min_height > position->y || position->y > first->max_height)
            return 0;
        const f32 dx = position->x - first->position.x;
        const f32 dz = position->z - first->position.z;
        if (!(first_radius * first_radius >= dx * dx + dz * dz))
            return 0;
    } else {
        if (NuFabs(local.x) > first_radius && NuFabs(local.x) > second_radius)
            return 0;
        if (local.z < -first_radius || local.z > connection->horizontal_distance + second_radius)
            return 0;
        const i32 angle = AISysPathIntersectionAngle((second_radius - first_radius) / connection->horizontal_distance);
        NUVEC wall = local;
        if (wall.x < 0.0f)
            wall.x = -wall.x;
        NuVecRotateY(&wall, &wall, -angle);
        NUVEC radial;
        if (wall.z < 0.0f) {
            if (first->min_height > position->y || position->y > first->max_height)
                return 0;
            if (!(first_radius * first_radius >= NuVecXZDistSqr(position, &first->position, &radial)))
                return 0;
        } else if (wall.z > NU_COS_LUT(angle) * connection->horizontal_distance) {
            if (second->min_height > position->y || position->y > second->max_height)
                return 0;
            if (!(second_radius * second_radius >= NuVecXZDistSqr(position, &second->position, &radial)))
                return 0;
        } else {
            f32 minimum, maximum;
            if (first_radius > local.z) {
                minimum = first->min_height;
                maximum = first->max_height;
            } else if (local.z > connection->horizontal_distance - second_radius) {
                minimum = second->min_height;
                maximum = second->max_height;
            } else {
                const f32 fraction =
                    (local.z - first_radius) / (connection->horizontal_distance - (first_radius + second_radius));
                const f32 inverse = 1.0f - fraction;
                minimum = second->min_height * fraction + first->min_height * inverse;
                maximum = second->max_height * fraction + first->max_height * inverse;
            }
            if (minimum > position->y || position->y > maximum || !(first_radius > wall.x))
                return 0;
        }
    }
    if (path_info != NULL) {
        path_info->direction = 0;
        path_info->flags = ((path_info->flags | 1) & ~8) | (narrow << 3);
        path_info->connection = connection;
        path_info->dist = local.z / connection->horizontal_distance;
        path_info->width = local.x;
    }
    return 1;
}

static u32 AISysCharacterTestPathCnx(AISYS *system, APIOBJECT *object, AIPACKET *packet, AIPATHCNX *connection,
                                     i32 direction, f32 *nearest_distance) {
    AIPATH *path = packet->path_info.path;
    if (connection->last_search_checksum == path->search_checksum || path->nodes == NULL) {
        return 0;
    }
    connection->last_search_checksum = path->search_checksum;
    if (object->character_data != NULL) {
        u32 flags = connection->traversal_flags[0] | connection->traversal_flags[1];
        if (flags != 0) {
            if (direction != -1) {
                flags = connection->traversal_flags[direction];
            }
            if (flags != 0 && ((packet->capabilities & flags) == 0 || (flags & 0x98000000) != 0)) {
                return 0;
            }
        }
    }
    if ((object->flags_low & 0x80) == 0 && packet->current_route != 0xff &&
        ((static_cast<u64>(connection->route_mask) >> packet->current_route) & 1) == 0) {
        AIPATHCNX *current = packet->path_info.connection;
        if (current == NULL || current == connection) {
            return 0;
        }
        AIPATHNODE *shared = NULL;
        if (current->node_indices[0] == connection->node_indices[0]) {
            shared = &path->nodes[connection->node_indices[0]];
        } else if (current->node_indices[0] == connection->node_indices[1]) {
            shared = &path->nodes[current->node_indices[0]];
        } else if (current->node_indices[1] == connection->node_indices[0]) {
            shared = &path->nodes[connection->node_indices[0]];
        } else if (current->node_indices[1] == connection->node_indices[1]) {
            shared = &path->nodes[connection->node_indices[1]];
        }
        if (shared == NULL || ((static_cast<u64>(shared->route_boundary_mask) >> packet->current_route) & 1) == 0) {
            return 0;
        }
    }
    if (direction == -1) {
        direction = 0;
    }
    AIPATHNODE *first = &path->nodes[connection->node_indices[0]];
    AIPATHNODE *second = &path->nodes[connection->node_indices[1]];
    f32 first_radius = first->radius;
    bool narrow = false;
    if (first_radius > object->collision_radius + 0.05f) {
        first_radius -= object->collision_radius;
    } else {
        narrow = true;
    }
    f32 second_radius = second->radius;
    if (second_radius > object->collision_radius + 0.05f) {
        second_radius -= object->collision_radius;
    } else {
        narrow = true;
    }
    if (first->has_special != 0 &&
        (path->updated_node_bits[connection->node_indices[0] >> 3] & (1 << (connection->node_indices[0] & 7))) == 0) {
        AIPathNodeUpdatePos(system, path, first);
    }
    if (second->has_special != 0 &&
        (path->updated_node_bits[connection->node_indices[1] >> 3] & (1 << (connection->node_indices[1] & 7))) == 0) {
        AIPathNodeUpdatePos(system, path, second);
    }
    NUVEC delta;
    delta.x = object->position.x - first->position.x;
    delta.z = object->position.z - first->position.z;
    NUVEC local;
    NuVecRotateY(&local, &delta, -connection->rotation);
    NUVEC radial;
    if (NuFabs(second_radius - first_radius) > connection->horizontal_distance) {
        if (second_radius > first_radius) {
            first_radius = second_radius;
            first = second;
        }
        f32 height_distance = NuFabs(first->position.y - packet->terrain_origin.y);
        f32 distance = NuVecXZDist(&object->position, &first->position, &radial);
        f32 clearance;
        if (first->min_height > packet->terrain_origin.y || packet->terrain_origin.y > first->max_height) {
            clearance = distance - first_radius;
            clearance = height_distance > clearance ? height_distance : clearance;
        } else if (first_radius >= distance) {
            packet->path_info.flags = ((packet->path_info.flags | 1) & ~8) | (narrow << 3);
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->last_path_position = object->position;
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            i32 node_index = first - path->nodes;
            path->inside_node_bits[node_index / 8] |= 1 << (node_index % 8);
            packet->inside_path_node = node_index;
            if (second_radius * second_radius >= NuVecXZDistSqr(&object->position, &second->position, &radial)) {
                node_index = second - path->nodes;
                path->inside_node_bits[node_index / 8] |= 1 << (node_index % 8);
                packet->inside_path_node = node_index;
            }
            return 1;
        } else {
            clearance = distance - first_radius;
        }
        if (*nearest_distance > clearance) {
            *nearest_distance = clearance;
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            if (first_radius < distance) {
                packet->last_path_position.x = first_radius * radial.x / distance + first->position.x;
                packet->last_path_position.z = first_radius * radial.z / distance + first->position.z;
            } else {
                packet->last_path_position.x = object->position.x;
                packet->last_path_position.z = object->position.z;
            }
            packet->last_path_position.y = first->position.y;
            packet->inside_path_node = -1;
        }
        return 0;
    }

    const i32 angle = AISysPathIntersectionAngle((second_radius - first_radius) / connection->horizontal_distance);
    NUVEC wall = local;
    if (wall.x < 0.0f) {
        wall.x = -wall.x;
    }
    NuVecRotateY(&wall, &wall, -angle);
    const f32 length = NU_COS_LUT(angle) * connection->horizontal_distance;
    if (wall.z < 0.0f) {
        f32 height_distance = NuFabs(first->position.y - packet->terrain_origin.y);
        f32 distance = NuVecXZDist(&object->position, &first->position, &radial);
        f32 clearance;
        if (first->min_height > packet->terrain_origin.y || packet->terrain_origin.y > first->max_height) {
            clearance = distance - first_radius;
            clearance = height_distance > clearance ? height_distance : clearance;
        } else if (first_radius >= distance) {
            packet->path_info.flags = ((packet->path_info.flags | 1) & ~8) | (narrow << 3);
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->last_path_position = object->position;
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            u8 node_index = connection->node_indices[0];
            path->inside_node_bits[node_index >> 3] |= 1 << (node_index & 7);
            packet->inside_path_node = connection->node_indices[0];
            return 1;
        } else {
            clearance = distance - first_radius;
        }
        if (*nearest_distance > clearance) {
            *nearest_distance = clearance;
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            if (first_radius < distance) {
                packet->last_path_position.x = first_radius * radial.x / distance + first->position.x;
                packet->last_path_position.z = first_radius * radial.z / distance + first->position.z;
            } else {
                packet->last_path_position.x = object->position.x;
                packet->last_path_position.z = object->position.z;
            }
            packet->last_path_position.y = first->position.y;
            packet->inside_path_node = -1;
        }
    }
    if (wall.z > length) {
        f32 height_distance = NuFabs(second->position.y - packet->terrain_origin.y);
        f32 distance = NuVecXZDist(&object->position, &second->position, &radial);
        f32 clearance;
        if (second->min_height > packet->terrain_origin.y || packet->terrain_origin.y > second->max_height) {
            clearance = distance - second_radius;
            clearance = height_distance > clearance ? height_distance : clearance;
        } else if (second_radius >= distance) {
            packet->path_info.flags = ((packet->path_info.flags | 1) & ~8) | (narrow << 3);
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->last_path_position = object->position;
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            u8 node_index = connection->node_indices[1];
            path->inside_node_bits[node_index >> 3] |= 1 << (node_index & 7);
            packet->inside_path_node = connection->node_indices[1];
            return 1;
        } else {
            clearance = distance - second_radius;
        }
        if (*nearest_distance > clearance) {
            *nearest_distance = clearance;
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            if (second_radius < distance) {
                packet->last_path_position.x = second_radius * radial.x / distance + second->position.x;
                packet->last_path_position.z = second_radius * radial.z / distance + second->position.z;
            } else {
                packet->last_path_position.x = object->position.x;
                packet->last_path_position.z = object->position.z;
            }
            packet->last_path_position.y = second->position.y;
            packet->inside_path_node = -1;
        }
    } else if (!(wall.z < 0.0f)) {
        f32 height, minimum, maximum;
        if (first_radius > local.z) {
            height = first->position.y;
            minimum = first->min_height;
            maximum = first->max_height;
            if (local.z > connection->horizontal_distance - second_radius) {
                minimum = second->min_height < minimum ? second->min_height : minimum;
                maximum = second->max_height > maximum ? second->max_height : maximum;
            }
        } else if (local.z > connection->horizontal_distance - second_radius) {
            height = second->position.y;
            minimum = second->min_height;
            maximum = second->max_height;
        } else {
            f32 denominator = connection->horizontal_distance - (first_radius + second_radius);
            f32 numerator = local.z - first_radius;
            f32 fraction = denominator == 0.0f || numerator == 0.0f ? 0.0f : numerator / denominator;
            f32 inverse = 1.0f - fraction;
            height = fraction * second->position.y + first->position.y * inverse;
            minimum = second->min_height * fraction + first->min_height * inverse;
            maximum = fraction * second->max_height + first->max_height * inverse;
        }
        const f32 height_distance = NuFabs(height - packet->terrain_origin.y);
        f32 clearance;
        if (height_distance > 0.0f && (minimum > packet->terrain_origin.y || packet->terrain_origin.y > maximum)) {
            clearance = wall.x - first_radius;
            clearance = height_distance > clearance ? height_distance : clearance;
        } else if (first_radius > wall.x) {
            packet->path_info.flags = ((packet->path_info.flags | 1) & ~8) | (narrow << 3);
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            packet->last_path_position = object->position;
            packet->inside_path_node = -1;
            if (first_radius > local.z) {
                if (first_radius * first_radius >= NuVecXZDistSqr(&object->position, &first->position, &radial)) {
                    u8 node_index = connection->node_indices[0];
                    path->inside_node_bits[node_index >> 3] |= 1 << (node_index & 7);
                    packet->inside_path_node = connection->node_indices[0];
                }
            } else if (local.z > length - second_radius) {
                if (second_radius * second_radius >= NuVecXZDistSqr(&object->position, &second->position, &radial)) {
                    u8 node_index = connection->node_indices[1];
                    path->inside_node_bits[node_index >> 3] |= 1 << (node_index & 7);
                    packet->inside_path_node = connection->node_indices[1];
                }
            }
            return 1;
        } else {
            clearance = wall.x - first_radius;
        }
        if (*nearest_distance > clearance) {
            *nearest_distance = clearance;
            AISysCharacterSetPathCnx(packet, &object->position, connection, direction);
            packet->path_info.dist = local.z / connection->horizontal_distance;
            packet->path_info.width = local.x;
            if (first_radius > wall.x) {
                packet->last_path_position.x = object->position.x;
                packet->last_path_position.z = object->position.z;
            } else {
                wall.x = first_radius;
                NuVecRotateY(&wall, &wall, angle);
                if (local.x < 0.0f) {
                    wall.x = -wall.x;
                }
                NuVecRotateY(&wall, &wall, connection->rotation);
                packet->last_path_position.x = first->position.x + wall.x;
                packet->last_path_position.z = first->position.z + wall.z;
            }
            packet->last_path_position.y = first->position.y;
            packet->inside_path_node = -1;
        }
    }
    return 0;
}

static void AISysResetPathSearchConnectionChecks(AIPATH *path) {
    if (path == NULL || path->nodes == NULL || path->node_count == 0) {
        return;
    }

    ++path->search_checksum;
    if (path->search_checksum == 0) {
        path->search_checksum = 1;
    }

    ++path->search_reset_node;
    if (path->search_reset_node >= path->node_count) {
        path->search_reset_node = 0;
    }

    AIPATHNODE &node = path->nodes[path->search_reset_node];
    for (i32 connection_index = 0; connection_index < node.connection_count; ++connection_index) {
        node.connections[connection_index]->last_search_checksum = 0;
    }
}

static AILOCATOR *AISysLoadFindLocator(AISYS *system, char *name) {
    for (i32 index = 0; index < system->locator_count; ++index) {
        if (NuStrICmp(system->locators[index].name, name) == 0) {
            return &system->locators[index];
        }
    }
    return NULL;
}

static AIAREA *AISysLoadAreaReference(AISYS *system) {
    char name[16];
    if (EdFileReadInt() == 0) {
        return NULL;
    }
    EdFileRead(name, sizeof(name));
    return AISysLoadFindArea(system, name);
}

static AILOCATOR *AISysLoadLocatorReference(AISYS *system) {
    char name[16];
    if (EdFileReadInt() == 0) {
        return NULL;
    }
    EdFileRead(name, sizeof(name));
    return AISysLoadFindLocator(system, name);
}

static void AISysLoadAreas(AISYS *system, i32 version) {
    system->area_count = EdFileReadInt();
    if (system->area_count == 0) {
        return;
    }

    system->areas = static_cast<AIAREA *>(AISysLoadAlloc(system, system->area_count * sizeof(AIAREA)));
    for (i32 index = 0; index < system->area_count; ++index) {
        AIAREA *area = &system->areas[index];
        EdFileRead(area->name, sizeof(area->name));
        area->min_x = EdFileReadFloat();
        area->min_y = EdFileReadFloat();
        area->min_z = EdFileReadFloat();
        area->max_x = EdFileReadFloat();
        area->max_y = EdFileReadFloat();
        area->max_z = EdFileReadFloat();
        area->flags = EdFileReadShort();
        area->system = system;
        if (version > 19) {
            area->game_flags = static_cast<u8>(EdFileReadChar());
        } else {
            EdFileReadChar();
        }
        EdFileReadChar();
    }
}

static void AISysLoadLocators(AISYS *system, i32 version) {
    system->locator_count = EdFileReadInt();
    if (system->locator_count == 0) {
        return;
    }

    system->locators = static_cast<AILOCATOR *>(AISysLoadAlloc(system, system->locator_count * sizeof(AILOCATOR)));
    for (i32 index = 0; index < system->locator_count; ++index) {
        AILOCATOR *locator = &system->locators[index];
        EdFileRead(locator->name, sizeof(locator->name));
        EdFileReadNuVec(&locator->position);
        locator->flags = EdFileReadShort();
        u8 path_index = static_cast<u8>(EdFileReadChar());
        locator->path_info.path = system->path_sys->paths[path_index];
        locator->path_info.direction = static_cast<u8>(EdFileReadChar());
        u16 connection_index = static_cast<u16>(EdFileReadShort());
        locator->path_info.connection = &locator->path_info.path->connections[connection_index];
        locator->path_info.dist = EdFileReadFloat();
        locator->path_info.width = EdFileReadFloat();
        if (version > 14) {
            locator->locator_flags = EdFileReadInt();
        }
    }
}

static void AISysLoadLocatorSets(AISYS *system, i32 version) {
    if (version <= 17) {
        return;
    }

    system->locator_set_count = EdFileReadInt();
    if (system->locator_set_count == 0) {
        return;
    }

    system->locator_sets =
        static_cast<AILOCATORSET *>(AISysLoadAlloc(system, system->locator_set_count * sizeof(AILOCATORSET)));
    for (i32 index = 0; index < system->locator_set_count; ++index) {
        AILOCATORSET *locator_set = &system->locator_sets[index];
        EdFileRead(locator_set->name, sizeof(locator_set->name));
        locator_set->locator_count = static_cast<i8>(EdFileReadInt());
        locator_set->locator_entries =
            static_cast<u8 *>(AISysLoadAlloc(system, locator_set->locator_count * sizeof(AILOCATORSET)));
        for (i32 locator_index = 0; locator_index < locator_set->locator_count; ++locator_index) {
            locator_set->locator_entries[locator_index] = static_cast<u8>(EdFileReadChar());
        }
        locator_set->assigned = static_cast<u8 *>(AISysLoadAlloc(system, locator_set->locator_count));
        memset(locator_set->assigned, 0, locator_set->locator_count);
    }
}

static void AISysLoadCreatures(AISYS *system, i32 version) {
    system->creature_count = EdFileReadInt();
    if (system->creature_count == 0) {
        return;
    }

    system->creatures = static_cast<AICREATURE *>(AISysLoadAlloc(system, system->creature_count * sizeof(AICREATURE)));
    for (i32 index = 0; index < system->creature_count; ++index) {
        AICREATURE *creature = &system->creatures[index];
        EdFileRead(creature->name, sizeof(creature->name));
        EdFileRead(creature->script_name, sizeof(creature->script_name));

        if (creature->script_name[0] == '\0') {
            strcpy(creature->script_name, "default");
        } else if (version > 13) {
            char character_name[32];
            EdFileRead(character_name, sizeof(character_name));
            if (GlobalCharacterTypeIDFn != NULL) {
                creature->type = static_cast<i16>(GlobalCharacterTypeIDFn(character_name));
            }
        }

        EdFileReadNuVec(&creature->pos);
        creature->y_rot = static_cast<NUANG>(EdFileReadShort());

        if (version > 15) {
            creature->set = static_cast<u8>(EdFileReadChar());
            creature->count = static_cast<u8>(EdFileReadChar());
            creature->count_across = static_cast<u8>(EdFileReadChar());
            creature->active_mask = EdFileReadUnsignedInt();
            creature->x_spacing = EdFileReadFloat();
            creature->z_spacing = EdFileReadFloat();
            creature->flags = EdFileReadInt();
            u8 path_index = static_cast<u8>(EdFileReadChar());
            creature->path_info.path = system->path_sys->paths[path_index];
            creature->path_info.direction = static_cast<u8>(EdFileReadChar());
            u16 connection_index = static_cast<u16>(EdFileReadShort());
            creature->path_info.connection = &creature->path_info.path->connections[connection_index];
        }

        if (version > 2) {
            for (i32 param = 0; param < 4; ++param) {
                creature->script_params[param] = EdFileReadFloat();
            }
        }

        if (version != 3) {
            creature->area = AISysLoadAreaReference(system);
        }
        if (version > 5) {
            creature->locator = AISysLoadLocatorReference(system);
        }
        if (version > 16) {
            creature->respawn_locator = AISysLoadLocatorReference(system);
        }

        if (version > 7) {
            creature->activation_difficulty = static_cast<u8>(EdFileReadChar());
            creature->min_respawn_count = EdFileReadChar();
            creature->max_respawn_count = EdFileReadChar();
            creature->activate_type = static_cast<u8>(EdFileReadChar());
            creature->min_respawn_time = EdFileReadFloat();
            creature->max_respawn_time = EdFileReadFloat();
        }
        if (version > 9) {
            creature->start_stagger = EdFileReadFloat();
        }
        if (creature->activate_type == 1) {
            char area_name[16];
            EdFileRead(area_name, sizeof(area_name));
            creature->activate_area = AISysLoadFindArea(system, area_name);
        }
        if (version > 10) {
            creature->view_distance = EdFileReadFloat();
            creature->hear_distance = EdFileReadFloat();
            creature->max_view_height = EdFileReadFloat();
            creature->min_view_height = EdFileReadFloat();
            EdFileReadInt();
        }

        if (creature->view_distance == 0.0f && GetViewRangeFn != NULL) {
            creature->view_distance = GetViewRangeFn(creature->type);
        }
        if (creature->hear_distance == 0.0f && GetHearDistanceFn != NULL) {
            creature->hear_distance = GetHearDistanceFn(creature->type);
        }
        if (creature->max_view_height == 0.0f && GetMaxViewHeightFn != NULL) {
            creature->max_view_height = GetMaxViewHeightFn(creature->type);
        }
        if (creature->min_view_height == 0.0f && GetMinViewHeightFn != NULL) {
            creature->min_view_height = GetMinViewHeightFn(creature->type);
        }
    }
}

static void AISysLoadAntinodes(AISYS *system, i32 version, NUGSCN *scene) {
    system->antinode_count = EdFileReadInt();
    if (system->antinode_count == 0) {
        return;
    }

    system->antinodes = static_cast<AIANTINODE *>(AISysLoadAlloc(system, system->antinode_count * sizeof(AIANTINODE)));
    for (i32 index = 0; index < system->antinode_count; ++index) {
        AIANTINODE *antinode = &system->antinodes[index];
        antinode->enabled = 1;
        antinode->position.x = EdFileReadFloat();
        antinode->position.y = EdFileReadFloat();
        antinode->position.z = EdFileReadFloat();
        antinode->radius = EdFileReadFloat();
        antinode->base_radius = antinode->radius;
        antinode->base_height = antinode->radius;
        antinode->min_y = EdFileReadFloat();
        antinode->max_y = EdFileReadFloat();
        antinode->min_y_offset = antinode->min_y - antinode->position.y;
        antinode->max_y_offset = antinode->max_y - antinode->position.y;

        if (version > 14) {
            antinode->flags = EdFileReadInt();
            antinode->base_radius = EdFileReadFloat();
            antinode->base_height = EdFileReadFloat();
            antinode->type = static_cast<u8>(EdFileReadChar());
            EdFileReadChar();
            EdFileReadChar();
        } else {
            EdFileReadChar();
            EdFileReadChar();
            EdFileReadChar();
        }
        antinode->game_flags = static_cast<u8>(EdFileReadChar());

        char special_name[256];
        i32 special_name_length = EdFileReadChar();
        if (special_name_length != 0) {
            EdFileRead(special_name, special_name_length);
            antinode->has_special =
                static_cast<u8>(NuSpecialFind(scene, &antinode->special_handle, special_name, 1) != 0);
            antinode->special_position.x = EdFileReadFloat();
            antinode->special_position.y = EdFileReadFloat();
            antinode->special_position.z = EdFileReadFloat();
            if (version > 14) {
                antinode->rotation_offset = EdFileReadInt();
            }
        }
    }
}

extern "C" {

    AIPATHNODE *AIPathFindNode(AISYS *system, AIPATH *path, char *name);

    void AIMoveInstruction(AIPACKET *packet, NUVEC *destination, f32 stopping_distance, AIPATHINFO *path_info, i32 mode,
                           f32 movement_parameter) {
        AIGROUP *group = packet->group;
        if (group != NULL && group->is_in_formation) {
            switch (mode) {
                case AIPACKET_MOVEMENT_WANDER:
                    if (group->leader == reinterpret_cast<APIOBJECT *>(packet->owner))
                        FormationMove(group, RowMoveWander);
                    AIFormationFollow(packet);
                    return;
                case AIPACKET_MOVEMENT_FORMATION:
                    mode = AIPACKET_MOVEMENT_TO_DESTINATION;
                    break;
                case AIPACKET_MOVEMENT_TO_DESTINATION:
                    if (group->leader == reinterpret_cast<APIOBJECT *>(packet->owner))
                        FormationMove(group, RowMoveTowards);
                    AIFormationFollow(packet);
                    return;
            }
        }

        if (destination != NULL) {
            packet->fallback_destination = *destination;
        }
        if (path_info != NULL) {
            packet->fallback_path_info = *path_info;
        }
        packet->fallback_stopping_distance = stopping_distance;
        packet->movement_flags = (packet->movement_flags & static_cast<u8>(~AIPACKET_MOVEMENT_MODE_MASK)) |
                                 (mode & AIPACKET_MOVEMENT_MODE_MASK);
        packet->movement_parameter = movement_parameter;
    }

    void *AIPAthFindPathCnx(AISYS_s *system, AIPATH_s *path, char *from_name, char *to_name, i32 *direction) {
        if (path == NULL) {
            if (system == NULL || system->path_sys == NULL || system->path_sys->path_count == 0) {
                return NULL;
            }
            path = system->path_sys->active_path;
        }
        if (path == NULL) {
            return NULL;
        }

        AIPATHNODE *from = AIPathFindNode(system, path, from_name);
        AIPATHNODE *to = AIPathFindNode(system, path, to_name);
        if (to == NULL || from == NULL || from == to) {
            return NULL;
        }

        const i32 to_index = to - path->nodes;
        for (i32 index = 0; index < from->connection_count; ++index) {
            AIPATHCNX *connection = from->connections[index];
            if (connection->node_indices[0] == to_index) {
                *direction = 1;
                return connection;
            }
            if (connection->node_indices[1] == to_index) {
                *direction = 0;
                return connection;
            }
        }
        return NULL;
    }

    AILOCATOR *AIPathFindLocator(AISYS *sys, char *name) {
        if (sys != NULL) {
            for (i32 i = 0; i < sys->locator_count; ++i) {
                if (NuStrICmp(sys->locators[i].name, name) == 0) {
                    return &sys->locators[i];
                }
            }
        }
        return NULL;
    }

    AILOCATORSET *AIPathFindLocatorSet(AISYS *system, char *name) {
        if (system != NULL) {
            for (i32 index = 0; index < system->locator_set_count; ++index) {
                if (NuStrICmp(system->locator_sets[index].name, name) == 0) {
                    return &system->locator_sets[index];
                }
            }
        }
        return NULL;
    }

    AIPATHNODE *AIPathFindNode(AISYS *system, AIPATH *path, char *name) {
        if (path == NULL) {
            if (system == NULL || system->path_sys == NULL || system->path_sys->path_count == 0) {
                return NULL;
            }
            path = system->path_sys->active_path;
            if (path == NULL) {
                return NULL;
            }
        }

        for (i32 index = 0; index < path->node_count; ++index) {
            if (path->nodes[index].name != NULL && NuStrICmp(path->nodes[index].name, name) == 0) {
                return &path->nodes[index];
            }
        }
        return NULL;
    }

    void AIPathNodeUpdatePos(AISYS *system, AIPATH *path, AIPATHNODE *node) {
        if (node->has_special == 0) {
            return;
        }

        const i32 node_index = static_cast<i32>(node - path->nodes);
        const u8 node_bit = static_cast<u8>(1u << (node_index % 8));
        u8 &updated_nodes = path->updated_node_bits[node_index / 8];
        if ((updated_nodes & node_bit) != 0) {
            return;
        }
        updated_nodes |= node_bit;

        nuhspecial_s *special = &node->special_handle;
        if ((node->runtime_flags & AIPATHNODE_RUNTIME_SPECIAL_UNAVAILABLE) != 0) {
            if (NuSpecialGetVisibilityFn(special) == 0) {
                return;
            }

            node->runtime_flags &= static_cast<u8>(~AIPATHNODE_RUNTIME_SPECIAL_UNAVAILABLE);
            for (i32 index = 0; index < node->connection_count; ++index) {
                AIPATHCNX *connection = node->connections[index];
                const u8 other_index =
                    connection->direction_a == node_index ? connection->direction_b : connection->direction_a;
                if ((path->nodes[other_index].runtime_flags & AIPATHNODE_RUNTIME_SPECIAL_UNAVAILABLE) == 0) {
                    connection->traversal_flags[0] &= ~AIPATH_CONNECTION_FLAG_SPECIAL_UNAVAILABLE;
                    connection->traversal_flags[1] &= ~AIPATH_CONNECTION_FLAG_SPECIAL_UNAVAILABLE;
                }
            }
        } else {
            if (NuSpecialGetVisibilityFn(special) == 0 &&
                (FindAlternativeSpecialObjectFn == NULL || FindAlternativeSpecialObjectFn(system, special) == 0)) {
                node->runtime_flags |= AIPATHNODE_RUNTIME_SPECIAL_UNAVAILABLE;
                for (i32 index = 0; index < node->connection_count; ++index) {
                    AIPATHCNX *connection = node->connections[index];
                    connection->traversal_flags[0] |= AIPATH_CONNECTION_FLAG_SPECIAL_UNAVAILABLE;
                    connection->traversal_flags[1] |= AIPATH_CONNECTION_FLAG_SPECIAL_UNAVAILABLE;
                }
                return;
            }
        }

        const NUVEC previous_position = node->position;
        NuVecMtxTransform(&node->position, &node->special_position, NuSpecialGetDrawMtx(special));
        node->min_height = node->position.y + node->min_height_offset;
        node->max_height = node->position.y + node->max_height_offset;

        for (i32 index = 0; index < node->connection_count; ++index) {
            AIPATHCNX *connection = node->connections[index];
            AIPATHNODE *other_node;
            NUVEC *from;
            NUVEC *to;
            if (connection->direction_a == node_index) {
                other_node = &path->nodes[connection->direction_b];
                from = &other_node->position;
                to = &node->position;
            } else {
                other_node = &path->nodes[connection->direction_a];
                from = &node->position;
                to = &other_node->position;
            }

            NUVEC direction;
            connection->horizontal_distance = NuVecXZDist(from, to, &direction);
            connection->rotation = static_cast<i16>(NuAtan2(direction.x, direction.z) * 10430.378f);

            if (connection->max_horizontal_distance != 0.0f) {
                if (connection->max_horizontal_distance + node->radius + other_node->radius <
                    connection->horizontal_distance) {
                    connection->traversal_flags[0] |= AIPATH_CONNECTION_FLAG_DYNAMIC_TOO_LONG;
                    connection->traversal_flags[1] |= AIPATH_CONNECTION_FLAG_DYNAMIC_TOO_LONG;
                } else {
                    connection->traversal_flags[0] &= ~AIPATH_CONNECTION_FLAG_DYNAMIC_TOO_LONG;
                    connection->traversal_flags[1] &= ~AIPATH_CONNECTION_FLAG_DYNAMIC_TOO_LONG;
                }
            }
        }

        nuinstanim_s *animation;
        if (previous_position.x != node->position.x || previous_position.y != node->position.y ||
            previous_position.z != node->position.z ||
            ((animation = NuSpecialGetInstAnim(special)) != NULL && (animation->flags & NUINSTANIM_FLAG_PLAYING) != 0 &&
             animation->tfactor != 0.0f)) {
            node->runtime_flags |= AIPATHNODE_RUNTIME_POSITION_CHANGED;
        } else {
            node->runtime_flags &= static_cast<u8>(~AIPATHNODE_RUNTIME_POSITION_CHANGED);
        }
    }

    void AISYSRebuildFromEditorData(void) {
    }

    void AIScriptForceParamReEval(AISCRIPTPROCESS *processor) {
        if (processor != NULL) {
            processor->param_stack[0].is_first_time_state = 1;
            processor->param_stack[1].is_first_time_state = 1;
        }
    }

    char *AIScriptNameFromIx(AISYS *system, i32 index) {
        i32 current_index = 0;
        if (system != NULL) {
            AISCRIPT *script = reinterpret_cast<AISCRIPT *>(NuLinkedListGetHead(&system->scripts));
            while (script != NULL) {
                if (current_index == index) {
                    return script->name;
                }
                script = reinterpret_cast<AISCRIPT *>(NuLinkedListGetNext(&system->scripts, &script->list_node));
                ++current_index;
            }
        }

        AISCRIPT *script = reinterpret_cast<AISCRIPT *>(NuLinkedListGetHead(&global_aiscripts));
        while (script != NULL) {
            AISCRIPT *level_script = NULL;
            if (system != NULL) {
                level_script = reinterpret_cast<AISCRIPT *>(NuLinkedListGetHead(&system->scripts));
                while (level_script != NULL) {
                    if (NuStrICmp(script->name, level_script->name) == 0) {
                        break;
                    }
                    level_script =
                        reinterpret_cast<AISCRIPT *>(NuLinkedListGetNext(&system->scripts, &level_script->list_node));
                }
            }
            if (level_script == NULL) {
                if (current_index == index) {
                    return script->name;
                }
                ++current_index;
            }
            script = reinterpret_cast<AISCRIPT *>(NuLinkedListGetNext(&global_aiscripts, &script->list_node));
        }
        return NULL;
    }

} // extern "C"

static AICONDITION *AIScriptFinishStateConditions(NULISTHDR *conditions, AIPACKET *packet, AISCRIPTPROCESS *processor,
                                                  AICONDITION *condition, i32 stack_index) {
    if (processor->param_stack[stack_index].force_complex_eval) {
        if (processor->param_stack[stack_index].is_first_time_state) {
            AICONDITION *remaining = reinterpret_cast<AICONDITION *>(
                NuLinkedListGetNext(conditions, reinterpret_cast<NULISTLNK *>(condition)));
            while (remaining != NULL) {
                if (remaining->is_complex) {
                    processor->param_stack[stack_index].complex_params[remaining->param_idx] =
                        AIParamToFloatEx(packet, processor, remaining->complex_arg);
                }
                remaining = reinterpret_cast<AICONDITION *>(NuLinkedListGetNext(conditions, &remaining->list_node));
            }
        }
        processor->param_stack[stack_index].force_complex_eval = 0;
    }
    processor->param_stack[stack_index].is_first_time_state = 0;
    return condition;
}

static AICONDITION *AIScriptProcessStateConditions(AISYS *system, NULISTHDR *conditions, APIOBJECT *, AIPACKET *packet,
                                                   AISCRIPTPROCESS *processor, f32, i32 stack_index) {
    if (processor == NULL || conditions == NULL) {
        return NULL;
    }

    AICONDITION *condition = reinterpret_cast<AICONDITION *>(NuLinkedListGetHead(conditions));
    f32 condition_value = 0.0f;

    while (condition != NULL) {
        i32 comparison_passed = 0;
        if (condition->def != NULL) {
            if (condition->def->eval_fn != NULL) {
                condition_value =
                    condition->def->eval_fn(system, processor, packet, condition->arg, condition->void_arg);
            }

            f32 comparison_value;
            if (condition->param_cond != NULL && condition->param_cond->def != NULL) {
                AICONDITIONFN *comparison_fn = condition->param_cond->def->eval_fn;
                comparison_value = comparison_fn != NULL
                                       ? comparison_fn(system, processor, packet, condition->arg, condition->void_arg)
                                       : condition_value;
            } else if (condition->is_complex) {
                if (processor->param_stack[stack_index].is_first_time_state) {
                    processor->param_stack[stack_index].complex_params[condition->param_idx] =
                        AIParamToFloatEx(packet, processor, condition->complex_arg);
                }
                comparison_value = processor->param_stack[stack_index].complex_params[condition->param_idx];
            } else if (condition->is_param_idx_valid) {
                comparison_value = processor->params[condition->param_idx];
            } else {
                comparison_value = condition->param_val;
            }

            switch (condition->type) {
                case AICONDITION_EQUAL:
                    if (condition_value == comparison_value) {
                        comparison_passed = 1;
                    }
                    break;
                case AICONDITION_LESS_THAN:
                    if (condition_value < comparison_value) {
                        comparison_passed = 1;
                    }
                    break;
                case AICONDITION_GREATER_THAN:
                    if (condition_value > comparison_value) {
                        comparison_passed = 1;
                    }
                    break;
                case AICONDITION_LESS_THAN_OR_EQUAL:
                    if (condition_value <= comparison_value) {
                        comparison_passed = 1;
                    }
                    break;
                case AICONDITION_GREATER_THAN_OR_EQUAL:
                    if (condition_value >= comparison_value) {
                        comparison_passed = 1;
                    }
                    break;
                case AICONDITION_NOT_EQUAL:
                    if (condition_value != comparison_value) {
                        comparison_passed = 1;
                    }
                    break;
            }
        }

        if (!comparison_passed) {
            // A failed member invalidates the rest of its AND group.  The
            // first-state pass still snapshots complex operands so later
            // frames compare against the same values as the target.
            while (condition->bool_and) {
                condition = reinterpret_cast<AICONDITION *>(NuLinkedListGetNext(conditions, &condition->list_node));
                if (condition == NULL) {
                    break;
                }
                if (processor->param_stack[stack_index].is_first_time_state && condition->is_complex) {
                    processor->param_stack[stack_index].complex_params[condition->param_idx] =
                        AIParamToFloatEx(packet, processor, condition->complex_arg);
                    condition->param_val = processor->param_stack[stack_index].complex_params[condition->param_idx];
                }
            }
        } else if (!condition->bool_and) {
            return AIScriptFinishStateConditions(conditions, packet, processor, condition, stack_index);
        }

        if (condition != NULL) {
            condition = reinterpret_cast<AICONDITION *>(NuLinkedListGetNext(conditions, &condition->list_node));
        }
    }

    return AIScriptFinishStateConditions(conditions, packet, processor, NULL, stack_index);
}

extern "C" {

    void AIScriptProcess(AISYS *system, APIOBJECT *object, AIPACKET *packet, AISCRIPTPROCESS *processor, f32 elapsed) {
        if (processor == NULL || processor->script == NULL) {
            return;
        }

        processor->script_timer += elapsed;

        for (i32 index = 0; index < processor->active_ref_count; ++index) {
            AIREFSCRIPT *reference = processor->active_refs[index];
            if (NuLinkedListGetHead(&reference->conditions) != NULL &&
                AIScriptProcessStateConditions(system, &reference->conditions, object, packet, processor, elapsed, 1) ==
                    NULL) {
                processor->active_ref_count = index;
                if (index == 0) {
                    AICREATURE *creature = packet->field_0x134 != 0xff ? &system->creatures[packet->field_0x134] : NULL;
                    AIScriptProcessorInit(system, packet, processor, creature, NULL, NULL, 0, processor->base_script,
                                          reference->return_state);
                } else {
                    AIScriptProcessorInit(system, packet, processor, NULL, NULL, NULL, 0,
                                          processor->active_refs[index - 1]->script, reference->return_state);
                }
                break;
            }
        }

        if (processor->next_state == NULL) {
            if (processor->active_ref_count != 0 && processor->state == processor->script->base_state) {
                const i32 completed_index = processor->active_ref_count - 1;
                AIREFSCRIPT *completed_reference = processor->active_refs[completed_index];
                processor->active_ref_count = completed_index;
                if (completed_index == 0) {
                    AICREATURE *creature = packet->field_0x134 != 0xff ? &system->creatures[packet->field_0x134] : NULL;
                    AIScriptProcessorInit(system, packet, processor, creature, NULL, NULL, 0, processor->base_script,
                                          completed_reference->return_state);
                } else {
                    AIScriptProcessorInit(system, packet, processor, NULL, NULL, NULL, 0,
                                          processor->active_refs[completed_index - 1]->script,
                                          completed_reference->return_state);
                }
            }
        }
        if (processor->next_state != NULL) {
            AIScriptSetState(processor, processor->next_state);
            processor->next_state = NULL;
        }

        if (processor->state == NULL) {
            AIScriptSetState(processor, reinterpret_cast<AISTATE *>(NuLinkedListGetHead(&processor->script->states)));
        }

        if (processor->state != NULL) {
            while (processor->action_node != NULL) {
                AIACTION *action = reinterpret_cast<AIACTION *>(processor->action_node);
                bool action_completed = true;
                if (action->def != NULL && action->def->eval_fn != NULL) {
                    i32 first_time = 0;
                    if (processor->is_first_time_action) {
                        processor->action_data_1 = 0;
                        processor->action_data_2 = 0;
                        processor->action_data_3 = NULL;
                        processor->action_data_6 = 0;
                        processor->action_data_4 = 0.0f;
                        processor->action_data_5 = 0.0f;
                        processor->action_data_7 = 0;
                        processor->action_timer = 0.0f;
                        processor->action_pos.x = processor->action_pos.y = processor->action_pos.z = 0.0f;
                        memset(&processor->path_info, 0, sizeof(processor->path_info));

                        if (packet != NULL) {
                            packet->goal_path_node = NULL;
                            packet->goal_speed_mode = 0;
                            packet->movement_instruction_parameter = 0.0f;
                            packet->frame_flags = 0;
                            if (ScriptProcessFirstTimeActionFn != NULL) {
                                ScriptProcessFirstTimeActionFn(system, packet, processor);
                            }
                        }

                        processor->is_first_time_action = 0;
                        first_time = 1;
                    }

                    LOG_INFO_IF(
                        packet == NULL && first_time,
                        "level-script action processor=%p script=%s state=%s action=%s params=%d [%s, %s, %s, %s]",
                        (void *)processor, processor->script->name, processor->state->name, action->def->name,
                        action->param_count, action->param_count > 0 ? action->params[0] : "",
                        action->param_count > 1 ? action->params[1] : "",
                        action->param_count > 2 ? action->params[2] : "",
                        action->param_count > 3 ? action->params[3] : "");
                    action_completed = action->def->eval_fn(system, processor, packet, action->params,
                                                            action->param_count, first_time, elapsed) != 0;
                    LOG_INFO_IF(packet == NULL && action_completed,
                                "level-script action complete processor=%p action=%s", (void *)processor,
                                action->def->name);
                    if (!action_completed) {
                        break;
                    }
                } else {
                    processor->is_first_time_action = 0;
                }

                NULISTLNK *action_node = processor->action_node;
                if (processor->is_first_time_action) {
                    continue;
                }
                if (action_node != NULL) {
                    action_node = NuLinkedListGetNext(&processor->state->actions, action_node);
                } else {
                    action_node = NuLinkedListGetHead(&processor->state->actions);
                }
                if (action_node != NULL) {
                    processor->is_first_time_action = 1;
                }
                processor->action_node = action_node;
            }
        }

        if (system != NULL && processor->script != NULL && processor->active_ref_count < 4) {
            NULISTHDR *references = &processor->script->ref_scripts;
            for (AIREFSCRIPT *reference = reinterpret_cast<AIREFSCRIPT *>(NuLinkedListGetHead(references));
                 reference != NULL; reference = reinterpret_cast<AIREFSCRIPT *>(
                                        NuLinkedListGetNext(&processor->script->ref_scripts, &reference->list_node))) {
                processor->param_stack[1].is_first_time_state = 1;
                processor->param_stack[1].force_complex_eval = 1;

                if (NuLinkedListGetHead(&reference->conditions) != NULL &&
                    AIScriptProcessStateConditions(system, &reference->conditions, object, packet, processor, elapsed,
                                                   1) == NULL) {
                    continue;
                }
                if (reference->script == NULL) {
                    continue;
                }

                processor->param_stack[0].is_first_time_state = 1;
                processor->param_stack[0].force_complex_eval = 1;
                AICONDITION *entry_condition = AIScriptProcessStateConditions(
                    system, &reference->script->base_state->conditions, object, packet, processor, elapsed, 0);
                if (entry_condition != NULL && entry_condition->next_state != reference->script->base_state) {
                    const i32 reference_index = processor->active_ref_count;
                    processor->active_refs[reference_index] = reference;
                    processor->active_ref_count = reference_index + 1;
                    AIScriptProcessorInit(system, packet, processor, NULL, NULL, NULL, 0, reference->script,
                                          entry_condition->next_state);
                    return;
                }
            }

            if (processor->state != NULL) {
                references = &processor->state->ref_scripts;
                for (AIREFSCRIPT *reference = reinterpret_cast<AIREFSCRIPT *>(NuLinkedListGetHead(references));
                     reference != NULL; reference = reinterpret_cast<AIREFSCRIPT *>(
                                            NuLinkedListGetNext(references, &reference->list_node))) {
                    processor->param_stack[1].is_first_time_state = 1;
                    processor->param_stack[1].force_complex_eval = 1;

                    if (NuLinkedListGetHead(&reference->conditions) != NULL &&
                        AIScriptProcessStateConditions(system, &reference->conditions, object, packet, processor,
                                                       elapsed, 1) == NULL) {
                        continue;
                    }
                    if (reference->script == NULL) {
                        continue;
                    }

                    processor->param_stack[0].is_first_time_state = 1;
                    processor->param_stack[0].force_complex_eval = 1;
                    AICONDITION *entry_condition = AIScriptProcessStateConditions(
                        system, &reference->script->base_state->conditions, object, packet, processor, elapsed, 0);
                    if (entry_condition != NULL && entry_condition->next_state != reference->script->base_state) {
                        const i32 reference_index = processor->active_ref_count;
                        processor->active_refs[reference_index] = reference;
                        processor->active_ref_count = reference_index + 1;
                        AIScriptProcessorInit(system, packet, processor, NULL, NULL, NULL, 0, reference->script,
                                              entry_condition->next_state);
                        return;
                    }
                }
            }
        }

        if (processor->interrupt_timer > 0.0f) {
            processor->interrupt_timer -= elapsed;
            if (processor->interrupt_timer <= 0.0f && processor->interrupt_state != NULL) {
                AIScriptSetState(processor, processor->interrupt_state);
                processor->interrupt_priority = 0;
                processor->interrupt_state = NULL;
                return;
            }
        }

        if (processor->state != NULL) {
            AICONDITION *transition = AIScriptProcessStateConditions(system, &processor->state->conditions, object,
                                                                     packet, processor, elapsed, 0);
            if (transition != NULL) {
                processor->unknown_flag_4 = transition->keep_blocked;
                AIScriptSetState(processor, transition->next_state);
            }
        }
    }

    void AIScriptSetLevelPath(char *path) {
        AiLevelPathName = path;
    }

    // libTTapp.so 0x3e5fd0: carve a zeroed, 16-byte-aligned block from the
    // permbuffer cursor. Returns NULL when the cursor or the end pointer is
    // missing or the buffer has less room than requested.
    void *AISysBufferAlloc(VARIPTR *cursor, VARIPTR *buf_end, u32 size) {
        void *block = NULL;
        if (cursor != NULL && buf_end != NULL) {
            const u32 offset = (u32)cursor->addr;
            if (buf_end->addr > (usize)(offset + size)) {
                block = (void *)(usize)((offset + 0xf) & ~0xfu);
                cursor->addr = (usize)block + size;
                memset(block, 0, size);
            }
        }
        return block;
    }

    void AISysCharacterMovement(AISYS *system, AIPACKET *packet, APIOBJECT *object, i32 checks) {
        packet->movement_event_flags &= static_cast<u8>(~AIPACKET_PATH_CONNECTION_CHANGED);
        if (MidSpecialMoveFn == NULL || MidSpecialMoveFn(system, packet, object) == 0) {
            if (packet->path_info.path != NULL && packet->path_info.connection != NULL) {
                packet->movement_destination = object->position;
                packet->movement_stopping_distance = 0.0f;
                switch (packet->movement_flags & AIPACKET_MOVEMENT_MODE_MASK) {
                    case AIPACKET_MOVEMENT_TO_DESTINATION:
                        AIMoveToDestination(system, packet, object, checks);
                        break;
                    case AIPACKET_MOVEMENT_RETREAT:
                        AIRetreatFromDestination(system, packet, object, checks);
                        break;
                    case AIPACKET_MOVEMENT_CIRCLE:
                        AICircle(system, packet, object, checks);
                        break;
                    case AIPACKET_MOVEMENT_WANDER:
                        AIWander(system, packet, object, checks);
                        break;
                    case AIPACKET_MOVEMENT_AVOIDING_CAMERA:
                        AIMoveToDestinationAvoidingCamera(system, packet, object, checks);
                        break;
                    case AIPACKET_MOVEMENT_DIRECT:
                        AIMoveDirectlyToDestination(system, packet, object, checks);
                        break;
                }
            } else {
                packet->movement_destination = object->position;
                packet->movement_stopping_distance = 0.0f;
                const u8 mode = packet->movement_flags & AIPACKET_MOVEMENT_MODE_MASK;
                if (mode == AIPACKET_MOVEMENT_DIRECT || mode == AIPACKET_MOVEMENT_TO_DESTINATION ||
                    mode == AIPACKET_MOVEMENT_AVOIDING_CAMERA) {
                    packet->movement_stopping_distance = packet->fallback_stopping_distance;
                    packet->movement_destination = packet->fallback_destination;
                }
            }

            packet->movement_flags &= static_cast<u8>(~AIPACKET_MOVEMENT_MODE_MASK);
        } else {
            packet->field_0x1e6 |= AIPACKET_RUNTIME_SPECIAL_MOVE;
            packet->field_0x1e7 |= AIPACKET_MOVEMENT_SPECIAL_HANDLED;
        }
    }

    void AISysCreatureAntinodeInteraction(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *) {
        if (system != NULL) {
            AIANTINODE *active[192];
            i32 count = 0;
            AIANTINODE *antinode = system->antinodes;
            for (i32 index = 0; index < system->antinode_count && count < 192; ++index, ++antinode) {
                if (antinode->enabled &&
                    (!antinode->has_special || NuSpecialGetVisibilityFn(&antinode->special_handle))) {
                    active[count++] = antinode;
                }
            }
            antinode = dynamic_antinodes;
            for (i32 index = 0; count < 192 && index < 64; ++index, ++antinode) {
                if (antinode->enabled &&
                    (!antinode->has_special || NuSpecialGetVisibilityFn(&antinode->special_handle))) {
                    active[count++] = antinode;
                }
            }
            if (count != 0) {
                for (i32 index = 0; index < object_count; ++index) {
                    APIOBJECT *object = objects[index];
                    if ((object->object_flags & 0x20080) != 0) {
                        continue;
                    }
                    for (i32 obstacle = 0; obstacle < count; ++obstacle) {
                        antinode = active[obstacle];
                        if (NuSpecialExistsFn(&object->collision_special) &&
                            NuSpecialCompare(&object->collision_special, &antinode->special_handle)) {
                            continue;
                        }
                        if ((antinode->excluded_character_types >> object->field_0x289) & 1) {
                            continue;
                        }
                        NUVEC difference;
                        f32 radius = object->ai->mover_height + antinode->radius;
                        difference.x = object->ai->movement_position.x - antinode->position.x;
                        if (difference.x > radius || difference.x < -radius) {
                            continue;
                        }
                        difference.z = object->ai->movement_position.z - antinode->position.z;
                        if (difference.z > radius || difference.z < -radius ||
                            object->collision_min.y > antinode->max_y) {
                            continue;
                        }
                        if (antinode->min_y <= object->collision_max.y) {
                            checkantinodefns[antinode->type](object, antinode, &difference, radius);
                        }
                    }
                }
            }
        }
        AIAntinodeCullSingleFrame();
    }

    void AISysCreatureInteraction2D(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable,
                                    f32 delta_time) {
        for (i32 index = 0; index < object_count; ++index) {
            f32 timer = objects[index]->ai->antinode_timer - delta_time;
            objects[index]->ai->antinode_timer = timer < 0.0f ? 0.0f : timer;
        }
        for (i32 first_index = 0; first_index < object_count - 1; ++first_index) {
            APIOBJECT *first = objects[first_index];
            for (i32 second_index = first_index + 1; second_index < object_count; ++second_index) {
                APIOBJECT *second = objects[second_index];
                AIPACKET *first_packet = first->ai;
                AIPACKET *second_packet = second->ai;
                f32 first_radius = first_packet->mover_height;
                f32 second_radius = second_packet->mover_height;
                i32 first_fixed;
                i32 second_fixed;
                if (first->collision_link == second || second->collision_link == first) {
                    first_fixed = second_fixed = 1;
                } else {
                    first_fixed = immovable[first_index];
                    second_fixed = immovable[second_index];
                }
                bool fixed_first = (first->flags_low & 0x80) != 0 || first_fixed != 0;
                bool fixed_second = (second->flags_low & 0x80) != 0 || second_fixed != 0;
                if (fixed_first && fixed_second) {
                    continue;
                }
                f32 radius = first_radius + second_radius;
                NUVEC difference;
                difference.x = first_packet->movement_position.x - second_packet->movement_position.x;
                if (difference.x > radius || difference.x < -radius) {
                    continue;
                }
                difference.z = first_packet->movement_position.z - second_packet->movement_position.z;
                if (difference.z > radius || difference.z < -radius) {
                    continue;
                }
                if (first->collision_min.y > second->collision_max.y ||
                    second->collision_min.y > first->collision_max.y) {
                    continue;
                }
                f32 distance = NuFsqrt(difference.x * difference.x + difference.z * difference.z);
                if (distance < radius) {
                    difference.y = 0.0f;
                    f32 scale = (radius - distance) / distance;
                    if (fixed_first) {
                        NuVecScale(&difference, &difference, scale);
                        NuVecSub(&second->ai->movement_position, &second->ai->movement_position, &difference);
                    } else if (fixed_second) {
                        NuVecScale(&difference, &difference, scale);
                        NuVecAdd(&first->ai->movement_position, &first->ai->movement_position, &difference);
                    } else {
                        NuVecScale(&difference, &difference, scale);
                        NuVecSub(&second->ai->movement_position, &second->ai->movement_position, &difference);
                    }
                    AIPACKET *first_ai = first->ai;
                    AIPACKET *second_ai = second->ai;
                    f32 first_timer = first_ai->antinode_timer;
                    f32 second_timer = second_ai->antinode_timer;
                    if (first_timer > antinode_time && second_timer > antinode_time) {
                        if (first_timer > second_timer) {
                            second_ai->antinode_timer = first_timer;
                            second_ai->antinode_clockwise = first_ai->antinode_clockwise;
                        } else {
                            first_ai->antinode_timer = second_timer;
                            first_ai->antinode_clockwise = second_ai->antinode_clockwise;
                        }
                    } else if (second_timer > antinode_time) {
                        first_ai->antinode_timer = second_timer;
                        first_ai->antinode_clockwise = second_ai->antinode_clockwise;
                    } else if (first_timer > 0.0f) {
                        second_ai->antinode_timer = first_timer;
                        second_ai->antinode_clockwise = first_ai->antinode_clockwise;
                    }
                    if (((first->field_0x1f4 ^ second->field_0x1f4) & 1) != 0) {
                        first->ai->field_0x1e5 |= 0x40;
                        second->ai->field_0x1e5 |= 0x40;
                    }
                }
            }
        }
        AISysCreatureAntinodeInteraction(system, object_count, objects, immovable);
    }

    void AISysCreatureInteraction3D(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable,
                                    f32 delta_time) {
        for (i32 index = 0; index < object_count; ++index) {
            f32 timer = objects[index]->ai->antinode_timer - delta_time;
            objects[index]->ai->antinode_timer = timer < 0.0f ? 0.0f : timer;
        }
        for (i32 first_index = 0; first_index < object_count - 1; ++first_index) {
            APIOBJECT *first = objects[first_index];
            for (i32 second_index = first_index + 1; second_index < object_count; ++second_index) {
                APIOBJECT *second = objects[second_index];
                AIPACKET *first_packet = first->ai;
                AIPACKET *second_packet = second->ai;
                f32 first_radius = first_packet->mover_height;
                f32 second_radius = second_packet->mover_height;
                i32 first_fixed;
                i32 second_fixed;
                if (first->collision_link == second || second->collision_link == first) {
                    first_fixed = second_fixed = 1;
                } else {
                    first_fixed = immovable[first_index];
                    second_fixed = immovable[second_index];
                }
                bool fixed_first = (first->flags_low & 0x80) != 0 || first_fixed != 0;
                bool fixed_second = (second->flags_low & 0x80) != 0 || second_fixed != 0;
                if (fixed_first && fixed_second) {
                    continue;
                }
                f32 radius = first_radius + second_radius;
                NUVEC difference;
                difference.x = first_packet->movement_position.x - second_packet->movement_position.x;
                if (difference.x > radius || difference.x < -radius) {
                    continue;
                }
                difference.z = first_packet->movement_position.z - second_packet->movement_position.z;
                if (difference.z > radius || difference.z < -radius) {
                    continue;
                }
                difference.y = first_packet->movement_position.y - second_packet->movement_position.y;
                if (difference.y > radius || difference.y < -radius) {
                    continue;
                }
                f32 distance =
                    NuFsqrt(difference.x * difference.x + difference.y * difference.y + difference.z * difference.z);
                if (distance < radius) {
                    f32 scale = (radius - distance) / distance;
                    if (fixed_first) {
                        NuVecScale(&difference, &difference, scale);
                        NuVecSub(&second->ai->movement_position, &second->ai->movement_position, &difference);
                    } else if (fixed_second) {
                        NuVecScale(&difference, &difference, scale);
                        NuVecAdd(&first->ai->movement_position, &first->ai->movement_position, &difference);
                    } else {
                        NuVecScale(&difference, &difference, scale);
                        NuVecSub(&second->ai->movement_position, &second->ai->movement_position, &difference);
                    }
                    if (((first->field_0x1f4 ^ second->field_0x1f4) & 1) != 0) {
                        first->ai->field_0x1e5 |= 0x40;
                        second->ai->field_0x1e5 |= 0x40;
                    }
                }
            }
        }
        AISysCreatureAntinodeInteraction(system, object_count, objects, immovable);
    }

    AIAREA *AISysFindArea(AISYS *sys, char *name) {
        if (sys != NULL) {
            for (i32 i = 0; i < sys->area_count; ++i) {
                if (NuStrICmp(sys->areas[i].name, name) == 0) {
                    return &sys->areas[i];
                }
            }
        }
        return NULL;
    }

    AIPATH *AISysFindPath(AISYS *sys, char *name) {
        if (sys != NULL && sys->path_sys != NULL) {
            for (i32 i = 0; i < sys->path_sys->path_count; ++i) {
                AIPATH *path = sys->path_sys->paths[i];
                if (NuStrICmp(path->name, name) == 0) {
                    return path;
                }
            }
        }
        return NULL;
    }

    void AISysGetCharacterPathPos(AISYS *system, APIOBJECT *object, AIPACKET *packet, i32 checks, i32 ground) {
        f32 nearest_distance_squared = 3.402823466e+38f;

        const u8 previous_flags = packet->path_info.flags;
        packet->path_info.flags =
            (previous_flags &
             static_cast<u8>(~(AIPATHINFO_FLAG_ON_PATH | AIPATHINFO_FLAG_WAS_ON_PATH | AIPATHINFO_FLAG_NARROW_PATH))) |
            ((previous_flags & AIPATHINFO_FLAG_ON_PATH) << 1);

        if (system == NULL || system->path_sys == NULL || system->path_sys->path_count == 0) {
            AISysCharacterSetPath(packet, NULL);
            AISysCharacterSetPathCnx(packet, &object->position, NULL, 0);
            return;
        }

        AIPATH *path = packet->path_info.path;
        if (path == NULL) {
            path = (packet->navigation_flags & AIPACKET_NAVIGATION_FLAG_SEARCH_ALL_PATHS) != 0
                       ? system->path_sys->paths[0]
                       : system->path_sys->active_path;
            AISysCharacterSetPath(packet, path);
            AISysCharacterSetPathCnx(packet, &object->position, NULL, 0);
            packet->path_info.next_check = 0;
            packet->path_info.path_index = 0;
        }
        path = packet->path_info.path;
        AISysResetPathSearchConnectionChecks(path);

        AIPATHCNX *current_connection = packet->path_info.connection;
        if (current_connection != NULL) {
            const i32 current_direction = packet->path_info.direction;
            if (AISysCharacterTestPathCnx(system, object, packet, current_connection, current_direction,
                                          &nearest_distance_squared) != 0) {
                return;
            }

            const bool check_adjacent_connections =
                (object->flags_low & APIOBJECT_FLAG_PLAYER_ACTIVE) != 0 ||
                (packet->field_0x180 == NULL && packet->path_connection_state == 0 && ground != 0);
            if (!check_adjacent_connections) {
                return;
            }

            path = packet->path_info.path;
            AIPATHNODE &first_node = path->nodes[current_connection->node_indices[current_direction]];
            for (i32 connection_index = 0; connection_index < first_node.connection_count; ++connection_index) {
                AIPATHCNX *candidate = first_node.connections[connection_index];
                if (candidate != current_connection &&
                    AISysCharacterTestPathCnx(system, object, packet, candidate,
                                              candidate->node_indices[0] !=
                                                  current_connection->node_indices[current_direction],
                                              &nearest_distance_squared) != 0)
                    return;
            }

            path = packet->path_info.path;
            const i32 other_direction = current_direction == 0;
            AIPATHNODE &second_node = path->nodes[current_connection->node_indices[other_direction]];
            for (i32 connection_index = 0; connection_index < second_node.connection_count; ++connection_index) {
                AIPATHCNX *candidate = second_node.connections[connection_index];
                if (candidate != current_connection &&
                    AISysCharacterTestPathCnx(system, object, packet, candidate,
                                              candidate->node_indices[0] !=
                                                  current_connection->node_indices[other_direction],
                                              &nearest_distance_squared) != 0)
                    return;
            }
        }

        path = packet->path_info.path;
        if ((packet->navigation_flags & AIPACKET_NAVIGATION_FLAG_SEARCH_ALL_PATHS) == 0) {
            i32 remaining_checks = path->node_count < checks ? path->node_count : checks;
            while (remaining_checks-- > 0) {
                AIPATHNODE &node = path->nodes[packet->path_info.next_check];
                for (i32 connection_index = 0; connection_index < node.connection_count; ++connection_index) {
                    if (AISysCharacterTestPathCnx(system, object, packet, node.connections[connection_index], -1,
                                                  &nearest_distance_squared) != 0) {
                        return;
                    }
                }
                path = packet->path_info.path;
                packet->path_info.next_check = (packet->path_info.next_check + 1) % path->node_count;
            }
            return;
        }

        // Characters allowed to search across paths retain their current
        // cursor after probing, just as they retain their current connection.
        AIPATH *original_path = packet->path_info.path;
        AIPATHCNX *original_connection = packet->path_info.connection;
        const u8 original_direction = packet->path_info.direction;

        if (packet->path_info.path_index == 0xff) {
            packet->path_info.path_index = original_path->index;
        }
        if (packet->path_info.path_index >= system->path_sys->path_count) {
            packet->path_info.path_index = 0;
        }

        path = system->path_sys->paths[packet->path_info.path_index];
        packet->path_info.path = path;
        if (packet->path_info.next_check >= path->node_count) {
            packet->path_info.next_check = 0;
        }
        AISysResetPathSearchConnectionChecks(path);

        i32 position_inside_path_bounds = 0;
        for (i32 remaining_checks = checks; remaining_checks > 0; --remaining_checks) {
            if (!position_inside_path_bounds) {
                position_inside_path_bounds =
                    AIPathCheckExtents(packet->path_info.path, &packet->owner->apiobj.position);
            }

            if (position_inside_path_bounds) {
                path = packet->path_info.path;
                AIPATHNODE &node = path->nodes[packet->path_info.next_check];
                for (i32 connection_index = 0; connection_index < node.connection_count; ++connection_index) {
                    if (AISysCharacterTestPathCnx(system, object, packet, node.connections[connection_index], -1,
                                                  &nearest_distance_squared) != 0) {
                        return;
                    }
                }

                ++packet->path_info.next_check;
                path = packet->path_info.path;
                if (packet->path_info.next_check < path->node_count) {
                    continue;
                }
            }

            packet->path_info.next_check = 0;
            ++packet->path_info.path_index;
            if (packet->path_info.path_index >= system->path_sys->path_count) {
                packet->path_info.path_index = 0;
            }

            path = system->path_sys->paths[packet->path_info.path_index];
            packet->path_info.path = path;
            AISysCharacterSetPathCnx(packet, &object->position, NULL, 0);
            AISysResetPathSearchConnectionChecks(packet->path_info.path);
            position_inside_path_bounds = 0;
        }

        packet->path_info.path = original_path;
        packet->path_info.connection = original_connection;
        packet->path_info.direction = original_direction;
    }

    u32 AISysGetPathColour(i32 index) {
        return iAISysPathColoursG[index];
    }

    i32 AISysGetPathColourCount(void) {
        return 5;
    }

    void AISysGetPathPos(AISYS *system, NUVEC *position, AIPATHINFO *info, AIPATH *path, i32 checks) {
        AISysGetPathPosEx(system, position, info, path, checks, 0, NULL);
    }

    void AISysGetPathPosEx(AISYS *system, NUVEC *position, AIPATHINFO *info, AIPATH *path, i32 checks,
                           i32 search_all_paths, f32 *nearest_distance_squared) {
        info->flags &= static_cast<u8>(~AIPATHINFO_FLAG_ON_PATH);
        if (system == NULL || system->path_sys == NULL || system->path_sys->path_count == 0) {
            return;
        }

        if (search_all_paths != 0) {
            path = info->path;
            if (path == NULL) {
                path = system->path_sys->paths[0];
                info->connection = NULL;
                info->path = path;
                info->next_check = 0;
            }
        } else {
            if (path == NULL) {
                path = system->path_sys->active_path;
            }
            if (info->path != path) {
                info->connection = NULL;
                info->path = path;
                info->next_check = 0;
            }
        }
        AISysResetPathSearchConnectionChecks(path);

        AIPATHCNX *current = info->connection;
        if (current != NULL) {
            if (WithinConnection(system, position, path, current, 1, NULL, 0xff, 0xff, info, 0.0f, 1)) {
                return;
            }
            AIPATHNODE *node = &info->path->nodes[current->node_indices[info->direction]];
            for (i32 index = 0; index < node->connection_count; ++index) {
                AIPATHCNX *candidate = node->connections[index];
                if (candidate != current &&
                    WithinConnection(system, position, path, candidate, 1, NULL, 0xff, 0xff, info, 0.0f, 1)) {
                    return;
                }
            }
            node = &info->path->nodes[current->node_indices[info->direction == 0]];
            for (i32 index = 0; index < node->connection_count; ++index) {
                AIPATHCNX *candidate = node->connections[index];
                if (candidate != current &&
                    WithinConnection(system, position, path, candidate, 1, NULL, 0xff, 0xff, info, 0.0f, 1)) {
                    return;
                }
            }
        }

        if (search_all_paths != 0) {
            AIPATH *saved_path = info->path;
            AIPATHCNX *saved_connection = info->connection;
            u8 saved_direction = info->direction;
            if (info->path_index == 0xff) {
                info->path_index = saved_path->index;
            }
            if (info->path_index >= system->path_sys->path_count) {
                info->path_index = 0;
            }
            info->path = system->path_sys->paths[info->path_index];
            if (info->next_check >= info->path->node_count) {
                info->next_check = 0;
            }
            AISysResetPathSearchConnectionChecks(info->path);
            u8 start_path = info->path_index;
            u16 start_node = info->next_check;
            i32 inside_extents = 0;
            while (checks != 0) {
                if (inside_extents == 0) {
                    inside_extents = AIPathCheckExtents(info->path, position);
                }
                if (inside_extents != 0) {
                    AIPATHNODE *node = &info->path->nodes[info->next_check];
                    for (i32 index = 0; index < node->connection_count; ++index) {
                        AIPATHCNX *candidate = node->connections[index];
                        if (WithinConnection(system, position, info->path, candidate, 1, NULL, 0xff, 0xff, info, 0.0f,
                                             1)) {
                            return;
                        }
                        if (nearest_distance_squared != NULL) {
                            AIPATHCNX *connection = node->connections[index];
                            f32 distance = NuLineToPointDistSqrEx(
                                &info->path->nodes[connection->node_indices[0]].position,
                                &info->path->nodes[connection->node_indices[1]].position, position, NULL);
                            if (distance < *nearest_distance_squared) {
                                *nearest_distance_squared = distance;
                                saved_connection = candidate;
                                saved_path = info->path;
                            }
                        }
                    }
                    ++info->next_check;
                }
                if (inside_extents == 0 || info->next_check >= info->path->node_count) {
                    info->next_check = 0;
                    ++info->path_index;
                    if (info->path_index >= system->path_sys->path_count) {
                        info->path_index = 0;
                    }
                    info->path = system->path_sys->paths[info->path_index];
                    info->connection = NULL;
                    AISysResetPathSearchConnectionChecks(info->path);
                    inside_extents = 0;
                }
                --checks;
                if (info->path_index == start_path && info->next_check == start_node) {
                    break;
                }
            }
            info->path = saved_path;
            info->connection = saved_connection;
            info->direction = saved_direction;
        } else {
            i32 remaining = info->path->node_count < checks ? info->path->node_count : checks;
            while (remaining != 0) {
                AIPATHNODE *node = &info->path->nodes[info->next_check];
                for (i32 index = 0; index < node->connection_count; ++index) {
                    if (WithinConnection(system, position, path, node->connections[index], 1, NULL, 0xff, 0xff, info,
                                         0.0f, 1)) {
                        return;
                    }
                }
                info->next_check = (info->next_check + 1) % info->path->node_count;
                --remaining;
            }
        }
    }

    void *AISysLoadEx(void *buffer, void *buffer_end, i32 storage_size, void *scene, char *directory, char *name,
                      char *parameter, char *load_directory) {
        VARIPTR *cursor = static_cast<VARIPTR *>(buffer);
        VARIPTR *end = static_cast<VARIPTR *>(buffer_end);
        NUGSCN *gscene = static_cast<NUGSCN *>(scene);
        VARIPTR pak_start = *end;
        void *pak = NULL;
        char ai2_path[256];
        char pak_path[256];
        char script_path[256];

        if (ai_usepackfile != 0) {
            sprintf(pak_path, "%sLevels\\%s\\%s\\ai.pak", AiLevelPathName, directory, parameter);
            i32 pak_size = static_cast<i32>(NuFileSize(pak_path));
            if (pak_size > 0) {
                pak_start.addr = end->addr - ALIGN(pak_size + 1, 0x10);
                pak = NuFilePakLoad(pak_path, &pak_start, *end, 0x10);
            }
        }

        sprintf(ai2_path, "%sLevels\\%s\\%s\\%s.ai2", AiLevelPathName, load_directory, parameter, name);

        AISYS *system = static_cast<AISYS *>(AISysBufferAlloc(cursor, end, sizeof(AISYS)));
        if (system == NULL) {
            return NULL;
        }
        memset(system, 0, sizeof(AISYS));

        system->storage_size = storage_size;
        system->storage = AISysBufferAlloc(cursor, end, storage_size);
        system->storage_end = *cursor;
        memset(system->storage, 0, storage_size);
        system->storage_cursor.addr = reinterpret_cast<usize>(system->storage);

        EdFileSetMedia(1);
        i32 is_open = 0;
        if (pak != NULL) {
            sprintf(pak_path, "%s.ai2", name);
            EdFileSetPakFile(pak);
            is_open = EdFileOpen(pak_path, NUFILE_READ);
        }
        if (is_open == 0) {
            EdFileSetPakFile(NULL);
            is_open = EdFileOpen(ai2_path, NUFILE_READ);
        }

        if (is_open != 0) {
            i32 version = EdFileReadInt();
            system->scene = gscene;

            // The shipped TCS assets use version 20. These section readers
            // follow the original version-20 branches and retain the original
            // gates that affect the current layout.
            if (version == 20) {
                system->path_sys = AISysLoadPaths(system, version, gscene);
                AISysLoadAreas(system, version);
                AISysLoadLocators(system, version);
                AISysLoadLocatorSets(system, version);
                AISysLoadCreatures(system, version);
                AISysLoadAntinodes(system, version, gscene);
                if (GameAILoadFn != NULL) {
                    GameAILoadFn(system, version, gscene, cursor, end);
                }
            }
            EdFileClose();
        }

        sprintf(script_path, "%sLevels\\%s\\%s", AiLevelPathName, directory, parameter);
        AIScriptLoadAllPakFile(pak, script_path, cursor, &pak_start, system);
        AISysSetLevelPath(system, NULL);
        return system;
    }

    void AISysProcess(AISYS *system, APIOBJECT *player_1, APIOBJECT *player_2) {
        if (system == NULL) {
            return;
        }

        system->player_1 = player_1;
        system->player_2 = player_2;

        AIANTINODE *dynamic = dynamic_antinodes;
        for (i32 index = 0; index < system->antinode_count + 64; ++index) {
            const i32 antinode_count = system->antinode_count;
            AIANTINODE *antinode =
                index < antinode_count ? &system->antinodes[index] : &dynamic[index - antinode_count];
            if (antinode->has_special == 0) {
                continue;
            }

            nuhspecial_s *special = &antinode->special_handle;
            if (FindAlternativeSpecialObjectFn != NULL && NuSpecialGetVisibilityFn(special) == 0) {
                FindAlternativeSpecialObjectFn(system, special);
            }
            if (NuSpecialGetVisibilityFn(special) == 0) {
                continue;
            }

            NUMTX *draw_matrix = NuSpecialGetDrawMtx(special);
            NuVecMtxTransform(&antinode->position, &antinode->special_position, draw_matrix);
            antinode->min_y = antinode->min_y_offset + antinode->position.y;
            antinode->max_y = antinode->position.y + antinode->max_y_offset;

            if (antinode->type != 0) {
                NUVEC forward = {0.0f, 0.0f, 1.0f};
                NUVEC rotated;
                NuVecMtxRotate(&rotated, &forward, draw_matrix);
                antinode->flags = NuAngAdd(NuAtan2D(rotated.x, rotated.z), antinode->rotation_offset);
            }
        }

        for (i32 index = 0; index < 16; ++index) {
            AIGROUP *group = &system->groups[index];
            if (group->is_used) {
                group->can_respawn = group->member_is_alive == 0;
            }
        }

        for (i32 index = 0; index < system->locator_count; ++index) {
            AILOCATOR *locator = &system->locators[index];
            if (locator->path_info.path == NULL || locator->path_info.connection == NULL) {
                continue;
            }

            AIPATHNODE *start = &locator->path_info.path->nodes[locator->path_info.connection->node_indices[0]];
            AIPATHNODE *end = &locator->path_info.path->nodes[locator->path_info.connection->node_indices[1]];
            if ((start->runtime_flags & AIPATHNODE_RUNTIME_POSITION_CHANGED) == 0 &&
                (end->runtime_flags & AIPATHNODE_RUNTIME_POSITION_CHANGED) == 0) {
                continue;
            }

            NUVEC delta;
            NUVEC direction;
            NuVecSub(&delta, &end->position, &start->position);
            NuVecNorm(&direction, &delta);

            const f32 position_on_connection = locator->path_info.dist;
            f32 radius;
            if (position_on_connection > 1.0f) {
                radius = end->radius;
            } else if (position_on_connection < 0.0f) {
                radius = start->radius;
            } else {
                radius = end->radius * position_on_connection + (1.0f - position_on_connection) * start->radius;
            }

            const f32 direction_x = direction.x;
            direction.x = direction.z * radius;
            direction.z = -direction_x * radius;
            locator->position = start->position;

            NUVEC offset;
            NuVecScale(&offset, &delta, position_on_connection);
            NuVecAdd(&locator->position, &locator->position, &offset);
            NuVecScale(&offset, &direction, locator->path_info.width);
            NuVecAdd(&locator->position, &locator->position, &offset);

            const i32 path_angle = static_cast<i32>(NuAtan2(delta.x, delta.z) * 10430.378f);
            locator->flags = NuAngAdd(path_angle, locator->locator_flags);
        }
    }

    void AISysProcessCharacter(AISYS *system, APIOBJECT *object, AIPACKET *packet, i32 checks, f32 elapsed,
                               i32 use_three_dimensions, i32 process_ai) {
        AISCRIPTPROCESS *primary_processor = &packet->script_process;
        AISCRIPTPROCESS *processor = packet->alternate_script_process;
        if (processor == NULL) {
            processor = primary_processor;
        }

        if ((packet->field_0x1e6 & AIPACKET_RUNTIME_INITIALISED) == 0) {
            AISTATE *saved_state = primary_processor->state;
            if (AIScriptSetStateByName(primary_processor, "Initialise") != 0) {
                AIScriptProcess(system, object, packet, primary_processor, 0.0f);
                AIScriptSetState(primary_processor, saved_state);
            }
            packet->field_0x1e6 |= AIPACKET_RUNTIME_INITIALISED;
        }

        if (process_ai != 0) {
            AISysUpdateCharacterPathPos(system, object, packet, checks, elapsed);

            const u8 runtime_flags = packet->field_0x1e6;
            packet->movement_flags &= static_cast<u8>(~AIPACKET_MOVEMENT_OPTION_TRANSIENT);
            packet->circle_active = 0;
            packet->frame_flags = 0;
            packet->field_0x1e6 = runtime_flags & static_cast<u8>(~AIPACKET_RUNTIME_SPECIAL_MOVE);

            if ((object->field_0x1f8 & APIOBJECT_FLAG_AI_PLAYER_MASK) == APIOBJECT_FLAG_PLAYER_ACTIVE) {
                packet->path_connection_state = 0;
                packet->movement_destination = object->position;
                packet->field_0x180 = NULL;
                packet->field_0x1e6 = runtime_flags & 7;
                packet->field_0x1e7 &= 0x3f;
                packet->navigation_flags &= static_cast<u8>(~AIPACKET_NAVIGATION_FLAG_TRANSIENT);
            } else {
                if (AIRespawnOnPathFn != NULL && AIRespawnOnPathFn(object) != 0) {
                    return;
                }

                NuSpecialClear(&object->field_0x208);
                object->field_0x1fa &= static_cast<u8>(~4u);
                if (system != NULL && processor->script != NULL) {
                    AIScriptProcess(system, object, packet, processor, elapsed);
                }

                packet->navigation_flags &= static_cast<u8>(~AIPACKET_NAVIGATION_FLAG_TRANSIENT);
                packet->frame_state = 0;
                packet->field_0x1e6 &= 0x0f;

                const u8 source_flags = packet->field_0x1e7;
                packet->field_0x1e7 = source_flags & 0x3f;
                if ((source_flags & AIPACKET_MOVEMENT_SOURCE_ACTIVE) != 0) {
                    const u8 source = (source_flags & AIPACKET_MOVEMENT_SOURCE_MASK) >> 2;
                    if (source == 1) {
                        if (packet->field_0x134 != 0xff) {
                            AICREATURE *creature = &system->creatures[packet->field_0x134];
                            AIMoveInstruction(packet, &creature->pos, 0.0f, &creature->path_info,
                                              AIPACKET_MOVEMENT_TO_DESTINATION, 0.0f);
                        }
                    } else if (source == 2) {
                        if (packet->locator != NULL) {
                            AIPATHINFO *locator_path = &packet->locator->path_info;
                            AIMoveInstruction(packet, &packet->locator->position, 0.0f, locator_path,
                                              AIPACKET_MOVEMENT_TO_DESTINATION, 0.0f);
                        }
                    } else {
                        packet->field_0x1e7 = source_flags & 0x2f;
                    }
                }

                AISysCharacterMovement(system, packet, object, checks);

                if ((packet->field_0x1e7 & AIPACKET_MOVEMENT_SOURCE_MASK) != 0 && packet->field_0x180 == NULL &&
                    packet->movement_target_radius > 0.0f &&
                    (object->field_0x1f4 & APIOBJECT_MOTION_FLAG_AI_CONTROLLED) != 0) {
                    NUVEC *source_position = NULL;
                    const u8 source = (packet->field_0x1e7 & AIPACKET_MOVEMENT_SOURCE_MASK) >> 2;
                    if (source == 1 && packet->field_0x134 != 0xff) {
                        source_position = &system->creatures[packet->field_0x134].pos;
                    } else if (source == 2 && packet->locator != NULL) {
                        source_position = &packet->locator->position;
                    }

                    if (source_position != NULL) {
                        NUVEC source_delta;
                        const f32 distance_squared = NuVecXZDistSqr(source_position, &object->position, &source_delta);
                        const f32 radius = packet->movement_target_radius;
                        const u8 current_source_flags = packet->field_0x1e7;
                        if ((current_source_flags & AIPACKET_MOVEMENT_SOURCE_ACTIVE) == 0) {
                            if (distance_squared > radius * radius) {
                                const f32 release_radius = radius + 0.5f;
                                if (distance_squared > release_radius * release_radius) {
                                    packet->field_0x1e7 = current_source_flags | AIPACKET_MOVEMENT_SOURCE_ACTIVE;
                                } else {
                                    NUVEC movement_delta = {
                                        packet->movement_destination.x - object->position.x,
                                        0.0f,
                                        packet->movement_destination.z - object->position.z,
                                    };
                                    if (NuVecDot(&movement_delta, &source_delta) < 0.0f) {
                                        packet->movement_destination = object->position;
                                        packet->movement_stopping_distance = 0.0f;
                                    }
                                }
                            }
                        } else if (distance_squared < radius * radius) {
                            packet->field_0x1e7 =
                                current_source_flags & static_cast<u8>(~AIPACKET_MOVEMENT_SOURCE_ACTIVE);
                        }
                    }
                }
            }
        }

        f32 clearance = 0.0f;
        if (packet->movement_stopping_distance != 0.0f) {
            clearance = packet->mover_height;
        }

        NUVEC delta;
        if (use_three_dimensions == 0) {
            const f32 distance = NuVecXZDist(&packet->movement_destination, &object->position, &delta);
            const f32 stopping_clearance = clearance + packet->movement_stopping_distance;
            if (distance > packet->mover_height + clearance + packet->movement_stopping_distance) {
                const f32 scale =
                    distance != 0.0f && packet->mover_height != 0.0f ? packet->mover_height / distance : 0.0f;
                NuVecScale(&delta, &delta, scale);
                NuVecAdd(&packet->movement_position, &object->position, &delta);
            } else {
                const f32 scale = distance != 0.0f && stopping_clearance != 0.0f ? stopping_clearance / distance : 0.0f;
                NuVecScale(&delta, &delta, scale);
                NuVecSub(&packet->movement_position, &packet->movement_destination, &delta);
            }
        } else {
            const f32 distance = NuVecDist(&packet->movement_destination, &object->position, &delta);
            const f32 stopping_clearance = clearance + packet->movement_stopping_distance;
            if (distance > packet->mover_height + clearance + packet->movement_stopping_distance) {
                const f32 scale =
                    distance != 0.0f && packet->mover_height != 0.0f ? packet->mover_height / distance : 0.0f;
                NuVecScale(&delta, &delta, scale);
                NuVecAdd(&packet->movement_position, &object->position, &delta);
            } else {
                const f32 scale = distance != 0.0f && stopping_clearance != 0.0f ? stopping_clearance / distance : 0.0f;
                NuVecScale(&delta, &delta, scale);
                NuVecSub(&packet->movement_position, &packet->movement_destination, &delta);
            }
        }
    }

    i32 AISysSetLevelPath(AISYS *system, char *path_name) {
        if (system == NULL || system->path_sys == NULL) {
            return 0;
        }

        AIPATH *path = NULL;
        if (path_name == NULL) {
            path = system->path_sys->paths[0];
        } else {
            for (i32 i = 0; i < system->path_sys->path_count; ++i) {
                if (NuStrICmp(system->path_sys->paths[i]->name, path_name) == 0) {
                    path = system->path_sys->paths[i];
                    break;
                }
            }
        }

        if (path == NULL || system->path_sys->active_path == path) {
            return 0;
        }
        system->path_sys->active_path = path;
        return 1;
    }

    void AISysUpdateCharacterPathPos(AISYS *system, APIOBJECT *object, AIPACKET *packet, i32 checks, f32 elapsed) {
        u8 movement_source_flags = packet->field_0x1e7;
        const bool should_find_path =
            (movement_source_flags & AIPACKET_MOVEMENT_FORCE_PATH_REFRESH) == 0 &&
            (((object->flags_low & 4) == 0 || (packet->path_info.flags & AIPATHINFO_FLAG_ON_PATH) == 0) ||
             object->supporting_platform_id != -1);

        if (should_find_path) {
            if (packet->path_info.path == NULL) {
                packet->last_path_position = object->position;
            } else {
                if ((object->field_0x1fa & 8) == 0) {
                    AISysGetCharacterPathPos(system, object, packet, 1, checks);
                } else {
                    AISysGetCharacterPathPos(system, object, packet, 0, checks);
                }
                movement_source_flags = packet->field_0x1e7;
            }

            if ((packet->path_info.flags & AIPATHINFO_FLAG_ON_PATH) == 0) {
                packet->time_off_path += elapsed;
            } else {
                packet->time_off_path = 0.0f;
            }
        } else {
            AIPATH *path = packet->path_info.path;
            if (path != NULL && packet->path_info.connection != NULL) {
                u8 node_index = packet->path_info.connection->node_indices[0];
                AIPATHNODE *first = &path->nodes[node_index];
                AIPATHNODE *second = &path->nodes[packet->path_info.connection->node_indices[1]];
                if (first->has_special != 0 &&
                    (path->updated_node_bits[node_index / 8] & (1u << (node_index % 8))) == 0) {
                    AIPathNodeUpdatePos(system, path, first);
                }
                if (second->has_special != 0) {
                    node_index = packet->path_info.connection->node_indices[1];
                    path = packet->path_info.path;
                    if ((path->updated_node_bits[node_index / 8] & (1u << (node_index % 8))) == 0) {
                        AIPathNodeUpdatePos(system, path, second);
                    }
                }
            }

            path = packet->path_info.path;
            if (packet->inside_path_node != -1 && path != NULL) {
                const i16 node_index = packet->inside_path_node;
                path->inside_node_bits[node_index / 8] |= static_cast<u8>(1u << (node_index % 8));
            }
            packet->time_off_path = 0.0f;
            movement_source_flags = packet->field_0x1e7;
        }

        packet->field_0x1e7 = movement_source_flags & static_cast<u8>(~AIPACKET_MOVEMENT_FORCE_PATH_REFRESH);
    }

    void AddToAIGroup(AIGROUP *group, APIOBJECT *object) {
        if (object == NULL || group == NULL || group->member_count >= 16) {
            return;
        }

        const u8 member_index = group->member_count;
        const u32 count_across = group->count_across;
        i32 row_count = static_cast<i32>(NuFdiv(static_cast<f32>(member_index + 1), static_cast<f32>(count_across)));
        if ((member_index + 1) % group->count_across != 0)
            ++row_count;
        if (row_count > 4) {
            return;
        }

        if (member_index == 0) {
            group->leader = object;
            if (group->x_spacing > 0.0f) {
                group->radius = (static_cast<f32>(count_across) - 1.0f) * group->x_spacing * 0.5f;
                group->rotation_speed = static_cast<i32>(10430.378f * NuFdiv(group->max_speed, group->radius * 60.0f));
            } else {
                group->radius = 0.0f;
                group->rotation_speed = 100;
            }
        }

        group->row_count = static_cast<u8>(row_count);
        object->ai->group = group;
        object->ai->group_member_index = group->member_count;
        object->ai->group_row = static_cast<i32>(NuFdiv(static_cast<f32>(static_cast<u32>(group->member_count)),
                                                        static_cast<f32>(static_cast<u32>(group->count_across))));
        object->ai->group_column = group->member_count % group->count_across;
        group->members[group->member_count] = object;
        ++group->member_count;
    }

    f32 AiRndrCullDistance;
    void SetAiRndrCullDistance(f32 distance) {
        AiRndrCullDistance = distance;
    }

    AIEDITOR_RENDER_STATE *aieditor;
    void NuRndrLine3dDbg(f32, f32, f32, f32, f32, f32, i32);
    void AiRndrLine3d(NURND_VERTEX3D *vertices, NUMTL *, NUMTX *matrix) {
        NUVEC first = vertices[0].position;
        NUVEC second = vertices[1].position;
        NUMTX matrix_copy;
        if (matrix != NULL) {
            if ((reinterpret_cast<usize>(matrix) & 15) != 0) {
                matrix_copy = *matrix;
                matrix = &matrix_copy;
            }
            NuVecMtxTransform(&first, &first, matrix);
            NuVecMtxTransform(&second, &second, matrix);
        }
        if (AiRndrCullDistance > 0.0f) {
            NUVEC position;
            if (aieditor && (aieditor->flags & 2))
                position = aieditor->camera_position;
            else
                position = *(NUVEC *)&global_camera.mtx.m30;
            f32 dx = position.x - first.x;
            f32 dy = position.y - first.y;
            f32 dz = position.z - first.z;
            f32 limit = AiRndrCullDistance * AiRndrCullDistance;
            f32 dx2 = position.x - second.x;
            f32 dy2 = position.y - second.y;
            f32 dz2 = position.z - second.z;
            if (dx * dx + dy * dy + dz * dz > limit && dx2 * dx2 + dy2 * dy2 + dz2 * dz2 > limit)
                return;
        }
        NuRndrLine3dDbg(first.x, first.y, first.z, second.x, second.y, second.z, vertices[0].colour);
    }
    void AiRndrLine3dDbg(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1, u32 colour) {
        if (AiRndrCullDistance > 0.0f) {
            NUVEC position;
            if (aieditor && (aieditor->flags & 2))
                position = aieditor->camera_position;
            else
                position = *(NUVEC *)&global_camera.mtx.m30;
            f32 dx = position.x - x0;
            f32 dy = position.y - y0;
            f32 dz = position.z - z0;
            f32 limit = AiRndrCullDistance * AiRndrCullDistance;
            if (!(dx * dx + dy * dy + dz * dz < limit)) {
                dx = position.x - x1;
                dy = position.y - y1;
                dz = position.z - z1;
                if (!(dx * dx + dy * dy + dz * dz < limit))
                    return;
            }
        }
        NuRndrLine3dDbg(x0, y0, z0, x1, y1, z1, colour);
    }

    void AiSysOnlyUsePakFile(i32 enabled) {
        ai_onlyusepackfile = enabled;
    }

    void AiSysSetStateDebugee(AISCRIPTPROCESS *processor) {
        pSetStateDebugee = processor;
    }

    void AiSysUsePackFile(i32 enabled) {
        ai_usepackfile = enabled;
    }

    void CalculateLocatorDirection(i32 direction, numtx_s *matrix, NUVEC *out) {
        switch (direction) {
            case 0:
            case 1:
                out->z = 0.0f;
                out->x = 0.0f;
                out->y = 1.0f;
                return;
            case 2:
                out->z = 0.0f;
                out->x = 0.0f;
                out->y = -1.0f;
                return;
            case 3:
                out->x = -1.0f;
                out->y = 0.0f;
                out->z = 0.0f;
                break;
            case 4:
                out->x = 1.0f;
                out->y = 0.0f;
                out->z = 0.0f;
                break;
            case 5:
                out->x = 0.0f;
                out->y = -1.0f;
                out->z = 0.0f;
                break;
            case 6:
                out->x = 0.0f;
                out->y = 1.0f;
                out->z = 0.0f;
                break;
            case 7:
                out->x = 0.0f;
                out->y = 0.0f;
                out->z = 1.0f;
                break;
            case 8:
                out->x = 0.0f;
                out->y = 0.0f;
                out->z = -1.0f;
                break;
            default:
                return;
        }
        NuVecMtxRotate(out, out, matrix);
    }

    AIGROUP *CreateAIGroup(AISYS *system, i32 count_across, f32 x_spacing, f32 z_spacing, f32 max_speed) {
        if (count_across == 0 || system == NULL) {
            return NULL;
        }

        i32 index;
        AIGROUP *group;
        for (index = 0; index < 16; ++index) {
            group = &system->groups[index];
            if (group->is_used == 0)
                break;
        }
        if (index == 16)
            return NULL;
        system->groups[index].is_used = 1;
        group->count_across = count_across;
        group->x_spacing = x_spacing;
        group->z_spacing = z_spacing;
        group->is_in_formation = 0;
        group->max_speed = max_speed;
        return group;
    }

    void DestroyAIGroup(AIGROUP *group) {
        if (group != NULL)
            memset(group, 0, sizeof(*group));
    }

    void FindAIDirectionedRandomPointOnNetwork2D(AIPACKET *packet, NUVEC *position, NUVEC *direction, f32 distance,
                                                 f32 offset, i32 flags, NUVEC *result) {
        AIPATHCNX *connection = packet->path_info.connection;
        if (connection == NULL) {
            *result = packet->owner->apiobj.position;
            return;
        }

        f32 direction_x = direction->x;
        f32 direction_z = direction->z;
        AIPATHNODE *nodes = packet->path_info.path->nodes;
        AIPATHNODE *start = &nodes[connection->direction_a];
        AIPATHNODE *end = &nodes[connection->direction_b];
        f32 dx = end->position.x - start->position.x;
        f32 dz = end->position.z - start->position.z;
        if (direction_x * dx + direction_z * dz < 0.0f) {
            dx = start->position.x - end->position.x;
            dz = start->position.z - end->position.z;
            AIPATHNODE *swap = start;
            start = end;
            end = swap;
        }
        f32 length = connection->horizontal_distance;
        dx /= length;
        dz /= length;
        if ((flags & 2) != 0) {
            distance *= length;
        }
        f32 along = (position->x - start->position.x) * dx + (position->z - start->position.z) * dz + distance;
        if (along > length || along < 0.0f) {
            AIPATHNODE *junction = along < 0.0f ? start : end;
            i32 best_index = -1;
            f32 best_dot = -FLT_MAX;
            for (i32 index = 0; index < junction->connection_count; ++index) {
                AIPATHCNX *candidate = junction->connections[index];
                if (candidate == connection) {
                    continue;
                }
                AIPATHNODE *candidate_start = &nodes[candidate->direction_a];
                AIPATHNODE *candidate_end = &nodes[candidate->direction_b];
                dx = candidate_end->position.x - candidate_start->position.x;
                dz = candidate_end->position.z - candidate_start->position.z;
                i32 reverse = 0;
                if (direction_x * dx + direction_z * dz < 0.0f) {
                    dx = candidate_start->position.x - candidate_end->position.x;
                    dz = candidate_start->position.z - candidate_end->position.z;
                    reverse = 1;
                }
                dx /= candidate->horizontal_distance;
                dz /= candidate->horizontal_distance;
                if ((candidate->traversal_flags[reverse] & 0xc0000000u) == 0) {
                    f32 dot = direction_x * dx + direction_z * dz;
                    if (dot > best_dot) {
                        best_dot = dot;
                        best_index = index;
                    }
                }
            }
            if (best_index != -1) {
                connection = junction->connections[best_index];
                if (along < 0.0f) {
                    length = connection->horizontal_distance;
                    along += length;
                } else {
                    along -= length;
                    length = connection->horizontal_distance;
                }
                start = &nodes[connection->direction_a];
                end = &nodes[connection->direction_b];
                dx = end->position.x - start->position.x;
                dz = end->position.z - start->position.z;
                if (direction_x * dx + direction_z * dz < 0.0f) {
                    dx = start->position.x - end->position.x;
                    dz = start->position.z - end->position.z;
                    AIPATHNODE *swap = start;
                    start = end;
                    end = swap;
                }
                dx /= length;
                dz /= length;
            }
        }
        f32 fraction = NuFmin(NuFmax(along / length, 0.0f), 1.0f);
        f32 inverse_fraction = 1.0f - fraction;
        f32 radius = fraction * end->radius + inverse_fraction * start->radius;
        along = connection->horizontal_distance * fraction;
        if ((flags & 1) != 0) {
            offset *= radius;
        }
        offset = NuFmin(NuFmax(offset, -radius), radius);
        result->x = start->position.x + along * dx + dz * offset;
        result->y = fraction * end->position.y + inverse_fraction * start->position.y;
        result->z = start->position.z + along * dz - dx * offset;
    }

    void FollowAPIObject(APIOBJECT *object, APIOBJECT *target, i32 flags, f32 movement_parameter) {
        AIPACKET *packet;
        AIPACKET *target_packet;

        NUVEC *destination;
        i32 movement_mode;
        if ((flags & 1) != 0) {
            packet = object->ai;
            target_packet = target->ai;
            bool use_target_path_position = false;
            AIPATH *path = packet->path_info.path;
            if (path != NULL && path == target_packet->path_info.path) {
                if (((target_packet->path_info.flags & AIPATHINFO_FLAG_ON_PATH) != 0 ||
                     packet->path_info.connection != target_packet->path_info.connection) &&
                    packet->path_info.connection != NULL && target_packet->path_info.connection != NULL &&
                    (packet->runtime_flags & AIPACKET_RUNTIME_ROUTE_SELECTED) == 0) {
                    const u8 object_direction = packet->path_info.connection->direction_a;
                    const u8 target_direction = target_packet->path_info.connection->direction_a;
                    if (path->route_matrix[object_direction][target_direction] != 0xff ||
                        object_direction == target_direction) {
                        use_target_path_position = true;
                    }
                }
            }

            if (use_target_path_position) {
                destination = &target_packet->last_path_position;
                movement_mode = AIPACKET_MOVEMENT_TO_DESTINATION;
            } else {
                destination = &target_packet->terrain_origin;
                movement_mode = AIPACKET_MOVEMENT_DIRECT;
            }
        } else {
            packet = object->ai;
            target_packet = target->ai;
            destination = &target_packet->last_path_position;
            movement_mode = AIPACKET_MOVEMENT_TO_DESTINATION;
        }

        const f32 stopping_distance = (flags & 2) != 0 ? 0.0f : target_packet->mover_height;
        AIMoveInstruction(packet, destination, stopping_distance, &target_packet->path_info, movement_mode,
                          movement_parameter);
    }

    typedef void AIEDITORCALLBACK(void);

    AIEDITORCALLBACK *AIPathDeletedFn;
    AIEDITORPATHNODECALLBACK *AIPathNodeDeletedFn;
    AIEDITORCALLBACK *AIPathNodeMovedFn;
    AIEDITORCALLBACK *GameAISYSRebuildFromEditorDataFn;
    AIEDITORCALLBACK *GameAISaveFn;

    void InitFn_AIPathDeleted(AIEDITORCALLBACK *function) {
        AIPathDeletedFn = function;
    }

    void InitFn_AIPathNodeDeleted(AIEDITORPATHNODECALLBACK *function) {
        AIPathNodeDeletedFn = function;
    }

    void InitFn_AIPathNodeMoved(AIEDITORCALLBACK *function) {
        AIPathNodeMovedFn = function;
    }

    void InitFn_GameAISYSRebuildFromEditorData(AIEDITORCALLBACK *function) {
        GameAISYSRebuildFromEditorDataFn = function;
    }

    void InitFn_GameAISave(AIEDITORCALLBACK *function) {
        GameAISaveFn = function;
    }

    void InitFn_GameParamToFloat(GAMEPARAMTOFLOAT *function) {
        GameParamToFloatFn = function;
    }

    void InitFn_ScriptProcessFirstTimeAction(SCRIPTPROCESSFIRSTTIMEACTION *function) {
        ScriptProcessFirstTimeActionFn = function;
    }

    void LEGO_AISysCreatureInteraction2D(AISYS *system, i32 object_count, APIOBJECT **objects, i32 *immovable,
                                         f32 delta_time) {
        for (i32 index = 0; index < object_count; ++index) {
            f32 timer = objects[index]->ai->antinode_timer - delta_time;
            if (timer < 0.0f) {
                timer = 0.0f;
            }
            objects[index]->ai->antinode_timer = timer;
        }
        for (i32 first_index = 0; first_index < object_count - 1; ++first_index) {
            APIOBJECT *first = objects[first_index];
            for (i32 second_index = first_index + 1; second_index < object_count; ++second_index) {
                APIOBJECT *second = objects[second_index];
                AIPACKET *first_packet = first->ai;
                AIPACKET *second_packet = second->ai;
                f32 first_radius = first_packet->mover_height;
                f32 second_radius = second_packet->mover_height;
                f32 radius = first_radius + second_radius;
                NUVEC difference;
                difference.x = first_packet->movement_position.x - second_packet->movement_position.x;
                if (difference.x > radius || difference.x < -radius) {
                    continue;
                }
                difference.z = first_packet->movement_position.z - second_packet->movement_position.z;
                if (difference.z > radius || difference.z < -radius) {
                    continue;
                }
                if (first->collision_min.y > second->collision_max.y ||
                    second->collision_min.y > first->collision_max.y) {
                    continue;
                }
                if (first->collision_link == second || second->collision_link == first) {
                    continue;
                }
                i32 first_fixed = immovable != NULL ? immovable[first_index] : 0;
                i32 second_fixed = immovable != NULL ? immovable[second_index] : 0;
                if (first_packet->dont_avoid_character == reinterpret_cast<GameObject_s *>(second)) {
                    first_fixed = 1;
                }
                if (second_packet->dont_avoid_character == reinterpret_cast<GameObject_s *>(first)) {
                    second_fixed = 1;
                }
                if ((first->flags_low & 0x80) != 0) {
                    first_fixed = 1;
                }
                if ((second->flags_low & 0x80) != 0) {
                    second_fixed = 1;
                }
                bool fixed_first;
                bool fixed_second;
                if (first->collision_priority > second->collision_priority) {
                    fixed_first = true;
                    fixed_second = second_fixed != 0;
                } else if (first->collision_priority < second->collision_priority) {
                    fixed_first = first_fixed != 0;
                    fixed_second = true;
                } else {
                    continue;
                }
                if (fixed_first && fixed_second) {
                    continue;
                }
                f32 distance = NuFsqrt(difference.x * difference.x + difference.z * difference.z);
                if (distance < radius) {
                    difference.y = 0.0f;
                    f32 overlap = radius - distance;
                    if (fixed_first) {
                        if (first->collision_priority > second->resolved_collision_priority) {
                            f32 scale = overlap;
                            if (scale == 0.0f || distance == 0.0f) {
                                scale = 0.0f;
                            } else {
                                scale /= distance;
                            }
                            NuVecScale(&difference, &difference, scale);
                            NuVecSub(&second->ai->movement_position, &second->ai->movement_position, &difference);
                            second->resolved_collision_priority = first->collision_priority;
                        }
                    } else if (fixed_second) {
                        if (second->collision_priority > first->resolved_collision_priority) {
                            f32 scale = overlap;
                            if (scale == 0.0f || distance == 0.0f) {
                                scale = 0.0f;
                            } else {
                                scale /= distance;
                            }
                            NuVecScale(&difference, &difference, scale);
                            NuVecAdd(&first->ai->movement_position, &first->ai->movement_position, &difference);
                            first->resolved_collision_priority = second->collision_priority;
                        }
                    } else {
                        f32 scale = overlap;
                        if (scale == 0.0f || distance == 0.0f) {
                            scale = 0.0f;
                        } else {
                            scale /= distance;
                        }
                        NuVecScale(&difference, &difference, scale);
                        NuVecSub(&second->ai->movement_position, &second->ai->movement_position, &difference);
                    }
                    AIPACKET *first_ai = first->ai;
                    AIPACKET *second_ai = second->ai;
                    f32 first_timer = first_ai->antinode_timer;
                    f32 second_timer = second_ai->antinode_timer;
                    if (first_timer > antinode_time && second_timer > antinode_time) {
                        if (first_timer > second_timer) {
                            second_ai->antinode_timer = first_timer;
                            second_ai->antinode_clockwise = first_ai->antinode_clockwise;
                        } else {
                            first_ai->antinode_timer = second_timer;
                            first_ai->antinode_clockwise = second_ai->antinode_clockwise;
                        }
                    } else if (second_timer > antinode_time) {
                        first_ai->antinode_timer = second_timer;
                        first_ai->antinode_clockwise = second_ai->antinode_clockwise;
                    } else if (first_timer > 0.0f) {
                        second_ai->antinode_timer = first_timer;
                        second_ai->antinode_clockwise = first_ai->antinode_clockwise;
                    }
                    if (((first->field_0x1f4 ^ second->field_0x1f4) & 1) != 0) {
                        first->ai->field_0x1e5 |= 0x40;
                        second->ai->field_0x1e5 |= 0x40;
                    }
                }
            }
        }
        AISysCreatureAntinodeInteraction(system, object_count, objects, immovable);
    }

    void QueryLocalMessage(void) {
    }

} // extern "C"
