#if defined(_MSC_VER)
#undef __cplusplus
#endif
#include <stdio.h>
#include <ncursesw/ncurses.h>
#include <stdbool.h>
#include <locale.h>

WINDOW *left, *right;

int main(int argc, char **argv) {
	freopen("logfile.log", "w", stderr);
	int rows, cols;

	char hint_str[200] = "\0";

	// The library uses the locale which the calling program has initialized.
	// If the locale is not initialized, the library assumes that characters
    // are printable as in ISO-8859-1, to work with certain legacy programs.
    // You should initialize the locale and not rely on specific details of
    // the library when the locale has not been setup.
	// Source: man ncurses, line 30
	setlocale(LC_ALL, "");

	// https://code-live.ru/post/cpp-ncurses-hello-world
	initscr();

	//noecho();
	cbreak();
	intrflush(stdscr, false);
	keypad(stdscr, true);

	getmaxyx(stdscr, rows, cols);

	int cols_panel = cols / 2;
	int rows_panel = rows - 4; // заголовок и 3 строчки снизу

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_WHITE, COLOR_BLACK);

	left = newwin(rows_panel, cols_panel, 1, 0);
	wbkgd(left, COLOR_PAIR(1));
	right = newwin(rows_panel, cols_panel, 1, cols - cols_panel);
	wbkgd(right, COLOR_PAIR(1));

	bool active_left = true;
	//bool switch_panel = true;
	int c;
	//int x = cols / 2 - 7, y = rows / 2;
	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	int *x, *y;

	int cl_x = 16, cl_y = rows - 2;

	bool redraw = true;
	while (true) {
		if (active_left) {
			x = &x1, y = &y1;
		}
		else {
			x = &x2, y = &y2;
		}

		if(redraw) {
			// draw
			wclear(left);
			wclear(right);
			clear();

			mvwaddstr(stdscr, 0, 0, "Menu");
			mvwaddstr(stdscr, rows - 3, 0, "Hint: ");
				mvwaddstr(stdscr, rows - 3, 6, hint_str);
			mvwaddstr(stdscr, rows - 2, 0, "[Command line]$ ");
			mvwaddstr(stdscr, rows - 1, 0, "Functional keys. F10: Exit");

			box(left, ACS_VLINE, ACS_HLINE);
			box(right, ACS_VLINE, ACS_HLINE);

			// highlight panel
			wattron(active_left ? left : right, COLOR_PAIR(2));

			char str1[] = "Left panel";
			mvwaddstr(left, rows_panel / 2 + y1, cols_panel / 2 - sizeof(str1) / 2 + x1, str1);

			char str2[] = "Right panel";
			mvwaddstr(right, rows_panel / 2 + y2, cols_panel / 2 - sizeof(str2) / 2 + x2, str2);

			wattroff(active_left ? left : right, COLOR_PAIR(2));
			// highlight end

			refresh();
			wrefresh(left);
			wrefresh(right);

			// перемещаем курсор на командную строку
			wmove(stdscr, cl_y, cl_x);
			//switch_panel = false;
		}

		// handle action
		redraw = true;
		c = getch();
		switch(c) {
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
				active_left = !active_left;
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
