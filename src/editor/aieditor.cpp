
#include "decomp.h"
#include "editor/edpath.h"
struct eduiitem_s;
struct eduimenu_s;

DECOMP_ASSERT(sizeof(aieditor_settings_s) == 0x200, "aieditor settings size");

aieditor_settings_s aieditorsettings;
extern "C" {
    i32 AIEDITOR_ROUTES = -1;
}
static __used__ void aieditor_cbSetEditorMode(eduimenu_s *, eduiitem_s *, unsigned int) {
}

static __used__ void aieditor_cbCancelSelectEditorMode(eduimenu_s *, eduimenu_s *) {
}

static __used__ void aieditor_cbCancelSaveMenu(eduimenu_s *, eduiitem_s *, unsigned int) {
}

extern "C" {

    typedef void AIEDITORMOVEPLAYERS(void);
    AIEDITORMOVEPLAYERS *AIEditorMovePlayersFn;

    void aieditor_SetCurrentScript(char *, i32);

    void InitFn_AIEditorMovePlayers(AIEDITORMOVEPLAYERS *function) {
        AIEditorMovePlayersFn = function;
    }

    void aieditor_AddMessage(void) {
    }

    void aieditor_ClearAllPathCnxTypes(void) {
    }

    void aieditor_ClearMainMenu(void) {
    }

    void aieditor_Enter(void) {
    }

    void aieditor_Init(void) {
    }

    void aieditor_Leave(void) {
    }

    void aieditor_Proc(void) {
    }

    void aieditor_Register(void) {
    }

    void aieditor_RegisterDefaultPathCnxTypes(void) {
    }

    void aieditor_RegisterPathCnxType(void) {
    }

    void aieditor_Render(void) {
    }

    void aieditor_Reset(void) {
        aieditorsettings.current_path_type = -1;
        aieditorsettings.show_creatures = 0;
        aieditorsettings.enabled = 0;
        aieditorsettings.snap_height = 0;
        aieditor_SetCurrentScript((char *)"default", 0);
    }

    void aieditor_Save(void) {
    }

    void aieditor_SetCurrentScript(char *, i32) {
    }

    void aieditor_SetMode(void) {
    }

    void aieditor_cbCancelMainMenu(void) {
    }

    void aieditor_cbDrawAllToggle(void) {
    }

    void aieditor_cbGoToPlayer(void) {
    }

    void aieditor_cbMovePlayer(void) {
    }

    void aieditor_cbSave(void) {
    }

    void aieditor_cbShowCreaturesSetToggle(void) {
    }

    void aieditor_cbShowCreaturesToggle(void) {
    }

    void aieditor_cbSnapHeightToggle(void) {
    }

    void aieditor_cbSolidAntinodeDisplayToggle(void) {
    }

    void aieditor_cbSolidPathDisplayToggle(void) {
    }

    void aieditor_cbStopPlatformsToggle(void) {
    }

    void aieditor_cvSelectEditorMode(void) {
    }

} // extern "C"
