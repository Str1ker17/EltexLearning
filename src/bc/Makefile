.PHONY: solution clean

#ifeq($(MAKELEVEL),0)
CC := gcc
CFLAGS := -std=c99
LDFLAGS := -lcurses
# "ncursesw panel" => "-lncursesw -lpanel"
#endif

HEADERS = $(wildcard *.h)
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
LIBPATH := ../libncurses_util
STATIC_LIBRARIES := libncurses_util.a
#SHARED_LIBRARIES := -lncursesw

solution: bc.exe

include $(LIBPATH)/Makefile

bc.exe: $(OBJECTS) $(STATIC_LIBRARIES)
	$(CC) -o $@ $(OBJECTS) $(LIBPATH)/$(STATIC_LIBRARIES) $(LDFLAGS)

$(OBJECTS): $(HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f ./*.exe
	rm -f ./*.o
