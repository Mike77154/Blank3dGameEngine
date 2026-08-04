CC ?= gcc
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude

CORE = src/wmagazine89.c
CORE_OBJ = wmagazine89.o

libwmagazine89.a: $(CORE_OBJ)
	ar rcs $@ $(CORE_OBJ)

$(CORE_OBJ): $(CORE) include/wmagazine89.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(CORE) -o $(CORE_OBJ)

all: libwmagazine89.a wmagazine89_demo smoke_test

wmagazine89_demo: $(CORE) demo/wmagazine89_demo.c include/wmagazine89.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CORE) demo/wmagazine89_demo.c -o $@

smoke_test: $(CORE) tests/smoke_test.c include/wmagazine89.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(CORE) tests/smoke_test.c -o $@

previews: wmagazine89_demo
	./wmagazine89_demo

test: smoke_test
	./smoke_test

clean:
	rm -f wmagazine89_demo smoke_test $(CORE_OBJ) libwmagazine89.a size_report
	rm -f previews/*.wav

.PHONY: all previews test clean
