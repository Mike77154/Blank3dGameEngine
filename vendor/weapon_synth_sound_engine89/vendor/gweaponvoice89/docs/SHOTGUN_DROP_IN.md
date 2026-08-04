# Existing shotgun adapter

`gvoice89 v2.0` and `gweaponvoice89 v2.0` were compiled as replacements inside the existing `gshotgunsequence89 v1.3` source tree. The original adapter, smoke test and voice-handler test compiled and passed without source changes.

The compatibility entry point:

```c
gwv89_init(&handler, slots, capacity, sample_rate);
```

keeps the old call shape and selects `min(capacity, 64)` physical voices. New code can use:

```c
gwv89_init_ex(&handler, slots, 256U, 64U, sample_rate);
```

A full `gss89_context` per logical slot may be expensive. For 256 logical shotgun events, prefer lightweight logical recipes plus a 32–64 state heavy-synth pool attached through `physical_state_changed`.
