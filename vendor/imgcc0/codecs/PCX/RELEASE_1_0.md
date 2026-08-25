# PCX 1.0.0

Este corte formaliza la API pública del parser/decoder PCX.

## API pública estable

- `pcx.h` como include único
- carga RGB, indexada, por archivo y por memoria
- encoder público para RGB24 e indexados 1/2/4/8 bpp
- inspección tolerante y estricta
- diagnósticos estructurados (`PCXDiagnostics`)
- reportes listos para herramientas/CI en texto y JSON
- `tools/pcx_diag_cli` para inspección desde shell/CI

## Política de versión

A partir de `1.0.0`, los cambios incompatibles deben subir major,
las ampliaciones compatibles suben minor y los fixes compatibles
suben patch.

## Cierre final antes de release

- strict mode endurecido: 8bpp indexado y 24-bit requieren versión 5
- roundtrips encode/decode cubiertos en smoke tests
