#pragma once

#ifdef _MSC_VER
#undef __cplusplus
#endif

// tell that we want to affair with large files
//#define _FILE_OFFSET_BITS 64
//#define _LARGEFILE64_SOURCE

#include <stddef.h>

#define COPYFILE_MODE(x) ((x) & 0xff)

// up to 256 modes
// internal modes
#define COPYFILE_STDIO 0
#define COPYFILE_FD 1
#define COPYFILE_SENDFILE 2
// external modes
#define COPYFILE_CP 128
//#define COPYFILE_DD 129

// valid only for readwrite
#define COPYFILE_FLAG_STDIO_UNLOCKED 0x100
#define COPYFILE_FLAG_STDIO_CHUNKS 0x200

// Current chunk (buffer) value for library
extern const size_t copyfile_chunk;

// Behave like syscall:
// 0 on success, -1 on error
extern int copyfile(const char *src, const char *dst, int mode, size_t *progress);
