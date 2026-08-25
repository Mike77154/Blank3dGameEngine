# world3d89 v2 protocol

`world3d89` is a tiny 3D world manager for C89 engines. It does not render, simulate physics, parse meshes, or own assets. Its job is to decide what world cells should exist, what entities live in them, what bridges should be asked to load/activate/deactivate/unload, and what spatial queries can be answered cheaply.

## Hard rules

- C89 / gnu89 compatible.
- No `malloc`, `free`, `realloc`.
- No heap ownership.
- No `float` or `double` math.
- All positions use 16.16 fixed point: `w3d_fp`.
- The engine passes one external memory block into `w3d_world_init()`.

## Main model

```text
world
├─ cells / chunks
│  ├─ state: unloaded/loading/resident/active/unloading
│  ├─ masks: layer/phase/difficulty/service
│  ├─ asset refs: visual/collision/nav/audio/script/proxy
│  └─ entity intrusive list
├─ static cell hash
├─ streaming sources
├─ entity registry
├─ portal graph
├─ event queue
└─ bridge callbacks
```

## Cell states

```text
UNLOADED   = no cell runtime data requested
LOADING    = bridge has been asked to load it; engine must later commit
RESIDENT   = loaded but not fully active
ACTIVE     = loaded and active around a streaming source
UNLOADING  = bridge has been asked to unload it; engine must later commit
```

If `auto_commit_streaming` is nonzero, the library commits state changes immediately. If it is zero, the engine must call `w3d_world_commit_cell_state()` when the asset/collision/nav/etc. bridge has actually completed.

## Streaming source hysteresis

Use `w3d_world_set_stream_source_hysteresis()` when you want anti-flicker radii:

```c
w3d_world_set_stream_source_hysteresis(
    &world,
    0,
    player_pos,
    w3d_fp_from_int(64),   /* active enter */
    w3d_fp_from_int(80),   /* active exit  */
    w3d_fp_from_int(128),  /* resident enter */
    w3d_fp_from_int(160),  /* resident exit  */
    W3D_LAYER_ALL,
    W3D_PHASE_ALL,
    W3D_DIFF_ALL,
    1);
```

This prevents border cells from rapidly loading/unloading as the source moves around an edge.

## Streaming budgets

Set in `w3d_world_config.budget`:

```c
cfg.budget.max_load_requests_per_tick = 2;
cfg.budget.max_unload_requests_per_tick = 2;
cfg.budget.max_activate_requests_per_tick = 4;
cfg.budget.max_deactivate_requests_per_tick = 4;
```

The world still evaluates all cells, but only emits up to the budgeted number of bridge requests per tick. Deferred requests are visible through `w3d_world_status.deferred_requests_last_tick`.

## Data layers and service masks

A cell can exist for different systems without every bridge caring about it:

```c
d.layer_mask = W3D_LAYER_GAMEPLAY | W3D_LAYER_VISUAL | W3D_LAYER_COLLISION;
d.phase_mask = W3D_PHASE_DEFAULT | W3D_PHASE_NIGHT;
d.difficulty_mask = W3D_DIFF_NORMAL | W3D_DIFF_HARD;
d.service_mask = W3D_SERVICE_ASSET | W3D_SERVICE_RENDER | W3D_SERVICE_COLLISION;
```

`w3d_world_set_runtime_masks()` lets the engine hide/reveal groups of cells for puzzles, day/night variants, cutscenes, debug layers, or difficulty-specific layouts.

## Cell hash

`w3d_world_find_cell()` is O(1) average using static open addressing. Pick `max_cell_hash_entries` larger than `max_cells`, ideally around 2x.

```c
cfg.max_cells = 512;
cfg.max_cell_hash_entries = 1031;
```

## Queries

`w3d_query_aabb()` computes the cell coordinate range touched by the query AABB, finds matching cells through the hash, then walks only the intrusive entity lists inside those cells. It does not sweep every entity in the world.

```c
w3d_handle hits[32];
int n = w3d_query_aabb(&world, trigger_box, W3D_LAYER_GAMEPLAY, hits, 32);
```

The query expands by one cell in each direction to avoid missing small entities straddling cell boundaries.

## Portals / interiors

Use portals for rooms, doors, windows, corridors, RE-style interiors, camera room activation, and occlusion-ish visibility groups.

```c
w3d_world_define_portal(&world, room_a, room_b, door_bounds,
    W3D_PORTAL_OPEN, W3D_LAYER_ALL, W3D_PHASE_ALL, 0);

w3d_world_query_visible_cells_from(&world, room_a, 4, W3D_LAYER_VISUAL, out_cells, max_out);
```

Closed portals block traversal. One-way portals are supported with `W3D_PORTAL_ONE_WAY`.

## HLOD / proxy refs

Cells can point to a cheap proxy asset for far representation:

```c
w3d_world_set_cell_proxy(&world, cell_index, proxy_asset_ref, w3d_fp_from_int(96));
```

The library only emits proxy enable/disable requests; your renderer/asset bridge decides what to do with the ref.

## Declarative descriptor mini-format

This parser is intentionally tiny, line-based, and integer-only. It is meant as a protocol example, not as the final DSL.

```text
# cell cx cy cz asset flags layer phase difficulty service
cell 0 0 0 100 32 4294967295 1 4294967295 127
cell 1 0 0 101 32 4294967295 1 4294967295 127

# proxy cx cy cz proxy_asset distance_units
proxy 1 0 0 900 20

# portal from_cell to_cell flags user_index
portal 0 1 1 77

# source x y z active_radius resident_radius layer
source 0 0 0 64 160 4294967295
```

## Build

```sh
mingw32-make -f Makefile.mingw
./test_world3d89.exe
```

The package was checked with:

```sh
gcc -std=gnu89 -Wall -Wextra -pedantic
```

## Integration idea for Blank3D

```text
Blank3D player/camera pose
↓
world3d89 streaming source
↓
world3d89 decides cell desired states
↓
asset bridge loads cell data
render bridge activates visual cells
collision bridge activates collision chunks
physics bridge wakes physical islands
nav bridge loads nav chunks
audio bridge starts/stops zones
script bridge spawns/despawns script scopes
```
