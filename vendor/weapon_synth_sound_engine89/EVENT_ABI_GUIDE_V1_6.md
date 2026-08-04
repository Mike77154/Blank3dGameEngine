# Event ABI guide v1.6

The v1.6 ABI is append-only relative to v1.5. Existing event numeric values are
unchanged; five Phase 3 values were appended before `WSSE89_EVENT_COUNT`.

## Phase 3 events

### `WSSE89_EVENT_WEAPON_PROFILE`
Payload: `wsounddna89_profile weapon_profile`.

Sets the physical identity used by subsequent profiled shots.

### `WSSE89_EVENT_WEAPON_MODE`
Payload: `wsounddna89_mode weapon_mode`.

Sets Realistic, Hybrid or Cinematic mode and synchronizes the Phase 1/2 acoustic
profile.

### `WSSE89_EVENT_WEAPON_FIRE`
Payload: `wsse89_weapon_fire_event weapon_fire`.

Call `wsse89_weapon_fire_defaults()` before overriding position, instance key
or gain. Returns a normal `gv89_handle`.

### `WSSE89_EVENT_METRICS_ENABLE`
Payload: `wsound89_u16 metrics_enabled`.

Enables or disables post-world analysis.

### `WSSE89_EVENT_METRICS_RESET`
No payload.

Clears the accumulator without changing the current weapon profile.

## Queries

```c
wsse89_get_last_dna(...)
wsse89_get_metrics(...)
wsse89_validate_metrics(...)
```

The metrics query does not reset the accumulator.
