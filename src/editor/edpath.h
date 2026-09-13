#pragma once

#include "editor/aieditor_settings.h"
#include "editor/aieditor_state.h"
#include "editor/edpath_types.h"
#include "gameapi/ai/aisys/aisys.h"

struct nupad_s;
struct eduimenu_s;

typedef void AIEDITORPATHNODECALLBACK(EDAIPATHNODE_s *node);
extern "C" {
    extern AIEDITORPATHNODECALLBACK *AIPathNodeDeletedFn;
    void InitFn_AIPathNodeDeleted(AIEDITORPATHNODECALLBACK *function);
}

eduimenu_s *pathEditor_Process(nupad_s *pad);
