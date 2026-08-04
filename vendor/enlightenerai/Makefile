CC ?= gcc
CFLAGS ?= -std=c89 -Wall -Wextra -Werror -pedantic -ffreestanding -fno-builtin -Iinclude -O2
AR ?= ar
ARFLAGS ?= rcs

SRC = \
	src/eai_math.c \
	src/eai_core.c \
	src/eai_events.c \
	src/eai_memory.c \
	src/eai_nav_graph.c \
	src/eai_nav_astar.c \
	src/eai_nav_path.c \
	src/eai_nav_zones.c \
	src/eai_perception_vision.c \
	src/eai_perception_hearing.c \
	src/eai_targeting.c \
	src/eai_tactical_cover.c \
	src/eai_tactical_flank.c \
	src/eai_actions.c \
	src/eai_debug.c \
	src/eai_fsm_adapter.c

OBJ = $(SRC:.c=.o)
LIB = libenlightenerai.a

TESTS = \
	tests/test_astar \
	tests/test_perception \
	tests/test_targeting \
	tests/test_tactical \
	tests/test_hardening

.PHONY: all clean test

all: $(LIB) examples/minimal_example

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $(OBJ)

examples/minimal_example: examples/minimal_example.c $(LIB)
	$(CC) $(CFLAGS) -o $@ examples/minimal_example.c $(LIB)

tests/test_astar: tests/test_astar.c $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_astar.c $(LIB)

tests/test_perception: tests/test_perception.c $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_perception.c $(LIB)

tests/test_targeting: tests/test_targeting.c $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_targeting.c $(LIB)

tests/test_tactical: tests/test_tactical.c $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_tactical.c $(LIB)

tests/test_hardening: tests/test_hardening.c $(LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_hardening.c $(LIB)

test: $(TESTS)
	./tests/test_astar
	./tests/test_perception
	./tests/test_targeting
	./tests/test_tactical
	./tests/test_hardening

clean:
	rm -f $(OBJ) $(LIB) $(TESTS) examples/minimal_example
