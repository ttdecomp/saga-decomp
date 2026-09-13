#include "nu2api_nufile_types.h"
#include "nu2api/nucore/numemory.h"
#include "nu2api/nucore/nustring.h"
#include "nu2api/nufile/nufile.h"

#include <cstring>
#include <pthread.h>

NuFileDevice *NuFileDevice::sm_Devices[16];
i32 NuFileDevice::sm_NumDevices;
NuFileDevice *NuFileDevice::sm_DefaultDevice;
NuFileDevice *NuFileDevice::sm_HostDevice;
i32 NuFileDevice::sm_NumRules;
NuFileDevice::PathRule NuFileDevice::sm_Rules[32];
NuFileDevice::DirectoryHandle NuFileDevice::sm_DirectoryHandles[16];
pthread_mutex_t NuFileDevice::sm_CriticalSection;

void NuFileDevice::AddDevice(NuFileDevice *device) {
    device->device_id = sm_NumDevices;
    sm_Devices[sm_NumDevices++] = device;
}

void NuFileDevice::AddPathRule(NuFileDeviceType type, char const *path) {
    if (sm_NumRules >= 32)
        return;
    PathRule &rule = sm_Rules[sm_NumRules];
    rule.device_type = type;
    if (path) {
        char *copy = static_cast<char *>(NuMemoryGet()->GetThreadMem()->_BlockAlloc(
            std::strlen(path) + 1, 4, 4, __FILE__, 0));
        std::strcpy(copy, path);
        rule.path = copy;
    } else {
        rule.path = NULL;
    }
    rule.path_length = NuStrLen(path);
    ++sm_NumRules;
}

i32 NuFileDevice::AllocDirectoryHandle(char const *path) {
    pthread_mutex_lock(&sm_CriticalSection);
    i32 handle = 0;
    for (i32 i = 1; i < 16; ++i) {
        if (!sm_DirectoryHandles[i].device) {
            handle = i;
            break;
        }
    }
    if (handle) {
        DirectoryHandle &entry = sm_DirectoryHandles[handle];
        entry.device = this;
        if (path) {
            char *copy = static_cast<char *>(NuMemoryGet()->GetThreadMem()->_BlockAlloc(
                std::strlen(path) + 1, 4, 4, __FILE__, 0));
            std::strcpy(copy, path);
            entry.path = copy;
        } else {
            entry.path = NULL;
        }
    }
    pthread_mutex_unlock(&sm_CriticalSection);
    return handle;
}

void NuFileDevice::ClearPathRules() {
    sm_NumRules = 0;
}

NuFileBase *NuFileDevice::FileOpen(char const *path, NuFile::OpenMode::T mode) const {
    char formatted[1024];
    NuFileBase *file = NULL;
    if (path[0] && FormatName(formatted, sizeof(formatted), path))
        file = CreateNuFile(formatted, mode);
    return file;
}

i64 NuFileDevice::FileSize(char const *path) const {
    if (!path || !path[0])
        return -1;
    NuFileBase *file = FileOpen(path, NuFile::OpenMode::READ);
    if (!file)
        return -1;
    file->Seek(0, NuFile::SeekOrigin::END);
    i64 size = file->GetPos();
    file->Close();
    return size;
}

i32 NuFileDevice::FormatName(char *output, i32 size, char const *path) const {
    char relative[512] = "";
    output[0] = 0;
    char *normalized;
    if (((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':' &&
        (path[2] == '/' || path[2] == '\\')) {
        NuStrCat(output, path);
        normalized = output + 3;
    } else {
        const char *colon = path;
        for (i32 i = 0; *colon != ':' && *colon && i < 8; ++i)
            ++colon;
        if (*colon == ':')
            path = colon + 1;
        if (*mount_name > 0) {
            NuStrCat(output, mount_name);
            char *last = output + NuStrLen(mount_name) - 1;
            if (*last == '/' || *last == '\\')
                *last = 0;
        }
        i32 length = NuStrLen(output);
        normalized = output + length;
        if ((((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':' &&
             (path[2] == '/' || path[2] == '\\')) ||
            path[0] == '/' || path[0] == '\\') {
            NuStrCat(relative, path);
        } else {
            if (length > 0 && current_dir[0] != '\\' && current_dir[0] != '/')
                NuStrCat(relative, separator);
            NuStrCat(relative, current_dir);
            NuStrCat(relative, path);
        }
    }
    if (flags & 2)
        NuStrUpr(relative, relative);
    if (*relative)
        NuFileNormalise(normalized, size, relative);
    char *source = normalized;
    while (*source) {
        if (*source == '/' || *source == '\\') {
            *normalized++ = *separator;
            do {
                ++source;
            } while (*source == '/' || *source == '\\');
        } else {
            *normalized++ = *source++;
        }
    }
    *normalized = 0;
    return 1;
}

void NuFileDevice::FreeDirectoryHandle(i32 handle) {
    pthread_mutex_lock(&sm_CriticalSection);
    char *path = sm_DirectoryHandles[handle].path;
    sm_DirectoryHandles[handle].device = NULL;
    if (path)
        NuMemoryGet()->GetThreadMem()->BlockFree(path, 4);
    sm_DirectoryHandles[handle].path = NULL;
    pthread_mutex_unlock(&sm_CriticalSection);
}

NuFileDevice *NuFileDevice::GetDeviceByType(NuFileDeviceType type) {
    if (type == static_cast<NuFileDeviceType>(6))
        return sm_DefaultDevice;
    for (i32 i = 0; i < sm_NumDevices; ++i) {
        NuFileDevice *device = sm_Devices[i];
        if (device && device->device_type == type)
            return device;
    }
    return NULL;
}

NuFileDevice *NuFileDevice::GetDeviceFromDirectoryHandle(i32 handle) {
    return sm_DirectoryHandles[handle].device;
}

NuFileDevice *NuFileDevice::GetDeviceFromPath(char const *path) {
    if (((path[0] | 0x20) >= 'a' && (path[0] | 0x20) <= 'z') && path[1] == ':' &&
        (path[2] == '/' || path[2] == '\\'))
        return sm_DefaultDevice;
    if (path[0] == 'h' && path[1] == 'o' && path[2] == 's' && path[3] == 't' && path[4] == ':')
        return sm_HostDevice;

    for (i32 i = 0; i < 8; ++i) {
        if (path[i] == ':') {
            for (i32 j = 0; j < sm_NumDevices; ++j) {
                NuFileDevice *device = sm_Devices[j];
                if (device && *device->label &&
                    NuStrNICmp(path, device->label, NuStrLen(device->label)) == 0)
                    return device;
            }
            return NULL;
        }
    }

    NuFileDevice *device = sm_DefaultDevice;
    if (sm_NumRules > 0) {
        char normalized[512];
        NuFileNormalise(normalized, sizeof(normalized), path);
        for (i32 i = 0; i < sm_NumRules; ++i) {
            PathRule &rule = sm_Rules[i];
            if (NuStrNICmp(normalized, rule.path, rule.path_length) == 0)
                device = GetDeviceByType(rule.device_type);
        }
    }
    return device;
}

void NuFileDevice::Interrogate() {
    status = 1;
}

NuFileDevice::NuFileDevice() {
    device_type = NUFILE_DEVICE_UNKNOWN;
    flags = 0;
    status = 0;
    separator = "\\";
    label = "";
    current_dir[0] = 0;
    device_id = -1;
}

i32 NuFileDevice::QueryInstallProgress() {
    return 100;
}

void NuFileDevice::SetCurrentDir(char const *path) {
    char suffix[2] = "\\";
    if (!path || !*path) {
        current_dir[0] = 0;
        return;
    }
    i32 length = NuStrCpy(current_dir, path);
    if (length && path[length - 1] != '/' && path[length - 1] != '\\') {
        suffix[0] = *separator;
        NuStrCat(current_dir, suffix);
    }
}

void NuFileDevice::SetDefaultDevice(NuFileDeviceType type) {
    for (i32 i = 0; i < sm_NumDevices; ++i) {
        NuFileDevice *device = sm_Devices[i];
        if (device != NULL && device->device_type == type) {
            sm_DefaultDevice = device;
            return;
        }
    }
}

void NuFileDevice::SetLabel(char *name) {
    label = name;
}

void NuFileDevice::SetMountName(char *name) {
    mount_name = name;
}

NuFileDevice::~NuFileDevice() {
    if (device_id >= 0)
        sm_Devices[device_id] = NULL;
    if (this == sm_DefaultDevice)
        sm_DefaultDevice = NULL;
    if (this == sm_HostDevice)
        sm_HostDevice = NULL;
}
