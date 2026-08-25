# Arquitectura

`giffywebp-c89` está organizado para parecerse a una librería tipo `libwebp`, pero con restricciones más duras:

- **C89** puro.
- **Sin `malloc`** dentro del core propio.
- **Scratch arena** inyectada por el llamador.
- **Módulos separados** por responsabilidad.

## Layout

```text
src/
├── webp/      # headers públicos
├── dec/       # decode core (dispatcher + VP8L + VP8 fase 1)
├── demux/     # RIFF/WebP parser, frames, metadata
├── mux/       # writer del contenedor WebP
└── utils/     # arena, bitreader, endian, fixed, huffman
```

## Flujo de decode

```text
bitstream -> demux/container parse -> feature probe
         -> dec/webp_dec.c dispatcher
         -> dec/vp8l_dec.c (lossless)
            o
         -> dec/vp8_dec.c (lossy fase 1: frame-tag/key-header/partitions)
         -> RGBA de salida en buffer del usuario
```

## Reglas de memoria

- Toda la memoria temporal sale de `GWPDecoderOptions.scratch`.
- Los Huffman tables y buffers temporales se reservan dentro de la arena.
- El buffer de salida RGBA también lo aporta el usuario.

## VP8L

La ruta VP8L hoy contiene:

- lectura del header `VP8L`
- lectura de color cache
- lectura de prefix codes
- entropy image / meta prefix codes
- LZ77 y color cache codes
- transforms inversas en orden inverso:
  - predictor
  - color transform
  - subtract-green
  - color-indexing

## VP8 lossy fase 1

La ruta lossy quedó partida para crecer por capas:

```text
src/dec/
├── vp8_frame.[ch]   # frame tag + key-frame header + first partition span
├── vp8_yuv.[ch]     # fixed-point YUV420 -> packed RGBA/BGRA/ARGB
└── vp8_dec.[ch]     # dispatcher/stub de la ruta lossy
```

Lo siguiente para cerrar VP8 lossy es:

1. bool decoder
2. parseo del resto del frame header comprimido
3. predicción intra
4. token decode
5. dequant + IDCT/WHT enteras
6. loop filter
7. raster YUV -> RGBA


## Phase 7

- `tests/conformance/` centraliza corpus pin, oracle, diff por planos y reportes de triage.
- `examples/gwpdumpyuv` existe para inspeccionar la salida YUV420 real de la ruta VP8 interna.
