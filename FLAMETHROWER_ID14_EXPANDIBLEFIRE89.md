# Blank3D Weapon ID 14: Flamethrower + expandiblefire89

## Goal

ID 14 proves that the existing Weapon System can express a flamethrower without
adding a dedicated flamethrower subsystem. The weapon emits ordinary physical
projectiles. Each projectile is a very small procedural cube whose gameplay
size expands with actual travelled distance and is killed at a configured
range.

The cube is the mechanical anchor. A future billboard, animated texture,
particle sprite or shader may be attached to that projectile without becoming
the authority for position, range or collision size.

## Vendor boundary

`vendor/weapon_system/expandiblefire89/`

The library owns only:

- caller-owned per-projectile growth state;
- accumulated travelled distance;
- linear start/end scale;
- growth distance;
- kill distance;
- fixed-point scale application helper.

It does not own weapons, actors, pools, rendering, collision, damage, audio,
input or memory allocation.

Protocol: signed Q20.12 (`4096 == 1.0`).

## ID 14 recipe

`config/weapons/flamethrower.ini`

Key values:

```ini
[weapon]
id=14
ammo_id=13
projectile_id=14
projectile_mesh_id=13
fire_mode=auto
cooldown_ms=45
projectile_speed=8.5
projectile_life_ms=1200
damage=4.0
spread=5.5
projectile_radius=0.20
projectile_mesh_scale=0.42

[modules]
expandible_fire=1
expandible_fire_scale_start=0.25
expandible_fire_scale_end=2.40
expandible_fire_growth_distance=5.20
expandible_fire_kill_distance=6.25
expandible_fire_collision_growth=1

projectile_visual=expandible_fire
projectile_visual_unlit=1
projectile_visual_additive=1
```

At birth the effective mesh scale is about `0.105` and collision radius about
`0.05`. At full growth the mesh scale is about `1.008` and collision radius
about `0.48`. At 6.25 travelled world units the projectile is released even if
its backup lifetime has not expired.

## Runtime ownership

```text
GWeapon fire event
      |
      v
gprojectilespawn89
      |
      v
Bullet pool  ---- base mesh/radius
      |
      v
expandiblefire89
  distance travelled
  scale multiplier
      |
      +--> mesh scale
      +--> collision radius
      +--> kill by distance
      |
      v
existing collision/damage/render/lifecycle
```

The player hitscan shortcut explicitly refuses recipes with
`expandible_fire=1`, so ID 14 always retains a real projectile lifetime.

## Mesh

Projectile mesh 13 is a single `Giffany Shapes3D` box. It is intentionally
low-poly and immutable; growth is applied in the projectile pose/runtime, not
by rebuilding geometry every frame.

## Future presentation

Billboards/textures may later consume the same projectile transform and growth
factor. The primitive/collision behaviour does not need to change.
