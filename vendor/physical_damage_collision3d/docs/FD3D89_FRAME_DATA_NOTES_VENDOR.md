# Frame-data notes used by fdata3d89

The library keeps fighting-game timing as explicit frame arrays and adapts the hitbox/hurtbox model to 3D volumes.

3D additions:

- lane masks allow side lanes, belt-fighter depth lanes, or arena zones;
- side-step frames can evade non-homing attacks;
- AABB volumes can represent fists, legs, weapons, body sections, projectiles, throws, and proximity tests;
- authored spheres are conservatively converted to AABBs to avoid multiplication-heavy sphere checks in tiny C89 runtimes.

This is not AI and not animation. It is the deterministic frame-data resolver that animation, physics, camera, and gameplay systems can call.


## Protocol patch notes

- `FD3D_FX_FROM_INT(v)` is multiplication-based to avoid left-shifting negative signed values.
- `FD3D_ArenaAlloc` accepts `align == 0` for default alignment and rejects non-power-of-two non-zero alignments.
- These changes keep the library C89-only, fixed-point-only, and heap-free.
