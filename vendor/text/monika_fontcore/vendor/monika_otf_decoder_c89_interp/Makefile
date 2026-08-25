CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -O2

all: otf_dump

otf_dump: src/otf_c89.c src/otf_c89.h examples/otf_dump.c
	$(CC) $(CFLAGS) -I./src src/otf_c89.c examples/otf_dump.c -o otf_dump

clean:
	rm -f otf_dump otf_dump.exe *.o src/*.o examples/*.o
