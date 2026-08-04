# Blank3D v3.14.0 — gautomotion89 AI movement flavor

`gautomotion89` is vendored under `vendor/gautomotion89/` without changing its
agnostic C89 API. It remains fixed-point Q16.16, caller-owned, provider-ready,
and independent from Blank3D, FPIL, rendering, physics, ECS and entity layouts.

Blank3D adds a narrow bridge:

```text
src/blank3d_automotion.h
src/blank3d_automotion.c
```

The bridge converts between Gamlib3D Q20.12 and gautomotion89 Q16.16 and keeps
one pattern state per NPC. Movement decisions remain in FPIL. Gravity, jumping,
flying and dives remain owned by the vertical stack.

## FPIL actions

Select and tune a reusable style:

```text
motionstyle=zigzag
motionamplitude=0.8
motionfrequency=1.5
motionradius=3
motionstopdistance=0.5
motionsafedistance=6
motionenabled=1
motionreset
```

Use the currently selected style:

```text
moveforepattern=4
moveforeflatpattern=4
```

Direct shortcuts:

```text
zigzagplayer=4
zigzagplayerflat=4
sineplayer=4
sineplayerflat=4
helixplayer=4
helixplayerflat=4
orbitplayer=3
orbitplayerflat=3
pingpong=1.25
automovetoplayer=4
moveawayplayer=4
```

`flat` variants preserve actor Y. This lets jump89, fly89 and airdiver89 own the
vertical axis while gautomotion89 adds variation only in X/Z.

For `pingpong=N`, N is the lateral amplitude. For `orbitplayer=N`, N is the
orbit radius. Frequency is controlled separately with `motionfrequency=N`.

## Existing AI examples

- `enemy.fpi`: sine-wave ground pursuit.
- `hopper_enemy.fpi`: zigzag horizontal travel while jump89 owns the arc.
- `dive_enemy.fpi`: sine-wave aerial alignment; dive and return remain airdiver89.
- `gunner_enemy.fpi`: subtle ping-pong sidestep while using its private weapons.

## Responsibility split

```text
FPIL decision
    ↓
gautomotion89 trajectory intention
    ↓
Blank3D transform bridge
    ↓
vertical stack / collision / physics host
```

The original provider API and sequence module are included unchanged for future
ECS, socket, physics-provider and scripted-sequence integrations.
