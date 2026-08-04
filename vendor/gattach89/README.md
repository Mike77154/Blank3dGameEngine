# gattach89

`gattach89` is a small C89 fixed-point attachment/socket layer for low-level 3D engines.
It lets any entity expose named sockets and lets any other entity, prop, FX point, or hitbox follow those sockets.

## Constraints

- C89 source.
- Fixed point only: `gatt_fix` uses 20.12 fixed-point values.
- Static arrays only.
- No dynamic heap ownership by the library.
- No floating-point types.
- No required renderer, physics engine, ECS, or animation system.

## What it solves

Use it for:

- Visible weapons in hands.
- Weapons on back, hip, holster, or floor after detaching.
- Hidden/visible mesh parts: helmet, coat, backpack, broken arm, monster mutation parts.
- Subentities: shield, drone, tentacle, flashlight, muzzle flash, attached hitbox.
- FX points: muzzle, shell eject, blood point, flame point, audio/script event point.

## Model

```txt
entity transform
  -> optional bone transform
    -> socket local offset
      -> attachment local offset
        -> child world transform
```

A socket belongs to one owner entity. An attachment links a child entity to a socket.

## Important API

```c
GAtt89_World world;
gatt89_world_init(&world);

hand = gatt89_socket_add(&world, player_id, "hand_r",
                         GATTACH89_SOCKET_BONE, right_hand_bone,
                         GATTACH89_INVALID_ID, &socket_offset);

weapon = gatt89_attach_add(&world, player_id, pistol_id, "pistol_visible", "hand_r",
                           GATTACH89_ATTACH_ENTITY,
                           GATTACH89_AF_CALL_CHILD_SETTER |
                           GATTACH89_AF_RENDER_PACKET |
                           GATTACH89_AF_INHERIT_PARENT_VIS,
                           &weapon_offset);

gatt89_update(&world, &callbacks, &output);
```

## Callback bridge

The engine provides:

- `get_entity_xform`
- `get_bone_xform`
- `get_entity_visible`
- `set_entity_xform`
- `set_part_visible`
- `on_event`

That keeps `gattach89` agnostic. It can feed ECS, Thing system, renderer, collision, physics, script events, audio, or gameplay logic.

## Build

```sh
make
make check
./demo_gattach89
```

MinGW32 example:

```sh
gcc -std=c89 -pedantic -Wall -Wextra -Iinclude -c src/gattach89.c -o src/gattach89.o
```

## Notes for your engine

Recommended update order:

```txt
1. entity lifecycle / parent movement
2. animation pose
3. bone transform cache
4. gattach89_update
5. renderer batches
6. collision/hitbox bridge
7. physics bridge
8. script/audio events
```

For a weapon, make the weapon a child entity, not just a mesh. Then the weapon can own its own `muzzle`, `eject`, and `shell` sockets.

```txt
player hand_r -> pistol entity
pistol muzzle -> muzzle flash FX
pistol eject  -> casing/shell spawn
```

## Integration idea

Use this as a common declarative reaction bus for:

- `World3D89`
- `Scene3D89`
- `GFO`
- `GLK`
- `VisualStack2D` overlays that track 3D targets
- audio events
- script events
- hit/shoot/death events
- ECS/Thing lifecycle
