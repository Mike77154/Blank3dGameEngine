CC = gcc
AR = ar
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror -O2 -Iinclude

all: libshotpumpkin89.a shotpumpkin89_demo shotpumpkin89_test

shotpumpkin89.o: src/shotpumpkin89.c include/shotpumpkin89.h
	$(CC) $(CFLAGS) -c src/shotpumpkin89.c -o $@

libshotpumpkin89.a: shotpumpkin89.o
	$(AR) rcs $@ shotpumpkin89.o

shotpumpkin89_demo: libshotpumpkin89.a examples/shotpumpkin89_demo.c include/shotpumpkin89.h
	$(CC) $(CFLAGS) examples/shotpumpkin89_demo.c libshotpumpkin89.a -o $@

shotpumpkin89_test: libshotpumpkin89.a tests/test_shotpumpkin89.c include/shotpumpkin89.h
	$(CC) $(CFLAGS) tests/test_shotpumpkin89.c libshotpumpkin89.a -o $@

clean:
	rm -f shotpumpkin89.o libshotpumpkin89.a shotpumpkin89_demo shotpumpkin89_test shotpumpkin89_demo.wav
