#.PHONY: libncurses_util

export CC := gcc
export CFLAGS := -std=c99
export GPATH := libncurses_util

STATIC_LIB := libncurses_util

$(STATIC_LIB): $(STATIC_LIB).a

$(STATIC_LIB).a:
	$(MAKE) -C $(STATIC_LIB)

clean:
	$(MAKE) -C $(STATIC_LIB) clean
