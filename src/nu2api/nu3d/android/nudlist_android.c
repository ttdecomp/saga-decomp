#include "nu2api/nu3d/nudlist.h"

static void NuDisplayListSetNext(nudisplaylistitem_s *item, void *next) {
    item->next = next;
}

static void NuDisplayListSetID_CALL(nudisplaylistitem_s *item) {
    item->id = 3;
}

static void NuDisplayListSetID_CNT(nudisplaylistitem_s *item) {
    item->id = 0;
}

static void NuDisplayListSetID_NEXT(nudisplaylistitem_s *item) {
    item->id = 1;
}

static void NuDisplayListSetID_RET(nudisplaylistitem_s *item) {
    item->id = 4;
}

static void NuDisplayListSetID(nudisplaylistitem_s *item, u8 id) {
    switch (id) {
    case 0: NuDisplayListSetID_CNT(item); break;
    case 1: NuDisplayListSetID_NEXT(item); break;
    case 3: NuDisplayListSetID_CALL(item); break;
    case 4: NuDisplayListSetID_RET(item); break;
    }
}

static void NuDisplayListSetItem(nudisplaylistitem_s *item, u8 type, u8 id, void *next) {
    item->type = type;
    NuDisplayListSetNext(item, next);
    NuDisplayListSetID(item, id);
}

extern "C" void NuDisplayListAddClut(nudisplaylistitem_s *item, i32) {
    NuDisplayListSetItem(item, 0x87, 0, nullptr);
}

extern "C" void NuDisplayListAddTexture(nudisplaylistitem_s *item, i32) {
    NuDisplayListSetItem(item, 0x87, 0, nullptr);
}

extern "C" void NuDisplayListAddMaterialState(nudisplaylistitem_s *item, void *mtl) {
    NuDisplayListSetItem(item, 0x80, 3, mtl);
}

extern "C" void NuDisplayListAddMicrocode(nudisplaylistitem_s *item, void *) {
    NuDisplayListSetItem(item, 0x87, 0, nullptr);
}

extern "C" void NuDisplayListAddLightState(nudisplaylistitem_s *item, void *) {
    NuDisplayListSetItem(item, 0x87, 0, nullptr);
}
