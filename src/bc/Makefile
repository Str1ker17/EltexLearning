.PHONY: clean

# твою ж дивизию! определение переменных не требует таба!
ifeq ($(MAKELEVEL),0)
CC := gcc
LD := $(CC)
endif

ifeq ($(strip $(OBJDIR)),)
OBJDIR := ./
endif

CFLAGS_LOCAL := -std=gnu99
LDFLAGS_LOCAL := -lncursesw

HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)

$(BINDIR)bc.exe: $(OBJECTS) $(LIBDIR)libeditor.a $(LIBDIR)libcopyfile.a $(LIBDIR)libncurses_util.a $(LIBDIR)liblinux_util.a
	cd $(OBJDIR) && $(LD) -o $@ $^ $(LDFLAGS) $(LDFLAGS_LOCAL)
	@echo -e \\x1b[96m LD \\x1b[0m $(shell realpath --relative-to $(ROOTDIR) $@)

$(OBJECTS): $(HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_LOCAL) -c -o $(OBJDIR)$@ $<
	@echo -e \\x1b[33m CC \\x1b[0m $(shell realpath --relative-to $(ROOTDIR) $(OBJDIR)$@)

clean:
	rm -f $(BINDIR)*.exe
	rm -f $(OBJDIR)*.o
