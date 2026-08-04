# ggunmach89 API

## Initialization

### `ggm89_config_preset`

```c
void ggm89_config_preset(ggm89_config *config, int preset_id);
```

Fills a configuration with one of the built-in presets. The resulting
structure may be edited before initialization.

### `ggm89_init`

```c
int ggm89_init(ggm89_state *state,
               const ggm89_config *config,
               ggm89_u32 sample_rate);
```

Clears and initializes caller-owned state. Returns nonzero on success.

Accepted sample-rate range: 8000 through 96000 Hz.

Validation rejects zero barrel counts, more than 16 barrels, and nominal
rates outside 60 through 12000 shots per minute.

### `ggm89_reset`

Reinitializes an existing state while preserving its configuration and
sample rate.

## Transport and load

### `ggm89_start`

Targets full rotor speed and enters spin-up.

### `ggm89_stop`

Targets zero speed and enters spin-down.

### `ggm89_set_target_speed_q15`

Sets a normalized target speed from 0 through 32767.

### `ggm89_set_firing_load_q15`

Sets mechanical load from 0 through 32767. Higher values:

- strengthen cam/bolt transients;
- add a small configured RPM dip;
- make the mechanism sound denser.

It does not synthesize a muzzle blast.

## Tone controls

### `ggm89_set_eq_gain_q12`

Sets one of six EQ band gains. Values are clamped from 0 through 8192,
representing 0.0 through 2.0.

Approximate band splits:

```text
Band 0: below 120 Hz
Band 1: 120-360 Hz
Band 2: 360-1000 Hz
Band 3: 1.0-2.8 kHz
Band 4: 2.8-7.0 kHz
Band 5: above 7.0 kHz
```

The EQ is implemented by parallel differences of fixed-point one-pole
low-pass states. It is intentionally inexpensive rather than a mastering
equalizer.

### `ggm89_set_distortion_drive_q12`

Sets pre-drive from 1024 through 12288. Unity is 4096.

### `ggm89_set_reverb_q15`

Sets wet mix and feedback. Both values are internally clamped to safe
ranges.

## Rendering

### `ggm89_render_mono`

```c
void ggm89_render_mono(ggm89_state *state,
                       ggm89_s16 *output,
                       ggm89_u16 frame_count);
```

Writes signed 16-bit mono PCM into a caller-provided buffer.

The function performs no allocation and has no hidden global mutable state.
Separate `ggm89_state` values can be rendered independently.

## Queries

- `ggm89_get_mode`
- `ggm89_get_current_rpm`
- `ggm89_state_size_bytes`
- `ggm89_preset_name`
- `ggm89_version_string`

## State modes

```text
GGM89_STATE_OFF
GGM89_STATE_SPINUP
GGM89_STATE_RUNNING
GGM89_STATE_SPINDOWN
```
