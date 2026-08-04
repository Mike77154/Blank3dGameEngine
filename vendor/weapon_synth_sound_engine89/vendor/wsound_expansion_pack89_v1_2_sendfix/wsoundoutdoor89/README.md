# wsoundoutdoor89 v1.1 send-bus

Procedural outdoor reflection processor for strict C89 fixed-point engines.

Presets:

- open field;
- urban canyon;
- forest;
- mountain.

Two processing entry points are available:

```c
wsoundoutdoor89_process_sample(ctx, input);      /* dry + reflections */
wsoundoutdoor89_process_wet_sample(ctx, input);  /* reflections only */
```

Use the wet-only function on an auxiliary bus. Feed it short weapon-report or explosion sends rather than the complete master mix. Small friction, casing, thermal and particle details should normally remain dry.

The v1.1 urban preset uses six damped taps instead of eight to avoid broadband buildup during rapid fire.

Protocol: C89, integer/fixed-point only, caller-owned delay memory, no heap.
