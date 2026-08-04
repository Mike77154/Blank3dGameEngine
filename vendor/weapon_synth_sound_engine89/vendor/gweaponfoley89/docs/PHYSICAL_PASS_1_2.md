# Physical micro-contact pass 1.2

## Scope

The revision changes only the organization and tuning of existing synthesis controls:

- maximum contacts per preset: 4 -> 8;
- per-contact six-band EQ;
- per-contact fixed-point soft distortion;
- revised micro-contact order, offsets and envelopes;
- lighter master EQ/distortion after contact summing.

It adds no sample playback, resonator, modal bank, compressor, transient shaper, additional noise source, oscillator or effect.

## Mechanical references translated into contact order

### Striker-fired pistol

The Beretta APX A1 manual describes trigger-bar and blocking-lever motion, striker spring compression, striker release, full-forward travel and rebound to neutral. The preset therefore uses release, striker-stop and spring-rebound contacts rather than one broadband burst.

### Bolt-action rifle

The Remington Model 700 manual separates raising the bolt handle, rearward bolt travel, forward travel and pushing the handle down. The dry bolt preset expands this into unlock, rail travel, rear stop, forward travel, front seat, locking rotation and detent settle.

### Pump shotgun controls

The Mossberg manual identifies a manually moved top safety button and a separate action-lock lever. The library keeps the safety click separate from the hammer/body dry-fire and does not synthesize the pump/chamber sound.

### Closed-bolt SMG

Heckler & Koch describes the MP5 as firing from a closed bolt with roller-delayed blowback. The dry mechanism preset is consequently modeled as compact trigger/sear/hammer contacts, while the selector remains a separate detent event.

### M72-style launcher

FM 3-21.8 describes an M72 launcher made from two nested tubes and housing a percussion-type firing mechanism. The launcher presets therefore bias tube/latch contacts toward low-mid energy and keep a smaller percussion contact inside the sequence.

## Acoustic rationale

Aramaki and Kronland-Martinet's impact-sound work uses dynamically filtered noise to reproduce perceptual material and damping cues. Version 1.2 stays within that subtractive/noise-based approach: each micro-contact gets its own spectral damping approximation through the fixed six-band filter and its own nonlinear density.

The implementation deliberately does not claim to be a complete rigid-body modal simulation. It is a compact perceptual approximation for game Foley under strict C89/fixed-point/static-memory constraints.

## Equal-seed output measurements

All physical renders are 44.1 kHz, mono, signed 16-bit PCM. No A/B normalization was applied.

| preset | RMS dBFS | peak FS | crest dB | centroid kHz | active ms | centroid vs 1.1 kHz |
|---|---:|---:|---:|---:|---:|---:|
| pistol_empty | -28.35 | 0.944 | 27.85 | 10.05 | 32.1 | -1.20 |
| pistol_handling | -28.68 | 0.841 | 27.17 | 10.00 | 77.6 | -2.39 |
| magnum_empty | -24.78 | 0.939 | 24.23 | 7.78 | 53.9 | -2.70 |
| magnum_latch | -26.19 | 0.904 | 25.31 | 8.08 | 73.5 | -1.40 |
| sniper_empty | -28.16 | 0.930 | 27.54 | 10.36 | 41.2 | -0.43 |
| sniper_bolt_dry | -21.38 | 0.900 | 20.46 | 7.82 | 201.1 | -2.81 |
| smg_empty | -26.54 | 0.934 | 25.95 | 9.12 | 35.0 | -3.08 |
| smg_selector | -32.48 | 0.926 | 31.81 | 11.10 | 30.7 | -0.91 |
| launcher_empty | -23.45 | 0.917 | 22.69 | 6.44 | 74.0 | -1.96 |
| launcher_latch | -22.02 | 0.910 | 21.20 | 5.81 | 91.0 | -3.08 |
| shotgun_empty | -25.48 | 0.944 | 24.98 | 8.21 | 53.2 | -2.73 |
| shotgun_safety | -31.76 | 0.898 | 30.82 | 10.99 | 34.1 | -0.50 |

The maximum measured peak is 0.944 FS. Compared with 1.1, every preset's measured spectral centroid moved downward by approximately 0.43 to 3.08 kHz, consistent with less extreme-high hiss and more low/mid mechanical body.
