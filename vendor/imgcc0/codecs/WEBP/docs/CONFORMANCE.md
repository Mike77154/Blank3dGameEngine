# Phase 7: conformance harness

## Objetivo

Convertir la rama VP8/VP8L de “ya decodifica” a “ya se puede auditar con corpus real y oracle oficial”.

## Piezas

- `tests/conformance/corpus_pin.txt`: fija el snapshot oficial.
- `tests/conformance/manifests/*.txt`: separa buckets por ruta (`yuv420p`, `pam`, alpha, lossless, odd-size, segmentation).
- `tests/conformance/run_oracle.py`: corre decoder propio vs oracle.
- `tests/conformance/compare_planes.py`: detecta primer desvío por plano.
- `tests/conformance/report_failures.py`: resume y agrupa fallos.
- `examples/gwpdumpyuv`: saca YUV420 plano real de la ruta VP8 interna.

## Regla práctica

- Usa **`yuv420p`** para cerrar exactitud de la ruta lossy central.
- Usa **`pam`** para lossless y para `ALPH + VP8 `.
- Usa `gwpvp8probe` sobre cualquier caso que falle para mapear el bug a `entropy`, `modes`, `residual` o `filter`.
