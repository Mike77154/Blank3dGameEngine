# gcrosshair_runtime89

High-level plug-and-play facade over the whole bundle.

```text
runtime89
  +-- base89      INI preset recipes
  +-- params89    state/spread resolution
  +-- core89      built-in vector renderer
  +-- anim89      declarative event animation executor
  `-- provider89  HUD/vector/primitive/asset dependency injection
```

## Minimal standalone route

```c
GC89R_Runtime runtime;
GC89P_SoftwareSurface surface;
GC89_InputState input;

gc89r_init(&runtime, "recipes/gcrosshair.ini");
gc89r_set_preset(&runtime, 144);
gc89p_surface_init(&surface, pixels, w, h, w * 4);
gc89r_draw_rgba(&runtime, &input, &surface, &report);
```

## Host integration

Get the provider bridge with `gc89r_providers(&runtime)` and bind any mix of:

- `GC89P_HudProvider`
- `GC89P_VectorProvider`
- `GC89_DrawCallbacks` primitive provider
- `GC89P_AssetProvider`

`runtime89` automatically sends `semantic_id`, `semantic_name` and `semantic_category` to vector providers. That matters for legacy logical presets whose old ABI3 geometry is only a stub: an external vector/HUD library can distinguish `ring_small`, `diamond`, `scope_mil_dot`, etc. even when their old low-level fields are identical.

No provider is mandatory. Native ABI3 shapes fall through to `core89` and the bundled software primitives.


## Declarative animation

`runtime89` automatically converts input edges into recipe events when `auto_input_events=1`: AIM enter/exit, FIRE rising edge, HIT rising edge, and DISABLED enter/exit. Manual events are available through `gc89r_trigger_event()`. Animation recipes can drive scale, rotation, center offset, alpha, thickness, and dot size with fixed-point curves and attack/hold/return timing.
