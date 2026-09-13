#include "gamelib_util_types.h"
#include "gamelib/util/Utilities.h"
#include "gameapi/edtools/gameapi_edtools_types.h"
#include "legoapi/legoapi_types.h"
#include <string.h>
NetSession *theSession;
i16 NetReplicator::smNextId;
i16 NetChangedReplicator::mTableInited;
i16 NetChangedReplicator::mCrc32Table[256];

extern EdRegistry theRegistry;
extern NetTransporter theNetwork;
extern MemoryManager theMemoryManager;

void NetworkSyncPause() {
}

void NetRotator2::PredictValue(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **,
                               float *, i32) {
}

bool NetPredictor::AllowPush(EdClass const *, void const *, ReplicatorData &, i32, i32) {
    return false;
}

void NetPredictor::CheckPredictionError(EdClass const *, void *, float *, float *, i32) {
}

void NetPredictor::DoPrediction(EdClass const *, void *, ReplicatorData &, NetPredictor::PredictorTime *, i32) {
}

void NetPredictor::DoPrediction(EdClass const *, void *, ReplicatorData &, i32) {
}

void NetPredictor::SerialiseObject(EdStream &, NetPeer *, EdClass const *, void *, ReplicatorData &,
                                   NetPredictor::PredictorTime *, i16 *) {
}

void NetPredictor::SerialiseObject(EdStream &, NetPeer *, EdClass const *, void *, ReplicatorData &, i16 *) {
}

void NetPredictor::StoreSampleData(EdClass const *, void *, NetPredictor::PredictorTime *,
                                   NetPredictor::PredictorData **, float *, i32) {
}

void NetPredictor2::PredictValue(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **,
                                 float *, i32) {
}

void NetPredictor3::PredictValue(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **,
                                 float *, i32) {
}

NetReplicator::NetReplicator(i32 group, float minimum_seconds, float maximum_seconds) {
    list_previous = 0;
    list_next = 0;
    minimum_interval = minimum_seconds < 0.1f ? 100 : static_cast<u32>(minimum_seconds * 1000.0f);
    if (maximum_seconds > 0.0f && maximum_seconds < 0.1f) {
        maximum_interval = 100;
    } else {
        maximum_interval = static_cast<u32>(maximum_seconds * 1000.0f);
    }
    field_14 = 0;
    field_16 = 0;
    id = smNextId++;
    replication_group = static_cast<u16>(group);
}

void NetReplicator::SerialiseObject(EdStream &, NetPeer *, EdClass const *, void *, ReplicatorData &, i16 *) {
}

void NetReplicator::DoPrediction(EdClass const *, void *, ReplicatorData &, i32) {
}

void NetworkObject::Destroy() {
    i32 class_id = theRegistry.GetClassId(object_class);
    i32 data_size = theNetwork.replicator_data_sizes[class_id];
    if (data_size > 0) {
        theMemoryManager.FreePool(replicator_data, static_cast<u32>(data_size));
        replicator_data = NULL;
    }
    id = 0;
    object = NULL;
    object_class = NULL;
    owner = NULL;
    flags = 0;
}

void NetworkObject::Initialise(i32 guid, void *new_object, EdClass *new_class, NetPeer const &new_owner,
                               i32 new_flags) {
    id = static_cast<i16>(guid);
    object = new_object;
    object_class = new_class;
    owner = &new_owner;
    flags |= static_cast<u16>(new_flags);

    i32 data_size = theNetwork.replicator_data_sizes[theRegistry.GetClassId(new_class)];
    if (data_size > 0) {
        replicator_data = theMemoryManager.AllocPool(static_cast<u32>(data_size), 1);
    }
}

void NetListenerList::Find(NetListenerBinding *) {
}

bool NetConstReplicator::AllowPush(EdClass const *, void const *, ReplicatorData &data, i32 force, i32) {
    u32 *last_push = reinterpret_cast<u32 *>((reinterpret_cast<uintptr_t>(data.cursor) + 3) & ~3u);
    data.cursor = reinterpret_cast<u8 *>(last_push + 1);

    u32 now = UtilGetFrameStartTime();
    if (force == 0 && *last_push != 0) {
        return false;
    }
    *last_push = now;
    return true;
}

NetListenerBinding::NetListenerBinding(NetListenerInterface *, unsigned char, char *) {
}

void NetListenerBinding::operator=(NetListenerBinding const &) {
}

void NetListenerBinding::operator==(NetListenerBinding const &) {
}

bool NetSimpleReplicator::AllowPush(EdClass const *, void const *, ReplicatorData &data, i32 force, i32) {
    u32 *last_push = reinterpret_cast<u32 *>((reinterpret_cast<uintptr_t>(data.cursor) + 3) & ~3u);
    data.cursor = reinterpret_cast<u8 *>(last_push + 1);

    u32 now = UtilGetFrameStartTime();
    if (force != 0 || now - *last_push > minimum_interval) {
        *last_push = now;
        return true;
    }
    return false;
}

bool NetChangedReplicator::AllowPush(EdClass const *object_class, void const *object, ReplicatorData &data, i32 force,
                                     i32 skip_checksum) {
    u32 checksum = 0xffffffffu;
    u32 *last_push = reinterpret_cast<u32 *>((reinterpret_cast<uintptr_t>(data.cursor) + 3) & ~3u);
    u32 *last_checksum = last_push + 1;
    data.cursor = reinterpret_cast<u8 *>(last_checksum + 1);

    u32 now = UtilGetFrameStartTime();
    u32 elapsed = now - *last_push;
    if (force != 0) {
        *last_push = now;
        return true;
    }
    if (maximum_interval != 0 && elapsed > maximum_interval) {
        *last_push = now;
        return true;
    }
    if (elapsed <= minimum_interval || skip_checksum != 0) {
        return false;
    }

    CheckSumObject(object_class, object, checksum);
    if (*last_checksum == checksum) {
        return false;
    }
    *last_checksum = checksum;
    *last_push = now;
    return true;
}

void NetChangedReplicator::CheckSum(unsigned char const *bytes, u32 size, u32 &checksum) const {
    u32 value = checksum;
    for (u32 i = 0; i < size; i++) {
        value = static_cast<i32>(mCrc32Table[static_cast<u8>(value ^ bytes[i])]) ^ (value >> 8);
        checksum = value;
    }
    checksum = ~value;
}

void NetChangedReplicator::CheckSumObject(EdClass const *object_class, void const *object, u32 &checksum) const {
    EdMember *member = object_class->members;
    while (member != NULL) {
        if (member->class_marker < 0) {
            EdClass *member_class = theRegistry.GetClass(member->type_id);
            void *member_object = member->vtable->get_member_object(member, object);
            if (member_object != NULL) {
                CheckSumObject(member_class, member_object, checksum);
            }
        } else if (member->replication_group == replication_group) {
            EdType *type = theRegistry.GetType(member->type_id);
            i32 size = member->array_size > 0 ? member->array_size : type->size;
            u8 data[256];
            member->vtable->get_member_data(member, object, member->type_id, data, sizeof(data));
            CheckSum(data, static_cast<u32>(size), checksum);
        }
        member = member->next;
    }
}

void NetChangedReplicator::InitTable() {
    if (mTableInited != 0) {
        return;
    }
    mTableInited = 1;

    for (u32 i = 0; i < 256; i++) {
        u32 value = i;
        for (i32 bit = 0; bit < 8; bit++) {
            if ((value & 1) != 0) {
                value = (value >> 1) ^ 0xedb88320u;
            } else {
                value >>= 1;
            }
        }
        mCrc32Table[i] = static_cast<i16>(value);
    }
}

void NetworkObjectManager::Acquire(i32) {
}

void NetworkObjectManager::AddToLocalObjectList(NetworkObject *object) {
    NetworkObject **slot = local_objects;
    if (local_object_count <= 0) {
        local_object_count = 1;
    } else if (*slot != NULL) {
        i32 index = 0;
        do {
            index++;
            slot++;
            if (index == local_object_count) {
                local_object_count = static_cast<i32>(slot - local_objects) + 1;
                break;
            }
        } while (*slot != NULL);
    }
    *slot = object;
}

void NetworkObjectManager::BindFilter(NOSFilter *filter, EdClass const *object_class) {
    filters[theRegistry.GetClassId(const_cast<EdClass *>(object_class))] = filter;
}

void NetworkObjectManager::BindReplicator(NetReplicator *, EdClass const *) {
}

void NetworkObjectManager::CalcReplicatorDataSize(NetReplicator *, EdClass const *, i32 &, i32 &) {
}

void NetworkObjectManager::ChangeContext(NOSContext &new_context) {
    memmove(&context, &new_context, sizeof(context));
}

void NetworkObjectManager::ConstructObject(NetworkObject *, NetworkObjectManager::NetPeerPush *) {
}

void NetworkObjectManager::ContinuityBreak(i32, float) {
}

NetworkObject *NetworkObjectManager::FindNetworkObject(void *object) {
    if (object == NULL) {
        return NULL;
    }
    for (i32 i = 0; i < 2048; i++) {
        if (objects[i].id != 0 && objects[i].object == object) {
            return &objects[i];
        }
    }
    return NULL;
}

void NetworkObjectManager::FlushObjects(i32) {
}

i32 NetworkObjectManager::GetNextGuid() {
    if (guid_group < 0) {
        return 0;
    }

    i32 group_start = guid_group << 10;
    i32 group_end = (guid_group + 1) << 10;
    if (next_guid < 0) {
        next_guid = group_start == 0 ? 0 : group_start - 1;
    }

    i32 attempts = 0;
    do {
        next_guid++;
        attempts++;
        if (next_guid >= group_end) {
            next_guid = group_start == 0 ? 1 : group_start;
        }
        if (objects[next_guid].object == NULL) {
            return next_guid;
        }
    } while (attempts != 1024);
    return 0;
}

void *NetworkObjectManager::GetObject(i32 id) {
    if (id < 1 || id > 2048) {
        return NULL;
    }
    return objects[id].object;
}

void NetworkObjectManager::GetPeerStatus() {
}

void NetworkObjectManager::ImportObjects() {
}

void NetworkObjectManager::Init() {
}

void NetworkObjectManager::InitClassStats() {
}

i32 NetworkObjectManager::IsLocal(i32 id) {
    if (id == 0) {
        return 1;
    }
    NetworkObject *network_object = FindNetworkObject(id);
    if (network_object == NULL) {
        return 1;
    }
    return network_object->owner->local;
}

void NetworkObjectManager::IsPeerReady(NetPeer const &) const {
}

void NetworkObjectManager::IsPeerStarted(NetPeer const &) const {
}

NetworkObjectManager::NetworkObjectManager() {
}

void NetworkObjectManager::NotifyCreateObject(void *object, EdClass *object_class, void *, i32, i32 guid, i32 flags) {
    if ((flags & 1) == 0) {
        RegisterObject(object, object_class, guid);
    }
}

void NetworkObjectManager::NotifyDestroyObject(void *object, EdClass *object_class, i32 guid, i32 flags) {
    if ((flags & 1) == 0) {
        ReleaseObject(object, object_class, guid);
    }
}

void NetworkObjectManager::ObjectCall(void *, i32, NetMessage, NetPeer const *) {
}

void NetworkObjectManager::ObjectOtherCall(void *, i32, NetMessage) {
}

void NetworkObjectManager::ObjectOwnerCall(void *, i32, NetMessage) {
}

NetPeer const *NetworkObjectManager::Owner(i32 id) {
    if (id == 0) {
        return NULL;
    }
    NetworkObject *network_object = FindNetworkObject(id);
    if (network_object == NULL) {
        return NULL;
    }
    return network_object->owner;
}

void NetworkObjectManager::PeerJoined(NetPeer const &) {
}

void NetworkObjectManager::PeerLeft(NetPeer const &, ePeerLeftReason) {
}

void NetworkObjectManager::Push(NetworkObject const *, NetReplicator *, ReplicatorData &,
                                NetworkObjectManager::NetPeerPush *) {
}

void NetworkObjectManager::PushObject(NetworkObject *, NetworkObjectManager::NetPeerPush *, i32) {
}

void NetworkObjectManager::Receive(NetMessage, unsigned char, NetPeer const &) {
}

void NetworkObjectManager::ReceiveAcquireMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveAcquiredMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveAdoptedMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveConstructorMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveContinuityBreak(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveObjectCallMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveReleaseMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveRemoteCallMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveReplicaMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveStartMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveStatusMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::ReceiveStopMessage(NetMessage &, NetPeer const &) {
}

void NetworkObjectManager::Recover(NetworkObject *) {
}

void NetworkObjectManager::RegisterObject(void *, EdClass *, i32) {
}

void NetworkObjectManager::RegisterObjectCall(void (*callback)(void *, NetMessage &), i32 id) {
    if (registered_call_count <= 31) {
        RegisteredCall &call = registered_calls[registered_call_count++];
        call.type = 1;
        call.callback = reinterpret_cast<void *>(callback);
        call.id = id;
    }
}

void NetworkObjectManager::RegisterRemoteCall(void (*callback)(NetMessage &), i32 id) {
    if (registered_call_count <= 31) {
        RegisteredCall &call = registered_calls[registered_call_count++];
        call.type = 0;
        call.callback = reinterpret_cast<void *>(callback);
        call.id = id;
    }
}

void NetworkObjectManager::ReleaseObject(void *, EdClass *, i32) {
}

void NetworkObjectManager::RemoteCall(i32, NetMessage, NetPeer const *) {
}

void NetworkObjectManager::RemoveFromLocalObjectList(NetworkObject *object) {
    NetworkObject **slot = local_objects;
    for (i32 i = 0; i < local_object_count; i++, slot++) {
        if (*slot == object) {
            *slot = NULL;
            return;
        }
    }
}

void NetworkObjectManager::RemovePendingObject(NetworkObject *object) {
    for (i32 i = 0; i < 32; i++) {
        if (pending_objects[i].object == object) {
            pending_objects[i].object = NULL;
            return;
        }
    }
}

void NetworkObjectManager::Reset() {
}

void NetworkObjectManager::SendAcquireMessage(NetworkObject *) {
}

void NetworkObjectManager::SendAcquiredMessage(i16, NetPeer const &) {
}

void NetworkObjectManager::SendAdoptedMessage(i16) {
}

void NetworkObjectManager::SendPushMessage(NetMessage *, NetworkObjectManager::NetPeerPush const *, i32) {
}

void NetworkObjectManager::Start(NOSContext const &) {
}

NetworkObjectManager::PendingObject *NetworkObjectManager::StealPendingObject() {
    for (i32 i = 0; i < 32; i++) {
        if (UtilGetFrameStartTime() > pending_objects[i].field_04 + 1000) {
            pending_objects[i].field_00 = 0;
            pending_objects[i].field_04 = 0;
            return &pending_objects[i];
        }
    }
    return NULL;
}

void NetworkObjectManager::Stop() {
}

void NetworkObjectManager::Term() {
}

void NetworkObjectManager::Update() {
}

void NetworkObjectManager::UpdateLocalObjectList() {
}

NetworkObjectManager::~NetworkObjectManager() {
}
