# After phase10

El siguiente salto natural ya no es raw-frame básico, porque la ruta **lossless-first** quedó viva.
Lo que sigue es:

- encode **VP8 lossy** desde RGBA/YUVA
- mixed mode real por frame (`VP8` vs `VP8L`)
- heurística más fina de keyframes/deltas (`kmin`, `kmax`, coste de blend/dispose)
- compresión VP8L avanzada (predictor/color/cache/backrefs) para bajar tamaño frente al encoder literal-only actual
- conformance de encode contra `cwebp`, `img2webp` y `gif2webp`
