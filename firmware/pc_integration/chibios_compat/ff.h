/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * FatFs Stub for PC Emulator
 * Provides minimal types and function stubs to satisfy include requirements.
 * Actual filesystem operations use standard C++ filesystem on PC.
 */

#ifndef _FATFS
#define _FATFS  68300

#ifdef __cplusplus
extern "C" {
#endif

#include <cstdint>

/* ============================================================================
 * Integer Types (from integer.h)
 * ============================================================================ */

typedef int             INT;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef short           SHORT;
typedef unsigned short  WORD;
typedef unsigned short  WCHAR;
typedef long            LONG;
typedef unsigned long   DWORD;
typedef unsigned long long QWORD;

/* ============================================================================
 * FatFs Configuration Constants (from ffconf.h)
 * ============================================================================ */

#define _FFCONF         68300
#define _FS_READONLY    0
#define _FS_MINIMIZE    0
#define _USE_STRFUNC    2
#define _USE_FIND       1
#define _USE_MKFS       0
#define _USE_FASTSEEK   1
#define _USE_EXPAND     0
#define _USE_CHMOD      0
#define _USE_LABEL      0
#define _USE_FORWARD    0
#define _CODE_PAGE      437
#define _USE_LFN        2
#define _MAX_LFN        255
#define _LFN_UNICODE    1
#define _STRF_ENCODE    3
#define _FS_RPATH       0
#define _VOLUMES        1
#define _STR_VOLUME_ID  0
#define _MULTI_PARTITION 0
#define _MIN_SS         512
#define _MAX_SS         512
#define _USE_TRIM       0
#define _FS_NOFSINFO    0
#define _FS_TINY        0
#define _FS_EXFAT       0
#define _FS_NORTC       0
#define _NORTC_MON      1
#define _NORTC_MDAY     1
#define _NORTC_YEAR     2017
#define _FS_LOCK        0
#define _FS_REENTRANT   0
#define _USE_BUFF_WO_ALIGNMENT 0

/* ============================================================================
 * Type Definitions
 * ============================================================================ */

/* Type of path name strings */
typedef WCHAR TCHAR;
#define _T(x) L ## x
#define _TEXT(x) L ## x

/* File size type */
typedef DWORD FSIZE_t;

/* File system object structure (FATFS) - minimal stub */
typedef struct {
    BYTE    fs_type;
    BYTE    drv;
    WORD    id;
    WORD    csize;
    DWORD   n_fatent;
    DWORD   fsize;
    DWORD   volbase;
    DWORD   fatbase;
    DWORD   dirbase;
    DWORD   database;
    DWORD   winsect;
    BYTE    win[_MAX_SS];
} FATFS;

/* Object ID structure */
typedef struct {
    FATFS*  fs;
    WORD    id;
    BYTE    attr;
    BYTE    stat;
    DWORD   sclust;
    FSIZE_t objsize;
} _FDID;

/* File object structure (FIL) */
typedef struct {
    _FDID   obj;
    BYTE    flag;
    BYTE    err;
    FSIZE_t fptr;
    DWORD   clust;
    DWORD   sect;
    DWORD   dir_sect;
    BYTE*   dir_ptr;
    DWORD*  cltbl;
    BYTE    buf[_MAX_SS];
} FIL;

/* Directory object structure (DIR) */
typedef struct {
    _FDID   obj;
    DWORD   dptr;
    DWORD   clust;
    DWORD   sect;
    BYTE*   dir;
    BYTE    fn[12];
    DWORD   blk_ofs;
    const TCHAR* pat;
} DIR;

/* File information structure (FILINFO) */
typedef struct {
    FSIZE_t fsize;
    WORD    fdate;
    WORD    ftime;
    BYTE    fattrib;
    TCHAR   altname[13];
    TCHAR   fname[_MAX_LFN + 1];
} FILINFO;

/* File function return code (FRESULT) */
typedef enum {
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT,
    FR_LOCKED,
    FR_NOT_ENOUGH_CORE,
    FR_TOO_MANY_OPEN_FILES,
    FR_INVALID_PARAMETER
} FRESULT;

/* File attribute bits */
#define AM_RDO  0x01    /* Read only */
#define AM_HID  0x02    /* Hidden */
#define AM_SYS  0x04    /* System */
#define AM_VOL  0x08    /* Volume label */
#define AM_LFN  0x0F    /* LFN entry */
#define AM_DIR  0x10    /* Directory */
#define AM_ARC  0x20    /* Archive */
#define AM_MASK 0x3F    /* Mask of defined bits */

/* File access mode and open method flags */
#define FA_READ             0x01
#define FA_WRITE            0x02
#define FA_OPEN_EXISTING    0x00
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA_OPEN_APPEND      0x30

/* ============================================================================
 * Function Stubs (PC implementations would use std::filesystem)
 * ============================================================================ */

FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode);
FRESULT f_close(FIL* fp);
FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br);
FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw);
FRESULT f_lseek(FIL* fp, FSIZE_t ofs);
FRESULT f_truncate(FIL* fp);
FRESULT f_sync(FIL* fp);
FRESULT f_opendir(DIR* dp, const TCHAR* path);
FRESULT f_closedir(DIR* dp);
FRESULT f_readdir(DIR* dp, FILINFO* fno);
FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);
FRESULT f_findnext(DIR* dp, FILINFO* fno);
FRESULT f_mkdir(const TCHAR* path);
FRESULT f_unlink(const TCHAR* path);
FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new);
FRESULT f_stat(const TCHAR* path, FILINFO* fno);
FRESULT f_chmod(const TCHAR* path, BYTE attr, BYTE mask);
FRESULT f_utime(const TCHAR* path, const FILINFO* fno);
FRESULT f_chdir(const TCHAR* path);
FRESULT f_chdrive(const TCHAR* path);
FRESULT f_getcwd(TCHAR* buff, UINT len);
FRESULT f_getfree(const TCHAR* path, DWORD* nclst, FATFS** fatfs);
FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt);

/* Macro functions */
#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj.objsize))
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj.objsize)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)

#ifndef EOF
#define EOF (-1)
#endif

/* RTC function */
DWORD get_fattime(void);

#ifdef __cplusplus
}
#endif

#endif /* _FATFS */
