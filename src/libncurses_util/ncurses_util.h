#pragma once

#if defined(_MSC_VER)
#undef __cplusplus
#endif

#include <stdbool.h>
#include <stdint.h>
#include <ncursesw/ncurses.h>

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

#define custom_assert(x, y) (void)(!!(x) || y((#x), __FILE__, __LINE__))

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

enum raw_keys {
	  RAW_KEY_ERR = -1
    , RAW_KEY_TAB = 9
    , RAW_KEY_ENTER = 10
    , RAW_KEY_NUMPAD_ENTER = 343
    , RAW_KEY_ESC = 27
    , RAW_KEY_HOME = 0x1b5b317e
    , RAW_KEY_HOME_ALT = 0x106
    , RAW_KEY_END = 0x1b5b347e
    , RAW_KEY_END_ALT = 0x168
    , RAW_KEY_PAGE_DOWN = 0x152
    , RAW_KEY_PAGE_UP = 0x153
};

bool ncurses_raise_error(const char *x, const char *file, const int line);
chtype *create_chstr(char *str, int len, chtype attr);
int mvwaddattrfstr(WINDOW *wnd, int y, int x, int len, char *str, chtype attr);
int64_t raw_wgetch(WINDOW *wnd);
