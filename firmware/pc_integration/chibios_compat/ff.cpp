/*
 * Copyright (C) 2024 Mayhem PC Emulator Project
 *
 * FatFs Stub Implementation for PC Emulator
 * Provides minimal stub implementations - actual file I/O would use std::filesystem
 */

#ifdef PORTAPACK_PC_EMULATOR

#include "ff.h"
#include <ctime>

/* ============================================================================
 * Stub Function Implementations
 * These are minimal stubs that return success - actual implementations
 * would wrap std::filesystem for real file operations
 * ============================================================================ */

FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode) {
    (void)fp; (void)path; (void)mode;
    return FR_OK;
}

FRESULT f_close(FIL* fp) {
    (void)fp;
    return FR_OK;
}

FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br) {
    (void)fp; (void)buff; (void)btr;
    if (br) *br = 0;
    return FR_OK;
}

FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw) {
    (void)fp; (void)buff; (void)btw;
    if (bw) *bw = btw;
    return FR_OK;
}

FRESULT f_lseek(FIL* fp, FSIZE_t ofs) {
    (void)fp; (void)ofs;
    return FR_OK;
}

FRESULT f_truncate(FIL* fp) {
    (void)fp;
    return FR_OK;
}

FRESULT f_sync(FIL* fp) {
    (void)fp;
    return FR_OK;
}

FRESULT f_opendir(DIR* dp, const TCHAR* path) {
    (void)dp; (void)path;
    return FR_OK;
}

FRESULT f_closedir(DIR* dp) {
    (void)dp;
    return FR_OK;
}

FRESULT f_readdir(DIR* dp, FILINFO* fno) {
    (void)dp;
    if (fno) {
        fno->fname[0] = 0;  // Empty name signals end of directory
    }
    return FR_OK;
}

FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern) {
    (void)dp; (void)fno; (void)path; (void)pattern;
    if (fno) {
        fno->fname[0] = 0;
    }
    return FR_OK;
}

FRESULT f_findnext(DIR* dp, FILINFO* fno) {
    (void)dp;
    if (fno) {
        fno->fname[0] = 0;
    }
    return FR_OK;
}

FRESULT f_mkdir(const TCHAR* path) {
    (void)path;
    return FR_OK;
}

FRESULT f_unlink(const TCHAR* path) {
    (void)path;
    return FR_OK;
}

FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new) {
    (void)path_old; (void)path_new;
    return FR_OK;
}

FRESULT f_stat(const TCHAR* path, FILINFO* fno) {
    (void)path;
    if (fno) {
        fno->fsize = 0;
        fno->fdate = 0;
        fno->ftime = 0;
        fno->fattrib = 0;
        fno->fname[0] = 0;
    }
    return FR_NO_FILE;
}

FRESULT f_chmod(const TCHAR* path, BYTE attr, BYTE mask) {
    (void)path; (void)attr; (void)mask;
    return FR_OK;
}

FRESULT f_utime(const TCHAR* path, const FILINFO* fno) {
    (void)path; (void)fno;
    return FR_OK;
}

FRESULT f_chdir(const TCHAR* path) {
    (void)path;
    return FR_OK;
}

FRESULT f_chdrive(const TCHAR* path) {
    (void)path;
    return FR_OK;
}

FRESULT f_getcwd(TCHAR* buff, UINT len) {
    if (buff && len > 0) {
        buff[0] = '/';
        buff[1] = 0;
    }
    return FR_OK;
}

FRESULT f_getfree(const TCHAR* path, DWORD* nclst, FATFS** fatfs) {
    (void)path;
    if (nclst) *nclst = 1000000;  // Fake large free space
    if (fatfs) *fatfs = nullptr;
    return FR_OK;
}

FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt) {
    (void)fs; (void)path; (void)opt;
    return FR_OK;
}

/* RTC function - returns current time in FAT format */
DWORD get_fattime(void) {
    time_t t = time(nullptr);
    struct tm* tm = localtime(&t);

    DWORD fattime =
        ((DWORD)(tm->tm_year - 80) << 25) |
        ((DWORD)(tm->tm_mon + 1) << 21) |
        ((DWORD)tm->tm_mday << 16) |
        ((DWORD)tm->tm_hour << 11) |
        ((DWORD)tm->tm_min << 5) |
        ((DWORD)(tm->tm_sec / 2));

    return fattime;
}

#endif /* PORTAPACK_PC_EMULATOR */
