# BMP 1.0.0

Este corte formaliza la API pública del parser/decoder BMP/DIB.

## API pública estable

- `bmp/bmp.h` como include único
- parse, decode y extracción de payload encapsulado
- diagnósticos estructurados (`bmp_diagnostics`)
- reportes listos para herramientas/CI en texto y JSON
- `tools/bmp_diag_cli` para inspección desde shell/CI

## Política de versión

A partir de `1.0.0`, los cambios incompatibles deben subir major,
las ampliaciones compatibles suben minor y los fixes compatibles
suben patch.

## Pulido final

- roundtrip encode/decode cubierto también para BMP top-down sin compresión
- encoder endurecido para perfiles ICC embebidos incompletos
