# v1.1 air, EQ and reverb tuning

## Slower air-body attack

The white-noise body now uses attacks from 12 to 26 ms instead of 4 to 12 ms. The ballistic crack remains independent of the ADSR, so supersonic presets retain a sharp transient while the aerodynamic layer swells behind it.

## Six-band fixed-point equalizer

The EQ is implemented as five parallel one-pole low-pass memories followed by subtraction into six adjacent bands. Approximate crossover regions at 48 kHz are centered around 200 Hz, 500 Hz, 1.2 kHz, 3 kHz and 7 kHz.

All gains use Q12:

- 4096 = 1.0x
- 2048 = 0.5x
- 6144 = 1.5x

Most presets reduce sub/low energy and raise presence and air. `HEAVY_ROUND_PASS` retains more low-mid body, while `GAME_WHIZZ` has the strongest presence boost.

## Reverb tail correction

Version 1.0 stopped the voice when the ADSR ended, also stopping the reflection buffer. Version 1.1 separates `source_samples` from `total_samples`. The source falls silent after the ADSR, but chorus and reverb keep rendering for `reverb_tail_ms`.

The wet calculation also uses a true Q15 multiply in v1.1. In v1.0 it was shifted by 16 bits, effectively halving the configured wet level.
