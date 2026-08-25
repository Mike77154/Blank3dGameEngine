# polls89 + input_keys89 bridge

`polls89` keeps its existing `key_pc_code` ABI and platform backends.
`input_keys89` is the canonical identity used when polls89 communicates with
other heads of the Blank3D input stack.

Use `polls_input_keys89.h` for legacy/canonical conversion. Do not cast
`key_pc_code` directly to a HID usage.

Platform backends also expose raw `*_input_key_down(..., input_key89)` helpers
for the canonical polls89 -> input_hook89 adapter. Existing scanner attachment
and logical-button APIs remain supported.
