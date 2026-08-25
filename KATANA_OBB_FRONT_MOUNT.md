# Katana OBB + front-centred test mount (v3.27.1)

## Collision

The offensive katana volume is now a real `HB3_SHAPE_OBB` created through
`ml3_attack_set_hit_obb()`. It follows the blade world matrix and keeps the
previous OBB so `hitbox3d` can perform its conservative swept-OBB check.

The box is active only inside the configured Mecanim damage window. Enemy
hurt volumes remain independent defensive shapes.

```ini
[physical_damage]
blade_base_cm=4
blade_tip_cm=68
damage_box_half_width_cm=8
damage_box_half_thickness_cm=6
```

`damage_radius_cm` is retained as a backward-compatible alias for old INIs.

## Presentation

The independent `player.melee_r` test carrier is now centred approximately
1.90 m in front of the player and 1.05 m above the actor root. The mount is now loaded from `config/melee/katana.ini`; this is a
debug/test placement and remains outside `GWeapon89`.

## Controls

- `K`: play slash.
- `F3`: show/hide the red OBB and green enemy hurt volumes.
