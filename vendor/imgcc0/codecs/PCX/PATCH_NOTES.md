# Round 8

- Nuevas APIs de decodificación estricta: `pcx_load_strict(...)`, `pcx_load_memory_strict(...)`, `pcx_load_indexed_strict(...)` y variantes `*_with_info`.
- `PCXFileInfo` ahora expone `strictHeaderPasses` para distinguir archivos que solo pasan el modo tolerante frente a los que también cumplen la validación histórica.
- Tests ampliados para cubrir decodificación estricta por archivo y por memoria, tanto RGB como indexada.

# Cambios aplicados al parser/decoder PCX

## Parser
- Se añadió `pcx_parse_header_ex(..., flags)`.
- Se añadió `PCX_PARSE_FLAG_STRICT` para validación histórica estricta.
- Se añadió `pcx_validate_header_strict(...)`.
- `pcx_parse_header(...)` sigue siendo tolerante por compatibilidad.

## Validaciones estrictas nuevas
- versiones conocidas `{0,2,3,4,5}`
- `encoding == 1`
- `reserved == 0`
- `bytesPerLine` par
- combinaciones compatibles con modos clásicos soportados

## Paleta 256 colores
- La paleta VGA ahora se busca desde `EOF - 769`.
- Se rechazan falsos positivos si el trailer caería antes del fin real de los datos decodificados.
- Se mantiene fallback a grayscale/header16 cuando no hay trailer válido.

## API nueva
- `pcx_inspect_file_strict(...)`

## Tests nuevos
- caso 8bpp con basura antes de la paleta al final del archivo
- validación estricta de archivo raw no estándar

## Segunda ronda
- API indexada nueva: `pcx_load_indexed()` / `pcx_load_fp_indexed()` / variantes `with_info`.
- El decoder indexado conserva índices originales y paleta final resuelta.
- Tests adicionales: RLE truncado, `reserved` no cero en modo estricto, verificación de API indexada, rechazo de `rgb24` en la API indexada.
- Seeds iniciales para fuzzing en `tests/fuzz_corpus/`.


## Tercera ronda
- API de memoria: `pcx_load_memory()`, `pcx_load_indexed_memory()`, `pcx_inspect_memory()`, variantes `*_with_info` y `*_strict`.
- Harnesses listos para fuzzing:
  - `tests/fuzz_libfuzzer.c`
  - `tests/fuzz_afl_stdin.c`
- Targets nuevos en `Makefile`:
  - `make fuzz-libfuzzer`
  - `make fuzz-stdin`
  - `make fuzz-afl`
  - `make seedcheck-libfuzzer`
  - `make seedcheck-stdin`
- Diccionario `tests/pcx.dict` y seeds extra para cubrir mono, rgb24, reserved no cero y cabecera truncada.

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
- Se añadieron harnesses separados por modo:
  - inspección (`tests/fuzz_libfuzzer_inspect.c` / `tests/fuzz_stdin_inspect.c`)
  - decode RGB (`tests/fuzz_libfuzzer_rgb.c` / `tests/fuzz_stdin_rgb.c`)
  - decode indexado (`tests/fuzz_libfuzzer_indexed.c` / `tests/fuzz_stdin_indexed.c`)
- Nuevos targets en `Makefile`:
  - `make fuzz-libfuzzer-modes`
  - `make fuzz-stdin-modes`
  - `make seedcheck-libfuzzer-modes`
  - `make seedcheck-stdin-modes`
  - `make ci-fuzz-smoke`
- `seedcheck-libfuzzer` ahora respeta `FUZZ_RUNS` para smoke jobs reproducibles.
- Se añadió `.github/workflows/ci.yml` con matriz GCC/Clang y job de fuzz smoke.

## Sexta ronda
- Se añadió `tests/test_smoke.c` para smoke testing nativo en C, útil para correr ASan/UBSan sin depender del flujo Python/ctypes.
- `tests/test_decoder.py` ahora respeta `PCX_TEST_CC`, `CC`, `PCX_TEST_CFLAGS` y `PCX_TEST_LDFLAGS`, de modo que la matriz GCC/Clang realmente usa el compilador pedido.
- Nuevos targets finos en `Makefile`:
  - `make test-smoke`
  - `make test-sanitize`
  - `make seedcheck-libfuzzer-inspect|rgb|indexed`
  - `make seedcheck-stdin-inspect|rgb|indexed`
  - `make corpus-min-libfuzzer-inspect|rgb|indexed`
  - `make bundle-ci-inspect|rgb|indexed`
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
- Se añadió `PCXDiagnostics` con bitmask de warnings, `strictWouldFail` y `paletteSource`.
- Nuevos helpers públicos:
  - `pcx_diagnostics_init()`
  - `pcx_warning_mask_has()`
  - `pcx_warning_flag_to_string()`
  - `pcx_palette_source_to_string()`
  - `pcx_collect_header_diagnostics()`
- `PCXFileInfo` ahora incluye `diagnostics`, de modo que `pcx_inspect_*_ex()` y `pcx_load_*_with_info()` exponen por qué un asset solo pasa en modo tolerante.
- `pcx_load_palette_ex()` reporta si la paleta real vino de:
  - trailer VGA al EOF
  - paleta de 16 colores de cabecera
  - fallback gris
  - fallback a header16 por ausencia de trailer VGA
- Se añadieron pruebas para:
  - warnings de `reserved != 0`
  - padding por `bytesPerLine > mínimo`
  - fallback correcto cuando falta la paleta VGA al EOF


## Round 10 / 1.0.0

- Se añadió `pcx.h` como include único para formalizar la API pública.
- Se añadió `pcx_report.h`/`pcx_report.c` con:
  - `pcx_version_string()`
  - `pcx_decoded_format_to_string()`
  - `pcx_format_diagnostics_text()`
  - `pcx_format_diagnostics_json()`
  - `pcx_write_diagnostics_text()`
  - `pcx_write_diagnostics_json()`
- Se añadió `tools/pcx_diag_cli` para emitir diagnósticos en texto o JSON, útil para herramientas y CI.
- Se añadió `VERSION` y `RELEASE_1_0.md` para marcar el corte estable `1.0.0`.

## Corte final pre-1.0

- módulo nuevo `pcx_encoder.[ch]` con encoder público para RGB24 e indexados 1/2/4/8 bpp
- wrappers desde `PCXImage` / `PCXIndexedImage` y escritura directa a archivo
- `strict` ahora exige versión 5 para 8bpp indexado y 24-bit
- smoke tests de roundtrip encode/decode y validación de versión


## 1.5.0

- deterministic source/binary release packaging
- CycloneDX SBOM generation for source + binary layouts
- SHA-256 / SHA-512 release manifests
- reproducibility verification tooling
- GitHub release provenance + SBOM attestation workflow
- CodeQL, Scorecard, and Dependabot supply-chain automation

## New in 1.8.0

- Workflow action refs pinned to immutable commit SHAs.
- Added GitHub policy auditing via `make policy-audit`.
- Added maintainer governance templates for CODEOWNERS and branch protection.
- Added dependency-review workflow for public-repository PRs.

## New in 1.9.0

- Added Conan 2 packaging (`conanfile.py` + `test_package/`).
- Added local vcpkg overlay port.
- Added `pkg-config` consumer smoke example.
- Added CPack archive generation plus packaging audit automation.

