# Blank3D Flamethrower FireLoop Billboard Skin

## Goal

Replace the visible low-poly expanding flame cubes of weapon ID 14 with an
animated camera-facing fire texture while leaving `expandiblefire89` as the
mechanical/collision authority.

## Source selection

Input: user supplied `FireLoop1.zip`.

Selected sequence:

- source resolution: 128x128 RGBA
- source frames: 50
- runtime cadence: 33 ms/frame (~30 fps)
- atlas layout: 8 columns x 7 rows
- atlas dimensions: 1024x896
- runtime asset:
  `config/weapons/assets/flamethrower_fireloop_128_50_atlas.png`

The 256x256/50 sequence was not used because a single atlas would exceed the
current Blank3D image-runtime 1024-pixel per-axis limit and its fixed 6 MiB
RGBA scratch buffer. The 128x128/50 sequence preserves the full fluid loop
without widening the global image memory budget merely for this experiment.

The atlas was assembled deterministically from the supplied PNGs. Every used
128x128 atlas cell was verified pixel-for-pixel against its source frame.

## Runtime ownership

`expandiblefire89` still owns:

- projectile position
- velocity
- travel-distance growth
- kill distance
- collision radius growth
- lifetime

The renderer uses the projectile/box center and current expanded mesh scale to
place the visual card. When the atlas loads successfully, the old cube mesh is
not submitted. If the atlas cannot load, the old procedural cube path remains
as a fallback.

## Billboard animation

`src/blank3d_flamethrower_billboard89.c` is fixed-point/C89 presentation math.
It chooses the frame from projectile age plus a deterministic per-slot phase:

`frame = ((age_ms / 33) + slot * 7) % 50`

This prevents a stream of puffs from looking like identical synchronized
stickers. Odd/even phase also mirrors some cards horizontally.

The card width/height follows the real `mesh_scale` returned by
`expandiblefire89`, plus a small deterministic 0.90..1.10 pulse.

## Rendering layers

Each live flame box is skinned with three additive passes of the same atlas
cell:

1. outer orange glow, scale 1.30
2. colored flame body, scale 1.00
3. white-hot core, scale 0.72

The source alpha is preserved. The renderer uses a half-pixel UV inset so
`GL_LINEAR` does not bleed neighboring atlas cells.

The card basis comes directly from the active camera `right` and `up` vectors,
so every flame remains camera-facing in FPS, TPS, OTS and scope camera modes.

## Stream illumination

The muzzle pulse still uses `GL_LIGHT1`.

The active flamethrower stream now uses `GL_LIGHT2`, computed from the centroid
of live weapon-ID-14 projectiles. Intensity/radius increase with live puff
count (capped) and use deterministic fixed-point flicker before the renderer
boundary.

This lets the flame illuminate nearby world geometry while remaining separate
from the muzzle flash light.

## State isolation

The world billboard pass restores:

- texture binding
- depth write
- blend enable/function
- color
- lighting
- depth test

so casings and later 3D geometry do not inherit flame additive state.

## Test

Run:

`make test-flamethrower`

or only the visual/atlas gate:

`make test-flamethrower-billboard`


## v2: route and atlas metadata de-hardcoded

The runtime no longer contains a compiled FireLoop path or atlas constants.
`projectile_visual_image` is an AssetRoute89 request, and all frame/grid/cadence/
scale/glow/core/light values live in `config/weapons/flamethrower.ini`.
`config/weapons/assets` is a generic image root, so the current INI uses the
logical name `flamethrower_fireloop_128_50_atlas` rather than a filesystem path.
Any weapon can opt into the same generic projectile billboard consumer by
providing a `projectile_visual_image` recipe.
