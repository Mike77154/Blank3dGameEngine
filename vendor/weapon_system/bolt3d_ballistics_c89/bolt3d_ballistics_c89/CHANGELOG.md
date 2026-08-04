# Changelog

## 0.3.0

- Added `B3D_QueryGravityHook` for solver-mode gravity supplied by a host physics, field, or gameplay system.
- Added `B3D_SolverGravityRequest` with transform, velocity, acceleration, age, timestep, binding, and fallback-gravity context.
- Preserved the original negative-Y projectile gravity whenever the hook is absent or declines the request.
- Added custom-vector and fallback gravity tests plus a solver example.

## 0.2.0

- Added parallel caller-owned solver-state and contact arenas.
- Added solver and external-transform motion modes.
- Added fixed-point basis and transform math.
- Added iterative block, slide, bounce, stick, pierce, overlap, and ignore responses.
- Added transform read/write, collision query, and contact response hooks.
- Added solver transform/contact events.
- Added separate source and target pointers in event payloads.
- Propagated collider user types through hits, filters, contacts, and events.
- Applied hit filtering to world/custom trace results.
- Implemented ballistic stick-on-world behavior.
- Prevented custom hits from being treated as entity damage by default.
- Added solver object preset, example, and tests.

## 0.1.0

- Initial fixed-point projectile, hitscan, melee, collision, explosion, hook, and event implementation.
