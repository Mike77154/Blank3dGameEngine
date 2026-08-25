# Blank3D + 3d_mounting_system89 box-car prototype

This branch vendors the user-supplied `3d_mounting_system89 v0.1.0` unchanged
under `vendor/3d_mounting_system89/` and adds a thin Blank3D host adapter.

## Goal

Prove that a normal Blank3D player can become a `guest` of a moving `host`
without transferring Actor/Object/ECS ownership.

## Scene

- White car proxy: one procedural box, 2.4 x 1.2 x 4.0 world units.
- Spawn: `(5, 0.6, 6)`; its bottom sits on the y=0 floor.
- Static mount point: `driver_seat`, 1.05 units above the car origin.
- Mount radius: 2.75 units.
- Player remains the normal Blank3D player object.

## Controls

- `M`: mount when near the white box; press again to dismount.
- Existing movement DSL remains authoritative.
- While mounted, walk/run/strafe/turn verbs are consumed by the car.
- `V` remains camera-profile cycling; it was deliberately not reused.

## Speeds

- Normal player move configured by startup.rpy: 7 u/s.
- Car drive: 16 u/s.
- Car run/boost path: 19 u/s.
- Car strafe prototype: 10 u/s.
- Car turn: 105 deg/s.

The vehicle is intentionally faster so mounting can be verified by feel, not
only by logs.

## Ownership and update order

```
DDSL/gameverbs
    |
    +-- unmounted -> player locomotion / gloco
    |
    +-- mounted   -> car Transform
                       |
                  mount89_update
                       |
                       +--> writes player guest Transform at driver_seat

collision/world sync
render
```

When mounted, player gloco locomotion is suspended. The integration does not
manually copy the car position into the player from the input code; the only
attachment authority is `mount89_update()` through the transform provider.

On dismount the guest is kept in world space, moved two units to the car's
right, placed back on the y=0 test floor, and player locomotion is restored.

## Deliberate prototype limits

This is a mounting/drive proof, not a vehicle-physics implementation. The box
currently has no suspension, wheel model, acceleration curve, engine torque,
terrain solver, or dedicated vehicle collision body. Those can be providers
above/below the same mounting relationship later.

## QA

`make test-mount-vehicle` proves:

1. far-away player cannot mount;
2. nearby player mounts;
3. mount89 places the guest at the seat;
4. one second of car drive travels ~16 units, greater than player speed 7;
5. guest follows the moving host;
6. dismount breaks the link and returns player to ground.

`make syntax-check` covers the full Blank3D source set with the vendored
mount89 source linked into the runner.
