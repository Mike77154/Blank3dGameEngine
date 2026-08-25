# Weapon infinite-ammo flag + Buster report reuse

## Goal

Allow individual weapon profiles to opt out of ammunition economy without
hard-coding a weapon ID, while preserving fire mode, cooldown, projectile,
morethanone89 charge stages, and all normal Weapon System providers.

## INI

In `[weapon]`:

```ini
infinite_ammo=1
```

Accepted aliases in the profile parsers are `infinite_ammo`, `ammo_infinite`,
and `infinite`.

## Manager semantics

`GWP89_WeaponProfile.infinite_ammo` defaults to 0. The manager also exposes
`GWP89_FLAG_INFINITE_AMMO`, so a flag provider may override the profile per
actor/weapon.

When the resolved flag is true:

- ammo availability never blocks an accepted shot;
- internal clip is not decremented;
- reserve ammo is not consumed;
- no AMMO_CHANGED event is emitted because no ammunition changed;
- reload requests return OK as a no-op;
- cooldown, trigger/fire mode, recoil, projectile emission, morethanone89,
  collision and presentation remain unchanged.

The Buster profile enables the flag. Ordinary pistol remains finite; the
regression test explicitly exhausts its two-round test clip and requires the
third shot to return NO_AMMO.

## Pistol fire sound

The common pistol now reuses the Buster's synthesized firing-report DNA:

- profile: `smg`
- action: `pistol`
- muzzle: `bare`
- pressure_energy_q15: `18000`
- fire_gain_q15: `25500`
- action_speed: `1.2`

The pistol keeps its own magazine, ammo-motion, casing and reload layers. This
means only the firing report is intentionally matched; normal pistol mechanical
foley is preserved.

## QA

`make test-infinite-weapon-ammo` verifies:

1. Buster fires repeatedly with reserve=0 and does not decrement clip.
2. Buster reload is a no-op.
3. Pistol remains finite.
4. Pistol and Buster share the fire-critical synthesized audio parameters.
