# imgcc0 + SpritePlane89 image pipeline

## Purpose

Blank3D now has one runtime image-decoding authority and two independent presentation paths:

```text
file -> imgcc0 -> RGBA8 -> Blank3D image registry -> renderer texture
                                      |-> HUD / BigHUD / crosshair / scope
                                      `-> SpritePlane89 world planes
                                                `-> optional weapon muzzle-image client
```

`imgcc0` does not render. `SpritePlane89` does not decode files. The Weapon System does not own
generic sprite planes.

## Vendored libraries

- `vendor/imgcc0`: complete uploaded imgcc0 v0.6.0 ZRAGF-cutover tree.
- `vendor/spriteplane89`: complete uploaded SpritePlane89 tree.

Blank3D already contained `shotpumpkin89`, which also exported lower-case `sp89_*` symbols.
The vendored SpritePlane89 copy therefore uses the mechanical lower-case namespace
`sprpl89_*`; its uppercase `SP89_*` constants, data layout, fixed-point math and behavior are
unchanged. See `vendor/spriteplane89/BLANK3D_NAMESPACE_CUTOVER.md`.

## Image registry

`src/blank3d_image_assets.*` owns fixed-capacity image registrations. The registry is lazy:
registering an image does not touch OpenGL and the first draw performs the decode/upload. Reusing
an asset reuses its texture.

Current host policy:

- maximum 128 registered assets;
- dynamic IDs 20000..32000;
- decoded image maximum 1024x1024;
- caller-owned static imgcc0 work buffers;
- first decoded frame is uploaded. Animated/multipage playback is not enabled by this adapter yet.

The complete imgcc0 vendor remains present; this first host adapter intentionally treats images as
static textures.

## Generic SpritePlane89 world usage

The RPY command is independent of weapons:

```rpy
spriteplane "art/stickers/umbrella.tga" pos 2 1 -5 size 1.2 1.2 mode fixed axis xy life 0 blend alpha depth 1
spriteplane "art/screens/security.png" pos -3 2 -8 size 4 2.25 mode fixed axis xy life 0 blend alpha depth 1
spriteplane "art/fx/ghost.png" pos 0 2 -6 size 2 2 mode camera life 0 blend alpha depth 1
```

Useful applications include stickers, signs, wall stains, monitor/TV faces, foliage cards,
particles, distant impostors and persistent billboards. `life 0` means persistent.

A numeric image can also be registered for 2D consumers:

```rpy
image_asset 2100 "art/hud/custom_health.png"
```

BigHUD/HUD sprite IDs can then reference 2100. Existing vector HUD primitives are untouched.

SpritePlane89 atlas frame descriptors are cached by image ID. Repeated temporary spawns (for
example muzzle flashes) do not consume a new fixed atlas-frame slot every shot.

## Weapon muzzle image

The existing Weapon System `GWP89_EVENT_MUZZLE_REQUEST` remains the event authority. Blank3D now
has an optional image presentation client in `src/blank3d_muzzle_image.*`.

Example module recipe:

```ini
[modules]
emit_muzzle=1
muzzle_image=art/fx/muzzle/pistol_flash.png
muzzle_image_width=0.70
muzzle_image_height=0.70
muzzle_image_ms=70
muzzle_image_blend=additive
muzzle_image_billboard=camera
```

Valid billboard values in the weapon recipe are `camera`, `view`, and `fixed`; blend is `alpha`
or `additive`. An empty `muzzle_image` keeps the previous procedural/legacy muzzle path only.
There is no weapon-ID special case.

## Crosshair images without losing vectors

GCrosshair89 already exposes `draw_mode=vector | image | hybrid`. Blank3D now installs an actual
imgcc0-backed HUD image provider. IDs 1000..1041 from
`config/crosshair/assets/assets.ini` are registered automatically and decoded lazily.

Example image-only variant:

```ini
[normal]
draw_mode=image
image_id=1000
image_width_px=48
image_height_px=48
image_tint=0xFFFFFFFF
```

Hybrid keeps both:

```ini
[normal]
draw_mode=hybrid
arm_mask=15
gap_px=5
arm_length_px=7
thickness_px=1
image_id=1000
image_width_px=40
image_height_px=40
image_tint=0x80FFFFFF
```

Vector-only recipes continue through the same original line/dot/vector code.

## Scope / reticle raster layers

GScope89 already emits `GSP89_CMD_SPRITE` from its raster module. Blank3D now consumes that command
through the same HUD image provider and supports the command UV rectangle (0..10000 domain).
The existing vector scope commands remain unchanged, so vector, raster and mixed scopes can coexist.

The scope asset provider named `imgcc0` resolves raster asset paths into dynamic image IDs. It does
not replace GScope's vector provider.

## BigHUD / HUD

`Blank3DHudSpriteProvider` now has both pixel-source and normalized-UV draw callbacks. Existing
BigHUD sprite/nine-slice nodes continue to use numeric sprite IDs; register those IDs with
`image_asset` and they now resolve to decoded textures. Provider-less fallback rectangles remain in
place for builds that do not install the image backend.

## Ownership summary

- `imgcc0`: file decode -> RGBA8.
- `Blank3DImageAssets`: fixed registry, lazy decode/upload and texture lifetime.
- `SpritePlane89`: generic 3D plane/billboard geometry.
- `blank3d_muzzle_image`: optional weapon-event-to-SpritePlane adapter.
- HUD/BigHUD: existing 2D composition; image provider is optional.
- GCrosshair89: retains vector/image/hybrid authoring.
- GScope89: retains vector and raster layers.

No ChatGPT image-generation asset is part of this integration.
