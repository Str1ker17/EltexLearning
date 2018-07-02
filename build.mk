.PHONY: all clean

CC := gcc
LD := $(CC)
AR := ar

CFLAGS := -Wall

OBJDIR := obj
SRCDIR := src
LIBDIR := lib

SOURCES = $(wildcard $(SRCDIR)/$@/*.c)
HEADERS = $(wildcard $(SRCDIR)/$@/*.h)
OBJECTS = $(SOURCES:.c=.o)

all: subdirs libncurses_util

subdirs:
	mkdir bin 2> /dev/null || true
	mkdir lib 2> /dev/null || true
	mkdir obj 2> /dev/null || true

libncurses_util: $(OBJECTS)
	$(AR) -r $(LIBDIR)/$@.a $?

%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -rf ./bin
	rm -rf ./lib
	rm -rf ./obj
