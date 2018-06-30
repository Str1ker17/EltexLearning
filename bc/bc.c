/*
 * Bionicle Commander v0.1
 * @Author: Str1ker
 * @Created: 28 Jun 2018
 * @Modified: 30 Jun 2018
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
#include <wchar.h>
#include <ctype.h>

#include <ncursesw/ncurses.h>
#include <locale.h>
//#include <glib.h>

#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "dircont.h"
#include "dirpath.h"
#include "ncurses_util.h"

#define nullptr NULL
#define COLOR_INTENSITY 8


#ifndef DT_DIR
#define DT_DIR 4
#endif

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

typedef struct {
	WINDOW *curs_win;
	WINDOW *curs_pad;
	char *path; // текущая директория в виде строки
	DIRPATH dpath; // текущая директория в виде связного списка
	DIRCONT contents;
	int top; // смещение относительно верха
	int position; // индекс выделенного элемента в contents
	int width;
	int height;
	bool active;
	bool redraw; // требуется ли перерисовка
} BCPANEL;

void init_panel(BCPANEL *panel, int rows_panel, int cols_panel, int y, int x, char *path) {
	panel->redraw = true;
	panel->position = 0;

	WINDOW *wnd = newwin(rows_panel, cols_panel, y, x);
	//WINDOW *pad = subpad(wnd, 1, cols_panel, 0, 0);
	nassert(wnd);
	//nassert(pad);
	getmaxyx(wnd, panel->height, panel->width);
	nassert(wbkgd(wnd, COLOR_PAIR(1)));
	panel->curs_win = wnd;
	//panel->curs_pad = pad;
	nassert(scrollok(wnd, true)); // включаем скроллинг; также wsetscrreg(WINDOW*, int top, int bot)
	//panel->path = getcwd(NULL, PATH_MAX);
	dpt_init(&panel->dpath, path);
	panel->path = dpt_string(&panel->dpath, NULL);

	//crassert(panel->path != NULL);
	dcl_init(&panel->contents);
}

int fs_comparator(DIRCONT_ENTRY *left, DIRCONT_ENTRY *right) {
	// .. first
	if(strcmp(left->entry.d_name, "..") == 0) return -1;
	if(strcmp(right->entry.d_name, "..") == 0) return 1;

	// directories first
	if(left->entry.d_type == DT_DIR && right->entry.d_type != DT_DIR) return -1;
	if(left->entry.d_type != DT_DIR && right->entry.d_type == DT_DIR) return 1;

	// sort directories and files alphabetically
	return strcmp(left->entry.d_name, right->entry.d_name);
}

#ifdef QSORT_DEBUG
int fs_log_comparator(DIRCONT_ENTRY *left, DIRCONT_ENTRY *right) {
	int ret = fs_comparator(left, right);
	fprintf(stderr, "<> compare: '%s' %c '%s'\n", left->entry.d_name,
		ret < 0 ? '<' : ret > 0 ? '>' : '=', right->entry.d_name);
	return ret;
}
#else
#define fs_log_comparator fs_comparator
#endif

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
	
	dcl_clear(&panel->contents);

	while(true) {
		struct dirent *entry = readdir(dir);
		if(entry == NULL)
			break;
		if(strcmp(entry->d_name, ".") == 0) {
			// save ourselves
			continue;
		}

		crassert(dcl_push_back(&panel->contents, entry));
	}

	closedir(dir);
	panel->contents.ino_self = st.st_ino;

	dcl_quick_sort(&panel->contents, &fs_log_comparator);
}

void print_files(BCPANEL *panel) {
	WINDOW *wnd = panel->curs_win;
	DIRCONT *cont = &panel->contents;
	struct dirent *curr;
	int pos = 0;
	int splitter1 = panel->width / 2 + 4;
	mvwvline(wnd, 1, splitter1, '\'', panel->height);
	while((curr = dcl_next(cont)) != NULL) {
		// 2 is rows skipped from the top corner of window
		// 16 is max chars in filename
		if(panel->active && panel->position == pos) {
			nassert(wattron(wnd, COLOR_PAIR(4)));
		}
		else {
			nassert(wattron(wnd, COLOR_PAIR(1)));
		}
		char spec;
		chtype attr;
		switch(curr->d_type) {
			case DT_DIR:  spec = '/'; attr = A_BOLD; break; // DT_DIR
			default: spec = ' '; attr = A_NORMAL; break;
		}
		mvwaddch(wnd, pos + 2, 1, spec | attr);
		//mvwaddnstr(wnd, pos + 2, 2, curr->d_name, splitter1 - 3);
		//chstr = create_chstr(curr->d_name, strlen(curr->d_name), attr);
		//mvwaddchnstr(wnd, pos + 2, 2, chstr, splitter1 - 3);
		//free(chstr);
		mvwaddattrfstr(wnd, pos + 2, 2, splitter1 - 2, curr->d_name, attr);
		if (panel->active && panel->position == pos) {
			nassert(wattroff(wnd, COLOR_PAIR(4)));
		}
		else {
			nassert(wattron(wnd, COLOR_PAIR(1)));
		}
		pos++;
	}
}

void fs_move(BCPANEL *panel) {
	DIRCONT *dcl = &panel->contents;
	DIRPATH *dpt = &panel->dpath;
	DIRCONT_ENTRY *cursor = NULL;
	struct dirent *curr;
	int pos = 0;
	while(true) {
		curr = dcl_next_r(dcl, &cursor);
		if(curr == NULL)
			return;
		if(pos == panel->position)
			break; // gotcha
		pos++;
	}

	if(curr->d_type != DT_DIR)
		return;

	// сохраняем последний каталог
	dirent prev_copy = { .d_name = "\0" };
	if(dpt != NULL) prev_copy = dpt->tail->entry;

	crassert(dpt_move(dpt, curr->d_name));
	free(panel->path);
	panel->path = dpt_string(dpt, NULL);
	panel->position = 0;

	// ищем каталог из которого вышли, если вышли
	if(strcmp(curr->d_name, "..") == 0) {
		// перечитаем позже
		// нет, сейчас
		reread_files(panel);
		dcl = &panel->contents;

		pos = 0;
		cursor = NULL; // WARNING: обязательно обнулять курсор при обходе!
		while(true) {
			curr = dcl_next_r(dcl, &cursor);
			if(curr == NULL)
				break;
			if(strcmp(curr->d_name, prev_copy.d_name) == 0) {
				panel->position = pos;
				break; // gotcha
			}
			pos++;
		}
	}

	// а вот перерисуем позже
	panel->redraw = true;
}

void switch_tabs(BCPANEL *left, BCPANEL *right, BCPANEL **curr) {
	BCPANEL *curr_ptr = *curr;
	curr_ptr->active = false;
	curr_ptr->redraw = true;
	*curr = (curr_ptr == left ? right : left);

	curr_ptr = *curr;
	curr_ptr->active = true;
	curr_ptr->redraw = true;
}

void redraw_panel(BCPANEL *panel) {
	if(!panel->redraw)
		return;
	WINDOW *wnd = panel->curs_win;
	nassert(werase(wnd));

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

	int pathmaxlen = panel->width - 8; // 3 left, 3 right
	int pathlen = strlen(panel->path);
	int pathcutlen = min(pathlen, pathmaxlen);
	char *ptr = panel->path + pathlen - pathcutlen;
	//nassert(mvwaddstr(wnd, 2, 3, panel->path - min(pathlen, pathmaxlen)));
	nassert(mvwaddch(wnd, 0, 3, ' '));
	nassert(mvwaddnstr(wnd, 0, 4, ptr, pathmaxlen));
	nassert(mvwaddch(wnd, 0, 4 + pathcutlen, ' '));

	if(panel->active) {
		nassert(wattroff(wnd, COLOR_PAIR(3)));
	}

	panel->redraw = false;
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
	char shell_str[40] = "\0";
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
	//nassert(cbreak());
	nassert(raw());
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
	init_panel(&pleft, rows_panel, cols_panel, 1, 0, getcwd(NULL, 0));
	init_panel(&pright, rows_panel, cols_panel, 1, cols - cols_panel, getenv("HOME"));
	left = pleft.curs_win;
	right = pright.curs_win;

	// create auxillary windows
	nassert(menubar = newwin(1, cols, 0, 0));
	nassert(hintbar = newwin(1, cols, rows - 3, 0));
	nassert(shellbar = newwin(1, cols, rows - 2, 0));
	nassert(fnkeybar = newwin(1, cols, rows - 1, 0));

	nassert(wbkgd(menubar, COLOR_PAIR(4)));

	int cl_y = rows - 2;

	while (true) {
		// draw
		//nassert(werase(menubar));
		nassert(werase(hintbar));
		if(pleft.redraw || pright.redraw)
			nassert(werase(shellbar));
		//nassert(werase(fnkeybar));

		nassert(mvwaddstr(menubar, 0, 1, "Menu"));

		nassert(mvwprintw(hintbar, 0, 0, "Hint: %s", hint_str));

		int cl_x = snprintf(shell_str, sizeof(shell_str)
			, "[nosh %s]$ ", pcurr->dpath.tail->entry.d_name);
		nassert(mvwaddstr(shellbar, 0, 0, shell_str));

		nassert(mvwaddattrfstr(fnkeybar, 0, cols - 8, 2, "10", A_BOLD));
		nassert(wattron(fnkeybar, COLOR_PAIR(4)));
		nassert(mvwaddattrfstr(fnkeybar, 0, cols - 6, 5, "Quit\n", A_NORMAL));
		nassert(wattroff(fnkeybar, COLOR_PAIR(4)));

		redraw_panel(&pleft);
		redraw_panel(&pright);

		nassert(wnoutrefresh(stdscr));
		nassert(wnoutrefresh(left));
		nassert(wnoutrefresh(right));
		//nassert(pnoutrefresh(left, ));

		nassert(wnoutrefresh(menubar));
		nassert(wnoutrefresh(hintbar));
		nassert(wnoutrefresh(shellbar));
		nassert(wnoutrefresh(fnkeybar));
		nassert(doupdate());

		// перемещаем курсор на командную строку
		nassert(wmove(stdscr, cl_y, cl_x));
		//switch_panel = false;

		// handle action
		int c = getch();
		nassert(c);
		//wint_t c;
		//nassert(wget_wch(stdscr, &c));
		int64_t raw_key = 0;
		if (c == 27) {
			nassert(nodelay(stdscr, true));
			hint_str[0] = '\0';
			do {
				//sprintf(hint_str + strlen(hint_str), "%02x", c);
				raw_key = raw_key << 8 | c;
			//} while ((wget_wch(stdscr, &c)) != ERR);
			} while ((c = getch()) != ERR);
			//sprintf(hint_str + strlen(hint_str), ", %0lX", raw_key);
			nassert(nodelay(stdscr, false));
		}
		else {
			raw_key = c;
		}

		switch (raw_key) {
			case KEY_UP:
				pcurr->position = max(0, pcurr->position - 1);
				pcurr->redraw = true;
				break;

			case KEY_DOWN:
				pcurr->position = min(pcurr->contents.count - 1, pcurr->position + 1);
				pcurr->redraw = true;
				break;

			case RAW_KEY_HOME:
				pcurr->position = 0;
				pcurr->redraw = true;
				break;

			case RAW_KEY_END:
				pcurr->position = pcurr->contents.count - 1;
				pcurr->redraw = true;
				break;

			case RAW_KEY_TAB: // no macro for tab, really?..
				switch_tabs(&pleft, &pright, &pcurr);
				break;

			case RAW_KEY_ENTER:
			case RAW_KEY_NUMPAD_ENTER: // enter
				fs_move(pcurr);
				break;

			case KEY_F(10):
				goto end_loop;

			default: /*redraw = false;*/
				if (isprint(c)) {
					nassert(mvwaddch(shellbar, 0, cl_x, c));
				}
				else {
					snprintf(hint_str, sizeof(hint_str), "pressed key is 0x%lx", raw_key);
				}
				break;
		}
	}

end_loop:
	nassert(endwin());

	return 0;
}
