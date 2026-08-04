# External provider mode

FCASING89 1.1 adds an optional provider table without removing the original
standalone simulation. Every service is independently hookable:

- gravity
- move math
- rotate math
- scale math
- collision queries

The library still owns its fixed casing pool, LOD, profiles, lifetime, events,
sleep state, and render collection. It never allocates provider memory.

## Binding a provider

```c
FCasing89Provider provider;

fcasing89_provider_init(&provider);
provider.user = &my_static_engine_context;
provider.enabled_mask =
    FCASING89_PROVIDER_GRAVITY |
    FCASING89_PROVIDER_MOVE |
    FCASING89_PROVIDER_ROTATE |
    FCASING89_PROVIDER_SCALE |
    FCASING89_PROVIDER_COLLISION;

provider.gravity = my_gravity;
provider.move = my_transform;
provider.rotate = my_transform;
provider.scale = my_transform;
provider.collision = my_collision;

fcasing89_set_provider(&casings, &provider);
```

`provider.user` is only stored and passed back. It may point to static data, an
arena-owned object, or an engine singleton. FCASING89 does not allocate, own,
resize, or free it.

Use `fcasing89_remove_provider(&casings)` to return every service to the
original built-in implementation.

## Fallback contract

Each callback returns:

- non-zero: the provider handled the request
- zero: FCASING89 executes its original internal path for that request

A callback may therefore be installed globally and selectively decline work.
A missing callback also falls back automatically. The `enabled_mask` can turn
services on and off without rebuilding the table.

## Gravity

The gravity callback receives the casing position, velocity, profile/state
metadata, `dt_ms`, and the acceleration FCASING89 would normally use.

Write a Q8 acceleration vector to `out_acceleration`. This permits ordinary
vertical gravity, directional gravity, local gravity volumes, zero gravity, or
a provider-specific lookup.

## Move and rotate

Move and rotate use `FCasing89ProviderTransformQuery`:

- `current`: current position or rotation
- `delta`: already time-stepped Q8 delta for this update
- `source`: velocity for move, spin for rotate
- `operation`: move or rotate
- `dt_ms`: current update step

The provider writes the resulting position/rotation to `out_value`.

The built-in fallback is `current + delta`.

## Scale

Scale is evaluated when a casing is spawned. Without
`FCASING89_FLAG_CUSTOM_SCALE`, the requested factor is `(1,1,1)` in Q8.
With the flag set, `FCasing89Spawn.scale` is used.

The provider receives:

- `current`: `(1,1,1)`
- `delta`: requested scale factor
- `source`: requested scale factor

The built-in fallback is component-wise Q8 multiplication. The final value is
stored in `FCasing89Node.scale` and returned in
`FCasing89RenderItem.scale`.

## Collision

External collision is queried for simulated casings after gravity and move.
Fake LOD casings intentionally keep their original no-collision path.

The query contains:

- old and proposed position
- current velocity
- casing radius
- profile/index/state metadata
- `dt_ms`

`FCasing89ProviderCollisionResult` is prefilled with the proposed position and
current velocity. A provider that owns the collision world should return
non-zero on every query, including no-hit queries. On no hit, it can leave the
prefilled result unchanged.

For a hit, set `FCASING89_COLLISION_HIT`, write a corrected position, and write
a Q8 normalized surface normal.

Optional flags:

- `FCASING89_COLLISION_SLEEP`: force the casing to sleep
- `FCASING89_COLLISION_VELOCITY_VALID`: use provider-resolved velocity
- `FCASING89_COLLISION_NO_BOUNCE_EVENT`: do not enqueue bounce audio

Without `VELOCITY_VALID`, FCASING89 applies the profile bounce and friction to
the supplied normal. This keeps collision detection provider-agnostic while
preserving casing behavior.

If the collision callback returns zero, the original `floor_y` collision is
used for that update.

## No dynamic allocation

The provider mode adds only copied function pointers, fixed structs, and Q8
integer operations. There is no `malloc`, `realloc`, `free`, heap container,
`float`, or `double`.
