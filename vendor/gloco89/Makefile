CC ?= gcc
CFLAGS ?= -ansi -pedantic -Wall -Wextra -Iinclude
AR ?= ar
ARFLAGS ?= rcs

OBJS = src/gloco89.o src/gloco89_profiles.o src/gloco89_bridge.o

all: libgloco89.a demo_gloco89 demo_providers

libgloco89.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

src/gloco89.o: src/gloco89.c include/gloco89.h
	$(CC) $(CFLAGS) -c src/gloco89.c -o $@

src/gloco89_profiles.o: src/gloco89_profiles.c include/gloco89.h include/gloco89_profiles.h
	$(CC) $(CFLAGS) -c src/gloco89_profiles.c -o $@

src/gloco89_bridge.o: src/gloco89_bridge.c include/gloco89.h include/gloco89_bridge.h
	$(CC) $(CFLAGS) -c src/gloco89_bridge.c -o $@

demo_gloco89: demo/demo_gloco89.c libgloco89.a
	$(CC) $(CFLAGS) demo/demo_gloco89.c libgloco89.a -o demo_gloco89

demo_providers: demo/demo_providers.c libgloco89.a
	$(CC) $(CFLAGS) demo/demo_providers.c libgloco89.a -o demo_providers

clean:
	rm -f src/*.o libgloco89.a demo_gloco89 demo_providers demo/test_engine_ready

test-engine-ready:
	$(CC) -std=c89 -pedantic-errors -Wall -Wextra -Werror -Iinclude \
	 src/gloco89.c src/gloco89_bridge.c src/gloco89_profiles.c demo/test_engine_ready.c \
	 -o demo/test_engine_ready
	./demo/test_engine_ready
