# VP8 fase 2: control header

Esta fase implementa la parte de VP8 que prepara el decoder para entrar a macroblocks y coeficientes, pero todavía no reconstruye la imagen.

## Qué parsea

- frame tag y key-frame header
- color space y clamping bits
- segmentation header
- loop filter header
- número y tamaños de token partitions
- quant header
- reference header
- factores de dequant por segmento

## Qué produce

La salida principal de esta fase es `GWPVP8ControlHeader`, que deja lista la información de control para la siguiente etapa.

## No implementado aún

- coefficient probability updates
- macroblock prediction records
- token / residue decode
- transforms / reconstruction
- final loop filter pass
