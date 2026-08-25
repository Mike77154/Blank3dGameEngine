CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude -Iexamples
AR ?= ar

all: libgiffyhb.a demo

libgiffyhb.a: src/giffy_hb.o
	$(AR) rcs libgiffyhb.a src/giffy_hb.o

src/giffy_hb.o: src/giffy_hb.c include/giffy_hb.h
	$(CC) $(CFLAGS) -c src/giffy_hb.c -o src/giffy_hb.o

demo: examples/main.o libgiffyhb.a
	$(CC) $(CFLAGS) examples/main.o libgiffyhb.a -o demo

examples/main.o: examples/main.c examples/demo_font.h include/giffy_hb.h
	$(CC) $(CFLAGS) -c examples/main.c -o examples/main.o

run: demo
	./demo

clean:
	rm -f src/*.o examples/*.o libgiffyhb.a demo demo.exe
