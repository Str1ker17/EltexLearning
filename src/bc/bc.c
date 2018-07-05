/*
 * Bionicle Commander v0.1
 * @Author: Str1ker
 * @Created: 28 Jun 2018
 * @Modified: 5 Jul 2018
 */

#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include <ncursesw/ncurses.h>
#include <locale.h>

#include <linux/limits.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "dircont.h"
#include "dirpath.h"
#include "../libncurses_util/ncurses_util.h"
#include <limits.h>
#include <fcntl.h>

#define nullptr NULL

#undef DT_DIR

// количество строк, зарезервированных под что-то, кроме списка файлов
#define PANEL_SPEC_LINES 3

typedef struct {
	WINDOW *curs_win; // ncurses-окно
	//WINDOW *curs_pad; // ncurses-pad, не реализовано
	char *path; // текущая директория в виде строки
	DIRPATH dpath; // текущая директория в виде связного списка
	DIRCONT contents; // содержимое текущей директории (файлы и директории) в виде связного списка
	size_t top_elem; // смещение ncurses-окна относительно верха (прокрутка), не реализовано
	size_t position; // индекс выделенного элемента в contents
	size_t width;
	size_t height;
	bool active; // активна ли эта панель
	bool redraw; // требуется ли перерисовка
} BCPANEL;

enum {
	  CPAIR_DEFAULT = 1
	, CPAIR_PANEL
	, CPAIR_PANEL_TITLE
	, CPAIR_HIGHLIGHT
	, CPAIR_EXECUTABLE
};

void signal_winch_handler(int signo) {
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, (char*)&size);
	nassert(resizeterm(size.ws_row, size.ws_col));
}

void init_panel(BCPANEL *panel, int rows_panel, int cols_panel, int y, int x, char *path) {
	panel->redraw = true;
	panel->position = 0;

	// TODO: create pad to minimize calls cost of print_files()
	WINDOW *wnd = newwin(rows_panel, cols_panel, y, x);
	nassert(wnd);
	getmaxyx(wnd, panel->height, panel->width);
	nassert(wbkgd(wnd, COLOR_PAIR(CPAIR_PANEL)));
	panel->curs_win = wnd;

	dpt_init(&panel->dpath, path);
	panel->path = dpt_string(&panel->dpath, NULL);

	dcl_init(&panel->contents);
}

void free_panel(BCPANEL *panel) {
	dcl_clear(&panel->contents);
	dcl_clear(&panel->dpath);
	free(panel->path);
}

bool resize_screen(WINDOW **wnd_arr, int rows, int cols, int *rows_panel, int *cols_panel, int *cl_y) {
	// DONE: temporarily block SIGWINCH maybe? no, cuz we do nothing with stdscr there
	if(rows < 5)
		return false;

	WINDOW *left_panel = wnd_arr[0];
	WINDOW *right_panel = wnd_arr[1];
	WINDOW *menu_bar = wnd_arr[2];
	WINDOW *hint_bar = wnd_arr[3];
	WINDOW *shell_bar = wnd_arr[4];
	WINDOW *fnkey_bar = wnd_arr[5];

	int _cols_panel = cols / 2;
	int _rows_panel = max(1, rows - 4); // заголовок и 3 строчки снизу
	nassert(wresize(left_panel, _rows_panel, cols - _cols_panel)); // левая панель мб шире
	nassert(wresize(right_panel, _rows_panel, _cols_panel));
	nassert(mvwin(right_panel, 1, cols - _cols_panel));

	nassert(wresize(menu_bar, 1, cols));
	//nassert(mvwin(menu_bar, 0, 0)); // redundant
	nassert(wresize(hint_bar, 1, cols));
	nassert(mvwin(hint_bar, max(1, rows - 3), 0));
	nassert(wresize(shell_bar, 1, cols));
	nassert(mvwin(shell_bar, max(1, rows - 2), 0));
	nassert(wresize(fnkey_bar, 1, cols));
	nassert(mvwin(fnkey_bar, max(1, rows - 1), 0));

	left_panel->_clear = true;
	right_panel->_clear = true;
	menu_bar->_clear = true;
	hint_bar->_clear = true;
	shell_bar->_clear = true;
	fnkey_bar->_clear = true;

	*rows_panel = _rows_panel;
	*cols_panel = _cols_panel;
	*cl_y = rows - 2;
	return true;
}

int fs_comparator(DIRCONT_ENTRY *left, DIRCONT_ENTRY *right) {
	// .. first
	if(strcmp(left->ent.d_name, "..") == 0) return -1;
	if(strcmp(right->ent.d_name, "..") == 0) return 1;

	// directories first
	if((left->st.st_mode & __S_IFDIR) && !(right->st.st_mode & __S_IFDIR)) return -1;
	if(!(left->st.st_mode & __S_IFDIR) && (right->st.st_mode & __S_IFDIR)) return 1;

	// sort directories and files alphabetically
	return strcmp(left->ent.d_name, right->ent.d_name);
}

int vscroll(BCPANEL *panel, int lines) {
	if(lines == 0)
		return 0; // already there
	int lines_delta;
	if (lines < 0) {
		 // up
		lines_delta = -min((size_t)(-lines), panel->top_elem);
	}
	else {
		// down
		size_t height_use = panel->height < PANEL_SPEC_LINES ? 0 : panel->height - PANEL_SPEC_LINES;
		if (panel->contents.count <= panel->top_elem + height_use)
			lines_delta = 0;
		else
			lines_delta = min(panel->contents.count - (panel->top_elem + height_use), (size_t)lines);
	}

	if(lines_delta != 0) {
		panel->top_elem += lines_delta;
		panel->redraw = true;
	}
	return lines_delta;
}

void reread_files(BCPANEL *panel) {
	struct stat st = { .st_ino = 0 };
	stat(panel->path, &st); // TODO: check for error
	if(st.st_ino == panel->contents.ino_self)
		return;

	// пытаемся открыть папку, при неудаче откатываемся на уровень выше
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
	chdir(panel->path);

	while(true) {
		struct dirent *entry = readdir(dir);
		if(entry == NULL)
			break;
		if(strcmp(entry->d_name, ".") == 0) {
			// save ourselves
			continue;
		}
		if(strcmp(panel->path, "/") == 0 && strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		DIRCONT_ENTRY ent = { .ent = *entry };
		custom_assert(stat(entry->d_name, &ent.st) == 0, ncurses_raise_error);
		custom_assert(dcl_push_back(&panel->contents, &ent), ncurses_raise_error);
	}

	closedir(dir);
	panel->contents.ino_self = st.st_ino;

	dcl_quick_sort(&panel->contents, &fs_comparator);
}

void print_files(BCPANEL *panel) {
	WINDOW *wnd = panel->curs_win;
	DIRCONT *cont = &panel->contents;

	size_t pos = 0;
	DIRCONT_LIST_ENTRY *cursor = NULL;
	DIRCONT_ENTRY *curr;
	size_t pos_scr = 0;
	int splitter1 = panel->width / 2 + 4;
	mvwvline(wnd, 1, splitter1, '\'', panel->height);
	while((curr = dcl_next_r(cont, &cursor)) != NULL) {
		// 2 is rows skipped from the top corner of window
		// 16 is max chars in filename
		if(pos < panel->top_elem) {
			++pos;
			continue;
		}

		// tune item look
		/*
		switch(curr->ent.d_type) {
			case DT_DIR:  spec = '/'; attr = A_BOLD; break; // DT_DIR
			default: spec = ' '; attr = A_NORMAL; break;
		}
		*/

		char spec = ' ';
		chtype attr = A_NORMAL;
		int cpair = CPAIR_PANEL;

		__mode_t st_mode = curr->st.st_mode;
		if(st_mode & __S_IFDIR) { spec = '/'; attr = A_BOLD; }
		else if(st_mode & __S_IEXEC) { spec = '*'; attr = A_BOLD; }

		if (panel->active && panel->position == pos) cpair = CPAIR_HIGHLIGHT;
		else if(!(st_mode & __S_IFDIR) && (st_mode & __S_IEXEC)) cpair = CPAIR_EXECUTABLE;

		// actually draw
		if (cpair != CPAIR_PANEL)
			nassert(wattron(wnd, COLOR_PAIR(cpair)));

		int status = mvwaddch(wnd, pos_scr + 2, 1, spec | attr);
		mvwaddattrfstr(wnd, pos_scr + 2, 2, splitter1 - 2, curr->ent.d_name, attr);

		if (cpair != CPAIR_PANEL)
			nassert(wattroff(wnd, COLOR_PAIR(cpair)));

		if(status == ERR)
			break; // вывели всё
		++pos;
		++pos_scr;
	}
}

void fs_move(BCPANEL *panel) {
	DIRCONT *dcl = &panel->contents;
	DIRPATH *dpt = &panel->dpath;
	DIRCONT_LIST_ENTRY *cursor = NULL;
	DIRCONT_ENTRY *curr;
	size_t pos = 0;
	// смотрим, в какую по списку папку нужно перейти
	while(true) {
		curr = dcl_next_r(dcl, &cursor);
		if(curr == NULL)
			return;
		if(pos == panel->position)
			break; // gotcha
		pos++;
	}

	if(!(curr->st.st_mode & __S_IFDIR))
		return;

	// сохраняем последний каталог
	DIRCONT_ENTRY prev_copy = { .ent = { .d_name = "\0" } };
	if(dpt->tail != NULL) prev_copy = dpt->tail->value;
	// и каталог на который нажали
	DIRCONT_ENTRY prev_sel = *curr;

	custom_assert(dpt_move(dpt, curr->ent.d_name), ncurses_raise_error);
	free(panel->path);
	panel->path = dpt_string(dpt, NULL);
	panel->position = 0;
	panel->top_elem = 0;

	// перечитаем позже
	// нет, сейчас
	reread_files(panel);

	// ищем каталог из которого вышли, если вышли
	if(strcmp(prev_sel.ent.d_name, "..") == 0) {
		dcl = &panel->contents;

		pos = 0;
		cursor = NULL; // WARNING: обязательно обнулять курсор при обходе!
		while(true) {
			curr = dcl_next_r(dcl, &cursor);
			if(curr == NULL)
				break;
			if(strcmp(curr->ent.d_name, prev_copy.ent.d_name) == 0) {
				panel->position = pos;
				break; // gotcha
			}
			pos++;
		}
	}

	// на случай, если в каталог не получилось зайти, нужно обновить строку
	free(panel->path);
	panel->path = dpt_string(dpt, NULL);

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

	// выводим текущую папку в заголовке
	if(panel->active) {
		nassert(wattron(wnd, COLOR_PAIR(CPAIR_PANEL_TITLE)));
	}

	int pathmaxlen = panel->width - 8; // 3 left, 3 right
	int pathlen = strlen(panel->path);
	int pathcutlen = min(pathlen, pathmaxlen);
	char *ptr = panel->path + pathlen - pathcutlen;
	nassert(mvwaddch(wnd, 0, 3, ' '));
	nassert(mvwaddnstr(wnd, 0, 4, ptr, pathmaxlen));
	nassert(mvwaddch(wnd, 0, 4 + pathcutlen, ' '));

	if(panel->active) {
		nassert(wattroff(wnd, COLOR_PAIR(CPAIR_PANEL_TITLE)));
	}

	panel->redraw = false;
}

int main(int argc, char **argv) {
	// The library uses the locale which the calling program has initialized.
	// If the locale is not initialized, the library assumes that characters
	// are printable as in ISO-8859-1, to work with certain legacy programs.
	// You should initialize the locale and not rely on specific details of
	// the library when the locale has not been setup.
	// Source: man ncurses, line 30
	setlocale(LC_ALL, "");

	freopen("logfile.log", "w", stderr);

	WINDOW *left, *right;
	WINDOW *menubar, *hintbar, *shellbar, *fnkeybar;

	BCPANEL pleft = { .active = true }, pright = { .active = false };
	BCPANEL *pcurr = &pleft;

	int rows, cols;

	char hint_str[200] = "\0";
	char shell_str[40] = "\0";
	char path_str[PATH_MAX] = "\0";

	// https://code-live.ru/post/cpp-ncurses-hello-world
	nassert(initscr());
	nassert(cbreak());
	//nassert(raw());
	nassert(noecho());
	nassert(intrflush(stdscr, false));
	nassert(keypad(stdscr, true));

	getmaxyx(stdscr, rows, cols);
	custom_assert(rows >= 5, ncurses_raise_error);

	int cols_panel = cols / 2;
	int rows_panel = rows - 4; // заголовок и 3 строчки снизу

	nassert(start_color());
	nassert(init_pair(CPAIR_DEFAULT, COLOR_WHITE, COLOR_BLACK)); // non-panel
	nassert(init_pair(CPAIR_PANEL, COLOR_WHITE, COLOR_BLUE)); // panel
	nassert(init_pair(CPAIR_PANEL_TITLE, COLOR_BLACK, COLOR_WHITE)); // panel title
	nassert(init_pair(CPAIR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN)); // file cursos highlight
	nassert(init_pair(CPAIR_EXECUTABLE, COLOR_GREEN, COLOR_BLUE)); // executable file

	// create panels
	getcwd(path_str, PATH_MAX);
	init_panel(&pleft, rows_panel, cols - cols_panel, 1, 0, path_str); // мб шире
	init_panel(&pright, rows_panel, cols_panel, 1, cols - cols_panel, getenv("HOME"));
	left = pleft.curs_win;
	right = pright.curs_win;

	// create auxillary windows
	nassert(menubar = newwin(1, cols, 0, 0));
	nassert(hintbar = newwin(1, cols, rows - 3, 0));
	nassert(shellbar = newwin(1, cols, rows - 2, 0));
	nassert(fnkeybar = newwin(1, cols, rows - 1, 0));
	menubar->_clear = true;
	hintbar->_clear = true;
	shellbar->_clear = true;
	fnkeybar->_clear = true;

	nassert(wbkgd(menubar, COLOR_PAIR(CPAIR_HIGHLIGHT)));

	int cl_y = max(0, rows - 2);
	int cl_x = 0;
	chtype single_c = 0;

	WINDOW *wnd_arr[] = { left, right, menubar, hintbar, shellbar, fnkeybar };
	//resize_screen(wnd_arr, rows, cols, &rows_panel, &cols_panel, &cl_y);

	while (true) {
		// resize
		int rows_now, cols_now;
		getmaxyx(stdscr, rows_now, cols_now);
		if(rows != rows_now || cols != cols_now) {
			if(resize_screen(wnd_arr, rows_now, cols_now, &rows_panel, &cols_panel, &cl_y)) {
				rows = rows_now;
				cols = cols_now;
				pleft.width = cols_panel;
				pleft.height = rows_panel;
				pright.width = cols_panel;
				pright.height = rows_panel;
				nassert(werase(stdscr));
			}
		}

		// draw
		if(menubar->_clear) {
			menubar->_clear = false;
			nassert(werase(menubar));
			nassert(mvwaddstr(menubar, 0, 1, "Menu"));
		}

		if(hintbar->_clear) {
			hintbar->_clear = false;
			nassert(werase(hintbar));
			mvwprintw(hintbar, 0, 0, "Hint: %s", hint_str);
		}

		if(shellbar->_clear) {
			shellbar->_clear = false;
			nassert(werase(shellbar));
			cl_x = snprintf(shell_str, sizeof(shell_str)
				, "[nosh %.29s]$ ", pcurr->dpath.tail ? pcurr->dpath.tail->value.ent.d_name : "/");
			mvwaddstr(shellbar, 0, 0, shell_str);
			if(single_c != 0)
				mvwaddch(shellbar, 0, cl_x, single_c);
		}
		
		if(fnkeybar->_clear) {
			fnkeybar->_clear = false;
			nassert(werase(fnkeybar));
			mvwaddattrfstr(fnkeybar, 0, cols - 8, 2, "10", A_BOLD);
			nassert(wattron(fnkeybar, COLOR_PAIR(CPAIR_HIGHLIGHT)));
			mvwaddattrfstr(fnkeybar, 0, cols - 6, 5, "Quit", A_NORMAL);
			nassert(wattroff(fnkeybar, COLOR_PAIR(CPAIR_HIGHLIGHT)));
		}

		if(left->_clear) {
			pleft.redraw = true;
			left->_clear = false;
		}

		if(right->_clear) {
			pright.redraw = true;
			right->_clear = false;
		}

		redraw_panel(&pleft);
		redraw_panel(&pright);

		//nassert(werase(stdscr));

		// If two windows overlap, you can refresh them in either order and the overlap region
		// will be modified only when it is explicitly changed.
		// WARNING:
		// Whether wnoutrefresh() copies to the virtual screen the entire contents of a window or just
		// changed portions has never been well-documented in historic curses versions (including SVr4).
		// It might be unwise to rely on either behavior in programs that might have to be linked with
		// other curses implementations.
		// Instead, you can do an explicit touchwin() before the wnoutrefresh() call
		// to guarantee an entire-contents copy anywhere.
		// Source: man wnoutrefresh, line 48
		nassert(wnoutrefresh(stdscr));

		nassert(wnoutrefresh(menubar));
		nassert(wnoutrefresh(hintbar));
		nassert(wnoutrefresh(shellbar));
		nassert(wnoutrefresh(fnkeybar));

		nassert(wnoutrefresh(left));
		nassert(wnoutrefresh(right));

		nassert(doupdate());

		// перемещаем курсор на командную строку
		wmove(stdscr, cl_y, cl_x);

		// handle action
		int64_t raw_key = raw_wgetch(stdscr);

		switch (raw_key) {
			case KEY_UP:
				//pcurr->position = max(0, pcurr->position - 1);
				if(pcurr->position > 0) {
					if(pcurr->top_elem == pcurr->position) {
						vscroll(pcurr, -1);
					}
					--(pcurr->position);
					pcurr->redraw = true;
				}
				break;

			case KEY_DOWN:
				//pcurr->position = min(pcurr->contents.count - 1, pcurr->position + 1);
				if(pcurr->position < pcurr->contents.count - 1) {
					if(pcurr->top_elem + (pcurr->height - 3) - 1 == pcurr->position) {
						vscroll(pcurr, 1);
					}
					++(pcurr->position);
					pcurr->redraw = true;
				}
				break;

			case RAW_KEY_HOME:
			case RAW_KEY_HOME_ALT:
				//vscroll(pcurr, INT_MIN);
				pcurr->top_elem = 0;
				pcurr->position = 0;
				pcurr->redraw = true;
				break;

			case RAW_KEY_END:
			case RAW_KEY_END_ALT:
				vscroll(pcurr, INT_MAX);
				pcurr->position = pcurr->contents.count - 1;
				pcurr->redraw = true;
				break;

			case RAW_KEY_TAB: // no macro for tab, really?..
				switch_tabs(&pleft, &pright, &pcurr);
				shellbar->_clear = true;
				break;

			case RAW_KEY_ENTER:
			case RAW_KEY_NUMPAD_ENTER: // enter
				fs_move(pcurr);
				shellbar->_clear = true;
				break;

			case KEY_F(10):
				goto end_loop;

			default: /*redraw = false;*/
				if ((raw_key >> 8) == 0 && isprint(raw_key)) {
					single_c = raw_key;
					shellbar->_clear = true;
				}
				else {
					snprintf(hint_str, sizeof(hint_str), "pressed key is 0x%lx", raw_key);
					hintbar->_clear = true;
				}
				break;
		}
	}

end_loop:
	nassert(endwin());

	free_panel(&pleft);
	free_panel(&pright);
	return 0;
}
