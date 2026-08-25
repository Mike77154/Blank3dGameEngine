# Satellaborner89 + Telesearcher89 weapon-system integration

Both libraries live under `vendor/weapon_system/` and are deliberately
provider-driven.

## Responsibilities

`Satellaborner89` resolves a hostile target position at fire time and replaces
the projectile spawn origin with `target_position + configured_offset`.
It never owns actors, collision, damage, rendering, or projectile storage.

`Telesearcher89` runs after a physical projectile exists. Every projectile
update it asks the host for the selected/reacquired target position and returns
a velocity steered toward it. It never moves world objects itself.

Blank3D supplies the provider using ActorSystem/GFaction truth. An already valid
hostile target is preferred; otherwise the host searches for an attackable
actor. This keeps the vendors agnostic to Player/Enemy/Ally classes.

## Weapon INI keys

Place in `[modules]`:

```ini
satellaborner=0
satellaborner_offset_x=0
satellaborner_offset_y=0
satellaborner_offset_z=0

telesearcher=0
telesearcher_reacquire=1
telesearcher_gain=1.0
telesearcher_offset_x=0
telesearcher_offset_y=0
telesearcher_offset_z=0
```

Aliases `spawn_at_target`, `spawn_target_offset_*`, `projectile_homing`,
`homing_reacquire`, `homing_gain`, and `homing_offset_*` are accepted.

If either module is enabled for a player weapon, Blank3D disables its normal
small-arms hitscan shortcut for that weapon so the feature receives a real
projectile lifetime.

## Compositions

Exact target birth:

```ini
satellaborner=1
satellaborner_offset_y=0
telesearcher=0
```

Orbital-style strike:

```ini
satellaborner=1
satellaborner_offset_y=12
telesearcher=1
telesearcher_reacquire=0
telesearcher_gain=1.0
telesearcher_offset_y=0.8
```

Muzzle-spawned homing projectile:

```ini
satellaborner=0
telesearcher=1
telesearcher_reacquire=1
telesearcher_gain=0.35
```

Everything is C89, fixed-point/integer only, and uses no heap allocation.
