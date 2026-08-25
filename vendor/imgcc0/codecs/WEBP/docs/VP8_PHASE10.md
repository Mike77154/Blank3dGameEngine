# Phase 10 — raw-frame encode + animation optimizer (lossless-first slice)

## Qué aterriza esta fase

Esta fase añade el primer corte realmente usable de **encode desde píxel crudo** dentro del árbol C89:

- `src/webp/encode.h`
- `src/enc/bit_writer.[ch]`
- `src/enc/huffman_enc.[ch]`
- `src/enc/vp8l_enc.[ch]`
- `src/enc/webp_enc.c`
- `examples/gwpencode.c`
- `examples/gwpanimframes.c`
- `tests/test_vp8_phase10.py`

## Alcance real

### Still encode

- `GWPEncodePixels()` acepta `RGBA/BGRA/ARGB` crudo.
- La ruta implementada en esta fase es **lossless / VP8L**.
- La salida es **WebP válido** (`RIFF/WEBP/VP8L`).
- El encode es **literal-only**: sin transforms, sin color cache y sin back-references.
- Como es lossless literal-only, la ruta conserva exactamente RGBA.

### Animated raw-frame encode

- `GWPAnimEncoderAddFramePixels()` acepta frames crudos del tamaño del canvas.
- Usa la ruta **VP8L lossless** interna para codificar cada frame/rectángulo.
- Mantiene un canvas RGBA dentro de `work_mem` provista por el llamador.
- Hace un optimizador simple pero útil:
  - colapsa frames idénticos acumulando duración
  - calcula bounding box mínima de diferencias
  - alinea offsets impares hacia afuera cuando la política de mux exige offsets pares
  - fuerza keyframe completo cuando corresponde (primer frame o `kmax`)
  - usa `NO_BLEND` para escribir el rectángulo exacto del frame final

## Qué todavía no pretende cerrar

- encode **lossy VP8** desde RGBA/YUVA
- mixed mode real `VP8` vs `VP8L` por frame
- heurística RD / `kmin`/`kmax` tan fina como `img2webp`
- near-lossless y sharp-yuv reales
- compresión VP8L avanzada con transforms / cache / LZ77

## Resultado práctico

Esta fase deja el árbol listo para:

1. **codificar still WebP lossless** desde PAM/RGBA
2. **codificar WebP animado** desde secuencias RGBA
3. validar round-trip encode->decode dentro del propio árbol
4. seguir iterando hacia encode lossy/mixed sin cambiar la forma pública básica de la API
