#include <stdio.h>
#include "legoapi/items/objects/gameobjects.h"
#include "nu2api/nu3d/nuspecial.h"
#include "legoapi/world/world.h"
#include "legoapi/core/input/qrand.h"
#include "nu2api/numath/nutrig.h"
extern i16 tSTRANGER;
#include "legoapi/menus/screens/gamemenuall.h"
#include "legoapi/legoapi_types.h"
#include "globals.h"
#include "batman.h"
#include "legoapi/items/base/animpacket.h"
#include "legoapi/characters/core/character.h"

f32 CustomiseMenuTime[2];
GAMESAVE_s OldCustomiseGame = {};
i32 customiser_save_done = 0;
i32 customiser_quit = 0;
i32 customiser_changed = 0;
extern "C" void ResetAnimPacket(void *, i32);

struct CUSTOMISEMOTION_s {
    f32 field_00, field_04, delay;
    i16 angles[3];
    u8 reserved_12[2];
};
DECOMP_ASSERT(sizeof(CUSTOMISEMOTION_s) == 0x14, "Customise motion size");
static i32 CustomiseMode[2];
static u16 CustomiseYRot[2];
static CUSTOMISEMOTION_s HeadAnim[2], ArmsAnim[2], LegsAnim[2];
static u16 CustomiseBob[2];
static f32 Customise_NameAlpha;
static f32 CustomiseNameBoardTMul[2], CustomiseNameBoardMul[2], CustomiseNameLetterBlipScale[2];
u16 CustomiseRotY[2], CustomiseTiltX[2], CustomiseTiltZ[2];
extern NUVEC CustomisePos[2];
i32 Customiser_NextPieceRight(CUSTOMISER *, i32, i32, i32, i32);
void Customiser_SetNameAndIcon(CUSTOMISER *, i32);

void Customiser_Init(CUSTOMISER *customiser) {
    if (customiser == NULL)
        return;
    for (i32 i = 0; i < 2; i++) {
        ResetAnimPacket(&customiser->animation_packets[i], -1);
        customiser->animation_values[i] = 0;
        customiser->animation_active[i] = 0;
        customiser->animation_state[i] = 2;
        CustomiseMode[i] = 2;
    }
    if (WORLD->camera_splines[18] != NULL) {
        {

            NUVEC *points = WORLD->camera_splines[18]->pts;
            CustomisePos[0] = points[0 * 2];
            CustomiseYRot[0] =
                NuAtan2D(points[0 * 2 + 1].x - CustomisePos[0].x, points[0 * 2 + 1].z - CustomisePos[0].z);
            HeadAnim[0].angles[2] = 0;
            HeadAnim[0].delay = ((static_cast<f32>(qrand()) * 1.5259021893143654e-05f) * 0.5f) * 0.5f;
            HeadAnim[0].angles[0] = 0;
            HeadAnim[0].angles[1] = 0;
            HeadAnim[0].field_04 = 0.0f;
            HeadAnim[0].field_00 = 0.0f;
            ArmsAnim[0].angles[2] = 0;
            ArmsAnim[0].delay = ((static_cast<f32>(qrand()) * 1.5259021893143654e-05f) * 0.5f) * 0.5f;
            ArmsAnim[0].angles[0] = 0;
            ArmsAnim[0].angles[1] = 0;
            ArmsAnim[0].field_04 = 0.0f;
            ArmsAnim[0].field_00 = 0.0f;
            LegsAnim[0].angles[2] = 0;
            LegsAnim[0].delay = ((static_cast<f32>(qrand()) * 1.5259021893143654e-05f) * 0.5f) * 0.5f;
            LegsAnim[0].angles[0] = 0;
            LegsAnim[0].angles[1] = 0;
            LegsAnim[0].field_04 = 0.0f;
            LegsAnim[0].field_00 = 0.0f;
        }
        {

            NUVEC *points = WORLD->camera_splines[18]->pts;
            CustomisePos[1] = points[1 * 2];
            CustomiseYRot[1] =
                NuAtan2D(points[1 * 2 + 1].x - CustomisePos[1].x, points[1 * 2 + 1].z - CustomisePos[1].z);
            HeadAnim[1].angles[2] = 0;
            HeadAnim[1].delay = ((static_cast<f32>(qrand()) * 1.5259021893143654e-05f) * 0.5f) * 0.5f;
            HeadAnim[1].angles[0] = 0;
            HeadAnim[1].angles[1] = 0;
            HeadAnim[1].field_04 = 0.0f;
            HeadAnim[1].field_00 = 0.0f;
            ArmsAnim[1].angles[2] = 0;
            ArmsAnim[1].delay = ((static_cast<f32>(qrand()) * 1.5259021893143654e-05f) * 0.5f) * 0.5f;
            ArmsAnim[1].angles[0] = 0;
            ArmsAnim[1].angles[1] = 0;
            ArmsAnim[1].field_04 = 0.0f;
            ArmsAnim[1].field_00 = 0.0f;
            LegsAnim[1].angles[2] = 0;
            LegsAnim[1].delay = ((static_cast<f32>(qrand()) * 1.5259021893143654e-05f) * 0.5f) * 0.5f;
            LegsAnim[1].angles[0] = 0;
            LegsAnim[1].angles[1] = 0;
            LegsAnim[1].field_04 = 0.0f;
            LegsAnim[1].field_00 = 0.0f;
        }

        CustomiseRotY[0] = qrand();
        CustomiseRotY[1] = CustomiseRotY[0] + 0x4000;
        CustomiseTiltX[0] = qrand();
        CustomiseTiltX[1] = CustomiseTiltX[0] + 0x8000;
        CustomiseTiltZ[0] = qrand();
        CustomiseTiltZ[1] = CustomiseTiltZ[0] + 0x8000;
        CustomiseBob[0] = qrand();
        CustomiseBob[1] = CustomiseBob[0] + 0x8000;
    }
    Customiser_SetNameAndIcon(customiser, -1);
    Customise_NameAlpha = 0.0f;
    if (customiser->piece_counts[0] > 0)
        Game.customizer.pieces[0] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[0]) - 1, customiser->piece_counts[0], 0, 0);
    if (customiser->piece_counts[1] > 0)
        Game.customizer.pieces[1] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[1]) - 1, customiser->piece_counts[1], 0, 1);
    if (customiser->piece_counts[2] > 0)
        Game.customizer.pieces[2] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[2]) - 1, customiser->piece_counts[2], 0, 2);
    if (customiser->piece_counts[3] > 0)
        Game.customizer.pieces[3] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[3]) - 1, customiser->piece_counts[3], 0, 3);
    if (customiser->piece_counts[4] > 0)
        Game.customizer.pieces[4] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[4]) - 1, customiser->piece_counts[4], 0, 4);
    if (customiser->piece_counts[5] > 0)
        Game.customizer.pieces[5] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[5]) - 1, customiser->piece_counts[5], 0, 5);
    if (customiser->piece_counts[6] > 0)
        Game.customizer.pieces[6] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[6]) - 1, customiser->piece_counts[6], 0, 6);
    if (customiser->piece_counts[7] > 0)
        Game.customizer.pieces[7] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[7]) - 1, customiser->piece_counts[7], 0, 7);
    if (customiser->piece_counts[8] > 0)
        Game.customizer.pieces[8] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.pieces[8]) - 1, customiser->piece_counts[8], 0, 8);
    if (customiser->piece_counts[0] > 0)
        Game.customizer.secondary_pieces[0] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[0]) - 1, customiser->piece_counts[0], 1, 0);
    if (customiser->piece_counts[1] > 0)
        Game.customizer.secondary_pieces[1] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[1]) - 1, customiser->piece_counts[1], 1, 1);
    if (customiser->piece_counts[2] > 0)
        Game.customizer.secondary_pieces[2] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[2]) - 1, customiser->piece_counts[2], 1, 2);
    if (customiser->piece_counts[3] > 0)
        Game.customizer.secondary_pieces[3] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[3]) - 1, customiser->piece_counts[3], 1, 3);
    if (customiser->piece_counts[4] > 0)
        Game.customizer.secondary_pieces[4] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[4]) - 1, customiser->piece_counts[4], 1, 4);
    if (customiser->piece_counts[5] > 0)
        Game.customizer.secondary_pieces[5] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[5]) - 1, customiser->piece_counts[5], 1, 5);
    if (customiser->piece_counts[6] > 0)
        Game.customizer.secondary_pieces[6] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[6]) - 1, customiser->piece_counts[6], 1, 6);
    if (customiser->piece_counts[7] > 0)
        Game.customizer.secondary_pieces[7] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[7]) - 1, customiser->piece_counts[7], 1, 7);
    if (customiser->piece_counts[8] > 0)
        Game.customizer.secondary_pieces[8] = Customiser_NextPieceRight(
            customiser, static_cast<u16>(Game.customizer.secondary_pieces[8]) - 1, customiser->piece_counts[8], 1, 8);
    CustomiseNameBoardTMul[1] = 0.0f;
    CustomiseNameBoardMul[0] = 0.0f;
    CustomiseNameLetterBlipScale[1] = 1.0f;
    CustomiseNameLetterBlipScale[0] = 1.0f;
}

void Customiser_Reset(CUSTOMISER *customiser) {
    if (customiser == NULL) {
        return;
    }

    CustomiseMenuTime[0] = 0.0f;
    ResetAnimPacket(&customiser->animation_packets[0], -1);
    customiser->animation_values[0] = 0;
    customiser->animation_active[0] = 0;
    customiser->animation_state[0] = 2;

    CustomiseMenuTime[1] = 0.0f;
    ResetAnimPacket(&customiser->animation_packets[1], -1);
    customiser->animation_values[1] = 0;
    customiser->animation_active[1] = 0;
    customiser->animation_state[1] = 2;
}

void Customiser_Draw3D(CUSTOMISER *) {
}

void Customiser_Update(CUSTOMISER *, WORLDINFO_s *) {
}

void CustomiserMenu_End() {
}

void Customiser_DumpAll(CUSTOMISER *, WORLDINFO_s *) {
}

i32 Customiser_GetIcon(CUSTOMISER *customiser, CUSTOMISESAVE_s *save, i32) {
    CUSTOMPIECE *first = &customiser->piece_sets[0][static_cast<u16>(save->pieces[0])];
    CUSTOMPIECE *second = &customiser->piece_sets[1][static_cast<u16>(save->pieces[1])];
    i32 id;
    if (first->layer_flags & 0x20)
        goto second_piece;
    if (second->layer_flags & 1)
        goto second_piece;
    id = first->icon_character_id;
    if (id != -1)
        goto resolved_icon;
    id = first->character_id;
    if (id != -1)
        goto resolved_icon;
second_piece:
    id = second->icon_character_id;
    if (id == -1)
        id = second->character_id;
resolved_icon:
    if (id != -1)
        return CDataList[id].field20_0x42;
    return LEGOOBJ_ICON_WEIRDO;
}

void Customiser_LoadAll(CUSTOMISER *, WORLDINFO_s *) {
}

void CustomiserMenu_Draw(MENU_s *) {
}

void Customiser_Configure(char *, variptr_u *, variptr_u *, i32, i32, i32 (*)(CUSTOMPIECE *),
                          void (*)(CUSTOMPIECE *, nufpar_s *), i32 (*)(char *), CUSTOMISESAVE_s *, i16 *) {
}

void Customiser_InitNames(CUSTOMISER *) {
}

void CustomiserMenu_Update(MENU_s *) {
}

void Customiser_PieceConfig(CUSTOMPIECE *, nufpar_s *) {
}

i32 Customiser_MenuAvailable(CUSTOMISER *customiser) {
    if (customiser == NULL || APICharacterLoaded(customiser->character_ids[0]) == NULL) {
        return 0;
    }
    const i16 first_animation = customiser->animation_packets[0].requested_animation;
    if (first_animation != 99 && first_animation != 190) {
        return 0;
    }
    if (APICharacterLoaded(customiser->character_ids[1]) == NULL) {
        return 0;
    }
    const i16 second_animation = customiser->animation_packets[1].requested_animation;
    return second_animation == 99 || second_animation == 190;
}

struct CUSTOMPIECERESOURCE {
    u32 field_0;
    u8 special_data[0x10];
    void *model;
    u8 tail[8];
};
DECOMP_ASSERT(sizeof(CUSTOMPIECERESOURCE) == 0x20, "CUSTOMPIECERESOURCE size");
i32 Customiser_NextPieceLeft(CUSTOMISER *customiser, i32 index, i32 count, i32 unused, i32 category) {
    i32 found = 0;
    i32 attempts = 0;
    if (customiser) {
        while (attempts < count && !found) {
            --index;
            if (index < 0)
                index += count;
            i32 available = 1;
            if (category != 2) {
                WORLDINFO_s *world = WorldInfo_CurrentlyActive();
                if (world && HUB_ADATA && world->area == HUB_ADATA) {
                    CUSTOMPIECERESOURCE *resources = world->customiser_resources[category];
                    if (resources) {
                        CUSTOMPIECERESOURCE *resource = &resources[index];
                        if (customiser->categories[category]->uses_special)
                            available = NuSpecialExistsFn(resource->special_data) != 0;
                        else
                            available = resource->model != NULL;
                    } else
                        available = 0;
                }
            }
            if (available) {
                if (!(customiser->piece_sets[category][index].availability_flags & 0x180) ||
                    Game_100PercentComplete()) {
                    if (customiser->piece_available(&customiser->piece_sets[category][index]))
                        found = 1;
                    else
                        ++attempts;
                } else
                    ++attempts;
            } else
                ++attempts;
        }
    }
    return index;
}
i32 Customiser_NextPieceRight(CUSTOMISER *customiser, i32 index, i32 count, i32 unused, i32 category) {
    i32 found = 0;
    i32 attempts = 0;
    if (customiser) {
        while (attempts < count && !found) {
            ++index;
            if (index >= count)
                index -= count;
            i32 available = 1;
            if (category != 2) {
                WORLDINFO_s *world = WorldInfo_CurrentlyActive();
                if (world && HUB_ADATA && world->area == HUB_ADATA) {
                    CUSTOMPIECERESOURCE *resources = world->customiser_resources[category];
                    if (resources) {
                        CUSTOMPIECERESOURCE *resource = &resources[index];
                        if (customiser->categories[category]->uses_special)
                            available = NuSpecialExistsFn(resource->special_data) != 0;
                        else
                            available = resource->model != NULL;
                    } else
                        available = 0;
                }
            }
            if (available) {
                if (!(customiser->piece_sets[category][index].availability_flags & 0x180) ||
                    Game_100PercentComplete()) {
                    if (customiser->piece_available(&customiser->piece_sets[category][index]))
                        found = 1;
                    else
                        ++attempts;
                } else
                    ++attempts;
            } else
                ++attempts;
        }
    }
    return index;
}

void Customiser_PieceAvailable(CUSTOMPIECE *) {
}

void Customiser_SetAnimsToLoad(CUSTOMISER *, i32) {
}

void Customiser_SetNameAndIcon(CUSTOMISER *customiser, i32 index) {
    if (customiser == NULL)
        return;
    bool single = index != -1;
    if (index == 0 || !single) {
        customiser->display_names[0][0] = 0;
        sprintf(customiser->display_names[0], "%s %i", TTab[tSTRANGER], 1);
        CDataList[customiser->character_ids[0]].field20_0x42 = Customiser_GetIcon(customiser, &Game.customizer, 0);
    }
    if (index == 1 || !single) {
        customiser->display_names[1][0] = 0;
        sprintf(customiser->display_names[1], "%s %i", TTab[tSTRANGER], 2);
        CDataList[customiser->character_ids[1]].field20_0x42 =
            Customiser_GetIcon(customiser, reinterpret_cast<CUSTOMISESAVE_s *>(Game.customizer.secondary_pieces), 1);
    }
}

void Customiser_DrawAccessories(CUSTOMISER *, GameObject_s *, numtx_s *) {
}

void Customiser_DumpAccessories(CUSTOMISER *) {
}

void Customiser_FindPieceByName(CUSTOMISER *, char *, i32 *, i32 *) {
}

void Customiser_LoadAccessories(CUSTOMISER *, APICHARACTERMODELLIST_s *) {
}

void Customiser_TransformToPanel(CUSTOMISER *) {
}

void Customiser_AddPartAccessories(CUSTOMISER *, GameObject_s *, i32, i32, float) {
}

void Customiser_SetUpCharacterData(CUSTOMISER *) {
}

void Customiser_SaveModelTextureIDs(CUSTOMISER *, CHARACTERMODEL_s *) {
}

void Customiser_Set100PercentPieces(CUSTOMISER *) {
}

void Customiser_GetActiveWeirdoIndex(i32 *index, i32 *count) {
    if (MenuPacket.active_player[0] != 0) {
        if (MenuPacket.active_player[1] == 0) {
            *index = 0;
            *count = 1;
            return;
        }
    } else if (MenuPacket.active_player[1] != 0) {
        *index = 1;
        *count = 1;
        return;
    }
    *index = 0;
    *count = 2;
}

void Customiser_ResetModelTextureIDs(CUSTOMISER *customiser) {
    if (customiser == NULL) {
        return;
    }

    customiser->model_texture_ids[0] = 0;
    customiser->model_texture_ids[1] = 0;
    customiser->model_texture_ids[2] = 0;
    customiser->model_texture_ids[3] = 0;
    customiser->model_texture_ids[4] = 0;
    customiser->model_texture_ids[5] = 0;
    customiser->model_texture_ids[6] = 0;
    customiser->model_texture_ids[7] = 0;
    customiser->model_texture_ids[8] = 0;
    customiser->model_texture_ids[9] = 0;
    customiser->model_texture_ids[10] = 0;
    customiser->model_texture_ids[11] = 0;
    customiser->model_texture_ids[12] = 0;
    customiser->model_texture_ids[13] = 0;
    customiser->model_texture_ids[14] = 0;
    customiser->model_texture_ids[15] = 0;
    customiser->model_texture_ids[16] = 0;
    customiser->model_texture_ids[17] = 0;
}

void Customiser_RestoreModelTextureIDs(CUSTOMISER *) {
}

void Customiser_CopyDefaultPiecesToSave(CUSTOMISER *customiser, CUSTOMISESAVE_s *save) {
    if (customiser == NULL) {
        return;
    }
    if (save == NULL) {
        save = customiser->save;
        if (save == NULL) {
            return;
        }
    }
    save->pieces[0] = customiser->default_pieces[0][0];
    save->pieces[1] = customiser->default_pieces[0][1];
    save->pieces[2] = customiser->default_pieces[0][2];
    save->pieces[3] = customiser->default_pieces[0][3];
    save->pieces[4] = customiser->default_pieces[0][4];
    save->pieces[5] = customiser->default_pieces[0][5];
    save->pieces[6] = customiser->default_pieces[0][6];
    save->pieces[7] = customiser->default_pieces[0][7];
    save->pieces[8] = customiser->default_pieces[0][8];
    save->secondary_pieces[0] = customiser->default_pieces[1][0];
    save->secondary_pieces[1] = customiser->default_pieces[1][1];
    save->secondary_pieces[2] = customiser->default_pieces[1][2];
    save->secondary_pieces[3] = customiser->default_pieces[1][3];
    save->secondary_pieces[4] = customiser->default_pieces[1][4];
    save->secondary_pieces[5] = customiser->default_pieces[1][5];
    save->secondary_pieces[6] = customiser->default_pieces[1][6];
    save->secondary_pieces[7] = customiser->default_pieces[1][7];
    save->secondary_pieces[8] = customiser->default_pieces[1][8];
}

static __used__ bool Customiser_PieceAvailable_Default(CUSTOMPIECE *) {
    return {};
}

void Customise_GetToggleString(i32) {
}
