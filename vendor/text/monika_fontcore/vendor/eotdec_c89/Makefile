CC ?= cc
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -O2
INCLUDES = -Iinclude

all: eottool

eottool: src/eotdec.c src/mtxdec.c tools/eottool.c include/eotdec.h include/mtxdec.h
	$(CC) $(CFLAGS) $(INCLUDES) -o eottool src/eotdec.c src/mtxdec.c tools/eottool.c

clean:
	rm -f eottool eottool.exe tests/*.out.ttf tests/*.out.otf tests/*.ctf

.PHONY: all clean
