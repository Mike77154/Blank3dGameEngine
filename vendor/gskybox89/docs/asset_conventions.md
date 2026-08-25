# gskybox89 asset conventions

`gskybox89` stays tiny: the core renderer only emits sky geometry and UVs.
`gskybox89_assets` is a no-heap helper that maps common file names to the six
cube faces so an engine can bind the correct texture handle per face.

## Internal face order

| Internal face | Axis name | Source/Valve suffix | Word suffix |
|---|---|---|---|
| `GSKYBOX89_FACE_POS_X` | `px` | `RT` | `right` |
| `GSKYBOX89_FACE_NEG_X` | `nx` | `LF` | `left` |
| `GSKYBOX89_FACE_POS_Y` | `py` | `UP` | `up` / `top` |
| `GSKYBOX89_FACE_NEG_Y` | `ny` | `DN` | `down` / `bottom` |
| `GSKYBOX89_FACE_POS_Z` | `pz` | `FT` | `front` |
| `GSKYBOX89_FACE_NEG_Z` | `nz` | `BK` | `back` |

## Supported naming styles

- Source/Valve six-face: `Sky_Night01FT.vmt`, `Sky_Night01RT.tga`, `sky_rt0001.bmp`.
- Axis six-face: `px.png`, `nx.png`, `py.png`, `ny.png`, `pz.png`, `nz.png`.
- Word six-face: `my_sky_front.bmp`, `my_sky_right.tga`, `my_sky_bottom.ppm`.

Trailing numeric frame suffixes are tolerated for Source-style raw renders, for
example `sky_rt0001.bmp` maps to `+X`.

## VMF/VMT support

`gskybox89_asset_vmf_extract_skyname()` extracts the `skyname` key from a Source
VMF map file. This is useful when a map says:

```txt
"skyname" "sky_night01"
```

`gskybox89_asset_vmt_extract_basetexture()` extracts `$basetexture` from a VMT
file, for example:

```txt
"$basetexture" "Skybox/Sky_Night01FT"
```

The library does not decode VTF. Recommended flow for Source content:

```txt
VMF -> skyname
VMT -> basetexture face names
VTF/TGA/BMP -> engine texture loader
bind_face(layer=CUBE6, face=...) -> your renderer binds that texture
```

## Tiny preview decoders

`demo_asset_preview_ppm` includes demo-only decoders for:

- uncompressed BMP 24/32-bit
- uncompressed TGA 24/32-bit
- PPM P6

PNG/JPG/VTF are intentionally mapper-only in C. Convert PNG faces to PPM for the
preview demo, or let your real engine's texture system load them.
