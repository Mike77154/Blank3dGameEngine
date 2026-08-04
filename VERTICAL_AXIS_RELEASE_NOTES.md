# Blank3D v3.11.1 release notes

## Player dual-capability validation

The sample player now drives `jump89` with `J` and `fly89` with `F/G`.  Jump
state has Y-axis priority while active, and the regression suite verifies that a
body marked `flying_entity` can still perform and finish a grounded gravity arc.
Active reload moved to `H`.

## Formal Y-axis flow

The direct vertical prototype has been replaced by one engine-owned vertical
coordinator: `Blank3DVerticalAxis`. Player and NPC Y operations, simple gravity,
jump state, floor policy, flying immunity, and aerial return state now pass
through that layer.

## Engine gravity

`config/blank3d.toml` now controls a deliberately simple constant downward
motion:

```toml
[gravity]
enabled = true
fall_speed = 9.0
jump_acceleration = 22.0
```

Ordinary registered actors are moved toward their floor every update. Flying or
gravity-immune bodies are skipped. Active jumps use acceleration and finish by
clamping and resetting at the floor.

## Independent vendors

- `fly89`: explicit gravity-free vertical movement;
- `jump89`: reusable grounded jump and gravity arc;
- `airdiver89`: descend, ram, save position, and return;
- `vertical_motion89_common`: shared math/transform/physics provider ABI.

Each capability compiles independently as a C89 static library and contains no
Blank3D or Gamlib3D dependency.

## Sample migration

- Player flight uses `fly89` through DDSL2 and is marked `flyingentity=1`.
- Hopper leap uses `jump89`; scripted downward movement was removed.
- Dive enemy uses `airdiver89` for its perch, ram, and return cycle.
- Existing `moveup`, `movedown`, `saveposition`, `diveplayer`, and
  `returntoposition` script commands remain compatible through the new flow.
