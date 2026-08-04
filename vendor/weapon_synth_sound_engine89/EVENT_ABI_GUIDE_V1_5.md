# Event ABI guide v1.5 — acoustic additions

The existing v1.4 event ABI remains source-compatible. Version 1.5 appends seven event classes:

```text
WSSE89_EVENT_ACOUSTIC_ENABLE
WSSE89_EVENT_ACOUSTIC_PROFILE
WSSE89_EVENT_ACOUSTIC_PATH
WSSE89_EVENT_ACOUSTIC_MATERIAL
WSSE89_EVENT_ACOUSTIC_SPACE
WSSE89_EVENT_ACOUSTIC_PORTAL
WSSE89_EVENT_PRESSURE_TRIGGER
```

## Suggested game-engine mapping

```text
AudioEnvironmentChanged → ACOUSTIC_SPACE + ACOUSTIC_MATERIAL
ListenerPathSolved      → ACOUSTIC_PATH
DoorOrWindowChanged     → ACOUSTIC_PORTAL
WeaponFired             → REPORT (auto pressure/directivity)
ProjectileNearListener  → PROJECTILE
ProjectileCollision     → IMPACT / RICOCHET
GrenadeDetonated        → GRENADE_BLAST (auto pressure)
RocketDetonated         → ROCKET_BLAST (auto pressure)
Accessibility/Profile   → ACOUSTIC_PROFILE
```

`wsse89_event_sink()` remains suitable as a callback target. An engine may enqueue events from gameplay and consume them on the audio thread. Do not mutate the same acoustic context concurrently from multiple threads without a host-side command queue.

For full per-source spatial accuracy use `wsoundacoustic89.h` directly and pool one context per active source or acoustic bus. The façade's `gsynthworld89` acoustic context is a post-mix convenience path.
