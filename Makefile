.PHONY: solution clean distclean debug release

# CONFIGURATION
ifeq ($(strip $(CC)),)
CC := gcc
endif

LD := $(CC)
CFLAGS :=
LDFLAGS :=

ROOTDIR := $(CURDIR)
BINDIR := $(ROOTDIR)/bin/
LIBDIR := $(ROOTDIR)/lib/
OBJDIR := $(ROOTDIR)/obj/
SRCDIR := $(ROOTDIR)/src/

# save time by omitting `default' targets
MKFLG := -r

# export ALL variables
export

# BUILD CONFIGURATIONS
release: CFLAGS := -Wextra -Wpedantic -O3 -march=native -D NDEBUG=1 -D RELEASE=1 $(CFLAGS)
release: solution
	echo Built Release

debug: CFLAGS := -Wextra -O0 -ggdb -ffunction-sections -D DEBUG=1 -D _DEBUG=1 $(CFLAGS)
debug: solution
	echo Built Debug

# TARGETS
solution:
	$(MAKE) $(MKFLG) -C $(SRCDIR) projects

clean:
	$(MAKE) $(MKFLG) -C $(SRCDIR) clean

distclean:
	$(MAKE) $(MKFLG) -C $(SRCDIR) distclean

# last resort target, to redirect all lower level targets
# https://www.gnu.org/software/make/manual/html_node/Last-Resort.html
%::
	$(MAKE) $(MKFLG) -C $(SRCDIR) $@
