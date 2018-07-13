.PHONY: clean

# твою ж дивизию! определение переменных не требует таба!
ifeq ($(MAKELEVEL),0)
CC := gcc
OBJDIR := ./
endif

ifeq ($(strip($(OBJDIR))),)
endif

CFLAGS_LOCAL := -std=gnu99

HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)

$(LIBDIR)libeditor.a: $(OBJECTS)
	cd $(OBJDIR) && ar rcs $@ $^

$(OBJECTS): $(HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_LOCAL) -c -o $(OBJDIR)$@ $<

clean:
	rm -f $(LIBDIR)*.a
	rm -f $(OBJDIR)*.o
