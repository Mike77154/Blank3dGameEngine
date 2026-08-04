# Projectile mesh, Gatling spawn and orientation fix — v3.2

## Root causes

1. `gbulletmesh89` and `rocketmeshes` were present under the weapon-system vendor
   tree but were not compiled by the root Makefile. The runner substituted generic
   spheres, cylinders and cones.
2. Those generic cylinders were authored along local Y, while the projectile pose
   merely copied player rotation. They therefore appeared vertical and ignored the
   true pitch/spread vector.
3. The Gatling manager produced projectile events, but the tiny vertical generic
   tracer could be visually lost; under a fully occupied fixed bullet pool, a new
   Gatling tracer could also be rejected.

## Corrections

- Added `blank3d_projectile_mesh` as the authoritative adapter for the two
  specialized mesh libraries.
- Added a single-pellet builder to `gbulletmesh89`, matching the manager's one-event-
  per-pellet shotgun behavior.
- Standardized all projectile meshes to local +Z as the nose.
- Constructed every bullet transform from its actual velocity vector, not from the
  player's body transform.
- Routed Gatling shots through the normal provider event path and added deterministic
  tracer recycling when the fixed pool is saturated. Explosive projectiles are never
  stolen.
- Added portable tests for all eight mappings, long-axis orientation and a real
  Gatling shot (`weapon_id=8`, `mesh_id=8`, belt 300 -> 299).

## Build

```sh
make clean
make
```

The distribution intentionally contains no foreign `.o` or `.a` build artifacts.
