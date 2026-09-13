#include "legoapi/characters/core/customiser.h"

#include "globals.h"
#include "legoapi/items/objects/gameobjects.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/world/area.h"
#include "legoapi/world/world.h"
#include "nu2api/nu3d/nuspecial.h"

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
