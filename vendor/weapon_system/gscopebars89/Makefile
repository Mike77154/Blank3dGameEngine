CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude -Ideps

all: libgscopebars89.a

libgscopebars89.a: gscopebars89.o
	$(AR) rcs $@ $^

gscopebars89.o: src/gscopebars89.c include/gscopebars89.h deps/gscopepaint89.h
	$(CC) $(CFLAGS) -c src/gscopebars89.c -o $@

demo_bars: src/gscopebars89.c examples/demo_bars.c ../gscopepaint89/src/gscopepaint89.c
	$(CC) $(CFLAGS) -I../gscopepaint89/include ../gscopepaint89/src/gscopepaint89.c src/gscopebars89.c examples/demo_bars.c -o $@

demo: demo_bars

clean:
	rm -f *.o *.a demo_bars demo_bars.exe

.PHONY: all demo clean
