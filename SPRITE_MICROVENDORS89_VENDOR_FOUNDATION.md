# Sprite microvendors 89

These four cores are intentionally independent:

```text
StaticSprite89     <- one sampled sprite description
ImageSequencer89   <- timed image/source-rect selection
RenList89          <- authoring/parser IR
TileCell89         <- atlas/grid/tile-map geometry
```

Optional adapters compose them without contaminating the cores:

```text
RenList89 -> ImageSequencer89
TileCell89 -> ImageSequencer89
ImageSequencer89 -> StaticSprite89
```

No core depends on Blank3D, OpenGL, imgcc0, AssetRoute89 or SpriteAsset89.
A host may later resolve each opaque image request through any asset service.

## Fifth vendor and live Blank3D bridge

`GMSpritestrip89` is now the fifth agnostic sprite-authoring vendor:

```text
GMSpritestrip89 -> SpriteAsset89 v0.1.2
```

The five vendors are connected to Blank3D only through
`src/blank3d_projectile_sprite89.[ch]`. The cores themselves remain renderer,
filesystem, decoder and gameplay agnostic. See
`PROJECTILE_SPRITE_FIVE_MODES89.md` for the live weapon/projectile recipes.
