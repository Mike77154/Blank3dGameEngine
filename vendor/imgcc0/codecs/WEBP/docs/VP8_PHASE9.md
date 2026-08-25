# Phase 9: anim mux + encoder

Esta fase suma la mitad que faltaba del árbol animado:

- ya había **decode/composición** (`ANIM` / `ANMF`)
- ahora también hay **mux/encode animado**

## Qué implementa

- `GWPAnimEncoder` público (`src/webp/anim_encode.h`)
- `GWPAnimEncoderAddFrameWebP()`:
  - acepta un still WebP de un solo frame
  - extrae `VP8 ` / `VP8L` y `ALPH` si aplica
  - lo empaqueta como `ANMF`
- `GWPAnimEncoderAddFrameBitstream()`:
  - acepta bitstream crudo `VP8` / `VP8L`
  - acepta `ALPH` opcional para el caso `VP8 + ALPH`
- `GWPAnimEncoderAssemble()`:
  - escribe `RIFF/WEBP`
  - escribe `VP8X`, `ANIM`, `ANMF`
  - escribe metadata top-level (`ICCP`, `EXIF`, `XMP `)

## Política de offsets

Los offsets impares se ajustan a par por defecto (`offset &= ~1`) para seguir la semántica práctica del Mux API oficial.

## Límite honesto

Esta fase **no** comprime RGBA crudo a `VP8` / `VP8L` internamente.
Sirve para ensamblar animaciones a partir de frames **ya codificados**.
