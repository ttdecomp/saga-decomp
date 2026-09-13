#include "decomp.h"
#include "globals.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/nucore/nugcutscene.h"
#include "legoapi/world/world_shared.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "legoapi/characters/core/character.h"
#include "legoapi/menus/core/text.h"
#include "legoapi/world/area.h"
#include "legoapi/world/level.h"
#include "nu2api/nu3d/nucamera.h"
#include "nu2api/nu3d/nuspecial.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nufile/nufpar.h"
#include "nu2api/numath/nufloat.h"
#include "nu2api/numath/nuquat.h"
#include "nu2api/numusic/numusic.h"
#include "nu2api/numusic/sfx.h"
struct CUTSCENEPLAYERCLIP;
struct instNUGCUTCHAR_s;
struct NUGCUTCHAR_s;
struct NUGCUTRIGID_s;
struct instNUGCUTRIGID_s;
extern "C" void instNuGCutSceneEnd(instNUGCUTSCENE_s *instance);
i16 GetMusicIndex(char *name, nusound_filename_info_s *table, i32 default_index);

static CUTINFO *CS_CutInfo;
static VARIPTR *CS_buffptr;
static VARIPTR *CS_buffend;
static i32 CS_texanimcount;
static i32 CS_fadefogcount;

static void CS_play_sfx(NUFPAR *fp) {
    if (NuFParGetWord(fp) == 0) {
        return;
    }

    i32 sfx_id = GetSfxId(fp->word_buf);
    if (sfx_id == -1) {
        return;
    }

    i32 slot;
    for (slot = 0; slot < 6 && CS_CutInfo->sfx[slot].id != -1; ++slot) {
    }
    if (slot == 6) {
        return;
    }

    CUTSCENESFX *sfx = &CS_CutInfo->sfx[slot];
    sfx->id = static_cast<i16>(sfx_id);
    sfx->flags &= ~1U;
    while (NuFParGetWord(fp) != 0) {
        if (NuStrICmp(fp->word_buf, "frame") == 0) {
            sfx->frame = NuFParGetFloat(fp);
            if (sfx->frame < 1.0f) {
                sfx->frame = 1.0f;
            }
        } else if (NuStrICmp(fp->word_buf, "pos") == 0) {
            f32 value = NuFParGetFloat(fp);
            if (value == 0.0f) {
                continue;
            }
            sfx->position.x = NuAToF(fp->word_buf);
            value = NuFParGetFloat(fp);
            if (value == 0.0f) {
                continue;
            }
            sfx->position.y = NuAToF(fp->word_buf);
            value = NuFParGetFloat(fp);
            if (value == 0.0f) {
                continue;
            }
            sfx->position.z = NuAToF(fp->word_buf);
            sfx->flags |= 1;
        }
    }
}

static void copyAnims(NUGCUTSCENE_s *, NUGCUTSCENE_s *);
void NewCopyAnims(instNUGCUTSCENE_s *);
i32 instNuGCutSceneSwapBuffers(instNUGCUTSCENE_s *, i32);

static void CS_sfx(NUFPAR *fp) {
    if (NuFParGetWord(fp) != 0) {
        CS_CutInfo->legacy_music_index = GetMusicIndex(fp->word_buf, MusicInfo, -1);
        CS_CutInfo->music_handle = music_man.GetTrackHandle(TRACK_CLASS_CUTSCENE, fp->word_buf);
    }
}

i32 CUTCOUNT = 0;
CUTINFO *CutList = NULL;
i32 ACTIVECUTCOUNT = 0;
i32 CS_area = 0;
CUTSYS *CS_cutsys = NULL;
WORLDINFO *CS_worldinfo = NULL;
f32 CutSceneScale = 1.0f;
extern "C" {
    u8 CUTSUBTITLEDEFAULT_R = 0xff;
    u8 CUTSUBTITLEDEFAULT_G = 0xff;
    u8 CUTSUBTITLEDEFAULT_B = 0xff;
    u8 CUTSUBTITLEDEFAULT_A = 0x80;
    u8 CUTSUBTITLEDEFAULT_ALIGN = 0;
    f32 CUTSUBTITLEDEFAULT_X = 0.0f;
    f32 CUTSUBTITLEDEFAULT_Y = -0.5f;
    f32 CUTSUBTITLEDEFAULT_SCALEX = 1.0f;
    f32 CUTSUBTITLEDEFAULT_SCALEY = 1.0f;
    f32 CUTSUBTITLEDEFAULT_FITWIDTH = 1.7f;
}
i32 CUTCAM = 0;
i32 CUTCAMONLY = 0;
NUMTX cutscenecammtx = {};
i32 cutscenecamchange = 0;
f32 cutscenecam_fstop = 0.0f;
u8 cutscenecam_usefocusloc = 0;
NUVEC cutscenecam_focusloc = {};
f32 cutscenecam_focusDistance = 0.0f;
f32 cutscenecam_focalLength = 0.0f;
i32 CameraDOFHack = 0;
u8 set_cutscenecammtx = 0;

static void CS_no_fog(NUFPAR *) {
    CS_CutInfo->flags |= 4;
}

static void CS_lowend_lowbits(NUFPAR *) {
    CS_CutInfo->flags |= 0x10000;
}

static void CS_is_outro(NUFPAR *) {
    CS_CutInfo->flags |= 0x20000;
}

static void CS_unskippable_in_story(NUFPAR *) {
    CS_CutInfo->flags |= 0x40000;
}

static void CS_skip_use_goto(NUFPAR *) {
    CS_CutInfo->flags |= 0x80000;
}

static void CS_in_game(NUFPAR *) {
    CS_CutInfo->flags = (CS_CutInfo->flags & ~3U) | 0x800;
}

static void CS_snap_out(NUFPAR *) {
    CS_CutInfo->flags |= 0x10;
}

static void CS_cam_only(NUFPAR *) {
    CS_CutInfo->flags |= 0x20;
}

static void CS_wipe_out(NUFPAR *) {
    CS_CutInfo->flags |= 0x100;
}

static void CS_new_mode(NUFPAR *) {
    CS_CutInfo->flags |= 0x400;
}

static void CS_replace_players(NUFPAR *) {
    CS_CutInfo->flags |= 0x40;
}

static void CS_looping(NUFPAR *) {
    CS_CutInfo->flags |= 0x200;
}

static void CS_start_cam(NUFPAR *) {
    CS_CutInfo->flags |= 0x2000;
}

static void CS_super_widescreen(NUFPAR *) {
    CS_CutInfo->flags |= 0x4000;
}

static void CS_level_intro(NUFPAR *) {
    CS_CutInfo->flags |= 0x1000;
}

static void CS_hold_audio(NUFPAR *) {
    CS_CutInfo->linked_audio = 1;
}

static void CS_playonce(NUFPAR *) {
    CS_CutInfo->end_flags |= 1;
}

static void CS_nextcutscene_inplayablelevel(NUFPAR *) {
    CS_CutInfo->end_flags |= 2;
}

static void CS_farclip(NUFPAR *fp) {
    CS_CutInfo->camera_far_clip = static_cast<u16>(NuFParGetInt(fp));
}

static void CS_render_group(NUFPAR *fp) {
    CS_CutInfo->debris_render_group = static_cast<i8>(NuFParGetInt(fp));
}

static void CS_reflect_range(NUFPAR *fp) {
    CS_CutInfo->reflection_range = static_cast<u8>(NuFParGetInt(fp));
}

static void CS_blobshadow_fadefar(NUFPAR *fp) {
    CS_CutInfo->blob_shadow_fade_far = static_cast<u8>(NuFParGetInt(fp));
}

static void CS_blobshadow_fadenear(NUFPAR *fp) {
    CS_CutInfo->blob_shadow_fade_near = static_cast<u8>(NuFParGetInt(fp));
}

static void CS_blobshadow_alpha(NUFPAR *fp) {
    i32 alpha = NuFParGetInt(fp);
    if (alpha < 0) {
        alpha = 0;
    } else if (alpha > 0xfe) {
        alpha = 0xfe;
    }
    CS_CutInfo->blob_shadow_alpha = static_cast<u8>(alpha);
}

static void CS_nearclip(NUFPAR *fp) {
    CS_CutInfo->camera_near_clip = NuFParGetFloat(fp);
}

static void CS_burnout_flare(NUFPAR *fp) {
    CS_CutInfo->burnout_flare = NuFParGetFloat(fp);
    CS_CutInfo->flags |= 0x80;
}

static void CS_burnout_intensity(NUFPAR *fp) {
    CS_CutInfo->burnout_intensity = NuFParGetFloat(fp);
    CS_CutInfo->flags |= 0x80;
}

static void CS_burnout_threshold(NUFPAR *fp) {
    CS_CutInfo->burnout_threshold = NuFParGetFloat(fp);
    CS_CutInfo->flags |= 0x80;
}

static void CS_fpsec(NUFPAR *fp) {
    CS_CutInfo->frames_per_second = NuFParGetFloat(fp);
}

static void CS_lowend_disthack(NUFPAR *fp) {
    if (NuFParGetWord(fp) != 0) {
        f32 distance = NuAToF(fp->word_buf);
        if (distance > 0.0f) {
            CS_CutInfo->low_end_distance = distance;
        }
    }
}

static void CS_draw_gizmo_sys(NUFPAR *fp) {
    if (NuFParGetWord(fp) == 0) {
        return;
    }
    if (NuStrICmp(fp->word_buf, "on") == 0) {
        CS_CutInfo->flags |= 0x8000;
    } else if (NuStrICmp(fp->word_buf, "off") == 0) {
        CS_CutInfo->flags &= ~0x8000U;
    }
}

static void CS_deb_page(NUFPAR *fp) {
    if (NuFParGetWord(fp) == 0) {
        return;
    }
    if (NuStrICmp(fp->word_buf, "level") == 0) {
        CS_CutInfo->flags |= 8;
    } else {
        CS_CutInfo->flags &= ~8U;
    }
}

static void CS_draw_world(NUFPAR *fp) {
    if (NuFParGetWord(fp) == 0) {
        return;
    }
    if (NuStrICmp(fp->word_buf, "on") == 0) {
        CS_CutInfo->flags |= 2;
    } else if (NuStrICmp(fp->word_buf, "off") == 0) {
        CS_CutInfo->flags &= ~2U;
    }
}

static void CS_next_cut_scene(NUFPAR *fp) {
    if (NuFParGetWord(fp) != 0 && NuStrLen(fp->word_buf) <= 0x3f) {
        NuStrCpy(CS_CutInfo->next_cutscene, fp->word_buf);
        CS_CutInfo->linked_audio = 1;
    }
}

static void CS_go_through_door(NUFPAR *fp) {
    if (NuFParGetWord(fp) != 0 && NuStrLen(fp->word_buf) <= 0xf) {
        NuStrCpy(CS_CutInfo->door_name, fp->word_buf);
    }
}

static void CS_tex_anim(NUFPAR *fp) {
    if (CS_texanimcount > 3) {
        return;
    }
    CUTSCENETEXANIM *animation = &CS_CutInfo->texture_animations[CS_texanimcount];
    animation->index = NuFParGetInt(fp);
    if (animation->index == -1) {
        return;
    }
    animation->frame = static_cast<f32>(NuFParGetInt(fp));
    if (animation->frame >= 0.0f) {
        ++CS_texanimcount;
    }
}

static void CS_fadefog(NUFPAR *fp) {
    if (CS_fadefogcount > 1) {
        return;
    }
    CUTSCENEFADEFOG *fade = &CS_CutInfo->fade_fog[CS_fadefogcount];
    fade->frame = NuFParGetFloat(fp);
    if (fade->frame < 0.0f) {
        return;
    }
    fade->near_distance = NuFParGetFloat(fp);
    if (fade->near_distance < 0.0f) {
        return;
    }
    fade->far_distance = NuFParGetFloat(fp);
    if (fade->far_distance < 0.0f) {
        return;
    }
    fade->value = NuFParGetFloat(fp);
    if (fade->value >= 0.0f) {
        ++CS_fadefogcount;
    }
}

static void CS_cutsceneplayerobj(NUFPAR *fp) {
    if (CS_CutInfo->state_count > 0x1f || NuFParGetWord(fp) == 0) {
        return;
    }

    CUTSCENEPLAYEROBJ *object = &CS_CutInfo->state_entries[CS_CutInfo->state_count];
    if (NuSpecialFind(CS_worldinfo->current_gscn, &object->special, fp->word_buf, 1) == 0) {
        return;
    }
    object->flags &= ~(CLIP_OBJECT_SHOW | CLIP_OBJECT_HIDE);
    while (NuFParGetWord(fp) != 0) {
        if (NuStrICmp(fp->word_buf, "on") == 0) {
            object->flags = (object->flags | CLIP_OBJECT_SHOW) & ~CLIP_OBJECT_HIDE;
        } else if (NuStrICmp(fp->word_buf, "off") == 0) {
            object->flags = (object->flags & ~CLIP_OBJECT_SHOW) | CLIP_OBJECT_HIDE;
        } else if (NuStrICmp(fp->word_buf, "end_anim") == 0 || NuStrICmp(fp->word_buf, "endanim") == 0 ||
                   NuStrICmp(fp->word_buf, "anim_end") == 0 || NuStrICmp(fp->word_buf, "animend") == 0) {
            object->flags = (object->flags & ~(CLIP_OBJECT_SHOW | CLIP_OBJECT_HIDE)) | CLIP_OBJECT_ANIM_END;
        }
    }
    ++CS_CutInfo->state_count;
}

static void CS_goto_level(NUFPAR *fp) {
    if (NuFParGetWord(fp) == 0) {
        return;
    }
    i32 level_index;
    LEVELDATA *level = Level_FindByName(fp->word_buf, &level_index);
    if (level_index != -1 && level == HUB_LDATA && CS_area != -1) {
        i32 status_index;
        Area_FindStatusLevel(&ADataList[CS_area], &status_index);
        if (status_index != -1) {
            level_index = status_index;
        }
    }
    CS_CutInfo->skip_level = static_cast<i16>(level_index);
}

static void CS_subtitle(NUFPAR *fp) {
    if (CS_CutInfo->subtitle_data == NULL) {
        CS_CutInfo->subtitle_data = reinterpret_cast<CUTSCENESUBTITLE *>(CS_buffptr->addr);
    }

    CUTSCENESUBTITLE *subtitle = &CS_CutInfo->subtitle_data[CS_CutInfo->subtitle_count];
    subtitle->text_id = -1;
    subtitle->start_frame = 0.0f;
    subtitle->end_frame = 0.0f;
    subtitle->red = CUTSUBTITLEDEFAULT_R;
    subtitle->green = CUTSUBTITLEDEFAULT_G;
    subtitle->blue = CUTSUBTITLEDEFAULT_B;
    subtitle->alpha = CUTSUBTITLEDEFAULT_A;
    subtitle->alignment = CUTSUBTITLEDEFAULT_ALIGN;
    subtitle->x = CUTSUBTITLEDEFAULT_X;
    subtitle->y = CUTSUBTITLEDEFAULT_Y;
    subtitle->x_scale = CUTSUBTITLEDEFAULT_SCALEX;
    subtitle->y_scale = CUTSUBTITLEDEFAULT_SCALEY;
    subtitle->max_width = CUTSUBTITLEDEFAULT_FITWIDTH;
    subtitle->fade_time = 0.0f;

    while (NuFParGetWord(fp) != 0) {
        if (NuStrICmp(fp->word_buf, "text_id") == 0) {
            subtitle->text_id = static_cast<i16>(NuFParGetInt(fp));
        } else if (NuStrICmp(fp->word_buf, "colour") == 0) {
            subtitle->red = static_cast<u8>(NuFParGetFloat(fp) * 255.0f);
            subtitle->green = static_cast<u8>(NuFParGetFloat(fp) * 255.0f);
            subtitle->blue = static_cast<u8>(NuFParGetFloat(fp) * 255.0f);
            subtitle->alpha = static_cast<u8>(NuFParGetFloat(fp) * 128.0f);
        } else if (NuStrICmp(fp->word_buf, "r") == 0) {
            subtitle->red = static_cast<u8>(NuFParGetFloat(fp) * 255.0f);
        } else if (NuStrICmp(fp->word_buf, "g") == 0) {
            subtitle->green = static_cast<u8>(NuFParGetFloat(fp) * 255.0f);
        } else if (NuStrICmp(fp->word_buf, "b") == 0) {
            subtitle->blue = static_cast<u8>(NuFParGetFloat(fp) * 255.0f);
        } else if (NuStrICmp(fp->word_buf, "a") == 0) {
            subtitle->alpha = static_cast<u8>(NuFParGetFloat(fp) * 128.0f);
        } else if (NuStrICmp(fp->word_buf, "align") == 0) {
            if (NuStrICmp(fp->word_buf, "centre") == 0) {
                subtitle->alignment = 0;
            } else if (NuStrICmp(fp->word_buf, "top") == 0) {
                subtitle->alignment = 1;
            } else if (NuStrICmp(fp->word_buf, "top_right") == 0) {
                subtitle->alignment = 9;
            } else if (NuStrICmp(fp->word_buf, "right") == 0) {
                subtitle->alignment = 8;
            } else if (NuStrICmp(fp->word_buf, "bottom_right") == 0) {
                subtitle->alignment = 12;
            } else if (NuStrICmp(fp->word_buf, "bottom") == 0) {
                subtitle->alignment = 4;
            } else if (NuStrICmp(fp->word_buf, "bottom_left") == 0) {
                subtitle->alignment = 6;
            } else if (NuStrICmp(fp->word_buf, "left") == 0) {
                subtitle->alignment = 2;
            } else if (NuStrICmp(fp->word_buf, "top_left") == 0) {
                subtitle->alignment = 3;
            }
        } else if (NuStrICmp(fp->word_buf, "start_time") == 0) {
            subtitle->start_frame = NuFabs(NuFParGetFloat(fp));
        } else if (NuStrICmp(fp->word_buf, "end_time") == 0) {
            subtitle->end_frame = NuFabs(NuFParGetFloat(fp));
        } else if (NuStrICmp(fp->word_buf, "pos") == 0) {
            subtitle->x = NuFParGetFloat(fp);
            subtitle->y = NuFParGetFloat(fp);
        } else if (NuStrICmp(fp->word_buf, "x") == 0) {
            subtitle->x = NuFParGetFloat(fp);
        } else if (NuStrICmp(fp->word_buf, "y") == 0) {
            subtitle->y = NuFParGetFloat(fp);
        }

        if (NuStrICmp(fp->word_buf, "scale") == 0) {
            subtitle->x_scale = NuFParGetFloat(fp);
            subtitle->y_scale = subtitle->x_scale;
        } else if (NuStrICmp(fp->word_buf, "x_scale") == 0) {
            subtitle->x_scale = NuFParGetFloat(fp);
        } else if (NuStrICmp(fp->word_buf, "y_scale") == 0) {
            subtitle->y_scale = NuFParGetFloat(fp);
        } else if (NuStrICmp(fp->word_buf, "fit_width") == 0) {
            subtitle->max_width = NuFabs(NuFParGetFloat(fp));
        } else if (NuStrICmp(fp->word_buf, "fade_time") == 0) {
            subtitle->fade_time = NuFabs(NuFParGetFloat(fp));
        }
    }

    i16 text_id = subtitle->text_id;
    if (text_id >= 0 && text_id < Text_GetMaxOverallStrings() && TTab[text_id] != NULL &&
        subtitle->start_frame < subtitle->end_frame) {
        if (subtitle->fade_time > 0.0f && subtitle->end_frame - subtitle->start_frame < subtitle->fade_time * 2.0f) {
            subtitle->fade_time = 0.0f;
        }
        CS_buffptr->addr += sizeof(CUTSCENESUBTITLE);
        ++CS_CutInfo->subtitle_count;
    }
}

static NUFPCOMJMP CutScene_ConfigKeywords[] = {
    {const_cast<char *>("fpsec"), CS_fpsec},
    {const_cast<char *>("no_fog"), CS_no_fog},
    {const_cast<char *>("draw_world"), CS_draw_world},
    {const_cast<char *>("lowend_lowbits"), CS_lowend_lowbits},
    {const_cast<char *>("lowend_disthack"), CS_lowend_disthack},
    {const_cast<char *>("is_outro"), CS_is_outro},
    {const_cast<char *>("unskippable_in_story"), CS_unskippable_in_story},
    {const_cast<char *>("skip_use_goto"), CS_skip_use_goto},
    {const_cast<char *>("in_game"), CS_in_game},
    {const_cast<char *>("blobshadow_alpha"), CS_blobshadow_alpha},
    {const_cast<char *>("blobshadow_fadenear"), CS_blobshadow_fadenear},
    {const_cast<char *>("blobshadow_fadefar"), CS_blobshadow_fadefar},
    {const_cast<char *>("reflect_range"), CS_reflect_range},
    {const_cast<char *>("render_group"), CS_render_group},
    {const_cast<char *>("deb_page"), CS_deb_page},
    {const_cast<char *>("snap_out"), CS_snap_out},
    {const_cast<char *>("wipe_out"), CS_wipe_out},
    {const_cast<char *>("new_mode"), CS_new_mode},
    {const_cast<char *>("cam_only"), CS_cam_only},
    {const_cast<char *>("replace_players"), CS_replace_players},
    {const_cast<char *>("burnout_threshold"), CS_burnout_threshold},
    {const_cast<char *>("burnout_intensity"), CS_burnout_intensity},
    {const_cast<char *>("burnout_flare"), CS_burnout_flare},
    {const_cast<char *>("looping"), CS_looping},
    {const_cast<char *>("start_cam"), CS_start_cam},
    {const_cast<char *>("super_widescreen"), CS_super_widescreen},
    {const_cast<char *>("go_through_door"), CS_go_through_door},
    {const_cast<char *>("sfx"), CS_sfx},
    {const_cast<char *>("play_sfx"), CS_play_sfx},
    {const_cast<char *>("goto_level"), CS_goto_level},
    {const_cast<char *>("next_cut_scene"), CS_next_cut_scene},
    {const_cast<char *>("holdaudio"), CS_hold_audio},
    {const_cast<char *>("hold_audio"), CS_hold_audio},
    {const_cast<char *>("level_intro"), CS_level_intro},
    {const_cast<char *>("tex_anim"), CS_tex_anim},
    {const_cast<char *>("fadefog"), CS_fadefog},
    {const_cast<char *>("subtitle"), CS_subtitle},
    {const_cast<char *>("play_once"), CS_playonce},
    {const_cast<char *>("nextcutscene_inplayablelevel"), CS_nextcutscene_inplayablelevel},
    {const_cast<char *>("cutsceneplayerobj"), CS_cutsceneplayerobj},
    {const_cast<char *>("cutsceneplayer_obj"), CS_cutsceneplayerobj},
    {const_cast<char *>("nearclip_360"), CS_nearclip},
    {const_cast<char *>("farclip_360"), CS_farclip},
    {const_cast<char *>("nearclip_pc"), CS_nearclip},
    {const_cast<char *>("farclip_pc"), CS_farclip},
    {const_cast<char *>("nearclip"), CS_nearclip},
    {const_cast<char *>("farclip"), CS_farclip},
    {const_cast<char *>("drawgizmosys"), CS_draw_gizmo_sys},
    {NULL, NULL},
};

static __used__ void CutScene_Configure(CUTINFO *cut, char *name, VARIPTR *buf, VARIPTR *buf_end) {
    CUTSCENEPLAYEROBJ state_entries[32];

    CS_CutInfo = cut;
    CS_buffptr = buf;
    CS_buffend = buf_end;
    cut->state_count = 0;
    cut->flags = 3;
    cut->frames_per_second = 30.0f;
    cut->burnout_threshold = 1.0f;
    cut->burnout_intensity = 0.0f;
    cut->burnout_flare = 0.0f;
    cut->camera_near_clip = 0.0f;
    cut->camera_far_clip = 0;
    cut->legacy_music_index = -1;
    cut->skip_level = -1;
    cut->linked_audio = 0;
    cut->debris_render_group = 2;
    cut->door_name[0] = '\0';
    cut->next_cutscene[0] = '\0';
    cut->end_flags = 0;
    cut->music_handle = -1;

    for (CUTSCENESFX &sfx : cut->sfx) {
        sfx.id = -1;
    }
    cut->blob_shadow_alpha = 0xff;
    cut->blob_shadow_fade_near = 0xff;
    cut->blob_shadow_fade_far = 0xff;
    cut->reflection_range = 0xff;
    memset(cut->texture_animations, 0, sizeof(cut->texture_animations));
    for (CUTSCENETEXANIM &animation : cut->texture_animations) {
        animation.index = -1;
    }
    memset(cut->fade_fog, 0, sizeof(cut->fade_fog));
    cut->subtitle_data = NULL;
    cut->subtitle_count = 0;
    cut->end_flags &= ~3U;
    cut->pad_18b = 0;
    cut->low_end_distance = 0.0f;
    cut->field_194 = 0.0f;
    cut->state_entries = state_entries;

    NUFPAR *fp = NuFParCreate(name);
    if (fp == NULL) {
        return;
    }
    CS_texanimcount = 0;
    CS_fadefogcount = 0;
    NuFParPushCom(fp, CutScene_ConfigKeywords);
    while (NuFParGetLine(fp) != 0) {
        if (NuFParGetWord(fp) != 0) {
            NuFParInterpretWord(fp);
        }
    }
    NuFParDestroy(fp);

    if (cut->blob_shadow_fade_near > cut->blob_shadow_fade_far) {
        cut->blob_shadow_fade_near = cut->blob_shadow_fade_far;
    }
    if (cut->subtitle_data != NULL && cut->subtitle_count == 0) {
        cut->subtitle_data = NULL;
    }
    if (cut->state_count == 0) {
        cut->state_entries = NULL;
    } else {
        buf->void_ptr = reinterpret_cast<void *>(ALIGN(buf->addr, 4));
        cut->state_entries = reinterpret_cast<CUTSCENEPLAYEROBJ *>(buf->void_ptr);
        memmove(cut->state_entries, state_entries, cut->state_count * sizeof(CUTSCENEPLAYEROBJ));
        buf->void_ptr = reinterpret_cast<char *>(buf->void_ptr) + cut->state_count * sizeof(CUTSCENEPLAYEROBJ);
    }
}

void *CutScenes_Load(char *config, NUGSCN *gscn1, NUGSCN *gscn2, i32 param1, VARIPTR *buf, VARIPTR *buf_end, i32 param2,
                     i32 param3, WORLDINFO *world) {
    NUFPAR *fp;
    CUTSYS *sys;
    void *initial;
    CUTINFO *cut;
    char name[128];
    char full_path[128];
    CUTINFO *entries[32];

    if (CutList != NULL && CUTCOUNT > 0) {
        sys = (CUTSYS *)ALIGN(buf->addr, 4);
        sys->cuts = reinterpret_cast<CUTINFO **>(sys + 1);
        sys->count = CUTCOUNT;
        CS_area = param2;
        CS_worldinfo = world;
        CS_cutsys = sys;
        buf->void_ptr = reinterpret_cast<char *>(sys->cuts) + CUTCOUNT * sizeof(CUTINFO *);
        return sys;
    }
    if (config == NULL) {
        return NULL;
    }

    fp = NuFParCreateMem((char *)"cutscenes", config, 0xffff);
    if (fp == NULL) {
        return NULL;
    }

    initial = buf->void_ptr;
    sys = (CUTSYS *)ALIGN((usize)initial, 4);
    sys->cuts = reinterpret_cast<CUTINFO **>(sys + 1);
    sys->count = 0;
    sys->character_bits = reinterpret_cast<u32 *>(sys + 1);
    CS_area = param2;
    CS_worldinfo = world;
    CS_cutsys = sys;
    buf->void_ptr = reinterpret_cast<char *>(sys->character_bits) + ((CHARCOUNT + 0x1f) >> 5) * 4;

    while (NuFParGetLine(fp) != 0) {
        if (NuFParGetWord(fp) == 0 || NuStrICmp(fp->word_buf, "cutscene") != 0 || sys->count > 0x1f ||
            NuFParGetWord(fp) == 0) {
            continue;
        }

        cut = (CUTINFO *)ALIGN(buf->addr, 4);
        entries[sys->count] = cut;
        buf->void_ptr = cut + 1;
        NuStrCpy(name, fp->word_buf);
        for (char *lower = name; *lower != '\0'; ++lower) {
            *lower = (char)NuToLower((u8)*lower);
        }
        i32 len = NuStrLen(name);
        while (len > 0 && name[len - 1] != '.') {
            --len;
        }
        if (len > 0) {
            name[len - 1] = '\0';
        }

        if (name[0] == 'c' && name[1] == 'u' && name[2] == 't' && name[3] == '\\') {
            NuStrCpy(name, name + 4);
        }
        NuStrCpy(full_path, "cut\\");
        NuStrCat(full_path, name);
        NuStrCat(full_path, ".txt");
        CutScene_Configure(cut, full_path, buf, buf_end);

        if ((reinterpret_cast<u8 *>(cut)[0x51] & 8) == 0 && !InStory()) {
            continue;
        }
        buf->void_ptr = (char *)ALIGN(buf->addr, 0x40);
        NuStrCpy(full_path, "cut\\");
        NuStrCat(full_path, name);
        NuStrCat(full_path, ".cu2");
        cut->scene = NuGCutSceneLoad(full_path, buf, buf_end, 0);
        if (cut->scene == NULL) {
            NuStrCpy(full_path, "cut\\");
            NuStrCat(full_path, name);
            NuStrCat(full_path, ".cut");
            cut->scene = NuGCutSceneLoad(full_path, buf, buf_end, 0);
            if (cut->scene == NULL) {
                continue;
            }
        }
        char *base_name = name;
        for (char *cursor = name; *cursor != '\0'; ++cursor) {
            if (*cursor == '\\') {
                base_name = cursor + 1;
            }
        }
        NuStrCpy(cut->name, base_name);
        NuGCutSceneFixUp(reinterpret_cast<NUGCUTSCENE_s *>(cut->scene), gscn1, 0, static_cast<i8>(param1));
        NuGCutSceneFixUpExtra(reinterpret_cast<NUGCUTSCENE_s *>(cut->scene), gscn2);
        cut->instance = instNuGCutSceneCreate(reinterpret_cast<NUGCUTSCENE_s *>(cut->scene), NULL, NULL, name, buf, 0);
        if (cut->instance != NULL) {
            reinterpret_cast<instNUGCUTSCENE_s *>(cut->instance)->rate = cut->frames_per_second * DEFAULTFRAMETIME;
            buf->void_ptr = (char *)ALIGN(buf->addr, 0x10);
        }
        sys->count++;
    }
    NuFParDestroy(fp);
    if (sys->count == 0) {
        buf->void_ptr = initial;
        return NULL;
    }
    buf->void_ptr = reinterpret_cast<void *>(ALIGN(buf->addr, 4));
    sys->cuts = reinterpret_cast<CUTINFO **>(buf->void_ptr);
    memmove(sys->cuts, entries, sys->count * sizeof(CUTINFO *));
    buf->void_ptr = reinterpret_cast<char *>(buf->void_ptr) + sys->count * sizeof(CUTINFO *);
    return sys;
}

// --- Extern "C" block: functions with confirmed C linkage in original libTTapp.so ---
extern "C" void *NuAnimData2FixPtrs(void *, isize, isize, i32);
extern "C" StateAnim *StateAnimFixPtrs(StateAnim *, isize);
extern "C" i32 StateAnimEvaluate(StateAnim *, u8 *, u8 *, f32);
extern "C" void NuAnimCurve2SetApplyToMatrix_3(ani3_animheader_s *, i32, f32, NUMTX *);
extern "C" i32 LookupDebrisEffectPage(char *, i32);
extern "C" i32 LookupDebrisEffectPageOnly(char *, i32);
extern "C" {
    extern i32 NuGCutDebFixUp_SearchAllPages;
    extern NUGCUTLOCATORFNENTRY_s *locatorfns;
    extern i32 (*LookupLocatorVfxFn)(char *);
    extern i32 (*NuCutSceneSFXFixUp)(usize);
}
void NuGCutRigidCalcMtx(NUGCUTRIGID_s *, f32, numtx_s *);

static instNUGCUTSCENE_s *background_cutscene_instances;
static instNUGCUTSCENE_s *active_cutscene_instances;
static i32 cutscene_synchro_counter;
static i32 cutscene_quit_prompt;
static i32 cutscene_has_quit;
static i32 cutscene_lock_player;
static i32 termcutstream_hack;

extern "C" {
    instNUGCUTSCENE_s *cutscene_load_instance;
    i32 NumCommonStreamingBuffers = 2;
}

static void NuGCutSceneFixPtrs_Title(NUGCUTSCENE_s *cutscene, isize anim_delta) {
    isize data_delta = cutscene->string_delta;
    if (cutscene->strings != NULL) {
        cutscene->strings = reinterpret_cast<char *>(reinterpret_cast<isize>(cutscene->strings) + data_delta);
    }
    if (cutscene->camera_system != NULL) {
        cutscene->camera_system =
            reinterpret_cast<NUGCUTCAMERASYS_s *>(reinterpret_cast<isize>(cutscene->camera_system) + data_delta);
        NUGCUTCAMERASYS_s *system = cutscene->camera_system;
        if (system->cameras != NULL) {
            system->cameras = reinterpret_cast<NUGCUTCAMERA_s *>(reinterpret_cast<isize>(system->cameras) + data_delta);
        }
        if (anim_delta != 0) {
            if (cutscene->version > 4) {
                if (system->focus_animation != NULL) {
                    system->focus_animation =
                        static_cast<nuanimdata2_s *>(NuAnimData2FixPtrs(system->focus_animation, anim_delta, 0, 0));
                }
                system->focus_state_animation = StateAnimFixPtrs(system->focus_state_animation, anim_delta);
            }
            if (system->animation != NULL) {
                system->animation =
                    static_cast<nuanimdata2_s *>(NuAnimData2FixPtrs(system->animation, anim_delta, 0, 0));
            }
            system->state_animation = StateAnimFixPtrs(system->state_animation, anim_delta);
        }
    }
    if (cutscene->locator_system != NULL) {
        cutscene->locator_system =
            reinterpret_cast<NUGCUTLOCATORSYS_s *>(reinterpret_cast<isize>(cutscene->locator_system) + data_delta);
        NUGCUTLOCATORSYS_s *system = cutscene->locator_system;
        if (system->locators != NULL) {
            system->locators =
                reinterpret_cast<NUGCUTLOCATOR_s *>(reinterpret_cast<isize>(system->locators) + data_delta);
            if (anim_delta != 0) {
                for (i32 i = 0; i < system->locator_count; ++i) {
                    NUGCUTLOCATOR_s *locator = &system->locators[i];
                    if (locator->animation != NULL) {
                        locator->animation =
                            static_cast<nuanimdata2_s *>(NuAnimData2FixPtrs(locator->animation, anim_delta, 0, 0));
                    }
                }
            }
        }
        if (system->types != NULL) {
            system->types =
                reinterpret_cast<NUGCUTLOCATORTYPE_s *>(reinterpret_cast<isize>(system->types) + data_delta);
            for (i32 i = 0; i < system->type_count; ++i) {
                if (system->types[i].name != NULL) {
                    system->types[i].name = reinterpret_cast<char *>(reinterpret_cast<isize>(system->types[i].name) +
                                                                     reinterpret_cast<isize>(cutscene->strings) - 1);
                }
            }
        }
    }
    if (cutscene->rigid_system != NULL) {
        cutscene->rigid_system =
            reinterpret_cast<NUGCUTRIGIDSYS_s *>(reinterpret_cast<isize>(cutscene->rigid_system) + data_delta);
        NUGCUTRIGIDSYS_s *system = cutscene->rigid_system;
        if (system->rigids != NULL) {
            system->rigids = reinterpret_cast<NUGCUTRIGID_s *>(reinterpret_cast<isize>(system->rigids) + data_delta);
            for (i32 i = 0; i < system->count; ++i) {
                NUGCUTRIGID_s *rigid = &system->rigids[i];
                if (rigid->name != NULL) {
                    rigid->name = reinterpret_cast<char *>(reinterpret_cast<isize>(rigid->name) +
                                                           reinterpret_cast<isize>(cutscene->strings) - 1);
                }
                if (anim_delta != 0) {
                    if (rigid->animation != NULL) {
                        rigid->animation =
                            static_cast<nuanimdata2_s *>(NuAnimData2FixPtrs(rigid->animation, anim_delta, 0, 0));
                    }
                    rigid->state_animation = StateAnimFixPtrs(rigid->state_animation, anim_delta);
                }
            }
        }
    }
    if (cutscene->character_system != NULL) {
        cutscene->character_system =
            reinterpret_cast<NUGCUTCHARSYS_s *>(reinterpret_cast<isize>(cutscene->character_system) + data_delta);
        NUGCUTCHARSYS_s *system = cutscene->character_system;
        if (system->characters != NULL) {
            system->characters =
                reinterpret_cast<NUGCUTCHAR_s *>(reinterpret_cast<isize>(system->characters) + data_delta);
            for (i32 i = 0; i < system->character_count; ++i) {
                NUGCUTCHAR_s *character = &system->characters[i];
                if (character->name != NULL) {
                    character->name = reinterpret_cast<char *>(reinterpret_cast<isize>(character->name) +
                                                               reinterpret_cast<isize>(cutscene->strings) - 1);
                }
                if (anim_delta != 0) {
                    if (character->animation != NULL) {
                        character->animation =
                            static_cast<nuanimdata2_s *>(NuAnimData2FixPtrs(character->animation, anim_delta, 0, 0));
                    }
                    if (character->face_animation != NULL) {
                        character->face_animation = static_cast<nuanimdata2_s *>(
                            NuAnimData2FixPtrs(character->face_animation, anim_delta, 0, 0));
                    }
                    if (character->extra_animation != NULL) {
                        character->extra_animation = static_cast<nuanimdata2_s *>(
                            NuAnimData2FixPtrs(character->extra_animation, anim_delta, 0, 0));
                    }
                }
            }
        }
        if (cutscene->version > 3 && cutscene->character_animations != NULL) {
            cutscene->character_animations = reinterpret_cast<NUGCUTCHARANIM_s *>(
                reinterpret_cast<isize>(cutscene->character_animations) + data_delta);
            if (system->characters != NULL) {
                for (i32 i = 0; i < system->character_count; ++i) {
                    NUGCUTCHARANIM_s *animation = &cutscene->character_animations[i];
                    if (animation->animation != NULL) {
                        animation->animation =
                            static_cast<nuanimdata2_s *>(NuAnimData2FixPtrs(animation->animation, anim_delta, 0, 0));
                    }
                }
            }
        }
    }
    if (cutscene->trigger_system != NULL) {
        cutscene->trigger_system =
            reinterpret_cast<NUGCUTTRIGGERSYS_s *>(reinterpret_cast<isize>(cutscene->trigger_system) + data_delta);
        NUGCUTTRIGGERSYS_s *system = cutscene->trigger_system;
        if (system->events != NULL) {
            system->events =
                reinterpret_cast<NUGCUTTRIGGEREVENT_s *>(reinterpret_cast<isize>(system->events) + data_delta);
            for (i32 i = 0; i < system->event_count; ++i) {
                NUGCUTTRIGGEREVENT_s *event = &system->events[i];
                if (event->field_04 != NULL) {
                    event->field_04 = reinterpret_cast<void *>(reinterpret_cast<isize>(event->field_04) + data_delta);
                }
                event->state_animation = StateAnimFixPtrs(event->state_animation, data_delta);
            }
        }
    }
    if (cutscene->bounds != NULL) {
        cutscene->bounds = reinterpret_cast<void *>(reinterpret_cast<isize>(cutscene->bounds) + data_delta);
    }
}

extern "C" {

    i32 NuGCutDebFixUp_SearchAllPages = 0;
    NUGCUTLOCATORFNENTRY_s *locatorfns = NULL;
    i32 (*LookupLocatorVfxFn)(char *) = NULL;
    i32 (*NuCutSceneSFXFixUp)(usize) = NULL;
    NUGCUTSCENE_s *NuGCutSceneLoad(char *name, VARIPTR *buf, VARIPTR *buf_end, i32 flags) {
        char path[1036];
        usize available = buf_end->addr - buf->addr;
        strcpy(path, name);
        buf->addr = ALIGN(buf->addr, 0x10);
        NUGCUTSCENE_s *cutscene = reinterpret_cast<NUGCUTSCENE_s *>(buf->void_ptr);
        i32 bytes = NuFileLoadBuffer(path, cutscene, static_cast<i32>(available));
        if (bytes == 0) {
            return NULL;
        }

        if (cutscene->version > 9) {
            isize anim_delta = reinterpret_cast<isize>(cutscene) - cutscene->relocation_delta;
            cutscene->string_delta = reinterpret_cast<isize>(cutscene) - cutscene->string_delta;
            cutscene->loaded_size = bytes;
            cutscene->relocation_delta = anim_delta;
            if ((cutscene->flags & 8) != 0) {
                NuGCutSceneFixPtrs_Title(cutscene, 0);
            } else {
                NuGCutSceneFixPtrs_Title(cutscene, anim_delta);
            }
            if ((cutscene->flags & 1) != 0 && flags == 0) {
                usize stream_buffer = ALIGN(buf->addr + cutscene->field_4c, 0x10);
                cutscene->stream_buffer_0 = reinterpret_cast<NUGCUTSCENE_s *>(stream_buffer);
                stream_buffer += cutscene->stream_buffer_size;
                if (NumCommonStreamingBuffers == 0) {
                    stream_buffer = ALIGN(stream_buffer + 0x400, 0x10);
                    cutscene->stream_buffer_1 = reinterpret_cast<NUGCUTSCENE_s *>(stream_buffer);
                    stream_buffer += cutscene->stream_buffer_size + 0x400;
                } else {
                    stream_buffer += 0x400;
                }
                buf->addr = stream_buffer;
                char *filename = reinterpret_cast<char *>(buf->void_ptr);
                buf->addr += NuStrLen(name) + 4;
                NuStrCpy(filename, name);
                cutscene->filename = filename;
            } else {
                buf->addr += bytes;
            }
            NuGCutSceneRemapFocusIdToLocaterNum(cutscene, buf);
            return cutscene;
        }

        isize anim_delta = reinterpret_cast<isize>(cutscene) - cutscene->string_delta;
        cutscene->string_delta = anim_delta;
        if (cutscene->version > 1) {
            anim_delta = reinterpret_cast<isize>(cutscene) - cutscene->relocation_delta;
            cutscene->relocation_delta = anim_delta;
        }
        if ((cutscene->flags & 8) != 0) {
            NuGCutSceneFixPtrs_Title(cutscene, 0);
        } else {
            NuGCutSceneFixPtrs_Title(cutscene, anim_delta);
        }
        if (cutscene->version > 1 && (cutscene->flags & 1) != 0 && flags == 0) {
            usize stream_buffer;
            if (cutscene->camera_system != NULL && cutscene->camera_system->animation != NULL) {
                stream_buffer = reinterpret_cast<usize>(cutscene->camera_system->animation);
            } else if (cutscene->rigid_system != NULL && cutscene->rigid_system->rigids[0].animation != NULL) {
                stream_buffer = reinterpret_cast<usize>(cutscene->rigid_system->rigids[0].animation);
            } else if (cutscene->character_system != NULL) {
                NUGCUTCHAR_s *character = cutscene->character_system->characters;
                stream_buffer = reinterpret_cast<usize>(character->animation);
                if (stream_buffer == 0) {
                    stream_buffer = reinterpret_cast<usize>(character->face_animation);
                }
            } else {
                stream_buffer = 0;
            }
            if (cutscene->field_4c != 0) {
                stream_buffer = buf->addr + cutscene->field_4c;
            }
            stream_buffer = ALIGN(stream_buffer, 0x10);
            cutscene->stream_buffer_0 = reinterpret_cast<NUGCUTSCENE_s *>(stream_buffer);
            stream_buffer += cutscene->stream_buffer_size;
            if (NumCommonStreamingBuffers == 0) {
                stream_buffer = ALIGN(stream_buffer + 0x400, 0x10);
                cutscene->stream_buffer_1 = reinterpret_cast<NUGCUTSCENE_s *>(stream_buffer);
                stream_buffer += cutscene->stream_buffer_size + 0x400;
            } else {
                stream_buffer += 0x400;
            }
            buf->addr = stream_buffer;
            char *filename = reinterpret_cast<char *>(buf->void_ptr);
            buf->addr += NuStrLen(name) + 4;
            NuStrCpy(filename, name);
            cutscene->filename = filename;
            return cutscene;
        }
        buf->addr += bytes;
        return cutscene;
    }
    void NuGCutSceneFixUp(NUGCUTSCENE_s *cutscene, NUGSCN *scene, i32 flags, i8 area) {
        if (cutscene == NULL) {
            return;
        }
        if (cutscene->version > 1) {
            cutscene->scene = scene;
            cutscene->extra_scene = reinterpret_cast<void *>(static_cast<usize>(flags));
        }
        if (scene != NULL && cutscene->rigid_system != NULL && cutscene->rigid_system->rigids != NULL) {
            for (u32 i = 0; i < cutscene->rigid_system->count; ++i) {
                NUGCUTRIGID_s *rigid = &cutscene->rigid_system->rigids[i];
                nuhspecial_s special;
                if (NuSpecialFind(scene, &special, rigid->name, 1) != 0) {
                    rigid->flags |= 4;
                    rigid->scene = special.scene;
                    rigid->special_object = special.display_special != NULL ? special.display_special : special.special;
                }
                if (rigid->locator_count != 0 && cutscene->locator_system != NULL && rigid->locator < 0xff) {
                    rigid->locator_index = static_cast<u8>(rigid->locator);
                    rigid->locator = reinterpret_cast<usize>(&cutscene->locator_system->locators[rigid->locator_index]);
                } else {
                    rigid->locator_index = 0xff;
                }
            }
        }
        if (cutscene->character_system != NULL && NuCutSceneFindCharacters != NULL) {
            NuCutSceneFindCharacters(cutscene);
        }
        NUGCUTLOCATORSYS_s *system = cutscene->locator_system;
        if (system == NULL || system->types == NULL || system->type_count == 0) {
            return;
        }
        for (u32 i = 0; i < system->type_count; ++i) {
            NUGCUTLOCATORTYPE_s *type = &system->types[i];
            if ((type->flags & 1) != 0) {
                if (NuGCutDebFixUp_SearchAllPages == 0) {
                    type->function_index = static_cast<u16>(LookupDebrisEffectPageOnly(type->name, area));
                } else {
                    type->function_index = static_cast<u16>(LookupDebrisEffectPage(type->name, area));
                }
            } else if ((type->flags & 2) != 0) {
                i32 function_index = -1;
                if (locatorfns != NULL && type->name != NULL && locatorfns[0].name != NULL) {
                    for (i32 j = 0; locatorfns[j].name != NULL; ++j) {
                        if (NuStrICmp(type->name, locatorfns[j].name) == 0) {
                            function_index = j;
                            break;
                        }
                    }
                }
                type->function_index = static_cast<u16>(function_index);
            } else if ((type->flags & 0x10) != 0 && LookupLocatorVfxFn != NULL && type->name != NULL) {
                char *underscore = NuStrRChr(type->name, '_');
                if (underscore != NULL && underscore[1] > '/' && underscore[1] < ':') {
                    *underscore = '\0';
                }
                type->function_index = static_cast<u16>(LookupLocatorVfxFn(type->name));
            } else if ((type->flags & 4) != 0 && NuCutSceneSFXFixUp != NULL && type->name != NULL) {
                type->function_index = static_cast<u16>(NuCutSceneSFXFixUp(reinterpret_cast<usize>(type->name)));
                if (static_cast<i16>(type->function_index) != -1) {
                    cutscene->flags |= 4;
                }
            }
        }
    }
    void NuGCutSceneFixUpExtra(NUGCUTSCENE_s *cutscene, NUGSCN *area) {
        if (cutscene == NULL || cutscene->rigid_system == NULL || area == NULL) {
            return;
        }

        NUGCUTRIGIDSYS_s *system = cutscene->rigid_system;
        for (u32 i = 0; i < system->count; ++i) {
            NUGCUTRIGID_s *rigid = &system->rigids[i];
            if ((rigid->flags & 4) != 0) {
                continue;
            }

            nuhspecial_s special;
            if (NuSpecialFind(area, &special, rigid->name, 1) == 0) {
                continue;
            }
            rigid->flags |= 4;
            rigid->scene = special.scene;
            rigid->special_object = special.display_special != NULL ? special.display_special : special.special;
        }
    }
    instNUGCUTSCENE_s *instNuGCutSceneCreate(NUGCUTSCENE_s *cutscene, NUGSCN *scene, void *extra, char *name,
                                             VARIPTR *buf, i32 flags) {
        if (cutscene == NULL) {
            return NULL;
        }
        buf->addr = ALIGN(buf->addr, 0x10);
        instNUGCUTSCENE_s *instance = reinterpret_cast<instNUGCUTSCENE_s *>(buf->void_ptr);
        buf->void_ptr = instance + 1;
        memset(instance, 0, sizeof(*instance));
        instance->alpha = 1.0f;
        instance->field_c4 = scene;
        instance->field_c8 = extra;
        if (cutscene->version > 1) {
            const u8 cutscene_flags = static_cast<u8>(cutscene->flags);
            if ((cutscene_flags & 1) != 0) {
                instance->stream_buffer_0 = cutscene->stream_buffer_0;
                instance->stream_buffer_1 = cutscene->stream_buffer_1;
                instance->pending_stream_buffer = cutscene->stream_buffer_1;
                instance->flags_8b &= ~0x10U;
            }
            instance->flags_8b = (instance->flags_8b & ~0x20U) | ((cutscene_flags & 9) != 0 ? 0x20 : 0);
        }
        instance->cutscene = cutscene;
        instance->cutscene_copy = cutscene;
        if (name != NULL) {
            sprintf(instance->name, name);
        }

        if (cutscene->camera_system != NULL && cutscene->camera_system->camera_count != 0) {
            buf->addr = ALIGN(buf->addr, 0x10);
            instance->camera_instance = reinterpret_cast<instNUGCUTSCENECAMERA_s *>(buf->void_ptr);
            buf->void_ptr = instance->camera_instance + 1;
            memset(instance->camera_instance, 0, sizeof(*instance->camera_instance));
            buf->addr = ALIGN(buf->addr, 0x10);
            instance->camera_instance->camera_states = reinterpret_cast<instNUGCUTCAMSTATE_s *>(buf->void_ptr);
            buf->void_ptr = instance->camera_instance->camera_states + cutscene->camera_system->camera_count;
            memset(instance->camera_instance->camera_states, 0,
                   cutscene->camera_system->camera_count * sizeof(instNUGCUTCAMSTATE_s));
        }

        if (cutscene->rigid_system != NULL && cutscene->rigid_system->count != 0) {
            instance->rigid_instance = reinterpret_cast<instNUGCUTRIGIDSYS_s *>(ALIGN(buf->addr, 0x10));
            buf->void_ptr = instance->rigid_instance + 1;
            buf->addr = ALIGN(buf->addr, 0x10);
            instance->rigid_instance->rigids = reinterpret_cast<instNUGCUTRIGID_s *>(buf->void_ptr);
            buf->void_ptr = instance->rigid_instance->rigids + cutscene->rigid_system->count;
            memset(instance->rigid_instance->rigids, 0, cutscene->rigid_system->count * sizeof(instNUGCUTRIGID_s));
            for (u32 i = 0; i < cutscene->rigid_system->count; ++i) {
                NUGCUTRIGID_s *rigid = &cutscene->rigid_system->rigids[i];
                instNUGCUTRIGID_s *inst_rigid = &instance->rigid_instance->rigids[i];
                if (scene == NULL || scene == rigid->scene) {
                    inst_rigid->scene = scene == NULL ? rigid->scene : scene;
                    if (inst_rigid->scene != NULL && inst_rigid->scene->display_list != NULL) {
                        inst_rigid->special = NULL;
                        inst_rigid->display_special = rigid->special_object;
                    } else {
                        inst_rigid->special = rigid->special_object;
                        inst_rigid->display_special = NULL;
                    }
                } else if (rigid->special_object != NULL) {
                    inst_rigid->scene = scene;
                    if (scene->display_list == NULL) {
                        const usize offset = (reinterpret_cast<usize>(rigid->special_object) -
                                              reinterpret_cast<usize>(rigid->scene->specials)) &
                                             ~static_cast<usize>(3);
                        inst_rigid->special = reinterpret_cast<u8 *>(scene->specials) + offset;
                        inst_rigid->display_special = NULL;
                    } else {
                        const usize offset = (reinterpret_cast<usize>(rigid->special_object) -
                                              reinterpret_cast<usize>(rigid->scene->display_list->specials)) &
                                             ~static_cast<usize>(0xf);
                        inst_rigid->special = NULL;
                        inst_rigid->display_special = reinterpret_cast<u8 *>(scene->display_list->specials) + offset;
                    }
                    if ((rigid->flags & 2) != 0) {
                        inst_rigid->visible = rigid->flags & 1;
                    }
                }
            }
        }

        if (cutscene->character_system != NULL && cutscene->character_system->character_count != 0) {
            instance->character_instance = reinterpret_cast<instNUGCUTCHARSYS_s *>(ALIGN(buf->addr, 0x10));
            buf->void_ptr = instance->character_instance + 1;
            buf->addr = ALIGN(buf->addr, 0x10);
            instance->character_instance->characters = reinterpret_cast<instNUGCUTCHAR_s *>(buf->void_ptr);
            buf->void_ptr = instance->character_instance->characters + cutscene->character_system->character_count;
            memset(instance->character_instance->characters, 0,
                   cutscene->character_system->character_count * sizeof(instNUGCUTCHAR_s));
            for (u32 i = 0; i < cutscene->character_system->character_count; ++i) {
                NUGCUTCHAR_s *character = &cutscene->character_system->characters[i];
                instNUGCUTCHAR_s *inst_character = &instance->character_instance->characters[i];
                inst_character->field_16 = 0xff;
                inst_character->field_15 = 0xff;
                if ((character->flags & 2) != 0) {
                    if (NuCutSceneCharacterCreateData != NULL) {
                        NuCutSceneCharacterCreateData(character, inst_character, buf);
                    }
                } else {
                    inst_character->character_model = character->character_model;
                }
            }
        }

        if (cutscene->locator_system != NULL && cutscene->locator_system->locator_count != 0) {
            instance->locator_instance = reinterpret_cast<instNUGCUTLOCATORSYS_s *>(ALIGN(buf->addr, 0x10));
            buf->void_ptr = instance->locator_instance + 1;
            buf->addr = ALIGN(buf->addr, 0x10);
            instance->locator_instance->locators = reinterpret_cast<instNUGCUTLOCATOR_s *>(buf->void_ptr);
            buf->void_ptr = instance->locator_instance->locators + cutscene->locator_system->locator_count;
            memset(instance->locator_instance->locators, 0,
                   cutscene->locator_system->locator_count * sizeof(instNUGCUTLOCATOR_s));
            for (u32 i = 0; i < cutscene->locator_system->locator_count; ++i) {
                NUGCUTLOCATOR_s *locator = &cutscene->locator_system->locators[i];
                NUGCUTLOCATORTYPE_s *type = &cutscene->locator_system->types[locator->type_index];
                instNUGCUTLOCATOR_s *inst_locator = &instance->locator_instance->locators[i];
                if ((type->flags & 1) != 0) {
                    if ((locator->flags & 0x20) != 0) {
                        inst_locator->effect_handle = -1;
                    }
                } else if ((type->flags & 2) != 0) {
                } else if ((type->flags & 0x10) != 0) {
                    inst_locator->effect_handle = -1;
                } else if ((type->flags & 8) != 0) {
                    buf->addr = ALIGN(buf->addr, 4);
                    void *effect = buf->void_ptr;
                    inst_locator->effect_handle = static_cast<i32>(reinterpret_cast<usize>(effect));
                    buf->addr += 0x18;
                    memset(effect, 0, 0x18);
                    if (locator->field_5b != 0) {
                        *reinterpret_cast<NUGCUTLOCATOR_s **>(static_cast<u8 *>(effect) + 0x14) = locator;
                    }
                } else if ((type->flags & 4) != 0) {
                    buf->addr = ALIGN(buf->addr, 0x10);
                    void *effect = buf->void_ptr;
                    inst_locator->effect_handle = static_cast<i32>(reinterpret_cast<usize>(effect));
                    buf->addr += 0x10;
                    memset(effect, 0, 0x10);
                }
            }
        }

        if (cutscene->trigger_system != NULL && extra != NULL) {
            const usize state_size = static_cast<usize>(cutscene->trigger_system->event_count) * sizeof(u32);
            buf->addr = ALIGN(buf->addr, 0x10);
            instance->trigger_instance = reinterpret_cast<instNUGCUTTRIGGERSYS_s *>(buf->void_ptr);
            buf->void_ptr = instance->trigger_instance + 1;
            instance->trigger_instance->owner = extra;
            instance->trigger_instance->event_states = reinterpret_cast<u32 *>(buf->void_ptr);
            buf->addr += state_size;
            memset(instance->trigger_instance->event_states, 0, state_size);
        }

        NUVEC *bounds = static_cast<NUVEC *>(cutscene->bounds);
        if (bounds == NULL) {
            instance->transformed_bounds_center.x = 0.0f;
            instance->transformed_bounds_center.y = 0.0f;
            instance->transformed_bounds_center.z = 0.0f;
        } else {
            instance->transformed_bounds_center.x = (bounds[1].x + bounds[0].x) * 0.5f;
            instance->transformed_bounds_center.y = (bounds[1].y + bounds[0].y) * 0.5f;
            instance->transformed_bounds_center.z = (bounds[1].z + bounds[0].z) * 0.5f;
        }
        instance->rate = 1.0f;

        (*NuThreadDisableThreadSwap)();
        if (flags == 0) {
            instance->next = active_cutscene_instances;
            if (active_cutscene_instances != NULL) {
                active_cutscene_instances->previous = instance;
            }
            active_cutscene_instances = instance;
        } else {
            instance->next = background_cutscene_instances;
            if (background_cutscene_instances != NULL) {
                background_cutscene_instances->previous = instance;
            }
            background_cutscene_instances = instance;
        }
        instance->allocation_size = static_cast<i32>(buf->addr - reinterpret_cast<usize>(instance));
        (*NuThreadEnableThreadSwap)();
        return instance;
    }

    void instNuGCutSceneReset(instNUGCUTSCENE_s *instance) {
        if (instance == NULL) {
            return;
        }
        instance->current_frame = 1.0f;
        instance->render_frame = 1.0f;
        instance->flags_88 &= 0xf8;
        instance->flags_89 &= 0xef;
        instance->flags_8c &= 0xbf;
        instance->flags_8d &= 0xef;
        if (instance->rate < 0.0f) {
            instance->rate = -instance->rate;
        }
        instance->cutscene = instance->cutscene_copy;
        if (instance->rigid_instance != NULL && instance->cutscene->rigid_system != NULL) {
            for (u32 i = 0; i < instance->cutscene->rigid_system->count; ++i) {
                NUGCUTRIGID_s *rigid = &instance->cutscene->rigid_system->rigids[i];
                instNUGCUTRIGID_s *inst_rigid = &instance->rigid_instance->rigids[i];
                inst_rigid->state_index = 0;
                if ((rigid->flags & 6) == 6) {
                    inst_rigid->visible = rigid->flags & 1;
                }
            }
        }
    }

    void instNuGCutSceneStart(instNUGCUTSCENE_s *instance) {
        instance->current_frame = 1.0f;
        instance->render_frame = 1.0f;
        instance->flags_89 &= 0xef;
        instance->flags_8d &= 0xef;
        instance->flags_88 = (instance->flags_88 & 0xfe) | 2;
        if (instance->rate < 0.0f) {
            instance->rate = -instance->rate;
        }
        if (instance->camera_instance != NULL && instance->cutscene->camera_system != NULL) {
            instNUGCUTSCENECAMERA_s *camera = instance->camera_instance;
            NUGCUTCAMERASYS_s *system = instance->cutscene->camera_system;
            camera->state_index = 0;
            camera->next_target_index = 0;
            camera->camera_index = static_cast<i8>(system->field_10);
            for (u32 i = 0; i < system->camera_count; ++i) {
                camera->camera_states[i].flags &= ~2U;
                camera->camera_states[i].event_index = 0;
            }
        }
        if (instance->rigid_instance != NULL && instance->cutscene->rigid_system != NULL) {
            for (u32 i = 0; i < instance->cutscene->rigid_system->count; ++i) {
                NUGCUTRIGID_s *rigid = &instance->cutscene->rigid_system->rigids[i];
                instNUGCUTRIGID_s *inst_rigid = &instance->rigid_instance->rigids[i];
                inst_rigid->state_index = 0;
                if ((rigid->flags & 6) == 6) {
                    inst_rigid->visible = rigid->flags & 1;
                }
            }
        }
    }

    void instNuGCutScenePause(instNUGCUTSCENE_s *instance, u8 paused) {
        instance->paused = paused;
    }

    void instNuGCutSceneDestroy(instNUGCUTSCENE_s *instance) {
        instance->cutscene = instance->cutscene_copy;
        if ((instance->flags_88 & 2) != 0) {
            instNuGCutSceneEnd(instance);
        }
        if (instance->next != NULL) {
            instance->next->previous = instance->previous;
        }
        if (instance->previous == NULL) {
            active_cutscene_instances = instance->next;
        } else {
            instance->previous->next = instance->next;
        }
    }

    i32 NuGCutSceneIsBackgroundLoading(void) {
        return 1;
    }

    i32 instNuGCutScenePreload(instNUGCUTSCENE_s *instance) {
        if (cutscene_load_instance != NULL) {
            return 0;
        }
        cutscene_load_instance = instance;
        return 1;
    }

    void instNuGCutSceneServiceLoad(void) {
        if (cutscene_load_instance == NULL || cutscene_load_instance->cutscene == NULL) {
            return;
        }

        char path[256];
        char extension[8];
        NuStrCpy(path, cutscene_load_instance->cutscene->filename);
        char *dot = strchr(path, '.');
        NuStrCpy(extension, dot);

        i32 stream_number = cutscene_load_instance->stream_index + 1;
        i32 tens = stream_number / 10;
        if (tens != 0) {
            *dot++ = static_cast<char>('0' + tens);
        }
        *dot = static_cast<char>('0' + stream_number - tens * 10);
        NuStrCpy(dot + 1, extension);

        instNUGCUTSCENE_s *instance = cutscene_load_instance;
        VARIPTR buffer = {instance->pending_stream_buffer};
        instance->pending_stream_buffer = NULL;
        if (buffer.void_ptr == NULL) {
            return;
        }

        instance->flags_8c |= 0x80;
        VARIPTR buffer_end;
        buffer_end.addr = buffer.addr + instance->cutscene->stream_buffer_size;
        NuGCutSceneLoad(path, &buffer, &buffer_end, 1);
        instance = cutscene_load_instance;
        instance->flags_8b |= 8;
        if (instance->stream_index == 0 && (instance->cutscene->flags & 8) != 0) {
            instNuGCutSceneSwapBuffers(instance, 1);
            instance = cutscene_load_instance;
            if (NumCommonStreamingBuffers > 1) {
                instance->flags_8b |= 0x10;
                NewCopyAnims(instance);
            } else {
                copyAnims(instance->cutscene, static_cast<NUGCUTSCENE_s *>(instance->cutscene->stream_buffer_1));
            }
            instance->cutscene->flags &= ~8U;
            instance->flags_8b &= ~8U;
            instance->stream_index = 1;
        }
        instance->flags_8c &= ~0x80U;
        cutscene_load_instance = NULL;
    }

    void NuGCutSceneSysBackgroundFlush(void) {
        background_cutscene_instances = NULL;
    }

    void NuGCutSceneSysPostBackgroundLoad(void) {
        instNUGCUTSCENE_s *instance = background_cutscene_instances;
        instNUGCUTSCENE_s *active = active_cutscene_instances;
        if (instance == NULL) {
            return;
        }

        do {
            instNUGCUTSCENE_s *next = instance->next;
            instance->next = active;
            if (active != NULL) {
                active->previous = instance;
            }
            instance->previous = NULL;
            active = instance;
            instance = next;
        } while (instance != NULL);

        background_cutscene_instances = NULL;
        active_cutscene_instances = active;
    }
} // extern "C"

struct instNUGCUTSCENE_s;
struct NUGCUTLOCATORSYS_s;
struct instNUGCUTLOCATOR_s;
struct NUGCUTLOCATOR_s;
struct numtx_s;
extern "C" i32 NuGCutLocatorCalcMtx(NUGCUTLOCATOR_s *, f32, NUMTX *, nuanimtime_s *);
extern "C" i32 NuGCutLocatorIsVisble(NUGCUTLOCATOR_s *, f32, nuanimtime_s *, f32 *, f32 *);
extern "C" void NuAnimData2CalcTime(nuanimdata2_s *, f32, nuanimtime_s *);
extern "C" void instNuGCutLocatorUpdate(instNUGCUTSCENE_s *, NUGCUTLOCATORSYS_s *, instNUGCUTLOCATOR_s *,
                                        NUGCUTLOCATOR_s *, f32, NUMTX *, i32);
void Draw3DObjectMtx(WORLDINFO_s *, i32, numtx_s *);
extern CUTSCENESYS *CutSceneSys;

static __used__ void LocatorFunction_Blaster(instNUGCUTSCENE_s *, NUGCUTLOCATORSYS_s *, instNUGCUTLOCATOR_s *,
                                             NUGCUTLOCATOR_s *locator, float frame, numtx_s *parent_mtx, int) {
    nuanimtime_s time;
    NuAnimData2CalcTime(locator->animation, frame, &time);
    if (NuGCutLocatorIsVisble(locator, frame, &time, NULL, NULL) == 0) {
        return;
    }
    NUMTX matrix;
    NuGCutLocatorCalcMtx(locator, frame, &matrix, &time);
    if ((locator->flags & 4) != 0) {
        NuMtxPreTranslate(&matrix, &locator->pivot);
    }
    if (parent_mtx != NULL) {
        NuMtxMul(&matrix, &matrix, parent_mtx);
    }
    if (CutSceneSys != NULL) {
        Draw3DObjectMtx(NULL, CutSceneSys->blaster_object_0, &matrix);
        Draw3DObjectMtx(NULL, CutSceneSys->blaster_object_1, &matrix);
    }
}

extern "C" {
    __attribute__((weak)) NUGCUTLOCATORFNENTRY_s cutscene_locatorfns[] = {
        {"blaster", 0, 0, 0, LocatorFunction_Blaster},
        {NULL, 0, 0, 0, NULL},
    };
}

void instNuGCutSceneEndButNotSystems(instNUGCUTSCENE_s *instance);
void instNuGCutSceneResetCamLock(instNUGCUTSCENE_s *instance);
extern "C" void DebFreeInstantly(i32 *handle);
extern "C" void (*ReleaseLocatorVfxFn)(i32);

static __used__ void instNuGCutRigidSysEnd(instNUGCUTSCENE_s *instance, float frame) {
    NUGCUTRIGIDSYS_s *system = instance->cutscene->rigid_system;
    instNUGCUTRIGID_s *inst_rigids = instance->rigid_instance->rigids;

    for (u32 i = 0; i < system->count; ++i) {
        NUGCUTRIGID_s *rigid = &system->rigids[i];
        if ((rigid->flags & 4) == 0 || (rigid->flags & 2) != 0) {
            continue;
        }

        instNUGCUTRIGID_s *inst_rigid = &inst_rigids[i];
        if (rigid->state_animation != NULL) {
            u8 visible;
            if (StateAnimEvaluate(rigid->state_animation, &inst_rigid->state_index, &visible, frame) != 0) {
                NuSpecialSetVisibility(inst_rigid, visible != 0);
            }
        }

        if (NuSpecialGetVisibilityFn(inst_rigid) == 0) {
            continue;
        }

        NUMTX matrix;
        NuGCutRigidCalcMtx(rigid, frame, &matrix);
        if (static_cast<i8>(instance->flags_88) < 0) {
            NuMtxMul(&matrix, &matrix, &instance->matrix);
        }
        NuSpecialSetDrawMtx(inst_rigid, &matrix);
    }
}

static void instNuGCutLocatorSysEnd(instNUGCUTLOCATORSYS_s *instance, NUGCUTLOCATORSYS_s *system, float) {
    for (u32 i = 0; i < system->locator_count; ++i) {
        instNUGCUTLOCATOR_s *inst_locator = &instance->locators[i];
        inst_locator->field_00 = 0;
        NUGCUTLOCATOR_s *locator = &system->locators[i];
        NUGCUTLOCATORTYPE_s *type = &system->types[locator->type_index];
        if ((type->flags & 8) != 0) {
            u8 *sfx = reinterpret_cast<u8 *>(static_cast<usize>(static_cast<u32>(inst_locator->effect_handle)));
            if (*reinterpret_cast<i16 *>(sfx + 8) != 0 && *reinterpret_cast<void **>(sfx) != NULL) {
                u8 *owner = *reinterpret_cast<u8 **>(sfx);
                u8 *table = *reinterpret_cast<u8 **>(owner + 0xc);
                const u16 indices[3] = {*reinterpret_cast<u16 *>(sfx + 0xe), *reinterpret_cast<u16 *>(sfx + 0x10),
                                        *reinterpret_cast<u16 *>(sfx + 0xa)};
                for (i32 j = 0; j < 3; ++j) {
                    if (indices[j] != 0) {
                        u8 *voice = *reinterpret_cast<u8 **>(table - 4 + indices[j] * 4);
                        *reinterpret_cast<i32 *>(voice + 0x80) = 0;
                        *reinterpret_cast<i32 *>(voice + 0x84) = 0;
                    }
                }
            }
        } else if ((type->flags & 1) != 0) {
            if ((locator->flags & 0x20) != 0 && inst_locator->effect_handle >= 0) {
                DebFreeInstantly(&inst_locator->effect_handle);
            }
        } else if ((type->flags & 0x10) != 0) {
            if (ReleaseLocatorVfxFn != NULL) {
                ReleaseLocatorVfxFn(inst_locator->effect_handle);
            }
            inst_locator->effect_handle = -1;
            inst_locator->field_00 = 0;
        }
    }
}

void instNuGCutSceneEndFirstFrame(instNUGCUTSCENE_s *instance) {
    NUGCUTSCENE_s *cutscene = instance->cutscene;
    instance->current_frame = 1.0f;
    instance->render_frame = 1.0f;
    instance->flags_88 &= ~2U;
    instance->flags_89 &= ~0x10U;
    instance->flags_8c &= ~0x40U;

    f32 frame;
    if ((instance->flags_8a & 4) != 0) {
        frame = cutscene->duration - instance->current_frame;
        if (cutscene->rigid_system != NULL) {
            instNuGCutRigidSysEnd(instance, frame);
        }
        if (instance->character_instance != NULL) {
            NUGCUTCHARSYS_s *system = cutscene->character_system;
            for (i32 i = 0; i < system->character_count; ++i) {
                instNUGCUTCHAR_s *inst_character = &instance->character_instance->characters[i];
                NUGCUTCHAR_s *character = &system->characters[i];
                if (inst_character->character_model != NULL) {
                    if ((character->flags & 2) == 0 && NuCutSceneCharacterEval != NULL) {
                        NuCutSceneCharacterEval(instance, cutscene, inst_character, character, frame);
                    }
                    if (nu_current_thread_id == 0 && NuCutSceneCharacterRelease != NULL) {
                        NuCutSceneCharacterRelease(inst_character, character);
                    }
                }
            }
        }
    } else {
        frame = instance->current_frame;
        if (cutscene->rigid_system != NULL) {
            instNuGCutRigidSysEnd(instance, frame);
        }
        if (instance->character_instance != NULL) {
            NUGCUTCHARSYS_s *system = cutscene->character_system;
            for (i32 i = 0; i < system->character_count; ++i) {
                instNUGCUTCHAR_s *inst_character = &instance->character_instance->characters[i];
                NUGCUTCHAR_s *character = &system->characters[i];
                if (inst_character->character_model != NULL) {
                    if ((character->flags & 2) == 0 && NuCutSceneCharacterEval != NULL) {
                        NuCutSceneCharacterEval(instance, cutscene, inst_character, character, frame);
                    }
                    if (nu_current_thread_id == 0 && NuCutSceneCharacterRelease != NULL) {
                        NuCutSceneCharacterRelease(inst_character, character);
                    }
                }
            }
        }
    }
    if (instance->locator_instance != NULL) {
        instNuGCutLocatorSysEnd(instance->locator_instance, cutscene->locator_system, frame);
    }
    instNuGCutSceneResetCamLock(instance);
}

extern "C" void instNuGCutSceneEnd(instNUGCUTSCENE_s *instance) {
    NUGCUTSCENE_s *cutscene = instance->cutscene;
    instNuGCutSceneEndButNotSystems(instance);

    const f32 end_frame = cutscene->duration;
    instance->flags_88 &= ~2U;
    instance->flags_89 |= 0x10;
    instance->current_frame = end_frame;
    instance->flags_8c &= ~0x40U;
    ForcePlayEndFrame = 1;

    f32 frame;
    if ((instance->flags_8a & 4) != 0) {
        frame = cutscene->duration - instance->current_frame;
        if (cutscene->rigid_system != NULL) {
            instNuGCutRigidSysEnd(instance, frame);
        }
        if (instance->character_instance != NULL) {
            NUGCUTCHARSYS_s *system = cutscene->character_system;
            for (i32 i = 0; i < system->character_count; ++i) {
                NUGCUTCHAR_s *character = &system->characters[i];
                instNUGCUTCHAR_s *inst_character = &instance->character_instance->characters[i];
                if (inst_character->character_model != NULL) {
                    if ((character->flags & 2) == 0 && NuCutSceneCharacterEval != NULL) {
                        NuCutSceneCharacterEval(instance, cutscene, inst_character, character, frame);
                    }
                    if (nu_current_thread_id == 0 && NuCutSceneCharacterRelease != NULL) {
                        NuCutSceneCharacterRelease(inst_character, character);
                    }
                }
            }
        }
    } else {
        frame = instance->current_frame;
        if (cutscene->rigid_system != NULL) {
            instNuGCutRigidSysEnd(instance, frame);
        }
        if (instance->character_instance != NULL) {
            NUGCUTCHARSYS_s *system = cutscene->character_system;
            for (i32 i = 0; i < system->character_count; ++i) {
                NUGCUTCHAR_s *character = &system->characters[i];
                instNUGCUTCHAR_s *inst_character = &instance->character_instance->characters[i];
                if (inst_character->character_model != NULL) {
                    if ((character->flags & 2) == 0 && NuCutSceneCharacterEval != NULL) {
                        NuCutSceneCharacterEval(instance, cutscene, inst_character, character, frame);
                    }
                    if (nu_current_thread_id == 0 && NuCutSceneCharacterRelease != NULL) {
                        NuCutSceneCharacterRelease(inst_character, character);
                    }
                }
            }
        }
    }

    if (instance->locator_instance != NULL) {
        instNuGCutLocatorSysEnd(instance->locator_instance, cutscene->locator_system, frame);
    }
    ForcePlayEndFrame = 0;
    instNuGCutSceneResetCamLock(instance);
}

static void instNuGCutRigidSysUpdate(instNUGCUTSCENE_s *, float, int);
static void instNuGCutCamSysUpdate(instNUGCUTSCENE_s *, float);
static void instNuGCutTriggerSysUpdate(instNUGCUTSCENE_s *, float);
static void instNuGCutSceneClipTest(instNUGCUTSCENE_s *);
extern "C" void instNuGCutSceneEnd(instNUGCUTSCENE_s *instance);

static inline i32 instNuGCutSceneRepeatCount(instNUGCUTSCENE_s *instance) {
    const u32 packed = *reinterpret_cast<u32 *>(&instance->flags_88);
    return static_cast<i32>(packed << 14) >> 27;
}

static inline void instNuGCutSceneDecrementRepeatCount(instNUGCUTSCENE_s *instance, i32 repeat_count) {
    u32 packed = *reinterpret_cast<u32 *>(&instance->flags_88);
    packed &= 0xfffc1fffU;
    packed |= static_cast<u32>((repeat_count - 1) & 0x1f) << 13;
    *reinterpret_cast<u32 *>(&instance->flags_88) = packed;
}

static inline u8 instNuGCutSceneLastStream(instNUGCUTSCENE_s *instance) {
    return instance->cutscene_copy->last_stream;
}

static inline NUGCUTSCENE_s *instNuGCutSceneCurrentStreamBuffer(instNUGCUTSCENE_s *instance) {
    return (instance->flags_8b & 0x10) != 0 ? instance->stream_buffer_1 : instance->stream_buffer_0;
}

static inline void instNuGCutSceneCopyStreamAnimations(instNUGCUTSCENE_s *instance) {
    if (NumCommonStreamingBuffers > 1) {
        NewCopyAnims(instance);
    } else {
        copyAnims(instance->cutscene, instNuGCutSceneCurrentStreamBuffer(instance));
    }
}

static __used__ void instNuGCutSceneUpdate(instNUGCUTSCENE_s *instance, int paused, int skip, float elapsed) {
    NUGCUTSCENE_s *cutscene = instance->cutscene;
    instance->flags_8d &= ~0x10U;
    u16 *sync_flags = reinterpret_cast<u16 *>(&instance->flags_8a);
    *sync_flags = static_cast<u16>((*sync_flags & 0xf807U) | (cutscene_synchro_counter << 3));
    if ((instance->flags_89 & 8) != 0 || (cutscene->flags & 8) != 0) {
        return;
    }
    instance->elapsed = elapsed;
    if (NuCutSceneRequestSFX != NULL && (cutscene->flags & 4) != 0 && (instance->flags_8c & 0x20) != 0) {
        NUMTX *camera = NuCameraGetMtx();
        if (camera != NULL && camera->m30 > instance->bounds_min.x && camera->m30 < instance->bounds_max.x &&
            camera->m31 > instance->bounds_min.y && camera->m31 < instance->bounds_max.y &&
            camera->m32 > instance->bounds_min.z && camera->m32 < instance->bounds_max.z) {
            NuCutSceneRequestSFX(instance);
        }
    }
    instance->flags_8b &= ~0x40U;
    if (instance->skip_countdown != 0 && (instance->flags_88 & 1) == 0 && (instance->flags_8c & 0xf) != 0) {
        instance->flags_8b |= 0x40;
        instance->flags_8c = (instance->flags_8c & 0xf0) | ((instance->flags_8c - 1) & 0xf);
        paused = 1;
    }
    if ((instance->flags_88 & 2) == 0) {
        if ((instance->flags_88 & 0x41) != 0x40) {
            return;
        }
        instNuGCutSceneStart(instance);
        cutscene = instance->cutscene;
    }
    if ((cutscene->flags & 8) != 0 && (instance->flags_8b & 8) == 0) {
        return;
    }
    if (cutscene->version > 1 && (cutscene->flags & 1) != 0) {
        if ((instance->flags_8b & 0x20) != 0 && cutscene_load_instance == NULL && (instance->flags_8b & 8) == 0 &&
            instance->stream_index != instNuGCutSceneLastStream(instance)) {
            cutscene_load_instance = instance;
        }
        if (instance->queued_stream_instance != NULL && cutscene_load_instance == NULL &&
            instance->stream_index == instNuGCutSceneLastStream(instance)) {
            instNUGCUTSCENE_s *queued_instance = instance->queued_stream_instance;
            instance->queued_stream_instance = NULL;
            cutscene_load_instance = queued_instance;
        }
    }
    instNuGCutSceneClipTest(instance);
    if ((instance->flags_89 & 4) == 0) {
        return;
    }
    if ((instance->flags_88 & 4) != 0) {
        paused = 1;
    }
    if (termcutstream_hack != 0 || (instance->flags_8c & 0x10) != 0) {
        if (cutscene->version > 1 && (cutscene->flags & 1) != 0) {
            if (skip != 0) {
                instance->flags_8d |= 4;
            } else {
                cutscene_quit_prompt = 1;
            }
        } else if ((instance->flags_8c & 0x10) != 0) {
            if (skip != 0) {
                instance->current_frame = cutscene->duration - 1.0f - instance->rate * elapsed;
                cutscene_has_quit = 1;
            } else {
                cutscene_quit_prompt = 1;
            }
        }
    }
    if ((instance->flags_8d & 2) != 0) {
        cutscene_lock_player = 1;
    }
    if ((instance->flags_88 & 1) == 0) {
        if (paused == 0) {
            instance->flags_88 |= 1;
        }
    } else if (paused == 0) {
        const f32 frame_delta = instance->rate * elapsed;
        instance->current_frame += frame_delta;
        bool streaming = cutscene->version > 1 && (cutscene->flags & 1) != 0;
        if (streaming && (instance->flags_8d & 4) != 0) {
            const u8 last_stream = instNuGCutSceneLastStream(instance);
            if (instance->stream_index == last_stream) {
                instance->current_frame = cutscene->duration - 1.0f;
            } else {
                instance->current_frame -= frame_delta;
                if (cutscene_load_instance == NULL) {
                    if (instance->stream_index == static_cast<u8>(last_stream - 1)) {
                        instNuGCutSceneSwapBuffers(instance, 0);
                        instNuGCutSceneCopyStreamAnimations(instance);
                        instance->stream_index = last_stream;
                        instNuGCutSceneStart(instance);
                        cutscene = instance->cutscene;
                        instance->current_frame = cutscene->duration - 1.0f;
                    } else {
                        instance->stream_index = static_cast<u8>(last_stream - 1);
                        instance->pending_stream_buffer = (instance->flags_8b & 0x10) != 0
                                                              ? static_cast<void *>(instance->stream_buffer_0)
                                                              : static_cast<void *>(instance->stream_buffer_1);
                        cutscene_load_instance = instance;
                    }
                }
            }
            cutscene = instance->cutscene;
            streaming = cutscene->version > 1 && (cutscene->flags & 1) != 0;
        }
        const f32 end_frame = cutscene->duration - 1.0f;
        if (instance->current_frame < 1.0f) {
            if ((instance->flags_88 & 0x10) != 0) {
                if ((instance->flags_88 & 8) != 0) {
                    if (instance->rate < 0.0f) {
                        instance->rate = -instance->rate;
                    }
                    instance->current_frame = 2.0f - instance->current_frame;
                } else {
                    const i32 repeat_count = instNuGCutSceneRepeatCount(instance);
                    if (repeat_count > 0) {
                        const f32 overflow = 1.0f - instance->current_frame;
                        instNuGCutSceneDecrementRepeatCount(instance, repeat_count);
                        instNuGCutSceneStart(instance);
                        instance->flags_88 |= 1;
                        instance->current_frame += overflow;
                    } else if (instance->chained_instance != NULL) {
                        const f32 overflow = 1.0f - instance->current_frame;
                        instNuGCutSceneEndFirstFrame(instance);
                        instNuGCutSceneStart(instance->chained_instance);
                        instance->chained_instance->current_frame += overflow;
                        instNuGCutSceneClipTest(instance->chained_instance);
                        instNuGCutSceneUpdate(instance->chained_instance, 0, skip, elapsed);
                        instance->chained_instance = NULL;
                        return;
                    } else {
                        instNuGCutSceneEndFirstFrame(instance);
                        return;
                    }
                }
            } else if ((instance->flags_88 & 0x20) != 0) {
                instance->current_frame = 1.0f;
            } else {
                instNuGCutSceneEndFirstFrame(instance);
            }
        } else if (instance->current_frame >= end_frame) {
            if (streaming) {
                const u8 last_stream = instNuGCutSceneLastStream(instance);
                if (instance->stream_index < last_stream) {
                    if ((instance->flags_8b & 8) != 0) {
                        f32 overflow = instance->current_frame - end_frame;
                        if (overflow > cutscene->duration) {
                            overflow = 0.0f;
                        }
                        instance->accumulated_stream_duration += cutscene->duration;
                        ++instance->stream_index;
                        if (instNuGCutSceneSwapBuffers(instance, 0) != 0) {
                            instNuGCutSceneCopyStreamAnimations(instance);
                            instance->flags_8b &= ~8U;
                            instNuGCutSceneStart(instance);
                            instance->flags_88 |= 1;
                            instance->current_frame += overflow;
                            cutscene = instance->cutscene;
                        } else {
                            --instance->stream_index;
                            instance->current_frame = end_frame;
                        }
                    } else {
                        instance->current_frame = end_frame;
                    }
                } else if ((instance->flags_8c & 0x40) != 0) {
                    instance->current_frame = end_frame;
                    instance->flags_89 |= 0x10;
                } else {
                    instNuGCutSceneEnd(instance);
                    instance->accumulated_stream_duration = 0.0f;
                }
            } else if ((instance->flags_88 & 0x10) != 0) {
                instance->current_frame -= end_frame;
                instance->current_frame = end_frame - instance->current_frame;
                if (instance->rate > 0.0f) {
                    instance->rate = -instance->rate;
                }
            } else if ((instance->flags_8c & 0x40) != 0) {
                instance->current_frame = end_frame;
                instance->flags_89 |= 0x10;
            } else {
                if (instance->end_callback != NULL) {
                    void (*callback)(instNUGCUTSCENE_s *) = instance->end_callback;
                    instance->end_callback = NULL;
                    callback(instance);
                }
                const i32 repeat_count = instNuGCutSceneRepeatCount(instance);
                if (repeat_count > 0) {
                    f32 overflow = instance->current_frame - end_frame;
                    if (overflow > cutscene->duration) {
                        overflow = 0.0f;
                    }
                    instNuGCutSceneDecrementRepeatCount(instance, repeat_count);
                    instNuGCutSceneStart(instance);
                    instance->flags_88 |= 1;
                    instance->current_frame += overflow;
                } else if (instance->chained_instance != NULL) {
                    f32 overflow = instance->current_frame - end_frame;
                    if (overflow > cutscene->duration) {
                        overflow = 0.0f;
                    }
                    instNuGCutSceneEnd(instance);
                    instNuGCutSceneStart(instance->chained_instance);
                    instance->chained_instance->current_frame += overflow;
                    instNuGCutSceneClipTest(instance->chained_instance);
                    instNuGCutSceneUpdate(instance->chained_instance, 0, skip, elapsed);
                    instance->chained_instance = NULL;
                    return;
                } else if ((instance->flags_88 & 8) != 0) {
                    f32 overflow = instance->current_frame - end_frame;
                    if (overflow > cutscene->duration) {
                        overflow = 0.0f;
                    }
                    instNuGCutSceneStart(instance);
                    instance->flags_88 |= 1;
                    instance->current_frame += overflow;
                } else {
                    instNuGCutSceneEnd(instance);
                }
            }
        }
    }
    if ((instance->flags_88 & 2) == 0) {
        return;
    }
    instance->render_frame = instance->current_frame;
    if ((instance->flags_8a & 4) == 0) {
        const f32 render_frame = instance->render_frame;
        if (instance->character_instance != NULL) {
            NUGCUTCHARSYS_s *system = cutscene->character_system;
            if (NuCutSceneCharacterProcess != NULL && system->character_count != 0) {
                for (i32 i = 0; i < system->character_count; ++i) {
                    instNUGCUTCHAR_s *character_instance = &instance->character_instance->characters[i];
                    if (character_instance->character_model != NULL) {
                        NuCutSceneCharacterProcess(instance, cutscene, character_instance, &system->characters[i],
                                                   render_frame, paused);
                    }
                }
            }
        }
        if (instance->camera_instance != NULL) {
            instNuGCutCamSysUpdate(instance, render_frame);
        }
        if (instance->trigger_instance != NULL) {
            instNuGCutTriggerSysUpdate(instance, render_frame);
        }
        if (instance->rigid_instance != NULL) {
            instNuGCutRigidSysUpdate(instance, render_frame, paused);
        }
    } else {
        const f32 render_frame = cutscene->duration - instance->render_frame;
        if (instance->character_instance != NULL) {
            NUGCUTCHARSYS_s *system = cutscene->character_system;
            if (NuCutSceneCharacterProcess != NULL && system->character_count != 0) {
                for (i32 i = 0; i < system->character_count; ++i) {
                    instNUGCUTCHAR_s *character_instance = &instance->character_instance->characters[i];
                    if (character_instance->character_model != NULL) {
                        NuCutSceneCharacterProcess(instance, cutscene, character_instance, &system->characters[i],
                                                   render_frame, paused);
                    }
                }
            }
        }
        if (instance->camera_instance != NULL) {
            instNuGCutCamSysUpdate(instance, render_frame);
        }
        if (instance->trigger_instance != NULL) {
            instNuGCutTriggerSysUpdate(instance, render_frame);
        }
        if (instance->rigid_instance != NULL) {
            instNuGCutRigidSysUpdate(instance, render_frame, paused);
        }
    }
}

static __used__ void instNuGCutCamSysUpdate(instNUGCUTSCENE_s *instance, float frame) {
    cutscenecam_focusDistance = 0.0f;
    cutscenecam_focalLength = 0.0f;
    cutscenecam_usefocusloc = 0;
    cutscenecam_fstop = 0.0f;

    NUGCUTCAMERASYS_s *system = instance->cutscene->camera_system;
    instNUGCUTSCENECAMERA_s *camera_instance = instance->camera_instance;
    if (system->focus_state_animation != NULL && instance->cutscene->version > 4) {
        u8 focus_index = 0xff;
        if (StateAnimEvaluate(system->focus_state_animation, &camera_instance->focus_state_index, &focus_index,
                              frame) != 0) {
            if (focus_index == 0xff) {
                camera_instance->focus_index = -1;
            } else {
                camera_instance->focus_index = static_cast<i8>(instance->cutscene->focus_camera_indices[focus_index]);
            }
        }
    }

    if (system->state_animation != NULL) {
        u8 camera_index = 0xff;
        if (StateAnimEvaluate(system->state_animation, &camera_instance->state_index, &camera_index, frame) != 0) {
            camera_instance->camera_index = static_cast<i8>(camera_index);
            instance->flags_8d |= 0x10;
            cutscenecamchange = 1;
        }
        const u8 state_index = camera_instance->state_index;
        if (state_index < system->state_animation->count) {
            const f32 next_state_frame = system->state_animation->times[state_index];
            if (next_state_frame - frame < 1.0f &&
                system->state_animation->values[state_index] != static_cast<u8>(camera_instance->camera_index)) {
                f32 render_frame = next_state_frame - 1.0f;
                if ((instance->flags_8a & 4) != 0) {
                    render_frame = instance->cutscene->duration - render_frame;
                }
                instance->render_frame = render_frame < 1.0f ? 1.0f : render_frame;
            }
        }
    }

    u8 target_index = camera_instance->next_target_index;
    while (target_index < camera_instance->target_count &&
           frame >= camera_instance->targets[target_index].start_frame) {
        const i8 mapped_camera = system->target_camera_map[camera_instance->targets[target_index].target_index];
        instNUGCUTCAMSTATE_s *state = &camera_instance->camera_states[mapped_camera];
        state->flags |= 2;
        state->event_index = target_index;
        camera_instance->next_target_index = ++target_index;
    }
    while (target_index != 0 && frame < camera_instance->targets[target_index - 1].start_frame) {
        camera_instance->next_target_index = --target_index;
        const i8 mapped_camera = system->target_camera_map[camera_instance->targets[target_index].target_index];
        instNUGCUTCAMSTATE_s *state = &camera_instance->camera_states[mapped_camera];
        state->flags |= 2;
        state->event_index = target_index;
    }

    i32 camera_index = camera_instance->camera_index;
    if (camera_index < 0) {
        CutSceneCameraCTRL = 0;
        return;
    }

    CameraDOFHack = 2;
    NUGCUTCAMERA_s *camera = &system->cameras[camera_index];
    instNUGCUTCAMSTATE_s *camera_state = &camera_instance->camera_states[camera_index];
    CutSceneCameraCTRL = 1;
    if ((camera->flags & 1) == 0 || system->animation == NULL ||
        NuAnimNumNodes(system->animation) <= camera->animation_node) {
        cutscenecammtx = camera->base_matrix;
    } else {
        const u32 focus_magic = system->focus_animation == NULL ? 0 : *reinterpret_cast<u32 *>(system->focus_animation);
        if (instance->cutscene->version > 4 && system->focus_animation != NULL &&
            focus_magic - ANI3_MAGIC_VERSION_4 < 2) {
            f32 *values =
                NuAnimCurveExtractAllNodeCurves_3(reinterpret_cast<ani3_animheader_s *>(system->focus_animation),
                                                  camera->animation_node, instance->render_frame, NULL);
            cutscenecam_fstop = values[2];
            cutscenecam_focalLength = values[0] * 1.3f;
            if ((camera->field_43 & 2) == 0) {
                if (system->focus_state_animation != NULL && camera_instance->focus_index >= 0) {
                    NUGCUTLOCATOR_s *focus_locator =
                        &instance->cutscene->locator_system->locators[camera_instance->focus_index];
                    if (focus_locator->animation != NULL) {
                        NUMTX focus_matrix;
                        NuGCutLocatorCalcMtx(focus_locator, frame, &focus_matrix, NULL);
                        if ((focus_locator->flags & 4) != 0) {
                            NuMtxPreTranslate(&focus_matrix, &focus_locator->pivot);
                        }
                        if (static_cast<i8>(instance->flags_88) < 0) {
                            NuMtxMul(&focus_matrix, &focus_matrix, &instance->matrix);
                        }
                        NuMtxGetTranslation(&focus_matrix, &cutscenecam_focusloc);
                        cutscenecam_usefocusloc = 1;
                    }
                }
            } else {
                cutscenecam_focusDistance = values[1];
            }
        }
        const u32 animation_magic = *reinterpret_cast<u32 *>(system->animation);
        if (animation_magic - ANI3_MAGIC_VERSION_4 < 2) {
            NuAnimCurve2SetApplyToMatrix_3(reinterpret_cast<ani3_animheader_s *>(system->animation),
                                           camera->animation_node, instance->render_frame, &cutscenecammtx);
        }
    }

    if (static_cast<i8>(instance->flags_88) < 0) {
        NuMtxMul(&cutscenecammtx, &cutscenecammtx, &instance->matrix);
    }
    set_cutscenecammtx = 1;
    if ((camera_state->flags & 2) == 0) {
        return;
    }

    instNUGCUTCAMTGT_s *target = &camera_instance->targets[camera_state->event_index];
    f32 duration = fabsf(target->duration);
    const bool reverse = NuFsign(target->duration) < 0.0f;
    if (reverse && duration + target->start_frame <= frame) {
        camera_state->flags &= ~2U;
        return;
    }

    const NUVEC translation = *NUMTX_GET_ROW_VEC(&cutscenecammtx, 3);
    NUMTX look_at_matrix = cutscenecammtx;
    NuMtxLookAtZ(&look_at_matrix, target->target);

    f32 blend;
    if (duration > 0.01f) {
        blend = (frame - target->start_frame) / duration;
        if (blend > 1.0f || (blend >= 0.0f && reverse)) {
            blend = 1.0f - blend;
        }
    } else {
        blend = reverse ? 0.0f : 1.0f;
    }

    NUQUAT look_at_rotation;
    NUQUAT camera_rotation;
    NUQUAT blended_rotation;
    NuMtxToQuat(&look_at_matrix, &look_at_rotation);
    NuMtxToQuat(&cutscenecammtx, &camera_rotation);
    NuQuatSlerp(&blended_rotation, &camera_rotation, &look_at_rotation, blend);
    NuQuatToMtx(&blended_rotation, &cutscenecammtx);
    *NUMTX_GET_ROW_VEC(&cutscenecammtx, 3) = translation;
}

static __used__ void instNuGCutSceneClipTest(instNUGCUTSCENE_s *instance) {
    instance->flags_89 |= 4;
    NUGCUTSCENE_s *cutscene = instance->cutscene;
    if (cutscene->bounds == NULL) {
        return;
    }
    if ((instance->flags_89 & 2) != 0 &&
        NuCameraDistSqr(&instance->transformed_bounds_center) > instance->max_camera_distance_squared) {
        instance->flags_89 &= ~4U;
        return;
    }
    if ((instance->flags_89 & 1) != 0) {
        NUVEC *bounds = static_cast<NUVEC *>(cutscene->bounds);
        NUMTX *matrix = static_cast<i8>(instance->flags_88) < 0 ? &instance->matrix : &numtx_identity;
        if (NuCameraClipTestExtents(&bounds[0], &bounds[1], matrix, 0.0f, 0) == 0) {
            instance->flags_89 &= ~4U;
        }
    }
}

static __used__ void instNuGCutRigidSysRender(instNUGCUTSCENE_s *instance, float frame, int paused) {
    NUGCUTRIGIDSYS_s *system = instance->cutscene->rigid_system;
    if (system == NULL || instance->rigid_instance == NULL) {
        return;
    }
    for (u32 i = 0; i < system->count; ++i) {
        NUGCUTRIGID_s *rigid = &system->rigids[i];
        if ((rigid->flags & 6) != 6) {
            continue;
        }
        instNUGCUTRIGID_s *inst_rigid = &instance->rigid_instance->rigids[i];
        u8 visible = inst_rigid->visible;
        if (rigid->state_animation != NULL &&
            StateAnimEvaluate(rigid->state_animation, &inst_rigid->state_index, &visible, frame) != 0) {
            inst_rigid->visible = visible != 0;
        }
        if (inst_rigid->visible == 0) {
            continue;
        }
        NUMTX matrix;
        NuGCutRigidCalcMtx(rigid, frame, &matrix);
        if ((instance->flags_88 & 0x80) != 0) {
            NuMtxMul(&matrix, &matrix, &instance->matrix);
        }
        if (instance->alpha == 1.0f) {
            NuSpecialDrawAt(inst_rigid, &matrix);
        } else {
            NuSpecialDrawAtAlpha(inst_rigid, &matrix, instance->alpha);
        }
        if (NuCutSceneRigidPostRender != NULL && (rigid->flags & 0x18) != 0) {
            NuCutSceneRigidPostRender(rigid, inst_rigid, &matrix);
        }
        if (rigid->locator_index != 0xff && rigid->locator_count != 0 && instance->cutscene->locator_system != NULL &&
            instance->locator_instance != NULL) {
            NUGCUTLOCATORSYS_s *locator_system = instance->cutscene->locator_system;
            for (u32 locator_offset = 0; locator_offset < rigid->locator_count; ++locator_offset) {
                u32 locator_index = rigid->locator_index + locator_offset;
                instNuGCutLocatorUpdate(instance, locator_system, &instance->locator_instance->locators[locator_index],
                                        &locator_system->locators[locator_index], frame, &matrix, paused);
            }
        }
    }
}

static __used__ void instNuGCutRigidSysUpdate(instNUGCUTSCENE_s *instance, float frame, int paused) {
    NUGCUTRIGIDSYS_s *system = instance->cutscene->rigid_system;
    if (system == NULL || instance->rigid_instance == NULL) {
        return;
    }

    for (u32 i = 0; i < system->count; ++i) {
        NUGCUTRIGID_s *rigid = &system->rigids[i];
        if ((rigid->flags & 4) == 0 || (rigid->flags & 2) != 0) {
            continue;
        }

        instNUGCUTRIGID_s *inst_rigid = &instance->rigid_instance->rigids[i];
        u8 visible;
        if (rigid->state_animation != NULL &&
            StateAnimEvaluate(rigid->state_animation, &inst_rigid->state_index, &visible, frame) != 0) {
            NuSpecialSetVisibility(inst_rigid, visible != 0);
        }

        if (NuSpecialGetVisibilityFn(inst_rigid) == 0) {
            continue;
        }

        NUMTX matrix;
        NuGCutRigidCalcMtx(rigid, frame, &matrix);
        if ((instance->flags_88 & 0x80) != 0) {
            NuMtxMul(&matrix, &matrix, &instance->matrix);
        }
        NuSpecialSetDrawMtx(inst_rigid, &matrix);

        if (rigid->locator_index != 0xff && rigid->locator_count != 0 && instance->cutscene->locator_system != NULL &&
            instance->locator_instance != NULL) {
            NUGCUTLOCATORSYS_s *locator_system = instance->cutscene->locator_system;
            for (u32 locator_offset = 0; locator_offset < rigid->locator_count; ++locator_offset) {
                u32 locator_index = rigid->locator_index + locator_offset;
                instNuGCutLocatorUpdate(instance, locator_system, &instance->locator_instance->locators[locator_index],
                                        &locator_system->locators[locator_index], frame, &matrix, paused);
            }
        }
    }
}

static __used__ void instNuGCutTriggerSysUpdate(instNUGCUTSCENE_s *instance, float frame) {
    instNUGCUTTRIGGERSYS_s *trigger_instance = instance->trigger_instance;
    NUGCUTTRIGGERSYS_s *system = instance->cutscene->trigger_system;
    for (i32 i = 0; i < system->event_count; ++i) {
        NUGCUTTRIGGEREVENT_s *event = &system->events[i];
        if (event->state_animation == NULL) {
            continue;
        }
        u8 value;
        if (StateAnimEvaluate(event->state_animation, reinterpret_cast<u8 *>(&trigger_instance->event_states[i]),
                              &value, frame) == 0) {
            continue;
        }
        u8 *trigger_owner = static_cast<u8 *>(trigger_instance->owner);
        u8 *trigger_states = *reinterpret_cast<u8 **>(trigger_owner + 0xc);
        const i32 trigger_index = static_cast<i16>(event->field_00);
        if (value != 0) {
            trigger_states[trigger_index * 4 + 2] |= 1;
        } else {
            trigger_states[trigger_index * 4 + 2] &= ~1U;
        }
    }
}

extern "C" void NuGCutSceneSysUpdate(i32 paused, i32 skip, f32 elapsed) {
    if (++cutscene_synchro_counter > 0xff) {
        cutscene_synchro_counter = 1;
    }
    cutscene_quit_prompt = 0;
    cutscene_has_quit = 0;
    cutscene_lock_player = 0;
    cutscenecamchange = 0;
    if (paused != 0) {
        skip = 0;
    }
    for (instNUGCUTSCENE_s *instance = active_cutscene_instances; instance != NULL; instance = instance->next) {
        const i8 instance_sync = static_cast<i8>(*reinterpret_cast<u16 *>(&instance->flags_8a) >> 3);
        if (instance_sync != cutscene_synchro_counter) {
            instNuGCutSceneUpdate(instance, paused, skip, elapsed);
        }
    }
}

extern "C" void NuGCutSceneSysRender(i32 paused) {
    for (instNUGCUTSCENE_s *instance = active_cutscene_instances; instance != NULL; instance = instance->next) {
        const f32 frame = (instance->flags_8a & 4) == 0 ? instance->render_frame
                                                        : instance->cutscene->duration - instance->render_frame;
        if ((instance->flags_89 & 8) == 0 && (instance->flags_88 & 2) != 0 && (instance->flags_89 & 4) != 0 &&
            instance->rigid_instance != NULL) {
            instNuGCutRigidSysRender(instance, frame, paused);
        }

        if ((instance->flags_89 & 8) == 0 && (instance->flags_88 & 2) != 0 && (instance->flags_89 & 4) != 0 &&
            instance->character_instance != NULL && instance->cutscene->character_system != NULL &&
            NuCutSceneCharacterRender != NULL) {
            NUGCUTCHARSYS_s *system = instance->cutscene->character_system;
            for (u32 i = 0; i < system->character_count; ++i) {
                instNUGCUTCHAR_s *inst_character = &instance->character_instance->characters[i];
                if (inst_character->character_model != NULL) {
                    NuCutSceneCharacterRender(instance, instance->cutscene, inst_character, &system->characters[i],
                                              frame, paused);
                }
            }
        }

        if ((instance->flags_89 & 8) != 0 || (instance->flags_88 & 2) == 0 || (instance->flags_89 & 4) == 0 ||
            instance->locator_instance == NULL || instance->cutscene->locator_system == NULL) {
            continue;
        }
        if ((instance->flags_8c & 0x40) != 0 && instance->current_frame == instance->cutscene->duration - 1.0f) {
            continue;
        }
        if ((instance->flags_8b & 0x40) != 0) {
            continue;
        }

        NUGCUTLOCATORSYS_s *system = instance->cutscene->locator_system;
        NUMTX *parent_matrix = static_cast<i8>(instance->flags_88) < 0 ? &instance->matrix : NULL;
        for (u32 i = 0; i < system->locator_count; ++i) {
            NUGCUTLOCATOR_s *locator = &system->locators[i];
            if ((locator->flags & 3) == 0) {
                instNuGCutLocatorUpdate(instance, system, &instance->locator_instance->locators[i], locator, frame,
                                        parent_matrix, paused);
            }
        }
    }
}

static __used__ void CutScene_OverrideConfigFileName_LSW(char *, int, int) {
}

void NewCopyAnims(instNUGCUTSCENE_s *instance) {
    NUGCUTSCENE_s *source = instance->cutscene_copy;
    NUGCUTSCENE_s *destination =
        (instance->flags_8b & 0x10) != 0 ? instance->stream_buffer_1 : instance->stream_buffer_0;
    instance->cutscene = destination;

    destination->filename = source->filename;
    destination->stream_buffer_0 = source->stream_buffer_0;
    destination->stream_buffer_1 = source->stream_buffer_1;
    reinterpret_cast<u8 *>(&destination->flags)[0] =
        (reinterpret_cast<u8 *>(&destination->flags)[0] & ~2U) | (reinterpret_cast<u8 *>(&source->flags)[0] & 2);

    NUGCUTLOCATORSYS_s *destination_locators = destination->locator_system;
    NUGCUTLOCATORSYS_s *source_locators = source->locator_system;
    if (destination_locators != NULL) {
        if (destination_locators->locator_count != 0) {
            for (i32 i = 0; i < destination_locators->locator_count; ++i) {
                nuanimdata2_s *animation = destination_locators->locators[i].animation;
                destination_locators->locators[i] = source_locators->locators[i];
                destination_locators->locators[i].animation = animation;
            }
        }
        destination_locators->types = source_locators->types;
    }

    NUGCUTRIGIDSYS_s *destination_rigids = destination->rigid_system;
    NUGCUTRIGIDSYS_s *source_rigids = source->rigid_system;
    if (destination_rigids != NULL && destination_rigids->rigids != NULL && destination_rigids->count != 0) {
        for (i32 i = 0; i < destination_rigids->count; ++i) {
            nuanimdata2_s *animation = destination_rigids->rigids[i].animation;
            StateAnim *state_animation = destination_rigids->rigids[i].state_animation;
            destination_rigids->rigids[i] = source_rigids->rigids[i];
            destination_rigids->rigids[i].animation = animation;
            destination_rigids->rigids[i].state_animation = state_animation;
            NUGCUTRIGID_s *rigid = &destination_rigids->rigids[i];
            if (rigid->locator != 0 && rigid->locator_count != 0 && rigid->locator_index != 0xff) {
                rigid->locator = reinterpret_cast<usize>(&destination_locators->locators[rigid->locator_index]);
            }
        }
    }

    NUGCUTCHARSYS_s *destination_characters = destination->character_system;
    NUGCUTCHARSYS_s *source_characters = source->character_system;
    if (destination_characters != NULL && destination_characters->character_count != 0) {
        for (i32 i = 0; i < destination_characters->character_count; ++i) {
            NUGCUTCHAR_s *character = &destination_characters->characters[i];
            nuanimdata2_s *animation = character->animation;
            nuanimdata2_s *face_animation = character->face_animation;
            nuanimdata2_s *extra_animation = character->extra_animation;
            NUGCUTLOCATOR_s *locator = character->locator;
            *character = source_characters->characters[i];
            character->animation = animation;
            character->face_animation = face_animation;
            character->extra_animation = extra_animation;
            if (locator != NULL && character->has_locator != 0 && character->locator_index != 0xff) {
                character->locator = &destination_locators->locators[character->locator_index];
            }
        }
    }

    if (destination->trigger_system != NULL) {
        NUGCUTTRIGGERSYS_s *destination_triggers = destination->trigger_system;
        NUGCUTTRIGGERSYS_s *source_triggers = source->trigger_system;
        for (i32 i = 0; i < destination_triggers->event_count; ++i) {
            destination_triggers->events[i].field_00 = source_triggers->events[i].field_00;
            destination_triggers->events[i].field_04 = source_triggers->events[i].field_04;
        }
    }
}

static __used__ void copyAnims(NUGCUTSCENE_s *destination, NUGCUTSCENE_s *source) {
    NUGCUTCAMERASYS_s *destination_camera = destination->camera_system;
    NUGCUTCAMERASYS_s *source_camera = source->camera_system;
    if (destination->version > 4) {
        destination_camera->focus_animation = source_camera->focus_animation;
        destination_camera->focus_state_animation = source_camera->focus_state_animation;
    }
    if (source_camera->animation != NULL) {
        destination_camera->animation = source_camera->animation;
    }
    destination_camera->state_animation = source_camera->state_animation;

    NUGCUTLOCATORSYS_s *source_locators = source->locator_system;
    if (source_locators != NULL && source_locators->locators != NULL && source_locators->locator_count != 0) {
        NUGCUTLOCATORSYS_s *destination_locators = destination->locator_system;
        for (i32 i = 0; i < source_locators->locator_count; ++i) {
            if (source_locators->locators[i].animation != NULL) {
                destination_locators->locators[i].animation = source_locators->locators[i].animation;
            }
        }
        destination_locators->types = source_locators->types;
    }

    NUGCUTRIGIDSYS_s *source_rigids = source->rigid_system;
    if (source_rigids != NULL && source_rigids->rigids != NULL && source_rigids->count != 0) {
        NUGCUTRIGIDSYS_s *destination_rigids = destination->rigid_system;
        for (i32 i = 0; i < source_rigids->count; ++i) {
            if (source_rigids->rigids[i].animation != NULL) {
                destination_rigids->rigids[i].animation = source_rigids->rigids[i].animation;
            }
            destination_rigids->rigids[i].state_animation = source_rigids->rigids[i].state_animation;
        }
    }

    NUGCUTCHARSYS_s *source_characters = source->character_system;
    if (source_characters != NULL && source_characters->characters != NULL && source_characters->character_count != 0) {
        NUGCUTCHARSYS_s *destination_characters = destination->character_system;
        for (i32 i = 0; i < source_characters->character_count; ++i) {
            if (source_characters->characters[i].animation != NULL) {
                destination_characters->characters[i].animation = source_characters->characters[i].animation;
            }
            if (source_characters->characters[i].face_animation != NULL) {
                destination_characters->characters[i].face_animation = source_characters->characters[i].face_animation;
            }
            if (source_characters->characters[i].extra_animation != NULL) {
                destination_characters->characters[i].extra_animation =
                    source_characters->characters[i].extra_animation;
            }
        }
    }

    if (source->trigger_system != NULL) {
        NUGCUTTRIGGERSYS_s *source_triggers = source->trigger_system;
        if (source_triggers->events != NULL && source_triggers->event_count > 0) {
            NUGCUTTRIGGERSYS_s *destination_triggers = destination->trigger_system;
            for (i32 i = 0; i < source_triggers->event_count; ++i) {
                destination_triggers->events[i].state_animation = source_triggers->events[i].state_animation;
            }
        }
    }
    destination->duration = source->duration;
}
