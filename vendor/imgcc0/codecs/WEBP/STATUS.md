# Estado rápido

## Listo

- Layout modular tipo librería (`webp/`, `dec/`, `demux/`, `mux/`, `utils/`, `enc/`).
- Parser RIFF/WebP.
- Demux de chunks extendidos y metadata.
- Mux básico de contenedor estático.
- Ruta VP8L integrada con arena estática y sin `malloc` propio.
- Ruta VP8 still con reconstruct + filter + packed output.
- Soporte `ALPH + VP8 `.
- **Phase 7**: corpus fijado, oracle `dwebp`, dif por planos y triage.
- **Phase 8**: decode/composición de animación (`ANIM`/`ANMF`).
- **Phase 9**: mux/assembler animado desde still WebP o bitstreams ya codificados.
- **Phase 10**: encode lossless raw-frame (`VP8L`) para still y animación.
- **Phase 11**:
  - mejoras reales de compresión VP8L (`near_lossless`, `subtract-green`, color cache, backrefs limitados)
  - bridge opcional a `cwebp` para still **lossy** desde píxel crudo cuando el toolchain oficial está instalado
  - mixed mode por frame en animación cuando `allow_mixed` + backend oficial están presentes
  - harness de conformance de encode contra `cwebp`, `img2webp` y `gif2webp`
- **Phase 12**:
  - planner nativo de VP8 lossy (`GWPAnalyzeVP8LossyPlan`) con stats por macroblock y sugerencias de `segments` / `partitions` / `filter_strength` / `sharpness`
  - ejemplos still/anim con presets y knobs más cercanos a `cwebp` / `img2webp` / `gif2webp`
  - corrección del scheduler animado para que `--no-min-size` no fuerce keyframes para cada frame
- **Phase 13**:
  - IR intra-only nativa (`GWPBuildVP8IntraEmitPlan`) construida sobre el planner
  - ruta `gwpencode --lossy` usable sin `cwebp` mediante un **native lossy proxy**
  - ruta `gwpanimframes --lossy` usable sin toolchain externo

## Pendiente / en validación

- Cerrar un encoder **VP8 lossy** autónomo, sin depender del backend oficial.
- Cerrar la última milla del emisor nativo hacia un bitstream `VP8 ` real (bool encoder + headers + token stream).
- Añadir transforms VP8L más avanzados (predictor/color transform/color-index) para bajar más tamaño.


## Phase 14
Native VP8 bool/tree/token writer landed for opaque lossy still paths, with a narrow intra-only subset, plus encode-conformance scaffolding against official tools.


## Phase 15
- native `VP8 ` writer upgraded with:
  - auto/forced token partitions (`1/2/4/8`)
  - segmentation (`1..4`) with quant/filter deltas
  - `skip_coeff` signalling
  - coefficient probability updates from a pre-pass histogram
  - richer 4x4 B_PRED mode search and UV mode search
- conformance harness now compares phase15 native, legacy-native compatibility mode, and official tools when available

## Aún pendiente
- intra16/Y2 path on the native writer side
- non-DC residual coding on the native writer side
- closer RD parity with full `cwebp`


## Phase 16
- native writer now adds a real intra16/Y2 path for smooth macroblocks
- sparse AC residual coding landed for native luma/chroma block decisions
- phase16 regression coverage checks intra16 activation, Y2 signalling and lossy round-trip validity
