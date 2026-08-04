# Tuning guide

## Make it more monumental

Increase scale in this order:

1. `reverb_mix_q15` moderately
2. `release_ms`
3. EQ band 0 (80 Hz)
4. EQ band 1 (180 Hz)
5. `reverb_feedback_q15`
6. low companion level

The internal early-reflection pattern follows the reverb mix, so a moderate increase enlarges both the near field and the tail. Do not start by increasing distortion or the clean sine level.

## Make it less dry without losing impact

- keep `shock_level_q15` high enough for the direct front
- raise `reverb_mix_q15` before raising feedback
- lengthen `release_ms` before making the sine louder
- use high `reverb_damp_q15` for a dark tail
- keep chorus mix low; it is now a room-send thickener, not a direct effect

## Remove the comic or toy-like character

- cut EQ band 4 (3.2 kHz)
- cut EQ band 5 (7.2 kHz) more strongly
- reduce `WSRB89_NOISE_BODY_HIGH_OCTAVE`
- reduce `saw_level_q15`
- keep `sub_pitch_drop_ms` long enough to glide, but not so long that C1 remains audible as a note
- increase low companion/body ratio instead of adding distortion

## Make the initial `PAAAAS` harder

- raise `shock_level_q15`
- raise `noise_level_q15[WSRB89_NOISE_CRACK]`
- shorten `crack_decay_ms`
- add a small amount of 1.2-3.2 kHz only if the hit becomes too dull
- avoid boosting 7.2 kHz for a monumental preset

## Shape the seven-noise stack

- raise `WSRB89_NOISE_BODY` for the core roar
- raise `WSRB89_NOISE_BODY_LOW_OCTAVE` for mass, distance and enclosed pressure
- raise `WSRB89_NOISE_BODY_HIGH_OCTAVE` only for brief fracture or close-range bite
- raise `WSRB89_NOISE_DEBRIS` for impact material
- keep both companions below the central body unless a deliberately exaggerated effect is wanted
- raise `WSRB89_NOISE_RUMBLE` for a non-tonal low-frequency aftermath
- use `rumble_cutoff_hz` and `rumble_decay_ms` to control weight and duration
- raise `WSRB89_NOISE_CRACKLE` for sparse fracture and debris detail
- use `crackle_cutoff_hz` and `crackle_decay_ms` to control bite and persistence
- use `motion_lfo_rate_millihz` and `motion_lfo_depth_q15` for subtle rumble movement
- set rumble and crackle to zero to recover the v1.3 five-noise topology
- set both spectral companions plus rumble/crackle to zero to recover the original three-noise topology

The octave companions are independent deterministic noise sources. They do not reuse or resample the central body stream.

## Make concrete

- raise debris level and decay
- keep 450 Hz present for body
- use a restrained 1.2-3.2 kHz bite
- retain a dark room response

## Make metal

- raise the saw layer only slightly
- add 1.2 and 3.2 kHz, but keep 7.2 kHz controlled
- use a little more room-send chorus
- shorten the body while leaving sparse debris longer

## Make it distant

- lower `svf_cutoff_hz`
- reduce the top two EQ bands
- lengthen attack from 1 ms to roughly 3-8 ms
- increase dark reverb moderately
- lower direct shock level

## Make it indoor

- increase reverb mix and feedback
- increase damping to darken repeated reflections
- emphasize 80-180 Hz
- reduce 1.2 kHz if the enclosure becomes boxy or nasal

## Rumble, crackle and LFO

The rumble and crackle each have an independent state-variable filter:

- rumble always uses the low-pass output;
- crackle always uses the band-pass output;
- their cutoff and damping parameters are independent from the main body/transient filters.

The motion LFO is a deterministic sine table oscillator. It applies a downward-only gain movement to the rumble, avoiding positive gain overshoot and preserving limiter headroom. Set `motion_lfo_depth_q15` to zero for a static rumble.

## SVF stability

The public setter clamps the cutoff-derived coefficient and damping. The integrator also clamps its internal high state before the Q15 multiply, preventing signed 32-bit overflow at the supported sample rates.

## Polyphony

Each independent v1.3 voice owns approximately 47.4 KiB on i386. A practical static strategy is:

```text
2-4 close-impact voices  -> full workspace each
2-4 distant voices       -> full workspace or shared external room bus
extra impacts            -> steal the quietest tail
```

For large weapon systems, a future split between dry synthesis and scene-level reverb can let many impact voices share one environment processor.
