# v1.4.3 — Long sample cursor overflow fix

- Fixes repeated playback of demo sample banks longer than 65,535 frames.
- Replaces the single 32-bit Q16 playback position with a 32-bit frame cursor
  plus a 16-bit fractional accumulator.
- At 44.1 kHz the old cursor wrapped every 65,536 frames (about 1.486 seconds),
  causing the five-second rocket impact bank to restart and sound like four
  separate impacts.
- Applies to every long demo bank, not only `wsound_rocketblast89`.
- No public engine ABI change; this is a showcase/provider cursor correction.
