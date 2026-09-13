#ifndef NU2API_NUFILE_TYPES_H
#define NU2API_NUFILE_TYPES_H
#pragma once

#include "nu2api/nucore/common.h"
#include "nu2api/nufile/android/nufile_android.h"
#include <pthread.h>

struct NuFileAndroidAPK;
struct NuFileBase;
struct NuFileDevice;
struct NuFileDeviceAndroidAPK;
enum NuFileDeviceType { NUFILE_DEVICE_UNKNOWN = 1, NUFILE_DEVICE_ANDROID_APK = 3 };
struct nufile_info_s;

namespace NuFile {
    struct InitData {
        u32 flags;
        u32 unknown;
    };
} // namespace NuFile
struct NuFileDevice {
    static void AddDevice(NuFileDevice *);
    void AddPathRule(NuFileDeviceType, char const *);
    i32 AllocDirectoryHandle(char const *);
    static void ClearPathRules();
    void FreeDirectoryHandle(i32);
    static NuFileDevice *GetDeviceByType(NuFileDeviceType);
    static NuFileDevice *GetDeviceFromDirectoryHandle(i32);
    static NuFileDevice *GetDeviceFromPath(char const *);
    NuFileDevice();
    static void SetDefaultDevice(NuFileDeviceType);
    void SetLabel(char *);
    void SetMountName(char *);
    virtual NuFileBase *FileOpen(char const *, NuFile::OpenMode::T) const;
    virtual i64 FileSize(char const *) const;
    // The original base implementations report unsupported operations.
    virtual bool FileRename(char const *, char const *) {
        return false;
    }
    virtual bool FileGetInfo(char const *, nufile_info_s *) {
        return false;
    }
    virtual bool FileDelete(char const *) {
        return false;
    }
    virtual bool FileTouch(char const *) {
        return false;
    }
    virtual i32 DirOpen(char const *) {
        return 0;
    }
    virtual void DirClose(i32) {
    }
    virtual bool DirExists(char const *) {
        return false;
    }
    virtual bool DirRead(i32, nufile_info_s *) {
        return false;
    }
    virtual bool DirCreatePath(char const *) {
        return false;
    }
    virtual bool DirRemove(char const *) {
        return false;
    }
    virtual bool DirRemoveRecursive(char const *, bool) {
        return false;
    }
    virtual bool DirRename(char const *, char const *) {
        return false;
    }
    virtual bool GetPositionOnDisc(char const *, i64 &) const {
        return false;
    }
    virtual i32 FormatName(char *, i32, char const *) const;
    virtual void SetCurrentDir(char const *);
    virtual ~NuFileDevice();
    virtual void Interrogate();
    virtual i32 QueryInstallProgress();
    virtual NuFileBase *CreateNuFile(char const *, NuFile::OpenMode::T) const = 0;

    static NuFileDevice *sm_Devices[16];
    static i32 sm_NumDevices;
    static NuFileDevice *sm_DefaultDevice;
    static NuFileDevice *sm_HostDevice;
    static i32 sm_NumRules;
    struct PathRule {
        NuFileDeviceType device_type;
        char *path;
        i32 path_length;
    };
    static PathRule sm_Rules[32];
    struct DirectoryHandle {
        NuFileDevice *device;
        char *path;
    };
    static DirectoryHandle sm_DirectoryHandles[16];
    static pthread_mutex_t sm_CriticalSection;

  protected:
    i32 device_id;
    NuFileDeviceType device_type;
    u32 flags;
    i32 status;
    const char *separator;
    const char *label;
    const char *mount_name;
    char current_dir[128];
};
struct NuFileDeviceAndroidAPK : NuFileDevice {
    virtual NuFileBase *CreateNuFile(char const *, NuFile::OpenMode::T) const override;
    NuFileDeviceAndroidAPK(char const *, NuFile::InitData const &);
    virtual ~NuFileDeviceAndroidAPK();
};

DECOMP_ASSERT(sizeof(NuFileDevice) == 0xa0, "NuFileDevice target layout");
DECOMP_ASSERT(sizeof(NuFileDeviceAndroidAPK) == 0xa0, "APK device target layout");

#endif // NU2API_NUFILE_TYPES_H
