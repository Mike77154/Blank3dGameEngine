# ggatlingwhistle89 v1.5

Three-event fixed-point whistle/air-cut synthesizer for a rotary gun:

1. `GGW89_EVENT_SPIN_UP`
2. `GGW89_EVENT_FIRE_LOOP`
3. `GGW89_EVENT_SPIN_DOWN`

Version 1.5 adds an indefinite sustained firing loop centered on **A6 =
1760 Hz**. The ascending transition now ends at A6 and the descending
transition begins at A6, so all three events share the same pitch anchor.

## Sound design

The dominant layer remains the clean central sine introduced in v1.4. It is
surrounded by quieter aerodynamic and mechanical texture rather than exposed
retro-synth modulation:

- central A6 sine at 1760 Hz;
- A5 lower sine at 880 Hz as restrained body support;
- very quiet filtered saw for roughness;
- very quiet four-times upper sine;
- narrow-index filtered FM metal skin;
- band-passed white-noise hiss for turbulent air;
- subtle rotation-rate pitch movement;
- six-barrel passage modulation on texture layers, not the main sine;
- six-band EQ emphasizing mechanical mids and controlled metal hiss.

The result is intended to suggest a rotor or jet-like metal whistle cutting
air while the weapon remains at firing speed. It is a compact procedural sound
design abstraction, not a physical aeroacoustic solver.

## Events

### `GGW89_EVENT_SPIN_UP`

- Main sine: 300 Hz -> 1760 Hz.
- Duration: 610 ms.
- Ends at the exact pitch used by the firing loop.

### `GGW89_EVENT_FIRE_LOOP`

- Main sine: A6, 1760 Hz, sustained indefinitely.
- Lower body sine: A5, 880 Hz.
- `start_hz == end_hz`, so there is no sweep.
- Smooth approximately 13 Hz texture motion.
- Approximately 78 Hz six-barrel passage texture.
- Main sine level: 18400 Q15.
- Noise/hiss level: 10600 Q15.
- Very quiet saw: 430 Q15.
- Narrow filtered FM skin: 1380 Q15.
- Does not stop by itself. Trigger `GGW89_EVENT_SPIN_DOWN` to replace it.

### `GGW89_EVENT_SPIN_DOWN`

- Main sine: 1760 Hz -> 350 Hz.
- Duration: 670 ms.
- Begins at the exact pitch used by the firing loop.

## Loop behavior

`GGW89_EVENT_FIRE_LOOP` is procedural and indefinite. It does not replay a
finite sample and therefore has no sample-boundary seam. The envelope reaches
full level and remains there until another event is triggered.

For a basic sequence:

```c
ggw89_trigger(&state, GGW89_EVENT_SPIN_UP);
/* process until inactive */
ggw89_trigger(&state, GGW89_EVENT_FIRE_LOOP);
/* process while firing */
ggw89_trigger(&state, GGW89_EVENT_SPIN_DOWN);
```

`integration_example.c` shows automatic handoff from spin-up to the loop.
Use `ggw89_trigger_continuous()` for the pitch-matched A6 loop-to-spin-down
change; it preserves oscillator phases, filter histories and current envelopes
to avoid a trigger-reset click. A production mixer may still overlap two
`GGW89_State` instances if it wants a longer artistic crossfade.

## New v1.5 API/configuration

```c
#define GGW89_EVENT_FIRE_LOOP 3
void ggw89_trigger_continuous(GGW89_State *state, int event_id);
```

`GGW89_Config` gains:

```c
int loop_mode;
```

`GGW89_State` gains:

```c
int looping;
```

Custom configurations may set `loop_mode` to nonzero to sustain indefinitely.

## Constraints

- Strict C89 source style.
- Fixed-point arithmetic only.
- No malloc, calloc, realloc or free.
- No heap ownership.
- No float or double.
- No `stdint.h` requirement.
- Caller owns state and output buffers.
- Mono signed 16-bit PCM output.

## Build

```sh
make
./test_ggatlingwhistle89
./demo_wav
```

The demo creates `preview_up_fireloop_down.wav` with 610 ms spin-up, two
seconds of sustained A6 firing loop and 670 ms spin-down.
