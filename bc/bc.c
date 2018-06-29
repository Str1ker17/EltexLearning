/*
 * Bionicle Commander v0.1
 * @Author: Str1ker
 * @Created: 28 Jun 2018
 */

#if defined(_MSC_VER)
#undef __cplusplus
#define __typeof(x) decltype(x)
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ncursesw/ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <linux/limits.h>
//#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "dircont.h"

#define nullptr NULL

//		} else ncurses_raise_error("macro error in " #x, __FILE__, __LINE__); 
#ifndef _MSC_VER
#define nassert(x)                                                            \
	{                                                                         \
		__typeof(x) y = (x);                                                  \
		if(__builtin_types_compatible_p(__typeof(y), void*)) {                \
			if((void*)(uintptr_t)(y) == (void*)(uintptr_t)NULL)               \
				ncurses_raise_error(#x, __FILE__, __LINE__);                  \
		} else if(__builtin_types_compatible_p(__typeof(y), int)) {           \
			if((int)(intptr_t)(y) == ERR)                                     \
				ncurses_raise_error((#x), __FILE__, __LINE__);                \
		} else ((void)0);                                                     \
	}
#else
#define nassert(x) (x)
#endif

#define crassert(x) (void)(!!(x) || ncurses_raise_error((#x), __FILE__, __LINE__))

typedef struct {
	WINDOW *curs_win;
	char *path;
	DIRCONT contents;
	int position;
	bool active;
	bool redraw;
} BCPANEL;

WINDOW *left, *right;
WINDOW *menubar, *hintbar, *shellbar, *fnkeybar;

BCPANEL pleft = {
	  .active = true
	, .redraw = true
	, .position = 0
	, .curs_win = nullptr
	, .path = "\0"
};
BCPANEL pright = {
	  .active = false
	, .redraw = true
	, .position = 0
	, .curs_win = nullptr
	, .path = "\0"
};
BCPANEL *pcurr = &pleft;

bool ncurses_raise_error(const char *x, const char *file, const int line) {
	endwin();
	fprintf(stderr, "[x] ncurses err: '%s' at %s:%d\n", x, file, line);
	_exit(1);
	return false;
}

void list_files(BCPANEL *panel) {
	struct stat st;
	stat(panel->path, &st); // TODO: check for error
	if(st.st_ino == panel->contents.ino_self)
		return;

	DIR *dir = opendir(panel->path);
	crassert(dir != NULL);
	
	dcl_clear(&panel->contents);

	while(true) {
		struct dirent *entry = readdir(dir);
		if(entry == NULL)
			break;
		if(strcmp(entry->d_name, ".") == 0) {
			// save ourselves
			continue;
		}

		crassert(dcl_add(&panel->contents, entry));
	}

	closedir(dir);
	panel->contents.ino_self = st.st_ino;
}

void print_files(BCPANEL *panel) {
	WINDOW *wnd = panel->curs_win;
	DIRCONT *cont = &panel->contents;
	struct dirent *curr;
	int pos = 2;
	while((curr = dcl_next(cont)) != NULL) {
		mvwaddnstr(wnd, pos, 1, curr->d_name, 16);
		pos++;
	}
}

int main(int argc, char **argv) {
	freopen("logfile.log", "w", stderr);
	int rows, cols;

	char hint_str[200] = "\0";
	//char buf[NAME_MAX + 1] = "\0";

	// The library uses the locale which the calling program has initialized.
	// If the locale is not initialized, the library assumes that characters
	// are printable as in ISO-8859-1, to work with certain legacy programs.
	// You should initialize the locale and not rely on specific details of
	// the library when the locale has not been setup.
	// Source: man ncurses, line 30
	setlocale(LC_ALL, "");

	// https://code-live.ru/post/cpp-ncurses-hello-world
	nassert(initscr());

	nassert(noecho());
	nassert(cbreak());
	//intrflush(stdscr, false);
	nassert(keypad(stdscr, true));

	getmaxyx(stdscr, rows, cols);

	int cols_panel = cols / 2;
	int rows_panel = rows - 4; // заголовок и 3 строчки снизу

	nassert(start_color());
	nassert(init_pair(1, COLOR_WHITE, COLOR_BLUE));
	nassert(init_pair(2, COLOR_WHITE, COLOR_BLACK));

	// create panels
	nassert(left = newwin(rows_panel, cols_panel, 1, 0));
	nassert(wbkgd(left, COLOR_PAIR(1)));
	pleft.curs_win = left;
	nassert(scrollok(left, true)); // включаем скроллинг; также wsetscrreg(WINDOW*, int top, int bot)
	pleft.path = getcwd(NULL, PATH_MAX);
	crassert(pleft.path != NULL);
	dcl_init(&pleft.contents);

	nassert(right = newwin(rows_panel, cols_panel, 1, cols - cols_panel));
	nassert(wbkgd(right, COLOR_PAIR(1)));
	pright.curs_win = right;
	nassert(scrollok(right, true));
	pright.path = getenv("HOME");
	crassert(pleft.path != NULL);
	dcl_init(&pright.contents);

	// create auxillary windows
	nassert(menubar = newwin(1, cols, 0, 0));
	nassert(hintbar = newwin(1, cols, rows - 3, 0));
	nassert(shellbar = newwin(1, cols, rows - 2, 0));
	nassert(fnkeybar = newwin(1, cols, rows - 1, 0));

	//int x = cols / 2 - 7, y = rows / 2;
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	int *x, *y;

	int cl_x = 16, cl_y = rows - 2;

	bool redraw = true;
	while (true) {
		if (pleft.active) {
			x = &x1, y = &y1;
		}
		else {
			x = &x2, y = &y2;
		}

		if (redraw) {
			// draw
			werase(left);
			werase(right);
			//clear();
			werase(menubar);
			werase(hintbar);
			werase(shellbar);
			werase(fnkeybar);

			mvwaddstr(menubar, 0, 0, "Menu");
			mvwprintw(hintbar, 0, 0, "Hint: %s", hint_str);
			mvwaddstr(shellbar, 0, 0, "[Command line]$ ");
			mvwaddstr(fnkeybar, 0, 0, "Functional keys. F10: Exit");

			box(left, ACS_VLINE, ACS_HLINE);
			box(right, ACS_VLINE, ACS_HLINE);

			// выводим список файлов
			list_files(&pleft);
			list_files(&pright);

			print_files(&pright);
			print_files(&pleft);

			// highlight panel
			wattron(pleft.active ? left : right, COLOR_PAIR(2));

			char str1[] = "Left panel";
			mvwaddstr(left, rows_panel / 2 + y1, cols_panel / 2 - sizeof(str1) / 2 + x1, str1);

			char str2[] = "Right panel";
			mvwaddstr(right, rows_panel / 2 + y2, cols_panel / 2 - sizeof(str2) / 2 + x2, str2);

			wattroff(pleft.active ? left : right, COLOR_PAIR(2));
			// highlight end

			wnoutrefresh(stdscr);
			wnoutrefresh(left);
			wnoutrefresh(right);

			wnoutrefresh(menubar);
			wnoutrefresh(hintbar);
			wnoutrefresh(shellbar);
			wnoutrefresh(fnkeybar);
			doupdate();

			// перемещаем курсор на командную строку
			wmove(stdscr, cl_y, cl_x);
			//switch_panel = false;
		}

		// handle action
		redraw = true;
		int c = getch();
		switch (c) {
			case KEY_UP:
				--(*y);
				break;

			case KEY_DOWN:
				++(*y);
				break;

			case KEY_LEFT:
				--(*x);
				break;

			case KEY_RIGHT:
				++(*x);
				break;

			case 9: // no macro for tab, really?..
				//active_left = !active_left;
				pleft.active = !pleft.active;
				//switch_panel = true;
				break;

			case KEY_F(10):
				goto end_loop;

			default: /*redraw = false;*/
				snprintf(hint_str, sizeof(hint_str), "pressed key is %d", c);
				break;
		}
	}

end_loop:
	endwin();

	return 0;
}
