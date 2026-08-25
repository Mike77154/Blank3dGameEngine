CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -std=c89 -pedantic -Wall -Wextra -Werror
CPPFLAGS ?= -Iinclude -I../gcrosshair_base89/include -I../gcrosshair_params89/include -I../gcrosshair_core89/include -I../gcrosshair_provider89/include -I../gcrosshair_anim89/include

LIB = libgcrosshair_runtime89.a
OBJ = gcrosshair_runtime89.o
BASELIB = ../gcrosshair_base89/libgcrosshair_base89.a
PARAMSLIB = ../gcrosshair_params89/libgcrosshair_params89.a
CORELIB = ../gcrosshair_core89/libgcrosshair_core89.a
PROVIDERLIB = ../gcrosshair_provider89/libgcrosshair_provider89.a
ANIMLIB = ../gcrosshair_anim89/libgcrosshair_anim89.a
LIBS = $(LIB) $(ANIMLIB) $(PROVIDERLIB) $(CORELIB) $(PARAMSLIB) $(BASELIB)

all: $(LIB) test_runtime test_all_preset_animation example_plug_and_play

$(OBJ): src/gcrosshair_runtime89.c include/gcrosshair_runtime89.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/gcrosshair_runtime89.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) rcs $(LIB) $(OBJ)

test_runtime: tests/test_runtime.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_runtime.c $(LIBS) -o test_runtime

test_all_preset_animation: tests/test_all_preset_animation.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_all_preset_animation.c $(LIBS) -o test_all_preset_animation

example_plug_and_play: examples/example_plug_and_play.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) examples/example_plug_and_play.c $(LIBS) -o example_plug_and_play

test: test_runtime test_all_preset_animation
	./test_runtime
	./test_all_preset_animation

clean:
	rm -f $(OBJ) $(LIB) test_runtime test_runtime.exe test_all_preset_animation test_all_preset_animation.exe \
	      example_plug_and_play example_plug_and_play.exe

.PHONY: all test clean
