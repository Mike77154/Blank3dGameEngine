# VP8 Phase 6: precisión + alpha

Esta fase sube la exactitud del decoder VP8 still en tres frentes:

- predictores intra 4x4 ajustados al trazado del RFC 6386
- scheduler del loop filter reordenado y separado en MB-edge / subblock-edge
- soporte de `ALPH + VP8 ` para WebP lossy con transparencia

## Cambios principales

- `src/dec/vp8_recon.c`
  - fórmulas 4x4 finas para `VE/HE/LD/RD/VR/VL/HD/HU`
- `src/dec/vp8_filter.[ch]`
  - `common_adjust`, `subblock_filter` y `MBfilter` alineados con la mecánica del RFC
  - límites separados: `mbedge_limit` y `sub_bedge_limit`
- `src/dec/vp8_picture.[ch]`
  - `filter_subblocks` depende de coeficientes reales decodificados o `B_PRED`
  - orden de barrido: borde MB vertical, subbloques verticales, borde MB horizontal, subbloques horizontales
- `src/dec/alpha_dec.[ch]`
  - parseo de header `ALPH`
  - ruta raw+unfilter
  - ruta comprimida con VP8L implícito y extracción del canal verde
- `src/dec/vp8l_dec.[ch]`
  - helper interno/exportado para decodificar image-stream lossless implícito de alpha

## Estado

- `VP8 + ALPH` ya entra por la API principal `GWPDecode()`
- el alpha raw usa los filtros `None/Horizontal/Vertical/Gradient`
- el alpha comprimido via VP8L se decodifica como image-stream sin cabecera y extrae el canal verde
- la ruta lossy sigue siendo una implementación propia en fase de convergencia, no una copia de libwebp ni una garantía bit-exacta total
