# gweapon89 manager v2 — providerized weapon orchestrator

`gweapon89` is a C89, fixed-point, static-storage-friendly weapon manager for **weapons owned by any actor**. A player, enemy, ally, turret, vehicle, trap, scripted prop, or network proxy is represented only by external integer IDs.

Version 2 converts the manager into a **mutable provider pipeline**. Every important dependency can be supplied by the host, modified by middleware, replaced completely, or cancelled without editing the weapon core.

## Core guarantees

- C89 (`-std=c89 -pedantic`).
- No `malloc`, `calloc`, `realloc`, or `free`.
- No `float` or `double`; gameplay values use fixed point.
- The core includes no `stdio.h`, `stdlib.h`, `ctype.h`, or `math.h`; file access is an IO provider.
- Static provider registry: `GWP89_MAX_PROVIDERS`, default 48.
- Stable provider handles even when higher-priority providers are added later.
- Zero-provider fallback: the internal behavior and legacy `GWP89_Hooks` remain usable.
- Actor-agnostic: no player-only branch.
- Event queue remains available even when direct providers are installed.

## Provider capabilities

Every provider receives a mutable `GWP89_ProviderPacket` in two phases:

```text
PRE providers
    │
    ├─ may inspect or modify the request
    ├─ may claim the fallback with HANDLED
    ├─ may cancel the stage with CANCEL
    └─ may stop lower-priority providers with STOP
    ▼
Internal or legacy fallback
    ▼
POST providers
    └─ may inspect or modify the final result
```

Provider return bits:

| Result | Effect |
|---|---|
| `GWP89_PROVIDER_PASS` | Observe only. |
| `GWP89_PROVIDER_MODIFIED` | Packet or event was changed. |
| `GWP89_PROVIDER_HANDLED` | Provider owns the fallback for that stage. |
| `GWP89_PROVIDER_CANCEL` | Abort that stage or suppress that event. |
| `GWP89_PROVIDER_STOP` | Do not invoke lower-priority providers. |

Bits may be combined.

## Services

The provider bus exposes dedicated services for:

```text
math3d              transform            numeric
flags               camera               sockets
inventory           raycast              pose
hud                 crosshair            scope
draw                zoom                 recoil
spread              projectile           projectile_life
health              muzzle               casing
trail               mesh                 reload
active_reload       event_bus            io
actor_state
```

This covers the requested external ownership of:

- 3D math and actor transforms;
- numeric systems and runtime modifiers;
- flags and permission gates;
- cameras, zoom, sockets, origins, and aiming positions;
- general inventory, reserve ammo, magazines, and equipment;
- raycasts and hit information;
- HUDs, crosshairs, scopes, and drawing;
- projectile spawning, speed, range, travel distance, lifetime, and mesh;
- spread, recoil, health/damage notifications, muzzle flashes, casings, and trails;
- normal reload and active reload;
- text/INI loading through a host-owned IO provider;
- actor frame state, effective input, delta time, cooldown/reload advancement, and trigger latch through an actor-state provider.

## Minimal provider registration

```c
static int my_projectile_provider(void *ctx, GWP89_ProviderPacket *packet)
{
    MyProjectileSystem *system;
    system = (MyProjectileSystem *)ctx;

    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;
    if (packet->operation != GWP89_OP_EMIT_EVENT) return GWP89_PROVIDER_PASS;
    if (!packet->event) return GWP89_PROVIDER_PASS;

    my_spawn_projectile(system, packet->event);
    return GWP89_PROVIDER_HANDLED;
}

GWP89_Manager weapons;
int projectile_provider_handle;

gwp89_init(&weapons);
projectile_provider_handle = gwp89_add_provider(
    &weapons,
    GWP89_SERVICE_PROJECTILE,
    100,
    "my_projectiles",
    &my_projectile_system,
    my_projectile_provider
);
```

Higher priority values run first. The returned handle remains stable and can be passed to `gwp89_remove_provider()`.

## Actor-state updates

`GWP89_SERVICE_ACTOR_STATE` receives `GWP89_OP_UPDATE` before any timer or trigger policy is applied. Its PRE provider may mutate the effective `GWP89_FireInput`, change `ms_value` for delta time, cancel the update, or return `HANDLED` to own cooldown/reload/latch advancement. The same mutated input continues through fire validation, camera, socket, pose, and event creation.

## Runtime numeric modifiers

The numeric provider can alter weapon values per actor, per weapon, and per call:

```c
static int difficulty_numbers(void *ctx, GWP89_ProviderPacket *packet)
{
    (void)ctx;
    if (packet->phase != GWP89_PHASE_PRE) return GWP89_PROVIDER_PASS;

    if (packet->operation == GWP89_OP_QUERY_FX &&
        packet->key == GWP89_NUM_DAMAGE_FX &&
        packet->team_id != 1) {
        packet->fx_value = (packet->fx_value * 3L) / 2L;
        return GWP89_PROVIDER_MODIFIED;
    }
    return GWP89_PROVIDER_PASS;
}
```

Numeric keys include clip size, ammo cost, pellet count, burst count, fire mode, cooldown, reload time, projectile lifetime, damage, speed, range, spread, radius, mesh scales, recoil, and active-reload timing.

## Event fanout

One weapon event may be delivered to several systems before it is queued:

```text
FIRE_ACCEPTED
 ├─ event_bus
 ├─ HUD
 ├─ crosshair
 ├─ scope
 ├─ zoom
 ├─ recoil
 └─ draw

PROJECTILE_REQUEST
 ├─ event_bus
 ├─ projectile_life   (may modify life/range first)
 ├─ projectile        (spawner sees modified event)
 ├─ draw
 └─ health            (when HIT_VALID is present)
```

A projectile-life provider may therefore change `life_ms`, `range_fx`, or `travel_distance_fx` before the projectile provider receives the event.

## Providerized pose resolution

When no monolithic legacy `resolve_pose` hook handles the shot, the fallback pipeline is:

```text
actor transform provider
        │
camera + zoom providers
        │
projectile/muzzle/casing socket providers
        │
spread provider
        │
math3d normalize provider
        │
raycast provider
        │
math3d endpoint fallback
        │
pose POST providers
```

The old `hooks.resolve_pose` remains supported as a high-level fallback and receives the provider-resolved camera/transform input.

## Inventory ownership

The inventory provider can own:

- reserve ammo query/set/add/consume;
- magazine/clip query/set;
- equipment notifications.

When no inventory provider handles an operation, the manager uses legacy ammo hooks and then its internal static ammo bank.

## Reload ownership

Normal reload exposes:

```text
GWP89_OP_RELOAD_BEGIN
GWP89_OP_RELOAD_TICK
GWP89_OP_RELOAD_COMPLETE
```

Active reload exposes:

```text
GWP89_OP_ACTIVE_RELOAD_PRESS
GWP89_EVENT_ACTIVE_RELOAD_WINDOW
GWP89_EVENT_ACTIVE_RELOAD_SUCCESS
GWP89_EVENT_ACTIVE_RELOAD_FAIL
```

The fallback active-reload implementation is optional per weapon and can be replaced completely by a provider.

## IO ownership

The core never opens files directly. `gwp89_load_ini_file()` emits:

```text
service   = GWP89_SERVICE_IO
operation = GWP89_OP_READ_TEXT_FILE
text_in   = requested path
text_out  = caller-owned static buffer
```

An IO provider fills `text_out`, sets `text_length` and `result_code`, then returns `HANDLED`. `gwp89_load_ini_text()` remains available when the host already owns the bytes.

`adapters/stdio_ini_provider_example.c` is an optional boundary adapter for desktop hosts. Embedded, archive, virtual-file-system, network, ROM, or engine-resource providers can use the same ABI without adding any file API to the core.

## Build and validation

```sh
make -f Makefile.mingw
./provider_validation
./legacy_validation
./ini_validation
```

MSYS2/MinGW32:

```bat
mingw32-make -f Makefile.mingw
provider_validation.exe
legacy_validation.exe
ini_validation.exe
```

## Files

```text
gweapon89_manager/
├─ src/
│  ├─ gweapon89.h
│  └─ gweapon89.c
├─ adapters/
│  ├─ blank3d_hook_example.c
│  ├─ provider_pack_example.c
│  ├─ stdio_ini_provider_example.h
│  └─ stdio_ini_provider_example.c
├─ demo/
│  ├─ provider_validation.c
│  ├─ legacy_validation.c
│  └─ ini_validation.c
├─ config/
│  └─ provider_weapon.ini
├─ docs/
│  ├─ ARCHITECTURE.md
│  ├─ PROVIDER_ABI.md
│  ├─ PROVIDER_MATRIX.md
│  └─ VALIDATION.md
├─ Makefile.mingw
├─ VERSION.txt
└─ README.md
```
