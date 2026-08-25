# giffywebp-c89

Proyecto modular en **C89**, sin `malloc` dentro del core de decode/mux/demux, con **scratch arena** provista por el llamador, inspirado en la separación de componentes de `libwebp`.

## Qué incluye

- Parser/demux del contenedor RIFF/WebP (`VP8X`, `VP8L`, `VP8 `, `ALPH`, `ANIM`, `ANMF`, `ICCP`, `EXIF`, `XMP `).
- Mux de contenedor para imágenes estáticas WebP.
- Decoder **VP8L** (lossless) orientado a memoria estática.
- Decoder **VP8 lossy still** con:
  - bool decoder modular estilo RFC
  - parseo del control header y entropy header
  - decode de modos y coeficientes
  - inverse WHT / DCT enteras
  - predicción 16x16 / 8x8 / 4x4
  - reconstrucción completa de frame Y/U/V
  - in-loop filter sobre frame reconstruido
  - write-back a `RGBA/BGRA/ARGB`
  - soporte `ALPH + VP8 `
- **Phase 7**: harness de conformance con oracle `dwebp`, corpus oficial fijado, dif por planos y triage.
- **Phase 8**: decode/composición de **WEBP animado** (`ANIM`/`ANMF`) + harness de conformance animado.
- **Phase 9**: mux/encoder animado sin `malloc` en el core, orientado a ensamblar animaciones a partir de frames `VP8` / `VP8L` ya codificados o de still WebP de un solo frame.
- **Phase 10**: encode lossless desde `RGBA/BGRA/ARGB` crudo y ruta animada raw-frame con bounding-box diff, colapso de frames idénticos y keyframes completos simples.
- **Phase 11**: compresión VP8L mejorada (`near_lossless`, `subtract-green`, color-cache, backrefs limitados), bridge opcional a `cwebp` para still lossy y mixed mode por frame cuando el toolchain oficial está disponible.
- **Phase 12**: planner nativo de VP8 lossy por macroblock, bridge still/anim más alineado con presets y knobs oficiales, y scheduler animado corregido para que `kmax` gobierne la inserción de keyframes en vez de forzar all-keyframes cuando `minimize_size` está apagado.

## Ejemplos

- `gwpinfo`        -> inspección del contenedor WebP
- `gwpdecode`      -> decodifica VP8L y VP8 lossy estático a PAM RGBA
- `gwpvp8probe`    -> inspecciona frame/control/entropy/modes/residual summary VP8 lossy
- `gwpvp8kernels`  -> smoke test de kernels IDCT/WHT/recon/filter
- `gwpdumpyuv`     -> vuelca la salida YUV420 plana real de la ruta VP8
- `gwpanimdump`    -> decodifica WebP animado a secuencia PAM + JSONL de tiempos
- `gwpanimux`      -> arma WebP animado desde un manifest TSV de still WebP ya codificados
- `gwpencode`      -> codifica still WebP (VP8L interno; VP8 lossy opcional vía `cwebp`) desde PAM RGBA
- `gwpanimframes`  -> codifica WebP animado desde una secuencia de PAM RGBA; mixed opcional con backend oficial
- `gwpvp8plan`     -> analiza una imagen cruda y propone un plan lossy por macroblock
- `gwpvp8emit`     -> construye la IR intra-only nativa y puede emitir un proxy lossy sin tools externos

## Build

```sh
make
```

## Uso rápido

```sh
./examples/gwpinfo image.webp
./examples/gwpdecode image.webp out.pam
./examples/gwpvp8probe lossy.webp
./examples/gwpdumpyuv lossy.webp out.yuv
./examples/gwpanimdump anim.webp out/frame
```

## Phase 7 / conformance

```sh
./tests/conformance/fetch_libwebp_testdata.sh ./.cache/libwebp-test-data
make
python tests/conformance/run_oracle.py \
  --oracle dwebp \
  --dwebp /usr/local/bin/dwebp \
  --corpus-dir ./.cache/libwebp-test-data \
  --manifest tests/conformance/manifests/still_lossy_yuv.txt \
  --manifest tests/conformance/manifests/still_alpha_pam.txt \
  --manifest tests/conformance/manifests/lossless_pam.txt \
  --out-dir ./.cache/conformance
python tests/conformance/report_failures.py ./.cache/conformance/results.jsonl
```

## Límites prácticos

- El árbol ya incluye decoder VP8L utilizable y decoder VP8 still funcional.
- La ruta VP8 lossy sigue en modo **best-effort serio**, no prometida como bit-exact total.
- Phase 7 añade la infraestructura para encontrar exactamente dónde diverge frente a `libwebp`.
- Los ejemplos (`examples/`) pueden usar `malloc`; la restricción fuerte de **sin heap** aplica al core (`src/`).

## Phase 8 / animación

```sh
make
python tests/test_vp8_phase8.py
python tests/conformance/run_anim_oracle.py \
  --corpus-dir ./.cache/libwebp-test-data \
  --manifest tests/conformance/manifests/animated_pam.txt \
  --out-dir ./.cache/conformance-anim \
  --mean-threshold 1 \
  --max-threshold 1
```

## Phase 9 / anim mux + encoder

```sh
make
./examples/gwpanimux out.webp 640 480 0 0x00000000 frames.tsv
python tests/test_vp8_phase9.py
```

`frames.tsv` usa este formato por línea:

```text
path\tduration_ms\tx\ty\tblend\tdispose
```

Notas:

- el core sigue sin `malloc`; las entradas quedan referenciadas por puntero hasta `Assemble()`
- los offsets impares se ajustan a par por defecto, en la misma dirección práctica del Mux API oficial
- esta fase arma la animación a partir de **frames ya codificados** (`VP8` / `VP8L` o still WebP); la compresión interna desde RGBA crudo queda para una fase posterior

## Phase 10 / raw-frame encode

```sh
make
./examples/gwpencode in.pam out.webp
./examples/gwpanimframes out_anim.webp f0.pam f1.pam f2.pam
python tests/test_vp8_phase10.py
```

Notas:

- la ruta cruda base es **lossless / VP8L**
- el anim encoder usa un canvas RGBA en `work_mem` y recorta la bounding-box mínima de diferencias

## Phase 11 / encode upgrades

```sh
make
./examples/gwpencode --method 4 --near-lossless 80 in.pam out.webp
./examples/gwpencode --lossy --allow-tools --cwebp /usr/bin/cwebp in.pam out_lossy.webp
./examples/gwpanimframes --mixed --allow-tools --cwebp /usr/bin/cwebp out_anim.webp f0.pam f1.pam f2.pam
python tests/test_vp8_phase11.py
```

Notas:

- el encoder interno mejoró la compresión VP8L con `near_lossless`, `subtract-green`, color cache y backrefs limitados
- el encode **lossy** sigue siendo opcional y usa el backend oficial `cwebp` cuando está disponible
- `tests/conformance/run_encode_oracle.py` y `run_anim_encode_oracle.py` comparan el encode propio contra `cwebp`, `img2webp` y `gif2webp`


## Phase 12 / native lossy planning + official knobs

```sh
make
./examples/gwpvp8plan --quality 72 --preset photo --dump-mbs in.pam
./examples/gwpencode --lossy --allow-tools --cwebp /usr/bin/cwebp --preset photo --filter-strength 40 in.pam out.webp
./examples/gwpanimframes --no-min-size --kmin 3 --kmax 5 --mixed --allow-tools --cwebp /usr/bin/cwebp out_anim.webp f0.pam f1.pam
python tests/test_vp8_phase12.py
python tests/conformance/discover_official_tools.py
```

Notas:

- `gwpvp8plan` no emite todavía bitstream VP8; deja listo el análisis por macroblock que hace falta para cerrar un encoder lossy nativo.
- `gwpencode` y `gwpanimframes` reflejan mejor knobs de las herramientas oficiales: `preset`, `alpha-q`, `filter-strength`, `sharp-yuv`, `kmin`, `kmax`, `loop` y `min-size`.
- el scheduler animado ya no convierte todos los frames en keyframes cuando `--no-min-size` está activo; ahora manda `kmax`, salvo primer frame/canvas inválido.


## Phase 13 / native intra-only emitter foundation

```sh
make
./examples/gwpvp8emit --quality 58 --preset photo --dump-json in.pam
./examples/gwpvp8emit --proxy-webp out.webp in.pam
./examples/gwpencode --lossy in.pam out.webp
./examples/gwpanimframes --lossy out_anim.webp f0.pam f1.pam f2.pam
python tests/test_vp8_phase13.py
```

Notas:

- esta fase sube el planner a una **IR intra-only** por celdas (`16x16` / `8x8` / `4x4`) con `qindex` heurístico.
- la salida utilizable sin `cwebp` es un **native lossy proxy**; sigue siendo WebP válido, pero todavía no un bitstream `VP8 ` emitido íntegramente por el encoder propio.
- still y animación ya tienen una ruta lossy **sin tools externos**.


## Phase 14
Native VP8 bool/tree/token writer landed for opaque lossy still paths, with a narrow intra-only subset, plus encode-conformance scaffolding against official tools.
