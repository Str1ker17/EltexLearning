.PHONY: clean

# твою ж дивизию! определение переменных не требует таба!
ifeq ($(MAKELEVEL),0)
CC := gcc
LD := $(CC)
OBJDIR := ./
endif

CFLAGS_LOCAL := -std=c99
LDFLAGS_LOCAL :=

ifeq ($(strip($(OBJDIR))),)
endif

ifeq ($(strip($(LIBDIR))),)
# TODO
#LDFLAGS_LOCAL := $(wildcard ../*/.)$(LDFLAGS_LOCAL)
endif

HEADERS := $(wildcard *.h)
SOURCES := $(wildcard *.c)
OBJECTS := $(SOURCES:.c=.o)

$(BINDIR)bfcopy.exe: $(OBJECTS) $(LIBDIR)libcopyfile.a $(LIBDIR)libncurses_util.a
	cd $(OBJDIR) && $(LD) -o $@ $^ $(LDFLAGS) $(LDFLAGS_LOCAL)

$(OBJECTS): $(HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_LOCAL) -c -o $(OBJDIR)$@ $<

clean:
	rm -f $(BINDIR)*.exe
	rm -f $(OBJDIR)*.o
