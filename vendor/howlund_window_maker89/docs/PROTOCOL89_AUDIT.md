# Protocol89 audit

Original archive SHA-256:

`a1c1cb62179abdc6cbb1c5fc8a4fcc571ef5c3f60a530d47be67ca6ea3e30e63`

Initial source audit found dynamic allocation in the portable core and built-in backends plus floating-point opacity / scale interfaces. The old public namespace also used `HWND_*` and `hwnd_*` even though `HWND` is a Win32 native type.

The sanitized maintained source uses:

- static fixed-capacity core pool
- static fixed-capacity built-in backend pools
- Q16 opacity / scale API
- no internal render-resolution state
- `HOWM89_*` / `howm89_*` public names

`make protocol-audit` rejects heap APIs, C floating-point keywords, explicit 64-bit integer types, `stdint.h`, and leaked legacy public HWND-like names in maintained C/header sources.

Platform backends necessarily use the native ABI types required by their OS APIs (for example Win32 `HWND`, X11 `Window`, Cocoa `NSWindow`). Those names/types do not cross the portable HOWM89 public header.
