# ProjectileVisual2D89 v0.1.0

Renderer-agnostic C89/fixed-point WeaponSystem microvendor for 2D projectile visuals in 3D worlds.

It owns **billboard presentation policy**, not assets or rendering APIs:

- asks a provider for the current projectile frame;
- turns that frame into a view-facing billboard;
- computes TL/TR/BR/BL world corners in Q16.16;
- supports main/glow/core passes;
- supports alpha or additive blend hints;
- deterministic per-projectile pulse and phase flip;
- optional age fade/tint gradient;
- mesh replacement decision;
- per-projectile light sample and stream-light accumulator.

The frame provider is deliberately opaque. A host can back it with one static image, a sequence, RenList89, TileCell89, GMSpritestrip89, SpriteAsset89, or something else entirely.

The core does not include OpenGL, imgcc0, filesystem APIs, AssetRoute89, Blank3D types, or any of the sprite microvendors.

## Protocol

- C89
- fixed-capacity output (3 passes)
- no malloc/calloc/realloc/free
- no float/double
- no explicit 64-bit types
- provider-owned image/frame lookup
- host-owned renderer
