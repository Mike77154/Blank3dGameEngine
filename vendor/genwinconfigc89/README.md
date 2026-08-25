# GenWinConfigC89

Portable native-window intent/state manager. It owns no OS handle and includes no platform API. A `gwc89_provider` creates/applies/destroys/queries the real window.

Config fields: client size, position/auto-position, centering, resizable, decorated, visible, topmost, play mode, monitor index, title.

Modes: windowed, fullscreen, borderless, borderless fullscreen.

Designed to accept Win32 today and Howlund/SDL/X11/Cocoa/fake-host providers later.

Protocol89: ISO C89, fixed/caller-owned state, no heap, no float/double, no explicit 64-bit types.
