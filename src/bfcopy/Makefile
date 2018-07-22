.PHONY: clean

CFLAGS_LOCAL := -std=c99
LDFLAGS_LOCAL :=

LOCAL_HEADERS := $(wildcard *.h)
LOCAL_SOURCES := $(wildcard *.c)
LOCAL_STATIC_DEPENDENCIES := $(LIBDIR)libcopyfile.a $(LIBDIR)liblinux_util.a
LOCAL_SHARED_DEPENDENCIES :=
LOCAL_MODULE := bfcopy.exe

include $(BUILD_EXECUTABLE)

clean:
	rm -f $(BINDIR)/$(LOCAL_MODULE)
