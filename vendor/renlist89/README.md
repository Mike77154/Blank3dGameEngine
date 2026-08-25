# RenList89

A standalone, agnostic C89 parser for recipe-style image lists inspired by
Ren'Py image/ATL authoring.

RenList89 deliberately does **not** know SpriteAsset89, Blank3D, OpenGL,
AssetRoute89 or an image decoder. It parses text into a fixed-capacity document
IR containing named animations, frames, per-frame durations and arbitrary
key/value properties.

Core frame syntax:

```text
image flame burn:
    fps 30
    "fire_001"
    pause 0.040
    frame "fire_002" for 55
    repeat
```

Unknown `key value` directives become properties, enabling recipe metadata such
as `billboard view`, `blend add`, `glow 1` or future engine-specific hints
without coupling the parser to their meaning.
