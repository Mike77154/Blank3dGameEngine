# gvehicle89 optional movement/physics providers

## Goal

`gvehicle89` remains a standalone fixed-point Vehicle System, but its internal
movement and body-physics work is no longer mandatory. A host may provide only
the capabilities it wants to own; every unhandled capability falls back to the
existing internal subvendors.

```text
Game input
   |
   v
gvehicle89 facade
   |
   +-- movement provider? -- handled --> host backend (Gamlib/custom)
   |          |
   |          +-- declined -----------> *movement89 fallback
   |
   +-- physics provider? ---- handled --> host backend (VPhysics/custom)
              |
              +-- declined -----------> vehiclephysics89 fallback
```

The contracts live in:

```text
vendor/gvehicle89/vendor/vehicleprovider89/
```

They deliberately know nothing about Blank3D, Gamlib3D, VPhysics, Actor, ECS,
rendering or the operating system.

## Movement provider capabilities

A provider advertises a module mask and receives only enabled modules:

- `VEHICLEPROVIDER89_MODULE_CAR`
- `VEHICLEPROVIDER89_MODULE_TANK`
- `VEHICLEPROVIDER89_MODULE_WATER`
- `VEHICLEPROVIDER89_MODULE_AIR`
- `VEHICLEPROVIDER89_MODULE_SPACE`

Returning `VEHICLEPROVIDER89_HANDLED` suppresses the corresponding internal
movement subvendor for that frame. Returning `DECLINED` immediately restores
the internal fallback.

This means one vehicle may use an external CAR movement backend while another
continues using `tankmovement89`, `watermovement89`, etc.

## Physics provider phases

Physics delegation is phase-granular:

- `VEHICLEPROVIDER89_PHYS_GRAVITY`
- `VEHICLEPROVIDER89_PHYS_MASSPOINTS`
- `VEHICLEPROVIDER89_PHYS_LINEAR_DRAG`
- `VEHICLEPROVIDER89_PHYS_INTEGRATE`

A VPhysics adapter can therefore claim only integration first, while the
existing `vehiclephysics89` still supplies gravity/contact/drag. A later fuller
adapter may claim all phases. This avoids an all-or-nothing backend switch.

Vehicle-vs-vehicle pair collision resolution remains in the existing
`gveh_collision_bridge`/vehicle-world path in this phase; it has not been
silently double-routed into an external solver.

## Per-vehicle and world-default providers

Providers can be installed directly on one `gveh_vehicle`:

```c
vehicleprovider89_movement movement;
vehicleprovider89_physics physics;

vehicleprovider89_movement_clear(&movement);
movement.user = my_context;
movement.module_mask = VEHICLEPROVIDER89_MODULE_CAR;
movement.step = my_movement_step;
gveh_vehicle_set_movement_provider(&vehicle, &movement);

vehicleprovider89_physics_clear(&physics);
physics.user = my_context;
physics.phase_mask = VEHICLEPROVIDER89_PHYS_INTEGRATE;
physics.step = my_physics_step;
gveh_vehicle_set_physics_provider(&vehicle, &physics);
```

Or installed as defaults on a `gveh_vehicle_world`. Defaults are applied to
already-active vehicles and inherited by future spawns:

```c
gveh_vehicle_world_set_movement_provider(&world, &movement);
gveh_vehicle_world_set_physics_provider(&world, &physics);
```

Passing `0` clears either provider and restores the internal path.

## Blank3D bridge

`Blank3DMountVehicle` stores the provider contracts and reapplies them after
profile/reset rebuilds:

```c
blank3d_mount_vehicle_set_movement_provider(&vehicle, &movement);
blank3d_mount_vehicle_set_physics_provider(&vehicle, &physics);
```

Therefore hot respawn/profile replacement does not silently lose an external
backend.

## Intended host compositions

```text
A) standalone
carmovement89 + vehiclephysics89

B) VPhysics body backend
carmovement89 + VPhysics provider

C) Gamlib/custom movement backend
Gamlib movement provider + vehiclephysics89

D) fully external
Gamlib/custom movement provider + VPhysics provider
```

The provider seam is implemented now; no external backend is forced on by
default. That preserves current driving behavior and lets adapters be enabled
only where they are actually useful.
