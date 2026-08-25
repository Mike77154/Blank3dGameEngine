# Official tool profiles

Se añadieron opciones espejo en los ejemplos y scripts para acercarse a la semántica de:

- `cwebp`
- `img2webp`
- `gif2webp`

Campos expuestos en esta fase:

- `preset`
- `alpha-q`
- `filter-strength`
- `sharp-yuv`
- `kmin`
- `kmax`
- `loop`
- `min-size`

Los scripts en `tests/conformance/` ahora aceptan estas banderas para comparar la salida propia con herramientas oficiales cuando estén instaladas.
