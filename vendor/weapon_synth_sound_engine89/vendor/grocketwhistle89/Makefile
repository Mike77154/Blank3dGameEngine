CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2

CORE_OBJ = grocketwhistle89.o

all: libgrocketwhistle89.a demo_wav demo_ab

grocketwhistle89.o: grocketwhistle89.c grocketwhistle89.h
	$(CC) $(CFLAGS) -c grocketwhistle89.c -o grocketwhistle89.o

libgrocketwhistle89.a: $(CORE_OBJ)
	$(AR) rcs libgrocketwhistle89.a $(CORE_OBJ)

demo_wav: grocketwhistle89.c grocketwhistle89.h demo_wav.c
	$(CC) $(CFLAGS) grocketwhistle89.c demo_wav.c -o demo_wav

demo_ab: grocketwhistle89.c grocketwhistle89.h demo_ab.c
	$(CC) $(CFLAGS) grocketwhistle89.c demo_ab.c -o demo_ab

test_api: grocketwhistle89.c grocketwhistle89.h test_api.c
	$(CC) $(CFLAGS) grocketwhistle89.c test_api.c -o test_api

test: test_api
	./test_api

previews: demo_wav demo_ab
	./demo_wav
	./demo_ab

clean:
	rm -f *.o *.a demo_wav demo_wav.exe demo_ab demo_ab.exe test_api test_api.exe
