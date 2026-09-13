#include "decomp.h"
#include "globals.h"
#include "batman.h"
#include "legoapi/core/input/qrand.h"
#include "legoapi/world/mission.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/core/render.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nurndr.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuprim.h"

struct AIROW_s;
struct nuqthdr_s;
struct nunativegscene_s;
struct SHOPINPUT;

extern "C" void SetQFont2D(void);
extern "C" i32 NuRndrBeginScene(i32 flags);
extern "C" void NuRndrClear(i32 flags, i32 colour, f32 alpha);
extern "C" void NuRndrEndScene(void);
extern FadeSystem FadeSys;
extern NUCAMERA *pNuCam;
extern "C" f32 NuIOS_GetAspectRatio(void);
void SetPanelLights(f32 intensity);
void TimingBars(void);
void Arcade_ResetPanel(void);
void TBOPENFN(char *name, i32 bar);
void TBCLOSEFN(char *name, i32 bar);

extern NUMTL *FadeMtl2;
extern i32 TimingBarSet;

extern f32 statstime;
extern f32 cointotaltime;
extern f32 goldbricktime;
extern i32 SuperStory;

static f32 redbrickslidetime;

void PanelRender(WORLDINFO_s *) {
    NuRndrBeginScene(-1);
    SetQFont2D();
    SetPanelLights(1.0f);
    NuRndrClear(0xe00, 0xff000000, 1.0f);
    DrawPauseFade();
    if (TimingBarSet == 5) {
        TBOPENFN("Panel", 5);
    }
    DrawPanel();
    if (TimingBarSet == 5) {
        TBCLOSEFN("Panel", 5);
    }
    TimingBars();
    FadeSys.Update();
    static_cast<ThingManager *>(theGameThings)->DisplayThings(NULL);
    NuRndrEndScene();
    FadeSys.Draw();

    if (WORLD != NULL && WORLD->current_level == TITLES_LDATA) {
        i32 colour = static_cast<i32>((1.0f - newgamealpha) * 255.0f);

        NuRndrBeginScene(-1);
        colour <<= 24;
        ++NuPrimCSPos;
        NuPrimSetCoordinateSystem(NUPRIM_SCALEMODE_ABSOLUTE);
        NuPrim2DBegin(4, 5, FadeMtl2);

        struct PanelFadeVertex {
            f32 x;
            f32 y;
            f32 z;
            u32 colour;
        };

        PanelFadeVertex *vertex = reinterpret_cast<PanelFadeVertex *>(g_NuPrim_StreamBufferPtr->void_ptr);
        if (g_NuPrim_NeedsOverbrightening == 0) {
            vertex->colour = colour & 0xff000000u;
        } else {
            vertex->colour = colour;
        }
        NuPrim2DAddXYZ(0.0f, 0.0f, 0.0f);

        vertex = reinterpret_cast<PanelFadeVertex *>(g_NuPrim_StreamBufferPtr->void_ptr);
        if (g_NuPrim_NeedsOverbrightening == 0) {
            vertex->colour = colour & 0xff000000u;
        } else {
            vertex->colour = colour;
        }
        NuPrim2DAddXYZ(1.0f, 1.0f, 0.0f);

        NuPrim2DEnd();
        --NuPrimCSPos;
        NuPrimSetCoordinateSystem(NuPrimCoordSystemStack[NuPrimCSPos]);
        NuRndrEndScene();
    }
}

void Panel_Clear() {
    statstime = 0.0f;
    DrawMiniKitTime = 0.0f;
    MiniKitScale = 1.0f;
    DrawBuildUpTime = 0.0f;
    builduptime = 0.0f;
    BuildUpScale = 1.0f;
    DrawRedBrickTime = 0.0f;
    RedBrickScale = 1.0f;
    DrawCoinTotalTime = 0.0f;
    cointotaltime = 0.0f;
    CoinTotalScale = 1.0f;
    redbrickslidetime = 0.0f;
    goldbricktime = 0.0f;
    Arcade_ResetPanel();
}

extern ADDGAMEMSG AddGameMsg_Default;
extern u8 CoinTab[4];
extern f32 COINMSGTIME;
GAMEMESSAGE_s *AddGameMsg(ADDGAMEMSG *);
void EndScoreMessage(GAMEMESSAGE_s *);
void GameMsg_DrawAdjustNewPos_CoinToTotal(GAMEMESSAGE_s *);
extern "C" void PlaySfx(char *, NUVEC *);
i32 CoinsGoToMainTotal();

void AddCoinsToPanel(i32 coins, nuvec_s *position, i32 player, float, GameObject_s *, i32 random_types) {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if (coins == 0)
        return;
    if (ChallengeMode || Mission_Active(NULL) != NULL)
        coins = 0;
    u8 counts[10] = {};
    i32 remainder = coins % 10;
    if (remainder > 0)
        coins -= remainder;
    if (coins > 512000)
        coins = 512000;
    if (coins > 0)
        PlaySfx("CoinsLand", position);
    if (random_types) {
        while (coins > 0) {
            i32 type = GetRandomCoinType();
            i32 remaining = coins - GizmoPickupType[type].score;
            if (remaining >= 0) {
                ++counts[type];
                coins = remaining;
            }
        }
    } else if (coins > 0) {
        for (i32 i = 3; coins > 0; --i) {
            i32 type = CoinTab[i];
            while (coins - GizmoPickupType[type].score >= 0) {
                coins -= GizmoPickupType[type].score;
                ++counts[type];
            }
        }
    }
    if (static_cast<u32>(player) > 1)
        player = -1;
    NUVEC target;
    target.x = player == 1 ? PANEL_COINX : -PANEL_COINX;
    bool main_total = CoinsGoToMainTotal() != 0;
    f32 target_scale;
    if (main_total) {
        target.y = STATSPOSY;
        target_scale = COINTOTAL_COINSIZE;
    } else {
        target.y = STATSPOSY + PANEL_COINY;
        target_scale = PANEL_COINSCALE_END;
    }
    target.z = 1.0f;
    DrawBuildUpTime = COINMSGTIME + 1.0f;
    for (i32 i = 0; i < 4; ++i) {
        GIZMO_PICKUP_TYPE *type = &GizmoPickupType[CoinTab[i]];
        i32 base_model = static_cast<i16>(type->first_model_id);
        for (i32 j = 0; j < counts[CoinTab[i]]; ++j) {
            i32 model = base_model;
            if (type->random_model_count != 0)
                model += qrand() / (65535 / type->random_model_count + 1);
            if (world->lev_objs[model].active == 0 || player == -1)
                continue;
            ADDGAMEMSG_ALIGNED16 message = AddGameMsg_Default;
            message.position = position;
            message.target_position = &target;
            message.target_scale = target_scale;
            message.icon = model;
            message.extra_position = reinterpret_cast<NUVEC *>(&world->lev_objs[message.icon]);
            message.score = type->score;
            message.end_fn = EndScoreMessage;
            message.player_index = player;
            message.field_0x4d = 1;
            message.field_0x20 = AddCoinDelay[player];
            message.flags = 0x12d;
            message.scale = 1.0f;
            message.duration = COINMSGTIME;
            if (main_total) {
                message.update_fn = GameMsg_DrawAdjustNewPos_CoinToTotal;
                message.field_0x4e = 1;
            }
            AddGameMsg(&message);
            AddCoinDelay[player] += 0.1f;
        }
    }
}

i32 CoinsGoToMainTotal() {
    WORLDINFO_s *world = WorldInfo_CurrentlyActive();
    if ((HUB_ADATA != NULL && HUB_ADATA == world->area) || SuperStory != 0 ||
        (world->area != NULL && (world->area->flags & (AREAFLAG_SUPER_BONUS_AREA & ~AREAFLAG_BONUS_AREA)) != 0) ||
        (world->current_level != NULL && (world->current_level->flags & LEVEL_STATUS) != 0)) {
        return 1;
    }
    return 0;
}

void InitPanel(i32) {
    const f32 panel_fov = pNuCam->fov / 0.75f;
    const f32 aspect_ratio = NuIOS_GetAspectRatio();
    const f32 divisor = (1.0f - panel_fov) * 0.22f + 2.545f;
    PANEL3DMULX = aspect_ratio * panel_fov / divisor;
    PANEL3DMULY = panel_fov / divisor;
}

// DrawPanel reads the private slide timer maintained by the panel lifecycle.
f32 Panel_GetRedBrickSlideTime() { return redbrickslidetime; }
