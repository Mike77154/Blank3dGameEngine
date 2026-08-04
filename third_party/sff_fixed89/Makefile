CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -std=gnu89 -pedantic -Wall -Wextra -Werror -fno-strict-aliasing
CPPFLAGS ?= -Isff -Isublibs/pcx89 -Isublibs/png89 -Isublibs/zlib89
BUILD_DIR ?= build

SOURCES = \
	sff/sff_api.c \
	sff/sff_decomp.c \
	sff/sff_endian.c \
	sff/sff_map.c \
	sff/sff_pcx.c \
	sff/sff_png.c \
	sff/sff_png_builtin.c \
	sff/sff_v1.c \
	sff/sff_v2.c \
	sublibs/pcx89/pcx89.c \
	sublibs/png89/png89.c \
	sublibs/png89/png89_zlib89.c \
	sublibs/zlib89/zlib89.c

OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)
LIBRARY = $(BUILD_DIR)/libsff_fixed89.a
SMOKE = $(BUILD_DIR)/smoke

.PHONY: all test audit clean

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(SMOKE): tests/smoke.c $(LIBRARY)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/smoke.c $(LIBRARY) -o $@

test: $(SMOKE)
	./$(SMOKE)

audit:
	@! grep -RInE '(malloc|calloc|realloc|free)[[:space:]]*\(' sff sublibs extras examples
	@! grep -RInE '(^|[^A-Za-z0-9_])(float|double)([^A-Za-z0-9_]|$$)' sff sublibs extras examples
	@echo "constraint audit: ok (library/runtime paths use no heap calls, float, or double)"

clean:
	rm -rf $(BUILD_DIR)
