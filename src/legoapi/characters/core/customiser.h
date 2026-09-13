#pragma once

#include "decomp.h"

struct CUSTOMISER;
struct CUSTOMISESAVE_s;

i32 Customiser_NextPieceLeft(CUSTOMISER *customiser, i32 index, i32 count, i32 unused, i32 category);
i32 Customiser_NextPieceRight(CUSTOMISER *customiser, i32 index, i32 count, i32 unused, i32 category);
void Customiser_ResetModelTextureIDs(CUSTOMISER *customiser);
void Customiser_CopyDefaultPiecesToSave(CUSTOMISER *customiser, CUSTOMISESAVE_s *save);
