.PHONY: clean

# твою ж дивизию! определение переменных не требует таба!
ifeq ($(MAKELEVEL),0)
CC := gcc
LD := $(CC)
endif

ifeq ($(strip $(OBJDIR)),)
OBJDIR := ./
endif

CFLAGS_LOCAL := -std=c99

HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)

$(LIBDIR)libncurses_util.a: $(OBJECTS)
	cd $(OBJDIR) && ar rcs $@ $^
	@echo -e \\x1b[32m AR \\x1b[0m $(shell realpath --relative-to $(ROOTDIR) $@)

$(OBJECTS): $(HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_LOCAL) -c -o $(OBJDIR)$@ $<
	@echo -e \\x1b[33m CC \\x1b[0m $(shell realpath --relative-to $(ROOTDIR) $(OBJDIR)$@)

clean:
	rm -f $(LIBDIR)*.a
	rm -f $(OBJDIR)*.o
