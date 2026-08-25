# Predictor + tiled LZW/Deflate + modos Deflate

Esta vuelta extiende el path de compresión general de `tifx` con tres piezas nuevas:

- `Predictor = 2` (**horizontal differencing**)
- `LZW` / `Deflate` en **tiles** para datos de 8 bits por muestra
- selección explícita de modo Deflate: `stored`, `fixed` o `dynamic`

## Reglas prácticas

- `Predictor` solo se acepta para **gray8**, **RGB24** y **RGBA32**
- `Predictor` solo se acepta con `Compression = 5 / 8 / 32946`
- en **tiles**, `LZW` / `Deflate` quedan habilitados para **gray8/RGB24/RGBA32**
- en **bilevel**, los tiles siguen por ahora en `Compression = 1` o `32773`

## Deflate sin heap

El código enlaza con `zlib`, pero toda la memoria temporal que usa `tifx` sale de un **allocator fijo** interno. No se usa `malloc`, `calloc`, `realloc` ni `free` desde la librería.

## Tests cubiertos

- `test_tiled_lzw_gray_predictor_roundtrip`
- `test_tiled_deflate_rgb_predictor_roundtrip`
- `test_deflate_fixed_rgb_roundtrip`
- `test_deflate_dynamic_rgb_roundtrip`
- `test_bigtiff_deflate_fixed_predictor_roundtrip`
