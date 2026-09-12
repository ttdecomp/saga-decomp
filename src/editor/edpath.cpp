#include "decomp.h"
#include "editor/edpath.h"
#include <string.h>
#include <stdio.h>

#include "gameapi/edtools/edui.h"
#include "gameapi/edtools/edcam.h"
#include "nu2api/nucore/nupad.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/nucore/nustring.h"
extern "C" {
    extern void *ed_fnt;
    extern i32 AIEDITOR_ROUTES;
    extern i32 near_clip_at_cursor;
    extern f32 default_path_heighttol;
    void creatureEditor_PathNodeMoved(EDAIPATHNODE_s *);
    void locatorEditor_PathNodeMoved(EDAIPATHNODE_s *);
    eduiitem_s *eduiItemSelCreate(i32, const void *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    eduiitem_s *eduiItemToggleCreate(i32, const void *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    eduiitem_s *eduiItemTextPickCreate(i32, const void *, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    eduiitem_s *eduiItemCheckCreate(i32, const void *, i32, i32, void (*)(eduimenu_s *, eduiitem_s *, u32), char *);
    void aieditor_cbCancelMainMenu(eduimenu_s *, eduimenu_s *);
    void aieditor_cvSelectEditorMode(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbSave(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbGoToPlayer(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbMovePlayer(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbSolidPathDisplayToggle(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbStopPlatformsToggle(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbDrawAllToggle(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbSnapHeightToggle(eduimenu_s *, eduiitem_s *, u32);
    void aieditor_cbShowCreaturesToggle(eduimenu_s *, eduiitem_s *, u32);
    void cbNearClipAtCursor(eduimenu_s *, eduiitem_s *, u32);
    NUVEC edpath_addoffset;
    f32 default_path_node_radius = .25f;
    extern char *(*SpecialRouteCharacterNameFn)(u8);
}
struct AIPATHCNXTYPE {
    u32 flags;
    u32 unknown_4;
    char name[0x44];
};
static AIPATHCNXTYPE aipathcnxtypes[32];
static i32 naipathcnxtypes;
static u32 attr[4] = {0x80000000, 0x80ff0000, 0x80808080, 0x80404040};
static void DestroyAIPathNode(EDAIPATHNODE_s *, EDAIPATH_s *);
extern "C" void aieditor_ClearMainMenu(void);

struct EDAISHAREDPATHNODE_s;
struct AIPATH_s;
struct eduimenu_s;
struct eduiitem_s;
struct nuvec_s;
struct nupad_s;

struct EdUiNameInputItem : eduiitem_s {
    u32 unknown_48;
    char name[0x40];
    u8 unknown_08c[0x15a - 0x8c];
    i16 max_name_length;
};
DECOMP_ASSERT(offsetof(EdUiNameInputItem, name) == 0x4c, "editor name input offset");
DECOMP_ASSERT(offsetof(EdUiNameInputItem, max_name_length) == 0x15a, "editor name input limit offset");


static __used__ void ParseAIPathCnxFlag(char *) {
}

static __used__ void pathEditorDrawPath(EDAIPATH_s *, i32) {
}

static __used__ void TestPointPathCheck(nuvec_s *, EDAIPATHNODE_s *, EDAIPATHNODE_s *, f32 *, f32 *, i32 *, f32) {
}

static __used__ void pathEditor_cbCreatePath(eduimenu_s *, eduiitem_s *, u32) {
    EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->free_paths);
    if (path == nullptr) {
        return;
    }
    NuLinkedListRemove(&aieditor->free_paths, &path->link);
    NuLinkedListAppend(&aieditor->paths, &path->link);
    char name[16];
    i32 number = 0;
    EDAIPATH_s *existing;
    do {
        sprintf(name, "NewPath%d", ++number);
        existing = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
        while (existing != nullptr && NuStrICmp(name, existing->name) != 0) {
            existing = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &existing->link);
        }
    } while (existing != nullptr);
    strcpy(path->name, name);
    path->flags &= ~1;
    aieditor->current_path = path;
    aieditor_ClearMainMenu();
}

static __used__ void pathEditor_cbDeletePath(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void pathEditor_cbRenameNode(eduimenu_s *, eduiitem_s *item, u32) {
    EDAIPATH_s *path = aieditor->current_path;
    if (path == nullptr || path->current_node == nullptr) {
        return;
    }
    EdUiNameInputItem *input = static_cast<EdUiNameInputItem *>(item);
    if (input->name[0] == '\0') {
        memset(path->current_node->name, 0, sizeof(path->current_node->name));
        return;
    }
    EDAIPATHNODE_s *node = (EDAIPATHNODE_s *)NuLinkedListGetHead(&path->nodes);
    while (node != nullptr) {
        if (NuStrICmp(node->name, input->name) == 0) {
            return;
        }
        node = (EDAIPATHNODE_s *)NuLinkedListGetNext(&aieditor->current_path->nodes, &node->link);
    }
    strcpy(aieditor->current_path->current_node->name, input->name);
}

static __used__ void pathEditor_cbRenamePath(eduimenu_s *, eduiitem_s *item, u32) {
    EDAIPATH_s *current = aieditor->current_path;
    EdUiNameInputItem *input = static_cast<EdUiNameInputItem *>(item);
    if (current == nullptr || input->name[0] == '\0') {
        return;
    }
    EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
    while (path != nullptr) {
        if (NuStrICmp(path->name, input->name) == 0) {
            return;
        }
        path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
    }
    strcpy(aieditor->current_path->name, input->name);
}

static __used__ void pathEditor_cbCancelRenamePathMenu(eduimenu_s *, eduimenu_s *);
static __used__ void pathEditor_cbCancelRenameNodeMenu(eduimenu_s *, eduimenu_s *);
static __used__ void pathEditor_cbCancelSelectMenu(eduimenu_s *, eduimenu_s *);
static __used__ void pathEditor_cbSetCurrentPath(eduimenu_s *, eduiitem_s *, u32);
static __used__ void pathEditor_cbSetShareNode(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void pathEditor_cbShareNodeMenu(eduimenu_s *parent, eduiitem_s *, u32) {
    EDAIPATH_s *current = aieditor->current_path;
    if (current == nullptr || current->current_node == nullptr) {
        return;
    }
    eduimenu_s *menu = eduiMenuCreate(0xdc, 0x46, 0xf0, 0xfa, ed_fnt,
                                     pathEditor_cbCancelSelectMenu, (char *)"Share node with...");
    if (menu == nullptr) {
        return;
    }
    eduiitem_s *all_paths = eduiItemSelCreate(0, &attr, 0, 1, pathEditor_cbSetShareNode, (char *)"All paths");
    eduiMenuAddItem(menu, all_paths);

    i32 index = 1;
    EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
    while (path != nullptr) {
        if (path != aieditor->current_path) {
            i32 selected = 0;
            EDAISHAREDPATHNODE_s *shared = aieditor->current_path->current_node->shared_node;
            if (shared != nullptr) {
                EDAIPATHNODE_s *node = (EDAIPATHNODE_s *)NuLinkedListGetHead(&path->nodes);
                while (node != nullptr) {
                    if (node->shared_node == shared) {
                        selected = 1;
                        break;
                    }
                    node = (EDAIPATHNODE_s *)NuLinkedListGetNext(&path->nodes, &node->link);
                }
            }
            eduiitem_s *item = eduiItemToggleCreate(index, &attr, selected, index + 1,
                                                     pathEditor_cbSetShareNode, path->name);
            eduiMenuAddItem(menu, item);
            ++index;
        }
        path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
    }
    eduiMenuAttach(parent, menu);
}

static __used__ void pathEditorCalcRouteIterator(AIPATH_s *, f32 *, u8 *, i32, i32, f32, i32) {
}

static __used__ void pathEditor_cbCnxFlagsToggle(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void pathEditor_cbDeletePathNode(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void pathEditor_cbRenameNodeMenu(eduimenu_s *parent, eduiitem_s *, u32) {
    EDAIPATH_s *path = aieditor->current_path;
    if (path == nullptr || path->current_node == nullptr) {
        return;
    }
    eduimenu_s *menu = eduiMenuCreate(0xf0, 0x5a, 0xf0, 0xfa, ed_fnt,
                                     pathEditor_cbCancelRenameNodeMenu, (char *)"Rename Node");
    if (menu == nullptr) {
        return;
    }
    eduiitem_s *item = eduiItemTextPickCreate(0, &attr, pathEditor_cbRenameNode, (char *)"Node Name");
    eduiMenuAddItem(menu, item);
    EdUiNameInputItem *input = static_cast<EdUiNameInputItem *>(edui_last_item);
    strcpy(input->name, aieditor->current_path->current_node->name);
    input->max_name_length = 15;
    eduiMenuAttach(parent, menu);
    menu->x = parent->x + 10;
    menu->y = parent->y + 40;
}

static __used__ void pathEditor_cbRenamePathMenu(eduimenu_s *parent, eduiitem_s *, u32) {
    EDAIPATH_s *path = aieditor->current_path;
    if (path == nullptr || (path->flags & 1) != 0) {
        return;
    }
    eduimenu_s *menu = eduiMenuCreate(0xf0, 0x5a, 0xf0, 0xfa, ed_fnt,
                                     pathEditor_cbCancelRenamePathMenu, (char *)"Rename Path");
    if (menu == nullptr) {
        return;
    }
    eduiitem_s *item = eduiItemTextPickCreate(0, &attr, pathEditor_cbRenamePath, (char *)"Path Name");
    eduiMenuAddItem(menu, item);
    EdUiNameInputItem *input = static_cast<EdUiNameInputItem *>(edui_last_item);
    strcpy(input->name, aieditor->current_path->name);
    input->max_name_length = 15;
    eduiMenuAttach(parent, menu);
    menu->x = parent->x + 10;
    menu->y = parent->y + 40;
}

static __used__ void pathEditor_cbSelectPathMenu(eduimenu_s *parent, eduiitem_s *, u32) {
    eduimenu_s *menu = eduiMenuCreate(0xdc, 0x46, 0xf0, 0xfa, ed_fnt,
                                     pathEditor_cbCancelSelectMenu, (char *)"Select Path");
    if (menu == nullptr) {
        return;
    }
    i32 index = 0;
    EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
    while (path != nullptr) {
        eduiitem_s *item = eduiItemSelCreate(index, &attr, 0, 0, pathEditor_cbSetCurrentPath, path->name);
        eduiMenuAddItem(menu, item);
        ++index;
        path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
    }
    eduiMenuAttach(parent, menu);
}

static __used__ void pathEditor_cbSetCurrentPath(eduimenu_s *, eduiitem_s *item, u32) {
    if (item != nullptr) {
        EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
        i32 index = 0;
        while (path != nullptr && index < item->data) {
            path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
            ++index;
        }
        if (path != nullptr && item->data >= 0) {
            aieditor->current_path = path;
            EDAIPATHNODE_s *nearest = nullptr;
            f32 best_distance = 3.402823466e38f;
            EDAIPATHNODE_s *node = (EDAIPATHNODE_s *)NuLinkedListGetHead(&path->nodes);
            while (node != nullptr) {
                NUVEC difference;
                f32 distance = NuVecXZDistSqr(&aieditor->selection_position, &node->position, &difference);
                if (distance < best_distance) {
                    f32 height = aieditor->selection_position.y - node->position.y;
                    f32 upper = NuFmax(0.2f, node->upper_height);
                    f32 lower = NuFmin(-0.2f, node->lower_height);
                    if (height <= upper && height >= lower) {
                        best_distance = distance;
                        nearest = node;
                    }
                }
                node = (EDAIPATHNODE_s *)NuLinkedListGetNext(&path->nodes, &node->link);
            }
            path->other_node = nearest;
            path->current_node = nearest;
            if (nearest != nullptr) {
                edcamSetPos(&nearest->position);
            }
        }
    }
    aieditor_ClearMainMenu();
}

static __used__ void pathEditor_cbNodeFlagsToggle(eduimenu_s *, eduiitem_s *item, u32) {
    EDAIPATHNODE_s *node = aieditor->current_path->current_node;
    if (node != nullptr) {
        u32 flags = node->flags;
        u32 mask = item->data;
        if ((u8)flags & mask) {
            flags &= ~mask;
        } else {
            flags |= mask;
        }
        node->flags = flags;
        if ((i8)aieditor->current_path->current_node->flags < 0) {
            node->platform.scene = nullptr;
            node->platform.special = nullptr;
            node->platform.display_special = nullptr;
        }
    }
}

static __used__ void pathEditor_cbCancelSelectMenu(eduimenu_s *, eduimenu_s *menu) {
    eduiMenuDestroy(menu);
}

static __used__ void pathEditor_cbDisconnectPathNode(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void pathEditorCalculateDistanceTable(AIPATH_s *, i32, variptr_u *, variptr_u *) {
}

static __used__ void pathEditor_cbCancelDeleteAreaMenu(eduimenu_s *, eduimenu_s *) {
    aieditor_ClearMainMenu();
}

static __used__ void pathEditor_cbCancelDeleteNodeMenu(eduimenu_s *, eduimenu_s *) {
    aieditor_ClearMainMenu();
}

static __used__ void pathEditor_cbCancelRenameNodeMenu(eduimenu_s *, eduimenu_s *menu) {
    eduiMenuDestroy(menu);
}

static __used__ void pathEditor_cbCancelRenamePathMenu(eduimenu_s *, eduimenu_s *menu) {
    eduiMenuDestroy(menu);
}

static __used__ void pathEditor_cbDrawWallsplinesToggle(eduimenu_s *, eduiitem_s *item, u32) {
    aieditorsettings.draw_wallsplines = item->highlighted;
}

static __used__ void pathEditor_cbCancelDeleteCreatureMenu(eduimenu_s *, eduimenu_s *) {
    aieditor_ClearMainMenu();
}

static __used__ void pathEditor_cbCancelDisconnectNodeMenu(eduimenu_s *, eduimenu_s *) {
    aieditor_ClearMainMenu();
}

static __used__ void routeEditor_cbCancelRouteUsers(eduimenu_s *, eduimenu_s *);
static __used__ void routeEditor_cbSetRouteUsers(eduimenu_s *, eduiitem_s *, u32);

static __used__ void routeEditor_cbRouteUsers(eduimenu_s *parent, eduiitem_s *, u32) {
    if (SpecialRouteCharacterNameFn == nullptr || aieditor->current_path == nullptr ||
        aieditor->current_path->current_route == nullptr) {
        return;
    }
    eduimenu_s *menu = eduiMenuCreate(0xdc, 0x46, 0xf0, 0xfa, ed_fnt,
                                     routeEditor_cbCancelRouteUsers, (char *)"Route Users");
    if (menu == nullptr) {
        return;
    }
    i32 index = 0;
    char *name = SpecialRouteCharacterNameFn(0);
    while (index < 64 && name != nullptr) {
        i32 selected = (aieditor->current_path->current_route->user_mask >> index) & 1;
        eduiitem_s *item = eduiItemCheckCreate(index, &attr, selected, index + 1,
                                               routeEditor_cbSetRouteUsers, name);
        eduiMenuAddItem(menu, item);
        eduiMenuAttach(parent, menu);
        ++index;
        name = SpecialRouteCharacterNameFn(index);
    }
    if (index == 0 || index == 64) {
        i32 selected = (aieditor->current_path->current_route->user_mask >> 63) & 1;
        eduiitem_s *item = eduiItemCheckCreate(63, &attr, selected, 64,
                                               routeEditor_cbSetRouteUsers, (char *)"Global");
        eduiMenuAddItem(menu, item);
    }
}

static __used__ void routeEditor_cbCreateRoute(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void routeEditor_cbDeleteRoute(eduimenu_s *, eduiitem_s *, u32) {
}

static __used__ void routeEditor_cbRenameRoute(eduimenu_s *, eduiitem_s *item, u32) {
    EDAIPATH_s *path = aieditor->current_path;
    if (path == nullptr || path->current_route == nullptr) {
        return;
    }
    EdUiNameInputItem *input = static_cast<EdUiNameInputItem *>(item);
    if (input->name[0] == '\0') {
        return;
    }
    for (i32 index = 0; index < 16; ++index) {
        if (NuStrICmp(path->routes[index].name, input->name) == 0) {
            return;
        }
    }
    strcpy(aieditor->current_path->current_route->name, input->name);
}

static __used__ void routeEditor_cbCancelRenameRouteMenu(eduimenu_s *, eduimenu_s *);

static __used__ void routeEditor_cbSetRouteUsers(eduimenu_s *, eduiitem_s *item, u32) {
    if (item == nullptr || aieditor->current_path == nullptr) {
        return;
    }
    EDAIPATHROUTE_s *route = aieditor->current_path->current_route;
    if (route == nullptr || (u32)item->data >= 64) {
        return;
    }
    if ((route->user_mask >> item->data) & 1) {
        route->user_mask &= ~(1ULL << item->data);
        item->highlighted = 0;
    } else {
        route->user_mask |= 1ULL << item->data;
        item->highlighted = 1;
    }
}

static __used__ void routeEditor_cbRenameRouteMenu(eduimenu_s *parent, eduiitem_s *, u32) {
    EDAIPATH_s *path = aieditor->current_path;
    if (path == nullptr || path->current_route == nullptr) {
        return;
    }
    eduimenu_s *menu = eduiMenuCreate(0xf0, 0x5a, 0xf0, 0xfa, ed_fnt,
                                     routeEditor_cbCancelRenameRouteMenu, (char *)"Rename Route");
    if (menu == nullptr) {
        return;
    }
    eduiitem_s *item = eduiItemTextPickCreate(0, &attr, routeEditor_cbRenameRoute, (char *)"Route Name");
    eduiMenuAddItem(menu, item);
    EdUiNameInputItem *input = static_cast<EdUiNameInputItem *>(edui_last_item);
    strcpy(input->name, aieditor->current_path->current_route->name);
    input->max_name_length = 15;
    eduiMenuAttach(parent, menu);
    menu->x = parent->x + 10;
    menu->y = parent->y + 40;
}

static __used__ void routeEditor_cbCancelRouteUsers(eduimenu_s *, eduimenu_s *menu) {
    eduiMenuDestroy(menu);
}

static __used__ void routeEditor_cbCancelRenameRouteMenu(eduimenu_s *, eduimenu_s *menu) {
    eduiMenuDestroy(menu);
}

extern "C" {

    void pathEditorCreateData(void) {
    }

    void pathEditorDrawPaths(void) {
        AIEDITOR_RENDER_STATE *state = aieditor;
        EDAIPATHWALL_s *wall = (EDAIPATHWALL_s *)NuLinkedListGetHead(&state->path_walls);
        while (wall != nullptr) {
            wall->flags &= ~u8(1);
            wall = (EDAIPATHWALL_s *)NuLinkedListGetNext(&state->path_walls, &wall->link);
        }

        if (aieditorsettings.draw_all_paths) {
            i32 index = 0;
            EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&state->paths);
            while (path != nullptr) {
                path->draw_index = index++;
                path = (EDAIPATH_s *)NuLinkedListGetNext(&state->paths, &path->link);
            }
            if (state->current_path != nullptr) {
                pathEditorDrawPath(state->current_path, state->current_path->draw_index);
            }
            path = (EDAIPATH_s *)NuLinkedListGetHead(&state->paths);
            while (path != nullptr) {
                if (path != state->current_path) {
                    pathEditorDrawPath(path, path->draw_index);
                }
                path = (EDAIPATH_s *)NuLinkedListGetNext(&state->paths, &path->link);
            }
        } else {
            pathEditorDrawPath(state->current_path, 0);
        }
    }

    void pathEditorSaveData(void) {
    }

    void pathEditor_CalcNodeIXs(void) {
        EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
        while (path != nullptr) {
            i32 index = 0;
            EDAIPATHNODE_s *node = (EDAIPATHNODE_s *)NuLinkedListGetHead(&path->nodes);
            while (node != nullptr) {
                node->index = index++;
                node = (EDAIPATHNODE_s *)NuLinkedListGetNext(&path->nodes, &node->link);
            }
            path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
        }
    }

    EDAIPATH_s *pathEditor_GetPath(const char *name) {
        EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
        while (path != nullptr && name != nullptr) {
            if (NuStrICmp(name, path->name) == 0) {
                return path;
            }
            path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
        }
        return aieditor->current_path;
    }

    void pathEditor_OnPathCheck(void) {
    }

    void pathEditor_QuickOnPathCheck(void) {
    }

    void pathEditor_UpdateNodesOnPlatforms(void) {
        EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths);
        while (path != nullptr) {
            EDAIPATHNODE_s *node = (EDAIPATHNODE_s *)NuLinkedListGetHead(&path->nodes);
            while (node != nullptr) {
                if (NuSpecialExistsFn(&node->special)) {
                    NuVecMtxTransform(&node->position, &node->special_position, NuSpecialGetDrawMtx(&node->special));
                }
                node = (EDAIPATHNODE_s *)NuLinkedListGetNext(&path->nodes, &node->link);
            }
            path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link);
        }
    }

} // extern "C"

void pathEditor_Enter(void) {
}

void pathEditor_Render(i32, i32, float, float) {
}

static EDAIPATHNODE_s *pathEditor_GetNearestNode(EDAIPATH_s *path, i32 require_radius) {
    EDAIPATHNODE_s *nearest = NULL;
    f32 nearest_distance = 3.402823466e38f;
    if (path != NULL) {
        for (EDAIPATHNODE_s *node = reinterpret_cast<EDAIPATHNODE_s *>(NuLinkedListGetHead(&path->nodes)); node != NULL;
             node = reinterpret_cast<EDAIPATHNODE_s *>(NuLinkedListGetNext(&path->nodes, &node->link))) {
            NUVEC delta;
            f32 distance = NuVecXZDistSqr(&aieditor->cursor_position, &node->position, &delta);
            if (distance < nearest_distance) {
                f32 height = aieditor->cursor_position.y - node->position.y;
                f32 upper = NuFmax(0.2f, node->height_max);
                f32 lower = NuFmin(-0.2f, node->height_min);
                if (height <= upper && height >= lower && (!require_radius || distance < node->radius * node->radius)) {
                    nearest = node;
                    nearest_distance = distance;
                }
            }
        }
    }
    return nearest;
}

static void pathEditor_PathNodeMoved(EDAIPATHNODE_s *node) {
    if (!(node->flags & 0x80) && NuSpecialExistsFn(&aieditor->cursor_platform)) {
        node->platform = aieditor->cursor_platform;
        NuVecInvMtxTransform(&node->platform_position, &node->position,
                             NuSpecialGetDrawMtx(&aieditor->cursor_platform));
    } else {
        node->platform.scene = NULL;
        node->platform.special = NULL;
        node->platform.display_special = NULL;
    }
    creatureEditor_PathNodeMoved(node);
    locatorEditor_PathNodeMoved(node);
    if (AIPathNodeDeletedFn != NULL)
        AIPathNodeDeletedFn(aieditor->current_path->current_node);
    if (node->shared_node != NULL) {
        for (EDAIPATH_s *path = reinterpret_cast<EDAIPATH_s *>(NuLinkedListGetHead(&aieditor->paths)); path != NULL;
             path = reinterpret_cast<EDAIPATH_s *>(NuLinkedListGetNext(&aieditor->paths, &path->link))) {
            for (EDAIPATHNODE_s *other = reinterpret_cast<EDAIPATHNODE_s *>(NuLinkedListGetHead(&path->nodes));
                 other != NULL;
                 other = reinterpret_cast<EDAIPATHNODE_s *>(NuLinkedListGetNext(&path->nodes, &other->link))) {
                if (other != node && other->shared_node == node->shared_node) {
                    other->radius = node->radius;
                    other->position.x = node->position.x;
                    other->height_min = node->height_min;
                    other->position.y = node->position.y;
                    other->height_max = node->height_max;
                    other->position.z = node->position.z;
                    other->platform = node->platform;
                    other->platform_position = node->platform_position;
                    creatureEditor_PathNodeMoved(other);
                    locatorEditor_PathNodeMoved(other);
                    break;
                }
            }
        }
    }
}

static __used__ void pathEditor_DestroySharedNode(EDAISHAREDPATHNODE_s *shared) {
    if (shared != NULL) {
        for (EDAIPATH_s *path = (EDAIPATH_s *)NuLinkedListGetHead(&aieditor->paths); path != NULL;
             path = (EDAIPATH_s *)NuLinkedListGetNext(&aieditor->paths, &path->link)) {
            for (EDAIPATHNODE_s *node = (EDAIPATHNODE_s *)NuLinkedListGetHead(&aieditor->current_path->nodes);
                 node != NULL;
                 node = (EDAIPATHNODE_s *)NuLinkedListGetNext(&aieditor->current_path->nodes, &node->link)) {
                if (node->shared_node == shared) {
                    node->shared_node = NULL;
                    break;
                }
            }
        }
        NuLinkedListRemove(&aieditor->shared_path_nodes, &shared->link);
        NuLinkedListAppend(&aieditor->free_shared_path_nodes, &shared->link);
    }
}

static void DestroyAIPathNode(EDAIPATHNODE_s *node, EDAIPATH_s *path) {
    if (node == NULL || path == NULL)
        return;
    if (node->shared_node != NULL) {
        --node->shared_node->reference_count;
        if (node->shared_node->reference_count <= 1)
            pathEditor_DestroySharedNode(node->shared_node);
        node->shared_node = NULL;
    }
    for (i32 connection = 0; connection < 8; ++connection) {
        EDAIPATHNODE_s *other = node->connections[connection].node;
        if (other != NULL) {
            for (i32 a = 0; a < 8; ++a) {
                if (node->connections[a].node == other) {
                    for (i32 b = 0; b < 8; ++b) {
                        if (other->connections[b].node == node) {
                            memset(&node->connections[a], 0, sizeof(EDAIPATHCNX_s));
                            memset(&other->connections[b], 0, sizeof(EDAIPATHCNX_s));
                            goto disconnected;
                        }
                    }
                }
            }
        }
    disconnected:;
    }
    NuLinkedListRemove(&path->nodes, &node->link);
    --path->node_count;
    memset(node, 0, sizeof(*node));
    NuLinkedListAppend(&aieditor->free_path_nodes, &node->link);
}

eduimenu_s *pathEditor_Process(nupad_s *pad) {
    if (pad->digital_buttons_pressed & 0x80) {
        eduimenu_s *menu = eduiMenuCreate(200, 70, 240, 270, ed_fnt, aieditor_cbCancelMainMenu, "Options");
        if (menu == NULL)
            return NULL;
        eduiMenuAddItem(
            menu, eduiItemSelCreate(AIEDITOR_ROUTES, attr, 0, 0, aieditor_cvSelectEditorMode, "Select Editor Mode"));
        if (aieditor->current_path && aieditor->current_path->current_node)
            eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbRenameNodeMenu, "Rename Node"));
        eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbSelectPathMenu, "Select Path Network"));
        eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, aieditor_cbSave, "Save AI Data"));
        eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, aieditor_cbGoToPlayer, "Go To Player"));
        eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, aieditor_cbMovePlayer, "Move Player"));
        eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbCreatePath, "Create Path Network"));
        if (aieditor->current_path) {
            if (aieditor->current_path->current_node)
                eduiMenuAddItem(menu,
                                eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbShareNodeMenu, "Share node with..."));
            if (aieditor->current_path && !(aieditor->current_path->flags & 1))
                eduiMenuAddItem(menu, eduiItemSelCreate(0, attr, 0, 0, pathEditor_cbDeletePath, "Delete Path Network"));
            if (aieditor->current_path && !(aieditor->current_path->flags & 1))
                eduiMenuAddItem(menu,
                                eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbRenamePathMenu, "Rename Path Network"));
        }
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, static_cast<i8>(aieditorsettings.path_flags) >> 7, 1,
                                                   aieditor_cbSolidPathDisplayToggle, "Solid Path Display"));
        i32 row = 2;
        if (aieditor->current_path) {
            EDAIPATHNODE_s *node = aieditor->current_path->current_node;
            EDAIPATHNODE_s *nearest = aieditor->current_path->nearest_node;
            EDAIPATHCNX_s *connection = NULL;
            if (node && nearest) {
                for (i32 i = 0; i < 8; ++i)
                    if (node->connections[i].node == nearest) {
                        connection = &node->connections[i];
                        break;
                    }
            }
            if (connection && naipathcnxtypes != 0) {
                eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, NULL, "================="));
                for (i32 i = 0; i < naipathcnxtypes; ++i) {
                    eduiMenuAddItem(menu,
                                    eduiItemToggleCreate(i, attr, (connection->flags & aipathcnxtypes[i].flags) != 0,
                                                         i + 2, pathEditor_cbCnxFlagsToggle, aipathcnxtypes[i].name));
                    row = i + 3;
                }
                eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, NULL, "================="));
            }
            node = aieditor->current_path->current_node;
            if (node) {
                eduiMenuAddItem(menu, eduiItemToggleCreate(0x80, attr, (node->flags >> 7) & 1, row,
                                                           pathEditor_cbNodeFlagsToggle, "Never On A Platform"));
                ++row;
            }
        }
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, -((aieditorsettings.path_flags >> 6) & 1), row,
                                                   aieditor_cbStopPlatformsToggle, "Stop Platforms"));
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, -((aieditorsettings.path_flags2 >> 1) & 1), row + 1,
                                                   pathEditor_cbDrawWallsplinesToggle, "Draw Wallsplines"));
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, -((aieditorsettings.path_flags >> 1) & 1), row + 2,
                                                   aieditor_cbDrawAllToggle, "Draw All Paths"));
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, -((aieditorsettings.path_flags >> 4) & 1), row + 3,
                                                   aieditor_cbSnapHeightToggle, "Snap Height"));
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, -((aieditorsettings.path_flags >> 3) & 1), row + 4,
                                                   aieditor_cbShowCreaturesToggle, "Show Creatures"));
        eduiMenuAddItem(menu, eduiItemToggleCreate(1, attr, near_clip_at_cursor, row + 5, cbNearClipAtCursor,
                                                   "Near Clip At Cursor"));
        return menu;
    }
    EDAIPATH_s *path = aieditor->current_path;
    if (!path)
        return NULL;
    EDAIPATHNODE_s *previous = path->current_node;
    if (!(path->flags & 1))
        aieditorsettings.path_flags &= ~1;
    if (aieditorsettings.path_flags & 1) {
        AIPATH_s *runtime = aieditor->runtime_path;
        i32 nearest = -1;
        f32 best = 3.402823466e38f;
        if (runtime && runtime->node_count) {
            for (i32 i = 0; i < runtime->node_count; ++i) {
                NUVEC delta;
                AIPATHNODE_s *node = &runtime->nodes[i];
                f32 distance = NuVecXZDistSqr(&aieditor->cursor_position, &node->position, &delta);
                if (distance < best && distance < node->radius_squared) {
                    best = distance;
                    nearest = i;
                }
            }
        }
        path->runtime_nearest = static_cast<i16>(nearest);
        if (pad->digital_buttons_pressed & 0x40) {
            path = aieditor->current_path;
            if (path->runtime_nearest >= 0)
                path->runtime_start = path->runtime_nearest;
        }
        if (pad->digital_buttons_pressed & 0x10) {
            path = aieditor->current_path;
            if (path->runtime_nearest >= 0)
                path->runtime_end = path->runtime_nearest;
        }
        if (pad->digital_buttons_pressed & 0x100) {
            path = aieditor->current_path;
            if (path->runtime_nearest >= 0) {
                path->runtime_start = path->runtime_nearest;
                edcamSetPos(&aieditor->runtime_path->nodes[path->runtime_nearest].position);
                path = aieditor->current_path;
            }
            EDAIPATHNODE_s *node = pathEditor_GetNearestNode(path, 0);
            path->current_node = node;
            if (aieditor->current_path->current_node)
                edcamSetPos(&aieditor->current_path->current_node->position);
        }
        return NULL;
    }
    if (pad->digital_buttons & 0x40) {
        EDAIPATHNODE_s *nearest = path->nearest_node;
        if (!nearest) {
            if (pad->digital_buttons_pressed & 0x40) {
                EDAIPATHNODE_s *node = NULL;
                if (path->node_count <= 253) {
                    node = reinterpret_cast<EDAIPATHNODE_s *>(NuLinkedListGetHead(&aieditor->free_path_nodes));
                    if (node) {
                        NuLinkedListRemove(&aieditor->free_path_nodes, &node->link);
                        NuLinkedListAppend(&path->nodes, &node->link);
                        ++path->node_count;
                        node->position = aieditor->camera_position;
                        node->radius = default_path_node_radius;
                        node->height_max = default_path_heighttol;
                        node->height_min = -default_path_heighttol;
                    }
                }
                path->current_node = node;
                if (previous) {
                    path = aieditor->current_path;
                    node = path->current_node;
                    if (node) {
                        node->radius = previous->radius;
                        node->height_max = previous->height_max;
                        node->height_min = previous->height_min;
                        bool connected = false;
                        for (i32 i = 0; i < 8 && !connected; ++i)
                            if (!node->connections[i].node) {
                                for (i32 j = 0; j < 8; ++j)
                                    if (!previous->connections[j].node) {
                                        node->connections[i].node = previous;
                                        node->connections[i].flags = 0;
                                        previous->connections[j].node = node;
                                        previous->connections[j].flags = 0;
                                        connected = true;
                                        break;
                                    }
                            }
                        if (!connected) {
                            DestroyAIPathNode(node, path);
                            aieditor->current_path->current_node = previous;
                        }
                    } else
                        aieditor->current_path->current_node = previous;
                }
            }
        } else if (pad->digital_buttons_pressed & 0x40) {
            path->current_node = nearest;
            NUVEC position = {nearest->position.x, aieditor->cursor_position.y, nearest->position.z};
            edcamSetPos(&position);
        } else if (path->current_node && path->current_node == nearest) {
            f32 old_x = nearest->position.x, old_z = nearest->position.z;
            nearest->position = aieditor->camera_position;
            pathEditor_PathNodeMoved(nearest);
            locatorEditor_PathNodeMoved(aieditor->current_path->current_node);
            if (AIPathNodeDeletedFn)
                AIPathNodeDeletedFn(aieditor->current_path->current_node);
            if (pad->digital_buttons & 0x200) {
                path = aieditor->current_path;
                f32 dx = path->current_node->position.x - old_x, dz = path->current_node->position.z - old_z;
                for (EDAIPATHNODE_s *node = reinterpret_cast<EDAIPATHNODE_s *>(NuLinkedListGetHead(&path->nodes)); node;
                     node = reinterpret_cast<EDAIPATHNODE_s *>(
                         NuLinkedListGetNext(&aieditor->current_path->nodes, &node->link))) {
                    if (node != aieditor->current_path->current_node) {
                        node->position.x += dx;
                        node->position.z += dz;
                        locatorEditor_PathNodeMoved(node);
                        if (AIPathNodeDeletedFn)
                            AIPathNodeDeletedFn(node);
                    }
                }
            }
        }
    }
    if (pad->digital_buttons_pressed & 0x20) {
        path = aieditor->current_path;
        EDAIPATHNODE_s *node = path->current_node, *nearest = path->nearest_node;
        if (node && nearest && node != nearest) {
            bool connected = false;
            for (i32 i = 0; i < 8; ++i)
                if (node->connections[i].node == nearest) {
                    connected = true;
                    break;
                }
            if (connected) {
                eduimenu_s *menu = eduiMenuCreate(200, 70, 240, 270, ed_fnt, pathEditor_cbCancelDisconnectNodeMenu,
                                                  "Disconnect current path node??");
                if (menu) {
                    eduiMenuAddItem(menu, eduiItemSelCreate(0, attr, 0, 0, pathEditor_cbDisconnectPathNode, "No"));
                    eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbDisconnectPathNode, "Yes"));
                }
                return menu;
            }
            for (i32 i = 0; i < 8 && !connected; ++i)
                if (!node->connections[i].node) {
                    for (i32 j = 0; j < 8; ++j)
                        if (!nearest->connections[j].node) {
                            node->connections[i].node = nearest;
                            node->connections[i].flags = 0;
                            nearest->connections[j].node = node;
                            nearest->connections[j].flags = 0;
                            connected = true;
                            break;
                        }
                }
            NUVEC position = {nearest->position.x, aieditor->cursor_position.y, nearest->position.z};
            edcamSetPos(&position);
        }
    }
    if (pad->digital_buttons_pressed & 0x10) {
        path = aieditor->current_path;
        if (path->current_node && path->current_node == path->nearest_node) {
            eduimenu_s *menu = eduiMenuCreate(200, 70, 240, 270, ed_fnt, pathEditor_cbCancelDeleteNodeMenu,
                                              "Delete current path node??");
            if (menu) {
                eduiMenuAddItem(menu, eduiItemSelCreate(0, attr, 0, 0, pathEditor_cbDeletePathNode, "No"));
                eduiMenuAddItem(menu, eduiItemSelCreate(1, attr, 0, 0, pathEditor_cbDeletePathNode, "Yes"));
            }
            return menu;
        }
    }
    if (pad->digital_buttons_pressed & 0x100) {
        if (aieditorsettings.path_flags & 2) {
            EDAIPATH_s *nearest_path = NULL;
            EDAIPATHNODE_s *nearest_node = NULL;
            f32 best = 3.402823466e38f;
            for (path = reinterpret_cast<EDAIPATH_s *>(NuLinkedListGetHead(&aieditor->paths)); path;
                 path = reinterpret_cast<EDAIPATH_s *>(NuLinkedListGetNext(&aieditor->paths, &path->link))) {
                EDAIPATHNODE_s *node = pathEditor_GetNearestNode(path, 0);
                if (node) {
                    NUVEC delta;
                    f32 distance = NuVecXZDistSqr(&aieditor->cursor_position, &node->position, &delta);
                    if (distance < best) {
                        nearest_path = path;
                        nearest_node = node;
                        best = distance;
                    }
                }
            }
            if (nearest_path && nearest_node) {
                aieditor->current_path = nearest_path;
                nearest_path->current_node = nearest_node;
                edcamSetPos(&nearest_node->position);
            }
        } else {
            path = aieditor->current_path;
            path->current_node = pathEditor_GetNearestNode(path, 0);
            EDAIPATHNODE_s *node = aieditor->current_path->current_node;
            if (node) {
                if (edpath_addoffset.x != 0 || edpath_addoffset.y != 0 || edpath_addoffset.z != 0) {
                    NuVecAdd(&node->position, &node->position, &edpath_addoffset);
                    pathEditor_PathNodeMoved(aieditor->current_path->current_node);
                    locatorEditor_PathNodeMoved(aieditor->current_path->current_node);
                }
                edcamSetPos(&aieditor->current_path->current_node->position);
            }
        }
    }
    aieditor->flags &= ~1;
    path = aieditor->current_path;
    EDAIPATHNODE_s *node = path->current_node;
    if (node && node == path->nearest_node) {
        if (pad->digital_buttons & (0x8000 | 0x2000)) {
            node->radius *= (pad->digital_buttons & 0x8000) ? .99f : 1.01f;
            if (node->radius < .01f)
                node->radius = .01f;
            pathEditor_PathNodeMoved(node);
            locatorEditor_PathNodeMoved(aieditor->current_path->current_node);
            if (AIPathNodeDeletedFn)
                AIPathNodeDeletedFn(aieditor->current_path->current_node);
            path = aieditor->current_path;
        }
        if (aieditorsettings.path_flags & 0x80) {
            if (pad->digital_buttons & 0x1000) {
                aieditor->flags |= 4;
                node = path->current_node;
                f32 step = node->height_max * .05f;
                if (pad->digital_buttons & 4)
                    node->height_max += step;
                else if (pad->digital_buttons & 1)
                    node->height_max -= step;
                if (node->height_max < .01f)
                    node->height_max = .01f;
            } else if (pad->digital_buttons & 0x4000) {
                aieditor->flags |= 4;
                node = path->current_node;
                f32 step = -node->height_min * .05f;
                if (pad->digital_buttons & 4)
                    node->height_min += step;
                else if (pad->digital_buttons & 1)
                    node->height_min -= step;
                if (node->height_min > -.01f)
                    node->height_min = -.01f;
            }
        }
        aieditor->flags |= 1;
    }
    i32 direction = 0;
    bool named_only = false;
    if (pad->digital_buttons & 0x100) {
        if (pad->digital_buttons_pressed & 8)
            direction = 1;
        else if (pad->digital_buttons_pressed & 2)
            direction = -1;
        else if (pad->digital_buttons_pressed & 4) {
            direction = 1;
            named_only = true;
        } else if (pad->digital_buttons_pressed & 1) {
            direction = -1;
            named_only = true;
        }
    }
    if (!direction && !(aieditorsettings.path_flags & 0x80)) {
        if (pad->digital_buttons_pressed & 0x1000)
            direction = 1;
        else if (pad->digital_buttons_pressed & 0x4000)
            direction = -1;
    }
    if (direction) {
        path = aieditor->current_path;
        if (path->current_node)
            path->current_node = reinterpret_cast<EDAIPATHNODE_s *>(
                direction > 0 ? NuLinkedListGetNext(&path->nodes, &path->current_node->link)
                              : NuLinkedListGetPrev(&path->nodes, &path->current_node->link));
        path = aieditor->current_path;
        if (!path->current_node)
            path->current_node = reinterpret_cast<EDAIPATHNODE_s *>(direction > 0 ? NuLinkedListGetHead(&path->nodes)
                                                                                  : NuLinkedListGetTail(&path->nodes));
        path = aieditor->current_path;
        if (named_only && path->current_node && NuStrLen(path->current_node->name) == 0) {
            EDAIPATHNODE_s *start = path->current_node;
            do {
                path = aieditor->current_path;
                path->current_node = reinterpret_cast<EDAIPATHNODE_s *>(
                    direction > 0 ? NuLinkedListGetNext(&path->nodes, &path->current_node->link)
                                  : NuLinkedListGetPrev(&path->nodes, &path->current_node->link));
                path = aieditor->current_path;
                if (!path->current_node)
                    path->current_node = reinterpret_cast<EDAIPATHNODE_s *>(
                        direction > 0 ? NuLinkedListGetHead(&path->nodes) : NuLinkedListGetTail(&path->nodes));
                path = aieditor->current_path;
                if (!path->current_node || path->current_node == start)
                    break;
            } while (NuStrLen(path->current_node->name) == 0);
        }
        if (path->current_node)
            edcamSetPos(&path->current_node->position);
    }
    path = aieditor->current_path;
    path->nearest_node = pathEditor_GetNearestNode(path, 1);
    return NULL;
}
