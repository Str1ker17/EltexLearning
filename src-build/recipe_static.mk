LOCAL_OBJECTS := $(LOCAL_SOURCES:.c=.o)

$(LIBDIR)$(LOCAL_MODULE): $(LOCAL_DEPENDENCIES)
$(LIBDIR)$(LOCAL_MODULE): $(LOCAL_OBJECTS)
	cd $(OBJDIR) && ar rcs $@ $^
	@echo -e \\x1b[32m AR \\x1b[0m $(shell realpath --relative-to $(ROOTDIR) $@)

$(LOCAL_OBJECTS): $(LOCAL_HEADERS)
%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_LOCAL) -c -o $(OBJDIR)$@ $<
	@echo -e \\x1b[33m CC \\x1b[0m $(shell realpath --relative-to $(ROOTDIR) $(OBJDIR)$@)
