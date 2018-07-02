.PHONY: clean

#ifeq($(MAKELEVEL),0)
CC := gcc
CFLAGS := -std=c99
#endif

HEADERS = $(wildcard *.h)
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

libncurses_util.a: $(OBJECTS)
	ar rcs $@ $?

$(OBJECTS): $(HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f ./*.a
	rm -f ./*.o
