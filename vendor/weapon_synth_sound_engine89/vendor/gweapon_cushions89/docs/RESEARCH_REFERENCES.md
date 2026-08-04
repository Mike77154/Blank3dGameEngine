# Research references used for the synthesis design

- Mengual, Moffat, Reiss, **Modal Synthesis of Weapon Sounds**, AES 61st International Conference (2016). The paper models weapon sounds procedurally with extracted spectral peaks, additive/modal components, envelopes, and residual noise.
- Murphy et al., **Developing a method to assess noise reduction of firearm suppressors for small-caliber weapons**, Proceedings of Meetings on Acoustics (2019). Distinguishes trigger/primer, muzzle gas blast, N-shaped ballistic shock wave, and cycling mechanisms.
- Schlecht & Habets, **On Lossless Feedback Delay Networks** (2016). FDN structure used as the basis for a bounded late reverberation tail.
- Valve, **Steam Audio C API simulation documentation**. Used only to preserve the architectural distinction between early convolution/reflections and late parametric reverberation; this bundle implements only the late tail requested.

The implementations are original fixed-point approximations and include no copied recordings or impulse responses.
