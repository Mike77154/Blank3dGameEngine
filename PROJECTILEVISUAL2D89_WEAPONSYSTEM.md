# ProjectileVisual2D89 - WeaponSystem billboard authority

`vendor/weapon_system/projectilevisual2d89` is the renderer-agnostic authority for 2D projectile presentation in a 3D world.

## Boundary

The weapon/projectile asks for a billboard. ProjectileVisual2D89 then:

1. asks a host frame provider for the current visual frame;
2. applies authored width/height and frame scale;
3. applies deterministic per-projectile pulse/phase;
4. uses the active camera right/up basis;
5. produces TL/TR/BR/BL world-space billboard corners in Q16.16;
6. emits fixed-capacity main/glow/core render passes;
7. decides whether the billboard replaces the legacy projectile mesh;
8. produces a point-light sample;
9. can aggregate multiple projectile light samples into a flickering stream light.

It does **not** own image decoding, OpenGL, filesystems, or authoring formats.

## Five source modes

Blank3D connects its frame provider to the existing five-mode dispatcher:

```text
ProjectileVisual2D89
        |
        | sample_frame(weapon_id, age, slot)
        v
Blank3DProjectileSprite89
        |
        +-- StaticSprite89
        +-- ImageSequencer89
        +-- RenList89 -> ImageSequencer89
        +-- TileCell89 -> ImageSequencer89
        +-- GMSpritestrip89 -> SpriteAsset89
```

Therefore ProjectileVisual2D89 never needs to know whether a frame came from a static image, loose sequence, RenList recipe, atlas/grid/tile cells, or GameMaker `_stripN` subimages.

## Renderer boundary

The core emits corners, tint, source rectangle, blend hint and image handle. Blank3D converts the Q16 corners to its Q12 world format and the OpenGL backend only submits the quad.

This removes camera-facing geometry math from `monika_blank3d.c`.

## Fixed-point / ownership

- C89
- no heap
- no float/double in the vendor
- no explicit 64-bit types
- fixed maximum of three passes per projectile
- caller/provider owns frame/image lookup
- renderer owns texture upload/draw

## Validation

```sh
make test-projectilevisual2d89-vendor
make test-projectilevisual2d89-bridge
make test-projectile-sprite-modes89
make test-flamethrower-billboard
make test-casing-physics
make syntax-check
```
