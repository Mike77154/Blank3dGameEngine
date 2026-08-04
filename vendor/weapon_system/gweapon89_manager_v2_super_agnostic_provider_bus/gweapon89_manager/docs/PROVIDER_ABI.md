# Provider ABI reference

## Registration

```c
int gwp89_add_provider(
    GWP89_Manager *manager,
    int service,
    int priority,
    const char *name,
    void *context,
    GWP89_ProviderFn callback
);
```

The returned integer is a stable slot handle. Remove it with:

```c
gwp89_remove_provider(&manager, handle);
```

The registry is fixed-size and allocation-free. Providers are executed by descending priority; equal priorities preserve registration order.

## Packet contract

`GWP89_ProviderPacket` is intentionally broad so one ABI can bridge engines with different subsystem boundaries.

Important fields:

| Field | Purpose |
|---|---|
| `phase` | `GWP89_PHASE_PRE` or `GWP89_PHASE_POST`. |
| `service` | Service currently being invoked. |
| `operation` | Query, mutation, lifecycle, or event operation. |
| `key` | Numeric or flag key. |
| actor/team/weapon IDs | Host identity and lookup data. |
| `i_value`, `i_value2`, `amount` | Mutable integer values. |
| `ms_value` | Mutable millisecond value. |
| `fx_value`, `fx_value2` | Mutable fixed-point values. |
| `vec_a`, `vec_b`, `vec_c` | Input/output vectors for math and rays. |
| `transform` | Position, basis, and scale. |
| `camera` | Camera origin, basis, view style, and zoom. |
| `hit` | Raycast hit result. |
| profile/user/input pointers | Current weapon context. |
| pose request/result pointers | Mutable aiming context. |
| `event` | Mutable event before queueing or after dispatch. |
| `text_in`, `text_out`, `text_capacity`, `text_length` | Allocation-free text transfer for IO providers. |
| `payload` | Host-defined extension pointer. |

Providers must only use fields relevant to the current service and operation.

## Operations by service

### Actor state

- `GWP89_OP_UPDATE`

PRE receives `user`, a mutable effective `input`, and frame delta in `ms_value`. Providers may modify input or time, return `CANCEL`, or return `HANDLED` to replace the manager's default cooldown, reload, and trigger-latch advancement. POST receives the resulting state. The original caller input is never modified; the provider operates on a per-call copy.

### Math3D

- `GWP89_OP_MATH_ADD`
- `GWP89_OP_MATH_SCALE`
- `GWP89_OP_MATH_NORMALIZE`

Inputs use `vec_a`, `vec_b`, and `fx_value`; output uses `vec_c`.

### Transform

- `GWP89_OP_GET_ACTOR_TRANSFORM`

Write `packet->transform` and return `HANDLED` to replace the input transform.

### Numeric

- `GWP89_OP_QUERY_INT`
- `GWP89_OP_QUERY_FX`

Read `key`, then mutate `i_value` or `fx_value`. Numeric providers normally return `MODIFIED`, not `HANDLED`, because several modifiers may stack.

### Flags

- `GWP89_OP_QUERY_FLAG`
- `GWP89_OP_FIRE_VALIDATE`
- `GWP89_OP_FIRE_ACCEPTED`

`QUERY_FLAG` uses `key` and `i_value`. `FIRE_VALIDATE` may return `CANCEL` before ammo is consumed.

### Camera and zoom

- Camera: `GWP89_OP_GET_CAMERA`
- Zoom: `GWP89_OP_QUERY_FX` and event fanout

Camera providers write `packet->camera`. Zoom providers may alter the live `fx_value` and later receive visual events.

### Sockets

- `GWP89_OP_GET_SOCKET`

`socket_kind` identifies projectile, muzzle, casing, or aim. Write `transform` and return `HANDLED`.

### Inventory

- `GWP89_OP_AMMO_QUERY`
- `GWP89_OP_AMMO_CONSUME`
- `GWP89_OP_AMMO_SET`
- `GWP89_OP_AMMO_ADD`
- `GWP89_OP_CLIP_QUERY`
- `GWP89_OP_CLIP_SET`
- `GWP89_OP_EQUIP`

For consume, set `result_code` to `GWP89_OK` or `GWP89_NO_AMMO` and return `HANDLED`.

### Spread

- `GWP89_OP_APPLY_SPREAD`

Input direction is `vec_a`; spread is `fx_value`; pellet index/count are `i_value/i_value2`. Output direction is `vec_b`.

### Raycast

- `GWP89_OP_RAYCAST`

Origin is `vec_a`, direction is `vec_b`, maximum range is `fx_value`, and output is `hit`.

### Pose

- `GWP89_OP_RESOLVE_POSE`

A pose provider may write `pose_result` and return `HANDLED`, replacing both the legacy pose hook and the internal socket/raycast fallback.

### Reload

- `GWP89_OP_RELOAD_BEGIN`
- `GWP89_OP_RELOAD_TICK`
- `GWP89_OP_RELOAD_COMPLETE`

`ms_value` carries total or remaining time. A completion provider that returns `HANDLED` owns the magazine transfer.

### Active reload

- `GWP89_OP_ACTIVE_RELOAD_PRESS`

Input: elapsed time in `i_value`, remaining time in `ms_value`. Output: adjusted remaining time and `result_code`.

### IO

- `GWP89_OP_READ_TEXT_FILE`

`text_in` is the requested path or resource name. The core supplies a writable `text_out` buffer and `text_capacity`. A provider writes at most `text_capacity - 1` bytes, terminates the text, sets `text_length` and `result_code`, and returns `HANDLED`.

The provider may represent stdio, an archive, a virtual file system, ROM data, a network resource, or an engine asset database. The core itself performs no file access.

### Output/event services

HUD, crosshair, scope, draw, projectile, muzzle, casing, trail, mesh, recoil, zoom, reload, active reload, inventory, and event bus receive `GWP89_OP_EMIT_EVENT`.

Projectile-life providers receive `GWP89_OP_PROJECTILE_LIFE`. Health providers receive `GWP89_OP_DAMAGE_REQUEST` when the projectile event carries `GWP89_EVENT_FLAG_HIT_VALID`.

The mutable event is in `packet->event`.

## Priority examples

```text
priority 300  network authority / anti-cheat gates
priority 200  gameplay modifiers / buffs / difficulty
priority 100  engine subsystem provider
priority   0  telemetry / debug observers
priority -50  final diagnostics
```

A provider can combine flags:

```c
return GWP89_PROVIDER_MODIFIED |
       GWP89_PROVIDER_HANDLED |
       GWP89_PROVIDER_STOP;
```

## Cancellation semantics

- Fire validation cancellation occurs before ammo consumption.
- Reload-begin cancellation leaves reload inactive.
- Inventory mutation cancellation aborts the requested mutation.
- Event cancellation suppresses queue insertion and legacy direct callbacks for that event.
- Pose/raycast cancellation should be used carefully; a provider that replaces a result should normally use `HANDLED`.

## Compatibility

`GWP89_Hooks` remains intact. Providers are evaluated first; a matching provider that returns `HANDLED` suppresses the corresponding legacy subsystem hook. With zero gameplay providers, the legacy path is unchanged. File-path loading specifically requires an IO provider; callers without one use `gwp89_load_ini_text()`.
