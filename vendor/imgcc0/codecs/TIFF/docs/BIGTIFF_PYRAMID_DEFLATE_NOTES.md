# BigTIFF tiled pyramids + Deflate predictor notes

Esta vuelta deja cubierto en tests y ejemplos el path de **BigTIFF tiled tree/pyramids** con:

- `Compression = 8` (`Deflate`)
- `Predictor = 2` (`horizontal differencing`)
- `deflate_mode = fixed` y `dynamic`
- `SubIFDs` para reduced-resolution (`NewSubfileType = 1`)

## Qué se validó

- root full-resolution en tiles + Deflate dinámico + Predictor
- nivel reducido en `SubIFD` + tiles + Deflate fixed + Predictor
- roundtrip completo parse/decode en BigTIFF

## Heurística nueva del encoder Deflate

Cuando `deflate_mode = TIFX_DEFLATE_AUTO`, el encoder ya no cae siempre en stored.
Ahora analiza el payload por bloques internos del stream zlib y selecciona entre:

- `stored`
- `fixed`
- `dynamic`

Además, ajusta parámetros del compresor usando `deflateParams()` y `deflateTune()` con un allocator fijo interno.

## Nota

El stream TIFF sigue siendo **zlib completo por strip/tile**. La selección por bloques ocurre _dentro_ de ese stream, no cambia la semántica TIFF del contenedor.
