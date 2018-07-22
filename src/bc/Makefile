.PHONY: clean

CFLAGS_LOCAL := -std=gnu99
LDFLAGS_LOCAL := -lncursesw

LOCAL_HEADERS := $(wildcard *.h)
LOCAL_SOURCES := $(wildcard *.c)
LOCAL_STATIC_DEPENDENCIES := $(LIBDIR)libeditor.a $(LIBDIR)libcopyfile.a $(LIBDIR)liblinux_util.a $(LIBDIR)libncurses_util.a
LOCAL_SHARED_DEPENDENCIES :=
LOCAL_MODULE := bc.exe

include $(BUILD_EXECUTABLE)

clean:
	rm -f $(BINDIR)/$(LOCAL_MODULE)
