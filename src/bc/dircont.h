#pragma once

#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdbool.h>
#include <dirent.h>

typedef struct dirent dirent;

// Double-linked forward list with fake node, counter and end pointer
// Push: O(1)
// Pop: O(1)
// Take random: O(n)
// Iterate over all: O(n)
typedef struct __dircont_entry {
	struct __dircont_entry *prev;
	struct __dircont_entry *next;
	struct dirent entry;
	//struct stat *stat; // TODO
} DIRCONT_ENTRY;

typedef struct __dircont {
	DIRCONT_ENTRY *head;
	DIRCONT_ENTRY *tail;
	DIRCONT_ENTRY *cur;
	__ino64_t ino_self;
	size_t count;
} DIRCONT;

void dcl_init(DIRCONT *dcl);
DIRCONT *dcl_create();
bool dcl_push_back(DIRCONT *dcl, struct dirent *entry);
void dcl_clear(DIRCONT *dcl);
struct dirent *dcl_next(DIRCONT *dcl);
struct dirent *dcl_next_r(DIRCONT *dcl, DIRCONT_ENTRY **cur);
DIRCONT_ENTRY *dcl_at(DIRCONT *dcl, size_t index);

void dcl_quick_sort(DIRCONT *dcl, int(*comparator)(DIRCONT_ENTRY*, DIRCONT_ENTRY*));
