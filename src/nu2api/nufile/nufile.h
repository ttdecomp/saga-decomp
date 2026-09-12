#pragma once

#include "nu2api/nucore/common.h"

typedef i32 NUFILE;

// Only the timestamp portion is recovered so far. The preceding metadata
// and byte at 0x15 are not interpreted by NuFileIsNewer.
typedef struct nufile_info_s {
    u8 unknown_00[16];
    i8 second;
    i8 minute;
    i8 hour;
    i8 day;
    i8 month;
    u8 unknown_15;
    i16 year;
} NUFILE_INFO;

typedef enum nufilemode_e {
    NUFILE_READ = 0,
    NUFILE_WRITE = 1,
    NUFILE_APPEND = 2,
    NUFILE_READ_NOWAIT = 3,
    NUFILE_READWRITE = 4,
    NUFILE_MODE_CNT = 5,
} NUFILEMODE;

typedef enum {
    NUFILETYPE_UNKNOWN = -1,
    NUFILETYPE_ANIM = 0,
    NUFILETYPE_BSANIM = 1,
    NUFILETYPE_CHARACTER = 2,
    NUFILETYPE_SCENE = 3,
    NUFILETYPE_TEXTURE = 4,
    NUFILETYPE_CUTSCENE = 5,
    NUFILETYPE_SFX = 6,
    NUFILETYPE_SOUNDSTREAM = 7,
    NUFILETYPE_VIDEO = 8,
    NUFILETYPE_CNT = 9,
} NUFILETYPE;

typedef struct filebuff_s {
    struct fileinfo_s *file_info;
    i32 time;
    char data[1024];
} FILEBUFF;

typedef struct fileinfo_s {
    i32 handle;
    i64 read_pos;
    i64 file_pos;
    i64 file_len;
    i64 buf_start;
    i32 buf_len;
    i32 use_buf;
    i32 unused;
    i32 mode;
    FILEBUFF *buf;
} FILEINFO;

struct nufile_device_s;

typedef i32 nufiledevFormatName(struct nufile_device_s *, char *, char *, i32);
typedef i32 nufiledevInterrogate(struct nufile_device_s *);

enum NUFILE_DEVICE_STATUS {
    NUFILE_DEVICE_STATUS_EXCLUDE_FROM_ENUMERATION = 1 << 1,
};

typedef struct nufile_device_s {
    i32 id;
    i32 unit_id;
    i32 attr;
    i32 status;
    char dir_separator;

    i32 max_name_size;
    i32 max_ext_size;
    i32 max_dir_entries;

    i32 free_space;
    i32 params[4];
    char name[16];
    i32 match_len;
    i32 name_len;
    char root[32];
    char desc[64];
    char cur_dir[128];
    char sys_dir[128];
    char dll_dir[128];

    nufiledevFormatName *format_name_fn;
    nufiledevInterrogate *interrogate_fn;
} NUFILE_DEVICE;

typedef struct numemfile_s {
    char *buffer;
    char *end;
    char *ptr;
    NUFILEMODE mode;
    i32 used;
} NUMEMFILE;

typedef enum nufileseek_e {
    NUFILE_SEEK_START = 0,
    NUFILE_SEEK_CURRENT = 1,
    NUFILE_SEEK_END = 2,
} NUFILESEEK;

typedef struct nudatinfo_s {
    i32 file_offset;
    i32 file_len;
    i32 decompressed_len;
    i32 compression_mode;
} NUDATFINFO;

typedef struct nudfnodev1_s {
    i16 child_idx;
    i16 sibling_idx;
    char *name;
} NUDFNODE_V1;

typedef struct nudfnodev2_s {
    i16 child_idx;
    i16 sibling_idx;
    char *name;
    i16 unknown;
    i16 unknown2;
} NUDFNODE;

typedef struct nudatopenfileinfo_s {
    NUFILE dat_file;
    i32 info_idx;
    i64 pos;
} NUDATOPENFILEINFO;

typedef struct nudathdr_s {
    i32 version;
    i32 file_count;
    NUDATFINFO *file_info;
    i32 tree_node_count;
    NUDFNODE *file_tree;
    i32 leaf_names_len;
    char *leaf_names;
    u32 *hash_idxs;
    i32 hash_count;
    i32 hashes_len;
    char *hashes;
    NUDATOPENFILEINFO open_files[20];
    i16 unknown;
    i16 mode;
    i32 unknown2;
    char *name;
} NUDATHDR;

typedef struct nudatfileinfo_s {
    NUDATHDR *hdr;
    i64 start;
    i64 pos;
    i32 file_len;
    i32 decompressed_len;
    i32 open_file_idx;
    i32 is_used;
    i32 compression_mode;
} NUDATFILEINFO;

typedef i32 NUPSFILE;

enum NUFILE_OFFSETS {
    NUFILE_OFFSET_PS = 1,
    NUFILE_OFFSET_MEM = 0x400,
    NUFILE_OFFSET_DAT = 0x800,
    NUFILE_OFFSET_MC = 0x1000,
    NUFILE_OFFSET_NATIVE = 0x2000,
};

typedef struct fileextinfo_s {
    char extension[13];
    char type;
    char platform;
    char len;
} FILEEXTINFO;

extern i32 read_critical_section;

#ifdef __cplusplus
namespace NuFile {
    namespace SeekOrigin {
        enum T {
            START = 0,
            CURRENT = 1,
            END = 2,
        };
    };

    namespace OpenMode {
        enum T {
            READ = 0,
            WRITE = 1,
        };
    }

    class IFile {
      public:
        // Return types uncertain.
        // NOLINTBEGIN
        virtual i32 GetCapabilities() const {
            return {};
        }

        virtual const char *GetFilename() const {
            return {};
        }

        virtual u32 GetType() const {
            return {};
        }

        virtual i64 Seek(i64 offset, SeekOrigin::T) {
            return {};
        }

        virtual isize Read(void *buf, usize size) {
            return {};
        }

        virtual isize Write(const void *buf, usize size) {
            return {};
        }

        virtual i64 GetPos() const {
            return {};
        }

        virtual i64 GetSize() const {
            return {};
        }

        virtual void Close() {
        }

        virtual void Flush() {
        }
        // NOLINTEND
    };
}; // namespace NuFile

class NuFileBase : public NuFile::IFile {
  public:
    NuFileBase(const char *filepath, NuFile::OpenMode::T mode, u32 type);

    virtual i32 GetCapabilities() const override;
    virtual const char *GetFilename() const override;
    virtual u32 GetType() const override;

    virtual i64 GetPos() const override;
    virtual i64 GetSize() const override;

    virtual void Flush() override;

    void Closedown();
    void Init();

  protected:
    virtual ~NuFileBase();

  private:
    // Type uncertain.
    i32 unknown;

    u32 type;
    NuFile::OpenMode::T mode;

    char filepath[0x100];
};

extern "C" {
    // Returns one when the second record has the later timestamp.
    i32 NuFileIsNewer(NUFILE_INFO *first, NUFILE_INFO *second);
    // Alignment is specified as a bit mask (for example, 15 for 16-byte alignment).
    i32 NuFileAlign(NUFILE file, i32 mask);
    i32 NuFileAlignRead(NUFILE file, i32 mask);
    void *NuFileLoad(char *path);
    i32 NuFileCopy(char *dest, char *source);
    i32 NuFileCopyEx(char *dest, char *source, void *buffer, i32 capacity);
    i32 NuFileExistQuiet(char *filepath);
    void NuFileBeginBlkWrite(NUFILE file, i32 tag, i32 negative_size);
    void NuFileEndBlkWrite(NUFILE file);
    i32 NuFileBeginBlkRead(NUFILE file, i32 expected_tag);
    void NuFileEndBlkRead(NUFILE file);
    i32 NuFileGetBlkSize(void);
    void NuFileInitAddress(i32 capacity);
    void NuFileTidyAddress(void);
    void NuFileWriteAddress(NUFILE file, void *address);
    void NuFileSetAddress(NUFILE file, void *address);
    void NuFilePatchAddress(NUFILE file);
    void NuFileGetCurrentDllPath(char *dest);
    i32 NuFileGetCurrentPath(char *dest);
    i32 NuFileGetCurrentSysPath(char *dest);
    void NuFileSetCurrentDirectory(char *path);
    void NuFileSetCurrentSysDirectory(char *path);
    NUFILE_DEVICE *NuFileFindDevice(i32 id, i32 unit);
    i32 NuFileEnumerateDevices(NUFILE_DEVICE **result);
    i32 NuFileRefreshDevices(NUFILE_DEVICE **result);
    i32 NuFileGetDevices(NUFILE_DEVICE **result);
    i32 NuFileGetCurrentDirectory(char *dest);
    i32 NuFileFormatName(char *dest, char *name, i32 capacity);
    void NuFileSetAppDirectory(char *path);
    void NuFileGetAppDirectory(char *dest);
    i32 NuMcCheckCardPresent(i32 port, i32 slot);
    i32 NuMcCheckCardFormatted(i32 port, i32 slot);
    i32 NuMcCheckCardFreeSpace(i32 port, i32 slot);
    i32 NuFileAppendPath(char *dest, char *path, char *name);
    i32 NuFileExtractFile(char *dest, char *path);
    i32 NuFileExtractFilename(char *dest, char *path);
    i32 NuFileExtractPath(char *dest, char *path);
    i32 NuFileExtractExt(char *dest, char *path);
#endif
    extern char g_datfileMode;
    extern i32 NuFile_SwapEndianOnWrite;
    extern NUFILE_DEVICE *default_device;

    i32 DEV_FormatName(NUFILE_DEVICE *device, char *formatted_name, char *path, i32 buf_size);
    i32 DEVHOST_Interrogate(NUFILE_DEVICE *device);
    void NuFileSetBadGameDisc(void);
    i32 NuFileCheckBadGameDiscStatus(void);

    void NuFileCorrectSlashes(NUFILE_DEVICE *device, char *path);
    void NuFileReldirFix(NUFILE_DEVICE *device, char *path);

    // NuFile stuff
    NUFILE NuFileOpen(char *filepath, NUFILEMODE mode);
    void NuFileClose(NUFILE file);
    i32 NuFileStatus(NUFILE file);
    NUFILE NuFileOpenDF(char *filepath, NUFILEMODE mode, NUDATHDR *header, i32 _unused);
    i32 NuFileRead(NUFILE file, void *buf, i32 size);
    i32 NuFileWrite(NUFILE file, void *data, i32 size);
    void NuFileWriteString(NUFILE file, const char *text);
    i32 NuFileWriteStringV(NUFILE file, const char *format, ...);
    NUFILE_DEVICE *NuFileGetDeviceFromPath(char *path);
    i32 NuFileFormat(char *path);
    i32 NuMcFormat(i32 port, i32 slot);
    i64 NuFileOpenSize(NUFILE file);
    i64 NuFileSeek(NUFILE file, i64 offset, NUFILESEEK seekMode);
    i32 NuFileLoadBuffer(char *filepath, void *buf, i32 buf_size);
    i32 NuFileLoadBufferVP(char *filepath, VARIPTR *buf, VARIPTR *buf_end);
    i32 NuFileGetLastError(void);
    i32 NuFileGetMediaMode(void);
    NUFILE_DEVICE *NuFileGetCurrentDevice(void);
    i32 NuFileGetEndianSwap(void);
    void NuFileSetCurrentDevice(NUFILE_DEVICE *device);
    i32 NuFileExists(char *name);
    i64 NuFileSize(char *filepath);
    i32 NuFileExtConvert(char *dest, char *path);
    i32 NuFileExtGetExt(char *dest, i32 dest_size, NUFILETYPE type);
    i32 NuFileExtGetType(char *path, i32 path_len);
    i32 NuFileExtRemove(char *dest, char *path);
    i32 NuFileEOF(NUFILE file);
    i64 NuFilePos(NUFILE file);
    void NuFileUpCase(NUFILE_DEVICE *device, char *filepath);

    // read types
    i8 NuFileReadChar(NUFILE file);
    i32 NuFileReadDir(NUFILE file);
    f32 NuFileReadFloat(NUFILE file);
    i32 NuFileReadInt(NUFILE file);
    i16 NuFileReadShort(NUFILE file);
    u8 NuFileReadUnsignedChar(NUFILE file);
    u32 NuFileReadUnsignedInt(NUFILE file);
    u16 NuFileReadUnsignedShort(NUFILE file);
    u16 NuFileReadWChar(NUFILE file);

    i32 NuFileWriteInt(NUFILE file, i32 value);
    i32 NuFileWriteFloat(NUFILE file, float value);
    i32 NuFileWriteShort(NUFILE file, i16 value);
    i32 NuFileWriteUnsignedShort(NUFILE file, u16 value);
    i32 NuFileWriteChar(NUFILE file, i8 value);
    i32 NuFileWriteUnsignedChar(NUFILE file, u8 value);
    i32 NuFileSwapEndianOnWrite(i32 enabled);
    void NuFileSetCurrentDllDirectory(char *path);
    u32 NuFileWriteUnsignedInt(NUFILE file, u32 value);

    // Platform-specific file functions
    i32 NuGetFileHandlePS(void);
    i32 NuPSFileOpen(char *filepath, NUFILEMODE mode);
    i32 NuPSFileClose(i32 index);
    i32 NuPSFileRead(i32 index, void *dest, i32 len);
    i32 NuPSFileWrite(i32 index, const void *src, i32 len);
    i64 NuPSFileLSeek(i32 index, i64 offset, NUFILESEEK whence);

    // Memory file functions
    NUFILE NuMemFileOpen(void *buf, i32 buf_size, NUFILEMODE mode);
    void NuMemFileClose(NUFILE file);
    i32 NuMemFileRead(NUFILE file, void *buf, i32 size);
    i32 NuMemFileWrite(NUFILE file, void *data, i32 size);
    i64 NuMemFileSeek(NUFILE file, i64 offset, NUFILESEEK whence);
    i64 NuMemFilePos(NUFILE file);
    void *NuMemFileAddr(NUFILE file);

    // Memory card functions
    i32 NuMcOpen(i32 port, i32 slot, char *filepath, i32 mode, i32 async);
    i32 NuMcClose(i32 fd, i32 async);
    i32 NuMcSeek(i32 fd, i32 offset, NUFILESEEK mode, i32 async);
    i32 NuMcOpenSize(i32 fd);
    i32 NuMcRead(i32 fd, void *buf, i32 size, i32 async);
    i32 NuMcWrite(i32 fd, void *data, i32 size, i32 async);

    // NuDat functions
    NUDATHDR *NuDatOpen(char *filepath, VARIPTR *buf, i32 *_unused);
    NUDATHDR *NuDatOpenEx(char *filepath, VARIPTR *buf, i32 *_unused, i16 mode);
    void NuDatFileClose(NUFILE file);
    NUDATHDR *NuDatSet(NUDATHDR *header);
    i32 NuDatFileOpenSize(NUFILE file);

    // NuDatFile functions
    NUFILE NuDatFileOpen(NUDATHDR *hdr, char *filepath, NUFILEMODE mode);
    i64 NuDatFilePos(NUFILE file);
    i32 NuDatFileRead(NUFILE file, void *buf, i32 size);
    i64 NuDatFileSeek(NUFILE file, i64 offset, NUFILESEEK whence);
    i32 NuDatFileFindTree(NUDATHDR *header, char *name);
    i32 NuDatFileLoadBuffer(NUDATHDR *dat, char *name, void *dest, i32 maxSize);

    i32 NuPPGetSize(NUFILE file);
    i32 NuPPLoadBuffer(NUFILE file, void *buf, i32 buf_size);
    void NuPPUnpack(void *source, void *destination);

    void NuPSFileInitDevices(i32 device_id, i32 reboot_iop, i32 eject);
    i32 NuFileInitEx(i32 device_id, i32 reboot_iop, i32 eject);
    void NuFileInit(i32 device_id);

#ifdef __cplusplus
}

i32 NuFileNormalise(char *dst, i32 length, const char *src);
#endif

i32 NuMemFileOpenSize(NUFILE file);
i32 NuMcFileOpenSize(NUFILE file);

#ifdef __cplusplus

i64 NuDatCalcPos(NUDATHDR *header, i32 index);

i32 NuDatFileFindHash(NUDATHDR *header, char *name);
i32 NuDatFileGetFreeInfo(void);
i32 NuDatFileGetFreeHandleIX(NUDATHDR *hdr, i32 info_idx);

#endif
