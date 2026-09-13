#ifndef GAMELIB_UTIL_TYPES_H
#define GAMELIB_UTIL_TYPES_H
#pragma once

#include "nu2api/nucore/fixed_width.h"
#include "decomp.h"
#include <stddef.h>

struct AIPATHNODE_s;
struct AndroidOBBUtils;
struct BOLT_s;
struct CRC16;
struct EdClass;
struct EdStream;
struct FtpFile;
struct GIZFORCE_s;
struct GIZMOBLOWUP_s;
struct GameObject_s;
struct NOSContext;
struct NOSFilter;
struct NetAddress;
struct NetChangedReplicator;
struct NetConstReplicator;
struct NetFtpManager;
struct NetListenerBinding;
struct NetListenerInterface;
struct NetListenerList;
struct NetMessage;
struct NetPeer;
struct NetPredictor;
struct NetPredictor2;
struct NetPredictor3;
struct NetReplicator;
struct NetRotator2;
struct NetSample;
struct NetSimpleReplicator;
struct NetSmallStats;
struct NetStats;
struct NetTransporter;
struct NetworkObject;
struct NetworkObjectManager;
struct nucolour3_s;
struct NuFileDeviceAndroidOBBType;
struct ReplicatorData;
struct TouchHacks;
struct V2SessionManager;
struct VirtualStackAllocator;
struct VuVec;
struct WORLDINFO_s;
struct ePeerLeftReason;

struct AIPATHNODE_s;
struct BOLT_s;
struct EdClass;
struct EdStream;
struct GIZFORCE_s;
struct GIZMOBLOWUP_s;
struct GameObject_s;
struct NOSContext {
    u8 bytes[0x10];
};
struct NOSFilter {};
struct NetAddress {
    u32 value;
};
struct NetListenerInterface {};
struct NetPeer {
    u8 reserved_00[0xc];
    u8 local;
};
struct ReplicatorData {
    u8 reserved_00[8];
    u8 *cursor;
};
struct WORLDINFO_s;
struct ePeerLeftReason {};

struct NuFileDeviceAndroidOBBType {
    enum T { NONE = 0, MAIN = 1, PATCH = 2 };
};
struct AndroidOBBUtils {
    static char ms_packageName[3][512];
    static bool ms_initializedPackage[3];
    static bool ms_initializedPackageIsAsset[3];
    static void InitPackagePaths();
    static i32 LookupPackagePath(char *, NuFileDeviceAndroidOBBType::T);
    static i32 OpenFile(char const *);
};
struct CRC16 {
    CRC16();
    void hash(unsigned char const *, i32);
    void hashInverse(unsigned char const *, i32);
};
struct FtpFile {
    u8 reserved_00[8];
    i32 accepted;
    u8 reserved_0c[0x94];
    struct TransferReference {
        u8 reserved_00[0x4b0];
        i32 reference_count;
    };
    TransferReference *network_object; // 0xa0
    u8 reserved_a4[0x8];
    void *transfer; // 0xac
    i32 Accept();
    void Accept(i32);
    void Accept(i32, void *);
    void Init(i32, char const *, i32, NetAddress const &, void *, i32);
    void RecvData(NetMessage &);
    void SendData();
    void Term();
    void Update();
};
struct NetReplicator {
    static i16 smNextId;

    u32 list_previous;
    u32 list_next;
    u16 id;
    u16 replication_group;
    u16 minimum_interval;
    u16 maximum_interval;
    u16 field_14;
    u16 field_16;

    NetReplicator(i32, float, float);
    virtual bool AllowPush(EdClass const *, void const *, ReplicatorData &, i32, i32) = 0;
    virtual void SerialiseObject(EdStream &, NetPeer *, EdClass const *, void *, ReplicatorData &, i16 *);
    virtual void DoPrediction(EdClass const *, void *, ReplicatorData &, i32);
};
struct NetChangedReplicator : NetReplicator {
    static i16 mTableInited;
    static i16 mCrc32Table[256];

    bool AllowPush(EdClass const *, void const *, ReplicatorData &, i32, i32) override;
    void CheckSum(unsigned char const *, u32, u32 &) const;
    void CheckSumObject(EdClass const *, void const *, u32 &) const;
    void InitTable();
};
struct NetConstReplicator : NetReplicator {
    bool AllowPush(EdClass const *, void const *, ReplicatorData &, i32, i32) override;
};
struct NetFtpManager {
    FtpFile files[32];
    i32 Abort(char const *, NetAddress const &, i32, i32);
    void FindTransfer(char const *, NetAddress const &, i32);
    void FindTransfer(char const *, NetAddress const &, i32) const;
    i32 Get(char const *, void *, i32, NetAddress const &);
    FtpFile const *GetTransfers() const;
    void Init();
    NetFtpManager();
    void PeerLeft(NetAddress const &, ePeerLeftReason);
    void Receive(NetMessage, unsigned char, NetAddress const &);
    void Reset();
    i32 Send(char const *, void const *, i32, NetAddress const &);
    void Term();
    void Update();
    virtual ~NetFtpManager();
};
struct NetListenerBinding {
    NetListenerBinding(NetListenerInterface *, unsigned char, char *);
    void operator=(NetListenerBinding const &);
    void operator==(NetListenerBinding const &);
};
struct NetListenerList {
    void Find(NetListenerBinding *);
};
struct NetMessage {
    struct MessageData {
        u8 bytes[0x4b0];
        u32 references;
    };
    static MessageData sm_poolMessageData[512];
    i32 swap_endianness;
    MessageData *data;
    u32 read_offset;
    u32 write_offset;
    void DebugPrint() const;
    void RaiseError();
};
struct NetSession {
    u8 reserved_00[0x34];
    u32 error;
};
extern NetSession *theSession;
DECOMP_ASSERT(sizeof(NetMessage) == 0x10, "NetMessage ABI");
DECOMP_ASSERT(offsetof(NetMessage, data) == 4, "NetMessage data offset");
DECOMP_ASSERT(offsetof(NetMessage, read_offset) == 8, "NetMessage read cursor offset");
DECOMP_ASSERT(offsetof(NetMessage, write_offset) == 12, "NetMessage write cursor offset");
static_assert(sizeof(NetMessage::MessageData) == 0x4b4, "NetMessage pool entry size");
static_assert(offsetof(NetMessage::MessageData, references) == 0x4b0, "NetMessage pool reference offset");
struct NetPredictor : NetReplicator {
    struct PredictorData {};
    struct PredictorTime {};
    bool AllowPush(EdClass const *, void const *, ReplicatorData &, i32, i32) override;
    void CheckPredictionError(EdClass const *, void *, float *, float *, i32);
    void DoPrediction(EdClass const *, void *, ReplicatorData &, NetPredictor::PredictorTime *, i32);
    void DoPrediction(EdClass const *, void *, ReplicatorData &, i32) override;
    void SerialiseObject(EdStream &, NetPeer *, EdClass const *, void *, ReplicatorData &,
                         NetPredictor::PredictorTime *, i16 *);
    void SerialiseObject(EdStream &, NetPeer *, EdClass const *, void *, ReplicatorData &, i16 *) override;
    void StoreSampleData(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **,
                         float *, i32);
};
struct NetPredictor2 {
    void PredictValue(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **, float *,
                      i32);
};
struct NetPredictor3 {
    void PredictValue(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **, float *,
                      i32);
};
struct NetRotator2 {
    void PredictValue(EdClass const *, void *, NetPredictor::PredictorTime *, NetPredictor::PredictorData **, float *,
                      i32);
};
struct NetSample {
    i32 values[4];
    void Max(NetSample const &);
    void Reset();
    void operator+=(NetSample const &);
    void operator-=(NetSample const &);
};
struct NetSimpleReplicator : NetReplicator {
    bool AllowPush(EdClass const *, void const *, ReplicatorData &, i32, i32) override;
};
static_assert(sizeof(void *) != 4 || sizeof(NetReplicator) == 0x18, "NetReplicator 32-bit size");
struct NetSmallStats {
    struct eInfo {};
    void Draw(float, float, float, float, NetSmallStats::eInfo) const;
};
struct NetStats {
    void Draw(float, float, float, float, NetSmallStats::eInfo) const;
    void Update();
};
struct NetTransporter {
    u8 reserved_00[0x10c34];
    i32 replicator_data_sizes[64];
    u8 reserved_10d34[0x110f4 - 0x10d34];
    void AddListener(NetListenerInterface *, unsigned char, char *);
    void Distribute(NetMessage const &, unsigned char, NetPeer const &) const;
    void FtpComplete(FtpFile *, i32) const;
    void FtpDownload(FtpFile *) const;
    void FtpUpload(FtpFile *) const;
    void NosAcquire(NetworkObject *, NetPeer const &) const;
    void NosAdopted(NetworkObject *, NetPeer const &) const;
    void PeerDead(NetPeer const &) const;
    void PeerJoined(NetPeer const &) const;
    void PeerLeft(NetPeer const &, ePeerLeftReason) const;
    void PeerRequest(NetPeer const &) const;
    void RemoveListener(NetListenerInterface *, unsigned char);
    void StatsReceiveMessage(NetMessage, unsigned char);
    void StatsSendMessage(NetMessage, unsigned char);
    void StatsUpdate();
};
struct NetworkObject {
    u16 flags;
    i16 id;
    u32 reserved_04;
    NetPeer const *owner;
    void *object;
    EdClass *object_class;
    void *replicator_data;

    void Destroy();
    void Initialise(i32, void *, EdClass *, NetPeer const &, i32);
};
static_assert(sizeof(void *) != 4 || sizeof(NetworkObject) == 0x18, "NetworkObject 32-bit size");
struct NetworkObjectManager {
    // The manager reset routine is an intentional no-op in the original.
    struct NetPeerPush {
        void FlushMessages();
        void GetMessage(i32);
        void GetReliableMessage(i32);
        void NextStage();
        void Stop();
        void Sync();
    };
    void Acquire(i32);
    void AddToLocalObjectList(NetworkObject *);
    void BindFilter(NOSFilter *, EdClass const *);
    void BindReplicator(NetReplicator *, EdClass const *);
    void CalcReplicatorDataSize(NetReplicator *, EdClass const *, i32 &, i32 &);
    void ChangeContext(NOSContext &);
    void ConstructObject(NetworkObject *, NetworkObjectManager::NetPeerPush *);
    void ContinuityBreak(i32, float);
    NetworkObject *FindNetworkObject(i32);
    NetworkObject *FindNetworkObject(void *);
    struct PendingObject {
        u32 field_00;
        u32 field_04;
        NetworkObject *object;
    };

    PendingObject *FindPendingObject(NetworkObject *);
    void FlushObjects(i32);
    i32 GetGuid(void *);
    i32 GetNextGuid();
    void *GetObject(i32);
    void GetPeerStatus();
    void ImportObjects();
    void Init();
    void InitClassStats();
    i32 IsLocal(i32);
    void IsPeerReady(NetPeer const &) const;
    void IsPeerStarted(NetPeer const &) const;
    NetworkObjectManager();
    void NotifyCreateObject(void *, EdClass *, void *, i32, i32, i32);
    void NotifyDestroyObject(void *, EdClass *, i32, i32);
    void ObjectCall(void *, i32, NetMessage, NetPeer const *);
    void ObjectOtherCall(void *, i32, NetMessage);
    void ObjectOwnerCall(void *, i32, NetMessage);
    NetPeer const *Owner(i32);
    void PeerJoined(NetPeer const &);
    void PeerLeft(NetPeer const &, ePeerLeftReason);
    void Push(NetworkObject const *, NetReplicator *, ReplicatorData &, NetworkObjectManager::NetPeerPush *);
    void PushObject(NetworkObject *, NetworkObjectManager::NetPeerPush *, i32);
    void Receive(NetMessage, unsigned char, NetPeer const &);
    void ReceiveAcquireMessage(NetMessage &, NetPeer const &);
    void ReceiveAcquiredMessage(NetMessage &, NetPeer const &);
    void ReceiveAdoptedMessage(NetMessage &, NetPeer const &);
    void ReceiveConstructorMessage(NetMessage &, NetPeer const &);
    void ReceiveContinuityBreak(NetMessage &, NetPeer const &);
    void ReceiveObjectCallMessage(NetMessage &, NetPeer const &);
    void ReceiveReleaseMessage(NetMessage &, NetPeer const &);
    void ReceiveRemoteCallMessage(NetMessage &, NetPeer const &);
    void ReceiveReplicaMessage(NetMessage &, NetPeer const &);
    void ReceiveStartMessage(NetMessage &, NetPeer const &);
    void ReceiveStatusMessage(NetMessage &, NetPeer const &);
    void ReceiveStopMessage(NetMessage &, NetPeer const &);
    void Recover(NetworkObject *);
    void RegisterObject(void *, EdClass *, i32);
    void RegisterObjectCall(void (*)(void *, NetMessage &), i32);
    void RegisterRemoteCall(void (*)(NetMessage &), i32);
    void ReleaseObject(void *, EdClass *, i32);
    void RemoteCall(i32, NetMessage, NetPeer const *);
    void RemoveFromLocalObjectList(NetworkObject *);
    void RemovePendingObject(NetworkObject *);
    void Reset();
    void SendAcquireMessage(NetworkObject *);
    void SendAcquiredMessage(i16, NetPeer const &);
    void SendAdoptedMessage(i16);
    void SendPushMessage(NetMessage *, NetworkObjectManager::NetPeerPush const *, i32);
    void Start(NOSContext const &);
    PendingObject *StealPendingObject();
    void Stop();
    void Term();
    void Update();
    void UpdateLocalObjectList();
    virtual ~NetworkObjectManager();

    u8 reserved_04[0xc];
    NOSContext context;
    u8 reserved_20[8];
    i32 guid_group;
    i32 next_guid;
    NetworkObject objects[2048];
    i32 local_object_count;
    NetworkObject *local_objects[1024];
    PendingObject pending_objects[32];
    u8 reserved_d1b4[0x400];
    NOSFilter *filters[64];
    struct RegisteredCall {
        i32 type;
        void *callback;
        i32 id;
    } registered_calls[32];
    i32 registered_call_count;
};
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, objects) == 0x30,
              "NetworkObjectManager::objects 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, local_object_count) == 0xc030,
              "NetworkObjectManager::local_object_count 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, local_objects) == 0xc034,
              "NetworkObjectManager::local_objects 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, pending_objects) == 0xd034,
              "NetworkObjectManager::pending_objects 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, filters) == 0xd5b4,
              "NetworkObjectManager::filters 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, registered_calls) == 0xd6b4,
              "NetworkObjectManager::registered_calls 32-bit offset");
static_assert(sizeof(void *) != 4 || offsetof(NetworkObjectManager, registered_call_count) == 0xd834,
              "NetworkObjectManager::registered_call_count 32-bit offset");
struct TouchHacks {
    static bool TouchControlsActive;

    struct TintStack {
        float ambient[3];

        TintStack();
        ~TintStack();
    };
    static bool AiPlayerTakeDamageOnKillRescue(GameObject_s &);
    void CalculateJumpVelToHitPoint(GameObject_s &, VuVec const &);
    void CalculateJumpVelToHitPointDblJump(GameObject_s &, VuVec const &);
    void CalculateXZVelForArcToHitPoint(VuVec const &, VuVec const &, float, float);
    static i32 CanBlowupBeBlownUp(GIZMOBLOWUP_s &, i32);
    void CanForceTargetObj(GameObject_s &, GameObject_s &);
    static bool CanJump(GameObject_s &);
    void CanJumpToPoint(GameObject_s &, AIPATHNODE_s const &);
    void CanJumpToPoint(GameObject_s &, VuVec const &);
    static bool CanLunge(GameObject_s &);
    static bool CanPoo(GameObject_s &);
    static bool CanShoot(GameObject_s &);
    static bool CanSlam(GameObject_s &);
    void CanTagTo(GameObject_s &, GameObject_s &);
    static bool CanTagVehicle(GameObject_s &, GameObject_s &);
    void CanThrowBountyBomb(GameObject_s &);
    static bool CanToggleTo(GameObject_s &, i32);
    static bool CanUseBuildIt(GameObject_s &);
    static bool CanUseGizForce(GameObject_s &);
    static bool CanUseGizForce(GameObject_s &, GIZFORCE_s &);
    static bool CanUseHatMachine(GameObject_s &);
    static bool CanUseLever(GameObject_s &);
    static bool CanUseTeleport(GameObject_s &);
    static bool CanUseVehicleSmartBomb(GameObject_s &);
    static bool CanUseZipup(GameObject_s &);
    static bool CheckForAboutToRunIntoKillTerrain(GameObject_s &, float);
    void CheckForAboutToRunOffAnEdge(GameObject_s &, float);
    void CheckJumpForLandingSpot(GameObject_s &, float);
    static void CleanupAllMechObjectInterfaces(WORLDINFO_s *);
    void FindBombTarget(GameObject_s &);
    static nucolour3_s *GetFlashColour();
    static float GetIncomingPartRange();
    static i32 GetLoseStudsDieValue();
    static i32 GetLoseStudsFallValue();
    bool InParty(GameObject_s &);
    void PlaySmartBombBuildupEffects(GameObject_s &, float, float);
    static bool ShouldAutoGrabDragBomb(GameObject_s &);
    static bool ShouldBlock(GameObject_s &);
    static i32 ShouldDeflectBolt(GameObject_s &, BOLT_s &);
    static bool ShouldFlash(float);
    static bool ShouldKeepWeaponOut(GameObject_s &);
    static bool ShouldPutWeaponAway(GameObject_s &);
    bool SolveRoot(float, float, float, float &, float &);
    void TriggerVehicleSmartBomb(GameObject_s &);
};
struct V2SessionManager {
    u8 reserved_00[0x4];
    i32 field_04;
    u8 reserved_08[0x2c - 0x8];
    i32 field_2c;
    u8 reserved_30[0x34 - 0x30];
    i32 field_34;
    u8 reserved_38[0x44 - 0x38];
    i32 fields_44[9];
    i32 field_68;
    i32 field_6c;
    u8 reserved_70[0x74 - 0x70];
    i32 field_74;
    u8 reserved_78[0x90 - 0x78];
    i32 field_90;
    u8 reserved_94[0x9c - 0x94];
    i32 field_9c;
    void Log(char *, ...);
    void RemoveAllPeers(ePeerLeftReason);
    void RemovePeer(NetPeer *, ePeerLeftReason);
    void Reset();
    void SetHostGameData(i32 *, i32);
    void Update();
    V2SessionManager(char *);
    void VerifyStrings(char **, char **, i32, char *);
};
struct VirtualStackAllocator {
    u8 owns_memory;
    u8 *cursor;
    u8 *end;
    u8 *base;

    VirtualStackAllocator();
    VirtualStackAllocator(VirtualStackAllocator &, u32);
    VirtualStackAllocator(i32);
    VirtualStackAllocator(void *, u32);
    void setExternalMemoryPool(void *, u32);
    ~VirtualStackAllocator();
};

#endif // GAMELIB_UTIL_TYPES_H
