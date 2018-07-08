#pragma once

#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct __dircont_entry {
	struct dirent ent;
	struct stat st;
} DIRCONT_ENTRY;

// Double-linked list with fake node, counter and end pointer
// Push: O(1)
// Pop: O(1)
// Take random: O(n)
// Iterate over all: O(n)
typedef struct __dircont_list_entry {
	struct __dircont_list_entry *prev;
	struct __dircont_list_entry *next;
	DIRCONT_ENTRY value;
} DIRCONT_LIST_ENTRY;

typedef struct __dircont {
	DIRCONT_LIST_ENTRY *head;
	DIRCONT_LIST_ENTRY *tail;
	//DIRCONT_LIST_ENTRY *cur;
	__ino64_t ino_self;
	size_t count;
} DIRCONT;

void dcl_init(DIRCONT *dcl);
DIRCONT *dcl_create();
bool dcl_push_back(DIRCONT *dcl, DIRCONT_ENTRY *entry);
void dcl_clear(DIRCONT *dcl);
DIRCONT_ENTRY *dcl_next_r(DIRCONT *dcl, DIRCONT_LIST_ENTRY **cur);
DIRCONT_ENTRY *dcl_at(DIRCONT *dcl, size_t index);

void dcl_quick_sort(DIRCONT *dcl, int(*comparator)(DIRCONT_ENTRY*, DIRCONT_ENTRY*));
