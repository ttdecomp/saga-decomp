#pragma once

#include "nu2api/nucore/common.h"
#include "decomp.h"

struct nueffecttex_s {
    i16 unknown_00;
    i16 width, height;
    u8 unknown_06[14];
};
DECOMP_ASSERT(sizeof(nueffecttex_s) == 20, "nueffecttex_s size");
struct nuframebuffer_s;

struct NuProxyBuffer {
    nueffecttex_s *texture;
    i32 kind;
    bool enabled, resolved;
    u8 padding[2];
};
DECOMP_ASSERT(sizeof(NuProxyBuffer) == 12, "NuProxyBuffer size");

extern "C" {
    nuframebuffer_s *NuFramebufferCreate();
    void NuFramebufferDestroy(nuframebuffer_s *);
    void NuFramebufferAttachTex2D(nuframebuffer_s *, i32, nueffecttex_s *, i32);
    void NuFramebufferBind(nuframebuffer_s *);
    nuframebuffer_s *NuFramebufferGetBound();
    nuframebuffer_s *NuFramebufferGetFrontBuffer();
    nuframebuffer_s *NuFramebufferGetObject(i32);
    nueffecttex_s *NuFramebufferGetAttachedTex(nuframebuffer_s *, i32, i32 *, i32 *);
    i32 NuFramebufferGetWidth(nuframebuffer_s *);
    i32 NuFramebufferGetHeight(nuframebuffer_s *);
    void NuFramebufferResolve(i32, bool);
    void NuFramebufferResolveAll(bool);
    void NuFramebufferResolveMultisample(i32);
    void NuFramebufferEnableGuards(nuframebuffer_s *, bool);
    void NuFramebufferCopyTex2D(i32, nueffecttex_s *, i32, i32, i32, i32, i32);
    nueffecttex_s *NuEffectTexCreate2D(i32, i32, i32, i32, i32);
    void NuEffectTexGetDimension(nueffecttex_s *, i32, i32 *, i32 *);
    nueffecttex_s *NuEffectTexGetEffectFromNative(i32 tex_id);
    void NuEffectTexLockVP(void *, void *);
    void NuEffectTexUnlockVP();
    void NuEffectTexMapNative(nueffecttex_s *texture);
    void NuRenderContextSetViewport(i32, i32, i32, i32);
}

inline void NuPostResolve(NuProxyBuffer *proxy) {
    if (!proxy->resolved) {
        NuFramebufferResolve(proxy->kind, proxy->enabled);
        proxy->resolved = true;
    }
}
