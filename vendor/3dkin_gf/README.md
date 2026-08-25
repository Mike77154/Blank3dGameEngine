# 3DKin-GF v0.1.0

A tiny 3D condition checker and conservative movement solver for C89 engines.

It provides the 3D equivalent of the useful GameMaker/Construct collision questions:

- `place_meeting(x, y, z, target)`
- `place_free(x, y, z)`
- `instance_place(x, y, z, target)`
- `is_on_floor()` (solid or jump-thru floor)
- `is_on_wall()` / Construct-style `is_by_wall()`
- `is_under_ceiling()`
- `is_overlapping_at_offset(dx, dy, dz, target)`
- string-based verb checking for DSL/VM integrations
- a small stepped, axis-separated motion solver

## Hard constraints

- ISO C89
- no `malloc`, `calloc`, `realloc`, `free`, or heap ownership
- no `float` or `double`
- no `long long`
- fixed-point coordinates
- fixed `gk3d_arena` object pool embedded in `gk3d_world`
- no renderer or physics dependency

## Core idea

A query never changes the object's real position. It creates a candidate AABB at the requested XYZ position, checks candidate overlaps, and returns either a Boolean or the first matching object ID.

The native broadphase is AABB-vs-AABB. `is_on_floor()` accepts solids and jump-thrus, while `is_on_wall()` accepts solids only. Objects marked `GK3D_FLAG_PRECISE` can invoke a user-supplied narrowphase callback for a real mesh, capsule, voxel, BSP, or engine-specific collision test. If no callback handles the pair, the AABB result is the deterministic fallback.

## Minimal use

```c
#include "gk3d.h"

gk3d_world world;
int player;

gk3d_world_init(&world);

player = gk3d_world_add_box(&world, 1,
    GK3D_FROM_INT(0), GK3D_FROM_INT(1), GK3D_FROM_INT(0),
    GK3D_FROM_INT(1), GK3D_FROM_INT(2), GK3D_FROM_INT(1),
    0u);

if (gk3d_is_on_floor(&world, player)) {
    /* The verb/condition is true. */
}
```

## Build

MSYS2/MinGW or Unix-like shell:

```sh
make verify
```

Windows MinGW32 command prompt:

```bat
build_mingw32.bat
```

A freestanding i686 Windows/MinGW-compatible static archive is also included at:

```text
prebuilt/mingw32/libgk3d.a
```

## Coordinate convention

A box uses minimum-corner coordinates plus positive extents:

```text
(x, y, z, width, height, depth)
```

The default is Y-up, positive Y upward. Change it at runtime:

```c
gk3d_world_set_up(&world, GK3D_AXIS_Z, 1);
```

## Solver scope

`gk3d_solve_move()` is intentionally small. It steps along X, then Y, then Z, clamps each blocked axis with fixed-point binary refinement, and returns allowed deltas and hit normals. It is a condition-oriented fallback, not a rigid-body simulator.

## License

CC0-1.0. See `LICENSE.txt`.
