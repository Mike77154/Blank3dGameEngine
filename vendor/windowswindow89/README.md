# WindowsWindow89

Optional Win32 window backend for Howlund Window Maker 89.

This vendor owns only the native Win32 window implementation. It does not own
render resolution, screen scaling, camera dimensions, scene dimensions, or any
Blank3D gameplay policy.

The portable path is:

`GenWinConfigC89 -> HOWM89 adapter -> HOWM89 -> WindowsWindow89`

Applications may omit this vendor entirely and register another HOWM89 backend.
