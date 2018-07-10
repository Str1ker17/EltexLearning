.PHONY: solution clean subdirs debug release

# CONFIGURATION
ifeq ($(strip $(CC)),)
CC := gcc
endif

LD := $(CC)

CFLAGS := -Wall -Wno-unused-parameter -std=c99
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
solution: subdirs lib/libncurses_util.a lib/libeditor.a bin/bterm.exe bin/bc.exe bin/be.exe

subdirs:
	mkdir bin 2> /dev/null || true
	mkdir lib 2> /dev/null || true
	mkdir obj 2> /dev/null || true

lib/libncurses_util.a: src/libncurses_util/ncurses_util.h src/libncurses_util/ncurses_util.c src/libncurses_util/linux_util.h src/libncurses_util/linux_util.c
	$(CC) -o obj/ncurses_util.o src/libncurses_util/ncurses_util.c $(COMPILE)
	$(CC) -o obj/linux_util.o src/libncurses_util/linux_util.c $(COMPILE)
	ar rcs lib/libncurses_util.a obj/ncurses_util.o obj/linux_util.o
	
# fdopen() and fileno() are GNU extensions
# sigaction() seems to be too
lib/libeditor.a: src/libeditor/libeditor.h src/libeditor/libeditor.c src/libeditor/vector.h src/libeditor/vector.c
	$(CC) -o obj/libeditor.o src/libeditor/libeditor.c $(COMPILE) -std=gnu99
	$(CC) -o obj/vector.o src/libeditor/vector.c $(COMPILE)
	ar rcs lib/libeditor.a obj/libeditor.o obj/vector.o
	
bin/bterm.exe: lib/libncurses_util.a src/bterm/bterm.c
	$(CC) -o obj/bterm.o src/bterm/bterm.c $(COMPILE)
	$(LD) -o bin/bterm.exe obj/bterm.o lib/libncurses_util.a $(LINK)

bin/bc.exe: lib/libncurses_util.a src/bc/bc.c src/bc/dircont.h src/bc/dircont.c src/bc/dirpath.h src/bc/dirpath.c
	$(CC) -o obj/bc.o src/bc/bc.c $(COMPILE)
	$(CC) -o obj/dircont.o src/bc/dircont.c $(COMPILE)
	$(CC) -o obj/dirpath.o src/bc/dirpath.c $(COMPILE)
	$(LD) -o bin/bc.exe obj/bc.o obj/dircont.o obj/dirpath.o lib/libncurses_util.a $(LINK)

bin/be.exe: lib/libncurses_util.a lib/libeditor.a src/be/be.c
	$(CC) -o obj/be.o src/be/be.c $(COMPILE)
	$(LD) -o bin/be.exe obj/be.o $(LINK) lib/libeditor.a lib/libncurses_util.a

clean:
	rm -rf bin
	rm -rf lib
	rm -rf obj
