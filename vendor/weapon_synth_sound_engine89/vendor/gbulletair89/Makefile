CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude

all: demo_wav test_c89

demo_wav: src/gbulletair89.c demo/demo_wav.c include/gbulletair89.h
	$(CC) $(CFLAGS) $(INCLUDES) src/gbulletair89.c demo/demo_wav.c -o demo_wav

test_c89: src/gbulletair89.c tests/test_c89.c include/gbulletair89.h
	$(CC) $(CFLAGS) $(INCLUDES) src/gbulletair89.c tests/test_c89.c -o test_c89

preview: demo_wav
	cd preview && ../demo_wav

clean:
	rm -f demo_wav test_c89 preview/*.wav
