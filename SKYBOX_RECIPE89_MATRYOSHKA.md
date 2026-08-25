# SkyboxRecipe89: INI matryoshka recipes for gskybox89

## Goal

The global `config/blank3d.toml` no longer authors a skybox. It only selects a
catalog and one named recipe:

```toml
[skybox]
enabled = true
catalog = "config/skybox/catalog.ini"
recipe = "matryoshka_hybrid_example"
```

All geometry, image requests, layout, UV orientation and optional atmosphere
belong to independent INI recipes. `SkyboxRecipe89` is a renderer/filesystem
agnostic C89 vendor; the Blank3D bridge supplies file IO and applies the final
flattened recipe to `gskybox89`.

## Runtime graph

```text
blank3d.toml ---------+
RPYL -----------------+--> SkyboxRecipe89 catalog/name
runtime API ----------+          |
                                recursive INI includes
                                      |
                                      v
                                flattened recipe
                                      |
                                      v
                                  gskybox89
                                      |
                         Blank3D sky image adapter
                                      |
                    AssetRoute89 -> imgcc0 -> OpenGL
```

## Catalog

`config/skybox/catalog.ini`:

```ini
[recipes]
procedural_default=recipes/procedural_default.ini
night=recipes/night.ini
industrial=recipes/industrial.ini
```

The catalog lets authoring code use stable logical names while the recipe files
can be reorganized freely.

## Matryoshka/subrecipes

A recipe may include other recipe fragments recursively:

```ini
[recipe]
include=../components/cube_geometry.ini
include=../components/dome_atmosphere.ini
subrecipe=../sources/night_cross.ini

[skybox]
enabled=1
radius=96
```

`include` and `subrecipe` are aliases. Includes are evaluated first. Keys in the
including file are evaluated afterwards, so the outer recipe always wins.

The vendor uses fixed caller-owned storage, limits recursion to 8 levels, and
rejects include cycles.

## Source modes

### 1. Procedural layers

No image is required. `gskybox89` may emit its screen gradient and/or dome.

```ini
[skybox]
screen=1
cube=0
dome=1

[screen]
top=80,128,190,255
bottom=170,200,230,255

[dome]
segments=12
rings=5
blend=alpha
top=34,84,160,160
horizon=220,230,240,96
```

### 2. Six independent cube faces

```ini
[source]
type=faces6

[faces]
px=desert_px
nx=desert_nx
py=desert_py
ny=desert_ny
pz=desert_pz
nz=desert_nz
```

Each request is independently resolved by AssetRoute89 and decoded by imgcc0,
so the six files do not have to use the same raster format.

### 3. One cube atlas texture

```ini
[source]
type=atlas
image=desert_cross
layout=cross4x3
uv_inset_pixels=1
```

Supported predefined layouts:

- `strip6x1` / `horizontal_strip`
- `strip1x6` / `vertical_strip`
- `cross4x3` / `horizontal_cross`
- `cross3x4` / `vertical_cross`
- `grid3x2`
- `grid2x3`
- `custom`

For `custom`, normalized Q16-friendly decimal rectangles can be authored per
face:

```ini
[source]
type=atlas
image=my_special_layout
layout=custom

[atlas]
px=0.50,0.333333,0.75,0.666666
nx=0.00,0.333333,0.25,0.666666
py=0.25,0.00,0.50,0.333333
ny=0.25,0.666666,0.50,1.00
pz=0.25,0.333333,0.50,0.666666
nz=0.75,0.333333,1.00,0.666666
```

The same decoded image handle is reused by all six faces; only the source UV
rectangle changes.

### 4. One fullscreen texture

```ini
[source]
type=screen
image=painted_sky
```

This textures the screen layer instead of using its procedural gradient.

### 5. One panorama / dome texture

```ini
[source]
type=panorama
image=night_equirect

[dome]
segments=16
rings=5
blend=off
```

The texture is wrapped around the dome layer. This is useful for single-image
panoramas without converting them to six separate faces.

### 6. Specialized six-file families

A conventional skybox family can be generated from one base name:

```ini
[source]
type=family
base=Sky_Night01
convention=source
extension=tga
```

Conventions:

- `axis`: `px nx py ny pz nz`
- `source` / `valve`: `RT LF UP DN FT BK`
- `word`: `_right _left _up _down _front _back`

This is useful for engines/tools that ship a specialized skybox as a named
six-file family rather than a single atlas.

## Cube UV corrections

Recipes can correct source conventions without changing image files:

```ini
[cube]
flip_u_mask=0
flip_v_mask=0
swap_uv_mask=0
```

The masks use the six gskybox89 face bits. `uv_inset_pixels` can also shrink
atlas cells slightly to suppress linear-filter bleeding across cell borders.

## RPYL selection

RPYL can change skies without rewriting global configuration:

```text
skybox night
skybox_recipe matryoshka_hybrid_example
skybox_reload
skybox off
```

A direct recipe path is available for development/hot authoring:

```text
skybox_recipe_path "config/skybox/recipes/test_weather.ini"
```

## What is intentionally not hard-coded

Neither `gskybox89` nor `SkyboxRecipe89` knows PNG/JPEG/TGA/etc. AssetRoute89
resolves image requests and imgcc0 owns raster decoding. Recipe source mode and
raster file format are orthogonal.

A native DDS cubemap resource is **not** currently treated as six faces because
the present imgcc0 DDS codec rejects DDS cubemap/array/volume resource types.
A normal 2D DDS may still be used as an individual face or atlas if the decoder
accepts it. True DDS cubemap support belongs in the shared imgcc0 image layer,
not as a private decoder hidden inside the skybox recipe system.

## Protocol

SkyboxRecipe89 core:

- ISO C89
- caller-owned fixed workspace
- no malloc/realloc/free
- no renderer dependency
- no filesystem dependency
- no float/double
- fixed-point decimal parsing
- recursive includes with bounded depth and cycle rejection
