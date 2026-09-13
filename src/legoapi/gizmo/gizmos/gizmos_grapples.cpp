#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/gizmos/transport/grapples.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nuvec.h"

void Grapple_DrawLine(GameObject_s *) {
}

void Grapple_MoveCode(GameObject_s *) {
}

i32 Grapple_LookAtPos(GameObject_s *, nuvec_s *) {
    return 0;
}

void Grapple_ReachedTop(GameObject_s *) {
}

void Grapple_FindNearest(WORLDINFO_s *, nuvec_s *, GameObject_s *, float *) {
}

void Grapple_FindNearestToPos(WORLDINFO_s *, nuvec_s *) {
}

// Static grapple list helpers. Moved from gizmisc_stubs.cpp.

static __used__ void Grapple_FindNearestInList(nuvec_s *, GRAPPLE_s *, int, GameObject_s *, GRAPPLE_s **, float *) {
}
