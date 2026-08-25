# GenWinConfigC89 -> HOWM89 -> WindowsWindow89 integration

## Authority split

Blank3D no longer creates native Win32 windows directly.

```text
Blank3D / GameVerbs / GeneralConfig.cfg
                |
                v
         GenWinConfigC89
                |
          gwc89_provider
                |
                v
 HOWM89 <-> GenWinConfigC89 adapter
                |
                v
      Howlund Window Maker 89
                |
        HOWM89 backend ABI
                |
                v
         WindowsWindow89
                |
                v
              Win32
```

`GenWinConfigC89` does not include HOWM89 and does not know that Howlund exists.
HOWM89 does not know Blank3D. WindowsWindow89 is an optional OS backend.

## Blank3D bridge

`src/blank3d_window_win32.c` is retained as a thin host interop bridge only.
It no longer calls `CreateWindowA`, `RegisterClassA`, `AdjustWindowRect`,
`SetWindowLongA` or `SetWindowPos`.

Its remaining Win32-specific job is renderer interop: obtain the native HWND
from HOWM89 and acquire/release the HDC consumed by the OpenGL backend.

## Adapter

The reusable adapter is located at:

`vendor/howlund_window_maker89/adapters/genwinconfigc89/`

It translates GenWin state into HOWM89 operations for:

- client size
- title
- explicit position
- centered creation on monitor 0 when supported
- visibility
- resizable/decorated/topmost state
- windowed/borderless/fullscreen/borderless-fullscreen modes
- create/apply/destroy/query lifecycle

The adapter has optional after-create and before-destroy hooks so a host may
attach renderer resources without putting renderer policy in either vendor.

## WindowsWindow89

`vendor/windowswindow89/` is the vendorized descendant of Blank3D's former
direct native-window creator. It is now a HOWM89 backend, not engine code.

It owns Win32 window creation, class registration, client-size translation,
style changes, fullscreen transitions and native HWND exposure. It does not
own render resolution, screen scaling, camera size or scene size.

HOWM89's own bundled Win32/X11/Cocoa backends remain present and optional.
Blank3D currently selects WindowsWindow89 because it preserves the historical
Win32 host behavior while keeping that behavior outside the engine.

## Center/monitor note

HOWM89 gained the optional universal operation `center_on_monitor` and
`howm89_window_center_on_monitor()`. WindowsWindow89 currently implements
monitor index 0 (the historical primary-monitor behavior). Other monitor
indices remain a backend capability for a later extension.
