#ifndef NU2API_NUCORE_TYPES_H
#define NU2API_NUCORE_TYPES_H
#pragma once

#include "nu2api/nucore/fixed_width.h"
#include "nu2api/nucore/numem.h"

#include "nu2api/nucore/NuCopyFilter.h"
#include "nu2api/nucore/NuDataPortManager.h"
#include "nu2api/nucore/NuDeferredFilter.h"
#include "nu2api/nucore/NuDeferredFilterGen.h"
#include "nu2api/nucore/NuDeviceSpecs.h"
#include "nu2api/nucore/NuDynamicLight.h"
#include "nu2api/nucore/NuMainFilter.h"
#include "nu2api/nucore/NuMainFilterGen.h"
#include "nu2api/nucore/NuMotionAccumFilter.h"
#include "nu2api/nucore/NuMotionAccumFilterGen.h"
#include "nu2api/nucore/NuMotionFilter.h"
#include "nu2api/nucore/NuMotionFilterGen.h"
#include "nu2api/nucore/NuNetEmu.h"
#include "nu2api/nucore/NuPlatform.h"
#include "nu2api/nucore/NuPostFilter.h"
#include "nu2api/nucore/NuPostFilterGen.h"
#include "nu2api/nucore/NuSpeedBlurFilter.h"
#include "nu2api/nucore/NuSpeedBlurFilterGen.h"
#include "nu2api/nucore/NuTouchInputButton.h"
#include "nu2api/nucore/NuTouchInputElement.h"
#include "nu2api/nucore/NuTouchInputStick.h"
#include "nu2api/nucore/NuVoiceAndroid.h"
#include "nu2api/nucore/nugcutscene.h"

enum NUAPPLICATIONSTATUS : i32;
struct NUGCUTCHAR_s;
struct NUGCUTLOCATOR_s;
struct NUGCUTRIGID_s;
struct NUGCUTSCENE_s;
struct NUJOINTANIM_s;
enum NUPADMOTIONVALUE : u32;
struct NUPADTYPE;
struct NUTHREADCAFECORE;
struct NUTHREADXBOX360CORE;
struct NetSmallStats;
struct NuApplicationState;
struct NuButtonLayout;
struct NuCopyFilter;
struct NuDataPortManager;
struct NuDeferredFilter;
struct NuDeferredFilterGen;
struct NuDeviceSpecs;
struct NuDynamicLight;
struct NuFadeObjGType;
struct NuIOS_InAppProduct;
struct NuInputDevice;
struct NuInputDevicePS;
struct NuInputManager;
struct NuInputTouchData;
struct NuMainFilter;
struct NuMainFilterGen;
struct NuMemory;
struct NuMemoryManager;
struct NuMemoryPool;
struct NuMotionAccumFilter;
struct NuMotionAccumFilterGen;
struct NuMotionFilter;
struct NuMotionFilterGen;
struct NuMusic;
struct NuNetEmu;
struct NuPlatform;
struct NuPostFilter;
struct NuPostFilterGen;
struct NuRenderDevice;
struct NuSoundBuffer;
struct NuSoundSource;
struct NuSpeedBlurFilter;
struct NuSpeedBlurFilterGen;
struct NuSymbolQuery;
struct NuThread;
struct NuThreadBase;
struct NuThreadManager;
struct NuTouchInputButton;
struct NuTouchInputElement;
struct NuTouchInputStick;
struct NuVirtualTouchDevice;
struct NuVoiceAndroid;
struct NuWindGType;
struct SLPlayItf_;
struct ShaderObjectKey;
struct VuMtx;
struct VuVec;
struct _vuv_s;
struct ani3_animheader_s;
struct d3dsamplerstate_u;
struct nuanimbuff_s;
struct nuanimdatachunk_s;
struct nucamera_s;
struct nudeferredshadingenum_e;
struct nudisplaylistitem_s;
struct nudisplayscene_s;
struct nudynamiclight_s;
struct nueffecttex_s;
struct nufnt_s;
struct nuframebuffer_s;
struct nugeom_s;
struct nugscn_s;
struct nuhspecial_s;
struct numemblk_s;
struct numtl_s;
struct numtx_s;
struct nunativetex_s;
struct nunetaddr_s;
struct nupad_s;
struct nushaderprogram_s;
struct nutex_s;
struct nutexturetype_e;
struct nuvec4_s;
struct nuvec_s;
struct rndrstream_s;

enum NUAPPLICATIONSTATUS : i32;
struct NUGCUTCHAR_s;
struct NUGCUTSCENE_s;
enum NUPADMOTIONVALUE : u32;
struct NUPADTYPE {};
struct NUTHREADCAFECORE {};
struct NUTHREADXBOX360CORE {};
struct NuIOS_InAppProduct {};
struct NuInputTouchData;
struct NuSoundBuffer;
struct NuSoundSource;
struct NuSymbolQuery {};
#include "gamelib/nuwind/nuwind.h"
struct SLPlayItf_ {};
struct ShaderObjectKey;
struct VuMtx;
struct VuVec;
struct _vuv_s;
struct ani3_animheader_s;
struct d3dsamplerstate_u {};
struct nuanimbuff_s;
struct nuanimdatachunk_s {};
struct nucamera_s;
struct nudeferredshadingenum_e {};
struct nudisplaylistitem_s;
struct nudisplayscene_s;
struct nudynamiclight_s {};
struct nueffecttex_s;
struct nufnt_s {};
struct nuframebuffer_s {};
struct nugeom_s {};
struct nugscn_s;
struct nuhspecial_s;
struct numtl_s;
struct numtx_s;
struct nunativetex_s;
struct nunetaddr_s {};
struct nupad_s;
struct nutex_s;
struct nutexturetype_e {};
struct nuvec4_s;
struct nuvec_s;
struct rndrstream_s;

struct NuApplicationState {
    void SetStatus(NUAPPLICATIONSTATUS);
    ~NuApplicationState();
};
struct NuButtonLayout;
struct NuInputDevice {
    void DisableDPD();
    void EnableDPD();
    void GetAttachmentType() const;
    void GetCaps() const;
    void GetIndexByType() const;
    void GetLastValidIndexByType() const;
    void GetLastValidType() const;
    f32 GetMotionValue(NUPADMOTIONVALUE) const;
    void GetMouseData() const;
    void GetPort() const;
    void GetTouchData() const;
    void GetVolume() const;
    void HasHeadphonesConnected() const;
    void IsButtonPressed(u32) const;
    void IsIntercepted() const;
    void KillRumble();
    void ProcessTouchData();
    void SetMotors(float, float);
    void SupportsCaps(u32) const;
    ~NuInputDevice();
};
struct NuInputDevicePS {
    void DisableDPDPS(u32);
    void EnableDPDPS(u32);
    void GetIdentifierPS(u32);
    void HandleGamePadAxis_ANDROID_SPECIFIC(float, float, float, float, float, float);
    void HandleSensor_ANDROID_SPECIFIC(i32, float, float, float);
    i32 HandleTouch_ANDROID_SPECIFIC(i32, i32, i32, float, float);
};
struct NuInputManager {
    void GetDevice(u32) const;
    void GetFirstDeviceByType(NUPADTYPE) const;
    void KillRumbleAll();
};
struct NuMemoryPool {
    struct FreeBlock;
    struct IEventHandler {};
    struct IVisitor {};
    struct Page;
    u32 GetAllocatedBytes();
    const char *GetDebugName() const;
    u32 GetFreeBytes();
    u32 GetLargeBlockBytes();
    u32 GetPagedBytes();
    NuMemoryPool::FreeBlock volatile *InterlockedPop(NuMemoryPool::FreeBlock volatile **);
    void InterlockedPush(NuMemoryPool::FreeBlock volatile **, void *);
    NuMemoryPool::FreeBlock volatile *Merge(NuMemoryPool::FreeBlock volatile *, NuMemoryPool::FreeBlock volatile *);
    NuMemoryPool::Page *Merge(NuMemoryPool::Page *, NuMemoryPool::Page *);
    NuMemoryPool::FreeBlock volatile *MergeSort(NuMemoryPool::FreeBlock volatile *, u32);
    NuMemoryPool::Page *MergeSort(NuMemoryPool::Page *, u32);
    NuMemoryPool(NuMemoryPool::IEventHandler *, u32, char const *);
    void *PageAlloc(u32, char const *);
    void ReleaseAllPages();
    void ReleaseUnreferencedPages();
    void ReleaseUnreferencedPages_OLD();
    static void VisitPools(NuMemoryPool::IVisitor *);
    ~NuMemoryPool();
};
struct NuMemoryManager {
    struct Context;
    struct DebugHeader;
    struct ErrorCode {};
    struct FreeHeader;
    struct Header;
    struct IPageVisitor {};
    struct IVisitor {};
    void ClearBlockDebugContext(void *);
    void DumpBlock(u32, NuSymbolQuery *, NuMemoryManager::Header *, u32, u32, u32);
    void DumpBlocksForContext(u32, NuSymbolQuery *, NuMemoryManager::Context *, u32);
    void FindAndTouchMatchingBlocks(NuMemoryManager::DebugHeader *, u32 *, u32);
    void GetAllocatedBytes();
    u32 GetBlockAlignment(void *);
    u32 GetBlockDebugBackTrace(void *, void **);
    u32 GetBlockDebugContext(void *);
    u32 GetBlockSize(void *);
    u32 GetCategoryAllocatedBytes(u16);
    u32 GetCurrentContextID() const;
    const char *GetCurrentContextName() const;
    const char *GetDebugName() const;
    u32 GetFreeBytes() const;
    u32 GetNumFreeFragments() const;
    u16 GetOverrideCategory();
    u16 GetOverrideCategoryBGThread();
    u32 GetPagedBytes();
    static u32 GetSmallBinSize(u32);
    bool IsZombie();
    NuMemoryManager::FreeHeader *MergeLargeBinSegments(NuMemoryManager::FreeHeader *, NuMemoryManager::FreeHeader *);
    void PushContext(char const *);
    void ReleaseExternalPage(void *);
    void SetBlockDebugContext(void *, u32);
    void SetBlockDebugName(void *, char const *);
    void SetOverrideCategory(u16);
    void SetOverrideCategoryBGThread(u16);
    void SortLargeBin(u32);
    NuMemoryManager::FreeHeader *SortLargeBinSegment(NuMemoryManager::FreeHeader *, u32);
    void UnTouchAllBlocks();
    void ValidateBlock(void *);
    void ValidateBlockDeferredContent(NuMemoryManager::Header *, char const *);
    static void VisitManagers(NuMemoryManager::IVisitor *);
    void VisitPages(NuMemoryManager::IPageVisitor *);
    void *_BlockAlloc(u32, u32, u32, char const *, u16);
    void _MultiBlockAlloc(u32, u32, u32, void **, u32, char const *, u16);
    void BlockFree(void *, u32);
};
struct NuMemory {
    struct MemErrorHandler {
        void CloseDump(NuMemoryManager *, u32);
        void Dump(NuMemoryManager *, u32, char const *);
        void HandleError(NuMemoryManager *, NuMemoryManager::ErrorCode, char const *);
        void OpenDump(NuMemoryManager *, char const *, u32 &);
    };
    NuMemoryPool *CreateDynamicMemoryPool(u32, char const *);
    NuMemoryPool *CreateFixedMemoryPool(u32, u32, char const *);
    NuMemoryPool *CreateMemoryPool(NuMemoryPool::IEventHandler *, u32, char const *);
    void DestroyMemoryManager(NuMemoryManager *);
    void DestroyMemoryPool(NuMemoryPool *);
    NuMemoryManager *GetThreadMem();
    u32 MoveFreeMem2IntoMem1();
    void SetSoakTestMode();
};
NuMemory *NuMemoryGet();
class NuRenderDevice;
struct NuThread {
    void Resume();
    void SetDebugName(char const *);
};
struct NuThreadBase {
    const char *GetDebugName() const;
};
struct NuThreadManager {
    NuThread *CreateThreadSuspended(void (*)(void *), void *, i32, char const *, i32, NUTHREADCAFECORE,
                                    NUTHREADXBOX360CORE);
    void FreeTLS(i32);
    NuThread *GetMainThread();
};
struct NuVirtualTouchDevice {
    f32 GetAspectRatio();
    void Render();
    void SetCurrentLayoutIndex(u32);
};
#endif // NU2API_NUCORE_TYPES_H
