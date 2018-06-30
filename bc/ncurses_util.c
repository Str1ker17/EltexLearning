#include "ncurses_util.h"
#include <stdlib.h>

bool ncurses_raise_error(const char *x, const char *file, const int line) {
	endwin();
	fflush(stdout);
	fprintf(stdout, "[x] ncurses err: '%s' at %s:%d\n", x, file, line);
	fprintf(stderr, "[x] ncurses err: '%s' at %s:%d\n", x, file, line);
	exit(1);
	return false;
}

chtype *create_chstr(char *str, int len, chtype attr) {
	chtype *chstr = (chtype*)malloc(sizeof(chtype) * len + 1);
	for(int i = 0; i < len; i++) {
		chstr[i] = ((chtype)str[i]) | attr;
	}
	chstr[len] = 0;
	return chstr;
}

// move, add attributed string with fixed length
int mvwaddattrfstr(WINDOW *wnd, int y, int x, int len, char *str, chtype attr) {
	int pos = 0;
	while(*str != '\0' && pos < len) {
		if(wmove(wnd, y, x) == ERR) return ERR;
		if(waddch(wnd, (*str) | attr) == ERR) return ERR;
		str++;
		x++;
		pos++;
	}
	for(int i = pos; i < len; i++) {
		if(wmove(wnd, y, x) == ERR) return ERR;
		if(waddch(wnd, ' ' | attr) == ERR) return ERR; // add spaces
		x++;
		pos++;
	}
	return 0; // OK
}
