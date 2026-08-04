# Blank3D v3.16.0 — Dynamic ECG combat HUD

`ecg_embedded_c89_v1_2_0_dynamic_hud` is now vendored and connected to the
actual player state.

## Runtime behavior

- HP 75–100: `FINE` green waveform.
- HP 50–74: `CAUTION` yellow waveform.
- HP 25–49: `ORANGE` waveform.
- HP 1–24: `DANGER` red waveform.
- HP 0: custom `FLATLINE` profile.
- Nearby living enemies raise the displayed threat percentage and pulse rate.
- Enemy projectiles near the player also raise threat.
- Receiving damage forces threat to 100 and flashes a red panel outline for
  520 milliseconds.
- The trace scrolls continuously by rotating the fixed 80-sample profiles.
- The previous segmented ammunition GunBar remains directly below the monitor.

## Rendering bridge

The ECG library renders into a caller-owned static 360 x 112 RGB framebuffer.
Blank3D blits it through the OpenGL 1.1 `glDrawPixels` path with unpack
alignment set to one byte. No texture allocation, heap allocation or platform
window dependency was added to the ECG library.

## Source integration

- `src/blank3d_ecg_vitals.c`
- `src/blank3d_ecg_vitals.h`
- `src/blank3d_hud.c`
- `src/blank3d_hud.h`
- `src/monika_blank3d.c`
- `vendor/ecg_embedded_c89/`
- `tests/test_ecg_vitals.c`

## Validation

```bash
make test-ecg-vitals
make test-motion-attacks
make test-motion-q16-win32
make syntax-check
make audit
```
