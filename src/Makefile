# твою ж дивизию! определение переменных не требует таба!

# на всякий случай
ifeq ($(MAKELEVEL),0)
ROOTDIR := $(realpath $(CURDIR)/../)
BINDIR := $(ROOTDIR)/bin/
LIBDIR := $(ROOTDIR)/lib/
OBJDIR := $(ROOTDIR)/obj/
SRCDIR := $(ROOTDIR)/src/

ifeq ($(strip $(CC)),)
#CC := gcc
endif

LD := $(CC)
endif

# не работает
#vpath %.h .
#vpath %.c .
vpath %.o $(OBJDIR)
vpath %.a $(LIBDIR)

# работает
VPATH := $(LIBDIR) $(OBJDIR) $(BINDIR)

export

.PHONY: subdirs projects clean distclean
# https://superuser.com/a/593404
SUBDIRS := $(shell find . -mindepth 1 -maxdepth 1 -type d  \( ! -iname ".*" \) | sed 's|^\./||g')
.PHONY: $(SUBDIRS)

projects: subdirs $(SUBDIRS)
# это позволяет собрать только одну подпапку, например: make libncurses_util
# но чёт работает не очень :(
# fixed
bc: libncurses_util libeditor libcopyfile
be: libncurses_util libeditor
bterm: libncurses_util
bfcopy: libncurses_util libcopyfile
libeditor: libncurses_util
libncurses_util:

$(SUBDIRS): subdirs
	$(MAKE) -$(MAKEFLAGS) -C $@

subdirs:
	echo Compiler is $(CC)
	@mkdir $(BINDIR) 2> /dev/null || true
	@mkdir $(LIBDIR) 2> /dev/null || true
	@mkdir $(OBJDIR) 2> /dev/null || true

clean:
	rm -rf $(BINDIR)
	rm -rf $(LIBDIR)
	rm -rf $(OBJDIR)
	
distclean:
	@$(foreach dir,$(SUBDIRS),$(MAKE) -$(MAKEFLAGS) -C $(dir) clean;)
