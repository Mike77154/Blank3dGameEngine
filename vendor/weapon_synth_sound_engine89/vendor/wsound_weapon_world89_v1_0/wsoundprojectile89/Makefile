CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundprojectile89.a smoke

libwsoundprojectile89.a: build/wsoundprojectile89.o
	$(AR) rcs $@ $^

build/wsoundprojectile89.o: src/wsoundprojectile89.c include/wsoundprojectile89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundprojectile89.c -o $@

smoke: tests/smoke.c libwsoundprojectile89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundprojectile89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundprojectile89.a smoke
