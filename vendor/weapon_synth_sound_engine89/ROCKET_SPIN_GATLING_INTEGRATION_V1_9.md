# Rocket Spin + Gatling Integration — v1.9.0

## Rocket signal chain

```text
launch / booster
  gpaah89 rocket launcher report
  + wsoundpressure89 rocket pulse
  + muzzle expansion
  + short gfire89 launch plume
        ↓
trajectory
  grocketwhistle89 v1.2 air-space / dual EQ
  + grocketspin89 RPG-7 sustainer spin
  + discreet gfire89 torch/plume
        ↓
impact
  wsound_rocketblast89 heavy impact
  + rumble + crackle + debris fire
```

The launch and impact remain independent events. The flight layers follow the
projectile by `instance_key`, pan and motion updates.

## Gatling signal chain

```text
belt box / ready
        ↓
spin-up
  ggunmach89 motor
  + GGunTuberotator89 barrel-cutting texture
  + ggatlingwhistle89 spin-up tone
        ↓
fire loop
  80 GPAAH Gatling reports at 3000 rpm
  + belt feed
  + receiver excitation
  + brass casings
  + all three motor layers simultaneously underneath
        ↓
spin-down / belt movement
```

The motor bed is intentionally subordinate to the reports. Default showcase
levels put the firing peak about 14.5 dB above the spin-up bed.

## New events

```c
WSSE89_EVENT_ROCKET_SPIN_START
WSSE89_EVENT_ROCKET_SPIN_MOTION
WSSE89_EVENT_ROCKET_SPIN_STOP
WSSE89_EVENT_GATLING_START
WSSE89_EVENT_GATLING_FIRE_START
WSSE89_EVENT_GATLING_FIRE_STOP
WSSE89_EVENT_GATLING_STOP
```

## Runtime constraints

- C89 strict build.
- Integer/fixed-point synthesis.
- Caller-owned state and static pools.
- No malloc/calloc/realloc/free.
- No float/double/long long.
- Deterministic seeds and renders.
