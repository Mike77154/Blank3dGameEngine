# GFO — GameMaker-ish Object DSL (C89, no malloc, no floats)

This library compiles a lightweight symbol-driven DSL into compact bytecode and dispatches lifecycle
actions to engine-provided bindings (invokers `foo()` and handlers `@bar`). It is designed for low-end
targets (PS1-tier): fixed memory, no heap allocations, integer-only.

## DSL

```ini
[player]
constants = playerstats.cns
flags     = player.ini
life      = 100

*create
init_player()
@special_physics

*step
player_controller()

*render
draw_player()

*destroy
particle_emit()
```

### Script blocks (inline, no indentation required)

```ini
[player]
*step
script: python
print("tick", self)
:
```

## Build

A tiny example is in `examples/main.c`.

```bash
cc -Iinclude src/*.c examples/main.c -o gfo_demo
./gfo_demo
```

## Memory sizing

GFO never calls `malloc()`: you provide an arena block to `gfo_init()` / `gfo_init_ex()`.

- `gfo_init()` uses default internal caps derived from `gfo_limits`.
- `gfo_init_ex()` lets you provide explicit caps (`gfo_caps`) to shrink/expand internal buffers.
- `gfo_estimate_arena_bytes(lim, caps)` returns the exact number of arena bytes required for a configuration
  (including alignment), so you can size your block confidently.


## Notes

- All strings are slices into the original DSL buffer. Keep the buffer alive if you use script blocks.
- The symbol table interns identifiers and allows post-compilation rebinding via engine callbacks.
- `GFO_VAL_SYM` argument values are **interned symbol IDs** (`gfo_u16`). Use `gfo_sym_name_by_id()` if you need the spelling.
- Properties declared in a section header (`key = value`) become **type default props**.
  Property assignments inside lifecycle blocks are reserved and are **not treated as type defaults**.
- Fixed-width types: C89 has no `<stdint.h>`. GFO tries to select a true 32-bit type for `gfo_u32/gfo_i32`.
  You may override with `#define GFO_U32_TYPE ...` / `#define GFO_I32_TYPE ...` before including `gfo_common.h`.

