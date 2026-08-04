# Rocket rumble/crackle integration report — v1.4.1

## Added signal layers

```text
independent noise #6 -> rumble pre-smoothing -> low-pass SVF -> body/direct + room
independent noise #7 -> sparse gate -> band-pass SVF -> transient/direct + room
motion sine LFO    -> downward-only rumble amplitude modulation
```

The existing main body SVF, transient SVF, two six-band SVF/EQ banks and chorus LFO remain intact. The chorus LFO still affects only the room send.

## Public controls

- `noise_level_q15[WSRB89_NOISE_RUMBLE]`
- `noise_level_q15[WSRB89_NOISE_CRACKLE]`
- `rumble_decay_ms`, `crackle_decay_ms`
- `rumble_cutoff_hz`, `rumble_damp_q15`
- `crackle_cutoff_hz`, `crackle_damp_q15`
- `motion_lfo_rate_millihz`, `motion_lfo_depth_q15`

`gsso89_rocket_params` now carries a complete `wsrb89_params` block when `use_custom_params` is enabled, so the unified event ABI exposes all of these controls.

## Memory

- workspace: 48,004 bytes, unchanged
- rocket context: 632 bytes on the tested 64-bit ABI
- integrated rocket provider voice: 48,776 bytes on the tested build

## Preview order

`audio/12_rocketblast_rumble_crackle_layers.wav`:

1. previous five-noise stack
2. rumble isolated
3. crackle isolated
4. complete seven-noise Heavy Impact
