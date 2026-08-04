# BigVaderHudder DSL v2

BVH is a C89 HUD DSL toolkit for games, dashboards, sims and overlays.
This revision is built around a caller-owned `BVH_Context` with bounded internal storage.

## Core promises

- C89 source.
- No dynamic allocator calls.
- No heap-owned AST or bytecode buffers.
- Fractional values use Q16.16 fixed-point.
- The runtime is callback based; BVH describes HUD data and behavior, it does not draw.

## Pipeline

```text
source .bhud
  -> token stream
  -> parser
  -> AST
  -> typed values
  -> IR
  -> bytecode
  -> VM/runtime callbacks
  -> JSON transpiler
```

## Build

```sh
make clean
make
```

The default build uses strict C89 flags:

```sh
-std=c89 -Wall -Wextra -Werror -pedantic
```

## CLI

```sh
./bvh tokens examples/doom_classic.bhud
./bvh ast examples/doom_classic.bhud
./bvh ir examples/doom_classic.bhud
./bvh run examples/doom_classic.bhud
./bvh compile examples/doom_classic.bhud -o out.bvbc
./bvh transpile examples/doom_classic.bhud -o out.json --pretty
```

## DSL sample

```bhud
hud doom_classic -=)
layer overlay
z 999
alpha 1.0

node health -=)
type text
bind player.health
color #FF0000
pos bottom_left + 120,18
(=-
```

## Typed values

BVH keeps the raw property string, then adds typed metadata when the syntax is recognized:

| Property | Example | Typed kind |
|---|---|---|
| `alpha` | `0.95` | `fixed` |
| `z` | `999` | `int` |
| `size` | `640,80` | `vec2i` |
| `frames` | `0..100` | `range` |
| `color` | `#FF0000` | `color` |
| `pos` | `bottom_left + 120,18` | `anchor_pos` |
| other single word | `overlay` | `symbol` |
| other complex text | `weapon.loaded "/" weapon.reserve` | `raw` |

## Embedding

```c
static BVH_Context ctx;
BVH_Error err;

bvh_context_init(&ctx);
if (!bvh_parse_file(&ctx, "examples/doom_classic.bhud", &err)) {
    /* handle err */
}
if (!bvh_compile_program(&ctx, &err)) {
    /* handle err */
}
if (!bvh_exec_bytecode(&ctx.bytecode, NULL, stdout, &err)) {
    /* handle err */
}
```
