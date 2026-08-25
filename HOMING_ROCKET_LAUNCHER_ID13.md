# Homing Rocket Launcher — Weapon ID 13

`homing_rocket_launcher.ini` is a normal physical launcher recipe that uses
`Telesearcher89` without `Satellaborner89`. The missile is born at the weapon
muzzle and then reacquires an attackable Actor/GFaction target each projectile
update.

## Identity

- Weapon ID: 13
- Ammo ID: 12 (`homing_rockets`)
- Projectile ID: 13
- Projectile mesh ID: 12 (`rocketmeshes:RPG7_PG7VR`)
- Explosion radius: same 8-world-unit class used by the normal rocket launcher
- Starting test loadout: 1 launcher + 6 homing rockets

## DSL core

```ini
[modules]
satellaborner=0
telesearcher=1
telesearcher_reacquire=0
telesearcher_gain=0.28
telesearcher_offset_y=0.85
```

`reacquire=0` keeps the first valid target stable instead of choosing the nearest
hostile again every frame. If that target becomes invalid, the Blank3D provider
can still fall through to another attackable Actor/GFaction target. The vendor
remains unaware of Player, Zombie, factions, collision, rendering and damage.

The weapon intentionally reuses the existing rocket-launcher chassis for the
held model. Weapon identity and projectile identity are nevertheless separate.
