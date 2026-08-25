# VP8 fase 3: entropy header + macroblock modes + token path scaffold

## Lo que se agregó

### Nuevos módulos

- `src/dec/vp8_probdata.[ch]`
  - tablas fijas de RFC 6386 para:
    - `coeff_update_probs`
    - `default_coeff_probs`
    - `kf_y_mode_probs`
    - `kf_uv_mode_probs`
    - `kf_b_mode_probs`
    - categorías CAT1..CAT6
    - árboles y tablas auxiliares (`coeff_bands`, contextos, zig-zag)
- `src/dec/vp8_tree.[ch]`
  - lector genérico de árboles booleanos VP8
- `src/dec/vp8_entropy.[ch]`
  - inicialización de probabilidades por defecto
  - parseo del entropy header y conteo de updates de coeficientes
  - parseo de `coeff_skip_enabled` / `coeff_skip_prob`
- `src/dec/vp8_modes.[ch]`
  - parseo resumido de modos intra de key-frame
  - histogramas de segmentos, modos Y, modos UV y modos de subbloque
  - sin `malloc`, usando solo contexto local fijo
- `src/dec/vp8_tokens.[ch]`
  - inicialización de token decoders por partición
  - lectura de tokens de coeficientes con categorías y bit de signo
  - aún no hace la reconstrucción completa del bloque

### Cambios fuertes en `vp8_dec.[ch]`

- `GWPVP8ControlHeader` ahora expone:
  - `entropy`
  - `mode_summary`
  - `control_header_bytes_touched`
  - `entropy_header_bytes_touched`
  - `part0_bytes_touched`
- `GWPVP8ParseControlHeader()` ahora recorre:
  - frame tag + key-frame header
  - segmentation / loop-filter
  - token partitions
  - quant / dequant
  - reference header
  - entropy header
  - key-frame mode summary
- `GWPDecodeVP8Stub()` ya deja lista la preparación para el camino de tokens

## Qué valida esta fase

- que el primer partition parsee completo hasta los modos intra
- que las probabilidades de coeficientes se actualicen de forma coherente
- que el resumen de macroblocks sea consistente con `mb_cols * mb_rows`
- que el layout por particiones del bitstream quede listo para la fase de coeficientes

## Qué sigue faltando

- decode real de bloques DCT/WHT
- dezigzag + dequant por coeficiente sobre bloques reconstruidos
- predicción intra final aplicada al residual
- loop filter exacto sobre el frame reconstruido
- composición `ALPH` para el caso lossy+alpha
- escritura final RGBA/BGRA desde la ruta lossy completa

## Criterio de cierre de esta fase

La rama lossy deja de ser sólo “header parser” y pasa a ser:

```text
control header + entropy header + key-frame mode summary + token path scaffold
```

Eso la vuelve un buen punto de apoyo para entrar en la siguiente fase pesada:

```text
token/coeff decode + IDCT/WHT + reconstruction + loop filter
```
