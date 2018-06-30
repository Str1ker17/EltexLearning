#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dirpath.h"

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

bool dpt_move(DIRPATH *dpt, char *subdir) {
	if(strcmp(subdir, ".") == 0)
		return true; // already there
	if(strcmp(subdir, "..") == 0) {
		return dpt_up(dpt);
	}
	dirent entry;
	strncpy(entry.d_name, subdir, NAME_MAX);
	return dcl_push_back(dpt, &entry);
}

// DONE: this is O(1)
bool dpt_up(DIRPATH *dpt) {
	if(dpt->tail == NULL)
		return false; // nowhere to move
	DIRCONT_ENTRY *cur = dpt->tail;
	if(cur->prev != NULL)
		cur = cur->prev;

	// remove tail
	if(cur->next == NULL) {
		// single element
		dcl_clear(dpt);
	}
	else {
		free(dpt->tail);
		cur->next = NULL;
		dpt->tail = cur;
		dpt->count--;
	}
	return true;
}

char *dpt_string(DIRPATH *dpt, char *buf) {
	if(buf == NULL) {
		buf = (char*)malloc(PATH_MAX);
		if(buf == NULL)
			return NULL;
	}
	char *ptr = buf;

	int depth = 0;
	dirent *entry;
	while((entry = dcl_next(dpt)) != NULL) {
		// nice solution :)
		// https://www.guyrutenberg.com/2008/12/20/expanding-macros-into-string-constants-in-c
		
		ptr += snprintf(ptr, NAME_MAX + 2, "/%s", entry->d_name); // TODO: check for overflow
		depth++;
	}

	if(depth == 0) {
		strcpy(buf, "/");
	}

	return buf;
}

void dpt_init(DIRPATH *dpt, char *path) {
	dcl_init(dpt); // initialize from garbage
	char pth[PATH_MAX];
	strncpy(pth, path, PATH_MAX - 1);

	const char delim[] = "/";
	// Now we can start
	char *pch = strtok(pth, delim);
	// Check whether (pointer to) word exists
	while(pch != NULL) {
		// Print word only if it's LONGER
		if(strlen(pch) > 0) {
			dpt_move(dpt, pch);
		}
		// Get next word
		pch = strtok(NULL, delim);
	}
}

DIRPATH *dpt_create(char *path) {
	DIRPATH *dpt = dcl_create();
	if(dpt == NULL)
		return NULL;

	dpt_init(dpt, path);
	return dpt;
}