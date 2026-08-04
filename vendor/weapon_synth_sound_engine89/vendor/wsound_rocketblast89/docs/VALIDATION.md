# Validation report

Release candidate: `wsound_rocketblast89 1.3.0`

## Compiler and runtime checks

- strict `-std=c89 -pedantic -Wall -Wextra -Werror`: passed
- all seven presets at 8,000, 22,050, 44,100 and 48,000 Hz: passed
- deterministic three-noise versus five-noise checksum test: passed
- AddressSanitizer and UndefinedBehaviorSanitizer with halt-on-error: passed
- i386 32-bit object compilation for all core translation units: passed
- banned allocation and floating-point token audit in public/core code: passed
- no C99 line comments in public/core code: passed
- signed SVF overflow found during v1.3 development: fixed and revalidated

The Windows batch file is included, but Windows linking was not performed in the Linux validation container.

## Static sizes

- workspace: 48,004 bytes
- context: 500 bytes in the tested i386 object ABI
- context: 504 bytes in the tested 64-bit executable ABI
- total independent i386 voice state: 48,504 bytes (about 47.4 KiB)

## WAV output

All generated previews are 44,100 Hz, stereo, signed 16-bit PCM. No generated sample reached signed full scale.

| File | Duration | Peak absolute sample | Full-scale samples |
|---|---:|---:|---:|
| `wsrb89_ab_3noise_vs_5noise.wav` | 8.50 s | 24,575 | 0 |
| `wsrb89_ab_v1_1_funny_vs_v1_2_massive.wav` | 8.50 s | 24,575 | 0 |
| `wsrb89_ab_v1_2_dry_vs_v1_3_monumental.wav` | 8.50 s | 24,575 | 0 |
| `wsrb89_airburst.wav` | 4.00 s | 18,947 | 0 |
| `wsrb89_all_presets.wav` | 22.75 s | 24,575 | 0 |
| `wsrb89_compact_rpg.wav` | 4.00 s | 18,621 | 0 |
| `wsrb89_concrete_paas.wav` | 4.00 s | 24,539 | 0 |
| `wsrb89_distant_paas.wav` | 4.00 s | 24,575 | 0 |
| `wsrb89_heavy_impact.wav` | 4.00 s | 24,575 | 0 |
| `wsrb89_indoor_bunker.wav` | 4.00 s | 24,575 | 0 |
| `wsrb89_metal_strike.wav` | 4.00 s | 22,682 | 0 |

## Measured Heavy Impact change from v1.2

The first 50 ms is slightly less dominant, while the body and spatial field remain much stronger from 0.2 to 2.5 seconds:

| Window | v1.2 RMS | v1.3 RMS | Change |
|---|---:|---:|---:|
| 0.2-0.5 s | -21.21 dBFS | -13.75 dBFS | +7.46 dB |
| 0.5-1.0 s | -44.84 dBFS | -22.88 dBFS | +21.96 dB |
| 1.0-1.5 s | -49.55 dBFS | -31.32 dBFS | +18.22 dB |
| 1.5-2.5 s | -49.21 dBFS | -39.44 dBFS | +9.77 dB |

In a 20-500 ms spectral analysis, v1.3 moved more energy into 40-80 Hz and reduced the 5.12-10 kHz fraction by about 11.3 dB relative to v1.2. This is a digital comparison of the generated waveforms, not a calibrated SPL or weapon measurement.

## Compatibility

- the public `wsrb89_params` layout remains compatible with v1.2
- `wsrb89_context` and `wsrb89_workspace` layouts changed; recompile integrations against the v1.3 header
- all buffers remain caller-owned and statically sized
