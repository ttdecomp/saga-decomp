#ifndef LEGOAPI_RENDER_FX_H
#define LEGOAPI_RENDER_FX_H

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/numath/nuvec.h"

// Debris / particle / flight-spline helpers (module legoapi/render/fx).

struct GAMEDEBRISENTRY_s {
    i32 effect;
    char name[16];
};

struct APIDEBRISSYS_s {
    i32 named_count;
    i32 capacity;
    GAMEDEBRISENTRY_s *entries;
};

#ifdef __cplusplus
extern "C" {
#endif
    i32 FindGameDebris(APIDEBRISSYS_s *debris_sys, char *name);
    i32 AddGameDebris(APIDEBRISSYS_s *debris_sys, i32 type, NUVEC *pos);
    i32 AddGameDebrisMom(APIDEBRISSYS_s *debris_sys, i32 type, NUVEC *pos, i32 count, NUVEC *momentum);
    i32 AddGameDebrisMomentum(APIDEBRISSYS_s *system, i32 type, NUVEC *position, NUVEC *emitter_momentum,
                              NUVEC *particle_momentum);
    i32 AddGameDebrisXYZ(APIDEBRISSYS_s *system, i32 type, f32 x, f32 y, f32 z);
    i32 AddGameDebrisRot(APIDEBRISSYS_s *system, i32 type, NUVEC *position, i32 count, i16 z_rotation, i16 y_rotation);
    i32 AddGameDebrisMtx(APIDEBRISSYS_s *system, i32 type, NUVEC *position, i32 count, NUMTX *matrix);
    APIDEBRISSYS_s *InitGameDebris(VARIPTR *cursor, VARIPTR end, i32 count, i32 named_count, char **names, char page);
    i32 PARTLookupType(char *name);
    i32 LookupDebrisEffect(char *name);
    i32 LookupDebrisEffectPage(char *name, char page);
    i32 LookupDebrisEffectPageOnly(char *name, char page);
    void DebrisTypeStatusAlwaysOff(i32 type);
    void DebrisTypeStatusAlwaysOn(i32 type);
    void DebrisTypeStatusNormal(i32 type);
    void KillPart(PART_s *part, i32 reason);
    i32 ParticlesPerFrame(f32 particles_per_frame, f32 frame_time);
    i32 ParticlesPerSecond(f32 particles_per_second, f32 frame_time);
    i32 AddFiniteShotPART(i32 part_type, NUVEC *pos, i32 count);
    void AddFiniteShotDebrisEffect(i32 *handle, i32 effect, NUVEC *position, i32 count);
    void AddVariableShotDebrisEffectTimed1(i32 effect, NUVEC *position, i32 count, f32 time, i16 z_rotation,
                                           i16 y_rotation, NUMTX *orientation);
#ifdef __cplusplus
}
#endif
void FlightSpline_Init(WORLDINFO_s *world, flightspline_s *spline, i32 unknown);

#endif
