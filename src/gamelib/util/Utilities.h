#pragma once

#include "nu2api/nucore/common.h"

#ifdef __cplusplus
extern "C" {
#endif

u32 UtilGetTime(void);
u32 UtilGetFrameStartTime(void);
void UtilFrameStart(void);

#ifdef __cplusplus
}
#endif
