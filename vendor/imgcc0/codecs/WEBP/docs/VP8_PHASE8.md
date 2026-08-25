# Phase 8: animation decode + animated conformance

## Qué entró

- API pública nueva: `src/webp/anim_decode.h`
- Decoder animado sin `malloc` en el core:
  - parseo fino de `ANIM` / `ANMF`
  - extracción de subchunks por frame (`ALPH`, `VP8 `, `VP8L`)
  - composición sobre canvas RGBA con:
    - blend alpha (`B = 0`)
    - overwrite (`B = 1`)
    - dispose none (`D = 0`)
    - dispose background (`D = 1`)
- Ejemplo nuevo: `examples/gwpanimdump`
- Harness nuevo:
  - `tests/conformance/compare_animation.py`
  - `tests/conformance/run_anim_oracle.py`
  - `tests/conformance/discover_cases.py`

## Estado

- Animación **decode/demux/composite**: sí.
- Animación **encode/mux completo**: no; el mux sigue enfocado en contenedor estático.
- Oracle animado oficial dentro de este paquete: el arnés queda preparado y el flujo local usa Pillow para humo rápido; para validación externa fina conviene seguir cruzando contra utilidades/libwebp del ecosistema oficial.

## Nota de exactitud

La composición alpha puede diferir en ±1 LSB frente a otros decoders según redondeo y espacio de color. Por eso el arnés animado acepta umbrales configurables por frame.
