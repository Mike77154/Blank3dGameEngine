# Compatibility location: macos32

The original Howlund package exposed a separate macos32 directory.
Howlund Window Maker 89 keeps the capability but consolidates duplicate implementations:

- Windows 32/64 -> backends/win32/
- Linux X11 32/64 -> backends/x11/
- macOS family -> backends/cocoa/

Use backends/auto/ only if you want compile-time default selection.
The portable core never selects or requires a backend.
