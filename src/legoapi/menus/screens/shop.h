#pragma once

#include "legoapi/legoapi_types.h"
#include "nu2api/nu3d/nuspline.h"

extern shopitem_s TopShelf[6];
extern shopitem_s *CharItems;
extern shopitem_s *HintItems;
extern shopitem_s *ExtraItems;
extern shopitem_s *CodeItems;
extern shopitem_s *BrickItems;
extern shopitem_s CutItems[128];
extern nuhspecial_s extrasils[44];
extern nuhspecial_s atoz0to9icon[36];

extern u32 codelist[144];
extern i32 SHOPCHARCOUNT;
extern i32 SHOPHINTCOUNT;
extern i32 SHOPEXTRACOUNT;
extern i32 CutScenePlayCount;
extern i32 SHOPACTIVE;
extern i32 enteredshop;
extern f32 ShopNameAlpha;
extern f32 ShopLockedScale;
i32 Shop_UpdateHint(HINT_s *hint);

extern i32 CharShelfIds[7];
extern i32 HintShelfIds[7];
extern i32 ExtraShelfIds[7];
extern i32 BrickShelfIds[7];
extern i32 CutShelfIds[7];
extern NUVEC CharCurPos[7];
extern NUVEC HintCurPos[7];
extern NUVEC ExtraCurPos[7];
extern NUVEC BrickCurPos[7];
extern NUVEC CutCurPos[7];

extern NUVEC ShelfPos[6];
extern NUVEC SubShelfPos[7];
extern NUVEC CodePos[7];
extern u16 shelfang;

extern nuhspecial_s iconback;
extern nuhspecial_s infoblank;
extern nuhspecial_s cutblank;
extern nuhspecial_s cutfilm_unlocked;
extern nuhspecial_s cutfilm_locked;
extern nuhspecial_s toolblank;
extern nuhspecial_s codeblank;
extern nuhspecial_s question;
extern nuhspecial_s arrow1;
extern nuhspecial_s arrow2;
extern nuhspecial_s arrow3;
extern nuhspecial_s arrow4;
extern NUGSPLINE *shopcamspline;
extern NUVEC *shopcampos;
extern NUVEC *shopcamlookat;
extern i32 oldpicked;
extern i32 picked;
extern i32 subpicked;
extern i32 subitemselected;

using ShopMenuCallback = i32 (*)(MENU_s *);
using ShopDrawCallback = void (*)();
extern i32 codevalid;
extern i32 codechar;
extern char codechars[];
extern NUVEC selectedoff;
extern void *shopcutsceneplayer;
extern i32 shopmenu;
extern i32 col;
extern char usercode[];

struct SHOPINPUT {
    union {
        i32 value[10];
        struct {
            i32 left_pressed, right_pressed, up_pressed, down_pressed;
            i32 left_held, right_held, up_held, down_held;
            i32 confirm, cancel;
        };
    };
};

void InitShop(WORLDINFO_s *world);
void LoadShelfSplines();
void GetShopCamLookPos(NUVEC *position);
void UpdateCharacterIDs();
int DoShopMenu(MENU_s *menu);
int UpdateShop(MENU_s *menu);
void EndShopMenu(i32 exit_mode);
void DrawTopShelf(i32 picked_item);
i32 CheckCash(shopitem_s *items, i32 item);
i32 BuyShopItem(shopitem_s *items, i32 index, i32 charge);
i32 SelectShopItem(shopitem_s *items, i32 index);
void SelectSubItem();
i32 MoveSubItemsLeft(i32 *ids, NUVEC *positions, i32 count);
i32 MoveSubItemsRight(i32 *ids, NUVEC *positions, i32 count);
