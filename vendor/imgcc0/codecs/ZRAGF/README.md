# zragflib

`zragflib` es una librería C pequeña con dos capas:

- un formato nativo `ZRAGF` (LZ + Huffman + frame con checksum), y
- una capa `zlib-like` para `raw deflate`, `zlib` y `gzip`.

## Estado actual

### Lo que ya está bien amarrado
- roundtrip correcto del formato nativo
- CRC/Adler correctos en plataformas LP64
- wrappers `raw/zlib/gzip` con cabeceras y trailers validados
- soporte buffered por chunks (`NO_FLUSH`, `SYNC_FLUSH`, `FINISH`)
- backend RFC1951 con selección entre `STORED`, `FIXED` y `DYNAMIC`
- mapeo correcto de longitud `258 -> símbolo 285` (sin penalización espuria de 5 bits extra)
- estrategias zlib-like útiles: `DEFAULT`, `FIXED`, `HUFFMAN_ONLY` y `RLE`
- validación externa automatizada contra `python/zlib` para rutas `STORED`, `FIXED` y `DYNAMIC`
- decoder `inflate` capaz de leer streams raw externos FIXED y DYNAMIC
- tests básicos, bench y harness de fuzz
- benchmark opcional contra `zlib`
- build CMake con warnings duros opcionales

### Lo que sigue siendo experimental
- el subárbol `zragf_deflate/*` aún mezcla piezas maduras con otras de laboratorio
- la heurística de tokenización todavía es sencilla frente a `zlib/libdeflate`
- no pretende todavía ser un reemplazo competitivo en ratio ni velocidad

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Tests incluidos

- `tests/test_native_roundtrip.c`
- `tests/test_wrapper_roundtrip.c`
- `tests/test_corruption.c`
- `tests/test_phase2_deflate.c`
- `tests/test_external_vectors.c`
- `tests/test_phase4_tuning.c`

## Ejemplos

- `examples/example_native.c`
- `examples/example_zlib_compat.c`

## Bench rápido

```bash
cc -I. bench/bench_roundtrip.c build/libzragflib.a -o bench_roundtrip
./bench_roundtrip
```

## Fase 3

- El encoder DYNAMIC ya emite cabeceras RFC1951 válidas con RLE de code lengths.
- La ruta pública raw/zlib/gzip puede seleccionar `DYNAMIC` cuando realmente gana por tamaño.
- Se añadió una validación externa automatizada para comprobar que `python/zlib` acepta los streams generados.
- El wrapper `deflateZ()` ahora puede usar un buffer temporal interno cuando el peor caso STORED no cabe, pero el stream real sí cabe en el `avail_out` del caller.

## Fase 4

- Se corrigió un bug importante del encoder: antes una longitud de match `258` podía salir como símbolo `284` con `5` bits extra; ahora sale como el símbolo especial `285`, como dicta RFC1951.
- Ese arreglo baja mucho el tamaño en datos repetitivos y en runs largos (`198` bytes vs `488` en el caso repetitivo de referencia; `80` vs `238` en el caso RLE de referencia).
- La tabla fija RFC1951 ya cubre `0..287` sin escribir fuera de rango.
- La capa pública ya usa de verdad varias estrategias zlib-like:
  - `ZRAGF_Z_DEFAULT_STRATEGY`
  - `ZRAGF_Z_FIXED`
  - `ZRAGF_Z_HUFFMAN_ONLY`
  - `ZRAGF_Z_RLE`
- Se añadió un benchmark opcional `bench/bench_compare_zlib.c` para comparar ratio/tiempo contra `zlib`.


## Fase 6

- La capa zlib-like ahora respeta mejor la semántica de `windowBits` en `inflateInit2()`:
  - `0` => zlib header con tamaño de ventana tomado del stream
  - `24..31` => gzip-only
  - `40..47` => autodetección zlib/gzip
- Se añadieron helpers de integración: `zragf_deflateReset()`, `zragf_inflateReset()`, `zragf_inflateReset2()`, `zragf_deflatePending()` y `zragf_deflateBound()`.
- `zragf_stream` ahora expone también `data_type`, `adler` y `reserved`, acercándose más a `z_stream`.
- `next_in` ya se avanza al final del input consumido en vez de quedar en `NULL`, para un comportamiento más compatible.
- Se añadió `tests/test_phase6_compat.c` para validar autodetección, `windowBits=0`, `Reset`, `Reset2`, `Bound` y `Pending`.

> Igual que zlib, `inflate()` no concatena automáticamente miembros gzip: al llegar a `STREAM_END`, hay que llamar `inflateReset()` si quieres consumir el siguiente miembro del archivo concatenado.


## Fase 7

- Se añadieron `zragf_deflateSetDictionary()` y `zragf_inflateSetDictionary()`.
- Los streams `zlib` con `FDICT` ya devuelven `ZRAGF_NEED_DICT` con `strm->adler` igual al Adler-32 esperado del diccionario, y pueden continuar tras `zragf_inflateSetDictionary()`.
- Se añadieron `zragf_deflateSetHeader()` y `zragf_inflateGetHeader()` para metadata gzip (`extra`, `name`, `comment`, `text`, `time`, `os`, `hcrc`).
- Se validó interop cruzada con `zlib` externo en ambos sentidos: diccionarios preset y cabeceras gzip.
- Se añadió `tests/test_phase7_headers_dict.c`.


## Fase 10

- El backend wrapper dejó de tratar los bloques **no finales** como `STORED` por defecto: ahora puede emitir `FIXED` o `DYNAMIC` manteniendo el bitstream abierto entre bloques.
- La ruta streaming de `deflateZ()` conserva **estado de bits** entre bloques comprimidos, así que ya no inserta relleno espurio a byte entre bloques no finales.
- Se añadió un **historial rodante real** en la capa de compresión wrapper, reutilizando hasta 32 KiB de salida previa para encontrar matches entre chunks.
- Las **preset dictionaries** ya no solo cumplen el contrato del wrapper: ahora también se usan como historial real para encontrar matches y bajar tamaño en la primera tanda de datos.
- Se añadió `tests/test_phase10_backend_running.c` para validar:
  - ratio razonable en streaming raw por chunks
  - interop externa del stream generado
  - efectividad real de preset dictionary en compresión
- Se añadió `bench/bench_streaming_compare.c` para comparar `one-shot`, streaming por chunks y el caso de diccionario preset.

## Phase 18

- Hardening agresivo del parser inflate para inputs raros y trails corruptos.
- Límite defensivo de 1 MiB para metadata gzip (`FEXTRA`/`FNAME`/`FCOMMENT`) antes del payload.
- El parser incremental y el core buffered comparten ahora helpers de longitudes/distancias y la preparación de la tabla dummy de distancias.
- `fuzz/fuzz_inflate.c` pasó a ser un harness incremental más serio; se añadió `fuzz/inflate.dict` y un corpus semilla inicial.
- Nuevo test `tests/test_phase18_fuzz_regressions.c` con replay incremental, casos válidos, flags gzip reservados, metadata gigante sin terminador y smoke determinista de mutaciones.


## C89 / C90 build (phase 66)

The library target now supports an opt-in strict C89/C90 build:

```sh
cmake -S . -B build-c89 -DZRAGF_ENABLE_C89=ON -DZRAGF_BUILD_TESTS=OFF
cmake --build build-c89
```

This phase cleaned the remaining C99-only `inline` and `stdint.h` usage from **library** sources and added a small C89 smoke example (`examples/example_c89_simple.c`).


## Public benchmark suite (phase 67)

A reproducible comparative benchmark executable is now included:

```sh
cmake -S . -B build
cmake --build build --target zragf_bench_phase67_public
./build/zragf_bench_phase67_public --csv bench/results/phase67_public.csv --summary bench/results/phase67_public.md
```

It ships with deterministic synthetic corpora and also accepts external files on the command line.
See `docs/BENCHMARKS.md` for details and `tools/run_phase67_public.py` for a simple runner.


## Phase 68 additions

- versioned external corpus manifests for the public benchmark
- manifest-aware benchmark/fetch helper scripts
- optional miniz integration in the public benchmark when miniz is provided


## Phase 69

- se añadió un runner reproducible de benchmark con **lockfile de corpus por SHA-256** (`tools/run_phase69_public.py`)
- se añadió un fetcher/locker que puede congelar manifests (`tools/fetch_phase69_corpora.py`)
- se añadió validación diferencial local para PNG/TIFF (`tools/validate_phase69_png_tiff.py`)
- se añadió un corpus smoke bloqueado offline en `bench/corpora/phase69_local_locked_manifest.json`

- se añadió materialización cache-first para corpus reales (`tools/fetch_phase70_corpora.py`)
- se añadió validación PNG/TIFF más dura con parser estructural local (`tools/validate_phase70_png_tiff.py`)
- se añadió runner reproducible de fase 70 (`tools/run_phase70_public.py`)


## Protocol89 sanitation (C89 fixed-point / static-memory)

The maintained tree now builds as C90/C89 by default and keeps ZRAGF-owned C code free of C-runtime dynamic-storage calls, binary floating-point types, explicit 64-bit integer types, `stdint.h`, pointer-width integer casts, and C99 bounded formatting. Runtime fallback storage uses a compile-time static arena (`ZRAGF_P89_STATIC_BYTES`, default 64 MiB); caller-provided workspaces remain the preferred deterministic path.

Tests, fuzz harnesses, public benchmarks, corpus manifests/materialization tools, documentation, examples, packaging files, and historical validation logs are preserved. Host-side differential oracles such as system zlib/libdeflate are external dependencies and are not claimed to obey ZRAGF's internal static-memory policy.

A permanent byte-exact regression test compares 36 deterministic native/raw/zlib/gzip outputs against the pre-sanitation golden binary. The golden payload is 122165 bytes with SHA-256 `13e5b87531e593d66dfb00db14b85ce0dc25a46990c7c91256b9dc5b5eaa0107`.
