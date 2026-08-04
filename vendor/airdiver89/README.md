# airdiver89

Provider-driven aerial descent, ram, and return-path capability.

The library can:

- store an actor's current aerial return position;
- descend vertically to an absolute Y coordinate;
- move/ram toward any 3D point with normalized, clamped stepping;
- return to the previously stored position without overshooting it.

It does not know players, enemies, damage, AI states, or Gamlib3D. The host
supplies the target point and decides what happens when `out_reached` becomes
true. This supports dive bombers, falling attacks, bird-like pecks, drop pods,
scripted landings, and return-to-perch behavior.

## Build

```sh
make
```

Produces `build/libairdiver89.a`.

## Required providers

- math: `add`, `sub`, `mul`, `abs_value`, `length3`, `normalize3`;
- transform: `get_position`, `set_position`.

The optional physics provider can resolve the requested movement against the
host world's collisions and floor.
