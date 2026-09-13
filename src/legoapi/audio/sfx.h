#ifndef LEGOAPI_AUDIO_SFX_H
#define LEGOAPI_AUDIO_SFX_H

#include "decomp.h"
#include "legoapi/legoapi_types.h"
#include "nu2api/numath/nuvec.h"

// Audio / SFX playback API (module legoapi/audio, sfx.cpp). PlaySfx is a
// C-linkage symbol in the original (unmangled); TickTockSfx is C++.

#ifdef __cplusplus
extern "C" {
#endif
    void PlaySfx(char *name, nuvec_s *pos);
    i32 IsSfxLooping(i32 sfx_id);
#ifdef __cplusplus
}
#endif

void TickTockSfx(void);
void GameAudio_PlaySfxById(i32 sfx_id, NUVEC *position, i32 flags, i32 volume);
void AddLevelSfxFromId(i32 sfx_id, i32 *sfx_ids, i32 *sfx_count, i32 max_sfx);

#endif
