# NumBar preset catalog

All presets are reusable and bind only to `numbar.*` channels. Position, scale,
z-order, ownership and gameplay data live in the orchestrator INI.

| Preset | Main GBar89/BVH surfaces exercised |
|---|---|
| `re5_radial_green_3d` | Circular ring, damage lag, bevel, outline, shadow, extrusion, inner shadow, gloss, counter |
| `megaman_vertical_yellow` | Bottom-to-top segmented meter with horizontal cuts only |
| `megaman_vertical_blue` | Alternate vertical palette and rail-like pixel frame |
| `ammo_cartridges_gold` | Segments, dynamic segment/unit count, bullet vector units, pair counter, patterns and compositor |
| `boss_layered_red` | Layered meter, dynamic layer count/size, layer colors, layer pips and damage lag |
| `shield_hex_blue` | Hexagonal mask, centered fill, overlay channel, grid and smooth interpolation |
| `stamina_arc_yellow` | Partial radial arc, radial segmentation, round caps, ticks and sweep highlight |
| `oxygen_capsule_blue` | Tall segmented capsule, rail frame, scanlines and damage lag |
| `minimal_flat_white` | Minimal linear meter and smooth value interpolation |
| `industrial_rail_orange` | Rail frame, slanted mask, dynamic middle band, ticks, diagonal background and extrusion |
| `poison_dither_purple` | Dither, crosshatch, dynamic state, automatic thresholds and blink |
| `heart_units_red` | Heart-shaped vector units |
| `radial_segmented_cyan` | Full segmented ring, spokes, detail rings, ticks, dynamic phase and sweep highlight |
| `sprite_nineslice_template` | Nine-slice sprite channels, source rectangles, margins, padding and lag sprite |
| `custom_vector_diamond` | Custom normalized vector polygon units |
| `radial_pie_alarm` | Radial pie, markers, automatic low/critical states, blink, spokes and rings |
| `sprite_clip_template` | Sprite clipping with separate background/fill/lag sources |
| `custom_sliced_boss` | User-defined mask slices over a bevel/checker boss meter |
| `eased_regen_green` | Smooth value plus `smoothstep` easing and regeneration state color |
| `dual_overlay_mid_battery` | Value, overlay and mid channels together; markers and battery vector units |

## Shipped orchestrators

```text
config/hud/gameplay.ini
    Default player health, vertical energy and ammunition.

config/hud/examples/enemy_health.ini
    `self.health` example for an enemy/ally GFO.

config/hud/examples/boss_hud.ini
    Target/boss layered meter example.

config/hud/examples/showcase.ini
    Small mixed showcase.

config/hud/examples/all_presets.ini
    Parser/QA showcase that instantiates all shipped presets.
```

## Replacing a style

Only edit the INI:

```ini
preset=presets/re5_radial_green_3d.bhud
```

can become:

```ini
preset=presets/radial_segmented_cyan.bhud
```

The GFO operation and gameplay binding remain untouched.
