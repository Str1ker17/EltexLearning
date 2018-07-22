.PHONY: default solution debug release clean distclean

# CONFIGURATION
ifeq ($(strip $(CC)),)
CC := gcc
endif

LD := $(CC)
CFLAGS := -Wno-unused-parameter
LDFLAGS :=

ROOTDIR := $(CURDIR)
BINDIR := $(ROOTDIR)/bin/
LIBDIR := $(ROOTDIR)/lib/
OBJDIR := $(ROOTDIR)/obj/
SRCDIR := $(ROOTDIR)/src/

# -r: save time by omitting `default' targets
# -R: avoid auto-setting of CC, LD and some other variables
# -s: silence is golden, enjoy short compilation messages
MKFLG := rRs

# export ALL variables
export

CFLAGS := -Wextra -pedantic -O3 -march=native -D NDEBUG=1 -D RELEASE=1 $(CFLAGS)
override CONFIGURATION := release

# BUILD CONFIGURATIONS
release: CFLAGS := -Wextra -pedantic -O3 -march=native -D NDEBUG=1 -D RELEASE=1 $(CFLAGS)
release: override CONFIGURATION := release
release: solution

debug: CFLAGS := -Wextra -pedantic -O0 -ggdb -ffunction-sections -D DEBUG=1 -D VALGRIND_SUCKS $(CFLAGS)
debug: override CONFIGURATION := debug
debug: solution

# TARGETS
solution:
	$(MAKE) -$(MKFLG)C $(SRCDIR) projects

clean:
	$(MAKE) -$(MKFLG)C $(SRCDIR) clean

distclean:
	$(MAKE) -$(MKFLG)C $(SRCDIR) distclean

# last resort target, to redirect all lower level targets
# https://www.gnu.org/software/make/manual/html_node/Last-Resort.html
%::
	$(MAKE) -$(MKFLG)C $(SRCDIR) $@
