# Blank3D v3.9.8 - weapon manifest section isolation

## Defect

`blank3d_weapon_ini_load_manifest()` previously treated every non-comment key
in `config/weapons/weapons.ini` as a profile path. After loading the nine
entries in `[weapons]`, it entered `[weapon.N]` catalog sections and attempted
to open metadata values such as `1`, `pistol` and `999` as files. The function
returned failure with a partially populated manager.

## Fix

- Track the current INI section.
- Accept profile paths only while inside `[weapons]`.
- Ignore catalog and capacity metadata in `[weapon.N]`.
- Update stale tests from removed `loadout.ini` to `weapons.ini`.

This preserves C89, fixed/static ownership and the existing fallback path.
