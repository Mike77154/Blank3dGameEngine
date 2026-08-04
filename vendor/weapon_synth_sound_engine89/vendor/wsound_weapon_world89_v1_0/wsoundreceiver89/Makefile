CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundreceiver89.a smoke

libwsoundreceiver89.a: build/wsoundreceiver89.o
	$(AR) rcs $@ $^

build/wsoundreceiver89.o: src/wsoundreceiver89.c include/wsoundreceiver89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundreceiver89.c -o $@

smoke: tests/smoke.c libwsoundreceiver89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundreceiver89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundreceiver89.a smoke
