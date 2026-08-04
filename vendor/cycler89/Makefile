CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -Wall -Wextra -pedantic -O2
INCLUDES = -Iinclude
OBJECT = build/cycler89.o
LIBRARY = build/libcycler89.a

.PHONY: all clean

all: $(LIBRARY)

build:
	mkdir -p build

$(OBJECT): src/cycler89.c include/cycler89.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c src/cycler89.c -o $(OBJECT)

$(LIBRARY): $(OBJECT)
	$(AR) rcs $(LIBRARY) $(OBJECT)

clean:
	rm -rf build
