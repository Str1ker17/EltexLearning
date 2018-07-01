.PHONY: solution clean debug release

CFLAGS := -Wall -O3 -march=native
LDFLAGS := -lncursesw

COMPILE := $(CFLAGS) -c
LINK := $(LDFLAGS)

solution: COMPILE := $(CFLAGS) -c
solution: subdirs libncurses_util bc be

subdirs:
	mkdir bin 2> /dev/null || true
	mkdir lib 2> /dev/null || true
	mkdir obj 2> /dev/null || true

libncurses_util: src/libncurses_util/ncurses_util.h src/libncurses_util/ncurses_util.c
	gcc $(COMPILE) -std=c99 -o obj/ncurses_util.o src/libncurses_util/ncurses_util.c
	ar rcs lib/libncurses_util.a obj/ncurses_util.o
	
bc: libncurses_util
	gcc $(COMPILE) -std=c99 -o obj/bc.o src/bc/bc.c
	gcc $(COMPILE) -std=c99 -o obj/dircont.o src/bc/dircont.c
	gcc $(COMPILE) -std=c99 -o obj/dirpath.o src/bc/dirpath.c
	gcc $(LINK) -o bin/bc.exe obj/bc.o obj/dircont.o obj/dirpath.o lib/libncurses_util.a

# fdopen() and fileno() are GNU extension
be: libncurses_util
	gcc $(COMPILE) -std=gnu99 -o obj/be.o src/be/be.c
	gcc $(COMPILE) -std=c99 -o obj/vector.o src/be/vector.c
	gcc $(LINK) -o bin/be.exe obj/be.o obj/vector.o lib/libncurses_util.a
	
clean:
	rm -rf bin
	rm -rf lib
	rm -rf obj
