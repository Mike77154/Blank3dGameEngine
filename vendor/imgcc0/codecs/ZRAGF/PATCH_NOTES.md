# zragflib patch notes

## Trabajo realizado

### 1) Correctness del formato nativo
- Se corrigió el uso de `zragf_u32`: ya no depende de `unsigned long` en LP64.
- Eso arregla CRC32/Adler y el roundtrip nativo que antes fallaba en 64-bit.
- Se endureció `zragf_decompress()` para tratar output mayor al esperado como corrupción.

### 2) Símbolos duplicados / backend RFC1951
- Se eliminó el stub duplicado de `zragf_deflate_rfc1951_compress` del stream layer.
- El backend RFC1951 baseline quedó centralizado en `zragf_deflate_rfc1951.c`.
- Se añadió soporte a bloques STORED finales y no finales para el wrapper por chunks.

### 3) Build system
- `CMakeLists.txt` ahora usa `project()` y fija C99.
- Se añadió `zragflib_internal.c` al build.
- Se agregaron opciones `ZRAGF_ENABLE_STRICT`, `ZRAGF_ENABLE_ASAN`, `ZRAGF_BUILD_TESTS`.

### 4) Endurecimiento de compilación
- Todos los `.c` del árbol compilan con:
  `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Werror`
- Se sacaron helpers internos del header para evitar `unused static` por TU.
- Se limpiaron conversiones/sign warnings en el subárbol DEFLATE.

### 5) Modelo de estado explícito
- Se introdujo `zragf_state_kind`.
- Se reemplazó la detección heurística del tipo de `state` en `zragf_stream.c`.
- Los estados nativos y zlib-like ahora se distinguen de forma explícita.

### 6) Wrapper zlib/raw/gzip más completo y honesto
- `zragf_stream.c` fue reescrito como capa buffered por chunks.
- `NO_FLUSH` acumula.
- `SYNC_FLUSH` emite bloques STORED no finales.
- `FINISH` emite el bloque final y trailer.
- `inflateZ()` valida cabeceras/trailers para `zlib` y `gzip`.
- Se validan Adler32, CRC32 e ISIZE cuando aplica.

### 7) Tests, fuzz, bench y ejemplos
- `tests/test_native_roundtrip.c`
- `tests/test_wrapper_roundtrip.c`
- `tests/test_corruption.c`
- `fuzz/fuzz_inflate.c`
- `bench/bench_roundtrip.c`
- `examples/example_native.c`
- `examples/example_zlib_compat.c`

### 8) Heurísticas/documentación
- Se eliminó la heurística insegura de `deflate_hash.c` que infería validez de datos por bytes cero.
- `deflate_core.c` ahora lleva conteo explícito de bytes válidos en ventana.
- Se documentó que el backend avanzado `zragf_deflate/*` sigue siendo experimental.
- El wrapper público usa hoy un backend STORED baseline estable.

## Validación ejecutada
- Build estricta de la librería con `-Werror` sobre todos los `.c` -> OK.
- Tests manuales compilados con flags estrictos -> 5/5 OK:
  - `test_native_roundtrip`
  - `test_wrapper_roundtrip`
  - `test_corruption`
  - `test_external_vectors`
  - `test_phase2_deflate`
- Verificación cruzada adicional con Python estándar: los streams raw/zlib/gzip emitidos por `zragflib` se descomprimen correctamente con `zlib`/`gzip`.
- Ejemplos compilados y ejecutados.

## Fase 2

### 9) DEFLATE real en el wrapper público
- `zragf_deflate_rfc1951.c` ya no se limita a `STORED`.
- Se activó tokenización LZ77 real con búsqueda hash-chain y lazy matching simple.
- El compresor compara candidatos `STORED`, `FIXED` y `DYNAMIC` y elige el menor.
- El wrapper público raw/zlib/gzip ahora puede emitir bloques DEFLATE comprimidos de verdad.

### 10) FIXED / DYNAMIC end-to-end
- Se corrigió la emisión FIXED para escribir códigos Huffman en orden LSB-first correcto.
- Se endureció el decoder Huffman de `inflate` usando códigos canónicos invertidos para lectura bit-a-bit correcta.
- `inflateZ()` ahora decodifica correctamente streams raw DEFLATE externos con bloques FIXED y DYNAMIC.

### 11) Cobertura nueva de pruebas
- `tests/test_phase2_deflate.c` valida tres cosas:
  - inflate de un stream raw FIXED externo conocido
  - inflate de un stream raw DYNAMIC externo conocido
  - que el wrapper raw emita un bloque FIXED real para un payload compresible
- `tests/test_external_vectors.c` valida compatibilidad de `inflateZ()` con vectores externos raw/zlib/gzip generados fuera de la librería.
- `tests/test_external_vectors.c` añade compatibilidad comprobada con vectores externos raw/zlib/gzip.


## Phase 2 status

- Public RFC1951 encoder path now selects between **STORED** and **FIXED** blocks using real tokenization and size comparison.
- Raw/zlib/gzip wrapper finish-path now uses the RFC1951 encoder instead of STORED-only output.
- Dynamic **decoding** is supported and validated against known vectors.
- Dynamic **encoding** code remains in-tree but is **not selected by the public encoder yet**, because it still needs stricter validation against external inflaters on pathological inputs.

### 9) Fase 3: encoder DYNAMIC validado externamente
- Se reemplazó la cabecera DYNAMIC provisional por una codificación RFC1951 válida de code lengths.
- Ahora se generan tokens RLE (16/17/18) para la secuencia HLIT/HDIST y se construye el árbol CL real.
- Se corrigió el doble conteo accidental del símbolo EOB en la construcción de frecuencias dinámicas.
- El selector público del backend RFC1951 ya puede elegir `DYNAMIC` cuando produce el mejor tamaño.
- Se añadieron pruebas de humo para bloque DYNAMIC y una validación externa con `python/zlib`.
- El wrapper `deflateZ()` ahora puede usar un scratch buffer temporal para no fallar por el peor caso STORED cuando la salida real sí cabe.


## Phase 4

- Corregido el mapeo de longitud 258 (ya no se emite como símbolo 284 con 5 bits extra).
- Arreglada la tabla fija RFC1951 para cubrir 0..287 sin UB.
- La estrategia zlib-like ya no se ignora del todo: Z_FIXED, Z_HUFFMAN_ONLY y Z_RLE tienen comportamiento real.
- Añadido benchmark opcional contra zlib.


## Phase 5 / Fase 5 – Real streaming deflate

- `zragf_deflateZ()` now emits RFC1951 blocks incrementally on `NO_FLUSH` once buffered input crosses a streaming threshold, instead of waiting for `FINISH`.
- Added internal pending-output queue so small `avail_out` buffers can drain a stream across repeated calls without losing state.
- `SYNC_FLUSH` / `FULL_FLUSH` flush buffered input and emit an empty STORED flush block.
- `FINISH` emits the final block plus wrapper trailer and only returns `ZRAGF_STREAM_END` once all pending bytes are drained to the caller.
- Added `tests/test_phase5_streaming.c` validated against external zlib for raw, zlib, and gzip wrappers.


## Phase 5.1 / Fase 5.1 – Incremental inflate + running backend

- `zragf_inflateZ()` dejó de ser full-buffer: ahora mantiene un parser incremental real de raw/zlib/gzip entre llamadas.
- El decoder conserva estado de bloque DEFLATE, bitreader, ventana, tablas Huffman y copias parciales (`match copy`) cuando falta input o `avail_out`.
- Se añadió verificación incremental de trailers zlib/gzip (Adler32, CRC32, ISIZE) al cerrar el stream.
- El backend de entrada comprimida ahora compacta el buffer interno por tramos consumidos en vez de depender de `memmove` frontal continuo.
- Añadido `tests/test_phase5_incremental_inflate.c`, validado con streams raw/zlib/gzip generados por zlib externo y drenados con buffers de entrada/salida pequeños.


## Phase 6 / Fase 6 – Compatibilidad zlib más cerrada

- `inflateInit2()` ahora acepta `windowBits=0`, `24..31` (gzip-only) y `40..47` (autodetección zlib/gzip).
- Para zlib-wrapped streams con `windowBits=0`, el tamaño máximo de ventana se toma del header CMF/FLG del stream, y se rechaza un header que exceda el máximo pedido por el caller.
- Se añadieron `zragf_deflateReset()`, `zragf_inflateReset()`, `zragf_inflateReset2()`, `zragf_deflatePending()` y `zragf_deflateBound()`.
- `zragf_stream` expone ya `data_type`, `adler` y `reserved`, y los wrappers mantienen `adler`/`CRC32` incrementalmente.
- `next_in` en wrappers ya se avanza como en zlib cuando el input se consume, en vez de anularse.
- Se añadió `tests/test_phase6_compat.c`, que valida:
  - autodetección zlib/gzip con `47`
  - `windowBits=0`
  - `deflateBound()` como cota superior usable
  - `deflatePending()` con salida pendiente real
  - reutilización de stream con `deflateReset()`
  - reutilización de stream con `inflateReset()` y `inflateReset2()`
- La fase 6 mantiene el comportamiento deliberado de zlib respecto a miembros gzip concatenados: no los consume automáticamente; el caller debe hacer `inflateReset()` entre miembros si hay más datos.


## Phase 7 / Fase 7 – preset dictionaries + gzip metadata

- Se añadieron `zragf_deflateSetDictionary()` y `zragf_inflateSetDictionary()`.
- Los streams zlib con `FDICT` ahora propagan `ZRAGF_NEED_DICT` y exponen el Adler-32 esperado en `strm->adler`, como hace zlib.
- `zragf_write_wrapper_header()` ya puede escribir `DICTID` en cabeceras zlib cuando hay diccionario preset.
- Se añadió `zragf_deflateSetHeader()` para emitir metadata gzip variable (`extra`, `name`, `comment`, `text`, `time`, `os`, `hcrc`).
- Se añadió `zragf_inflateGetHeader()` y el parser incremental gzip ahora rellena esa estructura y valida `FHCRC` si está presente.
- Se añadió `tests/test_phase7_headers_dict.c`, validando interop cruzada con zlib externo en cuatro caminos:
  - zlib externo + preset dict -> `zragf_inflateZ()`
  - `zragf_deflateZ()` + preset dict -> zlib externo
  - `zragf_deflateSetHeader()` -> `inflateGetHeader()` externo
  - `deflateSetHeader()` externo -> `zragf_inflateGetHeader()`

## Phase 8 / Fase 8 – Copy / Prime / Sync + tuning hook

- Se añadieron `zragf_deflateCopy()` y `zragf_inflateCopy()` con copia profunda del estado interno, buffers pendientes, ventana, diccionario y tablas Huffman activas.
- Se añadió `zragf_inflatePrime()` para insertar bits previos al primer `inflate()` en modo raw, siguiendo la semántica típica de zlib para arrancar en mitad de byte.
- Se añadió `zragf_inflateSync()` con búsqueda del patrón `00 00 FF FF` para reengancharse en un punto de `FULL_FLUSH`; tras un resync se omite la validación final de checksums del wrapper, priorizando recuperación parcial del stream.
- Se añadió `zragf_deflateTune()` y el backend RFC1951 ahora recibe esos parámetros para ajustar lazy probing, `nice_length` y `max_chain` en la tokenización LZ77.
- Se añadió `tests/test_phase8_copy_prime_sync.c`, validando:
  - `deflateCopy()` sobre un stream raw aún no finalizado y comprobando identidad de salida
  - `inflateCopy()` sobre un stream zlib a mitad de descompresión
  - `inflatePrime()` con un raw bit-shifted reconstruido correctamente
  - `inflateSync()` recuperando un tail raw después de basura inicial + marcador de sync


## Phase 9 / Fase 9 – Reconfiguración en caliente + installable package

- Se añadió `zragf_deflateParams()` para reconfigurar `level`/`strategy` a mitad de stream; la función fuerza un `SYNC_FLUSH` interno y devuelve `ZRAGF_BUF_ERROR` si el caller debe drenar salida antes de completar el cambio.
- Se añadió `zragf_inflateValidate()` para activar/desactivar validación de `Adler32`, `CRC32`, `ISIZE` y `FHCRC` sin tocar el parser incremental.
- Se añadió `tests/test_phase9_params_validate.c`, validando interop con `zlib` externo y trailer corruption tolerante cuando la validación está desactivada.
- El proyecto ya instala `libzragflib.a`, `zragflib.h`, docs, `zragflibConfig.cmake` y `zragflib.pc`, y fue verificado con un consumidor externo por `find_package()` y `pkg-config`.


## Phase 10 / Fase 10 – running backend real + cross-chunk history

- El wrapper dejó de codificar bloques no finales como `STORED` por defecto: ahora puede emitir `FIXED`/`DYNAMIC` también durante `NO_FLUSH` y `SYNC_FLUSH`.
- El backend RFC1951 recibió una ruta **stream-aware** que preserva `bitbuf/bitcount` entre bloques comprimidos, evitando el padding ilegal a byte entre bloques no finales.
- Se añadió soporte interno de **history prefix** al tokenizador LZ77, de modo que el compresor puede buscar matches contra un prefijo de hasta 32 KiB sin copiar ese prefijo al output.
- `deflateZ()` ahora mantiene una ventana rodante de historial y la pasa al backend en cada bloque nuevo; eso permite matches entre chunks y hace que el streaming raw por trozos quede cerca del tamaño `one-shot`.
- Las **preset dictionaries** dejaron de ser solo metadata/header: ahora también actúan como historial efectivo en la compresión y reducen tamaño real cuando hay substrings compartidos.
- Se añadió `tests/test_phase10_backend_running.c`, validando ratio razonable en streaming raw, decodificación externa y mejora real con preset dictionary.
- Se añadió `bench/bench_streaming_compare.c` con comparación rápida entre `one-shot raw`, `zragf chunked raw`, `zlib chunked raw` y un caso corto con preset dictionary.


## Phase 11 / Fase 11 – adaptive block splitting + backend consolidation

- Se añadió `zragf_deflate/deflate_plan.c` con un planificador ligero de cortes basado en textura local del input (ventanas de 4 KiB, repetición local y dispersión de bytes).
- El encoder RFC1951 público ya no trata cada chunk como un único bloque forzosamente: ahora puede probar divisiones candidatas y elegir una secuencia de sub-bloques cuando reduce el tamaño total.
- La ruta pública y la wrapper siguen usando el mismo backend principal, pero ahora el subárbol `zragf_deflate/*` sí participa en el camino productivo a través del planificador de bloques.
- Se añadió una ruta diagnóstica interna `compress_chunk_single_*` para comparar “single-block” vs “split-aware” sin tocar la API pública.
- Se endureció la heurística de activación para no castigar datos casi-incompresibles o extremadamente repetitivos: si el bloque ya cae a STORED o ya comprime muy fuerte, el planner no se activa.
- Se añadió `tests/test_phase11_splitter.c`, validando:
  - compatibilidad externa raw con `zlib`
  - mejora real frente al camino single-block sobre un corpus mixto
- Se añadió `bench/bench_phase11_split_compare.c` para medir directamente el delta entre single-block y split-aware.


## Phase 12
- Moved the main RFC1951 matcher/tokenizer into zragf_deflate/deflate_matcher.[ch].
- Public RFC1951 encode path now builds tokens through the shared matcher module for both plain and dictionary-backed blocks.
- Added regression test for dictionary-assisted tokenization and external inflate interop.


## Phase 16

- Added a shared Huffman helper module used by `deflate_emit`, `deflate_cost`, and the legacy `deflate_huffman` wrapper.
- Moved matcher corpus profiling and policy selection into `deflate_core` so the old core layer now owns reusable backend policy instead of only stub logic.
- Hardened the matcher against noisy and alternating corpora with adaptive chain and lazy caps plus cheap candidate prechecks.
- Added `tests/test_phase16_backend_hardening.c`.

## Fase 18

- `inflate` endurecido para metadata gzip patológica: ahora se rechazan cabeceras con `FEXTRA`/`FNAME`/`FCOMMENT` que superen 1 MiB antes de cerrar la cabecera.
- `inflate_core.c` dejó de truncar silenciosamente repeticiones `16/17/18` que se pasaban del total esperado; ahora falla como stream inválido.
- Se compartieron helpers de símbolos longitud/distancia y de preparación de tablas dummy entre `zragf_stream.c` y `zragf_inflate/inflate_core.c`.
- `fuzz/fuzz_inflate.c` reescrito como harness incremental que toca raw/zlib/gzip/auto, `inflateGetHeader()`, `inflateValidate()`, `inflateSetDictionary()` e `inflateSync()`.
- Añadidos `fuzz/inflate.dict`, `fuzz/corpus/inflate/*` y `tests/test_phase18_fuzz_regressions.c`.


## Phase 22
- Added a 4-byte fast reject in the shared matcher to cut 3-byte hash collisions on structured text.
- Made RFC1951 split planning profile-aware so large structured-text buffers skip the expensive exact repartitioning path.
- Added phase22 structured-text guard test and speed benchmark.
- Bumped version to 0.22.0 / zragf/compat-0.22.0.

## Phase 23
- Added an 8-byte fast match extension path in `zragf_deflate/deflate_matcher.c`.
- Added sparse hash insertion across long structured-text matches to cut matcher/tokenizer overhead without changing stream correctness.
- Refined structured-text policy caps in `zragf_deflate/deflate_core.c`.
- Added `tests/test_phase23_fastpath.c` and `bench/bench_phase23_speed_text.c`.
- Bumped version to 0.23.0 / zragf/compat-0.23.0.


## Phase 30
- Whole-buffer hot path: token buffer now reserves more aggressively for large inputs, reducing realloc churn.
- Matcher chain tables now come from one contiguous allocation and initialize with a single memset.
- Generic long-match fast path is enabled only on large non-text buffers to preserve JSON/log ratio wins from phases 21-23.
- Lazy lookahead is skipped only for large-buffer long matches, which cuts matcher cost without disturbing text-heavy tuning.
- Added `test_phase30_wholebuf.c` and `bench_phase30_wholebuf.c`.

## Phase 31
- Shared dynamic Huffman preparation across cost + emit (`zragf_deflate_prepare_dynamic`).
- Added prepared-path helpers for dynamic blocks to avoid rebuilding lengths/codes twice.
- Reused preinitialized FIXED tables in the hot path.
- RFC1951 single-block driver now reuses prepared dynamic metadata for estimate + emit.


## Phase 32
- Replaced per-match length/dist symbol loops with precomputed lookup tables.
- Switched the matcher hot hash to a lighter zlib-style 3-byte rolling hash mix.
- Inlined token/stat updates on the matcher fast path to lower function-call overhead in large dynamic blocks.
- Added phase32 matcher/token regression test and benchmark.


## Phase 39
- Reworked recursive partition search to propagate partial prefix state (segment start, accumulated size, bitcount) instead of re-estimating the full prefix at every branch.
- Preserved prepared-segment cache and winner emission from phase 38.
- Added `tests/test_phase39_partition_state.c` and `bench/bench_phase39_partition_state.c`.
- Bumped project version to 0.39.0 and `zragf_version()` to `zragf/compat-0.39.0`.


## Phase 40

- Added suffix/partition memoization to the RFC1951 partition search.
- Added prepared-segment donor reuse across bitcount/final-state estimate variants.
- Raised prepared-segment retention ceiling modestly with a token-count guard.
- Added `tests/test_phase40_dual_cache.c` and `bench/bench_phase40_dual_cache.c`.
- Bumped version to 0.40.0 / `zragf/compat-0.40.0`.


## Phase 41

- Added `header_bits` to `zragf_deflate_dynamic_prepared` so dynamic-cost estimation no longer re-walks the header RLE on every prepared-path cost check.
- Added fast length/distance descriptor lookup tables to `deflate_cost.c` and `deflate_emit.c`, removing repeated linear symbol mapping in the hot prepared dynamic path.
- Added `tests/test_phase41_winner_dynamic.c` and `bench/bench_phase41_winner_dynamic.c`.
- Bumped version to `0.41.0` / `zragf/compat-0.41.0`.


## Phase 42

- Tightened the whole-buffer public path so an exact estimate-cache hit can emit from a prepared donor segment instead of rebuilding matcher/tokens/stats.
- Added cloning of donor prepared segments when an exact cache entry is materialized from donor-based estimation, so later exact hits can reuse prepared state directly.
- Added `tests/test_phase42_public_emit_cache.c` and `bench/bench_phase42_public_emit_cache.c`.
- Bumped version to `0.43.0` / `zragf/compat-0.43.0`.


## Phase 44

- Expanded suffix-tail cache reuse so cached tails can be reused under a stricter `next_index` when the chosen split offsets still satisfy the tighter tail constraints.
- Added an early cached-tail check inside the recursive partition search to skip descent when the cached tail cannot beat the current best total.
- Increased `ZRAGF_RFC_SUFFIX_CACHE_CAP` from `96` to `128`.
- Preserved the mixed-corpus target `65698` while shaving a modest amount of search time off the split-aware path.

## Phase 53
- Added a direct exact-tail query cache in the partition search.
- Promoted exact-tail lookups into that query cache to avoid repeated scans.

## Phase 55
- Added a direct exact-query cache for suffix partition results in `zragf_deflate_rfc1951.c`.
- Seeded the suffix query cache from exact lookups and stores, reducing repeated linear scans of the main suffix cache.
- Added `test_phase55_suffix_query_cache` and `bench_phase55_suffix_query_cache`.
- Bumped project version to `0.55.0` and `zragf_version()` to `zragf/compat-0.55.0`.


Phase 56
--------

- Added slot-hint caches for exact lookups in the estimate, suffix, suffix-lower-bound, and suffix-exact caches.
- Seeded hint caches from both store paths and successful linear fallback lookups.
- Added `tests/test_phase56_slot_hint_maps.c` and `bench/bench_phase56_slot_hint_maps.c`.
- Bumped project version to `0.56.0` and `zragf_version()` to `zragf/compat-0.56.0`.

## Phase 57
- Added a prepared-donor slot-hint map for estimate-cache segment reuse, keyed by `(start,end)` so repeated prepared lookups avoid a linear scan of the estimate cache.
- Seeded the prepared-donor hint map from both exact stores and successful fallback scans.
- Added `tests/test_phase57_prepared_donor_hints.c` and `bench/bench_phase57_prepared_donor_hints.c`.
- Bumped project version to `0.57.0` and `zragf_version()` to `zragf/compat-0.57.0`.


## Phase 58
- Added a generation-scoped miss-hint cache for prepared-donor lookups in `zragf_deflate_rfc1951.c` so repeated exact misses can skip linear scans until the estimate cache mutates.
- Added `tests/test_phase58_prepared_donor_miss_hints.c` and `bench/bench_phase58_prepared_donor_miss_hints.c`.
- Bumped project version to `0.58.0` and `zragf_version()` to `zragf/compat-0.58.0`.


## Phase 59

- Upgraded exact suffix/lower-bound/exact-tail query caches to a 2-choice lookup/store policy to reduce direct-mapped collisions in partition-search hot paths.
- Increased query cache capacities for suffix, lower-bound, and exact-tail query tables.
- Added `tests/test_phase59_query_assoc.c` and `bench/bench_phase59_query_assoc.c`.
- Bumped project version to `0.59.0` and `zragf_version()` to `zragf/compat-0.59.0`.
- Added phase 60: two-choice associative slot-hint maps for estimate/prepared/suffix/lb/exact lookups, preserving the 65698-byte mixed-corpus split result while reducing the local phase59 query-assoc bench from ~33.50 ms to ~31.00 ms on the same harness.


## Phase 61
- Added generation-scoped miss-hint caches for suffix, suffix-lower-bound, and suffix-exact cache lookups.
- Incremented per-cache generations on store so repeated exact misses can skip linear rescans safely.
- Added phase61 miss-path regression test and bench.


## Phase 62
- Added 2-choice generation-scoped miss hints for prepared-donor misses in estimate-cache donor lookup.
- Added bound-aware suffix recursion with stricter tail pruning using cached lower bounds and parent upper bounds.
- Added early donor impossibility check for segments larger than the prepared-segment cap.

## Phase 63
- Added exact prepared-donor query cache keyed by `(start,end)` with generation tracking.
- Seed query cache from prepared donor stores, slot-hint hits, and successful linear scans.
- Preserved mixed-corpus split ratio target (`65698`) while trimming repeated donor lookups.

## Phase 64

- Added configurable global allocator hooks for native/one-shot paths.
- Routed default stream allocators through the same global allocator backend.
- Added native fixed-workspace APIs for one-shot compress/decompress.
- Added public helper APIs: `zragf_strerror()`, `zragf_build_config()`, `zragf_compressBound()`, `zragf_compress2()`, `zragf_uncompress()`, and `zragf_inspect_wrapper()`.
- Added roadmap status doc and a minimal workspace example.
- Bumped project version to `0.64.0` and `zragf_version()` to `zragf/compat-0.64.0`.


## Phase 66
- Added opt-in `ZRAGF_ENABLE_C89=ON` support in CMake.
- Removed remaining library-side `inline` / `stdint.h` dependencies that blocked a strict C89 build.
- Added `examples/example_c89_simple.c` and `tests/test_phase66_c89_smoke.c`.
- Updated `zragf_build_config()` to report the new C89 opt-in status.


## Protocol89 sanitation pass

- Forced the maintained build to C90/C89.
- Replaced ZRAGF-owned runtime dynamic storage with static-arena/workspace storage.
- Converted benchmark binary-FP timing/ratio fields to integer fixed-scale metrics.
- Preserved all original files and infrastructure; added a 297-file preservation manifest.
- Added source-policy audit and byte-exact golden-master CTest coverage.
- Golden comparison: 122165 bytes, SHA-256 `13e5b87531e593d66dfb00db14b85ce0dc25a46990c7c91256b9dc5b5eaa0107`, exact match against the pre-sanitation library.
