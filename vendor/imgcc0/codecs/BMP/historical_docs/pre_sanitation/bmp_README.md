# BMP library

Implementación en C para parsear, decodificar y codificar BMP desde memoria.

## Cobertura principal

- **Headers/DIB**: BITMAPCOREHEADER (12), BITMAPINFOHEADER (40), V2 (52), V3 (56), V4 (108), V5 (124)
- **Decoding**:
  - BI_RGB: 1/4/8/16/24/32 bpp
  - BI_BITFIELDS / BI_ALPHABITFIELDS: 16/32 bpp
  - BI_RLE4 / BI_RLE8
  - Paletas CORE (3 bytes) y Windows (4 bytes)
  - Top-down y bottom-up
- **Encoding**:
  - Indexed 1/4/8 bpp (sin compresión, RLE4, RLE8)
  - 16 bpp RGB555 / RGB565 / masks personalizadas
  - 24 bpp BGR
  - 32 bpp BGRX / BGRA / masks personalizadas
  - Headers 12/40/52/56/108/124
  - BITMAPV5 con perfil ICC embebido opcional

## Limitaciones deliberadas

- BI_JPEG y BI_PNG se parsean como payload encapsulado: puedes extraer el buffer original con `bmp_get_embedded_payload()` o `bmp_copy_embedded_payload()`, pero no se decodifican a RGBA dentro de esta implementación.
- No se implementa la familia OS/2 2.x de 64 bytes ni compresiones OS/2 exóticas.
- El campo alpha de paleta se ignora (RGBQUAD reservado), tal como suele hacerse en BMP clásico.

## API rápida

- `bmp_parse_memory()`
- `bmp_get_embedded_payload()` / `bmp_copy_embedded_payload()`
- `bmp_decode_to_rgba32()` / `bmp_decode_to_rgba32_alloc()`
- `bmp_encode_rgba32()`
- `bmp_encode_indexed()`
- `bmp_free_memory()`

## Fuzz corpus

Se añadieron seeds simples en `tests/fuzz_corpus/` para arrancar fuzzing de `bmp_parse_memory()` y `bmp_decode_to_rgba32()`.


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
afl-fuzz -i tests/fuzz_corpus -o findings -- ./tests/fuzz_bmp_afl @@
```

También se añadió un diccionario base en `tests/bmp.dict`.

## Round 8

- Los modos `BI_JPEG` y `BI_PNG` siguen siendo *parse + extracción de payload*, pero ahora el parser exige que el payload tenga firma válida del formato encapsulado.
- Se añadieron helpers para consultar si la firma coincide y qué extensión esperada (`jpg`/`png`) corresponde al payload encapsulado.

## Round 9

Se añadió diagnóstico estructurado para distinguir BMPs *válidos pero sospechosos* de BMPs realmente inválidos.

### Nuevos helpers

- `bmp_collect_diagnostics()`
- `bmp_warning_mask_has()`
- `bmp_warning_string()`

### Warnings típicos

- `BMP_WARN_FILE_SIZE_HEADER_ZERO`
- `BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL`
- `BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED`
- `BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO`
- `BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE`
- `BMP_WARN_TOP_DOWN`
- `BMP_WARN_EMBEDDED_PAYLOAD`

### Nota práctica

Esto permite aceptar assets heredados o “raritos” sin perder información sobre por qué no son del todo limpios.


## Round 10 / 1.0.0

- Se añadió `bmp_report.h`/`bmp_report.c` con versión pública, texto/JSON de diagnósticos y helpers de nombres (`dibType`, `compression`).
- `bmp/bmp.h` ahora exporta también el módulo de reportes como parte de la API pública.
- Se añadió `tools/bmp_diag_cli` para emitir diagnósticos en texto o JSON desde shell/CI.
- Se añadió `VERSION` y `RELEASE_1_0.md` para marcar el corte estable `1.0.0`.

### Diagnóstico para herramientas / CI

```bash
make diag-cli
./tools/bmp_diag_cli --json tests/fuzz_corpus/valid_rgb24.bmp
./tools/bmp_diag_cli --text tests/fuzz_corpus/payload_png.bmp
```
