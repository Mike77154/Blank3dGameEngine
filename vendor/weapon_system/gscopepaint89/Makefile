CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude

all: libgscopepaint89.a demo_paint

libgscopepaint89.a: gscopepaint89.o
	$(AR) rcs $@ $^

gscopepaint89.o: src/gscopepaint89.c include/gscopepaint89.h
	$(CC) $(CFLAGS) -c src/gscopepaint89.c -o $@

demo_paint: src/gscopepaint89.c examples/demo_paint.c include/gscopepaint89.h
	$(CC) $(CFLAGS) src/gscopepaint89.c examples/demo_paint.c -o $@

clean:
	rm -f *.o *.a demo_paint demo_paint.exe
