# Atom FX audition

The approved base atom keeps the SimSynth-style `SH` release at exactly 12 ms.
The effect audition uses the same deterministic noise seed for every stage, so
the differences come from the processing rather than a different random burst.

## Runtime effect flags

```c
CH89_EFFECT_NONE
CH89_EFFECT_DISTORTION
CH89_EFFECT_EQ
CH89_EFFECT_CHORUS
CH89_EFFECT_REVERB
CH89_EFFECT_ALL
```

The flags can be combined and changed per `ch89_gesture`:

```c
ch89_set_effect_flags(
    &gesture,
    CH89_EFFECT_DISTORTION |
    CH89_EFFECT_EQ |
    CH89_EFFECT_CHORUS
);
```

## Stage montage order

`wav/25_atom_fx_stage_montage.wav` is peak-matched for fair listening:

```text
1. dry atom
2. distortion
3. distortion + moving six-band EQ
4. distortion + EQ + chorus
5. full chain with tiny reverb
```

The individual WAV files preserve their natural output level.

## Strength montage order

`wav/29_atom_fx_strength_montage.wav` is also peak-matched:

```text
1. subtle/default
2. medium audition
3. pushed audition
```

Only `subtle/default` is the normal atom preset. Medium and pushed are audition
settings intended to reveal the useful range of the effect chain.

## Signal order

```text
SH/ECKT envelope
  -> optional voice distortion
  -> optional moving six-band EQ
  -> sum voices/strokes
  -> optional chorus
  -> optional tiny reverb
  -> output gain
```
