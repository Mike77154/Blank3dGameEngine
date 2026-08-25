# Phase 11

## Qué aterriza

- ruta **lossless-first** mejorada con:
  - `near_lossless` simple
  - `exact` real para alpha invisible
  - transform `subtract-green`
  - color-cache opcional
  - backrefs limitados (distancia 1 / runs)
- ruta **lossy raw encode** opcional vía **bridge a `cwebp`**
- **mixed mode por frame** en animación cuando `allow_mixed` + backend oficial están disponibles
- harness de **encode conformance** contra `cwebp`, `img2webp` y `gif2webp`

## Estado honesto

- el encoder interno puro C89 sigue siendo **VP8L-first**
- el encode **VP8 lossy** auto-contenido todavía no está cerrado como bitstream emitter propio
- la ruta lossy de esta fase usa el backend oficial cuando está presente en el entorno
