# PlanarConfiguration = 2

Esta mordida agrega soporte real de `PlanarConfiguration = 2` para **RGB24** y **RGBA32**.

## Qué hace

- parse/decode de imágenes `RGB` y `RGBA` con planos separados
- writer clásico y BigTIFF con `planar_config = 2`
- soporte tanto en **strips** como en **tiles**
- compresión de salida soportada para este path:
  - `1` = sin compresión
  - `32773` = PackBits

## Semántica de buffers

La API pública sigue siendo chunky:

- el **writer** recibe `RGB24` o `RGBA32` interleaved por píxel
- si `planar_config = 2`, la serialización TIFF sale por planos separados
- el **decoder** reconstruye siempre salida chunky

## Notas

- `gray8` y `bilevel` siguen usando `PlanarConfiguration = 1`
- no se agregó todavía `PlanarConfiguration = 2` para palette/grayscale
- no se agregó LZW/Deflate en esta mordida
