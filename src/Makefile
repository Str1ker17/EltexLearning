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
# DEPENDENCIES
bc: libncurses_util libeditor libcopyfile
be: libncurses_util libeditor
bterm: libncurses_util
bfcopy: libncurses_util libcopyfile
libeditor: libncurses_util
libncurses_util:

$(SUBDIRS): subdirs
	$(MAKE) -$(MAKEFLAGS)C $@

last-config-gen:
	@echo [i] Compiler is $(CC), configuration is $(CONFIGURATION) | tee $(ROOTDIR)/last-config.now

last-config: last-config-gen
#last-config: DIFFCODE:=$(shell diff -N last-config last-config.now > /dev/null ; echo \$\? >&2 | tee last-config.code); echo $(DIFFCODE)
#	echo $(DIFFCODE)
#ifneq ($(DIFFCODE),0)
#	@echo -e "\033[0;31mConfiguration changed, running clean\033[0m"
#	$(MAKE) -sr clean
#	cp -f last-config.now last-config
#endif

subdirs: last-config
	@mkdir $(BINDIR) 2> /dev/null || true
	@mkdir $(LIBDIR) 2> /dev/null || true
	@mkdir $(OBJDIR) 2> /dev/null || true

clean:
	rm -rf $(BINDIR)
	rm -rf $(LIBDIR)
	rm -rf $(OBJDIR)
	
distclean:
	@$(foreach dir,$(SUBDIRS),$(MAKE) -$(MAKEFLAGS) -C $(dir) clean;)
