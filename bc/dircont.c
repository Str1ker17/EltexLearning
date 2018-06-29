#include <stdlib.h>
#include "dircont.h"

void dcl_init(DIRCONT *dcl) {
	dcl->head = NULL;
	dcl->cur = NULL;
	dcl->ino_self = 0;
}

DIRCONT *dcl_create() {
	DIRCONT *dcl = (DIRCONT*)malloc(sizeof(DIRCONT));
	if(dcl == NULL)
		return NULL;
	dcl_init(dcl);
	return dcl;
}

bool dcl_add(DIRCONT *dcl, struct dirent *entry) {
	DIRCONT_ENTRY *dce = (DIRCONT_ENTRY*)malloc(sizeof(DIRCONT_ENTRY));
	if(dce == NULL)
		return false;
	dce->entry = *entry;
	dce->next = NULL; // avoid loop

	if (dcl->head == NULL) {
		// add to empty list
		dcl->head = dce;
	}
	else {
		dcl->tail->next = dce;
	}

	dcl->tail = dce;

	return true;
}

void dcl_clear(DIRCONT *dcl) {
	DIRCONT_ENTRY *cur = dcl->head;
	while(cur != NULL) {
		DIRCONT_ENTRY *next = cur->next;
		free(cur);
		cur = next;
	}
	dcl_init(dcl);
}

struct dirent *dcl_next(DIRCONT *dcl) {
	if (dcl->cur == NULL) {
		dcl->cur = dcl->head;
	}
	else {
		dcl->cur = dcl->cur->next;
	}

	if(dcl->cur == NULL) {
		return NULL;
	}
	return &(dcl->cur->entry);
}