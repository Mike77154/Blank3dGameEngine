# Hand grenade weapon integration

Weapon 10 is a throwable hand grenade and is intentionally distinct from weapon 6, the 40 mm grenade launcher.

- `grenade_launcher`: `rocketmeshes:grenade40_lv`, ammo `grenades_40mm`, launcher muzzle.
- `hand_grenade`: `ghandgrenade3d89:pineapple_classic`, ammo `hand_grenades`, thrown physical projectile.

The vendored provider contains all 41 hand-grenade presets and three LODs. The initial runtime mesh uses `GHG3D_PRESET_PINEAPPLE_CLASSIC` at game LOD. It is normalized to Blank3D local +Z and remains a physical gravity/bounce projectile.
