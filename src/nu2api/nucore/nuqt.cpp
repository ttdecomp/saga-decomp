#include "nu2api/nucore/nuqt.h"
#include "nu2api/nufile/nufile.h"

#include <string.h>

static int ElOverlaps(nuqtdim_s *a, nuqtdim_s *b) {
    if (a->x1 > b->x0 && b->x1 > a->x0 && a->y0 > b->y1 && b->y0 > a->y1)
        return 1;
    return 0;
}

static i32 InsertData(nuqthdr_s *header, i32 index, void *item) {
    u8 *destination = header->entries[index].data;
    if (destination == NULL) {
        // The original tests the remaining capacity in this direction and copies
        // through the cached null pointer, even after assigning the entry data.
        if (header->data_capacity - header->data_used <= header->element_size) {
            header->entries[index].data = header->data + header->data_used;
            memcpy(destination, item, header->element_size);
            header->data_used += header->element_size;
            ++header->entries[index].count;
            return 1;
        }
    } else {
        u8 *source = header->data + header->data_used;
        u8 *target = source + header->element_size;
        if (header->data + header->data_capacity >= target) {
            while (source != destination) {
                --target;
                --source;
                *target = *source;
            }
            memcpy(destination, item, header->element_size);
            header->data_used += header->element_size;
            ++header->entries[index].count;
            return 1;
        }
    }
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

static i32 AddElementR(nuqthdr_s *header, i32 index, nuqtdim_s *bounds,
                       nuqtdim_s *item_bounds, void *item, i32 depth) {
    i32 result = 0;
    --depth;
    nuqtdim_s q0 = {bounds->x0, (bounds->x0 + bounds->x1) / 2.0f,
                    bounds->y0, (bounds->y0 + bounds->y1) / 2.0f};
    nuqtdim_s q1 = {(bounds->x0 + bounds->x1) / 2.0f, bounds->x1,
                    bounds->y0, (bounds->y0 + bounds->y1) / 2.0f};
    nuqtdim_s q2 = {bounds->x0, (bounds->x0 + bounds->x1) / 2.0f,
                    (bounds->y0 + bounds->y1) / 2.0f, bounds->y1};
    nuqtdim_s q3 = {(bounds->x0 + bounds->x1) / 2.0f, bounds->x1,
                    (bounds->y0 + bounds->y1) / 2.0f, bounds->y1};
    nuqtentry_s *entry = &header->entries[index];
    if (entry->count >= 0) {
        if (entry->count < static_cast<i32>(header->field_30) || depth == 0)
            return InsertData(header, index, item);

        u8 *old_data = entry->data;
        entry->children[0] = AddNode(header, index);
        entry->children[1] = AddNode(header, index);
        entry->children[2] = AddNode(header, index);
        entry->children[3] = AddNode(header, index);
        if (entry->children[0] == 0 || entry->children[1] == 0 ||
            entry->children[2] == 0 || entry->children[3] == 0) {
            entry->data = old_data;
            return InsertData(header, index, item);
        }
        u8 *old_item = old_data;
        for (i32 i = 0; i < entry->count; ++i) {
            // The original tests the incoming item's bounds when redistributing
            // existing data, and uses the saved data pointer for each recursive item.
            if (ElOverlaps(&q0, item_bounds))
                result = AddElementR(header, entry->children[0], &q0,
                                     item_bounds, old_item, depth);
            if (ElOverlaps(&q1, item_bounds))
                result = AddElementR(header, entry->children[1], &q1,
                                     item_bounds, old_item, depth);
            if (ElOverlaps(&q2, item_bounds))
                result = AddElementR(header, entry->children[2], &q2,
                                     item_bounds, old_item, depth);
            if (ElOverlaps(&q3, item_bounds))
                result = AddElementR(header, entry->children[3], &q3,
                                     item_bounds, old_item, depth);
            old_item += header->element_size;
        }
        RemoveData(header, reinterpret_cast<char *>(old_data), entry->count);
        entry->count = -1;
    }
    if (entry->count < 0) {
        if (ElOverlaps(&q0, item_bounds) && entry->children[0] != 0)
            result = AddElementR(header, entry->children[0], &q0,
                                 item_bounds, item, depth);
        if (ElOverlaps(&q1, item_bounds) && entry->children[1] != 0)
            result = AddElementR(header, entry->children[1], &q1,
                                 item_bounds, item, depth);
        if (ElOverlaps(&q2, item_bounds) && entry->children[2] != 0)
            result = AddElementR(header, entry->children[2], &q2,
                                 item_bounds, item, depth);
        if (ElOverlaps(&q3, item_bounds) && entry->children[3] != 0)
            result = AddElementR(header, entry->children[3], &q3,
                                 item_bounds, item, depth);
    }
    return result;
}

extern "C" void NuQTAddElement(nuqthdr_s *header, void *item, f32 x0, f32 x1,
                                 f32 y0, f32 y1) {
    i16 root = 0;
    nuqtdim_s bounds = {header->bounds_values[3], header->bounds_values[4],
                        header->bounds_values[1], header->bounds_values[2]};
    nuqtdim_s item_bounds = {x0, x1, y0, y1};
    AddElementR(header, static_cast<u16>(root), &bounds, &item_bounds, item,
                header->field_34);
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

static void NuQTFixAddress(nuqthdr_s *header) {
    uintptr_t base = (uintptr_t)header;
    header->entries = (nuqtentry_s *)((u8 *)header->entries + base);
    header->data = header->data + base;
    for (i32 index = 0; index < header->entry_count; ++index) {
        if (header->entries[index].count > 0)
            header->entries[index].data = header->entries[index].data + base;
    }
}

extern "C" nuqthdr_s *NuQTRead(char *path, u8 **cursor, u8 **end) {
    *cursor = reinterpret_cast<u8 *>((reinterpret_cast<usize>(*cursor) + 15) & ~usize(15));
    nuqthdr_s *header = reinterpret_cast<nuqthdr_s *>(*cursor);
    i32 size = NuFileLoadBuffer(path, *cursor, *end - *cursor);
    if (size != 0) {
        *cursor += size;
        NuQTFixAddress(header);
        return header;
    }
    return NULL;
}

extern "C" i32 NuQTWrite(char *path, nuqthdr_s *header) {
    NUFILE file = NuFileOpen(path, NUFILE_WRITE);
    if (file != 0) {
        i32 size = reinterpret_cast<usize>(header->data) + header->data_capacity - reinterpret_cast<usize>(header);
        NuQTUnfixAddress(header);
        NuFileWrite(file, header, size);
        NuQTFixAddress(header);
        NuFileClose(file);
        return 1;
    }
    return 0;
}

extern "C" i32 NuQTCreate(i32 entry_capacity, i32 data_capacity, i32 element_size,
                          u32 field_34, u32 field_30, u32 field_04, u32 field_0c,
                          u32 field_08, u32 field_10, u8 **cursor, u8 **end) {
    *cursor = reinterpret_cast<u8 *>((reinterpret_cast<usize>(*cursor) + 15) & ~usize(15));
    i32 required_size = sizeof(nuqthdr_s) + sizeof(nuqtentry_s) * entry_capacity +
                        16 + data_capacity;
    if (reinterpret_cast<usize>(*cursor) + required_size <= reinterpret_cast<usize>(*end)) {
        nuqthdr_s *header = reinterpret_cast<nuqthdr_s *>(*cursor);
        *cursor += sizeof(nuqthdr_s);
        header->field_00[0] = 0;
        header->entry_count = 0;
        header->entry_capacity = entry_capacity;
        header->data_used = 0;
        header->data_capacity = data_capacity;
        header->field_00[1] = field_04;
        header->field_00[3] = field_0c;
        header->field_00[2] = field_08;
        header->field_00[4] = field_10;
        header->element_size = element_size;
        header->field_34 = field_34;
        header->field_30 = field_30;
        header->entries = reinterpret_cast<nuqtentry_s *>(*cursor);
        *cursor += sizeof(nuqtentry_s) * entry_capacity;
        *cursor = reinterpret_cast<u8 *>((reinterpret_cast<usize>(*cursor) + 15) & ~usize(15));
        header->data = *cursor;
        *cursor += data_capacity;
    }
    return 0;
}
