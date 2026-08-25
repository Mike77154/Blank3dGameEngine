# Integration notes

## What mount89 owns

- Static registry of mount points.
- Static registry of active host/guest links.
- Compatibility masks.
- Local guest offsets.
- Parent-before-child update ordering.
- Cycle rejection.
- Snap / keep-world / explicit-offset mounting.
- Optional position/rotation inheritance.

## What mount89 intentionally does not own

- Object lifetime.
- ECS parenting.
- Vehicle controls.
- Turret aiming.
- AI.
- Animation state selection.
- Physics bodies or joints.
- Collision enable/disable.
- Input ownership.
- Inventory/equipment semantics.
- Networking.

Those systems can react to mount events or query mount state.

## Bone adapter pattern

A skeletal adapter can use `source_id` as a bone index:

```c
static int model_mount_point(void *user, int host_id, int bone_index,
                             mount89_transform *out_local)
{
    model_instance *m;
    m = model_lookup(user, host_id);
    if (m == 0) return 0;
    return model_get_bone_host_local(m, bone_index, out_local);
}
```

No skeleton code is compiled into mount89.

## Physics adapter pattern

If a guest is kinematic while mounted, the host engine can switch that state in
its `MOUNT89_EVENT_MOUNTED` handler and restore it on `MOUNT89_EVENT_UNMOUNTED`.
The core remains physics-agnostic.

## Multiple seats / slots

One MountPoint accepts one active guest. Multiple occupancy is represented by
multiple points:

```text
host
 +-- seat_0
 +-- seat_1
 +-- roof_left
 +-- roof_right
```

This keeps occupancy deterministic and avoids hidden per-point heap/list state.
