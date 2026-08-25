# GCROSSHAIR89 INI Recipe Runtime Integration

## Goal

Blank3D now uses the new `gcrosshair89` recipe/provider/runtime bundle for the normal gameplay crosshair instead of the previous hardcoded four-line HUD crosshair.

The scoped sniper HUD remains on the separate `gscope*` stack. This migration replaces the normal weapon crosshair path only.

## What changed

### Before

The weapon vendor tree contained the older ABI3 `gcrosshair_core89`, `gcrosshair_params89`, and `gcrosshair_base89` packages, but `blank3d_hud.c` did not use them for the normal crosshair. It drew four white lines directly with `b3d_draw_line()`.

That meant weapon crosshair selection and geometry were effectively hardcoded in the HUD.

### Now

The new bundle is vendored at:

```text
vendor/weapon_system/gcrosshair89/
  gcrosshair_core89/
  gcrosshair_params89/
  gcrosshair_base89/
  gcrosshair_provider89/
  gcrosshair_runtime89/
```

The live recipe catalog is copied to:

```text
config/crosshair/gcrosshair.ini
config/crosshair/lists/
config/crosshair/presets/
config/crosshair/parts/
config/crosshair/assets/
config/crosshair/examples/
```

The runtime path is:

```text
weapon INI
  crosshair_preset=<name>
        |
        v
config/crosshair/gcrosshair.ini
        |
        v
lists -> presets -> reusable parts
        |
        v
gcrosshair_runtime89
        |
        v
gcrosshair_provider89
        |------------------------------|
        v                              v
external HUD/vector providers     core89 fallback
        |                              |
        `--------------+---------------'
                       v
             Blank3D primitives
```

## Blank3D adapter

`src/blank3d_crosshair.c` and `src/blank3d_crosshair.h` wrap the generic runtime for Blank3D.

The adapter:

- loads `config/crosshair/gcrosshair.ini`;
- resolves presets by recipe name rather than a C enum;
- feeds AIM/FIRE/HIT state into `gcrosshair_params89`/runtime;
- advances crosshair animation with the real HUD `frame_ms` rather than a fixed frame tick;
- renders through `gcrosshair_provider89`;
- binds Blank3D line/dot/image callbacks as the primitive fallback;
- exposes the provider runtime so an external HUD/vector/asset provider can replace the fallback later;
- accepts `none`, `off`, or `disabled` as a weapon-level way to suppress the normal crosshair.

## Weapon INI selection

Each weapon can now choose its normal crosshair in `[modules]`:

```ini
crosshair_preset=pistol_crisp
```

Current assignments:

```text
pistol             -> pistol_crisp
machine_gun        -> smg_tracking
shotgun            -> shotgun_dynamic
magnum             -> precision_dot
sniper             -> sniper_hairline
 grenade_launcher  -> projectile_lead_circle
rocket_launcher    -> projectile_lead_broken
gatling            -> spray_control
slingshot           -> projectile_lead_circle
hand_grenade        -> aoe_wide_ring
```

Unknown names fall back to `blank3d_default` with a diagnostic message.

## Expansion without recompiling Blank3D

The vendored gcrosshair89 base remains 192 presets. Blank3D's active project root
adds two compatibility recipes (IDs 192 and 193) for the exact legacy pistol
first/third-person visuals, so `config/crosshair/gcrosshair.ini` now loads 194.

The vendor-style included mod example root:

```text
config/crosshair/examples/gcrosshair_modded.ini
```

includes the vendor base192 plus an external preset:

```text
id=192
name=mod_green_circle
```

A loader validation confirmed 193 presets and resolved preset 192 as `mod_green_circle` without any C change.

This is the intended extension model: add/rearrange INI lists, preset recipes, and reusable parts instead of extending switch statements or hardcoded HUD geometry.

## Build integration

The project Makefile now compiles the five gcrosshair89 components into Blank3D and adds:

```text
make test-crosshair-runtime
```

That test loads the actual project recipe root, verifies the stock 192-preset catalog, resolves the weapon-selected preset names, and confirms that rendering reaches the bound provider primitives.

## Validation

Validated with GCC in strict C89 mode:

```text
make syntax-check CC=gcc
PASS

make test-crosshair-runtime CC=gcc
PASS: 91 lines, 8 dots, 0 images

make test-weapon-ini CC=gcc
PASS: 10 weapon INI definitions

make test-weapon-modules CC=gcc
PASS
```

The repository's pre-existing `make test-core` failure at return code 42 was reproduced unchanged from the original uploaded ZIP before this migration, so it is not a regression introduced by gcrosshair89.

## Design result

The normal weapon reticle is no longer a special-case drawing fragment in `blank3d_hud.c`. It is now a data-driven recipe selected by the weapon and rendered through a provider-capable runtime, while preserving a built-in core/primitive fallback for plug-and-play use.
