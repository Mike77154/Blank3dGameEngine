# gvehicle89 v6 engine adapter notes

This version turns the v5 vehicle library into a cleaner vehicle subengine layer.
It keeps the original C89/fixed-point/no-heap rule set and adds the engine-facing bridge files requested for integration.

## Layout

```txt
gvehicle89/
├─ vehicle gameplay kernel
├─ sim-lite physics
├─ profile-driven vehicle handling
├─ world-probe adapter
├─ FX telemetry layer
└─ optional arcade systems
```

## New public headers

```txt
include/
├─ gveh_config.h             build knobs and fixed-size ABI guards
├─ gveh_engine_adapter.h     fixed-slot vehicle manager
├─ gveh_entity_bridge.h      engine entity transform bridge
├─ gveh_collision_bridge.h   vehicle-vs-vehicle contact solver
└─ gveh_profile_bank.h       profile lookup by index/name
```

## New source files

```txt
src/
├─ gveh_engine_adapter.c
├─ gveh_entity_bridge.c
├─ gveh_collision_bridge.c
└─ gveh_profile_bank.c
```

## New demo

```txt
demo/demo_multi_vehicle89.c
```

This demo spawns three vehicles through `gveh_vehicle_world`, steps them in a fixed-slot world, resolves vehicle-vs-vehicle contacts, and writes:

```txt
demo_multi_vehicle89.csv
demo_multi_vehicle89.ppm
demo_multi_vehicle89.png
```

## Integration shape

```txt
engine input/controller  -> gveh_vehicle_world_input(...)
engine terrain/collision -> gveh_world_i probes
engine entities          -> gveh_entity_bridge_i callbacks
vehicle simulation       -> gveh_vehicle_world_step(...)
engine transforms        <- gveh_entity_bridge_push(...)
particles/audio          <- vehicle.fxq events
```

## Notes

- `gveh_i32`/`gveh_u32` now default to `signed int`/`unsigned int` and are compile-time checked as 32-bit.
- Roll is now applied in `gveh_basis_yaw_pitch_roll`, so aircraft/spacecraft get real banked local axes.
- `gveh_profile.module_flags` gates major simulation families at runtime.
- Compile-time module knobs live in `gveh_config.h`.
- `gveh_vehicle_world_step` keeps one shared frame tick for all vehicles in a multi-vehicle frame.
