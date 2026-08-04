CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude
AR ?= ar

all: build/libreactiveloader.a build/demo_reactiveloader

build:
	mkdir -p build

build/reactiveloader.o: src/reactiveloader.c include/reactiveloader.h | build
	$(CC) $(CFLAGS) -c src/reactiveloader.c -o build/reactiveloader.o

build/libreactiveloader.a: build/reactiveloader.o
	$(AR) rcs build/libreactiveloader.a build/reactiveloader.o

build/demo_reactiveloader: demo/demo_reactiveloader.c build/libreactiveloader.a include/reactiveloader.h | build
	$(CC) $(CFLAGS) demo/demo_reactiveloader.c build/libreactiveloader.a -o build/demo_reactiveloader

run: build/demo_reactiveloader
	./build/demo_reactiveloader

clean:
	rm -rf build
