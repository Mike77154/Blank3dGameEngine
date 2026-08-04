# Circular health and clean vertical segmentation

## Fault reproduced

The previous replacement profile declared `health_ring` as `kind linear`, so
BigVaderHudder correctly rendered a horizontal bar rather than a ring.

The vertical meter combined `kind segmented` with `pixel_cells true` and
`pixel_size 4`. GBar89's pixel-cell compositor draws cell separators along
both axes, which produced the two longitudinal black dividers visible inside
the narrow meter.

## Correction

`config/hud/gameplay.bighud` now declares the primary health meter as one
continuous radial segment:

```text
kind radial_ring
radial_segments 1
radial_gap_deg 0
radial_cap round
```

The existing 3D composition and delayed damage band are retained through
bevel, outline, shadow, extrusion, inner shadow, gloss and `damage_lag`.

The secondary meter remains a bottom-to-top segmented meter:

```text
kind segmented
segments 28
segment_gap 1
pixel_cells false
pixel_quantize false
fill_grid false
fill_scanlines false
draw_pattern false
```

Consequently, its only interior divisions are the horizontal gaps generated
between the 28 stacked rectangles.

## Unchanged nodes

- `vitals_ecg`
- `ammo_bar`
- `ammo_counter`
