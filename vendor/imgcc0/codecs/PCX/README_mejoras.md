# PCX decoder reforzado

Este refactor convierte el decoder en una base bastante más seria para PCX clásico.

## Qué cubre ahora

- Cabecera PCX de 128 bytes con validación estricta de dimensiones y `bytesPerLine`.
- Decodificación `RLE` con protección contra:
  - EOF prematuro
  - runs de longitud 0
  - runs que se salen de la scanline esperada
- Compatibilidad adicional con `encoding = 0` (raw sin RLE) como modo tolerante.
- Soporte real para imágenes indexadas cuyo total de bits por píxel sea `<= 8`:
  - `1bpp x 1 plano`
  - `1bpp x 2/3/4 planos`
  - `2bpp x 1 plano`
  - `4bpp x 1 plano`
  - `8bpp x 1 plano`
  - y combinaciones indexadas multPlano cuyo total siga siendo `<= 8`
- Soporte para `24bpp truecolor` (`8bpp x 3 planos`, RGB planar).
- Trailer VGA de 256 colores (`0x0C + 768 bytes`) cuando existe.
- Fallbacks razonables de paleta:
  - paleta de 16 colores de la cabecera
  - rampa de grises si `paletteInfo == 2` y no existe trailer VGA
  - blanco/negro por defecto si la paleta mono de cabecera está vacía/degenerada
- API de inspección ampliada con `PCXFileInfo`.
- API nueva `pcx_load_indexed()` / `pcx_load_fp_indexed()` para preservar índices + paleta.
- API de memoria para pipelines y fuzzing sin depender de nombres de archivo:
  - `pcx_load_memory()`
  - `pcx_load_indexed_memory()`
  - `pcx_inspect_memory()`
  - variantes `*_with_info` y `*_strict`
- Harnesses para fuzzing listos para `libFuzzer` y AFL++/stdin.
- Diccionario `tests/pcx.dict` y corpus ampliado.
- Compilación limpia con:

```bash
gcc -std=c89 -Wall -Wextra -Werror -pedantic
```

## Qué sigue quedando fuera

El decoder sigue rechazando layouts no estándar o ambiguos con más de 8 bits indexados y diferentes de `24bpp RGB`, por ejemplo `8bpp x 4 planos`.

## Build rápido

```bash
make
make shared
make test
```

## Tests

El script `tests/test_decoder.py`:

- genera fixtures PCX sintéticos
- compila `libpcx.so`
- carga la librería vía `ctypes`
- valida varios modos de color y errores de corrupción

Eso te deja un smoke test portátil sin depender de imágenes externas.

## Fuzz corpus

Se añadieron seeds simples en `tests/fuzz_corpus/` para arrancar fuzzing de `pcx_load()` y `pcx_load_indexed()`.


## Fuzzing rápido

```bash
make fuzz-libfuzzer
make seedcheck-libfuzzer
make fuzz-stdin
make seedcheck-stdin
# opcional en toolchains compatibles:
# make fuzz-libfuzzer FUZZ_SANITIZERS=undefined
# make fuzz-libfuzzer FUZZ_SANITIZERS=address,undefined
```

Si usas AFL++:

```bash
make fuzz-afl AFL_CC=afl-clang-fast
afl-fuzz -i tests/fuzz_corpus -o findings -- ./tests/fuzz_pcx_afl @@
```

## Round 8

- Modo estricto ya no solo inspecciona: también existe como API de carga/decodificación (`pcx_load_strict`, `pcx_load_indexed_strict`, variantes por memoria y con metadatos).
- `PCXFileInfo.strictHeaderPasses` ayuda a detectar si un asset pasa solo en modo tolerante o si también cumple las reglas históricas del formato.

## Round 9

Se añadió una capa de diagnóstico estructurado para distinguir entre:

- archivos que **sí son válidos en modo estricto**
- archivos que **se aceptan en modo tolerante** pero con advertencias

### Nuevos metadatos

`PCXFileInfo` ahora incluye:

- `diagnostics.warningMask`
- `diagnostics.strictWouldFail`
- `diagnostics.paletteSource`

### Warnings típicos

- `PCX_WARN_UNKNOWN_VERSION`
- `PCX_WARN_RAW_ENCODING`
- `PCX_WARN_RESERVED_NONZERO`
- `PCX_WARN_ODD_BYTES_PER_LINE`
- `PCX_WARN_SCANLINE_PADDING`
- `PCX_WARN_EXTENDED_PALETTE_MISSING`
- `PCX_WARN_HEADER16_PALETTE_FALLBACK`
- `PCX_WARN_GRAYSCALE_FALLBACK`

### Uso rápido

- `pcx_inspect_file_ex()` para ver warnings de cabecera
- `pcx_load_with_info()` / `pcx_load_indexed_with_info()` para además saber de dónde salió la paleta realmente usada


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

### Diagnóstico para herramientas / CI

```bash
make diag-cli
./tools/pcx_diag_cli --json tests/fuzz_corpus/valid_indexed8_palette.pcx
./tools/pcx_diag_cli --text tests/fuzz_corpus/valid_rgb24_round3.pcx
```

## Encoder 1.0

El proyecto ya expone encoder público para:

- RGB24 (`pcx_encode_rgb24`)
- indexado 1/2/4/8 bpp (`pcx_encode_indexed`)
- wrappers desde `PCXImage` y `PCXIndexedImage`
- escritura directa a archivo

Notas:

- el encoder emite `version=5` por defecto
- para 8bpp agrega trailer VGA (`0x0C` + 768 bytes)
- para 24-bit escribe 3 planos de 8 bits sin paleta

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

