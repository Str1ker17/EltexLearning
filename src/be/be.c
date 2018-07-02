/*
 * Bionicle Editor v0.1
 * @Author: Str1ker
 * @Created: 1 Jul 2018
 */

#ifdef _MSC_VER
#undef __cplusplus
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <string.h>
#include <ctype.h>

#include <ncursesw/ncurses.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// reuse code
#include "../libncurses_util/ncurses_util.h"
#include "vector.h"

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
	SCRLINE *str_buf;
	FILE *f;
	size_t file_offset_begin;
	size_t file_offset_end;
	VECTOR file_lines;
	size_t top_line;
	bool redraw;
} SCRBUF;

#define lassert(x) (void)((!!(x)) || syscall_error((#x), __FILE__, __LINE__))
#define printl(...) { endwin(); printf(__VA_ARGS__); exit(EXIT_FAILURE); }
#define logprint(...) fprintf(stderr, __VA_ARGS__)

int syscall_error(const char *x, const char *file, const int line) {
	endwin();
	int err_no = errno;
	if (err_no != 0) {
		char *errstr = strerror(err_no);
		fprintf(stderr, "[x] syscall err: '%s' at %s:%d (%d = %s)\n", x, file, line, err_no, errstr);
	}
	else {
		fprintf(stderr, "[x] runtime err: '%s' at %s:%d\n", x, file, line);
	}
	exit(EXIT_FAILURE);
	return EXIT_FAILURE;
}

// Return line length, -1 on error, 0 on EOF
int read_line(FILE *f, SCRLINE *scrline) {
	size_t len = 0;
	char *str = NULL;
	scrline->file_offset = ftell(f);
	len = getline(&str, &len, f);
	if(len == -1) {
		return -1;
	}

	// truncate \r and \n at the end maybe?
	size_t visible_len = len;
	char *ptr = str + len - 1;
	while(ptr >= str && (*ptr == '\n' || *ptr == '\r')) {
		--ptr;
		--visible_len;
	}

	scrline->str = str;
	scrline->len = len;
	scrline->visible_len = visible_len;

	return len;
}

// Return lines successfully read
int read_from(SCRBUF *scrbuf, size_t offset, int line_from) {
	int i;
	lassert(fseek(scrbuf->f, offset, SEEK_SET) == 0);
	for(i = line_from; i < scrbuf->rows; i++) {
		if(read_line(scrbuf->f, &(scrbuf->str_buf[i])) == -1) {
			// EOF!
			break;
		}
	}
	return i - line_from;
}

// positive is down, negative is up
// Returns number of lines successfully scrolled
int vscroll(SCRBUF *scrbuf, int lines) {
	if(lines > 0) {
		lines = min(lines, scrbuf->file_lines.size - (scrbuf->top_line + scrbuf->rows));
	}
	else if (lines < 0) { // lines < 0
		lines = -min(-lines, scrbuf->top_line);
	}

	if(lines == 0) return 0; // already there

	scrbuf->top_line += lines;
	// no optimizations, but I think we can allow ourselves this
	read_from(scrbuf, scrbuf->file_lines.array[scrbuf->top_line], 0);
	scrbuf->redraw = true;

	return lines;
}

size_t dry_read(SCRBUF *scrbuf) {
	size_t lines = 0;
	while(true) {
		char *buf = NULL;
		size_t n = 0;
		size_t pos = ftell(scrbuf->f);
		n = getline(&buf, &n, scrbuf->f);
		if(n == -1)
			break;
		lassert(vector_push_back(&scrbuf->file_lines, pos));
	}
	rewind(scrbuf->f);
	lassert(vector_shrink_to_fit(&scrbuf->file_lines));
	return lines;
}

bool edit_char(SCRBUF *scrbuf, int cur_y, int cur_x, char c) {
	if(scrbuf->top_line + cur_y > scrbuf->file_lines.size)
		return false; // attempt to edit non-existent line
	if(scrbuf->str_buf[cur_y].visible_len <= cur_x)
		return false; // attempt to edit non-existent char in line

	SCRLINE *line = &(scrbuf->str_buf[cur_y]);

	// edit file
	size_t prev = ftell(scrbuf->f);
	//int fd = fileno(scrbuf->f);
	lassert(fseek(scrbuf->f, line->file_offset + cur_x, SEEK_SET) == 0);
	lassert(fwrite(&c, 1, 1, scrbuf->f) == 1);
	//lassert(fputc(c, scrbuf->f) != EOF);
	lassert(fseek(scrbuf->f, prev, SEEK_SET) == 0);
	//lassert(lseek(fd, line->file_offset + cur_x, SEEK_SET) != -1);
	//int ret = write(fd, &c, 1);
	//lassert(lseek(fd, prev, SEEK_SET) != -1);
	// edit scrbuf
	line->str[cur_x] = c;
	// edit screen
	scrbuf->redraw = true;

	return true;
}

int main(int argc, char **argv) {
	freopen("logfile.log", "w", stderr);
	if(argc < 2) {
		printl("[x] Specify file to open\n");
	}

	//int fd = open(argv[1], O_RDWR);
	//lassert(fd != -1);
	//FILE *f = fdopen(fd, "rw");
	FILE *f = fopen(argv[1], "r+"); // NEVER USE "rw+"!!!
	lassert(f != NULL);
	int fd = fileno(f);

	struct stat st;
	fstat(fd, &st);

	if(st.st_size > 128 * 1024 * 1024) {
		printl("[x] File is larger than 128 MB; try another editor\n");
	}

	// file opened, enter curses mode
	nassert(initscr());
	nassert(cbreak());
	nassert(noecho());
	nassert(keypad(stdscr, true));

	nassert(start_color());
	nassert(init_pair(1, COLOR_BLACK, COLOR_WHITE));

	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	int cur_x = 0, cur_y = 0;

	// read file, assume it's text file
	SCRLINE *scr_lines = (SCRLINE*)calloc(rows, sizeof(SCRLINE)); // alloc zero memory
	lassert(scr_lines != NULL);

	SCRBUF scrbuf = {
		  .cols = cols
		, .rows = rows
		, .f = f
		, .file_offset_begin = 0
		, .file_offset_end = -1
		, .top_line = 0
		, .str_buf = scr_lines
		, .redraw = true
	};
	lassert(vector_init(&scrbuf.file_lines, rows));

	size_t lines_total = dry_read(&scrbuf);
    logprint("[i] dry_read processed %lu lines\n", lines_total);

	if(read_from(&scrbuf, 0, 0) == 0) {
		fprintf(stderr, "[!] read_from returned 0 on file '%s'\n", argv[1]);
	}

	while(true) {
		if(!scrbuf.redraw)
			goto move_cursor;

		// draw
		werase(stdscr);

		// put text
		int i;
		for(i = 0; i < rows; i++) {
			char *str = scr_lines[i].str;
			size_t len = scr_lines[i].visible_len;
			lassert(len < 1000); // try

			if(str == NULL) // eof
				break;
			
			//nassert(mvwaddstr(stdscr, i, 0, scr_buf[i].str));
			//mvwaddattrfstr(stdscr, i, 0, min(scr_buf[i].len, cols), scr_buf[i].str, A_NORMAL);
			int pos = 0;
			while(pos < len) {
				char sym = *str;
				if (isprint(sym)) {
					mvwaddch(stdscr, i, pos, sym);
				}
				else { // convert unprintable to spaces
					mvwaddch(stdscr, i, pos, ' ');
				}
				++str;
				++pos;
			}
		}

		wnoutrefresh(stdscr);
		doupdate();

		scrbuf.redraw = false;

move_cursor:
		// handle cursor
		cur_x = min(max(((ssize_t)scr_lines[cur_y].visible_len) - 1, 0), cur_x);
		wmove(stdscr, cur_y, cur_x);

		// handle action
		int64_t key = raw_wgetch(stdscr);
		switch(key) {
			case KEY_UP:
				//cur_y = max(0, cur_y - 1);
				--cur_y;
				if(cur_y < 0) {
					vscroll(&scrbuf, cur_y);
					cur_y = 0;
				}
				break;

			case KEY_DOWN:
				//cur_y = min(rows - 1, cur_y + 1);
				++cur_y;
				if(cur_y > rows - 1) {
					vscroll(&scrbuf, cur_y - (rows - 1));
					cur_y = rows - 1;
				}
				break;

			case KEY_LEFT:
				cur_x = max(0, cur_x - 1);
				break;

			case KEY_RIGHT:
				cur_x = min(cols - 1, cur_x + 1);
				break;

			case RAW_KEY_HOME:
			case RAW_KEY_HOME_ALT:
				cur_x = 0;
				break;

			case RAW_KEY_END:
			case RAW_KEY_END_ALT:
				cur_x = cols - 1;
				break;

			case 0x152: // PAGE DOWN
				vscroll(&scrbuf, rows - 1);
				break;

			case 0x153: // PAGE UP
				vscroll(&scrbuf, -(rows - 1));
				break;

			case KEY_F(10):
				goto end_loop;

			default:
				if (isprint(key)) {
					edit_char(&scrbuf, cur_y, cur_x, (char)key);
				}
				else {
					nassert(wattron(stdscr, COLOR_PAIR(1)));
					mvwprintw(stdscr, rows - 1, 0, " You pressed: 0x%lx Press F10 to quit. ", key);
					nassert(wattroff(stdscr, COLOR_PAIR(1)));
				}
				break;
		}
	}

end_loop:
	nassert(endwin());
	//close(fd);
	fclose(f);
	free(scr_lines);
	return EXIT_SUCCESS;
}
