# Changelog

## 1.4.0

- expanded the independent noise bank from five to seven generators
- added `WSRB89_NOISE_RUMBLE`, a long low-frequency stochastic aftermath layer
- added `WSRB89_NOISE_CRACKLE`, a sparse high-frequency fracture/debris layer
- added independent rumble low-pass SVF and crackle band-pass SVF
- added public cutoff, damping, decay and level controls for both layers
- added a dedicated fixed-point motion LFO for rumble amplitude movement
- retained the existing chorus LFO exclusively on the room send
- retuned all seven presets for the new layers
- added deterministic tests proving the two layers and motion LFO affect output
- added five-noise versus seven-noise and isolated-layer WAV previews
- workspace remains 48,004 bytes; context is 632 bytes on the tested 64-bit ABI
- public parameter/context ABI changed; source compatibility remains straightforward

## 1.3.0

- rebuilt the spatial response around separate direct, early-reflection and late-reverb fields
- added five asymmetric early reflections at approximately 12, 23, 37, 56 and 82 ms
- added low-pass diffusion to the early field so it enlarges the hit without creating bright echoes
- expanded the late reverb from two combs and one allpass to three combs and two allpasses per channel
- moved chorus to the room send only; the direct pressure front no longer receives modulation
- added a stochastic low-frequency room cloud derived from the body and low companion
- added an independent transient SVF and six-band EQ state bank
- strongly reduced persistent 3.2 kHz and 7.2 kHz energy in the monumental presets
- lengthened body and room decay while reducing the first 50 ms dominance
- retuned all seven presets without increasing distortion drive
- fixed signed-overflow risk in the SVF integrator by clamping the high state before multiplication
- fixed residual DC quantization so finished tails settle to exact zero
- increased per-voice workspace to 48,004 bytes for the denser room network
- retained the v1.2 public parameter structure; context/workspace binary layout changed
- added a v1.2 dry versus v1.3 monumental Heavy Impact A/B preview

## 1.2.0

- rebuilt the internal routing around separate body and transient paths
- replaced the slow linear shock lobe with a faster curved positive/negative pressure shape
- made the upper noise companion a brief tearing layer tied to the crack envelope
- made the lower companion slower, denser and softly saturated
- reduced raw broadband leakage from the central body
- shortened and frequency-jittered the C2-to-C1 sine/saw thump
- reduced the saw contribution so it roughens the sub instead of sounding like a note
- moved distortion before the body SVF to prevent regenerated high-frequency fizz
- preserved the transient as a mostly dry centered component
- reduced chorus depth/rate/mix and placed effects behind the direct impact
- retuned all seven presets for stronger 80/180 Hz weight and less persistent upper hiss
- retained the v1.1 public parameter ABI
- added a v1.1 versus v1.2 Heavy Impact A/B preview

## 1.1.0

- expanded the noise bank from three to five independent generators
- added low and high spectral companions around the central body
- exposed named public indices for all five noise levels
- added a deterministic three-noise versus five-noise A/B preview

## 1.0.0

- initial strict-C89 fixed-point release
- three dedicated noise generators
- C2-to-C1 sine/saw layer
- bipolar pressure impulse
- ADSR one-shot envelope plus crack and debris envelopes
- SVF, six-band EQ, distortion, chorus and stereo reverb
- seven presets
