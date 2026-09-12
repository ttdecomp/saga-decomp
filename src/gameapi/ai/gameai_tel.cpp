#include "decomp.h"
#include "gameapi/edtools/edstubs.h"
#include "legoapi/gizmo/base/gizmo.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nu3d/nuspline.h"

static TELEPORT_s *Tel_teleport;
static WORLDINFO_s *Tel_worldinfo;

static __used__ void Tel_1_way(nufpar_s *) {
    Tel_teleport->flags |= 1;
}
static __used__ void Tel_crawl(nufpar_s *) {
    Tel_teleport->flags |= 4;
}
static __used__ void Tel_duration(nufpar_s *parser) {
    TELEPORT_s *teleport = Tel_teleport;
    teleport->duration = NuFParGetFloat(parser);
}
static __used__ void Tel_flap(nufpar_s *parser) {
    if (NuFParGetWord(parser) == 0) {
        return;
    }

    if (NuStrIStr(parser->line_buf, "flap1") != NULL) {
        if (NuSpecialFind(Tel_worldinfo->current_gscn, &Tel_teleport->flap1_special, parser->word_buf, 1) != 0) {
            Tel_teleport->flap1_matrix = *NuSpecialGetDrawMtx(&Tel_teleport->flap1_special);
        }
    } else {
        if (NuSpecialFind(Tel_worldinfo->current_gscn, &Tel_teleport->flap2_special, parser->word_buf, 1) != 0) {
            Tel_teleport->flap2_matrix = *NuSpecialGetDrawMtx(&Tel_teleport->flap2_special);
        }
    }
}
static __used__ void Tel_flip_flap(nufpar_s *) {
    Tel_teleport->flags |= 8;
}
static __used__ void Tel_Hacky_Cam(nufpar_s *) {
    Tel_teleport->flags |= 0x10;
}
static __used__ void Tel_name(nufpar_s *parser) {
    if (NuFParGetWord(parser) != 0) {
        GizmoGetUniqueName(WORLD->gizmo_sys, "TLT_", parser->word_buf, Tel_teleport->name, sizeof(Tel_teleport->name));
    }
}
static __used__ void Tel_obj(nufpar_s *parser) {
    if (NuFParGetWord(parser) != 0) {
        NuSpecialFind(Tel_worldinfo->current_gscn, &Tel_teleport->blocking_special, parser->word_buf, 1);
    }
}
static __used__ void Tel_range(nufpar_s *parser) {
    f32 range = NuFParGetFloat(parser);
    Tel_teleport->flags |= 2;
    Tel_teleport->range_squared = range * range;
}
static __used__ void Tel_spline(nufpar_s *parser) {
    if (NuFParGetWord(parser) == 0) {
        return;
    }

    Tel_teleport->path = edSpline_SplineFind(Tel_worldinfo->current_gscn, parser->word_buf);
    if (Tel_teleport->path == NULL || Tel_teleport->path->length != 4) {
        Tel_teleport->path = NULL;
        return;
    }

    for (i32 i = 0; i < Tel_worldinfo->teleport_count; ++i) {
        if (Tel_worldinfo->teleports[i].path == Tel_teleport->path) {
            Tel_teleport->path = NULL;
            return;
        }
    }

    NuStrCpy(Tel_teleport->name, "TLT_");
    NuStrCat(Tel_teleport->name, Tel_teleport->path->name);
    GizmoGetUniqueName(Tel_worldinfo->gizmo_sys, "TLT_", Tel_teleport->name, Tel_teleport->name,
                       sizeof(Tel_teleport->name));
}

static NUFPCOMJMP Teleport_ConfigKeywords[] = {
    {"spline", Tel_spline}, {"duration", Tel_duration},   {"range", Tel_range},        {"1_way", Tel_1_way},
    {"crawl", Tel_crawl},   {"flip_flap", Tel_flip_flap}, {"hackycam", Tel_Hacky_Cam}, {"obj", Tel_obj},
    {"flap1", Tel_flap},    {"flap2", Tel_flap},          {"name", Tel_name},          {NULL, NULL}};

void Teleports_Configure(WORLDINFO_s *world, char *config) {
    world->teleports = NULL;
    if (world->current_gscn == NULL) {
        return;
    }

    NUFPAR *parser = NuFParCreateMem("teleports", config, 0xffff);
    if (parser == NULL) {
        return;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 4);
    TELEPORT_s *teleport = reinterpret_cast<TELEPORT_s *>(world->giz_buffer.void_ptr);
    world->teleports = teleport;
    NuFParPushCom(parser, Teleport_ConfigKeywords);

    i32 active = 0;
    while (NuFParGetLine(parser) != 0) {
        if (NuFParGetWord(parser) == 0) {
            continue;
        }

        if (active) {
            if (NuStrICmp(parser->word_buf, "teleport_end") != 0) {
                NuFParInterpretWord(parser);
                active = 1;
                continue;
            }

            if (NuStrLen(Tel_teleport->name) == 0) {
                NuStrCpy(Tel_teleport->name, "TLT_");
                NuStrCat(Tel_teleport->name, "TeleportNoSpline!");
                GizmoGetUniqueName(WORLD->gizmo_sys, "TLT_", Tel_teleport->name, Tel_teleport->name,
                                   sizeof(Tel_teleport->name));
            }

            active = 0;
            if (teleport->path != NULL) {
                ++world->teleport_count;
                ++teleport;
            }
            continue;
        }

        if (NuStrICmp(parser->word_buf, "teleport_start") != 0) {
            continue;
        }

        Tel_worldinfo = world;
        Tel_teleport = teleport;
        NuStrCpy(teleport->name, "");
        teleport->enabled = 1;
        teleport->path = NULL;
        teleport->duration = 5.0f;
        teleport->range_squared = 0.0f;
        teleport->flags = 0;
        teleport->active = 0;
        teleport->blocking_special = {};
        teleport->flap1_special = {};
        teleport->flap2_special = {};
        active = 1;
    }

    NuFParDestroy(parser);
    if (world->teleport_count > 0) {
        world->giz_buffer.addr = ALIGN(reinterpret_cast<usize>(teleport), 16);
    } else {
        world->teleports = NULL;
    }
}
