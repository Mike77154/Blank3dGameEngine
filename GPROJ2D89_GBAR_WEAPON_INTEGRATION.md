# GProj2D89 -> Weapon System -> GBar integration

Blank3D vendors `GProj2D89 v1.0.1` at `vendor/weapon_system/gproj2d89/`.

Runtime authority remains `gweapon89`: the equipped weapon profile owns `ammo_id`,
clip capacity and loaded rounds. `blank3d_systems_ammo_id()` exposes only the
current ammo identity to the HUD. The HUD does not mutate weapon state.

Flow:

```
gweapon89 profile.ammo_id + clip state
        |
        v
blank3d_systems_ammo_id / blank3d_systems_clip
        |
        v
Blank3DGProjAmmoGBar bridge
        |              \
        | geometry      \ count/range remains telemetry
        v                v
GProj2D89            GBar89 segmented/vector units
        \                /
         \              /
          HUD RenderOps
```

`GBar89` now supports an optional caller-owned `GBar89_UnitRenderFn`. Existing
`unit_points` behavior is unchanged when no renderer is installed. For HUD nodes
whose name contains `ammo`, Blank3D installs the GProj renderer. GBar still owns
slot layout, fill/empty state, direction, padding, scaling and count. GProj owns
the cartridge/grenade/missile vector silhouette.

Current weapon ammo mapping:

- 1 pistol -> pistol
- 2 shotgun -> shotgun buckshot
- 3 magnum -> magnum
- 4 sniper -> sniper
- 5 grenade launcher -> 40 mm grenade
- 6 rocket launcher -> missile
- 7 gatling -> machine gun
- 9 hand grenade -> hand grenade
- 10 uzi -> machine gun
- 12 homing rocket -> missile
- unknown/non-ballistic ammo falls back visually to pistol without changing gameplay

The bridge is strict C89, fixed-point through GProj2D89, and uses caller/stack
bounded storage only; it performs no heap allocation.

## v3.27.5 BVHUD recipe authority + visual QA

The ammo preset no longer declares GBar89's built-in `unit_shape bullet`.
It now explicitly selects the external ammunition vector provider:

    vector_ammo_indicator true
    unit_renderer gproj_ammo

`Blank3DBigHudNode.unit_renderer_kind` carries this recipe decision into the
runtime. The old node-name `ammo` detection remains only as backwards
compatibility for older recipes.

Visual QA also exposed non-uniform X/Y scaling in the GProj->GBar adapter.
The bridge now preserves the source silhouette aspect ratio before centering it
inside each GBar unit slot. This makes shotgun shell + pellet geometry visibly
match the GProj2D89 catalog instead of looking like a stretched generic block.

Visual regression artifact:

    tests/gproj_bvhud_visual.svg
    tests/gproj_bvhud_visual.png

The visual test loads the real `ammo_cartridges_gold.bhud`, asserts that no
GBar built-in unit points are present, selects shotgun ammo through the bridge,
and renders the resulting GBar/GProj composition.
