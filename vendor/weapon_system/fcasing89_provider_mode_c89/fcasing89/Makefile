CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude
AR ?= ar

all: libfcasing89.a demo_casing_stdout demo_external_provider

libfcasing89.a: src/fcasing89.o
	$(AR) rcs $@ src/fcasing89.o

src/fcasing89.o: src/fcasing89.c include/fcasing89.h
	$(CC) $(CFLAGS) -c src/fcasing89.c -o src/fcasing89.o

demo_casing_stdout: examples/demo_casing_stdout.c libfcasing89.a
	$(CC) $(CFLAGS) examples/demo_casing_stdout.c libfcasing89.a -o demo_casing_stdout

demo_external_provider: examples/demo_external_provider.c libfcasing89.a
	$(CC) $(CFLAGS) examples/demo_external_provider.c libfcasing89.a -o demo_external_provider

clean:
	rm -f src/*.o libfcasing89.a demo_casing_stdout.exe demo_casing_stdout demo_external_provider.exe demo_external_provider

.PHONY: all clean
