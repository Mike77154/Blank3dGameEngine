# Blank3D: gvehicle89 + gvehpos89 + Mount89 integration

## Purpose

This integration turns the earlier white-box Mount89 proof into a real vehicle
stack.  The white box remains only a temporary low-poly presentation mesh; its
motion is no longer authored by direct Transform movement in Blank3D.

The authority split is intentional:

```text
Actor / Game Verbs
       |
       v
gvehpos89                 possession / seat / driver authority
       |
       v
Mount89                   spatial guest -> mount-point relationship
       |
       v
Vehicle Object Transform
       ^
       |
gvehicle89                vehicle simulation authority
```

GAttach is not used here.  GAttach remains the attachment mechanism for things
such as a weapon in a character hand/socket.  Mount89 remains the mechanism for
an actor occupying/operating another host such as a vehicle, turret, seat, or
mounted weapon.

## Vendor placement

The original libraries from `vehicle.zip` are kept as independent vendors:

```text
vendor/gvehicle89/
vendor/gvehpos89/
vendor/3d_mounting_system89/
```

Blank3D-specific knowledge lives in:

```text
src/blank3d_mount_vehicle.h
src/blank3d_mount_vehicle.c
```

Neither gvehicle89 nor gvehpos89 owns Blank3D Actor, GFO, ECS, GAttach, or
Gamlib3D transforms.

## Runtime path

When the player is on foot, normal Blank3D locomotion owns movement input.
When the player becomes the driver, the same Game Verbs feed a `GVPos_Input`.
The frame path is:

```text
walk_forward / turn_left / ...
        |
        v
Blank3D vehicle adapter
        |
        v
gvehpos89 driver input
        |
        v
gvehicle89 input / drivetrain / wheels / suspension / body
        |
        v
gveh entity bridge
        |
        v
Blank3D vehicle Transform
        |
        v
Mount89 update
        |
        v
player Transform at driver seat
```

There is no `transform_move_local_flat(car, ...)` driving the vehicle in the
adapter.  If the vehicle moves, gvehicle89 moved it.

## Seats

Vehicle mount points are authored from `gveh_profile.seats[]`.  The current
`warthog89` profile exposes three seats, therefore the adapter creates three
Mount89 points automatically:

```text
seat 0 -> driver
seat 1 -> passenger
seat 2 -> passenger
```

`gvehpos89` is the gameplay authority for whether a seat is free/reserved/
occupied and who the driver is.  Mount89 is the spatial authority once that
occupancy becomes mounted.

The upstream `set_actor_attached` callback from gvehpos89 is deliberately
mapped to Mount89.  It is *not* mapped to GAttach despite the callback's name.

## RPY authoring

Canonical test scene command:

```rpy
vehicle warthog89 pos 5 2.5 6 speed 16 run 19
```

`vehicle <profile>` resolves against the gvehicle89 profile bank, so the
adapter is not hardcoded to the Warthog profile.  `mount_car` remains as a
backward-compatible alias selecting `warthog89`.

The current white box is only a chassis visualization.  It is drawn lower than
the simulation body center so its visible base sits near the floor while the
real profile body/wheels/suspension retain their simulation coordinates.

## Controls in the prototype

- `M`: request driver mount / request exit.
- Existing forward movement verb: throttle.
- Existing left/right/turn verbs: steering.
- Existing backward verb: brake-to-reverse using gvehicle89 gear 0.

Normal player locomotion resumes after exit.

## Current provider boundary

This first integration intentionally supplies gvehicle89 with a small world
provider that represents the test scene as flat dry asphalt at `y = 0` and no
water.  It proves the vehicle stack without embedding Blank3D collision logic
inside the vendor.

The next environment step should replace only that provider with a Blank3D
world/collision query adapter.  gvehicle89 itself should remain unchanged.

## Reverse and basis bridge

The stock drivetrains already author `gear_ratio[0]` as a negative reverse
gear, but the upstream drivetrain update previously clamped every gear below
1 back to first gear.  Blank3D now vendors a small fix that keeps gear 0
reachable while automatic shifting remains limited to forward gears.

The adapter owns the direction-change policy:

- forward input while reversing first brakes, then selects forward gear;
- backward input while moving forward first brakes, then selects reverse;
- no direct vehicle Transform translation is used.

Blank3D/Gamlib3D also defines yaw-0 forward as `-Z`, whereas gvehicle89 uses
`+Z`.  The adapter applies the required 180-degree yaw basis conversion in
both directions so positive gvehicle throttle agrees with the visible
Blank3D vehicle nose.

## Parked suspension spawn priming

For the `warthog89` profile, the original test spawn at `y=2.5` landed exactly
on `suspension.rest_len + wheel.radius`.  gvehicle89 only marks a wheel
grounded when suspension compression is strictly positive, so the first frame
had zero support, dropped under gravity, and entered a long bounce cycle.

The Blank3D flat-ground adapter now detects a wheeled spawn close to its neutral
contact height and primes 0.10 world units of suspension compression.  Deliberately
airborne spawns are not snapped to the ground.

## Fixed-point / allocation policy

The integrated boundary is C89 and fixed-point.  Source audits over
`vendor/gvehicle89`, `vendor/gvehpos89`, and `src/blank3d_mount_vehicle.*`
found no heap allocation calls.  Executable source contains no `float` or
`double` tokens; occurrences found by the audit are documentation statements
saying those types are not used.

## Verification

The integration test verifies the complete authority chain rather than moving
an artificial car Transform:

1. a distant actor cannot mount;
2. `warthog89` produces three seats;
3. a nearby actor becomes the gvehpos89 driver;
4. Mount89 reports the actor mounted;
5. an unoccupied vehicle remains vertically stable at the authored 2.5-unit test spawn;
6. forward Game Verb input reaches gvehicle89 and moves along the Blank3D Transform forward axis;
7. gvehicle89 reports positive forward speed;
8. backward input brakes then reaches the authored reverse gear 0;
9. the vehicle Transform moves through the entity bridge;
10. the player follows the moving seat through Mount89;
11. exit clears driver/mount state and restores the actor to the world.

The real startup RPY path is also covered by `tests/test_languages.c`.
