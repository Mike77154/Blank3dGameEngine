# gweapon89 v2 architecture

## Responsibility boundary

The manager owns only portable weapon orchestration:

1. actor-to-weapon state;
2. trigger modes and cooldown state;
3. default magazine/reserve behavior;
4. default reload and optional active reload;
5. weapon profile parsing from host-supplied text;
6. provider pipeline and event queue.

Everything else may be externally owned, including path-to-text IO.

## Full flow

```text
Host fire input
    │
    ├─ actor_state provider (effective input, dt, timers, latch)
    ├─ transform provider
    ├─ camera provider
    ├─ zoom provider
    └─ flags/fire validation
            │
            ▼
      trigger/cooldown policy
            │
            ├─ numeric provider values
            ├─ inventory clip/reserve providers
            └─ reload state
            │
            ▼
       pose per pellet
            ├─ socket provider
            ├─ spread provider
            ├─ math3d provider
            ├─ raycast provider
            ├─ legacy resolve_pose fallback
            └─ pose provider post-processing
            │
            ▼
      mutable event fanout
            ├─ HUD / crosshair / scope / zoom
            ├─ draw / mesh / recoil
            ├─ projectile life → projectile
            ├─ muzzle / casing / trail
            ├─ health when a hit is valid
            └─ generic event bus
            │
            ▼
       static event queue
```

## Provider chaining

Providers are not one-provider-per-service. Multiple systems may stack on the same service.

Example numeric chain:

```text
base profile damage
    │
    ├─ difficulty provider: ×1.25
    ├─ actor buff provider: +10
    ├─ material provider: ×0.80
    └─ network authority provider: clamp
    ▼
final event damage
```

Example event chain:

```text
projectile event
    │
    ├─ projectile_life provider changes life/range
    ├─ projectile provider spawns physical entity
    ├─ draw provider creates debug visualization
    ├─ health provider receives valid ray hit
    └─ event bus logs replication packet
```

## Fallback layers

Each dependency has a fallback ladder:

```text
provider chain
    │ not handled
    ▼
legacy hook, where one exists
    │ not handled / unavailable
    ▼
internal fixed-point fallback
```

Examples:

- actor update: provider-owned update → internal timers and trigger latch;
- inventory: provider → legacy ammo hooks → internal ammo bank;
- pose: pose provider → legacy resolve-pose hook → sockets/spread/raycast/math fallback;
- projectile output: provider → legacy emit-projectile hook → queued event only;
- clip: provider → internal user state;
- INI path loading: IO provider → caller may instead pass text directly.

## Actor agnosticism

The manager never checks for a player type. Identity is supplied as:

```c
actor_id
actor_kind
team_id
```

The host defines their meaning.

## Static storage

Default capacities:

```text
64 weapon profiles
32 actor/user states
64 ammo types per internal actor bank
256 queued events
48 providers
```

All capacities are compile-time macros and may be reduced for smaller targets.

## Runtime mutability

Providers can affect current behavior without editing profiles:

- temporary buffs/debuffs;
- difficulty scaling;
- network authority;
- per-frame input/time/state policy;
- per-camera aim policy;
- actor-specific sockets;
- inventory ownership;
- alternate projectile implementations;
- hitscan versus physical projectile policy;
- scope-dependent spread and zoom;
- custom reload or active-reload minigames;
- accessibility HUD/crosshair variants;
- renderer-specific muzzle/casing/trail implementations;
- stdio, virtual-file-system, archive, ROM, or network-backed profile loading.

## Event mutation order

The event is copied to a mutable local value before provider fanout. PRE providers mutate that value. The final value is then:

1. inserted into the queue;
2. sent to legacy callbacks when the primary service was not handled;
3. exposed to POST providers.

For projectile events, `projectile_life` runs before `projectile`, ensuring the spawner receives modified lifetime and travel distance.
