.PHONY: solution clean subdirs debug release

# CONFIGURATION
export CC := gcc
export LD := $(CC)
export CFLAGS :=
export LDFLAGS :=

export ROOTDIR := $(CURDIR)
export BINDIR := $(ROOTDIR)/bin/
export LIBDIR := $(ROOTDIR)/lib/
export OBJDIR := $(ROOTDIR)/obj/
export SRCDIR := $(ROOTDIR)/src/

MKFLG := -sr

# BUILD CONFIGURATIONS
release: CFLAGS := -Wextra -Wpedantic -O3 -march=native -D NDEBUG=1 -D RELEASE=1 $(CFLAGS)
release: solution
	echo Built Release

debug: CFLAGS := -Wextra -O0 -ggdb -ffunction-sections -D DEBUG=1 -D _DEBUG=1 $(CFLAGS)
debug: solution
	echo Built Debug

# TARGETS
solution: projects

projects:
	$(MAKE) $(MKFLG) -C $(SRCDIR) projects

clean:
	$(MAKE) $(MKFLG) -C $(SRCDIR) clean

distclean:
	$(MAKE) $(MKFLG) -C $(SRCDIR) distclean
