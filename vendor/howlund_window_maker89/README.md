# Howlund Window Maker 89 (HOWM89)

HOWM89 is a renderer-agnostic, engine-agnostic window facade.
Its only job is to request and control native top-level windows through a backend provider.
It does not own rendering resolution, camera size, gameplay room size, input, timing, or a graphics context.

## Public prefix

The old public `HWND_*` / `hwnd_*` namespace was removed because `HWND` is the native Win32 window-handle type.
The public API is now `HOWM89_*` / `howm89_*`.

## Protocol89 core

- ISO C89
- no malloc/calloc/realloc/free
- no float/double
- no explicit 64-bit integer types
- no stdint.h
- fixed-capacity window pool (`HOWM89_MAX_WINDOWS`, default 16)
- Q16 opacity and DPI scale
- caller/provider-owned backend state through `backend_user`

## Backend model

Core build: no backend is required or auto-selected.

Optional built-in backend implementations:

- Win32 (`backends/win32`) -- works for 32-bit and 64-bit Windows builds
- X11 (`backends/x11`) -- works for 32-bit and 64-bit Xlib targets
- Cocoa (`backends/cocoa`) -- Objective-C OS adapter
- auto selector (`backends/auto`) -- optional convenience only

The duplicate architecture directories from the old package are represented under `backends/by_system_backend/` as compatibility documentation.

## Minimal provider usage

```c
HOWM89_BackendVTable backend;
memset(&backend, 0, sizeof(backend));
backend.create_window = my_create;
backend.destroy_window = my_destroy;

howm89_register_backend(&backend, my_backend_state);
window = howm89_window_create("Game", 960, 540);
```

No OS name is present in that path.

## Lazy built-in usage

Build with `HOWM89_BUILD_DEFAULT_BACKEND=ON`, include `backends/auto/howm89_auto_backend.h`, then call:

```c
howm89_register_default_backend();
howm89_library_init();
window = howm89_window_create("Game", 960, 540);
```

The core library itself still remains platform-neutral.

## Deliberately removed from the window core

`internal_size` / logical render size was removed from HOWM89.
That belongs to a video/render configuration layer, not to a universal native-window maker.

## v1.0.1 integration additions

- optional `center_on_monitor` backend operation
- `howm89_window_center_on_monitor()` portable facade
- optional `adapters/genwinconfigc89` bridge

The adapter is not part of the HOWM89 core dependency surface; applications
that do not use GenWinConfigC89 may omit it.
