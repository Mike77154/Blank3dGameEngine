# gscope89 provider architecture v0.4

The bundle follows one rule: **internal implementation by default, external provider when requested**.
No provider is required to use the library. A host may replace one domain, several domains, or the whole path.

## Route syntax

Provider routes are ordinary INI data:

```ini
[providers]
recipe=auto:host
preset=auto:host
vector=auto:host
primitive=auto:host
raster=auto:host
bars=auto:host
paint=auto:host
hud=auto:host
telemetry=auto:host
asset=auto:host
zoom=auto:host
animation=auto:host
```

Modes:

- `internal`: always use the bundled implementation.
- `auto:name`: ask provider `name`; if it returns `GPR89_FALLBACK`, use the bundled implementation.
- `external:name`: provider `name` is mandatory; missing/unhandled callbacks are an error.
- A bare name is shorthand for `auto:name`.

The default route for every domain is `internal`.

## Domains

| Domain | Bundled fallback | Provider can replace |
|---|---|---|
| `recipe` | `gscopeini89` filesystem resolver | VFS/archive/network/custom recipe source returning `gri89_doc` |
| `preset` | `gscopepresets89` INI catalog | external reticle/preset database |
| `vector` | batch of `gsv89_shape` | complete vector backend |
| `primitive` | `gsv89_emit_shape()` | individual shape/primitive backend |
| `raster` | `gscoperaster89` | sprites/layers supplied by an engine HUD |
| `bars` | `gscopebars89` | engine-specific bar/widget renderer |
| `paint` | final `gsp89_draw_cmd` sink | renderer command backend |
| `hud` | `gsh89_cmd` sink | external semantic HUD system |
| `telemetry` | caller-owned snapshot | external gameplay/telemetry source |
| `asset` | no named lookup | external texture/font/asset registry |
| `zoom` | `gtelescopiczoom89` math/state | camera and/or optical math provider |
| `animation` | `gscopeanim89` fixed-point shape transform | external animation/transform backend |

`gaimquery89` already receives the collision/raycast world through a callback, so its world dependency was provider-style from the beginning. `gsway89` can remain internal or be bypassed by a telemetry provider that supplies the final HUD snapshot.

## Fallback chain

For a reticle in `auto` mode:

```text
preset recipe
    |
    v
vector provider? -------------------- handled ---> done
    |
    | fallback
    v
for each shape
    |
    +--> primitive provider? -------- handled ---> next shape
    |        |
    |        | fallback
    |        v
    +--> bundled gscopevector89
              |
              v
         gscopepaint89
              |
              v
         paint provider? ------------ handled ---> host renderer
              |
              | fallback
              v
         application draw sink
```

This means a host can own only the final renderer while retaining the library's vector construction, or own the vector system while still using the library's preset recipes.

## Registration

Providers are static/caller-owned descriptions copied into a fixed-size hub. Callback pointers and user pointers are borrowed. No allocation is introduced.

```c
gscb89_ctx scope;
gpr89_provider engine;

gscb89_init(&scope);
gpr89_provider_init(&engine, "host");
engine.capabilities = GPR89_CAP_PAINT | GPR89_CAP_ZOOM;
engine.emit_draw_cmd = my_draw;
engine.zoom_camera = &camera;
gscb89_register_provider(&scope, &engine);
```

A host that needs a provider before the first recipe is loaded can set the route directly:

```c
gscb89_set_provider_route(&scope,
                           GPR89_DOMAIN_RECIPE,
                           GPR89_MODE_AUTO,
                           "host");
```

## ABI and ownership

- `gscopeprovider89`: ABI version 2 (animation callback/domain added).
- `gtelescopiczoom89`: provider ABI version 2.
- no malloc/realloc/free/calloc
- no owned heap
- no float/double
- integer/fixed-point values only
- maximum registered providers: 8 by default
- all fallback behavior is deterministic and synchronous

## Mixed internal/external example

A host is not forced to replace whole subsystems. A vector provider may return `GPR89_FALLBACK` for a batch; then each shape is offered to the primitive provider. A primitive provider may handle only dots/lines and fall back on circles/arcs. Those internal primitives eventually become paint commands, where a paint provider gets another chance to consume them. This allows provider composition at multiple abstraction levels without duplicating the INI content model.
