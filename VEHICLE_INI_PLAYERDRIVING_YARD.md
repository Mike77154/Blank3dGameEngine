# Blank3D Vehicle INI + PlayerDriving Yard

## Purpose

This pass turns the single white-box vehicle prototype into an authored vehicle yard and separates vehicle identity from driver authority.

A vehicle INI describes the vehicle itself. It does not mean "the Player owns this vehicle". The same recipe can advertise Player and NPC driving eligibility while Mount89/gvehpos89 continue to own seat occupancy and gvehicle89 continues to own vehicle simulation/provider dispatch.

## Runtime chain

```text
vehicle INI
  profile / movement / tune / visual / driver policy
                    |
                    v
          Blank3DVehicleSystem
                    |
          +---------+---------+
          |                   |
      Player driver        NPC policy
          |                   |
 driver_ddsl (while mounted)  future AI command source
          |
        Game Verbs
          |
       gvehpos89
          |
        Mount89
          |
       gvehicle89
          |
 movement provider/fallback + physics provider/fallback
```

Mount89 remains spatial occupancy. GAttach is not used for vehicle occupants.

## RPYL yard

`scripts/startup.rpy` now creates three independent vehicles:

```rpy
vehicle_ini "config/vehicles/test_car.ini" pos 5 2.5 7
vehicle_ini "config/vehicles/test_motorcycle.ini" pos -5 2.3 8
vehicle_ini "config/vehicles/test_tank.ini" pos 14 1.0 9
```

The current visuals deliberately remain boxes so simulation and authoring can be tested before model presentation is attached:

- car: normal box
- motorcycle: narrow box
- tank: large box

`M` mounts/dismounts the nearest eligible vehicle.

## General vehicle recipe

Common fields supported by the new INI layer include:

```ini
[vehicle]
name=test_car
profile=warthog89
movement=car
playerdriving=1
npcdriving=1
driver_ddsl=scripts/vehicle_driver.ddsl2

[driving]
drive_speed=24
run_speed=30
mount_radius=4.0
throttle_rise=2.8
throttle_fall=4.5
invert_steer=1
torque_scale=8.0
reverse_scale=1.65
velocity_retention=0.985
mass_scale=0.75

[visual]
scale_x=1.0
scale_y=0.85
scale_z=1.0
visual_y_offset=-1.90
```

`movement` currently recognizes car, motorcycle, bus, tank, water, air and space seams. Motorcycle and bus are Phase-1 specialization seams: motorcyclemovement89/busmovement89 currently reuse the wheeled baseline where appropriate; true motorcycle lean/counter-steer and articulated bus behavior are not claimed yet.

## PlayerDriving DDSL authority

Each recipe may name its driver DDSL. The currently selected vehicle driver's script is loaded while the Player occupies its driver seat and participates in normal hot reload.

The stock test script is:

```ddsl2
If vehicle_playerdriving and key_hold Up then vehicle_forward
If vehicle_playerdriving and key_hold Shift and key_hold Up then vehicle_run_forward
If vehicle_playerdriving and key_hold Down then vehicle_reverse
If vehicle_playerdriving and key_hold Shift and key_hold Down then vehicle_run_reverse
If vehicle_playerdriving and key_hold Left then vehicle_steer_left
If vehicle_playerdriving and key_hold Right then vehicle_steer_right
```

While mounted, ordinary Player locomotion is suppressed by the movement-provider boundary, so the Player does not walk independently inside the seat. When dismounted, `vehicle_playerdriving` is false and the vehicle rules are inert.

This makes the driven vehicle temporarily consume Player-style Game Verbs without turning the vehicle into an Actor or conflating it with Player state.

## NPC driving policy

`npcdriving=1` is now an authored eligibility/policy field and is carried into seat policy. It intentionally does not hardcode an NPC AI into the vehicle.

A future NPC driver should feed the same vehicle command boundary from FPIL/AI while gvehpos89 + Mount89 establish occupancy. The current QA does not claim autonomous NPC boarding/driving yet.

## Acceleration and reverse fix

The slow-car report exposed a deeper issue than engine torque: gveh body integration retained only the legacy velocity fraction each frame, giving a low terminal speed even when torque was increased.

The physical profile now has an authorable `velocity_retention` value. The old retention remains the default, so old profiles and provider-free simulations remain behavior-compatible. The test car chooses a higher value (`0.985`) so acceleration is visible and sustained.

The INI also controls fixed-point throttle ramping:

- `throttle_rise`: rate at which pedal request grows while held
- `throttle_fall`: rate at which it releases
- `drive_speed` / `run_speed`: normal/full-throttle target policy
- `reverse_scale`: reverse tuning

The regression requires late forward speed to exceed early forward speed and exceed the 7 world-unit/s Player baseline. Reverse must enter real gear 0, grow in magnitude, and exceed 6 world-unit/s in the long hold test.

## Steering fix

Blank3D/Gamlib and gvehicle have different coordinate conventions. The existing yaw bridge remains intact, while each vehicle recipe can select:

```ini
invert_steer=1
```

The regression does not merely check the request sign. It records the vehicle's local RIGHT axis, drives forward while requesting RIGHT, and requires the physical displacement to project positively onto that axis.

## Mount radius fix

`mount_radius` is now passed to the actual gvehpos89 seat entry radius. Previously Blank3D's preliminary proximity check used the authored radius while gvehpos89 still used a fixed 3-unit seat radius, which could make a tall vehicle appear mountable and then reject the actual mount.

## Provider compatibility

The existing optional-provider contract is preserved:

```text
movement provider HANDLED -> skip internal *movement89 module
movement provider DECLINED -> internal movement fallback

physics provider HANDLED -> skip internal vehiclephysics89 phase
physics provider DECLINED -> internal physics fallback
```

`velocity_retention` belongs only to the internal integration fallback. If an external physics provider handles INTEGRATE, the internal retention is not applied.

## Current test vehicles

### Car
- `warthog89`
- `carmovement89`
- progressive throttle/reverse
- corrected physical steering
- tuned velocity retention

### Motorcycle
- `rally89`
- `motorcyclemovement89` seam
- narrow placeholder box
- independent mount/drive path proven
- no true lean/counter-steer yet

### Tank
- `tank4_m1a1_89`
- `tankmovement89`
- large placeholder box
- independent mount/drive path proven
- turret/weapon operation remains separate from movement and belongs to Mount89 + Weapon System composition later

## Architectural rule

```text
Vehicle INI       = what the vehicle is / what driving policies it allows
DDSL2 / AI        = who is issuing driving verbs right now
gvehpos89         = seat/driver possession rules
Mount89           = guest -> mount point spatial relationship
gvehicle89        = vehicle orchestration
*movement89       = specialized movement fallback
vehiclephysics89  = physical fallback
external providers= optional replacement by capability
```

No vehicle recipe owns the Player or an NPC.
