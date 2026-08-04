# grecoil89_v1

`grecoil89` is a tiny C89 recoil gameplay solver for 3D games.

It is designed for engines that need deterministic, low-level, no-heap gameplay code:

- C89 / `-std=c89`
- fixed-point Q8.8
- no `malloc`, `free`, `realloc`
- no heap ownership
- no `float` / `double`
- declarative weapon profiles
- optional recoil/spray patterns
- spring-damping recovery
- camera-agnostic
- weapon-system-agnostic
- callback bridge

## What the solver does

Every shot pushes velocity into two small springs:

1. angle spring: pitch/yaw/roll recoil
2. weapon offset spring: side/up/back weapon movement

Then every update tick damps and returns those offsets toward neutral.

This gives the weapon a usable kick without hard-coding the solver to a renderer, camera, animation system, physics body, raycaster, HUD, or weapon system.

## Research model used

The library follows a game-feel model inspired by common FPS recoil practices:

- Real recoil comes from conservation of momentum and the projectile/gas impulse.
- Muzzle rise is a torque problem caused by recoil force and weapon/support geometry.
- In games, recoil feels better when separated into aim recoil, camera punch, weapon kick, spread bloom, patterns, and recovery.
- A spring-damper is a practical way to make recoil return smoothly without jerky crosshair jumps.

## Coordinate convention

Angles:

```txt
pitch: positive = muzzle / aim climbs upward
yaw:   positive = aim drifts right
roll:  positive = view/model rolls clockwise by your engine convention
```

Weapon offset:

```txt
side: positive = right
up:   positive = up
back: positive = weapon moves backward toward holder/camera
```

## Basic usage

```c
#include "grecoil89.h"
#include "grecoil89_profiles.h"

GRecState state;
GRecContext ctx;
GRecOutput out;
const GRecProfile *profile;

profile = grec_get_profile(GREC_PROFILE_RIFLE_556);
grec_state_init(&state, 1234u);
grec_context_for_mode(&ctx, GREC_MODE_HIP);

/* On fire */
grec_fire(&state, profile, &ctx);

/* Each fixed gameplay tick */
grec_update(&state, profile);
grec_sample(&state, profile, &ctx, &out);

/* Your engine consumes out.camera_angles, out.aim_angles,
   out.weapon_angles, out.weapon_offset and out.spread. */
```

## Callback bridge usage

```c
#include "grecoil89_bridge.h"

GRecBridge bridge;

grec_bridge_init(&bridge,
                 engine_user_pointer,
                 apply_camera_fn,
                 apply_aim_fn,
                 apply_weapon_fn,
                 on_fire_fn,
                 on_update_fn);

/* after fire */
grec_bridge_after_fire(&bridge, &state, profile, &ctx);

/* after update */
grec_bridge_after_update(&bridge, &state, profile, &ctx);
```

## Build

Linux/MSYS2/MinGW style:

```sh
make
./demo_grecoil89
./demo_bridge
```

Manual:

```sh
gcc -std=c89 -Wall -Wextra -pedantic -Iinclude \
  src/grecoil89.c src/grecoil89_profiles.c src/grecoil89_bridge.c \
  demo/demo_grecoil89.c -o demo_grecoil89
```

## Files

```txt
grecoil89_v1/
├─ include/
│  ├─ grecoil89.h
│  ├─ grecoil89_profiles.h
│  ├─ grecoil89_bridge.h
│  └─ grecoil89_provider.h
├─ src/
│  ├─ grecoil89.c
│  ├─ grecoil89_profiles.c
│  ├─ grecoil89_bridge.c
│  └─ grecoil89_provider.c
├─ demo/
│  ├─ demo_grecoil89.c
│  ├─ demo_bridge.c
│  └─ demo_provider.c
├─ tools/
│  └─ grecoil89_curve_dump.c
├─ docs/
│  ├─ DESIGN.md
│  ├─ PROFILE_GUIDE.md
│  ├─ INTEGRATION.md
│  ├─ PROVIDER.md
│  └─ PROVIDER_TEST_LOG.txt
├─ Makefile
├─ build_mingw32.bat
└─ LICENSE-CC0.txt
```

## External transform provider (v1.1)

For a generic external camera/entity/socket transform library, include:

```c
#include "grecoil89_provider.h"
```

The provider supports:

- `PUSH`: `grecoil89` calls external `set_rotate` / `set_move` callbacks.
- `PULL`: the external library queries the latest transform layer.
- independent aim, camera and weapon rotation channels.
- weapon translation channel.
- channel masks and explicit neutral clear.

See `docs/PROVIDER.md` and `demo/demo_provider.c`.
