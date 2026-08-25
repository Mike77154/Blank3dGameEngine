# API notes

## Formato nativo

`zragf_compress()` y `zragf_decompress()` usan un frame propio:

- magic `ZRAG`
- versión
- flags
- tamaño original
- payload LZ o LZ+Huffman
- checksum CRC32 opcional

## Capa zlib-like

`zragf_deflateInit2()` interpreta `windowBits` así:

- `-15..-1` -> raw deflate
- `1..15` -> zlib wrapper
- `16..31` -> gzip wrapper

## Semántica actual

La capa compatible ya soporta **streaming real** en los wrappers zlib-like:

- `deflateZ()` puede emitir salida incremental con `NO_FLUSH`
- `SYNC_FLUSH` / `FULL_FLUSH` vacían lo pendiente y fuerzan bloque de flush
- `inflateZ()` conserva parser incremental entre llamadas para raw/zlib/gzip
- `FINISH` solo devuelve `STREAM_END` cuando ya no quedan bytes pendientes de drenar

## Fase 7

Se añadieron piezas de compatibilidad pública que zlib trata como contrato de integración:

- `zragf_deflateSetDictionary()`
- `zragf_inflateSetDictionary()`
- `zragf_deflateSetHeader()`
- `zragf_inflateGetHeader()`

Notas de alcance:

- las **preset dictionaries** quedan soportadas y validadas en streams `zlib` (y en raw `inflate` como preload de ventana)
- `gzip` ahora puede **emitir** y **leer** metadata `extra/name/comment/text/time/os/hcrc`
- la ruta pública ya interoperó con `zlib` externo para diccionarios y cabeceras gzip


## Phase 5 note

`zragf_deflateZ()` now supports real incremental output for the zlib-compatible wrapper path: input can be fed in chunks, `NO_FLUSH` may emit non-final blocks before `FINISH`, and small `avail_out` buffers are handled through an internal pending-output queue.


## Fase 9

Compatibilidad nueva útil para integración:

- `zragf_deflateParams()` permite cambiar `level` y `strategy` durante un stream activo; el cambio se aplica después de vaciar lo ya pendiente con un `SYNC_FLUSH` interno.
- `zragf_inflateValidate()` permite desactivar temporalmente la validación final de checksums del wrapper cuando se necesita rescate tolerante de payload.
- La librería instala ahora target CMake (`zragf::zragflib`) y archivo `pkg-config`, además del header público y docs.


## Fase 10

Backend y streaming más cerrados:

- los bloques comprimidos **no finales** ahora conservan su estado de bits entre llamadas, así que `NO_FLUSH` puede emitir secuencias `FIXED/DYNAMIC` válidas a través de varios bloques
- `deflateZ()` mantiene un historial rodante de hasta 32 KiB y lo usa como prefijo de búsqueda para matches entre chunks
- las preset dictionaries ya influyen también en el backend de compresión, no solo en el contrato `FDICT/DICTID`
