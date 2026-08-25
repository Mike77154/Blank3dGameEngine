# Design notes

## The query model

The library treats collision checks as hypothetical placement tests:

1. Copy the caller's AABB.
2. Replace or offset its XYZ position.
3. Filter candidate objects.
4. Run AABB broadphase.
5. If either side requests precise collision, invoke the host callback.
6. Return Boolean or the first matching ID.
7. Never mutate the caller during a check.

This reproduces the useful observable behavior of temporary-move collision queries without write/restore side effects.

## Condition checker vs. physics engine

3DKin-GF answers questions for scripts, AI verbs, state machines, and behaviors. It deliberately does not own velocity, mass, impulses, rendering, meshes, or scene graphs.

## Floor and wall

`is_on_floor` probes a tiny distance opposite the configured up direction and accepts solids or jump-thrus. `is_under_ceiling` probes upward. `is_on_wall` probes both signs of the two non-up axes.

The default probe is one raw fixed-point unit, or 1/256 world unit with the default 24.8 format. It can be changed with `gk3d_world_set_probe`.

## Real-collision fallback

AABB is always the broadphase. When `GK3D_FLAG_PRECISE` is present, the host can decide overlap using its actual geometry. This keeps the condition API stable while allowing Emmerald 3D, BSP, voxel, capsule, or mesh code to provide the real answer.
