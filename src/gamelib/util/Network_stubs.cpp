#include "gamelib_util_types.h"

NetworkObject *NetworkObjectManager::FindNetworkObject(i32 id) {
    if (id == 0 || objects[id].object == NULL) {
        return NULL;
    }
    return &objects[id];
}

NetworkObjectManager::PendingObject *NetworkObjectManager::FindPendingObject(NetworkObject *object) {
    i32 i = 0;
    while (pending_objects[i].object != object) {
        if (++i == 32) {
            return NULL;
        }
    }

    if (object != NULL) {
        return &pending_objects[i];
    }
    pending_objects[i].field_00 = 0;
    pending_objects[i].field_04 = 0;
    return &pending_objects[i];
}

i32 NetworkObjectManager::GetGuid(void *object) {
    if (object == NULL) {
        return 0;
    }
    NetworkObject *network_object = FindNetworkObject(object);
    if (network_object == NULL) {
        return 0;
    }
    return network_object->id;
}
