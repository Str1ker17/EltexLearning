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
#include <string.h>

#include <ncursesw/ncurses.h>
#include <locale.h>

#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "dircont.h"
#include "dirpath.h"

#define nullptr NULL
#define COLOR_INTENSITY 8
// this is may be cuz of non-raw keyboard and getting keychars instead of keycodes
#define KEY_TAB 9
#define KEY_RETURN 10

#ifndef DT_DIR
#define DT_DIR 4
#endif

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
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

typedef struct {
	WINDOW *curs_win;
	char *path;
	DIRPATH dpath;
	DIRCONT contents;
	int position;
	int width;
	int height;
	bool active;
	bool redraw;
} BCPANEL;

bool ncurses_raise_error(const char *x, const char *file, const int line) {
	endwin();
	fprintf(stderr, "[x] ncurses err: '%s' at %s:%d\n", x, file, line);
	exit(1);
	return false;
}

void init_panel(BCPANEL *panel, int rows_panel, int cols_panel, int y, int x, char *path) {
	panel->redraw = true;
	panel->position = 0;

	WINDOW *wnd = newwin(rows_panel, cols_panel, y, x);
	nassert(wnd);
	getmaxyx(wnd, panel->height, panel->width);
	nassert(wbkgd(wnd, COLOR_PAIR(1)));
	panel->curs_win = wnd;
	nassert(scrollok(wnd, true)); // включаем скроллинг; также wsetscrreg(WINDOW*, int top, int bot)
	//panel->path = getcwd(NULL, PATH_MAX);
	dpt_init(&panel->dpath, path);
	panel->path = dpt_string(&panel->dpath, NULL);

	//crassert(panel->path != NULL);
	dcl_init(&panel->contents);
}

void fs_move(BCPANEL *panel) {
	DIRCONT *dcl = &panel->contents;
	DIRPATH *dpt = &panel->dpath;
	struct dirent *curr;
	int pos = 0;
	while(true) {
		curr = dcl_next(dcl);
		if(curr == NULL)
			return;
		if(pos == panel->position)
			break; // gotcha
		pos++;
	}

	if(curr->d_type != DT_DIR)
		return;

	crassert(dpt_move(dpt, curr->d_name));
	free(panel->path);
	panel->path = dpt_string(dpt, NULL);
	panel->position = 0;
	// перечитаем позже
	panel->redraw = true;
}

void reread_files(BCPANEL *panel) {
	struct stat st;
	stat(panel->path, &st); // TODO: check for error
	if(st.st_ino == panel->contents.ino_self)
		return;

	DIR *dir;
	while(true) {
		dir = opendir(panel->path);
		if(dir != NULL)
			break;
		dpt_up(&panel->dpath);
		free(panel->path);
		panel->path = dpt_string(&panel->dpath, NULL);
	}
	//crassert(dir != NULL);
	
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
	int pos = 0;
	int splitter1 = panel->width / 2 + 3;
	mvwvline(wnd, 1, splitter1, '\'', panel->height);
	while((curr = dcl_next(cont)) != NULL) {
		// 2 is rows skipped from the top corner of window
		// 16 is max chars in filename
		if(panel->active && panel->position == pos) {
			nassert(wattron(wnd, COLOR_PAIR(4)));
		}
		char spec;
		switch(curr->d_type) {
			case 4:  spec = '/'; break; // DT_DIR
			default: spec = ' '; break;
		}
		mvwaddch(wnd, pos + 2, 2, spec);
		mvwaddnstr(wnd, pos + 2, 3, curr->d_name, splitter1 - 3);
		if(panel->active && panel->position == pos) {
			nassert(wattroff(wnd, COLOR_PAIR(4)));
		}
		pos++;
	}
}

void switch_tabs(BCPANEL *left, BCPANEL *right, BCPANEL *curr) {
	curr->active = false;
	curr->redraw = true;
	curr = (curr == left ? right : left);
	curr->active = true;
	curr->redraw = true;
}

void redraw_panel(BCPANEL *panel) {
	WINDOW *wnd = panel->curs_win;

	// выводим список файлов
	reread_files(panel);
	print_files(panel);

	//nassert(box(wnd, ACS_VLINE, ACS_HLINE));
	//nassert(box(wnd, '\'', '\''));
	nassert(wborder(wnd, '\'', '\'', '\'', '\'', '\'', '\'', '\'', '\''));
	/*wborder(wnd
		, 0xff5c, 0xff5c
		, 0xff5c, 0xff5c
		, *"\342\224\230"
		, *"\342\224\220"
		, *"\342\224\224"
		, *"\342\224\214"
	);*/

	// выводим текущую папку в заголовке
	if(panel->active) {
		nassert(wattron(wnd, COLOR_PAIR(3)));
	}

	int pathmaxlen = panel->width - 6; // 3 left, 3 right
	int pathlen = strlen(panel->path);
	char *ptr = panel->path + pathlen - min(pathlen, pathmaxlen);
	//nassert(mvwaddstr(wnd, 2, 3, panel->path - min(pathlen, pathmaxlen)));
	nassert(mvwaddnstr(wnd, 0, 3, ptr, pathmaxlen));

	if(panel->active) {
		nassert(wattroff(wnd, COLOR_PAIR(3)));
	}
}

int main(int argc, char **argv) {
	freopen("logfile.log", "w", stderr);

	WINDOW *left, *right;
	WINDOW *menubar, *hintbar, *shellbar, *fnkeybar;

	BCPANEL pleft, pright;
	BCPANEL *pcurr = &pleft;
	pcurr->active = true;

	int rows, cols;

	char hint_str[200] = "\0";
	//char buf[PATH_MAX] = "\0";

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
	nassert(init_pair(1, COLOR_WHITE, COLOR_BLUE)); // default
	nassert(init_pair(2, COLOR_WHITE, COLOR_BLACK)); // non-panel
	nassert(init_pair(3, COLOR_BLACK, COLOR_WHITE)); // panel title
	nassert(init_pair(4, COLOR_BLACK, COLOR_CYAN)); // file cursos highlight

	// create panels
	init_panel(&pleft, rows_panel, cols_panel, 1, 0, getcwd(NULL, NULL));
	init_panel(&pright, rows_panel, cols_panel, 1, cols - cols_panel, getenv("HOME"));
	left = pleft.curs_win;
	right = pright.curs_win;

	// create auxillary windows
	nassert(menubar = newwin(1, cols, 0, 0));
	nassert(hintbar = newwin(1, cols, rows - 3, 0));
	nassert(shellbar = newwin(1, cols, rows - 2, 0));
	nassert(fnkeybar = newwin(1, cols, rows - 1, 0));

	int cl_x = 16, cl_y = rows - 2;

	bool redraw = true;
	while (true) {
		pcurr = pleft.active ? &pleft : &pright;

		if (redraw) {
			// draw
			nassert(werase(left));
			nassert(werase(right));
			//clear();
			nassert(werase(menubar));
			nassert(werase(hintbar));
			nassert(werase(shellbar));
			nassert(werase(fnkeybar));

			nassert(mvwaddstr(menubar, 0, 0, "Menu"));
			nassert(mvwprintw(hintbar, 0, 0, "Hint: %s", hint_str));
			nassert(mvwaddstr(shellbar, 0, 0, "[Command line]$ "));
			nassert(mvwaddstr(fnkeybar, 0, 0, "Functional keys. F10: Exit"));

			redraw_panel(&pleft);
			redraw_panel(&pright);

			nassert(wnoutrefresh(stdscr));
			nassert(wnoutrefresh(left));
			nassert(wnoutrefresh(right));

			nassert(wnoutrefresh(menubar));
			nassert(wnoutrefresh(hintbar));
			nassert(wnoutrefresh(shellbar));
			nassert(wnoutrefresh(fnkeybar));
			nassert(doupdate());

			// перемещаем курсор на командную строку
			nassert(wmove(stdscr, cl_y, cl_x));
			//switch_panel = false;
		}

		// handle action
		redraw = true;
		int c = getch();
		nassert(c);
		switch (c) {
			case KEY_UP:
				pcurr->position = max(0, pcurr->position - 1);
				break;

			case KEY_DOWN:
				pcurr->position = min(pcurr->contents.count - 1, pcurr->position + 1);
				break;

			case KEY_HOME:
				pcurr->position = 0;
				break;

			case KEY_END:
				pcurr->position = pcurr->contents.count - 1;
				break;

			case KEY_TAB: // no macro for tab, really?..
				switch_tabs(&pleft, &pright, pcurr);
				break;

			//case KEY_ENTER:
			case KEY_RETURN: // enter
				fs_move(pcurr);
				break;

			case KEY_F(10):
				goto end_loop;

			default: /*redraw = false;*/
				snprintf(hint_str, sizeof(hint_str), "pressed key is %d", c);
				break;
		}
	}

end_loop:
	nassert(endwin());

	return 0;
}
