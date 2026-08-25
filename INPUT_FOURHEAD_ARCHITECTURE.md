# Blank3D four-head input architecture

Date: 2026-08-11

Blank3D's input system is intentionally split into four cooperating libraries.
They are not alternatives and none of them is the gameplay action system.
Together they keep scripts and engine logic independent from the operating
system, native keycodes, input provider, and frame-to-frame timing rules.

## The four heads

### 1. input_keys89 - canonical identity

`input_keys89` owns the portable vocabulary for keyboard identity:

- names and aliases (`Up`, `ArrowUp`, `CursorUp`)
- USB HID page + usage
- deterministic name <-> `input_key89` conversion

It does not poll hardware, track state, or know gameplay actions.

Example:

```
Up -> input_key89 0x0752 -> HID keyboard usage 0x52
```

### 2. polls89 - OS/device acquisition

`polls89` owns native acquisition and its existing logical-button bindings.
The platform backends may use Win32, Linux evdev, macOS, or another native
mechanism internally. Native details stay below this boundary.

The four-head path adds raw canonical queries such as:

```
winpckeys_input_key_down(..., input_key89 key)
linuxpckeys_input_key_down(..., input_key89 key)
macpckeys_input_key_down(..., input_key89 key)
```

The legacy `key_pc_code` API remains supported. `polls_input_keys89` is the
explicit bridge between that ABI and canonical HID identity. A `key_pc_code`
is never assumed to be a HID usage by numeric cast.

The active Win/Linux/mac keyboard bindings are fixed-capacity and do not use
malloc/realloc/free.

### 3. input_hook89 - provider normalization and capture

`input_hook89` receives a provider and presents a common HID keyboard snapshot.
Its core does not need to know whether the snapshot came from Win32, evdev,
macOS, SDL, a low-level hook, or `polls89`.

The canonical adapter is:

```
input_hook_backend_polls89
```

It asks the polls provider for raw `input_key89` state and fills the universal
256-usage HID keyboard bitset. Existing direct input_hook providers remain
available; they are not deleted by this integration.

The public input_hook key-name helpers now delegate to `input_keys89` so a
second independent name/key table is not required for the canonical path.

### 4. input_scanner89 - temporal semantics

`input_scanner89` consumes states and owns frame-to-frame interpretation:

- down / hold
- pressed / released
- repeat
- tapped
- long press / long hold
- frame-duration counters

It does not need Win32 VK values, Linux key codes, macOS key codes, or DDSL2
commands. Its keyboard backend keeps its legacy integer API but now also has
`input_key89` bridge helpers.

## Default Blank3D chain

The built-in Blank3D keyboard route is now:

```
DDSL2 key name
      |
      v
input_keys89
portable name / HID identity
      |
      v
polls89 native provider
Win32 / Linux / macOS physical state
      |
      v
input_hook89 polls adapter
universal HID keyboard snapshot
      |
      v
input_scanner89
pressed / hold / released / repeat / tap
      |
      v
DDSL2 wildcard command
```

DDSL2 remains a wildcard key assigner. The input libraries do not define
`move_forward`, `jump`, `reload`, or any other gameplay verb.

For example this remains data, not input-library policy:

```
If key_hold Up then move_forward
```

The right-hand command could be any command registered by the host.

## Blank3D core/platform split

`blank3d_input.h/.c` is platform-neutral. It owns no Win32 provider object and
includes no Win32 input backend header. It consumes only `ihk_backend`.

`blank3d_input_platform.h/.c` owns the native provider. The built-in selector
chooses a polls89 family at compile time:

- Windows 32/64 -> winpckeys
- Linux 32/64 -> linuxpckeys
- macOS 32/64 -> macpckeys
- unsupported target -> no built-in provider; caller may inject an `ihk_backend`

This means the runner can have a convenient native default without making the
input core itself Windows-born.

## Compatibility rules

1. Keep all four libraries. Do not collapse polls, hook, scanner, and keys into
   one module.
2. `input_keys89` is the canonical identity vocabulary between the four heads.
3. Native keycodes stay inside platform acquisition code.
4. Legacy key enums/APIs may remain, but crossing to another head uses an
   explicit bridge rather than numeric casts.
5. Temporal semantics belong to `input_scanner89` even when another provider
   exposes basic edge information for capture purposes.
6. Gameplay meaning remains outside all four input libraries.
7. No heap is required in the active four-head keyboard route.

## Regression that motivated the bridge

The old path could accidentally treat `KEY_PC_UP == 94` as if `94` were the
USB HID usage. The actual HID keyboard usage for Up is `0x52` (82). Letters
hid the bug because several legacy values happened to line up with HID values.

The canonical path now performs:

```
"Up" -> input_keys89 -> HID 07:52 -> polls native mapping
```

rather than:

```
key_pc enum -> numeric cast -> HID
```
