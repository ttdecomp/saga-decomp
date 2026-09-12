#include "decomp.h"
#include "globals.h"
#include "legoapi/characters/core/players.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/world.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/nu3d/nuspecial.h"

#include <string.h>

extern "C" {
    extern i16 id_GRABMACHINE;
    extern i16 id_GRABMAGNET;
    extern i16 id_ROBOTBASE;
}

GRABBER_s *Grab_grabber;

static __used__ void Grab_invert_x(nufpar_s *) {
    Grab_grabber->invert_x = 1;
}
static __used__ void Grab_move_xy(nufpar_s *parser) {
    if (NuFParGetInt(parser) != 0) {
        Grab_grabber->move_xy = 1;
    }
}
static __used__ void Grab_radius(nufpar_s *parser) {
    Grab_grabber->radius = NuFParGetFloat(parser);
}
static __used__ void Grab_rotate(nufpar_s *parser) {
    GRABBER_s *grabber = Grab_grabber;
    i32 rotate = NuFParGetInt(parser) & 1;
    grabber->flags_559 = (grabber->flags_559 & ~0x08) | (rotate << 3);
}
static __used__ void Grab_scale(nufpar_s *parser) {
    Grab_grabber->scale = NuFParGetFloat(parser);
}
static __used__ void Grab_shadow(nufpar_s *parser) {
    Grab_grabber->flags_559 |= 0x10;
    if (NuFParGetWord(parser) != 0 && NuStrICmp(parser->word_buf, const_cast<char *>("off")) == 0) {
        Grab_grabber->flags_559 &= ~0x10;
    }
}
static __used__ void Grab_speed(nufpar_s *parser) {
    Grab_grabber->speed = NuFParGetFloat(parser);
}
static __used__ void Grab_type(nufpar_s *parser) {
    if (NuFParGetWord(parser) == 0) {
        return;
    }
    if (NuStrICmp(parser->word_buf, const_cast<char *>("GRABMAGNET")) == 0) {
        Grab_grabber->character_id = id_GRABMAGNET;
    } else if (NuStrICmp(parser->word_buf, const_cast<char *>("ROBOTBASE")) == 0) {
        Grab_grabber->character_id = id_ROBOTBASE;
    } else {
        Grab_grabber->character_id = id_GRABMACHINE;
    }
}

static NUFPCOMJMP Grabber_ConfigKeywords[] = {
    {"radius", Grab_radius},     {"speed", Grab_speed},     {"rotate", Grab_rotate},
    {"scale", Grab_scale},       {"move_xy", Grab_move_xy}, {"type", Grab_type},
    {"invert_x", Grab_invert_x}, {"shadow", Grab_shadow},   {NULL, NULL}};

void Grabber_Configure(WORLDINFO_s *world, char *config) {
    if (VehicleArea != 0 || (world->current_level->flags & 0xe0) != 0 || world->grabber != NULL) {
        return;
    }

    nuhspecial_s special;
    if (NuSpecialFind(world->current_gscn, &special, "grabber", 1) == 0) {
        return;
    }

    world->giz_buffer.addr = ALIGN(world->giz_buffer.addr, 16);
    GRABBER_s *grabber = reinterpret_cast<GRABBER_s *>(world->giz_buffer.addr);
    world->grabber = grabber;
    Grab_grabber = grabber;
    world->giz_buffer.addr += sizeof(*grabber);
    grabber->special = special;

    NUFPAR *parser = NuFParCreateMem("grabbers", config, 0xffff);
    if (parser == NULL) {
        return;
    }

    NuFParPushCom(parser, Grabber_ConfigKeywords);
    u8 active = 0;
    while (NuFParGetLine(parser) != 0) {
        if (NuFParGetWord(parser) == 0) {
            continue;
        }

        if (NuStrICmp(parser->word_buf, "grabber_start") == 0) {
            grabber->radius = 0.5f;
            grabber->move_xy = 0;
            grabber->flags_559 = (grabber->flags_559 & ~0x08) | 0x10;
            grabber->invert_x = 0;
            grabber->speed = 1.0f;
            grabber->scale = 1.0f;
            grabber->character_id = id_GRABMACHINE;
            active = 1;
        } else if (NuStrICmp(parser->word_buf, "grabber_end") == 0) {
            break;
        } else if (active != 0) {
            NuFParInterpretWord(parser);
        }
    }
    NuFParDestroy(parser);

    if (grabber->character_id != -1) {
        grabber->character_model = APICharacterLoaded(grabber->character_id);
    } else {
        grabber->character_model = NULL;
    }

    if (grabber->character_model == NULL) {
        NuSpecialSetVisibility(&grabber->special, 1);
        grabber->character_id = -1;
    } else {
        NuSpecialSetVisibility(&grabber->special, 0);
    }

    grabber->matrix = *NuSpecialGetDrawMtx(&grabber->special);
    if (world->current_level == DEATHSTARRESCUEA_LDATA) {
        grabber->matrix.m31 -= 0.3f;
    }

    memmove(&grabber->initial_position, &grabber->position, sizeof(grabber->position));
    NuSpecialFind(world->current_gscn, &grabber->shadow_special, "grabber_shadow", 1);
    grabber->platform_id = -1;
}
