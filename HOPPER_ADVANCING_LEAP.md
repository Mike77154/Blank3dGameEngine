# Hopper advancing leap upgrade — v3.10.2

## Goal

Turn the vertical-displacement test hopper into a pursuer that performs large,
readable leaps toward the player and stops hopping once it reaches close range.

## Runtime additions

`src/monika_blank3d.c` now exposes:

```text
plrflatdistwithin=N
plrflatdistfurther=N
moveforeflat=N
```

The planar conditions and action operate on X/Z only. They intentionally do not
replace the existing full-3D `plrdist*` and `movefore` operations.

## Sample state machine

```text
         farther than 5, grounded
 IDLE ------------------------------> ASCEND
  ^                                      |
  |                                      | height >= 3
  |                                      v
  +-------------- grounded <--------- DESCEND

At any airborne point, entering the 5-unit horizontal radius disables forward
movement and makes the hopper finish its landing vertically.
```

## Default tuning

```text
horizontal pursuit speed = 8 units/second
vertical ascent speed     = 7 units/second
vertical descent speed    = 7 units/second
apex height               = 3 units
stop/restart radius        = 5 horizontal units
```

All values live in `scripts/hopper_enemy.fpi` and can be tuned without recompiling.
