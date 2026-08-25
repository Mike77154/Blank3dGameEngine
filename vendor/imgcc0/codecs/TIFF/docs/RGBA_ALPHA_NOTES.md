# RGBA32 + ExtraSamples

Esta mordida agrega soporte real para RGBA `8,8,8,8` con `ExtraSamples` en TIFF clásico y BigTIFF.

## Decode

- parsea el tag `ExtraSamples`
- reconoce `alpha_mode = 1` (associated alpha / premultiplied)
- reconoce `alpha_mode = 2` (unassociated alpha / straight alpha)
- si la imagen es `PhotometricInterpretation = RGB`, `SamplesPerPixel = 4` y `BitsPerSample = {8,8,8,8}`, la salida pasa a `TIFX_PIXEL_RGBA32`

## Encode

- `TIFX_PIXEL_RGBA32` en TIFF clásico y BigTIFF
- `PlanarConfiguration = 1` o `2`
- `SamplesPerPixel = 4`
- `BitsPerSample = {8,8,8,8}`
- `ExtraSamples = {1}` o `{2}` según `alpha_mode`

## Restricciones

- el alpha sigue limitado a un único canal extra
- classic single-image/tiled/tree: `Compression = 1` o `32773`
- BigTIFF single/page: `Compression = 1`
- BigTIFF tiled/tree: `Compression = 1` o `32773`

## Ejemplos

- `examples/write_gradient.c` ahora genera `gradient_rgba.tif`, `gradient_rgba_big.tif`, `gradient_rgba_planar.tif` y `gradient_rgba_planar_big.tif`
- `examples/inspect_tiff.c` imprime `alpha_mode`, `extra_samples_count` y `planar_config`
