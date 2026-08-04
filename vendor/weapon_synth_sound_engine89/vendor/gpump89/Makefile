CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
CPPFLAGS ?= -Iinclude -Isrc

all: gpump89_demo test_gpump89

gpump89_demo: src/gpump89.c examples/gpump89_demo_wav.c include/gpump89.h src/gpump89_grains.inc
	$(CC) $(CFLAGS) $(CPPFLAGS) src/gpump89.c examples/gpump89_demo_wav.c -o $@

test_gpump89: src/gpump89.c tests/test_gpump89.c include/gpump89.h src/gpump89_grains.inc
	$(CC) $(CFLAGS) $(CPPFLAGS) src/gpump89.c tests/test_gpump89.c -o $@

check: test_gpump89
	./test_gpump89

clean:
	rm -f gpump89_demo test_gpump89 *.o

.PHONY: all check clean
