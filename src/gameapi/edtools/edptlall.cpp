#include "gameapi_edtools_types.h"
#include "gameapi/edtools/edstubs.h"
#include "gameapi/edtools/edui.h"
#include "legoapi/legoapi_types.h"

extern "C" {
    extern edpp_particle_s edpp_ptls[512];
    extern i32 edpp_nearest;
    extern debkeydatatype_s *debkeydata;
    extern debinftype **debtab;
    extern i32 debris_render_group;
    i32 edpp_dpad_mode;
    f32 edpp_scale_factor = 1.0f;
    i32 edpp_create_type = -1;
    i32 edptl_clipboard_entry = -1;
    u8 edpp_effect_list;
    i32 edpp_num_orphans;
    i32 edptl_repeatboxxzlock = 1;
    eduimenu_s *edptl_switchtype_menu;
    eduimenu_s *edptl_page_menu;
    eduimenu_s *edptl_star_menu;
    eduimenu_s *edptl_soundid_menu;
    void DebFreeInstantly(i32 *handle);
    void DebReAlloc(debkeydatatype_s *key, i32 particle_count);
    void DebrisSetDetailLevels(i32 handle, i32 detail_levels);
    void edppDeleteEffect(i32 index);
    void edppRestartAllEffectsInLevel(void);
}

void edppStartSingleEffect(i32 index);
void edppPtlDestroy(i32 index);

static edui_slider_s *repeatbox_x_item;
static edui_slider_s *repeatbox_z_item;

static void UpdateTotalPtls(debinftype *effect) {
    f32 elapsed_time = 0.0f;
    f32 active_time = 0.0f;
    while (effect->particle_lifetime > elapsed_time) {
        f32 remaining_time = effect->particle_lifetime - elapsed_time;
        f32 emission_time = effect->emission_period_random + effect->emission_pause;
        if (remaining_time < emission_time) {
            active_time += remaining_time;
            elapsed_time += remaining_time;
        } else {
            active_time += emission_time;
            elapsed_time += emission_time;
        }

        remaining_time = effect->particle_lifetime - elapsed_time;
        if (remaining_time < effect->emission_pause_random) {
            elapsed_time += remaining_time;
        } else {
            elapsed_time += effect->emission_pause_random;
        }
    }

    i16 particle_count = static_cast<i16>(static_cast<i32>(static_cast<f32>(effect->frequency) *
                                                           (active_time / elapsed_time) * effect->particle_lifetime));
    if (particle_count < 1) {
        particle_count = 1;
    }
    effect->max_particles = static_cast<i16>(particle_count * (effect->trail_count + 1));

    for (i32 i = 0; i < 512; ++i) {
        i32 instance_id = edpp_ptls[i].instance_id;
        if (instance_id == 99999) {
            continue;
        }
        if (instance_id == -1) {
            continue;
        }

        debkeydatatype_s *key = &debkeydata[instance_id];
        if (debtab[key->effect_index] == effect) {
            DebReAlloc(key, effect->max_particles);
        }
    }
}

// Particle list editor subsystem stubs (static, internal linkage).

static __used__ void edptlcbPageMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSetGroup(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    if (edpp_ptls[edpp_nearest].instance_id == -1) {
        return;
    }

    i16 render_group = static_cast<i16>(static_cast<i32>(static_cast<edui_slider_s *>(item)->value));
    edpp_ptls[edpp_nearest].render_group = render_group;
    debkeydata[edpp_ptls[edpp_nearest].instance_id].render_group = render_group;
}
static __used__ void edptlcbStarMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbStopPage(eduimenu_s *, eduiitem_s *item, u32) {
    edppStopPage(static_cast<i8>(item->data));
}
static __used__ void edptlcbClearPage(eduimenu_s *, eduiitem_s *item, u32) {
    edppClearPage(static_cast<i8>(item->data));
}
static __used__ void edptlcbGhostMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbGroupMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSetDetail(eduimenu_s *, eduiitem_s *item, u32) {
    i32 nearest = edpp_nearest;
    if (nearest == 0) {
        return;
    }

    i8 detail_levels;
    if (item->highlighted) {
        detail_levels = static_cast<i8>(item->data | edpp_ptls[nearest].detail_levels);
    } else {
        detail_levels = static_cast<i8>(~item->data & edpp_ptls[nearest].detail_levels);
    }
    edpp_ptls[nearest].detail_levels = detail_levels;

    i32 instance_id = edpp_ptls[nearest].instance_id;
    if (instance_id == -1) {
        return;
    }
    if (instance_id == 99999) {
        return;
    }
    DebrisSetDetailLevels(instance_id, detail_levels);
}
static __used__ void edptlcbStartPage(eduimenu_s *, eduiitem_s *item, u32) {
    edppStartPage(static_cast<i8>(item->data));
}
static __used__ void edptlcbBounceMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbDetailMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSetMaxThin(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->thinning = static_cast<edui_slider_s *>(item)->value;
}
static __used__ void edptlcbSetSoundID(eduimenu_s *menu, eduiitem_s *item, u32) {
    edptl_soundid_menu = NULL;

    u32 data = static_cast<u32>(item->data);
    i32 sound_id = static_cast<u16>(data);
    if (sound_id == 9999) {
        sound_id = -1;
    }
    i32 sound_index = data >> 16;

    if (edpp_nearest != -1) {
        i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
        if (instance_id != -1) {
            debinftype *effect = debtab[debkeydata[instance_id].effect_index];
            effect->sound_data[sound_index * 3] = sound_id;
        }
    }

    for (i32 i = 0; i < 512; ++i) {
        i32 instance_id = edpp_ptls[i].instance_id;
        if (instance_id == 99999 || instance_id == -1) {
            continue;
        }

        debkeydatatype_s *key = &debkeydata[instance_id];
        debinftype *effect = debtab[key->effect_index];
        key->process_collision_sound = 0;
        if (effect->sound_data[0] != -1) {
            key->process_collision_sound = 1;
        }
        if (effect->sound_data[3] != -1) {
            key->process_collision_sound = 1;
        }
        if (effect->sound_data[6] != -1) {
            key->process_collision_sound = 1;
        }
        if (effect->sound_data[9] != -1) {
            key->process_collision_sound = 1;
        }
    }

    eduiMenuDetach(menu);
    eduiMenuDestroy(menu);
}
static __used__ void edptlcbSoundXMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSoundsMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSwitchMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSetDpadMode(eduimenu_s *, eduiitem_s *item, u32) {
    edpp_dpad_mode = item->data;
}
static __used__ void edptlcbSetDrawflag(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    if (edpp_ptls[edpp_nearest].instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[edpp_ptls[edpp_nearest].instance_id].effect_index];
    if (effect->time_group == item->data) {
        return;
    }

    DebFreeInstantly(&edpp_ptls[edpp_nearest].instance_id);
    effect->time_group = static_cast<i8>(item->data);
    edppStartSingleEffect(edpp_nearest);
}
static __used__ void edptlcbSetSwitchId(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    if (edpp_ptls[edpp_nearest].instance_id == -1) {
        return;
    }

    i32 switch_id = static_cast<i32>(static_cast<edui_slider_s *>(item)->value);
    edpp_ptls[edpp_nearest].switch_id = switch_id;
    debkeydata[edpp_ptls[edpp_nearest].instance_id].trigger_second = switch_id;
}
static __used__ void edptlcbSoundIDMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbCutClipboard(eduimenu_s *menu, eduiitem_s *, u32) {
    debtab[edpp_create_type]->category = 4;
    edptl_clipboard_entry = edpp_create_type;

    eduimenu_s *parent = menu->parent;
    if (parent != NULL) {
        eduiMenuDetach(menu);
    }
    if (menu->callback != NULL) {
        menu->callback(menu, parent);
    }
}
static __used__ void edptlcbDpadModeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbDrawflagMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbSetSwitchVar(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    if (edpp_ptls[edpp_nearest].instance_id == -1) {
        return;
    }

    f32 switch_variable = static_cast<edui_slider_s *>(item)->value;
    edpp_ptls[edpp_nearest].switch_variable = switch_variable;
    debkeydata[edpp_ptls[edpp_nearest].instance_id].switch_variable = switch_variable;
}
static __used__ void edptlChangeRepeatBox(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }
    if (edpp_ptls[edpp_nearest].instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[edpp_ptls[edpp_nearest].instance_id].effect_index];
    edui_slider_s *slider = static_cast<edui_slider_s *>(item);
    if (item->data == 0) {
        f32 value = slider->value;
        effect->repeat_box.x = value;
        if (edptl_repeatboxxzlock != 0 && repeatbox_z_item != NULL) {
            effect->repeat_box.z = value;
            repeatbox_z_item->value = value;
            repeatbox_z_item->normalized_value = (value - repeatbox_z_item->minimum) / repeatbox_z_item->range;
        }
    } else if (item->data == 1) {
        effect->repeat_box.y = slider->value;
    } else if (item->data == 2) {
        f32 value = slider->value;
        effect->repeat_box.z = value;
        if (edptl_repeatboxxzlock != 0 && repeatbox_x_item != NULL) {
            effect->repeat_box.x = value;
            repeatbox_x_item->value = value;
            repeatbox_x_item->normalized_value = (value - repeatbox_x_item->minimum) / repeatbox_x_item->range;
        }
    }
}
static __used__ void edptlcbClipboardMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbDeleteOrphans(eduimenu_s *menu, eduiitem_s *, u32) {
    for (i32 i = 0; i < 512; ++i) {
        if (edpp_ptls[i].effect_index == -1) {
            edppPtlDestroy(i);
        }
    }

    edpp_num_orphans = 0;
    eduimenu_s *parent = menu->parent;
    if (parent != NULL) {
        eduiMenuDetach(menu);
    }
    if (menu->callback != NULL) {
        menu->callback(menu, parent);
    }
}
static __used__ void edptlcbSetSwitchType(eduimenu_s *menu, eduiitem_s *item, u32) {
    if (edpp_nearest != -1 && edpp_ptls[edpp_nearest].instance_id != -1) {
        i32 switch_type = item->data;
        edpp_ptls[edpp_nearest].switch_type = switch_type;
        debkeydata[edpp_ptls[edpp_nearest].instance_id].trigger_first = switch_type;
    }

    eduiMenuDetach(menu);
    eduiMenuDestroy(menu);
    edptl_switchtype_menu = NULL;
}
static __used__ void edptlcbApplyGhostTime(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->trail_time = static_cast<edui_slider_s *>(item)->value;
}

static __used__ void edptlcbApplyNumGhosts(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->trail_count = static_cast<u8>(static_cast<i32>(static_cast<edui_slider_s *>(item)->value));
    UpdateTotalPtls(effect);
}
static __used__ void cbPtlChangeIvalOffRan(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->start_offset_random = static_cast<edui_slider_s *>(item)->value;
    UpdateTotalPtls(effect);
}
static __used__ void cbPtlChangeIvalOff(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->emission_pause_random = static_cast<edui_slider_s *>(item)->value;
    UpdateTotalPtls(effect);
}
static __used__ void cbPtlChangeIvalOnRan(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->emission_pause = static_cast<edui_slider_s *>(item)->value;
    UpdateTotalPtls(effect);
}
static __used__ void cbPtlChangeIvalOn(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->emission_period_random = static_cast<edui_slider_s *>(item)->value;
    UpdateTotalPtls(effect);
}
static __used__ void cbPtlChangeETime(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->particle_lifetime = static_cast<edui_slider_s *>(item)->value;
    UpdateTotalPtls(effect);
}
static __used__ void cbPtlChangeGenRate(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->frequency = static_cast<i16>(static_cast<i32>(static_cast<edui_slider_s *>(item)->value));
    UpdateTotalPtls(effect);
}
static __used__ void edptlcbApplyStarRatio(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->radial_floor = static_cast<edui_slider_s *>(item)->value;
}
static __used__ void edptlcbCancelPageMenu(eduimenu_s *, eduimenu_s *) {
    eduiMenuDestroy(edptl_page_menu);
    edptl_page_menu = NULL;
}
static __used__ void edptlcbCancelStarMenu(eduimenu_s *, eduimenu_s *) {
    eduiMenuDestroy(edptl_star_menu);
    edptl_star_menu = NULL;
}
static __used__ void edptlcbChangeDistortX(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->field_140 = static_cast<edui_slider_s *>(item)->value;
}
static __used__ void edptlcbChangeDistortY(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->field_144 = static_cast<edui_slider_s *>(item)->value;
}
static __used__ void edptlcbChangeRampTime(eduimenu_s *, eduiitem_s *item, u32) {
    if (edpp_nearest == -1) {
        return;
    }

    i32 instance_id = edpp_ptls[edpp_nearest].instance_id;
    if (instance_id == -1) {
        return;
    }

    debinftype *effect = debtab[debkeydata[instance_id].effect_index];
    effect->scale_in_time = static_cast<edui_slider_s *>(item)->value;
}
static __used__ void edptlcbEmptyClipboard(eduimenu_s *menu, eduiitem_s *, u32) {
    i32 create_type = edpp_create_type;
    edpp_create_type = edptl_clipboard_entry;
    edppDeleteEffect(edpp_create_type);
    edpp_create_type = -1;

    eduimenu_s *parent = menu->parent;
    if (parent != NULL) {
        eduiMenuDetach(menu);
    }
    if (menu->callback != NULL) {
        menu->callback(menu, parent);
    }

    edptl_clipboard_entry = -1;
    edpp_create_type = create_type;
}
static __used__ void edptlcbOrphanListMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbPasteClipboard(eduimenu_s *menu, eduiitem_s *, u32) {
    debinftype *effect = debtab[edptl_clipboard_entry];
    edptl_clipboard_entry = -1;
    effect->category = edpp_effect_list;

    eduimenu_s *parent = menu->parent;
    if (parent != NULL) {
        eduiMenuDetach(menu);
    }
    if (menu->callback != NULL) {
        menu->callback(menu, parent);
    }
}
static __used__ void edptlcbResetParticles(eduimenu_s *, eduiitem_s *, u32) {
    for (i32 i = 0; i < 512; ++i) {
        i32 *instance_id = &edpp_ptls[i].instance_id;
        if (*instance_id != 99999 && *instance_id != -1) {
            DebFreeInstantly(instance_id);
            *instance_id = 99999;
        }
    }
    edppRestartAllEffectsInLevel();
}
static __used__ void edptlcbSetMasterGroup(eduimenu_s *, eduiitem_s *item, u32) {
    debris_render_group = static_cast<i32>(static_cast<edui_slider_s *>(item)->value);
}
static __used__ void edptlcbSetScaleFactor(eduimenu_s *, eduiitem_s *item, u32) {
    edpp_scale_factor = static_cast<edui_slider_s *>(item)->value;
}
static __used__ void edptlcbSwitchTypeMenu(eduimenu_s *, eduiitem_s *, u32) {
}
static __used__ void edptlcbTestDetailMenu(eduimenu_s *, eduiitem_s *, u32) {
}
