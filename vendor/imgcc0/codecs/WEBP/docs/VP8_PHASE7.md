# VP8 phase 7: conformance + exactitud residual

## Objetivo

Cerrar la brecha entre “decoder funcional” y “decoder auditable” mediante:

- corpus oficial fijado
- oracle con `dwebp`
- diff por planos (`pam` / `yuv420p`)
- triage quirúrgico por caso
- fixes puntuales en bordes y scheduler del loop filter

## Código tocado

- `src/dec/vp8_dec.c`
  - clamp de muestras vecinas al ancho/alto visible del frame
  - helper interno `GWPDecodeVP8WithState()`
- `src/dec/vp8_picture.c`
  - scheduler del loop filter acotado a la geometría visible
- `examples/gwpdumpyuv.c`
  - volcado YUV420 plano real de la ruta VP8
- `tests/conformance/*`
  - harness, manifests, oracle, diff, reportes

## Resultado

Ahora la librería puede compararse de forma repetible contra un oracle oficial y reportar el **primer desvío** por plano.
