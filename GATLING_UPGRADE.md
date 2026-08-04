# Gatling Gun upgrade

- Weapon id: `8`
- Ammo id: `7`
- Inventory ammo item: `107` (`gatling_belt`)
- Active belt: `300`
- Default reserve: `900`
- Fire mode: automatic
- Cadence: `35 ms`
- Gameplay spin-up: `360 ms`
- Projectile mesh id: `8`
- Audio:
  - M134D motor
  - medium six-tube rotor
  - Gatling whistle
  - continuous firing load
  - belt feed
  - heavy per-shot report
  - rifle-brass casings
  - spin-down tail

## Controls

- `7`: equip Gatling directly
- `N/M`: previous/next; Gatling sits after machine gun and before sniper
- Hold left mouse or `Space`: spin up, then fire continuously
- Release: stop feed and synthesize spin-down
- `R`: reload the persistent 300-round belt from GKInventory reserve

## Configuration

```toml
[weapon]
gatling_spinup_ms = 360

[inventory]
ammo_gatling = 900
```
