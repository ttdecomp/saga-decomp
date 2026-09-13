#include "nu2api/nu3d/nuprim.h"
#include "nu2api/nu3d/nuprim_internal.h"
#include "nu2api/nu3d/nudlist.h"
#include "nu2api/nu3d/nurndrstat.h"
#include <string.h>

static u16 *g_NuPrim_VertexCountPtr;
static u16 g_NuPrim_CurrentPrimType = 10000;

extern "C" {
static void NuPrimPushCoordSystem(NUPRIMSCALEMODE scale_mode) {
    NuPrimCSPos++;
    NuPrimSetCoordinateSystem(scale_mode);
}
}

extern "C" {
static NUDISPLAYLIST *NuDisplayListGet2dList(void) {
    return &global_dlist_manager.dlist_2d;
}
}

static void NuDisplayListSetNext(NUDISPLAYLISTITEM *item, void *next) {
    item->next = next;
}

static void NuDisplayListSetID_CALL(NUDISPLAYLISTITEM *item) {
    item->id = 3;
}

static NUDISPLAYLISTITEM *NuDisplayListAddItem(NUDISPLAYLIST *list, u8 type, void *next) {
    list->items->type = type;
    NuDisplayListSetID_CALL(list->items);
    NuDisplayListSetNext(list->items, next);
    list->items = reinterpret_cast<NUDISPLAYLISTITEM *>(reinterpret_cast<u8 *>(list->items) + 0x10);
    return reinterpret_cast<NUDISPLAYLISTITEM *>(reinterpret_cast<u8 *>(list->items) - 0x10);
}

struct PrimStreamHeader {
    u32 prim_type;
    u32 pad0;
    u16 pad1;
    u16 vertex_count;
    u32 pad2;
};
static_assert(sizeof(PrimStreamHeader) == 0x10, "PrimStreamHeader must be 0x10");

extern "C" void NuPrim2DBegin(u32 prim_type, u32, NUMTL *mtl) {
    if (mtl == nullptr) {
        mtl = numtl_defaultmtl2d;
    }

    g_NuPrim_NeedsOverbrightening = mtl->tex_id != 0;
    g_NuPrim_NeedsHalfUVs = mtl->shader_desc.vtx_desc.has_half_uvs;
    VARIPTR *buf = NuDisplayListGetBuffer();

    NUDISPLAYLIST *list;
    if (mtl->display_list != nullptr) {
        list = mtl->display_list;
        NUDLDLISTSCENE *scene = list->dlist;
        scene->flags |= NUDL_SCENE_FLAG_CLIP_MATERIALS;
        u8 *used = scene->mtl_used[(scene->render_buffer >> 7) & 1];
        used[list->mtl_id >> 3] |= static_cast<u8>(1 << (list->mtl_id & 7));
    } else {
        list = NuDisplayListGet2dList();
        NuDisplayListLinkMtl(list, mtl);
    }

    RndrStateSetConstAlphaTint(0, 0, 0.0f, nullptr, nullptr);
    DisplayListUpdateRenderState(list, &render_state);
    NuDisplayListLinkItems(list, 1);
    g_NuPrim_StreamBufferPtr = buf;

    PrimStreamHeader *header = reinterpret_cast<PrimStreamHeader *>(buf->addr);
    header->prim_type = prim_type;
    header->vertex_count = 0;
    buf->addr += sizeof(PrimStreamHeader);
    g_NuPrim_VertexCountPtr = &header->vertex_count;
    g_NuPrim_CurrentPrimType = static_cast<u16>(prim_type);
    g_NuPrim_VertexCount = 0;
    NuDisplayListAddItem(list, 0x93, header);
}

extern "C" void NuPrim3DBegin(u32 prim_type, u32, NUMTL *mtl, NUMTX *world_mtx) {
    if (mtl == nullptr) {
        mtl = numtl_defaultmtl3d;
    }

    g_NuPrim_NeedsOverbrightening = mtl->tex_id != 0;
    g_NuPrim_NeedsHalfUVs = mtl->shader_desc.vtx_desc.has_half_uvs;

    VARIPTR *buf = NuDisplayListGetBuffer();
    NUDISPLAYLIST *list;
    if (mtl->display_list != nullptr) {
        list = mtl->display_list;
        NUDLDLISTSCENE *scene = list->dlist;
        scene->flags |= NUDL_SCENE_FLAG_CLIP_MATERIALS;
        u8 *used = scene->mtl_used[(scene->render_buffer >> 7) & 1];
        used[list->mtl_id >> 3] |= static_cast<u8>(1 << (list->mtl_id & 7));
    } else {
        list = numtl_defaultmtl3d->display_list;
    }

    DisplayListUpdateRenderState(list, &render_state);
    NuDisplayListLinkItems(list, 2);
    g_NuPrim_StreamBufferPtr = buf;

    NUMTX *transform = static_cast<NUMTX *>(DisplayListCreateGeomTransformPS(
        buf, world_mtx != nullptr ? world_mtx : &numtx_identity, nullptr, nullptr, nullptr));

    NUDISPLAYLISTGEOM *geometry = reinterpret_cast<NUDISPLAYLISTGEOM *>(buf->addr);
    geometry->primitive_type = static_cast<i32>(prim_type);
    geometry->vertex_count = 0;
    buf->addr += sizeof(NUDISPLAYLISTGEOM);

    g_NuPrim_VertexCountPtr = reinterpret_cast<u16 *>(&geometry->vertex_count);
    g_NuPrim_CurrentPrimType = static_cast<u16>(prim_type);

    NuDisplayListAddItem(list, 0x8c, transform);
    NuDisplayListAddItem(list, 0x82, geometry);
}

extern "C" void NuPrim2DEnd(void) {
    *g_NuPrim_VertexCountPtr = static_cast<u16>(g_NuPrim_VertexCount);
    g_NuPrim_VertexCount = 0;
}

extern "C" void NuPrim3DEnd(void) {
    *g_NuPrim_VertexCountPtr = static_cast<u16>(g_NuPrim_VertexCount);
    g_NuPrim_VertexCount = 0;
}

extern "C" void NuPrim2DAddXYZ(float x, float y, float z) {
    PrimVertexRaw *vtx = (PrimVertexRaw *)g_NuPrim_StreamBufferPtr->addr;
    vtx->x = NuPrim_XBias + NuPrim_XScale * x;
    vtx->y = NuPrim_YBias + NuPrim_YScale * y;
    vtx->z = z;
    g_NuPrim_StreamBufferPtr->addr += sizeof(PrimVertexRaw);
    g_NuPrim_VertexCount++;

    if (g_NuPrim_CurrentPrimType == 4 && (g_NuPrim_VertexCount & 1) == 0) {
        u32 *words = (u32 *)(usize)(g_NuPrim_StreamBufferPtr->addr - 0x30);
        g_NuPrim_StreamBufferPtr->addr += 0x60;
        g_NuPrim_VertexCount += 4;

        memcpy(&words[12], &words[6], 0x18);
        memcpy(&words[18], &words[12], 0x18);
        words[24] = words[0];
        words[25] = words[7];
        words[26] = words[8];
        words[27] = words[9];
        words[28] = words[4];
        words[29] = words[11];
        words[6] = words[12];
        words[7] = words[1];
        words[8] = words[2];
        words[9] = words[3];
        words[10] = words[16];
        words[11] = words[5];
        memcpy(&words[30], &words[0], 0x18);
    }
}

extern "C" void NuPrimInit(VARIPTR *, VARIPTR) {
    NuPrimCSPos = -1;
    NuPrimPushCoordSystem(NUPRIM_SCALEMODE_PS2);
}

extern "C" void NuPrimSetCoordinateSystem(NUPRIMSCALEMODE scale_mode) {
    NuPrimCoordSystemStack[NuPrimCSPos] = scale_mode;

    switch (scale_mode) {
        case NUPRIM_SCALEMODE_PS2:
            NuPrim_XScale = 0.003125f;
            NuPrim_YScale = -0.008928572f;
            NuPrim_XBias = -1.0f;
            NuPrim_YBias = 1.0f;
            break;
        case NUPRIM_SCALEMODE_NORMALISED:
            NuPrim_XScale = 1.0f;
            NuPrim_YScale = -1.0f;
            NuPrim_XBias = 0.0f;
            NuPrim_YBias = 0.0f;
            break;
        case NUPRIM_SCALEMODE_ABSOLUTE:
            NuPrim_XScale = 2.0f;
            NuPrim_YScale = -2.0f;
            NuPrim_XBias = -1.0f;
            NuPrim_YBias = 1.0f;
            break;
    }
}
