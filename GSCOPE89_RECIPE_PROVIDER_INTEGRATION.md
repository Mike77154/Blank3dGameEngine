# gscope89 recipe/provider integration

Blank3D now uses `vendor/weapon_system/gscope89` as the sole sniper-scope stack.
The previous individually vendored `gscope*`, `gsniperhud89`, `gsway89`,
`gaimquery89` and telescopic zoom copies were removed after migration.

## Runtime chain

```text
config/weapons/sniper.ini
  -> scope_recipe=config/scope/hud/sniper_re5_psg1.ini
     -> providers/plug_and_play.ini
     -> components/sniper_mask.ini
     -> components/sniper_telemetry.ini
     -> catalogs/scopes.ini -> presets/scopes/re5_psg1.ini
        -> vector_reticle=re5_psg1_game_scope
           -> reticles/catalog.ini
              -> reticles/presets/062_re5_psg1_game_scope.ini
                 -> geometry components/fragments
     -> catalogs/zoom.ini -> zoom preset
     -> catalogs/sway.ini -> sway preset
```

`Blank3DSniper` loads the master recipe with `gscopebundle89`, creates zoom,
sway and semantic HUD profiles from the resolved document, and resolves the
formal vector reticle by name. If a formal vector reticle exists, the simple
semantic crosshair/ticks are suppressed so the two reticles are not drawn on
top of each other. The semantic reticle remains a fallback when no formal
vector preset resolves.

## Providers

The master recipe uses the `plug_and_play` policy. Each domain can therefore
use bundled behavior or a named external provider with `auto:host` fallback:
recipe, preset, vector, primitive, raster, bars, paint, HUD, telemetry, asset
and zoom. `blank3d_sniper_register_scope_provider()` exposes provider
registration to the host. Zoom is rebound after registration so camera/math
providers can become active without changing the weapon recipe.

## Compatibility

Weapon INI gained `scope_recipe=`. `scope_preset=` is retained as a legacy
fallback. A caller may still pass a preset name directly to
`blank3d_sniper_init()`; recipe paths are preferred.

The 192 legacy numeric IDs remain ABI compatibility only. Runtime reticle
selection and geometry come from INI catalogs/components and can expand beyond
192 without changing C geometry.

## v0.4 animation extension

The master recipe now also selects `animation`. `gscopeanim89` keeps preset
geometry immutable and transforms scratch shapes during emission. Blank3D
routes `aim_enter`, `aim_exit`, `fire`, and `damage` gameplay edges to that
animator and advances it with real frame milliseconds. Animation is provider
domain 11 (`auto:host` by default), with fixed-point internal fallback.

The default `reticle_default` animation set is golden-safe at idle; users can
select `reticle_lively` in the HUD recipe for an always-on breathing reticle
without changing C.
