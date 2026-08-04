CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude
NAME = wsoundportal89
all: lib$(NAME).a smoke
build:
	mkdir -p build
build/$(NAME).o: src/$(NAME).c include/$(NAME).h include/wsound89_common.h | build
	$(CC) $(CFLAGS) $(INCLUDES) -c src/$(NAME).c -o $@
lib$(NAME).a: build/$(NAME).o
	$(AR) rcs $@ $<
smoke: tests/smoke.c lib$(NAME).a
	$(CC) $(CFLAGS) $(INCLUDES) tests/smoke.c lib$(NAME).a -o smoke
clean:
	rm -rf build lib$(NAME).a smoke
