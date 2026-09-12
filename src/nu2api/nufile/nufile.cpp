#include "nu2api/nufile/nufile.h"

#include <string.h>

#include "decomp.h"

#include "nu2api/nucore/common.h"
#include "nu2api/nucore/deflate.h"
#include "nu2api/nucore/implode.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nucore/nuthread.h"
#include "nu2api/nuplatform/nuplatform.h"

#ifdef ANDROID
#include "nu2api/nufile/android/nufile_android.h"
#endif

i32 read_critical_section = -1;

static NUDATHDR *curr_dat;
i32 nufile_try_packed = 0;
static FILEINFO file_info[33];
i32 nufile_buffering_enabled;
static i32 nufile_lasterror = 0;
NUDATFILEINFO dat_file_infos[20];

extern "C" i32 NuFileGetLastError(void) {
    return nufile_lasterror;
}

NUFILE_DEVICE host_device = {
    0,
    0,
    0xe,
    0,
    '/',

    0x400,
    -1,
    -1,

    -1,
    {0, 0, 0, 0},
    "d:",
    2,
    2,
    "",
    "Host Xbox 360 filesystem",
    "",
    "",
    "",

    &DEV_FormatName,
    &DEVHOST_Interrogate,
};

NUFILE_DEVICE *default_device = &host_device;

static i32 file_time_count;
static FILEBUFF file_buff[4];

static void AquireFileBuffer(FILEINFO *info) {
    i32 i;
    i32 buf_idx;
    i32 time;
    FILEBUFF *buf;
    i32 bytes_read;

    buf_idx = 0;
    time = file_time_count;

    for (i = 0; i < 4; i++) {
        if (file_buff[i].time < time) {
            time = file_buff[i].time;
            buf_idx = i;
        }
    }

    buf = &file_buff[buf_idx];
    if (buf->file_info != NULL) {
        buf->file_info->buf = NULL;
    }

    buf->file_info = info;
    buf->time = file_time_count++;
    info->buf = buf;

    if (info->buf_len != 0) {
        NuPSFileLSeek(info->handle, -info->buf_len, NUFILE_SEEK_CURRENT);
        bytes_read = NuPSFileRead(info->handle, buf->data, info->buf_len);
    }
}

i32 numdevices = 0;

NUFILE_DEVICE devices[16] = {0};
NUFILE_DEVICE enum_devices[16];
i32 enum_numdevices;

i32 NuFileEnumerateDevices(NUFILE_DEVICE **result) {
    i32 count = 0;
    for (i32 index = 0; index < numdevices; ++index) {
        if ((devices[index].interrogate_fn(&devices[index]) & NUFILE_DEVICE_STATUS_EXCLUDE_FROM_ENUMERATION) != 0) {
            continue;
        }
        enum_devices[count] = devices[index];
        ++count;
    }
    *result = enum_devices;
    enum_numdevices = count;
    return count;
}

i32 file_criticalsection;

i32 NuFileGetDevices(NUFILE_DEVICE **result) {
    if (result != NULL) {
        *result = enum_devices;
    }
    return enum_numdevices;
}

i32 NuFileRefreshDevices(NUFILE_DEVICE **result) {
    for (i32 index = 0; index < numdevices; ++index) {
        if (devices[index].id == 1) {
            i32 port = devices[index].params[0];
            i32 slot = devices[index].params[1];
            if (NuMcCheckCardPresent(port, slot) != 0) {
                if (NuMcCheckCardFormatted(port, slot) != 0) {
                    devices[index].attr |= 8;
                }
                devices[index].free_space = NuMcCheckCardFreeSpace(port, slot);
            } else {
                devices[index].id = -1;
            }
        }
    }
    if (result != NULL) {
        *result = devices;
    }
    return numdevices;
}

NUFILE_DEVICE *NuFileFindDevice(i32 id, i32 unit) {
    if (id == -3) {
        return default_device;
    }
    if (id == -2 || id == -1) {
        id = 2;
    }
    for (i32 index = 0; index < numdevices; ++index) {
        if (devices[index].id == id && (unit < 0 || devices[index].unit_id == unit)) {
            return &devices[index];
        }
    }
    return NULL;
}

NUFILE_DEVICE *NuFileGetDeviceFromPath(char *path) {
    NUFILE_DEVICE *device;
    i32 i;
    i32 device_idx;

    device = NULL;

    for (i = 0; i < 8; i++) {
        if (path[i] == ':') {
            break;
        }
    }

    if (i < 8) {
        for (device_idx = 0; device_idx < numdevices; device_idx++) {
            if (NuStrNICmp(path, devices[device_idx].name, devices[device_idx].match_len) == 0) {
                device = &devices[device_idx];
                break;
            }
        }
    }

    return device;
}

i32 DontDoFileUpperCaseHack;

void NuFileUpCase(NUFILE_DEVICE *device, char *filepath) {
    char separator;

    if (device != NULL) {
        separator = device->dir_separator;
    } else {
        separator = '\\';
    }

    if (DontDoFileUpperCaseHack) {
        for (; *filepath != '\0'; filepath++) {
            if (*filepath == '\\' || *filepath == '/') {
                *filepath = separator;
            }
        }
    } else {
        for (; *filepath != '\0'; filepath++) {
            switch (*filepath) {
                case 'a' ... 'z':
                    *filepath -= 0x20;
                    break;
                case '/':
                case '\\':
                    *filepath = separator;
                    break;
            }
        }
    }
}

NUFILE NuFileOpen(char *filepath, NUFILEMODE mode) {
    return NuFileOpenDF(filepath, mode, curr_dat, 0);
}

void NuFileClose(NUFILE file) {
    FILEINFO *info;

#ifdef ANDROID
    if (file >= 0x2000) {
        NuFileAndroidAPK::CloseFile(file);
        return;
    }
#endif

    if (file >= 0x1000) {
        NuMcClose(file - 0x1000, 0);
        return;
    }

    if (file >= 0x400) {
        NuMemFileClose(file);
        return;
    }

    file -= 1;

    while (NuPSFileClose(file) < 0) {
    }

    info = &file_info[file];
    if (info->buf != NULL) {
        info->buf->file_info = NULL;
    }

    memset(info, 0, sizeof(fileinfo_s));
}

NUFILE NuFileOpenDF(char *filepath, NUFILEMODE mode, NUDATHDR *header, i32 _unused) {
    LOG_DEBUG("filepath=%s, mode=%d", filepath, mode);

    char buf[256];
    i64 len;
    NUFILE_DEVICE *device;
    i32 mc_slot;
    i32 mc_port;
    i32 ps_index;
    i32 file_index;
    NUFILE file_handle;

    device = NuFileGetDeviceFromPath(filepath);
    if (device == NULL) {
        if (mode != NUFILE_WRITE && mode != NUFILE_APPEND) {
            file_handle = 0;

            if (header != NULL) {
                file_handle = NuDatFileOpen(header, filepath, mode);
            } else {
#ifdef ANDROID
                if (g_apkFileDevice != NULL) {
                    file_handle = NuFileAndroidAPK::OpenFile(filepath, NuFile::OpenMode::READ);
                }
#endif
            }

            if (file_handle > 0) {
                return file_handle;
            }
        }

        device = default_device;
    }

    if ((device->attr & 1) != 0 && mode == NUFILE_WRITE && mode == NUFILE_APPEND) {
        device = &host_device;
    }

    (*device->format_name_fn)(device, buf, filepath, 0x100);

    if (device->id == 2) {
        NuStrCat(buf, ";1");
    }

    if (device->id == 1) {
        mc_port = buf[2] - 0x30;
        mc_slot = buf[3] - 0x30;

        file_index = NuMcOpen(mc_port, mc_slot, buf + device->name_len, mode, 0);
        if (file_index >= 0) {
            file_index += 0x1000;
        } else {
            file_index = 0;
        }

        return file_index;
    } else {
        file_index = NuPSFileOpen(buf, mode);
        if (file_index < 0) {
            return 0;
        }

        ps_index = file_index;

        memset(&file_info[ps_index], 0, sizeof(FILEINFO));

        file_info[ps_index].handle = file_index;
        file_info[ps_index].mode = mode;

        if (mode != NUFILE_WRITE) {
            if (file_index <= 16) {
                do {
                    len = NuFileSeek(ps_index + 1, 0, NUFILE_SEEK_END);
                } while (len < 0);

                file_info[ps_index].file_len = len;

                if (mode == NUFILE_READ_NOWAIT) {
                    while (NuFileStatus(ps_index + 1) != 0) {
                    }
                }

                do {
                    len = NuFileSeek(ps_index + 1, 0, NUFILE_SEEK_START);
                } while (len < 0);

                if (mode == NUFILE_READ_NOWAIT) {
                    while (NuFileStatus(ps_index + 1) != 0) {
                    }
                }
            }
        } else {
            file_info[ps_index].file_len = 0;
        }

        if (mode == NUFILE_READ) {
            file_info[ps_index].use_buf = nufile_buffering_enabled;
        }
    }

    return ps_index + 1;
}

i64 NuFileSeek(NUFILE file, i64 offset, NUFILESEEK whence) {
    FILEINFO *info;
    i64 pos;

#ifdef ANDROID
    NuFile::SeekOrigin::T native_mode;

    if (file >= 0x2000) {
        switch (whence) {
            default:
                native_mode = NuFile::SeekOrigin::START;
                break;
            case NUFILE_SEEK_CURRENT:
                native_mode = NuFile::SeekOrigin::CURRENT;
                break;
            case NUFILE_SEEK_END:
                native_mode = NuFile::SeekOrigin::END;
                break;
        }

        return NuFileAndroidAPK::SeekFile(file, offset, native_mode);
    }
#endif

    if (file >= 0x1000) {
        pos = (i64)NuMcSeek(file - 0x1000, offset, whence, 0);
        return pos;
    }

    if (file >= 0x400) {
        pos = NuMemFileSeek(file, (i32)offset, whence);
        return pos;
    }

    file -= 1;
    info = &file_info[file];

    if (info->use_buf) {
        switch (whence) {
            default:
                info->read_pos = offset;
                break;
            case NUFILE_SEEK_CURRENT:
                info->read_pos += offset;
                break;
            case NUFILE_SEEK_END:
                info->read_pos = info->file_len - offset;
                break;
        }
    } else {
        pos = NuPSFileLSeek(file, offset, whence);
        return pos;
    }

    return info->read_pos;
}

i32 NuFileExistQuiet(char *filepath) {
    NUFILE file = 0;
    if (filepath == NULL || *filepath == '\0') {
        return 0;
    }
    i32 buffering = nufile_buffering_enabled;
    nufile_buffering_enabled = 0;
    if (curr_dat != NULL) {
        i32 index = NuDatFileFindTree(curr_dat, filepath);
        if (index >= 0) {
            nufile_buffering_enabled = buffering;
            return curr_dat->file_info[index].decompressed_len;
        }
    }
    file = NuFileOpenDF(filepath, NUFILE_READ, curr_dat, 1);
    if (file != 0) {
        NuFileClose(file);
    }
    nufile_buffering_enabled = buffering;
    return file != 0;
}

i64 NuFileSize(char *filepath) {
    i32 file;
    i64 pos;
    i32 buffering;

    buffering = nufile_buffering_enabled;
    nufile_buffering_enabled = 0;

    if (curr_dat != NULL) {
        file = NuDatFileFindTree(curr_dat, filepath);
        if (file >= 0) {
            return (u64)curr_dat->file_info[file].decompressed_len;
        }
    }

    pos = -1;

    if (filepath != NULL && *filepath != '\0') {
        i32 file;

        file = NuFileOpen(filepath, NUFILE_READ);
        if (file != 0) {
            do {
                pos = NuFileSeek(file, 0, NUFILE_SEEK_END);
            } while (pos < 0);

            pos = NuFilePos(file);
            NuFileClose(file);
        }
    }

    nufile_buffering_enabled = buffering;

    return pos;
}

i64 NuFilePos(NUFILE file) {
    FILEINFO *info;
    i64 pos;

#ifdef ANDROID
    if (file >= 0x2000) {
        return NuFileAndroidAPK::GetFilePos(file);
    }
#endif

    if (file >= 0x1000) {
        pos = NuMcSeek(file - 0x1000, 0, NUFILE_SEEK_END, 0);
        return pos;
    }

    if (file >= 0x400) {
        return NuMemFilePos(file);
    }

    file -= 1;
    info = &file_info[file];

    if (info->use_buf) {
        return info->read_pos;
    }

    do {
        pos = NuPSFileLSeek(file, 0, NUFILE_SEEK_CURRENT);
    } while (pos < 0);

    return pos;
}

i64 NuFileOpenSize(NUFILE file) {
#ifdef ANDROID
    if (file >= 0x2000) {
        return NuFileAndroidAPK::GetFileSize(file);
    }
#endif

    if (file >= 0x1000) {
        return NuMcFileOpenSize(file);
    }

    if (file >= 0x800) {
        return NuDatFileOpenSize(file);
    }

    if (file >= 0x400) {
        return NuMemFileOpenSize(file);
    }

    file -= 1;

    return file_info[file].file_len;
}

i32 NuFileRead(NUFILE file, void *buf, i32 size) {
    FILEINFO *info;
    i32 bytes_read;
    char *char_buf;
    i32 available;

#ifdef ANDROID
    if (file >= 0x2000) {
        return NuFileAndroidAPK::ReadFile(file, buf, size);
    }
#endif

    if (file >= 0x1000) {
        return NuMcRead(file - 0x1000, buf, size, 0);
    }

    if (file >= 0x400) {
        return NuMemFileRead(file, buf, size);
    }

    file -= 1;
    info = &file_info[file];

    if (info->use_buf) {
        if (info->buf == NULL) {
            AquireFileBuffer(info);
        }

        bytes_read = 0;
        char_buf = (char *)buf;

        while (size > 0) {
            if (info->read_pos >= info->file_len) {
                return bytes_read;
            }

            if (info->read_pos < info->buf_start || info->read_pos >= info->buf_start + info->buf_len) {
                if (info->read_pos != info->file_pos) {
                    NuPSFileLSeek(info->handle, info->read_pos, NUFILE_SEEK_START);
                    info->file_pos = info->read_pos;
                }

                info->buf_len = NuPSFileRead(info->handle, info->buf->data, 0x400);
                info->buf_start = info->file_pos;
                info->file_pos += info->buf_len;
            }

            available = MIN(info->buf_len + (i32)(info->buf_start - info->read_pos), size);
            if (available != 0) {
                u32 to_read;
                void *file_buf;

                to_read = available;
                file_buf = info->buf;
                memcpy(char_buf, (void *)((usize)file_buf + (info->read_pos - info->buf_start) + 8), to_read);
            }

            char_buf += available;
            bytes_read += available;
            size -= available;
            info->read_pos += available;
        }

        return bytes_read;
    } else {
        return NuPSFileRead(file, buf, size);
    }
}

i32 NuFileWrite(NUFILE file, void *data, i32 size) {
    if (file >= 0x1000) {
        return NuMcWrite(file - 0x1000, data, size, 0);
    }

    if (file >= 0x400) {
        return NuMemFileWrite(file, data, size);
    }

    file -= 1;

    return NuPSFileWrite(file, data, size);
}

i8 NuFileReadChar(NUFILE file) {
    i8 value = 0;
    NuFileRead(file, &value, sizeof(i8));
    return value;
}

f32 NuFileReadFloat(NUFILE file) {
    f32 value;
    NuFileRead(file, &value, sizeof(f32));
    return value;
}

i32 NuFileReadInt(NUFILE file) {
    i32 value;
    NuFileRead(file, &value, sizeof(i32));
    return value;
}

i16 NuFileReadShort(NUFILE file) {
    i16 value;
    NuFileRead(file, &value, sizeof(i16));
    return value;
}

u8 NuFileReadUnsignedChar(NUFILE file) {
    u8 value = 0;
    NuFileRead(file, &value, sizeof(u8));
    return value;
}

u32 NuFileReadUnsignedInt(NUFILE file) {
    u32 value;
    NuFileRead(file, &value, sizeof(u32));
    return value;
}

u16 NuFileReadUnsignedShort(NUFILE file) {
    u16 value;
    NuFileRead(file, &value, sizeof(u16));
    return value;
}

u16 NuFileReadWChar(NUFILE file) {
    i16 value = 0;
    NuFileRead(file, &value, sizeof(u16));
    return value;
}

i32 NuFile_SwapEndianOnWrite;

static void NuFileEndianSwap16(void *value) {
    u8 *value_ptr;
    u16 swapped;
    u8 *swap_ptr;

    value_ptr = (u8 *)value;
    swap_ptr = (u8 *)&swapped;
    swap_ptr[0] = value_ptr[1];
    swap_ptr[1] = value_ptr[0];
    *((u16 *)value) = swapped;
}

static void NuFileEndianSwap32(void *value) {
    u8 *value_ptr;
    u32 swapped;
    u8 *swap_ptr;

    value_ptr = (u8 *)value;
    swap_ptr = (u8 *)&swapped;

    swap_ptr[0] = value_ptr[3];
    swap_ptr[1] = value_ptr[2];
    swap_ptr[2] = value_ptr[1];
    swap_ptr[3] = value_ptr[0];

    *((u32 *)value) = swapped;
}

i32 NuFileWriteInt(NUFILE file, i32 value) {
    i32 tmp;

    tmp = value;
    if (NuFile_SwapEndianOnWrite != 0) {
        NuFileEndianSwap32(&tmp);
    }

    return NuFileWrite(file, &tmp, sizeof(i32));
}

i32 NuFileWriteFloat(NUFILE file, float value) {
    float tmp = value;
    if (NuFile_SwapEndianOnWrite != 0) {
        NuFileEndianSwap32(&tmp);
    }
    return NuFileWrite(file, &tmp, sizeof(tmp));
}

i32 NuFileWriteShort(NUFILE file, i16 value) {
    i16 tmp = value;
    if (NuFile_SwapEndianOnWrite != 0) {
        NuFileEndianSwap16(&tmp);
    }
    return NuFileWrite(file, &tmp, sizeof(tmp));
}

i32 NuFileWriteUnsignedShort(NUFILE file, u16 value) {
    u16 tmp = value;
    if (NuFile_SwapEndianOnWrite != 0) {
        NuFileEndianSwap16(&tmp);
    }
    return NuFileWrite(file, &tmp, sizeof(tmp));
}

i32 NuFileWriteChar(NUFILE file, i8 value) {
    return NuFileWrite(file, &value, sizeof(value));
}

i32 NuFileWriteUnsignedChar(NUFILE file, u8 value) {
    return NuFileWrite(file, &value, sizeof(value));
}

u32 NuFileWriteUnsignedInt(NUFILE file, u32 value) {
    u32 tmp;

    tmp = value;
    if (NuFile_SwapEndianOnWrite != 0) {
        NuFileEndianSwap32(&tmp);
    }

    return NuFileWrite(file, &tmp, sizeof(u32));
}

i32 NuFileLoadBuffer(char *filepath, void *buf, i32 buf_size) {
    NUFILE file;
    i32 iVar2;
    i32 lVar1;
    i32 loaded;

    nufile_lasterror = 0;
    loaded = 0;

    if (curr_dat != NULL) {
        loaded = NuDatFileLoadBuffer(curr_dat, filepath, buf, buf_size);

        if (nufile_lasterror == -1) {
            return 0;
        }
    }

    if (loaded == 0) {
        if (NuFileExists(filepath)) {
            file = NuFileOpen(filepath, NUFILE_READ);
            if (file != 0) {
                if (nufile_try_packed) {
                    loaded = NuPPLoadBuffer(file, buf, buf_size);
                } else {
                    loaded = NuFileOpenSize(file);

                    if (loaded <= buf_size && loaded != 0) {
                        while (NuFileRead(file, buf, loaded) < 0) {
                            while (NuFileSeek(file, 0, NUFILE_SEEK_START) < 0) {
                            }
                        }
                    } else {
                        nufile_lasterror = -1;
                        loaded = 0;
                    }
                }

                NuFileClose(file);
            } else {
                nufile_lasterror = -3;
            }
        } else {
            nufile_lasterror = -2;
        }
    }

    return loaded;
}

i32 NuFileLoadBufferVP(char *filepath, VARIPTR *buf, VARIPTR *buf_end) {
    i32 len;

    len = NuFileLoadBuffer(filepath, buf->void_ptr, buf_end->addr - buf->addr);

    buf->addr += len;

    return len;
}

void NuFileCorrectSlashes(NUFILE_DEVICE *device, char *path) {
    char separator;

    if (device != NULL) {
        separator = device->dir_separator;
    } else {
        separator = '\\';
    }

    for (; *path != '\0'; path++) {
        if (*path == '\\' || *path == '/') {
            *path = separator;
        }
    }
}

void NuFileReldirFix(NUFILE_DEVICE *device, char *path) {
    char *cursor;
    char *inner;

    cursor = inner = path;

    while (*cursor != '\0') {
        if (*cursor == device->dir_separator && cursor[1] == '.' && cursor[2] == '.' &&
            cursor[3] == device->dir_separator) {
            inner = cursor;

            while (inner > path) {
                inner--;
                if (*inner == device->dir_separator) {
                    cursor += 3;
                    break;
                }

                if (*inner == ':') {
                    inner++;
                    cursor += 4;
                    break;
                }
            }
        }

        *inner = *cursor;
        inner++;
        cursor++;
    }

    *inner = '\0';
}

i32 NuFileStatus(NUFILE file) {
    i32 _unused;
    i32 _unused2;
    NUDATFILEINFO *info;
    NUDATOPENFILEINFO *open_info;

    _unused = 0;

    if (file >= 0x2000) {
        do {
        } while (true);
    }

    if (file >= 0x400) {
        return 0;
    }

    if (file >= 0x800) {
        file -= 0x800;
        info = dat_file_infos + file;
        open_info = &info->hdr->open_files[info->open_file_idx];

        return NuFileStatus(open_info->dat_file);
    }

    file -= 1;
    _unused2 = 0;

    return _unused;
}

static NUDATFILEINFO *unpack_file_info;
static NUDATOPENFILEINFO *unpack_file_odi;
static i64 unpack_file_pos;
static i32 decode_buffer_left;
static char decode_buffer[0x40000];
static i32 decode_buffer_pos;
static char read_buffer[0x40000];
static i32 read_buffer_size;
static i32 read_buffer_decoded_size;

static void NuDatFileDecodeInit() {
    unpack_file_info = NULL;
    unpack_file_odi = NULL;
    unpack_file_pos = -1;
}

static void NuDatFileDecodeNewFile(NUDATFILEINFO *file, NUDATOPENFILEINFO *open_file) {
    unpack_file_info = file;
    unpack_file_odi = open_file;
    unpack_file_pos = file->start;
    decode_buffer_left = 0;
}

static void NuDatFileDecodeNext() {
    char char_buf[12];
    i32 int_buf[3];
    i32 compressed_size;
    NUDATFILEINFO *info;
    NUDATOPENFILEINFO *open_info;

    switch (unpack_file_info->compression_mode) {
        case 2:
            NuFileRead(unpack_file_odi->dat_file, char_buf, 0xc);

            read_buffer_decoded_size = ExplodeBufferSize(char_buf);
            read_buffer_size = ExplodeCompressedSize(char_buf) - 0xc;
            NuFileRead(unpack_file_odi->dat_file, read_buffer, read_buffer_size);

            unpack_file_info->pos += read_buffer_size + 0xc;
            unpack_file_odi->pos = unpack_file_info->pos;
            break;
        case 3:
            NuFileRead(unpack_file_odi->dat_file, int_buf, 0xc);

            read_buffer_decoded_size = int_buf[2];
            read_buffer_size = int_buf[1];
            NuFileRead(unpack_file_odi->dat_file, read_buffer, read_buffer_size);

            unpack_file_info->pos += read_buffer_size + 0xc;
            unpack_file_odi->pos = unpack_file_info->pos;
    }
}

static void APIEndianSwap(void *data, i32 count, i32 size) {
}

NUDATHDR *NuDatOpen(char *filepath, VARIPTR *buf, i32 *_unused) {
    return NuDatOpenEx(filepath, buf, _unused, 0);
}

NUDATHDR *NuDatOpenEx(char *filepath, VARIPTR *buf, i32 *_unused, i16 mode) {
    char _unused2[256];
    i32 path_len;
    i32 i;
    i32 j;
    NUFILE file;
    i64 seek_offset;
    i32 file_len;
    i32 bytes_read;
    i32 _unused4;
    i32 read_buf[2];
    i32 total_read;
    i32 n;
    NUDATHDR *hdr;
    NUDFNODE_V1 *old_ver;
    VARIPTR hash;
    u32 hash_idx;
    i32 idx_to_swap;
    u32 tmp_idx;
    NUDATFINFO tmp;
    i32 dfnodev2_size;
    i32 dfnodev1_size;
    i32 buffering;
    i32 _unused3;

    _unused3 = 0;
    total_read = 0;

    buffering = nufile_buffering_enabled;

    file = NuFileOpenDF(filepath, (NUFILEMODE)mode, NULL, 0);
    if (file != 0) {
        bytes_read = 0;

        file_len = NuFileOpenSize(file);
        bytes_read = NuFileRead(file, read_buf, 8);
        APIEndianSwap(read_buf, 2, 4);

        seek_offset = (i64)read_buf[0];
        if (seek_offset < 0) {
            seek_offset *= -0x100;
        }

        NuFileSeek(file, seek_offset, NUFILE_SEEK_START);

        path_len = ALIGN(NuStrLen(filepath) + 1, 0x10);

        hdr = (NUDATHDR *)buf->void_ptr;
        buf->addr += sizeof(NUDATHDR);
        memset(hdr, 0, sizeof(NUDATHDR));

        hdr->unknown = 1;

        hdr->name = (char *)buf->void_ptr;
        buf->addr += path_len;
        NuStrCpy(hdr->name, filepath);

        bytes_read = NuFileRead(file, &hdr->version, 4);
        total_read += bytes_read;
        APIEndianSwap(&hdr->version, 1, 4);

        bytes_read = NuFileRead(file, &hdr->file_count, 4);
        total_read += bytes_read;
        APIEndianSwap(&hdr->file_count, 1, 4);

        hdr->file_info = (NUDATFINFO *)ALIGN(buf->addr, 0x20);
        buf->addr = ALIGN(buf->addr, 0x20);
        buf->addr += hdr->file_count * sizeof(NUDATFINFO);
        bytes_read = NuFileRead(file, hdr->file_info, hdr->file_count * sizeof(NUDATFINFO));
        total_read += bytes_read;
        for (i = 0; i < hdr->file_count; i++) {
            APIEndianSwap(&hdr->file_info[i].file_offset, 1, 4);
            APIEndianSwap(&hdr->file_info[i].file_len, 1, 4);
            APIEndianSwap(&hdr->file_info[i].decompressed_len, 1, 4);
        }

        bytes_read = NuFileRead(file, &hdr->tree_node_count, 4);
        total_read += bytes_read;
        APIEndianSwap(&hdr->tree_node_count, 1, 4);

        hdr->file_tree = (NUDFNODE *)buf->void_ptr;
        buf->addr += hdr->tree_node_count * sizeof(NUDFNODE);

        if (hdr->version < -4) {
            // Using `sizeof()` causes this not to match.
            dfnodev2_size = hdr->tree_node_count * 0xc;
            bytes_read = NuFileRead(file, hdr->file_tree, dfnodev2_size);
            total_read += bytes_read;
        } else {
            dfnodev1_size = hdr->tree_node_count * sizeof(NUDFNODE_V1);
            bytes_read = NuFileRead(file, hdr->file_tree, dfnodev1_size);
            total_read += bytes_read;
            old_ver = (NUDFNODE_V1 *)hdr->file_tree;

            for (n = hdr->tree_node_count - 1; n > 0; n--) {
                hdr->file_tree[n].unknown = 0;
                hdr->file_tree[n].unknown2 = 0;
                hdr->file_tree[n].name = old_ver[n].name;
                hdr->file_tree[n].sibling_idx = old_ver[n].sibling_idx;
                hdr->file_tree[n].child_idx = old_ver[n].child_idx;
            }
        }

        for (n = 0; n < hdr->tree_node_count; n++) {
            APIEndianSwap(&hdr->file_tree[n].child_idx, 1, 2);
            APIEndianSwap(&hdr->file_tree[n].sibling_idx, 1, 2);
            APIEndianSwap(&hdr->file_tree[n].name, 1, 4);

            if (hdr->version < -4) {
                APIEndianSwap(&hdr->file_tree[n].unknown, 1, 2);
                APIEndianSwap(&hdr->file_tree[n].unknown2, 1, 2);
            }
        }

        bytes_read = NuFileRead(file, &hdr->leaf_names_len, 4);
        total_read += bytes_read;
        APIEndianSwap(&hdr->leaf_names_len, 1, 4);

        hdr->leaf_names = buf->char_ptr;
        buf->addr += hdr->leaf_names_len;
        bytes_read = NuFileRead(file, hdr->leaf_names, hdr->leaf_names_len);
        total_read += bytes_read;

        for (n = 0; n < hdr->tree_node_count; n++) {
            _unused4 = 0;
            hdr->file_tree[n].name = reinterpret_cast<char *>(
                reinterpret_cast<usize>(hdr->leaf_names) + reinterpret_cast<usize>(hdr->file_tree[n].name) - _unused4);
        }

        hdr->file_tree[0].name = NULL;

        buf->addr -= hdr->leaf_names_len;
        buf->addr = buf->addr - hdr->tree_node_count * sizeof(NUDFNODE);
        hdr->file_tree = NULL;
        hdr->leaf_names = NULL;

        hdr->hash_idxs = NULL;
        hdr->hash_count = 0;
        hdr->hashes_len = 0;
        hdr->hashes = NULL;

        if (hdr->version < -1) {
            hdr->hash_idxs = (u32 *)ALIGN(buf->addr, 0x20);
            buf->addr = ALIGN(buf->addr, 0x20);
            buf->addr += hdr->file_count * sizeof(i32);
            bytes_read = NuFileRead(file, hdr->hash_idxs, hdr->file_count * sizeof(i32));
            total_read += bytes_read;

            for (n = 0; n < hdr->file_count; n++) {
                APIEndianSwap(&hdr->hash_idxs[n], 1, 4);
            }

            bytes_read = NuFileRead(file, &hdr->hash_count, 4);
            total_read += bytes_read;
            APIEndianSwap(&hdr->hash_count, 1, 4);

            bytes_read = NuFileRead(file, &hdr->hashes_len, 4);
            total_read += bytes_read;
            APIEndianSwap(&hdr->hashes_len, 1, 4);

            hdr->hashes = (char *)ALIGN(buf->addr, 0x20);
            buf->addr = ALIGN(buf->addr, 0x20);
            buf->addr += hdr->hashes_len;
            bytes_read = NuFileRead(file, hdr->hashes, hdr->hashes_len);
            total_read += bytes_read;

            hash.char_ptr = hdr->hashes;
            for (n = 0; n < hdr->hash_count; n++) {
                hash.addr += NuStrLen(hash.char_ptr) + 1;
                if ((hash.addr & 1) != 0) {
                    hash.addr += 1;
                }

                APIEndianSwap(hash.char_ptr, 1, 2);
                hash.addr += 2;
            }
        }

        for (n = 0; n < 20; n++) {
            hdr->open_files[n].info_idx = -1;
            hdr->open_files[n].dat_file = 0;
            hdr->open_files[n].pos = 0;
        }

        hdr->open_files[0].dat_file = file;
        hdr->open_files[0].pos = 0;

        hdr->mode = mode;

        if (hdr->version > -3) {
            for (i = 0; i < hdr->file_count - 1; i++) {
                hash_idx = hdr->hash_idxs[i];
                idx_to_swap = i;

                for (j = i + 1; j < hdr->file_count; j++) {
                    if (hdr->hash_idxs[j] <= hash_idx) {
                        hash_idx = hdr->hash_idxs[j];
                        idx_to_swap = j;
                    }
                }

                if (i != idx_to_swap) {
                    tmp_idx = hdr->hash_idxs[i];
                    hdr->hash_idxs[j] = hdr->hash_idxs[idx_to_swap];
                    hdr->hash_idxs[idx_to_swap] = tmp_idx;

                    tmp = hdr->file_info[i];
                    hdr->file_info[i] = hdr->file_info[idx_to_swap];
                    hdr->file_info[idx_to_swap] = tmp;
                }
            }
        }

        return hdr;
    }

    nufile_buffering_enabled = buffering;

    return NULL;
}

NUDATHDR *NuDatSet(NUDATHDR *header) {
    NUDATHDR *dat = curr_dat;
    curr_dat = header;
    return dat;
}

extern "C" NUDATHDR *NuDatGet(void) {
    return curr_dat;
}

static i32 OpenDatFileBase(NUDATHDR *hdr, i32 file_idx) {
    NUDATOPENFILEINFO *open_file;

    open_file = hdr->open_files + file_idx;
    if (open_file->dat_file == 0) {
        open_file->dat_file = NuFileOpenDF(hdr->name, (NUFILEMODE)hdr->mode, NULL, 0);
        open_file->pos = 0;
    }

    return open_file->dat_file;
}

NUFILE NuDatFileOpen(NUDATHDR *hdr, char *filepath, NUFILEMODE mode) {
    i32 node_idx;
    i32 info_idx;
    i32 file_idx;
    NUDATOPENFILEINFO *open_info;
    NUDATFILEINFO *info;
    i64 start;
    i64 seek_pos;

    if (mode == NUFILE_READ) {
        node_idx = NuDatFileFindTree(hdr, filepath);
        if (node_idx > -1 && hdr->file_info[node_idx].file_len != 0) {
            info_idx = NuDatFileGetFreeInfo();
            if (info_idx != -1) {
                file_idx = NuDatFileGetFreeHandleIX(hdr, info_idx);
                if (file_idx != -1) {
                    open_info = &hdr->open_files[file_idx];

                    NuThreadCriticalSectionBegin(file_criticalsection);

                    OpenDatFileBase(hdr, file_idx);

                    dat_file_infos[info_idx].hdr = hdr;
                    dat_file_infos[info_idx].start = NuDatCalcPos(hdr, hdr->file_info[node_idx].file_offset);
                    dat_file_infos[info_idx].pos = dat_file_infos[info_idx].start;

                    dat_file_infos[info_idx].file_len = hdr->file_info[node_idx].file_len;
                    dat_file_infos[info_idx].decompressed_len = hdr->file_info[node_idx].decompressed_len;
                    dat_file_infos[info_idx].compression_mode = hdr->file_info[node_idx].compression_mode;

                    dat_file_infos[info_idx].open_file_idx = file_idx;

                    open_info->pos = dat_file_infos[info_idx].pos;

                    do {
                        seek_pos = NuFileSeek(open_info->dat_file, dat_file_infos[info_idx].start, NUFILE_SEEK_START);
                    } while (seek_pos < 0);

                    if (dat_file_infos[info_idx].compression_mode == 2 ||
                        dat_file_infos[info_idx].compression_mode == 3) {
                        NuDatFileDecodeNewFile(&dat_file_infos[info_idx], open_info);
                    }

                    NuThreadCriticalSectionEnd(file_criticalsection);

                    return info_idx + 0x800;
                }

                dat_file_infos[info_idx].is_used = 0;
            }
        }
    }

    return 0;
}

void NuDatFileClose(NUFILE file) {
    NUDATFILEINFO *info;
    NUDATHDR *hdr;

    file -= 0x800;
    info = dat_file_infos + file;
    hdr = info->hdr;

    if (info->open_file_idx >= 0) {
        hdr->open_files[info->open_file_idx].info_idx = -1;
    }

    info->is_used = 0;
}

i64 NuDatFileSeek(NUFILE file, i64 offset, NUFILESEEK whence) {
    NUDATFILEINFO *info;
    NUDATHDR *hdr;
    NUDATOPENFILEINFO *open_info;
    i64 offset_in_dat;

    file -= 0x800;
    info = &dat_file_infos[file];
    open_info = &info->hdr->open_files[info->open_file_idx];

    switch (whence) {
        default:
            offset_in_dat = offset + info->start;
            break;
        case NUFILE_SEEK_CURRENT:
            offset_in_dat = offset + info->pos;
            break;
        case NUFILE_SEEK_END:
            offset_in_dat = info->start + info->file_len - offset;
            break;
    }

    info->pos = NuFileSeek(open_info->dat_file, offset_in_dat, NUFILE_SEEK_START);
    open_info->pos = info->pos;

    return info->pos;
}

i64 NuDatFilePos(NUFILE file) {
    NUDATFILEINFO *info;

    file -= 0x800;
    info = dat_file_infos + file;

    return info->pos - info->start;
}

i32 NuDatFileRead(NUFILE file, void *buf, i32 size) {
    NUDATFILEINFO *info;
    NUDATOPENFILEINFO *open_file;
    i32 inflated_size;
    i32 total_read;
    i32 bytes_read;
    i32 to_read;

    file -= 0x800;
    info = &dat_file_infos[file];
    open_file = &info->hdr->open_files[info->open_file_idx];

    if (info->compression_mode != 0) {
        total_read = 0;

        while (size != 0) {
            if (decode_buffer_left == 0) {
                NuDatFileDecodeNext();

                if (read_buffer_size == read_buffer_decoded_size) {
                    memcpy(decode_buffer, read_buffer, read_buffer_size);
                } else if (info->compression_mode == 2) {
                    ExplodeBufferNoHeader(read_buffer, decode_buffer, read_buffer_size, read_buffer_decoded_size);
                } else if (info->compression_mode == 3) {
                    inflated_size =
                        InflateBuffer(decode_buffer, read_buffer_decoded_size, read_buffer, read_buffer_size);
                }

                decode_buffer_pos = 0;
                decode_buffer_left = read_buffer_decoded_size;
            }

            bytes_read = MIN(size, decode_buffer_left);
            memcpy(buf, decode_buffer + decode_buffer_pos, bytes_read);
            buf = (void *)((char *)buf + bytes_read);

            decode_buffer_pos += bytes_read;
            decode_buffer_left -= bytes_read;
            size -= bytes_read;
            total_read += bytes_read;
        }

        return total_read;
    }

    if (info->pos != open_file->pos) {
        NuFileSeek(open_file->dat_file, info->pos, NUFILE_SEEK_START);
        open_file->pos = info->pos;
    }

    to_read = MIN(size, MAX(info->file_len + info->start - info->pos, 0));
    if (to_read != 0) {
        total_read = NuFileRead(open_file->dat_file, buf, to_read);
        if (total_read > -1) {
            info->pos += total_read;
            open_file->pos = info->pos;
        }

        return total_read;
    }

    return 0;
}

i32 NuDatFileGetFreeInfo(void) {
    i32 found;
    i32 i;

    found = -1;

    NuThreadCriticalSectionBegin(file_criticalsection);

    for (i = 0; i < 20; i++) {
        if (!dat_file_infos[i].is_used) {
            dat_file_infos[i].is_used = 1;
            found = i;
            break;
        }
    }

    NuThreadCriticalSectionEnd(file_criticalsection);

    return found;
}

i32 NuDatFileGetFreeHandleIX(NUDATHDR *hdr, i32 info_idx) {
    i32 file_idx;
    i32 i;

    file_idx = -1;

    NuThreadCriticalSectionBegin(file_criticalsection);

    for (i = 0; i < 20; i++) {
        if (hdr->open_files[i].info_idx == -1) {
            hdr->open_files[i].info_idx = info_idx;
            file_idx = i;
            break;
        }
    }

    NuThreadCriticalSectionEnd(file_criticalsection);

    return file_idx;
}

i32 NuDatFileOpenSize(NUFILE file) {
    NUDATFILEINFO *info;

    file -= 0x800;
    info = &dat_file_infos[file];

    if (info->compression_mode != 0) {
        return info->decompressed_len;

    } else {
        return info->file_len;
    }
}

i32 NuDatFileLoadBuffer(NUDATHDR *dat, char *name, void *dest, i32 buf_size) {
    NUFILE file;
    char *buf;

    nufile_lasterror = 0;

    file = NuDatFileOpen(dat, name, NUFILE_READ);

    if (file != 0) {
        i32 size;

        if (dat->mode == 3) {
            while (NuFileStatus(file) != 0) {
            }
        }

        if (dat_file_infos[file - 0x800].compression_mode != 0) {
            size = dat_file_infos[file - 0x800].decompressed_len;
        } else {
            size = dat_file_infos[file - 0x800].file_len;
        }

        buf = (char *)dest;

        if (size <= buf_size && size != 0) {
            LOG_DEBUG("Loading %d bytes from dat file", size);

            while (NuDatFileRead(file, buf, size) < 0) {
                while (NuDatFileSeek(file, 0, NUFILE_SEEK_START) < 0) {
                }
            }

            if (dat->mode == 3) {
                while (NuFileStatus(file) != 0) {
                }
            }

            NuFileClose(file);

            return size;
        }

        if (size != 0) {
            nufile_lasterror = -1;
        }

        NuFileClose(file);
    }

    return 0;
}

NUMEMFILE memfiles[20];

NUFILE NuMemFileOpen(void *buf, i32 buf_size, NUFILEMODE mode) {
    i32 i;

    if (buf_size > 0 && (mode == NUFILE_READ || mode == NUFILE_WRITE)) {
        for (i = 0; i < 20; i++) {
            if (!memfiles[i].used) {
                memfiles[i].buffer = (char *)buf;
                memfiles[i].end = (char *)buf + buf_size - 1;
                memfiles[i].ptr = memfiles[i].buffer;
                memfiles[i].mode = mode;
                memfiles[i].used = 1;

                return i + 0x400;
            }
        }
    }

    return 0;
}

void NuMemFileClose(NUFILE file) {
    if (file >= 0x2000) {
        do {
        } while (true);
    }

    if (file >= 0x800) {
        NuDatFileClose(file);
    } else {
        file -= 0x400;
        memfiles[file].used = 0;
    }
}

i32 NuMemFileRead(NUFILE file, void *buf, i32 size) {
    i32 remaining;

    if (file >= 0x800) {
        return NuDatFileRead(file, buf, size);
    }

    file -= 0x400;

    size = MIN(memfiles[file].end - memfiles[file].ptr + 1, size);

    if (size != 0) {
        memcpy(buf, memfiles[file].ptr, size);

        memfiles[file].ptr = memfiles[file].ptr + size;
    }

    return size;
}

i32 NuMemFileWrite(NUFILE file, void *data, i32 size) {
    file -= 0x400;

    size = MIN(memfiles[file].end - memfiles[file].ptr + 1, size);

    if (size != 0) {
        memcpy(memfiles[file].ptr, data, size);

        memfiles[file].ptr += size;
    }

    return size;
}

i64 NuMemFileSeek(NUFILE file, i64 offset, NUFILESEEK whence) {
    if (file >= 0x800) {
        return NuDatFileSeek(file, offset, whence);
    }

    file -= 0x400;

    switch (whence) {
        default:
            memfiles[file].ptr = memfiles[file].buffer + offset;
            break;
        case NUFILE_SEEK_CURRENT:
            memfiles[file].ptr = memfiles[file].ptr + offset;
            break;
        case NUFILE_SEEK_END:
            memfiles[file].ptr = memfiles[file].end - offset;
            break;
    }

    return (i64)(memfiles[file].ptr - memfiles[file].buffer);
}

i64 NuMemFilePos(NUFILE file) {
    if (file >= 0x800) {
        return NuDatFilePos(file);
    }

    file -= 0x400;

    return (i64)(memfiles[file].ptr - memfiles[file].buffer);
}

i32 NuMemFileOpenSize(NUFILE file) {
    file -= 0x400;

    return memfiles[file].end - memfiles[file].buffer;
}

void *NuMemFileAddr(NUFILE file) {
    file -= 0x400;

    return (void *)memfiles[file].ptr;
}

struct NUPPHEADER {
    u32 magic;
    u32 unpacked_size;
    u32 packed_size;
};

#define NUPP_SWAP32(value)                                                                                             \
    (((value) >> 24) | (((value) & 0x00ff0000) >> 8) | (((value) & 0x0000ff00) << 8) | ((value) << 24))

extern "C" void NuPPUnpack(void *source, void *destination);

extern "C" i32 NuPPGetSize(NUFILE file) {
    i32 size;
    i32 original_position;
    NUPPHEADER header;
    original_position = NuFileSeek(file, 0, NUFILE_SEEK_CURRENT);
    NuFileSeek(file, 0, NUFILE_SEEK_START);
    NuFileRead(file, &header, sizeof(header));
    if (header.magic == 0x02434e52) {
        header.unpacked_size = NUPP_SWAP32(header.unpacked_size);
        header.packed_size = NUPP_SWAP32(header.packed_size) + 6;
        size = header.unpacked_size;
    } else {
        size = NuFileOpenSize(file);
    }
    NuFileSeek(file, original_position, NUFILE_SEEK_START);
    return size;
}

i32 NuPPLoadBuffer(NUFILE file, void *buf, i32 buf_size) {
    i32 size;
    i32 unused = 0;
    i32 loaded;
    u8 *source;
    NUPPHEADER header;
    do {
        while (NuFileSeek(file, 0, NUFILE_SEEK_START) < 0) {
        }
        loaded = NuFileRead(file, &header, sizeof(header));
    } while (loaded < 0);
    if (header.magic == 0x02434e52) {
        header.unpacked_size = NUPP_SWAP32(header.unpacked_size);
        header.packed_size = NUPP_SWAP32(header.packed_size) + 18;
        source = static_cast<u8 *>(buf);
        source += header.unpacked_size + 256;
        source -= header.packed_size;
        do {
            while (NuFileSeek(file, 4, NUFILE_SEEK_START) < 0) {
            }
            loaded = NuFileRead(file, source, header.packed_size - 4);
        } while (loaded < 0);
        NuPPUnpack(source - 4, buf);
        size = header.unpacked_size;
    } else {
        size = NuFileOpenSize(file);
        do {
            while (NuFileSeek(file, 0, NUFILE_SEEK_START) < 0) {
            }
            loaded = NuFileRead(file, buf, size);
        } while (loaded < 0);
    }
    return size;
}
#undef NUPP_SWAP32

// The original decoder accesses both whole registers and their low bytes.
// Keep those views together to preserve its byte stores at the file's O0 setting.
union NUPPREG {
    u32 value;
    u16 word;
    u8 byte;
};
union NUPPADDR {
    uintptr_t value;
    u8 *bytes;
};
extern "C" void NuPPUnpack(void *source, void *destination) {
    NUPPREG last;
    NUPPREG carry;
    NUPPREG saved_byte;
    NUPPADDR dest;
    NUPPREG temp;
    NUPPREG upper;
    NUPPREG bits;
    NUPPREG count;
    NUPPREG distance;
    NUPPREG length;
    NUPPADDR copy;
    NUPPADDR saved;
    NUPPADDR input;
    NUPPADDR input_end;
    NUPPADDR output;
    NUPPADDR output_end;
    // The original reserves an aligned scratch area, stores the unpacked size
    // at word 64, and places a zero restoration sentinel immediately before it.
    u16 saved_stack[66] __attribute__((aligned(32)));
    copy.value = 0x0;
    length.value = copy.value;
    upper.value = length.value;
    temp.value = upper.value;
    dest.value = temp.value;
    saved_byte.value = dest.value;
    carry.value = saved_byte.value;
    last.value = carry.value;
    saved.value = (uintptr_t)(saved_stack + 64);
    // NuPPLoadBuffer supplies the header starting at byte four; magic is skipped.
    source = (u8 *)source + 4;
    copy.value = (uintptr_t)source;
    dest.value = (uintptr_t)destination;
    length.value = (u8)(*(u8 *)(copy.value));
    last.value = (u8)(*(u8 *)((copy.value + 0x1)));
    temp.value = (u8)(*(u8 *)((copy.value + 0x2)));
    carry.value = (u8)(*(u8 *)((copy.value + 0x3)));
    length.value = (length.value << 0x18);
    last.value = (last.value << 0x10);
    temp.value = (temp.value << 0x8);
    length.value = (length.value | last.value);
    length.value = (length.value | temp.value);
    length.value = (length.value | carry.value);
    copy.value = (copy.value + 0x4);
    *(u32 *)(saved.value) = length.value;
    input.value = copy.value;
    input.value = (input.value + 0xa);
    output.value = dest.value;
    output_end.value = output.value;
    output_end.value = (output_end.value + length.value);
    length.value = (u8)(*(u8 *)(copy.value));
    last.value = (u8)(*(u8 *)((copy.value + 0x1)));
    temp.value = (u8)(*(u8 *)((copy.value + 0x2)));
    carry.value = (u8)(*(u8 *)((copy.value + 0x3)));
    length.value = (length.value << 0x18);
    last.value = (last.value << 0x10);
    temp.value = (temp.value << 0x8);
    length.value = (length.value | last.value);
    length.value = (length.value | temp.value);
    length.value = (length.value | carry.value);
    copy.value = (copy.value + 0x4);
    input_end.value = input.value;
    input_end.value = (input_end.value + length.value);
    saved.value = (saved.value - 0x2);
    *(u16 *)(saved.value) = 0x0;
    // RNC method 2 interleaves control bits with literal and distance bytes.
    bits.value = 0xffffff00;
    carry.value = 0x1;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto next_token;
refill_bulk_length:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto bulk_length_accumulate;
refill_match_length_bit:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto match_length_accumulate;
refill_match_length_extension:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto match_length_extension;
refill_match_length_low:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto match_length_low;
refill_distance_prefix:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto distance_prefix;
refill_distance_high_bit:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto distance_high_bit;
refill_distance_extension:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto distance_extension;
refill_distance_low_bit:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto distance_low_accumulate;
start_bulk_length:;
    count.value = 0x3;
    goto bulk_length_bit;
bulk_length_next:;
bulk_length_bit:;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_bulk_length;
bulk_length_accumulate:;
    distance.value = (distance.value + distance.value);
    distance.value = (distance.value + carry.value);
    carry.value = (distance.value >> 0x10);
    carry.value = (carry.value & 0x1);
    count.value = (count.value - 0x1);
    if ((i32)(count.value) >= 0)
        goto bulk_length_next;
    distance.value = (distance.value + 0x2);
copy_bulk_literals:;
    last.value = (u8)(*(u8 *)(input.value));
    temp.value = (u8)(*(u8 *)((input.value + 0x1)));
    *(u8 *)(output.value) = last.byte;
    output.bytes[0x1] = temp.byte;
    last.value = (u8)(*(u8 *)((input.value + 0x2)));
    temp.value = (u8)(*(u8 *)((input.value + 0x3)));
    output.bytes[0x2] = last.byte;
    output.bytes[0x3] = temp.byte;
    input.value = (input.value + 0x4);
    distance.value = (distance.value - 0x1);
    output.value = (output.value + 0x4);
    if (!((i32)(distance.value) < 0)) {
        goto copy_bulk_literals;
    } else {
        goto token_after_bulk;
    }
decode_match_length:;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_match_length_bit;
match_length_accumulate:;
    count.value = (count.value + count.value);
    count.value = (count.value + carry.value);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_match_length_extension;
match_length_extension:;
    if (carry.value == 0)
        goto match_distance_from_length;
    count.value = (count.value - 0x1);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_match_length_low;
match_length_low:;
    count.value = (count.value + count.value);
    count.value = (count.value + carry.value);
    carry.value = (count.value >> 0x10);
    carry.value = (carry.value & 0x1);
    last.value = (count.value - 0x9);
    if (!(last.value == 0)) {
        goto decode_match_distance;
    } else {
        goto start_bulk_length;
    }
match_distance_from_length:;
    goto decode_match_distance;
match_distance_from_three:;
decode_match_distance:;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_distance_prefix;
distance_prefix:;
    if (carry.value == 0)
        goto distance_byte_from_zero;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_distance_high_bit;
distance_high_bit:;
    distance.value = (distance.value + distance.value);
    distance.value = (distance.value + carry.value);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_distance_extension;
distance_extension:;
    if (carry.value != 0)
        goto distance_extended_prefix;
    if (distance.value != 0)
        goto distance_pack_from_one;
    distance.value = (distance.value + 0x1);
    goto distance_low_bit;
distance_low_from_extension:;
distance_low_bit:;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_distance_low_bit;
distance_low_accumulate:;
    distance.value = (distance.value + distance.value);
    distance.value = (distance.value + carry.value);
    carry.value = (distance.value >> 0x10);
    carry.value = (carry.value & 0x1);
    goto distance_pack;
distance_pack_from_one:;
distance_pack:;
    last.value = (distance.value << 0x8);
    last.value = (last.value & 0xff00);
    distance.value = (distance.value >> 0x8);
    distance.value = (distance.value | last.value);
    goto read_distance_byte;
distance_byte_from_zero:;
    goto read_distance_byte;
distance_byte_from_pair:;
read_distance_byte:;
    upper.value = (distance.value & 0xff00);
    distance.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    distance.value = (distance.value | upper.value);
    copy.value = output.value;
    copy.value = (copy.value - distance.value);
    copy.value = (copy.value - 0x1);
    carry.value = (count.value & 0x1);
    count.value = (count.value >> 1);
    if (carry.value == 0)
        goto copy_pairs_even;
    last.value = (u8)(*(u8 *)(copy.value));
    copy.value = (copy.value + 0x1);
    *(u8 *)(output.value) = last.byte;
    output.value = (output.value + 0x1);
    goto copy_pairs_setup;
copy_pairs_even:;
copy_pairs_setup:;
    count.value = (count.value - 0x1);
    if (distance.value != 0)
        goto copy_back_reference_start;
    upper.value = (distance.value & 0xff00);
    distance.value = (u8)(*(u8 *)(copy.value));
    distance.value = (distance.value | upper.value);
copy_repeated_byte:;
    *(u8 *)(output.value) = distance.byte;
    count.value = (count.value - 0x1);
    output.bytes[0x1] = distance.byte;
    output.value = (output.value + 0x2);
    if (!((i32)(count.value) < 0)) {
        goto copy_repeated_byte;
    } else {
        goto token_after_repeat;
    }
copy_back_reference_start:;
copy_back_reference:;
    temp.value = (u8)(*(u8 *)((copy.value + 0x1)));
    last.value = (u8)(*(u8 *)(copy.value));
    output.bytes[0x1] = temp.byte;
    *(u8 *)(output.value) = last.byte;
    copy.value = (copy.value + 0x2);
    count.value = (count.value - 0x1);
    output.value = (output.value + 0x2);
    if (!((i32)(count.value) < 0)) {
        goto copy_back_reference;
    } else {
        goto token_after_copy;
    }
refill_token:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    if (!(carry.value != 0)) {
        goto literal_copy;
    } else {
        goto match_after_refill;
    }
literal_after_token:;
literal_copy:;
    last.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    *(u8 *)(output.value) = last.byte;
    output.value = (output.value + 0x1);
    goto next_token;
token_after_bulk:;
    goto next_token;
token_after_repeat:;
    goto next_token;
token_after_copy:;
    goto next_token;
token_after_block:;
next_token:;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    if (carry.value != 0)
        goto match_token_from_one;
    last.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    *(u8 *)(output.value) = last.byte;
    output.value = (output.value + 0x1);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    if (!(carry.value == 0)) {
        goto match_token_boundary;
    } else {
        goto literal_after_token;
    }
match_token_from_one:;
match_token_boundary:;
    bits.value = (bits.value & 0xff);
    if (!(bits.value == 0)) {
        goto decode_match;
    } else {
        goto refill_token;
    }
match_after_refill:;
decode_match:;
    count.value = 0x2;
    distance.value = 0x0;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_match_kind;
match_kind:;
    if (carry.value == 0)
        goto decode_match_length;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_short_kind;
match_short_kind:;
    if (carry.value == 0)
        goto distance_byte_from_pair;
    count.value = (count.value + 0x1);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_long_kind;
match_long_kind:;
    if (carry.value == 0)
        goto match_distance_from_three;
    count.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    if (count.value != 0)
        goto match_long_length;
    count.value = (count.value + 0x8);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (!(bits.value == 0)) {
        goto block_end;
    } else {
        goto refill_block_end;
    }
match_long_length:;
    count.value = (count.value + 0x8);
    goto decode_match_distance;
distance_extended_prefix:;
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_extended_bit;
distance_extended_bit:;
    distance.value = (distance.value + distance.value);
    distance.value = (distance.value + carry.value);
    carry.value = (distance.value >> 0x10);
    carry.value = (carry.value & 0x1);
    distance.value = (distance.value | 0x4);
    bits.value = (bits.value + bits.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    bits.value = (bits.value & 0xff);
    if (bits.value == 0)
        goto refill_extended_low;
distance_extended_low:;
    if (!(carry.value == 0)) {
        goto distance_pack;
    } else {
        goto distance_low_from_extension;
    }
refill_match_kind:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto match_kind;
refill_short_kind:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto match_short_kind;
refill_long_kind:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto match_long_kind;
refill_extended_bit:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto distance_extended_bit;
refill_extended_low:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
    goto distance_extended_low;
refill_block_end:;
    bits.value = (u8)(*(u8 *)(input.value));
    input.value = (input.value + 0x1);
    bits.value = (bits.value + bits.value);
    bits.value = (bits.value + carry.value);
    carry.value = (bits.value >> 0x8);
    carry.value = (carry.value & 0x1);
block_end:;
    if (carry.value != 0)
        goto token_after_block;
    length.value = (u16)(*(u16 *)(saved.value));
    saved.value = (saved.value + 0x2);
    if (length.value == 0)
        goto unpack_empty_return;
// Retained from the original, although this decoder only pushes a zero sentinel.
restore_saved_bytes:;
    saved_byte.value = (u16)(*(u16 *)(saved.value));
    saved.value = (saved.value + 0x2);
    *(u8 *)(output.value) = saved_byte.byte;
    output.value = (output.value + 0x1);
    length.value = (length.value - 0x1);
    if (!(length.value == 0)) {
        goto restore_saved_bytes;
    } else {
        goto unpack_return;
    }
unpack_empty_return:;
unpack_return:;
    return;
}

static FILEEXTINFO extensions[64];
static i32 num_extensions;

static i32 MatchExtension(char *extension, char *path_end, i32 path_len) {
    while (*extension != '\0') {
        --path_end;
        if (path_len == 0 || *extension != NuToLower(*path_end)) {
            return 0;
        }
        ++extension;
        --path_len;
    }
    return 1;
}

static FILEEXTINFO *NuFileExtGetInfo(char *path, i32 path_len) {
    if (path_len == 0) {
        path_len = NuStrLen(path);
    }

    FILEEXTINFO *info = extensions;
    while (info->extension[0] != '\0') {
        if (MatchExtension(info->extension, path + path_len, path_len) != 0) {
            return info;
        }
        ++info;
    }
    return NULL;
}

i32 NuFileExtGetType(char *path, i32 path_len) {
    FILEEXTINFO *info = NuFileExtGetInfo(path, path_len);
    if (info != NULL)
        return info->type;
    return NUFILETYPE_UNKNOWN;
}

i32 NuFileExtRemove(char *dest, char *path) {
    i32 length = NuStrCpy(dest, path);
    FILEEXTINFO *info = NuFileExtGetInfo(path, length);
    if (info != NULL) {
        dest[length - info->len] = '\0';
        return info->type;
    }
    return NUFILETYPE_UNKNOWN;
}

i32 NuFileExtGetExt(char *dest, i32 dest_size, NUFILETYPE type) {
    for (FILEEXTINFO *info = extensions; info->extension[0] != '\0'; ++info) {
        if (info->platform != PC_PLATFORM || info->type != type) {
            continue;
        }
        if (info->len > dest_size) {
            return 0;
        }

        const char *source = info->extension + info->len;
        for (i32 i = 0; i < info->len; ++i) {
            dest[i] = *--source;
        }
        dest[info->len] = '\0';
        return 1;
    }
    return 0;
}

i32 NuFileExtConvert(char *dest, char *path) {
    i32 path_len = NuStrCpy(dest, path);
    FILEEXTINFO *source = NuFileExtGetInfo(path, path_len);
    if (source == NULL) {
        return 0;
    }
    if (source->platform == PC_PLATFORM) {
        return 1;
    }

    for (FILEEXTINFO *target = extensions; target->extension[0] != '\0'; ++target) {
        if (target->platform == PC_PLATFORM && target->type == source->type) {
            char *out = dest + path_len - source->len + target->len;
            *out = '\0';
            for (char *extension = target->extension; *extension != '\0'; ++extension) {
                *--out = *extension;
            }
            return 1;
        }
    }
    return 0;
}

static void AddExtension(char *extension, i32 type, i32 platform) {
    char *ext;
    i32 len;
    FILEEXTINFO *next;

    next = &extensions[num_extensions];

    len = NuStrLen(extension);
    next->len = len;

    ext = next->extension;
    while (len != 0) {
        len--;
        *ext = extension[len];
        ext++;
    }

    *ext = '\0';

    next->type = type;
    next->platform = platform;
    num_extensions++;
}

static void NuFileExtInit(void) {
    static i32 first_time = 1;

    if (!first_time) {
        return;
    }

    first_time = 0;

    memset(extensions, 0, sizeof(extensions));
    num_extensions = 0;

    AddExtension(".ca3", NUFILETYPE_ANIM, X360_PLATFORM);
    AddExtension(".cbs", NUFILETYPE_BSANIM, X360_PLATFORM);
    AddExtension("_360.ghg", NUFILETYPE_CHARACTER, X360_PLATFORM);
    AddExtension("_360.gsc", NUFILETYPE_SCENE, X360_PLATFORM);
    AddExtension("_360.tex", NUFILETYPE_TEXTURE, X360_PLATFORM);
    AddExtension(".cc2", NUFILETYPE_CUTSCENE, X360_PLATFORM);
    AddExtension(".wavx", NUFILETYPE_SFX, X360_PLATFORM);
    AddExtension(".wavm", NUFILETYPE_SOUNDSTREAM, X360_PLATFORM);
    AddExtension(".wmv", NUFILETYPE_VIDEO, X360_PLATFORM);

    AddExtension(".ca3", NUFILETYPE_ANIM, PS3_PLATFORM);
    AddExtension(".cbs", NUFILETYPE_BSANIM, PS3_PLATFORM);
    AddExtension("_ps3.ghg", NUFILETYPE_CHARACTER, PS3_PLATFORM);
    AddExtension("_ps3.gsc", NUFILETYPE_SCENE, PS3_PLATFORM);
    AddExtension("_ps3.tex", NUFILETYPE_TEXTURE, PS3_PLATFORM);
    AddExtension(".cc2", NUFILETYPE_CUTSCENE, PS3_PLATFORM);
    AddExtension(".msf", NUFILETYPE_SFX, PS3_PLATFORM);
    AddExtension(".mib", NUFILETYPE_SOUNDSTREAM, PS3_PLATFORM);
    AddExtension(".pam", NUFILETYPE_VIDEO, PS3_PLATFORM);

    AddExtension(".an3", NUFILETYPE_ANIM, PC_PLATFORM);
    AddExtension(".bsa", NUFILETYPE_BSANIM, PC_PLATFORM);

#ifndef ANDROID
    AddExtension("_pc.ghg", NUFILETYPE_CHARACTER, PC_PLATFORM);
    AddExtension("_pc.gsc", NUFILETYPE_SCENE, PC_PLATFORM);
#endif

    AddExtension("_pc.dds", NUFILETYPE_TEXTURE, PC_PLATFORM);
    AddExtension(".cu2", NUFILETYPE_CUTSCENE, PC_PLATFORM);
    AddExtension(".wav", NUFILETYPE_SFX, PC_PLATFORM);
    AddExtension(".mib", NUFILETYPE_SOUNDSTREAM, PC_PLATFORM);
    AddExtension(".bik", NUFILETYPE_VIDEO, PC_PLATFORM);

#ifdef ANDROID
    AddExtension("_ios.gsc", NUFILETYPE_SCENE, PC_PLATFORM);
    AddExtension("_lr_ios.ghg", NUFILETYPE_CHARACTER, PC_PLATFORM);
#endif

    AddExtension(".an3", NUFILETYPE_ANIM, DEFAULT_PLATFORM);
    AddExtension(".bsa", NUFILETYPE_BSANIM, DEFAULT_PLATFORM);
    AddExtension(".ghg", NUFILETYPE_CHARACTER, DEFAULT_PLATFORM);
    AddExtension(".gsc", NUFILETYPE_SCENE, DEFAULT_PLATFORM);
    AddExtension(".pnt", NUFILETYPE_TEXTURE, DEFAULT_PLATFORM);
    AddExtension(".cu2", NUFILETYPE_CUTSCENE, DEFAULT_PLATFORM);
    AddExtension(".vag", NUFILETYPE_SFX, DEFAULT_PLATFORM);
    AddExtension(".mib", NUFILETYPE_SOUNDSTREAM, DEFAULT_PLATFORM);
    AddExtension(".pss", NUFILETYPE_VIDEO, DEFAULT_PLATFORM);

    AddExtension(".ca3", NUFILETYPE_ANIM, GAMECUBE_PLATFORM);
    AddExtension(".cbs", NUFILETYPE_BSANIM, GAMECUBE_PLATFORM);
    AddExtension(".chg", NUFILETYPE_CHARACTER, GAMECUBE_PLATFORM);
    AddExtension(".csc", NUFILETYPE_SCENE, GAMECUBE_PLATFORM);
    AddExtension(".ctx", NUFILETYPE_TEXTURE, GAMECUBE_PLATFORM);
    AddExtension(".cc2", NUFILETYPE_CUTSCENE, GAMECUBE_PLATFORM);
    AddExtension(".dsp", NUFILETYPE_SFX, GAMECUBE_PLATFORM);
    AddExtension(".gcm", NUFILETYPE_SOUNDSTREAM, GAMECUBE_PLATFORM);
    AddExtension(".h4m", NUFILETYPE_VIDEO, GAMECUBE_PLATFORM);

    AddExtension(".an3", NUFILETYPE_ANIM, XBOX_PLATFORM);
    AddExtension(".bsa", NUFILETYPE_BSANIM, XBOX_PLATFORM);
    AddExtension(".hx2", NUFILETYPE_CHARACTER, XBOX_PLATFORM);
    AddExtension(".nx2", NUFILETYPE_SCENE, XBOX_PLATFORM);
    AddExtension(".dds", NUFILETYPE_TEXTURE, XBOX_PLATFORM);
    AddExtension(".cu2", NUFILETYPE_CUTSCENE, XBOX_PLATFORM);
    AddExtension(".wavx", NUFILETYPE_SFX, XBOX_PLATFORM);
    AddExtension(".wavm", NUFILETYPE_SOUNDSTREAM, XBOX_PLATFORM);
    AddExtension(".wmv", NUFILETYPE_VIDEO, XBOX_PLATFORM);

    AddExtension(".an3", NUFILETYPE_ANIM, PSP_PLATFORM);
    AddExtension(".bsa", NUFILETYPE_BSANIM, PSP_PLATFORM);
    AddExtension(".phg", NUFILETYPE_CHARACTER, PSP_PLATFORM);
    AddExtension(".psc", NUFILETYPE_SCENE, PSP_PLATFORM);
    AddExtension(".pnt", NUFILETYPE_TEXTURE, PSP_PLATFORM);
    AddExtension(".cu2", NUFILETYPE_CUTSCENE, PSP_PLATFORM);
    AddExtension(".vag", NUFILETYPE_SFX, PSP_PLATFORM);
    AddExtension(".at3", NUFILETYPE_SOUNDSTREAM, PSP_PLATFORM);
    AddExtension(".pss", NUFILETYPE_VIDEO, PSP_PLATFORM);
}

i32 NuFileInitEx(i32 device_id, i32 reboot_iop, i32 eject) {
    NuPSFileInitDevices(device_id, reboot_iop, eject);

    memset(memfiles, 0, sizeof(memfiles));
    memset(dat_file_infos, 0, sizeof(dat_file_infos));

    file_criticalsection = NuThreadCreateCriticalSection();

    NuFileExtInit();
    NuDatFileDecodeInit();

    return device_id;
}

void NuFileInit(i32 device_id) {
    NuFileInitEx(device_id, 1, 0);
}
