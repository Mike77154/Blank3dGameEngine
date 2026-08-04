CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libwsoundcombatbus89.a smoke

libwsoundcombatbus89.a: build/wsoundcombatbus89.o
	$(AR) rcs $@ $^

build/wsoundcombatbus89.o: src/wsoundcombatbus89.c include/wsoundcombatbus89.h include/wsound89_common.h
	mkdir -p build
	$(CC) $(CFLAGS) -c src/wsoundcombatbus89.c -o $@

smoke: tests/smoke.c libwsoundcombatbus89.a
	$(CC) $(CFLAGS) tests/smoke.c libwsoundcombatbus89.a -o smoke
	./smoke

clean:
	rm -rf build libwsoundcombatbus89.a smoke
