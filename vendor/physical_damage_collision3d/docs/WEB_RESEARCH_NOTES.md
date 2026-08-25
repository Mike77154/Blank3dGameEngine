# Web research notes used for this pass

PDC3D follows four practical melee-combat ideas from current/common engine practice:

1. **Frame-data attack windows**: attacks should have explicit startup, active and recovery frames. PDC3D keeps this in `pdc3d_attack_profile` and delegates timing to `melee3d`.
2. **Swept traces instead of only overlaps**: fast melee should sample previous and current positions. PDC3D exposes `pdc3d_attack_set_hit_capsule_from_sockets()` so animation/weapon sockets can feed a swept capsule every fixed tick.
3. **Shape traces, not only line traces**: capsule/sphere/box style queries are better for weapons, claws, bites and tentacles because they represent thickness.
4. **Fixed timestep**: combat resolution is deterministic and stable when advanced on a fixed tick, not variable render delta.

All of this was adapted to the user's protocol: C89, fixed point, no malloc/calloc/realloc/free, no heap, no float/double.
