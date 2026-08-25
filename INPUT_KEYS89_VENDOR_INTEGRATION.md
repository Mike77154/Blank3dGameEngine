# input_keys89 standalone + Blank3D vendor integration

## Why it exists

Blank3D previously had two symbolic key domains in the input path:
`polls89/key_pc` and `input_hook89` USB HID usages. Letter keys hid the mismatch,
while arrows/digits/modifiers exposed it.

`input_keys89` is now the stateless authority for key identity.

```
text name / alias
       |
       v
 input_keys89
       |
       +---- canonical input_key89 (HID page + usage)
       |
       v
 input_hook89 state -> input_scanner89 temporal state
```

It does not know gameplay actions. The DDSL2 wildcard assignment remains free
to bind any recognized physical key to any user-defined verb/string.

## Standalone contract

- C89 / `-pedantic`
- no malloc/calloc/realloc/free
- no float/double
- no OS backend
- no pressed/held/released state
- no DDSL2/gameplay dependency
- caller-owned buffer for generated key names

## Blank3D integration

`src/blank3d_input.c` resolves keyboard names through
`input_keys89_from_name()` + `input_keys89_keyboard_usage()`. `polls89` also
bridges its legacy `key_pc_code` ABI explicitly to `input_key89`, so all four
heads share the same identity when crossing module boundaries.

The previous fallback chain through `key_pc` and `input_hook_key_from_name()`
has been removed from Blank3D. `polls89` remains an active acquisition head. `key_pc.c` may therefore be
compiled as part of the native provider, but it is no longer used as a second
DDSL key-name authority or cast directly to HID.

Arrow-only `player.ddsl2` therefore reaches the same HID-domain state whether
the native acquisition provider is Win32, Linux or macOS.
