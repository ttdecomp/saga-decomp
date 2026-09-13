// Post-effect + framebuffer state — see nuposteffect_plain.cpp.
#pragma once

#include "decomp.h"
#include "nu2api/nu3d/nupostparams.h"

struct nuframebuffer_s;
struct nueffecttex_s;

extern i32 g_effectFlags;

extern "C" bool NuPostEffectIsInitialised(u32 mask);
extern "C" void NuPostEffectReset(void);
extern "C" void NuPostEffectEnd(void);
extern "C" void NuPostEffectAddDynamicLight(void *light);
extern "C" void NuPostEffectInit(u32 flags, void *buffer, void *buffer_end);
extern "C" void NuPostEffectDestroy();
extern "C" void NuPostEffectRender(nuframebuffer_s *output);
extern "C" void NuPostEffectEnable(u32 mask);
extern "C" void NuPostEffectDisable(u32 mask);
extern "C" bool NuPostEffectIsEnabled(u32 mask);
extern "C" void NuPostEffectBloom(const NuBloomParameters *);
extern "C" void NuPostEffectDepthOfField(const NuDepthOfFieldParameters *);
extern "C" void NuPostEffectDeferredShading(const f32 *);
extern "C" void NuPostEffectMotionBlur(const NUMTX *, const NUMTX *, f32, f32, f32);
extern "C" void NuPostEffectAccumulationMotionBlur(i32, f32, i32);
extern "C" void NuPostEffectSpeedBlur(const NuSpeedBlurParameters *);
extern "C" i32 NuPostEffectGetActiveDynamicLightCount();
extern "C" nueffecttex_s *NuPostEffectGetBackBuffer(i32 frame);
extern "C" nueffecttex_s *NuPostEffectGetDepthBuffer(i32 frame);
extern "C" f32 NuPostEffectTiming(i32 *last_frame);

extern "C" void NuFramebufferClear(u32 clear_flags, u32 colour);
extern "C" void NuFramebufferSwapBuffers(void);
