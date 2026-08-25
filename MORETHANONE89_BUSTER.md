# morethanone89 + Buster integration

## Purpose

`morethanone89` is a vendored, fixed-capacity C89 selector for Buster-style
charged projectile stages. It deliberately does not own input, charging,
ammunition, projectile pools, physics or rendering.

The host passes a hold duration in milliseconds. The library returns the
highest configured stage whose `min_ms` threshold has been reached.

## Runtime split

```text
DDSL2 shoot
  -> gtrigger89 PRESS_CHARGE_RELEASE
       PRESS   -> normal GWeapon shot immediately
       HOLD    -> accumulate elapsed time
       RELEASE -> morethanone89
                    no stage -> no second shot
                    stage N  -> synthesize one semi-auto release shot
                                  -> apply stage event overrides
                                  -> normal projectile stack
```

This preserves ordinary pistol-like tapping. A short tap fires exactly one
base projectile. Holding past `activation_ms` enables a charged release shot.

## Vendor

`vendor/weapon_system/morethanone89`

Constraints:
- C89 / gnu89 compatible.
- No malloc/calloc/realloc/free.
- No float/double.
- Fixed maximum of 8 stages.
- Q16 values for numeric projectile overrides.

Public operations:
- `mto89_profile_init`
- `mto89_set_stage`
- `mto89_validate`
- `mto89_resolve`

A stage may override:
- projectile id
- projectile mesh id
- damage
- speed
- radius
- mesh scale
- lifetime

Unspecified fields continue to come from the base GWeapon event/profile.

## Trigger model

`gtrigger89` gained `GTRIGGER89_MODEL_PRESS_CHARGE_RELEASE`.

Unlike the existing `CHARGE_RELEASE` model used by release-only weapons, this
model emits the ordinary press immediately and only reports charge metadata on
release. It does not autonomously fire on release; `morethanone89` must resolve
a valid stage before Blank3D authorizes the second shot.

## INI example

```ini
[modules]
trigger=press_charge_release
charge_time_ms=1500
more_than_one=1
player_physical_projectile=1

[more_than_one]
enabled=1
activation_ms=250
stages=3

[more_than_one_0]
min_ms=250
projectile_id=16
projectile_mesh_id=15
damage=12.0
projectile_speed=54.0

[more_than_one_1]
min_ms=650
projectile_id=17
projectile_mesh_id=16
damage=26.0
projectile_speed=50.0

[more_than_one_2]
min_ms=1150
projectile_id=18
projectile_mesh_id=17
damage=52.0
projectile_speed=46.0
```

## Buster test profile

`config/weapons/buster.ini` is weapon ID 15. It uses:
- projectile 15 / mesh 14 for the normal lemon;
- projectile 16 / mesh 15 for charge tier 1;
- projectile 17 / mesh 16 for charge tier 2;
- projectile 18 / mesh 17 for charge tier 3.

All four meshes are procedural Blank3D/Giffany Shapes3D placeholders. No
external game model or texture asset is bundled.

`player_physical_projectile=1` keeps the Buster on Blank3D's real physical
projectile path rather than the player camera-hitscan optimization. The
existing player projectile-aim adapter still converges the muzzle projectile
toward the camera-selected aim point.

## Tested behavior

The real `buster.ini`, parser, gtrigger89, morethanone89 and GWeapon manager are
used by `tests/test_buster_morethanone.c`:

- 100 ms: projectile 15 only.
- 300 ms: projectile 15 on press, projectile 16 on release.
- 700 ms: projectile 15 on press, projectile 17 on release.
- 1200 ms: projectile 15 on press, projectile 18 on release.
- after a charged release and ordinary cooldown, a short tap returns to one
  normal projectile 15.

The release event test also verifies mesh, damage, speed, radius, scale and
lifetime overrides from the INI.
