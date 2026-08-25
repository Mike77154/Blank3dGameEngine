# 3d_mounting_system89 v0.1.0

A small, engine-agnostic 3D mounting/attachment core written in strict C89 style.
It does not know what a vehicle, turret, rider, enemy, weapon, chair, camera, or
item is. It only knows hosts, mount points, guests, links, and transforms.

## Constraints

- C89 / gnu89 friendly.
- No malloc, realloc, free, heap ownership, float, or double.
- Fixed-size compile-time arrays.
- Position: signed Q16.16 stored in `long`.
- Orientation: 3x3 signed Q1.14 matrix stored in `short`.
- Rigid transforms only: no scale/shear in v0.1.0.
- No ownership changes in the host engine's Object/ECS hierarchy.
- All engine integration happens through callbacks/providers.

## Why this shape

The design borrows three useful ideas from mainstream engines without copying
their object model:

1. A mount point is a named/identified transform relative to a host, analogous
   to a socket.
2. A dynamic mount point can be resolved from a skeleton/bone each update.
3. Mounting can either snap to the point or preserve the guest's current world
   transform by computing a local offset.

Official references used while designing it:

- Epic Games, Skeletal Mesh Sockets:
  https://dev.epicgames.com/documentation/unreal-engine/skeletal-mesh-sockets-in-unreal-engine
- Epic Games, Static Mesh Sockets:
  https://dev.epicgames.com/documentation/unreal-engine/using-sockets-with-static-meshes-in-unreal-engine
- Godot, BoneAttachment3D:
  https://docs.godotengine.org/en/stable/classes/class_boneattachment3d.html
- Unity, Transform.SetParent:
  https://docs.unity3d.com/2022.3/Documentation/ScriptReference/Transform.SetParent.html

## Core model

```text
Host object
   |
   +-- MountPoint (static local transform)
   |       |
   |       +-- MountLink --> Guest object
   |
   +-- MountPoint (dynamic local transform provider, e.g. bone)
           |
           +-- MountLink --> Guest object
```

The host and guest remain independent engine objects. `mount89_update()` only
asks the transform provider for host transforms and writes the resulting guest
transform.

## Providers

### Object transform provider

```c
mount89_set_transform_provider(&ctx, get_world, set_world, user);
```

The engine owns the objects and their transforms. mount89 only reads/writes them.

### Dynamic point provider

```c
mount89_set_point_provider(&ctx, resolve_point, user);
```

Use this for skeleton bones, animated sockets, procedural anchors, physics
anchors, or any other host-local point that changes over time.

The callback returns a transform in HOST-LOCAL space. mount89 then composes it
with the host world transform.

### Event provider

```c
mount89_set_event_provider(&ctx, on_event, user);
```

Emits successful MOUNTED and UNMOUNTED events. Gameplay systems may respond by
changing animation, collision, AI state, input ownership, etc. mount89 itself
never does those jobs.

## Mount modes

- `MOUNT89_MOUNT_SNAP`: guest offset becomes identity, so the guest snaps to the
  mount point.
- `MOUNT89_MOUNT_KEEP_WORLD`: computes `inverse(point_world) * guest_world`, so
  the guest remains where it is at the moment the relationship is created.
- `MOUNT89_MOUNT_OFFSET`: uses a caller-supplied guest-local offset.

## Inheritance flags

- `MOUNT89_INHERIT_POSITION`
- `MOUNT89_INHERIT_ROTATION`
- `MOUNT89_INHERIT_ALL`

This lets a mount follow only position or only orientation when desired.

## Compatibility masks

Each point has an `accept_mask`. `0` means wildcard. Otherwise at least one bit
must overlap the `guest_mask` supplied to `mount89_mount()` / `mount89_can_mount()`.
This is deliberately generic: the bits can mean rider, prop, small creature,
weapon, camera, or anything else defined by the caller.

## Nested mounts

Nested mounting is supported:

```text
monster A
  +-- monster B
       +-- soldier C
```

Update order is resolved by mount depth so parents are written before mounted
children. Cycles are rejected when a link is created.

## Example

```c
mount89_context mounts;
mount89_transform saddle;

mount89_init(&mounts);
mount89_set_transform_provider(&mounts, my_get_world, my_set_world, my_world);

mount89_transform_identity(&saddle);
saddle.p[1] = mount89_fx_from_int(2L);

mount89_add_static_point(&mounts,
                         horse_id,
                         saddle_point_id,
                         RIDER_MASK,
                         &saddle);

mount89_mount(&mounts,
              horse_id,
              saddle_point_id,
              enemy_id,
              RIDER_MASK,
              MOUNT89_MOUNT_SNAP,
              MOUNT89_INHERIT_ALL,
              0,
              0);

/* once per frame, after host/bone transforms are current */
mount89_update(&mounts);
```

## Suggested engine update order

```text
animation/skeleton pose
       |
physics/world transforms
       |
mount89_update
       |
collision proxies / hitboxes that consume final guest transform
       |
render
```

Dynamic bone points should be resolved after the skeleton pose for the frame is
current.

## Capacity

Defaults:

```c
#define MOUNT89_MAX_POINTS 128
#define MOUNT89_MAX_LINKS   64
```

Override before including `mount89.h`, and compile the library with the same
values.

## Building

MinGW/MSYS2:

```sh
./build_mingw.bat
```

POSIX shell with GCC/Clang:

```sh
sh build.sh
```

The test executable prints `mount89: all tests passed` when successful.
