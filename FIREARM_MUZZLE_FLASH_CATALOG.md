# Blank3D firearm muzzle-flash catalog

This build temporarily shares the same native progressive JPEG muzzle asset across the conventional firearms while keeping every recipe independent.

Shared asset:

`config/weapons/assets/pistol_muzzle_flash.jpg`

The path is intentionally stored in each weapon INI rather than hard-coded. Replacing one weapon later only requires changing that weapon's `muzzle_image=` entry and, if desired, retuning its width, height, lifetime, glow and dynamic-light pulse.

## Current tuning

| Weapon | Image WxH | Image ms | Glow scale / alpha | Light ms | Intensity | Radius | Presentation muzzle Y / Z |
|---|---:|---:|---:|---:|---:|---:|---:|
| pistol | 1.10 x 1.10 | 58 | 1.55 / 160 | 72 | 3.25 | 9.0 | 0.02 / -0.86 |
| machine_gun | 1.00 x 0.92 | 42 | 1.45 / 125 | 50 | 2.75 | 8.5 | 0.015 / -1.16 |
| shotgun | 1.60 x 1.45 | 68 | 1.65 / 180 | 82 | 4.75 | 13.0 | 0.03 / -1.40 |
| magnum | 1.38 x 1.30 | 62 | 1.65 / 176 | 84 | 4.40 | 11.5 | 0.025 / -1.03 |
| sniper | 1.28 x 1.18 | 64 | 1.62 / 172 | 92 | 5.10 | 15.5 | 0.02 / -1.62 |
| gatling | 0.92 x 0.84 | 30 | 1.40 / 112 | 38 | 2.35 | 7.5 | 0.01 / -1.54 |
| uzi | 0.88 x 0.82 | 40 | 1.40 / 115 | 48 | 2.40 | 7.5 | 0.02 / -1.05 |

The three automatic weapons deliberately use shorter image lifetimes so repeated fire remains a sequence of flashes instead of a permanently illuminated billboard. Gatling's 30 ms muzzle image is shorter than its current 35 ms shot interval.

## Replacement workflow

For a future per-weapon muzzle image, edit only the target INI:

```ini
[modules]
muzzle_image=config/weapons/assets/my_new_flash.jpg
muzzle_image_width=1.10
muzzle_image_height=1.10
muzzle_image_ms=58
muzzle_image_billboard=view
muzzle_image_blend=additive
muzzle_image_glow=1
muzzle_image_glow_scale=1.55
muzzle_image_glow_alpha=160
muzzle_light=1
muzzle_light_ms=72
muzzle_light_intensity=3.25
muzzle_light_radius=9.0
muzzle_light_r=255
muzzle_light_g=205
muzzle_light_b=112
```

Position remains owned by `[presentation]` through `muzzle_y` and `muzzle_z`, so a texture replacement does not require code changes.

## Boundary

This pass intentionally covers pistol, machine gun, shotgun, magnum, sniper, gatling and Uzi. Grenade/rocket launchers, Shango, flamethrower, slingshot, hand grenade and Buster use different visual semantics and were left unchanged rather than forcing a firearm starburst onto them.
