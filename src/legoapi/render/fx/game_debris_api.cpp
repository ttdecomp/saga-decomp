// Game-facing debris API: original 0x3ca25e..0x3ca9ea uses unoptimized x86 code.
// Keep these wrappers separate from the optimized particle engine.
#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "legoapi/render/fx.h"
#include "nu2api/nu3d/nutex.h"
#include "nu2api/nu3d/numtl.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/common.h"
#include "nu2api/numath/nurand.h"
#include "nu2api/numath/nuvec.h"
#include "nu2api/numath/numtx.h"
#include "nu2api/numath/nutrig.h"
#include "nu2api/numath/nufloat.h"

#include <string.h>
#include <stdlib.h>

extern "C" {
    void AddFiniteShotDebrisEffect(i32 *, i32, NUVEC *, i32);
    void AddFiniteShotDebrisEffect2(i32 *, i32, NUVEC *, NUVEC *, NUVEC *, i32);
    void AddVariableShotDebrisEffect(i32, NUVEC *, i32, i16, i16);
    void AddVariableShotDebrisEffectMtx3(i32, NUVEC *, NUVEC *, i32, NUMTX *, NUMTX *);
    extern i32 edpp_types_used;
    extern debinftype **debtab;
    i32 LookupDebrisEffectPage(char *, char);
    i32 LookupDebrisEffectPageOnly(char *, char);
    i32 AddGameDebris(APIDEBRISSYS_s *system, i32 type, NUVEC *position) {
        if (type >= 0 && type < system->capacity && system->entries[type].effect != -1) {
            i32 handle = -1;
            AddFiniteShotDebrisEffect(&handle, system->entries[type].effect, position, 1);
            return 1;
        }
        return 0;
    }

    i32 AddGameDebrisMomentum(APIDEBRISSYS_s *system, i32 type, NUVEC *position, NUVEC *emitter_momentum,
                              NUVEC *particle_momentum) {
        if (type >= 0 && type < system->capacity && system->entries[type].effect != -1) {
            i32 handle = -1;
            AddFiniteShotDebrisEffect2(&handle, system->entries[type].effect, position, emitter_momentum,
                                       particle_momentum, 1);
            return 1;
        }
        return 0;
    }

    i32 AddGameDebrisXYZ(APIDEBRISSYS_s *system, i32 type, f32 x, f32 y, f32 z) {
        if (type >= 0 && type < system->capacity && system->entries[type].effect != -1) {
            i32 handle = -1;
            NUVEC position = {x, y, z};
            AddFiniteShotDebrisEffect(&handle, system->entries[type].effect, &position, 1);
            return 1;
        }
        return 0;
    }

    i32 AddGameDebrisRot(APIDEBRISSYS_s *system, i32 type, NUVEC *position, i32 count, i16 z_rotation, i16 y_rotation) {
        if (type >= 0 && type < system->capacity && system->entries[type].effect != -1 && count > 0) {
            AddVariableShotDebrisEffect(system->entries[type].effect, position, count, z_rotation, y_rotation);
            return 1;
        }
        return 0;
    }

    i32 AddGameDebrisMtx(APIDEBRISSYS_s *system, i32 type, NUVEC *position, i32 count, NUMTX *matrix) {
        if (type >= 0 && type < system->capacity && system->entries[type].effect != -1 && count > 0) {
            NUMTX_ALIGNED16 orientation;
            NuMtxSetRotationX(&orientation, 0x4000);
            NuMtxMulR(&orientation, &orientation, matrix);
            AddVariableShotDebrisEffectMtx3(system->entries[type].effect, position, &nuvec_zero, count, &orientation,
                                            &numtx_identity);
            return 1;
        }
        return 0;
    }

    i32 AddGameDebrisMom(APIDEBRISSYS_s *system, i32 type, NUVEC *position, i32 count, NUVEC *momentum) {
        if (type >= 0 && type < system->capacity && system->entries[type].effect != -1 && count > 0) {
            NUVEC zero = {0.0f, 0.0f, 0.0f};
            if (momentum == NULL)
                momentum = &zero;
            AddVariableShotDebrisEffectMtx3(system->entries[type].effect, position, momentum, count, NULL, NULL);
            return 1;
        }
        return 0;
    }

    i32 FindGameDebris(APIDEBRISSYS_s *debris_sys, char *name) {
        for (i32 index = debris_sys->named_count; index < debris_sys->capacity; ++index) {
            if (NuStrICmp(name, debris_sys->entries[index].name) == 0) {
                return index;
            }
        }
        return -1;
    }

    APIDEBRISSYS_s *InitGameDebris(VARIPTR *cursor, VARIPTR end, i32 count, i32 flags, char **names, char page) {
        (void)end;
        if (cursor->addr == 0) {
            return NULL;
        }

        APIDEBRISSYS_s *sys = BUFFER_ALLOC_T(cursor, APIDEBRISSYS_s);
        sys->named_count = flags;
        sys->capacity = count;
        sys->entries = BUFFER_ALLOC_ARRAY(cursor, count, GAMEDEBRISENTRY_s);

        memset(sys->entries, 0xff, static_cast<usize>(count) * sizeof(*sys->entries));

        // Seed the named entries from the debris_name table.
        for (i32 i = 0; i < sys->named_count; i++) {
            GAMEDEBRISENTRY_s &entry = sys->entries[i];
            NuStrCpy(entry.name, names[i]);
            entry.effect = LookupDebrisEffectPage(entry.name, page);
        }

        // The original appends the currently registered page effects after
        // the fixed debris_name set.  effecttypes[0] is reserved, and the
        // pointer table is append-only while pages are loaded.
        i32 i = sys->named_count;
        for (i32 j = 1; i < sys->capacity && j < edpp_types_used; j++) {
            debinftype *effect = debtab != NULL ? debtab[j] : NULL;
            if (effect == NULL) {
                break;
            }
            GAMEDEBRISENTRY_s &entry = sys->entries[i];
            NuStrCpy(entry.name, effect->name);
            entry.effect = LookupDebrisEffectPageOnly(entry.name, page);
            i++;
        }

        for (; i < sys->capacity; i++) {
            sys->entries[i].effect = -1;
        }

        return sys;
    }
}
