CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundimpact89.a smoke

libwsoundimpact89.a: build/wsoundimpact89.o
	$(AR) rcs $@ $^

build/wsoundimpact89.o: src/wsoundimpact89.c include/wsoundimpact89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundimpact89.c -o $@

smoke: tests/smoke.c libwsoundimpact89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundimpact89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundimpact89.a smoke
