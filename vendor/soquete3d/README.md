# Soquete3D 1.0

Soquete3D is a small C89-compatible locator for **where a thing or named socket is right now**.

It is deliberately **not**:

- a world manager;
- a scene graph;
- an entity-component system;
- an animation system;
- a collision system;
- an ownership or lifetime manager.

The caller publishes poses. Soquete3D stores them in fixed-capacity arrays and answers queries.

## Core model

```text
engine / animation / physics
          |
          | publishes current poses with stamp N
          v
+-----------------------------+
|          Soquete3D          |
| thing key -> world pose     |
| socket key -> local/world   |
| exact-stamp freshness check |
+-----------------------------+
          |
          | query(key, stamp N)
          v
 weapon / camera / FX / HUD / AI
```

A local socket has exactly one owner **thing**. Sockets cannot own sockets, so there is no recursive hierarchy hiding inside the library.

For skeletal animation, the animation provider can publish a socket already resolved in world space with `soq3d_publish_world_socket()`.

## Why poses contain a 3x3 basis

A position-only socket would fail when its owner rotates. Soquete3D accepts a row-major fixed-point basis plus a translation. The basis may contain rotation and scale. Soquete3D does not generate rotations or own transforms; it only composes one local socket pose with one current owner pose.

## Fixed point

- signed Q16.16
- `SOQ3D_FX_ONE == 65536`
- saturating addition and multiplication
- no `float`, `double`, `long long`, allocator, heap, or math library
- requires 32-bit `unsigned int`

## Basic use

```c
soq3d_context locator;
soq3d_key actor;
soq3d_key muzzle;
soq3d_pose actor_world;
soq3d_pose muzzle_local;
soq3d_pose muzzle_world;

soq3d_init(&locator);
actor = soq3d_key_from_cstr("actor.player");
muzzle = soq3d_key_from_cstr("actor.player.muzzle");

muzzle_local = soq3d_pose_make(
    soq3d_vec3_make(0, 0, soq3d_fx_from_int(1)),
    soq3d_basis_identity());
soq3d_define_local_socket(&locator, muzzle, actor, &muzzle_local);

soq3d_begin_frame(&locator, 400U);
actor_world = soq3d_pose_identity();
actor_world.position.x = soq3d_fx_from_int(12);
soq3d_publish_thing(&locator, actor, &actor_world);

if (soq3d_get_socket(&locator, muzzle, 400U, &muzzle_world) == SOQ3D_OK) {
    /* muzzle_world is guaranteed to come from stamp 400. */
}
```

Use `SOQ3D_STAMP_ANY` when stale data is acceptable.

## Capacities

Defaults:

```c
#define SOQ3D_MAX_THINGS 256
#define SOQ3D_MAX_SOCKETS 512
```

Override them at compile time before including the header, or through compiler definitions:

```sh
gcc -DSOQ3D_MAX_THINGS=1024 -DSOQ3D_MAX_SOCKETS=2048 ...
```

No capacity change happens at runtime.

## Build

```sh
make
make audit
```

MinGW32/MSYS2:

```sh
mingw32-make CC=gcc AR=ar
```

The Makefile builds:

- `build/libsoquete3d.a`
- `build/test_soquete3d`
- `build/soquete3d_demo`

## Integration rule

A provider bridge should do one of two things per simulation frame:

1. publish each thing's world pose; or
2. publish already-resolved world sockets from a skeletal/animation system.

Soquete3D never pulls from the engine and never mutates the engine. This keeps the ABI agnostic.

## License

CC0 1.0 Universal. See `LICENSE.txt`.
