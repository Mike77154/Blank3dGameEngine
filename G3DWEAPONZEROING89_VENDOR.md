# g3dweaponzeroing89 vendor

The crosshair/muzzle zeroing solver now lives in:

```
vendor/weapon_system/g3dweaponzeroing89/
├── include/g3dweaponzeroing89.h
└── src/g3dweaponzeroing89.c
```

Blank3D's `src/blank3d_ballistics.c` is only an adapter between `GWP89_Event`,
`Blank3DWeaponModules`, and the neutral vendor API.

## Pipeline

```
ga imquery89 / raycast -> target_point
camera + muzzle + target -> g3dweaponzeroing89
result.launch_direction -> physical projectile provider
```

## Maneuverable configuration

`G3DZ89_Config` exposes:

- `tps_zero_distance`
- `tps_muzzle_clearance`
- `min_flight_time`
- `max_flight_time`
- `ballistic_iterations`

The request chooses FPS/TPS, hit validity, speed, range, gravity and linear,
gravity or Bolt physics. The result reports the corrected target, launch vector,
gravity and diagnostic flags.

## Diagnostic flags

- `G3DZ89_RESULT_TARGET_REBUILT`
- `G3DZ89_RESULT_HEMISPHERE_REPAIRED`
- `G3DZ89_RESULT_BALLISTIC_COMPENSATED`

The vendor has no dependency on CamaraNaku or Blank3D state. It currently uses
Gamlib3D's fixed-point vector primitives as its math backend and performs no
allocation.


As of the Weapon Launch extraction, this solver is grouped under `vendor/weapon_system/` and is consumed by `gweaponlaunch89`.
