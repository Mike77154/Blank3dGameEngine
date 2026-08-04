# v1.2 body filter and attack tuning

## Dedicated body low-pass

Version 1.2 adds a two-stage one-pole cascade controlled by `body_lp_q15`. It processes only the ADSR-shaped aerodynamic noise. The ballistic crack is mixed after this filter, preserving the short N-wave transient.

At 48 kHz the supplied presets use Q15 coefficients from 21000 to 25200. Lower coefficients sound darker and heavier; higher coefficients retain more upper-mid detail.

## Less white-noise character

The original swept HP/LP stage remains, but its low-pass coefficients are lower than in v1.1. The six-band EQ was also retuned:

- more low and mid body;
- controlled presence;
- reduced air;
- strongly reduced ultra-air.

The result is intentionally closer to compressed turbulent air than an exposed white-noise generator.

## Slightly slower onset

Preset attacks now range from 20 to 36 ms. This is only a small extension over v1.1: enough to create a more inhaled aerodynamic swell without making the trajectory feel sluggish. The crack layer remains independent and can still occur before the noise body reaches full level.
