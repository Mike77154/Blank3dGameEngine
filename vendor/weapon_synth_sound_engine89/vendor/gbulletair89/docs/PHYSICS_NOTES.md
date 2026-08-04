# Physics basis

`gbulletair89` deliberately separates two audible mechanisms:

1. **Aerodynamic fly-by / whiz**: band-limited white noise shaped by a short A-D-S-R one-shot envelope. The high-pass and low-pass coefficients sweep during the event to imply changing angle, distance and Doppler-like spectral motion.
2. **Supersonic crack**: a very short bipolar N-shaped pressure transient. Published gunshot-acoustics summaries describe a rapid positive overpressure, a ramp toward negative pressure and an abrupt return, with a typical intershock interval below 200 microseconds for a bullet a few centimetres long.

The crack is not synthesized with a slow ADSR. It is generated directly as 5 to 10 samples at 48 kHz, while the surrounding noise layer uses the requested regular attack and short release.

## Signal path

```text
xorshift white noise
        |
        +--> slow random flutter
        |
        +--> swept high-pass --> two-pole swept low-pass --> ADSR
                                                        |
short N-wave impulse -----------------------------------+
                                                        |
                                              fixed soft clip
                                                        |
                                              low-mix chorus
                                                        |
                                      stereo trajectory panning
                                                        |
                                         short early reflections
```

## Why the effects are restrained

A strong chorus creates a repeating comb-filter pitch and makes a projectile sound like a sci-fi laser. Long reverb also smears the tiny shock transient. The presets therefore use low chorus wet levels and short reflection taps; the indoor preset is intentionally the wettest.

## Fixed-point format

- Audio and coefficients: signed Q15.
- Distortion drive: unsigned Q8.8.
- All delay storage: signed 16-bit samples.
- No dynamic allocation and no floating-point arithmetic.
