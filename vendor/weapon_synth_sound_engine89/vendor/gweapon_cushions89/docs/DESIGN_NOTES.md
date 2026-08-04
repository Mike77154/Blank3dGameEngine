# Design notes

- **Weapon body:** a bank of short damped feedback delays approximates modal resonances without trigonometric runtime math.
- **Muzzle gas:** one random source is split into low, band-limited mid, and high turbulence with independent decays.
- **Ballistic crack:** a finite linear positive-to-negative N-wave is followed by a short high-passed air wake. It is triggered independently from muzzle blast.
- **Late tail:** a four-line Hadamard feedback delay network creates a diffuse bounded tail. It intentionally does not calculate geometry or early reflections.
- **Cinema thump:** a 256-entry fixed sine table, falling pitch, brown-noise pressure component, and integer soft clipping provide an optional non-literal sweetener.

These layers are artistic procedural approximations for interactive sound design, not forensic firearm simulators.
