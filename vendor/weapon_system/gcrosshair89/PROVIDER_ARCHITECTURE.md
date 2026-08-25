# gcrosshair89 provider architecture

The bundle now supports two equal operating modes without changing preset recipes.

## 1. Plug-and-play standalone

`base89 -> params89 -> provider89 -> core89 -> bundled software primitives`

Use `gc89p_draw_rgba()` with a caller-owned RGBA8888 buffer.

## 2. Host-integrated

`base89 -> params89 -> provider89 -> host HUD/vector/primitive/asset providers`

A host may replace any service independently. Missing or declined services fall back downward.

## Service boundaries

| Service | Owns | Fallback |
|---|---|---|
| HUD | viewport, anchor, begin/end layer | supplied width/height + screen center |
| Vector | complete logical shape | core89 built-in vector decomposition |
| Primitive | line, dot, image primitives | secondary primitive provider |
| Asset | image/texture ID drawing | primitive `draw_image` |

This is intentionally capability-based rather than renderer-name-based. The crosshair library does not need to know whether the host is OpenGL, Direct3D, SDL, a custom HUD, a retained vector UI, or a software renderer.

## Partial providers are valid

A vector provider can draw circles but return `GC89P_UNHANDLED` for brackets. The latter automatically falls back to core89.

A primitive provider can provide only `draw_line`; `draw_dot` can come from a fallback provider.

An asset provider can recognize only some image IDs and decline the rest.

## Recipes remain portable

The 192 INI presets do not name a renderer or API. They keep describing shapes, colors, states, spread and animation. Provider selection is an integration concern, so the same preset tree works in standalone tools, a game HUD, or a different vector library.

## Semantic provider identity (provider ABI 2)

A high-level vector provider also receives optional semantic metadata:

- preset ID
- preset name
- preset category

This is deliberately separate from `GC89_DrawSpec`. It lets a host vector system implement named logical shapes without polluting the stable drawing structure or coupling recipes to one renderer.

`gcrosshair_runtime89` fills this metadata automatically.

## runtime89 plug-and-play facade

For applications that do not want to wire the four lower libraries manually, `gcrosshair_runtime89` owns the normal pipeline:

```text
preset ID -> INI style -> input resolve -> semantic metadata -> providers -> fallback
```

The provider runtime remains exposed, so plug-and-play mode and host-integrated mode use the exact same code path.

## Recipe filesystem provider

The INI loader itself is now provider-capable through `GCB89_RecipeIoProvider`. A game can source the same recipe tree from stdio, a virtual filesystem, PAK archive, ROM table or embedded resource store. `stdio` remains the default fallback.
