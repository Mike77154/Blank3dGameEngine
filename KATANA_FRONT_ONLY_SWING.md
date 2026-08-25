# Katana front-only swing (v3.27.3)

## Problem

The giant katana had a frontal-looking socket, but the Mecanim pivot was only
0.80 m away from the player. A 1.26 m collision blade and the enlarged visual
mesh could rotate back through the actor body during the slash. Moving only
the hitbox would have desynchronized damage from presentation.

## Fix

The complete `player.melee_r` root is now positioned from the katana INI:

```ini
[mount]
mount_lateral_cm=0
mount_height_cm=105
mount_forward_cm=190
```

`mount_forward_cm` is positive in content and converted to player-local `-Z`
by the runner. The following all inherit the same frontal root:

- katana89 visual mesh;
- NationalMecanicanimal89 blade part;
- current and previous PDC3D attack OBBs;
- conservative swept collision.

No offset is added only to debug drawing or only to damage. Therefore the red
box remains attached to the actual visible blade.

## Safety regression

`tests/test_katana_melee.c` samples every rendered vertex through the animated
world matrix and also obtains the conservative AABB of every active OBB. Both
must stay at least 0.35 m in front of the player origin throughout the slash.

The system remains independent from `GWeapon89` and does not change firearms,
projectiles, NPC weapon logic, or camera ray authority.
