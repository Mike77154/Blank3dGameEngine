# Known deviations (phase 7 starting point)

## Ya atacado en este tirón

- Clamp de muestras de borde para predicción en frames con dimensiones no múltiplo de 16/8.
- Scheduler del loop filter acotado al ancho/alto real del frame, evitando filtrar contra padding fuera de imagen.

## Lo que el harness puede seguir revelando

- Desvíos finos de `B_PRED` 4x4 en streams intra raros.
- Diferencias en loop filter fuerte/suave bajo ciertas combinaciones de segmentación y sharpness.
- Casos de `ALPH + VP8 ` donde el color base y el alpha pasen por separado pero no exactos en packed RGBA.
