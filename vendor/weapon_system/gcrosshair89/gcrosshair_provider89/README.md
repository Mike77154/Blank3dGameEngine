# gcrosshair_provider89

Provider bridge for the INI-driven gcrosshair89 bundle.

The important rule is that presets remain renderer-agnostic. A preset describes a logical crosshair. At runtime the provider bridge decides who draws it.

## Provider chain

```text
GC89_Style (INI recipe)
        |
        v
GC89_DrawSpec (params89)
        |
        v
provider89
  |
  +-- HUD provider -------- viewport, anchor, begin/end layer
  |
  +-- Vector provider ----- whole circle/diamond/bracket/etc.
  |       |
  |       `-- decline -> core89 internal vector renderer
  |                         |
  |                         `-- primitive provider
  |                              |
  |                              `-- missing primitive -> fallback primitive provider
  |
  `-- Asset provider ------ image_id / texture owned by host
          |
          `-- decline -> primitive draw_image callback
```

Every provider is optional and may decline work. This makes integration incremental: a game can begin with the bundled renderer and later replace only the vector system, only the HUD placement, only images, or every service.

## Standalone mode

`gc89p_draw_rgba()` uses the bundled C89 software line/dot primitives on a caller-owned RGBA8888 buffer. No heap is used.

```c
GC89P_SoftwareSurface surface;
gc89p_surface_init(&surface, pixels, width, height, width * 4);
gc89p_draw_rgba(&core, &spec, &surface, &report);
```

This is enough for the current vector-only 192 preset pack.

## External vector system

Bind one function with `gc89p_runtime_set_vector_provider()`. It receives the complete logical `GC89_DrawSpec`, resolved center and fixed-point scale. Return `GC89P_HANDLED` when your vector library drew it. Return `GC89P_UNHANDLED` for shapes you do not support; provider89 falls back to core89 automatically.

## Primitive system

Bind the existing `GC89_DrawCallbacks` ABI through `gc89p_runtime_set_primitive_provider()`. Providers can expose only the functions they have. A second callback table can be installed with `gc89p_runtime_set_primitive_fallback()` to fill missing primitives.

## HUD system

`GC89P_HudProvider` can override viewport dimensions, choose the anchor/reticle origin, and receive begin/end layer notifications. If absent, provider89 uses the screen size supplied to `gc89p_draw()` and the normal screen center.

## Asset/image system

`GC89P_AssetProvider` receives image IDs directly. It can map them to the host's texture manager, atlas, sprite system or UI assets. If it declines an image, the normal primitive `draw_image` callback is tried.

## Constraints

- C89
- Q16.16 fixed point
- no malloc/realloc/calloc/free
- no float/double
- caller-owned memory only
- existing core89/params89/base89 ABI remains untouched
