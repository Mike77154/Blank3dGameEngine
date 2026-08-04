# fly89

Provider-driven, gravity-free vertical displacement for opaque actors.

## Purpose

- ascend or descend at a requested speed;
- move toward an absolute Y coordinate;
- clamp downward motion to a floor;
- ask whether an actor is grounded and measure height above its floor.

`fly89` does not apply gravity by itself. The host decides whether an actor is
marked as a flying/gravity-immune entity. This makes the library useful for
flying actors, noclip movement, ladders, elevators, swimming, scripted vertical
motion, and editor gizmos.

## Build

```sh
make
```

Produces `build/libfly89.a`. Override `CC`, `AR`, or `CFLAGS` as needed.
The shared provider header is under `../vertical_motion89_common/include`.

## Required providers

- math: `add`, `sub`, `mul`, `abs_value`;
- transform: `get_position`, `set_position`.

The physics provider is optional. When supplied, its floor and movement
callbacks take precedence over the fallback transform write.
