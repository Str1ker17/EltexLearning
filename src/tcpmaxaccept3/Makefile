.PHONY: clean

CFLAGS_LOCAL := -std=gnu99
LDFLAGS_LOCAL := -lpthread

LOCAL_HEADERS := $(wildcard *.h)
LOCAL_SOURCES := $(wildcard *.c)
LOCAL_STATIC_DEPENDENCIES := $(LIBDIR)liblinux_util.a $(LIBDIR)libvector.a
LOCAL_SHARED_DEPENDENCIES :=
LOCAL_MODULE := tcpmaxaccept3.exe

include $(BUILD_EXECUTABLE)

clean:
	rm -f $(BINDIR)/$(LOCAL_MODULE)
