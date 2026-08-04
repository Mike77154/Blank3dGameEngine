# Transient correction report — v1.0.1

The original preview WAV files did not reach full-scale PCM clipping, but the
synthesizer still produced two audible failure modes before the final output:

1. the seven-layer accumulator was hard-clipped before the EQ;
2. the high-octave and fragment layers could create very large one-sample
   discontinuities.

Those discontinuities are especially unpleasant on phone speakers, where the
small driver and protection DSP can turn them into brittle static.

## Before and after

| Preview | Maximum jump v1.0.0 | Maximum jump v1.0.1 | Reduction |
|---|---:|---:|---:|
| M67 open | 9256 | 5331 | 42.4% |
| 40 mm HE open | 16019 | 7393 | 53.8% |
| 40 mm HEDP hard | 22817 | 9101 | 60.1% |
| Indoor confined | 8224 | 7330 | 10.9% |
| Concrete impact | 24595 | 11626 | 52.7% |
| Dirt impact | 6845 | 4209 | 38.5% |
| Distant | 2579 | 2535 | 1.7% |
| Arcade heavy | 12787 | 9708 | 24.1% |

## Applied changes

- Fixed-point soft-knee protection before the EQ.
- Fixed-point soft-knee protection at the EQ output.
- One-pole smoothing on the differentiated high-octave noise layer.
- One-pole smoothing on the fragment source.
- Short attack ramp on each fragment event instead of a one-sample jump.

The low-frequency sine, saw woofer, debris body, reverb topology and preset
lengths were left intact so the explosion keeps its original weight.
