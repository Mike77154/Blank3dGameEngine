# Katana Mecanim + Physical Damage prototype — v3.27.1

This experiment intentionally lives outside `GWeapon89` and every ballistic
path. The katana is a general animated scene object whose animation owns the
damage timing.

```text
player.melee_r (Soquete3D carrier)
        |
        v
katana89 procedural mesh
        |
        v
NationalMecanicanimal89 clip
        |
        +-- marker: active begin
        |       |
        |       v
        |   swept blade OBB ON
        |       |
        |       v
        |   enemy vulnerable hurt volumes
        |       |
        |       v
        |   damage event -> Blank3D actor damage
        |
        +-- marker: active end -> blade OBB OFF
```

## Runtime controls

- `K`: trigger the katana slash clip.
- `F3`: show or hide the physical collision volumes.
  - red: offensive blade hit OBB;
  - green: enemy vulnerable hurt capsules.

The katana remains a visible test object, centred directly in front of the player, even while firearms are equipped,
because this build is validating an independent object-animation/damage
domain rather than adding it to the weapon inventory.

## Data-driven active window

`config/melee/katana.ini` controls both the clip and the collision window.

```ini
[mecanim]
tick_ms=16
duration_frames=28
active_start_frame=7
active_end_frame=14

; Optional time form. Non-zero millisecond values override frame values.
active_start_ms=0
active_end_ms=0
```

The physical volume does not exist during wind-up or recovery. The interval is
`[active_start_frame, active_end_frame)`: the end value is exclusive. It is
created at the active-begin marker, follows the animated blade every simulation
tick, and is cleared after the final active tick.

## Terminology at the collision boundary

The visual request is implemented as the expected melee interaction:

- katana: offensive **hit OBB**;
- enemy: vulnerable **hurt capsule**.

The two overlap to generate a damage event. The distinction keeps the same
object from being both attacker and receiver and matches the terminology used
inside the supplied `PhysicalDamageCollision3D` collision kernel.

## Swept collision

The blade OBB is rebuilt from the animated world matrix of the katana:

```text
blade midpoint -> animated world position -> OBB centre
blade X/Y/Z axes -> animated world matrix -> OBB basis
blade length/width/thickness -> OBB half extents
```

`hitbox3d` preserves the previous and current OBB. Its conservative swept-OBB
query merges both poses, so a fast slash does not depend only on one
instantaneous box. The one-hit log prevents a box that overlaps an enemy for
several active frames from applying damage repeatedly during one swing.

## Isolation from firearms and NPCs

This module does not call or modify:

- `GWeapon89`;
- player camera ray authority;
- muzzle zeroing;
- projectile simulation;
- NPC weapon managers.

Only hostile enemy actors are registered as vulnerable targets in this first
prototype. Allies are skipped.

## Important files

```text
src/blank3d_katana_mesh.c/.h
src/blank3d_katana_melee.c/.h
config/melee/katana.ini
tests/test_katana_melee.c
vendor/katana89/
vendor/physical_damage_collision3d/
```

The active integration uses the focused `hurtbox3d + hitbox3d + melee3d`
kernel from the supplied PhysicalDamageCollision3D package. This avoids
pulling unrelated grab/throw/defense domains into the experiment.

## Validation

```text
make test-katana-melee
make syntax-check
make audit
```

The portable test verifies:

1. katana89 produces a non-empty renderer mesh;
2. NationalMecanicanimal89 runs the slash clip;
3. the damage volume is frame gated;
4. an enemy receives the configured damage;
5. one swing damages one enemy only once;
6. the animation returns to idle;
7. no allocator call is introduced into active gameplay code.

The non-Windows environment can parse the full runner but cannot produce or
execute the final MinGW32/OpenGL window. Visual placement and feel therefore
still need the MSYS2 Win32 run.
