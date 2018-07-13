#pragma once

// https://stackoverflow.com/a/3219471/1543625
// Don't bother with libraries, the code is really simple.
#define ANSI_COLOR_BLACK         "\x1b[30m"
#define ANSI_COLOR_RED           "\x1b[31m"
#define ANSI_COLOR_GREEN         "\x1b[32m"
#define ANSI_COLOR_YELLOW        "\x1b[33m"
#define ANSI_COLOR_BLUE          "\x1b[34m"
#define ANSI_COLOR_MAGENTA       "\x1b[35m"
#define ANSI_COLOR_CYAN          "\x1b[36m"
#define ANSI_COLOR_WHITE         "\x1b[37m"

// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_BACKGROUND_BLACK    "\x1b[40m"
#define ANSI_BACKGROUND_RED      "\x1b[41m"
#define ANSI_BACKGROUND_GREEN    "\x1b[42m"
#define ANSI_BACKGROUND_YELLOW   "\x1b[43m"
#define ANSI_BACKGROUND_BLUE     "\x1b[44m"
#define ANSI_BACKGROUND_MAGENTA  "\x1b[45m"
#define ANSI_BACKGROUND_CYAN     "\x1b[46m"
#define ANSI_BACKGROUND_WHITE    "\x1b[47m"

#define ANSI_COLOR_RESET         "\x1b[0m"

// output
#define logprint(...) fprintf(stderr, __VA_ARGS__)

// assert
#define lassert(x) (void)((!!(x)) || syscall_error((#x), __FILE__, __LINE__))
#define __syscall(x) lassert((x) != -1)

// debug
#define VERBOSE if(verbose)

// exported functions
int syscall_error(const char *x, const char *file, const int line);

void syscall_print_error(const char *x, const char *file, const int line, const int err_no);
