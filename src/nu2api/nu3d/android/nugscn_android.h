#include "nu2api/nucore/common.h"
#include "nu2api/nufile/nufile.h"

#include <GLES2/gl2.h>

i32 NuGScnUploadGfxDataFromFilePS(VARIPTR *buf, VARIPTR buf_end, i32 file);
extern u32 g_lastBoundVAO;

#ifdef __cplusplus
extern "C" {
#endif
    extern i32 g_vaoLifetimeMutex;
    // Placeholder only: the original consumes three stack arguments; their types remain unresolved.
    void NuGSceneSetCrossFade(void);
    void NuGSceneSetCrossFadeAlpha(void);
    void NuGSceneProcessCrossFade(void);
#ifdef __cplusplus
}
#endif
