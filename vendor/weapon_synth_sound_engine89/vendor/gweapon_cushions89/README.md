# GWeapon Cushions 89 v1.0

Five independent, provider-friendly procedural audio layers for a weapon system:

1. **gweaponbody89** - input-excited modal/comb resonant body.
2. **gmuzzlegas89** - filtered propellant-gas turbulence.
3. **gballisticcrack89** - delayed supersonic N-wave plus air edge.
4. **glatetail89** - late diffuse four-line FDN; intentionally no early reflections.
5. **gcinemathump89** - optional descending sub-bass sweetener.

All cores are strict C89, fixed-point, caller-owned, deterministic, and contain no heap calls, floating point, or `math.h`. The demo uses only procedural synthesis. No recorded gun samples are included.

## Build and hear it

```sh
make
make test
make samples
```

`audio/00_full_demo.wav` sequence:

- 1 s: deliberately dry baseline pistol shot.
- 4 s: layered pistol.
- 8 s: layered shotgun.
- 13 s: layered sniper pass.
- 18 s: layered launcher/tunnel exaggeration.

The remaining WAV files are isolated stems.

## Integration rule

Treat each module as a source/effect provider behind your engine ABI. `gweaponbody89` and `glatetail89` accept an input sample; the gas, crack, and thump modules generate independent layers when triggered. Built-in presets are examples, not mandatory assets.

License: CC0-1.0.

## Existing three-synth sequence audition

`audio/07_three_original_synths_plus_five_cushions.wav` reprocesses the previously rendered shotgun sequence. The original mechanism/report stays intact; the five new modules add body, gas, delayed crack, late tail, and thump at the two shot timestamps. `audio/06_cushions_over_sequence_stem.wav` contains only the added layers.

Prebuilt `.a` files in this archive are host-validation artifacts from the build environment. Rebuild with the included Makefiles or `build_mingw32.bat` for the target toolchain.
