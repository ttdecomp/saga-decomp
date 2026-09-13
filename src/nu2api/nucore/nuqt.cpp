#include "nu2api/nucore/nuqt.h"

#include <string.h>

static int ElOverlaps(nuqtdim_s *a, nuqtdim_s *b) {
    if (a->x1 > b->x0 && b->x1 > a->x0 && a->y0 > b->y1 && b->y0 > a->y1)
        return 1;
    return 0;
}

static void RemoveData(nuqthdr_s *header, char *data, i32 count) {
    i32 length = header->element_size * count;
    char *end = (char *)header->data + header->data_used;
    char *destination = data;
    char *source = destination + length;
    while (source < end) {
        *destination = *source;
        ++destination;
        ++source;
    }
}

static i32 AddNode(nuqthdr_s *header, i32 child) {
    i32 index = 0;
    if (header->entry_count < header->entry_capacity) {
        index = header->entry_count;
        memset(&header->entries[index], 0, sizeof(nuqtentry_s));
        header->entries[index].child = child;
        header->entries[index].count = -1;
        ++header->entry_count;
    }
    return index;
}

static void NuQTFixAddress(nuqthdr_s *header) {
    uintptr_t base = (uintptr_t)header;
    header->entries = (nuqtentry_s *)((u8 *)header->entries + base);
    header->data = header->data + base;
    for (i32 index = 0; index < header->entry_count; ++index) {
        if (header->entries[index].count > 0)
            header->entries[index].data = header->entries[index].data + base;
    }
}

static void NuQTUnfixAddress(nuqthdr_s *header) {
    uintptr_t base = -(uintptr_t)header;
    for (i32 index = 0; index < header->entry_count; ++index) {
        if (header->entries[index].count > 0)
            header->entries[index].data = header->entries[index].data + base;
    }
    header->entries = (nuqtentry_s *)((u8 *)header->entries + base);
    header->data = header->data + base;
}
