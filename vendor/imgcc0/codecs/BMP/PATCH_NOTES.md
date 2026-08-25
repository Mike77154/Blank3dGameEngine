# Round 8

- Validación real de payload encapsulado para `BI_JPEG` y `BI_PNG`: el parser ahora comprueba firmas JPEG/PNG antes de aceptar el archivo.
- Nuevas helpers públicas: `bmp_validate_embedded_payload_signature(...)`, `bmp_embedded_payload_signature_matches(...)` y `bmp_embedded_payload_extension(...)`.
- Tests ampliados para cubrir payload válido, firma incorrecta y truncación corta en payload encapsulado.

# Cambios aplicados al parser/decoder BMP

## Parser endurecido
- Validación de `bfReserved1 == 0` y `bfReserved2 == 0`.
- Reglas top-down: solo `BI_RGB` o `BI_BITFIELDS`.
- Validación de `colors_used` para bitmaps indexados.
- Validación temprana de bitmasks (`BITFIELDS` / `ALPHABITFIELDS`).
- Validación de offset mínimo de pixeles contra header + masks + palette.
- Validación de `bV5Reserved == 0`.
- Validación de offsets/tamaño de perfil ICC en V5.

## Tests nuevos
- reserved fields no cero
- top-down + RLE8 inválido
- `colors_used` fuera de rango en 4bpp
- `bfOffBits` apuntando dentro de palette/masks
- máscaras superpuestas en `BITFIELDS`

## Segunda ronda
- `BI_JPEG` y `BI_PNG` ahora se tratan explícitamente como payload encapsulado.
- API nueva: `bmp_image_has_embedded_payload()`, `bmp_get_embedded_payload()`, `bmp_copy_embedded_payload()`.
- Para JPEG/PNG se evita inventar paletas externas; `palette_entries` pasa a 0.
- Validación de `bpp` para JPEG/PNG limitada a valores documentados (incluyendo `0`).
- Tests adicionales para extracción de payload PNG/JPEG y rechazo de `bpp` inválido en esos modos.
- Seeds iniciales para fuzzing en `tests/fuzz_corpus/`.


## Tercera ronda
- Harnesses listos para fuzzing:
  - `tests/fuzz_libfuzzer.c`
  - `tests/fuzz_afl_stdin.c`
- Targets nuevos en `Makefile`:
  - `make fuzz-libfuzzer`
  - `make fuzz-stdin`
  - `make fuzz-afl`
  - `make seedcheck-libfuzzer`
  - `make seedcheck-stdin`
- Diccionario `tests/bmp.dict` y seeds extra para `BI_JPEG`, reserved fields inválidos y `BI_PNG` con `bpp` ilegal.

## Cuarta ronda
- Harness persistente/fallback nuevo: `tests/fuzz_afl_persistent.c`.
- Helpers nuevos en `scripts/`:
  - `minimize_corpus_libfuzzer.sh`
  - `minimize_corpus_afl.sh`
  - `tmin_corpus_afl.sh`
  - `repro_crash.sh`
  - `triage_crashes.py`
- Targets nuevos en `Makefile`:
  - `make fuzz-persistent`
  - `make fuzz-afl-persistent`
  - `make persistent-check`
  - `make corpus-min-libfuzzer`
  - `make corpus-min-afl`
  - `make corpus-tmin-afl`
  - `make crash-triage-libfuzzer`
  - `make crash-triage-stdin`
- Documentación operativa en `FUZZING.md`.
- El triage ignora archivos `README*` y ocultos dentro de `tests/fuzz_crashes/`.
- Directorios de salida separados para triage: `tests/fuzz_triage_libfuzzer/` y `tests/fuzz_triage_stdin/`.


## Quinta ronda
- Se añadieron harnesses separados por camino funcional:
  - parse (`tests/fuzz_libfuzzer_parse.c` / `tests/fuzz_stdin_parse.c`)
  - decode RGBA (`tests/fuzz_libfuzzer_decode.c` / `tests/fuzz_stdin_decode.c`)
  - payload encapsulado (`tests/fuzz_libfuzzer_payload.c` / `tests/fuzz_stdin_payload.c`)
- Nuevos targets en `Makefile`:
  - `make fuzz-libfuzzer-modes`
  - `make fuzz-stdin-modes`
  - `make seedcheck-libfuzzer-modes`
  - `make seedcheck-stdin-modes`
  - `make ci-fuzz-smoke`
- `seedcheck-libfuzzer` ahora respeta `FUZZ_RUNS` para smoke jobs reproducibles.
- Se añadió `.github/workflows/ci.yml` con matriz GCC/Clang y job de fuzz smoke.

## Sexta ronda
- Nuevos targets finos en `Makefile`:
  - `make test-sanitize`
  - `make seedcheck-libfuzzer-parse|decode|payload`
  - `make seedcheck-stdin-parse|decode|payload`
  - `make corpus-min-libfuzzer-parse|decode|payload`
  - `make bundle-ci-parse|decode|payload`
- Se añadió `scripts/bundle_ci_artifacts.sh` para empaquetar logs, corpus fuente, corpus minimizado y binario del harness en CI.
- `.github/workflows/ci.yml` ahora incluye:
  - smoke job con sanitizers (`address,undefined` y `undefined`)
  - jobs de fuzz smoke por target con artifacts subidos por GitHub Actions.


## Séptima ronda
- `scripts/triage_crashes.py` ahora hace bucketing más fino por tipo de sanitizer, firma normalizada y top frames relevantes.
- El triage produce además de `report.json`:
  - `buckets.json`
  - `report.html`
  - directorios `buckets/<bucket-id>/` con entradas agrupadas
- Se añadió `scripts/render_ci_bundle.py` para generar:
  - `manifest.json`
  - `index.json`
  - `index.html`
  - `summary.md`
- `scripts/bundle_ci_artifacts.sh` ahora puede copiar archivos **y directorios extra** dentro del bundle, no solo logs.
- Nuevos aliases en `Makefile`:
  - `make crash-buckets-libfuzzer`
  - `make crash-buckets-stdin`
  - `make test-round7`
- Self-test nuevo para las herramientas de triage/bundle:
  - `tests/mock_crash_harness.py`
  - `tests/test_round7_tools.py`
- `.github/workflows/ci.yml` ahora ejecuta `make test-round7` y publica `summary.md` en el job summary de GitHub Actions.

## Novena ronda
- Se añadió `bmp_diagnostics` con bitmask de warnings y estado de firma para payload encapsulado.
- Nuevos helpers públicos:
  - `bmp_diagnostics_default()`
  - `bmp_warning_mask_has()`
  - `bmp_warning_string()`
  - `bmp_collect_diagnostics()`
- `bmp_image` ahora conserva `diagnostics` tras `bmp_parse_memory()`.
- El parser ya no trata `colors_used` en truecolor como una paleta real: ahora lo tolera y lo reporta como warning.
- Nuevos warnings no fatales para:
  - `file_size == 0`
  - `file_size` menor que el tamaño real del buffer cuando solo sobra basura al final
  - `image_size == 0` en compresiones que permiten inferencia por EOF
  - `colors_used != 0` en truecolor
  - `colors_important` por encima de la paleta declarada
  - BMP top-down válido
  - payload encapsulado JPEG/PNG válido


## Round 10 / 1.0.0

- Se añadió `bmp_report.h`/`bmp_report.c` con versión pública, texto/JSON de diagnósticos y helpers de nombres (`dibType`, `compression`).
- `bmp/bmp.h` ahora exporta también el módulo de reportes como parte de la API pública.
- Se añadió `tools/bmp_diag_cli` para emitir diagnósticos en texto o JSON desde shell/CI.
- Se añadió `VERSION` y `RELEASE_1_0.md` para marcar el corte estable `1.0.0`.

## New in 1.9.0

- Workflow action refs pinned to immutable commit SHAs.
- Added GitHub policy auditing via `make policy-audit`.
- Added maintainer governance templates for CODEOWNERS and branch protection.
- Added dependency-review workflow for public-repository PRs.
