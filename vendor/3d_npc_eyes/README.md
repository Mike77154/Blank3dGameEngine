# 3D_NPC_Eyes

**3D_NPC_Eyes** is a small, engine-agnostic 3D perception library for NPCs and AI agents.

It gives an AI a simple way to ask:

```c
Can this sensor perceive this target?
```

The library does **not** know about players, monsters, BSPs, grids, physics engines, renderers, FPIL, DDSL, or any game-specific entity system. You feed it positions, masks, target samples, and an optional raycast callback.

## Design rules

- C89-compatible source style.
- No `malloc`, `calloc`, `realloc`, or `free` in the library code.
- No heap ownership inside the library.
- No `float` or `double` types in public/source/example/test C files.
- Integer/fixed-scale directional math.
- User-owned arrays for targets and results.
- Optional line-of-sight through a user-supplied callback.
- No FPIL or DDSL integration yet.

## Included sensors

| Sensor | Use |
|---|---|
| Cone | Guard/NPC forward vision |
| Sphere | Omnidirectional perception/proximity |
| Box | Area trigger/zone watcher |
| Frustum | Camera-like pyramid view |

## Core flow

```txt
target sample point
   -> sensor shape test
   -> optional raycast callback
   -> visible / partial / occluded / rejected reason
```

Multiple samples per target allow partial visibility:

```txt
center ray clear
head ray blocked
feet ray blocked
=> partial visibility
```

## Build

```sh
make
make test
make examples
make check
```

The default build uses:

```sh
-std=c89 -pedantic -Wall -Wextra
```

## Minimal example

```c
#include "3d_npc_eyes.h"

static int my_raycast(
    void *world_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 block_mask,
    tdne_ray_hit *out_hit
)
{
    (void)world_user;
    (void)from;
    (void)to;
    (void)block_mask;
    if (out_hit != 0) {
        out_hit->hit = TDNE_FALSE;
    }
    return TDNE_FALSE;
}

int main(void)
{
    tdne_sensor eyes;
    tdne_target target;
    tdne_result result;

    tdne_sensor_init_cone(
        &eyes,
        tdne_vec3_make(0, 0, 0),
        tdne_vec3_make(0, 0, 1),
        500,
        90
    );

    tdne_target_init(&target, tdne_vec3_make(0, 0, 120), 12, 1UL, 0);
    tdne_eval_target(&eyes, &target, my_raycast, 0, &result);

    return result.visible ? 0 : 1;
}
```

## Raycast contract

You provide this callback if you want occlusion checks:

```c
typedef int (*tdne_raycast_fn)(
    void *world_user,
    const tdne_vec3 *from,
    const tdne_vec3 *to,
    tdne_u32 block_mask,
    tdne_ray_hit *out_hit
);
```

Return value:

```txt
0 = clear line
non-zero = blocked line
```

This keeps the library detached from your collision world. Your engine can implement the callback using a grid, BSP, octree, physics scene, portal graph, voxel map, or anything else.

## Target masks

Sensors and targets both use bitmasks:

```c
tdne_sensor_set_masks(&eyes, see_mask, block_mask);
tdne_target_init(&target, pos, radius, target_mask, user_ptr);
```

A target is considered perceivable only when:

```txt
sensor.see_mask & target.mask
```

is non-zero.

## Result states

```txt
visible
partial
occluded
out_of_range
out_of_shape
masked
bad_input
none
```

## Static arrays only

Scanning uses user-owned buffers:

```c
tdne_target targets[64];
tdne_result results[64];

count = tdne_scan_targets(
    &eyes,
    targets,
    target_count,
    results,
    64,
    my_raycast,
    world,
    TDNE_SCAN_SORT_BY_SCORE
);
```

## Coordinate notes

The public coordinate type is `long` through `tdne_i32`. Direction vectors are normalized to `TDNE_DIR_SCALE`, which defaults to `1024`.

Distance and multiplication helpers use saturating integer operations. For very large coordinate systems, scale your world positions before calling the library, or use local NPC-space coordinates around the sensor.

## File layout

```txt
3D_NPC_Eyes/
├─ include/3d_npc_eyes.h
├─ src/3d_npc_eyes.c
├─ examples/
│  ├─ demo_basic.c
│  ├─ demo_grid_raycast.c
│  └─ demo_frustum.c
├─ tests/test_3d_npc_eyes.c
├─ docs/
│  ├─ DESIGN.md
│  └─ PORTABILITY.md
├─ scripts/check_forbidden.py
├─ Makefile
└─ README.md
```
