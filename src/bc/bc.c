/*
 * Bionicle Commander v0.1.2
 * @Author: Str1ker
 * @Created: 28 Jun 2018
 * @Modified: 11 Jul 2018
 */

#if defined(_MSC_VER)
#undef __cplusplus
#define __builtin_alloca(size) NULL
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <ncursesw/ncurses.h>
#include <locale.h>

#include <signal.h>
#include <linux/limits.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <alloca.h>

#include "dircont.h"
#include "dirpath.h"
#include "../libncurses_util/ncurses_util.h"
#include "../liblinux_util/linux_util.h"

// количество строк, зарезервированных под что-то, кроме списка файлов
#define PANEL_SPEC_LINES 3

typedef struct {
	WINDOW *curs_win; // ncurses-окно
	//WINDOW *curs_pad; // ncurses-pad, не реализовано
	char *path; // текущая директория в виде строки
	DIRPATH dpath; // путь до текущей директории в виде связного списка
	DIRCONT contents; // содержимое текущей директории (файлы и директории) в виде связного списка
	size_t top_elem; // смещение ncurses-окна относительно верха (прокрутка)
	size_t position; // индекс выделенного элемента в contents
	size_t width;
	size_t height;
	bool active; // активна ли эта панель
	bool redraw; // требуется ли перерисовка
} BCPANEL;

typedef enum {
	  CPAIR_DEFAULT = 1
	, CPAIR_PANEL
	, CPAIR_PANEL_TITLE
	, CPAIR_HIGHLIGHT
	, CPAIR_EXECUTABLE
	, CPAIR_TYPE_DIR = CPAIR_PANEL
	, CPAIR_TYPE_REG = CPAIR_PANEL
	, CPAIR_TYPE_SYMLINK = CPAIR_EXECUTABLE + 1
	, CPAIR_TYPE_CHR
	, CPAIR_TYPE_BLK
	, CPAIR_TYPE_FIFO
	, CPAIR_TYPE_SOCKET
} cpair_list;

typedef enum {
	  FS_ACTION_ENTER = 0 // cd to directory, run executable file, run associated program for others
	, FS_ACTION_VIEW = 3 // F3
	, FS_ACTION_EDIT = 4 // F4
	, FS_ACTION_COPY = 5 // F5
} fs_action_e;

// creative place
/*int __verbose_chdir(const char *path) {
	fprintf(stderr, "chdir(%s)\n", path);
	return chdir(path);
}

#define chdir(path) __verbose_chdir(path)*/

void signal_winch_handler(int signo) {
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, (char*)&size);
	if(size.ws_row < 5)
		return; // do not resize is less than
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

	dcl_init(&panel->contents);
	dpt_init(&panel->dpath, path);
	panel->path = (char*)malloc(PATH_MAX); // minimize mallocs count
	dpt_string(&panel->dpath, panel->path);
}

void free_panel(BCPANEL *panel) {
	dcl_clear(&panel->contents);
	dcl_clear(&panel->dpath);
	free(panel->path);
}

void enter_curses_mode() {
	nassert(initscr());
	//nassert(cbreak());
	nassert(raw());
	nassert(noecho());
	nassert(intrflush(stdscr, false));
	nassert(keypad(stdscr, true));
}

void leave_curses_mode() {
	nassert(endwin());
}

void invalidate_screen(WINDOW **wnd_arr) {
	for(int i = 0; i < 6; i++) {
		wnd_arr[i]->_clear = true;
	}
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
	nassert(mvwin(menu_bar, 0, 0)); // redundant
	nassert(wresize(hint_bar, 1, cols));
	nassert(mvwin(hint_bar, max(1, rows - 3), 0));
	nassert(wresize(shell_bar, 1, cols));
	nassert(mvwin(shell_bar, max(1, rows - 2), 0));
	nassert(wresize(fnkey_bar, 1, cols));
	nassert(mvwin(fnkey_bar, max(1, rows - 1), 0));

	invalidate_screen(wnd_arr);

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
	if(S_ISDIR(left->st.st_mode) && !S_ISDIR(right->st.st_mode)) return -1;
	if(!S_ISDIR(left->st.st_mode) && S_ISDIR(right->st.st_mode)) return 1;

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

bool reread_files(BCPANEL *panel) {
	struct stat st = { .st_ino = 0 };
	__syscall(stat(panel->path, &st)); // TODO: check for error
	if(st.st_ino == panel->contents.ino_self)
		return true; // already there

	// пытаемся открыть папку, при неудаче откатываемся на уровень выше
	DIR *dir = opendir(panel->path);
	if(dir == NULL)
		return false;
	
	dcl_clear(&panel->contents);
	__syscall(chdir(panel->path));

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
		__syscall(stat(entry->d_name, &ent.st));
		lassert(dcl_push_back(&panel->contents, &ent));
	}

	__syscall(closedir(dir)); // skip return value?
	panel->contents.ino_self = st.st_ino;

	dcl_quick_sort(&panel->contents, &fs_comparator);
	return true;
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
		if(pos < panel->top_elem) {
			++pos;
			continue;
		}

		// tune item look. default is __S_IFREG
		char spec = ' ';
		chtype attr = A_NORMAL;
		int cpair = CPAIR_TYPE_REG;

		// from sys/stat.h:
		// /* File types.  */
		// #define	__S_IFDIR	0040000	/* Directory.  */
		// #define	__S_IFCHR	0020000	/* Character device.  */
		// #define	__S_IFBLK	0060000	/* Block device.  */
		// #define	__S_IFREG	0100000	/* Regular file.  */
		// #define	__S_IFIFO	0010000	/* FIFO.  */
		// #define	__S_IFLNK	0120000	/* Symbolic link.  */
		// #define	__S_IFSOCK	0140000	/* Socket.  */

		__mode_t st_mode = curr->st.st_mode;
		__mode_t st_type = st_mode & __S_IFMT;

		if ((st_mode & __S_IFREG) && (st_mode & __S_IEXEC)) {
			spec = '*'; attr = A_BOLD; cpair = CPAIR_EXECUTABLE;
		}

		switch(st_type) {
			// S_IFLNK won't work with stat(), only lstat()
			case __S_IFLNK:  { spec = '@'; cpair = CPAIR_TYPE_SYMLINK; } break;
			case __S_IFSOCK: { spec = '='; cpair = CPAIR_TYPE_SOCKET; } break;
			case __S_IFDIR:  { spec = '/'; cpair = CPAIR_TYPE_DIR; attr = A_BOLD; } break;
			case __S_IFBLK:  { spec = '+'; cpair = CPAIR_TYPE_BLK; attr = A_BOLD; } break;
			case __S_IFCHR:  { spec = '-'; cpair = CPAIR_TYPE_CHR; attr = A_BOLD; } break;
			case __S_IFIFO:  { spec = '|'; cpair = CPAIR_TYPE_FIFO; } break;
			default: {
				if(st_type != __S_IFREG)
					logprint("[!] Type of file '%s' is not S_IFREG: %08x\n"
						, curr->ent.d_name, htonl(st_type));
			}
		}

		if (panel->active && panel->position == pos) cpair = CPAIR_HIGHLIGHT;

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

bool fs_chdir(BCPANEL *panel, char *path) {
	if(chdir(path) == -1)
		return false;

	char prev_path[4096] = "\0";
	strcpy(prev_path, panel->path);
	struct dirent prev = { .d_name = "\0" };
	if(panel->dpath.tail != NULL)
		strcpy(prev.d_name, panel->dpath.tail->value.ent.d_name);

	lassert(dpt_move(&panel->dpath, path));
	dpt_string(&panel->dpath, panel->path);

	if(!reread_files(panel)) {
		__syscall(chdir(prev_path));
		return false;
	}

	panel->position = 0;
	panel->top_elem = 0;

	// save prev position maybe?
	if(strcmp(path, "..") == 0) {
		 // up
		DIRCONT_LIST_ENTRY *cursor = NULL;
		DIRCONT_ENTRY *cur;
		size_t pos = 0;
		while ((cur = dcl_next_r(&panel->contents, &cursor)) != NULL) {
			if(strcmp(cur->ent.d_name, prev.d_name) == 0) {
				panel->position = pos;
				break;
			}
			++pos;
		}
		// TODO: improve to disable up-scrolling
		ssize_t bot_elem = min(panel->position + 5, panel->contents.count);
		ssize_t top_elem = max(0, bot_elem - ((((ssize_t)(panel->height)) - PANEL_SPEC_LINES)));
		panel->top_elem = top_elem;
	}

	panel->redraw = true;
	return true;
}

bool fs_exec(BCPANEL *panel, const char *filename, fs_action_e action) {
	bool ret = true;
	leave_curses_mode();

	pid_t pid;
	int stat_loc;
	int exec_ret = 0;
	bool *exec_failure = alloca(sizeof(bool));
	char exec_name[NAME_MAX + 2 + 1] = "./";
	strcpy(exec_name + 2, filename);

	switch(pid = fork()) {
		case -1: {
			*exec_failure = true;
			syscall_print_error("fork() failed", __FILE__, __LINE__, errno);
			ret = false;
		} break;

		case 0: { // child
			lassert(dup2(STDOUT_FILENO, STDERR_FILENO) == STDERR_FILENO);

			switch(action) {
				case FS_ACTION_ENTER: exec_ret = execl(exec_name, exec_name, NULL); break;
				case FS_ACTION_VIEW: exec_ret = execlp("less", "less", exec_name, NULL); break;
				case FS_ACTION_EDIT: exec_ret = execlp("nano", "nano", exec_name, NULL); break;
				default: printf("Wrong action!\n"); *exec_failure = true; break;
			}

			if(exec_ret == -1) {
				*exec_failure = true;
				syscall_print_error("execl() failed", __FILE__, __LINE__, errno);
			}
			// unconditional exit
			exit(EXIT_FAILURE);
		} break;

		default: { // parent, actions performed on successful fork()
			lassert(waitpid(pid, &stat_loc, 0) != -1);
			if(action == FS_ACTION_ENTER) {
				int ret_code = (stat_loc >> 8) & 0xff;
				if (ret_code == 0) printf(ANSI_BACKGROUND_GREEN);
				else printf(ANSI_BACKGROUND_RED);
				printf(ANSI_COLOR_WHITE "%s" ANSI_COLOR_RESET ": Process exited with code %d\n"
					, exec_name, ret_code);
			}
		} break;
	}

	if(action == FS_ACTION_ENTER || *exec_failure) {
		printf("Press any key to continue... ");
		getchar();
	}

	enter_curses_mode();
	return ret;
}

bool fs_action(BCPANEL *panel, fs_action_e action) {
	DIRCONT *dcl = &panel->contents;
	// смотрим, в какую по списку папку нужно перейти
	DIRCONT_ENTRY curr = *dcl_at(dcl, panel->position); // создать КОПИЮ

	switch(action) {
		case FS_ACTION_ENTER: {
			if(S_ISDIR(curr.st.st_mode))
				return fs_chdir(panel, curr.ent.d_name);
			if(S_ISREG(curr.st.st_mode) && curr.st.st_mode & __S_IEXEC)
				return fs_exec(panel, curr.ent.d_name, action);

			// we can do nothing on other types
			return false;
		}

		case FS_ACTION_VIEW: {
			if(S_ISREG(curr.st.st_mode) && curr.st.st_mode & __S_IREAD)
				return fs_exec(panel, curr.ent.d_name, action);

			// we can do nothing on other types
			return false;
		}

		case FS_ACTION_EDIT: {
			if(S_ISREG(curr.st.st_mode) && curr.st.st_mode & __S_IWRITE)
				return fs_exec(panel, curr.ent.d_name, action);

			// we can do nothing on other types
			return false;
		}

		default: return false; // unknown action
	}
}

void switch_tabs(BCPANEL *left, BCPANEL *right, BCPANEL **curr) {
	BCPANEL *curr_ptr = *curr;
	curr_ptr->active = false;
	curr_ptr->redraw = true;
	*curr = (curr_ptr == left ? right : left);

	curr_ptr = *curr;
	curr_ptr->active = true;
	curr_ptr->redraw = true;

	chdir(curr_ptr->path);
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

	WINDOW *menubar, *hintbar, *shellbar, *fnkeybar;

	BCPANEL pleft = { .active = true }, pright = { .active = false };
	BCPANEL *pcurr = &pleft;

	int rows, cols;

	char hint_str[200] = "\0";
	char shell_str[40] = "\0";
	char path_str[PATH_MAX] = "\0";

	// https://code-live.ru/post/cpp-ncurses-hello-world
	enter_curses_mode();

	getmaxyx(stdscr, rows, cols);
	lassert(rows >= 5);

	sigset_t set;
	__syscall(sigfillset(&set));
	//__syscall(sigdelset(&set, SIGWINCH));
	struct sigaction sga = {
		  .sa_mask = set
		, .sa_flags = SA_RESTART
		, .sa_restorer = (void(*)(void))NULL
		, .sa_handler = signal_winch_handler
	};
	__syscall(sigaction(SIGWINCH, &sga, NULL));

	int cols_panel = cols / 2;
	int rows_panel = rows - 4; // заголовок и 3 строчки снизу

	nassert(start_color());
	nassert(init_pair(CPAIR_DEFAULT, COLOR_WHITE, COLOR_BLACK)); // non-panel
	nassert(init_pair(CPAIR_PANEL, COLOR_WHITE, COLOR_BLUE)); // panel
	nassert(init_pair(CPAIR_PANEL_TITLE, COLOR_BLACK, COLOR_WHITE)); // panel title
	nassert(init_pair(CPAIR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN)); // file cursos highlight
	nassert(init_pair(CPAIR_EXECUTABLE, COLOR_GREEN, COLOR_BLUE)); // executable file

	nassert(init_pair(CPAIR_TYPE_SYMLINK, COLOR_WHITE, COLOR_BLUE)); // symlink
	nassert(init_pair(CPAIR_TYPE_CHR, COLOR_MAGENTA, COLOR_BLUE)); // char file
	nassert(init_pair(CPAIR_TYPE_BLK, COLOR_MAGENTA, COLOR_BLUE)); // block file
	nassert(init_pair(CPAIR_TYPE_FIFO, COLOR_BLACK, COLOR_BLUE)); // named pipe
	nassert(init_pair(CPAIR_TYPE_SOCKET, COLOR_BLACK, COLOR_BLUE)); // socket

	// create panels
	getcwd(path_str, PATH_MAX);
	init_panel(&pleft, rows_panel, cols - cols_panel, 1, 0, path_str); // мб шире
	init_panel(&pright, rows_panel, cols_panel, 1, cols - cols_panel, getenv("HOME"));
	reread_files(&pright);
	reread_files(&pleft); // left is active by default

	WINDOW *left = pleft.curs_win;
	WINDOW *right = pright.curs_win;

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
			cl_x = snprintf(shell_str, sizeof(shell_str), "[nosh %.*s]$ "
				, (int)pcurr->width, pcurr->dpath.tail ? pcurr->dpath.tail->value.ent.d_name : "/");
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
				if(pcurr->position > 0) {
					if(pcurr->top_elem == pcurr->position) {
						vscroll(pcurr, -1);
					}
					--(pcurr->position);
					pcurr->redraw = true;
				}
				break;

			case KEY_DOWN:
				if(pcurr->position < pcurr->contents.count - 1) {
					if(pcurr->top_elem + (pcurr->height - PANEL_SPEC_LINES) - 1 == pcurr->position) {
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
				fs_action(pcurr, FS_ACTION_ENTER);
				shellbar->_clear = true;
				break;

			case RAW_KEY_F3:
			case KEY_F(3):
				fs_action(pcurr, FS_ACTION_VIEW);
				break;

			case RAW_KEY_F4:
			case KEY_F(4):
				fs_action(pcurr, FS_ACTION_EDIT);
				break;

			case KEY_F(10):
				goto end_loop;

			default: /*redraw = false;*/
				if ((raw_key >> 8) == 0 && isprint(raw_key)) {
					single_c = raw_key;
					shellbar->_clear = true;
				}
				else {
					snprintf(hint_str, sizeof(hint_str) - 1, "pressed key is 0x%lx", raw_key);
					hintbar->_clear = true;
				}
				break;
		}
	}

end_loop:
	leave_curses_mode();

	free_panel(&pleft);
	free_panel(&pright);
	return 0;
}
