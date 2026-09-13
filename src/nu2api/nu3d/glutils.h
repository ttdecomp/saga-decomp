#pragma once

#include "nu2api/nu3d/numtl.h"
#include "nu2api/nu3d/nutex.h"

NUMTL *CreateAlphaBlendTexture(VARIPTR *buffer, VARIPTR buffer_end, char *name, i32 disable_depth_write, i32 alpha_mode,
                               i32 sort_priority, i32 depth_mode);
NUMTL *CreateSubtractiveTexture(char *name);
