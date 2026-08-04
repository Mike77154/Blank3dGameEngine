# jump89

Generic fixed-point jump capability for an opaque actor.

`jump89_start()` accepts an upward impulse only when the actor is grounded.
`jump89_tick()` integrates vertical velocity with the host-provided gravity
acceleration, resolves movement through an optional physics provider, clamps to
the floor, and resets the actor-local state after landing.

The host owns one `jump89_state` per actor, so many actors can jump through one
shared `jump89_context` without hidden global state or allocation.

## Build

```sh
make
```

Produces `build/libjump89.a`.

## Required providers

- math: `add`, `sub`, `mul`, `abs_value`;
- transform: `get_position`, `set_position`.

The optional physics provider can supply a dynamic floor, grounded state, and
collision-resolved position. Gravity enable/disable is passed explicitly by the
host, so the library is not tied to a particular configuration system.
