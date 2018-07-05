#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "dircont.h"

void dcl_init(DIRCONT *dcl) {
	dcl->head = NULL;
	dcl->tail = NULL;
	//dcl->cur = NULL;
	dcl->ino_self = 0;
	dcl->count = 0;
}

DIRCONT *dcl_create() {
	DIRCONT *dcl = (DIRCONT*)malloc(sizeof(DIRCONT));
	if(dcl == NULL)
		return NULL;
	dcl_init(dcl);
	return dcl;
}

// O(1)
bool dcl_push_back(DIRCONT *dcl, DIRCONT_ENTRY *entry) {
	DIRCONT_LIST_ENTRY *dce = (DIRCONT_LIST_ENTRY*)malloc(sizeof(DIRCONT_LIST_ENTRY));
	if(dce == NULL)
		return false;
	dce->value = *entry;
	dce->next = NULL; // avoid loop

	if (dcl->head == NULL) {
		// add to empty list
		dce->prev = NULL;
		dcl->head = dce;
	}
	else {
		dce->prev = dcl->tail;
		dcl->tail->next = dce;
	}

	dcl->tail = dce;
	++(dcl->count);
	return true;
}

// O(n)
void dcl_clear(DIRCONT *dcl) {
	DIRCONT_LIST_ENTRY *cur = dcl->head;
	while(cur != NULL) {
		DIRCONT_LIST_ENTRY *next = cur->next;
		free(cur);
		cur = next;
	}
	dcl_init(dcl);
}

// Single take, O(1)
/*DIRCONT_ENTRY *dcl_next(DIRCONT *dcl) {
	if (dcl->cur == NULL) {
		dcl->cur = dcl->head;
	}
	else {
		dcl->cur = dcl->cur->next;
	}

	if(dcl->cur == NULL) {
		return NULL;
	}
	return &(dcl->cur->value);
}*/

// Reentrant version of above call.
// Useful when not iterating over all, to avoid leaving non-NULL cur pointer.
DIRCONT_ENTRY *dcl_next_r(DIRCONT *dcl, DIRCONT_LIST_ENTRY **cur) {
	if (*cur == NULL) {
		*cur = dcl->head;
	}
	else {
		*cur = (*cur)->next;
	}

	if(*cur == NULL) {
		return NULL;
	}
	return &((*cur)->value);
}

// O(n), depending on index
DIRCONT_ENTRY *dcl_at_core(DIRCONT_LIST_ENTRY *dce, size_t index) {
	size_t pos = 0;
	while(true) {
		if(dce == NULL)
			return NULL;
		if(pos == index)
			break;
		dce = dce->next;
		pos++;
	}
	return &(dce->value);
}

DIRCONT_ENTRY *dcl_at(DIRCONT *dcl, size_t index) {
	if(index >= dcl->count)
		return NULL;
	return dcl_at_core(dcl->head, index);
}

// O(n log n), I hope
void dcl_quick_sort_core(DIRCONT_LIST_ENTRY *head, DIRCONT_LIST_ENTRY *tail, ssize_t len
	, int(*cmpr)(DIRCONT_ENTRY*, DIRCONT_ENTRY*)) {
	DIRCONT_LIST_ENTRY *l_entry = head, *r_entry = tail;
	ssize_t l_idx = 0, r_idx = len - 1;
	// берём опорный элемент
	ssize_t m_idx = rand() % len; // get rid of dcl->count
	//ssize_t m_idx = len / 2 + rand() % len / 4; // TODO: unstable
	DIRCONT_ENTRY m = *dcl_at_core(l_entry, m_idx); // FIXED: get copy instead of link

	do {
		// TODO: check for NULL
		while(l_entry != NULL && cmpr(&l_entry->value, &m) < 0) { l_entry = l_entry->next; l_idx++; }
		while(r_entry != NULL && cmpr(&r_entry->value, &m) > 0) { r_entry = r_entry->prev; r_idx--; }
		
		if(l_idx <= r_idx) {
			// FIXME: rebind links maybe?
			DIRCONT_ENTRY tmp = l_entry->value;
			l_entry->value = r_entry->value;
			r_entry->value = tmp;

			// taking ->next after swap!
			l_entry = l_entry->next; l_idx++;
			r_entry = r_entry->prev; r_idx--;
		}
	} while(l_idx <= r_idx);

	if(r_idx > 0) dcl_quick_sort_core(head, r_entry, r_idx + 1, cmpr);
	if(l_idx < len - 1) dcl_quick_sort_core(l_entry, tail, len - l_idx, cmpr);
}

// In-place quicksort of double linked list
void dcl_quick_sort(DIRCONT *dcl, int(*comparator)(DIRCONT_ENTRY*, DIRCONT_ENTRY*)) {
	dcl_quick_sort_core(dcl->head, dcl->tail, dcl->count, comparator);
}
