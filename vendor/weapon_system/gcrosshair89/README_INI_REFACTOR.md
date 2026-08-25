# gcrosshair89 — INI recipe bundle

This bundle contains the three supplied libraries with the preset catalog
refactored out of C and into a hierarchical INI recipe system.

```text
gcrosshair_base89/recipes/gcrosshair.ini
  -> lists/base192.ini
     -> presets/000_*.ini ... presets/191_*.ini
        -> optional reusable [include] INI parts
```

`gcrosshair_core89` remains the renderer/core math library and
`gcrosshair_params89` remains the style/shape parameter helper library. Neither
contained the 192-preset catalog, so their runtime logic was intentionally kept
unchanged. All three still share the same ABI3 types header.

The base library now owns the fixed-capacity recipe loader. It has no calls to
malloc/realloc/calloc/free and uses no float/double. Existing catalog APIs lazy
load the stock root, while `gcb89_recipe_load_root()` lets an application choose
a different catalog.

See `gcrosshair_base89/RECIPE_SYSTEM.md` for the format and the modded 193-preset
example.

## Provider bridge added

The bundle also contains `gcrosshair_provider89`, a provider/fallback layer that keeps the INI presets independent from the renderer.

It can use:

- the bundled software line/dot primitives (`gc89p_draw_rgba`),
- an external whole-shape vector provider,
- an external primitive provider,
- a HUD provider for viewport/anchor/layer lifecycle,
- an asset provider for host texture/sprite systems,
- or any mixture of the above, with per-capability fallback.

See `PROVIDER_ARCHITECTURE.md` and `gcrosshair_provider89/README.md`.

## Event animation layer

The bundle now includes `gcrosshair_anim89`. Every one of the 192 preset INIs includes a reusable animation profile under `gcrosshair_base89/recipes/animations/`. Event recipes are separate from the original `[animation]` range fields, so the original pre-INI preset bytes remain regression-testable and identical.

See `ANIMATION_RECIPE_ARCHITECTURE.md`, `ANIMATION_VALIDATION.txt`, and `PRE_INI_EQUIVALENCE.md`.
