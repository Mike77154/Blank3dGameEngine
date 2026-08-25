# Howlund Window Maker -> GenWinConfigC89 provider status

The earlier future integration described by this note is now complete.

```text
GenWinConfigC89
      |
      | gwc89_provider
      v
HOWM89 GenWin adapter
      |
      v
Howlund Window Maker 89
      |
      +-- WindowsWindow89 (Blank3D selected backend)
      +-- bundled HOWM89 Win32 backend (optional)
      +-- bundled X11 backend (optional)
      +-- bundled Cocoa backend (optional)
      +-- user supplied backend (optional)
```

The Howlund core remains Protocol89-style fixed-capacity/no-heap/fixed-point.
The integration adds a portable `center_on_monitor` operation and the optional
GenWinConfig adapter. `GenWinConfigC89` itself remains unchanged and has no
HOWM89 dependency.

Blank3D's former direct Win32 creator was converted into `WindowsWindow89`, an
optional HOWM89 backend. `src/blank3d_window_win32.c` is now only the native
handle/HDC interop bridge required by the Win32 OpenGL host.
