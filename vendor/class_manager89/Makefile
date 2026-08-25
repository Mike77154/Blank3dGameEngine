CC ?= cc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude

all: test_cm89 test_cm89_bound demo

test_cm89: src/cm89.c tests/test_cm89.c
	$(CC) $(CFLAGS) src/cm89.c tests/test_cm89.c -o test_cm89

test_cm89_bound: src/cm89.c tests/test_cm89_bound.c
	$(CC) $(CFLAGS) -DCM89_ENABLE_INTERNAL_INSTANCES=0 \
		-DCM89_MAX_CLASSES=64 -DCM89_MAX_BASES=4 -DCM89_MAX_MRO=16 \
		-DCM89_MAX_CLASS_MEMBERS=32 -DCM89_NAME_MAX=32 \
		src/cm89.c tests/test_cm89_bound.c -o test_cm89_bound

demo: src/cm89.c examples/demo.c
	$(CC) $(CFLAGS) src/cm89.c examples/demo.c -o demo

test: test_cm89 test_cm89_bound
	./test_cm89
	./test_cm89_bound

clean:
	rm -f test_cm89 test_cm89_bound demo test_cm89.exe test_cm89_bound.exe demo.exe
