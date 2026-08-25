CC ?= cc
AR ?= ar
CFLAGS ?= -std=c89 -pedantic-errors -Wall -Wextra -Werror -O2
INCLUDES = -Iinclude
BUILD = build
LIB = $(BUILD)/libpdc3d.a

VENDOR_SRC = \
    src/vendor/hurtbox3d.c \
    src/vendor/hitbox3d.c \
    src/vendor/melee3d.c \
    src/vendor/grab3d.c \
    src/vendor/throws3d.c \
    src/vendor/weakspots3d.c \
    src/vendor/framedata/fd3d89.c \
    src/vendor/defense/gblock3d89.c \
    src/vendor/defense/gguard3d89.c \
    src/vendor/defense/gparry3d89.c \
    src/vendor/defense/gshell3d89.c \
    src/vendor/defense/gcounter3d89.c

PDC_SRC = \
    src/pdc3d_arena.c \
    src/pdc3d_bridge.c \
    src/pdc3d_damage.c \
    src/pdc3d_framedata.c \
    src/pdc3d_defense.c \
    src/pdc3d.c

OBJS = $(VENDOR_SRC:%.c=$(BUILD)/%.o) $(PDC_SRC:%.c=$(BUILD)/%.o)

DEMOS = \
    $(BUILD)/demo_pdc3d_zombie_bite \
    $(BUILD)/demo_pdc3d_swept_blade \
    $(BUILD)/demo_pdc3d_arena \
    $(BUILD)/demo_pdc3d_defense_pipeline \
    $(BUILD)/demo_pdc3d_guard_shell_counter \
    $(BUILD)/demo_pdc3d_framedata_layer \
    $(BUILD)/demo_pdc3d_fd3d_protocol_patch

all: $(LIB) demos

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)

$(BUILD)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

demos: $(DEMOS)

$(BUILD)/demo_pdc3d_zombie_bite: examples/demo_pdc3d_zombie_bite.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

$(BUILD)/demo_pdc3d_swept_blade: examples/demo_pdc3d_swept_blade.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

$(BUILD)/demo_pdc3d_arena: examples/demo_pdc3d_arena.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

$(BUILD)/demo_pdc3d_defense_pipeline: examples/demo_pdc3d_defense_pipeline.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

$(BUILD)/demo_pdc3d_guard_shell_counter: examples/demo_pdc3d_guard_shell_counter.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

$(BUILD)/demo_pdc3d_framedata_layer: examples/demo_pdc3d_framedata_layer.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

$(BUILD)/demo_pdc3d_fd3d_protocol_patch: examples/demo_pdc3d_fd3d_protocol_patch.c $(LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIB) -o $@

run: all
	$(BUILD)/demo_pdc3d_zombie_bite
	$(BUILD)/demo_pdc3d_swept_blade
	$(BUILD)/demo_pdc3d_arena
	$(BUILD)/demo_pdc3d_defense_pipeline
	$(BUILD)/demo_pdc3d_guard_shell_counter
	$(BUILD)/demo_pdc3d_framedata_layer
	$(BUILD)/demo_pdc3d_fd3d_protocol_patch

clean:
	rm -rf $(BUILD)

.PHONY: all demos run clean
