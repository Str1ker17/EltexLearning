#pragma once

#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdbool.h>
#include <dirent.h>

// Single-linked forward list with end pointer
// Add: O(1)
typedef struct dircont {
	struct dircont *next;
	struct dirent entry;
	//struct stat *stat; // TODO
} DIRCONT_ENTRY;

typedef struct {
	DIRCONT_ENTRY *head;
	DIRCONT_ENTRY *tail;
	DIRCONT_ENTRY *cur;
	__ino64_t ino_self;
} DIRCONT;

void dcl_init(DIRCONT *dcl);
DIRCONT *dcl_create();
bool dcl_add(DIRCONT *dcl, struct dirent *entry);
void dcl_clear(DIRCONT *dcl);
struct dirent *dcl_next(DIRCONT *dcl);
