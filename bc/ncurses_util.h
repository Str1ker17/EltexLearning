#pragma once

#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <ncursesw/ncurses.h>

// this is may be cuz of non-raw keyboard and getting keychars instead of keycodes
#define KEY_TAB 9
#define KEY_RETURN 10

//		} else ((void)0);                                                     
#ifndef _MSC_VER
#define nassert(x)                                                            \
	{                                                                         \
		__typeof(x) y = (x);                                                  \
		if(__builtin_types_compatible_p(__typeof(y), WINDOW*)) {              \
			if((void*)(uintptr_t)(y) == (void*)(uintptr_t)NULL)               \
				ncurses_raise_error((#x), __FILE__, __LINE__);                \
		} else if(__builtin_types_compatible_p(__typeof(y), int)) {           \
			if((int)(intptr_t)(y) == ERR)                                     \
				ncurses_raise_error((#x), __FILE__, __LINE__);                \
		} else ncurses_raise_error("macro error in " #x, __FILE__, __LINE__); \
	}
#else
#define nassert(x) (x)
#endif

#define crassert(x) (void)(!!(x) || ncurses_raise_error((#x), __FILE__, __LINE__))

enum raw_keys {
	  RAW_KEY_TAB = 9
	, RAW_KEY_ENTER = 10
	, RAW_KEY_NUMPAD_ENTER = 343
	, RAW_KEY_HOME = 0x1b5b317e
	, RAW_KEY_END = 0x1b5b347e
};

bool ncurses_raise_error(const char *x, const char *file, const int line);
chtype *create_chstr(char *str, int len, chtype attr);
int mvwaddattrfstr(WINDOW *wnd, int y, int x, int len, char *str, chtype attr);
