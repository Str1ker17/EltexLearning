#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void syscall_print_error(const char *x, const char *file, const int line, const int err_no) {
	if (err_no != 0) {
		char *errstr = strerror(err_no);
		fprintf(stderr, "[x] syscall err: '%s' at %s:%d (%d = %s)\n", x, file, line, err_no, errstr);
	}
	else {
		fprintf(stderr, "[x] runtime err: '%s' at %s:%d\n", x, file, line);
	}
}

int syscall_error(const char *x, const char *file, const int line) {
	// Turn off curses mode if we have it
#ifdef CURSES
	endwin();
#endif

	// Get errno and errstr for last syscall
	int err_no = errno;
	syscall_print_error(x, file, line, err_no);
	exit(EXIT_FAILURE);
	return EXIT_FAILURE;
}
