#ifndef LEGOAPI_AI_CORE_AI_SYS_STUBS_H
#define LEGOAPI_AI_CORE_AI_SYS_STUBS_H

#include "decomp.h"

// AI world-query helpers (module legoapi/ai/core, ai_sys_stubs.cpp). These are
// C-linkage symbols (unmangled). AIPAthFindPathCnx is intentionally not here:
// episodeI and episodeII call it with different arities (each byte-matched), so
// it is declared locally in those files.

#ifdef __cplusplus
extern "C" {
#endif

    struct AISYS_s;

    struct AILOCATOR_s;
    struct AIAREA_s;
    struct AIPATH_s;
    struct AIPATHNODE_s;

    void *AISysBufferAlloc(VARIPTR *cursor, VARIPTR *buf_end, u32 size);

    AILOCATOR_s *AIPathFindLocator(AISYS_s *aisys, char *name);
    AIAREA_s *AISysFindArea(AISYS_s *ai_sys, char *name);
    AIPATH_s *AISysFindPath(AISYS_s *ai_sys, char *name);
    AIPATHNODE_s *AIPathFindNode(AISYS_s *aisys, AIPATH_s *path, char *name);
    void AIPathNodeUpdatePos(AISYS_s *system, AIPATH_s *path, AIPATHNODE_s *node);

#ifdef __cplusplus
}
#endif

#endif
