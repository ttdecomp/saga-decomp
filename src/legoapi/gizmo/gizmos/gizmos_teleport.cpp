#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuspline.h"

static i32 FindTeleport_Direction;

TELEPORT_s *Teleport_Find(GameObject_s *object, float range_squared, VuVec *position) {
    if (WORLD->teleports == NULL)
        return NULL;
    f32 radius = 2.5f * object->apiobj.collision_radius;
    const f32 radius_squared = radius * radius;
    i32 nearest = -1;
    f32 nearest_distance = 100000000.0f;
    for (i32 i = 0; i < WORLD->teleport_count; ++i) {
        TELEPORT_s *teleport = &WORLD->teleports[i];
        if (teleport->active != 0 || teleport->enabled == 0)
            continue;
        if (NuSpecialExistsFn(&teleport->blocking_special)) {
            nuinstanim_s *animation = NuSpecialGetInstAnim(&teleport->blocking_special);
            if (animation != NULL) {
                if (animation->playing || animation->ltime == 1.0f)
                    continue;
            } else if (NuSpecialGetVisibilityFn(&teleport->blocking_special)) {
                continue;
            }
        }
        const f32 fallback_range = (teleport->flags & 2) != 0 ? teleport->range_squared : radius_squared;
        NUVEC *point = teleport->path->pts;
        f32 distance = NuVecDistSqr(&object->apiobj.collision_position, point, NULL);
        if (((range_squared != 0.0f && distance < range_squared) || distance < fallback_range) &&
            (nearest == -1 || distance < nearest_distance)) {
            if (position != NULL && point != NULL) {
                position->x = point->x;
                position->y = point->y;
                position->z = point->z;
            }
            FindTeleport_Direction = 0;
            nearest = i;
            nearest_distance = distance;
        }
        if ((teleport->flags & 1) != 0)
            continue;
        point = &teleport->path->pts[teleport->path->length - 1];
        distance = NuVecDistSqr(&object->apiobj.collision_position, point, NULL);
        if (((range_squared != 0.0f && distance < range_squared) || distance < fallback_range) &&
            (nearest == -1 || distance < nearest_distance)) {
            if (position != NULL && point != NULL) {
                position->x = point->x;
                position->y = point->y;
                position->z = point->z;
            }
            FindTeleport_Direction = 1;
            nearest = i;
            nearest_distance = distance;
        }
    }
    return nearest == -1 ? NULL : &WORLD->teleports[nearest];
}

void Teleports_Reset(WORLDINFO_s *world) {
    if (world->teleports == NULL || WORLD->teleport_count <= 0) {
        return;
    }

    TELEPORT_s *teleport = world->teleports;
    for (i32 i = 0; i < WORLD->teleport_count; ++i, ++teleport) {
        teleport->active = 0;
        teleport->enabled = 1;
        teleport->field_78 = 0;
        teleport->field_74 = 0;
        teleport->field_7a = 0;
        teleport->field_76 = 0;
    }
}

void Teleport_MoveCode(GameObject_s *, i32) {
}

void Teleport_NetMoveCode(GameObject_s *) {
}

i32 Teleport_UpdateHints(HINT_s *hint) {
    if (WORLD->teleport_count <= 0 || player->field_0x7a5 != 0xff)
        return 0;
    for (i32 i = 0; i < 2; ++i) {
        GameObject_s *object = Player[i];
        if (object == NULL || ((object->field_0xe24 & 0x20) == 0 && Teleport_Find(object, 1.5625f, NULL) == NULL))
            continue;
        if (hint->control_mode_ids[0] == 0x268)
            return AvailableToPlayer(0x40000, -1, 0, 1) != 0;
        if (hint->control_mode_ids[0] == 0x622 && FreePlay != 0 && AvailableToPlayer(0x40000, -1, 0, 1) == 0)
            return 1;
        return 0;
    }
    return 0;
}

void Teleports_UpdateAfterGameObjects(WORLDINFO_s *) {
}

void Teleports_UpdateBeforeGameObjects(WORLDINFO_s *) {
}

#include "legoapi/gizmo/base/TeleportObjectInterface.h"

void TELEPORT_s::ClearMechObjectInterface() {
    delete mech_object_interface;
}

MechObjectInterface *TELEPORT_s::GetMechObjectInterface() {
    if (mech_object_interface == NULL) {
        new TeleportObjectInterface(*this, -1);
    }
    return mech_object_interface;
}
