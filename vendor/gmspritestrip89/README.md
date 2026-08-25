# GMSpritestrip89 v0.1.0

GMSpritestrip89 is a tiny C89, no-heap, provider-friendly wrapper around SpriteAsset89 for **GameMaker-style horizontal sprite strips**.

It exists to vendor the `_stripN` trick as a standalone module:

- `spr_x_walk_strip14.png` → 14 subimages, horizontal strip
- clean logical asset name → `spr_x_walk`
- optional auto parsing from filename/path or explicit frame count
- forwards **image providers** and **render providers** unchanged
- keeps GameMaker-ish semantics such as `subimage`, `image_speed`, and frame playback

## What it does

- parses `_stripN` from a filename or asset name
- strips `_stripN` from the logical asset name
- asks an image provider for `width` and `height`
- verifies `width % N == 0`
- computes `frame_width = width / N`
- defines a clip made of `N` horizontal subimages
- exposes player helpers for `subimage` and `image_speed`

## Dependency model

This package is standalone as a vendor drop because it **vendors SpriteAsset89 v0.1.2 inside `vendor/`**.

## Quick use

```c
GMSpritestrip89 gm;
GMSS89_ImageProvider img;
GMSS89_RenderProvider rnd;
gmss89_init(&gm);
gmss89_set_image_provider(&gm, &img);
gmss89_set_render_provider(&gm, &rnd);
gmss89_define_strip_from_path(&gm, "assets/spr_x_walk_strip14.png", "default", 80U, GMSS89_LOOP_FORWARD);
```
