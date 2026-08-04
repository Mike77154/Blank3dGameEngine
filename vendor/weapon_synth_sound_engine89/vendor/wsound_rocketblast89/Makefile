CC ?= gcc
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
INCLUDES = -Iinclude -Isrc
CORE = src/wsrb89_dsp.c src/wsrb89_presets.c src/wsound_rocketblast89.c

all: wsrb89_demo

wsrb89_demo: $(CORE) demo/wsrb89_demo.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) demo/wsrb89_demo.c -o wsrb89_demo

preview: wsrb89_demo
	./wsrb89_demo

clean:
	rm -f wsrb89_demo *.o src/*.o demo/*.o

.PHONY: all preview clean

smoke: $(CORE) tests/wsrb89_smoke.c
	$(CC) $(CFLAGS) $(INCLUDES) $(CORE) tests/wsrb89_smoke.c -o wsrb89_smoke
	./wsrb89_smoke
