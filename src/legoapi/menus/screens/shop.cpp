#include "decomp.h"
#include "gameapi/gui/apimenu.h"
#include "gamelib/crc/crc.h"
#include "globals.h"
#include "legoapi/characters/motion.h"
#include "legoapi/characters/core/character.h"
#include "legoapi/items/base/collection.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/menus/screens/shop.h"
#include "legoapi/menus/screens/gamemenuall.h"
#include "legoapi/cutscenes/cutscenes.h"
#include "legoapi/menus/screens/store.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "legoapi/world/world_shared.h"
#include "nu2api/nu3d/nugscn.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nucore/nustring.h"
#include "legoapi/core/input/gamepads.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/render/fx.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/world/area.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nutrig.h"

#include <string.h>
#include <stdio.h>
extern "C" void PlaySfx(char *, NUVEC *);
extern "C" void NuIOS_RecordFlurryEvent(char *);
void GameAudio_PlaySfx(i32, NUVEC *, i32, i32);
void SetLevelLights(void *, f32);
void DrawItem(nuhspecial_s *, nuvec_s *, float, float, float, u16, u16, u16);
void Draw3DObject(WORLDINFO_s *, i32, nuvec_s *, u16, u16, u16, float, float, float, i32);
void *AddGameMessage(char *, NUVEC *, f32, NUVEC *, f32, u8, u8, u8, u32, f32);
extern f32 HUB_EPISODETITLEY;
extern f32 HUB_EPISODESUBTITLEY;
i32 AddToCollection(i32);
void AddToCompletionPoints(u32);
void AddToGoldBricks();
extern GAMESAVE_s TempGame;
extern i32 hub_forceshopsave;
i32 POINTS_PER_HINT;
// Shop state is file-local, matching the original _ZL symbols.
static f32 pickedbing;
static i32 charcheatix;
static i32 extracheatix;
static f32 cheattimer;
static f32 codebigscale[6];
static f32 codeshelfscale[6];
static f32 codemenuscale[6];
i32 codevalid;
i32 codechar;
char codechars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

shopitem_s TopShelf[6] = {};
shopitem_s *CharItems = NULL;
shopitem_s *HintItems = NULL;
shopitem_s *ExtraItems = NULL;
shopitem_s *CodeItems = NULL;
shopitem_s *BrickItems = NULL;
shopitem_s CutItems[128] = {};
nuhspecial_s extrasils[44] = {};
nuhspecial_s atoz0to9icon[36] = {};

u32 codelist[144] = {};
i32 SHOPCHARCOUNT = 0;
i32 SHOPHINTCOUNT = 0;
i32 SHOPEXTRACOUNT = 0;
i32 CutScenePlayCount = 0;
i16 HintTab[24] = {-1};

i32 CharShelfIds[7] = {};
i32 HintShelfIds[7] = {};
i32 ExtraShelfIds[7] = {};
i32 BrickShelfIds[7] = {};
i32 CutShelfIds[7] = {};
NUVEC CharCurPos[7] = {};
NUVEC HintCurPos[7] = {};
NUVEC ExtraCurPos[7] = {};
NUVEC BrickCurPos[7] = {};
NUVEC CutCurPos[7] = {};

static NUGSPLINE *splshelf = NULL;
static NUGSPLINE *splcharshelf = NULL;
static NUGSPLINE *splcodes = NULL;
NUVEC ShelfPos[6] = {};
NUVEC SubShelfPos[7] = {};
NUVEC CodePos[7] = {};
u16 shelfang = 0;

nuhspecial_s iconback = {};
nuhspecial_s infoblank = {};
nuhspecial_s cutblank = {};
nuhspecial_s cutfilm_unlocked = {};
nuhspecial_s cutfilm_locked = {};
nuhspecial_s toolblank = {};
nuhspecial_s codeblank = {};
nuhspecial_s question = {};
nuhspecial_s arrow1 = {};
nuhspecial_s arrow2 = {};
nuhspecial_s arrow3 = {};
nuhspecial_s arrow4 = {};
NUGSPLINE *shopcamspline = NULL;
NUVEC *shopcampos = NULL;
NUVEC *shopcamlookat = NULL;
static f32 TopShelfScale[6] = {};
static f32 TopBigScale[6] = {};
static f32 topscale[6] = {};
static f32 TopShelfPush[6] = {};
static f32 TopBigPush[6] = {};
static f32 toppush[6] = {};
i32 oldpicked = 1;
i32 picked = 1;
i32 subpicked = 0;
i32 subitemselected = 0;
static void (*drawptr)() = NULL;

static f32 scale2 = 0.0f;
static f32 scalenorm;
static f32 scalepicked;
static f32 scale3 = 0.0f;
static f32 scale4 = 0.0f;
static f32 subpush[3] = {};
static i32 moveitems = 0;
NUVEC selectedoff = {};
void *shopcutsceneplayer = NULL;

f32 ShopLockedScale = 0.0f;
f32 ShopNameAlpha = 0.0f;
i32 enteredshop = 0;
i32 SHOPACTIVE = 0;
i32 shopmenu = 0;
i32 col = 0;
char usercode[6] = {'A', 'A', 'A', 'A', 'A', 'A'};
extern i32 shop_from_cutsceneplayer;

static f32 SubBigCharPush = 0.0f;
static f32 SubNormCharPush = 0.0f;
static ShopMenuCallback menuptr = NULL;
static ShopMenuCallback oldmenuptr = NULL;
static ShopMenuCallback menuparent[3] = {};
static ShopMenuCallback oldmenuparent[3] = {};
static ShopDrawCallback drawparent[3] = {};
static ShopDrawCallback olddrawparent[3] = {};
static ShopDrawCallback drawpanelptr = NULL;
static ShopDrawCallback olddrawpanelptr = NULL;
static ShopDrawCallback olddrawptr = NULL;
static i32 currentmenulevel = 0;
static i32 oldcurrentmenulevel = 0;
static i32 currentdrawlevel = 0;
static i32 oldcurrentdrawlevel = 0;
static f32 oldpickedscale = 0.0f;
static f32 oldpickedpush = 0.0f;
static f32 slidetimer = 0.0f;
static f32 scaleoverride[7] = {};
static i32 ExitMenu = 0;
static i32 lastitem = 0;
static f32 hintdrawwait = 0.0f;
static i32 scrollkeyhit = 0;
static i32 SHOPCUTCOUNT = 128;
static i32 easesubin = 0;
static f32 inoutscale = 0.0f;
static char *cheatname = NULL;

extern i32 ItemMenu(MENU_s *menu);
extern void DrawItemMenu2D();
extern void InitAlphaList();
extern void InitExtraList();
extern HINT_s *Hint_FindHint(i32 hint_id);
extern i16 HintTab[24];
extern void GameCam_Blend(GAMECAMERA_s *camera, f32 duration, f32 curve, i32 mode);

i32 DoShopMenu(MENU_s *menu) {
    i32 result = 0;
    if (menuptr != NULL) {
        result = menuptr(menu);
        if (menu->cancel_pressed != 0) {
            result = currentmenulevel < 1 ? 5 : 6;
            for (i32 i = 4; i < 13; ++i) {
                menu->item_width[i] = 0.0f;
            }
            subitemselected = 0;
        }

        if (result == 5) {
            return 1;
        }
        if (result == 6) {
            GameCam_Blend(GameCam, 0.6f, 0.0f, 1);
            drawptr = NULL;

            menuptr = menuparent[currentmenulevel];
            menuparent[currentmenulevel] = NULL;
            --currentmenulevel;
            if (currentmenulevel < 0) {
                currentmenulevel = 0;
            }

            drawpanelptr = drawparent[currentdrawlevel];
            drawparent[currentdrawlevel] = NULL;
            --currentdrawlevel;
            if (currentdrawlevel < 0) {
                currentdrawlevel = 0;
            }
        }
    }
    return 0;
}

i32 UpdateShop(MENU_s *menu) {
    UpdateCharacterIDs();

    scaleoverride[0] = 0.0f;
    scaleoverride[1] = 0.4f;
    scaleoverride[2] = 0.8f;
    scaleoverride[4] = 0.8f;
    scaleoverride[5] = 0.4f;
    scaleoverride[6] = 0.0f;

    if (1.0f > ShopNameAlpha) {
        ShopNameAlpha += FRAMETIME + FRAMETIME;
        if (ShopNameAlpha > 1.0f) {
            ShopNameAlpha = 1.0f;
        }
    }

    ShopLockedScale = SeekLinearF(ShopLockedScale, 1.0f, FRAMETIME * 3.0f);
    SubNormCharPush = 0.02f;
    SubBigCharPush = 0.05f;

    if (menuptr == NULL) {
        if (shop_from_cutsceneplayer != 0) {
            shop_from_cutsceneplayer = 0;
            topscale[5] = oldpickedscale;
            menuptr = oldmenuptr;
            toppush[5] = oldpickedpush;
            drawparent[0] = olddrawparent[0];
            menuparent[0] = oldmenuparent[0];
            drawparent[1] = olddrawparent[1];
            menuparent[1] = oldmenuparent[1];
            drawparent[2] = olddrawparent[2];
            menuparent[2] = oldmenuparent[2];
            drawpanelptr = olddrawpanelptr;
            drawptr = olddrawptr;
            currentmenulevel = oldcurrentmenulevel;
            currentdrawlevel = oldcurrentdrawlevel;
            picked = oldpicked;
        } else {
            menuptr = ItemMenu;
            ExitMenu = 0;
            slidetimer = 0.125f;
            enteredshop = 1;
            currentmenulevel = 0;
            currentdrawlevel = 0;
            menuparent[0] = NULL;
            drawparent[0] = NULL;
            picked = oldpicked;
            drawpanelptr = DrawItemMenu2D;
        }
    }

    return DoShopMenu(menu) != 0;
}

i32 BuyShopItem(shopitem_s *items, i32 index, i32 charge) {
    shopitem_s *item = &items[index];
    u32 *bits = NULL;
    if (item->type == 0)
        bits = Game.shop_hint_purchased_bits;
    else if (item->type == 1)
        bits = Game.shop_character_purchased_bits;
    else if (item->type == 2)
        bits = Game.extra_unlocked_bits;
    else if (item->type == 4)
        bits = &Game.shop_gold_brick_purchased_bits;
    else if (item->type == 5) {
        PlaySfx("MenuSelect", 0);
        return 1;
    }
    if (bits != NULL)
        bits[index / 32] |= static_cast<u32>(u64(1) << (index % 32));
    if (charge != 0)
        Game.coins -= item->price;
    pickedbing = 0.35f;
    item->unlocked = 1;
    if (charge != 0)
        PlaySfx("Shop_BuyCheat", &SubShelfPos[3]);
    return 1;
}

void SelectSubItem() {
    shopitem_s *items;
    i32 index;
    switch (picked) {
        case 0:
            items = HintItems;
            index = HintShelfIds[3];
            break;
        case 1:
            items = CharItems;
            index = CharShelfIds[3];
            break;
        case 2:
            items = ExtraItems;
            index = ExtraShelfIds[3];
            break;
        case 4:
            items = BrickItems;
            index = BrickShelfIds[3];
            break;
        case 5:
            items = CutItems;
            index = CutShelfIds[3];
            break;
        default:
            return;
    }
    if (index != -1 && items != NULL) {
        const i32 result = SelectShopItem(items, index);
        if (result == -1)
            PlaySfx("Shop_NotEnufMuny", &SubShelfPos[3]);
        if (result == 1)
            ShopNameAlpha = 0.0f;
    }
}

i32 SelectShopItem(shopitem_s *items, i32 index) {
    shopitem_s *item = &items[index];
    char event[128];
    switch (item->type) {
        case 0: {
            const i32 id = static_cast<u16>(item->item_id);
            if (item->unlocked == 1 || item->price == 0)
                return 1;
            if (!CheckCash(items, index))
                break;
            AddToCompletionPoints(POINTS_PER_HINT);
            if (HINT_s *hint = Hint_FindHint(HintTab[id]))
                hint->flags |= HINT_SHOP_PURCHASED;
            return BuyShopItem(items, index, 1);
        }
        case 1: {
            const i32 id = static_cast<u16>(item->item_id);
            if (item->unlocked == 1 || id >= CHARCOUNT)
                return 0;
            if (!CollectIDUnlocked(id))
                goto locked;
            if (!CheckCash(items, index))
                break;
            if (!AddToCollection(id))
                return 1;
            AddToCompletionPoints(POINTS_PER_CHARACTER);
            sprintf(event, "hubshop_buychar_%s", CDataList[id].file);
            NuIOS_RecordFlurryEvent(event);
            return BuyShopItem(items, index, 1);
        }
        case 2: {
            const i32 id = static_cast<u16>(item->item_id);
            if (item->unlocked == 1 || (Game.extra_purchased_bits[id >> 5] & (1U << (id & 31))))
                return 0;
            const i32 area = static_cast<i8>(Cheat[id].area);
            if (area != -1 && !Game.area_save[area].red_brick_collected)
                goto locked;
            if (!CheckCash(items, index))
                break;
            Game.extra_purchased_bits[id >> 5] |= 1U << (id & 31);
            AddToCompletionPoints(POINTS_PER_CHEAT);
            sprintf(event, "hubshop_buyextra_%s", Cheat[id].name);
            NuIOS_RecordFlurryEvent(event);
            return BuyShopItem(items, index, 1);
        }
        case 4:
            if (item->unlocked == 1 || item->price == 0)
                return 0;
            if (static_cast<f32>(index * 3600) > Game.field30_0x7c2c)
                goto locked;
            if (!CheckCash(items, index))
                break;
            AddToCompletionPoints(POINTS_PER_GOLDBRICK);
            AddToGoldBricks();
            sprintf(event, "hubshop_buygoldbrick_%i", index + 1);
            NuIOS_RecordFlurryEvent(event);
            return BuyShopItem(items, index, 1);
        case 5:
            if (!CutScenePlayer_CanStart(index)) {
                PlaySfx("MenuNoEntry", 0);
                ShopLockedScale = 1.5f;
                return 0;
            }
            oldmenuptr = menuptr;
            for (i32 i = 0; i < 3; ++i) {
                olddrawparent[i] = drawparent[i];
                oldmenuparent[i] = menuparent[i];
            }
            olddrawpanelptr = drawpanelptr;
            olddrawptr = drawptr;
            oldcurrentmenulevel = currentmenulevel;
            oldcurrentdrawlevel = currentdrawlevel;
            oldpickedscale = topscale[5];
            oldpickedpush = toppush[5];
            menuptr = NULL;
            for (i32 i = 0; i < 3; ++i) {
                drawparent[i] = NULL;
                menuparent[i] = NULL;
            }
            currentmenulevel = currentdrawlevel = 0;
            oldpicked = picked;
            SHOPACTIVE = 0;
            BuyShopItem(items, index, 0);
            TempGame.field30_0x7c2c = Game.field30_0x7c2c;
            pickedbing = 0.0f;
            if (memcmp(&TempGame, &Game, sizeof(Game)))
                hub_forceshopsave = 1;
            CutScenePlayer_Start(index, -1);
            return 1;
        default:
            return -1;
    }
    CoinTotalScale = 1.5f;
    return -1;
locked:
    GameAudio_PlaySfx(0x32, NULL, 0, 0);
    ShopLockedScale = 1.5f;
    return 0;
}

static NUVEC HubShopPos = {-26.3f, 0.0f, -49.5f};

i32 Shop_UpdateHint(HINT_s *hint) {
    if (!Game.coins || !WORLD || WORLD->current_level != HUB_LDATA)
        return 0;
    if (GameCam->sock_position.location.sock != 0 || !player)
        return 0;
    if (!(NuVecXZDistSqr(&player->apiobj.position, &HubShopPos, NULL) < 1.0f))
        return 0;

    if (hint->control_mode_ids[0] == 0x5ea) {
        for (i32 i = 0; i < ShopCollection.count_y && i < 100; ++i) {
            if (CollectIDUnlocked(ShopCollection.list[i].id) && !Collection_Got(ShopCollection.list[i].id) &&
                Game.coins >= static_cast<u32>(ShopCollection.list[i].field3_0x4))
                return 1;
        }
    } else if (hint->control_mode_ids[0] == 0x5eb) {
        for (i32 i = 0; i < 44; ++i) {
            const i8 area = static_cast<i8>(Cheat[i].area);
            if (!(Game.extra_purchased_bits[i >> 5] >> (i & 31) & 1) &&
                (area == -1 || Game.area_save[area].red_brick_collected) &&
                Game.coins >= static_cast<u32>(Cheat[i].extra_price))
                return 1;
        }
    } else if (hint->control_mode_ids[0] == 0x5ec) {
        for (i32 i = 0; i < SHOPGOLDBRICKS; ++i) {
            if (!(static_cast<u64>((&Game.shop_gold_brick_purchased_bits)[i >> 5]) >> (i & 31) & 1) &&
                static_cast<f32>(i * 3600) <= Game.field30_0x7c2c &&
                Game.coins >= static_cast<u32>(BrickItems[i].price))
                return 1;
        }
    }
    return 0;
}

void BuyAllShopExtras() {
    WORLDINFO_s *world = WORLD;
    shopitem_s *item = ExtraItems;
    for (i32 i = 0; i < 44; ++i, ++item) {
        Game.purchased_extra_bits[i >> 5] |= 1u << (i & 31);
        if (world->current_level == HUB_LDATA)
            item->unlocked = 1;
        i32 area = static_cast<i8>(Cheat[i].area);
        if (area != -1 && Game.area_save[area].field_0x5[1] == 0)
            Game.area_save[area].field_0x5[1] = 1;
    }
    Game.unlocked_extra_bits[0] = 0xffffffff;
    Game.unlocked_extra_bits[1] = 0xffffffff;
}

void GetShopCamLookPos(nuvec_s *position) {
    if (menuptr == ItemMenu && splshelf != NULL && picked != -1)
        *position = splshelf->pts[picked];
    else
        *position = *shopcamlookat;
}

i32 MoveSubItemsRight(i32 *ids, NUVEC *positions, i32 count) {
    if (slidetimer > 0.0f) {
        const f32 phase = 1.0f - slidetimer * 8.0f;
        const i32 angle = static_cast<i32>(phase * 32768.0f + 16384.0f);
        f32 t = 1.0f - (NuTrigTable[(angle >> 1) & 0x7fff] + 1.0f) * 0.5f;
        for (i32 i = 1; i < 7; ++i) {
            t = NuFmax(0.0f, NuFmin(t, 1.0f));
            positions[i].x = SubShelfPos[i].x + (SubShelfPos[i - 1].x - SubShelfPos[i].x) * t;
            positions[i].y = SubShelfPos[i].y + (SubShelfPos[i - 1].y - SubShelfPos[i].y) * t;
            positions[i].z = SubShelfPos[i].z + (SubShelfPos[i - 1].z - SubShelfPos[i].z) * t;
        }
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        scale3 = scalepicked + (scalenorm - scalepicked) * t;
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        scale4 = scalenorm + (scalepicked - scalenorm) * t;
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        subpush[1] = SubBigCharPush + (SubNormCharPush - SubBigCharPush) * t;
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        subpush[2] = SubNormCharPush + (SubBigCharPush - SubNormCharPush) * t;
        scaleoverride[6] = phase;
        scaleoverride[0] = 1.0f - phase;
        return 0;
    }
    for (i32 i = 0; i < 6; ++i)
        ids[i] = ids[i + 1];
    ids[6] = ids[5] + 1;
    if (ids[6] >= count)
        ids[6] = 0;
    for (i32 i = 0; i < 7; ++i)
        positions[i] = SubShelfPos[i];
    slidetimer = 0.0f;
    scale2 = scalenorm;
    scale3 = scalepicked;
    scale4 = scalenorm;
    subpush[0] = SubNormCharPush;
    subpush[1] = SubBigCharPush;
    subpush[2] = SubNormCharPush;
    --moveitems;
    scaleoverride[0] = 0.0f;
    scaleoverride[1] = 0.4f;
    scaleoverride[2] = 0.8f;
    scaleoverride[4] = 0.8f;
    scaleoverride[5] = 0.4f;
    scaleoverride[6] = 0.0f;
    return 1;
}

i32 MoveSubItemsLeft(i32 *ids, NUVEC *positions, i32 count) {
    if (slidetimer > 0.0f) {
        const f32 phase = 1.0f - slidetimer * 8.0f;
        const i32 angle = static_cast<i32>(phase * 32768.0f + 16384.0f);
        f32 t = 1.0f - (NuTrigTable[(angle >> 1) & 0x7fff] + 1.0f) * 0.5f;
        for (i32 i = 0; i < 6; ++i) {
            t = NuFmax(0.0f, NuFmin(t, 1.0f));
            positions[i].x = SubShelfPos[i].x + (SubShelfPos[i + 1].x - SubShelfPos[i].x) * t;
            positions[i].y = SubShelfPos[i].y + (SubShelfPos[i + 1].y - SubShelfPos[i].y) * t;
            positions[i].z = SubShelfPos[i].z + (SubShelfPos[i + 1].z - SubShelfPos[i].z) * t;
        }
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        scale2 = scalenorm + (scalepicked - scalenorm) * t;
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        scale3 = scalepicked + (scalenorm - scalepicked) * t;
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        subpush[0] = SubNormCharPush + (SubBigCharPush - SubNormCharPush) * t;
        t = NuFmax(0.0f, NuFmin(t, 1.0f));
        subpush[1] = SubBigCharPush + (SubNormCharPush - SubBigCharPush) * t;
        scaleoverride[0] = phase;
        scaleoverride[6] = 1.0f - phase;
        return 0;
    }
    for (i32 i = 6; i > 0; --i)
        ids[i] = ids[i - 1];
    ids[0] = ids[1] - 1;
    if (ids[0] < 0)
        ids[0] = count - 1;
    for (i32 i = 0; i < 7; ++i)
        positions[i] = SubShelfPos[i];
    slidetimer = 0.0f;
    scale2 = scalenorm;
    scale3 = scalepicked;
    scale4 = scalenorm;
    subpush[0] = SubNormCharPush;
    subpush[1] = SubBigCharPush;
    subpush[2] = SubNormCharPush;
    ++moveitems;
    scaleoverride[0] = 0.0f;
    scaleoverride[1] = 0.4f;
    scaleoverride[2] = 0.8f;
    scaleoverride[4] = 0.8f;
    scaleoverride[5] = 0.4f;
    scaleoverride[6] = 0.0f;
    return 1;
}

void Shop_CollectAllCharacters(i32 mode) {
    if (mode != 0)
        return;
    const i32 count = ShopCollection.count_y;
    for (i32 i = 0; i != count && i != 100; ++i) {
        Game.shop_character_purchased_bits[i >> 5] |= static_cast<u32>(u64(1) << (i & 31));
        if (WORLD->current_level == HUB_LDATA)
            CharItems[i].unlocked = SAVE_ON;
    }
}

void InitShop(WORLDINFO_s *world) {
    CharItems =
        static_cast<shopitem_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 100 * sizeof(shopitem_s)));
    HintItems =
        static_cast<shopitem_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 64 * sizeof(shopitem_s)));
    ExtraItems =
        static_cast<shopitem_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 64 * sizeof(shopitem_s)));
    CodeItems =
        static_cast<shopitem_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 64 * sizeof(shopitem_s)));
    BrickItems =
        static_cast<shopitem_s *>(GameBufferAlloc(&world->giz_buffer, &world->unknown_0108, 14 * sizeof(shopitem_s)));

    if (CharItems == NULL || HintItems == NULL || ExtraItems == NULL || CodeItems == NULL || BrickItems == NULL) {
        return;
    }

    TopShelf[0].type = 0;
    NuStrCpy(TopShelf[0].name, "Hint");
    NuStrCpy(TopShelf[0].special_name, "info");
    NuSpecialFind(things_scene, &TopShelf[0].special, "info", 1);

    TopShelf[1].type = 1;
    TopShelf[1].name[0] = '\0';
    TopShelf[1].special_name[0] = '\0';
    memset(&TopShelf[1].special, 0, sizeof(TopShelf[1].special));

    TopShelf[2].type = 2;
    NuStrCpy(TopShelf[2].name, "Extra");
    NuStrCpy(TopShelf[2].special_name, "tool_box");
    NuSpecialFind(world->current_gscn, &TopShelf[2].special, "tool_box", 1);

    TopShelf[3].type = 3;
    NuStrCpy(TopShelf[3].name, "Code");
    NuStrCpy(TopShelf[3].special_name, "shop_question");
    NuSpecialFind(world->current_gscn, &TopShelf[3].special, "shop_question", 1);

    TopShelf[4].type = 4;
    NuStrCpy(TopShelf[4].name, "Gold Bricks");
    NuStrCpy(TopShelf[4].special_name, "gold_brick");
    NuSpecialFind(things_scene, &TopShelf[4].special, "gold_brick", 1);

    TopShelf[5].type = 5;
    NuStrCpy(TopShelf[5].name, "Cut Scenes");
    NuStrCpy(TopShelf[5].special_name, "FMV");
    NuSpecialFind(world->current_gscn, &TopShelf[5].special, "fmv", 1);

    memset(codelist, 0, sizeof(codelist));
    SHOPCHARCOUNT = 0;
    i32 code_count = 0;
    for (i32 i = 0; i < ShopCollection.count_y && i < 100; ++i) {
        COLLECTID *collect = &ShopCollection.list[i];
        shopitem_s *item = &CharItems[SHOPCHARCOUNT];
        item->unlocked = 0;
        item->type = 1;

        if (((static_cast<u64>(Game.shop_character_purchased_bits[SHOPCHARCOUNT >> 5]) >> (SHOPCHARCOUNT & 0x1f)) &
             1) != 0) {
            item->unlocked = 1;
        }

        const i32 character_id = collect->id;
        if (CDataList[character_id].name_id == -1) {
            NuStrCpy(item->name, CDataList[character_id].dir);
        } else {
            NuStrCpy(item->name, TTab[CDataList[character_id].name_id]);
        }
        memset(&item->special, 0, sizeof(item->special));

        if (code_count <= 143) {
            codelist[code_count] = CRC_ProcessStringIgnoreCase(collect->cheat_code);
            ++code_count;
        }
        ++SHOPCHARCOUNT;
    }

    UpdateCharacterIDs();
    charcheatix = code_count;
    for (i32 i = 0; i < 44; ++i) {
        if (code_count < 144)
            codelist[code_count++] = CRC_ProcessStringIgnoreCase(Cheat[i].code);
    }
    extracheatix = code_count;
    NuSpecialFind(things_scene, &iconback, "icon_back_neutral", 1);

    NuSpecialFind(WORLD->current_gscn, &infoblank, "info_blank", 1);
    NuSpecialFind(WORLD->current_gscn, &cutblank, "fmv_blank", 1);
    NuSpecialFind(WORLD->current_gscn, &cutfilm_unlocked, "shop_film1", 1);
    NuSpecialFind(WORLD->current_gscn, &cutfilm_locked, "shop_film1b", 1);
    NuSpecialFind(WORLD->current_gscn, &toolblank, "tool_blank", 1);
    NuSpecialFind(WORLD->current_gscn, &codeblank, "code_blank", 1);
    NuSpecialFind(things_scene, &question, "question_icon", 1);
    NuSpecialFind(WORLD->current_gscn, &arrow1, "shop_arrow1", 1);
    NuSpecialFind(WORLD->current_gscn, &arrow2, "shop_arrow2", 1);
    NuSpecialFind(WORLD->current_gscn, &arrow3, "shop_arrow3", 1);
    NuSpecialFind(WORLD->current_gscn, &arrow4, "shop_arrow4", 1);
    NuSpecialSetVisibility(&arrow1, 0);
    NuSpecialSetVisibility(&arrow2, 0);
    NuSpecialSetVisibility(&arrow3, 0);
    NuSpecialSetVisibility(&arrow4, 0);

    InitExtraList();
    InitAlphaList();

    for (i32 i = 0; i < SHOPHINTCOUNT; ++i) {
        HINT_s *hint = Hint_FindHint(HintTab[i]);
        shopitem_s *item = &HintItems[i];
        item->item_id = i;
        item->type = 0;
        item->price = hint != NULL ? hint->shop_price : 0;
        item->unlocked = SAVE_OFF;
        if ((static_cast<u64>(Game.shop_hint_purchased_bits[i >> 5]) >> (i & 31)) & 1)
            item->unlocked = SAVE_ON;
        if (item->price == 0) {
            item->unlocked = 1;
        }
        item->special = atoz0to9icon[i];
    }

    shopcamspline = NuSplineFind(WORLD->current_gscn, const_cast<char *>("shop_cam"));
    if (shopcamspline == NULL) {
        return;
    }
    shopcampos = shopcamspline->pts;
    shopcamlookat = shopcamspline->pts + 1;
    LoadShelfSplines();

    if (SHOPCHARCOUNT > 0) {
        for (i32 i = 0; i < 7; ++i) {
            CharShelfIds[i] = (SHOPCHARCOUNT + i - 3) % SHOPCHARCOUNT;
            CharCurPos[i] = SubShelfPos[i];
        }
    }
    if (SHOPHINTCOUNT > 0) {
        for (i32 i = 0; i < 7; ++i) {
            HintShelfIds[i] = (SHOPHINTCOUNT + i - 3) % SHOPHINTCOUNT;
            HintCurPos[i] = SubShelfPos[i];
        }
    }
    if (SHOPEXTRACOUNT > 0) {
        for (i32 i = 0; i < 7; ++i) {
            ExtraShelfIds[i] = (SHOPEXTRACOUNT + i - 3) % SHOPEXTRACOUNT;
            ExtraCurPos[i] = SubShelfPos[i];
        }
    }

    const i32 brick_shelf_ids[7] = {11, 12, 13, 0, 1, 2, 3};
    for (i32 i = 0; i < 7; ++i) {
        BrickShelfIds[i] = brick_shelf_ids[i];
        BrickCurPos[i] = SubShelfPos[i];
    }

    for (i32 i = 0; i < SHOPGOLDBRICKS; ++i) {
        shopitem_s *item = &BrickItems[i];
        item->item_id = i;
        item->type = 4;
        item->price = 10000 + i * 5000;
        item->unlocked = SAVE_OFF;
        if ((static_cast<u64>((&Game.shop_gold_brick_purchased_bits)[i >> 5]) >> (i & 31)) & 1)
            item->unlocked = SAVE_ON;
        item->special = TopShelf[4].special;
    }

    SHOPCUTCOUNT = CutScenePlayCount;
    for (i32 i = 0; i < CutScenePlayCount && i < 128; ++i) {
        shopitem_s *item = &CutItems[i];
        item->item_id = i;
        item->type = 5;
        item->price = 0;
        item->unlocked = 0;
        item->special = TopShelf[5].special;
    }

    if (shop_from_cutsceneplayer == 0 && CutScenePlayCount > 0) {
        for (i32 i = 0; i < 7; ++i) {
            CutShelfIds[i] = (CutScenePlayCount + i - 3) % CutScenePlayCount;
            CutCurPos[i] = SubShelfPos[i];
        }
    }

    const f32 shelf_scale[6] = {0.9f, 0.28f, 0.9f, 0.95f, 0.8f, 0.9f};
    const f32 shelf_push[6] = {0.045f, 0.045f, 0.045f, 0.05f, 0.015f, 0.045f};
    const f32 current_push[6] = {0.0f, 0.0f, -0.005f, 0.0f, -0.01f, 0.0f};
    memcpy(TopShelfScale, shelf_scale, sizeof(TopShelfScale));
    memcpy(topscale, shelf_scale, sizeof(topscale));
    memcpy(TopShelfPush, shelf_push, sizeof(TopShelfPush));
    memcpy(TopBigPush, current_push, sizeof(TopBigPush));
    memcpy(toppush, current_push, sizeof(toppush));
    const f32 big_scale[6] = {1.44f, 0.448f, 1.44f, 1.5675f, 1.35f, 1.44f};
    memcpy(TopBigScale, big_scale, sizeof(TopBigScale));
    SubBigCharPush = 0.05f;
    SubNormCharPush = 0.02f;
    subpush[0] = subpush[2] = SubNormCharPush;
    subpush[1] = SubBigCharPush;
    for (i32 i = 0; i < 6; ++i) {
        codebigscale[i] = 1.254f;
        codeshelfscale[i] = codemenuscale[i] = 0.76f;
    }
    picked = oldpicked;
    subpicked = 0;
    scalenorm = 1.0f;
    scalepicked = 1.4f;
    scale2 = scale4 = scalenorm;
    scale3 = scalepicked;
    shopcutsceneplayer = CutScenePlayer_Available();
}

i32 CheckCash(shopitem_s *items, i32 item) {
    return Game.coins >= items[item].price;
}

i32 CodeMenu(MENU_s *);

extern void GameAudio_PlaySfx(i32, nuvec_s *, i32, i32);
extern void GameCam_Blend(GAMECAMERA_s *, f32, f32, i32);
extern void Hint_CancelCurrent(void);
extern void DrawSubItemMenu2D(void);
extern void DrawSubItemMenu3D(void);
extern void DrawCodeMenu(void);
extern void DrawCodeMenu3D(void);
static i32 SubItemMenu(MENU_s *);
extern void Hint_ResetHint(i32, i32);
extern void Hint_SetHintFromId(i32, i32, i32);
extern i16 HintTab[24];
extern i16 tUNKNOWN;
extern "C" void PlaySfx(char *, NUVEC *);
extern void AddToCompletionPoints(u32);

static f32 ShopClamp01(f32 value) {
    return NuFmax(0.0f, NuFmin(value, 1.0f));
}

static f32 ShopSinePhase(f32 phase) {
    const i32 angle = static_cast<i32>(phase * 32768.0f + 16384.0f);
    return (NuTrigTable[(angle >> 1) & 0x7fff] + 1.0f) * 0.5f;
}

static void Shop_GetInput(SHOPINPUT *input) {
    memset(input, 0, sizeof(*input));
    const u32 select = GAMEPAD_MENUSELECT;
    const i32 client = netclient;
    const u32 cancel = GAMEPAD_MENUCANCEL;
    const u32 left = GAMEPAD_DLEFT | GAMEPAD_TOGGLELEFT;
    const u32 right = GAMEPAD_DRIGHT | GAMEPAD_TOGGLERIGHT;
    const u32 up = GAMEPAD_DUP;
    const u32 down = GAMEPAD_DDOWN;
    for (i32 p = 0; p < 2; ++p) {
        if (!MenuPacket.active_player[p])
            continue;
        const u32 pressed = GamePad[p].buttons_pressed;
        if ((pressed & select) != 0 && client == 0) {
            input->confirm = 1;
            return;
        }
        if ((pressed & cancel) != 0) {
            input->cancel = 1;
            return;
        }
        if (client != 0)
            continue;
        const u32 held = GamePad[p].buttons_held | GamePad[p].unknown_0c;
        const u32 edge = pressed | GamePad[p].unknown_10;
        if (held & left)
            input->left_held = 1;
        if (held & right)
            input->right_held = 1;
        if (input->left_held && input->right_held)
            input->left_held = input->right_held = 0;
        if (held & up)
            input->up_held = 1;
        if (held & down)
            input->down_held = 1;
        if (input->up_held && input->down_held)
            input->up_held = input->down_held = 0;
        if (edge & left)
            input->left_pressed = 1;
        if (edge & right)
            input->right_pressed = 1;
        if (input->left_pressed && input->right_pressed)
            input->left_pressed = input->right_pressed = 0;
        if (edge & up)
            input->up_pressed = 1;
        if (edge & down)
            input->down_pressed = 1;
        if (input->up_pressed && input->down_pressed)
            input->up_pressed = input->down_pressed = 0;
    }
}

static i32 SubItemMenu(MENU_s *menu) {
    static i32 movesfxlock;
    SHOPINPUT input;
    Shop_GetInput(&input);
    if (hintdrawwait > 0.0f) {
        hintdrawwait -= FRAMETIME;
        ShopNameAlpha = 0.0f;
    }
    if (ExitMenu)
        ShopNameAlpha = 0.0f;

    shopitem_s *items = NULL;
    i32 *ids = NULL, *count = NULL;
    NUVEC *positions = NULL;
    f32 ypush = 0.0f;
    switch (picked) {
        case 0:
            items = HintItems;
            ids = HintShelfIds;
            positions = HintCurPos;
            count = &SHOPHINTCOUNT;
            if (hintdrawwait <= 0.0f && (items[ids[3]].unlocked == 1 || items[ids[3]].price == 0)) {
                Hint_ResetHint(HintTab[static_cast<u16>(items[ids[3]].item_id)], 0);
                Hint_SetHintFromId(HintTab[static_cast<u16>(items[ids[3]].item_id)], 1, 0);
            } else
                Hint_CancelCurrent();
            ypush = SubNormCharPush;
            break;
        case 1:
            items = CharItems;
            ids = CharShelfIds;
            positions = CharCurPos;
            count = &SHOPCHARCOUNT;
            ypush = SubNormCharPush;
            break;
        case 2:
            items = ExtraItems;
            ids = ExtraShelfIds;
            positions = ExtraCurPos;
            count = &SHOPEXTRACOUNT;
            ypush = SubNormCharPush;
            break;
        case 4:
            items = BrickItems;
            ids = BrickShelfIds;
            positions = BrickCurPos;
            count = &SHOPGOLDBRICKS;
            ypush = SubNormCharPush;
            break;
        case 5:
            items = CutItems;
            ids = CutShelfIds;
            positions = CutCurPos;
            count = &SHOPCUTCOUNT;
            ypush = SubNormCharPush;
            break;
    }
    if (slidetimer >= 0.0f) {
        slidetimer -= FRAMETIME;
        if (slidetimer < 0.0f)
            ShopNameAlpha = 0.0f;
    }
    if (easesubin != 0) {
        const f32 t = ShopClamp01(1.0f - ShopSinePhase(1.0f - slidetimer * 8.0f));
        if (easesubin == 1)
            inoutscale = t;
        else if (easesubin == -1)
            inoutscale = 1.0f - t;
    }

    i32 left = 0, right = 0;
    if (moveitems != 0) {
        if (movesfxlock == 0) {
            movesfxlock = 1;
            GameAudio_PlaySfx(0x2f, positions, 0, 0);
        }
        const i32 previous = moveitems;
        i32 moved = 0;
        if (previous < 0) {
            moved = MoveSubItemsLeft(ids, positions, *count);
            left = moveitems != 0 && previous != moveitems;
        } else if (previous > 0) {
            moved = MoveSubItemsRight(ids, positions, *count);
            right = moveitems != 0 && previous != moveitems;
        }
        if (moved && items != NULL && items[ids[3]].type == 0 && items[ids[3]].unlocked == 1)
            SelectSubItem();
    } else
        movesfxlock = 0;

    i32 cancel = 0, confirm = 0;
    if (slidetimer < 0.0f) {
        easesubin = 0;
        if (input.left_held)
            left = 1;
        else if (input.right_held)
            right = 1;
        else if (input.confirm && !ExitMenu)
            confirm = 1;
        else if (input.cancel)
            cancel = 1;
    }
    if (subitemselected == 1)
        subitemselected = 2;
    if (menu->confirm_pressed) {
        if (menu->selected_row == 1) {
            if (menu->selected_column == 3)
                confirm = 1;
            else {
                moveitems = menu->selected_column - 3;
                if (moveitems < 0)
                    left = 1;
                else if (moveitems > 0)
                    right = 1;
            }
        } else if (menu->selected_row == 2 && subitemselected > 0) {
            if (menu->selected_column == 0)
                confirm = 1;
            else {
                subitemselected = 0;
                PlaySfx("MenuSelect", 0);
            }
        }
    }
    if (menu->left_pressed && subitemselected == 0) {
        moveitems = -static_cast<i32>(menu->horizontal_scroll_distance / 0.15f);
        left = 1;
    }
    if (menu->right_pressed && subitemselected == 0) {
        moveitems = static_cast<i32>(menu->horizontal_scroll_distance / 0.15f);
        right = 1;
    }

    i32 scrolling = 0;
    if (!ExitMenu) {
        if (left || right) {
            scaleoverride[0] = left ? 0.0f : 1.0f;
            scaleoverride[1] = 0.4f;
            scaleoverride[2] = 0.8f;
            scaleoverride[4] = 0.8f;
            scaleoverride[5] = 0.4f;
            scaleoverride[6] = left ? 1.0f : 0.0f;
            NUVEC pos = SubShelfPos[left ? 0 : 6];
            pos.y += ypush;
            AddGameDebris(WORLD->debris_sys, qrand() / 10923 + 76, &pos);
            pos = SubShelfPos[left ? 6 : 0];
            pos.y += ypush;
            AddGameDebris(WORLD->debris_sys, qrand() / 10923 + 76, &pos);
            if (moveitems == 0)
                moveitems = left ? -1 : 1;
            slidetimer = 0.125f;
            hintdrawwait = 0.2f;
            scrolling = 1;
        }
        if (cancel) {
            easesubin = -1;
            slidetimer = 0.125f;
            ExitMenu = 1;
            GameAudio_PlaySfx(0x31, NULL, 0, 0);
        }
        if (confirm) {
            if (subitemselected > 0) {
                SelectSubItem();
                subitemselected = 0;
            } else
                subitemselected = 1;
        }
    }
    if (ExitMenu && slidetimer < 0.0f) {
        ExitMenu = 0;
        easesubin = -1;
        lastitem = -1;
        slidetimer = 0.125f;
        GameCam_Blend(GameCam, 0.6f, 0.0f, 1);
        ShopNameAlpha = 0.0f;
        return 6;
    }
    scrollkeyhit = scrolling;
    return 0;
}

i32 CodeMenu(MENU_s *) {
    static f32 timer;
    static __used__ i32 itemchanged;
    static i32 movesfxlock;
    SHOPINPUT input;
    Shop_GetInput(&input);
    if (slidetimer >= 0.0f)
        slidetimer -= FRAMETIME;
    if (timer >= 0.0f)
        timer -= FRAMETIME;
    itemchanged = 0;
    if (easesubin) {
        f32 factor = 1.0f - ShopSinePhase(1.0f - 8.0f * slidetimer);
        if (easesubin == 1)
            inoutscale = 0.0f + ShopClamp01(factor);
        else if (easesubin == -1)
            inoutscale = 1.0f - ShopClamp01(factor);
    }
    if (slidetimer <= 0.0f) {
        easesubin = 0;
        codevalid = 0;
        i32 left = 0, right = 0, down = 0, up = 0;
        if (input.left_held)
            left = 1;
        else if (input.right_held)
            right = 1;
        else if (input.down_held) {
            if (timer <= 0.0f || input.down_pressed)
                down = 1;
        } else if (input.up_held) {
            if (timer <= 0.0f || input.up_pressed)
                up = 1;
        }
        i32 confirm = input.confirm != 0;
        i32 cancel = !confirm && input.cancel != 0;
        if (down) {
            timer = 0.125f;
            movesfxlock = 1;
            if (--codechar < 0)
                codechar = 35;
            usercode[col] = codechars[codechar];
            GameAudio_PlaySfx(0x2f, &CodePos[2], 0, 0);
        }
        if (up) {
            timer = 0.125f;
            movesfxlock = 1;
            if (++codechar >= 36)
                codechar = 0;
            codevalid = 0;
            usercode[col] = codechars[codechar];
            GameAudio_PlaySfx(0x2f, &CodePos[2], 0, 0);
        }
        if (left) {
            lastitem = col;
            if (--col < 0)
                col = 0;
            codevalid = 0;
            if (!usercode[col])
                codechar = 0;
            else
                for (i32 i = 0; i < 36; ++i) {
                    if (usercode[col] == codechars[i]) {
                        codechar = i;
                        break;
                    }
                }
            if (lastitem != col) {
                slidetimer = 0.125f;
                if (!movesfxlock) {
                    movesfxlock = 1;
                    GameAudio_PlaySfx(0x2f, &CodePos[2], 0, 0);
                }
            }
        }
        if (right) {
            lastitem = col;
            if (++col > 5)
                col = 5;
            codevalid = 0;
            if (!usercode[col])
                codechar = 0;
            else
                for (i32 i = 0; i < 36; ++i) {
                    if (usercode[col] == codechars[i]) {
                        codechar = i;
                        break;
                    }
                }
            if (lastitem != col) {
                slidetimer = 0.125f;
                if (!movesfxlock) {
                    movesfxlock = 1;
                    GameAudio_PlaySfx(0x2f, &CodePos[2], 0, 0);
                }
            }
        }
        if (cancel) {
            easesubin = -1;
            slidetimer = 0.125f;
            codevalid = codechar = 0;
            ExitMenu = 1;
            GameAudio_PlaySfx(0x31, NULL, 0, 0);
        }
        if (confirm) {
            char code[7];
            code[6] = 0;
            usercode[col] = codechars[codechar];
            for (i32 i = 0; i < 6; ++i)
                code[i] = usercode[i];
            u32 crc = CRC_ProcessStringIgnoreCase(code);
            i32 i;
            for (i = 0; i < 144; ++i)
                if (codelist[i] && codelist[i] == crc)
                    break;
            if (i == 144) {
                PlaySfx("Shop_NotEnufMuny", &SubShelfPos[3]);
                cheatname = NULL;
            } else {
                codevalid = 1;
                if (i < charcheatix) {
                    u16 id = CharItems[i].item_id;
                    if (Game_CharacterSave && !(Game_CharacterSave[id] & SAVE_CHARACTER_UNLOCKED)) {
                        Game_CharacterSave[id] |= SAVE_CHARACTER_UNLOCKED;
                        AddToCompletionPoints(POINTS_PER_CHARACTER);
                        PlaySfx("Shop_BuyCheat", &SubShelfPos[3]);
                    } else
                        PlaySfx("Shop_NotEnufMuny", &SubShelfPos[3]);
                    cheatname = TTab[CDataList[id].name_id];
                } else if (i < extracheatix) {
                    i -= charcheatix;
                    if (!(Game.extra_purchased_bits[i / 32] & (1U << (i & 31)))) {
                        Game.extra_purchased_bits[i / 32] |= 1U << (i & 31);
                        ExtraItems[i].unlocked = SAVE_ON;
                        AddToCompletionPoints(POINTS_PER_CHEAT);
                        i8 area = static_cast<i8>(Cheat[i].area);
                        if (area != -1 && !Game.area_save[area].red_brick_collected) {
                            Game.area_save[area].red_brick_collected = SAVE_COMPLETE;
                            AddToCompletionPoints(POINTS_PER_REDBRICK);
                        }
                        Game.extra_unlocked_bits[i / 32] |= static_cast<u32>(1ULL << (i % 32));
                        PlaySfx("Shop_BuyCheat", &SubShelfPos[3]);
                    } else
                        PlaySfx("Shop_NotEnufMuny", &SubShelfPos[3]);
                    cheatname = TTab[Cheat[i].text_id ? *Cheat[i].text_id : tUNKNOWN];
                }
                pickedbing = 0.35f;
                cheattimer = 3.1499998569488525f;
            }
        }
    } else
        movesfxlock = 0;

    if (slidetimer >= 0.0f) {
        f32 factor = 1.0f - ShopSinePhase(1.0f - 8.0f * slidetimer);
        if (lastitem != -1) {
            factor = ShopClamp01(factor);
            if (easesubin == 1) {
                codemenuscale[lastitem] =
                    codeshelfscale[lastitem] + (codebigscale[lastitem] - codeshelfscale[lastitem]) * factor;
                factor = ShopClamp01(factor);
                subpush[2] = SubNormCharPush + (SubBigCharPush - SubNormCharPush) * factor;
            } else {
                codemenuscale[lastitem] =
                    codebigscale[lastitem] + (codeshelfscale[lastitem] - codebigscale[lastitem]) * factor;
                factor = ShopClamp01(factor);
                subpush[2] = SubBigCharPush + (SubNormCharPush - SubBigCharPush) * factor;
            }
        }
        if (col != -1) {
            factor = ShopClamp01(factor);
            if (easesubin == -1) {
                codemenuscale[col] = codebigscale[col] + (codeshelfscale[col] - codebigscale[col]) * factor;
                factor = ShopClamp01(factor);
                subpush[1] = SubBigCharPush + (SubNormCharPush - SubBigCharPush) * factor;
            } else {
                codemenuscale[col] = codeshelfscale[col] + (codebigscale[col] - codeshelfscale[col]) * factor;
                factor = ShopClamp01(factor);
                subpush[1] = SubNormCharPush + (SubBigCharPush - SubNormCharPush) * factor;
            }
        }
    } else
        lastitem = -1;
    if (ExitMenu && slidetimer < 0.0f) {
        ExitMenu = 0;
        ShopNameAlpha = 0.0f;
        easesubin = -1;
        lastitem = -1;
        subpush[1] = 0.05f;
        slidetimer = 0.125f;
        GameCam_Blend(GameCam, 1.0f, 0.0f, 1);
        return 6;
    }
    return 7;
}

i32 ItemMenu(MENU_s *menu) {
    static __used__ i32 itemchanged;
    static i32 movesfxlock;
    static i32 SubMenu;
    SHOPINPUT input;
    const i32 entry_picked = picked;
    Shop_GetInput(&input);

    if (slidetimer >= 0.0f) {
        slidetimer -= FRAMETIME;
        ShopNameAlpha = 0.0f;
    }
    if (ExitMenu)
        ShopNameAlpha = 0.0f;

    i32 candidate = entry_picked;
    i32 cancel = 0;
    i32 selection = -1;
    if (menu->confirm_pressed != 0 && menu->selected_row == 0) {
        candidate = static_cast<i32>(menu->selected_column) + 1;
        if (candidate == entry_picked) {
            input.value[8] = 1;
        }
    }

    itemchanged = 0;
    Hint_CancelCurrent();
    hintdrawwait = 0.2f;

    if (slidetimer < 0.0f) {
        movesfxlock = 0;

        if (input.value[0] != 0 && picked > 1) {
            candidate = picked - 1;
            if (candidate == 4 && SHOPGOLDBRICKS == 0)
                candidate = picked - 2;
            if (candidate == 3)
                candidate = 2;
        } else if (input.value[1] != 0 && picked <= 3) {
            candidate = picked + 1;
            if (candidate == 3)
                candidate = picked + 2;
            if (candidate == 4 && SHOPGOLDBRICKS == 0)
                candidate = picked;
        } else if (input.value[8] != 0) {
            if (picked == 0) {
                picked = 1;
            } else if (picked == 5) {
                picked = SHOPGOLDBRICKS != 0 ? 4 : 3;
            } else
                selection = picked;
            if (selection == -1)
                candidate = entry_picked;
        } else if (input.value[9] != 0) {
            cancel = 1;
        }
    }

    if (candidate != entry_picked) {
        lastitem = picked;
        SubMenu = 0;
        picked = candidate <= 0 ? 1 : candidate > 5 ? 5 : candidate;

        slidetimer = 0.125f;
        if (movesfxlock == 0) {
            movesfxlock = 1;
            GameAudio_PlaySfx(0x2f, reinterpret_cast<nuvec_s *>(SubShelfPos) + 5, 0, 0);
        }
        GameCam_Blend(GameCam, 0.3f, 0.0f, 1);
        ShopNameAlpha = 0.0f;
    }

    if (selection != -1) {
        if (selection != picked) {
            picked = selection;
            if (!movesfxlock) {
                movesfxlock = 1;
                GameAudio_PlaySfx(0x2f, &SubShelfPos[5], 0, 0);
            }
            ShopNameAlpha = 0.0f;
        }
        lastitem = picked;
        picked = -1;
        slidetimer = 0.125f;
        SubMenu = 1;
        GameAudio_PlaySfx(0x30, NULL, 0, 0);
        GameCam_Blend(GameCam, 0.6f, 0.0f, 1);
        ShopNameAlpha = 0.0f;
    }
    if (cancel) {
        ExitMenu = 1;
        slidetimer = 0.125f;
        lastitem = picked;
        picked = -1;
        SubMenu = 0;
        GameAudio_PlaySfx(0x31, NULL, 0, 0);
    }

    if (slidetimer < 0.0f)
        enteredshop = 0;
    else if ((lastitem != picked || enteredshop) && !SubMenu) {
        const f32 reverse_phase = slidetimer * 8.0f;
        const f32 old_factor = ShopClamp01(1.0f - ShopSinePhase(1.0f - reverse_phase));
        const f32 forward_factor = ShopClamp01(1.0f - ShopSinePhase(reverse_phase));

        if (lastitem != -1) {
            topscale[lastitem] = TopBigScale[lastitem] + (TopShelfScale[lastitem] - TopBigScale[lastitem]) * old_factor;
            toppush[lastitem] = TopBigPush[lastitem] + (TopShelfPush[lastitem] - TopBigPush[lastitem]) * forward_factor;
        }

        if (picked != -1 && lastitem != -1) {
            topscale[picked] = TopShelfScale[picked] + (TopBigScale[picked] - TopShelfScale[picked]) * old_factor;
            toppush[picked] = TopShelfPush[picked] + (TopBigPush[picked] - TopShelfPush[picked]) * forward_factor;
        }
    }

    if (SubMenu == 0) {
        if (ExitMenu && slidetimer < 0.0f) {
            menuptr = nullptr;
            return 5;
        }
        return 7;
    }

    if (slidetimer > 0.0f)
        return 7;

    picked = lastitem;
    SubMenu = 0;
    if (TopShelf[lastitem].type == 3) {
        lastitem = -1;
        easesubin = 1;
        inoutscale = 0.0f;
        cheatname = 0;
        slidetimer = 0.125f;
        shopmenu = 11;
        col = 0;

        ++currentmenulevel;
        menuparent[currentmenulevel] = menuptr;
        menuptr = CodeMenu;
        memcpy(usercode, "AAAAAA", 6);

        ++currentdrawlevel;
        drawparent[currentdrawlevel] = drawpanelptr;
        drawpanelptr = DrawCodeMenu;
        drawptr = DrawCodeMenu3D;
    } else {
        easesubin = 1;
        ++currentmenulevel;
        inoutscale = 0.0f;
        menuparent[currentmenulevel] = menuptr;
        menuptr = SubItemMenu;

        ++currentdrawlevel;
        slidetimer = 0.125f;
        drawparent[currentdrawlevel] = drawpanelptr;
        drawpanelptr = DrawSubItemMenu2D;
        drawptr = DrawSubItemMenu3D;
    }
    return 7;
}
extern i16 tSELECT, tEXIT, tBACK, tBUY, tPLAY, tSELECTING;
void DrawPlayerIconPrompts(i32, i32, f32, i32, i32, i32, i32, i32, i32, f32, i32, i32, i32, i32);

void DrawShopPrompts() {
    i32 select = -1;
    i32 back = -1;
    f32 alpha = 1.0f;
    if (menuptr == ItemMenu) {
        select = tSELECT;
        back = tEXIT;
    } else if (menuptr == SubItemMenu) {
        select = picked == 5 ? tPLAY : tBUY;
        back = tBACK;
        alpha = 0.25f;
        switch (picked) {
            case 0: {
                shopitem_s *item = &HintItems[HintShelfIds[3]];
                if (item->price <= Game.coins && item->unlocked != 1)
                    alpha = 1.0f;
                break;
            }
            case 1: {
                shopitem_s *item = &CharItems[CharShelfIds[3]];
                if (item->price <= Game.coins && CollectIDUnlocked(static_cast<u16>(item->item_id)) &&
                    item->unlocked != 1)
                    alpha = 1.0f;
                break;
            }
            case 2: {
                shopitem_s *item = &ExtraItems[ExtraShelfIds[3]];
                u16 id = item->item_id;
                i8 area = static_cast<i8>(Cheat[id].area);
                if (item->price <= Game.coins && (id <= 7 || area == -1 || Game.area_save[area].red_brick_collected) &&
                    !(Game.extra_purchased_bits[id >> 5] >> (id & 31) & 1) && item->unlocked != 1)
                    alpha = 1.0f;
                break;
            }
            case 4: {
                i32 id = BrickShelfIds[3];
                if (BrickItems[id].price <= Game.coins && static_cast<f32>(id * 3600) <= Game.field30_0x7c2c)
                    alpha = 1.0f;
                break;
            }
            case 5:
                if (CutScenePlayer_CanStart(CutShelfIds[3]))
                    alpha = 1.0f;
                break;
        }
    } else if (menuptr == CodeMenu) {
        select = tSELECT;
        back = tBACK;
    }
    if (netclient)
        select = -1;
    if (select != -1 || back != -1)
        DrawPlayerIconPrompts(MenuPacket.active_player[0], select, alpha, -1, back, -1, tSELECTING,
                              MenuPacket.active_player[1], select, alpha, -1, back, -1, tSELECTING);
}

void LoadShelfSplines() {
    splshelf = NuSplineFind(WORLD->current_gscn, const_cast<char *>("shelf_top1"));
    splcharshelf = NuSplineFind(WORLD->current_gscn, const_cast<char *>("shelf_bottom"));
    splcodes = NuSplineFind(WORLD->current_gscn, const_cast<char *>("shelf_6"));

    if (splshelf == NULL || splcharshelf == NULL || splcodes == NULL) {
        return;
    }
    const i32 shelf_count = static_cast<i16>(splshelf->length);
    const i32 character_count = static_cast<i16>(splcharshelf->length);
    const i32 code_count = static_cast<i16>(splcodes->length);

    memset(SubShelfPos, 0, sizeof(SubShelfPos));
    memset(ShelfPos, 0, sizeof(ShelfPos));
    memset(CodePos, 0, 6 * sizeof(*CodePos));

    for (i32 i = 0; i < shelf_count; ++i) {
        ShelfPos[i] = splshelf->pts[i];
    }
    ShelfPos[2].x += (ShelfPos[3].x - ShelfPos[2].x) * 0.5f;
    ShelfPos[2].z += (ShelfPos[3].z - ShelfPos[2].z) * 0.5f;
    for (i32 i = 0; i < character_count; ++i) {
        SubShelfPos[i] = splcharshelf->pts[i];
    }
    for (i32 i = 0; i < code_count; ++i) {
        CodePos[i] = splcodes->pts[i];
    }

    NUVEC diff;
    NuVecSub(&diff, &CodePos[5], &CodePos[4]);
    NuVecAdd(&CodePos[6], &CodePos[5], &diff);
    CodePos[6].y += 0.02f;
    NuVecScale(&diff, &diff, 0.5f);
    NuVecSub(&CodePos[0], &CodePos[0], &diff);
    NuVecSub(&CodePos[1], &CodePos[1], &diff);
    NuVecSub(&CodePos[2], &CodePos[2], &diff);
    NuVecSub(&CodePos[3], &CodePos[3], &diff);
    NuVecSub(&CodePos[4], &CodePos[4], &diff);
    NuVecSub(&CodePos[5], &CodePos[5], &diff);
    NuVecSub(&CodePos[6], &CodePos[6], &diff);

    NUVEC start = splshelf->pts[0];
    NUVEC end = splshelf->pts[shelf_count];
    NuVecSub(&diff, &end, &start);
    shelfang = static_cast<u16>(NuAtan2D(diff.x, diff.z));
}

void DrawShop3D(WORLDINFO_s *world) {
    if (drawptr != NULL) {
        drawptr();
    }
    DrawTopShelf(picked);
    SetLevelLights(world->rtl_set, 1.0f);
}


void DrawCodeMenu() {
    if (cheattimer > 0.0f) {
        if (cheatname && cheattimer < 3.1499998569488525f && NuFmod(cheattimer, 0.35f) < 0.175f) {
            SmartTextEx(cheatname, 0.0f, (HUB_EPISODETITLEY + HUB_EPISODESUBTITLEY) * 0.5f, 1.0f, 0.8f, 0.8f, 0.8f, 0,
                        0, 255, 0, 1.7f, 1, NULL, 0, 128);
        }
        cheattimer -= FRAMETIME;
    }
}


// The original shop inlines these NuMtx rotations (0x244679 onward).
// Keep the arithmetic in sync with the canonical numtx.cpp implementations.
static inline void ShopRotateX(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m01 = m->m01;
    f32 m11 = m->m11;
    f32 m21 = m->m21;
    f32 m31 = m->m31;

    m->m01 = m01 * cosx - m->m02 * sinx;
    m->m02 = m01 * sinx + m->m02 * cosx;
    m->m11 = m11 * cosx - m->m12 * sinx;
    m->m12 = m11 * sinx + m->m12 * cosx;
    m->m21 = m21 * cosx - m->m22 * sinx;
    m->m22 = m21 * sinx + m->m22 * cosx;
    m->m31 = m31 * cosx - m->m32 * sinx;
    m->m32 = m31 * sinx + m->m32 * cosx;
}

static inline void ShopRotateY(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m10 = m->m10;
    f32 m20 = m->m20;
    f32 m30 = m->m30;

    m->m00 = m00 * cosx + m->m02 * sinx;
    m->m02 = m->m02 * cosx - m00 * sinx;
    m->m10 = m10 * cosx + m->m12 * sinx;
    m->m12 = m->m12 * cosx - m10 * sinx;
    m->m20 = m20 * cosx + m->m22 * sinx;
    m->m22 = m->m22 * cosx - m20 * sinx;
    m->m30 = m30 * cosx + m->m32 * sinx;
    m->m32 = m->m32 * cosx - m30 * sinx;
}

static inline void ShopRotateZ(NUMTX *m, NUANG a) {
    f32 cosx = NU_COS_LUT(a);
    f32 sinx = NU_SIN_LUT(a);
    f32 m00 = m->m00;
    f32 m10 = m->m10;
    f32 m20 = m->m20;
    f32 m30 = m->m30;

    m->m00 = m00 * cosx - m->m01 * sinx;
    m->m01 = m00 * sinx + m->m01 * cosx;
    m->m10 = m10 * cosx - m->m11 * sinx;
    m->m11 = m10 * sinx + m->m11 * cosx;
    m->m20 = m20 * cosx - m->m21 * sinx;
    m->m21 = m20 * sinx + m->m21 * cosx;
    m->m30 = m30 * cosx - m->m31 * sinx;
    m->m31 = m30 * sinx + m->m31 * cosx;
}
static void Shop_DrawCharacter(shopitem_s *item, NUVEC *position, f32 scale_value, f32 ypush, u16 xrot, u16 yrot,
                               u16 zrot);
extern AREADATA *ANEWHOPE_ADATA;


void DrawSubItems() {
    f32 alpha = 1.0f;
    if (GetMenuID() == 13 && !TestForController()) {
        const f32 since_touch = GlobalTimer.time_elapsed - (LastTouchTime + 2.64f);
        if (since_touch > 4.0f) {
            const f32 phase = NuFmod(since_touch, 4.0f);
            const i32 angle = static_cast<i32>(phase * 0.25f * 65536.0f);
            const f32 pulse = NuTrigTable[(angle >> 1) & 0x7fff] - 0.8f;
            if (pulse >= 0.0f) {
                alpha = pulse + 1.0f;
            }
        }
    }

    f32 bing = 0.0f;
    i32 phase_angle = 0x2000;
    if (pickedbing > 0.0f) {
        bing = pickedbing / 0.35f;
        pickedbing -= FRAMETIME;
        phase_angle = (static_cast<i32>(pickedbing * 32768.0f + 16384.0f) >> 1) & 0x7fff;
    } else {
        pickedbing = 0.0f;
    }
    f32 bing_scale = 1.0f - 0.5f * (NuTrigTable[phase_angle] + 1.0f);

    shopitem_s *items = NULL;
    i32 *shelf_ids = NULL;
    NUVEC *positions = NULL;
    f32 base_scale = 0.9f;
    switch (picked) {
        case 0:
            items = HintItems;
            shelf_ids = HintShelfIds;
            positions = HintCurPos;
            break;
        case 1:
            items = CharItems;
            shelf_ids = CharShelfIds;
            positions = CharCurPos;
            base_scale = 0.9f;
            break;
        case 2:
            items = ExtraItems;
            shelf_ids = ExtraShelfIds;
            positions = ExtraCurPos;
            break;
        case 3:
            return;
        case 4:
            items = BrickItems;
            shelf_ids = BrickShelfIds;
            positions = BrickCurPos;
            break;
        case 5:
            items = CutItems;
            shelf_ids = CutShelfIds;
            positions = CutCurPos;
            break;
        default:
            return;
    }
    const f32 item_scale = picked == 1 ? 0.28f : (picked == 4 ? 0.8f : base_scale);
    MENU *menu = &GameMenu[GameMenuLevel];
    for (i32 index = 0; index < 7; ++index) {
        i32 slot = index;
        if (index == 6) {
            slot = 3;
        } else if (index >= 3) {
            slot = index - 1;
        }

        if (moveitems > 0 && slot == 0)
            continue;
        const i32 item_id = shelf_ids[slot];
        shopitem_s *item = &items[item_id];
        NUVEC position = positions[slot];
        f32 scale = item_scale * inoutscale;
        f32 ypush = SubNormCharPush;
        u16 rotation = 0;
        if (slot == 0 || (slot == 1 && moveitems > 0)) {
            scale = item_scale * inoutscale * scaleoverride[0];
            ypush *= scaleoverride[0];
        } else if (slot == 2) {
            scale = item_scale * scale2 * inoutscale;
            ypush = subpush[0];
        } else if (slot == 3) {
            scale = item_scale * scale3 * inoutscale + 0.5f * bing_scale;
            ypush = subpush[1];
            if (pickedbing >= 0.0f)
                rotation = static_cast<i32>(-bing * 65536.0f);
        } else if (slot == 4) {
            scale = item_scale * scale4 * inoutscale;
            ypush = subpush[2];
        }
        if (!subitemselected)
            scale *= alpha;
        if (subitemselected == 2 && slot == 3) {
            NuVecAdd(&position, &position, &selectedoff);
            scale *= 1.25f;
        }
        NUVEC screen;
        NUVEC hit_position = position;
        hit_position.y += ypush;
        f32 aspect = GetAspectRatio();
        NuCameraTransformScreenClip(&screen, &hit_position, 1, NULL);
        menu->item_x[slot + 4] = screen.x;
        menu->item_y[slot + 4] = screen.y;
        f32 hit_size = 0.0f;
        f32 hit_width = 0.0f;
        if (subitemselected < 1) {
            hit_size = (scale / item_scale) * 0.15f;
            hit_width = hit_size / aspect;
        }
        menu->item_width[slot + 4] = hit_size;
        menu->item_height[slot + 4] = hit_width;
        menu->item_column[slot + 4] = slot;
        menu->item_row[slot + 4] = 1;

        u16 angle = shelfang;
        switch (picked) {
            case 0: {
                if (item->unlocked == 1) {
                    u16 spin = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 4.0f) * 0.25f * 65536.0f);
                    if (pickedbing > 0.0f && slot == 3)
                        spin += rotation;
                    angle += spin;
                    DrawItem(&TopShelf[0].special, &position, scale, 1.0f, ypush, 0, angle, 0);
                } else {
                    DrawItem(&infoblank, &position, scale, 1.0f, ypush, 0, angle, 0);
                    /* This is intentionally the original's odd TopShelf offset. */
                    DrawItem(reinterpret_cast<nuhspecial_s *>(reinterpret_cast<u8 *>(TopShelf) + 0x1c4), &position,
                             scale, 1.0f, ypush, 0, angle, 0);
                }
                break;
            }
            case 1: {
                u16 phase = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 1.75f) / 1.75f * 65536.0f);
                f32 oscillation = NuTrigTable[((phase + 0x4000) >> 1) & 0x7fff];
                if (item->unlocked == 1)
                    angle += static_cast<i32>(oscillation * 4369.0f);
                Shop_DrawCharacter(item, &position, scale, ypush, 0, angle, 0);
                break;
            }
            case 2: {
                if (item->unlocked == 1) {
                    u16 spin = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 4.0f) * 0.25f * 65536.0f);
                    if (pickedbing > 0.0f && slot == 3)
                        spin += rotation;
                    angle += spin;
                }
                DrawItem(&toolblank, &position, scale, 1.0f, ypush, 0, angle, 0);
                u16 id = item->item_id;
                i8 area = static_cast<i8>(Cheat[id].area);
                nuhspecial_s *special = &item->special;
                if (!(Game.extra_purchased_bits[id >> 5] >> (id & 31) & 1) && id > 7 && area != -1 &&
                    !Game.area_save[area].red_brick_collected)
                    special = &extrasils[item_id];
                if (NuSpecialExistsFn(special) != 0) {
                    NUMTX_ALIGNED16 matrix;
                    NUANGVEC angles = {0, angle, 0};
                    NuMtxSetRotateXYZVU0(&matrix, &angles);
                    NUVEC size;
                    size.x = size.y = size.z = scale;
                    NuMtxScaleVU0(&matrix, &size);
                    *reinterpret_cast<NUVEC *>(&matrix.m30) = position;
                    matrix.m31 += ypush;
                    NuSpecialDrawAt(special, &matrix);
                }
                break;
            }
            case 4: {
                if (item->unlocked == 1) {
                    u16 spin = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 4.0f) * 0.25f * 65536.0f);
                    if (pickedbing > 0.0f && slot == 3)
                        spin += rotation;
                    angle += spin + static_cast<u16>(item->item_id * 0x1249) - 0x2000;
                }
                if (NuSpecialExistsFn(&item->special) != 0) {
                    NUMTX_ALIGNED16 matrix;
                    NUVEC size = {scale, scale, scale};
                    NuMtxSetScale(&matrix, &size);
                    ShopRotateX(&matrix, 0);
                    ShopRotateY(&matrix, angle);
                    ShopRotateZ(&matrix, 0);
                    NuMtxTranslate(&matrix, &position);
                    matrix.m31 += ypush;
                    NuSpecialDrawAt(&item->special, &matrix);
                }
                break;
            }
            case 5: {
                nuhspecial_s *blank = &cutblank;
                nuhspecial_s *film = &cutfilm_locked;
                if (shopcutsceneplayer != NULL && CutScenePlayer_CanStart(item->item_id) != 0) {
                    blank = &toolblank;
                    CUTSCENEPLAYER_s *clips = static_cast<CUTSCENEPLAYER_s *>(shopcutsceneplayer);
                    LEVELDATA *level = &LDataList[clips->clips[item->item_id].level_id];
                    if (level->episode_index != -1) {
                        if (level->episode_index & 1)
                            blank = &codeblank;
                    } else if (level->area_index != -1 && ANEWHOPE_ADATA && level->area_index == ANEWHOPE_ADATA->index)
                        blank = &codeblank;
                    rotation = static_cast<i32>(NuFmod(GameTimer.time_elapsed, 4.0f) * 0.25f * 65536.0f) +
                               static_cast<u16>(item->item_id * 0x1555);
                    film = &cutfilm_unlocked;
                }
                if (item->unlocked == 1)
                    angle += rotation;
                DrawItem(blank, &position, scale, 1.0f, ypush, 0, angle, 0);
                DrawItem(film, &position, scale, 1.0f, ypush, 0, angle, 0);
                break;
            }
            default:
                break;
        }
    }
}


static void Shop_DrawCharacter(shopitem_s *item, NUVEC *position, f32 scale_value, f32 ypush, u16 xrot, u16 yrot,
                               u16 zrot) {
    if (!NuSpecialExistsFn(&iconback))
        return;
    i32 top_shelf_character = 0;
    if (item == &TopShelf[1]) {
        top_shelf_character = 1;
        const f32 cycle_length = static_cast<f32>(SHOPCHARCOUNT) * 0.2f;
        const i32 character_index = static_cast<i32>(NuFmod(GameTimer.time_elapsed, cycle_length) / 0.2f);
        item = &CharItems[character_index];
    }

    const i32 character_id = static_cast<u16>(item->item_id);
    const bool unlocked = CollectIDUnlocked(character_id) != NULL;
    const i32 unavailable = !unlocked || item->unlocked != 1;
    f32 alpha = 1.0f;
    if (!CollectIDUnlocked(character_id))
        alpha = 0.5f;

    NUVEC scale;
    scale.x = scale.y = scale.z = scale_value;
    NUMTX_ALIGNED16 matrix;
    NuMtxSetScale(&matrix, &scale);
    ShopRotateX(&matrix, xrot);
    ShopRotateY(&matrix, yrot);
    ShopRotateZ(&matrix, zrot);
    NuMtxTranslate(&matrix, position);
    matrix.m31 += ypush;

    NuSpecialDrawAtAlpha(&iconback, &matrix, top_shelf_character ? 1.0f : alpha);

    i32 icon_object_id = CDataList[character_id].field20_0x42;
    if (icon_object_id != -1) {
        icon_object_id += unavailable;
        WORLDINFO_s *world = WORLD;
        LEVEL_OBJECT_RUNTIME_s *icon = &world->lev_objs[icon_object_id];
        if (icon->active != 0) {
            NuSpecialDrawAtAlpha(&icon->special, &matrix, alpha);
        }
    }
}


void DrawTopShelf(i32) {
    f32 alpha_scale = 1.0f;
    if (GetMenuID() == 13 && subitemselected == 0 && TestForController() == 0) {
        const f32 elapsed_since_touch = GlobalTimer.time_elapsed - (LastTouchTime + 1.32f);
        if (elapsed_since_touch > 4.0f) {
            const f32 phase = NuFmod(elapsed_since_touch, 4.0f);
            const i32 angle = static_cast<i32>(phase * 0.25f * 65536.0f);
            const f32 pulse = NuTrigTable[(angle >> 1) & 0x7fff] - 0.8f;
            if (pulse >= 0.0f) {
                alpha_scale = pulse + 1.0f;
            }
        }
    }

    Shop_DrawCharacter(&TopShelf[1], &ShelfPos[1], topscale[1], toppush[1], 0, shelfang, 0);

    if (NuSpecialExistsFn(&TopShelf[2].special) != 0) {
        NUANGVEC rotation = {0, shelfang, 0};
        NUMTX_ALIGNED16 matrix;
        NuMtxSetRotateXYZVU0(&matrix, &rotation);
        const f32 scale_value = topscale[2] * alpha_scale;
        NUVEC scale = {scale_value, scale_value, scale_value};
        NuMtxScaleVU0(&matrix, &scale);
        matrix.m30 = ShelfPos[2].x;
        matrix.m31 = ShelfPos[2].y + toppush[2] + 0.005f;
        matrix.m32 = ShelfPos[2].z;
        NuSpecialDrawAt(&TopShelf[2].special, &matrix);
    }

    if (SHOPGOLDBRICKS > 0 && NuSpecialExistsFn(&TopShelf[4].special) != 0) {
        NUVEC scale = {topscale[4] * alpha_scale, topscale[4] * alpha_scale, topscale[4] * alpha_scale};
        NUMTX_ALIGNED16 matrix;
        NuMtxSetScale(&matrix, &scale);
        ShopRotateX(&matrix, 0);
        ShopRotateY(&matrix, static_cast<u16>(shelfang + 0x2000));
        ShopRotateZ(&matrix, 0);
        NuMtxTranslate(&matrix, &ShelfPos[4]);
        matrix.m31 += toppush[4] - 0.0325f;
        NuSpecialDrawAt(&TopShelf[4].special, &matrix);
    }

    if (GetMenuID() != 13) {
        return;
    }

    MENU *menu = &GameMenu[GameMenuLevel];
    for (i32 shelf_index = 1; shelf_index <= 4; ++shelf_index) {
        const i32 menu_index = shelf_index - 1;
        if (shelf_index == 3) {
            menu->item_width[menu_index] = 0.0f;
            continue;
        }

        NUVEC world_position = ShelfPos[shelf_index];
        world_position.y += toppush[shelf_index];
        const f32 size = topscale[shelf_index] / TopShelfScale[shelf_index] * 0.15f;
        const f32 aspect_ratio = GetAspectRatio();
        NUVEC screen_position;
        NuCameraTransformScreenClip(&screen_position, &world_position, 1, NULL);
        menu->item_x[menu_index] = screen_position.x;
        menu->item_y[menu_index] = screen_position.y;
        menu->item_width[menu_index] = size;
        menu->item_height[menu_index] = size / aspect_ratio;
        menu->item_column[menu_index] = menu_index;
        menu->item_row[menu_index] = 0;
    }
}


void DrawShopPanel() {
    if (SHOPACTIVE && drawpanelptr)
        drawpanelptr();
}


void DrawCodeMenu3D() {
    i32 letters[6];
    u16 angle = shelfang;
    for (i32 i = 0; i < 6; ++i) {
        if (usercode[i] >= 'A' && usercode[i] <= 'Z')
            letters[i] = usercode[i] - 'A';
        else if (usercode[i] >= '0' && usercode[i] <= '9')
            letters[i] = usercode[i] - '0' + 26;
    }
    f32 bing = 0.0f;
    if (pickedbing > 0.0f) {
        bing = pickedbing / 0.35f * 0.5f;
        pickedbing -= FRAMETIME;
    } else
        pickedbing = 0.0f;
    for (i32 i = 0; i < 6; ++i) {
        f32 scale = 0.95f * codemenuscale[i] * inoutscale;
        f32 push = SubNormCharPush;
        if (col == i) {
            scale += bing * 0.5f;
            push = subpush[1];
        } else if (lastitem == i) {
            scale += bing * 0.5f;
            push = subpush[2];
        }
        DrawItem(&codeblank, &CodePos[i], scale, 1.0f, push, 0, angle, 0);
        DrawItem(&atoz0to9icon[letters[i]], &CodePos[i], scale, 1.0f, push, 0, angle, 0);
    }
    static f32 arrowbing;
    arrowbing = 0.5f * NuTrigTable[(static_cast<i32>(inoutscale * 32768.0f) >> 1) & 0x7fff];
    f32 scale = inoutscale * 0.76f;
    Draw3DObject(WORLD, 317, &CodePos[6], 0, shelfang, 0, scale, scale, scale, 2);
    enum { SHOP_CODE_ARROW_MESSAGE_FLAGS = 0x83 };
    AddGameMessage(">", &CodePos[6], 0.65f * inoutscale, NULL, 0.0f, 255, 255, 255, SHOP_CODE_ARROW_MESSAGE_FLAGS,
                   0.0f);
}
