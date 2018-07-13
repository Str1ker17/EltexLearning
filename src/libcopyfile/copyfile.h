#pragma once

#ifdef _MSC_VER
#undef __cplusplus
#endif

#define COPYFILE_MODE(x) ((x) & 0xff)

// up to 256 modes
#define COPYFILE_READWRITE_BYTES 0
#define COPYFILE_READWRITE_CHUNKS 1
#define COPYFILE_SENDFILE 2

// valid only for readwrite
#define COPYFILE_USE_UNLOCKED 0x100
#define COPYFILE_USE_FD 0x200

// Behave like syscall:
// 0 on success, -1 on error
int copyfile(const char *src, const char *dst, int mode);
