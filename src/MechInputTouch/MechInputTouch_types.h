#ifndef MECHINPUTTOUCH_TYPES_H
#define MECHINPUTTOUCH_TYPES_H
#pragma once

#include "nu2api/nucore/fixed_width.h"
#include "nu2api/nucore/hashedkey.hpp"
#include "nu2api/nucore/numechptr.hpp"
#include "nu2api/nucore/nulist.h"
#include "legoapi/items/objects/basething.h"
#include "legoapi/render/core/SwipeDecalRenderer.h"
#include "nu2api/nucore/NuTouchInputElement.h"
#include "nu2api/nucore/nuvuvec.hpp"
#include "decomp.h"

struct AIPATHCNX_s;
struct AIPATH_s;
struct AISYS_s;
struct GameObject_s;
struct HudRadarPulse;
struct JumpTriggerPacket;
struct MechAddon;
struct MechAutoJumpConnection;
struct MechAutoJumpManager;
struct MechAutofireAddon;
struct MechEdgeStopAddon;
struct MechInputTouchBonusCavalryController;
struct MechInputTouchButton;
struct MechInputTouchButtonControlled;
struct MechInputTouchButtonFaker;
struct MechInputTouchDeathStarTurretController;
struct MechInputTouchGestureBasedController;
struct MechInputTouchGestureTracker;
struct MechInputTouchGestureTrackingSystem;
struct MechInputTouchMainController;
struct MechInputTouchMainDummyButton;
struct MechInputTouchMainDummyStick;
struct MechInputTouchMenuController;
struct MechInputTouchPodraceController;
struct MechInputTouchSpeederChaseController;
struct MechInputTouchSystem;
struct MechInputTouchVirtualConsoleController;
struct MechJumpAutoPilotAddon;
struct MechObjectInterface;
struct MechSystems;
struct MoveToMarker;
struct MechTempPosInterface;
struct MechTouchTask;
struct MechTouchTaskAstroJetPack;
struct MechTouchTaskAttack;
struct MechTouchTaskBigJump;
struct MechTouchTaskBlock;
struct MechTouchTaskBuildIt;
struct MechTouchTaskGoTo;
struct MechTouchTaskHatMachine;
struct MechTouchTaskJump;
struct MechTouchTaskPanel;
struct MechTouchTaskPlannedDoubleClickGoTo;
struct MechTouchTaskPlannedGoTo;
struct MechTouchTaskPullLever;
struct MechTouchTaskTag;
struct MechTouchTaskUseForce;
struct MechTouchTaskUseTeleport;
struct MechTouchTaskUseZipUp;
struct MechTouchUI;
struct MechTouchUICharIcon;
struct MechTouchUIElement;
struct MechTouchUIPartySelector;
struct MechTouchUIPauseButton;
struct MechTouchUIPlayerButton;
struct MechTouchUITagButton;
struct MechTouchUITexButton;
struct NuInputTouch;
struct NuInputTouchData;
struct NuTouchInputElement;
struct NuVec2;
struct NuVirtualTouchDevice;
struct SwipeDecalRenderer;
struct ThingProcessData;
struct ThingRenderData;
struct ThingResetData;
struct TouchHolder;
struct WORLDINFO_s;
struct nuvec_s;

struct AIPATHCNX_s;
struct AIPATH_s;
struct AISYS_s;
struct JumpTriggerPacket {};
struct MechAutoJumpVector {
    f32 x;
    f32 y;
    f32 z;
};
struct MechAutoJumpConnection {
    NULISTLNK link;
    AIPATH_s *path;
    AIPATHCNX_s *connection;
    i32 direction;
    u8 active;
    u8 use_path_direction;
    u8 is_using;
    u8 allow_streak;
    f32 cooldown;
    u32 base_colour;
    u32 colour;
    MechAutoJumpVector base_colour_components;
    f32 streak_alpha;
    void *streak_handle;
    f32 streak_reserved;
};
DECOMP_ASSERT(sizeof(MechAutoJumpConnection) == 0x3c, "MechAutoJumpConnection size");
struct NuVec2 {
    float x;
    float y;
};
struct TouchHolder {
    u8 field_0x0[0x0c];
    NuVec2 down_position;
    u8 field_0x14[0x18];
    NuVec2 touch_position;
};
struct MechInputTouchGestureTracker {
    virtual bool OnDown(GameObject_s &, TouchHolder &);
    virtual bool OnRelease(GameObject_s &, TouchHolder &);
    virtual bool OnClick(GameObject_s &, TouchHolder &);
    virtual bool OnDoubleClick(GameObject_s &, TouchHolder &);
    virtual bool OnHold(GameObject_s &, TouchHolder &);
    virtual bool OnSwipe(GameObject_s &, TouchHolder &, i32);
};
struct ClickToPressStartGestureTracker : MechInputTouchGestureTracker {
    bool OnClick(GameObject_s &, TouchHolder &) override;
};
typedef void (*MechTouchUICallback)(MechTouchUIElement &, TouchHolder &);
float GetAspectRatio();

struct MechTouchUIElement : NuMechPtr<MechTouchUIElement, 4>::ManagedBase {
    MechTouchUIElement()
        : position(), radius_x(0.0f), radius_y(0.0f), on_down(NULL), on_click(NULL), on_hold(NULL), on_release(NULL),
          on_leave(NULL), hovered(0), disabled(0), visible(1), rectangular(0), owner(NULL) {
    }
    MechTouchUIElement(VuVec const &pos, float radius)
        : position(pos), radius_x(GetAspectRatio() * radius), radius_y(radius), on_down(NULL), on_click(NULL),
          on_hold(NULL), on_release(NULL), on_leave(NULL), hovered(0), disabled(0), visible(1), owner(NULL) {
    }
    virtual ~MechTouchUIElement() {
    }
    virtual void Process(float);
    virtual void Render();

    VuVec position;
    float radius_x;
    float radius_y;
    MechTouchUICallback on_down;
    MechTouchUICallback on_click;
    MechTouchUICallback on_hold;
    MechTouchUICallback on_release;
    MechTouchUICallback on_leave;
    u8 hovered;
    u8 disabled;
    u8 visible;
    u8 rectangular;
    TouchHolder *owner;
};
struct NuInputTouch;
struct NuInputTouchData;
struct NuVirtualTouchDevice;
struct nupad_s;
struct ThingProcessData {
    float t; // frame time for this pass
    u32 paused;
    nupad_s **pads; // -> the caller's nupad_s* pair on its stack
    i32 flags;      // 2 = process pass
};
struct ThingRenderData {};
struct ThingResetData {};
struct WORLDINFO_s;
struct nuvec_s;

struct MechAddon {
    enum ProcessStage : i32 { PROCESS_STAGE_0 = 0 };
    enum RenderStage : i32 { RENDER_STAGE_0 = 0 };
    MechAddon(MechObjectInterface &object, u32 id) : target(&object), hash_id(id), next(NULL) {
    }
    virtual ~MechAddon() {
    }
    virtual void OnAdded() {
    }
    virtual void OnRemoved() {
    }
    virtual bool OnProcess(ProcessStage, f32) {
        return true;
    }
    virtual void OnRender(RenderStage) {
    }
    NuMechPtr<MechObjectInterface, 4> target;
    u32 hash_id;
    MechAddon *next;
};
DECOMP_ASSERT(sizeof(MechAddon) == 0x18, "MechAddon ABI");
DECOMP_ASSERT(offsetof(MechAddon, next) == 0x14, "MechAddon next offset");
DECOMP_ASSERT(sizeof(MechAddon::ProcessStage) == 4, "MechAddon process stage ABI");
struct MechAutoJumpManager : BaseThing {
    char const *GetName() override {
        return "MechAutoJumpManager";
    }
    MechAutoJumpConnection *AddAutoJumpConnection(AIPATH_s *, AIPATHCNX_s *, i32, bool, i32, bool);
    void DeleteJumpConnection(MechAutoJumpConnection *);
    void DeleteJumpConnectionsAndStreaks();
    void Init();
    MechAutoJumpManager(AISYS_s *);
    void PreProcessJumpConnections();
    void Process();
    void ProcessJumpConnections();
    void Render();
    virtual ~MechAutoJumpManager();

    NULISTHDR streaks;
    NULISTHDR jump_connections;
    AISYS_s *ai_sys;
    float streak_time;
};
DECOMP_ASSERT(sizeof(MechAutoJumpManager) == 0x28, "MechAutoJumpManager size");
struct MechAutofireAddon {
    MechAutofireAddon(MechObjectInterface &);
    void OnProcess(MechAddon::ProcessStage, float);
    virtual ~MechAutofireAddon();
};
struct MechEdgeStopAddon : MechAddon {
    static HashedKey s_hashId;
    MechEdgeStopAddon(MechObjectInterface &);
    bool OnProcess(MechAddon::ProcessStage, float) override;
    ~MechEdgeStopAddon() override;
    GameObject_s *character;
    f32 stop_timer;
    f32 jump_start_height;
    u8 was_jumping : 1;
};
DECOMP_ASSERT(sizeof(MechEdgeStopAddon) == 0x28, "MechEdgeStopAddon ABI");
DECOMP_ASSERT(offsetof(MechEdgeStopAddon, character) == 0x18, "MechEdgeStopAddon character offset");
DECOMP_ASSERT(offsetof(MechEdgeStopAddon, stop_timer) == 0x1c, "MechEdgeStopAddon timer offset");
DECOMP_ASSERT(offsetof(MechEdgeStopAddon, jump_start_height) == 0x20, "MechEdgeStopAddon height offset");
struct MechInputTouchBonusCavalryController {
    void Activate();
    void Deactivate();
    MechInputTouchBonusCavalryController(i32);
    void OnDown(GameObject_s &, TouchHolder &);
    void OnRelease(GameObject_s &, TouchHolder &);
    void Update(NuInputTouchData const *);
    virtual ~MechInputTouchBonusCavalryController();
};
struct MechInputTouchButton : NuTouchInputElement {
    void ClearTouchLocked(bool);
    bool CouldTouchBeLockedBy(u32);
    i32 FindPossibleTriggeringIndexFromID(u32);
    MechInputTouchButton(NuTouchInputElement::TYPE, i32, i32);
    MechInputTouchButton(i32, u32, float, float, float, float, i32);
    virtual ~MechInputTouchButton() {
    }
    virtual void Render();
    virtual void Update(NuInputTouchData const *);
    virtual void Reset();
    virtual char const *GetName();
    virtual char const *GetDebugText();
    void SetTouchLocked(u32, bool);

    bool possible_triggering_touches[10];
    u32 possible_triggering_touch_ids[10];
    float touch_history[44];
    u32 touch_locked_by;
    i32 button_id;
    bool has_pending_touches;
    bool is_pressed;
};
struct MechInputTouchButtonControlled : MechInputTouchButton {
    MechInputTouchButtonControlled(MechInputTouchMainController &, i32);
    virtual ~MechInputTouchButtonControlled() {
    }
    virtual void Reset();
    virtual bool ControlledUpdate(NuInputTouchData const *);
    virtual void ControlledRender();
    virtual void ControlledReset();

    i32 controller_index;
};
struct MechInputTouchButtonFaker {
    MechInputTouchButtonFaker(i32, u32, float, float, float, float);
    void Render();
    void Update(NuInputTouchData const *);
};
struct MechInputTouchDeathStarTurretController {
    void Activate();
    void Deactivate();
    MechInputTouchDeathStarTurretController(i32);
    void OnDown(GameObject_s &, TouchHolder &);
    void OnRelease(GameObject_s &, TouchHolder &);
    void OnSwipe(GameObject_s &, TouchHolder &, i32);
    void Update(NuInputTouchData const *);
    virtual ~MechInputTouchDeathStarTurretController();
};
struct MechInputTouchGestureTrackingSystem {
    void GetTouch(NuInputTouch const &);
    void LookForClicks(GameObject_s &);
    void LookForDown(GameObject_s &);
    void LookForGestures(GameObject_s &);
    void LookForHold(GameObject_s &);
    void LookForRelease(GameObject_s &);
    void LookForSwipe(GameObject_s &);
    MechInputTouchGestureTrackingSystem();
    void Process(GameObject_s &, NuInputTouchData const &);
    void ReadData(GameObject_s &, NuInputTouchData const &);
    void RegisterGestureTracker(MechInputTouchGestureTracker &, i32);
    void UnregisterGestureTracker(MechInputTouchGestureTracker &);
    void Update(NuInputTouchData const *);
    virtual ~MechInputTouchGestureTrackingSystem();
};
struct MechInputTouchMainController : NuTouchInputElement {
    enum eButtonTypes : u32 {};
    f32 stick_values[4];
    union {
        u32 buttons_pressed;
        u8 button_pressed[4];
    };
    union {
        u32 buttons_were_pressed;
        u8 button_was_pressed[4];
    };
    union {
        u32 buttons_repeat;
        u8 button_repeats[4];
    };
    float buttons_repeat_timers[4];
    float field_5c;
    float field_60;
    i32 player_id;
    u8 field_68;
    u8 pad_69[3];

    MechInputTouchMainController(i32);
    void RemoveUnpressedButtons(NuInputTouchData &, NuInputTouchData const &);
    void Render() override;
    void ResetButtons();
    void Update(NuInputTouchData const *) override;
    void UpdateButtons();
    virtual i32 GetPlayerId() const {
        return player_id;
    }
    ~MechInputTouchMainController() override;
};
DECOMP_ASSERT(sizeof(MechInputTouchMainController) == 0x6c, "Main touch controller ABI");
DECOMP_ASSERT(offsetof(MechInputTouchMainController, button_pressed) == 0x40, "Current touch buttons offset");
DECOMP_ASSERT(offsetof(MechInputTouchMainController, button_was_pressed) == 0x44, "Previous touch buttons offset");
DECOMP_ASSERT(offsetof(MechInputTouchMainController, button_repeats) == 0x48, "Repeated touch buttons offset");
DECOMP_ASSERT(offsetof(MechInputTouchMainController, buttons_repeat_timers) == 0x4c, "Touch repeat timers offset");

struct MechInputTouchMainDummyButton : NuTouchInputElement {
    MechInputTouchMainController *controller;
    u32 button_type;

    void Render() override {
    }
    void Update(NuInputTouchData const *) override {
    }
    u8 IsPressed() const override;
    MechInputTouchMainDummyButton(MechInputTouchMainController &, u32, MechInputTouchMainController::eButtonTypes);
};
struct MechInputTouchMainDummyStick : NuTouchInputElement {
    MechInputTouchMainController *controller;

    void Render() override {
    }
    void Update(NuInputTouchData const *) override {
    }
    float GetStickX() const override {
        return controller->stick_values[2];
    }
    float GetStickY() const override {
        return controller->stick_values[3];
    }
    virtual i32 GetPlayerId() const {
        return controller->GetPlayerId();
    }
    MechInputTouchMainDummyStick(MechInputTouchMainController &, NuTouchInputElement::TYPE);
};
struct MechInputTouchMenuController {
    static i32 AnyTouchesThisFrame; // original bss, consumed by startup/menu presentation
    static i32 PackButtonPressed;   // original bss (read/cleared by NuMain)
    static i32 PackButtonID;        // original bss (menu id for in-app purchase pack)
    void Activate();
    void Deactivate();
    MechInputTouchMenuController(i32);
    void OnClick(GameObject_s &, TouchHolder &);
    void OnDoubleClick(GameObject_s &, TouchHolder &);
    void OnDown(GameObject_s &, TouchHolder &);
    void OnHold(GameObject_s &, TouchHolder &);
    void OnRelease(GameObject_s &, TouchHolder &);
    void OnSwipe(GameObject_s &, TouchHolder &, i32);
    void Render();
    void Update(NuInputTouchData const *);
    void UpdateButtons(i32);
    virtual ~MechInputTouchMenuController();
};
struct MechInputTouchPodraceController {
    void Activate();
    void Deactivate();
    MechInputTouchPodraceController(i32);
    void OnDown(GameObject_s &, TouchHolder &);
    void OnRelease(GameObject_s &, TouchHolder &);
    void Update(NuInputTouchData const *);
    virtual ~MechInputTouchPodraceController();
};
struct MechInputTouchSpeederChaseController {
    void Activate();
    void Deactivate();
    void IsDownSwipe(NuVec2 const &, NuVec2 const &);
    void IsSwipeAgainstDirection(NuVec2 const &, NuVec2 const &, bool);
    void IsSwipeWithDirection(NuVec2 const &, NuVec2 const &, bool);
    void IsUpSwipe(NuVec2 const &, NuVec2 const &);
    MechInputTouchSpeederChaseController(i32);
    void OnClick(GameObject_s &, TouchHolder &);
    void OnDoubleClick(GameObject_s &, TouchHolder &);
    void OnDown(GameObject_s &, TouchHolder &);
    void OnRelease(GameObject_s &, TouchHolder &);
    void OnSwipe(GameObject_s &, TouchHolder &, i32);
    void Update(NuInputTouchData const *);
    virtual ~MechInputTouchSpeederChaseController();
};
struct MechInputTouchSystem {
    static i32 s_baseControlMode;
    static i32 s_actualTouchMode;
    virtual ~MechInputTouchSystem() {
    }
    virtual char const *GetName();
    void AddChangeLayoutButtons(NuVirtualTouchDevice &, i32);
    void ChooseTouchLayout(bool);
    static void ConvertToScreenCoords(float, float, float &, float &);
    bool CouldTouchBeLockedBy(u32, MechInputTouchButton *);
    void CreateGamePanels();
    void CreateGamePlayLayoutBlank(NuVirtualTouchDevice &, i32);
    void CreateGamePlayLayoutConsoleMode(NuVirtualTouchDevice &, i32);
    void CreateGamePlayLayoutGestureBased(NuVirtualTouchDevice &, i32);
    void CreateGamePlayLayoutGestureBased_Cavalry(NuVirtualTouchDevice &, i32);
    void CreateGamePlayLayoutGestureBased_DeathStarTurret(NuVirtualTouchDevice &, i32);
    void CreateGamePlayLayoutGestureBased_Podrace(NuVirtualTouchDevice &, i32);
    void CreateGamePlayLayoutGestureBased_SpeederChase(NuVirtualTouchDevice &, i32);
    static f32 DetermineMoveDir2D(GameObject_s &, VuVec const &, bool, VuVec &);
    static void FindTargetForce(WORLDINFO_s *, GameObject_s &, VuVec const &, VuVec const &, float &,
                                MechObjectInterface *&, bool &, bool);
    static MechObjectInterface *FindTargetObject(GameObject_s &, VuVec const &, i32, MechObjectInterface *,
                                                 MechTempPosInterface *);
    void Init();
    MechInputTouchSystem();
    virtual void ProcessEvenWhenPaused(ThingProcessData *);
    void ResetAllOwners();
    void SetTouchLockedBy(u32, MechInputTouchButton *, bool);
    MechInputTouchButton *TouchLockedBy(u32);

    i32 control_mode;
    u8 initialized;
    MechInputTouchButton *locked_buttons[10];
    u32 locked_touch_ids[10];
};
struct MechInputTouchVirtualConsoleController {
    static i16 s_textures[9];
    static float s_defaultDPadPosX;
    static float s_defaultDPadPosY;
    static float s_defaultButtonsPosX;
    static float s_defaultButtonsPosY;
    static float s_defaultDPadPosX_SmallScreen;
    static float s_defaultDPadPosY_SmallScreen;
    static float s_defaultButtonsPosX_SmallScreen;
    static float s_defaultButtonsPosY_SmallScreen;

    void Activate();
    void Deactivate();
    void LoadPerm();
    MechInputTouchVirtualConsoleController(i32);
    void OnDown(GameObject_s &, TouchHolder &);
    void OnRelease(GameObject_s &, TouchHolder &);
    void ProcessDragMovement(GameObject_s &);
    void ResetButtonPositionsToDefault();
    void ShouldBeActive();
    void Update(NuInputTouchData const *);
    void UpdateButtonPositions();
    void UpdateDPadPos();
    virtual ~MechInputTouchVirtualConsoleController();
};
struct MechJumpAutoPilotAddon {
    void AnalyseJumpTrajectory();
    void CalculateModifiedJumpTrajectory();
    void LookForBottomInt(VuVec const &);
    void LookForLandingPoint();
    void LookForLandingSpotAroundPoint(VuVec const &);
    void LookForTerrInt(VuVec const &);
    MechJumpAutoPilotAddon(MechObjectInterface &);
    void ModifyJump();
    void OnProcess(MechAddon::ProcessStage, float);
    void ProcJumpingToCertainDoom();
    void Recalculate();
    virtual ~MechJumpAutoPilotAddon();
};
struct GIZOBSTACLE_s;
struct GIZMOBLOWUP_s;
struct GIZFORCE_s;
struct GIZBUILDIT_s;
struct LEVER_s;
struct TELEPORT_s;
struct HATMACHINE_s;
struct GIZPANEL_s;
struct GIZTURRET_s;
struct PART_s;

struct MechObjectInterface : NuMechPtr<MechObjectInterface, 4>::ManagedBase {
    virtual ~MechObjectInterface() {
    }
    virtual void GetPos(VuVec &, i32) const {
    }
    virtual void GetFloorTargetPos(VuVec &, i32) const;
    virtual f32 GetRadius() const {
        return 0.0f;
    }
    virtual f32 GetHeight() const {
        const f32 radius = GetRadius();
        return radius + radius;
    }
    virtual const char *GetTargetName() const {
        return "";
    }
    virtual i32 GetObjectType() const {
        return 0;
    }
    virtual void TargetedFlash() {
    }
    virtual bool IsDead() {
        return 1;
    }
    virtual void *GetTgtVoidPtr() {
        return NULL;
    }
    virtual GameObject_s *GetCharacterObject() {
        return NULL;
    }
    virtual GIZOBSTACLE_s *GetGizObstacle() {
        return NULL;
    }
    virtual GIZMOBLOWUP_s *GetGizBlowup() {
        return NULL;
    }
    virtual GIZFORCE_s *GetGizForce() {
        return NULL;
    }
    virtual GIZBUILDIT_s *GetGizBuildit() {
        return NULL;
    }
    virtual LEVER_s *GetGizLever() {
        return NULL;
    }
    virtual TELEPORT_s *GetTeleport() {
        return NULL;
    }
    virtual HATMACHINE_s *GetHatMachine() {
        return NULL;
    }
    virtual GIZPANEL_s *GetPanel() {
        return NULL;
    }
    virtual GIZTURRET_s *GetGizTurret() {
        return NULL;
    }
    virtual PART_s *GetPart() {
        return NULL;
    }
};
DECOMP_ASSERT(sizeof(MechObjectInterface) == 8, "MechObjectInterface ABI");
DECOMP_ASSERT(sizeof(NuMechPtr<MechObjectInterface, 4>) == 12, "Mech object reference ABI");

struct MechAddonCollection {
    explicit MechAddonCollection(MechObjectInterface &object) : target(&object), first(NULL) {
    }
    ~MechAddonCollection() {
        MechAddon *addon = first;
        while (addon != NULL) {
            MechAddon *next = addon->next;
            addon->OnRemoved();
            delete addon;
            addon = next;
        }
    }
    virtual void Add(MechAddon &addon) {
        if (target.Get() != addon.target.Get())
            return;
        if (first != NULL) {
            for (MechAddon *current = first; current != NULL; current = current->next) {
                if (current == &addon)
                    return;
            }
            MechAddon *last = first;
            while (last->next != NULL)
                last = last->next;
            last->next = &addon;
        } else {
            first = &addon;
        }
        addon.OnAdded();
    }
    virtual void Remove(MechAddon &addon) {
        MechAddon *current = first;
        MechAddon *previous = NULL;
        while (current != NULL && current != &addon) {
            previous = current;
            current = current->next;
        }
        if (current != NULL) {
            if (previous != NULL)
                previous->next = current->next;
            else
                first = current->next;
            current->OnRemoved();
            delete current;
        }
    }
    virtual void Process(MechAddon::ProcessStage stage, f32 elapsed) {
        MechAddon *addon = first;
        while (addon != NULL) {
            MechAddon *next = addon->next;
            if (!addon->OnProcess(stage, elapsed))
                Remove(*addon);
            addon = next;
        }
    }
    virtual void Render(MechAddon::RenderStage stage) {
        // The original loop at 0x46f780 does not advance its current pointer.
        MechAddon *addon = first;
        while (addon != NULL)
            addon->OnRender(stage);
    }
    NuMechPtr<MechObjectInterface, 4> target;
    MechAddon *first;
};
DECOMP_ASSERT(sizeof(MechAddonCollection) == 0x14, "MechAddonCollection ABI");
DECOMP_ASSERT(offsetof(MechAddonCollection, first) == 0x10, "MechAddonCollection head offset");
// MechSystems is a BaseThing: AddOnceOnlyThings registers it on the
// GameThingManager and ProcessThings dispatches into it every frame.
// Virtual order = vtable for MechSystems @0x66b320 (rel slots):
//   0x08 GetName, 0x18 Reset, 0x1c Process, 0x20 ProcessEvenWhenPaused,
//   0x24 ProcessOnlyWhenPaused, 0x28 Render, 0x2c Display,
//   0x34 EnterLevel(WORLDINFO), 0x38 ExitLevel(WORLDINFO)
// (RemoveDependancies/EnterLevel(ThingLevelData)/ExitLevel(ThingLevelData)/
//  Effects keep the BaseThing slots).
struct MechSystems : BaseThing {
    static u8 SkipTextScroll;
    static MechSystems *Get();
    virtual ~MechSystems();
    char const *GetName() override;
    void Reset(ThingResetData *) override;
    void Process(ThingProcessData *) override;
    void ProcessEvenWhenPaused(ThingProcessData *) override;
    void ProcessOnlyWhenPaused(ThingProcessData *) override;
    void Render(ThingRenderData *) override;
    void Display(ThingRenderData *) override;
    virtual void EnterLevel(WORLDINFO_s *);
    virtual void ExitLevel(WORLDINFO_s *);
    void FindMoveToMarkerAtPos(VuVec const &, bool);
    void HookUpClickToPressStart();
    void Init();
    void LoadPerm();
    MechSystems();
    void NewMoveToMarker(MechObjectInterface &);
    void NewRadarPulse(VuVec const &, bool);
    void NewSwipeMarker(TouchHolder &, i32, SwipeDecalRenderer::Style);
    MechTouchUITagButton *NewTagButton(GameObject_s &, TouchHolder &);
    void RenderCurrentPlayerHighlight();
    void UnhookClickToPressStart();

    union {
        u32 unknown_0x10[6];
        struct {
            u32 reserved_10[3];
            struct numtl_s *radar_pulse_material;
            u32 reserved_20[2];
        };
    };
    MechInputTouchSystem input_touch_system;
    MechInputTouchGestureTrackingSystem gesture_tracking_system;
    u8 unknown_0x88[0x265c - 0x88];
    u8 ui_storage[0x84];
    u8 player_button_storage[0x164];
    u8 pause_button_storage[0x44];
    // Constructed after the UI controls, as in the original MechSystems ctor.
    u32 click_to_press_start_tracker_storage;
    MoveToMarker *move_to_markers[32];
    SwipeDecalRenderer *swipe_markers[4];
    MechTouchUITagButton *level_ui_elements[3];
    HudRadarPulse *radar_pulses[4];
    u8 initialized;
    u8 field_0x2939[3];

    MechTouchUI &TouchUI() {
        return *reinterpret_cast<MechTouchUI *>(ui_storage);
    }
    MechTouchUIPlayerButton &PlayerButton() {
        return *reinterpret_cast<MechTouchUIPlayerButton *>(player_button_storage);
    }
    MechTouchUIPauseButton &PauseButton() {
        return *reinterpret_cast<MechTouchUIPauseButton *>(pause_button_storage);
    }
    ClickToPressStartGestureTracker &ClickToPressStartTracker() {
        return *reinterpret_cast<ClickToPressStartGestureTracker *>(&click_to_press_start_tracker_storage);
    }
};
DECOMP_ASSERT(sizeof(ClickToPressStartGestureTracker) == sizeof(u32), "click-to-start tracker size");
DECOMP_ASSERT(offsetof(MechSystems, click_to_press_start_tracker_storage) == 0x2888, "click-to-start tracker offset");
DECOMP_ASSERT(offsetof(MechSystems, gesture_tracking_system) == 0x84, "MechSystems gesture tracking system offset");
DECOMP_ASSERT(offsetof(MechSystems, move_to_markers) == 0x288c, "MechSystems move markers offset");
DECOMP_ASSERT(offsetof(MechSystems, swipe_markers) == 0x290c, "MechSystems swipe marker slots offset");
DECOMP_ASSERT(offsetof(MechSystems, level_ui_elements) == 0x291c, "MechSystems level UI elements offset");
DECOMP_ASSERT(offsetof(MechSystems, radar_pulses) == 0x2928, "MechSystems radar pulse slots offset");
struct MechTempPosInterface : MechObjectInterface {
    VuVec position;
    f32 radius;
    MechTempPosInterface() : position(VuVec_Zero), radius(0.2f) {
    }
    void GetPos(VuVec &result, i32) const override {
        result.xyz = position.xyz;
    }
    void GetFloorTargetPos(VuVec &, i32) const override;
    f32 GetRadius() const override {
        return radius;
    }
    f32 GetHeight() const override {
        return 0.2f;
    }
    const char *GetTargetName() const override {
        return "(TEMPPOS)";
    }
    i32 GetObjectType() const override {
        return 1;
    }
    MechTempPosInterface(VuVec const &);
    MechTempPosInterface(nuvec_s const &);
};
DECOMP_ASSERT(sizeof(MechTempPosInterface) == 0x1c, "Temporary position interface size");
DECOMP_ASSERT(offsetof(MechTempPosInterface, position) == 8, "Temporary target position offset");
DECOMP_ASSERT(offsetof(MechTempPosInterface, radius) == 0x18, "Temporary target radius offset");

struct MechInputTouchGestureBasedController : MechInputTouchMainController, MechInputTouchGestureTracker {
    struct StickMode {
        u32 value;
    };
    void Activate();
    void Deactivate();
    void KillTasks(bool);
    MechInputTouchGestureBasedController(i32, MechInputTouchGestureBasedController::StickMode);
    bool MenuDisable();
    bool OnClick(GameObject_s &, TouchHolder &) override;
    bool OnDoubleClick(GameObject_s &, TouchHolder &) override;
    bool OnDown(GameObject_s &, TouchHolder &) override;
    bool OnHold(GameObject_s &, TouchHolder &) override;
    bool OnRelease(GameObject_s &, TouchHolder &) override;
    bool OnSwipe(GameObject_s &, TouchHolder &, i32) override;
    void PerformCloseMechanic(GameObject_s &, TouchHolder &);
    void ProcessAutoJumpOverGap(GameObject_s *);
    void ProcessAutoJumpWhenStuck(GameObject_s &);
    void ProcessDragMovement(GameObject_s &);
    void Render() override;
    void StartJumpUsingAIPath(JumpTriggerPacket const &, i32);
    void StartNewTask(MechTouchTask *, TouchHolder &, bool, bool);
    void TriggerJumpTask(JumpTriggerPacket const &, bool, bool, bool);
    void Update(NuInputTouchData const *) override;
    ~MechInputTouchGestureBasedController() override;

    MechTempPosInterface temporary_position;
    StickMode stick_mode;
    GameObject_s *field_90;
    GameObject_s *field_94;
    f32 field_98;
    f32 field_9c;
    f32 field_a0;
    u8 active;
    u8 field_a5;
    u8 field_a6;
    u8 pad_a7;
    MechTouchTask *current_task;
    NuMechPtr<MechObjectInterface, 4> target;
};
DECOMP_ASSERT(sizeof(MechInputTouchGestureBasedController) == 0xb8, "Gesture touch controller ABI");
DECOMP_ASSERT(offsetof(MechInputTouchGestureBasedController, temporary_position) == 0x70,
              "Gesture controller temporary target offset");
DECOMP_ASSERT(offsetof(MechInputTouchGestureBasedController, stick_mode) == 0x8c,
              "Gesture controller stick mode offset");
DECOMP_ASSERT(offsetof(MechInputTouchGestureBasedController, active) == 0xa4, "Gesture controller active flag offset");
DECOMP_ASSERT(offsetof(MechInputTouchGestureBasedController, current_task) == 0xa8,
              "Gesture controller current task offset");
DECOMP_ASSERT(offsetof(MechInputTouchGestureBasedController, target) == 0xac,
              "Gesture controller target reference offset");
struct MechTouchTask {
    MechTouchTask(MechInputTouchGestureBasedController &);
    virtual ~MechTouchTask();
    virtual const HashedKey &GetHashId() {
        return HashId;
    }
    virtual void OnStart() {
    }
    virtual void OnStop() {
    }
    virtual void OnSuspend() {
    }
    virtual void OnResume() {
    }
    virtual bool Update() {
        return false;
    }
    virtual void BackgroundProcess() {
    }
    virtual void Render() {
    }
    virtual void UpdateTarget(MechObjectInterface &) {
    }
    virtual i32 IsGoToTask() {
        return 0;
    }
    virtual bool IsBigJumpTask() {
        return false;
    }
    static HashedKey HashId;
    MechTouchTask *next;
    MechInputTouchGestureBasedController *controller;
    u32 field_0xc;
    f32 elapsed;
    u8 flags;
    u8 pad_15[3];
};
DECOMP_ASSERT(sizeof(MechTouchTask) == 0x18, "MechTouchTask ABI");
struct MechTouchTaskAstroJetPack {
    static HashedKey HashId;
    MechTouchTaskAstroJetPack(MechInputTouchGestureBasedController &);
    void Update();
};
struct MechTouchTaskAttack {
    static HashedKey HashId;
    MechTouchTaskAttack(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    void OnStart();
    void OnStop();
    void Render();
    void Update();
};
struct MechTouchTaskBigJump {
    static HashedKey HashId;
    MechTouchTaskBigJump(MechInputTouchGestureBasedController &, MechObjectInterface &, signed char);
    MechTouchTaskBigJump(MechInputTouchGestureBasedController &, nuvec_s &, signed char);
    void Update();
};
struct MechTouchTaskBlock : MechTouchTask {
    static HashedKey HashId;
    MechTouchTaskBlock(MechInputTouchGestureBasedController &);
    const HashedKey &GetHashId() override {
        return HashId;
    }
    bool Update() override;
};
struct MechTouchTaskGoTo : MechTouchTask {
    MechTouchTaskGoTo(MechInputTouchGestureBasedController &, MechObjectInterface *);
    const HashedKey &GetHashId() override {
        return HashId;
    }
    void OnStart() override;
    void OnStop() override;
    void Render() override;
    bool Update() override;
    void UpdateStuck();
    void UpdateTarget(MechObjectInterface &) override;
    i32 IsGoToTask() override {
        return 1;
    }
    virtual ~MechTouchTaskGoTo();
    static HashedKey HashId;
    NuMechPtr<MechObjectInterface, 4> target;
    u8 pad_24[8];
    i32 room;
    f32 field_30;
    f32 field_34;
    f32 field_38;
    f32 field_3c;
    f32 field_40;
    f32 field_44;
    f32 field_48;
    u8 field_4c;
    u8 field_4d;
    u8 field_4e;
    u8 field_4f;
    u8 field_50;
    u8 field_51;
    u8 pad_52[2];
    f32 field_54;
    f32 field_58;
    f32 field_5c;
};
DECOMP_ASSERT(sizeof(MechTouchTaskGoTo) == 0x60, "MechTouchTaskGoTo ABI");
struct MechTouchTaskBuildIt : MechTouchTaskGoTo {
    MechTouchTaskBuildIt(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    const HashedKey &GetHashId() override {
        return HashId;
    }
    bool Update() override;
    static HashedKey HashId;
};
struct MechTouchTaskHatMachine {
    static HashedKey HashId;
    MechTouchTaskHatMachine(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    void Update();
};
struct MechTouchTaskJump {
    static HashedKey HashId;
    MechTouchTaskJump(MechInputTouchGestureBasedController &, JumpTriggerPacket const &, bool, bool);
    void OnStop();
    void Update();
};
struct MechTouchTaskPanel {
    static HashedKey HashId;
    MechTouchTaskPanel(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    void Update();
};
struct MechTouchTaskPlannedDoubleClickGoTo : MechTouchTask {
    static HashedKey HashId;
    void BackgroundProcess() override;
    MechTouchTaskPlannedDoubleClickGoTo(MechInputTouchGestureBasedController &, MechObjectInterface *);
    const HashedKey &GetHashId() override {
        return HashId;
    }
    void OnResume() override;
    void OnStart() override;
    void OnStop() override;
    bool Update() override;
    ~MechTouchTaskPlannedDoubleClickGoTo() override;

    NuMechPtr<MechObjectInterface, 4> target;
    NuMechPtr<MechObjectInterface, 4> move_to_marker;
    MechTempPosInterface target_position;
    u8 field_4c;
    bool finished;
    u8 field_4e;
    u8 pad_4f;
};
struct MechTouchTaskPlannedGoTo {
    static HashedKey HashId;
    void AnalysePath();
    void BackgroundProcess();
    void GenerateWaypoints();
    MechTouchTaskPlannedGoTo(MechInputTouchGestureBasedController &, MechObjectInterface *, bool *);
    void OnResume();
    void OnStart();
    void OnStop();
    void SetupForAnalysis();
    void Update();
    virtual ~MechTouchTaskPlannedGoTo();
};
struct MechTouchTaskPullLever {
    static HashedKey HashId;
    MechTouchTaskPullLever(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    void Update();
};
struct MechTouchTaskTag {
    static HashedKey HashId;
    MechTouchTaskTag(MechInputTouchGestureBasedController &, GameObject_s &);
    void Update();
};
struct MechTouchTaskUseForce {
    static HashedKey HashId;
    MechTouchTaskUseForce(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    void OnStart();
    void OnStop();
    void Update();
};
struct MechTouchTaskUseTeleport {
    static HashedKey HashId;
    MechTouchTaskUseTeleport(MechInputTouchGestureBasedController &, MechObjectInterface *, VuVec const &);
    void Update();
};
struct MechTouchTaskUseZipUp : MechTouchTask {
    static HashedKey HashId;
    MechTouchTaskUseZipUp(MechInputTouchGestureBasedController &);
    const HashedKey &GetHashId() override {
        return HashId;
    }
    void OnStart() override;
    bool Update() override;
};
DECOMP_ASSERT(sizeof(MechTouchTaskBlock) == 0x18, "Block touch task ABI");
DECOMP_ASSERT(sizeof(MechTouchTaskUseZipUp) == 0x18, "Zip-up touch task ABI");
DECOMP_ASSERT(sizeof(MechTouchTaskPlannedDoubleClickGoTo) == 0x50, "Double-click touch task ABI");
DECOMP_ASSERT(offsetof(MechTouchTaskPlannedDoubleClickGoTo, finished) == 0x4d, "Double-click completion flag offset");
struct MechTouchUI : MechInputTouchGestureTracker {
    bool AddUIElement(MechTouchUIElement &);
    void Init();
    MechTouchUI();
    bool OnClick(GameObject_s &, TouchHolder &) override;
    bool OnDoubleClick(GameObject_s &, TouchHolder &) override;
    bool OnDown(GameObject_s &, TouchHolder &) override;
    bool OnHold(GameObject_s &, TouchHolder &) override;
    bool OnRelease(GameObject_s &, TouchHolder &) override;
    MechTouchUIElement *PickElement(NuVec2 &);
    void Process(float);
    bool RemoveUIElement(MechTouchUIElement &);
    void Render();
    ~MechTouchUI();

    MechTouchUIElement *elements[32];
};
struct MechTouchUICharIcon : MechTouchUIElement {
    MechTouchUICharIcon(MechTouchUIPartySelector &, VuVec const &, i32, float);
    void Process(float) override;
    void Render() override;
    void SetupDisabled();
    i32 character_id;
    f32 icon_scale;
    u8 selected;
    u8 field_0x45;
    u8 field_0x46;
    u8 field_0x47;
    f32 *alpha_target;
    f32 alpha_start;
    f32 alpha_end;
    f32 alpha_elapsed;
    f32 alpha_duration;
    f32 alpha_delay;
    f32 icon_alpha;
    MechTouchUIPartySelector *selector;
    u8 field_0x68[0x80 - 0x68];
};
struct MechTouchUIPartySelector {
    void BlendOut();
    bool BlendedOut();
    void Cleanup();
    MechTouchUIPartySelector(MechTouchUIPlayerButton &, i32 *);
    ~MechTouchUIPartySelector();
    i32 icon_count;
    MechTouchUICharIcon *icons[32];
    MechTouchUIPlayerButton *player_button;
    u8 field_0x88;
};
struct MechTouchUIPauseButton : MechTouchUIElement {
    MechTouchUIPauseButton();
    void Process(float) override;
    void Render() override;
    float disable_timer;
    float skip_prompt_timer;
};
struct MechTouchUIPlayerButton : MechTouchUIElement {
    MechTouchUIPlayerButton();
    void Process(float) override;
    void SetupTargetIds();
    void ShowChooser();
    void TriggerTagNext();
    union {
        MechTouchUIPartySelector *selector;
        void *panel_state;
    };
    u8 chooser_mode;
    u8 field_0x41[3];
    i32 target_ids[32];
    i32 free_play_target_ids[32];
    u8 field_0x144[0x164 - 0x144];
};
struct MechTouchUIAnimation {
    f32 *target;
    f32 from;
    f32 to;
    f32 elapsed;
    f32 duration;
    f32 delay;
    f32 value;
};
struct MechTouchUITagButton : MechTouchUIElement {
    void FadeOut();
    MechTouchUITagButton(GameObject_s &, TouchHolder &);
    void Process(float) override;
    void Render() override;
    ~MechTouchUITagButton() override;
    NuMechPtr<MechObjectInterface, 4> target_object;
    TouchHolder *touch_holder;
    MechTouchUIAnimation first_fade;
    MechTouchUIAnimation hover_animation;
    MechTouchUIAnimation size_animation;
    MechTouchUIAnimation second_fade;
    f32 field_0xbc;
    MechTouchUIAnimation timer_animation;
    f32 timer;
    u8 tag_state;
    u8 fading_out;
    u8 touched;
    u8 enabled;
};
struct MechTouchUITexButton : MechTouchUIElement {
    MechTouchUITexButton(VuVec const &, float);
    void Process(float) override;
    void Render() override;
    void UpdateTexture(i16);
    ~MechTouchUITexButton() override;
    struct numtl_s *material;
    f32 *alpha_target;
    f32 alpha_from, alpha_to, alpha_elapsed, alpha_duration, alpha_delay, alpha;
    f32 *scale_target;
    f32 scale_from, scale_to, scale_elapsed, scale_duration, scale_delay, scale;
};
DECOMP_ASSERT(sizeof(MechTouchUITexButton) == 0x78, "MechTouchUITexButton size");

DECOMP_ASSERT(sizeof(NuVec2) == 0x8, "NuVec2 size");
DECOMP_ASSERT(sizeof(MechInputTouchGestureTracker) == 0x4, "MechInputTouchGestureTracker size");
DECOMP_ASSERT(sizeof(MechTouchUIElement) == 0x3c, "MechTouchUIElement size");
DECOMP_ASSERT(sizeof(MechTouchUI) == 0x84, "MechTouchUI size");
DECOMP_ASSERT(sizeof(MechTouchUIPlayerButton) == 0x164, "MechTouchUIPlayerButton size");
DECOMP_ASSERT(offsetof(MechTouchUIPlayerButton, panel_state) == 0x3c, "Player button panel state offset");
DECOMP_ASSERT(offsetof(MechTouchUIPlayerButton, selector) == 0x3c, "player button selector offset");
DECOMP_ASSERT(offsetof(MechTouchUIPlayerButton, target_ids) == 0x44, "player button target ids offset");
DECOMP_ASSERT(offsetof(MechTouchUIPlayerButton, free_play_target_ids) == 0xc4,
              "player button free-play target ids offset");
DECOMP_ASSERT(sizeof(MechTouchUICharIcon) == 0x80, "MechTouchUICharIcon size");
DECOMP_ASSERT(sizeof(MechTouchUIPartySelector) == 0x8c, "MechTouchUIPartySelector size");
DECOMP_ASSERT(offsetof(MechTouchUIPartySelector, player_button) == 0x84, "party selector player button offset");
DECOMP_ASSERT(sizeof(MechTouchUIPauseButton) == 0x44, "MechTouchUIPauseButton size");
DECOMP_ASSERT(sizeof(MechTouchUITagButton) == 0xe4, "MechTouchUITagButton size");
DECOMP_ASSERT(offsetof(MechTouchUITagButton, target_object) == 0x3c, "tag target offset");
DECOMP_ASSERT(offsetof(MechTouchUITagButton, touch_holder) == 0x48, "tag holder offset");
DECOMP_ASSERT(offsetof(MechTouchUITagButton, first_fade) == 0x4c, "tag first fade offset");
DECOMP_ASSERT(offsetof(MechTouchUITagButton, second_fade) == 0xa0, "tag second fade offset");
DECOMP_ASSERT(offsetof(MechTouchUITagButton, second_fade.value) == 0xb8, "tag fade value offset");
DECOMP_ASSERT(offsetof(MechTouchUITagButton, fading_out) == 0xe1, "tag fade flag offset");
DECOMP_ASSERT(offsetof(MechInputTouchSystem, locked_buttons) == 0xc, "MechInputTouchSystem button offset");
DECOMP_ASSERT(offsetof(MechInputTouchSystem, locked_touch_ids) == 0x34, "MechInputTouchSystem touch id offset");
DECOMP_ASSERT(sizeof(MechInputTouchSystem) == 0x5c, "MechInputTouchSystem size");
DECOMP_ASSERT(sizeof(MechSystems) == 0x293c, "MechSystems size");

#endif // MECHINPUTTOUCH_TYPES_H
