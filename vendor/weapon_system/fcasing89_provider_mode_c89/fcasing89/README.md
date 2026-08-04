# FCASING89

Shooter-agnostic shell casing visual module for C89 engines.

- C89-compatible API
- fixed-point Q8 math
- no malloc, free, realloc, heap, float, or double
- caller-owned system struct
- fixed pool with free-list reuse
- simulated near casings and fake far casings
- distance/budget based LOD
- bounce audio event queue
- render item collection for any renderer
- optional external provider for gravity, move/rotate/scale math, and collisions
- does not know player/ally/enemy

## Build

```sh
make
./demo_casing_stdout
```

Or compile directly:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude src/fcasing89.c examples/demo_casing_stdout.c -o demo_casing_stdout
```

## Minimal usage

```c
FCasing89System casings;
FCasing89Config cfg;
FCasing89Camera cam;
FCasing89Spawn spawn;

fcasing89_default_config(&cfg);
fcasing89_init(&casings, &cfg, 12345UL);

cam.pos = fcasing89_vec3(FCASING89_TO_FIX(0), FCASING89_TO_FIX(20), FCASING89_TO_FIX(-20));
cam.valid = 1U;
fcasing89_set_camera(&casings, &cam);

spawn.origin = fcasing89_vec3(FCASING89_TO_FIX(0), FCASING89_TO_FIX(18), FCASING89_TO_FIX(0));
spawn.forward = fcasing89_vec3(0L, 0L, FCASING89_FIX_ONE);
spawn.right = fcasing89_vec3(FCASING89_FIX_ONE, 0L, 0L);
spawn.up = fcasing89_vec3(0L, FCASING89_FIX_ONE, 0L);
spawn.profile_id = FCASING89_PROFILE_RIFLE;
spawn.count = 1U;
spawn.importance = 220U;
spawn.flags = 0U;
spawn.seed_bias = 0U;
spawn.local_side_bias = 0L;
spawn.local_up_bias = 0L;
spawn.local_back_bias = 0L;

fcasing89_begin_frame(&casings);
fcasing89_emit(&casings, &spawn);
fcasing89_update(&casings, 16U);
```

## Design

The module is agnostic by contract: weapons, enemies, allies, turrets, editor tools, and cutscenes all send the same `FCasing89Spawn` packet. Visual quality is selected by `importance`, camera distance, and pool pressure.


## External provider mode

Version 1.1 keeps the complete original standalone behavior and adds optional,
per-service callbacks. An engine may provide gravity, move, rotate, scale, and
collision services. Missing callbacks or callbacks that return zero fall back
to FCASING89's original fixed-point implementation.

See `docs/provider_mode.md` and `examples/demo_external_provider.c`.
