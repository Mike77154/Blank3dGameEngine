# Blank3D formal vertical axis and vendored capabilities — v3.11.0

## Architecture

```text
DDSL2 / FPIL / RPYL / engine update
                |
                v
      Blank3DVerticalAxis
  (policy, config, actor body state)
       /          |          \
      v           v           v
   fly89       jump89      airdiver89
      \           |           /
       +---- vmotion89 providers ----+
             |        |        |
            math   transform  physics(optional)
```

`src/blank3d_vertical_axis.*` is the only Blank3D-specific vertical coordinator.
It adapts Gamlib3D Q20.12 transforms to the provider ABI and owns engine policy:

- global simple gravity;
- per-body floor;
- `flying_entity` and `gravity_immune` flags;
- one actor-local `jump89_state`;
- one actor-local `airdiver89_state`.

The three vendors contain no Gamlib3D, DDSL2, FPIL, RPYL, Windows, OpenGL,
collision-library, or Blank3D includes.

## Dumb engine gravity

Every registered vertical body is ticked once per engine update. When gravity is
enabled and the body is neither jumping, flying, nor gravity-immune, Blank3D
moves it downward at a constant configured speed and clamps it to its floor.
There is intentionally no acceleration in this fallback gravity.

```toml
[gravity]
enabled = true
fall_speed = 9.0
jump_acceleration = 22.0
```

- `enabled=false`: disables global falling and prevents new `jump89` jumps;
- `fall_speed`: constant downward speed for ordinary bodies;
- `jump_acceleration`: downward acceleration used by active jump states.

A physics provider may replace floor, grounded, and position-resolution behavior
without replacing the vertical coordinator or any vendor.

## Entity policy

Scene-side actor flags:

```text
player floor 0
player flyingentity 1
player gravityimmune 0

hopper_enemy 3 pos 0 0 34 hp 30 floor 0

dive_enemy 4 pos 0 8 12 hp 30 floor 0 flyingentity 1
```

A body with `flyingentity=1` is immune to the engine's automatic gravity but can
still use explicit `fly89` or `airdiver89` operations. A normal body is pulled to
its floor. `gravityimmune=1` provides an additional policy escape hatch.

## FPIL surface

### fly89 route

```text
moveup=N
movedown=N
```

### jump89 route

```text
jump=N
jumpstart=N
leap=N
```

Conditions:

```text
grounded
airborne
jumping
falling
flyingentity
heightbelow=N
heightatleast=N
```

### airdiver89 route

```text
saveposition
savehome
descendtoy=Y
airdescendtoy=Y
diveplayer=N
ramplayer=N
returntoposition=N
returnhome=N
```

`diveplayer`/`ramplayer` receives the target point from the host. Damage and
state transitions remain AI/engine responsibilities rather than vendor logic.

## Provider contract

The host gives each vendor an opaque `void *actor` plus:

```text
math provider
  scalar format, arithmetic, length and normalization

transform provider
  get_position(actor)
  set_position(actor)

physics provider (optional)
  get_floor_y(actor)
  is_grounded(actor)
  move_position(actor, desired, floor policy) -> resolved
```

Blank3D's built-in adapter uses Gamlib3D transforms. An external engine can use
ECS component handles, rigid bodies, character controllers, or another fixed
point format without changing the three libraries.

## Sample behaviors

The hopper now starts a `jump89` impulse and uses flat X/Z pursuit while the
jump is active. Gravity supplies the descending half of the arc. It stops
horizontal advance when near the player, lands, and does not jump again until
the player leaves the radius.

The dive enemy remains a flying body. `airdiver89` saves its aerial perch, rams
the player target, then returns to the stored point. Its AI decides when to
align, attack, damage, return, and repeat.
