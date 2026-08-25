# Validación

## VP8L

La ruta VP8L se validó con un corpus pequeño y sintético usando `ffmpeg` como decoder de referencia del WebP resultante.

Casos cubiertos por `tests/test_vp8l_reference.py`:

- color sólido
- gradiente
- alpha variable
- patrón estilo paleta
- ruido pseudoaleatorio
- patrón más grande con alpha discontinua

## Nota sobre alpha y colores invisibles

La comparación se hace contra la imagen **decodificada desde el WebP** y no necesariamente contra la imagen fuente original, porque algunos encoders lossless pueden cambiar RGB en píxeles totalmente transparentes si no se usa un modo exacto.


## Phase 7

Para validación fina contra corpus real, usa `tests/conformance/run_oracle.py` y luego `tests/conformance/report_failures.py`.
