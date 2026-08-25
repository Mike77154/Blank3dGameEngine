# DDSL2 non-letter keyboard fix

## Symptom

`W/A/S/D` worked through `player.ddsl2`, but replacing them with
`Up/Down/Left/Right` made player movement stop.

## Cause

`blank3d_input_query()` resolved a control name to `key_pc_code` and then cast
that enum value directly to an `input_hook89` keyboard usage index.

Those numeric domains are not identical. A-Z happen to share their USB HID
usage numbers, masking the bug. Navigation keys, top-row digits, modifiers and
several extended controls do not. For example:

- `KEY_PC_UP == 94`
- USB HID Keyboard `Up == 0x52 == 82`

The Win32 input_hook backend correctly writes the Up-arrow state into HID usage
82, while Blank3D was reading slot 94.

## Fix

Blank3D now resolves control names with `input_keys89`, the canonical HID
identity vocabulary shared by the four input heads. The public `key_pc` API is
retained as a polls89 legacy ABI, with explicit `polls_input_keys89` conversion;
it is never treated as a HID usage integer.

Regression coverage now uses actual HID usage values for W, Up, top-row 1,
Left Shift and J.

`player.ddsl2` is also shipped with arrow-only movement as requested.
