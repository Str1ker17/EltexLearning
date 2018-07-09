.PHONY: solution clean subdirs debug release

# CONFIGURATION
ifeq ($(strip $(CC)),)
CC := gcc
endif

LD := $(CC)

CFLAGS := -Wall -Wno-unused-parameter
LDFLAGS := -lncursesw

# := is an immediate expansion, = is a delayed expansion
COMPILE = $(CFLAGS) -c
LINK = $(LDFLAGS)

# BUILD CONFIGURATIONS
release: CFLAGS := -Wextra -Wpedantic -O3 -march=native -D NDEBUG=1 -D RELEASE=1 $(CFLAGS)
release: solution
	echo Built Release

debug: CFLAGS := -Wextra -O0 -ggdb -ffunction-sections -D DEBUG=1 -D _DEBUG=1 $(CFLAGS)
debug: solution
	echo Built Debug

# TARGETS
solution: subdirs libncurses_util bc be

subdirs:
	mkdir bin 2> /dev/null || true
	mkdir lib 2> /dev/null || true
	mkdir obj 2> /dev/null || true

libncurses_util: src/libncurses_util/ncurses_util.h src/libncurses_util/ncurses_util.c
	$(CC) -std=c99 -o obj/ncurses_util.o src/libncurses_util/ncurses_util.c $(COMPILE)
	ar rcs lib/libncurses_util.a obj/ncurses_util.o

bc: libncurses_util
	$(CC) -std=c99 -o obj/bc.o src/bc/bc.c $(COMPILE)
	$(CC) -std=c99 -o obj/dircont.o src/bc/dircont.c $(COMPILE)
	$(CC) -std=c99 -o obj/dirpath.o src/bc/dirpath.c $(COMPILE)
	$(LD) -o bin/bc.exe obj/bc.o obj/dircont.o obj/dirpath.o lib/libncurses_util.a $(LINK)

# fdopen() and fileno() are GNU extensions
# sigaction() seems to be too
be: libncurses_util
	$(CC) -std=gnu99 -o obj/be.o src/be/be.c $(COMPILE)
	$(CC) -std=c99 -o obj/vector.o src/be/vector.c $(COMPILE)
	$(LD) -o bin/be.exe obj/be.o obj/vector.o lib/libncurses_util.a $(LINK)

clean:
	rm -rf bin
	rm -rf lib
	rm -rf obj
