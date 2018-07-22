.PHONY: subdirs projects clean distclean last-config

# твою ж дивизию! определение переменных не требует таба!
# на всякий случай
ifeq ($(MAKELEVEL),0)
ROOTDIR := $(realpath $(CURDIR)/../)
BINDIR := $(ROOTDIR)/bin/
LIBDIR := $(ROOTDIR)/lib/
OBJDIR := $(ROOTDIR)/obj/
SRCDIR := $(ROOTDIR)/src/
endif

# DEFAULT COMPILER
ifeq ($(strip $(CC)),)
CC := gcc
endif

# avoid usage of `ld' itself
LD := $(CC)

ifeq ($(strip $(CONFIGURATION)),)
CONFIGURATION := null
endif

# не работает
#vpath %.h .
#vpath %.c .
#vpath %.o $(OBJDIR)
#vpath %.a $(LIBDIR)
#vpath %.exe $(BINDIR)

# работает
VPATH := $(LIBDIR) $(OBJDIR) $(BINDIR)

export

# https://superuser.com/a/593404
SUBDIRS := $(shell find . -mindepth 1 -maxdepth 1 -type d  \( ! -iname ".*" \) | sed 's|^\./||g')
.PHONY: $(SUBDIRS)

projects: subdirs $(SUBDIRS)
# это позволяет собрать только одну подпапку, например: make libncurses_util
# также здесь прописываем зависимости проектов
include Makefile.projdep.mak

$(SUBDIRS): subdirs
	$(MAKE) -$(MAKEFLAGS)C $@

last-config:
	echo [i] Compiler is $(CC), linker is $(LD), configuration is $(CONFIGURATION) | tee "$(ROOTDIR)/make.config.now"
	diff -N "$(ROOTDIR)/make.config.prev" "$(ROOTDIR)/make.config.now" > /dev/null 2> /dev/null || ( echo -e "\\x1b[0;33mConfiguration changed (code $$?), running clean\\x1b[0m" ; $(MAKE) -rR clean )
	cp -f "$(ROOTDIR)/make.config.now" "$(ROOTDIR)/make.config.prev"

subdirs: last-config
	-mkdir $(BINDIR) 2> /dev/null
	-mkdir $(LIBDIR) 2> /dev/null
	-mkdir $(OBJDIR) 2> /dev/null

clean:
	-rm -rf $(BINDIR)
	-rm -rf $(LIBDIR)
	-rm -rf $(OBJDIR)
	-rm -f "$(ROOTDIR)/make.config.prev"
	
distclean:
	@$(foreach dir,$(SUBDIRS),$(MAKE) -$(MAKEFLAGS)C $(dir) clean;)
