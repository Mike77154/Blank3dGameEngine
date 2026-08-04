# Speaker-safe crackle correction — v1.0.2

## Ventanas reportadas

- 7–8 s del montaje: inicio de `40 mm HEDP hard impact`.
- 14–15 s del montaje: inicio de `concrete impact`.

## Causa

El `fragment_density` se evaluaba una vez por muestra. En los presets 2 y 4,
los valores 340 y 390 equivalían aproximadamente a 229 y 262 oportunidades
de disparo por segundo a 44.1 kHz. Como cada pulso duraba varios milisegundos,
los eventos se superponían y la capa dejaba de sonar como fragmentos discretos;
se convertía en ruido áspero de alta frecuencia. El WAV no clippeaba a 0 dBFS,
pero una bocina de celular podía entrar en limitación o breakup mecánico.

## Cambios

1. La decisión de disparo se evalúa cada cuatro muestras.
2. No se permite retrigger mientras `fragment_gate` sea mayor a 6500 Q15.
3. Cada evento usa una amplitud pseudoaleatoria de 18500–26691 Q15.
4. El ruido de fragmentos pasa por dos filtros low-pass en cascada.
5. El ataque de `env_fragment` se suavizó de 9800 a 5200 Q15.

## Resultado medido en los primeros 500 ms

| Preset | Métrica | v1.0.1 | v1.0.2 | Reducción |
|---|---:|---:|---:|---:|
| HEDP | media de |Δx| | 382.3 | 125.1 | 67.3% |
| HEDP | media de |Δ²x| | 535.7 | 104.7 | 80.5% |
| Concrete | media de |Δx| | 756.4 | 188.5 | 75.1% |
| Concrete | media de |Δ²x| | 1092.1 | 145.7 | 86.7% |

La energía de fragmentos queda concentrada principalmente entre 1 y 8 kHz,
con una reducción fuerte por encima de 8 kHz. El cuerpo grave, sine, saw,
duración, EQ general, chorus y reverb no fueron eliminados.
