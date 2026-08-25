# ShangoWeapon — INI-authored orbital strike

This branch adds `weapon_id=12`, `config/weapons/shango_weapon.ini`. The weapon is deliberately composed from existing Weapon System providers instead of a monolithic Shango implementation.

## Composition

- `Satellaborner89`: resolves a hostile target and relocates projectile spawn to target + `(0,12,0)`.
- `Telesearcher89`: keeps the physical projectile steering toward the target torso.
- `projectile_mesh_id=11`: procedural Shapes3D energy cylinder, no external asset.
- `projectile_visual=shango_press`: generic INI presentation mode. It expands radially for `projectile_visual_expand_ms`, then compresses axially for `projectile_visual_crush_ms`.
- `projectile_visual_unlit=1` + `projectile_visual_additive=1`: the host renders the energy mesh as a luminous unlit two-pass additive column.

## Core DSL recipe

```ini
[modules]
satellaborner=1
satellaborner_offset_y=12
telesearcher=1
telesearcher_reacquire=0
telesearcher_gain=1.0
projectile_visual=shango_press
projectile_visual_expand_ms=180
projectile_visual_crush_ms=520
projectile_visual_radial_start=0.30
projectile_visual_radial_peak=2.35
projectile_visual_radial_end=1.05
projectile_visual_axial_start=6.50
projectile_visual_axial_end=0.28
projectile_visual_unlit=1
projectile_visual_additive=1
```

The visual recipe is not keyed to weapon 12. Any future projectile weapon can reuse `projectile_visual=shango_press`.

## Test-build loadout

`player_weapons.ini` grants weapon 12 and six `shango_cells` in this demo branch so the effect can be tested immediately. Remove `weapon_12=1` and `ammo_11=6` to restore non-default possession.
