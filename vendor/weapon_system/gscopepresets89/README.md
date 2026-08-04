# gscopepresets89

Catalog of **192 pure-vector reticles** for `gscopevector89`.

## Required presets

- `svd_pso1_dragunov`: chevrons, wind scale, vertical post and 1.7 m-style curved stadia layout.
- `pso1_1`: simplified PSO family variant.
- `psg1_hensoldt_6x42`: the simple full-field cross shown in the original Hensoldt PSG 1 telescope data sheet, with the center tagged as an independently colorable illumination part.
- `rpg7_pgo7` and `rpg7_pgo7v3`: vector interpretations of the PGO-7 family grid, range ladder, central post and curved stadia.

The catalog also includes classic duplex/German/post/dot patterns, MIL and MOA scales, BDC ladders, open-center tactical reticles, Christmas-tree/grid patterns, target brackets/parentheses, M40A1/M24/M21/PU/ZF4-inspired patterns, digital/thermal patterns, and eight launcher presets.

## Per-part customization

Every preset is split into semantic parts such as primary lines, posts, center, ticks, range, BDC, wind, lead, stadia, labels, illumination, frame and warning. Copy its default palette and edit any part:

```c
gsv89_palette palette;
const gsvp89_preset *preset;

preset = gsvp89_get(GSVP89_SVD_PSO1_DRAGUNOV);
gsvp89_default_palette(preset, &palette);

gsv89_palette_set_part(&palette, GSV89_PART_BDC,
                        gsp89_rgba(255, 32, 32, 255),
                        gsp89_rgba(0, 0, 0, 255),
                        2, 1, 12, GSP89_BLEND_ALPHA,
                        GSP89_FLAG_OUTLINE, 1);

gsvp89_emit(&painter, preset, &palette, 255);
```

## Source basis

The historical names describe visual reticle layouts for game/rendering use, not calibrated ballistic tables.

- The Hensoldt `TELESCOPE SIGHT 6 x 42 PSG 1` data sheet, Illustration 3, shows a plain full-field cross and states that the sight uses an illuminated reticle in the first image plane.
- The Australian War Memorial identifies the PSO-1 as a 4x sight using the Dragunov tactical ranging reticle, while scanned SVD manual diagrams show the central chevron, additional chevrons, +/-10 scale and curved 1.7 m rangefinder.
- PGO-7/PGO-7V3 documentation and diagrams show the central vertical post, split upper grid, multiple ammunition/range scales and curved stadia.
- Leupold's official tactical reticle manual documents MIL dots, hash marks, open centers and 0.2 mil subdivisions used as the basis for generic tactical presets.

No manufacturer artwork or raster asset is embedded; all preset geometry is newly expressed as integer vector data.

## Build and validation

`make` builds `libgscopepresets89.a` using the vendored dependency headers. Inside the complete bundle, `make demo` builds the catalog example and `make test` verifies all 192 presets emit commands without sprites or renderer glyphs.


## Expanded 192-preset edition

The original preset IDs **0..58 remain unchanged**. IDs **59..191** add 133 newly drawn vector interpretations spanning Resident Evil sniper optics, WWI/WWII and Cold War sights, Soviet and European rangefinding layouts, modern MIL/MOA grids, BDC ladders, hunting reticles, and digital overlays.

Requested entries include `re5_s75_scope`, `re5_psg1_game_scope`, `re_rev_m1891_pu`, `re_rev_muramasa`, `re_village_sa110_scope`, and `re_reverse_sa110_ots`. See `CATALOG_192.md`, `catalog_192.json`, `SOURCES.md`, and `generated_preview/`.
