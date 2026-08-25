# input_keys89 1.0.0

Small C89, stateless keyboard vocabulary library.

## Responsibility

`input_keys89` owns only the identity of a key:

```
text name / alias <-> input_key89 <-> USB HID page + usage
```

It intentionally does **not** poll devices, track pressed/held/released state,
or assign gameplay actions. That keeps it suitable for wildcard key assignment.

## Examples

- `W` -> keyboard HID `0x1A`
- `1` -> `0x1E`
- `Up`, `ArrowUp`, `CursorUp` -> `0x52`
- `LeftShift`, `lshift`, `Shift` -> `0xE1`
- `F24` -> `0x73`

Names are case-insensitive and separators such as `_`, `-`, and spaces are
ignored while resolving aliases.

## Build

```sh
make
make test
make audit
```

No heap allocation, no float/double, no OS dependency.
