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
		dpt_up(dpt);
		return true;
	}
	dirent entry;
	strncpy(entry.d_name, subdir, NAME_MAX);
	return dcl_add(dpt, &entry);
}

// TODO: this is O(n), but may become O(1) after some work
void dpt_up(DIRPATH *dpt) {
	if(dpt->head == NULL)
		return; // nowhere to move
	DIRCONT_ENTRY *cur = dpt->head;
	while(cur->next != NULL && cur->next != dpt->tail)
		cur = cur->next;

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
}

char *dpt_string(DIRPATH *dpt, char *buf) {
	if(buf == NULL) {
		buf = (char*)malloc(PATH_MAX);
	}
	char *ptr = buf;

	int depth = 0;
	dirent *entry;
	while((entry = dcl_next(dpt)) != NULL) {
		// nice solution :)
		// https://www.guyrutenberg.com/2008/12/20/expanding-macros-into-string-constants-in-c
		ptr += snprintf(ptr, NAME_MAX + 1, "/%s", entry->d_name);
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