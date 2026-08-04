# ECG FULL-SURFACE + TRANSPARENCY POLISH — v3.17.1

## Hallazgos de la captura real

- El área activa empezaba en `active_y = 48`.
- Con `active.y_step = 3` y `waveform_y_units = 30`, la gráfica necesitaba
  cerca de 150 píxeles, pero la superficie sólo medía 112.
- `glDrawPixels(..., GL_RGB, ...)` convertía tanto el clear como la caja
  posterior en píxeles totalmente opacos.
- `interval_numerator = 7200` daba unos 112 ms por columna a 64 BPM:
  aproximadamente 3.6 s para las 32 columnas visibles.

## Cambios

- `B3D_ECG_HEIGHT`: 112 → 160.
- Preset `size`: 360×160.
- Fondo y overlay: altura 148.
- Barrido:
  - `interval_numerator 3200`
  - `interval_min_ms 18`
  - `interval_max_ms 58`
  - `damage_interval_ms 14`
- Nuevo bridge RGBA estático:
  - `content_alpha`
  - `background_alpha`
  - `clear_alpha`
- Preset:
  - contenido 255
  - fondo 76
  - clear 0

## Compatibilidad

- C89 estricto.
- Sin malloc/realloc/free.
- El enemigo, Air Lunger y Ground Lancer no fueron modificados.
- El `.bighud` conserva el control de todos los valores nuevos.

## QA

- `make test-bighud`: OK.
- `make test-ecg-vitals`: OK.
- `make test-motion-attacks`: OK.
- `make test-motion-q16-win32`: OK.
- `make syntax-check`: OK.
- `make audit`: OK.
