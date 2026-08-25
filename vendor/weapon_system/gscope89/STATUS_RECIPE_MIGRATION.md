# Recipe/provider migration status v0.4

Implemented and validated:

- Recursive INI composition remains the single assembly model.
- All 192 vector reticles remain external formal INI presets and assemblies.
- 21 shared geometry fragments are reused by 54 presets.
- Legacy IDs 0..191 remain ABI aliases; catalogs remain runtime-authoritative.
- Provider-aware domains now include recipe, preset, vector, primitive, raster,
  bars, paint, HUD, telemetry, asset, zoom and **animation**.
- Provider ABI is now 2; animation can be internal, `auto:<provider>`, or
  `external:<provider>`.
- `gscopeanim89` loads animation sets from the same resolved HUD recipe.
- Golden-safe default animation set has no always-on clips.
- Event recipes included: fire scale/recovery, aim enter/exit, damage shake.
- Optional `reticle_lively` adds an always-on breath drift.

Golden validation:

- 192 legacy presets regenerated from the frozen pre-INI source.
- 192 recipe presets regenerated independently.
- draw-command binary comparison: 192/192 byte-identical.
- final fixed-point PPM comparison: 192/192 byte-identical.
- visual failures: 0.
- command failures: 0.
- ordered PPM SHA-256 (both): `a49d90b6279441c97fd3a31c8c5935e9131833626e31e0277600a2ac9ff00b95`.
- ordered command SHA-256 (both): `230f2a698976ca2f4ae6c83445d795c4d14e220859ff01ac4b1d7f4413496cb7`.

Runtime validation:

- 192 presets / 3,953 shapes / 5,388 static draw commands remain valid.
- fire animation smoke: scale rises above identity and settles to exactly 1000.
- provider smoke: animation provider sees the batch, returns fallback, and the
  bundled fixed-point animator completes it; primitive/paint/zoom fallback still works.
- `make test` passes with `-std=c89 -pedantic -Wall -Wextra -Werror`.
- new animation/provider/bundle runtime code contains no malloc/calloc/realloc/free
  calls and no float/double types.
