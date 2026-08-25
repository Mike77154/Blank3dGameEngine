# gvehicle89 parked-bounce / forward-axis / reverse fix

## Symptoms reproduced

- The white `warthog89` box bounced continuously before the player mounted it.
- Positive throttle moved in the opposite direction from the visible Blank3D nose.
- Reverse ratio 0 existed in the drivetrain data but could never be selected.

## Root causes

### Parked bounce

The test scene authored the vehicle body center at `y=2.5`.  For `warthog89`:

```text
susp.rest_len = 1.5
wheel.radius  = 1.0
neutral contact threshold = 2.5
```

`gveh_apply_wheels()` requires `compression > 0` before a wheel becomes grounded.
At exactly 2.5, the first frame therefore had no wheel support; gravity generated a
large downward velocity and the simple suspension/mass-point solver entered a
repeating bounce.  The adapter now primes 0.10 units of compression only for
wheeled spawns close to the neutral contact height.  Airborne spawns stay airborne.

### Forward axis

Gamlib3D local forward at yaw 0 is `-Z`; gvehicle89 local forward at yaw 0 is `+Z`.
The original adapter copied yaw numerically, causing positive throttle to move out
of the visible rear of the box.  The adapter now converts yaw with a half-turn at
the boundary and converts it back on entity-bridge writes.  Pitch and roll are not
offset.

### Reverse

Both stock drivetrains already define `gear_ratio[0]` as a negative reverse gear,
but `gveh_drivetrain_update()` clamped `current_gear < 1` to first gear.  The vendor
fix allows gear 0 and excludes it from forward automatic shifting.  The Blank3D
adapter selects direction safely:

```text
W while reversing -> brake -> near stop -> forward gear
S while advancing -> brake -> near stop -> reverse gear 0
```

No direct vehicle Transform movement is used.

## Regression test

`tests/test_mount_vehicle.c` now verifies:

1. distant mount rejection;
2. 3 `warthog89` seats;
3. 240 idle frames with negligible vertical movement;
4. gvehpos89 driver ownership;
5. Mount89 spatial ownership;
6. positive throttle displacement along the Blank3D Transform forward vector;
7. real gvehicle89 forward speed;
8. backward input eventually produces displacement opposite the forward vector via gear 0;
9. dismount restores on-foot state.

The fix stays inside the gvehicle/Blank3D adapter boundary; Mount89 and GAttach are
unchanged.
