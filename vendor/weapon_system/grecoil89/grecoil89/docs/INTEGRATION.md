# Integration notes

## First person

```txt
out.aim_angles     -> raycast/projectile direction
out.camera_angles  -> camera punch/additive view angle
out.weapon_angles  -> viewmodel rotation
out.weapon_offset  -> viewmodel translation
out.spread         -> crosshair bloom / projectile cone
```

## RE4 over-the-shoulder

```txt
out.aim_angles     -> laser/crosshair ray direction
out.camera_angles  -> very small shoulder punch
out.weapon_angles  -> character weapon socket additive rotation
out.weapon_offset  -> optional socket offset / animation layer
out.spread         -> reticle bloom or hidden accuracy penalty
```

## Classic fixed camera

```txt
out.aim_angles     -> actual muzzle/projectile direction
out.camera_angles  -> mostly ignored or converted into small screen shake
out.weapon_angles  -> weapon/torso additive pose
out.weapon_offset  -> weapon socket kick
out.spread         -> aim cone
```

## Enemy / NPC / turret

The solver does not care who owns it.

Give every firing entity one `GRecState`, usually stored inside your weapon component or actor component.

```txt
EnemyWeaponComponent
├─ current_weapon_id
├─ ammo
├─ cooldown
└─ GRecState recoil
```

## No ownership

The solver never owns memory.

- patterns are `const` static arrays
- profiles are `const` static structs
- state is caller-owned
- callbacks are caller-owned

## Frame update warning

This library expects a fixed gameplay tick. If your renderer runs variable FPS, keep recoil inside the fixed simulation step and sample after the last fixed tick.
