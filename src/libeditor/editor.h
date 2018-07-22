#pragma once

#ifdef _MSC_VER
#undef __cplusplus
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../libvector/vector.h"

typedef uint8_t byte;

typedef struct __scr_line {
	char *str;
	size_t len;
	size_t visible_len;
	size_t file_offset;
	size_t line_no;
} SCRLINE;

typedef struct __scr_buf {
	int rows;
	int cols;
	SCRLINE *scr_lines;
	FILE *f;
	size_t filesize;
	VECTOR file_lines;
	size_t top_line;
	size_t left_char;
	bool redraw;
} SCRBUF;

size_t dry_read(SCRBUF *scrbuf);
int read_wet_from(SCRBUF *scrbuf, size_t offset, size_t count);

int editor_main(int argc, char **argv);
