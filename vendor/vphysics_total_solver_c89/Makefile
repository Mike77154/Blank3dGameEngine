CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c89 -O2 -Wall -Wextra
STRICT_CFLAGS = -std=c89 -O2 -Wall -Wextra -Werror -pedantic-errors

SOURCES = \
	src/vp_world.c \
	src/vp_body.c \
	src/vp_contact.c \
	src/vp_joint.c \
	src/vp_solver.c \
	src/vp_ragdoll.c \
	src/vp_debugdraw.c \
	src/vp_state.c \
	src/vp_total_solver.c

OBJECTS = $(SOURCES:.c=.o)

.PHONY: all strict examples test clean

all: $(OBJECTS)

strict:
	$(CC) $(CPPFLAGS) $(STRICT_CFLAGS) -c $(SOURCES)

examples: plane_drop total_solver_demo

plane_drop: examples/plane_drop.c $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

total_solver_demo: examples/total_solver_demo.c $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

total_solver_selftest: tests/total_solver_selftest.c $(SOURCES)
	$(CC) $(CPPFLAGS) $(STRICT_CFLAGS) $^ -o $@

pair_cache_churn: tests/pair_cache_churn.c $(SOURCES)
	$(CC) $(CPPFLAGS) $(STRICT_CFLAGS) $^ -o $@

test: total_solver_selftest pair_cache_churn
	./total_solver_selftest
	./pair_cache_churn

clean:
	rm -f $(OBJECTS) *.o plane_drop total_solver_demo total_solver_selftest pair_cache_churn
