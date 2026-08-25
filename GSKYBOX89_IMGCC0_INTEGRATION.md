# gskybox89 + AssetRoute89 + imgcc0 integration

> **Recipe update:** direct skybox authoring in `blank3d.toml` has been superseded by `SkyboxRecipe89`. The global TOML now keeps only `enabled`, `catalog`, and `recipe`. See `SKYBOX_RECIPE89_MATRYOSHKA.md`.


## Purpose

Blank3D vendors `gskybox89` unchanged and uses it as the renderer-agnostic sky
geometry/state core. Image ownership stays in Blank3D's common image pipeline:

```text
blank3d.toml [skybox]
        |
        v
Blank3DSkybox89
        |
        +--> AssetRoute89 (logical image name -> path)
        |
        +--> Blank3DImageAssets
                 |
                 v
              imgcc0
                 |
                 v
          OpenGL image backend
                 |
                 v
       gskybox89 bind_face callback
```

`gskybox89` therefore does not gain PNG/JPEG/etc decoders and remains usable in
other engines and renderers.

## Runtime configuration

The skybox is disabled by default so integrating the vendor does not change an
existing scene. Example six-face setup:

```toml
[skybox]
enabled = true
screen = true
cube = true
dome = false
radius = 80.0

px = "sunset_px"
nx = "sunset_nx"
py = "sunset_py"
ny = "sunset_ny"
pz = "sunset_pz"
nz = "sunset_nz"
```

Put those files under an AssetRoute image root such as
`config/skybox/assets/`. The extension is not required in the TOML when the
logical name is unique. Explicit paths remain valid as a fallback.

The six axis names are:

- `px`: +X / right
- `nx`: -X / left
- `py`: +Y / up
- `ny`: -Y / down
- `pz`: +Z / front
- `nz`: -Z / back

`Blank3DSkybox89` also exposes `blank3d_skybox89_add_face_path()` and delegates
filename convention inference to `gskybox89_assets`, including Source-style
FT/BK/LF/RT/UP/DN names.

## Image formats

Faces use the same `imgcc0` decode path as the rest of Blank3D. The integration
test deliberately decodes six different formats through one skybox:

```text
+X PNG
-X progressive JPEG
+Y BMP
-Y TGA
+Z PCX
-Z TIFF
```

This is a codec-agnostic face consumer: other formats accepted by the active
`imgcc0` build can be supplied in the same way. `gskybox89` itself never
branches on file format.

## Rendering order and state

Blank3D renders the sky after opaque world/actor geometry and before transparent
projectile/trail/sprite effects. Cube geometry is centered on the camera eye,
uses depth test with depth writes disabled, and leaves the camera rotation to
the existing Blank3D view matrix.

The OpenGL adapter saves/restores the relevant depth, blend, cull, texture and
lighting state so later muzzle, projectile, casing and HUD rendering does not
inherit sky state.

If a configured face cannot be resolved/decoded/uploaded, that face is skipped.
The optional procedural screen/dome layers remain available as fallback instead
of making a missing texture fatal.

## Current common image-registry limits

The skybox intentionally uses `Blank3DImageAssets` rather than creating a
private decoder/allocator. Consequently it currently inherits the common image
registry limits, notably a maximum decoded dimension of 1024 pixels per axis
and the existing fixed scratch/file capacities. This can be raised centrally
later if higher-resolution sky faces are required; no gskybox89 API change is
needed.

## Ownership boundary

`vendor/gskybox89/` is retained byte-for-byte from the supplied package.
Blank3D-specific integration lives in:

```text
src/blank3d_skybox89.h
src/blank3d_skybox89.c
src/blank3d_skybox89_gl.c
```

The portable bridge handles face requests and common image handles. The GL file
is only the host rendering adapter.

## Validation

Useful targets:

```sh
make test-gskybox89-vendor
make test-gskybox89-imgcc0
make test-image-stack
make test-muzzle-image-pipeline
make test-projectilevisual2d89-bridge
make test-projectile-sprite-modes89
make syntax-check
```
