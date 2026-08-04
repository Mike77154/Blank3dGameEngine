# Research and design notes — v1.5

## Sustained firing note

The firing loop uses A6, exactly 1760 Hz, as an artistic pitch anchor. This is
not presented as a literal measured resonance of one specific M134 assembly.
It was selected because it preserves a clear air-cut whistle while leaving
room underneath for A5 body support and broadband mechanical texture.

The three event boundaries are deliberately matched:

```text
spin-up endpoint:   1760 Hz
firing loop:        1760 Hz
spin-down start:    1760 Hz
```

That makes the pitch logic coherent when an engine sequences or crossfades the
events.

## Mechanical interpretation

The sustained sound is separated into:

- dominant tonal airflow/rotor whistle;
- lower structural body;
- band-passed turbulent hiss;
- weak inharmonic FM metal skin;
- weak saw roughness;
- sixfold passage texture on secondary layers.

At the loop default, the rotation texture is approximately 13 Hz. Six passage
cycles per group rotation produce an approximately 78 Hz mechanical texture.
This modulates hiss and metallic support more than the central sine, preventing
the main A6 note from sounding like a science-fiction vibrato patch.

## Scope

This library is a procedural game-audio model. It does not claim to reproduce
one exact weapon, mounting, microphone position or airflow condition. The
implementation favors controllable perceptual cues under strict C89 and
fixed-point constraints.
