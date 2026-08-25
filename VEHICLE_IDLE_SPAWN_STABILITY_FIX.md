# Vehicle idle spawn stability fix

## Problem

The first Warthog prototype used a `0.10`-unit suspension preload to avoid a
zero-compression free-fall frame.  That preload became excessive once INI
vehicle authoring introduced lighter/scaled wheeled profiles (especially the
`rally89` motorcycle recipe), producing a large idle vertical oscillation.
Tank profiles had a separate one-frame pop because `tankmovement89` clamps a
TankSim body to `y >= 2` after integration while the yard authored the tank at
`y=1`.

## Fix

The Blank3D/gvehicle bridge now primes wheeled ground contact with the smallest
positive Q8 compression (`1/256` unit) instead of `0.10`.  This is sufficient
for `carmovement89`'s strict `compression > 0` contact gate without storing a
visible spring impulse.

For `GVEH_CLASS_TANKSIM`, the existing `y >= 2` chassis floor is applied before
the initial entity state is published.  This does not create a new tank rule;
it simply applies the rule already enforced by `tankmovement89_post_integrate`
one frame earlier.

Airborne wheeled spawns more than 0.25 units away from neutral wheel-contact
height are not snapped and retain their authored height.

## Regression

`tests/test_vehicle_ini_yard.c` now leaves car, motorcycle and tank untouched
for 600 frames (10 seconds at 60 Hz) before mounting anything and requires each
body's Y range to stay within 1/64 unit.  This catches recurrence of the
mechanical-bull spawn oscillation.
