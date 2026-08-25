# gvehicle89 magic modules, Phase 1 and Phase 2

## Phase 1

### gveh_nitro
Reusable nitro bottle logic. The state stores charge, last force, active flag and cooldown. `gveh_nitro_step` returns a forward boost force, but it does not apply it by itself, so a game can route it to wheels, jet thrust or a scripted boost.

### gveh_kudos
PGR-like style scoring. It watches forward speed, side speed, grounded ratio and risk signals. It awards combo score for drifting, airtime, risk and clean driving.

### gveh_skill
Driver growth and assist layer. It can smooth steering at speed and accumulate driver, drift and stunt skill values.

### gveh_tuning
NFS-like staged setup. It mutates a `gveh_profile` before the vehicle is initialized: engine torque, max speed, nitro capacity/force, tire LUT peaks, braking, suspension and mass.

### gveh_driver_profile
Forza/Drivatar-lite behavior knobs. It is deterministic and cheap: aggression, brake bias, apex bias, smoothness, mistake rate and rubber-band value. The current demo uses it as an input modifier.

## Phase 2

### gveh_riskboost
Burnout-like boost pool. It refills from drift, airtime, risk signals and takedown rewards, then burns when nitro/boost is requested.

### gveh_takedown
Impact detector. It compares previous forward speed vs new forward speed and side aggression signals. It awards takedowns and can feed boost/nitro rewards.

### gveh_aftertouch
Temporary post-crash control window. Triggered by hard impact, then applies side/forward control forces for a few ticks.

### gveh_crashbreaker
Charged post-crash impulse. It accumulates from impacts/risk and fires an upward+forward impulse when requested and full.

## Integration notes

`gveh_vehicle` now owns state for these modules. `gveh_profile` owns the configs. `gveh_input` adds optional controls:

```c
nitro;
risk;
aggression;
crashbreak;
aftertouch_x;
aftertouch_z;
```

Old code that zeroes `gveh_input` through `gveh_input_clear` remains safe.
