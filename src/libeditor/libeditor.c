#include "libeditor.h"

/*
 * Bionicle Editor (Library) v0.2
 * @Author: Str1ker
 * @Created: 1 Jul 2018
 * @Modified: 10 Jul 2018
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
#include <locale.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

// reuse code
#include "../libncurses_util/ncurses_util.h"
#include "../libncurses_util/linux_util.h"

#define printl(...) { endwin(); printf(__VA_ARGS__); exit(EXIT_FAILURE); }
#define debug_cur(x,y) fprintf(stderr, "Cursor at row=%d col=%d\n", y, x)

void signal_winch_handler(int signo) {
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, (char*)&size);
	nassert(resizeterm(size.ws_row, size.ws_col));
}

int init_screenbuf(SCRBUF *scrbuf) {
	getmaxyx(stdscr, scrbuf->rows, scrbuf->cols);

	if(scrbuf->scr_lines != NULL) {
		free(scrbuf->scr_lines);
	}
	scrbuf->scr_lines = (SCRLINE*)calloc(scrbuf->rows, sizeof(SCRLINE)); // alloc zeroed memory
	lassert(scrbuf->scr_lines != NULL);

	return 0;
}

int read_wet_from(SCRBUF *scrbuf, size_t offset, size_t count) {
	for(size_t i = 0; i < count; i++) {
		char **str_ptr = &(scrbuf->scr_lines[i].str);
		if(*str_ptr != NULL) {
			free(*str_ptr);
			*str_ptr = NULL;
		}

		size_t line_num = offset + i;
		if(line_num >= scrbuf->file_lines.size) {
			// ������
			//*str_ptr = NULL;
			continue;
		}

		// ���������� ����� ������ �� ������ dry_read
		uintptr_t file_offset = scrbuf->file_lines.array[line_num];
		__syscall(fseek(scrbuf->f, file_offset, SEEK_SET));
		size_t line_len;
		if (line_num + 1 == scrbuf->file_lines.size) {
			line_len = scrbuf->filesize - file_offset;
		}
		else {
			line_len = scrbuf->file_lines.array[line_num + 1] - file_offset;
		}

		// ������ ������
		*str_ptr = (char*)malloc(line_len + 1);
		lassert(*str_ptr != NULL);
		lassert(fread(*str_ptr, line_len, 1, scrbuf->f) == 1);
		(*str_ptr)[line_len] = '\0';

		scrbuf->scr_lines[i].line_no = line_num;
		scrbuf->scr_lines[i].len = line_len;
		scrbuf->scr_lines[i].file_offset = file_offset;

		// truncate \r and \n at the end maybe?
		size_t visible_len = line_len;
		char *ptr = (*str_ptr) + line_len - 1;
		while(ptr >= (*str_ptr) && (*ptr == '\n' || *ptr == '\r')) {
			--ptr;
			--visible_len;
		}

		scrbuf->scr_lines[i].visible_len = visible_len;
	}
	return 0;
}

// positive is down, negative is up
// Returns number of lines successfully scrolled
int vscroll(SCRBUF *scrbuf, int lines) {
	if(lines > 0) {
		lines = min(lines, 
			(ssize_t)scrbuf->file_lines.size - (ssize_t)(scrbuf->top_line + scrbuf->rows));
	}
	else if (lines < 0) { // lines < 0
		lines = -min(-lines, (ssize_t)scrbuf->top_line);
	}

	if(lines == 0) return 0; // already there

	scrbuf->top_line += lines;
	// no optimizations, but I think we can allow ourselves this
	//read_from(scrbuf, scrbuf->file_lines.array[scrbuf->top_line], 0);
	read_wet_from(scrbuf, scrbuf->top_line, scrbuf->rows);
	scrbuf->redraw = true;

	return lines;
}

size_t dry_read(SCRBUF *scrbuf) {
	size_t lines = 0;
	size_t pos;
	while(true) {
		char *buf = NULL;
		size_t n = 0;
		pos = ftell(scrbuf->f);
		ssize_t status = getline(&buf, &n, scrbuf->f);
       	free(buf);
		if(status == -1)
			break;

		lassert(vector_push_back(&scrbuf->file_lines, pos));
		++lines;
	}
	scrbuf->filesize = pos;
	rewind(scrbuf->f);
	lassert(vector_shrink_to_fit(&scrbuf->file_lines));
	return lines;
}

bool edit_char(SCRBUF *scrbuf, size_t cur_y, size_t cur_x, char c) {
	if(scrbuf->top_line + cur_y > scrbuf->file_lines.size)
		return false; // attempt to edit non-existent line
	if(scrbuf->scr_lines[cur_y].visible_len <= scrbuf->left_char + cur_x)
		return false; // attempt to edit non-existent char in line

	SCRLINE *line = &(scrbuf->scr_lines[cur_y]);

	// edit file
	size_t prev = ftell(scrbuf->f);

	__syscall(fseek(scrbuf->f, line->file_offset + scrbuf->left_char + cur_x, SEEK_SET));
	lassert(fwrite(&c, 1, 1, scrbuf->f) == 1);
	__syscall(fseek(scrbuf->f, prev, SEEK_SET));

	// edit scrbuf
	line->str[scrbuf->left_char + cur_x] = c;
	// edit screen
	scrbuf->redraw = true;

	return true;
}

int editor_main(int argc, char **argv) {
	setlocale(LC_ALL, "");
	if(argc < 2) {
		printl("[x] Specify file to open\n");
	}

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
	//nassert(cbreak());
	nassert(raw());
	nassert(noecho());
	nassert(keypad(stdscr, true));

	struct sigaction sga = {
		  .sa_mask = { .__val = { 0 } }
		, .sa_flags = 0
		, .sa_restorer = (void(*)(void))NULL
		, .sa_handler = signal_winch_handler
	};
	__syscall(sigaction(SIGWINCH, &sga, NULL));

	nassert(start_color());
	nassert(init_pair(1, COLOR_BLACK, COLOR_WHITE));

	int rows, cols;
	int cur_x = 0, cur_y = 0;
	bool show_hint = false;

	// read file, assume it's text file
	SCRBUF scrbuf = {
		  .f = f
		, .filesize = 0
		, .top_line = 0
		, .left_char = 0
		, .scr_lines = NULL
		, .redraw = true
	};
	init_screenbuf(&scrbuf);
	getmaxyx(stdscr, rows, cols);
	lassert(vector_init(&scrbuf.file_lines, scrbuf.rows));

	// starting dry read
	size_t left_char_prev;
	size_t lines_total = dry_read(&scrbuf);
	logprint("[i] dry_read processed %zu lines\n", lines_total);

	read_wet_from(&scrbuf, 0, scrbuf.rows);
	int64_t key = 0;

	while(true) {
		// resize
		getmaxyx(stdscr, rows, cols);
		if(rows != scrbuf.rows || cols != scrbuf.cols) {
			init_screenbuf(&scrbuf);
			read_wet_from(&scrbuf, scrbuf.top_line, scrbuf.rows);
			rows = scrbuf.rows;
			cols = scrbuf.cols;
			scrbuf.redraw = true;
		}

		if(!scrbuf.redraw)
			goto move_cursor;

		// draw
		werase(stdscr);

		// put text
		int i;
		for(i = 0; i < rows; i++) {
			char *str = scrbuf.scr_lines[i].str;
			if(str == NULL) // eof
				break;

			size_t len = scrbuf.scr_lines[i].visible_len;
			
			size_t pos = 0;
			size_t pos_scr = 0;
			if(i == cur_y) {
				pos = scrbuf.left_char;
				str += pos; // move to pos chars
			}
			while(pos < len) {
				char sym = *str;
				if (isprint(sym)) {
					mvwaddch(stdscr, i, pos_scr, sym);
				}
				else { // convert unprintable to spaces
					mvwaddch(stdscr, i, pos_scr, ' ');
				}
				++str;
				++pos;
				++pos_scr;
			}
		}

		// put hint
		if(show_hint) {
			nassert(wattron(stdscr, COLOR_PAIR(1)));
			mvwprintw(stdscr, rows - 1, 0, " You pressed: 0x%lx Press F10 to quit. ", key);
			nassert(wattroff(stdscr, COLOR_PAIR(1)));
			show_hint = false;
		}

		wnoutrefresh(stdscr);
		doupdate();

		scrbuf.redraw = false;

move_cursor:
		// handle cursor
		//cur_x = min(max(((ssize_t)scrbuf.scr_lines[cur_y].visible_len) - 1, 0), cur_x);
		wmove(stdscr, cur_y, cur_x);

		// handle action
		int vscrolled;
		key = raw_wgetch(stdscr);
		switch(key) {
			case KEY_UP:
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				scrbuf.left_char = 0;
				--cur_y;
				if(cur_y < 0) {
					vscroll(&scrbuf, cur_y);
					cur_y = 0;
				}
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case KEY_DOWN:
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				scrbuf.left_char = 0;
				++cur_y;
				if(cur_y > rows - 1) {
					vscroll(&scrbuf, cur_y - (rows - 1));
					cur_y = rows - 1;
				}
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case KEY_LEFT:
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				cur_x = max(0, cur_x - 1);
				/*if(cur_x == 0) {
					if(scrbuf.left_char > 0) {
						--(scrbuf.left_char);
						scrbuf.redraw = true;
					}
				}
				else {
					--cur_x;
				}*/
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case KEY_RIGHT:
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				cur_x = min(cols - 1, cur_x + 1);
				/*if(cur_x == cols - 1) { // scroll right
					if(scrbuf.left_char < scrbuf.scr_lines[cur_y].visible_len - cols) {
						++(scrbuf.left_char);
						scrbuf.redraw = true;
					}
				}
				else if((ssize_t)(scrbuf.scr_lines[cur_y].visible_len - scrbuf.left_char) > cur_x - 1) {
					++cur_x;
				}*/
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case RAW_KEY_HOME:
			case RAW_KEY_HOME_ALT:
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				if(scrbuf.left_char > 0) scrbuf.redraw = true;
				scrbuf.left_char = 0;
				cur_x = 0;
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case RAW_KEY_END:
			case RAW_KEY_END_ALT:
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				left_char_prev = scrbuf.left_char;
				scrbuf.left_char = max(0, ((ssize_t)scrbuf.scr_lines[cur_y].visible_len) - cols);
				cur_x = min(((ssize_t)scrbuf.scr_lines[cur_y].visible_len) - cols, cols - 1);
				if(left_char_prev != scrbuf.left_char) scrbuf.redraw = true;
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case RAW_KEY_PAGE_DOWN: // PAGE DOWN
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				scrbuf.left_char = 0;
				vscrolled = vscroll(&scrbuf, rows - 1);
				cur_y = min(rows - 1, cur_y + (rows - 1) - vscrolled);
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case RAW_KEY_PAGE_UP: // PAGE UP
				fprintf(stderr, "Before: "); debug_cur(cur_x, cur_y);
				scrbuf.left_char = 0;
				vscrolled = vscroll(&scrbuf, -(rows - 1));
				cur_y = max(0, cur_y - ((rows - 1) - -vscrolled));
				fprintf(stderr, "After:  "); debug_cur(cur_x, cur_y);
				break;

			case KEY_F(10):
				goto end_loop;

			case RAW_KEY_ERR:
				break;

			default:
				if ((key >> 8) == 0 && isprint(key)) { // � ��� � ����������?
					edit_char(&scrbuf, cur_y, cur_x, (char)key);
				}
				else {
					show_hint = true;
					scrbuf.redraw = true;
				}
				break;
		}
	}

end_loop:

	nassert(endwin());

	fclose(f);

	// we can free all heap memory there
	// but we shall do that completely, with deeper levels
	for(int i = 0; i < scrbuf.rows; i++) {
		if(scrbuf.scr_lines[i].str != NULL)
			free(scrbuf.scr_lines[i].str);
	}
	free(scrbuf.scr_lines);
	vector_free(&scrbuf.file_lines);

	return EXIT_SUCCESS;
}
