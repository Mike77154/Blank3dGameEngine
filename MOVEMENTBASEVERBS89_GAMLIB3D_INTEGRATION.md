# 3d_movementbaseverbs89 + Gamlib3D integration

## Why the bridge exists

`3d_movementbaseverbs89` is intentionally engine-neutral. Its core uses Q16.16
and expresses rotation amounts as Q16.16 turns. Blank3D already uses Gamlib3D,
whose transform/math core uses Q20.12 and stores rotations as Q20.12 degrees.
The integration therefore uses an explicit optional provider adapter instead of
making either library pretend the two fixed-point domains are identical.

## Gamlib3D surface actually present in this package

The vendored Gamlib3D tree currently contains:

- `gamlib3d_transform`
- `gamlib3d_camera`
- `gamlib3d_scalar`
- `math_helpers/gamlib3d_math`
- `math_helpers/gamlib3d_matrix`

Its README mentions historical `gamlib3d_movement`, `gamlib3d_rotation` and
`gamlib3d_scale` modules, but those source files are not present in this
Blank3D package. The movement-verb adapter therefore correctly targets the
available transform API directly:

- `transform_move_local_flat()`
- `transform_move_local()`
- `transform_rotate()`

## Provider boundary

```text
DDSL2 / host / AI / editor
          |
          v
3d_movementbaseverbs89
  game verbs + base verbs
          |
          +---- game provider ----> Blank3D policy
          |                          - fly89/jump89 Y ownership
          |                          - Cameranaku turn sync
          |
          `---- base provider ----> Gamlib3D adapter
                                     |
                                     v
                                  Transform
```

The adapter is optional and lives under:

```text
vendor/3d_movementbaseverbs89/adapters/gamlib3d/
```

The movement library core does not include Gamlib3D headers.

## Numeric bridge

Linear amounts:

```text
movementbaseverbs Q16.16 -> Gamlib3D Q20.12
```

Rotation amounts:

```text
movementbaseverbs Q16.16 turns -> Gamlib3D Q20.12 degrees
```

No float or double conversion is used.

## Coordinate convention

Gamlib3D defines local forward as `-Z`. The adapter calls Gamlib3D's own local
movement functions instead of editing X/Y/Z directly, so the provider follows
Gamlib3D's current handedness and yaw/pitch/roll convention automatically.

Blank3D installs the adapter in `MBV89_GAMLIB3D_MOVE_FLAT` mode for player
walking/strafe behavior, matching the previous use of
`transform_move_local_flat()`.

Full 6DOF remains available for other engines/actors by changing the adapter
mode to `MBV89_GAMLIB3D_MOVE_6DOF`.

## Gameplay ownership preserved

The semantic game-provider intercepts only behavior that already belongs to
other Blank3D systems:

- `fly_up`, `fly_down` -> vertical axis / fly89; active jump still owns Y
- `turn_left`, `turn_right` -> Cameranaku + player yaw synchronization

Other game verbs fall through to the base provider:

- `walk_forward`, `walk_backward`
- `strafe_left`, `strafe_right`
- `run_forward`, `run_backward`

The old DDSL2 names remain valid. `move_up`/`move_down` stay semantic flight
aliases in Blank3D for compatibility instead of becoming raw Y edits.

## Text vocabulary

The library now owns parsing of its own names and common compatibility aliases,
including:

```text
move_forward / move_foward
move_backward / move_back
walk_backward / walk_back
run_backward / run_back
rotate_backward / rotate_back
```

That keeps the movement vocabulary in the movement library instead of copying
another name table into the engine.
