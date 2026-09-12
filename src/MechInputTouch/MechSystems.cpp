#include "MechInputTouch_types.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/items/base/apiobject.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/numtx.h"

#include <stddef.h>
#include <string.h>
#include <new>

extern void (*preRenderFlashingHack)(void);
extern void (*postRenderFlashingHack)(void);
void PreRenderFlashHack();
void PostRenderFlashHack();

u8 MechSystems::SkipTextScroll = 0;

// vtable for MechSystems @0x66b320 slot 0x08: returns the static name string
// @0x573c42 (from MechSystems::GetName @0x45def0).
char const *MechSystems::GetName() {
    return "MechSystems";
}

void MechSystems::Display(ThingRenderData *) {
    TouchUI().Render();
}

void MechSystems::EnterLevel(WORLDINFO_s *) {
    if (initialized == 0) {
        Init();
    }
    if (initialized != 0) {
        PlayerButton().SetupTargetIds();
    }
}

void MechSystems::ExitLevel(WORLDINFO_s *) {
    if (level_ui_elements[0] != NULL) {
        TouchUI().RemoveUIElement(*level_ui_elements[0]);
        delete level_ui_elements[0];
        level_ui_elements[0] = NULL;
    }
    if (level_ui_elements[1] != NULL) {
        TouchUI().RemoveUIElement(*level_ui_elements[1]);
        delete level_ui_elements[1];
        level_ui_elements[1] = NULL;
    }
    if (level_ui_elements[2] != NULL) {
        TouchUI().RemoveUIElement(*level_ui_elements[2]);
        delete level_ui_elements[2];
        level_ui_elements[2] = NULL;
    }
}

void MechSystems::FindMoveToMarkerAtPos(VuVec const &, bool) {
}

i32 MechInputTouchMenuController::AnyTouchesThisFrame = 0;
i32 MechInputTouchMenuController::PackButtonPressed = 0;
i32 MechInputTouchMenuController::PackButtonID = 0;

MechSystems *MechSystems::Get() {
    static MechSystems *instance = NULL;
    if (instance == NULL) {
        instance = new MechSystems();
    }
    return instance;
}

void MechSystems::HookUpClickToPressStart() {
    gesture_tracking_system.RegisterGestureTracker(ClickToPressStartTracker(), 0);
}

void MechSystems::Init() {
    if (initialized == 0) {
        input_touch_system.Init();
        TouchUI().Init();
        TouchUI().AddUIElement(PlayerButton());
        TouchUI().AddUIElement(PauseButton());
        initialized = 1;
        preRenderFlashingHack = PreRenderFlashHack;
        postRenderFlashingHack = PostRenderFlashHack;
    }
}

void MechSystems::LoadPerm() {
}

MechSystems::MechSystems() {
    new (ui_storage) MechTouchUI();
    new (player_button_storage) MechTouchUIPlayerButton();
    new (pause_button_storage) MechTouchUIPauseButton();
    initialized = 0;
    new (&click_to_press_start_tracker_storage) ClickToPressStartGestureTracker();
    unknown_0x10[4] = 0;
    for (i32 i = 0; i < 4; ++i) {
        unknown_0x10[i] = 0;
    }
    memset(move_to_markers, 0, sizeof(move_to_markers));
    swipe_markers[0] = NULL;
    swipe_markers[1] = NULL;
    swipe_markers[2] = NULL;
    swipe_markers[3] = NULL;
    level_ui_elements[0] = NULL;
    level_ui_elements[1] = NULL;
    level_ui_elements[2] = NULL;
    radar_pulses[0] = NULL;
    radar_pulses[1] = NULL;
    radar_pulses[2] = NULL;
    radar_pulses[3] = NULL;
}

void MechSystems::NewMoveToMarker(MechObjectInterface &) {
}

void MechSystems::NewRadarPulse(VuVec const &position, bool paused) {
    i32 slot;
    if (radar_pulses[0] == NULL) {
        slot = 0;
    } else if (radar_pulses[1] == NULL) {
        slot = 1;
    } else if (radar_pulses[2] == NULL) {
        slot = 2;
    } else if (radar_pulses[3] == NULL) {
        slot = 3;
    } else {
        return;
    }
    HudRadarPulse *pulse = new HudRadarPulse(position);
    radar_pulses[slot] = pulse;
    if (paused) {
        pulse->paused = 1;
    }
}

void MechSystems::NewSwipeMarker(TouchHolder &holder, i32 direction, SwipeDecalRenderer::Style style) {
    i32 slot;
    if (swipe_markers[0] == NULL) {
        slot = 0;
    } else if (swipe_markers[1] == NULL) {
        slot = 1;
    } else if (swipe_markers[2] == NULL) {
        slot = 2;
    } else if (swipe_markers[3] == NULL) {
        slot = 3;
    } else {
        return;
    }
    swipe_markers[slot] = new SwipeDecalRenderer(holder, direction, style);
}

MechTouchUITagButton *MechSystems::NewTagButton(GameObject_s &object, TouchHolder &holder) {
    if (level_ui_elements[0] != NULL && level_ui_elements[0]->target_object.Get() == object.GetMechObjectInterface()) {
        return NULL;
    }
    if (level_ui_elements[1] != NULL && level_ui_elements[1]->target_object.Get() == object.GetMechObjectInterface()) {
        return NULL;
    }
    if (level_ui_elements[2] != NULL && level_ui_elements[2]->target_object.Get() == object.GetMechObjectInterface()) {
        return NULL;
    }

    i32 slot;
    if (level_ui_elements[0] == NULL) {
        slot = 0;
    } else if (level_ui_elements[1] == NULL) {
        slot = 1;
    } else if (level_ui_elements[2] == NULL) {
        slot = 2;
    } else {
        return NULL;
    }
    level_ui_elements[slot] = new MechTouchUITagButton(object, holder);
    TouchUI().AddUIElement(*level_ui_elements[slot]);
    return level_ui_elements[slot];
}

void MechSystems::Process(ThingProcessData *) {
    if (initialized == 0) {
        Init();
    }
}

void MechSystems::ProcessEvenWhenPaused(ThingProcessData *data) {
    if (initialized == 0) {
        Init();
    }
    if (radar_pulses[0] != NULL) {
        radar_pulses[0]->Process(FRAMETIME);
        if (radar_pulses[0]->IsFinished()) {
            delete radar_pulses[0];
            radar_pulses[0] = NULL;
        }
    }
    if (radar_pulses[1] != NULL) {
        radar_pulses[1]->Process(FRAMETIME);
        if (radar_pulses[1]->IsFinished()) {
            delete radar_pulses[1];
            radar_pulses[1] = NULL;
        }
    }
    if (radar_pulses[2] != NULL) {
        radar_pulses[2]->Process(FRAMETIME);
        if (radar_pulses[2]->IsFinished()) {
            delete radar_pulses[2];
            radar_pulses[2] = NULL;
        }
    }
    if (radar_pulses[3] != NULL) {
        radar_pulses[3]->Process(FRAMETIME);
        if (radar_pulses[3]->IsFinished()) {
            delete radar_pulses[3];
            radar_pulses[3] = NULL;
        }
    }
    input_touch_system.ProcessEvenWhenPaused(data);
    TouchUI().Process(FRAMETIME);
}

void MechSystems::ProcessOnlyWhenPaused(ThingProcessData *) {
    if (initialized == 0) {
        Init();
    }
}

void MechSystems::Render(ThingRenderData *) {
    if (initialized != 0) {
        for (i32 i = 0; i < HIGHGAMEOBJECT; ++i) {
            if (Obj[i].touch_task != NULL) {
                Obj[i].touch_task->Render();
            }
        }
        for (i32 i = 0; i < 32; ++i) {
            if (move_to_markers[i] != NULL) {
                move_to_markers[i]->Render();
            }
        }
    }
    WORLDINFO *world = WorldInfo_CurrentlyActive();
    if (world != NULL && world->mech_auto_jump_manager != NULL) {
        world->mech_auto_jump_manager->Render();
    }
}

void MechSystems::RenderCurrentPlayerHighlight() {
    if (player != NULL && WORLD->lev_objs[166].active != 0) {
        NUVEC scale = {0.6f, 0.6f, 0.6f};
        NUVEC position = player->apiobj.position;
        position.y = GameShadow(player, &position, 2.0f, -1) + 0.001f;

        NUMTX matrix;
        NuMtxSetRotationY(&matrix, 0);
        NuMtxRotateX(&matrix, 0x4000);
        NuMtxScale(&matrix, &scale);
        NuMtxTranslate(&matrix, &position);
        NuSpecialDrawAtAlpha(&WORLD->lev_objs[166].special, &matrix, 0.25f);
    }
}

void MechSystems::Reset(ThingResetData *) {
}

void MechSystems::UnhookClickToPressStart() {
    gesture_tracking_system.UnregisterGestureTracker(ClickToPressStartTracker());
}

MechSystems::~MechSystems() {
    PauseButton().~MechTouchUIPauseButton();
    PlayerButton().~MechTouchUIPlayerButton();
    TouchUI().~MechTouchUI();
}
