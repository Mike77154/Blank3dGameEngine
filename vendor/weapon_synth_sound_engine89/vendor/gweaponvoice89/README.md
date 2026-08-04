# gweaponvoice89 v2.0

Weapon-aware policy layer over `gvoice89 v2.0`, designed for large procedural gunfights without mixing every logical event every sample.

## Default scale

- **256 logical voice slots**
- **64 physical mixed voices**
- **192 concurrently virtual voices** when the pool is full
- 8 weapon event classes with independent logical limits and physical reserves
- sample-safe batch starts, distance priority, occlusion/focus audibility, instance-key limits and deterministic stealing

The old four-argument `gwv89_init(ctx, slots, capacity, sample_rate)` remains available and chooses up to 64 physical voices. Use `gwv89_init_ex` to set the physical budget explicitly.

## Default weapon policy

| Class | Logical cap | Physical reserve | Base priority | Virtual behavior |
|---|---:|---:|---:|---|
| Report | 96 | 24 | 560 | Advance |
| Mechanism | 40 | 8 | 480 | Pause |
| Foley | 48 | 2 | 260 | Kill |
| Casing | 96 | 0 | 110 | Kill |
| Ricochet | 64 | 4 | 330 | Advance |
| Impact | 96 | 12 | 400 | Advance |
| Explosion | 24 | 8 | 650 | Continue |
| Ambience | 32 | 0 | 80 | Advance |

The caps are independent constraints, not a fixed partition. The global logical capacity remains 256.

## Stealing order

The core rejects or steals only when a logical, class or instance-key limit is reached. The default policy favors:

1. explosions and nearby reports;
2. mechanisms and impacts;
3. ricochets;
4. Foley;
5. distant casings and ambience.

Distance can lower priority as well as audibility. Per-event bias, `CRITICAL`, `NEVER_STEAL`, `NEVER_VIRTUAL`, start protection and minimum physical hold time can override the defaults.

## Stress test result

The included six-second WAV schedules 512 procedural weapon events through a 256-logical / 64-physical handler:

- starts: **512**
- peak logical: **256**
- peak physical: **64**
- controlled steals: **124**
- group-limit steals: **92**
- promotions: **270**
- demotions: **46**
- limiter hits: **0**
- clipped samples: **0**

## Memory measured on this build

- `sizeof(gv89_voice)`: **168 bytes**
- `sizeof(gwv89_context)`: **2,080 bytes**
- 256 slots plus weapon context: **45,088 bytes (44.03 KiB)**

This excludes the caller's synthesis/provider states. Use the physical-state callback and a caller-owned pool when each full weapon synth context is large.

## Build

```sh
make test
./build/stress_demo audio/weapon_voice_256logical_64physical_stress.wav
```

The library code is C89, fixed/integer-only, heapless and backend-agnostic.
