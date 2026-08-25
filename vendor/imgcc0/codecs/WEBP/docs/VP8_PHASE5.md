# VP8 phase 5: frame reconstruction completa

## Objetivo

Cerrar la primera ruta end-to-end de VP8 lossy estático:

```text
bitstream VP8 key-frame
-> parse control/entropy/modes
-> token/coeff decode
-> reconstrucción Y/U/V
-> in-loop filter
-> pack RGBA/BGRA/ARGB
```

## Módulos implicados

- `vp8_dec.c`
  - orquesta la tubería completa
- `vp8_modes.[ch]`
  - lectura de segment id / skip / y_mode / uv_mode / b_modes
- `vp8_tokens.[ch]`
  - lectura de coeficientes por bloque
- `vp8_transform.[ch]`
  - dequant + inverse DCT/WHT
- `vp8_recon.[ch]`
  - predicción 16x16 / 8x8 / 4x4 y add-residual
- `vp8_picture.[ch]`
  - buffers padded, loop filter y pack final

## Estado real

La tubería ya reconstruye frames reales y sale a RGBA, pero todavía no pretende equivalencia bit-exacta. En particular:

- varios predictores 4x4 direccionales se implementan como aproximaciones enteras razonables
- el planificador del loop filter es funcional, pero simplificado
- ante truncación en una token partition, el decode degrada a residual cero para no abortar toda la imagen

## Validación local de esta fase

- `make`
- `tests/test_vp8_phase5.py`
- `examples/gwpdecode image.webp out.pam`

La validación de esta fase comprueba que la salida RGBA exista, tenga las dimensiones correctas y mantenga una distancia media razonable frente a la referencia de Pillow sobre varios WebP lossy generados localmente.
