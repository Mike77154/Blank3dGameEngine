CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude

all: libgtelescopiczoom89.a

gtelescopiczoom89.o: gtelescopiczoom89/src/gtelescopiczoom89.c gtelescopiczoom89/include/gtelescopiczoom89.h include/gscope89_common.h
	$(CC) $(CFLAGS) -c gtelescopiczoom89/src/gtelescopiczoom89.c -o $@

libgtelescopiczoom89.a: gtelescopiczoom89.o
	$(AR) rcs $@ $<

provider_example: examples/provider_example.c gtelescopiczoom89/src/gtelescopiczoom89.c gtelescopiczoom89/include/gtelescopiczoom89.h include/gscope89_common.h
	$(CC) $(CFLAGS) -Igtelescopiczoom89/include examples/provider_example.c gtelescopiczoom89/src/gtelescopiczoom89.c -o $@

provider_smoke: tests/provider_smoke.c gtelescopiczoom89/src/gtelescopiczoom89.c gtelescopiczoom89/include/gtelescopiczoom89.h include/gscope89_common.h
	$(CC) $(CFLAGS) -Igtelescopiczoom89/include tests/provider_smoke.c gtelescopiczoom89/src/gtelescopiczoom89.c -o $@

test: provider_example provider_smoke
	./provider_example
	./provider_smoke

clean:
	rm -f gtelescopiczoom89.o libgtelescopiczoom89.a provider_example provider_smoke
