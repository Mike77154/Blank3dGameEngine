# VP8 Phase 4

Esta fase empuja la rama lossy con tres piezas nuevas:

- `vp8_residual.[ch]`: recorrido real de macroblocks para key-frame VP8, con
  parseo intercalado de modos + token/coeff residuals usando las token
  partitions por fila de macroblocks.
- `vp8_transform.[ch]`: kernels enteros para inverse WHT 4x4 e inverse DCT 4x4,
  más dequant por bloque.
- `vp8_recon.[ch]` y `vp8_filter.[ch]`: suma predictor+residue y filtros simple /
  normal sobre bordes verticales y horizontales.

## Qué sí queda hecho

- resumen real de residual tokens sobre WebP VP8 reales
- histogramas de token types y conteo de bloques con coeficientes
- WHT/DCT enteros utilizables desde C89
- add residual + predicción macroblock-level DC/V/H/TM
- loop filter base con control params derivados de level/sharpness

## Qué sigue faltando para decoder lossy completo

- reconstrucción frame-completa macroblock por macroblock
- b-predictors 4x4 conectados a la ruta de decode final
- composición final YUV -> RGBA desde la ruta VP8 completa
- `ALPH` lossy+alpha en la ruta de salida
- bit-exact cross-check exhaustivo contra libwebp / libvpx
