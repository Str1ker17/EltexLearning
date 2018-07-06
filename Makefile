.PHONY: solution clean debug release

ifeq($(CC),)
CC := gcc
endif

LD := $(CC)

CFLAGS := -Wall -O3 -march=native
LDFLAGS := -lncursesw

# := is an implicit expansion, = is a delayed expansion
COMPILE = $(CFLAGS) -c
LINK = $(LDFLAGS)

# BUILD CONFIGURATIONS
debug: CFLAGS := -Wextra -O0 -ggdb -ffunction-sections -D DEBUG=1 -D _DEBUG=1
debug: solution
	echo Built Debug
	
release: CFLAGS := -Wextra -Wpedantic -O3 -march=native -D NDEBUG=1 -D RELEASE=1
release: solution
	echo Built Release

# TARGETS
solution: subdirs libncurses_util bc be

subdirs:
	mkdir bin 2> /dev/null || true
	mkdir lib 2> /dev/null || true
	mkdir obj 2> /dev/null || true

libncurses_util: src/libncurses_util/ncurses_util.h src/libncurses_util/ncurses_util.c
	$(CC) $(COMPILE) -std=c99 -o obj/ncurses_util.o src/libncurses_util/ncurses_util.c
	ar rcs lib/libncurses_util.a obj/ncurses_util.o

bc: libncurses_util
	$(CC) $(COMPILE) -std=c99 -o obj/bc.o src/bc/bc.c
	$(CC) $(COMPILE) -std=c99 -o obj/dircont.o src/bc/dircont.c
	$(CC) $(COMPILE) -std=c99 -o obj/dirpath.o src/bc/dirpath.c
	$(LD) -o bin/bc.exe obj/bc.o obj/dircont.o obj/dirpath.o lib/libncurses_util.a $(LINK)

# fdopen() and fileno() are GNU extensions
# sigaction seems to be too
be: libncurses_util
	$(CC) $(COMPILE) -std=gnu99 -o obj/be.o src/be/be.c
	$(CC) $(COMPILE) -std=c99 -o obj/vector.o src/be/vector.c
	$(LD) -o bin/be.exe obj/be.o obj/vector.o lib/libncurses_util.a $(LINK)

clean:
	rm -rf bin
	rm -rf lib
	rm -rf obj
