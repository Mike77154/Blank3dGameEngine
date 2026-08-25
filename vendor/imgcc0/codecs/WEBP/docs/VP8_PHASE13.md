# Phase 13 - planner -> native intra-only emitter foundation

## Qué aterriza esta fase

- Nueva API pública para construir un **plan de emisión intra-only** a partir del planner nativo:
  - `GWPBuildVP8IntraEmitPlan()`
  - `GWPEncodeLossyNativeProxy()`
  - `GWPEstimateVP8IntraEmitScratch()`
  - `GWPEstimateNativeLossyScratch()`
- Nuevo módulo `src/enc/vp8_intra_emit.[ch]`.
- Nueva CLI `examples/gwpvp8emit`.
- La ruta `gwpencode --lossy` ya no depende obligatoriamente de `cwebp`: si el backend oficial no está disponible, cae a un **native lossy proxy**.
- `gwpanimframes --lossy` también queda utilizable sin tools externos.

## Qué hace exactamente el emisor nativo en esta fase

Esta fase convierte el planner por macroblock en una **IR intra-only utilizable**:

- parte la imagen en macroblocks 16x16
- elige granularidad por bloque (`16x16`, `8x8`, `4x4`) a partir de `segment_id`, varianza y calidad
- calcula color promedio por celda
- fija `qindex` heurístico por celda
- construye una representación intra-only estable, serializable y fácil de depurar

Esa IR se usa para pintar una **proxy lossy nativa** que luego se empaqueta como still WebP válido mediante la ruta interna VP8L.

## Lo honesto

Todavía **no** es un emisor final de bitstream `VP8 ` conforme al RFC. Es el paso intermedio serio que faltaba entre:

```text
planner de macroblocks
    -> emisor intra-only/IR
    -> proxy native-lossy usable sin tools
    -> futuro bitstream VP8 key-frame propio
```

## Por qué vale la pena igual

- ya existe una ruta **lossy sin `cwebp`** para still y animación
- ya existe una IR explícita y depurable para cerrar el encoder VP8 de verdad
- el bundle deja de depender del toolchain oficial para ofrecer una opción lossy funcional

## Uso rápido

```sh
make
./examples/gwpvp8emit --quality 58 --preset photo --dump-json in.pam
./examples/gwpvp8emit --proxy-webp out.webp in.pam
./examples/gwpencode --lossy in.pam out.webp
./examples/gwpanimframes --lossy out_anim.webp f0.pam f1.pam f2.pam
python tests/test_vp8_phase13.py
```
