CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Iinclude

all: npc_perception_demo incremental_demo test_geder_truth_gate

npc_perception_demo: src/geder_truth_gate.c examples/npc_perception_demo.c include/geder_truth_gate.h
	$(CC) $(CFLAGS) src/geder_truth_gate.c examples/npc_perception_demo.c -o $@

incremental_demo: src/geder_truth_gate.c examples/incremental_demo.c include/geder_truth_gate.h
	$(CC) $(CFLAGS) src/geder_truth_gate.c examples/incremental_demo.c -o $@

test_geder_truth_gate: src/geder_truth_gate.c tests/test_geder_truth_gate.c include/geder_truth_gate.h
	$(CC) $(CFLAGS) src/geder_truth_gate.c tests/test_geder_truth_gate.c -o $@

test: test_geder_truth_gate
	./test_geder_truth_gate

clean:
	rm -f npc_perception_demo incremental_demo test_geder_truth_gate

.PHONY: all test clean
