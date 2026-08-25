# Native VP8 intra-only path

## Estado actual

La ruta nativa nueva usa una representación intermedia por celdas intra-only:

- `mode = 0` -> celda tipo macroblock 16x16
- `mode = 1` -> celda tipo 8x8
- `mode = 2` -> celda tipo 4x4

Cada celda guarda:

- rectángulo visible
- `segment_id`
- `qindex` heurístico
- color promedio RGBA
- promedios Y/U/V derivados

## Qué sale del emisor hoy

Hoy el emisor genera una **proxy native-lossy** de bloque promedio que sirve para:

- medir tamaño sin usar tools oficiales
- validar scheduling animado sin `cwebp`
- depurar `segments`, `partitions` y `qindex`
- tener una base concreta para el futuro writer de particiones/token stream VP8

## Qué falta para cerrarlo como VP8 real

- bool/range **encoder** VP8
- writer real de frame tag / headers / partitions
- elección de modos intra a nivel macroblock/subblock
- emisión de coeficientes/token stream
- acople de loop filter/segmentación con el bitstream real
- conformance exacta contra `cwebp`
