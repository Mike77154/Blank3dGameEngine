CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude

LIB_OBJS = src/gattach89.o src/gattach89_profiles.o src/gattach89_render_bridge.o

all: libgattach89.a demo_gattach89 test_gattach89

libgattach89.a: $(LIB_OBJS)
	$(AR) rcs $@ $(LIB_OBJS)

demo_gattach89: demo/demo_gattach89.o libgattach89.a
	$(CC) $(CFLAGS) -o $@ demo/demo_gattach89.o libgattach89.a

test_gattach89: tests/test_gattach89.o libgattach89.a
	$(CC) $(CFLAGS) -o $@ tests/test_gattach89.o libgattach89.a

check: test_gattach89
	./test_gattach89

clean:
	rm -f src/*.o demo/*.o tests/*.o libgattach89.a demo_gattach89 test_gattach89
