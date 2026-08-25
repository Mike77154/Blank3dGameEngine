# VP8 phase 12

Esta fase empuja tres frentes prácticos:

1. **Planner nativo para VP8 lossy**: `src/enc/vp8_lossy_plan.[ch]` analiza RGBA/BGRA/ARGB, calcula luma media y varianza por macroblock y propone `segments`, `partitions`, `filter_strength` y `sharpness`.
2. **Puente más alineado con tools oficiales**: `gwpencode` y `gwpanimframes` ahora exponen `preset`, `alpha-q`, `filter-strength`, `sharp-yuv`, `kmin`, `kmax`, `loop`, `min-size` y defaults más cercanos a `cwebp` / `img2webp` / `gif2webp`.
3. **Scheduler animado corregido**: al apagar `minimize_size` ya no se fuerzan keyframes para cada frame. La inserción de keyframes queda gobernada por `kmax` y por el primer frame/canvas inválido.

## Estado honesto

- El planner todavía **no emite bitstream VP8 nativo**.
- Sí deja lista la información que más adelante necesita un encoder intra-only: complejidad por MB, segmentación sugerida y filtros sugeridos.
- La ruta lossy usable sigue apoyándose en `cwebp` cuando se habilitan tools externas.
