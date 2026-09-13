#include "gamelib/crc/crc.h"

static i32 g_crc_initialised = 0;
i32 *g_crc_table = NULL;

#define CRC32_POLY 0x04C11DB7

static inline char CRC_ToUpper(char c) {
    if (static_cast<u8>(c - 'a') <= static_cast<u8>('z' - 'a')) {
        return c - ('a' - 'A');
    }
    return c;
}

void CRC_Init(VARIPTR *buffer_start) {
    if (g_crc_initialised) {
        return;
    }

    g_crc_table = reinterpret_cast<i32 *>(ALIGN(buffer_start->addr, alignof(i32)));
    buffer_start->addr = reinterpret_cast<usize>(g_crc_table + 0x100);

    for (u32 i = 0; i < 0x100; i++) {
        u32 crc = (i & 0x80U) != 0 ? (i << 25) ^ CRC32_POLY : i << 25;

        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        if ((crc & 0x80000000U) != 0) {
            crc = (crc << 1) ^ CRC32_POLY;
        } else {
            crc <<= 1;
        }
        g_crc_table[i] = crc;
    }

    g_crc_initialised = 1;
}

u32 CRC_Process(const void *data, u32 size) {
    if (size == 0) {
        return 0;
    }

    const u8 *cursor = static_cast<const u8 *>(data);
    const u8 *end = cursor + size;
    u32 crc = 0;
    do {
        const u32 table_index = *cursor++ ^ (crc >> 24);
        crc = (crc << 8) ^ g_crc_table[table_index];
    } while (cursor != end);
    return crc;
}

u32 CRC_ProcessString(const char *str) {
    u32 crc = 0;
    for (char c = *str++; c != '\0'; c = *str++) {
        const u32 table_index = static_cast<u32>(static_cast<i32>(c)) ^ (crc >> 24);
        crc = (crc << 8) ^ g_crc_table[table_index];
    }
    return crc;
}

u32 CRC_ProcessStringN(const char *str, u32 size) {
    u32 crc = 0;
    for (u32 i = 0; str[i] != '\0' && i < size; ++i) {
        const u32 table_index = static_cast<u32>(static_cast<i32>(str[i])) ^ (crc >> 24);
        crc = (crc << 8) ^ g_crc_table[table_index];
    }
    return crc;
}

u32 CRC_ProcessStringIgnoreCase(const char *str) {
    u32 crc = 0;
    for (char c = *str++; c != '\0'; c = *str++) {
        const u32 table_index = static_cast<u32>(static_cast<i32>(CRC_ToUpper(c))) ^ (crc >> 24);
        crc = (crc << 8) ^ g_crc_table[table_index];
    }
    return crc;
}

u32 CRC_ProcessStringNIgnoreCase(const char *str, u32 size) {
    u32 crc = 0;
    for (u32 i = 0; str[i] != '\0' && i < size; ++i) {
        const u32 table_index = static_cast<u32>(static_cast<i32>(CRC_ToUpper(str[i]))) ^ (crc >> 24);
        crc = (crc << 8) ^ g_crc_table[table_index];
    }
    return crc;
}
