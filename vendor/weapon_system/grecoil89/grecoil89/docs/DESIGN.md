# grecoil89 design

## Why this is a separate solver

Recoil touches many subsystems:

- weapon fire event
- projectile/raycast aim
- player camera
- third-person weapon bone/socket
- first-person viewmodel
- HUD crosshair/spread
- audio/FX intensity
- AI/vehicle/turret firing logic

If recoil is written directly inside the weapon code, it becomes hard to reuse for enemies, turrets, vehicles, fixed cameras, over-the-shoulder cameras, FPS cameras, or sniper scopes.

`grecoil89` keeps the recoil state independent and returns plain offsets.

## Solver layers

```txt
fire event
   |
   +-- optional pattern step
   +-- optional random yaw/pitch
   +-- impulse into angle spring
   +-- impulse into weapon-position spring
   +-- spread bloom
   |
update tick
   |
   +-- damping while still inside recovery delay
   +-- spring pull once recovery delay ends
   +-- clamp max recoil
   +-- decay spread toward base
   |
sample
   |
   +-- aim output
   +-- camera output
   +-- weapon output
   +-- spread output
```

## Fixed-point format

The solver uses Q8.8 fixed-point.

```txt
1.0 = 256
0.5 = 128
2.0 = 512
```

Use these helpers:

```c
GREC_FP_FROM_INT(1)
GREC_FP_FROM_RATIO(75, 100)
grec_mul(a, b)
```

The fixed multiply avoids `long long`. Keep gameplay values modest. Recoil angles and weapon offsets are intentionally small.

## Fixed update recommendation

Best integration is one solver update per fixed gameplay tick, for example 60 Hz:

```txt
accumulator += frame_time
while accumulator >= fixed_tick:
    weapon/gameplay update
    recoil update
    accumulator -= fixed_tick
render interpolated scene
```

Do not call this with arbitrary float delta. The library is intentionally deterministic and fixed-point.

## Realism notes

Real recoil is not only camera shake. A usable model separates:

- impulse: immediate force of a shot
- torque / muzzle rise: pitch climb
- damping: shooter/weapon resistance
- recovery: returning aim/model to neutral
- spread: accuracy loss during repeated fire
- patterns: learned automatic-weapon spray behavior

The library provides knobs for each of those without pretending to be a full firearm simulator.
