# gvehicle89

C89 fixed-point vehicle sandbox library.

## Goals

- C89-friendly source.
- No dynamic allocation in the library.
- Fixed-point arithmetic only.
- Data-driven vehicle profiles.
- Sim-lite driving with arcade/Halo/Rally/GT/Wave style presets.

## Included modules

```txt
gvehicle89/
├─ include/gveh.h
├─ include/gveh_tag.h
├─ include/gveh_masspoint.h
├─ include/gveh_drive.h idea folded into drivetrain/core step
├─ include/gveh_susp.h
├─ include/gveh_contact.h idea represented by gveh_world probe callbacks
├─ include/gveh_camera.h
├─ include/gveh_ai.h
├─ include/gveh_fx.h
└─ include/gveh_debug.h
```

Additional modules:

```txt
├─ gveh_tire.h          tire force LUTs
├─ gveh_surface.h       terrain material grip/drag
├─ gveh_drivetrain.h    engine, gearbox, brake torque
├─ gveh_watercraft.h    buoy probes and water response
├─ gveh_world.h         engine-side collision/ground/water callbacks
└─ gveh_math.h          fixed-point helpers and vector math
```

## Build

```sh
make
./demo_vehicle89          # warthog-like
./demo_vehicle89 gt       # GT sport profile
./demo_vehicle89 rally    # rally profile
./demo_vehicle89 daytona  # arcade stock profile
./demo_vehicle89 wave     # jetski profile
./demo_vehicle89 tank     # tank-lite profile
```

The demo emits:

```txt
demo_vehicle89.csv
demo_vehicle89.ppm
```

## Optional providers

Movement and body physics can be delegated independently through the vendored
`vehicleprovider89` contract. Missing callbacks or callbacks that return
`VEHICLEPROVIDER89_DECLINED` use the original internal implementation.

This permits internal movement + external physics, external movement + internal
physics, fully external backends, or the unchanged fully internal stack.
World-level default provider setters also propagate to active vehicles and new
spawns.

## Integration

Use `gveh_profile_*` to create a profile, initialize a `gveh_vehicle`, then call `gveh_vehicle_step` at a fixed tick.

```c
gveh_runtime rt;
gveh_profile p;
gveh_vehicle v;
gveh_input in;

gveh_runtime_init(&rt);
gveh_profile_warthog89(&p);
gveh_vehicle_init(&v, &p, gveh_v3(0, gveh_fx_from_int(2), 0));
gveh_input_clear(&in);
in.throttle = GVEH_FX_ONE;
gveh_vehicle_step(&rt, &v, &in, GVEH_FX_ONE / 30);
```

Replace `gveh_demo_ground_probe`, `gveh_demo_sphere_probe`, and `gveh_demo_water_sample` with your engine callbacks.

## Profiles

- `warthog89`: masspoint-heavy, bouncy, four-wheel steer flavor.
- `gt_sport89`: tire/suspension heavy sim-lite.
- `rally89`: loose terrain and drift-friendly.
- `daytona89`: arcade drift and speed.
- `wave_jetski89`: buoy probes and water sampling.
- `tank_lite89`: heavy tracked vehicle placeholder.

## Notes

This is a first working core. The collision callbacks are deliberately engine-agnostic: plug in BSP, triangle mesh, heightfield, or capsules from your runtime.

## v2 magic extra modules

This package now includes Phase 1 and Phase 2 vehicle-magic modules as optional data-driven systems:

Phase 1:
- `gveh_nitro`: bottle charge, burn/refill, boost force.
- `gveh_kudos`: PGR-style score/combo for drift, air, clean driving and risk.
- `gveh_skill`: driver growth plus input smoothing/help.
- `gveh_tuning`: NFS-style stages for engine, nitro, tire, brake, suspension, weight and drift.
- `gveh_driver_profile`: Forza-style lightweight driver personality knobs.

Phase 2:
- `gveh_riskboost`: Burnout-style boost gained from drift, airtime and danger signals.
- `gveh_takedown`: impact/drop detection with rewards.
- `gveh_aftertouch`: post-crash steering force window.
- `gveh_crashbreaker`: charged post-crash impulse event.

All modules use static structs, caller-owned storage, fixed-point `gveh_fx`, and compile as C89.
The demo drives each profile while pulsing nitro, risk, aggression, aftertouch and crashbreaker inputs. CSV output includes kudos, combo, nitro, riskboost, takedowns, skill and crashbreaker charge.

v3 air modules
==============

This package adds optional airplane and helicopter modules inspired by classic arcade and simulation flight games.

New headers:
- include/gveh_airframe.h
- include/gveh_rotorcraft.h
- include/gveh_airassist.h
- include/gveh_airgame.h

New profile constructors:
- gveh_profile_thunder_chopper89(&p)
- gveh_profile_strike_chopper89(&p)
- gveh_profile_sim_heli89(&p)
- gveh_profile_ace_fighter89(&p)
- gveh_profile_fs_lightplane89(&p)

Demo args:
- ./demo_vehicle89 h
- ./demo_vehicle89 s
- ./demo_vehicle89 m
- ./demo_vehicle89 a
- ./demo_vehicle89 f

Added telemetry:
airspeed, air_lift, rotor_rpm, rotor_lift, fuel, cannon, missiles, lock, cargo.

## v4 air canon expansion

Added modules:

```txt
gveh_air_damage       engine/rotor/wing/tail/fuel component damage
gveh_avionics         radar, TADS, warning receiver, lock timer
gveh_wingman          simple wingman commands and squad score
gveh_air_mission      destroy/rescue/recon/protect objectives and medals
gveh_spacecraft       shields/lasers/engine energy and spacecraft handling
```

New demo profile shortcuts:

```sh
./demo_vehicle89 c   # comanche_voxel89
./demo_vehicle89 l   # longbow_campaign89
./demo_vehicle89 u   # gunship_flight89
./demo_vehicle89 x   # rogue_xwing89
./demo_vehicle89 i   # tie_interceptor89
./demo_vehicle89 b   # battlefront_bomber89
```

## v5 tank4 expansion

Added tank-sim modules inspired by classic and modern tank games:

```txt
gveh_tank_tracks       left/right track torque, pivot turn, track damage
gveh_tank_turret       independent turret yaw, gun elevation, stabilizer, recoil
gveh_tank_gun          ammo bins, reload, ready rack behavior, smoke rounds
gveh_tank_armor        front/side/rear/top armor, slope, spaced armor, ERA-lite
gveh_tank_modules      engine, transmission, ammo rack, fuel, turret ring, breech, optics, tracks
gveh_tank_crew         commander/gunner/loader/driver health, shock, bail state
gveh_tank_firecontrol  rangefinder, lead, optic zoom, thermal flag, accuracy
gveh_tank_station      driver/gunner/commander/external station and hatch state
gveh_tank_platoon      hold/advance/hulldown/fire/smoke/overwatch orders
gveh_tank4             facade that binds all tank subsystems
```

New profile constructors:

```c
gveh_profile_tank4_m1a1_89(&p);
gveh_profile_tank4_t72_89(&p);
gveh_profile_tank4_tiger_89(&p);
gveh_profile_tank4_sherman_89(&p);
gveh_profile_tank4_destroyer_89(&p);
gveh_profile_tank4_scout_89(&p);
```

New demo shortcuts:

```sh
./demo_vehicle89 1   # tank4_m1a1_89
./demo_vehicle89 2   # tank4_t72_89
./demo_vehicle89 3   # tank4_tiger_89
./demo_vehicle89 4   # tank4_sherman_89
./demo_vehicle89 5   # tank4_destroyer_89
./demo_vehicle89 6   # tank4_scout_89
```

New tank telemetry columns:

```txt
tank_left,tank_right,turret_yaw,turret_elev,tank_ammo,tank_fired,tank_pen,
tank_effective,tank_spall,crew_shock,crew_alive,module_last,platoon_order,platoon_score
```

## Vehicle System split — phase 1

`gveh_vehicle_step()` is now an orchestration facade rather than the home of every movement solver.
The deterministic movement pieces live under `vendor/`:

```txt
vendor/
├─ vehiclephysics89/
├─ carmovement89/
├─ motorcyclemovement89/   # specialization seam, not dispatched yet
├─ busmovement89/          # specialization seam, not dispatched yet
├─ tankmovement89/
├─ watermovement89/
├─ airmovement89/
├─ rotorcraftmovement89/
└─ spacemovement89/
```

The public `gveh_vehicle_step()` API is unchanged. Phase 1 was required to be behavior-preserving:
reference and split builds produce byte-identical `demo_vehicle89.csv` and
`demo_multi_vehicle89.csv` output for the same source baseline.

Gameplay modules (nitro, kudos, takedowns, avionics, missions, damage, tank crew/gun/fire-control)
remain in the facade/package for now; they are intentionally not mislabeled as movement physics.
