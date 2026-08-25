# gcrosshair89 animation runtime upgrade

Blank3D now vendors `gcrosshair89` runtime ABI 2, including `gcrosshair_anim89`.

## Runtime path

```text
weapon INI
  -> crosshair preset name
  -> config/crosshair preset INI
  -> reusable animations/*.ini
  -> gcrosshair_runtime89 ABI 2
  -> gcrosshair_anim89
  -> params/core/provider89
  -> external provider when installed
  -> Blank3D OpenGL primitive fallback otherwise
```

`blank3d_crosshair_draw()` no longer manually calls the legacy core animation
triggers on AIM/FIRE/HIT. The ABI-2 runtime owns input edges and executes the
recipe event selected by the loaded preset.

Blank3D converts real `frame_ms` to a fixed integer animation clock at roughly
60 Hz (`B3D_CROSSHAIR_ANIM_TICK_MS = 16`). This makes compact recipe timings
such as `attack_ticks=2` visible and frame-rate independent without float/double.

## Blank3D compatibility pistol

The old first-person and third-person four-line crosshairs remain project-local
presets 192 and 193. Their neutral geometry is unchanged. Both include
`animations/blank3d_classic.ini`, so FIRE/HIT/AIM and CUSTOM1/CUSTOM2 are now
fully declarative while preserving the classic neutral look.

Manual events are available through `blank3d_crosshair_trigger_event()` and
current animation state through `blank3d_crosshair_animation_modifier()`.
