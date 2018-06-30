#include <stdlib.h>
#include "dircont.h"
#include <stdio.h>

#ifdef QSORT_DEBUG
#define fprintf(...)

DIRCONT *dcl_g;
ssize_t offset;
#endif

void dcl_init(DIRCONT *dcl) {
	dcl->head = NULL;
	dcl->cur = NULL;
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
bool dcl_push_back(DIRCONT *dcl, struct dirent *entry) {
	DIRCONT_ENTRY *dce = (DIRCONT_ENTRY*)malloc(sizeof(DIRCONT_ENTRY));
	if(dce == NULL)
		return false;
	dce->entry = *entry;
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
	DIRCONT_ENTRY *cur = dcl->head;
	while(cur != NULL) {
		DIRCONT_ENTRY *next = cur->next;
		free(cur);
		cur = next;
	}
	dcl_init(dcl);
}

// Single take, O(1)
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

// Reentrant version of above call.
// Useful when not iterating over all, to avoid leaving non-NULL cur pointer.
struct dirent *dcl_next_r(DIRCONT *dcl, DIRCONT_ENTRY **cur) {
	if (*cur == NULL) {
		*cur = dcl->head;
	}
	else {
		*cur = (*cur)->next;
	}

	if(*cur == NULL) {
		return NULL;
	}
	return &((*cur)->entry);
}

// O(n), depending on index
DIRCONT_ENTRY *dcl_at_core(DIRCONT_ENTRY *dce, ssize_t index) {
	ssize_t pos = 0;
	while(true) {
		if(dce == NULL)
			return NULL;
		if(pos == index)
			break;
		dce = dce->next;
		pos++;
	}
	return dce;
}

DIRCONT_ENTRY *dcl_at(DIRCONT *dcl, ssize_t index) {
	if(index >= dcl->count)
		return NULL;
	return dcl_at_core(dcl->head, index);
}

void dcl_quick_sort_core(DIRCONT_ENTRY *head, DIRCONT_ENTRY *tail, ssize_t len
	, int(*comparator)(DIRCONT_ENTRY*, DIRCONT_ENTRY*)) {
	DIRCONT_ENTRY *l_entry = head, *r_entry = tail;
	ssize_t l_idx = 0, r_idx = len - 1;
	// берём опорный элемент
	ssize_t m_idx = rand() % len; // get rid of dcl->count
	//ssize_t m_idx = len / 2 + rand() % len / 4; // TODO: unstable
	DIRCONT_ENTRY m = *dcl_at_core(l_entry, m_idx); // FIXED: get copy instead of link

#ifdef QSORT_DEBUG
	DIRCONT_ENTRY *cur = head;
	dirent *drnt = &cur->entry;
	fprintf(stderr, "[i] Sorting %ld items, m_idx = %ld (%s), source:\n", len, m_idx, m.entry.d_name);
	do {
		char spec;
		switch(drnt->d_type) {
			case 4: spec = '/'; break; // DT_DIR
			default: spec = ' '; break;
		}
		fputc(spec, stderr);
		fprintf(stderr, "%s\n", drnt->d_name);
		if(cur == tail)
			break;
	} while((drnt = dcl_next_r(NULL, &cur)) != NULL);
	fputc('\n', stderr);
	fflush(stderr);
#endif

	do {
		// TODO: check for NULL
		while(l_entry != NULL && comparator(l_entry, &m) < 0) { l_entry = l_entry->next; l_idx++; }
		while(r_entry != NULL && comparator(r_entry, &m) > 0) { r_entry = r_entry->prev; r_idx--; }
		
		if(l_idx <= r_idx) {

#ifdef QSORT_DEBUG
			fprintf(stderr, "Swap: '%s' <=> '%s' (%ld, %ld)\n"
				, l_entry->entry.d_name, r_entry->entry.d_name, l_idx, r_idx);
#endif

			// FIXME: rebind links maybe?
			dirent tmp = l_entry->entry;
			l_entry->entry = r_entry->entry;
			r_entry->entry = tmp;

			// taking ->next after swap!
			l_entry = l_entry->next; l_idx++;
			r_entry = r_entry->prev; r_idx--;
		}
	} while(l_idx <= r_idx);

#ifdef QSORT_DEBUG
	cur = dcl_g->head;
	drnt = &cur->entry;
	int idx = (int)(0x80000000U);
	fprintf(stderr, "[i] Sorted %ld items, m_idx = %ld (%s), result:\n", len, m_idx, m.entry.d_name);
	while(true) {
		if(cur == head) {
			fprintf(stderr, "->\n");
			idx = 0;
		}
		char spec;
		switch(drnt->d_type) {
			case 4: spec = '/'; break; // DT_DIR
			default: spec = ' '; break;
		}
		fputc(spec, stderr);
		fprintf(stderr, "%-40s %c\n", drnt->d_name, idx == l_idx ? 'L' : idx == r_idx ? 'R' : ' ');
		if(cur == tail)
			fprintf(stderr, "<-\n");
		if(cur == dcl_g->tail)
			break;
		if((drnt = dcl_next_r(NULL, &cur)) == NULL)
			break;
		idx++;
	};
	fputc('\n', stderr);
	fflush(stderr);

	ssize_t caller_offset = offset;
	if(r_idx > 0) {
		fprintf(stderr, "Split: L part = '%s' <=> '%s' [%ld, %ld]\n"
			, head->entry.d_name, r_entry->entry.d_name, caller_offset, caller_offset + r_idx);
	}
	if(l_idx < len - 1) {
		fprintf(stderr, "Split: R part = '%s' <=> '%s' [%ld, %ld]\n"
			, l_entry->entry.d_name, tail->entry.d_name, caller_offset + l_idx, caller_offset + len - 1);
	}
#endif

	if(r_idx > 0) dcl_quick_sort_core(head, r_entry, r_idx + 1, comparator);

#ifdef QSORT_DEBUG
	offset = caller_offset + l_idx;
#endif

	if(l_idx < len - 1) dcl_quick_sort_core(l_entry, tail, len - l_idx, comparator);
}

// In-place quicksort of double linked list
void dcl_quick_sort(DIRCONT *dcl, int(*comparator)(DIRCONT_ENTRY*, DIRCONT_ENTRY*)) {

#ifdef QSORT_DEBUG
	dcl_g = dcl;
	offset = 0;
#endif

	dcl_quick_sort_core(dcl->head, dcl->tail, dcl->count, comparator);
}

#ifdef QSORT_DEBUG
#undef fprintf
#define fprintf(...) fprintf(__VA_ARGS__)
#endif
