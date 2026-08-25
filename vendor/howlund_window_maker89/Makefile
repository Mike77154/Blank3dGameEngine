CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude

all: libhowm89.a

howm89.o: src/howm89.c include/howm89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/howm89.c -o $@

libhowm89.a: howm89.o
	$(AR) rcs $@ $^

test: tests/test_howm89.c src/howm89.c include/howm89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) src/howm89.c tests/test_howm89.c -o test_howm89
	./test_howm89

x11-check:
	$(CC) $(CPPFLAGS) -Ibackends/x11 $(CFLAGS) -c backends/x11/howm89_x11_backend.c -o howm89_x11_backend.o

protocol-audit:
	@! grep -RInE '\\b(malloc|calloc|realloc|free|float|double|long[[:space:]]+long|int64_t|uint64_t)\\b|stdint\\.h' include src tests backends --include='*.c' --include='*.h' --include='*.m' || (echo "Protocol89 violation"; exit 1)
	@! grep -RInE '\\bHWND_(Window|Result|Bool|PlatformVTable)\\b|\\bhwnd_window_|\\bhwnd_library_|\\bhwnd_register_platform' include src tests backends --include='*.c' --include='*.h' --include='*.m' || (echo "Legacy public HWND API leak"; exit 1)
	@echo "HOWM89 Protocol89 audit: PASS"

clean:
	rm -f howm89.o howm89_x11_backend.o libhowm89.a test_howm89

.PHONY: all test x11-check protocol-audit clean
