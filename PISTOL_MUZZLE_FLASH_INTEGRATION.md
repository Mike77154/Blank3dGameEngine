# Pistol muzzle flash integration

The uploaded 512x512 muzzle-flash artwork is installed as the pistol raster muzzle presentation.
No generative image tool is used and the uploaded JPEG is preserved byte-for-byte at:

`config/weapons/assets/pistol_muzzle_flash.jpg`

## Progressive JPEG compatibility note

The uploaded source is a progressive JPEG (SOF2). The currently vendored `c89jpeg` decoder in this
Blank3D branch intentionally supports baseline sequential JPEG (SOF0) only, so the exact decoded RGB
pixels are also stored in an uncompressed runtime-safe TGA:

`config/weapons/assets/pistol_muzzle_flash_runtime.tga`

The TGA is a deterministic format conversion, not a redraw. A pixel comparison after decoding the
source JPEG and the TGA must be exact.

## Pistol recipe

`config/weapons/pistol.ini` now selects the TGA with:

```ini
muzzle_image=config/weapons/assets/pistol_muzzle_flash_runtime.tga
muzzle_image_width=1.10
muzzle_image_height=1.10
muzzle_image_ms=58
muzzle_image_billboard=view
muzzle_image_blend=additive
muzzle_image_glow=1
muzzle_image_glow_scale=1.35
muzzle_image_glow_alpha=96
```

Additive blending makes black pixels contribute zero light. The optional glow is produced at runtime
as a second, larger additive SpritePlane using the same texture with reduced alpha. It does not bake
bloom into the source image and it requires no shader, heap allocation, float, or double.

## Runtime path

`pistol fire event -> blank3d_muzzle_image_emit -> SpritePlane89 core + glow -> Blank3DImageAssets -> imgcc0 -> OpenGL additive blend`

## QA

Run:

```sh
make test-pistol-muzzle-flash
make test-muzzle-image-pipeline
make test-image-stack
make syntax-check
```

The dedicated test validates INI parsing, two SpritePlane emission, additive blending, glow scale and
alpha, TGA decoding through imgcc0, 512x512 dimensions, and 58 ms lifetime.
